// =============================================================================
//  第 16 課：vector 的迭代器操作 3  —  反向迭代器 rbegin() / rend() 與 base()
// =============================================================================
//
// 【主題資訊 Information】
//   reverse_iterator rbegin() noexcept;   // 指向「最後一個元素」
//   reverse_iterator rend()   noexcept;   // 指向「第一個元素之前」的哨兵
//   iterator         base() const;        // 由反向迭代器換回正向迭代器
//   標頭檔：<vector>（reverse_iterator 定義在 <iterator>，由 <vector> 間接引入）
//   標準版本：C++98
//   複雜度：rbegin()/rend()/base() 皆 O(1)
//
// 【詳細解釋 Explanation】
//
// 【1. 反向迭代器不是新東西，它是一個 adaptor（配接器）】
//   std::reverse_iterator 是一個「包在正向迭代器外面的殼」，內部只存一個
//   正向迭代器 current，然後把所有運算方向反過來：
//       operator++()  → --current
//       operator--()  → ++current
//       operator*()   → { auto tmp = current; return *--tmp; }   ← 關鍵！
//   因為方向被反過來，你才能用完全相同的 for 迴圈語法（++、!=）反向走訪，
//   而 std::sort、std::find 這些演算法也能直接吃反向迭代器，不必另寫反向版。
//
// 【2. base() 為什麼差一格 —— 這是最常被問、也最常搞混的一點】
//   規則是：*rit 等價於 *(rit.base() - 1)。也就是 base() 回傳的正向迭代器
//   永遠比 rit 實際指向的元素「往後多一格」。
//
//       元素索引：      0     1     2     3     4
//       元素值：      [10]  [20]  [30]  [40]  [50]
//       正向：        begin()                        end()
//                       ↑                             ↑
//       反向：                                     rbegin()（指向 50）
//                    rend()（指向 10 之前）
//
//       rbegin().base() == end()      （實測：true）
//       rend().base()   == begin()    （實測：true）
//
//   為什麼要這樣設計？因為兩個區間都必須是半開的：
//       正向區間 [begin, end)  ↔  反向區間 [rbegin, rend)
//   如果 base() 指向同一個元素，那 rend() 就必須指向「begin() 的前一格」——
//   而那是一個不存在、也不允許被計算的位置（begin() - 1 是 UB）。
//   把整體平移一格，就讓 rend().base() 落在合法的 begin() 上，兩邊區間
//   剛好一一對應。代價就是 base() 與 *rit 差一格，必須背下來。
//
// 【3. 反向迭代器最實用的場合：需要「刪除」時】
//   反向迭代器不能直接餵給 erase()（erase 只吃正向迭代器），要先 base()。
//   由於差一格，正確寫法是：
//       v.erase(std::next(rit).base());   // 刪掉 *rit 這個元素
//   直接寫 v.erase(rit.base()) 會刪到「它後面那一個」，是很經典的錯誤。
//
// 【概念補充 Concept Deep Dive】
//   operator*() 每次都要先複製再 --，聽起來比正向迭代器多一個減法。
//   實務上編譯器在 -O2 幾乎都能把這個暫時變數消掉，但「反向走訪 vector 比
//   正向慢一點」在某些情境仍可測得到，原因不在這個減法，而在硬體：
//   CPU 的 prefetcher 對「位址遞增」的存取樣式最友善，遞減樣式的預取效果
//   通常較差。這是實作/硬體層面的觀察，不是標準的規定。
//   若只是要反向印出，rbegin()/rend() 的可讀性通常勝過手寫索引倒數迴圈，
//   而且完全避免了 size() 為 0 時 `for (int i = v.size() - 1; i >= 0; --i)`
//   在 unsigned 型別下反而變成天文數字的經典 bug。
//
// 【注意事項 Pay Attention】
//   1. *rit 與 *rit.base() 不是同一個元素，差一格。
//   2. *v.rend() 是 UB（它是哨兵）；v.rbegin() 對空 vector 也不可解參考。
//   3. 反向迭代器不能直接傳給 erase()/insert()，要先 .base()。
//   4. 別用 `for (unsigned i = v.size() - 1; i >= 0; --i)` 反向走訪：
//      v 為空時 size() - 1 會下溢成極大值，且 unsigned 恆 >= 0，迴圈不會結束。
//      這正是 rbegin()/rend() 想解決的問題。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】反向迭代器與 base()
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. rit.base() 和 rit 指向同一個元素嗎？為什麼？
//     答：不是，差一格：*rit 等於 *(rit.base() - 1)。
//         這樣設計是為了讓 [rbegin, rend) 與 [begin, end) 兩個半開區間能一一
//         對應：rbegin().base() == end()、rend().base() == begin()（本機實測）。
//         若 base() 指向同一元素，rend().base() 就得是 begin() - 1，那是非法位置。
//     追問：那要用反向迭代器刪除元素該怎麼寫？
//         → v.erase(std::next(rit).base())；寫成 v.erase(rit.base()) 會刪錯一個。
//
// 🔥 Q2. reverse_iterator 是如何實作「++ 卻往回走」的？
//     答：它是 adaptor，內部包一個正向迭代器 current，把運算子反向轉發：
//         operator++ 呼叫 --current，operator* 則是先複製再 --，回傳 *--tmp。
//         所以它不需要容器支援反向儲存，任何雙向以上的迭代器都能被包成反向版。
//
// ⚠️ 陷阱. 為什麼不要寫 for (unsigned i = v.size() - 1; i >= 0; --i)？
//     答：兩個問題疊在一起。v 為空時 size() 是 0，0 - 1 在 unsigned 下會回繞成
//         極大值（本機 size_t 為 64 位元，結果是 18446744073709551615），
//         直接越界存取；而且 unsigned 恆 >= 0，這個條件永遠成立，迴圈不會停。
//     為什麼會錯：腦中把 size() 當成有號 int，覺得「空的時候 i = -1，
//         條件不成立就結束了」。但 size() 回傳的是無號的 size_type，
//         -1 只是它回繞後的位元樣貌。用 rbegin()/rend() 就完全繞開這個坑。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <iterator>  // std::next

void demo_reverse_iterator() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // rbegin() 指向最後一個元素；rend() 指向第一個元素的「前一個位置」
    std::vector<int>::reverse_iterator rit;

    std::cout << "反向遍歷：";
    for (rit = v.rbegin(); rit != v.rend(); ++rit) {
        std::cout << *rit << " ";
    }
    std::cout << std::endl;

    // 反向迭代器一樣可以寫入
    for (auto it = v.rbegin(); it != v.rend(); ++it) *it += 1;
    std::cout << "反向走訪並各加 1：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;
}

void demo_base() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    auto r = v.rbegin();                     // 指向 50
    std::cout << std::boolalpha;
    std::cout << "*rbegin()            = " << *r << std::endl;
    std::cout << "*(rbegin().base()-1) = " << *(r.base() - 1) << "（同一個元素）" << std::endl;
    std::cout << "rbegin().base() == end()   : " << (v.rbegin().base() == v.end())   << std::endl;
    std::cout << "rend().base()   == begin() : " << (v.rend().base()   == v.begin()) << std::endl;

    // 用反向迭代器找到「最後一個 30」並刪除它 —— 注意 std::next(...).base()
    for (auto it = v.rbegin(); it != v.rend(); ++it) {
        if (*it == 30) {
            v.erase(std::next(it).base());   // 正確：刪掉 *it 本身
            break;                           // erase 後迭代器失效，立即離開迴圈
        }
    }
    std::cout << "刪除 30 之後：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 344. Reverse String
//   題目：就地反轉字元陣列 s，不得配置額外陣列（空間 O(1)）。
//   為什麼用到本主題：這題示範「一對相向而行的迭代器」。可以用正向 + 反向
//         迭代器各據一端往中間收斂，正是 rbegin() 的典型用途；
//         迴圈的終止判斷也再次用到半開區間的性質。
//   複雜度：時間 O(n)、空間 O(1)。
// -----------------------------------------------------------------------------
void reverseString(std::vector<char>& s) {
    auto left  = s.begin();
    auto right = s.rbegin();                 // 反向端：指向最後一個字元
    // 走 size()/2 次即可；用距離判斷比互比兩種不同型別的迭代器安全
    for (std::size_t i = 0; i < s.size() / 2; ++i) {
        std::swap(*left, *right);
        ++left;
        ++right;                             // 反向迭代器的 ++ 是往左走
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例】從 log 尾端往回找「最近一次」ERROR
//   情境：服務掛掉時要查最近一筆錯誤。log 是依時間遞增append 的，
//         最近的錯誤在尾端 —— 從頭掃到尾等於白掃整個檔案。
//   為什麼用到本主題：rbegin() 讓「從最新往回找」變成一行迴圈；
//         找到後用 base() 換算回正向位置，才能回報它是第幾行。
// -----------------------------------------------------------------------------
std::string findLatestError(const std::vector<std::string>& logs, std::size_t& line_no) {
    for (auto rit = logs.crbegin(); rit != logs.crend(); ++rit) {
        if (rit->find("[ERROR]") != std::string::npos) {
            // base() 比 rit 多一格，所以行號（0-based）要 -1
            line_no = static_cast<std::size_t>(rit.base() - logs.cbegin()) - 1;
            return *rit;
        }
    }
    line_no = 0;
    return "";
}

int main() {
    std::cout << "=== 反向迭代器基本操作 ===" << std::endl;
    demo_reverse_iterator();

    std::cout << "\n=== base() 的差一格關係 ===" << std::endl;
    demo_base();

    std::cout << "\n=== LeetCode 344. Reverse String ===" << std::endl;
    std::vector<char> s = {'h', 'e', 'l', 'l', 'o'};
    reverseString(s);
    for (char c : s) std::cout << c;
    std::cout << std::endl;

    std::cout << "\n=== 日常實務：從尾端找最近一次 ERROR ===" << std::endl;
    std::vector<std::string> logs = {
        "10:00:01 [INFO]  service started",
        "10:00:09 [ERROR] db connect timeout",
        "10:00:11 [INFO]  retry succeeded",
        "10:00:15 [ERROR] disk usage 91%",
        "10:00:20 [INFO]  heartbeat ok"
    };
    std::size_t line = 0;
    std::string hit = findLatestError(logs, line);
    std::cout << "最近的錯誤在第 " << line << " 行（0-based）：" << hit << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 16 課：vector 的迭代器操作3.cpp" -o iter3

// === 預期輸出 ===
// === 反向迭代器基本操作 ===
// 反向遍歷：50 40 30 20 10 
// 反向走訪並各加 1：11 21 31 41 51 
//
// === base() 的差一格關係 ===
// *rbegin()            = 50
// *(rbegin().base()-1) = 50（同一個元素）
// rbegin().base() == end()   : true
// rend().base()   == begin() : true
// 刪除 30 之後：10 20 40 50 
//
// === LeetCode 344. Reverse String ===
// olleh
//
// === 日常實務：從尾端找最近一次 ERROR ===
// 最近的錯誤在第 3 行（0-based）：10:00:15 [ERROR] disk usage 91%
