#include <iostream>
#include <thread>

int main() {
    unsigned int cores = std::thread::hardware_concurrency();
    
    std::cout << "硬體支援的並行執行緒數: " << cores << std::endl;
    
    // 可能回傳 0 表示無法偵測
    if (cores == 0) {
        std::cout << "無法偵測，使用預設值" << std::endl;
        cores = 2;
    }
    
    return 0;
}
