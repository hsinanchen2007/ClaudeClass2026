// # 第二階段：序列容器 — vector

// ## 第 15 課：vector 元素刪除：pop_back、erase、clear

// ---

// ### 核心概念

// 刪除元素是 vector 操作中需要特別注意的部分。不同的刪除方式有不同的效能特性，而且刪除操作會導致迭代器失效，處理不當容易產生 bug。

// ---

// ### 一、pop_back：刪除尾端元素

// 最簡單也最高效的刪除方式：

// ```cpp
// #include <vector>
// #include <iostream>

// int main() {
//     std::vector<int> v = {1, 2, 3, 4, 5};
    
//     std::cout << "原始: ";
//     for (int x : v) std::cout << x << " ";
//     std::cout << std::endl;
    
//     v.pop_back();  // 刪除 5
//     v.pop_back();  // 刪除 4
    
//     std::cout << "pop_back 兩次後: ";
//     for (int x : v) std::cout << x << " ";  // 1 2 3
//     std::cout << std::endl;
    
//     std::cout << "size: " << v.size() << std::endl;        // 3
//     std::cout << "capacity: " << v.capacity() << std::endl; // 仍然 >= 5
    
//     return 0;
// }
// ```

// **注意**：對空的 vector 呼叫 `pop_back()` 是未定義行為！

// ```cpp
// #include <vector>

// int main() {
//     std::vector<int> v;
    
//     // 危險！未定義行為
//     // v.pop_back();
    
//     // 安全做法
//     if (!v.empty()) {
//         v.pop_back();
//     }
    
//     return 0;
// }
// ```

// ---

// ### 二、erase：刪除指定位置的元素

// ```cpp
// #include <vector>
// #include <iostream>

// int main() {
//     std::vector<int> v = {10, 20, 30, 40, 50};
    
//     // 刪除第一個元素
//     v.erase(v.begin());
//     std::cout << "刪除第一個: ";
//     for (int x : v) std::cout << x << " ";  // 20 30 40 50
//     std::cout << std::endl;
    
//     // 刪除索引 2 的元素（現在是 40）
//     v.erase(v.begin() + 2);
//     std::cout << "刪除索引 2: ";
//     for (int x : v) std::cout << x << " ";  // 20 30 50
//     std::cout << std::endl;
    
//     // 刪除最後一個元素（等同 pop_back）
//     v.erase(v.end() - 1);
//     std::cout << "刪除最後: ";
//     for (int x : v) std::cout << x << " ";  // 20 30
//     std::cout << std::endl;
    
//     return 0;
// }
// ```

// ---

// ### 三、erase 的回傳值

// `erase` 回傳指向**被刪除元素之後**那個元素的迭代器：

// ```cpp
// #include <vector>
// #include <iostream>

// int main() {
//     std::vector<int> v = {10, 20, 30, 40, 50};
    
//     auto it = v.erase(v.begin() + 2);  // 刪除 30
    
//     std::cout << "刪除後 it 指向: " << *it << std::endl;  // 40
    
//     // 如果刪除的是最後一個元素，回傳 end()
//     it = v.erase(v.end() - 1);  // 刪除 50
//     if (it == v.end()) {
//         std::cout << "it 現在是 end()" << std::endl;
//     }
    
//     return 0;
// }
// ```

// ---

// ### 四、erase 範圍刪除

// ```cpp
// #include <vector>
// #include <iostream>

// int main() {
//     std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
//     // 刪除 [begin+2, begin+5) 範圍，即索引 2, 3, 4
//     v.erase(v.begin() + 2, v.begin() + 5);
    
//     std::cout << "刪除範圍後: ";
//     for (int x : v) std::cout << x << " ";  // 1 2 6 7 8 9 10
//     std::cout << std::endl;
    
//     // 刪除前三個元素
//     v.erase(v.begin(), v.begin() + 3);
    
//     std::cout << "再刪除前三個: ";
//     for (int x : v) std::cout << x << " ";  // 7 8 9 10
//     std::cout << std::endl;
    
//     // 刪除所有元素（等同 clear）
//     v.erase(v.begin(), v.end());
//     std::cout << "size: " << v.size() << std::endl;  // 0
    
//     return 0;
// }
// ```

// ---

// ### 五、clear：清空所有元素

// ```cpp
// #include <vector>
// #include <iostream>

// int main() {
//     std::vector<int> v = {1, 2, 3, 4, 5};
//     v.reserve(100);
    
//     std::cout << "clear 前 - size: " << v.size() 
//               << ", capacity: " << v.capacity() << std::endl;
    
//     v.clear();
    
//     std::cout << "clear 後 - size: " << v.size() 
//               << ", capacity: " << v.capacity() << std::endl;
//     // size = 0, capacity 不變
    
//     return 0;
// }
// ```

// ---

// ### 六、觀察元素的銷毀

// ```cpp
// #include <vector>
// #include <iostream>
// #include <string>

// struct Item {
//     std::string name;
    
//     Item(const std::string& n) : name(n) {
//         std::cout << "建構 " << name << std::endl;
//     }
    
//     Item(const Item& other) : name(other.name) {
//         std::cout << "複製 " << name << std::endl;
//     }
    
//     Item(Item&& other) noexcept : name(std::move(other.name)) {
//         std::cout << "移動 " << name << std::endl;
//     }
    
//     Item& operator=(Item&& other) noexcept {
//         name = std::move(other.name);
//         std::cout << "移動賦值 " << name << std::endl;
//         return *this;
//     }
    
//     ~Item() {
//         std::cout << "銷毀 " << name << std::endl;
//     }
// };

// int main() {
//     std::vector<Item> v;
//     v.reserve(5);
    
//     v.emplace_back("A");
//     v.emplace_back("B");
//     v.emplace_back("C");
//     v.emplace_back("D");
    
//     std::cout << "\n=== erase B ===" << std::endl;
//     v.erase(v.begin() + 1);
//     // 觀察：C 和 D 會往前移動，然後最後一個位置的元素被銷毀
    
//     std::cout << "\n=== 目前內容 ===" << std::endl;
//     for (const auto& item : v) {
//         std::cout << item.name << " ";
//     }
//     std::cout << std::endl;
    
//     std::cout << "\n=== clear ===" << std::endl;
//     v.clear();
    
//     std::cout << "\n=== 程式結束 ===" << std::endl;
//     return 0;
// }
// ```可以看到 `erase` 的運作方式：
// 1. C 和 D 往前移動賦值
// 2. 最後一個位置（已被移走的 D）被銷毀（name 已空）

// ---

// ### 七、迴圈中刪除元素的正確方式

// 這是最容易出錯的地方：

// ```cpp
// #include <vector>
// #include <iostream>

// int main() {
//     std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
//     // 目標：刪除所有偶數
    
//     // ❌ 錯誤做法 1：索引會亂掉
//     /*
//     for (size_t i = 0; i < v.size(); ++i) {
//         if (v[i] % 2 == 0) {
//             v.erase(v.begin() + i);
//             // 問題：刪除後，原本 i+1 變成 i，但 i 會 ++，跳過了一個元素
//         }
//     }
//     */
    
//     // ❌ 錯誤做法 2：迭代器失效
//     /*
//     for (auto it = v.begin(); it != v.end(); ++it) {
//         if (*it % 2 == 0) {
//             v.erase(it);
//             // 問題：erase 後 it 失效，++it 是未定義行為
//         }
//     }
//     */
    
//     // ✅ 正確做法 1：利用 erase 的回傳值
//     for (auto it = v.begin(); it != v.end(); ) {
//         if (*it % 2 == 0) {
//             it = v.erase(it);  // erase 回傳下一個元素的迭代器
//         } else {
//             ++it;
//         }
//     }
    
//     std::cout << "刪除偶數後: ";
//     for (int x : v) std::cout << x << " ";  // 1 3 5 7 9
//     std::cout << std::endl;
    
//     return 0;
// }
// ```

// ---

// ### 八、更高效的批量刪除：Erase-Remove 慣用法

// 逐一刪除元素效率很差（每次都要搬移後續元素），更好的做法是使用 **Erase-Remove 慣用法**：

// ```cpp
// #include <vector>
// #include <iostream>
// #include <algorithm>

// int main() {
//     std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
//     // std::remove 把不符合條件的元素往前搬，回傳新的「邏輯結尾」
//     // 然後用 erase 刪除尾端的「垃圾」
    
//     // 刪除所有偶數
//     auto new_end = std::remove_if(v.begin(), v.end(), 
//                                   [](int x) { return x % 2 == 0; });
    
//     std::cout << "remove_if 後: ";
//     for (int x : v) std::cout << x << " ";
//     std::cout << std::endl;
//     // 可能看到：1 3 5 7 9 6 7 8 9 10（後面是殘留資料）
    
//     std::cout << "邏輯大小: " << (new_end - v.begin()) << std::endl;
//     std::cout << "實際大小: " << v.size() << std::endl;
    
//     // 真正刪除
//     v.erase(new_end, v.end());
    
//     std::cout << "erase 後: ";
//     for (int x : v) std::cout << x << " ";  // 1 3 5 7 9
//     std::cout << std::endl;
    
//     return 0;
// }
// ```

// 通常寫成一行：

// ```cpp
// #include <vector>
// #include <iostream>
// #include <algorithm>

// int main() {
//     std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
//     // Erase-Remove 慣用法（一行）
//     v.erase(std::remove_if(v.begin(), v.end(), 
//                            [](int x) { return x % 2 == 0; }),
//             v.end());
    
//     for (int x : v) std::cout << x << " ";  // 1 3 5 7 9
//     std::cout << std::endl;
    
//     return 0;
// }
// ```

// ---

// ### 九、C++20 的 std::erase 和 std::erase_if

// C++20 簡化了這個操作：

// ```cpp
// #include <vector>
// #include <iostream>

// int main() {
//     std::vector<int> v1 = {1, 2, 3, 2, 4, 2, 5};
    
//     // C++20：刪除所有值為 2 的元素
//     auto count1 = std::erase(v1, 2);
//     std::cout << "刪除了 " << count1 << " 個 2: ";
//     for (int x : v1) std::cout << x << " ";  // 1 3 4 5
//     std::cout << std::endl;
    
//     std::vector<int> v2 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
//     // C++20：刪除所有偶數
//     auto count2 = std::erase_if(v2, [](int x) { return x % 2 == 0; });
//     std::cout << "刪除了 " << count2 << " 個偶數: ";
//     for (int x : v2) std::cout << x << " ";  // 1 3 5 7 9
//     std::cout << std::endl;
    
//     return 0;
// }
// ```---

// ### 十、效能比較

// ```cpp
// #include <vector>
// #include <iostream>
// #include <algorithm>
// #include <chrono>

// int main() {
//     const int N = 100000;
    
//     // 方法 1：逐一 erase（O(n²)）
//     auto start1 = std::chrono::high_resolution_clock::now();
//     {
//         std::vector<int> v;
//         for (int i = 0; i < N; ++i) v.push_back(i);
        
//         for (auto it = v.begin(); it != v.end(); ) {
//             if (*it % 2 == 0) {
//                 it = v.erase(it);
//             } else {
//                 ++it;
//             }
//         }
//     }
//     auto end1 = std::chrono::high_resolution_clock::now();
    
//     // 方法 2：Erase-Remove 慣用法（O(n)）
//     auto start2 = std::chrono::high_resolution_clock::now();
//     {
//         std::vector<int> v;
//         for (int i = 0; i < N; ++i) v.push_back(i);
        
//         v.erase(std::remove_if(v.begin(), v.end(),
//                                [](int x) { return x % 2 == 0; }),
//                 v.end());
//     }
//     auto end2 = std::chrono::high_resolution_clock::now();
    
//     auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);
//     auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);
    
//     std::cout << "逐一 erase:      " << duration1.count() << " ms" << std::endl;
//     std::cout << "Erase-Remove:    " << duration2.count() << " ms" << std::endl;
    
//     return 0;
// }
// ```差異非常明顯！Erase-Remove 慣用法快了 200 倍以上。

// ---

// ### 十一、刪除並保持順序 vs 不保持順序

// 如果不需要保持元素順序，可以用更快的方法：

// ```cpp
// #include <vector>
// #include <iostream>
// #include <algorithm>

// // 快速刪除單一元素（不保持順序）
// template <typename T>
// void fast_erase(std::vector<T>& v, size_t index) {
//     // 把最後一個元素移到要刪除的位置，然後 pop_back
//     if (index < v.size() - 1) {
//         v[index] = std::move(v.back());
//     }
//     v.pop_back();
// }

// int main() {
//     std::vector<int> v = {10, 20, 30, 40, 50};
    
//     std::cout << "原始: ";
//     for (int x : v) std::cout << x << " ";  // 10 20 30 40 50
//     std::cout << std::endl;
    
//     // 刪除索引 1（值為 20）
//     fast_erase(v, 1);
    
//     std::cout << "fast_erase 後: ";
//     for (int x : v) std::cout << x << " ";  // 10 50 30 40（順序改變了）
//     std::cout << std::endl;
    
//     return 0;
// }
// ```

// 這個方法是 O(1)，比標準 `erase` 的 O(n) 快很多。

// ---

// ### 刪除操作一覽表

// | 方法 | 說明 | 時間複雜度 | 回傳值 |
// |------|------|------------|--------|
// | `pop_back()` | 刪除尾端元素 | O(1) | void |
// | `erase(pos)` | 刪除指定位置 | O(n) | 下一個元素的迭代器 |
// | `erase(first, last)` | 刪除範圍 | O(n) | 下一個元素的迭代器 |
// | `clear()` | 清空所有元素 | O(n) | void |
// | `std::erase(v, val)` | 刪除所有等於 val 的元素（C++20） | O(n) | 刪除的數量 |
// | `std::erase_if(v, pred)` | 刪除所有符合條件的元素（C++20） | O(n) | 刪除的數量 |

// ---

// ### 迭代器失效規則

// | 操作 | 失效的迭代器 |
// |------|--------------|
// | `pop_back()` | 指向被刪元素的迭代器、`end()` |
// | `erase(pos)` | pos 及其之後的所有迭代器 |
// | `erase(first, last)` | first 及其之後的所有迭代器 |
// | `clear()` | 所有迭代器 |

// ---

// ### 練習題

// 1. **實作題**：寫一個函數，刪除 vector 中所有重複的元素，只保留第一次出現的（保持順序）。
//    ```cpp
//    void remove_duplicates(std::vector<int>& v);
//    // {1, 2, 3, 2, 1, 4} → {1, 2, 3, 4}
//    ```

// 2. **改錯題**：找出以下程式碼的問題並修正：
//    ```cpp
//    std::vector<int> v = {1, 2, 3, 4, 5};
//    for (auto it = v.begin(); it != v.end(); ++it) {
//        if (*it % 2 == 0) {
//            v.erase(it);
//        }
//    }
//    ```

// 3. **思考題**：為什麼 `clear()` 不會釋放記憶體（不改變 capacity）？這樣設計有什麼好處？

// 4. **效能題**：如果要刪除 vector 中間的一個元素，且不需要保持順序，用什麼方法最快？寫出實作。

// ---

// 下一課我們講 **vector 的迭代器操作**，深入理解迭代器的使用方式與失效規則。

// 準備好繼續嗎？



//### 十一、刪除並保持順序 vs 不保持順序
//如果不需要保持元素順序，可以用更快的方法：
#include <vector>
#include <iostream>
#include <algorithm>

// 快速刪除單一元素（不保持順序）
template <typename T>
void fast_erase(std::vector<T>& v, size_t index) {
    // 把最後一個元素移到要刪除的位置，然後 pop_back
    if (index < v.size() - 1) {
        v[index] = std::move(v.back());
    }
    v.pop_back();
}

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};
    
    std::cout << "原始: ";
    for (int x : v) std::cout << x << " ";  // 10 20 30 40 50
    std::cout << std::endl;
    
    // 刪除索引 1（值為 20）
    fast_erase(v, 1);
    
    std::cout << "fast_erase 後: ";
    for (int x : v) std::cout << x << " ";  // 10 50 30 40（順序改變了）
    std::cout << std::endl;
    
    return 0;
}