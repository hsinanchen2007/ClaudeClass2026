// =============================================================================
//  第 2 課 範例 17  —  C 與 C++ 關鍵差異的一次性總覽（可執行對照）
// =============================================================================
//
//  ※ 本檔結構說明：
//    開頭一大段以 `//` 寫成的內容是課程講義（Markdown 格式，含表格與
//    示範用的程式片段，那些片段「不會被編譯」）。真正會被編譯執行的程式碼，
//    從下方 `#include <iostream>` 那一行開始。閱讀時請注意區分。
//
// 【主題資訊 Information】
//   標準版本：本檔用到的特性（重載、預設參數、引用、struct 成員函式、
//             std::string）全部是 C++98 就有的；本檔以 C++17 編譯
//   標頭檔  ：<iostream>、<string>
//   對照對象：C89/C99 vs C++
//
// 【詳細解釋 Explanation】
//
// 【1. 五個差異，其實是同一個方向】
//   本檔逐一示範的五項差異，共通的設計動機是
//   「把原本由程式設計師用紀律維持的事，交給編譯器在編譯期保證」：
//     * 函數重載   —— 同一個概念用同一個名字，不必手動把型別編進函式名
//                    （C 得寫 squareInt / squareDouble）。
//     * 預設參數   —— 不必為了「少傳一個參數」而多寫一個轉呼叫的函式。
//     * 引用參數   —— 表達「這裡一定有一個有效物件」，呼叫端不必檢查 null，
//                    函式內也不必解參考。
//     * struct 成員函式 —— 資料與操作它的邏輯放在一起，而不是散落各處。
//     * std::string —— 自動管理記憶體、知道自己的長度、可以用 == 比內容。
//
// 【2. 函數重載的底層機制：name mangling】
//   C 的函式 square 在目的檔中的符號就叫 square，同名必然衝突。
//   C++ 會把參數型別編進符號名稱（name mangling），
//   使 square(int) 與 square(double) 產生不同符號，因而能共存。
//   兩個重要推論：
//     (a) 重載的區分依據是「參數列」，不含回傳型別 ——
//         只有回傳型別不同無法構成重載，因為呼叫端的寫法無從區分。
//     (b) C++ 呼叫 C 函式庫時必須用 extern "C"，
//         否則會去找修飾過的符號而連結失敗。這就是幾乎每個 C 標頭檔裡
//         都有 #ifdef __cplusplus / extern "C" 那段樣板的原因。
//
// 【3. 預設參數的規則】
//   只能在「宣告處」指定一次（通常寫在標頭檔），定義處再寫一次會編譯錯誤；
//   而且必須由右至左連續指定 —— 因為呼叫端是由左至右填入引數，
//   若中間有空缺，編譯器無法判斷你省略的是哪一個。
//
// 【4. 引用與指標：選哪一個由語意決定】
//   引用必須在宣告時綁定、之後不可改綁、不可為 null。
//   所以 `void increment(int& value)` 這個簽章本身就向呼叫端保證
//   「一定會有一個有效的物件」，函式內不必寫 null 檢查。
//   換成 `void increment(int* value)` 就必須檢查，因為指標可能是 nullptr。
//   實務準則：「這個東西一定存在」→ 引用；
//   「可能不存在或需要改指向」→ 指標（現代 C++ 中更常是 std::optional）。
//
// 【概念補充 Concept Deep Dive】
//   C++ 的 `struct` 與 `class` 只差一件事：預設存取權限
//   （struct 預設 public，class 預設 private），其餘完全相同。
//   struct 一樣可以有建構函式、成員函式、繼承，甚至虛擬函式。
//   這與 C 的 struct（純粹是資料的集合）是本質上的不同。
//   慣例是：純資料聚合、沒有不變量要維護 → struct；
//   有不變量要靠成員函式守護 → class。
//
//   std::string 相對 char 陣列的價值，最容易被忽略的是「比較」：
//   C 的 `char* a, *b; a == b` 比較的是「位址」而不是內容，
//   要比內容必須用 strcmp —— 這是 C 最經典的陷阱之一。
//   std::string 的 == 直接比內容，符合直覺。
//   另外 strlen 是 O(n)（要掃到結尾的 '\0'），
//   而 std::string::size() 是 O(1)（長度是存起來的）。
//   注意 size() 回傳的是「位元組數」而非「字元數」：
//   UTF-8 下一個中文字通常佔 3 個位元組。
//
// 【注意事項 Pay Attention】
// 1. 重載的區分依據是參數列，不包含回傳型別。
// 2. 預設參數只能在宣告處指定一次，且必須由右至左連續指定。
// 3. 引用必須在宣告時初始化，且之後不可改綁、不可為 null。
// 4. C 的 char* 用 == 比的是位址不是內容；std::string 的 == 才是比內容。
// 5. 本檔以 std::cout 取代 printf：cout 是型別安全的，
//    不會發生 printf("%d", someDouble) 這類格式與型別對不上的未定義行為。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】C 與 C++ 的關鍵差異
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++ 的函數重載是怎麼做到的？C 為什麼不行？
//     答：靠 name mangling（名稱修飾）。C++ 會把參數型別編進目的檔中的
//         符號名稱，使 square(int) 與 square(double) 產生不同符號而能共存；
//         C 的符號名稱就是函式名本身，同名必然衝突。
//     追問：那可以只靠回傳型別不同來構成重載嗎？
//         → 不行。重載的區分依據只有參數列，不含回傳型別 ——
//           因為 `square(5);` 這樣的呼叫（忽略回傳值）無從判斷該選哪一個。
//
// 🔥 Q2. 什麼時候該用引用、什麼時候該用指標？
//     答：看語意。引用不可為 null、不可改綁，所以它表達的是
//         「這裡一定有一個有效的物件」——參數用引用等於把這個保證
//         寫進了簽章，函式內不必檢查 null。
//         指標則表達「可能不存在，或需要改指向別的東西」。
//         實務準則是能用引用就用引用，很多 null 檢查因此根本不必寫。
//     追問：那「可能不存在」的情況，現代 C++ 有更好的表達方式嗎？
//         → 有。std::optional<T>（C++17）表達「可能沒有值」比裸指標清楚，
//           而且不會混淆「空指標」與「指向一個空物件」；
//           若牽涉所有權則用 unique_ptr / shared_ptr。
//           裸指標在現代 C++ 中應該只用於「非擁有、可為空」的觀察用途。
//
// ⚠️ 陷阱. 「C++ 就是 C 加上類別，所以我把 .c 改成 .cpp、
//           繼續用原本的寫法就是在寫 C++ 了。」
//     答：能編譯不代表是好的 C++，而且兩者並不完全相容
//         （例如 C++ 不允許 void* 隱式轉成 T*，C 允許）。
//         更重要的是，繼續用 malloc/free 就等於放棄了
//         建構／解構函數與 RAII —— 而那正是 C++ 相對 C 最大的價值：
//         資源的釋放由編譯器保證，不再依賴人的紀律。
//     為什麼會錯：把 C++ 的差異理解成「多了一些語法糖」，
//         而不是「多了一套資源與生命週期的管理模型」。
//         本課列出的九項差異裡，真正改變寫程式方式的是
//         new/delete 會呼叫建構與解構函數這一項 ——
//         它一路長成 RAII、智能指標，以及後面所有關於物件生命週期的課。
// ═══════════════════════════════════════════════════════════════════════════
//
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔不附
//   理由：本檔是語言特性的橫向對照（重載、預設參數、引用、struct、string），
//   不是單一演算法主題。LeetCode 題目不會因為你用 printf 還是 cout、
//   用 char 陣列還是 std::string 而有不同答案。
//   依規格「寧缺勿濫」從缺——本檔各節的 C/C++ 對照示範本身就是重點。
// -----------------------------------------------------------------------------

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

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2 課：C 與 C++ 的關鍵差異17.cpp" -o cpp_vs_c

// ── 輸出但書 ────────────────────────────────────────────────────────────
// 1. 本檔輸出完全由程式邏輯決定，逐位元組可重現
//    （實測連跑 5 次 md5 相同）。
// 2. [2] 函數重載：square(5) 選中 int 版本得 25，
//    square(3.14) 選中 double 版本得 9.8596 —— 兩者共存靠的是
//    name mangling 把參數型別編進了符號名稱。
// 3. [3] 預設參數：第一次呼叫省略第二個引數（預設 1 次），
//    第二次明確傳 3，因此印出 1 行 + 3 行。
// 4. [8] std::string 的「長度: 13」是 "Hello, World!" 的位元組數
//    （13 個 ASCII 字元，剛好等於字元數）。
//    若換成中文字串，size() 回傳的仍是位元組數而非字元數 ——
//    UTF-8 下一個中文字通常佔 3 個位元組。
// 5. 全檔沒有未定義行為，也沒有印出任何位址或不確定值。
// 6. 本機環境：GCC 15.2.0 (Ubuntu 15.2.0-16ubuntu1) / libstdc++ / x86-64。

// === 預期輸出 ===
// ========================================
//    C 與 C++ 關鍵差異展示
// ========================================
//
// [1] 變數宣告位置
//   迴圈 i = 0
//   迴圈 i = 1
//   迴圈 i = 2
//
// [2] 函數重載
//   square(5) = 25
//   square(3.14) = 9.8596
//
// [3] 預設參數
//   Hello!
//   Hi!
//   Hi!
//   Hi!
//
// [4] 引用參數
//   原始值: 10
//   increment 後: 11
//
// [5] 布林類型
//   isReady = 1
//   isReady = true
//
// [6] 動態記憶體 (new/delete)
//   *p = 42
//   記憶體已釋放
//
// [7] struct 成員函數
//   Rectangle: 5 x 3, Area = 15
//
// [8] std::string 類別
//   Hello, World!
//   長度: 13
//
// ========================================
//    展示完成
// ========================================
