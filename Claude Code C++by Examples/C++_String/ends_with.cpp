// =============================================================================
// 檔名: ends_with.cpp
// 主題: std::string::ends_with (C++20: 檢查字串結尾)
// 參考連結 (References):
//   - cppreference: https://en.cppreference.com/cpp/string/basic_string/ends_with
//   - cplusplus.com: https://cplusplus.com/reference/string/string/
//     (cplusplus.com 尚未收錄此 C++20 新成員;主頁仍可作為 std::string 的整體參考)
// =============================================================================
//
// 【函式資訊 Information】
//   constexpr bool ends_with(std::string_view sv) const noexcept;       // (1)
//   constexpr bool ends_with(char ch) const noexcept;                    // (2)
//   constexpr bool ends_with(const char* s) const;                       // (3)
//
// 所屬類別: std::basic_string<CharT, Traits, Allocator>
// 回傳型別: bool
// 參數:    與 starts_with 對稱 ── string_view / char / C-string
// 標準:    C++20 加入。與 starts_with 同時引入,語意完全對稱。
//
// 【詳細解釋 Explanation】
//
// (1) 函式定位 ── 與 starts_with 互為鏡像:
//     ends_with(suffix) 判斷 *this 是否以 suffix 結尾。等同於以下 reference impl:
//
//         constexpr bool ends_with(string_view sv) const noexcept {
//             return size() >= sv.size() &&
//                    compare(size() - sv.size(), sv.size(), sv) == 0;
//         }
//
//     先檢查長度避免 underflow (size_type 是 unsigned,size() - sv.size() 若 sv 較長
//     會溢位成超大的 size_t),然後用 compare 比對「字串尾段」與 suffix。
//
// (2) C++20 之前的痛苦寫法:
//
//         bool legacy_ends_with(const std::string& s, const std::string& suf) {
//             if (s.size() < suf.size()) return false;          // 必要的長度檢查
//             return s.compare(s.size() - suf.size(),            // 起點
//                              suf.size(),                       // 比對長度
//                              suf) == 0;                        // 比對對象
//         }
//
//     compare 的 4 參數版本最常被誤用 ── 參數順序「(start, count, str)」很饒口。
//     C++20 之後一行 s.ends_with(suf) 解決,可讀性大幅提升。
//
// (3) 為什麼 ends_with 比 starts_with 更需要它:
//     starts_with 至少還能用 substr(0, n) 比較 (低效但直觀);
//     而 ends_with 因為 substr 必須計算 size() - suffix.size() 並且兩端都要對長度做防護,
//     寫起來最容易出錯。
//
// (4) 三種重載與 starts_with 完全對稱:
//     - sv (string_view) : 接受任何字串型別,免 allocation
//     - char             : 比 ends_with("c") 還精準;絕對 noexcept
//                          常用於檢查最後一字元 (例如句點、換行)
//     - const char*      : 不接受 nullptr;與 sv 重疊性高,通常用 sv 即可
//
// (5) 時間複雜度: O(N),N = suffix 長度。即使本字串很長也僅檢查尾部 N 字元。
//
// 【注意事項 Pay Attention】
// 1. C++20 之前的等效:
//      bool ew = (s.size() >= suf.size() &&
//                 s.compare(s.size()-suf.size(), suf.size(), suf) == 0);
// 2. suffix 比 s 長 → 直接 false。
// 3. 對檔案副檔名等任務,搭配 starts_with 是常見組合。
// 4. 中文/UTF-8 多位元組字元:逐 byte 比對通常仍能正確判斷後綴 (UTF-8 是自同步編碼),
//    但 char 重載僅檢查單 byte,中文字尾不應用 char 重載。
// 5. 編譯器要求: GCC 11+、Clang 12+、MSVC 19.22+。
//
// 【概念補充 Concept Deep Dive】
//
// (A) string_view 重載為什麼是 ends_with 的最佳選擇:
//     比對「尾段」需要先取出尾段才能比對。若用 substr 會新建 string;
//     用 string_view 則只是 (data + offset, len) ── 不配置記憶體。
//     ends_with 內部正是這樣做:不複製字串,只做指標與長度運算。
//
// (B) 為何 C++20 才加入 ── 與 starts_with 同個答案:
//     - C++ 標準演進緩慢
//     - C++17 才有 string_view,使簽章合理化
//     - C++20 同時引入 starts_with/ends_with,被視為「字串便利函式統一補強」
//     - Java 1.0 (1996) 就有 String.endsWith;C++ 等到 2020 年。
//
// (C) 副檔名檢查的標準寫法:
//
//         bool isImage(const std::string& f) {
//             return f.ends_with(".png") || f.ends_with(".jpg") ||
//                    f.ends_with(".gif") || f.ends_with(".webp");
//         }
//
//     比 substr+比較 更乾淨。對 case-insensitive 的需求需先 tolower 整段或用 ICU。
//
// (D) 行末 CRLF 的判斷:
//     ends_with("\r\n") 對網路協定 (HTTP、SMTP) 解析行邊界非常實用。
//     比手動 if (s[size-2] == '\r' && s[size-1] == '\n') 安全 (沒長度檢查的話會 UB)。
//
// (E) 與 std::ranges::ends_with (C++23) 的關係:
//     C++23 加入了通用版本,可作用於任何 range。對純字串需求 string::ends_with
//     仍是最直接、最被支援的選擇。
//
// =============================================================================

/*
補充筆記：std::string::ends_with
  - std::string::ends_with 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::ends_with 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::string::ends_with
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. ends_with 是哪個標準?手寫版本最容易錯在哪?
//     答：C++20(與 starts_with 同時引入,語意對稱)。
//         手寫 s.substr(s.size() - n) == suffix 有兩個問題:一是建臨時 string
//         (複製 + 可能 heap 配置);二是當 n > s.size() 時,s.size() - n 會無號回繞成
//         巨大值,substr 便拋 std::out_of_range。
//     追問：ends_with 遇到 suffix 比 s 長?→ 直接回傳 false,安全。
//
// 🔥 Q2. C++20 之前正確又零配置的寫法是什麼?
//     答：先檢查 s.size() >= n,再 s.compare(s.size() - n, n, suffix) == 0。
//         那個長度檢查不能省——它正是 ends_with 幫你封裝掉的部分。
//
// ⚠️ 陷阱. 拿它判副檔名要注意什麼?
//     答：ends_with 是逐位元組比較,不做大小寫轉換,也不理解 UTF-8 字元邊界
//         (size() 回傳的是位元組數,不是字元數)。所以 ".JPG" 不會被 ends_with(".jpg") 匹配。
//     為什麼會錯：把它想成「檔名比對工具」,它其實只是位元組層級的字尾比較。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>

void demoEndsWith() {
    std::string s = "image.png";
    std::cout << std::boolalpha;
    std::cout << s.ends_with(".png") << "\n";   // true
    std::cout << s.ends_with(".jpg") << "\n";   // false
    std::cout << s.ends_with('g')    << "\n";   // true

    // C++20 之前的等效寫法
    auto endsWithLegacy = [](const std::string& a, const std::string& b) {
        return a.size() >= b.size() &&
               a.compare(a.size() - b.size(), b.size(), b) == 0;
    };
    std::cout << "legacy = " << endsWithLegacy(s, ".png") << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 風格範例】檔案副檔名過濾 (例:LeetCode 2255 系列、檔案處理題常用)
// 題目情境: 給一組檔名,挑出所有圖片檔。
// 為何用 ends_with: 直接、可讀、不需 substr 配置;C++20 起的標準寫法。
// 複雜度:    O(N * |suffix|),N = 檔案數。
// -----------------------------------------------------------------------------
#include <vector>
std::vector<std::string> filterImages(const std::vector<std::string>& files) {
    std::vector<std::string> out;
    for (const auto& f : files) {
        if (f.ends_with(".png") || f.ends_with(".jpg") || f.ends_with(".gif")) {
            out.push_back(f);
        }
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1408. String Matching in an Array
// 題目: 找出陣列中是其他字串「子字串」的字串。
// 為何用 ends_with: 這題正規解用 find;這裡示範一個邊界情境:
//                   檢查某 word 是否被另一個 word 以「結尾」形式包含。
//                   完整解仍需用 find,但展示如何結合 ends_with。
// -----------------------------------------------------------------------------
std::vector<std::string> stringMatching(const std::vector<std::string>& words) {
    std::vector<std::string> out;
    for (size_t i = 0; i < words.size(); ++i) {
        for (size_t j = 0; j < words.size(); ++j) {
            if (i == j) continue;
            if (words[j].find(words[i]) != std::string::npos) {
                out.push_back(words[i]);
                break;
            }
        }
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】判斷是否為 rotated log file (.gz, .1, .2, ...)
// 為何用 ends_with: 維運常見 ── 找出所有「目前」log,排除已經 rotate 過的。
//                   配合多個 ends_with 寫成判斷式很直觀。
// -----------------------------------------------------------------------------
bool isRotatedLog(const std::string& filename) {
    if (filename.ends_with(".gz") || filename.ends_with(".bz2") ||
        filename.ends_with(".xz") || filename.ends_with(".zip")) return true;
    // 結尾是 .數字 (例如 access.log.1)
    size_t dot = filename.rfind('.');
    if (dot != std::string::npos && dot + 1 < filename.size()) {
        bool allDigits = true;
        for (size_t i = dot + 1; i < filename.size(); ++i) {
            if (filename[i] < '0' || filename[i] > '9') { allDigits = false; break; }
        }
        if (allDigits) return true;
    }
    return false;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 2185. Counting Words With a Given Prefix - 後綴版
// 題目: LeetCode 2185 變奏為「Counting Words With a Given Suffix」── 計算
//       words 中有幾個以 suffix 結尾。
// 為何用 ends_with: C++20 一行解,語意清晰,毋須擔心 underflow。
// 複雜度: O(N * suffix.size())。
// -----------------------------------------------------------------------------
int countSuffix(const std::vector<std::string>& words, const std::string& suffix) {
    int cnt = 0;
    for (const auto& w : words) {
        if (w.ends_with(suffix)) ++cnt;
    }
    return cnt;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】判斷文字行是否以 '\' 結尾 (續行符號)
// 為何用 ends_with(char): Makefile / shell script 用 '\' 表示這行未結束;
//                          ends_with(char) noexcept、最直接。
// -----------------------------------------------------------------------------
bool isLineContinuation(const std::string& line) {
    return !line.empty() && line.ends_with('\\');
}

int main() {
    demoEndsWith();

    std::cout << "\n=== filterImages ===\n";
    for (auto& f : filterImages({"a.txt","b.png","c.jpg","d.cpp"}))
        std::cout << f << "\n";

    std::cout << "\n=== LeetCode 1408 ===\n";
    for (auto& w : stringMatching({"mass","as","hero","superhero"}))
        std::cout << w << " ";
    std::cout << "\n";

    std::cout << "\n=== LeetCode 2185 變奏 ===\n";
    std::cout << countSuffix({"pay","attention","practice","attend"}, "tion") << "\n";  // 1
    std::cout << countSuffix({"hello","world","foo","helloworld"}, "ld") << "\n";       // 2

    std::cout << "\n=== 日常實務: 判斷 rotated log ===\n";
    for (auto& f : {"access.log","access.log.1","access.log.gz","app.log"}) {
        std::cout << f << " -> " << std::boolalpha << isRotatedLog(f) << "\n";
    }

    std::cout << "\n=== 日常實務: line continuation ===\n";
    std::cout << isLineContinuation("CFLAGS = -O2 \\") << "\n";   // true
    std::cout << isLineContinuation("clean:")         << "\n";   // false

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：ends_with 是 C++ 哪個版本加入的?與 starts_with 的關係?
    //    A：兩者都在 C++20 加入 (P0457),語意完全對稱。ends_with 是檢查
    //       後綴,starts_with 是檢查前綴。兩者都有 string_view / char /
    //       const char* 三種 overload,且都是 constexpr noexcept (除了
    //       const char* 版本因 strlen 不是 noexcept)。
    //
    //  Q2：C++20 之前要怎麼寫等效的 ends_with?
    //    A：必須先做長度防護 (size_type 是 unsigned,size() - suffix.size()
    //       若 suffix 較長會 underflow 變成超大值),再用 compare 比尾段:
    //         s.size() >= suf.size() && s.compare(s.size()-suf.size(),
    //                                              suf.size(), suf) == 0;
    //       這個寫法很容易漏掉長度檢查或寫錯 compare 的參數順序,所以
    //       ends_with 比 starts_with 更需要被加入標準。
    //
    //  Q3：ends_with(char) 與 ends_with("c") 哪個好?
    //    A：ends_with(char) 更好。它是 noexcept、實作通常一條
    //       size() > 0 && back() == ch 比較;ends_with("c") 走 const char*
    //       overload 需要 strlen 後再比尾段,雖然編譯器可能優化,但語意
    //       與成本明確選 ends_with(char) 較好。檢查行尾換行、句點時最常用。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra ends_with.cpp -o ends_with

// === 預期輸出 (節錄) ===
// === LeetCode 2185 變奏 ===
// 1
// 2
// === 日常實務: line continuation ===
// true
// false
