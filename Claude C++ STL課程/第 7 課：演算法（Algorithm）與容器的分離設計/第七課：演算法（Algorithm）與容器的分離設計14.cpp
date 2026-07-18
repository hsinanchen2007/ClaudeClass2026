#include <iostream>
#include <vector>
#include <numeric>

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    
    // accumulate：累加, 這裡計算 vec 中所有元素的總和
    std::cout << "=== accumulate ===" << std::endl;
    int sum = std::accumulate(vec.begin(), vec.end(), 0);
    std::cout << "總和: " << sum << std::endl;
    
    // accumulate 可以自訂運算, 這裡計算 vec 中所有元素的乘積
    int product = std::accumulate(vec.begin(), vec.end(), 1, 
        std::multiplies<int>());
    std::cout << "乘積: " << product << std::endl;
    
    // iota：填充遞增序列, 這裡填充一個從 1 開始的遞增序列到 seq 中
    std::cout << "\n=== iota ===" << std::endl;
    std::vector<int> seq(10);
    std::iota(seq.begin(), seq.end(), 1);  // 從 1 開始
    std::cout << "iota: ";
    for (int n : seq) std::cout << n << " ";
    std::cout << std::endl;
    
    // partial_sum：部分和, 這裡計算 vec 中元素的部分和，結果存儲在 partial 中
    std::cout << "\n=== partial_sum ===" << std::endl;
    std::vector<int> partial(vec.size());
    std::partial_sum(vec.begin(), vec.end(), partial.begin());
    std::cout << "部分和: ";
    for (int n : partial) std::cout << n << " ";
    std::cout << std::endl;
    
    // adjacent_difference：相鄰差, 這裡計算 data 中相鄰元素的差，結果存儲在 diff 中
    std::cout << "\n=== adjacent_difference ===" << std::endl;
    std::vector<int> data = {1, 3, 6, 10, 15};
    std::vector<int> diff(data.size());
    std::adjacent_difference(data.begin(), data.end(), diff.begin());
    std::cout << "相鄰差: ";
    for (int n : diff) std::cout << n << " ";
    std::cout << std::endl;
    
    // inner_product：內積, 這裡計算 v1 和 v2 的內積（點積）
    std::cout << "\n=== inner_product ===" << std::endl;
    std::vector<int> v1 = {1, 2, 3};
    std::vector<int> v2 = {4, 5, 6};
    int dot = std::inner_product(v1.begin(), v1.end(), v2.begin(), 0);
    std::cout << "內積: " << dot << " (1*4 + 2*5 + 3*6 = 32)" << std::endl;
    
    return 0;
}
