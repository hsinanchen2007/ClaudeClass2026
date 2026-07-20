/*
 * ================================================================
 * 【第二階段：std::thread 基礎】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -pthread -o summary summary.cpp
 *
 * 本階段涵蓋課程：
 * - 課程 2.1：第一個多執行緒程式（建立、join、detach）
 * - 課程 2.2：執行緒函式的多種形式（函式、Lambda、成員函式、Functor）
 * - 課程 2.3：傳遞參數給執行緒（傳值、std::ref、std::move）
 * - 課程 2.4：join() 與 detach()（阻塞等待 vs 獨立運行）
 * - 課程 2.5：joinable() 狀態檢查（狀態轉換與安全模式）
 * - 課程 2.6：執行緒識別與資訊（get_id、hardware_concurrency、yield）
 * - 課程 2.7：執行緒的移動語意（move-only、容器管理、ScopedThread）
 * ================================================================
 */

// =============================================================================
//  summary.cpp  —  std::thread 的完整生命週期規格（第二階段教科書）
// =============================================================================
//
// 【主題資訊 Information】
//   class thread {                                    // C++11，標頭 <thread>
//     thread() noexcept;                              // 預設建構：不關聯任何執行緒
//     template<class F, class... Args>
//       explicit thread(F&& f, Args&&... args);       // 建構即啟動
//     thread(thread&& other) noexcept;                // move-only
//     thread& operator=(thread&& other) noexcept;     // ⚠️ 見【注意事項 2】
//     thread(const thread&)            = delete;      // 不可複製
//     thread& operator=(const thread&) = delete;
//     ~thread();                                      // joinable() → std::terminate()
//
//     bool joinable() const noexcept;
//     void join();                                    // 非 joinable → 丟 system_error
//     void detach();                                  // 非 joinable → 丟 system_error
//     id   get_id() const noexcept;
//     static unsigned hardware_concurrency() noexcept;   // 實作定義，可能回 0
//   };
//
//   複雜度：建立一條執行緒在 Linux 上約 10–30 µs（實作定義，本機量級）；
//           join() 是阻塞等待，成本取決於目標執行緒還要跑多久。
//   本階段全部設施皆為 **C++11**；C++20 的 std::jthread 見課程 1.5。
//
// 【詳細解釋 Explanation】
//
// 【1. joinable() 的精確定義 —— 這是整個生命週期的核心】
// 幾乎所有 std::thread 的規則都可以從 joinable() 推導出來。它的定義是：
//     joinable() == true  ⟺  這個 thread 物件目前「關聯著一條執行緒」
// 注意這裡說的是**關聯**，不是「執行緒還在跑」。這是最常見的誤解：
//     std::thread t([]{});                  // 任務瞬間就結束了
//     std::this_thread::sleep_for(1s);      // 確定它跑完了
//     t.joinable();                         // ← 仍然是 true！
// 執行緒**結束執行**不會讓 joinable() 變成 false。只有「你明確處理掉它」才會：
//     ┌─────────────────────────┬────────────────┐
//     │ 事件                    │ 之後 joinable()│
//     ├─────────────────────────┼────────────────┤
//     │ 預設建構 thread t;      │ false          │
//     │ thread t(f);            │ true           │
//     │ t.join()                │ false          │
//     │ t.detach()              │ false          │
//     │ thread t2 = move(t);    │ t=false,t2=true│
//     │ 任務函式執行完畢        │ **不變**（仍 true）│
//     └─────────────────────────┴────────────────┘
// 因為「執行緒已結束」與「你已經收尾」是兩件事：join() 除了等待，
// 還要**回收執行緒資源**（Linux 上對應 pthread_join，回收其 stack 與 TCB）。
// 沒有人回收，就是資源洩漏 —— 這正是解構子不能默默放過的原因。
//
// 【2. 為什麼解構子選擇 std::terminate（而不是自動 join 或 detach）】
// ⚠️ 先講清楚分類：**這不是 UB，是標準明文保證會發生的事**。
//    「解構時 joinable() 為 true → 呼叫 std::terminate()」是確定的行為，
//    不是「可能崩潰」。（相對地，detach 後存取已銷毀的變數才是 UB —— 見【注意事項 4】。）
//
// 委員會面對的是兩個都不好的預設值：
//   (a) 預設自動 join：解構點會**意外阻塞**。`}` 這個右大括號看起來不花時間，
//       實際上可能卡住數分鐘。更糟的是它會**掩蓋 bug**：你忘了同步，
//       程式卻因為解構順序碰巧正確而看似正常。
//   (b) 預設自動 detach：執行緒繼續跑，但它捕獲的區域變數已經銷毀
//       → 悄無聲息的 UB，而且往往在壓力測試才出現。
// 兩害相權，標準選擇「**強迫你明確表態**」：沒表態就當場終止，
// 讓錯誤在最靠近成因的地方、以最大的聲音爆出來。這是 C++ 少見的
// 「fail loudly」設計，正因為並行 bug 的除錯成本太高。
//   → C++20 的 std::jthread 之所以能安全地選 (a)，是因為它**同時**帶來了
//     stop_token：有了協作式取消，「解構時等它結束」才不會無限期阻塞。
//     兩者是成套設計，不是「C++20 終於想通了」。
//
// 【3. join() 與 detach() 的精確語意】
//   join()
//     * 阻塞呼叫端，直到目標執行緒的任務函式返回（或以例外離開 → terminate）。
//     * 回收執行緒資源；之後 joinable() == false。
//     * 建立 **happens-before**：目標執行緒中的所有寫入，對 join() 返回後的
//       呼叫端都是可見的。這點極重要 —— 它是本階段唯一的同步機制，
//       也是為什麼 demoThreadPool 那種「各自算完再 join 收集」不需要 mutex。
//     * 對 non-joinable 物件呼叫 → 丟 `std::system_error`
//       （本機實測 what()="Invalid argument"、code=22/EINVAL；可被 catch 接住，
//        接住就不會 terminate —— 見【面試題 陷阱 2】）。
//   detach()
//     * 立即返回；執行緒轉為背景獨立執行，與此物件脫鉤。
//     * 之後 joinable() == false，而且**標準沒有提供任何方法再等它**。
//     * main() 返回後，整個行程開始收尾；detached 執行緒可能在任意一點被
//       強制結束，其堆疊上的 RAII 解構子**不保證執行**（檔案 buffer 可能沒 flush）。
//
// 【4. move-only 的必然性】
// std::thread 不可複製，這不是「為了效能」，而是**所有權模型的必然結果**：
// 一條 OS 執行緒只能被回收（join）一次。若允許複製，兩個物件關聯同一條執行緒，
// 就會出現「誰負責 join」的問題，且第二次 join 必定失敗。
// 因此 std::thread 採用 unique_ptr 式的獨佔所有權：只能移動，不能複製。
//   * 函式可以**回傳** std::thread（見 lesson_2_7::createWorker，走移動/RVO）。
//   * 函式可以**接收** std::thread by value（呼叫端須 std::move）。
//   * 放進 std::vector 必須用 emplace_back 或 push_back(std::move(t))。
//
// 【5. 參數傳遞：decay-copy 規則（課程 2.3 的底層機制）】
// 上面課程 2.3 的說明「std::thread 預設複製所有傳入的參數」是對的，
// 但底層機制值得講精確，因為它決定了哪些寫法會**編譯失敗**：
//   建構子會對每個參數做 **decay-copy**（衰退複製）：陣列/函式衰退成指標、
//   去掉 cv 與 reference，把結果存進執行緒自己的儲存區；
//   真正呼叫時，這些儲存的副本是以 **rvalue** 傳給你的函式。
//   後果：
//     void f(int& x);
//     int a = 0;
//     std::thread t(f, a);      // ← **編譯錯誤**，不是靜默複製！
//   本機 g++ 15.2.0 的錯誤訊息是：
//     static assertion failed: std::thread arguments must be invocable
//     after conversion to rvalues
//   因為存起來的副本是 rvalue，無法綁定到 non-const 的 `int&`。
//   要傳真正的引用必須用 `std::ref(a)`（包成 reference_wrapper，
//   它有隱式轉換到 T&），本機實測 `std::thread t(f, std::ref(a))` 後 a==42。
//   ⚠️ 注意 `const int&` 則可以編譯 —— 但綁到的是**執行緒內的副本**，
//      不是你的原變數，這比編譯錯誤更隱蔽。
//
// 【6. hardware_concurrency() 是「提示」，不是保證】
//   * 回傳值是**實作定義**的。本機實測 = 16（與 nproc 一致）。
//   * 標準允許回傳 **0**，代表「無法判定」。所以任何正式程式碼都必須寫
//     `unsigned n = std::thread::hardware_concurrency(); if (n == 0) n = 4;`
//     （上面課程 2.6 的範例正是這樣寫的，那不是多餘的防禦）。
//   * 它**不知道** cgroup / taskset / 容器的 CPU 配額。在 Kubernetes 裡
//     limits.cpu=2 的容器中，它常常仍回報宿主機的核心數（例如 16），
//     導致開出遠超配額的執行緒數而互相搶佔。容器環境要另外讀
//     cgroup 的 cpu.max，或用 sched_getaffinity。
//
// 【概念補充 Concept Deep Dive】
// (A) 一條 std::thread 在 OS 層是什麼
//     Linux/glibc(NPTL) 是 **1:1 模型**：一個 std::thread = 一個 kernel 排程實體，
//     底層是 clone(CLONE_VM|CLONE_THREAD|CLONE_SETTLS|...)。
//     共享：位址空間（heap、全域、程式碼）、檔案描述子表。
//     各自擁有：stack（Linux 預設 8 MB 虛擬保留，實體記憶體按需配置）、
//               暫存器狀態、TLS（thread_local 變數）、errno。
//     → 「開 10000 條執行緒」的問題不在 8 MB×10000 的虛擬位址（64-bit 夠用），
//       而在 kernel 的排程負擔與 context switch（user→user 約 1–3 µs）。
//       這是第三階段 thread pool 存在的理由。
// (B) join() 底下發生什麼
//     對應 pthread_join：在目標執行緒的 exit futex 上等待。若目標已結束，
//     join() 立即返回；否則呼叫端進入睡眠，由 kernel 在目標結束時喚醒。
//     這裡的 futex 操作同時提供了【詳細解釋 3】說的 happens-before 保證。
// (C) std::thread::id 會被重用
//     id 只保證「在同一時刻，不同的活躍執行緒有不同的 id」。
//     一條執行緒結束並被回收後，它的 id **可以**被之後建立的執行緒重用。
//     所以不可以把 thread::id 當成「永久唯一識別碼」存進 log 做長期關聯。
//     預設建構的 thread（不關聯執行緒）其 id 是一個特殊值，
//     所有這類物件的 id 彼此相等。
// (D) Most Vexing Parse（課程 2.2 提到的陷阱，這裡補上原理）
//         std::thread t(Counter());
//     C++ 的文法允許把它解析成「宣告一個名為 t 的**函式**，
//     回傳 std::thread，參數是一個『回傳 Counter 的無參數函式指標』」。
//     只要一個東西**能**被解析成宣告，標準就規定必須解析成宣告。
//     解法：`std::thread t{Counter()};`（大括號無法是函式宣告，推薦）
//           或 `std::thread t((Counter()));`（多一層括號破壞文法）。
//
// 【注意事項 Pay Attention】
// 1. **會呼叫 std::terminate() 的四種情況（標準保證，不是 UB）**：
//    (a) 解構時 joinable() 仍為 true；
//    (b) **移動指派**到一個 joinable 的物件（`t1 = std::move(t2);` 而 t1 還關聯著
//        執行緒）—— 等同於在解構前沒收尾；
//    (c) 任務函式以未捕捉的例外離開（例外**不會**跨執行緒傳播到 join() 端，
//        想傳遞例外要用 std::future / std::promise，見第四階段）；
//    (d) join()/detach() 丟出的 system_error 沒被接住而傳到 main 之外。
// 2. 承上 (b)：`t = std::thread(f);` 這種「重新指派」的寫法，只有在 t 目前
//    non-joinable 時才安全。上面 lesson_2_5::SafeThread::start() 先寫
//    `if (t.joinable()) t.join();` 就是為了這件事，不是多此一舉。
// 3. `join()` 之後**不能**再 join；第二次會丟 std::system_error。
//    這與 (a) 的 terminate 是不同機制：例外可以被接住，terminate 不行。
// 4. **detach + 區域變數是 UB，不可描述成固定結果**：
//    detached 執行緒存取已銷毀的變數，可能印出舊值、可能印出垃圾、
//    可能什麼都不印、也可能崩潰 —— 標準未定義，且會隨最佳化等級、
//    堆疊重用情形而改變。上面課程 2.4 把危險示範註解起來是正確做法。
// 5. 多執行緒同時寫 `std::cout`：C++11 起標準保證不會有 **data race**
//    （不會 UB），但**完全不保證輸出不交錯**。demoMultiple() 印出的 A/B
//    順序每次執行都不同，這是預期行為，不是 bug。
//    要「整行不被切斷」需自己加鎖，或用 C++20 的 std::osyncstream。
// 6. `hardware_concurrency()` 可能回 0，且不感知容器配額（見【詳細解釋 6】）。
// 7. 執行緒的建立成本不低（本機量級 10–30 µs）。工作若只有幾微秒，
//    開執行緒的代價會遠大於工作本身 —— 那是 thread pool 的領域。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::thread 生命週期
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::thread 解構時如果還是 joinable，會發生什麼？為什麼這樣設計？
//     答：**標準保證呼叫 std::terminate()**，程式當場中止（這不是 UB，是明文
//         規定的確定行為）。設計理由是兩個候選預設值都更糟：自動 join 會讓
//         解構點意外阻塞並掩蓋忘記同步的 bug；自動 detach 會讓執行緒存取
//         已銷毀的物件造成隱蔽 UB。標準因此強迫使用者明確表態。
//     追問：那 C++20 的 jthread 為什麼就敢自動 join？
//         → 因為它同時引入 stop_token：解構是 request_stop() + join()，
//           有協作式取消才不會無限期阻塞。兩者是成套設計。
//
// 🔥 Q2. joinable() 什麼時候是 false？執行緒「跑完了」算不算？
//     答：**不算**。任務函式執行完畢不會改變 joinable()。false 只有四種來源：
//         預設建構、已 join()、已 detach()、已被 move 走。
//         因為 join() 除了等待還要回收執行緒資源，「結束執行」與「已收尾」
//         是兩件不同的事。
//     追問：那 t.join() 之後再呼叫一次 t.join() 會怎樣？
//         → 丟 std::system_error（本機實測 code=22/EINVAL）。它是**例外**，
//           可以被 catch 接住；接住就不會 terminate。
//
// 🔥 Q3. 為什麼 std::thread 不能複製？
//     答：一條 OS 執行緒只能被回收（join）一次。若可複製，兩個物件關聯同一條
//         執行緒，就無法回答「誰負責 join」，而且第二次 join 必然失敗。
//         所以它採用 unique_ptr 式的獨佔所有權語意：move-only。
//     追問：那要放進 std::vector 怎麼辦？
//         → emplace_back 就地建構，或 push_back(std::move(t))。
//
// 🔥 Q4. void f(int&); int a; std::thread t(f, a); 這樣寫 a 會被修改嗎？
//     答：這行**根本編譯不過**。std::thread 對參數做 decay-copy 存進自己的
//         儲存區，呼叫時以 rvalue 傳入，無法綁到 non-const 的 int&。
//         本機 g++ 的訊息是 "std::thread arguments must be invocable after
//         conversion to rvalues"。要傳引用必須用 std::ref(a)。
//     追問：如果參數型別改成 const int& 呢？
//         → 那就編得過，但綁到的是**執行緒內的副本**，改動看不到、
//           而且沒有任何警告。這比編譯錯誤更危險。
//
// ⚠️ 陷阱 1. 「執行緒已經跑完了，所以不 join 也沒關係」——對嗎？
//     答：錯，而且是最常見的致命錯誤。任務跑完不會讓 joinable() 變 false，
//         解構時仍然 joinable → **std::terminate()**。
//         不論任務多短、你 sleep 多久確保它跑完，都一樣。
//     為什麼會錯：把 joinable() 想成「執行緒還活著嗎」。它真正的意思是
//         「這個物件還關聯著一條**尚未被回收**的執行緒嗎」。資源回收這件事
//         必須由你明確觸發，OS 不會因為函式返回就自動幫你做完。
//
// ⚠️ 陷阱 2. 在解構子裡寫 `t.join();` 就安全了吧？
//     答：不夠。若 t 已經是 non-joinable（例如稍早被 move 走或已 join 過），
//         join() 會丟 std::system_error；而**在解構子中拋出例外**會直接
//         導致 std::terminate（解構子預設 noexcept）。
//         正確寫法一定是 `if (t.joinable()) t.join();`
//         —— 上面 lesson_2_5::SafeThread 與 lesson_2_7::ScopedThread 都是這個模式。
//     為什麼會錯：以為「join 一定安全」。join() 是有前置條件的操作，
//         而解構子是最不能拋例外的地方，兩者相遇就是 terminate。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <memory>
#include <vector>
#include <atomic>
#include <stdexcept>
#include <mutex>                // LeetCode 1114 與實務範例需要
#include <condition_variable>   // LeetCode 1114 的順序控制
#include <functional>           // std::function（1114 的題目介面）、std::ref
#include <numeric>              // std::accumulate（實務範例 1 彙總結果）
#include <algorithm>            // std::sort（實務範例輸出穩定化）


// ===== 課程 2.1：第一個多執行緒程式 =====
//
// std::thread 是 C++11 引入的執行緒類別，建立物件時傳入可呼叫物件，
// 執行緒立即開始執行。
//
// 生命週期規則（非常重要！）：
//   建構 → 執行中 → 必須呼叫 join() 或 detach()
//   若在解構前未呼叫，程式會呼叫 std::terminate() 崩潰。
//
// join()：阻塞等待執行緒結束，之後 joinable() = false
// detach()：讓執行緒在背景獨立運行，之後 joinable() = false
//           主程式結束時，尚未完成的 detach 執行緒會被強制終止
//
// 多執行緒的輸出順序是不確定的（非確定性）。

namespace lesson_2_1 {

void sayHello() {
    std::cout << "[2.1] Hello from thread!" << std::endl;
}

// 展示 join() 的阻塞行為
void demoJoin() {
    std::cout << "[2.1] --- join() 示範 ---" << std::endl;

    std::thread t([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "[2.1]   子執行緒完成工作" << std::endl;
    });

    std::cout << "[2.1]   主執行緒等待中..." << std::endl;
    t.join();  // 阻塞到 t 完成
    std::cout << "[2.1]   join() 返回，繼續執行" << std::endl;
}

// 展示多執行緒同時執行（輸出順序不確定）
void demoMultiple() {
    std::cout << "[2.1] --- 多執行緒示範 ---" << std::endl;

    std::thread t1([]() {
        for (int i = 0; i < 3; ++i) std::cout << "A";
    });
    std::thread t2([]() {
        for (int i = 0; i < 3; ++i) std::cout << "B";
    });

    t1.join();
    t2.join();
    std::cout << std::endl;
    std::cout << "[2.1]   輸出順序不確定，這是多執行緒的本質" << std::endl;
}

} // namespace lesson_2_1


// ===== 課程 2.2：執行緒函式的多種形式 =====
//
// std::thread 接受任何「可呼叫物件（Callable）」：
//
// 1. 一般函式（Function）     - thread(func)
// 2. Lambda 表達式            - thread([](){})，最靈活，可捕獲外部變數
// 3. 成員函式（Member Func）  - thread(&Class::method, &obj)
//    語法：成員函式指標 + 物件指標 + 額外參數
// 4. 函式物件（Functor）      - thread{MyFunctor()}
//    注意 Most Vexing Parse：thread(MyFunctor()) 被解析為函式宣告
//    解決：thread((MyFunctor())) 或 thread{MyFunctor()}（推薦）

namespace lesson_2_2 {

// 形式 1：一般函式
void normalFunction() {
    std::cout << "[2.2]   [一般函式] 執行中" << std::endl;
}

// 形式 3：成員函式
class Worker {
public:
    void doWork() {
        std::cout << "[2.2]   [成員函式] 執行中" << std::endl;
    }
    void doWorkWithId(int id) {
        std::cout << "[2.2]   [成員函式帶參數] Worker " << id << " 執行中" << std::endl;
    }
};

// 形式 4：函式物件（Functor）
class Counter {
    int count;
public:
    explicit Counter(int c) : count(c) {}
    void operator()() const {
        std::cout << "[2.2]   [Functor] 計數：";
        for (int i = 0; i < count; ++i) std::cout << i << " ";
        std::cout << std::endl;
    }
};

void demoAllForms() {
    std::cout << "[2.2] --- 四種可呼叫物件示範 ---" << std::endl;

    Worker worker;

    // 1. 一般函式
    std::thread t1(normalFunction);

    // 2. Lambda（無捕獲）
    std::thread t2([]() {
        std::cout << "[2.2]   [Lambda] 執行中" << std::endl;
    });

    // 2b. Lambda（有捕獲）
    int value = 42;
    std::thread t2b([value]() {
        std::cout << "[2.2]   [Lambda 捕獲] value = " << value << std::endl;
    });

    // 3. 成員函式（需要物件指標）
    std::thread t3(&Worker::doWork, &worker);
    std::thread t3b(&Worker::doWorkWithId, &worker, 99);

    // 4. 函式物件（大括號初始化避免 Most Vexing Parse）
    std::thread t4{Counter(4)};

    t1.join(); t2.join(); t2b.join();
    t3.join(); t3b.join(); t4.join();
}

} // namespace lesson_2_2


// ===== 課程 2.3：傳遞參數給執行緒 =====
//
// std::thread 預設【複製】所有傳入的參數，即使函式期望引用也一樣。
//
// 傳遞方式總結：
// ┌─────────────────────────────────────────────┐
// │  傳值        thread(f, arg)    複製 arg      │
// │  傳引用      thread(f, ref(x)) 傳原始變數    │
// │  傳const引用 thread(f, cref(x))只讀          │
// │  傳指標      thread(f, &arg)   位址被複製    │
// │  移動語意    thread(f, move(p)) 轉移所有權   │
// └─────────────────────────────────────────────┘
//
// 傳字串陷阱：傳遞 char* 給期望 std::string 的函式，
//             配合 detach() 可能在轉換前緩衝區就被銷毀。
//             解決：明確轉換 std::string(buffer)。
//
// std::move() 用於只能移動的物件（如 std::unique_ptr）。

namespace lesson_2_3 {

void byValue(int x) {
    x = 999;  // 不影響原值
    std::cout << "[2.3]   byValue: x = " << x << "（副本被修改）" << std::endl;
}

void byRef(int& x) {
    x = 999;  // 修改原值
    std::cout << "[2.3]   byRef: x 已被修改為 " << x << std::endl;
}

void byString(const std::string& s) {
    std::cout << "[2.3]   byString: \"" << s << "\"" << std::endl;
}

void processUniquePtr(std::unique_ptr<int> ptr) {
    std::cout << "[2.3]   processUniquePtr: *ptr = " << *ptr << std::endl;
}

void demoParams() {
    std::cout << "[2.3] --- 參數傳遞示範 ---" << std::endl;

    int a = 1, b = 1;

    // 傳值：a 不被修改
    std::thread t1(byValue, a);
    t1.join();
    std::cout << "[2.3]   a = " << a << "（未被修改，如預期）" << std::endl;

    // std::ref 傳引用：b 被修改
    std::thread t2(byRef, std::ref(b));
    t2.join();
    std::cout << "[2.3]   b = " << b << "（已被修改）" << std::endl;

    // 明確轉換字串（避免 char* 陷阱）
    std::thread t3(byString, std::string("安全字串"));
    t3.join();

    // std::move 傳遞 unique_ptr（不可複製，只能移動）
    auto ptr = std::make_unique<int>(42);
    std::thread t4(processUniquePtr, std::move(ptr));
    t4.join();
    // ptr 現在是空的（所有權已轉移）
    std::cout << "[2.3]   ptr 移動後是否為空：" << (ptr == nullptr ? "是" : "否") << std::endl;
}

} // namespace lesson_2_3


// ===== 課程 2.4：join() 與 detach() =====
//
// 每個 std::thread 解構前必須明確選擇：
//
// join()
//   - 阻塞當前執行緒，直到目標執行緒結束
//   - 確保執行緒完成工作後才繼續
//   - 更安全，大多數情況應優先使用
//
// detach()
//   - 讓執行緒在背景獨立運行
//   - 立即返回，不等待
//   - 危險：detach 的執行緒不應捕獲或操作可能被提前銷毀的區域變數
//   - 適用場景：真正的背景任務（如日誌記錄器）
//
// 何時選擇？
//   需要結果 / 後續依賴執行緒完成 / 使用區域變數  → join
//   真正的背景任務 / 不需要結果                    → detach（謹慎）

namespace lesson_2_4 {

void demoJoinVsDetach() {
    std::cout << "[2.4] --- join vs detach 示範 ---" << std::endl;

    // join：確保完成
    std::thread t1([]() {
        std::cout << "[2.4]   [t1] 開始工作" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::cout << "[2.4]   [t1] 工作完成" << std::endl;
    });
    std::cout << "[2.4]   [main] 等待 t1..." << std::endl;
    t1.join();
    std::cout << "[2.4]   [main] t1 已結束，繼續" << std::endl;

    // detach：背景運行
    std::thread t2([]() {
        std::cout << "[2.4]   [t2] 背景執行中" << std::endl;
    });
    t2.detach();
    std::cout << "[2.4]   [main] t2 已分離（背景運行）" << std::endl;

    // 給 t2 一點時間完成輸出
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

// 危險示範（僅說明，此處不實際執行不安全版本）：
// void dangerous() {
//     int localVar = 42;
//     std::thread t([&localVar]() {  // 捕獲區域變數引用
//         std::this_thread::sleep_for(...);
//         std::cout << localVar; // localVar 已被銷毀！未定義行為
//     });
//     t.detach();
// }  // localVar 在此銷毀，但執行緒仍在運行

} // namespace lesson_2_4


// ===== 課程 2.5：joinable() 狀態檢查 =====
//
// joinable() 返回 true 表示執行緒物件關聯著一個真正的執行緒，
// 且尚未被 join 或 detach。
//
// joinable() == false 的情況：
//   - 預設建構的 std::thread（無關聯執行緒）
//   - 呼叫 join() 之後
//   - 呼叫 detach() 之後
//   - 被 std::move() 奪走所有權之後
//
// 對 non-joinable 執行緒呼叫 join() 或 detach() 會拋出 std::system_error。
//
// 最佳實踐：在類別的解構函式中一定要檢查 joinable() 後再 join。

namespace lesson_2_5 {

void demoJoinable() {
    std::cout << "[2.5] --- joinable() 狀態示範 ---" << std::endl;

    // 預設建構：不 joinable
    std::thread t0;
    std::cout << "[2.5]   預設建構 joinable: " << t0.joinable() << std::endl;

    // 帶執行緒：joinable
    std::thread t1([]() {});
    std::cout << "[2.5]   建立後 joinable: " << t1.joinable() << std::endl;
    t1.join();
    std::cout << "[2.5]   join 後 joinable: " << t1.joinable() << std::endl;

    // detach 後：不 joinable
    std::thread t2([]() {});
    t2.detach();
    std::cout << "[2.5]   detach 後 joinable: " << t2.joinable() << std::endl;

    // move 後：原物件不 joinable，新物件 joinable
    std::thread t3([]() {});
    std::thread t4 = std::move(t3);
    std::cout << "[2.5]   move 後原物件 joinable: " << t3.joinable() << std::endl;
    std::cout << "[2.5]   move 後新物件 joinable: " << t4.joinable() << std::endl;
    t4.join();
}

// 安全的執行緒管理類別：解構時自動 join
class SafeThread {
    std::thread t;
public:
    template<typename Func, typename... Args>
    void start(Func&& f, Args&&... args) {
        if (t.joinable()) t.join();  // 先結束舊的
        t = std::thread(std::forward<Func>(f), std::forward<Args>(args)...);
    }

    void join() {
        if (t.joinable()) t.join();
    }

    ~SafeThread() {
        join();  // 解構時自動 join，避免 terminate
    }
};

void demoSafeThread() {
    std::cout << "[2.5] --- SafeThread 示範 ---" << std::endl;
    SafeThread st;
    st.start([]() { std::cout << "[2.5]   SafeThread 任務 1" << std::endl; });
    st.start([]() { std::cout << "[2.5]   SafeThread 任務 2" << std::endl; });
    // 解構時自動 join 任務 2
}

} // namespace lesson_2_5


// ===== 課程 2.6：執行緒識別與資訊 =====
//
// 取得執行緒 ID：
//   std::this_thread::get_id()     - 在執行緒內部取得自己的 ID
//   thread_object.get_id()         - 在外部取得指定執行緒的 ID
//   std::thread::id 型別可比較和輸出
//
// 查詢硬體資訊：
//   std::thread::hardware_concurrency() - 回傳 CPU 核心數（可能為 0）
//   回傳 0 時應使用預設值（如 2 或 4）
//
// std::this_thread 命名空間：
//   get_id()      - 取得當前執行緒 ID
//   sleep_for()   - 休眠指定時間段
//   sleep_until() - 休眠到指定時間點
//   yield()       - 讓出 CPU 時間給其他執行緒（避免忙等待）

namespace lesson_2_6 {

// 儲存主執行緒 ID，供子執行緒比較
std::thread::id mainThreadId;

void checkThreadIdentity() {
    if (std::this_thread::get_id() == mainThreadId) {
        std::cout << "[2.6]   這是主執行緒" << std::endl;
    } else {
        std::cout << "[2.6]   這是子執行緒，ID: "
                  << std::this_thread::get_id() << std::endl;
    }
}

void demoThreadInfo() {
    std::cout << "[2.6] --- 執行緒識別與資訊示範 ---" << std::endl;

    mainThreadId = std::this_thread::get_id();
    std::cout << "[2.6]   主執行緒 ID: " << mainThreadId << std::endl;

    // 主執行緒身份確認
    checkThreadIdentity();

    // 子執行緒身份確認
    std::thread t(checkThreadIdentity);
    std::cout << "[2.6]   從外部看 t 的 ID: " << t.get_id() << std::endl;
    t.join();

    // 查詢 CPU 核心數
    unsigned int cores = std::thread::hardware_concurrency();
    if (cores == 0) cores = 2;  // 無法偵測時使用預設值
    std::cout << "[2.6]   硬體並行執行緒數: " << cores << std::endl;
}

// yield() 應用：避免忙等待
void demoYield() {
    std::cout << "[2.6] --- yield() 示範 ---" << std::endl;

    std::atomic<bool> ready{false};

    std::thread t([&ready]() {
        while (!ready) {
            std::this_thread::yield();  // 讓出 CPU，避免忙等待浪費資源
        }
        std::cout << "[2.6]   子執行緒收到信號，開始執行" << std::endl;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    ready = true;
    t.join();
}

// 根據核心數建立執行緒池
void demoThreadPool() {
    std::cout << "[2.6] --- 根據核心數分配工作 ---" << std::endl;

    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 2;
    std::cout << "[2.6]   建立 " << numThreads << " 個工作執行緒" << std::endl;

    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < numThreads; ++i) {
        threads.emplace_back([i]() {
            std::cout << "[2.6]   工作執行緒 " << i
                      << " (ID: " << std::this_thread::get_id() << ")" << std::endl;
        });
    }
    for (auto& t : threads) t.join();
}

} // namespace lesson_2_6


// ===== 課程 2.7：執行緒的移動語意 =====
//
// std::thread 是「只能移動（move-only）、不能複製」的型別。
//
// 原因：一個執行緒只能有一個擁有者。若允許複製，兩個物件指向同一個
//       執行緒，誰負責 join？重複 join 會崩潰。
//
// 移動操作：
//   std::thread t2 = std::move(t1);
//   移動後：t1 變成 non-joinable，t2 取得所有權。
//
// 函式可以回傳 std::thread（編譯器自動移動）。
// 函式可以接受 std::thread 參數（呼叫端需 std::move）。
//
// 警告：不可移動到仍是 joinable 的執行緒物件，否則 std::terminate()。
//
// 執行緒容器：std::vector<std::thread>，用 emplace_back 或 push_back 搭配 move。
//
// ScopedThread：RAII 包裝類別，離開作用域自動 join。

namespace lesson_2_7 {

// 複製會導致編譯錯誤（下面被注解的行）：
// std::thread t2 = t1;   // 錯誤：use of deleted function
// std::thread t2(t1);    // 錯誤：use of deleted function

// 函式回傳 std::thread（自動移動，RVO）
std::thread createWorker(int id) {
    return std::thread([id]() {
        std::cout << "[2.7]   工廠建立的執行緒 " << id << " 執行中" << std::endl;
    });
}

// 函式接受 std::thread（取得所有權）
void takeOwnership(std::thread t) {
    std::cout << "[2.7]   takeOwnership：取得執行緒所有權" << std::endl;
    if (t.joinable()) t.join();
}

// RAII 包裝：解構時自動 join
class ScopedThread {
    std::thread t;
public:
    explicit ScopedThread(std::thread thread)
        : t(std::move(thread)) {
        if (!t.joinable()) {
            throw std::logic_error("ScopedThread：執行緒不可 join");
        }
    }

    ~ScopedThread() {
        t.join();  // 自動 join
    }

    // 禁止複製
    ScopedThread(const ScopedThread&) = delete;
    ScopedThread& operator=(const ScopedThread&) = delete;

    // 允許移動
    ScopedThread(ScopedThread&&) = default;
    ScopedThread& operator=(ScopedThread&&) = default;
};

void demoMoveSemantics() {
    std::cout << "[2.7] --- 移動語意示範 ---" << std::endl;

    // 手動移動所有權
    std::thread t1([]() {
        std::cout << "[2.7]   t1 的任務執行中" << std::endl;
    });
    std::cout << "[2.7]   t1 joinable: " << t1.joinable() << std::endl;

    std::thread t2 = std::move(t1);  // 所有權轉移

    std::cout << "[2.7]   移動後 t1 joinable: " << t1.joinable() << "（已空）" << std::endl;
    std::cout << "[2.7]   移動後 t2 joinable: " << t2.joinable() << "（取得所有權）" << std::endl;
    t2.join();  // 由 t2 負責
}

void demoFactory() {
    std::cout << "[2.7] --- 工廠函式示範 ---" << std::endl;

    std::thread t = createWorker(42);
    t.join();
}

void demoTakeOwnership() {
    std::cout << "[2.7] --- 所有權轉移到函式 ---" << std::endl;

    std::thread t([]() {
        std::cout << "[2.7]   工作執行緒" << std::endl;
    });
    takeOwnership(std::move(t));
    std::cout << "[2.7]   t joinable 後: " << t.joinable() << std::endl;
}

void demoThreadVector() {
    std::cout << "[2.7] --- std::vector<thread> 示範 ---" << std::endl;

    std::vector<std::thread> threads;

    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([i]() {
            std::cout << "[2.7]   向量中的執行緒 " << i << std::endl;
        });
    }

    for (auto& t : threads) {
        t.join();
    }
}

void demoScopedThread() {
    std::cout << "[2.7] --- ScopedThread RAII 示範 ---" << std::endl;

    {
        ScopedThread st(std::thread([]() {
            std::cout << "[2.7]   ScopedThread 的任務" << std::endl;
        }));
        // 離開作用域時自動 join
    }
    std::cout << "[2.7]   ScopedThread 已自動 join" << std::endl;
}

} // namespace lesson_2_7


// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 1114. Print in Order
//   題目：同一個 Foo 物件的 first()/second()/third() 會被**三條不同的執行緒**
//         各呼叫一次，但呼叫順序任意；請保證輸出一定是 "onetwothree"。
//
//   為什麼用到本主題：本階段唯一學過的同步工具是 join()，而 join() 只能由
//   「持有 thread 物件的人」使用。本題刻意把執行緒的建立權拿走 ——
//   你無法決定誰先啟動，也不能對別人的執行緒 join。
//   這正好凸顯了 join() 的適用邊界：
//     * join() 建立的是「執行緒**整體結束** happens-before 我繼續」；
//     * 本題需要的是「函式**內部某一點**之後，另一條執行緒才能繼續」，
//       粒度更細，join() 做不到 → 必須引入 condition_variable。
//   這就是第三階段（同步機制）存在的理由，也是本階段最好的收尾。
//
//   作法：用一個受 mutex 保護的 step_ 狀態機（1→2→3），
//         每個階段用 predicate 迴圈等自己的號碼被叫到。
//   ⚠️ 必須用 predicate 版 wait：spurious wakeup 是標準允許的。
// -----------------------------------------------------------------------------
namespace leetcode_1114 {

class Foo {
    std::mutex              m_;
    std::condition_variable cv_;
    int                     step_ = 1;   // 下一個該執行的階段
public:
    void first(std::function<void()> printFirst) {
        std::unique_lock<std::mutex> lk(m_);
        printFirst();
        step_ = 2;
        lk.unlock();          // 先解鎖再通知，避免被喚醒者立刻又卡在鎖上
        cv_.notify_all();
    }

    void second(std::function<void()> printSecond) {
        std::unique_lock<std::mutex> lk(m_);
        cv_.wait(lk, [this] { return step_ == 2; });   // predicate 迴圈
        printSecond();
        step_ = 3;
        lk.unlock();
        cv_.notify_all();
    }

    void third(std::function<void()> printThird) {
        std::unique_lock<std::mutex> lk(m_);
        cv_.wait(lk, [this] { return step_ == 3; });
        printThird();
    }
};

// 刻意以「相反順序」啟動三條執行緒，證明輸出順序與啟動順序無關。
std::string run() {
    Foo foo;
    std::string out;
    std::mutex outMtx;
    auto emit = [&out, &outMtx](const char* s) {
        std::lock_guard<std::mutex> lk(outMtx);
        out += s;
    };

    std::thread t3([&] { foo.third ([&] { emit("three"); }); });
    std::thread t2([&] { foo.second([&] { emit("two");   }); });
    std::thread t1([&] { foo.first ([&] { emit("one");   }); });

    t3.join();   // join 的順序不影響結果 —— 順序是由 cv 保證的，不是 join
    t2.join();
    t1.join();
    return out;
}

} // namespace leetcode_1114


// -----------------------------------------------------------------------------
// 【日常實務範例 1】批次影像縮圖處理（Batch Thumbnail Pipeline）
//   情境：上傳服務收到一批圖片，要為每張產生縮圖並算出校驗碼。
//         這是典型的 **embarrassingly parallel**（各張圖互不相干）工作，
//         正好用本階段的 hardware_concurrency + vector<thread> + join 解決。
//
//   為什麼用到本主題：
//     * 課程 2.6：用 hardware_concurrency() 決定切幾份（並處理回傳 0 的情況）
//     * 課程 2.7：std::thread 是 move-only，只能用 emplace_back 放進 vector
//     * 課程 2.4：join() 建立 happens-before —— 這是本範例**不需要 mutex**
//                 保護 results 的原因：每條執行緒只寫自己那一格，
//                 主執行緒 join 完才讀，寫入對讀取可見。
// -----------------------------------------------------------------------------
namespace practical_thumbnails {

struct Image {
    std::string name;
    int         width;
    int         height;
};

struct Thumb {
    std::string name;
    int         width;
    int         height;
    unsigned    checksum;
};

// 模擬縮圖：等比例縮到寬度 <= 128，並算一個決定性的校驗碼
Thumb makeThumbnail(const Image& img) {
    int w = img.width, h = img.height;
    while (w > 128) { w /= 2; h /= 2; }
    unsigned sum = 0;
    for (char c : img.name) sum = sum * 31u + static_cast<unsigned>(c);
    sum ^= static_cast<unsigned>(w * 7919 + h * 104729);
    return Thumb{img.name, w, h, sum};
}

std::vector<Thumb> processBatch(const std::vector<Image>& images) {
    // 課程 2.6：hardware_concurrency() 可能回 0，一定要有 fallback
    unsigned n = std::thread::hardware_concurrency();
    if (n == 0) n = 4;
    if (n > images.size()) n = static_cast<unsigned>(images.size());
    if (n == 0) return {};

    // 每條執行緒只寫自己負責的索引區間 → 不同執行緒寫不同元素，無 data race。
    // （vector 先 resize 到最終大小，之後不再增長，因此不會有重新配置。）
    std::vector<Thumb> results(images.size());

    std::vector<std::thread> workers;      // thread 是 move-only（課程 2.7）
    workers.reserve(n);

    const size_t chunk = (images.size() + n - 1) / n;   // 無條件進位切分
    for (unsigned i = 0; i < n; ++i) {
        size_t begin = i * chunk;
        size_t end   = std::min(begin + chunk, images.size());
        if (begin >= end) break;
        workers.emplace_back([&images, &results, begin, end] {
            for (size_t k = begin; k < end; ++k) {
                results[k] = makeThumbnail(images[k]);
            }
        });
    }

    for (auto& t : workers) t.join();   // join 建立 happens-before：以下讀取安全
    return results;
}

} // namespace practical_thumbnails


// -----------------------------------------------------------------------------
// 【日常實務範例 2】背景健康檢查任務的所有權轉移（Health Checker Registry）
//   情境：服務啟動時要為每個下游依賴（DB、快取、訊息佇列）各開一條背景執行緒
//         定期做健康檢查。這些執行緒由工廠函式建立，交給一個 registry 持有，
//         registry 解構時必須確保全部乾淨收尾。
//
//   為什麼用到本主題：這是課程 2.7「移動語意」的真實用途 ——
//     * 工廠函式**回傳** std::thread（移動／RVO）
//     * registry **接收**所有權（std::move 進 vector）
//     * RAII 解構子負責 join，並且一定要先檢查 joinable()（見【面試題 陷阱 2】）
//   對照 lesson_2_7::ScopedThread：那是單條執行緒的 RAII，這裡是多條的版本。
// -----------------------------------------------------------------------------
namespace practical_health {

struct HealthReport {
    std::string dependency;
    int         checks = 0;
    bool        healthy = false;
};

// 工廠函式：建立一條背景檢查執行緒並「回傳」它（所有權轉移給呼叫端）
std::thread makeChecker(const std::string& name, int rounds, HealthReport& out) {
    return std::thread([name, rounds, &out] {
        out.dependency = name;
        for (int i = 0; i < rounds; ++i) {
            // 真實系統這裡是 ping / SELECT 1 / TCP connect
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            ++out.checks;
        }
        out.healthy = (out.checks == rounds);
    });
}

// RAII registry：持有多條 thread，解構時全部 join
class CheckerRegistry {
    std::vector<std::thread> threads_;
public:
    void adopt(std::thread t) {                 // by value + move = 明確接收所有權
        if (!t.joinable()) {
            throw std::logic_error("CheckerRegistry：收到不可 join 的執行緒");
        }
        threads_.push_back(std::move(t));
    }

    void joinAll() {
        for (auto& t : threads_) {
            if (t.joinable()) t.join();         // 一定要先檢查，否則丟 system_error
        }
        threads_.clear();
    }

    ~CheckerRegistry() {
        // 解構子不可拋例外 → joinable() 檢查是必要的，不是防禦性冗餘
        for (auto& t : threads_) {
            if (t.joinable()) t.join();
        }
    }

    // registry 本身也應該是 move-only（它持有 move-only 的資源）
    CheckerRegistry() = default;
    CheckerRegistry(const CheckerRegistry&)            = delete;
    CheckerRegistry& operator=(const CheckerRegistry&) = delete;
    CheckerRegistry(CheckerRegistry&&)                 = default;
    CheckerRegistry& operator=(CheckerRegistry&&)      = default;

    size_t size() const { return threads_.size(); }
};

} // namespace practical_health


// ===== main()：完整示範第二階段所有概念 =====

int main() {
    std::cout << "================================================================" << std::endl;
    std::cout << "  第二階段：std::thread 基礎 — 總複習" << std::endl;
    std::cout << "================================================================" << std::endl;
    std::cout << std::endl;

    // --- 課程 2.1 ---
    std::cout << "========== 課程 2.1：第一個多執行緒程式 ==========" << std::endl;
    {
        // 最簡單的執行緒
        std::thread t(lesson_2_1::sayHello);
        t.join();
    }
    lesson_2_1::demoJoin();
    lesson_2_1::demoMultiple();
    std::cout << std::endl;

    // --- 課程 2.2 ---
    std::cout << "========== 課程 2.2：執行緒函式的多種形式 ==========" << std::endl;
    lesson_2_2::demoAllForms();
    std::cout << std::endl;

    // --- 課程 2.3 ---
    std::cout << "========== 課程 2.3：傳遞參數給執行緒 ==========" << std::endl;
    lesson_2_3::demoParams();
    std::cout << std::endl;

    // --- 課程 2.4 ---
    std::cout << "========== 課程 2.4：join() 與 detach() ==========" << std::endl;
    lesson_2_4::demoJoinVsDetach();
    std::cout << std::endl;

    // --- 課程 2.5 ---
    std::cout << "========== 課程 2.5：joinable() 狀態檢查 ==========" << std::endl;
    lesson_2_5::demoJoinable();
    lesson_2_5::demoSafeThread();
    std::cout << std::endl;

    // --- 課程 2.6 ---
    std::cout << "========== 課程 2.6：執行緒識別與資訊 ==========" << std::endl;
    lesson_2_6::demoThreadInfo();
    lesson_2_6::demoYield();
    lesson_2_6::demoThreadPool();
    std::cout << std::endl;

    // --- 課程 2.7 ---
    std::cout << "========== 課程 2.7：執行緒的移動語意 ==========" << std::endl;
    lesson_2_7::demoMoveSemantics();
    lesson_2_7::demoFactory();
    lesson_2_7::demoTakeOwnership();
    lesson_2_7::demoThreadVector();
    lesson_2_7::demoScopedThread();
    std::cout << std::endl;

    // --- 生命週期規格實測（對應本檔開頭【詳細解釋 1】與【面試題】）---
    std::cout << "========== 生命週期規格實測 ==========" << std::endl;
    {
        // (1) 「任務跑完」不會讓 joinable() 變 false —— 最常見的致命誤解
        std::thread t([]{});                                  // 任務瞬間結束
        std::this_thread::sleep_for(std::chrono::milliseconds(50));  // 確保它跑完
        std::cout << "[LC] 任務已結束，但 joinable() 仍為 "
                  << std::boolalpha << t.joinable()
                  << "（若就此解構 → 標準保證 std::terminate）" << std::endl;
        t.join();
        std::cout << "[LC] join() 之後 joinable() = " << t.joinable() << std::endl;

        // (2) 對 non-joinable 再 join() → 丟 std::system_error（例外，可接住）
        try {
            t.join();                                          // 第二次
        } catch (const std::system_error& e) {
            std::cout << "[LC] 第二次 join() 丟出 system_error，code = "
                      << e.code().value()
                      << "（例外可被接住，與 terminate 是不同機制）" << std::endl;
        }

        // (3) move 之後，來源變成 non-joinable
        std::thread a([]{});
        std::thread b = std::move(a);
        std::cout << "[LC] move 後 來源.joinable()=" << a.joinable()
                  << "，目標.joinable()=" << b.joinable() << std::endl;
        b.join();
    }
    std::cout << std::endl;

    // --- LeetCode 1114 ---
    std::cout << "=== LeetCode 1114. Print in Order ===" << std::endl;
    {
        // 連跑 5 次：即使每次的執行緒排程都不同，輸出也必須固定
        bool allOk = true;
        std::string sample;
        for (int i = 0; i < 5; ++i) {
            std::string r = leetcode_1114::run();
            if (i == 0) sample = r;
            if (r != "onetwothree") allOk = false;
        }
        std::cout << "輸出 = " << sample << std::endl;
        std::cout << "連續 5 次皆為 onetwothree：" << (allOk ? "是" : "否")
                  << "（三條執行緒刻意以相反順序啟動；順序由 "
                     "condition_variable 保證，不是靠 join）" << std::endl;
    }
    std::cout << std::endl;

    // --- 日常實務範例 1 ---
    std::cout << "=== 日常實務範例 1：批次影像縮圖處理 ===" << std::endl;
    {
        using namespace practical_thumbnails;
        std::vector<Image> batch = {
            {"sunset.jpg",   4032, 3024},
            {"portrait.png", 2048, 3072},
            {"logo.svg",      512,  512},
            {"banner.webp",  1920,  600},
            {"icon.png",      256,  256},
            {"panorama.jpg", 8192, 2048},
            {"avatar.jpg",    600,  600},
        };

        unsigned hc = std::thread::hardware_concurrency();
        std::cout << "hardware_concurrency() = " << hc
                  << "（**實作定義**，本機值；標準允許回傳 0）" << std::endl;

        auto thumbs = processBatch(batch);
        std::cout << "處理張數 = " << thumbs.size() << std::endl;
        for (const auto& t : thumbs) {
            std::cout << "  " << t.name << " -> " << t.width << "x" << t.height
                      << "  checksum=" << t.checksum << std::endl;
        }
        // 彙總值與執行緒切分方式無關 → 決定性，可用來驗證平行化正確
        unsigned total = std::accumulate(
            thumbs.begin(), thumbs.end(), 0u,
            [](unsigned acc, const Thumb& t) { return acc ^ t.checksum; });
        std::cout << "全部 checksum 的 XOR = " << total
                  << "（與切成幾條執行緒無關，故為決定性結果）" << std::endl;
    }
    std::cout << std::endl;

    // --- 日常實務範例 2 ---
    std::cout << "=== 日常實務範例 2：背景健康檢查與所有權轉移 ===" << std::endl;
    {
        using namespace practical_health;
        HealthReport db, cache, mq;
        {
            CheckerRegistry registry;
            // 工廠函式回傳 std::thread（移動），registry 接收所有權
            registry.adopt(makeChecker("postgres", 3, db));
            registry.adopt(makeChecker("redis",    3, cache));
            registry.adopt(makeChecker("rabbitmq", 3, mq));
            std::cout << "registry 持有執行緒數 = " << registry.size() << std::endl;
            registry.joinAll();     // 明確收尾（解構子也會做，這裡示範顯式呼叫）
        }   // registry 解構：即使上面沒 joinAll，解構子也會 join

        for (const auto* r : {&db, &cache, &mq}) {
            std::cout << "  " << r->dependency << " 檢查次數=" << r->checks
                      << " 健康=" << std::boolalpha << r->healthy << std::endl;
        }
        std::cout << "（每條檢查緒只寫自己的 HealthReport，主緒 join 後才讀 → "
                     "join 的 happens-before 保證無需額外加鎖）" << std::endl;
    }
    std::cout << std::endl;

    std::cout << "================================================================" << std::endl;
    std::cout << "  第二階段完成！" << std::endl;
    std::cout << "  涵蓋：建立執行緒、可呼叫物件、參數傳遞、join/detach、" << std::endl;
    std::cout << "        joinable 狀態、執行緒 ID、移動語意與執行緒容器" << std::endl;
    std::cout << "================================================================" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread summary.cpp -o summary
//
// 本檔全部設施皆為 C++11，故 -std=c++17 即可（C++11 也能編）。
// ThreadSanitizer 實測：g++ -fsanitize=thread → 0 warning、exit 0。

// ── 哪些是決定性的、哪些每次執行都不同 ──────────────────────────────
// 【每次執行都不同】（多執行緒的本質，不是 bug）

// === 預期輸出 ===
// ================================================================
//   第二階段：std::thread 基礎 — 總複習
// ================================================================
//
// ========== 課程 2.1：第一個多執行緒程式 ==========
// [2.1] Hello from thread!
// [2.1] --- join() 示範 ---
// [2.1]   主執行緒等待中...
// [2.1]   子執行緒完成工作
// [2.1]   join() 返回，繼續執行
// [2.1] --- 多執行緒示範 ---
// AAABBB
// [2.1]   輸出順序不確定，這是多執行緒的本質
//
// ========== 課程 2.2：執行緒函式的多種形式 ==========
// [2.2] --- 四種可呼叫物件示範 ---
// [2.2]   [一般函式] 執行中
// [2.2]   [Lambda] 執行中
// [2.2]   [Lambda 捕獲] value = 42
// [2.2]   [成員函式] 執行中
// [2.2]   [成員函式帶參數] Worker 99 執行中
// [2.2]   [Functor] 計數：0 1 2 3
//
// ========== 課程 2.3：傳遞參數給執行緒 ==========
// [2.3] --- 參數傳遞示範 ---
// [2.3]   byValue: x = 999（副本被修改）
// [2.3]   a = 1（未被修改，如預期）
// [2.3]   byRef: x 已被修改為 999
// [2.3]   b = 999（已被修改）
// [2.3]   byString: "安全字串"
// [2.3]   processUniquePtr: *ptr = 42
// [2.3]   ptr 移動後是否為空：是
//
// ========== 課程 2.4：join() 與 detach() ==========
// [2.4] --- join vs detach 示範 ---
// [2.4]   [main] 等待 t1...
// [2.4]   [t1] 開始工作
// [2.4]   [t1] 工作完成
// [2.4]   [main] t1 已結束，繼續
// [2.4]   [main] t2 已分離（背景運行）
// [2.4]   [t2] 背景執行中
//
// ========== 課程 2.5：joinable() 狀態檢查 ==========
// [2.5] --- joinable() 狀態示範 ---
// [2.5]   預設建構 joinable: 0
// [2.5]   建立後 joinable: 1
// [2.5]   join 後 joinable: 0
// [2.5]   detach 後 joinable: 0
// [2.5]   move 後原物件 joinable: 0
// [2.5]   move 後新物件 joinable: 1
// [2.5] --- SafeThread 示範 ---
// [2.5]   SafeThread 任務 1
// [2.5]   SafeThread 任務 2
//
// ========== 課程 2.6：執行緒識別與資訊 ==========
// [2.6] --- 執行緒識別與資訊示範 ---
// [2.6]   主執行緒 ID: 124158353602496
// [2.6]   這是主執行緒
// [2.6]   從外部看 t 的 ID: 124158243501760
// [2.6]   這是子執行緒，ID: 124158243501760
// [2.6]   硬體並行執行緒數: 16
// [2.6] --- yield() 示範 ---
// [2.6]   子執行緒收到信號，開始執行
// [2.6] --- 根據核心數分配工作 ---
// [2.6]   建立 16 個工作執行緒
// [2.6]   工作執行緒 1 (ID: 124158235109056)
// [2.6]   工作執行緒 0 (ID: 124158243501760)[2.6]   工作執行緒
// 3 (ID: 124158329476800)
// [2.6]   工作執行緒 2 (ID: 124158321084096)
// [2.6]   工作執行緒 4 (ID: 124158346262208)
// [2.6]   工作執行緒 5 (ID: 124158337869504)
// [2.6]   工作執行緒 6 (ID: 124158226716352)
// [2.6]   工作執行緒 7 (ID: 124158218323648)
// [2.6]   工作執行緒 8 (ID: 124158209930944)
// [2.6]   工作執行緒 9 (ID: 124158201538240)
// [2.6]   工作執行緒 10 (ID: 124158193145536)
// [2.6]   工作執行緒 11 (ID: 124158184752832)
// [2.6]   工作執行緒 12 (ID: 124158176360128)
// [2.6]   工作執行緒 13 (ID: 124158167967424)
// [2.6]   工作執行緒 14 (ID: 124158159574720)
// [2.6]   工作執行緒 15 (ID: 124158151182016)
//
// ========== 課程 2.7：執行緒的移動語意 ==========
// [2.7] --- 移動語意示範 ---
// [2.7]   t1 joinable: 1
// [2.7]   移動後 t1 joinable: 0（已空）[2.7]   t1 的任務執行中
//
// [2.7]   移動後 t2 joinable: 1（取得所有權）
// [2.7] --- 工廠函式示範 ---
// [2.7]   工廠建立的執行緒 42 執行中
// [2.7] --- 所有權轉移到函式 ---
// [2.7]   takeOwnership：取得執行緒所有權
// [2.7]   工作執行緒
// [2.7]   t joinable 後: 0
// [2.7] --- std::vector<thread> 示範 ---
// [2.7]   向量中的執行緒 0
// [2.7]   向量中的執行緒 2
// [2.7]   向量中的執行緒 3
// [2.7]   向量中的執行緒 1
// [2.7] --- ScopedThread RAII 示範 ---
// [2.7]   ScopedThread 的任務
// [2.7]   ScopedThread 已自動 join
//
// ========== 生命週期規格實測 ==========
// [LC] 任務已結束，但 joinable() 仍為 true（若就此解構 → 標準保證 std::terminate）
// [LC] join() 之後 joinable() = false
// [LC] 第二次 join() 丟出 system_error，code = 22（例外可被接住，與 terminate 是不同機制）
// [LC] move 後 來源.joinable()=false，目標.joinable()=true
//
// === LeetCode 1114. Print in Order ===
// 輸出 = onetwothree
// 連續 5 次皆為 onetwothree：是（三條執行緒刻意以相反順序啟動；順序由 condition_variable 保證，不是靠 join）
//
// === 日常實務範例 1：批次影像縮圖處理 ===
// hardware_concurrency() = 16（**實作定義**，本機值；標準允許回傳 0）
// 處理張數 = 7
//   sunset.jpg -> 126x94  checksum=2581619801
//   portrait.png -> 128x192  checksum=3934477078
//   logo.svg -> 128x128  checksum=2014616481
//   banner.webp -> 120x37  checksum=2347718075
//   icon.png -> 128x128  checksum=3543070516
//   panorama.jpg -> 128x32  checksum=382235208
//   avatar.jpg -> 75x75  checksum=3882228628
// 全部 checksum 的 XOR = 2719510461（與切成幾條執行緒無關，故為決定性結果）
//
// === 日常實務範例 2：背景健康檢查與所有權轉移 ===
// registry 持有執行緒數 = 3
//   postgres 檢查次數=3 健康=true
//   redis 檢查次數=3 健康=true
//   rabbitmq 檢查次數=3 健康=true
// （每條檢查緒只寫自己的 HealthReport，主緒 join 後才讀 → join 的 happens-before 保證無需額外加鎖）
//
// ================================================================
//   第二階段完成！
//   涵蓋：建立執行緒、可呼叫物件、參數傳遞、join/detach、
//         joinable 狀態、執行緒 ID、移動語意與執行緒容器
// ================================================================
//
//   * 課程 2.1 demoMultiple 的 "AAABBB"：A 與 B 由兩條執行緒各印 3 個，
//     可能出現 AAABBB、ABABAB、BBBAAA… 任何交錯。
//   * 課程 2.2 / 2.6 / 2.7 各行可能**在同一行內互相插斷**（上面實跑輸出
//     就有多處，例如 "value = 42" 後面直接接上另一條執行緒的輸出）。
//     原因：std::cout 標準保證無 data race，但**不保證多次 << 之間不被插隊**。
//   * 所有 std::thread::id 數值（本機兩次實跑分別為 138095071311744 與
//     129043427739520）。id 格式與數值皆為**實作定義**，且執行緒結束後可被重用。
//   * 課程 2.6 「工作執行緒 N」的完成順序。
// 【實作定義（換機器/編譯器就不同）】
//   * hardware_concurrency() = 16（本機 g++ 15.2.0 / 16 硬體執行緒，與 nproc 一致）。
//     標準允許回傳 0；容器環境下它通常**不感知** cgroup 配額。
//     連帶「建立 16 個工作執行緒」這行也會隨機器改變。
// 【決定性（本機連跑 3 次逐字一致）】
//   * 「生命週期規格實測」全段：true / false / code=22 都是標準規定的行為。
//   * 「LeetCode 1114」輸出恆為 onetwothree —— 這正是該題的要求，
//     且三條執行緒是刻意以相反順序啟動的（連跑 5 次驗證）。
//   * 「實務範例 1」的縮圖尺寸與 checksum：與切成幾條執行緒無關。
//     （worker 數量會隨 hardware_concurrency 改變，但結果不變 —— 這正是
//       平行化正確性的驗證方式。）
//   * 「實務範例 2」的檢查次數 3 與 健康=true。
// exit code = 0（本機實測）。
