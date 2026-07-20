// =============================================================================
// 檔名: data.cpp
// 主題: std::string::data (取得內部緩衝區指標)
// 參考連結 References:
//   cppreference: https://en.cppreference.com/cpp/string/basic_string/data
//   cplusplus.com: https://cplusplus.com/reference/string/string/data/
// =============================================================================
//
// 【函式資訊 Information】
//   const char* data() const noexcept;        // C++03 起
//   char*       data()       noexcept;        // C++17 起,允許非 const
//
// 回傳: 指向內部字元陣列的指標,後面緊跟一個 '\0' (C++11 起保證)。
//
// =============================================================================
//
// 【詳細解釋 Explanation】
//
// (一) data() 的角色:C++ 與 C 的橋樑
// ----------------------------------------------------------------------------
// std::string 是 C++ 物件,但作業系統 / C library / 第三方 C API 多數仍以
// const char* + size_t 的形式收受字串。data() 就是把 string 內部的記憶體
// 暴露出來,讓 C API 可以直接讀取 (write、send、printf、strstr 等)。
//
// 換言之,data() 不是「複製」一份資料,而是「借出 buffer 指標」。任何透過
// 這個指標的存取都直接反映在 string 上,而 string 本身沒被修改 (只要你
// 不寫進去)。
//
// (二) data() 與 c_str() 的關係:幾乎雙胞胎
// ----------------------------------------------------------------------------
//   - C++11 起,兩者都保證回傳 null-terminated 的 C-string。
//   - C++17 起,data() 多了「非 const 版本」可寫,c_str() 永遠是 const。
//   - 兩者時間複雜度都是 O(1),都不會配置新記憶體。
//
// 何時選哪個?
//   - 把 string 餵給 C API (例如 printf("%s", ...)) → c_str() 語意清楚
//   - 需要寫入 buffer (與 OS API 對接、例如 read(fd, p, n)) → data()  (C++17+)
//   - 與 std::vector::data() 介面一致時優先用 data()
//
// (三) C++11 之前 data() 的歷史包袱
// ----------------------------------------------------------------------------
// C++03 標準下,data() 不保證以 '\0' 結尾,行為與 c_str() 不同。
// 也就是 C++03 的 data() 只能拿來搭 size() 一起讀 (像 buffer + length 對),
// 不能當 C-string 直接傳給 strlen。
// 從 C++11 起,標準明文要求 data()[size()] 必須是 '\0',此後 data() 與
// c_str() 行為等價。
//
// (四) 非 const data() (C++17) 為何重要?
// ----------------------------------------------------------------------------
// 在 C++17 之前,要把 string 的 buffer 當寫入目標 (例如系統呼叫 read()
// 把資料放進來),只能:
//   - 先 resize 到足夠長度
//   - 對 &s[0] 寫入 (一直是合法操作,但語意奇怪)
// C++17 提供非 const data() 後,可以堂堂正正寫:
//   ssize_t n = read(fd, s.data(), s.size());
// 程式碼意圖更清楚,也更符合「把 string 當 buffer 用」的常見場景。
//
// (五) 時間複雜度
// ----------------------------------------------------------------------------
// O(1) — 直接回傳內部指標。
//
// (六) noexcept 的意義
// ----------------------------------------------------------------------------
// data() 標記為 noexcept,代表它絕不會丟例外。這個保證很重要,因為:
//   - 在 destructor、catch 區塊中可以安心呼叫
//   - 編譯器可以做更激進的最佳化 (不必生成例外處理 stack frame)
//
// (七) C++ 標準演進
// ----------------------------------------------------------------------------
//   C++03 : 只有 const 版本,且不保證 null-terminated。
//   C++11 : 保證 data()[size()] == '\0'。
//   C++17 : 加入非 const 版本,允許直接寫入。
//   C++20 : data() 在 constexpr 上下文可用。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// 1) Pointer Invalidation (指標失效) 的重要紀律
//    data() 回傳的指標在以下操作後會失效 (dangling):
//      - reserve() 觸發成長
//      - append() / insert() / += 導致 capacity 變化
//      - operator= / swap()
//      - clear() (size 變 0,但 capacity 通常不變,指標可能仍可解參考但語意已變)
//      - destructor
//    經驗法則:取得 data() 後,別在不可控的代碼段中保留太久。最安全的
//    用法是「取得 → 立刻交給 C API → 不再使用」。
//
// 2) 與 std::span / std::string_view 的搭配
//    C++17 起,常見模式是把 (data(), size()) 包成 std::string_view,
//    傳給 view-friendly 的 API:
//        my_api(std::string_view(s.data(), s.size()));
//    或更簡單地直接把 string 隱式轉成 string_view:
//        my_api(s);
//    這比手動傳 (data, size) 更安全 (string_view 自帶 size,不會誤算長度)。
//
// 3) 為何 C++17 加非 const data() 而不是「乾脆讓 c_str() 也非 const」?
//    保留 c_str() 的純 const 語意,讓「給 C API 用」這件事的意圖永遠
//    乾淨明確。data() 則承擔「buffer 指標」的雙重身份 (讀+寫)。這是
//    語意分工的好設計。
//
// 4) 嵌入 '\0' 的字串
//    std::string 可以包含內嵌的 '\0' 字元 (size 計入,但 strlen 會在第一個
//    '\0' 截斷)。這時 data() + size() 才是「完整內容」,c_str() / strlen
//    會誤判長度。處理二進位資料時要特別小心。
//
// 5) constexpr basic_string (C++20)
//    C++20 起 std::string 大多數操作可以在 constexpr 上下文使用,包括
//    data()。這讓編譯期字串處理 (例如靜態雜湊表的 key) 成為可能。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. 不要在指標仍被使用時對 string 做 reallocation 操作 (reserve、append、
//    insert 觸發成長等),否則指標會懸掛 (dangling)。
// 2. 不能寫入 data()[size()] 為非 '\0' 值 (該位置必為 '\0'),否則 UB。
// 3. data() 在 C++03 不保證以 '\0' 結尾;C++11 起保證。
// 4. 給 C API 用時優先選 c_str() (語意更清楚);需要寫入 buffer 時用 data()。
// 5. 處理含內嵌 '\0' 的二進位資料,務必傳 (data(), size()) 而非單獨 data()。
//
// =============================================================================

/*
補充筆記：std::string::data
  - data 回傳 string 內部緩衝區；C++17 起非 const string 的 data 可用來修改字元。
  - 修改不能越過 size，也不能自己寫入超出範圍的 null terminator。
  - 任何造成重配的操作都會讓先前 data 指標失效。
  - std::string::data 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::string::data
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. data() 和 c_str() 差在哪?各自從哪個標準版本開始是現在這樣?
//     答:C++11 起,const 版本的 data() 與 c_str() 完全等價 —— 都回傳
//         null-terminated 的 const char*,且 data()[size()] == '\0' 是標準保證
//         (c_str() 從 C++98 就有此保證,data() 是 C++11 才補上)。
//         C++17 起 data() 另有非 const overload 回傳 char*,可直接寫入 buffer;
//         c_str() 至今只有 const 版本。
//     追問:非 const data() 可以寫 data()[size()] 嗎?→ 可以讀,但寫成
//         非 '\0' 的值是 UB,那會破壞 null-terminated 的不變式。
//
// 🔥 Q2. 為什麼 C++11 之後 COW(Copy-On-Write)實作不再合法?
//     答:C++11 沒有明文寫「禁止 COW」,而是新增的要求讓 COW 無法合法實作,
//         其中最關鍵的一條正好牽涉本檔:非 const 的 data()、operator[]、
//         at()、begin()、end() 被要求「不得使既有指標/引用/迭代器失效」——
//         但 COW 必須在此刻「分家」複製,勢必失效。
//         另外標準要求對不同 string 物件的並行操作不得產生 data race
//         (COW 共享 buffer 與 refcount 會違反),且 operator[] 要求攤還常數
//         時間(COW 的隱藏 deep copy 破壞此性質)。結果是現代實作全面改用
//         SSO + eager copy。
//     追問:GCC 為此付出什麼代價?→ GCC 5 改寫 basic_string 造成 ABI 斷裂,
//         才有 std::__cxx11 這個 inline namespace 與 Dual ABI。
//
// ⚠️ 陷阱.「data() 不保證 null 結尾,所以不能直接餵給 C API」—— 對嗎?
//     答:這是 C++98/03 的正確答案,對 C++11 之後是錯的。C++11 起
//         data() 與 c_str() 指向同一塊 null-terminated 的 buffer,
//         直接傳給 C API 沒有問題。
//     為什麼會錯:這句話在舊教材裡是標準答案,很多人背了就沒再更新。
//         要注意真正還成立的限制是另一件事:若字串「中間」含有 '\0',
//         C API 仍會在第一個 '\0' 停止 —— 那是 C 字串的限制,不是
//         data() 少了結尾 '\0'。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <cstring>
#include <vector>

void demoData() {
    std::string s = "Hello";

    // 透過 data() 直接取得指標傳給 C API
    std::printf("data()    = %s\n", s.data());
    std::printf("strlen    = %zu\n", std::strlen(s.data()));

    // C++17 起可以非 const 修改
    s.data()[0] = 'h';
    std::cout << "after write: " << s << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例】LeetCode 459. Repeated Substring Pattern (Easy)
//
// 題目敘述:
//   給定字串 s,判斷它是否能由它的某個 substring 重複多次組成。
//   範例: "abab" → true  (由 "ab" 重複 2 次)
//        "aba"  → false
//        "abcabcabcabc" → true  (由 "abc" 重複 4 次,或 "abcabc" 重複 2 次)
//
// 為何用 data:
//   有個經典技巧:s 是 repeated 的「若且唯若」(s+s).substr(1, 2n-2) 包含 s。
//   範例用 std::strstr (C 函式) 來搜尋子字串,需要 const char*,
//   data() 提供原始指標,完美對接 C API。展示 std::string 與 C 標準
//   函式庫的互通性。
//
// 解題思路:
//   把 s 接成兩倍 doubled = s + s,去除頭尾各一字元得到 trimmed,
//   然後檢查 trimmed 是否包含 s。
//   證明:若 s 是某 substring 重複而成,doubled 中至少還會出現一次 s
//   (錯位剛好對齊);若不是,則 trimmed 中找不到 s。
//
// 複雜度: 時間 O(n) (strstr 在多數實作為 KMP 或類似演算法),空間 O(n)
// -----------------------------------------------------------------------------
bool repeatedSubstringPattern(const std::string& s) {
    std::string doubled = s + s;
    std::string trimmed = doubled.substr(1, doubled.size() - 2);
    // 用 strstr (C API) 找子字串
    return std::strstr(trimmed.data(), s.data()) != nullptr;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】把 std::string 的內容餵給 write() 系統呼叫
//
// 為何用 data:
//   POSIX 的 write(fd, buf, len) 要求 (const void*, size_t)。
//   data() 提供 raw pointer,size() 提供長度,完美匹配。
//   寫 log、傳 socket、寫檔、stdout 輸出 — 所有「把 string 送出去」
//   的場景幾乎都會走這個路徑。
//
// 注意:write() 可能 partial write (只寫了一部分),所以用 while 迴圈
// 確保全部寫完。這也是「真正生產級的 IO 寫法」。
// -----------------------------------------------------------------------------
#include <unistd.h>
ssize_t writeAll(int fd, const std::string& payload) {
    ssize_t total = 0;
    const char* p = payload.data();
    size_t left = payload.size();
    while (left > 0) {
        ssize_t n = write(fd, p + total, left);
        if (n < 0) return -1;            // 失敗
        if (n == 0) break;
        total += n;
        left  -= static_cast<size_t>(n);
    }
    return total;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1773. Count Items Matching a Rule
// 題目: items 是 [type, color, name] 三欄;依 ruleKey/ruleValue 計數。
// 為何用 data: 我們示範把規則值與物件欄位字串用 std::memcmp + .data() 直接比對,
//              零拷貝且不依賴 NUL。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
#include <cstring>
int countMatches(const std::vector<std::vector<std::string>>& items,
                 const std::string& ruleKey,
                 const std::string& ruleValue) {
    int idx = (ruleKey == "type") ? 0 : (ruleKey == "color") ? 1 : 2;
    int cnt = 0;
    for (const auto& it : items) {
        const std::string& v = it[idx];
        if (v.size() == ruleValue.size() &&
            std::memcmp(v.data(), ruleValue.data(), v.size()) == 0) ++cnt;
    }
    return cnt;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】用 data() 從 socket 讀進預先 resize 的 string buffer
// 為何用 data: read() 需要 char*,C++17 起 non-const data() 提供合法 char*。
//              這是「string 當作可寫 buffer」的標準寫法,免去 vector<char> + 轉換。
// (本範例只示範介面,不真的接 socket — 用 memcpy 模擬讀入)
// -----------------------------------------------------------------------------
size_t fakeRecvIntoString(std::string& buf, const char* fakeNetwork, size_t n) {
    if (buf.size() < n) buf.resize(n);
    std::memcpy(buf.data(), fakeNetwork, n);    // 等價 read(fd, buf.data(), n)
    return n;
}

int main() {
    demoData();
    std::cout << "\n=== LeetCode 459 ===\n";
    std::cout << std::boolalpha
              << repeatedSubstringPattern("abab") << "\n"      // true
              << repeatedSubstringPattern("aba")  << "\n"      // false
              << repeatedSubstringPattern("abcabcabcabc") << "\n";// true

    std::cout << "\n=== LeetCode 1773 ===\n";
    std::vector<std::vector<std::string>> items = {
        {"phone","blue","pixel"},
        {"computer","silver","lenovo"},
        {"phone","gold","iphone"}};
    std::cout << countMatches(items, "color", "silver") << "\n";   // 1
    std::cout << countMatches(items, "type",  "phone")  << "\n";   // 2

    std::cout << "\n=== 日常實務: writeAll to stdout ===\n";
    writeAll(1, std::string("(via write syscall)\n"));

    std::cout << "\n=== 日常實務: recv into string buffer ===\n";
    std::string buf;
    const char* net = "HELLO_NET";
    size_t n = fakeRecvIntoString(buf, net, 9);
    std::cout << "recv " << n << " bytes: " << buf << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：const data() 與 non-const data() 的差別?何時用哪個?
    //    A：const data() (C++03 起) 回傳 const char*,只能讀。non-const
    //       data() (C++17 起) 回傳 char*,可寫入內部 buffer。寫入時不可改
    //       到 size() 位置的 '\0' (會破壞 string 不變式)。對接 read(fd, p, n)
    //       這類 OS API 必選 non-const data() (先 resize 到足夠長度)。
    //
    //  Q2：C++11 之前 data() 與 c_str() 有什麼不同?
    //    A：C++03 規格下 data() 不保證 null-terminated,只能配合 size() 當
    //       「buffer + length 對」用,不能直接傳給 strlen / printf("%s")。
    //       c_str() 永遠保證 null-terminated。C++11 起兩者統一,data()[size()]
    //       必須是 '\0',此後完全等價 (除了 const-ness)。
    //
    //  Q3：data() 回傳的指標 lifetime 多長?何時會失效?
    //    A：只要對 string 做任何「可能 reallocate」的修改 (push_back、append、
    //       += 、insert、reserve、resize 超過 capacity、operator= 等) 或讓
    //       string 解構,先前的 data() 指標立刻失效,繼續使用是 UB。在多執
    //       行緒環境另一個執行緒寫入也會失效。實務上不要把 data() 指標保留
    //       超過一個函式的範圍。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra data.cpp -o data

// === 預期輸出 (節錄) ===
// === LeetCode 1773 ===
// 1
// 2
// === 日常實務: recv into string buffer ===
// recv 9 bytes: HELLO_NET
