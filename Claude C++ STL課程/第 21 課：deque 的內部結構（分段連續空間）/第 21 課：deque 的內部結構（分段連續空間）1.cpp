#include <iostream>
#include <deque>
using namespace std;

int main() {
    deque<int> dq;

    // 尾端插入
    dq.push_back(10);
    dq.push_back(20);
    dq.push_back(30);

    // 頭端插入 — 這就是 deque 的優勢
    dq.push_front(5);
    dq.push_front(1);

    // 此時邏輯順序：1, 5, 10, 20, 30
    cout << "deque 內容：";
    for (int val : dq) {
        cout << val << " ";
    }
    cout << endl;
    // 輸出：deque 內容：1 5 10 20 30

    // 隨機存取 — 和 vector 一樣方便
    cout << "dq[0] = " << dq[0] << endl;  // 1
    cout << "dq[2] = " << dq[2] << endl;  // 10

    // 雙端刪除
    dq.pop_front();  // 移除 1
    dq.pop_back();   // 移除 30

    cout << "pop 後：";
    for (int val : dq) {
        cout << val << " ";
    }
    cout << endl;
    // 輸出：pop 後：5 10 20

    cout << "size = " << dq.size() << endl;  // 3

    return 0;
}
