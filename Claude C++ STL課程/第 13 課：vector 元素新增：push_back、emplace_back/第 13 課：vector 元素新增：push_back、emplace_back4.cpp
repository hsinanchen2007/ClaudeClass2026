#include <vector>
#include <iostream>
#include <string>

struct Person {
    std::string name;
    int age;
    double height;
    
    Person(const std::string& n, int a, double h)
        : name(n), age(a), height(h) {
        std::cout << "建構 Person: " << name << std::endl;
    }
    
    Person(const Person& other)
        : name(other.name), age(other.age), height(other.height) {
        std::cout << "複製 Person: " << name << std::endl;
    }
    
    Person(Person&& other) noexcept
        : name(std::move(other.name)), age(other.age), height(other.height) {
        std::cout << "移動 Person: " << name << std::endl;
    }
};

int main() {
    std::vector<Person> people;
    people.reserve(3);
    
    std::cout << "=== 使用 emplace_back ===" << std::endl;
    people.emplace_back("Alice", 30, 165.5);  // 直接建構
    
    std::cout << "\n=== 使用 push_back ===" << std::endl;
    people.push_back(Person("Bob", 25, 175.0));  // 建構 + 移動
    
    std::cout << "\n=== 使用大括號 push_back ===" << std::endl;
    people.push_back({"Charlie", 35, 180.0});  // 建構 + 移動
    
    std::cout << "\n=== 結束 ===" << std::endl;
    return 0;
}
