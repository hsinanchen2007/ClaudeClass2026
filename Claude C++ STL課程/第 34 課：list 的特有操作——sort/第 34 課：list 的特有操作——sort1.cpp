// lesson34_list_sort.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -o lesson34 lesson34_list_sort.cpp

#include <iostream>
#include <list>
#include <string>
#include <functional>
#include <chrono>
#include <vector>
#include <algorithm>
#include <random>
using namespace std;

template <typename T>
void print_list(const string& label, const list<T>& lst) {
    cout << label << " [" << lst.size() << "]: ";
    for (const auto& val : lst) cout << val << " ";
    cout << (lst.empty() ? "(空)" : "") << endl;
}

// ===== 手動實作歸併排序（教學用） =====
template <typename T>
void my_list_sort(list<T>& lst) {
    // 基底情況：0 或 1 個元素，已經排序
    if (lst.size() <= 1) return;

    // ① 分割：用快慢指標找中點
    list<T> second_half;
    auto slow = lst.begin();
    auto fast = lst.begin();

    // fast 每次走 2 步，slow 每次走 1 步
    while (fast != lst.end()) {
        ++fast;
        if (fast != lst.end()) {
            ++fast;
            ++slow;
        }
    }
    // slow 現在指向後半段的開頭

    // 用 splice 把後半段切出來（O(n) 因為跨 list 要計數）
    second_half.splice(second_half.begin(), lst, slow, lst.end());

    // ② 遞迴排序兩半
    my_list_sort(lst);           // 排序前半段
    my_list_sort(second_half);   // 排序後半段

    // ③ 合併（用 list::merge，原地操作）
    lst.merge(second_half);
}

int main() {
    // ===== 1. list::sort 基本用法 =====
    cout << "===== list::sort 基本用法 =====" << endl;
    {
        list<int> lst = {5, 2, 8, 1, 9, 3, 7, 4, 6};
        print_list("排序前", lst);

        lst.sort();
        print_list("排序後", lst);
    }

    // ===== 2. 降序排序 =====
    cout << "\n===== 降序排序 =====" << endl;
    {
        list<int> lst = {5, 2, 8, 1, 9, 3, 7, 4, 6};
        print_list("排序前", lst);

        lst.sort(greater<int>());
        print_list("降序後", lst);
    }

    // ===== 3. 自訂物件排序 =====
    cout << "\n===== 自訂物件排序 =====" << endl;
    {
        struct Student {
            string name;
            double gpa;
        };

        list<Student> students = {
            {"Alice", 3.5},
            {"Bob", 3.9},
            {"Charlie", 3.2},
            {"David", 3.7},
            {"Eve", 3.9}
        };

        cout << "排序前：" << endl;
        for (const auto& s : students)
            cout << "  " << s.name << " GPA: " << s.gpa << endl;

        // 按 GPA 降序排序
        students.sort([](const Student& a, const Student& b) {
            return a.gpa > b.gpa;
        });

        cout << "按 GPA 降序：" << endl;
        for (const auto& s : students)
            cout << "  " << s.name << " GPA: " << s.gpa << endl;
    }

    // ===== 4. 穩定性驗證 =====
    cout << "\n===== 穩定性驗證 =====" << endl;
    {
        struct Item {
            int key;
            string label;
        };

        list<Item> items = {
            {3, "A"}, {1, "B"}, {2, "C"},
            {1, "D"}, {3, "E"}, {2, "F"}
        };

        cout << "排序前：";
        for (const auto& item : items)
            cout << item.key << item.label << " ";
        cout << endl;

        items.sort([](const Item& a, const Item& b) {
            return a.key < b.key;
        });

        cout << "排序後：";
        for (const auto& item : items)
            cout << item.key << item.label << " ";
        cout << endl;
        cout << "（相同 key 的元素保持原始順序 → 穩定排序）" << endl;
    }

    // ===== 5. 手動實作 vs list::sort 驗證 =====
    cout << "\n===== 手動歸併排序 vs list::sort =====" << endl;
    {
        list<int> lst1 = {38, 27, 43, 3, 9, 82, 10};
        list<int> lst2 = lst1;  // 複製一份

        print_list("原始資料   ", lst1);

        my_list_sort(lst1);
        print_list("手動歸併   ", lst1);

        lst2.sort();
        print_list("list::sort ", lst2);

        cout << "兩者結果相同：" << (lst1 == lst2 ? "是" : "否") << endl;
    }

    // ===== 6. 排序的迭代器穩定性 =====
    cout << "\n===== sort 的迭代器穩定性 =====" << endl;
    {
        list<int> lst = {50, 30, 10, 40, 20};

        // 保存指向元素 30 的迭代器
        auto it = lst.begin();
        advance(it, 1);  // 指向 30
        cout << "sort 前 *it = " << *it << endl;
        cout << "sort 前 it 的地址 = " << &(*it) << endl;

        lst.sort();

        // 迭代器仍然有效，仍指向 30
        cout << "sort 後 *it = " << *it << endl;
        cout << "sort 後 it 的地址 = " << &(*it) << endl;
        cout << "（地址相同 → 節點沒動，只是重新接線）" << endl;

        print_list("排序結果", lst);
    }

    // ===== 7. 效能比較：list::sort vs 複製到 vector 再 sort =====
    cout << "\n===== 效能比較 =====" << endl;
    {
        const int N = 500000;
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<int> dist(1, 1000000);

        // 建立隨機 list
        list<int> lst1;
        for (int i = 0; i < N; i++) {
            lst1.push_back(dist(gen));
        }
        list<int> lst2 = lst1;  // 複製一份

        // 方法 1：直接 list::sort
        auto start1 = chrono::high_resolution_clock::now();
        lst1.sort();
        auto end1 = chrono::high_resolution_clock::now();
        auto ms1 = chrono::duration_cast<chrono::milliseconds>(end1 - start1).count();

        // 方法 2：複製到 vector → sort → 複製回 list
        auto start2 = chrono::high_resolution_clock::now();
        vector<int> vec(lst2.begin(), lst2.end());
        std::sort(vec.begin(), vec.end());
        lst2.assign(vec.begin(), vec.end());
        auto end2 = chrono::high_resolution_clock::now();
        auto ms2 = chrono::duration_cast<chrono::milliseconds>(end2 - start2).count();

        cout << N << " 個元素排序：" << endl;
        cout << "  list::sort         : " << ms1 << " ms" << endl;
        cout << "  vector sort + 複製 : " << ms2 << " ms" << endl;

        if (ms1 > ms2) {
            cout << "  → vector 方案更快（快取效率的威力）" << endl;
        } else {
            cout << "  → list::sort 更快（少了複製的開銷）" << endl;
        }

        // 驗證兩者結果相同
        cout << "  結果一致：" << (lst1 == lst2 ? "是" : "否") << endl;
    }

    // ===== 8. 多鍵排序 =====
    cout << "\n===== 多鍵排序（利用穩定性） =====" << endl;
    {
        struct Employee {
            string department;
            string name;
            int salary;
        };

        list<Employee> employees = {
            {"工程", "Alice",   80000},
            {"行銷", "Bob",     70000},
            {"工程", "Charlie", 75000},
            {"行銷", "David",   70000},
            {"工程", "Eve",     80000},
            {"行銷", "Frank",   65000}
        };

        // 先按次要鍵排序（名字）
        employees.sort([](const Employee& a, const Employee& b) {
            return a.name < b.name;
        });

        // 再按主要鍵排序（部門）— 因為 sort 是穩定的，
        // 同部門內的名字順序會保持！
        employees.sort([](const Employee& a, const Employee& b) {
            return a.department < b.department;
        });

        cout << "按部門→名字排序：" << endl;
        for (const auto& e : employees) {
            cout << "  " << e.department << " | "
                 << e.name << " | $" << e.salary << endl;
        }
    }

    return 0;
}
