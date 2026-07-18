// ============================================================
// 第 36 課 總結：list 的特有操作——reverse
// 編譯：g++ -std=c++17 -O2 -o summary summary.cpp
// ============================================================
// 【list::reverse()】反轉整個 list，O(n)
//   ★ 只改每個節點的 prev/next 指標，不搬移資料
//   ★ 迭代器全部有效（值不變、地址不變、在 list 中位置改變）
//
// 【list::reverse vs std::reverse】
//   list::reverse：改指標連線 → 迭代器值不變，位置改變
//   std::reverse ：交換元素值 → 迭代器位置不變，值改變
//   大型物件時 list::reverse 快很多（不搬移資料）
// ============================================================

#include <iostream>
#include <list>
#include <algorithm>
#include <chrono>
using namespace std;

template <typename T>
void print(const string& label, const list<T>& lst) {
    cout << "  " << label << ": ";
    for (const auto& v : lst) cout << v << " ";
    cout << endl;
}

int main() {
    // 1. 基本 reverse
    cout << "===== 基本 reverse =====\n";
    {
        list<int> lst = {1,2,3,4,5};
        print("反轉前", lst);
        lst.reverse();
        print("反轉後", lst);
    }

    // 2. 迭代器穩定性
    cout << "\n===== 迭代器穩定性 =====\n";
    {
        list<int> lst = {10,20,30,40,50};
        auto it20 = next(lst.begin());
        cout << "  reverse 前 *it20=" << *it20 << " addr=" << &(*it20) << "\n";
        lst.reverse();
        cout << "  reverse 後 *it20=" << *it20 << " addr=" << &(*it20) << "\n";
        cout << "  （值和地址不變，位置從第2變成倒數第2）\n";
    }

    // 3. list::reverse vs std::reverse
    cout << "\n===== list::reverse vs std::reverse =====\n";
    {
        list<int> lst1 = {10,20,30,40,50};
        auto it1 = next(lst1.begin());
        cout << "  list::reverse 前 *it=" << *it1 << "\n";
        lst1.reverse();
        cout << "  list::reverse 後 *it=" << *it1 << "（值不變，位置改變）\n";

        list<int> lst2 = {10,20,30,40,50};
        auto it2 = next(lst2.begin());
        cout << "  std::reverse  前 *it=" << *it2 << "\n";
        std::reverse(lst2.begin(), lst2.end());
        cout << "  std::reverse  後 *it=" << *it2 << "（值改變，位置不變）\n";
    }

    // 4. 大型物件效能比較
    cout << "\n===== 大型物件效能 =====\n";
    {
        struct Big { char data[1024]; int id; Big(int i=0):id(i){data[0]=0;} };
        const int N = 200000;
        list<Big> lst;
        for (int i = 0; i < N; i++) lst.emplace_back(i);

        auto t1 = chrono::high_resolution_clock::now();
        lst.reverse();
        auto t2 = chrono::high_resolution_clock::now();
        auto us1 = chrono::duration_cast<chrono::microseconds>(t2-t1).count();

        auto t3 = chrono::high_resolution_clock::now();
        std::reverse(lst.begin(), lst.end());
        auto t4 = chrono::high_resolution_clock::now();
        auto us2 = chrono::duration_cast<chrono::microseconds>(t4-t3).count();

        cout << "  " << N << " 個 1KB 物件：\n";
        cout << "    list::reverse: " << us1 << " us\n";
        cout << "    std::reverse:  " << us2 << " us\n";
    }

    // 5. 迴文檢測
    cout << "\n===== 迴文檢測 =====\n";
    {
        auto isPalin = [](const list<int>& lst) {
            list<int> rev = lst; rev.reverse(); return lst == rev;
        };
        list<int> a = {1,2,3,2,1}, b = {1,2,3,4,5};
        cout << "  {1,2,3,2,1} 迴文? " << (isPalin(a)?"是":"否") << "\n";
        cout << "  {1,2,3,4,5} 迴文? " << (isPalin(b)?"是":"否") << "\n";
    }

    // 6. reverse + splice 組合：反轉後半段
    cout << "\n===== reverse + splice =====\n";
    {
        list<int> lst = {1,2,3,4,5,6,7,8};
        print("原始    ", lst);
        auto mid = lst.begin(); advance(mid, 4);
        list<int> half2;
        half2.splice(half2.begin(), lst, mid, lst.end());
        half2.reverse();
        lst.splice(lst.end(), half2);
        print("後半反轉", lst);  // 1 2 3 4 8 7 6 5
    }

    return 0;
}
