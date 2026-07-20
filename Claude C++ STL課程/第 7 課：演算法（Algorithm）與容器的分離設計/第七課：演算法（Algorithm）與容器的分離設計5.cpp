// =============================================================================
//  第七課：演算法（Algorithm）與容器的分離設計5.cpp
//    —  std::for_each：唯一「會回傳函式物件」的演算法
// =============================================================================
//
// 【主題資訊 Information】
//   template<class InputIt, class UnaryFunc>
//   UnaryFunc for_each(InputIt first, InputIt last, UnaryFunc f);        // C++98
//
//   template<class InputIt, class Size, class UnaryFunc>
//   InputIt for_each_n(InputIt first, Size n, UnaryFunc f);              // C++17 ★
//
//   標準版本：for_each 為 C++98；**for_each_n 是 C++17 新增**
//   迭代器需求：Input Iterator
//   複雜度：恰好呼叫 f 共 last - first 次（**這是標準明文保證的**）
//   回傳：**for_each 回傳 f 本身（C++11 起是 std::move(f)）**，for_each_n 回傳前進後的迭代器
//   標頭檔：<algorithm>
//
// 【詳細解釋 Explanation】
//
// 【1. for_each 的回傳值是它唯一不可取代的價值】
// 幾乎所有 for_each 的用途都能被 C++11 的 range-based for 取代，而且後者更短：
//     for (int n : vec) std::cout << n << " ";              // 通常首選
//     std::for_each(vec.begin(), vec.end(), [](int n){...}); // 較囉唆
// 那 for_each 還剩什麼？答案是**它會把函式物件回傳給你**。
// 如果你的「操作」本身帶有狀態（例如邊走邊統計），for_each 走完之後
// 把累積了狀態的物件交還，這是 range-for 做不到的：
//     Sum result = std::for_each(v.begin(), v.end(), Sum());
//     result.total   // ← 走訪過程累積的結果
// 這在 lambda 出現前是「邊走訪邊累積」的標準寫法。
//
// 【2. 為什麼有了 lambda 捕獲之後，這個特性變得不那麼必要】
// C++11 之後可以直接用捕獲參考的 lambda：
//     int sum = 0;
//     std::for_each(v.begin(), v.end(), [&sum](int n){ sum += n; });
// 狀態放在外面的區域變數，不需要靠回傳值拿回來。所以現代程式碼中
// 「回傳函式物件」的用法已相對少見。但它仍有兩個場合有意義：
//   (a) 函式物件是可重用的具名型別（例如統計器 class），語意比 lambda 清楚；
//   (b) 需要把「操作 + 狀態」當成一個單位傳遞、複製或序列化時。
// 面試時這仍是高頻題，因為它測的是「你知不知道 for_each 有回傳值」。
//
// 【3. for_each 修改元素：參數必須是參考】
//     std::for_each(v.begin(), v.end(), [](int& n) { n *= 2; });   // ✓ 會改到
//     std::for_each(v.begin(), v.end(), [](int  n) { n *= 2; });   // ✗ 改的是副本
// 這是最常見的 for_each 錯誤——沒有 &，改的是傳值進來的副本，
// 容器完全不會變，而且**編譯器不會警告**（那是合法程式碼）。
// 順帶一提，這也說明 for_each 與 transform 的分工：
//   for_each   → 就地修改或產生副作用（印出、寫檔、累積）
//   transform  → 產生新值寫到輸出範圍（函數式風格，不改輸入）
//
// 【4. for_each 是唯一「明文保證呼叫次數與順序」的演算法】
// 標準對 for_each 的規定是：對 [first, last) 中的每個迭代器，
// **依序**（in order）呼叫 f 恰好一次。這與 count_if / find_if 不同——
// 那些演算法的謂詞呼叫次數不受保證（可能提早結束、可能被向量化）。
// 因此 for_each 是少數「允許謂詞有副作用」的演算法。
// 注意：C++17 加上執行策略（如 std::execution::par）之後，
// **平行版本就不再保證順序**，此時副作用必須自行同步。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼函式物件是傳值（by value）而不是傳參考
//   for_each 的簽名是 UnaryFunc f（傳值），所以你傳進去的 Sum() 會被複製。
//   這正是為什麼**必須用回傳值取回結果**——你在外面建立的那個物件不會被改到。
//   若寫成：
//       Sum s;  std::for_each(v.begin(), v.end(), s);  std::cout << s.total;
//   會印出 0，因為被走訪的是 s 的副本。這是經典陷阱。
//
// (B) C++11 起回傳的是 std::move(f)
//   標準在 C++11 把回傳改成 std::move(f)，讓帶有大量狀態或不可複製移動
//   語意的函式物件也能高效回傳。這對使用者是透明的，但解釋了為什麼
//   「傳進去的是副本、回傳的是那個副本」在效能上不成問題。
//
// (C) for_each_n（C++17）與迴圈的差別
//   for_each_n(first, n, f) 對前 n 個元素套用 f，回傳前進 n 步後的迭代器。
//   它的價值在於**不需要知道 last**——處理串流或只想處理前 N 筆時很方便，
//   而且回傳的迭代器可以直接接著下一段處理，形成分批（batch）流程。
//
// 【注意事項 Pay Attention】
// 1. **要修改元素，lambda 參數必須寫成參考**（int&）。寫成傳值不會有任何警告，
//    但容器完全不會被改到。
// 2. 函式物件是**傳值**進去的；不用回傳值就拿不到累積的狀態。
// 3. 單純走訪請優先用 range-based for，更短更清楚；
//    for_each 的價值在「回傳函式物件」與「需要明確 iterator 範圍（非整個容器）」。
// 4. for_each_n 是 C++17；-std=c++11 編不過。
// 5. 加上執行策略的平行版本**不再保證呼叫順序**，副作用需自行同步。
// 6. for_each 無法提早中斷。需要「找到就停」請用 find_if / any_of，
//    別在 for_each 裡丟例外來當 break 用。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::for_each
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++11 已經有 range-based for 了，for_each 還有什麼存在價值？
//     答：三點：(1) **它會回傳函式物件**，可取回走訪過程累積的狀態，
//         這是 range-for 做不到的；(2) 它接受任意 iterator 範圍，
//         可以只處理容器的一部分，range-for 只能整個容器跑；
//         (3) C++17 起可加執行策略做平行走訪。
//     追問：那什麼時候該用 range-for？→ 單純走訪整個容器時一律用 range-for，
//         更短、更不容易寫錯範圍。
//
// 🔥 Q2. 這段程式為什麼印出 0？
//        Sum s; std::for_each(v.begin(), v.end(), s); std::cout << s.total;
//     答：因為 for_each 的函式物件參數是**傳值**，被走訪的是 s 的副本，
//         原本的 s 完全沒被改到。正確寫法是接回傳值：
//         Sum r = std::for_each(v.begin(), v.end(), s); 然後看 r.total。
//     追問：那為什麼標準不改成傳參考？→ 傳值讓演算法能接受暫時物件
//         （如 Sum()）與 lambda，語意也更一致；需要狀態就用回傳值取回，
//         C++11 起回傳的是 std::move(f)，沒有額外複製成本。
//
// ⚠️ 陷阱. std::for_each(v.begin(), v.end(), [](int n) { n *= 2; });
//        跑完為什麼 v 沒變？
//     答：lambda 參數 int n 是傳值，每次拿到的是元素的副本，
//         乘 2 之後副本就被丟棄。要真正改到容器內容必須寫 [](int& n)。
//     為什麼會錯：一般人把 for_each 想成「就地對每個元素動手」，
//         但實際傳給 lambda 的是什麼，完全由你自己宣告的參數型別決定。
//         而且這是**合法程式碼，編譯器不會警告**，所以特別容易漏掉。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

// -----------------------------------------------------------------------------
// 【日常實務範例 1】Web server log 逐行統計（有狀態的函式物件）
//   情境：掃描一批 HTTP 狀態碼，同時算出 2xx/4xx/5xx 各有幾筆、以及總請求數。
//         這種「一次走訪、同時累積多個指標」正是有狀態函式物件的經典場景——
//         用 count_if 做要掃三次，用 for_each 只掃一次。
//   為什麼用到本主題：示範 for_each 唯一不可取代的特性：回傳累積了狀態的物件。
// -----------------------------------------------------------------------------
struct HttpStatusCollector {
    int total = 0;
    int success = 0;   // 2xx
    int clientErr = 0; // 4xx
    int serverErr = 0; // 5xx

    void operator()(int code) {
        ++total;
        if (code >= 200 && code < 300)      ++success;
        else if (code >= 400 && code < 500) ++clientErr;
        else if (code >= 500 && code < 600) ++serverErr;
    }
};

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5};

    // for_each：對每個元素執行操作, 使用 lambda 表達式輸出元素
    std::cout << "=== for_each（輸出）===" << std::endl;
    std::cout << "元素: ";
    std::for_each(vec.begin(), vec.end(), [](int n) {
        std::cout << n << " ";
    });
    std::cout << std::endl;

    // for_each 也可以修改元素（透過參考）, 使用 lambda 表達式將每個元素乘以 2
    // ★ 關鍵在 int& —— 少了 & 就是改副本，容器不會變，而且編譯器不會警告
    std::cout << "\n=== for_each（修改）===" << std::endl;
    std::for_each(vec.begin(), vec.end(), [](int& n) {
        n *= 2;
    });
    std::cout << "乘以 2 後: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    // ★ 對照：傳值參數改不到容器（合法程式碼，無警告，但完全沒作用）
    std::cout << "\n=== 陷阱對照：參數少了 & ===" << std::endl;
    std::vector<int> copyDemo = {1, 2, 3};
    std::for_each(copyDemo.begin(), copyDemo.end(), [](int n) { n *= 100; });
    std::cout << "用 [](int n) 乘 100 之後: ";
    for (int n : copyDemo) std::cout << n << " ";
    std::cout << "  ← 完全沒變" << std::endl;

    // for_each 會回傳函數物件（可以累積狀態）, 使用 struct 定義一個累積總和的函數物件
    std::cout << "\n=== for_each（累積）===" << std::endl;
    struct Sum {
        int total = 0;
        void operator()(int n) { total += n; }
    };

    Sum result = std::for_each(vec.begin(), vec.end(), Sum());
    std::cout << "總和: " << result.total << std::endl;

    // ★ 對照：函式物件是傳值進去的，外部物件不會被改到
    std::cout << "\n=== 陷阱對照：不接回傳值就拿不到結果 ===" << std::endl;
    Sum outside;
    std::for_each(vec.begin(), vec.end(), outside);   // 走訪的是 outside 的副本
    std::cout << "沒接回傳值的 outside.total = " << outside.total
              << "  ← 仍是 0" << std::endl;

    std::cout << "\n=== 日常實務：一次走訪統計 HTTP 狀態碼 ===" << std::endl;
    std::vector<int> accessLog = {200, 200, 404, 500, 200, 301, 403, 502, 200};
    HttpStatusCollector stats =
        std::for_each(accessLog.begin(), accessLog.end(), HttpStatusCollector());
    std::cout << "總請求數: " << stats.total << std::endl;
    std::cout << "2xx 成功: " << stats.success << std::endl;
    std::cout << "4xx 用戶端錯誤: " << stats.clientErr << std::endl;
    std::cout << "5xx 伺服器錯誤: " << stats.serverErr << std::endl;
    std::cout << "(同一次走訪就算出四個指標；用 count_if 要掃三遍)" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第七課：演算法（Algorithm）與容器的分離設計5.cpp -o demo5

// === 預期輸出 ===
// === for_each（輸出）===
// 元素: 1 2 3 4 5
//
// === for_each（修改）===
// 乘以 2 後: 2 4 6 8 10
//
// === 陷阱對照：參數少了 & ===
// 用 [](int n) 乘 100 之後: 1 2 3   ← 完全沒變
//
// === for_each（累積）===
// 總和: 30
//
// === 陷阱對照：不接回傳值就拿不到結果 ===
// 沒接回傳值的 outside.total = 0  ← 仍是 0
//
// === 日常實務：一次走訪統計 HTTP 狀態碼 ===
// 總請求數: 9
// 2xx 成功: 4
// 4xx 用戶端錯誤: 2
// 5xx 伺服器錯誤: 2
// (同一次走訪就算出四個指標；用 count_if 要掃三遍)
