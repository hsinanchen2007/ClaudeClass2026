// # 第二階段：序列容器 — vector

// ## 第 14 課：vector 元素插入：insert、emplace

// ---

// ### 核心概念

// 上一課學了在尾端新增元素，這一課要學習在**任意位置**插入元素。這是 vector 較昂貴的操作，因為需要搬移後續元素，但有時確實需要這個功能。

// ---

// ### 一、insert 基本用法：插入單一元素

// ```cpp
// #include <vector>
// #include <iostream>

// int main() {
//     std::vector<int> v = {1, 2, 3, 4, 5};
    
//     // insert 需要一個迭代器指定插入位置
//     // 新元素會插入在該迭代器「之前」
    
//     // 在開頭插入
//     v.insert(v.begin(), 0);
//     // v: {0, 1, 2, 3, 4, 5}
    
//     // 在第三個位置插入（索引 2）
//     v.insert(v.begin() + 2, 100);
//     // v: {0, 1, 100, 2, 3, 4, 5}
    
//     // 在結尾插入（等同於 push_back）
//     v.insert(v.end(), 6);
//     // v: {0, 1, 100, 2, 3, 4, 5, 6}
    
//     for (int x : v) {
//         std::cout << x << " ";
//     }
//     std::cout << std::endl;
    
//     return 0;
// }
// ```

// **重點**：`insert(pos, value)` 會將 value 插入在 pos **之前**。

// ---

// ### 二、insert 的回傳值

// `insert` 回傳指向**新插入元素**的迭代器：

// ```cpp
// #include <vector>
// #include <iostream>

// int main() {
//     std::vector<int> v = {1, 2, 3};
    
//     auto it = v.insert(v.begin() + 1, 100);
    
//     std::cout << "插入的值: " << *it << std::endl;  // 100
//     std::cout << "插入位置的索引: " << (it - v.begin()) << std::endl;  // 1
    
//     // 可以利用回傳值繼續操作
//     v.insert(it, 99);  // 在 100 之前再插入 99
    
//     for (int x : v) {
//         std::cout << x << " ";  // 1 99 100 2 3
//     }
//     std::cout << std::endl;
    
//     return 0;
// }
// ```

// ---

// ### 三、插入多個相同元素

// ```cpp
// #include <vector>
// #include <iostream>

// int main() {
//     std::vector<int> v = {1, 2, 3};
    
//     // 在位置 1 插入 4 個 0
//     v.insert(v.begin() + 1, 4, 0);
    
//     for (int x : v) {
//         std::cout << x << " ";  // 1 0 0 0 0 2 3
//     }
//     std::cout << std::endl;
    
//     return 0;
// }
// ```

// ---

// ### 四、插入一個範圍的元素

// ```cpp
// #include <vector>
// #include <iostream>
// #include <array>

// int main() {
//     std::vector<int> v = {1, 2, 3};
    
//     // 從另一個容器插入範圍
//     std::vector<int> source = {10, 20, 30};
//     v.insert(v.begin() + 1, source.begin(), source.end());
    
//     std::cout << "插入 vector 範圍後: ";
//     for (int x : v) {
//         std::cout << x << " ";  // 1 10 20 30 2 3
//     }
//     std::cout << std::endl;
    
//     // 從陣列插入
//     int arr[] = {100, 200};
//     v.insert(v.end(), std::begin(arr), std::end(arr));
    
//     std::cout << "插入陣列後: ";
//     for (int x : v) {
//         std::cout << x << " ";  // 1 10 20 30 2 3 100 200
//     }
//     std::cout << std::endl;
    
//     return 0;
// }
// ```

// ---

// ### 五、插入初始化串列

// ```cpp
// #include <vector>
// #include <iostream>

// int main() {
//     std::vector<int> v = {1, 2, 3};
    
//     // 使用初始化串列插入多個元素
//     v.insert(v.begin() + 1, {10, 20, 30});
    
//     for (int x : v) {
//         std::cout << x << " ";  // 1 10 20 30 2 3
//     }
//     std::cout << std::endl;
    
//     return 0;
// }
// ```

// ---

// ### 六、emplace：就地建構插入

// 類似 `emplace_back`，`emplace` 可以直接在指定位置建構物件：

// ```cpp
// #include <vector>
// #include <iostream>
// #include <string>

// struct Person {
//     std::string name;
//     int age;
    
//     Person(const std::string& n, int a) : name(n), age(a) {
//         std::cout << "建構 " << name << std::endl;
//     }
    
//     Person(const Person& other) : name(other.name), age(other.age) {
//         std::cout << "複製 " << name << std::endl;
//     }
    
//     Person(Person&& other) noexcept 
//         : name(std::move(other.name)), age(other.age) {
//         std::cout << "移動 " << name << std::endl;
//     }
// };

// int main() {
//     std::vector<Person> people;
//     people.reserve(5);
    
//     people.emplace_back("Alice", 30);
//     people.emplace_back("Charlie", 35);
    
//     std::cout << "\n=== 使用 insert ===" << std::endl;
//     people.insert(people.begin() + 1, Person("Bob", 25));
    
//     std::cout << "\n=== 使用 emplace ===" << std::endl;
//     people.emplace(people.begin() + 1, "David", 28);
    
//     std::cout << "\n=== 最終結果 ===" << std::endl;
//     for (const auto& p : people) {
//         std::cout << p.name << " (" << p.age << ")" << std::endl;
//     }
    
//     return 0;
// }
// ```

// 輸出：
// ```
// 建構 Alice
// 建構 Charlie

// === 使用 insert ===
// 建構 Bob
// 移動 Bob
// 移動 Charlie
// 銷毀 Bob

// === 使用 emplace ===
// 建構 David
// 移動 Bob
// 移動 Charlie

// === 最終結果 ===
// Alice (30)
// David (28)
// Bob (25)
// Charlie (35)
// ```

// 可以看到 `emplace` 省略了臨時物件的建構和銷毀。

// ---

// ### 七、插入操作的效能代價

// 插入操作需要搬移後續所有元素：

// ```cpp
// #include <vector>
// #include <iostream>
// #include <chrono>

// int main() {
//     const int N = 100000;
    
//     // 測試在開頭插入
//     auto start1 = std::chrono::high_resolution_clock::now();
//     {
//         std::vector<int> v;
//         v.reserve(N);
//         for (int i = 0; i < N; ++i) {
//             v.insert(v.begin(), i);  // 每次都要搬移所有元素
//         }
//     }
//     auto end1 = std::chrono::high_resolution_clock::now();
    
//     // 測試在結尾插入
//     auto start2 = std::chrono::high_resolution_clock::now();
//     {
//         std::vector<int> v;
//         v.reserve(N);
//         for (int i = 0; i < N; ++i) {
//             v.push_back(i);  // 不需要搬移
//         }
//     }
//     auto end2 = std::chrono::high_resolution_clock::now();
    
//     auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);
//     auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);
    
//     std::cout << "在開頭插入 " << N << " 次: " << duration1.count() << " ms" << std::endl;
//     std::cout << "在結尾插入 " << N << " 次: " << duration2.count() << " ms" << std::endl;
    
//     return 0;
// }
// ```

// 結果會顯示開頭插入慢非常多（O(n²) vs O(n)）。

// ---

// ### 八、插入時的迭代器失效

// 插入可能導致擴容，使所有迭代器失效；即使不擴容，插入點之後的迭代器也會失效：

// ```cpp
// #include <vector>
// #include <iostream>

// int main() {
//     std::vector<int> v = {1, 2, 3, 4, 5};
    
//     auto it = v.begin() + 2;  // 指向 3
//     std::cout << "插入前 *it = " << *it << std::endl;  // 3
    
//     // 在 it 之前插入元素
//     auto new_it = v.insert(it, 100);
    
//     // it 現在已失效！不應該再使用
//     // std::cout << *it << std::endl;  // 未定義行為
    
//     // 應該使用 insert 回傳的新迭代器
//     std::cout << "new_it 指向: " << *new_it << std::endl;  // 100
//     std::cout << "new_it + 1 指向: " << *(new_it + 1) << std::endl;  // 3
    
//     return 0;
// }
// ```

// ---

// ### 九、安全的連續插入

// 如果需要連續插入多個元素，要注意迭代器失效問題：

// ```cpp
// #include <vector>
// #include <iostream>

// int main() {
//     std::vector<int> v = {1, 2, 3, 4, 5};
    
//     // 錯誤示範：迭代器失效
//     /*
//     auto it = v.begin() + 2;
//     v.insert(it, 10);
//     v.insert(it, 20);  // it 已失效！未定義行為
//     */
    
//     // 正確做法 1：使用回傳值更新迭代器
//     auto it = v.begin() + 2;
//     it = v.insert(it, 10);
//     it = v.insert(it, 20);  // 在 10 之前插入 20
    
//     std::cout << "做法 1: ";
//     for (int x : v) {
//         std::cout << x << " ";  // 1 2 20 10 3 4 5
//     }
//     std::cout << std::endl;
    
//     // 正確做法 2：使用索引
//     v = {1, 2, 3, 4, 5};
//     size_t index = 2;
//     v.insert(v.begin() + index, 10);
//     ++index;  // 更新索引
//     v.insert(v.begin() + index, 20);
    
//     std::cout << "做法 2: ";
//     for (int x : v) {
//         std::cout << x << " ";  // 1 2 10 20 3 4 5
//     }
//     std::cout << std::endl;
    
//     // 正確做法 3：一次插入多個（最高效）
//     v = {1, 2, 3, 4, 5};
//     v.insert(v.begin() + 2, {10, 20, 30});
    
//     std::cout << "做法 3: ";
//     for (int x : v) {
//         std::cout << x << " ";  // 1 2 10 20 30 3 4 5
//     }
//     std::cout << std::endl;
    
//     return 0;
// }
// ```

// ---

// ### 十、實際應用場景

// #### 場景一：維持排序插入

// ```cpp
// #include <vector>
// #include <iostream>
// #include <algorithm>

// void sorted_insert(std::vector<int>& v, int value) {
//     // 找到第一個大於等於 value 的位置
//     auto pos = std::lower_bound(v.begin(), v.end(), value);
//     v.insert(pos, value);
// }

// int main() {
//     std::vector<int> v = {1, 3, 5, 7, 9};
    
//     sorted_insert(v, 4);
//     sorted_insert(v, 0);
//     sorted_insert(v, 10);
    
//     for (int x : v) {
//         std::cout << x << " ";  // 0 1 3 4 5 7 9 10
//     }
//     std::cout << std::endl;
    
//     return 0;
// }
// ```

// #### 場景二：在特定條件處插入

// ```cpp
// #include <vector>
// #include <iostream>
// #include <string>

// int main() {
//     std::vector<std::string> lines = {
//         "Line 1",
//         "Line 2",
//         "// INSERT HERE",
//         "Line 3",
//         "Line 4"
//     };
    
//     // 找到標記並插入新內容
//     for (auto it = lines.begin(); it != lines.end(); ++it) {
//         if (*it == "// INSERT HERE") {
//             it = lines.insert(it, "New Content");
//             ++it;  // 跳過剛插入的元素，指向標記
//             it = lines.erase(it);  // 刪除標記
//             --it;  // 調整迭代器，因為 erase 後 it 指向下一個元素
//             break;
//         }
//     }
    
//     for (const auto& line : lines) {
//         std::cout << line << std::endl;
//     }
    
//     return 0;
// }
// ```

// #### 場景三：合併兩個已排序的 vector

// ```cpp
// #include <vector>
// #include <iostream>

// std::vector<int> merge_sorted(const std::vector<int>& a, const std::vector<int>& b) {
//     std::vector<int> result;
//     result.reserve(a.size() + b.size());
    
//     size_t i = 0, j = 0;
//     while (i < a.size() && j < b.size()) {
//         if (a[i] <= b[j]) {
//             result.push_back(a[i++]);
//         } else {
//             result.push_back(b[j++]);
//         }
//     }
    
//     // 插入剩餘元素
//     result.insert(result.end(), a.begin() + i, a.end());
//     result.insert(result.end(), b.begin() + j, b.end());
    
//     return result;
// }

// int main() {
//     std::vector<int> a = {1, 3, 5, 7};
//     std::vector<int> b = {2, 4, 6, 8, 9, 10};
    
//     auto merged = merge_sorted(a, b);
    
//     for (int x : merged) {
//         std::cout << x << " ";  // 1 2 3 4 5 6 7 8 9 10
//     }
//     std::cout << std::endl;
    
//     return 0;
// }
// ```

// ---

// ### insert 各種重載版本一覽

// | 語法 | 說明 | 回傳值 |
// |------|------|--------|
// | `insert(pos, value)` | 在 pos 前插入 value | 指向新元素的迭代器 |
// | `insert(pos, count, value)` | 在 pos 前插入 count 個 value | 指向第一個新元素的迭代器 |
// | `insert(pos, first, last)` | 在 pos 前插入 [first, last) 範圍 | 指向第一個新元素的迭代器 |
// | `insert(pos, init_list)` | 在 pos 前插入初始化串列 | 指向第一個新元素的迭代器 |
// | `emplace(pos, args...)` | 在 pos 前就地建構 | 指向新元素的迭代器 |

// ---

// ### 效能特性

// | 操作位置 | 時間複雜度 | 說明 |
// |----------|------------|------|
// | 尾端 | O(1) 攤銷 | 等同 push_back |
// | 開頭 | O(n) | 需搬移所有元素 |
// | 中間 | O(n) | 需搬移後續元素 |

// ---

// ### 練習題

// 1. **實作題**：寫一個函數 `insert_unique`，只有當元素不存在時才插入到已排序的 vector 中，保持排序順序。
//    ```cpp
//    bool insert_unique(std::vector<int>& v, int value);
//    // 回傳 true 表示成功插入，false 表示元素已存在
//    ```

// 2. **預測題**：以下程式碼執行後 v 的內容是什麼？
//    ```cpp
//    std::vector<int> v = {1, 2, 3};
//    auto it = v.insert(v.begin(), {10, 20});
//    v.insert(it + 1, 100);
//    ```

// 3. **思考題**：如果需要頻繁在中間位置插入元素，vector 是好的選擇嗎？應該考慮什麼替代容器？

// 4. **改進題**：以下程式碼效能不佳，請改進：
//    ```cpp
//    std::vector<int> v;
//    for (int i = 0; i < 1000; ++i) {
//        v.insert(v.begin(), i);
//    }
//    ```

// ---

// 下一課我們講 **vector 元素刪除：pop_back、erase、clear**，學習各種移除元素的方式及其效能特性。

// 準備好繼續嗎？


#include <vector>
#include <iostream>

std::vector<int> merge_sorted(const std::vector<int>& a, const std::vector<int>& b) {
    std::vector<int> result;
    result.reserve(a.size() + b.size());
    
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] <= b[j]) {
            result.push_back(a[i++]);
        } else {
            result.push_back(b[j++]);
        }
    }
    
    // 插入剩餘元素
    result.insert(result.end(), a.begin() + i, a.end());
    result.insert(result.end(), b.begin() + j, b.end());
    
    return result;
}

int main() {
    std::vector<int> a = {1, 3, 5, 7};
    std::vector<int> b = {2, 4, 6, 8, 9, 10};
    
    auto merged = merge_sorted(a, b);
    
    for (int x : merged) {
        std::cout << x << " ";  // 1 2 3 4 5 6 7 8 9 10
    }
    std::cout << std::endl;
    
    return 0;
}
