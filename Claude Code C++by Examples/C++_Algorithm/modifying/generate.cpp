// ============================================================
// std::generate / std::generate_n
// 分類 (Category): Modifying sequence operations (修改型)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/generate
//   * https://en.cppreference.com/w/cpp/algorithm/generate_n
//   * https://cplusplus.com/reference/algorithm/generate/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::generate 解決的問題:
//
//   「對範圍中每個位置,呼叫一次無參數的 g(),把回傳值寫進去。」
//
// 它的兄弟 std::fill 是「每個位置都填同一個 value」;
// 而 std::generate 是「每個位置呼叫一次 g(),所以每次寫入的值可以不同」。
//
//   ┌──────────┬──────────────────────────────────────────────┐
//   │ fill     │ 寫入「同一個 value」                          │
//   │ generate │ 每次呼叫「g()」取得新值                        │
//   └──────────┴──────────────────────────────────────────────┘
//
// 所以 generate 的 g 通常帶有「狀態」 — 例如:
//   * 一個遞增計數器 (產生 1, 2, 3, ...)
//   * 一個 PRNG (產生隨機數)
//   * 一個讀取串流的函式 (從 socket 讀下一個資料)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、g 必須是「無參數可呼叫物件」                           │
// └────────────────────────────────────────────────────────────┘
//
// generate 呼叫 g 的方式就是 g() — 不會傳元素的當前值,也不會傳索引。
// 如果你需要「依索引產生值」,要自己在 lambda 裡用一個捕獲的計數器:
//
//   int i = 0;
//   std::generate(v.begin(), v.end(), [&]{ return i++ * i; });
//
// 「讀取目前狀態 → 產生值 → 改變狀態」是 generate 的常見模式。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、與 std::iota 的差別                                    │
// └────────────────────────────────────────────────────────────┘
//
//   * std::iota 專門做「連續遞增」(0, 1, 2, ...) — 一行寫完。
//   * std::generate 是「任意產生規則」 — 自由度最高,但要自己寫 lambda。
//
// 要連續整數用 iota;要規則性序列、隨機數、外部來源用 generate。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class FwdIt, class Generator>
//   void generate(FwdIt first, FwdIt last, Generator g);
//
//   template <class OutputIt, class Size, class Generator>
//   OutputIt generate_n(OutputIt first, Size n, Generator g);
//
//   * C++20 起為 constexpr。
//   * C++17 起有執行策略多載。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、回傳值 (Return value)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   generate   : void
//   generate_n : 指向「最後寫入位置的下一個」
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 恰好 N 次呼叫 g
//   空間: O(1)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. g 通常需要 mutable lambda 或捕獲參考 (因為要修改內部狀態)。
//   2. 與平行版本搭配時,g 必須是執行緒安全 (PRNG 通常不是)。
//   3. generate_n + back_inserter 是「動態追加 N 個自訂值」的最好寫法。
//   4. g 的順序保證: 非平行版按 first→last 依序呼叫。
//
// ============================================================

/*
補充筆記：std::generate
  - generate 會對範圍中每個位置呼叫 generator，適合產生序列或測試資料。
  - generator 若保存狀態，呼叫順序就是資料內容的一部分。
  - 不要讓 generator 回傳懸空 reference；通常回傳值最清楚。
  - std::generate 會改變目標範圍內容或元素順序；呼叫前要確認輸出範圍空間足夠、iterator 有效。
  - copy/transform/generate/fill 寫入目的地時不會自動擴容，除非你使用 back_inserter 這類 insert iterator。
  - remove/unique 不會真的縮短容器，只把保留元素移到前面並回傳新邏輯結尾；還要搭配 erase 才會改變 size。
  - reverse/rotate/swap_ranges 會改變元素位置；若外部保存索引或 iterator，要重新確認是否仍指向預期元素。
  - move algorithm 會把元素搬到目的地，來源仍有效但值可能改變；後續只能重新指定或安全銷毀。
  - shuffle/sample 需要亂數引擎；不要每次呼叫都用同一個固定種子，除非你刻意要可重現測試結果。
*/

// ===========================================================================
// 【面試題】std::generate / std::generate_n
// ---------------------------------------------------------------------------
// 🔥 Q1. generate 和 fill 差在哪?generator 的簽章有什麼限制?
//     答:fill 把「同一個值」寫進每個位置;generate 對每個位置呼叫一次 g(),
//         所以每次寫入的值可以不同。g 必須是「無參數」的可呼叫物件——generate
//         不會把元素現值或索引傳給它,要依索引產生值就得自己在 lambda 裡捕獲計數器。
//     追問:那要「依現有元素算出新值」呢?(答:那是 std::transform 的工作,不是 generate)
//
// 🔥 Q2. 什麼時候用 iota、什麼時候用 generate?
//     答:單純的連續遞增(0, 1, 2, ...)用 <numeric> 的 std::iota,一行就好;
//         規則較複雜、要隨機數、或值來自外部來源(串流、socket)時用 generate。
//         generate 的自由度最高,代價是要自己寫帶狀態的 lambda。
//
// ⚠️ 陷阱. std::generate 加上 C++17 的平行執行策略,g 還能保有內部狀態嗎?
//     答:不行。帶狀態的 generator(計數器、PRNG)幾乎都不是執行緒安全的,
//         平行版會由多個執行緒同時呼叫 g,造成資料競爭,產生的順序也不再可預期。
//         要平行就得讓 g 無狀態,或每個執行緒各自持有獨立的 PRNG。
//     為什麼會錯:序列版看起來「就是一個由前往後的迴圈」,大家便假設加上 policy 只是
//         變快而已;實際上執行策略把「順序」與「執行緒安全」的責任都丟回給你。
//
// Q3. generate 和 generate_n 的回傳值?
//     答:generate 回傳 void;generate_n 回傳最後寫入位置的下一個。
//         generate_n(std::back_inserter(v), n, g) 是「動態追加 n 個自訂值」的標準寫法,
//         因為 generate_n 同樣不會替你擴容。
// ===========================================================================

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <random>
#include <sstream>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: 用遞增整數填入 ---
    std::vector<int> v(5);
    int counter = 0;
    std::generate(v.begin(), v.end(), [&]{ return ++counter; });
    std::cout << "generate 1..5: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 2: 用 lambda 產生平方序列 ---
    std::vector<int> sq(6);
    int i = 0;
    std::generate(sq.begin(), sq.end(),
                  [&]{ ++i; return i*i; });
    std::cout << "squares: ";
    for (int x : sq) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 3: generate_n + back_inserter 動態產生 ---
    std::vector<int> r;
    int seed = 0;
    std::generate_n(std::back_inserter(r), 4, [&]{ seed += 10; return seed; });
    std::cout << "generate_n: ";
    for (int x : r) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 4: 結合 PRNG 產生隨機數 (固定 seed 以便重現) ---
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 9);
    std::vector<int> rd(5);
    std::generate(rd.begin(), rd.end(), [&]{ return dist(rng); });
    std::cout << "random:   ";
    for (int x : rd) std::cout << x << ' ';
    std::cout << '\n';

    // === 實務範例 ===
    void practical_random_test_data();
    void practical_uuid_like_generator();
    void leetcode_1929_concat_double_array();
    void practical_fibonacci_sequence();
    practical_random_test_data();
    practical_uuid_like_generator();
    leetcode_1929_concat_double_array();
    practical_fibonacci_sequence();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================
//
// generate 的天然用途偏向「產生資料」 — LeetCode 上「需要產生序列」
// 的題目相對少。最自然的應用是「測試資料產生」「序號產生」等實務情境。

// ----------------------------------------------------------------
// 實務範例 1:產生隨機測試資料 (test fixture)
// ----------------------------------------------------------------
// 場景:寫單元測試或 benchmark 時,需要快速產生大量隨機輸入。
//
// 為什麼用 std::generate:
//   把一個 PRNG 包進 lambda,對整個容器一次填滿,程式碼最短。
//   固定 seed 還能確保測試「可重現」。
void practical_random_test_data() {
    std::mt19937 rng(2026);
    std::uniform_int_distribution<int> dist(1, 100);
    std::vector<int> data(8);
    std::generate(data.begin(), data.end(), [&]{ return dist(rng); });
    std::cout << "Test data (n=8): ";
    for (int x : data) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例 2:序號產生器 (sequence ID generator)
// ----------------------------------------------------------------
// 場景:為新建的 N 筆訂單產生連號序號 (ORD-00001, ORD-00002, ...)。
//
// 為什麼用 std::generate_n:
//   * 「個數已知 (N 筆)」+「每次值不同」 — 完美對應 generate_n。
//   * 配合 back_inserter,容器會自動長到 N 個。
void practical_uuid_like_generator() {
    std::vector<std::string> ids;
    int counter = 0;
    std::generate_n(std::back_inserter(ids), 5, [&]{
        std::ostringstream os;
        os << "ORD-" << std::setw(5) << std::setfill('0') << ++counter;
        return os.str();
    });
    std::cout << "Order IDs: ";
    for (auto& s : ids) std::cout << s << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1929: 串聯兩次陣列 (Concatenation of Array)
// ----------------------------------------------------------------
// 題目:給陣列 nums (長度 n),回傳 ans (長度 2n),
//      ans[i] = nums[i % n] (亦即 nums + nums)。
//
// 為什麼用 std::generate_n:
//   想成「依索引函數產生 2n 個元素」 — generate_n 配 lambda 直觀表達。
//   雖然這題也可用 copy 兩次完成;generate_n 示範「逐元素產生」的彈性。
//
// 複雜度:時間 O(n);空間 O(n)。
void leetcode_1929_concat_double_array() {
    std::vector<int> nums{1, 3, 2, 1};
    int n = nums.size();
    std::vector<int> ans;
    int i = 0;
    std::generate_n(std::back_inserter(ans), 2 * n,
                    [&]{ return nums[i++ % n]; });
    std::cout << "LC1929:";
    for (int x : ans) std::cout << ' ' << x;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:費氏數列 (Fibonacci sequence) 生成
// ----------------------------------------------------------------
// 場景:演算法練習、教學示範,要產生前 N 個費氏數。
//      用 generate 配上閉包 (兩個變數捕獲 a, b) 一行完成。
void practical_fibonacci_sequence() {
    std::vector<long long> fib(10);
    long long a = 0, b = 1;
    std::generate(fib.begin(), fib.end(), [&]{
        long long cur = a;
        long long next = a + b;
        a = b;
        b = next;
        return cur;
    });
    std::cout << "fib(10):";
    for (long long x : fib) std::cout << ' ' << x;
    std::cout << '\n';
}

// === 預期輸出 (Expected output) ===
// generate 1..5: 1 2 3 4 5
// squares: 1 4 9 16 25 36
// generate_n: 10 20 30 40
// random:   ... (mt19937(42) 固定序列,實際數字依實作可能略不同)
// Test data (n=8): ... (mt19937(2026) 固定序列)
// Order IDs: ORD-00001 ORD-00002 ORD-00003 ORD-00004 ORD-00005
// LC1929: 1 3 2 1 1 3 2 1
// fib(10): 0 1 1 2 3 5 8 13 21 34
