// lesson31_overload.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -o lesson31c lesson31_overload.cpp

#include <iostream>
#include <string>

// 接受左值的版本
void process(const std::string& s) {
    std::cout << "  [左值版本] 收到 \"" << s << "\"（會拷貝）\n";
}

// 接受右值的版本
void process(std::string&& s) {
    std::cout << "  [右值版本] 收到 \"" << s << "\"（可以移動！）\n";
}

int main() {
    std::string name = "Dragon";

    std::cout << "傳入左值：\n";
    process(name);                    // 呼叫 const string& 版本

    std::cout << "\n傳入右值：\n";
    process(std::string("Phoenix"));  // 呼叫 string&& 版本

    std::cout << "\n傳入字面量：\n";
    process("Knight");                // "Knight" 建構暫時 string → 呼叫 string&& 版本

    std::cout << "\n傳入運算結果：\n";
    process(name + " King");          // name + " King" 產生暫時 string → 右值版本

    return 0;
}
