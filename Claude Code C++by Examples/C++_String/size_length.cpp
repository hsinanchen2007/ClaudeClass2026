// =============================================================================
// 檔名: size_length.cpp
// 主題: std::string::size / length (取得字串長度)
// 參考: https://en.cppreference.com/cpp/string/basic_string/size
//       https://en.cppreference.com/cpp/string/basic_string/length
//       https://cplusplus.com/reference/string/string/size/
//       https://cplusplus.com/reference/string/string/length/
// =============================================================================
//
// 【函式資訊 Information】
//   size_type size()   const noexcept;
//   size_type length() const noexcept;     // 完全等同 size()
//
// size_type 通常是 std::size_t (unsigned 型別)。
//
// 【詳細解釋 Explanation】
//
// (1) 雙名稱的歷史背景:STL 通用 vs 字串傳統
//   C++ 標準庫有兩套思考:
//     - 容器派 (vector, list, map):提供 size()
//     - 字串派 (繼承 C 與其他語言慣例):字串叫 length()
//   為了同時符合兩種習慣,std::basic_string 同時提供兩個名稱,
//   而且行為「完全相同」(同一份程式碼、同樣 noexcept、同樣複雜度)。
//   這也是 std::basic_string 是少數同時繼承「容器」與「字串型別」雙重身分
//   的型別。
//
// (2) 為什麼長度可以 O(1)?
//   C 字串 strlen 是 O(N),因為 char* 沒有「長度欄位」,得自己走到 '\0'。
//   std::string 在物件內部明確存了 _size,所以 size() 只是 return _size;
//   一個記憶體載入指令,O(1) 且 noexcept。
//   這個差異是 std::string 比 char* 在現代程式碼中更受歡迎的關鍵原因之一 —
//   迴圈條件 i < s.size() 不會像 i < strlen(s) 那樣每次重算長度。
//
// (3) bytes vs characters 的迷思
//   std::string 是 byte container,size() 回傳的是「byte 數」,不是
//   「人類可見的字元數」。對 ASCII 字串兩者相同,但對 UTF-8 字串:
//     std::string s = "你好";        // 6 bytes (在 UTF-8 環境)
//     s.size()  == 6
//     人眼字元數 == 2
//   要算「真正的字元數 / grapheme 數」需要 ICU、libutf8proc 等函式庫;
//   標準函式庫沒有提供。size() 永遠是 byte 數,務必牢記。
//
// (4) 何時用 size、何時用 length?
//   - 寫泛型程式碼 (template) → 用 size(),所有容器都有
//   - 強調「字元長度」的語意 → 用 length(),語意清楚
//   - 個人 / 團隊習慣統一 → 任選一個都好
//   兩者編譯後完全一樣,沒有效能差異。
//
// (5) signed / unsigned 陷阱
//   size() 回傳 size_t (unsigned)。寫 for 迴圈時:
//     for (int i = 0; i < s.size(); ++i)         // 有 signed/unsigned 警告
//     for (size_t i = 0; i < s.size(); ++i)      // OK
//     for (auto i = 0u; i < s.size(); ++i)       // OK
//     for (size_t i = s.size() - 1; i >= 0; --i) // BUG! 永不結束 (i 是 unsigned)
//   反向走訪請改用 reverse_iterator 或 ssize() (C++20)。
//   C++20 加入 std::ssize() 自由函式,回傳 signed,專門解這個問題。
//
// (6) 歷史背景
//   - C++98: 兩個函式都已存在
//   - C++11: 加上 noexcept、回傳型別正名為 size_type
//   - C++20: std::ssize(s) 補足 signed 版本
//
// 【注意事項 Pay Attention】
// 1. 回傳型別是 size_type (通常是 size_t — unsigned 型別)。
//    寫 `for (int i = 0; i < s.size(); ++i)` 會有 signed/unsigned 比較警告。
//    建議用 `size_t`、`auto`、或範圍 for。
// 2. 含內嵌 '\0' 時,size() 仍是真實長度,但 strlen(s.c_str()) 會被截斷。
// 3. 與 std::strlen 不同:strlen 是 O(N),size 是 O(1)。
// 4. size() 不含結尾的 '\0' (那是內部 buffer 才有,不算字串內容)。
// 5. 對 UTF-8 字串,size() 是 bytes 數,不是字元數。
// 6. size() 在 C++11 之後保證 noexcept,可安全用於 destructor 與
//    move 操作中。
//
// 【概念補充 Concept Deep Dive】
//
// ★ size 是「冗餘」嗎?為何不每次走訪算長度?
//   C 字串 char* 沒有獨立的長度欄位,因此每次 strlen 都得走訪;這在
//   迴圈中是常見的效能殺手 (O(N²) 的循環模式)。
//   std::string 把 size 視為「結構性元資料」,維護的成本只在「有寫入時
//   更新」,讀取永遠 O(1)。這是封裝 (encapsulation) 帶來的時間複雜度收益。
//
// ★ size_type 是什麼?
//   std::basic_string<CharT, Traits, Allocator> 的 size_type 等於
//   std::allocator_traits<Allocator>::size_type,在預設 allocator 下
//   就是 std::size_t。寫泛型程式時應使用 std::string::size_type 而非
//   寫死 size_t,以支援自訂 allocator。
//
// ★ size() == 0 vs empty():請永遠用 empty()
//   雖然兩者結果相同、效能相同,但 empty() 語意更清楚,且 [[nodiscard]]
//   會幫你抓常見誤用 (寫 s.empty(); 想要清空)。
//
// ★ length() vs std::strlen() 的時機
//   - 已是 std::string → length()/size() (O(1))
//   - 拿到 char* → strlen() (O(N))
//   經常踩雷:把 std::string 轉 c_str() 再 strlen,白白多一個 O(N)。
//
// ★ 實際儲存的位元組
//   雖然 size() 不含 '\0',但 string 內部 buffer 必定多預留一個位元組給 '\0',
//   所以實際耗用記憶體 = capacity() + 1 (再加上 SSO buffer 與 metadata)。
//
// =============================================================================

/*
補充筆記：std::string::size_length
  - std::string::size_length 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::size_length 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::string::size / length
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. size() 和 length() 有什麼差別?
//     答:完全沒有差別 —— 兩者是同義函式,同樣的回傳值、同樣 noexcept、
//         同樣 O(1)。並存純粹是歷史與概念的雙重身分:length() 呼應字串
//         傳統(其他語言、C 的 strlen 語意),size() 呼應 Container 概念
//         (vector/list/map 都叫 size)。寫泛型程式時用 size() 較一致。
//     追問:為什麼可以 O(1),strlen 卻是 O(n)?→ std::string 物件內部
//         明確存了長度欄位,size() 只是讀一個成員;char* 沒有長度資訊,
//         strlen 必須一路掃到 '\0'。
//
// 🔥 Q2. 為什麼不該寫 if (s.size() - 1 >= 0) 或 for (int i = s.size() - 1; i >= 0; --i)?
//     答:size() 回傳的是 size_type(無號,通常是 std::size_t)。無號數
//         永遠 >= 0,所以第一個判斷恆真;空字串時 s.size() - 1 更會
//         wrap around 成巨大值而非 -1。第二個寫法把它存進 int 雖然
//         迴圈能停,卻在超大字串上有截斷風險。
//     追問:同源的經典陷阱還有哪個?→ find() 找不到回傳 npos
//         (= size_type(-1)),所以只能寫 != std::string::npos,
//         不能寫 >= 0 或存進 int 再比 -1。
//
// ⚠️ 陷阱. std::string s = "中文"; s.size() 是多少?
//     答:在 UTF-8 原始碼下是 6,不是 2。size()/length() 回傳的是
//         「byte 數」,不是人眼看到的字元數 —— 每個常用漢字在 UTF-8 下
//         佔 3 bytes。同理 s[0] 拿到的是第一個 byte(不完整的字元),
//         把它印出來會是亂碼。
//     為什麼會錯:直覺把 std::string 當成「字元序列」,但它其實是
//         「byte 序列」,標準庫完全不理解編碼。要算真正的字元數必須自己
//         解析 UTF-8 leading byte,或改用 ICU 之類的函式庫;標準庫沒有
//         現成方案。反轉字串、截斷字串時踩到這點會直接產生破碎的編碼。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>

void demoSizeLength() {
    std::string s = "Hello";
    std::cout << "size()   = " << s.size()   << "\n";   // 5
    std::cout << "length() = " << s.length() << "\n";   // 5

    // 內嵌 '\0' 的情況
    std::string t("ab\0cd", 5);
    std::cout << "t.size() = " << t.size() << "\n";              // 5
    std::cout << "strlen   = " << std::strlen(t.c_str()) << "\n";// 2 (在第一個 \0 截斷)

    // UTF-8: 底下的 size 是 bytes
    std::string utf8 = "\xE4\xBD\xA0\xE5\xA5\xBD"; // "你好" UTF-8 編碼
    std::cout << "utf8.size() = " << utf8.size() << " (bytes, 非字元數)\n"; // 6
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 58. Length of Last Word
// 題目: 給字串(可能含尾隨空白),回傳最後一個單字的長度。
// 為何用 size: 從尾端反掃需要知道字串長度作為起點;此題核心就是
//              「以 size() 為起點往左找」的經典模式。
// 解題思路: 跳過尾端空白,再連續往前算非空白字元的長度。
// 複雜度: 時間 O(n)、空間 O(1)。
// -----------------------------------------------------------------------------
int lengthOfLastWord(const std::string& s) {
    int i = static_cast<int>(s.size()) - 1;
    // 跳過尾部空白
    while (i >= 0 && s[i] == ' ') --i;
    int end = i;
    while (i >= 0 && s[i] != ' ') --i;
    return end - i;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】文字進度條
// 為何用 size: 計算「已完成」字元的數量、總長度,生成終端機進度條。
//              CLI 工具、ETL 任務、批次腳本經常需要視覺化進度。
//              用 size() 做百分比換算、用 append 配合長度填字元,
//              是日常 ops 工具的標準寫法。
// -----------------------------------------------------------------------------
std::string progressBar(int done, int total, int width = 20) {
    if (total <= 0) return "[]";
    int filled = static_cast<int>(static_cast<double>(done) / total * width);
    if (filled > width) filled = width;
    std::string bar = "[";
    bar.append(filled, '#');
    bar.append(width - filled, '-');
    bar += "] ";
    bar += std::to_string(done) + "/" + std::to_string(total);
    return bar;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1page. 1page0. Maximum Number of Balloons
// 題目: 給字串 text,看能組合幾個 "balloon" (字母多次取出)。
// 為何用 size: 用 cnt['b']..cnt['n'] 後,balloon 含 2 個 'l'/'o',
//              所以 min(cnt['l']/2, cnt['o']/2, cnt['b'], cnt['a'], cnt['n']) 即答案。
//              size 用於確認 "balloon" 字串長度。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
int maxNumberOfBalloons(const std::string& text) {
    int cnt[128] = {0};
    for (unsigned char c : text) ++cnt[c];
    const std::string word = "balloon";
    (void)word.size();    // 教學:確認 word.size() == 7
    int b = cnt['b'], a = cnt['a'], l = cnt['l'] / 2, o = cnt['o'] / 2, n = cnt['n'];
    return std::min({b, a, l, o, n});
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】將位元組大小格式化為「N bytes / N.NN KB / N.NN MB」
// 為何用 size: 對輸入做明確的型別/長度判斷;這裡 size 是字串本身的長度檢查。
// (主要展示字串長度與 to_string 配合的真實情境)
// -----------------------------------------------------------------------------
std::string sizeOfText(const std::string& s) {
    return std::to_string(s.size()) + " bytes";
}

int main() {
    demoSizeLength();
    std::cout << "\n=== LeetCode 58 ===\n";
    std::cout << lengthOfLastWord("Hello World") << "\n";        // 5
    std::cout << lengthOfLastWord("   fly me   to   the moon  ") << "\n"; // 4

    std::cout << "\n=== LeetCode 1189 ===\n";
    std::cout << maxNumberOfBalloons("nlaebolko")          << "\n";   // 1
    std::cout << maxNumberOfBalloons("loonbalxballpoon")   << "\n";   // 2

    std::cout << "\n=== 日常實務: 進度條 ===\n";
    std::cout << progressBar(0,  10) << "\n";
    std::cout << progressBar(3,  10) << "\n";
    std::cout << progressBar(10, 10) << "\n";

    std::cout << "\n=== 日常實務: sizeOfText ===\n";
    std::cout << sizeOfText("Hello, World!") << "\n";   // 13 bytes
    std::cout << sizeOfText("你好") << "\n";           // 6 bytes (UTF-8)

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:size() 與 length() 真的「完全一樣」嗎?編譯後產生不同程式碼嗎?
    //    A:語意完全相同 — 標準明訂「length() shall return size()」,
    //      回傳值、noexcept、複雜度全部一致。多數實作 length() 直接 inline
    //      呼叫 size(),最佳化後產生相同 assembly。選哪個純看可讀性偏好。
    //
    //  Q2:為什麼要回傳 size_type (unsigned),不是 int?反向迴圈怎麼寫安全?
    //    A:size_type 是配合 allocator 設計的 unsigned 整數 (預設 size_t),
    //      可表示最大記憶體範圍。寫反向迴圈時不要用 for(size_t i=size()-1;
    //      i>=0; --i),i 是 unsigned,>=0 永真會無窮迴圈。改用
    //      reverse_iterator 或 C++20 的 std::ssize() (回傳 signed)。
    //
    //  Q3:size() 對 UTF-8 字串 "你好" 回傳什麼?如何取真實字元數?
    //    A:回傳 byte 數,通常是 6 (每個 CJK 字元 3 bytes)。標準函式庫沒有
    //      內建「grapheme 計數」工具。要算人眼字元數需用 ICU、libutf8proc
    //      或自行掃描 UTF-8 lead bytes。實務上 size() 只當「byte 數」用,
    //      不要當「字元數」。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra size_length.cpp -o size_length

// === 預期輸出 (節錄) ===
// === LeetCode 1189 ===
// 1
// 2
// === 日常實務: sizeOfText ===
// 13 bytes
// 6 bytes
