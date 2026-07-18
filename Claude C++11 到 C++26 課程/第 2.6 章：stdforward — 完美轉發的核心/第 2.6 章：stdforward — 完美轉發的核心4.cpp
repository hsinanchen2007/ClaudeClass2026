#include <iostream>
#include <string>
#include <memory>
#include <utility>

class Person {
    std::string name_;
    int age_;

public:
    Person(const std::string& name, int age)
        : name_(name), age_(age) {
        std::cout << "  Person(const string&, int) 複製 name\n";
    }

    Person(std::string&& name, int age)
        : name_(std::move(name)), age_(age) {
        std::cout << "  Person(string&&, int) 移動 name\n";
    }

    void print() const {
        std::cout << "  " << name_ << ", age " << age_ << "\n";
    }
};

// 泛型工廠函式：完美轉發任意數量、任意型別的引數
template<typename T, typename... Args>
std::unique_ptr<T> make(Args&&... args) {
    return std::unique_ptr<T>(
        new T(std::forward<Args>(args)...)  // 完美轉發所有引數
    );
}

int main() {
    std::string name = "Alice";

    std::cout << "--- 傳入左值 ---\n";
    auto p1 = make<Person>(name, 30);
    p1->print();

    std::cout << "\n--- 傳入右值 ---\n";
    auto p2 = make<Person>(std::string("Bob"), 25);
    p2->print();

    std::cout << "\n--- 傳入字面值 ---\n";
    auto p3 = make<Person>("Charlie", 35);  // const char* → string 隱式轉換
    p3->print();

    return 0;
}
