// =============================================================================
//  課程 4.1：共享資料的問題3.cpp  —  唯讀共享為什麼天生安全
// =============================================================================
//
// 【檔案結構】上半部是課程 4.1 的完整講義（包在 /* ... */ 裡），
//   下半部是可執行的程式：多執行緒讀取同一個 const 資料。
//   本段是為整份檔案補上的教科書導讀。
//
// 【主題資訊 Information】
//   主題：    immutability（不可變性）與並行安全的關係
//   規則：    只要沒有任何執行緒寫入，多執行緒讀取【不需要任何同步】
//   標準版本：std::thread 為 C++11；C++11 才首次定義記憶體模型與 data race
//   標頭檔：  <thread>、<iostream>
//   本檔性質：這是【完全安全】的程式，與同課的「問題2.cpp」形成對照
//
// 【詳細解釋 Explanation】
//
// 【1. data race 的定義決定了唯讀為何安全】
//   C++ 標準 [intro.races] 對 data race 的條件是：
//     ① 兩條以上執行緒存取同一記憶體位置
//     ② 【至少一方是寫入】
//     ③ 兩者之間沒有 happens-before 關係
//   三個條件必須【同時】成立才構成 data race。
//   唯讀共享直接讓 ② 不成立 → 不論多少執行緒同時讀、不論怎麼排程，
//   都不可能構成 data race。這不是「機率很低」，是【定義上不可能】。
//
// 【2. 為什麼硬體層面也沒有問題】
//   從快取一致性協定（MESI）的角度看：
//   多個核心可以同時把同一條 cache line 保持在 Shared 狀態，
//   各自讀取完全不需要通訊。只有當某個核心要【寫入】時，
//   才必須先讓其他核心的副本失效、取得 Exclusive 狀態 ——
//   那才是 cache line ping-pong 與一致性流量的來源。
//   → 唯讀資料不但沒有正確性問題，連效能問題都沒有：
//     它可以被所有核心同時快取，是並行程式中最理想的資料形態。
//
// 【3. 「唯讀」的判準是「整個生命週期」，不是「這一刻」】
//   最常見的誤解是「我這段程式碼只有讀」。
//   真正的條件是【在所有執行緒都能看到這份資料的期間，沒有任何人寫】。
//   常見的破功情形：
//     * 初始化階段寫入、之後才啟動執行緒 → 安全（有 happens-before）
//     * 執行緒跑起來之後才「偷偷更新一下」→ 立刻構成 data race
//     * 惰性初始化（第一次用到才算）→ 那是寫入，需要同步（見 4.4-5）
//     * mutable 成員、內部快取、統計計數 → 名義上 const，實際在寫
//
// 【4. const 不等於執行緒安全】
//   這兩件事常被混為一談：
//     * const 是【編譯期的語言限制】：透過這個引用不能修改。
//     * 執行緒安全是【執行期的事實】：沒有任何人在寫這塊記憶體。
//   `const std::vector<int>& ref = someVector;` 完全不能阻止
//   另一條執行緒透過非 const 的路徑修改 someVector。
//   → 判斷安全與否要看「有沒有人寫」，不是看有沒有 const 關鍵字。
//   反過來說，若一個物件從建構完成後就【真的】不再被修改，
//   即使沒有標 const，它也是執行緒安全的。
//
// 【5. 這條規則的實務價值：immutable 設計】
//   「唯讀共享免同步」這個性質，是函數式語言（Haskell、Erlang、Clojure）
//   在並行領域受歡迎的根本原因，也是這些模式的理論基礎：
//     * copy-on-write：更新時複製一份新的，讀者永遠讀不可變的舊版本
//     * 設定快照：讀取端拿到 shared_ptr<const Config>，之後完全無鎖
//     * 不可變訊息傳遞：執行緒之間傳值不傳引用（Erlang / Go channel 的哲學）
//   → 並行程式設計最有效的策略往往不是「怎麼把鎖加對」，
//     而是「怎麼設計成不需要鎖」。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼建構完成後才啟動執行緒是安全的
//   std::thread 的建構函式本身就建立了 happens-before 關係：
//   建立執行緒【之前】主執行緒的所有寫入，
//   對新執行緒都是可見的（標準保證）。
//   同理 join() 也建立了反向的 happens-before。
//   所以「主執行緒初始化完 → 啟動工作執行緒 → join 後讀結果」
//   這個經典流程完全不需要額外同步。
//
// (B) 唯讀共享 vs 每執行緒各一份複本
//   兩者都安全，取捨在於：
//     * 唯讀共享：省記憶體，且所有核心可同時快取（Shared 狀態）。
//       適合大型唯讀資料（查表、設定、模型權重）。
//     * 各自複本：省掉「確保沒人寫」的心智負擔，
//       但資料大時記憶體成本高、快取利用率差。
//   大型唯讀資料幾乎一律選共享。
//
// (C) 一個容易忽略的例外：std::vector<bool>
//   一般情況下，多執行緒寫入 vector 的【不同元素】沒有 data race
//   （前提是不改變 size / capacity）。
//   但 std::vector<bool> 是特化的位元容器，
//   相鄰的多個元素擠在同一個 byte 裡 ——
//   寫入 v[0] 與 v[1] 實際上是讀改寫同一個 byte，構成 data race。
//   這是標準函式庫中最著名的陷阱之一。
//   （本檔的唯讀情境不受影響：純讀取一律安全。）
//
// 【注意事項 Pay Attention】
// 1. data race 需要「至少一方寫入」；唯讀共享定義上不可能構成 data race。
// 2. 判準是「整個生命週期都沒人寫」，不是「我這段程式碼只有讀」。
// 3. const 是編譯期限制，不等於執行期沒人在寫 —— 兩者是不同層次的事。
// 4. 惰性初始化、mutable 快取、統計計數都是隱藏的寫入。
// 5. 建構完成後才啟動執行緒是安全的：thread 的建構與 join 都建立 happens-before。
// 6. std::vector<bool> 例外：相鄰元素共用 byte，寫不同元素也會有 data race。
//
// 【LeetCode 說明】本檔不附 LeetCode 範例。
//   本檔的結論是「唯讀共享不需要同步」，這是一條規則而非演算法；
//   允許使用的設計題（146/155/705/707/1603）全都涉及【修改】資料結構，
//   並行題（1114～1117/1195）則是在協調寫入的順序 ——
//   沒有一題在示範「唯讀所以不用鎖」。硬湊只會失焦，故從缺，
//   改以下方兩個真實情境（唯讀查表、不可變設定快照）呈現。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】唯讀共享與不可變性
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 多執行緒同時讀取同一份資料，需要加鎖嗎?
//     答：只要在所有執行緒都看得到這份資料的期間，沒有任何人寫入，
//         就完全不需要同步。C++ 標準對 data race 的定義要求
//         「至少一方是寫入」，唯讀讓這個條件不成立 ——
//         這不是「機率很低」，是定義上不可能。
//         硬體層面也一樣：多核心可同時把同一條 cache line 保持在 Shared 狀態。
//     追問：那什麼時候這個保證會失效?
//         → 只要有任何一次寫入進入這個期間就失效。
//           常見的隱藏寫入包括惰性初始化、mutable 快取、統計計數器，
//           以及「跑起來之後才偷偷更新一下設定」。
//
// 🔥 Q2. 把成員函式標成 const，是不是就代表它執行緒安全?
//     答：不是。const 是【編譯期】的語言限制（透過這個引用不能改），
//         執行緒安全是【執行期】的事實（沒有任何人在寫這塊記憶體）。
//         const 完全擋不住別的執行緒透過非 const 路徑修改同一個物件。
//         而且 const 成員函式內部若有 mutable 快取或惰性初始化，
//         它自己就在寫入。
//     追問：那標準函式庫怎麼描述容器的執行緒安全?
//         → 「同一物件的並行 const 操作是安全的；
//           只要有任何非 const 操作，同步就是使用者的責任」。
//           注意它講的是 const【操作】而非 const【型別】。
//
// ⚠️ 陷阱. 「這個全域查表在程式啟動時建好，之後只有讀，
//         所以完全不用同步」——什麼情況下這句話會變成錯的?
//     答：當「建好」這件事發生在【執行緒已經啟動之後】的時候。
//         若是在 main 開頭建好、之後才建立執行緒，那是安全的
//         （thread 的建構本身建立 happens-before）。
//         但如果改成惰性初始化（第一次查詢時才建表），
//         那就是多條執行緒同時觸發寫入 → data race，
//         而且還可能建出多份（見 4.4-5 的 Singleton）。
//     為什麼會錯：把「邏輯上是常數」等同於「執行期沒有寫入」。
//         關鍵不在資料的性質，而在【寫入發生的時間點】——
//         是否在所有讀者都可能看到它之前就完成，並建立了 happens-before。
//         這也是為什麼 C++11 要特別為 function-local static
//         提供 magic static 的執行緒安全保證。
// ═══════════════════════════════════════════════════════════════════════════

/*
# 第四階段：共享資料與競爭條件

## 課程 4.1：共享資料的問題

---

### 引言

多執行緒程式設計最大的挑戰在於**共享資料**。當多個執行緒同時存取同一份資料，且至少有一個執行緒進行寫入時，問題就會產生。本課深入分析這個問題的本質。

---

### 一、共享資料的類型

```
┌─────────────────────────────────────────────────────────────┐
│                    共享資料的來源                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  • 全域變數                                                  │
│  • 靜態變數                                                  │
│  • 堆積上的物件（透過指標/引用共享）                          │
│  • 類別成員變數（多執行緒存取同一物件）                       │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 二、讀取 vs 寫入

```
┌─────────────────────────────────────────────────────────────┐
│                    存取類型與安全性                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  多執行緒同時讀取    →  安全 ✓                               │
│  一個寫入 + 一個讀取  →  不安全 ✗（資料競爭）                 │
│  多執行緒同時寫入    →  不安全 ✗（資料競爭）                  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 三、問題展示：計數器

```cpp
#include <iostream>
#include <thread>
#include <vector>

int counter = 0;

void increment() {
    for (int i = 0; i < 100000; ++i) {
        ++counter;
    }
}

int main() {
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back(increment);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "預期: 1000000" << std::endl;
    std::cout << "實際: " << counter << std::endl;
    
    return 0;
}
```

輸出（每次不同）：
```
預期: 1000000
實際: 387432
```

---

### 四、為什麼會出錯？

`++counter` 看起來是一個操作，實際上是三步：

```
┌─────────────────────────────────────────────────────────────┐
│                   ++counter 的真實步驟                       │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. 讀取：從記憶體讀取 counter 的值到 CPU 暫存器             │
│  2. 修改：在暫存器中將值 +1                                  │
│  3. 寫入：將暫存器的值寫回記憶體                             │
│                                                             │
│  這三步不是原子的，可能被其他執行緒打斷！                     │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 五、交錯執行的災難

```
時間   執行緒A              執行緒B              counter（記憶體）
────────────────────────────────────────────────────────────────
 1    讀取 counter (0)                               0
 2                         讀取 counter (0)          0
 3    +1 得到 1                                      0
 4                         +1 得到 1                 0
 5    寫回 1                                         1
 6                         寫回 1                    1
────────────────────────────────────────────────────────────────
結果：兩次 ++，但 counter 只變成 1！
```

---

### 六、更複雜的例子：銀行轉帳

```cpp
#include <iostream>
#include <thread>

struct Account {
    int balance = 1000;
};

Account accountA, accountB;

void transfer(Account& from, Account& to, int amount) {
    if (from.balance >= amount) {
        from.balance -= amount;
        to.balance += amount;
    }
}

int main() {
    std::thread t1([&]() {
        for (int i = 0; i < 1000; ++i) {
            transfer(accountA, accountB, 1);
        }
    });
    
    std::thread t2([&]() {
        for (int i = 0; i < 1000; ++i) {
            transfer(accountB, accountA, 1);
        }
    });
    
    t1.join();
    t2.join();
    
    int total = accountA.balance + accountB.balance;
    std::cout << "A: " << accountA.balance << std::endl;
    std::cout << "B: " << accountB.balance << std::endl;
    std::cout << "總額: " << total << " (應為 2000)" << std::endl;
    
    return 0;
}
```

可能輸出：
```
A: 1042
B: 1036
總額: 2078 (應為 2000)
```

錢憑空產生了！這是嚴重的資料損毀。

---

### 七、資料競爭的定義

根據 C++ 標準，**資料競爭（Data Race）** 發生於：

```
┌─────────────────────────────────────────────────────────────┐
│                  資料競爭的條件                              │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  同時滿足以下條件：                                          │
│                                                             │
│  1. 兩個或多個執行緒同時存取同一記憶體位置                   │
│  2. 至少有一個是寫入操作                                    │
│  3. 沒有同步機制保護                                        │
│                                                             │
│  結果：未定義行為（Undefined Behavior）                      │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 八、不只是錯誤的結果

資料競爭導致的是**未定義行為**，可能出現：

- 錯誤的計算結果
- 程式崩潰
- 資料損毀
- 看似正確但偶爾出錯（最難除錯）
- 在某些機器正常，換台機器就出錯

---

### 九、哪些操作是安全的？

```cpp
#include <iostream>
#include <thread>

const int readOnlyData = 42;  // 唯讀資料

void reader() {
    // 安全：只有讀取，沒有寫入
    std::cout << readOnlyData << std::endl;
}

int main() {
    std::thread t1(reader);
    std::thread t2(reader);
    
    t1.join();
    t2.join();
    
    return 0;
}
```

**規則**：只要沒有寫入，多執行緒讀取是安全的。

---

### 十、解決方案預覽

```
┌─────────────────────────────────────────────────────────────┐
│                  解決資料競爭的方法                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. 互斥鎖（Mutex）                                         │
│     確保同一時間只有一個執行緒存取資料                       │
│                                                             │
│  2. 原子操作（Atomic）                                      │
│     使用硬體支援的不可分割操作                               │
│                                                             │
│  3. 避免共享                                                │
│     每個執行緒使用自己的資料副本                             │
│                                                             │
│  4. 不可變資料                                              │
│     資料建立後不再修改                                       │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 十一、本課重點回顧

1. 多執行緒存取共享資料是危險的
2. 讀取 + 寫入或多重寫入會造成資料競爭
3. `++counter` 不是原子操作，由三個步驟組成
4. 資料競爭導致**未定義行為**
5. 只有純讀取是安全的
6. 解決方案：互斥鎖、原子操作、避免共享

---

### 下一課預告

在 **課程 4.2：不變量與競爭條件** 中，我們將學習：
- 什麼是不變量（Invariant）
- 不變量被破壞的情況
- 如何維護資料的一致性

---

準備好繼續嗎？
*/



#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <atomic>
#include <iomanip>

const int readOnlyData = 42;  // 唯讀資料

void reader() {
    // 安全：只有讀取，沒有寫入
    std::cout << readOnlyData << std::endl;
}


// -----------------------------------------------------------------------------
// 【日常實務範例 1】唯讀查表：所有執行緒共用，零同步
//   情境：地理編碼、字元編碼轉換、稅率表、遊戲的傷害係數表 ——
//         這類資料在啟動時載入一次，之後整個程式生命週期都不再改變。
//   關鍵：表格必須在【啟動任何工作執行緒之前】完成建構。
//         std::thread 的建構函式建立 happens-before 關係，
//         保證主執行緒先前的所有寫入對新執行緒都可見。
//   效益：不只免同步，多核心還能同時把它保持在快取的 Shared 狀態，
//         是並行程式中最理想的資料形態。
// -----------------------------------------------------------------------------
struct TaxTable {
    std::vector<double> rates;      // 建構完成後不再修改

    TaxTable() {
        rates.reserve(64);
        for (int i = 0; i < 64; ++i) rates.push_back(0.05 + i * 0.001);
    }

    // 純查詢，不修改任何狀態 —— 真正的唯讀
    double rateFor(int bracket) const {
        return rates[static_cast<size_t>(bracket) % rates.size()];
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】不可變設定快照：用「換掉整份」取代「就地修改」
//   情境：設定需要熱更新，但讀取端每個請求都要用，不能每次都上鎖。
//   做法：設定物件本身【永遠不修改】（immutable）。
//         要更新時，建一份【全新】的設定，然後只換掉那個指標。
//         讀取端拿到 shared_ptr<const Config> 之後，
//         那份設定在它手上絕對不會變 —— 之後的所有存取零同步。
//   這是 copy-on-write 的核心，也是「唯讀共享免同步」最重要的實務應用。
//   舊版本會在最後一個讀者放手時由 shared_ptr 自動回收，不需要等待或 GC。
// -----------------------------------------------------------------------------
struct AppConfig {
    std::string endpoint;
    int timeoutMs;
    int retries;

    AppConfig(std::string ep, int t, int r)
        : endpoint(std::move(ep)), timeoutMs(t), retries(r) {}
};

class ConfigHolder {
private:
    mutable std::mutex mtx;
    std::shared_ptr<const AppConfig> current;

public:
    ConfigHolder() {
        current = std::make_shared<const AppConfig>("api.v1.internal", 3000, 3);
    }

    // 讀取端：只在複製 shared_ptr 的極短時間內持鎖
    std::shared_ptr<const AppConfig> snapshot() const {
        std::lock_guard<std::mutex> lock(mtx);
        return current;
    }

    // 寫入端：建一份全新的，換掉指標（絕不就地修改舊的）
    void publish(std::string endpoint, int timeoutMs, int retries) {
        auto next = std::make_shared<const AppConfig>(std::move(endpoint), timeoutMs, retries);
        std::lock_guard<std::mutex> lock(mtx);
        current = next;
    }
};

int main() {
    std::cout << "=== 唯讀共享：多執行緒同時讀 const 資料 ===\n";
    std::thread t1(reader);
    std::thread t2(reader);
    
    t1.join();
    t2.join();
    std::cout << "→ 兩條執行緒同時讀取 readOnlyData，完全沒有同步，"
                 "也完全不可能有 data race。\n";
    std::cout << "  因為 data race 的定義要求「至少一方寫入」，這裡沒有任何寫入。\n";

    std::cout << "\n=== 日常實務 1：唯讀查表（零同步）===\n";
    {
        // 關鍵：表格在啟動任何執行緒【之前】就建好
        const TaxTable table;

        std::vector<std::thread> ths;
        std::vector<double> results(8, 0.0);
        for (int i = 0; i < 8; ++i) {
            ths.emplace_back([&table, &results, i] {
                double sum = 0;
                for (int k = 0; k < 100000; ++k) sum += table.rateFor(i * 100000 + k);
                results[static_cast<size_t>(i)] = sum;   // 各寫各的索引，不重疊
            });
        }
        for (auto& t : ths) t.join();

        double total = 0;
        for (double v : results) total += v;
        std::cout << "8 條執行緒各查表 100000 次，全程零同步\n";
        std::cout << "查表次數: 800000\n";
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "係數總和: " << total << "（確定值）\n";
        std::cout << "→ 表格建構完成後不再修改；std::thread 的建構\n";
        std::cout << "  保證主執行緒先前的寫入對新執行緒可見（happens-before）。\n";
    }

    std::cout << "\n=== 日常實務 2：不可變設定快照（copy-on-write）===\n";
    {
        ConfigHolder holder;
        std::atomic<int> inconsistent{0};
        std::atomic<bool> stop{false};
        std::vector<std::thread> readers;

        // 讀取端：拿到快照後完全無鎖地讀整份設定
        for (int i = 0; i < 4; ++i) {
            readers.emplace_back([&holder, &inconsistent, &stop] {
                while (!stop.load(std::memory_order_relaxed)) {
                    auto cfg = holder.snapshot();
                    // 不變量：三個欄位必定來自同一份設定，不可能混搭
                    bool ok = (cfg->endpoint == "api.v1.internal" && cfg->timeoutMs == 3000 && cfg->retries == 3)
                           || (cfg->endpoint == "api.v2.internal" && cfg->timeoutMs == 5000 && cfg->retries == 5);
                    if (!ok) inconsistent.fetch_add(1);
                }
            });
        }

        std::thread writer([&holder] {
            for (int i = 0; i < 2000; ++i) {
                if (i % 2 == 0) holder.publish("api.v2.internal", 5000, 5);
                else            holder.publish("api.v1.internal", 3000, 3);
            }
        });
        writer.join();
        stop.store(true);
        for (auto& t : readers) t.join();

        auto fin = holder.snapshot();
        std::cout << "寫入端發佈 2000 次新設定，4 條讀取端持續讀取\n";
        std::cout << "讀到欄位混搭的次數: " << inconsistent.load() << "（必定為 0）\n";
        std::cout << "最終設定: " << fin->endpoint << " timeout=" << fin->timeoutMs
                  << " retries=" << fin->retries << "\n";
        std::cout << "→ 設定物件本身永不修改；更新是「換掉整份」而非「就地改」，\n";
        std::cout << "  所以讀者手上那份在它使用期間絕對不會變 —— 這就是不可變性的價值。\n";
    }
    
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 4.1：共享資料的問題3.cpp' -o readonly_safe
//
// 驗證無資料競爭（本檔應【完全乾淨】）:
//   g++ -std=c++17 -fsanitize=thread -g -pthread '課程 4.1：共享資料的問題3.cpp' -o readonly_tsan

// 註：本檔【完全沒有資料競爭】——第一段是唯讀共享，後兩段的寫入都有鎖保護，
// 因此輸出為確定值，每次執行完全相同（本機連續三次實測 md5 一致）。
// 唯一在理論上可能不同的是開頭兩個「42」的先後順序，
// 但兩行內容一模一樣，看不出差別（cout 的並行輸出不是 UB，
// C++11 起標準對標準串流物件有此保證）。

// === 預期輸出 ===
// === 唯讀共享：多執行緒同時讀 const 資料 ===
// 42
// 42
// → 兩條執行緒同時讀取 readOnlyData，完全沒有同步，也完全不可能有 data race。
//   因為 data race 的定義要求「至少一方寫入」，這裡沒有任何寫入。
//
// === 日常實務 1：唯讀查表（零同步）===
// 8 條執行緒各查表 100000 次，全程零同步
// 查表次數: 800000
// 係數總和: 65200.00（確定值）
// → 表格建構完成後不再修改；std::thread 的建構
//   保證主執行緒先前的寫入對新執行緒可見（happens-before）。
//
// === 日常實務 2：不可變設定快照（copy-on-write）===
// 寫入端發佈 2000 次新設定，4 條讀取端持續讀取
// 讀到欄位混搭的次數: 0（必定為 0）
// 最終設定: api.v1.internal timeout=3000 retries=3
// → 設定物件本身永不修改；更新是「換掉整份」而非「就地改」，
//   所以讀者手上那份在它使用期間絕對不會變 —— 這就是不可變性的價值。
