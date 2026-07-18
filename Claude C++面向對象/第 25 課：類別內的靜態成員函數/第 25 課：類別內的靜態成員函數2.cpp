#include <iostream>
#include <string>
using namespace std;

class Character {
private:
    string name_;               // 非靜態
    int hp_;                    // 非靜態
    inline static int count_ = 0;  // 靜態

public:
    Character(const string& name, int hp)
        : name_(name), hp_(hp)
    {
        count_++;
    }

    // ====== 非靜態函數：有 this，可以訪問一切 ======
    // 非靜態函數可以訪問非靜態成員（name_ 和 hp_）和靜態成員（count_）
    // 非靜態函數也可以調用其他非靜態函數（printInfo()），因為它們都有 this 指針
    void printInfo() const {
        cout << "  " << name_ << " HP:" << hp_     // ✅ 非靜態成員
             << " (共 " << count_ << " 人)" << endl; // ✅ 靜態成員也行
    }

    // ====== 靜態函數：沒有 this，只能訪問靜態 ======
    // 靜態函數只能訪問靜態成員，不能訪問非靜態成員，因為它沒有 this 指針
    // 靜態函數也不能調用非靜態函數，因為它們需要 this 指針
    static void printCount() {
        cout << "  角色總數：" << count_ << endl;  // ✅ 靜態成員

        // cout << name_;      // ❌ 哪個對象的 name_？
        // cout << hp_;        // ❌ 哪個對象的 hp_？
        // printInfo();        // ❌ 需要 this 才能調用
    }

    // 靜態函數可以接收對象參數來間接訪問非靜態成員
    // 這裡的 compare 是靜態函數，但它接受兩個 Character 對象作為參數
    // 因為 compare 是 Character 的成員函數，所以它可以訪問 Character 的 private 成員
    // 注意：這裡訪問的是「參數對象」的成員，不是通過 this，因為 compare 是靜態函數沒有 this 指針
    static void compare(const Character& a, const Character& b) {
        cout << "  比較：" << a.name_ << "(HP:" << a.hp_ << ") vs "
             << b.name_ << "(HP:" << b.hp_ << ")" << endl;

        // 注意：這裡訪問的是「參數對象」的成員，不是通過 this
        // 因為 compare 是 Character 的成員函數，可以訪問 private
        if (a.hp_ > b.hp_)
            cout << "  → " << a.name_ << " 更強" << endl;
        else if (a.hp_ < b.hp_)
            cout << "  → " << b.name_ << " 更強" << endl;
        else
            cout << "  → 不分上下" << endl;
    }
};

int main() {
    cout << "=== 訪問規則 ===" << endl;

    Character warrior("戰士", 200);
    Character mage("法師", 120);

    cout << "\n--- 非靜態函數（需要對象）---" << endl;
    warrior.printInfo();
    mage.printInfo();

    cout << "\n--- 靜態函數（不需要對象）---" << endl;
    Character::printCount();

    cout << "\n--- 靜態函數接收對象參數 ---" << endl;
    Character::compare(warrior, mage);

    return 0;
}
