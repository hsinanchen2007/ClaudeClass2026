#include <vector>
#include <string>
#include <iostream>

struct Person {
    std::string name;
    int age;
    
    Person(const std::string& n, int a) : name(n), age(a) {}
};

int main() {
    // 使用初始化串列（需要隱式轉換或大括號建構）
    std::vector<Person> people = {
        {"Alice", 30},
        {"Bob", 25},
        {"Charlie", 35}
    };
    
    for (const auto& p : people) {
        std::cout << p.name << " is " << p.age << " years old." << std::endl;
    }
    
    return 0;
}
