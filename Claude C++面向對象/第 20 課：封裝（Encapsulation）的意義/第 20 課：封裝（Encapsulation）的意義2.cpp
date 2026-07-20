// =============================================================================
//  第 20 課：封裝的意義 2  —  用「動詞」取代「設值」：不變式由誰負責維護
// =============================================================================
//
// 【主題資訊 Information】
//   語法       ：class 內以 private / public 劃分存取區段
//   標準版本   ：存取控制 C++98；本檔對照提到的 std::clamp 是 C++17
//   標頭檔     ：<algorithm>（min/max/clamp）
//   核心概念   ：class invariant 由「唯一入口」集中維護
//   執行期成本 ：存取控制為零成本；檢查邏輯本身的成本則是刻意付出的代價
//
// 【詳細解釋 Explanation】
//   （第 1 檔示範了「沒有封裝的災難」，本檔示範對應的正確做法。
//     兩檔請對照閱讀：同樣的六個災難，在這裡全部被擋下來。）
//
// 【1. 封裝的本質：把「可能出錯的範圍」縮小到一個檔案】
//   第 1 檔的問題是「hp 可以在專案的任何角落被改」，所以出事時兇手可能
//   在任何地方。本檔把 hp_ 設為 private 之後，能夠修改它的程式碼
//   只剩下這個 class 內部的幾十行。這不是「讓別人碰不到」的潔癖，
//   而是把除錯的搜尋範圍從「整個專案」縮小到「一個 class」。
//   這個性質有個名字叫「局部推理（local reasoning）」：
//   你只要讀完這個 class，就能確信不變式成立，不必讀完整個專案。
//
// 【2. 為什麼是 takeDamage(30) 而不是 setHp(getHp() - 30)】
//   這是本課最關鍵的一點。比較兩種介面：
//       setHp(int v)         // 「把血量設成 v」——不知道呼叫端想幹嘛
//       takeDamage(int d)    // 「受到 d 點傷害」——知道這是一次攻擊
//   差別在於「語意」。setHp 只是一個賦值管道，它無從判斷 v 是否合理，
//   因為它不知道這次修改代表什麼事件。takeDamage 則知道：
//     * 傷害必須是正數（負傷害等於治療，那是另一個操作）；
//     * 結果不能低於 0；
//     * 血量歸零時要觸發「倒下」的事件。
//   這三條規則只寫一次、寫在唯一的入口，就不可能被繞過。
//   換句話說：封裝真正封的不是「資料」，而是「改變資料的規則」。
//
// 【3. 建構子：不變式的起點】
//   不變式必須「從物件誕生的第一刻就成立」，否則後面的檢查都沒有意義。
//   本檔的建構子用了三元運算子做防禦：
//       hp_(maxHp > 0 ? maxHp : 100)
//   這保證了即使呼叫端傳入 0 或負數，物件仍然處於合法狀態。
//   注意這裡有個更嚴格的設計選項：直接丟例外拒絕建立物件。
//   兩種都是合理的設計，取捨在於「悄悄修正」與「大聲失敗」哪個更適合：
//     * 遊戲設定檔讀進來的值 → 修正成預設值比讓遊戲崩潰好；
//     * 金融交易的金額       → 應該立刻丟例外，絕不能悄悄改成別的數字。
//   本檔選擇前者是因為它是教學範例；實務上請依領域決定。
//
// 【4. 成員初始化列表的順序陷阱】
//   建構子的初始化列表看起來是「照你寫的順序」執行，其實不是——
//   成員一律依照「在 class 中宣告的順序」初始化，與初始化列表的書寫順序無關。
//   本檔的宣告順序是 name_, hp_, maxHp_, attack_, level_, exp_, gold_，
//   初始化列表也照這個順序寫，所以沒有問題。但如果寫成：
//       Character(...) : maxHp_(m), hp_(maxHp_) { }    // 危險！
//   由於 hp_ 宣告在 maxHp_ 之前，hp_ 會「先」被初始化，
//   此時 maxHp_ 還沒有值，讀它是未定義行為。
//   GCC 的 -Wreorder 會警告這種「書寫順序與宣告順序不一致」的情況，
//   建議一律開啟。本檔已用 -Wall -Wextra 編譯並確認無此警告。
//
// 【5. 私有輔助函式：連「內部規則」也要收斂】
//   expToNextLevel() 與 checkLevelUp() 是 private 的。這說明封裝有兩層：
//     * 對外：藏起資料，只暴露有意義的操作；
//     * 對內：把「升級公式」這種會變動的規則收斂成一個函式，
//             讓 gainExp() 不必知道公式長什麼樣。
//   哪天升級公式從 level*100 改成指數成長，只要改 expToNextLevel() 一處。
//
// 【概念補充 Concept Deep Dive】
//   * const 成員函式（printStatus、isAlive、getName）是封裝的一部分：
//     它們在型別層次宣告「我不會修改物件」，讓 const Character& 也能呼叫。
//     這使得「唯讀存取」與「修改」在型別上就被區分開來。
//   * getName() 回傳 const string& 而不是 string，避免每次查詢都複製一份字串；
//     回傳 const 參考則確保呼叫端不能透過它修改內部狀態。
//     若寫成回傳非 const 的 string&，封裝就破功了——呼叫端可以直接改名字，
//     等於把 private 又打開了一個洞。
//   * 本檔沒有提供 setHp / setLevel 這類 setter，這是刻意的。
//     一個好的判準是：「這個 setter 對應到領域裡的哪個事件？」
//     如果答不出來，它多半就不該存在。
//   * spendGold 回傳 bool 而不是丟例外，是因為「金幣不足」是可預期的
//     正常流程分支，不是例外狀況。這也是一種設計判斷。
//
// 【注意事項 Pay Attention】
//   1. 成員初始化順序依「宣告順序」，不依初始化列表的書寫順序；請開 -Wreorder。
//   2. 不要為了「完整」而替每個成員都配 getter/setter；沒有語意的 setter
//      等於沒有封裝（詳見第 1 檔的陷阱題）。
//   3. 回傳 private 成員的非 const 參考／指標會讓封裝破功。
//   4. 建構子要保證「物件一誕生不變式就成立」，否則後面的檢查沒有意義。
//   5. 「悄悄修正」與「丟例外」兩種防禦策略各有適用場合，要依領域選擇，
//      不要無條件套用其中一種。
//   6. 存取控制是編譯期機制，擋的是意外與維護成本，不是惡意攻擊。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】封裝的實作與介面設計
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼提供 takeDamage(int) 比提供 setHp(int) 好？
//     答：因為 setHp 只是一個賦值管道，它不知道呼叫端想表達什麼事件，
//         也就無從判斷什麼值才合理。takeDamage 帶有語意——它知道這是一次
//         攻擊，所以能集中套用「傷害必須為正、結果不得低於 0、
//         歸零要觸發死亡事件」這些規則，而且只寫一次、不可能被繞過。
//         封裝封的不是資料，是「改變資料的規則」。
//     追問：那什麼時候 setter 是合理的？→ 當它對應到領域中一個真實事件、
//         而且真的有檢查時，例如 setNickname() 檢查長度與違禁字。
//         判準是「這個 setter 對應到哪個事件」，答不出來就不該有它。
//
// 🔥 Q2. 建構子的成員初始化列表，執行順序由什麼決定？
//     答：由成員在 class 中的「宣告順序」決定，與初始化列表的書寫順序無關。
//         所以寫成 : maxHp_(m), hp_(maxHp_) 時，若 hp_ 宣告在 maxHp_ 之前，
//         hp_ 會先被初始化，此時 maxHp_ 尚未有值，讀取它是未定義行為。
//         GCC 用 -Wreorder 可以警告這種不一致。
//     追問：為什麼標準要規定成這樣？→ 因為解構順序必須是建構順序的嚴格逆序，
//         而解構子沒有「初始化列表」可以參考。若建構順序能被每個建構子
//         各自決定，同一個 class 的不同建構子就會有不同的解構順序，
//         無法一致地產生解構程式碼。
//
// ⚠️ 陷阱. 這個 getter 為什麼讓封裝完全失效？
//         class Inventory {
//             std::vector<Item> items_;
//         public:
//             std::vector<Item>& getItems() { return items_; }   // 非 const 參考
//         };
//     答：它回傳的是內部容器的「非 const 參考」，等於把 private 成員的
//         完整寫入權直接交給呼叫端：
//             inv.getItems().clear();                  // 清空整個背包
//             inv.getItems().push_back(cheatItem);     // 繞過所有規則塞道具
//         這比直接把 items_ 宣告成 public 更糟——因為它「看起來」有封裝，
//         讓人誤以為受到保護。而且若 Inventory 有「總重量」之類的快取欄位，
//         這樣改完之後快取就與實際內容不一致了。
//     為什麼會錯：多數人把 getter 理解成「唯讀存取」，
//         但唯讀是由 const 保證的，不是由「名字叫 get」保證的。
//         正確做法是回傳 const 參考（const std::vector<Item>&），
//         或只回傳需要的資訊（size()、contains()），
//         真的要修改就提供 addItem() / removeItem() 這類有規則的操作。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <algorithm>   // std::clamp (C++17)
#include <stdexcept>
#include <vector>
using namespace std;

class Character {
private:
    // ====== 私有數據：外部無法直接碰 ======
    // 這些成員變數被封裝在類內，外部程式無法直接訪問或修改它們，必須通過公開的函數來操作，確保數據的合法性和一致性。
    string name_;
    int hp_;
    int maxHp_;
    int attack_;
    int level_;
    int exp_;
    int gold_;

    // ====== 私有輔助函數 ======
    // 計算升級所需的經驗值
    int expToNextLevel() const {
        return level_ * 100;   // 每級需要 level * 100 經驗
    }

    void checkLevelUp() {
        while (exp_ >= expToNextLevel()) {
            exp_ -= expToNextLevel();
            level_++;
            maxHp_ += 20;
            hp_ = maxHp_;       // 升級回滿血
            attack_ += 5;
            cout << "  ★ " << name_ << " 升級！Lv." << level_
                 << " (HP+" << 20 << " ATK+" << 5 << ")" << endl;
        }
    }

public:
    // ====== 建構函數：確保初始狀態合法 ======
    // 建構函數負責創建對象時的初始化，確保所有成員變數都被賦予合理的初始值，避免未定義行為。
    Character(const string& name, int maxHp, int attack)
        : name_(name)
        , hp_(maxHp > 0 ? maxHp : 100)         // 防止無效值
        , maxHp_(maxHp > 0 ? maxHp : 100)
        , attack_(attack > 0 ? attack : 10)
        , level_(1)
        , exp_(0)
        , gold_(0)
    {
        cout << "  [創建] " << name_ << " HP:" << hp_
             << " ATK:" << attack_ << endl;
    }

    // ====== 公開介面：受控的操作 ======
    // 這些函數提供了受控的方式來操作角色的狀態，確保數據的一致性和合法性。

    // 受傷——有保護的 HP 減少
    void takeDamage(int damage) {
        if (damage <= 0) {
            cout << "  " << name_ << "：無效的傷害值" << endl;
            return;
        }
        hp_ = max(0, hp_ - damage);   // HP 不會低於 0
        cout << "  " << name_ << " 受到 " << damage
             << " 點傷害 (HP: " << hp_ << "/" << maxHp_ << ")" << endl;

        if (hp_ == 0) {
            cout << "  " << name_ << " 倒下了！" << endl;
        }
    }

    // 治療——有保護的 HP 增加
    void heal(int amount) {
        if (amount <= 0 || hp_ == 0) {
            cout << "  " << name_ << "：無法治療" << endl;
            return;
        }
        int oldHp = hp_;
        hp_ = min(hp_ + amount, maxHp_);   // 不超過上限
        cout << "  " << name_ << " 恢復 " << (hp_ - oldHp)
             << " 點生命 (HP: " << hp_ << "/" << maxHp_ << ")" << endl;
    }

    // 獲得經驗——自動檢查升級
    void gainExp(int amount) {
        if (amount <= 0) return;
        exp_ += amount;
        cout << "  " << name_ << " 獲得 " << amount << " 經驗值" << endl;
        checkLevelUp();   // 內部自動處理升級邏輯
    }

    // 獲得金幣——有上限保護
    void earnGold(int amount) {
        if (amount <= 0) return;
        gold_ = min(gold_ + amount, 999999);   // 金幣上限
        cout << "  " << name_ << " 獲得 " << amount
             << " 金幣 (總計: " << gold_ << ")" << endl;
    }

    // 花費金幣——檢查是否足夠
    bool spendGold(int amount) {
        if (amount <= 0 || amount > gold_) {
            cout << "  " << name_ << "：金幣不足！"
                 << "(需要 " << amount << "，擁有 " << gold_ << ")" << endl;
            return false;
        }
        gold_ -= amount;
        cout << "  " << name_ << " 花費 " << amount
             << " 金幣 (剩餘: " << gold_ << ")" << endl;
        return true;
    }

    // 顯示狀態——只讀，不修改
    void printStatus() const {
        cout << "  ┌─────────────────────┐" << endl;
        cout << "  │ " << name_ << " (Lv." << level_ << ")" << endl;
        cout << "  │ HP:  " << hp_ << " / " << maxHp_ << endl;
        cout << "  │ ATK: " << attack_ << endl;
        cout << "  │ EXP: " << exp_ << " / " << expToNextLevel() << endl;
        cout << "  │ Gold: " << gold_ << endl;
        cout << "  └─────────────────────┘" << endl;
    }

    // 查詢函數（只讀）
    bool isAlive() const { return hp_ > 0; }
    const string& getName() const { return name_; }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 155. Min Stack
//
// 題目：設計一個堆疊，支援 push / pop / top，以及在「常數時間」取得堆疊中
//       的最小值 getMin()。
// 為什麼用到本主題：這題本質上就是一道封裝題。它的不變式是
//       「minStack_ 的頂端，永遠等於 dataStack_ 目前內容的最小值」。
//   這個條件橫跨兩個成員變數，任何一次 push 或 pop 都必須「同時」
//   更新兩者才能維持。如果把兩個 vector 開成 public 讓外部自由操作，
//   只要有人只動其中一個，getMin() 就永遠錯了，而且錯得無聲無息。
//   把它們設為 private、只透過 push/pop 修改，正是本課要講的
//   「用唯一入口維護跨成員不變式」。
// 複雜度：push / pop / top / getMin 皆為攤銷 O(1)；空間 O(n)。
// -----------------------------------------------------------------------------
class MinStack {
public:
    MinStack() = default;

    void push(int val) {
        data_.push_back(val);
        // 不變式維護點：同一個操作內同時更新兩個容器
        if (min_.empty() || val <= min_.back()) {
            min_.push_back(val);
        } else {
            min_.push_back(min_.back());
        }
    }

    void pop() {
        if (data_.empty()) return;      // 防禦：空堆疊 pop 不做事
        data_.pop_back();
        min_.pop_back();                // 兩者必須同進同出
    }

    int top() const {
        if (data_.empty()) throw std::logic_error("top() on empty stack");
        return data_.back();
    }

    int getMin() const {
        if (min_.empty()) throw std::logic_error("getMin() on empty stack");
        return min_.back();
    }

    bool empty() const { return data_.empty(); }
    size_t size() const { return data_.size(); }

private:
    // 兩者必須永遠等長，且 min_.back() 永遠是 data_ 的最小值。
    // 這個不變式只有靠「不開放外部直接存取」才可能被保證。
    vector<int> data_;
    vector<int> min_;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】銀行帳戶：跨成員不變式 + 「大聲失敗」的防禦策略
//
// 情境：帳戶要同時維護「餘額」與「交易紀錄」，而且必須保證
//   balance_ 永遠等於所有交易紀錄的總和，同時餘額不得為負。
// 為何用到本主題：這裡刻意採用與 Character 相反的防禦策略——
//   Character 對不合法輸入是「悄悄修正」（適合遊戲設定），
//   而金額錯誤絕不能悄悄改成別的數字，所以這裡改成「丟例外拒絕」。
//   同一個封裝原則，依領域選擇不同的失敗方式，這才是實務判斷。
// -----------------------------------------------------------------------------
class BankAccount {
public:
    explicit BankAccount(string owner, long long openingCents)
        : owner_(std::move(owner)), balanceCents_(0) {
        if (openingCents < 0) {
            throw std::invalid_argument("開戶金額不可為負");
        }
        deposit(openingCents);          // 走同一個入口，紀錄也會一併寫入
    }

    void deposit(long long cents) {
        if (cents <= 0) throw std::invalid_argument("存款金額必須為正");
        balanceCents_ += cents;
        history_.push_back(cents);      // 不變式：餘額 == 紀錄總和
    }

    // 回傳 bool 而非丟例外：餘額不足是「可預期的正常分支」，不是例外狀況
    bool withdraw(long long cents) {
        if (cents <= 0) throw std::invalid_argument("提款金額必須為正");
        if (cents > balanceCents_) return false;   // 不變式：餘額不得為負
        balanceCents_ -= cents;
        history_.push_back(-cents);
        return true;
    }

    long long balanceCents() const { return balanceCents_; }
    const string& owner() const { return owner_; }
    size_t transactionCount() const { return history_.size(); }

    // 注意：這裡回傳的是 const 參考，呼叫端讀得到但改不了。
    // 若寫成非 const 的 vector<long long>&，封裝立刻破功。
    const vector<long long>& history() const { return history_; }

    // 自我稽核：驗證不變式確實成立（正式產品裡通常寫成 assert 或單元測試）
    bool checkInvariant() const {
        long long sum = 0;
        for (long long t : history_) sum += t;
        return sum == balanceCents_ && balanceCents_ >= 0;
    }

private:
    string            owner_;
    long long         balanceCents_;    // 用「分」為單位，避免浮點誤差
    vector<long long> history_;
};

int main() {
    cout << "============================================" << endl;
    cout << "   第 20 課：封裝的意義" << endl;
    cout << "============================================" << endl;

    // 創建角色——建構函數保證初始狀態合法
    cout << "\n=== 創建角色 ===" << endl;
    Character hero("勇者", 100, 25);
    hero.printStatus();

    // 正常戰鬥流程
    cout << "\n=== 戰鬥 ===" << endl;
    hero.takeDamage(30);
    hero.takeDamage(50);
    hero.heal(40);

    // 嘗試非法操作——全部被攔截
    cout << "\n=== 嘗試非法操作 ===" << endl;
    hero.takeDamage(-100);     // 負傷害？不允許
    hero.heal(-50);            // 負治療？不允許

    // 經驗與升級——自動管理
    cout << "\n=== 經驗與升級 ===" << endl;
    hero.gainExp(80);
    hero.gainExp(50);     // 應該觸發升級（80+50=130 >= 100）
    hero.printStatus();

    // 金幣系統——有保護
    cout << "\n=== 金幣系統 ===" << endl;
    hero.earnGold(200);
    hero.spendGold(150);
    hero.spendGold(100);   // 應該失敗——金幣不足

    // 致命傷害
    cout << "\n=== 致命傷害 ===" << endl;
    hero.takeDamage(9999);
    hero.heal(50);          // 已死亡，無法治療

    hero.printStatus();

    // ────────────────────────────────────────────────────────────
    cout << "\n=== LeetCode 155. Min Stack ===" << endl;
    {
        MinStack st;
        st.push(-2);
        st.push(0);
        st.push(-3);
        cout << "  push -2, 0, -3 後 getMin() = " << st.getMin() << endl;  // -3
        st.pop();
        cout << "  pop 之後 top()   = " << st.top() << endl;               // 0
        cout << "  pop 之後 getMin() = " << st.getMin() << endl;           // -2

        // 示範不變式在各種操作下都成立
        st.push(-5);
        cout << "  push -5 後 getMin() = " << st.getMin() << endl;         // -5
        st.pop();
        cout << "  再 pop 後 getMin()  = " << st.getMin() << endl;         // -2
        cout << "  → data_ 與 min_ 是 private，外部不可能只動其中一個" << endl;
    }

    cout << "\n=== 日常實務：銀行帳戶（跨成員不變式）===" << endl;
    {
        BankAccount acc("王小明", 100000);   // 開戶 1000.00 元（以分為單位）
        cout << "  開戶餘額 = " << acc.balanceCents() << " 分" << endl;

        acc.deposit(25050);                  // 存入 250.50 元
        cout << "  存入 25050 分後餘額 = " << acc.balanceCents() << " 分" << endl;

        cout << "  提款 50000 分 → " << (acc.withdraw(50000) ? "成功" : "失敗")
             << "，餘額 = " << acc.balanceCents() << " 分" << endl;

        cout << "  提款 999999 分 → " << (acc.withdraw(999999) ? "成功" : "失敗")
             << "（餘額不足，屬正常分支，回傳 false 而非丟例外）" << endl;

        cout << "  交易筆數 = " << acc.transactionCount() << endl;
        cout << "  不變式檢查（餘額 == 紀錄總和 且 餘額 >= 0）："
             << (acc.checkInvariant() ? "成立" : "不成立") << endl;

        // 金額錯誤屬於「不該悄悄修正」的類型，所以丟例外
        try {
            acc.deposit(-100);
        } catch (const std::invalid_argument& e) {
            cout << "  存入 -100 分 → 捕捉到例外：" << e.what() << endl;
        }
        cout << "  例外之後餘額不變 = " << acc.balanceCents() << " 分" << endl;
        cout << "  → 與 Character 的「悄悄修正」不同：金額必須大聲失敗" << endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -Wreorder "第 20 課：封裝（Encapsulation）的意義2.cpp" -o enc2
//
// 說明：
//   1. 本檔已用 -Wreorder 額外驗證過：成員初始化列表的書寫順序與
//      class 內的宣告順序一致，沒有初始化順序陷阱。
//   2. 升級後的數值（Lv.2、HP 120、ATK 30）來自 class 內寫死的成長公式
//      （每級 maxHp +20、attack +5、expToNextLevel = level * 100），
//      不是標準或編譯器決定的。
//   3. BankAccount 一律以「分」為單位用 long long 儲存金額，
//      刻意避開浮點數表示誤差——金融計算不應使用 double。
//   4. 本檔所有輸出都是確定的：沒有用到亂數、時間或位址。

// === 預期輸出 ===
// ============================================
//    第 20 課：封裝的意義
// ============================================
//
// === 創建角色 ===
//   [創建] 勇者 HP:100 ATK:25
//   ┌─────────────────────┐
//   │ 勇者 (Lv.1)
//   │ HP:  100 / 100
//   │ ATK: 25
//   │ EXP: 0 / 100
//   │ Gold: 0
//   └─────────────────────┘
//
// === 戰鬥 ===
//   勇者 受到 30 點傷害 (HP: 70/100)
//   勇者 受到 50 點傷害 (HP: 20/100)
//   勇者 恢復 40 點生命 (HP: 60/100)
//
// === 嘗試非法操作 ===
//   勇者：無效的傷害值
//   勇者：無法治療
//
// === 經驗與升級 ===
//   勇者 獲得 80 經驗值
//   勇者 獲得 50 經驗值
//   ★ 勇者 升級！Lv.2 (HP+20 ATK+5)
//   ┌─────────────────────┐
//   │ 勇者 (Lv.2)
//   │ HP:  120 / 120
//   │ ATK: 30
//   │ EXP: 30 / 200
//   │ Gold: 0
//   └─────────────────────┘
//
// === 金幣系統 ===
//   勇者 獲得 200 金幣 (總計: 200)
//   勇者 花費 150 金幣 (剩餘: 50)
//   勇者：金幣不足！(需要 100，擁有 50)
//
// === 致命傷害 ===
//   勇者 受到 9999 點傷害 (HP: 0/120)
//   勇者 倒下了！
//   勇者：無法治療
//   ┌─────────────────────┐
//   │ 勇者 (Lv.2)
//   │ HP:  0 / 120
//   │ ATK: 30
//   │ EXP: 30 / 200
//   │ Gold: 50
//   └─────────────────────┘
//
// === LeetCode 155. Min Stack ===
//   push -2, 0, -3 後 getMin() = -3
//   pop 之後 top()   = 0
//   pop 之後 getMin() = -2
//   push -5 後 getMin() = -5
//   再 pop 後 getMin()  = -2
//   → data_ 與 min_ 是 private，外部不可能只動其中一個
//
// === 日常實務：銀行帳戶（跨成員不變式）===
//   開戶餘額 = 100000 分
//   存入 25050 分後餘額 = 125050 分
//   提款 50000 分 → 成功，餘額 = 75050 分
//   提款 999999 分 → 失敗（餘額不足，屬正常分支，回傳 false 而非丟例外）
//   交易筆數 = 3
//   不變式檢查（餘額 == 紀錄總和 且 餘額 >= 0）：成立
//   存入 -100 分 → 捕捉到例外：存款金額必須為正
//   例外之後餘額不變 = 75050 分
//   → 與 Character 的「悄悄修正」不同：金額必須大聲失敗
