// lesson33_list_merge.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -o lesson33 lesson33_list_merge.cpp

#include <iostream>
#include <list>
#include <string>
#include <functional>    // greater
using namespace std;

template <typename T>
void print_list(const string& label, const list<T>& lst) {
    cout << label << " [" << lst.size() << "]: ";
    for (const auto& val : lst) cout << val << " ";
    cout << (lst.empty() ? "(空)" : "") << endl;
}

int main() {
    // ===== 1. 基本 merge（升序） =====
    cout << "===== 基本 merge（升序） =====" << endl;
    {
        list<int> A = {2, 5, 8, 10};
        list<int> B = {1, 3, 6, 7, 9};
        print_list("A", A);
        print_list("B", B);

        A.merge(B);
        print_list("merge 後 A", A);
        print_list("merge 後 B", B);
    }

    // ===== 2. 降序 merge =====
    cout << "\n===== 降序 merge =====" << endl;
    {
        list<int> A = {10, 8, 5, 2};
        list<int> B = {9, 7, 6, 3, 1};
        print_list("A", A);
        print_list("B", B);

        A.merge(B, greater<int>());
        print_list("merge 後 A", A);
        print_list("merge 後 B", B);
    }

    // ===== 3. 穩定性驗證 =====
    cout << "\n===== 穩定性驗證 =====" << endl;
    {
        struct Item {
            string source;
            int value;
        };

        list<Item> A = {{"A", 1}, {"A", 3}, {"A", 5}};
        list<Item> B = {{"B", 1}, {"B", 3}, {"B", 4}};

        cout << "A: ";
        for (const auto& item : A) cout << item.source << item.value << " ";
        cout << endl;

        cout << "B: ";
        for (const auto& item : B) cout << item.source << item.value << " ";
        cout << endl;

        A.merge(B, [](const Item& a, const Item& b) {
            return a.value < b.value;
        });

        cout << "merge 後：";
        for (const auto& item : A) cout << item.source << item.value << " ";
        cout << endl;
        cout << "（相同值時，A 的元素排在 B 前面 → 穩定）" << endl;
    }

    // ===== 4. 未排序的 list 做 merge（錯誤示範） =====
    cout << "\n===== 未排序 merge（錯誤示範） =====" << endl;
    {
        list<int> A = {5, 2, 8};    // 未排序！
        list<int> B = {3, 9, 1};    // 未排序！
        print_list("A（未排序）", A);
        print_list("B（未排序）", B);

        A.merge(B);  // 不會崩潰，但結果不正確
        print_list("merge 結果 ", A);
        cout << "（結果不是排序的！因為輸入未排序）" << endl;
    }

    // ===== 5. 正確做法：先 sort 再 merge =====
    cout << "\n===== 正確做法：先 sort 再 merge =====" << endl;
    {
        list<int> A = {5, 2, 8};
        list<int> B = {3, 9, 1};
        print_list("A（未排序）", A);
        print_list("B（未排序）", B);

        A.sort();    // 先排序 → {2, 5, 8}
        B.sort();    // 先排序 → {1, 3, 9}
        print_list("sort 後 A  ", A);
        print_list("sort 後 B  ", B);

        A.merge(B);
        print_list("merge 後 A ", A);
        cout << "（結果正確排序！）" << endl;
    }

    // ===== 6. 自訂物件 merge =====
    cout << "\n===== 自訂物件 merge =====" << endl;
    {
        struct Student {
            string name;
            double gpa;
        };

        // 兩班學生，各自按 GPA 降序排列
        list<Student> classA = {{"Alice", 3.9}, {"Charlie", 3.5}, {"Eve", 3.2}};
        list<Student> classB = {{"Bob", 3.8}, {"David", 3.6}, {"Frank", 3.0}};

        auto by_gpa_desc = [](const Student& a, const Student& b) {
            return a.gpa > b.gpa;
        };

        cout << "A 班：";
        for (const auto& s : classA) cout << s.name << "(" << s.gpa << ") ";
        cout << endl;

        cout << "B 班：";
        for (const auto& s : classB) cout << s.name << "(" << s.gpa << ") ";
        cout << endl;

        classA.merge(classB, by_gpa_desc);

        cout << "合併（GPA 降序）：" << endl;
        for (const auto& s : classA) {
            cout << "  " << s.name << " - GPA: " << s.gpa << endl;
        }
    }

    // ===== 7. 多路合併（merge 多個 list） =====
    cout << "\n===== 多路合併 =====" << endl;
    {
        list<int> lists[] = {
            {1, 5, 9},
            {2, 6, 10},
            {3, 7, 11},
            {4, 8, 12}
        };

        cout << "合併前：" << endl;
        for (int i = 0; i < 4; i++) {
            cout << "  list " << i << ": ";
            for (int val : lists[i]) cout << val << " ";
            cout << endl;
        }

        // 逐一合併到 lists[0]
        for (int i = 1; i < 4; i++) {
            lists[0].merge(lists[i]);
        }

        print_list("合併結果", lists[0]);
    }

    // ===== 8. merge 的迭代器穩定性 =====
    cout << "\n===== merge 的迭代器穩定性 =====" << endl;
    {
        list<int> A = {2, 5, 8};
        list<int> B = {1, 3, 6};

        auto it_5 = A.begin();
        advance(it_5, 1);         // 指向 A 的 5
        auto it_3 = B.begin();
        advance(it_3, 1);         // 指向 B 的 3

        cout << "merge 前 *it_5=" << *it_5 << " *it_3=" << *it_3 << endl;

        A.merge(B);

        // 迭代器仍然有效！
        cout << "merge 後 *it_5=" << *it_5 << " *it_3=" << *it_3 << endl;
        print_list("A", A);
        print_list("B", B);
    }

    return 0;
}
