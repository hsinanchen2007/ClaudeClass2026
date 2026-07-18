// ============================================================
// std::count / std::count_if
// 分類 (Category): Non-modifying sequence operations (非修改型)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/count
//   * https://cplusplus.com/reference/algorithm/count/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// 「count」處理的問題只有一句話:
//
//   「在這段資料裡,符合條件的元素總共有幾個?」
//
// 兩個版本各自負責不同的「條件」:
//
//   ┌──────────────────────┬─────────────────────────────────┐
//   │ count(f, l, value)   │ 條件: 元素 == value             │
//   │ count_if(f, l, p)    │ 條件: 述詞 p(元素) == true       │
//   └──────────────────────┴─────────────────────────────────┘
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、count 跟手寫 for 的差別在哪?                          │
// └────────────────────────────────────────────────────────────┘
//
//   * 一行就把「計數」這個意圖講完,不必看 for 內部變數累加。
//   * 回傳型別是「迭代器距離型別」(difference_type),
//     一般來說是 std::ptrdiff_t — 這是「有號整數」,
//     之後與 size_t 比較時要小心 -Wsign-compare 警告。
//   * 任何能用迭代器走訪的容器都可以套用,包含 std::string、原生陣列。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、count vs find vs all/any_of 的選擇                     │
// └────────────────────────────────────────────────────────────┘
//
//   * 只想知道「有沒有」 → all_of / any_of / none_of (可短路)
//   * 只想知道「第一個在哪」 → find / find_if (找到就停)
//   * 想知道「總共幾個」 → count / count_if  (一定走完整個範圍)
//
// 切記:count_if 不會短路,即使你只是想判斷「至少 k 個」,
//      也是會把整個範圍走完。要短路就改用 any_of 或自己寫 for 加 early return。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class InputIt, class T>
//   typename iterator_traits<InputIt>::difference_type
//   count(InputIt first, InputIt last, const T& value);
//
//   template <class InputIt, class UnaryPred>
//   typename iterator_traits<InputIt>::difference_type
//   count_if(InputIt first, InputIt last, UnaryPred p);
//
//   * C++20 起為 constexpr。
//   * C++17 起有執行策略多載。
//   * C++20 引入 std::ranges::count / count_if。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 恰好 (last - first) 次 — O(n)
//   空間: O(1)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 回傳型別是「有號」difference_type,不是 size_t。
//      與 v.size() 比較時可能有警告,要 static_cast。
//   2. 對 std::set/map 用「成員函式 .count()」較快;對 std::multiset
//      .count(k) 是 O(log n + 出現次數),也比 std::count 快。
//   3. count 不能短路,只想「有沒有」用 any_of 更省。
//   4. 對自訂型別:count 需要 operator==;沒有就改用 count_if 配 lambda。
//
// ============================================================

/*
補充筆記：std::count
  - count/count_if 做線性計數，不需要資料排序。
  - 若資料已排序且只想數某個 key，equal_range 的兩端距離可能更有效。
  - count 比 find 多掃描到結尾，若只關心是否存在，find 更早停。
  - std::count 會改變目標範圍內容或元素順序；呼叫前要確認輸出範圍空間足夠、iterator 有效。
  - copy/transform/generate/fill 寫入目的地時不會自動擴容，除非你使用 back_inserter 這類 insert iterator。
  - remove/unique 不會真的縮短容器，只把保留元素移到前面並回傳新邏輯結尾；還要搭配 erase 才會改變 size。
  - reverse/rotate/swap_ranges 會改變元素位置；若外部保存索引或 iterator，要重新確認是否仍指向預期元素。
  - move algorithm 會把元素搬到目的地，來源仍有效但值可能改變；後續只能重新指定或安全銷毀。
  - shuffle/sample 需要亂數引擎；不要每次呼叫都用同一個固定種子，除非你刻意要可重現測試結果。
*/
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> v{1, 2, 3, 2, 5, 2, 7, 8, 2};

    // --- 範例 1: 計算 2 出現幾次 ---
    auto n2 = std::count(v.begin(), v.end(), 2);
    std::cout << "count(2) = " << n2 << '\n';

    // --- 範例 2: 計算偶數個數 (count_if) ---
    auto even = std::count_if(v.begin(), v.end(),
                              [](int x){ return x % 2 == 0; });
    std::cout << "even count = " << even << '\n';

    // --- 範例 3: 計算 > 5 的個數 ---
    auto big = std::count_if(v.begin(), v.end(),
                             [](int x){ return x > 5; });
    std::cout << "count(>5) = " << big << '\n';

    // --- 範例 4: 沒出現過的元素回傳 0 ---
    std::cout << "count(99) = " << std::count(v.begin(), v.end(), 99) << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_387_first_unique_char();
    void leetcode_1832_pangram();
    void practical_vote_counting();
    void leetcode_2overlap_count_zeros();
    void practical_inventory_low_stock();
    leetcode_387_first_unique_char();
    leetcode_1832_pangram();
    practical_vote_counting();
    leetcode_2overlap_count_zeros();
    practical_inventory_low_stock();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 387: 字串中第一個唯一字元
//                (First Unique Character in a String)
// ----------------------------------------------------------------
// 題目:給字串 s,找第一個只出現一次的字元的索引;不存在回傳 -1。
//
// 為什麼用 std::count:
//   「某字元出現幾次」剛好就是 count 的工作。對每個位置 i,
//   檢查 count(s, s[i]) 是否為 1,等於 1 就是答案。
//
// 解法步驟:
//   1. 走訪 i = 0 .. n-1。
//   2. 對 s[i] 用 std::count 算它在整個字串出現幾次。
//   3. 第一個 count == 1 的位置即為答案。
//
// 複雜度:時間 O(n × k),k 為字元集大小 (英文 26 → O(n));空間 O(1)。
void leetcode_387_first_unique_char() {
    std::string s = "loveleetcode";
    int answer = -1;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (std::count(s.begin(), s.end(), s[i]) == 1) {
            answer = static_cast<int>(i);
            break;
        }
    }
    std::cout << "LC387: first unique index = " << answer << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1832: 判斷句子是否為全字母句 (Pangram)
// ----------------------------------------------------------------
// 題目:給一個只含小寫英文字母的字串,判斷是否每個字母 a..z 都至少出現一次。
//
// 為什麼用 std::count:
//   對每個字母 c,只要 std::count(s, c) >= 1 就代表它出現過。
//   全部 26 個字母都符合 → 是 pangram。
//
// 解法步驟:
//   1. 對 c = 'a' .. 'z' 用 std::count 計次。
//   2. 任何一個 count 為 0 就不是 pangram。
//
// 複雜度:時間 O(26 × n) = O(n);空間 O(1)。
void leetcode_1832_pangram() {
    std::string s = "thequickbrownfoxjumpsoverthelazydog";
    bool is_pangram = true;
    for (char c = 'a'; c <= 'z'; ++c) {
        if (std::count(s.begin(), s.end(), c) == 0) {
            is_pangram = false;
            break;
        }
    }
    std::cout << "LC1832: " << (is_pangram ? "true" : "false") << '\n';
}

// ----------------------------------------------------------------
// 實務範例:計票系統
// ----------------------------------------------------------------
// 場景:選舉結束,票箱裡有一連串候選人代號。要求各候選人得票。
//
// 為什麼用 std::count:
//   * 候選人少且固定 (例如 3 個),逐一 count 就好,程式碼最直白。
//   * 候選人很多時,改用 unordered_map<string,int> 一次累加才划算 —
//     因為對 N 候選人 + n 票,逐一 count 是 O(N·n),map 累加是 O(n)。
void practical_vote_counting() {
    std::vector<std::string> votes{
        "alice", "bob", "alice", "carol", "bob", "alice", "carol", "alice"
    };
    auto a = std::count(votes.begin(), votes.end(), std::string("alice"));
    auto b = std::count(votes.begin(), votes.end(), std::string("bob"));
    auto c = std::count(votes.begin(), votes.end(), std::string("carol"));
    std::cout << "votes alice=" << a
              << " bob="   << b
              << " carol=" << c << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1295: 統計位數為偶數的數字 — count_if 直譯版
// ----------------------------------------------------------------
// 題目:給整數陣列,計算其中「位數為偶數」的元素個數。
//
// 為什麼用 std::count_if:
//   「計算符合 P 的個數」直接對應 count_if;判定邏輯放 lambda 即可。
//
// 複雜度:時間 O(n × log10(num));空間 O(1)。
void leetcode_2overlap_count_zeros() {
    std::vector<int> nums{12, 345, 2, 6, 7896};
    auto count_even_digits = std::count_if(nums.begin(), nums.end(), [](int x){
        int d = 0;
        do { ++d; x /= 10; } while (x);
        return d % 2 == 0;
    });
    std::cout << "LC1295 even-digit count: " << count_even_digits << '\n';
}

// ----------------------------------------------------------------
// 實務範例:庫存系統 — 統計「低庫存」商品數量
// ----------------------------------------------------------------
// 場景:每日報表要回報「庫存少於 10 件」的商品數量,以提醒進貨。
//      count_if + lambda 一行寫完,語意直白。
void practical_inventory_low_stock() {
    std::vector<int> stock{50, 5, 100, 3, 28, 7, 200};
    auto low = std::count_if(stock.begin(), stock.end(),
                             [](int s){ return s < 10; });
    std::cout << "low stock items: " << low << '\n';
}

// === 預期輸出 (Expected output) ===
// count(2) = 4
// even count = 5
// count(>5) = 2
// count(99) = 0
// LC387: first unique index = 2
// LC1832: true
// votes alice=4 bob=2 carol=2
// LC1295 even-digit count: 2
// low stock items: 3
