#include <vector>
#include <iostream>
#include <string>

struct Widget {
    std::string data;
    
    Widget(const std::string& s) : data(s) {
        std::cout << "從 const string& 建構" << std::endl;
    }
    
    Widget(std::string&& s) : data(std::move(s)) {
        std::cout << "從 string&& 建構" << std::endl;
    }
};

int main() {
    std::vector<Widget> v;
    v.reserve(5);
    
    std::string s = "Hello";
    
    std::cout << "emplace_back(s):" << std::endl;
    v.emplace_back(s);  // 呼叫 const string& 版本
    
    std::cout << "\nemplace_back(std::move(s)):" << std::endl;
    v.emplace_back(std::move(s));  // 呼叫 string&& 版本
    
    std::cout << "\nemplace_back(\"World\"):" << std::endl;
    v.emplace_back("World");  // 呼叫 string&& 版本（從字面量建構臨時 string）
    
    return 0;
}
