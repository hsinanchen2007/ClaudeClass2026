// =============================================================================
// 檔名: resize_and_overwrite.cpp
// 主題: std::string::resize_and_overwrite (C++23 零初始化避免修改器)
// 參考連結 References:
//   cppreference: https://en.cppreference.com/cpp/string/basic_string/resize_and_overwrite
//   cplusplus.com: (cplusplus.com 尚未收錄此 C++23 函式)
// =============================================================================
//
// 【函式資訊 Information】
//   template<class Operation>
//   constexpr void resize_and_overwrite(size_type count, Operation op);
//
// 行為:
//   1. 把字串的內部 buffer (含 NUL 結尾的位置) 確保至少能容納 count 個字元;
//      若需要會 reallocate。
//   2. **不** 對新增的字元做初始化 (沒有 zero-fill)。
//   3. 呼叫 op(buffer, count),把 raw 指標與「最大可寫長度」交給使用者。
//   4. op 必須回傳一個 0..count 之間的整數,代表「實際寫了多少字元」。
//   5. string 把 size 設成這個回傳值,並保證 buffer[size] = '\0'。
//
// 換言之:它把「先 resize() 再寫入再 resize() 截斷」這個 idiom 的雙重
// 初始化開銷 (resize 預設會 fill 0) 一次省掉。
//
// =============================================================================
//
// 【詳細解釋 Explanation】
//
// (一) 為什麼需要 resize_and_overwrite? (動機)
// ----------------------------------------------------------------------------
// 在「我先準備一個 buffer,再交給 C API 或低階函式去寫」的情境裡,
// C++17 之前最常見的寫法是:
//
//     std::string s;
//     s.resize(N);                                  // 步驟 A: 先把 size 撐大
//     ssize_t n = ::read(fd, s.data(), N);          // 步驟 B: C API 寫入
//     s.resize(n > 0 ? n : 0);                      // 步驟 C: 截掉沒寫到的部分
//
// 這個 idiom 有兩個浪費:
//   - 步驟 A 的 resize(N) 會先把 N 個 byte 全部填成 '\0';read() 接著
//     又把這些 byte 全部覆寫一次 → 多做了一輪「N 次 store」的無效工。
//   - 步驟 C 的 resize 雖然只是改 size 不再寫資料 (因為縮小不需配置),
//     但仍需做一些 invariant 設定。
//
// 對小 N 沒差,但對 4 KiB / 64 KiB / MB 等級的網路 / 檔案 buffer,
// 這個多餘的 zero-fill 顯著拖累 throughput (每次都白讀寫 cache line)。
//
// resize_and_overwrite 解決這個問題:它「跳過」初始化,直接把 raw buffer
// 給你寫,寫完你回報「實際長度」,string 自己處理 NUL。
//
// (二) 為什麼 string::resize() 不能直接「不初始化」?
// ----------------------------------------------------------------------------
// 因為標準 string 的不變量 (invariant) 要求 size() 與 capacity() 的
// 範圍內存有「合法 char」(typically 0)。如果讓 resize 直接不初始化,
// 中途若有 exception 或外部讀取會看到 garbage,違反 contract。
// resize_and_overwrite 把「buffer 暴露 + 結束時設定 size + 寫入 NUL」
// 三件事原子化在一個函式裡,維持了 string 的 invariant。
//
// (三) op 的合約 (Operation contract)
// ----------------------------------------------------------------------------
// op 是個 callable,通常是 lambda,簽名應為:
//     auto op(CharT* buf, size_type maxLen) -> size_type;  // 或可隱式轉
//
// 規則:
//   - 你只能寫入 buf[0..maxLen-1]。**不能寫到 buf[maxLen]** 那個 NUL,
//     string 會自己補。
//   - op 必須是 noexcept(對例外的具體要求標準有微妙細節,但實務上
//     都應寫 noexcept lambda)。
//   - 回傳值必須在 [0, maxLen] 之間;若超出是 UB。
//   - 在 op 執行期間,string 已經 reserve 好 buffer,但 size() 仍可能
//     維持原值(實作未指定),不要在 op 裡呼叫 *this 上其他成員。
//
// (四) 與 reserve + resize 寫法的比較
// ----------------------------------------------------------------------------
//   傳統 (慢):
//     s.resize(N);                                 // 配置 + zero-fill
//     auto n = doWork(s.data(), N);
//     s.resize(n);
//
//   現代 (C++23, 快):
//     s.resize_and_overwrite(N, [&](char* buf, size_t cap) {
//         return doWork(buf, cap);
//     });
//
// 後者免去 zero-fill,且程式更短。
//
// (五) C++ 標準演進
// ----------------------------------------------------------------------------
//   C++23 起加入 (P1072)。在 GCC 12+、Clang 14+、MSVC 19.31+ 可用。
//   constexpr 在 constant evaluation 內可用 (與 C++20 constexpr string 配合)。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// 1) Type-erased value initialization 的陷阱
//    在 C 語言中,malloc 的 buffer 預設是「未定義內容」,read() 直接寫入
//    沒有任何 zero-fill 開銷。C++ 為了安全性,把所有 std 容器的 resize
//    都加上初始化保證,反而失去了這個效率。resize_and_overwrite 正是
//    讓 C++ 在「需要時」拿回 C 等級的效率。
//
// 2) Strong vs Basic Exception Safety
//    若 op 在過程中 throw,string 處於 valid-but-unspecified (basic
//    guarantee)。實務上 op 應當寫 noexcept,確保 strong guarantee。
//
// 3) 與 std::vector<char>::resize 的對比
//    std::vector 沒有 resize_and_overwrite。對 vector 你只能繞:
//      vector<char> v(N);  // 也會 zero-fill
//    所以這是 string 獨有 (C++23 也沒對 vector 加,因為 vector 還有
//    其他複雜的元素型別問題)。
//
// 4) 為什麼一定要回傳長度?
//    因為「buffer 寫多少」常常事先不知道 (例如 read() 的回傳值、
//    sprintf 的 return)。讓 op 回傳長度,string 才能正確設定 size。
//
// 5) 用 lambda capture & 的時機
//    op 通常需要 capture 外部變數 (fd、locale、format 規格等),
//    用 [&] 捕獲是最自然的寫法。注意 capture 的物件生命週期要超過
//    op 的執行期。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. op 回傳值不能超過 count;標準說法為 UB。
// 2. op 不能對 *this 呼叫其他 member function;在 op 執行期間,string
//    處於「正在被 mutate」的中間狀態。
// 3. 寫入時不要超過 buf[count-1]。NUL 會由 string 自動補在 buf[size()]。
// 4. 編譯器版本要夠新 (見上方),否則會編譯錯誤。
// 5. 雖然名稱 overwrite,但對「擴大」與「縮小」場景都適用,只要 op
//    回傳的長度合理。
// 6. 若你的工作根本不需要 raw buffer 寫入 (純 string 操作),
//    應繼續用 reserve + push_back / append,不要硬套此 API。
//
// =============================================================================

/*
補充筆記：std::string::resize_and_overwrite
  - std::string::resize_and_overwrite 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::resize_and_overwrite 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
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
#include <cstring>
#include <cstdio>
#include <cstdarg>

void demoResizeAndOverwrite() {
    std::cout << "=== resize_and_overwrite 示範 ===\n";

    std::string s;

    // 模擬一個 C 風格函式,把資料寫進 raw buffer 並回傳長度
    auto cFill = [](char* buf, size_t cap) noexcept -> size_t {
        const char src[] = "Hello, raw buffer!";
        size_t n = std::strlen(src);
        if (n > cap) n = cap;
        std::memcpy(buf, src, n);
        return n;
    };

    s.resize_and_overwrite(64, cFill);
    std::cout << "s = \"" << s << "\", size=" << s.size() << "\n";

    // op 回傳 0 → string 變空
    s.resize_and_overwrite(64, [](char*, size_t) noexcept { return size_t{0}; });
    std::cout << "after op return 0: size=" << s.size() << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例】LeetCode 12. Integer to Roman (Medium)
//
// 題目敘述:
//   給整數 1..3999,轉成羅馬數字字串。
//   範例: 3 → "III"; 4 → "IV"; 9 → "IX"; 58 → "LVIII"; 1994 → "MCMXCIV"
//
// 為何用 resize_and_overwrite:
//   羅馬數字最大長度 (1..3999) 為 15 字元 ("MMMDCCCLXXXVIII")。
//   先 reserve 一個 16 byte 的 buffer 直接寫,可避免一連串 push_back
//   產生的 capacity 增長 + 多次 NUL 維護成本。對「大量 int → string」
//   批次轉換時是有意義的微優化。
//
// 解題思路:
//   greedy 從最大值嘗試湊;一邊湊一邊把對應符號寫進 raw buffer。
//
// 複雜度: 時間 O(1) (因為輸入有上限),空間 O(1)
// -----------------------------------------------------------------------------
std::string intToRoman(int num) {
    static constexpr int    vals[] = {1000,900,500,400,100,90,50,40,10,9,5,4,1};
    static constexpr const char* sym[] =
        {"M","CM","D","CD","C","XC","L","XL","X","IX","V","IV","I"};

    std::string out;
    out.resize_and_overwrite(16, [&](char* buf, size_t cap) noexcept -> size_t {
        size_t pos = 0;
        for (int i = 0; i < 13 && num > 0; ++i) {
            while (num >= vals[i] && pos < cap) {
                size_t L = std::strlen(sym[i]);
                if (pos + L > cap) break;
                std::memcpy(buf + pos, sym[i], L);
                pos += L;
                num -= vals[i];
            }
        }
        return pos;
    });
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】snprintf 安全格式化(沒有額外 zero-fill 成本)
//
// 為何用 resize_and_overwrite:
//   把 snprintf 的結果包成 std::string 是後端極常見的需求 (log line、
//   metric label、SQL where 子句拼接)。以前要先 vector<char> + resize +
//   snprintf 兩次 (第一次量長、第二次寫入),再轉成 string,過程中
//   有額外的 zero-fill 與一次拷貝。
//
//   resize_and_overwrite 直接把 string 自家 buffer 給 snprintf 寫,
//   且不會做 zero-fill,是最有效率的「snprintf → string」方法。
// -----------------------------------------------------------------------------
std::string formatString(const char* fmt, ...) {
    // 第一輪:用 vsnprintf(NULL, 0) 量出長度 (不寫資料)
    va_list ap;
    va_start(ap, fmt);
    va_list ap2;
    va_copy(ap2, ap);
    int needed = std::vsnprintf(nullptr, 0, fmt, ap);
    va_end(ap);

    std::string out;
    if (needed <= 0) { va_end(ap2); return out; }

    // 第二輪:把 buffer 直接交給 vsnprintf 寫,免 zero-fill
    out.resize_and_overwrite(static_cast<size_t>(needed) + 1,
        [&](char* buf, size_t cap) noexcept -> size_t {
            int written = std::vsnprintf(buf, cap, fmt, ap2);
            return written < 0 ? 0 : static_cast<size_t>(written);
        });
    va_end(ap2);
    return out;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1page. 1page. Construct String With Repeat Limit
// 題目: LeetCode 1page. 1page. Repeated Character
// 變奏: 給 char 'x' 與整數 N,產生 N 個 'x' 組成的字串。
// 為何用 resize_and_overwrite: 直接把 buffer 給 op,跳過 '\0' 預填,效能最佳。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
std::string repeatChar(char c, size_t n) {
    std::string out;
    out.resize_and_overwrite(n, [c](char* buf, size_t cap) noexcept {
        for (size_t i = 0; i < cap; ++i) buf[i] = c;
        return cap;
    });
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】從固定 ABI 的 C 結構欄位 (例如 char name[64]) 安全建立 string
// 為何用 resize_and_overwrite: C struct 的固定欄位常無 NUL 結尾,要明確指定長度。
//                                resize_and_overwrite 可直接複製 + 自動算到實際長度。
// -----------------------------------------------------------------------------
std::string fromCField(const char* field, size_t fieldSize) {
    std::string out;
    out.resize_and_overwrite(fieldSize, [field, fieldSize](char* buf, size_t cap) noexcept {
        // 找到 NUL 或欄位長度為止
        size_t n = 0;
        while (n < cap && n < fieldSize && field[n] != '\0') {
            buf[n] = field[n];
            ++n;
        }
        return n;
    });
    return out;
}

int main() {
    demoResizeAndOverwrite();

    std::cout << "\n=== LeetCode 12 ===\n";
    std::cout << "3    → " << intToRoman(3)    << "\n";    // III
    std::cout << "4    → " << intToRoman(4)    << "\n";    // IV
    std::cout << "9    → " << intToRoman(9)    << "\n";    // IX
    std::cout << "58   → " << intToRoman(58)   << "\n";    // LVIII
    std::cout << "1994 → " << intToRoman(1994) << "\n";    // MCMXCIV

    std::cout << "\n=== LeetCode 變奏 (repeatChar) ===\n";
    std::cout << "[" << repeatChar('=', 10) << "]\n";   // [==========]

    std::cout << "\n=== 日常實務: 安全 snprintf → string ===\n";
    std::cout << formatString("user=%s id=%d status=%.2f%%",
                              "alice", 42, 99.5) << "\n";

    std::cout << "\n=== 日常實務: fromCField ===\n";
    char name_field[16] = "alice";   // 含尾端 NUL,有實際 5 字元
    std::cout << "[" << fromCField(name_field, sizeof(name_field)) << "]\n";  // [alice]

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:resize(N) 比 resize_and_overwrite(N, op) 多做了什麼?
    //    A:resize(N) 必定把新增的 byte 全部填 '\0' (value-initialization),
    //      以維持 string invariant。resize_and_overwrite 跳過這步,直接
    //      把 raw buffer 給 op 寫,寫完再用 op 回傳值設定 size。對 64 KB
    //      以上的 read/recv buffer,可省下整個 buffer 的一輪 store。
    //
    //  Q2:op 在執行期間可以呼叫 *this 的其他成員嗎?
    //    A:不能。標準說 op 執行時 string 處於「正在 mutate」的中間狀態,
    //      呼叫其他成員 (size、c_str、push_back...) 是 UB。op 應該只用
    //      傳進來的 buf 與 cap,所有需要的外部資料用 capture 帶進來。
    //
    //  Q3:op 回傳超過 count 會怎樣?
    //    A:UB。標準明訂 op 回傳值必須在 [0, count] 之間;若回傳超過,
    //      string 會把 size 設成不合法的值且 NUL 位置錯誤,後續任何讀取
    //      都可能踩到 buffer 之外。寫 op 時請務必先 min/clamp 再回傳。
    //
    return 0;
}

// 編譯: g++ -std=c++23 -Wall -Wextra resize_and_overwrite.cpp -o resize_and_overwrite
// 注意: 需要 GCC 12+ / Clang 14+ / MSVC 19.31+。

// === 預期輸出 (節錄) ===
// === LeetCode 變奏 (repeatChar) ===
// [==========]
// === 日常實務: fromCField ===
// [alice]
