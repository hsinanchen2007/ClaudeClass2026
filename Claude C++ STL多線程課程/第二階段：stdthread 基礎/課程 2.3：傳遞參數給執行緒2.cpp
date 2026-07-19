#include <iostream>
#include <thread>
#include <functional>   // std::ref

// ⚠️ 本檔更正（原版根本編不過）：
//    原本寫 void modify(int& x) 配 std::thread t(modify, value);
//    這【不是】「value 被複製所以印出 1」——而是**編譯失敗**：
//    std::thread 會先 decay-copy 參數存起來,再以 rvalue 去呼叫,
//    rvalue 綁不到 non-const lvalue reference,於是 libstdc++ 直接
//    static_assert:「std::thread arguments must be invocable after
//    conversion to rvalues」。
//
//    所以要分成兩種寫法,想示範哪一種就用哪一種:

// (A) 想示範「執行緒拿到的是副本、改不到原值」→ 參數就收 by value
void modify_copy(int x) {
    x = 100;                       // 只改到副本
}

// (B) 想真的改到原值 → 參數收 reference,呼叫端必須用 std::ref 包起來
void modify_ref(int& x) {
    x = 100;
}

int main() {
    // (A) 傳值:原值不變
    int value = 1;
    std::thread t1(modify_copy, value);
    t1.join();
    std::cout << "傳值   後 value = " << value << std::endl;   // 1

    // (B) std::ref:原值被改
    int value2 = 1;
    std::thread t2(modify_ref, std::ref(value2));
    t2.join();
    std::cout << "std::ref 後 value = " << value2 << std::endl; // 100

    return 0;
}
