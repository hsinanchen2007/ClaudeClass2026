/*
 * ================================================================
 * 【第11課：vector 的容量管理：size、capacity、reserve】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 * 本課重點：
 * 1. size() / capacity() / empty() / max_size() 的語意
 * 2. reserve(n)：預先配置空間，不改變 size，不建立元素
 * 3. resize(n) / resize(n, val)：改變實際元素數量
 * 4. reserve vs resize 的關鍵差異對照
 * 5. shrink_to_fit()：請求釋放多餘容量（非強制）
 * 6. clear() 保留 capacity vs swap 技巧 vs clear+shrink
 * 7. 效能實測：有無 reserve 的差異
 * 8. 實際應用場景
 * ================================================================
 */

#include <iostream>
#include <vector>
#include <chrono>
#include <cstdint>

// ===== 重點一：size / capacity / empty / max_size =====
// size()     → 目前實際存放的元素數量
// capacity() → 目前配置的空間可容納的元素數量（>= size）
// empty()    → 等價於 size() == 0
// max_size() → 理論上的最大容量（受限於 allocator 和系統記憶體）
//
// 不變量：size() <= capacity() 永遠成立

void demo_size_capacity_empty() {
    std::cout << "\n===== 重點一：size / capacity / empty =====\n";

    std::vector<int> v = {1, 2, 3, 4, 5};

    std::cout << "size:     " << v.size()     << std::endl;  // 5（實際元素數量）
    std::cout << "capacity: " << v.capacity() << std::endl;  // >= 5（配置的空間）
    std::cout << "empty:    " << v.empty()    << std::endl;  // 0（false）

    std::vector<int> empty_v;
    std::cout << "空 vector size: " << empty_v.size() << std::endl;  // 0
    std::cout << "空 vector empty: " << empty_v.empty() << std::endl;  // 1

    // max_size 在 64 位元系統上通常是非常大的數字
    std::cout << "max_size: " << v.max_size() << std::endl;
}

// ===== 重點二：reserve(n) — 預先配置空間 =====
// reserve 只改變 capacity，不改變 size，也不建立任何元素。
// 主要用途：當事先知道約需多少元素時，預先配置空間避免多次擴容。
//
// reserve 的效果：
//   - 若 n > capacity()：重新配置，capacity 至少變為 n
//   - 若 n <= capacity()：什麼也不做（不會縮減空間）
//
// 重要警告：reserve 後 size 仍為 0，不能用 operator[] 直接存取！
//   v.reserve(100);
//   v[0] = 5;  // 未定義行為！size 是 0

void demo_reserve() {
    std::cout << "\n===== 重點二：reserve 預先配置空間 =====\n";

    std::vector<int> v;

    std::cout << "初始 - size: " << v.size()
              << ", capacity: " << v.capacity() << std::endl;

    v.reserve(100);  // 預先配置至少 100 個元素的空間

    std::cout << "reserve(100) 後 - size: " << v.size()
              << ", capacity: " << v.capacity() << std::endl;
    // size 仍是 0，capacity >= 100

    // 現在連續 push_back 100 次都不會觸發擴容
    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
    }

    std::cout << "100 次 push_back 後 - size: " << v.size()
              << ", capacity: " << v.capacity() << std::endl;

    // 示範 reserve 對小值無效（不縮減）
    v.reserve(10);  // capacity 不會縮小
    std::cout << "reserve(10) 後 capacity: " << v.capacity() << std::endl;
    // capacity 仍 >= 100，reserve 不縮減空間
}

// ===== 重點三：resize(n) — 改變實際元素數量 =====
// resize(n)     → 將 size 改為 n；擴大時新元素用預設值填充（0）
// resize(n, val)→ 將 size 改為 n；擴大時新元素用 val 填充
// 縮小時：多餘的元素被銷毀（呼叫解構子），但 capacity 通常不變

void demo_resize() {
    std::cout << "\n===== 重點三：resize 改變元素數量 =====\n";

    std::vector<int> v = {1, 2, 3, 4, 5};
    std::cout << "初始 size: " << v.size() << std::endl;  // 5

    // 擴大：新元素用預設值（0）填充
    v.resize(8);
    std::cout << "resize(8): ";
    for (int x : v) std::cout << x << " ";  // 1 2 3 4 5 0 0 0
    std::cout << std::endl;

    // 擴大並指定填充值
    v.resize(10, 99);
    std::cout << "resize(10, 99): ";
    for (int x : v) std::cout << x << " ";  // 1 2 3 4 5 0 0 0 99 99
    std::cout << std::endl;

    // 縮小：多餘的元素被銷毀
    v.resize(3);
    std::cout << "resize(3): ";
    for (int x : v) std::cout << x << " ";  // 1 2 3
    std::cout << std::endl;

    // 注意：縮小後 capacity 通常不變（不釋放記憶體）
    std::cout << "縮小後 capacity: " << v.capacity() << std::endl;  // 仍然 >= 10
}

// ===== 重點四：reserve vs resize 關鍵差異 =====
// | 函數           | 改變 size | 改變 capacity          | 建立元素      |
// |----------------|-----------|------------------------|---------------|
// | reserve(n)     | 否        | 是（若 n > capacity）  | 否            |
// | resize(n)      | 是        | 可能（若 n > capacity）| 是（擴大時）  |
//
// 使用時機：
//   reserve：知道大約需要多少元素，預先配置避免擴容，之後用 push_back 填入
//   resize：需要一個固定大小的 vector，直接可用 operator[] 存取

void demo_reserve_vs_resize() {
    std::cout << "\n===== 重點四：reserve vs resize 對照 =====\n";

    std::vector<int> v1, v2;

    v1.reserve(5);
    v2.resize(5);

    std::cout << "reserve(5): size=" << v1.size()
              << ", capacity=" << v1.capacity() << std::endl;
    // size=0, capacity>=5

    std::cout << "resize(5):  size=" << v2.size()
              << ", capacity=" << v2.capacity() << std::endl;
    // size=5, capacity>=5

    // reserve 後不能直接用 operator[]（UB！）
    // v1[0] = 10;  // 危險！size 是 0

    // resize 後可以直接用 operator[]
    v2[0] = 10;  // 合法，因為 size 是 5
    std::cout << "resize 後 v2[0] = " << v2[0] << std::endl;  // 10

    // 正確使用 reserve 的方式：用 push_back
    v1.push_back(99);
    std::cout << "reserve 後 push_back，v1[0] = " << v1[0] << std::endl;  // 99
}

// ===== 重點五：shrink_to_fit — 釋放多餘空間 =====
// C++11 引入，請求釋放多餘的 capacity。
// 注意：這是非強制性請求，編譯器可以忽略（實務上大多數實作會執行）

void demo_shrink_to_fit() {
    std::cout << "\n===== 重點五：shrink_to_fit 釋放多餘空間 =====\n";

    std::vector<int> v;
    v.reserve(1000);

    for (int i = 0; i < 10; ++i) {
        v.push_back(i);
    }

    std::cout << "shrink 前: size=" << v.size()
              << ", capacity=" << v.capacity() << std::endl;
    // size=10, capacity=1000

    v.shrink_to_fit();

    std::cout << "shrink 後: size=" << v.size()
              << ", capacity=" << v.capacity() << std::endl;
    // size=10, capacity 約等於 10（不保證，但大多數實作會縮減）
}

// ===== 重點六：清空 vector 的三種方式 =====
// 方法一：clear()             → size=0，capacity 不變（保留記憶體供再用）
// 方法二：swap 技巧           → size=0，capacity=0（真正釋放記憶體，C++11 前慣用法）
// 方法三：clear + shrink_to_fit → size=0，capacity≈0（C++11 後推薦）
//
// 選擇建議：
//   若之後還要重新填入大量資料 → 用 clear()（避免重新配置）
//   若確定不再需要這些記憶體   → 用 clear + shrink_to_fit

void demo_clearing_methods() {
    std::cout << "\n===== 重點六：清空 vector 的方式比較 =====\n";

    // --- 方法一：clear（保留 capacity）---
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        v.reserve(100);
        std::cout << "clear 前:  size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;

        v.clear();
        std::cout << "clear 後:  size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;
        // size=0, capacity 不變（仍是 100）
    }

    // --- 方法二：swap 技巧（真正釋放記憶體）---
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        v.reserve(100);
        std::cout << "swap 前:   size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;

        std::vector<int>().swap(v);  // 用空的臨時 vector 和 v 交換
        std::cout << "swap 後:   size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;
        // size=0, capacity=0
    }

    // --- 方法三：clear + shrink_to_fit（C++11 後推薦）---
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        v.reserve(100);
        std::cout << "c+s 前:    size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;

        v.clear();
        v.shrink_to_fit();
        std::cout << "c+s 後:    size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;
        // size=0, capacity 約為 0
    }
}

// ===== 重點七：效能實測 — 有無 reserve 的差異 =====
// reserve 避免了多次重新配置，可以顯著提升效能（尤其是大量資料時）

void demo_performance() {
    std::cout << "\n===== 重點七：效能比較（reserve vs 無 reserve）=====\n";

    const int N = 1000000;

    // 不使用 reserve
    auto start1 = std::chrono::high_resolution_clock::now();
    {
        std::vector<int> v1;
        for (int i = 0; i < N; ++i) {
            v1.push_back(i);
        }
    }
    auto end1 = std::chrono::high_resolution_clock::now();

    // 使用 reserve
    auto start2 = std::chrono::high_resolution_clock::now();
    {
        std::vector<int> v2;
        v2.reserve(N);
        for (int i = 0; i < N; ++i) {
            v2.push_back(i);
        }
    }
    auto end2 = std::chrono::high_resolution_clock::now();

    auto d1 = std::chrono::duration_cast<std::chrono::microseconds>(end1 - start1);
    auto d2 = std::chrono::duration_cast<std::chrono::microseconds>(end2 - start2);

    std::cout << "不使用 reserve（" << N << " 次 push_back）: "
              << d1.count() << " 微秒" << std::endl;
    std::cout << "使用 reserve  （" << N << " 次 push_back）: "
              << d2.count() << " 微秒" << std::endl;
}

// ===== 重點八：實際應用場景 =====
// 場景一：讀取已知大小資料時預先 reserve
// 場景二：建立固定大小緩衝區時用 resize
// 場景三：收集不確定數量結果時，以上界 reserve，完成後 shrink

std::vector<int> find_even_numbers(const std::vector<int>& input) {
    std::vector<int> result;
    result.reserve(input.size());  // 最多有 input.size() 個偶數（保守上界）

    for (int x : input) {
        if (x % 2 == 0) {
            result.push_back(x);
        }
    }

    result.shrink_to_fit();  // 釋放多餘空間（可選）
    return result;
}

void demo_practical_usage() {
    std::cout << "\n===== 重點八：實際應用場景 =====\n";

    // 場景一：固定大小緩衝區
    std::vector<uint8_t> buffer(1024);  // resize，直接可用
    std::cout << "buffer(1024) size: " << buffer.size() << std::endl;

    // 場景二：只配置空間，稍後填入
    std::vector<uint8_t> buffer2;
    buffer2.reserve(1024);  // 只 reserve，size 仍為 0
    std::cout << "buffer2 reserve(1024) size: " << buffer2.size()
              << ", capacity: " << buffer2.capacity() << std::endl;

    // 場景三：收集偶數
    std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto evens = find_even_numbers(nums);
    std::cout << "偶數結果: ";
    for (int x : evens) std::cout << x << " ";  // 2 4 6 8 10
    std::cout << std::endl;
}

// ===== 容量管理函數一覽表 =====
// | 函數              | 說明                                          |
// |-------------------|-----------------------------------------------|
// | size()            | 回傳目前元素數量                              |
// | capacity()        | 回傳目前配置空間可容納的元素數量              |
// | empty()           | 回傳 size() == 0                              |
// | max_size()        | 回傳理論最大容量                              |
// | reserve(n)        | 確保 capacity >= n，不改變 size               |
// | resize(n)         | 改變 size 為 n，必要時擴大 capacity           |
// | resize(n, val)    | 同上，新元素用 val 填充                       |
// | shrink_to_fit()   | 請求釋放多餘的 capacity（非強制）             |
// | clear()           | 移除所有元素，size 變 0，capacity 不變        |

int main() {
    std::cout << "====================================================\n";
    std::cout << " 第11課：vector 的容量管理 — 總複習\n";
    std::cout << "====================================================\n";

    demo_size_capacity_empty();
    demo_reserve();
    demo_resize();
    demo_reserve_vs_resize();
    demo_shrink_to_fit();
    demo_clearing_methods();
    demo_performance();
    demo_practical_usage();

    std::cout << "\n====================================================\n";
    std::cout << " 複習完畢！\n";
    std::cout << "====================================================\n";

    return 0;
}
