// =============================================================================
//  第 2 課：C 與 C++ 的關鍵差異 9  —  函式多載與 name mangling
// =============================================================================
//
// 【主題資訊 Information】
//   C   ：不支援函式多載。同一個名稱只能有一個函式，
//          否則連結階段出現 multiple definition
//   C++ ：支援多載。同名函式只要「參數列」不同即可（回傳型別不算）
//   機制：name mangling（名稱修飾）——編譯器把參數型別編進符號名稱
//   標準版本：函式多載自 C++98；C11 有 _Generic 可做出類似效果但機制完全不同
//   工具：nm -C（demangle）、c++filt 可還原被修飾的符號名
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 C 不能多載——問題出在連結器】
//   關鍵在於「編譯」與「連結」是兩個分離的階段。
//   C 的編譯器把函式 add 編譯成目標檔裡一個叫做 add 的符號，就這樣，
//   沒有任何型別資訊。連結器看到的世界只有一張「名稱 → 位址」的表。
//   於是若有兩個都叫 add 的函式，連結器根本無從選擇——它連兩者長什麼樣
//   都不知道。這不是 C 的編譯器懶惰，而是它的目標檔格式裡就沒有地方
//   放型別資訊。
//   所以 C 的解法只能是「把型別編進名字裡，但由人類手動做」：
//       add_int, add_double, add_long ...
//   標準函式庫也是這樣：abs / labs / llabs / fabs / fabsf 全是同一件事的
//   不同型別版本。
//
// 【2. C++ 的解法：讓編譯器自動把型別編進名字】
//   C++ 沒有改變連結器，而是改變了「編譯器產生的符號名稱」。
//   本機 g++ 15.2 實測（nm 觀察目標檔）：
//       int    add(int, int)        →  符號名 _Z3addii
//       double add(double, double)  →  符號名 _Z3adddd
//   拆解 _Z3addii：
//       _Z    ：GCC/Itanium ABI 的修飾前綴
//       3add  ：名稱長度 3，名稱是 add
//       ii    ：兩個參數，型別都是 int（d 就是 double）
//   於是連結器看到的仍是「兩個不同名稱的符號」，它完全不需要理解多載，
//   問題在編譯階段就被解決了。這是很典型的 C++ 設計手法：
//   在既有的底層機制上加一層編譯期的轉換，而不去動底層。
//
// 【3. 什麼算「不同的多載」——回傳型別不算】
//   可以區分多載的是：參數個數、參數型別、參數順序、const/volatile 修飾、
//   以及成員函式的 const 與 ref-qualifier。
//   「不算」的是：
//       int    f(int);
//       double f(int);        // 錯誤：只有回傳型別不同，無法多載
//   為什麼？因為呼叫端可以完全忽略回傳值：
//       f(1);                 // 這樣寫的話，編譯器要選哪一個？
//   沒有任何依據可以決定，所以標準直接禁止。
//   同理，top-level const 也不算：void g(int) 與 void g(const int) 是同一個，
//   因為傳值本來就是複製，const 對呼叫端沒有意義。
//   但 void g(int&) 與 void g(const int&) 是不同的多載——這裡的 const
//   在參數型別的一部分，不是 top-level。
//
// 【4. extern "C"：讓 C++ 寫的函式能被 C 呼叫】
//   混合語言專案（幾乎所有大型系統都是）必須處理這件事：
//       extern "C" int add_c(int a, int b) { return a + b; }
//   加上 extern "C" 後，編譯器「不做」name mangling，符號名就是 add_c
//   （本機實測確認），C 的程式碼才連結得到。
//   代價很直接：既然不做修飾，就不能多載——extern "C" 的函式只能有一個。
//   標準的標頭檔寫法是：
//       #ifdef __cplusplus
//       extern "C" {
//       #endif
//       int add_c(int, int);
//       #ifdef __cplusplus
//       }
//       #endif
//   這樣同一份標頭檔，C 與 C++ 都能用。
//
// 【5. C11 的 _Generic：C 的「假多載」】
//   C11 提供了型別泛型選擇，可以做出看起來像多載的巨集：
//       #define add(x, y) _Generic((x),
//                             int:    add_int,
//                             double: add_double)(x, y)
//       （實際寫在 C 原始碼時，每行結尾要加反斜線做行接續；
//         此處為了避免 -Wcomment 警告而略去反斜線）
//   但它與 C++ 多載有本質差異：_Generic 是「編譯期依第一個運算元的型別
//   選一個分支」的表達式，它是巨集層次的展開，不參與連結、
//   不能處理隱式轉換、也無法用於函式指標。
//   C++ 的多載則是完整的語言機制，會參與多載解析、模板推導與 ADL。
//
// 【概念補充 Concept Deep Dive】
//   * name mangling 的規則不在 C++ 標準裡，是各家 ABI 各自規定的。
//     GCC/Clang 在 Linux 上遵循 Itanium C++ ABI，MSVC 用自己的一套
//     （長得像 ?add@@YAHHH@Z）。這正是「不同編譯器編出來的 C++ 目標檔
//     通常不能互相連結」的根本原因，也是 C 至今仍是跨語言／跨編譯器
//     介面事實標準的原因——C 的符號名是可預測的。
//   * 這也解釋了為什麼 C++ 函式庫升級容易出現 ABI 相容問題：
//     函式簽名改一個參數型別，符號名就變了，舊的執行檔就找不到它。
//     GCC 5 的 std::string ABI 變更（_GLIBCXX_USE_CXX11_ABI）就是著名案例。
//   * 多載解析的優先序大致是：完全相符 > 提升（char→int）>
//     標準轉換（int→double、指標→bool）> 使用者自訂轉換 > 可變參數。
//     同一階級內若有多個候選同分，就是 ambiguous，編譯錯誤。
//
// 【注意事項 Pay Attention】
//   1. 只有回傳型別不同「不能」構成多載。
//   2. top-level const 不影響多載（void f(int) 與 void f(const int) 相同），
//      但參數是參考或指標時的 const 會影響。
//   3. 預設參數與多載並用容易產生歧義：
//         void f(int a, int b = 0);
//         void f(int a);              // f(1) 兩者皆可 → ambiguous
//   4. extern "C" 的函式不能多載，因為它不做 name mangling。
//   5. 不同編譯器的 mangling 規則不同，C++ 目標檔通常無法跨編譯器連結。
//   6. 多載遇到隱式轉換時可能選到意料之外的版本
//      （例如字串字面值會優先選 bool 版本，見同一課第 8 檔），
//      設計多載時要留意型別之間的轉換關係。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】函式多載與 name mangling
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++ 怎麼做到函式多載？連結器不是只認名稱嗎？
//     答：靠 name mangling。編譯器把參數型別編進符號名稱，
//         所以 int add(int,int) 與 double add(double,double) 在目標檔裡
//         是兩個不同的符號（本機 g++ 實測為 _Z3addii 與 _Z3adddd）。
//         連結器看到的仍然是兩個不同名字，它完全不需要理解多載這回事——
//         問題在編譯階段就解決了。
//     追問：那 mangling 規則是標準規定的嗎？→ 不是。標準只規定語言行為，
//         mangling 屬於 ABI，GCC/Clang 用 Itanium ABI、MSVC 用另一套。
//         這就是 C++ 目標檔通常無法跨編譯器連結的原因。
//
// 🔥 Q2. 為什麼「只有回傳型別不同」不能構成多載？
//     答：因為呼叫端可以完全忽略回傳值。寫 f(1); 這一行時，
//         編譯器沒有任何依據能決定該選 int f(int) 還是 double f(int)。
//         多載解析只看引數，看不到「呼叫端打算怎麼用回傳值」，
//         所以標準直接禁止這種宣告。
//     追問：那 const 成員函式為什麼可以多載？→ 因為隱含的 this 指標型別
//         不同（T* 與 const T*），它實質上是參數的一部分，
//         呼叫端物件是不是 const 就決定了要選哪一個。
//
// ⚠️ 陷阱. 為什麼這個 C++ 函式在 C 裡連結不到？
//         // math.cpp
//         int add(int a, int b) { return a + b; }
//         // main.c
//         extern int add(int, int);
//         int main(void) { return add(1, 2); }
//     答：C++ 編譯器把 add 修飾成 _Z3addii，但 C 編譯器產生的呼叫
//         要找的是 add。連結器找不到符號，報 undefined reference to `add'。
//         解法是在 C++ 這側加上 extern "C"：
//             extern "C" int add(int a, int b) { return a + b; }
//         這樣符號名就是未修飾的 add，代價是它不能再被多載。
//     為什麼會錯：多數人以為「函式簽名一樣就連結得到」，
//         把符號名想成等於原始碼裡寫的名字。
//         但 C++ 目標檔裡的符號名早已被編譯器改寫過，
//         原始碼看起來一樣不代表符號名一樣。
//
// 註：函式多載與 name mangling 屬於語言／連結機制，
//     LeetCode 只測演算法正確性、不涉及編譯連結過程，
//     任何題號都套不上，故本檔僅附日常實務範例。
// ═══════════════════════════════════════════════════════════════════════════

#include <stdio.h>
#include <string>
#include <typeinfo>

// -----------------------------------------------------------------------------
// 原始示範：C 語言的做法——每個型別一個名字
// -----------------------------------------------------------------------------
// C 語言中，每個函數必須有不同的名稱
int add_int(int a, int b) {
    return a + b;
}

double add_double(double a, double b) {
    return a + b;
}

// -----------------------------------------------------------------------------
// C++ 的做法：同一個名字，讓編譯器依型別去分辨
// -----------------------------------------------------------------------------
int    add(int a, int b)          { return a + b; }
double add(double a, double b)    { return a + b; }
long   add(long a, long b)        { return a + b; }
// 參數個數不同也是合法的多載
int    add(int a, int b, int c)   { return a + b + c; }

// -----------------------------------------------------------------------------
// 【日常實務範例 1】對外提供 C ABI：extern "C" 的實際用法
//
// 情境：用 C++ 寫一個影像處理核心，但要讓 Python(ctypes)／Rust／C 專案呼叫。
//   跨語言介面幾乎一律走 C ABI，因為只有 C 的符號名是可預測的。
// 為何用到本主題：extern "C" 關閉 name mangling，
//   代價是這些函式不能多載——所以介面層只能回到 C 風格的命名。
//   這是「內部用 C++ 的全部能力，邊界收斂成 C」的標準架構。
// -----------------------------------------------------------------------------
extern "C" {

// 對外的 C ABI：名稱必須自己帶型別，因為不能多載
int    imgcore_sum_i32(const int* data, int n) {
    int s = 0;
    for (int i = 0; i < n; ++i) s += data[i];
    return s;
}

double imgcore_sum_f64(const double* data, int n) {
    double s = 0.0;
    for (int i = 0; i < n; ++i) s += data[i];
    return s;
}

}  // extern "C"

// 內部實作則可以盡情使用 C++ 的多載
namespace internal {
int    sum(const int* data, int n)    { return imgcore_sum_i32(data, n); }
double sum(const double* data, int n) { return imgcore_sum_f64(data, n); }
}  // namespace internal

// -----------------------------------------------------------------------------
// 【日常實務範例 2】多載一組 log 函式，讓呼叫端不必自己轉字串
//
// 情境：專案裡的 log 介面要能接受各種型別，呼叫端只寫 logValue(x) 就好。
// 為何用到本主題：這是多載最有價值的日常用途——
//   把「型別 → 該怎麼格式化」的判斷從呼叫端移到函式庫端。
//   在 C 裡只能寫 log_int / log_double / log_str 讓呼叫端自己選。
// -----------------------------------------------------------------------------
std::string logValue(int v) {
    return "[int]    " + std::to_string(v);
}
std::string logValue(double v) {
    // 固定兩位小數，避免浮點輸出格式在不同環境有差異
    char buf[64];
    snprintf(buf, sizeof(buf), "%.2f", v);
    return std::string("[double] ") + buf;
}
std::string logValue(bool v) {
    return std::string("[bool]   ") + (v ? "true" : "false");
}
std::string logValue(const std::string& v) {
    return "[string] " + v;
}

int main() {
    printf("=== 原始示範：C 的做法（每個型別一個名字）===\n");
    printf("add_int(3, 5)          = %d\n", add_int(3, 5));
    printf("add_double(3.14, 2.86) = %f\n", add_double(3.14, 2.86));

    printf("\n=== C++ 的做法：同名多載 ===\n");
    printf("add(3, 5)          = %d       ← 選到 add(int, int)\n", add(3, 5));
    printf("add(3.14, 2.86)    = %f ← 選到 add(double, double)\n", add(3.14, 2.86));
    printf("add(3L, 5L)        = %ld       ← 選到 add(long, long)\n", add(3L, 5L));
    printf("add(1, 2, 3)       = %d        ← 參數個數不同也是多載\n", add(1, 2, 3));

    printf("\n=== 多載解析：隱式轉換會發生 ===\n");
    {
        // char 會被整數提升成 int，所以選到 add(int, int)
        const char c1 = 10, c2 = 20;
        printf("add('\\n', 20) 兩個 char  = %d  ← char 提升成 int\n", add(c1, c2));
        // float 會轉成 double
        const float f1 = 1.5f, f2 = 2.5f;
        printf("add(1.5f, 2.5f) 兩個 float = %f  ← float 轉成 double\n",
               add(f1, f2));
    }

    printf("\n=== name mangling：符號名帶著型別資訊 ===\n");
    printf("用 nm 觀察本檔編出的目標檔（本機 g++ 15.2 / Itanium ABI）：\n");
    printf("  int    add(int, int)       →  _Z3addii\n");
    printf("  double add(double, double) →  _Z3adddd\n");
    printf("  extern \"C\" 的函式           →  imgcore_sum_i32（完全不修飾）\n");
    printf("  拆解 _Z3addii：_Z=前綴, 3add=長度3的名稱 add, ii=兩個 int 參數\n");
    printf("  驗證指令：g++ -c 本檔 && nm 目標檔 | grep add\n");

    printf("\n=== 日常實務 1：extern \"C\" 對外介面 ===\n");
    {
        const int    ints[]    = {1, 2, 3, 4, 5};
        const double doubles[] = {1.5, 2.5, 3.0};
        printf("  C ABI  imgcore_sum_i32 = %d\n", imgcore_sum_i32(ints, 5));
        printf("  C ABI  imgcore_sum_f64 = %.2f\n", imgcore_sum_f64(doubles, 3));
        printf("  內部多載 internal::sum = %d / %.2f\n",
               internal::sum(ints, 5), internal::sum(doubles, 3));
        printf("  → 邊界用 C ABI（不能多載），內部照樣享受 C++ 多載\n");
    }

    printf("\n=== 日常實務 2：多載的 log 介面 ===\n");
    {
        printf("  %s\n", logValue(42).c_str());
        printf("  %s\n", logValue(3.14159).c_str());
        printf("  %s\n", logValue(true).c_str());
        printf("  %s\n", logValue(std::string("hello")).c_str());
        printf("  → 呼叫端一律寫 logValue(x)，型別判斷交給編譯器\n");
        printf("  → 注意 logValue(\"literal\") 會選到 bool 版本（見第 8 檔的陷阱）\n");
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2 課：C 與 C++ 的關鍵差異9.cpp" -o demo9
// 觀察符號: g++ -std=c++17 -c "第 2 課：C 與 C++ 的關鍵差異9.cpp" -o /tmp/o9.o
//           nm /tmp/o9.o | grep -E 'add|imgcore'      # 看修飾後的名稱
//           nm -C /tmp/o9.o | grep -E 'add|imgcore'   # -C 會還原成可讀形式
//
// 說明：
//   1. _Z3addii / _Z3adddd 這組符號名是 GCC/Clang 遵循的 Itanium C++ ABI 規則，
//      不是 C++ 標準規定的。MSVC 的修飾結果完全不同（形如 ?add@@YAHHH@Z）。
//   2. printf("%f") 預設輸出六位小數，所以 3.14 + 2.86 顯示為 6.000000。
//      這是 printf 的格式預設值，不是浮點誤差。
//   3. add(1.5f, 2.5f) 的兩個 float 引數會轉成 double 後選到 add(double,double)，
//      結果同樣以 %f 印出六位小數。

// === 預期輸出 ===
// === 原始示範：C 的做法（每個型別一個名字）===
// add_int(3, 5)          = 8
// add_double(3.14, 2.86) = 6.000000
//
// === C++ 的做法：同名多載 ===
// add(3, 5)          = 8       ← 選到 add(int, int)
// add(3.14, 2.86)    = 6.000000 ← 選到 add(double, double)
// add(3L, 5L)        = 8       ← 選到 add(long, long)
// add(1, 2, 3)       = 6        ← 參數個數不同也是多載
//
// === 多載解析：隱式轉換會發生 ===
// add('\n', 20) 兩個 char  = 30  ← char 提升成 int
// add(1.5f, 2.5f) 兩個 float = 4.000000  ← float 轉成 double
//
// === name mangling：符號名帶著型別資訊 ===
// 用 nm 觀察本檔編出的目標檔（本機 g++ 15.2 / Itanium ABI）：
//   int    add(int, int)       →  _Z3addii
//   double add(double, double) →  _Z3adddd
//   extern "C" 的函式           →  imgcore_sum_i32（完全不修飾）
//   拆解 _Z3addii：_Z=前綴, 3add=長度3的名稱 add, ii=兩個 int 參數
//   驗證指令：g++ -c 本檔 && nm 目標檔 | grep add
//
// === 日常實務 1：extern "C" 對外介面 ===
//   C ABI  imgcore_sum_i32 = 15
//   C ABI  imgcore_sum_f64 = 7.00
//   內部多載 internal::sum = 15 / 7.00
//   → 邊界用 C ABI（不能多載），內部照樣享受 C++ 多載
//
// === 日常實務 2：多載的 log 介面 ===
//   [int]    42
//   [double] 3.14
//   [bool]   true
//   [string] hello
//   → 呼叫端一律寫 logValue(x)，型別判斷交給編譯器
//   → 注意 logValue("literal") 會選到 bool 版本（見第 8 檔的陷阱）
