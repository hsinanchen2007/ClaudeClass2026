#include <iostream>
#include <thread>
#include <map>
#include <string>

std::map<int, std::string> cache;

// 危險！Check-Then-Act 競爭
std::string getValue(int key) {
    if (cache.find(key) == cache.end()) {  // 檢查
        // ← 另一執行緒可能在此插入相同 key！
        cache[key] = "computed_" + std::to_string(key);  // 行動
    }
    return cache[key];
}

int main() {
    std::thread t1([]() { getValue(1); });
    std::thread t2([]() { getValue(1); });
    
    t1.join();
    t2.join();
    
    return 0;
}
