#include <iostream>
#include <string>
using namespace std;

// ===== 沒有封裝的角色（全部 public）=====
// 這個結構體沒有任何封裝，所有成員都是 public 的，外部程式可以隨意修改它們，導致各種災難。
struct CharacterBad {
    string name;
    int hp;         // 生命值
    int maxHp;      // 最大生命值
    int attack;     // 攻擊力
    int level;      // 等級
    int exp;        // 經驗值
    int gold;       // 金幣
};

int main() {
    cout << "=== 沒有封裝的災難 ===" << endl;

    CharacterBad hero;
    hero.name = "勇者";
    hero.hp = 100;
    hero.maxHp = 100;
    hero.attack = 25;
    hero.level = 1;
    hero.exp = 0;
    hero.gold = 50;

    cout << hero.name << " HP:" << hero.hp << "/" << hero.maxHp << endl;

    // 災難 1：生命值可以超過最大值
    hero.hp = 99999;
    cout << "超量回血：HP = " << hero.hp << "/" << hero.maxHp << endl;

    // 災難 2：生命值可以是負數
    hero.hp = -500;
    cout << "負數 HP：HP = " << hero.hp << "/" << hero.maxHp << endl;

    // 災難 3：最大生命值可以被設成 0
    hero.maxHp = 0;
    cout << "最大HP歸零：HP = " << hero.hp << "/" << hero.maxHp << endl;

    // 災難 4：攻擊力可以隨便改
    hero.attack = 999999;
    cout << "作弊攻擊力：" << hero.attack << endl;

    // 災難 5：等級與經驗不一致
    hero.level = 99;
    hero.exp = 0;    // 99 級但經驗值是 0？
    cout << "等級 " << hero.level << "，經驗 " << hero.exp << endl;

    // 災難 6：金幣可以無限刷
    hero.gold = 2147483647;
    cout << "無限金幣：" << hero.gold << endl;

    cout << "\n所有數據都被任意篡改，毫無防護！" << endl;

    return 0;
}
