// lesson39_forward_list_operations.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -o lesson39 lesson39_forward_list_operations.cpp

#include <iostream>
#include <forward_list>
#include <vector>
#include <string>
#include <iterator>
using namespace std;

template <typename T>
void print_flist(const string& label, const forward_list<T>& flst) {
    cout << label << ": ";
    for (const auto& val : flst) cout << val << " ";
    int n = (int)distance(flst.begin(), flst.end());
    cout << "(" << n << ")" << endl;
}

int main() {
    // ===== 1. insert_after 所有重載 =====
    cout << "===== insert_after 所有重載 =====" << endl;
    {
        forward_list<int> flst = {10, 40, 70};
        print_flist("初始          ", flst);

        // 1a. 單一元素
        auto it = flst.begin();   // 指向 10
        auto ret = flst.insert_after(it, 20);
        print_flist("insert_after 20", flst);
        cout << "  回傳 → " << *ret << endl;   // 20

        // 1b. n 個相同值
        ret = flst.insert_after(ret, 2, 30);
        print_flist("insert 2個30  ", flst);
        cout << "  回傳 → " << *ret << endl;   // 最後一個 30

        // 1c. 初始化列表
        ret = flst.insert_after(flst.before_begin(), {1, 2, 3});
        print_flist("insert{1,2,3} ", flst);
        cout << "  回傳 → " << *ret << endl;   // 3

        // 1d. 迭代器範圍
        vector<int> extra = {97, 98, 99};
        // 找到最後一個元素
        auto last_it = flst.before_begin();
        for (auto& _ : flst) { (void)_; ++last_it; }
        ret = flst.insert_after(last_it, extra.begin(), extra.end());
        print_flist("insert尾端    ", flst);
        cout << "  回傳 → " << *ret << endl;   // 99
    }

    // ===== 2. emplace_after =====
    cout << "\n===== emplace_after =====" << endl;
    {
        forward_list<pair<string, int>> scores;

        auto pos = scores.before_begin();
        pos = scores.emplace_after(pos, "Alice", 95);
        pos = scores.emplace_after(pos, "Bob", 88);
        pos = scores.emplace_after(pos, "Charlie", 92);

        cout << "emplace_after 鏈式建構：" << endl;
        for (const auto& [name, score] : scores) {
            cout << "  " << name << ": " << score << endl;
        }
        // 利用回傳值做鏈式追加 → 模擬 push_back 的效果！
    }

    // ===== 3. erase_after =====
    cout << "\n===== erase_after =====" << endl;
    {
        forward_list<int> flst = {10, 20, 30, 40, 50, 60, 70};
        print_flist("初始          ", flst);

        // 3a. 刪除單一元素
        auto it = flst.begin();   // 指向 10
        auto ret = flst.erase_after(it);
        print_flist("刪20(10之後)  ", flst);
        cout << "  回傳 → " << *ret << endl;   // 30

        // 3b. 刪除第一個元素
        flst.erase_after(flst.before_begin());
        print_flist("刪第一個      ", flst);

        // 3c. 刪除範圍
        auto before = flst.begin();          // 指向 30
        auto last = before;
        advance(last, 3);                    // 指向 70
        flst.erase_after(before, last);
        print_flist("刪(30,70)     ", flst);  // {30, 70}
    }

    // ===== 4. 迴圈中安全刪除 =====
    cout << "\n===== 迴圈中安全刪除 =====" << endl;
    {
        forward_list<int> flst = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        print_flist("原始          ", flst);

        // 方法 1：手動追蹤 prev（教學用）
        auto prev = flst.before_begin();
        auto curr = flst.begin();
        while (curr != flst.end()) {
            if (*curr % 3 == 0) {
                curr = flst.erase_after(prev);
            } else {
                prev = curr;
                ++curr;
            }
        }
        print_flist("刪3的倍數(手動)", flst);
    }
    {
        // 方法 2：remove_if（推薦）
        forward_list<int> flst = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        flst.remove_if([](int x) { return x % 3 == 0; });
        print_flist("刪3的倍數(rmif)", flst);
    }

    // ===== 5. splice_after =====
    cout << "\n===== splice_after =====" << endl;

    // 5a. 移植整個 forward_list
    {
        forward_list<int> A = {1, 2, 3};
        forward_list<int> B = {10, 20, 30};
        print_flist("A            ", A);
        print_flist("B            ", B);

        A.splice_after(A.before_begin(), B);
        print_flist("splice整個後 A", A);
        print_flist("splice整個後 B", B);
    }

    // 5b. 移植單一元素
    cout << endl;
    {
        forward_list<int> A = {1, 2, 3};
        forward_list<int> B = {10, 20, 30};
        print_flist("A            ", A);
        print_flist("B            ", B);

        auto before_20 = B.begin();   // 指向 10（20 之前）
        A.splice_after(A.begin(), B, before_20);
        // 移植 10 之後的元素（= 20）到 A 的 1 之後
        print_flist("splice 20後 A", A);
        print_flist("splice 20後 B", B);
    }

    // 5c. 移植範圍
    cout << endl;
    {
        forward_list<int> A = {1, 2, 3};
        forward_list<int> B = {10, 20, 30, 40, 50};
        print_flist("A            ", A);
        print_flist("B            ", B);

        auto before_first = B.begin();   // 指向 10
        auto last = before_first;
        advance(last, 4);               // 指向 50

        A.splice_after(A.before_begin(), B, before_first, last);
        print_flist("splice範圍 A ", A);
        print_flist("splice範圍 B ", B);
    }

    // ===== 6. 同一 forward_list 內 splice_after =====
    cout << "\n===== 同一 list 內移動元素 =====" << endl;
    {
        forward_list<int> flst = {1, 2, 3, 4, 5};
        print_flist("初始          ", flst);

        // 把 4 移到最前面
        // 4 在 3 之後 → before_it = 指向 3 的迭代器
        auto before_4 = flst.begin();
        advance(before_4, 2);    // 指向 3

        flst.splice_after(flst.before_begin(), flst, before_4);
        print_flist("4→最前面      ", flst);  // {4, 1, 2, 3, 5}
    }

    // ===== 7. 模擬 push_back =====
    cout << "\n===== 模擬 push_back =====" << endl;
    {
        forward_list<int> flst;

        // 方法：維護一個「尾端」迭代器
        auto tail = flst.before_begin();

        for (int i = 1; i <= 5; i++) {
            tail = flst.insert_after(tail, i * 10);
        }
        print_flist("模擬push_back ", flst);  // {10, 20, 30, 40, 50}
        cout << "  → 透過維護 tail 迭代器，O(1) 尾端追加" << endl;
    }

    // ===== 8. 迭代器穩定性 =====
    cout << "\n===== splice_after 迭代器穩定性 =====" << endl;
    {
        forward_list<int> A = {1, 2, 3};
        forward_list<int> B = {10, 20, 30};

        auto it_20 = next(B.begin());   // 指向 20
        cout << "splice 前 *it_20 = " << *it_20 << endl;

        auto before_20 = B.begin();     // 指向 10（20 之前）
        A.splice_after(A.begin(), B, before_20);

        cout << "splice 後 *it_20 = " << *it_20 << endl;
        print_flist("A", A);
        print_flist("B", B);
        cout << "→ 迭代器仍有效，但歸屬已改變" << endl;
    }

    return 0;
}
