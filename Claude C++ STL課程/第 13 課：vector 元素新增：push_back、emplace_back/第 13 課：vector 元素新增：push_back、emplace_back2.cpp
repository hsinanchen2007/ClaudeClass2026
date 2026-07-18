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
    v.reserve(5);  // 預留空間，避免擴容干擾觀察
    
    std::cout << "=== 從左值 push_back ===" << std::endl;
    Tracker t1(1);
    v.push_back(t1);  // 複製
    
    std::cout << "\n=== 從右值 push_back ===" << std::endl;
    v.push_back(Tracker(2));  // 建構臨時物件，然後移動
    
    std::cout << "\n=== 用 std::move push_back ===" << std::endl;
    Tracker t3(3);
    v.push_back(std::move(t3));  // 移動
    
    std::cout << "\n=== 程式結束 ===" << std::endl;
    return 0;
}
