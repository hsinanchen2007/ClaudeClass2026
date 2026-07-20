// =============================================================================
//  課程 3.3：std::jthread (C++20) — 第 1 部分：自動 join 的 RAII 執行緒
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<thread>（std::jthread 本身）、<stop_token>（stop_token/stop_source）
//   標準版本：C++20（P0660R10）
//
//     class jthread {
//         jthread() noexcept;
//         template<class F, class... Args> explicit jthread(F&& f, Args&&... args);
//         ~jthread();                       // 若 joinable()：request_stop() 然後 join()
//         jthread(jthread&&) noexcept;      // 可移動
//         jthread(const jthread&) = delete; // 不可複製
//     };
//
//   複雜度：建構 = 一次 OS 執行緒建立（Linux 上是 clone(2)，微秒等級）；
//           解構 = 一次 join，成本取決於被等待的執行緒還要跑多久。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼要有 jthread：std::thread 的解構子是個地雷】
//   std::thread 的解構子邏輯是「如果還 joinable()，就呼叫 std::terminate()」。
//   這是 C++11 刻意的設計決定，理由是：解構時只有兩個選擇，
//     (a) 自動 join  → 解構子可能阻塞任意久，違反「解構要快」的直覺；
//     (b) 自動 detach → 執行緒可能存取已經銷毀的區域變數，變成 dangling。
//   標準委員會認為「兩個都可能錯，不如直接讓程式死得很大聲」，
//   於是選了 terminate()。這讓「忘記 join」從一個安靜的 bug 變成必定崩潰。
//
//   jthread 重新審視了這個取捨：它選擇 (a) 自動 join，但同時提供
//   協作式取消（stop_token），讓「解構會阻塞很久」這個缺點有解 ——
//   解構子會先 request_stop() 通知執行緒收工，再 join() 等它收完。
//
// 【2. 解構子的兩個步驟，順序不能顛倒】
//   ~jthread() 的規定行為是：
//       if (joinable()) { request_stop(); join(); }
//   先送出停止請求、再等待，這個順序是關鍵。如果反過來先 join()，
//   一個「跑無窮迴圈直到被要求停止」的執行緒永遠不會結束，解構子就卡死了。
//   本檔的 lambda 沒有接收 stop_token，跑完就結束，所以 request_stop()
//   實際上沒有作用 —— 但解構子仍然照規定送出請求，這不是錯誤。
//
// 【3. 不接 stop_token 也完全合法】
//   jthread 的建構子會做編譯期判斷：如果 callable 的第一個參數可以接受
//   std::stop_token，就把 token 當第一個引數傳進去；否則就照原樣呼叫。
//   所以下面這兩種寫法都對：
//       std::jthread a([]{ ... });                       // 不需要取消
//       std::jthread b([](std::stop_token st){ ... });   // 需要協作式取消
//   本檔示範前者：最單純的「我只是要一個會自己收尾的執行緒」。
//
// 【4. RAII 帶來的例外安全】
//   jthread 是一個 RAII 型別：資源（OS 執行緒）在建構時取得、在解構時釋放。
//   因為 C++ 保證「離開作用域時（含 stack unwinding）自動呼叫解構子」，
//   所以就算 main 中途拋出例外，jthread 一樣會被 join，不會漏掉。
//   這正是 std::thread 做不到、必須自己寫 ThreadGuard 才能補上的部分。
//
// 【概念補充 Concept Deep Dive】
//   * jthread 的物件佈局：libstdc++ 的實作是
//       std::thread _M_thread;        // 底層執行緒把手
//       std::stop_source _M_stop_source;  // 共享的停止狀態（引用計數的堆積物件）
//     所以 sizeof(std::jthread) 通常大於 sizeof(std::thread)。
//     本機（x86-64 Linux / libstdc++ 15.2）實測值由程式印出，
//     這是【實作定義】的數值，不同標準庫可能不同。
//
//   * stop_source 內含一個引用計數的共享狀態（類似 shared_ptr 的 control block），
//     所以 get_stop_token() 取得的 token 即使在 jthread 死掉之後仍然有效，
//     只是它的 stop_requested() 會維持在最後的狀態。
//
//   * 「自動 join」不是編譯器魔法，就是解構子裡老實呼叫 pthread_join()。
//     成本與手寫 t.join() 完全一樣，jthread 沒有額外的執行期開銷。
//
// 【注意事項 Pay Attention】
//   1. jthread 解構會【阻塞】直到執行緒結束。若執行緒跑很久，離開作用域就會卡住。
//      解法是讓執行緒接收 stop_token 並定期檢查（見本課第 3、4、5 部分）。
//   2. 成員變數若含 jthread，請把它宣告在【最後】。解構順序與宣告順序相反，
//      這樣 jthread 會最先被解構（先 join），確保執行緒不會存取到已銷毀的其他成員。
//   3. jthread 不可複製、只可移動。被移動走的 jthread 變成 non-joinable，
//      解構時什麼都不做。
//   4. 若對 jthread 呼叫 detach()，之後解構子就不會 join —— 等於放棄了 jthread
//      的全部好處，通常代表設計有問題。
//   5. GCC 需要 -std=c++20 才有 std::jthread。注意：只用 -std=c++17 編這個檔會
//      失敗，這點已用 -pedantic-errors 驗證過。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::jthread 的 RAII 語意
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::thread 解構時若還 joinable 會發生什麼？為什麼標準要這樣設計？
//     答：呼叫 std::terminate()，程式直接中止。這是標準【保證】的行為，
//         不是未定義行為，也不是「可能會崩潰」。設計理由是解構時自動 join
//         會讓解構子阻塞任意久，自動 detach 又會造成 dangling reference，
//         兩害相權，委員會選擇讓錯誤立刻可見。
//     追問：那 jthread 為什麼就敢自動 join？
//         → 因為它同時提供 stop_token，解構子先 request_stop() 再 join()，
//           讓「阻塞很久」這個缺點有標準化的解法。
//
// 🔥 Q2. ~jthread() 到底做了哪兩件事？順序可以對調嗎？
//     答：先 request_stop()、再 join()。不能對調。若先 join()，一個
//         「while (!st.stop_requested())」的執行緒永遠等不到停止請求，
//         解構子就會永久阻塞。
//     追問：如果 callable 根本沒接 stop_token 呢？
//         → request_stop() 照樣會被呼叫，只是沒有人去讀那個旗標，等同無作用。
//           這不是錯誤，行為仍完全良好定義。
//
// ⚠️ 陷阱. 「用了 jthread 就不會有 dangling reference 了」——為什麼這句話是錯的？
//     答：jthread 只保證「離開作用域前一定 join」，它保證的是【執行緒已結束】。
//         但如果你在 class 裡把 jthread 宣告在其他成員【之前】，
//         解構順序（與宣告順序相反）會讓其他成員先死、jthread 最後才 join，
//         執行緒在這個空窗期仍可能存取已銷毀的成員 → 這仍是未定義行為。
//     為什麼會錯：多數人把 jthread 當成「萬能保險」，忽略了它的保護範圍
//         只到「自己的解構子」為止，管不到同一個物件裡其他成員的生命週期。
//         正確做法是把 jthread 放在成員宣告的最後一行。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔從缺
//   本檔示範的是「RAII 自動 join」這個資源管理慣用法，屬於 C++ 語言層的
//   生命週期議題。LeetCode 的並行題（1114/1115/1116/1117/1195）考的都是
//   「多執行緒之間如何協調順序」，需要的是 mutex / condition_variable /
//   semaphore，與本檔主題不相干。硬湊一題反而會誤導讀者，故從缺。
//   協調類的題目安排在本課第 7 部分（工作執行緒池）與第四階段。

#include <chrono>
#include <iostream>
#include <stop_token>
#include <thread>

// -----------------------------------------------------------------------------
// 【基本示範】最單純的 jthread：建立即執行，離開作用域自動 join
// -----------------------------------------------------------------------------
void demoBasic() {
    std::jthread jt([]() {
        std::cout << "Hello from jthread!" << std::endl;
    });

    // 不需要呼叫 join()！
    // 離開作用域時 jt 解構 → request_stop() + join()
}

// -----------------------------------------------------------------------------
// 【對照示範】jthread 在例外拋出時仍然會 join（std::thread 這裡會 terminate）
// -----------------------------------------------------------------------------
void demoExceptionSafety() {
    try {
        std::jthread jt([]() {
            std::cout << "  背景工作開始" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            std::cout << "  背景工作結束" << std::endl;
        });

        throw std::runtime_error("中途拋出例外");
        // 這裡不會執行到，但 stack unwinding 仍會呼叫 ~jthread() → 自動 join
    } catch (const std::exception& e) {
        std::cout << "  main 捕獲例外: " << e.what() << std::endl;
    }
    std::cout << "  （執行緒已被安全回收，程式沒有 terminate）" << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】背景健康檢查（heartbeat）執行緒
//   情境：服務啟動時開一條背景執行緒，每隔一段時間探測下游依賴是否還活著，
//         把結果寫進共享狀態供 /healthz endpoint 讀取。
//   為何用 jthread：健康檢查執行緒的生命週期應該【完全等同於】它所屬的服務物件。
//         用 jthread 當成員變數（且宣告在最後），服務物件一解構，
//         執行緒自動收到停止請求並被 join，不需要寫任何 shutdown() 樣板碼。
// -----------------------------------------------------------------------------
class HealthMonitor {
public:
    explicit HealthMonitor(int intervalMs)
        // 注意 worker_ 宣告在最後 → 它會【最先】被解構，
        // 確保執行緒停下來之後，checkCount_ 等成員才被銷毀。
        : intervalMs_(intervalMs),
          worker_([this](std::stop_token st) { run(st); }) {}

    int checkCount() const { return checkCount_; }

private:
    void run(std::stop_token st) {
        while (!st.stop_requested()) {
            ++checkCount_;
            std::cout << "  [health] 第 " << checkCount_ << " 次探測: upstream OK"
                      << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs_));
        }
        std::cout << "  [health] 收到停止請求，監控結束" << std::endl;
    }

    int intervalMs_;
    int checkCount_ = 0;
    std::jthread worker_;   // ← 必須是最後一個成員
};

int main() {
    std::cout << "=== 基本用法：自動 join ===" << std::endl;
    demoBasic();
    std::cout << "（demoBasic 已返回，代表執行緒確定已結束）" << std::endl;

    std::cout << "\n=== 例外安全：拋例外也會 join ===" << std::endl;
    demoExceptionSafety();

    std::cout << "\n=== 物件大小（實作定義，本機 libstdc++ x86-64） ===" << std::endl;
    std::cout << "sizeof(std::thread)  = " << sizeof(std::thread) << std::endl;
    std::cout << "sizeof(std::jthread) = " << sizeof(std::jthread) << std::endl;

    std::cout << "\n=== 日常實務：背景健康檢查執行緒 ===" << std::endl;
    {
        HealthMonitor monitor(60);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::cout << "  main: 服務即將關閉，monitor 準備解構" << std::endl;
    }  // monitor 解構 → worker_ 解構 → request_stop() + join()
    std::cout << "  main: monitor 已完全回收" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra -pthread "課程 3.3：std jthread (C++20)1.cpp" -o jthread1
//   注意：本檔【必須】用 -std=c++20。std::jthread / std::stop_token 是 C++20 才加入的，
//   用 -std=c++17 -pedantic-errors 編譯會直接失敗（已實測驗證）。

// === 預期輸出 ===
// （健康檢查的探測次數與 sizeof 為實作定義／時間相關，每次執行可能不同）
// === 基本用法：自動 join ===
// Hello from jthread!
// （demoBasic 已返回，代表執行緒確定已結束）
//
// === 例外安全：拋例外也會 join ===
//   背景工作開始
//   背景工作結束
//   main 捕獲例外: 中途拋出例外
//   （執行緒已被安全回收，程式沒有 terminate）
//
// === 物件大小（實作定義，本機 libstdc++ x86-64） ===
// sizeof(std::thread)  = 8
// sizeof(std::jthread) = 16
//
// === 日常實務：背景健康檢查執行緒 ===
//   [health] 第 1 次探測: upstream OK
//   [health] 第 2 次探測: upstream OK
//   [health] 第 3 次探測: upstream OK
//   [health] 第 4 次探測: upstream OK
//   main: 服務即將關閉，monitor 準備解構
//   [health] 收到停止請求，監控結束
//   main: monitor 已完全回收
