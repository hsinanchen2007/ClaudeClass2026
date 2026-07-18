#include <iostream>
#include <queue>
using namespace std;

int main() {
    queue<int> q;       // 預設底層是 deque<int>

    q.push(10);
    q.push(20);
    q.push(30);

    cout << "front: " << q.front() << endl;  // 10（最先進去的）
    cout << "back: " << q.back() << endl;    // 30（最後進去的）

    q.pop();   // 移除 10
    cout << "pop 後 front: " << q.front() << endl;  // 20

    while (!q.empty()) {
        cout << q.front() << " ";
        q.pop();
    }
    cout << endl;
    // 輸出：20 30（先進先出）

    return 0;
}
