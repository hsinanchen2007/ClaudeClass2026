// =============================================================================
//  06_initializer_list.cpp  —  std::initializer_list (C++11)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/utility/initializer_list
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、什麼是 std::initializer_list<T>？                     │
//  └────────────────────────────────────────────────────────────┘
//
//  一個輕量級的「同型別元素的 array view」。當你寫 {1, 2, 3} 這種 brace
//  enclosed list 在「會接受 initializer_list」的上下文中使用，編譯器自動
//  打包成 std::initializer_list<int>。
//
//  特性：
//   * 內含的元素是「const T」 — 只讀
//   * 有 begin() / end() / size() — 可以 range-for
//   * 拷貝極輕量（內部就是 const T* + size_t）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、用途 1：給自訂 class 寫 initializer_list ctor         │
//  └────────────────────────────────────────────────────────────┘
//
//      class MyVec {
//      public:
//          MyVec(std::initializer_list<int> il) {
//              for (int v : il) data_.push_back(v);
//          }
//          ...
//      };
//      MyVec v{1, 2, 3};
//
//  STL 容器都有這種 ctor — 這就是為何 vector<int> v{1, 2, 3} 能直接寫。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、用途 2：當函式參數 — 任意數量「同型別」引數           │
//  └────────────────────────────────────────────────────────────┘
//
//      template <class T>
//      T sum_all(std::initializer_list<T> il) {
//          T s{};
//          for (auto v : il) s += v;
//          return s;
//      }
//      sum_all({1, 2, 3, 4});       // → 10
//      sum_all<double>({1.5, 2.5}); // → 4.0
//
//  比 variadic templates 簡單，但要求所有引數同型別。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、跟 variadic template 的選擇                          │
//  └────────────────────────────────────────────────────────────┘
//
//   * 同型別任意數量 → initializer_list（語法簡單）
//   * 多型別任意數量 → variadic template
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 五、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：自訂 MyVec 接受 initializer_list
//   * Demo 2：sum_all 函式
//   * LeetCode 1431. Kids With the Greatest Number of Candies — 用 {} 直接傳
// =============================================================================

/*
補充筆記：initializer_list
  - initializer_list 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - initializer_list 內元素是 const，函式收到後不能修改元素本身。
  - initializer_list 只是視圖，若要長期保存內容，必須複製到 vector 或其他擁有資料的容器。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::initializer_list
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::initializer_list<T> 是什麼？和 vector 差在哪？
//     答：它是編譯器為 brace-enclosed list 產生的輕量唯讀「視圖」，內部大致就是
//         `const T*` + 長度，本身不擁有元素、拷貝極廉價。vector 則真正擁有並管理
//         堆積上的記憶體。所以 initializer_list 只適合當「傳入參數」用，要長期
//         保存內容必須複製到 vector 這類真正持有資料的容器（見本檔 MyVec）。
//     追問：能不能把既有的 array 轉成 initializer_list？（不行，它只能由 `{}`
//         字面量產生；要對既有記憶體做視圖，C++20 用 std::span）
//
// 🔥 Q2. 為什麼 initializer_list 和移動語意「不對盤」？
//     答：因為它的元素型別是 **const T**。`std::move(e)` 對 const 元素只會得到
//         `const T&&`，綁不上移動建構子的 `T&&`，卻能綁上拷貝建構子的 `const T&`
//         ——於是靜默退回拷貝。本機實測：在 initializer_list<T> 的建構子裡對元素
//         下 std::move，印出來的是 COPY 而不是 MOVE。
//     追問：那要支援 move-in 該怎麼寫？（改用 variadic template 做完美轉發，
//         或提供吃 `std::vector<T>&&` 的重載）
//
// ⚠️ 陷阱. `std::initializer_list<int> il = {1, 2, 3};` 之後把 il 存起來或回傳，安全嗎？
//     答：不安全。il 只是視圖，底層那個「暫時的元素陣列」的壽命跟一般臨時物件一樣
//         ——綁到具名變數時壽命被延長到該變數的作用域結束為止，但把 il 從函式
//         **回傳**、或存進生命期更長的成員，底層陣列早已消滅，留下懸垂指標。
//     為什麼會錯：initializer_list 長得像個「容器」，名字裡還有 list，讓人以為它
//         持有資料。它其實比較接近 string_view / span 那一類——是誰擁有資料，
//         誰就決定壽命，而它從來都不是擁有者。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <initializer_list>
#include <iostream>
#include <string>
#include <vector>

class MyVec {
public:
    MyVec(std::initializer_list<int> il) {
        data_.reserve(il.size());
        for (int v : il) data_.push_back(v);
    }
    void print(const std::string& tag) const {
        std::cout << tag;
        for (int v : data_) std::cout << ' ' << v;
        std::cout << '\n';
    }
private:
    std::vector<int> data_;
};

template <class T>
T sum_all(std::initializer_list<T> il) {
    T s{};
    for (auto v : il) s += v;
    return s;
}

// LC1431 helper：找最大值
static int maxOf(std::initializer_list<int> il) {
    return *std::max_element(il.begin(), il.end());
}

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：自訂類別吃 initializer_list
    // ─────────────────────────────────────────────────────────
    MyVec v{10, 20, 30, 40};
    v.print("[Demo1] MyVec:");

    // ─────────────────────────────────────────────────────────
    // Demo 2：泛型 sum_all
    // ─────────────────────────────────────────────────────────
    std::cout << "[Demo2] sum_all(int)    = " << sum_all({1, 2, 3, 4})         << '\n';
    std::cout << "[Demo2] sum_all(double) = " << sum_all<double>({1.5, 2.5, 3.5}) << '\n';

    // ─────────────────────────────────────────────────────────
    // LeetCode 1431（簡化版示範）
    //   題意之中我們經常寫 max_of({a, b, c}) 來省括號
    // ─────────────────────────────────────────────────────────
    int a = 7, b = 12, c = 5;
    std::cout << "[LC1431-ish] maxOf({" << a << "," << b << "," << c << "}) = "
              << maxOf({a, b, c}) << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：initializer_list 內元素 const？
    //    A：是。「const T」 — 不能在 ctor 內 std::move 出去。要支援 move
    //       要用 variadic template 或 vector<T>&&。這是 initializer_list
    //       與 perfect forwarding 不對盤的原因。
    //
    //  Q2：initializer_list 跟 array view 一樣嗎？
    //    A：概念類似（指標 + 大小），但 initializer_list 只能由「{} 字面
    //       量」產生 — 不能用 array 變數轉成 initializer_list。要從現存
    //       array 視為 view，C++20 用 std::span。
    //
    //  Q3：自訂 class 的 ctor 中，{} 跟 () 怎麼區分？
    //    A：{} 優先選 initializer_list ctor；如果有 initializer_list ctor
    //       但不匹配（型別不對），編譯失敗 — 不會去試「普通 ctor」。
    //       見 05 號檔的 vector{3, 5} vs vector(3, 5) 範例。
    //
    // ─────────────────────────────────────────────────────────
    // LeetCode 217. Contains Duplicate（用 initializer_list 一次塞入 set）
    //   題意：判斷 vector 內有沒有重複元素。
    //   為什麼放這？用 unordered_set 的「initializer_list ctor」一次把所有
    //                元素塞進去，若 set.size() < nums.size() 即代表有重複。
    // ─────────────────────────────────────────────────────────
    auto contains_duplicate = [](std::initializer_list<int> il) {
        // 把 il 整個丟給 set 的 ctor — 內部用 initializer_list ctor
        std::vector<int> tmp(il.begin(), il.end());
        std::sort(tmp.begin(), tmp.end());
        for (std::size_t i = 1; i < tmp.size(); ++i)
            if (tmp[i] == tmp[i - 1]) return true;
        return false;
    };
    std::cout << std::boolalpha;
    std::cout << "[LC217] {1,2,3,1} => " << contains_duplicate({1, 2, 3, 1}) << '\n'; // true
    std::cout << "[LC217] {1,2,3,4} => " << contains_duplicate({1, 2, 3, 4}) << '\n'; // false

    // ─────────────────────────────────────────────────────────
    // 實用範例：自訂 ColorSet — 一次傳多個顏色字串建立檢查器
    //   工作上常見：white-list / black-list 在 ctor 一次定義好
    // ─────────────────────────────────────────────────────────
    class AllowList {
    public:
        AllowList(std::initializer_list<std::string> il)
            : items_(il.begin(), il.end()) {}
        bool allowed(const std::string& s) const {
            for (const auto& x : items_) if (x == s) return true;
            return false;
        }
    private:
        std::vector<std::string> items_;
    };
    AllowList al{"GET", "POST", "PUT"};   // 一行表達白名單
    std::cout << "[Demo4] allowed(GET) = "    << al.allowed("GET") << '\n';
    std::cout << "[Demo4] allowed(DELETE) = " << al.allowed("DELETE") << '\n';

    return 0;
}
