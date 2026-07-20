/*
================================================================================
主題:std::as_const —— 把物件「當成 const」看待
標準:C++17 起
標頭:<utility>
參考:https://en.cppreference.com/w/cpp/utility/as_const
================================================================================

【一、課題介紹】
  std::as_const(x) 把 x 轉成 const T&。看起來「平淡無奇」,但它解決一個
  非常具體的痛點:**當你只想要呼叫 const 版本的成員函式 / 走 const 迭代器,
  又不想為此另寫一個 const 變數時,as_const 是最乾淨的寫法。**

  經典場景:
    1. STL 容器同時提供「const 版」與「非 const 版」的 begin() / end()。
       對 mutable 物件呼叫 begin() 預設拿到「非 const iterator」。
       若你想走訪時不打算修改,寫:
         for (const auto& x : container)            // 已經夠了
       但若你想用 .find() / .at() 等成員函式拿到 const 結果,as_const 直接幫忙。

    2. 把可寫的物件「保險地」傳給只接 const& 的 API,確保自己這支函式不會誤改它。

    3. 避免某些函式對「非 const」與「const」有完全不同的多載,讓你一眼挑到 const 版。

【二、觀念解釋】
  1. 標頭:<utility>。
  2. 形式:std::as_const(x) 等同於 const_cast<const T&>(x)。
     —— 但比 const_cast 安全:**它只能加 const,不能拿掉 const。**
  3. 它接「左值」,不接 rvalue(規範禁止 std::as_const(rvalue)),
     避免你在臨時物件上加 const 卻立刻搞丟參考。

【三、常見陷阱】
  - as_const 不是 deep const:它只把「最外層」變成 const。指標元素仍可被改。
  - 不要把 as_const 當作避免拷貝的工具;它就是個型別 cast,沒有效能含義。
  - 不可對 rvalue 使用,例如 std::as_const(std::string("hi")) 編譯失敗。

【四、與其他 utility 的比較】
  - vs const_cast<const T&>:as_const 安全(僅能加 const),且更具可讀性。
  - vs 直接寫 const T& y = x:as_const 不需另開變數,內聯使用。

【五、Leetcode 對應題目】
  題號:1. Two Sum(兩數之和)的 const 走訪示範
  難度:Easy
  連結:https://leetcode.com/problems/two-sum/
  選用理由:Two Sum 解法走訪 nums 時不應修改它;搭配 std::as_const 強迫
            自己用 const 介面,避免不小心把 nums 改了。

【六、日常工作實用範例】
  情境:有一個 mutable 的設定 map,你寫一個 dump() 函式想印出所有 key/value,
        想保險地以 const 方式遍歷,避免使用了會修改 map 的 operator[]。
================================================================================
*/

/*
補充筆記：std::as_const
  - std::as_const 屬於 utility 類工具；這些型別與函式常用來表達小型資料組合、可選值、型別安全聯集或值類別轉換。
  - pair/tuple 適合簡短聚合結果，但欄位語意複雜時應定義具名 struct，避免 first/second 或 get<0> 難讀。
  - optional 表示可能沒有值，使用前要檢查 has_value 或使用 value_or；value() 在無值時會丟例外。
  - variant 表示多選一型別，應用 visit 或 holds_alternative/get_if 安全存取目前替代項。
  - any 提供執行期任意型別保存，但取回需要知道正確型別；過度使用會失去靜態型別檢查優勢。
  - std::move/std::forward/std::exchange/as_const 都是表達意圖的工具；它們本身不一定搬移或複製資料。
  - std::as_const(x) 把 x 轉成 const reference，常用來強制呼叫 const overload。
  - as_const 不接受 rvalue，這是刻意避免回傳指向暫時物件的 const reference。
  - 它不會複製物件，只改變這次表達式看到的 const 視角。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::as_const（C++17）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::as_const 用來做什麼？
//     答：std::as_const(x) 回傳 const T&，用來強制選中 const 重載，比寫
//     const_cast<const T&>(x) 更清楚也更安全（const_cast 的字面意思容易被誤讀成
//     「拿掉 const」）。典型場景：容器同時有 begin() 與 begin() const，想確保拿到
//     const_iterator；或想避免呼叫到會產生非 const 副作用的重載。
//     追問：C++11/14 沒有它的時候怎麼做？（自己寫一個同樣的小函式模板，或直接用
//     cbegin()／cend() 這類 const 版本的具名函式）
//
// 🔥 Q2. 為什麼 std::as_const 的右值多載被 = delete？
//     答：as_const(T&&) 被刻意刪除，避免對臨時物件取得 const 參考之後懸垂——
//     const auto& r = std::as_const(makeTemp()); 若允許，r 綁定的來源在完整運算式結束
//     後就消失了。刪除該多載讓這個錯誤在編譯期就被擋下。
//
// ⚠️ 陷阱. as_const 會把物件變成 const 嗎？
//     答：不會。它只是回傳一個 const 參考，原物件的常數性完全沒變，之後仍可透過原名字
//     修改它。它改變的只有「這一次呼叫要選哪個重載」。
//     為什麼會錯：名字讀起來像一個轉換動作，讓人以為對象本身被「凍結」了。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <utility>
#include <vector>
#include <unordered_map>
#include <string>

// ---------------------------------------------------------------------------
// 範例 1:基本用法 —— 強迫 const 版本被選中
//
// 我們提供一個函式 process,有 const / 非 const 兩個多載。
// 直接呼叫 process(x) 會挑非 const 版;包一層 std::as_const 後,
// 編譯器只能挑 const 版。
// ---------------------------------------------------------------------------
void process(std::string&)       { std::cout << "  process(non-const)\n"; }
void process(const std::string&) { std::cout << "  process(const)\n"; }

void demo_basic() {
    std::cout << "[demo_basic]\n";
    std::string s = "hello";
    process(s);                                   // 走 non-const
    process(std::as_const(s));                    // 走 const
}

// ---------------------------------------------------------------------------
// 範例 2:走訪 unordered_map 用 const 介面
//
// unordered_map 的 operator[] 「不存在 key 會自動插入空項」,曾經是無數 bug 來源。
// 用 std::as_const(m) 後,你只能呼叫 const 版 (.find / .at),完全避開這個陷阱。
// ---------------------------------------------------------------------------
void demo_const_iteration() {
    std::cout << "[demo_const_iteration]\n";
    std::unordered_map<std::string, int> m = {{"a",1},{"b",2}};

    // 走訪時用 const 視角:得到的是 const_iterator
    for (const auto& kv : std::as_const(m)) {
        std::cout << "  " << kv.first << "=" << kv.second << "\n";
    }

    // 也避免不小心把 [] 寫上去:下行若沒先 as_const,
    //    int v = m["nope"];   會把 "nope" 自動插入 m,常見 bug
    auto it = std::as_const(m).find("nope");
    std::cout << "  find(\"nope\") -> "
              << (it == m.end() ? "not found" : "found") << "\n";
    std::cout << "  m.size after lookup = " << m.size() << "\n";  // 仍是 2
}

// ---------------------------------------------------------------------------
// 範例 3:Leetcode #1 Two Sum —— 用 as_const 確保不誤改
//
// 我們把 twoSum 的內部走訪寫成 std::as_const(nums),強迫自己用 const 介面,
// 杜絕「不小心 nums[i] = ... 把輸入改了」的可能。
//
// 解題思路與 01_pair.cpp 中相同(雜湊表 O(n))。
// ---------------------------------------------------------------------------
std::pair<int,int> twoSum(const std::vector<int>& nums, int target) {
    std::unordered_map<int,int> seen;
    // 注意:即使 nums 已是 const&,這裡用 std::as_const(nums) 純粹示範
    // 「對任何左值都可加上 const 視角」。
    int n = static_cast<int>(std::as_const(nums).size());
    for (int i = 0; i < n; ++i) {
        int need = target - std::as_const(nums)[i];
        auto it = seen.find(need);
        if (it != seen.end()) return {it->second, i};
        seen[std::as_const(nums)[i]] = i;
    }
    return {-1, -1};
}

void demo_leetcode_two_sum() {
    std::cout << "[demo_leetcode_two_sum]\n";
    std::vector<int> nums = {2, 7, 11, 15};
    auto [i, j] = twoSum(nums, 9);
    std::cout << "  index pair = (" << i << ", " << j << ")\n";
}

// ---------------------------------------------------------------------------
// 範例 4:日常工作實用範例 —— 印出設定 map 時用 const 視角
//
// 情境:dumpConfig() 印出設定的所有鍵值,絕對不應該動到 map。
//       明確透過 std::as_const(m) 走訪,既當文件、也防誤改。
// ---------------------------------------------------------------------------
void dumpConfig(std::unordered_map<std::string, std::string>& m) {
    for (const auto& kv : std::as_const(m)) {
        std::cout << "  " << kv.first << " = " << kv.second << "\n";
    }
}

void demo_practical_dump() {
    std::cout << "[demo_practical_dump]\n";
    std::unordered_map<std::string, std::string> cfg = {
        {"host", "127.0.0.1"},
        {"port", "8080"},
        {"app",  "MyService"},
    };
    dumpConfig(cfg);
}

// ---------------------------------------------------------------------------
// 實用範例 (額外):safe_lookup —— 不可改 vector 的封裝
//
// 工作中常見:你拿到一個 vector& 但承諾「絕對不會改它」,可用 std::as_const
// 把每次取用都轉成 const 視角, 從編譯期阻止誤改。對 code review 也有幫助。
// ---------------------------------------------------------------------------
int safe_lookup(std::vector<int>& v, int idx, int fallback) {
    // 明明 v 是非 const, 但我們只用 const 視角取值
    if (idx < 0 || idx >= (int)std::as_const(v).size()) return fallback;
    return std::as_const(v).at(idx);
    // 寫成 v[idx] 也可以, 但 std::as_const + .at() 強迫 const overload,
    // 還順便享有越界檢查 (拋例外, 而不是 UB)。
}

void demo_practical_safe_lookup() {
    std::cout << "[demo_practical_safe_lookup]\n";
    std::vector<int> data{10, 20, 30};
    std::cout << "  safe_lookup(1, -1) = " << safe_lookup(data, 1, -1) << "\n";   // 20
    std::cout << "  safe_lookup(5, -1) = " << safe_lookup(data, 5, -1) << "\n";   // -1
    // 注意 data.size() 沒被修改
    std::cout << "  data.size() = " << data.size() << "\n";
}

int main() {
    demo_basic();
    demo_const_iteration();
    demo_leetcode_two_sum();
    demo_practical_dump();
    demo_practical_safe_lookup();
    return 0;
}

/*
================================================================================
編譯與執行:
    g++ -std=c++17 -Wall -Wextra 10_as_const.cpp -o 10_as_const && ./10_as_const
================================================================================
*/

// 編譯: g++ -std=c++20 -Wall -Wextra 10_as_const.cpp -o 10_as_const

// === 預期輸出 ===
// [demo_basic]
//   process(non-const)
//   process(const)
// [demo_const_iteration]
//   b=2
//   a=1
//   find("nope") -> not found
//   m.size after lookup = 2
// [demo_leetcode_two_sum]
//   index pair = (0, 1)
// [demo_practical_dump]
//   app = MyService
//   port = 8080
//   host = 127.0.0.1
// [demo_practical_safe_lookup]
//   safe_lookup(1, -1) = 20
//   safe_lookup(5, -1) = -1
//   data.size() = 3
