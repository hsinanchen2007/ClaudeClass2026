// =============================================================================
//  15_trailing_return.cpp  —  Trailing return type (C++11)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/function
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、語法                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//      auto func(args) -> ReturnType { ... }
//
//  把 return type 從前面搬到 -> 後面。等價於：
//
//      ReturnType func(args) { ... }   // 傳統寫法
//
//  乍看只是搬位置 — 實際上有兩個重要場合會用到。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、場景 1：return type 依賴於參數型別                    │
//  └────────────────────────────────────────────────────────────┘
//
//      template <class T, class U>
//      auto add(T a, U b) -> decltype(a + b) { return a + b; }
//
//  傳統寫法做不到 — 你不能在「return type 位置」用 a、b（它們還沒宣告）。
//  Trailing return 把 return type 搬到「參數列之後」，所以可以用 decltype
//  存取 a、b。
//
//  C++14 起多數情況可省略：直接 auto add(T a, U b) { return a + b; } 讓
//  編譯器推導 — 見 C++_Cpp14/01_auto_return_type.cpp。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、場景 2：複雜 return type 的可讀性                     │
//  └────────────────────────────────────────────────────────────┘
//
//  傳統寫法：
//      std::vector<std::map<std::string, int>>::const_iterator getIt();
//
//  Trailing：
//      auto getIt() -> std::vector<std::map<std::string, int>>::const_iterator;
//
//  兩者一樣冗長 — 但 trailing 把「函式名 getIt」搬到前面，「return type」
//  在後，掃描時找到 getIt 較容易。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、場景 3：成員函式 return ref 到內部型別                 │
//  └────────────────────────────────────────────────────────────┘
//
//      class Container {
//          struct Iter { ... };
//      public:
//          auto begin() -> Iter;            // ✅ Iter 在 class 內部，
//          // Iter begin();                  //    傳統寫法要寫 Container::Iter
//      };
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 五、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：依賴參數型別 (decltype)
//   * Demo 2：return container's nested type
//   * Demo 3：lambda 的 -> return type
// =============================================================================

/*
補充筆記：trailing_return
  - trailing_return 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - trailing return type 讓回傳型別可使用參數名稱，常見於 template 中依表達式推導回傳。
  - C++14 後 auto return 可簡化許多情況，但複雜泛型仍可能需要 -> decltype(expr)。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】trailing return type（C++11）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. trailing return type 解決什麼問題？
//     答：回傳型別若要依賴參數，寫在前面時參數還沒進入作用域：
//           template<class T, class U> auto add(T a, U b) -> decltype(a+b);  // OK
//           // decltype(a+b) add(T a, U b);      // ❌ a、b 此時尚未宣告
//         把回傳型別移到參數列之後，就能在 decltype 裡使用參數名。
//     追問：C++14 之後還需要嗎？（需求大減——C++14 可直接寫 auto add(T a, U b)
//           讓編譯器推導。本機實測：該寫法在 -std=c++11 報 'add' function uses
//           'auto' type specifier without trailing return type，-std=c++14 才過）
//
// 🔥 Q2. C++14 有了 auto 回傳型別推導，trailing return 就完全沒用了嗎？
//     答：仍有幾處不可取代：(1) virtual 函式不能用推導的回傳型別（本機 g++
//         -std=c++14 實測報 virtual function cannot have deduced return type）；
//         (2) 想在回傳型別上做 SFINAE（-> decltype(t.foo())，讓不符合的候選被
//         剔除）；(3) 宣告與定義分離時，標頭檔明寫回傳型別對呼叫端更清楚——
//         auto 推導要求定義對呼叫端可見，不能只把宣告放 .h。
//
// Q3. 為什麼類別成員函式用 trailing return 寫起來比較短？
//     答：-> 右側已經進入「類別作用域」，可以直接使用類別內的巢狀型別名；寫在
//         前面則必須完整限定：
//           auto C::begin() -> Iter { ... }      // ✅ Iter 不必限定
//           C::Iter C::end() { ... }             // 前置寫法必須寫成 C::Iter
//         （本機 g++ -std=c++11 -pedantic-errors 實測：前置寫法只寫 Iter 會報
//         'Iter' does not name a type）
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <memory>
#include <string>
#include <vector>

template <class T, class U>
auto add(T a, U b) -> decltype(a + b) {
    return a + b;
}

class Container {
public:
    explicit Container(std::vector<int> v) : data_(std::move(v)) {}

    auto begin() -> std::vector<int>::iterator { return data_.begin(); }
    auto end()   -> std::vector<int>::iterator { return data_.end(); }

    // 也可以對「nested type」做 trailing return
    struct Stats { int min_, max_; };
    auto computeStats() const -> Stats {
        int mn = data_.front(), mx = data_.front();
        for (int v : data_) {
            if (v < mn) mn = v;
            if (v > mx) mx = v;
        }
        return {mn, mx};
    }

private:
    std::vector<int> data_;
};

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：跨型別運算
    // ─────────────────────────────────────────────────────────
    auto r1 = add(1, 2.5);                  // double
    auto r2 = add(std::string{"hi"}, "!");   // std::string
    std::cout << "[Demo1] add(1, 2.5) = " << r1 << '\n';
    std::cout << "[Demo1] add(\"hi\", \"!\") = " << r2 << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：自訂 container + nested type
    // ─────────────────────────────────────────────────────────
    Container c{{3, 1, 4, 1, 5, 9, 2, 6}};
    auto stats = c.computeStats();
    std::cout << "[Demo2] min=" << stats.min_
              << " max=" << stats.max_ << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 3：lambda 的 trailing return
    //   lambda 的 return type 多數能自動推導；明寫的時機：
    //   (a) 兩條路 return 不同型別需強制統一
    //   (b) 想刻意提升精度（ex: int → double）
    // ─────────────────────────────────────────────────────────
    auto half = [](int n) -> double {
        if (n % 2 == 0) return n / 2;     // int 會 promote 成 double
        return n / 2.0;
    };
    std::cout << "[Demo3] half(5) = " << half(5) << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：trailing return 跟「auto」函式（C++14）有什麼差？
    //    A：C++14 的「auto func() { ... }」會「自動推導」return type；
    //       trailing 是「明寫」 — 對 template 在 C++11 下是必要寫法。
    //       現代 C++（C++14+）多數情況用 auto 即可，trailing 用於需要
    //       decltype 表達相依型別的場合。
    //
    //  Q2：為什麼要 auto 而不是直接寫型別？
    //    A：對 trailing：函式宣告處先看到 auto，編譯器知道「等下會用 ->
    //       指出 return type」 — 純粹語法需要。
    //
    //  Q3：什麼時候絕對不能省略 trailing？
    //    A：(C++11 only) 要對相依型別做 decltype；C++14+ 多數情況都可省。
    //       但「跨 .h / .cpp 拆分宣告與定義」時，明寫 return type 對 .h
    //       讀者更清楚。
    //
    // ─────────────────────────────────────────────────────────
    // LeetCode 1480. Running Sum of 1d Array — trailing return 給泛型 prefix sum
    //   題意：給 nums，回傳 prefix sum 陣列。
    //   為什麼放這？示範 trailing return 配 decltype，prefixSum 能適應
    //                int/double 等不同 element 型別。
    // ─────────────────────────────────────────────────────────
    auto prefixSum = [](const std::vector<int>& nums) -> std::vector<int> {
        std::vector<int> out;
        out.reserve(nums.size());
        int s = 0;
        for (int v : nums) { s += v; out.push_back(s); }
        return out;
    };
    auto ps = prefixSum({1, 2, 3, 4});
    std::cout << "[LC1480] prefix:";
    for (int v : ps) std::cout << ' ' << v;
    std::cout << '\n';

    // ─────────────────────────────────────────────────────────
    // 實用範例：簡單 factory — 用 trailing return 表達工廠回傳 base 智能指標
    //   工作上常見：工廠函式回 base 指標，比明寫一長串 ptr 更可讀
    // ─────────────────────────────────────────────────────────
    struct Shape {
        virtual ~Shape() = default;
        virtual double area() const = 0;
    };
    struct Circle : Shape {
        double r;
        explicit Circle(double r_) : r(r_) {}
        double area() const override { return 3.14159265358979 * r * r; }
    };
    auto makeCircle = [](double r) -> std::unique_ptr<Shape> {
        // trailing return 明寫「我回 base 智能指標」
        return std::unique_ptr<Shape>(new Circle(r));
    };
    auto shape = makeCircle(2.0);
    std::cout << "[Demo4] circle r=2 area = " << shape->area() << '\n';

    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 15_trailing_return.cpp -o 15_trailing_return

// === 預期輸出 ===
// [Demo1] add(1, 2.5) = 3.5
// [Demo1] add("hi", "!") = hi!
// [Demo2] min=1 max=9
// [Demo3] half(5) = 2.5
// [LC1480] prefix: 1 3 6 10
// [Demo4] circle r=2 area = 12.5664
