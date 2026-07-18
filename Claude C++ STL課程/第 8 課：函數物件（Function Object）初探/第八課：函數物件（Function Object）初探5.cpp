#include <iostream>
#include <vector>
#include <algorithm>

class Counter {
private:
    int count_;   // 狀態：計數器
    int target_;  // 狀態：目標值
    
public:
    // 建構子：初始化狀態
    Counter(int target) : count_(0), target_(target) {}
    
    // operator()：讓物件可以像函數一樣被呼叫
    bool operator()(int x) {
        if (x == target_) {
            ++count_;
            return true;
        }
        return false;
    }
    
    // 取得累積的狀態
    int get_count() const { return count_; }
};

int main() {
    std::vector<int> vec = {1, 2, 3, 2, 4, 2, 5, 2, 6};
    
    // 建立函數物件
    Counter counter(2);
    
    // 傳給演算法（注意：for_each 會回傳函數物件的副本）
    Counter result = std::for_each(vec.begin(), vec.end(), counter);
    
    std::cout << "找到 2 的次數: " << result.get_count() << std::endl;
    
    return 0;
}
