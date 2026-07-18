/*
 * ============================================================================
 *  第 20～26 課：封裝專題 —— 合併精華總結
 * ============================================================================
 *
 *  本檔案合併了第 20～26 課的所有核心概念與精華範例，
 *  範例簡單扼要，但註釋完整覆蓋所有重點。
 *  全部放在一個 main() 下，可直接編譯執行。
 *  編譯：g++ -std=c++17 summary20-26.cpp -o summary20-26
 *
 *  目錄：
 *    第 20 課：封裝（Encapsulation）的意義
 *    第 21 課：getter 與 setter 設計模式
 *    第 22 課：const 成員函數
 *    第 23 課：mutable 關鍵字
 *    第 24 課：類別內的靜態成員變數
 *    第 25 課：類別內的靜態成員函數
 *    第 26 課：this 指標
 *
 * ============================================================================
 */

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>   // std::min, std::max
#include <cmath>       // sqrt, abs
using namespace std;

// ============================================================================
//  第 20 課：封裝（Encapsulation）的意義
// ============================================================================
/*
 *  封裝 = OOP 四大核心支柱之一（封裝/繼承/多型/抽象）
 *
 *  核心思想：將數據和操作數據的函數綁定在一起，控制外部對數據的訪問
 *
 *  封裝的三層防護：
 *    第一層：訪問控制 → private/protected/public，編譯器強制執行
 *    第二層：數據驗證 → setter/操作函數中檢查數據合法性
 *    第三層：不變量維護 → 確保對象始終處於合法狀態
 *
 *  不變量（Invariant）：
 *    - 對象在任何時刻都必須滿足的條件
 *    - 例如：0 <= hp <= maxHp, maxHp > 0, attack > 0
 *    - 每個 public 函數：開始前成立 → 執行中可暫時違反 → 結束後必須恢復
 *
 *  封裝的四大好處：
 *    1. 數據保護：防止外部破壞對象合法狀態
 *    2. 介面穩定：內部實現可改，public 介面不變
 *    3. 降低耦合：使用者不依賴內部細節
 *    4. 簡化使用：只需知道「做什麼」不需知道「怎麼做」
 *
 *  封裝的四個層次：
 *    1. 語法層：private/public 訪問控制
 *    2. 邏輯層：setter 中的數據驗證
 *    3. 介面層：只暴露必要操作，隱藏內部實現
 *    4. 模組層：命名空間/頭文件組織
 *
 *  設計原則：暴露最少介面
 *    - 只公開必要的操作
 *    - 不提供 setHp/setLevel 等 setter（避免繞過驗證）
 *    - private 輔助函數隱藏實現細節
 */

// 反面教材：無封裝的 struct，外部可任意篡改
struct CharacterBad {
    string name;
    int hp, maxHp, attack;
    // 六大災難：hp 超過 maxHp、hp 負數、maxHp 為 0、
    //           攻擊力隨改、等級經驗不一致、金幣無限刷
};

// 正面教材：完整封裝的 Character
class Character20 {
private:
    string name_;
    int hp_, maxHp_, attack_, level_, exp_, gold_;

    // 私有輔助函數：外部不可見（介面層封裝）
    int expToNextLevel() const { return level_ * 100; }
    void checkLevelUp() {
        while (exp_ >= expToNextLevel()) {
            exp_ -= expToNextLevel();
            level_++;
            maxHp_ += 20; hp_ = maxHp_; attack_ += 5;
            cout << "  ★ " << name_ << " 升級 Lv." << level_ << endl;
        }
    }

public:
    // 建構函數：確保初始狀態合法（防禦性檢查）
    Character20(const string& name, int maxHp, int atk)
        : name_(name)
        , hp_(maxHp > 0 ? maxHp : 100)
        , maxHp_(maxHp > 0 ? maxHp : 100)
        , attack_(atk > 0 ? atk : 10)
        , level_(1), exp_(0), gold_(0) {}

    // 受控操作：數據驗證 + 不變量維護
    void takeDamage(int dmg) {
        if (dmg <= 0) { cout << "  無效傷害" << endl; return; }
        hp_ = max(0, hp_ - dmg);  // 不變量：hp >= 0
        cout << "  " << name_ << " -" << dmg << " HP:" << hp_ << "/" << maxHp_ << endl;
        if (hp_ == 0) cout << "  " << name_ << " 倒下了！" << endl;
    }
    void heal(int amount) {
        if (amount <= 0 || hp_ == 0) return;
        hp_ = min(hp_ + amount, maxHp_);  // 不變量：hp <= maxHp
    }
    void gainExp(int amount) {
        if (amount <= 0) return;
        exp_ += amount;
        checkLevelUp();  // 自動處理升級邏輯（簡化使用）
    }

    // 只讀查詢（const）
    bool isAlive() const { return hp_ > 0; }
    const string& getName() const { return name_; }
    void printStatus() const {
        cout << "  " << name_ << " Lv." << level_ << " HP:" << hp_
             << "/" << maxHp_ << " ATK:" << attack_ << endl;
    }
};

void demo_lesson20() {
    cout << "\n╔══════════════════════════════════════════╗" << endl;
    cout << "║  第 20 課：封裝（Encapsulation）         ║" << endl;
    cout << "╚══════════════════════════════════════════╝" << endl;

    Character20 hero("勇者", 100, 25);
    hero.printStatus();

    hero.takeDamage(30);
    hero.takeDamage(-100);  // 無效傷害，被攔截
    hero.heal(20);
    hero.gainExp(150);      // 自動升級
    hero.printStatus();

    hero.takeDamage(9999);  // HP 歸零但不會變負
    hero.heal(50);          // 已死亡無法治療
}

// ============================================================================
//  第 21 課：getter 與 setter 設計模式
// ============================================================================
/*
 *  getter / setter 不是機械地為每個成員都加上，而是經過設計考量的介面！
 *
 *  getter 返回方式選擇：
 *    ┌────────────────┬──────────────────────────────────┐
 *    │ 型別           │ 建議返回方式                      │
 *    ├────────────────┼──────────────────────────────────┤
 *    │ 基本型別       │ 返回值（int getHp() const）        │
 *    │ (int/double)   │ 拷貝成本極低                      │
 *    ├────────────────┼──────────────────────────────────┤
 *    │ 大型物件       │ 返回 const 引用                    │
 *    │ (string/vector)│ const string& getName() const     │
 *    └────────────────┴──────────────────────────────────┘
 *
 *  危險的 getter（反面教材）：
 *    int& getBalanceDangerous() { return balance_; }
 *    → 返回非 const 引用，外部可直接修改內部狀態，繞過所有驗證！
 *    → 教訓：返回非 const 引用 = 把鑰匙交給外人，封裝形同虛設
 *
 *  用「行為」取代 setter（核心原則）：
 *    不提供 setHp()，而是提供 takeDamage() / heal()
 *    好處：語義清晰、自動驗證、自動觸發副作用
 *
 *  判斷原則：
 *    是否需要 getter？→ 外部有讀取的合理需求嗎？
 *    是否需要 setter？→ 能用更有意義的「行為」取代嗎？
 *    三種層級：
 *      1. 有 getter 沒 setter（外部可讀不可改）
 *      2. 沒 getter 沒 setter（純內部邏輯）
 *      3. 行為取代 setter（takeDamage/enrage）
 *
 *  命名慣例：
 *    風格 1（Java）：getHp() / setHp(int)  ← 最清晰
 *    風格 2（C++）：hp() / hp(int)          ← 重載風格
 *    風格 3（STL）：size() / length()       ← 通常沒 setter
 *    原則：項目中保持一致
 *
 *  鏈式調用（Method Chaining）：
 *    setter 返回 ClassName&，並在最後 return *this;
 *    例如：dlg.setTitle("警告").setMessage("確定？").show();
 */

// 展示危險 getter 的後果
class BankAccount21 {
private:
    int balance_;
public:
    BankAccount21(int b) : balance_(b) {}

    // 安全 getter
    int getBalance() const { return balance_; }

    // 危險 getter（反面教材）——返回非 const 引用，破壞封裝！
    int& getBalanceDangerous() { return balance_; }

    // 正確的修改介面（帶驗證）
    bool deposit(int amount) {
        if (amount <= 0) return false;
        balance_ += amount;
        return true;
    }
};

// 展示「行為取代 setter」
class Enemy21 {
private:
    string name_;
    int hp_, maxHp_, attackPower_;
    int aiState_;  // 純內部邏輯，沒 getter 沒 setter

public:
    Enemy21(const string& name, int maxHp, int atk)
        : name_(name), hp_(maxHp), maxHp_(maxHp), attackPower_(atk), aiState_(0) {}

    // 有 getter，沒 setter
    const string& getName() const { return name_; }
    int getHp() const { return hp_; }

    // 行為取代 setter：不是 setHp()，而是 takeDamage()
    void takeDamage(int dmg) {
        if (dmg <= 0) return;
        hp_ = max(0, hp_ - dmg);
        cout << "  " << name_ << " -" << dmg << " HP:" << hp_ << endl;
        if (hp_ == 0) cout << "  " << name_ << " 被擊敗！" << endl;
    }
    // 行為取代 setter：不是 setAttackPower()，而是 enrage()
    void enrage() {
        attackPower_ *= 2;
        cout << "  " << name_ << " 暴怒！ATK=" << attackPower_ << endl;
    }
};

// 展示鏈式調用
class DialogBox21 {
private:
    string title_, message_;
    int width_, height_;
public:
    DialogBox21() : title_(""), message_(""), width_(200), height_(100) {}

    // 返回自身引用 → 支持鏈式調用
    DialogBox21& setTitle(const string& t) { title_ = t; return *this; }
    DialogBox21& setMessage(const string& m) { message_ = m; return *this; }
    DialogBox21& setSize(int w, int h) {
        width_ = w > 0 ? w : 200;
        height_ = h > 0 ? h : 100;
        return *this;
    }
    void print() const {
        cout << "  [" << title_ << "] " << message_
             << " (" << width_ << "x" << height_ << ")" << endl;
    }
};

void demo_lesson21() {
    cout << "\n╔══════════════════════════════════════════╗" << endl;
    cout << "║  第 21 課：getter 與 setter 設計模式     ║" << endl;
    cout << "╚══════════════════════════════════════════╝" << endl;

    // 危險 getter 示範
    cout << "\n--- 危險 getter ---" << endl;
    BankAccount21 acc(1000);
    acc.deposit(500);
    cout << "  正常餘額：" << acc.getBalance() << endl;
    acc.getBalanceDangerous() = 999999;  // 繞過驗證直接竄改！
    cout << "  竄改後：" << acc.getBalance() << endl;
    cout << "  教訓：返回非 const 引用破壞封裝！" << endl;

    // 行為取代 setter
    cout << "\n--- 行為取代 setter ---" << endl;
    Enemy21 goblin("哥布林", 50, 15);
    goblin.takeDamage(20);  // 不是 setHp(30)
    goblin.enrage();         // 不是 setAttackPower(30)

    // 鏈式調用
    cout << "\n--- 鏈式調用 ---" << endl;
    DialogBox21 dlg;
    dlg.setTitle("警告").setMessage("確定刪除嗎？").setSize(400, 200);
    dlg.print();
}

// ============================================================================
//  第 22 課：const 成員函數
// ============================================================================
/*
 *  const 成員函數：
 *    語法：返回類型 函數名(參數) const { ... }
 *    含義：承諾不修改任何成員變數，編譯器強制執行
 *
 *  核心規則：
 *    非 const 對象 → 可調用 const 和非 const 函數
 *    const 對象    → 只能調用 const 函數
 *
 *  const 正確性（非常重要的設計原則）：
 *    所有不修改對象狀態的成員函數都應標記 const
 *    忘記加 const → const 引用接收時連 getter 都不能調用！
 *
 *  this 指標與 const：
 *    普通函數：this 類型為 T* const（可讀寫成員）
 *    const 函數：this 類型為 const T* const（只能讀取）
 *
 *  const 重載（const overloading）：
 *    同一函數可有 const 和非 const 兩個版本
 *    const 版本返回 const 引用（只讀）
 *    非 const 版本返回非 const 引用（可讀寫）
 *    編譯器根據對象是否 const 自動選擇版本
 *
 *  const 函數調用鏈：
 *    const → const       可以
 *    const → 非 const    不行（編譯錯誤）
 *    非 const → const    可以
 *    非 const → 非 const 可以
 *    簡記：const 函數只能調用 const 函數
 *
 *  淺層 const 陷阱（重要限制）：
 *    const 只保護對象的直接成員，不保護指標指向的間接數據
 *    data_ 指標本身不能改，但 *data_ 指向的內容可以改！
 *    const 是「淺層的」（shallow const）
 *
 *  const 正確性檢查清單：
 *    1. 函數會修改成員嗎？否 → 加 const
 *    2. 調用的其他函數都是 const 嗎？
 *    3. 返回成員引用？const 函數必須返回 const 引用
 *    4. 有沒有通過指標間接修改？（const 不保護）
 */

class Potion22 {
private:
    string name_;
    int healAmount_;
    int quantity_;

public:
    Potion22(const string& name, int heal, int qty)
        : name_(name), healAmount_(heal), quantity_(qty) {}

    // const 成員函數：只讀，不修改對象
    const string& getName() const { return name_; }
    int getQuantity() const { return quantity_; }
    void printInfo() const {
        cout << "  " << name_ << " 回復:" << healAmount_
             << " 數量:" << quantity_ << endl;
    }

    // 非 const 成員函數：會修改 quantity_
    bool use() {
        if (quantity_ <= 0) return false;
        quantity_--;  // 修改了成員 → 不能是 const
        cout << "  使用 " << name_ << " 剩餘:" << quantity_ << endl;
        return true;
    }
};

// const 重載示範
class TextBuffer22 {
private:
    string content_;
public:
    TextBuffer22(const string& text) : content_(text) {}

    // const 版本：const 對象調用，返回 const 引用
    const string& getText() const {
        cout << "  [const 版本]";
        return content_;
    }
    // 非 const 版本：非 const 對象調用，返回可修改引用
    string& getText() {
        cout << "  [非 const 版本]";
        return content_;
    }
    void print() const { cout << "  內容：" << content_ << endl; }
};

// 淺層 const 陷阱
class ShallowConst22 {
private:
    int* data_;
public:
    ShallowConst22(int v) : data_(new int(v)) {}
    ~ShallowConst22() { delete data_; }
    ShallowConst22(const ShallowConst22&) = delete;
    ShallowConst22& operator=(const ShallowConst22&) = delete;

    // 危險！const 函數卻能修改指標指向的數據
    void sneakyModify() const {
        // data_ = nullptr;  // 錯誤：不能改指標本身
        *data_ = 999;         // 可以！const 不保護指標指向的內容
    }
    int getValue() const { return *data_; }
};

void demo_lesson22() {
    cout << "\n╔══════════════════════════════════════════╗" << endl;
    cout << "║  第 22 課：const 成員函數                ║" << endl;
    cout << "╚══════════════════════════════════════════╝" << endl;

    // const 對象限制
    cout << "\n--- const 對象限制 ---" << endl;
    const Potion22 constPotion("聖水", 100, 5);
    constPotion.printInfo();       // const 函數 → OK
    constPotion.getQuantity();     // const 函數 → OK
    // constPotion.use();          // 編譯錯誤！const 對象不能調用非 const 函數

    Potion22 normalPotion("藥水", 50, 3);
    normalPotion.use();            // 非 const 對象 → 都可以調用
    normalPotion.printInfo();      // const 函數也能調用

    // const 重載
    cout << "\n--- const 重載 ---" << endl;
    TextBuffer22 buf("Hello");
    buf.getText() = "Modified";    // 非 const → 非 const 版本，可修改
    buf.print();

    const TextBuffer22 constBuf("ReadOnly");
    constBuf.getText();            // const → const 版本
    // constBuf.getText() = "Hack"; // 編譯錯誤！const 版本返回 const 引用
    cout << endl;

    // 淺層 const
    cout << "\n--- 淺層 const 陷阱 ---" << endl;
    ShallowConst22 sc(42);
    cout << "  修改前：" << sc.getValue() << endl;
    sc.sneakyModify();  // const 函數卻修改了指標指向的數據！
    cout << "  修改後：" << sc.getValue() << " (const 是淺層的！)" << endl;
}

// ============================================================================
//  第 23 課：mutable 關鍵字
// ============================================================================
/*
 *  問題起源：
 *    有些操作「邏輯上是只讀」，但「實作上需要修改某些內部數據」
 *    例如：快取、訪問計數器、延遲初始化
 *    沒有 mutable 就得：(a) 去掉 const  (b) 用 const_cast（危險）
 *
 *  語法：mutable 數據類型 變數名;
 *  含義：即使在 const 成員函數中，也允許修改這個變數
 *
 *  三大經典應用：
 *    1. 訪問計數器：getInfo() 記錄被查看次數
 *    2. 計算快取：getArea() 第一次計算後快取，之後直接返回
 *    3. 延遲初始化：建構時不算，需要時才算
 *
 *  判斷準則：
 *    ✅ 適合 mutable：快取、計數器、延遲初始化、互斥鎖
 *    ❌ 不適合 mutable：HP、攻擊力、等級等核心數據
 *    公式：去掉 mutable 變數，外部觀察行為是否完全相同？
 *      是 → 適合（只是少了快取/計數等內部機制）
 *      否 → 不適合（對象邏輯狀態真的不同了）
 *
 *  濫用的後果：
 *    把 HP/金幣設為 mutable → const 完全失去保護作用
 *    偽裝 const 的函數實際修改核心狀態 → 錯誤設計
 *
 *  mutable vs const_cast：
 *    mutable（推薦）：聲明時明確標記意圖，合法安全
 *    const_cast（不推薦）：隱藏修改意圖，對 const 對象是未定義行為
 *    結論：永遠用 mutable，不要用 const_cast
 */

// 綜合示範：訪問計數器 + 快取
class Circle23 {
private:
    double radius_;
    mutable int accessCount_;        // 訪問計數器（mutable）
    mutable bool areaCached_;        // 快取旗標（mutable）
    mutable double cachedArea_;      // 快取結果（mutable）
    static constexpr double PI = 3.14159265358979;

public:
    Circle23(double r) : radius_(r > 0 ? r : 1), accessCount_(0),
                         areaCached_(false), cachedArea_(0) {}

    // const 函數，但可以修改 mutable 成員
    double getArea() const {
        accessCount_++;  // mutable → const 中可修改
        if (!areaCached_) {
            cout << "  [計算面積...]" << endl;
            cachedArea_ = PI * radius_ * radius_;
            areaCached_ = true;
        } else {
            cout << "  [使用快取]" << endl;
        }
        return cachedArea_;
    }

    // setter 修改核心數據時，清除快取
    void setRadius(double r) {
        if (r <= 0) return;
        radius_ = r;
        areaCached_ = false;  // 快取失效
    }

    int getAccessCount() const { return accessCount_; }
};

// 反面教材：濫用 mutable
class BadMutable23 {
private:
    mutable int hp_;    // 錯誤！核心數據不該用 mutable
public:
    BadMutable23(int hp) : hp_(hp) {}
    // 偽裝成 const 卻修改核心狀態
    void takeDamage(int d) const { hp_ -= d; }  // 不應該是 const！
    int getHp() const { return hp_; }
};

void demo_lesson23() {
    cout << "\n╔══════════════════════════════════════════╗" << endl;
    cout << "║  第 23 課：mutable 關鍵字               ║" << endl;
    cout << "╚══════════════════════════════════════════╝" << endl;

    // 快取 + 計數器
    cout << "\n--- 快取 + 訪問計數器 ---" << endl;
    const Circle23 c(5.0);  // const 對象！
    cout << "  面積=" << c.getArea() << endl;  // 第一次計算
    cout << "  面積=" << c.getArea() << endl;  // 使用快取
    cout << "  面積=" << c.getArea() << endl;  // 使用快取
    cout << "  被訪問 " << c.getAccessCount() << " 次" << endl;

    // 濫用示範
    cout << "\n--- 濫用 mutable（反面教材）---" << endl;
    const BadMutable23 bad(100);
    bad.takeDamage(30);  // const 對象居然能受傷？！const 失效了
    cout << "  HP=" << bad.getHp() << " (const 完全失去保護！)" << endl;
}

// ============================================================================
//  第 24 課：類別內的靜態成員變數
// ============================================================================
/*
 *  核心觀念：
 *    普通成員變數：屬於「單一對象」，每個對象各有一份
 *    靜態成員變數：屬於「整個類別」，所有對象共享同一份
 *
 *  傳統語法（C++11 之前）：
 *    類別內聲明：static int totalCount_;
 *    類別外定義：int Soldier::totalCount_ = 0;  ← 真正分配記憶體
 *    缺少定義 → 連結錯誤（undefined reference）
 *
 *  C++17 inline static（推薦）：
 *    inline static int count_ = 0;  → 直接在類別內定義初始化，不需類別外定義
 *    任何型別都行（int, double, string...）
 *
 *  三種靜態常量比較：
 *    static const int X = 10;               → C++11，僅限整數型別
 *    inline static const string S = "hi";   → C++17，任何型別
 *    static constexpr double PI = 3.14;     → 編譯期常量（最高效）
 *
 *  兩種訪問方式：
 *    推薦：Class::member    → 清楚表明是類別共享數據
 *    不推薦：obj.member      → 容易誤導，看起來像對象自己的數據
 *
 *  生命週期：
 *    程式啟動 → 靜態成員初始化（main 之前）
 *    程式結束 → 靜態成員銷毀（main 之後）
 *    即使所有對象都銷毀了，靜態成員仍然存活
 *
 *  訪問控制：
 *    private 靜態成員 → 通過 public 的靜態 getter/setter 訪問
 *    靜態 setter 加驗證 → 防止非法修改
 *
 *  sizeof 特性：
 *    靜態成員不計入 sizeof(對象)
 *    存放在靜態/全域存儲區，不在對象內部
 *
 *  普通成員 vs 靜態成員：
 *    ┌────────────┬──────────────┬──────────────┐
 *    │ 特性       │ 普通成員      │ 靜態成員      │
 *    ├────────────┼──────────────┼──────────────┤
 *    │ 歸屬       │ 屬於對象      │ 屬於類別      │
 *    │ 份數       │ 每對象一份    │ 整個類別一份  │
 *    │ 計入sizeof │ 是           │ 否           │
 *    │ 生命週期   │ 與對象相同    │ 與程式相同    │
 *    │ 訪問方式   │ obj.member   │ Class::member│
 *    │ this 指標  │ 通過 this    │ 沒有 this    │
 *    └────────────┴──────────────┴──────────────┘
 */

class Soldier24 {
private:
    string name_;
    int id_;

    // C++17 inline static：直接定義初始化
    inline static int totalCount_ = 0;
    inline static int nextId_ = 1001;

public:
    Soldier24(const string& name)
        : name_(name), id_(nextId_++)
    {
        totalCount_++;
        cout << "  [入伍] " << name_ << " ID:" << id_
             << " 總人數:" << totalCount_ << endl;
    }
    ~Soldier24() {
        totalCount_--;
        cout << "  [退役] " << name_ << " 總人數:" << totalCount_ << endl;
    }

    // 公開靜態函數訪問私有靜態成員
    static int getTotalCount() { return totalCount_; }
};

// sizeof 對比 & constexpr static
class SizeDemo24 {
    int a_, b_;                           // 各 4 bytes
    inline static int shared_ = 0;        // 不計入 sizeof
    static constexpr double PI = 3.14;    // 編譯期常量，不計入 sizeof
};

void demo_lesson24() {
    cout << "\n╔══════════════════════════════════════════╗" << endl;
    cout << "║  第 24 課：類別內的靜態成員變數           ║" << endl;
    cout << "╚══════════════════════════════════════════╝" << endl;

    cout << "\n--- 對象計數器 + 自動 ID ---" << endl;
    {
        Soldier24 s1("阿強");
        Soldier24 s2("阿明");
        cout << "  目前總人數：" << Soldier24::getTotalCount() << endl;
    }
    // 離開作用域，全部解構
    cout << "  最終人數：" << Soldier24::getTotalCount() << endl;

    cout << "\n--- sizeof 不含靜態成員 ---" << endl;
    cout << "  SizeDemo24 大小：" << sizeof(SizeDemo24)
         << " bytes（只算兩個 int）" << endl;
}

// ============================================================================
//  第 25 課：類別內的靜態成員函數
// ============================================================================
/*
 *  語法：static 返回類型 函數名(參數);
 *
 *  五大特性：
 *    1. 不依賴對象，可用 Class::func() 調用
 *    2. 沒有 this 指標
 *    3. 只能訪問靜態成員（變數和函數）
 *    4. 不能訪問非靜態成員（沒有 this，不知道是哪個對象）
 *    5. 不能加 const（沒有 this，const 無意義）
 *
 *  訪問規則：
 *    ┌────────────────┬──────────┬───────────────┐
 *    │ 調用者         │ 靜態成員  │ 非靜態成員     │
 *    ├────────────────┼──────────┼───────────────┤
 *    │ 非靜態函數      │ 可以     │ 可以（有this） │
 *    │ 靜態函數        │ 可以     │ 不行（無this） │
 *    └────────────────┴──────────┴───────────────┘
 *    非靜態函數可以訪問一切；靜態函數只能訪問靜態的東西
 *
 *  透過參數間接訪問非靜態成員：
 *    靜態函數可以接收對象參數，訪問其 private 成員
 *    （因為靜態函數仍是類別的成員，有訪問 private 的權限）
 *
 *  三大經典應用：
 *    1. 工廠函數（Factory）：
 *       - 私有建構函數 + 靜態工廠方法
 *       - 函數名傳達語義：createSmall() 比 Potion(30,50) 清楚
 *    2. 工具函數類（Utility Class）：
 *       - 全部靜態函數，禁止實例化（delete 建構函數）
 *    3. 單例模式（Singleton）：
 *       - Meyers' Singleton：靜態局部變數，只初始化一次
 *       - static MyClass& getInstance() { static MyClass inst; return inst; }
 *       - C++11 後靜態局部變數初始化是線程安全的
 *
 *  靜態函數 vs 全域函數 vs 命名空間函數：
 *    全域函數：名字可能衝突，不能訪問 private → 盡量避免
 *    命名空間函數：有隔離，但不能訪問 private → 純工具用這個
 *    類別靜態函數：有歸屬，可訪問 private，可維護狀態 → 與類別相關用這個
 */

// 工廠函數示範
class Potion25 {
private:
    string name_;
    int heal_, price_;
    Potion25(const string& n, int h, int p) : name_(n), heal_(h), price_(p) {}

public:
    // 靜態工廠函數：提供命名的創建方式
    static Potion25 createSmall()  { return Potion25("小藥水", 30, 50); }
    static Potion25 createMedium() { return Potion25("中藥水", 70, 120); }
    static Potion25 createLarge()  { return Potion25("大藥水", 150, 300); }

    void printInfo() const {
        cout << "  " << name_ << " 回復:" << heal_ << " 價格:" << price_ << endl;
    }
};

// 工具函數類示範
class MathUtil25 {
public:
    static int clamp(int val, int lo, int hi) {
        return val < lo ? lo : val > hi ? hi : val;
    }
    static double distance(double x1, double y1, double x2, double y2) {
        return sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
    }
    MathUtil25() = delete;  // 禁止實例化
};

// 單例模式示範（Meyers' Singleton）
class GameManager25 {
private:
    int score_;
    GameManager25() : score_(0) { cout << "  [GameManager 初始化]" << endl; }
public:
    GameManager25(const GameManager25&) = delete;
    GameManager25& operator=(const GameManager25&) = delete;

    static GameManager25& getInstance() {
        static GameManager25 instance;  // 靜態局部變數，只初始化一次
        return instance;
    }
    void addScore(int pts) { score_ += pts; }
    int getScore() const { return score_; }
};

void demo_lesson25() {
    cout << "\n╔══════════════════════════════════════════╗" << endl;
    cout << "║  第 25 課：類別內的靜態成員函數           ║" << endl;
    cout << "╚══════════════════════════════════════════╝" << endl;

    // 工廠函數
    cout << "\n--- 工廠函數 ---" << endl;
    // Potion25 p("x", 1, 1);  // 編譯錯誤！private 建構函數
    Potion25 small = Potion25::createSmall();
    Potion25 large = Potion25::createLarge();
    small.printInfo();
    large.printInfo();

    // 工具函數類
    cout << "\n--- 工具函數類 ---" << endl;
    cout << "  clamp(150, 0, 100) = " << MathUtil25::clamp(150, 0, 100) << endl;
    cout << "  distance(0,0,3,4)  = " << MathUtil25::distance(0, 0, 3, 4) << endl;
    // MathUtil25 m;  // 編譯錯誤！建構函數 deleted

    // 單例模式
    cout << "\n--- 單例模式（Meyers' Singleton）---" << endl;
    GameManager25::getInstance().addScore(100);
    GameManager25::getInstance().addScore(250);
    cout << "  分數：" << GameManager25::getInstance().getScore() << endl;
    // 驗證唯一性
    GameManager25& r1 = GameManager25::getInstance();
    GameManager25& r2 = GameManager25::getInstance();
    cout << "  同一實例：" << (&r1 == &r2 ? "是" : "否") << endl;
}

// ============================================================================
//  第 26 課：this 指標
// ============================================================================
/*
 *  this 的本質：
 *    - 隱含的指標，指向調用成員函數的那個對象本身
 *    - hero.takeDamage(30) 實際上是 takeDamage(&hero, 30)
 *    - this 就是 &hero
 *
 *  this 的類型：
 *    普通函數 void func()       → T* const this（可讀寫成員）
 *    const 函數 void func() const → const T* const this（只能讀取）
 *    靜態函數 static void func() → 沒有 this
 *
 *  必須顯式使用 this 的六大場景：
 *    1. 參數與成員同名 → this->name = name; 消歧
 *       （建議：成員加下劃線後綴 name_ 從根本上避免同名）
 *    2. 鏈式調用 → return *this; 返回自身引用
 *    3. 傳遞 this 給外部函數 → registerPlayer(this); / logAction(*this);
 *    4. 自我賦值檢查 → if (this == &other) return *this;
 *    5. 拷貝自身 → MyClass copy = *this;
 *    6. 比較後返回自身 → return *this; 或 return other;
 *
 *  *this 解引用：
 *    this → 指標（地址）
 *    *this → 解引用，得到對象本身
 *    return *this → 返回對象的引用（配合返回類型 ClassName&）
 *
 *  常見誤區：
 *    1. 建構函數中洩漏 this → 對象尚未完全初始化，外部使用可能出錯
 *    2. 返回局部對象的指標 → 離開作用域指標懸空（dangling pointer）
 *       安全做法：return new Obj() 或 return Obj()（值返回，RVO 優化）
 *
 *  this 使用場景總結表：
 *    ┌──────────────────────┬──────────────────────┐
 *    │ 場景                 │ 用法                  │
 *    ├──────────────────────┼──────────────────────┤
 *    │ 同名消歧             │ this->name = name;    │
 *    │ 鏈式調用             │ return *this;          │
 *    │ 傳遞自身（指標）      │ func(this);           │
 *    │ 傳遞自身（引用）      │ func(*this);          │
 *    │ 自我賦值/操作檢查     │ if (this == &other)   │
 *    │ 拷貝自身             │ T copy = *this;       │
 *    │ 比較後返回自身        │ return *this;         │
 *    └──────────────────────┴──────────────────────┘
 */

// 鏈式調用（return *this）
class QueryBuilder26 {
private:
    string table_, conditions_, orderBy_;
    int limit_;
public:
    QueryBuilder26() : limit_(0) {}

    QueryBuilder26& from(const string& t) { table_ = t; return *this; }
    QueryBuilder26& where(const string& c) {
        if (!conditions_.empty()) conditions_ += " AND ";
        conditions_ += c;
        return *this;
    }
    QueryBuilder26& limit(int n) { limit_ = n > 0 ? n : 0; return *this; }

    string build() const {
        string sql = "SELECT * FROM " + table_;
        if (!conditions_.empty()) sql += " WHERE " + conditions_;
        if (limit_ > 0) sql += " LIMIT " + to_string(limit_);
        return sql;
    }
};

// 自我賦值檢查 + 同名消歧
class Buffer26 {
private:
    int* data_;
    int size_;
public:
    Buffer26(int size) : size_(size), data_(new int[size]) {
        for (int i = 0; i < size; i++) data_[i] = i;
    }
    ~Buffer26() { delete[] data_; }

    // 拷貝賦值：必須檢查自我賦值
    Buffer26& operator=(const Buffer26& other) {
        if (this == &other) {  // 核心：比較自身地址和另一個對象地址
            cout << "  [賦值] 自我賦值，跳過" << endl;
            return *this;
        }
        delete[] data_;
        size_ = other.size_;
        data_ = new int[size_];
        for (int i = 0; i < size_; i++) data_[i] = other.data_[i];
        cout << "  [賦值] 完成深拷貝" << endl;
        return *this;  // 支持 a = b = c 連續賦值
    }

    void print() const {
        cout << "  [";
        for (int i = 0; i < size_; i++) {
            if (i > 0) cout << ",";
            cout << data_[i];
        }
        cout << "]" << endl;
    }
};

// RPG 英雄：綜合 this 應用
class Hero26 {
private:
    string name_;
    int hp_, maxHp_, attack_;
    Hero26* leader_;

public:
    Hero26(const string& name, int maxHp, int atk)
        : name_(name), hp_(maxHp), maxHp_(maxHp), attack_(atk), leader_(nullptr) {}

    // 應用 1：leader_ = this（把自己設為隊長）
    Hero26& becomeLeader() {
        leader_ = this;
        cout << "  ★ " << name_ << " 成為隊長！" << endl;
        return *this;
    }

    // 應用 2：this == &other（防止跟隨自己）
    Hero26& follow(Hero26& other) {
        if (this == &other) {
            cout << "  " << name_ << "：不能跟隨自己" << endl;
            return *this;
        }
        leader_ = &other;
        cout << "  " << name_ << " 跟隨 " << other.name_ << endl;
        return *this;
    }

    // 應用 3：return *this（鏈式強化）
    Hero26& boostHp(int n) { maxHp_ += n; hp_ += n; return *this; }
    Hero26& boostAttack(int n) { attack_ += n; return *this; }

    // 應用 4：比較後返回 *this 或 other
    const Hero26& betterLeader(const Hero26& other) const {
        if (this == &other) return *this;
        return (maxHp_ + attack_ * 3 >= other.maxHp_ + other.attack_ * 3)
               ? *this : other;
    }

    // 應用 5：this == &target（自我治療判斷）
    void heal(Hero26& target, int amount) {
        if (this == &target) cout << "  " << name_ << " 自我治療";
        else cout << "  " << name_ << " 治療 " << target.name_;
        int actual = min(amount, target.maxHp_ - target.hp_);
        target.hp_ += actual;
        cout << " +" << actual << " HP:" << target.hp_ << "/" << target.maxHp_ << endl;
    }

    void takeDamage(int d) { hp_ = max(0, hp_ - d); }

    // 應用 6：leader_ == this（判斷自己是否隊長）
    void printStatus() const {
        cout << "  " << name_ << " HP:" << hp_ << "/" << maxHp_
             << " ATK:" << attack_;
        if (leader_ == this) cout << " (隊長)";
        else if (leader_) cout << " (跟隨:" << leader_->name_ << ")";
        cout << endl;
    }

    const string& getName() const { return name_; }
};

void demo_lesson26() {
    cout << "\n╔══════════════════════════════════════════╗" << endl;
    cout << "║  第 26 課：this 指標                     ║" << endl;
    cout << "╚══════════════════════════════════════════╝" << endl;

    // 鏈式調用
    cout << "\n--- 鏈式調用（return *this）---" << endl;
    string sql = QueryBuilder26()
        .from("players")
        .where("level > 10")
        .where("hp > 0")
        .limit(20)
        .build();
    cout << "  " << sql << endl;

    // 自我賦值檢查
    cout << "\n--- 自我賦值檢查 ---" << endl;
    Buffer26 buf(5);
    buf.print();
    buf = buf;  // this == &other → 安全跳過
    buf.print();

    // RPG 隊伍系統（綜合 this 應用）
    cout << "\n--- RPG 隊伍系統 ---" << endl;
    Hero26 warrior("亞瑟", 300, 40);
    Hero26 mage("梅林", 150, 70);
    Hero26 healer("伊蓮", 200, 20);

    // 鏈式強化
    warrior.boostHp(50).boostAttack(10);
    // 選隊長
    const Hero26& better = warrior.betterLeader(mage);
    cout << "  更適合當隊長：" << better.getName() << endl;
    // 組隊
    warrior.becomeLeader();
    mage.follow(warrior);
    healer.follow(warrior);
    warrior.follow(warrior);  // 攔截：不能跟隨自己
    // 狀態
    warrior.printStatus();
    mage.printStatus();
    healer.printStatus();
    // 治療
    warrior.takeDamage(120);
    healer.heal(warrior, 60);
    healer.heal(healer, 30);  // 自我治療
}

// ============================================================================
//  主程式
// ============================================================================
int main() {
    cout << "╔══════════════════════════════════════════════════╗" << endl;
    cout << "║  第 20～26 課：封裝專題 合併精華總結             ║" << endl;
    cout << "╚══════════════════════════════════════════════════╝" << endl;

    demo_lesson20();
    demo_lesson21();
    demo_lesson22();
    demo_lesson23();
    demo_lesson24();
    demo_lesson25();
    demo_lesson26();

    // ========================================================================
    //  速查表（Speed Lookup）
    // ========================================================================
    cout << "\n╔══════════════════════════════════════════════════════════════╗" << endl;
    cout << "║  第 20～26 課 速查表                                        ║" << endl;
    cout << "╠══════════════════════════════════════════════════════════════╣" << endl;
    cout << "║                                                            ║" << endl;
    cout << "║  【第 20 課：封裝】                                         ║" << endl;
    cout << "║    三層防護：訪問控制 → 數據驗證 → 不變量維護              ║" << endl;
    cout << "║    四大好處：數據保護 / 介面穩定 / 降低耦合 / 簡化使用     ║" << endl;
    cout << "║    四個層次：語法層 → 邏輯層 → 介面層 → 模組層            ║" << endl;
    cout << "║    核心目的：維護不變量，防止對象進入非法狀態              ║" << endl;
    cout << "║                                                            ║" << endl;
    cout << "║  【第 21 課：getter / setter】                              ║" << endl;
    cout << "║    基本型別 getter → 返回值    大型物件 → const 引用       ║" << endl;
    cout << "║    危險 getter → 返回非 const 引用 = 破壞封裝！            ║" << endl;
    cout << "║    核心原則：用行為取代 setter（takeDamage > setHp）        ║" << endl;
    cout << "║    鏈式調用：setter 返回 *this → 連續調用                  ║" << endl;
    cout << "║    命名風格：getX/setX（Java）或 x()/x(val)（C++重載）    ║" << endl;
    cout << "║                                                            ║" << endl;
    cout << "║  【第 22 課：const 成員函數】                               ║" << endl;
    cout << "║    語法：void func() const → 承諾不修改成員               ║" << endl;
    cout << "║    const 對象只能調用 const 函數                           ║" << endl;
    cout << "║    this 類型：普通 T* const / const 函數 const T* const    ║" << endl;
    cout << "║    const 重載：同名 const/非const 版本共存                 ║" << endl;
    cout << "║    調用鏈：const 函數只能調用 const 函數                   ║" << endl;
    cout << "║    淺層 const：不保護指標指向的內容                        ║" << endl;
    cout << "║                                                            ║" << endl;
    cout << "║  【第 23 課：mutable】                                      ║" << endl;
    cout << "║    語法：mutable int count_; → const 函數中也可修改       ║" << endl;
    cout << "║    適用：快取 / 計數器 / 延遲初始化 / 互斥鎖              ║" << endl;
    cout << "║    判斷：去掉 mutable 變數外部觀察是否一樣？是 → 適合     ║" << endl;
    cout << "║    濫用：核心數據用 mutable → const 完全失效               ║" << endl;
    cout << "║    mutable 合法安全 > const_cast 未定義行為                ║" << endl;
    cout << "║                                                            ║" << endl;
    cout << "║  【第 24 課：靜態成員變數】                                  ║" << endl;
    cout << "║    屬於類別，所有對象共享同一份                            ║" << endl;
    cout << "║    傳統：類內聲明 + 類外定義   C++17：inline static       ║" << endl;
    cout << "║    constexpr static → 編譯期常量，最高效                  ║" << endl;
    cout << "║    推薦 Class::member 訪問   不計入 sizeof                 ║" << endl;
    cout << "║    生命週期：程式啟動 → 程式結束（超過任何對象）           ║" << endl;
    cout << "║                                                            ║" << endl;
    cout << "║  【第 25 課：靜態成員函數】                                  ║" << endl;
    cout << "║    沒有 this 指標，只能訪問靜態成員                        ║" << endl;
    cout << "║    不能加 const（沒有 this → const 無意義）                ║" << endl;
    cout << "║    可接收對象參數間接訪問 private                          ║" << endl;
    cout << "║    工廠函數：私有建構 + 靜態創建方法                       ║" << endl;
    cout << "║    工具類：全靜態 + delete 建構函數                        ║" << endl;
    cout << "║    單例：static T& getInstance() { static T i; return i; }║" << endl;
    cout << "║    vs 全域函數：靜態函數有歸屬可訪問 private              ║" << endl;
    cout << "║                                                            ║" << endl;
    cout << "║  【第 26 課：this 指標】                                    ║" << endl;
    cout << "║    本質：隱含指標，指向調用函數的對象（== &obj）           ║" << endl;
    cout << "║    類型：普通 T* const / const → const T* const / 靜態無  ║" << endl;
    cout << "║    同名消歧：this->name = name;                           ║" << endl;
    cout << "║    鏈式調用：return *this;                                 ║" << endl;
    cout << "║    傳遞自身：func(this) 或 func(*this)                    ║" << endl;
    cout << "║    自我檢查：if (this == &other)                          ║" << endl;
    cout << "║    拷貝自身：T copy = *this;                              ║" << endl;
    cout << "║    建構中洩漏 this 有風險（對象尚未完全初始化）            ║" << endl;
    cout << "║                                                            ║" << endl;
    cout << "╚══════════════════════════════════════════════════════════════╝" << endl;

    return 0;
}
