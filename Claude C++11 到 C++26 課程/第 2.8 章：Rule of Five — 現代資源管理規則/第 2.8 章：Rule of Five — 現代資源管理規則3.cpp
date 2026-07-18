#include <iostream>
#include <string>
#include <vector>
#include <memory>

// Rule of Zero：讓成員自己管理資源
class User {
    std::string name_;                    // 自己知道怎麼複製和移動
    std::vector<int> scores_;             // 自己知道怎麼複製和移動
    std::unique_ptr<int[]> buffer_;       // 自己知道怎麼移動（不可複製）

    // 不需要寫任何一個！編譯器全部自動生成
    // 解構子：自動呼叫每個成員的解構子
    // 移動：自動逐成員移動
    // 複製：因為 unique_ptr 不可複製，所以整個類別不可複製
};

int main() {
    User a;
    // User b = a;           // 編譯錯誤：unique_ptr 不可複製
    User c = std::move(a);   // OK：逐成員移動

    return 0;
}
