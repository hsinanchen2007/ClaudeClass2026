#include <iostream>
#include <vector>

// 模擬一個 C API：第一次呼叫取得大小，第二次呼叫填入資料
// 回傳值是實際需要的大小
int get_system_data(int* buffer, int buffer_size) {
    // 假設系統有 7 筆資料
    const int data[] = {11, 22, 33, 44, 55, 66, 77};
    const int data_count = 7;

    if (buffer == nullptr || buffer_size < data_count) {
        // 告訴呼叫者需要多大的空間
        return data_count;
    }

    // 填入資料
    for (int i = 0; i < data_count; ++i) {
        buffer[i] = data[i];
    }
    return data_count;
}

int main() {
    // 第一階段：查詢需要多少空間
    int needed = get_system_data(nullptr, 0);
    std::cout << "需要 " << needed << " 個元素的空間" << std::endl;

    // 第二階段：配置空間並取得資料
    std::vector<int> v(needed);
    get_system_data(v.data(), v.size());

    std::cout << "取得的資料：";
    for (int x : v) {
        std::cout << x << " ";
    }
    std::cout << std::endl;

    return 0;
}
