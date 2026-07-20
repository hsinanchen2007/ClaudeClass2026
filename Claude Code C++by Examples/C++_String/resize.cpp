// =============================================================================
// 檔名: resize.cpp
// 主題: std::string::resize (改變字串長度)
// 參考:
//   - https://en.cppreference.com/cpp/string/basic_string/resize
//   - https://cplusplus.com/reference/string/string/resize/
// =============================================================================
//
// 【函式資訊 Information】
//   void resize(size_type count);                 // (1) 用 '\0' (char()) 填補新位置
//   void resize(size_type count, char ch);        // (2) 用 ch 填補新位置
//   template<class Op>
//   void resize_and_overwrite(size_type n, Op op); // (3) C++23: 先擴大,再讓 callback 寫入
//
// 回傳: void。
// 例外: 若 count > max_size() 丟 std::length_error;若記憶體配置失敗丟 std::bad_alloc。
//
// 【詳細解釋 Explanation】
// resize 是「強制把字串長度設為 count」的工具。它不像 reserve 只動 capacity,
// 而是真的改變 size() — 改完之後 s.size() == count,迭代器範圍 [begin(), end())
// 也會跟著變動。可以想像成「字串就是一個動態陣列,resize 直接把長度欄位改寫」。
//
// 【1. 兩種行為:截短 vs 延長】
// resize(count) 會根據 count 與目前 size() 的大小關係做不同的事:
//   * count == size():什麼都不做。
//   * count <  size():截短(truncate)。把第 count..size()-1 的字元視為「不存在」,
//                    內部把 size 改成 count,並在 buffer[count] 放上 '\0' 終止符,
//                    讓 c_str() 仍然回傳合法 C-string。
//                    底層的 storage 通常「不會釋放」,capacity 維持不變。
//                    若想真的釋放記憶體要再呼叫 shrink_to_fit() 或用
//                    string().swap(s) 的慣用法。
//   * count >  size():延長(extend)。把 size 擴到 count,額外的字元用第二參數
//                    ch 填入(若用 (1) 重載則是 '\0')。若 count 超過目前
//                    capacity(),會先 reallocate(可能搬移整個 buffer),
//                    所有舊的迭代器 / 指標 / 參考都會失效。
//
// 【2. 為什麼預設填 '\0' 而不是空白】
// resize 是底層工具,STL 設計者刻意讓「填什麼」可指定 — 若呼叫者只想預先把字串
// 拉長到某個尺寸(例如要把外部資料 memcpy 進去),用 '\0' 是「最安全的中性值」,
// 而不會被誤認為某種「應該有的內容」。但這也帶來一個陷阱:
//   std::string s; s.resize(10);  // s 內含 10 個 '\0'!
//   std::cout << s.size();        // 10
//   std::cout << s;               // 「印出」可能看起來是空的,但其實含 '\0'
// 實務上把 resize 的結果丟到 cout 通常會誤導人,要記得內含的是真正的 '\0' bytes。
//
// 【3. resize 與 capacity 的關係】
// resize 影響 size(),不一定影響 capacity():
//   * 縮短:capacity 通常不變。
//   * 延長到 ≤ capacity:不需要 realloc,只把後面的字元填上去 → O(增加量)。
//   * 延長到  > capacity:會觸發 grow(通常以 1.5x 或 2x 倍率),先配置新 buffer,
//     拷貝舊資料,釋放舊 buffer。此時所有指標 / 迭代器 / 參考全部失效,
//     成本是 O(count)。
// 因此「先 reserve 再 resize」是一個好習慣 — reserve 預先配到目標 capacity,
// 後續 resize 不會 realloc,搬移成本可預測。
//
// 【4. 與 reserve、assign、clear 的對照】
//   * reserve(n) — 只調整 capacity,size() 不變。
//   * resize(n)  — 強制把 size() 設成 n,可能改 capacity。
//   * assign     — 用新內容覆蓋,size() 變為新內容長度。
//   * clear()    — size() 變 0,capacity 不變。可視為 resize(0)。
//
// 【5. 版本演進與 C++23 resize_and_overwrite】
// C++98 ~ C++20 的 resize 一律會把延長部分初始化(填 '\0' 或 ch)。
// 對於「我準備要 memcpy/呼叫 read 寫入這段 buffer」的情境,這個 zero-init 是浪費
// 的(寫了 '\0' 之後馬上又被覆蓋掉)。C++23 引入了:
//   template<class Op>
//   void resize_and_overwrite(size_type n, Op op);
// 它會先把 size 擴到 n、capacity 至少到 n,但「不初始化」新位置的內容,然後
// 呼叫 op(buffer_ptr, n) 讓使用者自己寫入,op 回傳「實際使用了多少 byte」,
// resize_and_overwrite 再用該回傳值把 size() 縮到正確值。
// 典型用途:讀檔案、socket、syscall 的 buffer:
//   s.resize_and_overwrite(4096, [&](char* p, size_t cap){
//       ssize_t n = ::read(fd, p, cap);
//       return n < 0 ? 0 : static_cast<size_t>(n);
//   });
//
// 【6. 時間複雜度總結】
//   * 縮短:           O(1)            — 只動 size 與 buffer[count] = '\0'
//   * 延長 (不需 realloc): O(count - size) — 寫入新增字元
//   * 延長 (需 realloc):    O(count)        — 拷貝整個 buffer + 寫入新字元
//
// 【概念補充 Concept Deep Dive】
// (A) Exception safety
//   resize 提供 "strong exception guarantee":若延長過程中需要 realloc 但
//   配置失敗(bad_alloc),原 string 內容保持完整,size、capacity 都不變。
//   截短永遠不會丟例外(走的是 noexcept-ish 的內部路徑)。
//
// (B) reserve + resize 的搭配
//   若已知最終長度,標準寫法是:
//     std::string buf;
//     buf.reserve(n);   // 一次配到位
//     buf.resize(n);    // 把 size 拉到 n,不再 realloc
//   或更積極:
//     std::string buf(n, '\0');   // ctor 直接配並填
//   兩者效果類似,前者多了「先檢查/分階段填入」的彈性。
//
// (C) 釋放多餘 capacity 的手法
//   若 resize 縮短後想真的釋放記憶體:
//     s.shrink_to_fit();          // 標準作法 (C++11+),非強制行為
//     std::string(s).swap(s);     // 慣用法,強制重新配置
//
// (D) 為什麼不能用 resize 處理 unicode 字元數
//   resize 的單位是 byte (在 std::string 中是 char),不是 unicode code point
//   或字元。對 UTF-8 字串呼叫 resize 可能在多 byte 字元中間切斷,造成損壞。
//   要處理「字元數」概念請改用 std::u32string 或第三方 unicode 函式庫。
//
// 【注意事項 Pay Attention】
// 1. resize 若觸發 realloc,所有舊迭代器 / 指標 / 參考全部失效。
// 2. resize 不能超過 max_size(),否則丟 length_error;這在 64-bit 系統極少觸發,
//    但若不小心傳入 (size_t)-1 之類的負數,會立刻爆掉。
// 3. resize 後字串可能內含 '\0',若再用 strlen / printf("%s") 處理會在 '\0'
//    處被截斷,看起來像「資料消失」。要用 size() / data() + size 為準。
// 4. resize_and_overwrite (C++23) 是效能優化,沒有 zero-init,使用前要確認
//    callback 真的會把所有 byte 寫入(否則會讀到 garbage)。
// 5. 縮短後若不在意 capacity,留著未必是壞事 — 它讓後續 append/resize 不需
//    重新配置,類似 std::vector 的「不縮小」哲學。
//
// =============================================================================

/*
補充筆記：std::string::resize
  - resize 會改變 size；變大時新增字元，變小時截斷尾端。
  - resize 後舊 iterator/reference 可能失效，尤其當容量需要重配時。
  - 要先取得可寫空間再交給 C API 時，resize 和 data 常一起出現，但要維護實際長度。
  - std::string::resize 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】resize
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. resize() 和 reserve() 差在哪?
//     答:resize(n) 真的改變 size()——變長時預設用 '\0' 填補(第二個多載可指定
//     字元),變短時截斷;reserve(n) 只確保 capacity() >= n,size() 完全不變。
//     換句話說 resize 之後 s[n-1] 是合法索引,reserve 之後不是。
//     追問:resize 會使 iterator 失效嗎?→ 只要觸發重新配置就會失效。
//
// 🔥 Q2. 為什麼 resize() 的「填零」在高效能 I/O 情境是浪費?
//     答:典型寫法是先 s.resize(n) 開空間,再把 read()/recv() 的資料寫進
//     s.data()——那些 '\0' 才剛被寫入就立刻被覆蓋,等於白做一次 memset。
//     C++23 的 resize_and_overwrite(n, op) 就是為此而生:直接拿到未初始化的
//     buffer 寫入,再由 op 回報實際長度,省掉這次填零。
//     追問:C++23 之前有替代方案嗎?→ 只能忍受這次填零,或改用 vector<char>。
//
// ⚠️ 陷阱. std::string s; s[0] = 'a'; 合法嗎?
//     答:不合法(UB)。s 是空字串,s[0] 就是 s[s.size()];C++11 起允許「讀取」
//     它並得到 '\0' 的引用,但寫入非 '\0' 的值是 UB。正確做法是先 s.resize(1),
//     或直接用 push_back / +=。
//     為什麼會錯:誤以為 operator[] 會自動長出空間(那是 std::map 的行為)。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>

void demoResize() {
    std::string s = "Hello";

    // 延長 + 填字元
    s.resize(8, '*');
    std::cout << "(1) \"" << s << "\", size=" << s.size() << "\n";  // "Hello***"

    // 縮短
    s.resize(3);
    std::cout << "(2) \"" << s << "\"\n";   // "Hel"

    // 延長 + 預設 '\0'
    s.resize(6);
    std::cout << "(3) size=" << s.size() << " (內含 '\\0')\n";
}

// -----------------------------------------------------------------------------
// 【實務範例】從固定大小的二進位 buffer 載入字串
// 為何用 resize: 我們知道資料長度,resize 直接到目標大小,然後 memcpy 填入。
//                這比 reserve + 多次 += 快得多。
// -----------------------------------------------------------------------------
#include <cstring>
std::string fromBuffer(const char* data, size_t len) {
    std::string s;
    s.resize(len);                  // 一次配到位
    std::memcpy(s.data(), data, len);
    return s;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 344. Reverse String (示範 resize 的最簡用法)
// 題目: 給一個字串,原地反轉。
// 為何用 resize: 雖然這題本身不需要 resize,但我們示範「先 resize 出固定長度
//                的 result buffer,再用 index 填入」的常見模式 — 在競賽 / 系統
//                程式碼中,知道最終長度時這比反覆 push_back 快(沒有 grow 抖動)。
// -----------------------------------------------------------------------------
std::string reverseString(const std::string& s) {
    std::string out;
    out.resize(s.size());                       // 一次配到位,size 立即為 N
    for (size_t i = 0; i < s.size(); ++i) {
        out[i] = s[s.size() - 1 - i];           // 直接以 index 寫入(避開 push_back)
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 482. License Key Formatting
// 題目: 把字串中破折號移除,大寫,從尾端每 K 個分組,中間插破折號。
// 為何用 resize: 在估算最終長度後 resize 並填入,可避免反覆插入。
// -----------------------------------------------------------------------------
#include <cctype>
#include <algorithm>
std::string licenseKeyFormatting(const std::string& s, int k) {
    std::string clean;
    clean.reserve(s.size());
    for (char c : s) {
        if (c == '-') continue;
        clean += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    if (clean.empty()) return "";

    int n = static_cast<int>(clean.size());
    int firstGroup = n % k;
    if (firstGroup == 0) firstGroup = k;
    int dashCount = (n - firstGroup) / k;
    std::string out;
    out.resize(n + dashCount);
    int oi = 0;
    for (int i = 0; i < firstGroup; ++i) out[oi++] = clean[i];
    for (int i = firstGroup; i < n; i += k) {
        out[oi++] = '-';
        for (int j = 0; j < k; ++j) out[oi++] = clean[i + j];
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】用前導零補齊固定長度 (eg "42" -> "00042")
// 為何用 resize: 訂單編號、流水號、檔名前綴 (img_00042.png) 都要固定寬度。
//                先 resize 到目標長度,再從尾端覆寫實際數值。
//                C++20 之後也可用 std::format("{:05}", n),但 resize 寫法
//                依然常見且不需 <format> 支援。
// -----------------------------------------------------------------------------
std::string padLeftZero(int n, size_t width) {
    std::string s = std::to_string(n);
    if (s.size() >= width) return s;
    std::string out;
    out.resize(width, '0');
    // 把 s 拷貝到右側
    s.copy(&out[width - s.size()], s.size());
    return out;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 3】LeetCode 1page. 1page0. Truncate Sentence
// 題目: LeetCode 1816. Truncate Sentence
// 為何用 resize: 找到第 k 個空白位置後,直接 resize 把後面截斷;比 substr 更明確。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
std::string truncateSentenceResize(std::string s, int k) {
    int spaces = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == ' ' && ++spaces == k) {
            s.resize(i);              // 截斷到第 k 個空白前
            return s;
        }
    }
    return s;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】用 resize 預先擴大 buffer 給 C API 寫入,完成後 truncate
// 為何用 resize: 系統呼叫 (例如 readlink, getcwd) 需要 char buffer,先 resize 給足,
//                呼叫完成後再 resize 為實際長度。常用慣用法。
// -----------------------------------------------------------------------------
#include <cstring>
std::string fakeReadlink(const char* target) {
    std::string buf;
    buf.resize(256);                  // 預配置 256 byte buffer
    // 模擬 readlink 寫入 (這裡用 strcpy 模擬)
    size_t n = std::min(std::strlen(target), buf.size());
    std::memcpy(buf.data(), target, n);
    buf.resize(n);                    // 截斷到實際長度
    return buf;
}

int main() {
    demoResize();

    std::cout << "\n=== fromBuffer ===\n";
    char data[] = {'a', 'b', 'c'};
    std::cout << fromBuffer(data, 3) << "\n";

    std::cout << "\n=== LeetCode 344 ===\n";
    std::cout << reverseString("hello") << "\n";   // "olleh"

    std::cout << "\n=== LeetCode 482 ===\n";
    std::cout << licenseKeyFormatting("5F3Z-2e-9-w", 4) << "\n";  // 5F3Z-2E9W
    std::cout << licenseKeyFormatting("2-5g-3-J", 2)    << "\n";  // 2-5G-3J

    std::cout << "\n=== LeetCode 1816 (resize 版) ===\n";
    std::cout << "[" << truncateSentenceResize("Hello how are you Contestant", 4) << "]\n";

    std::cout << "\n=== 日常實務: 前導零 ===\n";
    std::cout << "img_" << padLeftZero(42, 5)    << ".png\n";   // img_00042.png
    std::cout << "ord_" << padLeftZero(12345, 6) << "\n";        // ord_012345

    std::cout << "\n=== 日常實務: fakeReadlink ===\n";
    auto r = fakeReadlink("/usr/local/bin");
    std::cout << "[" << r << "] size=" << r.size() << "\n";

    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra resize.cpp -o resize

// === 預期輸出 (節錄) ===
// === LeetCode 1816 (resize 版) ===
// [Hello how are you]
// === 日常實務: fakeReadlink ===
// [/usr/local/bin] size=14
