#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <utility>

int main() {
    const int N = 1000000;
    std::string source(10000, 'x');  // 一萬個字元的字串

    // 測試複製
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < N; ++i) {
        std::string copy = source;  // 複製
        (void)copy;                 // 防止優化掉
    }
    auto copy_time = std::chrono::high_resolution_clock::now() - start;

    // 測試移動
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < N; ++i) {
        std::string temp = source;           // 先建一份供移動
        std::string moved = std::move(temp); // 移動
        (void)moved;
        source = moved;  // 恢復 source
    }
    auto move_time = std::chrono::high_resolution_clock::now() - start;

    std::cout << "複製 " << N << " 次 (10000 字元): "
              << std::chrono::duration_cast<std::chrono::milliseconds>(copy_time).count()
              << " ms\n";
    std::cout << "移動 " << N << " 次 (10000 字元): "
              << std::chrono::duration_cast<std::chrono::milliseconds>(move_time).count()
              << " ms\n";

    return 0;
}
