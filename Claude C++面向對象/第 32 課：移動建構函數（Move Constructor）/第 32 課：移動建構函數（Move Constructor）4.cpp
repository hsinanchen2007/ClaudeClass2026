// lesson32_performance.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -O2 -o lesson32d lesson32_performance.cpp

#include <iostream>
#include <chrono>
#include <vector>
#include <string>

int main() {
    const int N = 500000;

    // 建立一批很長的字串
    std::vector<std::string> source;
    source.reserve(N);
    for (int i = 0; i < N; ++i) {
        source.push_back(std::string(1000, 'A' + (i % 26)));
    }

    // ──── 測試拷貝 ────
    auto t1 = std::chrono::high_resolution_clock::now();
    std::vector<std::string> copied;
    copied.reserve(N);
    for (const auto& s : source) {
        copied.push_back(s);           // 拷貝：每次配置 1000 bytes + 複製
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    // ──── 測試移動 ────
    auto t3 = std::chrono::high_resolution_clock::now();
    std::vector<std::string> moved;
    moved.reserve(N);
    for (auto& s : source) {
        moved.push_back(std::move(s)); // 移動：只搬指標，零配置
    }
    auto t4 = std::chrono::high_resolution_clock::now();

    auto copy_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    auto move_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();

    std::cout << "拷貝 " << N << " 個 1000 字元的字串：" << copy_ms << " ms\n";
    std::cout << "移動 " << N << " 個 1000 字元的字串：" << move_ms << " ms\n";
    std::cout << "移動加速比：約 " << (move_ms > 0 ? copy_ms / move_ms : 999) << " 倍\n";

    return 0;
}
