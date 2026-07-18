#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>

int main() {
    std::vector<int> vec = {5, 2, 8, 1, 9, 3, 7, 4, 6};
    
    // 形式一：只有範圍, 例如 sort 就是範圍版本的演算法
    std::cout << "=== 只有範圍 ===" << std::endl;
    std::sort(vec.begin(), vec.end());
    std::cout << "sort 後: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // 形式二：範圍 + 值, 例如 count 就是範圍版本的演算法
    std::cout << "\n=== 範圍 + 值 ===" << std::endl;
    int count = std::count(vec.begin(), vec.end(), 5);
    std::cout << "5 出現次數: " << count << std::endl;
    
    // 形式三：範圍 + 謂詞, 例如 count_if 就是範圍版本的演算法
    std::cout << "\n=== 範圍 + 謂詞 ===" << std::endl;
    int even_count = std::count_if(vec.begin(), vec.end(), 
        [](int n) { return n % 2 == 0; });
    std::cout << "偶數個數: " << even_count << std::endl;
    
    // 形式四：範圍 + 輸出迭代器, 例如 copy 就是範圍版本的演算法
    std::cout << "\n=== 範圍 + 輸出迭代器 ===" << std::endl;
    std::vector<int> dest(vec.size());
    std::copy(vec.begin(), vec.end(), dest.begin());
    std::cout << "copy 結果: ";
    for (int n : dest) std::cout << n << " ";
    std::cout << std::endl;
    
    // 形式五：兩個範圍, 例如 equal 就是範圍版本的演算法
    std::cout << "\n=== 兩個範圍 ===" << std::endl;
    std::vector<int> v1 = {1, 2, 3};
    std::vector<int> v2 = {1, 2, 3};
    bool are_equal = std::equal(v1.begin(), v1.end(), v2.begin());
    std::cout << "v1 和 v2 相等: " << (are_equal ? "是" : "否") << std::endl;
    
    return 0;
}
