#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Inventory {
private:
    string ownerName_;
    vector<string> items_;
    int gold_;

public:
    Inventory(const string& owner, int gold)
        : ownerName_(owner), gold_(gold) {}

    void addItem(const string& item) {
        items_.push_back(item);
    }

    // ====== 返回方式比較 ======

    // 方式 1：返回值（拷貝）— 適合基本型別
    // 注意：對於基本型別，返回值會自動拷貝，不會影響原物件
    int getGold() const { return gold_; }

    // 方式 2：返回 const 引用 — 適合大型物件，避免拷貝
    // 注意：返回 const 引用可以讓外部「看」但不能「改」，保護封裝
    const string& getOwnerName() const { return ownerName_; }

    // 方式 3：返回 const 引用 — 讓外部可以「看」但不能「改」
    // 注意：對於容器等大型物件，返回 const 引用可以避免不必要的拷貝，同時保護封裝
    const vector<string>& getItems() const { return items_; }

    // 方式 4（錯誤示範）：返回非 const 引用 — 破壞封裝！
    // 注意：這樣做會讓外部直接修改內部狀態，繞過所有驗證邏輯，極易導致錯誤！
    // vector<string>& getItemsDangerous() { return items_; }
    // ↑ 外部可以直接修改 items_，繞過所有驗證！
};

int main() {
    cout << "=== Getter 返回方式 ===" << endl;

    Inventory inv("勇者", 500);
    inv.addItem("鐵劍");
    inv.addItem("治療藥水");
    inv.addItem("火炬");

    // 方式 1：返回值（拷貝）
    int gold = inv.getGold();
    gold = 0;  // 修改的是拷貝，不影響原物件
    cout << "  修改拷貝後，實際金幣：" << inv.getGold() << endl;

    // 方式 2：返回 const 引用
    const string& name = inv.getOwnerName();
    cout << "  擁有者：" << name << endl;
    // name = "壞人";  // 編譯錯誤！const 引用不能修改

    // 方式 3：返回 const 引用（容器）
    const vector<string>& items = inv.getItems();
    cout << "  物品數量：" << items.size() << endl;
    for (const auto& item : items) {
        cout << "    - " << item << endl;
    }
    // items.push_back("偷加的");  // 編譯錯誤！const 引用

    return 0;
}
