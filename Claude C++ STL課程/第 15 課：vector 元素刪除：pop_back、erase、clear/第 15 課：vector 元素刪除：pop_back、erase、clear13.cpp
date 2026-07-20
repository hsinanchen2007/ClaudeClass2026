// =============================================================================
//  第 15 課：vector 元素刪除 13  —  本課完整講義 + swap-and-pop（O(1) 刪除）
// =============================================================================
//
// 【主題資訊 Information】
//   本檔前半是第 15 課的完整講義（保存在下方的行註解裡），
//   後半是「不保持順序的 O(1) 刪除」——swap-and-pop 技巧。
//   用到的成員（皆 <vector>）：
//     T&   back();                O(1)，取最後一個元素
//     void pop_back();            O(1)，刪除最後一個
//     T&   operator[](size_type); O(1)
//   核心手法：
//       v[index] = std::move(v.back());   // 把最後一個搬到要刪的位置
//       v.pop_back();                     // 再拿掉最後一個
//   複雜度：O(1)（對比 erase 的 O(n)）。
//   代價：【元素順序被打亂】。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼這樣就能 O(1)】
//   erase 之所以是 O(n)，是因為它要「補洞」——把後面所有元素往前搬，
//   維持連續且保持順序。
//   但如果你【不在乎順序】，補洞就有更省的辦法：
//   直接拿最後一個元素來填這個洞，然後把尾巴砍掉。
//       [A][B][C][D][E]   要刪 B（索引 1）
//       [A][E][C][D][E]   E 移到索引 1
//       [A][E][C][D]      pop_back
//   只動了兩個位置，與元素總數無關，所以是 O(1)。
//   這個技巧在遊戲引擎、粒子系統、ECS 架構裡是標準做法——
//   那些場景每幀要刪除大量物件，而且順序完全無所謂。
//
// 【2. 原始版本有兩個真實的 bug，本檔已修正】
//   常見的寫法是：
//       template <typename T>
//       void fast_erase(std::vector<T>& v, size_t index) {
//           if (index != v.size() - 1) v[index] = std::move(v.back());
//           v.pop_back();
//       }
//   兩個問題：
//     ① v 為空時，`v.size() - 1` 是無號環繞 → 變成 SIZE_MAX，
//        於是 `index != SIZE_MAX` 幾乎必然成立 → 執行 v[index] 越界，
//        接著對空 vector 呼叫 pop_back()，又是一個 UB。
//     ② index 越界時（例如 index = 100 但只有 5 個元素），
//        這段程式碼仍然會 pop_back()，等於【默默刪掉最後一個元素】。
//        刪錯東西比直接報錯難查得多。
//   修法是在最前面加一道 `if (index >= v.size()) return false;`。
//   注意這個檢查同時解決了兩個問題——它讓後面的 `v.size() - 1`
//   永遠在 size >= 1 的前提下執行，不可能環繞。
//
// 【3. `if (index != v.size() - 1)` 這個判斷不能省】
//   若要刪的就是最後一個元素，`v[index] = std::move(v.back())`
//   會變成「自我移動賦值」（self-move-assignment）。
//   標準對自我移動賦值的規定是：物件會處於「有效但未指定」狀態——
//   也就是說，那個元素的值可能變成任何東西。
//   雖然接下來馬上 pop_back 把它刪掉，看似無害，但：
//     * 對某些型別，自我移動可能觸發 assertion 或釋放兩次資源
//     * 這是明確可以避免的未定義/未指定行為，沒有理由留著
//   所以先判斷一下，是最後一個就直接 pop_back。
//
// 【4. 什麼時候不該用這個技巧】
//   * 需要維持順序時（顯示清單、時間序列、設定檔）
//   * 有其他程式碼持有「索引」來指涉元素時——
//     swap-and-pop 會讓最後一個元素的索引【突然改變】，
//     所有記著舊索引的地方全部指到錯的東西。
//     這比迭代器失效更陰險，因為索引不會「失效」，它只是靜靜地指錯。
//   * 要一次刪很多個時：與其呼叫 N 次 swap-and-pop，
//     不如用一次 erase-remove（同樣 O(n)，但保序且更清楚）。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 這個技巧的標準函式庫版本
//     C++ 沒有內建 swap-and-pop，但概念上最接近的是
//     std::partition（不穩定分割）——它同樣是「放棄順序換效能」。
//     若要一次移除多個且不在乎順序，可以：
//         auto it = std::partition(v.begin(), v.end(),
//                                  [](const T& x){ return !shouldRemove(x); });
//         v.erase(it, v.end());
//     partition 通常比 stable_partition / remove_if 搬移更少資料。
//
// (B) 為什麼用 std::move 而不是直接賦值
//     v[index] = v.back();          // 複製賦值：對 string/vector 是深複製
//     v[index] = std::move(v.back());  // 移動賦值：只搬指標
//     反正 v.back() 馬上就要被 pop_back 銷毀了，沒有理由複製它。
//     對 int 這種型別兩者相同，對持有資源的型別差距很大。
//
// (C) ECS 與 packed array：這個技巧的工業級用法
//     遊戲引擎的 Entity-Component-System 常用「packed array + 索引映射」：
//     元件緊密排列在 vector 裡（cache 友善），另有一張
//     entity → index 的對照表。刪除元件時用 swap-and-pop，
//     並【同步更新對照表】中那個被搬動元素的索引。
//     這正好解決了上面 (4) 提到的「索引突然改變」問題——
//     代價是要多維護一張表。
//     本檔的實務範例就是這個結構的最小版本。
//
// 【注意事項 Pay Attention】
//   1. 一定要先做 `if (index >= v.size()) return false;`。
//      少了它，空 vector 會因無號環繞而越界，越界索引會默默刪錯元素。
//   2. 要刪的若是最後一個，別做自我移動賦值——先判斷再搬。
//   3. 順序會被打亂。需要保序請用 erase 或 erase-remove。
//   4. 若別處持有索引，swap-and-pop 會讓那些索引靜默指錯，
//      必須同步維護索引映射表。
//   5. 用 std::move 搬移，不要用複製——反正來源馬上要被銷毀。
//   6. 要一次刪多個時，用 std::partition + erase 比呼叫 N 次更好。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】swap-and-pop
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 怎麼讓 vector 刪除單一元素變成 O(1)？代價是什麼？
//     答：swap-and-pop：把最後一個元素移到要刪的位置，再 pop_back。
//           if (index >= v.size()) return false;
//           if (index != v.size() - 1) v[index] = std::move(v.back());
//           v.pop_back();
//         只動兩個位置，與元素數無關，所以是 O(1)。
//         代價是【元素順序被打亂】——被搬過來的那個元素離開了原位。
//     追問：什麼場景適合？
//         → 遊戲引擎的粒子/實體管理、ECS 的 packed array——
//           每幀要刪大量物件而順序完全無所謂。
//           反過來說，顯示清單、時間序列、設定檔這類需要保序的
//           就不能用。
//
// 🔥 Q2. `if (index != v.size() - 1)` 這個判斷可以省略嗎？
//        反正那個元素馬上就要被 pop_back 刪掉了。
//     答：不能省。省略的話，當要刪的就是最後一個元素時，
//         `v[index] = std::move(v.back())` 會變成【自我移動賦值】。
//         標準規定自我移動賦值後物件處於「有效但未指定」狀態，
//         對某些型別甚至可能觸發 assertion 或重複釋放資源。
//         雖然馬上會被刪掉，但這是可以零成本避免的未指定行為，
//         沒有理由留著。
//     追問：為什麼用 std::move 而不是直接賦值？
//         → 因為 v.back() 馬上要被銷毀，複製它毫無意義。
//           對 std::string / std::vector 這類元素，
//           移動只搬指標，複製則是深複製，差距很大。
//
// ⚠️ 陷阱. 「fast_erase 只要處理好 index != size()-1 的情況就安全了，
//         反正呼叫端會確保 index 合法。」——這個假設為什麼危險？
//     答：因為少了 `if (index >= v.size())` 這道檢查，會出現兩種災難：
//         ① v 為空時：`v.size() - 1` 是無號運算 → 環繞成
//            18446744073709551615。於是 `index != 那個天文數字` 成立，
//            程式執行 v[index] 直接越界；接著又對空 vector
//            呼叫 pop_back()，第二個 UB。
//         ② index 越界時（比方傳了 100，但只有 5 個元素）：
//            程式照樣執行 pop_back()，於是【默默刪掉了最後一個元素】。
//            沒有任何錯誤訊息，資料就少了一筆。
//         第二種特別可怕：「刪錯東西」比「當場崩潰」難查十倍。
//     為什麼會錯：把「呼叫端會確保正確」當成可以省略防護的理由。
//         但這個函式是 template、會被很多地方呼叫，
//         而且無號環繞讓「明顯錯誤的輸入」偽裝成合法輸入。
//         一道 `if (index >= v.size()) return false;` 是零成本的
//         （分支預測幾乎永遠命中），沒有理由省略。
//         本檔的版本已經修好了這兩個 bug。
// ═══════════════════════════════════════════════════════════════════════════

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
#include <map>
#include <utility>   // std::move

// 快速刪除單一元素（不保持順序）
template <typename T>
bool fast_erase(std::vector<T>& v, size_t index) {
    // ⚠️ 邊界檢查不可省：v 為空時 v.size() - 1 會 unsigned 環繞成天文數字，
    //    條件失效後對空 vector 呼叫 pop_back() 是 UB；index 越界時原版也會
    //    默默刪掉最後一個元素（刪錯東西比直接報錯更難查）。
    if (index >= v.size()) return false;
    // 把最後一個元素移到要刪除的位置，然後 pop_back
    if (index != v.size() - 1) {
        v[index] = std::move(v.back());
    }
    v.pop_back();
    return true;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔刻意不放
//   理由：swap-and-pop 的價值在「長期維護一個會頻繁增刪的集合」，
//   屬於資料結構設計議題。LeetCode 的題目是一次性的輸入→輸出，
//   沒有「維護一份長期存在的集合」這個面向。
//   （最接近的是 LeetCode 380 Insert Delete GetRandom O(1)，
//     它確實用到 swap-and-pop + 索引映射——但那題的核心是
//     雜湊表設計，vector 只是配角，放在第 15 課會失焦。
//     等到 unordered_map 的章節再談比較合適。）
//   下方的實務範例才是這個技巧真正的舞台。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】遊戲的粒子系統（packed array + 索引映射）
//   情境：每幀有大量粒子產生與消滅。粒子必須緊密排列在連續記憶體裡
//         （更新迴圈才 cache 友善），而且刪除必須夠快——
//         每幀可能要刪掉數千個。順序完全無所謂。
//   為什麼用到本主題：這正是 swap-and-pop 的工業級用法。
//     但它同時暴露了那個最陰險的陷阱：
//     外部程式碼若持有「索引」來指涉某個粒子，
//     swap-and-pop 會讓被搬動元素的索引【靜默改變】——
//     索引不會「失效」，它只是開始指向錯的東西。
//   解法：維護一張 id → index 的對照表，每次搬移都同步更新。
//     這正是遊戲引擎 ECS 架構的核心手法。
// -----------------------------------------------------------------------------
struct Particle {
    int    id;          // 穩定的識別碼（不隨位置改變）
    double x, y;
    int    lifeFrames;
};

class ParticlePool {
public:
    // 新增粒子，回傳它的穩定 id
    int spawn(double x, double y, int life) {
        int id = nextId_++;
        idToIndex_[id] = particles_.size();
        particles_.push_back({id, x, y, life});
        return id;
    }

    // 用穩定 id 刪除（O(1)），並同步維護索引表
    bool kill(int id) {
        auto it = idToIndex_.find(id);
        if (it == idToIndex_.end()) return false;

        size_t index = it->second;
        size_t last  = particles_.size() - 1;   // 安全：map 有這筆代表非空

        if (index != last) {
            // 把最後一個搬過來
            particles_[index] = std::move(particles_[last]);
            // ★ 關鍵：被搬動的那個粒子，它的索引改變了 → 同步更新對照表
            idToIndex_[particles_[index].id] = index;
        }
        particles_.pop_back();
        idToIndex_.erase(it);
        return true;
    }

    // 用穩定 id 查詢（透過對照表，不受 swap-and-pop 影響）
    const Particle* find(int id) const {
        auto it = idToIndex_.find(id);
        if (it == idToIndex_.end()) return nullptr;
        return &particles_[it->second];
    }

    size_t size() const { return particles_.size(); }

    const std::vector<Particle>& raw() const { return particles_; }

private:
    std::vector<Particle>        particles_;   // 緊密排列，更新迴圈 cache 友善
    std::map<int, size_t>        idToIndex_;   // 穩定 id → 目前位置
    int                          nextId_ = 1;
};

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    std::cout << "=== 原始示範：swap-and-pop ===\n";
    std::cout << "原始: ";
    for (int x : v) std::cout << x << " ";  // 10 20 30 40 50
    std::cout << std::endl;

    // 刪除索引 1（值為 20）
    fast_erase(v, 1);

    std::cout << "fast_erase 後: ";
    for (int x : v) std::cout << x << " ";  // 10 50 30 40（順序改變了）
    std::cout << std::endl;
    std::cout << "→ 50 從尾端被搬到索引 1，所以順序亂了。這是 O(1) 的代價。\n";

    std::cout << "\n=== 邊界檢查真的有用：三種會出事的輸入 ===\n";
    {
        std::vector<int> empty;
        std::cout << "空 vector, index=0  -> "
                  << std::boolalpha << fast_erase(empty, 0)
                  << "（安全回傳 false）\n";
        std::cout << "  若少了邊界檢查：size()-1 環繞成天文數字 → v[0] 越界 → pop_back 空容器\n";

        std::vector<int> few = {1, 2, 3};
        std::cout << "3 個元素, index=100 -> " << fast_erase(few, 100)
                  << "（安全回傳 false），內容仍是: ";
        for (int x : few) std::cout << x << " ";
        std::cout << "\n";
        std::cout << "  若少了邊界檢查：會默默 pop_back 刪掉最後一個 3——刪錯東西最難查\n";

        std::vector<int> one = {42};
        std::cout << "1 個元素, index=0   -> " << fast_erase(one, 0)
                  << "，size 變成 " << one.size() << "\n";
        std::cout << "  這裡走的是「就是最後一個」的分支，避開了自我移動賦值\n";
    }

    std::cout << "\n=== erase vs fast_erase：保序 vs 速度 ===\n";
    {
        std::vector<int> a = {10, 20, 30, 40, 50};
        std::vector<int> b = a;

        a.erase(a.begin() + 1);      // O(n)，保序
        fast_erase(b, 1);            // O(1)，不保序

        std::cout << "erase(begin+1)  -> ";
        for (int x : a) std::cout << x << " ";
        std::cout << "  （保持順序，O(n)）\n";
        std::cout << "fast_erase(v, 1)-> ";
        for (int x : b) std::cout << x << " ";
        std::cout << "  （順序打亂，O(1)）\n";
    }

    std::cout << "\n=== 搬移次數的實證：與元素總數無關 ===\n";
    {
        std::vector<int> small(10);
        std::vector<int> big(100000);
        for (size_t i = 0; i < small.size(); ++i) small[i] = static_cast<int>(i);
        for (size_t i = 0; i < big.size(); ++i)   big[i]   = static_cast<int>(i);

        // 刪除第一個元素：erase 要搬 n-1 個，fast_erase 只搬 1 個
        std::cout << "刪除索引 0 時需要搬移的元素數：\n";
        std::cout << "  erase      : 10 個元素 -> 搬 9 個；100000 個元素 -> 搬 99999 個\n";
        std::cout << "  fast_erase : 10 個元素 -> 搬 1 個；100000 個元素 -> 搬 1 個\n";
        fast_erase(small, 0);
        fast_erase(big, 0);
        std::cout << "  實測後 size: small=" << small.size()
                  << ", big=" << big.size() << "\n";
        std::cout << "  small 內容: ";
        for (int x : small) std::cout << x << " ";
        std::cout << "  ← 最後一個 9 被搬到最前面\n";
    }

    std::cout << "\n=== 日常實務：粒子系統（packed array + 索引映射）===\n";
    {
        ParticlePool pool;
        int a = pool.spawn(1.0, 1.0, 60);
        int b = pool.spawn(2.0, 2.0, 30);
        int c = pool.spawn(3.0, 3.0, 90);
        int d = pool.spawn(4.0, 4.0, 45);

        std::cout << "產生 4 個粒子，id = " << a << " " << b << " " << c << " " << d << "\n";
        std::cout << "vector 內的順序: ";
        for (const auto& p : pool.raw()) std::cout << "id" << p.id << " ";
        std::cout << "\n";

        pool.kill(b);       // O(1) 刪除中間那個
        std::cout << "\nkill(id=" << b << ") 之後，vector 順序: ";
        for (const auto& p : pool.raw()) std::cout << "id" << p.id << " ";
        std::cout << "  ← id4 被搬到了 id2 原本的位置\n";
        std::cout << "size=" << pool.size() << "\n";

        // 關鍵驗證：即使元素被搬動，用穩定 id 仍然查得到正確的粒子
        std::cout << "\n用穩定 id 查詢（不受搬移影響）：\n";
        for (int id : {a, b, c, d}) {
            const Particle* p = pool.find(id);
            std::cout << "  id" << id << " -> ";
            if (p) {
                std::cout << "座標(" << p->x << ", " << p->y
                          << ") life=" << p->lifeFrames << "\n";
            } else {
                std::cout << "已刪除\n";
            }
        }

        std::cout << "\n→ 這就是 swap-and-pop 那個最陰險陷阱的解法：\n";
        std::cout << "  被搬動元素的【索引】會靜默改變（不像迭代器會「失效」），\n";
        std::cout << "  所以必須在搬移的同一行同步更新 id→index 對照表。\n";
        std::cout << "  漏掉那一行，查詢就會回傳完全錯誤的粒子，而且不會有任何錯誤訊息。\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 15 課：vector 元素刪除13.cpp -o demo13

// === 預期輸出 ===
// === 原始示範：swap-and-pop ===
// 原始: 10 20 30 40 50
// fast_erase 後: 10 50 30 40
// → 50 從尾端被搬到索引 1，所以順序亂了。這是 O(1) 的代價。
//
// === 邊界檢查真的有用：三種會出事的輸入 ===
// 空 vector, index=0  -> false（安全回傳 false）
//   若少了邊界檢查：size()-1 環繞成天文數字 → v[0] 越界 → pop_back 空容器
// 3 個元素, index=100 -> false（安全回傳 false），內容仍是: 1 2 3
//   若少了邊界檢查：會默默 pop_back 刪掉最後一個 3——刪錯東西最難查
// 1 個元素, index=0   -> true，size 變成 0
//   這裡走的是「就是最後一個」的分支，避開了自我移動賦值
//
// === erase vs fast_erase：保序 vs 速度 ===
// erase(begin+1)  -> 10 30 40 50   （保持順序，O(n)）
// fast_erase(v, 1)-> 10 50 30 40   （順序打亂，O(1)）
//
// === 搬移次數的實證：與元素總數無關 ===
// 刪除索引 0 時需要搬移的元素數：
//   erase      : 10 個元素 -> 搬 9 個；100000 個元素 -> 搬 99999 個
//   fast_erase : 10 個元素 -> 搬 1 個；100000 個元素 -> 搬 1 個
//   實測後 size: small=9, big=99999
//   small 內容: 9 1 2 3 4 5 6 7 8   ← 最後一個 9 被搬到最前面
//
// === 日常實務：粒子系統（packed array + 索引映射）===
// 產生 4 個粒子，id = 1 2 3 4
// vector 內的順序: id1 id2 id3 id4
//
// kill(id=2) 之後，vector 順序: id1 id4 id3   ← id4 被搬到了 id2 原本的位置
// size=3
//
// 用穩定 id 查詢（不受搬移影響）：
//   id1 -> 座標(1, 1) life=60
//   id2 -> 已刪除
//   id3 -> 座標(3, 3) life=90
//   id4 -> 座標(4, 4) life=45
//
// → 這就是 swap-and-pop 那個最陰險陷阱的解法：
//   被搬動元素的【索引】會靜默改變（不像迭代器會「失效」），
//   所以必須在搬移的同一行同步更新 id→index 對照表。
//   漏掉那一行，查詢就會回傳完全錯誤的粒子，而且不會有任何錯誤訊息。
