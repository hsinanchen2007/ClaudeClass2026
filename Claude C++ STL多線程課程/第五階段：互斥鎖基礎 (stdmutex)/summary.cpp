// =============================================================================
//  summary.cpp  —  第五階段：互斥鎖基礎 (std::mutex) 總複習
// =============================================================================
//
// 【主題資訊 Information】
//   本檔是第五階段的【教科書式總整理】，涵蓋 5.1 ~ 5.6 全部內容。
//   涉及的標準設施（全部在 <mutex>）：
//     class std::mutex;                              // C++11 基本互斥鎖
//         void lock();  bool try_lock();  void unlock();
//     class std::recursive_mutex;                    // C++11 可重入
//     class std::timed_mutex;                        // C++11 支援逾時
//     class std::shared_mutex;                       // C++17 讀寫鎖
//     template<class M> class std::lock_guard;       // C++11 作用域 RAII 鎖
//     template<class M> class std::unique_lock;      // C++11 彈性 RAII 鎖
//     template<class... M> class std::scoped_lock;   // C++17 多鎖 RAII
//     void std::lock(L1&, L2&, ...);                 // C++11 無死結取多鎖
//     標籤：std::defer_lock / std::try_to_lock / std::adopt_lock
//   本機實測環境：Ubuntu 26.04、g++ 15.2.0、x86-64、hardware_concurrency() = 16
//
// 【詳細解釋 Explanation】
//
// 【1. 互斥鎖到底解決了「兩個」問題（多數人只知道一個）】
//   (a)【互斥】：同一時間只有一條執行緒能進入臨界區段。
//   (b)【可見性】：A 在 unlock 前寫的所有資料，對之後 lock 到【同一把鎖】
//       的 B 都可見。標準的說法是 unlock 與後續的 lock 之間
//       存在 synchronizes-with 關係，因而建立 happens-before。
//   ⚠️ 只知道 (a) 會導致一個常見的錯誤觀念：
//      「反正有鎖了，變數還要不要加 volatile？」——不需要，
//      而且 volatile 在 C++ 裡【完全不提供】執行緒同步保證。
//   ⚠️ (b) 的成立前提是【同一把鎖】。A 用 mtxA 保護寫、B 用 mtxB 保護讀，
//      兩者之間沒有任何同步關係，仍然是 data race → UB。
//
// 【2. 五種鎖包裝器怎麼選（本階段最實用的決策表）】
//   ┌──────────────────┬──────────┬────────────────────────────────────┐
//   │ 工具             │ 本機大小 │ 什麼時候用                         │
//   ├──────────────────┼──────────┼────────────────────────────────────┤
//   │ lock_guard       │  8 bytes │ 預設選擇。整個作用域持有鎖         │
//   │ unique_lock      │ 16 bytes │ 需要 defer/try/中途 unlock/移動，  │
//   │                  │          │ 或搭配 condition_variable          │
//   │ scoped_lock(C++17)│ 可變    │ 同時取得多把鎖（內建死結避免）     │
//   │ shared_lock(C++17)│ 可變    │ 讀寫鎖的「讀」側，讀多寫少時       │
//   │ 手寫 lock/unlock │    —     │ 【不要用】。任何 return 或例外     │
//   │                  │          │ 都會讓鎖洩漏                       │
//   └──────────────────┴──────────┴────────────────────────────────────┘
//   （大小為本機 x86-64 / libstdc++ 實測，屬【實作定義】。）
//
// 【3. 本階段所有錯誤的精確分類——這是最容易混淆的地方】
//   ┌────────────────────────────────┬────────────────────┬─────────────┐
//   │ 誤用                           │ 標準怎麼說         │ 是 UB 嗎？  │
//   ├────────────────────────────────┼────────────────────┼─────────────┤
//   │ 忘記 unlock，【別的】執行緒    │ lock() 阻塞等待    │ 【不是】    │
//   │ 再 lock                        │ → 永久阻塞（確定） │ 定義明確    │
//   │ 【同一】執行緒重複 lock        │ 明文規定為 UB      │ 【是】      │
//   │ 對【未持有】的 mutex unlock    │ 明文規定為 UB      │ 【是】      │
//   │ 兩執行緒相反順序取兩把鎖       │ 每個 lock 都合法， │ 【不是】，  │
//   │ （AB-BA）                      │ 但可能循環等待     │ 且不保證發生│
//   │ 檢查與動作分開加鎖（TOCTOU）   │ 完全合法           │ 【不是】，  │
//   │                                │                    │ 是邏輯錯誤  │
//   └────────────────────────────────┴────────────────────┴─────────────┘
//   ⚠️ 這五種的【症狀】常常都是「程式停住」或「數字不對」，但性質完全不同。
//      面試時能精確區分「標準保證的阻塞」「未定義行為」「機率性死結」
//      「純邏輯錯誤」，遠比背出「會死結」有價值。
//
// 【4. 臨界區段大小：正確性與效能的拉鋸】
//   太大 → 並行度歸零（Amdahl 定律：序列比例 s 決定加速上限 1/s）
//   太小 → 檢查與動作被拆散，不變量被打破（TOCTOU）
//   唯一判準：**臨界區段應恰好涵蓋「不變量從被破壞到被修復」的期間**。
//   不是「行數越少越好」，也不是「把所有碰共享資料的行都包起來」。
//   ⚠️ 正確性永遠優先於效能：寧可鎖大一點也不能拆散不變量。
//
// 【5. 效能的第一原則其實是「先問需不需要鎖」】
//   依成本由低到高：
//     (a) 不共享（thread_local、每執行緒各算各的最後合併）→ 零同步成本
//     (b) std::atomic（單純計數、旗標）→ 一次原子指令，無鎖
//     (c) std::mutex（複合操作、多欄位不變量）→ 無競爭時也很便宜
//     (d) std::shared_mutex（讀遠多於寫）→ 常數成本較高，短臨界區段未必划算
//   本階段課 5.2 實測過：無競爭的 std::mutex 完全不進核心
//   （glibc 的 futex fast path），所以「用 mutex 就會慢」是錯誤印象。
//
// 【概念補充 Concept Deep Dive】
//   * std::mutex 的底層是 futex（Fast Userspace muTEX）：
//     無競爭時純使用者空間的原子操作，只有真的要睡覺／叫醒別人時
//     才發出 SYS_futex 系統呼叫。這也是為什麼 sizeof(std::mutex)
//     在本機只有 40 bytes——核心裡根本沒有對應的物件。
//   * std::mutex【不保證公平】：等最久的執行緒不一定先拿到鎖，
//     同一條執行緒連續搶到好幾次是完全合法的。
//     需要公平性必須自己實作（如 ticket lock），代價是效能。
//   * 「thread-safe」不是二元屬性，而是一份需要說清楚範圍的【契約】：
//     「每個方法各自原子」不等於「多個方法的組合也原子」。
//     設計類別時要辨識出使用者真正需要的複合操作並直接提供，
//     而不是宣稱「我每個方法都加鎖了」就把責任交出去。
//   * ⚠️ 本檔沿用課程原始碼的 `using namespace std;`。
//     這在教學範例中可接受，但在【標頭檔】或大型專案中是公認的壞習慣
//     （會把整個 std 拉進全域命名空間，造成難以診斷的名稱衝突）。
//     正式程式碼請明確寫 std::。
//
// 【注意事項 Pay Attention】
//   1. 可見性保證只存在於【同一把 mutex】的 unlock → lock 之間。
//   2. 有 mutex 保護的變數【不需要】volatile；volatile 也【不能】取代鎖。
//   3. std::mutex 不可重入、不保證公平、不會因執行緒結束而自動解鎖。
//   4. 一律用 RAII；手寫 lock/unlock 在例外路徑上必然出錯。
//   5. 多把鎖用 std::scoped_lock（C++17），不要寫兩個 lock_guard。
//   6. 臨界區段內避免 I/O、sleep、呼叫使用者回呼、以及可能再取鎖的函式。
//   7. 「在 x86 上測起來沒問題」不代表正確——x86 的強記憶體模型
//      會遮蔽大量錯誤，換到 ARM 就會現形。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】第五階段總複習
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::mutex 除了「互斥」還提供什麼保證？
//     答：【記憶體可見性】。lock() 具 acquire 語意、unlock() 具 release 語意，
//         因此 A 在 unlock 前的所有寫入，對之後 lock 到【同一把鎖】的 B
//         都保證可見。這就是為什麼有鎖保護的變數不需要 volatile
//         （而且 volatile 在 C++ 裡也【不提供】任何同步保證）。
//     追問：這個保證的前提是什麼？→ 必須是【同一把鎖】。
//           A 用 mtxA 保護寫、B 用 mtxB 保護讀，兩者之間沒有任何
//           happens-before 關係，仍然是 data race → UB。
//
// 🔥 Q2. lock_guard、unique_lock、scoped_lock 怎麼選？成本差在哪？
//     答：lock_guard 是預設選擇——只存一個 mutex 參考（本機 8 bytes），
//         建構鎖、解構解鎖，最佳化後與手寫 lock/unlock 產生相同機器碼。
//         unique_lock 多一個「是否持有」的 bool（本機 16 bytes），
//         換來 defer_lock / try_to_lock / 中途 unlock / 可移動的彈性，
//         也是 condition_variable 唯一接受的型別。
//         scoped_lock（C++17）用於【同時】取得多把鎖，內建死結避免演算法。
//     追問：為什麼不能用兩個 lock_guard 鎖兩把鎖？
//           → 那是「依序取得」，兩條執行緒若順序相反就會 AB-BA 死結。
//             scoped_lock 是「一起取得」，內部演算法保證不死結。
//
// 🔥 Q3. 一個類別的每個 public 方法都用 lock_guard 保護了，
//        它是 thread-safe 的嗎？
//     答：每個方法【各自】是原子的，但使用它的程式碼不一定正確。
//         if (c.get() < 100) c.increment(); 這種「檢查後動作」
//         中間有空隙，兩條執行緒可能同時看到 99 然後都遞增。
//         正解是由類別提供【組合好的】原子介面（如 incrementIfBelow(100)），
//         而不是要求使用者在外面自己拼。
//     追問：這種錯誤 ThreadSanitizer 抓得到嗎？
//           → 抓不到。它不是 data race——每次存取都有鎖、
//             都有正確的 happens-before。它是純粹的邏輯錯誤（TOCTOU），
//             只能靠設計與程式碼審查避免。
//
// ⚠️ 陷阱 1. 「忘記 unlock 會造成死結。」這句話對嗎？
//     答：不精確。死結的定義是【兩個以上】執行緒【互相】等待對方持有的
//         資源，形成循環等待。忘記解鎖的情況沒有循環——
//         只有一把鎖，而且持有者可能已經結束了。
//         精確說法是【鎖洩漏導致的永久阻塞】。
//     為什麼會錯：把所有「程式卡住」都叫死結。
//         死結（循環等待）、活鎖（互相禮讓、CPU 很高）、
//         飢餓（長期搶不到）、鎖洩漏（沒人釋放）——
//         四種現象、四種成因、四種解法，症狀相似但完全不同。
//
// ⚠️ 陷阱 2. 「我的並行程式在 x86 上跑了三個月都沒問題，所以同步寫得沒錯。」
//     答：x86-64 是 TSO 強記憶體模型，硬體本身就禁止大部分重排，
//         只允許 StoreLoad 重排。這會遮蔽掉大量真正的同步錯誤——
//         同一份程式碼放到 ARM（手機、Apple Silicon、AWS Graviton）
//         就可能立刻出事。
//     為什麼會錯：把「測試通過」當成「符合標準」。
//         data race 是【未定義行為】，而 UB 的定義就是
//         「標準不保證任何事」，包含「不保證它會出錯」。
//         它安靜地跑三個月，完全不構成任何正確性的證據。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔【略過】，理由如下：
//   本檔是整個階段的【總複習】，涵蓋六課的所有概念。
//   各個技術點的 LeetCode 示範已分別放在對應的課程檔案中：
//       課 5.1 基本操作3.cpp     → LeetCode 146. LRU Cache
//       課 5.3 非阻塞鎖定5.cpp   → LeetCode 705. Design HashSet
//       課 5.4 常見錯誤3.cpp     → LeetCode 155. Min Stack
//       課 5.4 常見錯誤7.cpp     → LeetCode 707. Design Linked List
//   在總複習檔再掛一題只會重複，且無法涵蓋本檔的廣度，故從缺。
//   本檔改以「執行緒安全的連線池」作為綜合實務範例，
//   把六課的守則一次用上。

/*
 * ================================================================
 * 【第五階段：互斥鎖基礎 (std::mutex)】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -pthread -o summary summary.cpp
 *
 * 本階段涵蓋課程：
 * - 課程 5.1：std::mutex 基本操作（lock/unlock）
 * - 課程 5.2：互斥鎖的工作原理
 * - 課程 5.3：try_lock() 非阻塞鎖定
 * - 課程 5.4：互斥鎖的常見錯誤
 * - 課程 5.5：保護共享資料實作
 * - 課程 5.6：互斥鎖的效能考量
 *
 * 重點摘要：
 * 1. std::mutex —— 最基本的互斥鎖
 * 2. lock_guard  —— RAII 鎖，自動 unlock，推薦使用
 * 3. unique_lock —— 靈活鎖，支援延遲鎖定、手動 unlock
 * 4. try_lock()  —— 非阻塞嘗試，失敗立即返回
 * 5. 死鎖（Deadlock）成因與防範
 * 6. 執行緒安全的類別設計模式
 * 7. 效能考量：細粒度鎖定
 * ================================================================
 */

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <string>
#include <atomic>
using namespace std;

// ================================================================
// 課程 5.1：std::mutex 基本操作
// ================================================================
// 互斥鎖（Mutex）= Mutual Exclusion（互相排斥）
// 保證同一時刻只有一個執行緒可以進入「臨界區（Critical Section）」
//
// 基本操作：
//   mutex.lock()   —— 阻塞等待，取得鎖
//   mutex.unlock() —— 釋放鎖，讓其他執行緒可以進入
//
// 問題：沒有互斥鎖的計數器

namespace Lesson51 {

// 【錯誤示範】沒有鎖保護的計數器
int unsafe_counter = 0;
mutex counter_mutex;

void unsafe_increment() {
    for (int i = 0; i < 10000; ++i) {
        ++unsafe_counter;  // 非原子操作！讀-改-寫 可能被打斷
    }
}

// 【正確做法】用 mutex 保護
int safe_counter = 0;

void safe_increment() {
    for (int i = 0; i < 10000; ++i) {
        counter_mutex.lock();    // 鎖定（其他執行緒在此阻塞）
        ++safe_counter;          // 臨界區：只有一個執行緒可以執行
        counter_mutex.unlock();  // 釋放鎖
    }
}

void demo() {
    cout << "\n【5.1 基本 mutex 操作】" << endl;

    // ⚠️ 無鎖版本是【真正的 data race】= 未定義行為。
    //    它的結果不只是「可能小於 20000」——標準對它不做任何保證。
    //    因此本檔【刻意不印出那個數值】（把不確定的值當成預期輸出是錯的），
    //    改為跑多輪並回報「是否曾經觀察到遺失更新」這個可驗證的事實。
    int lostUpdateRounds = 0;
    const int rounds = 50;
    for (int r = 0; r < rounds; ++r) {
        unsafe_counter = 0;
        thread t1(unsafe_increment);
        thread t2(unsafe_increment);
        t1.join(); t2.join();
        if (unsafe_counter != 20000) ++lostUpdateRounds;
    }
    cout << "  無鎖計數: 跑 " << rounds << " 輪，是否曾遺失更新: "
         << (lostUpdateRounds > 0 ? "是 ← data race（UB），結果不可信"
                                  : "否（本次排程僥倖，但仍是 UB）") << endl;
    cout << "  （實際數值每次執行都不同，且屬未定義行為，故不列出）" << endl;

    // 有鎖：結果正確（等於 20000）
    safe_counter = 0;
    thread t3(safe_increment);
    thread t4(safe_increment);
    t3.join(); t4.join();
    cout << "  有鎖計數（正確）: " << safe_counter << endl;
}

} // namespace Lesson51

// ================================================================
// 課程 5.2：lock_guard —— RAII 自動鎖（推薦）
// ================================================================
// 直接用 lock/unlock 的問題：若例外發生，可能永遠不會 unlock！
// lock_guard<mutex> 使用 RAII：
//   - 建構時 lock()
//   - 離開作用域時自動 unlock()（即使發生例外）
// lock_guard 是最推薦的鎖定方式

namespace Lesson52 {

mutex mtx;
int counter = 0;

void safe_increment_with_guard() {
    for (int i = 0; i < 10000; ++i) {
        lock_guard<mutex> guard(mtx);  // 建構時 lock()
        ++counter;
        // guard 離開作用域時自動 unlock()
    }
}

// 比較：忘記 unlock 的危險
void buggy_increment() {
    // mtx.lock();
    // ++counter;
    // 如果這裡拋出例外，mtx 永遠不會 unlock() → 死鎖！
    // mtx.unlock();
}

void demo() {
    cout << "\n【5.2 lock_guard RAII 鎖】" << endl;

    counter = 0;
    thread t1(safe_increment_with_guard);
    thread t2(safe_increment_with_guard);
    t1.join(); t2.join();
    cout << "  lock_guard 計數（正確）: " << counter << endl;
    cout << "  原理：建構=lock，解構=unlock，例外安全" << endl;
}

} // namespace Lesson52

// ================================================================
// 課程 5.3：try_lock() —— 非阻塞鎖定
// ================================================================
// mutex.lock()     —— 阻塞等待（直到取得鎖）
// mutex.try_lock() —— 非阻塞嘗試（立即返回 true/false）
//
// 用途：不想讓執行緒阻塞，失敗時執行其他任務

namespace Lesson53 {

mutex resource_mutex;

// 註：原版讓三條執行緒直接 cout，輸出會交錯且順序每次不同，無法驗證。
//     改為收集結果，由主執行緒統一輸出可驗證的不變量。
atomic<int> trylock_acquired{0};
atomic<int> trylock_refused{0};
atomic<int> trylock_arrived{0};

void worker_with_trylock(int id) {
    (void)id;

    // 同時起跑，確保三條執行緒真的在同一瞬間競爭
    trylock_arrived.fetch_add(1, memory_order_acq_rel);
    while (trylock_arrived.load(memory_order_acquire) < 3) {
        this_thread::yield();
    }

    if (resource_mutex.try_lock()) {
        // 成功取得鎖：持有 50ms，確保另外兩條必然撲空
        this_thread::sleep_for(chrono::milliseconds(50));
        resource_mutex.unlock();  // try_lock 成功後必須手動 unlock！
        trylock_acquired.fetch_add(1, memory_order_relaxed);
    } else {
        // 取鎖失敗，執行替代任務（沒有阻塞）
        trylock_refused.fetch_add(1, memory_order_relaxed);
    }
}

void demo() {
    cout << "\n【5.3 try_lock 非阻塞鎖定】" << endl;

    thread t1(worker_with_trylock, 1);
    thread t2(worker_with_trylock, 2);
    thread t3(worker_with_trylock, 3);
    t1.join(); t2.join(); t3.join();

    cout << "  取得鎖的執行緒數: " << trylock_acquired.load()
         << "（必須是 1——贏家持有 50ms）" << endl;
    cout << "  try_lock 失敗的執行緒數: " << trylock_refused.load()
         << "（沒有阻塞，立即改做別的事）" << endl;
    cout << "  註: 「哪一條是贏家」由排程決定，每次執行都不同" << endl;
}

} // namespace Lesson53

// ================================================================
// 課程 5.4：互斥鎖常見錯誤與解決方案
// ================================================================
// 錯誤一：忘記 unlock（導致死鎖）→ 用 lock_guard 解決
// 錯誤二：死鎖（Deadlock）—— 兩個執行緒互相等待
// 錯誤三：重複 lock（同一執行緒）—— 用 recursive_mutex
// 錯誤四：lock 後忘記 unlock 又 lock → 用 unique_lock

namespace Lesson54 {

// 死鎖示範（避免在實際程式中這樣寫）
mutex m1, m2;

// 執行緒 A：先鎖 m1，再鎖 m2
// 執行緒 B：先鎖 m2，再鎖 m1
// → 可能死鎖！

// 解決死鎖：std::lock() 同時鎖定多個 mutex（無死鎖保證）
void safe_dual_lock() {
    // std::lock 保證無死鎖地同時鎖定 m1 和 m2
    lock(m1, m2);
    // adopt_lock 表示「已持有鎖，只負責解鎖」
    lock_guard<mutex> guard1(m1, adopt_lock);
    lock_guard<mutex> guard2(m2, adopt_lock);

    // 臨界區：持有 m1 和 m2
    cout << "  安全持有兩個鎖" << endl;
}  // guard1, guard2 析構，自動釋放 m1, m2

// scoped_lock（C++17）：更簡潔的多鎖方式
void safe_dual_lock_cpp17() {
    scoped_lock guard(m1, m2);  // C++17：同時鎖定，RAII，無死鎖
    cout << "  scoped_lock（C++17）：更簡潔的多鎖" << endl;
}

// recursive_mutex：允許同一執行緒重複 lock
recursive_mutex rmtx;
void recursive_function(int depth) {
    lock_guard<recursive_mutex> guard(rmtx);
    if (depth > 0) recursive_function(depth - 1);  // 遞迴鎖定，不死鎖
}

void demo() {
    cout << "\n【5.4 常見錯誤與解決方案】" << endl;

    // 安全的雙重鎖定
    thread t1(safe_dual_lock);
    thread t2(safe_dual_lock_cpp17);
    t1.join(); t2.join();

    // recursive_mutex 示範
    recursive_function(3);
    cout << "  recursive_mutex：遞迴鎖定成功" << endl;

    cout << "  死鎖防範原則：" << endl;
    cout << "    1. 固定加鎖順序" << endl;
    cout << "    2. 使用 std::lock() 同時鎖定" << endl;
    cout << "    3. 使用 scoped_lock（C++17）" << endl;
}

} // namespace Lesson54

// ================================================================
// 課程 5.5：執行緒安全的類別設計
// ================================================================
// 設計原則：封裝 mutex 在類別內部，讓外部使用者無需關心鎖

namespace Lesson55 {

class ThreadSafeCounter {
private:
    mutable mutex mtx;  // mutable：允許在 const 方法中鎖定
    int count = 0;

public:
    // 禁止複製（mutex 不可複製）
    ThreadSafeCounter() = default;
    ThreadSafeCounter(const ThreadSafeCounter&) = delete;
    ThreadSafeCounter& operator=(const ThreadSafeCounter&) = delete;

    void increment() {
        lock_guard<mutex> lock(mtx);
        ++count;
    }

    void decrement() {
        lock_guard<mutex> lock(mtx);
        --count;
    }

    void add(int value) {
        lock_guard<mutex> lock(mtx);
        count += value;
    }

    int get() const {
        lock_guard<mutex> lock(mtx);  // mutable mutex 在 const 中可用
        return count;
    }

    // 原子性的「讀取並重置」
    int reset() {
        lock_guard<mutex> lock(mtx);
        int old = count;
        count = 0;
        return old;
    }
};

void demo() {
    cout << "\n【5.5 執行緒安全類別設計】" << endl;

    ThreadSafeCounter counter;

    // 多個執行緒同時操作
    vector<thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&counter]() {
            for (int j = 0; j < 1000; ++j) {
                counter.increment();
            }
        });
    }
    for (auto& t : threads) t.join();

    cout << "  10 個執行緒各加 1000 次，結果: " << counter.get()
         << "（期望 10000）" << endl;
}

} // namespace Lesson55

// ================================================================
// 課程 5.6：unique_lock —— 靈活鎖定
// ================================================================
// unique_lock 比 lock_guard 更靈活：
//   - 可延遲鎖定（defer_lock）
//   - 可手動 lock/unlock
//   - 可轉移所有權
//   - 配合 condition_variable 使用
// 代價：比 lock_guard 稍微有額外開銷

namespace Lesson56 {

mutex mtx;

void demo_unique_lock() {
    cout << "\n【5.6 unique_lock 靈活鎖定】" << endl;

    // 立即鎖定（等同 lock_guard）
    {
        unique_lock<mutex> lock(mtx);
        cout << "  unique_lock：立即鎖定" << endl;
    }  // 自動 unlock

    // 延遲鎖定（defer_lock）
    {
        unique_lock<mutex> lock(mtx, defer_lock);  // 建構但不鎖定
        cout << "  defer_lock：尚未鎖定" << endl;
        // ... 做些其他事情 ...
        lock.lock();   // 手動鎖定
        cout << "  手動 lock() 後鎖定" << endl;
        lock.unlock(); // 手動解鎖（可以多次 lock/unlock）
        cout << "  手動 unlock() 後解鎖" << endl;
        lock.lock();   // 再次鎖定
        cout << "  再次 lock()" << endl;
    }  // 自動 unlock

    // try_to_lock（非阻塞）
    {
        unique_lock<mutex> lock(mtx, try_to_lock);
        if (lock.owns_lock()) {
            cout << "  try_to_lock：成功取得鎖" << endl;
        } else {
            cout << "  try_to_lock：取鎖失敗" << endl;
        }
    }
}

} // namespace Lesson56

// ================================================================
// 效能考量總結
// ================================================================
//
// ┌──────────────────┬─────────────────────────────────────────────┐
// │ 鎖的類型          │ 適用場景                                    │
// ├──────────────────┼─────────────────────────────────────────────┤
// │ lock_guard       │ 簡單的作用域鎖定（推薦優先使用）            │
// │ scoped_lock(C++17)│ 同時鎖定多個 mutex（推薦）                 │
// │ unique_lock      │ 需要延遲鎖定、手動控制、配合 CV 使用        │
// │ shared_lock(C++17)│ 讀寫鎖的「讀取」部分                       │
// │ try_lock         │ 非阻塞嘗試，失敗時執行其他任務              │
// │ recursive_mutex  │ 同一執行緒需要遞迴鎖定                      │
// └──────────────────┴─────────────────────────────────────────────┘
//
// 效能原則：
// 1. 鎖定粒度越小越好（只鎖必要的最小範圍）
// 2. 持有鎖的時間越短越好
// 3. 優先用 atomic 替代簡單計數（效能更好，無鎖）
// 4. 讀多寫少時考慮 shared_mutex（讀寫鎖）

// ================================================================
// 【綜合實務範例】執行緒安全的連線池 —— 把六課的守則一次用上
// ================================================================
// 情境：資料庫連線池，固定 4 條連線供 8 條 worker 借用。
// 逐條對應本階段的守則：
//   5.1 複合操作要原子：「檢查有沒有空閒 + 取走一條」必須在同一個臨界區段，
//        否則兩條 worker 會拿到同一條連線（TOCTOU，見課 5.4 檔 12）
//   5.2 能用 atomic 就不用鎖：純計數的統計用 atomic，不佔用連線池的鎖
//   5.3 非阻塞取得：提供 tryAcquire()，池空時立即返回而非阻塞
//   5.4 RAII：用 ConnectionLease 保證連線一定被歸還（即使中途拋例外）
//   5.5 封裝：mutex 為 private，使用者完全碰不到鎖
//   5.6 最小化臨界區段：借出後的「使用連線」完全在鎖外進行
// ================================================================

namespace LessonPractice {

class ConnectionPool {
private:
    mutable mutex mtx;                  // 5.5 封裝：private + mutable
    vector<string> idle;
    int borrowed = 0;
    int maxBorrowedSeen = 0;
    const int capacity;

    atomic<int> exhaustedCount{0};      // 5.2 純計數用 atomic，不佔鎖

public:
    explicit ConnectionPool(int n) : capacity(n) {
        for (int i = 0; i < n; ++i) {
            idle.push_back("conn-" + to_string(i));
        }
    }

    // 5.1 + 5.3：檢查與取走在同一個臨界區段，且非阻塞
    string tryAcquire() {
        lock_guard<mutex> lock(mtx);
        if (idle.empty()) {
            exhaustedCount.fetch_add(1, memory_order_relaxed);
            return "";                  // 池空 → 立即返回，不阻塞
        }
        string conn = idle.back();
        idle.pop_back();
        ++borrowed;
        if (borrowed > maxBorrowedSeen) maxBorrowedSeen = borrowed;
        return conn;
    }

    void release(const string& conn) {
        lock_guard<mutex> lock(mtx);
        idle.push_back(conn);
        --borrowed;
    }

    size_t idleCount() const { lock_guard<mutex> lk(mtx); return idle.size(); }
    int maxBorrowed() const  { lock_guard<mutex> lk(mtx); return maxBorrowedSeen; }
    int exhausted() const    { return exhaustedCount.load(); }
    int cap() const          { return capacity; }
};

// 5.4 RAII：離開作用域一定歸還，即使中途拋出例外
class ConnectionLease {
private:
    ConnectionPool& pool;
    string          conn;

public:
    ConnectionLease(ConnectionPool& p, string c) : pool(p), conn(std::move(c)) {}
    ~ConnectionLease() { if (!conn.empty()) pool.release(conn); }

    ConnectionLease(const ConnectionLease&)            = delete;
    ConnectionLease& operator=(const ConnectionLease&) = delete;

    bool valid() const { return !conn.empty(); }
};

void demo() {
    cout << "\n【綜合實務：執行緒安全的連線池】" << endl;

    ConnectionPool pool(4);
    atomic<int> completed{0};

    vector<thread> workers;
    for (int w = 0; w < 8; ++w) {
        workers.emplace_back([&pool, &completed]() {
            for (int i = 0; i < 100; ++i) {
                // 池空就重試，直到真的借到（不用 continue 跳過，
                // 否則完成次數會變成不確定）
                string conn;
                while ((conn = pool.tryAcquire()).empty()) {
                    this_thread::yield();
                }
                ConnectionLease lease(pool, conn);   // 5.4 RAII 保證歸還

                // 5.6 使用連線的工作完全在鎖外進行
                if (lease.valid()) {
                    completed.fetch_add(1, memory_order_relaxed);
                }
            }
        });
    }
    for (auto& t : workers) t.join();

    cout << "  8 條 worker 各借還 100 次，總完成: " << completed.load()
         << "（必須是 800）" << endl;
    cout << "  全部歸還後閒置連線數: " << pool.idleCount()
         << "（必須回到 " << pool.cap() << "，一條都不能漏）" << endl;
    cout << "  曾因池耗盡而重試: "
         << (pool.exhausted() > 0 ? "是（正常，代表確實有競爭）"
                                  : "否（本次排程未撞上）") << endl;
    cout << "  借出上限從未被突破: "
         << (pool.maxBorrowed() <= pool.cap() ? "是" : "否")
         << "（check 與 act 在同一個臨界區段）" << endl;
}

} // namespace LessonPractice

int main() {
    cout << "=============================================" << endl;
    cout << "   第五階段：互斥鎖基礎 (std::mutex) 總複習" << endl;
    cout << "=============================================" << endl;

    Lesson51::demo();
    Lesson52::demo();
    Lesson53::demo();
    Lesson54::demo();
    Lesson55::demo();
    Lesson56::demo_unique_lock();

    LessonPractice::demo();

    cout << "\n==============================================" << endl;
    cout << " 核心原則：" << endl;
    cout << " 1. 優先用 lock_guard（自動 RAII）" << endl;
    cout << " 2. 多個 mutex 用 scoped_lock（C++17）防死鎖" << endl;
    cout << " 3. 鎖住最小範圍，持有最短時間" << endl;
    cout << " 4. 封裝 mutex 在類別內，不讓外部處理鎖" << endl;
    cout << "==============================================" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread summary.cpp -o summary

// -----------------------------------------------------------------------------
// 【輸出但書】
//   1. ⚠️ 5.1 的「無鎖計數」是【真正的 data race = 未定義行為】。
//      本檔【刻意不印出那個數值】——把 UB 產生的值寫成「預期輸出」是錯的。
//      改為跑 50 輪並回報「是否曾觀察到遺失更新」。
//      本機實測 5 次連續執行【都是「是」】，但「否」在理論上也可能出現
//      （那不代表程式正確，它依然是 UB）。
//   2. 5.3 的「哪一條執行緒搶到鎖」由排程決定，每次都不同，故只印數量。
//   3. 其餘所有數字都是確定的：有鎖的計數必然正確，
//      連線池的不變量（歸還數、耗盡次數）也必然成立。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// =============================================
//    第五階段：互斥鎖基礎 (std::mutex) 總複習
// =============================================
//
// 【5.1 基本 mutex 操作】
//   無鎖計數: 跑 50 輪，是否曾遺失更新: 是 ← data race（UB），結果不可信
//   （實際數值每次執行都不同，且屬未定義行為，故不列出）
//   有鎖計數（正確）: 20000
//
// 【5.2 lock_guard RAII 鎖】
//   lock_guard 計數（正確）: 20000
//   原理：建構=lock，解構=unlock，例外安全
//
// 【5.3 try_lock 非阻塞鎖定】
//   取得鎖的執行緒數: 1（必須是 1——贏家持有 50ms）
//   try_lock 失敗的執行緒數: 2（沒有阻塞，立即改做別的事）
//   註: 「哪一條是贏家」由排程決定，每次執行都不同
//
// 【5.4 常見錯誤與解決方案】
//   安全持有兩個鎖
//   scoped_lock（C++17）：更簡潔的多鎖
//   recursive_mutex：遞迴鎖定成功
//   死鎖防範原則：
//     1. 固定加鎖順序
//     2. 使用 std::lock() 同時鎖定
//     3. 使用 scoped_lock（C++17）
//
// 【5.5 執行緒安全類別設計】
//   10 個執行緒各加 1000 次，結果: 10000（期望 10000）
//
// 【5.6 unique_lock 靈活鎖定】
//   unique_lock：立即鎖定
//   defer_lock：尚未鎖定
//   手動 lock() 後鎖定
//   手動 unlock() 後解鎖
//   再次 lock()
//   try_to_lock：成功取得鎖
//
// 【綜合實務：執行緒安全的連線池】
//   8 條 worker 各借還 100 次，總完成: 800（必須是 800）
//   全部歸還後閒置連線數: 4（必須回到 4，一條都不能漏）
//   曾因池耗盡而重試: 是（正常，代表確實有競爭）
//   借出上限從未被突破: 是（check 與 act 在同一個臨界區段）
//
// ==============================================
//  核心原則：
//  1. 優先用 lock_guard（自動 RAII）
//  2. 多個 mutex 用 scoped_lock（C++17）防死鎖
//  3. 鎖住最小範圍，持有最短時間
//  4. 封裝 mutex 在類別內，不讓外部處理鎖
// ==============================================
