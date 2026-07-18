// ============================================================
// 第 26 課 總結：deque 的插入與刪除效能分析
// 編譯：g++ -std=c++17 -O2 -o summary summary.cpp
// ============================================================
// 【插入/刪除效能特性】
//   頭端 push_front / pop_front：O(1)
//   尾端 push_back / pop_back：  O(1)
//   中間 insert / erase：        O(n)
//
// 【deque 中間插入的優化】
//   deque 會自動選擇搬移較少元素的方向：
//     插入靠近頭部 → 向前搬移（搬的元素少）
//     插入靠近尾部 → 向後搬移（搬的元素少）
//   所以靠近兩端插入比 vector 快，中間位置差異不大
//
// 【vs vector 的比較】
//   靠近頭部插入：deque >> vector（vector 要搬全部元素）
//   中間插入：    差異不大（都是 O(n)）
//   靠近尾部插入：差異不大
// ============================================================

#include <iostream>
#include <vector>
#include <deque>
#include <chrono>
using namespace std;
using namespace chrono;

int main() {
    const int N = 100000;

    // ============================================================
    // 1. 靠近頭部插入（位置 1）
    // ============================================================
    cout << "===== 1. 在位置 1 插入 " << N << " 次 =====\n";
    {
        vector<int> vec(100, 0);
        auto t1 = high_resolution_clock::now();
        for (int i = 0; i < N; i++)
            vec.insert(vec.begin() + 1, i);
        auto t2 = high_resolution_clock::now();
        cout << "  vector: " << duration_cast<milliseconds>(t2 - t1).count() << " ms\n";
    }
    {
        deque<int> dq(100, 0);
        auto t1 = high_resolution_clock::now();
        for (int i = 0; i < N; i++)
            dq.insert(dq.begin() + 1, i);
        auto t2 = high_resolution_clock::now();
        cout << "  deque:  " << duration_cast<milliseconds>(t2 - t1).count() << " ms\n";
        cout << "  （deque 靠近頭部插入快很多！）\n";
    }

    // ============================================================
    // 2. 中間位置插入
    // ============================================================
    cout << "\n===== 2. 在中間位置插入 " << N << " 次 =====\n";
    {
        vector<int> vec(100, 0);
        auto t1 = high_resolution_clock::now();
        for (int i = 0; i < N; i++)
            vec.insert(vec.begin() + static_cast<long long>(vec.size()) / 2, i);
        auto t2 = high_resolution_clock::now();
        cout << "  vector: " << duration_cast<milliseconds>(t2 - t1).count() << " ms\n";
    }
    {
        deque<int> dq(100, 0);
        auto t1 = high_resolution_clock::now();
        for (int i = 0; i < N; i++)
            dq.insert(dq.begin() + static_cast<long long>(dq.size()) / 2, i);
        auto t2 = high_resolution_clock::now();
        cout << "  deque:  " << duration_cast<milliseconds>(t2 - t1).count() << " ms\n";
        cout << "  （中間插入差異不大，都是 O(n)）\n";
    }

    // ============================================================
    // 3. 靠近尾部插入
    // ============================================================
    cout << "\n===== 3. 在倒數第 2 個位置插入 " << N << " 次 =====\n";
    {
        vector<int> vec(100, 0);
        auto t1 = high_resolution_clock::now();
        for (int i = 0; i < N; i++)
            vec.insert(vec.end() - 1, i);
        auto t2 = high_resolution_clock::now();
        cout << "  vector: " << duration_cast<milliseconds>(t2 - t1).count() << " ms\n";
    }
    {
        deque<int> dq(100, 0);
        auto t1 = high_resolution_clock::now();
        for (int i = 0; i < N; i++)
            dq.insert(dq.end() - 1, i);
        auto t2 = high_resolution_clock::now();
        cout << "  deque:  " << duration_cast<milliseconds>(t2 - t1).count() << " ms\n";
    }

    cout << "\n=== 重點 ===\n";
    cout << "  deque 中間插入會自動選擇搬移較少元素的方向\n";
    cout << "  靠近頭部 → deque 遠快於 vector\n";
    cout << "  中間位置 → 差異不大\n";
    cout << "  頭尾兩端 → 都用 push_front/push_back 最快 O(1)\n";

    return 0;
}
