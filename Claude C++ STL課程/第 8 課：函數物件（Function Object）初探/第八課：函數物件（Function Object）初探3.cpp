// =============================================================================
//  第八課：函數物件 3  —  用普通函數當述詞與比較器（以及它的兩個極限）
// =============================================================================
//
// 【主題資訊 Information】
//   述詞（predicate）：回傳可轉成 bool、用來「判斷」的可呼叫物件
//       bool is_even(int n);                          → 一元述詞（unary predicate）
//       bool greater_than(int a, int b);              → 二元述詞 / 比較器
//   用法：
//       std::count_if(first, last, is_even);          // 傳函數名 → 隱式退化成函數指標
//       std::sort(first, last, greater_than);
//   標頭檔：<algorithm>
//   標準版本：C++98 起即可這樣用。
//
// 【詳細解釋 Explanation】
//
// 【1. 傳「函數名」進去時，實際上傳的是什麼】
//   寫 std::count_if(v.begin(), v.end(), is_even) 時，
//   is_even 這個**函數名**會發生「函數到指標的隱式轉換」（function-to-pointer decay），
//   實際傳進去的是 bool(*)(int) 這個**函數指標**。
//   所以模板參數 Predicate 被推導成 bool(*)(int)，
//   演算法內部呼叫 pred(x) 是一次**間接呼叫**——
//   透過一個執行期才知道的位址去跳轉。
//
// 【2. 極限一：普通函數無法攜帶狀態】
//   這是本檔最重要的一點，也是下一檔（第 4 檔）存在的理由。
//   is_even 這種「規則寫死」的判斷沒問題，但只要規則需要參數化就卡住了：
//       bool greater_than_n(int x) { return x > ???; }   // N 從哪來？
//   可行的爛解法只有兩個：
//     (a) 用全域變數存 N —— 不可重入、不執行緒安全、
//         而且兩個地方想用不同的 N 就直接打架。
//     (b) 改成二元函數多傳一個參數 —— 但 count_if 只會傳一個引數給述詞，
//         簽章對不上，根本傳不進去。
//   函數物件把 N 存成成員變數，一次解決兩個問題。
//
// 【3. 極限二：函數指標通常無法被內聯】
//   模板參數是 bool(*)(int) 時，所有函數指標都是**同一個型別**。
//   編譯器在實例化 count_if 時只知道「這裡會呼叫某個 bool(*)(int)」，
//   不知道具體是哪一個，因此難以把函數本體展開進去。
//   函數物件與 lambda 則是**每個都是獨立型別**，
//   模板實例化時 operator() 的內容直接可見，可以完全內聯。
//   這就是 std::sort 常勝過 C 的 qsort 的主因（qsort 只能收函數指標）。
//
//   ★ 但這裡要誠實：「能不能內聯」取決於最佳化等級與編譯器的判斷。
//     在 -O2 下，若函數定義在同一個翻譯單元且編譯器看得到，
//     它其實**有可能**去虛擬化（devirtualize）並內聯掉；
//     在 -O0 下則兩者都不內聯。所以效能差距不是恆定的，
//     跨翻譯單元、或函數位址被存進變數時，差距才會穩定顯現。
//     本檔的計時輸出走 stderr，正是因為它每次執行都不同、不該當成固定結果。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 三種可呼叫物件傳給演算法時的型別差異
//     普通函數名  → 推導成 bool(*)(int)       所有函數共用一個型別
//     函數物件    → 推導成 IsEven（你的類別）  每個 functor 一個獨立型別
//     lambda      → 推導成編譯器生成的匿名類別 每次書寫一個新型別
//     「型別是否獨一無二」正是能否內聯的關鍵。
//
// (B) 無捕獲的 lambda 可以轉成函數指標，有捕獲的不行
//     auto f = [](int x){ return x > 0; };
//     bool (*p)(int) = f;      // 合法：無捕獲 lambda 有隱式轉換運算子
//     int n = 5;
//     auto g = [n](int x){ return x > n; };
//     bool (*q)(int) = g;      // 編譯錯誤：帶狀態的東西塞不進一個位址
//     這個規則很能說明「函數指標為什麼帶不了狀態」——
//     一個指標就只有一個位址，沒有地方放 n。
//     也因此註冊 C API 回呼（callback）時，
//     常要走「無捕獲 lambda + void* user_data」這條路。
//
// (C) 函數重載時傳函數名會失敗
//     若 is_even 有多個重載版本，std::count_if(..., is_even) 會編譯失敗——
//     模板推導無法決定要取哪一個重載的位址。
//     解法是明確轉型 static_cast<bool(*)(int)>(is_even)，
//     或改包一層 lambda：[](int x){ return is_even(x); }（推薦，可讀性好得多）。
//
// (D) std::function 也帶不來內聯
//     把函數包進 std::function<bool(int)> 一樣是間接呼叫（type erasure），
//     而且還多一層。要效能就直接傳 functor / lambda 當模板參數。
//
// 【注意事項 Pay Attention】
//   1. 傳函數名會隱式退化成函數指標，型別是 bool(*)(int)。
//   2. 述詞**不應該有副作用**——標準不保證它被呼叫幾次、以什麼順序呼叫。
//   3. 比較器必須滿足嚴格弱序（見第 2 檔），用 >= 是未定義行為。
//   4. 函數有重載時，直接傳函數名會編譯失敗，要轉型或包 lambda。
//   5. 效能差距與最佳化等級高度相關，別把「functor 一定比較快」當定律，
//      要量測（本檔的計時輸出走 stderr，因為它不可重現）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】函數指標 vs 函數物件
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 把函數名 is_even 傳給 std::count_if，實際上傳進去的是什麼？
//     答：函數指標。函數名在這個情境會發生 function-to-pointer decay，
//         型別是 bool(*)(int)。所以模板參數被推導成函數指標型別，
//         演算法內部的 pred(x) 是一次透過執行期位址的間接呼叫。
//     追問：這和傳 lambda 有什麼不同？
//         → lambda 每次書寫都產生一個**獨一無二的匿名類別**，
//           模板實例化時 operator() 的內容直接可見，可以內聯；
//           而所有 bool(*)(int) 都是同一個型別，編譯器不知道實際指向誰。
//
// 🔥 Q2. 無捕獲的 lambda 可以轉成函數指標，有捕獲的為什麼不行？
//     答：因為函數指標就只是一個位址，沒有任何地方可以存狀態。
//         無捕獲 lambda 沒有狀態，所以標準特別為它定義了
//         一個到函數指標的隱式轉換運算子。
//         一旦有捕獲，closure 物件就帶了資料，塞不進一個位址。
//     追問：那要把帶狀態的 lambda 註冊給 C API 的 callback 怎麼辦？
//         → 標準做法是「無捕獲 lambda + void* user_data」：
//           把狀態放進 user_data，在 lambda 裡轉型取回。
//           C API 的 callback 幾乎都留了這個參數，原因就在這裡。
//
// ⚠️ 陷阱. 「我的述詞裡放個計數器統計被呼叫幾次，
//         這樣就能知道演算法的實際複雜度了吧？」
//     答：不可靠。標準**不保證**述詞被呼叫的次數與順序，
//         而且演算法可以自由**複製**你的述詞（見第 5 檔的 for_each）。
//         用值傳遞的述詞，內部累積的狀態根本傳不回來；
//         用參考捕獲雖然能拿到值，但那個數字取決於實作細節，
//         換一個標準函式庫版本就變了。
//         更嚴重的是：對 std::sort 這類演算法，有副作用的比較器
//         可能讓「同一對元素比較兩次得到不同答案」，直接違反嚴格弱序合約。
//     為什麼會錯：把述詞想成「我寫的、我控制的函數」。
//         實際上它是交給演算法的一份**合約**，
//         演算法有權複製它、以任意順序呼叫、呼叫任意次數。
//         述詞應該是**純函數**：同樣輸入永遠同樣輸出、沒有副作用。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <type_traits>

// 普通函數
bool is_even(int n) {
    return n % 2 == 0;
}

bool greater_than(int a, int b) {
    return a > b;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 27. Remove Element
//   題目：原地移除陣列中所有等於 val 的元素，回傳剩下的長度 k。
//   為什麼用到本主題：這題正是「述詞」的直接應用，
//     而且它同時暴露了普通函數的極限——
//     要移除的 val 是**執行期才知道的參數**，
//     普通函數沒有地方存它（除非用全域變數），
//     所以只能改用帶狀態的函數物件或帶捕獲的 lambda。
//     下面三種寫法並列，可以清楚看到「狀態」這個需求是怎麼冒出來的。
//   複雜度：O(n)。
// -----------------------------------------------------------------------------

// 寫法 A：普通函數 —— 只能寫死一個值，無法參數化
bool equals_two(int x) { return x == 2; }

int removeElementHardcoded(std::vector<int>& nums) {
    auto newEnd = std::remove_if(nums.begin(), nums.end(), equals_two);
    return static_cast<int>(newEnd - nums.begin());
}

// 寫法 B：函數物件 —— 把 val 存成成員變數（第 4 檔的主題）
struct EqualsTo {
    int val;
    bool operator()(int x) const { return x == val; }
};

int removeElementFunctor(std::vector<int>& nums, int val) {
    auto newEnd = std::remove_if(nums.begin(), nums.end(), EqualsTo{val});
    return static_cast<int>(newEnd - nums.begin());
}

// 寫法 C：lambda 捕獲 —— 同樣的事，語法更短（第 10 檔的主題）
int removeElementLambda(std::vector<int>& nums, int val) {
    auto newEnd = std::remove_if(nums.begin(), nums.end(),
                                 [val](int x) { return x == val; });
    return static_cast<int>(newEnd - nums.begin());
}

// -----------------------------------------------------------------------------
// 【日常實務範例】用「函數指標表」做 log 等級過濾
//   情境：log 收集器要依設定挑出想看的行。等級規則是固定的幾種
//         （只看錯誤 / 錯誤與警告 / 全部），不需要參數化，
//         這種情況普通函數就綽綽有餘，做成 functor 反而囉唆。
//   為什麼用到本主題：這是普通函數**適合**的場景，
//     用來對照第 4 檔「需要參數化就得換工具」的場景。
//     一張 { 名稱, 函數指標 } 的表格是 C 與 C++ 都常見的手法，
//     好處是資料驅動、要加新規則只需在表格加一列。
// -----------------------------------------------------------------------------
bool onlyError(const std::string& line) {
    return line.find("[ERROR]") != std::string::npos;
}
bool errorOrWarn(const std::string& line) {
    return line.find("[ERROR]") != std::string::npos
        || line.find("[WARN]")  != std::string::npos;
}
bool everything(const std::string&) {
    return true;
}

struct FilterRule {
    const char* name;
    bool (*match)(const std::string&);   // 函數指標：規則固定，不需狀態
};

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // 用普通函數作為謂詞
    int even_count = std::count_if(vec.begin(), vec.end(), is_even);
    std::cout << "偶數個數: " << even_count << std::endl;

    // 用普通函數作為比較器
    std::sort(vec.begin(), vec.end(), greater_than);
    std::cout << "降序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    // -------------------------------------------------------------------------
    std::cout << "\n=== 傳「函數名」進去，實際型別是函數指標 ===" << std::endl;
    {
        std::cout << std::boolalpha;
        std::cout << "decltype(is_even) 是函數型別嗎? "
                  << std::is_function<decltype(is_even)>::value << std::endl;
        std::cout << "decltype(&is_even) 是 bool(*)(int) 嗎? "
                  << std::is_same<decltype(&is_even), bool(*)(int)>::value << std::endl;

        // 所有 bool(*)(int) 都是同一個型別 —— 這正是無法內聯的原因
        auto anotherPred = [](int n) { return n % 3 == 0; };
        bool (*p1)(int) = is_even;
        bool (*p2)(int) = anotherPred;        // 無捕獲 lambda 可以轉成函數指標
        std::cout << "兩個不同函數的指標，型別相同嗎? "
                  << std::is_same<decltype(p1), decltype(p2)>::value
                  << "  ← 編譯器分不出誰是誰" << std::endl;

        std::cout << "sizeof(函數指標) = " << sizeof(p1)
                  << " bytes（就只是一個位址，沒地方放狀態）" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 極限一：普通函數帶不了狀態 ===" << std::endl;
    {
        std::cout << "想寫「大於 N」的述詞，N 從哪來？" << std::endl;
        std::cout << "  bool greater_than_n(int x) { return x > ???; }" << std::endl;
        std::cout << "  (a) 用全域變數  → 不可重入、不執行緒安全、兩處想用不同 N 就打架"
                  << std::endl;
        std::cout << "  (b) 多傳一個參數 → count_if 只傳一個引數給述詞，簽章對不上"
                  << std::endl;
        std::cout << "→ 這正是第 4 檔「帶狀態的函數物件」要解決的問題。" << std::endl;

        // 無捕獲 lambda 可以轉函數指標；有捕獲的不行
        int n = 5;
        auto noCapture   = [](int x) { return x > 0; };
        auto withCapture = [n](int x) { return x > n; };
        bool (*ok)(int) = noCapture;          // 合法
        std::cout << "無捕獲 lambda 轉成函數指標: 可以（ok(7)=" << ok(7) << "）"
                  << std::endl;
        std::cout << "有捕獲 lambda 轉成函數指標: 編譯錯誤——狀態塞不進一個位址"
                  << std::endl;
        std::cout << "  （有捕獲的仍可直接用: withCapture(7)=" << withCapture(7) << "）"
                  << std::endl;
        std::cout << "sizeof(無捕獲 lambda) = " << sizeof(noCapture)
                  << "，sizeof(捕獲一個 int 的 lambda) = " << sizeof(withCapture)
                  << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 極限二：型別是否獨一無二，決定能不能內聯 ===" << std::endl;
    {
        struct IsEvenFunctor { bool operator()(int n) const { return n % 2 == 0; } };
        auto isEvenLambda = [](int n) { return n % 2 == 0; };

        std::vector<int> v(1000);
        for (int i = 0; i < 1000; ++i) v[i] = i;

        auto c1 = std::count_if(v.begin(), v.end(), is_even);          // 函數指標
        auto c2 = std::count_if(v.begin(), v.end(), IsEvenFunctor{});  // 獨立型別
        auto c3 = std::count_if(v.begin(), v.end(), isEvenLambda);     // 獨立型別

        std::cout << "三種寫法結果相同: " << c1 << " / " << c2 << " / " << c3
                  << std::endl;
        std::cout << "但型別完全不同：" << std::endl;
        std::cout << "  函數指標 : 所有 bool(*)(int) 共用一個型別" << std::endl;
        std::cout << "  functor  : IsEvenFunctor 是獨立型別，內容編譯期可見" << std::endl;
        std::cout << "  lambda   : 每次書寫都產生新的匿名類別" << std::endl;
        std::cout << "→ 效能差距與最佳化等級高度相關，實測數據印在 stderr，" << std::endl;
        std::cout << "  因為它每次執行都不同，不該當成固定結果。" << std::endl;

        // 計時走 stderr：不可重現的數據不放進 stdout
        auto bench = [&v](const char* tag, auto pred) {
            auto t0 = std::chrono::steady_clock::now();
            long long total = 0;
            for (int rep = 0; rep < 2000; ++rep)
                total += std::count_if(v.begin(), v.end(), pred);
            auto t1 = std::chrono::steady_clock::now();
            std::cerr << "  [stderr] " << tag << ": "
                      << std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count()
                      << " us (total=" << total << ")\n";
        };
        std::cerr << "[stderr] 計時參考（每次執行都不同，且高度依賴 -O 等級）:\n";
        bench("函數指標", is_even);
        bench("functor ", IsEvenFunctor{});
        bench("lambda  ", isEvenLambda);
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== LeetCode 27. Remove Element ===" << std::endl;
    {
        std::vector<int> a = {3, 2, 2, 3};
        int k1 = removeElementHardcoded(a);
        std::cout << "寫死版 remove 2 from [3,2,2,3] -> k=" << k1 << ", 前 k 個 = ";
        for (int i = 0; i < k1; ++i) std::cout << a[i] << " ";
        std::cout << "  ← 只能移除寫死的 2" << std::endl;

        std::vector<int> b = {0,1,2,2,3,0,4,2};
        int k2 = removeElementFunctor(b, 2);
        std::cout << "functor 版 remove 2      -> k=" << k2 << ", 前 k 個 = ";
        for (int i = 0; i < k2; ++i) std::cout << b[i] << " ";
        std::cout << std::endl;

        std::vector<int> c = {0,1,2,2,3,0,4,2};
        int k3 = removeElementLambda(c, 3);
        std::cout << "lambda 版 remove 3       -> k=" << k3 << ", 前 k 個 = ";
        for (int i = 0; i < k3; ++i) std::cout << c[i] << " ";
        std::cout << std::endl;
        std::cout << "→ val 是執行期參數，普通函數存不了它，只能換工具。" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務：函數指標表做 log 過濾 ===" << std::endl;
    {
        const std::vector<std::string> logs = {
            "10:00:01 [INFO]  service started",
            "10:00:07 [ERROR] db connect failed",
            "10:00:09 [WARN]  retrying",
            "10:00:15 [INFO]  connected",
            "10:00:31 [ERROR] timeout on /api/user",
            "10:00:44 [DEBUG] cache hit ratio 0.93",
        };

        const FilterRule rules[] = {
            {"只看 ERROR",      onlyError},
            {"ERROR + WARN",    errorOrWarn},
            {"全部",            everything},
        };

        for (const auto& r : rules) {
            auto n = std::count_if(logs.begin(), logs.end(), r.match);
            std::cout << r.name << "（" << n << " 行）:" << std::endl;
            for (const auto& line : logs)
                if (r.match(line)) std::cout << "    " << line << std::endl;
        }
        std::cout << "→ 規則固定、不需參數化時，函數指標表最簡潔：" << std::endl;
        std::cout << "  資料驅動、要加規則只需在表格多一列。" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第八課：函數物件（Function Object）初探3.cpp -o demo3
//
// ⚠️ 本檔的計時結果印在 **stderr**，每次執行都不同，且高度依賴最佳化等級
//    （-O0 下函數指標與 functor 都不內聯，差距很小甚至相反；
//      -O2 下 functor/lambda 才可能顯出優勢）。
//    下方「預期輸出」只收 stdout，因此不含那些數字。

// === 預期輸出 ===
// 偶數個數: 5
// 降序: 10 9 8 7 6 5 4 3 2 1
//
// === 傳「函數名」進去，實際型別是函數指標 ===
// decltype(is_even) 是函數型別嗎? true
// decltype(&is_even) 是 bool(*)(int) 嗎? true
// 兩個不同函數的指標，型別相同嗎? true  ← 編譯器分不出誰是誰
// sizeof(函數指標) = 8 bytes（就只是一個位址，沒地方放狀態）
//
// === 極限一：普通函數帶不了狀態 ===
// 想寫「大於 N」的述詞，N 從哪來？
//   bool greater_than_n(int x) { return x > ???; }
//   (a) 用全域變數  → 不可重入、不執行緒安全、兩處想用不同 N 就打架
//   (b) 多傳一個參數 → count_if 只傳一個引數給述詞，簽章對不上
// → 這正是第 4 檔「帶狀態的函數物件」要解決的問題。
// 無捕獲 lambda 轉成函數指標: 可以（ok(7)=true）
// 有捕獲 lambda 轉成函數指標: 編譯錯誤——狀態塞不進一個位址
//   （有捕獲的仍可直接用: withCapture(7)=true）
// sizeof(無捕獲 lambda) = 1，sizeof(捕獲一個 int 的 lambda) = 4
//
// === 極限二：型別是否獨一無二，決定能不能內聯 ===
// 三種寫法結果相同: 500 / 500 / 500
// 但型別完全不同：
//   函數指標 : 所有 bool(*)(int) 共用一個型別
//   functor  : IsEvenFunctor 是獨立型別，內容編譯期可見
//   lambda   : 每次書寫都產生新的匿名類別
// → 效能差距與最佳化等級高度相關，實測數據印在 stderr，
//   因為它每次執行都不同，不該當成固定結果。
//
// === LeetCode 27. Remove Element ===
// 寫死版 remove 2 from [3,2,2,3] -> k=2, 前 k 個 = 3 3   ← 只能移除寫死的 2
// functor 版 remove 2      -> k=5, 前 k 個 = 0 1 3 0 4
// lambda 版 remove 3       -> k=7, 前 k 個 = 0 1 2 2 0 4 2
// → val 是執行期參數，普通函數存不了它，只能換工具。
//
// === 日常實務：函數指標表做 log 過濾 ===
// 只看 ERROR（2 行）:
//     10:00:07 [ERROR] db connect failed
//     10:00:31 [ERROR] timeout on /api/user
// ERROR + WARN（3 行）:
//     10:00:07 [ERROR] db connect failed
//     10:00:09 [WARN]  retrying
//     10:00:31 [ERROR] timeout on /api/user
// 全部（6 行）:
//     10:00:01 [INFO]  service started
//     10:00:07 [ERROR] db connect failed
//     10:00:09 [WARN]  retrying
//     10:00:15 [INFO]  connected
//     10:00:31 [ERROR] timeout on /api/user
//     10:00:44 [DEBUG] cache hit ratio 0.93
// → 規則固定、不需參數化時，函數指標表最簡潔：
//   資料驅動、要加規則只需在表格多一列。
