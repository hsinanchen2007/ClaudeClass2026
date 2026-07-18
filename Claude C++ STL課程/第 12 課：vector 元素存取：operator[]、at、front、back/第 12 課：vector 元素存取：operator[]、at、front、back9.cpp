#include <vector>
#include <iostream>
#include <optional>

template <typename T>
std::optional<T> safe_get(const std::vector<T>& v, size_t index) {
    if (index < v.size()) {
        return v[index];
    }
    return std::nullopt;
}

int main() {
    std::vector<int> v = {10, 20, 30};
    
    // 安全存取，不會拋例外也不會未定義行為
    if (auto val = safe_get(v, 1)) {
        std::cout << "v[1] = " << *val << std::endl;  // 20
    }
    
    if (auto val = safe_get(v, 10)) {
        std::cout << "v[10] = " << *val << std::endl;
    } else {
        std::cout << "索引 10 無效" << std::endl;
    }
    
    return 0;
}
