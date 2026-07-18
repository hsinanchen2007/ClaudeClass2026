/*
 * ============================================================
 * 【第二課：泛型編程（Generic Programming）概念】總複習 summary.cpp
 * ============================================================
 * 本課程重點：
 * 1. 為什麼需要泛型編程？（從重複程式碼談起）
 * 2. 函數重載（Function Overloading）的局限性
 * 3. 函數模板（Function Template）的語法與使用
 * 4. 類別模板（Class Template）的語法與使用
 * 5. 模板特化（Template Specialization）
 * 6. 模板中的型別推導（Type Deduction）
 * 7. 型別約束（Concepts）簡介（C++20）
 * 8. 非型別模板參數（Non-type Template Parameters）
 * ============================================================
 */

#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <type_traits>


// ===== 重點一：為什麼需要泛型編程？=====
// 問題：不使用泛型，同一個邏輯需要為每種型別重複寫一次

// 不好的做法（重複程式碼）：
int max_int(int a, int b) { return a > b ? a : b; }
double max_double(double a, double b) { return a > b ? a : b; }
// 如果要支援 float, long, char... 需要繼續重複寫！

// ===== 重點二：函數重載（解決部分問題，但仍有重複）=====
// 函數重載允許同名函數有不同的參數型別
// 但每個版本的函數體幾乎相同，邏輯上是重複的

// ===== 重點三：函數模板（Function Template）=====
// 語法：template <typename T> 返回型別 函數名稱(參數列表) { ... }
// 作用：讓編譯器根據實際使用的型別自動生成對應的函數
// 關鍵字 typename 和 class 在模板中等效，但 typename 更清晰表示「這是一個型別」

// 基本函數模板：
template <typename T>
T my_max(T a, T b) {
    // T 是一個佔位符（placeholder），代表「某種型別」
    // 當你呼叫 my_max(3, 5) 時，編譯器把 T 替換成 int
    // 當你呼叫 my_max(3.14, 2.72) 時，編譯器把 T 替換成 double
    return a > b ? a : b;
}

// 多個型別參數的模板：
template <typename T, typename U>
void print_pair(T first, U second) {
    std::cout << "(" << first << ", " << second << ")" << std::endl;
}

// 模板函數也可以有非模板參數：
template <typename T>
T clamp(T value, T min_val, T max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}


// ===== 重點四：類別模板（Class Template）=====
// 類別模板讓容器類別可以儲存任意型別的元素
// 這是 STL 容器（vector, list, set...）的實作基礎！

// 簡單的泛型 pair 類別：
template <typename First, typename Second>
class MyPair {
public:
    First first;
    Second second;

    // 建構子：初始化兩個成員
    MyPair(First f, Second s) : first(f), second(s) {}

    // 輸出方法
    void print() const {
        std::cout << "MyPair(" << first << ", " << second << ")" << std::endl;
    }
};

// 簡單的泛型 Stack（堆疊）：
template <typename T>
class MyStack {
private:
    std::vector<T> data_;  // 內部使用 vector 儲存元素

public:
    // push：放入元素（放在頂端）
    void push(const T& value) {
        data_.push_back(value);
    }

    // pop：取出頂端元素（後進先出 LIFO）
    void pop() {
        if (!data_.empty()) {
            data_.pop_back();
        }
    }

    // top：查看頂端元素（不移除）
    T& top() {
        return data_.back();
    }

    const T& top() const {
        return data_.back();
    }

    // empty：檢查是否為空
    bool empty() const {
        return data_.empty();
    }

    // size：元素個數
    size_t size() const {
        return data_.size();
    }
};

// 固定大小的泛型陣列（使用非型別模板參數）：
template <typename T, int N>
class FixedArray {
private:
    T data_[N];  // 大小在編譯期固定

public:
    FixedArray() {
        for (int i = 0; i < N; ++i) {
            data_[i] = T{};  // 值初始化
        }
    }

    T& operator[](int index) {
        return data_[index];
    }

    const T& operator[](int index) const {
        return data_[index];
    }

    int size() const { return N; }

    void print() const {
        std::cout << "FixedArray<" << N << ">: ";
        for (int i = 0; i < N; ++i) {
            std::cout << data_[i] << " ";
        }
        std::cout << std::endl;
    }
};


// ===== 重點五：模板特化（Template Specialization）=====
// 問題：有時候通用模板對特定型別的行為不對，需要提供「特化版本」
// 例如：比較 C 字串（char*）不能用 > 運算子（那是比較指標地址）

// 通用版本（適用大多數型別）：
template <typename T>
bool is_equal(T a, T b) {
    return a == b;
}

// 針對 const char* 的完全特化版本：
template <>
bool is_equal<const char*>(const char* a, const char* b) {
    // 字串比較需要用 strcmp，不能用 ==（== 比較的是指標地址）
    return std::string(a) == std::string(b);
}

// 部分特化（Partial Specialization）只能用於類別模板：
template <typename T>
class TypeInfo {
public:
    static void print() {
        std::cout << "一般型別" << std::endl;
    }
};

// 針對指標型別的部分特化：
template <typename T>
class TypeInfo<T*> {
public:
    static void print() {
        std::cout << "指標型別" << std::endl;
    }
};

// 針對 int 的完全特化：
template <>
class TypeInfo<int> {
public:
    static void print() {
        std::cout << "整數型別（int）" << std::endl;
    }
};


// ===== 重點六：模板中的型別推導（Type Deduction）=====
// C++ 編譯器可以從函數呼叫的參數自動推導模板參數型別
// 不需要顯式指定型別（但可以顯式指定）

template <typename T>
void show_type(T value) {
    // typeid(T).name() 可以取得型別名稱（實作相依，僅供參考）
    std::cout << "值: " << value
              << " | 型別（概略）: " << typeid(T).name() << std::endl;
}

// auto 與型別推導（C++11 起）：
template <typename T, typename U>
auto add(T a, U b) -> decltype(a + b) {
    // 尾置返回型別（trailing return type）
    // decltype(a + b) 讓編譯器推導 a + b 的型別作為返回型別
    return a + b;
}


// ===== 重點七：Concepts（C++20 型別約束）=====
// 問題：傳統模板的錯誤訊息非常難以閱讀
// Concepts 讓你可以在模板上加入「約束」，提供更清晰的錯誤訊息

// 使用 static_assert 模擬約束（C++11 方式）：
template <typename T>
T safe_sqrt(T value) {
    static_assert(std::is_floating_point<T>::value,
                  "safe_sqrt 只接受浮點型別（float 或 double）！");
    // 真實情境中應該 #include <cmath> 並使用 std::sqrt
    return value;  // 簡化示範
}

// 使用 std::enable_if（C++11 SFINAE 技巧）：
// SFINAE = Substitution Failure Is Not An Error（替換失敗不是錯誤）
template <typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type
only_for_integers(T value) {
    std::cout << "這個函數只接受整數！值：" << value << std::endl;
    return value;
}


// ===== 重點八：非型別模板參數（Non-type Template Parameters）=====
// 模板參數不只可以是「型別」，也可以是「值」
// 常用於在編譯期確定陣列大小、緩衝區大小等

// 示範：編譯期計算陣列大小
template <typename T, size_t N>
size_t array_size(T (&arr)[N]) {
    // 這個函數接受一個 C 陣列的引用，從型別中提取大小 N
    return N;
}

// 編譯期計算（constexpr 函數）：
template <int N>
struct Factorial {
    // 使用模板遞迴在編譯期計算階乘
    static const int value = N * Factorial<N - 1>::value;
};

template <>
struct Factorial<0> {
    // 遞迴基底：0! = 1
    static const int value = 1;
};


// ===== 型別推導與 auto 的關係 =====
// auto 在 C++11 中引入，讓編譯器自動推導變數型別
// auto 的推導規則與函數模板的型別推導規則相同

void demo_auto_deduction() {
    auto x = 42;          // x 是 int
    auto y = 3.14;        // y 是 double
    auto z = "hello";     // z 是 const char*
    auto v = std::vector<int>{1, 2, 3};  // v 是 vector<int>

    std::cout << "auto x: " << x << std::endl;
    std::cout << "auto y: " << y << std::endl;
    std::cout << "auto z: " << z << std::endl;
    std::cout << "auto v size: " << v.size() << std::endl;
}


int main() {
    std::cout << "====================================================" << std::endl;
    std::cout << "  第二課：泛型編程（Generic Programming）概念 - 總複習示範" << std::endl;
    std::cout << "====================================================" << std::endl;

    // --- 示範一：函數模板基本使用 ---
    std::cout << "\n--- 示範一：函數模板（my_max）---" << std::endl;
    std::cout << "my_max(3, 5): " << my_max(3, 5) << std::endl;
    std::cout << "my_max(3.14, 2.72): " << my_max(3.14, 2.72) << std::endl;
    std::cout << "my_max('a', 'z'): " << my_max('a', 'z') << std::endl;
    std::cout << "my_max(string, string): "
              << my_max(std::string("apple"), std::string("banana")) << std::endl;

    // --- 示範二：多型別參數 ---
    std::cout << "\n--- 示範二：多型別參數模板 ---" << std::endl;
    print_pair(42, 3.14);
    print_pair("hello", 100);
    print_pair(true, 'A');

    // --- 示範三：clamp 函數模板 ---
    std::cout << "\n--- 示範三：clamp（限制在範圍內）---" << std::endl;
    std::cout << "clamp(15, 0, 10): " << clamp(15, 0, 10) << std::endl;
    std::cout << "clamp(-5, 0, 10): " << clamp(-5, 0, 10) << std::endl;
    std::cout << "clamp(5, 0, 10): " << clamp(5, 0, 10) << std::endl;

    // --- 示範四：類別模板 ---
    std::cout << "\n--- 示範四：類別模板（MyPair）---" << std::endl;
    MyPair<int, std::string> p1(42, "Hello");
    MyPair<double, bool> p2(3.14, true);
    p1.print();
    p2.print();

    // --- 示範五：泛型 Stack ---
    std::cout << "\n--- 示範五：泛型 Stack ---" << std::endl;
    MyStack<int> int_stack;
    int_stack.push(10);
    int_stack.push(20);
    int_stack.push(30);
    std::cout << "Stack 頂端: " << int_stack.top() << std::endl;
    int_stack.pop();
    std::cout << "pop 後頂端: " << int_stack.top() << std::endl;
    std::cout << "Stack 大小: " << int_stack.size() << std::endl;

    // 字串的 Stack
    MyStack<std::string> str_stack;
    str_stack.push("world");
    str_stack.push("hello");
    std::cout << "字串 Stack 頂端: " << str_stack.top() << std::endl;

    // --- 示範六：非型別模板參數（FixedArray）---
    std::cout << "\n--- 示範六：非型別模板參數（FixedArray）---" << std::endl;
    FixedArray<int, 5> fixed_arr;
    for (int i = 0; i < fixed_arr.size(); ++i) {
        fixed_arr[i] = (i + 1) * 10;
    }
    fixed_arr.print();

    // --- 示範七：模板特化 ---
    std::cout << "\n--- 示範七：模板特化 ---" << std::endl;
    std::cout << "is_equal(5, 5): " << (is_equal(5, 5) ? "true" : "false") << std::endl;
    std::cout << "is_equal(\"hello\", \"hello\"): "
              << (is_equal("hello", "hello") ? "true" : "false") << std::endl;

    TypeInfo<int>::print();          // 使用完全特化版本
    TypeInfo<double>::print();       // 使用通用版本
    TypeInfo<int*>::print();         // 使用指標特化版本

    // --- 示範八：型別推導 ---
    std::cout << "\n--- 示範八：型別推導 ---" << std::endl;
    show_type(42);
    show_type(3.14);
    show_type('A');
    show_type(std::string("STL"));

    auto result = add(3, 4.5);
    std::cout << "add(3, 4.5) = " << result << std::endl;

    // --- 示範九：auto 型別推導 ---
    std::cout << "\n--- 示範九：auto 型別推導 ---" << std::endl;
    demo_auto_deduction();

    // --- 示範十：編譯期計算 ---
    std::cout << "\n--- 示範十：編譯期計算（Factorial）---" << std::endl;
    std::cout << "5! = " << Factorial<5>::value << std::endl;
    std::cout << "10! = " << Factorial<10>::value << std::endl;
    // 這些值在編譯期就確定了，執行期沒有任何計算！

    std::cout << "\n====================================================" << std::endl;
    std::cout << "重點整理：" << std::endl;
    std::cout << "  1. 泛型編程解決了重複程式碼的問題" << std::endl;
    std::cout << "  2. 函數模板：template<typename T> 讓函數可以處理多種型別" << std::endl;
    std::cout << "  3. 類別模板：template<typename T> class ... 讓容器可以裝任何型別" << std::endl;
    std::cout << "  4. 模板特化：為特定型別提供不同的實作" << std::endl;
    std::cout << "  5. 非型別模板參數：模板可以接受值（如整數）作為參數" << std::endl;
    std::cout << "  6. 型別推導：auto 和模板推導讓程式碼更簡潔" << std::endl;
    std::cout << "  7. STL 容器和演算法都是類別模板和函數模板" << std::endl;
    std::cout << "====================================================" << std::endl;

    return 0;
}
