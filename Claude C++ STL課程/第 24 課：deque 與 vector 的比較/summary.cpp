// ============================================================
// 第 24 課 總結：deque 與 vector 的比較
// 編譯：g++ -std=c++17 -O2 -o summary summary.cpp
// ============================================================
// 【效能比較】
//              vector          deque
// push_back    O(1) 攤銷       O(1) 攤銷（稍慢）
// push_front   O(n) 💀        O(1) 攤銷 ✅
// pop_front    O(n) 💀        O(1) ✅
// 隨機存取     O(1) 最快       O(1) 多一次間接尋址
// 順序遍歷     快（cache友好）  稍慢（跨buffer）
// 中間插刪     O(n)            O(n)（可能稍快）
// 記憶體       連續一整塊       分段式
//
// 【選擇指引】
//   需要頭端操作（push_front/pop_front）→ 用 deque
//   需要連續記憶體（data() 指標）       → 只能 vector
//   大量順序遍歷（cache 效能）          → vector 較好
//   一般用途                            → vector（預設首選）
//
// 【deque 獨有優勢】
//   1. push_front / pop_front 是 O(1)
//   2. 擴容不需要搬移所有元素（只新增 buffer）
//   3. 是 stack 和 queue 的預設底層容器
//
// 【vector 獨有優勢】
//   1. data() 指標 → 可傳給 C API
//   2. 更好的 cache locality → 遍歷更快
//   3. 記憶體更緊湊
// ============================================================

#include <iostream>
#include <vector>
#include <deque>
#include <chrono>
using namespace std;
using namespace chrono;

int main() {
    const int N = 5000000;

    // ============================================================
    // 1. push_back 效能比較
    // ============================================================
    cout << "===== 1. push_back " << N << " 次 =====\n";
    {
        vector<int> vec;
        auto t1 = high_resolution_clock::now();
        for (int i = 0; i < N; i++) vec.push_back(i);
        auto t2 = high_resolution_clock::now();
        cout << "  vector: " << duration_cast<milliseconds>(t2 - t1).count() << " ms\n";
    }
    {
        deque<int> dq;
        auto t1 = high_resolution_clock::now();
        for (int i = 0; i < N; i++) dq.push_back(i);
        auto t2 = high_resolution_clock::now();
        cout << "  deque:  " << duration_cast<milliseconds>(t2 - t1).count() << " ms\n";
    }

    // ============================================================
    // 2. push_front — deque 的獨家優勢
    // ============================================================
    cout << "\n===== 2. push_front " << N << " 次 =====\n";
    {
        deque<int> dq;
        auto t1 = high_resolution_clock::now();
        for (int i = 0; i < N; i++) dq.push_front(i);
        auto t2 = high_resolution_clock::now();
        cout << "  deque:  " << duration_cast<milliseconds>(t2 - t1).count() << " ms\n";
    }
    cout << "  vector: 不測試（O(n) 太慢）\n";

    // ============================================================
    // 3. 順序遍歷效能比較
    // ============================================================
    cout << "\n===== 3. 順序遍歷 " << N << " 個元素 =====\n";
    {
        vector<int> vec(N);
        long long sum = 0;
        auto t1 = high_resolution_clock::now();
        for (int i = 0; i < N; i++) sum += vec[i];
        auto t2 = high_resolution_clock::now();
        cout << "  vector: " << duration_cast<milliseconds>(t2 - t1).count()
             << " ms (sum=" << sum << ")\n";
    }
    {
        deque<int> dq(N);
        long long sum = 0;
        auto t1 = high_resolution_clock::now();
        for (int i = 0; i < N; i++) sum += dq[i];
        auto t2 = high_resolution_clock::now();
        cout << "  deque:  " << duration_cast<milliseconds>(t2 - t1).count()
             << " ms (sum=" << sum << ")\n";
    }

    // ============================================================
    // 4. 關鍵差異示範
    // ============================================================
    cout << "\n===== 4. 關鍵差異 =====\n";

    // vector 有 data() 指標
    vector<int> vec = {1, 2, 3};
    int* ptr = vec.data();  // ← 連續記憶體指標
    cout << "  vector.data() = " << ptr << " → 可傳給 C API\n";

    // deque 沒有 data()
    deque<int> dq = {1, 2, 3};
    // dq.data();  // ❌ 編譯錯誤！deque 不保證連續記憶體
    cout << "  deque 沒有 data()，因為記憶體不連續\n";

    // ============================================================
    // 重點整理
    // ============================================================
    cout << "\n=== 選擇指引 ===\n";
    cout << "  預設首選：vector\n";
    cout << "  需要 push_front / pop_front → deque\n";
    cout << "  需要傳指標給 C API → vector (data())\n";
    cout << "  大量遍歷重視 cache → vector\n";
    cout << "  stack / queue 底層 → deque（標準庫預設）\n";

    return 0;
}
