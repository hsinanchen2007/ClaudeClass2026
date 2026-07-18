#include <iostream>
#include <string>
using namespace std;

class Circle {
private:
    double radius_;

    // 快取相關——mutable 因為它們只是效能優化，不影響邏輯狀態
    // 這裡用 mutable 是因為即使在 const 函數中，我們也想更新快取狀態
    // areaCached_ 和 circumCached_ 用來記錄快取是否有效
    // cachedArea_ 和 cachedCircum_ 存儲計算結果
    mutable bool areaCached_;
    mutable double cachedArea_;
    mutable bool circumCached_;
    mutable double cachedCircum_;

    static constexpr double PI = 3.14159265358979;

public:
    Circle(double r)
        : radius_(r > 0 ? r : 1.0)
        , areaCached_(false), cachedArea_(0)
        , circumCached_(false), cachedCircum_(0)
    {
    }

    // getter：邏輯上只讀，但會更新快取
    // const 函數：允許在 const 對象上調用，並且可以修改 mutable 成員
    // 這裡的 getArea() 和 getCircumference() 是 const 的，因為它們不修改 Circle 的邏輯狀態（半徑不變），但它們會修改快取狀態，這就是 mutable 的典型用法
    // const 函數：邏輯上只讀, 但可以修改快取狀態
    double getArea() const {
        if (!areaCached_) {
            cout << "    [計算面積...]" << endl;
            cachedArea_ = PI * radius_ * radius_;   // ✅ mutable 可修改
            areaCached_ = true;                      // ✅ mutable 可修改
        } else {
            cout << "    [使用快取]" << endl;
        }
        return cachedArea_;
    }

    double getCircumference() const {
        if (!circumCached_) {
            cout << "    [計算周長...]" << endl;
            cachedCircum_ = 2 * PI * radius_;
            circumCached_ = true;
        } else {
            cout << "    [使用快取]" << endl;
        }
        return cachedCircum_;
    }

    // setter：修改半徑時，快取失效
    void setRadius(double r) {
        if (r <= 0) return;
        radius_ = r;
        areaCached_ = false;     // 清除快取
        circumCached_ = false;   // 清除快取
        cout << "  半徑改為 " << radius_ << "，快取已清除" << endl;
    }

    double getRadius() const { return radius_; }
};

int main() {
    cout << "=== 快取範例 ===" << endl;

    Circle c(5.0);

    // 第一次調用——觸發計算
    cout << "\n--- 第一次查詢 ---" << endl;
    cout << "  面積 = " << c.getArea() << endl;
    cout << "  周長 = " << c.getCircumference() << endl;

    // 第二次調用——使用快取
    cout << "\n--- 第二次查詢（快取命中）---" << endl;
    cout << "  面積 = " << c.getArea() << endl;
    cout << "  周長 = " << c.getCircumference() << endl;

    // 修改半徑——快取失效
    cout << "\n--- 修改半徑 ---" << endl;
    c.setRadius(10.0);

    // 再次查詢——重新計算
    cout << "\n--- 修改後查詢 ---" << endl;
    cout << "  面積 = " << c.getArea() << endl;
    cout << "  周長 = " << c.getCircumference() << endl;

    // 再次查詢——又是快取
    cout << "\n--- 再次查詢（快取命中）---" << endl;
    cout << "  面積 = " << c.getArea() << endl;

    return 0;
}
