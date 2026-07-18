#include <iostream>
#include <vector>
#include <utility>

class WithNoexcept {
public:
    int id;
    WithNoexcept(int i) : id(i) {}

    WithNoexcept(const WithNoexcept& o) : id(o.id) {
        std::cout << "  WithNoexcept [複製] id=" << id << "\n";
    }

    WithNoexcept(WithNoexcept&& o) noexcept : id(o.id) {
        o.id = -1;
        std::cout << "  WithNoexcept [移動] id=" << id << "\n";
    }
};

class WithoutNoexcept {
public:
    int id;
    WithoutNoexcept(int i) : id(i) {}

    WithoutNoexcept(const WithoutNoexcept& o) : id(o.id) {
        std::cout << "  WithoutNoexcept [複製] id=" << id << "\n";
    }

    // 注意：沒有 noexcept！
    WithoutNoexcept(WithoutNoexcept&& o) : id(o.id) {
        o.id = -1;
        std::cout << "  WithoutNoexcept [移動] id=" << id << "\n";
    }
};

int main() {
    std::cout << "=== 有 noexcept 的類別 ===\n";
    std::vector<WithNoexcept> v1;
    v1.reserve(2);  // 先保留 2 個空間
    v1.emplace_back(1);
    v1.emplace_back(2);
    std::cout << "-- 觸發擴容 --\n";
    v1.emplace_back(3);  // 超過容量，需要擴容搬移舊元素

    std::cout << "\n=== 沒有 noexcept 的類別 ===\n";
    std::vector<WithoutNoexcept> v2;
    v2.reserve(2);
    v2.emplace_back(1);
    v2.emplace_back(2);
    std::cout << "-- 觸發擴容 --\n";
    v2.emplace_back(3);  // 擴容時會退回用複製！

    return 0;
}
