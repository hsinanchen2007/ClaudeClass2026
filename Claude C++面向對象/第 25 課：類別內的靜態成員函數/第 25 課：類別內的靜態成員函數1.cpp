#include <iostream>
#include <string>
using namespace std;

class Bullet {
private:
    int damage_;
    string type_;

    // 靜態成員變數, 所有子彈共享同一份統計數據
    inline static int totalFired_ = 0;
    inline static int totalHit_ = 0;

public:
    Bullet(const string& type, int dmg)
        : type_(type), damage_(dmg)
    {
    }

    // 非靜態函數：有 this，可以訪問一切
    void fire() {
        totalFired_++;
        cout << "  發射 " << type_ << "（傷害:" << damage_ << "）" << endl;
    }

    void hit() {
        totalHit_++;
        cout << "  命中！" << type_ << " 造成 " << damage_ << " 傷害" << endl;
    }

    // ====== 靜態成員函數：沒有 this ======
    // 靜態函數只能訪問靜態成員（totalFired_ 和 totalHit_），不能訪問 damage_ 和 type_
    // 靜態函數也不能調用非靜態函數（fire() 和 hit()），因為它們需要 this 指針
    static int getTotalFired() { return totalFired_; }
    static int getTotalHit() { return totalHit_; }

    static double getHitRate() {
        if (totalFired_ == 0) return 0.0;
        return static_cast<double>(totalHit_) / totalFired_ * 100.0;
    }

    static void printStats() {
        cout << "  發射：" << totalFired_
             << "  命中：" << totalHit_
             << "  命中率：" << getHitRate() << "%" << endl;

        // 以下會編譯錯誤！靜態函數沒有 this
        // cout << damage_;    // ❌ 不能訪問非靜態成員
        // cout << type_;      // ❌ 不能訪問非靜態成員
        // fire();             // ❌ 不能調用非靜態函數
    }

    static void resetStats() {
        totalFired_ = 0;
        totalHit_ = 0;
        cout << "  統計數據已重置" << endl;
    }
};

int main() {
    cout << "=== 靜態成員函數基礎 ===" << endl;

    Bullet normal("普通彈", 10);
    Bullet fire("火焰彈", 25);

    // 射擊
    normal.fire();
    normal.fire();
    fire.fire();
    normal.hit();
    fire.hit();
    fire.fire();
    fire.hit();

    // 通過類別名調用靜態函數——不需要對象！
    cout << "\n--- 戰鬥統計（類別名調用）---" << endl;
    Bullet::printStats();

    // 也可以通過對象調用（但不推薦）
    cout << "\n--- 通過對象調用（不推薦）---" << endl;
    normal.printStats();   // 和 Bullet::printStats() 完全一樣

    // 重置
    cout << "\n--- 重置 ---" << endl;
    Bullet::resetStats();
    Bullet::printStats();

    return 0;
}
