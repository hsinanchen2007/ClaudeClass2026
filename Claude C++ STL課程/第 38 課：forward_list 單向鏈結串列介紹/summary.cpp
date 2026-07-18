// ============================================================
// 第 38 課 總結：forward_list 單向鏈結串列介紹
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【forward_list = 單向鏈結串列】每個節點只有 next 指標
//
// 【vs list 的差異】
//   ┌────────────────┬──────────────┬──────────────┐
//   │                │ list         │ forward_list │
//   ├────────────────┼──────────────┼──────────────┤
//   │ 指標           │ prev + next  │ next only    │
//   │ 迭代器方向     │ 雙向         │ 只能前進     │
//   │ size()         │ O(1) ✅      │ ❌ 沒有！    │
//   │ push_back      │ O(1) ✅      │ ❌ 沒有！    │
//   │ push_front     │ O(1) ✅      │ O(1) ✅      │
//   │ back()         │ O(1) ✅      │ ❌ 沒有！    │
//   │ insert         │ insert(pos)  │ insert_after │
//   │ erase          │ erase(pos)   │ erase_after  │
//   │ before_begin() │ ❌           │ ✅ 特有      │
//   │ 每節點記憶體   │ 24 bytes     │ 16 bytes     │
//   └────────────────┴──────────────┴──────────────┘
//
// 【何時用 forward_list？】
//   記憶體敏感 + 只需前向遍歷 + 只需頭端操作
//   實務上很少用，大多數場景 list 或 vector 更好
// ============================================================

#include <iostream>
#include <forward_list>
#include <list>
#include <vector>
#include <string>
#include <iterator>
using namespace std;

template <typename T>
void print(const string& label, const forward_list<T>& fl) {
    cout << "  " << label << ": ";
    for (const auto& v : fl) cout << v << " ";
    cout << "(" << distance(fl.begin(), fl.end()) << ")\n";
}

int main() {
    // 1. 初始化
    cout << "===== 初始化 =====\n";
    forward_list<int> fl1;                         print("空      ", fl1);
    forward_list<int> fl2(5, 42);                  print("5個42   ", fl2);
    forward_list<int> fl3 = {10,20,30,40,50};      print("列表    ", fl3);
    forward_list<int> fl4(fl3);                    print("複製    ", fl4);

    // 2. 沒有 size(), back(), push_back()
    cout << "\n===== 限制：沒有 size/back/push_back =====\n";
    cout << "  front() = " << fl3.front() << "\n";
    // fl3.back();       // ❌ 編譯錯誤
    // fl3.size();       // ❌ 編譯錯誤
    // fl3.push_back(x); // ❌ 編譯錯誤
    cout << "  用 distance 算長度：" << distance(fl3.begin(), fl3.end()) << "\n";

    // 3. push_front / pop_front
    cout << "\n===== push_front / pop_front =====\n";
    forward_list<int> fl;
    fl.push_front(30); fl.push_front(20); fl.push_front(10);
    print("push_front ×3", fl);
    fl.pop_front();
    print("pop_front    ", fl);

    // 4. before_begin（forward_list 特有）
    cout << "\n===== before_begin =====\n";
    {
        forward_list<int> fl = {20,30,40};
        fl.insert_after(fl.before_begin(), 10);
        print("before_begin 插入", fl);  // 10 20 30 40
    }

    // 5. insert_after / erase_after
    cout << "\n===== insert_after / erase_after =====\n";
    {
        forward_list<int> fl = {10,30,50};
        print("初始         ", fl);
        fl.insert_after(fl.begin(), 20);
        print("insert_after ", fl);  // 10 20 30 50
        fl.erase_after(fl.begin());
        print("erase_after  ", fl);  // 10 30 50
    }

    // 6. 特有成員函數
    cout << "\n===== sort / reverse / unique / remove / merge =====\n";
    {
        forward_list<int> fl = {5,3,8,1,9,2};
        fl.sort();     print("sort     ", fl);
        fl.reverse();  print("reverse  ", fl);

        forward_list<int> fl2 = {1,1,2,3,3,4};
        fl2.unique();  print("unique   ", fl2);

        forward_list<int> fl3 = {1,2,3,2,4,2};
        fl3.remove(2); print("remove(2)", fl3);

        forward_list<int> a = {1,3,5}, b = {2,4,6};
        a.merge(b);    print("merge    ", a);
    }

    // 7. 記憶體比較
    cout << "\n===== 記憶體比較 =====\n";
    cout << "  sizeof(forward_list<int>): " << sizeof(forward_list<int>) << " bytes\n";
    cout << "  sizeof(list<int>):         " << sizeof(list<int>) << " bytes\n";
    cout << "  sizeof(vector<int>):       " << sizeof(vector<int>) << " bytes\n";
    const int N = 100000;
    cout << "  " << N << " 個 int 估算：\n";
    cout << "    forward_list: ~" << (N*16)/1024 << " KB（每節點 16B）\n";
    cout << "    list:         ~" << (N*24)/1024 << " KB（每節點 24B）\n";
    cout << "    vector:       ~" << (N*4)/1024  << " KB（連續 4B）\n";

    // 8. 模擬 push_back（維護 tail 迭代器）
    cout << "\n===== 模擬 push_back =====\n";
    {
        forward_list<int> fl;
        auto tail = fl.before_begin();
        for (int i = 1; i <= 5; i++)
            tail = fl.insert_after(tail, i * 10);
        print("模擬push_back", fl);
    }

    return 0;
}
