// lesson31_list_operations.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -o lesson31 lesson31_list_operations.cpp

#include <iostream>
#include <list>
#include <string>
using namespace std;

template <typename T>
void print_list(const string& label, const list<T>& lst) {
    cout << label << " [" << lst.size() << "]: ";
    for (const auto& val : lst) cout << val << " ";
    cout << endl;
}

int main() {
    // ===== 1. 頭尾操作 =====
    cout << "===== 頭尾操作 =====" << endl;
    list<int> lst;

    lst.push_back(10);
    lst.push_back(20);
    lst.push_back(30);
    print_list("push_back x3    ", lst);

    lst.push_front(5);
    lst.push_front(1);
    print_list("push_front x2   ", lst);

    lst.pop_back();
    print_list("pop_back        ", lst);

    lst.pop_front();
    print_list("pop_front       ", lst);

    // front / back
    cout << "front: " << lst.front() << ", back: " << lst.back() << endl;

    // ===== 2. emplace 系列 =====
    cout << "\n===== emplace 系列 =====" << endl;
    list<pair<string, int>> scores;

    scores.emplace_back("Alice", 95);
    scores.emplace_back("Bob", 88);
    scores.emplace_front("Charlie", 92);

    for (const auto& [name, score] : scores) {
        cout << "  " << name << ": " << score << endl;
    }

    // ===== 3. insert 的各種重載 =====
    cout << "\n===== insert 操作 =====" << endl;
    list<int> lst2 = {10, 30, 50};
    print_list("初始            ", lst2);

    // 3a. 插入單一元素
    auto it = lst2.begin();
    advance(it, 1);  // 指向 30
    auto ret = lst2.insert(it, 20);
    print_list("insert(30前,20) ", lst2);
    cout << "  回傳迭代器指向: " << *ret << endl;
    cout << "  it 仍指向:      " << *it << endl;   // 30（未失效）

    // 3b. 插入 n 個相同值
    it = lst2.end();
    lst2.insert(it, 3, 99);
    print_list("insert(end,3,99)", lst2);

    // 3c. 插入初始化列表
    lst2.insert(lst2.begin(), {-2, -1, 0});
    print_list("insert({-2,-1,0})", lst2);

    // ===== 4. erase 操作 =====
    cout << "\n===== erase 操作 =====" << endl;
    list<int> lst3 = {10, 20, 30, 40, 50, 60, 70};
    print_list("初始            ", lst3);

    // 4a. 刪除單一元素
    auto eit = lst3.begin();
    advance(eit, 2);  // 指向 30
    auto next = lst3.erase(eit);
    print_list("erase(30)       ", lst3);
    cout << "  回傳迭代器指向: " << *next << endl;  // 40

    // 4b. 刪除範圍
    auto first = lst3.begin();
    auto last = lst3.begin();
    advance(first, 1);  // 指向 20
    advance(last, 3);   // 指向 60
    lst3.erase(first, last);
    print_list("erase(20~60)    ", lst3);  // 刪除 20, 40, 50

    // ===== 5. 迭代器穩定性展示 =====
    cout << "\n===== 迭代器穩定性 =====" << endl;
    list<int> lst4 = {100, 200, 300, 400, 500};

    auto it200 = lst4.begin();
    advance(it200, 1);
    auto it400 = lst4.begin();
    advance(it400, 3);

    cout << "插入前 *it200=" << *it200 << " *it400=" << *it400 << endl;

    // 在 200 和 300 之間插入 250
    auto it300 = lst4.begin();
    advance(it300, 2);
    lst4.insert(it300, 250);

    cout << "插入後 *it200=" << *it200 << " *it400=" << *it400 << endl;
    print_list("插入 250 後     ", lst4);

    // 刪除 300
    lst4.erase(it300);
    cout << "刪除後 *it200=" << *it200 << " *it400=" << *it400 << endl;
    print_list("刪除 300 後     ", lst4);

    // ===== 6. 迴圈中安全刪除 =====
    cout << "\n===== 迴圈中安全刪除 =====" << endl;
    list<int> lst5 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    print_list("刪除前（偶數）  ", lst5);

    for (auto it = lst5.begin(); it != lst5.end(); ) {
        if (*it % 2 == 0) {
            it = lst5.erase(it);   // erase 回傳下一個
        } else {
            ++it;
        }
    }
    print_list("刪除偶數後      ", lst5);

    // ===== 7. resize =====
    cout << "\n===== resize =====" << endl;
    list<int> lst6 = {10, 20, 30, 40, 50};
    print_list("初始            ", lst6);

    lst6.resize(3);
    print_list("resize(3)       ", lst6);

    lst6.resize(6, 99);
    print_list("resize(6, 99)   ", lst6);

    // ===== 8. clear =====
    cout << "\n===== clear =====" << endl;
    lst6.clear();
    print_list("clear 後        ", lst6);
    cout << "empty: " << (lst6.empty() ? "是" : "否") << endl;

    return 0;
}
