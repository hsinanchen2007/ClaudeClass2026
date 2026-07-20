// =============================================================================
//  第七課：演算法（Algorithm）與容器的分離設計10.cpp
//    —  重排家族：reverse / rotate / unique（unique 同樣不會真的刪除）
// =============================================================================
//
// 【主題資訊 Information】
//   void  reverse(BidIt f, BidIt l);                              // C++98
//   FwdIt rotate (FwdIt f, FwdIt middle, FwdIt l);                // C++98（C++11 起回傳 FwdIt）
//   FwdIt unique (FwdIt f, FwdIt l);                              // C++98
//   FwdIt unique (FwdIt f, FwdIt l, BinaryPred p);                // C++98
//
//   標準版本：全部 C++98；**rotate 在 C++11 之前回傳 void**，C++11 起回傳
//             「原本的 first 移動後所在的位置」
//   迭代器需求：reverse 要 Bidirectional；rotate / unique 要 Forward
//   複雜度：reverse 為 N/2 次 swap；rotate 最多 N 次 swap；unique 恰好 N-1 次比較
//   回傳：unique 回傳新的邏輯結尾（**與 remove 一樣，需搭配 erase**）
//   標頭檔：<algorithm>
//
// 【詳細解釋 Explanation】
//
// 【1. unique 和 remove 是同一個故事：它也不會真的刪除】
// 這是本課第 9 個檔案的直接延續。std::unique 同樣只拿得到 iterator，
// 拿不到容器本體，所以它能做的一樣只有「把要保留的往前搬，回傳新的邏輯結尾」。
//     auto new_end = std::unique(v.begin(), v.end());
//     v.erase(new_end, v.end());        // ← 這行不能省
// 判斷準則很簡單：**凡是會改變「有效元素個數」的演算法，都不會真的刪除**。
// 目前為止碰到的有兩個：remove（第 9 檔）與 unique（本檔）。
// C++20 同樣提供了一行版本：std::erase / erase_if 對付 remove 的情境；
// unique 則因語意較特殊，仍需維持 unique + erase 的寫法。
//
// 【2. unique 只處理「連續」重複——這是最常見的誤用】
//     {1, 1, 2, 2, 3}  → unique → {1, 2, 3}          ✓ 符合預期
//     {1, 2, 1, 2, 1}  → unique → {1, 2, 1, 2, 1}    ✗ 什麼都沒去掉！
// 因為 1 和 1 之間隔著 2，兩者不相鄰，unique 看不到。
// 這與 adjacent_find 只看相鄰是同一個限制（見本課第 3 個檔案）。
// **標準用法永遠是「先 sort 再 unique 再 erase」**：
//     std::sort(v.begin(), v.end());
//     v.erase(std::unique(v.begin(), v.end()), v.end());
// 排序讓相同的值聚在一起，unique 才看得到它們。整體是 O(N log N)。
// 反過來說，若你**不需要排序**（要保留原始順序），unique 就不是正確工具，
// 應該改用 std::set / std::unordered_set 記錄看過的值。
//
// 【3. rotate：看起來很冷門，其實是很多演算法的基礎】
//     rotate(first, middle, last) 把 [first, middle) 和 [middle, last) 交換位置，
//     結果是 middle 指向的元素變成新的第一個。
//     {1,2,3,4,5} 以 middle = begin()+2 旋轉 → {3,4,5,1,2}
// 它的價值在於**就地完成、不需要額外記憶體**，而且是很多操作的底層：
//   * 「把某元素移到最前面」= rotate(first, that, that+1)
//   * 「陣列整體右移 k 格」= rotate(first, last - k, last)
//   * insertion sort 的元素插入步驟本質上就是一次小 rotate
// C++11 起 rotate 回傳「原本的 first 現在跑到哪裡」，方便接續操作。
//
// 【4. reverse 為什麼要 Bidirectional Iterator】
// reverse 的實作是「頭尾雙指標往中間夾，一路 swap」：
//     while (first != last && first != --last) std::iter_swap(first++, last);
// 這裡用到了 --last，也就是**必須能往回走**，所以需要 Bidirectional Iterator。
// 這正是為什麼 std::forward_list **不能**用 std::reverse
// （它只有 Forward Iterator），而必須用它自己的成員函式 flst.reverse()——
// 單向鏈結串列可以靠改指標方向反轉，不需要往回走的能力。
// 這是本課「iterator category 決定演算法可用性」的又一個實例（見第 15 個檔案）。
//
// 【概念補充 Concept Deep Dive】
//
// (A) unique 的比較是「與前一個保留的元素比」還是「與前一個原始元素比」
//   標準規定 unique 比較的是 **相鄰的一對元素**，且保留每組連續相等元素中的
//   第一個。實作上是把當前元素與「上一個被保留的元素」比較。
//   對預設的 operator== 這兩種說法結果相同；但若你傳入自訂的二元謂詞且
//   那個謂詞**不是等價關係**（例如 |a-b| < 1 這種「近似相等」，不具遞移性），
//   結果就會依賴實作細節。**自訂謂詞必須是等價關係**，否則行為不可預期。
//
// (B) rotate 的三種實作策略
//   libstdc++ 依 iterator category 分派不同演算法：
//     * Forward Iterator      → 逐一 swap 前進
//     * Bidirectional Iterator → 三次 reverse（reverse 前段、reverse 後段、reverse 全部）
//     * Random Access Iterator → 用最大公因數（gcd）做環狀置換，swap 次數最少
//   這是 STL「同一個介面、依 iterator 能力自動選最佳實作」的經典示範，
//   也是分離設計的隱藏紅利：演算法可以針對能力更強的 iterator 偷偷加速。
//
// (C) 三次 reverse 就能做出 rotate
//   反轉 [first, middle)、反轉 [middle, last)、再反轉整個 [first, last)，
//   結果就是 rotate。這是面試常考的「陣列旋轉」手寫題標準解，
//   LeetCode 189. Rotate Array 的經典 O(1) 空間解法正是這個。
//
// 【注意事項 Pay Attention】
// 1. **std::unique 不會真的刪除元素**，必須搭配 erase：
//    v.erase(std::unique(v.begin(), v.end()), v.end());
// 2. **unique 只移除「連續」重複**。要去除全部重複必須**先 sort**，
//    或改用 set / unordered_set（若必須保留原始順序）。
// 3. unique 之後 [new_end, end) 是 valid but unspecified，不可依賴其值
//    （理由同 remove，見第 9 個檔案）。
// 4. **std::forward_list 不能用 std::reverse**（只有 Forward Iterator），
//    要用成員函式 flst.reverse()。std::list 也建議用 lst.reverse()（只改指標）。
// 5. rotate 的 middle 必須在 [first, last] 範圍內；超出範圍是未定義行為。
// 6. 傳給 unique 的自訂二元謂詞**必須是等價關係**（自反、對稱、遞移），
//    否則結果依賴實作細節。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】reverse / rotate / unique
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::unique 對 {1, 2, 1, 2, 1} 的結果是什麼？為什麼？
//     答：結果還是 {1, 2, 1, 2, 1}，一個都沒去掉。因為 unique 只移除
//         **連續**重複的元素，這裡任兩個 1 之間都隔著 2，不相鄰。
//         正確做法是先 std::sort 讓相同的值聚在一起，再 unique + erase。
//     追問：那如果必須保留原始順序、不能排序呢？→ unique 就不是正確工具了，
//         要改用 std::unordered_set 記錄看過的值，配合 remove_if 或手動篩選。
//
// 🔥 Q2. 完整去除 vector 中所有重複元素的標準寫法是什麼？複雜度多少？
//     答：std::sort(v.begin(), v.end());
//         v.erase(std::unique(v.begin(), v.end()), v.end());
//         複雜度 O(N log N)，由排序主導；unique 本身是 O(N)。
//         兩個步驟缺一不可：不 sort 則抓不到不相鄰的重複，
//         不 erase 則 size 不會變（unique 和 remove 一樣拿不到容器本體）。
//     追問：為什麼 unique 也不能真的刪除？→ 同 remove，演算法只有 iterator，
//         沒有容器本體，無法呼叫 vector::erase 改變 size。
//
// 🔥 Q3. 為什麼 std::forward_list 不能用 std::reverse？
//     答：因為 std::reverse 的實作是頭尾雙指標往中間夾、一路 swap，
//         過程中需要 --last **往回走**，所以要求 Bidirectional Iterator。
//         forward_list 只提供 Forward Iterator（單向），編譯就會失敗。
//         它有自己的成員函式 flst.reverse()——單向鏈結串列靠改指標方向就能反轉，
//         根本不需要往回走的能力。
//     追問：list 呢？→ list 是雙向的，std::reverse 能編譯也能跑，但仍應優先用
//         lst.reverse()：成員函式只改節點指標，不搬移元素值，效率更好。
//
// ⚠️ 陷阱. 這段「去重」程式碼錯在哪？
//        std::sort(v.begin(), v.end());
//        std::unique(v.begin(), v.end());
//        std::cout << "去重後有 " << v.size() << " 個";
//     答：漏了 erase。unique 只是把不重複的元素搬到前面並回傳新的邏輯結尾，
//         v.size() 完全沒變，印出來的還是原本的元素個數，
//         而且尾端留著 valid but unspecified 的殘骸。
//     為什麼會錯：跟 remove 是同一個坑——名字聽起來像「讓容器變成 unique」，
//         實際上它只能搬移。只要記住「**會改變有效元素個數的演算法都不能真的刪除**」，
//         remove 和 unique 就一起記住了。
//         附帶一提，現代 libstdc++ 把 unique 標成 [[nodiscard]]，
//         丟棄回傳值時 g++ -Wall 會警告，等於編譯器幫你抓這個 bug。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <forward_list>
#include <algorithm>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 26. Remove Duplicates from Sorted Array
//   題目：給定**已排序**的 nums，就地移除重複元素，回傳唯一元素的個數 k，
//         且 nums 前 k 個位置必須是那些唯一元素（保持排序）。
//   為什麼用到本主題：輸入已排序 → 重複元素必定相鄰 → 正是 unique 的適用條件。
//         而且題目「回傳 k、只檢查前 k 個」的規格，恰好就是 unique 的合約
//         （回傳新邏輯結尾、不處理尾端）——與 LeetCode 27 對應 remove 完全同理。
//   複雜度：O(N)（輸入已排序，不需再排）。
// -----------------------------------------------------------------------------
int removeDuplicates(std::vector<int>& nums) {
    auto new_end = std::unique(nums.begin(), nums.end());
    return static_cast<int>(new_end - nums.begin());
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 189. Rotate Array
//   題目：把陣列向右旋轉 k 步（k 可能大於陣列長度）。
//   為什麼用到本主題：這正是 std::rotate 的定義。要向右轉 k 步，
//         等於把 last - k 當成新的起點：rotate(first, last - k, last)。
//         注意必須先 k %= n，否則 last - k 會越界。
//   複雜度：O(N) 時間、O(1) 額外空間。
//   註：手寫版的經典解法是「三次 reverse」，而那正是 libstdc++ 對
//       Bidirectional Iterator 採用的實作策略之一。
// -----------------------------------------------------------------------------
void rotateArray(std::vector<int>& nums, int k) {
    if (nums.empty()) return;
    k %= static_cast<int>(nums.size());          // ★ k 可能大於長度，先取餘數
    if (k == 0) return;
    std::rotate(nums.begin(), nums.end() - k, nums.end());
}

// 對照組：手寫三次 reverse（面試手寫題的標準解）
void rotateArrayByReverse(std::vector<int>& nums, int k) {
    if (nums.empty()) return;
    k %= static_cast<int>(nums.size());
    if (k == 0) return;
    std::reverse(nums.begin(), nums.end());          // 整個反轉
    std::reverse(nums.begin(), nums.begin() + k);    // 前 k 個再反轉回來
    std::reverse(nums.begin() + k, nums.end());      // 剩下的反轉回來
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】使用者上傳的標籤（tag）批次去重
//   情境：部落格文章的標籤由使用者自由輸入，同一篇文章常出現重複標籤
//         （例如貼上時不小心重複）。存進資料庫前要去重。
//   為什麼用到本主題：這是「先 sort 再 unique 再 erase」三步驟的標準應用。
//         同時對照示範「必須保留原始輸入順序」時 unique 就不適用，
//         得改用 set 記錄看過的值。
// -----------------------------------------------------------------------------
std::vector<std::string> dedupTagsSorted(std::vector<std::string> tags) {
    std::sort(tags.begin(), tags.end());                          // ① 讓相同的聚在一起
    tags.erase(std::unique(tags.begin(), tags.end()), tags.end()); // ② + ③
    return tags;
}

// 保留原始順序的去重：unique 幫不上忙，要自己記錄看過的值
std::vector<std::string> dedupTagsKeepOrder(const std::vector<std::string>& tags) {
    std::vector<std::string> seen;
    std::vector<std::string> result;
    for (const auto& t : tags) {
        if (std::find(seen.begin(), seen.end(), t) == seen.end()) {
            seen.push_back(t);
            result.push_back(t);
        }
    }
    return result;   // 實務上資料量大時應改用 std::unordered_set 做 seen
}

int main() {
    // reverse：反轉, 這裡將 vec 的元素順序反轉
    // ★ reverse 需要 Bidirectional Iterator（實作要 --last 往回走）
    std::cout << "=== reverse ===" << std::endl;
    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::reverse(vec.begin(), vec.end());
    std::cout << "反轉後: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    // rotate：旋轉, 這裡將 vec2 的元素旋轉，使得 vec2.begin() + 2 的元素成為新的第一個元素
    std::cout << "\n=== rotate ===" << std::endl;
    std::vector<int> vec2 = {1, 2, 3, 4, 5};
    // 把 vec2.begin() + 2 變成新的第一個元素
    std::rotate(vec2.begin(), vec2.begin() + 2, vec2.end());
    std::cout << "rotate 後（3 變成第一個）: ";
    for (int n : vec2) std::cout << n << " ";
    std::cout << std::endl;

    // unique：移除連續重複, 這裡將 vec3 中連續重複的元素移除，最後使用 erase 刪除多餘的元素
    // ★ unique 和 remove 一樣不會真的刪除，erase 這行不能省
    std::cout << "\n=== unique ===" << std::endl;
    std::vector<int> vec3 = {1, 1, 2, 2, 2, 3, 3, 4, 5, 5};
    auto new_end = std::unique(vec3.begin(), vec3.end());
    vec3.erase(new_end, vec3.end());
    std::cout << "unique 後: ";
    for (int n : vec3) std::cout << n << " ";
    std::cout << std::endl;

    // ★ 陷阱：unique 只看「連續」重複
    std::cout << "\n=== 陷阱：unique 只處理連續重複 ===" << std::endl;
    std::vector<int> notAdjacent = {1, 2, 1, 2, 1};
    auto ne1 = std::unique(notAdjacent.begin(), notAdjacent.end());
    notAdjacent.erase(ne1, notAdjacent.end());
    std::cout << "{1,2,1,2,1} 直接 unique: ";
    for (int n : notAdjacent) std::cout << n << " ";
    std::cout << "  ← 一個都沒去掉" << std::endl;

    std::vector<int> sorted = {1, 2, 1, 2, 1};
    std::sort(sorted.begin(), sorted.end());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
    std::cout << "先 sort 再 unique:      ";
    for (int n : sorted) std::cout << n << " ";
    std::cout << "  ← 正確去重" << std::endl;

    // ★ forward_list 不能用 std::reverse，要用成員函式
    std::cout << "\n=== forward_list 只能用成員函式 reverse ===" << std::endl;
    std::forward_list<int> flst = {1, 2, 3, 4, 5};
    // std::reverse(flst.begin(), flst.end());  // 編譯錯誤！只有 Forward Iterator
    flst.reverse();                             // ✓ 改指標方向，不需往回走
    std::cout << "forward_list 反轉後: ";
    for (int n : flst) std::cout << n << " ";
    std::cout << std::endl;

    std::list<int> lst = {1, 2, 3, 4, 5};
    lst.reverse();       // list 兩種都能用，但成員函式只改指標、效率較好
    std::cout << "list 反轉後(成員函式): ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "\n=== LeetCode 26. Remove Duplicates from Sorted Array ===" << std::endl;
    std::vector<int> lc26 = {0, 0, 1, 1, 1, 2, 2, 3, 3, 4};
    int k = removeDuplicates(lc26);
    std::cout << "[0,0,1,1,1,2,2,3,3,4] -> k=" << k << ", 前 k 個: ";
    for (int i = 0; i < k; ++i) std::cout << lc26[i] << " ";
    std::cout << std::endl;

    std::cout << "\n=== LeetCode 189. Rotate Array ===" << std::endl;
    std::vector<int> lc189a = {1, 2, 3, 4, 5, 6, 7};
    rotateArray(lc189a, 3);
    std::cout << "[1..7] 右轉 3 步 (std::rotate): ";
    for (int n : lc189a) std::cout << n << " ";
    std::cout << std::endl;

    std::vector<int> lc189b = {1, 2, 3, 4, 5, 6, 7};
    rotateArrayByReverse(lc189b, 3);
    std::cout << "[1..7] 右轉 3 步 (三次 reverse): ";
    for (int n : lc189b) std::cout << n << " ";
    std::cout << "  ← 結果相同" << std::endl;

    std::vector<int> lc189c = {1, 2};
    rotateArray(lc189c, 5);          // k 大於長度，先取餘數
    std::cout << "[1,2] 右轉 5 步: ";
    for (int n : lc189c) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "\n=== 日常實務：文章標籤去重 ===" << std::endl;
    std::vector<std::string> tags = {"cpp", "stl", "cpp", "algorithm", "stl", "cpp"};
    std::cout << "原始標籤: ";
    for (const auto& t : tags) std::cout << t << " ";
    std::cout << std::endl;

    std::cout << "排序去重: ";
    for (const auto& t : dedupTagsSorted(tags)) std::cout << t << " ";
    std::cout << std::endl;

    std::cout << "保留原序: ";
    for (const auto& t : dedupTagsKeepOrder(tags)) std::cout << t << " ";
    std::cout << "  ← 需要保留輸入順序時 unique 幫不上忙" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第七課：演算法（Algorithm）與容器的分離設計10.cpp -o demo10

// === 預期輸出 ===
// === reverse ===
// 反轉後: 5 4 3 2 1 
//
// === rotate ===
// rotate 後（3 變成第一個）: 3 4 5 1 2 
//
// === unique ===
// unique 後: 1 2 3 4 5 
//
// === 陷阱：unique 只處理連續重複 ===
// {1,2,1,2,1} 直接 unique: 1 2 1 2 1   ← 一個都沒去掉
// 先 sort 再 unique:      1 2   ← 正確去重
//
// === forward_list 只能用成員函式 reverse ===
// forward_list 反轉後: 5 4 3 2 1 
// list 反轉後(成員函式): 5 4 3 2 1 
//
// === LeetCode 26. Remove Duplicates from Sorted Array ===
// [0,0,1,1,1,2,2,3,3,4] -> k=5, 前 k 個: 0 1 2 3 4 
//
// === LeetCode 189. Rotate Array ===
// [1..7] 右轉 3 步 (std::rotate): 5 6 7 1 2 3 4 
// [1..7] 右轉 3 步 (三次 reverse): 5 6 7 1 2 3 4   ← 結果相同
// [1,2] 右轉 5 步: 2 1 
//
// === 日常實務：文章標籤去重 ===
// 原始標籤: cpp stl cpp algorithm stl cpp 
// 排序去重: algorithm cpp stl 
// 保留原序: cpp stl algorithm   ← 需要保留輸入順序時 unique 幫不上忙
