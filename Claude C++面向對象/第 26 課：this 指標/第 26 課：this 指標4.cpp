#include <iostream>
#include <string>
using namespace std;

class Player;  // 前向聲明

// 外部系統：接收 Player 的引用或指標
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
        // 在建構時把自身註冊到系統
        registerPlayer(this);     // 傳遞 this 指標
    }

    void attack(Player& target) {
        // 把自身的引用傳給日誌系統
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

int main() {
    cout << "=== 場景三：傳遞 this ===" << endl;

    cout << "\n--- 創建玩家 ---" << endl;
    Player warrior("戰士", 200);
    Player mage("法師", 120);

    cout << "\n--- 戰鬥 ---" << endl;
    warrior.attack(mage);
    mage.attack(warrior);

    return 0;
}
