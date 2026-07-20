// =============================================================================
//  第一課：STL 的歷史與設計哲學 4  —  可組合性：演算法 × 配接器 × 函數物件
// =============================================================================
//
// 【主題資訊 Information】
//   template<class InputIt, class OutputIt, class UnaryPredicate>
//   OutputIt copy_if(InputIt first, InputIt last, OutputIt d_first, UnaryPredicate pred);
//   標頭檔：<algorithm>（copy_if）、<iterator>（back_inserter）
//   標準版本：copy_if 是 C++11（C++98 只有 remove_copy_if 這種「反向」寫法）；
//             back_inserter 是 C++98；lambda 是 C++11。
//   複雜度：O(N) 次述詞呼叫；回傳「輸出端的結尾迭代器」。
//   前置條件：d_first 開始的目的地必須有足夠空間 —— 除非你用 inserter 配接器。
//
// 【詳細解釋 Explanation】
//
// 【1. 這五行程式碼裡有三個獨立發展的概念】
//   std::copy_if(source.begin(), source.end(),
//                std::back_inserter(destination),
//                [](int n){ return n % 2 == 0; });
//   拆開來看：
//     ① copy_if        —— 演算法：定義「掃過一段區間、挑出符合的、寫到輸出端」
//     ② back_inserter  —— 迭代器配接器：把「寫入」翻譯成「push_back」
//     ③ lambda         —— 函數物件：定義「符合」是什麼意思
//   三者是三個不同的擴充點，互不知道對方存在。你可以把 ③ 換成任何述詞、
//   把 ② 換成 front_inserter 或 ostream_iterator、把 ① 換成 transform，
//   任意組合都成立。這就是「像樂高一樣組合」的具體意思。
//
// 【2. back_inserter 解決的是一個真實的危險】
//   std::copy_if 的第三個參數是輸出迭代器。如果你直接傳 destination.begin()：
//       std::copy_if(src.begin(), src.end(), dst.begin(), pred);   // 危險！
//   演算法會直接對 dst.begin() 做 *it = value 然後 ++it。它完全不知道
//   dst 有多大，也不會幫你擴容。若 dst 是空的，第一次寫入就是越界，
//   是未定義行為（不保證崩潰，可能默默踩壞記憶體）。
//   back_inserter 回傳一個 back_insert_iterator，它把 `*it = value`
//   重新定義成 `container.push_back(value)`，於是容量問題自動消失。
//
// 【3. back_insert_iterator 是怎麼騙過演算法的】
//   演算法只會對輸出迭代器做三件事：*it、= value、++it。
//   back_insert_iterator 就把這三個運算全部改寫：
//       operator*()  → 回傳 *this（自己）
//       operator=(v) → 呼叫 container->push_back(v)
//       operator++() → 什麼都不做，回傳 *this
//   所以 `*it = value; ++it;` 實際上等於 `container.push_back(value);`。
//   演算法一個字都不必改，就從「覆寫既有元素」變成「附加新元素」。
//   這是 C++ 用「運算子重載 + 靜態多型」達成 adapter pattern 的教科書案例。
//
// 【4. lambda 為什麼比函式指標好】
//   `[](int n){ return n % 2 == 0; }` 不是函式，是一個「匿名類別的物件」，
//   編譯器會為它生成一個帶 operator() 的獨一無二的類別。
//   因為型別是獨一無二的，copy_if 具現化時就知道要呼叫哪份程式碼，
//   可以完全 inline。換成函式指標，就退化成執行期間接呼叫。
//   額外好處：lambda 可以捕捉外部變數（見下方實務範例的 minLevel）。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 三種 inserter 配接器的差異
//     back_inserter(c)  → c.push_back(v)   需要容器有 push_back（vector/deque/list）
//     front_inserter(c) → c.push_front(v)  需要 push_front（deque/list，vector 沒有）
//     inserter(c, it)   → c.insert(it, v)  最通用，set/map 也能用
//     注意 front_inserter 會讓結果順序反轉（每個新元素都插到最前面）。
//
// (B) 效能代價：back_inserter 讓 reserve 失去機會
//     copy_if 事先不知道會挑出幾個元素，所以 back_inserter 只能一個一個
//     push_back，過程中可能觸發多次擴容與搬移。
//     若你能估算上界，先 destination.reserve(source.size()) 可以避免全部擴容。
//     這在資料量大時是有意義的最佳化 —— 本檔的 main 有實測示範。
//
// (C) copy_if 的回傳值常被忽略
//     copy_if 回傳「輸出端寫完之後的位置」。搭配一般迭代器時，
//     這個回傳值就是你判斷「實際寫了幾個」的唯一依據：
//         auto it = std::copy_if(src.begin(), src.end(), dst.begin(), pred);
//         dst.resize(it - dst.begin());     // 把多餘的截掉
//     搭配 back_inserter 時回傳值沒什麼用（容器大小自己知道）。
//
// 【注意事項 Pay Attention】
//   1. 別把 destination.begin() 當輸出端，除非你已經確保容量足夠
//      （例如先 resize）。否則就是越界的未定義行為。
//   2. copy_if 是 C++11 才加入的。C++98 只有 remove_copy_if
//      （語意相反：複製「不符合」的）。若要維護老專案要注意這點。
//   3. 述詞（predicate）不應該有副作用，也不應該修改元素。
//      標準允許實作以未指定的次數、未指定的順序呼叫述詞。
//   4. back_inserter 持有的是容器的指標。若在複製過程中另一條路徑
//      也在改動同一個容器，行為是未定義的。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】STL 的可組合性與迭代器配接器
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::copy_if 的第三個參數如果直接傳 dst.begin() 而 dst 是空的，
//        會發生什麼事？
//     答：未定義行為。copy_if 會對 dst.begin() 做 `*it = value; ++it;`，
//         但空 vector 的 begin() == end()，第一次寫入就已經越界。
//         演算法完全不知道容器容量，也不會幫你擴容。
//         正解是用 std::back_inserter(dst)，或先 dst.resize(足夠大小)。
//     追問：back_inserter 是怎麼做到的？
//         → 它回傳一個 back_insert_iterator，重載 operator*/=/++，
//           把 `*it = v` 翻譯成 `c.push_back(v)`，++ 則是 no-op。
//
// 🔥 Q2. lambda 和函式指標傳進演算法，效能上有什麼差別？
//     答：lambda 是獨一無二的匿名類別型別，演算法具現化時就知道
//         operator() 的完整內容，可以 inline 進迴圈。
//         函式指標是執行期的值，只能做間接呼叫，無法 inline。
//         這就是 std::sort 快過 qsort 的同一個原因。
//     追問：那 std::function 呢？
//         → std::function 是型別抹除的包裝，內部同樣是間接呼叫，
//           效能接近函式指標。需要 inline 就別把 lambda 包進 std::function。
//
// ⚠️ 陷阱. 「用了 back_inserter，就不必擔心效能了」——這句話錯在哪？
//     答：back_inserter 解決的是「正確性」（不會越界），不是「效能」。
//         它讓 copy_if 只能一次 push_back 一個元素，過程中可能觸發
//         多次重新配置與搬移（libstdc++ 是 2× 成長，實作定義）。
//         若能估出上界，先 reserve 可以把全部擴容省掉。
//     為什麼會錯：把「安全」誤當成「最佳」。STL 的預設選擇通常優先
//         保證正確性與泛用性，效能要靠使用者提供額外資訊（reserve、
//         move、自訂配置器）去補。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <deque>       // front_inserter 示範需要（vector 沒有 push_front）
#include <algorithm>
#include <iterator>
#include <string>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔刻意不放
//   理由：copy_if + back_inserter 的語意是「篩選出一份新資料」，
//   而 LeetCode 的陣列題（27 Remove Element、26 Remove Duplicates、
//   283 Move Zeroes …）幾乎都明文要求「就地修改、O(1) 額外空間」，
//   正好是 copy_if 做不到的事。那些題對應的是 std::remove_if
//   （erase-remove 慣用法），會在第 15 課專門處理。
//   在這裡硬塞一題「先 copy_if 到新 vector 再複製回去」的解法，
//   不但違反題目條件，還會讓讀者對這兩個演算法的分工產生錯誤印象，
//   所以寧可從缺。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例 1】從一批日誌中篩出等級達到門檻的行
//   情境：服務吐出的 log 混雜 DEBUG/INFO/WARN/ERROR，值班時只想看
//         WARN 以上的行，丟進告警通道。
//   為什麼用到本主題：這是 copy_if 的原型用途——輸入一段序列、
//     一個述詞、輸出一份新序列。而且述詞需要「捕捉外部變數」
//     （門檻等級由呼叫端決定），正好示範 lambda 相對函式指標的優勢：
//     函式指標無法攜帶 minLevel 這個狀態。
// -----------------------------------------------------------------------------
enum class Level { Debug = 0, Info = 1, Warn = 2, Error = 3 };

struct LogLine {
    Level       level;
    std::string text;
};

const char* levelName(Level l) {
    switch (l) {
        case Level::Debug: return "DEBUG";
        case Level::Info:  return "INFO";
        case Level::Warn:  return "WARN";
        case Level::Error: return "ERROR";
    }
    return "?";
}

std::vector<LogLine> filterByLevel(const std::vector<LogLine>& all, Level minLevel) {
    std::vector<LogLine> out;
    out.reserve(all.size());          // 上界已知：最多全中，先配置好省掉擴容
    std::copy_if(all.begin(), all.end(), std::back_inserter(out),
                 [minLevel](const LogLine& l) {      // 捕捉 minLevel —— 函式指標做不到
                     return static_cast<int>(l.level) >= static_cast<int>(minLevel);
                 });
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】不落地直接輸出：copy_if 接 ostream_iterator
//   情境：把篩選結果直接寫到輸出串流（或檔案），中間不建立任何容器。
//   為什麼用到本主題：這示範了「輸出端」也是一個可抽換的維度——
//     把 back_inserter 換成 ostream_iterator，同一個 copy_if
//     就從「填容器」變成「印到螢幕」，演算法一個字都沒改。
// -----------------------------------------------------------------------------
void printEvenTo(std::ostream& os, const std::vector<int>& src) {
    std::copy_if(src.begin(), src.end(),
                 std::ostream_iterator<int>(os, " "),   // 輸出端換成串流
                 [](int n) { return n % 2 == 0; });
}

int main() {
    std::cout << "=== 原始示範：copy_if + back_inserter + lambda ===\n";

    std::vector<int> source = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::vector<int> destination;

    // 組合多個 STL 元件：
    // 1. copy_if - 演算法
    // 2. back_inserter - 迭代器配接器
    // 3. lambda - 函數物件
    std::copy_if(
        source.begin(),                     // 來源開始
        source.end(),                       // 來源結束
        std::back_inserter(destination),    // 目標（自動擴展）
        [](int n) { return n % 2 == 0; }    // 條件：偶數
    );

    std::cout << "偶數: ";
    for (int n : destination) {
        std::cout << n << " ";
    }
    std::cout << std::endl;

    std::cout << "\n=== 換掉輸出端：同一個 copy_if，直接印到串流 ===\n";
    std::cout << "偶數: ";
    printEvenTo(std::cout, source);
    std::cout << "\n";

    std::cout << "\n=== 換掉輸出端：front_inserter 會讓順序反轉 ===\n";
    std::deque<int> dq;   // vector 沒有 push_front，所以這裡用 deque
    std::copy_if(source.begin(), source.end(), std::front_inserter(dq),
                 [](int n) { return n % 2 == 0; });
    std::cout << "front_inserter 結果: ";
    for (int n : dq) std::cout << n << " ";
    std::cout << "  ← 每個新元素都插到最前面，所以是反序\n";

    std::cout << "\n=== back_inserter 不會幫你 reserve：實測擴容次數 ===\n";
    {
        std::vector<int> big(10000);
        for (size_t i = 0; i < big.size(); ++i) big[i] = static_cast<int>(i);

        std::vector<int> noReserve;
        size_t reallocs = 0, lastCap = noReserve.capacity();
        std::copy_if(big.begin(), big.end(), std::back_inserter(noReserve),
                     [](int n) { return n % 2 == 0; });
        // 事後看不到過程，改用逐步觀察的方式重跑一次
        std::vector<int> probe;
        for (int n : big) {
            if (n % 2 == 0) {
                probe.push_back(n);
                if (probe.capacity() != lastCap) { lastCap = probe.capacity(); ++reallocs; }
            }
        }
        std::vector<int> withReserve;
        withReserve.reserve(big.size());
        size_t capBefore = withReserve.capacity();
        std::copy_if(big.begin(), big.end(), std::back_inserter(withReserve),
                     [](int n) { return n % 2 == 0; });

        std::cout << "不 reserve：最終 size=" << noReserve.size()
                  << "，過程中重新配置 " << reallocs << " 次\n";
        std::cout << "先 reserve：最終 size=" << withReserve.size()
                  << "，capacity 全程維持 " << capBefore
                  << "（重新配置 0 次）\n";
        std::cout << "（libstdc++ 的成長倍率是 2×，屬實作定義；MSVC 是 1.5×）\n";
    }

    std::cout << "\n=== 日常實務 1：篩出 WARN 以上的日誌 ===\n";
    std::vector<LogLine> logs = {
        {Level::Debug, "cache warm-up started"},
        {Level::Info,  "listening on :8080"},
        {Level::Warn,  "connection pool 80% full"},
        {Level::Error, "upstream timeout after 3 retries"},
        {Level::Info,  "health check ok"},
        {Level::Error, "failed to write checkpoint"},
    };
    auto alerts = filterByLevel(logs, Level::Warn);
    std::cout << "原始 " << logs.size() << " 行，篩出 " << alerts.size() << " 行：\n";
    for (const auto& l : alerts) {
        std::cout << "  [" << levelName(l.level) << "] " << l.text << "\n";
    }

    std::cout << "\n=== 日常實務 2：同一份 log 換個門檻，述詞捕捉不同的值 ===\n";
    auto errorsOnly = filterByLevel(logs, Level::Error);
    std::cout << "只要 ERROR，篩出 " << errorsOnly.size() << " 行：\n";
    for (const auto& l : errorsOnly) {
        std::cout << "  [" << levelName(l.level) << "] " << l.text << "\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第一課：STL 的歷史與設計哲學4.cpp -o demo4

// === 預期輸出 ===
// === 原始示範：copy_if + back_inserter + lambda ===
// 偶數: 2 4 6 8 10
//
// === 換掉輸出端：同一個 copy_if，直接印到串流 ===
// 偶數: 2 4 6 8 10
//
// === 換掉輸出端：front_inserter 會讓順序反轉 ===
// front_inserter 結果: 10 8 6 4 2   ← 每個新元素都插到最前面，所以是反序
//
// === back_inserter 不會幫你 reserve：實測擴容次數 ===
// 不 reserve：最終 size=5000，過程中重新配置 14 次
// 先 reserve：最終 size=5000，capacity 全程維持 10000（重新配置 0 次）
// （libstdc++ 的成長倍率是 2×，屬實作定義；MSVC 是 1.5×）
//
// === 日常實務 1：篩出 WARN 以上的日誌 ===
// 原始 6 行，篩出 3 行：
//   [WARN] connection pool 80% full
//   [ERROR] upstream timeout after 3 retries
//   [ERROR] failed to write checkpoint
//
// === 日常實務 2：同一份 log 換個門檻，述詞捕捉不同的值 ===
// 只要 ERROR，篩出 2 行：
//   [ERROR] upstream timeout after 3 retries
//   [ERROR] failed to write checkpoint
