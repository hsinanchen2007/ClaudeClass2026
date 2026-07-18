// ============================================================
// std::equal
// 分類 (Category): Non-modifying sequence operations (非修改型)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/equal
//   * https://cplusplus.com/reference/algorithm/equal/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::equal 解的問題很單純:
//
//   「兩段資料,是不是『逐元素一一對應、值都相等』?」
//
// 它不在乎兩端是不是同一種容器、是不是同一個迭代器型別、
// 甚至 value_type 也可以不同 (只要能用 == 或自訂 BinaryPred 比較)。
// 它只在乎一件事 — 配對起來的元素值是否兩兩相等。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼有「三參數版」與「四參數版」?                   │
// └────────────────────────────────────────────────────────────┘
//
// 歷史上 C++98/03 的 std::equal 只有「三參數版」:
//
//   bool equal(first1, last1, first2);
//
// 這個版本「假設」第二段資料的長度 ≥ 第一段;若實際短了,行為是 UB。
// 這是 C++ 標準函式庫裡很惡名昭彰的「不安全 API」之一。
//
// C++14 起加入了「四參數版」:
//
//   bool equal(first1, last1, first2, last2);
//
// 它會自己檢查兩段長度,長度不同就直接回 false,絕不會超讀。
// **強烈建議永遠使用四參數版本**,除非你很確定第二段一定夠長
// (例如比對一段 buffer 的開頭是不是某個固定 magic number)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、equal vs mismatch 的差別                               │
// └────────────────────────────────────────────────────────────┘
//
//   * equal    回傳 bool   — 「整段都一樣嗎?」
//   * mismatch 回傳 (it1, it2) — 「第一個不一樣的位置在哪?」
//
//   兩者背後的掃描邏輯一樣,差別只在「結果想拿到什麼」。
//   想要結果並做後續處理,用 mismatch;只要 yes/no,用 equal。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   // 三參數 (C++98) — 不安全,避免使用
//   template <class InputIt1, class InputIt2>
//   bool equal(InputIt1 first1, InputIt1 last1, InputIt2 first2);
//
//   // 四參數 (C++14) — 安全,推薦
//   template <class InputIt1, class InputIt2>
//   bool equal(InputIt1 first1, InputIt1 last1,
//              InputIt2 first2, InputIt2 last2);
//
//   // 兩者皆有可帶 BinaryPred 的多載
//
//   * C++20 起為 constexpr。
//   * C++17 起有執行策略多載。
//   * C++14 後若兩端都是 RandomAccessIterator,先 O(1) 比長度就能短路。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 最多 min(N1, N2) 次比較,O(n)
//   空間: O(1)
//   (RandomAccess + 四參數版可在長度不同時 O(1) 直接回 false)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 三參數版在第二段過短時是 UB!永遠用四參數版本。
//   2. 容器型別可以不同 — vector<int> 與 list<int> 同內容也會回 true。
//   3. 兩個空範圍視為「相等」(true)。
//   4. 對自訂型別:預設用 operator==,沒有就需要自訂 BinaryPred。
//   5. 浮點數比較不要直接用 ==,改傳一個容差判斷的 lambda。
//
// ============================================================

/*
補充筆記：std::equal
  - equal 比較兩段範圍元素是否逐一相等。
  - 使用只有 first2 的 overload 時，第二段必須足夠長。
  - 若比較浮點數或自訂型別，通常應提供 predicate 定義可接受誤差或欄位。
  - std::equal 會改變目標範圍內容或元素順序；呼叫前要確認輸出範圍空間足夠、iterator 有效。
  - copy/transform/generate/fill 寫入目的地時不會自動擴容，除非你使用 back_inserter 這類 insert iterator。
  - remove/unique 不會真的縮短容器，只把保留元素移到前面並回傳新邏輯結尾；還要搭配 erase 才會改變 size。
  - reverse/rotate/swap_ranges 會改變元素位置；若外部保存索引或 iterator，要重新確認是否仍指向預期元素。
  - move algorithm 會把元素搬到目的地，來源仍有效但值可能改變；後續只能重新指定或安全銷毀。
  - shuffle/sample 需要亂數引擎；不要每次呼叫都用同一個固定種子，除非你刻意要可重現測試結果。
*/
#include <algorithm>
#include <cctype>     // std::tolower
#include <iostream>
#include <list>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> a{1, 2, 3, 4};
    std::list<int>   b{1, 2, 3, 4};
    std::vector<int> c{1, 2, 9, 4};

    std::cout << std::boolalpha;

    // --- 範例 1: 不同容器 (vector vs list),內容相同 ---
    bool ab = std::equal(a.begin(), a.end(), b.begin(), b.end());
    std::cout << "vector vs list (same): " << ab << '\n';

    // --- 範例 2: 內容不同 ---
    bool ac = std::equal(a.begin(), a.end(), c.begin(), c.end());
    std::cout << "a vs c (different):    " << ac << '\n';

    // --- 範例 3: 長度不同 (四參數版直接短路為 false) ---
    std::vector<int> shorter{1, 2, 3};
    bool sh = std::equal(a.begin(), a.end(), shorter.begin(), shorter.end());
    std::cout << "different length:      " << sh << '\n';

    // --- 範例 4: 自訂述詞 — 字串大小寫不敏感比對 ---
    std::string s1 = "Hello";
    std::string s2 = "hElLo";
    bool ci = std::equal(s1.begin(), s1.end(), s2.begin(), s2.end(),
                         [](char x, char y){
                             return std::tolower(x) == std::tolower(y);
                         });
    std::cout << "case-insensitive:      " << ci << '\n';

    // --- 範例 5: 兩個空範圍視為相等 ---
    std::vector<int> e1, e2;
    std::cout << "empty == empty:        "
              << std::equal(e1.begin(), e1.end(), e2.begin(), e2.end()) << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_242_valid_anagram();
    void leetcode_125_palindrome();
    void practical_buffer_equal();
    void leetcode_796_rotate_string_via_equal();
    void practical_protocol_handshake_check();
    leetcode_242_valid_anagram();
    leetcode_125_palindrome();
    practical_buffer_equal();
    leetcode_796_rotate_string_via_equal();
    practical_protocol_handshake_check();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 242: 有效的字母異位詞 (Valid Anagram)
// ----------------------------------------------------------------
// 題目:給字串 s 與 t,t 是否為 s 的字母異位詞 (字元組成相同、順序不同)?
//
// 為什麼用 std::equal:
//   anagram 的判定法之一:把兩字串排序後逐字元比對。
//   排序後若內容相同,即代表字元組成相同 — 這就是 std::equal 的工作。
//
// 解法步驟:
//   1. 長度不同 → 直接 false (短路)。
//   2. 兩字串各自 sort。
//   3. std::equal 四參數版判斷是否完全相同。
//
// 複雜度:時間 O(n log n) (排序);空間 O(n) (這裡排了原字串)。
void leetcode_242_valid_anagram() {
    std::string s = "anagram";
    std::string t = "nagaram";
    if (s.size() != t.size()) { std::cout << "LC242: false\n"; return; }
    std::sort(s.begin(), s.end());
    std::sort(t.begin(), t.end());
    bool ok = std::equal(s.begin(), s.end(), t.begin(), t.end());
    std::cout << "LC242: " << std::boolalpha << ok << '\n';
}

// ----------------------------------------------------------------
// LeetCode 125 變體: 判斷字串是否為迴文 (Palindrome)
// ----------------------------------------------------------------
// 題目簡化版:給字串 s (僅含小寫英文),判斷是否為迴文。
//
// 為什麼用 std::equal:
//   迴文的定義:正讀 == 反讀。
//   只要把「前半段」與「後半段反向」做 std::equal 比對即可。
//
// 解法步驟:
//   1. 用 s.begin() ~ s.begin()+n/2 取前半段。
//   2. 用 s.rbegin() 反向迭代器取後半段反向視角。
//   3. std::equal 比對 → 相等代表是迴文。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_125_palindrome() {
    std::string s = "racecar";
    bool palin = std::equal(s.begin(), s.begin() + s.size() / 2,
                            s.rbegin());
    std::cout << "LC125: " << std::boolalpha << palin << '\n';
}

// ----------------------------------------------------------------
// 實務範例: 校驗兩個 byte buffer 內容是否相同
// ----------------------------------------------------------------
// 場景:檔案複製或網路傳輸後,要驗證兩個 byte buffer 完全一致。
//      四參數 std::equal 同時檢查長度與內容,寫法最乾淨。
void practical_buffer_equal() {
    std::vector<unsigned char> src{0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x11};
    std::vector<unsigned char> dst{0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x11};
    bool same = std::equal(src.begin(), src.end(),
                           dst.begin(), dst.end());
    std::cout << "buffer equal: " << std::boolalpha << same << '\n';
}

// ----------------------------------------------------------------
// LeetCode 796: 旋轉字串 (Rotate String) — 用 std::equal 比對
// ----------------------------------------------------------------
// 題目:給兩字串 s 與 goal,判斷 goal 是否可由 s 經過某次旋轉得到。
//
// 為什麼用 std::equal:
//   經典 trick:s + s 包含所有 s 的旋轉結果;
//   用 std::equal 在 s+s 上尋找 goal 是否為「某偏移處的子字串」。
//
// 複雜度:時間 O(n²) (找偏移);空間 O(n) (s + s)。
void leetcode_796_rotate_string_via_equal() {
    std::string s = "abcde", goal = "cdeab";
    if (s.size() != goal.size()) { std::cout << "LC796: false\n"; return; }
    std::string ss = s + s;
    bool ok = false;
    for (size_t i = 0; i + goal.size() <= ss.size(); ++i) {
        if (std::equal(goal.begin(), goal.end(), ss.begin() + i)) {
            ok = true; break;
        }
    }
    std::cout << "LC796: " << std::boolalpha << ok << '\n';
}

// ----------------------------------------------------------------
// 實務範例:協定握手 (handshake) 比對魔術數字 (magic number)
// ----------------------------------------------------------------
// 場景:許多檔案格式 / 網路協定一開頭都會有「魔術數字」,
//      接收端用 std::equal 比對前 N 個 byte 是否符合預期。
void practical_protocol_handshake_check() {
    std::vector<unsigned char> expected{0xCA, 0xFE, 0xBA, 0xBE};
    std::vector<unsigned char> packet  {0xCA, 0xFE, 0xBA, 0xBE, 0x00, 0x01, 0x02};
    bool ok = std::equal(expected.begin(), expected.end(), packet.begin());
    std::cout << "handshake match: " << std::boolalpha << ok << '\n';
}

// === 預期輸出 (Expected output) ===
// vector vs list (same): true
// a vs c (different):    false
// different length:      false
// case-insensitive:      true
// empty == empty:        true
// LC242: true
// LC125: true
// buffer equal: true
// LC796: true
// handshake match: true
