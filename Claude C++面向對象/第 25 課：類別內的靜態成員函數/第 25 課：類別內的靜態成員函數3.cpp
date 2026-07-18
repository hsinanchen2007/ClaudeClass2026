#include <iostream>
#include <string>
using namespace std;

class Potion {
private:
    string name_;
    int healAmount_;
    int price_;

    // 私有建構函數：外部不能直接 new
    // 只能透過工廠函數創建
    Potion(const string& name, int heal, int price)
        : name_(name), healAmount_(heal), price_(price)
    {
    }

public:
    // ====== 靜態工廠函數：提供命名的創建方式 ======
    // 每個工廠函數都創建一種特定類型的藥水
    // 這些函數可以有清晰的名稱，讓使用者知道創建的是什麼類型的物件
    static Potion createSmall() {
        return Potion("小型藥水", 30, 50);
    }

    static Potion createMedium() {
        return Potion("中型藥水", 70, 120);
    }

    static Potion createLarge() {
        return Potion("大型藥水", 150, 300);
    }

    static Potion createCustom(const string& name, int heal, int price) {
        // 帶驗證的創建
        int safeHeal = (heal > 0 && heal <= 999) ? heal : 50;
        int safePrice = (price > 0 && price <= 9999) ? price : 100;
        return Potion(name, safeHeal, safePrice);
    }

    void printInfo() const {
        cout << "  " << name_ << " (回復:" << healAmount_
             << " 價格:" << price_ << " 金幣)" << endl;
    }
};

int main() {
    cout << "=== 工廠函數 ===" << endl;

    // 不能直接建構：
    // Potion p("藥水", 50, 100);  // ❌ 建構函數是 private

    // 透過靜態工廠函數創建
    cout << "\n--- 預設藥水 ---" << endl;
    Potion small = Potion::createSmall();
    Potion medium = Potion::createMedium();
    Potion large = Potion::createLarge();

    small.printInfo();
    medium.printInfo();
    large.printInfo();

    cout << "\n--- 自定義藥水 ---" << endl;
    Potion special = Potion::createCustom("秘製靈藥", 500, 2000);
    special.printInfo();

    return 0;
}
