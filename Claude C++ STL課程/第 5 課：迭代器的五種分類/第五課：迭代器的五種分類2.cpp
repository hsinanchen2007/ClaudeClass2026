// =============================================================================
//  第五課：迭代器的五種分類 2  —  Input Iterator 的限制：單次走訪（single-pass）
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<iterator>（istream_iterator）、<sstream>（istringstream）
//   Input Iterator 的完整能力清單（標準要求）：
//     可以：*it（**只讀**）、++it / it++、it == / != 另一個同型別迭代器、可複製
//     不行：--it、it + n、it - n、it[n]、it1 < it2、透過 *it 寫入
//   最關鍵的限制：**只保證單次走訪（single-pass）**
//     —— ++it 之後，先前複製的副本即失效。
//   標準版本：分類自 C++98；C++20 的 ranges 用 concept 重新定義了這些要求
//             （std::input_iterator），語意大致相同但檢查發生在編譯期且訊息可讀。
//   複雜度：每次 ++ 觸發一次 operator>>。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼要有「五種分類」這件事本身】
//   演算法對迭代器有不同的能力需求：
//       std::find      只需要「能讀、能往前」          → Input 就夠
//       std::replace   需要「能寫」且能重複走訪        → Forward
//       std::reverse   需要「能往回走」                → Bidirectional
//       std::sort      需要「能跳躍、能算距離」        → Random Access
//   如果不分類，演算法就得在文件裡用自然語言描述需求，編譯器無從檢查。
//   分類之後，需求變成型別系統的一部分：
//   用能力不足的迭代器 → **編譯錯誤**，而不是執行期出錯或效能悄悄退化。
//   這是 STL 把「文件約定」升級成「編譯期保證」的關鍵設計。
//
// 【2. single-pass 到底是什麼意思】
//   標準的規定是：對 Input Iterator，一旦執行 ++it，
//   所有「it 的先前副本」都不再保證可解參考（也不保證仍等於任何東西）。
//       auto a = it;
//       ++it;
//       *a;         // 未定義行為
//   為什麼？因為 istream_iterator 的實作只保存「串流指標 + 目前讀到的值」，
//   ++ 會直接從串流讀下一個值覆蓋掉舊的 —— 舊副本裡的值早就沒了。
//   這正是「串流」的本質：資料流過去就不在了。
//
// 【3. 這個限制在實務上會擋掉什麼】
//   任何需要「回頭再看一次」的操作全部不能用：
//     - 先數一遍長度，再走一遍處理  → 不行（數完就沒了）
//     - std::sort / std::reverse     → 不行（需要更強的迭代器）
//     - 兩個迭代器同時走訪比較       → 不行（沒有 multi-pass 保證）
//   標準解法只有一個：**先把資料吸進容器**。
//       std::vector<int> v(istream_iterator<int>(iss), istream_iterator<int>());
//   代價是記憶體，換來的是完整的 Random Access 能力。
//
// 【4. 為什麼 Output Iterator 是「並列」而非「更弱」】
//   五種分類的階層圖常被畫成一條直線，但實際上開頭是分叉的：
//       Input ──┐
//               ├─→ Forward ─→ Bidirectional ─→ Random Access
//       Output ─┘
//   Input 只能讀不能寫，Output 只能寫不能讀 —— 兩者能力**不相包含**。
//   Forward 才是第一個「同時能讀能寫且保證 multi-pass」的分類。
//
// 【概念補充 Concept Deep Dive】
//   為什麼標準要區分「單次走訪」與「多次走訪」，而不是乾脆全部要求 multi-pass？
//   因為那會讓「串流」這類資料源根本無法納入 STL 生態。
//   Alexander Stepanov 的設計哲學是「找出每個演算法真正需要的最小要求」——
//   std::find 只需要單次走訪，所以它的要求就寫成 Input Iterator，
//   於是它同時能用在 vector、list 與 std::cin 上。
//   如果 find 隨手要求了 Forward，串流就被排除在外了，而那個要求根本用不到。
//   實務準則：**寫泛型函式時，只要求你真正用到的最低等級**，
//   這樣才能服務最多的呼叫端。這也是為什麼看 cppreference 時，
//   演算法簽名中的樣板參數名稱（InputIt / ForwardIt / RandomIt）
//   不是隨便取的 —— 那是規格的一部分。
//
// 【注意事項 Pay Attention】
//   1. Input Iterator 的副本在 ++ 之後即失效（single-pass），不可再解參考。
//   2. 不支援 --、+n、[]、<；誤用是編譯錯誤（這是好事）。
//   3. *it 只能讀不能寫；要寫請用 Output Iterator。
//   4. std::distance 對 Input Iterator 會**消耗掉整條串流**，
//      算完之後就沒有資料可讀了 —— 這是最容易踩的坑。
//   5. 要多次走訪唯一的辦法是先存進容器。
//   6. istream_iterator 建構時就會讀第一個值，這是有副作用的宣告。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Input Iterator 的限制
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 STL 要把迭代器分成五類？只有一種不行嗎？
//     答：因為不同演算法的真實需求不同。分類之後，「這個演算法需要什麼能力」
//         變成型別系統的一部分 —— 用能力不足的迭代器會**編譯錯誤**，
//         而不是執行期爆炸或效能悄悄從 O(1) 退化成 O(N)。
//         同時它也讓「只需要弱能力」的演算法（如 std::find）
//         能同時服務 vector、list 與 std::cin。
//     追問：那五種分類是一條直線嗎？
//           → 不是，開頭是分叉的：Input 與 Output 並列（一個只讀、一個只寫，
//             能力互不包含），兩者之上才匯流成 Forward，
//             再往上是 Bidirectional、Random Access。
//
// 🔥 Q2. 「單次走訪（single-pass）」具體限制了什麼？
//     答：一旦執行 ++it，所有先前複製的副本都不再保證可用。
//         因為 istream_iterator 內部只存「串流指標 + 目前的值」，
//         ++ 會直接從串流讀下一個值覆蓋掉舊的 —— 舊值早就流走了。
//         後果是：不能先數一遍再走一遍、不能用兩個迭代器同時走訪比較、
//         不能 sort 或 reverse。
//     追問：那實務上要怎麼繞過？
//           → 先用範圍建構子把資料吸進容器（vector），
//             之後就擁有完整的 Random Access 能力。代價是記憶體。
//
// ⚠️ 陷阱. 想先知道串流裡有幾個數字再決定怎麼處理，於是寫了
//          auto n = std::distance(first, last);
//          然後接著讀取 —— 為什麼一個數字都讀不到？
//     答：因為 std::distance 對 Input Iterator 的實作就是「一路 ++ 到 last」，
//         而每一次 ++ 都真的從串流讀走了一個值。
//         distance 回傳正確的個數，但**整條串流已經被消耗光了**，
//         之後 first 已經等於 last，再讀什麼都沒有。
//     為什麼會錯：把 distance 想成「查詢長度」這種無害的唯讀操作。
//         對 Random Access 迭代器它確實是無害的 O(1) 減法；
//         但對 Input Iterator 它是 O(N) 且**有破壞性**的走訪。
//         同一個函式在不同迭代器分類下有截然不同的行為與代價 ——
//         這正是為什麼理解分類這件事本身很重要。
//         正解：先吸進容器，再對容器呼叫 size()。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <sstream>
#include <iterator>
#include <vector>
#include <string>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】不強加。
//   理由：本檔講的是「Input Iterator 不能做什麼」——一個關於**型別能力邊界**
//         的觀念，不是解題技巧。LeetCode 的輸入一律是已經在記憶體裡的
//         vector / string（等同 Random Access Iterator），
//         永遠不會遇到單次走訪的限制。
//         這個限制真正會咬人的地方是處理串流與大檔案，
//         也就是下面實務範例的場景。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】處理超大 log 檔：為什麼不能「先數一遍再處理」
//   情境：一個 50GB 的 log 檔，需求是「統計錯誤行數，並印出前 3 筆錯誤」。
//         直覺寫法是「先掃一遍算總數，再掃一遍取前 3 筆」——
//         對串流而言這是行不通的（而且對大檔案，讀兩遍也慢一倍）。
//   為什麼用到本主題：這正是 single-pass 限制在真實工作中的樣子。
//         正解是**單趟走訪同時完成所有統計**，不回頭。
//         這裡用 istringstream 模擬檔案串流，讓輸出可重現。
// -----------------------------------------------------------------------------
struct LogSummary {
    int                      total_lines  = 0;
    int                      error_count  = 0;
    std::vector<std::string> first_errors;   // 只保留前 3 筆
};

LogSummary scanLogOnce(std::istream& in, std::size_t keep) {
    LogSummary s;
    std::string line;
    // 單趟走訪：一邊數、一邊挑，絕不回頭
    while (std::getline(in, line)) {
        ++s.total_lines;
        if (line.find("[ERROR]") != std::string::npos) {
            ++s.error_count;
            if (s.first_errors.size() < keep) s.first_errors.push_back(line);
        }
    }
    return s;
}

int main() {
    // Input Iterator 的限制示範, 這裡使用 istringstream 模擬輸入串流
    std::istringstream iss("10 20 30");
    // istream_iterator 是 Input Iterator
    std::istream_iterator<int> it(iss);

    // 可以讀取
    std::cout << "第一次讀取: " << *it << std::endl;

    // 可以前進
    ++it;
    std::cout << "前進後讀取: " << *it << std::endl;

    // 關鍵限制：不能回頭！
    // --it;  // 編譯錯誤：Input Iterator 不支援 --

    // 也不能跳躍
    // it + 1;  // 編譯錯誤：Input Iterator 不支援 +

    // 一旦遍歷過，就不能重新遍歷同一個串流
    // （除非重新建立串流）

    // --- 實際演示：distance 會把串流吃光 ---
    std::cout << "\n=== distance 對 Input Iterator 有破壞性 ===" << std::endl;
    {
        std::istringstream s1("1 2 3 4 5");
        std::istream_iterator<int> first(s1), last;
        auto n = std::distance(first, last);       // 一路 ++ 到底，資料被讀光
        std::cout << "  distance 回報個數: " << n << std::endl;
        std::cout << "  接著再讀取: ";
        int extra = 0;
        for (std::istream_iterator<int> again(s1); again != last; ++again) ++extra;
        std::cout << extra << " 個  ← 串流已經被 distance 消耗光了" << std::endl;
    }

    // --- 對照：Random Access 的 distance 是無害的 O(1) ---
    std::cout << "\n=== 對照：vector 的 distance 無害且 O(1) ===" << std::endl;
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        auto n = std::distance(v.begin(), v.end());
        std::cout << "  distance = " << n << std::endl;
        std::cout << "  之後仍可完整走訪: ";
        for (int x : v) std::cout << x << " ";
        std::cout << "  ← 完全沒被破壞" << std::endl;
    }

    // --- 正解：先吸進容器，就取得完整能力 ---
    std::cout << "\n=== 正解：先存進容器 ===" << std::endl;
    {
        std::istringstream s2("7 3 9 1 5");
        std::istream_iterator<int> first(s2), last;
        std::vector<int> v(first, last);           // 單趟讀完，存起來

        std::cout << "  個數: " << v.size() << std::endl;
        std::cout << "  正向: ";
        for (int x : v) std::cout << x << " ";
        std::cout << std::endl;
        std::cout << "  反向: ";
        for (auto rit = v.rbegin(); rit != v.rend(); ++rit) std::cout << *rit << " ";
        std::cout << std::endl;
        std::cout << "  → 想走幾遍都可以，因為 vector 是 Random Access" << std::endl;
    }

    std::cout << "\n=== 日常實務：大 log 檔的單趟掃描 ===" << std::endl;
    {
        std::istringstream fake_file(
            "2026-07-20 09:00:01 [INFO]  service started\n"
            "2026-07-20 09:00:05 [ERROR] db connection refused\n"
            "2026-07-20 09:00:07 [WARN]  retry in 5s\n"
            "2026-07-20 09:00:12 [ERROR] db connection refused\n"
            "2026-07-20 09:01:00 [INFO]  db connected\n"
            "2026-07-20 09:05:33 [ERROR] payment gateway timeout\n"
            "2026-07-20 09:06:00 [ERROR] order 5566 rollback\n");

        LogSummary s = scanLogOnce(fake_file, 3);
        std::cout << "  總行數  : " << s.total_lines << std::endl;
        std::cout << "  錯誤數  : " << s.error_count << std::endl;
        std::cout << "  前 3 筆錯誤:" << std::endl;
        for (const std::string& e : s.first_errors) {
            std::cout << "    " << e << std::endl;
        }
        std::cout << "  → 一趟走訪同時完成計數與取樣，不需要（也不能）回頭"
                  << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第五課：迭代器的五種分類2.cpp -o demo2

// === 預期輸出 ===
// 第一次讀取: 10
// 前進後讀取: 20
//
// === distance 對 Input Iterator 有破壞性 ===
//   distance 回報個數: 5
//   接著再讀取: 0 個  ← 串流已經被 distance 消耗光了
//
// === 對照：vector 的 distance 無害且 O(1) ===
//   distance = 5
//   之後仍可完整走訪: 1 2 3 4 5   ← 完全沒被破壞
//
// === 正解：先存進容器 ===
//   個數: 5
//   正向: 7 3 9 1 5
//   反向: 5 1 9 3 7
//   → 想走幾遍都可以，因為 vector 是 Random Access
//
// === 日常實務：大 log 檔的單趟掃描 ===
//   總行數  : 7
//   錯誤數  : 4
//   前 3 筆錯誤:
//     2026-07-20 09:00:05 [ERROR] db connection refused
//     2026-07-20 09:00:12 [ERROR] db connection refused
//     2026-07-20 09:05:33 [ERROR] payment gateway timeout
//   → 一趟走訪同時完成計數與取樣，不需要（也不能）回頭
