/*
 * ================================================================
 * 【第10課：vector 的宣告與初始化方式】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 * 本課重點：
 * 1. 預設建構（空 vector）
 * 2. 指定大小建構：vector(n) 和 vector(n, val)
 * 3. 初始化串列建構：{1, 2, 3}
 * 4. 小括號 () vs 大括號 {} 的關鍵差異（陷阱！）
 * 5. 從其他容器或迭代器範圍建構
 * 6. 移動建構：std::move
 * 7. assign 重新初始化
 * 8. 自訂類別的 vector 初始化
 * 9. C++17 類別模板引數推導（CTAD）
 * ================================================================
 */

#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <utility>  // std::move

// ===== 重點一：預設建構 — 空的 vector =====
// 三種等效寫法，建立不含任何元素的 vector（size=0, capacity=0）

void demo_default_construction() {
    std::cout << "\n===== 重點一：預設建構（空 vector）=====\n";

    std::vector<int>         v1;        // 最常見寫法
    std::vector<double>      v2{};      // C++11 統一初始化語法
    std::vector<std::string> v3 = {};   // 明確空的初始化串列

    std::cout << "v1 size: " << v1.size() << ", capacity: " << v1.capacity() << std::endl;
    std::cout << "v2 size: " << v2.size() << ", capacity: " << v2.capacity() << std::endl;
    std::cout << "v3 size: " << v3.size() << ", capacity: " << v3.capacity() << std::endl;
    // 三者均輸出 size: 0, capacity: 0
}

// ===== 重點二：指定大小的建構 =====
// vector(n)     → 建立含 n 個「預設值」元素（int 預設為 0）
// vector(n, v)  → 建立含 n 個值均為 v 的元素
// 注意：這裡用的是小括號 ()，不是大括號 {}

void demo_size_construction() {
    std::cout << "\n===== 重點二：指定大小建構 =====\n";

    // 5 個元素，每個都是 int 的預設值（0）
    std::vector<int> v1(5);
    std::cout << "v1(5): ";
    for (int x : v1) std::cout << x << " ";  // 0 0 0 0 0
    std::cout << std::endl;

    // 5 個元素，每個值為 42
    std::vector<int> v2(5, 42);
    std::cout << "v2(5, 42): ";
    for (int x : v2) std::cout << x << " ";  // 42 42 42 42 42
    std::cout << std::endl;

    // 3 個預設建構的 string（空字串）
    std::vector<std::string> v3(3);
    std::cout << "v3(3) size: " << v3.size() << std::endl;  // 3
}

// ===== 重點三：初始化串列（Initializer List）=====
// C++11 引入的大括號初始化，可以直接列出元素值

void demo_initializer_list() {
    std::cout << "\n===== 重點三：初始化串列 =====\n";

    std::vector<int> v1 = {1, 2, 3, 4, 5};  // 明確的初始化串列
    std::vector<int> v2{1, 2, 3, 4, 5};      // 同上，省略等號
    std::vector<int> v3({1, 2, 3, 4, 5});    // 同上，較少人這樣寫

    std::cout << "v1: ";
    for (int x : v1) std::cout << x << " ";  // 1 2 3 4 5
    std::cout << std::endl;

    std::cout << "v1.size(): " << v1.size() << std::endl;  // 5
    std::cout << "v2 和 v3 效果相同\n";
}

// ===== 重點四：小括號 vs 大括號的陷阱 ★★★ =====
// 這是最容易混淆的地方！
//
// 規則：
//   () 呼叫建構子
//       vector<int>(5, 10) 意思是「5 個元素，值為 10」
//   {} 優先嘗試初始化串列
//       vector<int>{5, 10} 意思是「元素是 5 和 10」（兩個元素）
//
// 記憶訣竅：
//   () → 建構子參數（size, value）
//   {} → 元素清單（逐一列舉）

void demo_parentheses_vs_braces() {
    std::cout << "\n===== 重點四：小括號 vs 大括號（重要！）=====\n";

    std::vector<int> v1(5, 10);   // 5 個元素，每個都是 10
    std::vector<int> v2{5, 10};   // 2 個元素：5 和 10

    std::cout << "v1(5, 10) size: " << v1.size() << std::endl;  // 5
    std::cout << "v1: ";
    for (int x : v1) std::cout << x << " ";  // 10 10 10 10 10
    std::cout << std::endl;

    std::cout << "v2{5, 10} size: " << v2.size() << std::endl;  // 2
    std::cout << "v2: ";
    for (int x : v2) std::cout << x << " ";  // 5 10
    std::cout << std::endl;

    // 更多範例
    std::vector<int> a(3);    // {0, 0, 0}  3 個預設元素
    std::vector<int> b{3};    // {3}        1 個元素，值為 3
    std::vector<int> c(3, 7); // {7, 7, 7}  3 個 7
    std::vector<int> d{3, 7}; // {3, 7}     2 個元素

    std::cout << "a(3) size: " << a.size() << " 內容:";
    for (int x : a) std::cout << " " << x;
    std::cout << std::endl;

    std::cout << "b{3} size: " << b.size() << " 內容:";
    for (int x : b) std::cout << " " << x;
    std::cout << std::endl;
}

// ===== 重點五：從其他容器或迭代器範圍建構 =====
// vector 可以從任何提供迭代器的容器（或 C 風格陣列）建構

void demo_range_construction() {
    std::cout << "\n===== 重點五：從範圍/其他容器建構 =====\n";

    // 從另一個 vector 複製建構
    std::vector<int> original = {1, 2, 3, 4, 5};
    std::vector<int> copy1(original);       // 複製建構
    std::vector<int> copy2 = original;      // 同上（複製賦值初始化）

    // 從迭代器範圍建構（取部分元素）
    std::vector<int> partial(original.begin() + 1, original.begin() + 4);
    std::cout << "partial: ";
    for (int x : partial) std::cout << x << " ";  // 2 3 4
    std::cout << std::endl;

    // 從 C 風格陣列建構
    int arr[] = {10, 20, 30, 40};
    std::vector<int> from_array(std::begin(arr), std::end(arr));
    // 或者：std::vector<int> from_array(arr, arr + 4);
    std::cout << "from_array: ";
    for (int x : from_array) std::cout << x << " ";  // 10 20 30 40
    std::cout << std::endl;

    // 從 std::array 建構
    std::array<int, 3> std_arr = {100, 200, 300};
    std::vector<int> from_std_array(std_arr.begin(), std_arr.end());
    std::cout << "from_std_array: ";
    for (int x : from_std_array) std::cout << x << " ";  // 100 200 300
    std::cout << std::endl;
}

// ===== 重點六：移動建構 =====
// 當來源 vector 不再需要時，可以「移動」而非「複製」。
// 移動操作只轉移內部指標的所有權，不複製元素，效率極高（O(1)）。
// 移動後，source 處於「有效但未指定」狀態，通常是空的。

void demo_move_construction() {
    std::cout << "\n===== 重點六：移動建構 =====\n";

    std::vector<int> source = {1, 2, 3, 4, 5};
    std::cout << "移動前 source.size(): " << source.size() << std::endl;  // 5

    std::vector<int> dest = std::move(source);  // 移動建構，O(1)

    std::cout << "移動後 source.size(): " << source.size() << std::endl;  // 通常是 0
    std::cout << "移動後 dest.size():   " << dest.size()   << std::endl;  // 5

    // 警告：移動後不應再使用 source 的元素（狀態未定義）
    // 但 source 仍然是合法物件，可以重新 assign
    source = {10, 20};
    std::cout << "重新賦值後 source.size(): " << source.size() << std::endl;  // 2
}

// ===== 重點七：assign 重新初始化已存在的 vector =====
// assign 會清空現有內容，重新設定 vector 的元素

void demo_assign() {
    std::cout << "\n===== 重點七：assign 重新初始化 =====\n";

    std::vector<int> v = {1, 2, 3};

    // 方法一：指定數量和值
    v.assign(5, 100);
    std::cout << "assign(5, 100): ";
    for (int x : v) std::cout << x << " ";  // 100 100 100 100 100
    std::cout << std::endl;

    // 方法二：從初始化串列
    v.assign({10, 20, 30});
    std::cout << "assign({10, 20, 30}): ";
    for (int x : v) std::cout << x << " ";  // 10 20 30
    std::cout << std::endl;

    // 方法三：從迭代器範圍
    std::vector<int> other = {7, 8, 9, 10, 11};
    v.assign(other.begin() + 1, other.end() - 1);
    std::cout << "從迭代器範圍 assign: ";
    for (int x : v) std::cout << x << " ";  // 8 9 10
    std::cout << std::endl;
}

// ===== 重點八：自訂類別的 vector 初始化 =====
// 使用大括號初始化串列，每個元素會呼叫對應建構子

struct Person {
    std::string name;
    int age;
    Person(const std::string& n, int a) : name(n), age(a) {}
};

void demo_custom_class() {
    std::cout << "\n===== 重點八：自訂類別的初始化 =====\n";

    // 使用初始化串列（需要隱式轉換或大括號建構）
    std::vector<Person> people = {
        {"Alice",   30},
        {"Bob",     25},
        {"Charlie", 35}
    };

    for (const auto& p : people) {
        std::cout << "  " << p.name << " is " << p.age << " years old.\n";
    }
}

// ===== 重點九：C++17 類別模板引數推導（CTAD）=====
// C++17 開始，某些情況下可省略模板參數，由編譯器自動推導

void demo_ctad() {
    std::cout << "\n===== 重點九：C++17 CTAD 自動推導 =====\n";

    // C++17 之前必須寫：std::vector<int> v = {1, 2, 3};
    // C++17 可以讓編譯器推導型別：
    std::vector v1 = {1, 2, 3};         // 推導為 vector<int>
    std::vector v2 = {1.5, 2.5, 3.5};  // 推導為 vector<double>

    // 從迭代器推導
    std::vector<int> source = {10, 20, 30};
    std::vector v3(source.begin(), source.end());  // 推導為 vector<int>

    std::cout << "v1 型別推導為 vector<int>，size: " << v1.size() << std::endl;
    std::cout << "v2 型別推導為 vector<double>，size: " << v2.size() << std::endl;
    std::cout << "v3 從迭代器推導，size: " << v3.size() << std::endl;

    // 注意：型別不明顯時，建議還是明確寫出型別
}

// ===== 各種初始化方式對照表 =====
// | 語法                           | 結果              | 說明             |
// |--------------------------------|-------------------|------------------|
// | vector<int> v;                 | 空 vector         | 預設建構         |
// | vector<int> v(5);              | {0,0,0,0,0}       | 5 個預設值元素   |
// | vector<int> v(5, 42);          | {42,42,42,42,42}  | 5 個值為 42      |
// | vector<int> v{5, 42};          | {5, 42}           | 2 個元素：5 和 42|
// | vector<int> v = {1,2,3};       | {1, 2, 3}         | 初始化串列       |
// | vector<int> v(other);          | 複製 other        | 複製建構         |
// | vector<int> v(std::move(other))| 接收 other 的資源 | 移動建構         |
// | vector<int> v(it1, it2);       | 範圍 [it1, it2)   | 迭代器範圍建構   |

int main() {
    std::cout << "====================================================\n";
    std::cout << " 第10課：vector 的宣告與初始化方式 — 總複習\n";
    std::cout << "====================================================\n";

    demo_default_construction();
    demo_size_construction();
    demo_initializer_list();
    demo_parentheses_vs_braces();
    demo_range_construction();
    demo_move_construction();
    demo_assign();
    demo_custom_class();
    demo_ctad();

    std::cout << "\n====================================================\n";
    std::cout << " 複習完畢！\n";
    std::cout << "====================================================\n";

    return 0;
}
