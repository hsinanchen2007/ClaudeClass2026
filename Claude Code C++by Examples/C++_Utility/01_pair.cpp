/*
================================================================================
主題:std::pair —— 配對(Pair)
標準:C++98 起
標頭:<utility>
參考:https://en.cppreference.com/w/cpp/utility/pair
================================================================================

【一、課題介紹】
  std::pair 是 C++ 標準函式庫中最基本的「組合型別(class template)」,
  用來把「兩個可以是不同型別」的值,打包成「一個」物件。

  為什麼需要它?
    在實務上,我們常常希望函式回傳「兩個值」,例如:
      - 一筆資料的 (索引, 內容)
      - 一筆查詢的 (是否成功, 結果)
      - 一個座標 (x, y)
    傳統 C 語言的做法是回傳一個 struct 或透過 out parameter (指標/參考);
    pair 則提供「不需要為這種小組合特別定義 struct」的便捷方式。

  std::map / std::unordered_map 內部就是用 std::pair<const Key, Value>
  來表示一筆 key-value,所以遍歷 map 時你看到的 it->first / it->second
  就是 pair 的成員。

【二、觀念解釋】
  1. 語法:
       std::pair<T1, T2> p;
     T1、T2 可以是任意型別(int、string、自訂類別、甚至另一個 pair 都行)。

  2. 公開成員(就只有兩個!):
       p.first   —— 第一個元素,型別為 T1
       p.second  —— 第二個元素,型別為 T2

  3. 建立方式(由舊到新):
       (a) 預設建構:        std::pair<int, std::string> p;        // (0, "")
       (b) 直接給值:        std::pair<int, std::string> p(1,"a"); // (1, "a")
       (c) std::make_pair:  auto p = std::make_pair(1, "a");      // C++98
       (d) 大括號初始化:    std::pair<int,std::string> p{1,"a"}; // C++11
       (e) CTAD 類別樣板實參推導:std::pair p(1, "a");            // C++17

  4. 比較運算子(<、==、!= 等)是「字典序比較」:
       先比 first,first 相等才比 second。

  5. 結構化繫結(C++17):
       auto [k, v] = std::make_pair(1, "hi");
     直接把 first / second 拆給 k / v,寫起來像 Python 的 tuple unpack。

  6. std::tie(C++11):
       int k; std::string v;
       std::tie(k, v) = std::make_pair(1, std::string("hi"));
     C++17 之前沒有結構化繫結時的常見替代寫法。

【三、常見陷阱】
  - 不要對 pair 的元素數量有錯誤期待:它「永遠是 2 個」,要更多請用 tuple。
  - std::make_pair 會自動「衰退」陣列為指標、函式為函式指標,
    所以 make_pair("hi", 1) 推導出來的 first 是 const char*,而不是 char[3]。
  - pair 沒有 .x / .y 這種有意義的命名;若可讀性重要,寧可寫 struct。

【四、與其他 utility 的比較】
  - vs std::tuple:tuple 可裝 N 個元素;pair 固定 2 個。
  - vs struct:struct 可命名欄位(.name / .age),程式更易讀。
  - vs std::array:array 是「同型別」N 個;pair 是「不同型別」2 個。

【五、Leetcode 對應題目】
  題號:1. Two Sum(兩數之和)
  難度:Easy
  連結:https://leetcode.com/problems/two-sum/
  題目大意:給一個陣列 nums 與目標值 target,找出兩個元素相加等於 target,
            回傳這兩個元素的「索引」。
  選用理由:回傳「一對索引」,正好用 std::pair<int,int> 表達。

【六、日常工作實用範例】
  情境:設定檔常見格式 "key=value",寫一個小函式把它拆成 (key, value)。
        例如 ".env" 檔、INI 檔的單行解析就是這個模式。
================================================================================
*/

/*
補充筆記：std::pair
  - std::pair 屬於 utility 類工具；這些型別與函式常用來表達小型資料組合、可選值、型別安全聯集或值類別轉換。
  - pair/tuple 適合簡短聚合結果，但欄位語意複雜時應定義具名 struct，避免 first/second 或 get<0> 難讀。
  - optional 表示可能沒有值，使用前要檢查 has_value 或使用 value_or；value() 在無值時會丟例外。
  - variant 表示多選一型別，應用 visit 或 holds_alternative/get_if 安全存取目前替代項。
  - any 提供執行期任意型別保存，但取回需要知道正確型別；過度使用會失去靜態型別檢查優勢。
  - std::move/std::forward/std::exchange/as_const 都是表達意圖的工具；它們本身不一定搬移或複製資料。
  - pair 適合兩個值語意很明顯的結果，例如 iterator/bool 或 key/value；語意複雜時應改成具名 struct。
  - std::tie 可用來拆 pair 或做字典序比較，但 structured binding 通常更易讀。
  - pair 的比較是先比 first，再比 second；這可用於排序，但要確認符合需求。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::pair
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::pair 和 std::tuple 的差別？什麼時候兩個都不該用？
//     答：pair 固定兩個元素（.first / .second），tuple 可以任意個（std::get<I>，C++14 起
//     也支援 std::get<T>）。但兩者的成員都沒有語意——當這組值代表一個有名字的概念時，
//     具名 struct 幾乎總是更好的選擇。C++17 起搭配 structured bindings，
//     auto [k, v] = *it; 讓可讀性大幅改善。
//     追問：std::tie 有什麼用？（① 拆解到既有變數：std::tie(a, b) = f(); ② 用
//     std::ignore 跳過某項 ③ 一行實作字典序比較：
//     return std::tie(a, b, c) < std::tie(o.a, o.b, o.c);）
//
// 🔥 Q2. make_pair 在 C++17 之後還需要嗎？
//     答：大致上不需要了，CTAD 讓 std::pair p{1, 2.0}; 直接推出 pair<int, double>。
//     但兩者不完全等價：make_pair 會對引數做 decay，CTAD 在某些情境推導結果不同，
//     所以不能無腦替換。
//
// ⚠️ 陷阱. std::pair p{"a", "b"}; 推導出來是什麼型別？
//     答：pair<const char*, const char*>，不是 pair<std::string, std::string>。若把它
//     存起來而字面量來源不再有效（例如來自暫存的 std::string 的 c_str()），就會懸垂。
//     修法：std::pair p{"a"s, "b"s};（using namespace std::string_literals）或明寫型別。
//     為什麼會錯：以為 CTAD 會「推導出你想要的型別」，實際上它只忠實反映引數的型別，
//     而字串字面量的型別就是 const char[N] → 衰退成 const char*。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <iostream>
#include <utility>     // std::pair, std::make_pair, std::get, std::piecewise_construct
#include <string>
#include <vector>
#include <unordered_map>
#include <tuple>       // std::tuple_size, std::tuple_element, std::forward_as_tuple

// ---------------------------------------------------------------------------
// 範例 1:最基本用法 —— 建立 pair、讀寫 first / second
// ---------------------------------------------------------------------------
void demo_basic() {
    std::cout << "[demo_basic]\n";

    // (a) 直接建構
    std::pair<int, std::string> p1(1, "apple");

    // (b) make_pair(C++98 起最常見的寫法)
    auto p2 = std::make_pair(2, std::string("banana"));

    // (c) C++17 CTAD:不用寫 <int, std::string>
    std::pair p3(3, std::string("cherry"));

    std::cout << "  p1 = (" << p1.first << ", " << p1.second << ")\n";
    std::cout << "  p2 = (" << p2.first << ", " << p2.second << ")\n";
    std::cout << "  p3 = (" << p3.first << ", " << p3.second << ")\n";
}

// ---------------------------------------------------------------------------
// 範例 2:結構化繫結(C++17)拆解 pair
// ---------------------------------------------------------------------------
void demo_structured_binding() {
    std::cout << "[demo_structured_binding]\n";

    auto p = std::make_pair(42, std::string("the answer"));

    // auto [a, b] = p;  // 把 p.first 給 a、p.second 給 b
    auto [code, msg] = p;
    std::cout << "  code=" << code << ", msg=" << msg << "\n";
}

// ---------------------------------------------------------------------------
// 範例 3:Leetcode #1 Two Sum —— 回傳一對索引
//
// 解題思路:
//   1. 建一個雜湊表 seen,記錄「值 -> 它在陣列中的索引」。
//   2. 走訪 nums,對每個元素 x 檢查 (target - x) 是否已經在 seen 裡。
//      若在,代表之前那個元素 + 現在這個 = target,直接回傳兩個索引。
//   3. 若不在,把 x 與當前索引存入 seen,繼續往下走。
//
// 為什麼用 std::pair?
//   題目要回傳「兩個索引」,用 pair<int,int> 自然且清楚,不需要為
//   這種「小回傳值」特別定義一個 struct。
//
// 時間複雜度:O(n),空間複雜度:O(n)。
// ---------------------------------------------------------------------------
std::pair<int, int> twoSum(const std::vector<int>& nums, int target) {
    std::unordered_map<int, int> seen;             // 值 -> 索引
    for (int i = 0; i < static_cast<int>(nums.size()); ++i) {
        int need = target - nums[i];               // 我們希望找到的另一個值
        auto it = seen.find(need);
        if (it != seen.end()) {
            // it->first 是值、it->second 是之前那個索引
            return std::make_pair(it->second, i);
        }
        seen[nums[i]] = i;                         // 把當前值記下來
    }
    return std::make_pair(-1, -1);                 // 找不到(題目保證有解,僅備用)
}

void demo_leetcode_two_sum() {
    std::cout << "[demo_leetcode_two_sum]\n";
    std::vector<int> nums = {2, 7, 11, 15};
    int target = 9;

    auto [i, j] = twoSum(nums, target);            // 結構化繫結拆 pair
    std::cout << "  nums[" << i << "] + nums[" << j << "] = "
              << nums[i] << " + " << nums[j] << " = " << target << "\n";
}

// ---------------------------------------------------------------------------
// 範例 3.5:其他常用小工具 —— std::get、比較、swap、tuple-like 介面
//
// std::pair 雖然只有兩個成員,但它與 std::tuple 共用一套「tuple-like」介面,
// 因此下列工具也能用在 pair 上:
//   - std::get<I>(p)、std::get<T>(p):依索引或型別取出成員(C++14 起支援型別形式)
//   - std::tuple_size<P>::value:回傳元素數量(對 pair 一律是 2)
//   - std::tuple_element<I, P>::type:取得第 I 個元素的型別
//   - 六個比較運算子(==、!=、<、<=、>、>=):字典序比較 first、second
//   - 成員 swap 與非成員 std::swap:交換兩個 pair 的內容
//   - std::piecewise_construct + std::forward_as_tuple:把建構參數「分組」傳給
//     兩邊成員,常見於 std::map::emplace 內部把 (key 的建構參數, value 的建構參數)
//     一次塞進 pair。
// ---------------------------------------------------------------------------
void demo_pair_helpers() {
    std::cout << "[demo_pair_helpers]\n";

    std::pair<int, std::string> p(1, "apple");

    // (a) std::get<I>:用索引取出元素
    std::cout << "  get<0>(p)=" << std::get<0>(p)
              << ", get<1>(p)=" << std::get<1>(p) << "\n";

    // (b) std::get<T>:用「不重複的型別」取出元素(C++14 起)
    std::cout << "  get<int>(p)=" << std::get<int>(p)
              << ", get<std::string>(p)=" << std::get<std::string>(p) << "\n";

    // (c) tuple_size / tuple_element:在編譯期得知大小與成員型別
    std::cout << "  tuple_size = " << std::tuple_size<decltype(p)>::value << "\n";
    using FirstT = std::tuple_element<0, decltype(p)>::type;
    static_assert(std::is_same<FirstT, int>::value, "first 應該是 int");

    // (d) 比較運算子(字典序:先比 first,first 相等再比 second)
    std::pair<int, std::string> a(1, "apple"), b(1, "banana"), c(2, "apple");
    std::cout << "  a==a: " << (a == a)
              << ", a!=b: " << (a != b)
              << ", a<b:  " << (a < b)              // first 相等 → 比 second
              << ", a<c:  " << (a < c) << "\n";     // first 較小

    // (e) swap:成員形式與非成員 std::swap 形式
    std::pair<int, std::string> x(1, "x"), y(2, "y");
    x.swap(y);                                      // 成員 swap
    std::cout << "  member swap: x=(" << x.first << "," << x.second << ")\n";
    std::swap(x, y);                                // 非成員 std::swap
    std::cout << "  std::swap : x=(" << x.first << "," << x.second << ")\n";

    // (f) piecewise_construct:把兩組建構參數分別轉發給 first / second
    //     pair<std::string, std::vector<int>> 的兩個成員都用各自的建構參數初始化。
    std::pair<std::string, std::vector<int>> pc(
        std::piecewise_construct,
        std::forward_as_tuple(3, 'A'),              // string(3, 'A') → "AAA"
        std::forward_as_tuple(4, 7));               // vector<int>(4, 7) → {7,7,7,7}
    std::cout << "  piecewise: first=\"" << pc.first
              << "\", second.size=" << pc.second.size()
              << ", second[0]=" << pc.second[0] << "\n";
}

// ---------------------------------------------------------------------------
// 範例 4:日常工作實用範例 —— 解析 "key=value" 設定行
//
// 情境:讀取 .env 或 INI 設定檔時,常常需要把 "DB_HOST=127.0.0.1"
//       這種一行字串拆成 ("DB_HOST", "127.0.0.1")。
//       這正是「回傳兩個不同型別的值(其實都是字串,但語意不同)」的場景。
//
// 注意:這裡為了範例簡單,沒處理 trim、註解、引號等細節。
// ---------------------------------------------------------------------------
std::pair<std::string, std::string> parseKeyValue(const std::string& line) {
    auto eq = line.find('=');                      // 找第一個 '='
    if (eq == std::string::npos) {
        return {line, ""};                         // 沒有 '=' 就視為只有 key
    }
    return {line.substr(0, eq), line.substr(eq + 1)};
}

void demo_practical_config() {
    std::cout << "[demo_practical_config]\n";
    std::vector<std::string> lines = {
        "DB_HOST=127.0.0.1",
        "DB_PORT=5432",
        "APP_NAME=MyService",
    };
    for (const auto& ln : lines) {
        auto [key, value] = parseKeyValue(ln);
        std::cout << "  key=[" << key << "], value=[" << value << "]\n";
    }
}

// ---------------------------------------------------------------------------
// 範例 5:Leetcode #56 Merge Intervals(合併區間) (難度: medium)
//
// 題目大意:給一個區間陣列 intervals,intervals[i] = [start_i, end_i],
//          合併所有重疊的區間,回傳不重疊的區間陣列。
//
// 解題思路:
//   1. 按 start 升序排序所有區間。
//   2. 走訪排序後的區間,如果當前區間的 start <= 結果最後一個區間的 end,
//      表示有重疊,合併 (更新 end 為 max(end, 當前.end))。
//   3. 否則直接 push_back。
//
// 為什麼用 std::pair?
//   區間天然就是「兩個值的組合」,用 pair<int,int> 表達最直接。
//   也讓 std::sort 直接走 pair 預設的字典序比較 (先比 first 即 start)。
//
// 時間 O(n log n),空間 O(1) (排序成本)。
// ---------------------------------------------------------------------------
std::vector<std::pair<int,int>>
mergeIntervals(std::vector<std::pair<int,int>> intervals) {
    if (intervals.empty()) return {};
    std::sort(intervals.begin(), intervals.end());  // pair 字典序就夠
    std::vector<std::pair<int,int>> out;
    out.push_back(intervals[0]);
    for (size_t i = 1; i < intervals.size(); ++i) {
        if (intervals[i].first <= out.back().second) {
            out.back().second = std::max(out.back().second, intervals[i].second);
        } else {
            out.push_back(intervals[i]);
        }
    }
    return out;
}

void demo_leetcode_merge_intervals() {
    std::cout << "[demo_leetcode_merge_intervals]\n";
    auto merged = mergeIntervals({{1,3},{2,6},{8,10},{15,18}});
    for (auto& [a, b] : merged)
        std::cout << "  [" << a << "," << b << "]\n";
}

// ---------------------------------------------------------------------------
// 主程式
// ---------------------------------------------------------------------------
int main() {
    demo_basic();
    demo_structured_binding();
    demo_pair_helpers();
    demo_leetcode_two_sum();
    demo_leetcode_merge_intervals();
    demo_practical_config();
    return 0;
}

/*
================================================================================
編譯與執行:
    g++ -std=c++17 -Wall -Wextra 01_pair.cpp -o 01_pair && ./01_pair

預期輸出:
    [demo_basic]
      p1 = (1, apple)
      p2 = (2, banana)
      p3 = (3, cherry)
    [demo_structured_binding]
      code=42, msg=the answer
    [demo_pair_helpers]
      get<0>(p)=1, get<1>(p)=apple
      get<int>(p)=1, get<std::string>(p)=apple
      tuple_size = 2
      a==a: 1, a!=b: 1, a<b:  1, a<c:  1
      member swap: x=(2,y)
      std::swap : x=(1,x)
      piecewise: first="AAA", second.size=4, second[0]=7
    [demo_leetcode_two_sum]
      nums[0] + nums[1] = 2 + 7 = 9
    [demo_practical_config]
      key=[DB_HOST], value=[127.0.0.1]
      key=[DB_PORT], value=[5432]
      key=[APP_NAME], value=[MyService]
================================================================================
*/

// 編譯: g++ -std=c++20 -Wall -Wextra 01_pair.cpp -o 01_pair

// === 預期輸出 ===
// [demo_basic]
//   p1 = (1, apple)
//   p2 = (2, banana)
//   p3 = (3, cherry)
// [demo_structured_binding]
//   code=42, msg=the answer
// [demo_pair_helpers]
//   get<0>(p)=1, get<1>(p)=apple
//   get<int>(p)=1, get<std::string>(p)=apple
//   tuple_size = 2
//   a==a: 1, a!=b: 1, a<b:  1, a<c:  1
//   member swap: x=(2,y)
//   std::swap : x=(1,x)
//   piecewise: first="AAA", second.size=4, second[0]=7
// [demo_leetcode_two_sum]
//   nums[0] + nums[1] = 2 + 7 = 9
// [demo_leetcode_merge_intervals]
//   [1,6]
//   [8,10]
//   [15,18]
// [demo_practical_config]
//   key=[DB_HOST], value=[127.0.0.1]
//   key=[DB_PORT], value=[5432]
//   key=[APP_NAME], value=[MyService]
