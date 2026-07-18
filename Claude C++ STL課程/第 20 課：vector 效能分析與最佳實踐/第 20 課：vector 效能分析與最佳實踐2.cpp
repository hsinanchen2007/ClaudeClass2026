#include <iostream>
#include <vector>

int main() {
    // 場景一：逐一 push_back，但預知總量
    // → 用 reserve
    {
        std::vector<int> v;
        v.reserve(1000);  // size 仍為 0
        for (int i = 0; i < 1000; ++i) {
            v.push_back(i);  // 不會觸發重新配置
        }
        std::cout << "reserve: size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;
    }

    // 場景二：需要直接用索引存取，或傳給 C 函數
    // → 用 resize
    {
        std::vector<int> v;
        v.resize(1000);  // size 變為 1000，元素初始化為 0
        for (int i = 0; i < 1000; ++i) {
            v[i] = i;  // 直接賦值
        }
        std::cout << "resize: size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;
    }

    // 場景三：不確定總量，但能估計上界
    // → 用 reserve（估計值），避免大部分重新配置
    {
        std::vector<int> v;
        v.reserve(500);  // 估計大約 500 個
        for (int i = 0; i < 480; ++i) {  // 實際不到 500
            v.push_back(i);
        }
        std::cout << "估計 reserve: size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;
    }

    return 0;
}
