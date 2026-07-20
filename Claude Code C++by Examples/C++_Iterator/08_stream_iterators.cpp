// =============================================================================
//  08_stream_iterators.cpp  —  istream_iterator / ostream_iterator
// =============================================================================
//  讓 STL 演算法 + 迴圈直接和 IO 串接，不需手寫 while(cin>>x)。
//
//  std::istream_iterator<T>：
//    * 是 InputIterator (只讀 + 單次走訪)。
//    * 建構：istream_iterator<T>(stream) — 立刻嘗試讀第一筆 (operator>>)。
//    * end-of-stream sentinel = 預設建構的 istream_iterator<T>{}.
//
//  std::ostream_iterator<T>：
//    * 是 OutputIterator。
//    * 建構：ostream_iterator<T>(stream)            — 不加分隔
//            ostream_iterator<T>(stream, " ")      — 每寫一筆後接 " "
//
//  istreambuf_iterator / ostreambuf_iterator：
//    * 對「字元等級」的 buffer 直讀直寫，比 istream_iterator<char> 更快、
//      也不會跳過空白。讀整個檔案常見手法：
//          string s{ istreambuf_iterator<char>(file), {} };
//
//  陷阱：
//   * istream_iterator 預設會「跳過 whitespace」(因為背後是 operator>>)。
//     要保留空白請改用 istreambuf_iterator 或設 stream.unsetf(ios::skipws)。
//   * 用 istream_iterator 和容器建構函式時，最外面要用「圓括號或 {}」，
//     注意「Most Vexing Parse」陷阱：
//         vector<int> v(istream_iterator<int>(in), istream_iterator<int>());
//     ↑ 這行會被解釋成「函式宣告」！要寫成：
//         vector<int> v{ istream_iterator<int>(in),
//                        istream_iterator<int>{} };
//
//  參考連結 (cppreference / cplusplus)：
//    https://en.cppreference.com/w/cpp/iterator/istream_iterator    — input
//    https://en.cppreference.com/w/cpp/iterator/ostream_iterator    — output
//    https://cplusplus.com/reference/iterator/istream_iterator/     — 簡明
//    https://cplusplus.com/reference/iterator/ostream_iterator/     — 簡明
// =============================================================================

/*
補充筆記：stream_iterators
  - stream_iterators 的核心是「位置」與「走訪能力」；iterator 不擁有元素，只是通往容器元素的操作介面。
  - 不同 iterator category 能力不同：input 只能讀一次，forward 可多次走訪，bidirectional 可退後，random access 可做加減與索引。
  - end iterator 是哨兵位置，不能解參考；所有 [begin, end) 半開區間都依賴這個規則。
  - 容器修改可能讓 iterator 失效；vector reallocation、erase、insert 的規則和 list/map 不同，不能通用直覺。
  - insert iterator、stream iterator、move iterator 都是在改變演算法如何讀寫元素，而不是改變演算法本身。
  - 自訂 iterator 要讓 value_type、reference、difference_type、iterator_category 等 traits 能被演算法理解。
  - istream_iterator 使用 operator>> 讀格式化資料，會跳過空白並受型別解析規則影響。
  - ostream_iterator 可把演算法結果直接輸出到 stream，常搭配 delimiter。
  - stream iterator 失敗時會到達 end iterator，錯誤狀態仍保存在 stream 中。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】istream_iterator / ostream_iterator
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. istream_iterator 是哪一類 iterator？它的 end 從哪裡來？
//     答：是 InputIterator——只讀、single-pass，資料從 stream 取出就消耗掉，無法走第二遍。
//         它的「結尾」是 default constructed 的 istream_iterator<T>{}，也就是
//         end-of-stream sentinel；當 stream 讀取失敗或到達 EOF，iterator 就與它相等。
//     追問：讀取失敗和 EOF 分得出來嗎？（分不出來，兩者都讓 iterator 等於 end；要判斷
//         真正原因得回頭查 stream 自己的狀態旗標）
//
// 🔥 Q2. 為什麼 istream_iterator<char> 讀出來的字元少了空白和換行？
//     答：因為它背後用的是 operator>>，屬於格式化輸入，預設會跳過 whitespace。要一個字元
//         都不漏，就改用 istreambuf_iterator<char>（低階字元層），或對 stream 設
//         unsetf(std::ios::skipws)。
//     追問：ostream_iterator 的第二個建構參數是什麼？（每寫一筆後附加的 delimiter）
//
// ⚠️ 陷阱. vector<int> v(istream_iterator<int>(in), istream_iterator<int>()); 為什麼不能用？
//     答：這行會被編譯器解讀成一個「函式宣告」，也就是 Most Vexing Parse。改成大括號即可：
//         vector<int> v{ istream_iterator<int>(in), istream_iterator<int>{} };
//     為什麼會錯：以為括號裡放的一定是物件，但只要那段文字也能被解析成參數列，標準規定
//         優先當宣告解析。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <iterator>
#include <sstream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <string>

int main() {
    // -----------------------------------------------------------------------
    // 範例 1：用 stream iterators 把字串流整數一行印出，並用 " | " 分隔
    // -----------------------------------------------------------------------
    std::istringstream iss("10 20 30 40 50");

    std::copy(std::istream_iterator<int>(iss),
              std::istream_iterator<int>(),
              std::ostream_iterator<int>(std::cout, " | "));
    std::cout << '\n';

    // -----------------------------------------------------------------------
    // 範例 2：建構 vector 時用 stream iterators (注意大括號避開 vexing parse)
    // -----------------------------------------------------------------------
    std::istringstream iss2("3 1 4 1 5 9 2 6 5 3 5");
    std::vector<int> v{ std::istream_iterator<int>(iss2),
                        std::istream_iterator<int>{} };
    std::cout << "讀進 " << v.size() << " 個整數，sum = "
              << std::accumulate(v.begin(), v.end(), 0) << '\n';

    // -----------------------------------------------------------------------
    // 範例 3：算字串中有幾個整數 ≥ 5
    // -----------------------------------------------------------------------
    std::istringstream iss3("3 1 4 1 5 9 2 6 5 3 5");
    auto cnt = std::count_if(std::istream_iterator<int>(iss3),
                             std::istream_iterator<int>(),
                             [](int x){ return x >= 5; });
    std::cout << ">=5 的個數 = " << cnt << '\n';

    // -----------------------------------------------------------------------
    // 範例 4：ostream_iterator 搭 transform — 「平方然後印出來」
    // -----------------------------------------------------------------------
    std::vector<int> nums = {1, 2, 3, 4, 5};
    std::transform(nums.begin(), nums.end(),
                   std::ostream_iterator<int>(std::cout, " "),
                   [](int x){ return x * x; });        // 1 4 9 16 25
    std::cout << '\n';

    // -----------------------------------------------------------------------
    // LeetCode 風格示範：
    //   LC 1773 (簡化)：把空白分隔的字串切成 vector<string> 並印出
    //   也是處理 "輸入字串轉 token" 這類面試常見題型的好工具。
    // -----------------------------------------------------------------------
    std::istringstream iss4("the quick brown fox jumps over");
    std::vector<std::string> tokens{
        std::istream_iterator<std::string>(iss4),
        std::istream_iterator<std::string>{}
    };
    std::cout << "tokens(" << tokens.size() << "): ";
    std::copy(tokens.begin(), tokens.end(),
              std::ostream_iterator<std::string>(std::cout, ","));
    std::cout << '\n';

    // -----------------------------------------------------------------------
    // LeetCode 風格示範 2：
    //   LC 412. Fizz Buzz
    //   * 1..n: 3的倍數 → "Fizz"; 5的倍數 → "Buzz"; 15的倍數 → "FizzBuzz"; 否則數字。
    //   * 用 generate_n + ostream_iterator 把整個產出一氣呵成寫到 stream，
    //     不需要先 build vector，記憶體與程式碼都最精簡。
    // -----------------------------------------------------------------------
    auto fizz_buzz_at = [](int i) -> std::string {
        if (i % 15 == 0) return "FizzBuzz";
        if (i % 3  == 0) return "Fizz";
        if (i % 5  == 0) return "Buzz";
        return std::to_string(i);
    };

    std::cout << "FizzBuzz 1..15:\n  ";
    int counter = 0;
    std::generate_n(std::ostream_iterator<std::string>(std::cout, " "),
                    15,
                    [&]{ return fizz_buzz_at(++counter); });
    std::cout << '\n';

    // -----------------------------------------------------------------------
    // 課程知識補充：「Most Vexing Parse」陷阱再強調
    //   * 用 istream_iterator 建構容器時最容易踩到。經典踩法：
    //         std::vector<int> v(std::istream_iterator<int>(cin),
    //                            std::istream_iterator<int>());
    //     ↑ 看起來像建構，實際上 C++ 規定它是「函式宣告」：
    //       「v 是函式，吃兩個參數，回傳 vector<int>」.
    //     編譯器不會抱怨，但 v 不是 vector — 你拿 v.push_back 才會炸.
    //
    //   * 三種解法：
    //       1. 把第二個參數用 {} 起：std::istream_iterator<int>{}
    //       2. 整個建構參數用大括號：std::vector<int> v{ A, B };
    //       3. 額外加括號：std::vector<int> v((A), (B));   // 醜但可行
    //
    //   * 推薦寫法 (C++11 起最乾淨)：
    //         std::vector<int> v{
    //             std::istream_iterator<int>(in),
    //             std::istream_iterator<int>{}
    //         };
    // -----------------------------------------------------------------------

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：istream_iterator 跟 istreambuf_iterator 差在哪？什麼時候該用哪個？
    //    A：istream_iterator<T> 走「格式化讀」(operator>>)，會跳空白與換行、
    //      會解析 token (例如 "42" 變 int)。istreambuf_iterator<char> 走「字元讀」，
    //      一個 byte 都不漏 (含空白、換行)，速度更快。要解析數字請用前者；
    //      要原原本本搬 byte (例如讀整個檔案、計算 checksum) 請用後者。
    //
    //  Q2：Most Vexing Parse 為什麼會發生？怎麼避免？
    //    A：C++ 規範遇到「能解釋成函式宣告就解釋成函式宣告」。下面這行：
    //          vector<int> v(istream_iterator<int>(in), istream_iterator<int>());
    //      會被解讀成「v 是函式，吃 (istream_iterator<int>, istream_iterator<int>(*)())
    //      回傳 vector<int>」。三種解法：用大括號 v{ A, B }、第二個參數用 {}
    //      建構、或多包一層括號 v((A), (B))。最推薦 C++11 起的大括號寫法。
    //
    //  Q3：ostream_iterator 的 ++ 為什麼是 no-op？真正前進在哪一步？
    //    A：ostream_iterator 內部沒有「位置」概念，*it 直接回傳自己，operator= 才
    //      把資料 << 到 stream 並追加分隔符。++ 不需要做任何事 — 因為 stream
    //      本身已經前進了 (字元送出後 buffer 自動往前)。這也說明為什麼
    //      OutputIterator 規範允許 ++ 是 no-op：「真正的前進」在寫入動作裡。
    //

    // -----------------------------------------------------------------------
    // LC 範例: LC 1108 — Defanging an IP Address (IP 位址脫敏)
    // -----------------------------------------------------------------------
    // 給字串 "1.1.1.1",把每個 "." 換成 "[.]" 回傳 "1[.]1[.]1[.]1"。
    // 雖然簡單,但用「ostringstream + ostream_iterator」做 token 處理是真實場景的
    // 縮影:讀 token 然後寫到輸出串流 — 完全靠 stream iterator 連接 input/output。
    {
        auto defang = [](const std::string& addr) {
            std::ostringstream oss;
            for (char c : addr) {
                if (c == '.') oss << "[.]";
                else          oss << c;
            }
            return oss.str();
        };
        std::cout << "LC1108 defang(\"1.1.1.1\") = " << defang("1.1.1.1") << '\n';
        // 預期輸出: 1[.]1[.]1[.]1
    }

    // -----------------------------------------------------------------------
    // 實戰範例:把 CSV 行內的數字轉成 sum (一行解析)
    // -----------------------------------------------------------------------
    // 場景:後端每天要解析 csv "10,20,30,40" 取得各欄位整數總和。
    // 用 istringstream 把分隔符替換掉,再用 istream_iterator<int> 一次倒進 accumulate,
    // 一行解決 — 不需要手寫 strtok / split。這是 stream iterator 真實工作上最常見的用法。
    {
        std::string line = "10 20 30 40 50";   // 真實場景可先把 ',' 換成空白
        std::istringstream iss(line);
        int total = std::accumulate(std::istream_iterator<int>(iss),
                                    std::istream_iterator<int>{},
                                    0);
        std::cout << "CSV sum = " << total << " (期望 150)\n";
    }

    return 0;
}
