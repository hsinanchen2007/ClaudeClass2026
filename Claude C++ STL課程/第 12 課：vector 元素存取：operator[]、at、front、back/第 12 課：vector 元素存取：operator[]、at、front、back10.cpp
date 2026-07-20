// =============================================================================
//  第 12 課：vector 元素存取 10  —  反向遍歷與 size_type 的無號地雷
// =============================================================================
//
// 【主題資訊 Information】
//   std::vector<T>::size_type size() const noexcept;
//   標頭檔：<vector>
//   標準版本：C++98（noexcept 標註自 C++11）。
//   關鍵事實：size_type 是【無號整數型別】（在 libstdc++ 上是 std::size_t，
//             64-bit 平台為 unsigned long，64 位元寬）。
//   本機實測：std::vector<int>().max_size() == 2305843009213693951
//             （= 2^61 - 1，因為 sizeof(int)==4 且要留給 ptrdiff_t）
//   無號運算的規則：溢位/下溢是【良好定義的模 2^N 環繞】，不是未定義行為。
//             所以 size_t(0) - 1 == 18446744073709551615，這是標準保證的結果。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼這個迴圈永遠不會結束】
//       for (size_t i = v.size() - 1; i >= 0; --i)
//   兩個問題疊在一起：
//     ① 條件 `i >= 0` 對無號數【恆為真】。無號數不可能是負的，
//        編譯器甚至會警告你這個比較永遠成立（-Wtype-limits）。
//        所以迴圈不會因為條件失敗而結束。
//     ② 當 i 減到 0 之後再 --i，會環繞成 SIZE_MAX（18446744073709551615），
//        接著 v[巨大數] 就是嚴重的越界存取——未定義行為。
//   更陰險的是 v 為空時：v.size() - 1 一開始就是 SIZE_MAX，
//   第一次迭代就直接越界。
//
// 【2. 為什麼 size_type 要選無號】
//   這是 C++98 的設計決定，理由在當時是合理的：
//     * 大小本質上非負，無號能表達兩倍的範圍。
//     * 16-bit 時代那多出來的一個 bit 很值錢。
//     * 與 sizeof、malloc 的 size_t 保持一致。
//   但代價就是本課這個坑。Bjarne Stroustrup 本人與 C++ 核心指引
//   都公開表示過「當年用無號是個錯誤」；Google C++ Style Guide 與
//   LLVM 等專案偏好用有號索引，正是為了避開這類環繞問題。
//   不過標準已經無法改了——改了會破壞全世界的 ABI。
//
// 【3. 三種正確的反向遍歷寫法】
//   (a) 索引下移一格（本檔原始示範）
//         for (size_t i = v.size(); i > 0; --i) use(v[i - 1]);
//       條件是 i > 0，不會環繞；存取時才 -1。空 vector 時迴圈直接不執行。
//   (b) 反向迭代器（最推薦，語意最清楚）
//         for (auto it = v.rbegin(); it != v.rend(); ++it) use(*it);
//       完全不出現算術，也不出現無號型別。
//   (c) 反向 range-for（C++20 的 std::views::reverse，或自訂 adapter）
//         for (int x : v | std::views::reverse) use(x);
//       C++20 才有，本檔用 C++17 所以只提不用。
//
// 【4. 這個坑的一般形式：任何 size() - k 都要小心】
//   不只反向迴圈。下面每一行都有同樣的問題：
//       if (v.size() - 1 > 0)            // v 空時 → SIZE_MAX > 0 → 真
//       for (size_t i = 0; i < v.size() - 1; ++i)   // v 空時 → 跑 SIZE_MAX 次
//       if (i < v.size() - n)            // 只要 n > size 就爆
//   通用修法是【把減法搬到不等式的另一邊，變成加法】：
//       if (v.size() > 1)                // 取代 v.size() - 1 > 0
//       for (size_t i = 0; i + 1 < v.size(); ++i)   // 取代 i < v.size() - 1
//   加法在這裡不會環繞（除非 i 已接近 SIZE_MAX，實務上不可能）。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 無號環繞是「良好定義」，不是未定義行為
//     這點常被混淆。C++ 標準明訂無號整數的算術是模 2^N 運算，
//     溢位/下溢都有確定結果。所以 size_t(0) - 1 == SIZE_MAX 是
//     可以依賴的事實，本檔敢直接印出來。
//     反過來，【有號】整數溢位才是未定義行為（INT_MAX + 1 不保證任何結果）。
//     真正的未定義行為發生在下一步：用那個環繞出來的巨大值去索引 vector。
//
// (B) 有號/無號混合比較的隱式轉換
//       int i = -1;
//       if (i < v.size())      // 危險！
//     這裡 i 會先被轉成 size_t，-1 變成 SIZE_MAX，
//     於是「-1 < size()」竟然是 false。
//     編譯器會發出 -Wsign-compare 警告——這個警告【不要關掉】。
//     本檔的編譯指令帶 -Wall -Wextra，正是為了讓這類問題現形。
//
// (C) 為什麼 rbegin() 不會有這個問題
//     reverse_iterator 內部持有的是「正向迭代器」，
//     解參考時做的是 *(current - 1)。
//     所以 rbegin() 實際存的是 end()，rend() 存的是 begin()，
//     整個比較都在迭代器層面進行，完全不涉及無號索引算術。
//     這也是為什麼 `*v.rend()` 是未定義行為——它會去解 *(begin()-1)。
//
// 【注意事項 Pay Attention】
//   1. `for (size_t i = v.size() - 1; i >= 0; --i)` 是無窮迴圈 +
//      越界存取。條件恆真，且 v 為空時第一輪就爆。
//   2. 無號環繞本身是良好定義的；真正的未定義行為是拿環繞後的
//      巨大值去索引容器。
//   3. 任何 `v.size() - k` 都要先確認 size() >= k，
//      或改寫成加法形式（i + k < v.size()）。
//   4. 不要關掉 -Wsign-compare。有號/無號混比是這類 bug 的溫床。
//   5. 反向遍歷優先用 rbegin()/rend()，語意清楚且完全避開算術。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】size_type 的無號陷阱
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. `for (size_t i = v.size() - 1; i >= 0; --i)` 有什麼問題？
//     答：兩個。① `i >= 0` 對無號數恆為真，迴圈條件永遠不會失敗；
//         ② i 減到 0 之後再 --，會環繞成 SIZE_MAX，
//            接著 v[SIZE_MAX] 是越界存取（未定義行為）。
//         此外若 v 是空的，v.size() - 1 一開始就是 SIZE_MAX，
//         第一輪迭代就直接爆掉。
//     追問：正確的寫法有哪些？
//         → ① for (size_t i = v.size(); i > 0; --i) use(v[i-1]);
//           ② for (auto it = v.rbegin(); it != v.rend(); ++it) use(*it);
//             後者最推薦——完全不出現無號算術。
//
// 🔥 Q2. 為什麼 vector 的 size_type 要用無號型別？現在還算是好設計嗎？
//     答：C++98 的決定：大小本質非負、無號多一個 bit 的範圍
//         （16-bit 時代很重要）、與 size_t 一致。
//         但現在普遍被視為設計失誤——Bjarne 本人與 C++ 核心指引
//         都表達過這個看法，Google/LLVM 等專案偏好有號索引。
//         標準無法改，因為會破壞全世界的 ABI。
//     追問：C++20 有沒有提供解法？
//         → 有 std::ssize()（回傳有號的 ptrdiff_t），
//           讓你寫 for (auto i = std::ssize(v) - 1; i >= 0; --i) 而不會環繞。
//
// ⚠️ 陷阱. `int i = -1; if (i < v.size()) { … }`
//         v.size() 是 3，所以 -1 < 3 應該成立、會進入分支——對嗎？
//     答：不對，這個條件是 false。有號與無號比較時，
//         有號的一方會被隱式轉成無號（整數提升的常見結果）：
//         int(-1) 轉成 size_t 就是 18446744073709551615，
//         它遠大於 3，所以條件不成立。
//         編譯器會給 -Wsign-compare 警告，那個警告不該被忽略。
//     為什麼會錯：直覺把 `<` 當成「數學上的小於」。
//         但 C++ 的比較運算子會先做「常用算術轉換」把兩邊統一型別，
//         而在 int 與 size_t（64-bit）之間，統一的方向是往無號走，
//         負數就在轉換途中變成了天文數字。
//         這個坑和反向迴圈是同一個根源：無號型別沒有負數的概念。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <string>
#include <cstdint>   // SIZE_MAX

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 66. Plus One
//   題目：用陣列表示一個非負整數（每個元素是一位數，最高位在前），
//         把它加一後回傳。例如 [1,2,3] -> [1,2,4]；[9,9] -> [1,0,0]。
//   為什麼用到本主題：這題天生就是「從最後一位往前掃並處理進位」，
//     正是反向索引遍歷的教科書應用。而且它的迴圈條件必須寫對——
//     若寫成 `for (size_t i = digits.size() - 1; i >= 0; --i)`，
//     全部是 9 的輸入（[9,9,9]）會讓 i 環繞成 SIZE_MAX 而越界。
//     這裡用 `i > 0` 搭配 `i - 1` 的安全形式。
//   複雜度：O(N) 時間；只有全 9 的情況才需要 O(N) 額外空間。
// -----------------------------------------------------------------------------
std::vector<int> plusOne(std::vector<int> digits) {
    // 從最後一位往前：條件是 i > 0，存取時才 i-1，永遠不會環繞
    for (size_t i = digits.size(); i > 0; --i) {
        if (digits[i - 1] < 9) {
            ++digits[i - 1];
            return digits;            // 沒有進位，直接結束
        }
        digits[i - 1] = 0;            // 這一位是 9 → 歸零，繼續往前進位
    }
    // 走到這裡代表全部都是 9（例如 999 -> 1000）
    digits.insert(digits.begin(), 1);
    return digits;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】從日誌尾端往前找「最近一次的錯誤」
//   情境：服務出問題時，值班工程師最想知道的是「最後一筆 ERROR 是什麼」。
//         日誌是按時間順序附加的，所以要從尾端往前找，找到就停——
//         不必掃完整份幾十萬行的日誌。
//   為什麼用到本主題：這是反向遍歷最常見的實務動機（early exit）。
//     並且它必須正確處理「整份日誌沒有任何 ERROR」與「日誌是空的」
//     這兩個邊界；用錯迴圈寫法，空日誌會直接讓程式越界。
//   回傳：找到的行號（0-based）與內容；找不到時 found=false。
// -----------------------------------------------------------------------------
struct SearchResult {
    bool        found;
    size_t      lineNo;
    std::string text;
};

SearchResult findLastError(const std::vector<std::string>& log) {
    // 安全形式：i 從 size() 開始，條件 i > 0，存取用 i-1
    for (size_t i = log.size(); i > 0; --i) {
        const std::string& line = log[i - 1];
        if (line.find("[ERROR]") != std::string::npos) {
            return {true, i - 1, line};
        }
    }
    return {false, 0, ""};
}

int main() {
    std::vector<int> v = {10, 20, 30};

    std::cout << "=== 原始示範：安全的反向遍歷 ===\n";

    // 危險！當 v.size() 是 0 時，v.size() - 1 會環繞成超大數字
    // for (size_t i = v.size() - 1; i >= 0; --i) {  // 條件永遠為真！
    //     std::cout << v[i] << std::endl;
    // }

    // 安全的反向遍歷方式
    for (size_t i = v.size(); i > 0; --i) {
        std::cout << v[i - 1] << " ";  // 30 20 10
    }
    std::cout << std::endl;

    std::cout << "\n=== 無號環繞是「良好定義」的，直接印給你看 ===\n";
    {
        std::vector<int> empty;
        // 注意：計算 size()-1 本身完全合法（無號環繞有明確定義），
        // 未定義行為發生在「拿這個值去索引」的那一步——所以我們只印不索引。
        size_t bad = empty.size() - 1;
        std::cout << "empty.size()          = " << empty.size() << "\n";
        std::cout << "empty.size() - 1      = " << bad << "\n";
        std::cout << "SIZE_MAX              = " << SIZE_MAX << "\n";
        std::cout << "兩者相等: " << std::boolalpha << (bad == SIZE_MAX) << "\n";
        std::cout << "→ 這個環繞是標準保證的（無號 = 模 2^N 運算），不是 UB。\n";
        std::cout << "  真正的 UB 是接下來拿它去做 empty[bad] —— 本檔不會這樣做。\n";
    }

    std::cout << "\n=== 有號/無號混合比較的隱式轉換 ===\n";
    {
        int i = -1;
        // 直接寫 i < v.size() 會觸發 -Wsign-compare 警告，
        // 這裡明確轉型示範「編譯器實際上做了什麼」
        std::cout << "i = " << i << ", v.size() = " << v.size() << "\n";
        std::cout << "數學上 -1 < 3 嗎? true\n";
        std::cout << "但 (size_t)i = " << static_cast<size_t>(i) << "\n";
        std::cout << "所以 (size_t)i < v.size() 是: "
                  << (static_cast<size_t>(i) < v.size()) << "\n";
        std::cout << "→ 這就是 -Wsign-compare 警告要提醒你的事。別關掉它。\n";
    }

    std::cout << "\n=== 三種反向遍歷寫法的對照 ===\n";
    {
        std::cout << "(a) 索引下移一格 : ";
        for (size_t i = v.size(); i > 0; --i) std::cout << v[i - 1] << " ";
        std::cout << "\n";

        std::cout << "(b) 反向迭代器   : ";
        for (auto it = v.rbegin(); it != v.rend(); ++it) std::cout << *it << " ";
        std::cout << "  ← 推薦：完全沒有無號算術\n";

        // (c) C++20 的 std::views::reverse 這裡不示範（本檔用 C++17）
        std::cout << "(c) C++20 可用 v | std::views::reverse（本檔為 C++17，不示範）\n";
    }

    std::cout << "\n=== 空容器：兩種寫法的差別 ===\n";
    {
        std::vector<int> empty;
        std::cout << "安全寫法對空 vector 的迭代次數: ";
        int count = 0;
        for (size_t i = empty.size(); i > 0; --i) ++count;
        std::cout << count << "（正確地一次都不跑）\n";

        std::cout << "反向迭代器對空 vector 的迭代次數: ";
        count = 0;
        for (auto it = empty.rbegin(); it != empty.rend(); ++it) ++count;
        std::cout << count << "（同樣正確）\n";
        std::cout << "危險寫法 i = size()-1 會從 " << SIZE_MAX
                  << " 開始 → 第一輪就越界\n";
    }

    std::cout << "\n=== 「size() - k」的通用修法：改寫成加法 ===\n";
    {
        std::vector<int> data = {1, 2, 3, 4};
        std::vector<int> none;

        std::cout << "相鄰差值（data，4 個元素）: ";
        // 危險寫法: for (size_t i = 0; i < data.size() - 1; ++i)
        // 安全寫法: i + 1 < size()
        for (size_t i = 0; i + 1 < data.size(); ++i) {
            std::cout << (data[i + 1] - data[i]) << " ";
        }
        std::cout << "\n";

        std::cout << "同一段程式對空 vector 的迭代次數: ";
        int count = 0;
        for (size_t i = 0; i + 1 < none.size(); ++i) ++count;
        std::cout << count << "（安全）\n";
        std::cout << "若寫成 i < none.size() - 1，會變成 i < " << SIZE_MAX
                  << " → 跑到越界為止\n";
    }

    std::cout << "\n=== LeetCode 66 Plus One ===\n";
    {
        auto show = [](std::vector<int> in) {
            auto out = plusOne(in);
            std::cout << "  [";
            for (size_t i = 0; i < in.size(); ++i) std::cout << (i ? "," : "") << in[i];
            std::cout << "] -> [";
            for (size_t i = 0; i < out.size(); ++i) std::cout << (i ? "," : "") << out[i];
            std::cout << "]\n";
        };
        show({1, 2, 3});
        show({4, 3, 2, 1});
        show({9});
        show({9, 9, 9});      // 全 9：最容易寫出環繞 bug 的輸入
    }

    std::cout << "\n=== 日常實務：從日誌尾端找最近一次 ERROR ===\n";
    {
        std::vector<std::string> log = {
            "2026-07-20 09:00:01 [INFO]  service started",
            "2026-07-20 09:00:05 [ERROR] failed to connect to db",
            "2026-07-20 09:00:06 [INFO]  retrying",
            "2026-07-20 09:00:07 [ERROR] upstream timeout",
            "2026-07-20 09:00:09 [INFO]  recovered",
            "2026-07-20 09:00:12 [WARN]  pool 80% full",
        };

        auto r = findLastError(log);
        std::cout << "日誌共 " << log.size() << " 行\n";
        std::cout << "最近一次 ERROR: " << (r.found ? "找到" : "沒有") << "\n";
        if (r.found) {
            std::cout << "  行號(0-based): " << r.lineNo << "\n";
            std::cout << "  內容: " << r.text << "\n";
            std::cout << "  → 從尾端第 " << (log.size() - r.lineNo)
                      << " 行就找到了，不必掃完整份日誌\n";
        }

        // 邊界一：沒有任何 ERROR
        std::vector<std::string> clean = {"[INFO] ok", "[INFO] ok"};
        std::cout << "全部正常的日誌: "
                  << (findLastError(clean).found ? "找到" : "沒有 ERROR（正確）") << "\n";

        // 邊界二：空日誌 —— 這正是危險寫法會爆掉的地方
        std::vector<std::string> emptyLog;
        std::cout << "空日誌: "
                  << (findLastError(emptyLog).found ? "找到" : "沒有 ERROR（正確，且沒有越界）")
                  << "\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 12 課：vector 元素存取10.cpp -o demo10
// （-Wall -Wextra 會開啟 -Wsign-compare，本課的坑正是靠它現形）

// === 預期輸出 ===
// === 原始示範：安全的反向遍歷 ===
// 30 20 10
//
// === 無號環繞是「良好定義」的，直接印給你看 ===
// empty.size()          = 0
// empty.size() - 1      = 18446744073709551615
// SIZE_MAX              = 18446744073709551615
// 兩者相等: true
// → 這個環繞是標準保證的（無號 = 模 2^N 運算），不是 UB。
//   真正的 UB 是接下來拿它去做 empty[bad] —— 本檔不會這樣做。
//
// === 有號/無號混合比較的隱式轉換 ===
// i = -1, v.size() = 3
// 數學上 -1 < 3 嗎? true
// 但 (size_t)i = 18446744073709551615
// 所以 (size_t)i < v.size() 是: false
// → 這就是 -Wsign-compare 警告要提醒你的事。別關掉它。
//
// === 三種反向遍歷寫法的對照 ===
// (a) 索引下移一格 : 30 20 10
// (b) 反向迭代器   : 30 20 10   ← 推薦：完全沒有無號算術
// (c) C++20 可用 v | std::views::reverse（本檔為 C++17，不示範）
//
// === 空容器：兩種寫法的差別 ===
// 安全寫法對空 vector 的迭代次數: 0（正確地一次都不跑）
// 反向迭代器對空 vector 的迭代次數: 0（同樣正確）
// 危險寫法 i = size()-1 會從 18446744073709551615 開始 → 第一輪就越界
//
// === 「size() - k」的通用修法：改寫成加法 ===
// 相鄰差值（data，4 個元素）: 1 1 1
// 同一段程式對空 vector 的迭代次數: 0（安全）
// 若寫成 i < none.size() - 1，會變成 i < 18446744073709551615 → 跑到越界為止
//
// === LeetCode 66 Plus One ===
//   [1,2,3] -> [1,2,4]
//   [4,3,2,1] -> [4,3,2,2]
//   [9] -> [1,0]
//   [9,9,9] -> [1,0,0,0]
//
// === 日常實務：從日誌尾端找最近一次 ERROR ===
// 日誌共 6 行
// 最近一次 ERROR: 找到
//   行號(0-based): 3
//   內容: 2026-07-20 09:00:07 [ERROR] upstream timeout
//   → 從尾端第 3 行就找到了，不必掃完整份日誌
// 全部正常的日誌: 沒有 ERROR（正確）
// 空日誌: 沒有 ERROR（正確，且沒有越界）
