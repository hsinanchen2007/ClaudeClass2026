// =============================================================================
//  課程 2.7：執行緒的移動語意 7  —  本課總整理 + RAII 執行緒包裝類別
// =============================================================================
//
// 本檔是課程 2.7 的**總結檔**：下方保留完整的原始課程講義（第一～十節），
// 這裡先補上結構化的主題資訊、深入解釋、概念補充、注意事項與面試題。
//
// 【主題資訊 Information】
//   主題：std::thread 的移動語意與 RAII 包裝
//   標頭檔：<thread> <utility> <stdexcept>        標準：C++11 起（jthread 為 C++20）
//
//   特殊成員函式一覽（[thread.thread.class]）：
//     thread() noexcept;                             // 預設：不關聯任何執行緒
//     template<class F, class... Args> explicit thread(F&& f, Args&&... args);
//     thread(const thread&)            = delete;     // 複製建構：刪除
//     thread& operator=(const thread&) = delete;     // 複製賦值：刪除
//     thread(thread&&) noexcept;                     // 移動建構：來源變預設狀態
//     thread& operator=(thread&&) noexcept;          // 移動賦值：若 joinable → terminate
//     ~thread();                                     // 若 joinable → terminate
//     void join(); void detach(); bool joinable() const noexcept;
//     void swap(thread&) noexcept;                   // 交換握把，永不 terminate
//
//   ★ 三條規則就能涵蓋本課全部內容：
//     ① 不可複製，只可移動（唯一擁有權）
//     ② 移動後來源變成 default constructed state（標準保證，非 unspecified）
//     ③ 任何「讓 thread 物件失去現有握把」的動作（解構、移動賦值），
//        都不允許它此刻還握著一條沒交代的執行緒 → 否則 std::terminate()
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼需要 RAII 包裝：手動 join 一定會漏】
// 手寫 join 有三個永遠補不完的漏洞：
//
//   (a) 例外路徑
//         std::thread t(work);
//         mayThrow();          // ← 若丟例外，直接跳過下面
//         t.join();
//       堆疊回溯時 t 解構，joinable 為真 → std::terminate()。
//       要手動補救得寫 try/catch 把 join 抄在兩個地方，醜而且容易漏。
//
//   (b) 多重 return
//       函式中間任何一個 return 都繞過 join。函式越長越容易漏，
//       而且新增一個 return 分支的人通常不會想到執行緒的事。
//
//   (c) 後續維護
//       今天寫對了，半年後有人在中間插一個 early return 就破功。
//       **靠人記得的規則，遲早會被違反。**
//
// RAII 的核心價值是：把「一定要做的收尾」綁在解構子上，
// 交給編譯器保證執行 —— 不管是正常結束、return、還是例外回溯。
//
// 【2. 三種 RAII 策略：join / detach / 可選（本檔都有實作）】
//   * **join 型（scoped_thread）**：解構時等待執行緒結束。
//     語意最安全（執行緒用到的區域變數保證還活著），但會阻塞。
//   * **detach 型**：解構時放生。幾乎不建議 —— 執行緒可能還在存取
//     即將銷毀的物件，是 use-after-free 的溫床。
//   * **可選型（thread_guard）**：只在還 joinable 時才 join，
//     允許使用者提前自己 join。實務上最好用。
//
// 【3. 本檔原始講義中的 ScopedThread 有一個真實的潛在缺陷（重要教學點）】
// 下方講義第九節的 ScopedThread 寫了：
//     ~ScopedThread() { t.join(); }                       // 無條件 join
//     ScopedThread(ScopedThread&&) = default;             // 允許移動
//     ScopedThread& operator=(ScopedThread&&) = default;
// 這兩者放在一起會出事。理由如下：
//   被移動走之後，來源物件的成員 t 變成 non-joinable（標準保證），
//   但它的解構子仍然**無條件**呼叫 t.join()。對 non-joinable 的 thread
//   呼叫 join() 會丟 std::system_error；而這個例外是從**解構子**丟出來的，
//   解構子預設為 noexcept，於是 → std::terminate()。
//
//   本機實測（GCC 15.2，程式碼與講義逐字相同）：
//       建立 st1
//       移動 st1 -> st2
//       terminate called after throwing an instance of 'std::system_error'
//         what():  Invalid argument
//       Aborted (core dumped)        ← rc=134
//
//   為什麼講義的 main() 跑起來沒事？因為它從頭到尾**沒有真的移動過**
//   ScopedThread，那條有問題的路徑沒被走到。這正是這類缺陷的典型樣貌：
//   平常都好好的，等到有人第一次把它放進 vector 或從函式回傳時才爆炸。
//
//   ★ 講義的原始程式碼**刻意原樣保留**（那是課程內容，不在此處修改）。
//     修正版請看下方的 SafeScopedThread —— 只要把解構子改成
//         ~SafeScopedThread() { if (t_.joinable()) t_.join(); }
//     一行就解決了。這也是為什麼 RAII 包裝類別的解構子
//     **永遠要先檢查 joinable()**。
//
// 【4. 為什麼 C++20 直接把 jthread 加進標準】
// 因為上面這些坑，每個團隊都得自己寫一次 scoped_thread，而且每個團隊
// 都有機會寫錯（就像講義這版一樣）。C++20 的 std::jthread 直接內建：
//   * 解構時自動 request_stop() + join()（不會 terminate）
//   * 內建 std::stop_token 協作式取消機制
//   * 移動賦值時若 joinable，同樣是 request_stop() + join()，
//     **不是** terminate —— 這點與 std::thread 根本不同（見檔 6）
// 若專案能用 C++20，**優先用 jthread**，不要再自己造 scoped_thread。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼「持有 thread 的類別」會自動變成 move-only
// 特殊成員函式的生成規則：若任一成員不可複製，則外層類別的複製建構子
// 被隱式定義為 deleted。所以只要有 std::thread 成員，整個類別自動失去
// 複製能力 —— 這是型別系統把「唯一擁有權」沿著組合關係自動傳染下去，
// 不需要你手動 = delete（但明確寫出來更清楚，是好習慣）。
//
// (B) Rule of Five 在這裡怎麼套
// 只要你自訂了解構子（RAII 類別一定會），編譯器就**不再自動生成移動操作**。
// 於是類別悄悄變成「只能複製、不能移動」，而複製又因為 thread 成員被刪除，
// 結果變成完全不能傳遞。所以 RAII 包裝類別必須顯式寫出五個特殊成員：
// 解構子、複製建構（delete）、複製賦值（delete）、移動建構、移動賦值。
// 而且若解構子有 join，移動操作就**不能只寫 = default**（見上面第 3 點）。
//
// (C) 解構子丟例外為什麼直接 terminate
// C++11 起解構子預設是 noexcept(true)。從 noexcept 函式丟出例外會直接
// 呼叫 std::terminate()，連堆疊回溯都不做。所以**解構子裡的任何操作
// 都必須確保不丟例外** —— 對 thread 而言就是 join 前先檢查 joinable()。
//
// (D) 為什麼 join 型比 detach 型安全
// 執行緒常常捕捉了外層作用域的參考或 this 指標。若解構時 detach，
// 作用域結束、物件銷毀，而執行緒還在讀那些已死的記憶體 → use-after-free（UB）。
// join 則保證「執行緒結束後才讓資源消失」，因果順序是對的。
// 這也是為什麼 std::jthread 選擇 join 而不是 detach 作為預設收尾動作。
//
// 【注意事項 Pay Attention】
// 1. RAII 包裝的解構子**一定要先檢查 joinable()** 再 join，
//    否則被移動走的空殼物件解構時會丟例外 → terminate（本檔實測）。
// 2. 若自訂了解構子，移動操作不會自動生成，必須顯式提供；
//    而且不能無腦 = default（要搭配 joinable 檢查的解構子才安全）。
// 3. 建構子裡丟例外是合理的設計（表示「沒有執行緒可管」），
//    但要注意：此時物件尚未建構完成，解構子**不會**被呼叫。
// 4. std::logic_error 定義在 <stdexcept>。原始講義倚賴其他標頭的
//    間接引入而能編譯，本檔顯式加上 #include <stdexcept> 以策安全
//    （這是唯一對原始程式碼所做的補強，不改變任何行為）。
// 5. 能用 C++20 就用 std::jthread，不要重造 scoped_thread。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】移動語意總整理與 RAII 執行緒包裝
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼需要 scoped_thread 這種 RAII 包裝？手動 join 有什麼問題？
//     答：手動 join 無法涵蓋所有離開路徑 —— 例外回溯、多重 return、
//         以及日後有人插入 early return，都會跳過 join。一旦跳過，
//         thread 解構時 joinable 為真 → std::terminate()。
//         RAII 把收尾綁在解構子上，由編譯器保證在所有路徑上執行。
//     追問：C++20 之後還要自己寫嗎？→ 不用，直接用 std::jthread，
//           它內建解構時自動 request_stop() + join()，還附帶取消機制。
//
// 🔥 Q2. 一個類別若有 std::thread 成員，它的複製與移動能力會怎樣？
//     答：複製自動消失（成員不可複製 → 外層複製建構子被隱式 deleted）。
//         移動則要看你有沒有自訂解構子：RAII 類別一定有解構子，
//         而自訂解構子會**抑制移動操作的自動生成**，所以必須顯式寫出來，
//         否則類別會變成既不能複製也不能移動。
//     追問：那寫 = default 就好了嗎？→ 不一定。若解構子是無條件 join，
//           預設的移動會留下一個 non-joinable 的空殼，它解構時 join 失敗
//           丟例外 → terminate。解構子必須先檢查 joinable()。
//
// 🔥 Q3. 請說明 std::thread 與 std::jthread 在「移動賦值到 joinable 物件」
//        時的行為差異，以及背後的設計理由。
//     答：std::thread 呼叫 std::terminate()；std::jthread 則是
//         request_stop() 之後 join()，程式正常存活。
//         差別在於 jthread 有 stop_token 協作式取消機制，能主動要求
//         舊執行緒收工，所以那個 join 不會盲等；std::thread 沒有任何
//         取消機制，自動 join 只能無限期等下去，因此標準寧可 terminate。
//         **有安全收尾手段的型別，才敢自動收尾。**
//     追問：jthread 的 join 一定不會卡住嗎？→ 不保證。stop_token 是
//           **協作式**的，若 worker 從不檢查 stop_requested()，
//           那個 join 一樣會永久阻塞。取消是請求，不是強制。
//
// ⚠️ 陷阱. 下面這個 scoped_thread 看起來完全正確，它有什麼問題？
//            ~ScopedThread() { t.join(); }
//            ScopedThread(ScopedThread&&) = default;
//     答：被移動走之後，來源物件的 t 變成 non-joinable，但解構子仍
//         無條件呼叫 t.join() → 丟 std::system_error → 從解構子丟例外
//         → std::terminate()（本機實測 rc=134）。
//         修法是解構子加一行檢查：if (t.joinable()) t.join();
//     為什麼會錯：只檢查了「有執行緒時能不能正確收尾」，卻忘了
//         「被移動走之後的空殼物件**也會被解構**」。C++ 的 moved-from
//         物件不會消失，它仍然活著、仍然會跑解構子 —— 這是 C++ 移動語意
//         與 Rust move 最關鍵的差別，也是自寫 RAII 類別最常見的破口。
//
// ⚠️ 陷阱 2. 「把 thread 包成 RAII 之後就完全安全了，不會再有 terminate」
//     答：不對。RAII 只解決「忘記 join」。若解構子選擇 join，而執行緒
//         永遠不結束（無窮迴圈、等一個永遠不來的事件），那麼解構時就會
//         **永久阻塞** —— 程式不會 abort，但會卡死，某些情況下更難查。
//     為什麼會錯：把「不會 abort」等同於「安全」。正確的收尾需要
//         RAII（保證會 join）**加上**取消機制（保證 join 得完），
//         兩者缺一不可。這正是 jthread 同時提供解構 join 與 stop_token 的原因。
// ═══════════════════════════════════════════════════════════════════════════

/*
# 第二階段：std::thread 基礎

## 課程 2.7：執行緒的移動語意

---

### 引言

`std::thread` 是**只能移動、不能複製**的類型。理解這個特性對於正確管理執行緒至關重要。

---

### 一、為什麼不能複製？

```
┌─────────────────────────────────────────────────────────────┐
│              std::thread 不能複製的原因                      │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  一個執行緒只能有一個擁有者                                   │
│                                                             │
│  如果允許複製：                                              │
│  • 兩個 thread 物件指向同一個執行緒                          │
│  • 誰負責 join？兩個都 join 會崩潰                           │
│  • 一個 detach 後另一個怎麼辦？                              │
│                                                             │
│  所以：複製被禁止，只能移動                                   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 二、複製會編譯錯誤

```cpp
#include <thread>

int main() {
    std::thread t1([]() {});
    
    // std::thread t2 = t1;        // 編譯錯誤！
    // std::thread t3(t1);         // 編譯錯誤！
    
    t1.join();
    return 0;
}
```

錯誤訊息：
```
error: use of deleted function 'std::thread::thread(const std::thread&)'
```

---

### 三、使用 std::move 轉移所有權

```cpp
#include <iostream>
#include <thread>

int main() {
    std::thread t1([]() {
        std::cout << "執行中" << std::endl;
    });
    
    std::cout << "t1 joinable: " << t1.joinable() << std::endl;
    
    // 移動所有權
    std::thread t2 = std::move(t1);
    
    std::cout << "移動後:" << std::endl;
    std::cout << "t1 joinable: " << t1.joinable() << std::endl;
    std::cout << "t2 joinable: " << t2.joinable() << std::endl;
    
    t2.join();  // 由 t2 負責 join
    return 0;
}
```

輸出：
```
t1 joinable: 1
移動後:
t1 joinable: 0
t2 joinable: 1
執行中
```

---

### 四、移動語意圖解

```
移動前：
┌─────────┐         ┌─────────────┐
│   t1    │────────►│  執行緒資源  │
│joinable │         │             │
└─────────┘         └─────────────┘

std::move(t1) 後：
┌─────────┐         ┌─────────────┐
│   t1    │    ┌───►│  執行緒資源  │
│  空的   │    │    │             │
└─────────┘    │    └─────────────┘
               │
┌─────────┐    │
│   t2    │────┘
│joinable │
└─────────┘
```

---

### 五、函式回傳執行緒

函式可以回傳 `std::thread`（自動移動）：

```cpp
#include <iostream>
#include <thread>

std::thread createThread(int id) {
    return std::thread([id]() {
        std::cout << "執行緒 " << id << std::endl;
    });
}

int main() {
    std::thread t = createThread(42);
    t.join();
    return 0;
}
```

---

### 六、函式接受執行緒參數

```cpp
#include <iostream>
#include <thread>

void takeOwnership(std::thread t) {
    std::cout << "取得執行緒所有權" << std::endl;
    t.join();
}

int main() {
    std::thread t([]() {
        std::cout << "工作中" << std::endl;
    });
    
    takeOwnership(std::move(t));  // 必須 move
    
    // t 現在是空的
    std::cout << "t joinable: " << t.joinable() << std::endl;
    
    return 0;
}
```

---

### 七、執行緒容器

使用 `std::vector` 管理多個執行緒：

```cpp
#include <iostream>
#include <thread>
#include <vector>

int main() {
    std::vector<std::thread> threads;
    
    // 建立多個執行緒
    for (int i = 0; i < 5; ++i) {
        threads.push_back(std::thread([i]() {
            std::cout << "執行緒 " << i << std::endl;
        }));
        // 或使用 emplace_back
        // threads.emplace_back([i]() { ... });
    }
    
    // 等待所有執行緒完成
    for (auto& t : threads) {
        t.join();
    }
    
    return 0;
}
```

---

### 八、移動賦值的陷阱

移動到一個 joinable 的執行緒會導致程式終止：

```cpp
#include <thread>

int main() {
    std::thread t1([]() {});
    std::thread t2([]() {});
    
    // 危險！t1 還是 joinable，程式會呼叫 std::terminate()
    // t1 = std::move(t2);
    
    // 正確做法：先 join 或 detach
    t1.join();
    t1 = std::move(t2);  // 現在安全了
    t1.join();
    
    return 0;
}
```

---

### 九、安全的執行緒包裝類別

```cpp
#include <iostream>
#include <thread>

class ScopedThread {
    std::thread t;
public:
    explicit ScopedThread(std::thread thread) 
        : t(std::move(thread)) {
        if (!t.joinable()) {
            throw std::logic_error("No thread");
        }
    }
    
    ~ScopedThread() {
        t.join();
    }
    
    // 禁止複製
    ScopedThread(const ScopedThread&) = delete;
    ScopedThread& operator=(const ScopedThread&) = delete;
    
    // 允許移動
    ScopedThread(ScopedThread&&) = default;
    ScopedThread& operator=(ScopedThread&&) = default;
};

int main() {
    ScopedThread st(std::thread([]() {
        std::cout << "自動管理的執行緒" << std::endl;
    }));
    
    // 離開作用域時自動 join
    return 0;
}
```

---

### 十、本課重點回顧

1. `std::thread` **不能複製**，只能移動
2. 使用 `std::move()` 轉移執行緒所有權
3. 移動後原物件變成 non-joinable
4. 函式可以回傳 `std::thread`（自動移動）
5. `std::vector<std::thread>` 可管理多個執行緒
6. 不可移動到仍是 joinable 的執行緒物件

---

### 第二階段完成！

恭喜你完成了 `std::thread` 基礎階段！你已經學會：

- ✅ 建立與啟動執行緒
- ✅ 各種可呼叫物件的使用
- ✅ 參數傳遞與 std::ref
- ✅ join() 與 detach() 的選擇
- ✅ joinable() 狀態檢查
- ✅ 執行緒 ID 與硬體資訊
- ✅ 移動語意與執行緒容器

---

### 下一階段預告

**第三階段：執行緒生命週期管理** 將深入探討：
- 課程 3.1：RAII 與執行緒管理
- 課程 3.2：執行緒守衛類別設計
- 課程 3.3：std::jthread (C++20)
- ...

---

準備好進入第三階段嗎？
*/


#include <iostream>
#include <stdexcept>   // std::logic_error / std::runtime_error（原講義倚賴間接引入，此處顯式加上）
#include <thread>
#include <utility>
#include <vector>

class ScopedThread {
    std::thread t;
public:
    explicit ScopedThread(std::thread thread) 
        : t(std::move(thread)) {
        if (!t.joinable()) {
            throw std::logic_error("No thread");
        }
    }
    
    ~ScopedThread() {
        t.join();
    }
    
    // 禁止複製
    ScopedThread(const ScopedThread&) = delete;
    ScopedThread& operator=(const ScopedThread&) = delete;
    
    // 允許移動
    ScopedThread(ScopedThread&&) = default;
    ScopedThread& operator=(ScopedThread&&) = default;
};


// -----------------------------------------------------------------------------
// 【日常實務範例 1】SafeScopedThread —— 修正版 RAII 包裝
// 上方講義第九節的 ScopedThread 在**被移動之後解構**會 std::terminate()
// （解構子無條件 join 一個已 non-joinable 的 thread → 丟例外 → terminate）。
// 這裡是修正版，差別只有解構子那一行的 joinable() 檢查，
// 但它讓這個類別真正可以被移動、可以放進容器、可以從函式回傳。
// 情境：批次工作排程器要把一批 worker 存進 vector 統一管理，
//       容器擴充時會移動元素 —— 沒有這個修正就會在擴充當下 abort。
// -----------------------------------------------------------------------------
class SafeScopedThread {
    std::thread t_;
public:
    explicit SafeScopedThread(std::thread t) : t_(std::move(t)) {
        if (!t_.joinable()) {
            throw std::logic_error("SafeScopedThread: 需要一個 joinable 的執行緒");
        }
    }

    // ★ 關鍵修正：先檢查 joinable，被移動走的空殼才不會炸
    ~SafeScopedThread() {
        if (t_.joinable()) t_.join();
    }

    SafeScopedThread(const SafeScopedThread&)            = delete;
    SafeScopedThread& operator=(const SafeScopedThread&) = delete;

    // 移動建構：搬走握把，來源變空殼（解構時因 joinable() 為 false 而安全跳過）
    SafeScopedThread(SafeScopedThread&&) noexcept = default;

    // 移動賦值：不能只寫 = default —— 預設版本會對 t_ 做 thread 的移動賦值，
    // 若 t_ 此刻仍 joinable 就會 std::terminate()（見檔 6）。必須自己先收尾。
    SafeScopedThread& operator=(SafeScopedThread&& other) noexcept {
        if (this != &other) {
            if (t_.joinable()) t_.join();   // 先把自己手上的收乾淨
            t_ = std::move(other.t_);       // 此時 t_ 保證 non-joinable，安全
        }
        return *this;
    }

    bool active() const noexcept { return t_.joinable(); }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】ThreadGuard —— 只守衛、不擁有
// 情境：某些既有程式碼已經有自己的 std::thread 變數與 join 時機，
//       你不想改動它的結構，只想補上「例外路徑也保證 join」這層保險。
// 與 SafeScopedThread 的差別：ThreadGuard **不接管所有權**，
// 只持有參考。呼叫端仍可自己提前 join；guard 只在還沒 join 時補刀。
// 這是《C++ Concurrency in Action》介紹的經典 thread_guard 模式。
// -----------------------------------------------------------------------------
class ThreadGuard {
    std::thread& t_;
public:
    explicit ThreadGuard(std::thread& t) noexcept : t_(t) {}
    ~ThreadGuard() {
        if (t_.joinable()) t_.join();   // 已經被手動 join 過就跳過
    }
    ThreadGuard(const ThreadGuard&)            = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;
};

// 用來示範「例外路徑也保證 join」的工作函式
static void riskyWork(bool shouldThrow) {
    std::thread worker([]() {
        std::cout << "  [worker] 背景工作執行中\n";
    });
    ThreadGuard guard(worker);          // 從這行起，所有離開路徑都保證 join

    if (shouldThrow) {
        throw std::runtime_error("工作中途失敗");   // 直接跳過下面，但 guard 仍會 join
    }
    std::cout << "  [work] 正常完成\n";
}

int main() {
    std::cout << "=== 原始示範：講義版 ScopedThread ===\n";
    ScopedThread st(std::thread([]() {
        std::cout << "自動管理的執行緒" << std::endl;
    }));
    // 離開作用域時自動 join
    //
    // 注意：這個用法安全，是因為 st 從頭到尾**沒有被移動過**。
    // 若寫 ScopedThread st2 = std::move(st);，st 會變成空殼，
    // 它解構時無條件 t.join() 會丟 std::system_error → std::terminate()。
    // 想親眼看到，請用 -DDEMONSTRATE_SCOPEDTHREAD_BUG 編譯（見檔尾）。

    std::cout << "\n=== 實務 1：SafeScopedThread（可安全移動的修正版）===\n";
    {
        SafeScopedThread a(std::thread([]() { std::cout << "  [a] 工作\n"; }));
        SafeScopedThread b = std::move(a);      // 移動：a 變空殼
        std::cout << "  移動後 a.active()=" << std::boolalpha << a.active()
                  << " b.active()=" << b.active() << "\n";
        // a 解構時因 joinable()==false 而安全跳過 —— 這就是那一行檢查的價值
    }
    std::cout << "  離開作用域，兩者都已正確收尾\n";

    std::cout << "\n=== 實務 1b：放進 vector（容器擴充會移動元素）===\n";
    {
        std::vector<SafeScopedThread> pool;
        pool.reserve(2);                        // 即使不 reserve 也安全（已可移動）
        for (int i = 0; i < 3; ++i) {           // 故意超過 reserve → 觸發重新配置
            pool.emplace_back(std::thread([i]() {
                std::cout << "  [batch worker " << i << "] 處理批次\n";
            }));
        }
        std::cout << "  容器內 worker 數 = " << pool.size()
                  << "（重新配置時移動了元素，講義原版在此會 abort）\n";
    }   // 全部解構 → 各自 join

    std::cout << "\n=== 實務 2：ThreadGuard 在例外路徑仍保證 join ===\n";
    riskyWork(false);
    try {
        riskyWork(true);                        // 中途丟例外
    } catch (const std::runtime_error& e) {
        std::cout << "  捕捉到例外: " << e.what()
                  << "（但 worker 已被 guard 正確 join，沒有 terminate）\n";
    }

#ifdef DEMONSTRATE_SCOPEDTHREAD_BUG
    // -------------------------------------------------------------------------
    // 刻意示範：講義版 ScopedThread 被移動後解構 → std::terminate()
    // 預設**不編譯**這段。加 -DDEMONSTRATE_SCOPEDTHREAD_BUG 才會啟用。
    // 啟用後程式必定 abort（SIGABRT / rc=134）：解構子對一個 non-joinable 的
    // thread 呼叫 join() 丟出 std::system_error，而從解構子丟例外
    // （解構子預設 noexcept）會直接呼叫 std::terminate()。
    // 這不是 UB，是「解構子丟例外」這條標準規則的必然結果。
    // -------------------------------------------------------------------------
    std::cout << "\n=== [DEMONSTRATE_SCOPEDTHREAD_BUG] 講義版被移動後解構 ===\n"
              << std::flush;
    {
        ScopedThread s1(std::thread([]() { std::cout << "  [s1] worker\n"; }));
        std::cout << "即將移動 s1 -> s2（s1 將變成空殼）\n" << std::flush;
        ScopedThread s2 = std::move(s1);
        std::cout << "離開作用域時，s1 的解構子會對 non-joinable 的 t 呼叫 join()\n"
                  << std::flush;
    }   // ☠️ s1 解構 → t.join() 丟 system_error → terminate
    std::cout << "這一行永遠不會被印出來\n";
#endif

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.7：執行緒的移動語意7.cpp" -o move7
//
// 想親眼看到講義版 ScopedThread 的潛在缺陷（程式會 abort）:
//   g++ -std=c++17 -Wall -Wextra -pthread -DDEMONSTRATE_SCOPEDTHREAD_BUG "課程 2.7：執行緒的移動語意7.cpp" -o move7_boom
//   ./move7_boom            # 必定 SIGABRT，echo $? 得到 134

// 註：以下為某次實際執行結果。**每次執行都不同** —— 各 worker 的輸出

// === 預期輸出 ===
//     何時出現、彼此的先後順序，以及文字是否在同一行交錯，
//     完全取決於 OS 排程。各段之間因為有 join（解構時）而順序確定。
//     預設編譯（不加 -D）程式**正常結束，rc=0**。
//
// === 原始示範：講義版 ScopedThread ===
//
// === 實務 1：SafeScopedThread（可安全移動的修正版）===
// 自動管理的執行緒
//   移動後 a.active()=false b.active()=true
//   [a] 工作
//   離開作用域，兩者都已正確收尾
//
// === 實務 1b：放進 vector（容器擴充會移動元素）===
//   [batch worker 0] 處理批次
//   容器內 worker 數 = 3（重新配置時移動了元素，講義原版在此會 abort）
//   [batch worker 1] 處理批次
//   [batch worker 2] 處理批次
//
// === 實務 2：ThreadGuard 在例外路徑仍保證 join ===
//   [work] 正常完成
//   [worker] 背景工作執行中
//   [worker] 背景工作執行中
//   捕捉到例外: 工作中途失敗（但 worker 已被 guard 正確 join，沒有 terminate）
//
//   ↑ 注意第 1～2 段：講義版 ScopedThread 的 "自動管理的執行緒" 出現在
//     「實務 1」標題之後 —— 因為 st 要到 main 結束才解構並 join，
//     那條執行緒在這之前隨時可能被排到。這是排程造成的，不是錯誤。
//
// === 加上 -DDEMONSTRATE_SCOPEDTHREAD_BUG 的實際輸出（程式必定 abort）===
// 註：以下同樣是實跑貼上。這是「解構子丟例外 → std::terminate()」的必然結果，
//     不是 UB。收集輸出必須用 stdbuf -o0，否則 abort 會讓緩衝內容遺失。
//
// （前面各段輸出相同，接著：）
// === [DEMONSTRATE_SCOPEDTHREAD_BUG] 講義版被移動後解構 ===
// 即將移動 s1 -> s2（s1 將變成空殼）
// 離開作用域時，s1 的解構子會對 non-joinable 的 t 呼叫 join()
//   [s1] worker
// terminate called after throwing an instance of 'std::system_error'
//   what():  Invalid argument
// Aborted (core dumped)
// $ echo $?
// 134                      ← 128 + 6 (SIGABRT)
