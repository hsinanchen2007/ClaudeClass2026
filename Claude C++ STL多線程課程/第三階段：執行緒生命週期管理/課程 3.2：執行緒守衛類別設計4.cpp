// =============================================================================
//  課程 3.2：執行緒守衛類別設計 4  —  課文總整理 + 用樣板參數做編譯期策略
// =============================================================================
//
// 【主題資訊 Information】
//   本檔是課程 3.2 的完整課文,可執行的部分示範【編譯期策略選擇】:
//
//       enum class ThreadAction { Join, Detach };
//
//       template<ThreadAction action>
//       class ManagedThread {
//           std::thread t;
//       public:
//           template<typename Func, typename... Args>
//           explicit ManagedThread(Func&& f, Args&&... args);
//           ~ManagedThread() {
//               if (t.joinable()) {
//                   if constexpr (action == ThreadAction::Join) t.join();
//                   else                                        t.detach();
//               }
//           }
//       };
//       using AutoJoinThread   = ManagedThread<ThreadAction::Join>;
//       using AutoDetachThread = ManagedThread<ThreadAction::Detach>;
//
//   標準版本：【C++17】—— 因為用了 if constexpr。
//             已用 -pedantic-errors 實測驗證:-std=c++11 與 -std=c++14 都會
//             被拒絕,錯誤訊息為「'if constexpr' only available with
//             '-std=c++17'」;只有 -std=c++17 通過。
//             (若要相容 C++11/14,需改用樣板特化或 tag dispatch。)
//   標頭檔  ：<thread>、<utility>
//   複雜度  ：解構時 Join 策略阻塞至執行緒結束;Detach 策略 O(1)
//
// 【詳細解釋 Explanation】
//
// 【1. 編譯期策略 vs 執行期策略:與 FlexibleThread 的關鍵差異】
//   回想「RAII 與執行緒管理4」的 FlexibleThread:
//       FlexibleThread ft(std::thread(f), FlexibleThread::Action::join);
//   它把策略存成一個【成員變數】,解構時用 if 判斷 —— 這是【執行期】決定。
//
//   ManagedThread 把策略提升成【樣板參數】:
//       AutoJoinThread jt(f);     // 策略編碼在型別裡
//   差別有三:
//     (a) 不需要儲存 action 成員 → 物件更小(只剩一個 std::thread)
//     (b) 解構時沒有分支 → 編譯器直接生成 join() 或 detach(),零執行期成本
//     (c) 【策略成為型別的一部分】→ AutoJoinThread 與 AutoDetachThread 是
//         兩個不同的型別,不能互相賦值,也不能放進同一個 vector。
//
//   (c) 是雙面刃:型別安全變強了(不可能把 join 版誤傳給要 detach 版的地方),
//   但彈性沒了(無法在執行期依設定檔決定策略)。這正是泛型程式設計裡
//   最典型的取捨 —— 沒有標準答案,取決於策略是否在編譯期就已確定。
//
// 【2. if constexpr 到底做了什麼(C++17)】
//   一般的 if 在執行期求值,而且【兩個分支都必須能編譯】。
//   if constexpr 在編譯期求值,而且【被捨棄的分支不會被實例化】。
//
//   對本檔而言,ManagedThread<Join> 實例化後,解構函式裡只會存在 t.join(),
//   else 分支的 t.detach() 根本不會被編譯進去 —— 不是「被最佳化掉」,
//   而是【從未存在】。這個區別在分支中有「對某些型別無效的操作」時
//   至關重要:
//       if constexpr (std::is_pointer_v<T>) return *v;  // 非指標時這行不存在
//       else                                return v;
//   若寫成一般的 if,非指標型別在 *v 那裡就會編譯失敗。
//
//   ⚠️ 注意:if constexpr 的條件必須是【編譯期常數運算式】。
//   本檔的 action 是樣板參數,天生就是編譯期常數,所以合法。
//
// 【3. 為什麼要提供型別別名?】
//       using AutoJoinThread = ManagedThread<ThreadAction::Join>;
//   直接寫 ManagedThread<ThreadAction::Join> 又長又難讀,而且在函式簽名、
//   容器宣告裡會重複出現。型別別名讓意圖一目了然,也讓日後要換實作時
//   只需改一行(例如把 AutoJoinThread 改成 std::jthread 的別名)。
//   這是「用型別表達意圖」的實踐。
//
// 【4. ⚠️ 這個類別繼承了「執行緒守衛類別設計3」指出的同一個 bug】
//       ManagedThread& operator=(ManagedThread&&) = default;
//   = default 會逐成員移動,對 std::thread 成員就是呼叫
//   std::thread::operator=(thread&&);而標準規定:【賦值目標若仍 joinable
//   → std::terminate()】。所以
//       AutoJoinThread a(f1), b(f2);
//       a = std::move(b);        // ← a 還 joinable → std::terminate()
//   會讓程式終止 —— 標準保證的確定結果,不是未定義行為,catch 攔不到。
//   (本機已在「執行緒守衛類別設計3」實測:exit code 134、
//    訊息 "terminate called without an active exception"。)
//
//   正確做法是顯式實作,先按自己的策略處置舊資源:
//       ManagedThread& operator=(ManagedThread&& o) noexcept {
//           if (this != &o) {
//               if (t.joinable()) {
//                   if constexpr (action == ThreadAction::Join) t.join();
//                   else                                        t.detach();
//               }
//               t = std::move(o.t);
//           }
//           return *this;
//       }
//   本檔保留課文原貌,但在示範 3 以 -DDEMONSTRATE_UB 隔離可重現的路徑。
//
// 【5. 這一課的演進脈絡(整個 3.1 + 3.2 的總結)】
//       ThreadGuard      持有參考,不擁有         → 宣告順序即正確性
//       ScopedThread     持有值,建構期建立不變式 → 解構可以無條件 join
//       FlexibleThread   執行期策略               → 彈性,但多一個成員
//       JoiningThread    加上移動語意             → 終於能進容器
//       ManagedThread    編譯期策略               → 零成本,但型別分裂
//       std::jthread     標準答案(C++20)        → 自動 join + 協作取消
//   每一步都在回答前一步留下的問題。理解這條線,比背下任何一個類別都重要。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 ManagedThread<Join> 和 ManagedThread<Detach> 是完全不同的型別?
//   樣板的每一組實參都會實例化出一個獨立的類別。它們沒有共同基底,
//   彼此之間沒有隱式轉換。所以你不能寫
//       std::vector<ManagedThread<...>> v;   // 必須指定一個具體策略
//   若真的需要在同一容器裡混放不同策略,得用型別抹除(type erasure),
//   例如 std::vector<std::function<void()>> 或自訂的多型包裝 —— 那就
//   把編譯期省下的成本又加回來了。
//
// (B) if constexpr 與 SFINAE、tag dispatch 的關係
//   在 C++17 之前,「依型別選擇不同實作」要靠:
//     * 樣板特化:寫兩個版本的類別/函式,程式碼重複
//     * tag dispatch:用多載 + 標籤型別引導,樣板寫法繁瑣
//     * SFINAE(enable_if):條件複雜時可讀性極差
//   if constexpr 讓這件事變成一個看起來像普通 if 的敘述,是 C++17 對
//   泛型程式設計可讀性最大的改善之一。C++20 的 concepts 又更進一步。
//
// (C) 為什麼樣板參數可以是 enum class 的值?
//   非型別樣板參數(non-type template parameter)允許整數型別、
//   列舉型別、指標/參考、以及 std::nullptr_t。enum class 屬於列舉型別,
//   所以完全合法,而且比用 int 或 bool 當樣板參數更有型別安全:
//   ManagedThread<0> 沒有意義,ManagedThread<ThreadAction::Join> 一看就懂。
//
// (D) 零成本抽象(zero-cost abstraction)在這裡的具體體現
//   ManagedThread<Join> 的 sizeof 等於 sizeof(std::thread)(本機實測 8),
//   沒有多出任何策略欄位;解構函式裡也沒有任何分支判斷。
//   換句話說,這層抽象在執行期【完全免費】—— 這正是 C++ 樣板相對於
//   虛擬函式(需要 vptr + 間接呼叫)的優勢所在。
//
// 【注意事項 Pay Attention】
//   1. 本檔需要 C++17(if constexpr);已用 -pedantic-errors 驗證
//      c++11 / c++14 皆被拒絕。
//   2. operator=(ManagedThread&&) = default 在賦值目標仍 joinable 時會
//      呼叫 std::terminate()(標準保證);示範 3 以 -DDEMONSTRATE_UB 隔離。
//   3. AutoJoinThread 與 AutoDetachThread 是不同型別,不可互相賦值,
//      也不能放進同一個容器。
//   4. AutoDetachThread 承襲 detach 的所有風險:沒有完成保證,
//      且不可捕捉區域變數的參考(詳見「執行緒守衛類別設計3」)。
//   5. 課文第六節的範例原本缺少 <utility>;完美轉發用到 std::forward,
//      應顯式含入,本檔已補。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】編譯期策略與 if constexpr
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. if constexpr 和一般的 if 差在哪裡?
//     答：一般的 if 在執行期求值,而且兩個分支都必須能通過編譯;
//         if constexpr 在編譯期求值,【被捨棄的分支不會被實例化】——
//         那段程式碼從未存在,而不是被最佳化掉。因此可以在分支裡寫
//         「對某些型別根本無效」的操作。它是 C++17 加入的。
//     追問：怎麼確認一個語法是不是某個標準版本才有的?
//         → 用 -pedantic-errors 編譯,不能只用 -fsyntax-only。GCC 預設
//           會把許多新標準特性當成擴充放行(只給 warning),看起來像
//           舊標準也支援。本檔實測:-std=c++14 -pedantic-errors 會明確
//           報錯「'if constexpr' only available with '-std=c++17'」。
//
// 🔥 Q2. 把策略做成樣板參數,比做成建構函式參數好在哪?差在哪?
//     答：好處是零執行期成本 —— 不需要儲存策略欄位(物件更小),
//         解構時也沒有分支;而且策略成為型別的一部分,誤用會在編譯期
//         就被擋下。代價是彈性:策略必須在編譯期確定,無法依設定檔或
//         使用者輸入決定,而且不同策略是不同型別,不能放進同一個容器。
//     追問：什麼時候該選執行期策略?
//         → 當策略來自程式外部(設定檔、命令列、環境變數),或需要把
//           不同策略的物件放進同一個集合時。這時 FlexibleThread 那種
//           存成員變數的做法才是對的。
//
// ⚠️ 陷阱 1. 「= default 的移動賦值最安全,反正編譯器最清楚該怎麼做。」
//     答：對持有 std::thread 的類別而言,= default 會逐成員移動,
//         而 std::thread 的移動賦值在目標仍 joinable 時是 std::terminate()。
//         編譯器產生的版本【不會】套用你的 join/detach 策略,它只是
//         機械地逐成員移動。必須顯式實作才能先處置舊資源。
//     為什麼會錯：把 = default 想成「編譯器會做正確的事」。它只做
//         「預設的事」—— 而預設對持有特殊資源的型別往往是錯的。
//         這正是 Rule of Five 存在的理由。
//
// ⚠️ 陷阱 2. 「ManagedThread<Join> 和 ManagedThread<Detach> 都是
//              ManagedThread,應該可以放進同一個 vector 吧?」
//     答：不行。樣板的每一組實參都實例化出獨立的類別,兩者之間沒有
//         共同基底、沒有隱式轉換,是徹底不相干的兩個型別。
//         要混放必須做型別抹除(type erasure),而那會把編譯期省下的
//         成本再加回來。
//     為什麼會錯：把樣板名稱當成型別。ManagedThread 本身不是型別,
//         它是「產生型別的樣板」;只有 ManagedThread<X> 才是型別。
// ═══════════════════════════════════════════════════════════════════════════

/*
# 第三階段：執行緒生命週期管理

## 課程 3.2：執行緒守衛類別設計

---

### 引言

上一課介紹了基本的 RAII 執行緒包裝。本課將設計一個更完善、更實用的執行緒守衛類別。

---

### 一、設計目標

```
┌─────────────────────────────────────────────────────────────┐
│              理想的執行緒守衛應具備                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ✓ 自動在解構時 join 或 detach                              │
│  ✓ 支援移動語意                                             │
│  ✓ 禁止複製                                                 │
│  ✓ 可以手動提前釋放                                         │
│  ✓ 可以取得底層執行緒的資訊                                  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 二、完整的 JoiningThread 類別

```cpp
#include <iostream>
#include <thread>
#include <utility>

class JoiningThread {
    std::thread t;
    
public:
    // 預設建構
    JoiningThread() noexcept = default;
    
    // 從 std::thread 移動建構
    explicit JoiningThread(std::thread thread) noexcept
        : t(std::move(thread)) {}
    
    // 直接建構執行緒（完美轉發）
    template<typename Func, typename... Args>
    explicit JoiningThread(Func&& f, Args&&... args)
        : t(std::forward<Func>(f), std::forward<Args>(args)...) {}
    
    // 移動建構
    JoiningThread(JoiningThread&& other) noexcept
        : t(std::move(other.t)) {}
    
    // 移動賦值
    JoiningThread& operator=(JoiningThread&& other) noexcept {
        if (this != &other) {
            if (t.joinable()) {
                t.join();  // 先處理當前執行緒
            }
            t = std::move(other.t);
        }
        return *this;
    }
    
    // 禁止複製
    JoiningThread(const JoiningThread&) = delete;
    JoiningThread& operator=(const JoiningThread&) = delete;
    
    // 解構：自動 join
    ~JoiningThread() {
        if (t.joinable()) {
            t.join();
        }
    }
    
    // 工具方法
    bool joinable() const noexcept { return t.joinable(); }
    std::thread::id get_id() const noexcept { return t.get_id(); }
    
    void join() {
        t.join();
    }
    
    void detach() {
        t.detach();
    }
    
    std::thread& get() noexcept { return t; }
};
```

---

### 三、使用範例

```cpp
#include <iostream>
#include <vector>

int main() {
    // 方式一：直接建構
    JoiningThread jt1([]() {
        std::cout << "執行緒 1" << std::endl;
    });
    
    // 方式二：從 std::thread 移動
    std::thread t([]() {
        std::cout << "執行緒 2" << std::endl;
    });
    JoiningThread jt2(std::move(t));
    
    // 方式三：帶參數
    JoiningThread jt3([](int x) {
        std::cout << "執行緒 3: " << x << std::endl;
    }, 42);
    
    // 不需要手動 join！
    return 0;
}
```

輸出：
```
執行緒 1
執行緒 2
執行緒 3: 42
```

---

### 四、在容器中使用

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<JoiningThread> threads;
    
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([i]() {
            std::cout << "Worker " << i << std::endl;
        });
    }
    
    std::cout << "所有執行緒已建立" << std::endl;
    
    // 離開作用域時，vector 解構
    // 每個 JoiningThread 解構時自動 join
    return 0;
}
```

---

### 五、DetachingThread 變體

如果預設行為是 detach：

```cpp
class DetachingThread {
    std::thread t;
    
public:
    template<typename Func, typename... Args>
    explicit DetachingThread(Func&& f, Args&&... args)
        : t(std::forward<Func>(f), std::forward<Args>(args)...) {}
    
    DetachingThread(DetachingThread&&) = default;
    DetachingThread& operator=(DetachingThread&&) = default;
    
    DetachingThread(const DetachingThread&) = delete;
    DetachingThread& operator=(const DetachingThread&) = delete;
    
    ~DetachingThread() {
        if (t.joinable()) {
            t.detach();
        }
    }
};
```

---

### 六、策略模式：可選擇的行為

```cpp
#include <iostream>
#include <thread>

enum class ThreadAction { Join, Detach };

template<ThreadAction action>
class ManagedThread {
    std::thread t;
    
public:
    template<typename Func, typename... Args>
    explicit ManagedThread(Func&& f, Args&&... args)
        : t(std::forward<Func>(f), std::forward<Args>(args)...) {}
    
    ManagedThread(ManagedThread&&) = default;
    ManagedThread& operator=(ManagedThread&&) = default;
    
    ManagedThread(const ManagedThread&) = delete;
    ManagedThread& operator=(const ManagedThread&) = delete;
    
    ~ManagedThread() {
        if (t.joinable()) {
            if constexpr (action == ThreadAction::Join) {
                t.join();
            } else {
                t.detach();
            }
        }
    }
};

// 型別別名
using AutoJoinThread = ManagedThread<ThreadAction::Join>;
using AutoDetachThread = ManagedThread<ThreadAction::Detach>;

int main() {
    AutoJoinThread jt([]() {
        std::cout << "會被 join" << std::endl;
    });
    
    return 0;
}
```

---

### 七、與 std::jthread 的比較

```
┌────────────────────────────────────────────────────────────┐
│          自訂 JoiningThread vs std::jthread               │
├────────────────────────────────────────────────────────────┤
│                                                            │
│  JoiningThread (自訂)           std::jthread (C++20)      │
│  ─────────────────────         ─────────────────────      │
│  • 可用於 C++11/14/17          • 需要 C++20               │
│  • 可自訂行為                   • 標準化，跨平台一致        │
│  • 需要自己維護                 • 內建支援取消機制          │
│                                • 經過充分測試              │
│                                                            │
│  建議：C++20 可用時，優先使用 std::jthread                 │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

---

### 八、本課重點回顧

1. 完善的執行緒守衛需要支援**移動語意**
2. 移動賦值前要先處理當前的 joinable 執行緒
3. 使用**完美轉發**支援直接建構執行緒
4. 可用模板參數在編譯期選擇 join 或 detach 行為
5. 執行緒守衛可以安全地放入容器
6. C++20 有標準的 `std::jthread`，功能更完整

---

### 下一課預告

在 **課程 3.3：std::jthread (C++20)** 中，我們將學習：
- 標準化的自動 join 執行緒
- 內建的取消機制（stop_token）
- 為什麼它比自訂類別更好

---

準備好繼續嗎？
*/



#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

enum class ThreadAction { Join, Detach };

template<ThreadAction action>
class ManagedThread {
    std::thread t;
    
public:
    template<typename Func, typename... Args>
    explicit ManagedThread(Func&& f, Args&&... args)
        : t(std::forward<Func>(f), std::forward<Args>(args)...) {}
    
    ManagedThread(ManagedThread&&) = default;
    ManagedThread& operator=(ManagedThread&&) = default;
    
    ManagedThread(const ManagedThread&) = delete;
    ManagedThread& operator=(const ManagedThread&) = delete;
    
    ~ManagedThread() {
        if (t.joinable()) {
            if constexpr (action == ThreadAction::Join) {
                t.join();
            } else {
                t.detach();
            }
        }
    }
};

// 型別別名
using AutoJoinThread = ManagedThread<ThreadAction::Join>;
using AutoDetachThread = ManagedThread<ThreadAction::Detach>;

// -----------------------------------------------------------------------------
// 示範 1:兩種策略,零執行期成本
// -----------------------------------------------------------------------------
void demoBothPolicies() {
    {
        AutoJoinThread jt([]() { std::cout << "  [Join 策略] 會被 join\n"; });
    }  // 解構 → if constexpr 選中 t.join(),阻塞等待
    std::cout << "  [main] Join 策略的執行緒已確定結束\n";

    {
        AutoDetachThread dt([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        });
    }  // 解構 → if constexpr 選中 t.detach(),立即返回
    std::cout << "  [main] Detach 策略立即返回(無完成保證)\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
}

// -----------------------------------------------------------------------------
// 示範 2:零成本抽象的實證 —— 策略沒有佔用任何空間
// -----------------------------------------------------------------------------
void demoZeroCost() {
    std::cout << "  sizeof(std::thread)      = " << sizeof(std::thread) << "\n";
    std::cout << "  sizeof(AutoJoinThread)   = " << sizeof(AutoJoinThread) << "\n";
    std::cout << "  sizeof(AutoDetachThread) = " << sizeof(AutoDetachThread) << "\n";
    std::cout << "  策略編碼在型別裡,不佔任何執行期空間: " << std::boolalpha
              << (sizeof(AutoJoinThread) == sizeof(std::thread)) << "\n";
    std::cout << "  (以上為本機實作定義的值,不同平台可能不同)\n";

    std::cout << "  兩種策略是不同型別: " << std::boolalpha
              << !std::is_same<AutoJoinThread, AutoDetachThread>::value << "\n";
}

// -----------------------------------------------------------------------------
// 示範 3:= default 移動賦值的 std::terminate() —— 預設不執行
//
//   觀察方式:
//     g++ -std=c++17 -Wall -Wextra -pthread -DDEMONSTRATE_UB "課程 3.2：執行緒守衛類別設計4.cpp" -o guard4_ub
//     ./guard4_ub
//   預期:terminate called without an active exception,exit code 134
// -----------------------------------------------------------------------------
void demoMoveAssignTerminate() {
#ifdef DEMONSTRATE_UB
    AutoJoinThread a([]() { std::this_thread::sleep_for(std::chrono::milliseconds(50)); });
    AutoJoinThread b([]() { std::this_thread::sleep_for(std::chrono::milliseconds(50)); });

    std::cout << "  即將執行 a = std::move(b) …\n" << std::flush;
    a = std::move(b);   // ← 此處 std::terminate()
    std::cout << "  (這行永遠不會被印出)\n";
#else
    std::cout << "  已略過(預設不執行,以免整個程式 abort)。\n";
    std::cout << "  要親眼看到 terminate,請加上 -DDEMONSTRATE_UB 重新編譯。\n";
#endif
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔【刻意不提供】
//
//   說明:LeetCode 的多執行緒題(1114 / 1115 / 1116 / 1117 / 1195)由評測
//   框架建立與回收執行緒,考的是順序協調(condition_variable / semaphore /
//   atomic),完全碰不到守衛類別、樣板策略或 if constexpr。
//   本檔主題是【泛型資源管理類別的設計】,與之無交集,故從缺。
//   (LeetCode 1114 的真實解法已在「執行緒守衛類別設計2」示範,
//    該處以 std::vector<JoiningThread> 驅動三條執行緒,與該檔主題相符。)
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】固定大小的 thread pool:用編譯期策略保證關閉語意
//
//   情境:服務內部的工作佇列 + 固定 N 條 worker。關閉時必須【等所有
//         worker 收工】—— 若有 worker 還在寫資料庫就被放生,會造成
//         資料不一致。
//
//   為何用編譯期策略:pool 的關閉語意是設計決策,在編譯期就已確定,
//         絕不該讓執行期的任何條件改變它。把它編碼成 AutoJoinThread,
//         等於用型別系統把「這裡不可以 detach」寫死 ——
//         誰想改成 detach,就得改型別,而那在 code review 時看得見。
//
//   關閉流程(graceful shutdown)的標準三步:
//     1. 設定停止旗標
//     2. notify_all 喚醒所有等待中的 worker
//     3. 等待 worker 逐一退出(這裡由 AutoJoinThread 的解構函式完成)
//   缺少任何一步都會造成 worker 永遠卡在 wait 上 → 關閉時卡死。
// -----------------------------------------------------------------------------
class ThreadPool {
    std::queue<std::function<void()>> tasks;
    std::mutex                        m;
    std::condition_variable           cv;
    bool                              stopping = false;
    std::atomic<int>                  completed{0};
    // ⚠️ workers 必須宣告在【最後】:成員反向解構,worker 先被 join,
    //    此時 tasks / m / cv 都還活著,worker 才能安全地收尾。
    std::vector<AutoJoinThread>       workers;

public:
    explicit ThreadPool(unsigned n) {
        workers.reserve(n);
        for (unsigned i = 0; i < n; ++i) {
            workers.emplace_back([this]() {
                for (;;) {
                    std::function<void()> job;
                    {
                        std::unique_lock<std::mutex> lk(m);
                        cv.wait(lk, [this] { return stopping || !tasks.empty(); });
                        if (stopping && tasks.empty()) return;  // 唯一的退出點
                        job = std::move(tasks.front());
                        tasks.pop();
                    }
                    job();
                    completed.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }
    }

    void submit(std::function<void()> job) {
        {
            std::lock_guard<std::mutex> lk(m);
            tasks.push(std::move(job));
        }
        cv.notify_one();
    }

    // 明確的關閉流程:設旗標 → 喚醒全部 → (解構時)join 全部
    void shutdown() {
        {
            std::lock_guard<std::mutex> lk(m);
            stopping = true;
        }
        cv.notify_all();
        workers.clear();  // 逐一解構 → AutoJoinThread 各自 join
    }

    int completedCount() const { return completed.load(std::memory_order_relaxed); }

    ~ThreadPool() {
        if (!workers.empty()) shutdown();
    }
};

int main() {
    std::cout << "=== 示範 1:Join / Detach 兩種編譯期策略 ===\n";
    demoBothPolicies();

    std::cout << "\n=== 示範 2:零成本抽象的實證 ===\n";
    demoZeroCost();

    std::cout << "\n=== 示範 3:= default 移動賦值會 std::terminate() ===\n";
    demoMoveAssignTerminate();

    std::cout << "\n=== 日常實務:固定大小 thread pool 的優雅關閉 ===\n";
    {
        ThreadPool pool(4);
        std::atomic<long> sum{0};

        for (int i = 1; i <= 100; ++i) {
            pool.submit([i, &sum]() { sum.fetch_add(i, std::memory_order_relaxed); });
        }

        pool.shutdown();  // 設旗標 → 喚醒 → join 全部 worker

        std::cout << "  已提交 100 個任務,完成數 = " << pool.completedCount() << "\n";
        std::cout << "  1..100 累加結果 = " << sum.load()
                  << "(正確答案 5050: " << std::boolalpha
                  << (sum.load() == 5050) << ")\n";
        std::cout << "  關閉後才讀取結果,無 data race —— join 建立了 happens-before\n";
    }

    std::cout << "\n=== 全部示範結束 ===\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.2：執行緒守衛類別設計4.cpp" -o guard4
//   ⚠️ 必須是 c++17:本檔使用 if constexpr,已用 -pedantic-errors 驗證
//      c++11 / c++14 皆會被拒絕。
// 觀察移動賦值的 terminate(程式會 abort,exit code 134):
//   g++ -std=c++17 -Wall -Wextra -pthread -DDEMONSTRATE_UB "課程 3.2：執行緒守衛類別設計4.cpp" -o guard4_ub

// 註:示範 1 的 Detach 策略沒有完成保證,其執行緒不產生輸出;
//     示範 2 的 sizeof 是本機(x86-64 / libstdc++)的實作定義值,
//     其他平台可能不同。

// === 預期輸出 ===
// === 示範 1:Join / Detach 兩種編譯期策略 ===
//   [Join 策略] 會被 join
//   [main] Join 策略的執行緒已確定結束
//   [main] Detach 策略立即返回(無完成保證)
//
// === 示範 2:零成本抽象的實證 ===
//   sizeof(std::thread)      = 8
//   sizeof(AutoJoinThread)   = 8
//   sizeof(AutoDetachThread) = 8
//   策略編碼在型別裡,不佔任何執行期空間: true
//   (以上為本機實作定義的值,不同平台可能不同)
//   兩種策略是不同型別: true
//
// === 示範 3:= default 移動賦值會 std::terminate() ===
//   已略過(預設不執行,以免整個程式 abort)。
//   要親眼看到 terminate,請加上 -DDEMONSTRATE_UB 重新編譯。
//
// === 日常實務:固定大小 thread pool 的優雅關閉 ===
//   已提交 100 個任務,完成數 = 100
//   1..100 累加結果 = 5050(正確答案 5050: true)
//   關閉後才讀取結果,無 data race —— join 建立了 happens-before
//
// === 全部示範結束 ===
