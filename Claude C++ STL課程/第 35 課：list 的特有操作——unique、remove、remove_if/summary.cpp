// ============================================================
// 第 35 課 總結：list 的特有操作——unique、remove、remove_if
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【remove(val)】移除所有等於 val 的元素
// 【remove_if(pred)】移除所有滿足 pred 的元素
// 【unique()】移除連續相鄰的重複元素
// 【unique(pred)】移除連續相鄰滿足 pred 的元素
//
// ★ 三者都是 O(n)，只遍歷一次
// ★ 未被刪元素的迭代器仍有效
// ★ unique 只去連續重複 → 要完全去重需先 sort
// ★ 這些是 list 的成員函數，比通用演算法更高效（直接改指標）
// ============================================================

#include <iostream>
#include <list>
#include <string>
#include <cmath>
using namespace std;

template <typename T>
void print(const string& label, const list<T>& lst) {
    cout << "  " << label << " [" << lst.size() << "]: ";
    for (const auto& v : lst) cout << v << " ";
    cout << endl;
}

int main() {
    // 1. remove
    cout << "===== remove =====\n";
    {
        list<int> lst = {1,3,2,3,4,3,5,3};
        print("原始      ", lst);
        lst.remove(3);
        print("remove(3) ", lst);
    }

    // 2. remove_if
    cout << "\n===== remove_if =====\n";
    {
        list<int> lst = {1,2,3,4,5,6,7,8,9,10};
        lst.remove_if([](int x) { return x % 2 == 0; });
        print("移除偶數  ", lst);
    }
    {
        struct Student { string name; double gpa; };
        list<Student> students = {
            {"Alice",3.8},{"Bob",2.1},{"Charlie",3.5},
            {"David",1.9},{"Eve",3.9}
        };
        students.remove_if([](const Student& s) { return s.gpa < 2.5; });
        cout << "  移除 GPA<2.5：";
        for (auto& s : students) cout << s.name << " ";
        cout << "\n";
    }

    // 3. unique（只去連續重複）
    cout << "\n===== unique =====\n";
    {
        list<int> lst = {1,1,2,3,3,3,2,2,1};
        print("原始      ", lst);
        lst.unique();
        print("unique    ", lst);
        cout << "  （非連續的重複仍保留！）\n";
    }

    // 4. sort + unique = 完全去重
    cout << "\n===== sort + unique =====\n";
    {
        list<int> lst = {3,1,4,1,5,9,2,6,5,3,5};
        print("原始      ", lst);
        lst.sort();    print("sort      ", lst);
        lst.unique();  print("unique    ", lst);
    }

    // 5. unique 自訂條件
    cout << "\n===== unique 自訂條件 =====\n";
    {
        list<double> lst = {1.0,1.1,1.2,2.5,2.6,3.0,3.3,3.4};
        lst.unique([](double a, double b) { return abs(a-b) < 0.5; });
        cout << "  差<0.5 視為重複：";
        for (double v : lst) cout << v << " ";
        cout << "\n";
    }

    // 6. 三者組合使用
    cout << "\n===== 組合使用 =====\n";
    {
        list<int> data = {-1,5,3,-2,5,0,3,8,-1,5,0,3,7};
        print("原始      ", data);
        data.remove_if([](int x) { return x < 0; });
        print("移除負數  ", data);
        data.remove(0);
        print("移除 0    ", data);
        data.sort();     print("sort      ", data);
        data.unique();   print("unique    ", data);
    }

    // 7. remove_if 搭配外部狀態
    cout << "\n===== remove_if + 外部狀態 =====\n";
    {
        list<int> lst = {5,12,3,18,7,25,9,14};
        int removed = 0;
        lst.remove_if([&removed](int x) {
            if (removed >= 3) return false;
            if (x > 10) { removed++; return true; }
            return false;
        });
        print("移除前3個>10", lst);
        cout << "  移除了 " << removed << " 個\n";
    }

    return 0;
}
