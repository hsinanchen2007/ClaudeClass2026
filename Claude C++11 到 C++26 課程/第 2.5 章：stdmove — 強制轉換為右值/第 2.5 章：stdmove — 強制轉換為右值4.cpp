#include <memory>
#include <iostream>

std::unique_ptr<int> create() {
    return std::make_unique<int>(42);
}

void take_ownership(std::unique_ptr<int> ptr) {
    std::cout << *ptr << "\n";
}

int main() {
    auto p = create();
    // take_ownership(p);            // 錯誤！unique_ptr 不可複製
    take_ownership(std::move(p));    // OK：明確轉移所有權
    // p 現在是 nullptr

    return 0;
}
