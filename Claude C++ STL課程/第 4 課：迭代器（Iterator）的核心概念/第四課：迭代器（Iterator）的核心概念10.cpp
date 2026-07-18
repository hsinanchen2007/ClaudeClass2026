#include <iostream>
#include <vector>

int main() {
    std::vector<int> vec = {10, 20, 30, 40, 50};
    
    // 取得底層指標
    int* ptr = vec.data();  // C++11
    // 或 int* ptr = &vec[0];
    
    // 取得迭代器
    auto it = vec.begin();
    
    std::cout << "=== 相似的操作 ===" << std::endl;
    std::cout << "指標解參考: *ptr = " << *ptr << std::endl;
    std::cout << "迭代器解參考: *it = " << *it << std::endl;
    
    std::cout << "\n指標算術: *(ptr + 2) = " << *(ptr + 2) << std::endl;
    std::cout << "迭代器算術: *(it + 2) = " << *(it + 2) << std::endl;
    
    // 迭代器可以轉換成指標（對於連續記憶體的容器）
    std::cout << "\n=== 迭代器轉指標 ===" << std::endl;
    int* from_iterator = &(*it);  // 取得迭代器指向的元素的位址
    std::cout << "*from_iterator = " << *from_iterator << std::endl;
    
    // 但指標不能直接當迭代器用在演算法中（需要配合 size）
    // 好消息是，原始指標本身就是一種迭代器！
    std::cout << "\n=== 指標作為迭代器 ===" << std::endl;
    int arr[] = {100, 200, 300};
    
    // 指標可以直接用在 STL 演算法
    auto found = std::find(arr, arr + 3, 200);
    if (found != arr + 3) {
        std::cout << "在陣列中找到 200" << std::endl;
    }
    
    return 0;
}
