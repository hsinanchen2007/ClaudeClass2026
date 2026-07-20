// =============================================================================
//  第 15 課-9：erase-remove 慣用法 —— 為什麼要「兩步」才刪得掉
// =============================================================================
//
// 【主題資訊 Information】
//   template<class ForwardIt, class UnaryPredicate>
//   ForwardIt std::remove_if(ForwardIt first, ForwardIt last, UnaryPredicate p);
//   ForwardIt std::remove   (ForwardIt first, ForwardIt last, const T& value);
//   iterator  vector<T>::erase(const_iterator first, const_iterator last);
//   標準版本：C++98 起；C++20 起可用 std::erase / std::erase_if 一行完成
//   複雜度：remove_if O(n) 且只掃一遍；erase(範圍) O(1)（只砍尾巴）→ 總計 O(n)
//   標頭檔：<algorithm>（remove_if）、<vector>（erase）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 std::remove_if 不會真的刪除元素】
//   這是整個 STL 最常被誤解的一件事。演算法（<algorithm>）只透過
//   「迭代器」操作元素，而迭代器只能讀寫元素、無法改變容器的大小——
//   它根本不知道自己指向的是 vector、deque 還是一段原生陣列，
//   當然也就拿不到 vector 的 size 成員。
//   所以 remove_if 能做的只有「重新排列元素」：把要保留的元素依序
//   搬到前段，然後回傳一個迭代器，告訴你「保留區到哪裡為止」。
//   真正把容器縮短，只有容器自己的成員函式 erase 做得到。
//   這個「演算法與容器分離」正是 STL 的核心設計，
//   代價就是刪除必須拆成兩步。
//
// 【2. remove_if 之後，尾巴那段是什麼】
//   remove_if 內部做的是 move-assign：把後面要保留的元素往前搬。
//   搬完之後，[new_end, end) 這段的元素處於
//   「valid but unspecified state（有效但未指定）」——
//   它們是合法可解構、可賦值的物件，但內容不保證是什麼。
//   注意這和「indeterminate value（不定值）」不同：讀取它們並非 UB，
//   只是你不該對讀到的內容有任何期待。
//   對 int 這種 trivially copyable 型別，move 實際上就是 copy，
//   所以在多數實作上會看到原本的尾端數值殘留；
//   但對 std::string 就可能看到一串空字串——這正是「不可依賴」的意思。
//
// 【3. erase 的區間版把尾巴一次砍掉】
//   v.erase(new_end, v.end()) 才是真正縮短 size 的那一步。
//   因為刪的是尾端，沒有任何元素需要往前搬，這一步是 O(刪除個數) 的
//   解構成本，不含搬移成本。
//   合起來 remove_if + erase 全程只掃一遍、每個元素最多搬一次，總計 O(n)。
//
// 【4. 和「迴圈裡逐一 erase」的複雜度差在哪】
//   逐一 erase：每刪一個都要把後方元素整批往前搬 → 最壞 O(n²)。
//   erase-remove：所有保留元素合計只搬一次 → O(n)。
//   資料量 10 萬、刪掉一半時，前者約 25 億次搬移、後者約 10 萬次。
//   這就是為什麼 Effective STL 把 erase-remove 列為預設寫法。
//
// 【5. remove vs remove_if vs unique】
//   remove(first, last, value)  → 移除「等於某值」的元素
//   remove_if(first, last, pred)→ 移除「滿足條件」的元素
//   unique(first, last)         → 移除「連續重複」的元素（需先排序才能全域去重）
//   三者都是「不真的刪除、回傳新邏輯結尾」，都要搭配 erase 收尾。
//
// 【概念補充 Concept Deep Dive】
//   ▸ remove_if 的內部長相（概念版）
//       ForwardIt result = first;
//       for (; first != last; ++first)
//           if (!p(*first)) *result++ = std::move(*first);
//       return result;
//     這就是經典的「快慢雙指標」：first 是讀指標、result 是寫指標。
//     LeetCode 上大量的「原地移除」題目要你手寫的，正是這幾行。
//   ▸ 為什麼回傳值一定要接住
//     new_end 是唯一能得知「保留區邊界」的方式。丟掉它之後，
//     容器的 size 沒變，你再也分不出哪些是有效資料、哪些是殘留。
//     常見 bug：只寫 std::remove_if(v.begin(), v.end(), pred); 就以為刪好了，
//     結果 v.size() 完全沒變。編譯器通常會給 [[nodiscard]] 相關警告。
//   ▸ C++20 的 std::erase_if
//     std::erase_if(v, pred); 一行等價於整個 erase-remove，
//     而且回傳被刪除的元素個數。新程式碼建議直接用它。
//
// 【注意事項 Pay Attention】
//   1. remove_if 不改變容器大小；不接 erase 就等於什麼都沒刪。
//   2. [new_end, end) 是 valid but unspecified，讀它不是 UB，
//      但內容不可依賴（不同型別、不同實作會不一樣）。
//   3. remove_if 不保證「不刪的元素」原本的相對順序以外的任何事，
//      但它確實是 stable 的：保留元素的相對順序不變。
//   4. 對 list 請改用成員函式 list::remove_if（O(1) 接指標，效率更好）。
//   5. 想全域去重必須先 sort 再 unique，unique 只處理「相鄰」重複。
//   6. C++20 起優先用 std::erase_if(v, pred)。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】erase-remove 慣用法
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::remove_if 為什麼不能真的刪除元素？
//     答：<algorithm> 的演算法只透過迭代器操作元素，而迭代器不持有
//         容器本身，拿不到 size、也呼叫不到容器的成員函式。
//         它只能把要保留的元素往前搬並回傳新的邏輯結尾，
//         真正縮短容器必須由 vector::erase 完成。
//     追問：那 std::list 為什麼有自己的 remove_if 成員函式？
//         → 因為 list 可以只改指標就摘掉節點，成員版是 O(n) 且不搬資料；
//           用通用演算法反而要搬動元素值，浪費效能。
//
// 🔥 Q2. erase-remove 為什麼是 O(n)，而迴圈裡逐一 erase 是 O(n²)？
//     答：逐一 erase 時，每刪一個元素都要把它後面的所有元素往前搬一格，
//         刪 k 個就搬 k 次、每次最多 n 格。
//         erase-remove 則是一次掃描把所有保留元素搬到定位（每個元素最多
//         搬一次），最後對尾端做一次性的區間 erase（不需搬移）。
//     追問：那什麼時候還是得用逐一 erase？
//         → 刪除時還要對被刪元素做額外處理（寫 log、回收資源、通知）時，
//           因為 remove_if 會把被刪元素覆蓋掉，來不及看它們。
//
// ⚠️ 陷阱. auto it = std::remove_if(v.begin(), v.end(), pred);
//          之後直接 for (int x : v) 印出來，看到的是什麼？
//     答：會印出「全部 v.size() 個元素」——因為 remove_if 沒有改變 size。
//         前段是保留下來的元素，後段 [it, v.end()) 是
//         valid but unspecified 的殘留內容。
//         很多人看到後段還印得出「像是舊資料」的東西，就以為 remove_if 壞了。
//     為什麼會錯：把 remove_if 想成了「容器的刪除方法」。
//         它的名字誤導性極強——它做的其實是 partition（分區），
//         叫它 "move_unwanted_to_the_back" 會誠實得多。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 26. Remove Duplicates from Sorted Array
//   題目：已排序陣列原地去重，回傳去重後的長度。
//   為什麼用到本主題：這題和 std::unique + erase 是同一個模式——
//         unique 也是「只重排、回傳新邏輯結尾、不改 size」的演算法，
//         必須配 erase 才真正縮短容器。
// -----------------------------------------------------------------------------
int removeDuplicates(std::vector<int>& nums) {
    auto newEnd = std::unique(nums.begin(), nums.end());
    nums.erase(newEnd, nums.end());     // 少了這行，size 完全不會變
    return static_cast<int>(nums.size());
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 283. Move Zeroes
//   題目：把所有 0 移到陣列尾端，其餘元素保持原有相對順序，且必須原地完成。
//   為什麼用到本主題：這題「不能刪除、只要搬到後面」，正好就是
//         remove 的前半段（partition）而不做 erase。
//         看懂這題就會明白 remove_if 的本質是分區而非刪除。
// -----------------------------------------------------------------------------
void moveZeroes(std::vector<int>& nums) {
    // remove 把非 0 元素穩定地搬到前段，回傳非 0 區的結尾
    auto nonZeroEnd = std::remove(nums.begin(), nums.end(), 0);
    // 這裡「刻意不 erase」，改成把尾段補 0——正是本題要的效果
    std::fill(nonZeroEnd, nums.end(), 0);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】感測器資料清洗：剔除超出量測範圍的無效讀數
//   情境：溫度感測器每秒回報一筆讀數，硬體故障時會回傳 -999 這類
//         哨兵值或明顯超界的數值。進統計之前必須先把它們濾掉。
//   為什麼用本主題：這是最典型的「批量條件刪除」，資料量大、
//         而且被刪的資料不需要另外處理 → erase-remove 是最佳解。
// -----------------------------------------------------------------------------
std::size_t dropInvalidReadings(std::vector<double>& readings,
                                double lo, double hi) {
    std::size_t before = readings.size();
    auto newEnd = std::remove_if(readings.begin(), readings.end(),
                                 [lo, hi](double t) { return t < lo || t > hi; });
    readings.erase(newEnd, readings.end());
    return before - readings.size();   // 回傳被剔除的筆數
}

int main() {
    std::cout << "=== 一、remove_if 只重排、不改變 size ===" << std::endl;
    std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::cout << "原始資料: ";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;

    auto new_end = std::remove_if(v.begin(), v.end(),
                                  [](int x) { return x % 2 == 0; });

    std::cout << "remove_if 後（整個容器）: ";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;
    std::cout << "  ↑ 前 5 個是保留的奇數；後 5 個是 valid but unspecified 的殘留" << std::endl;

    std::cout << "邏輯大小 (new_end - begin): " << (new_end - v.begin()) << std::endl;
    std::cout << "實際大小 (v.size())      : " << v.size() << "  ← 完全沒變！" << std::endl;

    std::cout << "\n=== 二、erase 才真正縮短容器 ===" << std::endl;
    v.erase(new_end, v.end());
    std::cout << "erase 後: ";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;
    std::cout << "實際大小: " << v.size() << std::endl;

    std::cout << "\n=== 三、殘留區「不可依賴」的證明（換成 string）===" << std::endl;
    std::vector<std::string> words = {"alpha", "bravo", "charlie", "delta", "echo"};
    auto wEnd = std::remove_if(words.begin(), words.end(),
                               [](const std::string& s) { return s.size() == 5; });
    std::cout << "保留區: ";
    for (auto it = words.begin(); it != wEnd; ++it) std::cout << "[" << *it << "] ";
    std::cout << std::endl;
    std::cout << "殘留區元素個數: " << (words.end() - wEnd)
              << "（內容為 valid but unspecified，故不印出其值）" << std::endl;
    words.erase(wEnd, words.end());

    std::cout << "\n=== 四、LeetCode 26. Remove Duplicates from Sorted Array ===" << std::endl;
    std::vector<int> nums1 = {0, 0, 1, 1, 1, 2, 2, 3, 3, 4};
    int len = removeDuplicates(nums1);
    std::cout << "去重後長度 " << len << ", 內容: ";
    for (int x : nums1) std::cout << x << " ";
    std::cout << std::endl;

    std::cout << "\n=== 五、LeetCode 283. Move Zeroes ===" << std::endl;
    std::vector<int> nums2 = {0, 1, 0, 3, 12};
    moveZeroes(nums2);
    std::cout << "{0,1,0,3,12} → ";
    for (int x : nums2) std::cout << x << " ";
    std::cout << std::endl;

    std::cout << "\n=== 六、日常實務：感測器資料清洗 ===" << std::endl;
    std::vector<double> readings = {21.5, -999.0, 22.1, 23.0, 150.0, 22.8, -999.0, 21.9};
    std::cout << "清洗前筆數: " << readings.size() << std::endl;
    std::size_t dropped = dropInvalidReadings(readings, -40.0, 85.0);
    std::cout << "剔除無效讀數: " << dropped << " 筆" << std::endl;
    std::cout << "清洗後: ";
    for (double t : readings) std::cout << t << " ";
    std::cout << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 15 課：vector 元素刪除：pop_back、erase、clear9.cpp" -o erase_remove
//
// 【關於下方預期輸出的但書】
//   第一段「remove_if 後（整個容器）」印出的後 5 個數字，屬於
//   valid but unspecified 的殘留區。讀取它們不是 undefined behavior，
//   但標準不保證其內容；下方數值僅為本機（GCC 15.2 / libstdc++）此次
//   實測所見，換編譯器、換元素型別都可能不同，程式碼絕不可依賴它。

// === 預期輸出 ===
// === 一、remove_if 只重排、不改變 size ===
// 原始資料: 1 2 3 4 5 6 7 8 9 10
// remove_if 後（整個容器）: 1 3 5 7 9 6 7 8 9 10
//   ↑ 前 5 個是保留的奇數；後 5 個是 valid but unspecified 的殘留
// 邏輯大小 (new_end - begin): 5
// 實際大小 (v.size())      : 10  ← 完全沒變！
//
// === 二、erase 才真正縮短容器 ===
// erase 後: 1 3 5 7 9
// 實際大小: 5
//
// === 三、殘留區「不可依賴」的證明（換成 string）===
// 保留區: [charlie] [echo]
// 殘留區元素個數: 3（內容為 valid but unspecified，故不印出其值）
//
// === 四、LeetCode 26. Remove Duplicates from Sorted Array ===
// 去重後長度 5, 內容: 0 1 2 3 4
//
// === 五、LeetCode 283. Move Zeroes ===
// {0,1,0,3,12} → 1 3 12 0 0
//
// === 六、日常實務：感測器資料清洗 ===
// 清洗前筆數: 8
// 剔除無效讀數: 3 筆
// 清洗後: 21.5 22.1 23 22.8 21.9
