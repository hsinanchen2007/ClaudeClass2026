/*
================================================================================
主題:std::variant —— 型別安全的 union(同一時間只裝其中一種型別)
標準:C++17 起
標頭:<variant>
參考:https://en.cppreference.com/w/cpp/utility/variant
================================================================================

【一、課題介紹】
  std::variant<T1, T2, ..., Tn> 表示「在這幾個型別中,當下持有其中一個」。
  你可以把它想成「型別安全版的 union」:
    - 傳統 C union 沒有記錄目前到底裝著哪個成員,使用者必須自己用 enum
      tag 配合,容易出錯。
    - std::variant 內部自帶 index(目前持有第幾個型別),且具備正確
      的建構/解構/拷貝/移動語意,還會在你「拿錯型別」時擲例外或返回 nullptr。

  為什麼需要它?
    - 表達「一個欄位可能是多種型別中的一種」(像是 JSON 的 value:
      可能是 number / string / bool / null / array / object)。
    - 表達「狀態機」當下的某個狀態,每個狀態有不同的附加資料。
    - 取代「(enum tag, void*)」這種 C 風格的可怕寫法。

【二、觀念解釋】
  1. 建立:
       std::variant<int, std::string, double> v;            // 預設裝第一個型別 int(=0)
       std::variant<int, std::string, double> v2 = 3.14;    // 裝 double
       std::variant<int, std::string, double> v3{std::string("hi")};

  2. 查詢目前裝什麼:
       v2.index();                  // 回傳 0 / 1 / 2(目前是第幾個型別)
       std::holds_alternative<double>(v2);  // 是否裝 double

  3. 取值:
       std::get<int>(v);            // 拿錯型別會擲 std::bad_variant_access
       std::get<0>(v);              // 也可以用編號
       std::get_if<int>(&v);        // 取不到回傳 nullptr,不擲例外(推薦)

  4. 走訪所有型別 —— std::visit + 多載 lambda(Overloaded pattern):
       std::visit([](auto&& x){ ... }, v);
     最常見的範式是傳一個能對所有可能型別都處理的「visitor」進去。

【三、常見陷阱】
  - 預設建構:會去呼叫「第一個型別」的預設建構子,所以放第一個的型別必須
    可預設建構,否則 variant 也無法預設建構(常見錯誤)。
  - std::get<T> 在沒裝 T 時會擲例外,效能/程式碼簡潔上多半會偏好 get_if。
  - 不要把 variant 當成 inheritance 的替代品來建構大型多型體系 ——
    variant 適合「型別少且固定」的場景,動態多型仍然有它的價值。

【四、與其他 utility 的比較】
  - vs std::optional:optional 是「有 / 沒有」(可視為 variant<T, monostate>)。
  - vs std::any:any 可裝「任何型別」,代價是型別必須 runtime 查;variant
    在編譯期就鎖死可能型別,效能與安全性都更好。
  - vs union:variant 是型別安全、且支援非 trivial 型別(string、vector...)。

【五、Leetcode 對應題目】
  題號:771. Jewels and Stones(寶石與石頭)— 變形版
  難度:Easy
  連結:https://leetcode.com/problems/jewels-and-stones/
  題目大意:給字串 jewels 與 stones,計算 stones 中有多少字元是寶石。
  選用理由:本範例把計分規則做成 variant<int, double, string>:
            可能是「常數加分」「加倍率」「特殊規則名」之一,展示如何用
            std::visit 對多種型別統一處理。

【六、日常工作實用範例】
  情境:設定欄位「逾時」可以填整數秒,或字串如 "30s"、"1m"。
        把這個欄位用 variant<int, std::string> 表達。
================================================================================
*/

/*
補充筆記：std::variant
  - variant 是封閉集合：值在列出的型別中恰好選一種。
  - std::visit 讓你在編譯期列出每種型別的處理方式，漏處理會更容易被發現。
  - 如果型別集合會被外部任意擴充，繼承或 type erasure 可能比 variant 合適。
  - std::variant 屬於 utility 類工具；這些型別與函式常用來表達小型資料組合、可選值、型別安全聯集或值類別轉換。
  - pair/tuple 適合簡短聚合結果，但欄位語意複雜時應定義具名 struct，避免 first/second 或 get<0> 難讀。
  - optional 表示可能沒有值，使用前要檢查 has_value 或使用 value_or；value() 在無值時會丟例外。
  - variant 表示多選一型別，應用 visit 或 holds_alternative/get_if 安全存取目前替代項。
  - any 提供執行期任意型別保存，但取回需要知道正確型別；過度使用會失去靜態型別檢查優勢。
  - std::move/std::forward/std::exchange/as_const 都是表達意圖的工具；它們本身不一定搬移或複製資料。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::variant（C++17）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::variant 和原生 union 差在哪？
//     答：variant 是型別安全的 tagged union：它額外保存一個 index 記錄目前持有哪個
//     alternative，切換型別時自動呼叫正確的解構與建構，取錯型別會拋
//     std::bad_variant_access（get）或回 nullptr（get_if）。原生 union 不記型別、不會
//     自動管理 non-trivial 成員（要手動 placement new 與顯式解構），讀取非最後寫入的
//     成員在 C++ 中是 UB。
//     追問：variant 會堆配置嗎？（不會，就地儲存；這也是它和 std::any 的關鍵差異）
//
// 🔥 Q2. std::visit 是什麼？為什麼比一串 holds_alternative 好？
//     答：std::visit(visitor, v) 對目前持有的 alternative 呼叫對應的重載，而且強制你
//     處理所有 alternative——漏掉就是編譯錯誤，日後新增型別時編譯器會提醒；if-else 串接
//     則會靜默漏掉。常配 overload 慣用法：
//     template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
//     追問：這段用到哪些 C++17 特性？（變參 using 展開，以及 CTAD 加推導指引；C++20 起
//     aggregate 可 CTAD，推導指引可省）所有分支的回傳型別必須一致嗎？（必須）
//
// Q3. get、get_if、holds_alternative 怎麼選？
//     答：std::get<T>(v) 取錯會拋例外；std::get_if<T>(&v) 回傳指標、不符時是 nullptr、
//     不拋例外；std::holds_alternative<T>(v) 只查詢不取值。注意 variant<int, int> 是
//     合法的，但因為型別有歧義，只能用索引 get<0>／get<1> 存取。
//
// ⚠️ 陷阱. valueless_by_exception() 什麼時候會成立？
//     答：對 variant 賦值或 emplace 成另一個 alternative 時，會先解構舊值再建構新值；
//     若新值的建構函式拋出例外，舊值已毀、新值未成，variant 就進入 valueless 狀態，
//     此時 index() 回 variant_npos，get 與 visit 都會拋 bad_variant_access。降低風險的
//     做法是讓所有 alternative 的移動建構函式是 noexcept。
//     為什麼會錯：以為「type-safe union 保證永遠有值」，忽略了例外可能打斷型別切換。
// ═══════════════════════════════════════════════════════════════════════════

#include <cmath>
#include <iostream>
#include <variant>
#include <string>
#include <vector>
#include <type_traits>

// ---------------------------------------------------------------------------
// 範例 1:基本建立、index、std::get / get_if
// ---------------------------------------------------------------------------
void demo_basic() {
    std::cout << "[demo_basic]\n";

    std::variant<int, std::string, double> v = 42;
    std::cout << "  index=" << v.index()
              << ", value=" << std::get<int>(v) << "\n";

    v = std::string("hello");
    std::cout << "  index=" << v.index()
              << ", value=" << std::get<std::string>(v) << "\n";

    v = 3.14;
    // 推薦:get_if 拿不到回 nullptr,不會擲例外
    if (auto p = std::get_if<double>(&v)) {
        std::cout << "  it is double = " << *p << "\n";
    }
}

// ---------------------------------------------------------------------------
// 範例 2:std::visit + 多載 lambda(Overloaded pattern)
//
// 想對 variant 內每一種可能型別「分別」處理,std::visit 是最乾淨的方法。
// 下面這個小工具 Overloaded 把多個 lambda 合成一個多載物件,
// std::visit 便會根據實際型別挑出對應的 lambda 來執行。
// ---------------------------------------------------------------------------
template <class... Fs> struct Overloaded : Fs... { using Fs::operator()...; };
template <class... Fs> Overloaded(Fs...) -> Overloaded<Fs...>;   // C++17 CTAD

void demo_visit() {
    std::cout << "[demo_visit]\n";
    std::vector<std::variant<int, std::string, double>> items = {
        1, std::string("two"), 3.5
    };

    for (const auto& item : items) {
        std::visit(Overloaded{
            [](int x)                 { std::cout << "  int=" << x << "\n"; },
            [](const std::string& s)  { std::cout << "  str=" << s << "\n"; },
            [](double d)              { std::cout << "  dbl=" << d << "\n"; },
        }, item);
    }
}

// ---------------------------------------------------------------------------
// 範例 2.5:其他常用工具
//
//   - std::holds_alternative<T>(v):測試 variant 目前是否裝著 T
//   - std::get<I>(v) / std::get_if<I>(&v):依索引取值(get<I> 拿錯擲例外)
//   - v.emplace<T>(args...):就地把 variant 換成型別 T,並用 args 建構
//   - v.valueless_by_exception():在「轉換中發生例外」造成無值狀態時為 true
//   - std::variant_size<V>::value:variant 可裝的型別數量(編譯期)
//   - std::variant_alternative<I, V>::type:第 I 個可裝的型別(編譯期)
//   - std::monostate:當 variant 第一個型別不可預設建構,
//                     就把 monostate 放在最前面當「空狀態」
//   - 比較運算子:同型別才比值,否則比 index
//   - std::variant_npos:index() 在 valueless 時回傳此值
// ---------------------------------------------------------------------------
void demo_variant_helpers() {
    std::cout << "[demo_variant_helpers]\n";

    std::variant<int, std::string, double> v = 42;

    // (a) holds_alternative
    std::cout << "  holds<int>="    << std::holds_alternative<int>(v)
              << ", holds<string>=" << std::holds_alternative<std::string>(v) << "\n";

    // (b) std::get<I> / std::get_if<I>:索引版本
    std::cout << "  get<0>(v)=" << std::get<0>(v) << "\n";
    if (auto p = std::get_if<0>(&v)) std::cout << "  get_if<0>=" << *p << "\n";

    // (c) emplace<T>:就地把 variant 換成另一個型別
    v.emplace<std::string>(5, '!');                // 等同 std::string(5, '!')
    std::cout << "  after emplace<string>: \"" << std::get<std::string>(v) << "\"\n";

    // (d) variant_size / variant_alternative:編譯期反射
    std::cout << "  variant_size=" << std::variant_size<decltype(v)>::value << "\n";
    using SecondT = std::variant_alternative<1, decltype(v)>::type;
    static_assert(std::is_same<SecondT, std::string>::value, "第 1 個型別應為 string");

    // (e) valueless_by_exception 與 variant_npos
    std::cout << "  valueless=" << v.valueless_by_exception() << "\n";
    std::cout << "  npos=" << std::variant_npos << "\n";

    // (f) std::monostate:讓「沒值」也能成為一個合法狀態
    //     若第一個型別本身不可預設建構,可在最前面放 monostate 當預設。
    std::variant<std::monostate, int, std::string> empty_first;
    std::cout << "  monostate first: index=" << empty_first.index() << "\n";

    // (g) 比較運算子:不同型別比 index;同型別才比值
    std::variant<int, std::string> p1 = 1;
    std::variant<int, std::string> p2 = std::string("x");
    std::cout << "  p1<p2 (int<string by index)? " << (p1 < p2) << "\n";

    // (h) std::get<T> 拿錯型別會擲例外
    try {
        (void)std::get<int>(v);                     // v 目前是 string,故擲例外
    } catch (const std::bad_variant_access& e) {
        std::cout << "  caught bad_variant_access: " << e.what() << "\n";
    }
}

// ---------------------------------------------------------------------------
// 範例 3:Leetcode #771 Jewels and Stones(變形版)
//
// 題目原版:給寶石字元集 jewels 與石頭字串 stones,回傳 stones 中是寶石的字元數。
//
// 我們的擴充:每顆寶石的「分數規則」可能是三種:
//   - int     : 直接加這個固定分數
//   - double  : 把目前累計分數乘上這個倍率
//   - string  : 特殊命名規則(這裡只示範 "DOUBLE_LATER" 表示後面數量翻倍)
// 這正好是 variant 的應用場景。為了範例簡單,規則用一個 map 建立。
//
// 解題思路:
//   1. 走訪 stones,每個字元若是寶石,根據規則更新分數。
//   2. 用 std::visit 對 variant 各型別分別處理。
// ---------------------------------------------------------------------------
#include <unordered_map>
using ScoreRule = std::variant<int, double, std::string>;

double computeScore(const std::string& jewels,
                    const std::string& stones,
                    const std::unordered_map<char, ScoreRule>& rules) {
    double score = 0;
    int laterMultiplier = 1;
    for (char c : stones) {
        if (jewels.find(c) == std::string::npos) continue;  // 不是寶石
        auto it = rules.find(c);
        if (it == rules.end()) { score += laterMultiplier; continue; }

        std::visit(Overloaded{
            [&](int x)               { score += x * laterMultiplier; },
            [&](double m)            { score *= m; },
            [&](const std::string& tag) {
                if (tag == "DOUBLE_LATER") laterMultiplier *= 2;
            },
        }, it->second);
    }
    return score;
}

void demo_leetcode_jewels() {
    std::cout << "[demo_leetcode_jewels]\n";
    std::unordered_map<char, ScoreRule> rules = {
        {'a', 10},                        // 每顆 a +10
        {'A', std::string("DOUBLE_LATER")}, // 遇到 A 之後寶石翻倍
        {'B', 1.5},                       // 遇到 B 把目前分數 ×1.5
    };
    std::cout << "  score=" << computeScore("aA", "aAaa", rules) << "\n";
    // 過程:a +10 → 10;A 之後翻倍;a +10*2=20 → 30;a +10*2=20 → 50
}

// ---------------------------------------------------------------------------
// 範例 4:日常工作實用範例 —— 設定欄位「逾時」可填 int 或 string
//
// 情境:很多設定 schema 允許使用者填整數秒(30)或人類友善字串("30s")。
//       用 variant<int, std::string> 表達這個欄位,讀取時用 visit 一次處理。
// ---------------------------------------------------------------------------
using Timeout = std::variant<int, std::string>;

int toSeconds(const Timeout& t) {
    return std::visit(Overloaded{
        [](int sec)               { return sec; },
        [](const std::string& s) {
            // 超簡化解析:結尾 's' 視為秒,'m' 視為分;其他一律當秒
            if (s.empty()) return 0;
            char unit = s.back();
            int n = std::stoi(s.substr(0, s.size() - 1));
            if (unit == 's') return n;
            if (unit == 'm') return n * 60;
            return std::stoi(s);
        },
    }, t);
}

void demo_practical_timeout() {
    std::cout << "[demo_practical_timeout]\n";
    Timeout a = 30;
    Timeout b = std::string("45s");
    Timeout c = std::string("2m");
    std::cout << "  a=" << toSeconds(a) << "s\n";
    std::cout << "  b=" << toSeconds(b) << "s\n";
    std::cout << "  c=" << toSeconds(c) << "s\n";
}

// ---------------------------------------------------------------------------
// 實用範例 (額外):小型計算器 —— variant<int, double, error>
//
// 工作中常見:同一個結果可能是「整數」「小數」或「錯誤訊息」三種型別之一,
// 用 variant 比回傳 (bool, value, error) 三元組更乾淨,呼叫端用 visit 一次處理。
// ---------------------------------------------------------------------------
struct CalcError { std::string msg; };
using CalcResult = std::variant<int, double, CalcError>;

CalcResult divide(double a, double b) {
    if (b == 0.0) return CalcError{"divide by zero"};
    double r = a / b;
    if (std::abs(r - static_cast<int>(r)) < 1e-9)
        return static_cast<int>(r);                  // 剛好是整數
    return r;
}

void demo_practical_calc() {
    std::cout << "[demo_practical_calc]\n";
    auto printResult = [](const CalcResult& r) {
        std::visit(Overloaded{
            [](int v)              { std::cout << "  整數結果: " << v << "\n"; },
            [](double v)           { std::cout << "  小數結果: " << v << "\n"; },
            [](const CalcError& e) { std::cout << "  錯誤: " << e.msg << "\n"; },
        }, r);
    };
    printResult(divide(10, 2));                     // 整數 5
    printResult(divide(10, 3));                     // 小數
    printResult(divide(10, 0));                     // 錯誤
}

int main() {
    demo_basic();
    demo_visit();
    demo_variant_helpers();
    demo_leetcode_jewels();
    demo_practical_timeout();
    demo_practical_calc();
    return 0;
}

/*
================================================================================
編譯與執行:
    g++ -std=c++17 -Wall -Wextra 04_variant.cpp -o 04_variant && ./04_variant
================================================================================
*/

// 編譯: g++ -std=c++20 -Wall -Wextra 04_variant.cpp -o 04_variant

// === 預期輸出 (節錄) ===
// [demo_basic]
//   index=0, value=42
//   index=1, value=hello
//   it is double = 3.14
// [demo_visit]
//   int=1
//   str=two
//   dbl=3.5
// [demo_variant_helpers]
//   holds<int>=1, holds<string>=0
//   get<0>(v)=42
//   get_if<0>=42
//   after emplace<string>: "!!!!!"
//   variant_size=3
//   valueless=0
//   npos=18446744073709551615
//   monostate first: index=0
//   p1<p2 (int<string by index)? 1
//   caught bad_variant_access: std::get: wrong index for variant
// [demo_leetcode_jewels]
// …（後略，完整輸出共 29 行）
