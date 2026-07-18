// ============================================================
// 第 23 課 總結：deque 的雙端操作
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【四大雙端操作】
//   push_back(val)   尾端插入  O(1) 攤銷
//   push_front(val)  頭端插入  O(1) 攤銷
//   pop_back()       尾端刪除  O(1)
//   pop_front()      頭端刪除  O(1)
//
// 【存取首尾元素】
//   front()  回傳第一個元素的引用
//   back()   回傳最後一個元素的引用
//
// 【典型應用】
//   佇列（FIFO）：push_back 入列 + pop_front 出列
//   堆疊（LIFO）：push_back 入棧 + pop_back 出棧
//
// 【emplace_back vs push_back】
//   push_back(string("Hello"))  → 建構臨時物件 + 移動到容器
//   emplace_back("Hello")       → 直接在容器內建構（零拷貝零移動）
// ============================================================

#include <iostream>
#include <deque>
#include <string>
using namespace std;

void print(const string& label, const deque<int>& dq) {
    cout << "  " << label << ": ";
    if (dq.empty()) {
        cout << "(空)";
    } else {
        cout << "front=" << dq.front() << " back=" << dq.back() << " | ";
        for (int val : dq) cout << val << " ";
    }
    cout << " (size=" << dq.size() << ")" << endl;
}

int main() {
    // ============================================================
    // 1. 基本雙端操作
    // ============================================================
    cout << "===== 1. 基本雙端操作 =====\n";
    deque<int> dq;
    print("初始狀態    ", dq);

    dq.push_back(10);
    dq.push_back(20);
    dq.push_back(30);
    print("push_back ×3", dq);

    dq.push_front(5);
    dq.push_front(1);
    print("push_front×2", dq);
    // 1 5 10 20 30

    dq.pop_front();
    print("pop_front   ", dq);
    // 5 10 20 30

    dq.pop_back();
    print("pop_back    ", dq);
    // 5 10 20
    cout << "\n";

    // ============================================================
    // 2. 模擬佇列（FIFO：先進先出）
    // ============================================================
    cout << "===== 2. 模擬佇列 (FIFO) =====\n";
    deque<int> queue;

    // 入列（從尾端加入）
    for (int i = 1; i <= 5; i++) {
        queue.push_back(i * 100);
        cout << "  入列 " << i * 100 << endl;
    }
    // 佇列：100 200 300 400 500

    // 出列（從頭端取出）
    cout << "  出列順序：";
    while (!queue.empty()) {
        cout << queue.front() << " ";
        queue.pop_front();
    }
    cout << "\n\n";
    // 出列順序：100 200 300 400 500

    // ============================================================
    // 3. 模擬堆疊（LIFO：後進先出）
    // ============================================================
    cout << "===== 3. 模擬堆疊 (LIFO) =====\n";
    deque<int> stack;

    for (int i = 1; i <= 5; i++) {
        stack.push_back(i * 10);
        cout << "  push " << i * 10 << endl;
    }

    cout << "  pop 順序：";
    while (!stack.empty()) {
        cout << stack.back() << " ";
        stack.pop_back();
    }
    cout << "\n\n";
    // pop 順序：50 40 30 20 10

    // ============================================================
    // 4. emplace_back vs push_back
    // ============================================================
    cout << "===== 4. emplace_back vs push_back =====\n";
    deque<string> names;

    // push_back：先建構臨時 string，再移動到 deque
    names.push_back(string("Alice"));

    // emplace_back：直接在 deque 內部建構（更高效）
    names.emplace_back("Bob");

    for (const auto& n : names) cout << "  " << n << "\n";
    cout << "  emplace_back 省去了臨時物件的建構和移動\n";

    return 0;
}
