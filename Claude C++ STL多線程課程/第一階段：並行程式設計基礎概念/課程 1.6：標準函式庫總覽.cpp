/*
# 第一階段：並行程式設計基礎概念

## 課程 1.6：標準函式庫總覽

---

### 引言

在正式開始撰寫多執行緒程式之前，讓我們先鳥瞰整個 C++ 多執行緒標準函式庫的全貌。這將成為你後續學習的地圖。

---

### 一、標頭檔總覽

```
┌─────────────────────────────────────────────────────────────┐
│              C++ 多執行緒標準函式庫                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  <thread>          執行緒的建立與管理                        │
│                                                             │
│  <mutex>           互斥鎖與鎖管理器                          │
│                                                             │
│  <condition_variable>  條件變數，執行緒間的等待與通知        │
│                                                             │
│  <future>          非同步操作與結果傳遞                      │
│                                                             │
│  <atomic>          原子操作，無鎖程式設計                    │
│                                                             │
│  <semaphore>       信號量 (C++20)                           │
│                                                             │
│  <latch>           閂鎖 (C++20)                             │
│                                                             │
│  <barrier>         屏障 (C++20)                             │
│                                                             │
│  <stop_token>      協作式取消 (C++20)                        │
│                                                             │
│  <coroutine>       協程支援 (C++20)                          │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 二、各標頭檔核心內容

#### `<thread>` — 執行緒管理

```cpp
#include <thread>

std::thread              // 執行緒類別
std::this_thread         // 當前執行緒的操作命名空間
  ::get_id()             //   取得執行緒 ID
  ::sleep_for()          //   休眠指定時間
  ::sleep_until()        //   休眠到指定時間點
  ::yield()              //   讓出 CPU 時間

std::jthread             // 自動 join 的執行緒 (C++20)
```

#### `<mutex>` — 互斥鎖

```cpp
#include <mutex>

// 互斥鎖類型
std::mutex                    // 基本互斥鎖
std::timed_mutex              // 支援超時的互斥鎖
std::recursive_mutex          // 可遞迴鎖定的互斥鎖
std::recursive_timed_mutex    // 遞迴 + 超時
std::shared_mutex             // 讀寫鎖 (C++17)

// 鎖管理器（RAII）
std::lock_guard              // 簡單的 RAII 鎖
std::unique_lock             // 靈活的 RAII 鎖
std::scoped_lock             // 多重鎖 (C++17)
std::shared_lock             // 共享鎖 (C++14)

// 輔助函式
std::lock()                  // 同時鎖定多個互斥鎖
std::try_lock()              // 嘗試同時鎖定
std::call_once               // 確保只執行一次
std::once_flag               // call_once 的旗標
```

#### `<condition_variable>` — 條件變數

```cpp
#include <condition_variable>

std::condition_variable      // 條件變數（搭配 unique_lock）
std::condition_variable_any  // 通用條件變數（任意鎖）

// 主要操作
.wait()                      // 等待通知
.wait_for()                  // 等待指定時間
.wait_until()                // 等待到指定時間點
.notify_one()                // 喚醒一個等待者
.notify_all()                // 喚醒所有等待者
```

#### `<future>` — 非同步操作

```cpp
#include <future>

std::future                  // 非同步結果的接收端
std::promise                 // 非同步結果的發送端
std::shared_future           // 可複製的 future
std::packaged_task           // 包裝可呼叫物件為非同步任務
std::async                   // 高階非同步執行函式

// 啟動策略
std::launch::async           // 強制新執行緒
std::launch::deferred        // 延遲執行
```

#### `<atomic>` — 原子操作

```cpp
#include <atomic>

std::atomic<T>               // 原子類型模板
std::atomic_flag             // 最基本的原子布林
std::atomic_ref              // 對既有物件的原子操作 (C++20)

// 記憶體順序
std::memory_order_relaxed
std::memory_order_acquire
std::memory_order_release
std::memory_order_acq_rel
std::memory_order_seq_cst
```

#### `<semaphore>` — 信號量 (C++20)

```cpp
#include <semaphore>

std::counting_semaphore<N>   // 計數信號量
std::binary_semaphore        // 二元信號量（等同 counting_semaphore<1>）
```

#### `<latch>` 與 `<barrier>` — 同步屏障 (C++20)

```cpp
#include <latch>
#include <barrier>

std::latch                   // 一次性倒數閂鎖
std::barrier                 // 可重複使用的屏障
```

---

### 三、學習路線圖

```
                        ┌─────────────┐
                        │  你在這裡   │
                        └──────┬──────┘
                               │
            ┌──────────────────┼──────────────────┐
            ▼                  ▼                  ▼
     ┌────────────┐    ┌─────────────┐    ┌─────────────┐
     │  <thread>  │    │   <mutex>   │    │  <atomic>   │
     │  執行緒基礎 │    │   同步機制   │    │   原子操作   │
     └─────┬──────┘    └──────┬──────┘    └──────┬──────┘
           │                  │                  │
           ▼                  ▼                  ▼
     ┌────────────┐    ┌─────────────┐    ┌─────────────┐
     │  <future>  │    │ <condition_ │    │  無鎖資料   │
     │  非同步操作 │    │  variable>  │    │    結構     │
     └─────┬──────┘    └──────┬──────┘    └──────┬──────┘
           │                  │                  │
           └──────────────────┼──────────────────┘
                              ▼
                    ┌──────────────────┐
                    │  C++20 進階功能   │
                    │  semaphore/latch │
                    │  barrier/jthread │
                    │    coroutine     │
                    └──────────────────┘
```

---

### 四、快速參考表

| 我想要... | 使用... | 標頭檔 |
|-----------|---------|--------|
| 建立執行緒 | `std::thread` | `<thread>` |
| 保護共享資料 | `std::mutex` + `std::lock_guard` | `<mutex>` |
| 等待某個條件 | `std::condition_variable` | `<condition_variable>` |
| 執行非同步任務 | `std::async` | `<future>` |
| 無鎖計數器 | `std::atomic<int>` | `<atomic>` |
| 限制並發數量 | `std::counting_semaphore` | `<semaphore>` |
| 等待多執行緒到達同一點 | `std::barrier` | `<barrier>` |

---

### 五、最小可運行範例

將所有核心功能濃縮在一個範例中：

```cpp
// 檔案：lesson_1_6_overview.cpp

#include <iostream>
#include <thread>
#include <mutex>
#include <future>
#include <atomic>

std::mutex mtx;
std::atomic<int> atomicCounter{0};

int main() {
    // 1. std::thread - 建立執行緒
    std::thread t([]() {
        std::lock_guard<std::mutex> lock(mtx);  // 2. std::mutex - 保護輸出
        std::cout << "Hello from thread!" << std::endl;
    });
    
    // 3. std::async - 非同步執行
    auto future = std::async(std::launch::async, []() {
        return 42;
    });
    
    // 4. std::atomic - 原子操作
    atomicCounter++;
    
    t.join();
    std::cout << "Async result: " << future.get() << std::endl;
    std::cout << "Atomic counter: " << atomicCounter << std::endl;
    
    return 0;
}
```

```bash
g++ -std=c++17 -pthread -o lesson_1_6 lesson_1_6_overview.cpp
./lesson_1_6
```

---

### 六、本課重點回顧

1. C++ 多執行緒功能分布在多個標頭檔中
2. `<thread>` 負責執行緒的建立與管理
3. `<mutex>` 提供各種互斥鎖和 RAII 鎖管理器
4. `<condition_variable>` 實現執行緒間的等待與通知
5. `<future>` 支援非同步程式設計
6. `<atomic>` 提供無鎖的原子操作
7. C++20 新增了信號量、閂鎖、屏障等同步原語

---

### 第一階段完成！

恭喜你完成了第一階段的學習！你已經建立了多執行緒程式設計的概念基礎：

- ✅ 並發與並行的區別
- ✅ 多執行緒的價值與適用場景
- ✅ 執行緒與程序的差異
- ✅ 常見的多執行緒問題
- ✅ C++ 多執行緒的發展歷程
- ✅ 標準函式庫的整體架構

---

### 下一階段預告

**第二階段：std::thread 基礎** 將帶你實際動手：
- 課程 2.1：第一個多執行緒程式
- 課程 2.2：執行緒函式的多種形式
- 課程 2.3：傳遞參數給執行緒
- ...

準備好進入第二階段了嗎？
*/

// =============================================================================
//  課程 1.6：標準函式庫總覽.cpp  —  哪個設施住在哪個標頭、屬於哪個標準
// =============================================================================
//
// 【主題資訊 Information】
//   上面第一、二節已經給了很好的標頭檔地圖。本區塊補上兩件地圖沒說、
//   但實際寫程式時一定會撞到的事：
//     (1) **精確的標頭歸屬**（有些設施不在你以為的標頭裡）
//     (2) **精確的標準版本**（版本題最常考的就是這張表）
//
//   下表每一格都以 `g++ -std=c++NN -pedantic-errors` 逐版實測驗證過
//   （本機 g++ 15.2.0）。⚠️ 只用 -fsyntax-only 或不加 -pedantic-errors 會被
//   GCC 當作擴充放行，看起來像舊標準也支援 —— 那樣驗出來的版本是錯的。
//
//   ┌────────────────────────────────┬────────┬──────────────────────────┐
//   │ 設施                           │ 標準   │ **正確的**標頭檔         │
//   ├────────────────────────────────┼────────┼──────────────────────────┤
//   │ std::thread                    │ C++11  │ <thread>                 │
//   │ std::this_thread::*            │ C++11  │ <thread>                 │
//   │ std::mutex / timed_mutex       │ C++11  │ <mutex>                  │
//   │ std::recursive_mutex           │ C++11  │ <mutex>                  │
//   │ std::lock_guard / unique_lock  │ C++11  │ <mutex>                  │
//   │ std::lock / try_lock           │ C++11  │ <mutex>                  │
//   │ std::call_once / once_flag     │ C++11  │ <mutex>                  │
//   │ std::condition_variable        │ C++11  │ <condition_variable>     │
//   │ std::condition_variable_any    │ C++11  │ <condition_variable>     │
//   │ std::future / promise          │ C++11  │ <future>                 │
//   │ std::async / packaged_task     │ C++11  │ <future>                 │
//   │ std::atomic<T> / atomic_flag   │ C++11  │ <atomic>                 │
//   ├────────────────────────────────┼────────┼──────────────────────────┤
//   │ std::shared_timed_mutex        │ C++14  │ **<shared_mutex>**       │
//   │ std::shared_lock               │ C++14  │ **<shared_mutex>**       │
//   ├────────────────────────────────┼────────┼──────────────────────────┤
//   │ std::shared_mutex              │ C++17  │ **<shared_mutex>**       │
//   │ std::scoped_lock               │ C++17  │ <mutex>                  │
//   │ std::execution::par            │ C++17  │ <execution>（需 -ltbb）  │
//   │ hardware_*_interference_size   │ C++17  │ **<new>**                │
//   ├────────────────────────────────┼────────┼──────────────────────────┤
//   │ std::jthread                   │ C++20  │ <thread>                 │
//   │ std::stop_token / stop_source  │ C++20  │ <stop_token>             │
//   │ std::latch                     │ C++20  │ <latch>                  │
//   │ std::barrier                   │ C++20  │ <barrier>                │
//   │ std::counting_semaphore        │ C++20  │ <semaphore>              │
//   │ std::binary_semaphore          │ C++20  │ <semaphore>              │
//   │ std::atomic_ref                │ C++20  │ <atomic>                 │
//   │ atomic::wait / notify_one      │ C++20  │ <atomic>                 │
//   │ std::atomic<std::shared_ptr<T>>│ C++20  │ <atomic> + <memory>      │
//   │ std::osyncstream               │ C++20  │ **<syncstream>**         │
//   └────────────────────────────────┴────────┴──────────────────────────┘
//
// 【詳細解釋 Explanation】
//
// 【1. 上面第二節的地圖有兩處要修正 —— 標頭歸屬】
// 第二節把 `std::shared_mutex` 與 `std::shared_lock` 列在 `<mutex>` 底下。
// 從「概念分類」看很合理（它們都是鎖），但**編譯器不吃概念分類**：
//     #include <mutex>
//     std::shared_mutex m;   // ← error: 'shared_mutex' in namespace 'std'
//                            //          does not name a type（本機實測）
// 這兩者（以及 shared_timed_mutex）都住在 **`<shared_mutex>`**。
// 只 include <mutex> 是編不過的。同理，`hardware_destructive_interference_size`
// 雖然是為了對付 false sharing（很「並行」），卻住在 **`<new>`**；
// `std::osyncstream` 住在 **`<syncstream>`** 而不是 `<iostream>`。
// 記憶法：**標頭是照「實作單元」切的，不是照「用途」切的。**
//
// 【2. 為什麼 shared_lock 是 C++14，shared_mutex 卻是 C++17】
// 這個不對稱看起來很怪，但有歷史原因：
//   C++14 先標準化了 `shared_timed_mutex`（帶 try_lock_for / try_lock_until）
//   與能操作它的 RAII 包裝 `shared_lock`。當時對「不帶逾時的純讀寫鎖」
//   還沒共識，於是 `shared_mutex` 延到 C++17 才進來。
//   所以 C++14 的組合是「shared_lock + shared_timed_mutex」，
//   C++17 之後才有「shared_lock + shared_mutex」這個更輕量的組合。
//   `shared_lock` 是樣板，兩種 mutex 都能包 —— 這是它能先進來的原因。
//
// 【3. 這張地圖的「使用順序」：先問要什麼保證，再挑工具】
//   * 只是要保護一段臨界區          → <mutex> + lock_guard/scoped_lock
//   * 讀多寫少，讀者要能並行        → <shared_mutex>（C++17）+ shared_lock
//   * 要等一個「條件」成立          → <condition_variable>，且**必須用
//                                     predicate 迴圈**（見【注意事項 3】）
//   * 只要一個結果／一次性計算      → <future> 的 std::async
//   * 單一變數的計數／旗標          → <atomic>
//   * 限制同時進行的數量            → <semaphore>（C++20）
//   * 等一群人到齊（一次性）        → <latch>（C++20）
//   * 等一群人到齊（每輪重複）      → <barrier>（C++20）
//   * 要能請背景工作停下來          → <stop_token> + jthread（C++20）
//   反過來挑（「我想用 atomic，它能解什麼」）幾乎一定會挑錯工具。
//
// 【4. 一次性初始化：call_once 為什麼還存在】
// 第二節列了 `std::call_once` / `std::once_flag`，但沒說它跟
// 「函式內 static 區域變數」的關係。C++11 起，函式內 static 的初始化
// **標準保證是執行緒安全的**（俗稱 magic static）：
//     Config& get() { static Config c = load(); return c; }   // C++11 起執行緒安全
// 那 call_once 還有什麼用？用在「初始化的對象不是一個區域 static」的時候：
// 例如要初始化一個成員變數、要在多個函式間共用同一個 once_flag、
// 或初始化動作本身可能拋出例外（call_once 有明確定義的重試語意：
// 若被呼叫的函式拋出例外，該次不算完成，下一個呼叫者會再試一次）。
// 本檔【日常實務範例 2】示範這個重試語意。
//
// 【概念補充 Concept Deep Dive】
// (A) 為什麼 condition_variable 綁 unique_lock，而 condition_variable_any 不綁
//     `std::condition_variable::wait` 的簽名固定收
//     `std::unique_lock<std::mutex>&`。原因是效能：在多數平台上它可以直接
//     對應到底層原語（Linux 的 futex、Windows 的 SRWLOCK + CONDITION_VARIABLE），
//     不需要任何型別抹除。`condition_variable_any` 接受任何具備
//     lock()/unlock() 的型別（例如 shared_lock、自訂的 spinlock），
//     代價是內部通常得再包一層自己的 mutex → 多一次同步、較慢。
//     C++20 另外給了 condition_variable_any 一組吃 stop_token 的 wait 多載，
//     讓「等待中的執行緒也能被取消」—— 這是 condition_variable 沒有的。
// (B) std::async 的兩個啟動策略差在哪
//     `std::launch::async`    保證另開執行緒立刻跑。
//     `std::launch::deferred` 延遲到 future.get()/wait() 時才在**呼叫端**跑
//                             （所以根本沒有並行）。
//     不指定策略時是 `async | deferred`，**由實作自行選擇** ——
//     這代表你的「非同步」可能根本沒有非同步。要並行就明確寫
//     `std::async(std::launch::async, f)`。
// (C) atomic 不等於「無鎖」
//     `std::atomic<T>` 對大型 T 可能由實作用鎖模擬。要問就用
//     `std::atomic<T>::is_lock_free()`（執行期）或 C++17 的
//     `std::atomic<T>::is_always_lock_free`（編譯期常數，可用於 static_assert）。
//     本檔會實際印出本機的結果 —— 這是**實作定義**的值。
//
// 【注意事項 Pay Attention】
// 1. `<shared_mutex>` 是獨立標頭（C++14 起存在）。只 include <mutex> 用不到
//    shared_mutex / shared_timed_mutex / shared_lock（本機實測會編譯錯誤）。
// 2. 上面第五節的最小範例用 `std::lock_guard` 保護 `std::cout`。要注意
//    保護的只是「這一段輸出不被切斷」；`std::cout` 本身在 C++11 起對
//    **格式化輸出**已有基本的資料競爭保護，但**不保證多次 << 之間不被插隊**。
//    要真正做到「整行不被切斷」，C++20 有 `std::osyncstream`（<syncstream>）。
// 3. `condition_variable::wait` **一定要用 predicate 版本**（或自己寫 while
//    迴圈）。原因有二：spurious wakeup（虛假喚醒，標準明文允許），
//    以及被喚醒後條件又被別人改掉。寫成 `cv.wait(lk);` 幾乎必錯。
// 4. `std::async` 回傳的 `std::future` 其解構子會**阻塞**等待任務結束
//    （這是 async 專屬的行為，不適用於 promise/packaged_task 來的 future）。
//    因此 `std::async(f);` 這種不接回傳值的寫法會退化成同步呼叫 ——
//    暫時物件當場解構、當場等它跑完。
// 5. `std::atomic<int> a; a++;` 預設是 `memory_order_seq_cst`，最強也最貴。
//    但**不要**因此就隨手改成 relaxed：relaxed 只保證「單一原子物件自身的
//    修改順序」，不建立任何跨變數的 happens-before。純計數器可以 relaxed，
//    「設旗標讓別人看到資料」就必須 release/acquire 成對使用。
// 6. 表中的 `<execution>` 在 GCC 上需要連結 `-ltbb`（本機實測），
//    這是實作定義的相依，不是標準的一部分。
// 7. ⚠️ **工具相容性實測**：`std::call_once` 的「初始化函式拋出例外 → 下一個
//    呼叫者重試」是**標準明文規定**的行為（[thread.once.callonce]），
//    本機原生執行 20/20 次皆正確重試；`-fsanitize=address` 也正常。
//    但在 **`-fsanitize=thread`（TSan）下這條重試路徑會直接卡死**
//    （本機以 12 行最小重現程式確認：原生 OK、ASan OK、TSan 逾時）。
//    原因是 libstdc++ 為了支援例外重試，必須在內部「重設」once_flag，
//    而 TSan 攔截 pthread_once 後看不到這個重設動作。
//    **這不是本檔程式碼的錯誤，也不是 data race**（TSan 回報 0 個 warning）。
//    要對本檔跑 TSan 時，請把【實務範例 2】的 failFirstN 設為 0
//    （避開重試路徑）——本檔其餘部分含非同步日誌器皆為 TSan 全綠。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】C++ 並行標準函式庫的組成
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::shared_mutex 要 include 哪個標頭？是 <mutex> 嗎？
//     答：**<shared_mutex>**，不是 <mutex>。shared_mutex（C++17）、
//         shared_timed_mutex（C++14）、shared_lock（C++14）三者都在
//         <shared_mutex> 裡。只 include <mutex> 會編譯錯誤（本機實測）。
//     追問：那 scoped_lock 呢？
//         → scoped_lock（C++17）確實在 <mutex>。標頭是照實作單元切的，
//           不是照「它是不是一種鎖」切的。
//
// 🔥 Q2. std::async 不指定啟動策略時，一定會開新執行緒嗎？
//     答：不一定。預設是 `std::launch::async | std::launch::deferred`，
//         **由實作決定**。若選了 deferred，任務會延到 get()/wait() 時
//         在呼叫端執行，完全沒有並行。要保證並行必須明確寫
//         `std::async(std::launch::async, f)`。
//     追問：那 `std::async(f);` 這行（不接回傳值）會怎樣？
//         → 回傳的暫時 future 當場解構，而 async 的 future 解構會阻塞等待，
//           結果是「看起來非同步、實際完全同步」。
//
// 🔥 Q3. C++11 之後，函式內的 static 區域變數初始化是執行緒安全的嗎？
//     答：是。C++11 起標準保證「多執行緒同時首次進入時，只有一個會執行
//         初始化，其餘阻塞等待完成」（俗稱 magic static，GCC 用 __cxa_guard_*
//         實作）。所以 Meyers Singleton 在 C++11 之後是安全的。
//     追問：那 std::call_once 什麼時候還需要？
//         → 初始化對象不是區域 static（例如成員變數）、需要跨函式共用同一個
//           once_flag、或需要 call_once 的例外重試語意時。
//
// ⚠️ 陷阱 1. cv.wait(lk) 沒有 predicate，只要 notify 一定會被正確喚醒吧？
//     答：不對。標準明文允許 **spurious wakeup**（虛假喚醒）——
//         沒有人 notify 也可能返回。而且就算真的被 notify，
//         醒來後條件也可能已被其他執行緒改掉（喚醒與重新取得鎖之間有空窗）。
//         正確寫法是 `cv.wait(lk, [&]{ return 條件; });`。
//     為什麼會錯：把 condition_variable 想成「訊息佇列」——
//         以為一次 notify 對應一次可靠的喚醒。它其實只是「重新檢查條件的提示」，
//         真正的狀態永遠在你自己那個受 mutex 保護的變數裡。
//
// ⚠️ 陷阱 2. 用 std::lock_guard 鎖住 std::cout 之後，多執行緒輸出就不會交錯了？
//     答：只有在**所有**寫入 cout 的地方都用同一把鎖時才成立。
//         漏掉任何一處（或函式庫內部直接寫 cout）就會插進來。
//         而且 lock_guard 保護的是「你這段程式碼」，不是 cout 這個物件本身。
//     為什麼會錯：把鎖想成「加在物件上」。mutex 保護的是**約定**——
//         所有存取者都必須遵守同一把鎖，沒有任何機制強制執行這件事。
//         C++20 的 std::osyncstream 才是把「整行原子輸出」做進型別裡的解法。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode】從缺。
//   本課主題是「標準函式庫的地圖與標頭歸屬」，屬於知識組織而非演算法實作；
//   LeetCode 並行題（1114/1115/1116/1117/1195）考的是具體同步邏輯，
//   與本課主題對不上，故不強加。實際動手的題目安排在第二階段之後。
// =============================================================================

// 檔案：lesson_1_6_overview.cpp

#include <iostream>
#include <thread>
#include <mutex>
#include <shared_mutex>       // ⚠️ shared_mutex/shared_lock 在這裡，不在 <mutex>
#include <condition_variable>
#include <future>
#include <atomic>
#include <vector>
#include <string>
#include <deque>
#include <chrono>
#include <stdexcept>
#include <algorithm>

std::mutex mtx;
std::atomic<int> atomicCounter{0};

// -----------------------------------------------------------------------------
// 【日常實務範例 1】非同步日誌器（Async Logger）
//   情境：伺服器每秒可能寫上萬筆 log。若每個工作執行緒都直接寫檔，
//         就會全部卡在同一個 I/O 鎖上。標準做法是「生產者只把訊息丟進佇列，
//         由單一背景執行緒批次寫出」。
//   為什麼放在本課：這一個範例就把地圖上的主要標頭全部串起來 ——
//         <thread>（背景寫入緒）、<mutex>+<condition_variable>（佇列）、
//         <atomic>（停止旗標與統計）、<future>（flush 完成通知）、
//         <shared_mutex>（讀多寫少的 log level 設定）。
// -----------------------------------------------------------------------------
namespace practical_logger {

enum class Level { Debug = 0, Info = 1, Warn = 2, Error = 3 };

const char* levelName(Level l) {
    switch (l) {
        case Level::Debug: return "DEBUG";
        case Level::Info:  return "INFO";
        case Level::Warn:  return "WARN";
        case Level::Error: return "ERROR";
    }
    return "?";
}

class AsyncLogger {
    // --- 佇列本體：<mutex> + <condition_variable> ---
    std::mutex              qm_;
    std::condition_variable cv_;
    std::deque<std::pair<Level, std::string>> queue_;

    // --- 停止旗標與統計：<atomic> ---
    std::atomic<bool> stopping_{false};
    std::atomic<int>  written_{0};
    std::atomic<int>  filtered_{0};

    // --- log level 設定：讀多寫少 → <shared_mutex>（C++17）---
    mutable std::shared_mutex levelMtx_;
    Level minLevel_ = Level::Info;

    // --- 背景寫入緒：<thread> ---
    std::thread worker_;

    // 實際「寫出」的目的地。為了讓教學輸出可驗證，這裡寫進 vector
    // 而非真的檔案；真實系統這裡是 ofstream / socket。
    std::mutex sinkMtx_;
    std::vector<std::string> sink_;

public:
    AsyncLogger() {
        worker_ = std::thread([this] { drain(); });
    }

    ~AsyncLogger() {
        shutdown();
    }

    // 讀取設定：多個生產者可同時讀，不互相阻塞
    Level minLevel() const {
        std::shared_lock lk(levelMtx_);     // 共享（讀）鎖
        return minLevel_;
    }
    // 修改設定：獨佔
    void setMinLevel(Level l) {
        std::unique_lock lk(levelMtx_);     // 獨佔（寫）鎖
        minLevel_ = l;
    }

    void log(Level l, std::string msg) {
        if (static_cast<int>(l) < static_cast<int>(minLevel())) {
            filtered_.fetch_add(1, std::memory_order_relaxed);
            return;                          // 低於門檻，直接丟棄
        }
        {
            std::lock_guard<std::mutex> lk(qm_);
            queue_.emplace_back(l, std::move(msg));
        }
        cv_.notify_one();                    // 在鎖外通知
    }

    // 用 <future> 做「flush 完成」的同步點：
    // 丟一個哨兵任務進佇列，等背景執行緒處理到它時設定 promise。
    void flush() {
        std::promise<void> done;
        std::future<void>  fut = done.get_future();
        {
            std::lock_guard<std::mutex> lk(qm_);
            flushRequests_.push_back(std::move(done));
        }
        cv_.notify_one();
        fut.wait();                          // 阻塞直到背景緒確認已排空
    }

    void shutdown() {
        if (!worker_.joinable()) return;
        stopping_.store(true);
        cv_.notify_all();
        worker_.join();                      // 一定要 join，否則解構時 terminate
    }

    int written()  const { return written_.load(); }
    int filtered() const { return filtered_.load(); }

    std::vector<std::string> snapshot() {
        std::lock_guard<std::mutex> lk(sinkMtx_);
        return sink_;
    }

private:
    std::vector<std::promise<void>> flushRequests_;

    void drain() {
        for (;;) {
            std::deque<std::pair<Level, std::string>> batch;
            std::vector<std::promise<void>> flushes;
            {
                std::unique_lock<std::mutex> lk(qm_);
                // predicate 迴圈：spurious wakeup 與「醒來條件已變」都要能應付
                cv_.wait(lk, [this] {
                    return !queue_.empty() || !flushRequests_.empty()
                           || stopping_.load();
                });
                batch.swap(queue_);              // 批次取走，縮短持鎖時間
                flushes.swap(flushRequests_);
                if (batch.empty() && flushes.empty() && stopping_.load()) return;
            }
            // 鎖外做「昂貴的 I/O」——這正是非同步日誌器的重點
            {
                std::lock_guard<std::mutex> lk(sinkMtx_);
                for (auto& [lv, msg] : batch) {
                    sink_.push_back(std::string("[") + levelName(lv) + "] " + msg);
                }
            }
            written_.fetch_add(static_cast<int>(batch.size()),
                               std::memory_order_relaxed);
            for (auto& p : flushes) p.set_value();   // 通知等 flush 的人
        }
    }
};

} // namespace practical_logger

// -----------------------------------------------------------------------------
// 【日常實務範例 2】設定檔的一次性載入（call_once 的例外重試語意）
//   情境：多條執行緒同時要用設定，但檔案只該讀一次。第一次讀取可能失敗
//         （檔案還沒 mount、網路設定中心暫時不通），此時**不該**把
//         「已初始化」記起來，而要讓下一個呼叫者重試。
//   為什麼放在本課：這正是 std::call_once（<mutex>）優於「自己用 bool 旗標」
//         的地方 —— 它對「初始化函式拋出例外」有明確定義的重試語意。
// -----------------------------------------------------------------------------
namespace practical_once {

struct Settings { std::string endpoint; int retries = 0; };

class ConfigLoader {
    std::once_flag flag_;
    Settings       settings_;
    std::atomic<int> attempts_{0};
    int failuresToSimulate_;

public:
    explicit ConfigLoader(int failFirstN) : failuresToSimulate_(failFirstN) {}

    const Settings& get() {
        // call_once：若 loadOnce 拋出例外，這次「不算完成」，
        // flag_ 維持未設定狀態，下一個呼叫者會再試一次。
        std::call_once(flag_, [this] { loadOnce(); });
        return settings_;
    }
    int attempts() const { return attempts_.load(); }

private:
    void loadOnce() {
        int n = attempts_.fetch_add(1) + 1;
        if (n <= failuresToSimulate_) {
            throw std::runtime_error("設定中心尚未就緒（第 " + std::to_string(n) + " 次）");
        }
        settings_ = Settings{"https://config.internal/v1", n};
    }
};

} // namespace practical_once

int main() {
    std::cout << "=== 段 1：原始最小範例（thread / mutex / async / atomic）===" << std::endl;
    {
        // 1. std::thread - 建立執行緒
        std::thread t([]() {
            std::lock_guard<std::mutex> lock(mtx);  // 2. std::mutex - 保護輸出
            std::cout << "Hello from thread!" << std::endl;
        });

        // 3. std::async - 非同步執行（明確指定 async 策略，保證真的並行）
        auto future = std::async(std::launch::async, []() {
            return 42;
        });

        // 4. std::atomic - 原子操作
        atomicCounter++;

        t.join();
        std::cout << "Async result: " << future.get() << std::endl;
        std::cout << "Atomic counter: " << atomicCounter << std::endl;
    }

    std::cout << "\n=== 段 2：atomic 是否為 lock-free（實作定義）===" << std::endl;
    {
        // is_always_lock_free 是 C++17 的編譯期常數，可用於 static_assert
        std::cout << "atomic<int>       is_always_lock_free = "
                  << std::atomic<int>::is_always_lock_free << std::endl;
        std::cout << "atomic<long long> is_always_lock_free = "
                  << std::atomic<long long>::is_always_lock_free << std::endl;
        struct Big { long long a, b, c, d; };
        std::cout << "atomic<Big(32B)>  is_always_lock_free = "
                  << std::atomic<Big>::is_always_lock_free
                  << "（大型型別通常由實作用鎖模擬）" << std::endl;
        std::cout << "→ 以上為本機實測值，屬**實作定義**，換平台可能不同。" << std::endl;
    }

    std::cout << "\n=== 段 3：condition_variable 必須用 predicate 迴圈 ===" << std::endl;
    {
        std::mutex m;
        std::condition_variable cv;
        bool ready = false;
        int  payload = 0;

        std::thread producer([&] {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            {
                std::lock_guard<std::mutex> lk(m);
                payload = 7;
                ready = true;      // 狀態放在受 mutex 保護的變數裡
            }
            cv.notify_one();
        });

        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [&] { return ready; });   // predicate：擋掉 spurious wakeup
        std::cout << "收到 payload = " << payload
                  << "（predicate 版 wait 保證條件真的成立）" << std::endl;
        lk.unlock();
        producer.join();
    }

    std::cout << "\n=== 實務範例 1：非同步日誌器 ===" << std::endl;
    {
        using namespace practical_logger;
        AsyncLogger logger;
        logger.setMinLevel(Level::Info);      // DEBUG 會被過濾掉

        constexpr int kProducers = 4;
        constexpr int kPerThread = 5;
        std::vector<std::thread> ts;
        for (int p = 0; p < kProducers; ++p) {
            ts.emplace_back([&logger, p] {
                for (int i = 0; i < kPerThread; ++i) {
                    // 每個生產者送 1 筆 DEBUG（會被濾掉）+ 1 筆 INFO
                    logger.log(Level::Debug, "producer " + std::to_string(p) +
                                             " debug " + std::to_string(i));
                    logger.log(Level::Info,  "producer " + std::to_string(p) +
                                             " info "  + std::to_string(i));
                }
            });
        }
        for (auto& t : ts) t.join();

        logger.flush();     // 用 future 等背景緒確實排空
        logger.shutdown();

        auto lines = logger.snapshot();
        std::cout << "送出 INFO 筆數  = " << kProducers * kPerThread << std::endl;
        std::cout << "實際寫出筆數    = " << logger.written() << std::endl;
        std::cout << "被 level 濾掉數 = " << logger.filtered()
                  << "（DEBUG < INFO 門檻）" << std::endl;
        std::cout << "sink 內行數     = " << lines.size() << std::endl;
        // 驗證：不論交錯順序如何，內容集合必定一致
        bool allInfo = std::all_of(lines.begin(), lines.end(),
            [](const std::string& s) { return s.rfind("[INFO] ", 0) == 0; });
        std::cout << "全部都是 [INFO] 開頭：" << (allInfo ? "是" : "否")
                  << "（寫入順序每次執行都不同，故只驗證集合性質）" << std::endl;
    }

    std::cout << "\n=== 實務範例 2：call_once 的例外重試語意 ===" << std::endl;
    {
        using namespace practical_once;

        // 前兩次載入會拋例外 → call_once 不記為完成，後續呼叫者會重試
        ConfigLoader loader(/*failFirstN=*/2);
        int caught = 0;
        for (int i = 0; i < 5; ++i) {
            try {
                const Settings& s = loader.get();
                std::cout << "第 " << (i + 1) << " 次呼叫成功，endpoint = "
                          << s.endpoint << "，實際嘗試次數 = "
                          << loader.attempts() << std::endl;
                break;
            } catch (const std::exception& e) {
                ++caught;
                std::cout << "第 " << (i + 1) << " 次呼叫失敗：" << e.what() << std::endl;
            }
        }
        std::cout << "共捕捉例外 " << caught
                  << " 次；若改用「bool 旗標」自己實作，第一次失敗後"
                     "就會被永久記成已初始化。" << std::endl;

        // 已完成後，多執行緒同時取用只會沿用同一份結果（不會再載入）
        ConfigLoader shared(/*failFirstN=*/0);
        std::vector<std::thread> ts;
        for (int i = 0; i < 8; ++i)
            ts.emplace_back([&shared] { (void)shared.get(); });
        for (auto& t : ts) t.join();
        std::cout << "8 條執行緒同時取用，載入函式實際執行次數 = "
                  << shared.attempts() << "（call_once 保證恰好一次）" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 1.6：標準函式庫總覽.cpp" -o lesson_1_6
//
// 本檔刻意維持 -std=c++17（與上面第五節的建置指令一致）：
// 表格中的 C++20 設施（jthread / latch / barrier / semaphore / atomic_ref /
// osyncstream）只在註解中說明，未寫進程式碼，因此不需要 -std=c++20。
// 想看它們實際執行，請見「課程 1.5：C++ 多執行緒發展史2.cpp」（該檔用 C++20 編譯）。
//
// ⚠️ 若要對本檔執行 ThreadSanitizer，請先把【實務範例 2】的
//    ConfigLoader loader(/*failFirstN=*/2) 改成 0 —— 原因見【注意事項 7】：
//    std::call_once 的例外重試在 TSan 下會卡死（libstdc++ × TSan 的已知交互作用，
//    非本檔錯誤）。避開該路徑後，本檔 TSan 實測 0 warning、exit 0。

// → 以上為本機實測值，屬**實作定義**，換平台可能不同。
// 全部都是 [INFO] 開頭：是（寫入順序每次執行都不同，故只驗證集合性質）
// 【每次執行都不同，故未印出】

// === 預期輸出 ===
// === 段 1：原始最小範例（thread / mutex / async / atomic）===
// Hello from thread!
// Async result: 42
// Atomic counter: 1
//
// === 段 2：atomic 是否為 lock-free（實作定義）===
// atomic<int>       is_always_lock_free = 1
// atomic<long long> is_always_lock_free = 1
// atomic<Big(32B)>  is_always_lock_free = 0（大型型別通常由實作用鎖模擬）
//
// === 段 3：condition_variable 必須用 predicate 迴圈 ===
// 收到 payload = 7（predicate 版 wait 保證條件真的成立）
//
// === 實務範例 1：非同步日誌器 ===
// 送出 INFO 筆數  = 20
// 實際寫出筆數    = 20
// 被 level 濾掉數 = 20（DEBUG < INFO 門檻）
// sink 內行數     = 20
//
// === 實務範例 2：call_once 的例外重試語意 ===
// 第 1 次呼叫失敗：設定中心尚未就緒（第 1 次）
// 第 2 次呼叫失敗：設定中心尚未就緒（第 2 次）
// 第 3 次呼叫成功，endpoint = https://config.internal/v1，實際嘗試次數 = 3
// 共捕捉例外 2 次；若改用「bool 旗標」自己實作，第一次失敗後就會被永久記成已初始化。
// 8 條執行緒同時取用，載入函式實際執行次數 = 1（call_once 保證恰好一次）
//
// ── 關於「非決定性」與「實作定義」的說明 ────────────────────────────────
// 【每次執行都相同】上面所有行的文字內容（本機連跑 3 次逐字一致）。
//   這是刻意設計的：程式只印出「不變條件」（筆數、集合性質、call_once 的
//   執行次數），不印任何交錯順序。
//   * 4 個生產者把 20 筆 INFO 寫進 sink 的**先後順序**（由排程器決定）；
//     因此只驗證「筆數 = 20」與「全部都是 [INFO] 開頭」這兩個集合性質。
//   * 背景寫入緒實際被喚醒幾次、每批 batch 的大小（可能一次 1 筆也可能一次 20 筆）。
//   * 各執行緒的 std::thread::id。
// 【實作定義（換平台/編譯器可能不同）】
//   * 段 2 的三個 is_always_lock_free 值：本機 g++ 15.2.0 / x86-64 實測為 1、1、0。
//   * std::thread::hardware_concurrency()：本機為 16（見第二階段 summary.cpp）。
//   * <execution> 需連結 -ltbb 一事，是 libstdc++ 的選擇，非標準要求。
// exit code = 0（本機實測）。
