#include <vector>
#include <cstdint>

int main() {
    // 建立 1MB 的緩衝區
    std::vector<uint8_t> buffer(1024 * 1024);  // resize，直接可用
    
    // 或者只配置空間，稍後填入
    std::vector<uint8_t> buffer2;
    buffer2.reserve(1024 * 1024);  // 只 reserve，size 仍為 0
    
    return 0;
}
