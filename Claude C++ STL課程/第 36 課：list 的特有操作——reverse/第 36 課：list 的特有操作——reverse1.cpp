// lesson36_list_reverse.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -o lesson36 lesson36_list_reverse.cpp

#include <iostream>
#include <list>
#include <string>
#include <algorithm>
#include <chrono>
using namespace std;

template <typename T>
void print_list(const string& label, const list<T>& lst) {
    cout << label << " [" << lst.size() << "]: ";
    for (const auto& val : lst) cout << val << " ";
    cout << (lst.empty() ? "(空)" : "") << endl;
}

int main() {
    // ===== 1. 基本 reverse =====
    cout << "===== 基本 reverse =====" << endl;
    {
        list<int> lst = {1, 2, 3, 4, 5};
        print_list("反轉前", lst);

        lst.reverse();
        print_list("反轉後", lst);
    }

    // ===== 2. 邊界情況 =====
    cout << "\n===== 邊界情況 =====" << endl;
    {
        // 空 list
        list<int> empty_lst;
        empty_lst.reverse();    // 不會崩潰
        print_list("空 list   ", empty_lst);

        // 單元素 list
        list<int> single = {42};
        single.reverse();       // 沒有效果
        print_list("單元素    ", single);

        // 兩元素 list
        list<int> two = {10, 20};
        two.reverse();
        print_list("兩元素反轉", two);
    }

    // ===== 3. 迭代器穩定性驗證 =====
    cout << "\n===== 迭代器穩定性 =====" << endl;
    {
        list<int> lst = {10, 20, 30, 40, 50};

        auto it_20 = lst.begin();
        advance(it_20, 1);    // 指向 20
        auto it_40 = lst.begin();
        advance(it_40, 3);    // 指向 40

        cout << "reverse 前：" << endl;
        cout << "  *it_20 = " << *it_20 << "  地址: " << &(*it_20) << endl;
        cout << "  *it_40 = " << *it_40 << "  地址: " << &(*it_40) << endl;

        lst.reverse();
        print_list("reverse 後 list", lst);

        cout << "reverse 後：" << endl;
        cout << "  *it_20 = " << *it_20 << "  地址: " << &(*it_20) << endl;
        cout << "  *it_40 = " << *it_40 << "  地址: " << &(*it_40) << endl;
        cout << "（值和地址都不變！只有在 list 中的位置改變了）" << endl;

        // 驗證：it_20 現在是倒數第 2 個
        auto check = lst.end();
        advance(check, -2);
        cout << "  倒數第 2 個元素 = " << *check << endl;
        cout << "  it_20 == 倒數第2? " << (check == it_20 ? "是" : "否") << endl;
    }

    // ===== 4. list::reverse vs std::reverse 行為差異 =====
    cout << "\n===== list::reverse vs std::reverse =====" << endl;
    {
        // list::reverse
        list<int> lst = {10, 20, 30, 40, 50};
        auto it_lst = lst.begin();
        advance(it_lst, 1);
        cout << "list::reverse:" << endl;
        cout << "  反轉前 *it = " << *it_lst << endl;
        lst.reverse();
        cout << "  反轉後 *it = " << *it_lst << "（值不變，位置改變）" << endl;

        // std::reverse on list（可以用，但語意不同）
        list<int> lst2 = {10, 20, 30, 40, 50};
        auto it_lst2 = lst2.begin();
        advance(it_lst2, 1);
        cout << "\nstd::reverse on list:" << endl;
        cout << "  反轉前 *it = " << *it_lst2 << endl;
        std::reverse(lst2.begin(), lst2.end());
        cout << "  反轉後 *it = " << *it_lst2 << "（值改變，位置不變）" << endl;

        print_list("  list::reverse 結果", lst);
        print_list("  std::reverse 結果 ", lst2);
    }

    // ===== 5. 大型物件的效能比較 =====
    cout << "\n===== 大型物件效能比較 =====" << endl;
    {
        // 模擬大型物件
        struct BigObject {
            char data[1024];   // 1KB
            int id;
            BigObject(int i = 0) : id(i) { data[0] = 0; }
        };

        const int N = 200000;
        list<BigObject> lst;
        for (int i = 0; i < N; i++) {
            lst.emplace_back(i);
        }

        // 測試 list::reverse
        auto start1 = chrono::high_resolution_clock::now();
        lst.reverse();
        auto end1 = chrono::high_resolution_clock::now();
        auto us1 = chrono::duration_cast<chrono::microseconds>(end1 - start1).count();

        // 測試 std::reverse
        auto start2 = chrono::high_resolution_clock::now();
        std::reverse(lst.begin(), lst.end());
        auto end2 = chrono::high_resolution_clock::now();
        auto us2 = chrono::duration_cast<chrono::microseconds>(end2 - start2).count();

        cout << N << " 個 1KB 物件反轉：" << endl;
        cout << "  list::reverse : " << us1 << " us" << endl;
        cout << "  std::reverse  : " << us2 << " us" << endl;

        if (us2 > us1) {
            cout << "  → list::reverse 快 " << (us2 * 100 / us1 - 100) << "%" << endl;
        } else {
            cout << "  → std::reverse 快（小物件或編譯器優化）" << endl;
        }

        // 驗證正確性：第一個元素應該是 id = 0（反轉兩次回到原始順序）
        cout << "  驗證 first.id = " << lst.front().id << " (應為 0)" << endl;
    }

    // ===== 6. 實際應用：迴文檢測 =====
    cout << "\n===== 迴文檢測 =====" << endl;
    {
        auto is_palindrome = [](const list<int>& lst) {
            list<int> rev = lst;
            rev.reverse();
            return lst == rev;
        };

        list<int> lst1 = {1, 2, 3, 2, 1};
        list<int> lst2 = {1, 2, 3, 4, 5};
        list<int> lst3 = {1};
        list<int> lst4 = {};

        print_list("{1,2,3,2,1}", lst1);
        cout << "  迴文? " << (is_palindrome(lst1) ? "是" : "否") << endl;

        print_list("{1,2,3,4,5}", lst2);
        cout << "  迴文? " << (is_palindrome(lst2) ? "是" : "否") << endl;

        print_list("{1}        ", lst3);
        cout << "  迴文? " << (is_palindrome(lst3) ? "是" : "否") << endl;

        print_list("{}         ", lst4);
        cout << "  迴文? " << (is_palindrome(lst4) ? "是" : "否") << endl;
    }

    // ===== 7. 組合技：reverse + splice =====
    cout << "\n===== 組合技：reverse + splice =====" << endl;
    {
        list<int> lst = {1, 2, 3, 4, 5, 6, 7, 8};
        print_list("原始    ", lst);

        // 反轉後半段
        // 步驟：切出後半段 → reverse → splice 回來
        auto mid = lst.begin();
        advance(mid, 4);   // 指向 5

        list<int> second_half;
        second_half.splice(second_half.begin(), lst, mid, lst.end());
        print_list("前半段  ", lst);
        print_list("後半段  ", second_half);

        second_half.reverse();
        print_list("反轉後半", second_half);

        lst.splice(lst.end(), second_half);
        print_list("合併結果", lst);
        // {1, 2, 3, 4, 8, 7, 6, 5}
    }

    return 0;
}
