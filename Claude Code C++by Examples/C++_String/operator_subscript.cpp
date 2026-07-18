// =============================================================================
// 檔名: operator_subscript.cpp
// 主題: std::string::operator[] (無邊界檢查的元素存取)
// 參考連結 References:
//   cppreference: https://en.cppreference.com/cpp/string/basic_string/operator_at
//   cplusplus.com: https://cplusplus.com/reference/string/string/operator[]/
// =============================================================================
//
// 【函式資訊 Information】
//   reference       operator[](size_type pos);
//   const_reference operator[](size_type pos) const;
//
// 回傳:
//   - pos < size()      : 第 pos 個字元的參考。
//   - pos == size()     : 在 const 物件上回傳結尾 '\0' 的參考 (C++11 起)。
//                          在非 const 物件上,讀取合法但不可寫入。
//   - pos > size()      : Undefined Behavior。
//
// =============================================================================
//
// 【詳細解釋 Explanation】
//
// (一) operator[] 的設計哲學:零成本抽象 (Zero-Overhead Principle)
// ----------------------------------------------------------------------------
// C++ 的設計信條之一是「你不為你不用的東西付費」。operator[] 沒有邊界檢查,
// 因為:
//   1. 多數時候 caller 已經透過 size()、迴圈邊界保證 pos 合法。
//   2. 邊界檢查會增加分支,影響 CPU 分支預測與 vectorization。
//   3. 對於需要安全的人,標準提供了 at() (帶檢查、會丟例外)。
// 這就是 [] 與 at() 並存的根本理由 — 給開發者「在效能與安全間自選」的權利。
//
// (二) 與 at() 的關鍵差異
// ----------------------------------------------------------------------------
//   at(pos)        : 有邊界檢查,越界丟 std::out_of_range → 安全但有額外開銷
//   operator[](pos): 無邊界檢查,越界 → Undefined Behavior → 較快
//
// 經驗法則:
//   - 在「迴圈內、條件已保證合法」的熱路徑 → operator[]
//   - 在「外部輸入、不確定範圍」的安全邊界 → at
//
// (三) C++11 之後關於 s[s.size()] 的特殊保證
// ----------------------------------------------------------------------------
// C++11 標準規定 std::string 的緩衝區末端必須有一個有效的 '\0'。
// 這意味著:
//   - 對 const string,s[s.size()] 合法,且回傳 '\0'。
//   - 對非 const string,s[s.size()] 可讀取 (回傳 '\0'),但「寫入非 '\0' 值」
//     是 Undefined Behavior。
// 這個保證讓 c_str() 與 data() 都能安全地把 string 當 C-string 用。
//
// (四) 回傳「reference」而非 value 的原因
// ----------------------------------------------------------------------------
// 回傳 char& 讓你可以像陣列一樣讀寫:s[0] = 'X';
// 若回傳 char by value,就只能讀不能寫。STL 為了讓 string 行為與 raw array
// 一致,選擇回傳 reference。
//
// (五) 時間複雜度與 cache 友善性
// ----------------------------------------------------------------------------
// 複雜度: O(1) — 直接以 base + offset 計算位址。
// 連續迴圈走訪整個字串時,記憶體存取是 sequential、cache-friendly 的,
// 比 std::list 等指標型容器快上數十倍。
//
// (六) Iterator / Pointer / Reference 失效規則
// ----------------------------------------------------------------------------
// 從 operator[] 取得的 reference,在以下操作後即失效:
//   - 任何使 capacity 改變的操作 (insert、append、reserve 觸發成長等)
//   - clear()、operator=()、assign()
//   - swap() (兩 string 會交換 buffer)
//   - destructor
// 永遠不要把 reference 暫存後跨越這些操作再使用。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// 1) Reference vs Proxy Reference
//    std::string::operator[] 回傳的是「真正的 char&」。
//    但要注意 std::vector<bool>::operator[] 回傳的是 proxy reference
//    (因為 bit packing,沒有真正的 bool& 可以拿)。string 的設計簡單清晰,
//    不會有這種「reference 不是 reference」的陷阱。
//
// 2) 為什麼 STL 不在 Debug 模式自動加邊界檢查?
//    其實大多數實作都做了:libstdc++ 的 _GLIBCXX_DEBUG、libc++ 的
//    _LIBCPP_DEBUG、MSVC 的 _ITERATOR_DEBUG_LEVEL 都會在 Debug 模式
//    為 operator[] 加 assert。這是標準允許的「品質實作」。
//    但在 Release 模式下,這些檢查全部被關掉,以維持零成本承諾。
//
// 3) 與 raw array 的對應
//    char arr[5] = "abcd";
//    arr[2]              // 與 std::string s = "abcd"; s[2]; 等價
//    string 的設計目的就是「在加上動態長度與安全管理之餘,保留 raw array
//    的存取直觀性」。這是設計上的優雅之處。
//
// 4) 如何避免 [] 越界
//    - 永遠先檢查 !s.empty() 才存取 s[0]、s.front()
//    - 迴圈條件用 i < s.size() (而非 i <= s.size())
//    - 對於外部輸入,用 at() 把錯誤轉成 catchable exception
//    - 若編譯時想自動偵測,加 -D_GLIBCXX_DEBUG 編譯
//
// 5) const 與非 const 的 overload 為何要兩個版本?
//    讓「const string」呼叫 [] 時拿到 const_reference (不能寫),
//    「非 const string」呼叫時拿到 reference (可寫)。
//    這是 const-correctness 的基本實作模式,STL 大量採用。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. 在熱迴圈裡,operator[] 比 at() 快 (沒有比較與 throw 的成本)。
// 2. 修改 operator[] 回傳的字元會直接寫入 string 內部緩衝區。
// 3. 同樣會被 reallocation/insert/erase/clear 等操作使參考失效。
// 4. 別把 operator[] 用在無法保證 pos 合法的地方 — 越界沒有任何提示,
//    錯誤可能延遲到 crash 或資料損壞才被發現。
// 5. s[s.size()] 在 C++11 起為合法讀取 (回傳 '\0'),但寫入非 '\0' 是 UB。
//
// =============================================================================

/*
補充筆記：std::string::operator_subscript
  - operator[] 存取字元不做邊界檢查；越界是 undefined behavior。
  - C++ 字串的索引單位是 char，不是 Unicode 使用者看到的字。
  - 如果索引來自外部輸入，先檢查 size 或改用 at。
  - std::string::operator_subscript 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/
#include <iostream>
#include <string>

void demoSubscript() {
    std::string s = "Hello";
    std::cout << "s[0] = " << s[0] << "\n";
    std::cout << "s[s.size()] = '" << s[s.size()] << "' (應為 '\\0')\n";

    s[0] = 'J';                 // 寫入
    std::cout << "after write : " << s << "\n";   // "Jello"
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例】LeetCode 387. First Unique Character in a String (Easy)
//
// 題目敘述:
//   給定字串 s (僅含小寫字母),找出第一個只出現一次的字元的索引;
//   若不存在,回傳 -1。
//   範例: "leetcode" → 0  ('l' 只出現一次)
//        "loveleetcode" → 2  ('v' 是第一個只出現一次的)
//        "aabb" → -1
//
// 為何用 operator[]:
//   兩次線性掃描:第一次計算每個字母的出現次數,第二次找第一個 cnt==1。
//   迴圈條件 i < s.size() 已經保證合法,沒必要加 at() 的邊界檢查成本,
//   這正是 operator[] 的最佳使用場景。
//
// 解題思路:
//   1. 開一個長度 26 的計數陣列 cnt[]。
//   2. 第一次走訪 s,累加 cnt[s[i]-'a']。
//   3. 第二次走訪 s,回傳第一個 cnt==1 的索引。
//
// 複雜度: 時間 O(n),空間 O(1) (固定 26 格)
// -----------------------------------------------------------------------------
int firstUniqChar(const std::string& s) {
    int cnt[26] = {0};
    for (size_t i = 0; i < s.size(); ++i) ++cnt[s[i] - 'a'];
    for (size_t i = 0; i < s.size(); ++i)
        if (cnt[s[i] - 'a'] == 1) return static_cast<int>(i);
    return -1;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】Hex dump - 把字串轉成 16 進位輸出
//
// 為何用 operator[]:
//   除錯封包、檢視二進位資料、log 顯示時很常需要把 raw bytes 印成 hex。
//   逐 byte 處理,熱迴圈用 [] 直接讀取最快;迴圈條件已經保證在範圍內,
//   不需邊界檢查。這是後端、嵌入式、網路除錯的日常工具。
//
// 範例: hexDump("Hello") → "48 65 6C 6C 6F"
// -----------------------------------------------------------------------------
std::string hexDump(const std::string& s) {
    static const char* digits = "0123456789ABCDEF";
    std::string out;
    out.reserve(s.size() * 3);
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        out += digits[c >> 4];
        out += digits[c & 0xF];
        if (i + 1 < s.size()) out += ' ';
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1684. (此處變奏) — Count characters of digits
// 題目: LeetCode 1page. 1page0. Number of Digit One 變奏 ── 簡化為「計算字串中數字總數」
// 為何用 operator[]: 在迴圈中以索引存取最直觀,且不需邊界檢查。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
int countDigitChars(const std::string& s) {
    int cnt = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] >= '0' && s[i] <= '9') ++cnt;
    }
    return cnt;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】把字串內所有小寫字母換成大寫 (in-place)
// 為何用 operator[]: in-place 修改最直接;non-const operator[] 回傳 char& 可寫。
// -----------------------------------------------------------------------------
void toUpperInPlace(std::string& s) {
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] >= 'a' && s[i] <= 'z') s[i] = s[i] - 'a' + 'A';
    }
}

int main() {
    demoSubscript();
    std::cout << "\n=== LeetCode 387 ===\n";
    std::cout << firstUniqChar("leetcode") << "\n";   // 0
    std::cout << firstUniqChar("loveleetcode") << "\n";// 2
    std::cout << firstUniqChar("aabb") << "\n";       // -1

    std::cout << "\n=== 日常實務: Hex dump ===\n";
    std::cout << hexDump("Hello") << "\n";    // 48 65 6C 6C 6F

    std::cout << "\n=== LeetCode 變奏 (countDigitChars) ===\n";
    std::cout << countDigitChars("abc123def45") << "\n";   // 5
    std::cout << countDigitChars("no-digits-here") << "\n"; // 0

    std::cout << "\n=== 日常實務: toUpperInPlace ===\n";
    std::string s = "Hello World";
    toUpperInPlace(s);
    std::cout << s << "\n";    // HELLO WORLD

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:operator[](pos) 與 at(pos) 在 release build 的效能差距大嗎?
    //    A:差距通常很小 (一個分支 + 一個 throw 路徑),但在 hot loop 中
    //      可能影響 vectorization 與 branch predictor。對 N 次存取,
    //      [] 的 inline 程式碼往往可被向量化,at 因含 throw 不易。
    //
    //  Q2:s[s.size()] 真的合法嗎?C++03 vs C++11 差在哪?
    //    A:C++11 起標準保證 s[s.size()] 回傳 '\0' 的合法 reference。
    //      C++03 沒此保證 (只是大多實作這樣做)。注意:對非 const string,
    //      讀取合法,但寫入非 '\0' 為 UB,因為會破壞結尾哨兵。
    //
    //  Q3:從 operator[] 拿到的 char& 在哪些操作後會 dangling?
    //    A:任何可能觸發 reallocate 或交換 buffer 的操作都會失效:
    //      append/push_back/insert/replace/reserve(更大)/operator=/
    //      assign/swap/clear。SSO 字串 move 後 reference 也會失效。
    //      不要把 [] 結果存進變數後跨越這些操作再用。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra operator_subscript.cpp -o operator_subscript

// === 預期輸出 (節錄) ===
// === LeetCode 變奏 (countDigitChars) ===
// 5
// 0
// === 日常實務: toUpperInPlace ===
// HELLO WORLD
