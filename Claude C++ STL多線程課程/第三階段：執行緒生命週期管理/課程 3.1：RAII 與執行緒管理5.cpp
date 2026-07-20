// =============================================================================
//  課程 3.1：RAII 與執行緒管理 5  —  課文總整理：手動管理 vs RAII 的對照實證
// =============================================================================
//
// 【主題資訊 Information】
//   本檔是課程 3.1 的完整課文 + 可執行的對照實驗。
//
//   核心規則(C++11 起,[thread.thread.destr]):
//       ~thread() { if (joinable()) std::terminate(); }
//
//   涉及的三個守衛設計(前四個檔案逐一展開):
//       ThreadGuard    持有 std::thread&   —— 不擁有,依賴宣告順序
//       ScopedThread   持有 std::thread    —— 擁有,建構函式建立不變式
//       FlexibleThread 持有 std::thread + 策略 —— 解構時 join 或 detach
//
//   標準版本：C++11(std::thread / enum class);C++20 的 std::jthread 是
//             標準化的 RAII 版本
//   標頭檔  ：<thread>、<stdexcept>
//
// 【詳細解釋 Explanation】
//
// 【1. 這一課真正要建立的心智模型】
//   多數教材把 RAII 講成「解構函式會自動釋放資源,很方便」。這太弱了。
//   RAII 真正的價值是【把「必須發生的事」從控制流中移除】。
//
//   手動管理的問題不是「要多打幾個字」,而是:
//   一個函式有 N 個離開點(return / break / throw / 例外從被呼叫函式傳上來),
//   你就必須在 N 個地方寫對清理程式碼。而其中「例外傳播」這個離開點
//   【在原始碼裡根本看不見】—— 任何一個看似無害的呼叫都可能丟例外。
//   所以手動管理的正確性,取決於你能不能窮舉所有看不見的離開點。
//   RAII 把 N 個離開點收斂成 1 個:作用域結束。
//
// 【2. 本檔的 manual() 為什麼是誠實的示範】
//   課文第六節的 manual() 是【虛擬碼】。本檔把它改成真的能編譯執行,
//   並在 condition2 的分支【故意】不寫 t.join() —— 這正是真實世界最常
//   發生的事:程式員在寫新的錯誤處理分支時,忘了那裡也有一個執行緒要收。
//
//   結果不是「可能崩潰」,而是【標準保證的 std::terminate()】:
//   t 解構時 joinable() 為 true,標準規定必須呼叫 std::terminate()。
//   這一點非常重要,請和另一類錯誤區分開:
//     * 解構 joinable 的 thread → std::terminate(),標準明文規定的確定結果
//     * data race(兩執行緒無同步地存取同一物件,至少一方寫入)→ 未定義行為,
//       沒有固定症狀,可能看似正常
//   兩者都很糟,但可預測性完全不同。前者一定會炸給你看,後者可能潛伏數月。
//
//   因為 terminate() 會讓整個程式 abort、無法被 catch,這個示範被放在
//   #ifdef DEMONSTRATE_UB 之後,預設不執行 —— 否則後面的示範都跑不到。
//
// 【3. most vexing parse:一個會讓示範「靜悄悄失效」的陷阱】
//   automatic() 裡刻意寫成 ScopedThread st{std::thread(work)};(大括號)。
//   如果寫成小括號:
//       ScopedThread st(std::thread(work));
//   C++ 的文法規則會把它解析成【一個函式宣告】:
//       宣告一個名為 st 的函式,回傳 ScopedThread,
//       參數是一個「回傳 std::thread、參數為 work 的函式指標」。
//   結果是:沒有物件被建立、沒有執行緒被啟動、也【不會有任何編譯錯誤】。
//   示範就這樣無聲地失效了。
//
//   為什麼會這樣?因為 C++ 有一條消歧義規則:「任何可能被解析成宣告的
//   東西,就解析成宣告」。C++11 引入的大括號初始化(brace initialization)
//   不能被解析成函式宣告,所以 {} 天生免疫這個問題 —— 這也是
//   「盡量用 {}」這條現代 C++ 建議的主要理由之一。
//
// 【4. 為什麼 C++20 要把這件事標準化成 std::jthread】
//   前面四個檔案示範了三種守衛,而每個專案都在重寫類似的東西 ——
//   這本身就是「標準缺了一塊」的訊號。std::jthread 補上了:
//       ~jthread() { if (joinable()) { request_stop(); join(); } }
//   注意它【不是】單純的「自動 join 版 thread」。關鍵在 request_stop():
//   它提供了 std::thread 從來沒有的【協作式取消管道】,所以那個自動的
//   join() 才不會變成「可能永遠不返回」。
//   換句話說:jthread 之所以敢做 std::thread 不敢做的事,是因為它同時
//   帶來了讓那件事變安全的前提條件。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 解構函式在例外傳播時一定會被呼叫嗎?
//   有前提。標準規定:若例外【沒有】被任何 handler 捕捉,是否執行
//   stack unwinding 是【實作定義】的([except.terminate])。也就是說,
//   uncaught exception 的情況下,你的 RAII 守衛可能根本沒機會執行。
//   GCC 在多數情境仍會 unwinding,但這不是可依賴的保證。
//   實務結論:main() 裡放一個 catch-all,別讓例外逸出。
//
// (B) 為什麼「解構函式不能丟例外」是 RAII 的隱形前提?
//   若 stack unwinding 期間又有第二個例外從解構函式逸出,執行期
//   無法決定該傳播哪一個 → 直接 std::terminate()。C++11 之後解構函式
//   預設 noexcept(true),使這件事更明確。因此所有 RAII 守衛的解構函式
//   都應該吞掉例外(至多記 log),不可讓它逸出。
//
// (C) 為什麼 std::thread 不像 std::unique_ptr 那樣「解構就釋放」?
//   因為「釋放執行緒」有兩種語意完全不同的做法,而標準無法替你選:
//     join()   —— 語意是「等它做完」,可能阻塞任意久
//     detach() —— 語意是「不管它了」,可能造成 dangling reference
//   unique_ptr 沒有這個問題:釋放記憶體只有一種意思。
//   標準的選擇是:與其猜錯,不如要求你明講。
//
// (D) 本課三個守衛的取捨總表
//   ┌────────────────┬──────────┬────────────┬──────────┬──────────────┐
//   │ 設計           │ 所有權   │ 可放容器   │ 解構策略 │ 主要風險     │
//   ├────────────────┼──────────┼────────────┼──────────┼──────────────┤
//   │ ThreadGuard    │ 否(參考)│ 否         │ join     │ 宣告順序/懸空│
//   │ ScopedThread   │ 是       │ 否(不可移)│ join     │ 彈性低       │
//   │ FlexibleThread │ 是       │ 否(不可移)│ join/det │ detach 的風險│
//   │ JoiningThread  │ 是       │ 是(可移動)│ join     │ 需自行維護   │
//   │ std::jthread   │ 是       │ 是         │ stop+join│ 需 C++20     │
//   └────────────────┴──────────┴────────────┴──────────┴──────────────┘
//
// 【注意事項 Pay Attention】
//   1. 解構 joinable 的 std::thread 會呼叫 std::terminate() —— 標準明文
//      規定的確定行為,不是未定義行為,也無法被 catch。
//   2. manual(false, true) 的呼叫被 #ifdef DEMONSTRATE_UB 隔離,預設不執行。
//      要親眼觀察請加 -DDEMONSTRATE_UB 重新編譯(程式會 abort)。
//   3. 用小括號建立 RAII 物件可能觸發 most vexing parse,示範會靜悄悄失效;
//      請用大括號 ScopedThread st{std::thread(work)}。
//   4. 課文第六節的 manual()/automatic() 原本是虛擬碼(work、condition1、
//      error 皆未宣告),本檔已補齊成可編譯的版本。
//   5. 本檔的 ScopedThread 不可複製也不可移動(自訂解構函式抑制了隱式
//      移動),無法放進 std::vector —— 這是刻意保留課文原貌。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】RAII 與執行緒生命週期
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼「函式中途丟例外」對手動 join 是致命的,對 RAII 卻不是?
//     答：例外是一個【在原始碼裡看不見的離開點】。手動管理必須在每一個
//         離開點都寫對 join,而例外可能從任何一個被呼叫的函式傳上來,
//         你無法窮舉。RAII 把所有離開點收斂成「作用域結束」這一件事,
//         解構函式在正常返回與 stack unwinding 時都會被呼叫。
//     追問：那 RAII 有沒有失效的情況?
//         → 有兩種。(1) 例外完全沒被 catch 時,標準允許實作不做 unwinding
//           就直接 terminate,守衛不會執行;(2) std::exit() / std::abort()
//           / 從 detached thread 存取已銷毀物件,都不會走 unwinding。
//           所以「main 要有 catch-all」不是形式主義。
//
// 🔥 Q2. std::jthread 只是「會自動 join 的 std::thread」嗎?
//     答：不是,這個理解會漏掉重點。jthread 的解構函式是
//         request_stop() 然後 join(),關鍵在前半段 —— 它內建了
//         stop_token 協作取消管道。沒有這個管道,自動 join 就等於
//         「解構函式可能永遠不返回」,那正是 std::thread 當初拒絕
//         自動 join 的原因。
//     追問：request_stop() 會強制終止執行緒嗎?
//         → 不會。它只是把 stop_token 的狀態設為「已請求停止」,
//           工作函式必須自己去輪詢 stop_requested() 並主動返回。
//           寫成無窮迴圈又不檢查 token 的工作函式,一樣會讓
//           jthread 的解構函式永遠卡住。C++ 沒有安全的強制中止機制。
//
// ⚠️ 陷阱 1. 「ScopedThread st(std::thread(work)); 這行為什麼沒有啟動執行緒?」
//     答：這觸發了 most vexing parse。編譯器把整行解析成【函式宣告】——
//         宣告一個叫 st、回傳 ScopedThread、參數是函式指標的函式。
//         沒有物件被建立,也沒有任何編譯錯誤。改用大括號
//         ScopedThread st{std::thread(work)} 即可,因為大括號初始化
//         不可能被解析成函式宣告。
//     為什麼會錯：以為「有小括號就是在呼叫建構函式」。實際上 C++ 有
//         「能解析成宣告就當宣告」的消歧義規則,而這條規則優先。
//
// ⚠️ 陷阱 2. 「忘記 join 導致的 terminate,加個 try/catch 包起來就能救吧?」
//     答：救不了。std::terminate() 不是丟例外,它是直接呼叫 terminate
//         handler(預設 std::abort()),完全不經過例外機制,catch(...)
//         攔不到。唯一的解法是一開始就不要讓 joinable 的 thread 被解構。
//     為什麼會錯：把 terminate 想成「一種很嚴重的例外」。它根本不是
//         例外 —— 它是例外機制【放棄之後】的最終手段。
// ═══════════════════════════════════════════════════════════════════════════

/*
# 第三階段：執行緒生命週期管理

## 課程 3.1：RAII 與執行緒管理

---

### 引言

RAII（Resource Acquisition Is Initialization）是 C++ 最重要的資源管理慣用法。將它應用於執行緒管理，可以避免忘記 join 或 detach 導致的程式崩潰。

---

### 一、問題：例外導致的資源洩漏

```cpp
#include <iostream>
#include <thread>

void riskyFunction() {
    std::thread t([]() {
        std::cout << "工作中" << std::endl;
    });
    
    // 如果這裡拋出例外...
    throw std::runtime_error("發生錯誤！");
    
    t.join();  // 永遠不會執行！
}  // t 解構時仍是 joinable → std::terminate()

int main() {
    try {
        riskyFunction();
    } catch (...) {
        std::cout << "捕獲例外" << std::endl;
    }
    return 0;
}
```

這段程式會崩潰，因為例外導致 `join()` 被跳過。

---

### 二、RAII 的核心概念

```
┌─────────────────────────────────────────────────────────────┐
│                    RAII 原則                                │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  建構函式：獲取資源                                          │
│  解構函式：釋放資源                                          │
│                                                             │
│  優點：                                                      │
│  • 無論正常返回或例外，解構函式都會被呼叫                     │
│  • 資源自動管理，不會忘記釋放                                │
│  • 程式碼更簡潔、更安全                                      │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 三、簡單的 RAII 執行緒包裝

```cpp
#include <iostream>
#include <thread>

class ThreadGuard {
    std::thread& t;
public:
    explicit ThreadGuard(std::thread& thread) : t(thread) {}
    
    ~ThreadGuard() {
        if (t.joinable()) {
            t.join();
        }
    }
    
    // 禁止複製
    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;
};

int main() {
    std::thread t([]() {
        std::cout << "工作中" << std::endl;
    });
    
    ThreadGuard guard(t);  // 保證 t 會被 join
    
    // 即使這裡拋出例外，guard 的解構函式仍會執行
    // throw std::runtime_error("測試");
    
    return 0;
}  // guard 解構 → 自動 join
```

---

### 四、擁有執行緒的 RAII 類別

上面的 `ThreadGuard` 只持有引用。更好的設計是擁有執行緒：

```cpp
#include <iostream>
#include <thread>

class ScopedThread {
    std::thread t;
public:
    explicit ScopedThread(std::thread thread) 
        : t(std::move(thread)) 
    {
        if (!t.joinable()) {
            throw std::logic_error("No thread");
        }
    }
    
    ~ScopedThread() {
        t.join();  // 一定是 joinable，不用檢查
    }
    
    // 禁止複製
    ScopedThread(const ScopedThread&) = delete;
    ScopedThread& operator=(const ScopedThread&) = delete;
};

int main() {
    ScopedThread st(std::thread([]() {
        std::cout << "安全的執行緒" << std::endl;
    }));
    
    // 自動管理，不需要手動 join
    return 0;
}
```

---

### 五、選擇 join 或 detach 的 RAII

```cpp
#include <iostream>
#include <thread>

class FlexibleThread {
public:
    enum class Action { join, detach };
    
private:
    std::thread t;
    Action action;
    
public:
    FlexibleThread(std::thread thread, Action a) 
        : t(std::move(thread)), action(a) {}
    
    ~FlexibleThread() {
        if (t.joinable()) {
            if (action == Action::join) {
                t.join();
            } else {
                t.detach();
            }
        }
    }
    
    FlexibleThread(const FlexibleThread&) = delete;
    FlexibleThread& operator=(const FlexibleThread&) = delete;
};

int main() {
    FlexibleThread ft1(
        std::thread([]() { std::cout << "Join 我" << std::endl; }),
        FlexibleThread::Action::join
    );
    
    FlexibleThread ft2(
        std::thread([]() { std::cout << "Detach 我" << std::endl; }),
        FlexibleThread::Action::detach
    );
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return 0;
}
```

---

### 六、RAII vs 手動管理對比

```cpp
// ❌ 手動管理：容易出錯
void manual() {
    std::thread t(work);
    
    if (condition1) {
        t.join();
        return;       // 要記得 join
    }
    
    if (condition2) {
        t.join();
        throw error;  // 要記得 join
    }
    
    t.join();         // 正常路徑也要 join
}

// ✅ RAII：自動管理
void automatic() {
    ScopedThread st(std::thread(work));
    
    if (condition1) {
        return;       // 自動 join
    }
    
    if (condition2) {
        throw error;  // 自動 join
    }
    
    // 自動 join
}
```

---

### 七、本課重點回顧

1. **RAII** 利用解構函式自動釋放資源
2. 解構函式在正常返回和例外時都會執行
3. `ThreadGuard` 持有引用，保證執行緒被 join
4. `ScopedThread` 擁有執行緒，更安全
5. RAII 讓程式碼更簡潔，避免遺漏 join/detach
6. C++20 的 `std::jthread` 就是標準化的 RAII 執行緒

---

### 下一課預告

在 **課程 3.2：執行緒守衛類別設計** 中，我們將：
- 完善 ThreadGuard 的設計
- 處理更多邊界情況
- 加入移動語意支援

---

準備好繼續嗎？
*/



// ⚠️ 本檔更正：原版這一段是直接從上面課文第六節貼下來的【虛擬碼】,
//    work / condition1 / condition2 / error / ScopedThread 全都沒有宣告,
//    所以整個檔案編譯失敗。已補齊必要的宣告與 main(),讓它可以真的編譯執行。

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

void work() {
    std::cout << "  [執行緒] 工作中" << std::endl;
}

// 擁有執行緒的 RAII 包裝（同課文第四節）
class ScopedThread {
    std::thread t;
public:
    explicit ScopedThread(std::thread thread)
        : t(std::move(thread))
    {
        if (!t.joinable()) {
            throw std::logic_error("No thread");
        }
    }

    ~ScopedThread() {
        t.join();  // 一定是 joinable，不用檢查
    }

    ScopedThread(const ScopedThread&) = delete;
    ScopedThread& operator=(const ScopedThread&) = delete;
};

// ❌ 手動管理：容易出錯
void manual(bool condition1, bool condition2) {
    std::thread t(work);

    if (condition1) {
        t.join();
        return;       // 要記得 join
    }

    if (condition2) {
        // 💀 這裡【故意忘記】join —— 這正是手動管理的典型出錯點。
        //    t 解構時仍是 joinable → std::terminate()（標準保證,不是隨機現象）
        throw std::runtime_error("發生錯誤！");
    }

    t.join();         // 正常路徑也要 join
}

// ✅ RAII：自動管理
void automatic(bool condition1, bool condition2) {
    // ⚠️ 這裡必須用大括號 {}：若寫成 ScopedThread st(std::thread(work));
    //    會觸發 most vexing parse —— 編譯器把整行當成「函式宣告」,
    //    根本不會建立物件、也不會啟動執行緒（示範會靜悄悄失效）。
    ScopedThread st{std::thread(work)};

    if (condition1) {
        return;       // 自動 join
    }

    if (condition2) {
        throw std::runtime_error("發生錯誤！");  // 自動 join
    }

    // 自動 join
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔【刻意不提供】
//
//   說明:LeetCode 現有的多執行緒題(1114 Print in Order、1115 Print FooBar
//   Alternately、1116 Print Zero Even Odd、1117 Building H2O、
//   1195 Fizz Buzz Multithreaded)考的是【執行緒之間的順序協調】,
//   解法核心是 condition_variable / semaphore / atomic。執行緒的建立與
//   回收由評測框架負責,作答者拿不到 std::thread 物件,因此 RAII 守衛、
//   join/detach、terminate 這些本課主題在那些題目裡完全不會出現。
//   硬湊一題只會建立錯誤的關聯,故從缺;改以貼近生產環境的實務範例呈現。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】服務啟動時的連線池預熱(connection pool warm-up)
//
//   情境:服務啟動時要先開好 N 條資料庫連線,避免第一批請求都吃到
//         建立連線的延遲。為了縮短啟動時間,N 條連線【平行】建立。
//
//   關鍵風險:如果其中一條連線失敗(帳密錯誤、DB 尚未就緒),啟動流程
//         要中止並回報錯誤。但此時還有其他 worker 正在跑 ——
//         若直接 throw,那些 joinable 的 std::thread 會在 stack unwinding
//         時被解構 → std::terminate() → 服務不是「啟動失敗」,而是
//         「整個行程消失」,連錯誤訊息都來不及寫進 log。
//
//   RAII 解法:用一個 guard 在作用域結束時無條件 join 全部 worker。
//         無論成功、失敗、或中途丟例外,都不會有 joinable 的 thread 被解構,
//         錯誤才能以例外的形式正常往上傳遞給啟動流程。
// -----------------------------------------------------------------------------
class JoinAllGuard {
    std::vector<std::thread>& workers;

public:
    explicit JoinAllGuard(std::vector<std::thread>& w) : workers(w) {}

    ~JoinAllGuard() {
        for (auto& t : workers) {
            if (t.joinable()) t.join();
        }
    }

    JoinAllGuard(const JoinAllGuard&) = delete;
    JoinAllGuard& operator=(const JoinAllGuard&) = delete;
};

struct PoolConfig {
    int         size;
    std::string badCredentialAt;  // 空字串代表全部正常
};

// 回傳成功建立的連線數;任一條失敗則丟例外(但保證所有 worker 已 join)
int warmUpConnectionPool(const PoolConfig& cfg) {
    std::vector<std::thread> workers;
    std::atomic<int>         opened{0};
    std::atomic<bool>        failed{false};
    std::string              failedName;
    std::mutex               nameMutex;

    workers.reserve(static_cast<std::size_t>(cfg.size));

    {
        // guard 宣告在 workers 之後 → 先解構 → 保證全部 join
        JoinAllGuard guard(workers);

        for (int i = 0; i < cfg.size; ++i) {
            workers.emplace_back([&, i]() {
                std::string name = "conn-" + std::to_string(i);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));

                if (name == cfg.badCredentialAt) {
                    failed.store(true, std::memory_order_relaxed);
                    std::lock_guard<std::mutex> lk(nameMutex);
                    failedName = name;
                    return;
                }
                opened.fetch_add(1, std::memory_order_relaxed);
            });
        }
    }  // guard 解構 → 全部 worker 已 join,以下讀取共享狀態沒有 data race

    if (failed.load(std::memory_order_relaxed)) {
        throw std::runtime_error("連線建立失敗: " + failedName);
    }
    return opened.load(std::memory_order_relaxed);
}

int main() {
    std::cout << "1) manual(condition1=true)：記得 join,正常結束" << std::endl;
    manual(true, false);

    std::cout << "2) automatic(condition1=true)：解構自動 join" << std::endl;
    automatic(true, false);

    std::cout << "3) automatic(condition2=true)：拋例外,解構仍自動 join" << std::endl;
    try {
        automatic(false, true);
    } catch (const std::exception& e) {
        std::cout << "   捕獲例外: " << e.what() << "（執行緒已被安全 join）" << std::endl;
    }

    std::cout << "4) manual(condition2=true)：忘記 join → std::terminate()" << std::endl;
#ifdef DEMONSTRATE_UB
    // ⚠️ 這個呼叫【一定】會讓程式 abort（std::terminate）,而且無法被 catch。
    //    這是 C++ 標準保證的行為,不是未定義行為、也不是隨機當機。
    //    觀察方式：
    //      g++ -std=c++17 -pthread -DDEMONSTRATE_UB -o raii5 '課程 3.1：RAII 與執行緒管理5.cpp'
    //      ./raii5   → 預期 terminate,exit code 134 (SIGABRT)
    try {
        manual(false, true);
    } catch (const std::exception&) {
        std::cout << "   （這行永遠不會被執行）" << std::endl;
    }
#else
    std::cout << "   已略過（預設不執行,以免整個程式 abort）。" << std::endl;
    std::cout << "   要親眼看到 terminate,請加上 -DDEMONSTRATE_UB 重新編譯。" << std::endl;
#endif

    std::cout << "\n5) 日常實務：連線池預熱（全部成功）" << std::endl;
    std::cout << "   已建立連線數 = " << warmUpConnectionPool({4, ""}) << " / 4" << std::endl;

    std::cout << "\n6) 日常實務：連線池預熱（第 3 條憑證錯誤）" << std::endl;
    try {
        int n = warmUpConnectionPool({4, "conn-2"});
        std::cout << "   已建立連線數 = " << n << std::endl;
    } catch (const std::exception& e) {
        std::cout << "   啟動中止: " << e.what() << std::endl;
        std::cout << "   所有 worker 已被 guard join,行程存活,錯誤可正常上報" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.1：RAII 與執行緒管理5.cpp" -o raii5
// 觀察 terminate（程式會 abort,exit code 134）:
//   g++ -std=c++17 -Wall -Wextra -pthread -DDEMONSTRATE_UB "課程 3.1：RAII 與執行緒管理5.cpp" -o raii5_ub

// === 預期輸出 ===
// 1) manual(condition1=true)：記得 join,正常結束
//   [執行緒] 工作中
// 2) automatic(condition1=true)：解構自動 join
//   [執行緒] 工作中
// 3) automatic(condition2=true)：拋例外,解構仍自動 join
//   [執行緒] 工作中
//    捕獲例外: 發生錯誤！（執行緒已被安全 join）
// 4) manual(condition2=true)：忘記 join → std::terminate()
//    已略過（預設不執行,以免整個程式 abort）。
//    要親眼看到 terminate,請加上 -DDEMONSTRATE_UB 重新編譯。
//
// 5) 日常實務：連線池預熱（全部成功）
//    已建立連線數 = 4 / 4
//
// 6) 日常實務：連線池預熱（第 3 條憑證錯誤）
//    啟動中止: 連線建立失敗: conn-2
//    所有 worker 已被 guard join,行程存活,錯誤可正常上報
