#include <iostream>
#include <vector>
#include <algorithm>

// 手寫的函數物件
class GreaterThanFunctor {
private:
    int threshold;
public:
    GreaterThanFunctor(int t) : threshold(t) {}
    bool operator()(int n) const {
        return n > threshold;
    }
};

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    int threshold = 5;
    
    // 方法一：手寫函數物件
    int count1 = std::count_if(vec.begin(), vec.end(), 
                               GreaterThanFunctor(threshold));
    
    // 方法二：Lambda（編譯器自動生成類似的類別）
    int count2 = std::count_if(vec.begin(), vec.end(),
        [threshold](int n) { return n > threshold; });
    
    std::cout << "函數物件結果: " << count1 << std::endl;
    std::cout << "Lambda 結果: " << count2 << std::endl;
    
    // 它們是等價的！
    
    return 0;
}
