// =============================================================================
//  課程 3.3：std::jthread (C++20) — 第 3 部分：stop_token 協作式取消
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<stop_token>（std::stop_token / std::stop_source / std::stop_callback）
//           <thread>（std::jthread）
//   標準版本：C++20
//
//     bool stop_token::stop_requested()  const noexcept;  // 是否已被要求停止
//     bool stop_token::stop_possible()   const noexcept;  // 是否還可能被要求停止
//     bool jthread::request_stop()             noexcept;  // 發出停止請求
//
//   複雜度：stop_requested() 是一次 atomic load（relaxed 等級的成本），
//           在迴圈中呼叫幾乎沒有負擔。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼是「協作式」而不是「強制」取消】
//   很多語言早期都提供過強制中止執行緒的 API（Java 的 Thread.stop()、
//   POSIX 的 pthread_cancel()），而且【全部都後悔了】。
//   原因是：強制中止會在任意一個指令邊界把執行緒砍掉，於是
//     * 它持有的 mutex 永遠不會被解鎖 → 其他執行緒全部卡死；
//     * 它配置到一半的記憶體不會被釋放 → 洩漏；
//     * 它的物件解構子不會執行 → 檔案沒 close、交易沒 rollback；
//     * 資料結構停在「改到一半」的中間狀態 → 不變式被破壞。
//   Java 在 1.2 就把 Thread.stop() 標成 deprecated，原因正是如此。
//
//   C++20 因此只提供「協作式」取消：request_stop() 只是把一個
//   atomic<bool> 設成 true，執行緒必須【自願】在安全的地方檢查並收工。
//   這樣退出點永遠落在你自己選的位置，不變式不會被破壞。
//
// 【2. 三個型別的分工】
//   * std::stop_source —— 發送端。持有共享停止狀態的擁有權，可 request_stop()。
//   * std::stop_token  —— 接收端。唯讀，只能查詢 stop_requested()。
//   * std::stop_callback —— 註冊在 token 上的回調，停止時自動觸發（見第 5 部分）。
//   三者共用同一塊引用計數的堆積狀態，語意上很像 shared_ptr 的 control block。
//   把 token（唯讀）傳給工作執行緒、把 source（可寫）留在控制端，
//   權責分離得非常乾淨。
//
// 【3. jthread 如何自動注入 stop_token】
//   jthread 的建構子會用 std::is_invocable_v 做編譯期判斷：
//     若 f 可以用 (stop_token, args...) 呼叫 → 就把自己的 token 當第一個引數傳進去；
//     否則 → 直接用 (args...) 呼叫。
//   所以你只要在 lambda 的參數列寫上 std::stop_token，就會自動拿到它，
//   不需要任何註冊動作。這是 C++20 少數「看起來像魔法、其實只是 SFINAE」的設計。
//
// 【4. 檢查頻率的取捨】
//   stop_requested() 要放在迴圈的哪裡、多久檢查一次，是設計上的權衡：
//     * 檢查太稀疏 → 取消延遲高，解構子要等很久（本檔最壞要等 200ms）。
//     * 檢查太密集 → 沒有實際成本問題（atomic load 很便宜），
//                    但若把它塞進真正的熱迴圈內層，可能妨礙向量化。
//   實務準則：在「每個工作單元的邊界」檢查一次，而不是每個指令都檢查。
//   若執行緒是阻塞在 I/O 或 condition_variable 上，光檢查旗標沒用 ——
//   要用 std::condition_variable_any 的 wait(lock, token, pred) 多載
//   （C++20 新增），它能被 request_stop() 喚醒。
//
// 【概念補充 Concept Deep Dive】
//   * 共享狀態的記憶體序：request_stop() 對停止旗標的寫入與
//     stop_requested() 的讀取之間，標準保證有適當的同步關係，
//     所以你不需要自己加 memory_order。標準要求 request_stop()
//     與「觀察到 true 的 stop_requested()」之間存在 happens-before，
//     這確保停止前寫入的其他資料對觀察端可見。
//
//   * stop_possible() 的用途：若一個 token 是預設建構的（沒有關聯 source），
//     或所有 source 都已銷毀且未曾請求停止，stop_possible() 回傳 false。
//     這讓你能寫出「這個 token 永遠不會被觸發，不必浪費資源監聽」的最佳化。
//
//   * 為什麼 request_stop() 回傳 bool：回傳「這次呼叫是否真的造成狀態改變」。
//     第一次呼叫回 true，之後重複呼叫回 false（狀態已是 requested）。
//     這讓「只有第一個請求者需要做額外收尾」這種邏輯可以直接寫。
//
//   * 停止請求【不可逆】。沒有 reset()、沒有 clear_stop()。
//     設計上刻意如此：可撤銷的取消會產生大量競態邊界情況。
//     需要重複啟停的話，正確做法是建立新的 stop_source。
//
// 【注意事項 Pay Attention】
//   1. 執行緒若沒有檢查 stop_token，request_stop() 就完全無效 ——
//      它不會強制中止任何東西。這是特性，不是缺陷。
//   2. 阻塞中的執行緒（sleep、read()、cv.wait()）不會被停止請求喚醒。
//      本檔的 sleep_for(200ms) 就是這種情況：請求送出後，最壞要等
//      這一輪 sleep 睡完才會被看到。要即時響應請改用
//      condition_variable_any::wait(lock, token, pred)。
//   3. 本檔輸出的「工作中... N」次數與時間相關，每次執行都可能不同。
//   4. 本檔必須用 -std=c++20（已用 -pedantic-errors 實測驗證）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】協作式取消 stop_token
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 C++20 只提供協作式取消，不做強制中止執行緒？
//     答：強制中止會在任意指令邊界砍掉執行緒，導致它持有的 mutex 永遠不解鎖、
//         配置的資源不釋放、解構子不執行、資料結構停在改到一半的狀態。
//         Java 的 Thread.stop() 與 POSIX 的 pthread_cancel() 都因此被視為
//         設計失誤。協作式取消讓退出點落在程式設計者選定的安全位置。
//     追問：那執行緒卡在阻塞式 I/O 上怎麼辦？
//         → stop_token 幫不上忙。要嘛用有 timeout 的 I/O 並在迴圈中檢查，
//           要嘛用 condition_variable_any 的 token 多載，
//           要嘛在 Linux 上用 eventfd/pipe 之類的喚醒管道自己實作。
//
// 🔥 Q2. jthread 怎麼知道要不要把 stop_token 傳給我的 lambda？
//     答：建構子在編譯期用 is_invocable 判斷：若 callable 可以用
//         (stop_token, args...) 呼叫，就把 token 當第一個引數傳入；
//         否則就用 (args...) 呼叫。純編譯期分派，沒有執行期成本。
//     追問：如果我的 lambda 第一個參數剛好是別的型別但可以從 stop_token 隱式
//         轉換過去呢？→ 那就會被判定為「可呼叫」而傳入 token，
//           這是實務上少見但確實存在的誤判陷阱，所以參數型別要寫精確。
//
// ⚠️ 陷阱. 呼叫 request_stop() 之後，執行緒是不是就「立刻」停了？
//     答：不是。request_stop() 只是把共享狀態的旗標設為 true 就回傳了，
//         它不等待、也不中斷任何東西。執行緒要等到自己下一次執行到
//         stop_requested() 才會發現。本檔中執行緒正在 sleep_for(200ms)，
//         所以最壞情況會延遲將近 200ms 才真正結束。
//     為什麼會錯：request_stop 這個名字讓人以為是同步的「停止操作」，
//         但它是非同步的「張貼一則請求」。真正等待結束的是 join()
//         （或 jthread 的解構子），這兩件事必須分開理解。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔從缺
//   LeetCode 的並行題（1114 Print in Order、1115 Print FooBar Alternately、
//   1116 Print Zero Even Odd、1117 Building H2O、1195 Fizz Buzz Multithreaded）
//   考的都是「用同步原語強制執行順序」，每個執行緒都是跑固定次數就自然結束，
//   完全沒有「提早取消一個長時間執行的工作」這個需求。
//   stop_token 在那些題目裡派不上用場，硬套只會模糊焦點，故從缺。

#include <atomic>
#include <chrono>
#include <iostream>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

// -----------------------------------------------------------------------------
// 【基本示範】接收 stop_token 的工作迴圈
// -----------------------------------------------------------------------------
void demoBasicCancel() {
    std::jthread jt([](std::stop_token stoken) {
        int count = 0;
        while (!stoken.stop_requested()) {
            std::cout << "工作中... " << ++count << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        std::cout << "收到停止請求，結束" << std::endl;
    });

    // 讓執行緒跑一下
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 請求停止（非同步：只設旗標就返回，不等待）
    jt.request_stop();

    // jt 解構時會等待執行緒結束
}

// -----------------------------------------------------------------------------
// 【對照示範】request_stop() 的回傳值：只有第一次會回 true
// -----------------------------------------------------------------------------
void demoRequestStopReturnValue() {
    std::jthread jt([](std::stop_token st) {
        while (!st.stop_requested()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    std::cout << "  第一次 request_stop() 回傳: " << std::boolalpha
              << jt.request_stop() << "（狀態確實被改變）" << std::endl;
    std::cout << "  第二次 request_stop() 回傳: " << std::boolalpha
              << jt.request_stop() << "（已經是 requested，沒有再改變）" << std::endl;
    std::cout << "  停止請求不可逆，沒有 reset() 這種 API" << std::endl;
}

// -----------------------------------------------------------------------------
// 【對照示範】檢查粒度決定取消延遲
//   粗粒度：只在外層迴圈檢查 → 必須跑完一整個大工作單元才會發現
//   細粒度：內層迴圈也檢查   → 反應快，但退出點變多，要確保每個都安全
// -----------------------------------------------------------------------------
void demoGranularity() {
    std::atomic<int> coarseUnits{0};

    std::jthread coarse([&coarseUnits](std::stop_token st) {
        while (!st.stop_requested()) {          // 只在工作單元邊界檢查
            for (int i = 0; i < 5; ++i) {       // 一個「工作單元」= 5 個子步驟
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
            ++coarseUnits;
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    auto t0 = std::chrono::steady_clock::now();
    coarse.request_stop();
    coarse.join();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - t0).count();

    std::cout << "  粗粒度檢查：完成 " << coarseUnits.load() << " 個工作單元" << std::endl;
    std::cout << "  從 request_stop() 到真正結束約 " << elapsed
              << " ms（等待當前工作單元跑完；每次執行都不同）" << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】可取消的檔案索引器（背景掃描 + 隨時中止）
//   情境：IDE 或桌面搜尋工具在背景建立檔案索引。使用者可能隨時關閉專案，
//         此時必須立刻停止掃描，而且【已經寫進索引的部分必須保持一致】——
//         不能停在「寫了一半的索引項目」上。
//   為何用 stop_token：檢查點刻意放在「處理完一個完整檔案之後」，
//         保證索引永遠是完整項目的集合，不會有半筆資料。
//         這正是協作式取消勝過強制中止的地方。
// -----------------------------------------------------------------------------
class FileIndexer {
public:
    void run(std::stop_token st, const std::vector<std::string>& files) {
        for (const auto& file : files) {
            // 檢查點：放在「檔案邊界」，而不是寫入索引的中途
            if (st.stop_requested()) {
                std::cout << "  [indexer] 收到取消，已索引 " << indexed_.size()
                          << " 個檔案後安全停止" << std::endl;
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(80));  // 模擬解析
            indexed_.push_back(file);
            std::cout << "  [indexer] 已索引: " << file << std::endl;
        }
        std::cout << "  [indexer] 全部索引完成" << std::endl;
    }

    std::size_t indexedCount() const { return indexed_.size(); }

private:
    std::vector<std::string> indexed_;
};

int main() {
    std::cout << "=== 基本示範：stop_token 工作迴圈 ===" << std::endl;
    demoBasicCancel();

    std::cout << "\n=== request_stop() 的回傳值語意 ===" << std::endl;
    demoRequestStopReturnValue();

    std::cout << "\n=== 檢查粒度與取消延遲 ===" << std::endl;
    demoGranularity();

    std::cout << "\n=== 日常實務：可取消的檔案索引器 ===" << std::endl;
    {
        FileIndexer indexer;
        std::vector<std::string> files = {"main.cpp",   "utils.hpp", "parser.cc",
                                          "network.cc", "db.cc",     "ui.cc"};
        std::jthread scanner([&indexer, &files](std::stop_token st) {
            indexer.run(st, files);
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        std::cout << "  main: 使用者關閉專案，請求取消索引" << std::endl;
        scanner.request_stop();
    }  // scanner 解構 → join，確保索引器完全停止後才離開作用域
    std::cout << "  main: 索引器已安全回收" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra -pthread "課程 3.3：std jthread (C++20)3.cpp" -o jthread3
//   注意：本檔【必須】用 -std=c++20（std::jthread / std::stop_token 是 C++20 新增，
//   已用 -std=c++17 -pedantic-errors 實測確認編譯失敗）。

// 註:
//   ⚠️ 以下所有「次數」與「毫秒數」都取決於執行緒排程，【每次執行都不同】。
//   唯一穩定的是結構：停止請求送出後，執行緒會在下一個檢查點才結束。

//   從 request_stop() 到真正結束約 51 ms（等待當前工作單元跑完；每次執行都不同）

// === 預期輸出 ===
// === 基本示範：stop_token 工作迴圈 ===
// 工作中... 1
// 工作中... 2
// 工作中... 3
// 工作中... 4
// 工作中... 5
// 收到停止請求，結束
//
// === request_stop() 的回傳值語意 ===
//   第一次 request_stop() 回傳: true（狀態確實被改變）
//   第二次 request_stop() 回傳: false（已經是 requested，沒有再改變）
//   停止請求不可逆，沒有 reset() 這種 API
//
// === 檢查粒度與取消延遲 ===
//   粗粒度檢查：完成 3 個工作單元
//
// === 日常實務：可取消的檔案索引器 ===
//   [indexer] 已索引: main.cpp
//   [indexer] 已索引: utils.hpp
//   [indexer] 已索引: parser.cc
//   main: 使用者關閉專案，請求取消索引
//   [indexer] 已索引: network.cc
//   [indexer] 收到取消，已索引 4 個檔案後安全停止
//   main: 索引器已安全回收
