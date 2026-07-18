/*
================================================================================
主題:std::tuple —— 元組(N 個不同型別的值)
標準:C++11 起
標頭:<tuple>
參考:https://en.cppreference.com/w/cpp/utility/tuple
================================================================================

【一、課題介紹】
  std::tuple 可以視為 std::pair 的「N 元素版」。當你想要把「3 個或更多」
  不同型別的值打包成一個物件時,就用 tuple。

  典型使用場景:
    - 函式想一次回傳「3 個值」(成功與否、資料、錯誤訊息)。
    - 暫時性的小組合,不值得另開一個 struct。
    - 與 std::tie 搭配,做多重指派。

【二、觀念解釋】
  1. 語法:
       std::tuple<T1, T2, ..., TN> t;

  2. 建立:
       std::tuple<int, std::string, double> t1(1, "hi", 3.14);
       auto t2 = std::make_tuple(1, std::string("hi"), 3.14);  // 推薦
       std::tuple t3(1, std::string("hi"), 3.14);              // C++17 CTAD

  3. 取值(注意:tuple 沒有 .first/.second,要用 std::get):
       std::get<0>(t1);   // 取第 0 個元素(編譯期常數)
       std::get<1>(t1);
       std::get<std::string>(t1);  // C++14 起,可用「型別」取(該型別需唯一)

  4. 結構化繫結(C++17,最方便):
       auto [a, b, c] = t1;

  5. std::tie(C++11):
       int a; std::string b; double c;
       std::tie(a, b, c) = t1;
     std::tie 也常用來做「多欄位字典序比較」,例如:
       return std::tie(year, month, day) < std::tie(o.year, o.month, o.day);

  6. 取得大小與型別(編譯期):
       std::tuple_size<decltype(t1)>::value     // 3
       std::tuple_element<0, decltype(t1)>::type // int

  7. 串接:std::tuple_cat(t1, t2)  把多個 tuple 接成一個。

【三、常見陷阱】
  - std::get<i>(t) 的 i 必須是「編譯期常數」,不能用變數。
  - 若 tuple 有重複型別,std::get<T>(t) 會編譯失敗(歧義)。
  - 過度濫用會傷害可讀性 —— 三個以上欄位且有明確語意,寧可寫 struct。

【四、與其他 utility 的比較】
  - vs std::pair:pair 永遠 2 個;tuple 可 0~N 個。
  - vs struct:struct 可命名欄位,大型資料模型用 struct 比較好維護。

【五、Leetcode 對應題目】
  題號:1768. Merge Strings Alternately(交替合併字串)
  難度:Easy
  連結:https://leetcode.com/problems/merge-strings-alternately/
  題目大意:給兩個字串,交替取字元組成新字串,較長那一方剩下的接在後面。
  選用理由:這裡用 tuple 同時回傳 (合併結果, word1 用了幾個字, word2 用了
            幾個字),示範一次回傳 3 個值的場景。

【六、日常工作實用範例】
  情境:HTTP 請求結果,常一次需要 (狀態碼, 主體, 錯誤訊息)。
================================================================================
*/

/*
補充筆記：std::tuple
  - std::tuple 屬於 utility 類工具；這些型別與函式常用來表達小型資料組合、可選值、型別安全聯集或值類別轉換。
  - pair/tuple 適合簡短聚合結果，但欄位語意複雜時應定義具名 struct，避免 first/second 或 get<0> 難讀。
  - optional 表示可能沒有值，使用前要檢查 has_value 或使用 value_or；value() 在無值時會丟例外。
  - variant 表示多選一型別，應用 visit 或 holds_alternative/get_if 安全存取目前替代項。
  - any 提供執行期任意型別保存，但取回需要知道正確型別；過度使用會失去靜態型別檢查優勢。
  - std::move/std::forward/std::exchange/as_const 都是表達意圖的工具；它們本身不一定搬移或複製資料。
  - tuple 可保存多個不同型別，但 get<0> 這類位置式存取會降低可讀性。
  - structured binding 能改善 tuple 使用體驗，但欄位名稱仍只在 binding 當下存在。
  - tuple 適合泛型工具和短距離回傳；若資料會跨模組傳遞，具名型別通常更好。
*/
#include <iostream>
#include <tuple>
#include <string>
#include <type_traits>

// ---------------------------------------------------------------------------
// 範例 1:基本建立、取值
// ---------------------------------------------------------------------------
void demo_basic() {
    std::cout << "[demo_basic]\n";

    auto t = std::make_tuple(42, std::string("hello"), 3.14);

    // 用 std::get<index> 取元素
    std::cout << "  get<0>=" << std::get<0>(t) << "\n";
    std::cout << "  get<1>=" << std::get<1>(t) << "\n";
    std::cout << "  get<2>=" << std::get<2>(t) << "\n";

    // C++14 起也可用「型別」取(此處型別都唯一,合法)
    std::cout << "  get<std::string>=" << std::get<std::string>(t) << "\n";
}

// ---------------------------------------------------------------------------
// 範例 2:結構化繫結拆解 tuple(C++17)
// ---------------------------------------------------------------------------
std::tuple<int, std::string, double> getStudent() {
    return std::make_tuple(1001, std::string("Alice"), 92.5);
}

void demo_structured_binding() {
    std::cout << "[demo_structured_binding]\n";

    auto [id, name, score] = getStudent();
    std::cout << "  id=" << id << ", name=" << name << ", score=" << score << "\n";
}

// ---------------------------------------------------------------------------
// 範例 3:std::tie 用法 —— 多欄位字典序比較
//
// 想法:Date 想要先比 year、再比 month、再比 day。
//       手寫 if 嵌套很冗長,用 std::tie 一行解決。
// ---------------------------------------------------------------------------
struct Date {
    int year, month, day;
    bool operator<(const Date& o) const {
        return std::tie(year, month, day) < std::tie(o.year, o.month, o.day);
    }
};

void demo_tie_compare() {
    std::cout << "[demo_tie_compare]\n";
    Date a{2025, 1, 15};
    Date b{2025, 1, 16};
    std::cout << "  a < b ? " << std::boolalpha << (a < b) << "\n";
}

// ---------------------------------------------------------------------------
// 範例 3.5:tuple 的其他常用工具
//
// std::tuple 除了 make_tuple、std::get、std::tie 之外,還有一群「家族」工具:
//
//   - std::forward_as_tuple(args...)
//       把實參「以原本的值類別(lvalue/rvalue)」打包成 tuple,
//       常用於把多個建構參數轉發給其他函式,例如 std::pair 的
//       piecewise_construct 介面。
//
//   - std::tuple_cat(t1, t2, ...)
//       把多個 tuple「串接」成一個更大的 tuple,連結時保留型別。
//
//   - std::tuple_size<T>::value、std::tuple_element<I, T>::type
//       編譯期取得 tuple 的大小與第 I 個元素的型別。
//
//   - std::ignore
//       搭配 std::tie 使用,當你只想取部分欄位、忽略其他欄位時很好用。
//
//   - 比較運算子:tuple 支援 ==、!=、<、<=、>、>=,皆為「字典序」。
//
//   - swap:成員 t.swap(o) 與非成員 std::swap(t, o) 都可以。
// ---------------------------------------------------------------------------
void demo_tuple_helpers() {
    std::cout << "[demo_tuple_helpers]\n";

    // (a) tuple_cat:把兩個 tuple 串成一個
    auto t1 = std::make_tuple(1, std::string("a"));
    auto t2 = std::make_tuple(3.14, 'X');
    auto t3 = std::tuple_cat(t1, t2);   // (int, string, double, char)
    std::cout << "  cat=(" << std::get<0>(t3) << ", " << std::get<1>(t3)
              << ", " << std::get<2>(t3) << ", " << std::get<3>(t3) << ")\n";

    // (b) tuple_size / tuple_element:編譯期反射
    std::cout << "  size of t3 = " << std::tuple_size<decltype(t3)>::value << "\n";
    using ThirdT = std::tuple_element<2, decltype(t3)>::type;
    static_assert(std::is_same<ThirdT, double>::value, "第 2 個元素應為 double");

    // (c) std::ignore:用 tie 解構時忽略不需要的欄位
    int id = 0; double score = 0;
    std::tie(id, std::ignore, score) =
        std::make_tuple(7, std::string("ignored"), 88.5);
    std::cout << "  via tie+ignore: id=" << id << ", score=" << score << "\n";

    // (d) 比較運算子:字典序比較
    auto a = std::make_tuple(1, 2, 3);
    auto b = std::make_tuple(1, 2, 4);
    std::cout << "  a==b: " << (a == b)
              << ", a<b: " << (a < b)
              << ", a!=b: " << (a != b) << "\n";

    // (e) swap:成員與非成員兩種寫法
    auto x = std::make_tuple(1, std::string("x"));
    auto y = std::make_tuple(2, std::string("y"));
    x.swap(y);
    std::cout << "  member swap: x=(" << std::get<0>(x) << "," << std::get<1>(x) << ")\n";
    std::swap(x, y);
    std::cout << "  std::swap : x=(" << std::get<0>(x) << "," << std::get<1>(x) << ")\n";

    // (f) forward_as_tuple:示意 —— 把參數「以原本值類別」打包
    //     這裡 (lvalue id, rvalue 字面值) 會被打包成
    //     std::tuple<int&, const char(&)[6]> 之類的型別。
    int local_id = 42;
    auto fwd = std::forward_as_tuple(local_id, "hello");
    static_assert(std::is_same<decltype(std::get<0>(fwd)), int&>::value,
                  "forward_as_tuple 對 lvalue 應產生 lvalue 參考");
    std::cout << "  forward_as_tuple: get<0>=" << std::get<0>(fwd) << "\n";
}

// ---------------------------------------------------------------------------
// 範例 4:Leetcode #1768 Merge Strings Alternately
//
// 解題思路:
//   1. 用兩個索引 i、j 同時走 word1、word2,每輪各取一個字元加到結果。
//   2. 短的那一方先走完,把長的剩餘部分整段接到結果尾端。
//
// 為什麼用 tuple?
//   一般 Leetcode 解法只回傳合併字串,但實務上我們常想一併知道
//   「兩邊各用了多少字元」(例如要做 log)。這裡用 tuple 一次回傳三個值。
//
// 時間複雜度:O(n + m),空間複雜度:O(n + m)。
// ---------------------------------------------------------------------------
std::tuple<std::string, int, int>
mergeAlternately(const std::string& w1, const std::string& w2) {
    std::string out;
    out.reserve(w1.size() + w2.size());
    int i = 0, j = 0;
    while (i < static_cast<int>(w1.size()) && j < static_cast<int>(w2.size())) {
        out.push_back(w1[i++]);
        out.push_back(w2[j++]);
    }
    while (i < static_cast<int>(w1.size())) out.push_back(w1[i++]);
    while (j < static_cast<int>(w2.size())) out.push_back(w2[j++]);
    return std::make_tuple(out, i, j);
}

void demo_leetcode_merge() {
    std::cout << "[demo_leetcode_merge]\n";
    auto [merged, used1, used2] = mergeAlternately("abc", "pqrst");
    std::cout << "  merged=" << merged
              << ", used1=" << used1 << ", used2=" << used2 << "\n";
}

// ---------------------------------------------------------------------------
// 範例 5:日常工作實用範例 —— HTTP 風格回傳值
//
// 情境:模擬一個 callApi() 函式,一次回傳:
//        (HTTP status code, response body, 錯誤訊息)
//      呼叫端用結構化繫結拆開,程式很乾淨。
//
// (注意:真實情況更建議用 struct;這裡是為了示範 tuple 的用法。)
// ---------------------------------------------------------------------------
std::tuple<int, std::string, std::string> callApi(const std::string& url) {
    if (url.empty()) {
        return {400, "", "url is empty"};          // C++17 大括號回傳
    }
    return {200, "{\"ok\":true}", ""};
}

void demo_practical_http() {
    std::cout << "[demo_practical_http]\n";
    auto [status, body, err] = callApi("https://api.example.com/ping");
    std::cout << "  status=" << status
              << ", body=" << body
              << ", err=[" << err << "]\n";

    auto [s2, b2, e2] = callApi("");
    std::cout << "  status=" << s2
              << ", body=[" << b2 << "]"
              << ", err=" << e2 << "\n";
}

// ---------------------------------------------------------------------------
// 主程式
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// 實用範例 (額外):std::apply —— 把 tuple「攤開」成函式參數
//
// 工作中常見:把多個參數先打包成 tuple 後傳遞 (例如延遲呼叫),
// 真正執行時用 std::apply 一次「展開」成參數送進函式,免寫一堆 std::get<i>(t)。
// ---------------------------------------------------------------------------
#include <functional>

static int sum3(int a, int b, int c) { return a + b + c; }

void demo_practical_apply() {
    std::cout << "[demo_practical_apply]\n";
    auto args = std::make_tuple(1, 2, 3);
    int r = std::apply(sum3, args);                 // 等同 sum3(1, 2, 3)
    std::cout << "  std::apply(sum3, (1,2,3)) = " << r << "\n";

    // 也常見和 lambda 配合:
    auto fmt = [](int id, const std::string& name, double s) {
        std::cout << "  fmt: id=" << id << ", name=" << name << ", s=" << s << "\n";
    };
    std::apply(fmt, std::make_tuple(7, std::string("Bob"), 88.5));
}

int main() {
    demo_basic();
    demo_structured_binding();
    demo_tie_compare();
    demo_tuple_helpers();
    demo_leetcode_merge();
    demo_practical_http();
    demo_practical_apply();
    return 0;
}

/*
================================================================================
編譯與執行:
    g++ -std=c++17 -Wall -Wextra 02_tuple.cpp -o 02_tuple && ./02_tuple
================================================================================
*/
