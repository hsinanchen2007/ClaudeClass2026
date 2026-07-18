// 檔案：lesson_5_5_dangerous_interface.cpp
// 說明：危險的介面設計

#include <iostream>
#include <mutex>
#include <vector>

class DangerousContainer {
private:
    std::mutex mtx;
    std::vector<int> data;
    
public:
    // 💀 危險！返回內部資料的引用
    std::vector<int>& getData() {
        std::lock_guard<std::mutex> lock(mtx);
        return data;  // 鎖在這裡就釋放了！
    }
    
    // 💀 危險！返回內部資料的指標
    int* getFirst() {
        std::lock_guard<std::mutex> lock(mtx);
        if (!data.empty()) {
            return &data[0];  // 返回後鎖就釋放了！
        }
        return nullptr;
    }
    
    void add(int value) {
        std::lock_guard<std::mutex> lock(mtx);
        data.push_back(value);
    }
};

/*
 * 問題分析：
 * 
 * Thread A:                          Thread B:
 * auto& vec = container.getData();   
 *   // 鎖已釋放！                    container.add(100);
 * vec.push_back(1);                    // 同時修改！
 *   // 💀 競爭條件！
 */

int main() {
    DangerousContainer container;
    container.add(10);
    container.add(20);
    
    // 示範危險的使用方式
    auto& vec = container.getData();  // 鎖已釋放
    vec.push_back(30);                 // 可能與其他執行緒競爭
    
    int* first = container.getFirst(); // 鎖已釋放
    if (first) {
        *first = 99;                   // 可能與其他執行緒競爭
    }
    
    // 輸出結果（不保證正確性）
    for (int val : container.getData()) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    
    return 0;
}