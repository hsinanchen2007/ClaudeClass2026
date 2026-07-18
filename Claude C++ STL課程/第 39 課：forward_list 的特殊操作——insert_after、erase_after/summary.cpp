// ============================================================
// 第 39 課 總結：forward_list 的特殊操作——insert_after、erase_after
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【為什麼是 _after？】
//   forward_list 只有 next 指標（沒有 prev）
//   → 無法直接修改「前一個」節點的 next → 只能在「之後」操作
//
// 【insert_after 重載】
//   insert_after(pos, val)              單一值，回傳新元素迭代器
//   insert_after(pos, n, val)           n 個相同值
//   insert_after(pos, {init})           初始化列表
//   insert_after(pos, first, last)      範圍
//   emplace_after(pos, args...)         原地建構
//
// 【erase_after 重載】
//   erase_after(pos)                    刪除 pos 的下一個
//   erase_after(pos, last)             刪除 (pos, last) 開區間
//
// 【splice_after】
//   A.splice_after(pos, B)              整個 B 移植到 pos 之後
//   A.splice_after(pos, B, before_it)   B 的 before_it 下一個移植
//   A.splice_after(pos, B, first, last) 範圍 (first,last) 移植
//
// 【迴圈中安全刪除】需要手動追蹤 prev
//   auto prev = fl.before_begin(), curr = fl.begin();
//   while (curr != fl.end()) {
//       if (cond) curr = fl.erase_after(prev);
//       else { prev = curr; ++curr; }
//   }
//   或更簡單用 fl.remove_if(pred)
// ============================================================

#include <iostream>
#include <forward_list>
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
    // 1. insert_after 所有重載
    cout << "===== insert_after =====\n";
    {
        forward_list<int> fl = {10,40,70};
        print("初始              ", fl);
        auto it = fl.begin();
        auto ret = fl.insert_after(it, 20);
        print("insert_after(20)  ", fl);
        ret = fl.insert_after(ret, 2, 30);
        print("insert 2×30      ", fl);
        fl.insert_after(fl.before_begin(), {1,2,3});
        print("insert{1,2,3}頭端 ", fl);
    }

    // 2. emplace_after 鏈式追加（模擬 push_back）
    cout << "\n===== emplace_after 鏈式追加 =====\n";
    {
        forward_list<pair<string,int>> scores;
        auto pos = scores.before_begin();
        pos = scores.emplace_after(pos, "Alice", 95);
        pos = scores.emplace_after(pos, "Bob", 88);
        pos = scores.emplace_after(pos, "Charlie", 92);
        for (auto& [n,s] : scores) cout << "  " << n << ":" << s << "\n";
    }

    // 3. erase_after
    cout << "\n===== erase_after =====\n";
    {
        forward_list<int> fl = {10,20,30,40,50,60,70};
        print("初始              ", fl);
        fl.erase_after(fl.begin());
        print("erase_after(begin)", fl);  // 刪20
        fl.erase_after(fl.before_begin());
        print("刪第一個          ", fl);  // 刪10
        auto before = fl.begin();
        auto last = before; advance(last, 3);
        fl.erase_after(before, last);
        print("刪範圍            ", fl);
    }

    // 4. 迴圈中安全刪除
    cout << "\n===== 迴圈中安全刪除 =====\n";
    {
        forward_list<int> fl = {1,2,3,4,5,6,7,8,9,10};
        print("原始              ", fl);
        // 方法 1：手動追蹤 prev
        auto prev = fl.before_begin();
        auto curr = fl.begin();
        while (curr != fl.end()) {
            if (*curr % 3 == 0) curr = fl.erase_after(prev);
            else { prev = curr; ++curr; }
        }
        print("刪3倍數(手動)     ", fl);
    }
    {
        // 方法 2：remove_if（推薦）
        forward_list<int> fl = {1,2,3,4,5,6,7,8,9,10};
        fl.remove_if([](int x) { return x % 3 == 0; });
        print("刪3倍數(remove_if)", fl);
    }

    // 5. splice_after
    cout << "\n===== splice_after =====\n";
    {
        forward_list<int> A = {1,2,3}, B = {10,20,30};
        A.splice_after(A.before_begin(), B);
        print("整個移植 A ", A);  // 10 20 30 1 2 3
        print("整個移植 B ", B);  // (空)
    }
    {
        forward_list<int> A = {1,2,3}, B = {10,20,30};
        A.splice_after(A.begin(), B, B.begin());
        // 移植 B.begin()(10) 之後的元素(20) 到 A.begin()(1) 之後
        print("單一移植 A ", A);  // 1 20 2 3
        print("單一移植 B ", B);  // 10 30
    }

    // 6. 同一 forward_list 內移動
    cout << "\n===== 同一 list 內移動 =====\n";
    {
        forward_list<int> fl = {1,2,3,4,5};
        print("初始       ", fl);
        auto before4 = fl.begin(); advance(before4, 2);
        fl.splice_after(fl.before_begin(), fl, before4);
        print("4→最前面   ", fl);  // 4 1 2 3 5
    }

    return 0;
}
