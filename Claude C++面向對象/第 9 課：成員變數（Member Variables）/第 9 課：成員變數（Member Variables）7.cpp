#include <iostream>
#include <string>
using namespace std;

class GameCharacter {
public:
    string name = "未命名";
    int hp = 100;
    int mp = 50;
    double speed = 1.0;
    bool isAlive = true;

    void show() {
        cout << name << " | HP:" << hp << " MP:" << mp
             << " 速度:" << speed
             << " 狀態:" << (isAlive ? "存活" : "陣亡") << endl;
    }
};

int main() {
    GameCharacter c1;           // 全部使用預設值
    c1.show();

    GameCharacter c2;
    c2.name = "戰士";           // 覆蓋部分預設值
    c2.hp = 150;
    c2.show();

    return 0;
}
