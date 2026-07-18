#include <iostream>
#include <thread>

// 1. 全域變數
thread_local int global_tl = 0;

void func() {
    // 2. 函式內的 static 變數
    thread_local static int local_tl = 0;
    
    ++global_tl;
    ++local_tl;
    
    std::cout << "global_tl=" << global_tl 
              << " local_tl=" << local_tl << std::endl;
}

class MyClass {
public:
    // 3. 類別的 static 成員
    thread_local static int member_tl;
};

thread_local int MyClass::member_tl = 0;

int main() {
    std::thread t1([]() { func(); func(); });
    std::thread t2([]() { func(); func(); });
    
    t1.join();
    t2.join();
    
    return 0;
}
