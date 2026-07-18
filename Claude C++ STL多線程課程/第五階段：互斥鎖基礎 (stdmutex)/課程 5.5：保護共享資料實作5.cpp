// 檔案：lesson_5_5_safe_interface.cpp
// 說明：安全的介面設計

#include <iostream>
#include <mutex>
#include <vector>
#include <functional>
#include <algorithm>

class SafeContainer {
private:
    mutable std::mutex mtx;
    std::vector<int> data;
    
public:
    // ✓ 安全：返回複本
    std::vector<int> getData() const {
        std::lock_guard<std::mutex> lock(mtx);
        return data;  // 返回複本，不是引用
    }
    
    // ✓ 安全：返回值的複本
    int getAt(size_t index) const {
        std::lock_guard<std::mutex> lock(mtx);
        if (index < data.size()) {
            return data[index];  // 返回值，不是引用
        }
        throw std::out_of_range("索引超出範圍");
    }
    
    // ✓ 安全：提供操作而非資料存取
    void add(int value) {
        std::lock_guard<std::mutex> lock(mtx);
        data.push_back(value);
    }
    
    // ✓ 安全：使用回呼函式處理資料
    void forEach(const std::function<void(int)>& func) const {
        std::lock_guard<std::mutex> lock(mtx);
        for (int value : data) {
            func(value);
        }
    }
    
    // ✓ 安全：在鎖保護下執行操作
    template<typename Func>
    auto withLock(Func&& func) -> decltype(func(data)) {
        std::lock_guard<std::mutex> lock(mtx);
        return func(data);
    }
    
    // ✓ 安全：批次操作
    void addAll(const std::vector<int>& values) {
        std::lock_guard<std::mutex> lock(mtx);
        data.insert(data.end(), values.begin(), values.end());
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx);
        return data.size();
    }
    
    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx);
        return data.empty();
    }
};

int main() {
    SafeContainer container;
    
    container.add(10);
    container.add(20);
    container.add(30);
    
    // 使用 forEach
    std::cout << "forEach: ";
    container.forEach([](int v) {
        std::cout << v << " ";
    });
    std::cout << std::endl;
    
    // 使用 withLock 進行複雜操作
    int sum = container.withLock([](std::vector<int>& data) {
        int total = 0;
        for (int v : data) {
            total += v;
        }
        return total;
    });
    std::cout << "Sum: " << sum << std::endl;
    
    // 獲取複本
    auto copy = container.getData();
    std::cout << "Copy size: " << copy.size() << std::endl;
    
    return 0;
}
