// =============================================================================
//  課程 4.5：競爭條件的檢測  —  用工具把「偶爾才錯」的 bug 變成「每次都報」
// =============================================================================
//
// 【主題資訊 Information】
//   動態偵測工具：
//     ThreadSanitizer  g++ -std=c++17 -fsanitize=thread -g prog.cpp -pthread
//     Helgrind         valgrind --tool=helgrind ./prog        （不必重編）
//   標準依據：data race 的定義在 [intro.races]（C++11 起正式納入記憶體模型）
//   標頭檔：<thread>、<mutex>、<atomic>、<condition_variable>
//   成本：TSan 執行慢約 5–15 倍、記憶體約 5–10 倍；Helgrind 約 20–100 倍
//
// 【詳細解釋 Explanation】
//
// 【1. 標準怎麼定義 data race —— 三個條件同時成立】
//   (a) 兩個以上的執行緒存取「同一個 memory location」；
//   (b) 其中至少一個是「寫入」；
//   (c) 兩者之間沒有 happens-before 關係（沒有任何同步把它們排序）。
//   三者同時成立 → 整個程式是 undefined behavior，不是「數值可能算錯」。
//   這個區別非常重要：UB 允許編譯器在最佳化時假設 race 不存在，因此
//   實際症狀可能是無限迴圈、讀到不可能的值、或整個迴圈被刪掉，
//   而不只是「counter 少加了幾次」。
//
// 【2. 為什麼 race 難以重現】
//   race 要出事，需要兩個執行緒的指令剛好交錯在某個極窄的視窗內。
//   單核心分時排程、CPU 快取狀態、其他行程的負載、甚至編譯最佳化等級，
//   都會改變交錯機率。所以典型現象是：開發機跑一萬次都對，
//   上線後在高負載機器上每天壞一次。這正是需要工具的原因 ——
//   不能靠「多跑幾次看看」來證明沒有 race。
//
// 【3. ThreadSanitizer 的原理：shadow memory + vector clock】
//   TSan 在編譯期對每個記憶體存取插入檢查碼，並替程式的每個 memory word
//   維護若干「shadow cell」，記錄最近幾次存取是「哪個執行緒、在哪個邏輯時鐘、
//   讀還是寫」。每次新存取進來時，TSan 拿它跟 shadow cell 比對：
//   若兩次存取來自不同執行緒、至少一個是寫、且兩者的 vector clock
//   無法排序（互不 happens-before）→ 立刻報告 data race。
//   關鍵在於：TSan 判斷的是「有沒有同步關係」，不是「有沒有真的算錯」。
//   所以就算某次執行結果完全正確，只要交錯路徑被走到，TSan 一樣會報。
//
// 【4. Helgrind 的原理與取捨】
//   Helgrind 走 Valgrind 的動態二進位插樁（DBI），在執行期翻譯指令，
//   所以「不需要重新編譯」——這是它最大的優點，適合手上只有 binary
//   或第三方 .so 的情況。代價是更慢、且對 C++11 atomic 的支援不如 TSan 完整，
//   容易對 lock-free 程式碼產生誤報。
//
// 【5. 靜態分析為什麼補不上這一塊】
//   Clang Static Analyzer / Coverity 等能在編譯期抓出「明顯沒上鎖」的樣式，
//   但無法得知執行期真正的執行緒交錯與別名關係，因此對 race 的偵測
//   本質上是不完備的。實務做法是：靜態分析當第一道篩子，
//   TSan 加上足夠的測試覆蓋率當主力。
//
// 【概念補充 Concept Deep Dive】
//   ▸ 為什麼 TSan「只能檢測實際執行到的程式碼」
//     TSan 是動態工具，只看得到這次執行真的走過的指令。
//     一條從未被測試覆蓋到的分支，裡面就算有 race 也不會被報出來。
//     所以 TSan 的效果上限，等於你的多執行緒測試覆蓋率。
//   ▸ 為什麼 TSan 不需要「剛好撞到」也能報
//     這是 happens-before 演算法的威力：它比較的是同步關係而非時間先後。
//     兩次存取就算實際相差 3 秒，只要中間沒有任何同步邊，一樣是 race。
//     這讓 TSan 的偵測率遠高於「跑很多次看有沒有壞」。
//   ▸ volatile 為什麼不能解決 race
//     volatile 只保證「不要把這個讀寫最佳化掉、不要重排 volatile 之間的順序」，
//     它既不提供原子性，也不提供跨執行緒的記憶體順序保證。
//     跨執行緒共享要用 std::atomic 或 mutex，這是兩件不同的事。
//   ▸ TSan 與 AddressSanitizer 不能同時開
//     兩者都要改寫記憶體佈局與插樁，-fsanitize=thread 與 -fsanitize=address
//     互斥，必須分兩次建置。
//
// 【注意事項 Pay Attention】
//   1. data race 是 undefined behavior，不可以描述成「結果會少加幾次」。
//      UB 沒有保證的結果——包含「這次看起來完全正常」。
//   2. TSan 回報「no warnings」不等於程式沒有 race，只代表這次執行沒踩到。
//   3. TSan 只用於開發與測試，絕不要編進正式環境的產品。
//   4. 本檔案下半部保留的 producer/consumer 是「刻意的錯誤示範」，
//      它確實含有 data race，這是示範的目的，不是本檔的 bug。
//      因為它是 UB，本檔不會把它的執行結果印出來當作預期輸出。
//   5. 加 sleep 只能提高踩到 race 的機率，不能證明沒有 race，
//      也不能拿來當修復手段。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】競爭條件的偵測
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是 data race？它和 race condition 是同一件事嗎？
//     答：data race 有精確的標準定義——多執行緒對同一 memory location
//         的衝突存取（至少一個寫），且兩者之間沒有 happens-before 關係，
//         結果是 undefined behavior。race condition 是較廣的工程用語，
//         指「結果取決於執行時序」，即使全程有正確上鎖也可能發生
//         （例如先 empty() 再 pop() 的 check-then-act）。
//         所以：所有 data race 都是 bug，但不是所有 race condition 都是 data race。
//     追問：舉一個「沒有 data race 卻仍然錯」的例子？
//         → 兩次呼叫各自上鎖的 empty() 與 pop()，兩把鎖之間狀態可能已被改掉。
//
// 🔥 Q2. TSan 是怎麼在「這次沒撞到」的情況下還能報出 race 的？
//     答：它不是靠時間先後，而是靠 happens-before。TSan 用 shadow memory
//         記錄每個記憶體位置最近的存取者與其 vector clock，
//         新存取進來時比對兩者的時鐘是否可排序。
//         只要中間沒有同步邊，即使兩次存取相隔很久也判定為 race。
//     追問：那 TSan 會漏報嗎？
//         → 會。它是動態工具，只看得到本次執行走到的程式碼路徑；
//           沒被測試覆蓋到的分支完全不會被檢查。
//
// 🔥 Q3. 為什麼把共享變數宣告成 volatile 不能解決多執行緒問題？
//     答：volatile 只約束編譯器不要省略或重排該變數的存取，
//         用途是記憶體對映 I/O 與 signal handler。
//         它不提供原子性（counter++ 仍是 load-modify-store 三步驟），
//         也不建立跨執行緒的 happens-before。要用 std::atomic 或 mutex。
//     追問：那 std::atomic 的預設記憶體順序是什麼？
//         → memory_order_seq_cst，最強也最好推理；放寬順序要有明確理由。
//
// ⚠️ 陷阱. 「我加了 -fsanitize=thread 跑過測試都沒警告，所以沒有 race。」
//     答：不成立。TSan 只能證明「這次執行踩到的路徑上沒發現 race」，
//         不能證明整個程式沒有 race。沒被執行到的分支、沒被觸發的交錯
//         都在偵測範圍之外。
//     為什麼會錯：把動態偵測工具當成了形式驗證。
//         TSan 是「找得到就一定是真的」（誤報極低），
//         但不是「找不到就一定沒有」（不保證完備）。
//         正確心態是：TSan 的價值上限 = 你的多執行緒測試覆蓋率。
// ═══════════════════════════════════════════════════════════════════════════

/*
# 第四階段：共享資料與競爭條件

## 課程 4.5：競爭條件的檢測

---

### 引言

競爭條件難以重現和除錯，幸運的是有專門的工具可以幫助我們檢測。本課介紹實用的檢測工具和技巧，特別是你已經熟悉的 ThreadSanitizer。

---

### 一、檢測工具總覽

```
┌─────────────────────────────────────────────────────────────┐
│                  競爭條件檢測工具                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  動態分析（執行時檢測）                                      │
│  • ThreadSanitizer (TSan) — 最常用                          │
│  • Helgrind (Valgrind)                                      │
│  • Intel Inspector                                          │
│                                                             │
│  靜態分析（編譯時檢測）                                      │
│  • Clang Static Analyzer                                    │
│  • Coverity                                                 │
│  • PVS-Studio                                               │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 二、ThreadSanitizer 基本使用

```cpp
// 檔案：race_demo.cpp
#include <thread>

int counter = 0;

void increment() {
    for (int i = 0; i < 1000; ++i) {
        counter++;
    }
}

int main() {
    std::thread t1(increment);
    std::thread t2(increment);
    
    t1.join();
    t2.join();
    
    return 0;
}
```

編譯與執行：

```bash
g++ -std=c++17 -fsanitize=thread -g -o race_demo race_demo.cpp -pthread
./race_demo
```

---

### 三、TSan 輸出解讀

```
==================
WARNING: ThreadSanitizer: data race (pid=12345)
  Write of size 4 at 0x000000601040 by thread T2:
    #0 increment() race_demo.cpp:7 (race_demo+0x...)
    #1 void std::__invoke_impl<...>

  Previous write of size 4 at 0x000000601040 by thread T1:
    #0 increment() race_demo.cpp:7 (race_demo+0x...)
    #1 void std::__invoke_impl<...>

  Location is global 'counter' of size 4 at 0x000000601040

SUMMARY: ThreadSanitizer: data race race_demo.cpp:7 in increment()
==================
```

關鍵資訊：
- **Write of size 4**：4 位元組的寫入操作
- **race_demo.cpp:7**：問題發生在第 7 行
- **global 'counter'**：問題變數是全域的 counter

---

### 四、TSan 常見報告類型

```
┌─────────────────────────────────────────────────────────────┐
│                  TSan 報告類型                               │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  data race                                                  │
│  → 兩個執行緒同時存取，至少一個寫入                          │
│                                                             │
│  thread leak                                                │
│  → 執行緒結束前未 join 或 detach                            │
│                                                             │
│  lock-order-inversion (potential deadlock)                  │
│  → 鎖的獲取順序不一致，可能死結                              │
│                                                             │
│  use of uninitialized mutex                                 │
│  → 使用未初始化的互斥鎖                                      │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 五、更複雜的案例

```cpp
// 檔案：complex_race.cpp
#include <iostream>
#include <thread>
#include <vector>

std::vector<int> data;

void producer() {
    for (int i = 0; i < 100; ++i) {
        data.push_back(i);
    }
}

void consumer() {
    for (int i = 0; i < 100; ++i) {
        if (!data.empty()) {
            int val = data.back();
            data.pop_back();
        }
    }
}

int main() {
    std::thread t1(producer);
    std::thread t2(consumer);
    
    t1.join();
    t2.join();
    
    return 0;
}
```

TSan 會報告 vector 內部的多處競爭。

---

### 六、TSan 的限制

```
┌─────────────────────────────────────────────────────────────┐
│                  TSan 的限制與注意事項                       │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  效能影響                                                    │
│  • 執行速度慢 5-15 倍                                        │
│  • 記憶體使用增加 5-10 倍                                    │
│                                                             │
│  檢測限制                                                    │
│  • 只能檢測實際執行到的程式碼                                │
│  • 無法檢測所有可能的交錯                                    │
│  • 需要足夠的測試覆蓋率                                      │
│                                                             │
│  使用建議                                                    │
│  • 開發和測試時使用，不用於生產環境                          │
│  • 結合壓力測試增加發現機率                                  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 七、Helgrind（Valgrind 工具）

另一個選擇，不需要重新編譯：

```bash
# 正常編譯
g++ -std=c++17 -g -o race_demo race_demo.cpp -pthread

# 使用 Helgrind 執行
valgrind --tool=helgrind ./race_demo
```

輸出：
```
==12345== Possible data race during write of size 4
==12345==    at 0x401234: increment() (race_demo.cpp:7)
==12345==  This conflicts with a previous write
==12345==    at 0x401234: increment() (race_demo.cpp:7)
```

---

### 八、TSan vs Helgrind 比較

| 特性 | TSan | Helgrind |
|------|------|----------|
| 速度 | 較快（5-15x） | 較慢（20-100x） |
| 需要重新編譯 | 是 | 否 |
| 誤報率 | 低 | 較高 |
| 支援 C++11 原子 | 完整 | 部分 |

---

### 九、手動檢測技巧

#### 技巧一：插入延遲

```cpp
void suspiciousFunction() {
    // 在可疑位置插入延遲，增加競爭發生機率
    checkCondition();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    doAction();
}
```

#### 技巧二：壓力測試

```cpp
#include <iostream>
#include <thread>
#include <vector>

void stressTest() {
    for (int trial = 0; trial < 1000; ++trial) {
        // 重設狀態
        counter = 0;
        
        std::vector<std::thread> threads;
        for (int i = 0; i < 10; ++i) {
            threads.emplace_back(increment);
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        if (counter != 10000) {
            std::cout << "競爭檢測！Trial " << trial 
                      << " counter=" << counter << std::endl;
        }
    }
}
```

---

### 十、檢測清單

```
┌─────────────────────────────────────────────────────────────┐
│               競爭條件檢測清單                               │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  □ 使用 TSan 編譯並執行測試                                 │
│  □ 確保測試覆蓋多執行緒路徑                                 │
│  □ 進行壓力測試（多次重複執行）                             │
│  □ 審查所有共享變數的存取                                   │
│  □ 檢查 Check-Then-Act 模式                                │
│  □ 檢查 Read-Modify-Write 操作                             │
│  □ 確認所有複合操作都有適當保護                             │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 十一、本課重點回顧

1. **ThreadSanitizer** 是最實用的競爭條件檢測工具
2. 使用 `-fsanitize=thread -g` 編譯
3. TSan 會報告資料競爭的位置和相關執行緒
4. TSan 會降低效能，僅用於開發和測試
5. **Helgrind** 不需重新編譯，但較慢
6. 結合**壓力測試**增加發現問題的機率

---

### 第四階段完成！

恭喜你完成了共享資料與競爭條件階段！你已經學會：

- ✅ 共享資料的問題本質
- ✅ 不變量與競爭條件的關係
- ✅ 臨界區段的識別與設計
- ✅ 常見競爭條件模式
- ✅ 使用工具檢測競爭條件

---

### 下一階段預告

**第五階段：互斥鎖基礎 (std::mutex)** 將學習如何解決這些問題：
- 課程 5.1：std::mutex 基本操作
- 課程 5.2：互斥鎖的工作原理
- 課程 5.3：try_lock() 非阻塞鎖定
- ...

---

準備好進入第五階段嗎？
*/



// 檔案：complex_race.cpp
#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

// -----------------------------------------------------------------------------
// 【刻意的錯誤示範】未同步的 producer / consumer
// -----------------------------------------------------------------------------
// 這段程式含有真正的 data race：兩個執行緒同時對同一個 std::vector
// 做 push_back / empty / back / pop_back，彼此之間沒有任何同步。
// 它是 undefined behavior，用 -fsanitize=thread 編譯就會看到 TSan
// 報出 vector 內部（size、緩衝區指標）的多處競爭。
//
// 因為是 UB，本檔「不會把它的執行結果印出來」——UB 沒有可以拿來當
// 預期輸出的固定答案。它只負責被跑一次，讓你拿去餵給 TSan / Helgrind。
// -----------------------------------------------------------------------------
std::vector<int> data;

void producer() {
    for (int i = 0; i < 100; ++i) {
        data.push_back(i);
    }
}

void consumer() {
    for (int i = 0; i < 100; ++i) {
        if (!data.empty()) {           // check ...
            int val = data.back();     // ... then act：兩步之間狀態可能已被改掉
            data.pop_back();
            (void)val;                 // 刻意不使用，只為呈現讀取也參與競爭
        }
    }
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1114. Print in Order
//   題目：三個執行緒分別呼叫 first()/second()/third()，可能以任意順序被呼叫，
//         要保證輸出一定是 "firstsecondthird"。
//   為什麼用到本主題：這題的「錯誤解法」正是本課要偵測的東西——
//         若不做同步，輸出順序取決於排程，就是典型的 race condition；
//         若改用 sleep 硬等，TSan 依然會報 race（sleep 不建立 happens-before）。
//         正解必須用 mutex + condition_variable 建立真正的同步邊，
//         讓 TSan 能證明存取之間存在 happens-before 而不再報告。
// -----------------------------------------------------------------------------
class Foo {
    std::mutex m_;
    std::condition_variable cv_;
    int stage_ = 1;                     // 下一個該執行的階段：1 → 2 → 3
public:
    void first(const std::function<void()>& printFirst) {
        std::unique_lock<std::mutex> lk(m_);
        printFirst();
        stage_ = 2;
        cv_.notify_all();
    }
    void second(const std::function<void()>& printSecond) {
        std::unique_lock<std::mutex> lk(m_);
        cv_.wait(lk, [this] { return stage_ == 2; });
        printSecond();
        stage_ = 3;
        cv_.notify_all();
    }
    void third(const std::function<void()>& printThird) {
        std::unique_lock<std::mutex> lk(m_);
        cv_.wait(lk, [this] { return stage_ == 3; });
        printThird();
    }
};

void demoPrintInOrder() {
    Foo foo;
    // 故意以「相反」的順序啟動執行緒，證明結果與啟動順序無關
    std::thread t3([&] { foo.third ([] { std::cout << "third";  }); });
    std::thread t2([&] { foo.second([] { std::cout << "second"; }); });
    std::thread t1([&] { foo.first ([] { std::cout << "first";  }); });
    t1.join();
    t2.join();
    t3.join();
    std::cout << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】網站流量統計：兩種正確的共享計數器寫法
//   情境：N 個 worker 執行緒各自處理請求，要累加到同一個全域計數器。
//   最常見的錯誤寫法是直接 ++counter（沒有任何保護）——那是 data race，
//   而且因為是 UB，「少加幾次」只是最溫和的可能症狀，不是保證的行為。
//   所以這裡只示範兩種「正確」的做法，兩者都會得到確定的結果。
//     ▸ mutex ：要保護一整段複合操作時用（例如同時更新多個欄位）
//     ▸ atomic：單一變數的 read-modify-write，成本低得多
// -----------------------------------------------------------------------------
long long counterWithMutex(int threads, int perThread) {
    long long counter = 0;
    std::mutex m;
    std::vector<std::thread> pool;
    for (int i = 0; i < threads; ++i) {
        pool.emplace_back([&] {
            for (int k = 0; k < perThread; ++k) {
                std::lock_guard<std::mutex> lk(m);
                ++counter;
            }
        });
    }
    for (auto& t : pool) t.join();
    return counter;
}

long long counterWithAtomic(int threads, int perThread) {
    std::atomic<long long> counter{0};
    std::vector<std::thread> pool;
    for (int i = 0; i < threads; ++i) {
        pool.emplace_back([&] {
            for (int k = 0; k < perThread; ++k) {
                // fetch_add 是不可分割的 read-modify-write，
                // 且預設的 memory_order_seq_cst 會建立跨執行緒的同步關係
                counter.fetch_add(1);
            }
        });
    }
    for (auto& t : pool) t.join();
    return counter.load();
}

int main() {
    std::cout << "=== 一、LeetCode 1114. Print in Order ===" << std::endl;
    // 執行 5 次：若同步寫對，5 次都必須是 firstsecondthird
    for (int i = 0; i < 5; ++i) demoPrintInOrder();

    std::cout << "\n=== 二、日常實務：正確的共享計數器 ===" << std::endl;
    const int kThreads = 8, kPer = 25000;
    std::cout << "預期總數        : " << static_cast<long long>(kThreads) * kPer << std::endl;
    std::cout << "mutex  版本結果 : " << counterWithMutex(kThreads, kPer) << std::endl;
    std::cout << "atomic 版本結果 : " << counterWithAtomic(kThreads, kPer) << std::endl;

    std::cout << "\n=== 三、刻意的錯誤示範：未同步的 producer/consumer ===" << std::endl;
    std::cout << "（含真實 data race，屬 undefined behavior；不印出其結果）" << std::endl;
    std::thread t1(producer);
    std::thread t2(consumer);
    t1.join();
    t2.join();
    std::cout << "已執行完畢。請用下列指令觀察 TSan 的報告：" << std::endl;
    std::cout << "  g++ -std=c++17 -fsanitize=thread -g <本檔>.cpp -o race_tsan -pthread" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 4.5：競爭條件的檢測2.cpp" -o race_detect
// 偵測: g++ -std=c++17 -fsanitize=thread -g -pthread "課程 4.5：競爭條件的檢測2.cpp" -o race_tsan
//
// 【關於下方預期輸出的但書】
//   ▸ 第一、二段是確定性的：只要同步寫對，每次執行都必須完全相同。
//   ▸ 第三段的 producer/consumer 含 data race，屬 undefined behavior，
//     沒有保證的行為；本檔刻意不輸出它的任何內部狀態，
//     因此下方預期輸出中不含任何來自該段的數值。
//   ▸ 若改用 -fsanitize=thread 建置，另會在 stderr 出現 TSan 的
//     「WARNING: ThreadSanitizer: data race」報告，其內容（位址、thread 編號、
//     堆疊）每次執行都不同，故不列入預期輸出。

// === 預期輸出 ===
// === 一、LeetCode 1114. Print in Order ===
// firstsecondthird
// firstsecondthird
// firstsecondthird
// firstsecondthird
// firstsecondthird
//
// === 二、日常實務：正確的共享計數器 ===
// 預期總數        : 200000
// mutex  版本結果 : 200000
// atomic 版本結果 : 200000
//
// === 三、刻意的錯誤示範：未同步的 producer/consumer ===
// （含真實 data race，屬 undefined behavior；不印出其結果）
// 已執行完畢。請用下列指令觀察 TSan 的報告：
//   g++ -std=c++17 -fsanitize=thread -g <本檔>.cpp -o race_tsan -pthread
