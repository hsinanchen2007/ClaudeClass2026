// # 第 2 課：C 與 C++ 的關鍵差異

// ---

// ## 2.1 課程目標

// 你已經有 C 語言的基礎，這是學習 C++ 的絕佳起點。本課將系統性地比較 C 和 C++ 的差異，讓你清楚知道哪些 C 的習慣可以保留，哪些需要改變，以及 C++ 新增了哪些重要特性。

// ---

// ## 2.2 總體差異概覽

// | 面向 | C 語言 | C++ |
// |------|--------|-----|
// | 設計範式 | 程序式 | 多範式（程序式 + OOP + 泛型 + 函數式） |
// | 資料與函數 | 分離 | 可封裝在類別中 |
// | 記憶體管理 | malloc / free | new / delete + 智能指標 |
// | 輸入輸出 | printf / scanf | iostream（cout / cin） |
// | 字串處理 | char 陣列 + 函數 | std::string 類別 |
// | 函數重載 | 不支援 | 支援 |
// | 預設參數 | 不支援 | 支援 |
// | 引用 | 不支援（只有指標） | 支援引用（reference） |
// | 布林類型 | 需要 stdbool.h（C99） | 內建 bool |
// | 命名空間 | 不支援 | 支援 namespace |

// ---

// ## 2.3 差異一：註解風格

// ### C 語言（C89）

// ```c
// /* 這是 C 風格的註解 */
// /* 
//    多行註解
//    也是這樣寫
// */
// ```

// ### C++（也支援單行註解）

// ```cpp
// // 這是 C++ 風格的單行註解
// /* C 風格的註解在 C++ 中也可以使用 */

// // C++ 程式設計師通常偏好使用 //
// // 因為更簡潔，不需要結束標記
// ```

// > **注意**：C99 之後，C 語言也支援 `//` 單行註解了，但這原本是 C++ 的特性。

// ---

// ## 2.4 差異二：變數宣告位置

// ### C 語言（C89）

// ```c
// #include <stdio.h>

// int main() {
//     /* C89 要求變數必須在區塊開頭宣告 */
//     int i;
//     int sum;
    
//     sum = 0;
//     for (i = 0; i < 10; i++) {
//         sum += i;
//     }
    
//     printf("Sum = %d\n", sum);
//     return 0;
// }
// ```

// ### C++

// ```cpp
// #include <iostream>

// int main() {
//     // C++ 可以在任何位置宣告變數
//     int sum = 0;
    
//     // 甚至可以在 for 迴圈內宣告
//     for (int i = 0; i < 10; i++) {
//         sum += i;
//     }
    
//     // 需要時才宣告，更清晰
//     int result = sum * 2;
    
//     std::cout << "Sum = " << sum << std::endl;
//     return 0;
// }
// ```

// > **最佳實踐**：在 C++ 中，變數應該在**第一次使用時**才宣告，並且最好同時初始化。這樣程式碼更清晰，也能減少錯誤。

// ---

// ## 2.5 差異三：輸入輸出

// ### C 語言

// ```c
// #include <stdio.h>

// int main() {
//     int age;
//     char name[50];
    
//     printf("請輸入你的名字: ");
//     scanf("%s", name);
    
//     printf("請輸入你的年齡: ");
//     scanf("%d", &age);
    
//     printf("你好 %s，你今年 %d 歲\n", name, age);
    
//     return 0;
// }
// ```

// ### C++

// ```cpp
// #include <iostream>
// #include <string>

// int main() {
//     std::string name;
//     int age;
    
//     std::cout << "請輸入你的名字: ";
//     std::cin >> name;
    
//     std::cout << "請輸入你的年齡: ";
//     std::cin >> age;
    
//     std::cout << "你好 " << name << "，你今年 " << age << " 歲" << std::endl;
    
//     return 0;
// }
// ```

// ### 比較分析

// | 特性 | C (printf/scanf) | C++ (cout/cin) |
// |------|------------------|----------------|
// | 類型安全 | 需要手動指定格式符 %d %s | 自動識別類型 |
// | 格式錯誤 | 可能導致未定義行為 | 編譯器可檢查 |
// | 可擴展性 | 不能用於自定義類型 | 可為自定義類別重載 << >> |
// | 效能 | 稍快 | 稍慢（但差異通常可忽略） |

// ---

// ## 2.6 差異四：記憶體配置

// ### C 語言

// ```c
// #include <stdio.h>
// #include <stdlib.h>

// int main() {
//     // 配置單一變數
//     int* p = (int*)malloc(sizeof(int));
//     *p = 42;
//     printf("值: %d\n", *p);
//     free(p);
    
//     // 配置陣列
//     int* arr = (int*)malloc(5 * sizeof(int));
//     for (int i = 0; i < 5; i++) {
//         arr[i] = i * 10;
//     }
//     free(arr);
    
//     return 0;
// }
// ```

// ### C++

// ```cpp
// #include <iostream>

// int main() {
//     // 配置單一變數
//     int* p = new int;      // 不需要 sizeof，不需要轉型
//     *p = 42;
//     std::cout << "值: " << *p << std::endl;
//     delete p;              // 使用 delete 而非 free
    
//     // 配置陣列
//     int* arr = new int[5]; // 配置陣列使用 new[]
//     for (int i = 0; i < 5; i++) {
//         arr[i] = i * 10;
//     }
//     delete[] arr;          // 釋放陣列使用 delete[]
    
//     return 0;
// }
// ```

// ### 關鍵差異

// | 特性 | C (malloc/free) | C++ (new/delete) |
// |------|-----------------|------------------|
// | 返回類型 | void*，需要轉型 | 正確的指標類型 |
// | 大小計算 | 需要 sizeof | 自動計算 |
// | 建構函數 | 不會調用 | 會調用建構函數 |
// | 解構函數 | 不會調用 | 會調用解構函數 |
// | 陣列處理 | 同一個 free | 需區分 delete 和 delete[] |

// > **重要**：在 C++ 中，`new` 配置的記憶體必須用 `delete` 釋放，`new[]` 配置的必須用 `delete[]` 釋放。混用會導致未定義行為！

// ---

// ## 2.7 差異五：布林類型

// ### C 語言

// ```c
// #include <stdio.h>
// #include <stdbool.h>  // C99 需要這個標頭檔

// int main() {
//     bool flag = true;   // 需要 stdbool.h
//     int old_style = 1;  // C89 常用 int 代替
    
//     if (flag) {
//         printf("flag is true\n");
//     }
    
//     return 0;
// }
// ```

// ### C++

// ```cpp
// #include <iostream>

// int main() {
//     bool flag = true;  // bool 是內建類型，不需要標頭檔
    
//     if (flag) {
//         std::cout << "flag is true" << std::endl;
//     }
    
//     // 可以直接輸出布林值
//     std::cout << "flag = " << flag << std::endl;              // 輸出 1
//     std::cout << "flag = " << std::boolalpha << flag << std::endl;  // 輸出 true
    
//     return 0;
// }
// ```

// ---

// ## 2.8 差異六：函數重載（Function Overloading）

// ### C 語言（不支援）

// ```c
// #include <stdio.h>

// // C 語言中，每個函數必須有不同的名稱
// int add_int(int a, int b) {
//     return a + b;
// }

// double add_double(double a, double b) {
//     return a + b;
// }

// int main() {
//     printf("%d\n", add_int(3, 5));
//     printf("%f\n", add_double(3.14, 2.86));
//     return 0;
// }
// ```

// ### C++（支援）

// ```cpp
// #include <iostream>

// // C++ 允許同名函數，只要參數不同
// int add(int a, int b) {
//     return a + b;
// }

// double add(double a, double b) {
//     return a + b;
// }

// int add(int a, int b, int c) {
//     return a + b + c;
// }

// int main() {
//     std::cout << add(3, 5) << std::endl;        // 調用 int 版本
//     std::cout << add(3.14, 2.86) << std::endl;  // 調用 double 版本
//     std::cout << add(1, 2, 3) << std::endl;     // 調用三參數版本
    
//     return 0;
// }
// ```

// 編譯器會根據**參數的類型和數量**自動選擇正確的函數版本，這稱為**函數重載（Function Overloading）**。

// ---

// ## 2.9 差異七：預設參數（Default Arguments）

// ### C 語言（不支援）

// ```c
// #include <stdio.h>

// void greet(const char* name, const char* greeting) {
//     printf("%s, %s!\n", greeting, name);
// }

// int main() {
//     // 每次都必須提供所有參數
//     greet("Alice", "Hello");
//     greet("Bob", "Hello");  // 即使 greeting 常常是 "Hello"
//     return 0;
// }
// ```

// ### C++（支援）

// ```cpp
// #include <iostream>
// #include <string>

// void greet(const std::string& name, const std::string& greeting = "Hello") {
//     std::cout << greeting << ", " << name << "!" << std::endl;
// }

// int main() {
//     greet("Alice");           // 使用預設值 "Hello"
//     greet("Bob");             // 使用預設值 "Hello"
//     greet("Charlie", "Hi");   // 使用自訂值 "Hi"
    
//     return 0;
// }
// ```

// ### 輸出

// ```
// Hello, Alice!
// Hello, Bob!
// Hi, Charlie!
// ```

// > **規則**：預設參數必須從右邊開始。也就是說，有預設值的參數必須放在沒有預設值的參數後面。

// ---

// ## 2.10 差異八：引用（Reference）

// 這是 C++ 新增的重要特性，C 語言只有指標，沒有引用。

// ### C 語言（使用指標）

// ```c
// #include <stdio.h>

// void swap(int* a, int* b) {
//     int temp = *a;
//     *a = *b;
//     *b = temp;
// }

// int main() {
//     int x = 10, y = 20;
//     swap(&x, &y);  // 必須傳遞位址
//     printf("x = %d, y = %d\n", x, y);
//     return 0;
// }
// ```

// ### C++（使用引用）

// ```cpp
// #include <iostream>

// void swap(int& a, int& b) {  // 使用引用
//     int temp = a;
//     a = b;
//     b = temp;
// }

// int main() {
//     int x = 10, y = 20;
//     swap(x, y);  // 不需要 &，語法更簡潔
//     std::cout << "x = " << x << ", y = " << y << std::endl;
//     return 0;
// }
// ```

// ### 引用 vs 指標

// | 特性 | 指標 | 引用 |
// |------|------|------|
// | 語法 | 需要 * 和 & | 更簡潔 |
// | 可為空 | 可以是 NULL | 不能為空，必須初始化 |
// | 可重新指向 | 可以 | 不可以，一旦綁定就固定 |
// | 使用方式 | 需要解引用 *p | 直接使用，像普通變數 |

// ---

// ## 2.11 差異九：結構體的使用

// ### C 語言

// ```c
// #include <stdio.h>

// struct Point {
//     int x;
//     int y;
// };

// int main() {
//     // C 語言中，宣告變數需要 struct 關鍵字
//     struct Point p1;
//     p1.x = 10;
//     p1.y = 20;
    
//     // 或者使用 typedef
//     // typedef struct Point Point;
    
//     printf("(%d, %d)\n", p1.x, p1.y);
//     return 0;
// }
// ```

// ### C++

// ```cpp
// #include <iostream>

// struct Point {
//     int x;
//     int y;
    
//     // C++ 的 struct 可以有成員函數！
//     void print() {
//         std::cout << "(" << x << ", " << y << ")" << std::endl;
//     }
// };

// int main() {
//     // C++ 不需要 struct 關鍵字
//     Point p1;
//     p1.x = 10;
//     p1.y = 20;
//     p1.print();  // 調用成員函數
    
//     return 0;
// }
// ```

// > **重要**：在 C++ 中，`struct` 和 `class` 幾乎相同，唯一的差異是預設存取權限：`struct` 預設 public，`class` 預設 private。

// ---

// ## 2.12 完整比較範例程式

// 以下程式展示了所有主要差異：

// ```cpp
// #include <iostream>
// #include <string>

// // ============================================================
// // 差異一：C++ 支援函數重載
// // ============================================================
// int square(int x) {
//     return x * x;
// }

// double square(double x) {
//     return x * x;
// }

// // ============================================================
// // 差異二：C++ 支援預設參數
// // ============================================================
// void printMessage(const std::string& msg, int times = 1) {
//     for (int i = 0; i < times; i++) {
//         std::cout << msg << std::endl;
//     }
// }

// // ============================================================
// // 差異三：C++ 支援引用參數
// // ============================================================
// void increment(int& value) {
//     value++;  // 直接修改原變數，不需要指標
// }

// // ============================================================
// // 差異四：C++ 的 struct 可以有成員函數
// // ============================================================
// struct Rectangle {
//     double width;
//     double height;
    
//     // 成員函數
//     double area() {
//         return width * height;
//     }
    
//     void print() {
//         std::cout << "Rectangle: " << width << " x " << height;
//         std::cout << ", Area = " << area() << std::endl;
//     }
// };

// // ============================================================
// // 主程式
// // ============================================================
// int main() {
//     std::cout << "========================================" << std::endl;
//     std::cout << "   C 與 C++ 關鍵差異展示" << std::endl;
//     std::cout << "========================================" << std::endl;
//     std::cout << std::endl;
    
//     // 1. 變數可以在任何位置宣告
//     std::cout << "[1] 變數宣告位置" << std::endl;
//     for (int i = 0; i < 3; i++) {  // i 在 for 迴圈內宣告
//         std::cout << "  迴圈 i = " << i << std::endl;
//     }
//     std::cout << std::endl;
    
//     // 2. 函數重載
//     std::cout << "[2] 函數重載" << std::endl;
//     std::cout << "  square(5) = " << square(5) << std::endl;
//     std::cout << "  square(3.14) = " << square(3.14) << std::endl;
//     std::cout << std::endl;
    
//     // 3. 預設參數
//     std::cout << "[3] 預設參數" << std::endl;
//     printMessage("  Hello!");           // 使用預設值 times = 1
//     printMessage("  Hi!", 3);           // 指定 times = 3
//     std::cout << std::endl;
    
//     // 4. 引用
//     std::cout << "[4] 引用參數" << std::endl;
//     int num = 10;
//     std::cout << "  原始值: " << num << std::endl;
//     increment(num);
//     std::cout << "  increment 後: " << num << std::endl;
//     std::cout << std::endl;
    
//     // 5. 內建 bool 類型
//     std::cout << "[5] 布林類型" << std::endl;
//     bool isReady = true;
//     std::cout << "  isReady = " << isReady << std::endl;
//     std::cout << "  isReady = " << std::boolalpha << isReady << std::endl;
//     std::cout << std::endl;
    
//     // 6. 動態記憶體配置
//     std::cout << "[6] 動態記憶體 (new/delete)" << std::endl;
//     int* p = new int(42);  // 配置並初始化為 42
//     std::cout << "  *p = " << *p << std::endl;
//     delete p;
//     std::cout << "  記憶體已釋放" << std::endl;
//     std::cout << std::endl;
    
//     // 7. struct 有成員函數
//     std::cout << "[7] struct 成員函數" << std::endl;
//     Rectangle rect;
//     rect.width = 5.0;
//     rect.height = 3.0;
//     std::cout << "  ";
//     rect.print();
//     std::cout << std::endl;
    
//     // 8. string 類別
//     std::cout << "[8] std::string 類別" << std::endl;
//     std::string greeting = "Hello";
//     std::string name = "World";
//     std::string message = greeting + ", " + name + "!";  // 字串串接
//     std::cout << "  " << message << std::endl;
//     std::cout << "  長度: " << message.length() << std::endl;
//     std::cout << std::endl;
    
//     std::cout << "========================================" << std::endl;
//     std::cout << "   展示完成" << std::endl;
//     std::cout << "========================================" << std::endl;
    
//     return 0;
// }
// ```

// ### 編譯與執行

// ```bash
// g++ -std=c++11 -o lesson02 lesson02.cpp
// ./lesson02
// ```

// ### 執行結果

// ```
// ========================================
//    C 與 C++ 關鍵差異展示
// ========================================

// [1] 變數宣告位置
//   迴圈 i = 0
//   迴圈 i = 1
//   迴圈 i = 2

// [2] 函數重載
//   square(5) = 25
//   square(3.14) = 9.8596

// [3] 預設參數
//   Hello!
//   Hi!
//   Hi!
//   Hi!

// [4] 引用參數
//   原始值: 10
//   increment 後: 11

// [5] 布林類型
//   isReady = 1
//   isReady = true

// [6] 動態記憶體 (new/delete)
//   *p = 42
//   記憶體已釋放

// [7] struct 成員函數
//   Rectangle: 5 x 3, Area = 15

// [8] std::string 類別
//   Hello, World!
//   長度: 13

// ========================================
//    展示完成
// ========================================
// ```

// ---

// ## 2.13 從 C 到 C++ 的轉換建議

// | C 習慣 | C++ 建議 |
// |--------|----------|
// | `printf` / `scanf` | 使用 `std::cout` / `std::cin` |
// | `char[]` 字串 | 使用 `std::string` |
// | `malloc` / `free` | 使用 `new` / `delete`（更好：智能指標） |
// | 宏定義常數 `#define PI 3.14` | 使用 `const double PI = 3.14;` |
// | 宏函數 | 使用 `inline` 函數或模板 |
// | 指標傳遞修改參數 | 考慮使用引用 |
// | 多個同功能不同名函數 | 使用函數重載 |

// ---

// ## 2.14 本課重點回顧

// | 差異項目 | 說明 |
// |----------|------|
// | 單行註解 | C++ 支援 `//`（C99 後 C 也支援） |
// | 變數宣告 | C++ 可在任意位置宣告 |
// | 輸入輸出 | C++ 使用 iostream，類型安全 |
// | 記憶體配置 | C++ 使用 new/delete，會調用建構/解構函數 |
// | bool 類型 | C++ 內建，不需標頭檔 |
// | 函數重載 | C++ 支援同名不同參數的函數 |
// | 預設參數 | C++ 支援為參數指定預設值 |
// | 引用 | C++ 新增的特性，比指標更安全易用 |
// | struct | C++ 的 struct 可以有成員函數 |

// ---

// ## 2.15 下一課預告

// 在下一課中，我們將設置 C++ 的編譯環境，學習如何使用 g++ 和 clang++ 編譯 C++ 程式，以及常用的編譯選項。

// 準備好進入 **第 3 課：C++ 編譯環境設置** 了嗎？



#include <iostream>
#include <string>

// ============================================================
// 差異一：C++ 支援函數重載
// ============================================================
int square(int x) {
    return x * x;
}

double square(double x) {
    return x * x;
}

// ============================================================
// 差異二：C++ 支援預設參數
// ============================================================
void printMessage(const std::string& msg, int times = 1) {
    for (int i = 0; i < times; i++) {
        std::cout << msg << std::endl;
    }
}

// ============================================================
// 差異三：C++ 支援引用參數
// ============================================================
void increment(int& value) {
    value++;  // 直接修改原變數，不需要指標
}

// ============================================================
// 差異四：C++ 的 struct 可以有成員函數
// ============================================================
struct Rectangle {
    double width;
    double height;
    
    // 成員函數
    double area() {
        return width * height;
    }
    
    void print() {
        std::cout << "Rectangle: " << width << " x " << height;
        std::cout << ", Area = " << area() << std::endl;
    }
};

// ============================================================
// 主程式
// ============================================================
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   C 與 C++ 關鍵差異展示" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // 1. 變數可以在任何位置宣告
    std::cout << "[1] 變數宣告位置" << std::endl;
    for (int i = 0; i < 3; i++) {  // i 在 for 迴圈內宣告
        std::cout << "  迴圈 i = " << i << std::endl;
    }
    std::cout << std::endl;
    
    // 2. 函數重載
    std::cout << "[2] 函數重載" << std::endl;
    std::cout << "  square(5) = " << square(5) << std::endl;
    std::cout << "  square(3.14) = " << square(3.14) << std::endl;
    std::cout << std::endl;
    
    // 3. 預設參數
    std::cout << "[3] 預設參數" << std::endl;
    printMessage("  Hello!");           // 使用預設值 times = 1
    printMessage("  Hi!", 3);           // 指定 times = 3
    std::cout << std::endl;
    
    // 4. 引用
    std::cout << "[4] 引用參數" << std::endl;
    int num = 10;
    std::cout << "  原始值: " << num << std::endl;
    increment(num);
    std::cout << "  increment 後: " << num << std::endl;
    std::cout << std::endl;
    
    // 5. 內建 bool 類型
    std::cout << "[5] 布林類型" << std::endl;
    bool isReady = true;
    std::cout << "  isReady = " << isReady << std::endl;
    std::cout << "  isReady = " << std::boolalpha << isReady << std::endl;
    std::cout << std::endl;
    
    // 6. 動態記憶體配置
    std::cout << "[6] 動態記憶體 (new/delete)" << std::endl;
    int* p = new int(42);  // 配置並初始化為 42
    std::cout << "  *p = " << *p << std::endl;
    delete p;
    std::cout << "  記憶體已釋放" << std::endl;
    std::cout << std::endl;
    
    // 7. struct 有成員函數
    std::cout << "[7] struct 成員函數" << std::endl;
    Rectangle rect;
    rect.width = 5.0;
    rect.height = 3.0;
    std::cout << "  ";
    rect.print();
    std::cout << std::endl;
    
    // 8. string 類別
    std::cout << "[8] std::string 類別" << std::endl;
    std::string greeting = "Hello";
    std::string name = "World";
    std::string message = greeting + ", " + name + "!";  // 字串串接
    std::cout << "  " << message << std::endl;
    std::cout << "  長度: " << message.length() << std::endl;
    std::cout << std::endl;
    
    std::cout << "========================================" << std::endl;
    std::cout << "   展示完成" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
