// lesson40_selection.cpp
// 編譯：g++ -std=c++17 -O2 -Wall -Wextra -o lesson40 lesson40_selection.cpp

#include <iostream>
#include <vector>
#include <deque>
#include <list>
#include <forward_list>
#include <chrono>
#include <random>
#include <algorithm>
#include <numeric>
#include <string>
#include <iterator>
using namespace std;

template <typename Func>
long long measure_us(Func f) {
    auto start = chrono::high_resolution_clock::now();
    f();
    auto end = chrono::high_resolution_clock::now();
    return chrono::duration_cast<chrono::microseconds>(end - start).count();
}

int main() {
    const int N = 100000;
    random_device rd;
    mt19937 gen(rd());

    // ===== 測試 1：頭端插入 =====
    cout << "===== 測試 1：" << N << " 次 push_front =====" << endl;
    {
        auto t_deque = measure_us([&]() {
            deque<int> dq;
            for (int i = 0; i < N; i++) dq.push_front(i);
        });
        auto t_list = measure_us([&]() {
            list<int> lst;
            for (int i = 0; i < N; i++) lst.push_front(i);
        });
        auto t_flist = measure_us([&]() {
            forward_list<int> flst;
            for (int i = 0; i < N; i++) flst.push_front(i);
        });

        cout << "  deque:        " << t_deque << " us" << endl;
        cout << "  list:         " << t_list << " us" << endl;
        cout << "  forward_list: " << t_flist << " us" << endl;
    }

    // ===== 測試 2：中間插入（已持有迭代器）=====
    cout << "\n===== 測試 2：中間插入（已持有迭代器）=====" << endl;
    {
        // 預先建立容器
        list<int> lst;
        for (int i = 0; i < N; i++) lst.push_back(i);
        auto lst_mid = lst.begin();
        advance(lst_mid, N / 2);

        vector<int> vec(N);
        iota(vec.begin(), vec.end(), 0);

        const int INSERT_COUNT = 10000;

        auto t_vec = measure_us([&]() {
            auto v = vec;
            for (int i = 0; i < INSERT_COUNT; i++) {
                v.insert(v.begin() + v.size() / 2, i);
            }
        });
        auto t_list = measure_us([&]() {
            auto l = lst;
            auto mid = l.begin();
            advance(mid, N / 2);
            for (int i = 0; i < INSERT_COUNT; i++) {
                l.insert(mid, i);  // mid 不會失效！
            }
        });

        cout << "  vector（中間insert " << INSERT_COUNT << "次）: " << t_vec << " us" << endl;
        cout << "  list  （中間insert " << INSERT_COUNT << "次）: " << t_list << " us" << endl;
        if (t_list < t_vec) {
            cout << "  → list 更快（迭代器穩定 + O(1) 插入）" << endl;
        } else {
            cout << "  → vector 更快（快取效率）" << endl;
        }
    }

    // ===== 測試 3：遍歷效能 =====
    cout << "\n===== 測試 3：遍歷 " << N << " 個元素 =====" << endl;
    {
        vector<int> vec(N);
        iota(vec.begin(), vec.end(), 0);

        list<int> lst(vec.begin(), vec.end());
        forward_list<int> flst(vec.begin(), vec.end());

        volatile long long sum = 0;

        auto t_vec = measure_us([&]() {
            sum = 0;
            for (int val : vec) sum += val;
        });
        auto t_list = measure_us([&]() {
            sum = 0;
            for (int val : lst) sum += val;
        });
        auto t_flist = measure_us([&]() {
            sum = 0;
            for (int val : flst) sum += val;
        });

        cout << "  vector:       " << t_vec << " us" << endl;
        cout << "  list:         " << t_list << " us" << endl;
        cout << "  forward_list: " << t_flist << " us" << endl;
        cout << "  → vector 通常最快（連續記憶體 + CPU 預取）" << endl;
    }

    // ===== 測試 4：排序 =====
    cout << "\n===== 測試 4：排序 " << N << " 個元素 =====" << endl;
    {
        vector<int> data(N);
        for (int i = 0; i < N; i++) data[i] = gen() % 1000000;

        vector<int> vec = data;
        list<int> lst(data.begin(), data.end());
        forward_list<int> flst(data.begin(), data.end());

        auto t_vec = measure_us([&]() {
            sort(vec.begin(), vec.end());
        });
        auto t_list = measure_us([&]() {
            lst.sort();
        });
        auto t_flist = measure_us([&]() {
            flst.sort();
        });

        cout << "  vector std::sort:   " << t_vec << " us" << endl;
        cout << "  list::sort:         " << t_list << " us" << endl;
        cout << "  forward_list::sort: " << t_flist << " us" << endl;
        cout << "  → vector 通常快 2~4 倍（快取效率）" << endl;
    }

    // ===== 測試 5：記憶體用量 =====
    cout << "\n===== 測試 5：記憶體估算（" << N << " 個 int）=====" << endl;
    {
        cout << "  vector:       " << (N * sizeof(int)) / 1024 << " KB" << endl;
        cout << "  list:         ~" << (N * 24) / 1024 << " KB（每節點 24B）" << endl;
        cout << "  forward_list: ~" << (N * 16) / 1024 << " KB（每節點 16B）" << endl;
        cout << "  forward_list 比 list 省 " << (N * 8) / 1024 << " KB" << endl;
    }

    return 0;
}
