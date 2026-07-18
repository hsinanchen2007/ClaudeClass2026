/*
 * ============================================================================
 *  第 26 課：this 指標 —— 完整總結
 * ============================================================================
 *
 *  本檔案涵蓋第 26 課所有 .cpp 範例的核心概念，包含：
 *    1. this 的本質與基礎驗證（檔案 1）
 *    2. 參數與成員同名的消歧（檔案 2）
 *    3. 鏈式調用（return *this）（檔案 3）
 *    4. 將 this 傳遞給外部函數（檔案 4）
 *    5. 自我賦值檢查（this == &other）（檔案 5）
 *    6. this 在普通 / const / 靜態成員函數中的類型差異（檔案 6）
 *    7. this 與靜態成員的關係（檔案 7）
 *    8. 建構函數中洩漏 this 的風險（檔案 8）
 *    9. 返回局部對象指標的危險與安全做法（檔案 9）
 *   10. *this 的多種用法總覽（檔案 10）
 *   11. 綜合範例：RPG 隊伍系統（檔案 11）
 *
 *  閱讀本檔案即可完整複習第 26 課全部內容，無需再查閱其他 .cpp 檔案。
 *
 * ============================================================================
 *  概念總覽：
 *
 *  this 是什麼？
 *    - this 是一個隱含的指標，指向調用成員函數的那個對象本身
 *    - 當你寫 hero.takeDamage(30) 時，編譯器實際上做的是：
 *        takeDamage(&hero, 30);
 *      其中 &hero 就是 this
 *
 *  this 的類型：
 *    ┌──────────────────────┬───────────────────────────┐
 *    │ 函數類型             │ this 的類型                │
 *    ├──────────────────────┼───────────────────────────┤
 *    │ void func()          │ MyClass* const this        │
 *    │ void func() const    │ const MyClass* const this  │
 *    │ static void func()   │ 沒有 this                 │
 *    └──────────────────────┴───────────────────────────┘
 *
 *    - 普通成員函數：this 類型為 T* const（指標本身 const，不能改指向；
 *      但可以透過 this 修改成員變數）
 *    - const 成員函數：this 類型為 const T* const（指標和指向的內容都 const，
 *      不能修改成員變數，只能讀取）
 *    - 靜態成員函數：沒有 this，因為靜態函數不屬於任何對象
 *
 *  必須顯式使用 this 的場景：
 *    1. 參數名與成員變數同名時，用 this-> 消歧
 *    2. 返回自身引用以實現鏈式調用（return *this）
 *    3. 將自身傳遞給外部函數（傳 this 或 *this）
 *    4. 自我賦值 / 自我操作檢查（this == &other）
 *    5. 拷貝自身（MyClass copy = *this）
 *    6. 比較後返回自身（return *this 或 return other）
 *
 *  常見誤區：
 *    - 在建構函數中洩漏 this（對象可能尚未完全初始化）
 *    - 返回局部對象的指標（離開作用域後指標懸空）
 *
 * ============================================================================
 */

#include <iostream>
#include <string>
using namespace std;

// ============================================================================
// 第一部分：this 的本質與基礎驗證
// （對應檔案 1：第 26 課：this 指標1.cpp）
//
// 重點：
//   - this 指向調用成員函數的對象，等於該對象的地址 (&obj)
//   - 不同對象調用同一個成員函數時，this 各自不同
//   - 在成員函數中存取成員變數時，name_ 和 this->name_ 完全等價
//     （編譯器會自動將 name_ 轉成 this->name_）
// ============================================================================

class Knight {
private:
    string name_;
    int hp_;

public:
    Knight(const string& name, int hp)
        : name_(name), hp_(hp)
    {
    }

    // 顯示 this 的值（即該對象的地址）
    void showThis() const {
        cout << "  " << name_ << " 的地址：" << this << endl;
    }

    // this 用來識別是哪個對象調用了函數
    void takeDamage(int dmg) {
        cout << "  this = " << this << " → " << name_
             << " 受到 " << dmg << " 傷害" << endl;
        hp_ -= dmg;  // 等價於 this->hp_ -= dmg;
    }

    // 隱式 vs 顯式使用 this
    void printInfo() const {
        cout << "  名字：" << name_ << endl;        // 隱式使用 this（編譯器自動轉換）
        cout << "  名字：" << this->name_ << endl;   // 顯式使用 this（效果完全相同）
    }
};

// ============================================================================
// 第二部分：參數與成員同名時的消歧
// （對應檔案 2：第 26 課：this 指標2.cpp）
//
// 重點：
//   - 當建構函數或 setter 的參數名和成員變數完全相同時，
//     必須用 this-> 來區分成員和參數
//   - 不用 this-> 的話，name = name 會變成參數自我賦值，成員不受影響
//   - 推薦的做法：成員變數加下劃線後綴（如 name_），從根本上避免同名問題
// ============================================================================

class Weapon {
private:
    string name;      // 注意：這裡故意不加下劃線後綴，以示範同名問題
    int damage;
    int durability;

public:
    // 參數名和成員名完全相同，必須用 this-> 消歧
    Weapon(const string& name, int damage, int durability) {
        // 如果寫 name = name; 只是參數給參數（自我賦值），成員不會被設定
        this->name = name;           // this->name 是成員，name 是參數
        this->damage = damage;       // this->damage 是成員，damage 是參數
        this->durability = durability;
    }

    // setter 中也常遇到同名問題
    void setDamage(int damage) {
        if (damage < 0) damage = 0;  // 這裡的 damage 是參數
        this->damage = damage;        // this->damage 是成員
    }

    void print() const {
        cout << "  " << name << " (傷害:" << damage
             << " 耐久:" << durability << ")" << endl;
    }
};

// ============================================================================
// 第三部分：鏈式調用（return *this）
// （對應檔案 3：第 26 課：this 指標3.cpp）
//
// 重點：
//   - 每個方法返回 *this（即自身的引用），就可以繼續在同一個對象上調用下一個方法
//   - 鏈式調用的內部原理：
//       q.from("players").where("lv > 10").limit(20).build();
//     展開後等價於：
//       QueryBuilder& ref1 = q.from("players");   // ref1 就是 q
//       QueryBuilder& ref2 = ref1.where("lv>10"); // ref2 也是 q
//       QueryBuilder& ref3 = ref2.limit(20);      // ref3 也是 q
//       string sql = ref3.build();                 // 最終在 q 上調用
//     全程都是操作同一個對象！
//   - *this 解引用 this 指標，得到對象本身（而非指標）
// ============================================================================

class QueryBuilder {
private:
    string table_;
    string conditions_;
    string orderBy_;
    int limit_;

public:
    QueryBuilder() : limit_(0) {}

    // 返回自身引用，支持鏈式調用
    QueryBuilder& from(const string& table) {
        table_ = table;
        return *this;    // *this 解引用 this 指標，得到對象本身
    }

    QueryBuilder& where(const string& condition) {
        if (!conditions_.empty()) conditions_ += " AND ";
        conditions_ += condition;
        return *this;    // 繼續返回自身引用
    }

    QueryBuilder& orderBy(const string& field) {
        orderBy_ = field;
        return *this;
    }

    QueryBuilder& limit(int n) {
        limit_ = (n > 0) ? n : 0;
        return *this;
    }

    string build() const {
        string sql = "SELECT * FROM " + table_;
        if (!conditions_.empty()) sql += " WHERE " + conditions_;
        if (!orderBy_.empty()) sql += " ORDER BY " + orderBy_;
        if (limit_ > 0) sql += " LIMIT " + to_string(limit_);
        return sql;
    }
};

// ============================================================================
// 第四部分：將 this / *this 傳遞給外部函數
// （對應檔案 4：第 26 課：this 指標4.cpp）
//
// 重點：
//   - this       → 傳遞指標（Player* 類型），用於需要指標的外部函數
//   - *this      → 傳遞引用（Player& 類型），用於需要引用的外部函數
//   - this->name_ → 訪問成員
//   - 實際應用：在建構函數中把自身註冊到某個系統、在成員函數中把自身傳給日誌系統
// ============================================================================

class Player;  // 前向聲明

// 外部系統函數的聲明
void registerPlayer(Player* p);
void logAction(const Player& p, const string& action);

class Player {
private:
    string name_;
    int hp_;

public:
    Player(const string& name, int hp)
        : name_(name), hp_(hp)
    {
        // 在建構時把自身（this 指標）註冊到外部系統
        registerPlayer(this);    // 傳遞 this（指標形式）
    }

    void attack(Player& target) {
        // 把自身的引用（*this）傳給日誌系統
        logAction(*this, "攻擊了 " + target.getName());
        target.takeDamage(20);
    }

    void takeDamage(int dmg) {
        hp_ -= dmg;
        logAction(*this, "受到 " + to_string(dmg) + " 傷害");
    }

    const string& getName() const { return name_; }
    int getHp() const { return hp_; }
};

// 外部函數的實現
void registerPlayer(Player* p) {
    cout << "  [系統] 註冊玩家：" << p->getName()
         << " (地址:" << p << ")" << endl;
}

void logAction(const Player& p, const string& action) {
    cout << "  [日誌] " << p.getName() << " " << action
         << " (HP:" << p.getHp() << ")" << endl;
}

// ============================================================================
// 第五部分：自我賦值檢查（this == &other）
// （對應檔案 5：第 26 課：this 指標5.cpp）
//
// 重點：
//   - 在拷貝賦值運算子（operator=）中，必須檢查自我賦值
//   - 判斷方法：if (this == &other)，即比較自身地址與另一個對象的地址
//   - 如果不檢查自我賦值，buf = buf 會導致：
//       1. 先 delete[] data_（銷毀自己的數據）
//       2. 再從 other.data_（已被銷毀的數據）複製 → 未定義行為！
//   - 檢查到自我賦值後，直接 return *this 跳過賦值邏輯
// ============================================================================

class Buffer {
private:
    int* data_;
    int size_;

public:
    Buffer(int size) : size_(size), data_(new int[size]) {
        for (int i = 0; i < size; i++) data_[i] = i;
        cout << "  [建構] 大小:" << size_ << endl;
    }

    ~Buffer() {
        delete[] data_;
        cout << "  [解構]" << endl;
    }

    // 拷貝賦值運算子——必須檢查自我賦值
    Buffer& operator=(const Buffer& other) {
        cout << "  [賦值] ";

        // 核心：自我賦值檢查
        // this 是左邊的對象，&other 是右邊的對象的地址
        if (this == &other) {
            cout << "偵測到自我賦值，跳過" << endl;
            return *this;  // 直接返回自身，不做任何操作
        }

        // 正常的深拷貝賦值邏輯
        delete[] data_;                                      // 釋放舊的記憶體
        size_ = other.size_;                                 // 複製大小
        data_ = new int[size_];                              // 分配新記憶體
        for (int i = 0; i < size_; i++) data_[i] = other.data_[i];  // 複製數據
        cout << "完成" << endl;

        return *this;  // 返回自身引用，支持 a = b = c 連續賦值
    }

    void print() const {
        cout << "  [";
        for (int i = 0; i < size_; i++) {
            if (i > 0) cout << ", ";
            cout << data_[i];
        }
        cout << "]" << endl;
    }
};

// ============================================================================
// 第六部分：this 的完整類型分析
// （對應檔案 6：第 26 課：this 指標6.cpp）
//
// 重點：
//   - 普通成員函數中：this 類型為 TypeDemo* const
//       → 指向非 const 的 TypeDemo（可以修改成員）
//       → 指標本身是 const（不能讓 this 指向別的對象）
//   - const 成員函數中：this 類型為 const TypeDemo* const
//       → 指向 const 的 TypeDemo（不能修改成員，只能讀取）
//       → 指標本身也是 const
//   - 靜態成員函數中：沒有 this
// ============================================================================

class TypeDemo {
private:
    int value_;

public:
    TypeDemo(int v) : value_(v) {}

    // 普通成員函數——this 類型：TypeDemo* const
    void normalFunc() {
        this->value_ = 42;     // 可以修改成員
        // this = nullptr;     // 錯誤！this 本身是 const，不能改指向
    }

    // const 成員函數——this 類型：const TypeDemo* const
    void constFunc() const {
        // this->value_ = 42;  // 錯誤！指向 const 對象，不能修改成員
        // this = nullptr;     // 錯誤！this 本身也是 const
        int v = this->value_;  // 可以讀取（只讀）
        (void)v;               // 避免未使用變數的警告
    }
};

// ============================================================================
// 第七部分：this 與靜態成員的關係
// （對應檔案 7：第 26 課：this 指標7.cpp）
//
// 重點：
//   - 普通成員函數：有 this，可以存取實例變數和靜態變數
//   - 靜態成員函數：沒有 this，只能存取靜態變數，不能存取實例變數
//   - 靜態變數不需要 this 就能存取（因為它屬於類別，而非特定對象）
//   - 靜態函數可以用 ClassName::staticFunc() 調用，不需要對象
// ============================================================================

class Demo {
private:
    int instanceVar_ = 10;              // 實例變數：每個對象各有一份
    inline static int staticVar_ = 20;  // 靜態變數：類別共享一份

public:
    // 普通成員函數：有 this
    void normalFunc() {
        cout << "  this = " << this << endl;
        cout << "  instanceVar_ = " << this->instanceVar_ << endl;  // 透過 this 存取實例變數
        cout << "  staticVar_ = " << staticVar_ << endl;             // 靜態變數不需要 this
    }

    // 靜態成員函數：沒有 this
    static void staticFunc() {
        // cout << this;         // 錯誤！靜態函數沒有 this
        // cout << instanceVar_; // 錯誤！沒有 this 就不能存取實例變數
        cout << "  staticVar_ = " << staticVar_ << endl;  // 靜態變數可以直接存取
    }
};

// ============================================================================
// 第八部分：建構函數中洩漏 this 的風險
// （對應檔案 8：第 26 課：this 指標8.cpp）
//
// 重點：
//   - 在建構函數中，對象還沒完全初始化完成
//   - 如果在建構函數中把 this 傳給外部函數，外部可能會使用到尚未初始化的成員
//   - 特別危險的情況：繼承時，基類建構完成但派生類還沒建構
//   - 最佳做法：建構完成後再把對象傳遞出去
// ============================================================================

class Dangerous {
private:
    int value_;

public:
    Dangerous(int v) : value_(v) {
        // 在建構函數執行過程中，對象可能還沒完全就緒
        cout << "  建構中... value_ = " << value_ << endl;
        cout << "  this = " << this << " (對象可能還沒完全就緒)" << endl;

        // 以下做法是危險的（已註釋掉）：
        // someGlobalFunction(this);  // 外部函數可能存取尚未初始化的成員
    }
};

// ============================================================================
// 第九部分：返回局部對象指標的危險與安全做法
// （對應檔案 9：第 26 課：this 指標9.cpp）
//
// 重點：
//   - 危險做法：在函數中創建局部對象，然後返回 &local（返回指向局部對象的指標）
//       → 局部對象離開作用域後被銷毀，指標懸空（dangling pointer）
//   - 安全做法一：返回 new 出來的對象（動態分配，堆上的對象不會自動銷毀）
//       → 調用者需要記得 delete
//   - 安全做法二：直接返回值（拷貝/移動語義，最推薦）
//       → 編譯器會優化（RVO / NRVO），通常零開銷
// ============================================================================

class Trap {
private:
    int value_;

public:
    Trap(int v) : value_(v) {}

    // 危險！返回指向局部對象的指標（已註釋掉）
    // static Trap* createBad() {
    //     Trap local(99);
    //     return &local;   // local 離開作用域就被銷毀了！指標懸空！
    // }

    // 安全做法一：返回動態分配的對象
    static Trap* createGood() {
        return new Trap(99);   // 堆上的對象不會自動銷毀，調用者需 delete
    }

    // 安全做法二（推薦）：返回值（拷貝/移動）
    static Trap createBest() {
        return Trap(99);       // 返回值，由編譯器優化（RVO）
    }

    int getValue() const { return value_; }
};

// ============================================================================
// 第十部分：*this 的多種用法總覽
// （對應檔案 10：第 26 課：this 指標10.cpp）
//
// 重點：
//   用法 1：返回自身引用（鏈式調用）     → return *this;
//   用法 2：拷貝自身（不修改原物件）     → Score copy = *this;
//   用法 3：比較自身與另一個對象          → if (this == &other)
//   用法 4：把自身傳給外部函數            → func(*this);
// ============================================================================

class Score {
private:
    string playerName_;
    int points_;

public:
    Score(const string& name, int pts)
        : playerName_(name), points_(pts)
    {
    }

    // 用法 1：返回自身引用（鏈式調用）
    Score& addPoints(int pts) {
        points_ += pts;
        return *this;    // *this 是 Score&（自身的引用）
    }

    // 用法 2：拷貝自身（不修改原物件）
    Score doubled() const {
        Score copy = *this;     // 用 *this 拷貝自身，生成一個新的副本
        copy.points_ *= 2;     // 修改副本，原物件不受影響
        return copy;
    }

    // 用法 3：比較自身與另一個對象
    bool isHigherThan(const Score& other) const {
        if (this == &other) return false;  // 和自己比，永遠返回 false
        return points_ > other.points_;
    }

    // 用法 4：把自身傳給外部函數（函數指標作參數）
    void printVia(void (*printFunc)(const Score&)) const {
        printFunc(*this);    // 把自身（*this）傳給外部函數
    }

    const string& getName() const { return playerName_; }
    int getPoints() const { return points_; }
};

// 用於 Score::printVia 的外部列印函數
void fancyPrint(const Score& s) {
    cout << "  ★ " << s.getName() << "：" << s.getPoints() << " 分 ★" << endl;
}

// ============================================================================
// 第十一部分：綜合範例 —— RPG 隊伍系統
// （對應檔案 11：第 26 課：this 指標11.cpp）
//
// 這個範例綜合展示 this 的所有應用場景：
//   應用 1：leader_ = this        → 把自己設為隊長
//   應用 2：this == &other        → 防止跟隨自己
//   應用 3：return *this          → 鏈式強化（boostHp().boostAttack()）
//   應用 4：betterLeader 中比較    → 比較後返回 *this 或 other
//   應用 5：this == &target       → 判斷是否在自我治療
//   應用 6：leader_ == this       → 判斷自己是否為隊長（printStatus 中使用）
// ============================================================================

class Hero {
private:
    string name_;
    string role_;      // 職業
    int hp_;
    int maxHp_;
    int attack_;
    Hero* leader_;     // 隊長指標

    inline static int heroCount_ = 0;  // 靜態成員：統計英雄總數

public:
    Hero(const string& name, const string& role, int maxHp, int atk)
        : name_(name), role_(role)
        , hp_(maxHp), maxHp_(maxHp), attack_(atk)
        , leader_(nullptr)
    {
        heroCount_++;
        cout << "  [加入] " << role_ << " " << name_ << endl;
    }

    // 應用 1：設定隊長——把自己設為隊長
    // leader_ = this 表示「我自己就是自己的隊長」
    Hero& becomeLeader() {
        leader_ = this;     // this 指向自身，存入 leader_ 指標
        cout << "  ★ " << name_ << " 成為隊長！" << endl;
        return *this;        // 返回自身引用，支持鏈式調用
    }

    // 應用 2：跟隨另一個英雄
    // 用 this == &other 防止自己跟隨自己（邏輯上不合理）
    Hero& follow(Hero& other) {
        if (this == &other) {
            cout << "  " << name_ << "：不能跟隨自己" << endl;
            return *this;
        }
        leader_ = &other;
        cout << "  " << name_ << " 跟隨 " << other.name_ << endl;
        return *this;
    }

    // 應用 3：鏈式強化（return *this 實現連續調用）
    Hero& boostHp(int amount) {
        maxHp_ += amount;
        hp_ += amount;
        cout << "  " << name_ << " HP+" << amount << endl;
        return *this;  // 返回自身引用 → 可繼續呼叫 boostAttack() 等
    }

    Hero& boostAttack(int amount) {
        attack_ += amount;
        cout << "  " << name_ << " ATK+" << amount << endl;
        return *this;
    }

    // 應用 4：比較自身與另一個對象，返回更適合當隊長的那位
    // 用 this == &other 檢查是否在和自己比較
    // 返回 *this 或 other（都是 const 引用）
    const Hero& betterLeader(const Hero& other) const {
        if (this == &other) return *this;  // 和自己比，直接返回自己
        int myScore = maxHp_ + attack_ * 3;
        int otherScore = other.maxHp_ + other.attack_ * 3;
        if (myScore >= otherScore) return *this;  // 自己更強，返回自身
        return other;                              // 對方更強，返回對方
    }

    // 應用 5：治療另一個英雄
    // 用 this == &target 判斷是否在自我治療
    void heal(Hero& target, int amount) {
        if (this == &target) {
            cout << "  " << name_ << " 自我治療 ";  // 治療自己
        } else {
            cout << "  " << name_ << " 治療 " << target.name_ << " ";
        }
        int actual = amount;
        if (target.hp_ + actual > target.maxHp_) {
            actual = target.maxHp_ - target.hp_;  // 不超過最大 HP
        }
        target.hp_ += actual;
        cout << "+" << actual << " (HP:" << target.hp_
             << "/" << target.maxHp_ << ")" << endl;
    }

    const string& getName() const { return name_; }
    int getHp() const { return hp_; }

    void takeDamage(int dmg) {
        hp_ = (hp_ - dmg > 0) ? hp_ - dmg : 0;
    }

    // 應用 6：在狀態顯示中用 leader_ == this 判斷自己是否為隊長
    void printStatus() const {
        cout << "  [" << role_ << "] " << name_
             << " HP:" << hp_ << "/" << maxHp_
             << " ATK:" << attack_;
        if (leader_) {
            if (leader_ == this)              // 用 this 判斷是否隊長
                cout << " (隊長)";
            else
                cout << " (跟隨:" << leader_->name_ << ")";
        }
        cout << endl;
    }
};

// ============================================================================
// main 函數：依序展示所有概念
// ============================================================================

int main() {
    cout << "============================================================" << endl;
    cout << "  第 26 課：this 指標 —— 完整總結" << endl;
    cout << "============================================================" << endl;

    // ====================================================================
    // 第一部分示範：this 的本質與基礎驗證
    // ====================================================================
    cout << "\n========== 第一部分：this 指向誰？ ==========" << endl;
    {
        Knight k1("亞瑟", 200);
        Knight k2("蘭斯洛特", 180);

        // 驗證 this 就是對象的地址
        cout << "\n--- 地址比較 ---" << endl;
        cout << "  &k1 = " << &k1 << endl;
        k1.showThis();   // this 應該等於 &k1
        cout << "  &k2 = " << &k2 << endl;
        k2.showThis();   // this 應該等於 &k2

        // 不同對象調用同一個函數，this 各自不同
        cout << "\n--- 不同對象，不同 this ---" << endl;
        k1.takeDamage(30);
        k2.takeDamage(50);

        // 隱式 vs 顯式
        cout << "\n--- 隱式 vs 顯式 this ---" << endl;
        k1.printInfo();
    }

    // ====================================================================
    // 第二部分示範：同名消歧
    // ====================================================================
    cout << "\n========== 第二部分：同名消歧 ==========" << endl;
    {
        Weapon sword("鐵劍", 25, 100);
        sword.print();

        sword.setDamage(40);
        sword.print();
    }

    // ====================================================================
    // 第三部分示範：鏈式調用
    // ====================================================================
    cout << "\n========== 第三部分：鏈式調用 ==========" << endl;
    {
        // 鏈式調用的原理：每個方法返回 *this，繼續在同一對象上調用
        string sql = QueryBuilder()
            .from("players")
            .where("level > 10")
            .where("hp > 0")
            .orderBy("level DESC")
            .limit(20)
            .build();

        cout << "  " << sql << endl;

        cout << "\n--- 另一個查詢 ---" << endl;
        string sql2 = QueryBuilder()
            .from("items")
            .where("rarity = 'legendary'")
            .where("price < 10000")
            .limit(5)
            .build();

        cout << "  " << sql2 << endl;
    }

    // ====================================================================
    // 第四部分示範：傳遞 this 給外部函數
    // ====================================================================
    cout << "\n========== 第四部分：傳遞 this ==========" << endl;
    {
        cout << "\n--- 創建玩家 ---" << endl;
        Player warrior("戰士", 200);  // 建構時自動調用 registerPlayer(this)
        Player mage("法師", 120);

        cout << "\n--- 戰鬥 ---" << endl;
        warrior.attack(mage);   // 內部用 logAction(*this, ...) 傳遞自身引用
        mage.attack(warrior);
    }

    // ====================================================================
    // 第五部分示範：自我賦值檢查
    // ====================================================================
    cout << "\n========== 第五部分：自我賦值檢查 ==========" << endl;
    {
        Buffer buf(5);
        buf.print();

        // 自我賦值：buf = buf → this == &other，被安全攔截
        buf = buf;
        buf.print();
    }

    // ====================================================================
    // 第六部分示範：this 的類型
    // ====================================================================
    cout << "\n========== 第六部分：this 的類型 ==========" << endl;
    {
        cout << "  普通成員函數中：TypeDemo* const this" << endl;
        cout << "    → 可以修改成員（this->value_ = 42）" << endl;
        cout << "    → 不能改 this 指向（this = nullptr 不行）" << endl;
        cout << endl;
        cout << "  const 成員函數中：const TypeDemo* const this" << endl;
        cout << "    → 不能修改成員（只能讀取）" << endl;
        cout << "    → 不能改 this 指向" << endl;
        cout << endl;
        cout << "  靜態成員函數中：沒有 this" << endl;

        TypeDemo td(10);
        td.normalFunc();   // 可以修改 value_
        td.constFunc();    // 只能讀取 value_
    }

    // ====================================================================
    // 第七部分示範：this 與靜態成員
    // ====================================================================
    cout << "\n========== 第七部分：this 與靜態成員 ==========" << endl;
    {
        Demo d;
        cout << "\n--- 普通函數（有 this）---" << endl;
        d.normalFunc();     // 可以存取實例變數和靜態變數

        cout << "\n--- 靜態函數（沒有 this）---" << endl;
        Demo::staticFunc(); // 只能存取靜態變數，不能存取實例變數
    }

    // ====================================================================
    // 第八部分示範：建構函數中洩漏 this 的風險
    // ====================================================================
    cout << "\n========== 第八部分：建構中洩漏 this 的風險 ==========" << endl;
    {
        Dangerous d(42);
        cout << "  建構完成，現在使用 this 才安全" << endl;
    }

    // ====================================================================
    // 第九部分示範：安全的創建方式
    // ====================================================================
    cout << "\n========== 第九部分：安全的創建方式 ==========" << endl;
    {
        // 安全做法一：動態分配（需要 delete）
        Trap* p = Trap::createGood();
        cout << "  動態創建：" << p->getValue() << endl;
        delete p;

        // 安全做法二（推薦）：值返回（編譯器 RVO 優化）
        Trap t = Trap::createBest();
        cout << "  值返回：" << t.getValue() << endl;
    }

    // ====================================================================
    // 第十部分示範：*this 的多種用法
    // ====================================================================
    cout << "\n========== 第十部分：*this 的各種用法 ==========" << endl;
    {
        Score s("陳信安", 100);

        // 用法 1：鏈式調用（return *this）
        cout << "\n--- 鏈式加分 ---" << endl;
        s.addPoints(50).addPoints(30).addPoints(20);
        cout << "  總分：" << s.getPoints() << endl;  // 100+50+30+20 = 200

        // 用法 2：拷貝自身（Score copy = *this）
        cout << "\n--- 翻倍（不影響原物件）---" << endl;
        Score doubled = s.doubled();
        cout << "  原始：" << s.getPoints() << endl;      // 200（不變）
        cout << "  翻倍：" << doubled.getPoints() << endl; // 400

        // 用法 3：比較（this == &other）
        cout << "\n--- 比較 ---" << endl;
        Score other("對手", 150);
        cout << "  " << s.getName() << " 高於 " << other.getName() << "？ "
             << (s.isHigherThan(other) ? "是" : "否") << endl;
        cout << "  和自己比？ "
             << (s.isHigherThan(s) ? "是" : "否") << endl;  // 和自己比返回 false

        // 用法 4：傳遞 *this 給外部函數
        cout << "\n--- 傳遞 *this ---" << endl;
        s.printVia(fancyPrint);
    }

    // ====================================================================
    // 第十一部分示範：綜合範例 —— RPG 隊伍系統
    // ====================================================================
    cout << "\n========== 第十一部分：RPG 隊伍系統綜合範例 ==========" << endl;
    {
        // 創建隊伍
        cout << "\n=== 創建英雄 ===" << endl;
        Hero warrior("亞瑟", "戰士", 300, 40);
        Hero mage("梅林", "法師", 150, 70);
        Hero healer("伊蓮", "治療師", 200, 20);

        // 鏈式強化（應用 3：return *this）
        cout << "\n=== 鏈式強化 ===" << endl;
        warrior.boostHp(50).boostAttack(10).boostHp(30);
        mage.boostAttack(20);

        // 比較誰更適合當隊長（應用 4：比較後返回 *this 或 other）
        cout << "\n=== 選擇隊長 ===" << endl;
        const Hero& better = warrior.betterLeader(mage);
        cout << "  更適合當隊長：" << better.getName() << endl;

        // 組隊
        cout << "\n=== 組隊 ===" << endl;
        warrior.becomeLeader();      // 應用 1：leader_ = this
        mage.follow(warrior);        // 設定 leader_ = &warrior
        healer.follow(warrior);

        // 嘗試跟隨自己（應用 2：this == &other 檢測）
        warrior.follow(warrior);     // 會被攔截

        // 隊伍狀態（應用 6：leader_ == this 判斷是否隊長）
        cout << "\n=== 隊伍狀態 ===" << endl;
        warrior.printStatus();
        mage.printStatus();
        healer.printStatus();

        // 戰鬥
        cout << "\n=== 戰鬥 ===" << endl;
        warrior.takeDamage(120);
        mage.takeDamage(80);
        cout << "  亞瑟 HP:" << warrior.getHp() << endl;
        cout << "  梅林 HP:" << mage.getHp() << endl;

        // 治療（應用 5：this == &target 判斷自我治療）
        cout << "\n=== 治療 ===" << endl;
        healer.heal(warrior, 60);
        healer.heal(mage, 100);
        healer.heal(healer, 30);    // 自我治療

        // 最終狀態
        cout << "\n=== 最終狀態 ===" << endl;
        warrior.printStatus();
        mage.printStatus();
        healer.printStatus();
    }

    // ====================================================================
    // 總結表格
    // ====================================================================
    cout << "\n============================================================" << endl;
    cout << "  this 指標使用場景總結" << endl;
    cout << "============================================================" << endl;
    cout << "  場景                         用法" << endl;
    cout << "  ----------------------------+----------------------------" << endl;
    cout << "  參數與成員同名               this->name = name;" << endl;
    cout << "  鏈式調用                     return *this;" << endl;
    cout << "  傳遞自身給外部函數（指標）    func(this);" << endl;
    cout << "  傳遞自身給外部函數（引用）    func(*this);" << endl;
    cout << "  自我賦值檢查                 if (this == &other)" << endl;
    cout << "  自我操作檢查                 if (this == &target)" << endl;
    cout << "  比較後返回自身               return *this;" << endl;
    cout << "  拷貝自身                     MyClass copy = *this;" << endl;
    cout << "============================================================" << endl;

    cout << "\n  本課重點回顧：" << endl;
    cout << "  - this 本質：指向當前對象的隱含指標" << endl;
    cout << "  - this 的類型：普通函數 T* const；const 函數 const T* const" << endl;
    cout << "  - 靜態函數：沒有 this" << endl;
    cout << "  - this->member：顯式訪問成員（通常可省略）" << endl;
    cout << "  - *this：解引用得到對象本身，用於返回引用或傳遞" << endl;
    cout << "  - return *this：鏈式調用的核心" << endl;
    cout << "  - this == &other：自我賦值/自我操作檢查" << endl;
    cout << "  - 同名消歧：用 this-> 區分參數和成員（建議用下劃線後綴避免）" << endl;
    cout << "  - 建構中的 this：對象未完全建構，傳出 this 有風險" << endl;

    return 0;
}
