// =============================================================================
//  11_string_view.cpp  —  std::string_view (C++17)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/string/basic_string_view
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、是什麼？                                               │
//  └────────────────────────────────────────────────────────────┘
//
//  「不擁有」字串資料的「唯讀 view」 — 內部就是 (const char*, size_t)。
//
//      std::string_view sv = "hello";
//
//  特性：
//   * 沒拷貝 — 引用任何「連續字元序列」(const char*, std::string, char[])
//   * 提供 string-like API（size(), substr(), find(), starts_with...）
//   * 不能保證 null-terminated（不像 c_str()）
//   * 沒擁有權 — 原資料消失就 dangling
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、為什麼好？                                             │
//  └────────────────────────────────────────────────────────────┘
//
//  C++17 之前，函式參數想吃「字串」會踩坑：
//
//      void f(const std::string& s);
//
//  傳 const char* 進來會「自動建一個 std::string 臨時物件」 — 額外配記憶
//  體 + 拷貝。對 hot path 是浪費。
//
//  string_view 取代之：
//
//      void f(std::string_view sv);
//
//  傳 const char*、std::string、std::string_view、char[] 全部「零拷貝」。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、注意「dangling view」                                 │
//  └────────────────────────────────────────────────────────────┘
//
//      std::string_view bad() {
//          std::string s = "hi";
//          return std::string_view{s};   // ❌ s 立刻死亡 → dangling
//      }
//
//  string_view 不擁有資料 — 一定要保證原資料活得比 view 久。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：吃多種來源
//   * Demo 2：substr 不拷貝
//   * LeetCode 125. Valid Palindrome — 用 string_view 做雙指針
// =============================================================================

/*
補充筆記：string view
  - string_view 不擁有字串，只保存指標與長度；來源資料死亡後 view 立刻變成 dangling。
  - 它不保證 null-terminated，因此需要 C API 時不能直接當成 c_str。
  - 適合函式參數接受字串字面量、std::string、子字串切片；不適合長期保存外部字串內容。
  - string view 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
*/
#include <cctype>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

static void show(std::string_view sv) {
    std::cout << "  size=" << sv.size() << " content=[" << sv << "]\n";
}

// LC125 用 string_view + 雙指針判斷 palindrome（只看 alphanumeric）
static bool isPalindrome(std::string_view s) {
    int i = 0, j = static_cast<int>(s.size()) - 1;
    auto isAlnum = [](char c) { return std::isalnum(static_cast<unsigned char>(c)); };
    auto toLower = [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); };

    while (i < j) {
        while (i < j && !isAlnum(s[i])) ++i;
        while (i < j && !isAlnum(s[j])) --j;
        if (toLower(s[i]) != toLower(s[j])) return false;
        ++i; --j;
    }
    return true;
}

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：吃多種來源
    // ─────────────────────────────────────────────────────────
    const char* cstr = "C string";
    std::string str = "std::string";
    char buf[] = "char array";

    show(cstr);
    show(str);
    show(buf);
    show("string literal");

    // ─────────────────────────────────────────────────────────
    // Demo 2：substr 不拷貝
    // ─────────────────────────────────────────────────────────
    std::string_view full = "the quick brown fox";
    auto sub = full.substr(4, 5);   // "quick"
    std::cout << "[Demo2] sub = [" << sub << "]  (no allocation)\n";

    // ─────────────────────────────────────────────────────────
    // LeetCode 125. Valid Palindrome
    // ─────────────────────────────────────────────────────────
    std::cout << std::boolalpha;
    std::cout << "[LC125] \"A man, a plan, a canal: Panama\" => "
              << isPalindrome("A man, a plan, a canal: Panama") << '\n';   // true
    std::cout << "[LC125] \"race a car\" => "
              << isPalindrome("race a car") << '\n';                        // false

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：什麼時候用 const std::string& vs std::string_view？
    //    A：(a) 函式只「讀字串」 → string_view（接受所有來源、零拷貝）
    //       (b) 需要 c_str() 給 C API → string&（保證 null-terminated）
    //       (c) 需要持有資料 → 拷貝成 std::string
    //
    //  Q2：string_view 與 c_str() 互動？
    //    A：string_view 沒保證 null-terminated。要傳給 C API 必須先轉 string：
    //         std::string{sv}.c_str()
    //       或在你的環境保證 view 來自 null-terminated 字串。
    //
    //  Q3：return string_view 安全嗎？
    //    A：要小心「指向的資料活多久」。從「全域字串字面量」回 view OK；
    //       從「local std::string」回 view 是 UB。Class 成員回成員 string
    //       的 view 也要小心（this 物件死亡就 dangling）。
    //
    // ─────────────────────────────────────────────────────────
    // LeetCode 14. Longest Common Prefix
    //   題意：給字串陣列，回傳「最長共同前綴」。
    //   為什麼放這？用 string_view 走 prefix，比較不必任何拷貝。
    //   難度: easy
    // ─────────────────────────────────────────────────────────
    auto longestCommonPrefix = [](const std::vector<std::string>& strs) -> std::string {
        if (strs.empty()) return "";
        std::string_view first{strs[0]};
        std::size_t end = first.size();
        for (std::size_t i = 1; i < strs.size(); ++i) {
            std::string_view s{strs[i]};
            std::size_t k = 0;
            while (k < end && k < s.size() && first[k] == s[k]) ++k;
            end = k;
            if (end == 0) break;
        }
        return std::string{first.substr(0, end)};
    };
    std::cout << "[LC14] LCP({\"flower\",\"flow\",\"flight\"}) = \""
              << longestCommonPrefix({"flower", "flow", "flight"}) << "\"\n";
    std::cout << "[LC14] LCP({\"dog\",\"racecar\",\"car\"}) = \""
              << longestCommonPrefix({"dog", "racecar", "car"}) << "\"\n";

    // ─────────────────────────────────────────────────────────
    // 實用範例：用 string_view 寫 token 切分（無拷貝）
    //   工作上常見：parse CSV / 路徑 / URL
    // ─────────────────────────────────────────────────────────
    auto split = [](std::string_view s, char delim) {
        std::vector<std::string_view> out;
        std::size_t start = 0;
        for (std::size_t i = 0; i <= s.size(); ++i) {
            if (i == s.size() || s[i] == delim) {
                out.push_back(s.substr(start, i - start));
                start = i + 1;
            }
        }
        return out;
    };
    std::string csv = "id,name,age,city";       // 字串存於 main 內，view 安全
    auto tokens = split(csv, ',');
    std::cout << "[Demo3] columns:";
    for (auto t : tokens) std::cout << ' ' << t;
    std::cout << '\n';

    return 0;
}
