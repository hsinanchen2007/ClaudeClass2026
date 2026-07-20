// =============================================================================
//  課程 2.6：執行緒識別與資訊6.cpp  —  yield() 與旗標式等待（busy-wait）
// =============================================================================
//
// 【主題資訊 Information】
//   void std::this_thread::yield() noexcept;                        // C++11
//   template<class T> struct std::atomic;                           // C++11
//   標頭檔：<thread>、<atomic>
//   語意：yield() 提供排程器一個【重新排程的機會】——是提示，不是保證。
//   複雜度：yield() 在 Linux 上對應 sched_yield()，是一次輕量系統呼叫。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼旗標一定要是 std::atomic<bool>，不能是普通 bool】
//   本檔的等待迴圈長這樣：
//       while (!ready) { std::this_thread::yield(); }
//   若 ready 是普通的 bool，這段程式碼有兩個獨立的致命問題：
//     (a) 【Data race = UB】一條執行緒讀、另一條寫同一個非 atomic 變數，
//         且沒有任何同步機制 → 依標準就是未定義行為。
//         不是「可能讀到舊值」這種程度，而是整個程式的行為都不再有定義。
//     (b) 【編譯器最佳化會殺掉迴圈】編譯器看到迴圈內沒有任何東西會修改
//         ready，完全有權把它提到迴圈外只讀一次，變成
//         if (!ready) { while (true) yield(); } —— 永遠出不來。
//   改成 std::atomic<bool> 同時解決兩者：它使這個存取不再是 data race，
//   而且預設的 memory_order_seq_cst 禁止編譯器把載入提到迴圈外。
//   ⚠️ 常見誤解：有人用 volatile bool 想解決 (b)。volatile 確實會阻止
//   那個最佳化，但它【完全不提供】執行緒間的同步保證，(a) 的 data race
//   仍然存在，依舊是 UB。在 C++ 裡，volatile 不是執行緒同步工具。
//
// 【2. ready = true 為什麼不只是「寫一個 bool」】
//   對 std::atomic<bool> 的預設寫入是 memory_order_seq_cst，
//   它同時是一個 release 操作：寫入 ready 之前的所有記憶體寫入，
//   對「觀察到 ready == true」的那條執行緒都可見。
//   這就是所謂的 release-acquire 配對。真實程式裡的模式是：
//       data = 準備好的結果;        // 普通寫入
//       ready.store(true);          // release：把上面的寫入「發佈」出去
//   等待端讀到 true 之後就能安全地讀 data。
//   若 ready 只是普通 bool，這個「發佈」保證完全不存在。
//
// 【3. yield() 在這個迴圈裡到底做了什麼（以及沒做什麼）】
//   做了：告訴排程器「我可以先讓別人跑」，降低在單核心 / 高負載機器上
//         把 CPU 完全霸佔、讓 setter 執行緒遲遲跑不到的風險。
//   沒做：它【不會】讓這條執行緒進入阻塞狀態。這條執行緒始終是 runnable，
//         排程器隨時可以立刻把 CPU 排回來。所以這個迴圈仍然會把
//         一顆核心的使用率拉到接近 100%。
//   結論：yield 迴圈只適合【預期馬上就會等到】的極短等待。
//         本檔刻意讓等待長達 100ms 來凸顯這一點——這其實正是
//         「不該用 busy-wait」的長度，真實程式該換 condition_variable。
//
// 【4. 三種等待方式的取捨】
//   ┌────────────────────────┬──────────────┬──────────────┬────────────────┐
//   │ 寫法                   │ CPU 使用     │ 反應延遲     │ 適用等待長度   │
//   ├────────────────────────┼──────────────┼──────────────┼────────────────┤
//   │ while(!ready){}        │ 吃滿一顆核心 │ 最低（奈秒） │ 極短（自旋鎖） │
//   │ while(!ready) yield(); │ 仍接近滿載   │ 很低         │ 短             │
//   │ condition_variable     │ 幾乎為 0     │ 微秒級       │ 中長（推薦）   │
//   └────────────────────────┴──────────────┴──────────────┴────────────────┘
//
// 【概念補充 Concept Deep Dive】
//   * std::atomic<bool> 幾乎必然是 lock-free 的（可用 is_lock_free() 查詢），
//     在 x86-64 上載入/儲存就是一般的 mov 指令加上編譯器層級的排序限制，
//     不是「每次都上鎖」——成本遠低於多數人的想像。
//   * seq_cst 的 store 在 x86-64 上會編成 XCHG（或 MOV + MFENCE），
//     這是本檔唯一有實質成本的地方；用 memory_order_release 可省下該屏障，
//     語意上對本例仍然正確。
//   * x86 有專門給自旋迴圈用的 PAUSE 指令（提示 CPU 這是 spin-wait，
//     降低功耗、避免記憶體順序違規的管線清空懲罰）。
//     C++ 標準沒有對應 API，實務上會用 _mm_pause()（intrin）或內嵌組語。
//   * C++20 起 std::atomic 有 wait()/notify_one()/notify_all()，
//     那才是「旗標式等待」的現代正解：語意跟本檔一樣，但等待端真的會阻塞。
//
// 【注意事項 Pay Attention】
//   1. 跨執行緒共享的旗標必須是 std::atomic（或有其他同步保護）。
//      普通 bool 是 data race → UB；volatile bool 【也不行】。
//   2. yield() 是提示，實作可以完全忽略；不可用它保證任何順序。
//   3. yield 迴圈不省 CPU。等待可能較久時請用 condition_variable。
//   4. 這種等待沒有逾時機制，旗標永遠不被設定就會永遠轉下去。
//      真實系統應加上截止時間。
//   5. 主執行緒設定旗標後仍必須 join()，否則 std::thread 解構會 terminate()。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】yield() 與旗標式等待
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 把 std::atomic<bool> ready 換成普通的 bool ready，會發生什麼事？
//     答：兩個獨立的問題同時出現。第一，一寫一讀且無同步 → data race，
//         依標準是未定義行為。第二，編譯器可以合法地把 !ready 的載入
//         提到迴圈外（迴圈內沒有東西會改它），迴圈因此可能永遠不結束。
//     追問：那加上 volatile 可以嗎？→ 不行。volatile 只阻止編譯器最佳化掉
//           重複載入，【完全不提供】執行緒間的同步或記憶體順序保證，
//           data race 依然存在。在 C++ 裡 volatile 不是並行工具。
//
// 🔥 Q2. while (!ready) yield(); 這種寫法什麼時候合理？什麼時候該換掉？
//     答：只有在「預期等待極短」時合理——短到連一次 context switch
//         的成本都比等待本身貴。一旦等待可能長達毫秒等級，
//         就該換成 condition_variable（或 C++20 的 atomic::wait），
//         讓執行緒真正阻塞、把 CPU 讓出來。
//     追問：yield 迴圈到底省了多少 CPU？→ 基本上沒省。
//           執行緒仍是 runnable，排程器可以立刻把它排回來，
//           那顆核心的使用率照樣接近 100%。
//
// ⚠️ 陷阱. ready.store(true) 之後，等待端【立刻】就會看到 true 嗎？
//     答：不保證「立刻」，只保證「最終」。標準只要求對某個原子物件的寫入
//         在有限時間內對其他執行緒可見，沒有規定上限。
//         實務上在 x86-64 這個延遲是奈秒級，但那是硬體特性，不是標準保證。
//     為什麼會錯：多數人腦中的模型是「有一塊所有 CPU 共享的記憶體，
//         寫進去大家馬上看到」。真實的機器有 store buffer 與各級快取，
//         seq_cst 保證的是【順序一致】與【最終可見】，不是【零延遲可見】。
// ═══════════════════════════════════════════════════════════════════════════

/*
# 第二階段：std::thread 基礎

## 課程 2.6：執行緒識別與資訊

---

### 引言

有時我們需要識別不同的執行緒，或查詢系統的硬體資訊來決定建立多少執行緒。本課介紹這些實用的工具。

---

### 一、取得執行緒 ID

每個執行緒都有唯一的識別碼：

```cpp
#include <iostream>
#include <thread>

int main() {
    // 取得主執行緒 ID
    std::cout << "主執行緒 ID: " << std::this_thread::get_id() << std::endl;

    std::thread t([]() {
        // 在執行緒內部取得自己的 ID
        std::cout << "子執行緒 ID: " << std::this_thread::get_id() << std::endl;
    });

    // 從外部取得執行緒 ID
    std::cout << "t 的 ID: " << t.get_id() << std::endl;

    t.join();
    return 0;
}
```

輸出（實際數值每次執行都不同，且格式是實作定義的）：
```
主執行緒 ID: 140234567891520
子執行緒 ID: 140234567891521
t 的 ID: 140234567891521
```

---

### 二、兩種取得 ID 的方式

```
┌─────────────────────────────────────────────────────────────┐
│                取得執行緒 ID 的兩種方式                      │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  std::this_thread::get_id()                                 │
│  → 在執行緒內部呼叫                                          │
│  → 取得「當前執行緒」的 ID                                   │
│                                                             │
│  thread_object.get_id()                                     │
│  → 在執行緒外部呼叫                                          │
│  → 取得「指定執行緒物件」的 ID                               │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 三、std::thread::id 類型

執行緒 ID 是特殊類型，可以比較和輸出：

```cpp
#include <iostream>
#include <thread>

std::thread::id mainThreadId;

void checkThread() {
    if (std::this_thread::get_id() == mainThreadId) {
        std::cout << "這是主執行緒" << std::endl;
    } else {
        std::cout << "這是子執行緒" << std::endl;
    }
}

int main() {
    mainThreadId = std::this_thread::get_id();

    checkThread();  // 在主執行緒呼叫

    std::thread t(checkThread);  // 在子執行緒呼叫
    t.join();

    return 0;
}
```

輸出：
```
這是主執行緒
這是子執行緒
```

---

### 四、hardware_concurrency()

查詢系統支援的並行執行緒數（通常等於 CPU 核心數）：

```cpp
#include <iostream>
#include <thread>

int main() {
    unsigned int cores = std::thread::hardware_concurrency();

    std::cout << "硬體支援的並行執行緒數: " << cores << std::endl;

    // 可能回傳 0 表示無法偵測
    if (cores == 0) {
        std::cout << "無法偵測，使用預設值" << std::endl;
        cores = 2;
    }

    return 0;
}
```

---

### 五、實際應用：根據核心數分配工作

```cpp
#include <iostream>
#include <thread>
#include <vector>

int main() {
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 2;

    std::cout << "建立 " << numThreads << " 個執行緒" << std::endl;

    std::vector<std::thread> threads;

    for (unsigned int i = 0; i < numThreads; ++i) {
        threads.emplace_back([i]() {
            std::cout << "執行緒 " << i
                      << " (ID: " << std::this_thread::get_id() << ")"
                      << std::endl;
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    return 0;
}
```

---

### 六、std::this_thread 命名空間總覽

```cpp
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    // 1. get_id() - 取得當前執行緒 ID
    std::cout << "ID: " << std::this_thread::get_id() << std::endl;

    // 2. sleep_for() - 休眠指定時間
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 3. sleep_until() - 休眠到指定時間點
    auto wakeTime = std::chrono::steady_clock::now()
                  + std::chrono::milliseconds(100);
    std::this_thread::sleep_until(wakeTime);

    // 4. yield() - 讓出 CPU 時間給其他執行緒
    std::this_thread::yield();

    return 0;
}
```

---

### 七、yield() 的用途

`yield()` 提示作業系統讓其他執行緒先執行：

```cpp
#include <iostream>
#include <thread>
#include <atomic>

std::atomic<bool> ready{false};

void waitForReady() {
    while (!ready) {
        std::this_thread::yield();  // 避免忙等待浪費 CPU
    }
    std::cout << "Ready!" << std::endl;
}

int main() {
    std::thread t(waitForReady);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ready = true;

    t.join();
    return 0;
}
```

⚠️ 上面那句註解「避免忙等待浪費 CPU」其實有語病：
    yield() 讓出的是「這一輪的時間片」，執行緒仍然是 runnable，
    排程器隨時可以立刻把 CPU 排回來，這個迴圈照樣會吃滿一顆核心。
    它降低的是「完全霸佔 CPU 讓別人跑不到」的風險，不是 CPU 使用率。
    等待可能較久時，正解是 condition_variable（或 C++20 的 atomic::wait）。

---

### 八、常用功能對照表

| 功能 | 語法 | 說明 |
|------|------|------|
| 取得當前執行緒 ID | `std::this_thread::get_id()` | 回傳 `std::thread::id` |
| 取得執行緒物件的 ID | `t.get_id()` | 回傳 `std::thread::id` |
| 查詢 CPU 核心數 | `std::thread::hardware_concurrency()` | 回傳 `unsigned int`，可能為 0 |
| 休眠一段時間 | `std::this_thread::sleep_for(duration)` | 阻塞當前執行緒（至少該時間） |
| 休眠到指定時間 | `std::this_thread::sleep_until(time_point)` | 阻塞當前執行緒 |
| 讓出 CPU | `std::this_thread::yield()` | 提示排程器（可被忽略） |

---

### 九、本課重點回顧

1. `std::this_thread::get_id()` 取得當前執行緒的 ID
2. `t.get_id()` 取得執行緒物件 t 的 ID
3. `std::thread::id` 類型可以比較和輸出（格式為實作定義）
4. `hardware_concurrency()` 回傳可並行執行緒數的提示（可能為 0）
5. `sleep_for()` 和 `sleep_until()` 用於休眠（只保證「至少」）
6. `yield()` 讓出 CPU 時間，但它是提示，且不會真正降低 CPU 使用率

---

### 下一課預告

在 **課程 2.7：執行緒的移動語意** 中，我們將學習：
- 為什麼 `std::thread` 不能複製
- 如何使用 `std::move` 轉移執行緒所有權
- 執行緒容器的管理

---

準備好繼續嗎？
*/



#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

std::atomic<bool> ready{false};

void waitForReady() {
    while (!ready) {
        // yield()：提示排程器可以先跑別人。
        // 注意：這【不會】讓本執行緒阻塞，CPU 使用率仍接近滿載。
        std::this_thread::yield();
    }
    std::cout << "Ready!" << std::endl;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1115. Print FooBar Alternately
//   題目：兩條執行緒各自呼叫 foo() 與 bar() n 次，
//         必須交替輸出 "foobarfoobar..."。
//   為什麼用到本主題：這正是本檔「atomic 旗標 + yield 自旋等待」模式的
//         雙向版本——用一個 atomic<bool> 決定現在輪到誰，
//         沒輪到的那條就 yield() 讓出 CPU 再檢查。
//         示範了旗標必須是 atomic（普通 bool 會 data race + 被最佳化掉），
//         以及 yield 自旋只適合「馬上就會輪到」的短等待。
// -----------------------------------------------------------------------------
class FooBar {
private:
    int n;
    std::atomic<bool> fooTurn{true};   // true → 該印 foo，false → 該印 bar

public:
    explicit FooBar(int count) : n(count) {}

    void foo(const std::function<void()>& printFoo) {
        for (int i = 0; i < n; ++i) {
            while (!fooTurn.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            printFoo();
            fooTurn.store(false, std::memory_order_release);
        }
    }

    void bar(const std::function<void()>& printBar) {
        for (int i = 0; i < n; ++i) {
            while (fooTurn.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            printBar();
            fooTurn.store(true, std::memory_order_release);
        }
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】背景設定檔載入完成前，工作執行緒先等一下（一次性初始化旗標）
//   情境：服務啟動時，主執行緒去讀遠端設定；工作執行緒已經被建立，
//         但在設定就緒前不能開始處理請求。
//   這是「一次性事件通知」的典型場景。這裡用 atomic 旗標示範，
//   並且【明確標示】真實服務應該用 condition_variable 或 std::call_once，
//   因為讀設定可能要花上百毫秒，用自旋等待會白白燒掉一顆核心。
// -----------------------------------------------------------------------------
struct ServiceConfig {
    std::string endpoint;
    int         timeoutMs = 0;
};

ServiceConfig       g_config;              // 由 loader 寫入，由 worker 讀取
std::atomic<bool>   g_configReady{false};  // release/acquire 的配對旗標

void configLoader() {
    // 模擬讀取遠端設定的耗時
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    // ── 普通寫入：只有在 store(true) 之前完成才會被安全發佈 ──
    g_config.endpoint  = "https://api.internal/v2";
    g_config.timeoutMs = 3000;

    // release：把上面所有寫入「發佈」給看到 true 的那條執行緒
    g_configReady.store(true, std::memory_order_release);
}

std::string workerAfterConfig() {
    // acquire：一旦讀到 true，就保證看得到 loader 在 store 之前的所有寫入
    while (!g_configReady.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    return g_config.endpoint + " (timeout=" + std::to_string(g_config.timeoutMs) + "ms)";
}

int main() {
    std::cout << "=== yield() 旗標等待（本課原始示範）===" << std::endl;
    {
        std::thread t(waitForReady);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        ready = true;   // 對 atomic 的預設寫入是 seq_cst，同時具備 release 語意

        t.join();
    }

    std::cout << "\n=== LeetCode 1115. Print FooBar Alternately (n = 3) ===" << std::endl;
    {
        FooBar fb(3);
        std::thread tb([&fb]() { fb.bar([]() { std::cout << "bar"; }); });
        std::thread tf([&fb]() { fb.foo([]() { std::cout << "foo"; }); });
        tf.join();
        tb.join();
        std::cout << std::endl;
    }

    std::cout << "\n=== 日常實務: 等待設定載入完成 ===" << std::endl;
    {
        std::thread loader(configLoader);

        // 兩條 worker 同時等同一個旗標
        std::string a, b;
        std::thread w1([&a]() { a = workerAfterConfig(); });
        std::thread w2([&b]() { b = workerAfterConfig(); });

        loader.join();
        w1.join();
        w2.join();

        std::cout << "worker-1 取得設定: " << a << std::endl;
        std::cout << "worker-2 取得設定: " << b << std::endl;
        std::cout << "（真實服務請改用 condition_variable：等待 30ms 用自旋"
                     "會白燒 CPU）" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 2.6：執行緒識別與資訊6.cpp' -o yield_flag

// -----------------------------------------------------------------------------
// 【輸出但書】
//   本檔輸出是確定的：三個區段都由 atomic 旗標強制了先後順序，
//   FooBar 也保證交替。唯一不確定的是「每一步花多久」，那不影響輸出文字。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// === yield() 旗標等待（本課原始示範）===
// Ready!
//
// === LeetCode 1115. Print FooBar Alternately (n = 3) ===
// foobarfoobarfoobar
//
// === 日常實務: 等待設定載入完成 ===
// worker-1 取得設定: https://api.internal/v2 (timeout=3000ms)
// worker-2 取得設定: https://api.internal/v2 (timeout=3000ms)
// （真實服務請改用 condition_variable：等待 30ms 用自旋會白燒 CPU）
