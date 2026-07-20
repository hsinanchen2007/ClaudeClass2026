// =============================================================================
//  第四課：迭代器的核心概念 11  —  範圍 for 的底層原理與值／參考的差別
// =============================================================================
//
// 【主題資訊 Information】
//   語法：for (宣告 : 範圍運算式) 陳述句
//   標準版本：
//     C++11  引入範圍 for；展開時 __begin 與 __end 必須是**同型別**
//     C++17  放寬為 __end 可以是不同型別的 sentinel（為 ranges 鋪路）；
//            並修正了暫時物件生命週期的部分問題
//     C++20  加入初始化子句：for (auto v = get(); auto& x : v)
//     C++23  修正「範圍運算式產生的暫時物件」的懸空問題（P2718R0）
//   複雜度：與等價的迭代器迴圈完全相同，O(N)。
//   標頭檔：不需要（語言核心特性）；泛型情境會用到 <iterator> 的 std::begin/end。
//
// 【詳細解釋 Explanation】
//
// 【1. 編譯器實際展開成什麼（C++17 起的規則）】
//       for (int n : vec) { body }
//   等價於：
//       {
//           auto&& __range = vec;                     // 萬用參考，可綁暫時物件
//           auto   __begin = /* 見下 */;
//           auto   __end   = /* 見下 */;
//           for (; __begin != __end; ++__begin) {
//               int n = *__begin;                     // ★ 這行是「初始化」= 複製
//               body
//           }
//       }
//   __begin / __end 的取得規則有三條，依序嘗試：
//     (a) 若 __range 是陣列 → __begin = __range, __end = __range + N
//     (b) 若型別有成員 begin() 與 end() → 用成員版
//     (c) 否則用 ADL 找自由函式 begin(__range) / end(__range)
//   所以「讓自訂類別支援範圍 for」只要提供 (b) 或 (c) 其中一種即可，
//   完全不需要繼承任何基底類別。
//
// 【2. 為什麼 for (auto x : v) 的修改不會生效】
//   關鍵就在展開後那行 `int n = *__begin;` —— 它是**初始化一個新變數**，
//   對 int 是複製一份值，對 std::string 是一次完整的深複製（含堆積配置）。
//   你改的是這個副本，容器裡的元素完全沒被碰到。
//   三種寫法的正確用途：
//       for (const auto& x : v)   唯讀 —— **預設就該這樣寫**，零複製
//       for (auto& x : v)         要修改元素
//       for (auto x : v)          真的需要一份可任意破壞的副本時
//   對 vector<std::string> 用 auto x 而非 const auto&，
//   在百萬筆資料上就是百萬次堆積配置與釋放 —— 這是很常見的效能地雷。
//
// 【3. auto&& 的角色：讓暫時物件活到迴圈結束】
//   __range 宣告成 auto&&（萬用參考）而不是 auto& 是有原因的：
//       for (const auto& x : makeVector()) { ... }
//   makeVector() 回傳的是暫時物件（prvalue）。auto&& 綁定它會觸發
//   「參考延長暫時物件生命週期」規則，讓它活到整個迴圈結束。
//   若寫成 auto&（左值參考）根本綁不上；寫成 auto 則會多複製一次。
//   **但這個保護只涵蓋最外層那個物件**：
//       for (const auto& c : getObject().getVector()) { ... }   // C++20 以前是懸空！
//   getObject() 產生的暫時物件在完整運算式結束時就被銷毀，
//   而 __range 只綁到它的成員 —— 於是迴圈存取的是已銷毀物件的成員。
//   這個坑到 C++23（P2718R0）才被語言層面修好。
//
// 【4. 範圍 for 內修改容器同樣會失效】
//   很多人以為範圍 for「比較安全」，其實它只是把迭代器藏起來：
//   __begin / __end 在迴圈開始時就取好了，
//   迴圈中 push_back 觸發重新配置 → 兩者立刻失效 → 未定義行為。
//   要邊走邊刪請用明確的迭代器迴圈搭配 erase 的回傳值（見本課第 8、9 個範例）。
//
// 【概念補充 Concept Deep Dive】
//   為什麼 __end 只求值「一次」，這件事很重要？
//   看等價展開就明白：__end 是在迴圈**開始前**計算並保存的，
//   之後每輪只做 __begin != __end 的比較。
//   對照傳統寫法：
//       for (auto it = v.begin(); it != v.end(); ++it)   // 每輪都呼叫 v.end()
//   兩者在 vector 上因為 end() 極廉價且可 inline 而沒有差別，
//   但若容器的 end() 是昂貴計算（例如某些惰性 view、或需要走訪才知道結尾的
//   forward_list 包裝），範圍 for 的「只算一次」就是實質優勢。
//   這也正是 C++20 ranges 引入 sentinel（__end 可以是不同型別、甚至只是個
//   「判斷是否結束」的謂詞物件）的動機 —— C++17 放寬 __begin/__end 型別必須相同
//   這條限制，就是為此鋪路。
//
// 【注意事項 Pay Attention】
//   1. for (auto x : v) 會複製每個元素；唯讀請一律寫 const auto&。
//   2. 迴圈內修改容器（push_back / erase）可能使內部迭代器失效 → UB。
//   3. for (auto& x : getVector()) 在 C++11/14/17 中，若範圍運算式是
//      「暫時物件的成員」會產生懸空參考；C++23 才修好。安全作法是先存進具名變數。
//   4. 對 std::vector<bool> 不能寫 for (auto& b : v) —— 它的 operator[]
//      回傳的是 proxy 物件而非真正的 bool&。要修改請用 for (auto&& b : v)。
//   5. 範圍 for 拿不到索引；需要索引時請用傳統 for，
//      或 C++20 的 std::views::enumerate（C++23 正式納入）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】範圍 for 的底層原理
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 範圍 for 展開成什麼？要讓自訂類別支援它需要做什麼？
//     答：展開成「取 __begin/__end → 迴圈 ++ → 解參考初始化迴圈變數」。
//         要支援它，自訂類別只要提供成員 begin()/end()，
//         或提供 ADL 找得到的自由函式 begin(obj)/end(obj)，二擇一即可。
//         回傳的物件最少要支援 operator*、operator++（前置）與 operator!=。
//         不需要繼承任何基底類別，也不需要提供 iterator_traits（除非要用 STL 演算法）。
//     追問：C++17 對範圍 for 做了什麼修改？
//           → 放寬 __begin 與 __end 可以是**不同型別**（sentinel）。
//             這讓「結尾條件不是一個真正的位置」的範圍成為可能，
//             例如以 '\0' 為結尾的 C 字串，是 C++20 ranges 的基礎。
//
// 🔥 Q2. for (auto x : v)、for (auto& x : v)、for (const auto& x : v) 該怎麼選？
//     答：預設寫 const auto&（唯讀且零複製）；要修改元素寫 auto&；
//         只有真的需要一份可破壞的副本時才寫 auto。
//         關鍵在展開後那行 `T x = *__begin;` 是一次初始化 ——
//         寫 auto 就是每輪複製一次，對 std::string 或大型結構代價很高。
//     追問：那什麼時候該用 auto&&？
//           → 泛型程式碼中（不確定元素型別會不會是 proxy 時），
//             以及 std::vector<bool> —— 它的 operator[] 回傳 proxy 物件，
//             for (auto& b : v) 編譯不過，必須寫 auto&&。
//
// ⚠️ 陷阱. for (const auto& c : getConfig().items()) { ... } 為什麼可能是懸空參考？
//     答：auto&& __range 只延長「範圍運算式本身」那個物件的生命週期。
//         這裡範圍運算式是 .items()（回傳成員的參考），
//         而 getConfig() 產生的暫時物件在該完整運算式結束時就被銷毀了 ——
//         __range 綁的是一個已死物件的成員。C++11/14/17 都是如此，
//         直到 C++23（P2718R0）才在語言層面修好。
//         安全寫法：auto cfg = getConfig(); for (const auto& c : cfg.items())
//     為什麼會錯：以為「範圍 for 會自動幫我延長生命週期」。
//         它確實會，但只針對最外層那一個物件；
//         一旦範圍運算式是「暫時物件的某個成員或方法回傳值」，保護就失效了。
//         這類 bug 很難察覺，因為被銷毀的記憶體通常還沒被覆寫，測試常常會通過。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <numeric>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1480. Running Sum of 1d Array
//   題目：回傳 runningSum，其中 runningSum[i] = nums[0] + ... + nums[i]。
//   為什麼用到本主題：這題示範範圍 for 的兩種正確用法在同一份程式碼裡的分工：
//         讀取來源用 const auto&（零複製），
//         就地改寫結果用 auto&（真正改到元素本身）。
//         若把第二個迴圈誤寫成 auto，前綴和就完全不會生效 ——
//         這正是本檔要傳達的核心差異，而且是可以實際跑出來看的。
//   複雜度：時間 O(N)、空間 O(1)（就地版本）。
// -----------------------------------------------------------------------------

// 版本 A：產生新陣列（用 const auto& 唯讀走訪來源）
std::vector<int> runningSumCopy(const std::vector<int>& nums) {
    std::vector<int> out;
    out.reserve(nums.size());
    int sum = 0;
    for (const auto& n : nums) {     // 唯讀 → const auto&
        sum += n;
        out.push_back(sum);
    }
    return out;
}

// 版本 B：就地改寫（必須用 auto&，否則改到的是副本）
void runningSumInPlace(std::vector<int>& nums) {
    int sum = 0;
    for (auto& n : nums) {           // 要修改 → auto&
        sum += n;
        n = sum;                     // 若上面寫 auto，這行只會改到副本
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例】設定檔正規化：去除鍵值前後空白並轉小寫
//   情境：讀進來的 key=value 設定常有多餘空白與大小寫不一致，
//         入庫前要統一正規化。資料量可能上萬筆。
//   為什麼用到本主題：這是「必須用 auto& 才會生效」的真實案例。
//         而且元素是 std::string —— 若誤用 auto，
//         不只結果錯誤，還會白白付出每筆一次的深複製成本。
// -----------------------------------------------------------------------------
struct ConfigEntry {
    std::string key;
    std::string value;
};

std::string trimAndLower(const std::string& s) {
    std::size_t b = s.find_first_not_of(" \t");
    if (b == std::string::npos) return "";
    std::size_t e = s.find_last_not_of(" \t");
    std::string out = s.substr(b, e - b + 1);
    for (char& c : out) {                     // 對 std::string 也能用範圍 for
        // 注意：<cctype> 的 tolower 必須先轉成 unsigned char，
        //       否則對負值的 char（非 ASCII）是未定義行為。
        //       這裡只處理 ASCII 英文字母，直接手算避免該問題。
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return out;
}

// -----------------------------------------------------------------------------
// 用來「量化」複製成本的輔助型別。
// 刻意不用計時（每次執行都不同、無法寫成穩定的預期輸出），
// 改成計算複製建構被呼叫幾次 —— 這是完全確定且可重現的證據。
// 注意：計數器放在檔案範圍，因為 C++ 的 local class 不能有 static 資料成員。
// -----------------------------------------------------------------------------
int g_copy_count = 0;

struct Tracked {
    int v = 0;
    Tracked() = default;
    Tracked(const Tracked& o) : v(o.v) { ++g_copy_count; }   // 每次複製 +1
    Tracked& operator=(const Tracked&) = default;
};

void normalizeConfig(std::vector<ConfigEntry>& entries) {
    for (auto& e : entries) {                 // ★ 必須是 auto&
        e.key   = trimAndLower(e.key);
        e.value = trimAndLower(e.value);
    }
}

int main() {
    std::vector<int> vec = {10, 20, 30, 40, 50};

    // 範圍 for（語法糖）
    std::cout << "範圍 for: ";
    for (int n : vec) {
        std::cout << n << " ";
    }
    std::cout << std::endl;

    // 等價的迭代器寫法
    std::cout << "迭代器: ";
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // 編譯器實際上把範圍 for 轉換成類似這樣：
    /*
    {
        auto&& __range = vec;
        auto __begin = __range.begin();
        auto __end = __range.end();
        for (; __begin != __end; ++__begin) {
            int n = *__begin;
            // 迴圈主體
        }
    }
    */

    // 如果要修改元素，使用參考
    std::cout << "\n修改元素（使用參考）: ";
    for (int& n : vec) {
        n *= 2;
    }
    for (int n : vec) {
        std::cout << n << " ";
    }
    std::cout << std::endl;

    // 值 vs 參考：修改是否生效
    std::cout << "\n=== 值 vs 參考：修改是否生效 ===" << std::endl;
    std::vector<int> a = {1, 2, 3};
    for (auto n : a) n *= 100;                 // 改的是副本
    std::cout << "  for (auto n : a)  之後: ";
    for (const auto& n : a) std::cout << n << " ";
    std::cout << "  ← 完全沒變" << std::endl;

    for (auto& n : a) n *= 100;                // 改的是元素本身
    std::cout << "  for (auto& n : a) 之後: ";
    for (const auto& n : a) std::cout << n << " ";
    std::cout << "  ← 生效了" << std::endl;

    // 複製成本：用「複製建構次數」量化，而非計時（可重現）
    std::cout << "\n=== 為什麼唯讀要寫 const auto&（用複製次數量化）===" << std::endl;
    std::vector<Tracked> tv(1000);

    g_copy_count = 0;
    for (auto t : tv) { (void)t.v; }           // 按值：每輪複製一次
    int by_value = g_copy_count;

    g_copy_count = 0;
    for (const auto& t : tv) { (void)t.v; }    // 按 const 參考：零複製
    int by_ref = g_copy_count;

    std::cout << "  1000 個元素，for (auto t : v)       複製次數 = " << by_value << std::endl;
    std::cout << "  1000 個元素，for (const auto& t : v) 複製次數 = " << by_ref << std::endl;
    std::cout << "  → 對 std::string 或大型結構，這就是每輪一次堆積配置的差別"
              << std::endl;

    // 自訂類別只要有 begin()/end() 就能用範圍 for
    std::cout << "\n=== 任何有 begin()/end() 的型別都能用範圍 for ===" << std::endl;
    std::string s = "STL";
    std::cout << "  std::string: ";
    for (char c : s) std::cout << c << " ";
    std::cout << std::endl;

    int arr[] = {7, 8, 9};                     // C 陣列由編譯器特別處理
    std::cout << "  C 陣列     : ";
    for (int n : arr) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "\n=== LeetCode 1480. Running Sum of 1d Array ===" << std::endl;
    std::vector<int> nums = {1, 2, 3, 4};
    std::cout << "  輸入            : 1 2 3 4" << std::endl;
    std::cout << "  版本 A（產生新的）: ";
    for (int n : runningSumCopy(nums)) std::cout << n << " ";
    std::cout << std::endl;

    std::vector<int> inplace = {1, 1, 1, 1, 1};
    runningSumInPlace(inplace);
    std::cout << "  版本 B（就地改寫）: 輸入 1 1 1 1 1 → ";
    for (int n : inplace) std::cout << n << " ";
    std::cout << "  （靠 auto& 才生效）" << std::endl;

    std::cout << "\n=== 日常實務：設定檔正規化 ===" << std::endl;
    std::vector<ConfigEntry> config = {
        {"  Timeout  ", "  30S  "},
        {"MAX_RETRIES", " 5"},
        {" Endpoint",   "HTTPS://API.EXAMPLE.COM "},
    };
    std::cout << "  正規化前:" << std::endl;
    for (const auto& e : config) {
        std::cout << "    [" << e.key << "] = [" << e.value << "]" << std::endl;
    }
    normalizeConfig(config);
    std::cout << "  正規化後:" << std::endl;
    for (const auto& e : config) {
        std::cout << "    [" << e.key << "] = [" << e.value << "]" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第四課：迭代器（Iterator）的核心概念11.cpp -o demo11

// === 預期輸出 ===
// 範圍 for: 10 20 30 40 50
// 迭代器: 10 20 30 40 50
//
// 修改元素（使用參考）: 20 40 60 80 100
//
// === 值 vs 參考：修改是否生效 ===
//   for (auto n : a)  之後: 1 2 3   ← 完全沒變
//   for (auto& n : a) 之後: 100 200 300   ← 生效了
//
// === 為什麼唯讀要寫 const auto&（用複製次數量化）===
//   1000 個元素，for (auto t : v)       複製次數 = 1000
//   1000 個元素，for (const auto& t : v) 複製次數 = 0
//   → 對 std::string 或大型結構，這就是每輪一次堆積配置的差別
//
// === 任何有 begin()/end() 的型別都能用範圍 for ===
//   std::string: S T L
//   C 陣列     : 7 8 9
//
// === LeetCode 1480. Running Sum of 1d Array ===
//   輸入            : 1 2 3 4
//   版本 A（產生新的）: 1 3 6 10
//   版本 B（就地改寫）: 輸入 1 1 1 1 1 → 1 2 3 4 5   （靠 auto& 才生效）
//
// === 日常實務：設定檔正規化 ===
//   正規化前:
//     [  Timeout  ] = [  30S  ]
//     [MAX_RETRIES] = [ 5]
//     [ Endpoint] = [HTTPS://API.EXAMPLE.COM ]
//   正規化後:
//     [timeout] = [30s]
//     [max_retries] = [5]
//     [endpoint] = [https://api.example.com]
