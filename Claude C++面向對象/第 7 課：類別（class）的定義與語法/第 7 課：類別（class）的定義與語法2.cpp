#include <iostream>
#include <string>
using namespace std;

class Person {
public:
    string name;
    int age;

    void introduce() {
        // 直接使用 name 和 age，不需要傳參數！
        cout << "你好，我叫 " << name << "，今年 " << age << " 歲。" << endl;
    }
};

int main() {
    Person p1;
    p1.name = "小明";
    p1.age = 20;
    p1.introduce();  // 輸出：你好，我叫 小明，今年 20 歲。

    Person p2;
    p2.name = "小華";
    p2.age = 22;
    p2.introduce();  // 輸出：你好，我叫 小華，今年 22 歲。

    return 0;
}
