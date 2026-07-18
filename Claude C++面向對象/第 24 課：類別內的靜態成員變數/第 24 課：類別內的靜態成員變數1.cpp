#include <iostream>
#include <string>
using namespace std;

class Soldier {
private:
    string name_;
    int id_;

    // ====== 靜態成員變數：聲明 ======
    // 靜態成員變數屬於類別本身，而不是任何特定的對象
    // 用於跟蹤所有士兵的總數和分配唯一 ID
    static int totalCount_;     // 所有士兵的總數
    static int nextId_;         // 下一個可用的 ID

public:
    Soldier(const string& name)
        : name_(name), id_(nextId_++)
    {
        totalCount_++;
        cout << "  [入伍] " << name_ << " (ID:" << id_
             << " 總人數:" << totalCount_ << ")" << endl;
    }

    ~Soldier() {
        totalCount_--;
        cout << "  [退役] " << name_ << " (ID:" << id_
             << " 總人數:" << totalCount_ << ")" << endl;
    }

    void report() const {
        cout << "  士兵 " << name_ << " (ID:" << id_ << ") 報到！" << endl;
    }

    // 靜態成員可以通過普通成員函數訪問
    // 顯示目前總人數
    void showTotal() const {
        cout << "  目前總人數：" << totalCount_ << endl;
    }
};

// ====== 靜態成員變數：定義與初始化（類別外）======
// 靜態成員變數必須在類別外定義和初始化
// 這裡我們將 totalCount_ 初始化為 0，nextId_ 初始化為 1001
// 注意：這裡的初始化是針對整個類別的，而不是任何特定的對象
int Soldier::totalCount_ = 0;
int Soldier::nextId_ = 1001;

int main() {
    cout << "=== 靜態成員變數基礎 ===" << endl;

    cout << "\n--- 創建士兵 ---" << endl;
    Soldier s1("阿強");
    Soldier s2("阿明");
    Soldier s3("阿華");

    cout << "\n--- 報到 ---" << endl;
    s1.report();
    s2.report();
    s3.report();
    s1.showTotal();

    cout << "\n--- 作用域結束，逆序解構 ---" << endl;
    // s3、s2、s1 依次解構

    return 0;
}
