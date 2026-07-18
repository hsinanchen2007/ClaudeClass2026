// ============================================================================
//  11_alias_template.cpp  ──  Alias Template (C++11)
// ============================================================================
//
//  【本篇目標】
//    Alias template = 「typedef 的 template 升級版」。讓你給「型別參數化的
//    別名」一個簡短名字，提升可讀性。
//
//  【為什麼需要】
//    C++03 typedef 不能 template：
//        typedef std::vector<int, MyAlloc<int>> IntVec;       // 只能對單一型別
//        // 想做 template 化的別名：辦不到。
//        // template<typename T> typedef std::vector<T, MyAlloc<T>> Vec<T>;  // 不合法
//
//    C++11 引入 using 別名語法，可以 template 化：
//        template<typename T>
//        using Vec = std::vector<T, MyAlloc<T>>;
//
//        Vec<int>  a;   // 等同 std::vector<int, MyAlloc<int>>
//        Vec<long> b;
//
//  【using vs typedef】
//    對「沒有 template 參數」的別名：兩者等價，但建議統一用 using，理由：
//      - 語法更直觀：using Foo = Bar; 比 typedef Bar Foo; 自然。
//      - 「函式指標」型別差別大：
//            typedef int (*Func)(int, int);              // 看半天才看懂
//            using Func = int(*)(int, int);              // 一眼看出
//
//  【常見實用案例】
//    1. 簡化巢狀型別：
//          template<typename T> using Vec      = std::vector<T>;
//          template<typename K, typename V> using HashMap = std::unordered_map<K, V>;
//
//    2. 修飾 trait 型別 (C++14 起標準庫廣泛使用 _t 後綴)：
//          template<typename T>
//          using remove_reference_t = typename std::remove_reference<T>::type;
//
//    3. 包裝 Result / Optional：
//          template<typename T> using Result = std::variant<T, ErrorCode>;
//
//  參考：https://en.cppreference.com/cpp/language/type_alias
// ============================================================================

/*
補充筆記：alias_template
  - alias_template 涉及模板實例化；請先判斷哪些型別由呼叫端推導，哪些型別由程式指定。
  - 泛型程式要把型別需求寫清楚，例如可比較、可移動、可呼叫或符合某個 concept。
  - 模板技巧的價值在減少重複且保留型別安全，不是讓錯誤訊息變得更長。
  - alias_template 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/
#include <algorithm>
#include <iostream>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <string>

// ─── 1. 基本 alias template：簡化容器寫法 ────────────────────────────────
template <typename T>
using Vec = std::vector<T>;

template <typename K, typename V>
using HashMap = std::unordered_map<K, V>;

// ─── 2. trait alias：對應 C++14 _t 後綴的標準庫風格 ──────────────────────
//   省掉 typename ... ::type 這個冗長語法。
template <typename T>
using remove_const_t = typename std::remove_const<T>::type;

template <typename T>
using remove_ref_t = typename std::remove_reference<T>::type;

// ─── 3. Result alias：用 union/variant 表達「成功或錯誤」 ────────────────
//   實務專案常見的「Either / Result」模式 (Rust / Haskell 風)。
//   這裡用 std::pair<T, std::string> 簡化示範 (string 為錯誤訊息)：
//        first = 結果 (錯誤時為預設值)
//        second = 錯誤訊息 (空 = 成功)
template <typename T>
using Result = std::pair<T, std::string>;

template <typename T>
Result<T> ok(T v) { return { v, {} }; }

template <typename T>
Result<T> err(const std::string& msg) { return { T{}, msg }; }

template <typename T>
bool is_ok(const Result<T>& r) { return r.second.empty(); }

// ─── 4. Leetcode 49 ── Group Anagrams (用 HashMap alias 簡化) ─────────────
//   題目：給字串陣列，把「字母組合相同」的字串分組。
//   範例：["eat","tea","tan","ate","nat","bat"]
//        → [["bat"],["nat","tan"],["ate","eat","tea"]]
//
//   經典解法：把每個字串排序當 key，原字串當 value。
//
//   時間：O(n · k log k)，n = 字串數、k = 平均字串長度
//   空間：O(n · k)
//   邊界：strs 為空 → 回傳空。
//
//   為什麼放這裡？
//     原本要寫 std::unordered_map<std::string, std::vector<std::string>>
//     很長。用 HashMap 別名後讀起來舒服多。
std::vector<std::vector<std::string>>
group_anagrams(const std::vector<std::string>& strs) {
    HashMap<std::string, std::vector<std::string>> bucket;
    for (const std::string& s : strs) {
        std::string key = s;
        std::sort(key.begin(), key.end());          // 排序後當 key
        bucket[key].push_back(s);
    }
    std::vector<std::vector<std::string>> out;
    out.reserve(bucket.size());
    for (auto& kv : bucket) out.push_back(std::move(kv.second));
    return out;
}

// ─── 5. 工作實用範例：函式指標 alias ────────────────────────────────────
//   日常工作會用到的 callback 簽名 alias，比 typedef 寫法直觀。
template <typename R, typename... Args>
using Func = R (*)(Args...);

// 一個範例 callback
int multiply(int a, int b) { return a * b; }

void run_with_cb(int x, int y, Func<int, int, int> cb) {
    std::cout << "cb(" << x << ", " << y << ") = " << cb(x, y) << "\n";
}

// ─── 6. Leetcode 1773 ── Count Items Matching a Rule ─────────────────────
//   難度: easy
//   題目：items[i] = [type, color, name]；給 ruleKey, ruleValue。
//        回傳「對應欄位 == ruleValue」的元素個數。
//   範例：items=[["phone","blue","pixel"],...], ruleKey="color", ruleValue="silver"
//
//   為什麼放在這裡？
//     用 alias template 簡化「三元組」型別 (StringTriple) 與「字串容器」
//     (Strings)，讓 API 簽名比寫 vector<vector<string>> 短得多。
template <typename T>
using Triple = std::vector<T>;          // 簡化：只是三元組

template <typename T>
using Items = std::vector<Triple<T>>;

int count_matches(const Items<std::string>& items,
                  const std::string& ruleKey,
                  const std::string& ruleValue) {
    int idx = ruleKey == "type" ? 0 : ruleKey == "color" ? 1 : 2;
    int count = 0;
    for (const auto& it : items) if (it[idx] == ruleValue) ++count;
    return count;
}

// ─── 7. 工作實用：Callback / EventHandler 別名 ───────────────────────────
//   常見場景：UI 事件、async 回呼。用 alias template 把長簽名收成短名稱。
#include <functional>
template <typename T>
using Callback = std::function<void(const T&)>;

template <typename T>
void notify_all(const std::vector<Callback<T>>& subs, const T& event) {
    for (const auto& cb : subs) cb(event);
}

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    // (1) Vec / HashMap
    Vec<int> v{1, 2, 3};
    std::cout << "Vec<int> size = " << v.size() << "\n";

    HashMap<std::string, int> ages = {{"Alice", 30}, {"Bob", 25}};
    std::cout << "Alice age = " << ages["Alice"] << "\n";

    // (2) trait alias 用法
    using A = const int;
    using B = remove_const_t<A>;          // B = int
    std::cout << std::boolalpha;
    std::cout << "is same int / B = " << std::is_same<int, B>::value << "\n";

    // (3) Result
    auto r1 = ok<int>(42);
    auto r2 = err<int>("disk full");
    std::cout << "r1 ok? " << is_ok(r1) << " value=" << r1.first
              << " err=\"" << r1.second << "\"\n";
    std::cout << "r2 ok? " << is_ok(r2) << " value=" << r2.first
              << " err=\"" << r2.second << "\"\n";

    // (4) Leetcode 49
    auto groups = group_anagrams({"eat","tea","tan","ate","nat","bat"});
    for (const auto& g : groups) {
        std::cout << "[ ";
        for (const auto& s : g) std::cout << s << ' ';
        std::cout << "]\n";
    }

    // (5) 函式指標 alias
    run_with_cb(3, 5, multiply);

    // (6) Leetcode 1773 Count Matches
    Items<std::string> items{
        {"phone","blue","pixel"},
        {"computer","silver","lenovo"},
        {"phone","gold","iphone"},
    };
    std::cout << "count_matches(color=silver) = "
              << count_matches(items, "color", "silver") << " (expect 1)\n";

    // (7) Callback alias
    std::vector<Callback<std::string>> subs;
    subs.push_back([](const std::string& m) { std::cout << "subA: " << m << "\n"; });
    subs.push_back([](const std::string& m) { std::cout << "subB: " << m << "\n"; });
    notify_all<std::string>(subs, "hello");

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼 alias template 不能被「特化」(specialize)？
    //    A：標準明文禁止 alias template 寫 specialization 或 partial
    //       specialization。原因是 alias 在語意上被視為「透明」(transparent)，
    //       它只是給某個型別表達式取個短名，編譯器看到 `Vec<int>` 就立
    //       刻替換為 `std::vector<int>`，不會在「實例化」階段做型別選
    //       擇。需要分流時，請改用 class template + partial specialization
    //       (例如 std::remove_reference 內部用 class)，再用 alias 包成
    //       _t 介面 (如 remove_reference_t)。
    //
    //  Q2：`using` alias 跟 C 的 `typedef` 在 template 場景差在哪？
    //    A：typedef 在 C++03 時代不能 template 化，無法寫
    //       `template<typename T> typedef std::vector<T> Vec;`。C++11 引
    //       入的 `using` 別名可以 template 化、語法更直觀 (左 = 右，跟
    //       變數宣告一致)，連函式指標型別都比 typedef 易讀。對非 template
    //       場景兩者等價，但建議統一用 using 以保持風格一致。
    //
    //  Q3：標準庫 C++14 起加入的 `_t` 後綴 (remove_reference_t、enable_if_t)
    //      到底解決了什麼痛點？
    //    A：解決 dependent name 必須加 typename 的冗詞問題。舊寫法是
    //       `typename std::remove_reference<T>::type`，又長又容易忘記
    //       typename 而編譯失敗。`std::remove_reference_t<T>` 用 alias
    //       template 包起來：`using remove_reference_t = typename
    //       remove_reference<T>::type;`，呼叫端不必再寫 typename，metaprogramming
    //       程式碼瞬間清爽。C++17 又再加 `_v` 後綴給 trait 的 ::value，
    //       完整補齊兩種冗詞。
    //
    return 0;
}

// ============================================================================
//  【小結】
//    1. Alias template 用 using 寫，可以 template 化，是 typedef 的升級版。
//    2. 標準庫廣泛使用 _t 後綴的 alias (e.g. remove_const_t)，省去
//       「typename …::type」冗詞。
//    3. 對函式指標、std::function、複雜泛型，alias 是可讀性救星。
//
//  【下一篇】
//    12_variable_template.cpp ── Variable template (C++14)。
// ============================================================================
