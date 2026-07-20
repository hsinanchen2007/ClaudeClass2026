// =============================================================================
//  第 15 課：vector 元素刪除 11  —  C++20 的 std::erase / std::erase_if
// =============================================================================
//
// ⚠️⚠️ 本檔需要 **C++20**，不能用 -std=c++17 編譯。⚠️⚠️
//      編譯指令見檔案最下方。這是本課唯一需要 C++20 的檔案，
//      因為它的主題本身就是 C++20 新增的 API。
//      驗證方式：g++ -std=c++17 -pedantic-errors 會明確報
//      「'erase' is not a member of 'std'」。
//
// 【主題資訊 Information】
//   template<class T, class Alloc, class U>
//   typename vector<T, Alloc>::size_type
//   erase(vector<T, Alloc>& c, const U& value);                     // (1)
//
//   template<class T, class Alloc, class Pred>
//   typename vector<T, Alloc>::size_type
//   erase_if(vector<T, Alloc>& c, Pred pred);                       // (2)
//
//   標頭檔：<vector>（不是 <algorithm>！它們是「容器的」自由函式）
//   標準版本：**C++20**（P1209R0）。
//   複雜度：O(n)，與 erase-remove 慣用法完全相同。
//   回傳：【被刪除的元素個數】—— 這是舊慣用法沒有的資訊。
//   注意：它們是【自由函式】不是成員函式，寫成 std::erase(v, x)
//         而不是 v.erase(x)。
//
// 【詳細解釋 Explanation】
//
// 【1. 它解決了什麼問題】
//   C++20 之前，刪除「所有符合條件的元素」必須寫：
//       v.erase(std::remove_if(v.begin(), v.end(), pred), v.end());
//   這行有四個容易出錯的地方：
//     ① 忘記外層的 erase（最經典的錯誤，size 完全沒變）
//     ② 第二個參數寫成別的東西而不是 v.end()
//     ③ 容器名字打錯（v.erase 配 w.begin()）
//     ④ 完全看不出「刪了幾個」
//   C++20 把整件事收編成一個函式：
//       auto n = std::erase_if(v, pred);
//   一行、不可能寫錯、而且回傳刪除數量。
//
// 【2. 為什麼是自由函式而不是成員函式】
//   因為 vector 已經有一個成員函式叫 erase（吃迭代器），
//   再加一個吃「值」的成員 erase 會造成重載歧義與混淆。
//   而且標準函式庫為【所有】序列容器都提供了對應的
//   std::erase / std::erase_if 自由函式：
//       vector, deque, list, forward_list, string, basic_string
//   （關聯容器 map/set 則是提供 erase_if，因為它們的 erase(key)
//     成員函式已經存在）。
//   用自由函式的形式，可以一致地涵蓋所有容器，各自用最有效率的實作——
//   例如 list 的版本內部用的是節點接合，不必搬移資料。
//
// 【3. 回傳「刪除數量」為什麼重要】
//   舊寫法要取得這個數字得多寫兩行：
//       auto ne = std::remove_if(v.begin(), v.end(), pred);
//       auto n  = std::distance(ne, v.end());
//       v.erase(ne, v.end());
//   而且不能把計數器塞進述詞裡——標準未規定述詞被呼叫幾次。
//   新版本直接回傳，語意也更清楚。
//   實務上這個數字很常用：「清理了 N 筆過期資料」「過濾掉 N 筆髒資料」
//   這類日誌與監控指標都需要它。
//
// 【4. std::erase(v, value) 與 std::erase_if(v, pred) 的分工】
//     std::erase(v, 2)                        刪除所有【等於】2 的元素
//     std::erase_if(v, [](int x){...})        刪除所有【符合述詞】的元素
//   前者用 operator== 比較，等價於 erase + remove；
//   後者用你給的述詞，等價於 erase + remove_if。
//   只是刪特定值時用 erase 更簡潔，不必寫 lambda。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 要在 C++17 專案裡先用怎麼辦
//     自己寫一個同名的輔助函式即可，實作就是舊慣用法：
//         template <class T, class Pred>
//         std::size_t erase_if_compat(std::vector<T>& c, Pred p) {
//             auto it = std::remove_if(c.begin(), c.end(), p);
//             auto n  = static_cast<std::size_t>(std::distance(it, c.end()));
//             c.erase(it, c.end());
//             return n;
//         }
//     本檔的 main 也放了這個相容版，並驗證兩者結果一致。
//     這樣未來升到 C++20 時只要把呼叫改名，語意完全不變。
//
// (B) 用 feature-test macro 做條件編譯
//     若同一份程式碼要同時支援兩種標準，可以檢查：
//         #if defined(__cpp_lib_erase_if) && __cpp_lib_erase_if >= 202002L
//     這比檢查 __cplusplus 準確得多——後者只告訴你語言版本，
//     不保證標準函式庫真的實作了那個功能。
//     本課的 summary.cpp 用的就是這個做法（它宣稱可用 C++17 編譯）。
//
// (C) 它們仍然是 O(n)，不是魔法
//     std::erase_if 內部就是 erase-remove，複雜度完全相同。
//     它的價值在「不會寫錯」與「回傳數量」，不在效能。
//     若你需要更快，唯一的路仍然是放棄保序（swap-and-pop，見第 13 檔）
//     或換用別的資料結構。
//
// 【注意事項 Pay Attention】
//   1. 本檔需要 C++20。以 -std=c++17 編譯會明確失敗。
//   2. 它們在 <vector> 裡，不是 <algorithm>。
//   3. 是自由函式：寫 std::erase(v, x)，不是 v.erase(x)。
//   4. 回傳的是刪除數量（size_type），不是迭代器。
//   5. 複雜度仍是 O(n)，與 erase-remove 相同——它省的是出錯機會，不是時間。
//   6. 關聯容器（map/set）只有 std::erase_if，沒有 std::erase
//      （因為它們本來就有 erase(key) 成員函式）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】C++20 的 std::erase / erase_if
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++20 的 std::erase_if 相比傳統的 erase-remove 慣用法，
//        好在哪裡？效能有差嗎？
//     答：效能完全相同（內部就是 erase-remove，都是 O(n)）。
//         好處在正確性與資訊量：
//         ① 一行寫完，不可能發生「忘了外層 erase」這種經典錯誤；
//         ② 不會寫錯第二個參數或搞混容器名字；
//         ③ 直接回傳【刪除的數量】，舊寫法要多算一次 std::distance。
//         這是標準函式庫「把容易寫錯的慣用法收編成函式」的典型例子。
//     追問：它為什麼是自由函式而不是成員函式？
//         → 因為 vector 已經有吃迭代器的成員 erase，再加一個吃值的
//           會造成混淆。用自由函式可以一致地涵蓋所有序列容器，
//           而且各容器能用自己最有效率的實作
//           （list 的版本用節點接合，完全不搬移資料）。
//
// 🔥 Q2. 在還不能用 C++20 的專案裡，要怎麼取得同樣的好處？
//     答：自己包一個同名的輔助函式，內部用舊慣用法實作並回傳
//         std::distance 算出的刪除數量。這樣呼叫端的語意就和
//         C++20 一致，未來升級只要改名（或直接刪掉自己的版本）。
//         若要同時支援兩種標準，用 feature-test macro 分流：
//         #if defined(__cpp_lib_erase_if) && __cpp_lib_erase_if >= 202002L
//     追問：為什麼用 __cpp_lib_erase_if 而不是 __cplusplus？
//         → __cplusplus 只表示語言版本，不保證標準函式庫已實作該功能。
//           feature-test macro 是針對「這個功能到底能不能用」的精確判斷。
//
// ⚠️ 陷阱. 「std::erase_if 是 C++20 的新演算法，
//         所以它比舊的 erase-remove 更有效率」——這個推論錯在哪？
//     答：兩者的複雜度與實際工作量完全相同，std::erase_if 的實作
//         內部就是 erase-remove。它沒有引入任何新的演算法，
//         也不會更快。
//         它改善的是【人為錯誤率】與【表達力】：一行寫完、
//         不可能忘記第二步、而且回傳刪除數量。
//         把「介面改善」誤讀成「效能改善」，會讓人在效能瓶頸上
//         找錯方向——真的要更快，得放棄保序（swap-and-pop）
//         或換資料結構。
//     為什麼會錯：預設「新的比較快」。但標準函式庫的演進有很多動機，
//         效能只是其中之一；安全性、正確性、表達力往往更常見。
//         std::erase_if、std::size、std::ssize、structured bindings
//         都屬於「讓正確的寫法變得更容易」，而非「讓程式跑更快」。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <algorithm>
#include <string>
#include <cstddef>

// -----------------------------------------------------------------------------
// C++17 相容版：語意與 std::erase_if 相同，內部就是 erase-remove。
// 放在這裡是為了 ① 展示 C++20 版本內部做的事，② 提供無法升級時的替代方案。
// -----------------------------------------------------------------------------
template <class T, class Pred>
std::size_t erase_if_compat(std::vector<T>& c, Pred p) {
    auto it = std::remove_if(c.begin(), c.end(), p);
    auto n  = static_cast<std::size_t>(std::distance(it, c.end()));
    c.erase(it, c.end());
    return n;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】監控系統：清理失效的指標資料並回報清理量
//   情境：監控 agent 每分鐘收集一批指標，寫入前要濾掉
//         ① 明顯異常的值（感測器故障時常見的 -999）
//         ② 名稱為空的項目（上游程式的 bug）
//         而且必須把「這輪清掉幾筆」寫進日誌，供後續追蹤資料品質。
//   為什麼用到本主題：這正是 std::erase_if 回傳值最有價值的場合——
//     舊寫法要多算一次 std::distance 才能得到這個數字，
//     而且絕不能把計數器塞進述詞裡（標準未規定述詞被呼叫幾次）。
//   保序性：std::erase_if 與 erase-remove 一樣保證維持原相對順序。
// -----------------------------------------------------------------------------
struct Metric {
    std::string name;
    double      value;
};

struct CleanReport {
    std::size_t removedBadValue;
    std::size_t removedNoName;
    std::size_t remaining;
};

CleanReport cleanMetrics(std::vector<Metric>& metrics) {
    CleanReport r{};

    // 兩次 erase_if，各自回傳清掉的數量
    r.removedBadValue = std::erase_if(metrics, [](const Metric& m) {
        return m.value <= -999.0;               // 感測器故障的哨兵值
    });

    r.removedNoName = std::erase_if(metrics, [](const Metric& m) {
        return m.name.empty();                  // 上游 bug 造成的空名稱
    });

    r.remaining = metrics.size();
    return r;
}

int main() {
    std::cout << "=== 原始示範：std::erase / std::erase_if（C++20）===\n";

    std::vector<int> v1 = {1, 2, 3, 2, 4, 2, 5};
    std::cout << "v1 原始: ";
    for (int x : v1) std::cout << x << " ";
    std::cout << "  (size=" << v1.size() << ")\n";

    // C++20：刪除所有值為 2 的元素
    auto count1 = std::erase(v1, 2);
    std::cout << "刪除了 " << count1 << " 個 2: ";
    for (int x : v1) std::cout << x << " ";  // 1 3 4 5
    std::cout << std::endl;

    std::vector<int> v2 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // C++20：刪除所有偶數
    auto count2 = std::erase_if(v2, [](int x) { return x % 2 == 0; });
    std::cout << "刪除了 " << count2 << " 個偶數: ";
    for (int x : v2) std::cout << x << " ";  // 1 3 5 7 9
    std::cout << std::endl;

    std::cout << "\n=== 對照：C++20 版 vs 傳統 erase-remove ===\n";
    {
        std::vector<int> a = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        std::vector<int> b = a;

        auto pred = [](int x) { return x % 3 == 0; };

        auto n1 = std::erase_if(a, pred);                 // C++20，一行

        auto ne = std::remove_if(b.begin(), b.end(), pred);   // 傳統，三行
        auto n2 = static_cast<std::size_t>(std::distance(ne, b.end()));
        b.erase(ne, b.end());

        std::cout << "std::erase_if      -> 刪除 " << n1 << " 個, 結果: ";
        for (int x : a) std::cout << x << " ";
        std::cout << "\n";
        std::cout << "erase-remove(3 行) -> 刪除 " << n2 << " 個, 結果: ";
        for (int x : b) std::cout << x << " ";
        std::cout << "\n";
        std::cout << "結果完全相同: " << std::boolalpha << (a == b && n1 == n2) << "\n";
        std::cout << "→ 效能也相同（內部就是 erase-remove）。\n";
        std::cout << "  新版省的是「出錯機會」與「多算一次 distance」，不是時間。\n";
    }

    std::cout << "\n=== C++17 相容版：語意一致，未來升級只要改名 ===\n";
    {
        std::vector<int> a = {5, 1, 5, 2, 5, 3};
        std::vector<int> b = a;

        auto pred = [](int x) { return x == 5; };
        auto n1 = std::erase_if(a, pred);        // C++20
        auto n2 = erase_if_compat(b, pred);      // 自己包的相容版

        std::cout << "std::erase_if     -> 刪除 " << n1 << " 個, 結果: ";
        for (int x : a) std::cout << x << " ";
        std::cout << "\n";
        std::cout << "erase_if_compat   -> 刪除 " << n2 << " 個, 結果: ";
        for (int x : b) std::cout << x << " ";
        std::cout << "\n";
        std::cout << "兩者一致: " << (a == b && n1 == n2) << "\n";
    }

    std::cout << "\n=== erase 與 erase_if 的分工 ===\n";
    {
        std::vector<std::string> a = {"apple", "", "banana", "", "cherry"};
        auto n1 = std::erase(a, std::string{});     // 刪除所有「等於空字串」的
        std::cout << "std::erase(v, \"\") 刪除 " << n1 << " 個空字串: ";
        for (const auto& s : a) std::cout << "[" << s << "] ";
        std::cout << "\n";

        std::vector<std::string> b = {"a", "bbbb", "cc", "ddddd", "e"};
        auto n2 = std::erase_if(b, [](const std::string& s) { return s.size() > 2; });
        std::cout << "std::erase_if(長度>2) 刪除 " << n2 << " 個: ";
        for (const auto& s : b) std::cout << "[" << s << "] ";
        std::cout << "\n";
        std::cout << "→ 刪特定值用 erase（不必寫 lambda）；\n";
        std::cout << "  依條件刪用 erase_if。\n";
    }

    std::cout << "\n=== 保序性：與 erase-remove 一樣保證維持原順序 ===\n";
    {
        std::vector<int> order = {10, 3, 20, 7, 30, 1, 40};
        std::erase_if(order, [](int x) { return x < 10; });
        std::cout << "刪除小於 10 的 -> ";
        for (int x : order) std::cout << x << " ";
        std::cout << "  ← 10,20,30,40 維持原相對順序\n";
    }

    std::cout << "\n=== 日常實務：清理監控指標並回報清理量 ===\n";
    {
        std::vector<Metric> metrics = {
            {"cpu.usage",     42.5},
            {"mem.used",      -999.0},    // 感測器故障
            {"disk.io",       128.0},
            {"",              55.0},      // 上游 bug：名稱是空的
            {"net.rx",        -999.0},    // 感測器故障
            {"net.tx",        340.5},
            {"",              -999.0},    // 兩種問題都有
            {"load.avg",      1.25},
        };

        std::cout << "收集到 " << metrics.size() << " 筆指標\n";
        CleanReport rep = cleanMetrics(metrics);

        std::cout << "清理報告：\n";
        std::cout << "  異常值(-999) 清掉: " << rep.removedBadValue << " 筆\n";
        std::cout << "  空名稱      清掉: " << rep.removedNoName << " 筆\n";
        std::cout << "  剩餘可用        : " << rep.remaining << " 筆\n";
        std::cout << "有效指標：\n";
        for (const auto& m : metrics) {
            std::cout << "  " << m.name << " = " << m.value << "\n";
        }
        std::cout << "→ 「清掉幾筆」是資料品質監控的關鍵指標，\n";
        std::cout << "  erase_if 直接回傳它，不必自己算 distance，\n";
        std::cout << "  更不能把計數器塞進述詞（述詞的呼叫次數未規定）。\n";
    }

    return 0;
}

// 編譯（必須用 C++20）:
//     g++ -std=c++20 -Wall -Wextra 第 15 課：vector 元素刪除11.cpp -o demo11
//
// 用 C++17 編譯會失敗，錯誤訊息是：
//     error: 'erase' is not a member of 'std'
//     error: 'erase_if' is not a member of 'std'; did you mean 'enable_if'?
// 這正是本檔要示範的 API 屬於 C++20 的證明。
// 無法升級到 C++20 的專案，請用本檔提供的 erase_if_compat，
// 或直接寫 erase-remove 慣用法（見第 10 檔）。

// === 預期輸出 ===
// === 原始示範：std::erase / std::erase_if（C++20）===
// v1 原始: 1 2 3 2 4 2 5   (size=7)
// 刪除了 3 個 2: 1 3 4 5
// 刪除了 5 個偶數: 1 3 5 7 9
//
// === 對照：C++20 版 vs 傳統 erase-remove ===
// std::erase_if      -> 刪除 3 個, 結果: 1 2 4 5 7 8 10
// erase-remove(3 行) -> 刪除 3 個, 結果: 1 2 4 5 7 8 10
// 結果完全相同: true
// → 效能也相同（內部就是 erase-remove）。
//   新版省的是「出錯機會」與「多算一次 distance」，不是時間。
//
// === C++17 相容版：語意一致，未來升級只要改名 ===
// std::erase_if     -> 刪除 3 個, 結果: 1 2 3
// erase_if_compat   -> 刪除 3 個, 結果: 1 2 3
// 兩者一致: true
//
// === erase 與 erase_if 的分工 ===
// std::erase(v, "") 刪除 2 個空字串: [apple] [banana] [cherry]
// std::erase_if(長度>2) 刪除 2 個: [a] [cc] [e]
// → 刪特定值用 erase（不必寫 lambda）；
//   依條件刪用 erase_if。
//
// === 保序性：與 erase-remove 一樣保證維持原順序 ===
// 刪除小於 10 的 -> 10 20 30 40   ← 10,20,30,40 維持原相對順序
//
// === 日常實務：清理監控指標並回報清理量 ===
// 收集到 8 筆指標
// 清理報告：
//   異常值(-999) 清掉: 3 筆
//   空名稱      清掉: 1 筆
//   剩餘可用        : 4 筆
// 有效指標：
//   cpu.usage = 42.5
//   disk.io = 128
//   net.tx = 340.5
//   load.avg = 1.25
// → 「清掉幾筆」是資料品質監控的關鍵指標，
//   erase_if 直接回傳它，不必自己算 distance，
//   更不能把計數器塞進述詞（述詞的呼叫次數未規定）。
