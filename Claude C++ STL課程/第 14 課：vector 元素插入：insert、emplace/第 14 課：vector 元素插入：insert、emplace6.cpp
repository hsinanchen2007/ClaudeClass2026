#include <vector>
#include <iostream>
#include <string>

struct Person {
    std::string name;
    int age;
    
    Person(const std::string& n, int a) : name(n), age(a) {
        std::cout << "建構 " << name << std::endl;
    }
    
    Person(const Person& other) : name(other.name), age(other.age) {
        std::cout << "複製 " << name << std::endl;
    }
    
    Person(Person&& other) noexcept 
        : name(std::move(other.name)), age(other.age) {
        std::cout << "移動 " << name << std::endl;
    }
    
    // 補上賦值運算子
    Person& operator=(const Person& other) {
        name = other.name;
        age = other.age;
        std::cout << "複製賦值 " << name << std::endl;
        return *this;
    }
    
    Person& operator=(Person&& other) noexcept {
        name = std::move(other.name);
        age = other.age;
        std::cout << "移動賦值 " << name << std::endl;
        return *this;
    }
};

int main() {
    std::vector<Person> people;
    people.reserve(5);
    
    people.emplace_back("Alice", 30);
    people.emplace_back("Charlie", 35);
    
    std::cout << "\n=== 使用 insert ===" << std::endl;
    people.insert(people.begin() + 1, Person("Bob", 25));
    
    std::cout << "\n=== 使用 emplace ===" << std::endl;
    people.emplace(people.begin() + 1, "David", 28);
    
    std::cout << "\n=== 最終結果 ===" << std::endl;
    for (const auto& p : people) {
        std::cout << p.name << " (" << p.age << ")" << std::endl;
    }
    
    return 0;
}
