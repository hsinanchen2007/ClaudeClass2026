// =============================================================================
//  第 14 課：vector 元素插入：insert、emplace12.cpp  —  本課完整講義＋可執行範例
// =============================================================================
//
// 【主題資訊 Information】
//   iterator insert(const_iterator pos, const T& value);              // 複製插入
//   iterator insert(const_iterator pos, T&& value);                   // 移動插入 C++11
//   iterator insert(const_iterator pos, size_type n, const T& value); // 插入 n 份
//   template<class InputIt>
//   iterator insert(const_iterator pos, InputIt first, InputIt last); // 插入範圍
//   iterator insert(const_iterator pos, std::initializer_list<T> il); // C++11
//   template<class... Args>
//   iterator emplace(const_iterator pos, Args&&... args);             // 就地建構 C++11
//
//   回傳：指向第一個被插入元素的 iterator；沒插入任何元素時回傳 pos（本機實測）。
//   標頭：<vector>
//   複雜度：O(插入個數 + end() - pos)。尾端插入攤銷 O(1)，中間／開頭 O(n)。
//
// 【詳細解釋 Explanation】
//   ★ 本檔以下（「# 第二階段」起）是本課的完整講義本體，包含逐段說明與大量範例，
//     請直接往下閱讀。此處只補三個「讀講義時要一起記住」的關鍵框架：
//
//   【1. 為什麼插入很貴】vector 承諾元素連續，插入必須把 pos 之後的元素整段後移，
//      搬移量 = end() - pos。這是「連續」這個承諾的必然代價，不是實作偷懶。
//   【2. 為什麼插在 pos「之前」】STL 用半開區間 [first, last)。定義成「之前」，
//      insert(end(), x) 才能自然表示附加到尾端，也才有辦法插到最前面。
//   【3. insert vs emplace 的關鍵不是速度，是初始化語意】
//      insert 走複製初始化（需可隱式轉換），emplace 走直接初始化
//      （完美轉發給建構子，可呼叫 explicit 建構子、且不做 narrowing 檢查）。
//
// 【概念補充 Concept Deep Dive】
//   容量不足時，一次插入其實做了：配置更大空間 → 搬 [begin, pos) → 就地建構新元素
//   → 搬 [pos, end) → 銷毀舊元素並釋放。其中「搬」是 move 還是 copy，取決於 T 的
//   移動建構子是否為 noexcept（std::move_if_noexcept）：若移動可能拋例外，
//   vector 為維持**強例外保證**會退化成複製 —— 這就是「移動建構子請標 noexcept」
//   的真正理由。成長倍率**是實作定義**（libstdc++ 實測 2×、MSVC 1.5×），
//   標準只規定 push_back 為攤銷 O(1)。
//
// 【注意事項 Pay Attention】
//   1. iterator 失效：有 reallocation → 全部失效；沒有 → 插入點及其之後失效。
//      不要賭，插入後一律用 insert 的回傳值重新取得 iterator。
//   2. pos 必須是「這個容器」的有效 iterator，傳別的容器的是 UB，
//      標準不保證任何特定症狀：可能當場崩潰，也可能安靜跑出錯誤結果。
//   3. 插入 0 個元素（n==0 或空範圍）合法，回傳 pos、容器不變。
//   4. 迴圈裡逐筆 insert 到前面是 O(n²)；改用範圍版一次插入是 O(n+m)。
//
// =============================================================================

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


// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector::insert / emplace
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. insert 回傳什麼？為什麼要有回傳值？
//     答：回傳指向「第一個被插入元素」的 iterator；若一個都沒插入
//         （n == 0 或空範圍），回傳傳進去的 pos。
//         有回傳值是因為插入可能使舊 iterator 失效，你需要一個保證有效的新 iterator
//         才能接著操作。
//     追問：什麼時候舊 iterator 會失效？→ 發生 reallocation 時全部失效；
//           沒有 reallocation 時，插入點及其之後失效。
//
// 🔥 Q2. 為什麼 result.insert(result.end(), a.begin()+i, a.end()) 比
//        逐個 push_back 剩餘元素好？
//     答：範圍版 insert 可以一次算出要插入幾個元素、必要時只做一次容量調整，
//         並用單次搬移把整段放進去；逐筆 push_back 則要反覆檢查容量。
//         本檔已先 reserve，兩者差距縮小，但範圍版語意更清楚也更不易寫錯。
//     追問：那範圍版對 input iterator（例如從 istream 讀）也能一次算出個數嗎？
//           → 不行。只有 forward iterator 以上能先算距離；input iterator
//             只能邊讀邊插，退化成逐筆成長。
//
// ⚠️ 陷阱. 下面這段想「把 src 全部插到 v 最前面」，複雜度是多少？
//         for (int x : src) v.insert(v.begin(), x);
//     答：O(n²)，而且順序還會被反轉。每次插在 begin() 都要把現有元素整段後移，
//         總搬移量是 1+2+…+n。正解是 v.insert(v.begin(), src.begin(), src.end())，
//         O(n+m) 且順序正確。
//     為什麼會錯：多數人記得「單次 insert 是 O(n)」，卻把迴圈裡的 n 當常數，
//         忘了它每輪都在變大，於是把 O(n²) 誤算成 O(n)；順序反轉則是完全沒想到。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <string>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 88. Merge Sorted Array
//   題目：nums1 長度為 m+n（後 n 格是預留的 0），nums2 有 n 個元素，
//         要把 nums2 併進 nums1 並保持排序，**就地**完成。
//   為什麼用到本主題：這題是「插入」的反面教材 —— 正解刻意**不用** insert。
//         若從前面插入，每次都要後移元素，是 O(m*n)；
//         改成從**後往前**填，利用尾端預留空間，一次掃描 O(m+n) 完成。
//         理解「為什麼避開中間插入」正是本課的重點。
// -----------------------------------------------------------------------------
void mergeSortedArray(std::vector<int>& nums1, int m, const std::vector<int>& nums2, int n) {
    int i = m - 1, j = n - 1, k = m + n - 1;
    while (j >= 0) {
        if (i >= 0 && nums1[i] > nums2[j]) {
            nums1[k--] = nums1[i--];
        } else {
            nums1[k--] = nums2[j--];
        }
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】設定檔區段插入：在 [server] 段落末尾補上預設鍵值
//   情境：載入 nginx/ini 風格設定檔後，若某個區段缺少必要鍵，
//         要在**該區段結尾**補上預設值，而不是塞到檔案最後（會跑進別的區段）。
//   為什麼用到本主題：這是範圍版 insert 的典型用途 ——
//         先定位區段邊界，再把一整批預設行一次插進去。
// -----------------------------------------------------------------------------
std::vector<std::string> fillSectionDefaults(std::vector<std::string> lines,
                                             const std::string& section,
                                             const std::vector<std::string>& defaults) {
    // 找到區段起點
    size_t start = lines.size();
    for (size_t i = 0; i < lines.size(); ++i) {
        if (lines[i] == section) { start = i + 1; break; }
    }
    if (start > lines.size()) return lines;

    // 從區段起點往後找到下一個區段標頭，即為本區段結尾
    size_t end = start;
    while (end < lines.size() && !lines[end].empty() && lines[end][0] != '[') ++end;

    // 一次把所有缺少的預設值插進區段結尾（範圍版 insert，只搬一次）
    lines.insert(lines.begin() + static_cast<long>(end), defaults.begin(), defaults.end());
    return lines;
}

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
    std::cout << "=== 合併兩個已排序 vector（範圍版 insert 收尾）===" << std::endl;
    std::vector<int> a = {1, 3, 5, 7};
    std::vector<int> b = {2, 4, 6, 8, 9, 10};

    auto merged = merge_sorted(a, b);

    std::cout << "  ";
    for (int x : merged) {
        std::cout << x << " ";
    }
    std::cout << std::endl;

    std::cout << "\n=== LeetCode 88. Merge Sorted Array ===" << std::endl;
    std::vector<int> nums1 = {1, 2, 3, 0, 0, 0};
    std::vector<int> nums2 = {2, 5, 6};
    mergeSortedArray(nums1, 3, nums2, 3);
    std::cout << "  [1,2,3,0,0,0] + [2,5,6] -> ";
    for (int x : nums1) std::cout << x << " ";
    std::cout << std::endl;

    std::vector<int> only = {0};
    std::vector<int> src = {1};
    mergeSortedArray(only, 0, src, 1);
    std::cout << "  m=0 的邊界情形 -> ";
    for (int x : only) std::cout << x << " ";
    std::cout << std::endl;

    std::cout << "\n=== 日常實務：設定檔區段補預設值 ===" << std::endl;
    std::vector<std::string> conf = {
        "[server]", "listen=8080", "", "[logging]", "level=info"
    };
    auto filled = fillSectionDefaults(conf, "[server]",
                                      {"workers=4", "keepalive=75"});
    for (const auto& line : filled) {
        std::cout << "  " << (line.empty() ? "(空行)" : line) << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 14 課：vector 元素插入：insert、emplace12.cpp" -o insert_emplace12

// === 預期輸出 ===
// === 合併兩個已排序 vector（範圍版 insert 收尾）===
//   1 2 3 4 5 6 7 8 9 10
//
// === LeetCode 88. Merge Sorted Array ===
//   [1,2,3,0,0,0] + [2,5,6] -> 1 2 2 3 5 6
//   m=0 的邊界情形 -> 1
//
// === 日常實務：設定檔區段補預設值 ===
//   [server]
//   listen=8080
//   workers=4
//   keepalive=75
//   (空行)
//   [logging]
//   level=info
