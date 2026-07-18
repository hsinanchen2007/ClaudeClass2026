#include <vector>
#include <iostream>

int main() {
    int n;
    std::cout << "請輸入資料筆數: ";
    std::cin >> n;
    
    std::vector<int> data;
    data.reserve(n);  // 預先配置，避免擴容
    
    std::cout << "請輸入 " << n << " 筆整數: ";
    for (int i = 0; i < n; ++i) {
        int x;
        std::cin >> x;
        data.push_back(x);
    }
    
    // 處理資料...
    
    return 0;
}
