// =============================================================================
//  第 18 課 範例 11  —  生命週期綜合實戰：遊戲場景的實體管理
// =============================================================================
//
//  ※ 本檔結構說明：
//    第 1 行起是一大段 /* ... */ 的課程講義（Markdown 格式，內含多個示範用的
//    程式片段，那些片段「不會被編譯」）。真正會被編譯執行的程式碼，
//    從下方 `#include <iostream>` 那一行開始。閱讀時請注意區分。
//
// 【主題資訊 Information】
//   標準版本：C++98 起有四種儲存期；C++11 起 static 區域變數初始化保證執行緒安全
//   標頭檔  ：<iostream>、<string>
//   複雜度  ：建構／解構皆 O(1)；重點在「何時發生」而非「多快」
//
// 【詳細解釋 Explanation】
//
// 【1. 這個範例把三種儲存期放在同一個場景裡對照】
//   遊戲場景是示範生命週期最直觀的比喻，本檔剛好涵蓋三種：
//     * player / npc（自動儲存期）：在 main 的 scope 內，活到 main 結束。
//     * enemy1 / enemy2 / summon（自動儲存期，但在更內層的 scope）：
//       函式或區塊一結束就消滅 —— 召喚物只活在它自己的 `{}` 裡。
//     * boss（動態儲存期）：new 出來的，必須自己 delete，
//       否則它會永遠留在場上（記憶體洩漏）。
//   static 計數器 totalAlive 則像「場上實體數」的 HUD，
//   讓每一次生死都能被量化觀察。
//
// 【2. 巢狀 scope：召喚物為什麼先消失】
//   battlePhase() 裡的火元素被包在自己的 `{ }` 中，
//   因此它在區塊結束時就被解構，早於同一函式中比它先建立的 enemy1／enemy2。
//   接著函式結束時，enemy2 → enemy1 依反序解構。
//   這完整示範了「解構順序 = 建構順序的反序（LIFO）」這條規則，
//   而且說明了「巢狀 scope 會讓內層物件先走」。
//
// 【3. 為什麼 totalAlive 要在建構時 ++、解構時 --】
//   注意本檔的解構函數是「先 --totalAlive 再輸出」，
//   所以印出來的數字已經不包含自己 —— 與第 17 課範例 7
//   刻意用 count - 1 的寫法不同。兩種寫法都對，重點是要一致，
//   否則同一份報表會出現差 1 的數字。
//   實務上建議採用本檔這種「先更新狀態、再輸出」的順序，比較不易出錯。
//
// 【4. 動態物件是唯一需要你負責的】
//   boss 用 new 建立，如果忘了 delete，這個物件會活到行程結束、
//   而且它的解構函數永遠不會被呼叫（[消滅] 那一行不會出現，
//   totalAlive 也永遠回不到 0）。
//   在真實遊戲裡，這就是「打完 Boss 記憶體卻一直漲」的典型原因。
//   現代 C++ 的做法是改用 std::unique_ptr<Entity>，
//   讓 boss 也退回自動管理，連 delete 都不必寫。
//
// 【概念補充 Concept Deep Dive】
//   `delete boss;` 實際上是兩個獨立步驟，順序不可顛倒：
//     (1) 呼叫 boss 所指物件的解構函數（此時才印出 [消滅]）；
//     (2) 把那塊記憶體還給配置器（operator delete）。
//   所以「解構」與「釋放記憶體」是兩件事。這也解釋了為什麼
//   placement new 建立的物件必須「手動呼叫解構函數」卻「不能 delete」——
//   那塊記憶體並非由 operator new 配置。
//
//   靜態成員 totalAlive 不屬於任何實例，存放在靜態儲存區，
//   所有 Entity 共用同一份，因此 sizeof(Entity) 不包含它。
//
// 【注意事項 Pay Attention】
// 1. new 與 delete 必須配對。忘記 delete 是記憶體洩漏（解構函數不會執行）；
//    delete 兩次、或 delete 後繼續使用，都是未定義行為。
// 2. 若 Entity 之後被當成多型基底（例如衍生出 Goblin、Dragon，
//    並以 Entity* 持有再 delete），基底的解構函數必須是 virtual，
//    否則是未定義行為，衍生類別的資源不會被釋放。
//    本範例沒有繼承，維持非 virtual 是正確且省成本的。
// 3. 靜態成員的 ++／-- 在多執行緒下並非原子操作；
//    真要跨執行緒統計需改用 std::atomic<int>。
// 4. 本檔使用 `using namespace std;`，教學檔可接受，
//    標頭檔與大型專案應避免。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】生命週期綜合
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 一個函式裡有三個區域物件，其中一個被包在內層的 { } 中，
//        它們的解構順序是什麼？
//     答：內層區塊的物件在該區塊的 `}` 就先解構；
//         其餘兩個等到函式結束時，以建構順序的反序解構。
//         本檔中即為：火元素（內層）→ 骷髏兵 → 哥布林。
//         規則只有一條：每個 scope 各自負責、一律反序。
//     追問：為什麼標準要規定反序而不是正序？
//         → 因為後建立的物件可能依賴先建立的物件（持有其參考或資源）。
//           反序能保證「被依賴者一定活得比依賴者久」，
//           RAII 的正確性建立在這個保證上。
//
// 🔥 Q2. 如果把 `delete boss;` 那一行刪掉，會發生什麼？
//     答：記憶體洩漏。而且不只是記憶體 —— boss 的解構函數永遠不會被呼叫，
//         所以 [消滅] 那一行不會出現，totalAlive 也永遠停在 1 回不到 0。
//         若解構函數負責的是關閉檔案、釋放鎖、送出結算封包，
//         那些副作用也全部不會發生。這比單純漏一塊記憶體嚴重得多。
//     追問：那要怎麼從根本上避免？
//         → 改用 std::unique_ptr<Entity> boss = std::make_unique<Entity>(...);
//           它會在離開 scope 時自動解構，連 delete 都不必寫，
//           而且提前 return 或丟出例外時也一樣會執行（Rule of Zero）。
//
// ⚠️ 陷阱. 「物件是在解構函數的第一行就已經消失了，
//           所以解構函數裡不能再存取成員變數。」
//     答：錯，剛好相反。進入解構函數時物件仍然完整存在，
//         所有成員都還有效 —— 這正是解構函數能釋放資源、能印出
//         自己的 name 與 type 的前提。
//         成員的解構發生在解構函數本體「執行完畢之後」，而不是之前。
//     為什麼會錯：把「開始解構」直覺地等同於「已經被銷毀」。
//         正確的心智模型是：解構函數是物件的「遺言」——
//         說完之後，成員才依宣告順序的反序被銷毀，
//         最後儲存空間才被回收。
// ═══════════════════════════════════════════════════════════════════════════
//
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔不附
//   理由：本檔主題是「物件在不同 scope 與儲存期下的生死時機」，
//   屬於語言的物件模型。LeetCode 評測的是演算法輸入輸出，
//   不會因為某個物件在第幾行被解構而有不同答案。
//   依規格「寧缺勿濫」從缺——本檔的遊戲場景實體管理
//   本身就是貼近真實工程（遊戲引擎的實體生命週期）的示範。
// -----------------------------------------------------------------------------

/*
好的，信安！讓我們把前面學的建構和解構串成一個完整的圖景。

---

# 第 18 課：對象的生命週期（Object Lifetime）

---

## 18.1 什麼是對象的生命週期？

對象的生命週期是指從**建構函數完成**到**解構函數開始**之間的這段時間。在這段時間內，對象是「活著的」，可以安全地使用。

```
    建構函數執行        對象存活期間          解構函數執行
    ┌─────────┐  ┌──────────────────┐  ┌─────────┐
────┤ 初始化   ├──┤  可以安全使用     ├──┤  清理    ├────
    └─────────┘  └──────────────────┘  └─────────┘
    ↑                                              ↑
   誕生                                           死亡
```

在 C 語言中，你只有兩種存儲期：自動（棧）和動態（堆），而且沒有建構/解構的概念。C++ 則有更豐富的對象類型，每種都有不同的生命週期規則。

---

## 18.2 C++ 的四種存儲期（Storage Duration）

```cpp
#include <iostream>
#include <string>
using namespace std;

class Probe {
private:
    string name;
public:
    Probe(const string& n) : name(n) {
        cout << "  [誕生] " << name << endl;
    }
    ~Probe() {
        cout << "  [死亡] " << name << endl;
    }
    void hello() const {
        cout << "  [存活] " << name << " 正在工作" << endl;
    }
};

// ====== 1. 靜態存儲期：全域對象 ======
Probe globalObj("全域物件");

void func() {
    // ====== 2. 自動存儲期：局部對象 ======
    Probe localObj("func 局部物件");
    localObj.hello();
    
    // ====== 3. 靜態存儲期：靜態局部對象 ======
    static Probe staticLocal("func 靜態局部物件");
    staticLocal.hello();
}

int main() {
    cout << "\n=== main() 開始 ===" << endl;
    
    cout << "\n--- 第一次調用 func() ---" << endl;
    func();
    
    cout << "\n--- 第二次調用 func() ---" << endl;
    func();
    
    cout << "\n--- 動態對象 ---" << endl;
    // ====== 4. 動態存儲期：堆上對象 ======
    Probe* heapObj = new Probe("動態物件");
    heapObj->hello();
    delete heapObj;
    
    cout << "\n=== main() 結束 ===" << endl;
    return 0;
}
```

### 預期輸出

```
  [誕生] 全域物件

=== main() 開始 ===

--- 第一次調用 func() ---
  [誕生] func 局部物件
  [存活] func 局部物件 正在工作
  [誕生] func 靜態局部物件
  [存活] func 靜態局部物件 正在工作
  [死亡] func 局部物件

--- 第二次調用 func() ---
  [誕生] func 局部物件
  [存活] func 局部物件 正在工作
  [存活] func 靜態局部物件 正在工作
  [死亡] func 局部物件

--- 動態對象 ---
  [誕生] 動態物件
  [存活] 動態物件 正在工作
  [死亡] 動態物件

=== main() 結束 ===
  [死亡] func 靜態局部物件
  [死亡] 全域物件
```

### 關鍵觀察

- **局部物件**：每次調用 `func()` 都重新建構，離開函數就解構
- **靜態局部物件**：第一次調用時建構，第二次調用時**不再建構**（已經存在），程式結束時才解構
- **動態物件**：`new` 時建構，`delete` 時解構
- **全域物件**：最先建構，最後解構

---

## 18.3 四種存儲期的完整對比

| 存儲期 | 宣告位置 | 誕生 | 死亡 | 特點 |
|--------|----------|------|------|------|
| **自動**（棧） | 函數/區塊內 | 執行到宣告時 | 離開作用域時 | 最常用，自動管理 |
| **靜態**（全域） | 函數外 | `main()` 之前 | `main()` 之後 | 整個程式存活 |
| **靜態**（局部） | 函數內 `static` | 第一次執行到時 | `main()` 之後 | 只初始化一次 |
| **動態**（堆） | `new` 創建 | `new` 時 | `delete` 時 | 手動管理，忘記就洩漏 |

---

## 18.4 自動存儲期的細節：作用域與嵌套

```cpp
#include <iostream>
#include <string>
using namespace std;

class Scope {
private:
    string name;
public:
    Scope(const string& n) : name(n) {
        cout << "  [+] " << name << endl;
    }
    ~Scope() {
        cout << "  [-] " << name << endl;
    }
};

int main() {
    cout << "=== 作用域嵌套觀察 ===" << endl;
    
    Scope a("a - main 層");
    
    {   // 第一層區塊
        Scope b("b - 第一層");
        
        {   // 第二層區塊
            Scope c("c - 第二層");
            
            {   // 第三層區塊
                Scope d("d - 第三層");
                cout << "  --- 最深處 ---" << endl;
            }   // d 死亡
            
            cout << "  --- 回到第二層 ---" << endl;
        }   // c 死亡
        
        cout << "  --- 回到第一層 ---" << endl;
    }   // b 死亡
    
    cout << "  --- 回到 main ---" << endl;
    return 0;
}   // a 死亡
```

### 預期輸出

```
=== 作用域嵌套觀察 ===
  [+] a - main 層
  [+] b - 第一層
  [+] c - 第二層
  [+] d - 第三層
  --- 最深處 ---
  [-] d - 第三層
  --- 回到第二層 ---
  [-] c - 第二層
  --- 回到第一層 ---
  [-] b - 第一層
  --- 回到 main ---
  [-] a - main 層
```

像堆疊一樣，後進先出：

```
進入 main：      [a]
進入第一層：     [a][b]
進入第二層：     [a][b][c]
進入第三層：     [a][b][c][d]
離開第三層：     [a][b][c]        ← d 解構
離開第二層：     [a][b]           ← c 解構
離開第一層：     [a]              ← b 解構
離開 main：      []               ← a 解構
```

---

## 18.5 if / for / switch 中的對象生命週期

很多人不知道，在控制結構中宣告的對象也有明確的生命週期：

```cpp
#include <iostream>
#include <string>
using namespace std;

class Tracker {
private:
    string name;
public:
    Tracker(const string& n) : name(n) {
        cout << "  [+] " << name << endl;
    }
    ~Tracker() {
        cout << "  [-] " << name << endl;
    }
    void work() const {
        cout << "  [=] " << name << " 工作中" << endl;
    }
};

int main() {
    // ====== if 語句中的對象 ======
    cout << "=== if 語句 ===" << endl;
    if (bool condition = true) {
        Tracker t("if 區塊物件");
        t.work();
    }   // t 在這裡死亡
    cout << "  if 區塊已結束\n" << endl;
    
    // ====== for 迴圈中的對象 ======
    cout << "=== for 迴圈 ===" << endl;
    for (int i = 0; i < 3; i++) {
        Tracker t("迴圈物件 #" + to_string(i));
        t.work();
        // t 在每次迭代結束時死亡，下次迭代重新建構
    }
    cout << "  for 迴圈已結束\n" << endl;
    
    // ====== for 初始化部分的對象 ======
    cout << "=== for 初始化區 ===" << endl;
    // 注意：C++ 沒有直接在 for 初始化區放自定義對象的語法
    // 但可以這樣理解等效行為
    {
        Tracker outer("迴圈外圍物件");
        for (int i = 0; i < 2; i++) {
            Tracker inner("迴圈內部 #" + to_string(i));
            inner.work();
        }
    }
    cout << "  完成\n" << endl;
    
    return 0;
}
```

### 預期輸出

```
=== if 語句 ===
  [+] if 區塊物件
  [=] if 區塊物件 工作中
  [-] if 區塊物件
  if 區塊已結束

=== for 迴圈 ===
  [+] 迴圈物件 #0
  [=] 迴圈物件 #0 工作中
  [-] 迴圈物件 #0
  [+] 迴圈物件 #1
  [=] 迴圈物件 #1 工作中
  [-] 迴圈物件 #1
  [+] 迴圈物件 #2
  [=] 迴圈物件 #2 工作中
  [-] 迴圈物件 #2
  for 迴圈已結束

=== for 初始化區 ===
  [+] 迴圈外圍物件
  [+] 迴圈內部 #0
  [=] 迴圈內部 #0 工作中
  [-] 迴圈內部 #0
  [+] 迴圈內部 #1
  [=] 迴圈內部 #1 工作中
  [-] 迴圈內部 #1
  [-] 迴圈外圍物件
  完成
```

**for 迴圈的重要觀察**：迴圈體內的對象在**每次迭代結束時解構，下次迭代開始時重新建構**。這代表每次迭代都是一個獨立的生命週期。

---

## 18.6 靜態局部對象的生命週期：延遲初始化

靜態局部對象有一個特殊行為——**延遲初始化（Lazy Initialization）**，只在第一次執行到時才建構：

```cpp
#include <iostream>
#include <string>
using namespace std;

class ExpensiveResource {
private:
    string name;
    
public:
    ExpensiveResource(const string& n) : name(n) {
        cout << "  [建構] " << name << "（模擬耗時初始化...）" << endl;
    }
    
    ~ExpensiveResource() {
        cout << "  [解構] " << name << endl;
    }
    
    void use() const {
        cout << "  [使用] " << name << endl;
    }
};

ExpensiveResource& getResource() {
    // 靜態局部對象：只在第一次調用時建構
    // C++11 保證這個初始化是線程安全的
    static ExpensiveResource resource("共享資源");
    return resource;
}

int main() {
    cout << "=== 延遲初始化展示 ===" << endl;
    
    cout << "\n--- 程式啟動，但還沒使用資源 ---" << endl;
    cout << "  (注意：資源還沒被建構)\n" << endl;
    
    cout << "--- 第一次調用 getResource() ---" << endl;
    getResource().use();    // 第一次：觸發建構
    
    cout << "\n--- 第二次調用 getResource() ---" << endl;
    getResource().use();    // 第二次：不再建構，直接使用
    
    cout << "\n--- 第三次調用 getResource() ---" << endl;
    getResource().use();    // 第三次：同上
    
    cout << "\n=== main() 結束 ===" << endl;
    return 0;
}
// 程式結束時，靜態局部對象才被解構
```

### 預期輸出

```
=== 延遲初始化展示 ===

--- 程式啟動，但還沒使用資源 ---
  (注意：資源還沒被建構)

--- 第一次調用 getResource() ---
  [建構] 共享資源（模擬耗時初始化...）
  [使用] 共享資源

--- 第二次調用 getResource() ---
  [使用] 共享資源

--- 第三次調用 getResource() ---
  [使用] 共享資源

=== main() 結束 ===
  [解構] 共享資源
```

這個模式在實際開發中非常常見，例如單例模式（Singleton，第 105 課會學到）就是基於這個原理。

---

## 18.7 多個全域對象的初始化順序陷阱

全域對象有一個著名的陷阱——**靜態初始化順序問題（Static Initialization Order Fiasco）**：

```cpp
#include <iostream>
#include <string>
using namespace std;

// === 檔案 A 中（概念上）===
class Config {
private:
    int maxConnections;
public:
    Config() : maxConnections(100) {
        cout << "  [Config] 初始化: maxConnections = " 
             << maxConnections << endl;
    }
    int getMax() const { return maxConnections; }
};

// === 檔案 B 中（概念上）===
class ConnectionPool {
private:
    int poolSize;
public:
    // 危險！這裡依賴 config 已經被初始化
    ConnectionPool(const Config& cfg) : poolSize(cfg.getMax()) {
        cout << "  [ConnectionPool] 初始化: poolSize = " 
             << poolSize << endl;
    }
};

// 全域對象
Config config;                    // 在這個檔案中先宣告
ConnectionPool pool(config);      // 依賴 config

int main() {
    cout << "\n=== 程式開始 ===" << endl;
    cout << "  在同一個編譯單元中，順序是確定的" << endl;
    cout << "  但如果 config 和 pool 在不同的 .cpp 檔案中..." << endl;
    cout << "  初始化順序就是未定義的！" << endl;
    return 0;
}
```

### 預期輸出

```
  [Config] 初始化: maxConnections = 100
  [ConnectionPool] 初始化: poolSize = 100

=== 程式開始 ===
  在同一個編譯單元中，順序是確定的
  但如果 config 和 pool 在不同的 .cpp 檔案中...
  初始化順序就是未定義的！
```

**問題**：如果 `Config config;` 和 `ConnectionPool pool(config);` 在**不同的 .cpp 檔案中**，C++ 標準不保證 `config` 會在 `pool` 之前初始化！`pool` 可能在 `config` 還是垃圾值的時候就讀取它。

**解決方案**：使用我們剛學的靜態局部對象來取代全域對象：

```cpp
#include <iostream>
#include <string>
using namespace std;

class Config {
private:
    int maxConnections;
public:
    Config() : maxConnections(100) {
        cout << "  [Config] 初始化" << endl;
    }
    int getMax() const { return maxConnections; }
};

class ConnectionPool {
private:
    int poolSize;
public:
    ConnectionPool(int size) : poolSize(size) {
        cout << "  [ConnectionPool] 初始化: " << poolSize << endl;
    }
};

// 用函數包裝，保證初始化順序
Config& getConfig() {
    static Config config;    // 第一次調用時建構
    return config;
}

ConnectionPool& getPool() {
    // getConfig() 保證在使用前就已初始化
    static ConnectionPool pool(getConfig().getMax());
    return pool;
}

int main() {
    cout << "=== 安全的初始化順序 ===" << endl;
    cout << "  Pool size = " << getPool().getMax() << endl;  
    // 假設 ConnectionPool 也有 getMax() 的話
    
    getPool();   // 第二次調用，不會重複初始化
    return 0;
}
```

---

## 18.8 臨時對象的生命週期

臨時對象是一種特殊的短命對象，它在表達式結束時就死亡：

```cpp
#include <iostream>
#include <string>
using namespace std;

class Temp {
private:
    string name;
public:
    Temp(const string& n) : name(n) {
        cout << "  [+] " << name << endl;
    }
    ~Temp() {
        cout << "  [-] " << name << endl;
    }
    
    string getName() const { return name; }
    
    // 返回自身引用，用於鏈式調用
    const Temp& show() const {
        cout << "  [=] " << name << " 展示中" << endl;
        return *this;
    }
};

Temp createTemp(const string& n) {
    return Temp(n);   // 返回一個臨時對象
}

int main() {
    cout << "=== 臨時對象的生命週期 ===" << endl;
    
    // 情境 1：臨時對象在語句結束時死亡
    cout << "\n--- 情境 1：純臨時對象 ---" << endl;
    Temp("短命鬼").show();
    cout << "  (臨時對象已死亡)\n" << endl;
    // Temp("短命鬼") 在這行的分號處就被解構了
    
    // 情境 2：用變數接住，延長生命
    cout << "--- 情境 2：用變數接住 ---" << endl;
    Temp saved = createTemp("被拯救的");
    saved.show();
    cout << "  (saved 還活著)\n" << endl;
    
    // 情境 3：const 引用延長臨時對象的生命
    cout << "--- 情境 3：const 引用延長生命 ---" << endl;
    const Temp& ref = Temp("引用續命");
    ref.show();
    cout << "  (ref 綁定的臨時對象還活著)\n" << endl;
    // 臨時對象的生命被延長到 ref 離開作用域時
    
    cout << "=== main() 結束 ===" << endl;
    return 0;
}
```

### 預期輸出

```
=== 臨時對象的生命週期 ===

--- 情境 1：純臨時對象 ---
  [+] 短命鬼
  [=] 短命鬼 展示中
  [-] 短命鬼
  (臨時對象已死亡)

--- 情境 2：用變數接住 ---
  [+] 被拯救的
  [=] 被拯救的 展示中
  (saved 還活著)

--- 情境 3：const 引用延長生命 ---
  [+] 引用續命
  [=] 引用續命 展示中
  (ref 綁定的臨時對象還活著)

=== main() 結束 ===
  [-] 引用續命
  [-] 被拯救的
```

### 臨時對象生命週期規則

| 情境 | 生命週期 |
|------|----------|
| `Temp("x").show();` | 到語句的分號 `;` 就死亡 |
| `Temp t = Temp("x");` | 和 `t` 的生命週期相同 |
| `const Temp& r = Temp("x");` | 延長到 `r` 離開作用域 |
| `Temp&& rr = Temp("x");` | 延長到 `rr` 離開作用域（右值引用，後面課程會學） |

---

## 18.9 常見的生命週期陷阱

### 陷阱 1：返回局部對象的引用（懸空引用）

```cpp
#include <iostream>
#include <string>
using namespace std;

class Data {
public:
    int value;
    Data(int v) : value(v) {
        cout << "  [+] Data(" << value << ")" << endl;
    }
    ~Data() {
        cout << "  [-] Data(" << value << ")" << endl;
    }
};

// 危險！返回局部對象的引用
Data& dangerous() {
    Data local(42);       // local 是局部對象
    return local;         // 返回 local 的引用
}   // local 在這裡死亡！返回的引用指向已死的對象！

// 安全：返回值（複製）
Data safe() {
    Data local(42);
    return local;         // 返回副本（編譯器可能優化掉複製）
}

int main() {
    cout << "=== 陷阱：懸空引用 ===" << endl;
    
    // Data& ref = dangerous();   // 未定義行為！
    // cout << ref.value << endl;  // 可能印出垃圾值或崩潰
    // 編譯器通常會警告：returning reference to local variable
    
    cout << "\n=== 安全：返回值 ===" << endl;
    Data d = safe();
    cout << "  d.value = " << d.value << endl;  // OK
    
    return 0;
}
```

### 陷阱 2：指標指向已死的局部對象

```cpp
#include <iostream>
using namespace std;

class Item {
public:
    int id;
    Item(int i) : id(i) {
        cout << "  [+] Item #" << id << endl;
    }
    ~Item() {
        cout << "  [-] Item #" << id << endl;
    }
};

Item* dangerousPointer() {
    Item local(99);
    return &local;    // 返回局部對象的地址——危險！
}   // local 死亡，返回的指標成為野指標

int main() {
    cout << "=== 陷阱：野指標 ===" << endl;
    
    // Item* ptr = dangerousPointer();
    // cout << ptr->id << endl;  // 未定義行為！
    
    // 正確做法：用 new 分配
    cout << "\n=== 正確做法 ===" << endl;
    Item* safePtr = new Item(99);
    cout << "  safePtr->id = " << safePtr->id << endl;
    delete safePtr;
    
    return 0;
}
```

### 陷阱 3：容器中的對象生命週期

```cpp
#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Element {
private:
    string name;
public:
    Element(const string& n) : name(n) {
        cout << "  [+] " << name << endl;
    }
    // 拷貝建構函數（vector 操作時會用到）
    Element(const Element& other) : name(other.name + "(副本)") {
        cout << "  [拷貝+] " << name << endl;
    }
    ~Element() {
        cout << "  [-] " << name << endl;
    }
};

int main() {
    cout << "=== vector 中的對象生命週期 ===" << endl;
    
    {
        vector<Element> vec;
        cout << "\n--- push_back ---" << endl;
        vec.push_back(Element("A"));   // 臨時對象 → 拷貝進 vector → 臨時對象死亡
        
        cout << "\n--- 再 push_back ---" << endl;
        vec.push_back(Element("B"));   // vector 可能重新分配記憶體
        // 如果重新分配，舊的元素會被拷貝到新位置，然後舊的被解構
        
        cout << "\n--- 離開區塊 ---" << endl;
    }
    // vector 解構，裡面所有元素也被解構
    
    cout << "\n--- 完成 ---" << endl;
    return 0;
}
```

### 預期輸出（可能因編譯器優化而略有不同）

```
=== vector 中的對象生命週期 ===

--- push_back ---
  [+] A
  [拷貝+] A(副本)
  [-] A

--- 再 push_back ---
  [+] B
  [拷貝+] B(副本)
  [拷貝+] A(副本)(副本)
  [-] A(副本)
  [-] B

--- 離開區塊 ---
  [-] A(副本)(副本)
  [-] B(副本)

--- 完成 ---
```

注意第二次 `push_back` 時，`vector` 容量不夠，重新分配記憶體，把原來的 A(副本) 拷貝到新位置，然後舊的被解構。這就是為什麼 `vector` 在大量插入時最好先 `reserve()` 預留空間。

---

## 18.10 完整綜合範例：模擬遊戲場景管理

```cpp
#include <iostream>
#include <string>
using namespace std;

// ============================================================
// 遊戲實體：追蹤完整生命週期
// ============================================================
class Entity {
private:
    string name;
    string type;
    static int totalAlive;

public:
    Entity(const string& n, const string& t) 
        : name(n), type(t) 
    {
        totalAlive++;
        cout << "  [產生] " << type << " \"" << name 
             << "\" (場上: " << totalAlive << ")" << endl;
    }
    
    ~Entity() {
        totalAlive--;
        cout << "  [消滅] " << type << " \"" << name 
             << "\" (場上: " << totalAlive << ")" << endl;
    }
    
    void action(const string& act) const {
        cout << "  [動作] " << name << " " << act << endl;
    }
    
    static int getAlive() { return totalAlive; }
};

int Entity::totalAlive = 0;

// 模擬戰鬥階段
void battlePhase() {
    cout << "\n  ── 戰鬥階段開始 ──" << endl;
    
    Entity enemy1("哥布林", "敵人");
    Entity enemy2("骷髏兵", "敵人");
    
    enemy1.action("發起攻擊");
    enemy2.action("施放魔法");
    
    {
        // 召喚物只在這個區塊內存在
        Entity summon("火元素", "召喚物");
        summon.action("燃燒一切");
    }
    // 火元素在這裡消失
    
    cout << "  召喚物已消失，繼續戰鬥..." << endl;
    enemy1.action("被擊敗");
    
    cout << "  ── 戰鬥階段結束 ──" << endl;
}

int main() {
    cout << "============================================" << endl;
    cout << "   第 18 課：對象生命週期 綜合範例" << endl;
    cout << "============================================" << endl;
    
    cout << "\n=== 場景初始化 ===" << endl;
    Entity player("勇者", "玩家");
    Entity npc("村長", "NPC");
    
    npc.action("說：歡迎來到新手村！");
    player.action("接受任務");
    
    cout << "\n=== 進入戰鬥 ===" << endl;
    battlePhase();
    
    cout << "\n=== 戰鬥結束，回到村莊 ===" << endl;
    cout << "  目前場上實體: " << Entity::getAlive() << endl;
    
    // 動態創建 Boss
    cout << "\n=== Boss 登場 ===" << endl;
    Entity* boss = new Entity("魔王", "Boss");
    boss->action("降臨！");
    player.action("迎戰魔王");
    boss->action("被消滅");
    
    cout << "\n=== 清理 Boss ===" << endl;
    delete boss;
    
    cout << "\n=== 遊戲結束 ===" << endl;
    cout << "  目前場上實體: " << Entity::getAlive() << endl;
    
    return 0;
}
```

### 編譯與執行

```bash
g++ -std=c++17 -o lesson18 lesson18.cpp
./lesson18
```

### 預期輸出

```
============================================
   第 18 課：對象生命週期 綜合範例
============================================

=== 場景初始化 ===
  [產生] 玩家 "勇者" (場上: 1)
  [產生] NPC "村長" (場上: 2)
  [動作] 村長 說：歡迎來到新手村！
  [動作] 勇者 接受任務

=== 進入戰鬥 ===

  ── 戰鬥階段開始 ──
  [產生] 敵人 "哥布林" (場上: 3)
  [產生] 敵人 "骷髏兵" (場上: 4)
  [動作] 哥布林 發起攻擊
  [動作] 骷髏兵 施放魔法
  [產生] 召喚物 "火元素" (場上: 5)
  [動作] 火元素 燃燒一切
  [消滅] 召喚物 "火元素" (場上: 4)
  召喚物已消失，繼續戰鬥...
  [動作] 哥布林 被擊敗
  ── 戰鬥階段結束 ──
  [消滅] 敵人 "骷髏兵" (場上: 3)
  [消滅] 敵人 "哥布林" (場上: 2)

=== 戰鬥結束，回到村莊 ===
  目前場上實體: 2

=== Boss 登場 ===
  [產生] Boss "魔王" (場上: 3)
  [動作] 魔王 降臨！
  [動作] 勇者 迎戰魔王
  [動作] 魔王 被消滅

=== 清理 Boss ===
  [消滅] Boss "魔王" (場上: 2)

=== 遊戲結束 ===
  目前場上實體: 2
  [消滅] NPC "村長" (場上: 1)
  [消滅] 玩家 "勇者" (場上: 0)
```

整個生命週期的完整時間線：

```
時間 →
勇者    ──|══════════════════════════════════════════════|── 解構
村長    ────|════════════════════════════════════════════|── 解構
哥布林      ────|════════════════════|── 解構
骷髏兵        ──|══════════════════|── 解構
火元素          ──|══════|── 解構
魔王                                    ──|══════|── delete
```

---

## 18.11 本課重點回顧

| 概念 | 說明 |
|------|------|
| 四種存儲期 | 自動（棧）、靜態（全域/局部 static）、動態（new/delete） |
| 自動存儲期 | 離開作用域自動解構，最常用最安全 |
| 靜態局部對象 | 第一次執行到時初始化，程式結束時解構，線程安全（C++11） |
| 全域對象陷阱 | 不同編譯單元的全域對象初始化順序未定義 |
| 臨時對象 | 語句結束時死亡，`const&` 可延長其生命 |
| 懸空引用 | 不要返回局部對象的引用或指標 |
| 解構順序 | 與建構順序相反（LIFO） |
| for 迴圈 | 每次迭代都是一次完整的建構→解構週期 |

---

## 18.12 下一課預告

下一課是第三階段的最後一課——**動態對象的創建與銷毀（new / delete）**。我們將深入探討 `new` / `delete` 的底層機制、陣列版本 `new[]` / `delete[]`、placement new，以及為什麼現代 C++ 建議盡量避免裸 `new`。

準備好進入 **第 19 課：動態對象的創建與銷毀（new / delete）** 了嗎？
*/



#include <iostream>
#include <string>
using namespace std;

// ============================================================
// 遊戲實體：追蹤完整生命週期
// ============================================================
class Entity {
private:
    string name;
    string type;
    static int totalAlive;

public:
    Entity(const string& n, const string& t) 
        : name(n), type(t) 
    {
        totalAlive++;
        cout << "  [產生] " << type << " \"" << name 
             << "\" (場上: " << totalAlive << ")" << endl;
    }
    
    ~Entity() {
        totalAlive--;
        cout << "  [消滅] " << type << " \"" << name 
             << "\" (場上: " << totalAlive << ")" << endl;
    }
    
    void action(const string& act) const {
        cout << "  [動作] " << name << " " << act << endl;
    }
    
    static int getAlive() { return totalAlive; }
};

int Entity::totalAlive = 0;

// 模擬戰鬥階段
void battlePhase() {
    cout << "\n  ── 戰鬥階段開始 ──" << endl;
    
    Entity enemy1("哥布林", "敵人");
    Entity enemy2("骷髏兵", "敵人");
    
    enemy1.action("發起攻擊");
    enemy2.action("施放魔法");
    
    {
        // 召喚物只在這個區塊內存在
        Entity summon("火元素", "召喚物");
        summon.action("燃燒一切");
    }
    // 火元素在這裡消失
    
    cout << "  召喚物已消失，繼續戰鬥..." << endl;
    enemy1.action("被擊敗");
    
    cout << "  ── 戰鬥階段結束 ──" << endl;
}

int main() {
    cout << "============================================" << endl;
    cout << "   第 18 課：對象生命週期 綜合範例" << endl;
    cout << "============================================" << endl;
    
    cout << "\n=== 場景初始化 ===" << endl;
    Entity player("勇者", "玩家");
    Entity npc("村長", "NPC");
    
    npc.action("說：歡迎來到新手村！");
    player.action("接受任務");
    
    cout << "\n=== 進入戰鬥 ===" << endl;
    battlePhase();
    
    cout << "\n=== 戰鬥結束，回到村莊 ===" << endl;
    cout << "  目前場上實體: " << Entity::getAlive() << endl;
    
    // 動態創建 Boss
    cout << "\n=== Boss 登場 ===" << endl;
    Entity* boss = new Entity("魔王", "Boss");
    boss->action("降臨！");
    player.action("迎戰魔王");
    boss->action("被消滅");
    
    cout << "\n=== 清理 Boss ===" << endl;
    delete boss;
    
    cout << "\n=== 遊戲結束 ===" << endl;
    cout << "  目前場上實體: " << Entity::getAlive() << endl;
    
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 18 課：對象的生命週期（Object Lifetime）11.cpp" -o lifetime11

// ── 輸出但書 ────────────────────────────────────────────────────────────
// 1. 本檔輸出由語言規則決定，逐位元組可重現（實測連跑 10 次 md5 相同）。
// 2. 三個關鍵對照：
//    (a) 戰鬥階段中，火元素（內層區塊）先於骷髏兵、哥布林消滅，
//        接著兩名敵人以建構順序的反序消滅 —— LIFO 規則。
//    (b) delete boss 之後才出現 [消滅] Boss，證明解構是由 delete 觸發的；
//        若刪掉那一行，這一行輸出永遠不會出現，且場上數回不到 0。
//    (c) main 結束時，村長 → 勇者 反序消滅，最終場上實體為 0
//        —— 沒有任何實體洩漏。
// 3. 本檔的解構函數是「先 --totalAlive 再輸出」，所以印出的數字
//    已不包含自己；這與第 17 課範例 7 刻意印 count - 1 的寫法不同，
//    兩者都正確，重點是同一份程式碼內要一致。
// 4. 本機環境：GCC 15.2.0 (Ubuntu 15.2.0-16ubuntu1) / libstdc++ / x86-64。

// === 預期輸出 ===
// ============================================
//    第 18 課：對象生命週期 綜合範例
// ============================================
//
// === 場景初始化 ===
//   [產生] 玩家 "勇者" (場上: 1)
//   [產生] NPC "村長" (場上: 2)
//   [動作] 村長 說：歡迎來到新手村！
//   [動作] 勇者 接受任務
//
// === 進入戰鬥 ===
//
//   ── 戰鬥階段開始 ──
//   [產生] 敵人 "哥布林" (場上: 3)
//   [產生] 敵人 "骷髏兵" (場上: 4)
//   [動作] 哥布林 發起攻擊
//   [動作] 骷髏兵 施放魔法
//   [產生] 召喚物 "火元素" (場上: 5)
//   [動作] 火元素 燃燒一切
//   [消滅] 召喚物 "火元素" (場上: 4)
//   召喚物已消失，繼續戰鬥...
//   [動作] 哥布林 被擊敗
//   ── 戰鬥階段結束 ──
//   [消滅] 敵人 "骷髏兵" (場上: 3)
//   [消滅] 敵人 "哥布林" (場上: 2)
//
// === 戰鬥結束，回到村莊 ===
//   目前場上實體: 2
//
// === Boss 登場 ===
//   [產生] Boss "魔王" (場上: 3)
//   [動作] 魔王 降臨！
//   [動作] 勇者 迎戰魔王
//   [動作] 魔王 被消滅
//
// === 清理 Boss ===
//   [消滅] Boss "魔王" (場上: 2)
//
// === 遊戲結束 ===
//   目前場上實體: 2
//   [消滅] NPC "村長" (場上: 1)
//   [消滅] 玩家 "勇者" (場上: 0)
