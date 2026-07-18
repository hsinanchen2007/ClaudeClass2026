#include <iostream>
#include <string>
using namespace std;

class Player {
public:
    string name;
    int hp = 100;       // 生命值
    int attack = 10;    // 攻擊力

    void attackTarget(Player& target) {
        cout << name << " 攻擊了 " << target.name << "！" << endl;
        target.hp -= attack;
        cout << target.name << " 剩餘 HP: " << target.hp << endl;
    }

    void showStatus() {
        cout << "[" << name << "] HP: " << hp
             << " / 攻擊力: " << attack << endl;
    }

    bool isAlive() {
        return hp > 0;
    }
};

int main() {
    Player warrior;
    warrior.name = "戰士";
    warrior.hp = 120;
    warrior.attack = 15;

    Player mage;
    mage.name = "法師";
    mage.hp = 80;
    mage.attack = 25;

    cout << "===== 戰鬥開始 =====" << endl;
    warrior.showStatus();
    mage.showStatus();

    cout << "\n--- 第 1 回合 ---" << endl;
    warrior.attackTarget(mage);     // 戰士攻擊法師

    cout << "\n--- 第 2 回合 ---" << endl;
    mage.attackTarget(warrior);     // 法師攻擊戰士

    cout << "\n--- 第 3 回合 ---" << endl;
    warrior.attackTarget(mage);     // 戰士再攻擊法師

    cout << "\n===== 戰鬥結束 =====" << endl;
    warrior.showStatus();
    mage.showStatus();

    // 檢查存活狀態
    cout << "\n" << warrior.name << (warrior.isAlive() ? " 存活" : " 陣亡") << endl;
    cout << mage.name << (mage.isAlive() ? " 存活" : " 陣亡") << endl;

    return 0;
}
