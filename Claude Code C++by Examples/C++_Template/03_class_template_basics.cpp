// ============================================================================
//  03_class_template_basics.cpp  ──  Class Template 基礎
// ============================================================================
//
//  【本篇目標】
//    從 function template 走到 class template。重點：
//      - 語法 template<typename T> class Foo { ... };
//      - 成員函式定義：class 內部寫 vs class 外部寫
//      - Class template 需要顯式指定型別 (C++17 之前)；CTAD (C++17) 簡介
//      - Member function template (類別本身或成員是 template)
//      - 為什麼 class template 通常完整定義要放在 header
//
// ----------------------------------------------------------------------------
//  【語法骨架】
//
//        template <typename T>
//        class Box {
//        public:
//            Box(T v) : value_(v) {}
//            T get() const { return value_; }
//            void set(T v) { value_ = v; }
//        private:
//            T value_;
//        };
//
//        Box<int>    a(42);
//        Box<double> b(3.14);
//
//    使用時必須寫 Box<int> 而不是 Box(C++17 之前)；C++17 起有 CTAD：
//
//        Box b(42);                // 推導為 Box<int>（C++17）
//
//    CTAD = Class Template Argument Deduction，需要使用者自己定義
//    deduction guide 或讓編譯器自動推 (從 constructor 簽名)。
//
// ----------------------------------------------------------------------------
//  【成員函式定義位置】
//
//    在 class 裡面定義（小函式建議這樣，自動是 inline）：
//
//        template<typename T>
//        class Foo {
//        public:
//            T get() const { return v_; }
//        private:
//            T v_;
//        };
//
//    在 class 外面定義（長函式可這樣，但語法要重新宣告 template）：
//
//        template<typename T>
//        class Foo {
//        public:
//            T get() const;
//        private:
//            T v_;
//        };
//
//        template<typename T>
//        T Foo<T>::get() const { return v_; }
//
//    注意：函式名前要加 Foo<T>:: ── 因為 Foo 是 template，必須帶上參數。
//
// ----------------------------------------------------------------------------
//  【Member function template】
//    Class 本身可能不是 template，或本身是 template 但成員函式又有自己的
//    type 參數，例如：
//
//        template<typename T>
//        class Container {
//        public:
//            template<typename U>           // <- 成員自己的 template 參數
//            void copy_from(const Container<U>& other) { ... }
//        };
//
//    這在 file 24 (type erasure) 與 file 28 (mini STL) 會頻繁出現。
//
// ----------------------------------------------------------------------------
//  【為什麼放 header？】
//    Compiler 在實例化 Box<int> 時必須看到 Box 的完整定義 (才能生出 int
//    特化版的程式碼)。若把 Box.hpp 跟 Box.cpp 拆開且 Box.cpp 沒有 explicit
//    instantiation，使用者 link 時會找不到 Box<int>::xxx 的符號。
//    詳見 file 25_template_compilation_model.cpp。
//
//  參考：https://en.cppreference.com/cpp/language/class_template
// ============================================================================

/*
補充筆記：class_template_basics
  - class_template_basics 涉及模板實例化；請先判斷哪些型別由呼叫端推導，哪些型別由程式指定。
  - 泛型程式要把型別需求寫清楚，例如可比較、可移動、可呼叫或符合某個 concept。
  - 模板技巧的價值在減少重複且保留型別安全，不是讓錯誤訊息變得更長。
  - class_template_basics 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/
#include <iostream>
#include <vector>
#include <stack>
#include <string>
#include <stdexcept>

// ─── 1. 最簡單的 class template：Box<T> ───────────────────────────────────
//   一個型別安全的「值容器」(只是包一個 T)。
//   類似 std::optional 但沒有 empty 狀態，只是教學用最小範例。
template <typename T>
class Box {
public:
    Box() = default;                      // 預設建構：T 也預設建構
    explicit Box(T v) : value_(v) {}      // 帶值建構

    const T& get() const { return value_; }
    T&       get()       { return value_; }
    void     set(const T& v) { value_ = v; }

private:
    T value_{};                           // T{} = value-initialize
};

// ─── 2. 工作實用範例：Pair<K,V> ───────────────────────────────────────────
//   類似 std::pair 的最小版。
//   用兩個型別參數，K 和 V 可以不同。
template <typename K, typename V>
class Pair {
public:
    Pair() = default;
    Pair(K k, V v) : key(std::move(k)), value(std::move(v)) {}

    K key{};
    V value{};
};

// ─── 3. Leetcode 155 ── Min Stack (template 化) ───────────────────────────
//   原題：設計 stack 支援 push/pop/top/getMin，所有操作都是 O(1)。
//   原題型別是 int，這裡 template 化讓它可用於任何「可比較」型別。
//
//   解法核心：用兩個 stack
//     - main_   ：正常存 push 的值
//     - mins_   ：每次 push 時，存「目前為止的最小值」
//   pop 時兩邊一起 pop。getMin 直接看 mins_.top()。
//
//   時間複雜度：所有操作 O(1)
//   空間複雜度：O(n)（mins_ 最壞情況跟 main_ 一樣大）
//
//   邊界條件：
//     - empty 時呼叫 top()/getMin()/pop() → 丟 std::out_of_range
//     - 重複最小值 → 必須在 mins_ 也 push 一份，否則 pop 會錯亂
template <typename T>
class MinStack {
public:
    void push(const T& v) {
        main_.push(v);
        if (mins_.empty() || v <= mins_.top()) mins_.push(v);
        else                                   mins_.push(mins_.top());
        // 上面這個寫法比「只在 v <= min 時 push」乾淨：mins_ 永遠跟 main_
        // 一樣高，pop 邏輯只要兩邊各 pop 一次就好。雖然多花一些空間，但
        // O(n) 與原本一樣，邏輯更不容易錯。
    }

    void pop() {
        if (main_.empty()) throw std::out_of_range("MinStack::pop on empty");
        main_.pop();
        mins_.pop();
    }

    const T& top() const {
        if (main_.empty()) throw std::out_of_range("MinStack::top on empty");
        return main_.top();
    }

    const T& getMin() const {
        if (mins_.empty()) throw std::out_of_range("MinStack::getMin on empty");
        return mins_.top();
    }

    bool empty() const { return main_.empty(); }
    std::size_t size() const { return main_.size(); }

private:
    std::stack<T> main_;
    std::stack<T> mins_;
};

// ─── 4. 成員函式 template：示範 Box 加上 cast<U>() ───────────────────────
//   member function template 範例：把 Box<T> 內部的 T 轉成 U。
//   這在 file 24 (type erasure) 也是核心技巧之一。
template <typename T>
class Box2 {
public:
    explicit Box2(T v) : value_(v) {}

    template <typename U>
    Box2<U> cast() const {                // 成員自己也是 template
        return Box2<U>(static_cast<U>(value_));
    }

    T value_;
};

// ─── 5. Leetcode 1480 ── Running Sum (class template 版「累加器」) ───────
//   難度: easy
//   題目：給 nums，回傳前綴和 result[i] = nums[0]+...+nums[i]。
//
//   為什麼放在這裡？
//     用 class template 「Accumulator<T>」 包裝狀態：每呼叫 add 一次累加並
//     回傳當下總和，比 free function 更貼近物件導向風格。
//
//   時間：O(n)；空間：O(n)。
template <typename T>
class Accumulator {
public:
    T add(const T& v) { sum_ += v; return sum_; }
    T total() const   { return sum_; }
    void reset()      { sum_ = T{}; }
private:
    T sum_{};
};

template <typename T>
std::vector<T> running_sum(const std::vector<T>& nums) {
    Accumulator<T> acc;
    std::vector<T> r;
    r.reserve(nums.size());
    for (const T& x : nums) r.push_back(acc.add(x));
    return r;
}

// ─── 6. 工作實用：通用 Timer 類別 ──────────────────────────────────────
//   日常需求：量某段函式跑多久。class template 讓「時間單位」可選 (秒、
//   毫秒、微秒)。Duration 預設 std::chrono::milliseconds。
#include <chrono>
template <typename Duration = std::chrono::milliseconds>
class Timer {
public:
    Timer() : start_(std::chrono::steady_clock::now()) {}
    typename Duration::rep elapsed() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<Duration>(now - start_).count();
    }
    void reset() { start_ = std::chrono::steady_clock::now(); }
private:
    std::chrono::steady_clock::time_point start_;
};

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    // (1) Box
    Box<int>    a(42);
    Box<double> b(3.14);
    Box<std::string> c(std::string("hello"));
    std::cout << "Box<int>::get()    = " << a.get() << "\n";
    std::cout << "Box<double>::get() = " << b.get() << "\n";
    std::cout << "Box<string>::get() = " << c.get() << "\n";

    // (2) Pair
    Pair<std::string, int> age("Alice", 30);
    std::cout << "Pair: " << age.key << " -> " << age.value << "\n";

    // (3) Leetcode 155
    MinStack<int> ms;
    ms.push(3);
    ms.push(5);
    ms.push(2);
    ms.push(2);                         // 故意製造重複最小
    ms.push(4);
    std::cout << "MinStack getMin = " << ms.getMin() << " (expect 2)\n";
    ms.pop();                           // pop 4
    ms.pop();                           // pop 2 (一份)
    std::cout << "MinStack getMin = " << ms.getMin() << " (expect 2)\n";
    ms.pop();                           // pop 2 (再一份)
    std::cout << "MinStack getMin = " << ms.getMin() << " (expect 3)\n";

    // (4) member function template
    Box2<int> i(7);
    auto d = i.cast<double>();          // U 推不出來時也可顯式指定
    std::cout << "Box2.cast<double>() = " << d.value_ << "\n";

    // (5) Accumulator + Running Sum
    auto rs = running_sum(std::vector<int>{1, 2, 3, 4});
    std::cout << "running_sum: ";
    for (int x : rs) std::cout << x << ' ';
    std::cout << "\n";

    // (6) Timer
    Timer<std::chrono::microseconds> t;
    long long s = 0;
    for (int k = 0; k < 100000; ++k) s += k;   // 做點事情
    std::cout << "Timer elapsed (us) = " << t.elapsed() << ", sum=" << s << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：class template 的成員函式什麼時候會被 instantiate？
    //    A：「按需 (on-demand)」實例化。建立 Box<int> 物件時，只有實際
    //       被呼叫到的成員函式才會被生成程式碼；沒被使用的成員函式即使
    //       對 T 不合法 (例如 T 沒有 operator<) 也不會編譯失敗。這個特
    //       性可以拿來實作「optional 行為」，但相對的，純粹宣告還沒呼
    //       叫時錯誤不會暴露，需要寫測試或 explicit instantiation 來驗證。
    //
    //  Q2：CTAD (C++17) 的 deduction guide 何時需要自己寫？
    //    A：當 constructor 的參數型別跟 class template 參數的對應關係不
    //       直觀時。例如 `template<typename T> class Wrap { Wrap(T*); };`
    //       直接呼叫 Wrap(p) 編譯器會推 T = int*，但你想要 T = int。
    //       這時要寫 `template<typename T> Wrap(T*) -> Wrap<T>;` 來明
    //       確指引推導結果。STL 的 std::vector、std::pair 都有對應 guide。
    //
    //  Q3：為什麼 Box<T>::get() 在 class 外定義時要寫 `Box<T>::`？
    //    A：因為 Box 是 template，不是具體型別；要先把它「鎖定」在
    //       template parameter T 上才有意義。語法上「先寫 template<typename T>
    //       重新引入 T，再寫 Box<T>::get」是規定。如果寫成 `Box::get`
    //       會被認定指涉特化版或語法錯誤。
    //
    return 0;
}

// ============================================================================
//  【小結】
//    1. 用 template<typename T> class Foo { ... }; 宣告 class template。
//    2. 使用時 (C++17 之前) 必須顯式指定型別：Foo<int> x;
//       C++17 起 CTAD 可以由 constructor 推導：Foo x(42);
//    3. 成員可以是 template；class 是 template 的同時，成員可以再帶自己
//       的 template 參數。
//    4. 通常完整定義放在 header；原因 file 25 詳述。
//
//  【下一篇】
//    04_template_parameters.cpp ── Template 參數的三種類型
//    (type / non-type / template template)。
// ============================================================================
