#include <iostream>
#include <vector>

void fill_data(int* buffer, int count) {
    for (int i = 0; i < count; ++i) {
        buffer[i] = i * 10;
    }
}

int main() {
    // ===== 錯誤做法：reserve =====
    {
        std::vector<int> v;
        v.reserve(5);  // 只分配記憶體，size 仍然是 0

        // 雖然記憶體已分配，但 vector 認為 size 是 0
        fill_data(v.data(), 5);  // 寫入「未被 vector 管理」的記憶體

        std::cout << "reserve 後 size = " << v.size() << std::endl;
        // v.size() 是 0，所以用 range-for 遍歷什麼都看不到
        for (int x : v) {
            std::cout << x << " ";  // 不會執行
        }
        std::cout << "（沒有輸出）" << std::endl;
    }

    // ===== 正確做法：resize =====
    {
        std::vector<int> v;
        v.resize(5);  // 分配記憶體且 size 變成 5

        fill_data(v.data(), 5);

        std::cout << "resize 後 size = " << v.size() << std::endl;
        for (int x : v) {
            std::cout << x << " ";  // 正常遍歷
        }
        std::cout << std::endl;
    }

    return 0;
}
