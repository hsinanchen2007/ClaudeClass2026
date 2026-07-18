#include <iostream>
#include <deque>
using namespace std;

int main() {
    deque<int> dq = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};

    // === 基本遍歷 ===
    cout << "正向遍歷：";
    for (auto it = dq.begin(); it != dq.end(); ++it) {
        cout << *it << " ";
    }
    cout << endl;
    // 正向遍歷：10 20 30 40 50 60 70 80 90 100

    // === 反向遍歷 ===
    cout << "反向遍歷：";
    for (auto it = dq.rbegin(); it != dq.rend(); ++it) {
        cout << *it << " ";
    }
    cout << endl;
    // 反向遍歷：100 90 80 70 60 50 40 30 20 10

    // === 隨機跳躍 ===
    auto it = dq.begin();
    it += 3;
    cout << "begin + 3 = " << *it << endl;  // 40

    it += 4;
    cout << "再 + 4 = " << *it << endl;     // 80

    it -= 5;
    cout << "再 - 5 = " << *it << endl;     // 30

    // === 迭代器距離 ===
    auto first = dq.begin();
    auto last = dq.end();
    cout << "距離：" << (last - first) << endl;  // 10

    // === 迭代器比較 ===
    auto a = dq.begin() + 2;   // 指向 30
    auto b = dq.begin() + 7;   // 指向 80
    cout << "a < b ? " << (a < b) << endl;   // 1 (true)
    cout << "b - a = " << (b - a) << endl;   // 5

    // === 下標存取 ===
    cout << "it[0]=" << dq.begin()[0]
         << " it[5]=" << dq.begin()[5] << endl;
    // it[0]=10 it[5]=60

    return 0;
}
