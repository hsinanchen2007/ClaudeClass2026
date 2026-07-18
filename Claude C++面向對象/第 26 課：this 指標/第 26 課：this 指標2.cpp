#include <iostream>
#include <string>
using namespace std;

class Weapon {
private:
    string name;
    int damage;
    int durability;

public:
    // 參數名和成員名完全相同！
    Weapon(const string& name, int damage, int durability) {
        // 不用 this 的話，name = name 是自我賦值（參數給參數）
        // 必須用 this-> 區分：
        this->name = name;
        this->damage = damage;
        this->durability = durability;
    }

    // setter 也常遇到同名問題
    void setDamage(int damage) {
        if (damage < 0) damage = 0;   // 這裡的 damage 是參數
        this->damage = damage;         // this->damage 是成員
    }

    void print() const {
        cout << "  " << name << " (傷害:" << damage
             << " 耐久:" << durability << ")" << endl;
    }
};

int main() {
    cout << "=== 場景一：同名消歧 ===" << endl;

    Weapon sword("鐵劍", 25, 100);
    sword.print();

    sword.setDamage(40);
    sword.print();

    return 0;
}
