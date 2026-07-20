// =============================================================================
// 檔名: assign.cpp
// 主題: std::string::assign (覆蓋整個字串內容的多重 overload)
// 參考連結 References:
//   cppreference: https://en.cppreference.com/cpp/string/basic_string/assign
//   cplusplus.com: https://cplusplus.com/reference/string/string/assign/
// =============================================================================
//
// 【函式資訊 Information】
//   string& assign(size_type count, CharT ch);                        // (1)
//   string& assign(const string& str);                                // (2)
//   string& assign(const string& str, size_type pos,
//                  size_type count = npos);                           // (3)
//   string& assign(string&& str) noexcept(/*...*/);                   // (4) C++11
//   string& assign(const CharT* s, size_type count);                  // (5)
//   string& assign(const CharT* s);                                   // (6)
//   template<class InputIt>
//   string& assign(InputIt first, InputIt last);                      // (7)
//   string& assign(std::initializer_list<CharT> ilist);               // (8) C++11
//   template<class StringViewLike>
//   string& assign(const StringViewLike& t);                          // (9) C++17
//   template<class StringViewLike>
//   string& assign(const StringViewLike& t, size_type pos,
//                  size_type count = npos);                           // (10) C++17
//
// 回傳: *this (允許鏈式 assign().assign().assign()...)
//
// =============================================================================
//
// 【詳細解釋 Explanation】
//
// (一) assign 是什麼?
// ----------------------------------------------------------------------------
// assign() 與 operator= 的目的相同 —— 把字串的「整個內容」替換成新的內容,
// 但 assign 提供了大量 overload,讓你可以:
//   - 從 substring 範圍指派 (string + pos + count)
//   - 從迭代器範圍指派 (begin/end of any range)
//   - 從重複字元指派 (count, ch)
//   - 從 raw pointer + 長度指派 (避免 strlen 掃描)
//   - 從 string_view-like 物件指派 (C++17)
//
// 簡單的「整個字串覆蓋」用 operator= 即可;當你需要「部分內容」或
// 「明確指定 buffer 長度避免 \0 截斷」時,assign 才是正確選擇。
//
// (二) 為什麼需要 assign(const CharT*, size_type)?
// ----------------------------------------------------------------------------
// 如果你的資料來源是「不一定 null-terminated 的二進位 buffer」,例如:
//   - 從 read()/recv() 收到的網路資料
//   - mmap 的檔案區段
//   - 含有中段 '\0' 的字串(密碼檔、protobuf payload、binary log)
// 此時 assign(buf, len) 會精確地複製 len 個 byte,不會在遇到 '\0' 時停下,
// 也不會在末端額外掃描 strlen(會造成 buffer overrun)。
// 這是 operator=(const char*) 永遠無法做到的事情。
//
// (三) assign(InputIt, InputIt) 的威力
// ----------------------------------------------------------------------------
// 任何 input iterator 都可以當來源 —— 包括:
//   - std::vector<char>::iterator
//   - std::istreambuf_iterator<char> (從 istream 一口氣讀整個檔案)
//   - 自訂的迭代器
// 經典的「讀整個檔案到 string」一行寫法:
//   std::ifstream ifs("file.txt");
//   std::string content;
//   content.assign(std::istreambuf_iterator<char>(ifs),
//                  std::istreambuf_iterator<char>());
// 這個寫法非常實用,但要注意:對 InputIterator 而言無法事先知道長度,
// STL 必須一邊讀一邊 push_back,可能多次 reallocation。對效能敏感的
// 場景可先 reserve()。對 ForwardIterator 以上,STL 會用 std::distance
// 預先計算長度,只配置一次。
//
// (四) assign 是否會釋放現有 buffer?
// ----------------------------------------------------------------------------
// 與 operator= 相同:若 capacity 夠大,直接覆寫舊內容,不重新配置;
// 若不夠,釋放舊 buffer 並配置新的。assign 不會自動 shrink(縮小)。
// 因此「先 reserve 再多次 assign」是常見的 buffer 重用 pattern。
//
// (五) Move-assign overload (4) 的 noexcept 條件
// ----------------------------------------------------------------------------
// assign(string&&) 的 noexcept 條件複雜:
//   - 若 allocator_traits<Allocator>::propagate_on_container_move_assignment::value
//     為 true,則 allocator 連同資源一起搬走,完全 noexcept。
//   - 若上面為 false,且 allocator 不相等,則需要 element-wise move,
//     此時 *可能* 拋出例外。
// 對標準預設的 std::allocator 而言一律 noexcept。
//
// (六) 與 operator= 的取捨
// ----------------------------------------------------------------------------
//   操作                          | 用 operator= ? | 用 assign ?
//   ------------------------------|----------------|--------------------------
//   完全覆蓋(已是合適來源型別) | ✓ 簡潔        | ✓ 但較囉嗦
//   substring 範圍               | ✗ 不支援       | ✓ assign(s, pos, n)
//   迭代器範圍                   | ✗ 不支援       | ✓ assign(it, it)
//   raw buffer + 明確長度        | ✗ strlen 截斷  | ✓ assign(p, n)
//   重複 N 個字元                | ✗ 不支援       | ✓ assign(n, ch)
//
// (七) C++ 標準演進
// ----------------------------------------------------------------------------
//   C++03    : (1)(2)(3)(5)(6)(7)
//   C++11    : 加入 (4) move、(8) initializer_list、operator= 也補齊
//   C++17    : 加入 (9)(10) string_view-like overload
//   C++20    : assign 在 constexpr 上下文可用 (constexpr basic_string)
//   C++23    : 額外有 assign_range (使用 ranges,單獨檔案介紹)
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// 1) Two-pointer iterator overload 的 SFINAE / concept 設計
//    早期 STL 用 SFINAE (enable_if) 區分「assign(integer, integer) 是
//    填充 N 個 char」與「assign(InputIt, InputIt) 是迭代器範圍」。
//    若你寫 s.assign(5, 'A'),5 是 int,'A' 也是 int 字面量 —— STL 必須
//    判斷此時應走 (1) 而非 (7)。標準明文要求:當兩個型別「不是 iterator」
//    時走填充版本;當是 iterator 時走範圍版本。C++20 起改用 concepts 表達。
//
// 2) Strong / Basic Exception Safety
//    assign 通常提供 basic guarantee:若中途 throw,字串內容處於
//    valid-but-unspecified 狀態(可能部分被替換)。要 strong guarantee
//    應自己用 swap idiom:
//        string tmp;
//        tmp.assign(...);    // 若 throw,*this 不變
//        swap(*this, tmp);
//
// 3) Self-aliasing 安全性
//    assign(s.data() + 2, 3) 這類「來源指向自己內部」的呼叫是定義良好的:
//    標準要求 assign 的實作必須容忍 source 與 *this 重疊。但若 source
//    指向「自己即將被釋放的 buffer」(來源是另一個 string 的 c_str(),
//    而那個 string 在賦值前才會被銷毀)就要小心。
//
// 4) 為什麼 STL 喜歡「同個語意提供多個介面」?
//    STL 的設計哲學是「為效能與正確性提供精準工具」,而非「最少 API 表面」。
//    operator= 提供口語化捷徑,assign 提供完整功能,兩者並存正體現此哲學。
//
// 5) Iterator Invalidation
//    assign 後,先前所有 iterator/pointer/reference 全部失效。
//    這跟 operator= 一致,跟所有可能 reallocate 的修改器一致。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. assign(p, count) 不依賴 '\0' 結尾;assign(p) 會 strlen,p 必須 NUL 結尾。
// 2. assign(N, ch) 不是「把現有字元都改成 ch」,而是「重新指派為長度 N 的
//    字串,內容全是 ch」。整個 size 會變成 N。
// 3. assign(s, pos, count):若 pos > s.size(),會 throw out_of_range;
//    若 pos+count 超過 s.size(),會自動截斷,不會 throw。
// 4. assign 後 capacity 不會自動縮小;要釋放空間需 shrink_to_fit。
// 5. assign 會使所有舊 iterator 失效。
// 6. 若來源是 InputIterator(而非 ForwardIterator 以上),效能可能較差,
//    應該事先 reserve。
//
// =============================================================================

/*
補充筆記：std::string::assign
  - std::string::assign 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::assign 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::string::assign
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. assign(std::move(other)) 一定是 O(1) 嗎?
//     答:不一定。長字串走 heap 模式時只要搬 pointer / size / capacity → O(1);
//         但短字串走 SSO(內嵌 buffer)時,資料就在物件本體裡,必須真的
//         memcpy 那塊 buffer,是有常數上界的複製,不是「只搬指標」。
//     追問:SSO 門檻是多少?→ libstdc++ 為 15 個字元,但這是
//           implementation-defined,標準沒有規定任何數字,不可依賴。
//
// 🔥 Q2. 被 move 走的來源字串,之後還能用嗎?會是空字串嗎?
//     答:狀態是「valid but unspecified」——物件仍然合法,可以安全解構、
//         可以重新賦值、可以呼叫 clear() 這類沒有前置條件的操作;
//         但標準「不保證」它是空字串。實務上多數實作會清空,不可依賴。
//     追問:那讀 moved-from 物件的內容算 UB 嗎?→ 不是 UB,但結果未指定;
//           拿它去做斷言或邏輯判斷就是 bug。
//
// 🔥 Q3. 什麼時候該用 assign 而不是 operator=?
//     答:需要「只取來源的一部分」或「明確指定長度」時。例如
//         assign(str, pos, count) 取子字串、assign(buf, len) 從
//         不一定 null-terminated 的二進位 buffer 精確複製 len 個 byte、
//         assign(first, last) 從任意 iterator 範圍指派。單純整條覆蓋用
//         operator= 就好。
//     追問:assign 會縮小 capacity 嗎?→ 不會。capacity 夠就原地覆寫,
//           所以「reserve 一次 + 反覆 assign」是常見的 buffer 重用手法。
//
// ⚠️ 陷阱. assign(buf) 和 assign(buf, n) 對含有中段 '\0' 的資料為何結果不同?
//     答:assign(const char*) 走 C 字串規則,在第一個 '\0' 就停下;
//         assign(const char*, count) 精確複製 count 個 byte,中段 '\0' 照收。
//         處理 recv() / mmap / protobuf payload 一定要用帶長度的版本。
//     為什麼會錯:std::string 確實能合法存放內含 '\0' 的資料(size() 正確),
//         於是大家忘了「從 const char* 灌進去」這條入口仍然由 '\0' 決定長度。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iterator>

void demoAssign() {
    std::cout << "=== assign 各種 overload ===\n";

    std::string s;

    // (1) 重複字元
    s.assign(5, 'A');
    std::cout << "(1) assign(5,'A')             : \"" << s << "\"\n";

    // (2) 從另一個 string
    std::string src = "Hello, World";
    s.assign(src);
    std::cout << "(2) assign(string&)           : \"" << s << "\"\n";

    // (3) 從 substring (pos=7, count=5 → "World")
    s.assign(src, 7, 5);
    std::cout << "(3) assign(string&,pos,count) : \"" << s << "\"\n";

    // (4) move
    std::string tmp = "MoveSource";
    s.assign(std::move(tmp));
    std::cout << "(4) assign(string&&)          : \"" << s << "\"\n";

    // (5) raw pointer + 明確長度 (可含內嵌 '\0')
    const char data[] = {'A','B','\0','C','D'};
    s.assign(data, sizeof(data));
    std::cout << "(5) assign(p,5)               : size=" << s.size()
              << " (含內嵌 \\0)\n";

    // (6) C-string
    s.assign("CString");
    std::cout << "(6) assign(const char*)       : \"" << s << "\"\n";

    // (7) iterator range
    std::vector<char> v = {'V','E','C'};
    s.assign(v.begin(), v.end());
    std::cout << "(7) assign(it,it)             : \"" << s << "\"\n";

    // (8) initializer_list
    s.assign({'I','L','I','S','T'});
    std::cout << "(8) assign(ilist)             : \"" << s << "\"\n";

    // (9) string_view-like (C++17)
    std::string_view sv = "ViewLike";
    s.assign(sv);
    std::cout << "(9) assign(string_view)       : \"" << s << "\"\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例】LeetCode 1816. Truncate Sentence (Easy)
//
// 題目敘述:
//   給一個句子 s 與正整數 k,回傳 s 的「前 k 個單字」組成的新句子。
//   範例: s = "Hello how are you Contestant", k = 4
//        回傳 "Hello how are you"
//
// 為何用 assign:
//   找到第 k 個空白後,我們可以用 assign(s, 0, pos) 一次把前綴複製到
//   結果字串。這比 substr 多一個好處:可以「重用」一個事先 reserve
//   過的結果 buffer,適合會被連續呼叫的批次處理 (例如把同一段文字
//   依不同 k 切多次)。
//
// 解題思路:
//   從頭數空白;遇到第 k 個空白時記下 pos;若整段沒有 k 個空白就回傳全部。
//
// 複雜度: 時間 O(n),空間 O(k)
// -----------------------------------------------------------------------------
std::string truncateSentence(const std::string& s, int k) {
    int spaces = 0;
    size_t cut = s.size();
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == ' ') {
            if (++spaces == k) { cut = i; break; }
        }
    }
    std::string out;
    out.reserve(cut);
    out.assign(s, 0, cut);   // 直接從來源 substring 賦值,免額外複製
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】整檔讀入(file slurp)成 std::string
//
// 為何用 assign:
//   後端 / DevOps 經常要把整個檔案吞進記憶體做 parse(設定檔、token
//   檔、SQL schema、Cert PEM、Webhook payload 暫存等)。
//   經典寫法是 istreambuf_iterator + assign:
//     content.assign(std::istreambuf_iterator<char>(ifs), {});
//   這個 idiom 比逐行 getline 累積快、比一次 read 進 vector 再轉 string
//   省一次複製。assign 的 InputIterator overload 正是為此而生。
//
//   注意:這個寫法在 InputIterator 模式下無法預知總長度,STL 會多次
//   resize。若檔案很大,先 seekg 量檔長 + reserve 可顯著加速。
// -----------------------------------------------------------------------------
std::string slurpFile(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return {};

    // 預先量檔長,減少 reallocation
    ifs.seekg(0, std::ios::end);
    std::streamsize len = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    std::string content;
    if (len > 0) content.reserve(static_cast<size_t>(len));

    content.assign(std::istreambuf_iterator<char>(ifs),
                   std::istreambuf_iterator<char>());
    return content;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1108. Defanging an IP Address (再用 assign 變奏)
// 題目: 把 "1.1.1.1" 變成 "1[.]1[.]1[.]1"。
// 為何用 assign: 重複呼叫時可重用同一個輸出 buffer,先 reserve、再 assign 為空、
//                然後逐字組裝,避免每次都重新配置。展示「buffer 重用」實務手法。
// -----------------------------------------------------------------------------
std::string defangIpReuseBuffer(const std::string& ip, std::string& buf) {
    buf.assign("");                       // 重設長度為 0,capacity 保留
    buf.reserve(ip.size() + 6);
    for (char c : ip) {
        if (c == '.') buf.append("[.]");
        else          buf.push_back(c);
    }
    return buf;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】從 vector<char> 網路 buffer 安全建立 string
// 為何用 assign: 接 socket 後資料常存於 vector<char>,可能含 '\0' 或無 NUL 結尾;
//                用 assign(first, last) 明確以長度為界,不依賴 NUL,完全安全。
// -----------------------------------------------------------------------------
std::string fromNetworkBuffer(const std::vector<char>& buf, size_t n) {
    std::string out;
    out.reserve(n);
    out.assign(buf.begin(), buf.begin() + static_cast<std::ptrdiff_t>(n));
    return out;
}

int main() {
    demoAssign();

    std::cout << "\n=== LeetCode 1816 ===\n";
    std::cout << "\"" << truncateSentence("Hello how are you Contestant", 4) << "\"\n";
    std::cout << "\"" << truncateSentence("What is the solution", 4) << "\"\n";

    std::cout << "\n=== LeetCode 1108 (buffer 重用) ===\n";
    {
        std::string buf;
        std::cout << defangIpReuseBuffer("1.1.1.1", buf) << "\n";
        std::cout << defangIpReuseBuffer("255.100.50.0", buf) << "\n";
    }

    std::cout << "\n=== 日常實務: file slurp 示範 (寫一個臨時檔再讀回) ===\n";
    {
        std::ofstream ofs("/tmp/_assign_demo.txt");
        ofs << "line1\nline2\nline3\n";
    }
    std::string body = slurpFile("/tmp/_assign_demo.txt");
    std::cout << "讀回 " << body.size() << " bytes:\n" << body;

    std::cout << "\n=== 日常實務: 從 vector<char> 建 string ===\n";
    std::vector<char> net = {'H','e','l','l','o','\0','x'};
    std::cout << "[" << fromNetworkBuffer(net, 7) << "] size=" << fromNetworkBuffer(net, 7).size() << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：assign 與 operator= 在 capacity 行為上有何差異?
    //    A：兩者都不會自動縮小 capacity ── 若舊 buffer 夠用就直接覆寫。
    //       唯一差別是 assign 提供更多重載 (substring、iterator、count+ch、
    //       raw buffer + length),operator= 只支援單一來源。重複使用同一個
    //       buffer 處理多筆輸入時,assign 配合事先 reserve 是經典 pattern。
    //
    //  Q2：為什麼 assign(p, count) 比 operator=(p) 更安全?
    //    A：operator=(const char*) 會用 strlen 找結尾 '\0',若資料含內嵌 '\0'
    //       會被截斷,若沒結尾 '\0' 會 buffer overrun。assign(p, count) 明確
    //       吃 count 個 byte,適合網路資料、二進位檔、protobuf payload 等
    //       不一定 NUL-terminated 的場景。
    //
    //  Q3：assign(InputIt, InputIt) 為何在大檔案讀取時可能很慢?
    //    A：InputIterator (例如 istreambuf_iterator) 不支援 std::distance,
    //       STL 無法事先得知總長度,只能逐字 push_back,可能多次 reallocate。
    //       對 ForwardIterator 以上會用 std::distance 預先計算,只配置一次。
    //       讀大檔前先 seekg 量檔長 + reserve 可顯著加速。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra assign.cpp -o assign

// === 預期輸出 (節錄) ===
// === LeetCode 1816 ===
// "Hello how are you"
// "What is the solution"
//
// === LeetCode 1108 (buffer 重用) ===
// 1[.]1[.]1[.]1
// 255[.]100[.]50[.]0
//
// === 日常實務: 從 vector<char> 建 string ===
// [Hello x] size=7   (內含一個 \0)
