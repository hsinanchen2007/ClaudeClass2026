/*
 * ============================================================
 * 【第 8 課：函數物件（Function Object）初探】總複習 summary.cpp
 * ============================================================
 * 本課程重點：
 * 1. 函數物件（Functor）的定義與原理：重載 operator() 的類別
 * 2. 函數物件 vs 普通函數：狀態攜帶能力的差異
 * 3. STL 內建函數物件：<functional> 中的算術、比較、邏輯類
 * 4. Lambda 表達式：現代 C++ 的就地函數物件
 * 5. Lambda 的捕獲機制：值捕獲、參考捕獲、混合捕獲
 * 6. mutable Lambda：允許修改值捕獲的副本
 * 7. 泛型 Lambda（C++14）：使用 auto 參數
 * 8. std::function：通用可呼叫物件包裝器
 * ============================================================
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>
#include <numeric>
#include <string>
#include <optional>

// ===== 重點一：函數物件（Functor）的基本定義 =====
// 函數物件是「重載了 operator() 的類別」的實例
// 它可以像函數一樣被呼叫，但本質上是一個物件
// 優勢：可以攜帶狀態（透過成員變數）

// 最基本的函數物件示範
class Adder {
public:
    // 關鍵：重載 operator()，讓物件可以像函數一樣被呼叫
    int operator()(int a, int b) const {
        return a + b;
    }
};

// 帶有狀態的函數物件：閾值過濾器
// 普通函數做不到「記住」一個可配置的閾值
class GreaterThan {
private:
    int threshold_;  // 狀態：被攜帶的閾值
public:
    GreaterThan(int t) : threshold_(t) {}

    bool operator()(int x) const {
        return x > threshold_;  // 使用存儲的狀態進行判斷
    }
};

// 帶有狀態的函數物件：整除判斷器
class IsDivisibleBy {
private:
    int divisor_;
public:
    IsDivisibleBy(int d) : divisor_(d) {}

    bool operator()(int x) const {
        return x % divisor_ == 0;
    }
};

// 完整結構的函數物件：計數器（示範狀態累積）
class Counter {
private:
    int count_;   // 狀態：計數器（隨呼叫累積）
    int target_;  // 狀態：目標值
public:
    Counter(int target) : count_(0), target_(target) {}

    bool operator()(int x) {
        if (x == target_) {
            ++count_;
            return true;
        }
        return false;
    }

    // 存取累積的狀態
    int get_count() const { return count_; }
};


// ===== 重點二：STL 內建函數物件（#include <functional>）=====
/*
 * 算術類：
 *   std::plus<T>        → x + y
 *   std::minus<T>       → x - y
 *   std::multiplies<T>  → x * y
 *   std::divides<T>     → x / y
 *   std::modulus<T>     → x % y
 *   std::negate<T>      → -x
 *
 * 比較類：
 *   std::equal_to<T>      → x == y
 *   std::not_equal_to<T>  → x != y
 *   std::greater<T>       → x > y
 *   std::less<T>          → x < y
 *   std::greater_equal<T> → x >= y
 *   std::less_equal<T>    → x <= y
 *
 * 邏輯類：
 *   std::logical_and<T>   → x && y
 *   std::logical_or<T>    → x || y
 *   std::logical_not<T>   → !x
 *
 * 最常用的場景：
 *   std::sort(v.begin(), v.end(), std::greater<int>());  // 降序排序
 *   std::accumulate(v.begin(), v.end(), 1, std::multiplies<int>());  // 連乘
 */


// ===== 重點三：Lambda 表達式 =====
// Lambda 是 C++11 引入的「就地」定義函數物件的語法糖
// 編譯器會自動將 Lambda 轉換成匿名類別

/*
 * Lambda 基本語法：
 * [捕獲列表](參數列表) -> 回傳型別 { 函數主體 }
 *
 * 捕獲列表說明：
 *   []        → 不捕獲任何外部變數
 *   [x]       → 值捕獲 x（複製一份）
 *   [&x]      → 參考捕獲 x（直接操作外部變數）
 *   [=]       → 值捕獲所有外部變數
 *   [&]       → 參考捕獲所有外部變數
 *   [=, &x]   → 預設值捕獲，但 x 用參考捕獲
 *   [&, x]    → 預設參考捕獲，但 x 用值捕獲
 */


// ===== 重點四：Lambda 等價於手寫函數物件 =====
// Lambda 就是編譯器自動生成的函數物件類別

// 手寫的函數物件
class GreaterThanFunctor {
private:
    int threshold_;
public:
    GreaterThanFunctor(int t) : threshold_(t) {}
    bool operator()(int n) const { return n > threshold_; }
};

// 等價的 Lambda（編譯器自動生成類似的匿名類別）:
// [threshold](int n) { return n > threshold; }


// ===== 重點五：std::function 通用包裝器 =====
// std::function<回傳型別(參數型別...)> 可以儲存：
//   1. 普通函數
//   2. 函數物件（Functor）
//   3. Lambda 表達式
// 用途：需要在執行時期動態切換可呼叫物件

int add(int a, int b) { return a + b; }

class Multiplier {
public:
    int operator()(int a, int b) const { return a * b; }
};


// ===== 實戰應用：商品管理 =====
struct Product {
    std::string name;
    double price;
    int quantity;
};


int main() {
    // ---- 示範一：函數物件基礎 ----
    std::cout << "===== 函數物件基礎 =====" << std::endl;

    Adder add_obj;
    // add_obj(3, 5) 等同於 add_obj.operator()(3, 5)
    std::cout << "Adder(3, 5) = " << add_obj(3, 5) << std::endl;  // 8

    // ---- 示範二：帶狀態的函數物件 ----
    std::cout << "\n===== 帶狀態的函數物件 =====" << std::endl;
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // 普通函數做不到這一點：傳入不同的閾值
    int count5 = std::count_if(vec.begin(), vec.end(), GreaterThan(5));
    int count7 = std::count_if(vec.begin(), vec.end(), GreaterThan(7));
    std::cout << "大於 5 的個數: " << count5 << std::endl;  // 5
    std::cout << "大於 7 的個數: " << count7 << std::endl;  // 3

    int div3 = std::count_if(vec.begin(), vec.end(), IsDivisibleBy(3));
    std::cout << "3 的倍數個數: " << div3 << std::endl;  // 3

    // ---- 示範三：狀態累積函數物件 ----
    std::cout << "\n===== 狀態累積函數物件 =====" << std::endl;
    std::vector<int> data = {1, 2, 3, 2, 4, 2, 5, 2, 6};
    Counter counter(2);
    // for_each 回傳函數物件的副本，需要用變數接收
    Counter result = std::for_each(data.begin(), data.end(), counter);
    std::cout << "2 出現的次數: " << result.get_count() << std::endl;  // 4

    // ---- 示範四：STL 內建函數物件 ----
    std::cout << "\n===== STL 內建算術函數物件 =====" << std::endl;
    std::plus<int> plus_obj;
    std::multiplies<int> mult_obj;
    std::cout << "plus(3, 5) = " << plus_obj(3, 5) << std::endl;        // 8
    std::cout << "multiplies(4, 6) = " << mult_obj(4, 6) << std::endl;  // 24

    // 計算連乘積（1*2*3*4*5 = 120）
    std::vector<int> nums = {1, 2, 3, 4, 5};
    int product = std::accumulate(nums.begin(), nums.end(), 1,
                                  std::multiplies<int>());
    std::cout << "1*2*3*4*5 = " << product << std::endl;  // 120

    std::cout << "\n===== STL 內建比較函數物件 =====" << std::endl;
    std::vector<int> sortable = {5, 2, 8, 1, 9, 3};

    // 使用 std::greater<int>() 進行降序排序
    std::sort(sortable.begin(), sortable.end(), std::greater<int>());
    std::cout << "降序: ";
    for (int n : sortable) std::cout << n << " ";  // 9 8 5 3 2 1
    std::cout << std::endl;

    // 使用 std::less<int>() 進行升序排序（預設行為）
    std::sort(sortable.begin(), sortable.end(), std::less<int>());
    std::cout << "升序: ";
    for (int n : sortable) std::cout << n << " ";  // 1 2 3 5 8 9
    std::cout << std::endl;

    // ---- 示範五：Lambda 基本用法 ----
    std::cout << "\n===== Lambda 基本用法 =====" << std::endl;
    std::vector<int> lv = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // 不捕獲任何變數的 Lambda
    auto print = [](int n) { std::cout << n << " "; };
    std::for_each(lv.begin(), lv.end(), print);
    std::cout << std::endl;

    // 帶回傳值的 Lambda
    auto square = [](int n) { return n * n; };
    std::cout << "5 的平方: " << square(5) << std::endl;  // 25

    // 統計偶數個數
    int even_count = std::count_if(lv.begin(), lv.end(),
        [](int n) { return n % 2 == 0; });
    std::cout << "偶數個數: " << even_count << std::endl;  // 5

    // ---- 示範六：Lambda 捕獲機制 ----
    std::cout << "\n===== Lambda 值捕獲 =====" << std::endl;
    int threshold = 5;

    // 值捕獲：Lambda 內部有 threshold 的副本
    int count_val = std::count_if(lv.begin(), lv.end(),
        [threshold](int n) { return n > threshold; });
    std::cout << "大於 " << threshold << " 的個數: " << count_val << std::endl;

    // 值捕獲 + mutable：允許修改副本（不影響外部）
    auto by_value = [threshold]() mutable {
        threshold = 100;  // 只修改 Lambda 內的副本
        return threshold;
    };
    by_value();
    std::cout << "外部 threshold 仍然是: " << threshold << std::endl;  // 5（未改變）

    std::cout << "\n===== Lambda 參考捕獲 =====" << std::endl;
    int sum = 0;
    // 參考捕獲：Lambda 直接修改外部的 sum
    std::for_each(lv.begin(), lv.end(), [&sum](int n) { sum += n; });
    std::cout << "總和: " << sum << std::endl;  // 55

    // 混合捕獲：[=, &b] 預設值捕獲，b 用參考捕獲
    std::cout << "\n===== 混合捕獲 =====" << std::endl;
    int a = 10, b = 20, c = 30;
    auto lambda1 = [=, &b]() {
        b = a + c;  // a 和 c 是副本，b 是參考，可以修改 b
    };
    lambda1();
    std::cout << "a=" << a << ", b=" << b << ", c=" << c << std::endl;
    // a=10, b=40, c=30（只有 b 被修改）

    // ---- 示範七：mutable Lambda ----
    std::cout << "\n===== mutable Lambda =====" << std::endl;
    int counter_var = 0;
    // 沒有 mutable：不能修改值捕獲的變數
    // auto bad = [counter_var]() { return ++counter_var; };  // 編譯錯誤！

    // 加上 mutable：可以修改副本
    auto increment = [counter_var]() mutable {
        return ++counter_var;  // 修改的是 Lambda 內部的副本
    };
    std::cout << "increment(): " << increment() << std::endl;  // 1
    std::cout << "increment(): " << increment() << std::endl;  // 2（Lambda 記住了狀態）
    std::cout << "increment(): " << increment() << std::endl;  // 3
    std::cout << "外部 counter_var: " << counter_var << std::endl;  // 0（未被修改）

    // ---- 示範八：泛型 Lambda（C++14）----
    std::cout << "\n===== 泛型 Lambda（C++14）=====" << std::endl;
    // auto 參數：同一個 Lambda 可用於不同型別
    auto generic_print = [](const auto& x) { std::cout << x << " "; };

    std::vector<int> int_vec = {1, 2, 3};
    std::vector<double> dbl_vec = {1.1, 2.2, 3.3};
    std::vector<std::string> str_vec = {"Hello", "World"};

    std::cout << "ints: ";
    std::for_each(int_vec.begin(), int_vec.end(), generic_print);
    std::cout << std::endl;

    std::cout << "doubles: ";
    std::for_each(dbl_vec.begin(), dbl_vec.end(), generic_print);
    std::cout << std::endl;

    std::cout << "strings: ";
    std::for_each(str_vec.begin(), str_vec.end(), generic_print);
    std::cout << std::endl;

    // ---- 示範九：std::function 包裝器 ----
    std::cout << "\n===== std::function 通用包裝器 =====" << std::endl;
    // std::function<int(int, int)> 可以儲存任何接受兩個 int 回傳 int 的可呼叫物件
    std::function<int(int, int)> func;

    func = add;          // 儲存普通函數
    std::cout << "普通函數 add(3,5) = " << func(3, 5) << std::endl;  // 8

    func = Multiplier(); // 儲存函數物件
    std::cout << "函數物件 Multiplier(3,5) = " << func(3, 5) << std::endl;  // 15

    func = [](int a, int b) { return a - b; };  // 儲存 Lambda
    std::cout << "Lambda 減法(3,5) = " << func(3, 5) << std::endl;  // -2

    // 儲存多個操作的向量（多型行為）
    std::vector<std::function<int(int, int)>> operations = {
        add,
        Multiplier(),
        [](int a, int b) { return a - b; },
        [](int a, int b) { return a / b; }
    };
    std::vector<std::string> op_names = {"加", "乘", "減", "除"};
    int x = 20, y = 4;
    for (size_t i = 0; i < operations.size(); ++i) {
        std::cout << x << " " << op_names[i] << " " << y << " = "
                  << operations[i](x, y) << std::endl;
    }

    // ---- 示範十：實戰：Lambda 與 STL 演算法組合 ----
    std::cout << "\n===== 實戰：商品管理系統 =====" << std::endl;
    std::vector<Product> products = {
        {"Apple",  1.50, 100},
        {"Banana", 0.75, 150},
        {"Orange", 2.00,  80},
        {"Mango",  3.00,  50},
        {"Grape",  2.50, 120}
    };

    // 按價格升序排序
    std::sort(products.begin(), products.end(),
        [](const Product& a, const Product& b) { return a.price < b.price; });
    std::cout << "按價格排序後：" << std::endl;
    for (const auto& p : products) {
        std::cout << "  " << p.name << ": $" << p.price << std::endl;
    }

    // 計算總庫存價值
    double total = std::accumulate(products.begin(), products.end(), 0.0,
        [](double sum, const Product& p) { return sum + p.price * p.quantity; });
    std::cout << "總庫存價值: $" << total << std::endl;

    // 統計低庫存產品（數量 < 100）
    int low_stock = std::count_if(products.begin(), products.end(),
        [](const Product& p) { return p.quantity < 100; });
    std::cout << "低庫存產品數: " << low_stock << std::endl;

    // 對所有產品打 9 折
    double discount = 0.9;
    std::for_each(products.begin(), products.end(),
        [discount](Product& p) { p.price *= discount; });
    std::cout << "打折後第一項: $" << products[0].price << std::endl;

    std::cout << "\n===== 第 8 課總複習完成 =====" << std::endl;

    return 0;
}

/*
 * ============================================================
 * 重點整理表
 * ============================================================
 *
 * | 方式           | 語法                          | 優點             | 缺點           |
 * |---------------|-------------------------------|-----------------|----------------|
 * | 普通函數       | bool f(int x) { ... }         | 簡單             | 無法攜帶狀態    |
 * | 函數物件       | class F { bool operator()().. }| 可攜帶狀態       | 需要寫類別      |
 * | Lambda        | [cap](params){ body }          | 就地定義，簡潔   | 複雜情況難閱讀   |
 * | std::function | std::function<sig> f = ...    | 通用，可動態切換 | 有額外開銷      |
 *
 * 使用建議：
 * - 簡單的一次性行為 → 用 Lambda
 * - 需要複雜狀態 → 用自訂函數物件類別
 * - 常見操作（排序、累積）→ 用 STL 內建函數物件
 * - 需要在執行時動態切換行為 → 用 std::function
 * ============================================================
 */
