// =============================================================================
// 檔名: operator_plus.cpp
// 主題: std::string 的 operator+ (非成員串接運算子)
// 參考連結 (References):
//   - cppreference: https://en.cppreference.com/cpp/string/basic_string/operator%2B
//   - cplusplus.com: https://cplusplus.com/reference/string/string/operators/
// =============================================================================
//
// 【函式資訊 Information】
// 非成員函式,提供 string + string、string + const char*、string + char、
// const char* + string、char + string、string + string_view 等多種重載。
// C++11 起也提供 rvalue 版本以支援移動最佳化。
//
// 範例宣告 (其中一些;實際宣告數量超過十個):
//   string operator+(const string& lhs, const string& rhs);   // (1) 兩個 lvalue
//   string operator+(const char* lhs, const string& rhs);     // (2) 字面值在左
//   string operator+(const string& lhs, const char* rhs);     // (3) 字面值在右
//   string operator+(char lhs, const string& rhs);            // (4) 單字元在左
//   string operator+(const string& lhs, char rhs);            // (5) 單字元在右
//   string operator+(string&& lhs, const string& rhs);        // (6) 左 rvalue 重用 buffer
//   string operator+(const string& lhs, string&& rhs);        // (7) 右 rvalue 重用 buffer
//   string operator+(string&& lhs, string&& rhs);             // (8) 兩 rvalue
//
// 所屬類別: 非成員函式 (non-member functions)
// 回傳型別: std::string (新建字串)
// 標準:    C++98 起 (基本); C++11 起加入 rvalue overload
//
// 【詳細解釋 Explanation】
//
// (1) 函式定位 ── 「拼接」最直觀的語法:
//     operator+ 是 string 連接最直觀的寫法:
//
//         std::string s = a + b + c;
//
//     但與 std::vector::push_back (修改既有物件) 不同,operator+ 「一定建立新字串」
//     並回傳 ── 它是個非成員函式,語意上是「lhs 與 rhs 串接後產生新值」。
//
// (2) 為何 += 比 + 更高效 ── 從配置次數說起:
//     考慮:
//
//         std::string s = a + b + c + d;
//
//     【最壞情況 (沒有 rvalue overload)】
//       1) tmp1 = a + b   → 配置一次
//       2) tmp2 = tmp1 + c → 配置一次
//       3) tmp3 = tmp2 + d → 配置一次
//       4) s = std::move(tmp3) → 不配置
//       共 3 次配置 + 多次拷貝。
//
//     【有 C++11 rvalue overload 的情況】
//       1) tmp1 = a + b   → 配置 (a, b 都 lvalue)
//       2) tmp2 = std::move(tmp1) + c → 重用 tmp1 的 buffer (若容量夠)
//       3) tmp3 = std::move(tmp2) + d → 同上
//       共可能只 1 次配置。
//
//     但 a + b 仍至少配置一次 ── 因此對「迴圈中拼接」的場景:
//
//         std::string s;
//         for (...) s = s + chunk;   // 每次都配置 + 拷貝 s
//
//         std::string s;
//         for (...) s += chunk;       // 通常只在 capacity 不夠時才配置
//
//     後者的效能差異可達 10-100 倍 (對長串接尤其明顯)。
//
// (3) C++11 rvalue overload 與「buffer 重用」:
//     當左運算元是右值 (例如 (a + b) 的結果),operator+ 會嘗試:
//       a. 若 lhs 的 capacity 足以容納 lhs.size() + rhs.size() → 直接 append rhs 到 lhs
//          並 return std::move(lhs);
//       b. 否則才配置新 buffer。
//     這就是為什麼 a + b + c 的鏈式表達式,從中段開始可以「不再配置」── 利用
//     第一個 (a+b) 的暫時物件作為「累積容器」。
//
// (4) 時間複雜度: O(N + M),N、M 是兩邊長度。
//     字元拷貝是線性的;rvalue overload 可省掉一次拷貝。
//
// (5) 例外安全 ── basic guarantee:
//     若配置失敗會丟 std::bad_alloc;原運算元保持有效 (但 lhs 是 rvalue 時可能已被
//     部分修改 ── 反正它要被丟棄,無關緊要)。
//
// (6) const char* + const char* 不能用!── 經典坑:
//
//         auto s = "hello" + "world";   // 編譯錯!
//
//     "hello" 與 "world" 都是 const char*,operator+ 對 (char*, char*) 沒有重載
//     (它是指標相加,不是字串相連)。正確寫法:
//
//         using namespace std::string_literals;
//         auto s = "hello"s + "world";       // s suffix 把它變 std::string
//         // 或
//         std::string s = std::string("hello") + "world";
//
// 【注意事項 Pay Attention】
// 1. 在迴圈中拼接優先使用 += 或 append,避免不必要的暫時物件。
// 2. const char* + const char* 不能用 operator+,因為它不是 string 的重載
//    (兩邊都不是 string)。寫 std::string("a") + "b" 才行。
// 3. 兩個大字串連接但只暫用,可考慮 string_view + 單次拼接的設計。
// 4. 例外安全: 配置失敗 → bad_alloc。
// 5. operator+ 不是 noexcept ── 任何配置都可能失敗。
// 6. 對「多段拼接已知所有片段」的場景,先 reserve(總長度) 再 += 是最高效的。
//
// 【概念補充 Concept Deep Dive】
//
// (A) RVO / NRVO ── 編譯器的「免拷貝最佳化」:
//     operator+ 回傳新 string,理論上會涉及一次拷貝/搬移。但編譯器在許多情境
//     會直接「就地構造」結果,這稱為 Return Value Optimization (RVO):
//
//         std::string make() { return std::string("hello") + "world"; }
//         std::string s = make();   // 沒有任何拷貝/搬移!
//
//     C++17 起 mandatory copy elision 把某些 RVO 場景變成「保證」沒有拷貝。
//
// (B) 為何鏈式 a + b + c + d + ... 仍可能慢:
//     即使有 rvalue overload + RVO,第一個 a + b 仍需配置;
//     若 a 很長且 b、c、d、... 也都很長,第一次配置的 capacity 可能不夠後續使用,
//     會觸發多次「重新配置 + 整段拷貝」。
//     更安全的寫法:
//
//         std::string s;
//         s.reserve(a.size() + b.size() + c.size() + d.size());
//         s += a; s += b; s += c; s += d;
//
//     這是「業界規範」── 把總長度算清楚,reserve 一次,append 多次。
//
// (C) std::format / fmt::format ── 現代替代品:
//     C++20 起 std::format 提供更高效、更可讀的格式化:
//
//         std::string s = std::format("{} {}: {}", date, level, msg);
//
//     比 + 拼接更易讀;內部用一次配置完成,效能也好。
//     若需大量字串拼接 (尤其是含格式化),優先選 std::format / fmt::format
//     而非 operator+。
//
// (D) std::ostringstream ── 流式串接:
//     對大量「不同類型」的拼接 (含 int、double):
//
//         std::ostringstream oss;
//         oss << "user=" << id << ", score=" << score;
//         std::string s = oss.str();
//
//     比 to_string + + 連接更乾淨。但 ostringstream 有 locale 開銷,
//     簡單字串拼接不必用它。
//
// (E) 為何 += 不直接也叫 operator+?
//     C++ 區分「修改既有物件 (compound assignment)」與「產生新物件 (binary op)」:
//       - += 修改 *this,可重用 capacity,通常 O(amortized) 1 次配置
//       - +  產生新物件,理論上一定有新配置 (即使 RVO 也只是省「回傳的」拷貝)
//     這個語意區分是 STL 一貫的設計:vector 也有 push_back vs concat,概念類似。
//
// (F) string + string_view (C++17 起):
//     string_view 是 view,operator+ 接受它就避免「先把 view 拷成 string」。
//     對大量短拼接的場景,把 const char* 改成 string_view 沒差;
//     但對動態 view (例如 substr 出來的) 就有效率差異。
//
// =============================================================================

/*
補充筆記：std::string::operator_plus
  - std::string::operator_plus 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::operator_plus 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
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

void demoOperatorPlus() {
    std::string a = "Hello";
    std::string b = ", ";
    std::string c = "World!";
    std::string s = a + b + c;
    std::cout << s << "\n";

    // const char* + string
    std::string t = "Prefix " + a;
    std::cout << t << "\n";

    // string + char
    std::string u = a + '!';
    std::cout << u << "\n";

    // 注意:這寫法錯誤 — "abc" + "def" 是 const char* + const char* (指標相加)
    // std::string bad = "abc" + "def";
    // 正解:用 string_literal 或先轉成 string
    using namespace std::string_literals;
    std::string ok = "abc"s + "def";
    std::cout << ok << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 412. Fizz Buzz (operator+= / 拼接基礎)
// 題目敘述:
//   給整數 n,輸出長度 n 的字串陣列 answer,規則:
//     - 若 i 同時是 3 與 5 的倍數,answer[i-1] = "FizzBuzz"
//     - 若 i 是 3 的倍數,answer[i-1] = "Fizz"
//     - 若 i 是 5 的倍數,answer[i-1] = "Buzz"
//     - 否則 answer[i-1] = i 的字串表示
// 為何用 operator+ (+=):
//   "Fizz" 與 "Buzz" 可能同時加入,用 += 拼接是最直觀的解法 ──
//   省去 if/else 巢狀結構。
// 注意: 這裡用 += 而非 + 純粹是效能習慣;範例本身即可示範 operator+ 家族。
// 複雜度: O(n * L),L = 字串平均長度 (常數)。
// -----------------------------------------------------------------------------
#include <vector>
std::vector<std::string> fizzBuzz(int n) {
    std::vector<std::string> out;
    out.reserve(n);
    for (int i = 1; i <= n; ++i) {
        std::string s;
        if (i % 3 == 0) s += "Fizz";
        if (i % 5 == 0) s += "Buzz";
        if (s.empty())  s = std::to_string(i);
        out.push_back(s);
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】錯誤訊息組合 ── 帶 context 的 exception message
// 為何用 operator+: 拋例外時讓訊息含「行為 + 對象 + 原因」最容易除錯。
//                   一次組好,呼叫端只看到清楚的 e.what()。
// -----------------------------------------------------------------------------
#include <stdexcept>
[[noreturn]] void throwIOError(const std::string& action,
                                const std::string& target,
                                const std::string& reason) {
    throw std::runtime_error("Failed to " + action + " " + target + ": " + reason);
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1page. 1page0. Defanging an IP Address
// 題目: LeetCode 1108. Defanging an IP Address (用 operator+ 變奏)
// 為何用 operator+: 教學最直觀但「非最佳」的字串拼接寫法 ── 每次都建立新 string。
//                    對比 += 或 reserve+append 看出效能差異。複雜度仍是 O(N) 但常數大。
// -----------------------------------------------------------------------------
std::string defangIpPlus(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '.') out = out + "[.]";   // 注意:這寫法每次都生新 string,效能不佳
        else          out = out + c;
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】組合 URL: 把 scheme/host/path 拼成完整網址
// 為何用 operator+: 三段組合,寫法直觀,適合此類「固定步驟」的拼接。
// -----------------------------------------------------------------------------
std::string buildUrl(const std::string& scheme,
                     const std::string& host,
                     const std::string& path) {
    return scheme + "://" + host + path;
}

int main() {
    demoOperatorPlus();
    std::cout << "\n=== LeetCode 412 ===\n";
    for (auto& s : fizzBuzz(15)) std::cout << s << " ";
    std::cout << "\n";

    std::cout << "\n=== LeetCode 1108 (operator+ 版) ===\n";
    std::cout << defangIpPlus("1.1.1.1") << "\n";

    std::cout << "\n=== 日常實務: 錯誤訊息組合 ===\n";
    try {
        throwIOError("open", "/etc/secret.conf", "Permission denied");
    } catch (const std::exception& e) {
        std::cout << "caught: " << e.what() << "\n";
    }

    std::cout << "\n=== 日常實務: buildUrl ===\n";
    std::cout << buildUrl("https", "example.com", "/api/v1/users") << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:auto s = "hello" + "world"; 為什麼編譯不過?
    //    A:兩邊都是 const char* (字串字面值),operator+ 對 (char*, char*)
    //       沒有重載 — 那會被當作指標相加。正確寫法:用 ""s suffix
    //       ("hello"s + "world"),或先 std::string("hello") + "world",
    //       至少要有一邊是 std::string / string_view 才會匹配到 operator+ 重載。
    //
    //  Q2:迴圈中累積字串為什麼必須用 += 而非 +?
    //    A:operator+ 是「非成員、產生新字串」 — for(...) r = r + w 每次都新配置
    //       並複製整個 r,N 次拼接是 O(N²)。+= 直接在 *this 後面 append,只在
    //       capacity 不夠時才 reallocate,攤銷 O(N)。已知總長更應先 reserve。
    //
    //  Q3:C++11 的 rvalue overload 對 a + b + c 鏈式拼接有什麼好處?
    //    A:operator+ 提供 (string&&, ...) / (..., string&&) 等 rvalue overload。
    //       a+b 產生暫時 rvalue,後續 +c、+d 會優先選 rvalue overload,直接
    //       重用該暫時物件的 buffer (若 capacity 夠) — 整條鏈可只配置一次。
    //       若擔心 capacity 不夠重新配置,還是改 reserve + += 最穩。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra operator_plus.cpp -o operator_plus

// === 預期輸出 (節錄) ===
// === LeetCode 1108 (operator+ 版) ===
// 1[.]1[.]1[.]1
// === 日常實務: buildUrl ===
// https://example.com/api/v1/users
