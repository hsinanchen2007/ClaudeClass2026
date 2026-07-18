#include <iostream>
#include <string>
using namespace std;

class QuestLog {
private:
    string questName_;
    int difficulty_;

    // 延遲初始化：詳細描述只在需要時才生成
    // mutable：即使在 const 函數中也能修改它們，因為它們只是效能優化，不影響邏輯狀態
    // descriptionReady_ 用來記錄描述是否已經生成，detailedDescription_ 存儲生成的描述
    // 這裡用 mutable 是因為即使在 const 函數中，我們也想更新描述生成的狀態，這是延遲初始化的典型用法
    mutable bool descriptionReady_;
    mutable string detailedDescription_;

    // 模擬昂貴的描述生成
    void generateDescription() const {
        cout << "    [生成詳細描述... 耗時操作]" << endl;
        detailedDescription_ = "【" + questName_ + "】\n";
        detailedDescription_ += "    難度：";
        for (int i = 0; i < difficulty_; i++) {
            detailedDescription_ += "★";
        }
        detailedDescription_ += "\n";
        detailedDescription_ += "    這是一個";
        if (difficulty_ >= 4) detailedDescription_ += "極其危險的";
        else if (difficulty_ >= 3) detailedDescription_ += "具有挑戰性的";
        else if (difficulty_ >= 2) detailedDescription_ += "需要謹慎的";
        else detailedDescription_ += "簡單的";
        detailedDescription_ += "任務。冒險者需做好充分準備。";

        descriptionReady_ = true;
    }

public:
    QuestLog(const string& name, int diff)
        : questName_(name)
        , difficulty_(diff > 0 && diff <= 5 ? diff : 1)
        , descriptionReady_(false)
    {
        cout << "  [登記任務] " << questName_ << endl;
        // 注意：不在建構時生成描述，延遲到需要時
    }

    // 簡單查詢——不需要詳細描述
    const string& getName() const { return questName_; }
    int getDifficulty() const { return difficulty_; }

    // 需要詳細描述時才觸發初始化
    const string& getDescription() const {
        if (!descriptionReady_) {
            generateDescription();   // 延遲初始化
        }
        return detailedDescription_;
    }
};

int main() {
    cout << "=== 延遲初始化 ===" << endl;

    // 創建多個任務——都不生成描述
    cout << "\n--- 登記任務 ---" << endl;
    QuestLog quest1("討伐火龍", 5);
    QuestLog quest2("採集草藥", 1);
    QuestLog quest3("護送商隊", 3);

    // 只用簡單查詢——不觸發描述生成
    cout << "\n--- 簡單查詢（不觸發生成）---" << endl;
    cout << "  任務1：" << quest1.getName()
         << "（難度 " << quest1.getDifficulty() << "）" << endl;
    cout << "  任務2：" << quest2.getName()
         << "（難度 " << quest2.getDifficulty() << "）" << endl;

    // 只有查看詳細描述時才觸發生成
    cout << "\n--- 查看詳細描述（觸發生成）---" << endl;
    cout << quest1.getDescription() << endl;

    // 第二次查看——直接返回，不重複生成
    cout << "\n--- 再次查看（已生成）---" << endl;
    cout << quest1.getDescription() << endl;

    // quest2 和 quest3 的描述始終沒被生成——節省了資源
    cout << "\n--- quest2 和 quest3 從未生成描述，節省了資源 ---" << endl;

    return 0;
}
