#include <vector>
#include <string>

int main() {
    std::vector<std::string> v;
    v.reserve(10);
    
    // 情況 1：已有物件，需要保留原物件
    std::string s1 = "Hello";
    v.push_back(s1);  // 複製，s1 仍然有效
    // 這種情況 emplace_back(s1) 效果相同
    
    // 情況 2：已有物件，不再需要原物件
    std::string s2 = "World";
    v.push_back(std::move(s2));  // 移動
    // 這種情況 emplace_back(std::move(s2)) 效果相同
    
    // 情況 3：從字面量或臨時值
    v.push_back("Foo");     // 建構臨時 string，然後移動
    v.emplace_back("Bar");  // 直接就地建構，較優
    
    // 情況 4：需要多個參數建構
    // push_back 需要先建構物件
    // emplace_back 可以直接傳參數
    
    return 0;
}
