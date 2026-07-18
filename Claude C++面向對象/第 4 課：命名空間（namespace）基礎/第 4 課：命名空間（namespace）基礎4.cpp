#include <iostream>
#include <vector>

// 不要在全域範圍使用 using namespace

void processData() {
    // 在函數內部使用是安全的，作用域限制在這個函數內
    using namespace std;
    
    vector<int> data = {1, 2, 3, 4, 5};
    for (int x : data) {
        cout << x << " ";
    }
    cout << endl;
}

int main() {
    // 這裡仍需使用完整名稱
    std::cout << "開始處理..." << std::endl;
    processData();
    std::cout << "處理完成" << std::endl;
    
    return 0;
}
