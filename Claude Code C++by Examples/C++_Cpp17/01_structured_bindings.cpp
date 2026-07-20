// =============================================================================
//  01_structured_bindings.cpp  —  Structured bindings (C++17)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/structured_binding
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、語法                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//      auto [a, b] = expr;
//      auto& [a, b] = lvalue_expr;        // 取參考
//      const auto& [a, b] = expr;          // const ref
//
//  expr 必須是：
//   * std::pair / std::tuple / std::array
//   * 普通 struct（aggregate） — 按宣告順序綁
//   * 自訂用 std::tuple_size / get<> 模型「裝得像 tuple」的型別
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、最有用場景                                             │
//  └────────────────────────────────────────────────────────────┘
//
//   1) 取多回傳值（std::pair / std::tuple）
//        auto [it, inserted] = m.insert({key, val});
//        auto [min, max] = std::minmax({1, 5, 3, 2});
//
//   2) 走訪 std::map — key/value 一行解開
//        for (auto& [k, v] : m) std::cout << k << '=' << v << '\n';
//
//   3) 把 struct 拆成名字
//        struct Point { int x, y; };
//        Point p{3, 4};
//        auto& [x, y] = p;        // x, y 是 p.x / p.y 的 ref
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、跟「pair 解開」舊寫法對比                              │
//  └────────────────────────────────────────────────────────────┘
//
//  C++14 舊寫法：
//      std::pair<int, std::string> p = make();
//      int n = p.first;
//      std::string s = p.second;
//
//  C++17：
//      auto [n, s] = make();      // 一行
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：拆 std::pair / std::tuple
//   * Demo 2：拆 struct
//   * Demo 3：走 map — for ([k, v] : m)
//   * LeetCode 1207. Unique Number of Occurrences
// =============================================================================

/*
補充筆記：structured_bindings
  - structured_bindings 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - structured binding 會把 pair、tuple、array 或有對應介面的 struct 拆成多個名字。
  - auto [a,b] 預設可能複製元素；要修改原資料應寫 auto& [a,b]。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】structured bindings
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. structured bindings 到底綁定到什麼？
//     答：編譯器先產生一個隱藏的未命名變數 e，auto / auto& / const auto& 這些
//         修飾其實是套在 e 上，不是套在 a、b 上。
//         a、b 本身不是獨立變數，而是「指涉 e 之子物件」的名字。
//     追問：auto [a,b] = p; 會不會複製？（會，e 是 p 的副本）
//
// 🔥 Q2. structured bindings 支援哪三種情況？
//     答：① 原生陣列：逐元素綁定，數量須完全相符。
//         ② tuple-like：型別特化了 std::tuple_size<E>，則以 std::tuple_element<i,E>
//            決定型別，取值走成員 e.get<i>() 或 ADL 的 get<i>(e)。
//         ③ 所有 non-static data member 皆為 public 且同屬一個 class。
//     追問：只有 private 成員的 class 可以嗎？
//           （不行，除非自行特化 tuple_size / tuple_element / get）
//
// ⚠️ 陷阱1. auto [a,b] = std::pair<int,double>{}; 之後 decltype(a) 是參考嗎？
//     答：不是。decltype(a) 給的是「被指涉的型別」，tuple-like 情形等於
//         std::tuple_element<0,E>::type，所以是 int 而非 int&。
//     為什麼會錯：多數人知道底層一定引入了隱藏參考，就以為 decltype 會反映出來；
//         但 a 不是變數而是名字，decltype 對它有特殊規則。
//
// ⚠️ 陷阱2. structured binding 的名字可以被 lambda 捕獲嗎？
//     答：C++17 不行（它們不是變數，標準明文禁止捕獲）；C++20 起放寬，允許捕獲。
//         C++17 的變通：先 auto x = a; 複製到真正的變數，再捕獲 x。
//     為什麼會錯：它們用起來就像一般區域變數，於是同一段程式 C++17 編不過、
//         C++20 卻能過，是典型的版本陷阱。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <map>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct Point { int x; int y; };

static std::tuple<int, double, std::string> getInfo() {
    return {42, 3.14, "answer"};
}

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：tuple / pair 解開
    // ─────────────────────────────────────────────────────────
    auto [n, d, s] = getInfo();
    std::cout << "[Demo1] n=" << n << " d=" << d << " s=" << s << '\n';

    auto pr = std::make_pair(1, "hello");
    auto [first, second] = pr;
    std::cout << "[Demo1] pair: " << first << ", " << second << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：拆 struct
    // ─────────────────────────────────────────────────────────
    Point p{10, 20};
    auto& [px, py] = p;
    px = 100;        // 影響 p.x
    std::cout << "[Demo2] p = (" << p.x << ", " << p.y << ")\n";
    (void)py;

    // ─────────────────────────────────────────────────────────
    // Demo 3：走 map
    // ─────────────────────────────────────────────────────────
    std::map<std::string, int> ages{{"alice", 30}, {"bob", 25}};
    for (auto& [name, age] : ages) {
        std::cout << "[Demo3] " << name << " is " << age << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // LeetCode 1207. Unique Number of Occurrences
    //   題意：給一個整數陣列，判斷「每個元素的出現次數」是否互不相同。
    //   解法：unordered_map 統計頻率 → 把頻率丟進 unordered_set；
    //         set 大小若等於 map 大小 → 全部不同，true。
    //   為何放這？示範 structured binding 在「走頻率表」時的乾淨寫法。
    // ─────────────────────────────────────────────────────────
    auto uniqueOccurrences = [](const std::vector<int>& arr) {
        std::unordered_map<int, int> freq;
        for (int x : arr) ++freq[x];
        std::unordered_set<int> seen;
        for (const auto& [val, cnt] : freq) {
            (void)val;                       // 我們只在乎 cnt
            if (!seen.insert(cnt).second) return false;
        }
        return true;
    };
    std::cout << std::boolalpha;
    std::cout << "[LC1207] {1,2,2,1,1,3} => "
              << uniqueOccurrences({1,2,2,1,1,3}) << '\n';   // true
    std::cout << "[LC1207] {1,2}        => "
              << uniqueOccurrences({1,2}) << '\n';            // false

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：拆出來的元素能改原物件嗎？
    //    A：要看你寫 auto& 還是 auto。auto& 拆出 ref，改了 = 改原物件；
    //       auto 是拷貝。例：
    //         auto& [k, v] : m  → 改 v 等於改 map 的值
    //         auto  [k, v] : m  → 改 v 沒影響
    //
    //  Q2：能跳過某個欄位嗎？
    //    A：C++17 不行，名字必須全列出（不能用 _、不能省略）。實務上常寫
    //       (void) 註解忽略。C++26 可能有 `_` 但目前還是要全列。
    //
    //  Q3：自訂 class 要怎麼支援？
    //    A：(a) plain struct（aggregate）自動支援
    //       (b) class with private data 要實作 std::tuple_size、tuple_element
    //           特化與 get<I>() — 較麻煩但可行
    //
    // ─────────────────────────────────────────────────────────
    // LeetCode 1. Two Sum — 用 structured binding 拆 map iterator
    //   題意：給 nums 與 target，找兩個 index 使 nums[i] + nums[j] = target。
    //   為什麼放這？emplace / insert 回傳 pair<iterator, bool>，
    //                structured binding 一行就拆乾淨。
    // ─────────────────────────────────────────────────────────
    auto twoSum = [](const std::vector<int>& nums, int target) {
        std::unordered_map<int, int> seen;            // value -> index
        for (int i = 0; i < static_cast<int>(nums.size()); ++i) {
            int need = target - nums[i];
            if (auto it = seen.find(need); it != seen.end()) {
                auto [val, idx] = *it;                  // 拆 key/value
                (void)val;
                return std::vector<int>{idx, i};
            }
            seen.emplace(nums[i], i);
        }
        return std::vector<int>{};
    };
    auto two = twoSum({2, 7, 11, 15}, 9);
    std::cout << "[LC1] indices:";
    for (int x : two) std::cout << ' ' << x;
    std::cout << '\n';

    // ─────────────────────────────────────────────────────────
    // 實用範例：解析「key=value」設定列表 — 用 pair 回傳並 structured bind
    //   工作上常見：parse 設定行 / HTTP header
    // ─────────────────────────────────────────────────────────
    auto parseKV = [](const std::string& line) -> std::pair<std::string, std::string> {
        auto eq = line.find('=');
        if (eq == std::string::npos) return {line, ""};
        return {line.substr(0, eq), line.substr(eq + 1)};
    };
    std::vector<std::string> lines{"host=localhost", "port=8080", "ssl=true"};
    for (const auto& line : lines) {
        auto [key, value] = parseKV(line);              // 一行解開兩個欄位
        std::cout << "[Demo4] " << key << " => " << value << '\n';
    }

    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 01_structured_bindings.cpp -o 01_structured_bindings

// === 預期輸出 ===
// [Demo1] n=42 d=3.14 s=answer
// [Demo1] pair: 1, hello
// [Demo2] p = (100, 20)
// [Demo3] alice is 30
// [Demo3] bob is 25
// [LC1207] {1,2,2,1,1,3} => true
// [LC1207] {1,2}        => false
// [LC1] indices: 0 1
// [Demo4] host => localhost
// [Demo4] port => 8080
// [Demo4] ssl => true
