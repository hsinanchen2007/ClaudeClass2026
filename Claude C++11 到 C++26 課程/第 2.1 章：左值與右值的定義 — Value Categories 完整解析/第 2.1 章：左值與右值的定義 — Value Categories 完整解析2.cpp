#include <iostream>
#include <string>

std::string greet() {
    return "Hello";  // 回傳值是 prvalue
}

int main() {
    // ===== 以下全部是 prvalue =====

    42;                    // 整數字面值
    3.14;                  // 浮點字面值
    true;                  // 布林字面值
    nullptr;               // 空指標字面值（注意：字串字面值不是！）

    int x = 10;
    2 + 3;                 // 算術運算結果
    x > 0;                 // 比較運算結果

    greet();               // 回傳非參考型別的函式呼叫

    int(42);               // 型別轉換
    static_cast<double>(x);// 轉型結果

    // Lambda 表達式本身是 prvalue
    [](int a) { return a * 2; };

    // 後置遞增/遞減的結果是 prvalue（返回的是舊值的副本）
    int y = 10;
    y++;                   // prvalue（不同於 ++y 是左值）

    return 0;
}
