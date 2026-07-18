#include <iostream>
#include <vector>
#include <deque>
#include <chrono>
using namespace std;
using namespace chrono;

int main() {
    const int N = 100000;

    // === 測試在頭部附近插入 ===
    cout << "--- 在位置 1 插入 " << N << " 次 ---" << endl;
    {
        vector<int> vec(100, 0);
        auto start = high_resolution_clock::now();
        for (int i = 0; i < N; i++)
            vec.insert(vec.begin() + 1, i);
        auto end = high_resolution_clock::now();
        cout << "vector: "
             << duration_cast<milliseconds>(end - start).count()
             << " ms" << endl;
    }
    {
        deque<int> dq(100, 0);
        auto start = high_resolution_clock::now();
        for (int i = 0; i < N; i++)
            dq.insert(dq.begin() + 1, i);
        auto end = high_resolution_clock::now();
        cout << "deque:  "
             << duration_cast<milliseconds>(end - start).count()
             << " ms" << endl;
    }

    // === 測試在中間插入 ===
    cout << "\n--- 在中間位置插入 " << N << " 次 ---" << endl;
    {
        vector<int> vec(100, 0);
        auto start = high_resolution_clock::now();
        for (int i = 0; i < N; i++)
            vec.insert(vec.begin() + vec.size() / 2, i);
        auto end = high_resolution_clock::now();
        cout << "vector: "
             << duration_cast<milliseconds>(end - start).count()
             << " ms" << endl;
    }
    {
        deque<int> dq(100, 0);
        auto start = high_resolution_clock::now();
        for (int i = 0; i < N; i++)
            dq.insert(dq.begin() + dq.size() / 2, i);
        auto end = high_resolution_clock::now();
        cout << "deque:  "
             << duration_cast<milliseconds>(end - start).count()
             << " ms" << endl;
    }

    // === 測試在尾部附近插入 ===
    cout << "\n--- 在倒數第 2 個位置插入 " << N << " 次 ---" << endl;
    {
        vector<int> vec(100, 0);
        auto start = high_resolution_clock::now();
        for (int i = 0; i < N; i++)
            vec.insert(vec.end() - 1, i);
        auto end = high_resolution_clock::now();
        cout << "vector: "
             << duration_cast<milliseconds>(end - start).count()
             << " ms" << endl;
    }
    {
        deque<int> dq(100, 0);
        auto start = high_resolution_clock::now();
        for (int i = 0; i < N; i++)
            dq.insert(dq.end() - 1, i);
        auto end = high_resolution_clock::now();
        cout << "deque:  "
             << duration_cast<milliseconds>(end - start).count()
             << " ms" << endl;
    }

    return 0;
}
