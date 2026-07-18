/*
 * ================================================================
 * 【第 14 課：vector 元素插入：insert、emplace】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. insert(pos, value)     —— 在迭代器位置前插入單一元素
 * 2. insert(pos, n, value)  —— 插入 n 個相同元素
 * 3. insert(pos, first, last)—— 插入範圍元素
 * 4. emplace(pos, args...)  —— 就地構造插入（C++11）
 * 5. 插入的效能代價：O(n) 位移
 * 6. insert 回傳迭代器（C++11 後）
 * ================================================================
 */

#include <iostream>
#include <vector>
#include <list>
#include <string>
using namespace std;

// ================================================================
// 重點一：insert 基本用法 —— 在指定位置前插入
// ================================================================
// 語法：iterator insert(iterator pos, const T& value)
// 新元素插入在 pos 「之前」
// 回傳值：指向新插入元素的迭代器（C++11）
// 注意：插入後，pos 之後所有元素都要向後位移（O(n)）

void demoInsertSingle() {
    cout << "\n【insert 插入單一元素】" << endl;

    vector<int> v = {1, 2, 3, 4, 5};

    // 在開頭插入（v.begin() 位置之前）
    auto it = v.insert(v.begin(), 0);
    // v: {0, 1, 2, 3, 4, 5}
    cout << "在開頭插入 0，返回的迭代器指向: " << *it << endl;

    // 在第三個位置插入（索引 2 之前）
    it = v.insert(v.begin() + 2, 100);
    // v: {0, 1, 100, 2, 3, 4, 5}
    cout << "在索引 2 插入 100，返回的迭代器指向: " << *it << endl;

    // 在結尾插入（等同於 push_back）
    v.insert(v.end(), 6);
    // v: {0, 1, 100, 2, 3, 4, 5, 6}

    cout << "最終元素: ";
    for (int n : v) cout << n << " ";
    cout << endl;
}

// ================================================================
// 重點二：insert 插入 n 個相同元素
// ================================================================
// 語法：iterator insert(iterator pos, size_type n, const T& value)
// 在 pos 之前插入 n 個 value 的副本

void demoInsertN() {
    cout << "\n【insert 插入多個相同元素】" << endl;

    vector<int> v = {1, 2, 3};

    // 在開頭插入 3 個 0
    v.insert(v.begin(), 3, 0);
    // v: {0, 0, 0, 1, 2, 3}
    cout << "插入 3 個 0: ";
    for (int n : v) cout << n << " ";
    cout << endl;

    // 在結尾插入 2 個 99
    v.insert(v.end(), 2, 99);
    // v: {0, 0, 0, 1, 2, 3, 99, 99}
    cout << "插入 2 個 99: ";
    for (int n : v) cout << n << " ";
    cout << endl;
}

// ================================================================
// 重點三：insert 插入範圍（另一個容器的元素）
// ================================================================
// 語法：iterator insert(iterator pos, InputIt first, InputIt last)
// 將 [first, last) 範圍的元素插入到 pos 之前

void demoInsertRange() {
    cout << "\n【insert 插入範圍元素】" << endl;

    vector<int> v1 = {1, 2, 3};
    vector<int> v2 = {10, 20, 30};

    // 將 v2 的全部元素插入到 v1 的中間（索引 1 之前）
    v1.insert(v1.begin() + 1, v2.begin(), v2.end());
    // v1: {1, 10, 20, 30, 2, 3}
    cout << "插入 v2 後: ";
    for (int n : v1) cout << n << " ";
    cout << endl;

    // 也可以插入初始化列表
    vector<int> v3 = {100, 200, 300};
    v3.insert(v3.begin(), {-3, -2, -1});
    cout << "插入初始化列表: ";
    for (int n : v3) cout << n << " ";
    cout << endl;

    // 從原生陣列插入
    int arr[] = {7, 8, 9};
    v3.insert(v3.end(), arr, arr + 3);
    cout << "插入原生陣列: ";
    for (int n : v3) cout << n << " ";
    cout << endl;
}

// ================================================================
// 重點四：emplace —— 就地構造插入（C++11）
// ================================================================
// 語法：iterator emplace(iterator pos, args...)
// 直接在 pos 前就地構造元素（傳建構函數參數）
// 省去建立臨時物件再複製/移動的開銷

struct Point {
    int x, y;
    Point(int x, int y) : x(x), y(y) {}
    friend ostream& operator<<(ostream& os, const Point& p) {
        return os << "(" << p.x << "," << p.y << ")";
    }
};

void demoEmplace() {
    cout << "\n【emplace 就地構造插入】" << endl;

    vector<Point> v;
    v.reserve(5);

    // push_back：需要構造 Point 再移動
    v.push_back(Point(1, 2));

    // emplace：直接傳建構參數，就地構造
    v.emplace(v.begin(), 0, 0);           // 在最前面插入 (0,0)
    v.emplace(v.end(), 3, 4);             // 在最後插入 (3,4)
    v.emplace(v.begin() + 1, 10, 20);    // 在索引 1 前插入

    cout << "Points: ";
    for (const Point& p : v) cout << p << " ";
    cout << endl;
}

// ================================================================
// 重點五：insert 的效能代價
// ================================================================
// vector 的 insert（非尾端）需要將插入位置後的所有元素向後位移
// 這是 O(n) 的操作！
//
// 若頻繁在中間插入，考慮使用：
//   - std::list（O(1) 任意插入，但無隨機存取）
//   - std::deque（O(1) 頭尾插入）

void demoPerformanceNote() {
    cout << "\n【效能說明】" << endl;

    vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // 在 v.begin() 插入：需要移動所有 10 個元素
    v.insert(v.begin(), 0);
    cout << "在頭部插入（O(n) 位移）: ";
    for (int n : v) cout << n << " ";
    cout << endl;

    // 在 v.end() 插入（等同 push_back）：O(1)
    v.insert(v.end(), 11);
    cout << "在尾部插入（O(1)）: 最後 = " << v.back() << endl;

    cout << "提示：頻繁在中間插入應改用 std::list" << endl;
}

// ================================================================
// 重點六：insert 回傳值的使用
// ================================================================
// C++11 後，insert 回傳指向新插入元素的迭代器
// 這讓鏈式操作成為可能

void demoReturnValue() {
    cout << "\n【insert 回傳迭代器的使用】" << endl;

    vector<int> v = {10, 30, 50};

    // 取得插入後的迭代器，繼續使用
    auto it = v.insert(v.begin() + 1, 20);  // 插入 20，it 指向 20
    cout << "插入 20 後，it 指向: " << *it << endl;

    // 在剛插入的元素後再插入
    v.insert(it + 1, 25);  // 在 20 後插入 25
    cout << "再插入 25: ";
    for (int n : v) cout << n << " ";
    cout << endl;
}

int main() {
    cout << "=============================================" << endl;
    cout << "   第 14 課：vector insert / emplace 總複習" << endl;
    cout << "=============================================" << endl;

    demoInsertSingle();
    demoInsertN();
    demoInsertRange();
    demoEmplace();
    demoPerformanceNote();
    demoReturnValue();

    cout << "\n==============================================" << endl;
    cout << " 重點：insert 插入在 pos「之前」" << endl;
    cout << " 效能：尾端插入 O(1)；中間插入 O(n)" << endl;
    cout << " emplace：就地構造，比 insert 少一次建構" << endl;
    cout << "==============================================" << endl;

    return 0;
}
