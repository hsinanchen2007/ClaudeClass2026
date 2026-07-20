// =============================================================================
//  第五課：迭代器的五種分類 1  —  Input Iterator：istream_iterator 讀取串流
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<iterator>
//   類別：template <class T, class CharT = char, ...> class istream_iterator;
//   建構方式：
//     std::istream_iterator<int> it(std::cin);   // 綁定串流；**建構時就會讀第一個值**
//     std::istream_iterator<int> end;            // 預設建構 = 「串流結束」哨兵
//   迭代器類別：**Input Iterator**（最弱的一種）
//     可以：*it（讀）、++it（前進）、it == end / it != end（比較）
//     不行：--it（後退）、it + n（跳躍）、寫入、多次遍歷同一段資料
//   標準版本：C++98 即有；C++11 起 istream_iterator 的預設建構子為 constexpr。
//   複雜度：每次 ++ 觸發一次 operator>>，成本取決於串流與型別解析。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「串流」只能配最弱的迭代器】
//   標準輸入是一條**單向流過去的資料**：讀掉的位元組就不在緩衝區裡了。
//   你無法「往回讀上一個數字」，也無法「跳到第 10 個數字」——
//   除非把整條串流的內容先存下來，但那就不是串流了。
//   Input Iterator 這個分類正是誠實地描述了這個限制：
//   它只承諾「能讀、能往前、能判斷結束」，其餘一概不保證。
//   這再次體現 STL 的設計原則：**介面能力必須反映底層真實能力**。
//
// 【2. 預設建構的 istream_iterator 為什麼代表「結束」】
//   它不綁定任何串流（內部串流指標為 null）。
//   標準規定：兩個 istream_iterator 相等，若且唯若「兩者都處於結束狀態」
//   或「兩者綁定同一串流且都尚未結束」。
//   而綁定串流的那個迭代器一旦讀取失敗（EOF 或格式錯誤），
//   就會把自己標記為結束狀態 → 於是 `it == end` 成立，迴圈自然終止。
//   這是「哨兵（sentinel）」模式的經典範例，也是 C++20 ranges
//   把 end 泛化成「可以是不同型別的哨兵」的思想源頭。
//
// 【3. 建構當下就會讀取第一個值（容易忽略的細節）】
//       std::istream_iterator<int> it(std::cin);   // ← 這行就已經嘗試讀了一個 int
//   這是所謂的「eager」行為。它的後果是：
//     - 如果串流一開始就是空的，it 立刻等於 end
//     - 建構這個物件本身就有副作用（會消耗輸入），不能當成無害的宣告
//
// 【4. 「輸入非數字就結束」是怎麼發生的】
//   istream_iterator<int> 用 operator>>(istream&, int&) 讀取。
//   遇到無法解析成 int 的字元時，串流會進入 fail 狀態，
//   迭代器隨即標記為結束。所以輸入 `1 2 3 x` 會讀到 1、2、3 就停止。
//   注意串流的 fail 狀態不會自動清除 —— 若之後還要繼續用 std::cin，
//   必須手動 cin.clear() 並丟棄壞掉的輸入。
//
// 【概念補充 Concept Deep Dive】
//   Input Iterator 有一條常被忽略的規則：**遞增之後，先前的副本就失效了**。
//       auto a = it;
//       ++it;
//       *a;          // 未定義行為！a 已經不保證有效
//   標準的用語是「Input Iterator 只保證單次走訪（single-pass）」。
//   這正是它與 Forward Iterator 的分界線 ——
//   Forward Iterator 保證「多次走訪（multi-pass）」：
//   複製一份迭代器、各自前進，兩者都仍然有效且會得到相同序列。
//   實務影響：任何需要「回頭再看一次」的演算法（例如 std::sort、
//   甚至只是先數一遍長度再處理）都不能用在 Input Iterator 上。
//   若真的需要，唯一辦法是先把資料吸進容器 —— 這正是本檔 main() 做的事：
//       std::vector<int> numbers(input_begin, input_end);
//   一旦進了 vector，你就得到了 Random Access Iterator，什麼都能做了。
//
// 【注意事項 Pay Attention】
//   1. 建構 istream_iterator 時就會讀取第一個值 —— 這是有副作用的宣告。
//   2. 預設建構的 istream_iterator 是「結束哨兵」，不綁定任何串流。
//   3. Input Iterator 只保證單次走訪；++ 之後舊的副本即失效。
//   4. 讀到無法解析的資料時串流進入 fail 狀態，迭代即結束；
//      若之後還要用該串流，必須 clear() 並清掉壞輸入。
//   5. 不能對 istream_iterator 用 std::sort / std::reverse / std::distance 後再讀取
//      —— 前兩者需要更強的迭代器，後者會把串流消耗掉。
//   6. 本程式的輸出取決於你餵給它什麼輸入；下方預期輸出是用特定輸入取得的。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Input Iterator 與 istream_iterator
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 istream_iterator 只能是 Input Iterator，不能更強？
//     答：因為串流的資料是單向流過的 —— 讀掉就沒了，無法回頭也無法跳躍。
//         STL 的原則是「介面能力必須反映底層真實能力」，
//         所以只承諾 *it、++it、比較這三件事。
//         若硬要提供 --it，就得在背後偷偷緩衝整條串流，那是欺騙使用者。
//     追問：預設建構的 istream_iterator 代表什麼？
//           → 「串流結束」的哨兵。它不綁定任何串流；
//             當綁定串流的那個迭代器讀取失敗（EOF 或格式錯誤）時，
//             會把自己標記為結束狀態，於是 it == end 成立。
//
// 🔥 Q2. Input Iterator 與 Forward Iterator 的分界線是什麼？
//     答：**多次走訪（multi-pass）保證**。
//         Forward Iterator 保證：複製一份迭代器、各自前進，
//         兩者都仍然有效，且走訪會得到相同的序列。
//         Input Iterator 沒有這個保證 —— ++it 之後，先前的副本即失效。
//         這就是為什麼需要「回頭再看一次」的演算法都要求 Forward 以上。
//     追問：那要對串流資料排序該怎麼做？
//           → 先用範圍建構子把它吸進容器：
//             std::vector<int> v(istream_iterator<int>(cin), istream_iterator<int>());
//             進了 vector 就有 Random Access Iterator，std::sort 才能用。
//
// ⚠️ 陷阱. 為什麼下面這行看起來很自然，卻不是在宣告 vector？
//          std::vector<int> v(std::istream_iterator<int>(std::cin),
//                             std::istream_iterator<int>());
//     答：這是 C++ 著名的「最令人苦惱的解析（Most Vexing Parse）」。
//         編譯器會把它解讀成**函式宣告**：一個名為 v、回傳 vector<int> 的函式，
//         第一個參數是名為 cin 的 istream_iterator<int>，
//         第二個參數是「一個回傳 istream_iterator<int> 的無參數函式指標」。
//         結果是「v 不是變數」，之後用 v[0] 就會出現莫名其妙的錯誤訊息。
//     為什麼會錯：C++ 的規則是「只要能解析成宣告，就解析成宣告」。
//         解法：(a) 像本檔一樣先把兩個迭代器宣告成具名變數（最清楚）；
//               (b) 用大括號初始化 std::vector<int> v{first, last};
//               (c) 多加一層括號 std::vector<int> v((istream_iterator<int>(cin)), ...);
//         C++11 的統一初始化語法就是為了收掉這類陷阱而設計的。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <iterator>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】不強加。
//   理由：LeetCode 的題目一律以「函式參數」的形式給你資料（vector、string），
//         從來不需要自己從 stdin 讀取 —— 平台已經幫你做好 I/O 了。
//         istream_iterator 的價值在命令列工具與資料管線，
//         那正是下面實務範例的場景。硬掛一題只會誤導讀者。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】命令列資料管線：從 stdin 讀數值並輸出統計
//   情境：維運常需要寫一次性的小工具，把上游指令的輸出接進來做統計，例如
//         cat latency.log | awk '{print $3}' | ./stats
//   為什麼用到本主題：這正是 istream_iterator 的天職 ——
//         「把整條串流一口氣吸進容器」只需要一行範圍建構。
//         而且它示範了 Input Iterator 的核心限制與解法：
//         串流只能走一次，所以要做多項統計就必須**先存進容器**。
//   注意：這裡用 istringstream 模擬串流，讓本範例的輸出是確定的；
//         實際工具中把 iss 換成 std::cin 即可。
// -----------------------------------------------------------------------------
struct Stats {
    std::size_t count = 0;
    long long   sum   = 0;
    int         min_v = 0;
    int         max_v = 0;
    double      mean  = 0.0;
};

Stats analyzeStream(std::istream& in) {
    // 一行把整條串流吸進 vector —— Input Iterator 最常見的用法
    std::istream_iterator<int> first(in);
    std::istream_iterator<int> last;                     // 結束哨兵
    std::vector<int> values(first, last);

    Stats s;
    if (values.empty()) return s;

    // 進了 vector 之後就是 Random Access Iterator，想走幾遍都可以
    s.count = values.size();
    for (int v : values) s.sum += v;
    auto mm = std::minmax_element(values.begin(), values.end());
    s.min_v = *mm.first;
    s.max_v = *mm.second;
    s.mean  = static_cast<double>(s.sum) / static_cast<double>(s.count);
    return s;
}

int main() {
    std::cout << "=== Input Iterator 示範 ===" << std::endl;
    std::cout << "請輸入一些整數（輸入非數字結束）：" << std::endl;

    // istream_iterator 是 Input Iterator
    std::istream_iterator<int> input_begin(std::cin);
    std::istream_iterator<int> input_end;  // 預設建構 = 結束標記

    // 讀取所有輸入到 vector, 使用 Input Iterator 的範圍建構函式
    // （注意：這裡先宣告成具名變數，正是為了避開「最令人苦惱的解析」）
    std::vector<int> numbers(input_begin, input_end);

    std::cout << "你輸入了 " << numbers.size() << " 個數字：";
    for (int n : numbers) {
        std::cout << n << " ";
    }
    std::cout << std::endl;

    // 進了 vector 之後就是 Random Access Iterator —— 想走幾遍都行
    std::cout << "\n=== 進了容器就不再受 Input Iterator 限制 ===" << std::endl;
    if (!numbers.empty()) {
        std::cout << "  第一次走訪（求和）  : ";
        long long sum = 0;
        for (int n : numbers) sum += n;
        std::cout << sum << std::endl;

        std::cout << "  第二次走訪（反向印）: ";
        for (auto rit = numbers.rbegin(); rit != numbers.rend(); ++rit) {
            std::cout << *rit << " ";
        }
        std::cout << std::endl;

        std::vector<int> sorted = numbers;
        std::sort(sorted.begin(), sorted.end());     // 對串流本身做不到這件事
        std::cout << "  排序後              : ";
        for (int n : sorted) std::cout << n << " ";
        std::cout << std::endl;
        std::cout << "  → 以上三件事對 istream_iterator 全部辦不到" << std::endl;
    } else {
        std::cout << "  （沒有輸入任何數字）" << std::endl;
    }

    std::cout << "\n=== 日常實務：資料管線統計（用 istringstream 模擬 stdin）==="
              << std::endl;
    std::istringstream fake_stdin("120 95 240 180 310 150 88");
    Stats s = analyzeStream(fake_stdin);
    std::cout << "  輸入: 120 95 240 180 310 150 88" << std::endl;
    std::cout << "  筆數: " << s.count << std::endl;
    std::cout << "  總和: " << s.sum << std::endl;
    std::cout << "  最小/最大: " << s.min_v << " / " << s.max_v << std::endl;
    std::cout << "  平均: " << s.mean << std::endl;
    std::cout << "  （實際工具中把 istringstream 換成 std::cin 即可）" << std::endl;

    return 0;
}

// 注意（輸出取決於輸入）：
//   本程式的前半段會從**標準輸入**讀取整數，因此輸出隨你餵的資料而變。
//   下方預期輸出是用以下指令取得的：
//       echo "3 1 4 1 5 9 end" | ./demo1
//   若直接執行而不給輸入（例如按 Ctrl-D），
//   則「你輸入了 0 個數字」，並顯示「（沒有輸入任何數字）」。
//   後半段的「日常實務」區塊使用固定的 istringstream，輸出永遠相同。

// 編譯: g++ -std=c++17 -Wall -Wextra 第五課：迭代器的五種分類1.cpp -o demo1

// === 預期輸出 ===
// === Input Iterator 示範 ===
// 請輸入一些整數（輸入非數字結束）：
// 你輸入了 6 個數字：3 1 4 1 5 9
//
// === 進了容器就不再受 Input Iterator 限制 ===
//   第一次走訪（求和）  : 23
//   第二次走訪（反向印）: 9 5 1 4 1 3
//   排序後              : 1 1 3 4 5 9
//   → 以上三件事對 istream_iterator 全部辦不到
//
// === 日常實務：資料管線統計（用 istringstream 模擬 stdin）===
//   輸入: 120 95 240 180 310 150 88
//   筆數: 7
//   總和: 1183
//   最小/最大: 88 / 310
//   平均: 169
//   （實際工具中把 istringstream 換成 std::cin 即可）
