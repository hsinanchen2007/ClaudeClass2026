#include <iostream>
#include <vector>

int main() {
    std::vector<bool> vb = {true, false, true};

    // 陷阱：auto 推導為 proxy，不是 bool
    auto val = vb[0];   // 型別是 vector<bool>::reference

    vb[0] = false;      // 修改原容器

    // val 是代理物件，它「連結」到 vb 的內部
    // 所以 val 也跟著變了！
    std::cout << "val = " << val << std::endl;  // 0（false），不是預期的 1（true）

    // 正確做法：明確宣告為 bool
    bool val2 = vb[1];  // 強制轉型為 bool，取得獨立的副本
    vb[1] = true;
    std::cout << "val2 = " << val2 << std::endl;    // 0（false），不受影響
    std::cout << "vb[1] = " << vb[1] << std::endl;  // 1（true），已修改

    return 0;
}
