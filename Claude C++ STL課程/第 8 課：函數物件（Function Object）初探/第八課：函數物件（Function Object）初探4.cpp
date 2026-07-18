#include <iostream>
#include <vector>
#include <algorithm>

// 普通函數：無法攜帶狀態
// 如果要判斷「大於 N」，N 從哪來？
// bool greater_than_n(int x) { return x > ???; }

// 函數物件：可以攜帶狀態！
class GreaterThan {
private:
    int threshold;
    
public:
    GreaterThan(int t) : threshold(t) {}
    
    bool operator()(int x) const {
        return x > threshold;
    }
};

class IsDivisibleBy {
private:
    int divisor;
    
public:
    IsDivisibleBy(int d) : divisor(d) {}
    
    bool operator()(int x) const {
        return x % divisor == 0;
    }
};

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // 使用帶狀態的函數物件
    int count_gt_5 = std::count_if(vec.begin(), vec.end(), GreaterThan(5));
    std::cout << "大於 5 的個數: " << count_gt_5 << std::endl;
    
    int count_gt_7 = std::count_if(vec.begin(), vec.end(), GreaterThan(7));
    std::cout << "大於 7 的個數: " << count_gt_7 << std::endl;
    
    int count_div_3 = std::count_if(vec.begin(), vec.end(), IsDivisibleBy(3));
    std::cout << "3 的倍數個數: " << count_div_3 << std::endl;
    
    int count_div_4 = std::count_if(vec.begin(), vec.end(), IsDivisibleBy(4));
    std::cout << "4 的倍數個數: " << count_div_4 << std::endl;
    
    return 0;
}
