/*
 * ============================================================================
 *  第 22 課：const 成員函數 — 綜合總結
 * ============================================================================
 *
 *  本檔案涵蓋第 22 課的所有核心概念，閱讀本檔案即可完整複習，
 *  無需回頭查看其他 cpp 檔案。
 *
 *  目錄：
 *    第一節：const 成員函數基礎 — 什麼是 const 成員函數？語法與強制力
 *    第二節：const 對象的限制 — const 對象只能調用 const 成員函數
 *    第三節：const 正確性 — 好設計 vs 壞設計的對比
 *    第四節：this 指標與 const — 底層原理解析
 *    第五節：const 重載 — 同名函數的 const / 非 const 版本共存
 *    第六節：const 函數調用鏈 — const 函數之間的互相調用規則
 *    第七節：實際應用 — const 引用參數在真實場景中的威力
 *    第八節：常見錯誤與陷阱 — 淺層 const、忘記加 const 等
 *    第九節：const 正確性檢查清單 — 寫完函數後的自我檢查
 *
 * ============================================================================
 */

#include <iostream>
#include <string>
#include <vector>
using namespace std;

/* ============================================================================
 *  第一節：const 成員函數基礎
 * ============================================================================
 *
 *  const 成員函數的核心概念：
 *    - 在函數參數列表的右括號「之後」、函數體的「之前」加上 const 關鍵字
 *    - 語法：  返回類型 函數名(參數) const { ... }
 *    - 語義：  承諾「不修改任何成員變數」，編譯器會強制執行這個承諾
 *    - 如果在 const 函數體內嘗試修改成員變數，編譯器會報錯
 *
 *  何時應該使用 const：
 *    - 所有 getter 函數（getName, getHp 等）
 *    - 所有只讀/查詢函數（printInfo, isAlive 等）
 *    - 任何不修改對象狀態的函數
 *
 *  何時不應該使用 const：
 *    - 會修改成員變數的函數（setter、use、takeDamage 等）
 *
 * ============================================================================
 */

// 示範類別：藥水 — 展示 const 與非 const 成員函數的基本區別
class Potion {
private:
    string name_;       // 藥水名稱
    int healAmount_;    // 回復量
    int quantity_;      // 持有數量

public:
    Potion(const string& name, int heal, int qty)
        : name_(name), healAmount_(heal), quantity_(qty)
    {
    }

    // ------ const 成員函數：承諾不修改對象 ------
    // 這些函數只讀取成員變數，不修改它們，因此標記為 const
    // 加了 const 後，這些函數可以在 const 對象上被調用
    const string& getName() const { return name_; }         // 返回 const 引用，避免拷貝
    int getHealAmount() const { return healAmount_; }       // 返回值拷貝，不需要 const 修飾返回值
    int getQuantity() const { return quantity_; }

    void printInfo() const {
        cout << "  " << name_ << " (回復:" << healAmount_
             << " 數量:" << quantity_ << ")" << endl;

        // 以下會編譯錯誤！const 函數不能修改成員
        // quantity_ = 0;       // 錯誤！不能在 const 函數中修改成員變數
        // name_ = "被篡改";    // 錯誤！不能在 const 函數中修改成員變數
    }

    // ------ 非 const 成員函數：可以修改對象 ------
    // 這些函數會修改成員變數 quantity_，所以不能標記為 const
    bool use() {
        if (quantity_ <= 0) {
            cout << "  " << name_ << " 已用完！" << endl;
            return false;
        }
        quantity_--;    // 修改了成員變數，因此本函數不能是 const
        cout << "  使用 " << name_ << "，回復 " << healAmount_
             << " HP (剩餘:" << quantity_ << ")" << endl;
        return true;
    }

    void restock(int amount) {
        if (amount > 0) {
            quantity_ += amount;    // 修改了成員變數
            cout << "  補貨 " << name_ << " +" << amount
                 << " (總計:" << quantity_ << ")" << endl;
        }
    }
};

/* ============================================================================
 *  第二節：const 對象的限制
 * ============================================================================
 *
 *  核心規則：
 *    非 const 對象 → 可以調用 const 和非 const 成員函數
 *    const 對象    → 只能調用 const 成員函數，不能調用非 const 成員函數
 *
 *  比喻：
 *    普通人（非 const）→ 可以「看」也可以「碰」展品
 *    參觀者（const）   → 只能「看」，不能「碰」
 *
 *  const 引用參數：
 *    函數參數使用 const 引用（const T&）是非常常見的用法
 *    表示函數內部「只看不碰」，不會修改傳入的對象
 *    在函數體內只能調用該對象的 const 成員函數
 *
 * ============================================================================
 */

// 示範類別：盾牌 — 展示 const 對象的限制
class Shield {
private:
    string name_;
    int defense_;
    int durability_;

public:
    Shield(const string& name, int def, int dur)
        : name_(name), defense_(def), durability_(dur)
    {
    }

    // const 成員函數 — const 對象和非 const 對象都能調用
    const string& getName() const { return name_; }
    int getDefense() const { return defense_; }
    int getDurability() const { return durability_; }

    void printShieldInfo() const {
        cout << "  " << name_ << " [防禦:" << defense_
             << " 耐久:" << durability_ << "]" << endl;
    }

    // 非 const 成員函數 — 只有非 const 對象能調用
    void takeDamage(int dmg) {
        durability_ -= dmg;
        if (durability_ < 0) durability_ = 0;
        cout << "  " << name_ << " 耐久 -" << dmg
             << " (剩餘:" << durability_ << ")" << endl;
    }

    void repair() {
        durability_ = 100;
        cout << "  " << name_ << " 修復完成" << endl;
    }
};

// 接收 const 引用的函數 — 模擬「只看不碰」
// 參數類型是 const Shield&，表示只能讀取盾牌資料，不能修改
void inspectShield(const Shield& s) {
    cout << "\n--- 檢查盾牌（const 引用）---" << endl;

    // 可以調用 const 成員函數
    s.printShieldInfo();
    cout << "  防禦力：" << s.getDefense() << endl;
    cout << "  耐久度：" << s.getDurability() << endl;

    // 不能調用非 const 成員函數 — 因為參數是 const 引用
    // s.takeDamage(10);   // 編譯錯誤！
    // s.repair();          // 編譯錯誤！
}

/* ============================================================================
 *  第三節：const 正確性（const correctness）
 * ============================================================================
 *
 *  const 正確性是 C++ 中非常重要的設計原則：
 *    「所有不修改對象狀態的成員函數，都應該標記為 const」
 *
 *  忘記加 const 的後果：
 *    當別人用 const 引用接收你的對象時，連 getter 都不能調用
 *    這是非常常見的 C++ 新手錯誤
 *
 *  以下用 BadDesign vs GoodDesign 對比說明
 *
 * ============================================================================
 */

// 反面教材：忘記加 const
class BadDesign {
private:
    string name_;
    int value_;

public:
    BadDesign(const string& n, int v) : name_(n), value_(v) {}

    // 這些函數明明不修改對象，卻忘記加 const！
    // 導致：無法在 const 對象或 const 引用上調用這些函數
    string getName() { return name_; }                                  // 缺少 const
    int getValue() { return value_; }                                   // 缺少 const
    void printBad() { cout << name_ << ":" << value_ << endl; }        // 缺少 const
};

// 正確設計：const 正確
class GoodDesign {
private:
    string name_;
    int value_;

public:
    GoodDesign(const string& n, int v) : name_(n), value_(v) {}

    // 所有不修改對象的函數都正確標記為 const
    const string& getName() const { return name_; }
    int getValue() const { return value_; }
    void printGood() const { cout << name_ << ":" << value_ << endl; }

    // 只有修改對象的函數才不加 const
    void setValue(int v) { value_ = v; }
};

// 用 const 引用接收 — 對比效果
void processBad(const BadDesign& b) {
    // b.getName();   // 編譯錯誤！getName 不是 const
    // b.printBad();  // 編譯錯誤！printBad 不是 const
    cout << "  BadDesign：什麼都不能做！" << endl;
}

void processGood(const GoodDesign& g) {
    g.printGood();                                     // 完美運作
    cout << "  name = " << g.getName() << endl;        // 完美運作
    cout << "  value = " << g.getValue() << endl;      // 完美運作
}

/* ============================================================================
 *  第四節：this 指標與 const — 底層原理
 * ============================================================================
 *
 *  每個成員函數都有一個隱含的 this 指標。const 改變的就是 this 的類型：
 *
 *  普通成員函數：
 *    void setHp(int hp)
 *    隱含參數：Player* const this
 *    → this 指標本身是 const（不能指向別的對象）
 *    → 但 this 指向的內容可以修改
 *
 *  const 成員函數：
 *    int getHp() const
 *    隱含參數：const Player* const this
 *    → this 指標本身是 const
 *    → this 指向的內容「也是 const」（不能修改！）
 *
 *  圖解：
 *    普通成員函數的 this：
 *      this --> [ name_ | hp_ | maxHp_ ]
 *               可讀寫   可讀寫  可讀寫
 *
 *    const 成員函數的 this：
 *      this --> [ name_ | hp_ | maxHp_ ]
 *               只讀     只讀    只讀
 *
 * ============================================================================
 */

// 示範類別：Demo — 展示 this 指標在 const 與非 const 函數中的差異
class Demo {
private:
    int value_;

public:
    Demo(int v) : value_(v) {}

    // 普通成員函數：this 的類型是 Demo* const
    // 可以通過 this 修改成員變數
    void modify() {
        this->value_ = 999;        // 可以修改，因為 this 指向的內容非 const
        cout << "  modify(): value_ = " << value_ << endl;
    }

    // const 成員函數：this 的類型是 const Demo* const
    // 不能通過 this 修改成員變數
    void inspect() const {
        // this->value_ = 999;     // 編譯錯誤！this 指向的內容是 const
        cout << "  inspect(): value_ = " << value_ << endl;
    }
};

/* ============================================================================
 *  第五節：const 重載（const overloading）
 * ============================================================================
 *
 *  同一個函數可以同時有 const 和非 const 兩個版本，
 *  編譯器會根據對象是否為 const 來選擇調用哪個版本。
 *
 *  選擇規則：
 *    對象/引用是 const     → 調用 const 版本
 *    對象/引用是非 const   → 調用非 const 版本（如果有的話）
 *                          → 如果沒有非 const 版本，也會調用 const 版本
 *
 *  常見用途：
 *    std::vector 的 operator[] 就有兩個版本：
 *      const 版本返回 const 引用（只讀）
 *      非 const 版本返回普通引用（可讀寫）
 *
 * ============================================================================
 */

// 示範類別：TextBuffer — 展示 const 重載
class TextBuffer {
private:
    string content_;

public:
    TextBuffer(const string& text) : content_(text) {}

    // const 版本：返回 const 引用（只讀）
    // 當 const 對象或 const 引用調用時，編譯器選擇此版本
    const string& getText() const {
        cout << "  [調用 const 版本]" << endl;
        return content_;
    }

    // 非 const 版本：返回非 const 引用（可讀寫）
    // 當非 const 對象調用時，編譯器優先選擇此版本
    // 返回非 const 引用，允許調用者直接修改 content_
    string& getText() {
        cout << "  [調用非 const 版本]" << endl;
        return content_;
    }

    void print() const {
        cout << "  內容：「" << content_ << "」" << endl;
    }
};

/* ============================================================================
 *  第六節：const 函數調用鏈
 * ============================================================================
 *
 *  調用規則：
 *    const 函數 --> const 函數      可以
 *    const 函數 --> 非 const 函數   不行（編譯錯誤）
 *    非 const 函數 --> const 函數   可以
 *    非 const 函數 --> 非 const 函數 可以
 *
 *  簡記：
 *    const 函數只能調用其他 const 函數
 *    非 const 函數可以調用所有函數
 *
 * ============================================================================
 */

// 示範類別：GameCharacter — 展示 const 函數之間的調用鏈
class GameCharacter {
private:
    string name_;
    int hp_;
    int maxHp_;
    int level_;

public:
    GameCharacter(const string& name, int maxHp, int level)
        : name_(name), hp_(maxHp), maxHp_(maxHp), level_(level)
    {
    }

    // ------ const 函數群 ------
    const string& getName() const { return name_; }
    int getHp() const { return hp_; }
    int getMaxHp() const { return maxHp_; }
    int getLevel() const { return level_; }

    // const 函數可以調用其他 const 函數（getHp, getMaxHp 都是 const）
    double getHpPercent() const {
        return (static_cast<double>(getHp()) / getMaxHp()) * 100.0;
    }

    // const 函數可以調用其他 const 函數（getName, getLevel, getHpPercent 都是 const）
    string getStatusText() const {
        string status = getName() + " Lv." + to_string(getLevel());
        double pct = getHpPercent();

        if (pct > 50.0)
            status += " [健康]";
        else if (pct > 20.0)
            status += " [受傷]";
        else if (pct > 0.0)
            status += " [瀕死]";
        else
            status += " [死亡]";

        return status;
    }

    void printFullStatus() const {
        // const 函數調用其他 const 函數 — 完全合法
        cout << "  " << getStatusText() << endl;
        cout << "  HP: " << getHp() << "/" << getMaxHp()
             << " (" << getHpPercent() << "%)" << endl;

        // 不能調用非 const 函數：
        // takeDamage(10);  // 編譯錯誤！const 函數不能調用非 const 函數
    }

    // ------ 非 const 函數 ------
    void takeDamage(int dmg) {
        hp_ = max(0, hp_ - dmg);
        // 非 const 函數可以調用 const 函數 — 完全合法
        cout << "  " << getName() << " 受傷！" << getStatusText() << endl;
    }
};

/* ============================================================================
 *  第七節：實際應用 — const 引用參數的威力
 * ============================================================================
 *
 *  const 成員函數最大的實際價值：
 *    讓你的類別可以安全地通過 const 引用傳遞
 *
 *  常見的接收 const 引用的場景：
 *    1. 分析函數 — 只需要讀取對象資料
 *    2. 比較函數 — 比較兩個對象
 *    3. 列表顯示 — 遍歷容器中的元素
 *    4. 搜尋函數 — 在容器中查找特定元素
 *
 *  如果成員函數沒有標記 const，以上所有場景都會編譯失敗！
 *
 * ============================================================================
 */

// 示範類別：Monster — 展示 const 引用的實際應用
class Monster {
private:
    string name_;
    int hp_;
    int attack_;
    string element_;

public:
    Monster(const string& name, int hp, int atk, const string& elem)
        : name_(name), hp_(hp), attack_(atk), element_(elem)
    {
    }

    // 全部是 const — 讓 Monster 可以安全地通過 const 引用傳遞
    const string& getName() const { return name_; }
    int getHp() const { return hp_; }
    int getAttack() const { return attack_; }
    const string& getElement() const { return element_; }
    bool isAlive() const { return hp_ > 0; }

    void printInfo() const {
        cout << "  " << name_ << " [" << element_ << "] HP:"
             << hp_ << " ATK:" << attack_ << endl;
    }

    // 非 const：會修改狀態
    void takeDamage(int dmg) {
        hp_ = max(0, hp_ - dmg);
    }
};

// 場景 1：分析函數 — 通過 const 引用只讀取資料
void analyzeMonster(const Monster& m) {
    cout << "  分析 " << m.getName() << "：" << endl;
    cout << "    屬性：" << m.getElement() << endl;
    cout << "    威脅等級：";
    if (m.getAttack() >= 50) cout << "高";
    else if (m.getAttack() >= 30) cout << "中";
    else cout << "低";
    cout << endl;
}

// 場景 2：比較函數 — 通過 const 引用比較兩個對象
void compareMonsters(const Monster& a, const Monster& b) {
    cout << "  比較 " << a.getName() << " vs " << b.getName() << "：";
    if (a.getAttack() > b.getAttack())
        cout << a.getName() << " 更強！" << endl;
    else if (a.getAttack() < b.getAttack())
        cout << b.getName() << " 更強！" << endl;
    else
        cout << "不分上下！" << endl;
}

// 場景 3：列表顯示 — 通過 const 引用遍歷容器
void showMonsterList(const vector<Monster>& monsters) {
    cout << "  怪物圖鑑（共 " << monsters.size() << " 隻）：" << endl;
    for (const auto& m : monsters) {
        cout << "    ";
        m.printInfo();  // printInfo 必須是 const 才能在這裡調用
    }
}

// 場景 4：搜尋函數 — 通過 const 引用在容器中查找
const Monster* findByElement(const vector<Monster>& monsters,
                              const string& element)
{
    for (const auto& m : monsters) {
        if (m.getElement() == element) {
            return &m;
        }
    }
    return nullptr;
}

/* ============================================================================
 *  第八節：常見錯誤與陷阱
 * ============================================================================
 *
 *  陷阱 1：在 const 函數中意外修改成員
 *    → 編譯器會直接報錯，這是最容易發現的錯誤
 *
 *  陷阱 2：通過指標間接修改（淺層 const）
 *    → const 只保護對象的直接成員，不保護指標指向的間接數據
 *    → 例如：data_ 指標本身不能改，但 *data_ 指向的內容居然可以改！
 *    → 這是 C++ const 的重要限制：它是「淺層的」（shallow const）
 *
 *    圖解：
 *      const 保護的範圍：
 *      +---------------------------------+
 *      | 對象本身                         |
 *      |  +----------+                   |
 *      |  | data_ ---+--> [堆上的 int]   |  <-- const 不保護這裡！
 *      |  | (指標)   |    (指向的內容)    |
 *      |  +----------+                   |
 *      |   ^ const 保護這裡              |
 *      +---------------------------------+
 *
 *  陷阱 3：忘記給 getter 加 const
 *    → 導致用 const 引用接收對象時，連 getter 都無法調用
 *    → 這是最常見的 C++ 新手錯誤
 *
 * ============================================================================
 */

// 示範陷阱 2：淺層 const — 指標間接修改
class ShallowConstDemo {
private:
    int* data_;      // 指向堆上的數據

public:
    ShallowConstDemo(int val) : data_(new int(val)) {}
    ~ShallowConstDemo() { delete data_; }

    // 禁止拷貝（簡化示範）
    ShallowConstDemo(const ShallowConstDemo&) = delete;
    ShallowConstDemo& operator=(const ShallowConstDemo&) = delete;

    // 危險！const 只保護指標本身（data_），不保護指向的內容（*data_）
    void sneakyModify() const {
        // data_ = nullptr;  // 編譯錯誤：不能修改指標本身
        *data_ = 999;        // 居然可以！const 不保護指標指向的數據
    }

    int getValue() const { return *data_; }
};

/* ============================================================================
 *  第九節：const 正確性檢查清單
 * ============================================================================
 *
 *  寫完一個成員函數後，問自己：
 *
 *    1. 這個函數會修改任何成員變數嗎？
 *       → 否 → 加 const
 *       → 是 → 不加 const
 *
 *    2. 這個函數調用的其他成員函數都是 const 嗎？
 *       → 如果你的函數是 const，它只能調用 const 函數
 *
 *    3. 返回值類型正確嗎？
 *       → const 函數返回成員的引用 → 必須是 const 引用
 *       → 返回值（拷貝）→ 不需要 const
 *
 *    4. 有沒有通過指標間接修改數據？
 *       → const 不保護指標指向的內容，需要自律
 *
 *  養成習慣：
 *    寫完一個成員函數，如果它不修改任何成員，立刻加上 const
 *
 * ============================================================================
 */

/* ============================================================================
 *  主程式：依序展示每個概念
 * ============================================================================
 */

int main() {

    // ========================================================================
    //  第一節示範：const 成員函數基礎
    // ========================================================================
    cout << "============================================================" << endl;
    cout << "  第一節：const 成員函數基礎" << endl;
    cout << "============================================================" << endl;

    Potion potion("治療藥水", 50, 3);

    // const 函數：只讀操作 — 不修改對象
    cout << "\n--- const 函數（只讀）---" << endl;
    potion.printInfo();                                // const 函數
    cout << "  名稱：" << potion.getName() << endl;    // const 函數
    cout << "  數量：" << potion.getQuantity() << endl; // const 函數

    // 非 const 函數：修改操作 — 會改變 quantity_
    cout << "\n--- 非 const 函數（修改）---" << endl;
    potion.use();       // 非 const 函數，quantity_ 從 3 變 2
    potion.use();       // 非 const 函數，quantity_ 從 2 變 1
    potion.printInfo(); // 確認數量已變為 1

    // ========================================================================
    //  第二節示範：const 對象的限制
    // ========================================================================
    cout << "\n============================================================" << endl;
    cout << "  第二節：const 對象的限制" << endl;
    cout << "============================================================" << endl;

    // 非 const 對象：所有函數都能調用
    cout << "\n--- 非 const 對象 ---" << endl;
    Shield shield("鐵盾", 40, 100);
    shield.printShieldInfo();       // const 函數 — 可以調用
    shield.takeDamage(20);          // 非 const 函數 — 可以調用
    shield.repair();                // 非 const 函數 — 可以調用

    // const 對象：只能調用 const 函數
    cout << "\n--- const 對象 ---" << endl;
    const Shield legendaryShield("傳說之盾", 100, 999);
    legendaryShield.printShieldInfo();      // const 函數 — 可以調用
    legendaryShield.getDefense();           // const 函數 — 可以調用
    // legendaryShield.takeDamage(10);      // 編譯錯誤！const 對象不能調用非 const 函數
    // legendaryShield.repair();            // 編譯錯誤！const 對象不能調用非 const 函數

    // const 引用參數：函數內部只能「看」不能「碰」
    inspectShield(shield);

    // ========================================================================
    //  第三節示範：const 正確性（好設計 vs 壞設計）
    // ========================================================================
    cout << "\n============================================================" << endl;
    cout << "  第三節：const 正確性（好設計 vs 壞設計）" << endl;
    cout << "============================================================" << endl;

    BadDesign bad("壞設計", 42);
    GoodDesign good("好設計", 42);

    cout << "\n--- 用 const 引用傳遞 ---" << endl;
    processBad(bad);      // 幾乎什麼都做不了，因為缺少 const
    processGood(good);    // 正常工作，因為所有 getter 都有 const

    // ========================================================================
    //  第四節示範：this 指標與 const
    // ========================================================================
    cout << "\n============================================================" << endl;
    cout << "  第四節：this 指標與 const（底層原理）" << endl;
    cout << "============================================================" << endl;

    Demo d(42);
    d.inspect();    // const 函數：this 類型是 const Demo* const，只能讀
    d.modify();     // 非 const 函數：this 類型是 Demo* const，可以讀寫
    d.inspect();    // 再次查看：值已從 42 變為 999

    // ========================================================================
    //  第五節示範：const 重載
    // ========================================================================
    cout << "\n============================================================" << endl;
    cout << "  第五節：const 重載" << endl;
    cout << "============================================================" << endl;

    // 非 const 對象 → 調用非 const 版本的 getText()
    cout << "\n--- 非 const 對象 ---" << endl;
    TextBuffer buf("Hello");
    buf.getText();                   // 調用非 const 版本
    buf.getText() = "Modified!";     // 非 const 版本返回非 const 引用，可以修改
    buf.print();                     // 確認內容已變為 "Modified!"

    // const 對象 → 調用 const 版本的 getText()
    cout << "\n--- const 對象 ---" << endl;
    const TextBuffer constBuf("ReadOnly");
    constBuf.getText();              // 調用 const 版本
    // constBuf.getText() = "Hack!"; // 編譯錯誤！const 版本返回 const 引用，不能修改
    constBuf.print();

    // const 引用 → 即使原始對象非 const，通過 const 引用也只能調用 const 版本
    cout << "\n--- const 引用 ---" << endl;
    const TextBuffer& ref = buf;
    ref.getText();                   // 調用 const 版本（因為 ref 是 const 引用）
    ref.print();

    // ========================================================================
    //  第六節示範：const 函數調用鏈
    // ========================================================================
    cout << "\n============================================================" << endl;
    cout << "  第六節：const 函數調用鏈" << endl;
    cout << "============================================================" << endl;

    GameCharacter hero("戰士", 200, 5);

    // printFullStatus 是 const 函數
    // 它內部調用 getStatusText(), getHp(), getMaxHp(), getHpPercent() — 都是 const
    // const 函數可以調用其他 const 函數
    cout << "\n--- 初始狀態 ---" << endl;
    hero.printFullStatus();

    // takeDamage 是非 const 函數
    // 它內部調用 getName(), getStatusText() — 都是 const
    // 非 const 函數可以調用 const 函數
    cout << "\n--- 受傷後 ---" << endl;
    hero.takeDamage(120);
    hero.printFullStatus();

    hero.takeDamage(60);
    hero.printFullStatus();

    // ========================================================================
    //  第七節示範：const 引用參數的實際應用
    // ========================================================================
    cout << "\n============================================================" << endl;
    cout << "  第七節：const 引用參數的實際應用" << endl;
    cout << "============================================================" << endl;

    vector<Monster> monsters = {
        Monster("火龍", 500, 60, "火"),
        Monster("冰狼", 300, 40, "冰"),
        Monster("雷鷹", 200, 55, "雷"),
        Monster("土蟲", 800, 20, "土")
    };

    // 場景 1：列表顯示 — 通過 const vector<Monster>& 傳遞
    cout << "\n--- 怪物列表 ---" << endl;
    showMonsterList(monsters);

    // 場景 2：分析函數 — 通過 const Monster& 傳遞
    cout << "\n--- 分析怪物 ---" << endl;
    analyzeMonster(monsters[0]);
    analyzeMonster(monsters[3]);

    // 場景 3：比較函數 — 兩個 const Monster& 傳遞
    cout << "\n--- 比較怪物 ---" << endl;
    compareMonsters(monsters[0], monsters[2]);
    compareMonsters(monsters[1], monsters[3]);

    // 場景 4：搜尋函數 — 返回 const Monster* 指標
    cout << "\n--- 搜尋怪物 ---" << endl;
    const Monster* found = findByElement(monsters, "雷");
    if (found) {
        cout << "  找到：";
        found->printInfo();     // printInfo 必須是 const 才能通過 const 指標調用
    }

    // ========================================================================
    //  第八節示範：淺層 const 陷阱
    // ========================================================================
    cout << "\n============================================================" << endl;
    cout << "  第八節：淺層 const 陷阱" << endl;
    cout << "============================================================" << endl;

    ShallowConstDemo scd(42);
    cout << "  修改前：" << scd.getValue() << endl;
    scd.sneakyModify();     // 在 const 函數中通過指標間接修改了數據！
    cout << "  修改後：" << scd.getValue() << endl;
    cout << "  （注意：const 函數居然成功修改了數據，因為 const 是淺層的）" << endl;

    // ========================================================================
    //  本課重點回顧
    // ========================================================================
    cout << "\n============================================================" << endl;
    cout << "  本課重點回顧" << endl;
    cout << "============================================================" << endl;
    cout << endl;
    cout << "  1. const 成員函數：承諾不修改任何成員變數，編譯器強制執行" << endl;
    cout << "  2. 語法位置：const 放在參數列表 ) 之後、{ 之前" << endl;
    cout << "  3. const 對象的限制：只能調用 const 成員函數" << endl;
    cout << "  4. const 正確性：所有不修改對象的函數都應標記 const" << endl;
    cout << "  5. this 指標：const 函數中 this 是 const T* const" << endl;
    cout << "  6. const 重載：同一函數的 const 和非 const 版本可共存" << endl;
    cout << "  7. 調用鏈：const 函數只能調用 const 函數" << endl;
    cout << "  8. 實際價值：讓類別可以安全地通過 const 引用傳遞" << endl;
    cout << "  9. 淺層 const：只保護直接成員，不保護指標指向的內容" << endl;
    cout << "  10. 養成習慣：不修改成員的函數，立刻加 const" << endl;
    cout << endl;

    return 0;
}
