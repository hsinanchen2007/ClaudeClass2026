// ============================================================
// 第 30 課 總結：list 的宣告與初始化
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【初始化方式】
//   1. 預設建構：list<int> lst;              空 list
//   2. 填充建構：list<int> lst(5);           5 個 0
//   3. 填充指定值：list<int> lst(5, 42);     5 個 42
//   4. 初始化列表：list<int> lst = {1,2,3};
//   5. 範圍建構：list<int> lst(v.begin(), v.end());
//   6. 複製建構：list<int> lst2(lst1);       深複製
//   7. 移動建構：list<int> lst2(move(lst1)); lst1 變空
//   8. assign：lst.assign({1,2,3});          重新指定
//   9. 小心 (n, val) vs {n, val}
// ============================================================

#include <iostream>
#include <list>
#include <vector>
#include <string>
using namespace std;

template <typename T>
void print(const string& label, const list<T>& lst) {
    cout << "  " << label << " [" << lst.size() << "]: ";
    for (const auto& v : lst) cout << v << " ";
    cout << endl;
}

int main() {
    cout << "===== 初始化方式 =====\n";
    list<int> d1;              print("1.預設     ", d1);
    list<int> d2(5);           print("2.填充(5)  ", d2);
    list<int> d3(5, 42);       print("3.填充(5,42)", d3);
    list<int> d4 = {10,20,30}; print("4.列表     ", d4);

    vector<int> vec = {7, 8, 9};
    list<int> d5(vec.begin(), vec.end());
    print("5.從vector ", d5);

    list<int> d6(d4);
    d6.push_back(99);
    print("6a.複製d4  ", d6);
    print("6b.d4不變  ", d4);

    list<int> d7(move(d6));
    print("7a.移動自d6", d7);
    print("7b.d6被掏空", d6);

    list<int> d8;
    d8.assign({100, 200, 300});
    print("8.assign   ", d8);

    cout << "\n===== 小括號 vs 大括號 =====\n";
    list<int> a(5, 1);   print("(5,1) → 5個1", a);
    list<int> b{5, 1};   print("{5,1} → 兩元素", b);

    cout << "\n===== 巢狀 list =====\n";
    list<list<int>> nested;
    nested.push_back({1, 2, 3});
    nested.push_back({4, 5});
    for (const auto& inner : nested) {
        cout << "  row: ";
        for (int v : inner) cout << v << " ";
        cout << endl;
    }

    return 0;
}
