// ============================================================
// 第 31 課 總結：list 的元素操作——push、pop、insert、erase
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【頭尾操作】O(1)
//   push_back(val)  / emplace_back(args...)   尾端插入
//   push_front(val) / emplace_front(args...)  頭端插入
//   pop_back() / pop_front()                  刪除首尾
//   front() / back()                          存取首尾
//
// 【insert】在指定位置前插入，回傳指向新元素的迭代器
//   insert(pos, val)        單一值
//   insert(pos, n, val)     n 個相同值
//   insert(pos, {init})     初始化列表
//   insert(pos, first, last) 範圍
//   ★ 其他迭代器不失效
//
// 【erase】刪除指定元素，回傳下一個元素的迭代器
//   erase(pos)              單一元素
//   erase(first, last)      範圍 [first, last)
//   ★ 只有被刪元素的迭代器失效
//
// 【迴圈中安全刪除】
//   for (auto it = lst.begin(); it != lst.end(); ) {
//       if (condition) it = lst.erase(it);  // erase 回傳下一個
//       else ++it;
//   }
// ============================================================

#include <iostream>
#include <list>
#include <string>
using namespace std;

template <typename T>
void print(const string& label, const list<T>& lst) {
    cout << "  " << label << " [" << lst.size() << "]: ";
    for (const auto& v : lst) cout << v << " ";
    cout << endl;
}

int main() {
    // 1. 頭尾操作
    cout << "===== 頭尾操作 =====\n";
    list<int> lst;
    lst.push_back(10); lst.push_back(20); lst.push_back(30);
    print("push_back ×3 ", lst);
    lst.push_front(5); lst.push_front(1);
    print("push_front ×2", lst);  // 1 5 10 20 30
    lst.pop_front(); lst.pop_back();
    print("pop 首尾     ", lst);  // 5 10 20
    cout << "  front=" << lst.front() << " back=" << lst.back() << "\n\n";

    // 2. emplace 系列
    cout << "===== emplace =====\n";
    list<pair<string,int>> scores;
    scores.emplace_back("Alice", 95);
    scores.emplace_front("Bob", 88);
    for (const auto& [n, s] : scores) cout << "  " << n << ":" << s << "\n";
    cout << "\n";

    // 3. insert
    cout << "===== insert =====\n";
    list<int> lst2 = {10, 30, 50};
    print("初始         ", lst2);
    auto it = lst2.begin(); advance(it, 1);
    auto ret = lst2.insert(it, 20);
    print("insert 20    ", lst2);
    cout << "  回傳→" << *ret << " it仍→" << *it << "\n";
    lst2.insert(lst2.end(), 3, 99);
    print("insert 3×99  ", lst2);
    lst2.insert(lst2.begin(), {-2, -1, 0});
    print("insert{-2..0}", lst2);
    cout << "\n";

    // 4. erase
    cout << "===== erase =====\n";
    list<int> lst3 = {10, 20, 30, 40, 50, 60, 70};
    print("初始         ", lst3);
    auto eit = lst3.begin(); advance(eit, 2);
    auto nxt = lst3.erase(eit);
    print("erase(30)    ", lst3);
    cout << "  回傳→" << *nxt << "\n";

    auto f = lst3.begin(); advance(f, 1);
    auto l = lst3.begin(); advance(l, 3);
    lst3.erase(f, l);
    print("erase 範圍   ", lst3);
    cout << "\n";

    // 5. 迭代器穩定性
    cout << "===== 迭代器穩定性 =====\n";
    list<int> lst4 = {100, 200, 300, 400, 500};
    auto it200 = lst4.begin(); advance(it200, 1);
    auto it400 = lst4.begin(); advance(it400, 3);
    auto it300 = lst4.begin(); advance(it300, 2);
    lst4.insert(it300, 250);
    cout << "  插入後 *it200=" << *it200 << " *it400=" << *it400 << " ✅\n";
    lst4.erase(it300);
    cout << "  刪300後 *it200=" << *it200 << " *it400=" << *it400 << " ✅\n\n";

    // 6. 迴圈中安全刪除
    cout << "===== 迴圈中安全刪除偶數 =====\n";
    list<int> lst5 = {1,2,3,4,5,6,7,8,9,10};
    print("刪前         ", lst5);
    for (auto it = lst5.begin(); it != lst5.end(); ) {
        if (*it % 2 == 0) it = lst5.erase(it);
        else ++it;
    }
    print("刪偶數後     ", lst5);

    // 7. resize / clear
    cout << "\n===== resize / clear =====\n";
    list<int> lst6 = {10,20,30,40,50};
    lst6.resize(3);        print("resize(3)    ", lst6);
    lst6.resize(6, 99);    print("resize(6,99) ", lst6);
    lst6.clear();          print("clear        ", lst6);

    return 0;
}
