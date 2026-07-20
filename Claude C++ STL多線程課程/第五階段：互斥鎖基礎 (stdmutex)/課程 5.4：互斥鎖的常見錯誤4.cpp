// =============================================================================
//  課程 5.4：互斥鎖的常見錯誤4.cpp  —  lock_guard 的內部構造與選用時機
// =============================================================================
//
// ⚠️ 【本檔與「常見錯誤3.cpp」的關係】
//    課程原始素材中，這兩個檔案的程式碼【完全相同】（都是 lesson_5_4_raii_fix）。
//    為了避免兩份教材重複，本檔在保留同一段示範程式碼的前提下，
//    改從【另一個角度】切入：
//      * 「常見錯誤3」講的是「為什麼需要 RAII」（例外路徑、離開路徑）；
//      * 【本檔】講的是「lock_guard 到底長什麼樣」——親手實作一個等價品，
//        並釐清它與 unique_lock / scoped_lock 的取捨與成本。
//    兩檔各自獨立可編譯，內容不重疊。
//
// 【主題資訊 Information】
//   template<class Mutex> class std::lock_guard;                    // C++11，<mutex>
//       using mutex_type = Mutex;
//       explicit lock_guard(mutex_type& m);              // 建構 → m.lock()
//       lock_guard(mutex_type& m, std::adopt_lock_t);    // 假設已持有，不再 lock
//       ~lock_guard();                                   // 解構 → m.unlock()
//       lock_guard(const lock_guard&) = delete;
//       lock_guard& operator=(const lock_guard&) = delete;
//   物件大小（實作定義）：本機 sizeof(std::lock_guard<std::mutex>) = 8 bytes
//                        （就是一個 mutex 的參考／指標，見程式輸出）
//
// 【詳細解釋 Explanation】
//
// 【1. lock_guard 的完整實作只有十幾行】
//   它沒有任何魔法。一個功能等價的版本長這樣：
//       template <class Mutex>
//       class MyLockGuard {
//           Mutex& m_;
//       public:
//           explicit MyLockGuard(Mutex& m) : m_(m) { m_.lock(); }
//           ~MyLockGuard() { m_.unlock(); }
//           MyLockGuard(const MyLockGuard&) = delete;
//           MyLockGuard& operator=(const MyLockGuard&) = delete;
//       };
//   關鍵在於這四件事：
//     (a) 只存【參考】，不擁有 mutex —— 所以 mutex 的生命週期必須更長；
//     (b) 建構即上鎖，沒有「未持有」的中間狀態；
//     (c) 解構即解鎖，由語言保證必然執行；
//     (d) 刪除複製 —— 鎖的所有權必須唯一。
//   本檔真的實作了一份並拿它跑多執行緒測試，證明它與標準版行為一致。
//
// 【2. 為什麼「只存參考」是個重要決定】
//   lock_guard 不管理 mutex 的生命週期，只借用它。這帶來一個責任：
//   【mutex 必須比 lock_guard 活得久】。以下是真實的懸空參考錯誤：
//       std::lock_guard<std::mutex> makeGuard() {
//           std::mutex local;                    // 區域變數
//           return std::lock_guard<std::mutex>(local);  // 💀 local 即將消滅
//       }
//   （這段實際上也無法編譯，因為 lock_guard 不可移動——
//     但同樣的錯誤在 unique_lock 上就編得過，而且是真的 UB。）
//   實務上 mutex 通常是類別的成員或全域變數，生命週期自然夠長，
//   所以這個問題較少浮現，但寫泛型程式碼時要留意。
//
// 【3. 為什麼 lock_guard 沒有 unlock() 方法】
//   這是刻意的設計，不是遺漏。lock_guard 的語意就是
//   「這個作用域從頭到尾持有鎖」——一個不變的事實。
//   若允許中途 unlock，就得多存一個「現在到底有沒有持有」的狀態，
//   解構時還要分支判斷，那就變成 unique_lock 了。
//   → 這是 C++ 標準函式庫常見的設計手法：把「能力」與「成本」綁在一起，
//     讓使用者用型別選擇自己需要的那一層，不為用不到的功能付費。
//     需要中途解鎖？請明確地選 unique_lock，而不是讓所有人都背這個成本。
//
// 【4. 三種 RAII 鎖的成本與能力對照】
//   ┌──────────────┬──────────┬────────────────────────────────────────┐
//   │ 型別         │ 本機大小 │ 能力                                   │
//   ├──────────────┼──────────┼────────────────────────────────────────┤
//   │ lock_guard   │  8 bytes │ 建構鎖、解構解鎖。沒有其他方法          │
//   │ unique_lock  │ 16 bytes │ + defer/try/adopt、中途 unlock、可移動  │
//   │              │          │   （多一個 bool 狀態，故大小加倍）      │
//   │ scoped_lock  │  可變    │ 同時鎖多把，內建死結避免演算法          │
//   └──────────────┴──────────┴────────────────────────────────────────┘
//   （大小為本機 x86-64 / libstdc++ 實測，屬【實作定義】，見程式輸出。）
//
// 【概念補充 Concept Deep Dive】
//   * lock_guard 是【樣板】而非只吃 std::mutex，所以任何滿足
//     BasicLockable 要求（有 lock() 與 unlock()）的型別都能用，
//     包括 std::recursive_mutex、std::timed_mutex、std::shared_mutex，
//     以及【你自己寫的鎖】——本課 5.2 的 SimpleSpinlock 就可以直接套進去。
//     本檔實測了這一點。
//   * adopt_lock 版本的建構子存在的理由：搭配 std::lock() 使用。
//         std::lock(m1, m2);                        // 一次取得，無死結
//         std::lock_guard<std::mutex> g1(m1, std::adopt_lock);  // 只負責解鎖
//         std::lock_guard<std::mutex> g2(m2, std::adopt_lock);
//     C++17 之後這整段可以直接寫成 std::scoped_lock lock(m1, m2);
//   * C++17 的 CTAD 讓樣板參數可以省略：std::lock_guard lock(mtx);
//     這不只是少打字——它讓「忘記變數名」這個致命手誤更容易被看出來
//     （見「常見錯誤3」的陷阱題）。
//
// 【注意事項 Pay Attention】
//   1. lock_guard 只存參考，【mutex 必須活得比它久】。
//   2. 它不可複製也不可移動，無法從函式回傳；需要轉移所有權用 unique_lock。
//   3. 它沒有 unlock()／owns_lock()，這是設計而非缺陷。
//   4. adopt_lock 版本假設【呼叫端已經持有鎖】；用在未鎖的 mutex 上是 UB。
//   5. 物件大小是實作定義的；本機為 8 bytes，不可寫死在任何假設裡。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】lock_guard 的實作與選用
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 請手寫一個功能等價於 std::lock_guard 的類別。
//     答：核心只有四點——存 mutex 的【參考】、建構子呼叫 lock()、
//         解構子呼叫 unlock()、把複製建構與複製指派 delete 掉。
//             template <class Mutex>
//             class MyLockGuard {
//                 Mutex& m_;
//             public:
//                 explicit MyLockGuard(Mutex& m) : m_(m) { m_.lock(); }
//                 ~MyLockGuard() { m_.unlock(); }
//                 MyLockGuard(const MyLockGuard&) = delete;
//                 MyLockGuard& operator=(const MyLockGuard&) = delete;
//             };
//     追問：為什麼一定要 delete 複製？→ 鎖的所有權必須唯一。
//           若能複製，兩個物件解構時會對同一把鎖 unlock 兩次，
//           第二次是對未持有的 mutex 解鎖，屬未定義行為。
//
// 🔥 Q2. lock_guard 為什麼沒有提供 unlock() 方法？
//     答：這是刻意的設計。lock_guard 的語意是「整個作用域都持有鎖」，
//         這是一個不變的事實，因此它不需要記錄「現在有沒有持有」。
//         若要支援中途解鎖，就得多一個 bool 狀態與解構時的分支判斷——
//         那正是 unique_lock（本機 16 bytes，lock_guard 只有 8 bytes）。
//         符合 C++「不為你沒用到的功能付出代價」的原則。
//     追問：那什麼時候真的需要中途 unlock？
//           → 最典型的是 condition_variable：cv.wait() 必須在等待期間
//             放開鎖，醒來後再重新取得，所以它只接受 unique_lock。
//
// ⚠️ 陷阱. lock_guard 只能搭配 std::mutex 使用，對嗎？
//     答：不對。它是一個【樣板】，接受任何滿足 BasicLockable
//         （具備 lock() 與 unlock() 成員）的型別。
//         所以 std::recursive_mutex、std::timed_mutex、std::shared_mutex，
//         甚至你自己寫的自旋鎖，都可以直接套用。
//     為什麼會錯：教材裡幾乎永遠寫 std::lock_guard<std::mutex>，
//         看久了就把樣板參數當成固定寫法。
//         實際上 <> 裡放什麼是你的選擇——這也是為什麼
//         C++17 的 CTAD（std::lock_guard lock(mtx);）能正確推導出型別。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔【略過】，理由如下：
//   本檔主題是 lock_guard 這個 RAII 包裝器的【內部構造與成本模型】，
//   屬於語言／函式庫實作層面的知識。
//   LeetCode 的並行題（1114 / 1115 / 1116 / 1117 / 1195）評測的是輸出順序，
//   用哪一種鎖包裝器（甚至完全手寫 lock/unlock）都不影響判定結果。
//   姊妹檔「常見錯誤3.cpp」已用 LeetCode 155. Min Stack 示範
//   RAII 鎖在設計題中的實際應用，這裡不重複。

#include <atomic>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

std::mutex mtx;

int getValueSafe(int input) {
    std::lock_guard<std::mutex> lock(mtx);  // ✓ RAII

    if (input < 0) {
        std::cout << "無效輸入" << std::endl;
        return -1;  // ✓ lock_guard 解構時自動 unlock
    }

    if (input == 0) {
        std::cout << "零值" << std::endl;
        return 0;   // ✓ 同樣會自動 unlock
    }

    return input * 2;
}  // ✓ 函式結束，lock_guard 解構，自動 unlock

// -----------------------------------------------------------------------------
// 【親手實作】一個功能等價於 std::lock_guard 的類別
//   目的：證明它沒有任何魔法——只有「建構即鎖、解構即解鎖、禁止複製」。
// -----------------------------------------------------------------------------
template <class Mutex>
class MyLockGuard {
private:
    Mutex& m_;                                   // 只存【參考】，不擁有

public:
    explicit MyLockGuard(Mutex& m) : m_(m) { m_.lock(); }
    ~MyLockGuard() { m_.unlock(); }

    // 鎖的所有權必須唯一：允許複製會導致同一把鎖被 unlock 兩次（UB）
    MyLockGuard(const MyLockGuard&)            = delete;
    MyLockGuard& operator=(const MyLockGuard&) = delete;
};

// -----------------------------------------------------------------------------
// 【證明它是樣板，不只吃 std::mutex】
//   自訂一個滿足 BasicLockable（有 lock() 與 unlock()）的自旋鎖，
//   直接套進【標準的】std::lock_guard。
// -----------------------------------------------------------------------------
class TinySpinlock {
private:
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;

public:
    void lock() {
        while (flag_.test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    }
    void unlock() { flag_.clear(std::memory_order_release); }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】計數器：同一份邏輯，三種鎖各跑一次
//   情境：最單純也最常見的共享狀態——一個被多執行緒更新的計數器。
//   目的：在同一個工作負載下，證明
//     (1) 自製的 MyLockGuard、(2) 標準 std::lock_guard、
//     (3) 標準 lock_guard 套自訂的 TinySpinlock
//   三者行為完全一致，結果都正確。
//   這也順帶說明：選哪一種鎖包裝器不影響【正確性】，
//   影響的是【能力】（能不能中途解鎖、能不能轉移所有權）與可讀性。
// -----------------------------------------------------------------------------
struct Counters {
    std::mutex   m1;
    long         viaMyGuard = 0;

    std::mutex   m2;
    long         viaStdGuard = 0;

    TinySpinlock m3;
    long         viaSpinGuard = 0;
};

void runCounterWorkload(Counters& c, int threads, int perThread) {
    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(threads) * 3);

    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&c, perThread]() {
            for (int i = 0; i < perThread; ++i) {
                MyLockGuard<std::mutex> lock(c.m1);       // 自製版
                ++c.viaMyGuard;
            }
        });
        workers.emplace_back([&c, perThread]() {
            for (int i = 0; i < perThread; ++i) {
                std::lock_guard<std::mutex> lock(c.m2);   // 標準版
                ++c.viaStdGuard;
            }
        });
        workers.emplace_back([&c, perThread]() {
            for (int i = 0; i < perThread; ++i) {
                std::lock_guard<TinySpinlock> lock(c.m3); // 標準版 + 自訂鎖
                ++c.viaSpinGuard;
            }
        });
    }
    for (auto& t : workers) t.join();
}

int main() {
    std::cout << "=== 課程示範: lock_guard 自動管理鎖 ===" << std::endl;
    std::cout << getValueSafe(10) << std::endl;
    std::cout << getValueSafe(-5) << std::endl;
    std::cout << getValueSafe(20) << std::endl;  // ✓ 正常執行

    std::cout << "\n=== 三種 RAII 鎖的大小（實作定義）===" << std::endl;
    std::cout << "sizeof(std::lock_guard<std::mutex>)  = "
              << sizeof(std::lock_guard<std::mutex>) << " bytes"
              << "  (就是一個 mutex 參考)" << std::endl;
    std::cout << "sizeof(std::unique_lock<std::mutex>) = "
              << sizeof(std::unique_lock<std::mutex>) << " bytes"
              << "  (多一個「是否持有」的狀態)" << std::endl;
    std::cout << "sizeof(MyLockGuard<std::mutex>)      = "
              << sizeof(MyLockGuard<std::mutex>) << " bytes"
              << "  (自製版與標準版同樣大小)" << std::endl;

    std::cout << "\n=== 驗證: 自製版與標準版行為一致 ===" << std::endl;
    {
        Counters c;
        const int threads = 4, perThread = 50000;
        runCounterWorkload(c, threads, perThread);

        const long expected = static_cast<long>(threads) * perThread;
        std::cout << "預期值: " << expected << std::endl;
        std::cout << "MyLockGuard  + std::mutex   : " << c.viaMyGuard
                  << (c.viaMyGuard == expected ? "  ✓" : "  ✗") << std::endl;
        std::cout << "std::lock_guard + std::mutex: " << c.viaStdGuard
                  << (c.viaStdGuard == expected ? "  ✓" : "  ✗") << std::endl;
        std::cout << "std::lock_guard + 自訂自旋鎖: " << c.viaSpinGuard
                  << (c.viaSpinGuard == expected ? "  ✓" : "  ✗") << std::endl;
        std::cout << "結論: lock_guard 是樣板，任何有 lock()/unlock() 的型別都能套"
                  << std::endl;
    }

    std::cout << "\n=== adopt_lock: 搭配 std::lock 取得多把鎖 ===" << std::endl;
    {
        std::mutex a, b;

        // 舊寫法（C++11）：std::lock 一次取得，再用 adopt_lock 接管解鎖責任
        {
            std::lock(a, b);                                  // 無死結地同時取得
            std::lock_guard<std::mutex> ga(a, std::adopt_lock); // 只負責解鎖
            std::lock_guard<std::mutex> gb(b, std::adopt_lock);
            std::cout << "std::lock + adopt_lock: 同時持有兩把鎖" << std::endl;
        }   // 兩個 guard 解構，自動解鎖

        // 新寫法（C++17）：一行搞定
        {
            std::scoped_lock lock(a, b);
            std::cout << "std::scoped_lock (C++17): 同一件事，一行搞定" << std::endl;
        }
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.4：互斥鎖的常見錯誤4.cpp' -o lock_guard_internals

// -----------------------------------------------------------------------------
// 【輸出但書】
//   1. 三個 sizeof 都是【實作定義】的值（本機 x86-64 / g++ 15.2 / libstdc++）。
//      換平台或換標準函式庫實作，數字都可能不同。
//   2. 計數器的結果是【確定】的：每次 ++ 都在鎖內完成，不存在 data race。
//   3. 三種寫法的【耗時】差異取決於機器與負載，故本檔不輸出時間，
//      只驗證正確性。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// === 課程示範: lock_guard 自動管理鎖 ===
// 20
// 無效輸入
// -1
// 40
//
// === 三種 RAII 鎖的大小（實作定義）===
// sizeof(std::lock_guard<std::mutex>)  = 8 bytes  (就是一個 mutex 參考)
// sizeof(std::unique_lock<std::mutex>) = 16 bytes  (多一個「是否持有」的狀態)
// sizeof(MyLockGuard<std::mutex>)      = 8 bytes  (自製版與標準版同樣大小)
//
// === 驗證: 自製版與標準版行為一致 ===
// 預期值: 200000
// MyLockGuard  + std::mutex   : 200000  ✓
// std::lock_guard + std::mutex: 200000  ✓
// std::lock_guard + 自訂自旋鎖: 200000  ✓
// 結論: lock_guard 是樣板，任何有 lock()/unlock() 的型別都能套
//
// === adopt_lock: 搭配 std::lock 取得多把鎖 ===
// std::lock + adopt_lock: 同時持有兩把鎖
// std::scoped_lock (C++17): 同一件事，一行搞定
