// =============================================================================
//  第八課：函數物件 2  —  sort 的預設順序，與「比較器」這個缺口
// =============================================================================
//
// 【主題資訊 Information】
//   template<class RandomIt>              void sort(RandomIt first, RandomIt last);
//   template<class RandomIt, class Compare>
//                                         void sort(RandomIt first, RandomIt last, Compare comp);
//   標頭檔：<algorithm>
//   標準版本：C++98；C++17 加平行版、C++20 加 std::ranges::sort。
//   複雜度：O(n log n) 比較次數（C++11 起是**最壞情況**保證，
//           C++03 時代只保證平均，這是 introsort 帶來的改進）。
//   預設比較：operator<，結果是**遞增（升序）**。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼預設是升序：因為預設比較器是 std::less，也就是 operator<】
//   單參數版的 sort 等同於
//       std::sort(first, last, std::less<>{});
//   而 std::less 就是呼叫 a < b。所以「升序」不是 sort 特別選的，
//   而是 **C++ 整個標準函式庫的統一約定**：
//   凡是需要排序語意的地方（sort、set、map、priority_queue、lower_bound…），
//   一律以 operator< 為準，且一律解讀成「a 排在 b 前面」。
//   這個約定讓你只要為自訂型別定義一個 operator<，
//   整個標準函式庫就都能用了。
//
// 【2. 那要降序怎麼辦：這就是「比較器」這個參數存在的理由】
//   sort 需要的不是「一個順序」，而是「一個能回答 a 該不該排在 b 前面的東西」。
//   這個東西必須是可呼叫物件（callable），有三種給法：
//       std::sort(v.begin(), v.end(), std::greater<int>());        // STL 內建函數物件
//       std::sort(v.begin(), v.end(), [](int a, int b){ return a > b; });  // lambda
//       std::sort(v.begin(), v.end(), MyComparator{});             // 自己寫的 functor
//   這正是本課的主軸：**把「行為」當成參數傳進去**。
//   函數物件之所以重要，就是因為它是「可以攜帶設定的行為」。
//
// 【3. 比較器的合約：Strict Weak Ordering（嚴格弱序）】
//   這是本檔最重要、也最常被違反的一點。
//   sort 對你傳進去的 comp 有四個要求：
//     (1) 非自反（irreflexive）：comp(a, a) 必須是 false
//     (2) 非對稱（asymmetric）：comp(a,b) 為真 ⟹ comp(b,a) 必為假
//     (3) 遞移（transitive）：comp(a,b) 且 comp(b,c) ⟹ comp(a,c)
//     (4) 等價的遞移性：a、b 不分先後且 b、c 不分先後 ⟹ a、c 也不分先後
//   關鍵在 (1)：比較器要回答的是「嚴格小於」，不是「小於等於」。
//   寫成 `return a >= b;` 會讓 comp(a,a) 為真，違反非自反 →
//   **整個排序的行為變成未定義**（見下方陷阱題）。
//
// 【4. sort 不是穩定排序】
//   兩個「比較起來不分先後」的元素，sort 之後的相對順序是未指定的。
//   需要保留原順序請用 std::stable_sort（代價是可能需要額外記憶體）。
//   實務上這件事在「先按 A 排、再按 B 排」的多欄位排序時最常咬人——
//   正解是寫一個一次比完所有欄位的比較器，而不是連續呼叫兩次 sort。
//
// 【概念補充 Concept Deep Dive】
//
// (A) std::sort 的實作：introsort（內省排序）
//     它是三個演算法的混合體：
//       * 主體用 quicksort（快、cache 友善）
//       * 遞迴深度超過 2*log2(n) 時切換成 heapsort（避免 quicksort 最壞 O(n²)）
//       * 分割區間小於某個門檻（libstdc++ 是 16）時改用 insertion sort
//         （小陣列上 insertion sort 的常數最小）
//     這個組合讓它同時擁有「平均很快」與「最壞仍是 O(n log n)」，
//     也是 C++11 能把最壞複雜度寫進標準的原因。
//
// (B) 為什麼違反 strict weak ordering 會出事，而不只是「排錯」
//     quicksort 的分割迴圈長得像：
//         while (comp(*i, pivot)) ++i;
//     這個迴圈**沒有邊界檢查**，它依賴「一定會有元素讓 comp 回傳 false」
//     這個數學性質來停下來。若 comp(a,a) 為真，
//     指標就可能一路衝出陣列範圍——這是越界存取，不是排序錯誤。
//     所以標準把它定為未定義行為：可能崩潰、可能默默寫壞記憶體、
//     也可能在小資料上看起來完全正常（因為沒進到 quicksort 分支）。
//     這也是為什麼這種 bug 特別難抓：測試資料小的時候測不出來。
//
// (C) C++20 的 operator<=>（三路比較）
//     自訂型別以前要定義 <、<=、>、>=、==、!= 六個運算子。
//     C++20 只要寫一個 auto operator<=>(const T&) const = default;
//     編譯器就會自動生成全部關係運算子。
//     對排序而言，這讓「為自訂型別提供正確的排序語意」變得幾乎零成本。
//
// 【注意事項 Pay Attention】
//   1. 比較器必須是**嚴格**小於（<），不能是 <=。用 <= 是未定義行為。
//   2. std::sort 不穩定；要穩定用 std::stable_sort。
//   3. 比較器不該有副作用，也不該依賴呼叫次數——
//      標準不保證它被呼叫幾次、以什麼順序呼叫。
//   4. 比較浮點數時要小心 NaN：任何與 NaN 的比較都是 false，
//      會同時違反非自反以外的多條性質，讓排序進入未定義行為。
//   5. sort 需要 Random Access Iterator，list 不能用（見第 5 課第 8 檔）。
//   6. 對已排序或幾乎排序的資料，std::sort 沒有特別優化。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】sort 的比較器與 strict weak ordering
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::sort 預設為什麼是升序？這個「預設」是哪來的？
//     答：單參數版等同於傳入 std::less<>，也就是用 operator< 比較，
//         而標準把 comp(a,b)==true 一律解讀成「a 排在 b 前面」。
//         這不是 sort 的個別決定，而是整個標準函式庫的統一約定——
//         set、map、priority_queue、lower_bound 全部以 operator< 為準。
//         好處是自訂型別只要定義一個 operator<，整個函式庫就都能用。
//     追問：那 priority_queue 用 std::less 為什麼是**大**根堆？
//         → 因為 priority_queue 的「頂端」定義成「比較起來最大的元素」，
//           語意層級不同。這是很多人第一次用會反直覺的地方。
//
// 🔥 Q2. 多欄位排序（先按部門、同部門再按薪水）該怎麼寫？
//     答：寫**一個**一次比完所有欄位的比較器，而不是連續呼叫兩次 sort：
//         if (a.dept != b.dept) return a.dept < b.dept;
//         return a.salary > b.salary;
//         最簡潔的寫法是用 std::tie 做字典序比較：
//         return std::tie(a.dept, b.salary) < std::tie(b.dept, a.salary);
//         （注意想要遞減的欄位要把 a、b 對調）
//     追問：為什麼不能先 sort 薪水、再 sort 部門？
//         → 因為 std::sort **不穩定**，第二次排序會打亂第一次的結果。
//           要用這種寫法必須改用 std::stable_sort，而且順序要反過來
//           （先排次要鍵、再排主要鍵），比較容易寫錯。
//
// ⚠️ 陷阱. 「降序比較器我寫成 [](int a, int b){ return a >= b; }，
//         測試都過了，應該沒問題吧？」
//     答：這是未定義行為，只是還沒爆而已。
//         比較器的合約要求「嚴格弱序」，第一條就是 comp(a, a) 必須為 false。
//         寫成 >= 會讓 comp(a,a) 回傳 true，違反非自反性。
//         後果不是「排序結果錯誤」這麼溫和——quicksort 的分割迴圈是
//             while (comp(*i, pivot)) ++i;
//         它**沒有邊界檢查**，靠「一定會有元素讓 comp 回傳 false」停下來。
//         合約被違反時，指標可能直接衝出陣列，變成越界存取。
//         正確寫法是 return a > b;
//     為什麼會錯：把比較器想成「排序的方向指示」，
//         覺得 > 和 >= 只差一個等號、意思差不多。
//         實際上 sort 是把它當成**數學上的嚴格序關係**在用，
//         並依賴其性質來保證演算法終止。
//         而且因為小資料不會走進 quicksort 分支（會用 insertion sort），
//         單元測試常常完全測不出來，直到上線遇到大資料才出事。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#include <tuple>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 179. Largest Number
//   題目：給一組非負整數，重新排列它們的順序，拼接成一個最大的數字（回傳字串）。
//         例：[3,30,34,5,9] -> "9534330"
//   為什麼用到本主題：這題**唯一的難點就是設計比較器**，
//     完美示範「排序的順序是由比較器定義的，不是由型別本身決定的」。
//     直覺的「數字大的排前面」是錯的：3 和 30，該把 3 排前面（"330" > "303"），
//     但 30 > 3。正確的規則是比較「拼起來哪個大」：
//         a 排在 b 前面  ⟺  a+b > b+a （字串串接後比字典序）
//     這個比較器滿足嚴格弱序（可以證明它有遞移性），所以 sort 能安全使用。
//   複雜度：O(n log n · k)，k 是數字的位數（字串比較的成本）。
// -----------------------------------------------------------------------------
std::string largestNumber(std::vector<int> nums) {
    std::vector<std::string> s;
    s.reserve(nums.size());
    for (int n : nums) s.push_back(std::to_string(n));

    // 關鍵比較器：拼起來大的排前面。注意用的是嚴格 >，不是 >=
    std::sort(s.begin(), s.end(), [](const std::string& a, const std::string& b) {
        return a + b > b + a;
    });

    // 全是 0 的特例：結果應該是 "0" 而不是 "000"
    if (!s.empty() && s[0] == "0") return "0";

    std::string out;
    for (const auto& x : s) out += x;
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】員工清單的多欄位排序：先部門（升）、同部門再薪水（降）
//   情境：後台要出一份報表，依部門分組，每組內薪水由高到低。
//   為什麼用到本主題：這是「比較器」最常見的實務用途，
//     也是最常見的錯誤來源——很多人會先 sort 薪水、再 sort 部門，
//     但 std::sort **不穩定**，第二次排序會打亂第一次的結果。
//     正解是寫一個一次比完所有欄位的比較器。
//     下面同時示範 std::tie 的簡潔寫法，並實測「兩次 sort」為什麼不可靠。
// -----------------------------------------------------------------------------
struct Employee {
    std::string name;
    std::string dept;
    int         salary;
};

// 正解：一個比較器一次比完所有欄位
struct ByDeptThenSalaryDesc {
    bool operator()(const Employee& a, const Employee& b) const {
        if (a.dept != b.dept) return a.dept < b.dept;   // 主要鍵：部門升序
        return a.salary > b.salary;                     // 次要鍵：薪水降序
    }
};

// 同一個規則的 std::tie 寫法（想要遞減的欄位把 a、b 對調）
bool byDeptThenSalaryTie(const Employee& a, const Employee& b) {
    return std::tie(a.dept, b.salary) < std::tie(b.dept, a.salary);
}

void printEmployees(const std::string& label, const std::vector<Employee>& v) {
    std::cout << label << std::endl;
    for (const auto& e : v) {
        std::cout << "    " << e.dept << "  " << e.name
                  << "  " << e.salary << std::endl;
    }
}

int main() {
    std::vector<int> vec = {5, 2, 8, 1, 9, 3};

    // sort 預設是升序
    std::sort(vec.begin(), vec.end());

    std::cout << "升序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    // 如果想要降序呢？
    // sort 需要一個「告訴它如何比較」的東西 —— 這就是比較器（comparator）

    // -------------------------------------------------------------------------
    std::cout << "\n=== 三種給比較器的方式，結果完全一樣 ===" << std::endl;
    {
        struct Desc {                                   // 自己寫的函數物件
            bool operator()(int a, int b) const { return a > b; }
        };

        std::vector<int> a = {5, 2, 8, 1, 9, 3};
        std::vector<int> b = a;
        std::vector<int> c = a;

        std::sort(a.begin(), a.end(), std::greater<int>());          // STL 內建
        std::sort(b.begin(), b.end(), [](int x, int y){ return x > y; });  // lambda
        std::sort(c.begin(), c.end(), Desc{});                        // 自訂 functor

        auto show = [](const char* tag, const std::vector<int>& v) {
            std::cout << tag;
            for (int n : v) std::cout << n << " ";
            std::cout << std::endl;
        };
        show("std::greater<int>()  : ", a);
        show("lambda               : ", b);
        show("自訂 functor          : ", c);
        std::cout << "→ sort 要的不是「順序」，是「一個能回答 a 該不該排前面的可呼叫物件」"
                  << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 預設升序的來源：std::less 就是 operator< ===" << std::endl;
    {
        std::vector<int> a = {5, 2, 8, 1, 9, 3};
        std::vector<int> b = a;

        std::sort(a.begin(), a.end());                       // 不給比較器
        std::sort(b.begin(), b.end(), std::less<int>());     // 明確給 less

        std::cout << "sort(v)              : ";
        for (int n : a) std::cout << n << " ";
        std::cout << std::endl;
        std::cout << "sort(v, less<int>()) : ";
        for (int n : b) std::cout << n << " ";
        std::cout << std::endl;
        std::cout << "兩者結果相同? " << std::boolalpha << (a == b) << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== strict weak ordering：>= 為什麼不能用 ===" << std::endl;
    {
        auto strictLess  = [](int a, int b) { return a > b; };    // 正確：嚴格大於
        auto sloppyLess  = [](int a, int b) { return a >= b; };   // 錯誤：含等於

        std::cout << "非自反性要求 comp(a, a) 必須為 false：" << std::endl;
        std::cout << "  正確比較器 comp(5, 5) = " << strictLess(5, 5)
                  << "  ← false，合約成立" << std::endl;
        std::cout << "  錯誤比較器 comp(5, 5) = " << sloppyLess(5, 5)
                  << "  ← true，合約已被違反" << std::endl;
        std::cout << "把違反合約的比較器傳給 std::sort 是**未定義行為**：" << std::endl;
        std::cout << "  可能崩潰、可能默默寫壞記憶體、也可能看起來完全正常。" << std::endl;
        std::cout << "  （所以這裡只呈現合約檢查，不實際跑那個 UB 的排序）" << std::endl;
        std::cout << "  小資料常走 insertion sort 分支而測不出來，這才是它危險的地方。"
                  << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== sort 不穩定，stable_sort 才穩定 ===" << std::endl;
    {
        // 用「同分但編號不同」的資料，觀察相對順序是否被保留。
        // 注意：元素數量必須夠多才看得出來——libstdc++ 對 16 個以下的區間
        // 走 insertion sort，那個分支碰巧是穩定的，小資料因此測不出差別。
        struct Item { int score; int id; };
        std::vector<Item> a;
        for (int i = 0; i < 40; ++i) a.push_back({i % 4, i});   // 只有 4 種分數
        std::vector<Item> b = a;

        auto byScore = [](const Item& x, const Item& y) { return x.score < y.score; };

        std::sort(a.begin(), a.end(), byScore);
        std::stable_sort(b.begin(), b.end(), byScore);

        // 只看分數 0 這一組的 id 順序，就足以判斷穩不穩定
        auto showGroup0 = [](const char* tag, const std::vector<Item>& v) {
            std::cout << tag;
            for (const auto& i : v)
                if (i.score == 0) std::cout << i.id << " ";
            std::cout << std::endl;
        };
        std::cout << "40 筆資料、只有 4 種分數；列出「分數 0」那一組的 id 順序："
                  << std::endl;
        std::cout << "原始順序          : 0 4 8 12 16 20 24 28 32 36" << std::endl;
        showGroup0("std::sort        : ", a);
        showGroup0("std::stable_sort : ", b);

        bool sortKeptOrder = std::is_sorted(a.begin(), a.end(),
            [](const Item& x, const Item& y) {
                if (x.score != y.score) return x.score < y.score;
                return x.id < y.id;
            });
        std::cout << "std::sort 這次有維持原順序嗎? " << std::boolalpha
                  << sortKeptOrder << std::endl;
        std::cout << "→ stable_sort **保證**同分者維持原本的 id 順序。" << std::endl;
        std::cout << "  std::sort 不保證；上面這行是本機此輪的實際結果，" << std::endl;
        std::cout << "  換編譯器、換資料量都可能不同，絕不可依賴。" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== LeetCode 179. Largest Number ===" << std::endl;
    {
        std::cout << "[10,2]          -> " << largestNumber({10, 2}) << std::endl;
        std::cout << "[3,30,34,5,9]   -> " << largestNumber({3, 30, 34, 5, 9}) << std::endl;
        std::cout << "[0,0]           -> " << largestNumber({0, 0})
                  << "（全 0 的特例）" << std::endl;
        std::cout << "[1]             -> " << largestNumber({1}) << std::endl;
        std::cout << "→ 難點全在比較器：a+b > b+a，不是 a > b。" << std::endl;
        std::cout << "  3 和 30 要把 3 排前面（\"330\" > \"303\"），但 30 > 3。"
                  << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務：多欄位排序（部門升序 + 薪水降序）===" << std::endl;
    {
        std::vector<Employee> staff = {
            {"Alice",   "Engineering", 95000},
            {"Bob",     "Sales",       72000},
            {"Carol",   "Engineering", 120000},
            {"Dave",    "Sales",       88000},
            {"Erin",    "Engineering", 88000},
            {"Frank",   "Marketing",   67000},
        };

        std::vector<Employee> a = staff, b = staff, wrong = staff;

        std::sort(a.begin(), a.end(), ByDeptThenSalaryDesc{});
        std::sort(b.begin(), b.end(), byDeptThenSalaryTie);

        printEmployees("正解 1：一個比較器比完所有欄位", a);
        printEmployees("正解 2：std::tie 字典序寫法（結果應相同）", b);
        std::cout << "  兩種寫法結果一致? " << std::boolalpha
                  << std::equal(a.begin(), a.end(), b.begin(),
                                [](const Employee& x, const Employee& y) {
                                    return x.name == y.name;
                                })
                  << std::endl;

        // 常見錯誤示範：連續兩次 sort
        std::sort(wrong.begin(), wrong.end(),
                  [](const Employee& x, const Employee& y){ return x.salary > y.salary; });
        std::sort(wrong.begin(), wrong.end(),
                  [](const Employee& x, const Employee& y){ return x.dept < y.dept; });
        printEmployees("有風險的寫法：先排薪水再排部門（依賴 sort 保留前一次順序）",
                       wrong);
        bool sameAsCorrect = std::equal(a.begin(), a.end(), wrong.begin(),
            [](const Employee& x, const Employee& y) { return x.name == y.name; });
        std::cout << "  這次的結果和正解相同嗎? " << std::boolalpha << sameAsCorrect
                  << std::endl;
        std::cout << "→ 這筆資料只有 6 筆，libstdc++ 會走 insertion sort 分支，" << std::endl;
        std::cout << "  碰巧是穩定的，所以看起來「對了」——這正是它危險的地方：" << std::endl;
        std::cout << "  它靠的是 std::sort **沒有承諾**的行為，資料一多就可能翻車。"
                  << std::endl;
        std::cout << "  要用「兩次排序」必須改 std::stable_sort，而且順序要反過來" << std::endl;
        std::cout << "  （先次要鍵、再主要鍵）。寫一個完整的比較器才是可靠的做法。"
                  << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第八課：函數物件（Function Object）初探2.cpp -o demo2

// === 預期輸出 ===
// 升序: 1 2 3 5 8 9
//
// === 三種給比較器的方式，結果完全一樣 ===
// std::greater<int>()  : 9 8 5 3 2 1
// lambda               : 9 8 5 3 2 1
// 自訂 functor          : 9 8 5 3 2 1
// → sort 要的不是「順序」，是「一個能回答 a 該不該排前面的可呼叫物件」
//
// === 預設升序的來源：std::less 就是 operator< ===
// sort(v)              : 1 2 3 5 8 9
// sort(v, less<int>()) : 1 2 3 5 8 9
// 兩者結果相同? true
//
// === strict weak ordering：>= 為什麼不能用 ===
// 非自反性要求 comp(a, a) 必須為 false：
//   正確比較器 comp(5, 5) = false  ← false，合約成立
//   錯誤比較器 comp(5, 5) = true  ← true，合約已被違反
// 把違反合約的比較器傳給 std::sort 是**未定義行為**：
//   可能崩潰、可能默默寫壞記憶體、也可能看起來完全正常。
//   （所以這裡只呈現合約檢查，不實際跑那個 UB 的排序）
//   小資料常走 insertion sort 分支而測不出來，這才是它危險的地方。
//
// === sort 不穩定，stable_sort 才穩定 ===
// 40 筆資料、只有 4 種分數；列出「分數 0」那一組的 id 順序：
// 原始順序          : 0 4 8 12 16 20 24 28 32 36
// std::sort        : 8 16 20 12 24 28 32 4 36 0
// std::stable_sort : 0 4 8 12 16 20 24 28 32 36
// std::sort 這次有維持原順序嗎? false
// → stable_sort **保證**同分者維持原本的 id 順序。
//   std::sort 不保證；上面這行是本機此輪的實際結果，
//   換編譯器、換資料量都可能不同，絕不可依賴。
//
// === LeetCode 179. Largest Number ===
// [10,2]          -> 210
// [3,30,34,5,9]   -> 9534330
// [0,0]           -> 0（全 0 的特例）
// [1]             -> 1
// → 難點全在比較器：a+b > b+a，不是 a > b。
//   3 和 30 要把 3 排前面（"330" > "303"），但 30 > 3。
//
// === 日常實務：多欄位排序（部門升序 + 薪水降序）===
// 正解 1：一個比較器比完所有欄位
//     Engineering  Carol  120000
//     Engineering  Alice  95000
//     Engineering  Erin  88000
//     Marketing  Frank  67000
//     Sales  Dave  88000
//     Sales  Bob  72000
// 正解 2：std::tie 字典序寫法（結果應相同）
//     Engineering  Carol  120000
//     Engineering  Alice  95000
//     Engineering  Erin  88000
//     Marketing  Frank  67000
//     Sales  Dave  88000
//     Sales  Bob  72000
//   兩種寫法結果一致? true
// 有風險的寫法：先排薪水再排部門（依賴 sort 保留前一次順序）
//     Engineering  Carol  120000
//     Engineering  Alice  95000
//     Engineering  Erin  88000
//     Marketing  Frank  67000
//     Sales  Dave  88000
//     Sales  Bob  72000
//   這次的結果和正解相同嗎? true
// → 這筆資料只有 6 筆，libstdc++ 會走 insertion sort 分支，
//   碰巧是穩定的，所以看起來「對了」——這正是它危險的地方：
//   它靠的是 std::sort **沒有承諾**的行為，資料一多就可能翻車。
//   要用「兩次排序」必須改 std::stable_sort，而且順序要反過來
//   （先次要鍵、再主要鍵）。寫一個完整的比較器才是可靠的做法。
