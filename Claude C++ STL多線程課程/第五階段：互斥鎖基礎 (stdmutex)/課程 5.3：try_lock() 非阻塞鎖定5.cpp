// 檔案：lesson_5_3_non_blocking_cache.cpp
// 說明：非阻塞的快取存取模式

#include <iostream>
#include <thread>
#include <mutex>
#include <map>
#include <string>
#include <chrono>
#include <vector>

class NonBlockingCache {
private:
    std::map<std::string, std::string> cache;
    std::mutex mtx;
    
public:
    // 嘗試從快取獲取值（非阻塞）
    bool tryGet(const std::string& key, std::string& value) {
        if (mtx.try_lock()) {
            auto it = cache.find(key);
            if (it != cache.end()) {
                value = it->second;
                mtx.unlock();
                return true;
            }
            mtx.unlock();
        }
        // 無法獲取鎖或找不到 key
        return false;
    }
    
    // 嘗試設定值（非阻塞）
    bool trySet(const std::string& key, const std::string& value) {
        if (mtx.try_lock()) {
            cache[key] = value;
            mtx.unlock();
            return true;
        }
        return false;
    }
    
    // 阻塞版本（保證成功）
    void set(const std::string& key, const std::string& value) {
        mtx.lock();
        cache[key] = value;
        mtx.unlock();
    }
    
    std::string get(const std::string& key) {
        mtx.lock();
        std::string result = cache[key];
        mtx.unlock();
        return result;
    }
};

NonBlockingCache cache;

void fastReader(int id) {
    std::string value;
    
    for (int i = 0; i < 5; ++i) {
        if (cache.tryGet("data", value)) {
            std::cout << "Reader " << id << "：快速讀取成功 = " << value << std::endl;
        } else {
            std::cout << "Reader " << id << "：快取忙碌，使用預設值" << std::endl;
            // 使用預設值或其他邏輯
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void writer() {
    for (int i = 0; i < 5; ++i) {
        std::string value = "value_" + std::to_string(i);
        cache.set("data", value);
        std::cout << "Writer：寫入 " << value << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}

int main() {
    cache.set("data", "initial");
    
    std::thread w(writer);
    std::thread r1(fastReader, 1);
    std::thread r2(fastReader, 2);
    
    w.join();
    r1.join();
    r2.join();
    
    return 0;
}
