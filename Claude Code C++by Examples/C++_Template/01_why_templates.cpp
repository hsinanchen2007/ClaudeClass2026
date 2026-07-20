// ============================================================================
//  01_why_templates.cpp  ──  為什麼需要 Template？(Why Templates?)
// ============================================================================
//
//  【本篇定位】
//    這是 C++ Template 系列的第一篇。本篇「不深入語法」，重點放在動機：
//      1. Template 出現之前，C/C++ 程式設計師如何處理「同一份邏輯處理
//         多種型別」這種需求？
//      2. 那些舊方法各自有哪些缺點？
//      3. Template 如何優雅地解決這些問題？
//    下一篇 (02_function_template_basics.cpp) 才會正式進入語法細節。
//
// ----------------------------------------------------------------------------
//  【場景】
//    你寫了一個 max(a, b)，回傳兩者中較大的。一開始可能寫成這樣：
//        int max(int a, int b) { return a > b ? a : b; }
//    但專案一大就發現：double 也要、long long 也要、自定義的 Money、
//    Date 都要⋯⋯。怎麼辦？以下是 template 出現之前的幾種「土法煉鋼」。
//
// ----------------------------------------------------------------------------
//  【舊方法 1 ── Function Overloading】
//
//        int    max(int a, int b)       { return a > b ? a : b; }
//        double max(double a, double b) { return a > b ? a : b; }
//        long   max(long a, long b)     { return a > b ? a : b; }
//
//    優點：型別安全、編譯期檢查、效能與直接寫等價。
//    缺點：每個型別都要重寫一遍 → 違反 DRY (Don't Repeat Yourself)；
//         邏輯一改，所有版本都要同步改；新型別出現要繼續加。
//
// ----------------------------------------------------------------------------
//  【舊方法 2 ── C 的 void* / 函式指標 (qsort 風格)】
//
//        // <cstdlib>
//        void qsort(void* base, size_t n, size_t size,
//                   int (*cmp)(const void*, const void*));
//
//    缺點：
//      - 完全失去型別資訊：cast 來 cast 去、編譯器無法檢查
//      - 比較函式內部要手動轉型，容易踩雷
//      - 函式指標呼叫無法 inline → 效能比 template 差
//      - runtime cost：每次呼叫都要走間接跳轉
//
// ----------------------------------------------------------------------------
//  【舊方法 3 ── C Macro】
//
//        #define MAX(a, b) ((a) > (b) ? (a) : (b))
//
//    缺點（macro 經典地雷）：
//      - 沒有型別檢查 (MAX("a", 1) 也會編譯通過或詭異錯誤)
//      - 「重複求值副作用」：MAX(i++, j++) 會把 i++、j++ 求值兩次！
//      - 不在符號表 → debugger 抓不到、不能放進 namespace
//      - 不能取位址、不能有 default 引數
//
// ----------------------------------------------------------------------------
//  【Template 的解法】
//
//        template <typename T>
//        T max_t(T a, T b) { return a > b ? a : b; }
//
//    一份原始碼，編譯器依使用時的型別「實例化 (instantiate)」對應版本。
//
//    Template 帶來的好處：
//      ◆ 型別安全：編譯期檢查，型別不對直接報錯。
//      ◆ 零 runtime overhead：每個型別都產出對應的具體版本，可以被
//                              inline，效能與手寫 overload 等價。
//      ◆ DRY：邏輯只寫一遍。
//      ◆ 不會重複求值（因為它是真正的函式呼叫，不是文字替換）。
//      ◆ 可以放在 namespace、可以 debug、可以取位址。
//
//    關鍵術語（之後篇章會反覆出現）：
//      - 「Template parameter」          模板參數，例如上面的 T。
//      - 「Template argument」           模板實參，例如 max_t<int>(...)
//                                       中的 int。
//      - 「Template instantiation」      模板實例化：編譯器依 argument
//                                       產出具體型別版本的程式碼。
//      - 「Template argument deduction」 模板引數推導：編譯器自己從函
//                                       式呼叫的引數型別猜出 T。
//
//    參考：https://en.cppreference.com/cpp/language/templates
// ============================================================================

/*
補充筆記：why_templates
  - why_templates 涉及模板實例化；請先判斷哪些型別由呼叫端推導，哪些型別由程式指定。
  - 泛型程式要把型別需求寫清楚，例如可比較、可移動、可呼叫或符合某個 concept。
  - 模板技巧的價值在減少重複且保留型別安全，不是讓錯誤訊息變得更長。
  - why_templates 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】為什麼需要 Template
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 模板是「執行期多型」還是「編譯期多型」？
//     答：編譯期多型（靜態多型）。模板是編譯期的程式碼生成規則，編譯器在實例化
//         時用具體型別產生一份獨立程式碼，沒有 vtable、沒有間接跳轉、可完全 inline。
//         virtual 才是 runtime polymorphism，靠 vptr/vtable 在執行期分派。
//     追問：那模板換來效能的代價是什麼？
//
// 🔥 Q2. 既然模板會 code bloat，它比 qsort 那種 void* + 函式指標好在哪？
//     答：void* 版本失去型別資訊、比較函式無法 inline、每次呼叫都走間接跳轉。
//         模板為每個型別各生一份「已針對該型別最佳化」的碼，效能等同手寫重載版本。
//         代價是編譯時間變長、二進位變大、錯誤訊息冗長，且定義必須放在 header。
//
// Q3. max_t<std::string>("apple", "banana") 為什麼必須顯式指定 T？
//     答：兩個字串字面量的型別是 const char[6] 與 const char[7]，陣列大小不同。
//         兩個引數共用同一個 T，推導結果互相衝突（deduction conflict）而失敗。
//         顯式指定 <std::string> 就跳過引數推導，兩邊都轉成 std::string 再比較。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>

// ─── 1. 最簡單的 function template ─────────────────────────────────────────
//   觀察重點：
//     - 對 int 可以、對 double 可以、對 std::string 也可以（因 string 有 operator>）。
//     - 對使用者自訂型別也行，只要那個型別有 overload operator>。
//     - 編譯器會自動推導 T，所以呼叫端寫 max_t(3, 7) 就好，不必 max_t<int>(3,7)。
template <typename T>
T max_t(T a, T b) {
    return (a > b) ? a : b;
}

// ─── 2. Leetcode 1480 ── Running Sum of 1d Array ──────────────────────────
//   題目：給一個陣列 nums，回傳 result[i] = nums[0] + nums[1] + ... + nums[i]。
//   範例：nums = [1,2,3,4] → [1,3,6,10]
//
//   為什麼這題適合 template？
//     原題雖然 type 是 int，但「累加前 i 個元素」這個邏輯跟具體型別無關，
//     只要型別支援 += 就行。我們做成 template，一份程式可以同時對 int /
//     long long / double / 自訂的 BigInt 等使用，避免重寫。
//
//   時間複雜度：O(n)，n = nums.size()
//   空間複雜度：O(n)（輸出 vector）；若就地修改原陣列可做到 O(1) 額外空間。
//   邊界條件：
//     - nums 為空 → 直接回傳空 vector
//     - 累加可能 overflow：呼叫端負責用合適大小的型別 (例如 long long)
template <typename T>
std::vector<T> running_sum(const std::vector<T>& nums) {
    std::vector<T> result;
    result.reserve(nums.size());            // 一次配置好，避免多次擴容
    T running = T{};                        // T{} = value-initialize；int 為 0
    for (const T& x : nums) {
        running += x;                       // 要求 T 支援 operator+=
        result.push_back(running);
    }
    return result;
}

// ─── 3. 工作實用範例：通用 clamp ──────────────────────────────────────────
//   clamp(v, lo, hi)：
//     - 若 v < lo  → 回傳 lo
//     - 若 v > hi  → 回傳 hi
//     - 否則      → 回傳 v
//
//   C++17 起標準庫已有 std::clamp（在 <algorithm>），這裡示範自己寫
//   template 版本以理解其原理。
//
//   日常工作會在哪裡用到？
//     - UI：亮度 0~100、音量 0~255、卷軸位置 0~max
//     - 遊戲：把座標限制在地圖內、限制血量在 [0, maxHP]
//     - 計算：避免結果溢出顯示範圍 (例如 RGB 0~255)
//
//   設計提醒：
//     - 引數順序設成 (v, lo, hi)，與 std::clamp 相同，便於將來無痛切換。
//     - 假設 lo <= hi；若不確定可加 assert，本範例為簡潔省略。
template <typename T>
T clamp_t(T v, T lo, T hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// ─── 4. Leetcode 1672 ── Richest Customer Wealth (template 版) ───────────
//   難度: easy
//   題目：accounts[i][j] 代表第 i 個顧客在第 j 家銀行的存款。
//        回傳「最有錢的顧客」總財產。
//   範例：[[1,2,3],[3,2,1]] → 6 (兩個人都是 6)
//
//   為什麼放在這裡？
//     再次驗證 template 的「一份程式可處理多型別」威力：原題型別是 int，
//     但這個邏輯對任何能 += 與比較的型別都成立。
//
//   時間複雜度：O(m·n)
//   空間複雜度：O(1)
template <typename T>
T maximum_wealth(const std::vector<std::vector<T>>& accounts) {
    T best = T{};
    for (const auto& row : accounts) {
        T sum = T{};
        for (const T& v : row) sum += v;
        if (sum > best) best = sum;
    }
    return best;
}

// ─── 5. 工作實用：通用 lerp (linear interpolation) ─────────────────────────
//   公式：lerp(a, b, t) = a + (b - a) * t；t 通常在 [0,1]。
//   常見應用：動畫位置插值、顏色漸層、音量淡入淡出。
//   C++20 標準庫已有 std::lerp，這裡示範 template 版以理解原理。
template <typename T>
T lerp_t(T a, T b, T t) {
    return a + (b - a) * t;
}

// ─── main：實測上面三段 ────────────────────────────────────────────────────
int main() {
    // (1) max_t：跨型別測試
    std::cout << "max_t(3, 7)                   = " << max_t(3, 7)         << "\n";
    std::cout << "max_t(2.5, 1.8)               = " << max_t(2.5, 1.8)     << "\n";
    // 字面量 "apple" 是 const char*，要不互相能比較會有歧義；
    // 這裡顯式指定 T = std::string 以避免推導問題。
    std::cout << "max_t<string>(\"apple\",\"banana\") = "
              << max_t<std::string>("apple", "banana") << "\n";

    // (2) Leetcode 1480 Running Sum
    std::vector<int> nums{1, 2, 3, 4};
    auto rs = running_sum(nums);    // T 推導為 int
    std::cout << "running_sum([1,2,3,4])        = ";
    for (int x : rs) std::cout << x << ' ';
    std::cout << "\n";

    // (3) clamp
    std::cout << "clamp_t(150, 0, 100)          = " << clamp_t(150, 0, 100) << "\n";
    std::cout << "clamp_t( -5, 0, 100)          = " << clamp_t(-5,  0, 100) << "\n";
    std::cout << "clamp_t( 50, 0, 100)          = " << clamp_t(50,  0, 100) << "\n";

    // (4) Leetcode 1672 Richest Customer Wealth
    std::vector<std::vector<int>> accounts{{1, 2, 3}, {3, 2, 1}};
    std::cout << "maximum_wealth                = " << maximum_wealth(accounts) << "\n";

    // (5) lerp
    std::cout << "lerp_t(0.0, 10.0, 0.5)        = " << lerp_t(0.0, 10.0, 0.5) << "\n";
    std::cout << "lerp_t(100, 200, 0)           = " << lerp_t(100, 200, 0) << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：Template 跟 #define MAX(a,b) 巨集，到底差在哪？
    //    A：巨集是「文字替換」，沒有型別檢查、會重複求值 (MAX(i++,j++) 把
    //       i++ 算兩次)、不在符號表所以無法 debug 也不能放 namespace。
    //       Template 是真正的函式，編譯期型別檢查、引數只求值一次、可
    //       inline、可放 namespace、可取位址，安全性與可維護性完勝巨集。
    //
    //  Q2：聽說 template 會造成「code bloat」，那它跟巨集相比效能優勢在哪？
    //    A：每個型別實例化會產生一份對應的程式碼，確實可能讓二進位變大；
    //       但每份都是「直接針對該型別最佳化」的版本，可被 inline，效能
    //       與手寫版本等價，沒有 void* / 函式指標那種間接呼叫的 runtime
    //       開銷。實務上 linker 會合併重複的弱符號 (COMDAT)，膨脹通常可控。
    //
    //  Q3：max_t<std::string>("apple","banana") 為什麼必須顯式指定 T？
    //    A：字串字面量的型別是 const char[6]、const char[7]，兩者陣列大
    //       小不同，且模板對「兩個引數共用同一個 T」會推導失敗 (deduction
    //       conflict)。顯式寫 <std::string> 跳過引數推導、強制把兩邊都
    //       轉成 std::string，才能順利呼叫 std::string 的 operator>。
    //
    return 0;
}

// ============================================================================
//  【小結】
//    Template 出現的核心動機：在不犧牲型別安全與效能的前提下，避免為了不
//    同型別寫一堆幾乎相同的程式碼。它把「處理什麼型別」的決策交給編譯
//    器，使我們專心寫「演算法」本身。
//
//  【下一篇預告】
//    02_function_template_basics.cpp 會講語法細節：
//      - template<typename T> 與 template<class T> 的差別
//      - 引數推導規則 (argument deduction)
//      - 多個 template 參數
//      - 顯式指定型別 vs 自動推導
// ============================================================================

// 編譯: g++ -std=c++20 -Wall -Wextra 01_why_templates.cpp -o 01_why_templates

// === 預期輸出 ===
// max_t(3, 7)                   = 7
// max_t(2.5, 1.8)               = 2.5
// max_t<string>("apple","banana") = banana
// running_sum([1,2,3,4])        = 1 3 6 10
// clamp_t(150, 0, 100)          = 100
// clamp_t( -5, 0, 100)          = 0
// clamp_t( 50, 0, 100)          = 50
// maximum_wealth                = 6
// lerp_t(0.0, 10.0, 0.5)        = 5
// lerp_t(100, 200, 0)           = 100
