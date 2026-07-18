// =============================================================================
// 檔名: max_size.cpp
// 主題: std::string::max_size (取得字串理論最大長度)
// 參考: https://en.cppreference.com/cpp/string/basic_string/max_size
//       https://cplusplus.com/reference/string/string/max_size/
// =============================================================================
//
// 【函式資訊 Information】
//   size_type max_size() const noexcept;
//
// 【詳細解釋 Explanation】
//
// (1) 設計理念:讓泛型程式碼能查詢容器極限
//   STL 設計時就規定:所有容器都得提供 max_size(),作為「在這個型別與當前
//   allocator / 平台下,最多能放多少元素」的查詢介面。這對下面這類場景
//   有意義:
//     - 寫泛型函式時想 assert N <= container.max_size()
//     - 序列化 / 反序列化:讀到「需要配置 N 個」時做 sanity check
//     - 動態決定演算法策略 (大數運算、字典樹等)
//   注意:這是「理論上限」,不是「保證能成功配置這麼多」 — 實際還受可用
//   實體記憶體與 OS 限制。
//
// (2) 數值由什麼決定?
//   max_size() 通常是這幾個值取最小:
//     a. allocator_traits<Alloc>::max_size(alloc) — allocator 容許值
//     b. std::numeric_limits<difference_type>::max() — iterator 距離型別上限
//     c. std::numeric_limits<size_type>::max() / sizeof(CharT) — 位址空間
//     d. 實作內部 metadata 欄位的位元寬度限制
//   實際數字在 64-bit Linux + libstdc++ 上,std::string::max_size() 大約是
//   2^61 - 1 (差不多是 2.3 EB);libc++ 與 MSVC 略有差異。
//
// (3) 不同平台 / 實作的數值差異
//   - 32-bit 系統:最多約 2^31 - 1 ≈ 2 GB,實際上更小 (受 size_type 限制)
//   - 64-bit Linux + libstdc++:約 2^61 - 1 (~ 2.3 EB)
//   - 64-bit libc++ (macOS):類似但不同
//   - 64-bit MSVC:約 2^31 (最多 4 GB? 視版本)
//   結論:絕對不要把這數字寫死在程式中。
//
// (4) 與 capacity / size 的層級關係
//     size()      ≤ capacity() ≤ max_size()
//   - size():當前已用
//   - capacity():已配置但可能未用
//   - max_size():理論上限,通常永遠不會碰到
//   超過 max_size 的 reserve / resize / append 會丟 std::length_error。
//
// (5) 使用情境少,但仍重要
//   絕大多數應用永遠用不到 max_size()。它的存在是讓:
//     - 標準函式庫內部 (例如 reserve 檢查 N <= max_size)
//     - 防禦性程式設計 (parser / protocol handler 收到 length 欄位後檢查)
//     - 教學 / 平台分析
//   有一個明確的查詢介面。
//
// (6) 歷史背景
//   - C++98: 已存在,但語意較模糊
//   - C++11: 加上 noexcept、規範與 allocator_traits 連動
//   - 後續版本沒有大改變
//
// (7) 複雜度
//   O(1)、noexcept、不會配置記憶體。
//
// 【注意事項 Pay Attention】
// 1. max_size() 與 capacity() 不同:
//      - capacity() : 目前已配置的可用空間 (不需重新配置就可使用的長度)
//      - max_size() : 理論上限,通常永遠不會碰到
// 2. 試圖 resize / reserve 超過 max_size() 會丟出 std::length_error。
// 3. 不同 STL 實作 (libstdc++、libc++、MSVC) 給出的數字不同,別把它寫死。
// 4. 實務上幾乎不會用到 max_size,僅在寫泛型工具或想印出系統極限時參考。
// 5. max_size() 不保證「能配置這麼多」 — 系統實際記憶體不夠時 reserve
//    仍會丟 std::bad_alloc。
//
// 【概念補充 Concept Deep Dive】
//
// ★ 為什麼 max_size() 不是 std::numeric_limits<size_t>::max()?
//   因為 string 必須能用 difference_type 表達「end - begin」這個距離,
//   而 difference_type 是有號型別,範圍只有 size_type 的一半。所以
//   max_size() 至少要 ≤ ptrdiff_t 的最大值,才能保證所有 iterator
//   減法都不溢位。
//
// ★ length_error vs bad_alloc 的差異
//   - length_error:邏輯錯誤 — 你要求的長度超過容器型別所能表達
//   - bad_alloc:資源錯誤 — 系統當前記憶體不足
//   兩者都繼承自 std::exception,但分屬 <stdexcept> 與 <new>。
//   設計防禦時兩者都該 catch,給使用者明確的錯誤訊息。
//
// ★ 為什麼 max_size() 是 const 成員,而不是 static?
//   因為某些 allocator 可以動態決定 max_size (例如 pool allocator
//   設定不同上限)。標準把它定義為 const 成員以保留這份彈性。
//   實務上對預設 allocator 的 std::string,max_size() 對所有實例都相同。
//
// ★ 該不該主動驗證 input <= max_size?
//   寫安全程式碼時推薦,例如解析網路協定的 length 欄位:
//     if (len > buffer.max_size()) return ErrorCode::TooLarge;
//   這比直接 reserve(len) 觸發 length_error 再 catch 更明確。
//
// =============================================================================

/*
補充筆記：std::string::max_size
  - std::string::max_size 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::max_size 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
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
#include <stdexcept>
#include <vector>
#include <algorithm>

void demoMaxSize() {
    std::string s;
    std::cout << "max_size() = " << s.max_size() << "\n";

    // 嘗試超過 max_size() 會丟出 length_error
    try {
        s.reserve(s.max_size() + 1);   // 多數實作會丟例外
    } catch (const std::length_error& e) {
        std::cout << "length_error: " << e.what() << "\n";
    } catch (const std::bad_alloc& e) {
        std::cout << "bad_alloc: "    << e.what() << "\n";
    }
}

// -----------------------------------------------------------------------------
// 【實務範例】列印系統的 string 極限
// 為何用 max_size: 寫 portable 程式時,有時要根據系統能力決定演算法策略
//                  (例如 string-based 大數運算的最大位數)。
// -----------------------------------------------------------------------------
void printLimits() {
    std::string s;
    std::cout << "本系統 std::string max_size = " << s.max_size()  << " 字元\n";
    std::cout << "預設 capacity              = " << s.capacity() << "\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】配置前的安全檢查
// 為何用 max_size: 從外部來源(網路、檔案)收到「需要配置 N bytes」時,
//                  先驗證 N <= max_size 才不會觸發 length_error 或耗光記憶體。
//                  在處理不可信輸入(parsers、protocol handlers)是關鍵防線。
//                  例如解析 HTTP Content-Length、protobuf length-delimited、
//                  自訂二進位協定都必須做這道檢查。
// -----------------------------------------------------------------------------
bool canSafelyAllocate(size_t requested) {
    std::string probe;
    return requested <= probe.max_size();
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 459. Repeated Substring Pattern (Easy)
//
// 題目敘述:
//   給字串 s,判斷它是否能由它的某個 substring 重複多次組成。
//   範例:
//     "abab"        → true   (由 "ab" 重複 2 次)
//     "aba"         → false
//     "abcabcabcabc"→ true   (由 "abc" 重複 4 次)
//
// 為何用 max_size:
//   經典解法是「s + s 後去頭尾各一字元,看是否仍包含 s」。
//   這個解法會建出長度為 2 * s.size() 的中間字串。
//   若 s 長度極大(惡意輸入或 fuzz 測試的長度欄位),2 * s.size() 可能
//   溢位或超過 max_size,導致 length_error / OOM 或位址環繞。
//   面對「來自外部、長度可能異常的字串」(如解析自封包、檔案、user input),
//   先用 max_size 做防禦性檢查是現代後端必備紀律 — 直接報錯比 crash 好。
//
// 解題思路:
//   1. 防禦性檢查: 若 2 * s.size() > max_size,直接回 false (拒絕處理)。
//   2. doubled = s + s,trimmed = doubled.substr(1, doubled.size() - 2)。
//   3. 若 trimmed 含 s 為子字串 → s 為某 substring 的重複。
//
// 為什麼 (s+s).substr(1, 2n-2) 包含 s 等價於可由子字串重複組成?
//   假設 s = t * k (t 重複 k 次),則 s + s = t * 2k。在 doubled 內部,
//   把第 1 個字元與最後 1 個字元去掉後,中間還是會看到 t * (2k - 2/|t|)
//   個 t,若 k >= 2 (即真的「重複」),s 仍是子字串。
//   若 s 不可由更短子字串重複組成,則 doubled 中 s 只在 offset 0 與 n
//   出現,被去頭尾後恰好都消失。
//
// 複雜度:時間 O(n) (find / strstr 通常為 KMP / Boyer-Moore),空間 O(n)。
// -----------------------------------------------------------------------------
bool repeatedSubstringPattern(const std::string& s) {
    // 防禦性: 確保 2 * s.size() 不會超過 string max_size
    std::string probe;
    if (s.size() > probe.max_size() / 2) {
        // 輸入超大,不處理 (避免 length_error / OOM)
        return false;
    }
    if (s.size() < 2) return false;

    std::string doubled = s + s;
    std::string trimmed = doubled.substr(1, doubled.size() - 2);
    return trimmed.find(s) != std::string::npos;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1page. 1page0. Add Strings - 大數防護版
// 題目: LeetCode 415. Add Strings
// 兩個用字串表示的非負大整數相加,結果也是字串。
// 為何用 max_size: 對潛在「超長」輸入先檢查 max_size,防止配置失敗。
//                  教學「在配置前先 sanity-check 長度」的防禦性寫法。
// 複雜度: O(max(N,M))。
// -----------------------------------------------------------------------------
std::string addStringsBig(const std::string& a, const std::string& b) {
    std::string res;
    size_t need = std::max(a.size(), b.size()) + 1;
    if (need > res.max_size()) return "(too big)";
    res.reserve(need);
    int i = static_cast<int>(a.size()) - 1;
    int j = static_cast<int>(b.size()) - 1;
    int carry = 0;
    while (i >= 0 || j >= 0 || carry) {
        int x = (i >= 0) ? a[i--] - '0' : 0;
        int y = (j >= 0) ? b[j--] - '0' : 0;
        int sum = x + y + carry;
        res.push_back(static_cast<char>('0' + sum % 10));
        carry = sum / 10;
    }
    std::reverse(res.begin(), res.end());
    return res;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】CDN / Proxy 解析 X-Forwarded-For 之長度上限
// 為何用 max_size: 防禦性檢查 — header 可能含上百個 IP,先卡上限避免 DoS。
// -----------------------------------------------------------------------------
bool isHeaderLengthOk(const std::string& header, size_t maxBytes = 8 * 1024) {
    // 兩重檢查: 程式限制 (8 KB) 與型別理論限制 (max_size)
    return header.size() <= maxBytes && header.size() <= header.max_size();
}

int main() {
    demoMaxSize();
    std::cout << "\n=== 系統極限 ===\n";
    printLimits();

    std::cout << "\n=== 日常實務: 配置前驗證 ===\n";
    std::cout << "100 bytes? " << std::boolalpha << canSafelyAllocate(100) << "\n";
    std::cout << "ridiculous (max_size + 100)? "
              << canSafelyAllocate(std::string().max_size() + 100) << "\n";

    std::cout << "\n=== LeetCode 459 (帶 max_size 防禦) ===\n";
    std::cout << "\"abab\"         → " << repeatedSubstringPattern("abab")          << "\n"; // 1
    std::cout << "\"aba\"          → " << repeatedSubstringPattern("aba")           << "\n"; // 0
    std::cout << "\"abcabcabcabc\" → " << repeatedSubstringPattern("abcabcabcabc")  << "\n"; // 1

    std::cout << "\n=== LeetCode 415 (addStringsBig) ===\n";
    std::cout << addStringsBig("11", "123")  << "\n";   // 134
    std::cout << addStringsBig("456", "77")  << "\n";   // 533

    std::cout << "\n=== 日常實務: header 長度防禦 ===\n";
    std::string h(100, 'X');
    std::cout << "100 bytes OK? " << isHeaderLengthOk(h) << "\n";   // true
    std::cout << "10K bytes OK? " << isHeaderLengthOk(std::string(10000, 'X')) << "\n"; // false

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:max_size() 跟 capacity() 有什麼層級關係?
    //    A:size() ≤ capacity() ≤ max_size()。capacity 是「目前已配置可用」,
    //       max_size 是「型別 + allocator 容許的理論上限」。實際上限可能更小
    //       (受實體記憶體 / OS 限制) — max_size 不保證真能配置這麼多。
    //
    //  Q2:超過 max_size() 的 reserve / resize 會丟什麼例外?跟 bad_alloc 差別?
    //    A:丟 std::length_error (邏輯錯誤,長度超出型別表達範圍),不是 bad_alloc
    //       (資源錯誤,系統暫時記憶體不足)。前者在 <stdexcept>、後者在 <new>,
    //       防禦性程式碼兩者都要 catch 給出明確訊息。
    //
    //  Q3:為什麼 max_size() 在 64-bit Linux 大約是 2^61 - 1 而不是 2^64 - 1?
    //    A:string 的 difference_type (= ptrdiff_t) 是有號型別,範圍只有 size_type
    //       的一半;為了讓所有 iterator 減法不溢位,max_size 必 ≤ ptrdiff_t::max。
    //       libstdc++ 又預留位元給 SSO flag 等 metadata,所以又少幾位。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra max_size.cpp -o max_size

// === 預期輸出 (節錄) ===
// === LeetCode 415 (addStringsBig) ===
// 134
// 533
// === 日常實務: header 長度防禦 ===
// 100 bytes OK? true
// 10K bytes OK? false
