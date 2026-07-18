#include <iostream>
#include <string>
using namespace std;

// ===== 濫用 mutable 的反面教材 =====
class BadExample {
private:
    string name_;
    mutable int hp_;        // ❌ 把核心數據設為 mutable
    mutable int gold_;      // ❌ 把核心數據設為 mutable

public:
    BadExample(const string& name, int hp, int gold)
        : name_(name), hp_(hp), gold_(gold) {}

    // 這些操作明明在修改對象狀態，卻偽裝成 const！
    void takeDamage(int dmg) const {   // ❌ 不應該是 const
        hp_ -= dmg;
        cout << "  " << name_ << " 受傷 HP:" << hp_ << endl;
    }

    void spendGold(int amount) const { // ❌ 不應該是 const
        gold_ -= amount;
        cout << "  花費 " << amount << " 金幣" << endl;
    }

    void print() const {
        cout << "  " << name_ << " HP:" << hp_
             << " Gold:" << gold_ << endl;
    }
};

// ===== 正確的設計 =====
class GoodExample {
private:
    string name_;
    int hp_;              // 核心數據：不用 mutable
    int gold_;            // 核心數據：不用 mutable
    mutable int viewCount_;  // ✅ 只有輔助數據用 mutable

public:
    GoodExample(const string& name, int hp, int gold)
        : name_(name), hp_(hp), gold_(gold), viewCount_(0) {}

    // 修改狀態的函數：不是 const
    void takeDamage(int dmg) {
        hp_ = max(0, hp_ - dmg);
        cout << "  " << name_ << " 受傷 HP:" << hp_ << endl;
    }

    void spendGold(int amount) {
        if (amount <= gold_) {
            gold_ -= amount;
            cout << "  花費 " << amount << " 金幣" << endl;
        }
    }

    // 只讀函數：是 const，只有 viewCount_ 用 mutable
    void print() const {
        viewCount_++;   // ✅ 合理的 mutable 用途
        cout << "  " << name_ << " HP:" << hp_
             << " Gold:" << gold_
             << " (查看次數:" << viewCount_ << ")" << endl;
    }
};

int main() {
    cout << "=== mutable 濫用 vs 正確使用 ===" << endl;

    // 濫用的後果：const 失去意義
    cout << "\n--- 濫用 mutable ---" << endl;
    const BadExample bad("壞設計", 100, 500);
    bad.takeDamage(30);     // const 對象居然能受傷？！
    bad.spendGold(200);     // const 對象居然能花錢？！
    bad.print();
    cout << "  const 完全失去了保護作用！" << endl;

    // 正確的設計
    cout << "\n--- 正確使用 ---" << endl;
    const GoodExample good("好設計", 100, 500);
    // good.takeDamage(30);   // ❌ 編譯錯誤！正確攔截
    // good.spendGold(200);   // ❌ 編譯錯誤！正確攔截
    good.print();             // ✅ 只有查看次數變化
    good.print();

    return 0;
}
