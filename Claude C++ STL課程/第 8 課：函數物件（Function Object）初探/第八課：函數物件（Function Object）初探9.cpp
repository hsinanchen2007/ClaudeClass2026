// =============================================================================
//  第八課：函數物件 9  —  Lambda 表達式：編譯器幫你寫的 functor
// =============================================================================
//
// 【主題資訊 Information】
//   語法：
//       [捕獲列表](參數列表) mutable noexcept -> 回傳型別 { 函式本體 }
//        ^^^^^^^^ 必要   ^^^^^^^^ 可省略  ^^^^^^^^^^^^^^^^^^^^^ 皆可省略
//   最短的合法 lambda：[]{}
//   標頭檔：不需要（語言特性）。
//   標準版本：
//       lambda 本體            C++11
//       泛型 lambda（auto 參數） C++14（本機 -pedantic-errors 實測確認）
//       init capture [x = ...]  C++14
//       constexpr lambda        C++17
//       模板參數 lambda <T>     C++20
//   本質：編譯器生成一個匿名的 **closure type**（閉包類別），
//         lambda 運算式產生它的一個實例。
//
// 【詳細解釋 Explanation】
//
// 【1. lambda 不是新東西，它是第 1～4 檔的語法糖】
//   這個 lambda：
//       auto square = [](int n) { return n * n; };
//   編譯器大致展開成：
//       class __lambda_17_19 {
//       public:
//           auto operator()(int n) const { return n * n; }
//       };
//       auto square = __lambda_17_19{};
//   完全就是第 1 檔手寫的 functor，只是類別名稱由編譯器產生、你看不到。
//   所以本課前八檔講的每一件事（可攜帶狀態、每個都是獨立型別、
//   可以被內聯、以值傳給演算法）**對 lambda 全部成立**。
//
// 【2. 為什麼一定要用 auto 接住】
//   closure type 是編譯器生成的匿名型別，**你根本寫不出它的名字**。
//   所以只有三種接法：
//       auto f = [](int x){ return x; };            // ✓ 最常見，零開銷
//       std::function<int(int)> g = ...;            // ✓ 但有 type erasure 開銷
//       int (*h)(int) = ...;                        // ✓ 僅限無捕獲 lambda
//   熱路徑一律用 auto。用 std::function 會失去內聯機會（見第 15 檔）。
//
// 【3. 回傳型別怎麼決定】
//   多數情況可以省略，編譯器從 return 敘述推導（規則同 auto，會退化：
//   丟掉頂層 const 與參考）。兩種必須明寫的情況：
//     (a) 有多個 return 且型別不一致 —— 推導會失敗：
//         [](int x) { if (x > 0) return 1; else return 2.0; }      // ✗ 錯誤
//         [](int x) -> double { if (x > 0) return 1; else return 2.0; }  // ✓
//     (b) 想回傳參考（避免複製）：
//         [](std::vector<int>& v) -> int& { return v[0]; }
//         省略的話會推導成 int（複製一份），改它不會影響原容器。
//
// 【4. lambda 相對於具名 functor 的真正優勢：就地定義】
//   第 4 檔的 GreaterThan 要寫 8 行、放在 main 外面，
//   讀者得跳到別處才知道它在做什麼。
//   lambda 就寫在使用它的那一行旁邊：
//       std::count_if(v.begin(), v.end(), [](int n){ return n % 2 == 0; });
//   「邏輯與使用點的距離」是可讀性最重要的因素之一，
//   這才是 lambda 真正改變 C++ 寫法的地方——不是效能，是可讀性。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 無捕獲 lambda 的 sizeof 也是 1
//     沒有捕獲就沒有成員變數，closure type 是空類別，
//     本機實測 sizeof 為 1（同第 1 檔的 Adder；標準只要求不為 0）。
//     每捕獲一個 int 就多 4 bytes（可能有對齊填充）。
//
// (B) operator() 預設是 const 的
//     這是刻意的：lambda 的預設語意是「不改變自己的狀態」。
//     要修改值捕獲的副本必須加 mutable（第 13 檔）。
//     這個預設讓 lambda 天生就是安全的述詞。
//
// (C) 每次書寫都產生一個新型別，即使程式碼一模一樣
//     auto a = [](int x){ return x; };
//     auto b = [](int x){ return x; };
//     decltype(a) 與 decltype(b) 是**不同型別**，不能互相賦值。
//     這正是「lambda = 編譯器生成的類別」最直接的證據。
//     也因此 std::vector<decltype(a)> 裝不下 b。
//
// (D) 立即呼叫的 lambda（IIFE）：用來初始化 const 變數
//     const auto config = [&] {
//         Config c;
//         c.load(path);
//         if (!c.valid()) c = Config::defaults();
//         return c;
//     }();                 // 注意結尾的 () —— 定義完立刻呼叫
//     這個手法讓「需要多步驟才能算出來的值」也能宣告成 const，
//     是現代 C++ 很受推崇的寫法。
//
// 【注意事項 Pay Attention】
//   1. closure type 寫不出名字，一定要用 auto（或 std::function/函數指標）。
//   2. 多個 return 型別不一致時推導會失敗，要明寫 -> 型別。
//   3. 想回傳參考必須明寫 -> T&，否則會推導成值（複製）。
//   4. operator() 預設 const，要改值捕獲的副本需要 mutable（第 13 檔）。
//   5. 兩個寫得一模一樣的 lambda 是不同型別，不能互相賦值。
//   6. 熱路徑別把 lambda 包進 std::function，那會失去內聯（第 15 檔）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Lambda 的本質
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. Lambda 到底是什麼？編譯器看到它會產生什麼？
//     答：lambda 是語法糖，編譯器會生成一個匿名的 **closure type**
//         （閉包類別），lambda 運算式產生它的一個實例。
//         捕獲的變數變成該類別的**成員變數**，
//         lambda 本體變成它的 **operator()**，而且預設帶 const。
//         所以 lambda 就是「編譯器幫你寫的函數物件」——
//         函數物件的所有性質（能帶狀態、每個是獨立型別、可內聯）它全都有。
//     追問：那為什麼一定要用 auto 接？
//         → closure type 是匿名的，你寫不出它的名字。
//           除了 auto，只能用 std::function（有 type erasure 開銷）
//           或函數指標（僅限無捕獲）。
//
// 🔥 Q2. 這兩個 lambda 是同一個型別嗎？
//        auto a = [](int x){ return x; };
//        auto b = [](int x){ return x; };
//     答：**不是**。每次書寫 lambda 運算式，編譯器就生成一個全新的
//         closure type，即使程式碼完全相同。所以 decltype(a) 與 decltype(b)
//         是兩個無關的型別，a = b; 會編譯失敗。
//         這正是「lambda 是編譯器生成的類別」最直接的證據。
//     追問：這會造成什麼實際影響？
//         → std::vector<decltype(a)> 裝不下 b；
//           想把多個不同 lambda 放進同一個容器，必須用
//           std::function<int(int)> 做型別抹除（第 15 檔）。
//
// ⚠️ 陷阱. 「這個 lambda 為什麼改不動原容器？
//         auto first = [](std::vector<int>& v) { return v[0]; };
//         first(vec) = 99;   // 編譯錯誤」
//     答：因為回傳型別**推導**的規則和 auto 一樣——會**退化**：
//         丟掉參考、丟掉頂層 const。v[0] 的型別是 int&，
//         但推導結果是 int，也就是回傳一份**複本**。
//         要回傳參考必須明寫：
//             [](std::vector<int>& v) -> int& { return v[0]; }
//     為什麼會錯：以為「回傳 v[0] 就會回傳 v[0] 本身」。
//         實際上型別推導預設走「值語意」，這和
//         `auto x = v[0];` 得到的是複本而非參考是同一條規則。
//         更危險的變體是回傳區域變數的參考：
//             [](int x) -> int& { int y = x; return y; }   // 懸空參考，UB
//         明寫 -> T& 時務必確認被參考的物件活得夠久。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>
#include <type_traits>
#include <cctype>      // std::isspace（引數須先轉 unsigned char）
#include <stdexcept>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔刻意不放
//   理由：本檔講的是 lambda 的**語法與本質**（closure type、回傳型別推導、
//   為什麼要用 auto 接），這些在任何一題 LeetCode 裡都會用到，
//   卻不是任何一題的**難點**。
//   換句話說，lambda 是解題的工具而非題目本身——
//   挑一題來「示範 lambda」，讀者學到的會是那題的演算法，不是 lambda。
//   本課已在第 2 檔（179. Largest Number，比較器設計）與
//   第 3 檔（27. Remove Element，述詞需要狀態）用 LeetCode 呈現過
//   「可呼叫物件真正影響解法」的場景，那才是它們該出現的位置。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】資料清洗流水線：把原始 CSV 欄位轉成可用的數值
//   情境：從第三方拿到的報表，欄位有各種髒資料——前後空白、千分位逗號、
//         百分比符號、缺值標記 "N/A"。要轉成數字才能計算。
//   為什麼用到本主題：清洗流程是一連串小步驟，每一步都是一個
//     「只在這裡用一次」的轉換規則。
//     為每個規則寫一個具名 functor 會讓程式碼支離破碎；
//     lambda 讓每一步就寫在它被使用的那一行旁邊，
//     整條流水線由上到下讀下來就是完整的處理邏輯。
//     這正是 lambda 最大的價值：**邏輯與使用點的距離**。
// -----------------------------------------------------------------------------
struct CleanResult {
    std::vector<double> values;
    int                 skipped = 0;
};

CleanResult cleanNumericColumn(const std::vector<std::string>& raw) {
    CleanResult out;

    // 每個規則都是就地定義的小 lambda，讀者不必跳到別處查
    auto trim = [](std::string s) {
        const auto notSpace = [](unsigned char c) { return !std::isspace(c); };
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
        s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
        return s;
    };

    auto stripChars = [](std::string s, const std::string& drop) {
        s.erase(std::remove_if(s.begin(), s.end(),
                    [&drop](char c) { return drop.find(c) != std::string::npos; }),
                s.end());
        return s;
    };

    auto isMissing = [](const std::string& s) {
        return s.empty() || s == "N/A" || s == "-" || s == "null";
    };

    for (const auto& cell : raw) {
        std::string s = trim(cell);
        if (isMissing(s)) { ++out.skipped; continue; }

        bool percent = !s.empty() && s.back() == '%';
        s = stripChars(s, ",%$ ");

        try {
            double v = std::stod(s);
            if (percent) v /= 100.0;
            out.values.push_back(v);
        } catch (const std::exception&) {
            ++out.skipped;              // 轉不動的也算跳過
        }
    }
    return out;
}

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // 最簡單的 Lambda
    std::cout << "=== 基本 Lambda ===" << std::endl;
    auto print = [](int n) { std::cout << n << " "; };

    std::for_each(vec.begin(), vec.end(), print);
    std::cout << std::endl;

    // 直接內聯使用
    std::cout << "\n=== 內聯 Lambda ===" << std::endl;
    std::for_each(vec.begin(), vec.end(), [](int n) {
        std::cout << n * n << " ";
    });
    std::cout << std::endl;

    // 帶回傳值的 Lambda
    std::cout << "\n=== 帶回傳值的 Lambda ===" << std::endl;
    auto square = [](int n) { return n * n; };
    std::cout << "5 的平方: " << square(5) << std::endl;

    // 用在 count_if
    std::cout << "\n=== 用在 count_if ===" << std::endl;
    int even_count = std::count_if(vec.begin(), vec.end(),
        [](int n) { return n % 2 == 0; });
    std::cout << "偶數個數: " << even_count << std::endl;

    // -------------------------------------------------------------------------
    std::cout << "\n=== lambda 就是編譯器生成的 functor ===" << std::endl;
    {
        // 手寫版
        struct SquareFunctor {
            int operator()(int n) const { return n * n; }
        };
        SquareFunctor manual;
        auto lam = [](int n) { return n * n; };

        std::cout << "手寫 functor(5) = " << manual(5) << std::endl;
        std::cout << "lambda(5)       = " << lam(5) << std::endl;
        std::cout << "sizeof(手寫 functor) = " << sizeof(manual)
                  << "，sizeof(lambda) = " << sizeof(lam)
                  << "（都是空類別，本機實測皆為 1）" << std::endl;
        std::cout << "→ lambda 展開後就是這個手寫類別，"
                  << "只是名字由編譯器產生、你看不到。" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 每次書寫都產生新型別 ===" << std::endl;
    {
        auto a = [](int x) { return x; };
        auto b = [](int x) { return x; };      // 寫得一模一樣

        std::cout << std::boolalpha;
        std::cout << "兩個內容完全相同的 lambda 是同型別嗎? "
                  << std::is_same<decltype(a), decltype(b)>::value << std::endl;
        std::cout << "→ 所以 a = b; 會編譯失敗，"
                  << "std::vector<decltype(a)> 也裝不下 b。" << std::endl;
        std::cout << "  要把不同 lambda 放進同一容器，"
                  << "得用 std::function 抹除型別（第 15 檔）。" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 回傳型別：推導會「退化」 ===" << std::endl;
    {
        std::vector<int> v = {10, 20, 30};

        auto byValue = [](std::vector<int>& c) { return c[0]; };          // 推導成 int
        auto byRef   = [](std::vector<int>& c) -> int& { return c[0]; };  // 明寫參考

        std::cout << "省略回傳型別 → 推導成 int（複本）嗎? "
                  << std::is_same<decltype(byValue(v)), int>::value << std::endl;
        std::cout << "明寫 -> int&  → 型別是 int& 嗎?      "
                  << std::is_same<decltype(byRef(v)), int&>::value << std::endl;

        byRef(v) = 99;                       // 只有這個能改到原容器
        std::cout << "透過 byRef 改值後 v[0] = " << v[0] << std::endl;
        std::cout << "→ 推導規則同 auto：會丟掉參考與頂層 const。" << std::endl;
        std::cout << "  想改原物件就必須明寫 -> T&。" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 多個 return 型別不一致時要明寫 ===" << std::endl;
    {
        // [](int x){ if (x>0) return 1; else return 2.0; }   // ✗ 推導衝突
        auto ok = [](int x) -> double { if (x > 0) return 1; else return 2.0; };
        std::cout << "明寫 -> double: ok(5) = " << ok(5)
                  << "，ok(-5) = " << ok(-5) << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 立即呼叫的 lambda（IIFE）：讓複雜初始化也能是 const ===" << std::endl;
    {
        // 注意結尾的 () —— 定義完立刻呼叫
        const auto summary = [&vec] {
            long long sum = std::accumulate(vec.begin(), vec.end(), 0LL);
            auto [mn, mx] = std::minmax_element(vec.begin(), vec.end());
            return std::to_string(*mn) + ".." + std::to_string(*mx)
                 + " 共 " + std::to_string(vec.size()) + " 筆，總和 "
                 + std::to_string(sum);
        }();

        std::cout << "summary = " << summary << std::endl;
        std::cout << "→ 需要多步驟才算得出的值，也能宣告成 const。" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務：CSV 欄位資料清洗 ===" << std::endl;
    {
        const std::vector<std::string> rawColumn = {
            "  1,234.50  ",
            "89%",
            "N/A",
            "$2,000",
            "",
            "  -  ",
            "45.75",
            "not-a-number",
            "12.5%",
        };

        std::cout << "原始欄位 " << rawColumn.size() << " 筆：" << std::endl;
        for (const auto& c : rawColumn) std::cout << "    [" << c << "]" << std::endl;

        auto r = cleanNumericColumn(rawColumn);

        std::cout << "清洗後 " << r.values.size() << " 筆有效、"
                  << r.skipped << " 筆跳過：" << std::endl;
        std::cout << "    ";
        for (double v : r.values) std::cout << v << "  ";
        std::cout << std::endl;

        double total = std::accumulate(r.values.begin(), r.values.end(), 0.0);
        std::cout << "  合計 = " << total << std::endl;
        std::cout << "→ trim / stripChars / isMissing 三個規則都就地定義，"
                  << std::endl;
        std::cout << "  整條流水線由上到下讀完就是全部邏輯，"
                  << "不必跳到別處查 functor 定義。" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第八課：函數物件（Function Object）初探9.cpp -o demo9
//
// ⚠️ 本檔用到結構化繫結 auto [mn, mx]（C++17），最低需要 -std=c++17。

// === 預期輸出 ===
// === 基本 Lambda ===
// 1 2 3 4 5 6 7 8 9 10
//
// === 內聯 Lambda ===
// 1 4 9 16 25 36 49 64 81 100
//
// === 帶回傳值的 Lambda ===
// 5 的平方: 25
//
// === 用在 count_if ===
// 偶數個數: 5
//
// === lambda 就是編譯器生成的 functor ===
// 手寫 functor(5) = 25
// lambda(5)       = 25
// sizeof(手寫 functor) = 1，sizeof(lambda) = 1（都是空類別，本機實測皆為 1）
// → lambda 展開後就是這個手寫類別，只是名字由編譯器產生、你看不到。
//
// === 每次書寫都產生新型別 ===
// 兩個內容完全相同的 lambda 是同型別嗎? false
// → 所以 a = b; 會編譯失敗，std::vector<decltype(a)> 也裝不下 b。
//   要把不同 lambda 放進同一容器，得用 std::function 抹除型別（第 15 檔）。
//
// === 回傳型別：推導會「退化」 ===
// 省略回傳型別 → 推導成 int（複本）嗎? true
// 明寫 -> int&  → 型別是 int& 嗎?      true
// 透過 byRef 改值後 v[0] = 99
// → 推導規則同 auto：會丟掉參考與頂層 const。
//   想改原物件就必須明寫 -> T&。
//
// === 多個 return 型別不一致時要明寫 ===
// 明寫 -> double: ok(5) = 1，ok(-5) = 2
//
// === 立即呼叫的 lambda（IIFE）：讓複雜初始化也能是 const ===
// summary = 1..10 共 10 筆，總和 55
// → 需要多步驟才算得出的值，也能宣告成 const。
//
// === 日常實務：CSV 欄位資料清洗 ===
// 原始欄位 9 筆：
//     [  1,234.50  ]
//     [89%]
//     [N/A]
//     [$2,000]
//     []
//     [  -  ]
//     [45.75]
//     [not-a-number]
//     [12.5%]
// 清洗後 5 筆有效、4 筆跳過：
//     1234.5  0.89  2000  45.75  0.125
//   合計 = 3281.27
// → trim / stripChars / isMissing 三個規則都就地定義，
//   整條流水線由上到下讀完就是全部邏輯，不必跳到別處查 functor 定義。
