// =============================================================================
//  課程 3.4：執行緒例外處理 — 第 3 部分：std::exception_ptr 跨執行緒傳遞例外
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<exception>（exception_ptr / current_exception / rethrow_exception）
//           <thread>、<stdexcept>
//   標準版本：C++11
//
//     namespace std {
//         using exception_ptr = /* 未指定的、可複製的指標型別 */;
//         exception_ptr current_exception() noexcept;   // 只在 catch 內有意義
//         [[noreturn]] void rethrow_exception(exception_ptr p);
//         template<class E> exception_ptr make_exception_ptr(E e) noexcept;
//     }
//
//   複雜度：current_exception() 通常只是對例外物件的引用計數 +1（O(1)）；
//           標準允許實作改為複製例外物件，但主流實作（libstdc++/libc++）
//           採用引用計數，不複製。
//
// 【詳細解釋 Explanation】
//
// 【1. exception_ptr 解決的核心問題】
//   第 2 部分說過：例外無法自動跨執行緒傳播，因為兩條執行緒的堆疊互相獨立。
//   要傳過去，必須有個東西能「把例外物件裝起來、搬到另一條執行緒、再拆開拋出」。
//   exception_ptr 就是這個容器。它的三步流程是：
//     (1) 在 worker 的 catch 區塊裡：p = std::current_exception();   ← 裝箱
//     (2) 透過任何共享機制把 p 傳給另一條執行緒（本檔用全域變數）
//     (3) 在接收端：std::rethrow_exception(p);                       ← 拆箱重拋
//   重拋之後，例外就在【接收端的堆疊】上正常傳播，可以用一般的 catch 接。
//
// 【2. 為什麼型別要「未指定」】
//   標準刻意不規定 exception_ptr 的具體型別（只說它可複製、可比較、
//   可用 nullptr 初始化）。這是為了讓實作能選擇最有效率的策略：
//   libstdc++ 讓它指向 Itanium ABI 的 __cxa_exception 控制區塊並操作
//   其引用計數，所以複製 exception_ptr 極便宜，且【不需要知道例外的靜態型別】。
//   這點很關鍵：worker 拋的可能是任何型別，接收端在編譯期並不知道是什麼，
//   但 exception_ptr 照樣能原封不動搬過去 —— 型別資訊藏在 RTTI 裡，
//   重拋時才被還原。
//
// 【3. current_exception() 的使用限制】
//   它只在「正在處理例外」時才回傳非空值，也就是必須在 catch 區塊
//   （或其直接呼叫的函式）中呼叫。在沒有活躍例外時呼叫，回傳空的
//   exception_ptr（其行為等同 nullptr，可用 if (p) 檢查）。
//   常見錯誤是在 catch 區塊【外面】呼叫它，然後困惑為什麼拿到空值。
//
// 【4. rethrow_exception 的兩個特性】
//   * 標記為 [[noreturn]]：它一定以拋出例外結束，絕不正常返回。
//     所以它後面的程式碼是不可達的。
//   * 傳入空的 exception_ptr 是【未定義行為】。務必先檢查 if (p)。
//     本檔的 main 就是先 if (threadException) 才 rethrow。
//
// 【5. 生命週期：為什麼例外物件不會失效】
//   一般而言，例外物件在 catch 區塊結束時就被銷毀。
//   但只要有任何 exception_ptr 指著它，引用計數就不為零，
//   物件會一直存活到最後一個 exception_ptr 消失為止。
//   所以 worker 執行緒早就結束了、它的堆疊也已回收，
//   main 仍能安全地把例外拆箱重拋 —— 例外物件根本不在那條堆疊上，
//   而是在專屬的例外儲存區（__cxa_allocate_exception 配置的堆積記憶體）。
//
// 【概念補充 Concept Deep Dive】
//   * exception_ptr 本身的執行緒安全性：
//     標準保證對【同一個】例外物件的引用計數操作是執行緒安全的
//     （多條執行緒各自持有指向同一例外的 exception_ptr 是安全的）。
//     但 exception_ptr 這個【變數】本身不是 atomic ——
//     多條執行緒同時「寫入同一個 exception_ptr 變數」仍然是 data race。
//     本檔的全域 threadException 只被一條 worker 寫入、
//     且 main 在 join() 之後才讀（join 建立 happens-before 關係），
//     所以是安全的。多條 worker 的正確做法見下方 demoMultipleWorkers()：
//     【每條執行緒一個獨立的 exception_ptr】，完全避開共享寫入。
//
//   * join() 提供的同步保證：標準規定 thread 的完成
//     synchronizes-with join() 的返回。這建立了 happens-before 關係，
//     所以 worker 在結束前對 threadException 的寫入，
//     對 join() 之後的 main 是可見的 —— 不需要額外加 mutex 或 atomic。
//     這是本檔能安全使用普通全域變數的唯一理由，不可推廣到沒有 join 的情況。
//
//   * make_exception_ptr(e)：不需要真的 throw 就能造出 exception_ptr。
//     內部通常是 try { throw e; } catch (...) { return current_exception(); }。
//     注意它會【切片（slicing）】—— 參數是傳值的，
//     傳衍生類別進去會被切成宣告的型別。要保留動態型別請用 throw + current_exception。
//
//   * 與 std::future 的關係：std::async / std::promise 內部正是用
//     exception_ptr 儲存例外的。future::get() 做的就是
//     「若共享狀態存的是例外，就 rethrow_exception」。
//     所以第 5 部分的 future 寫法是本檔機制的高階包裝，不是另一套機制。
//
// 【注意事項 Pay Attention】
//   1. current_exception() 必須在 catch 區塊內呼叫，否則回傳空值。
//   2. 對空的 exception_ptr 呼叫 rethrow_exception() 是【未定義行為】。
//      務必先 if (p) 檢查。
//   3. exception_ptr 變數本身不是 atomic。多條執行緒寫入同一個變數是 data race；
//      正確做法是每條執行緒一個，或用 mutex 保護收集容器。
//   4. rethrow_exception 是 [[noreturn]]，其後的程式碼不可達。
//   5. make_exception_ptr 傳值會造成物件切片，要保留動態型別請用 throw。
//   6. 例外物件只要還有 exception_ptr 指著就不會被銷毀 —— 反過來說，
//      長期持有 exception_ptr 會讓例外物件一直佔著記憶體。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::exception_ptr
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 請說明用 exception_ptr 把例外從工作執行緒傳回主執行緒的完整流程。
//     答：三步。(1) worker 用 try/catch(...) 包住工作，在 catch 內呼叫
//         std::current_exception() 取得 exception_ptr；(2) 透過共享機制
//         （全域變數、成員變數、或收集用的 vector）把它交給主執行緒；
//         (3) 主執行緒 join() 後檢查非空，呼叫 std::rethrow_exception(p)，
//         例外就在主執行緒的堆疊上重新拋出，用一般 catch 即可接住。
//     追問：worker 的堆疊都已經回收了，例外物件為什麼還活著？
//         → 因為例外物件從來就不在堆疊上。它由 __cxa_allocate_exception
//           配置在專屬的例外儲存區，exception_ptr 持有它的引用計數，
//           只要計數不為零就不會被釋放。
//
// 🔥 Q2. 本檔用普通全域變數傳遞 exception_ptr，沒加 mutex 也沒用 atomic，
//        為什麼不是 data race？
//     答：因為只有一條 worker 寫入，而 main 是在 t.join() 【之後】才讀取。
//         標準規定執行緒的完成 synchronizes-with join() 的返回，
//         這建立了 happens-before 關係，寫入對 join 之後的讀取必然可見。
//     追問：那如果有十條 worker 呢？
//         → 十條同時寫同一個變數就是 data race（UB）。正確做法是
//           每條 worker 配一個獨立的 exception_ptr（例如
//           std::vector<std::exception_ptr>，各寫各的索引），
//           或用 mutex 保護一個共享的收集容器。
//
// ⚠️ 陷阱. 為什麼下面這段拿到的 exception_ptr 是空的？
//         std::exception_ptr p;
//         try { risky(); } catch (...) { }
//         p = std::current_exception();     // ← 在 catch 區塊【外面】
//     答：current_exception() 只在「有活躍例外正在被處理」時才回傳非空值。
//         catch 區塊一結束，例外處理就完成了，此時呼叫只會拿到空的
//         exception_ptr。必須把它移進 catch 區塊【內部】。
//     為什麼會錯：把 current_exception() 想成「查詢最近一次發生的例外」的
//         全域查詢函式。它其實是「取得【當前正在處理中】的那個例外」，
//         語意上綁在 catch 區塊的動態範圍內，出了那個範圍就沒有當前例外了。
//         更危險的是：這個錯誤【不會有任何警告】，程式會安靜地認為
//         「沒有錯誤發生」，錯誤被完全吞掉。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔從缺
//   本檔主題是跨執行緒的錯誤傳遞機制。LeetCode 並行題
//   （1114/1115/1116/1117/1195）考的是同步原語的順序協調，
//   題目框架保證輸入合法、解法也不需要拋出例外，
//   完全沒有「把失敗回報給另一條執行緒」這個維度。故從缺。

#include <exception>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

std::exception_ptr threadException = nullptr;

// -----------------------------------------------------------------------------
// 【基本示範】worker 捕獲並裝箱，main 拆箱重拋
// -----------------------------------------------------------------------------
void worker() {
    try {
        throw std::runtime_error("執行緒內的錯誤！");
    } catch (...) {
        // 捕獲並儲存例外（必須在 catch 區塊【內部】呼叫）
        threadException = std::current_exception();
    }
}

// -----------------------------------------------------------------------------
// 【對照示範】current_exception() 在 catch 區塊外會拿到空值
// -----------------------------------------------------------------------------
void demoOutsideCatch() {
    std::exception_ptr inside = nullptr;
    std::exception_ptr outside = nullptr;

    try {
        throw std::logic_error("測試用例外");
    } catch (...) {
        inside = std::current_exception();      // ✓ 正確位置
    }
    outside = std::current_exception();          // ✗ 已離開 catch，拿到空值

    std::cout << "  catch 區塊【內】取得的 ptr 是否非空: " << std::boolalpha
              << static_cast<bool>(inside) << std::endl;
    std::cout << "  catch 區塊【外】取得的 ptr 是否非空: " << std::boolalpha
              << static_cast<bool>(outside)
              << "  ← 錯誤會被安靜吞掉，這是最危險的地方" << std::endl;
}

// -----------------------------------------------------------------------------
// 【對照示範】多條執行緒的正確做法：每條一個獨立的 exception_ptr
//   ✗ 錯誤做法：十條 worker 全部寫同一個全域 exception_ptr → data race（UB）
//   ✓ 正確做法：vector<exception_ptr> 各寫各的索引 → 沒有共享寫入
// -----------------------------------------------------------------------------
void demoMultipleWorkers() {
    const int N = 4;
    std::vector<std::exception_ptr> errors(N);   // 每條執行緒獨佔一格
    std::vector<std::thread> threads;

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([i, &errors]() {
            try {
                if (i % 2 == 1) {
                    throw std::runtime_error("worker " + std::to_string(i) +
                                             " 失敗");
                }
            } catch (...) {
                errors[i] = std::current_exception();   // 只寫自己那一格
            }
        });
    }
    for (auto& t : threads) t.join();   // join 建立 happens-before，讀取才安全

    for (int i = 0; i < N; ++i) {
        if (!errors[i]) {
            std::cout << "  worker " << i << ": 成功" << std::endl;
            continue;
        }
        try {
            std::rethrow_exception(errors[i]);
        } catch (const std::exception& e) {
            std::cout << "  worker " << i << ": 失敗 → " << e.what() << std::endl;
        }
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例】平行檔案匯入：收集所有分片的錯誤，一次回報
//   情境：把一個大型 CSV 切成數個分片，每條執行緒負責一片解析入庫。
//         任一分片失敗都必須讓整批匯入失敗並回報【具體原因】，
//         但不能因為第一片失敗就中斷其他片 —— 要先讓全部跑完，
//         才能一次告訴使用者「哪幾片壞了、分別是什麼問題」。
//   為何用 exception_ptr：這是「多個工作單元各自可能失敗、
//         需要把型別完整的錯誤帶回呼叫端」的標準解法。
//         若只回傳 bool 或錯誤碼，就丟失了例外型別與訊息；
//         用 exception_ptr 則能原封不動保留，呼叫端還能分型別處理。
//   注意：每條執行緒寫自己的 slot，完全沒有共享寫入，不需要 mutex。
//         這裡的 mutex 只用來保護輸出，與錯誤收集無關。
// -----------------------------------------------------------------------------
struct ShardError : std::runtime_error {
    int shard;
    ShardError(int s, const std::string& msg)
        : std::runtime_error(msg), shard(s) {}
};

class CsvImporter {
public:
    // 回傳每個分片的錯誤（nullptr 代表該片成功）
    std::vector<std::exception_ptr> importAll(const std::vector<std::string>& shards) {
        std::vector<std::exception_ptr> errors(shards.size());
        std::vector<std::thread> threads;
        std::mutex coutMtx;   // 只保護輸出，不保護 errors（各寫各的）

        for (std::size_t i = 0; i < shards.size(); ++i) {
            threads.emplace_back([this, i, &shards, &errors, &coutMtx]() {
                try {
                    parseShard(static_cast<int>(i), shards[i]);
                    std::lock_guard<std::mutex> lock(coutMtx);
                    std::cout << "  [shard " << i << "] 匯入成功" << std::endl;
                } catch (...) {
                    errors[i] = std::current_exception();   // 裝箱，交給呼叫端
                    std::lock_guard<std::mutex> lock(coutMtx);
                    std::cout << "  [shard " << i << "] 匯入失敗（錯誤已保存）"
                              << std::endl;
                }
            });
        }
        for (auto& t : threads) t.join();
        return errors;
    }

private:
    void parseShard(int idx, const std::string& content) {
        if (content.find(',') == std::string::npos) {
            throw ShardError(idx, "分片 " + std::to_string(idx) + " 欄位分隔符缺失");
        }
        if (content.size() < 5) {
            throw std::length_error("分片 " + std::to_string(idx) + " 內容過短");
        }
    }
};

int main() {
    std::cout << "=== 基本示範：exception_ptr 跨執行緒傳遞 ===" << std::endl;
    {
        std::thread t(worker);
        t.join();

        // 檢查是否有例外（rethrow 空的 exception_ptr 是 UB，必須先檢查）
        if (threadException) {
            try {
                std::rethrow_exception(threadException);
            } catch (const std::exception& e) {
                std::cout << "主執行緒捕獲: " << e.what() << std::endl;
            }
        }
    }

    std::cout << "\n=== 陷阱：current_exception() 的呼叫位置 ===" << std::endl;
    demoOutsideCatch();

    std::cout << "\n=== 多執行緒：每條一個獨立的 exception_ptr ===" << std::endl;
    demoMultipleWorkers();

    std::cout << "\n=== 日常實務：平行 CSV 匯入的錯誤收集 ===" << std::endl;
    {
        CsvImporter importer;
        auto errors = importer.importAll({
            "1001,ACME,3",      // OK
            "1002-GLOBEX-7",    // 缺分隔符
            "1003,INITECH,5",   // OK
            "x,y"               // 太短
        });

        std::cout << "  ---- 匯總報告 ----" << std::endl;
        int failed = 0;
        for (std::size_t i = 0; i < errors.size(); ++i) {
            if (!errors[i]) continue;
            ++failed;
            try {
                std::rethrow_exception(errors[i]);
            } catch (const ShardError& e) {
                // exception_ptr 完整保留了動態型別，這裡才能取到 e.shard
                std::cout << "  分片 " << e.shard << " → ShardError: " << e.what()
                          << std::endl;
            } catch (const std::exception& e) {
                std::cout << "  分片 " << i << " → " << e.what() << std::endl;
            }
        }
        std::cout << "  共 " << errors.size() << " 片，失敗 " << failed << " 片"
                  << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.4：執行緒例外處理3.cpp" -o except3
//   本檔只用到 C++11 起就有的功能，以 -std=c++17 -pedantic-errors 驗證通過。

// 註:
//   ⚠️ 「日常實務」與「多執行緒」兩段中，各分片/各 worker 的【輸出先後順序】
//   取決於執行緒排程，【每次執行都不同】；但匯總報告是在全部 join() 之後
//   才依索引順序輸出的，所以報告內容是確定的。

// === 預期輸出 ===
// === 基本示範：exception_ptr 跨執行緒傳遞 ===
// 主執行緒捕獲: 執行緒內的錯誤！
//
// === 陷阱：current_exception() 的呼叫位置 ===
//   catch 區塊【內】取得的 ptr 是否非空: true
//   catch 區塊【外】取得的 ptr 是否非空: false  ← 錯誤會被安靜吞掉，這是最危險的地方
//
// === 多執行緒：每條一個獨立的 exception_ptr ===
//   worker 0: 成功
//   worker 1: 失敗 → worker 1 失敗
//   worker 2: 成功
//   worker 3: 失敗 → worker 3 失敗
//
// === 日常實務：平行 CSV 匯入的錯誤收集 ===
//   [shard 2] 匯入成功
//   [shard 0] 匯入成功
//   [shard 1] 匯入失敗（錯誤已保存）
//   [shard 3] 匯入失敗（錯誤已保存）
//   ---- 匯總報告 ----
//   分片 1 → ShardError: 分片 1 欄位分隔符缺失
//   分片 3 → 分片 3 內容過短
//   共 4 片，失敗 2 片
