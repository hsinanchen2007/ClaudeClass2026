/*
# 第二階段：std::thread 基礎

## 課程 2.4：join() 與 detach()

---

### 引言

每個 `std::thread` 物件在解構前，必須明確決定如何處理其執行緒：等待它完成（join）或放手讓它獨立運行（detach）。這是執行緒管理最重要的決策。

---

### 一、兩種選擇

```
┌─────────────────────────────────────────────────────────────┐
│                  join() vs detach()                         │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  join()                        detach()                     │
│  ──────                        ────────                     │
│  • 阻塞等待執行緒結束           • 讓執行緒獨立運行            │
│  • 確保執行緒完成工作           • 不等待，立即返回            │
│  • 可以安全存取執行緒的結果     • 無法再與該執行緒互動         │
│  • 較常使用                    • 適合背景任務                │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 二、join() 的阻塞行為

`join()` 會阻塞當前執行緒，直到目標執行緒結束：

```cpp
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "1. 建立執行緒" << std::endl;
    
    std::thread t([]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "3. 執行緒完成" << std::endl;
    });
    
    std::cout << "2. 呼叫 join()，開始等待..." << std::endl;
    t.join();  // 在這裡阻塞 2 秒
    
    std::cout << "4. join() 返回，繼續執行" << std::endl;
    return 0;
}
```

輸出：
```
1. 建立執行緒
2. 呼叫 join()，開始等待...
3. 執行緒完成
4. join() 返回，繼續執行
```

---

### 三、detach() 的獨立運行

`detach()` 讓執行緒在背景獨立執行：

```cpp
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::thread t([]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "背景執行緒完成" << std::endl;  // 可能不會印出
    });
    
    t.detach();  // 放手，不等待
    
    std::cout << "主執行緒立即繼續" << std::endl;
    // 主程式結束時，背景執行緒會被強制終止
    return 0;
}
```

輸出（通常）：
```
主執行緒立即繼續
```

---

### 四、detach() 的危險

分離後的執行緒若存取已銷毀的資源，會導致未定義行為：

```cpp
#include <iostream>
#include <thread>
#include <chrono>

void dangerous() {
    int localVar = 42;
    
    std::thread t([&localVar]() {  // 捕獲區域變數的引用
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << localVar << std::endl;  // 危險！localVar 可能已被銷毀
    });
    
    t.detach();
}  // localVar 在這裡被銷毀，但執行緒還在運行

int main() {
    dangerous();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}
```

**規則**：detach 的執行緒不應存取可能被銷毀的區域變數。

---

### 五、joinable() 狀態檢查

在呼叫 join() 或 detach() 前，可以用 `joinable()` 檢查：

```cpp
#include <iostream>
#include <thread>

int main() {
    std::thread t([]() {
        std::cout << "Working" << std::endl;
    });
    
    std::cout << "joinable: " << t.joinable() << std::endl;  // true
    
    t.join();
    
    std::cout << "joinable: " << t.joinable() << std::endl;  // false
    
    return 0;
}
```

輸出：
```
joinable: 1
Working
joinable: 0
```

---

### 六、joinable() 的狀態轉換

```
┌─────────────────────────────────────────────────┐
│           joinable() 狀態變化                   │
├─────────────────────────────────────────────────┤
│                                                 │
│  std::thread t;          → joinable() = false  │
│  std::thread t(func);    → joinable() = true   │
│                                                 │
│  呼叫 join() 後          → joinable() = false  │
│  呼叫 detach() 後        → joinable() = false  │
│  被 move 走後            → joinable() = false  │
│                                                 │
└─────────────────────────────────────────────────┘
```

---

### 七、安全的 join 檢查

避免對 non-joinable 執行緒呼叫 join()：

```cpp
#include <iostream>
#include <thread>

int main() {
    std::thread t([]() {
        std::cout << "Task" << std::endl;
    });
    
    // 安全的寫法
    if (t.joinable()) {
        t.join();
    }
    
    // 再次呼叫是安全的（因為有檢查）
    if (t.joinable()) {
        t.join();  // 不會執行
    }
    
    return 0;
}
```

---

### 八、重複呼叫 join() 會崩潰

```cpp
#include <iostream>
#include <thread>

int main() {
    std::thread t([]() {});
    
    t.join();
    t.join();  // 崩潰！std::system_error
    
    return 0;
}
```

---

### 九、何時用 join，何時用 detach

| 情況 | 選擇 | 原因 |
|------|------|------|
| 需要執行緒的結果 | join | 必須等待完成 |
| 後續操作依賴執行緒完成 | join | 確保順序 |
| 真正的背景任務（如日誌） | detach | 不需要結果 |
| 執行緒使用區域變數 | join | 避免懸空引用 |
| 不確定時 | join | 較安全 |

---

### 十、完整範例

```cpp
// 檔案：lesson_2_4_join_detach.cpp

#include <iostream>
#include <thread>
#include <chrono>

int main() {
    // 範例 1：join
    std::thread t1([]() {
        std::cout << "[t1] 開始工作" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "[t1] 完成" << std::endl;
    });
    
    std::cout << "[main] 等待 t1..." << std::endl;
    t1.join();
    std::cout << "[main] t1 已結束" << std::endl;
    
    // 範例 2：detach
    std::thread t2([]() {
        std::cout << "[t2] 背景執行中..." << std::endl;
    });
    
    t2.detach();
    std::cout << "[main] t2 已分離" << std::endl;
    
    // 給 t2 一點時間完成輸出
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    return 0;
}
```

---

### 十一、本課重點回顧

1. 每個 `std::thread` 必須呼叫 `join()` 或 `detach()`
2. `join()` 阻塞等待執行緒結束
3. `detach()` 讓執行緒獨立運行，但要小心資源生命週期
4. `joinable()` 檢查執行緒是否可以被 join 或 detach
5. 對 non-joinable 執行緒呼叫 join() 會崩潰
6. 不確定時，優先選擇 `join()`

---

### 下一課預告

在 **課程 2.5：joinable() 狀態檢查** 中，我們將更深入探討：
- 執行緒物件的各種狀態
- 預設建構的執行緒
- 狀態轉換的完整圖解

---

準備好繼續嗎？
*/

// =============================================================================
//  課程 2.4：join() 與 detach()7.cpp  —  join / detach 總整理
// =============================================================================
//
// 【主題資訊 Information】
//   join()    : void join();    阻塞等待、建立同步、回收資源
//   detach()  : void detach();  切斷關聯、放棄所有權、由系統自動回收
//   標準版本  : C++11
//   標頭檔    : <thread>
//   共同前置  : 兩者都要求 joinable() == true,否則丟 std::system_error
//   共同後果  : 兩者呼叫後 joinable() 都變成 false
//   解構要求  : 解構時 joinable() 必須為 false,否則 std::terminate()
//
// 【詳細解釋 Explanation】
//
// 【1. 一張表看懂兩者的取捨】
//
//                        join()                  detach()
//   ────────────────────  ─────────────────────   ────────────────────────
//   呼叫端是否阻塞        是,直到執行緒結束      否,立刻返回
//   能否知道它做完了      能(join 返回即完成)   不能,永遠不知道
//   能否取得結果          能(join 後讀共享變數) 不能,沒有同步點
//   記憶體同步保證        有 happens-before       沒有
//   資源由誰回收          由 join() 回收          由執行期系統自動回收
//   程式結束時            一定已經處理完          不會被等待,可能被強制中斷
//   區域變數可否借用      可以(join 在銷毀前)   絕對不可以
//
// 這張表最重要的一列是「記憶體同步保證」。join() 之所以是預設選擇,
// 不只因為它會等,更因為它讓你「保證看得到」執行緒寫的資料。
// detach() 之後,你連對方做完了沒都不知道,自然也無從保證看得到結果 ——
// 要跨過這道牆,只能自己用 atomic、mutex 或條件變數建立同步。
//
// 【2. 決策流程】
//   問題一:我需要這個工作的結果嗎?
//     需要 → join()(或更好:std::async + future)
//   問題二:這個工作需要在呼叫端的作用域結束後繼續活著嗎?
//     不需要 → join()
//     需要   → 先問問題三
//   問題三:它需要碰任何呼叫端的資料嗎?
//     需要 → 不可以 detach。改用值傳遞/shared_ptr 完整帶走,
//            或改用執行緒池讓生命週期由池管理
//     不需要,而且程式結束時中斷它也無所謂 → 才可以考慮 detach()
//
// 結論:**join() 是預設,detach() 是例外。**
// 現代 C++ 更常見的答案是「兩者都不用」——改用 std::async、
// 執行緒池,或 C++20 的 std::jthread。
//
// 【3. 本檔輸出的兩段各自示範什麼】
//   範例 1(join)  :"[main] 等待 t1..." → "[t1] 完成" → "[main] t1 已結束"
//                    這個順序是 join() 鎖死的,每次都一樣。
//   範例 2(detach):detach 之後,主執行緒立刻印出 "t2 已分離",
//                    而 t2 什麼時候印出訊息完全不確定 ——
//                    程式甚至可能在它印出來之前就結束。
//                    本例靠 sleep 100ms「等它一下」,但那只是示範用的
//                    權宜之計,不是可靠的同步機制。
//
// ⚠️ 用 sleep 來「等」另一條執行緒,是多執行緒程式最常見的錯誤習慣之一。
//    它在你的機器上看起來能用,換到負載高的機器就會失敗,
//    而且失敗是隨機的。要等就用 join()、future 或條件變數。
//
// 【4. 為什麼 detach 之後仍然可能「看起來正常」】
// 這是 detach 最危險的地方。它造成的問題(懸空存取、程式結束時被中斷)
// 都屬於「時序敏感」的錯誤:在開發機的輕負載下幾乎不會出現,
// 上了正式環境、流量一大才第一次爆,而且無法穩定重現。
// 這也是為什麼多執行緒程式不能靠「我測過都沒問題」來驗證正確性。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼標準不提供「等待但有逾時」的 join
//   委員會的立場是:需要逾時,代表你要的其實是「任務的結果」而不是
//   「執行緒的生命週期」。那是 std::future 的職責(wait_for / wait_until)。
//   std::thread 刻意維持成低階、單純的所有權 handle。
//
// (B) 兩者都失敗的第三條路:std::async
//     auto fut = std::async(std::launch::async, work);
//     fut.get();   // 等待 + 取得回傳值 + 若有例外會在這裡重新丟出
//   它同時解決了 join 的三個不足:能拿回傳值、能傳遞例外、能設逾時。
//   缺點是預設策略可能不真的開執行緒(所以要明確寫 std::launch::async),
//   而且 future 的解構在某些情況下會阻塞。
//
// (C) C++20 的 std::jthread
//   解構時自動 request_stop() 並 join(),讓「忘記 join」不再是錯誤。
//   ⚠️ 注意它和 std::thread 的 move assignment 規則不同:
//      對仍 joinable 的 std::thread 做 move-assign 會 std::terminate();
//      而 std::jthread 會先 request_stop() + join(),然後正常存活。
//      這是兩套不同的規則,不要把「差不多」的印象帶過去。
//
// 【注意事項 Pay Attention】
// 1. join() 是預設;detach() 只在「真正的射後不理」時使用。
// 2. detach 的執行緒絕不可存取呼叫端的區域變數。
// 3. 程式結束不會等待 detach 的執行緒,它們可能在靜態物件銷毀期間
//    仍在執行,碰到已銷毀的全域物件(含 std::cout)就是 UB。
// 4. 用 sleep 來「等」另一條執行緒是錯的;要等就用 join/future/條件變數。
// 5. 兩者都只能呼叫一次,且都要求 joinable() 為 true。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】join 與 detach 總整理
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. join() 和 detach() 最關鍵的差別是什麼?
//     答：除了「等不等」之外,最關鍵的是記憶體同步保證。
//         join() 建立 happens-before:返回後你保證看得到執行緒寫的所有資料。
//         detach() 沒有任何同步點 —— 你連它做完了沒都不知道,
//         也無從保證看得到它的結果,要溝通只能自己用 atomic/mutex/條件變數。
//     追問：那什麼時候該用 detach?
//         → 很少。必須同時滿足:不需要結果、不碰呼叫端任何資料、
//           程式結束時被中斷也無所謂。多數情況下更好的選擇是
//           std::async、執行緒池,或 C++20 的 std::jthread。
//
// 🔥 Q2. 用 std::this_thread::sleep_for() 等待另一條執行緒完成,有什麼問題?
//     答：它不是同步機制,只是猜測。在你的機器上「等夠久」的時間,
//         在高負載的正式環境可能不夠,而且失敗是隨機、無法重現的。
//         它也不提供任何記憶體可見性保證。要等就用 join()、
//         future.get(),或條件變數。
//     追問：那為什麼教學範例裡常看到 sleep?
//         → 因為要示範 detach 的行為就必須讓主執行緒多活一會兒,
//           而正確的同步工具還沒教到。這是教學上的權宜之計,
//           不該帶進正式程式碼 —— 好的教材應該明確標注這一點。
//
// ⚠️ 陷阱. 「我的程式用了 detach,在開發機跑了幾百次都正常,
//         所以 detach 的用法是對的。」哪裡錯了?
//     答：detach 造成的問題全都是時序敏感的 —— 懸空存取、程式結束時
//         執行緒被中斷、在靜態物件銷毀期間碰到已銷毀的全域物件。
//         這些在開發機的輕負載下幾乎不會出現,要等到正式環境流量一大
//         才第一次爆,而且無法穩定重現。
//     為什麼會錯：把測試當成正確性的證明。測試只能證明「有 bug」,
//         不能證明「沒有 bug」,這一點在並行程式上被放大到極致。
//         正確做法是從設計上論證(這條執行緒碰的每樣東西都活得比它久嗎?),
//         並用 ThreadSanitizer / AddressSanitizer 主動偵測。
// ═══════════════════════════════════════════════════════════════════════════

#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1195. Fizz Buzz Multithreaded
//   題目：四條執行緒分別呼叫 fizz()、buzz()、fizzbuzz()、number(),
//         合作輸出 1..n 的 FizzBuzz 序列:
//         被 3 整除印 "fizz"、被 5 整除印 "buzz"、被 15 整除印 "fizzbuzz"、
//         其餘印數字本身。
//   為什麼用到本主題：這題是本課「執行緒生命週期管理」的完整演練 ——
//         四條執行緒必須「全部」被 join,少 join 任何一條,
//         主執行緒都可能在序列還沒印完就結束。
//         而且它示範了 detach 在這裡為什麼完全不能用:
//         detach 之後你無法知道四條執行緒都做完了沒,
//         輸出會被截斷,評測直接判錯。
//   作法：用一個共用的 cur_ 表示「現在該處理哪個數字」,
//         每條執行緒在條件變數上等到 cur_ 符合自己負責的規則才動作,
//         印完把 cur_ 往前推並喚醒其他人。cur_ > n_ 時全部收工返回。
//   複雜度：時間 O(n),空間 O(1)。
// -----------------------------------------------------------------------------
class FizzBuzz {
    int                     n_;
    int                     cur_ = 1;
    std::mutex              m_;
    std::condition_variable cv_;

public:
    explicit FizzBuzz(int n) : n_(n) {}

    void fizz(std::function<void()> printFizz) {
        while (true) {
            std::unique_lock<std::mutex> lk(m_);
            cv_.wait(lk, [this] {
                return cur_ > n_ || (cur_ % 3 == 0 && cur_ % 5 != 0);
            });
            if (cur_ > n_) return;
            printFizz();
            ++cur_;
            cv_.notify_all();
        }
    }

    void buzz(std::function<void()> printBuzz) {
        while (true) {
            std::unique_lock<std::mutex> lk(m_);
            cv_.wait(lk, [this] {
                return cur_ > n_ || (cur_ % 5 == 0 && cur_ % 3 != 0);
            });
            if (cur_ > n_) return;
            printBuzz();
            ++cur_;
            cv_.notify_all();
        }
    }

    void fizzbuzz(std::function<void()> printFizzBuzz) {
        while (true) {
            std::unique_lock<std::mutex> lk(m_);
            cv_.wait(lk, [this] {
                return cur_ > n_ || (cur_ % 15 == 0);
            });
            if (cur_ > n_) return;
            printFizzBuzz();
            ++cur_;
            cv_.notify_all();
        }
    }

    void number(std::function<void(int)> printNumber) {
        while (true) {
            std::unique_lock<std::mutex> lk(m_);
            cv_.wait(lk, [this] {
                return cur_ > n_ || (cur_ % 3 != 0 && cur_ % 5 != 0);
            });
            if (cur_ > n_) return;
            printNumber(cur_);
            ++cur_;
            cv_.notify_all();
        }
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】優雅關閉:用 join 而不是 detach 來結束背景工作
//   情境: 一個服務有背景的批次寫入執行緒。關閉服務時,必須確定
//         「還沒寫完的資料真的寫完了」才能結束程式 ——
//         這正是 detach 做不到、而 join 天生就能保證的事。
//   為什麼用本主題: 對照【詳細解釋 1】那張表最後兩列:
//         join 保證「程式結束時一定已經處理完」,而 detach
//         只會讓未完成的工作在行程結束時被無聲地丟掉。
// -----------------------------------------------------------------------------
class BatchWriter {
    std::mutex              m_;
    std::condition_variable cv_;
    std::vector<std::string> queue_;
    bool                    stopping_ = false;
    int                     written_  = 0;
    std::thread             worker_;

public:
    BatchWriter() {
        worker_ = std::thread([this] {
            while (true) {
                std::unique_lock<std::mutex> lk(m_);
                cv_.wait(lk, [this] { return stopping_ || !queue_.empty(); });

                // 關鍵:即使已收到停止訊號,也要先把佇列清空才離開
                while (!queue_.empty()) {
                    queue_.pop_back();
                    ++written_;
                }
                if (stopping_) return;
            }
        });
    }

    void submit(const std::string& row) {
        {
            std::lock_guard<std::mutex> lk(m_);
            queue_.push_back(row);
        }
        cv_.notify_one();
    }

    // 優雅關閉:通知 → join → 保證所有資料都已寫完
    void shutdown() {
        {
            std::lock_guard<std::mutex> lk(m_);
            stopping_ = true;
        }
        cv_.notify_all();
        if (worker_.joinable()) worker_.join();   // ← 用 detach 就沒有這個保證
    }

    int written() const { return written_; }

    ~BatchWriter() { shutdown(); }

    BatchWriter(const BatchWriter&) = delete;
    BatchWriter& operator=(const BatchWriter&) = delete;
};

int main() {
    std::cout << "=== 範例 1:join —— 順序完全確定 ===" << std::endl;
    {
        std::thread t1([]() {
            std::cout << "[t1] 開始工作" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::cout << "[t1] 完成" << std::endl;
        });

        std::cout << "[main] 等待 t1..." << std::endl;
        t1.join();
        std::cout << "[main] t1 已結束" << std::endl;
    }

    std::cout << "\n=== 範例 2:detach —— 時序不確定 ===" << std::endl;
    {
        std::thread t2([]() {
            std::cout << "[t2] 背景執行中..." << std::endl;
        });

        t2.detach();
        std::cout << "[main] t2 已分離" << std::endl;

        // ⚠️ 用 sleep「等」執行緒是錯誤示範,這裡只是為了讓 t2 有機會印出訊息。
        //    正式程式碼要等就該用 join/future/條件變數。
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "  ↑ [t2] 那行何時出現(甚至會不會出現)並無保證"
                  << std::endl;
    }

    std::cout << "\n=== LeetCode 1195. Fizz Buzz Multithreaded (n=15) ===" << std::endl;
    {
        FizzBuzz fb(15);
        // 為了閱讀方便,輸出時用空白分隔;LeetCode 的評測則是直接串接比對。
        // 這裡先收集到一個字串再一次印出,好處有二:
        //   (1) 可以去掉行尾多餘的空白(行尾空白是輸出比對的常見干擾源);
        //   (2) 這些 lambda 都是在持有 m_ 的情況下被呼叫的,所以附加到
        //       共用字串是安全的,不需要額外的鎖。
        std::string out;
        auto pFizz     = [&out] { out += "fizz ";     };
        auto pBuzz     = [&out] { out += "buzz ";     };
        auto pFizzBuzz = [&out] { out += "fizzbuzz "; };
        auto pNumber   = [&out](int x) { out += std::to_string(x) + " "; };

        std::thread a(&FizzBuzz::fizz,     &fb, pFizz);
        std::thread b(&FizzBuzz::buzz,     &fb, pBuzz);
        std::thread c(&FizzBuzz::fizzbuzz, &fb, pFizzBuzz);
        std::thread d(&FizzBuzz::number,   &fb, pNumber);

        // 四條都必須 join —— 少 join 任何一條,序列都可能被截斷
        a.join();
        b.join();
        c.join();
        d.join();

        while (!out.empty() && out.back() == ' ') out.pop_back();
        std::cout << out << std::endl;
        std::cout << "  ↑ 每次執行都固定是這個序列,"
                     "因為順序由條件變數保證,不是碰運氣" << std::endl;
    }

    std::cout << "\n=== 實務:優雅關閉,join 保證資料寫完 ===" << std::endl;
    {
        BatchWriter writer;
        for (int i = 1; i <= 500; ++i) {
            writer.submit("row-" + std::to_string(i));
        }
        writer.shutdown();   // 通知 + join
        std::cout << "  關閉後已寫入筆數 = " << writer.written()
                  << " (期望 500 —— join 保證背景執行緒把佇列清空了)"
                  << std::endl;
        std::cout << "  ↑ 若這裡改用 detach,程式可能在資料寫完前就結束,"
                     "而且你永遠不會知道" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.4：join() 與 detach()7.cpp" -o join_detach

// 注意:以下為某一次實際執行的結果。
//   * 範例 2 中「[t2] 背景執行中...」與「[main] t2 已分離」兩行的先後
//     每次執行都可能互換,甚至在極端情況下 [t2] 那行可能完全不出現 ——
//     那正是 detach 沒有任何保證的體現。
//   * 其餘每一段每次執行都相同:範例 1 的順序由 join 鎖死,
//     LeetCode 1195 的序列由條件變數保證,
//     實務段的 500 筆則由「通知 + join」保證寫完。

// === 預期輸出 ===
// === 範例 1:join —— 順序完全確定 ===
// [main] 等待 t1...
// [t1] 開始工作
// [t1] 完成
// [main] t1 已結束
//
// === 範例 2:detach —— 時序不確定 ===
// [main] t2 已分離
// [t2] 背景執行中...
//   ↑ [t2] 那行何時出現(甚至會不會出現)並無保證
//
// === LeetCode 1195. Fizz Buzz Multithreaded (n=15) ===
// 1 2 fizz 4 buzz fizz 7 8 fizz buzz 11 fizz 13 14 fizzbuzz
//   ↑ 每次執行都固定是這個序列,因為順序由條件變數保證,不是碰運氣
//
// === 實務:優雅關閉,join 保證資料寫完 ===
//   關閉後已寫入筆數 = 500 (期望 500 —— join 保證背景執行緒把佇列清空了)
//   ↑ 若這裡改用 detach,程式可能在資料寫完前就結束,而且你永遠不會知道
