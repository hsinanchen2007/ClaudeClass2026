// =============================================================================
// 檔名: constructor.cpp
// 主題: std::string 的建構子 (Constructors)
// 參考連結 References:
//   cppreference: https://en.cppreference.com/cpp/string/basic_string/basic_string
//   cplusplus.com: https://cplusplus.com/reference/string/string/string/
// =============================================================================
//
// 【函式資訊 Information】
// std::string 是 std::basic_string<char> 的別名 (typedef / alias)。
// 完整定義:
//   template<class CharT, class Traits = std::char_traits<CharT>,
//            class Allocator = std::allocator<CharT>>
//   class basic_string;
//
// 常見建構子原型 (本檔示範的版本):
//   (1)  string();                                  // 預設建構子
//   (2)  string(size_type count, char ch);          // 重複 count 個 ch
//   (3)  string(const string& other,
//               size_type pos,
//               size_type count = npos);            // 從 other 取子字串
//   (4)  string(const char* s, size_type count);    // 取 s 的前 count 個字元
//   (5)  string(const char* s);                     // 從 C-style 字串建構
//   (6)  template<class InputIt>
//        string(InputIt first, InputIt last);       // 從迭代器範圍建構
//   (7)  string(const string& other);               // 複製建構 (Copy ctor)
//   (8)  string(string&& other) noexcept;           // 移動建構 (Move ctor) C++11
//   (9)  string(std::initializer_list<char> ilist); // 初始化列表 C++11
//   (10) string(std::string_view sv);               // C++17 (透過 string_view 通用接收)
//
// =============================================================================
//
// 【詳細解釋 Explanation】
//
// (一) std::string 的本質
// ----------------------------------------------------------------------------
// std::string 並不是一個原語,而是「動態管理一段以 '\0' 結尾的 char 陣列」的
// RAII 容器。它的內部至少維護:
//   1. 一個指向資料緩衝區的指標 (data pointer)
//   2. 目前實際使用的字元數 (size)
//   3. 緩衝區可容納的最大字元數 (capacity)
//   4. allocator (預設為 std::allocator<char>)
//
// 為什麼需要這麼多種建構子?因為使用者建立字串的「來源」千變萬化:
// 從常數字面值、另一個 string、迭代器範圍、單一字元重複、初始化列表,甚至
// 直接從 raw buffer + 長度 (允許含 '\0')。把每一種都做成獨立的 overload,
// 可以避免使用者寫額外的轉換代碼,同時讓編譯器在多型解析時挑到「最有效率」
// 的版本。例如 string(size_t, char) 可一次配置記憶體再 memset,絕對比先
// 預設建構再 append 快。
//
// (二) 各建構子的時間複雜度
// ----------------------------------------------------------------------------
//   (1) O(1)               — 預設建構子,通常為空字串 (SSO 下不配置 heap)
//   (2) O(count)           — 填入 count 個字元
//   (3) O(count)           — 複製 count 個字元
//   (4) O(count)           — 同上
//   (5) O(strlen(s))       — 需先用 strlen 找結尾 '\0'
//   (6) O(distance)        — 視迭代器距離而定
//   (7) O(other.size())    — 需深拷貝
//   (8) O(1)               — 只搬移指標 (但 SSO 情況下可能 O(N))
//   (9) O(ilist.size())
//
// (三) SSO (Small String Optimization) — 必懂的底層技巧
// ----------------------------------------------------------------------------
// 大多數現代 STL 實作 (libstdc++、libc++、MSVC STL) 都採用 SSO:
//   - 短字串 (libstdc++ 約 ≤15 字元、libc++ 約 ≤22 字元) 直接內嵌在
//     std::string 物件本身的 stack buffer 中,完全不呼叫 new[]。
//   - 超過 SSO 上限才在 heap 配置記憶體。
//
// 影響:
//   - 短字串的建構與複製極快 (沒有 heap 分配)。
//   - 移動建構在「短字串」情況下並不會比複製快多少,因為沒有指標可搶。
//   - sizeof(std::string) 通常是 24 ~ 32 bytes (因內含 SSO buffer)。
//
// (四) C++ 標準演進
// ----------------------------------------------------------------------------
//   C++03 之前:某些實作允許 COW (Copy-On-Write) 共享底層 buffer。
//   C++11    :標準明文禁止 COW (因為多執行緒下會有 race),保證複製是深拷貝。
//   C++11    :新增移動建構 (8)、initializer_list 建構 (9)。
//   C++17    :新增從 std::string_view 隱式 (透過 ctor) 接收的版本。
//   C++23    :部分 overload 對 nullptr 明確 = delete,避免 UB。
//
// (五) 為何沒有「from int」、「from double」的建構子?
// ----------------------------------------------------------------------------
// 這是一個常見的初學者誤解:std::string s = 42; 是錯的。
// 設計者刻意不提供「數字 → 字串」的隱式建構,因為:
//   1. 字元 char 與整數 int 在 C++ 已經會隱式轉換,如果再加 string(int),
//      string s = 65; 該變成 "65" 還是 "A"? 會引爆語意混亂。
//   2. 數字轉字串通常需要指定進制、精度、locale,應透過 std::to_string、
//      std::format、std::ostringstream 顯式完成。
//
// (六) 移動建構 (Move Constructor) 的細節
// ----------------------------------------------------------------------------
// string(string&& other) noexcept 是 C++11 的關鍵成員,因為:
//   - 它保證 noexcept,使 std::vector<std::string> 在 reallocation 時可以
//     安全地用 move 取代 copy,大幅提昇容器操作效能。
//   - 被 move 的物件處於 "valid but unspecified" 狀態 — 你只能對它賦值或
//     解構,絕對不可假設它的內容是空字串(雖然多數實作確實會留下 "")。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// 1) 為什麼 std::string 是模板,而不是一個直接的 class?
//    因為「字串」其實是 char、wchar_t、char8_t、char16_t、char32_t 共通的
//    抽象。透過 template<class CharT> 的 basic_string,STL 可以一次涵蓋所有
//    寬度的字元型別:
//        std::string     = basic_string<char>
//        std::wstring    = basic_string<wchar_t>
//        std::u8string   = basic_string<char8_t>     // C++20
//        std::u16string  = basic_string<char16_t>
//        std::u32string  = basic_string<char32_t>
//    這正是 STL「以泛型抽象現實多樣性」的範例。
//
// 2) RAII (Resource Acquisition Is Initialization) 與建構子
//    建構子在 C++ 中不只是「初始化欄位」,它還承擔「申請資源 (heap memory)」
//    的責任。一旦建構成功,resource 即由 std::string 物件接管,離開 scope
//    時 destructor 自動釋放。這是 C++ 與 C 在記憶體管理上最大的哲學差異。
//
// 3) Most Vexing Parse 陷阱
//    std::string s();   // ← 錯!被解析為「函式宣告」
//    std::string s;     // ← 對,呼叫預設建構子
//    std::string s{};   // ← 對,且更明確 (uniform initialization, C++11)
//
// 4) explicit vs implicit constructors
//    std::string 的多數建構子並非 explicit,因此可寫成:
//        std::string s = "Hi";    // 隱式呼叫 string(const char*)
//    若改成 explicit,則只能寫:
//        std::string s("Hi");
//    這是設計上的「便利性 vs 安全性」取捨,STL 選擇了便利性。
//
// 5) std::string_view 與建構子的關係
//    C++17 加入 std::string_view 後,很多 API 改成接受 string_view 而非
//    string,以避免不必要的拷貝。但要建構真正的 string,仍需呼叫 string
//    的 ctor。理解這個分工有助於設計高效的 API。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. 建構子 (5) 不能傳入 nullptr,否則為 Undefined Behavior (C++23 起 deleted)。
//    錯誤示範: std::string s(nullptr);   // 不可
// 2. 建構子 (3) 若 pos > other.size(),會丟出 std::out_of_range 例外。
// 3. 建構子 (4) 允許字串內含 '\0',因為長度由 count 指定,不會提早截斷。
//    例如: std::string s("ab\0cd", 5);  // 內容是 "ab\0cd",size() == 5
// 4. 建構子 (5) 遇到 '\0' 即停止,故不能存放含內嵌 null 的二進位資料。
// 5. 移動建構子 (8) 後,被移動的物件處於 "valid but unspecified" 狀態,
//    只能對它做賦值或解構,不應假設其內容。
// 6. C++11 之前的某些實作採 COW (Copy-On-Write),C++11 之後標準禁止
//    (因為 thread-safety 問題),所以複製一定是深拷貝。
// 7. 例外安全性 (Exception Safety):
//    - 所有建構子若記憶體配置失敗會丟出 std::bad_alloc。
//    - 移動建構子保證 noexcept (C++11 起)。
//
// =============================================================================

/*
補充筆記：std::string::constructor
  - std::string::constructor 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::constructor 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
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
#include <vector>
#include <stdexcept>

// -----------------------------------------------------------------------------
// 示範各種建構子的用法 (簡單直接,讀者一看就懂)
// -----------------------------------------------------------------------------
void demoConstructors() {
    std::cout << "=== std::string 建構子示範 ===\n";

    // (1) 預設建構子: 建立空字串
    std::string s1;
    std::cout << "(1) default        : \"" << s1 << "\", size=" << s1.size() << "\n";

    // (2) 填充建構子: 5 個 'A' → "AAAAA"
    std::string s2(5, 'A');
    std::cout << "(2) fill           : \"" << s2 << "\"\n";

    // (3) 子字串建構子: 從 pos=7 取到結尾
    std::string source = "Hello, World!";
    std::string s3(source, 7);              // "World!"
    std::string s3b(source, 7, 5);          // "World"
    std::cout << "(3) substring      : \"" << s3 << "\" / \"" << s3b << "\"\n";

    // (4) 從 buffer + 長度建構: 可包含內嵌 '\0'
    const char buf[] = {'H', 'i', '\0', 'X'};
    std::string s4(buf, 4);                 // size() == 4,雖然中間有 '\0'
    std::cout << "(4) buffer+count   : size=" << s4.size() << "\n";

    // (5) 從 C-style 字串建構
    std::string s5("Hello");
    std::cout << "(5) C-string       : \"" << s5 << "\"\n";

    // (6) 從迭代器範圍建構
    std::vector<char> vec = {'C', '+', '+', ' ', 'S', 'T', 'L'};
    std::string s6(vec.begin(), vec.end());
    std::cout << "(6) iterator range : \"" << s6 << "\"\n";

    // (7) 複製建構子
    std::string s7(s5);
    std::cout << "(7) copy           : \"" << s7 << "\"\n";

    // (8) 移動建構子: s7 之後不應再使用其內容
    std::string s8(std::move(s7));
    std::cout << "(8) move           : \"" << s8 << "\"\n";

    // (9) 初始化列表 (C++11)
    std::string s9{'L', 'i', 's', 't'};
    std::cout << "(9) initializer    : \"" << s9 << "\"\n";

    // 例外: pos 超出範圍會丟 out_of_range
    try {
        std::string bad(source, 100);       // pos=100 > size()
    } catch (const std::out_of_range& e) {
        std::cout << "out_of_range caught: " << e.what() << "\n";
    }
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例】LeetCode 344. Reverse String (Easy)
//
// 題目敘述:
//   給定一個字元陣列 s,原地 (in-place) 反轉它。
//   範例: 輸入 ['h','e','l','l','o'] → 輸出 ['o','l','l','e','h']
//
// 為何用建構子:
//   這題雖然「題目本身」操作 vector<char>,但實務上常會把它先轉成
//   std::string 再用 STL 的 reverse iterator 處理。我們用「迭代器範圍
//   建構子 (6)」直接從 vector<char> 建構 std::string,展示 string 與
//   其他容器的互通性。這是「container interoperability」的最佳示範。
//
// 解題思路:
//   雙指標 i 從頭、j 從尾,逐對交換,直到 i >= j。
//
// 複雜度:
//   時間 O(n),空間 O(n) (因為複製到 string;若直接在 vector 上做則 O(1))
// -----------------------------------------------------------------------------
#include <algorithm>
void leetcode344_reverseString(std::vector<char>& s) {
    // 用「迭代器範圍建構子」把 vector<char> 變成 std::string
    std::string str(s.begin(), s.end());

    // 用兩端對換法反轉 (示範 string 的 [] 操作)
    for (size_t i = 0, j = str.size() - 1; i < j; ++i, --j) {
        std::swap(str[i], str[j]);
    }

    // 寫回 vector
    for (size_t i = 0; i < s.size(); ++i) s[i] = str[i];
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例】LeetCode 1108. Defanging an IP Address (Easy)
//
// 題目敘述:
//   把 IP 字串中的 '.' 全部換成 "[.]"。
//   輸入 "1.1.1.1" → 輸出 "1[.]1[.]1[.]1"
//
// 為何用建構子:
//   我們用「預設建構子 (1)」建立空字串,接著 reserve 足夠空間避免多次
//   reallocation。這是「先建空字串、後逐步填內容」的典型模式。
//
// 複雜度: 時間 O(n),空間 O(n)
// -----------------------------------------------------------------------------
std::string leetcode1108_defangIPaddr(const std::string& address) {
    std::string result;
    result.reserve(address.size() + 8);     // 每個 '.' 多 2 字元,IPv4 至多 4 個

    for (char c : address) {
        if (c == '.') result += "[.]";
        else          result += c;
    }
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】SQL prepared statement 的 placeholder 字串
//
// 為何用建構子:
//   後端開發在組 dynamic IN clause 時,常需要 "?, ?, ?, ?" 這種多 placeholder
//   字串。用「填充建構子 (2)」一次配出固定長度的 buffer 再填內容,比每次
//   += 的效率好很多 (避免反覆 reallocation)。
//
// 範例: makeSqlPlaceholders(3) → "?, ?, ?"
// 用途: SELECT * FROM users WHERE id IN (?, ?, ?, ?)
// -----------------------------------------------------------------------------
std::string makeSqlPlaceholders(int n) {
    if (n <= 0) return "";
    // 用「填充建構子」一次配出 n*3 個空白,然後逐字填入
    std::string s(n * 3, ' ');          // 預先配好空間
    for (int i = 0; i < n; ++i) {
        s[i * 3]     = '?';
        s[i * 3 + 1] = ',';
        s[i * 3 + 2] = ' ';
    }
    s.resize(s.size() - 2);             // 移除尾端 ", "
    return s;
}

// -----------------------------------------------------------------------------
// 程式進入點: 跑所有示範
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// 【LeetCode 範例 3】LeetCode 1431. Kids With the Greatest Number of Candies
// 題目: 給 candies[] 與 extra,每個小孩若加上 extra 後 >= max(candies) 則 true。
// 為何用 constructor: 我們示範 string(n, ch) 建構子建立結果 "TTFFT" 形式的旗標串。
//                     雖然原題用 vector<bool>,但 string-of-flags 是後端常見變奏。
// 複雜度: O(N)。
// 難度: easy
// -----------------------------------------------------------------------------
std::string kidsWithCandiesAsFlags(const std::vector<int>& candies, int extra) {
    int mx = 0;
    for (int c : candies) if (c > mx) mx = c;
    std::string out(candies.size(), 'F');               // 用建構子先全填 'F'
    for (size_t i = 0; i < candies.size(); ++i) {
        if (candies[i] + extra >= mx) out[i] = 'T';
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】從 char* + 長度建立 std::string (網路二進位資料)
// 為何用 constructor: string(p, n) 不依賴 NUL,適合 binary buffer。
//                     若改用 string(p) 會被內嵌 '\0' 截斷。
// -----------------------------------------------------------------------------
std::string fromRawBytes(const char* data, size_t n) {
    return std::string(data, n);              // 明確長度,二進位友善
}

int main() {
    demoConstructors();

    std::cout << "\n=== LeetCode 344 ===\n";
    std::vector<char> v = {'h', 'e', 'l', 'l', 'o'};
    leetcode344_reverseString(v);
    for (char c : v) std::cout << c;
    std::cout << "\n";

    std::cout << "\n=== LeetCode 1108 ===\n";
    std::cout << leetcode1108_defangIPaddr("1.1.1.1") << "\n";

    std::cout << "\n=== LeetCode 1431 ===\n";
    std::cout << kidsWithCandiesAsFlags({2,3,5,1,3}, 3) << "\n";   // TTTFT
    std::cout << kidsWithCandiesAsFlags({4,2,1,1,2}, 1) << "\n";   // TFFFF

    std::cout << "\n=== 日常實務: SQL placeholders ===\n";
    std::cout << "SELECT * FROM users WHERE id IN (" << makeSqlPlaceholders(5) << ")\n";

    std::cout << "\n=== 日常實務: 二進位 buffer 建立 string ===\n";
    const char raw[] = {'A','B','\0','C','D'};
    auto s = fromRawBytes(raw, sizeof(raw));
    std::cout << "size=" << s.size() << " (含內嵌 \\0)\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：哪些 constructor 不會配置 heap 記憶體?SSO 邊界是多少?
    //    A：SSO 範圍內的字串都不會配置 heap。常見邊界:libstdc++ 是 15 byte
    //       (sizeof(string)==32 byte 內嵌一個 16 byte buffer);libc++ 是 22 byte
    //       (sizeof(string)==24 byte);MSVC 是 15 byte。預設建構子、空字串
    //       字面值建構、短的字元重複建構 (如 string(5,'A')) 都走 SSO 路徑。
    //
    //  Q2：move ctor 一定 O(1) 嗎?SSO 下會發生什麼?
    //    A：SSO 下 move ctor 是 O(N) 而非 O(1)。因為短字串資料就在物件內嵌
    //       buffer 裡,沒有 heap pointer 可以「偷」,只能 byte-wise 拷貝那段
    //       inline buffer。Heap 配置的長字串才是真正的 O(1) (只搬指標 +
    //       size + capacity 三個 word)。
    //
    //  Q3：string(const char*) 與 string(const char*, size_t) 哪個適合二進位資料?
    //    A：二進位資料一定要用後者。string(const char*) 內部呼叫 strlen,
    //       遇到第一個 '\0' 就停下;若資料含內嵌 '\0' 會被截斷,若沒結尾
    //       '\0' 會 buffer overrun。string(p, n) 直接吃 n 個 byte,適合
    //       network packet、protobuf payload、密碼欄位等。
    //
    return 0;
}

// =============================================================================
// 編譯方式:
//   g++ -std=c++20 -Wall -Wextra constructor.cpp -o constructor
// 執行:
//   ./constructor
// =============================================================================

// === 預期輸出 (節錄) ===
// === LeetCode 1431 ===
// TTTFT
// TFFFF
// === 日常實務: 二進位 buffer 建立 string ===
// size=5 (含內嵌 \0)
