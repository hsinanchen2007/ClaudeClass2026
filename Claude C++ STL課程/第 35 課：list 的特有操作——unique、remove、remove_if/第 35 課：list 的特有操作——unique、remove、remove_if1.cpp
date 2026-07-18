// lesson35_list_unique_remove.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -o lesson35 lesson35_list_unique_remove.cpp

#include <iostream>
#include <list>
#include <string>
#include <cmath>
using namespace std;

template <typename T>
void print_list(const string& label, const list<T>& lst) {
    cout << label << " [" << lst.size() << "]: ";
    for (const auto& val : lst) cout << val << " ";
    cout << (lst.empty() ? "(空)" : "") << endl;
}

int main() {
    // ===== 1. remove 基本用法 =====
    cout << "===== remove 基本用法 =====" << endl;
    {
        list<int> lst = {1, 3, 2, 3, 4, 3, 5, 3};
        print_list("原始   ", lst);

        lst.remove(3);
        print_list("remove(3)", lst);
    }

    // ===== 2. remove 字串 =====
    cout << "\n===== remove 字串 =====" << endl;
    {
        list<string> words = {"apple", "banana", "apple", "cherry", "apple"};
        print_list("原始         ", words);

        words.remove("apple");
        print_list("remove(apple)", words);
    }

    // ===== 3. remove_if 基本用法 =====
    cout << "\n===== remove_if 基本用法 =====" << endl;
    {
        list<int> lst = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        print_list("原始       ", lst);

        // 移除所有偶數
        lst.remove_if([](int x) { return x % 2 == 0; });
        print_list("移除偶數   ", lst);
    }

    // ===== 4. remove_if 複雜條件 =====
    cout << "\n===== remove_if 複雜條件 =====" << endl;
    {
        struct Student {
            string name;
            double gpa;
        };

        list<Student> students = {
            {"Alice", 3.8}, {"Bob", 2.1}, {"Charlie", 3.5},
            {"David", 1.9}, {"Eve", 3.9}, {"Frank", 2.5}
        };

        cout << "原始：" << endl;
        for (const auto& s : students)
            cout << "  " << s.name << " GPA:" << s.gpa << endl;

        // 移除 GPA < 2.5 的學生
        students.remove_if([](const Student& s) {
            return s.gpa < 2.5;
        });

        cout << "移除 GPA < 2.5 後：" << endl;
        for (const auto& s : students)
            cout << "  " << s.name << " GPA:" << s.gpa << endl;
    }

    // ===== 5. unique 基本用法 =====
    cout << "\n===== unique 基本用法 =====" << endl;
    {
        list<int> lst = {1, 1, 2, 3, 3, 3, 2, 2, 1};
        print_list("原始         ", lst);

        lst.unique();
        print_list("unique 後    ", lst);
        cout << "（注意：非連續的重複仍然保留）" << endl;
    }

    // ===== 6. sort + unique = 移除所有重複 =====
    cout << "\n===== sort + unique = 完全去重 =====" << endl;
    {
        list<int> lst = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};
        print_list("原始         ", lst);

        lst.sort();
        print_list("sort 後      ", lst);

        lst.unique();
        print_list("unique 後    ", lst);
    }

    // ===== 7. unique 自訂條件 =====
    cout << "\n===== unique 自訂條件 =====" << endl;
    {
        list<double> lst = {1.0, 1.1, 1.2, 2.5, 2.6, 3.0, 3.3, 3.4};
        cout << "原始：";
        for (double val : lst) cout << val << " ";
        cout << endl;

        // 差距 < 0.5 視為重複
        lst.unique([](double a, double b) {
            return abs(a - b) < 0.5;
        });

        cout << "unique（差<0.5）：";
        for (double val : lst) cout << val << " ";
        cout << endl;
    }

    // ===== 8. unique 用在字串——忽略大小寫 =====
    cout << "\n===== unique 忽略大小寫 =====" << endl;
    {
        list<string> words = {"Hello", "hello", "HELLO", "World", "world", "Foo"};
        print_list("原始  ", words);

        auto case_insensitive_equal = [](const string& a, const string& b) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); i++) {
                if (tolower(a[i]) != tolower(b[i])) return false;
            }
            return true;
        };

        words.unique(case_insensitive_equal);
        print_list("unique", words);
    }

    // ===== 9. 迭代器穩定性驗證 =====
    cout << "\n===== 迭代器穩定性 =====" << endl;
    {
        list<int> lst = {10, 20, 30, 20, 40, 20, 50};

        // 保存指向 10 和 40 的迭代器
        auto it_10 = lst.begin();           // 指向 10
        auto it_40 = lst.begin();
        advance(it_40, 4);                  // 指向 40

        cout << "remove 前 *it_10=" << *it_10 << " *it_40=" << *it_40 << endl;

        lst.remove(20);  // 移除所有 20

        // 非被刪元素的迭代器仍然有效
        cout << "remove 後 *it_10=" << *it_10 << " *it_40=" << *it_40 << endl;
        print_list("結果  ", lst);
    }

    // ===== 10. 三者組合使用 =====
    cout << "\n===== 組合使用：清理資料 =====" << endl;
    {
        list<int> data = {-1, 5, 3, -2, 5, 0, 3, 8, -1, 5, 0, 3, 7};
        print_list("原始資料     ", data);

        // 步驟 1：移除負數
        data.remove_if([](int x) { return x < 0; });
        print_list("移除負數     ", data);

        // 步驟 2：移除特定值 0
        data.remove(0);
        print_list("移除 0       ", data);

        // 步驟 3：排序 + 去重
        data.sort();
        print_list("排序         ", data);

        data.unique();
        print_list("去重         ", data);
    }

    // ===== 11. remove vs 手動迴圈 erase 的對比 =====
    cout << "\n===== remove vs 手動 erase =====" << endl;
    {
        // 方法 1：用 remove（推薦）
        list<int> lst1 = {1, 2, 3, 2, 4, 2, 5};
        lst1.remove(2);
        print_list("remove(2)    ", lst1);

        // 方法 2：手動迴圈 erase（不推薦，但有時需要更複雜的邏輯）
        list<int> lst2 = {1, 2, 3, 2, 4, 2, 5};
        int count = 0;
        for (auto it = lst2.begin(); it != lst2.end(); ) {
            if (*it == 2) {
                it = lst2.erase(it);
                count++;
            } else {
                ++it;
            }
        }
        print_list("手動 erase   ", lst2);
        cout << "刪除了 " << count << " 個元素" << endl;

        // 方法 1 更簡潔，但方法 2 可以在刪除時做額外處理
        // 例如：記錄被刪除的元素、有條件地停止等
    }

    // ===== 12. remove_if 搭配外部狀態 =====
    cout << "\n===== remove_if 搭配外部狀態 =====" << endl;
    {
        list<int> lst = {5, 12, 3, 18, 7, 25, 9, 14};
        print_list("原始   ", lst);

        // 移除前 3 個大於 10 的元素
        int removed = 0;
        const int max_remove = 3;

        lst.remove_if([&removed, max_remove](int x) {
            if (removed >= max_remove) return false;
            if (x > 10) {
                removed++;
                return true;
            }
            return false;
        });

        print_list("結果   ", lst);
        cout << "移除了 " << removed << " 個大於 10 的元素" << endl;
    }

    return 0;
}
