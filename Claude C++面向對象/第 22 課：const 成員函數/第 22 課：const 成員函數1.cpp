#include <iostream>
#include <string>
using namespace std;

class Potion {
private:
    string name_;
    int healAmount_;
    int quantity_;

public:
    Potion(const string& name, int heal, int qty)
        : name_(name), healAmount_(heal), quantity_(qty)
    {
    }

    // ====== const 成員函數：承諾不修改對象 ======
    // const 成員函數承諾不修改對象的狀態（成員變量）
    // 這允許在 const 對象上調用這些函數，並且編譯器會強制執行這一承諾
    const string& getName() const { return name_; }
    int getHealAmount() const { return healAmount_; }
    int getQuantity() const { return quantity_; }

    void printInfo() const {
        cout << "  " << name_ << " (回復:" << healAmount_
             << " 數量:" << quantity_ << ")" << endl;

        // 以下會編譯錯誤！const 函數不能修改成員
        // quantity_ = 0;       // 錯誤！
        // name_ = "被篡改";    // 錯誤！
    }

    // ====== 非 const 成員函數：可以修改對象 ======
    // 非 const 成員函數可以修改對象的狀態，這些函數不能在 const 對象上調用
    // 這裡的 use() 函數會修改 quantity_，所以不能是 const
    bool use() {
        if (quantity_ <= 0) {
            cout << "  " << name_ << " 已用完！" << endl;
            return false;
        }
        quantity_--;
        cout << "  使用 " << name_ << "，回復 " << healAmount_
             << " HP (剩餘:" << quantity_ << ")" << endl;
        return true;
    }

    void restock(int amount) {
        if (amount > 0) {
            quantity_ += amount;
            cout << "  補貨 " << name_ << " +" << amount
                 << " (總計:" << quantity_ << ")" << endl;
        }
    }
};

int main() {
    cout << "=== const 成員函數基礎 ===" << endl;

    Potion potion("治療藥水", 50, 3);

    // const 函數：只讀操作
    cout << "\n--- const 函數（只讀）---" << endl;
    potion.printInfo();
    cout << "  名稱：" << potion.getName() << endl;
    cout << "  數量：" << potion.getQuantity() << endl;

    // 非 const 函數：修改操作
    cout << "\n--- 非 const 函數（修改）---" << endl;
    potion.use();
    potion.use();
    potion.printInfo();

    return 0;
}
