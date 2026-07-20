// =============================================================================
// 檔名: string_literals.cpp
// 主題: 字串字面值後綴 operator""s / operator""sv (C++14 / C++17)
// 參考連結 References:
//   cppreference: https://en.cppreference.com/cpp/string/basic_string/operator%22%22s
//   cppreference: https://en.cppreference.com/cpp/string/basic_string_view/operator%22%22sv
//   cplusplus.com: (cplusplus.com 對 user-defined literal 描述較少,以
//                   cppreference 為主)
// =============================================================================
//
// 【函式資訊 Information】
//   namespace std {
//     inline namespace literals {
//       inline namespace string_literals {
//         std::string  operator""s(const char*    str, size_t len);    // C++14
//         std::wstring operator""s(const wchar_t* str, size_t len);
//         std::u8string operator""s(const char8_t* str, size_t len);   // C++20
//         std::u16string operator""s(const char16_t* str, size_t len);
//         std::u32string operator""s(const char32_t* str, size_t len);
//       }
//       inline namespace string_view_literals {                        // C++17
//         std::string_view operator""sv(const char* str, size_t len) noexcept;
//         // ...其他 CharT 的對應版本
//       }
//     }
//   }
//
// 使用方式:
//   using namespace std::string_literals;          // 引入 ""s
//   using namespace std::string_view_literals;     // 引入 ""sv
//
//   auto s  = "Hello"s;       // std::string,size = 5
//   auto sv = "Hello"sv;      // std::string_view,size = 5
//
// =============================================================================
//
// 【詳細解釋 Explanation】
//
// (一) 為什麼需要 ""s?
// ----------------------------------------------------------------------------
// 在 C++14 之前,字串字面量 "foo" 的型別是 const char[4],而不是
// std::string。這帶來幾個小但討厭的問題:
//
//   1) 推導麻煩:
//        auto a = "foo";              // a 是 const char*,不是 std::string
//        std::vector v = { "a", "b" };// v 是 vector<const char*>!
//
//   2) overload 選錯:
//        void f(const std::string&);
//        void f(bool);
//        f("");           // 居然走 bool overload! 因為 const char* → bool
//                         //   優先於 const char* → std::string
//
//   3) 內含 NUL 字元時截斷:
//        std::string s = "ab\0cd";    // s 只有 "ab" (在 \0 截斷)
//
// 加上 ""s 後,字面量直接變成 std::string,這些問題消失:
//        auto a = "foo"s;             // a 是 std::string,內容 "foo"
//        std::vector v = { "a"s, "b"s };  // vector<std::string>
//        f(""s);                      // 明確走 string overload
//        auto z = "ab\0cd"s;          // size = 5,正確包含內嵌 NUL
//
// (二) ""sv 的用途
// ----------------------------------------------------------------------------
// "foo"sv 的型別是 std::string_view,跟 "foo" (const char*) 比有兩大優勢:
//   - 自帶長度 (透過 operator""sv 的 len 參數),無需 strlen 掃描;
//     對 constexpr 是純常數時間。
//   - 可正確攜帶內嵌 '\0' 而不被截斷。
// 這對「constexpr 比較」、「switch on string_view」、「constexpr table
// of strings」等場景特別好用。
//
// (三) ""s 與 ""sv 的差別
// ----------------------------------------------------------------------------
//   操作                     | "foo"            | "foo"s          | "foo"sv
//   ------------------------ | ---------------- | --------------- | ----------------
//   型別                     | const char[4]    | std::string     | std::string_view
//   含長度                   | 不含 (要 strlen) | 含              | 含
//   含 NUL 截斷?            | 是               | 否              | 否
//   有 .find / .substr ...?  | 否 (要轉)        | 是              | 是
//   會配置記憶體?           | 否               | 短字串靠 SSO,
//                            |                  | 長字串會 alloc  | 否
//   constexpr-friendly?      | 是               | C++20 起部分    | 完全
//
// (四) 為什麼字串字面值要放在 inline namespace?
// ----------------------------------------------------------------------------
// 為了避免「不小心 import 整個 std」的副作用。標準把 ""s 放在
//   std::literals::string_literals
// 這個內嵌 namespace 內。你只要 `using namespace std::literals;` 或
// `using namespace std::string_literals;` 就能取得 ""s,而不會把整個
// std 拉進來。這是 C++14 對「localize using-directive」的優雅設計。
//
// (五) 常見啟用方式
// ----------------------------------------------------------------------------
//   using namespace std::literals;                  // 把所有 std literal 都拉進來
//   using namespace std::string_literals;           // 只要 ""s
//   using namespace std::string_view_literals;      // 只要 ""sv
//   using namespace std::chrono_literals;           // (其他 literal 的對照)
//
// 一般建議「在 function scope 內 using」或「在 anonymous namespace 內
// using」,避免污染 header。
//
// (六) C++ 標準演進
// ----------------------------------------------------------------------------
//   C++11    : 加入 user-defined literal 機制 (operator"")。
//   C++14    : 標準提供 ""s (字串)、""h/""min/""s (chrono)、""i (complex)。
//   C++17    : 加入 ""sv (string_view literal),為 constexpr-friendly。
//   C++20    : char8_t 系列 ""s 與 ""sv,以及更多 constexpr-friendly。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// 1) Raw String Literal R"(...)"
//    與 ""s 是「兩個不同維度」的特性,可組合:
//        auto path = R"(C:\Users\Alice\Desktop)"s;   // 內容含反斜線,型別 string
//    Raw string 用來「免 escape」,""s 用來「轉成 string」。
//
// 2) UDL (User-Defined Literal) 的限制
//    使用者自訂 UDL 必須以底線開頭 (例如 "1_km"),沒有底線的後綴
//    保留給標準。""s、""sv、""min、""h 等是標準保留的。
//
// 3) ""s 在 header 中安全嗎?
//    建議「不要」在 header file 的 global namespace 寫 using namespace,
//    會污染所有 include 此 header 的 TU。可以在 inline 函式內部寫,
//    或用 fully-qualified 寫 std::string("foo") 也行。
//
// 4) 含內嵌 NUL 的 ""s 與雜湊 / IO
//    auto z = "ab\0cd"s; z.size() == 5。寫入 std::ostream 用 << 仍會
//    完整輸出 5 個 byte (含 \0,但 terminal 通常不顯示);用
//    std::cout << z.c_str() 會在 \0 截斷。注意此差異。
//
// 5) 為什麼 ""sv 是 noexcept 而 ""s 不是?
//    ""sv 只把 pointer + length 包成 view,絕不配置;""s 可能配置
//    (對長字串),所以可能 throw bad_alloc。標準據實標 noexcept。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. 必須先 using namespace std::literals 之類才能用 ""s / ""sv。
// 2. ""sv 不擁有任何記憶體;確保字面字串永遠存在於程式的整個生命期
//    (字面值有靜態儲存期,所以一般 OK)。
// 3. 不要把 ""s 用在「會被頻繁建構的 hot path」—— 每次都會配置 string,
//    比 ""sv 慢。
// 4. ""sv 的 operator==、< 全部用 string_view 提供的詞典序比較,
//    可在 constexpr 比較。
// 5. C++ 沒有「char-literal-to-int-literal」的 ""i 給字元 (你想到的可能是
//    虛數複數 ""i,屬 std::complex literals)。
//
// =============================================================================

/*
補充筆記：std::string::string_literals
  - std::string::string_literals 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::string_literals 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】字串字面值後綴 operator""s / operator""sv
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. "abc"、"abc"s、"abc"sv 三者差在哪?
//     答："abc" 是 const char[4],static storage duration,程式全程有效;
//         "abc"s (C++14, std::string_literals) 產生一個 std::string 臨時
//         物件,會複製字元、超過 SSO 門檻時還會 heap 配置;
//         "abc"sv (C++17, std::string_view_literals) 產生 std::string_view,
//         零配置,指向的就是那塊 static 資料。
//     追問：為什麼 "abc"sv 安全,指向臨時 string 的 view 卻不安全?
//         → 字面量是 static storage duration,活到程式結束;臨時 string
//           在完整表達式結束就沒了。
//
// 🔥 Q2. 有了 std::string("abc"),為什麼還需要 "abc"s?
//     答：operator""s 收的是 (const char*, size_t),長度由編譯器給,所以
//         能正確保留內含 '\0' 的字面量:"abc\0def"s 的 size() 是 7,而
//         std::string("abc\0def") 走 const char* 建構子,遇到第一個 '\0'
//         就停,只拿到 "abc"。另外在 auto / 模板推導處能直接得到 std::string
//         而不是 const char*。
//
// ⚠️ 陷阱. 為什麼 auto x = "a" + "b"; 編不過?
//     答：兩個字面量各自 decay 成 const char*,"+" 於是變成**指標算術**,
//         而兩個指標相加沒有定義 → 編譯錯誤。std::string 的 operator+
//         只有在**任一側**是 std::string 時才會被選中。
//     為什麼會錯：腦中把 "a" 當成「字串物件」,以為 + 是字串串接。實際上
//         它是 const char[2],C++ 語言層級沒有為原生字串提供串接運算子。
//     解法："a"s + "b" (C++14)、std::string("a") + "b",或先宣告一個
//         std::string 變數再相加。
//
// ⚠️ 陷阱. auto p = u8"hi"; 在 C++17 和 C++20 下,p 的型別一樣嗎?
//     答：不一樣,這是 C++20 的 breaking change。
//         C++17:u8"hi" 的型別是 const char[3],p 推導為 const char*;
//         C++20:新增了獨立型別 char8_t,u8"hi" 變成 const char8_t[3],
//               p 推導為 const char8_t*。
//         本機以 static_assert + -pedantic-errors 實測,兩邊互斥,確認無誤。
//     為什麼會錯：以為 u8 前綴只是「標記這是 UTF-8」、不影響型別。char8_t 是全新的
//         獨立型別,不是 unsigned char 的 typedef,所以 std::string s = u8"hi"; 在
//         C++17 編得過、C++20 直接編譯失敗。字元型別家族為 char / wchar_t /
//         char16_t / char8_t(C++20) / char32_t,對應 string / wstring / u16string /
//         u8string / u32string;注意 wchar_t 的寬度是 implementation-defined
//         (Linux 4 bytes、Windows 2 bytes),所以 wstring 不可攜。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

void demoBasicUsage() {
    using namespace std::string_literals;
    using namespace std::string_view_literals;

    std::cout << "=== 型別差異 ===\n";
    auto a  = "Hello";          // const char*
    auto b  = "Hello"s;         // std::string
    auto c  = "Hello"sv;        // std::string_view

    std::cout << "decltype(\"Hello\")    : const char[6]\n";
    std::cout << "decltype(\"Hello\"s)   : std::string,  size=" << b.size() << "\n";
    std::cout << "decltype(\"Hello\"sv)  : std::string_view, size=" << c.size() << "\n";
    (void)a;

    std::cout << "\n=== 含內嵌 NUL 不被截斷 ===\n";
    auto z = "ab\0cd"s;
    auto w = "ab\0cd"sv;
    std::cout << "\"ab\\0cd\"s  size = " << z.size() << "\n";   // 5
    std::cout << "\"ab\\0cd\"sv size = " << w.size() << "\n";   // 5
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例】LeetCode 1929. Concatenation of Array (Easy) -- 字串變體
//
// 題目原為整數陣列複製接續;此處改編為「字串列表複製接續」以呼應 ""s 用途。
//   給字串陣列 ans = ["a", "b", "c"],回傳 [ans, ans] 串接,即
//   ["a","b","c","a","b","c"]
//
// 為何用 ""s:
//   若直接寫 std::vector<std::string> v = { "a", "b", "c" },推導為
//   vector<const char*>。為了確保拿到 vector<std::string>,寫成
//   std::vector v = { "a"s, "b"s, "c"s } 是最乾淨的。""s 是讓
//   std::initializer_list 推導正確的關鍵。
//
// 解題思路:
//   reserve 兩倍長度後 append。
//
// 複雜度: 時間 O(n),空間 O(n)
// -----------------------------------------------------------------------------
std::vector<std::string> getConcatenation(std::vector<std::string> nums) {
    std::vector<std::string> out;
    out.reserve(nums.size() * 2);
    out.insert(out.end(), nums.begin(), nums.end());
    out.insert(out.end(), nums.begin(), nums.end());
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】constexpr 路由表 (用 ""sv 達到零配置)
//
// 為何用 ""sv:
//   後端 / 嵌入式常見「靜態路由表」需求:把 path / action 對應寫成
//   constexpr static array,讓 dispatch 時連 hash map 都不需要,直接
//   走線性 sv == sv 比較,完全不配置記憶體。
//
//   用 const char* 存的話需要 strcmp 與 strlen,且字面長度資訊丟失;
//   用 ""sv 不只省 strlen,還能 constexpr-time 比較 (C++20 起)。
//
//   這是嵌入式 / OS kernel / 高頻交易系統的「無 alloc dispatch」黃金 idiom。
// -----------------------------------------------------------------------------
constexpr const char* dispatch(std::string_view path) {
    using namespace std::string_view_literals;
    constexpr std::pair<std::string_view, const char*> table[] = {
        { "/health"sv,        "ok" },
        { "/api/v1/users"sv,  "list_users" },
        { "/api/v1/orders"sv, "list_orders" },
    };
    for (auto& [p, action] : table) {
        if (p == path) return action;
    }
    return "404";
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1page. 1page0. Defanging an IP Address
// 題目: LeetCode 1108. Defanging an IP Address
// 為何用 ""s: 用 "[.]"s 直接得到 std::string,不必依賴隱含轉換。
//             教學「字面值 + s suffix 立刻得到 string」的用法。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
std::string defangIpWithLiterals(const std::string& s) {
    using namespace std::string_literals;
    std::string out;
    out.reserve(s.size() + 6);
    for (char c : s) {
        if (c == '.') out += "[.]"s;     // string literal
        else          out += c;
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】保留內嵌 NUL 的二進位 mask string
// 為何用 ""s: "A\0B"s 長度 3 (含 NUL);用 const char* + strlen 會得 1。
// 場景: 比對 magic header (PNG/ZIP)、protocol byte sequence。
// -----------------------------------------------------------------------------
bool hasZipMagic(const std::string& payload) {
    using namespace std::string_literals;
    // ZIP local file header magic: PK\x03\x04 (兩個 NUL 字元)
    static const std::string magic = "PK\x03\x04"s;
    return payload.size() >= magic.size() &&
           payload.compare(0, magic.size(), magic) == 0;
}

int main() {
    demoBasicUsage();

    std::cout << "\n=== LeetCode 1929 (字串變體) ===\n";
    using namespace std::string_literals;
    auto vec = getConcatenation({ "a"s, "b"s, "c"s });
    std::cout << "[ ";
    for (auto& s : vec) std::cout << s << " ";
    std::cout << "]\n";

    std::cout << "\n=== LeetCode 1108 (字面值版) ===\n";
    std::cout << defangIpWithLiterals("1.1.1.1") << "\n";

    std::cout << "\n=== 日常實務: constexpr 路由表 ===\n";
    std::cout << "/health        → " << dispatch("/health") << "\n";
    std::cout << "/api/v1/users  → " << dispatch("/api/v1/users") << "\n";
    std::cout << "/foo           → " << dispatch("/foo") << "\n";

    std::cout << "\n=== 日常實務: hasZipMagic ===\n";
    std::cout << std::boolalpha
              << hasZipMagic("PK\x03\x04rest_of_zip"s) << "\n"   // true
              << hasZipMagic("not a zip"s)             << "\n";  // false

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:""s 與 ""sv 各是哪個 C++ 版本?為何分兩階段?
    //    A:""s 是 C++14 (P0007),""sv 是 C++17 (P0403)。""sv 必須等
    //      string_view 在 C++17 進標準後才能定義。""s 解決「字面值不是
    //      string」的型別推導問題,""sv 進一步提供零配置、constexpr 友善
    //      的不擁有版本。
    //
    //  Q2:為什麼 ""sv 是 noexcept,""s 不是?
    //    A:""sv 只把字面值的指標+長度封裝為 view,絕不配置記憶體,
    //      自然 noexcept。""s 對長度超出 SSO 的字面值會配置 heap buffer,
    //      可能丟 bad_alloc,因此不能標 noexcept。短字面值多數時候靠 SSO
    //      不會 alloc,但標準不能因此放寬合約。
    //
    //  Q3:為什麼 "ab\0cd"s 的 size() 是 5,但 std::string s = "ab\0cd"; 是 2?
    //    A:""s 走的是 operator""s(const char*, size_t),len 由編譯期字面
    //      長度提供 (5),完整保留內嵌 NUL。const char* 建構子靠 strlen
    //      掃描,遇到第一個 '\0' 就停 (得 2)。要安全保留 NUL 用 ""s 或
    //      string(ptr, n) 兩參數版。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra string_literals.cpp -o string_literals

// === 預期輸出 (節錄) ===
// === LeetCode 1108 (字面值版) ===
// 1[.]1[.]1[.]1
// === 日常實務: hasZipMagic ===
// true
// false
