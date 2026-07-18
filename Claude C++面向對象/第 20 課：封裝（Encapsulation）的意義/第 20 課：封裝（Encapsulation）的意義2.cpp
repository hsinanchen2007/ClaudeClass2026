#include <iostream>
#include <string>
#include <algorithm>   // std::clamp (C++17)
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

    return 0;
}
