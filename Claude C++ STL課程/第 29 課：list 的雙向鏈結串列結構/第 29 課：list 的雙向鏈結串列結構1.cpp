// lesson29_list_structure.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -o lesson29 lesson29_list_structure.cpp

#include <iostream>
#include <list>
#include <vector>
using namespace std;

int main() {
    // === 1. 基本建立 ===
    list<int> lst = {10, 20, 30, 40, 50};

    cout << "=== list 基本資訊 ===" << endl;
    cout << "size: " << lst.size() << endl;
    // cout << lst[0];   // 編譯錯誤！list 不支援 operator[]
    // cout << lst.at(0); // 編譯錯誤！list 不支援 at()

    // === 2. 遍歷方式 ===
    cout << "\n=== 遍歷 ===" << endl;

    // 方式一：範圍 for（最常用）
    cout << "範圍 for：";
    for (int val : lst) {
        cout << val << " ";
    }
    cout << endl;

    // 方式二：迭代器
    cout << "迭代器：  ";
    for (auto it = lst.begin(); it != lst.end(); ++it) {
        cout << *it << " ";
    }
    cout << endl;

    // 方式三：反向迭代器
    cout << "反向迭代：";
    for (auto rit = lst.rbegin(); rit != lst.rend(); ++rit) {
        cout << *rit << " ";
    }
    cout << endl;

    // === 3. 展示 list 不支援隨機存取 ===
    cout << "\n=== 迭代器類型限制 ===" << endl;
    auto it = lst.begin();

    // 可以用 ++ 和 -- （Bidirectional）
    ++it;  // 移到第 2 個元素
    cout << "++it → " << *it << endl;     // 20
    --it;  // 移回第 1 個元素
    cout << "--it → " << *it << endl;     // 10

    // 不能用 + 或 -（不是 Random Access）
    // auto it2 = it + 3;   // 編譯錯誤！
    // 要到第 4 個元素，必須用 std::advance
    advance(it, 3);  // 移動 3 步
    cout << "advance(it, 3) → " << *it << endl;  // 40

    // === 4. 展示元素地址不連續 ===
    cout << "\n=== 記憶體地址（不連續） ===" << endl;
    for (auto& val : lst) {
        cout << "值: " << val << "  地址: " << &val << endl;
    }

    // 對比 vector 的連續地址
    cout << "\n=== vector 記憶體地址（連續） ===" << endl;
    vector<int> vec = {10, 20, 30, 40, 50};
    for (auto& val : vec) {
        cout << "值: " << val << "  地址: " << &val << endl;
    }

    // === 5. 展示 O(1) 插入 ===
    cout << "\n=== O(1) 中間插入 ===" << endl;
    // 找到值為 30 的位置
    auto pos = lst.begin();
    while (*pos != 30) ++pos;

    // 在 30 前面插入 25
    lst.insert(pos, 25);

    cout << "在 30 前插入 25：";
    for (int val : lst) cout << val << " ";
    cout << endl;

    // 注意：pos 仍然有效！仍指向 30
    cout << "插入後 pos 仍指向：" << *pos << endl;  // 30

    // === 6. 展示 O(1) 刪除 ===
    cout << "\n=== O(1) 刪除 ===" << endl;
    // 刪除 pos 指向的元素（30）
    auto next_pos = lst.erase(pos);  // 回傳下一個元素的迭代器

    cout << "刪除 30 後：";
    for (int val : lst) cout << val << " ";
    cout << endl;

    cout << "erase 回傳的迭代器指向：" << *next_pos << endl;  // 40

    // === 7. front 和 back 操作 ===
    cout << "\n=== front / back ===" << endl;
    cout << "front: " << lst.front() << endl;
    cout << "back:  " << lst.back() << endl;

    return 0;
}
