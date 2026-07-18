// lesson34_traits.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -o lesson34b lesson34_traits.cpp

#include <iostream>
#include <type_traits>
#include <string>
#include <memory>

class FullFive {
    int* m_data;
public:
    FullFive() : m_data(new int(0)) {}
    ~FullFive() { delete m_data; }
    FullFive(const FullFive& o) : m_data(new int(*o.m_data)) {}
    FullFive& operator=(const FullFive& o) {
        if (this != &o) { *m_data = *o.m_data; }
        return *this;
    }
    FullFive(FullFive&& o) noexcept : m_data(o.m_data) { o.m_data = nullptr; }
    FullFive& operator=(FullFive&& o) noexcept {
        if (this != &o) { delete m_data; m_data = o.m_data; o.m_data = nullptr; }
        return *this;
    }
};

class OnlyThree {
    int* m_data;
public:
    OnlyThree() : m_data(new int(0)) {}
    ~OnlyThree() { delete m_data; }
    OnlyThree(const OnlyThree& o) : m_data(new int(*o.m_data)) {}
    OnlyThree& operator=(const OnlyThree& o) {
        if (this != &o) { *m_data = *o.m_data; }
        return *this;
    }
    // 沒有移動操作！
};

class MoveOnly {
    std::unique_ptr<int> m_data;
public:
    MoveOnly() : m_data(std::make_unique<int>(0)) {}
    // unique_ptr 讓拷貝被隱式刪除，移動自動生成
};

template <typename T>
void checkTraits(const char* name) {
    std::cout << name << ":\n";
    std::cout << "  可解構？           " << std::is_destructible_v<T> << "\n";
    std::cout << "  可拷貝建構？       " << std::is_copy_constructible_v<T> << "\n";
    std::cout << "  可拷貝賦值？       " << std::is_copy_assignable_v<T> << "\n";
    std::cout << "  可移動建構？       " << std::is_move_constructible_v<T> << "\n";
    std::cout << "  nothrow 移動建構？ " << std::is_nothrow_move_constructible_v<T> << "\n";
    std::cout << "  可移動賦值？       " << std::is_move_assignable_v<T> << "\n";
    std::cout << "  nothrow 移動賦值？ " << std::is_nothrow_move_assignable_v<T> << "\n";
    std::cout << "\n";
}

int main() {
    std::cout << std::boolalpha;
    checkTraits<FullFive>("FullFive (五法則完整)");
    checkTraits<OnlyThree>("OnlyThree (只有三法則)");
    checkTraits<MoveOnly>("MoveOnly (只能移動)");
    checkTraits<std::string>("std::string (標準庫)");
    return 0;
}
