#include <iostream>
#include <vector>

template <typename T>
void process(std::vector<T>& v) {
    // 對每個元素取引用來修改
    for (size_t i = 0; i < v.size(); ++i) {
        T& elem = v[i];   // 對 vector<int> OK
                           // 對 vector<bool> 編譯錯誤！
        // ... 處理 elem ...
    }
}

int main() {
    std::vector<int> vi = {1, 2, 3};
    process(vi);   // OK

    std::vector<bool> vb = {true, false, true};
    // process(vb);  // 編譯錯誤！T& elem = v[i] 對 bool 行不通

    return 0;
}
