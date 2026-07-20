// =============================================================================
//  第八課：函數物件 6  —  <functional> 的算術類函數物件
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<functional>
//   六個算術函數物件（全部是 C++98）：
//       std::plus<T>       a + b        二元
//       std::minus<T>      a - b        二元
//       std::multiplies<T> a * b        二元
//       std::divides<T>    a / b        二元
//       std::modulus<T>    a % b        二元（只適用整數型別）
//       std::negate<T>     -a           一元
//   透明版本（transparent functor）：std::plus<> 等，**C++14**
//       （本機以 -std=c++11 -pedantic-errors 實測會被拒絕，c++14 通過）
//   C++14 起這些的 operator() 都是 constexpr。
//
// 【詳細解釋 Explanation】
//
// 【1. 這些東西存在的理由：運算子不能當參數傳】
//   C++ 的 + 是**運算子**，不是物件，你沒辦法寫
//       std::accumulate(v.begin(), v.end(), 1, *);     // 語法錯誤
//   演算法需要的是一個**可呼叫物件**。
//   std::multiplies<int>() 就是把「乘法」這個運算包裝成物件的產物：
//       std::accumulate(v.begin(), v.end(), 1, std::multiplies<int>());
//   所以這批 functor 的本質是「把運算子提升成一級公民（first-class）」。
//
// 【2. 為什麼 accumulate 預設是加法，要乘法就得明講】
//   std::accumulate(first, last, init) 的三參數版預設用 operator+。
//   要換運算就傳第四個參數。
//   ★ 注意 init 的型別決定整個累積的型別，這是最常見的坑：
//       std::accumulate(v.begin(), v.end(), 0)      // int 累積 → 可能溢位
//       std::accumulate(v.begin(), v.end(), 0LL)    // long long 累積
//       std::accumulate(doubles.begin(), doubles.end(), 0)   // ✗ 用 int 累積浮點！
//   最後一行會把每個中間結果截斷成整數，是很經典的錯誤。
//   算乘積時 init 更要注意：必須是 1 不是 0（0 乘什麼都是 0）。
//
// 【3. C++14 的透明版本 std::plus<>：省掉型別，也解決型別不符】
//   舊寫法 std::plus<int> 把型別寫死了，兩個問題：
//     * 囉唆：型別明明可以推導。
//     * **會做隱式轉換**：std::plus<int>{}(1.5, 2.5) 先把兩個 double
//       轉成 int 變成 1 和 2，結果是 3 而不是 4.0 —— 安靜的精度損失。
//   C++14 的 std::plus<>（等同 std::plus<void>）改用模板化的 operator()：
//       template<class T, class U> auto operator()(T&& t, U&& u) const
//           -> decltype(std::forward<T>(t) + std::forward<U>(u));
//   型別由引數推導，不做非預期轉換，還支援兩邊型別不同。
//   **新程式碼應該優先用 std::plus<>**。
//
// 【4. 透明比較器在關聯容器上的另一個用途：異質查找】
//   std::set<std::string, std::less<>> 可以直接用 const char* 或 string_view
//   來 find，不會為了查一次而建立臨時 std::string。
//   這是 C++14 引入透明 functor 最實際的效能理由——
//   對高頻查找的 map/set，省掉的是每次查找一次記憶體配置。
//   （unordered 容器的異質查找要到 C++20 才支援，且需要額外提供
//     is_transparent 的 hash 與 equal。）
//
// 【概念補充 Concept Deep Dive】
//
// (A) 「transparent」這個名字是怎麼來的
//     這些型別內部有一個 using is_transparent = ...; 標記。
//     標準函式庫看到這個標記，就知道「這個比較器願意接受異質型別」，
//     於是啟用 map/set 的模板化 find/lower_bound 重載。
//     名字的意思是「對型別透明」——它不強迫參數轉成某個固定型別。
//
// (B) std::accumulate 與 std::reduce 的差別（C++17）
//     accumulate 保證**由左至右依序**累積，所以可以用在
//     非結合律的運算（例如減法、浮點加法的精度考量）。
//     reduce 不保證順序也不保證分組，因此可以平行化，
//     但要求運算滿足**結合律與交換律**。
//     浮點加法嚴格說不滿足結合律，所以 reduce 的浮點結果
//     可能與 accumulate 有微小差異——這是規格允許的。
//
// (C) modulus 只能用於整數
//     std::modulus<double> 會編譯失敗，因為 double 沒有 operator%。
//     浮點取餘要用 <cmath> 的 std::fmod。
//
// (D) 這批 functor 在現代程式碼中的地位
//     多數場合 lambda 更直白：
//         std::accumulate(v.begin(), v.end(), 1, [](int a, int b){ return a*b; });
//     但 std::multiplies<>() 更短、意圖更清楚，而且
//     當作**型別參數**時（如 std::set 的比較器）仍是首選。
//     實務上算術類的 std::plus/minus 用得少，
//     比較類的 std::greater/less 用得非常多（第 7 檔）。
//
// 【注意事項 Pay Attention】
//   1. accumulate 的 init **型別決定累積型別**：
//      用 0 累積浮點會全程截斷成整數；用 int 累積大數會溢位。
//   2. 算乘積的 init 必須是 1，不能是 0。
//   3. std::plus<int> 會做隱式轉換（可能精度損失）；std::plus<> 不會。
//   4. std::modulus 只適用整數型別；浮點請用 std::fmod。
//   5. 整數除以 0、以及 INT_MIN / -1、INT_MIN % -1 都是未定義行為，
//      divides/modulus 不會幫你檢查。
//   6. 透明版本 std::plus<> 是 **C++14**，不是 C++11。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】算術函數物件與 accumulate
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::plus<int> 和 C++14 的 std::plus<> 差在哪裡？該用哪個？
//     答：std::plus<int> 的 operator() 參數型別寫死成 int，
//         傳 double 進去會先隱式轉成 int——
//         std::plus<int>{}(1.5, 2.5) 得到 3，不是 4.0，是安靜的精度損失。
//         std::plus<>（= std::plus<void>，C++14）的 operator() 是模板，
//         型別由引數推導、支援兩邊型別不同、也不做非預期轉換。
//         新程式碼應優先用 std::plus<>。
//     追問：透明版本還有什麼好處？
//         → std::set<std::string, std::less<>> 支援**異質查找**：
//           可以直接用 const char* / string_view 去 find，
//           不必為了查一次而建立臨時 std::string，省掉一次記憶體配置。
//           啟用的關鍵是型別內部的 is_transparent 標記。
//
// 🔥 Q2. std::accumulate(v.begin(), v.end(), 0) 對一個 vector<double>
//        會發生什麼事？
//     答：**全程用 int 累積**，每個中間結果都被截斷成整數，小數部分全丟。
//         因為 accumulate 的累積型別是由 init 的型別決定的，不是元素型別。
//         正確寫法是 std::accumulate(v.begin(), v.end(), 0.0)。
//     追問：那 vector<int> 求總和用 0 有問題嗎？
//         → 元素多或值大時會**整數溢位**（有號溢位是未定義行為）。
//           大量資料應該用 0LL 讓它以 long long 累積。
//
// ⚠️ 陷阱. 「我要算陣列乘積，寫成
//         std::accumulate(v.begin(), v.end(), 0, std::multiplies<int>());
//         為什麼結果永遠是 0？」
//     答：因為 init 是 0，而 0 乘以任何數都是 0。
//         accumulate 的運作是 result = init; 然後 result = op(result, *it) 逐一套用。
//         第一步就是 0 * v[0] = 0，之後永遠是 0。
//         乘法的單位元素是 1，所以 init 必須是 1。
//     為什麼會錯：把 init 當成「起始的空值」，
//         直覺套用了「總和從 0 開始」的經驗。
//         正確的理解是：init 是該運算的**單位元素（identity）**——
//         加法是 0、乘法是 1、取最大值是負無限大、字串串接是空字串。
//         這個坑之所以危險，是因為結果 0 看起來像個「合理的數字」，
//         不像崩潰那樣明顯。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#include <numeric>
#include <type_traits>
#include <limits>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1480. Running Sum of 1d Array
//   題目：給陣列 nums，回傳 runningSum，其中 runningSum[i] = nums[0]+...+nums[i]。
//   為什麼用到本主題：這題就是「前綴和」，而標準函式庫剛好有現成的
//     std::partial_sum，它的預設運算正是 std::plus。
//     這完美示範本檔的核心概念——**運算是可以當參數傳的**：
//     同一個 partial_sum，換一個函數物件就變成前綴積、前綴最大值。
//     一行解完，而且換運算只要換一個引數。
//   複雜度：O(n)。
// -----------------------------------------------------------------------------
std::vector<int> runningSum(const std::vector<int>& nums) {
    std::vector<int> out(nums.size());
    // partial_sum 的預設運算就是 std::plus<>
    std::partial_sum(nums.begin(), nums.end(), out.begin());
    return out;
}

// 同一個演算法，換個函數物件就換一種前綴運算
std::vector<int> runningProduct(const std::vector<int>& nums) {
    std::vector<int> out(nums.size());
    std::partial_sum(nums.begin(), nums.end(), out.begin(), std::multiplies<>());
    return out;
}

std::vector<int> runningMax(const std::vector<int>& nums) {
    std::vector<int> out(nums.size());
    std::partial_sum(nums.begin(), nums.end(), out.begin(),
                     [](int a, int b) { return std::max(a, b); });
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】購物車金額計算：小計、折扣、稅
//   情境：電商後端要算一張訂單的金額。這是最容易踩「accumulate 累積型別」
//         這個坑的地方——金額是浮點（或應該用整數分），
//         而 init 寫成 0 會讓整筆訂單的小數全部消失。
//   為什麼用到本主題：一次示範三件事——
//     (1) init 型別決定累積型別（0 vs 0.0 的天壤之別）
//     (2) 用 transform_reduce（C++17）一次做「單價 × 數量再加總」
//     (3) 金額為什麼實務上該用整數（分）而非 double
// -----------------------------------------------------------------------------
struct CartItem {
    std::string name;
    int         unitPriceCents;   // 用「分」存，避免浮點誤差
    int         qty;
};

// 小計（分）：單價 × 數量後加總 —— transform_reduce 一次搞定（C++17）
long long subtotalCents(const std::vector<CartItem>& cart) {
    return std::transform_reduce(
        cart.begin(), cart.end(),
        0LL,                                   // 用 long long 累積，避免溢位
        std::plus<>(),                         // 歸約運算
        [](const CartItem& i) {                // 轉換：單價 × 數量
            return static_cast<long long>(i.unitPriceCents) * i.qty;
        });
}

std::string formatCents(long long cents) {
    bool neg = cents < 0;
    if (neg) cents = -cents;
    std::string s = std::to_string(cents / 100) + "."
                  + (cents % 100 < 10 ? "0" : "") + std::to_string(cents % 100);
    return (neg ? "-$" : "$") + s;
}

int main() {
    std::cout << "=== 算術類函數物件 ===" << std::endl;

    // plus<T>：加法
    std::plus<int> add;
    std::cout << "plus: 3 + 5 = " << add(3, 5) << std::endl;

    // minus<T>：減法
    std::minus<int> subtract;
    std::cout << "minus: 10 - 3 = " << subtract(10, 3) << std::endl;

    // multiplies<T>：乘法
    std::multiplies<int> multiply;
    std::cout << "multiplies: 4 * 6 = " << multiply(4, 6) << std::endl;

    // divides<T>：除法
    std::divides<int> divide;
    std::cout << "divides: 20 / 4 = " << divide(20, 4) << std::endl;

    // modulus<T>：取餘數
    std::modulus<int> mod;
    std::cout << "modulus: 17 % 5 = " << mod(17, 5) << std::endl;

    // negate<T>：取負值
    std::negate<int> neg;
    std::cout << "negate: -7 = " << neg(7) << std::endl;

    // 實際應用：計算乘積
    std::cout << "\n=== 實際應用 ===" << std::endl;
    std::vector<int> vec = {1, 2, 3, 4, 5};

    int product = std::accumulate(vec.begin(), vec.end(), 1,
                                   std::multiplies<int>());
    std::cout << "1 * 2 * 3 * 4 * 5 = " << product << std::endl;

    // -------------------------------------------------------------------------
    std::cout << "\n=== 為什麼需要這些 functor：運算子不能當參數傳 ===" << std::endl;
    {
        std::cout << "std::accumulate(v.begin(), v.end(), 1, *);  // 語法錯誤"
                  << std::endl;
        std::cout << "  + - * / 是「運算子」不是物件，傳不進去。" << std::endl;
        std::cout << "  std::multiplies<int>() 就是把乘法包裝成物件的產物。"
                  << std::endl;
        std::cout << "→ 這批 functor 的本質是「把運算子提升成一級公民」。"
                  << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 陷阱一：init 是「單位元素」，不是「空值」 ===" << std::endl;
    {
        int wrong = std::accumulate(vec.begin(), vec.end(), 0, std::multiplies<int>());
        int right = std::accumulate(vec.begin(), vec.end(), 1, std::multiplies<int>());
        std::cout << "算乘積用 init=0 : " << wrong
                  << "  ← 0 乘任何數都是 0，永遠得 0" << std::endl;
        std::cout << "算乘積用 init=1 : " << right
                  << "  ← 1 是乘法的單位元素" << std::endl;
        std::cout << "→ 加法的單位元素是 0、乘法是 1、字串串接是空字串。"
                  << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 陷阱二：init 的型別決定累積型別 ===" << std::endl;
    {
        std::vector<double> prices = {1.5, 2.25, 3.75, 0.5};

        auto badSum  = std::accumulate(prices.begin(), prices.end(), 0);    // int！
        auto goodSum = std::accumulate(prices.begin(), prices.end(), 0.0);  // double

        std::cout << "資料: 1.5 + 2.25 + 3.75 + 0.5 = 8.0" << std::endl;
        std::cout << "init = 0（int）   : " << badSum
                  << "   ← 每個中間結果都被截斷成整數" << std::endl;
        std::cout << "init = 0.0（double）: " << goodSum
                  << "  ← 正確" << std::endl;
        std::cout << "badSum 的型別是 int 嗎? " << std::boolalpha
                  << std::is_same<decltype(badSum), int>::value << std::endl;

        // 整數溢位的版本 —— 這裡只算「安全」的那條，並說明另一條為何危險。
        // 刻意**不執行** int 版：有號整數溢位是未定義行為，
        // 它產生的值不可預測、也不該被當成固定結果印出來。
        std::vector<int> big(1000, 3000000);
        auto safe = std::accumulate(big.begin(), big.end(), 0LL);    // long long
        std::cout << "1000 個 3000000 相加（正確答案 3000000000）：" << std::endl;
        std::cout << "  init=0LL (long long): " << safe << "  ← 正確" << std::endl;
        std::cout << "  init=0   (int)      : 累積型別會是 int，而 int 上限是 "
                  << std::numeric_limits<int>::max() << "，" << std::endl;
        std::cout << "                        3000000000 已超出範圍 → **有號整數溢位**，"
                  << std::endl;
        std::cout << "                        那是未定義行為，不會有「固定的錯誤答案」，"
                  << std::endl;
        std::cout << "                        所以這裡刻意不執行它。" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== C++14 的透明版本 std::plus<> ===" << std::endl;
    {
        // 寫死型別：會做隱式轉換
        auto bad  = std::plus<int>{}(1.5, 2.5);   // 先轉成 int：1 + 2
        auto good = std::plus<>{}(1.5, 2.5);      // 型別推導：1.5 + 2.5

        std::cout << "std::plus<int>{}(1.5, 2.5) = " << bad
                  << "   ← 兩個 double 先被截斷成 int，安靜的精度損失" << std::endl;
        std::cout << "std::plus<>{}   (1.5, 2.5) = " << good
                  << " ← 型別由引數推導，正確" << std::endl;
        std::cout << "前者的結果型別是 int 嗎?    " << std::boolalpha
                  << std::is_same<decltype(bad), int>::value << std::endl;
        std::cout << "後者的結果型別是 double 嗎? "
                  << std::is_same<decltype(good), double>::value << std::endl;
        std::cout << "→ std::plus<> 是 **C++14**（以 -std=c++11 -pedantic-errors "
                  << "實測會被拒絕）。" << std::endl;
        std::cout << "  新程式碼應優先用透明版本。" << std::endl;

        // 透明版本還支援兩邊型別不同
        std::cout << "std::plus<>{}(1, 2.5) = " << std::plus<>{}(1, 2.5)
                  << "（兩邊型別不同也行）" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== LeetCode 1480. Running Sum of 1d Array ===" << std::endl;
    {
        auto show = [](const char* tag, const std::vector<int>& v) {
            std::cout << tag << "[";
            for (std::size_t i = 0; i < v.size(); ++i) {
                if (i) std::cout << ",";
                std::cout << v[i];
            }
            std::cout << "]" << std::endl;
        };

        std::vector<int> nums = {1, 2, 3, 4};
        show("輸入          : ", nums);
        show("前綴和 (plus) : ", runningSum(nums));
        show("前綴積 (mult) : ", runningProduct(nums));

        std::vector<int> nums2 = {3, 1, 4, 1, 5, 9, 2, 6};
        show("輸入          : ", nums2);
        show("前綴和        : ", runningSum(nums2));
        show("前綴最大值    : ", runningMax(nums2));
        std::cout << "→ 同一個 std::partial_sum，換一個函數物件就換一種運算。"
                  << std::endl;
        std::cout << "  這正是「把運算當參數傳」的價值。" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務：購物車金額計算 ===" << std::endl;
    {
        const std::vector<CartItem> cart = {
            {"USB-C 傳輸線",     29900, 2},
            {"機械鍵盤",        349000, 1},
            {"滑鼠墊",           15900, 3},
            {"筆電支架",         89900, 1},
        };

        std::cout << "購物車內容：" << std::endl;
        for (const auto& i : cart) {
            std::cout << "    " << i.name << "  " << formatCents(i.unitPriceCents)
                      << " x" << i.qty
                      << "  = " << formatCents(static_cast<long long>(i.unitPriceCents) * i.qty)
                      << std::endl;
        }

        long long sub = subtotalCents(cart);
        long long discount = sub >= 300000 ? sub / 10 : 0;      // 滿 $3000 打 9 折
        long long taxable  = sub - discount;
        long long tax      = (taxable * 5 + 50) / 100;          // 5% 稅，四捨五入
        long long total    = taxable + tax;

        std::cout << "  小計            : " << formatCents(sub) << std::endl;
        std::cout << "  折扣 (滿額 9 折): " << formatCents(-discount) << std::endl;
        std::cout << "  稅 (5%)         : " << formatCents(tax) << std::endl;
        std::cout << "  總計            : " << formatCents(total) << std::endl;

        // 對照：如果用 double 存金額會怎樣
        double d = 0.0;
        for (int i = 0; i < 10; ++i) d += 0.1;
        std::cout << "  （為什麼用「分」存金額：0.1 累加 10 次 == 1.0 嗎? "
                  << std::boolalpha << (d == 1.0) << "）" << std::endl;
        std::cout << "→ transform_reduce 用 0LL 當 init，一次完成"
                  << "「單價×數量再加總」且不溢位。" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第八課：函數物件（Function Object）初探6.cpp -o demo6
//
// ⚠️ 本檔用到 std::transform_reduce（C++17）與透明 functor std::plus<>（C++14），
//    所以最低需要 -std=c++17。

// === 預期輸出 ===
// === 算術類函數物件 ===
// plus: 3 + 5 = 8
// minus: 10 - 3 = 7
// multiplies: 4 * 6 = 24
// divides: 20 / 4 = 5
// modulus: 17 % 5 = 2
// negate: -7 = -7
//
// === 實際應用 ===
// 1 * 2 * 3 * 4 * 5 = 120
//
// === 為什麼需要這些 functor：運算子不能當參數傳 ===
// std::accumulate(v.begin(), v.end(), 1, *);  // 語法錯誤
//   + - * / 是「運算子」不是物件，傳不進去。
//   std::multiplies<int>() 就是把乘法包裝成物件的產物。
// → 這批 functor 的本質是「把運算子提升成一級公民」。
//
// === 陷阱一：init 是「單位元素」，不是「空值」 ===
// 算乘積用 init=0 : 0  ← 0 乘任何數都是 0，永遠得 0
// 算乘積用 init=1 : 120  ← 1 是乘法的單位元素
// → 加法的單位元素是 0、乘法是 1、字串串接是空字串。
//
// === 陷阱二：init 的型別決定累積型別 ===
// 資料: 1.5 + 2.25 + 3.75 + 0.5 = 8.0
// init = 0（int）   : 6   ← 每個中間結果都被截斷成整數
// init = 0.0（double）: 8  ← 正確
// badSum 的型別是 int 嗎? true
// 1000 個 3000000 相加（正確答案 3000000000）：
//   init=0LL (long long): 3000000000  ← 正確
//   init=0   (int)      : 累積型別會是 int，而 int 上限是 2147483647，
//                         3000000000 已超出範圍 → **有號整數溢位**，
//                         那是未定義行為，不會有「固定的錯誤答案」，
//                         所以這裡刻意不執行它。
//
// === C++14 的透明版本 std::plus<> ===
// std::plus<int>{}(1.5, 2.5) = 3   ← 兩個 double 先被截斷成 int，安靜的精度損失
// std::plus<>{}   (1.5, 2.5) = 4 ← 型別由引數推導，正確
// 前者的結果型別是 int 嗎?    true
// 後者的結果型別是 double 嗎? true
// → std::plus<> 是 **C++14**（以 -std=c++11 -pedantic-errors 實測會被拒絕）。
//   新程式碼應優先用透明版本。
// std::plus<>{}(1, 2.5) = 3.5（兩邊型別不同也行）
//
// === LeetCode 1480. Running Sum of 1d Array ===
// 輸入          : [1,2,3,4]
// 前綴和 (plus) : [1,3,6,10]
// 前綴積 (mult) : [1,2,6,24]
// 輸入          : [3,1,4,1,5,9,2,6]
// 前綴和        : [3,4,8,9,14,23,25,31]
// 前綴最大值    : [3,3,4,4,5,9,9,9]
// → 同一個 std::partial_sum，換一個函數物件就換一種運算。
//   這正是「把運算當參數傳」的價值。
//
// === 日常實務：購物車金額計算 ===
// 購物車內容：
//     USB-C 傳輸線  $299.00 x2  = $598.00
//     機械鍵盤  $3490.00 x1  = $3490.00
//     滑鼠墊  $159.00 x3  = $477.00
//     筆電支架  $899.00 x1  = $899.00
//   小計            : $5464.00
//   折扣 (滿額 9 折): -$546.40
//   稅 (5%)         : $245.88
//   總計            : $5163.48
//   （為什麼用「分」存金額：0.1 累加 10 次 == 1.0 嗎? false）
// → transform_reduce 用 0LL 當 init，一次完成「單價×數量再加總」且不溢位。
