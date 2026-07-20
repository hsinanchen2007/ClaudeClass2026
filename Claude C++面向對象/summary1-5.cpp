/*
 * ================================================================
 *  【第 1～5 課總複習】summary1-5.cpp
 * ================================================================
 *  涵蓋課程：
 *    第 1 課：C++ 的歷史與設計哲學
 *    第 2 課：C 與 C++ 的關鍵差異
 *    第 3 課：C++ 編譯環境設置（g++、clang++、MSVC）
 *    第 4 課：命名空間（namespace）基礎
 *    第 5 課：輸入輸出流（iostream）入門
 *
 *  編譯：g++ -std=c++17 -Wall -Wextra -o summary1-5 summary1-5.cpp
 *  MSVC：cl /std:c++17 /utf-8 /EHsc /W4 summary1-5.cpp
 * ================================================================
 */

// =============================================================================
//
// 【主題資訊 Information】
//   範圍：  C++ 的語言定位（第 1 課）→ 與 C 的具體差異（第 2 課）
//           → 工具鏈（第 3 課）→ 名稱管理（第 4 課）→ I/O（第 5 課）
//   標準：  本檔以 C++17 為基準；文中凡涉及版本差異處均逐一標明
//   標頭檔：<iostream> <iomanip> <string> <vector> <algorithm> <limits> <cmath>
//   關鍵詞：zero-overhead principle、multi-paradigm、overloading、reference、
//           namespace、ADL、stream manipulator、sticky vs non-sticky
//
// 【詳細解釋 Explanation】
//
// 【1. 「零開銷原則」到底在講什麼（第 1 課的核心）】
// Stroustrup 的原話是兩句話，兩句都重要：
//     "What you don't use, you don't pay for."（沒用到的功能不付代價）
//     "What you do use, you couldn't hand-code any better."
//                                    （用到的功能，你手寫也不會更快）
// 第一句的意思是：C++ 有 class、virtual、exception，但你不寫 virtual
// 就不會有 vtable，不寫 exception 就（在多數實作下）不付執行期代價。
// 這是它與「一切皆物件、一切皆動態派發」的語言最根本的分歧。
// 第二句更關鍵：抽象不是免費的糖衣，而是**編譯期就被消掉**。
//   本檔 getMax<T> 這個樣板，編譯後與手寫的 int/double 版本產生相同的機器碼；
//   Calculator::multiply 這個成員函式，編譯後與 multiply(Calculator*, int, int)
//   這種 C 風格函式沒有差別 —— this 就是被隱藏起來的第一個參數。
// ⚠️ 但要誠實：零開銷原則是**設計目標**，不是所有情況下的保證。
//    RTTI、exception 的 landing pad、iostream 的虛擬繼承都有實際成本，
//    這也是嵌入式領域常關掉 -fno-exceptions / -fno-rtti 的原因。
//
// 【2. 為什麼 C++ 保留了那麼多 C 的東西（第 2 課的背景）】
// 第 2 課列的九個差異，每一個都可以問「那為什麼不乾脆廢掉 C 的做法？」
// 答案是相容性：C++ 要能直接吃掉既有的 C 程式庫與 C 標頭檔。
// 這帶來兩個長期後果，本檔的範例都碰得到：
//   (a) 同一件事有兩套做法（malloc/new、printf/cout、char*/string），
//       而新手常混用。原則是：**在 C++ 裡一律用 C++ 的那一套**，
//       只有跟 C API 介接時才碰 C 的那套。
//   (b) 有些 C 的坑原封不動被繼承下來：陣列退化成指標、
//       delete 與 delete[] 不可混用、未初始化變數是 UB。
// 特別注意本檔 demo_lesson2 裡的 new/delete 示範 —— 那是為了對照 malloc/free
// 才手寫的。現代 C++ 的正確答案是 std::vector 與智慧指標（第 20、21 課），
// 手動 new/delete 在實務程式碼中應該幾乎絕跡。
//
// 【3. 函數重載為什麼在 C 做不到：name mangling】
// C 的連結器只認函式名字，所以 square 只能有一個。
// C++ 編譯器會把參數型別編碼進符號名稱（name mangling），於是
//     int    square(int)      →  _Z6squarei
//     double square(double)   →  _Z6squared
// 兩者在連結器眼中是完全不同的符號，自然可以共存。
// 這也解釋了三件實務上會遇到的事：
//   * 回傳型別**不參與**重載決議（因為呼叫端不必然知道回傳型別要什麼），
//     所以 `int f(); double f();` 是編譯錯誤。
//   * 要讓 C++ 函式被 C 呼叫，必須寫 extern "C" 關掉 mangling。
//   * 連結錯誤訊息裡那串 _Z... 亂碼，可以用 c++filt 還原成人看得懂的簽名。
//
// 【4. namespace 的真正機制：ADL（第 4 課最常被略過的一半）】
// 課程講了三種存取方式，但沒講「為什麼 `std::cout << x` 能運作」。
// operator<< 並不是成員函式，它是自由函式 std::operator<<。
// 你寫 `cout << 42` 而沒有寫 `std::operator<<(cout, 42)`，靠的是
// **ADL（Argument-Dependent Lookup，實參相依查找）**：
//   編譯器看到未限定的函式名稱時，除了一般作用域，
//   還會去「所有實參型別所屬的 namespace」裡找。
//   cout 是 std::ostream → 於是 std 也被納入查找範圍 → 找到 std::operator<<。
// 這是為什麼自訂型別的 operator<< 應該定義在**該型別所在的 namespace 裡**，
// 而不是丟到全域 —— 否則使用者要寫完全限定名稱才叫得到。
//
// 【5. iostream 的 sticky 與 non-sticky（第 5 課最容易出錯的地方）】
// 格式操縱器分兩類，混淆它們是實務上格式跑掉的頭號原因：
//   * sticky（設定後一直有效，直到再次改變）：
//     setprecision、fixed/scientific、setfill、boolalpha、hex/oct/dec、left/right
//   * non-sticky（只影響「下一個」輸出項）：
//     setw —— 只有它是這樣
// 所以印表格時 setw 必須每一欄都重寫一次，而 setfill('0') 寫一次就會
// 一直污染後面所有輸出，用完必須手動還原（本檔每處都示範了還原）。
// 更穩健的做法是用 RAII 保存並還原整個流狀態（見本檔的 FormatGuard）。
//
// 【概念補充 Concept Deep Dive】
// (A) 引用（reference）在組合語言層面就是指標
//     int& 在多數實作中編譯成與 int* 相同的機器碼。差別全在編譯期的語意：
//     引用不可為 null、不可重新綁定、不需解參考語法。
//     所以「引用比指標快」是錯的（兩者相同），
//     「引用比指標安全」才是對的（因為它排除了 null 與重綁定兩類錯誤）。
//
// (B) std::endl 的成本是真的，但別過度解讀
//     endl = '\n' + flush。flush 會觸發一次實際的寫出系統呼叫，
//     在大量輸出的迴圈裡確實會顯著變慢。
//     但若輸出對象是終端機且該流是行緩衝的，差異就小得多。
//     實務準則：**預設用 '\n'**，只有在真的需要立即看到輸出時
//     （例如程式可能崩潰前的除錯訊息）才用 endl 或 std::flush。
//
// (C) cerr 無緩衝、clog 有緩衝，兩者都導向 stderr
//     這是常被記錯的一組。cerr 預設 unitbuf（每次輸出後自動 flush），
//     所以程式崩潰時 cerr 的訊息不會遺失；clog 沒有這個設定，
//     適合量大但不急的日誌。本檔的 log 實務範例正是用 clog。
//
// (D) 匿名 namespace 取代 static 的理由
//     兩者都達成「僅本翻譯單元可見」（internal linkage），但匿名 namespace
//     更通用：static 不能用在型別（class/struct）上，匿名 namespace 可以。
//     C++ 標準也曾一度不建議在命名空間範圍使用 static（後來取消了該不建議），
//     現代慣例仍偏好匿名 namespace。
//
// (E) __cplusplus 在 MSVC 上的陷阱
//     MSVC 預設永遠回報 199711L（C++98），不論你用 /std:c++17 還是 c++20，
//     必須加上 /Zc:__cplusplus 才會回報正確值。
//     這是跨平台程式碼用 __cplusplus 做條件編譯時的經典地雷。
//     GCC/Clang 沒有這個問題（本檔在 GCC 下實測回報 201703L）。
//
// 【注意事項 Pay Attention】
// 1. 零開銷是設計「目標」；RTTI、exception、iostream 仍有實際成本，別當成絕對保證。
// 2. new/delete 在本檔只為對照 C 而示範；現代程式碼應改用 vector 與智慧指標。
// 3. 回傳型別不參與重載決議 —— 只差回傳型別的兩個多載是編譯錯誤。
// 4. 標頭檔（.h）中絕對不要寫 using namespace，會污染所有 include 它的檔案。
// 5. setw 是唯一的 non-sticky 操縱器；setfill/setprecision 等設定後會持續生效，用完要還原。
// 6. 預設參數只能從右側開始給；且應寫在宣告處（通常是標頭檔），不要在定義處重複。
// 7. __cplusplus 在 MSVC 需搭配 /Zc:__cplusplus 才可靠。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】第 1～5 課（語言定位、C/C++ 差異、namespace、iostream）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是「零開銷原則（zero-overhead principle）」？請舉一個反例。
//     答：兩句話 ——「沒用到的功能不付代價」「用到的功能，手寫也不會更快」。
//         例：不寫 virtual 就沒有 vtable；樣板 getMax<int> 編譯後等同手寫版本。
//         反例（重點）：RTTI 與 exception 並不完全零開銷 —— 前者需要型別資訊表，
//         後者需要 unwind table，即使從未拋出也會增加二進位大小。
//         這正是嵌入式專案常用 -fno-rtti -fno-exceptions 的原因。
//     追問：那 std::vector 相對於原生陣列有額外開銷嗎？
//         → 存取元素沒有（operator[] 會被 inline 成同樣的位址計算）；
//           但它多存了 size/capacity 三個指標，且動態配置本身有成本。
//
// 🔥 Q2. C 沒有函數重載，C++ 怎麼做到的？為什麼回傳型別不能用來重載？
//     答：靠 name mangling —— 編譯器把參數型別編碼進符號名稱，
//         int square(int) 與 double square(double) 在連結器眼中是不同符號。
//         回傳型別不參與，是因為呼叫端可以忽略回傳值（如單獨一行 `f();`），
//         此時編譯器無從判斷該選哪個多載，規則上直接禁止。
//     追問：那要怎麼讓 C 程式碼呼叫 C++ 函式？
//         → 用 extern "C" 包起來關閉 mangling，代價是該函式不能重載。
//
// 🔥 Q3. `std::cout << myObj` 這行，編譯器是怎麼找到那個 operator<< 的？
//     答：靠 ADL（Argument-Dependent Lookup）。operator<< 是自由函式不是成員，
//         編譯器除了一般作用域，還會到「所有實參型別所在的 namespace」查找。
//         因為 cout 屬於 std，std::operator<< 才被找到。
//     追問：這對自訂型別有什麼實務啟示？
//         → 自訂型別的 operator<< 應定義在該型別所屬的 namespace 內，
//           讓 ADL 找得到；丟到全域會讓使用者被迫寫完全限定名稱。
//
// ⚠️ 陷阱 1. 為什麼這段印表格的程式碼，只有第一欄對齊了？
//         cout << setw(10) << "商品" << "單價" << "數量";
//     答：因為 setw 是唯一的 **non-sticky** 操縱器，只作用於緊接其後的
//         那一個輸出項。「單價」「數量」沒有各自的 setw，就不會有欄寬。
//         正確寫法是每一欄都重寫 setw(10)。
//     為什麼會錯：多數人看到 setprecision、fixed、setfill 設定一次就持續生效，
//         於是歸納出「操縱器都是設定狀態」的模型，把 setw 也套進去。
//         但 setw 設定的是 stream 的 width 欄位，而該欄位在**每次輸出後
//         會被重設為 0** —— 這是標準明確規定的行為，setw 是特例而非通例。
//
// ⚠️ 陷阱 2. 先 cin >> age 再 getline(cin, name)，為什麼 name 讀到空字串？
//     答：cin >> age 讀走數字後，**把換行字元留在緩衝區**（>> 遇空白即停，
//         不消耗該空白）。接著 getline 讀到的第一個字元就是那個 '\n'，
//         於是立刻回傳一個空字串。
//         解法：cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
//     為什麼會錯：腦中把 >> 想成「讀完這一行」，實際上它是
//         「跳過前導空白 → 讀到下一個空白為止 → 停住，不消耗結束的那個空白」。
//         >> 與 getline 對「行尾」的處理方式根本不同，混用就必然要手動清緩衝。
//
// 【LeetCode 實戰範例】—— 從缺（刻意不加）
//     第 1～5 課的內容是「語言定位、工具鏈、名稱管理、I/O 格式化」，
//     屬於環境與語法層面，不對應任何演算法或資料結構題型。
//     LeetCode 不考 namespace、不考編譯器巨集、也不考 iomanip 格式化
//     （其判題只比對回傳值，根本不看你的 stdout）。
//     本檔改以「log 分級輸出 + 對齊報表」這個真實情境示範 iostream 與 namespace
//     的實際用法。硬湊一題不相關的題目，比沒有更糟。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <iomanip>      // setw, setprecision, fixed, hex 等格式控制
#include <string>
#include <vector>
#include <algorithm>    // std::sort
#include <limits>       // numeric_limits
#include <cmath>        // std::sqrt

// ================================================================
// 第 1 課：C++ 的歷史與設計哲學
// ================================================================
//
// 【歷史時間線】
//   1972  Dennis Ritchie 創造 C 語言
//   1979  Bjarne Stroustrup 開發「C with Classes」
//   1983  更名為 C++（Rick Mascitti 建議，++ 來自遞增運算子）
//   1998  C++98 — 第一個 ISO 標準
//   2011  C++11 — 現代 C++ 的起點
//   2014/17/20/23 持續演進
//
// 【設計動機】
//   Stroustrup 想結合 Simula 的組織能力（class/物件）與 C 的執行效率
//
// 【四大設計原則】
//   1. 零開銷原則 — 不使用的功能不產生開銷
//   2. 直接映射硬體 — 保留指標、位元操作等底層能力
//   3. C 的相容性 — 大部分 C 程式碼也是合法 C++
//   4. 高階抽象可選 — class、template 等功能不強制使用
//
// 【四種範式】
//   程序式 / 物件導向 / 泛型 / 函數式
// ================================================================

// --- 範式一：程序式（純函數，零 OOP 開銷）---
int proceduralAdd(int a, int b) { return a + b; }

// --- 範式二：物件導向（class + 封裝）---
class Calculator {
    int lastResult_ = 0;          // 私有成員
public:
    int multiply(int a, int b) { lastResult_ = a * b; return lastResult_; }
    int getLastResult() const { return lastResult_; }   // const 保證不修改
};

// --- 範式三：泛型（template）---
template<typename T>
T getMax(T a, T b) { return (a > b) ? a : b; }

// --- 零開銷 & 硬體映射示範 ---
void demo_lesson1() {
    std::cout << "=== 第 1 課：C++ 歷史與設計哲學 ===\n\n";

    // 程序式
    std::cout << "[程序式] proceduralAdd(3,5) = " << proceduralAdd(3, 5) << "\n";

    // 物件導向
    Calculator calc;
    std::cout << "[OOP] calc.multiply(4,6) = " << calc.multiply(4, 6) << "\n";

    // 泛型 — 同一函數自動推導 int / double / char
    std::cout << "[泛型] getMax(10,20)     = " << getMax(10, 20) << "\n";
    std::cout << "[泛型] getMax(3.14,2.71) = " << getMax(3.14, 2.71) << "\n";
    std::cout << "[泛型] getMax('a','z')   = " << getMax('a', 'z') << "\n";

    // 函數式 — lambda + std::sort
    std::vector<int> nums = {5, 2, 8, 1, 9, 3};
    std::sort(nums.begin(), nums.end(), [](int a, int b) { return a < b; });
    std::cout << "[函數式] 排序: ";
    for (int n : nums) std::cout << n << " ";
    std::cout << "\n";

    // 硬體映射 — 指標 & 位元操作
    int val = 42;
    int* ptr = &val;            // 取址
    *ptr = 100;                 // 透過指標修改
    unsigned char flags = 0b00001111;
    flags <<= 2;                // 位元左移 → 0b00111100 = 60
    std::cout << "[硬體] *ptr=" << val << ", flags<<2=" << (int)flags << "\n\n";
}

// ================================================================
// 第 2 課：C 與 C++ 的關鍵差異
// ================================================================
//
// 【九大差異速查表】
//   項目           C                    C++
//   ───────────── ──────────────────── ────────────────────
//   變數宣告       區塊開頭(C89)         任意位置
//   I/O            printf/scanf          cout/cin（型別安全）
//   記憶體         malloc/free           new/delete（調建構/解構）
//   bool           需 stdbool.h          內建 bool/true/false
//   函數重載       不支援                支援（同名不同參數）
//   預設參數       不支援                支援（從右側開始）
//   引用           只有指標              支援 int& ref（更安全）
//   struct         只有資料              可含成員函數
//   字串           char[] + strcpy       std::string（+ 連接）
// ================================================================

// --- 差異五：函數重載 ---
int square(int x)    { return x * x; }       // int 版本
double square(double x) { return x * x; }    // double 版本
int add(int a, int b) { return a + b; }
int add(int a, int b, int c) { return a + b + c; }

// --- 差異六：預設參數（從右側開始）---
void greet(const std::string& name,
           const std::string& greeting = "Hello",
           const std::string& punct = "!") {
    std::cout << "  " << greeting << ", " << name << punct << "\n";
}

// --- 差異七：引用（Reference）---
void swapByRef(int& a, int& b) { int t = a; a = b; b = t; }  // 不需 * &

// --- 差異八：struct 可含成員函數 ---
struct Point {
    int x, y;
    void print() const { std::cout << "Point(" << x << "," << y << ")"; }
};

void demo_lesson2() {
    std::cout << "=== 第 2 課：C 與 C++ 關鍵差異 ===\n\n";

    // 變數宣告 — 在使用點附近宣告並初始化
    int sum = 0;
    for (int i = 0; i < 5; i++) sum += i;  // i 作用域僅在 for 內
    std::cout << "[變數宣告] sum(0..4) = " << sum << "\n";

    // I/O — 自動型別識別，不需 %d %f
    std::cout << "[cout] int=" << 42 << " double=" << 3.14
              << " bool=" << std::boolalpha << true << std::noboolalpha << "\n";

    // new/delete（vs malloc/free）
    int* p = new int(100);   // 配置+初始化，不需 sizeof/轉型
    std::cout << "[new] *p = " << *p << "\n";
    delete p; p = nullptr;   // 釋放後設 nullptr

    int* arr = new int[3]{10, 20, 30};
    std::cout << "[new[]] arr: " << arr[0] << " " << arr[1] << " " << arr[2] << "\n";
    delete[] arr;             // 陣列必須 delete[]！

    // 函數重載 — 編譯器依參數選版本
    std::cout << "[重載] square(5)=" << square(5)
              << " square(3.14)=" << square(3.14) << "\n";
    std::cout << "[重載] add(1,2)=" << add(1, 2)
              << " add(1,2,3)=" << add(1, 2, 3) << "\n";

    // 預設參數
    greet("Alice");                  // Hello, Alice!
    greet("Bob", "Hi");              // Hi, Bob!
    greet("Charlie", "Hey", "~");    // Hey, Charlie~

    // 引用 — 語法簡潔，不需 & 取址
    int x = 10, y = 20;
    swapByRef(x, y);
    std::cout << "[引用] swap後 x=" << x << " y=" << y << "\n";

    int orig = 42;
    int& alias = orig;  // 別名，修改 alias 即修改 orig
    alias = 99;
    std::cout << "[引用] alias=99 → orig=" << orig << "\n";

    // struct 有成員函數
    Point pt{3, 7};     // 不需 struct 關鍵字（C 需要）
    std::cout << "[struct] "; pt.print(); std::cout << "\n";

    // std::string
    std::string s = "Hello" + std::string(", ") + "World!";
    std::cout << "[string] " << s << " len=" << s.length()
              << " sub=" << s.substr(0, 5) << "\n\n";
}

// ================================================================
// 第 3 課：C++ 編譯環境設置
// ================================================================
//
// 【三大編譯器】g++（GCC）、clang++（LLVM）、cl（MSVC）
//
// 【開發編譯】g++ -std=c++17 -Wall -Wextra -g -O0 file.cpp -o out
// 【發布編譯】g++ -std=c++17 -Wall -Wextra -O2 file.cpp -o out
// 【MSVC】    cl /std:c++17 /utf-8 /EHsc /W4 /Zi file.cpp
//
// 【編譯器偵測巨集】
//   __clang__  → Clang（先判斷，因 Clang 也定義 __GNUC__）
//   __GNUC__   → GCC
//   _MSC_VER   → MSVC（值為版本號，如 1940）
//
// 【C++ 標準偵測】__cplusplus
//   199711L=C++98  201103L=C++11  201402L=C++14
//   201703L=C++17  202002L=C++20  202302L=C++23
//   MSVC 需加 /Zc:__cplusplus 才正確
//
// 【OS 偵測】_WIN32/_WIN64  __linux__  __APPLE__
//
// 【多檔案編譯】
//   g++ main.cpp util.cpp -o program
//   分步：g++ -c main.cpp → g++ -c util.cpp → g++ main.o util.o -o program
//   標頭檔保護：#pragma once 或 #ifndef HEADER_H
//
// 【常見錯誤】
//   'g++' not recognized → PATH 未設定
//   undefined reference  → 沒有連結對應 .cpp
//   'auto' error         → 需加 -std=c++11 或更高
// ================================================================

void demo_lesson3() {
    std::cout << "=== 第 3 課：編譯環境設置 ===\n\n";

    // 編譯器偵測
    std::cout << "[編譯器] ";
#if defined(__clang__)
    std::cout << "Clang " << __clang_major__ << "." << __clang_minor__;
#elif defined(__GNUC__)
    std::cout << "GCC " << __GNUC__ << "." << __GNUC_MINOR__;
#elif defined(_MSC_VER)
    std::cout << "MSVC " << _MSC_VER;
#else
    std::cout << "Unknown";
#endif
    std::cout << "\n";

    // C++ 標準偵測
    std::cout << "[標準] __cplusplus = " << __cplusplus << " → ";
#if __cplusplus >= 202002L
    std::cout << "C++20+";
#elif __cplusplus >= 201703L
    std::cout << "C++17";
#elif __cplusplus >= 201402L
    std::cout << "C++14";
#elif __cplusplus >= 201103L
    std::cout << "C++11";
#else
    std::cout << "C++98/03";
#endif
    std::cout << "\n";

    // OS 偵測
    std::cout << "[OS] ";
#if defined(_WIN64)
    std::cout << "Windows 64-bit";
#elif defined(_WIN32)
    std::cout << "Windows 32-bit";
#elif defined(__linux__)
    std::cout << "Linux";
#elif defined(__APPLE__)
    std::cout << "macOS";
#else
    std::cout << "Unknown";
#endif
    std::cout << "\n";

    // 現代 C++ 功能驗證
    auto x = 42;                                    // auto（C++11）
    std::vector<std::string> v = {"A", "B", "C"};   // 初始化列表（C++11）
    auto mul = [](int a, int b) { return a * b; };   // lambda（C++11）
    std::cout << "[C++11] auto=" << x << " lambda=" << mul(6, 7) << " vec=";
    for (const auto& s : v) std::cout << s << " ";   // 範圍 for
    std::cout << "\n\n";
}

// ================================================================
// 第 4 課：命名空間（namespace）基礎
// ================================================================
//
// 【目的】解決名稱衝突：不同函式庫可能定義相同名稱的函數
//
// 【語法】namespace Name { 變數/函式/類別... }
//
// 【三種存取方式】
//   1. 完全限定名稱：math::PI          ← 最安全，推薦
//   2. using 宣告：  using math::PI;   ← 引入特定成員
//   3. using 指令：  using namespace math; ← 引入全部（慎用！）
//
// 【重要規則】
//   - 標頭檔(.h)中 絕對禁止 using namespace！
//   - 大型專案優先用完全限定名稱
//   - 在函式/區塊內用 using 宣告可局部簡化
//
// 【命名空間別名】namespace mt = math_tools;
// 【匿名命名空間】namespace { } — 只在本檔案內可見（取代 static）
// 【巢狀命名空間】C++17: namespace a::b::c { }
// 【擴展】同名 namespace 可分散在多處，編譯器自動合併
// ================================================================

namespace math_ns {
    const double PI = 3.14159265;
    double circleArea(double r) { return PI * r * r; }
}

namespace str_ns {
    std::string toUpper(const std::string& s) {
        std::string r = s;
        for (char& c : r) if (c >= 'a' && c <= 'z') c -= 32;
        return r;
    }
}

// 巢狀命名空間（C++17 寫法）
namespace company::graphics {
    void draw() { std::cout << "  [Graphics] draw\n"; }
}

// 匿名命名空間 — 只在本檔案可見
namespace {
    int fileLocalCounter = 0;
    void countCall() { ++fileLocalCounter; }
}

void demo_lesson4() {
    std::cout << "=== 第 4 課：命名空間基礎 ===\n\n";

    // 方式一：完全限定名稱（最安全）
    std::cout << "[完全限定] math_ns::PI = " << math_ns::PI << "\n";
    std::cout << "[完全限定] circleArea(5) = " << math_ns::circleArea(5) << "\n";

    // 方式二：using 宣告（引入特定成員）
    {
        using str_ns::toUpper;
        std::cout << "[using宣告] toUpper(\"hello\") = " << toUpper("hello") << "\n";
    }

    // 方式三：using 指令（僅在區塊內）
    {
        using namespace math_ns;
        std::cout << "[using指令] PI = " << PI << "\n";
    }

    // 別名
    {
        namespace mt = math_ns;
        std::cout << "[別名] mt::PI = " << mt::PI << "\n";
    }

    // 巢狀命名空間
    company::graphics::draw();

    // 匿名命名空間
    countCall(); countCall(); countCall();
    std::cout << "[匿名ns] fileLocalCounter = " << fileLocalCounter << "\n\n";
}

// ================================================================
// 第 5 課：輸入輸出流（iostream）入門
// ================================================================
//
// 【四個標準流】
//   cout — 標準輸出（有緩衝）    對應 C 的 stdout
//   cin  — 標準輸入              對應 C 的 stdin
//   cerr — 標準錯誤（無緩衝）    對應 C 的 stderr
//   clog — 標準日誌（有緩衝）    對應 C 的 stderr
//
// 【<< 插入運算子】
//   - 自動識別型別，不需 %d %f
//   - 可鏈式串接：cout << a << b << c
//
// 【換行比較】
//   std::endl → '\n' + flush（慢）
//   '\n'      → 僅換行（快，推薦大量輸出時使用）
//   std::flush→ 只 flush 不換行
//
// 【>> 提取運算子】
//   - 自動跳過前導空白
//   - 遇到空白停止 → "Hello World" 只讀到 "Hello"
//   - 可連續串接：cin >> a >> b >> c
//
// 【getline】
//   std::getline(cin, str) → 讀整行含空格
//
// 【cin >> 與 getline 混用陷阱】
//   cin >> x 後緩衝區殘留 '\n'，getline 會讀到空字串
//   解法：cin.ignore(numeric_limits<streamsize>::max(), '\n');
//
// 【流狀態】
//   good() — 正常     eof()  — 到結尾
//   fail() — 失敗(可恢復：clear() + ignore())
//   bad()  — 嚴重錯誤(不可恢復)
//
// 【iomanip 格式控制】
//   setprecision(n) — 精度（sticky）
//   fixed / scientific / defaultfloat — 浮點格式
//   setw(n) — 欄寬（non-sticky，只影響下一個輸出）
//   setfill(c) — 填充字元    left / right — 對齊
//   hex / oct / dec — 進位    showbase — 顯示前綴
//   boolalpha — bool 輸出 true/false
// ================================================================

void demo_lesson5() {
    std::cout << "=== 第 5 課：iostream 入門 ===\n\n";

    // << 自動型別識別 + 鏈式輸出
    std::cout << "[<<] int=" << 42 << " double=" << 3.14
              << " char=" << 'A' << " str=" << "Hi" << "\n";

    // 換行方式
    std::cout << "[endl] " << std::endl;      // flush（慢）
    std::cout << "[\\n] \n";                   // 僅換行（快）

    // cin >> 與 getline 說明（不做互動，僅展示規則）
    std::cout << "[cin >>] 遇空白停止，'Hello World' 只讀 'Hello'\n";
    std::cout << "[getline] 讀整行含空格\n";
    std::cout << "[陷阱] cin>>x 後需 cin.ignore() 再 getline\n\n";

    // --- iomanip 格式化 ---
    double pi = 3.14159265358979;

    // 精度
    std::cout << "[精度] 預設: " << pi << "\n";
    std::cout << "  setprecision(3): " << std::setprecision(3) << pi << "\n";
    std::cout << "  fixed+prec(2):   " << std::fixed << std::setprecision(2) << pi << "\n";
    std::cout << "  scientific:      " << std::scientific << std::setprecision(3) << pi << "\n";
    std::cout << std::defaultfloat << std::setprecision(6); // 恢復

    // 欄寬 & 對齊（setw 只影響下一個輸出！）
    std::cout << "  right setw(10): [" << std::right << std::setw(10) << 42 << "]\n";
    std::cout << "  left  setw(10): [" << std::left  << std::setw(10) << 42 << "]\n";
    std::cout << "  補零  setw(6):  [" << std::right << std::setfill('0')
              << std::setw(6) << 42 << "]\n";
    std::cout << std::setfill(' '); // 恢復

    // 進位
    int num = 255;
    std::cout << "  dec=" << std::dec << num
              << " hex=" << std::hex << num
              << " oct=" << std::oct << num << "\n";
    std::cout << std::dec; // 恢復

    // boolalpha
    std::cout << "  bool: " << std::boolalpha << true << "/" << false
              << std::noboolalpha << "\n";

    // 表格示範
    std::cout << "\n  " << std::left
              << std::setw(10) << "商品" << std::setw(8) << "單價"
              << std::setw(6) << "數量" << std::setw(10) << "小計" << "\n";
    std::cout << "  " << std::string(34, '-') << "\n";
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "  " << std::setw(10) << "蘋果" << std::setw(8) << 35.0
              << std::setw(6) << 3 << std::setw(10) << 105.0 << "\n";
    std::cout << "  " << std::setw(10) << "香蕉" << std::setw(8) << 25.5
              << std::setw(6) << 5 << std::setw(10) << 127.5 << "\n";
    std::cout << std::defaultfloat << std::right; // 恢復

    // cerr & clog
    std::cout << "\n[cerr] 無緩衝，立即輸出（用於錯誤訊息）\n";
    std::cerr << "  → 這是 cerr 輸出\n";
    std::cout << "[clog] 有緩衝（用於日誌）\n";
    std::clog << "  → 這是 clog 輸出\n\n";
}

// ================================================================
// 【日常實務範例】結構化日誌模組：namespace + iostream + iomanip 的綜合應用
// ================================================================
//   情境：所有服務都需要一個最小的日誌工具，實務上的硬性要求是
//     (a) 分級（DEBUG/INFO/WARN/ERROR），可設定門檻過濾
//     (b) 對齊輸出，方便人眼掃描與 grep
//     (c) ERROR 走 cerr（無緩衝，程式崩潰也不遺失）；
//         其餘走 clog（有緩衝，量大時效能較好）
//     (d) 格式化不可污染呼叫端的 cout 狀態 —— 這是最容易被忽略的一點
//
//   本範例把前五課的東西全用上了：
//     第 1 課 多範式  → enum class + 函式 + RAII 類別混用
//     第 2 課 C++ 特性→ 預設參數、函數重載、引用、std::string
//     第 4 課 namespace→ 把工具收進 applog，避免與其他模組的 log 撞名
//     第 5 課 iostream → cerr/clog 分流、setw 對齊、sticky 狀態還原
// ================================================================

namespace applog {

enum class Level { Debug = 0, Info = 1, Warn = 2, Error = 3 };

// 目前的輸出門檻（低於此級別的訊息會被丟棄）
// 放在匿名 namespace 內：僅本翻譯單元可見，外部無法直接改
namespace {
    Level g_threshold = Level::Info;
}

void setThreshold(Level lv) { g_threshold = lv; }

const char* levelName(Level lv) {
    switch (lv) {
        case Level::Debug: return "DEBUG";
        case Level::Info:  return "INFO";
        case Level::Warn:  return "WARN";
        case Level::Error: return "ERROR";
    }
    return "UNKNOWN";
}

// ── RAII 格式守衛：離開作用域時自動還原流的格式狀態 ──────────────
// 這正是「sticky 操縱器會污染後續輸出」的正解：
// 與其每次手動還原（漏一次就出錯），不如讓解構函式保證還原。
// 這也預告了第 19 課 RAII 的核心思想。
class FormatGuard {
    std::ostream&      m_os;
    std::ios::fmtflags m_flags;
    char               m_fill;
    std::streamsize    m_prec;
public:
    explicit FormatGuard(std::ostream& os)
        : m_os(os), m_flags(os.flags()), m_fill(os.fill()), m_prec(os.precision()) {}
    ~FormatGuard() {                       // 保證還原，即使中途拋出例外
        m_os.flags(m_flags);
        m_os.fill(m_fill);
        m_os.precision(m_prec);
    }
    FormatGuard(const FormatGuard&) = delete;             // 守衛不該被複製
    FormatGuard& operator=(const FormatGuard&) = delete;
};

// 主要的 log 函式；第二個參數用預設值（第 2 課：預設參數從右側開始）
void write(const std::string& msg, Level lv = Level::Info) {
    if (static_cast<int>(lv) < static_cast<int>(g_threshold)) {
        return;                                    // 未達門檻，直接丟棄
    }
    // ERROR 走 cerr（無緩衝，崩潰不遺失）；其餘走 clog（有緩衝）
    std::ostream& os = (lv == Level::Error) ? std::cerr : std::clog;

    FormatGuard guard(os);                         // 離開時自動還原格式
    os << "  [" << std::left << std::setw(5) << levelName(lv) << "] "
       << msg << "\n";
}

// 函數重載（第 2 課）：多一個「數值欄位」的版本，方便輸出量測值
void write(const std::string& msg, double value, Level lv = Level::Info) {
    if (static_cast<int>(lv) < static_cast<int>(g_threshold)) return;
    std::ostream& os = (lv == Level::Error) ? std::cerr : std::clog;

    FormatGuard guard(os);
    os << "  [" << std::left << std::setw(5) << levelName(lv) << "] "
       << std::setw(28) << msg
       << std::right << std::fixed << std::setprecision(2) << value << "\n";
}

} // namespace applog

void demo_practical_log() {
    std::cout << "=== 日常實務：結構化日誌（namespace + iostream + RAII）===\n\n";

    // 要驗證 FormatGuard，必須觀察「log 實際寫入的那個流」。
    // 本模組把非 ERROR 訊息寫到 clog，所以我們就把 clog 設成一個已知狀態，
    // 稍後再回頭檢查它有沒有被 log 內部的 fixed+setprecision(2) 污染。
    //（注意：cout 是另一個獨立的流，它的格式狀態本來就不會被 clog 影響。）
    std::clog << std::defaultfloat << std::setprecision(8);
    std::clog << "  [基準] 呼叫 log 之前，clog： 3.14159265 → " << 3.14159265 << "\n\n";

    applog::setThreshold(applog::Level::Debug);   // 全部都印
    applog::write("服務啟動，載入設定檔", applog::Level::Info);
    applog::write("快取未命中，回源查詢", applog::Level::Debug);
    applog::write("回應時間偏高（毫秒）", 1234.5678, applog::Level::Warn);
    applog::write("資料庫連線失敗，將重試", applog::Level::Error);

    std::cout << "\n  ── 把門檻提高到 WARN，DEBUG/INFO 會被過濾掉 ──\n";
    applog::setThreshold(applog::Level::Warn);
    applog::write("這行是 INFO，不會出現", applog::Level::Info);
    applog::write("這行是 DEBUG，不會出現", applog::Level::Debug);
    applog::write("磁碟使用率（百分比）", 91.5, applog::Level::Warn);

    // 證明 FormatGuard 有效：log 內部把 clog 設成 fixed + setprecision(2)，
    // 但離開 write() 後 clog 的精度已被解構函式還原成我們設定的 8。
    std::clog << "\n  [驗證] log 內部用過 fixed+setprecision(2)，離開後 clog 已還原：\n";
    std::clog << "         3.14159265 → " << 3.14159265 << "\n";
    std::clog << "         （與上方 [基準] 相同 → FormatGuard 的解構函式確實還原了狀態。\n";
    std::clog << "           實測把 ~FormatGuard 的還原拿掉，這行會變成 fixed 的 3.14）\n";

    std::clog << std::defaultfloat << std::setprecision(6);   // 還原給後續使用
    std::cout << "\n";
}

// ================================================================
// 主程式
// ================================================================
int main() {
    std::cout << "================================================================\n";
    std::cout << "  C++ 面向對象 第 1～5 課 總複習\n";
    std::cout << "================================================================\n\n";

    demo_lesson1();   // 歷史與設計哲學
    demo_lesson2();   // C vs C++ 差異
    demo_lesson3();   // 編譯環境設置
    demo_lesson4();   // 命名空間
    demo_lesson5();   // iostream

    demo_practical_log();   // 日常實務：結構化日誌

    std::cout << "================================================================\n";
    std::cout << "  第 1～5 課總複習完成！\n";
    std::cout << "================================================================\n";

    return 0;
}

/*
 * ================================================================
 * 五課重點速查表
 * ================================================================
 *
 * 課次   主題                    核心關鍵字
 * ─────  ──────────────────────  ──────────────────────────────
 * 第1課  歷史與設計哲學          零開銷、硬體映射、C相容、多範式
 * 第2課  C vs C++ 差異           new/delete、引用、重載、預設參數、string
 * 第3課  編譯環境設置            g++ -std=c++17 -Wall、__cplusplus、_MSC_VER
 * 第4課  命名空間                namespace、using、別名、匿名ns、巢狀ns
 * 第5課  iostream                cout/cin、<</>>/getline、iomanip、cerr/clog
 *
 * ================================================================
 */

// 編譯: g++ -std=c++17 -Wall -Wextra summary1-5.cpp -o summary1-5

// 註：本檔輸出同時包含 cout、cerr、clog 三個流。下方預期輸出是把三者
//     合併擷取（2>&1）的結果；若只看終端機的 stdout，cerr/clog 那幾行
//     會出現在不同位置或被重新導向到別處。

// === 預期輸出 ===
// ================================================================
//   C++ 面向對象 第 1～5 課 總複習
// ================================================================
//
// === 第 1 課：C++ 歷史與設計哲學 ===
//
// [程序式] proceduralAdd(3,5) = 8
// [OOP] calc.multiply(4,6) = 24
// [泛型] getMax(10,20)     = 20
// [泛型] getMax(3.14,2.71) = 3.14
// [泛型] getMax('a','z')   = z
// [函數式] 排序: 1 2 3 5 8 9 
// [硬體] *ptr=100, flags<<2=60
//
// === 第 2 課：C 與 C++ 關鍵差異 ===
//
// [變數宣告] sum(0..4) = 10
// [cout] int=42 double=3.14 bool=true
// [new] *p = 100
// [new[]] arr: 10 20 30
// [重載] square(5)=25 square(3.14)=9.8596
// [重載] add(1,2)=3 add(1,2,3)=6
//   Hello, Alice!
//   Hi, Bob!
//   Hey, Charlie~
// [引用] swap後 x=20 y=10
// [引用] alias=99 → orig=99
// [struct] Point(3,7)
// [string] Hello, World! len=13 sub=Hello
//
// === 第 3 課：編譯環境設置 ===
//
// [編譯器] GCC 15.2
// [標準] __cplusplus = 201703 → C++17
// [OS] Linux
// [C++11] auto=42 lambda=42 vec=A B C 
//
// === 第 4 課：命名空間基礎 ===
//
// [完全限定] math_ns::PI = 3.14159
// [完全限定] circleArea(5) = 78.5398
// [using宣告] toUpper("hello") = HELLO
// [using指令] PI = 3.14159
// [別名] mt::PI = 3.14159
//   [Graphics] draw
// [匿名ns] fileLocalCounter = 3
//
// === 第 5 課：iostream 入門 ===
//
// [<<] int=42 double=3.14 char=A str=Hi
// [endl] 
// [\n] 
// [cin >>] 遇空白停止，'Hello World' 只讀 'Hello'
// [getline] 讀整行含空格
// [陷阱] cin>>x 後需 cin.ignore() 再 getline
//
// [精度] 預設: 3.14159
//   setprecision(3): 3.14
//   fixed+prec(2):   3.14
//   scientific:      3.142e+00
//   right setw(10): [        42]
//   left  setw(10): [42        ]
//   補零  setw(6):  [000042]
//   dec=255 hex=ff oct=377
//   bool: true/false
//
//   商品    單價  數量小計    
//   ----------------------------------
//   蘋果    35.0    3     105.0     
//   香蕉    25.5    5     127.5     
//
// [cerr] 無緩衝，立即輸出（用於錯誤訊息）
//   → 這是 cerr 輸出
// [clog] 有緩衝（用於日誌）
//   → 這是 clog 輸出
//
// === 日常實務：結構化日誌（namespace + iostream + RAII）===
//
//   [基準] 呼叫 log 之前，clog： 3.14159265 → 3.1415927
//
//   [INFO ] 服務啟動，載入設定檔
//   [DEBUG] 快取未命中，回源查詢
//   [WARN ] 回應時間偏高（毫秒）1234.57
//   [ERROR] 資料庫連線失敗，將重試
//
//   ── 把門檻提高到 WARN，DEBUG/INFO 會被過濾掉 ──
//   [WARN ] 磁碟使用率（百分比）91.50
//
//   [驗證] log 內部用過 fixed+setprecision(2)，離開後 clog 已還原：
//          3.14159265 → 3.1415927
//          （與上方 [基準] 相同 → FormatGuard 的解構函式確實還原了狀態。
//            實測把 ~FormatGuard 的還原拿掉，這行會變成 fixed 的 3.14）
//
// ================================================================
//   第 1～5 課總複習完成！
// ================================================================
