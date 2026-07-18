// ============================================================
// std::mismatch
// 分類 (Category): Non-modifying sequence operations (非修改型)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/mismatch
//   * https://cplusplus.com/reference/algorithm/mismatch/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::mismatch 解決的問題很單純,但用途出奇地廣:
//
//   「兩段資料同步逐元素比對,告訴我『第一個不相等的位置』。」
//
// 它和 std::equal 是一對兄弟:
//
//   * std::equal     回傳「整段一不一樣」的 bool
//   * std::mismatch  回傳「第一個不一樣是在哪兩個位置」 — pair<It1, It2>
//
// mismatch 的回傳值能告訴你「同前綴有多長」「差異在哪個 key」這些
// equal 拿不到的資訊,因此寫「比較工具」「diff」「LCP (longest common prefix)」
// 時非常自然就會用到 mismatch。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、三參數版本是危險的!                                   │
// └────────────────────────────────────────────────────────────┘
//
// 歷史上 C++98/03 只有「三參數版」mismatch:
//
//   pair<It1, It2> mismatch(first1, last1, first2);
//
// 它假設第二段資料的長度 ≥ 第一段。若實際短了,就會超讀 — UB。
//
// C++14 起加入「四參數版」(可指定 last2),會自己檢查長度。
// **新程式碼一律用四參數版本**,避免邊界錯誤。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、回傳值結構                                             │
// └────────────────────────────────────────────────────────────┘
//
// 回傳 std::pair<It1, It2>:
//   * pair.first  → 第一範圍中第一個「不相等」位置;
//                   若整段都相等,則為 last1。
//   * pair.second → 第二範圍中對應的位置。
//
// 從這個 pair 你可以推導出:
//   * 共同前綴長度       = distance(first1, pair.first)
//   * 兩邊在差異處的值   = *pair.first 與 *pair.second
//   * 是否完全相同       = (pair.first == last1) (前提:長度相同)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   // (1) 三參數 — 危險,避免使用
//   template <class InputIt1, class InputIt2>
//   std::pair<InputIt1, InputIt2>
//   mismatch(InputIt1 first1, InputIt1 last1, InputIt2 first2);
//
//   // (2) 四參數 (C++14) — 安全,推薦
//   template <class InputIt1, class InputIt2>
//   std::pair<InputIt1, InputIt2>
//   mismatch(InputIt1 first1, InputIt1 last1,
//            InputIt2 first2, InputIt2 last2);
//
//   // 兩者皆有可帶 BinaryPred 的多載
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 最多 min(N1, N2) 次比較 — O(n)
//   空間: O(1)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 三參數版有 UB 風險,新程式碼一律用四參數版本。
//   2. 兩段「全相等且等長」時,pair.first == last1 且 pair.second == last2。
//   3. 和 equal 比較:equal 只回 bool;mismatch 給更豐富的資訊。
//   4. 浮點數比較不要用 ==,要傳一個容差判斷的 lambda。
//
// ============================================================

/*
補充筆記：std::mismatch
  - mismatch 找兩段範圍第一個不同的位置。
  - 回傳 pair，分別指向兩段範圍的差異點。
  - 它很適合用來產生錯誤報告：不只知道不同，還知道從哪裡開始不同。
  - std::mismatch 會改變目標範圍內容或元素順序；呼叫前要確認輸出範圍空間足夠、iterator 有效。
  - copy/transform/generate/fill 寫入目的地時不會自動擴容，除非你使用 back_inserter 這類 insert iterator。
  - remove/unique 不會真的縮短容器，只把保留元素移到前面並回傳新邏輯結尾；還要搭配 erase 才會改變 size。
  - reverse/rotate/swap_ranges 會改變元素位置；若外部保存索引或 iterator，要重新確認是否仍指向預期元素。
  - move algorithm 會把元素搬到目的地，來源仍有效但值可能改變；後續只能重新指定或安全銷毀。
  - shuffle/sample 需要亂數引擎；不要每次呼叫都用同一個固定種子，除非你刻意要可重現測試結果。
*/
#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: 找第一個不相等的位置 ---
    std::vector<int> a{1, 2, 3, 4, 5};
    std::vector<int> b{1, 2, 9, 4, 5};
    auto p = std::mismatch(a.begin(), a.end(), b.begin(), b.end());
    if (p.first != a.end())
        std::cout << "mismatch at index " << (p.first - a.begin())
                  << ": " << *p.first << " vs " << *p.second << '\n';

    // --- 範例 2: 兩範圍完全相同 (pair.first == end) ---
    std::vector<int> c{1, 2, 3};
    std::vector<int> d{1, 2, 3};
    auto p2 = std::mismatch(c.begin(), c.end(), d.begin(), d.end());
    std::cout << "all equal: " << (p2.first == c.end() ? "yes" : "no") << '\n';

    // --- 範例 3: 取得「最長共同前綴 (LCP)」長度 ---
    // 這是 mismatch 最自然的用法之一:差異點之前的部分就是共同前綴。
    std::string s1 = "preconfig";
    std::string s2 = "preorder";
    auto pp = std::mismatch(s1.begin(), s1.end(), s2.begin(), s2.end());
    std::cout << "common prefix length = " << (pp.first - s1.begin())
              << " (\"" << std::string(s1.begin(), pp.first) << "\")\n";

    // --- 範例 4: 自訂述詞 — 大小寫不敏感比對 ---
    std::string x = "Hello", y = "HELLO";
    auto p3 = std::mismatch(x.begin(), x.end(), y.begin(), y.end(),
                            [](char ca, char cb){
                                return std::tolower(ca) == std::tolower(cb);
                            });
    std::cout << "case-insensitive equal: "
              << (p3.first == x.end() ? "yes" : "no") << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_14_longest_common_prefix();
    void practical_config_diff();
    void leetcode_2540_min_common_value_concept();
    void practical_diff_first_byte();
    leetcode_14_longest_common_prefix();
    practical_config_diff();
    leetcode_2540_min_common_value_concept();
    practical_diff_first_byte();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 14: 最長共同前綴 (Longest Common Prefix)
// ----------------------------------------------------------------
// 題目:給字串陣列 strs,找出所有字串的最長共同前綴 (LCP);
//      若不存在共同前綴則回傳 ""。
//
// 為什麼用 std::mismatch:
//   LCP 的定義就是「兩字串第一個不相等位置之前的部分」 — 100% 是 mismatch。
//   每多一個字串,就用 mismatch 把目前 LCP 截到該字串的對應差異點。
//
// 解法步驟:
//   1. 以 strs[0] 為初始 LCP。
//   2. 對 strs[1..n-1] 每一個 s,用 mismatch(LCP, s) 找差異點,
//      把 LCP 截到差異點之前。
//   3. 若 LCP 已成空字串可提早結束。
//
// 複雜度:時間 O(S),S 為所有字元總數;空間 O(1) (除了結果)。
void leetcode_14_longest_common_prefix() {
    std::vector<std::string> strs{"flower", "flow", "flight"};
    if (strs.empty()) { std::cout << "LC14: \"\"\n"; return; }
    std::string prefix = strs[0];
    for (std::size_t i = 1; i < strs.size(); ++i) {
        auto p = std::mismatch(prefix.begin(), prefix.end(),
                               strs[i].begin(), strs[i].end());
        prefix.erase(p.first, prefix.end());
        if (prefix.empty()) break;
    }
    std::cout << "LC14: \"" << prefix << "\"\n";
}

// ----------------------------------------------------------------
// 實務範例:比對兩份「有序設定檔」的第一個差異
// ----------------------------------------------------------------
// 場景:dev 與 prod 的兩份 std::map<key, value> 設定,要找第一個
//      不一致的設定項 (常見於部署檢查、配置 diff 工具)。
//
// 為什麼用 std::mismatch:
//   std::map 內部就是按 key 排序的,逐元素比對是合理的。
//   mismatch 一行就能告訴你「第一個不同 key + 兩邊的值」。
void practical_config_diff() {
    std::map<std::string, std::string> dev{
        {"db.host", "localhost"},
        {"db.port", "5432"},
        {"log.level", "debug"}
    };
    std::map<std::string, std::string> prod{
        {"db.host", "localhost"},
        {"db.port", "6543"},
        {"log.level", "info"}
    };
    auto p = std::mismatch(dev.begin(), dev.end(),
                           prod.begin(), prod.end());
    if (p.first == dev.end())
        std::cout << "configs identical\n";
    else
        std::cout << "first diff key=" << p.first->first
                  << " dev=" << p.first->second
                  << " prod=" << p.second->second << '\n';
}

// ----------------------------------------------------------------
// LeetCode 2540 概念:兩個有序陣列的第一個共同位置 (簡化)
// ----------------------------------------------------------------
// 題目簡化:給兩個排序好的陣列,找第一個「相同位置元素相等」之處。
//          (非原題,而是 mismatch 的對偶用法 — 找「第一個 match」。)
//
// 為什麼用 std::mismatch (反向想):
//   mismatch 找第一個不同;反過來「找第一個相同」就是把 predicate 反向。
//   或更直接:用迴圈手寫。這裡示範 mismatch 的 predicate 反向用法。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_2540_min_common_value_concept() {
    std::vector<int> a{1, 2, 3, 6, 7};
    std::vector<int> b{2, 4, 3, 6, 8};
    // 用 mismatch 找第一個「不滿足 a != b」的位置,即第一個相等位置
    auto p = std::mismatch(a.begin(), a.end(), b.begin(),
                           [](int x, int y){ return x != y; });
    if (p.first != a.end())
        std::cout << "first equal at idx="
                  << (p.first - a.begin()) << " val=" << *p.first << '\n';
    else
        std::cout << "no equal position\n";
}

// ----------------------------------------------------------------
// 實務範例:二進位檔案比對 — 找第一個不同 byte
// ----------------------------------------------------------------
// 場景:比對兩個檔案內容,要找出「第一個不同的 byte 位置」 —
//      mismatch 提供 (位置, 兩邊的值) 一次回傳。
void practical_diff_first_byte() {
    std::vector<unsigned char> f1{0x10, 0x20, 0x30, 0x44, 0x50};
    std::vector<unsigned char> f2{0x10, 0x20, 0x30, 0x55, 0x50};
    auto p = std::mismatch(f1.begin(), f1.end(), f2.begin(), f2.end());
    if (p.first != f1.end())
        std::cout << "first diff at idx=" << (p.first - f1.begin())
                  << " f1=" << static_cast<int>(*p.first)
                  << " f2=" << static_cast<int>(*p.second) << '\n';
    else
        std::cout << "files identical\n";
}

// === 預期輸出 (Expected output) ===
// mismatch at index 2: 3 vs 9
// all equal: yes
// common prefix length = 3 ("pre")
// case-insensitive equal: yes
// LC14: "fl"
// first diff key=db.port dev=5432 prod=6543
// first equal at idx=1 val=2
// first diff at idx=3 f1=68 f2=85
