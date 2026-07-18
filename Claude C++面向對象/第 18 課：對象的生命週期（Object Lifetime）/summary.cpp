/*
 * ================================================================
 * 【第 18 課：對象的生命週期（Object Lifetime）】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. 四種存儲期（Storage Duration）：自動（棧）、靜態（全域）、
 *    靜態（局部 static）、動態（堆）
 * 2. 作用域嵌套觀察（解構順序如堆疊 LIFO）
 * 3. if / for / switch 中的對象生命週期
 *    （for 迴圈每次迭代都是完整建構 -> 解構）
 * 4. 靜態局部對象的延遲初始化（Lazy Initialization）
 *    只初始化一次，C++11 保證線程安全
 * 5. 全域對象初始化順序陷阱（Static Initialization Order Fiasco）
 *    不同編譯單元的全域對象初始化順序未定義
 *    解決方案：用函數包裝靜態局部對象
 * 6. 臨時對象的生命週期：語句結束時死亡、用變數接住延長生命、
 *    const 引用延長臨時對象生命
 * 7. 常見陷阱：懸空引用（返回局部對象的引用）、
 *    野指標（返回局部對象的地址）
 * 8. vector 中的對象生命週期（push_back 觸發拷貝和重新分配）
 * 9. 綜合範例：遊戲場景管理（Entity with static 計數器）
 * ================================================================
 */

#include <iostream>
#include <string>
#include <vector>
using namespace std;

// ================================================================
// 重點一：對象的生命週期是什麼？
// ================================================================
// 對象的生命週期是指從「建構函數完成」到「解構函數開始」之間的時間。
//
//     建構函數執行        對象存活期間          解構函數執行
//     +-----------+  +------------------+  +-----------+
//  ---| 初始化    |--| 可以安全使用      |--| 清理      |---
//     +-----------+  +------------------+  +-----------+
//     ^                                                ^
//    誕生                                             死亡
//
// C++ 有四種存儲期，每種都有不同的生命週期規則：
// +----------+------------+----------------+------------------+-----------------------+
// | 存儲期   | 宣告位置   | 誕生           | 死亡             | 特點                  |
// +----------+------------+----------------+------------------+-----------------------+
// | 自動(棧) | 函數/區塊內| 執行到宣告時   | 離開作用域時     | 最常用，自動管理      |
// | 靜態(全域)| 函數外    | main() 之前    | main() 之後      | 整個程式存活          |
// | 靜態(局部)| 函數內static| 第一次執行到時| main() 之後      | 只初始化一次          |
// | 動態(堆) | new 創建   | new 時         | delete 時        | 手動管理，忘記就洩漏  |
// +----------+------------+----------------+------------------+-----------------------+

// ================================================================
// 通用生命週期探針類別（用於觀察建構與解構）
// ================================================================
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

// ================================================================
// 重點二：四種存儲期的演示
// ================================================================
// 1. 靜態存儲期（全域對象）：main() 之前建構，main() 之後解構
Probe globalObj("全域物件");

void demoStorageDuration() {
    // 2. 自動存儲期（局部對象）：離開作用域自動解構
    Probe localObj("func 局部物件");
    localObj.hello();

    // 3. 靜態存儲期（靜態局部對象）：第一次執行到時初始化，程式結束才解構
    static Probe staticLocal("func 靜態局部物件");
    staticLocal.hello();
}

// ================================================================
// 重點三：作用域嵌套觀察（LIFO 順序）
// ================================================================
// 解構順序與建構順序相反，像堆疊一樣後進先出：
//
// 進入 main：    [a]
// 進入第一層：   [a][b]
// 進入第二層：   [a][b][c]
// 離開第二層：   [a][b]       <- c 解構
// 離開第一層：   [a]          <- b 解構
// 離開 main：    []           <- a 解構

class ScopeTracker {
private:
    string name;
public:
    ScopeTracker(const string& n) : name(n) {
        cout << "  [+] " << name << endl;
    }
    ~ScopeTracker() {
        cout << "  [-] " << name << endl;
    }
};

void demoNestedScope() {
    ScopeTracker a("a - 外層");
    {
        ScopeTracker b("b - 第一層");
        {
            ScopeTracker c("c - 第二層");
            cout << "  --- 最深處 ---" << endl;
        }   // c 死亡
        cout << "  --- 回到第一層 ---" << endl;
    }   // b 死亡
    cout << "  --- 回到外層 ---" << endl;
}   // a 死亡

// ================================================================
// 重點四：if / for 中的對象生命週期
// ================================================================
// for 迴圈的重要觀察：迴圈體內的對象在每次迭代結束時解構，
// 下次迭代開始時重新建構。每次迭代都是一個獨立的生命週期。

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

void demoControlFlowLifetime() {
    // --- if 語句中的對象 ---
    cout << "\n  --- if 語句 ---" << endl;
    if (bool condition = true) {
        Tracker t("if 區塊物件");
        t.work();
    }   // t 在這裡死亡
    cout << "  if 區塊已結束" << endl;

    // --- for 迴圈中的對象（每次迭代建構一次、解構一次）---
    cout << "\n  --- for 迴圈（每次迭代都是完整建構->解構）---" << endl;
    for (int i = 0; i < 3; i++) {
        Tracker t("迴圈物件 #" + to_string(i));
        t.work();
        // t 在每次迭代結束時死亡，下次迭代重新建構
    }
    cout << "  for 迴圈已結束" << endl;

    // --- for 初始化區的對象生命週期 ---
    cout << "\n  --- for 外圍 vs 內部對象 ---" << endl;
    {
        Tracker outer("迴圈外圍物件");
        for (int i = 0; i < 2; i++) {
            Tracker inner("迴圈內部 #" + to_string(i));
            inner.work();
        }
    }   // outer 在這裡死亡
}

// ================================================================
// 重點五：靜態局部對象的延遲初始化（Lazy Initialization）
// ================================================================
// 靜態局部對象只在第一次執行到時才建構，之後不再重複建構。
// C++11 保證這個初始化是線程安全的（Magic Statics）。
// 這是單例模式（Singleton）的基礎原理。

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

void demoLazyInit() {
    cout << "  (程式啟動但還沒使用資源，資源尚未被建構)" << endl;

    cout << "\n  --- 第一次調用 getResource() ---" << endl;
    getResource().use();    // 第一次：觸發建構，然後使用

    cout << "  --- 第二次調用 getResource() ---" << endl;
    getResource().use();    // 第二次：不再建構，直接使用

    cout << "  --- 第三次調用 getResource() ---" << endl;
    getResource().use();    // 第三次：同上
    // 程式結束時，靜態局部對象才被解構
}

// ================================================================
// 重點六：全域對象初始化順序陷阱
//        （Static Initialization Order Fiasco）
// ================================================================
// 問題：如果兩個全域對象在不同的 .cpp 檔案中，
//       C++ 標準不保證它們的初始化順序！
//
// 危險寫法（概念示範，此處在同一檔案中是安全的）：
//   // 檔案 A:  Config config;
//   // 檔案 B:  ConnectionPool pool(config);  // config 可能還沒初始化！
//
// 解決方案：用函數包裝靜態局部對象，保證初始化順序。

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

class ConnectionPool {
private:
    int poolSize;
public:
    ConnectionPool(int size) : poolSize(size) {
        cout << "  [ConnectionPool] 初始化: poolSize = "
             << poolSize << endl;
    }
    int getSize() const { return poolSize; }
};

// 安全的做法：用函數包裝，保證初始化順序
Config& getConfig() {
    static Config config;       // 第一次調用時建構
    return config;
}

ConnectionPool& getPool() {
    // getConfig() 保證在使用前就已初始化
    static ConnectionPool pool(getConfig().getMax());
    return pool;
}

void demoStaticInitOrderFiasco() {
    // 展示安全的初始化方式
    cout << "  --- 透過函數包裝保證初始化順序 ---" << endl;
    cout << "  Pool size = " << getPool().getSize() << endl;
    getPool();   // 第二次調用，不會重複初始化
    cout << "  (第二次調用 getPool()，不再重複初始化)" << endl;
}

// ================================================================
// 重點七：臨時對象的生命週期
// ================================================================
// +--------------------------------+----------------------------------+
// | 情境                           | 生命週期                          |
// +--------------------------------+----------------------------------+
// | Temp("x").show();              | 到語句的分號 ; 就死亡             |
// | Temp t = Temp("x");            | 和 t 的生命週期相同               |
// | const Temp& r = Temp("x");     | 延長到 r 離開作用域               |
// | Temp&& rr = Temp("x");         | 延長到 rr 離開作用域（右值引用）  |
// +--------------------------------+----------------------------------+

class TempObj {
private:
    string name;
public:
    TempObj(const string& n) : name(n) {
        cout << "  [+] " << name << endl;
    }
    ~TempObj() {
        cout << "  [-] " << name << endl;
    }
    string getName() const { return name; }

    // 返回自身引用，用於鏈式調用
    const TempObj& show() const {
        cout << "  [=] " << name << " 展示中" << endl;
        return *this;
    }
};

TempObj createTemp(const string& n) {
    return TempObj(n);   // 返回一個臨時對象
}

void demoTemporaryLifetime() {
    // 情境 1：臨時對象在語句結束時死亡
    cout << "  --- 情境 1：純臨時對象（語句結束即死亡）---" << endl;
    TempObj("短命鬼").show();
    cout << "  (臨時對象已死亡)\n" << endl;

    // 情境 2：用變數接住，延長生命
    cout << "  --- 情境 2：用變數接住延長生命 ---" << endl;
    TempObj saved = createTemp("被拯救的");
    saved.show();
    cout << "  (saved 還活著)\n" << endl;

    // 情境 3：const 引用延長臨時對象的生命
    cout << "  --- 情境 3：const 引用延長臨時對象生命 ---" << endl;
    const TempObj& ref = TempObj("引用續命");
    ref.show();
    cout << "  (ref 綁定的臨時對象還活著)" << endl;
    // 臨時對象的生命被延長到 ref 離開作用域時
}

// ================================================================
// 重點八：常見陷阱 —— 懸空引用 & 野指標
// ================================================================

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

// 陷阱 1：返回局部對象的引用 —— 懸空引用（Dangling Reference）
// Data& dangerous() {
//     Data local(42);       // local 是局部對象
//     return local;         // 返回 local 的引用
// }   // local 在這裡死亡！返回的引用指向已死的對象！
// 編譯器通常會警告：returning reference to local variable

// 安全：返回值（複製 / 編譯器可能優化為 RVO）
Data safe() {
    Data local(42);
    return local;   // 返回副本（編譯器可能優化掉複製）
}

// 陷阱 2：返回局部對象的地址 —— 野指標（Dangling Pointer）
// Item* dangerousPointer() {
//     Item local(99);
//     return &local;    // 返回局部對象的地址 —— 危險！
// }   // local 死亡，返回的指標成為野指標

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

void demoDanglingTraps() {
    // --- 懸空引用的安全替代：返回值 ---
    cout << "  --- 安全做法：返回值（避免懸空引用）---" << endl;
    Data d = safe();
    cout << "  d.value = " << d.value << endl;

    // --- 野指標的安全替代：用 new 分配 ---
    cout << "\n  --- 安全做法：用 new 分配（避免野指標）---" << endl;
    Item* safePtr = new Item(99);
    cout << "  safePtr->id = " << safePtr->id << endl;
    delete safePtr;

    cout << "\n  --- 陷阱總結 ---" << endl;
    cout << "  1. 不要返回局部對象的引用 -> 懸空引用（Dangling Reference）" << endl;
    cout << "  2. 不要返回局部對象的地址 -> 野指標（Dangling Pointer）" << endl;
    cout << "  3. 安全做法：返回值（副本）或用 new 分配動態記憶體" << endl;
}

// ================================================================
// 重點九：vector 中的對象生命週期
// ================================================================
// push_back 會觸發拷貝建構函數。
// 當 vector 容量不夠時，會重新分配記憶體，把舊元素拷貝到新位置，
// 然後解構舊的元素。這就是為什麼大量插入時建議先 reserve()。

class Element {
private:
    string name;
public:
    Element(const string& n) : name(n) {
        cout << "  [+] " << name << endl;
    }
    // 拷貝建構函數（vector 操作時會觸發）
    Element(const Element& other) : name(other.name + "(副本)") {
        cout << "  [拷貝+] " << name << endl;
    }
    ~Element() {
        cout << "  [-] " << name << endl;
    }
};

void demoVectorLifetime() {
    {
        vector<Element> vec;
        cout << "  --- push_back(Element(\"A\")) ---" << endl;
        vec.push_back(Element("A"));   // 臨時對象 -> 拷貝進 vector -> 臨時對象死亡

        cout << "\n  --- push_back(Element(\"B\"))（可能觸發重新分配）---" << endl;
        vec.push_back(Element("B"));   // vector 可能重新分配記憶體
        // 如果重新分配，舊的元素會被拷貝到新位置，然後舊的被解構

        cout << "\n  --- 離開區塊（vector 解構，所有元素也被解構）---" << endl;
    }
    // vector 解構，裡面所有元素也被解構
}

// ================================================================
// 重點十：綜合範例 —— 遊戲場景管理
// ================================================================
// 使用 static 計數器追蹤場上存活實體數量，
// 展示自動、動態、嵌套作用域中對象的完整生命週期。

class Entity {
private:
    string name;
    string type;
    static int totalAlive;  // 靜態成員：追蹤全部存活實體數

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

// 模擬戰鬥階段（函數作用域內的自動對象生命週期）
void battlePhase() {
    cout << "\n  -- 戰鬥階段開始 --" << endl;

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

    cout << "  -- 戰鬥階段結束 --" << endl;
}

void demoGameScene() {
    Entity player("勇者", "玩家");
    Entity npc("村長", "NPC");

    npc.action("說：歡迎來到新手村！");
    player.action("接受任務");

    cout << "\n  === 進入戰鬥 ===" << endl;
    battlePhase();

    cout << "\n  === 戰鬥結束，回到村莊 ===" << endl;
    cout << "  目前場上實體: " << Entity::getAlive() << endl;

    // 動態創建 Boss
    cout << "\n  === Boss 登場 ===" << endl;
    Entity* boss = new Entity("魔王", "Boss");
    boss->action("降臨！");
    player.action("迎戰魔王");
    boss->action("被消滅");

    cout << "\n  === 清理 Boss ===" << endl;
    delete boss;

    cout << "\n  === 遊戲結束 ===" << endl;
    cout << "  目前場上實體: " << Entity::getAlive() << endl;
}

// ================================================================
// 主程式：依序展示所有重點
// ================================================================
int main() {
    cout << "=============================================" << endl;
    cout << "   第 18 課：對象的生命週期（Object Lifetime）" << endl;
    cout << "=============================================" << endl;

    // ----------------------------------------------------------
    // 【1】四種存儲期
    // ----------------------------------------------------------
    cout << "\n【1】四種存儲期（Storage Duration）" << endl;
    cout << "  (全域物件在 main() 之前已建構，見上方輸出)\n" << endl;

    cout << "  --- 第一次調用 demoStorageDuration() ---" << endl;
    demoStorageDuration();
    cout << "\n  --- 第二次調用 demoStorageDuration() ---" << endl;
    demoStorageDuration();
    // 觀察：局部物件每次都重建，靜態局部物件第二次不再建構

    cout << "\n  --- 動態存儲期（堆上對象）---" << endl;
    Probe* heapObj = new Probe("動態物件");
    heapObj->hello();
    delete heapObj;
    cout << "  (delete 時才解構動態物件)" << endl;

    // ----------------------------------------------------------
    // 【2】作用域嵌套觀察（LIFO 解構順序）
    // ----------------------------------------------------------
    cout << "\n【2】作用域嵌套觀察（解構順序如堆疊 LIFO）" << endl;
    demoNestedScope();

    // ----------------------------------------------------------
    // 【3】if / for 中的對象生命週期
    // ----------------------------------------------------------
    cout << "\n【3】if / for 中的對象生命週期" << endl;
    demoControlFlowLifetime();

    // ----------------------------------------------------------
    // 【4】靜態局部對象的延遲初始化（Lazy Initialization）
    // ----------------------------------------------------------
    cout << "\n【4】靜態局部對象的延遲初始化" << endl;
    cout << "  C++11 保證 static 局部變數初始化是線程安全的" << endl;
    demoLazyInit();

    // ----------------------------------------------------------
    // 【5】全域對象初始化順序陷阱
    // ----------------------------------------------------------
    cout << "\n【5】全域對象初始化順序陷阱（Static Initialization Order Fiasco）" << endl;
    cout << "  問題：不同 .cpp 檔案中的全域對象初始化順序是未定義的！" << endl;
    cout << "  解決方案：用函數包裝靜態局部對象\n" << endl;
    demoStaticInitOrderFiasco();

    // ----------------------------------------------------------
    // 【6】臨時對象的生命週期
    // ----------------------------------------------------------
    cout << "\n【6】臨時對象的生命週期" << endl;
    demoTemporaryLifetime();

    // ----------------------------------------------------------
    // 【7】常見陷阱：懸空引用 & 野指標
    // ----------------------------------------------------------
    cout << "\n【7】常見陷阱：懸空引用 & 野指標" << endl;
    demoDanglingTraps();

    // ----------------------------------------------------------
    // 【8】vector 中的對象生命週期
    // ----------------------------------------------------------
    cout << "\n【8】vector 中的對象生命週期" << endl;
    cout << "  push_back 會觸發拷貝，容量不夠時會重新分配記憶體" << endl;
    cout << "  建議：大量插入前先 reserve() 預留空間\n" << endl;
    demoVectorLifetime();

    // ----------------------------------------------------------
    // 【9】綜合範例：遊戲場景管理
    // ----------------------------------------------------------
    cout << "\n【9】綜合範例：遊戲場景管理" << endl;
    demoGameScene();

    // ----------------------------------------------------------
    // 重點回顧
    // ----------------------------------------------------------
    cout << "\n=============================================" << endl;
    cout << "本課重點回顧：" << endl;
    cout << "  1. 四種存儲期：自動(棧)、靜態(全域/局部static)、動態(new/delete)" << endl;
    cout << "  2. 自動存儲期：離開作用域自動解構，最常用最安全" << endl;
    cout << "  3. 解構順序與建構順序相反（LIFO，像堆疊）" << endl;
    cout << "  4. for 迴圈：每次迭代都是完整的建構->解構週期" << endl;
    cout << "  5. 靜態局部對象：延遲初始化，只初始化一次，C++11 線程安全" << endl;
    cout << "  6. 全域對象陷阱：不同編譯單元初始化順序未定義" << endl;
    cout << "     解決：用函數包裝靜態局部對象" << endl;
    cout << "  7. 臨時對象：語句結束時死亡，const& 可延長其生命" << endl;
    cout << "  8. 懸空引用：不要返回局部對象的引用或指標" << endl;
    cout << "  9. vector push_back 會觸發拷貝，容量不夠時重新分配" << endl;
    cout << "=============================================" << endl;

    return 0;
}
// 程式結束時解構順序：
// 靜態局部對象（getResource, getConfig, getPool 中的 static 對象）
// 全域物件（globalObj）
// 解構順序與建構順序相反
