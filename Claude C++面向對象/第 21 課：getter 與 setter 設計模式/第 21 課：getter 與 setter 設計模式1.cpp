#include <iostream>
#include <string>
using namespace std;

class Player {
private:
    string name_;
    int hp_;
    int maxHp_;
    int level_;

public:
    Player(const string& name, int maxHp)
        : name_(name), hp_(maxHp), maxHp_(maxHp), level_(1)
    {
    }

    // ====== Getter：返回數據的只讀訪問 ======
    // 注意：getter 通常是 const 成員函數，保證不修改對象狀態

    // 基本型別的 getter：返回值的拷貝
    int getHp() const { return hp_; }
    int getMaxHp() const { return maxHp_; }
    int getLevel() const { return level_; }

    // 字串的 getter：返回 const 引用（避免拷貝）
    const string& getName() const { return name_; }

    // ====== Setter：帶驗證的修改 ======
    // setter 允許修改內部狀態，但通常會帶有驗證邏輯，確保對象保持有效狀態

    void setHp(int newHp) {
        // 驗證：維護不變量
        if (newHp < 0) newHp = 0;
        if (newHp > maxHp_) newHp = maxHp_;
        hp_ = newHp;
    }

    void setName(const string& newName) {
        if (newName.empty()) {
            cout << "  名字不能為空！" << endl;
            return;
        }
        name_ = newName;
    }

    // 注意：level_ 沒有 setter！
    // 等級只能通過 gainExp() 等遊戲邏輯改變，不開放直接設定

    void printStatus() const {
        cout << "  " << name_ << " Lv." << level_
             << " HP:" << hp_ << "/" << maxHp_ << endl;
    }
};

int main() {
    cout << "=== Getter 與 Setter 基本用法 ===" << endl;

    Player hero("勇者", 100);
    hero.printStatus();

    // 使用 getter 讀取
    cout << "\n--- Getter ---" << endl;
    cout << "  名字：" << hero.getName() << endl;
    cout << "  HP：" << hero.getHp() << endl;
    cout << "  等級：" << hero.getLevel() << endl;

    // 使用 setter 修改（帶驗證）
    cout << "\n--- Setter（正常）---" << endl;
    hero.setHp(60);
    hero.printStatus();

    // setter 的保護作用
    cout << "\n--- Setter（異常值被攔截）---" << endl;
    hero.setHp(-500);       // 會被修正為 0
    hero.printStatus();

    hero.setHp(99999);      // 會被修正為 maxHp
    hero.printStatus();

    hero.setName("");        // 空名字被拒絕
    hero.setName("英雄");
    hero.printStatus();

    // hero.setLevel(99);   // 編譯錯誤！沒有 setLevel

    return 0;
}
