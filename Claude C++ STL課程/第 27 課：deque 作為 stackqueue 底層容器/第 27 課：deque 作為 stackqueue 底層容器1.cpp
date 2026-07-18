#include <iostream>
#include <stack>
using namespace std;

int main() {
    stack<int> s;       // 預設底層是 deque<int>

    s.push(10);
    s.push(20);
    s.push(30);

    cout << "top: " << s.top() << endl;    // 30
    cout << "size: " << s.size() << endl;  // 3

    s.pop();
    cout << "pop 後 top: " << s.top() << endl;  // 20

    // 逐一取出
    while (!s.empty()) {
        cout << s.top() << " ";
        s.pop();
    }
    cout << endl;
    // 輸出：20 10
    
    return 0;
}
