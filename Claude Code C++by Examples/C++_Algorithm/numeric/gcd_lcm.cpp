// ============================================================
// std::gcd / std::lcm    (C++17 起)
// 分類 (Category): Numeric operations (數值演算法)
// 標頭檔 (Header):  <numeric>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/numeric/gcd
//   * https://en.cppreference.com/w/cpp/numeric/lcm
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 — 兩個老朋友:GCD 與 LCM                       │
// └────────────────────────────────────────────────────────────┘
//
// std::gcd / std::lcm 解的問題:
//
//   * gcd(a, b) — 最大公因數 (Greatest Common Divisor)
//                 「能同時整除 a 和 b 的最大正整數」
//   * lcm(a, b) — 最小公倍數 (Least Common Multiple)
//                 「能被 a 和 b 同時整除的最小正整數」
//
// 範例:
//   gcd(12, 18) = 6    (12 = 2×2×3,18 = 2×3×3,共同因數最多取 2×3 = 6)
//   lcm(4, 6)   = 12   (4 = 2×2,6 = 2×3,合併取 2×2×3 = 12)
//   gcd(0, n)   = n    (任何整數都能整除 0,因此 0 與 n 的 gcd 就是 |n|)
//   gcd(0, 0)   = 0    (約定)
//   lcm(0, n)   = 0    (約定 — 0 是任何數的倍數)
//
// 雖然「兩行迴圈」就能寫出歐幾里得演算法,但 C++17 把它放進標準庫之後:
//   * 不用每次重寫,還能避免常見 bug (例如忘記取絕對值、整數溢位)。
//   * constexpr — 編譯期就能算出。
//   * 自動推導共同型別 (std::common_type_t)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、底層原理 — 歐幾里得 (Euclidean) 演算法                  │
// └────────────────────────────────────────────────────────────┘
//
// gcd 使用「輾轉相除法」:
//
//   gcd(a, b) = gcd(b, a mod b)   (b ≠ 0)
//   gcd(a, 0) = |a|
//
// 證明簡述:任何同時整除 a 與 b 的數,也會整除 a - kb;反之亦然。
// 所以 (a, b) 與 (b, a mod b) 共有的因數集合相同 → gcd 相等。
//
// 實作上 std::gcd 通常是 binary-gcd (用位移取代除法,更快),
// 但結果與輾轉相除法等價。
//
// lcm 公式:
//
//   lcm(a, b) = |a × b| / gcd(a, b)
//
// 注意先除後乘以避免溢位:|a / gcd(a, b) × b|,標準實作會自動處理。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class M, class N>
//   constexpr std::common_type_t<M, N> gcd(M m, N n);
//
//   template <class M, class N>
//   constexpr std::common_type_t<M, N> lcm(M m, N n);
//
//   * 兩者皆為 constexpr (C++17)。
//   * M、N 必須是「整數型別」(integral) 且不是 bool。
//   * 回傳型別是兩者的「共同型別」std::common_type_t<M, N>。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、複雜度與注意事項 (Pitfalls)                            │
// └────────────────────────────────────────────────────────────┘
//
//   時間: O(log(min(|a|, |b|)))
//   空間: O(1)
//
//   1. 引數不能是 bool — 編譯失敗。
//   2. 結果型別是 std::common_type_t<M, N>:
//        gcd(int, long) → long
//        gcd(unsigned, int) 視平台規則。
//      若擔心混型別,自己先 static_cast。
//   3. 負數會被取絕對值處理 — gcd(-12, 18) = 6。
//      但若 |a| 大到無法用回傳型別表達 (例如 INT_MIN),為未定義行為。
//   4. lcm 結果若無法用回傳型別表達 → 未定義行為。
//      實務上若可能溢位,改用更大的型別 (long long, __int128, big-int)。
//   5. 只接受兩個引數;要算多個數的 gcd/lcm,可用 std::accumulate:
//        std::accumulate(v.begin(), v.end(), 0, [](int a, int b){
//            return std::gcd(a, b);
//        });
//      ※ 注意 init 用 0 — gcd(0, x) = |x|,不會影響結果。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、與其他語言/函式庫的對照                                 │
// └────────────────────────────────────────────────────────────┘
//
//   Python      math.gcd, math.lcm (Python 3.9+)
//   Java        BigInteger.gcd
//   JavaScript  無內建 — 自己實作
//   C++17 起    std::gcd / std::lcm — 終於不用每次手寫
//
// ============================================================

/*
補充筆記：std::gcd_lcm
  - std::gcd/std::lcm 處理整數最大公因數與最小公倍數。
  - lcm 可能溢位，即使最後數學結果看起來合理也要注意型別範圍。
  - 負數輸入會以絕對值語意處理，結果非負。
  - std::gcd_lcm 屬於數值演算法，常用來把迴圈中的累加、差分、內積或前綴和寫成標準形式。
  - accumulate 的初始值型別會決定累加結果型別；用 0 累加 double 或 long long 時可能不小心截斷或溢位。
  - reduce 可重新排序運算順序以利平行化，因此 binary operation 必須接近結合律且沒有副作用；浮點數結果可能和 accumulate 不完全相同。
  - partial_sum 和 adjacent_difference 互為常見搭配，可用來建立前綴和與差分陣列。
  - gcd/lcm 對整數工作；lcm 可能溢位，即使最後型別是 long long 也要注意輸入大小。
  - 數值演算法最容易出錯的是型別和溢位，不是語法；工作程式應刻意選擇初始值型別。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::gcd / std::lcm (C++17)
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::gcd / std::lcm 是哪個版本加入的?在哪個標頭檔?回傳型別怎麼決定?
//     答:C++17 加入,放在 <numeric> (不是 <cmath>),兩者皆為 constexpr。回傳型別是
//         std::common_type_t<M, N>,例如 gcd(int, long) 得 long。引數必須是「整數型別、
//         且不是 bool」—— 傳 bool 或浮點數是 ill-formed,編譯當場失敗。
//     追問:複雜度?(O(log min(|a|, |b|));實作常用 binary GCD 取代除法,但那是實作選擇)
//
// 🔥 Q2. 負數丟進去會怎樣?gcd(0,0)、gcd(0,n)、lcm(0,n) 各是多少?
//     答:標準規定 gcd 回傳的是 |m| 與 |n| 的最大公因數,所以結果恆為非負:gcd(-12, 18) = 6,
//         lcm 同樣取絕對值語意。邊界值:gcd(0, 0) = 0、gcd(0, n) = |n| (任何數都整除 0)、
//         lcm(0, n) = 0。
//     追問:有沒有例外?(有 —— 若 |m| 或 |n| 無法用回傳型別表示,例如 INT_MIN,則為 UB)
//
// ⚠️ 陷阱. 要算「一整個 vector 的 gcd」而用 accumulate 包起來時,init 該寫多少?
//     答:gcd 要寫 0、lcm 要寫 1,也就是各自的單位元。因為 gcd(0, x) = |x|、lcm(1, x) = |x|,
//         init 才不會汙染結果。若把 gcd 的 init 誤寫成 1,gcd(1, x) 恆為 1 → 答案永遠是 1,
//         而且完全不會報錯,是很典型的「安靜的錯」。
//     為什麼會錯:直覺套用「求積 init 用 1」的習慣,忘了不同運算的單位元不同。
//
// Q3. lcm 為什麼還是可能溢位?
//     答:實作用 |a| / gcd(a, b) * |b| 的順序來避開「先乘再除」的中間值溢位,但那只解決中間
//         結果;若最小公倍數本身就超出回傳型別的表示範圍,依標準仍是 UB。多個數連續取 lcm
//         時特別容易踩到 —— 有疑慮就先換成 long long 之類更寬的型別。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: 最基本用法 ---
    std::cout << "gcd(12, 18) = " << std::gcd(12, 18) << '\n';
    std::cout << "gcd(17, 31) = " << std::gcd(17, 31) << '\n';   // 互質 → 1
    std::cout << "lcm(4, 6)   = " << std::lcm(4, 6)   << '\n';
    std::cout << "lcm(7, 9)   = " << std::lcm(7, 9)   << '\n';

    // --- 範例 2: 邊界 — 0 的特殊行為 ---
    std::cout << "gcd(0, 5)   = " << std::gcd(0, 5)   << '\n';   // 5
    std::cout << "gcd(0, 0)   = " << std::gcd(0, 0)   << '\n';   // 0
    std::cout << "lcm(0, 5)   = " << std::lcm(0, 5)   << '\n';   // 0

    // --- 範例 3: 負數會被取絕對值 ---
    std::cout << "gcd(-12, 18) = " << std::gcd(-12, 18) << '\n'; // 6
    std::cout << "lcm(-4, 6)   = " << std::lcm(-4, 6)   << '\n'; // 12

    // --- 範例 4: constexpr — 編譯期計算 ---
    constexpr int g = std::gcd(1024, 768);
    constexpr int l = std::lcm(3, 7);
    std::cout << "constexpr gcd(1024, 768) = " << g << '\n';     // 256
    std::cout << "constexpr lcm(3, 7)      = " << l << '\n';     // 21

    // --- 範例 5: 用 accumulate 求多個整數的 gcd/lcm ---
    std::vector<int> v{12, 18, 24, 36};
    int gv = std::accumulate(v.begin(), v.end(), 0,
                             [](int a, int b){ return std::gcd(a, b); });
    int lv = std::accumulate(v.begin(), v.end(), 1,
                             [](int a, int b){ return std::lcm(a, b); });
    std::cout << "gcd of {12,18,24,36} = " << gv << '\n';        // 6
    std::cout << "lcm of {12,18,24,36} = " << lv << '\n';        // 72

    // === LeetCode / 實務範例 ===
    void leetcode_1071_gcd_of_strings();
    void leetcode_914_x_of_a_kind();
    void practical_reduce_fraction();
    void leetcode_2807_insert_gcd_in_linked_list_concept();
    void practical_meeting_repeat_period();
    leetcode_1071_gcd_of_strings();
    leetcode_914_x_of_a_kind();
    practical_reduce_fraction();
    leetcode_2807_insert_gcd_in_linked_list_concept();
    practical_meeting_repeat_period();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 1071: 字串的最大公因子 (Greatest Common Divisor of Strings)
// ----------------------------------------------------------------
// 題目:給兩個字串 s, t,求最大「重複字串 X」使得 s 與 t 都是 X 重複若干次。
//
// 為什麼用 std::gcd:
//   有個美妙的數學結論 — 若 s 和 t 是同一個 X 重複而成,則 s+t == t+s。
//   且最大公因子的長度就是 gcd(|s|, |t|)。所以解法非常乾淨:
//     1. 若 s+t != t+s → 沒有公因子,回傳 ""。
//     2. 否則回傳 s 的前 gcd(|s|, |t|) 個字元。
//
// 複雜度:時間 O(|s| + |t|);空間 O(|s| + |t|)。
void leetcode_1071_gcd_of_strings() {
    auto gcdOfStrings = [](const std::string& s, const std::string& t) -> std::string {
        if (s + t != t + s) return "";
        return s.substr(0, std::gcd(s.size(), t.size()));
    };
    std::cout << "LC1071 gcdOfStrings(\"ABCABC\", \"ABC\") = \""
              << gcdOfStrings("ABCABC", "ABC") << "\"\n";
    std::cout << "LC1071 gcdOfStrings(\"ABABAB\", \"ABAB\") = \""
              << gcdOfStrings("ABABAB", "ABAB") << "\"\n";
    std::cout << "LC1071 gcdOfStrings(\"LEET\", \"CODE\") = \""
              << gcdOfStrings("LEET", "CODE") << "\"\n";
}

// ----------------------------------------------------------------
// LeetCode 914: 卡牌分組 (X of a Kind in a Deck of Cards)
// ----------------------------------------------------------------
// 題目:把卡牌依數字分組後,每組卡牌數量是否能找到一個 X >= 2,使得所有
//       組的卡牌數量都是 X 的倍數?
//
// 為什麼用 std::gcd:
//   只要算出「所有組數量的 gcd」是否 >= 2 即可。
//   gcd >= 2 → 存在這樣的 X (就是 gcd 本身);
//   gcd  < 2 → 不存在。
//
// 複雜度:時間 O(N + |groups| × log(max)),空間 O(|groups|)。
void leetcode_914_x_of_a_kind() {
    auto hasGroupsSizeX = [](std::vector<int> deck) {
        // 範圍 0..9999 (LC 題目限制 deck[i] < 10000),用陣列計數
        std::vector<int> count(10000, 0);
        for (int x : deck) ++count[x];
        // 從 0 開始 reduce — gcd(0, n) = n,正好作為單位元
        int g = 0;
        for (int c : count) if (c > 0) g = std::gcd(g, c);
        return g >= 2;
    };
    std::cout << "LC914 [1,2,3,4,4,3,2,1] = "
              << (hasGroupsSizeX({1,2,3,4,4,3,2,1}) ? "true" : "false") << '\n';
    std::cout << "LC914 [1,1,1,2,2,2,3,3] = "
              << (hasGroupsSizeX({1,1,1,2,2,2,3,3}) ? "true" : "false") << '\n';
}

// ----------------------------------------------------------------
// 實務範例:分數約分 (Reduce Fraction to Lowest Terms)
// ----------------------------------------------------------------
// 場景:把分數 num/den 化為最簡分式 (例如 6/8 → 3/4)。
//       常見於財務計算、UI 顯示比例 (16:9、4:3 螢幕比)、化學配比等。
//
// 為什麼用 std::gcd:
//   把分子分母同時除以 gcd(num, den) 即得最簡分式 — 教科書級解法。
//
// 複雜度:時間 O(log(min(|num|, |den|)));空間 O(1)。
void practical_reduce_fraction() {
    auto reduce = [](int num, int den) {
        int g = std::gcd(num, den);
        return std::pair<int,int>{num / g, den / g};
    };
    auto print = [&](int n, int d){
        auto [a, b] = reduce(n, d);
        std::cout << n << "/" << d << " = " << a << "/" << b << '\n';
    };
    print(6, 8);     // 3/4
    print(1920, 1080); // 16/9
    print(100, 25);  // 4/1
}

// ----------------------------------------------------------------
// LeetCode 2807 概念:在連結串列相鄰元素間插入 GCD (簡化為陣列版)
// 難度: medium
// ----------------------------------------------------------------
// 題目簡化:給陣列 nums,在每對相鄰元素之間插入它們的 gcd。
//
// 為什麼用 std::gcd:
//   題意 100% 對應 — gcd 是 C++17 起的標準函式,直接呼叫即可。
//
// 複雜度:時間 O(n × log(max));空間 O(n)。
void leetcode_2807_insert_gcd_in_linked_list_concept() {
    std::vector<int> nums{18, 6, 10, 3};
    std::vector<int> ans;
    for (size_t i = 0; i < nums.size(); ++i) {
        ans.push_back(nums[i]);
        if (i + 1 < nums.size())
            ans.push_back(std::gcd(nums[i], nums[i + 1]));
    }
    std::cout << "LC2807:";
    for (int x : ans) std::cout << ' ' << x;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:多個會議時段的「最小重複週期」
// ----------------------------------------------------------------
// 場景:三個會議分別每 6、9、12 分鐘提示一次;
//      要找它們「同時提示」的最小週期 — 即 lcm(6, 9, 12) = 36 分鐘。
void practical_meeting_repeat_period() {
    std::vector<int> intervals{6, 9, 12};
    int period = std::accumulate(intervals.begin(), intervals.end(), 1,
                                 [](int a, int b){ return std::lcm(a, b); });
    std::cout << "joint period: " << period << " minutes\n";
}

// === 預期輸出 (Expected output) ===
// gcd(12, 18) = 6
// gcd(17, 31) = 1
// lcm(4, 6)   = 12
// lcm(7, 9)   = 63
// gcd(0, 5)   = 5
// gcd(0, 0)   = 0
// lcm(0, 5)   = 0
// gcd(-12, 18) = 6
// lcm(-4, 6)   = 12
// constexpr gcd(1024, 768) = 256
// constexpr lcm(3, 7)      = 21
// gcd of {12,18,24,36} = 6
// lcm of {12,18,24,36} = 72
// LC1071 gcdOfStrings("ABCABC", "ABC") = "ABC"
// LC1071 gcdOfStrings("ABABAB", "ABAB") = "AB"
// LC1071 gcdOfStrings("LEET", "CODE") = ""
// LC914 [1,2,3,4,4,3,2,1] = true
// LC914 [1,1,1,2,2,2,3,3] = false
// 6/8 = 3/4
// 1920/1080 = 16/9
// 100/25 = 4/1
// LC2807: 18 6 6 2 10 1 3
// joint period: 36 minutes
