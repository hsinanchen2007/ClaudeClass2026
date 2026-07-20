// ============================================================
// std::find / std::find_if / std::find_if_not
// 分類 (Category): Non-modifying sequence operations (非修改型序列演算法)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/find
//   * https://cplusplus.com/reference/algorithm/find/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// 「線性搜尋 (Linear Search)」是所有資料結構演算法中最基礎的一個動作:
//   給我一段資料 [first, last),從頭逐一檢查每個元素,
//   找到第一個「符合條件」的元素就停下並回傳它的位置;
//   若整段都沒找到,則回傳 last 表示「沒有」。
//
// std::find 系列就是「線性搜尋」這個概念在 C++ STL 中的標準化實作。
// 它有三種變體,差別只在「符合條件」的判定方式:
//
//   ┌──────────────────┬─────────────────────────────────────┐
//   │ 函式             │ 條件 (元素 e 是否符合)              │
//   ├──────────────────┼─────────────────────────────────────┤
//   │ find(f,l,v)      │ e == v          ← 用 operator==     │
//   │ find_if(f,l,p)   │ p(e) == true    ← 用述詞 (Predicate) │
//   │ find_if_not(f,l,p)│ p(e) == false  ← 述詞反向          │
//   └──────────────────┴─────────────────────────────────────┘
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼要用 STL 提供的 find,而不是自己寫 for 迴圈?    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 「意圖明確」 — 看到 std::find 就知道是在做「找第一個符合的元素」,
//      不必去細讀 for 迴圈裡的每一行邏輯。
//   2. 「型別安全」 — 範圍 [first, last) 的迭代器抽象讓同一份程式
//      能適用於 vector / list / deque / array / 原生陣列 / C 字串 ...。
//   3. 「實作正確」 — 邊界處理、空容器、找不到的情況都已被標準函式處理好,
//      自己寫 for 容易在邊界 (off-by-one) 上犯錯。
//   4. 「可組合」 — 回傳值是迭代器,可以接著傳給其他 STL 函式
//      (例如 erase、distance、advance ...) 形成流暢的演算法管線。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class InputIt, class T>
//   InputIt find(InputIt first, InputIt last, const T& value);
//
//   template <class InputIt, class UnaryPred>
//   InputIt find_if(InputIt first, InputIt last, UnaryPred p);
//
//   template <class InputIt, class UnaryPred>
//   InputIt find_if_not(InputIt first, InputIt last, UnaryPred p);  // C++11
//
//   * C++20 起為 constexpr,可在編譯期使用。
//   * C++17 起新增「執行策略 (Execution Policy)」多載,可平行搜尋,
//     例如 std::find(std::execution::par, first, last, value)。
//   * C++20 引入 std::ranges::find,可直接傳容器並支援投影 (projection),
//     例如 std::ranges::find(users, 102, &User::id);
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、參數與回傳值 (Parameters & Return value)               │
// └────────────────────────────────────────────────────────────┘
//
//   first, last : 半開區間 [first, last) 的兩個迭代器
//   value       : 要比對的目標值,使用 operator== 比較
//   p           : 一元述詞,接受元素型別,回傳 bool
//
//   回傳: 找到 → 第一個符合的元素的迭代器
//         找不到 → last (即 end())
//   ★ 切記:沒找到不會回傳 nullptr 或 -1,而是 last,務必先比對才能解參考。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間: O(n) — 最差情況需掃完整個範圍
//   空間: O(1) — 不額外配置記憶體
//
//   若資料「已排序」,改用 std::binary_search / std::lower_bound 可降到 O(log n)。
//   若資料常需多次查詢且無順序需求,改用 std::set / std::unordered_set。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、需求 (Requirements)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   * InputIt: 至少是 LegacyInputIterator (前向走訪即可)。
//   * find:    元素型別需可與 T 用 operator== 比較。
//   * find_if: 述詞 p 必須是「純函式」— 同樣輸入給同樣結果,
//              且不可在搜尋過程中修改容器。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 「找不到回傳 last」這件事一定要記住,否則直接 *it 是未定義行為。
//   2. find vs find_if:
//        * find 用 operator==;若元素是自訂型別且沒定義 ==,會編譯錯誤。
//        * find_if 用 lambda/函式,可以做任意條件判斷。
//   3. find vs std::set::find / std::map::find:
//        * std::find 是 O(n) 線性掃描。
//        * 容器自帶的 .find() 是 O(log n) 或平均 O(1),效能差很多。
//        * 對 set/map/unordered_* 容器,優先使用「成員方法 .find()」。
//   4. find_if 的述詞「不要修改元素」— 雖然技術上可以,但語意上 find 是
//      非修改型 (Non-modifying) 演算法,改了會誤導讀者。要修改請改用 for_each。
//
// ============================================================

/*
補充筆記：std::find
  - find 找不到時回傳 npos，不是 -1；npos 是 size_type 的最大值。
  - 使用 find 結果做 substr 前一定要先判斷 npos，否則位置計算會溢位。
  - 搜尋單一字元、字串、或從指定 pos 開始，成本都和被搜尋長度相關。
  - std::find 會改變目標範圍內容或元素順序；呼叫前要確認輸出範圍空間足夠、iterator 有效。
  - copy/transform/generate/fill 寫入目的地時不會自動擴容，除非你使用 back_inserter 這類 insert iterator。
  - remove/unique 不會真的縮短容器，只把保留元素移到前面並回傳新邏輯結尾；還要搭配 erase 才會改變 size。
  - reverse/rotate/swap_ranges 會改變元素位置；若外部保存索引或 iterator，要重新確認是否仍指向預期元素。
  - move algorithm 會把元素搬到目的地，來源仍有效但值可能改變；後續只能重新指定或安全銷毀。
  - shuffle/sample 需要亂數引擎；不要每次呼叫都用同一個固定種子，除非你刻意要可重現測試結果。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::find / std::find_if / std::find_if_not
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::find 和 std::binary_search 差在哪?什麼時候用哪個?
//     答:find 是線性搜尋 O(n),對任何 input iterator 都適用,不要求排序;
//         binary_search 是二分搜尋 O(log n) 次比較,但要求區間已依同一準則排序,
//         而且只回傳 bool — 要位置得改用 lower_bound。
//         資料未排序時,為了用 binary_search 而先排序 O(n log n) 通常不划算,
//         除非同一份資料要查詢很多次。
//     追問:lower_bound 找不到時回傳什麼?(第一個大於 v 的位置或 last;
//           回傳值 != last 不代表找到,必須再檢查 *it == v)
//
// 🔥 Q2. std::find 找不到時回傳什麼?
//     答:回傳 last(也就是 end()),不是 nullptr 也不是 -1。
//         所以一定要先 if (it != v.end()) 才能解參考,否則是未定義行為。
//     追問:那 std::string::find 呢?(回傳 std::string::npos,是 size_type 的最大值 —
//           兩者是完全不同的 API,別把 npos 的習慣套到 std::find 上)
//
// ⚠️ 陷阱. 對 std::set 為什麼不該用 std::find?
//     答:因為通用演算法版本會退化。std::find 是線性 O(n),
//         而 set::find() 成員函式沿紅黑樹下降是 O(log n)。
//         同理 std::lower_bound 雖然比較次數是 O(log n),但 set 的 iterator 只是
//         bidirectional,每次「跳到中點」都得一步步走,iterator 前進總次數是 O(n);
//         set::lower_bound() 成員版才是真正的 O(log n)。
//         通則:關聯容器一律優先用同名的成員函式。
//     為什麼會錯:多數人腦中的模型是「STL 演算法是通用的,套在哪個容器上效能都一樣」。
//         實際上演算法只看得到 iterator category,看不到容器內部的樹或雜湊結構,
//         因此無法利用它;只有成員函式才知道自己底層長什麼樣。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <iostream>
#include <vector>
#include <string>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> v{4, 1, 7, 3, 8, 1, 9};

    // --- 範例 1: 用 find 找特定值 ---
    // 回傳指向「第一個 7」的迭代器;若不存在則回傳 v.end()。
    auto it = std::find(v.begin(), v.end(), 7);
    if (it != v.end())
        std::cout << "find 7 at index " << (it - v.begin()) << '\n';
    else
        std::cout << "7 not found\n";

    // --- 範例 2: 找不到的情況 (示範 end() 比對) ---
    auto it2 = std::find(v.begin(), v.end(), 100);
    std::cout << "find 100: " << (it2 == v.end() ? "not found" : "found") << '\n';

    // --- 範例 3: find_if — 找第一個偶數 ---
    // 條件用 lambda 表達。寫起來像「找第一個讓這個 lambda 回傳 true 的元素」。
    auto even = std::find_if(v.begin(), v.end(),
                             [](int x){ return x % 2 == 0; });
    if (even != v.end())
        std::cout << "first even = " << *even << '\n';

    // --- 範例 4: find_if_not — 找第一個「不」符合條件的元素 ---
    // 這裡找第一個非正數;v 全部是正數,所以回傳 end()。
    auto neg = std::find_if_not(v.begin(), v.end(),
                                [](int x){ return x > 0; });
    std::cout << "first non-positive: "
              << (neg == v.end() ? "(none)" : std::to_string(*neg)) << '\n';

    // --- 範例 5: 在字串容器中找 "hello" ---
    // 證明 std::find 對任何能用 == 比較的型別都適用。
    std::vector<std::string> words{"foo", "bar", "hello", "world"};
    auto wit = std::find(words.begin(), words.end(), "hello");
    std::cout << "string search: " << (wit != words.end() ? *wit : "(none)") << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_1346_check_double();
    void leetcode_2351_first_repeating();
    void practical_find_user_by_id();
    void leetcode_1_two_sum_via_find();
    void practical_check_string_blacklist();
    leetcode_1346_check_double();
    leetcode_2351_first_repeating();
    practical_find_user_by_id();
    leetcode_1_two_sum_via_find();
    practical_check_string_blacklist();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================
//
// 選題原則:挑「自然就應該用 std::find/find_if」的題目,不挑為了考算法
//          而硬湊的題目。這幾題都能直接展現 STL 線性搜尋的價值。

// ----------------------------------------------------------------
// LeetCode 1346: 檢查整數及其兩倍是否存在
//                (Check If N and Its Double Exist)
// ----------------------------------------------------------------
// 題目:給整數陣列 arr,判斷是否存在兩個不同索引 i, j,使得 arr[i] == 2 * arr[j]。
//
// 為什麼用 std::find_if:
//   題意就是「找一個元素」滿足某條件 — 對每個 j,我們只要在「其他位置」
//   找有沒有等於 2*arr[j] 的元素。這是 find_if 的標準用法。
//
// 解法步驟:
//   1. 對每個位置 j 取出 arr[j],計算目標值 target = 2 * arr[j]。
//   2. 用 find_if 在整個陣列中尋找符合 (元素 == target 且不是同一個位置) 的元素。
//   3. 找到就回傳 true,全部掃完都沒找到就回傳 false。
//
// 複雜度:時間 O(n^2),空間 O(1)。
//        (若要 O(n) 可改用 unordered_set,但本題是為了示範 find_if 的語意。)
void leetcode_1346_check_double() {
    std::vector<int> arr{10, 2, 5, 3};       // 10 == 2*5 → true
    bool found = false;
    for (size_t j = 0; j < arr.size() && !found; ++j) {
        int target = 2 * arr[j];
        // 在整個陣列中找「值等於 target 且位置不是 j」的元素
        // ⚠️ 踩雷（本檔曾經寫錯）：不可以用 (&v - &arr[0]) 反推索引——
        //    lambda 參數 int v 是【副本】,住在堆疊上,與 vector 的堆積緩衝區
        //    不屬於同一個 array object,相減是 UB（ASan: invalid-pointer-pair）。
        //    正解：對 iterator 取距離,而不是對「參數的位址」動手腳。
        auto it = std::find_if(arr.begin(), arr.end(),
            [&](const int& v){
                const size_t idx = static_cast<size_t>(&v - arr.data());
                return v == target && idx != j;
            });
        if (it != arr.end()) found = true;
    }
    std::cout << "LC1346: " << (found ? "true" : "false") << '\n';
}

// ----------------------------------------------------------------
// LeetCode 2351: 首字母重複出現
//                (First Letter to Appear Twice)
// ----------------------------------------------------------------
// 題目:給字串 s,回傳「第一個出現第二次」的字元 (即第一個重複字元)。
//
// 為什麼用 std::find:
//   走訪每個字元時,在「目前已看過的字元集合」裡用 std::find 查它是否已存在;
//   存在 → 答案就是它。這是 find 最直觀的用法之一。
//
// 解法步驟:
//   1. 維護一個 vector<char> seen 紀錄看過的字元。
//   2. 走訪 s 的每個 c,先用 std::find 檢查 seen 是否已含 c。
//      * 已含 → c 就是答案,回傳。
//      * 未含 → 把 c 加進 seen 繼續。
//
// 複雜度:時間 O(n × k),k 為不同字元數 (英文 ≤ 26 → 視為常數)。
//        空間 O(k)。
void leetcode_2351_first_repeating() {
    std::string s = "abccbaacz";   // 'c' 是第一個重複的字母
    std::vector<char> seen;
    char ans = '?';
    for (char c : s) {
        if (std::find(seen.begin(), seen.end(), c) != seen.end()) {
            ans = c;
            break;
        }
        seen.push_back(c);
    }
    std::cout << "LC2351: " << ans << '\n';
}

// ----------------------------------------------------------------
// 實務範例: 在使用者清單中依 ID 查找特定使用者
// ----------------------------------------------------------------
// 場景:後端服務常常要在記憶體中的小型清單 (例如 cache、設定列表) 裡
//      依某個欄位找到對應紀錄。當資料量小、不想引入 map/set 容器時,
//      std::find_if 配上 lambda 是最簡潔自然的寫法。
//
// 為什麼用 find_if:
//   * 條件不是「整個物件相等」,而是「物件的某個欄位相等」。
//   * 用 lambda 把條件直接寫在呼叫端,意圖一目了然。
struct User {
    int id;
    std::string name;
};
void practical_find_user_by_id() {
    std::vector<User> users{
        {101, "Alice"},
        {102, "Bob"},
        {103, "Charlie"}
    };
    int target = 102;
    // lambda 補上「u.id == target」的條件,find_if 完成線性搜尋
    auto it = std::find_if(users.begin(), users.end(),
                           [target](const User& u){ return u.id == target; });
    if (it != users.end())
        std::cout << "user " << target << " = " << it->name << '\n';
    else
        std::cout << "user " << target << " not found\n";
}

// ----------------------------------------------------------------
// LeetCode 1: 兩數之和 (Two Sum) — 用 std::find 的簡單寫法
// ----------------------------------------------------------------
// 題目:給陣列 nums 與 target,回傳兩個元素之和為 target 的索引。
//
// 為什麼用 std::find:
//   對每個 nums[i],在「i 之後」的範圍用 find 找 target - nums[i] —
//   經典 O(n²) 解法 (最佳是 unordered_map O(n),這裡示範 find 用法)。
//
// 複雜度:時間 O(n²);空間 O(1)。
void leetcode_1_two_sum_via_find() {
    std::vector<int> nums{2, 7, 11, 15};
    int target = 9;
    int a = -1, b = -1;
    for (size_t i = 0; i < nums.size(); ++i) {
        int need = target - nums[i];
        auto it = std::find(nums.begin() + i + 1, nums.end(), need);
        if (it != nums.end()) {
            a = i; b = it - nums.begin();
            break;
        }
    }
    std::cout << "LC1: [" << a << "," << b << "]\n";
}

// ----------------------------------------------------------------
// 實務範例:檢查使用者輸入字串是否在黑名單
// ----------------------------------------------------------------
// 場景:留言系統有「禁止字串」清單 (小型);若使用者輸入命中其中之一就拒絕。
//      若清單未排序,find 是最直接的做法。
void practical_check_string_blacklist() {
    std::vector<std::string> blacklist{"hack", "exploit", "rm -rf", "spam"};
    std::vector<std::string> inputs{"hello", "exploit", "love"};
    for (auto& s : inputs) {
        auto it = std::find(blacklist.begin(), blacklist.end(), s);
        std::cout << s << ": " << (it != blacklist.end() ? "blocked" : "ok") << '\n';
    }
}

// 編譯: g++ -std=c++20 -Wall -Wextra find.cpp -o find

// === 預期輸出 ===
// find 7 at index 2
// find 100: not found
// first even = 4
// first non-positive: (none)
// string search: hello
// LC1346: true
// LC2351: c
// user 102 = Bob
// LC1: [0,1]
// hello: ok
// exploit: blocked
// love: ok
