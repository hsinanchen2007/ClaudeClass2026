#include <iostream>

// Most Vexing Parse 問題
class Timer
{
public:
    Timer() { std::cout << "Timer 建構\n"; }
};

class Widget
{
public:
    Widget(Timer t) { std::cout << "Widget 建構\n"; }
};

int main()
{
    // 這是宣告一個函式，還是建構一個物件？
    Widget w(Timer());  // 被解析為函式宣告！
                        // w 是函式，接收一個回傳 Timer 的函式，回傳 Widget
    
    // 使用 {} 避免歧義
    Widget w1{Timer{}};  // 明確是物件建構
    Widget w2(Timer{});  // 也可以混用
    Widget w3{Timer()};  // 也行
}
