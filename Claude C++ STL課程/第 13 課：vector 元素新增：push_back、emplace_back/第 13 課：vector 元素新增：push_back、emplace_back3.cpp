#include <vector>
#include <iostream>

class Tracker {
public:
    int id;
    
    Tracker(int i) : id(i) {
        std::cout << "建構 Tracker(" << id << ")" << std::endl;
    }
    
    Tracker(const Tracker& other) : id(other.id) {
        std::cout << "複製建構 Tracker(" << id << ")" << std::endl;
    }
    
    Tracker(Tracker&& other) noexcept : id(other.id) {
        std::cout << "移動建構 Tracker(" << id << ")" << std::endl;
    }
    
    ~Tracker() {
        std::cout << "銷毀 Tracker(" << id << ")" << std::endl;
    }
};

int main() {
    std::vector<Tracker> v;
    v.reserve(3);
    
    std::cout << "=== emplace_back ===" << std::endl;
    v.emplace_back(1);  // 直接在 vector 內部建構，只有一次建構
    
    std::cout << "\n=== 對比 push_back ===" << std::endl;
    v.push_back(Tracker(2));  // 建構 + 移動 + 銷毀臨時物件
    
    std::cout << "\n=== 程式結束 ===" << std::endl;
    return 0;
}
