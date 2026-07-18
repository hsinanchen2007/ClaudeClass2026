#include <iostream>
#include <string>

// 重載 1：接收左值
void process(const std::string& s) {
    std::cout << "[左值版本] 收到: " << s << "\n";
    // 這裡只能讀取 s，如果需要保存，必須複製
}

// 重載 2：接收右值
void process(std::string&& s) {
    std::cout << "[右值版本] 收到: " << s << "\n";
    // 這裡可以安全地「偷走」s 的資源
    std::string local = std::move(s);  // 移動，而非複製
    std::cout << "  移動到 local: " << local << "\n";
}

int main() {
    std::string name = "Alice";

    process(name);                        // 呼叫左值版本
    process(std::string("Bob"));          // 呼叫右值版本
    process("Charlie");                   // 呼叫右值版本（隱含轉型產生臨時物件）
    process(std::move(name));             // 呼叫右值版本（明確轉為右值）

    std::cout << "name after move: \"" << name << "\"\n";

    return 0;
}
