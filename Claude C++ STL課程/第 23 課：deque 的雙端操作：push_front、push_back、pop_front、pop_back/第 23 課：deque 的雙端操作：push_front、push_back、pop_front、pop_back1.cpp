#include <iostream>
#include <deque>
using namespace std;

void print(const string& label, const deque<int>& dq) {
    cout << label << ": ";
    if (dq.empty()) {
        cout << "(空)";
    } else {
        cout << "front=" << dq.front() << " back=" << dq.back()
             << " | ";
        for (int val : dq) cout << val << " ";
    }
    cout << " (size=" << dq.size() << ")" << endl;
}

int main() {
    deque<int> dq;
    print("初始狀態  ", dq);
    // 初始狀態  : (空) (size=0)

    // === 尾端操作 ===
    dq.push_back(10);
    print("push_back 10", dq);
    // push_back 10: front=10 back=10 | 10  (size=1)

    dq.push_back(20);
    dq.push_back(30);
    print("push_back 20,30", dq);
    // push_back 20,30: front=10 back=30 | 10 20 30  (size=3)

    // === 頭端操作 ===
    dq.push_front(5);
    print("push_front 5", dq);
    // push_front 5: front=5 back=30 | 5 10 20 30  (size=4)

    dq.push_front(1);
    print("push_front 1", dq);
    // push_front 1: front=1 back=30 | 1 5 10 20 30  (size=5)

    // === 雙端刪除 ===
    dq.pop_front();
    print("pop_front   ", dq);
    // pop_front   : front=5 back=30 | 5 10 20 30  (size=4)

    dq.pop_back();
    print("pop_back    ", dq);
    // pop_back    : front=5 back=20 | 5 10 20  (size=3)

    // === 連續操作模擬佇列行為 ===
    cout << "\n--- 模擬佇列（FIFO）---" << endl;
    deque<int> queue;
    
    // 入列（從尾端加入）
    for (int i = 1; i <= 5; i++) {
        queue.push_back(i * 100);
        cout << "入列 " << i * 100;
        print("  → 佇列", queue);
    }

    // 出列（從頭端取出）
    while (!queue.empty()) {
        cout << "出列 " << queue.front();
        queue.pop_front();
        print("  → 佇列", queue);
    }

    return 0;
}
