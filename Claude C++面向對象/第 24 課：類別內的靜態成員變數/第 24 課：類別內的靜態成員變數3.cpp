#include <iostream>
#include <string>
using namespace std;

class Enemy {
public:
    // C++17 inline static：直接在類別內定義並初始化
    // 靜態成員變數屬於類別本身，而不是任何特定的對象
    inline static int totalKilled = 0;
    inline static int totalSpawned = 0;

private:
    string name_;
    bool alive_;

public:
    Enemy(const string& name) : name_(name), alive_(true) {
        totalSpawned++;
    }

    void kill() {
        if (alive_) {
            alive_ = false;
            totalKilled++;
            cout << "  " << name_ << " 被擊殺！" << endl;
        }
    }

    const string& getName() const { return name_; }
};

int main() {
    cout << "=== 靜態成員的訪問方式 ===" << endl;

    Enemy e1("哥布林");
    Enemy e2("骷髏兵");
    Enemy e3("狼人");

    e1.kill();
    e3.kill();

    cout << "\n--- 方式 1：通過類別名訪問（推薦）---" << endl;
    cout << "  生成數：" << Enemy::totalSpawned << endl;
    cout << "  擊殺數：" << Enemy::totalKilled << endl;

    cout << "\n--- 方式 2：通過對象訪問（不推薦）---" << endl;
    cout << "  生成數：" << e1.totalSpawned << endl;
    cout << "  擊殺數：" << e2.totalKilled << endl;

    // 方式 2 的問題：看起來像是對象自己的數據
    // 但實際上 e1.totalSpawned 和 e2.totalSpawned 是同一個變數
    cout << "\n--- 驗證：同一個變數 ---" << endl;
    cout << "  e1.totalKilled = " << e1.totalKilled << endl;
    cout << "  e2.totalKilled = " << e2.totalKilled << endl;
    cout << "  e3.totalKilled = " << e3.totalKilled << endl;
    cout << "  都是同一個值！" << endl;

    return 0;
}
