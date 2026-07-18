#include <iostream>
#include <string>

void greet(const std::string& name, const std::string& greeting = "Hello") {
    std::cout << greeting << ", " << name << "!" << std::endl;
}

int main() {
    greet("Alice");           // 使用預設值 "Hello"
    greet("Bob");             // 使用預設值 "Hello"
    greet("Charlie", "Hi");   // 使用自訂值 "Hi"
    
    return 0;
}
