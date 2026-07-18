#include <vector>
#include <string>

int main() {
    std::vector<std::string> v;
    v.reserve(10);
    
    std::string s = "Hello";
    
    // 這兩個效能幾乎相同
    v.push_back(s);     // 複製
    v.emplace_back(s);  // 也是複製！因為 s 是左值
    
    // 這兩個效能幾乎相同
    v.push_back(std::move(s));     // 移動
    v.emplace_back(std::move(s));  // 也是移動
    
    return 0;
}
