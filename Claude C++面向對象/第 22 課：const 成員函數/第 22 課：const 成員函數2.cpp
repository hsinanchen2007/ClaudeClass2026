#include <iostream>
#include <string>
using namespace std;

class Shield {
private:
    string name_;
    int defense_;
    int durability_;

public:
    Shield(const string& name, int def, int dur)
        : name_(name), defense_(def), durability_(dur)
    {
    }

    // const 成員函數
    // const 成員函數承諾不修改對象的狀態（成員變量）
    // 這允許在 const 對象上調用這些函數，並且編譯器會強制執行這一承諾
    const string& getName() const { return name_; }
    int getDefense() const { return defense_; }
    int getDurability() const { return durability_; }

    void printInfo() const {
        cout << "  " << name_ << " [防禦:" << defense_
             << " 耐久:" << durability_ << "]" << endl;
    }

    // 非 const 成員函數
    // 非 const 成員函數可以修改對象的狀態，這些函數不能在 const 對象上調用
    // 這裡的 takeDamage() 和 repair() 函數會修改 durability_，所以不能是 const
    void takeDamage(int dmg) {
        durability_ -= dmg;
        if (durability_ < 0) durability_ = 0;
        cout << "  " << name_ << " 耐久 -" << dmg
             << " (剩餘:" << durability_ << ")" << endl;
    }

    void repair() {
        durability_ = 100;
        cout << "  " << name_ << " 修復完成" << endl;
    }
};

// 接收 const 引用的函數——模擬「只看不碰」
// 這個函數接受一個 const Shield&，表示它只能「看」這個盾牌，但不能修改它
void inspectShield(const Shield& s) {
    cout << "\n--- 檢查盾牌（const 引用）---" << endl;

    // ✅ 可以調用 const 成員函數
    s.printInfo();
    cout << "  防禦力：" << s.getDefense() << endl;
    cout << "  耐久度：" << s.getDurability() << endl;

    // ❌ 不能調用非 const 成員函數
    // s.takeDamage(10);   // 編譯錯誤！
    // s.repair();          // 編譯錯誤！
}

int main() {
    cout << "=== const 對象的限制 ===" << endl;

    // 非 const 對象：所有函數都能調用
    // ✅ 非 const 對象可以調用 const 和非 const 成員函數
    cout << "\n--- 非 const 對象 ---" << endl;
    Shield shield("鐵盾", 40, 100);
    shield.printInfo();        // ✅ const 函數
    shield.takeDamage(20);     // ✅ 非 const 函數
    shield.repair();           // ✅ 非 const 函數

    // const 對象：只能調用 const 函數
    // ✅ const 對象只能調用 const 成員函數，不能調用非 const 成員函數
    cout << "\n--- const 對象 ---" << endl;
    const Shield legendaryShield("傳說之盾", 100, 999);
    legendaryShield.printInfo();       // ✅ const 函數
    legendaryShield.getDefense();      // ✅ const 函數
    // legendaryShield.takeDamage(10); // ❌ 編譯錯誤！
    // legendaryShield.repair();       // ❌ 編譯錯誤！

    // const 引用參數
    // ✅ const 引用允許我們在函數內部「只看不碰」對象，這是非常常見的用法  
    // 這裡我們傳入 legendaryShield 的 const 引用，函數內部只能調用 const 成員函數
    // 這裡的 inspectShield() 函數接受一個 const Shield&，表示它只能「看」這個盾牌，但不能修改它
    inspectShield(shield);

    return 0;
}
