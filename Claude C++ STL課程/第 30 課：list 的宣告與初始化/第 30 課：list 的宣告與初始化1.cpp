// lesson30_list_init.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -o lesson30 lesson30_list_init.cpp

#include <iostream>
#include <list>
#include <vector>
#include <string>
using namespace std;

// 輔助函數：印出 list 內容
template <typename T>
void print_list(const string& label, const list<T>& lst) {
    cout << label << " [size=" << lst.size() << "]: ";
    for (const auto& val : lst) {
        cout << val << " ";
    }
    cout << endl;
}

int main() {
    cout << "===== 1. 預設建構（空 list）=====" << endl;
    list<int> lst1;
    print_list("lst1", lst1);
    cout << "begin() == end()? " << (lst1.begin() == lst1.end() ? "是" : "否") << endl;

    cout << "\n===== 2. 填充建構 =====" << endl;
    list<int> lst2(5);        // 5 個預設值
    print_list("lst2(5)     ", lst2);

    list<int> lst3(5, 42);    // 5 個 42
    print_list("lst3(5, 42) ", lst3);

    list<string> lst4(3, "STL");
    print_list("lst4(3,STL) ", lst4);

    cout << "\n===== 3. 初始化列表建構 =====" << endl;
    list<int> lst5 = {10, 20, 30, 40, 50};
    print_list("lst5{...}   ", lst5);

    cout << "\n===== 4. 範圍建構 =====" << endl;
    // 從陣列
    int arr[] = {100, 200, 300};
    list<int> lst6(begin(arr), end(arr));
    print_list("從陣列      ", lst6);

    // 從 vector
    vector<int> vec = {7, 8, 9};
    list<int> lst7(vec.begin(), vec.end());
    print_list("從 vector   ", lst7);

    // 從另一個 list 的部分範圍
    auto it_start = lst5.begin();
    auto it_end = lst5.begin();
    advance(it_start, 1);  // 指向 20
    advance(it_end, 4);    // 指向 50
    list<int> lst8(it_start, it_end);
    print_list("部分範圍    ", lst8);  // {20, 30, 40}

    cout << "\n===== 5. 複製建構 =====" << endl;
    list<int> lst9(lst5);  // 深複製
    print_list("原始 lst5   ", lst5);
    print_list("複製 lst9   ", lst9);

    lst9.push_back(60);
    print_list("lst9 加 60  ", lst9);
    print_list("lst5 不變   ", lst5);  // 確認深複製

    cout << "\n===== 6. 移動建構 =====" << endl;
    list<int> src = {1, 2, 3, 4, 5};
    print_list("移動前 src  ", src);

    list<int> dst(std::move(src));
    print_list("移動後 dst  ", dst);
    print_list("移動後 src  ", src);  // 空的

    cout << "\n===== 7. assign 重新指定 =====" << endl;
    list<int> lst10 = {1, 2, 3};
    print_list("assign 前   ", lst10);

    lst10.assign(4, 99);
    print_list("assign(4,99)", lst10);

    lst10.assign({10, 20, 30, 40, 50});
    print_list("assign{...} ", lst10);

    lst10.assign(vec.begin(), vec.end());
    print_list("assign(vec) ", lst10);

    cout << "\n===== 8. 特殊情境 =====" << endl;

    // 小心：圓括號 vs 花括號
    list<int> a(5, 1);    // 5 個 1 → {1, 1, 1, 1, 1}
    list<int> b{5, 1};    // 初始化列表 → {5, 1}
    print_list("(5, 1) 填充 ", a);
    print_list("{5, 1} 列表 ", b);

    // 巢狀 list
    list<list<int>> nested;
    nested.push_back({1, 2, 3});
    nested.push_back({4, 5});
    nested.push_back({6, 7, 8, 9});
    cout << "\n巢狀 list:" << endl;
    int row = 0;
    for (const auto& inner : nested) {
        cout << "  row " << row++ << ": ";
        for (int val : inner) cout << val << " ";
        cout << endl;
    }

    return 0;
}
