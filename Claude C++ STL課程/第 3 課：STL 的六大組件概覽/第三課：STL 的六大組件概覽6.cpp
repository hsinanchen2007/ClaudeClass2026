#include <iostream>
#include <vector>
#include <algorithm>

// 方法一：普通函數
bool is_even_func(int n) {
    return n % 2 == 0;
}

// 方法二：函數物件
class IsEven {
public:
    bool operator()(int n) const {
        return n % 2 == 0;
    }
};

// 方法三：帶狀態的函數物件
class IsDivisibleBy {
    int divisor;
public:
    IsDivisibleBy(int d) : divisor(d) {}
    bool operator()(int n) const {
        return n % divisor == 0;
    }
};

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // 使用普通函數, 直接將函數指針傳遞給count_if
    int count1 = std::count_if(vec.begin(), vec.end(), is_even_func);
    std::cout << "偶數個數（函數）: " << count1 << std::endl;
    
    // 使用函數物件, 這裡直接創建一個IsEven物件並傳遞給count_if
    int count2 = std::count_if(vec.begin(), vec.end(), IsEven());
    std::cout << "偶數個數（函數物件）: " << count2 << std::endl;
    
    // 使用帶狀態的函數物件, 這裡創建一個IsDivisibleBy物件，並傳遞給count_if來計算3的倍數和5的倍數
    int count3 = std::count_if(vec.begin(), vec.end(), IsDivisibleBy(3));
    std::cout << "3的倍數個數: " << count3 << std::endl;
    
    int count4 = std::count_if(vec.begin(), vec.end(), IsDivisibleBy(5));
    std::cout << "5的倍數個數: " << count4 << std::endl;
    
    return 0;
}
