#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    int threshold = 5;
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // 值捕獲 [threshold] 或 [=]
    std::cout << "=== 值捕獲 ===" << std::endl;
    int count1 = std::count_if(vec.begin(), vec.end(),
        [threshold](int n) { return n > threshold; });
    std::cout << "大於 " << threshold << " 的個數: " << count1 << std::endl;
    
    // 值捕獲是「複製」，Lambda 內部的修改不影響外部
    auto by_value = [threshold]() mutable {  // mutable 允許修改捕獲的副本
        threshold = 100;  // 修改的是副本
        return threshold;
    };
    by_value();
    std::cout << "外部 threshold 仍然是: " << threshold << std::endl;
    
    // 參考捕獲 [&threshold] 或 [&]
    std::cout << "\n=== 參考捕獲 ===" << std::endl;
    int sum = 0;
    std::for_each(vec.begin(), vec.end(),
        [&sum](int n) { sum += n; });  // sum 以參考捕獲
    std::cout << "總和: " << sum << std::endl;
    
    // 參考捕獲可以修改外部變數
    int multiplier = 2;
    auto by_ref = [&multiplier]() {
        multiplier = 10;  // 修改外部變數
    };
    by_ref();
    std::cout << "multiplier 被修改為: " << multiplier << std::endl;
    
    return 0;
}
