#include <iostream>
#include <string>
using namespace std;

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

    // ====== const 函數群 ======
    const string& getName() const { return name_; }
    int getHp() const { return hp_; }
    int getMaxHp() const { return maxHp_; }
    int getLevel() const { return level_; }

    double getHpPercent() const {
        // const 函數可以調用其他 const 函數 ✅
        return (static_cast<double>(getHp()) / getMaxHp()) * 100.0;
    }

    string getStatusText() const {
        // 可以調用 getName(), getHpPercent() — 都是 const ✅
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
        // const 函數可以調用其他 const 函數
        cout << "  " << getStatusText() << endl;
        cout << "  HP: " << getHp() << "/" << getMaxHp()
             << " (" << getHpPercent() << "%)" << endl;

        // 不能調用非 const 函數：
        // takeDamage(10);  // ❌ 編譯錯誤！
    }

    // ====== 非 const 函數 ======
    void takeDamage(int dmg) {
        hp_ = max(0, hp_ - dmg);
        // 非 const 函數可以調用 const 函數 ✅
        cout << "  " << getName() << " 受傷！" << getStatusText() << endl;
    }
};

int main() {
    cout << "=== const 函數調用鏈 ===" << endl;

    GameCharacter hero("戰士", 200, 5);

    cout << "\n--- 初始狀態 ---" << endl;
    hero.printFullStatus();

    cout << "\n--- 受傷後 ---" << endl;
    hero.takeDamage(120);
    hero.printFullStatus();

    hero.takeDamage(60);
    hero.printFullStatus();

    return 0;
}
