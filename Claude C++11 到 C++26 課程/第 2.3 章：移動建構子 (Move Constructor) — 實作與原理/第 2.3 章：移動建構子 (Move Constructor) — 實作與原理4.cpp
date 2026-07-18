#include <iostream>
#include <string>
#include <type_traits>

// Case 1：什麼都沒寫 → 自動生成移動建構子
class AutoMove {
    std::string name_;
    int value_;
};

// Case 2：寫了解構子 → 不會自動生成移動建構子
class NoAutoMove {
    std::string name_;
    int value_;
public:
    ~NoAutoMove() {}  // 有自訂解構子
};

// Case 3：明確要求生成
class ExplicitMove {
    std::string name_;
    int value_;
public:
    ~ExplicitMove() {}
    ExplicitMove(ExplicitMove&&) = default;  // 明確要求生成
    ExplicitMove(const ExplicitMove&) = default;
};

int main() {
    std::cout << std::boolalpha;

    std::cout << "AutoMove 可移動建構？ "
              << std::is_move_constructible<AutoMove>::value << "\n";

    std::cout << "NoAutoMove 可移動建構？ "
              << std::is_move_constructible<NoAutoMove>::value << "\n";
    // ↑ 注意：這仍然是 true！因為 const T& 可以接收右值
    //   但實際上走的是複製，不是真正的移動

    std::cout << "NoAutoMove 有 nothrow 移動建構？ "
              << std::is_nothrow_move_constructible<NoAutoMove>::value << "\n";
    // ↑ 這才能看出真相：沒有真正的移動建構子

    std::cout << "ExplicitMove 有 nothrow 移動建構？ "
              << std::is_nothrow_move_constructible<ExplicitMove>::value << "\n";

    return 0;
}
