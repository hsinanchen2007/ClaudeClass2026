/*
# 第二階段：std::thread 基礎

## 課程 2.5：joinable() 狀態檢查

---

### 引言

`joinable()` 是判斷 `std::thread` 物件狀態的關鍵方法。理解何時執行緒是 joinable，能幫助你避免程式崩潰。

---

### 一、什麼是 joinable？

一個 `std::thread` 物件是 **joinable** 代表：
- 它關聯著一個真正的執行緒
- 該執行緒尚未被 join 或 detach

```
┌─────────────────────────────────────────┐
│         joinable 的意義                 │
├─────────────────────────────────────────┤
│                                         │
│  joinable() == true                     │
│  → 有一個執行緒正在運行（或已結束但未回收）│
│  → 必須呼叫 join() 或 detach()          │
│                                         │
│  joinable() == false                    │
│  → 沒有關聯的執行緒                      │
│  → 不需要（也不能）呼叫 join/detach      │
│                                         │
└─────────────────────────────────────────┘
```

---

### 二、何時 joinable 為 false？

```cpp
#include <iostream>
#include <thread>

int main() {
    // 情況 1：預設建構
    std::thread t1;
    std::cout << "預設建構: " << t1.joinable() << std::endl;
    
    // 情況 2：已經 join
    std::thread t2([]() {});
    t2.join();
    std::cout << "join 後: " << t2.joinable() << std::endl;
    
    // 情況 3：已經 detach
    std::thread t3([]() {});
    t3.detach();
    std::cout << "detach 後: " << t3.joinable() << std::endl;
    
    // 情況 4：被 move 走
    std::thread t4([]() {});
    std::thread t5 = std::move(t4);
    std::cout << "move 後(原): " << t4.joinable() << std::endl;
    std::cout << "move 後(新): " << t5.joinable() << std::endl;
    t5.join();
    
    return 0;
}
```

輸出：
```
預設建構: 0
join 後: 0
detach 後: 0
move 後(原): 0
move 後(新): 1
```

---

### 三、狀態轉換圖

```
                    ┌─────────────────┐
                    │  預設建構        │
                    │  joinable=false │
                    └────────┬────────┘
                             │ 賦值帶有執行緒的物件
                             ▼
┌──────────────┐    ┌─────────────────┐    ┌──────────────┐
│   join()     │◄───│   執行中         │───►│   detach()   │
│              │    │  joinable=true  │    │              │
└──────┬───────┘    └────────┬────────┘    └──────┬───────┘
       │                     │                    │
       │                     │ std::move()        │
       ▼                     ▼                    ▼
┌─────────────────────────────────────────────────────────┐
│                    joinable=false                        │
│               （不再關聯任何執行緒）                       │
└─────────────────────────────────────────────────────────┘
```

---

### 四、安全的執行緒管理模式

```cpp
#include <iostream>
#include <thread>

class SafeThread {
    std::thread t;
    
public:
    template<typename Func, typename... Args>
    void start(Func&& f, Args&&... args) {
        if (t.joinable()) {
            t.join();  // 先結束舊的
        }
        t = std::thread(std::forward<Func>(f), std::forward<Args>(args)...);
    }
    
    void join() {
        if (t.joinable()) {
            t.join();
        }
    }
    
    ~SafeThread() {
        join();  // 解構時自動 join
    }
};

int main() {
    SafeThread st;
    
    st.start([]() {
        std::cout << "任務 1" << std::endl;
    });
    
    st.start([]() {  // 自動 join 前一個
        std::cout << "任務 2" << std::endl;
    });
    
    // 解構時自動 join
    return 0;
}
```

---

### 五、常見錯誤與修正

#### 錯誤：不檢查就 join

```cpp
// 錯誤
std::thread t;
t.join();  // 崩潰！t 不是 joinable

// 正確
std::thread t;
if (t.joinable()) {
    t.join();
}
```

#### 錯誤：條件式建立後忘記處理

```cpp
// 錯誤
std::thread t;
if (someCondition) {
    t = std::thread(func);
}
t.join();  // 若條件為 false，崩潰！

// 正確
std::thread t;
if (someCondition) {
    t = std::thread(func);
}
if (t.joinable()) {
    t.join();
}
```

---

### 六、joinable 檢查的時機

| 情境 | 是否需要檢查 |
|------|-------------|
| 剛建立的執行緒，立即 join | 不需要（一定是 joinable）|
| 可能被 move 走的執行緒 | 需要 |
| 條件式建立的執行緒 | 需要 |
| 迴圈中重複使用的執行緒變數 | 需要 |
| 類別解構函式中 | 需要 |

---

### 七、本課重點回顧

1. `joinable()` 檢查執行緒物件是否關聯著一個執行緒
2. 預設建構、join 後、detach 後、move 後都是 non-joinable
3. 對 non-joinable 呼叫 `join()` 或 `detach()` 會拋出例外
4. 在不確定狀態時，先用 `joinable()` 檢查
5. 類別解構函式中應該檢查並 join

---

### 下一課預告

在 **課程 2.6：執行緒識別與資訊** 中，我們將學習：
- `get_id()` 取得執行緒 ID
- `hardware_concurrency()` 查詢 CPU 核心數
- `std::this_thread` 命名空間

---

準備好繼續嗎？
*/

// =============================================================================
//  課程 2.5：joinable() 狀態檢查2.cpp  —  安全的執行緒管理模式(本課總整理)
// =============================================================================
//
// 【主題資訊 Information】
//   核心慣用法: if (t.joinable()) { t.join(); }
//   標準版本  : C++11;C++20 的 std::jthread 讓多數樣板碼變得不必要
//   標頭檔    : <thread>;完美轉發需要 <utility>
//   不變量    : join()/detach() 要求 joinable()==true;解構要求 false
//   本檔重點  : 把這個不變量封裝進 RAII 類別,讓使用者不可能寫錯
//
// 【詳細解釋 Explanation】
//
// 【1. SafeThread 解決的三個問題】
// 這個類別看起來簡單,但它同時修掉了 std::thread 最容易出錯的三件事:
//
//   (a) 忘記 join → 解構時 std::terminate()
//       解構子裡的 join() 讓「忘記」不可能發生。
//
//   (b) 重複 join → 丟 std::system_error
//       每次 join 前都檢查 joinable(),所以重複呼叫是安全的 no-op。
//
//   (c) 對仍 joinable 的 thread 做 move assignment → std::terminate()
//       這是最隱蔽的一個,底下【2】專門說明。
//
// 【2. start() 裡那個 if 不是禮貌,是必需】
//     void start(...) {
//         if (t.joinable()) {
//             t.join();          // ← 少了這三行,程式會直接中止
//         }
//         t = std::thread(...);  // ← move assignment
//     }
//
// 對一個「仍然 joinable」的 std::thread 做 move assignment,
// 標準規定呼叫 std::terminate()。本機實測驗證:
//     std::thread a(f); std::thread b(g);
//     a = std::move(b);           // → exit code 134 (SIGABRT)
//                                 //   "terminate called without an active exception"
// 原因和「忘記 join」一樣:a 原本持有的執行緒會失去唯一的 handle,
// 成為沒有人能回收的孤兒。標準不願默默幫你 join(可能阻塞很久)
// 或 detach(可能造成懸空存取),因此選擇當場中止。
//
// 所以 start() 第二次被呼叫時,若沒有先 join 掉前一條,程式就死了。
// 這正是本檔 main() 連續呼叫兩次 start() 卻能正常運作的原因。
//
// ⚠️ C++20 的 std::jthread 規則不同:對仍 joinable 的 jthread 做
//    move assignment,它會先 request_stop() 並 join(),然後正常存活。
//    本機實測 jthread 版本 exit code 為 0。
//    **std::thread 會死、std::jthread 會活 —— 這是兩套規則,別混記。**
//
// 【3. 完美轉發讓 SafeThread 能接受任何可呼叫物件】
//     template<typename Func, typename... Args>
//     void start(Func&& f, Args&&... args) {
//         t = std::thread(std::forward<Func>(f), std::forward<Args>(args)...);
//     }
// Func&& 與 Args&&... 在模板中是「轉發參考」(forwarding reference),
// 搭配 std::forward 可以保留呼叫端傳進來的值類別:
// 左值仍是左值(會被複製)、右值仍是右值(會被移動)。
// 少了 std::forward,所有東西都會退化成左值,move-only 的型別
// (如 std::unique_ptr)就傳不進去了。
//
// 【4. 這個類別還缺什麼(誠實評估)】
// SafeThread 是教學用的最小版本,離產品級還差幾件事:
//   * 它可以被複製嗎?—— 不應該。它持有獨佔的執行緒所有權,
//     必須把複製建構/賦值 = delete(本檔已補上,原始版本沒有)。
//   * 解構子裡的 join() 可能阻塞很久,而解構子是 noexcept 的 ——
//     若 join 期間發生例外會直接 terminate。實務上要確保執行緒能被
//     及時停止(用 atomic 旗標或 stop_token)。
//   * 沒有「請求停止」的機制。C++20 的 std::jthread 內建 stop_token,
//     這正是它比手寫守衛好用的地方。
//
// 結論:**若專案能用 C++20,直接用 std::jthread,不要自己寫這個類別。**
// 本檔的價值在於理解 jthread 幫你做了什麼。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼解構子要 join 而不是 detach
//   detach 看似「比較不會卡住」,但它會讓執行緒在物件被銷毀後繼續跑,
//   而那條執行緒很可能還在存取這個物件的成員 —— 懸空存取。
//   RAII 的核心承諾是「物件銷毀時,它管理的資源已經被乾淨地釋放」,
//   join 才符合這個承諾。
//
// (B) 為什麼 std::thread 只能移動不能複製
//   一條作業系統執行緒只能被 join 一次、回收一次。允許複製會讓
//   兩個物件都嘗試回收同一條執行緒。這與 std::unique_ptr 是同一個設計:
//   代表獨佔所有權的型別,一律 move-only。
//
// (C) 為什麼 vector<std::thread> 是合法的
//   標準容器只要求元素「可移動」,不要求「可複製」。
//   vector 擴容時會移動元素而非複製,所以 move-only 型別放得進去。
//   這也是 pool.emplace_back(...) 建立執行緒池的標準寫法能成立的原因。
//
// 【注意事項 Pay Attention】
// 1. 對仍 joinable 的 std::thread 做 move assignment 會 std::terminate();
//    賦值前務必先 join 或 detach 掉原本那條。
// 2. std::jthread 在同樣情況下會 request_stop()+join() 而正常存活,
//    規則與 std::thread 不同。
// 3. 持有執行緒的類別應該禁止複製(= delete),否則所有權會被複製出多份。
// 4. 解構子中的 join() 可能阻塞;要確保執行緒能被及時停止。
// 5. 「檢查 joinable 再操作」只在單一擁有者的前提下可靠;
//    多執行緒共用同一個 thread 物件是 TOCTOU 競態。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】安全的執行緒管理模式
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. SafeThread::start() 在賦值新執行緒前,為什麼一定要先
//        if (t.joinable()) t.join();?
//     答：因為對一個仍然 joinable 的 std::thread 做 move assignment,
//         標準規定會呼叫 std::terminate()。本機實測 exit code 134。
//         原因是原本持有的執行緒會失去唯一的 handle,成為無人回收的孤兒;
//         標準不願默默幫你 join(可能阻塞)或 detach(可能懸空),
//         所以當場中止。少了那個 if,第二次呼叫 start() 程式就死了。
//     追問：C++20 的 std::jthread 也是這樣嗎?
//         → 不是,這是常見的混淆點。jthread 的 move assignment 會先
//           request_stop() 再 join(),然後正常存活(實測 exit code 0)。
//           std::thread 會死、std::jthread 會活,是兩套不同的規則。
//
// 🔥 Q2. 為什麼 RAII 守衛的解構子選擇 join() 而不是 detach()?
//     答：detach 會讓執行緒在物件銷毀後繼續執行,而那條執行緒很可能
//         還在存取這個物件的成員 —— 懸空存取,未定義行為。
//         RAII 的核心承諾是「物件銷毀時,它管理的資源已被乾淨釋放」,
//         只有 join 符合這個承諾。
//     追問：那解構子裡 join 卡很久怎麼辦?
//         → 這是這個模式的真實代價。解決方式是讓執行緒能被及時停止:
//           自己用 atomic 旗標,或直接用 C++20 的 std::jthread ——
//           它內建 stop_token,解構時會先 request_stop() 再 join()。
//
// ⚠️ 陷阱. 「SafeThread 已經在解構子裡處理好 join 了,所以它是個
//         完整安全的類別,可以直接放進 vector 或傳來傳去。」哪裡錯了?
//     答：原始版本少了最關鍵的一件事:它沒有禁止複製。
//         編譯器會為它產生預設的複製建構子 —— 但 std::thread 本身
//         不可複製,所以那個隱式複製建構子其實是被定義成 deleted 的,
//         真的去複製會編譯失敗。問題在於這是「碰巧安全」:
//         一旦類別裡多了其他可複製的成員,語意就會變得混亂。
//         正確做法是明確寫出 = delete,讓「這個類別獨佔一條執行緒」
//         這個意圖出現在程式碼上,而不是靠成員的性質意外達成。
//     為什麼會錯：把「編譯不過」當成「設計正確」。
//         依賴成員的性質來間接獲得想要的語意,是脆弱的 ——
//         它沒有寫下意圖,任何一次重構都可能悄悄破壞它。
//         (本檔的版本已補上明確的 = delete。)
// ═══════════════════════════════════════════════════════════════════════════

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

// 保護 std::cout,避免多執行緒輸出互相切開
std::mutex g_ioMutex;

void say(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_ioMutex);
    std::cout << msg << std::endl;
}

// -----------------------------------------------------------------------------
// SafeThread:把 std::thread 的生命週期不變量封裝起來
// -----------------------------------------------------------------------------
class SafeThread {
    std::thread t;

public:
    SafeThread() = default;

    template<typename Func, typename... Args>
    void start(Func&& f, Args&&... args) {
        // ⚠️ 這個檢查不是禮貌而是必需:
        //    對仍 joinable 的 std::thread 做 move assignment 會 std::terminate()
        if (t.joinable()) {
            t.join();  // 先結束舊的
        }
        // 完美轉發:保留呼叫端傳進來的值類別,move-only 的參數才傳得進去
        t = std::thread(std::forward<Func>(f), std::forward<Args>(args)...);
    }

    void join() {
        if (t.joinable()) {   // 可安全重複呼叫
            t.join();
        }
    }

    bool joinable() const noexcept { return t.joinable(); }

    ~SafeThread() {
        join();  // 解構時自動 join
    }

    // 明確禁止複製:這個類別獨佔一條執行緒的所有權
    SafeThread(const SafeThread&) = delete;
    SafeThread& operator=(const SafeThread&) = delete;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】可取消的背景輪詢:補上 SafeThread 缺少的「停止機制」
//   情境: 真實系統的背景執行緒通常是個迴圈,不會自己結束。
//         若解構子直接 join(),就會永遠卡住 —— 這正是 SafeThread
//         這類最小守衛在實務上不夠用的地方。
//         標準解法是加一個 atomic 停止旗標:解構時先通知停止,再 join。
//   為什麼用本主題: 這就是 C++20 的 std::jthread + stop_token 幫你做的事。
//         自己實作一次,才會理解 jthread 的價值不只是「自動 join」。
// -----------------------------------------------------------------------------
class CancellablePoller {
    std::atomic<bool> stop_{false};
    std::atomic<int>  rounds_{0};
    std::thread       worker_;

public:
    explicit CancellablePoller(const std::string& name) {
        worker_ = std::thread([this, name]() {
            while (!stop_.load(std::memory_order_relaxed)) {
                rounds_.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            say("    [" + name + "] 收到停止訊號,迴圈已結束");
        });
    }

    void requestStop() { stop_.store(true, std::memory_order_relaxed); }

    int rounds() const { return rounds_.load(std::memory_order_relaxed); }

    ~CancellablePoller() {
        requestStop();                              // ① 先通知停止
        if (worker_.joinable()) worker_.join();     // ② 再等它真的結束
    }

    CancellablePoller(const CancellablePoller&) = delete;
    CancellablePoller& operator=(const CancellablePoller&) = delete;
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔從缺,理由如下
//   本檔的主題是 std::thread 物件的所有權與生命週期管理(joinable 不變量、
//   move assignment 的陷阱、RAII 守衛設計),屬於 C++ 資源管理的範疇。
//   LeetCode 的五題並行題(1114 Print in Order、1115 Print FooBar Alternately、
//   1116 Print Zero Even Odd、1117 Building H2O、1195 Fizz Buzz Multithreaded)
//   由評測框架負責建立與回收執行緒,參賽者只實作被呼叫的成員函式,
//   完全接觸不到 std::thread 物件 —— joinable()、move assignment、
//   RAII 守衛在那些題目裡一次都用不到。
//   本課的其他總整理檔已分別示範了 1114(課程 2.1)、1115(課程 2.2)、
//   1116(課程 2.3)、1195(課程 2.4);此處硬湊剩下的 1117 Building H2O
//   會完全偏離主題(它考的是屏障與號誌),因此誠實從缺,
//   改以上面的可取消輪詢示範真實系統中的執行緒生命週期管理。
// -----------------------------------------------------------------------------

int main() {
    std::cout << "=== 原始示範:SafeThread 自動管理生命週期 ===" << std::endl;
    {
        SafeThread st;

        st.start([]() {
            say("  任務 1");
        });

        st.start([]() {   // 內部會先 join 前一個(少了那步就會 terminate)
            say("  任務 2");
        });

        // 解構時自動 join
    }
    std::cout << "  ↑ 兩次 start() 都正常 —— 因為 start() 內部先 join 了前一條"
              << std::endl;

    std::cout << "\n=== 完美轉發:連 move-only 的參數也傳得進去 ===" << std::endl;
    {
        SafeThread st;
        auto data = std::make_unique<std::string>("只能移動的資料");

        st.start([](std::unique_ptr<std::string> p) {
            say("  收到: " + *p);
        }, std::move(data));

        st.join();
        std::cout << "  移動後,原本的 unique_ptr "
                  << (data ? "仍有效" : "已為空(所有權已轉移)") << std::endl;
    }

    std::cout << "\n=== 為什麼 start() 裡的 join 不可省略 ===" << std::endl;
    std::cout << "  對仍 joinable 的 std::thread 做 move assignment"
                 " → std::terminate()" << std::endl;
    std::cout << "  本機實測:std::thread  版本 exit code = 134 (SIGABRT)"
              << std::endl;
    std::cout << "            std::jthread 版本 exit code = 0   (先 request_stop"
                 "+join,正常存活)" << std::endl;
    std::cout << "  ⚠️ 兩者是不同的規則,不要把 jthread 的印象套到 thread 上"
              << std::endl;

    std::cout << "\n=== 實務:可取消的背景輪詢(SafeThread 缺的那一塊) ===" << std::endl;
    {
        CancellablePoller poller("metrics");
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        // 解構時:先 requestStop() 再 join() —— 迴圈型的執行緒才不會卡死
    }
    std::cout << "  ↑ 少了停止旗標,解構子的 join() 會永遠卡住 ——"
                 "這正是 C++20 std::jthread + stop_token 要解決的問題"
              << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.5：joinable() 狀態檢查2.cpp" -o safe_thread

// 注意:以下為某一次實際執行的結果,每次執行都相同 ——
//       所有觀察點都在 join() 之後,而且刻意不輸出會隨排程浮動的數值
//       (例如輪詢實際跑了幾圈)。

// === 預期輸出 ===
// === 原始示範:SafeThread 自動管理生命週期 ===
//   任務 1
//   任務 2
//   ↑ 兩次 start() 都正常 —— 因為 start() 內部先 join 了前一條
//
// === 完美轉發:連 move-only 的參數也傳得進去 ===
//   收到: 只能移動的資料
//   移動後,原本的 unique_ptr 已為空(所有權已轉移)
//
// === 為什麼 start() 裡的 join 不可省略 ===
//   對仍 joinable 的 std::thread 做 move assignment → std::terminate()
//   本機實測:std::thread  版本 exit code = 134 (SIGABRT)
//             std::jthread 版本 exit code = 0   (先 request_stop+join,正常存活)
//   ⚠️ 兩者是不同的規則,不要把 jthread 的印象套到 thread 上
//
// === 實務:可取消的背景輪詢(SafeThread 缺的那一塊) ===
//     [metrics] 收到停止訊號,迴圈已結束
//   ↑ 少了停止旗標,解構子的 join() 會永遠卡住 ——這正是 C++20 std::jthread + stop_token 要解決的問題
