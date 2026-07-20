// =============================================================================
// 檔名: operator_string_view.cpp
// 主題: std::string::operator basic_string_view (隱式轉換為 string_view, C++17)
// 參考連結 References:
//   cppreference: https://en.cppreference.com/cpp/string/basic_string/operator_basic_string_view
//   cplusplus.com: https://cplusplus.com/reference/string_view/string_view/string_view/
// =============================================================================
//
// 【函式資訊 Information】
//   constexpr operator
//     std::basic_string_view<CharT, Traits>() const noexcept;     // since C++17
//
// 一個「conversion operator」(轉型運算子) —— 把 string 物件「視為」一個
// string_view。注意它是「隱式」(implicit) 轉換,沒有 explicit 關鍵字,
// 表示在需要 string_view 的地方,std::string 會自動被轉換,不必手動寫
// std::string_view{s} 或 static_cast。
//
// =============================================================================
//
// 【詳細解釋 Explanation】
//
// (一) 為何 C++17 加入這個轉換?
// ----------------------------------------------------------------------------
// std::string_view 是 C++17 引入的「不擁有所有權的字串視圖」(non-owning
// view)。在新的 API 設計中,函式參數應該優先收 string_view 而非
// const std::string&,理由:
//   - string_view 可同時接受 std::string、const char*、char[N]、子字串
//     而不需要為每一種寫 overload。
//   - 不需要建構臨時 std::string,不會配置記憶體。
//   - 比 const string& 更適合「我只是讀,不擁有」的語意。
//
// 為了讓 string 能「無痛」傳給接受 string_view 的 API,標準在 string 上
// 提供了這個 implicit conversion operator —— 你寫 f(my_string) 時,編譯器
// 會自動呼叫此 operator,把 string 包成 string_view 傳進去,零拷貝。
//
// (二) 轉換出來的 string_view 指向哪裡?
// ----------------------------------------------------------------------------
// 它指向「原 string 的內部 buffer」(也就是 data() 回傳的那塊),
// 長度等於 size()。**不會** 複製內容。所以:
//   - 高效能:O(1),只把指標 + 長度搬出來。
//   - 危險:string_view 的生命週期不能超過原 string 的生命週期。
//     若原 string 死了或內容被改 (insert/erase/append/operator=、reserve
//     觸發 reallocation 等),原 string_view 會變成 dangling view。
//
// (三) 與「明確 substr」的差異
// ----------------------------------------------------------------------------
//   std::string s = "hello";
//   std::string_view sv1 = s;           // 隱式轉換,O(1),指向 s 的 buffer
//   std::string_view sv2 = s.substr(1); // substr 回傳新 string,O(n) 複製
// 把整個 string 當 view 用,寧可走隱式轉換 + sv.substr() (string_view 的
// substr 也是 O(1),不複製)。
//
// (四) 為什麼是 implicit 而不是 explicit?
// ----------------------------------------------------------------------------
// 因為這個轉換「便宜、安全、語意自然」(只要遵守 lifetime 即可)。
// 反過來,std::string_view → std::string 必須是 explicit (要呼叫者明確
// 寫 std::string{sv}),因為那會配置記憶體,不該被「不知不覺」發生。
//
// (五) constexpr & noexcept
// ----------------------------------------------------------------------------
// 兩個都標。在 constexpr 上下文 (C++20 起 constexpr basic_string) 中可用,
// 而且絕不丟例外。轉換內部只做 string_view{data(), size()} 而已,自然
// noexcept。
//
// (六) 為什麼有時編譯器仍要求顯式轉換?
// ----------------------------------------------------------------------------
// 在「同時有多個 implicit 轉換可選」(ambiguous overload) 或「樣板推導」
// (template type deduction) 中,implicit 轉換不會發生。例如:
//   template<class T> void f(const T&);
//   f(my_string);  // T 推導為 std::string,不會自動轉 string_view
// 這時要寫 f(std::string_view{my_string})。
//
// (七) C++ 標準演進
// ----------------------------------------------------------------------------
//   C++17 起加入 (基本型態為非 explicit、noexcept、constexpr)。
//   C++20 起在更多 constexpr 場合可用 (因為 string 本身變 constexpr)。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// 1) string_view 三大典型用法
//    - 函式參數:把 const std::string& 換成 std::string_view。
//    - 切片:sv.substr(2, 5),不複製、不配置。
//    - 解析:把整段文字 walk 過去,各個 token 都用 view 表示。
//
// 2) string_view 的危險:dangling view
//    - 不要把暫時 string 的 view 存起來:
//        std::string_view bad = (returnsString());  // 暫時死了 → dangling
//    - 不要在 modify string 後繼續用舊 view:
//        std::string s = "abc";
//        std::string_view v = s;
//        s += "xxxxxxxxxx";   // 可能 reallocate → v 失效
//
// 3) string 的 ABI 細節
//    隱式轉換出來的 string_view 對長字串、SSO (small string optimization)
//    短字串都正常。SSO 短字串的 buffer 在 string 物件本體內,只要 string
//    物件還活著就有效。
//
// 4) 編寫新 API 的建議
//    新介面 (C++17 之後) 應該預設用 std::string_view:
//      - 參數想「讀」字串內容 → string_view
//      - 參數想「擁有」字串內容 → std::string (by value, then move 進去)
//      - 參數想「修改且擁有」字串內容 → std::string&
//
// 5) std::span / std::string_view / std::ranges 系列的關係
//    string_view 是「char 範圍版的 std::span<const char>」,概念一樣 ——
//    pointer + length + non-owning。掌握這個 view 心法,寫現代 C++ 會
//    舒服許多。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. 轉出來的 view 跟原 string 共用 buffer,**不要** 在 view 還活著時
//    對 string 做會 reallocate / 改變 size 的操作。
// 2. string_view 不一定是 NUL-terminated,不能直接傳給 C API
//    (如 fopen) — 必要時要 std::string{sv}.c_str()。
// 3. implicit 轉換不會發生在 template 型別推導中。
// 4. 對於 const char* 而言,若你寫 f(s) 且 f 同時有 const char*
//    與 string_view overload,implicit 轉換可能造成 ambiguous,要小心。
// 5. 此 operator 是 C++17 才有,C++14 之前沒有 string_view,請務必加
//    -std=c++17 或更高。
//
// =============================================================================

/*
補充筆記：std::string::operator_string_view
  - std::string::operator_string_view 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::operator_string_view 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::string::operator string_view (隱式轉換)
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::string 為什麼提供「隱式」轉換到 string_view?
//     答：C++17 起 basic_string 有 operator basic_string_view() const noexcept,
//         而且**沒有** explicit。所以任何吃 string_view 的函式都能直接收
//         std::string,不必手寫 static_cast。轉換本身只複製「指標 + 長度」,
//         O(1)、不配置、不複製任何字元。
//     追問：那反方向 string_view → string 為什麼要 explicit?
//         → 那是會配置記憶體的深複製,隱式做太容易寫出看不見的成本。
//
// 🔥 Q2. 這個隱式轉換安全嗎?
//     答：轉換動作本身安全 (O(1)、noexcept),但產生的 view 生命週期綁在
//         來源 string 身上。只要來源是具名、活著的 string,而 view 只在
//         呼叫期間使用,就沒問題;一旦把 view 存起來或帶出來源的作用域,
//         就是 dangling。來源被 append/resize 而重新配置時同樣失效。
//     追問：為什麼 f(std::string_view) 收字面量比 f(const std::string&) 快?
//         → 後者要為 const char* 隱式建構一個臨時 std::string (可能 heap
//           配置),前者只是包一組指標與長度。
//
// ⚠️ 陷阱. std::string_view sv = getName();  (getName 回傳 std::string by value)
//     答：隱式轉換讓它順利編譯,但那個臨時 std::string 在完整表達式結束時
//         就解構 → sv 立即 dangling,之後每次讀取都是 UB。
//     為什麼會錯：轉換是隱式的,編譯器不會擋;而 const std::string& r =
//         getName(); 卻是**合法**的 (const reference 延長臨時物件生命週期)。
//         這個「引用會延長、view 不會」的不對稱正是最愛考的點。
//         GCC/Clang 的 -Wdangling-gsl 能抓到部分這類情形。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

// 一個「現代風格」函式:接收 string_view,可同時用 string、const char*、
// char[N]、子字串呼叫,且不複製。
size_t countVowels(std::string_view sv) {
    size_t n = 0;
    for (char c : sv) {
        switch (c) {
            case 'a': case 'e': case 'i': case 'o': case 'u':
            case 'A': case 'E': case 'I': case 'O': case 'U':
                ++n;
        }
    }
    return n;
}

void demoOperatorStringView() {
    std::cout << "=== std::string → std::string_view 隱式轉換 ===\n";

    std::string s = "Hello, World";
    std::string_view sv = s;          // 隱式轉換,O(1)、不複製
    std::cout << "sv.data() == s.data() ? " << std::boolalpha
              << (sv.data() == s.data()) << "\n";
    std::cout << "sv.size() == s.size() ? "
              << (sv.size() == s.size()) << "\n";

    // 同個函式接收任何字串型來源
    std::cout << "\n=== 同一個函式接收不同來源 ===\n";
    std::cout << "from string : " << countVowels(s)         << "\n";
    std::cout << "from cstr   : " << countVowels("Sky")     << "\n";
    std::cout << "from substr : " << countVowels(sv.substr(7))  << "\n";  // "World"

    // 危險示範:reallocate 後 view dangling (僅示範概念,實務勿用)
    std::cout << "\n=== 危險範例 (僅展示) ===\n";
    std::string t = "short";
    std::string_view v = t;
    std::cout << "before: " << v << "\n";
    t.append(1000, 'X');              // 可能 reallocate → v 失效
    // 不要再用 v! 此處只列警語,不真的存取避免印出 garbage
    std::cout << "(此後 v 可能 dangling,絕對不可再讀)\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例】LeetCode 392. Is Subsequence (Easy)
//
// 題目敘述:
//   給字串 s 與 t,判斷 s 是否為 t 的「子序列」(可從 t 中刪除任意字元
//   後得到 s,順序保留)。
//   範例: s="abc", t="ahbgdc" → true
//        s="axc", t="ahbgdc" → false
//
// 為何用 operator string_view:
//   API 寫成接受 string_view,可同時直接吃 string 與字面量,呼叫端
//   零拷貝零配置。對「字串只讀類」函式 (例如 isSubsequence、startsWith、
//   endsWith) 是最現代化的設計。
//
// 解題思路:
//   雙指標 i 走 s,j 走 t;每次 t[j]==s[i] 就 ++i;結束看 i==s.size()。
//
// 複雜度: 時間 O(|t|),空間 O(1)
// -----------------------------------------------------------------------------
bool isSubsequence(std::string_view s, std::string_view t) {
    size_t i = 0;
    for (size_t j = 0; j < t.size() && i < s.size(); ++j) {
        if (t[j] == s[i]) ++i;
    }
    return i == s.size();
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】HTTP path 前綴比對 / route dispatch
//
// 為何用 operator string_view:
//   Web 後端 router 會大量比對 request path 是否以某個前綴開頭
//   (e.g. "/api/v1/users", "/api/v1/orders") 來決定走哪個 handler。
//   把 path 當 string_view,routes 表也是 string_view,零拷貝零配置,
//   而且 path 本身不論是 std::string、char* 或 substr 視圖都能直接傳。
// -----------------------------------------------------------------------------
const char* routeFor(std::string_view path) {
    struct Route { std::string_view prefix; const char* handler; };
    static constexpr Route table[] = {
        {"/api/v1/users",  "users_handler"},
        {"/api/v1/orders", "orders_handler"},
        {"/static/",       "static_handler"},
        {"/health",        "health_handler"},
    };
    for (auto& r : table) {
        if (path.size() >= r.prefix.size() &&
            path.substr(0, r.prefix.size()) == r.prefix) {
            return r.handler;
        }
    }
    return "not_found";
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1page. 1page0. Substrings of Size Three with Distinct Characters
// 題目: LeetCode 1location7. 1location0. Substrings of Size Three with Distinct Characters
// 計算 s 中長度 3 且 3 個字元相異的子字串數量。
// 為何用 string_view: 用 string_view::substr 做 O(1) 視窗滑動,免重複配置記憶體。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
int countGoodSubstrings(const std::string& s) {
    if (s.size() < 3) return 0;
    std::string_view sv = s;          // implicit conversion via operator string_view
    int cnt = 0;
    for (size_t i = 0; i + 3 <= sv.size(); ++i) {
        auto w = sv.substr(i, 3);
        if (w[0] != w[1] && w[1] != w[2] && w[0] != w[2]) ++cnt;
    }
    return cnt;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】Log filter:接受任何來源 (string / char* / 子字串) 統一過濾
// 為何用 string_view: 隱式轉換讓 API 一個就好,呼叫端不需要構建臨時 string。
// -----------------------------------------------------------------------------
bool isErrorLine(std::string_view line) {
    return line.find("ERROR") != std::string_view::npos
        || line.find("FATAL") != std::string_view::npos;
}

int main() {
    demoOperatorStringView();

    std::cout << "\n=== LeetCode 392 ===\n";
    std::string s1 = "abc", t1 = "ahbgdc";
    std::cout << std::boolalpha
              << isSubsequence(s1, t1) << "\n"        // string → view (隱式)
              << isSubsequence("axc", "ahbgdc") << "\n";  // cstr → view

    std::cout << "\n=== LeetCode 1location0 (countGoodSubstrings) ===\n";
    std::cout << countGoodSubstrings("xyzzaz")    << "\n";   // 1
    std::cout << countGoodSubstrings("aababcabc") << "\n";   // 4

    std::cout << "\n=== 日常實務: HTTP route dispatch ===\n";
    std::string p = "/api/v1/users/42";
    std::cout << p << " → " << routeFor(p) << "\n";          // string 隱式轉換
    std::cout << "/health → " << routeFor("/health") << "\n";
    std::cout << "/foo → "    << routeFor("/foo")    << "\n";

    std::cout << "\n=== 日常實務: isErrorLine ===\n";
    std::string line1 = "10:00 INFO  ok";
    std::cout << std::boolalpha
              << isErrorLine(line1) << "\n"             // false
              << isErrorLine("11:00 ERROR oom") << "\n" // true (cstr 隱式)
              << isErrorLine(std::string_view{"FATAL!"}) << "\n"; // true

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:為什麼 string→string_view 是 implicit,反方向卻是 explicit?
    //    A:string→view 只是抓 pointer+length,O(1) 且不配置記憶體,
    //      語意安全 (只要遵守 lifetime);反方向會 allocate 並複製內容,
    //      代價高,標準刻意要求呼叫者明寫 std::string(sv) 以提醒成本。
    //
    //  Q2:既然有 implicit operator,為何 template<class T>void f(const T&)
    //      傳 std::string 不會被視為 string_view?
    //    A:template 型別推導 (deduction) 不考慮使用者定義的轉換,T 直接
    //      被推成 std::string。要走 view 必須顯式 std::string_view{s},或
    //      讓 f 直接以 std::string_view 為參數型別。
    //
    //  Q3:這個 operator 對 SSO 短字串安全嗎?
    //    A:安全。view 指向 string 物件 data() 回傳的位址,不論長字串
    //      (heap buffer) 或 SSO 短字串 (object 內 inline buffer),只要
    //      原 string 沒被 move/destroy/resize,view 都有效。但 string
    //      被 move 後,SSO 內嵌 buffer 的位址會改變,view 隨即 dangling。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra operator_string_view.cpp -o operator_string_view

// === 預期輸出 (節錄) ===
// === LeetCode 1location0 (countGoodSubstrings) ===
// 1
// 4
// === 日常實務: isErrorLine ===
// false
// true
// true
