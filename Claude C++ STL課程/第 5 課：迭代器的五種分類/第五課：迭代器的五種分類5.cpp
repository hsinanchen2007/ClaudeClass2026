#include <iostream>
#include <forward_list>
#include <unordered_set>
 
int main() {
    // forward_list 的迭代器是 Forward Iterator
    std::cout << "=== forward_list ===" << std::endl;
    std::forward_list<int> flist = {10, 20, 30, 40, 50};
    
    // 保存一個迭代器
    auto saved = flist.begin();
    ++saved;  // 指向 20
    
    // 先遍歷一次
    std::cout << "第一次遍歷: ";
    for (auto it = flist.begin(); it != flist.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
    
    // 可以再遍歷一次（Input Iterator 不行）
    std::cout << "第二次遍歷: ";
    for (auto it = flist.begin(); it != flist.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
    
    // 之前保存的迭代器還有效
    std::cout << "保存的迭代器指向: " << *saved << std::endl;
    
    // 但不能後退
    // --saved;  // 編譯錯誤！
    
    // 再利用保存的迭代器繼續遍歷
    std::cout << "從保存的迭代器繼續遍歷: ";
    for (auto it = saved; it != flist.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    return 0;
}
 