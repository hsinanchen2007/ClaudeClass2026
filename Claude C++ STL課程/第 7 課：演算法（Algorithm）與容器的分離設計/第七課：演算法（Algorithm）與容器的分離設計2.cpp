// =============================================================================
//  第七課：演算法（Algorithm）與容器的分離設計2.cpp
//    —  STL 演算法的五種參數形式（範圍／值／謂詞／輸出／雙範圍）
// =============================================================================
//
// 【主題資訊 Information】
//   五種常見簽名（皆在 <algorithm>，數值類在 <numeric>）：
//     (1) 只有範圍      f(first, last)                       例：sort, reverse
//     (2) 範圍 + 值     f(first, last, value)                例：count, find, fill
//     (3) 範圍 + 謂詞   f(first, last, pred)                 例：count_if, find_if
//     (4) 範圍 + 輸出   f(first, last, d_first)              例：copy, transform
//     (5) 兩個範圍      f(first1, last1, first2)             例：equal, mismatch
//
//   標準版本：C++98 起；C++14 起 equal / mismatch 增加「雙範圍四參數」安全版；
//             C++17 起多數演算法增加執行策略（execution policy）多載
//   標頭檔：<algorithm>、<numeric>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼是「first, last」而不是「container, size」】
// STL 一律用**半開區間 [first, last)** 表示範圍——包含 first，不含 last。
// 這不是隨便選的，半開區間有三個關鍵好處：
//   (a) 空範圍的表示很自然：first == last 就是空，不需要特例。
//       若用閉區間 [first, last]，空範圍要怎麼表示？last = first - 1？
//       那對「陣列第一個元素」就會產生不存在的位址，是 UB。
//   (b) 長度直接是 last - first，不用 +1，off-by-one 錯誤少一半。
//   (c) 迴圈條件是 while (first != last)，不需要 <=，
//       因此只需要 iterator 支援 !=，不必支援 <（list 的 iterator 就不支援 <）。
// 這也是為什麼 end() 指向「最後一個元素的下一個位置」，而不是最後一個元素。
//
// 【2. 為什麼「值」和「謂詞」要拆成兩個函式（count / count_if）】
// 直覺會想：為什麼不做成一個 count，靠多載自動判斷第三個參數是值還是謂詞？
// 因為**會有歧義**。考慮 std::vector<std::function<bool(int)>>：
// 元素本身就是可呼叫物件，此時「找等於某個 function 的元素」與
// 「找滿足某條件的元素」在型別上無法區分。
// STL 的解法是用**命名**來消歧義（`_if` 後綴），而不是靠多載規則猜測。
// 這是 STL 一貫的態度：寧可名字多一點，也不要有解析上的模稜兩可。
//
// 【3. 輸出迭代器形式為什麼只給 d_first、不給 d_last】
//     std::copy(src.begin(), src.end(), dest.begin());
// 注意目的地只有起點，沒有終點——演算法**不會**幫你檢查目的地夠不夠大。
// 原因還是分離設計：演算法只拿到 iterator，它不知道 dest 是 vector 還是陣列，
// 也無從得知還剩多少空間。寫超過就是緩衝區溢位（未定義行為）。
// 解決方式有兩種：
//   (a) 事先 resize：std::vector<int> dest(src.size());
//   (b) 用 insert iterator：std::back_inserter(dest)——把「寫入」轉成 push_back，
//       容器會自己成長（見本課第 6 個檔案）。
//
// 【4. 雙範圍形式的隱藏地雷（C++14 修正）】
//     std::equal(v1.begin(), v1.end(), v2.begin());   // 三參數版：只給 v2 起點
// 這個版本假設 v2 至少和 v1 一樣長。若 v2 比較短，就會讀過頭 → 未定義行為。
// C++14 因此加入四參數版：
//     std::equal(v1.begin(), v1.end(), v2.begin(), v2.end());  // 安全
// 四參數版會先比長度，長度不同直接回傳 false，不會越界。
// **新程式碼一律用四參數版。**
//
// 【概念補充 Concept Deep Dive】
//
// (A) 謂詞（predicate）到底是什麼型別
//   謂詞不是特定型別，是**任何可以用 pred(x) 呼叫並得到可轉成 bool 的東西**：
//   函式指標、lambda、函式物件（重載 operator() 的 struct）、std::function 都行。
//   演算法用 template 參數接它，所以 lambda 可以被完整 inline，
//   不像 std::function 需要一次間接呼叫（type erasure 的代價）。
//   → 傳 lambda 給 STL 演算法通常比傳 std::function 快。
//
// (B) 謂詞必須是「純函式」
//   標準要求謂詞不得修改傳入的元素，且對相同輸入必須給相同結果。
//   為什麼？因為標準**不保證**謂詞被呼叫的次數與順序
//   （例如 count_if 可以被向量化、sort 的比較次數依實作而定）。
//   若謂詞有副作用（例如內部計數器），結果就不可預測。
//   本檔第五段的 lambda 都是純函式，安全。
//
// (C) 為什麼 count 回傳 difference_type 而不是 size_t
//   std::count 回傳 iterator_traits<It>::difference_type（通常是 ptrdiff_t，有號）。
//   本檔用 int 接是為了教學簡潔且數值很小；
//   實務上大範圍應該用 auto 或 std::ptrdiff_t，避免窄化。
//
// 【注意事項 Pay Attention】
// 1. [first, last) 是半開區間；last 不可解參考（dereference），它不指向有效元素。
// 2. **輸出形式不檢查目的地容量**，寫超過是未定義行為。
//    不確定大小就用 std::back_inserter。
// 3. equal / mismatch 的三參數版要求第二個範圍**至少一樣長**；
//    C++14 起請改用四參數版。
// 4. 謂詞應為純函式、無副作用；呼叫次數與順序不受標準保證。
// 5. count 的回傳型別是有號的 difference_type，與 size() 的無號 size_type 不同，
//    兩者混用比較可能觸發 -Wsign-compare 警告。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】演算法的參數形式與半開區間
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 STL 一律使用半開區間 [first, last)，而不是閉區間 [first, last]？
//     答：三個理由：(1) 空範圍自然表示成 first == last，不需特例；閉區間要表示
//         空範圍得寫 last = first - 1，對陣列首元素會產生無效位址。
//         (2) 長度就是 last - first，不必 +1。
//         (3) 迴圈條件只需 !=，不需要 <，因此非隨機存取的 iterator（如 list）
//         也能用同一套演算法。
//     追問：那 end() 可以解參考嗎？→ 不行，*end() 是未定義行為；
//         它只是個「越界哨兵」位置，不指向任何有效元素。
//
// 🔥 Q2. 為什麼有 count 又有 count_if，不做成同一個名字的多載就好？
//     答：因為會產生無法解析的歧義。若容器的元素本身就是可呼叫物件
//         （例如 vector<function<bool(int)>>），「比對相等」與「套用謂詞」
//         在型別層次無法區分。STL 選擇用 _if 後綴以命名消歧義，
//         而不是讓多載解析去猜測使用者的意圖。
//     追問：還有哪些同樣的命名慣例？→ _if（改判斷方式）、_copy（不就地修改、
//         輸出到別處）、_n（用個數取代終點），例如 remove_copy_if、copy_n。
//
// ⚠️ 陷阱. std::copy(src.begin(), src.end(), dest.begin()) 在 dest 是空 vector
//        時會怎樣？
//     答：緩衝區溢位，是未定義行為。可能當場崩潰、可能默默寫壞相鄰記憶體、
//         也可能因為 capacity 剛好夠而「看起來正常」——正是最危險的情況。
//         copy 只拿到目的地的**起點**，沒有終點，無從得知還剩多少空間。
//     為什麼會錯：很多人以為 STL 演算法「會自己處理容器成長」。
//         但演算法只看得到 iterator，看不到容器本體，根本沒有能力呼叫
//         push_back。要自動成長必須明講：std::back_inserter(dest)。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>
#include <iterator>

// -----------------------------------------------------------------------------
// 【日常實務範例 1】API 回應時間監控：一次示範五種參數形式
//   情境：收集一批 API 回應時間（毫秒），要產出監控報表：
//         排序看分布(形式1)、統計逾時次數(形式2/3)、
//         匯出超標樣本(形式4)、與上一輪基準比對是否有變化(形式5)。
//   為什麼用到本主題：真實的資料處理流程幾乎一定會同時用到這五種形式，
//         認得形式就能猜出簽名，不必每次查文件。
// -----------------------------------------------------------------------------
void apiLatencyReport(std::vector<int> samples, const std::vector<int>& baseline) {
    // 形式一：只有範圍 —— 排序後才能看分位數
    std::sort(samples.begin(), samples.end());
    std::cout << "排序後樣本: ";
    for (int ms : samples) std::cout << ms << " ";
    std::cout << std::endl;

    // 形式二：範圍 + 值 —— 剛好等於 500ms（閘道預設逾時值）的次數
    auto exactly500 = std::count(samples.begin(), samples.end(), 500);
    std::cout << "剛好 500ms 的樣本數: " << exactly500 << std::endl;

    // 形式三：範圍 + 謂詞 —— SLA 門檻 300ms
    auto slowCount = std::count_if(samples.begin(), samples.end(),
                                   [](int ms) { return ms > 300; });
    std::cout << "超過 300ms (違反 SLA) 的樣本數: " << slowCount << std::endl;

    // 形式四：範圍 + 輸出迭代器 —— 匯出違規樣本（用 back_inserter 自動成長）
    std::vector<int> violations;
    std::copy_if(samples.begin(), samples.end(), std::back_inserter(violations),
                 [](int ms) { return ms > 300; });
    std::cout << "違規樣本明細: ";
    for (int ms : violations) std::cout << ms << " ";
    std::cout << std::endl;

    // 形式五：兩個範圍 —— 與上一輪基準比對（用 C++14 四參數安全版）
    bool same = std::equal(samples.begin(), samples.end(),
                           baseline.begin(), baseline.end());
    std::cout << "與上一輪基準相同: " << (same ? "是" : "否") << std::endl;

    // 順帶：mismatch 告訴你「第一個不同的位置」，比 equal 資訊更多
    auto diff = std::mismatch(samples.begin(), samples.end(),
                              baseline.begin(), baseline.end());
    if (diff.first != samples.end()) {
        std::cout << "第一個差異在索引 " << (diff.first - samples.begin())
                  << ": 本輪 " << *diff.first
                  << " vs 基準 " << *diff.second << std::endl;
    }
}

int main() {
    std::vector<int> vec = {5, 2, 8, 1, 9, 3, 7, 4, 6};

    // 形式一：只有範圍, 例如 sort 就是範圍版本的演算法
    std::cout << "=== 只有範圍 ===" << std::endl;
    std::sort(vec.begin(), vec.end());
    std::cout << "sort 後: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    // 形式二：範圍 + 值, 例如 count 就是範圍版本的演算法
    std::cout << "\n=== 範圍 + 值 ===" << std::endl;
    int count = std::count(vec.begin(), vec.end(), 5);
    std::cout << "5 出現次數: " << count << std::endl;

    // 形式三：範圍 + 謂詞, 例如 count_if 就是範圍版本的演算法
    std::cout << "\n=== 範圍 + 謂詞 ===" << std::endl;
    int even_count = std::count_if(vec.begin(), vec.end(),
        [](int n) { return n % 2 == 0; });
    std::cout << "偶數個數: " << even_count << std::endl;

    // 形式四：範圍 + 輸出迭代器, 例如 copy 就是範圍版本的演算法
    std::cout << "\n=== 範圍 + 輸出迭代器 ===" << std::endl;
    std::vector<int> dest(vec.size());   // ★ 必須先配置好空間，copy 不會幫你成長
    std::copy(vec.begin(), vec.end(), dest.begin());
    std::cout << "copy 結果: ";
    for (int n : dest) std::cout << n << " ";
    std::cout << std::endl;

    // 形式五：兩個範圍, 例如 equal 就是範圍版本的演算法
    std::cout << "\n=== 兩個範圍 ===" << std::endl;
    std::vector<int> v1 = {1, 2, 3};
    std::vector<int> v2 = {1, 2, 3};
    // C++14 起建議用四參數版：長度不同時直接回 false，不會讀過頭
    bool are_equal = std::equal(v1.begin(), v1.end(), v2.begin(), v2.end());
    std::cout << "v1 和 v2 相等: " << (are_equal ? "是" : "否") << std::endl;

    // 三參數版的危險示範（僅說明，不實際越界）：
    std::vector<int> shorter = {1, 2};
    bool safe = std::equal(v1.begin(), v1.end(), shorter.begin(), shorter.end());
    std::cout << "v1 和較短的 shorter 相等(四參數安全版): " << (safe ? "是" : "否") << std::endl;

    std::cout << "\n=== 日常實務：API 回應時間監控報表 ===" << std::endl;
    std::vector<int> samples  = {120, 450, 500, 88, 310, 500, 210};
    std::vector<int> baseline = {88, 120, 210, 310, 420, 500, 500};
    apiLatencyReport(samples, baseline);

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第七課：演算法（Algorithm）與容器的分離設計2.cpp -o demo2

// === 預期輸出 ===
// === 只有範圍 ===
// sort 後: 1 2 3 4 5 6 7 8 9 
//
// === 範圍 + 值 ===
// 5 出現次數: 1
//
// === 範圍 + 謂詞 ===
// 偶數個數: 4
//
// === 範圍 + 輸出迭代器 ===
// copy 結果: 1 2 3 4 5 6 7 8 9 
//
// === 兩個範圍 ===
// v1 和 v2 相等: 是
// v1 和較短的 shorter 相等(四參數安全版): 否
//
// === 日常實務：API 回應時間監控報表 ===
// 排序後樣本: 88 120 210 310 450 500 500 
// 剛好 500ms 的樣本數: 2
// 超過 300ms (違反 SLA) 的樣本數: 4
// 違規樣本明細: 310 450 500 500 
// 與上一輪基準相同: 否
// 第一個差異在索引 4: 本輪 450 vs 基準 420
