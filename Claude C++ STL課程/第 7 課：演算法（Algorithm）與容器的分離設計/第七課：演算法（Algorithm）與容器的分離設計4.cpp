// =============================================================================
//  第七課：演算法（Algorithm）與容器的分離設計4.cpp
//    —  計數與條件判斷：count / count_if / all_of / any_of / none_of
// =============================================================================
//
// 【主題資訊 Information】
//   difference_type count   (InputIt f, InputIt l, const T& value);   // C++98
//   difference_type count_if(InputIt f, InputIt l, UnaryPred p);      // C++98
//   bool all_of (InputIt f, InputIt l, UnaryPred p);                  // C++11 ★
//   bool any_of (InputIt f, InputIt l, UnaryPred p);                  // C++11 ★
//   bool none_of(InputIt f, InputIt l, UnaryPred p);                  // C++11 ★
//
//   標準版本：count / count_if 為 C++98；**all_of / any_of / none_of 是 C++11 新增**
//   迭代器需求：Input Iterator（全部）
//   複雜度：count / count_if 恆為 O(N)（一定掃完）；
//           all_of / any_of / none_of 最壞 O(N)，但**找到反例就提早停**
//   回傳：count 系列回傳 difference_type（有號，通常是 ptrdiff_t）；
//         三個 _of 回傳 bool
//   標頭檔：<algorithm>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 count_if(...) > 0 是效能地雷】
// 這是本檔最重要的一課。以下兩行語意相同，效能天差地遠：
//     if (std::count_if(v.begin(), v.end(), pred) > 0)   // ✗ 一定掃完 N 個
//     if (std::any_of  (v.begin(), v.end(), pred))       // ✓ 第一個命中就停
// count_if 的合約是「回傳滿足條件的**數量**」，要算出數量就非掃完不可，
// 即使第一個元素就命中也一樣。any_of 的合約只是「有沒有」，
// 因此實作可以在第一個命中處立刻 return true。
// 對百萬筆資料而言，這是「掃 1 筆」與「掃 1,000,000 筆」的差別。
// **口訣：要數量用 count_if，要是非題用 any_of / all_of / none_of。**
//
// 【2. 三個 _of 的空範圍語意（很容易答錯）】
// 對空範圍（first == last）：
//     all_of  → true    （vacuous truth，空集合上任何全稱命題都為真）
//     any_of  → false   （沒有元素，就沒有元素滿足條件）
//     none_of → true    （沒有元素滿足條件，成立）
// all_of 回傳 true 常讓人意外，但這是數學上的正確定義：
// 「所有元素都滿足 P」在沒有元素時不存在反例，因此為真。
// 實務含意：驗證邏輯若寫成 all_of(items, isValid)，**空清單會通過驗證**。
// 若空清單應該視為錯誤，必須另外檢查 !items.empty()。
//
// 【3. 三者的邏輯關係】
//     none_of(f, l, p)  ≡  !any_of(f, l, p)
//     all_of (f, l, p)  ≡  none_of(f, l, [&](auto x){ return !p(x); })
// 既然可以互相推導，為什麼標準提供三個？因為**可讀性即正確性**：
// 「所有交易金額都必須為正」寫成 all_of(..., isPositive) 一眼就懂；
// 寫成 !any_of(..., [](auto& t){ return !isPositive(t); }) 則需要解雙重否定，
// 而雙重否定正是 code review 最容易漏看 bug 的地方。
//
// 【4. count 回傳型別的細節】
// count 回傳 iterator_traits<It>::difference_type，通常是 ptrdiff_t（有號 64 位元）。
// **不是** size_t。這造成一個常見的警告來源：
//     if (std::count(...) == v.size())   // -Wsign-compare：有號 vs 無號
// 正確做法是把 size() 轉型，或用 auto 接 count 的結果後與
// static_cast<std::ptrdiff_t>(v.size()) 比較。
// 本檔用 int 接是因為數值很小且教學上直觀；大範圍請用 auto。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 count 一定得掃完，編譯器不能幫忙最佳化掉
//   即使你寫 count_if(...) > 0，編譯器**通常無法**把它降級成 any_of：
//   要證明「提早結束不改變程式行為」，編譯器得證明謂詞沒有副作用、
//   沒有拋出例外、且結果只用於這個比較。跨 translation unit 或謂詞稍複雜時
//   這個推論就失敗了。所以這件事得由程式員負責，不能指望 -O2。
//
// (B) short-circuit 的實作方式
//   any_of 的標準實作就是 return find_if(f, l, p) != l;
//   all_of  是 return find_if_not(f, l, p) == l;
//   none_of 是 return find_if(f, l, p) == l;
//   ——三者都是 find 系列的薄包裝，這也解釋了為什麼它們會提早結束：
//   find 系列本來就是命中即停。
//
// (C) 為什麼這三個到 C++11 才進標準
//   C++98 就能用 find_if 手寫出來，但那樣的程式碼把「意圖」藏在
//   「!= end()」的比較裡。C++11 引入 lambda 讓謂詞變得好寫之後，
//   標準委員會補上這三個名字，讓意圖直接寫在函式名上。
//   這是 STL 一貫的哲學：**演算法的名字就是文件**。
//
// 【注意事項 Pay Attention】
// 1. **只想知道「有沒有」時，絕不要用 count_if(...) > 0**——用 any_of。
// 2. 空範圍時 all_of 與 none_of 都回傳 true，any_of 回傳 false。
//    驗證邏輯要考慮「空輸入是否應該通過」。
// 3. count 系列回傳有號的 difference_type，與 size() 的無號型別比較
//    會觸發 -Wsign-compare。
// 4. all_of / any_of / none_of 是 C++11；-std=c++98 編不過。
// 5. 謂詞必須無副作用：標準不保證呼叫次數（提早結束時就不會全部呼叫）。
// 6. 對 map / set 要「數某個 key 有幾個」，用成員函式 m.count(key)（O(log N)），
//    不要用 std::count（O(N)）。名字一樣，複雜度差很多。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】計數與條件判斷演算法
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. if (count_if(v.begin(), v.end(), pred) > 0) 和 if (any_of(...)) 有什麼差別？
//     答：語意相同，效能可能差 N 倍。count_if 的合約是回傳「數量」，
//         就算第一個元素就命中也必須掃完整個範圍才能得出總數；
//         any_of 只需回答是非題，第一個命中就 return true。
//         百萬筆資料時是掃 1 筆與掃 100 萬筆的差別。
//     追問：編譯器 -O2 不能自動最佳化嗎？→ 通常不行。要降級成 any_of，
//         編譯器得先證明謂詞無副作用、不拋例外、結果只用於該比較，
//         跨 TU 或謂詞稍複雜就推論不出來。這責任在程式員身上。
//
// 🔥 Q2. 對空的 vector 呼叫 all_of、any_of、none_of 各回傳什麼？
//     答：all_of = true、any_of = false、none_of = true。
//         all_of 回傳 true 是數學上的 vacuous truth：空集合上找不到反例，
//         全稱命題即為真。
//     追問：這在實務上會造成什麼 bug？→ 若把 all_of(items, isValid) 當成
//         驗證關卡，**空清單會直接通過驗證**。若空輸入該視為錯誤，
//         必須另外加 !items.empty() 檢查。
//
// ⚠️ 陷阱. 對 std::map<string,int> 統計某個 key 的數量，寫
//        std::count(m.begin(), m.end(), key) 為什麼不對？
//     答：兩個問題。第一，型別就不對——map 的元素是 pair<const K, V>，
//         不是 key，這行根本編譯不過。第二，就算改成 count_if 比對 first，
//         那也是 O(N) 線性掃描；map 自己的 m.count(key) 是 O(log N)。
//     為什麼會錯：std::count 和 map::count 同名，很容易以為是同一個東西。
//         它們只是名字撞了：通用演算法看不到紅黑樹結構，只能一個一個比；
//         成員函式知道自己是排序樹，能直接走搜尋路徑。
//         **同名時一律優先用成員函式**（見本課第 16 個檔案）。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 169. Majority Element
//   題目：給定大小為 n 的陣列，找出出現次數超過 ⌊n/2⌋ 的多數元素（保證存在）。
//   為什麼用到本主題：排序後多數元素必定落在正中間（因為它佔超過一半），
//         再用 std::count 驗證這個候選確實超過半數——正是 count 的典型用途。
//   複雜度：O(N log N)（排序主導）。
//   註：Boyer-Moore 投票法可做到 O(N) 時間 O(1) 空間；這裡示範的是本主題解法。
// -----------------------------------------------------------------------------
int majorityElement(std::vector<int> nums) {
    std::sort(nums.begin(), nums.end());
    int candidate = nums[nums.size() / 2];   // 超過半數 → 必定佔據中位數位置
    // 用 count 驗證（回傳 difference_type，用 auto 接避免型別假設）
    auto occurrences = std::count(nums.begin(), nums.end(), candidate);
    return occurrences > static_cast<std::ptrdiff_t>(nums.size() / 2) ? candidate : -1;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】批次上傳前的訂單資料驗證
//   情境：電商後台一次匯入一批訂單，送進金流前必須全部通過檢查：
//         金額為正、都有訂單編號、沒有測試訂單混入。
//   為什麼用到本主題：三個檢查剛好對應 all_of（全部合格）、
//         none_of（沒有任何一筆是測試單）、any_of（有沒有需要人工審核的大額單）。
//   重點：這裡刻意用 any_of 而非 count_if(...) > 0，
//         因為只要找到一筆就能決定，不必掃完整批。
// -----------------------------------------------------------------------------
struct Order {
    std::string id;
    int amountTwd;
    bool isTest;
};

void validateBatch(const std::vector<Order>& batch) {
    // 空批次防呆：all_of 對空範圍回傳 true，會讓空批次「通過驗證」
    if (batch.empty()) {
        std::cout << "  [REJECT] 批次為空（注意：all_of 對空範圍回傳 true，"
                     "必須另外擋掉）" << std::endl;
        return;
    }

    bool allAmountsValid = std::all_of(batch.begin(), batch.end(),
                                       [](const Order& o) { return o.amountTwd > 0; });
    bool allHaveId = std::all_of(batch.begin(), batch.end(),
                                 [](const Order& o) { return !o.id.empty(); });
    bool noTestOrders = std::none_of(batch.begin(), batch.end(),
                                     [](const Order& o) { return o.isTest; });
    // 只要有一筆大額就要人工審核 → 用 any_of，找到即停
    bool needsReview = std::any_of(batch.begin(), batch.end(),
                                   [](const Order& o) { return o.amountTwd >= 100000; });
    // 這裡要的是「數量」而不是是非題，才用 count_if
    auto largeCount = std::count_if(batch.begin(), batch.end(),
                                    [](const Order& o) { return o.amountTwd >= 100000; });

    std::cout << "  金額全部為正: " << (allAmountsValid ? "是" : "否") << std::endl;
    std::cout << "  訂單編號齊全: " << (allHaveId ? "是" : "否") << std::endl;
    std::cout << "  無測試訂單:   " << (noTestOrders ? "是" : "否") << std::endl;
    std::cout << "  需人工審核:   " << (needsReview ? "是" : "否")
              << " (大額訂單 " << largeCount << " 筆)" << std::endl;
    std::cout << "  結論: "
              << ((allAmountsValid && allHaveId && noTestOrders) ? "可送金流" : "退回修正")
              << std::endl;
}

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // count：計算某值出現次數, count_if：計算滿足條件的元素數
    std::cout << "=== count ===" << std::endl;
    std::vector<int> v2 = {1, 2, 2, 3, 2, 4, 2, 5};
    std::cout << "2 出現次數: " << std::count(v2.begin(), v2.end(), 2) << std::endl;

    // count_if：計算滿足條件的元素數, 使用 lambda 表達式作為條件
    std::cout << "\n=== count_if ===" << std::endl;
    int even_count = std::count_if(vec.begin(), vec.end(),
        [](int n) { return n % 2 == 0; });
    std::cout << "偶數個數: " << even_count << std::endl;

    // all_of：是否全部滿足條件, any_of：是否有任一滿足條件, none_of：是否全部不滿足條件
    // ★ 三者都是 C++11 新增，且都會 short-circuit（找到反例就停）
    std::cout << "\n=== all_of ===" << std::endl;
    bool all_positive = std::all_of(vec.begin(), vec.end(),
        [](int n) { return n > 0; });
    std::cout << "全部是正數: " << (all_positive ? "是" : "否") << std::endl;

    // any_of：是否有任一滿足條件, 使用 lambda 表達式檢查是否有偶數
    std::cout << "\n=== any_of ===" << std::endl;
    bool has_even = std::any_of(vec.begin(), vec.end(),
        [](int n) { return n % 2 == 0; });
    std::cout << "有偶數: " << (has_even ? "是" : "否") << std::endl;

    // none_of：是否全部不滿足條件, 使用 lambda 表達式檢查是否有負數
    std::cout << "\n=== none_of ===" << std::endl;
    bool no_negative = std::none_of(vec.begin(), vec.end(),
        [](int n) { return n < 0; });
    std::cout << "沒有負數: " << (no_negative ? "是" : "否") << std::endl;

    // ★ 空範圍語意：面試最愛問
    std::cout << "\n=== 空範圍的三個 _of（面試常考）===" << std::endl;
    std::vector<int> empty;
    std::cout << "空範圍 all_of  (n > 0): "
              << (std::all_of(empty.begin(), empty.end(), [](int n) { return n > 0; }) ? "true" : "false")
              << "   ← vacuous truth，沒有反例即為真" << std::endl;
    std::cout << "空範圍 any_of  (n > 0): "
              << (std::any_of(empty.begin(), empty.end(), [](int n) { return n > 0; }) ? "true" : "false")
              << std::endl;
    std::cout << "空範圍 none_of (n > 0): "
              << (std::none_of(empty.begin(), empty.end(), [](int n) { return n > 0; }) ? "true" : "false")
              << std::endl;

    std::cout << "\n=== LeetCode 169. Majority Element ===" << std::endl;
    std::cout << "[3,2,3] -> " << majorityElement({3, 2, 3}) << std::endl;
    std::cout << "[2,2,1,1,1,2,2] -> " << majorityElement({2, 2, 1, 1, 1, 2, 2}) << std::endl;

    std::cout << "\n=== 日常實務：訂單批次驗證 ===" << std::endl;
    std::cout << "批次 A (正常):" << std::endl;
    validateBatch({{"ORD-1001", 2500, false}, {"ORD-1002", 380000, false}});
    std::cout << "批次 B (有問題):" << std::endl;
    validateBatch({{"ORD-2001", -50, false}, {"", 1200, false}, {"TEST-01", 999, true}});
    std::cout << "批次 C (空批次):" << std::endl;
    validateBatch({});

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第七課：演算法（Algorithm）與容器的分離設計4.cpp -o demo4

// === 預期輸出 ===
// === count ===
// 2 出現次數: 4
//
// === count_if ===
// 偶數個數: 5
//
// === all_of ===
// 全部是正數: 是
//
// === any_of ===
// 有偶數: 是
//
// === none_of ===
// 沒有負數: 是
//
// === 空範圍的三個 _of（面試常考）===
// 空範圍 all_of  (n > 0): true   ← vacuous truth，沒有反例即為真
// 空範圍 any_of  (n > 0): false
// 空範圍 none_of (n > 0): true
//
// === LeetCode 169. Majority Element ===
// [3,2,3] -> 3
// [2,2,1,1,1,2,2] -> 2
//
// === 日常實務：訂單批次驗證 ===
// 批次 A (正常):
//   金額全部為正: 是
//   訂單編號齊全: 是
//   無測試訂單:   是
//   需人工審核:   是 (大額訂單 1 筆)
//   結論: 可送金流
// 批次 B (有問題):
//   金額全部為正: 否
//   訂單編號齊全: 否
//   無測試訂單:   否
//   需人工審核:   否 (大額訂單 0 筆)
//   結論: 退回修正
// 批次 C (空批次):
//   [REJECT] 批次為空（注意：all_of 對空範圍回傳 true，必須另外擋掉）
