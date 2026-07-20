// =============================================================================
// 檔名: copy.cpp
// 主題: std::string::copy (把字串內容拷貝到 char buffer)
// 參考:
//   - https://en.cppreference.com/cpp/string/basic_string/copy
//   - https://cplusplus.com/reference/string/string/copy/
// =============================================================================
//
// 【函式資訊 Information】
//   size_type copy(char* dest, size_type count, size_type pos = 0) const;
//
// 回傳實際拷貝的字元數 (= min(count, size() - pos))。
//
// 【詳細解釋 Explanation】
//
// 一、設計理念
//   copy 是「把字串內容寫入 raw char buffer」的橋樑函式。
//   它服務的對象是「需要 char[N] 緩衝區」的場景:
//     - C 系統 API (POSIX、Windows API、結構欄位)。
//     - 二進位協議封包 (固定長度欄位)。
//     - 需要與舊 C 程式碼共存的低階介面。
//   現代 C++ 程式幾乎不會直接呼叫 copy ── 大多用 substr 或 std::copy 取代。
//
// 二、底層運作
//   實作幾乎等同:
//     size_t n = std::min(count, size() - pos);
//     std::char_traits<char>::copy(dest, data() + pos, n);
//     return n;
//   也就是「逐 byte memcpy」,效能與 memcpy 相同。
//
// 三、為何「不」附加 '\0' (重要差異!)
//   許多新手以為 copy 會像 strcpy 一樣自動補 '\0',結果造成「印 buffer 時亂碼」。
//   標準故意「不」加 '\0' 的原因:
//     1. 二進位資料友善 ── 若強制補 '\0',就無法寫入「剛好 N 字元」的固定欄位。
//     2. 呼叫者最了解需求 ── 是要 null-terminated string?還是 padded buffer?
//     3. 與 C 的 memcpy 行為一致,不額外越界寫入。
//   結論:呼叫者需要 C-string 時,自己寫 dest[copied] = '\0';
//
// 四、與其他函式比較 (重要對照表)
//   ┌─────────────┬───────────────┬───────────┬──────────┬─────────┐
//   │ 函式        │ 拷貝範圍      │ 補 '\0'?  │ 安全長度 │ 配置?  │
//   ├─────────────┼───────────────┼───────────┼──────────┼─────────┤
//   │ string::copy│ 固定 count    │ 否        │ 是       │ 否      │
//   │ strcpy      │ 直到 '\0'     │ 是        │ 否 (UB)  │ 否      │
//   │ strncpy     │ 固定 n (補'\0'│ 部分 (n   │ 是       │ 否      │
//   │             │ 或填滿)       │ 不夠才 0) │          │         │
//   │ memcpy      │ 固定 n        │ 否        │ 是       │ 否      │
//   │ substr      │ 固定 count    │ 是 (內部) │ 是       │ 是 (堆)│
//   │ std::copy   │ iterator 範圍 │ 否        │ 是       │ 否      │
//   └─────────────┴───────────────┴───────────┴──────────┴─────────┘
//
// 五、與 strncpy 的關鍵差異
//   strncpy 有兩個惱人特性,而 copy 都沒有:
//     (1) 來源長度 < n 時,strncpy 會「補 '\0' 填滿到 n」,浪費 CPU 且常被誤用。
//     (2) 來源長度 >= n 時,strncpy「不」補 '\0',造成「看似安全實則危險」。
//   string::copy 沒有這些坑:它「只」拷貝 min(count, size()-pos),
//   結尾要不要 '\0' 由你決定,行為單純可控。
//
// 六、為何很少用?
//   現代 C++ 處理字串通常選 substr (可獨立存活的 string),
//   或 std::string_view (零拷貝觀察)。copy 的場景被限縮到:
//     - 必須交給「不知道 std::string 的 C 函式」。
//     - 結構體中有「固定長度 char[N]」欄位需要填寫。
//     - 二進位協議封包格式化。
//   若程式碼幾乎不接 C API,你大概一輩子用不到 copy。
//
// 七、版本演進
//   - C++98 : 加入。
//   - C++11 : noexcept (條件式)。
//   - C++20 : constexpr。
//
// 時間複雜度: O(count)
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為何 copy 不安全?應該禁用嗎?
//   copy 本身是 type-safe 的 (回傳 size_t、不寫超過 count),
//   不安全是因為它要求「呼叫者保證 dest 容量足夠」── 沒有方式驗證。
//   靜態分析工具 (clang-tidy 的 cert-msc-) 會警告 strcpy 但通常不警告 string::copy,
//   因為後者已要求顯式 count。但仍要謹慎使用。
//
// (B) Two-Phase Copy 模式
//   常見模式:先用 size() 查詢字串長度,再 copy 到適當大小的 buffer:
//     size_t n = s.size();
//     auto buf = std::make_unique<char[]>(n + 1);
//     s.copy(buf.get(), n);
//     buf[n] = '\0';
//   雖然 substr().c_str() 更短,但這個模式在 C 互通時(例如 dup 後傳給 fork+exec)很常見。
//
// (C) 為何不直接 memcpy(dest, s.data(), n)?
//   功能完全一樣,且大多數實作 string::copy 內部就是呼叫 memcpy / char_traits::copy。
//   差別在「邊界檢查」── string::copy 會自動處理 pos > size() 的例外,
//   而 memcpy 不會。string::copy 是更「型別安全」的封裝。
//
// (D) Self-overlap 行為
//   若 dest 與 s.data() 有重疊區段 (例如把字串內容拷到自己內部某處),
//   行為「未定義」── 標準只保證類似 memcpy 而非 memmove。
//   實務上幾乎不會發生,但要避免奇怪的 in-place 操作。
//
// 【注意事項 Pay Attention】
// 1. dest 必須有足夠空間,否則 buffer overflow → UB。
// 2. copy 不附加 '\0',若要當作 C-string 使用,記得 dest[copied] = '\0';
// 3. pos > size() 會丟 std::out_of_range。
// 4. 與 strcpy 不同:strcpy 直到 '\0' 為止,copy 是固定長度。
// 5. 與 strncpy 不同:strncpy 會「填 '\0' 填滿到 n」,copy 不會。
// 6. 大多數情況可改用 substr 或 std::copy(string.begin(), ...) 替代。
// 7. 來源與目的不可重疊 (overlap) ── 不像 memmove。
//
// =============================================================================

/*
補充筆記：std::string::copy
  - std::copy 假設目的地範圍有足夠空間；如果目的容器是空的，應使用 back_inserter。
  - 來源與目的重疊時 copy 不一定安全；往右重疊通常應使用 copy_backward。
  - copy 是值複製；若元素拷貝成本高，請確認是否應改用 move 或保存指標/handle。
  - std::string::copy 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::string::copy
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. copy() 回傳什麼?典型用途是什麼?
//     答:回傳「實際拷貝的字元數」= min(count, size() - pos),型別是
//         size_type,不是 dest 指標(這點與 memcpy / strncpy 不同)。
//         最常見用途就是拿它補結尾:dest[copied] = '\0';
//     追問:pos 越界會怎樣?→ pos > size() 丟 std::out_of_range;
//           count 超過剩餘長度不會拋例外,會自動截短。
//
// 🔥 Q2. string::copy 和 strncpy 差在哪?
//     答:strncpy 有兩個坑——來源比 n 短時會補 '\0' 填滿到 n(浪費),
//         來源 >= n 時又「不」補 '\0'(看似安全實則危險)。
//         string::copy 行為單純:只拷 min(count, size()-pos) 個字元,
//         不多寫也不填充,結尾要不要 '\0' 完全由呼叫者決定。
//     追問:那為什麼不直接 memcpy(dest, s.data(), n)?→ 功能幾乎相同,
//           但 string::copy 會對 pos 做邊界檢查並丟例外,封裝較安全。
//
// ⚠️ 陷阱. char buf[32]; s.copy(buf, 5); std::cout << buf; 為什麼印出亂碼?
//     答:因為 copy() 絕對不會在尾端補 '\0'。buf 後面仍是未初始化的內容,
//         把它當 C-string 印就會一直印到碰巧遇到的某個 '\0' 為止。
//         正解:size_t n = s.copy(buf, 5); buf[n] = '\0';
//     為什麼會錯:大家把 copy 當成 strcpy 的成員函式版,以為會自動終止字串。
//         標準刻意不補,是為了讓固定長度的二進位欄位能剛好填滿 N 個 byte。
//
// ⚠️ 陷阱 2. dest 空間不夠會怎樣?
//     答:直接 buffer overflow → UB。copy() 只保證「不寫超過 count 個字元」,
//         但它無從得知 dest 到底有多大,容量是呼叫者的責任。
//         另外來源與目的若重疊也是 UB——它的語意是 memcpy 不是 memmove。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <cstring>

void demoCopy() {
    std::string s = "Hello, World!";
    char buf[32] = {0};

    size_t copied = s.copy(buf, 5, 7);    // 拷貝 5 個字元,從 pos=7 起
    buf[copied] = '\0';                   // 自己補上結尾
    std::cout << "copied=" << copied << ", buf=\"" << buf << "\"\n";  // "World"
}

// -----------------------------------------------------------------------------
// 【實務範例】把 std::string 內容寫入 C 風格固定緩衝區 (例如 syscall 結構)
// 為何用 copy: 與 C API 互動時,常需要寫入 char[N] 欄位 (如 sockaddr_un.sun_path)。
//              copy 提供安全寬度控制 (不會讀取超過 count 個字元)。
// -----------------------------------------------------------------------------
struct FixedRecord {
    char name[16];          // 固定長度欄位
    int  id;
};

FixedRecord makeRecord(const std::string& name, int id) {
    FixedRecord r{};
    size_t n = std::min(name.size(), sizeof(r.name) - 1);   // 留一位給 '\0'
    name.copy(r.name, n);
    r.name[n] = '\0';
    r.id = id;
    return r;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 28. Find the Index of the First Occurrence (strStr)
//
// 題目敘述:
//   給 haystack 與 needle 兩個字串,回傳 needle 在 haystack 中第一次出現
//   的索引;若不存在,回傳 -1。
//   範例:
//     haystack="sadbutsad", needle="sad"  → 0
//     haystack="leetcode",  needle="leeto"→ -1
//
// 為何用 copy:
//   現代解法直接 haystack.find(needle) 即可。本範例改示範一個有教學
//   意義的「混用 C 函式 + std::string::copy」實作 — 把 needle 內容用
//   string::copy 寫入一個 null-terminated 的 char buffer,然後呼叫 C
//   標準函式 strstr 做搜尋。這是 std::string 與「只認 const char*」
//   舊 API 互通的縮影:固定長度欄位、syscall buffer、log 標籤格式
//   都會走這條路。string::copy 的優勢是「明確指定長度,不會像 strcpy
//   那樣依賴 needle 結尾的 '\0',更適合二進位或長度未知的來源」。
//
// 解題思路:
//   1. 配置 char buf[needle.size() + 1] (+1 給 '\0')。
//   2. needle.copy(buf, needle.size()) 把字元拷貝過去 (copy 不附加 '\0')。
//   3. 自己補 buf[needle.size()] = '\0'。
//   4. 呼叫 strstr(haystack.c_str(), buf):若回 nullptr → -1,
//      否則用指標相減算 offset。
//
// 注意:這個寫法是為了示範 copy + C 互通,效能上 std::string::find 通常更好。
//
// 複雜度:時間 O(N * M) (依 strstr 實作而定),空間 O(M)。
// -----------------------------------------------------------------------------
int strStr(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return 0;
    // 1) 把 needle 內容用 copy 拷貝到 null-terminated char buffer
    std::string buf_holder(needle.size() + 1, '\0');  // 內含結尾 '\0'
    needle.copy(buf_holder.data(), needle.size());     // 注意 copy 不會加 '\0'
    // buf_holder[needle.size()] 已是 '\0' (上面初始化時就填好)

    // 2) 用 C 標準的 strstr 做搜尋
    const char* hp = haystack.c_str();
    const char* found = std::strstr(hp, buf_holder.c_str());
    return (found == nullptr) ? -1 : static_cast<int>(found - hp);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】填入網路封包欄位 (固定長度,不要 '\0')
// 為何用 copy: 與 strncpy 不同,std::string::copy 不會自動補 '\0'。
//              對「靠長度識別」的二進位協議 (如某些金融訊息) 反而是優點。
//              另外可控制是否補空白填滿到固定長度。
// -----------------------------------------------------------------------------
void fillFixedField(char* dest, size_t fieldSize,
                    const std::string& value, char pad = ' ') {
    size_t n = std::min(value.size(), fieldSize);
    value.copy(dest, n);
    if (n < fieldSize) {
        std::fill(dest + n, dest + fieldSize, pad);
    }
}

#include <algorithm>

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1929. Concatenation of Array
// 題目: 給陣列 nums,回傳 ans = nums + nums (長度 2n)。
// 為何用 copy: 變奏為字串版 ── 把 s 串自身,把 s 的內容用 copy 拷到 buffer 的第二段。
//              展示 copy 寫入「指定 offset」的用法。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
std::string concatSelf(const std::string& s) {
    std::string out(s.size() * 2, '\0');
    s.copy(out.data(), s.size(), 0);
    s.copy(out.data() + s.size(), s.size(), 0);
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】把字串前 N 字拷貝到 C-style char buffer 並補 NUL
// 為何用 copy: 對接舊 C API (如 syslog、SDK 結構欄位) 時最常見的橋接做法。
//              copy 不自動補 NUL,需要呼叫後自己手動加 '\0'。
// -----------------------------------------------------------------------------
void toCBufferWithNul(const std::string& s, char* dest, size_t cap) {
    if (cap == 0) return;
    size_t n = s.copy(dest, cap - 1);     // 留一格放 NUL
    dest[n] = '\0';
}

int main() {
    demoCopy();

    std::cout << "\n=== makeRecord ===\n";
    auto r1 = makeRecord("Alice", 1);
    auto r2 = makeRecord("ThisNameIsTooLongToFit", 2);  // 會被截斷
    std::cout << r1.name << " / " << r1.id << "\n";
    std::cout << r2.name << " / " << r2.id << "\n";

    std::cout << "\n=== 日常實務: 固定長度欄位 ===\n";
    char field[8];
    fillFixedField(field, sizeof(field), "ABC", ' ');
    std::cout << "[";
    for (size_t i = 0; i < sizeof(field); ++i) std::cout << field[i];
    std::cout << "]\n";

    std::cout << "\n=== LeetCode 28: strStr (用 copy + C strstr) ===\n";
    std::cout << "haystack=\"sadbutsad\", needle=\"sad\"  → "
              << strStr("sadbutsad", "sad")    << "\n";    // 0
    std::cout << "haystack=\"leetcode\",  needle=\"leeto\"→ "
              << strStr("leetcode",  "leeto")  << "\n";    // -1
    std::cout << "haystack=\"hello\",     needle=\"\"     → "
              << strStr("hello",     "")       << "\n";    // 0

    std::cout << "\n=== LeetCode 1929 (concatSelf) ===\n";
    std::cout << concatSelf("abc") << "\n";   // abcabc
    std::cout << concatSelf("12")  << "\n";   // 1212

    std::cout << "\n=== 日常實務: toCBufferWithNul ===\n";
    char cbuf[8];
    toCBufferWithNul("HelloWorld", cbuf, sizeof(cbuf));
    std::cout << "[" << cbuf << "]\n";        // [HelloWo]

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：std::string::copy 與 std::copy 有什麼差別?
    //    A：std::string::copy(dest, count, pos) 是成員函式,直接 memcpy 效能
    //       與寫法都最直接;但拿不到 iterator,只能傳 char*。std::copy 是
    //       <algorithm> 中的泛型演算法,接受 iterator 範圍可寫到任何 output
    //       iterator (例如 std::back_inserter)。寫到 char[N] buffer 時兩者
    //       效能相同,選 string::copy 較簡潔。
    //
    //  Q2：copy() 為什麼不會自動補 '\0'?與 strncpy 比較有何優勢?
    //    A：標準刻意不補 '\0',讓 copy 對「二進位固定欄位」友善 (例如金融
    //       訊息協議的 8 byte 欄位,需要剛好 8 byte 不能多 '\0')。比 strncpy
    //       好的地方:strncpy 來源短於 n 時會「補滿 '\0' 直到 n 個 byte」,
    //       浪費 CPU 又常被誤用;copy 行為單純可控:就只拷 min(count, size-pos)。
    //
    //  Q3：copy() 回傳什麼?需要它做什麼?
    //    A：回傳實際拷貝的字元數 = min(count, size() - pos),型別是 size_type。
    //       常見用途:呼叫後立即拿這個回傳值補 '\0' (dest[copied] = '\0' 把
    //       buffer 變成 C-string),或加總到流量計數器。回傳值不是 dest 指標,
    //       這點與 memcpy / strncpy 不同。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra copy.cpp -o copy

// === 預期輸出 (節錄) ===
// === LeetCode 1929 (concatSelf) ===
// abcabc
// 1212
// === 日常實務: toCBufferWithNul ===
// [HelloWo]
