// =============================================================================
//  課程 3.1：RAII 與執行緒管理 3  —  ScopedThread：以「所有權」建立類別不變式
// =============================================================================
//
// 【主題資訊 Information】
//   class ScopedThread {
//       std::thread t;                                 // 持有值 → 擁有所有權
//   public:
//       explicit ScopedThread(std::thread thread);     // 不 joinable 就丟例外
//       ~ScopedThread();                               // 無條件 join()
//       ScopedThread(const ScopedThread&)            = delete;
//       ScopedThread& operator=(const ScopedThread&) = delete;
//   };
//
//   標準版本：C++11
//   標頭檔  ：<thread>、<stdexcept>(std::logic_error)
//   複雜度  ：解構時的 join() 阻塞至目標執行緒結束
//   出處    ：《C++ Concurrency in Action》scoped_thread(對照 thread_guard)
//
// 【詳細解釋 Explanation】
//
// 【1. 與上一課 ThreadGuard 的根本差異:所有權】
//   ThreadGuard 的成員是 std::thread&    → 「我只是幫你看著,thread 是你的」
//   ScopedThread 的成員是 std::thread    → 「thread 交給我了,你別再碰」
//
//   這一個字的差別改變了三件事:
//     (a) 呼叫端在建構後【無法】再存取那個 thread(沒有提供 getter),
//         因此不可能出現「守衛與呼叫端各做一次 join」的衝突。
//     (b) 生命週期由物件本身管理,不再依賴宣告順序 —— ThreadGuard 最大
//         的坑(thread 先於 guard 解構 → dangling reference)在此消失。
//     (c) 因為 std::thread 是 move-only,建構函式必須以「傳值 + std::move」
//         的方式接收,呼叫端得寫 ScopedThread st(std::thread(f)) 或
//         ScopedThread st(std::move(existing_thread))。
//
// 【2. 建構函式為什麼要檢查 joinable(),還丟例外?】
//   這是本檔最值得學的一點:【用建構函式建立類別不變式(class invariant)】。
//
//       explicit ScopedThread(std::thread thread) : t(std::move(thread)) {
//           if (!t.joinable()) throw std::logic_error("No thread");
//       }
//
//   建構成功之後,「t 一定是 joinable」成為這個類別永遠成立的性質。
//   有了這個不變式,解構函式才敢寫成:
//
//       ~ScopedThread() { t.join(); }     // 不需要 if (t.joinable())
//
//   這不是偷懶,而是「不變式已經保證了前提」。對照 ThreadGuard 必須每次
//   檢查 joinable(),因為它管不到別人會不會先把 thread join 掉。
//
//   什麼情況會傳進一個非 joinable 的 thread?
//     * 預設建構的 std::thread(沒有關聯任何執行緒)
//     * 已經被 join() 或 detach() 過的 thread
//     * 已被 move 走內容的 thread(moved-from 狀態)
//   這些都代表呼叫端的邏輯錯誤,所以丟 std::logic_error 而不是 runtime_error
//   —— 語意上這是「程式寫錯了」,不是「執行期環境出狀況」。
//
// 【3. 為什麼不變式不會在建構之後被破壞?】
//   因為這個類別【沒有任何一個成員函式能改變 t 的狀態】:
//     * 複製建構 / 複製賦值:= delete
//     * 移動建構 / 移動賦值:因為使用者自訂了解構函式,編譯器【不會】
//       隱式產生移動操作([class.copy.ctor]/8);又因為複製被 delete,
//       所以連退化成複製都不行 → ScopedThread 完全不可搬移。
//     * 沒有 get()、沒有 detach()、沒有 join() 對外暴露。
//   所以 t 從建構到解構都維持 joinable。這是「不可變狀態 → 簡化不變式」
//   的經典設計。
//
// 【4. 這個設計的代價:完全不可搬移】
//   不可搬移意味著:
//     * 不能 return ScopedThread(...)(C++17 的 guaranteed copy elision 讓
//       「直接回傳純右值」可行,但回傳具名區域變數仍需要移動 → 不行)
//     * 不能放進 std::vector<ScopedThread>(vector 需要能移動元素)
//     * 不能當函式參數傳值
//   對「只在一個作用域裡用完就丟」的情境完全夠用,這也是它叫
//   Scoped-Thread 的原因。若需要放進容器,請見「執行緒守衛類別設計1/2」
//   的 JoiningThread —— 它補上了移動操作。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼參數是 std::thread(傳值)而不是 std::thread&&?
//   兩種寫法都能用,但傳值 + std::move 是現代 C++ 的慣例(sink parameter):
//       ScopedThread(std::thread t) : t_(std::move(t)) {}      // 傳值
//       ScopedThread(std::thread&& t) : t_(std::move(t)) {}    // 傳右值參考
//   傳值版本同時接受右值(移動進來)與具名左值(呼叫端明確寫 std::move),
//   而且只寫一個多載;傳右值參考版本語意相同但少了一次移動。
//   std::thread 的移動成本極低(交換一個 handle),所以傳值的可讀性優勢
//   大於那次移動的成本。
//
// (B) 「建構函式丟例外」時,已經移動進來的 thread 怎麼辦?
//   關鍵:成員 t 的初始化在 constructor body 之前就完成了。當 body 裡的
//   throw 發生時,C++ 保證【已建構完成的成員會被反向解構】。
//   但這裡有個微妙處:此時 t 是「非 joinable」的(這正是丟例外的原因),
//   所以 ~thread() 不會 terminate(),安全。
//   反過來想:如果條件寫反了 —— 對 joinable 的 thread 丟例外 —— 成員解構
//   時就會撞上 std::terminate(),連例外都傳不出去。順序與條件都不能寫錯。
//
// (C) 為什麼解構函式呼叫 join() 而不是包 try/catch?
//   嚴格說起來,生產程式碼應該包。join() 在下列情況會丟 std::system_error:
//     * resource_deadlock_would_occur:對自己所在的執行緒 join()
//     * no_such_process / invalid_argument:底層 handle 已失效
//   解構函式隱含 noexcept,例外逸出即 std::terminate()。本檔保持教學版的
//   簡潔寫法,但真實專案建議:
//       ~ScopedThread() noexcept { try { t.join(); } catch (...) { /* log */ } }
//
// (D) 「移動操作被抑制」的完整規則
//   編譯器【不會】隱式產生移動建構/移動賦值,只要類別有以下任一項:
//     1. 使用者宣告的複製建構或複製賦值(包含 = delete 與 = default)
//     2. 使用者宣告的移動建構或移動賦值(另一個)
//     3. 使用者宣告的解構函式
//   ScopedThread 同時踩到 1 和 3。這就是所謂 Rule of Five/Zero 的由來:
//   一旦你手寫了其中一個,其餘的就要一起想清楚,否則會得到「編譯器
//   悄悄不給你移動」這種難以察覺的結果。
//
// 【注意事項 Pay Attention】
//   1. 建構函式接收的是【值】,呼叫端必須交出所有權:
//        ScopedThread st(std::thread(f));            // 臨時物件,直接移動
//        ScopedThread st(std::move(existing));       // 具名變數,顯式 move
//        ScopedThread st(existing);                  // ❌ 編譯失敗(thread 不可複製)
//   2. 傳入非 joinable 的 thread 會丟 std::logic_error —— 這是刻意的,
//      代表呼叫端邏輯有誤。
//   3. 此類別不可複製也不可移動,無法放進 std::vector、無法從函式回傳。
//   4. 解構函式的 join() 若丟例外會導致 std::terminate();教學版未處理,
//      生產程式碼請自行包 try/catch。
//   5. 需要 <stdexcept> 才能使用 std::logic_error。原始版本只 include
//      了 <iostream> 與 <thread>,能編譯純屬標頭傳遞依賴(transitive
//      include)的巧合,換個標準函式庫實作就可能失敗 —— 已在本檔補上。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】ScopedThread 與類別不變式
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. ScopedThread 的解構函式為什麼敢直接寫 t.join(),不檢查 joinable()?
//     答：因為建構函式已經建立了類別不變式 —— 傳進來的 thread 若不是
//         joinable 就丟 std::logic_error,建構根本不會成功。而這個類別
//         沒有任何成員函式能改變 t 的狀態(複製被 delete、移動被抑制、
//         也沒暴露 join/detach/get),所以 t 從生到死都是 joinable。
//     追問：那如果有人幫這個類別加上移動建構呢?
//         → 不變式立刻被打破:被移走的那個物件會處於 moved-from 狀態,
//           t 變成非 joinable,解構時 t.join() 就會丟 std::system_error,
//           從 noexcept 的解構函式逸出 → std::terminate()。所以加移動
//           操作的同時,解構函式一定要改回 if (t.joinable()) t.join()。
//
// 🔥 Q2. 為什麼 ScopedThread 不能放進 std::vector?
//     答：std::vector 在成長(reallocation)時需要把元素搬到新的記憶體,
//         因此元素型別必須可移動或可複製。ScopedThread 的複製被 = delete,
//         而使用者自訂解構函式又抑制了隱式移動建構的生成,兩條路都斷了。
//     追問：怎麼修?
//         → 明確補上 ScopedThread(ScopedThread&&) noexcept = default; 與
//           對應的移動賦值,同時把解構函式改成有檢查的版本(見 Q1 追問)。
//           這正是「執行緒守衛類別設計1」JoiningThread 在做的事。
//
// ⚠️ 陷阱. 「ScopedThread st(existing_thread); 為什麼編譯不過?我明明有
//            寫建構函式接收 std::thread。」
//     答：參數是【傳值】的 std::thread,用具名左值初始化它需要複製建構,
//         而 std::thread 的複製建構是 = delete。必須寫成
//         ScopedThread st(std::move(existing_thread)) 明確交出所有權。
//     為什麼會錯：把「傳值參數」直覺想成「會自動幫我複製一份」,
//         忘了對 move-only 型別而言,傳值只能靠移動來初始化。
//         這也正是設計者要的效果:讓「所有權轉移」在呼叫端可見。
// ═══════════════════════════════════════════════════════════════════════════

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

// -----------------------------------------------------------------------------
// 主角:以所有權建立不變式的執行緒守衛
// -----------------------------------------------------------------------------
class ScopedThread {
    std::thread t;

public:
    explicit ScopedThread(std::thread thread)
        : t(std::move(thread))
    {
        // 建立類別不變式:成功建構 ⇒ t 必為 joinable
        if (!t.joinable()) {
            throw std::logic_error("No thread");
        }
    }

    ~ScopedThread() {
        t.join();  // 不變式保證一定 joinable,故不需檢查
    }

    // 禁止複製(移動也因自訂解構函式而被抑制)
    ScopedThread(const ScopedThread&) = delete;
    ScopedThread& operator=(const ScopedThread&) = delete;
};

// -----------------------------------------------------------------------------
// 示範 1:基本用法 —— 臨時 thread 直接移動進來
// -----------------------------------------------------------------------------
void demoBasic() {
    std::cout << "  [main] 建立 ScopedThread\n";
    {
        ScopedThread st(std::thread([]() {
            std::cout << "  [worker] 安全的執行緒\n";
        }));
    }  // st 解構 → join()
    std::cout << "  [main] 離開作用域,worker 已 join\n";
}

// -----------------------------------------------------------------------------
// 示範 2:從具名變數移動 —— 必須顯式 std::move
// -----------------------------------------------------------------------------
void demoMoveFromNamed() {
    std::thread worker([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        std::cout << "  [worker] 由具名 thread 移動而來\n";
    });

    std::cout << "  [main] 移動前 worker.joinable() = "
              << std::boolalpha << worker.joinable() << "\n";

    {
        ScopedThread st(std::move(worker));  // 所有權轉移
        // 此後 worker 處於 moved-from 狀態
        std::cout << "  [main] 移動後 worker.joinable() = " << worker.joinable() << "\n";
    }

    std::cout << "  [main] ScopedThread 已解構\n";
}

// -----------------------------------------------------------------------------
// 示範 3:不變式的守門員 —— 傳入非 joinable 的 thread 會被擋下
// -----------------------------------------------------------------------------
void demoInvariantGuard() {
    try {
        std::thread empty;  // 預設建構,沒有關聯任何執行緒 → 非 joinable
        ScopedThread st(std::move(empty));
        std::cout << "  這行不會被執行\n";
    } catch (const std::logic_error& e) {
        std::cout << "  攔截到 std::logic_error: " << e.what() << "\n";
        std::cout << "  建構失敗 → 不變式未被破壞,解構函式也不會被呼叫\n";
    }
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔【刻意不提供】
//
//   說明:LeetCode 現有的多執行緒題(1114 Print in Order、1115 Print FooBar
//   Alternately、1116 Print Zero Even Odd、1117 Building H2O、
//   1195 Fizz Buzz Multithreaded)全部考「執行緒間的順序協調」,
//   評測框架自行負責建立與回收執行緒,作答者拿不到 std::thread 物件,
//   自然也用不到 join/detach 與 RAII 守衛。本檔主題是【所有權與類別
//   不變式】,與那些題目沒有真實交集,故從缺。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】設定檔熱重載監看器(config watcher)
//
//   情境:服務啟動時開一個背景執行緒定期檢查設定檔是否變更。
//         這個 watcher 的生命週期必須【嚴格等於】服務物件的生命週期 ——
//         服務被銷毀後 watcher 還在跑,就會存取到已銷毀的成員。
//   為何適合 ScopedThread:
//     * 所有權明確:watcher thread 屬於 ConfigService,外界拿不到它,
//       不可能有人在外面偷偷 detach() 或 join() 造成雙重釋放。
//     * 不變式:只要 ConfigService 建構成功,watcher 就一定在跑;
//       ConfigService 解構,watcher 就一定被 join。中間沒有模糊地帶。
//   注意:成員宣告順序 —— stopFlag 必須宣告在 watcher 之【前】,
//         這樣解構時 watcher 先 join、stopFlag 才銷毀(反向解構)。
// -----------------------------------------------------------------------------
class ConfigService {
    std::atomic<bool>         stopFlag{false};  // 先宣告 → 後解構
    std::vector<std::string>  reloadLog;
    std::mutex                logMutex;
    ScopedThread              watcher;          // 後宣告 → 先解構(先 join)

public:
    explicit ConfigService(int checkCount)
        : watcher(std::thread([this, checkCount]() {
              for (int i = 0; i < checkCount && !stopFlag.load(); ++i) {
                  std::this_thread::sleep_for(std::chrono::milliseconds(10));
                  std::lock_guard<std::mutex> lk(logMutex);
                  reloadLog.push_back("check#" + std::to_string(i + 1) + " 設定未變更");
              }
          }))
    {}

    ~ConfigService() {
        stopFlag.store(true);  // 先請 watcher 收工
        // watcher 成員解構 → join(),保證 reloadLog / logMutex 尚未銷毀
    }

    std::vector<std::string> snapshot() {
        std::lock_guard<std::mutex> lk(logMutex);
        return reloadLog;
    }
};

int main() {
    std::cout << "=== 示範 1:基本用法 ===\n";
    demoBasic();

    std::cout << "\n=== 示範 2:從具名 thread 移動 ===\n";
    demoMoveFromNamed();

    std::cout << "\n=== 示範 3:不變式守門員(非 joinable 被擋下)===\n";
    demoInvariantGuard();

    std::cout << "\n=== 日常實務:設定檔熱重載監看器 ===\n";
    {
        ConfigService svc(3);
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        std::cout << "  服務執行中,已累積 " << svc.snapshot().size() << " 筆檢查紀錄\n";
    }  // svc 解構:stopFlag=true → watcher join → 資源安全釋放
    std::cout << "  服務已關閉,watcher 已被 join\n";

    std::cout << "\n=== 全部示範結束 ===\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.1：RAII 與執行緒管理3.cpp" -o raii3

// === 預期輸出 ===
// === 示範 1:基本用法 ===
//   [main] 建立 ScopedThread
//   [worker] 安全的執行緒
//   [main] 離開作用域,worker 已 join
//
// === 示範 2:從具名 thread 移動 ===
//   [main] 移動前 worker.joinable() = true
//   [main] 移動後 worker.joinable() = false
//   [worker] 由具名 thread 移動而來
//   [main] ScopedThread 已解構
//
// === 示範 3:不變式守門員(非 joinable 被擋下)===
//   攔截到 std::logic_error: No thread
//   建構失敗 → 不變式未被破壞,解構函式也不會被呼叫
//
// === 日常實務:設定檔熱重載監看器 ===
//   服務執行中,已累積 3 筆檢查紀錄
//   服務已關閉,watcher 已被 join
//
// === 全部示範結束 ===
