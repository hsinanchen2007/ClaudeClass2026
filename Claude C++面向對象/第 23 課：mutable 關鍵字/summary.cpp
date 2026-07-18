/*
 * ============================================================================
 *   第 23 課：mutable 關鍵字 —— 完整總結
 * ============================================================================
 *
 *   本檔案涵蓋第 23 課所有 .cpp 檔案的核心觀念與範例程式碼，
 *   閱讀此檔案即可完整複習，無需再翻閱其他檔案。
 *
 * ============================================================================
 *   目錄
 * ============================================================================
 *   一、問題的起源：為什麼需要 mutable？
 *   二、mutable 的語法與基本用法（訪問計數器）
 *   三、經典應用一：計算快取（Cache）
 *   四、經典應用二：延遲初始化（Lazy Initialization）
 *   五、mutable 的判斷準則（適合 vs 不適合）
 *   六、反面教材：濫用 mutable 的後果
 *   七、mutable vs const_cast 的比較
 *   八、綜合範例：怪物圖鑑系統（結合快取 + 延遲初始化 + 計數器）
 *   九、本課重點回顧
 * ============================================================================
 */

#include <iostream>
#include <string>
using namespace std;

// ============================================================================
//   一、問題的起源：為什麼需要 mutable？
// ============================================================================
//
//   上一課我們學到：const 成員函數承諾「不修改任何成員變數」。
//   但實務中，有些操作「邏輯上是只讀」，「實作上卻需要修改某些內部數據」：
//
//   場景 1：快取（Cache）
//     → getResult() 第一次計算結果，之後直接返回快取值
//     → 邏輯上是「讀取」，但需要修改快取變數
//
//   場景 2：訪問計數器（Access Counter）
//     → getInfo() 每次被調用時記錄訪問次數
//     → 邏輯上是「讀取」，但需要修改計數器
//
//   場景 3：互斥鎖（Mutex）
//     → getData() 需要加鎖保護線程安全
//     → 邏輯上是「讀取」，但需要修改鎖的狀態
//
//   場景 4：延遲初始化（Lazy Initialization）
//     → getDescription() 第一次被調用時才生成描述文字
//     → 邏輯上是「讀取」，但需要修改內部狀態
//
//   如果沒有 mutable，你就被迫二選一：
//     (a) 去掉 const（破壞介面承諾）
//     (b) 用 const_cast 強制轉型（危險且醜陋，屬於未定義行為）
//
//   mutable 提供了乾淨的解決方案。
//
//   語法：
//     mutable 數據類型 變數名;
//
//   含義：
//     即使在 const 成員函數中，也允許修改這個變數。
//


// ============================================================================
//   二、mutable 的語法與基本用法 —— 訪問計數器
// ============================================================================
//
//   最基本的 mutable 用途：在 const 函數中遞增一個「查看次數」計數器。
//   計數器不影響對象的邏輯狀態（怪物的名字、HP、攻擊力都沒變），
//   所以將它標記為 mutable 是合理的。
//

class Monster {
private:
    string name_;
    int hp_;
    int attack_;

    // mutable 成員：const 函數也能修改它
    // inspectCount_ 用來記錄被查看了多少次，邏輯上不影響怪物狀態
    mutable int inspectCount_;

public:
    Monster(const string& name, int hp, int atk)
        : name_(name), hp_(hp), attack_(atk), inspectCount_(0)
    {
    }

    // const 函數 —— 邏輯上只讀，但可以修改 inspectCount_（因為它是 mutable）
    void printInfo() const {
        inspectCount_++;   // mutable 成員可以在 const 函數中被修改
        cout << "  " << name_ << " [HP:" << hp_ << " ATK:" << attack_
             << "] (被查看了 " << inspectCount_ << " 次)" << endl;
    }

    const string& getName() const { return name_; }
    int getHp() const { return hp_; }

    // 查看被檢查了幾次——這個函數也是 const 的，因為它不修改怪物的邏輯狀態
    int getInspectCount() const { return inspectCount_; }

    // 非 const 函數：實際修改怪物狀態，不允許在 const 對象上調用
    void takeDamage(int dmg) {
        hp_ = max(0, hp_ - dmg);
    }
};

// 示範函數：展示基本 mutable 用法
void demoBasicMutable() {
    cout << "=== 二、mutable 基本用法 —— 訪問計數器 ===" << endl;

    const Monster dragon("火龍", 500, 60);  // const 對象！

    // 可以調用 const 函數，inspectCount_ 會被修改（因為是 mutable）
    dragon.printInfo();   // 被查看了 1 次
    dragon.printInfo();   // 被查看了 2 次
    dragon.printInfo();   // 被查看了 3 次

    cout << "  總共被查看：" << dragon.getInspectCount() << " 次" << endl;

    // dragon.takeDamage(10);  // 編譯錯誤！const 對象不能調用非 const 函數

    cout << endl;
}


// ============================================================================
//   三、經典應用一：計算快取（Cache）
// ============================================================================
//
//   mutable 最常見的用途之一。
//   當某個計算很昂貴時，希望只算一次然後快取結果。
//
//   設計要點：
//   (1) 快取相關的成員變數（cachedArea_、areaCached_ 等）標記為 mutable
//   (2) getter 函數標記為 const（因為邏輯上不改變物件狀態）
//   (3) setter 修改核心數據時，清除快取（讓快取失效）
//

class Circle {
private:
    double radius_;   // 核心數據：半徑

    // 快取相關 —— 全部用 mutable，因為它們只是效能優化，不影響邏輯狀態
    // areaCached_ / circumCached_ ：標記快取是否有效
    // cachedArea_ / cachedCircum_ ：存儲計算結果
    mutable bool areaCached_;
    mutable double cachedArea_;
    mutable bool circumCached_;
    mutable double cachedCircum_;

    static constexpr double PI = 3.14159265358979;

public:
    Circle(double r)
        : radius_(r > 0 ? r : 1.0)
        , areaCached_(false), cachedArea_(0)
        , circumCached_(false), cachedCircum_(0)
    {
    }

    // const 函數：邏輯上只讀，但會更新快取（mutable 成員）
    // 第一次調用時觸發計算，之後直接返回快取值
    double getArea() const {
        if (!areaCached_) {
            cout << "    [計算面積...]" << endl;
            cachedArea_ = PI * radius_ * radius_;   // mutable 可修改
            areaCached_ = true;                      // mutable 可修改
        } else {
            cout << "    [使用快取]" << endl;
        }
        return cachedArea_;
    }

    double getCircumference() const {
        if (!circumCached_) {
            cout << "    [計算周長...]" << endl;
            cachedCircum_ = 2 * PI * radius_;
            circumCached_ = true;
        } else {
            cout << "    [使用快取]" << endl;
        }
        return cachedCircum_;
    }

    // setter：修改半徑時，快取失效，需要清除
    void setRadius(double r) {
        if (r <= 0) return;
        radius_ = r;
        areaCached_ = false;     // 清除快取
        circumCached_ = false;   // 清除快取
        cout << "  半徑改為 " << radius_ << "，快取已清除" << endl;
    }

    double getRadius() const { return radius_; }
};

// 示範函數：展示快取用法
void demoCaching() {
    cout << "=== 三、計算快取（Cache）===" << endl;

    Circle c(5.0);

    // 第一次調用——觸發計算
    cout << "\n--- 第一次查詢 ---" << endl;
    cout << "  面積 = " << c.getArea() << endl;
    cout << "  周長 = " << c.getCircumference() << endl;

    // 第二次調用——使用快取，不重複計算
    cout << "\n--- 第二次查詢（快取命中）---" << endl;
    cout << "  面積 = " << c.getArea() << endl;
    cout << "  周長 = " << c.getCircumference() << endl;

    // 修改半徑——快取失效
    cout << "\n--- 修改半徑 ---" << endl;
    c.setRadius(10.0);

    // 再次查詢——重新計算
    cout << "\n--- 修改後查詢 ---" << endl;
    cout << "  面積 = " << c.getArea() << endl;
    cout << "  周長 = " << c.getCircumference() << endl;

    // 再次查詢——又是快取
    cout << "\n--- 再次查詢（快取命中）---" << endl;
    cout << "  面積 = " << c.getArea() << endl;

    cout << endl;
}


// ============================================================================
//   四、經典應用二：延遲初始化（Lazy Initialization）
// ============================================================================
//
//   有些數據的初始化成本很高，希望在「真正需要時才初始化」。
//   建構時不生成詳細描述，等到第一次調用 getDescription() 時才觸發生成。
//   之後再次調用直接返回已生成的結果，不重複生成。
//
//   與快取的差異：
//   - 快取是「算過一次就記住」
//   - 延遲初始化是「建構時不算，等到需要時才算」
//   兩者都依賴 mutable 來實現。
//

class QuestLog {
private:
    string questName_;
    int difficulty_;

    // 延遲初始化的成員 —— mutable 因為邏輯上是只讀的，只是推遲了生成時機
    mutable bool descriptionReady_;        // 描述是否已經生成
    mutable string detailedDescription_;   // 存儲生成的描述

    // 私有 const 輔助函數：模擬昂貴的描述生成操作
    void generateDescription() const {
        cout << "    [生成詳細描述... 耗時操作]" << endl;
        detailedDescription_ = "【" + questName_ + "】\n";
        detailedDescription_ += "    難度：";
        for (int i = 0; i < difficulty_; i++) {
            detailedDescription_ += "★";
        }
        detailedDescription_ += "\n";
        detailedDescription_ += "    這是一個";
        if (difficulty_ >= 4) detailedDescription_ += "極其危險的";
        else if (difficulty_ >= 3) detailedDescription_ += "具有挑戰性的";
        else if (difficulty_ >= 2) detailedDescription_ += "需要謹慎的";
        else detailedDescription_ += "簡單的";
        detailedDescription_ += "任務。冒險者需做好充分準備。";

        descriptionReady_ = true;
    }

public:
    QuestLog(const string& name, int diff)
        : questName_(name)
        , difficulty_(diff > 0 && diff <= 5 ? diff : 1)
        , descriptionReady_(false)
    {
        cout << "  [登記任務] " << questName_ << endl;
        // 注意：不在建構時生成描述，延遲到需要時
    }

    // 簡單查詢——不需要詳細描述，不觸發昂貴操作
    const string& getName() const { return questName_; }
    int getDifficulty() const { return difficulty_; }

    // 需要詳細描述時才觸發初始化
    const string& getDescription() const {
        if (!descriptionReady_) {
            generateDescription();   // 延遲初始化：第一次調用才生成
        }
        return detailedDescription_;
    }
};

// 示範函數：展示延遲初始化
void demoLazyInit() {
    cout << "=== 四、延遲初始化（Lazy Initialization）===" << endl;

    // 創建多個任務——建構時都不生成描述
    cout << "\n--- 登記任務 ---" << endl;
    QuestLog quest1("討伐火龍", 5);
    QuestLog quest2("採集草藥", 1);
    QuestLog quest3("護送商隊", 3);

    // 只用簡單查詢——不觸發描述生成
    cout << "\n--- 簡單查詢（不觸發生成）---" << endl;
    cout << "  任務1：" << quest1.getName()
         << "（難度 " << quest1.getDifficulty() << "）" << endl;
    cout << "  任務2：" << quest2.getName()
         << "（難度 " << quest2.getDifficulty() << "）" << endl;

    // 只有查看詳細描述時才觸發生成
    cout << "\n--- 查看詳細描述（觸發生成）---" << endl;
    cout << quest1.getDescription() << endl;

    // 第二次查看——直接返回，不重複生成
    cout << "\n--- 再次查看（已生成，不重複）---" << endl;
    cout << quest1.getDescription() << endl;

    // quest2 和 quest3 的描述始終沒被生成——節省了資源
    cout << "\n--- quest2 和 quest3 從未生成描述，節省了資源 ---" << endl;

    cout << endl;
}


// ============================================================================
//   五、mutable 的判斷準則
// ============================================================================
//
//   mutable 不能隨便用，以下是判斷是否適合使用 mutable 的準則：
//
//   ✅ 適合用 mutable 的情況（修改的是「實現細節」，不是「邏輯狀態」）：
//     - 快取 / 備忘錄（Cache / Memoization）
//     - 訪問計數器（Access Counter）
//     - 延遲初始化（Lazy Initialization）
//     - 互斥鎖（Mutex）—— 多線程場景
//     - 調試 / 日誌用的輔助數據
//
//   ❌ 不適合用 mutable 的情況（修改的是「對象的邏輯狀態」）：
//     - HP、攻擊力、等級等核心數據
//     - 用戶名、帳戶餘額等業務數據
//     - 任何「外部可觀察到的變化」
//
//   核心原則：
//     從外部觀察者的角度，const 函數調用前後，對象「看起來」沒有變化。
//
//   判斷公式：
//     問：去掉這個 mutable 變數，從外部觀察者來看，
//         對象的行為是否完全相同？
//       → 是（只是少了快取/計數等內部機制）→ ✅ 適合 mutable
//       → 否（對象的邏輯狀態真的不同了）    → ❌ 不適合 mutable
//


// ============================================================================
//   六、反面教材：濫用 mutable 的後果
// ============================================================================
//
//   把核心數據（如 HP、金幣）設為 mutable，然後偽裝成 const 函數去修改它們。
//   這會讓 const 完全失去保護作用，是錯誤的設計。
//

// ----- 錯誤示範 -----
class BadExample {
private:
    string name_;
    mutable int hp_;        // 錯誤：把核心數據設為 mutable
    mutable int gold_;      // 錯誤：把核心數據設為 mutable

public:
    BadExample(const string& name, int hp, int gold)
        : name_(name), hp_(hp), gold_(gold) {}

    // 這些操作明明在修改對象的邏輯狀態，卻偽裝成 const！
    void takeDamage(int dmg) const {   // 錯誤：不應該是 const
        hp_ -= dmg;
        cout << "  " << name_ << " 受傷 HP:" << hp_ << endl;
    }

    void spendGold(int amount) const { // 錯誤：不應該是 const
        gold_ -= amount;
        cout << "  花費 " << amount << " 金幣" << endl;
    }

    void print() const {
        cout << "  " << name_ << " HP:" << hp_
             << " Gold:" << gold_ << endl;
    }
};

// ----- 正確示範 -----
class GoodExample {
private:
    string name_;
    int hp_;                     // 核心數據：不用 mutable
    int gold_;                   // 核心數據：不用 mutable
    mutable int viewCount_;      // 只有輔助數據用 mutable

public:
    GoodExample(const string& name, int hp, int gold)
        : name_(name), hp_(hp), gold_(gold), viewCount_(0) {}

    // 修改狀態的函數：不是 const（正確做法）
    void takeDamage(int dmg) {
        hp_ = max(0, hp_ - dmg);
        cout << "  " << name_ << " 受傷 HP:" << hp_ << endl;
    }

    void spendGold(int amount) {
        if (amount <= gold_) {
            gold_ -= amount;
            cout << "  花費 " << amount << " 金幣" << endl;
        }
    }

    // 只讀函數：是 const，只有 viewCount_ 用 mutable（合理用途）
    void print() const {
        viewCount_++;
        cout << "  " << name_ << " HP:" << hp_
             << " Gold:" << gold_
             << " (查看次數:" << viewCount_ << ")" << endl;
    }
};

// 示範函數：對比濫用與正確使用
void demoGoodVsBad() {
    cout << "=== 六、mutable 濫用 vs 正確使用 ===" << endl;

    // ----- 濫用的後果：const 失去意義 -----
    cout << "\n--- 濫用 mutable ---" << endl;
    const BadExample bad("壞設計", 100, 500);
    bad.takeDamage(30);     // const 對象居然能受傷？！const 完全失效
    bad.spendGold(200);     // const 對象居然能花錢？！
    bad.print();
    cout << "  const 完全失去了保護作用！" << endl;

    // ----- 正確的設計 -----
    cout << "\n--- 正確使用 ---" << endl;
    const GoodExample good("好設計", 100, 500);
    // good.takeDamage(30);   // 編譯錯誤！正確攔截——const 保護起作用了
    // good.spendGold(200);   // 編譯錯誤！正確攔截
    good.print();             // 只有查看次數變化（合理的 mutable）
    good.print();

    cout << endl;
}


// ============================================================================
//   七、mutable vs const_cast 的比較
// ============================================================================
//
//   你可能會想：不用 mutable，用 const_cast 強制去掉 const 不行嗎？
//
//   答案是：不行。兩者有本質差異：
//
//   mutable（推薦）：
//     - 在聲明時明確標記意圖
//     - 編譯器知道這個成員可以在 const 中修改
//     - 合法、安全、符合 C++ 標準
//     - 其他程式員看到 mutable 就知道設計意圖
//
//   const_cast（不推薦）：
//     - 隱藏了修改意圖
//     - 對 const 對象使用是「未定義行為」（Undefined Behavior）
//     - 編譯器可能把 const 對象放在只讀記憶體中，強制修改可能導致崩潰
//     - 代碼維護者不知道為什麼要這樣做
//
//   結論：需要在 const 函數中修改成員時，永遠使用 mutable，不要用 const_cast。
//

class Counter {
private:
    int count_;

public:
    Counter() : count_(0) {}

    // 方法 1（推薦）：用 mutable
    // 如果 count_ 宣告為 mutable int count_;
    // 則可以直接寫：
    //   void increment() const { count_++; }
    // 語義清晰，編譯器知道這是特例。

    // 方法 2（不推薦）：用 const_cast
    // 強行去掉 const 限定，語義混亂，容易出錯
    void incrementBad() const {
        // const_cast<Counter*>(this) 強制去掉 this 的 const 屬性
        // 技術上可以編譯，但對 const 對象這是未定義行為！
        const_cast<Counter*>(this)->count_++;
        cout << "  const_cast 方式：count = " << count_ << endl;
    }

    int getCount() const { return count_; }
};

// 示範函數：mutable vs const_cast
void demoMutableVsConstCast() {
    cout << "=== 七、mutable vs const_cast ===" << endl;

    const Counter c;     // const 對象
    c.incrementBad();    // 技術上能跑，但行為是未定義的！
    c.incrementBad();

    // 警告：對 const 對象使用 const_cast 修改數據
    // 在 C++ 標準中是「未定義行為」（Undefined Behavior）！
    // 編譯器可能把 const 對象放在只讀記憶體中，
    // 強制修改可能導致程式崩潰。

    cout << endl;
}


// ============================================================================
//   八、綜合範例：怪物圖鑑系統
// ============================================================================
//
//   這個範例結合了 mutable 的三大用途：
//   (1) 訪問計數器（viewCount_）—— 記錄每個條目被查看了幾次
//   (2) 延遲初始化（detailGenerated_ + detailCache_）—— 詳細資料只在需要時生成
//   (3) 計算快取 —— 生成後快取，不重複生成
//
//   所有查詢函數都是 const 的，可以安全地接收 const 引用，
//   但內部的 mutable 成員允許在 const 上下文中更新輔助狀態。
//

class MonsterEntry {
private:
    // ====== 邏輯狀態（不可變的圖鑑資料）======
    string name_;
    string element_;
    int baseHp_;
    int baseAttack_;
    int rarity_;              // 1~5 星

    // ====== 輔助狀態（mutable）======
    mutable int viewCount_;               // 被查看次數（訪問計數器）
    mutable bool detailGenerated_;        // 詳細資料是否已生成（延遲初始化旗標）
    mutable string detailCache_;          // 詳細資料快取（快取 + 延遲初始化）

    // 私有輔助函數（const）：生成詳細資料
    void generateDetail() const {
        cout << "    [生成詳細資料...]" << endl;

        detailCache_ = "=== " + name_ + " ===\n";
        detailCache_ += "  屬性：" + element_ + "\n";
        detailCache_ += "  稀有度：";
        for (int i = 0; i < rarity_; i++) detailCache_ += "★";
        detailCache_ += "\n";
        detailCache_ += "  基礎 HP：" + to_string(baseHp_) + "\n";
        detailCache_ += "  基礎 ATK：" + to_string(baseAttack_) + "\n";

        // 添加弱點資訊（根據屬性推算）
        detailCache_ += "  弱點：";
        if (element_ == "火") detailCache_ += "水";
        else if (element_ == "水") detailCache_ += "雷";
        else if (element_ == "雷") detailCache_ += "土";
        else if (element_ == "土") detailCache_ += "風";
        else if (element_ == "風") detailCache_ += "火";
        else detailCache_ += "無";
        detailCache_ += "\n";

        // 添加威脅評估
        int threat = baseHp_ / 100 + baseAttack_ / 10 + rarity_;
        detailCache_ += "  威脅指數：" + to_string(threat) + "\n";

        detailGenerated_ = true;   // 標記已生成，下次直接返回快取
    }

public:
    MonsterEntry(const string& name, const string& elem,
                 int hp, int atk, int rare)
        : name_(name), element_(elem)
        , baseHp_(hp), baseAttack_(atk)
        , rarity_(rare > 0 && rare <= 5 ? rare : 1)
        , viewCount_(0)
        , detailGenerated_(false)
    {
    }

    // ====== 所有查詢函數都是 const ======

    // 簡要資訊——只讀，但遞增查看次數（mutable）
    void printBrief() const {
        viewCount_++;
        cout << "  ";
        for (int i = 0; i < rarity_; i++) cout << "★";
        cout << " " << name_ << " [" << element_ << "]"
             << " (查看:" << viewCount_ << ")" << endl;
    }

    // 詳細資訊——延遲生成 + 快取（結合兩種 mutable 用法）
    const string& getDetail() const {
        viewCount_++;
        if (!detailGenerated_) {
            generateDetail();   // 延遲初始化：第一次才生成
        }
        return detailCache_;    // 之後直接返回快取
    }

    // 其他 getter
    const string& getName() const { return name_; }
    int getViewCount() const { return viewCount_; }
    int getRarity() const { return rarity_; }
};

// 展示圖鑑——接收 const 陣列，所有 mutable 操作在 const 上下文中正常工作
void showEncyclopedia(const MonsterEntry entries[], int count) {
    cout << "\n  +---------------------------+" << endl;
    cout << "  |     怪 物 圖 鑑           |" << endl;
    cout << "  +---------------------------+" << endl;

    for (int i = 0; i < count; i++) {
        entries[i].printBrief();   // const 函數，但內部 mutable 成員被更新
    }
}

// 查看特定怪物詳情——接收 const 引用
void showDetail(const MonsterEntry& entry) {
    cout << "\n" << entry.getDetail();   // 延遲生成 + 快取
}

// 示範函數：綜合範例
void demoComprehensive() {
    cout << "=== 八、綜合範例：怪物圖鑑系統 ===" << endl;

    // 創建圖鑑條目
    const int COUNT = 4;
    MonsterEntry entries[COUNT] = {
        MonsterEntry("炎龍王", "火", 800, 70, 5),
        MonsterEntry("冰霜狼", "水", 400, 45, 3),
        MonsterEntry("雷電鷹", "雷", 300, 60, 4),
        MonsterEntry("泥土蟲", "土", 600, 20, 1)
    };

    // 瀏覽圖鑑——只觸發簡要資訊，不生成詳細資料
    showEncyclopedia(entries, COUNT);

    // 查看炎龍王的詳細資料——第一次觸發生成
    cout << "\n--- 查看炎龍王詳情 ---";
    showDetail(entries[0]);

    // 再次查看——使用快取，不重複生成
    cout << "\n--- 再次查看炎龍王詳情（快取命中）---";
    showDetail(entries[0]);

    // 查看雷電鷹——觸發生成
    cout << "\n--- 查看雷電鷹詳情 ---";
    showDetail(entries[2]);

    // 查看各怪物被查看次數
    cout << "\n--- 查看統計 ---" << endl;
    for (int i = 0; i < COUNT; i++) {
        cout << "  " << entries[i].getName()
             << "：被查看 " << entries[i].getViewCount() << " 次" << endl;
    }

    // 冰霜狼和泥土蟲的詳細描述從未被生成——節省資源
    cout << "\n  冰霜狼和泥土蟲的詳細描述從未生成，節省了資源！" << endl;

    cout << endl;
}


// ============================================================================
//   九、本課重點回顧
// ============================================================================
//
//   | 概念            | 說明                                               |
//   |-----------------|----------------------------------------------------|
//   | mutable 語法    | mutable int count_;                                |
//   | mutable 含義    | 即使在 const 成員函數中也可以修改                   |
//   | 適用場景        | 快取、計數器、延遲初始化、互斥鎖                    |
//   | 核心原則        | 只用於「不影響邏輯狀態」的實現細節                   |
//   | 判斷方法        | 去掉 mutable 變數，外部觀察是否一樣？               |
//   | vs const_cast   | mutable 合法安全，const_cast 是未定義行為            |
//   | 濫用危險        | 把核心數據設為 mutable 會讓 const 完全失效           |
//   | 設計指導        | mutable 成員應該盡可能少                            |
//


// ============================================================================
//   主程式：執行所有範例
// ============================================================================

int main() {
    cout << "============================================" << endl;
    cout << "   第 23 課：mutable 關鍵字 —— 完整總結" << endl;
    cout << "============================================" << endl;
    cout << endl;

    // 二、基本用法：訪問計數器
    demoBasicMutable();

    // 三、計算快取
    demoCaching();

    // 四、延遲初始化
    demoLazyInit();

    // 六、濫用 vs 正確使用
    demoGoodVsBad();

    // 七、mutable vs const_cast
    demoMutableVsConstCast();

    // 八、綜合範例
    demoComprehensive();

    cout << "============================================" << endl;
    cout << "   總結完畢" << endl;
    cout << "============================================" << endl;

    return 0;
}
