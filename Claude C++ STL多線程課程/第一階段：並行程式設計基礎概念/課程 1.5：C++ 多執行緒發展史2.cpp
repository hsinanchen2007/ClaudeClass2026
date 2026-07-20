/*
# 第一階段：並行程式設計基礎概念

## 課程 1.5：C++ 多執行緒發展史

---

### 引言

C++ 直到 2011 年才在語言標準中正式支援多執行緒。在此之前，程式設計師只能依賴平台特定的 API。了解這段歷史，能幫助你理解為什麼現代 C++ 的多執行緒設計是這樣的。

---

### 一、C++11 之前的黑暗時代

#### 平台特定的解決方案

```
┌─────────────────────────────────────────────────────────────┐
│                C++11 之前的多執行緒                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Windows                    POSIX (Linux/macOS/Unix)        │
│  ─────────────────         ─────────────────────────        │
│  CreateThread()            pthread_create()                 │
│  WaitForSingleObject()     pthread_join()                   │
│  EnterCriticalSection()    pthread_mutex_lock()             │
│  LeaveCriticalSection()    pthread_mutex_unlock()           │
│                                                             │
│  完全不同的 API，程式碼無法跨平台！                           │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

#### POSIX Threads 範例（舊式寫法）

```cpp
// C++11 之前的 POSIX 多執行緒寫法
#include <pthread.h>
#include <stdio.h>

void* threadFunction(void* arg) {
    int* value = (int*)arg;
    printf("執行緒收到值: %d\n", *value);
    return NULL;
}

int main() {
    pthread_t thread;
    int value = 42;
    
    // 建立執行緒
    pthread_create(&thread, NULL, threadFunction, &value);
    
    // 等待執行緒結束
    pthread_join(thread, NULL);
    
    return 0;
}
```

**問題**：
- 需要手動管理記憶體與資源
- 型別不安全（使用 `void*`）
- 無法跨平台
- 容易出錯

---

### 二、C++11：標準多執行緒的誕生

C++11 引入了完整的多執行緒支援，這是 C++ 歷史上最重要的更新之一。

#### C++11 新增的標頭檔

```cpp
#include <thread>              // std::thread
#include <mutex>               // std::mutex, std::lock_guard
#include <condition_variable>  // std::condition_variable
#include <future>              // std::future, std::promise, std::async
#include <atomic>              // std::atomic
```

#### 現代 C++ 寫法對比

```cpp
// C++11 的現代寫法
#include <iostream>
#include <thread>

void threadFunction(int value) {
    std::cout << "執行緒收到值: " << value << std::endl;
}

int main() {
    std::thread t(threadFunction, 42);  // 型別安全！
    t.join();
    return 0;
}
```

**優點**：
- 型別安全
- 跨平台
- 支援 Lambda
- RAII 風格的資源管理

---

### 三、C++ 標準演進時間線

```
┌────────────────────────────────────────────────────────────────┐
│                    C++ 多執行緒演進史                           │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  C++11 (2011) ─── 奠基                                         │
│  │  • std::thread                                              │
│  │  • std::mutex, std::lock_guard                              │
│  │  • std::condition_variable                                  │
│  │  • std::future, std::promise, std::async                    │
│  │  • std::atomic                                              │
│  │  • 記憶體模型 (Memory Model)                                 │
│  │                                                             │
│  C++14 (2014) ─── 小幅改進                                     │
│  │  • std::shared_timed_mutex（讀寫鎖）                         │
│  │  • std::shared_lock                                         │
│  │                                                             │
│  C++17 (2017) ─── 重要擴展                                     │
│  │  • std::shared_mutex                                        │
│  │  • std::scoped_lock（多重鎖）                                │
│  │  • 平行演算法（Parallel Algorithms）                         │
│  │                                                             │
│  C++20 (2020) ─── 大幅增強                                     │
│  │  • std::jthread（自動 join）                                 │
│  │  • std::counting_semaphore                                  │
│  │  • std::latch, std::barrier                                 │
│  │  • std::stop_token（協作式取消）                             │
│  │  • 協程（Coroutines）                                       │
│  │                                                             │
│  C++23 (2023) ─── 持續完善                                     │
│     • 更多原子操作改進                                          │
│     • std::expected（錯誤處理）                                 │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

---

### 四、各版本關鍵功能速覽

#### C++11：基礎建設

```cpp
// 執行緒
std::thread t(func);

// 互斥鎖
std::mutex mtx;
std::lock_guard<std::mutex> lock(mtx);

// 非同步
auto future = std::async(std::launch::async, func);
```

#### C++17：便利性提升

```cpp
// 同時鎖定多個互斥鎖（避免死結）
std::scoped_lock lock(mutex1, mutex2, mutex3);

// 平行演算法
std::sort(std::execution::par, vec.begin(), vec.end());
```

#### C++20：現代化完善

```cpp
// 自動 join 的執行緒
std::jthread t(func);  // 解構時自動 join

// 信號量
std::counting_semaphore<10> sem(5);

// 協作式取消
std::jthread t([](std::stop_token token) {
    while (!token.stop_requested()) {
        // 工作...
    }
});
t.request_stop();
```

---

### 五、為什麼要了解這段歷史？

```
┌─────────────────────────────────────────────────────────────┐
│                 了解歷史的實際價值                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. 閱讀舊程式碼                                             │
│     許多專案仍使用 POSIX 或 Windows API                      │
│                                                             │
│  2. 理解設計決策                                             │
│     知道為什麼 C++ 標準庫是這樣設計的                        │
│                                                             │
│  3. 選擇正確工具                                             │
│     根據編譯器支援程度選擇適當的功能                         │
│                                                             │
│  4. 漸進式升級                                               │
│     了解如何將舊程式碼遷移到現代 C++                         │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 六、編譯器支援參考

| 功能 | GCC | Clang | MSVC |
|------|-----|-------|------|
| C++11 多執行緒 | 4.8+ | 3.3+ | 2012+ |
| C++17 平行演算法 | 9+ | 10+ | 2017+ |
| C++20 jthread | 10+ | 12+ | 2019+ |
| C++20 協程 | 10+ | 12+ | 2019+ |

---

### 七、本課重點回顧

1. C++11 之前，多執行緒只能依賴平台特定 API（POSIX、Windows）
2. C++11 首次將多執行緒納入語言標準，提供跨平台支援
3. C++14/17/20 持續增強，加入讀寫鎖、平行演算法、協程等功能
4. 現代 C++ 的多執行緒設計強調型別安全與 RAII
5. 本課程將涵蓋 C++11 到 C++20 的所有重要功能

---

### 下一課預告

在 **課程 1.6：標準函式庫總覽** 中，我們將：
- 完整介紹 C++ 多執行緒相關的標頭檔
- 預覽各個重要類別與函式
- 建立整個課程的學習地圖

---

準備好繼續嗎？
*/

// =============================================================================
//  課程 1.5：C++ 多執行緒發展史2.cpp  —  從 C++11 記憶體模型到 C++20 協作式取消
// =============================================================================
//
// 【主題資訊 Information】
//   本檔以「可實際執行」的方式，把上面時間線裡的每一個世代跑一遍。
//   標頭檔與標準版本對照（本檔全部以 g++ -std=c++NN -pedantic-errors 逐版實測）：
//
//   ┌──────────────────────────────────┬────────┬──────────────────────┐
//   │ 設施                             │ 標準   │ 標頭檔               │
//   ├──────────────────────────────────┼────────┼──────────────────────┤
//   │ 記憶體模型 (happens-before)      │ C++11  │ （語言核心，非標頭） │
//   │ std::thread / this_thread        │ C++11  │ <thread>             │
//   │ std::mutex / lock_guard          │ C++11  │ <mutex>              │
//   │ std::unique_lock / call_once     │ C++11  │ <mutex>              │
//   │ std::condition_variable          │ C++11  │ <condition_variable> │
//   │ std::future/promise/async        │ C++11  │ <future>             │
//   │ std::atomic<T> / atomic_flag     │ C++11  │ <atomic>             │
//   │ thread_local                     │ C++11  │ （語言關鍵字）       │
//   ├──────────────────────────────────┼────────┼──────────────────────┤
//   │ std::shared_timed_mutex          │ C++14  │ <shared_mutex>       │
//   │ std::shared_lock                 │ C++14  │ <shared_mutex>       │
//   ├──────────────────────────────────┼────────┼──────────────────────┤
//   │ std::shared_mutex                │ C++17  │ <shared_mutex>       │
//   │ std::scoped_lock                 │ C++17  │ <mutex>              │
//   │ 平行演算法 std::execution::par   │ C++17  │ <execution>          │
//   │ hardware_destructive_interfer... │ C++17  │ <new>                │
//   ├──────────────────────────────────┼────────┼──────────────────────┤
//   │ std::jthread                     │ C++20  │ <thread>             │
//   │ std::stop_token / stop_source    │ C++20  │ <stop_token>         │
//   │ std::latch                       │ C++20  │ <latch>              │
//   │ std::barrier                     │ C++20  │ <barrier>            │
//   │ std::counting_semaphore          │ C++20  │ <semaphore>          │
//   │ std::atomic_ref                  │ C++20  │ <atomic>             │
//   │ atomic::wait/notify_one          │ C++20  │ <atomic>             │
//   │ std::osyncstream                 │ C++20  │ <syncstream>         │
//   │ 協程 co_await/co_yield           │ C++20  │ <coroutine>          │
//   └──────────────────────────────────┴────────┴──────────────────────┘
//
// 【詳細解釋 Explanation】
//
// 【1. C++11 真正的分水嶺：記憶體模型，不是 std::thread】
// 上面的時間線把 std::thread 排在 C++11 的第一項，但那其實是「使用者看得到的
// 那一層」。C++11 真正的地基是 **記憶體模型**（memory model）：
//   * 首次定義「兩條執行緒同時存取同一物件」這件事的語意。
//   * 定義 **data race**：兩個執行緒未同步地存取同一記憶體位置，且至少一方是
//     寫入 → **未定義行為（UB）**。
//   * 定義 **happens-before** 關係，以及 atomic 的 memory_order。
// 沒有這一層，`std::thread` 只是個包裝 pthread 的類別，編譯器仍可以合法地
// 做出破壞多執行緒正確性的最佳化（見 課程 1.5-1 的【詳細解釋 1】）。
//
// 這裡要立刻澄清一組最常被混用的名詞：
//   * **data race**：未同步的並行存取且至少一方是寫 → 這是 UB，由標準定義。
//   * **race condition**：程式結果取決於執行時序 → 這是**邏輯 bug**，
//     不一定是 UB。你可以寫出「完全沒有 data race，但仍有 race condition」的
//     程式（例如兩個執行緒各自用 atomic 正確地做 check-then-act，
//     檢查與動作之間仍可能被插隊）。
//   兩者不可混為一談：消滅 data race 是「讓程式有定義」，
//   消滅 race condition 是「讓程式邏輯正確」。
//
// 【2. C++14：一個「半成品」讀寫鎖 —— shared_timed_mutex】
// C++14 只加了兩樣東西：`std::shared_timed_mutex` 與 `std::shared_lock`。
// 注意名字裡的 **timed**：它一定帶有 try_lock_for / try_lock_until 這組
// 逾時介面。委員會當時對「不帶逾時的純讀寫鎖」還沒有共識，於是先把
// 帶逾時的版本標準化。
// 結果就是那個經典陷阱：**C++14 有讀寫鎖，但沒有 std::shared_mutex**。
// `std::shared_mutex` 要等到 C++17。這是本課最常被考的版本細節之一。
// （為什麼要分兩個型別？逾時介面需要額外的狀態與可能的系統呼叫路徑；
//   不需要逾時的使用者不該為此付出代價 —— 這是 C++「不用就不付錢」原則。）
//
// 【3. C++17：把「正確用法」變成預設 —— scoped_lock 與平行演算法】
// C++11 想同時鎖住兩個 mutex 而不死結，得寫三行：
//     std::lock(m1, m2);                                   // 一次鎖住、內建避免死結
//     std::lock_guard<std::mutex> g1(m1, std::adopt_lock); // 再各自接管
//     std::lock_guard<std::mutex> g2(m2, std::adopt_lock);
// C++17 的 `std::scoped_lock lk(m1, m2);` 一行等價，而且因為有 CTAD，
// 連樣板參數都不用寫。它對多個 mutex 使用與 std::lock 相同的避免死結演算法
// （反覆 try_lock + 退讓），所以「按固定順序上鎖」的紀律可以交給它。
// 注意：`scoped_lock` 鎖**一個** mutex 時等價於 lock_guard；鎖**零個**時合法且什麼都不做。
//
// 平行演算法（`std::sort(std::execution::par, ...)`）也是 C++17。
// ⚠️ 實作細節：在 libstdc++（GCC）上，`<execution>` 的平行後端是 Intel TBB，
//    **編譯期只要 -std=c++17，但連結期必須加 -ltbb**，否則會是一長串
//    undefined reference to `tbb::detail::...`（本機實測確認）。
//    這是「標準版本正確、但連結失敗」的典型案例，本檔因此不示範平行演算法，
//    以免整份教材需要額外相依。
//
// 【4. C++20：從「你要記得收尾」到「編譯器幫你收尾」】
// C++20 的並行新增可以歸成兩條主線：
//   (a) **生命週期與取消**：jthread + stop_token/stop_source/stop_callback。
//       std::thread 的解構子在 joinable 時會呼叫 std::terminate（見第二階段
//       summary.cpp）；jthread 的解構子改成「request_stop() 然後 join()」，
//       把最常見的錯誤直接消滅在型別設計裡。
//       而 stop_token 是**協作式**取消：它只是設一個旗標並喚醒等待者，
//       不會強行中止執行緒 —— C++ 從未提供「強制 kill 執行緒」的機制，
//       因為那必然破壞 RAII 與不變條件。
//   (b) **同步原語補完**：latch（一次性倒數）、barrier（可重複使用的會合點）、
//       counting_semaphore / binary_semaphore（計數許可）。
//       這些在 C++11 都得用 mutex + condition_variable 手寫，
//       本檔【日常實務範例 1】會把兩種寫法並排跑給你看。
//       它們底下多半直接走 atomic::wait/notify（Linux 上是 futex），
//       比「mutex + cv」少一層互斥鎖，競爭低時明顯更便宜。
//
// 【5. C++23 與其後：務實的說法】
// 必須誠實：**C++23 在並行方面沒有 C++11/17/20 那種量級的新增**。
// 它主要是修補與周邊改善（例如 <stdatomic.h> 的 C 相容標頭正式納入）。
// 上面時間線提到的 `std::expected` 確實是 C++23，但它是**錯誤處理**設施，
// 不是並行設施，只是常被搭配使用 —— 教材裡不該把它算成「多執行緒功能」。
// 真正的下一次大改版是 **std::execution（senders/receivers，P2300）**，
// 目標是 C++26 的非同步執行框架。時間線標題寫「C++23 持續完善」是恰當的，
// 但別讓學生以為 C++23 帶來了新的同步原語。
//
// 【概念補充 Concept Deep Dive】
// (A) 為什麼 std::thread 的解構子選擇 terminate？
//     委員會面對兩難：預設 join 會讓解構點意外阻塞（而且會掩蓋忘記同步的
//     bug）；預設 detach 會讓執行緒在捕獲的物件死後繼續存取 → 隱蔽 UB。
//     兩害相權，選擇「強迫使用者明確表態」，違反就 terminate。
//     C++20 的 jthread 之所以能安全地選 join，是因為它**同時**引入了
//     stop_token —— 有了協作式取消，「解構時等它結束」才不會無限期阻塞。
//     這兩個設施是**成套設計**的，不是兩個獨立的新功能。
// (B) latch vs barrier 的差別（面試常混）
//     latch 是**一次性**的倒數器：count_down() 到 0 之後永遠開啟，不能重置。
//     barrier 是**可重複使用**的會合點：所有參與者到齊後執行一次 completion
//     function，然後自動進入下一個世代（generation）。
//     迴圈式的分階段計算用 barrier；「等所有初始化完成再開始」用 latch。
// (C) 為什麼 C++ 沒有 thread.kill()？
//     強制終止會讓目標執行緒在任意指令邊界停止：持有的 mutex 不會解鎖、
//     堆疊上的 RAII 解構子不會執行、half-updated 的資料結構就那樣留著。
//     pthread_cancel 存在但只在 cancellation point 生效且極難用對。
//     C++ 的答案一律是協作式：C++11 自己用 atomic<bool> 旗標，
//     C++20 標準化為 stop_token。
//
// 【注意事項 Pay Attention】
// 1. **C++14 沒有 std::shared_mutex**，只有 std::shared_timed_mutex。
//    但 **std::shared_lock 是 C++14**（它可以搭配 shared_timed_mutex 使用）。
//    這組不對稱是版本題的最愛。
// 2. 上面時間線把「協程（Coroutines）」列在 C++20 是對的，但它是**語言特性**
//    （co_await/co_yield/co_return + <coroutine> 的低階介面），
//    標準庫在 C++20 **沒有**提供現成的 task/generator 型別
//    （std::generator 要到 C++23）。實務上多半得自己寫或用第三方庫。
// 3. `std::jthread` 的解構是「request_stop() 然後 join()」。若你的迴圈
//    **沒有檢查 stop_token**，解構仍會無限期等下去 —— jthread 不是魔法。
// 4. `std::counting_semaphore<N>` 的 N 是**編譯期的最大計數上限**，
//    不是初始值；初始值由建構子參數給。寫成 `counting_semaphore<> s{3}` 時
//    預設上限是 PTRDIFF_MAX。
// 5. 編譯器支援表（原文第六節）是**歷史參考**，不等於「該版本完全符合標準」。
//    例如 GCC 對 <execution> 需要額外連結 TBB。要確認就實際編譯，
//    而且要用 **-pedantic-errors** —— 只用 -fsyntax-only 或不加 pedantic，
//    GCC 會把新標準的設施當作擴充放行，讓你誤以為舊標準也支援。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】C++ 多執行緒發展史 / 各標準版本的分界
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::shared_mutex 是哪個標準加入的？
//     答：**C++17**。C++14 加入的是 `std::shared_timed_mutex`（帶逾時介面）
//         與 `std::shared_lock`。也就是「C++14 就有讀寫鎖，但那個型別叫
//         shared_timed_mutex」。不需要逾時就用 C++17 的 shared_mutex，
//         省下逾時介面的狀態成本。
//     追問：那 std::shared_lock 是哪一版？
//         → C++14，跟 shared_timed_mutex 同批進來。這組不對稱常被考。
//
// 🔥 Q2. C++20 的 jthread 相較 std::thread 解決了什麼？只是自動 join 嗎？
//     答：不只。它解決的是**一整套生命週期問題**：解構子是
//         「request_stop() 然後 join()」，並內建 stop_token 讓執行緒
//         能協作式地被要求停止。只有自動 join 而沒有取消機制的話，
//         解構會在無窮迴圈的工作執行緒上永遠阻塞 —— 兩者是成套設計。
//     追問：那 jthread 可以強制中止一條不理會 stop_token 的迴圈嗎？
//         → 不行。C++ 從無強制 kill；不檢查 token 的迴圈會讓解構一直等下去。
//
// 🔥 Q3. C++11 對多執行緒最重要的貢獻是什麼？
//     答：記憶體模型。它首次定義 data race、happens-before 與 memory_order，
//         使「並行程式的正確性」成為標準可描述的性質。std::thread 只是
//         建立在這個地基上的 API；沒有記憶體模型，編譯器最佳化仍可合法地
//         破壞多執行緒程式。
//     追問：data race 和 race condition 差在哪？
//         → data race 是未同步的並行存取且至少一方是寫，屬 UB；
//           race condition 是結果取決於時序的邏輯問題，可以完全沒有 data race。
//
// ⚠️ 陷阱 1. 「C++17 有平行演算法，所以 -std=c++17 就能直接用 std::execution::par」——對嗎？
//     答：標準版本判斷是對的（`<execution>` 確實是 C++17），但在 GCC/libstdc++ 上
//         **連結期需要 -ltbb**，否則會出現一長串 `undefined reference to
//         tbb::detail::...`（本機實測）。程式碼與標準都沒錯，錯在建置設定。
//     為什麼會錯：多數人把「標準支援」與「開箱即用」畫上等號。但平行後端是
//         實作自由，libstdc++ 選擇委派給 Intel TBB，這屬於**實作定義**的範疇。
//
// ⚠️ 陷阱 2. 用 -fsyntax-only 驗證「某設施屬於哪個標準」，可靠嗎？
//     答：不可靠。GCC 預設會把不少較新標準的設施當作**擴充**接受，
//         不加 -pedantic-errors 時，用 -std=c++11 也可能「編得過」C++17 的東西，
//         讓你誤判版本。要驗證版本，必須
//         `g++ -std=c++NN -pedantic-errors` 實際編譯（本檔所有版本標註皆如此驗證）。
//     為什麼會錯：把「編譯器接受」等同於「標準要求」。編譯器擴充、
//         標準庫標頭的間接引入（某個標頭順手包了另一個）都會造成假陽性。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode】從缺。
//   本課主題是「標準演進的歷史與版本分界」，LeetCode 並行題
//   （1114/1115/1116/1117/1195）考的是同步邏輯的實作，與版本歷史對不上。
//   硬湊一題只會稀釋本課重點，故不加。相關題目請見第二階段 summary.cpp。
// =============================================================================

// C++11 的現代寫法
#include <iostream>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <atomic>
#include <semaphore>
#include <latch>
#include <stop_token>
#include <vector>
#include <string>
#include <chrono>

void threadFunction(int value) {
    std::cout << "執行緒收到值: " << value << std::endl;
}

// -----------------------------------------------------------------------------
// 段 2：C++11 → C++17，「同時鎖住兩個 mutex」的寫法演進
// -----------------------------------------------------------------------------
namespace evolution_lock {

struct Account {
    std::mutex m;
    int balance;
    explicit Account(int b) : balance(b) {}
};

// C++11 寫法：std::lock 一次鎖住（內建避免死結），再各自 adopt
void transfer_cpp11(Account& from, Account& to, int amount) {
    std::lock(from.m, to.m);
    std::lock_guard<std::mutex> g1(from.m, std::adopt_lock);
    std::lock_guard<std::mutex> g2(to.m,   std::adopt_lock);
    from.balance -= amount;
    to.balance   += amount;
}

// C++17 寫法：scoped_lock 一行等價（CTAD 讓樣板參數也免寫）
void transfer_cpp17(Account& from, Account& to, int amount) {
    std::scoped_lock lk(from.m, to.m);
    from.balance -= amount;
    to.balance   += amount;
}

} // namespace evolution_lock

// -----------------------------------------------------------------------------
// 段 3：C++14 shared_timed_mutex vs C++17 shared_mutex
//        兩者用法幾乎相同，差別在「是否帶逾時介面」。
// -----------------------------------------------------------------------------
namespace evolution_rwlock {

// C++14 版本：只有 shared_timed_mutex 可用
class ConfigCpp14 {
    mutable std::shared_timed_mutex m_;
    std::string value_ = "initial";
public:
    std::string read() const {
        std::shared_lock<std::shared_timed_mutex> lk(m_);   // 多讀者可並行
        return value_;
    }
    void write(std::string v) {
        std::unique_lock<std::shared_timed_mutex> lk(m_);   // 寫者獨佔
        value_ = std::move(v);
    }
    // shared_timed_mutex 獨有：帶逾時的嘗試
    bool try_read_for(std::chrono::milliseconds d, std::string& out) const {
        std::shared_lock<std::shared_timed_mutex> lk(m_, std::defer_lock);
        if (!lk.try_lock_for(d)) return false;
        out = value_;
        return true;
    }
};

// C++17 版本：改用 shared_mutex，不需要逾時就不必付逾時的成本
class ConfigCpp17 {
    mutable std::shared_mutex m_;
    std::string value_ = "initial";
public:
    std::string read() const {
        std::shared_lock lk(m_);            // CTAD，樣板參數免寫
        return value_;
    }
    void write(std::string v) {
        std::unique_lock lk(m_);
        value_ = std::move(v);
    }
};

} // namespace evolution_rwlock

// -----------------------------------------------------------------------------
// 【日常實務範例 1】連線池（Connection Pool）：限制同時開啟的連線數
//   情境：資料庫／HTTP 後端通常有連線數上限，超出就會被拒絕或排隊。
//         我們要讓「最多同時 N 條連線」，其餘的呼叫端阻塞等待。
//   為什麼放在本課：這正是 C++20 counting_semaphore 要解決的問題。
//         同一個需求在 C++11 得用 mutex + condition_variable 手寫，
//         兩種寫法並排看，就能理解 C++20 補了什麼洞。
// -----------------------------------------------------------------------------
namespace practical_pool {

constexpr int kMaxConns = 3;      // 連線上限
constexpr int kWorkers  = 8;      // 同時想要連線的工作執行緒數

// 監測用：記錄「同時在池內的最大數量」與「是否曾超出上限」
struct PoolStats {
    std::atomic<int>  current{0};
    std::atomic<int>  peak{0};
    std::atomic<bool> violated{false};

    void enter(int limit) {
        int now = current.fetch_add(1) + 1;
        if (now > limit) violated.store(true);
        // 以 CAS 迴圈更新峰值（peak = max(peak, now)）
        int old = peak.load();
        while (now > old && !peak.compare_exchange_weak(old, now)) { /* 重試 */ }
    }
    void leave() { current.fetch_sub(1); }
};

// ---- C++11 寫法：mutex + condition_variable 手寫號誌 ----
class PoolCpp11 {
    std::mutex m_;
    std::condition_variable cv_;
    int available_;
public:
    explicit PoolCpp11(int n) : available_(n) {}

    void acquire() {
        std::unique_lock<std::mutex> lk(m_);
        // 一定要用 predicate 迴圈：可能有 spurious wakeup，
        // 也可能被喚醒後名額又被別人搶走。
        cv_.wait(lk, [this] { return available_ > 0; });
        --available_;
    }
    void release() {
        {
            std::lock_guard<std::mutex> lk(m_);
            ++available_;
        }
        cv_.notify_one();   // 在鎖外通知，避免被喚醒者立刻又卡在鎖上
    }
};

// ---- C++20 寫法：counting_semaphore 直接表達「N 個許可」----
// 樣板參數 kMaxConns 是「編譯期最大計數上限」，建構子參數才是初始值。
using PoolCpp20 = std::counting_semaphore<kMaxConns>;

std::string run_cpp11_pool() {
    PoolCpp11 pool(kMaxConns);
    PoolStats stats;
    std::vector<std::thread> ts;

    for (int i = 0; i < kWorkers; ++i) {
        ts.emplace_back([&pool, &stats] {
            pool.acquire();
            stats.enter(kMaxConns);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));  // 模擬查詢
            stats.leave();
            pool.release();
        });
    }
    for (auto& t : ts) t.join();

    return std::string("並行連線未超過上限：") +
           (stats.violated.load() ? "否（有違規！）" : "是") +
           "，上限 = " + std::to_string(kMaxConns);
}

std::string run_cpp20_pool() {
    PoolCpp20 sem(kMaxConns);      // 初始持有 kMaxConns 個許可
    PoolStats stats;
    {
        std::vector<std::jthread> ts;  // C++20：解構自動 join，不必手動迴圈
        for (int i = 0; i < kWorkers; ++i) {
            ts.emplace_back([&sem, &stats] {
                sem.acquire();                 // 取一個許可（無許可則阻塞）
                stats.enter(kMaxConns);
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                stats.leave();
                sem.release();                 // 歸還
            });
        }
    }   // ← 離開作用域：所有 jthread 解構 → 自動 join

    return std::string("並行連線未超過上限：") +
           (stats.violated.load() ? "否（有違規！）" : "是") +
           "，上限 = " + std::to_string(kMaxConns);
}

} // namespace practical_pool

// -----------------------------------------------------------------------------
// 【日常實務範例 2】背景監控取樣器（Metrics Sampler）：優雅關閉
//   情境：背景執行緒每隔一段時間取樣一次 CPU／佇列長度，程式結束時
//         必須能「請它停下來並等它收完尾」，不能硬殺（否則檔案沒 flush）。
//   為什麼放在本課：這是 C++11 手寫 atomic<bool> 停止旗標
//         → C++20 stop_token 標準化的最典型案例。
// -----------------------------------------------------------------------------
namespace practical_sampler {

// ---- C++11 寫法：自己維護 atomic<bool> 旗標 + 手動 join ----
class SamplerCpp11 {
    std::atomic<bool> stop_{false};
    std::atomic<int>  samples_{0};
    std::thread       th_;
public:
    void start() {
        th_ = std::thread([this] {
            while (!stop_.load(std::memory_order_relaxed)) {
                samples_.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
    }
    void stop() {
        stop_.store(true, std::memory_order_relaxed);
        if (th_.joinable()) th_.join();   // 忘了這行 → 解構時 std::terminate
    }
    int samples() const { return samples_.load(); }

    ~SamplerCpp11() {
        // 必須自己記得收尾，否則 joinable 的 thread 解構會 terminate
        if (th_.joinable()) { stop_.store(true); th_.join(); }
    }
};

// ---- C++20 寫法：jthread + stop_token，收尾寫在型別裡 ----
class SamplerCpp20 {
    std::atomic<int> samples_{0};
    std::jthread     th_;
public:
    void start() {
        th_ = std::jthread([this](std::stop_token st) {
            // 協作式取消：自己檢查 token，標準不會強行中止你
            while (!st.stop_requested()) {
                samples_.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
    }
    int samples() const { return samples_.load(); }
    // 不需要寫解構子：jthread 解構 = request_stop() + join()
};

} // namespace practical_sampler

int main() {
    std::cout << "=== 段 1：C++11 的現代寫法（對照 1.5-1 的 pthread）===" << std::endl;
    std::thread t(threadFunction, 42);  // 型別安全！
    t.join();

    std::cout << "\n=== 段 2：C++11 std::lock vs C++17 scoped_lock ===" << std::endl;
    {
        using namespace evolution_lock;
        Account a(1000), b(1000);
        transfer_cpp11(a, b, 300);
        std::cout << "C++11 std::lock + adopt_lock : a=" << a.balance
                  << " b=" << b.balance << std::endl;
        transfer_cpp17(b, a, 100);
        std::cout << "C++17 scoped_lock（一行等價） : a=" << a.balance
                  << " b=" << b.balance << std::endl;
    }

    std::cout << "\n=== 段 3：C++14 shared_timed_mutex vs C++17 shared_mutex ===" << std::endl;
    {
        using namespace evolution_rwlock;
        ConfigCpp14 c14;
        c14.write("v1.4-設定值");
        std::string out;
        bool ok = c14.try_read_for(std::chrono::milliseconds(50), out);
        std::cout << "C++14 shared_timed_mutex: read=" << c14.read()
                  << "，try_read_for 成功=" << (ok ? "是" : "否")
                  << "（逾時介面是 timed 版獨有）" << std::endl;

        ConfigCpp17 c17;
        c17.write("v1.7-設定值");
        std::cout << "C++17 shared_mutex      : read=" << c17.read()
                  << "（無逾時介面，較輕量）" << std::endl;
    }

    std::cout << "\n=== 段 4：C++20 latch —— 等所有工作執行緒就緒再一起開始 ===" << std::endl;
    {
        constexpr int kN = 4;
        std::latch ready(kN);          // 一次性倒數，歸零後永遠開啟
        std::atomic<int> done{0};
        {
            std::vector<std::jthread> ts;
            for (int i = 0; i < kN; ++i) {
                ts.emplace_back([&ready, &done] {
                    ready.count_down();    // 我就緒了
                    ready.wait();          // 等大家都就緒
                    done.fetch_add(1);
                });
            }
        }   // jthread 解構自動 join
        std::cout << "latch 會合完成，抵達的執行緒數 = " << done.load()
                  << "（latch 是一次性，barrier 才可重複使用）" << std::endl;
    }

    std::cout << "\n=== 實務範例 1：連線池（C++11 手寫號誌 vs C++20 semaphore）===" << std::endl;
    std::cout << "C++11 mutex+cv       : " << practical_pool::run_cpp11_pool() << std::endl;
    std::cout << "C++20 counting_sema. : " << practical_pool::run_cpp20_pool() << std::endl;

    std::cout << "\n=== 實務範例 2：背景監控取樣器（C++11 停止旗標 vs C++20 stop_token）===" << std::endl;
    {
        practical_sampler::SamplerCpp11 s11;
        s11.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        s11.stop();
        std::cout << "C++11：需自寫 atomic<bool> + 手動 join，取樣數 > 0 ： "
                  << (s11.samples() > 0 ? "是" : "否") << std::endl;

        int n20 = 0;
        {
            practical_sampler::SamplerCpp20 s20;
            s20.start();
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            n20 = s20.samples();
        }   // ← 離開作用域：jthread 解構自動 request_stop() + join()
        std::cout << "C++20：解構自動 request_stop()+join()，取樣數 > 0 ： "
                  << (n20 > 0 ? "是" : "否") << std::endl;
    }

    std::cout << "\n（取樣次數本身每次執行都不同，取決於排程與計時器精度，"
              << "故此處只驗證「有取到樣本」。）" << std::endl;
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra -pthread "課程 1.5：C++ 多執行緒發展史2.cpp" -o lesson_1_5b
//
// ⚠️ 本檔必須用 -std=c++20：段 4 用到 std::latch，實務範例用到 std::jthread、
//    std::stop_token 與 std::counting_semaphore，這些都是 C++20 才有的設施。
//    用 -std=c++17 編譯會失敗（這正是本課要教的版本分界）。

// （取樣次數本身每次執行都不同，取決於排程與計時器精度，故此處只驗證「有取到樣本」。）
// 但底下這些量**每次執行都不同**，所以刻意沒有印出來：

// === 預期輸出 ===
// === 段 1：C++11 的現代寫法（對照 1.5-1 的 pthread）===
// 執行緒收到值: 42
//
// === 段 2：C++11 std::lock vs C++17 scoped_lock ===
// C++11 std::lock + adopt_lock : a=700 b=1300
// C++17 scoped_lock（一行等價） : a=800 b=1200
//
// === 段 3：C++14 shared_timed_mutex vs C++17 shared_mutex ===
// C++14 shared_timed_mutex: read=v1.4-設定值，try_read_for 成功=是（逾時介面是 timed 版獨有）
// C++17 shared_mutex      : read=v1.7-設定值（無逾時介面，較輕量）
//
// === 段 4：C++20 latch —— 等所有工作執行緒就緒再一起開始 ===
// latch 會合完成，抵達的執行緒數 = 4（latch 是一次性，barrier 才可重複使用）
//
// === 實務範例 1：連線池（C++11 手寫號誌 vs C++20 semaphore）===
// C++11 mutex+cv       : 並行連線未超過上限：是，上限 = 3
// C++20 counting_sema. : 並行連線未超過上限：是，上限 = 3
//
// === 實務範例 2：背景監控取樣器（C++11 停止旗標 vs C++20 stop_token）===
// C++11：需自寫 atomic<bool> + 手動 join，取樣數 > 0 ： 是
// C++20：解構自動 request_stop()+join()，取樣數 > 0 ： 是
//
//
// ── 關於「非決定性」的說明 ────────────────────────────────────────────────
// 上面每一行的**文字內容**都是決定性的，因為本檔刻意只印出「不變條件」
// （餘額、抵達數、是否超過上限、是否取到樣本），而不印交錯順序。
//   * 8 條工作執行緒實際拿到連線的先後順序（由 OS 排程器決定）
//   * 取樣器在 30 ms 內取到的樣本數（取決於排程延遲與計時器精度，
//     sleep_for(5ms) 只保證「至少」睡 5 ms，不保證剛好）
//   * 各執行緒的 std::thread::id
// 段 4 的「抵達的執行緒數 = 4」之所以固定，是因為 latch.wait() 保證
// 所有 4 條都通過會合點後才會離開作用域並 join —— 這是同步保證，不是巧合。
// exit code = 0（本機實測，g++ 15.2.0 / 16 硬體執行緒）。
