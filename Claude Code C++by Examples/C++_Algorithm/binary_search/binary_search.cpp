// ============================================================
// std::binary_search
// 分類 (Category): Binary search operations (二分搜尋,需已排序)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/binary_search
//   * https://cplusplus.com/reference/algorithm/binary_search/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::binary_search 解的問題:
//
//   「在已排序的範圍中,某個 value 是不是存在?」
//
// 重點是「已排序」 — 二分搜尋的整個前提就是資料有單調順序。
// 若範圍未排序,行為未定義 (UB)。
//
// 它只回 bool — 「在」或「不在」。如果你還想知道「在哪裡」、
// 「有幾個」,要用其他工具:
//
//   * 想知道位置 → std::lower_bound + 自己驗證 *it == value
//   * 想知道個數 → std::equal_range(lo, hi),hi - lo 即個數
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼是 O(log N)?                                     │
// └────────────────────────────────────────────────────────────┘
//
// 二分搜尋的精神:每次把「可能還有答案的範圍」減半。
//
//   N → N/2 → N/4 → ... → 1
//
// 共需 log2(N) 步。對 1 百萬筆資料,最多 20 次比較;
// 1 十億筆也只需 30 次 — 線性搜尋的差距是百萬倍。
//
// 這就是「資料先排好」帶來的最大效益 — 後續每次查詢都極快。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、「等價 (equivalent)」與「相等 (equal)」的細節          │
// └────────────────────────────────────────────────────────────┘
//
// 標準說 binary_search 找的是「等價於 value 的元素」。
//
//   a 與 b「等價」 ⇔ !comp(a, b) && !comp(b, a)
//
// 預設比較器是 operator<,所以「等價」變成「不小於對方且對方也不小於自己」 —
// 對普通整數而言就是「相等」。
//
// 對複雜類型 (例如自訂 comp 比較某個欄位),「等價」可能跟「全部相同」不同 —
// set/map 內部就是用「等價」概念來判定唯一性。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class FwdIt, class T>
//   bool binary_search(FwdIt first, FwdIt last, const T& value);
//
//   template <class FwdIt, class T, class Compare>
//   bool binary_search(FwdIt first, FwdIt last, const T& value, Compare comp);
//
//   * C++20 起為 constexpr。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度與需求                                           │
// └────────────────────────────────────────────────────────────┘
//
//   時間: O(log N) 比較
//   空間: O(1)
//   需求: ForwardIterator;範圍必須依 comp 已排序
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 範圍必須已排序,否則 UB!
//   2. 只回 bool,不回位置。要位置請用 lower_bound + 驗證。
//   3. 對 set / map 用容器自帶的 set::count / set::contains 更快、更直觀。
//   4. comp 必須與排序時相同,否則一致性破壞 → UB。
//
// ============================================================

/*
補充筆記：std::binary_search
  - binary_search 只回答是否存在，不告訴你位置；需要位置時應改用 lower_bound。
  - 它的結果依賴排序前置條件；未排序資料上即使偶爾看起來正確，也只是碰巧。
  - 若資料中有重複值，binary_search 不會告訴你有幾個，計數應使用 equal_range 或上下界相減。
  - std::binary_search 屬於二分搜尋家族，前提是輸入範圍已依同一個比較規則排序；未排序資料上使用結果沒有意義。
  - 這類函式通常回傳位置或範圍，不一定回傳 bool；拿到 iterator 後要先檢查是否等於 end()。
  - lower_bound 找第一個不小於 value 的位置，upper_bound 找第一個大於 value 的位置，兩者相減可得到重複值數量。
  - binary_search 只回答是否存在；若後續需要插入位置、索引或重複區間，lower_bound/equal_range 更實用。
  - 比較器必須和排序時使用的規則一致；用不同規則搜尋同一批資料，結果會像資料沒排序一樣不可信。
  - 在 vector 上 iterator 相減可得到索引，在 list 上不行；iterator category 會影響你能不能做 O(1) 距離計算。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::binary_search
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::find 和 std::binary_search 的差別?什麼時候用哪個?
//     答:find 是線性搜尋 O(n),對任何 input iterator 都適用,且不要求排序。
//         binary_search 是 O(log n) 次比較,但要求區間已依同一 comp 排序,
//         而且只回 bool 不回位置 — 要位置得改用 lower_bound。
//         資料若未排序,為了用 binary_search 而先 sort(O(n log n))通常不划算,
//         除非同一批資料要查詢很多次。
//     追問:關聯容器(set/map)上該用 std::find 還是成員 find()?
//           (答:一律用成員版,自由函式版會退化)
//
// 🔥 Q2. binary_search 和 lower_bound / upper_bound / equal_range 的關係?
//     答:四者同屬二分搜尋家族、共用「區間已排序」前置條件。
//         lower_bound 回傳第一個 >= value 的位置;upper_bound 回傳第一個 > value;
//         equal_range 一次回傳 {lower, upper},個數即 upper - lower。
//         binary_search 只回 bool,語意上等價於「做 lower_bound 後再檢查是否等價」。
//     追問:那要「位置 + 存在性」怎麼寫?
//           (答:lower_bound 後檢查 it != last && *it == value,不要呼叫兩次)
//
// ⚠️ 陷阱1. binary_search 判斷「找到」用的是 == 嗎?
//     答:不是。標準找的是「等價(equivalent)」的元素,定義為
//         !comp(a, b) && !comp(b, a) — 全程只用 comp,從不呼叫 operator==。
//     為什麼會錯:多數人腦中的模型是「逐一比對相等」。但在自訂 comp
//         (例如只比較字串長度)之下,"xyz" 與 "ccc" 是等價卻不相等的;
//         這也正是本檔範例 2 回傳 true 的原因。set/map 判定鍵唯一性用的
//         同樣是「等價」而非「相等」。
//
// ⚠️ 陷阱2. 對沒排序的資料呼叫 binary_search 會怎樣?
//     答:UB(未定義行為)。不會丟例外、不會回傳錯誤碼,只會給出「看起來
//         像答案」的 bool。同理,搜尋用的 comp 必須與當初排序用的完全一致,
//         否則等同於在未排序資料上二分。
//     為什麼會錯:因為它「不會 crash 也不會報錯」,小資料上甚至常常剛好答對,
//         讓人誤以為前置條件只是效能建議,而不是正確性要求。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> v{1, 3, 5, 7, 9, 11};

    std::cout << std::boolalpha;

    // --- 範例 1: 存在與不存在 ---
    std::cout << "find 5: "  << std::binary_search(v.begin(), v.end(), 5)  << '\n';
    std::cout << "find 8: "  << std::binary_search(v.begin(), v.end(), 8)  << '\n';
    std::cout << "find 11: " << std::binary_search(v.begin(), v.end(), 11) << '\n';

    // --- 範例 2: 用自訂 comp (對 string 比長度排序的範圍) ---
    std::vector<std::string> words{"a", "bb", "ccc", "dddd"};
    auto by_len = [](const std::string& a, const std::string& b){
        return a.size() < b.size();
    };
    std::cout << "exists string of length 3: "
              << std::binary_search(words.begin(), words.end(),
                                    std::string("xyz"), by_len) << '\n';

    // --- 範例 3: 想取得位置而不只是 bool — 配合 lower_bound ---
    int target = 7;
    auto it = std::lower_bound(v.begin(), v.end(), target);
    if (it != v.end() && *it == target)
        std::cout << "found 7 at index " << (it - v.begin()) << '\n';

    // --- 範例 4: 邊界 — 空範圍 ---
    std::vector<int> e;
    std::cout << "empty contains 1: "
              << std::binary_search(e.begin(), e.end(), 1) << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_704_binary_search();
    void leetcode_1346_check_double_after_sort();
    void practical_config_keyword_validation();
    void leetcode_74_search_2d_matrix();
    void practical_feature_flag_lookup();
    leetcode_704_binary_search();
    leetcode_1346_check_double_after_sort();
    practical_config_keyword_validation();
    leetcode_74_search_2d_matrix();
    practical_feature_flag_lookup();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 704: 二分搜尋 (Binary Search)
// ----------------------------------------------------------------
// 題目:給已排序的整數陣列 nums 與目標 target,若存在回傳其索引;
//      否則回傳 -1。
//
// 為什麼用 std::binary_search + std::lower_bound:
//   * binary_search 快速判斷「存在」(O(log n))。
//   * 真要拿索引,再用 lower_bound 補上 — 這是「STL 二分搜尋」的標準組合。
//
// 複雜度:時間 O(log n);空間 O(1)。
void leetcode_704_binary_search() {
    std::vector<int> nums{-1, 0, 3, 5, 9, 12};
    int target = 9;
    bool exists = std::binary_search(nums.begin(), nums.end(), target);
    int idx = -1;
    if (exists) {
        auto it = std::lower_bound(nums.begin(), nums.end(), target);
        idx = static_cast<int>(it - nums.begin());
    }
    std::cout << "LC704: " << idx << '\n';

    int t2 = 2;
    int idx2 = std::binary_search(nums.begin(), nums.end(), t2) ? 0 : -1;
    std::cout << "LC704(miss): " << idx2 << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1346: 檢查整數及其兩倍是否存在 (排序後用 binary_search)
// ----------------------------------------------------------------
// 題目:給陣列 arr,是否存在 i ≠ j 使得 arr[i] == 2 * arr[j]?
//
// 為什麼用 std::binary_search:
//   把陣列排序後,對每個 x,用 binary_search 在陣列中找 2*x 是否存在。
//   小心 x == 0 的情況 (因為 2 * 0 == 0,要排除自身)。
//   排序 O(N log N) + 每次查詢 O(log N) = O(N log N) — 比平方好很多。
//
// 解法步驟:
//   1. sort 讓我們可以用二分搜。
//   2. 對每個 x,binary_search(2 * x)。
//   3. x == 0 時要至少出現兩次才算 (因為要 i != j)。
//
// 複雜度:時間 O(N log N);空間 O(1) (in-place sort)。
void leetcode_1346_check_double_after_sort() {
    std::vector<int> arr{10, 2, 5, 3};
    std::sort(arr.begin(), arr.end());
    bool ok = false;
    for (int x : arr) {
        if (x == 0) {
            // 兩個 0 才算數
            int zero_count = static_cast<int>(std::count(arr.begin(), arr.end(), 0));
            if (zero_count >= 2) { ok = true; break; }
        } else if (std::binary_search(arr.begin(), arr.end(), 2 * x)) {
            ok = true; break;
        }
    }
    std::cout << "LC1346: " << std::boolalpha << ok << '\n';
}

// ----------------------------------------------------------------
// 實務範例:設定檔關鍵字白名單驗證
// ----------------------------------------------------------------
// 場景:CLI 工具讀入設定後,需快速驗證使用者輸入的選項
//      是否屬於「已知合法選項」清單。將清單預先排序,
//      即可用 binary_search 在 O(log n) 內驗證。
void practical_config_keyword_validation() {
    std::vector<std::string> valid_keys{
        "cache_size", "host", "log_level", "port", "timeout", "user"
    };
    std::vector<std::string> user_inputs{"port", "passwd", "log_level", "xyz"};
    for (const auto& key : user_inputs) {
        bool ok = std::binary_search(valid_keys.begin(), valid_keys.end(), key);
        std::cout << "key=" << key << " valid=" << (ok ? "yes" : "no") << '\n';
    }
}

// ----------------------------------------------------------------
// LeetCode 74: 搜尋二維矩陣 (Search a 2D Matrix)
// 難度: medium
// ----------------------------------------------------------------
// 題目:給一個 m x n 的矩陣,每列由左到右遞增,且每列首元素大於上一列尾元素 —
//      整體可視為「一條已排序的一維陣列」。回傳 target 是否存在。
//
// 為什麼用 std::binary_search:
//   把矩陣「展平」成單一序列 (索引 i 對應 (i/n, i%n)),整體已排序。
//   不過 STL 的 binary_search 需要 iterator — 我們可以先把它複製成 vector,
//   或者用 lower_bound + 索引轉換。這裡用最直白的「複製到 vector」示範。
//
// 複雜度:時間 O(m*n) 複製 + O(log(m*n)) 搜尋;空間 O(m*n)。
//        若不複製、直接用索引換算,可達 O(log(m*n)) 時間、O(1) 空間。
void leetcode_74_search_2d_matrix() {
    std::vector<std::vector<int>> matrix{
        {1,  3,  5,  7},
        {10, 11, 16, 20},
        {23, 30, 34, 60},
    };
    std::vector<int> flat;
    for (const auto& row : matrix)
        for (int x : row) flat.push_back(x);
    int target = 11;
    bool ok = std::binary_search(flat.begin(), flat.end(), target);
    std::cout << "LC74(11): " << std::boolalpha << ok << '\n';
    std::cout << "LC74(13): "
              << std::binary_search(flat.begin(), flat.end(), 13) << '\n';
}

// ----------------------------------------------------------------
// 實務範例:Feature Flag 白名單查詢
// ----------------------------------------------------------------
// 場景:服務啟動時讀入「啟用的功能旗標」清單 (字串),預先排序;
//      每次請求進來時,需 O(log n) 判斷某功能是否啟用。
//      若清單規模小且穩定,binary_search on sorted vector 比 unordered_set
//      還快 (cache friendly、無雜湊計算)。
void practical_feature_flag_lookup() {
    std::vector<std::string> flags{
        "dark_mode", "experimental_ui", "fast_login", "metrics_v2", "new_dashboard"
    };
    // 假設已預先排序
    std::vector<std::string> queries{"fast_login", "old_dashboard", "metrics_v2"};
    for (const auto& q : queries) {
        bool on = std::binary_search(flags.begin(), flags.end(), q);
        std::cout << "flag " << q << " = " << (on ? "ON" : "off") << '\n';
    }
}

// === 預期輸出 (Expected output) ===
// find 5: true
// find 8: false
// find 11: true
// exists string of length 3: true
// found 7 at index 3
// empty contains 1: false
// LC704: 4
// LC704(miss): -1
// LC1346: true
// key=port valid=yes
// key=passwd valid=no
// key=log_level valid=yes
// key=xyz valid=no
// LC74(11): true
// LC74(13): false
// flag fast_login = ON
// flag old_dashboard = off
// flag metrics_v2 = ON
