#include <iostream>
#include <stack>
#include <string>
using namespace std;

bool isBalanced(const string& expr) {
    stack<char> s;   // 底層是 deque<char>

    for (char ch : expr) {
        if (ch == '(' || ch == '[' || ch == '{') {
            s.push(ch);
        }
        else if (ch == ')' || ch == ']' || ch == '}') {
            if (s.empty()) return false;

            char top = s.top();
            if ((ch == ')' && top == '(') ||
                (ch == ']' && top == '[') ||
                (ch == '}' && top == '{')) {
                s.pop();
            } else {
                return false;
            }
        }
    }
    return s.empty();
}

int main() {
    cout << isBalanced("(a + b) * [c - d]")   << endl;  // 1 (true)
    cout << isBalanced("{(a + b) * [c - d]}")  << endl;  // 1 (true)
    cout << isBalanced("(a + b]")              << endl;  // 0 (false)
    cout << isBalanced("((a + b)")             << endl;  // 0 (false)
    cout << isBalanced("")                     << endl;  // 1 (true)

    return 0;
}
