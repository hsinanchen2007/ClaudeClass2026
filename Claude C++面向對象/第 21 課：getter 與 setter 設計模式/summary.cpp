/*
 * ============================================================================
 *  第 21 課：getter 與 setter 設計模式 —— 完整複習總結
 * ============================================================================
 *
 *  本檔案涵蓋第 21 課的所有核心概念，閱讀此檔即可完整複習，無需翻閱其他檔案。
 *
 *  目錄：
 *    第一節：getter 與 setter 的基本概念與語法
 *    第二節：getter 的返回方式選擇（值拷貝 vs const 引用）
 *    第三節：危險的 getter —— 返回非 const 引用的嚴重後果
 *    第四節：用「行為」取代 setter —— 更好的設計思維
 *    第五節：getter/setter 命名慣例（Java 風格 vs C++ 風格 vs STL 風格）
 *    第六節：鏈式調用（Method Chaining）—— setter 返回 *this
 *    第七節：綜合範例 —— RPG 裝備系統（整合所有概念）
 *    第八節：設計決策總結表與本課重點回顧
 *
 * ============================================================================
 */

#include <iostream>
#include <string>
#include <vector>
using namespace std;

/* ============================================================================
 *  第一節：getter 與 setter 的基本概念與語法
 * ============================================================================
 *
 *  什麼是 getter 和 setter？
 *  ─────────────────────────
 *  上一課（第 20 課）學了封裝 —— 把數據藏在 private 裡保護起來。
 *  但外部有時確實需要「讀取」或「修改」某些數據，這時就需要：
 *
 *    getter（取值器）：讓外部「讀取」私有數據
 *    setter（設值器）：讓外部「受控地修改」私有數據
 *
 *  重要觀念：getter/setter 不是給每個成員變數都機械地加上的，
 *           它們是經過設計考量的介面！
 *
 *  ┌─────────────────────────────┐
 *  │  private:                   │
 *  │    int hp_;                 │
 *  │    string name_;            │
 *  │                             │
 *  │  public:                    │
 *  │    int getHp() const;       │  <-- getter：只讀
 *  │    void setHp(int newHp);   │  <-- setter：帶驗證的寫入
 *  └─────────────────────────────┘
 *
 *  getter 的特點：
 *    - 通常是 const 成員函數（保證不修改對象狀態）
 *    - 基本型別（int, double, bool）返回值的拷貝
 *    - 大型物件（string, vector）返回 const 引用，避免拷貝開銷
 *
 *  setter 的特點：
 *    - 允許修改內部狀態，但通常帶有驗證邏輯
 *    - 確保對象始終保持有效狀態（維護不變量）
 *    - 不是每個成員都需要 setter！有些成員只能通過特定行為改變
 */

// ---------- 範例：Player 類別展示基本 getter/setter ----------
class Player {
private:
    string name_;       // 玩家名字
    int hp_;            // 目前生命值
    int maxHp_;         // 最大生命值
    int level_;         // 等級

public:
    // 建構函數：使用初始化列表設定初始值
    Player(const string& name, int maxHp)
        : name_(name), hp_(maxHp), maxHp_(maxHp), level_(1)
    {
    }

    // ====== Getter：返回數據的只讀訪問 ======
    // 注意：getter 通常是 const 成員函數，保證不修改對象狀態

    // 基本型別的 getter：返回值的拷貝（拷貝成本極低）
    int getHp() const { return hp_; }
    int getMaxHp() const { return maxHp_; }
    int getLevel() const { return level_; }

    // 字串的 getter：返回 const 引用（避免不必要的拷貝）
    const string& getName() const { return name_; }

    // ====== Setter：帶驗證的修改 ======
    // setter 允許修改內部狀態，但通常會帶有驗證邏輯，確保對象保持有效狀態

    void setHp(int newHp) {
        // 驗證：維護不變量 —— HP 不能小於 0，也不能超過 maxHp
        if (newHp < 0) newHp = 0;           // 下界保護
        if (newHp > maxHp_) newHp = maxHp_;  // 上界保護
        hp_ = newHp;
    }

    void setName(const string& newName) {
        if (newName.empty()) {              // 驗證：名字不能為空
            cout << "  名字不能為空！" << endl;
            return;                          // 拒絕修改，直接返回
        }
        name_ = newName;
    }

    // 注意：level_ 沒有 setter！
    // 等級只能通過 gainExp() 等遊戲邏輯改變，不開放直接設定
    // 這是一個設計決策 —— 不是所有成員都需要 setter

    void printStatus() const {
        cout << "  " << name_ << " Lv." << level_
             << " HP:" << hp_ << "/" << maxHp_ << endl;
    }
};

/* ============================================================================
 *  第二節：getter 的返回方式選擇
 * ============================================================================
 *
 *  getter 的返回方式很有講究，選錯會造成效能問題或安全問題：
 *
 *  ┌────────────────────┬──────────────────────────────────────┐
 *  │ 型別               │ 建議返回方式                          │
 *  ├────────────────────┼──────────────────────────────────────┤
 *  │ 基本型別            │ 直接返回值（拷貝成本極低）              │
 *  │ (int, double, bool)│ 例如：int getHp() const;              │
 *  ├────────────────────┼──────────────────────────────────────┤
 *  │ 小型物件            │ 直接返回值也可以                       │
 *  │ (如自定義的 Point)  │ 例如：Point getPosition() const;      │
 *  ├────────────────────┼──────────────────────────────────────┤
 *  │ 大型物件            │ 返回 const 引用（避免不必要的拷貝）     │
 *  │ (string, vector)   │ 例如：const string& getName() const;  │
 *  └────────────────────┴──────────────────────────────────────┘
 *
 *  注意：返回引用意味著對象不存在後引用就懸空（dangling reference）
 *
 *  錯誤示範：返回非 const 引用 —— 破壞封裝！
 *    vector<string>& getItemsDangerous() { return items_; }
 *    外部可以直接修改 items_，繞過所有驗證！
 */

// ---------- 範例：Inventory 類別展示不同的返回方式 ----------
class Inventory {
private:
    string ownerName_;          // 擁有者名字
    vector<string> items_;      // 物品列表
    int gold_;                  // 金幣數量

public:
    Inventory(const string& owner, int gold)
        : ownerName_(owner), gold_(gold) {}

    void addItem(const string& item) {
        items_.push_back(item);
    }

    // ====== 返回方式比較 ======

    // 方式 1：返回值（拷貝）—— 適合基本型別
    // 對於基本型別，返回值會自動拷貝，不會影響原物件
    int getGold() const { return gold_; }

    // 方式 2：返回 const 引用 —— 適合大型物件，避免拷貝
    // 返回 const 引用可以讓外部「看」但不能「改」，保護封裝
    const string& getOwnerName() const { return ownerName_; }

    // 方式 3：返回 const 引用 —— 讓外部可以「看」但不能「改」
    // 對於容器等大型物件，返回 const 引用可以避免不必要的拷貝，同時保護封裝
    const vector<string>& getItems() const { return items_; }

    // 方式 4（錯誤示範）：返回非 const 引用 —— 破壞封裝！
    // 這樣做會讓外部直接修改內部狀態，繞過所有驗證邏輯，極易導致錯誤！
    // vector<string>& getItemsDangerous() { return items_; }
    // ↑ 外部可以直接修改 items_，繞過所有驗證！
};

/* ============================================================================
 *  第三節：危險的 getter —— 返回非 const 引用的嚴重後果
 * ============================================================================
 *
 *  這是一個極其常見的封裝破壞模式。
 *
 *  當 getter 返回非 const 引用時：
 *    - 外部可以直接修改內部狀態
 *    - 完全繞過 deposit()、withdraw() 等帶驗證的方法
 *    - 沒有任何記錄、沒有任何驗證
 *    - 封裝形同虛設
 *
 *  教訓：返回非 const 引用等於「把鑰匙交給外人」！
 *
 *  正確做法：
 *    - getter 返回 const 引用（只讀）
 *    - 所有修改必須通過帶驗證的方法（如 deposit、withdraw）
 */

// ---------- 範例：BankAccount 展示危險 getter 的後果 ----------
class BankAccount {
private:
    string owner_;                      // 帳戶持有人
    int balance_;                       // 餘額
    vector<string> transactionLog_;     // 交易記錄

public:
    BankAccount(const string& owner, int initial)
        : owner_(owner), balance_(initial)
    {
        transactionLog_.push_back("開戶：" + to_string(initial));
    }

    // ===== 安全的 getter =====
    // 這些 getter 都是 const 成員函數，保證不修改對象狀態
    int getBalance() const { return balance_; }
    const string& getOwner() const { return owner_; }
    const vector<string>& getLog() const { return transactionLog_; }

    // ===== 危險的 getter（反面教材）=====
    // 返回非 const 引用，允許外部直接修改內部狀態
    // 這樣的設計完全破壞了封裝，外部可以繞過所有驗證邏輯，直接竄改內部數據！
    int& getBalanceDangerous() { return balance_; }
    vector<string>& getLogDangerous() { return transactionLog_; }

    // ===== 正確的修改介面 =====
    // 帶有驗證邏輯，確保對象保持有效狀態，並且記錄所有操作
    bool deposit(int amount) {
        if (amount <= 0) return false;      // 驗證：存款金額必須為正
        balance_ += amount;
        transactionLog_.push_back("存入：" + to_string(amount));
        return true;
    }

    bool withdraw(int amount) {
        if (amount <= 0 || amount > balance_) return false;  // 驗證：金額合理且餘額充足
        balance_ -= amount;
        transactionLog_.push_back("取出：" + to_string(amount));
        return true;
    }

    void printStatement() const {
        cout << "  帳戶：" << owner_ << endl;
        cout << "  餘額：" << balance_ << endl;
        cout << "  交易記錄：" << endl;
        for (const auto& log : transactionLog_) {
            cout << "    " << log << endl;
        }
    }
};

/* ============================================================================
 *  第四節：用「行為」取代 setter —— 更好的設計思維
 * ============================================================================
 *
 *  很多初學者會機械地為每個成員變數都寫 getter/setter，這是錯誤的做法！
 *
 *  錯誤思維：「每個 private 成員都需要 getter 和 setter」
 *  正確思維：「只在有合理需求時才提供 getter 或 setter」
 *
 *  判斷原則：
 *  ─────────
 *  是否需要 getter？
 *    問自己：外部是否有「讀取」這個數據的合理需求？
 *    是 --> 提供 getter
 *    否 --> 不提供（完全隱藏）
 *
 *  是否需要 setter？
 *    問自己：外部是否有「直接修改」這個數據的合理需求？
 *    否 --> 不提供 setter
 *    是 --> 再問：能否用更有意義的「行為」取代？
 *          能 --> 提供行為函數（如 takeDamage、heal、enrage）
 *          不能 --> 提供帶驗證的 setter
 *
 *  三種存取層級：
 *    1. 有 getter，沒有 setter —— 外部需要讀取，但不能直接修改
 *    2. 沒有 getter，也沒有 setter —— 純粹的內部邏輯，外部完全不需要知道
 *    3. 用「行為」取代 setter —— 提供有意義的操作，而非裸露的設值
 */

// ---------- 範例：Enemy 類別展示「行為取代 setter」的設計 ----------
class Enemy {
private:
    string name_;           // 敵人名字
    int hp_;                // 目前生命值
    int maxHp_;             // 最大生命值
    int attackPower_;       // 攻擊力
    int internalId_;        // 內部 ID，外部完全不需要知道
    int aiState_;           // AI 狀態，純粹的內部邏輯

public:
    Enemy(const string& name, int maxHp, int atk)
        : name_(name), hp_(maxHp), maxHp_(maxHp)
        , attackPower_(atk)
        , internalId_(rand())   // 內部使用，無 getter/setter
        , aiState_(0)           // 內部使用，無 getter/setter
    {
    }

    // ===== 有 getter，沒有 setter =====
    // 外部需要讀取，但不能直接修改
    const string& getName() const { return name_; }
    int getHp() const { return hp_; }
    int getMaxHp() const { return maxHp_; }

    // ===== 沒有 getter，也沒有 setter =====
    // internalId_ —— 外部完全不需要知道
    // aiState_    —— 純粹的內部邏輯

    // ===== 用「行為」取代 setter =====

    // 不提供 setHp()，而是提供有意義的行為 takeDamage：
    // 這比 setHp() 好在哪裡？
    //   1. 語義清晰：「受到傷害」比「設定 HP」更直觀
    //   2. 自動驗證：傷害不能為負，HP 不能低於 0
    //   3. 自動觸發副作用：HP 歸零時輸出「被擊敗」
    void takeDamage(int damage) {
        if (damage <= 0) return;            // 驗證：傷害必須為正
        hp_ = max(0, hp_ - damage);         // 保護：HP 不低於 0
        cout << "  " << name_ << " 受到 " << damage
             << " 傷害 (HP:" << hp_ << "/" << maxHp_ << ")" << endl;

        if (hp_ == 0) {                     // 副作用：擊敗判定
            cout << "  " << name_ << " 被擊敗！" << endl;
        }
    }

    // 不提供 setAttackPower()，而是提供有意義的行為 enrage：
    void enrage() {
        attackPower_ *= 2;                  // 暴怒：攻擊力翻倍
        cout << "  " << name_ << " 進入暴怒狀態！ATK="
             << attackPower_ << endl;
    }

    int attack() {
        aiState_++;                          // 內部狀態更新，外部不知道
        cout << "  " << name_ << " 發動攻擊！(ATK:"
             << attackPower_ << ")" << endl;
        return attackPower_;
    }

    bool isAlive() const { return hp_ > 0; }
};

/* ============================================================================
 *  第五節：getter/setter 命名慣例
 * ============================================================================
 *
 *  C++ 中有幾種常見的命名風格：
 *
 *  風格 1：get/set 前綴（Java 風格）
 *    int getHp() const;
 *    void setHp(int hp);
 *    const string& getName() const;
 *    void setName(const string& name);
 *    優點：最清晰易讀，一眼就知道是 getter 還是 setter
 *
 *  風格 2：無前綴，同名函數重載（C++ 風格）
 *    int hp() const;                  // getter：無參數
 *    void hp(int newHp);             // setter：有參數
 *    const string& name() const;     // getter
 *    void name(const string& n);     // setter
 *    優點：更簡潔，符合 C++ 的重載精神
 *
 *  風格 3：STL 風格（getter 用名詞，沒有 setter）
 *    如 vector::size(), string::length()
 *    通常不提供 setter，只提供行為函數
 *
 *  本課程統一使用風格 1（get/set 前綴），因為最清晰易讀。
 *  關鍵原則：在項目中保持一致，不要混用不同風格。
 */

// ---------- 範例：兩種命名風格的對比 ----------

// 風格 1：get/set 前綴（Java 風格）
class StyleJava {
private:
    int hp_;
    string name_;
public:
    StyleJava() : hp_(0), name_("") {}
    int getHp() const { return hp_; }               // getter
    void setHp(int hp) { hp_ = hp; }                // setter
    const string& getName() const { return name_; }  // getter
    void setName(const string& name) { name_ = name; } // setter
};

// 風格 2：無前綴，同名函數重載（C++ 風格）
class StyleCpp {
private:
    int hp_;
    string name_;
public:
    StyleCpp() : hp_(0), name_("") {}
    int hp() const { return hp_; }                    // getter：無參數
    void hp(int newHp) { hp_ = newHp; }              // setter：有參數
    const string& name() const { return name_; }      // getter：無參數
    void name(const string& newName) { name_ = newName; } // setter：有參數
};

/* ============================================================================
 *  第六節：鏈式調用（Method Chaining）
 * ============================================================================
 *
 *  setter 可以返回 *this（自身的引用）來支持鏈式調用。
 *
 *  傳統方式（一行一行設定）：
 *    dlg.setTitle("警告");
 *    dlg.setMessage("確定要刪除嗎？");
 *    dlg.setSize(400, 200);
 *    dlg.show();
 *
 *  鏈式調用（一氣呵成）：
 *    dlg.setTitle("警告")
 *       .setMessage("確定要刪除嗎？")
 *       .setSize(400, 200)
 *       .show();
 *
 *  實現方式：
 *    每個 setter 的返回型別改為 ClassName&，並在最後 return *this;
 *
 *  鏈式調用的關鍵就是 return *this; —— 返回自身的引用，
 *  讓下一個 . 運算子可以繼續調用。
 */

// ---------- 範例：DialogBox 展示鏈式調用 ----------
class DialogBox {
private:
    string title_;      // 對話框標題
    string message_;    // 對話框訊息
    int width_;         // 寬度
    int height_;        // 高度
    bool visible_;      // 是否可見

public:
    DialogBox()
        : title_("未命名"), message_(""), width_(200), height_(100)
        , visible_(false)
    {
    }

    // setter 返回自身引用，支持鏈式調用
    // 注意：返回型別是 DialogBox&，不是 void
    DialogBox& setTitle(const string& title) {
        title_ = title;
        return *this;       // 返回自身，支持鏈式調用
    }

    DialogBox& setMessage(const string& msg) {
        message_ = msg;
        return *this;       // 返回自身，支持鏈式調用
    }

    DialogBox& setSize(int w, int h) {
        width_ = (w > 0) ? w : 200;    // 帶驗證：寬度必須為正
        height_ = (h > 0) ? h : 100;   // 帶驗證：高度必須為正
        return *this;       // 返回自身，支持鏈式調用
    }

    DialogBox& show() {
        visible_ = true;
        return *this;       // 返回自身，支持鏈式調用
    }

    void print() const {
        cout << "  [" << title_ << "] " << message_
             << " (" << width_ << "x" << height_ << ")"
             << " Visible:" << (visible_ ? "Yes" : "No") << endl;
    }
};

/* ============================================================================
 *  第七節：綜合範例 —— RPG 裝備系統
 * ============================================================================
 *
 *  這個綜合範例整合了本課所有概念：
 *    - 有 getter 沒有 setter 的成員（rarity_, basePower_）
 *    - 用行為取代 setter（enhance 取代 setEnhanceLevel，equip/unequip 取代 setIsEquipped）
 *    - 計算屬性的 getter（getPower 調用 calculatePower，不是直接返回成員）
 *    - 帶驗證的類 setter 行為（rename 驗證名字是否為空、長度是否合理）
 *    - 私有輔助函數（maxLevelForRarity, calculatePower）
 *    - enum class 的使用（Rarity 稀有度）
 *
 *  設計決策總結表：
 *  ┌────────────────────┬─────────────┬──────────────────────────┐
 *  │ 成員變數           │ Getter?     │ Setter?                  │
 *  ├────────────────────┼─────────────┼──────────────────────────┤
 *  │ name_              │ 有 getName  │ 無，用 rename() 行為取代  │
 *  │ rarity_            │ 有 getRarity│ 無，稀有度不可改          │
 *  │ basePower_         │ 有          │ 無，基礎值不可改          │
 *  │ enhanceLevel_      │ 有          │ 無，用 enhance() 行為取代 │
 *  │ maxEnhanceLevel_   │ 無（不需要）│ 無，內部邏輯              │
 *  │ isEquipped_        │ 有          │ 無，用 equip/unequip 取代 │
 *  │ calculatePower()   │ 有 getPower │ N/A（計算屬性）           │
 *  └────────────────────┴─────────────┴──────────────────────────┘
 */

// 裝備稀有度（使用 enum class 確保型別安全）
enum class Rarity {
    Common,      // 普通
    Uncommon,    // 非凡
    Rare,        // 稀有
    Epic,        // 史詩
    Legendary    // 傳說
};

// 輔助函數：將稀有度轉換為字串
string rarityToString(Rarity r) {
    switch (r) {
        case Rarity::Common:    return "普通";
        case Rarity::Uncommon:  return "非凡";
        case Rarity::Rare:      return "稀有";
        case Rarity::Epic:      return "史詩";
        case Rarity::Legendary: return "傳說";
    }
    return "未知";
}

// ---------- 綜合範例：Equipment 裝備類別 ----------
class Equipment {
private:
    string name_;               // 裝備名稱
    Rarity rarity_;             // 稀有度
    int basePower_;             // 基礎能力值
    int enhanceLevel_;          // 強化等級
    int maxEnhanceLevel_;       // 最大強化等級（依稀有度決定）
    bool isEquipped_;           // 是否已裝備

    // 私有輔助：根據稀有度決定最大強化等級
    static int maxLevelForRarity(Rarity r) {
        switch (r) {
            case Rarity::Common:    return 5;
            case Rarity::Uncommon:  return 10;
            case Rarity::Rare:      return 15;
            case Rarity::Epic:      return 20;
            case Rarity::Legendary: return 25;
        }
        return 5;
    }

    // 私有輔助：計算實際能力值（每級強化增加 basePower_ 的 10%）
    int calculatePower() const {
        return basePower_ + (basePower_ * enhanceLevel_ / 10);
    }

public:
    Equipment(const string& name, Rarity rarity, int basePower)
        : name_(name)
        , rarity_(rarity)
        , basePower_(basePower > 0 ? basePower : 10)    // 驗證：basePower 必須為正
        , enhanceLevel_(0)
        , maxEnhanceLevel_(maxLevelForRarity(rarity))
        , isEquipped_(false)
    {
    }

    // ====== Getter：只讀訪問 ======
    const string& getName() const { return name_; }
    Rarity getRarity() const { return rarity_; }
    int getBasePower() const { return basePower_; }
    int getEnhanceLevel() const { return enhanceLevel_; }
    int getPower() const { return calculatePower(); }   // 計算屬性，不是直接返回成員
    bool getIsEquipped() const { return isEquipped_; }

    // ====== 沒有直接的 setter！全部用行為取代 ======

    // 「強化」行為取代 setEnhanceLevel
    bool enhance() {
        if (enhanceLevel_ >= maxEnhanceLevel_) {
            cout << "  " << name_ << " 已達最大強化等級！" << endl;
            return false;
        }
        enhanceLevel_++;
        cout << "  " << name_ << " 強化成功！+"
             << enhanceLevel_ << " (能力:" << calculatePower() << ")" << endl;
        return true;
    }

    // 「裝備/卸下」行為取代 setIsEquipped
    void equip() {
        if (isEquipped_) {
            cout << "  " << name_ << " 已經裝備了" << endl;
            return;
        }
        isEquipped_ = true;
        cout << "  裝備了 " << name_ << endl;
    }

    void unequip() {
        if (!isEquipped_) {
            cout << "  " << name_ << " 並未裝備" << endl;
            return;
        }
        isEquipped_ = false;
        cout << "  卸下了 " << name_ << endl;
    }

    // 「改名」— 少數合理的類 setter 行為，帶有多重驗證
    bool rename(const string& newName) {
        if (newName.empty()) {
            cout << "  名字不能為空！" << endl;
            return false;
        }
        if (newName.length() > 20) {
            cout << "  名字太長！（最多 20 個字元）" << endl;
            return false;
        }
        string oldName = name_;
        name_ = newName;
        cout << "  " << oldName << " -> " << name_ << endl;
        return true;
    }

    void printInfo() const {
        cout << "  [" << name_;
        if (enhanceLevel_ > 0) cout << " +" << enhanceLevel_;
        cout << "] " << rarityToString(rarity_)
             << " 能力:" << calculatePower()
             << "(基礎:" << basePower_ << ")"
             << " 強化:" << enhanceLevel_ << "/" << maxEnhanceLevel_
             << " " << (isEquipped_ ? "已裝備" : "背包中") << endl;
    }
};

/* ============================================================================
 *  第八節：本課重點回顧
 * ============================================================================
 *
 *  ┌──────────────────────┬──────────────────────────────────────────┐
 *  │ 概念                 │ 說明                                     │
 *  ├──────────────────────┼──────────────────────────────────────────┤
 *  │ getter 的作用        │ 提供私有數據的只讀訪問                    │
 *  │ setter 的作用        │ 提供帶驗證的受控修改                      │
 *  │ 基本型別 getter      │ 返回值 (int getHp() const)               │
 *  │ 大型物件 getter      │ 返回 const 引用 (const string& getName())│
 *  │ 危險 getter          │ 返回非 const 引用會破壞封裝               │
 *  │ 行為取代 setter      │ 用 takeDamage() 取代 setHp()，更有意義    │
 *  │ 非機械式設計         │ 只在有合理需求時才提供 getter/setter       │
 *  │ 鏈式調用             │ setter 返回 *this 支持連續調用             │
 *  │ 命名慣例             │ getX/setX（Java）或同名重載（C++）        │
 *  │ 設計原則             │ 暴露最少的介面，優先用行為取代直接修改     │
 *  └──────────────────────┴──────────────────────────────────────────┘
 *
 * ============================================================================
 */


// ============================================================================
//  main 函數：逐節展示所有概念
// ============================================================================
int main() {
    cout << "============================================================" << endl;
    cout << "  第 21 課：getter 與 setter 設計模式 —— 完整複習總結" << endl;
    cout << "============================================================" << endl;

    // ================================================================
    // 第一節示範：getter 與 setter 基本用法
    // ================================================================
    cout << "\n\n======== 第一節：getter 與 setter 基本用法 ========" << endl;
    {
        Player hero("勇者", 100);
        hero.printStatus();                 // 勇者 Lv.1 HP:100/100

        // 使用 getter 讀取數據
        cout << "\n--- 使用 Getter 讀取 ---" << endl;
        cout << "  名字：" << hero.getName() << endl;   // 返回 const string&
        cout << "  HP：" << hero.getHp() << endl;       // 返回 int（值拷貝）
        cout << "  等級：" << hero.getLevel() << endl;   // 返回 int（值拷貝）

        // 使用 setter 修改（帶驗證）
        cout << "\n--- 使用 Setter 修改（正常值）---" << endl;
        hero.setHp(60);                     // 正常設定
        hero.printStatus();                 // HP:60/100

        // setter 的保護作用 —— 異常值會被攔截修正
        cout << "\n--- Setter 保護（異常值被攔截修正）---" << endl;
        hero.setHp(-500);                   // 負值被修正為 0
        hero.printStatus();                 // HP:0/100

        hero.setHp(99999);                  // 超過 maxHp 被修正為 maxHp
        hero.printStatus();                 // HP:100/100

        hero.setName("");                   // 空名字被拒絕
        hero.setName("英雄");               // 正常修改
        hero.printStatus();                 // 英雄 Lv.1 HP:100/100

        // hero.setLevel(99);               // 編譯錯誤！level_ 沒有 setter
    }

    // ================================================================
    // 第二節示範：getter 的返回方式
    // ================================================================
    cout << "\n\n======== 第二節：getter 的返回方式 ========" << endl;
    {
        Inventory inv("勇者", 500);
        inv.addItem("鐵劍");
        inv.addItem("治療藥水");
        inv.addItem("火炬");

        // 方式 1：返回值（拷貝）—— 修改拷貝不影響原物件
        int gold = inv.getGold();           // 得到值的拷貝
        gold = 0;                            // 修改的是拷貝
        cout << "  修改拷貝後，實際金幣：" << inv.getGold() << endl;  // 仍然是 500

        // 方式 2：返回 const 引用 —— 避免拷貝，但不能修改
        const string& name = inv.getOwnerName();
        cout << "  擁有者：" << name << endl;
        // name = "壞人";                   // 編譯錯誤！const 引用不能修改

        // 方式 3：返回 const 引用（容器）—— 可以遍歷，但不能修改
        const vector<string>& items = inv.getItems();
        cout << "  物品數量：" << items.size() << endl;
        for (const auto& item : items) {
            cout << "    - " << item << endl;
        }
        // items.push_back("偷加的");       // 編譯錯誤！const 引用不能修改
    }

    // ================================================================
    // 第三節示範：危險的 getter
    // ================================================================
    cout << "\n\n======== 第三節：危險的 getter（反面教材）========" << endl;
    {
        BankAccount account("陳信安", 1000);

        // 正常操作 —— 通過帶驗證的方法
        cout << "\n--- 正常操作（透過 deposit/withdraw）---" << endl;
        account.deposit(500);
        account.withdraw(200);
        account.printStatement();

        // 使用危險的 getter 繞過所有保護！
        cout << "\n--- 使用危險的 getter 直接竄改 ---" << endl;

        // 直接修改餘額，沒有驗證，沒有記錄！
        account.getBalanceDangerous() = 999999;
        cout << "  餘額被直接竄改為：" << account.getBalance() << endl;

        // 直接竄改交易記錄！
        account.getLogDangerous().clear();
        account.getLogDangerous().push_back("一切正常，沒有異常");

        cout << "\n--- 竄改後的帳戶（完全失控）---" << endl;
        account.printStatement();
        // 教訓：返回非 const 引用等於把鑰匙交給外人，封裝形同虛設！
    }

    // ================================================================
    // 第四節示範：用「行為」取代 setter
    // ================================================================
    cout << "\n\n======== 第四節：用行為取代 setter ========" << endl;
    {
        Enemy goblin("哥布林", 50, 15);
        cout << "  " << goblin.getName() << " HP:" << goblin.getHp()
             << "/" << goblin.getMaxHp() << endl;

        // 不是 goblin.setHp(30)，而是用行為 takeDamage：
        goblin.takeDamage(20);              // 語義清晰的行為

        // 不是 goblin.setAttackPower(30)，而是用行為 enrage：
        goblin.enrage();                    // 暴怒：攻擊力翻倍

        // 正常攻擊
        int dmg = goblin.attack();
        cout << "  造成了 " << dmg << " 點傷害" << endl;

        // 以下都不可能做到（封裝保護）：
        // goblin.hp_ = 9999;              // 編譯錯誤！private
        // goblin.internalId_;             // 編譯錯誤！private
        // goblin.aiState_ = -1;           // 編譯錯誤！private
    }

    // ================================================================
    // 第五節示範：命名風格比較
    // ================================================================
    cout << "\n\n======== 第五節：getter/setter 命名慣例 ========" << endl;
    {
        // 風格 1：get/set 前綴（Java 風格）
        cout << "\n--- 風格 1：get/set 前綴（Java 風格）---" << endl;
        StyleJava s1;
        s1.setHp(100);                      // setter：setHp
        s1.setName("風格一");                // setter：setName
        cout << "  " << s1.getName() << " HP:" << s1.getHp() << endl;

        // 風格 2：同名函數重載（C++ 風格）
        cout << "\n--- 風格 2：同名重載（C++ 風格）---" << endl;
        StyleCpp s2;
        s2.hp(200);                          // setter：hp(int)
        s2.name("風格二");                    // setter：name(string)
        cout << "  " << s2.name() << " HP:" << s2.hp() << endl;  // getter

        cout << "\n  兩種風格都可以，關鍵是在項目中保持一致。" << endl;
    }

    // ================================================================
    // 第六節示範：鏈式調用
    // ================================================================
    cout << "\n\n======== 第六節：鏈式調用（Method Chaining）========" << endl;
    {
        // 傳統方式：一行一行設定
        cout << "\n--- 傳統方式 ---" << endl;
        DialogBox dlg1;
        dlg1.setTitle("警告");
        dlg1.setMessage("你確定要刪除嗎？");
        dlg1.setSize(400, 200);
        dlg1.show();
        dlg1.print();

        // 鏈式調用：一氣呵成
        cout << "\n--- 鏈式調用（一氣呵成）---" << endl;
        DialogBox dlg2;
        dlg2.setTitle("歡迎")               // 返回 DialogBox&
            .setMessage("歡迎回到遊戲世界！") // 繼續調用
            .setSize(500, 250)               // 繼續調用
            .show();                          // 繼續調用
        dlg2.print();
    }

    // ================================================================
    // 第七節示範：綜合範例 RPG 裝備系統
    // ================================================================
    cout << "\n\n======== 第七節：綜合範例 RPG 裝備系統 ========" << endl;
    {
        // 創建裝備
        cout << "\n--- 創建裝備 ---" << endl;
        Equipment sword("炎龍劍", Rarity::Epic, 50);
        Equipment shield("橡木盾", Rarity::Common, 30);
        sword.printInfo();
        shield.printInfo();

        // 裝備操作（行為，不是 setter）
        cout << "\n--- 裝備操作（行為取代 setter）---" << endl;
        sword.equip();                       // 正常裝備
        sword.equip();                       // 重複裝備 —— 被攔截
        shield.equip();

        // 強化（行為，不是 setter）
        cout << "\n--- 強化炎龍劍（史詩裝備，上限 20）---" << endl;
        for (int i = 0; i < 5; i++) {
            sword.enhance();
        }

        // 強化普通盾牌到頂
        cout << "\n--- 強化橡木盾（普通裝備，上限 5）---" << endl;
        for (int i = 0; i < 7; i++) {
            shield.enhance();                // 第 6、7 次會失敗
        }

        // 改名（帶驗證的類 setter 行為）
        cout << "\n--- 改名（帶驗證）---" << endl;
        sword.rename("烈焰魔劍");            // 正常改名
        sword.rename("");                    // 空名字被攔截

        // 使用 getter 查看數據
        cout << "\n--- 使用 Getter 查看最終數據 ---" << endl;
        cout << "  武器：" << sword.getName() << endl;
        cout << "  稀有度：" << rarityToString(sword.getRarity()) << endl;
        cout << "  實際能力：" << sword.getPower() << endl;          // 計算屬性
        cout << "  強化等級：" << sword.getEnhanceLevel() << endl;
        cout << "  已裝備：" << (sword.getIsEquipped() ? "是" : "否") << endl;

        // 最終狀態
        cout << "\n--- 最終狀態 ---" << endl;
        sword.printInfo();
        shield.printInfo();
    }

    cout << "\n============================================================" << endl;
    cout << "  複習完畢！核心原則：暴露最少的介面，優先用行為取代直接修改。" << endl;
    cout << "============================================================" << endl;

    return 0;
}
