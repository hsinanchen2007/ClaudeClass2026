// ============================================================
// 第 29 課 總結：list 的雙向鏈結串列結構
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【list = 雙向鏈結串列（doubly linked list）】
//
// 【內部結構】
//   每個節點包含三部分：
//     prev 指標 → 指向前一個節點
//     data      → 實際資料
//     next 指標 → 指向下一個節點
//   節點散布在堆積各處，記憶體地址不連續
//
// 【vs vector/deque 的關鍵差異】
//   ┌──────────┬──────────┬──────────┬──────────┐
//   │          │ vector   │  deque   │  list    │
//   ├──────────┼──────────┼──────────┼──────────┤
//   │ 隨機存取 │ O(1) ✅  │ O(1)    │ ❌ 不支援│
//   │ 頭端插刪 │ O(n) ❌  │ O(1)    │ O(1) ✅  │
//   │ 尾端插刪 │ O(1)     │ O(1)    │ O(1) ✅  │
//   │ 中間插刪 │ O(n)     │ O(n)    │ O(1) ✅  │
//   │ 迭代器   │ Random   │ Random  │ Bidirect │
//   │ 記憶體   │ 連續     │ 分段連續│ 不連續   │
//   └──────────┴──────────┴──────────┴──────────┘
//
// 【list 的優勢】
//   1. 中間插刪 O(1)（已有迭代器時）
//   2. 插入/刪除不會使其他迭代器失效
//   3. splice O(1) 把節點從一個 list 搬到另一個
//
// 【list 的劣勢】
//   1. 不支援 operator[] 和 at()（必須用迭代器遍歷）
//   2. 每個節點多兩個指標（記憶體開銷大）
//   3. 記憶體不連續（cache 不友善，遍歷慢）
//
// 【迭代器類型】BidirectionalIterator
//   ✅ ++it, --it（前進、後退）
//   ❌ it + n, it - n, it[n]（不支援隨機跳躍）
//   要跳 n 步用 std::advance(it, n)
// ============================================================

#include <iostream>
#include <list>
#include <vector>
using namespace std;

int main() {
    list<int> lst = {10, 20, 30, 40, 50};

    // 1. 遍歷方式
    cout << "===== 遍歷 =====\n";
    cout << "  範圍 for：";
    for (int val : lst) cout << val << " ";
    cout << "\n  反向迭代：";
    for (auto rit = lst.rbegin(); rit != lst.rend(); ++rit)
        cout << *rit << " ";
    cout << endl;

    // 2. 不支援隨機存取 → 用 advance
    cout << "\n===== 迭代器操作 =====\n";
    auto it = lst.begin();
    ++it;  cout << "  ++it → " << *it << "\n";     // 20
    --it;  cout << "  --it → " << *it << "\n";     // 10
    // it + 3;  // ❌ 編譯錯誤！
    advance(it, 3);
    cout << "  advance(it, 3) → " << *it << "\n";  // 40

    // 3. 記憶體地址不連續
    cout << "\n===== 記憶體地址（不連續）=====\n";
    for (auto& val : lst)
        cout << "  值:" << val << "  地址:" << &val << "\n";

    cout << "\n===== vector 地址（連續）=====\n";
    vector<int> vec = {10, 20, 30, 40, 50};
    for (auto& val : vec)
        cout << "  值:" << val << "  地址:" << &val << "\n";

    // 4. O(1) 插入（已有迭代器）
    cout << "\n===== O(1) 插入 =====\n";
    auto pos = lst.begin();
    while (*pos != 30) ++pos;
    lst.insert(pos, 25);
    cout << "  在 30 前插入 25：";
    for (int v : lst) cout << v << " ";
    cout << "\n  pos 仍指向：" << *pos << "（未失效）\n";

    // 5. O(1) 刪除
    cout << "\n===== O(1) 刪除 =====\n";
    auto next_pos = lst.erase(pos);
    cout << "  刪除 30 後：";
    for (int v : lst) cout << v << " ";
    cout << "\n  erase 回傳下一個：" << *next_pos << "\n";

    // 6. front / back
    cout << "\n===== front / back =====\n";
    cout << "  front=" << lst.front() << " back=" << lst.back() << "\n";

    return 0;
}
