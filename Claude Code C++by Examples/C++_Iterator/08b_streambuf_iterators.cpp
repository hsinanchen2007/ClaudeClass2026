// =============================================================================
//  08b_streambuf_iterators.cpp  —  istreambuf_iterator / ostreambuf_iterator
// =============================================================================
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼這個 iterator 存在?                              │
//  └────────────────────────────────────────────────────────────┘
//
//  C++ I/O 系統有兩層:
//
//      [使用者層] std::istream / std::ostream         ← 高階,負責「格式化」
//                  ▲ operator>>、operator<<            (跳空白、解析數字、寬度等)
//                  │
//      [低階層 ] std::streambuf                         ← 低階,純粹「字元緩衝」
//                  ▲ sgetc / sbumpc / sputc            (一個 byte 進、一個 byte 出)
//                  │
//      [作業系統] read() / write() syscall             ← 底層 OS I/O
//
//  istream_iterator 走的是「高階格式化讀」:
//      * 會跳過空白與換行
//      * 會解析 token (例如把 "42" 變 int)
//      * 內部用 operator>> 讀
//
//  istreambuf_iterator 走的是「低階純字元讀」:
//      * 一個字元都不漏 (含空白、換行、Tab)
//      * 不解析、不跳東西
//      * 內部用 streambuf::sgetc / sbumpc 讀
//      * 比 istream_iterator 快 — 少了 operator>> 的解析、locale 處理、
//        sentinel object 建構等開銷
//
//  「想把整個檔案吃成 string」、「想把 stream 整段轉到另一個 stream」
//  ── 這類「不在乎內容是什麼,只是搬 byte」的場景,就該用 streambuf 版。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、類別與底層機制                                         │
//  └────────────────────────────────────────────────────────────┘
//
//   * std::istreambuf_iterator<CharT>   是 InputIterator
//      - 內部存一個 std::basic_streambuf<CharT>* 指標
//      - operator*() 呼叫 sbuf->sgetc()  (peek,不前進)
//      - operator++() 呼叫 sbuf->sbumpc() (取出並前進)
//      - 預設建構 (空參數) 代表「end-of-stream」
//      - 比較相等:兩邊都是 EOF 時相等,否則不相等
//
//   * std::ostreambuf_iterator<CharT>   是 OutputIterator
//      - 內部存 streambuf*
//      - *it = c 等同 sbuf->sputc(c)
//      - ++ 是 no-op (跟所有 OutputIterator 一樣)
//
//  CharT 通常是 char (即 char/byte stream),也可以 wchar_t (寬字元)。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、與 istream_iterator 的差別 (一定要搞清楚)              │
//  └────────────────────────────────────────────────────────────┘
//
//      stream:   "hello world\n42"
//
//      istream_iterator<char>     → 'h','e','l','l','o','w','o','r','l','d','4','2'
//                                   (跳掉空白與換行!)
//      istreambuf_iterator<char>  → 'h','e','l','l','o',' ','w','o','r','l','d',
//                                   '\n','4','2'   ← 一個字元都不漏
//
//  一句話總結:
//    * 要「格式化解析」 → istream_iterator
//    * 要「原原本本搬字元」 → istreambuf_iterator
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、最常見用途                                             │
//  └────────────────────────────────────────────────────────────┘
//
//   1. 把整個 stream / 整個檔案吃成 std::string,一行解決:
//        std::ifstream f("a.txt");
//        std::string s{ std::istreambuf_iterator<char>(f), {} };
//      ── 這是讀檔最 idiomatic 的 C++ 寫法之一。
//
//   2. 把一個 stream 整個搬到另一個 stream (file copy / dump):
//        std::copy(std::istreambuf_iterator<char>(in), {},
//                  std::ostreambuf_iterator<char>(out));
//      ── 等價於 out << in.rdbuf(),但這是 iterator 版。
//
//   3. Hash / Checksum 整個檔案:直接餵進 std::accumulate,
//      用 lambda 維護累積 hash (見實務範例)。
//
//   4. 字元頻率統計 (例如 LeetCode 387):邊讀邊累加,
//      不必先讀進整個 string。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 五、何時不要用                                             │
//  └────────────────────────────────────────────────────────────┘
//
//   * 你想讀「一個個整數 / 浮點數」 → 用 istream_iterator<T>,
//     streambuf 版只給你 char,你還要自己解析。
//   * 你需要 random access (跳到第 N 個字元) → streambuf 是 InputIterator,
//     沒有 +n。要 random access 請先讀進 string,再用 string::iterator。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 六、Pitfalls (陷阱)                                        │
//  └────────────────────────────────────────────────────────────┘
//
//   1. Most Vexing Parse:
//        std::string s(std::istreambuf_iterator<char>(in),  // ← 被當作函式宣告!
//                      std::istreambuf_iterator<char>());
//      解法:用 {} 大括號初始化、或多包一層括號:
//        std::string s{ std::istreambuf_iterator<char>(in),
//                       std::istreambuf_iterator<char>{} };
//   2. 它讀的是「字元 (char)」,不是「整數」 — T 不能是 int。
//   3. 雖然是 InputIterator,但「同一個 stream 上有兩個 istreambuf_iterator」
//      會互相干擾 (它們共用底層 streambuf)。要避免複製著用。
//   4. 沒有「ungetc」概念 — ++ 過去就回不來了。要 peek 用 *it (不會推進)。
//   5. 對二進位檔案要 stream 開 std::ios::binary,否則 Windows 上的
//      "\r\n" 換行可能被轉換,讀出的字數會跟檔案大小不一致。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 七、參考連結 (References)                                  │
//  └────────────────────────────────────────────────────────────┘
//
//    https://en.cppreference.com/w/cpp/iterator/istreambuf_iterator   — input
//    https://en.cppreference.com/w/cpp/iterator/ostreambuf_iterator   — output
//    https://en.cppreference.com/w/cpp/io/basic_streambuf               — 底層 streambuf
//    https://cplusplus.com/reference/iterator/istreambuf_iterator/    — 簡明
//    https://cplusplus.com/reference/iterator/ostreambuf_iterator/    — 簡明
// =============================================================================

/*
補充筆記：streambuf_iterators
  - streambuf_iterators 的核心是「位置」與「走訪能力」；iterator 不擁有元素，只是通往容器元素的操作介面。
  - 不同 iterator category 能力不同：input 只能讀一次，forward 可多次走訪，bidirectional 可退後，random access 可做加減與索引。
  - end iterator 是哨兵位置，不能解參考；所有 [begin, end) 半開區間都依賴這個規則。
  - 容器修改可能讓 iterator 失效；vector reallocation、erase、insert 的規則和 list/map 不同，不能通用直覺。
  - insert iterator、stream iterator、move iterator 都是在改變演算法如何讀寫元素，而不是改變演算法本身。
  - 自訂 iterator 要讓 value_type、reference、difference_type、iterator_category 等 traits 能被演算法理解。
  - istreambuf_iterator 逐字元讀取 stream buffer，不做格式化解析，適合複製原始文字內容。
  - ostreambuf_iterator 直接寫字元到 buffer，少了格式化層，通常比 operator<< 單字元輸出更直接。
  - 處理 binary bytes 時仍要注意 char signedness 和檔案開啟模式。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】istreambuf_iterator / ostreambuf_iterator
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. istreambuf_iterator 和 istream_iterator 差在哪？該用哪一個？
//     答：istream_iterator 走高階格式化層（operator>>），會跳過空白、會把 "42" 解析成
//         int；istreambuf_iterator 直接走 streambuf 字元層，一個 byte 都不漏，也不解析。
//         要「原原本本搬字元」就用 streambuf 版，要「解析成型別」才用 istream_iterator。
//     追問：為什麼 streambuf 版比較快？（省掉 operator>> 的解析、locale 處理與 sentinel
//         物件建構）
//
// 🔥 Q2. 怎麼用一行把整個檔案讀進 std::string？
//     答：std::string s{ std::istreambuf_iterator<char>(file), {} };
//         第二個 {} 就是 default constructed 的 end-of-stream sentinel。用 streambuf 版
//         才能保留原檔所有空白與換行。
//     追問：為什麼不用 istream_iterator<char>？（它會把空白與換行全部吃掉）
//
// Q3. istreambuf_iterator 的 operator* 和 operator++ 底層各做什麼？
//     答：operator* 呼叫 sgetc()，只 peek 目前字元、不前進；operator++ 呼叫 sbumpc()，
//         取出字元並前進。它是 InputIterator，同樣是 single-pass。
//         ostreambuf_iterator 則是 OutputIterator：*it = c 等同 sputc(c)，++ 是 no-op。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <iterator>
#include <sstream>
#include <fstream>
#include <string>
#include <algorithm>
#include <numeric>

int main() {
    // -----------------------------------------------------------------------
    // 範例 1:把整個 stream「逐字元」吃成 string (連空白、換行都保留)
    //   這就是讀整個檔案最簡潔的寫法。
    //   注意 {} 列表初始化避開 Most Vexing Parse。
    // -----------------------------------------------------------------------
    std::istringstream iss("hello world\nthis is line 2");
    std::string content{ std::istreambuf_iterator<char>(iss),
                         std::istreambuf_iterator<char>{} };
    std::cout << "content size = " << content.size()
              << " (期望 26,含換行與空白)\n";
    std::cout << "content = [" << content << "]\n";

    // -----------------------------------------------------------------------
    // 範例 2:對比 — istream_iterator<char> 會跳空白
    // -----------------------------------------------------------------------
    std::istringstream iss2("hello world");
    std::string compact{ std::istream_iterator<char>(iss2),
                         std::istream_iterator<char>{} };
    std::cout << "istream_iterator<char> 結果 = [" << compact
              << "]  (空白被跳掉)\n";

    // -----------------------------------------------------------------------
    // 範例 3:ostreambuf_iterator — 把字串「直送」cout,不經 operator<<
    //   等同 std::cout << s,但展示 OutputIterator 介面
    // -----------------------------------------------------------------------
    std::string msg = "[ostreambuf_iterator demo]\n";
    std::copy(msg.begin(), msg.end(),
              std::ostreambuf_iterator<char>(std::cout));

    // -----------------------------------------------------------------------
    // 範例 4:把一個 stream 整個轉到另一個 stream (常用於 file copy / dump)
    //   等價於 dst << src.rdbuf(),但這是 iterator 版,可塞進演算法。
    // -----------------------------------------------------------------------
    std::istringstream src("ABCDE-FGHIJ");
    std::ostringstream dst;
    std::copy(std::istreambuf_iterator<char>(src),
              std::istreambuf_iterator<char>{},
              std::ostreambuf_iterator<char>(dst));
    std::cout << "stream→stream 結果 = " << dst.str() << '\n';

    // -----------------------------------------------------------------------
    // 範例 5:算字元出現次數 (純 byte 走訪 → istreambuf_iterator 最適合)
    // -----------------------------------------------------------------------
    std::istringstream iss3("mississippi");
    auto count_s = std::count(std::istreambuf_iterator<char>(iss3),
                              std::istreambuf_iterator<char>{},
                              's');
    std::cout << "'s' 在 \"mississippi\" 出現 " << count_s
              << " 次 (期望 4)\n";

    // === LeetCode / 實務範例 ===
    void leetcode_387_first_unique_char();
    void leetcode_771_jewels_and_stones();
    void practical_simple_checksum_of_stream();
    void leetcode_1119_remove_vowels();
    void practical_file_byte_count();
    leetcode_387_first_unique_char();
    leetcode_771_jewels_and_stones();
    practical_simple_checksum_of_stream();
    leetcode_1119_remove_vowels();
    practical_file_byte_count();

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:istreambuf_iterator 為什麼比 istream_iterator<char> 快?
    //    A:istream_iterator 走 operator>>,會經過 sentry 物件建構、locale 處理、
    //      whitespace 跳過、num_get / facet 等格式化機制,每讀一字元都付出可觀
    //      overhead。istreambuf_iterator 直接呼叫 streambuf::sgetc / sbumpc,
    //      只做最低階的「拿一個 byte」動作,沒有解析。讀大檔的差異可達數倍。
    //
    //  Q2:讀整個檔案的最 idiomatic 寫法是什麼?
    //    A:std::string s{ std::istreambuf_iterator<char>(file),
    //                      std::istreambuf_iterator<char>{} };
    //      利用 string 的 (InputIt first, InputIt last) ctor,一行讀完整檔。
    //      注意大括號避開 Most Vexing Parse,並且開檔請用 std::ios::binary 避免
    //      Windows 上 "\r\n" 被自動轉換造成 size 不一致。
    //
    //  Q3:同一個 istream 上能不能同時用兩個 istreambuf_iterator?
    //    A:理論上可以建立多個,但它們共用底層 streambuf,任一者 ++ 都會推進
    //      shared cursor,結果就是「兩個 iterator 像在搶同一份資料」。實務上請
    //      把它當「single owner」看待 — 一個 stream 同時間只用一個 iterator,
    //      不要拷貝來拷貝去 (這也跟 InputIterator 的 single-pass 限制吻合)。
    //

    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 387: First Unique Character in a String
// ----------------------------------------------------------------
// 題目:給字串 s,回傳「第一個只出現一次的字元」之索引;若全部都重複回傳 -1。
//      例如 s = "loveleetcode" → 索引 2 (字元 'v')。
//
// 為什麼這題對 streambuf_iterator 適合示範:
//   * 雖然實務上字串直接 string::iterator 即可,
//     但若資料來源是「stream / file」(例如從 socket 傳來的長文字),
//     用 istreambuf_iterator 可以「邊讀邊建頻率表」,不必先全部讀進 string。
//   * 此處示範「來源是 stream」時的寫法 — 工作上常見:
//     log 檔讀到 RAM 前,先掃一次找特殊字元位置。
//
// 解法核心:
//   * 兩遍掃描:第一遍建頻率表 cnt[256] (ASCII),第二遍找第一個 cnt==1 的位置。
//
// 複雜度:時間 O(n);空間 O(1) (頻率表 256 大小固定)。
void leetcode_387_first_unique_char() {
    auto first_uniq_char = [](std::istream& in) -> int {
        std::string s{ std::istreambuf_iterator<char>(in),
                       std::istreambuf_iterator<char>{} };
        int cnt[256] = {0};
        for (unsigned char c : s) ++cnt[c];
        for (int i = 0; i < (int)s.size(); ++i)
            if (cnt[(unsigned char)s[i]] == 1) return i;
        return -1;
    };
    std::istringstream iss4("loveleetcode");
    std::cout << "LC387 first unique idx = " << first_uniq_char(iss4)
              << " (期望 2,字元 'v')\n";
}

// ----------------------------------------------------------------
// LeetCode 771: Jewels and Stones
// ----------------------------------------------------------------
// 題目:給 jewels 字串 (每個字元代表一種「寶石」)、stones 字串 (你擁有的石頭),
//      回傳 stones 中「是寶石」的個數。
//      例:jewels = "aA", stones = "aAAbbbb" → 3 (a 一個 + A 兩個)。
//
// 為什麼這題對 streambuf_iterator 適合示範:
//   * stones 從 stream 讀進來時,用 std::accumulate + istreambuf_iterator
//     一行算完,完全不需要 std::string 暫存。
//   * std::accumulate 只要 InputIterator → istreambuf_iterator 完美對應。
//
// 解法核心:
//   * 預先把 jewels 的字元放進 isJewel[256] 查表 (O(1) 查詢)。
//   * accumulate 走過 stream 每個字元,對是寶石的累加 1。
//
// 複雜度:時間 O(|jewels| + |stones|);空間 O(1) (查表大小固定)。
void leetcode_771_jewels_and_stones() {
    auto num_jewels = [](const std::string& jewels, std::istream& stones) {
        bool isJewel[256] = {false};
        for (unsigned char c : jewels) isJewel[c] = true;
        return std::accumulate(
            std::istreambuf_iterator<char>(stones),
            std::istreambuf_iterator<char>{},
            0,
            [&](int acc, char c){
                return acc + (isJewel[(unsigned char)c] ? 1 : 0);
            });
    };
    std::istringstream stones("aAAbbbb");
    std::cout << "LC771 jewels in stones = " << num_jewels("aA", stones)
              << " (期望 3)\n";
}

// ----------------------------------------------------------------
// 實務範例:對 stream 計算簡易 checksum (XOR sum)
// ----------------------------------------------------------------
// 場景:資料管道收到一段 byte,要快速比對「跟發送端的 checksum 一致嗎?」
//      用最簡單的 XOR sum (rolling parity):每個 byte XOR 起來。
//      雖然不是 cryptographic hash,但可以快速檢查資料是否傳輸錯誤。
//
// 重點:
//   * istreambuf_iterator 邊讀邊算,記憶體佔用 O(1) — 對大檔超有用。
//   * 換成 std::ifstream 同樣寫法,可以對檔案算 checksum。
void practical_simple_checksum_of_stream() {
    std::istringstream payload("The quick brown fox jumps over the lazy dog");

    unsigned char checksum = std::accumulate(
        std::istreambuf_iterator<char>(payload),
        std::istreambuf_iterator<char>{},
        (unsigned char)0,
        [](unsigned char acc, char c){
            return acc ^ static_cast<unsigned char>(c);
        });

    std::cout << "checksum (XOR sum) = 0x"
              << std::hex << (int)checksum << std::dec << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1119: Remove Vowels from a String (移除母音)
// ----------------------------------------------------------------
// 題目:給字串,移除所有 a/e/i/o/u 後回傳。
// 為什麼這題對 streambuf_iterator 完美:
//   * 輸入是「位元流」(可以從 stdin / 檔案來),不需要解析 token。
//   * 用 std::copy_if + istreambuf_iterator 來源 + ostreambuf_iterator 目的,
//     一行寫完 — 完全位元層級的串流轉換。
//   * 比起先讀進 string 再 erase,記憶體佔用 O(1) — 對大檔有意義。
//
// 複雜度:時間 O(n)、空間 O(1) 額外。
void leetcode_1119_remove_vowels() {
    std::istringstream in("leetcodeisacommunityforcoders");
    std::ostringstream out;
    std::copy_if(std::istreambuf_iterator<char>(in),
                 std::istreambuf_iterator<char>{},
                 std::ostreambuf_iterator<char>(out),
                 [](char c){
                     return c!='a' && c!='e' && c!='i' && c!='o' && c!='u';
                 });
    std::cout << "LC1119 = " << out.str()
              << " (期望 ltcdscmmntyfrcdrs)\n";
}

// ----------------------------------------------------------------
// 實務範例:對 stream 計算 byte 數 (不讀進記憶體)
// ----------------------------------------------------------------
// 場景:web server 想知道「請求 body 有多少 byte」,但不想把整段倒進 string
// (可能 100 MB)。用 std::distance + istreambuf_iterator,邊走邊數,O(1) 空間。
// 同樣的寫法可換成 std::ifstream,就能算「檔案有多大」(不過實作上有更快的 fstat 方式)。
void practical_file_byte_count() {
    std::istringstream payload(
        "POST /api/upload HTTP/1.1\r\nContent-Type: text/plain\r\n\r\nbody body body\r\n"
    );
    auto n = std::distance(std::istreambuf_iterator<char>(payload),
                           std::istreambuf_iterator<char>{});
    std::cout << "Stream byte count = " << n << '\n';
}

// 編譯: g++ -std=c++20 -Wall -Wextra 08b_streambuf_iterators.cpp -o 08b_streambuf_iterators

// === 預期輸出 ===
// content size = 26 (期望 26,含換行與空白)
// content = [hello world
// this is line 2]
// istream_iterator<char> 結果 = [helloworld]  (空白被跳掉)
// [ostreambuf_iterator demo]
// stream→stream 結果 = ABCDE-FGHIJ
// 's' 在 "mississippi" 出現 4 次 (期望 4)
// LC387 first unique idx = 2 (期望 2,字元 'v')
// LC771 jewels in stones = 3 (期望 3)
// checksum (XOR sum) = 0x4f
// LC1119 = ltcdscmmntyfrcdrs (期望 ltcdscmmntyfrcdrs)
// Stream byte count = 71
