#include <iostream>
#include <string>
using namespace std;

class MonsterEntry {
private:
    // ====== 邏輯狀態（不可變的圖鑑資料）======
    string name_;
    string element_;
    int baseHp_;
    int baseAttack_;
    int rarity_;          // 1~5 星

    // ====== 輔助狀態（mutable）======
    mutable int viewCount_;               // 被查看次數
    mutable bool detailGenerated_;        // 詳細資料是否已生成
    mutable string detailCache_;          // 詳細資料快取

    // 私有輔助：生成詳細資料
    void generateDetail() const {
        cout << "    [生成詳細資料...]" << endl;

        detailCache_ = "=== " + name_ + " ===\n";
        detailCache_ += "  屬性：" + element_ + "\n";
        detailCache_ += "  稀有度：";
        for (int i = 0; i < rarity_; i++) detailCache_ += "★";
        detailCache_ += "\n";
        detailCache_ += "  基礎 HP：" + to_string(baseHp_) + "\n";
        detailCache_ += "  基礎 ATK：" + to_string(baseAttack_) + "\n";

        // 添加弱點資訊
        detailCache_ += "  弱點：";
        if (element_ == "火") detailCache_ += "水";
        else if (element_ == "水") detailCache_ += "雷";
        else if (element_ == "雷") detailCache_ += "土";
        else if (element_ == "土") detailCache_ += "風";
        else if (element_ == "風") detailCache_ += "火";
        else detailCache_ += "無";
        detailCache_ += "\n";

        // 添加威脅評估
        int threat = baseHp_ / 100 + baseAttack_ / 10 + rarity_;
        detailCache_ += "  威脅指數：" + to_string(threat) + "\n";

        detailGenerated_ = true;
    }

public:
    MonsterEntry(const string& name, const string& elem,
                 int hp, int atk, int rare)
        : name_(name), element_(elem)
        , baseHp_(hp), baseAttack_(atk)
        , rarity_(rare > 0 && rare <= 5 ? rare : 1)
        , viewCount_(0)
        , detailGenerated_(false)
    {
    }

    // ====== 所有查詢函數都是 const ======

    // 簡要資訊——只讀，遞增查看次數
    void printBrief() const {
        viewCount_++;
        cout << "  ";
        for (int i = 0; i < rarity_; i++) cout << "★";
        cout << " " << name_ << " [" << element_ << "]"
             << " (查看:" << viewCount_ << ")" << endl;
    }

    // 詳細資訊——延遲生成 + 快取
    const string& getDetail() const {
        viewCount_++;
        if (!detailGenerated_) {
            generateDetail();   // 延遲初始化
        }
        return detailCache_;
    }

    // 其他 getter
    const string& getName() const { return name_; }
    int getViewCount() const { return viewCount_; }
    int getRarity() const { return rarity_; }
};

// 展示圖鑑——接收 const 引用
void showEncyclopedia(const MonsterEntry entries[], int count) {
    cout << "\n  ╔═══════════════════════════╗" << endl;
    cout << "  ║     怪 物 圖 鑑           ║" << endl;
    cout << "  ╚═══════════════════════════╝" << endl;

    for (int i = 0; i < count; i++) {
        entries[i].printBrief();   // const 函數 ✅
    }
}

// 查看特定怪物詳情——接收 const 引用
void showDetail(const MonsterEntry& entry) {
    cout << "\n" << entry.getDetail();  // 延遲生成 + 快取 ✅
}

int main() {
    cout << "============================================" << endl;
    cout << "   第 23 課：mutable 綜合範例" << endl;
    cout << "============================================" << endl;

    // 創建圖鑑條目
    const int COUNT = 4;
    MonsterEntry entries[COUNT] = {
        MonsterEntry("炎龍王", "火", 800, 70, 5),
        MonsterEntry("冰霜狼", "水", 400, 45, 3),
        MonsterEntry("雷電鷹", "雷", 300, 60, 4),
        MonsterEntry("泥土蟲", "土", 600, 20, 1)
    };

    // 瀏覽圖鑑——只觸發簡要資訊
    showEncyclopedia(entries, COUNT);

    // 查看炎龍王的詳細資料——第一次觸發生成
    cout << "\n--- 查看炎龍王詳情 ---";
    showDetail(entries[0]);

    // 再次查看——使用快取
    cout << "\n--- 再次查看炎龍王詳情 ---";
    showDetail(entries[0]);

    // 查看雷電鷹——觸發生成
    cout << "\n--- 查看雷電鷹詳情 ---";
    showDetail(entries[2]);

    // 查看各怪物被查看次數
    cout << "\n--- 查看統計 ---" << endl;
    for (int i = 0; i < COUNT; i++) {
        cout << "  " << entries[i].getName()
             << "：被查看 " << entries[i].getViewCount() << " 次" << endl;
    }

    // 注意：冰霜狼和泥土蟲的詳細描述從未被生成——節省資源
    cout << "\n  冰霜狼和泥土蟲的詳細描述從未生成，節省了資源！" << endl;

    return 0;
}
