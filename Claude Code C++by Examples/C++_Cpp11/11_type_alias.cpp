// =============================================================================
//  11_type_alias.cpp  —  using = 取代 typedef (C++11)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/type_alias
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、語法                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//  C++98 的 typedef：
//      typedef unsigned int u32;
//      typedef std::vector<int>::iterator IntIt;
//
//  C++11 起 using 別名：
//      using u32 = unsigned int;
//      using IntIt = std::vector<int>::iterator;
//
//  讀法更直觀（「new = old」），且 using 支援「alias template」。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、alias template — typedef 做不到                        │
//  └────────────────────────────────────────────────────────────┘
//
//  template alias = 「給 template 取個短名」：
//
//      template <class T>
//      using Vec = std::vector<T>;
//
//      Vec<int> v;        // 等同 std::vector<int>
//
//  典型用例：
//   * 縮短長 template type
//   * 統一專案內常用型別命名
//   * 跟 std::enable_if、std::conditional 等 metafunction 結合
//
//      template <class T>
//      using EnableIfInt = typename std::enable_if<std::is_integral<T>::value, T>::type;
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、function pointer 的 alias — using 比 typedef 直觀      │
//  └────────────────────────────────────────────────────────────┘
//
//  typedef：
//      typedef int (*FP)(int, int);
//
//  using：
//      using FP = int(*)(int, int);
//
//  using 把「= 右側」當成「整個型別」 — 直覺、不必拆 ()。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：基本 using = 別名
//   * Demo 2：alias template — 簡寫 vector / map
//   * Demo 3：function pointer alias
// =============================================================================

/*
補充筆記：type_alias
  - type_alias 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - using Alias = Type 比 typedef 更容易閱讀，尤其是函式指標和 template alias。
  - alias 不建立新型別，只是原型別的另一個名字；型別安全性不會因此提高。
*/
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

// 簡單別名
using i64 = long long;
using StrVec = std::vector<std::string>;

// alias template
template <class K, class V>
using Map = std::map<K, V>;

template <class T>
using Vec = std::vector<T>;

// function pointer alias
using BinaryOp = int(*)(int, int);

int add(int a, int b) { return a + b; }
int mul(int a, int b) { return a * b; }

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：簡單別名
    // ─────────────────────────────────────────────────────────
    i64 big = 9000000000LL;        // 注意：千位分隔符 9'000'000'000 是 C++14 特性
    StrVec names{"alice", "bob", "charlie"};
    std::cout << "[Demo1] big=" << big << " names.size=" << names.size() << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：alias template
    // ─────────────────────────────────────────────────────────
    Map<std::string, int> ages{{"alice", 30}, {"bob", 25}};
    Vec<int> nums{1, 2, 3, 4, 5};
    std::cout << "[Demo2] ages.size=" << ages.size()
              << " nums.size=" << nums.size() << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 3：function pointer alias
    // ─────────────────────────────────────────────────────────
    BinaryOp ops[] = {add, mul};
    for (auto op : ops) {
        std::cout << "[Demo3] op(3, 4) = " << op(3, 4) << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：什麼時候用 typedef、什麼時候 using？
    //    A：新程式碼一律用 using — 語法直觀、支援 template、跟 namespace
    //       聲明一致（using namespace foo 也是 using）。typedef 只剩「跟
    //       老 codebase 一致」這條理由。
    //
    //  Q2：alias 跟 derived class 差在哪？
    //    A：alias 完全是「同一個型別」 — 沒有新型別產生；不能 overload 對
    //       基底與 alias 的不同處理。derived class 是「新型別」 — 可獨立
    //       overload。
    //
    //  Q3：strong typedef（不允許隱式互轉）怎麼做？
    //    A：標準沒有。要做 strong typedef 用「empty derived class」或第三
    //       方函式庫（strong_typedef、Boost.Serialization）。例：
    //         struct UserId { int v; };  // 跟 int 不互通
    //
    // ─────────────────────────────────────────────────────────
    // LeetCode 1. Two Sum
    //   題意：給 nums 與 target，找兩個 index 使其和為 target。
    //   為什麼放這？用 type alias 表達「索引集合」型別，介面更明確。
    // ─────────────────────────────────────────────────────────
    using Indices = std::vector<int>;
    using IndexMap = std::map<int, int>;       // value -> index
    auto twoSum = [](const std::vector<int>& nums, int target) -> Indices {
        IndexMap seen;
        for (int i = 0; i < static_cast<int>(nums.size()); ++i) {
            int need = target - nums[i];
            auto it = seen.find(need);
            if (it != seen.end()) return Indices{it->second, i};
            seen[nums[i]] = i;
        }
        return Indices{};
    };
    Indices ans = twoSum({2, 7, 11, 15}, 9);
    std::cout << "[LC1] indices:";
    for (int x : ans) std::cout << ' ' << x;
    std::cout << '\n';

    // ─────────────────────────────────────────────────────────
    // 實用範例：Callback 型別 alias — 讓 API 更清楚
    //   工作上常見：定義 EventHandler 型別，傳遞時不必每次寫一長串
    // ─────────────────────────────────────────────────────────
    using OnClick = std::function<void(int, int)>;
    auto registerHandler = [](OnClick cb) {
        cb(100, 200);                  // 模擬事件觸發
    };
    registerHandler([](int x, int y) {
        std::cout << "[Demo4] clicked at (" << x << "," << y << ")\n";
    });

    return 0;
}
