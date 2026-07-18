#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

struct Person {
    std::string name;
    int age;
};

int main() {
    // 基本排序, 這裡將 vec 中的元素從小到大排序
    std::cout << "=== sort ===" << std::endl;
    std::vector<int> vec = {5, 2, 8, 1, 9, 3, 7};
    std::sort(vec.begin(), vec.end());
    std::cout << "升序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // 自訂比較函數, 這裡將 vec 中的元素從大到小排序
    std::cout << "\n=== sort 自訂比較 ===" << std::endl;
    std::sort(vec.begin(), vec.end(), std::greater<int>());
    std::cout << "降序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // stable_sort：保持相等元素的原始順序, 這裡將 people 按年齡排序，相同年齡的保持原順序
    std::cout << "\n=== stable_sort ===" << std::endl;
    std::vector<Person> people = {
        {"Alice", 25},
        {"Bob", 30},
        {"Charlie", 25},
        {"Diana", 30}
    };
    
    // 按年齡排序，相同年齡的保持原順序, 這裡使用 lambda 表達式作為比較函數
    std::stable_sort(people.begin(), people.end(),
        [](const Person& a, const Person& b) {
            return a.age < b.age;
        });
    
    std::cout << "按年齡 stable_sort:" << std::endl;
    for (const auto& p : people) {
        std::cout << "  " << p.name << ": " << p.age << std::endl;
    }
    
    return 0;
}
