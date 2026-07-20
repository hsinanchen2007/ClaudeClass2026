// =============================================================================
//  第七課：演算法（Algorithm）與容器的分離設計7.cpp
//    —  std::transform：一元／二元轉換，函數式風格的核心
// =============================================================================
//
// 【主題資訊 Information】
//   OutIt transform(InputIt  f1, InputIt  l1, OutIt d_first, UnaryOp  op);   // 一元
//   OutIt transform(InputIt1 f1, InputIt1 l1, InputIt2 f2,
//                   OutIt d_first, BinaryOp op);                             // 二元
//
//   標準版本：C++98 起（C++17 加執行策略多載、C++20 加 constexpr）
//   迭代器需求：Input Iterator（來源）+ Output Iterator（目的地）
//   複雜度：恰好套用 op 共 last1 - first1 次
//   回傳：目的地最後寫入位置的下一個迭代器
//   標頭檔：<algorithm>
//
// 【詳細解釋 Explanation】
//
// 【1. transform 就是函數式語言的 map】
// 在 Haskell 是 map、在 Python 是 map()、在 JavaScript 是 Array.prototype.map，
// 在 C++ 就是 std::transform：**把一個序列的每個元素套用函數，產生新序列**。
// 它與 for_each 的分工很清楚：
//     for_each   → 為了副作用（印出、寫檔、累加）；不關心回傳值
//     transform  → 為了產生新值；op 的回傳值就是寫進目的地的東西
// 一個常見的壞味道是「用 for_each + 捕獲參考的 lambda 手動 push_back」，
// 那其實就是 transform，直接用 transform 意圖更清楚。
//
// 【2. 就地轉換是合法的：d_first 可以等於 first】
//     std::transform(v.begin(), v.end(), v.begin(), op);   // ✓ 合法且常用
// 標準明確允許目的地與來源是同一個範圍（一元版本），因為 op 是逐元素套用、
// 讀完當前元素才寫回同一位置，不會踩到還沒讀的資料。
// 但**不可以**把目的地設成來源範圍中間的某個位置（部分重疊），那是未定義行為。
// 二元版本同理：d_first 可以等於 first1 或 first2，但不能是它們中間。
//
// 【3. 二元版本只給第二個範圍的起點——這是個地雷】
//     std::transform(v1.begin(), v1.end(), v2.begin(), out.begin(), op);
// 注意 v2 只給了起點。若 v2 比 v1 短，就會讀過頭 → 未定義行為。
// 這與 equal 的三參數版是同一類問題（見本課第 2 個檔案），
// 但 transform **至今沒有**四參數的安全版本，所以責任完全在呼叫端：
// **使用二元 transform 前務必自行確認第二個範圍夠長。**
//
// 【4. 目的地一樣不會自動成長】
// 和 copy 一樣，transform 只拿到 d_first。本檔的 squares 先用
// std::vector<int> squares(src.size()) 配置好空間，就是這個原因。
// 大小未知時用 std::back_inserter(dest)。
//
// 【概念補充 Concept Deep Dive】
//
// (A) ★ std::toupper 與 char 的經典 UB（本檔已修正）
//   很多教材寫成：
//       std::transform(s.begin(), s.end(), s.begin(), ::toupper);
//   這在**只有 ASCII 時碰巧正確**，但嚴格說是未定義行為。原因：
//   <cctype> 的 toupper 要求傳入值必須可表示為 unsigned char，或等於 EOF。
//   而 char 在 x86 Linux（含本機 g++ 15.2）預設是**有號的**，
//   遇到 UTF-8 中文或 Latin-1 重音字元時，那個 byte 的值會是負數
//   （例如 0xE4 以 signed char 解讀是 -28），既不在 unsigned char 範圍內
//   也不是 EOF → 未定義行為（實務上常見的後果是查表越界）。
//   **正確寫法是先轉成 unsigned char：**
//       [](unsigned char c) { return static_cast<char>(std::toupper(c)); }
//   本檔已採用此寫法。注意 char 是否有號是**實作定義**的
//   （x86/x86-64 Linux 為有號，ARM Linux 預設為無號），
//   所以「在我的機器上沒事」不能作為正確性依據。
//
// (B) 為什麼 transform 不保證套用順序
//   標準只保證 op 被套用「恰好 N 次」，**不保證順序**（這點與 for_each 不同，
//   for_each 明文保證依序）。因此 op 若帶有依賴順序的副作用，結果不可預期。
//   op 應該是純函式。需要依序的副作用請改用 for_each。
//
// (C) 回傳值與串接
//   transform 回傳目的地的結束位置，可以像 copy 一樣串接多段輸出，
//   也可以用來計算「實際寫了幾個」（雖然一元版本必然等於輸入長度）。
//
// 【注意事項 Pay Attention】
// 1. **目的地不會自動成長**；要嘛先配置好大小，要嘛用 std::back_inserter。
// 2. 二元版本**只給第二個範圍的起點**，沒有安全的四參數版；
//    第二個範圍太短就是未定義行為，必須自行確保長度。
// 3. 就地轉換（d_first == first）合法；**部分重疊**則是未定義行為。
// 4. 傳 ::toupper / ::tolower 給 char 序列時，**必須先轉 unsigned char**，
//    否則對非 ASCII 位元組是未定義行為。char 是否有號屬實作定義。
// 5. op **不保證套用順序**（與 for_each 不同），不要依賴副作用的先後。
// 6. 只是想產生副作用（印出、寫檔）請用 for_each；transform 是為了「產生值」。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::transform
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. transform 和 for_each 有什麼差別？各自什麼時候用？
//     答：transform 關心 op 的**回傳值**，把它寫進目的地範圍，是函數式的 map，
//         用於「由舊序列產生新序列」；for_each 忽略回傳值，用於**副作用**
//         （印出、寫檔、累積統計）。另外 for_each 明文保證依序呼叫且恰好一次，
//         transform 只保證次數不保證順序。
//     追問：那 transform 可以就地修改嗎？→ 可以，把 d_first 設成 first 即可，
//         標準明確允許；但目的地與來源**部分重疊**則是未定義行為。
//
// 🔥 Q2. 為什麼 std::transform(s.begin(), s.end(), s.begin(), ::toupper)
//        對含中文的 std::string 是有問題的？
//     答：<cctype> 的 toupper 要求引數必須可表示為 unsigned char 或等於 EOF。
//         x86 Linux 上 char 預設有號，UTF-8 中文的位元組（如 0xE4）解讀成
//         signed char 是負數，兩個條件都不滿足 → 未定義行為。
//         正確寫法是 [](unsigned char c){ return static_cast<char>(std::toupper(c)); }。
//     追問：那全形中文可以正確轉大寫嗎？→ 不能。std::string 是位元組序列，
//         一個中文字在 UTF-8 下佔 3 個 byte，逐 byte 轉換沒有意義。
//         多語系大小寫轉換需要 ICU 這類函式庫。
//
// ⚠️ 陷阱. std::transform(v1.begin(), v1.end(), v2.begin(), out.begin(), op);
//        當 v2 比 v1 短時會發生什麼？
//     答：讀取越界，未定義行為。二元版本只給第二個範圍的**起點**，
//         演算法會依 v1 的長度不斷推進 v2 的迭代器，v2 用完後繼續讀就越界了。
//     為什麼會錯：大家記得 equal 在 C++14 有了四參數安全版，就以為 transform
//         也有——**它沒有**。這裡沒有任何安全網，長度檢查完全是呼叫端的責任。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <iterator>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 977. Squares of a Sorted Array
//   題目：給定非遞減排序的整數陣列 nums（可能含負數），
//         回傳每個元素平方後、仍為非遞減排序的陣列。
//   為什麼用到本主題：transform 負責「每個元素平方」（一元 map），
//         由於負數平方後大小順序會反轉，還需要再排序一次。
//   複雜度：O(N log N)。
//   註：雙指標由外往內填可做到 O(N)；這裡示範的是本主題（transform）的解法。
// -----------------------------------------------------------------------------
std::vector<int> sortedSquares(const std::vector<int>& nums) {
    std::vector<int> result(nums.size());          // 先配置好，transform 不會成長
    std::transform(nums.begin(), nums.end(), result.begin(),
                   [](int n) { return n * n; });
    std::sort(result.begin(), result.end());
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】CSV 欄位轉換：溫度單位換算 + 產出報表欄
//   情境：資料管線收到一批攝氏溫度讀值，下游系統要求華氏，
//         同時要產生「與前一小時基準的差值」欄位供異常偵測使用。
//   為什麼用到本主題：單位換算是一元 transform（每筆各自轉換）；
//         與基準相減是二元 transform（兩個等長序列逐項運算）。
//         這正是資料管線最常見的兩種 transform 用法。
// -----------------------------------------------------------------------------
void buildTemperatureReport(const std::vector<double>& celsius,
                            const std::vector<double>& baseline) {
    // 一元 transform：攝氏 -> 華氏
    std::vector<double> fahrenheit(celsius.size());
    std::transform(celsius.begin(), celsius.end(), fahrenheit.begin(),
                   [](double c) { return c * 9.0 / 5.0 + 32.0; });

    std::cout << "  攝氏: ";
    for (double c : celsius) std::cout << c << " ";
    std::cout << std::endl;
    std::cout << "  華氏: ";
    for (double f : fahrenheit) std::cout << f << " ";
    std::cout << std::endl;

    // 二元 transform：與基準相減
    // ★ 二元版沒有安全的四參數版本，長度必須自己確認
    if (baseline.size() < celsius.size()) {
        std::cout << "  [SKIP] 基準資料長度不足，二元 transform 會讀越界" << std::endl;
        return;
    }
    std::vector<double> delta(celsius.size());
    std::transform(celsius.begin(), celsius.end(), baseline.begin(), delta.begin(),
                   [](double now, double base) { return now - base; });
    std::cout << "  與基準差: ";
    for (double d : delta) std::cout << d << " ";
    std::cout << std::endl;
}

int main() {
    // 一元 transform：對每個元素套用轉換, 產生一個新的範圍
    std::cout << "=== 一元 transform ===" << std::endl;
    std::vector<int> src = {1, 2, 3, 4, 5};
    std::vector<int> squares(src.size());   // ★ 先配置空間

    std::transform(src.begin(), src.end(), squares.begin(),
        [](int n) { return n * n; });

    std::cout << "平方: ";
    for (int n : squares) std::cout << n << " ";
    std::cout << std::endl;

    // 二元 transform：結合兩個範圍, 產生一個新的範圍
    // ★ v2 只給起點！v2 必須至少和 v1 一樣長，否則讀越界
    std::cout << "\n=== 二元 transform ===" << std::endl;
    std::vector<int> v1 = {1, 2, 3, 4, 5};
    std::vector<int> v2 = {10, 20, 30, 40, 50};
    std::vector<int> sums(v1.size());

    std::transform(v1.begin(), v1.end(), v2.begin(), sums.begin(),
        [](int a, int b) { return a + b; });

    std::cout << "v1 + v2: ";
    for (int n : sums) std::cout << n << " ";
    std::cout << std::endl;

    // 實用案例：字串轉大寫, 使用 lambda 表達式將每個字元轉換為大寫
    // ★ 必須先轉 unsigned char：char 在 x86 Linux 是有號的，
    //   非 ASCII 位元組傳給 toupper 是未定義行為
    std::cout << "\n=== 字串轉大寫（就地轉換）===" << std::endl;
    std::string str = "Hello, World!";
    std::transform(str.begin(), str.end(), str.begin(),
        [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    std::cout << "轉換後: " << str << std::endl;

    // ★ 目的地大小未知時用 back_inserter
    std::cout << "\n=== 目的地大小未知：用 back_inserter ===" << std::endl;
    std::vector<std::string> names = {"alice", "bob", "carol"};
    std::vector<std::size_t> lengths;
    std::transform(names.begin(), names.end(), std::back_inserter(lengths),
                   [](const std::string& s) { return s.size(); });
    std::cout << "各名字長度: ";
    for (std::size_t n : lengths) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "\n=== LeetCode 977. Squares of a Sorted Array ===" << std::endl;
    std::cout << "[-4,-1,0,3,10] -> ";
    for (int n : sortedSquares({-4, -1, 0, 3, 10})) std::cout << n << " ";
    std::cout << std::endl;
    std::cout << "[-7,-3,2,3,11]  -> ";
    for (int n : sortedSquares({-7, -3, 2, 3, 11})) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "\n=== 日常實務：溫度資料欄位轉換 ===" << std::endl;
    buildTemperatureReport({20.0, 22.5, 25.0}, {19.0, 22.0, 26.5});
    std::cout << "  --- 基準長度不足的情況 ---" << std::endl;
    buildTemperatureReport({20.0, 22.5, 25.0}, {19.0});

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第七課：演算法（Algorithm）與容器的分離設計7.cpp -o demo7

// === 預期輸出 ===
// === 一元 transform ===
// 平方: 1 4 9 16 25
//
// === 二元 transform ===
// v1 + v2: 11 22 33 44 55
//
// === 字串轉大寫（就地轉換）===
// 轉換後: HELLO, WORLD!
//
// === 目的地大小未知：用 back_inserter ===
// 各名字長度: 5 3 5
//
// === LeetCode 977. Squares of a Sorted Array ===
// [-4,-1,0,3,10] -> 0 1 9 16 100
// [-7,-3,2,3,11]  -> 4 9 9 49 121
//
// === 日常實務：溫度資料欄位轉換 ===
//   攝氏: 20 22.5 25
//   華氏: 68 72.5 77
//   與基準差: 1 0.5 -1.5
//   --- 基準長度不足的情況 ---
//   攝氏: 20 22.5 25
//   華氏: 68 72.5 77
//   [SKIP] 基準資料長度不足，二元 transform 會讀越界
