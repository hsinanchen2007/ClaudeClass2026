/*
================================================================================
主題:std::optional —— 可能有值,也可能沒有
標準:C++17 起
標頭:<optional>
參考:https://en.cppreference.com/w/cpp/utility/optional
================================================================================

【一、課題介紹】
  std::optional<T> 表達一個「可能存在,也可能不存在」的 T 值。
  你可以把它想成「可以為空的 T」,類似:
    - Java 的 Optional<T>
    - Rust 的 Option<T>
    - C 語言常見的「回傳 -1 代表沒找到」這種 hack 的正確替代方案

  為什麼需要它?
    傳統 C/C++ 表達「沒有值」的方式各有缺點:
      (a) 用 -1 / 空字串 / 0 等魔術值 → 容易與「真實值」混淆,且需要文件約定。
      (b) 用 nullptr 指標 → 須在 heap 配置 / 自行管理生命週期。
      (c) 用 out parameter + bool → 呼叫端寫起來醜、容易忘記檢查。
    optional 把「沒有值」當成型別系統的一等公民,呼叫端必須顯式檢查。

【二、觀念解釋】
  1. 建立:
       std::optional<int> a;            // 空(沒有值)
       std::optional<int> b = 42;       // 有值
       std::optional<int> c = std::nullopt;     // 明確表示「空」
       auto d = std::make_optional<std::string>("hi");

  2. 檢查與取值:
       if (b.has_value()) { ... }       // 或 if (b)
       int x = *b;                      // 解參考(無值時行為未定義!)
       int y = b.value();               // 無值時丟 std::bad_optional_access
       int z = b.value_or(0);           // 無值時用 0,推薦寫法

  3. 重新賦值與清空:
       b = 100;
       b.reset();                       // 變回無值
       b = std::nullopt;                // 等同 reset()

  4. 函式回傳:
       std::optional<User> findUser(int id);
     呼叫端:
       if (auto u = findUser(7)) { use(*u); }

【三、常見陷阱】
  - **直接 *opt 不檢查**是最常見錯誤,務必先 if (opt) 或用 value_or。
  - optional<T&> 不被允許(C++17 規範);若要「optional 的 reference」,
    可改用指標、std::reference_wrapper,或 C++26 才會加入的 optional<T&>。
  - optional<bool> 有「三態」:無值 / true / false,小心邏輯。
  - 不要把 optional 當成「錯誤碼」用 —— 真要表達錯誤資訊請用 expected
    (C++23)或自訂 Result 型別。

【四、與其他 utility 的比較】
  - vs nullptr 指標:optional 不需要 heap、不會懸空;指標適合「可選的多型物件」。
  - vs (bool, T) tuple:optional 語意更清楚,還能用 value_or。
  - vs std::variant<T, std::monostate>:optional 是這個 pattern 的特化版,語法更簡。

【五、Leetcode 對應題目】
  題號:704. Binary Search(二分搜尋)
  難度:Easy
  連結:https://leetcode.com/problems/binary-search/
  題目大意:在已排序陣列中找 target,有就回傳索引,沒有回傳 -1。
  選用理由:Leetcode 用 -1 表示「找不到」是經典的 magic number;
            我們改成回傳 std::optional<int>,呈現 optional 的正確用法。

【六、日常工作實用範例】
  情境:從快取 / 資料庫查使用者,可能找不到。
================================================================================
*/

/*
補充筆記：std::optional
  - optional<T> 表示「可能沒有 T」，比 magic value 如 -1 更能表達語意。
  - 讀值前用 if (opt) 或 has_value 檢查；value 在空狀態會丟 bad_optional_access。
  - optional 不保存失敗原因；需要錯誤資訊時應考慮 expected 或自訂 result 型別。
  - std::optional 屬於 utility 類工具；這些型別與函式常用來表達小型資料組合、可選值、型別安全聯集或值類別轉換。
  - pair/tuple 適合簡短聚合結果，但欄位語意複雜時應定義具名 struct，避免 first/second 或 get<0> 難讀。
  - optional 表示可能沒有值，使用前要檢查 has_value 或使用 value_or；value() 在無值時會丟例外。
  - variant 表示多選一型別，應用 visit 或 holds_alternative/get_if 安全存取目前替代項。
  - any 提供執行期任意型別保存，但取回需要知道正確型別；過度使用會失去靜態型別檢查優勢。
  - std::move/std::forward/std::exchange/as_const 都是表達意圖的工具；它們本身不一定搬移或複製資料。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::optional（C++17）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::optional 解決什麼問題？和「回傳指標」「哨兵值」比較？
//     答：表達「可能沒有值」。相較哨兵值（回傳 -1、npos）不會污染值域、API 語意明確；
//     相較回傳指標，optional 是值語意——內含物件、賦值即複製、不需堆配置、沒有生命週期
//     問題。開銷約為「內含物件 + 一個 bool 旗標」再按對齊補齊。
//     追問：optional<T&> 可以嗎？（C++17 不支援參考特化；P2988 在 C++26 才加入。C++17
//     要表達「可選的參考」請用指標，或 optional<std::reference_wrapper<T>>）
//
// 🔥 Q2. *o、o.value()、o.value_or(x) 差在哪？
//     答：o.value() 會檢查，空時拋 std::bad_optional_access；*o 與 o-> 不檢查，空時是
//     UB（可能讀到垃圾繼續跑，不保證崩潰）——這與 vector 的 at() vs operator[] 是同一組
//     設計哲學：讓已確認非空的熱路徑省下檢查。
//     追問：value_or 有什麼隱藏成本？（引數不論用不用得到都會先求值，而且回傳的是值不是
//     參考，所以 o.value_or(expensive()) 每次都會呼叫 expensive()）
//
// ⚠️ 陷阱. 對 std::optional<bool> o = false; 而言，if (o) 和 if (*o) 一樣嗎？
//     答：完全不一樣。if (o) 問的是「有沒有值」→ true；if (*o) 問的是「值本身是不是
//     true」→ false。這正是 optional<bool> 被建議避免的主因，三態語意極易誤讀。
//     為什麼會錯：optional 的 operator bool 被直覺讀成「取出裡面的 bool」，實際上它是
//     has_value() 的同義詞。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>

// ---------------------------------------------------------------------------
// 範例 1:基本建立與檢查
// ---------------------------------------------------------------------------
void demo_basic() {
    std::cout << "[demo_basic]\n";

    std::optional<int> a;                          // 空
    std::optional<int> b = 42;                     // 有值

    std::cout << "  a.has_value()=" << std::boolalpha << a.has_value() << "\n";
    std::cout << "  b.has_value()=" << b.has_value() << "\n";

    // 推薦:用 value_or 提供預設值,完全免去 if
    std::cout << "  a.value_or(-1)=" << a.value_or(-1) << "\n";
    std::cout << "  b.value_or(-1)=" << b.value_or(-1) << "\n";

    // 直接 if (opt) 也可以,等同 has_value()
    if (b) std::cout << "  b is set, *b=" << *b << "\n";
}

// ---------------------------------------------------------------------------
// 範例 2:重設與 nullopt
// ---------------------------------------------------------------------------
void demo_reset() {
    std::cout << "[demo_reset]\n";
    std::optional<std::string> name = "Alice";
    std::cout << "  before reset: " << name.value_or("(empty)") << "\n";

    name.reset();                                  // 或 name = std::nullopt;
    std::cout << "  after  reset: " << name.value_or("(empty)") << "\n";
}

// ---------------------------------------------------------------------------
// 範例 2.5:其他常用工具 —— make_optional / value / emplace / swap / 比較 / 例外
//
// 除了 has_value、value_or、reset、operator* 之外,std::optional 還有一群
// 重要工具,實務常用:
//
//   - std::make_optional(args...):類似 std::make_pair,自動推導型別建立 optional
//                                  並支援以多個參數「就地建構」內含值。
//   - opt.value():取值,空值時丟 std::bad_optional_access
//   - opt->member:當 T 是類別時,等同 (*opt).member
//   - opt.emplace(args...):以建構參數「就地」覆蓋內含值
//   - opt.swap(other) / std::swap(opt1, opt2):交換內含值
//   - 比較運算子:optional 之間或 optional 與裸值之間都可比較
//                空 optional 視為「比任何有值都小」(nullopt 為最小)
// ---------------------------------------------------------------------------
void demo_optional_helpers() {
    std::cout << "[demo_optional_helpers]\n";

    // (a) std::make_optional —— 兩種常見用法
    auto a = std::make_optional<std::string>("hello");          // 直接給值
    auto b = std::make_optional<std::vector<int>>(3, 7);        // 就地建構 vector(3,7)
    std::cout << "  make_optional<string>=*a=\"" << *a << "\"\n";
    std::cout << "  make_optional<vector>: size=" << b->size()
              << ", b->[0]=" << (*b)[0] << "\n";                // operator->

    // (b) value() vs operator*:value() 在空值時會丟例外
    std::optional<int> empty;
    try {
        (void)empty.value();                                    // 故意觸發例外
    } catch (const std::bad_optional_access& e) {
        std::cout << "  caught bad_optional_access: " << e.what() << "\n";
    }

    // (c) emplace:就地覆蓋(避免先建一個臨時物件再 move)
    std::optional<std::string> s;
    s.emplace(5, '*');                                          // 等同 std::string(5,'*')
    std::cout << "  after emplace: *s=\"" << *s << "\"\n";

    // (d) swap:成員與非成員形式
    std::optional<int> x = 1, y = 2;
    x.swap(y);
    std::cout << "  member swap: *x=" << *x << ", *y=" << *y << "\n";
    std::swap(x, y);
    std::cout << "  std::swap : *x=" << *x << ", *y=" << *y << "\n";

    // (e) 比較運算子:nullopt 永遠最小
    std::optional<int> n;                                       // 空
    std::optional<int> v = 0;
    std::cout << "  nullopt < 0? " << (n < v)                   // true
              << ", n == nullopt? " << (n == std::nullopt)      // true
              << ", v == 0? " << (v == 0) << "\n";              // true
}

// ---------------------------------------------------------------------------
// 範例 3:Leetcode #704 Binary Search 改良版
//
// 標準解法回傳 -1 代表「找不到」,但 -1 與真實索引混在同一型別 int 中,
// 呼叫端容易忘記檢查。改回傳 std::optional<int> 後,「找不到」變成
// 型別系統的一部分,呼叫端必須處理。
//
// 解題思路:
//   標準二分搜尋,左閉右閉區間 [lo, hi]:
//     - 取中點 mid
//     - 等於 target → 回傳 mid
//     - 小於 target → lo = mid + 1
//     - 大於 target → hi = mid - 1
//   區間空了就 return std::nullopt。
//
// 時間複雜度:O(log n),空間複雜度:O(1)。
// ---------------------------------------------------------------------------
std::optional<int> binarySearch(const std::vector<int>& nums, int target) {
    int lo = 0, hi = static_cast<int>(nums.size()) - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;              // 防止 lo+hi 溢位
        if (nums[mid] == target) return mid;
        if (nums[mid] < target) lo = mid + 1;
        else                    hi = mid - 1;
    }
    return std::nullopt;                           // 沒找到
}

void demo_leetcode_binary_search() {
    std::cout << "[demo_leetcode_binary_search]\n";
    std::vector<int> nums = {-1, 0, 3, 5, 9, 12};

    if (auto idx = binarySearch(nums, 9)) {
        std::cout << "  found 9 at index " << *idx << "\n";
    }
    if (auto idx = binarySearch(nums, 2)) {
        std::cout << "  found 2 at index " << *idx << "\n";
    } else {
        std::cout << "  2 not found\n";
    }
}

// ---------------------------------------------------------------------------
// 範例 4:日常工作實用範例 —— 從快取查使用者
//
// 情境:UserCache::find(id) 可能找得到也可能找不到。回傳 optional<User>
//       讓呼叫端寫起來像 Java/Rust 的 Optional,清楚明確。
// ---------------------------------------------------------------------------
struct User {
    int id;
    std::string name;
};

class UserCache {
public:
    void put(const User& u) { db_[u.id] = u; }

    std::optional<User> find(int id) const {
        auto it = db_.find(id);
        if (it == db_.end()) return std::nullopt;
        return it->second;
    }
private:
    std::unordered_map<int, User> db_;
};

void demo_practical_user_cache() {
    std::cout << "[demo_practical_user_cache]\n";
    UserCache cache;
    cache.put({1, "Alice"});
    cache.put({2, "Bob"});

    if (auto u = cache.find(1)) {
        std::cout << "  found user: id=" << u->id << ", name=" << u->name << "\n";
    }
    if (auto u = cache.find(99)) {
        std::cout << "  found user 99\n";
    } else {
        std::cout << "  user 99 not found\n";
    }

    // value_or 在「型別有預設值」時非常實用
    User fallback = cache.find(99).value_or(User{-1, "(guest)"});
    std::cout << "  fallback: id=" << fallback.id
              << ", name=" << fallback.name << "\n";
}

// ---------------------------------------------------------------------------
// 實用範例 (額外):parse_int —— 字串轉整數,可能失敗
//
// 工作中超常見:把使用者輸入字串轉成 int。傳統作法用 std::stoi 但會 throw,
// errno、邊界檢查都要自己做;用 optional 把「成功 / 失敗」變成回傳值的一部分,
// 呼叫端寫起來像 Rust 的 parse(): -> Option<i32>。
// ---------------------------------------------------------------------------
std::optional<int> parse_int(const std::string& s) {
    if (s.empty()) return std::nullopt;
    try {
        size_t pos = 0;
        int v = std::stoi(s, &pos);
        if (pos != s.size()) return std::nullopt;   // 後面還有非數字字元
        return v;
    } catch (...) {
        return std::nullopt;                         // 任何例外都當失敗
    }
}

void demo_practical_parse_int() {
    std::cout << "[demo_practical_parse_int]\n";
    for (const char* lit : {"42", "-7", "12abc", "", "9999999999999"}) {
        std::string s = lit;
        auto v = parse_int(s);
        std::cout << "  parse_int(\"" << s << "\") = ";
        if (v) std::cout << *v; else std::cout << "(failed)";
        std::cout << "\n";
    }
}

int main() {
    demo_basic();
    demo_reset();
    demo_optional_helpers();
    demo_leetcode_binary_search();
    demo_practical_user_cache();
    demo_practical_parse_int();
    return 0;
}

/*
================================================================================
編譯與執行:
    g++ -std=c++17 -Wall -Wextra 03_optional.cpp -o 03_optional && ./03_optional
================================================================================
*/
