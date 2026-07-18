// lesson32_list_splice.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -o lesson32 lesson32_list_splice.cpp

#include <iostream>
#include <list>
#include <string>
#include <unordered_map>
using namespace std;

template <typename T>
void print_list(const string& label, const list<T>& lst) {
    cout << label << " [" << lst.size() << "]: ";
    for (const auto& val : lst) cout << val << " ";
    cout << (lst.empty() ? "(空)" : "") << endl;
}

int main() {
    // ===== 1. splice 版本 1：移植整個 list =====
    cout << "===== splice 版本 1：移植整個 list =====" << endl;
    {
        list<int> A = {1, 2, 3};
        list<int> B = {10, 20, 30};
        print_list("A ", A);
        print_list("B ", B);

        auto pos = A.begin();
        advance(pos, 1);   // 指向 2

        A.splice(pos, B);  // 把 B 全部移到 2 前面
        print_list("splice 後 A", A);
        print_list("splice 後 B", B);
    }

    // ===== 2. splice 版本 2：移植單一元素 =====
    cout << "\n===== splice 版本 2：移植單一元素 =====" << endl;
    {
        list<int> A = {1, 2, 3};
        list<int> B = {10, 20, 30};
        print_list("A ", A);
        print_list("B ", B);

        auto pos = A.end();          // 插入到 A 的尾端
        auto elem = B.begin();
        advance(elem, 1);           // 指向 B 的 20

        A.splice(pos, B, elem);     // 把 B 的 20 移到 A 的尾端
        print_list("splice 後 A", A);
        print_list("splice 後 B", B);
    }

    // ===== 3. splice 版本 3：移植一段範圍 =====
    cout << "\n===== splice 版本 3：移植一段範圍 =====" << endl;
    {
        list<int> A = {1, 2, 3};
        list<int> B = {10, 20, 30, 40, 50};
        print_list("A ", A);
        print_list("B ", B);

        auto first = B.begin();
        advance(first, 1);          // 指向 20
        auto last = B.begin();
        advance(last, 4);           // 指向 50

        A.splice(A.end(), B, first, last);  // 移植 {20, 30, 40}
        print_list("splice 後 A", A);
        print_list("splice 後 B", B);
    }

    // ===== 4. 同一 list 內 splice（重新排序元素） =====
    cout << "\n===== 同一 list 內 splice =====" << endl;
    {
        list<int> lst = {1, 2, 3, 4, 5};
        print_list("初始  ", lst);

        // 把 4 移到最前面
        auto elem = lst.begin();
        advance(elem, 3);   // 指向 4
        lst.splice(lst.begin(), lst, elem);
        print_list("4→前面", lst);

        // 把 2 移到最後面
        elem = lst.begin();
        advance(elem, 2);   // 現在是 {4,1,2,3,5}，第3個是 2
        lst.splice(lst.end(), lst, elem);
        print_list("2→後面", lst);
    }

    // ===== 5. 迭代器穩定性驗證 =====
    cout << "\n===== 迭代器穩定性 =====" << endl;
    {
        list<int> A = {1, 2, 3};
        list<int> B = {10, 20, 30};

        // 保存迭代器
        auto it_20 = B.begin();
        advance(it_20, 1);     // 指向 B 的 20

        cout << "splice 前 *it_20 = " << *it_20 << endl;

        // 把 20 從 B 移到 A
        A.splice(A.end(), B, it_20);

        // it_20 仍然有效！
        cout << "splice 後 *it_20 = " << *it_20 << endl;  // 仍然是 20

        print_list("A", A);
        print_list("B", B);
    }

    // ===== 6. 實際應用：簡易 LRU Cache =====
    cout << "\n===== 實際應用：LRU Cache =====" << endl;
    {
        const int CAPACITY = 4;
        list<string> cache;                                    // 前面 = 最近使用
        unordered_map<string, list<string>::iterator> index;   // 快速查找

        auto access = [&](const string& key) {
            cout << "  存取 " << key << " → ";

            auto found = index.find(key);
            if (found != index.end()) {
                // 命中：用 splice 移到最前面
                cache.splice(cache.begin(), cache, found->second);
                cout << "命中！";
            } else {
                // 未命中：加入最前面
                if ((int)cache.size() >= CAPACITY) {
                    // 滿了，移除最後一個
                    cout << "移除 " << cache.back() << "，";
                    index.erase(cache.back());
                    cache.pop_back();
                }
                cache.push_front(key);
                index[key] = cache.begin();
                cout << "加入。";
            }

            // 顯示快取狀態
            cout << " cache: ";
            for (const auto& item : cache) cout << item << " ";
            cout << endl;
        };

        access("A");   // 加入
        access("B");   // 加入
        access("C");   // 加入
        access("D");   // 加入（滿了）
        access("B");   // 命中 → splice 到前面
        access("E");   // 未命中 → 移除最舊的 A
        access("C");   // 命中 → splice 到前面
        access("F");   // 未命中 → 移除最舊的 D
    }

    // ===== 7. 實際應用：多優先級任務佇列 =====
    cout << "\n===== 實際應用：多優先級任務佇列 =====" << endl;
    {
        list<string> high, medium, low;

        medium.push_back("寫報告");
        medium.push_back("開會");
        low.push_back("整理桌面");
        high.push_back("修復 Bug");
        medium.push_back("Code Review");

        auto print_all = [&]() {
            print_list("  HIGH  ", high);
            print_list("  MEDIUM", medium);
            print_list("  LOW   ", low);
        };

        cout << "初始狀態：" << endl;
        print_all();

        // 「開會」升級為 high
        auto it = medium.begin();
        advance(it, 1);  // 指向「開會」
        high.splice(high.end(), medium, it);

        cout << "\n「開會」升為 HIGH：" << endl;
        print_all();

        // 「修復 Bug」完成，移除
        high.pop_front();

        cout << "\n「修復 Bug」完成：" << endl;
        print_all();

        // 把 low 的所有任務併入 medium
        medium.splice(medium.end(), low);

        cout << "\nLOW 全部併入 MEDIUM：" << endl;
        print_all();
    }

    return 0;
}
