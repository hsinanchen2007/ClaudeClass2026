// ============================================================
// std::unique / std::unique_copy
// 分類 (Category): Modifying sequence operations (修改型)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/unique
//   * https://cplusplus.com/reference/algorithm/unique/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 — 又一個必懂的「最大誤解」                     │
// └────────────────────────────────────────────────────────────┘
//
// std::unique **只移除「相鄰」重複,不是全域去重!**
//
// 也就是說:
//
//   {1, 2, 2, 3, 2, 2, 1, 1}  →  unique 後  →  {1, 2, 3, 2, 1}
//                                                  ↑    ↑    ↑
//                                                 相鄰兩個 2 → 留 1
//                                                 但 index 4 還是 2 (不相鄰)
//                                                 它和前面的 2 不相鄰,所以留下
//
// 想做「全域去重」(整個容器中每個值只留一次) 必須先排序:
//
//   std::sort(v.begin(), v.end());
//   v.erase(std::unique(v.begin(), v.end()), v.end());
//
// 這個三行 idiom 是 C++ STL 中最經典的「去重」寫法。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、跟 remove 一樣 — unique 也不會真的縮小容器              │
// └────────────────────────────────────────────────────────────┘
//
// 跟 std::remove 一樣,std::unique 是 algorithm,不能改容器大小:
//
//   1. 它把「保留的元素」往前搬。
//   2. 回傳「邏輯結尾」的迭代器。
//   3. 結尾後的元素是「廢值」,容器 size() 不變。
//   4. 想真刪除要再呼叫容器的 erase。
//
// 慣用法:
//
//   v.erase(std::unique(v.begin(), v.end()), v.end());
//
// C++20 起也有 std::erase_if 等更現代寫法,但這個 idiom 仍非常常見。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、為什麼設計成「相鄰才去重」?                           │
// └────────────────────────────────────────────────────────────┘
//
// 因為 unique 只需要「線性掃描」就能完成,O(N)。
// 如果要全域去重,演算法必須維護「已看過」的集合 — 那是 sort 的工作,
// 或要 hash set (空間換時間)。STL 把這兩件事拆開:
//
//   * sort + unique → 全域去重 (O(N log N))
//   * unique_copy 配合 hash set → 自製全域去重 (O(N))
//
// 這個拆分讓 unique 本身保持簡單、純粹、線性。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、自訂「相等」的判斷                                      │
// └────────────────────────────────────────────────────────────┘
//
// 不一定要用 operator== 比較 — 可以給一個二元述詞 p:
//
//   p(a, b) 回傳 true 即視為「a 和 b 相等」(於 unique 而言是「重複」)。
//
// 範例:把絕對值相同的視為重複:
//   std::unique(v.begin(), v.end(),
//               [](int x, int y){ return std::abs(x) == std::abs(y); });
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class FwdIt>
//   FwdIt unique(FwdIt first, FwdIt last);
//
//   template <class FwdIt, class BinaryPred>
//   FwdIt unique(FwdIt first, FwdIt last, BinaryPred p);
//
//   template <class InputIt, class OutputIt>
//   OutputIt unique_copy(InputIt first, InputIt last, OutputIt d_first);
//
//   template <class InputIt, class OutputIt, class BinaryPred>
//   OutputIt unique_copy(InputIt first, InputIt last, OutputIt d_first,
//                        BinaryPred p);
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間: O(N) 次比較 + O(N) 次搬移
//   空間: O(1) (in-place)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 「相鄰才去重」!想全域去重要先 sort。
//   2. unique 不會改容器大小 — 配合 erase 才能真刪除。
//   3. 自訂述詞 p(a, b) 必須是「等價關係」(對稱、反身、傳遞)。
//   4. 對 std::list 有更好的成員 list::unique()。
//
// ============================================================

/*
補充筆記：std::unique
  - std::unique 只移除相鄰重複，因此若要全域去重，通常要先 sort。
  - 它和 remove 一樣只回傳 logical end，不會改變容器大小。
  - 自訂相等 predicate 要符合等價語意；不穩定的 predicate 會讓哪些元素留下來變得難以理解。
  - std::unique 會改變目標範圍內容或元素順序；呼叫前要確認輸出範圍空間足夠、iterator 有效。
  - copy/transform/generate/fill 寫入目的地時不會自動擴容，除非你使用 back_inserter 這類 insert iterator。
  - remove/unique 不會真的縮短容器，只把保留元素移到前面並回傳新邏輯結尾；還要搭配 erase 才會改變 size。
  - reverse/rotate/swap_ranges 會改變元素位置；若外部保存索引或 iterator，要重新確認是否仍指向預期元素。
  - move algorithm 會把元素搬到目的地，來源仍有效但值可能改變；後續只能重新指定或安全銷毀。
  - shuffle/sample 需要亂數引擎；不要每次呼叫都用同一個固定種子，除非你刻意要可重現測試結果。
*/

// ===========================================================================
// 【面試題】std::unique / unique_copy
// ---------------------------------------------------------------------------
// 🔥 Q1. std::unique 有什麼陷阱?
//     答:兩個。(1) 它只移除「相鄰」的重複元素——要移除全部重複值必須先 sort;
//         (2) 和 remove 一樣不改變容器大小,只回傳新的邏輯終點,必須搭配 erase。
//         標準去重寫法:sort + unique + erase,總複雜度 O(n log n)。
//     追問:如果不需要排序後的順序,有更快的做法嗎?
//         (答:丟進 std::unordered_set 去重,平均 O(n))
//
// 🔥 Q2. {1,1,2,2,2,3,1,1} 呼叫 unique 後,前段內容是什麼?
//     答:{1, 2, 3, 1},回傳的 iterator 指向第 5 個位置;尾端 4 個元素是
//         valid but unspecified。注意最後那個 1 沒有被去掉——因為它和開頭的 1 不相鄰。
//         這正是「沒先 sort」的典型 bug。
//
// Q3. 每一組相鄰重複中,unique 保留的是第一個還是最後一個?
//     答:保留每一組的第一個。這件事在元素是「等價但不相同」的物件時很重要
//         (例如自訂 predicate 只比對 id,那保留下來的是先出現的那筆完整資料)。
//     追問:自訂 predicate 有什麼要求?
//         (答:必須是等價關係——具反身性、對稱性、傳遞性;否則哪些元素會留下無法推理)
//
// Q4. std::unique 和 std::list::unique() 的差別?
//     答:成員版真的會刪除節點、size() 會變小;通用演算法版只搬值、size 不變。
//         同樣的分工也出現在 remove / list::remove。
// ===========================================================================

#include <algorithm>
#include <cmath>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: 相鄰去重 (未排序時注意:不會去掉「不相鄰」的重複) ---
    std::vector<int> v{1, 1, 2, 2, 3, 1, 1};
    auto e = std::unique(v.begin(), v.end());
    v.erase(e, v.end());
    std::cout << "adjacent dedup: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 2: 全域去重 (sort + unique 經典 idiom) ---
    std::vector<int> w{3, 1, 2, 1, 3, 2};
    std::sort(w.begin(), w.end());
    w.erase(std::unique(w.begin(), w.end()), w.end());
    std::cout << "sort+unique:    ";
    for (int x : w) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 3: 自訂述詞 — 視「絕對值相同」為相同 ---
    std::vector<int> a{1, -1, 2, -2, 3};
    a.erase(std::unique(a.begin(), a.end(),
                        [](int x, int y){ return std::abs(x) == std::abs(y); }),
            a.end());
    std::cout << "abs-unique: ";
    for (int x : a) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 4: unique_copy (原資料不變,結果寫到輸出端) ---
    std::vector<int> b{1, 1, 2, 2, 3, 3, 3};
    std::vector<int> r;
    std::unique_copy(b.begin(), b.end(), std::back_inserter(r));
    std::cout << "src: ";
    for (int x : b) std::cout << x << ' ';
    std::cout << '\n';
    std::cout << "uniq_copy: ";
    for (int x : r) std::cout << x << ' ';
    std::cout << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_26_remove_duplicates();
    void leetcode_1047_remove_adjacent_duplicates();
    void practical_compress_log();
    void leetcode_80_remove_duplicates_at_most_two();
    void practical_dedup_url_history();
    leetcode_26_remove_duplicates();
    leetcode_1047_remove_adjacent_duplicates();
    practical_compress_log();
    leetcode_80_remove_duplicates_at_most_two();
    practical_dedup_url_history();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 26: 移除有序陣列中的重複項 (Remove Duplicates from Sorted Array)
// ----------------------------------------------------------------
// 題目:給「已排序」陣列 nums,原地刪除重複元素使每個元素只出現一次,
//      回傳新長度 k。
//
// 為什麼用 std::unique:
//   題目給「已排序」這個前提 — 重複元素必相鄰 — 完美符合 unique 的「相鄰才去重」。
//   一行解決,k 就是 distance(begin, new_end)。
//
// 解法步驟:
//   1. std::unique(nums.begin(), nums.end()) 取得新邏輯結尾。
//   2. k = new_end - begin。題目只在乎前 k 個位置,不必再 erase。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_26_remove_duplicates() {
    std::vector<int> nums{0,0,1,1,1,2,2,3,3,4};
    auto new_end = std::unique(nums.begin(), nums.end());
    int k = static_cast<int>(new_end - nums.begin());
    std::cout << "LC26: k=" << k << ", nums[0..k)=";
    for (auto it = nums.begin(); it != new_end; ++it) std::cout << *it << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1047: 刪除字串中的所有相鄰重複項 (簡化版)
// ----------------------------------------------------------------
// 題目簡化:給字串 s,「一次性」刪除所有相鄰重複的字元 (只做一次,
//          不遞迴 — 原題 LC1047 是要做到沒有相鄰重複為止,
//          這裡示範 std::unique 適用的「單次相鄰去重」面向)。
//
// 為什麼用 std::unique:
//   * 字串也是字元的 sequence,unique 直接適用。
//   * 一行示範「一次相鄰去重」的效果。
//
// 複雜度:時間 O(n),空間 O(1)。
void leetcode_1047_remove_adjacent_duplicates() {
    std::string s = "aabccba";   // 一次去重後: "abcba"
    s.erase(std::unique(s.begin(), s.end()), s.end());
    std::cout << "LC1047(once): " << s << '\n';
}

// ----------------------------------------------------------------
// 實務範例:壓縮 log 中相鄰重複事件 (compress consecutive duplicates)
// ----------------------------------------------------------------
// 場景:監控系統不斷回報「OK」訊息,日誌出現大量連續 "OK"。
//      要把「相鄰重複的事件」合併成一筆,以減少顯示量;
//      但要保留事件的時間順序,所以「不能」先 sort。
//
// 為什麼用 std::unique_copy:
//   * 「相鄰才去重」剛好就是我們要的 — 保留時序、合併連續重複。
//   * 用 _copy 版可保留原 log 不變。
void practical_compress_log() {
    std::vector<std::string> logs{
        "OK","OK","OK","FAIL","FAIL","OK","WARN","WARN","WARN","OK"
    };
    std::vector<std::string> out;
    std::unique_copy(logs.begin(), logs.end(), std::back_inserter(out));
    std::cout << "Compressed log: ";
    for (auto& s : out) std::cout << s << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 80: 移除有序陣列中的重複項 II (Remove Duplicates II)
// 難度: medium
// ----------------------------------------------------------------
// 題目:給已排序陣列 nums,原地刪除重複元素,使每個元素「最多出現兩次」。
//
// 為什麼用 std::unique + 自訂謂詞:
//   不能直接 unique (那只會留 1 個);要用「過了 2 次就視為重複」的 predicate。
//   一個經典寫法:用 unique + capture 計數 (帶狀態的 lambda)。
//
// 解法步驟:
//   1. 維護 count = 連續重複次數;若 a == b 且 count 已 >= 1 → 視為重複。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_80_remove_duplicates_at_most_two() {
    std::vector<int> nums{1,1,1,2,2,3};
    // 自己手寫雙指針版 (unique 的帶狀態謂詞較易混亂)
    int j = 0;
    for (int x : nums) {
        if (j < 2 || nums[j - 2] != x) {
            nums[j++] = x;
        }
    }
    std::cout << "LC80: k=" << j << ", nums[0..k)=";
    for (int i = 0; i < j; ++i) std::cout << nums[i] << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:瀏覽紀錄 (URL history) 去除「連續點同一頁」
// ----------------------------------------------------------------
// 場景:使用者連點同一個 URL 不該每次都記一筆;
//      要做「相鄰去重」,unique 是天然工具。
void practical_dedup_url_history() {
    std::vector<std::string> hist{
        "/home", "/home", "/about", "/about", "/about", "/contact", "/home"
    };
    hist.erase(std::unique(hist.begin(), hist.end()), hist.end());
    std::cout << "history:";
    for (auto& u : hist) std::cout << " " << u;
    std::cout << '\n';
}

// 編譯: g++ -std=c++20 -Wall -Wextra unique.cpp -o unique

// === 預期輸出 ===
// adjacent dedup: 1 2 3 1
// sort+unique:    1 2 3
// abs-unique: 1 2 3
// src: 1 1 2 2 3 3 3
// uniq_copy: 1 2 3
// LC26: k=5, nums[0..k)=0 1 2 3 4
// LC1047(once): abcba
// Compressed log: OK FAIL OK WARN OK
// LC80: k=5, nums[0..k)=1 1 2 2 3
// history: /home /about /contact /home
