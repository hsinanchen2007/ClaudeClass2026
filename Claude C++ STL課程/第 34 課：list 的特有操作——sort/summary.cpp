// ============================================================
// 第 34 課 總結：list 的特有操作——sort
// 編譯：g++ -std=c++17 -O2 -o summary summary.cpp
// ============================================================
// 【為什麼 list 要自己的 sort？】
//   std::sort 需要 RandomAccessIterator（it+n, it-n）
//   list 只有 BidirectionalIterator → 不能用 std::sort
//   → list 提供自己的成員函數 lst.sort()
//
// 【list::sort 的特性】
//   演算法：歸併排序（Merge Sort）O(n log n)
//   穩定排序：相同值保持原始順序
//   迭代器全部有效（節點不動，只改指標連線）
//
// 【用法】
//   lst.sort()              預設升序
//   lst.sort(comp)          自訂比較（降序、多鍵等）
//   lst.sort(greater<>())   降序
//
// 【效能】list::sort 比「複製到 vector → sort → 複製回來」慢
//   因為 list 節點散布在記憶體各處，cache 不友善
// ============================================================

#include <iostream>
#include <list>
#include <vector>
#include <string>
#include <functional>
#include <algorithm>
#include <chrono>
#include <random>
using namespace std;

template <typename T>
void print(const string& label, const list<T>& lst) {
    cout << "  " << label << ": ";
    for (const auto& v : lst) cout << v << " ";
    cout << endl;
}

int main() {
    // 1. 基本排序
    cout << "===== 升序 / 降序 =====\n";
    {
        list<int> lst = {5,2,8,1,9,3,7,4,6};
        print("原始  ", lst);
        lst.sort();
        print("升序  ", lst);
        lst.sort(greater<int>());
        print("降序  ", lst);
    }

    // 2. 自訂物件排序
    cout << "\n===== 自訂物件排序 =====\n";
    {
        struct Student { string name; double gpa; };
        list<Student> students = {
            {"Alice",3.5},{"Bob",3.9},{"Charlie",3.2},
            {"David",3.7},{"Eve",3.9}
        };
        students.sort([](const Student& a, const Student& b) {
            return a.gpa > b.gpa;
        });
        for (auto& s : students)
            cout << "  " << s.name << " GPA:" << s.gpa << "\n";
    }

    // 3. 穩定性驗證
    cout << "\n===== 穩定性 =====\n";
    {
        struct Item { int key; string label; };
        list<Item> items = {
            {3,"A"},{1,"B"},{2,"C"},{1,"D"},{3,"E"},{2,"F"}
        };
        items.sort([](const Item& a, const Item& b) { return a.key < b.key; });
        cout << "  ";
        for (auto& i : items) cout << i.key << i.label << " ";
        cout << "\n  （相同 key 保持原始順序 → 穩定）\n";
    }

    // 4. 迭代器穩定性
    cout << "\n===== 迭代器穩定性 =====\n";
    {
        list<int> lst = {50,30,10,40,20};
        auto it = next(lst.begin());
        cout << "  sort 前 *it=" << *it << " addr=" << &(*it) << "\n";
        lst.sort();
        cout << "  sort 後 *it=" << *it << " addr=" << &(*it) << "\n";
        cout << "  （值和地址不變，只是在 list 中位置改變）\n";
    }

    // 5. 多鍵排序（利用穩定性）
    cout << "\n===== 多鍵排序 =====\n";
    {
        struct Emp { string dept; string name; };
        list<Emp> emps = {
            {"工程","Alice"},{"行銷","Bob"},{"工程","Charlie"},
            {"行銷","David"},{"工程","Eve"}
        };
        emps.sort([](const Emp& a, const Emp& b) { return a.name < b.name; });
        emps.sort([](const Emp& a, const Emp& b) { return a.dept < b.dept; });
        // 穩定：同部門內名字順序保持
        for (auto& e : emps) cout << "  " << e.dept << " | " << e.name << "\n";
    }

    // 6. 效能比較
    cout << "\n===== 效能比較 =====\n";
    {
        const int N = 500000;
        mt19937 gen(42);
        list<int> lst1;
        for (int i = 0; i < N; i++) lst1.push_back(gen() % 1000000);
        list<int> lst2 = lst1;

        auto t1 = chrono::high_resolution_clock::now();
        lst1.sort();
        auto t2 = chrono::high_resolution_clock::now();

        auto t3 = chrono::high_resolution_clock::now();
        vector<int> vec(lst2.begin(), lst2.end());
        sort(vec.begin(), vec.end());
        lst2.assign(vec.begin(), vec.end());
        auto t4 = chrono::high_resolution_clock::now();

        auto ms1 = chrono::duration_cast<chrono::milliseconds>(t2 - t1).count();
        auto ms2 = chrono::duration_cast<chrono::milliseconds>(t4 - t3).count();
        cout << "  list::sort:         " << ms1 << " ms\n";
        cout << "  vector sort+複製:   " << ms2 << " ms\n";
    }

    return 0;
}
