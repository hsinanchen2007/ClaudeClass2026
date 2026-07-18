// ============================================================
// 第 32 課 總結：list 的特有操作——splice
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【splice = 接合】把節點從一個 list 移植到另一個，O(1)，零拷貝
//
// 【三個版本】
//   A.splice(pos, B)                  把 B 全部移到 A 的 pos 前
//   A.splice(pos, B, it)              把 B 的 it 一個節點移到 A
//   A.splice(pos, B, first, last)     把 B 的 [first,last) 移到 A
//
// 【同一 list 內也可以 splice（重排元素）】
//   lst.splice(lst.begin(), lst, it)  把 it 移到最前面
//
// 【關鍵特性】
//   ✅ 零拷貝：只改指標，不 new/delete
//   ✅ 迭代器全部有效（包括被移植的節點）
//   ✅ B 的 size 自動減少，A 自動增加
//
// 【典型應用】LRU Cache、優先級任務佇列
// ============================================================

#include <iostream>
#include <list>
#include <string>
#include <unordered_map>
using namespace std;

template <typename T>
void print(const string& label, const list<T>& lst) {
    cout << "  " << label << " [" << lst.size() << "]: ";
    for (const auto& v : lst) cout << v << " ";
    cout << (lst.empty() ? "(空)" : "") << endl;
}

int main() {
    // 1. 移植整個 list
    cout << "===== splice 整個 list =====\n";
    {
        list<int> A = {1,2,3}, B = {10,20,30};
        auto pos = A.begin(); advance(pos, 1);
        A.splice(pos, B);
        print("A", A);  // 1 10 20 30 2 3
        print("B", B);  // (空)
    }

    // 2. 移植單一元素
    cout << "\n===== splice 單一元素 =====\n";
    {
        list<int> A = {1,2,3}, B = {10,20,30};
        auto elem = B.begin(); advance(elem, 1);
        A.splice(A.end(), B, elem);
        print("A", A);  // 1 2 3 20
        print("B", B);  // 10 30
    }

    // 3. 移植範圍
    cout << "\n===== splice 範圍 =====\n";
    {
        list<int> A = {1,2,3}, B = {10,20,30,40,50};
        auto f = B.begin(); advance(f, 1);
        auto l = B.begin(); advance(l, 4);
        A.splice(A.end(), B, f, l);
        print("A", A);  // 1 2 3 20 30 40
        print("B", B);  // 10 50
    }

    // 4. 同一 list 內重排
    cout << "\n===== 同一 list 內 splice =====\n";
    {
        list<int> lst = {1,2,3,4,5};
        print("初始 ", lst);
        auto elem = lst.begin(); advance(elem, 3);
        lst.splice(lst.begin(), lst, elem);
        print("4→前面", lst);  // 4 1 2 3 5
    }

    // 5. 迭代器穩定性
    cout << "\n===== 迭代器穩定性 =====\n";
    {
        list<int> A = {1,2,3}, B = {10,20,30};
        auto it20 = B.begin(); advance(it20, 1);
        cout << "  splice 前 *it20=" << *it20 << "\n";
        A.splice(A.end(), B, it20);
        cout << "  splice 後 *it20=" << *it20 << " ✅ 仍有效\n";
        print("A", A);
    }

    // 6. 實際應用：LRU Cache
    cout << "\n===== LRU Cache =====\n";
    {
        const int CAP = 4;
        list<string> cache;
        unordered_map<string, list<string>::iterator> idx;

        auto access = [&](const string& key) {
            cout << "  存取 " << key << " → ";
            auto found = idx.find(key);
            if (found != idx.end()) {
                cache.splice(cache.begin(), cache, found->second);
                cout << "命中  ";
            } else {
                if ((int)cache.size() >= CAP) {
                    idx.erase(cache.back());
                    cache.pop_back();
                }
                cache.push_front(key);
                idx[key] = cache.begin();
                cout << "加入  ";
            }
            cout << "cache: ";
            for (auto& c : cache) cout << c << " ";
            cout << "\n";
        };

        access("A"); access("B"); access("C"); access("D");
        access("B");  // 命中 → splice 到前面
        access("E");  // 未命中 → 移除最舊的 A
    }

    return 0;
}
