#include <iostream>
#include <vector>
#include <string>

std::vector<std::string> generate_data() {
    std::vector<std::string> result;
    result.reserve(3);
    result.emplace_back("alpha");
    result.emplace_back("beta");
    result.emplace_back("gamma");
    return result;  // NRVO（具名回傳值優化）通常會消除拷貝
}

int main() {
    // 情況一：函數回傳值（通常 NRVO 會優化，不需要手動 move）
    std::vector<std::string> v1 = generate_data();

    // 情況二：明確轉移一個已存在的 vector
    std::vector<std::string> v2;
    v2 = std::move(v1);
    // v1 現在是「合法但未指定狀態」（通常是空的）

    std::cout << "v1.size() = " << v1.size() << std::endl;   // 0
    std::cout << "v2.size() = " << v2.size() << std::endl;   // 3

    for (const auto& s : v2) {
        std::cout << s << " ";
    }
    std::cout << std::endl;

    return 0;
}
