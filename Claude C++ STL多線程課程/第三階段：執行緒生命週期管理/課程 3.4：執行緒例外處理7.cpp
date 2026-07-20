// =============================================================================
//  課程 3.4：執行緒例外處理 7  —  課文總整理 + std::async 的例外聚合
// =============================================================================
//
// 【主題資訊 Information】
//   template<class F, class... Args>
//   std::future<R> std::async(std::launch policy, F&& f, Args&&... args);
//
//   啟動政策(launch policy):
//       std::launch::async     —— 保證另開執行緒【立即】執行
//       std::launch::deferred  —— 延後到 get()/wait() 時,在【呼叫端】同步執行
//       預設(async | deferred)—— 由實作選擇,【不保證】會平行執行
//
//   標準版本：C++11
//   標頭檔  ：<future>
//   複雜度  ：get() 阻塞至結果就緒;例外會在 get() 處重新拋出
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 std::async 比裸的 std::thread 更適合「有回傳值」的工作】
//   用 std::thread 取得結果要自己搭一整套:建 promise、move 進去、
//   在 worker 裡 try/catch、set_value 或 set_exception、外面再 get。
//   std::async 把這一整套包好了 —— 你只要寫一個【會回傳值、可能丟例外】
//   的普通函式,它自動:
//     * 正常回傳 → 值被放進共享狀態
//     * 拋出例外 → 例外被裝箱進共享狀態
//   呼叫端一律用 fut.get() 取,成功拿到值、失敗就地重新拋出。
//   換句話說,async/future 讓「跨執行緒的函式呼叫」看起來就像普通呼叫,
//   連錯誤處理的寫法都一樣。
//
// 【2. 例外聚合:本檔的核心模式】
//   五個任務中有一個會失敗。關鍵在於【失敗不會拖垮其他任務】:
//     * 每個任務有自己的 future,彼此獨立。
//     * 失敗的那個把例外存進自己的共享狀態,其他四個照常完成。
//     * 主執行緒逐一 get(),用 try/catch 個別處理。
//   這正是「批次作業」最常見的需求:我要知道【哪些成功、哪些失敗、
//   為什麼失敗】,而不是整批一起死。
//
//   ⚠️ 注意這裡的重要細節:例外是在【get() 的時候】才拋出,
//   不是在任務失敗的當下。所以 try/catch 必須包住 get(),
//   包住 std::async(...) 那一行是沒有用的。
//
// 【3. ★ std::async 回傳的 future,解構函式會【阻塞】—— 唯一的特例】
//   一般的 std::future(例如從 std::promise::get_future() 取得的)
//   解構時不會等待任何東西。但【由 std::async 建立】的 future 是特例:
//   標準規定它的解構函式會【阻塞等待任務完成】([futures.async]/5)。
//
//   這個特例造成一個非常著名的陷阱:
//       std::async(std::launch::async, longTask);   // ← 沒有接住回傳值!
//       std::async(std::launch::async, longTask);   // ← 第二個
//   看起來像「開兩條執行緒平行跑」,實際上:
//     第一行產生的 future 是【臨時物件】,在該敘述結束時就解構,
//     解構函式阻塞等 longTask 跑完 —— 於是第二行根本還沒開始。
//   結果是【完全串行執行】,而且完全沒有編譯錯誤或警告。
//   本檔的示範 3 用實測時間證明這件事。
//
//   正解:一定要把 future 存起來(通常放進 vector),
//   讓它們活到你真正要收集結果的時候。
//
// 【4. 啟動政策:預設值為什麼危險】
//   不指定政策時,預設是 std::launch::async | std::launch::deferred,
//   意思是「實作可以自己選」。若實作選了 deferred:
//     * 任務【不會】在背景執行,而是延後到 get() 時在呼叫端同步跑;
//     * 若你從未呼叫 get(),任務【永遠不會執行】;
//     * 依賴平行化的效能假設完全落空。
//   libstdc++ 目前的預設行為傾向 async,但這【不是標準保證】。
//   實務準則:需要真的平行,就【明確寫】std::launch::async。
//   本檔的課文範例正是這樣寫的,值得學起來。
//
// 【5. wait_for 是判斷 deferred 的唯一可靠方式】
//   若想知道一個 future 是不是 deferred:
//       if (fut.wait_for(std::chrono::seconds(0)) == std::future_status::deferred)
//   對 deferred 的 future,wait_for 會【立刻】回傳 deferred 而不等待。
//   這也是為什麼「用 wait_for(0) 輪詢是否完成」在遇到 deferred 時
//   會變成無窮迴圈 —— 它永遠不會變成 ready,因為根本沒有人在跑它。
//
// 【概念補充 Concept Deep Dive】
//
// (A) async 的執行緒從哪裡來?
//   標準【沒有】規定 std::async 必須使用執行緒池。libstdc++ 目前的實作是
//   每次呼叫都建立一條新執行緒(std::launch::async 時)。
//   所以在迴圈裡呼叫上萬次 std::async 會建立上萬條執行緒 ——
//   這是實務上 async 最主要的效能陷阱。高頻場景請自建 thread pool。
//
// (B) future vs shared_future
//   std::future 是【獨佔】的:get() 會把結果移走,只能呼叫一次,
//   而且不可複製。若需要多個執行緒都拿到同一份結果,要用
//   std::shared_future(可複製,get() 回傳 const 參考,可重複呼叫)。
//   從 future 轉過去用 fut.share()。
//
// (C) 為什麼 async 的 future 解構要阻塞,promise 的不用?
//   因為 async 【擁有】那條執行緒。若解構不等待,任務會變成沒人管的
//   背景工作,而它可能還在存取呼叫端的區域變數(async 的引數是複製的,
//   但捕捉參考的 lambda 就不是)—— 那會是懸空參考。
//   promise/future 則相反:執行緒的生命週期由使用者自己管(自己 join),
//   future 只是取結果的把手,沒有理由阻塞。
//   這個不對稱常被批評為 std::async 設計上的瑕疵,C++ 委員會也
//   討論過多次,但為了相容性至今未改。
//
// (D) 例外在 async 裡的完整路徑
//   任務函式 throw
//     → async 內部的包裝器 catch (...)
//     → std::current_exception() 裝箱成 exception_ptr
//     → 存進共享狀態
//     → 呼叫端 fut.get()
//     → 偵測到存的是例外 → std::rethrow_exception()
//     → 在呼叫端以【原始型別】重新拋出
//   全程型別完整保留,自訂例外類別的額外欄位也帶得回來。
//
// 【注意事項 Pay Attention】
//   1. 【最大的陷阱】不接住 std::async 的回傳值 → 臨時 future 立刻解構
//      並阻塞等待 → 變成串行執行,而且沒有任何警告。
//   2. 例外在 get() 處才拋出,不是在任務失敗當下;try/catch 要包 get()。
//   3. 不指定政策時,實作可能選 deferred → 不平行、且沒 get() 就不執行。
//      要平行請明確寫 std::launch::async。
//   4. libstdc++ 的 std::async 每次呼叫都建立新執行緒,不是執行緒池;
//      高頻呼叫請自建 thread pool。
//   5. future::get() 只能呼叫一次;需要多方讀取請用 shared_future。
//   6. 本檔第一段(課文範例)的輸出是確定的 —— 所有輸出都由主執行緒
//      在 get() 之後產生,不受排程影響。
//   7. libstdc++ 15 已把 std::async 標成 [[nodiscard]](_GLIBCXX_NODISCARD),
//      不接住回傳值會產生編譯警告 —— 這正是為了攔截第 1 點那個陷阱。
//      但它【只是警告】,加上 (void) 轉型即可消音,行為完全不變,
//      所以不能依賴編譯器替你把關。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::async 與例外聚合
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::async(std::launch::async, f); 這一行(不接回傳值)有什麼問題?
//     答：回傳的 future 是臨時物件,在該敘述結束時立刻解構。而
//         【由 std::async 建立的 future】其解構函式會阻塞等待任務完成
//         ——這是標準給 async 的特例。所以這行等於「同步呼叫 f()」,
//         寫在迴圈裡就是完全串行,而且沒有任何編譯警告。
//     追問：那為什麼 promise 產生的 future 解構就不阻塞?
//         → 因為 async 擁有那條執行緒,不等待就會讓任務變成沒人管的
//           背景工作(可能存取已銷毀的物件);promise 的情境下執行緒由
//           使用者自己 join,future 只是取結果的把手。這個不對稱常被
//           視為 std::async 的設計瑕疵。
//
// 🔥 Q2. 五個 async 任務中有一個丟例外,其他四個會怎樣?例外何時被看到?
//     答：其他四個完全不受影響,照常完成。失敗的那個把例外裝箱存進
//         自己的共享狀態。例外是在主執行緒呼叫【那一個 future 的 get()】
//         時才重新拋出,不是在任務失敗的當下。所以 try/catch 必須包住
//         get(),包住 std::async(...) 那一行完全沒有作用。
//     追問：例外的型別會被保留嗎?
//         → 會。內部走的是 exception_ptr + rethrow_exception,
//           拋出的是原始的動態型別,自訂例外類別的額外欄位也取得到。
//
// ⚠️ 陷阱 1. 「std::async 不寫政策,反正實作會幫我開執行緒吧?」
//     答：不保證。預設政策是 async|deferred,實作可以選 deferred ——
//         那樣任務不會在背景執行,而是延後到 get() 時在【呼叫端】
//         同步執行;若你從未呼叫 get(),任務【永遠不會執行】。
//         要真的平行,必須明確寫 std::launch::async。
//     為什麼會錯：把「預設」當成「預設會做我想要的事」。這裡的預設
//         其實是「把選擇權交給實作」,而兩個選項的語意天差地遠。
//
// ⚠️ 陷阱 2. 「用 while (fut.wait_for(0s) != std::future_status::ready)
//              輪詢等待完成,這樣比 get() 有彈性。」
//     答：若那個 future 是 deferred 的,這會變成【無窮迴圈】——
//         deferred 的任務要等 get()/wait() 才會執行,它永遠不會自己
//         變成 ready,wait_for 會一直回傳 std::future_status::deferred。
//         正確做法是先檢查是否為 deferred,或一開始就指定 launch::async。
//     為什麼會錯：以為 future 只有 ready / timeout 兩種狀態,
//         忘了還有第三種 deferred —— 而它的語意是「還沒有人開始跑」。
// ═══════════════════════════════════════════════════════════════════════════

/*
# 第三階段：執行緒生命週期管理

## 課程 3.4：執行緒例外處理

---

### 引言

執行緒中拋出的例外不會自動傳遞到建立它的執行緒。如果例外未被捕獲，程式會直接終止。本課學習如何正確處理執行緒中的例外。

---

### 一、問題：例外不會跨執行緒傳遞

```cpp
#include <iostream>
#include <thread>
#include <stdexcept>

void worker() {
    throw std::runtime_error("執行緒內的錯誤！");
}

int main() {
    std::thread t(worker);
    
    try {
        t.join();
    } catch (const std::exception& e) {
        // 這裡捕獲不到！
        std::cout << "捕獲: " << e.what() << std::endl;
    }
    
    return 0;
}
```

輸出：
```
terminate called after throwing an instance of 'std::runtime_error'
Aborted
```

例外在執行緒內未被捕獲，程式直接終止。

---

### 二、解決方案一：執行緒內部捕獲

最簡單的方法是在執行緒函式內部處理例外：

```cpp
#include <iostream>
#include <thread>
#include <stdexcept>

void worker() {
    try {
        throw std::runtime_error("執行緒內的錯誤！");
    } catch (const std::exception& e) {
        std::cout << "執行緒內捕獲: " << e.what() << std::endl;
    }
}

int main() {
    std::thread t(worker);
    t.join();
    
    std::cout << "程式正常結束" << std::endl;
    return 0;
}
```

輸出：
```
執行緒內捕獲: 執行緒內的錯誤！
程式正常結束
```

---

### 三、解決方案二：使用 std::exception_ptr 傳遞例外

```cpp
#include <iostream>
#include <thread>
#include <stdexcept>
#include <exception>

std::exception_ptr threadException = nullptr;

void worker() {
    try {
        throw std::runtime_error("執行緒內的錯誤！");
    } catch (...) {
        // 捕獲並儲存例外
        threadException = std::current_exception();
    }
}

int main() {
    std::thread t(worker);
    t.join();
    
    // 檢查是否有例外
    if (threadException) {
        try {
            std::rethrow_exception(threadException);
        } catch (const std::exception& e) {
            std::cout << "主執行緒捕獲: " << e.what() << std::endl;
        }
    }
    
    return 0;
}
```

輸出：
```
主執行緒捕獲: 執行緒內的錯誤！
```

---

### 四、exception_ptr 關鍵函式

```
┌─────────────────────────────────────────────────────────────┐
│              例外傳遞相關函式                                │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  std::current_exception()                                   │
│  → 捕獲當前例外，回傳 exception_ptr                          │
│                                                             │
│  std::rethrow_exception(ptr)                                │
│  → 重新拋出 exception_ptr 指向的例外                         │
│                                                             │
│  std::make_exception_ptr(e)                                 │
│  → 從例外物件建立 exception_ptr                              │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 五、封裝成類別

```cpp
#include <iostream>
#include <thread>
#include <stdexcept>
#include <exception>
#include <functional>

class SafeThread {
    std::thread t;
    std::exception_ptr exPtr = nullptr;
    
public:
    template<typename Func, typename... Args>
    explicit SafeThread(Func&& f, Args&&... args) {
        t = std::thread([this, f = std::forward<Func>(f), 
                         ...args = std::forward<Args>(args)]() mutable {
            try {
                f(args...);
            } catch (...) {
                exPtr = std::current_exception();
            }
        });
    }
    
    void join() {
        t.join();
        if (exPtr) {
            std::rethrow_exception(exPtr);
        }
    }
    
    ~SafeThread() {
        if (t.joinable()) {
            t.join();
        }
    }
};

int main() {
    try {
        SafeThread st([]() {
            throw std::runtime_error("錯誤！");
        });
        st.join();
    } catch (const std::exception& e) {
        std::cout << "捕獲: " << e.what() << std::endl;
    }
    
    return 0;
}
```

---

### 六、解決方案三：使用 std::future（推薦）

`std::async` 和 `std::future` 自動處理例外傳遞：

```cpp
#include <iostream>
#include <future>
#include <stdexcept>

int worker() {
    throw std::runtime_error("非同步任務的錯誤！");
    return 42;
}

int main() {
    std::future<int> result = std::async(std::launch::async, worker);
    
    try {
        int value = result.get();  // 例外在這裡被重新拋出
        std::cout << "結果: " << value << std::endl;
    } catch (const std::exception& e) {
        std::cout << "捕獲: " << e.what() << std::endl;
    }
    
    return 0;
}
```

輸出：
```
捕獲: 非同步任務的錯誤！
```

---

### 七、使用 std::promise 傳遞例外

```cpp
#include <iostream>
#include <thread>
#include <future>
#include <stdexcept>

void worker(std::promise<int> prom) {
    try {
        throw std::runtime_error("Worker 錯誤！");
        prom.set_value(42);
    } catch (...) {
        prom.set_exception(std::current_exception());
    }
}

int main() {
    std::promise<int> prom;
    std::future<int> fut = prom.get_future();
    
    std::thread t(worker, std::move(prom));
    
    try {
        int value = fut.get();
        std::cout << "結果: " << value << std::endl;
    } catch (const std::exception& e) {
        std::cout << "捕獲: " << e.what() << std::endl;
    }
    
    t.join();
    return 0;
}
```

---

### 八、各種方法比較

| 方法 | 優點 | 缺點 |
|------|------|------|
| 執行緒內捕獲 | 簡單 | 無法傳遞給呼叫者 |
| exception_ptr | 可傳遞任何例外 | 需要手動管理 |
| std::future | 自動傳遞 | 需要用 async 或 promise |
| promise | 靈活控制 | 較複雜 |

---

### 九、最佳實踐

```cpp
#include <iostream>
#include <future>
#include <vector>
#include <stdexcept>

int task(int id) {
    if (id == 2) {
        throw std::runtime_error("Task 2 失敗");
    }
    return id * 10;
}

int main() {
    std::vector<std::future<int>> futures;
    
    // 啟動多個任務
    for (int i = 0; i < 5; ++i) {
        futures.push_back(std::async(std::launch::async, task, i));
    }
    
    // 收集結果，處理例外
    for (int i = 0; i < 5; ++i) {
        try {
            int result = futures[i].get();
            std::cout << "Task " << i << " 結果: " << result << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Task " << i << " 例外: " << e.what() << std::endl;
        }
    }
    
    return 0;
}
```

輸出：
```
Task 0 結果: 0
Task 1 結果: 10
Task 2 例外: Task 2 失敗
Task 3 結果: 30
Task 4 結果: 40
```

---

### 十、本課重點回顧

1. 執行緒中的例外**不會自動傳遞**到其他執行緒
2. 未捕獲的例外會導致 `std::terminate()`
3. `std::exception_ptr` 可以儲存和傳遞例外
4. `std::current_exception()` 捕獲當前例外
5. `std::rethrow_exception()` 重新拋出例外
6. **推薦使用 `std::future`**，它自動處理例外傳遞

---

### 下一課預告

在 **課程 3.5：執行緒本地儲存** 中，我們將學習：
- `thread_local` 關鍵字
- 每個執行緒獨立的變數
- 使用場景與注意事項

---

準備好繼續嗎？
*/



#include <chrono>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

int task(int id) {
    if (id == 2) {
        throw std::runtime_error("Task 2 失敗");
    }
    return id * 10;
}

// -----------------------------------------------------------------------------
// 示範 1:課文範例 —— 例外聚合
//   五個任務中第 2 個會失敗,其餘四個不受影響照常完成。
//   例外在【該 future 的 get()】處才重新拋出。
// -----------------------------------------------------------------------------
void demoTextbook() {
    std::vector<std::future<int>> futures;

    // 啟動多個任務(明確指定 launch::async,保證真的平行)
    for (int i = 0; i < 5; ++i) {
        futures.push_back(std::async(std::launch::async, task, i));
    }

    // 收集結果,個別處理例外
    for (int i = 0; i < 5; ++i) {
        try {
            int result = futures[i].get();
            std::cout << "  Task " << i << " 結果: " << result << std::endl;
        } catch (const std::exception& e) {
            std::cout << "  Task " << i << " 例外: " << e.what() << std::endl;
        }
    }
}

// -----------------------------------------------------------------------------
// 示範 2:try/catch 包錯地方就沒有用
//   例外是在 get() 才拋出,不是在 std::async(...) 那一行。
// -----------------------------------------------------------------------------
void demoCatchPlacement() {
    // ✗ 錯誤示範:包住 async 呼叫本身,什麼都攔不到
    std::future<int> fut;
    try {
        fut = std::async(std::launch::async, task, 2);  // 任務會失敗,但這裡不拋
        std::cout << "  async 呼叫本身沒有拋出任何例外(任務才剛開始跑)\n";
    } catch (const std::exception&) {
        std::cout << "  (這行永遠不會被執行)\n";
    }

    // ✓ 正確示範:包住 get()
    try {
        int v = fut.get();
        std::cout << "  取得結果 " << v << "\n";
    } catch (const std::exception& e) {
        std::cout << "  在 get() 處攔到例外: " << e.what() << "\n";
    }
}

// -----------------------------------------------------------------------------
// 示範 3:★ 不接住回傳值 → 臨時 future 解構阻塞 → 完全串行
//
//   用實測時間證明:三個各睡 100ms 的任務,
//     * 不接住回傳值 → 約 300ms(串行)
//     * 存進 vector   → 約 100ms(平行)
//   這是 std::async 最惡名昭彰的陷阱,而且沒有任何編譯警告。
// -----------------------------------------------------------------------------
void demoTemporaryFutureBlocks() {
    using Clock = std::chrono::steady_clock;
    auto sleep100 = []() { std::this_thread::sleep_for(std::chrono::milliseconds(100)); };

    // (a) 不接住回傳值:每一行的臨時 future 立刻解構並阻塞
    //
    //   ⚠️ 真實世界的 bug 長這樣(沒有 (void) 轉型):
    //         std::async(std::launch::async, sleep100);
    //   但 libstdc++ 15 已把 std::async 標成 _GLIBCXX_NODISCARD(即 [[nodiscard]]),
    //   正是為了攔截這個經典錯誤 —— 直接這樣寫會得到
    //   「ignoring return value ... declared with attribute 'nodiscard'」警告。
    //   本檔為了維持零警告編譯,顯式加上 (void) 轉型;
    //   語意完全相同(臨時 future 仍在該敘述結束時解構並阻塞)。
    //   換句話說:編譯器現在會提醒你,但【只有警告,不是錯誤】——
    //   加了 (void) 或用舊版函式庫就沒有提醒了,行為卻一樣糟。
    auto t0 = Clock::now();
    (void)std::async(std::launch::async, sleep100);   // 臨時 future → 解構即等待
    (void)std::async(std::launch::async, sleep100);
    (void)std::async(std::launch::async, sleep100);
    auto serialMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                        Clock::now() - t0).count();

    // (b) 存進 vector:三個任務真正同時在跑
    t0 = Clock::now();
    {
        std::vector<std::future<void>> futs;
        futs.reserve(3);
        for (int i = 0; i < 3; ++i) {
            futs.push_back(std::async(std::launch::async, sleep100));
        }
    }  // futs 解構 → 此時才等待(三者已平行跑完)
    auto parallelMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                          Clock::now() - t0).count();

    std::cout << "  (a) 不接住回傳值   : 約 " << serialMs   << " ms\n";
    std::cout << "  (b) 存進 vector    : 約 " << parallelMs << " ms\n";
    std::cout << "  (a) 明顯接近三倍 → 證實臨時 future 的解構會阻塞等待\n";
    std::cout << "  兩者相差是否超過 2 倍: " << std::boolalpha
              << (serialMs > parallelMs * 2) << "\n";
}

// -----------------------------------------------------------------------------
// 示範 4:deferred 政策 —— 任務不會自己開始跑
// -----------------------------------------------------------------------------
void demoDeferred() {
    bool executed = false;

    auto fut = std::async(std::launch::deferred, [&executed]() {
        executed = true;
        return 99;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::cout << "  等了 50ms 之後,任務是否已執行: " << std::boolalpha
              << executed << "(deferred 不會自己開始)\n";

    // wait_for 是判斷 deferred 的可靠方式
    auto status = fut.wait_for(std::chrono::seconds(0));
    std::cout << "  wait_for(0) 回傳 deferred: "
              << (status == std::future_status::deferred) << "\n";

    int v = fut.get();   // ← 此刻才在【呼叫端】同步執行
    std::cout << "  呼叫 get() 之後才執行,結果 = " << v
              << ",executed = " << executed << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔【刻意不提供】
//
//   說明:LeetCode 的多執行緒題(1114 Print in Order、1115 Print FooBar
//   Alternately、1116 Print Zero Even Odd、1117 Building H2O、
//   1195 Fizz Buzz Multithreaded)考的是【執行緒間的順序協調】,
//   由評測框架建立執行緒並呼叫成員函式,既沒有回傳值、也沒有錯誤路徑,
//   更不會用到 std::async 或 future。本檔主題是【非同步任務的結果與
//   例外聚合】,與之無交集,故從缺。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】服務就緒探測(readiness probe):平行檢查所有下游依賴
//
//   情境:Kubernetes 的 readiness probe 打進來時,服務要在數百毫秒內
//         回答「我準備好了嗎」。而「準備好」意味著所有下游依賴
//         (資料庫、快取、訊息佇列、物件儲存)都連得上。
//
//   為何用 async + future:
//     * 這些檢查彼此獨立,必須【平行】做 —— 串行做四個各 30ms 的檢查
//       就要 120ms,平行只要 30ms,對 probe 的時限差很多。
//     * 每個檢查都可能以不同的例外型別失敗,而我們需要
//       【逐項的成敗與原因】,不是一個籠統的 false。
//     * async 讓每個檢查寫成普通的「回傳值或丟例外」函式,
//       錯誤處理完全沿用同步程式的寫法。
//
//   關鍵:所有 future 都存進 vector,絕不寫成 std::async(...); 這種
//         不接回傳值的形式 —— 否則四個檢查會變成串行,probe 就超時了。
// -----------------------------------------------------------------------------
class DependencyDown : public std::runtime_error {
    std::string dep_;
public:
    DependencyDown(const std::string& dep, const std::string& why)
        : std::runtime_error(why), dep_(dep) {}
    const std::string& dependency() const noexcept { return dep_; }
};

struct ProbeResult {
    std::string name;
    bool        healthy;
    std::string detail;
};

// 模擬一次健康檢查:回傳延遲毫秒數,或丟出 DependencyDown
int checkDependency(const std::string& name, bool willFail) {
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    if (willFail) {
        throw DependencyDown(name, "connection refused");
    }
    return 30;
}

std::vector<ProbeResult> runReadinessProbe(const std::vector<std::string>& deps,
                                           const std::string& failing) {
    // ⚠️ 必須存起來;寫成 std::async(...); 會變成串行
    std::vector<std::future<int>> futures;
    futures.reserve(deps.size());

    for (const auto& d : deps) {
        futures.push_back(
            std::async(std::launch::async, checkDependency, d, d == failing));
    }

    std::vector<ProbeResult> results;
    results.reserve(deps.size());
    for (std::size_t i = 0; i < deps.size(); ++i) {
        try {
            int ms = futures[i].get();
            results.push_back({deps[i], true, "ok (" + std::to_string(ms) + "ms)"});
        } catch (const DependencyDown& e) {
            // 原始型別完整保留,取得到自訂的 dependency() 欄位
            results.push_back({deps[i], false,
                               "FAILED [" + e.dependency() + "]: " + e.what()});
        }
    }
    return results;
}

int main() {
    std::cout << "=== 示範 1:課文範例(例外聚合)===" << std::endl;
    demoTextbook();

    std::cout << "\n=== 示範 2:try/catch 該包在哪裡 ===" << std::endl;
    demoCatchPlacement();

    std::cout << "\n=== 示範 3:不接住回傳值 → 串行執行 ===" << std::endl;
    demoTemporaryFutureBlocks();

    std::cout << "\n=== 示範 4:deferred 政策 ===" << std::endl;
    demoDeferred();

    std::cout << "\n=== 日常實務:服務就緒探測 ===" << std::endl;
    std::vector<std::string> deps{"postgres", "redis", "kafka", "s3"};
    auto results = runReadinessProbe(deps, "kafka");

    int healthy = 0;
    for (const auto& r : results) {
        std::cout << "  " << (r.healthy ? "[UP]   " : "[DOWN] ")
                  << r.name << " → " << r.detail << "\n";
        if (r.healthy) ++healthy;
    }
    std::cout << "  就緒狀態: " << healthy << " / " << results.size()
              << " 個依賴正常 → readiness = "
              << std::boolalpha << (healthy == static_cast<int>(results.size()))
              << "\n";

    std::cout << "\n=== 全部示範結束 ===" << std::endl;
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.4：執行緒例外處理7.cpp" -o exc7

// 註:示範 3 印出的【毫秒數】取決於機器與當下負載,每次執行都不同 ——
//     有意義的是兩者的【比例】(不接回傳值約為存進 vector 的三倍),
//     那個比例才是本示範要證明的事,程式最後一行也直接驗證它。
//     其餘各段的輸出都是確定的(全部由主執行緒在 get() 之後產生)。

// === 預期輸出 ===
// === 示範 1:課文範例(例外聚合)===
//   Task 0 結果: 0
//   Task 1 結果: 10
//   Task 2 例外: Task 2 失敗
//   Task 3 結果: 30
//   Task 4 結果: 40
//
// === 示範 2:try/catch 該包在哪裡 ===
//   async 呼叫本身沒有拋出任何例外(任務才剛開始跑)
//   在 get() 處攔到例外: Task 2 失敗
//
// === 示範 3:不接住回傳值 → 串行執行 ===
//   (a) 不接住回傳值   : 約 300 ms
//   (b) 存進 vector    : 約 100 ms
//   (a) 明顯接近三倍 → 證實臨時 future 的解構會阻塞等待
//   兩者相差是否超過 2 倍: true
//
// === 示範 4:deferred 政策 ===
//   等了 50ms 之後,任務是否已執行: false(deferred 不會自己開始)
//   wait_for(0) 回傳 deferred: true
//   呼叫 get() 之後才執行,結果 = 99,executed = true
//
// === 日常實務:服務就緒探測 ===
//   [UP]   postgres → ok (30ms)
//   [UP]   redis → ok (30ms)
//   [DOWN] kafka → FAILED [kafka]: connection refused
//   [UP]   s3 → ok (30ms)
//   就緒狀態: 3 / 4 個依賴正常 → readiness = false
//
// === 全部示範結束 ===
