#include <iostream>
#include <string>
using namespace std;

class Monster {
private:
    string name_;
    int hp_;
    int attack_;
    mutable int inspectCount_;   // mutable：const 函數也能改它
                                 // 這裡用來記錄被查看了多少次, 邏輯上不影響怪物狀態

public:
    Monster(const string& name, int hp, int atk)
        : name_(name), hp_(hp), attack_(atk), inspectCount_(0)
    {
    }

    // const 函數——邏輯上只讀, 但可以修改 inspectCount_
    void printInfo() const {
        inspectCount_++;   // ✅ 可以修改！因為是 mutable
        cout << "  " << name_ << " [HP:" << hp_ << " ATK:" << attack_
             << "] (被查看了 " << inspectCount_ << " 次)" << endl;
    }

    const string& getName() const { return name_; }
    int getHp() const { return hp_; }

    // 查看被檢查了幾次, 這個函數也是 const 的，因為它不修改怪物的狀態
    int getInspectCount() const { return inspectCount_; }

    // 非 const：實際修改怪物狀態, 這裡不允許在 const 對象上調用
    void takeDamage(int dmg) {
        hp_ = max(0, hp_ - dmg);
    }
};

int main() {
    cout << "=== mutable 基本用法 ===" << endl;

    const Monster dragon("火龍", 500, 60);  // const 對象！

    // 可以調用 const 函數
    dragon.printInfo();
    dragon.printInfo();
    dragon.printInfo();

    cout << "  總共被查看：" << dragon.getInspectCount() << " 次" << endl;

    // dragon.takeDamage(10);  // ❌ 編譯錯誤！const 對象

    return 0;
}
