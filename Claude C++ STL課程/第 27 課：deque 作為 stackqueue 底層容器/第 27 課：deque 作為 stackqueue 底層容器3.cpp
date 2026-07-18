#include <iostream>
#include <deque>
#include <vector>
using namespace std;

template <typename T, typename Container = deque<T>>
class MyStack {
private:
    Container c;    // 唯一的資料成員

public:
    // push：委託給底層容器的 push_back
    void push(const T& value) {
        c.push_back(value);
    }

    // emplace：委託給底層容器的 emplace_back
    template <typename... Args>
    void emplace(Args&&... args) {
        c.emplace_back(forward<Args>(args)...);
    }

    // pop：委託給底層容器的 pop_back
    void pop() {
        c.pop_back();
    }

    // top：委託給底層容器的 back
    T& top() {
        return c.back();
    }

    const T& top() const {
        return c.back();
    }

    bool empty() const { return c.empty(); }
    size_t size() const { return c.size(); }

    // 配接器不提供迭代器！
    // 不提供 begin(), end()
    // 不提供 operator[]
    // 這就是「限制介面」的意義
};

int main() {
    // 用預設底層（deque）
    MyStack<int> s1;
    s1.push(10);
    s1.push(20);
    s1.push(30);

    while (!s1.empty()) {
        cout << s1.top() << " ";
        s1.pop();
    }
    cout << endl;
    // 輸出：30 20 10

    // 用 vector 作為底層
    MyStack<int, vector<int>> s2;
    s2.push(100);
    s2.push(200);
    cout << "top: " << s2.top() << endl;  // 200

    return 0;
}
