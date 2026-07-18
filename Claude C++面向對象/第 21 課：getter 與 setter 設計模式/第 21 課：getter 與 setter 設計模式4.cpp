#include <iostream>
#include <string>
using namespace std;

class Enemy {
private:
    string name_;
    int hp_;
    int maxHp_;
    int attackPower_;
    int internalId_;        // 內部 ID，外部完全不需要知道
    int aiState_;           // AI 狀態，純粹的內部邏輯

public:
    Enemy(const string& name, int maxHp, int atk)
        : name_(name), hp_(maxHp), maxHp_(maxHp)
        , attackPower_(atk)
        , internalId_(rand())  // 內部使用
        , aiState_(0)          // 內部使用
    {
    }

    // ===== 有 getter，沒有 setter =====
    // 外部需要讀取，但不能直接修改
    const string& getName() const { return name_; }
    int getHp() const { return hp_; }
    int getMaxHp() const { return maxHp_; }

    // ===== 沒有 getter，也沒有 setter =====
    // internalId_ — 外部完全不需要知道
    // aiState_    — 純粹的內部邏輯

    // ===== 用「行為」取代 setter =====
    // 不提供 setHp()，而是提供有意義的行為：
    void takeDamage(int damage) {
        if (damage <= 0) return;
        hp_ = max(0, hp_ - damage);
        cout << "  " << name_ << " 受到 " << damage
             << " 傷害 (HP:" << hp_ << "/" << maxHp_ << ")" << endl;

        if (hp_ == 0) {
            cout << "  " << name_ << " 被擊敗！" << endl;
        }
    }

    // 不提供 setAttackPower()，而是：
    void enrage() {  // 暴怒：攻擊力翻倍
        attackPower_ *= 2;
        cout << "  " << name_ << " 進入暴怒狀態！ATK="
             << attackPower_ << endl;
    }

    int attack() {
        aiState_++;   // 內部狀態更新，外部不知道
        cout << "  " << name_ << " 發動攻擊！(ATK:"
             << attackPower_ << ")" << endl;
        return attackPower_;
    }

    bool isAlive() const { return hp_ > 0; }
};

int main() {
    cout << "=== 行為取代 setter ===" << endl;

    Enemy goblin("哥布林", 50, 15);
    cout << "  " << goblin.getName() << " HP:" << goblin.getHp()
         << "/" << goblin.getMaxHp() << endl;

    // 不是 goblin.setHp(30)，而是：
    goblin.takeDamage(20);

    // 不是 goblin.setAttackPower(30)，而是：
    goblin.enrage();

    // 正常攻擊
    int dmg = goblin.attack();
    cout << "  造成了 " << dmg << " 點傷害" << endl;

    // 以下都不可能做到（封裝保護）：
    // goblin.hp_ = 9999;          // 編譯錯誤
    // goblin.internalId_;         // 編譯錯誤
    // goblin.aiState_ = -1;       // 編譯錯誤

    return 0;
}
