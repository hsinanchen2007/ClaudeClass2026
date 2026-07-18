/*
 * ================================================================
 * 【第 7 課：演算法（Algorithm）與容器的分離設計】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. STL 演算法與容器分離的設計哲學
 * 2. 迭代器作為橋樑：演算法只認識迭代器
 * 3. 常用演算法：find, sort, count, transform, for_each
 * 4. 同一演算法適用多種容器
 * 5. 謂詞（Predicate）與自定義比較
 * 6. <algorithm> 中的重要演算法分類
 * ================================================================
 */

#include <iostream>
#include <vector>
#include <list>
#include <set>
#include <array>
#include <algorithm>
#include <numeric>
#include <functional>
#include <string>
using namespace std;

// ================================================================
// 重點一：分離設計哲學
// ================================================================
// STL 的核心設計：演算法（algorithm）獨立於容器（container）
// 演算法不直接操作容器，而是透過「迭代器」操作資料
//
// 傳統方式：每種資料結構有自己的一套操作函數（大量重複）
// STL 方式：一個演算法 + 任何容器的迭代器 = 通用操作
//
// 好處：
//   - N 個容器 × M 個演算法 = 只需要 N+M 個實作（不是 N×M）
//   - 新增容器或演算法不影響另一邊

// ================================================================
// 重點二：同一個演算法用於不同容器
// ================================================================
// std::find 只需要「開始迭代器」和「結束迭代器」
// 可以用在 vector、list、set、array、甚至原生陣列

void demoFindUniversal() {
    cout << "\n【同一個 find 用在不同容器】" << endl;

    vector<int> vec = {10, 20, 30, 40, 50};
    list<int> lst = {10, 20, 30, 40, 50};
    set<int> s = {10, 20, 30, 40, 50};
    int arr[] = {10, 20, 30, 40, 50};

    // 完全相同的 std::find 介面
    auto v_it = find(vec.begin(), vec.end(), 30);
    auto l_it = find(lst.begin(), lst.end(), 30);
    auto s_it = find(s.begin(), s.end(), 30);
    auto a_it = find(arr, arr + 5, 30);     // 原生陣列：指標就是迭代器

    cout << "在 vector 找到 30: " << (v_it != vec.end() ? "✓" : "✗") << endl;
    cout << "在 list 找到 30:   " << (l_it != lst.end() ? "✓" : "✗") << endl;
    cout << "在 set 找到 30:    " << (s_it != s.end() ? "✓" : "✗") << endl;
    cout << "在 array 找到 30:  " << (a_it != arr + 5 ? "✓" : "✗") << endl;
}

// ================================================================
// 重點三：排序演算法 —— std::sort
// ================================================================
// sort 要求「隨機存取迭代器」（vector、array、deque）
// list 沒有隨機存取迭代器，需用自己的 list::sort()

void demoSort() {
    cout << "\n【std::sort 排序】" << endl;

    vector<int> v = {5, 2, 8, 1, 9, 3};

    // 預設：升序（小到大）
    sort(v.begin(), v.end());
    cout << "升序: ";
    for (int n : v) cout << n << " ";
    cout << endl;

    // 自定義：降序（大到小）
    sort(v.begin(), v.end(), greater<int>());
    cout << "降序: ";
    for (int n : v) cout << n << " ";
    cout << endl;

    // 使用 lambda 自定義排序規則
    vector<string> words = {"banana", "apple", "cherry", "date"};
    sort(words.begin(), words.end(), [](const string& a, const string& b) {
        return a.length() < b.length();  // 按字串長度升序
    });
    cout << "按長度排序: ";
    for (const string& w : words) cout << w << " ";
    cout << endl;
}

// ================================================================
// 重點四：計數演算法 —— std::count / std::count_if
// ================================================================
// count(begin, end, value)：計算等於 value 的元素數量
// count_if(begin, end, pred)：計算滿足謂詞的元素數量

void demoCount() {
    cout << "\n【std::count / count_if】" << endl;

    vector<int> v = {1, 3, 5, 2, 4, 3, 6, 3, 8};

    // 計算特定值的數量
    int threes = count(v.begin(), v.end(), 3);
    cout << "3 出現了 " << threes << " 次" << endl;

    // 計算滿足條件的數量（謂詞）
    int evens = count_if(v.begin(), v.end(), [](int n) {
        return n % 2 == 0;  // 偶數
    });
    cout << "偶數有 " << evens << " 個" << endl;

    int gt4 = count_if(v.begin(), v.end(), [](int n) {
        return n > 4;
    });
    cout << "大於 4 的有 " << gt4 << " 個" << endl;
}

// ================================================================
// 重點五：轉換演算法 —— std::transform
// ================================================================
// transform：對每個元素套用函數，結果寫入目標位置
// transform(srcBegin, srcEnd, destBegin, func)
// transform(src1Begin, src1End, src2Begin, destBegin, binaryFunc)

void demoTransform() {
    cout << "\n【std::transform】" << endl;

    vector<int> v = {1, 2, 3, 4, 5};

    // 每個元素乘以 2，結果存到新 vector
    vector<int> doubled(v.size());
    transform(v.begin(), v.end(), doubled.begin(), [](int n) {
        return n * 2;
    });
    cout << "乘以 2: ";
    for (int n : doubled) cout << n << " ";
    cout << endl;

    // 就地修改（原地轉換）
    transform(v.begin(), v.end(), v.begin(), [](int n) {
        return n * n;  // 平方
    });
    cout << "平方後: ";
    for (int n : v) cout << n << " ";
    cout << endl;

    // 字串轉大寫
    string s = "hello world";
    transform(s.begin(), s.end(), s.begin(), ::toupper);
    cout << "轉大寫: " << s << endl;
}

// ================================================================
// 重點六：for_each —— 對每個元素執行操作
// ================================================================
// for_each(begin, end, func)：對每個元素呼叫 func
// 注意：不會修改元素（除非 func 透過參考修改）
// C++11 後的 range-based for 更簡潔，但 for_each 可以傳回函數物件

void demoForEach() {
    cout << "\n【std::for_each】" << endl;

    vector<int> v = {10, 20, 30, 40, 50};

    // 簡單的 for_each
    cout << "印出每個元素: ";
    for_each(v.begin(), v.end(), [](int n) {
        cout << n << " ";
    });
    cout << endl;

    // for_each 配合有狀態的 lambda
    int sum = 0;
    for_each(v.begin(), v.end(), [&sum](int n) {
        sum += n;  // 捕獲 sum 的參考
    });
    cout << "總和: " << sum << endl;
}

// ================================================================
// 重點七：搜尋與查找演算法
// ================================================================
// find_if(begin, end, pred)：找到第一個滿足謂詞的元素
// any_of / all_of / none_of：條件判斷

void demoSearch() {
    cout << "\n【搜尋與條件判斷】" << endl;

    vector<int> v = {1, 3, 5, 7, 8, 11, 13};

    // find_if：找第一個偶數
    auto it = find_if(v.begin(), v.end(), [](int n) {
        return n % 2 == 0;
    });
    if (it != v.end()) {
        cout << "第一個偶數: " << *it << endl;
    }

    // any_of：是否有任何元素滿足條件
    bool hasNeg = any_of(v.begin(), v.end(), [](int n) { return n < 0; });
    cout << "有負數: " << (hasNeg ? "是" : "否") << endl;

    // all_of：是否所有元素滿足條件
    bool allPos = all_of(v.begin(), v.end(), [](int n) { return n > 0; });
    cout << "都是正數: " << (allPos ? "是" : "否") << endl;

    // none_of：是否沒有元素滿足條件
    bool noneGt100 = none_of(v.begin(), v.end(), [](int n) { return n > 100; });
    cout << "沒有大於 100: " << (noneGt100 ? "是" : "否") << endl;
}

// ================================================================
// 重點八：數值演算法 —— accumulate, iota
// ================================================================
// accumulate：累積計算（求和、乘積等）
// iota：填入連續值

void demoNumeric() {
    cout << "\n【數值演算法 <numeric>】" << endl;

    vector<int> v(5);
    iota(v.begin(), v.end(), 1);  // 填入 1, 2, 3, 4, 5
    cout << "iota 填入: ";
    for (int n : v) cout << n << " ";
    cout << endl;

    // 求和
    int sum = accumulate(v.begin(), v.end(), 0);
    cout << "總和: " << sum << endl;

    // 乘積（初始值設為 1，使用 multiplies）
    int product = accumulate(v.begin(), v.end(), 1, multiplies<int>());
    cout << "乘積 (5!): " << product << endl;
}

// ================================================================
// 重點九：常用演算法快速參考
// ================================================================
//
// ┌──────────────────┬──────────────────────────────────────────┐
// │ 演算法            │ 說明                                      │
// ├──────────────────┼──────────────────────────────────────────┤
// │ find             │ 找第一個等於 value 的元素                  │
// │ find_if          │ 找第一個滿足謂詞的元素                     │
// │ sort             │ 排序（需隨機存取迭代器）                    │
// │ stable_sort      │ 穩定排序（相等元素保持原來順序）            │
// │ count            │ 計算等於 value 的元素數                    │
// │ count_if         │ 計算滿足謂詞的元素數                       │
// │ transform        │ 套用函數轉換每個元素                        │
// │ for_each         │ 對每個元素執行操作                          │
// │ any_of           │ 是否有任意元素滿足條件                      │
// │ all_of           │ 是否所有元素滿足條件                        │
// │ none_of          │ 是否沒有元素滿足條件                        │
// │ accumulate       │ 累積計算（求和等）                          │
// │ iota             │ 填入連續遞增值                              │
// │ max_element      │ 找最大元素的迭代器                          │
// │ min_element      │ 找最小元素的迭代器                          │
// │ reverse          │ 反轉範圍                                    │
// │ unique           │ 移除相鄰重複元素                            │
// │ remove_if        │ 邏輯移除滿足條件的元素                      │
// └──────────────────┴──────────────────────────────────────────┘

int main() {
    cout << "============================================" << endl;
    cout << "   第 7 課：演算法與容器的分離設計展示" << endl;
    cout << "============================================" << endl;

    demoFindUniversal();
    demoSort();
    demoCount();
    demoTransform();
    demoForEach();
    demoSearch();
    demoNumeric();

    cout << "\n============================================" << endl;
    cout << " 設計哲學：演算法 + 迭代器 = 與容器無關的通用操作" << endl;
    cout << "============================================" << endl;

    return 0;
}
