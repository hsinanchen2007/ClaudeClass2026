// =============================================================================
//  summary.cpp  —  函數物件（Functor）／Lambda／std::function：可呼叫物件的全景
// =============================================================================
//
// 【主題資訊 Information】
//   函數物件（Functor）  : 任何重載了 operator() 的類別實例        [C++98,  <無需標頭>]
//   STL 內建函數物件      : std::plus / less / greater / ...        [C++98,  <functional>]
//   透明比較器            : std::less<>（void 特化）               [C++14,  <functional>]
//   Lambda 表達式         : [capture](params) -> ret { body }       [C++11]
//     └ 泛型 Lambda（auto 參數）                                    [C++14]
//     └ constexpr Lambda、capture 內可用 *this                      [C++17]
//     └ 模板參數 Lambda []<typename T>(T x){}                       [C++20]
//   型別抹除包裝器        : std::function<R(Args...)>               [C++11,  <functional>]
//
//   複雜度：functor／lambda 的 operator() 呼叫本身是 O(1) 且可被 inline；
//           std::function 的 operator() 是 O(1) 但含一次「間接呼叫」，通常無法 inline。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼需要函數物件？——「函數不能記住東西」】
//   考慮「找出所有大於 threshold 的元素」。用普通函數當謂詞會卡住：
//       bool greater_than(int x) { return x > ???; }   // threshold 從哪來？
//   C 的解法是全域變數（不可重入、不可並行、醜）；或多傳一個 void* user_data
//   （qsort_r／pthread 都這樣做，型別不安全）。
//   C++ 的解法是把「狀態」放進物件、把「行為」放進 operator()：
//       class GreaterThan { int t_; public: bool operator()(int x) const { return x > t_; } };
//   於是 GreaterThan(5) 和 GreaterThan(7) 是兩個不同的「函數」——這就是 closure
//   （閉包）的本質：程式碼 + 被捕獲的環境。
//
// 【2. 為什麼 functor 比 function pointer 快？——這是關鍵的效能論點】
//   std::sort(v.begin(), v.end(), std::greater<int>()) 比
//   qsort(..., cmp_function_pointer) 快，原因不在演算法，在「型別」：
//     * 函數指標：型別是 bool(*)(int,int)，「值」在執行期才知道 → 編譯器必須
//       產生一次 indirect call，無法 inline。
//     * 函數物件：型別是 std::greater<int>，「呼叫哪段程式碼」在編譯期就固定了
//       （由型別決定，不是由值決定）→ sort 的模板實例化後可完全 inline，
//       比較動作直接變成一條 cmp 指令。
//   結論：functor 的呼叫資訊藏在「型別」裡，函數指標的呼叫資訊藏在「值」裡。
//   這也是為什麼空的 functor（sizeof == 1，本機實測）能有零執行期成本。
//
// 【3. Lambda 是什麼？——編譯器幫你寫的匿名 class】
//   Lambda 不是新的執行期機制，純粹是語法糖。編譯器看到
//       int t = 5;
//       auto f = [t](int n) { return n > t; };
//   會 desugar 成大致等價的：
//       class __lambda_7_14 {
//           int t;                                    // 捕獲 → 成員變數
//       public:
//           __lambda_7_14(int t_) : t(t_) {}
//           bool operator()(int n) const { return n > t; }   // 注意：const！
//       };
//       auto f = __lambda_7_14(t);
//   這個編譯器生成的型別叫 closure type，它是 unique、unnamed 的。
//   由此可推出所有 lambda 的行為：
//     * 為什麼要 auto 接？→ 型別無名，你根本寫不出來。
//     * 為什麼 sizeof 等於捕獲總和？→ 捕獲就是成員變數（本機實測：無捕獲 1、
//       [int] 4、[&int] 8、[int,double] 16、[int,double,long long] 24 bytes）。
//     * 為什麼值捕獲不能改？→ operator() 預設是 const 成員函式。
//
// 【4. mutable 的真正意義——不是「讓變數可變」】
//   多數人以為 mutable 是「解除 const」，其實它做的是：
//       把 operator() const 變成 operator()（拿掉 const）。
//   所以 mutable lambda 改的是「自己的成員變數」，外部變數完全不受影響。
//   副作用是：mutable lambda 不能用 const 的 closure 物件呼叫。
//
// 【5. 捕獲方式的取捨】
//   [x]   值捕獲：拷貝一份進 closure。安全，但有拷貝成本。
//   [&x]  參考捕獲：closure 內存的是參考（實作上是指標，本機 sizeof 為 8）。
//         零拷貝，但 closure 活得比 x 久就是懸空參考（UB）。
//   [=]   隱式值捕獲所有用到的變數。注意：不會捕獲 static 與全域（它們本來就可見）。
//   [&]   隱式參考捕獲。寫起來最短，也最容易寫出懸空。
//   [=, &b] / [&, b]  混合：預設 + 例外。規則是「預設與例外必須不同」。
//   一句話原則：closure 不逃出當前 scope → [&] 沒問題；要存起來／丟給 thread
//   ／放進 std::function → 一律值捕獲。
//
// 【6. std::function：型別抹除（type erasure）】
//   問題：每個 lambda 都是不同型別，那 std::vector<???> 要怎麼裝一堆 lambda？
//   std::function 用型別抹除解決：它把「任何符合 R(Args...) 簽名的可呼叫物件」
//   統一包成同一個型別，內部靠虛擬呼叫（或等價的函數指標表）分派。
//   代價有三：
//     (a) 一次間接呼叫，通常無法 inline；
//     (b) 若 closure 太大會 heap 配置（libstdc++ 有 small buffer optimization，
//         sizeof(std::function<int(int,int)>) 本機實測為 32 bytes）；
//     (c) 空的 std::function 被呼叫會丟 std::bad_function_call。
//   選用準則：模板參數（template<class F>）能吃下 callable 時就別用 std::function；
//   只有「需要在執行期切換行為」或「要存進容器／成員變數」才用它。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 空基底最佳化（EBO）與零成本抽象
//     std::greater<int> 沒有任何成員，sizeof 是 1（本機實測；C++ 規定完整物件
//     大小不得為 0）。但當它被當成 std::sort 的模板參數時，實作會用 EBO 或直接
//     以「型別」呼叫，完全不佔空間、不產生額外指令。這就是所謂 zero-overhead
//     abstraction：抽象只存在於編譯期。
//
// (B) 為什麼 [=] 捕獲成員變數其實是捕獲 this？
//     在成員函式裡寫 [=]{ return member_; }，捕獲的不是 member_ 的副本，而是
//     this 指標（member_ 實際是 this->member_）。物件先死、closure 後跑 → 懸空。
//     這個陷阱太常見，C++20 已把「以 [=] 隱式捕獲 this」標為 deprecated，
//     要求明確寫 [this]（捕獲指標）或 [*this]（C++17，複製整個物件）。
//
// (C) STL 演算法傳的是「副本」
//     std::for_each(first, last, f) 收的是 f 的副本，且回傳那個副本。所以帶狀態
//     的 functor 一定要用回傳值取狀態：Counter r = std::for_each(...); r.get_count();
//     直接讀原本的 counter 會拿到 0——這是本課示範三特意演示的重點。
//
// (D) 透明比較器 std::less<>（C++14）
//     std::less<int> 只能比 int；std::less<>（即 std::less<void>）的 operator()
//     是模板，能比任何有 < 的型別，還能做異質查找（map<string,X> 用 string_view
//     查詢而不建臨時 string）。新程式碼建議一律寫 std::greater<>() 而非
//     std::greater<int>()。
//
// 【注意事項 Pay Attention】
// 1. 參考捕獲 + 延後執行 = 懸空參考（UB）。UB 不保證崩潰，也可能「看起來正常」，
//    這比崩潰更危險。要存起來的 closure 一律值捕獲。
// 2. mutable 改的是 closure 自己的副本，外部變數永遠不變——本檔示範七會實際印出來。
// 3. std::function 若為空（未賦值）就呼叫，標準保證丟 std::bad_function_call，
//    這是明確定義的例外，不是 UB。
// 4. 泛型 lambda 的 auto 參數是 C++14；C++11 只能寫死型別。用 -std=c++11
//    -pedantic-errors 編本檔會直接報錯。
// 5. std::for_each 回傳 functor 副本；忘記接回傳值是初學者最常見的「狀態不見了」。
// 6. 除法 lambda [](int a,int b){ return a/b; } 若 b 為 0 是 UB，本檔僅以 20/4
//    示範，實務上務必先檢查除數。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】函數物件 / Lambda / std::function
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. Lambda 到底是什麼？編譯器把它變成了什麼？
//     答：Lambda 是語法糖，編譯器會生成一個 unnamed、unique 的 class
//         （closure type）：捕獲變成成員變數，函數主體變成 operator()，
//         預設帶 const。所以 lambda 就是「編譯器代寫的函數物件」，
//         沒有任何執行期魔法。
//     追問：那 sizeof(lambda) 是多少？→ 等於所有捕獲成員的大小（含對齊）；
//         無捕獲的 lambda 是空類別，sizeof 為 1（本機實測）。
//
// 🔥 Q2. 為什麼 std::sort 傳 functor 比 qsort 傳函數指標快？
//     答：functor 的「呼叫目標」由型別決定，編譯期就固定，模板實例化後比較
//         動作可以完全 inline；函數指標的目標是執行期的值，只能做 indirect
//         call，無法 inline，還會擋住向量化。差距常見在 2~3 倍。
//     追問：那 std::function 呢？→ std::function 是型別抹除，等於又把資訊
//         從型別搬回值，效能特性接近函數指標，別在熱路徑用。
//
// 🔥 Q3. mutable 這個關鍵字，在 lambda 上到底做了什麼？
//     答：把 operator() 的 const 拿掉。它讓你能修改「closure 內部那份副本」，
//         而不是外部變數。外部變數自始至終不受影響。
//     追問：那 mutable lambda 每次呼叫會重置嗎？→ 不會。副本是 closure 的
//         成員，狀態會跨呼叫累積（本檔 increment() 印出 1、2、3 即為證）。
//
// ⚠️ 陷阱 1. 「用 [&] 最方便，反正效能最好」——為什麼這句話會害你？
//     答：[&] 讓 closure 內存的是參考。只要 closure 的生命週期超過被捕獲的
//         變數（存進成員、丟進 std::thread、放入 callback 註冊表），就是
//         懸空參考，屬於 UB：可能崩潰、可能讀到垃圾、也可能「測試時剛好正常」。
//     為什麼會錯：多數人把 lambda 想成「馬上就執行完的一小段程式」，
//         只看到 std::sort、std::count_if 這種立即呼叫的場景。一旦 lambda
//         被儲存起來延後執行，那個心智模型就整個失效了。
//
// ⚠️ 陷阱 2. Counter counter(2); std::for_each(v.begin(), v.end(), counter);
//            之後 counter.get_count() 為什麼是 0？
//     答：for_each 收的是 counter 的「副本」，累加全發生在副本上。正確寫法是
//         接住回傳值：Counter r = std::for_each(...); r.get_count()。
//     為什麼會錯：直覺上以為傳進去的是「那個物件」，但 STL 演算法的謂詞參數
//         是 by value 的（標準要求可複製），這是 STL 一貫的值語意設計。
// ═══════════════════════════════════════════════════════════════════════════

/*
 * ============================================================
 * 【第 8 課：函數物件（Function Object）初探】總複習 summary.cpp
 * ============================================================
 * 本課程重點：
 * 1. 函數物件（Functor）的定義與原理：重載 operator() 的類別
 * 2. 函數物件 vs 普通函數：狀態攜帶能力的差異
 * 3. STL 內建函數物件：<functional> 中的算術、比較、邏輯類
 * 4. Lambda 表達式：現代 C++ 的就地函數物件
 * 5. Lambda 的捕獲機制：值捕獲、參考捕獲、混合捕獲
 * 6. mutable Lambda：允許修改值捕獲的副本
 * 7. 泛型 Lambda（C++14）：使用 auto 參數
 * 8. std::function：通用可呼叫物件包裝器
 * ============================================================
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>
#include <numeric>
#include <string>
#include <optional>
#include <utility>
#include <cctype>

// ===== 重點一：函數物件（Functor）的基本定義 =====
// 函數物件是「重載了 operator() 的類別」的實例
// 它可以像函數一樣被呼叫，但本質上是一個物件
// 優勢：可以攜帶狀態（透過成員變數）

// 最基本的函數物件示範
class Adder {
public:
    // 關鍵：重載 operator()，讓物件可以像函數一樣被呼叫
    int operator()(int a, int b) const {
        return a + b;
    }
};

// 帶有狀態的函數物件：閾值過濾器
// 普通函數做不到「記住」一個可配置的閾值
class GreaterThan {
private:
    int threshold_;  // 狀態：被攜帶的閾值
public:
    GreaterThan(int t) : threshold_(t) {}

    bool operator()(int x) const {
        return x > threshold_;  // 使用存儲的狀態進行判斷
    }
};

// 帶有狀態的函數物件：整除判斷器
class IsDivisibleBy {
private:
    int divisor_;
public:
    IsDivisibleBy(int d) : divisor_(d) {}

    bool operator()(int x) const {
        return x % divisor_ == 0;
    }
};

// 完整結構的函數物件：計數器（示範狀態累積）
class Counter {
private:
    int count_;   // 狀態：計數器（隨呼叫累積）
    int target_;  // 狀態：目標值
public:
    Counter(int target) : count_(0), target_(target) {}

    bool operator()(int x) {
        if (x == target_) {
            ++count_;
            return true;
        }
        return false;
    }

    // 存取累積的狀態
    int get_count() const { return count_; }
};


// ===== 重點二：STL 內建函數物件（#include <functional>）=====
/*
 * 算術類：
 *   std::plus<T>        → x + y
 *   std::minus<T>       → x - y
 *   std::multiplies<T>  → x * y
 *   std::divides<T>     → x / y
 *   std::modulus<T>     → x % y
 *   std::negate<T>      → -x
 *
 * 比較類：
 *   std::equal_to<T>      → x == y
 *   std::not_equal_to<T>  → x != y
 *   std::greater<T>       → x > y
 *   std::less<T>          → x < y
 *   std::greater_equal<T> → x >= y
 *   std::less_equal<T>    → x <= y
 *
 * 邏輯類：
 *   std::logical_and<T>   → x && y
 *   std::logical_or<T>    → x || y
 *   std::logical_not<T>   → !x
 *
 * 最常用的場景：
 *   std::sort(v.begin(), v.end(), std::greater<int>());  // 降序排序
 *   std::accumulate(v.begin(), v.end(), 1, std::multiplies<int>());  // 連乘
 */


// ===== 重點三：Lambda 表達式 =====
// Lambda 是 C++11 引入的「就地」定義函數物件的語法糖
// 編譯器會自動將 Lambda 轉換成匿名類別

/*
 * Lambda 基本語法：
 * [捕獲列表](參數列表) -> 回傳型別 { 函數主體 }
 *
 * 捕獲列表說明：
 *   []        → 不捕獲任何外部變數
 *   [x]       → 值捕獲 x（複製一份）
 *   [&x]      → 參考捕獲 x（直接操作外部變數）
 *   [=]       → 值捕獲所有外部變數
 *   [&]       → 參考捕獲所有外部變數
 *   [=, &x]   → 預設值捕獲，但 x 用參考捕獲
 *   [&, x]    → 預設參考捕獲，但 x 用值捕獲
 */


// ===== 重點四：Lambda 等價於手寫函數物件 =====
// Lambda 就是編譯器自動生成的函數物件類別

// 手寫的函數物件
class GreaterThanFunctor {
private:
    int threshold_;
public:
    GreaterThanFunctor(int t) : threshold_(t) {}
    bool operator()(int n) const { return n > threshold_; }
};

// 等價的 Lambda（編譯器自動生成類似的匿名類別）:
// [threshold](int n) { return n > threshold; }


// ===== 重點五：std::function 通用包裝器 =====
// std::function<回傳型別(參數型別...)> 可以儲存：
//   1. 普通函數
//   2. 函數物件（Functor）
//   3. Lambda 表達式
// 用途：需要在執行時期動態切換可呼叫物件

int add(int a, int b) { return a + b; }

class Multiplier {
public:
    int operator()(int a, int b) const { return a * b; }
};


// ===== 實戰應用：商品管理 =====
struct Product {
    std::string name;
    double price;
    int quantity;
};


// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 179. Largest Number
//   題目：給一組非負整數，重新排列使其串接後成為最大的數（以字串回傳）。
//   為什麼用到本主題：本題的靈魂就是「自訂比較器」。排序準則不是數值大小，
//     而是「a+b 串接 > b+a 串接」——這種準則無法用 std::greater 表達，
//     必須自己寫 lambda 當 functor 傳給 std::sort。這正是函數物件存在的理由：
//     把「行為」當參數傳遞。
//   複雜度：O(n log n · L)，L 為數字字串平均長度。
// -----------------------------------------------------------------------------
std::string largestNumber(std::vector<int> nums) {
    std::vector<std::string> s;
    s.reserve(nums.size());
    for (int n : nums) s.push_back(std::to_string(n));

    // 關鍵：串接比較的 lambda。注意這個比較是嚴格弱序（strict weak ordering），
    // 否則 std::sort 的行為是 UB。
    std::sort(s.begin(), s.end(),
        [](const std::string& a, const std::string& b) { return a + b > b + a; });

    // 邊界：全為 0 時會排成 "000..."，必須壓成單一 "0"
    if (!s.empty() && s[0] == "0") return "0";

    std::string out;
    for (const auto& t : s) out += t;
    return out;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 1480. Running Sum of 1d Array
//   題目：回傳前綴和陣列，runningSum[i] = nums[0] + ... + nums[i]。
//   為什麼用到本主題：這題可以「一行不寫迴圈」——std::partial_sum 搭配
//     STL 內建函數物件 std::plus<int>()。它示範了內建 functor 的典型用途：
//     把「要做什麼運算」當參數餵給泛型演算法。換成 std::multiplies<int>()
//     立刻變成前綴積，演算法本體一個字都不用改。
//   複雜度：O(n)。
// -----------------------------------------------------------------------------
std::vector<int> runningSum(const std::vector<int>& nums) {
    std::vector<int> out(nums.size());
    std::partial_sum(nums.begin(), nums.end(), out.begin(), std::plus<int>());
    return out;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 3】LeetCode 283. Move Zeroes
//   題目：把所有 0 移到陣列尾端，且非 0 元素保持原相對順序，需就地修改。
//   為什麼用到本主題：std::stable_partition 需要一個「謂詞 functor」來決定
//     誰排前面。這裡的 lambda [](int n){ return n != 0; } 就是謂詞。
//     stable 版本保證非 0 元素相對順序不變，正好對應題目要求。
//   複雜度：std::stable_partition 為 O(n)（有額外記憶體時）或 O(n log n)（無）。
// -----------------------------------------------------------------------------
void moveZeroes(std::vector<int>& nums) {
    std::stable_partition(nums.begin(), nums.end(),
                          [](int n) { return n != 0; });
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】可設定門檻的 log 等級過濾器（帶狀態的 functor）
//   場景：監控系統要能在執行期調整「只顯示 WARN 以上」或「全部顯示」。
//   為什麼用 functor：門檻是狀態。用普通函數就得靠全域變數（不可重入、
//     多執行緒下會互相干擾）；用 functor 則每個 filter 實例各自帶自己的門檻，
//     可以同時存在多個不同設定的過濾器。
// -----------------------------------------------------------------------------
enum class LogLevel { Debug = 0, Info = 1, Warn = 2, Error = 3 };

class LevelFilter {
private:
    LogLevel min_level_;
    mutable int passed_ = 0;   // 統計通過筆數；mutable 讓 const operator() 也能累加
public:
    explicit LevelFilter(LogLevel min_level) : min_level_(min_level) {}

    bool operator()(LogLevel lv) const {
        if (static_cast<int>(lv) >= static_cast<int>(min_level_)) {
            ++passed_;
            return true;
        }
        return false;
    }
    int passed() const { return passed_; }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】用 std::function 建立指令分派表（command dispatch table）
//   場景：CLI 工具／伺服器收到指令字串後，要對應到不同的處理函式。
//   為什麼用 std::function：處理器可能是普通函數、可能是帶設定的 functor、
//     也可能是就地寫的 lambda——型別全都不同，只有 std::function 這種型別
//     抹除容器才裝得進同一個 map。這就是「需要在執行期切換行為」的典型場景，
//     也是少數該付出 std::function 開銷的地方。
// -----------------------------------------------------------------------------
std::string dispatch(
    const std::vector<std::pair<std::string, std::function<std::string(const std::string&)>>>& table,
    const std::string& cmd, const std::string& arg) {
    for (const auto& entry : table) {
        if (entry.first == cmd) return entry.second(arg);
    }
    return "unknown command: " + cmd;
}


int main() {
    // ---- 示範一：函數物件基礎 ----
    std::cout << "===== 函數物件基礎 =====" << std::endl;

    Adder add_obj;
    // add_obj(3, 5) 等同於 add_obj.operator()(3, 5)
    std::cout << "Adder(3, 5) = " << add_obj(3, 5) << std::endl;  // 8

    // ---- 示範二：帶狀態的函數物件 ----
    std::cout << "\n===== 帶狀態的函數物件 =====" << std::endl;
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // 普通函數做不到這一點：傳入不同的閾值
    int count5 = std::count_if(vec.begin(), vec.end(), GreaterThan(5));
    int count7 = std::count_if(vec.begin(), vec.end(), GreaterThan(7));
    std::cout << "大於 5 的個數: " << count5 << std::endl;  // 5
    std::cout << "大於 7 的個數: " << count7 << std::endl;  // 3

    int div3 = std::count_if(vec.begin(), vec.end(), IsDivisibleBy(3));
    std::cout << "3 的倍數個數: " << div3 << std::endl;  // 3

    // ---- 示範三：狀態累積函數物件 ----
    std::cout << "\n===== 狀態累積函數物件 =====" << std::endl;
    std::vector<int> data = {1, 2, 3, 2, 4, 2, 5, 2, 6};
    Counter counter(2);
    // for_each 回傳函數物件的副本，需要用變數接收
    Counter result = std::for_each(data.begin(), data.end(), counter);
    std::cout << "2 出現的次數: " << result.get_count() << std::endl;  // 4

    // ---- 示範四：STL 內建函數物件 ----
    std::cout << "\n===== STL 內建算術函數物件 =====" << std::endl;
    std::plus<int> plus_obj;
    std::multiplies<int> mult_obj;
    std::cout << "plus(3, 5) = " << plus_obj(3, 5) << std::endl;        // 8
    std::cout << "multiplies(4, 6) = " << mult_obj(4, 6) << std::endl;  // 24

    // 計算連乘積（1*2*3*4*5 = 120）
    std::vector<int> nums = {1, 2, 3, 4, 5};
    int product = std::accumulate(nums.begin(), nums.end(), 1,
                                  std::multiplies<int>());
    std::cout << "1*2*3*4*5 = " << product << std::endl;  // 120

    std::cout << "\n===== STL 內建比較函數物件 =====" << std::endl;
    std::vector<int> sortable = {5, 2, 8, 1, 9, 3};

    // 使用 std::greater<int>() 進行降序排序
    std::sort(sortable.begin(), sortable.end(), std::greater<int>());
    std::cout << "降序: ";
    for (int n : sortable) std::cout << n << " ";  // 9 8 5 3 2 1
    std::cout << std::endl;

    // 使用 std::less<int>() 進行升序排序（預設行為）
    std::sort(sortable.begin(), sortable.end(), std::less<int>());
    std::cout << "升序: ";
    for (int n : sortable) std::cout << n << " ";  // 1 2 3 5 8 9
    std::cout << std::endl;

    // ---- 示範五：Lambda 基本用法 ----
    std::cout << "\n===== Lambda 基本用法 =====" << std::endl;
    std::vector<int> lv = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // 不捕獲任何變數的 Lambda
    auto print = [](int n) { std::cout << n << " "; };
    std::for_each(lv.begin(), lv.end(), print);
    std::cout << std::endl;

    // 帶回傳值的 Lambda
    auto square = [](int n) { return n * n; };
    std::cout << "5 的平方: " << square(5) << std::endl;  // 25

    // 統計偶數個數
    int even_count = std::count_if(lv.begin(), lv.end(),
        [](int n) { return n % 2 == 0; });
    std::cout << "偶數個數: " << even_count << std::endl;  // 5

    // ---- 示範六：Lambda 捕獲機制 ----
    std::cout << "\n===== Lambda 值捕獲 =====" << std::endl;
    int threshold = 5;

    // 值捕獲：Lambda 內部有 threshold 的副本
    int count_val = std::count_if(lv.begin(), lv.end(),
        [threshold](int n) { return n > threshold; });
    std::cout << "大於 " << threshold << " 的個數: " << count_val << std::endl;

    // 值捕獲 + mutable：允許修改副本（不影響外部）
    auto by_value = [threshold]() mutable {
        threshold = 100;  // 只修改 Lambda 內的副本
        return threshold;
    };
    by_value();
    std::cout << "外部 threshold 仍然是: " << threshold << std::endl;  // 5（未改變）

    std::cout << "\n===== Lambda 參考捕獲 =====" << std::endl;
    int sum = 0;
    // 參考捕獲：Lambda 直接修改外部的 sum
    std::for_each(lv.begin(), lv.end(), [&sum](int n) { sum += n; });
    std::cout << "總和: " << sum << std::endl;  // 55

    // 混合捕獲：[=, &b] 預設值捕獲，b 用參考捕獲
    std::cout << "\n===== 混合捕獲 =====" << std::endl;
    int a = 10, b = 20, c = 30;
    auto lambda1 = [=, &b]() {
        b = a + c;  // a 和 c 是副本，b 是參考，可以修改 b
    };
    lambda1();
    std::cout << "a=" << a << ", b=" << b << ", c=" << c << std::endl;
    // a=10, b=40, c=30（只有 b 被修改）

    // ---- 示範七：mutable Lambda ----
    std::cout << "\n===== mutable Lambda =====" << std::endl;
    int counter_var = 0;
    // 沒有 mutable：不能修改值捕獲的變數
    // auto bad = [counter_var]() { return ++counter_var; };  // 編譯錯誤！

    // 加上 mutable：可以修改副本
    auto increment = [counter_var]() mutable {
        return ++counter_var;  // 修改的是 Lambda 內部的副本
    };
    std::cout << "increment(): " << increment() << std::endl;  // 1
    std::cout << "increment(): " << increment() << std::endl;  // 2（Lambda 記住了狀態）
    std::cout << "increment(): " << increment() << std::endl;  // 3
    std::cout << "外部 counter_var: " << counter_var << std::endl;  // 0（未被修改）

    // ---- 示範八：泛型 Lambda（C++14）----
    std::cout << "\n===== 泛型 Lambda（C++14）=====" << std::endl;
    // auto 參數：同一個 Lambda 可用於不同型別
    auto generic_print = [](const auto& x) { std::cout << x << " "; };

    std::vector<int> int_vec = {1, 2, 3};
    std::vector<double> dbl_vec = {1.1, 2.2, 3.3};
    std::vector<std::string> str_vec = {"Hello", "World"};

    std::cout << "ints: ";
    std::for_each(int_vec.begin(), int_vec.end(), generic_print);
    std::cout << std::endl;

    std::cout << "doubles: ";
    std::for_each(dbl_vec.begin(), dbl_vec.end(), generic_print);
    std::cout << std::endl;

    std::cout << "strings: ";
    std::for_each(str_vec.begin(), str_vec.end(), generic_print);
    std::cout << std::endl;

    // ---- 示範九：std::function 包裝器 ----
    std::cout << "\n===== std::function 通用包裝器 =====" << std::endl;
    // std::function<int(int, int)> 可以儲存任何接受兩個 int 回傳 int 的可呼叫物件
    std::function<int(int, int)> func;

    func = add;          // 儲存普通函數
    std::cout << "普通函數 add(3,5) = " << func(3, 5) << std::endl;  // 8

    func = Multiplier(); // 儲存函數物件
    std::cout << "函數物件 Multiplier(3,5) = " << func(3, 5) << std::endl;  // 15

    func = [](int a, int b) { return a - b; };  // 儲存 Lambda
    std::cout << "Lambda 減法(3,5) = " << func(3, 5) << std::endl;  // -2

    // 儲存多個操作的向量（多型行為）
    std::vector<std::function<int(int, int)>> operations = {
        add,
        Multiplier(),
        [](int a, int b) { return a - b; },
        [](int a, int b) { return a / b; }
    };
    std::vector<std::string> op_names = {"加", "乘", "減", "除"};
    int x = 20, y = 4;
    for (size_t i = 0; i < operations.size(); ++i) {
        std::cout << x << " " << op_names[i] << " " << y << " = "
                  << operations[i](x, y) << std::endl;
    }

    // ---- 示範十：實戰：Lambda 與 STL 演算法組合 ----
    std::cout << "\n===== 實戰：商品管理系統 =====" << std::endl;
    std::vector<Product> products = {
        {"Apple",  1.50, 100},
        {"Banana", 0.75, 150},
        {"Orange", 2.00,  80},
        {"Mango",  3.00,  50},
        {"Grape",  2.50, 120}
    };

    // 按價格升序排序
    std::sort(products.begin(), products.end(),
        [](const Product& a, const Product& b) { return a.price < b.price; });
    std::cout << "按價格排序後：" << std::endl;
    for (const auto& p : products) {
        std::cout << "  " << p.name << ": $" << p.price << std::endl;
    }

    // 計算總庫存價值
    double total = std::accumulate(products.begin(), products.end(), 0.0,
        [](double sum, const Product& p) { return sum + p.price * p.quantity; });
    std::cout << "總庫存價值: $" << total << std::endl;

    // 統計低庫存產品（數量 < 100）
    int low_stock = std::count_if(products.begin(), products.end(),
        [](const Product& p) { return p.quantity < 100; });
    std::cout << "低庫存產品數: " << low_stock << std::endl;

    // 對所有產品打 9 折
    double discount = 0.9;
    std::for_each(products.begin(), products.end(),
        [discount](Product& p) { p.price *= discount; });
    std::cout << "打折後第一項: $" << products[0].price << std::endl;

    // ---- LeetCode 179: 自訂比較器 lambda ----
    std::cout << "\n=== LeetCode 179. Largest Number ===" << std::endl;
    std::cout << "[10,2]        -> " << largestNumber({10, 2}) << std::endl;
    std::cout << "[3,30,34,5,9] -> " << largestNumber({3, 30, 34, 5, 9}) << std::endl;
    std::cout << "[0,0]         -> " << largestNumber({0, 0}) << std::endl;

    // ---- LeetCode 1480: STL 內建函數物件 std::plus ----
    std::cout << "\n=== LeetCode 1480. Running Sum of 1d Array ===" << std::endl;
    for (int n : runningSum({1, 2, 3, 4})) std::cout << n << " ";
    std::cout << std::endl;
    for (int n : runningSum({1, 1, 1, 1, 1})) std::cout << n << " ";
    std::cout << std::endl;

    // ---- LeetCode 283: lambda 當謂詞 ----
    std::cout << "\n=== LeetCode 283. Move Zeroes ===" << std::endl;
    std::vector<int> mz = {0, 1, 0, 3, 12};
    moveZeroes(mz);
    for (int n : mz) std::cout << n << " ";
    std::cout << std::endl;

    // ---- 日常實務 1: 帶狀態的 log 過濾器 ----
    std::cout << "\n=== 日常實務: log 等級過濾器 ===" << std::endl;
    std::vector<LogLevel> log_stream = {
        LogLevel::Debug, LogLevel::Info, LogLevel::Warn,
        LogLevel::Error, LogLevel::Debug, LogLevel::Error
    };
    // 兩個過濾器各自帶不同門檻，同時存在、互不干擾
    LevelFilter warn_up(LogLevel::Warn);
    LevelFilter all_logs(LogLevel::Debug);
    int shown = std::count_if(log_stream.begin(), log_stream.end(), std::ref(warn_up));
    int all_cnt = std::count_if(log_stream.begin(), log_stream.end(), std::ref(all_logs));
    std::cout << "WARN 以上筆數: " << shown << " / 全部: " << all_cnt << std::endl;
    // 注意：這裡刻意用 std::ref 包裝，才能讀到原物件累積的狀態。
    // 若直接傳 warn_up，count_if 操作的是副本，warn_up.passed() 會是 0。
    std::cout << "warn_up 內部累計 (經 std::ref): " << warn_up.passed() << std::endl;

    // ---- 日常實務 2: std::function 指令分派表 ----
    std::cout << "\n=== 日常實務: std::function 指令分派表 ===" << std::endl;
    std::vector<std::pair<std::string, std::function<std::string(const std::string&)>>> table = {
        {"upper",  [](const std::string& s) {
                       std::string r = s;
                       for (char& ch : r) ch = static_cast<char>(
                           std::toupper(static_cast<unsigned char>(ch)));
                       return r;
                   }},
        {"reverse", [](const std::string& s) { return std::string(s.rbegin(), s.rend()); }},
        {"len",     [](const std::string& s) { return std::to_string(s.size()); }}
    };
    std::cout << "upper(hello)   = " << dispatch(table, "upper", "hello") << std::endl;
    std::cout << "reverse(hello) = " << dispatch(table, "reverse", "hello") << std::endl;
    std::cout << "len(hello)     = " << dispatch(table, "len", "hello") << std::endl;
    std::cout << dispatch(table, "sort", "hello") << std::endl;

    std::cout << "\n===== 第 8 課總複習完成 =====" << std::endl;

    return 0;
}

/*
 * ============================================================
 * 重點整理表
 * ============================================================
 *
 * | 方式           | 語法                          | 優點             | 缺點           |
 * |---------------|-------------------------------|-----------------|----------------|
 * | 普通函數       | bool f(int x) { ... }         | 簡單             | 無法攜帶狀態    |
 * | 函數物件       | class F { bool operator()().. }| 可攜帶狀態       | 需要寫類別      |
 * | Lambda        | [cap](params){ body }          | 就地定義，簡潔   | 複雜情況難閱讀   |
 * | std::function | std::function<sig> f = ...    | 通用，可動態切換 | 有額外開銷      |
 *
 * 使用建議：
 * - 簡單的一次性行為 → 用 Lambda
 * - 需要複雜狀態 → 用自訂函數物件類別
 * - 常見操作（排序、累積）→ 用 STL 內建函數物件
 * - 需要在執行時動態切換行為 → 用 std::function
 * ============================================================
 */

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
// ===== 函數物件基礎 =====
// Adder(3, 5) = 8
//
// ===== 帶狀態的函數物件 =====
// 大於 5 的個數: 5
// 大於 7 的個數: 3
// 3 的倍數個數: 3
//
// ===== 狀態累積函數物件 =====
// 2 出現的次數: 4
//
// ===== STL 內建算術函數物件 =====
// plus(3, 5) = 8
// multiplies(4, 6) = 24
// 1*2*3*4*5 = 120
//
// ===== STL 內建比較函數物件 =====
// 降序: 9 8 5 3 2 1 
// 升序: 1 2 3 5 8 9 
//
// ===== Lambda 基本用法 =====
// 1 2 3 4 5 6 7 8 9 10 
// 5 的平方: 25
// 偶數個數: 5
//
// ===== Lambda 值捕獲 =====
// 大於 5 的個數: 5
// 外部 threshold 仍然是: 5
//
// ===== Lambda 參考捕獲 =====
// 總和: 55
//
// ===== 混合捕獲 =====
// a=10, b=40, c=30
//
// ===== mutable Lambda =====
// increment(): 1
// increment(): 2
// increment(): 3
// 外部 counter_var: 0
//
// ===== 泛型 Lambda（C++14）=====
// ints: 1 2 3 
// doubles: 1.1 2.2 3.3 
// strings: Hello World 
//
// ===== std::function 通用包裝器 =====
// 普通函數 add(3,5) = 8
// 函數物件 Multiplier(3,5) = 15
// Lambda 減法(3,5) = -2
// 20 加 4 = 24
// 20 乘 4 = 80
// 20 減 4 = 16
// 20 除 4 = 5
//
// ===== 實戰：商品管理系統 =====
// 按價格排序後：
//   Banana: $0.75
//   Apple: $1.5
//   Orange: $2
//   Grape: $2.5
//   Mango: $3
// 總庫存價值: $872.5
// 低庫存產品數: 2
// 打折後第一項: $0.675
//
// === LeetCode 179. Largest Number ===
// [10,2]        -> 210
// [3,30,34,5,9] -> 9534330
// [0,0]         -> 0
//
// === LeetCode 1480. Running Sum of 1d Array ===
// 1 3 6 10 
// 1 2 3 4 5 
//
// === LeetCode 283. Move Zeroes ===
// 1 3 12 0 0 
//
// === 日常實務: log 等級過濾器 ===
// WARN 以上筆數: 3 / 全部: 6
// warn_up 內部累計 (經 std::ref): 3
//
// === 日常實務: std::function 指令分派表 ===
// upper(hello)   = HELLO
// reverse(hello) = olleh
// len(hello)     = 5
// unknown command: sort
//
// ===== 第 8 課總複習完成 =====
