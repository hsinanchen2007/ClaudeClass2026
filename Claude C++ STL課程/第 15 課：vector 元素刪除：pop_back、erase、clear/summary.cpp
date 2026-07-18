/*
 * ============================================================
 * 【第 15 課：vector 元素刪除：pop_back、erase、clear】總複習 summary.cpp
 * ============================================================
 * 本課程重點：
 * 1. pop_back()：刪除尾端元素（O(1)，最高效）
 * 2. erase(pos)：刪除指定位置元素（O(n)，需搬移後方元素）
 * 3. erase(first, last)：刪除範圍元素（O(n)）
 * 4. clear()：清空所有元素（size=0，但 capacity 不變）
 * 5. erase 的回傳值：指向被刪元素之後那個元素的迭代器
 * 6. 迴圈中安全刪除元素的正確方式（避免迭代器失效）
 * 7. Erase-Remove 慣用法（高效批量刪除 O(n)）
 * 8. C++20 std::erase / std::erase_if
 * 9. 不保持順序的快速刪除（O(1) swap-and-pop）
 * 10. 效能比較：逐一 erase（O(n²)）vs Erase-Remove（O(n)）
 * ============================================================
 * 刪除操作一覽表：
 * | 方法                    | 時間複雜度 | 回傳值               |
 * | pop_back()              | O(1)       | void                 |
 * | erase(pos)              | O(n)       | 下一個元素的迭代器   |
 * | erase(first, last)      | O(n)       | 下一個元素的迭代器   |
 * | clear()                 | O(n)       | void                 |
 * | std::erase(v,val)       | O(n)       | 刪除的數量（C++20）  |
 * | std::erase_if(v,pred)   | O(n)       | 刪除的數量（C++20）  |
 * ============================================================
 * 迭代器失效規則：
 * | 操作              | 失效的迭代器                    |
 * | pop_back()        | 指向被刪元素的迭代器、end()     |
 * | erase(pos)        | pos 及其之後的所有迭代器        |
 * | erase(first,last) | first 及其之後的所有迭代器      |
 * | clear()           | 所有迭代器                      |
 * ============================================================
 */

#include <vector>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <string>

// ===== 重點一：pop_back() — 刪除尾端元素 =====
// pop_back() 是最簡單也最高效的刪除方式
// 時間複雜度：O(1) — 直接減少 size，不移動其他元素
// 重要：capacity 不會改變，只有 size 減少
// 注意：對空 vector 呼叫 pop_back() 是【未定義行為 (UB)】！
void demo_pop_back() {
    std::cout << "\n===== pop_back() 示範 =====" << std::endl;

    std::vector<int> v = {1, 2, 3, 4, 5};
    v.reserve(10); // 預先擴容，capacity = 10

    std::cout << "原始: ";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;

    v.pop_back(); // 刪除 5
    v.pop_back(); // 刪除 4

    std::cout << "pop_back 兩次後: ";
    for (int x : v) std::cout << x << " "; // 1 2 3
    std::cout << std::endl;

    std::cout << "size: " << v.size() << std::endl;        // 3
    std::cout << "capacity: " << v.capacity() << std::endl; // 仍然 10（不變！）

    // 安全做法：呼叫前先檢查 empty()
    std::vector<int> empty_v;
    if (!empty_v.empty()) {
        empty_v.pop_back(); // 安全
    } else {
        std::cout << "vector 是空的，不能呼叫 pop_back()" << std::endl;
    }
    // 錯誤示範（已注釋掉，避免 UB）：
    // empty_v.pop_back();  // 危險！未定義行為
}

// ===== 重點二：erase(pos) — 刪除指定位置元素 =====
// erase 接受一個迭代器（指向要刪除的元素）
// 刪除後，該位置之後的所有元素往前移動一格
// 時間複雜度：O(n)
// 回傳值：指向被刪除元素「之後」那個元素的迭代器
//         若刪除最後一個元素，回傳 end()
void demo_erase_single() {
    std::cout << "\n===== erase(pos) 示範 =====" << std::endl;

    std::vector<int> v = {10, 20, 30, 40, 50};

    // 刪除第一個元素（索引 0）
    v.erase(v.begin());
    std::cout << "刪除第一個後: ";
    for (int x : v) std::cout << x << " "; // 20 30 40 50
    std::cout << std::endl;

    // 刪除現在索引 2 的元素（值 40）
    v.erase(v.begin() + 2);
    std::cout << "刪除索引 2 後: ";
    for (int x : v) std::cout << x << " "; // 20 30 50
    std::cout << std::endl;

    // 刪除最後一個元素（等同 pop_back，但效率稍低）
    v.erase(v.end() - 1);
    std::cout << "刪除最後後: ";
    for (int x : v) std::cout << x << " "; // 20 30
    std::cout << std::endl;
}

// ===== 重點三：erase 的回傳值 — 下一個有效迭代器 =====
// erase 回傳指向「被刪除元素之後」那個元素的迭代器
// 這個特性在迴圈刪除時非常重要（見重點六）
void demo_erase_return_value() {
    std::cout << "\n===== erase 回傳值示範 =====" << std::endl;

    std::vector<int> v = {10, 20, 30, 40, 50};

    // 刪除 30（索引 2），取得回傳的迭代器
    auto it = v.erase(v.begin() + 2);
    std::cout << "刪除 30 後，it 指向: " << *it << std::endl; // 40

    // 若刪除最後一個，回傳 end()
    it = v.erase(v.end() - 1); // 刪除 50
    if (it == v.end()) {
        std::cout << "it 現在是 end()（刪除最後元素的情況）" << std::endl;
    }
}

// ===== 重點四：erase(first, last) — 範圍刪除 =====
// 刪除半開區間 [first, last) 內的所有元素
// 時間複雜度：O(n)
// 回傳值：指向 last 位置（即原本 last 指向的元素，現在往前移了）的迭代器
void demo_erase_range() {
    std::cout << "\n===== erase(first, last) 範圍刪除 =====" << std::endl;

    std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // 刪除 [begin+2, begin+5)，即索引 2, 3, 4（值 3, 4, 5）
    v.erase(v.begin() + 2, v.begin() + 5);
    std::cout << "刪除索引 2~4 後: ";
    for (int x : v) std::cout << x << " "; // 1 2 6 7 8 9 10
    std::cout << std::endl;

    // 刪除前三個元素
    v.erase(v.begin(), v.begin() + 3);
    std::cout << "再刪除前三個後: ";
    for (int x : v) std::cout << x << " "; // 7 8 9 10
    std::cout << std::endl;

    // 刪除所有元素（等同 clear()）
    v.erase(v.begin(), v.end());
    std::cout << "全部刪除後 size: " << v.size() << std::endl; // 0
}

// ===== 重點五：clear() — 清空所有元素 =====
// clear() 呼叫每個元素的解構子，並將 size 設為 0
// 【重要】capacity 不變！這樣設計的好處：
//   - 如果你打算重新填入資料，不用再次配置記憶體
//   - 避免頻繁的記憶體配置/釋放
// 若要同時釋放記憶體，可使用 shrink_to_fit()
void demo_clear() {
    std::cout << "\n===== clear() 示範 =====" << std::endl;

    std::vector<int> v = {1, 2, 3, 4, 5};
    v.reserve(100); // 故意把 capacity 設大

    std::cout << "clear 前 - size: " << v.size()
              << ", capacity: " << v.capacity() << std::endl;

    v.clear();

    std::cout << "clear 後 - size: " << v.size()
              << ", capacity: " << v.capacity() << std::endl;
    // size = 0，capacity = 100（不變！）
}

// ===== 重點六：迴圈中安全刪除元素 =====
// 這是最容易出錯的地方！
// 錯誤做法：
//   (1) 用索引迴圈後 ++i：刪除後索引亂掉，跳過元素
//   (2) 用 it++（在 erase 後）：erase 後 it 失效，UB
// 正確做法：利用 erase 的回傳值
void demo_loop_erase() {
    std::cout << "\n===== 迴圈中安全刪除 =====" << std::endl;

    std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // 正確做法：利用 erase 回傳下一個有效迭代器
    for (auto it = v.begin(); it != v.end(); /* 不在這裡遞增 */) {
        if (*it % 2 == 0) {
            it = v.erase(it); // erase 回傳下一個元素的迭代器
        } else {
            ++it; // 只有不刪除時才前進
        }
    }

    std::cout << "刪除偶數後: ";
    for (int x : v) std::cout << x << " "; // 1 3 5 7 9
    std::cout << std::endl;
}

// ===== 重點七：Erase-Remove 慣用法 — 高效批量刪除 =====
// 逐一 erase 是 O(n²)（每次刪除後要搬移後面所有元素）
// Erase-Remove 慣用法是 O(n)（一次掃描，只移動一次）
//
// 原理：std::remove_if 把「不符合刪除條件」的元素往前搬
//       回傳新的「邏輯結尾」迭代器（new_end）
//       然後用 erase(new_end, end) 真正刪除尾部的殘留資料
void demo_erase_remove() {
    std::cout << "\n===== Erase-Remove 慣用法 =====" << std::endl;

    std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // 完整版（展示中間過程）
    auto new_end = std::remove_if(v.begin(), v.end(),
                                  [](int x) { return x % 2 == 0; });

    std::cout << "remove_if 後的原始資料（尾部有殘留）: ";
    for (int x : v) std::cout << x << " "; // 1 3 5 7 9 [舊資料殘留...]
    std::cout << std::endl;
    std::cout << "邏輯大小: " << (new_end - v.begin()) << std::endl;
    std::cout << "實際 size: " << v.size() << std::endl;

    // 真正刪除尾部垃圾
    v.erase(new_end, v.end());

    std::cout << "erase 後: ";
    for (int x : v) std::cout << x << " "; // 1 3 5 7 9
    std::cout << std::endl;

    // 通常寫成一行（慣用寫法）
    std::vector<int> v2 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    v2.erase(std::remove_if(v2.begin(), v2.end(),
                             [](int x) { return x % 2 == 0; }),
             v2.end());

    std::cout << "一行版 Erase-Remove: ";
    for (int x : v2) std::cout << x << " "; // 1 3 5 7 9
    std::cout << std::endl;
}

// ===== 重點八：C++20 std::erase 和 std::erase_if =====
// C++20 引入了更簡潔的方式，且回傳刪除的元素數量
void demo_cpp20_erase() {
    std::cout << "\n===== C++20 std::erase / std::erase_if =====" << std::endl;

    // std::erase(v, val)：刪除所有等於 val 的元素
    std::vector<int> v1 = {1, 2, 3, 2, 4, 2, 5};
    auto count1 = std::erase(v1, 2);
    std::cout << "刪除了 " << count1 << " 個 2: ";
    for (int x : v1) std::cout << x << " "; // 1 3 4 5
    std::cout << std::endl;

    // std::erase_if(v, pred)：刪除所有符合條件的元素
    std::vector<int> v2 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto count2 = std::erase_if(v2, [](int x) { return x % 2 == 0; });
    std::cout << "刪除了 " << count2 << " 個偶數: ";
    for (int x : v2) std::cout << x << " "; // 1 3 5 7 9
    std::cout << std::endl;
}

// ===== 重點九：不保持順序的快速刪除（O(1)）=====
// 如果不需要保持元素順序，可以用「swap-and-pop」技巧：
//   把最後一個元素移到要刪除的位置，然後 pop_back
// 時間複雜度：O(1)，比標準 erase 的 O(n) 快很多
template <typename T>
void fast_erase(std::vector<T>& v, size_t index) {
    if (index < v.size() - 1) {
        v[index] = std::move(v.back()); // 把最後一個移過來
    }
    v.pop_back(); // 刪除最後一個（現在是我們要刪的位置）
}

void demo_fast_erase() {
    std::cout << "\n===== 不保持順序的快速刪除（O(1)）=====" << std::endl;

    std::vector<int> v = {10, 20, 30, 40, 50};

    std::cout << "原始: ";
    for (int x : v) std::cout << x << " "; // 10 20 30 40 50
    std::cout << std::endl;

    // 刪除索引 1（值 20）
    fast_erase(v, 1);

    std::cout << "fast_erase(v, 1) 後: ";
    for (int x : v) std::cout << x << " "; // 10 50 30 40（順序改變！）
    std::cout << std::endl;
    std::cout << "注意：50 從尾端移到了索引 1 的位置（順序已改變）" << std::endl;
}

// ===== 重點十：效能比較 — 逐一 erase(O(n²)) vs Erase-Remove(O(n)) =====
// 差異可達 200 倍以上！
void demo_performance() {
    std::cout << "\n===== 效能比較：逐一 erase vs Erase-Remove =====" << std::endl;

    const int N = 50000; // 適中的 N，讓示範跑得較快

    // 方法一：逐一 erase（O(n²)）
    auto start1 = std::chrono::high_resolution_clock::now();
    {
        std::vector<int> v;
        for (int i = 0; i < N; ++i) v.push_back(i);

        for (auto it = v.begin(); it != v.end(); ) {
            if (*it % 2 == 0) {
                it = v.erase(it);
            } else {
                ++it;
            }
        }
    }
    auto end1 = std::chrono::high_resolution_clock::now();

    // 方法二：Erase-Remove 慣用法（O(n)）
    auto start2 = std::chrono::high_resolution_clock::now();
    {
        std::vector<int> v;
        for (int i = 0; i < N; ++i) v.push_back(i);

        v.erase(std::remove_if(v.begin(), v.end(),
                               [](int x) { return x % 2 == 0; }),
                v.end());
    }
    auto end2 = std::chrono::high_resolution_clock::now();

    auto ms1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count();
    auto ms2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2).count();

    std::cout << "逐一 erase（O(n²)）:   " << ms1 << " ms" << std::endl;
    std::cout << "Erase-Remove（O(n)）: " << ms2 << " ms" << std::endl;
    std::cout << "結論：Erase-Remove 快了大約 "
              << (ms1 > 0 && ms2 > 0 ? ms1 / ms2 : 100)
              << "x 以上（N 越大差距越明顯）" << std::endl;
}

// ===== 重點十一：觀察元素的建構/移動/銷毀過程（搭配 erase）=====
// erase 刪除中間元素時：
//   1. 被刪元素的解構子被呼叫
//   2. 後面的元素透過移動賦值往前搬
//   3. 最後一個位置的元素被銷毀（因為已移走）
struct TrackedItem {
    std::string name;

    TrackedItem(const std::string& n) : name(n) {
        std::cout << "  建構: " << name << std::endl;
    }
    TrackedItem(TrackedItem&& other) noexcept : name(std::move(other.name)) {
        std::cout << "  移動: " << name << std::endl;
    }
    TrackedItem& operator=(TrackedItem&& other) noexcept {
        name = std::move(other.name);
        std::cout << "  移動賦值: " << name << std::endl;
        return *this;
    }
    ~TrackedItem() {
        std::cout << "  銷毀: " << (name.empty() ? "(已移走)" : name) << std::endl;
    }
};

void demo_destruction_order() {
    std::cout << "\n===== erase 時的元素銷毀觀察 =====" << std::endl;

    std::vector<TrackedItem> v;
    v.reserve(4);
    v.emplace_back("A");
    v.emplace_back("B");
    v.emplace_back("C");
    v.emplace_back("D");

    std::cout << "\n--- 刪除 B（索引 1）---" << std::endl;
    v.erase(v.begin() + 1);
    // C 和 D 往前移動賦值，然後最後位置（已移走的 D）被銷毀

    std::cout << "\n現在的內容：";
    for (const auto& item : v) std::cout << item.name << " ";
    std::cout << std::endl;

    std::cout << "\n--- 呼叫 clear() ---" << std::endl;
    v.clear();
}

int main() {
    std::cout << "==========================================" << std::endl;
    std::cout << " 第 15 課：vector 元素刪除總複習" << std::endl;
    std::cout << "==========================================" << std::endl;

    demo_pop_back();
    demo_erase_single();
    demo_erase_return_value();
    demo_erase_range();
    demo_clear();
    demo_loop_erase();
    demo_erase_remove();
    demo_cpp20_erase();
    demo_fast_erase();
    demo_performance();
    demo_destruction_order();

    std::cout << "\n===== 本課核心重點整理 =====" << std::endl;
    std::cout << "1. pop_back()：O(1)，空 vector 呼叫是 UB，要先 empty() 檢查" << std::endl;
    std::cout << "2. erase(pos)：O(n)，回傳下一個有效迭代器" << std::endl;
    std::cout << "3. erase(first,last)：O(n)，範圍刪除" << std::endl;
    std::cout << "4. clear()：size=0，capacity 不變（設計目的：避免重新配置）" << std::endl;
    std::cout << "5. 迴圈刪除要用 it = v.erase(it) 而非 v.erase(it); ++it" << std::endl;
    std::cout << "6. Erase-Remove 慣用法：O(n)，比逐一 erase(O(n²)) 快百倍" << std::endl;
    std::cout << "7. C++20 std::erase_if：更簡潔的批量刪除，回傳刪除數量" << std::endl;
    std::cout << "8. 不保持順序時，swap-and-pop 可做到 O(1) 刪除" << std::endl;

    return 0;
}
