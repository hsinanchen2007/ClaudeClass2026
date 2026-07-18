// ============================================================
// 第 40 課 總結：list 與 forward_list 的選擇時機
// 編譯：g++ -std=c++17 -O2 -o summary summary.cpp
// ============================================================
// 【四大序列容器選擇指南】
//   ┌──────────────┬─────────┬─────────┬─────────┬─────────────┐
//   │              │ vector  │ deque   │ list    │ forward_list│
//   ├──────────────┼─────────┼─────────┼─────────┼─────────────┤
//   │ 隨機存取     │ O(1) ★ │ O(1)   │ ❌      │ ❌          │
//   │ 頭端插刪     │ O(n)   │ O(1) ★ │ O(1)    │ O(1)        │
//   │ 尾端插刪     │ O(1) ★ │ O(1)   │ O(1)    │ ❌          │
//   │ 中間插刪     │ O(n)   │ O(n)   │ O(1) ★ │ O(1) ★     │
//   │ 迭代器穩定   │ ❌     │ ❌     │ ✅ ★   │ ✅ ★       │
//   │ 記憶體效率   │ ★★★  │ ★★   │ ★      │ ★★         │
//   │ cache 效率   │ ★★★  │ ★★   │ ★      │ ★           │
//   │ splice       │ ❌     │ ❌     │ O(1) ★ │ O(1) ★     │
//   └──────────────┴─────────┴─────────┴─────────┴─────────────┘
//
// 【選擇建議】
//   預設首選：vector（最好的 cache 效率）
//   需要雙端操作：deque
//   需要頻繁中間插刪 + 迭代器穩定：list
//   記憶體極度敏感 + 只需前向：forward_list
//   需要 splice：list / forward_list
// ============================================================

#include <iostream>
#include <vector>
#include <deque>
#include <list>
#include <forward_list>
#include <chrono>
#include <random>
#include <algorithm>
#include <numeric>
#include <iterator>
using namespace std;

auto measure_us(auto f) {
    auto t = chrono::high_resolution_clock::now();
    f();
    return chrono::duration_cast<chrono::microseconds>(
        chrono::high_resolution_clock::now() - t).count();
}

int main() {
    const int N = 100000;

    // 1. 頭端插入
    cout << "===== 1. " << N << " 次 push_front =====\n";
    {
        auto t_dq = measure_us([&]{ deque<int> d; for(int i=0;i<N;i++) d.push_front(i); });
        auto t_lst = measure_us([&]{ list<int> l; for(int i=0;i<N;i++) l.push_front(i); });
        auto t_fl = measure_us([&]{ forward_list<int> f; for(int i=0;i<N;i++) f.push_front(i); });
        cout << "  deque:        " << t_dq << " us\n";
        cout << "  list:         " << t_lst << " us\n";
        cout << "  forward_list: " << t_fl << " us\n";
    }

    // 2. 中間插入（已有迭代器）
    cout << "\n===== 2. 中間 insert 10000 次 =====\n";
    {
        const int INS = 10000;
        auto t_vec = measure_us([&]{
            vector<int> v(N);
            for(int i=0;i<INS;i++) v.insert(v.begin()+v.size()/2, i);
        });
        auto t_lst = measure_us([&]{
            list<int> l(N);
            auto mid=l.begin(); advance(mid, N/2);
            for(int i=0;i<INS;i++) l.insert(mid, i);
        });
        cout << "  vector: " << t_vec << " us\n";
        cout << "  list:   " << t_lst << " us\n";
    }

    // 3. 遍歷效能
    cout << "\n===== 3. 遍歷 " << N << " 個元素 =====\n";
    {
        vector<int> vec(N); iota(vec.begin(), vec.end(), 0);
        list<int> lst(vec.begin(), vec.end());
        forward_list<int> fl(vec.begin(), vec.end());
        volatile long long sum = 0;

        auto t_vec = measure_us([&]{ sum=0; for(int v:vec) sum+=v; });
        auto t_lst = measure_us([&]{ sum=0; for(int v:lst) sum+=v; });
        auto t_fl  = measure_us([&]{ sum=0; for(int v:fl)  sum+=v; });
        cout << "  vector:       " << t_vec << " us\n";
        cout << "  list:         " << t_lst << " us\n";
        cout << "  forward_list: " << t_fl  << " us\n";
        cout << "  → vector 最快（連續記憶體 + CPU 預取）\n";
    }

    // 4. 排序效能
    cout << "\n===== 4. 排序 " << N << " 個元素 =====\n";
    {
        mt19937 gen(42);
        vector<int> data(N);
        for(auto& v:data) v = gen() % 1000000;
        vector<int> vec = data;
        list<int> lst(data.begin(), data.end());
        forward_list<int> fl(data.begin(), data.end());

        auto t_vec = measure_us([&]{ sort(vec.begin(), vec.end()); });
        auto t_lst = measure_us([&]{ lst.sort(); });
        auto t_fl  = measure_us([&]{ fl.sort(); });
        cout << "  vector std::sort:   " << t_vec << " us\n";
        cout << "  list::sort:         " << t_lst << " us\n";
        cout << "  forward_list::sort: " << t_fl  << " us\n";
    }

    // 5. 記憶體用量
    cout << "\n===== 5. 記憶體估算（" << N << " 個 int）=====\n";
    cout << "  vector:       " << (N*sizeof(int))/1024 << " KB\n";
    cout << "  list:         ~" << (N*24)/1024 << " KB（每節點 24B）\n";
    cout << "  forward_list: ~" << (N*16)/1024 << " KB（每節點 16B）\n";
    cout << "  forward_list 比 list 省 " << (N*8)/1024 << " KB\n";

    // 選擇指南
    cout << "\n===== 選擇指南 =====\n";
    cout << "  預設首選 → vector\n";
    cout << "  需要 push_front → deque\n";
    cout << "  需要頻繁中間插刪 + 迭代器穩定 → list\n";
    cout << "  需要 splice → list\n";
    cout << "  記憶體極度敏感 + 只需前向 → forward_list\n";
    cout << "  需要 data() 指標 → vector\n";

    return 0;
}
