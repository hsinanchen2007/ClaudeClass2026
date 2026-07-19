/*
 * ============================================================
 * 【第 2 課：C 與 C++ 的關鍵差異】總複習 summary.cpp
 * ============================================================
 *
 * 本課程重點：
 * 1. 差異一：變數宣告位置（C89 vs C++）
 * 2. 差異二：輸入輸出方式（printf/scanf vs cout/cin）
 * 3. 差異三：記憶體管理（malloc/free vs new/delete）
 * 4. 差異四：布林類型（stdbool.h vs 內建 bool）
 * 5. 差異五：函數重載（Function Overloading）
 * 6. 差異六：預設參數（Default Arguments）
 * 7. 差異七：引用（Reference）
 * 8. 差異八：struct 的增強（可以有成員函數）
 * 9. 差異九：std::string 類別
 *
 * ============================================================
 * 總體差異概覽：
 *   面向         C 語言                C++
 *   設計範式     程序式                多範式
 *   記憶體管理   malloc / free         new / delete + 智能指標
 *   輸入輸出     printf / scanf        iostream（cout / cin）
 *   字串處理     char 陣列 + 函數      std::string 類別
 *   函數重載     不支援                支援
 *   預設參數     不支援                支援
 *   引用         不支援（只有指標）    支援引用（reference）
 *   布林類型     需要 stdbool.h        內建 bool
 *   命名空間     不支援                支援 namespace
 * ============================================================
 */

#include <iostream>
#include <string>
#include <limits>

#include <cmath>
// ============================================================
// 差異一：變數宣告位置
// ============================================================
//
// C89 風格（限制）：
//   - 變數必須在區塊（{}）的開頭宣告
//   - 不能在使用之前的任意位置宣告
//   示例：
//     int i;      // 必須在最上面
//     int sum;
//     sum = 0;
//     for (i = 0; i < 10; i++) { ... }  // i 是預先宣告的
//
// C++ 風格（自由）：
//   - 可以在任意位置宣告變數
//   - 推薦：在第一次使用時才宣告，並同時初始化
//   - 甚至可以在 for 迴圈頭部宣告迴圈變數！
//
// 最佳實踐：在最接近使用點的地方宣告變數，這樣：
//   1. 程式碼更清晰（宣告和使用放在一起）
//   2. 減少錯誤（避免意外使用未初始化的值）
//   3. 縮小變數的作用域（for 迴圈內的 i 在迴圈外不可見）

void demonstrateVariableDeclaration() {
    std::cout << "\n=== 差異一：變數宣告位置 ===" << std::endl;

    // C++ 風格：在第一次需要時才宣告
    int sum = 0;  // 宣告時立即初始化

    // for 迴圈頭部宣告的變數，作用域只在迴圈內
    for (int i = 0; i < 5; i++) {
        sum += i;
    }
    // 此處 i 已不可見（C89 中需要在上方宣告 int i，這裡都能用）

    // 需要時才宣告 result，更清晰
    int result = sum * 2;
    std::cout << "sum = " << sum << ", result = " << result << std::endl;
}


// ============================================================
// 差異二：輸入輸出方式
// ============================================================
//
// C 語言（printf/scanf）：
//   - 需要手動指定格式符（%d, %f, %s 等）
//   - 格式符錯誤不會在編譯時報錯，可能在執行時崩潰
//   - 不能用於自定義類型
//   示例：printf("年齡: %d\n", age);
//         scanf("%d", &age);  // 注意需要 &
//
// C++（cout/cin）：
//   - 自動識別類型，不需要格式符
//   - 類型錯誤可以在編譯時被檢查
//   - 可以為自定義類別重載 << 和 >> 運算子
//   示例：std::cout << "年齡: " << age << std::endl;
//         std::cin >> age;   // 不需要 &

void demonstrateIO() {
    std::cout << "\n=== 差異二：輸入輸出 ===" << std::endl;

    int age = 25;
    double height = 175.5;
    std::string name = "張小明";
    bool isStudent = true;

    // 鏈式輸出：<< 可以連續使用
    std::cout << "姓名: " << name << std::endl;
    std::cout << "年齡: " << age << " 歲" << std::endl;
    std::cout << "身高: " << height << " cm" << std::endl;

    // 自動識別 bool 類型，輸出 1（預設）或 true（用 boolalpha）
    std::cout << "是學生: " << isStudent << std::endl;           // 輸出 1
    std::cout << "是學生: " << std::boolalpha << isStudent << std::endl;  // 輸出 true
}


// ============================================================
// 差異三：記憶體管理（new/delete vs malloc/free）
// ============================================================
//
// C 語言（malloc/free）：
//   int* p = (int*)malloc(sizeof(int));  // 需要轉型！需要 sizeof！
//   free(p);
//
//   int* arr = (int*)malloc(5 * sizeof(int));
//   free(arr);  // 無論是單變數還是陣列，都用 free
//
// C++（new/delete）：
//   int* p = new int;      // 不需要 sizeof，不需要轉型
//   delete p;              // 釋放單變數
//
//   int* arr = new int[5]; // 配置陣列
//   delete[] arr;          // 釋放陣列必須用 delete[]！！
//
// 關鍵差異：
//   1. new/delete 會自動調用建構/解構函數
//   2. new 返回正確的指標類型，不需要轉型
//   3. 陣列必須嚴格配對：new[] 對應 delete[]，混用是未定義行為！

void demonstrateMemoryManagement() {
    std::cout << "\n=== 差異三：記憶體管理 ===" << std::endl;

    // 配置單一整數
    int* p = new int;  // 不需要 sizeof，不需要轉型
    *p = 42;
    std::cout << "new int 的值: " << *p << std::endl;
    delete p;          // 用 delete 釋放（不是 free）
    p = nullptr;       // 好習慣：釋放後設為 nullptr，避免懸空指標

    // 配置並同時初始化
    int* q = new int(100);  // 初始化為 100
    std::cout << "new int(100) 的值: " << *q << std::endl;
    delete q;
    q = nullptr;

    // 配置陣列
    int* arr = new int[5];  // 配置 5 個 int 的陣列
    for (int i = 0; i < 5; i++) {
        arr[i] = i * 10;
    }
    std::cout << "陣列: ";
    for (int i = 0; i < 5; i++) {
        std::cout << arr[i] << " ";
    }
    std::cout << std::endl;
    delete[] arr;  // 釋放陣列必須用 delete[]！！
    arr = nullptr;

    // 注意：new 配置記憶體失敗時會拋出 std::bad_alloc 例外
    // 而 malloc 失敗時返回 NULL
}


// ============================================================
// 差異四：布林類型
// ============================================================
//
// C 語言（C89）：
//   - 沒有 bool 類型，通常用 int 代替（0 為假，非0 為真）
//   - C99 之後，需要 #include <stdbool.h> 才有 bool
//
// C++：
//   - bool 是內建類型，不需要任何標頭檔
//   - true 和 false 是關鍵字
//   - std::boolalpha 可以讓 cout 輸出 "true" 和 "false"

void demonstrateBool() {
    std::cout << "\n=== 差異四：布林類型 ===" << std::endl;

    bool isReady = true;   // 內建，不需要 stdbool.h
    bool isDone = false;

    // 預設輸出 1 或 0
    std::cout << "isReady = " << isReady << std::endl;
    std::cout << "isDone = " << isDone << std::endl;

    // 使用 std::boolalpha 輸出 "true" 或 "false"
    std::cout << std::boolalpha;
    std::cout << "isReady = " << isReady << std::endl;
    std::cout << "isDone = " << isDone << std::endl;
    std::cout << std::noboolalpha;  // 恢復預設

    // bool 的運算
    bool a = true, b = false;
    std::cout << "a && b = " << std::boolalpha << (a && b) << std::endl;
    std::cout << "a || b = " << (a || b) << std::endl;
    std::cout << "!a = " << (!a) << std::endl;
    std::cout << std::noboolalpha;
}


// ============================================================
// 差異五：函數重載（Function Overloading）
// ============================================================
//
// C 語言：不支援，每個函數必須有不同的名稱
//   int add_int(int a, int b) { ... }
//   double add_double(double a, double b) { ... }
//   → 名稱不同但功能相似，維護困難
//
// C++：支援同名函數，只要參數不同即可
//   int add(int a, int b) { ... }
//   double add(double a, double b) { ... }
//   int add(int a, int b, int c) { ... }
//   → 編譯器根據參數的類型和數量自動選擇正確版本
//
// 重載的規則：
//   - 參數的「類型」不同 → 可以重載
//   - 參數的「數量」不同 → 可以重載
//   - 只有「返回類型」不同 → 不能重載（編譯錯誤）

// 同名但參數類型不同
int square(int x) {
    std::cout << "  [呼叫 int 版本]" << std::endl;
    return x * x;
}

double square(double x) {
    std::cout << "  [呼叫 double 版本]" << std::endl;
    return x * x;
}

// 同名但參數數量不同
int add(int a, int b) {
    return a + b;
}

int add(int a, int b, int c) {
    return a + b + c;
}

double add(double a, double b) {
    return a + b;
}

void demonstrateFunctionOverloading() {
    std::cout << "\n=== 差異五：函數重載 ===" << std::endl;

    // 編譯器自動選擇正確的版本
    std::cout << "square(5) = " << square(5) << std::endl;
    std::cout << "square(3.14) = " << square(3.14) << std::endl;
    std::cout << "add(3, 5) = " << add(3, 5) << std::endl;
    std::cout << "add(1, 2, 3) = " << add(1, 2, 3) << std::endl;
    std::cout << "add(3.14, 2.86) = " << add(3.14, 2.86) << std::endl;
}


// ============================================================
// 差異六：預設參數（Default Arguments）
// ============================================================
//
// C 語言：不支援，每次呼叫都必須提供所有參數
//   void greet(const char* name, const char* greeting) { ... }
//   greet("Alice", "Hello");  // 即使每次 greeting 都是 "Hello"
//
// C++：支援為參數指定預設值
//   void greet(const std::string& name, const std::string& greeting = "Hello")
//   greet("Alice");           // 使用預設 "Hello"
//   greet("Bob", "Hi");       // 覆蓋預設值
//
// 規則：預設參數必須從最右邊開始！
//   void func(int a, int b = 10, int c = 20)  // 合法
//   void func(int a = 5, int b, int c = 20)   // 非法！

// 預設參數必須從右側開始設定
void greet(const std::string& name,
           const std::string& greeting = "Hello",
           const std::string& punctuation = "!") {
    std::cout << greeting << ", " << name << punctuation << std::endl;
}

void printMessage(const std::string& msg, int times = 1) {
    for (int i = 0; i < times; i++) {
        std::cout << "  " << msg << std::endl;
    }
}

void demonstrateDefaultArguments() {
    std::cout << "\n=== 差異六：預設參數 ===" << std::endl;

    greet("Alice");               // 使用所有預設值
    greet("Bob", "Hi");           // 覆蓋 greeting，punctuation 使用預設
    greet("Charlie", "Hey", "~"); // 覆蓋所有參數

    printMessage("Hello!");       // times = 1（預設）
    printMessage("重複三次!", 3); // times = 3
}


// ============================================================
// 差異七：引用（Reference）
// ============================================================
//
// C 語言：只有指標，要修改外部變數必須傳遞位址
//   void swap(int* a, int* b) { ... }
//   swap(&x, &y);  // 必須用 & 取位址
//
// C++：有引用，語法更簡潔，行為更安全
//   void swap(int& a, int& b) { ... }
//   swap(x, y);    // 直接傳變數，不需要 &
//
// 引用 vs 指標的關鍵差異：
//   特性           指標              引用
//   語法           需要 * 和 &       更簡潔
//   可為空         可以是 nullptr    不能為空，必須初始化
//   可重新綁定     可以              不可以，一旦綁定就固定
//   使用方式       需要解引用 *p     直接使用，像普通變數

// 使用指標的 swap（C 風格）
void swapByPointer(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

// 使用引用的 swap（C++ 風格）
void swapByReference(int& a, int& b) {
    int temp = a;  // 直接使用 a，不需要解引用
    a = b;
    b = temp;
}

// const 引用：高效地傳遞大型物件，同時防止被修改
// 避免了複製大型字串的開銷，又不允許函數修改它
void printString(const std::string& str) {
    std::cout << "字串: " << str << " (長度: " << str.length() << ")" << std::endl;
}

void demonstrateReference() {
    std::cout << "\n=== 差異七：引用 ===" << std::endl;

    int x = 10, y = 20;
    std::cout << "交換前: x = " << x << ", y = " << y << std::endl;

    // 指標方式：需要 &
    swapByPointer(&x, &y);
    std::cout << "指標交換後: x = " << x << ", y = " << y << std::endl;

    // 引用方式：不需要 &，更簡潔
    swapByReference(x, y);
    std::cout << "引用交換後: x = " << x << ", y = " << y << std::endl;

    // const 引用示範
    std::string bigString = "這是一段很長的字串，傳遞引用比複製更高效";
    printString(bigString);

    // 引用的別名特性
    int original = 42;
    int& alias = original;  // alias 是 original 的別名
    alias = 100;            // 修改 alias 就是修改 original
    std::cout << "original = " << original << " (透過 alias 修改)" << std::endl;
}


// ============================================================
// 差異八：struct 的增強
// ============================================================
//
// C 語言 struct：
//   - 只能包含資料成員（變數）
//   - 宣告變數時需要加 struct 關鍵字：struct Point p1;
//   - 通常配合 typedef 使用
//
// C++ struct：
//   - 可以包含成員函數！
//   - 宣告變數不需要 struct 關鍵字：Point p1;
//   - 實際上和 class 幾乎相同（唯一差異：預設存取權限不同）
//     - struct 預設 public
//     - class 預設 private

struct Point {
    int x;  // 資料成員
    int y;

    // 成員函數！（C 語言的 struct 不能有這個）
    void print() const {
        std::cout << "Point(" << x << ", " << y << ")" << std::endl;
    }

    // 計算到原點的距離
    double distanceToOrigin() const {
        return std::sqrt(x * x + y * y);
    }

    // 平移方法
    void translate(int dx, int dy) {
        x += dx;
        y += dy;
    }
};

struct Rectangle {
    double width;
    double height;

    double area() const {
        return width * height;
    }

    double perimeter() const {
        return 2 * (width + height);
    }

    void print() const {
        std::cout << "Rectangle(" << width << " x " << height
                  << "), 面積=" << area()
                  << ", 周長=" << perimeter() << std::endl;
    }
};

void demonstrateStruct() {
    std::cout << "\n=== 差異八：struct 增強 ===" << std::endl;

    // C++ 不需要 struct 關鍵字（C 語言需要 struct Point p1;）
    Point p1;
    p1.x = 10;
    p1.y = 20;
    p1.print();                    // 呼叫成員函數
    p1.translate(5, -3);           // 平移
    p1.print();

    Rectangle rect;
    rect.width = 5.0;
    rect.height = 3.0;
    rect.print();
}


// ============================================================
// 差異九：std::string 類別
// ============================================================
//
// C 語言（char 陣列）：
//   char name[50];            // 固定大小，容易溢位
//   strcpy(name, "Alice");    // 需要 string.h
//   strcat(name, " Smith");   // 字串連接需要函數
//   printf("%s\n", name);
//
// C++（std::string）：
//   std::string name = "Alice";  // 可動態增長
//   name += " Smith";             // 直接用 + 連接！
//   std::cout << name;
//   std::cout << name.length();  // 內建的成員函數

void demonstrateString() {
    std::cout << "\n=== 差異九：std::string 類別 ===" << std::endl;

    // 建立字串
    std::string greeting = "Hello";
    std::string name = "World";

    // 字串連接（+運算子，比 C 語言的 strcat 更直覺）
    std::string message = greeting + ", " + name + "!";
    std::cout << "連接結果: " << message << std::endl;

    // 常用成員函數
    std::cout << "長度: " << message.length() << std::endl;
    std::cout << "是否為空: " << std::boolalpha << message.empty() << std::endl;

    // 字串比較（可以直接用 == 和 < 等運算子）
    std::string a = "apple", b = "banana";
    std::cout << "\"apple\" == \"apple\": " << (a == "apple") << std::endl;
    std::cout << "\"apple\" < \"banana\": " << (a < b) << std::endl;
    std::cout << std::noboolalpha;

    // 子字串
    std::string sub = message.substr(0, 5);  // 取前 5 個字元
    std::cout << "前5個字元: " << sub << std::endl;

    // 查找
    size_t pos = message.find("World");
    if (pos != std::string::npos) {
        std::cout << "\"World\" 在位置: " << pos << std::endl;
    }
}


// ============================================================
// 主程式：展示所有關鍵差異
// ============================================================
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   C 與 C++ 關鍵差異完整展示" << std::endl;
    std::cout << "========================================" << std::endl;

    demonstrateVariableDeclaration();
    demonstrateIO();
    demonstrateMemoryManagement();
    demonstrateBool();
    demonstrateFunctionOverloading();
    demonstrateDefaultArguments();
    demonstrateReference();
    demonstrateStruct();
    demonstrateString();

    std::cout << "\n========================================" << std::endl;
    std::cout << "   所有差異展示完成" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}

/*
 * ============================================================
 * 從 C 到 C++ 的轉換建議
 * ============================================================
 *
 * C 習慣                  C++ 建議
 * ──────────────────────  ─────────────────────────────────
 * printf / scanf          使用 std::cout / std::cin
 * char[] 字串             使用 std::string
 * malloc / free           使用 new / delete（更好：智能指標）
 * #define PI 3.14         使用 const double PI = 3.14;
 * 宏函數 #define ADD(a,b)  使用 inline 函數或模板
 * 指標修改參數             考慮使用引用（int& x）
 * 多個同功能不同名函數     使用函數重載
 * struct 關鍵字            直接用類型名稱宣告
 *
 * ============================================================
 * 本課重點回顧
 * ============================================================
 *
 * 差異項目       說明
 * ─────────────  ───────────────────────────────────────────
 * 變數宣告       C++ 可在任意位置宣告（在使用點附近宣告）
 * 輸入輸出       C++ 使用 iostream，自動識別類型，類型安全
 * 記憶體配置     C++ 使用 new/delete，會調用建構/解構函數
 * bool 類型      C++ 內建，不需標頭檔，true/false 是關鍵字
 * 函數重載       C++ 支援同名不同參數的函數
 * 預設參數       C++ 支援為參數指定預設值（從右側開始）
 * 引用           C++ 新增特性，語法更簡潔，比指標更安全
 * struct 增強    C++ 的 struct 可以有成員函數
 * std::string    比 char[] 更安全、更方便的字串類型
 * ============================================================
 */
