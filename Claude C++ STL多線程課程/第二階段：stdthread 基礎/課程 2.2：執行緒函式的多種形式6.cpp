/*
# 第二階段：std::thread 基礎

## 課程 2.2：執行緒函式的多種形式

---

### 引言

`std::thread` 可以接受任何**可呼叫物件（Callable）**作為執行緒的入口點。本課介紹四種常見形式。

---

### 一、四種可呼叫物件

```
┌─────────────────────────────────────────┐
│         std::thread 可接受的形式         │
├─────────────────────────────────────────┤
│                                         │
│  1. 一般函式（Function）                 │
│  2. Lambda 表達式                        │
│  3. 成員函式（Member Function）          │
│  4. 函式物件（Functor / Function Object）│
│                                         │
└─────────────────────────────────────────┘
```

---

### 二、一般函式

最基本的形式：

```cpp
#include <iostream>
#include <thread>

void greet() {
    std::cout << "Hello!" << std::endl;
}

int main() {
    std::thread t(greet);
    t.join();
    return 0;
}
```

---

### 三、Lambda 表達式

最靈活且常用的形式：

```cpp
#include <iostream>
#include <thread>

int main() {
    // 無捕獲的 Lambda
    std::thread t1([]() {
        std::cout << "Lambda 1" << std::endl;
    });
    
    // 有捕獲的 Lambda
    int value = 42;
    std::thread t2([value]() {
        std::cout << "Value: " << value << std::endl;
    });
    
    t1.join();
    t2.join();
    return 0;
}
```

輸出：
```
Lambda 1
Value: 42
```

---

### 四、成員函式

需要傳入物件指標和成員函式指標：

```cpp
#include <iostream>
#include <thread>

class Worker {
public:
    void doWork() {
        std::cout << "Worker doing work" << std::endl;
    }
    
    void doWorkWithParam(int id) {
        std::cout << "Worker " << id << " working" << std::endl;
    }
};

int main() {
    Worker worker;
    
    // 成員函式（無參數）
    std::thread t1(&Worker::doWork, &worker);
    
    // 成員函式（有參數）
    std::thread t2(&Worker::doWorkWithParam, &worker, 123);
    
    t1.join();
    t2.join();
    return 0;
}
```

語法：`std::thread(成員函式指標, 物件指標, 參數...)`

---

### 五、函式物件（Functor）

定義了 `operator()` 的類別：

```cpp
#include <iostream>
#include <thread>

class Task {
public:
    void operator()() const {
        std::cout << "Task executed" << std::endl;
    }
};

int main() {
    Task task;
    
    // 方式一：傳入物件
    std::thread t1(task);
    
    // 方式二：傳入臨時物件（注意額外括號）
    std::thread t2((Task()));
    
    // 方式三：使用大括號初始化（推薦）
    std::thread t3{Task()};
    
    t1.join();
    t2.join();
    t3.join();
    return 0;
}
```

**注意**：`std::thread t(Task());` 會被解析為函式宣告（Most Vexing Parse），需要額外括號或大括號。

---

### 六、帶參數的函式物件

```cpp
#include <iostream>
#include <thread>

class Counter {
    int count;
public:
    Counter(int c) : count(c) {}
    
    void operator()() const {
        for (int i = 0; i < count; ++i) {
            std::cout << i << " ";
        }
        std::cout << std::endl;
    }
};

int main() {
    std::thread t{Counter(5)};
    t.join();
    return 0;
}
```

輸出：
```
0 1 2 3 4
```

---

### 七、四種形式對照表

| 形式 | 語法 | 適用場景 |
|------|------|----------|
| 一般函式 | `thread(func)` | 簡單獨立的任務 |
| Lambda | `thread([](){})` | 需要捕獲變數、一次性任務 |
| 成員函式 | `thread(&Class::method, &obj)` | 物件導向設計 |
| 函式物件 | `thread{Functor()}` | 需要保存狀態的可重用任務 |

---

### 八、完整範例

```cpp
// 檔案：lesson_2_2_callable.cpp

#include <iostream>
#include <thread>

// 1. 一般函式
void normalFunction() {
    std::cout << "[Function]" << std::endl;
}

// 2. 類別（成員函式 + 函式物件）
class MyClass {
public:
    void memberFunc() {
        std::cout << "[Member Function]" << std::endl;
    }
    
    void operator()() {
        std::cout << "[Functor]" << std::endl;
    }
};

int main() {
    MyClass obj;
    
    std::thread t1(normalFunction);              // 一般函式
    std::thread t2([]() {                        // Lambda
        std::cout << "[Lambda]" << std::endl; 
    });
    std::thread t3(&MyClass::memberFunc, &obj);  // 成員函式
    std::thread t4{MyClass()};                   // 函式物件
    
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    
    return 0;
}
```

---

### 九、本課重點回顧

1. `std::thread` 接受任何可呼叫物件
2. **一般函式**：最簡單，直接傳入函式名稱
3. **Lambda**：最靈活，可捕獲外部變數
4. **成員函式**：需傳入 `&Class::method` 和物件指標
5. **函式物件**：需定義 `operator()`，注意 Most Vexing Parse

---

### 下一課預告

在 **課程 2.3：傳遞參數給執行緒** 中，我們將學習：
- 傳值 vs 傳引用
- `std::ref` 的使用
- 參數傳遞的陷阱

---

準備好繼續嗎？
*/

// =============================================================================
//  課程 2.2：執行緒函式的多種形式6.cpp  —  四種可呼叫物件總整理
// =============================================================================
//
// 【主題資訊 Information】
//   建構      : template<class F, class... Args> explicit thread(F&& f, Args&&... args);
//   標準版本  : C++11
//   標頭檔    : <thread>
//   接受的形式: ① 一般函式 ② Lambda ③ 成員函式指標 + 物件 ④ 函式物件(Functor)
//   共通機制  : 全部由 INVOKE 統一處理;所有參數都會被「複製或移動」進執行緒
//
// 【詳細解釋 Explanation】
//
// 【1. 四種形式其實是同一套機制】
// 初學時會覺得這是四種不同的語法,其實 std::thread 只做一件事:
// 把你給的東西丟給 INVOKE 這個統一的呼叫協定。INVOKE 的規則是:
//   * 如果 f 是成員函式指標,而第一個參數是物件(或指標、reference_wrapper)
//     → 呼叫 (obj.*f)(其餘參數...)
//   * 否則 → 直接呼叫 f(所有參數...)
// 這也是 std::bind、std::function、std::invoke 使用的同一套規則。
// 理解 INVOKE,四種形式就自動統一了 —— 不需要死背四條語法。
//
// 【2. 成員函式為什麼「看起來」特別麻煩】
//     std::thread t(&MyClass::memberFunc, &obj);
// 需要兩個東西:成員函式指標,以及要在哪個物件上呼叫。
// 原因很單純:成員函式一定要有 this。C++ 的成員函式指標並不綁定物件,
// 它只描述「類別裡的哪一個函式」,所以呼叫時必須另外提供物件。
//
// 這裡的第二個參數有三種寫法,語意差很多:
//     std::thread t(&MyClass::f, &obj);              // 傳指標:共享同一個物件
//     std::thread t(&MyClass::f, obj);               // 傳物件:複製一份!
//     std::thread t(&MyClass::f, std::ref(obj));     // 明確表達共享
// 中間那種最容易出錯 —— 你以為在操作 obj,其實執行緒操作的是一個副本,
// 所有修改都不會反映回來。這和課程 2.2/4 的 functor 複製是同一個坑。
//
// 【3. 傳 &obj 時的生命週期責任】
// 傳指標代表「執行緒會直接使用這個物件」,因此 obj 必須活得比執行緒久。
// 若 obj 是區域變數而執行緒被 detach,物件銷毀後執行緒仍在用它 ——
// 未定義行為。這是 detach + 成員函式最常見的災難組合(見課程 2.4)。
//
// 【4. 四種形式怎麼選】
//   一般函式  :邏輯獨立、會被多處呼叫、需要單元測試 → 最單純。
//   Lambda    :一次性、需要捕獲外部變數 → 現代 C++ 的預設選擇。
//   成員函式  :任務本來就是某個物件的行為(worker.run()) → 物件導向設計。
//   Functor   :任務需要具名型別、需要組態、會被放進容器或佇列 → 執行緒池的基礎。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 std::thread 一律複製參數
//   因為執行緒的生命週期和呼叫端脫鉤。若 thread 保留參考,呼叫端的區域變數
//   一離開作用域,執行緒就在存取死掉的記憶體。統一複製是安全的預設,
//   而要共享必須明確寫 std::ref —— 讓危險選項需要多打字,是好的 API 設計。
//
// (B) std::ref 到底是什麼
//   它產生一個 std::reference_wrapper<T>,是個「可以被複製的參考」。
//   thread 複製的是這個輕量包裝,而包裝內部存的是指標,
//   於是「複製語意」與「共享語意」得以並存。INVOKE 規則也明確支援
//   用 reference_wrapper 當成員函式的物件參數。
//
// (C) 無捕獲 lambda 可以退化成函式指標
//   沒有捕獲的 lambda 沒有狀態,因此可以隱式轉換成一般函式指標;
//   有捕獲的則不行 —— 函式指標無處存放那些狀態。
//   這說明了 lambda 本質上就是編譯器產生的 functor:
//   捕獲的變數成為它的資料成員,函式本體成為 operator()。
//
// 【注意事項 Pay Attention】
// 1. 成員函式形式的第二個參數,傳 &obj 是共享、傳 obj 是複製,別搞混。
// 2. 傳 &obj 就要保證物件活得比執行緒久,detach 時尤其危險。
// 3. 所有傳給 thread 的參數都會被複製或移動;要共享一律用 std::ref。
// 4. 無參數的臨時 functor 記得用 {},否則觸發 Most Vexing Parse。
// 5. 每個建立出來的 thread 都要 join() 或 detach()。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】執行緒可呼叫物件的四種形式
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼用成員函式建立執行緒要寫成 std::thread t(&MyClass::f, &obj);
//        比一般函式多一個參數?
//     答：因為成員函式需要 this。C++ 的成員函式指標不綁定物件,
//         它只表示「類別中的哪個函式」,所以呼叫時必須另外提供物件。
//         std::thread 用的是 INVOKE 規則:第一個參數是成員函式指標時,
//         第二個參數就被當成呼叫它的物件。
//     追問：第二個參數傳 obj 和傳 &obj 有什麼差別?
//         → 傳 &obj 是共享同一個物件;傳 obj 會「複製」一份,
//           執行緒改的是副本,你完全看不到效果。想明確共享請用 std::ref(obj)。
//
// 🔥 Q2. std::thread 的參數為什麼一律被複製?這造成什麼影響?
//     答：因為執行緒的生命週期與呼叫端脫鉤,若保留參考,呼叫端的區域變數
//         一離開作用域執行緒就在存取死記憶體。統一複製是安全的預設。
//         影響是:想讓執行緒修改外部變數,必須明確用 std::ref 包起來,
//         否則改的只是副本。
//     追問：std::ref 為什麼能繞過這個限制?
//         → 它產生 reference_wrapper,是個「可被複製的參考」;
//           thread 複製的是這個輕量包裝,包裝內部存的是指標,
//           所以複製語意和共享語意可以並存。
//
// ⚠️ 陷阱. 「我把物件傳給 thread 執行它的成員函式,執行緒裡修改了成員,
//         join 之後卻發現原物件完全沒變。是不是 join 沒等到?」
//     答：join 沒問題,問題在你寫的是 std::thread t(&C::f, obj) 而不是
//         &obj 或 std::ref(obj)。thread 把 obj 複製了一份,
//         執行緒修改的是那個副本,而副本在執行緒結束後就被銷毀了。
//     為什麼會錯：直覺上「把物件傳給函式呼叫它的方法」應該是作用在原物件上,
//         因為平常寫 obj.f() 就是如此。但 std::thread 的參數傳遞是
//         「按值複製」的,它更像 std::bind 而不像直接呼叫。
//         這也是為什麼實務上多數人寧可寫 lambda:
//         std::thread t([&obj]{ obj.f(); }); 意圖一目了然。
// ═══════════════════════════════════════════════════════════════════════════

#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

// 保護 std::cout,避免多執行緒輸出互相切開
std::mutex g_ioMutex;

void say(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_ioMutex);
    std::cout << msg << std::endl;
}

// 1. 一般函式
void normalFunction() {
    say("  [Function]");
}

// 2. 類別(成員函式 + 函式物件)
class MyClass {
public:
    void memberFunc() {
        say("  [Member Function]");
    }

    void operator()() {
        say("  [Functor]");
    }
};

// 示範「傳 obj 是複製、傳 &obj 是共享」
class Accumulator {
    int total_ = 0;
public:
    void add(int v) { total_ += v; }
    int  total() const { return total_; }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1115. Print FooBar Alternately
//   題目：一個 FooBar 物件有 foo() 與 bar() 兩個方法,分別由兩條執行緒呼叫,
//         各執行 n 次;要求輸出必須是 "foobar" 交替重複 n 遍。
//   為什麼用到本主題：這題的兩條執行緒,各自執行的是「同一個物件的不同成員函式」——
//         正是本課第 3 種形式(成員函式指標 + 物件指標)的教科書級應用:
//             std::thread ta(&FooBar::foo, &fb, printFoo);
//             std::thread tb(&FooBar::bar, &fb, printBar);
//         注意兩條執行緒必須指向「同一個」FooBar(所以傳 &fb 而不是 fb)——
//         若不小心傳成 fb,兩條執行緒各拿到一個副本,各自的 turn 旗標互不相通,
//         程式會直接死結。這正好呼應本檔【陷阱】那一題。
//   作法：用一個 fooTurn_ 旗標決定現在輪到誰,搭配條件變數等待與通知。
//   複雜度：時間 O(n),空間 O(1)。
// -----------------------------------------------------------------------------
class FooBar {
    int                     n_;
    std::mutex              m_;
    std::condition_variable cv_;
    bool                    fooTurn_ = true;   // true = 該印 foo

public:
    explicit FooBar(int n) : n_(n) {}

    void foo(std::function<void()> printFoo) {
        for (int i = 0; i < n_; ++i) {
            std::unique_lock<std::mutex> lk(m_);
            cv_.wait(lk, [this] { return fooTurn_; });   // 有 predicate,可擋偽喚醒
            printFoo();
            fooTurn_ = false;
            cv_.notify_all();
        }
    }

    void bar(std::function<void()> printBar) {
        for (int i = 0; i < n_; ++i) {
            std::unique_lock<std::mutex> lk(m_);
            cv_.wait(lk, [this] { return !fooTurn_; });
            printBar();
            fooTurn_ = true;
            cv_.notify_all();
        }
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】背景健康檢查:任務本來就是某個物件的行為
//   情境: 監控系統裡的每個 Monitor 物件負責盯一個服務,
//         它有自己的狀態(服務名、檢查次數、失敗次數)。
//         這種「任務就是這個物件在做的事」的情況,用成員函式形式最自然 ——
//         不必把狀態搬到外面,也不必為了傳狀態而寫一堆參數。
//   為什麼用本主題: 對應【詳細解釋 4】中「成員函式」的適用場景,
//                   並示範必須傳 &monitor 而非 monitor 才能看到結果。
// -----------------------------------------------------------------------------
class ServiceMonitor {
    std::string service_;
    int         checks_  = 0;
    int         failures_ = 0;

public:
    explicit ServiceMonitor(std::string s) : service_(std::move(s)) {}

    void runChecks(int times) {
        for (int i = 0; i < times; ++i) {
            ++checks_;
            // 模擬:名字長度為偶數的服務每 3 次失敗 1 次
            if (service_.size() % 2 == 0 && i % 3 == 2) ++failures_;
        }
        say("    [" + service_ + "] 完成 " + std::to_string(checks_) +
            " 次檢查,失敗 " + std::to_string(failures_) + " 次");
    }

    int failures() const { return failures_; }
};

int main() {
    std::cout << "=== 原始示範:四種形式 ===" << std::endl;
    {
        MyClass obj;

        std::thread t1(normalFunction);              // ① 一般函式
        std::thread t2([]() {                        // ② Lambda
            say("  [Lambda]");
        });
        std::thread t3(&MyClass::memberFunc, &obj);  // ③ 成員函式 + 物件指標
        std::thread t4{MyClass()};                   // ④ 函式物件

        t1.join();
        t2.join();
        t3.join();
        t4.join();
        std::cout << "  ↑ 四種寫法都被同一套 INVOKE 規則處理" << std::endl;
    }

    std::cout << "\n=== 傳 obj(複製) vs 傳 &obj(共享) ===" << std::endl;
    {
        Accumulator byCopy;
        std::thread t(&Accumulator::add, byCopy, 100);   // 傳物件 → 複製一份
        t.join();
        std::cout << "  傳 obj  → 原物件 total = " << byCopy.total()
                  << "  (改到的是副本,原物件沒變)" << std::endl;

        Accumulator byPtr;
        std::thread t2(&Accumulator::add, &byPtr, 100);  // 傳指標 → 共享
        t2.join();
        std::cout << "  傳 &obj → 原物件 total = " << byPtr.total()
                  << "  (這次真的改到了)" << std::endl;

        Accumulator byRef;
        std::thread t3(&Accumulator::add, std::ref(byRef), 100);  // 明確共享
        t3.join();
        std::cout << "  std::ref → 原物件 total = " << byRef.total()
                  << "  (意圖最清楚的寫法)" << std::endl;
    }

    std::cout << "\n=== LeetCode 1115. Print FooBar Alternately (n=4) ===" << std::endl;
    {
        FooBar fb(4);
        // 注意傳的是 &fb —— 兩條執行緒必須操作同一個物件
        std::thread ta(&FooBar::foo, &fb, [] { std::cout << "foo"; });
        std::thread tb(&FooBar::bar, &fb, [] { std::cout << "bar"; });
        ta.join();
        tb.join();
        std::cout << std::endl;
        std::cout << "  ↑ 每次執行都固定是 foobarfoobarfoobarfoobar" << std::endl;
    }

    std::cout << "\n=== 實務:背景健康檢查(成員函式形式) ===" << std::endl;
    {
        ServiceMonitor api("api-gateway");     // 11 字元(奇數)
        ServiceMonitor db("postgres");         // 8 字元(偶數)

        std::thread m1(&ServiceMonitor::runChecks, &api, 9);
        std::thread m2(&ServiceMonitor::runChecks, &db, 9);
        m1.join();
        m2.join();

        std::cout << "  join 後讀回結果:api 失敗 " << api.failures()
                  << " 次、db 失敗 " << db.failures()
                  << " 次(因為傳的是 &monitor,狀態真的被更新了)" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.2：執行緒函式的多種形式6.cpp" -o callable_forms

// 注意:以下為某一次實際執行的結果。
//   * 第一段四行 [Function]/[Lambda]/[Member Function]/[Functor] 的先後順序
//     每次執行都可能不同。實務段兩行也是。
//   * 但每一行都完整(有 mutex 保護)。
//   * LeetCode 1115 那段每次執行都固定是 foobarfoobarfoobarfoobar ——
//     那是條件變數保證出來的,不是碰運氣。
//   * 「傳 obj / 傳 &obj / std::ref」三行的結果(0 / 100 / 100)是語意保證,
//     每次執行都相同。

// === 預期輸出 ===
// === 原始示範:四種形式 ===
//   [Function]
//   [Lambda]
//   [Functor]
//   [Member Function]
//   ↑ 四種寫法都被同一套 INVOKE 規則處理
//
// === 傳 obj(複製) vs 傳 &obj(共享) ===
//   傳 obj  → 原物件 total = 0  (改到的是副本,原物件沒變)
//   傳 &obj → 原物件 total = 100  (這次真的改到了)
//   std::ref → 原物件 total = 100  (意圖最清楚的寫法)
//
// === LeetCode 1115. Print FooBar Alternately (n=4) ===
// foobarfoobarfoobarfoobar
//   ↑ 每次執行都固定是 foobarfoobarfoobarfoobar
//
// === 實務:背景健康檢查(成員函式形式) ===
//     [api-gateway] 完成 9 次檢查,失敗 0 次
//     [postgres] 完成 9 次檢查,失敗 3 次
//   join 後讀回結果:api 失敗 0 次、db 失敗 3 次(因為傳的是 &monitor,狀態真的被更新了)
