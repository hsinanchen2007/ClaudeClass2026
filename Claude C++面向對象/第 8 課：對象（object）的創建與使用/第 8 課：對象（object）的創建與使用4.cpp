#include <iostream>
#include <string>
using namespace std;

class Counter {
public:
    int count = 0;

    void increment() {
        count++;
    }

    void show() {
        cout << "count = " << count << endl;
    }
};

int main() {
    Counter a;
    Counter b;

    a.increment();   // a 的 count 變成 1
    a.increment();   // a 的 count 變成 2
    a.increment();   // a 的 count 變成 3

    b.increment();   // b 的 count 變成 1（和 a 無關！）

    cout << "a: ";
    a.show();

    cout << "b: ";
    b.show();

    return 0;
}
