#include <iostream>
#include <vector>
#include <deque>
#include <chrono>
using namespace std;
using namespace chrono;

int main() {
    const int N = 10000000;  // 一千萬

    // === 測試 push_back ===
    {
        vector<int> vec;
        auto start = high_resolution_clock::now();
        for (int i = 0; i < N; i++) vec.push_back(i);
        auto end = high_resolution_clock::now();
        cout << "vector push_back: "
             << duration_cast<milliseconds>(end - start).count()
             << " ms" << endl;
    }
    {
        deque<int> dq;
        auto start = high_resolution_clock::now();
        for (int i = 0; i < N; i++) dq.push_back(i);
        auto end = high_resolution_clock::now();
        cout << "deque  push_back: "
             << duration_cast<milliseconds>(end - start).count()
             << " ms" << endl;
    }

    // === 測試 push_front ===
    {
        deque<int> dq;
        auto start = high_resolution_clock::now();
        for (int i = 0; i < N; i++) dq.push_front(i);
        auto end = high_resolution_clock::now();
        cout << "deque  push_front: "
             << duration_cast<milliseconds>(end - start).count()
             << " ms" << endl;
    }
    // vector 的 push_front 太慢，不測一千萬個

    // === 測試順序遍歷 ===
    {
        vector<int> vec(N);
        long long sum = 0;
        auto start = high_resolution_clock::now();
        for (int i = 0; i < N; i++) sum += vec[i];
        auto end = high_resolution_clock::now();
        cout << "vector 遍歷:      "
             << duration_cast<milliseconds>(end - start).count()
             << " ms (sum=" << sum << ")" << endl;
    }
    {
        deque<int> dq(N);
        long long sum = 0;
        auto start = high_resolution_clock::now();
        for (int i = 0; i < N; i++) sum += dq[i];
        auto end = high_resolution_clock::now();
        cout << "deque  遍歷:      "
             << duration_cast<milliseconds>(end - start).count()
             << " ms (sum=" << sum << ")" << endl;
    }

    return 0;
}
