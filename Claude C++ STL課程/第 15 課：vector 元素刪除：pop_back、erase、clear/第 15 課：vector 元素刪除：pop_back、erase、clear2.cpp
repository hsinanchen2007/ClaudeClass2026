#include <vector>

int main() {
    std::vector<int> v;
    
    // 危險！未定義行為
    // v.pop_back();
    
    // 安全做法
    if (!v.empty()) {
        v.pop_back();
    }
    
    return 0;
}
