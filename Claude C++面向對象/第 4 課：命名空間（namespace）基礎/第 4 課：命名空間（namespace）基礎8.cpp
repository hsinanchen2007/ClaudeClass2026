// =============================================================================
//  第 4 課：命名空間（namespace）基礎（8） — 完整講義 + 可執行綜合示範
// =============================================================================
//
// 【本檔結構說明 — 先讀這段】
//   本檔由兩部分組成：
//     ① 第 1 個 /* ... */ 區塊：完整的課程講義（Markdown 格式），
//        其中包含大量「示意用」的程式碼片段，那些是註解，不會被編譯。
//     ② 註解區塊結束之後：真正會被編譯執行的綜合示範程式。
//   ★ 講義裡的片段與下方實際程式碼「高度相似但不完全相同」。
//     修改行為時請務必改「註解區塊之後」那一份 —— 那才是實際跑的程式。
//
// 【主題資訊 Information】
//   定義      ：namespace 名稱 { 宣告... }                       // C++98
//   巢狀(傳統)：namespace a { namespace b { ... } }              // C++98
//   巢狀(簡寫)：namespace a::b { ... }                           // ★ C++17
//   巢狀+inline：namespace a::inline b { ... }                   // ★ C++20
//   限定存取  ：a::b::member
//   using 宣告：using 命名空間::成員;                             // C++98
//   using 指示：using namespace 命名空間;                         // C++98
//   別名      ：namespace 別名 = 原命名空間;                      // C++98
//   匿名      ：namespace { ... }                                // C++98
//   複雜度    ：全部零執行期成本 —— 命名空間只影響編譯期的名稱查找
//               與連結期的符號修飾，不產生任何物件或間接層。
//
//   ★ 標準版本以 -pedantic-errors 實測驗證（見末尾但書），不是憑印象：
//     namespace a::b        在 -std=c++14 下明確報錯，需 C++17。
//     namespace a::inline b 在 -std=c++17 下明確報錯，需 C++20。
//   ★ 注意：講義的 4.7 節提到 namespace fs = std::filesystem;
//     本檔實際程式碼「沒有」使用它（未 include <filesystem>），
//     那段只是說明用的示意片段。
//
// 【詳細解釋 Explanation】
//
// 【1. 命名空間要解決的問題】
//   C 語言只有一個全域名稱空間，兩個函式庫各有一個 draw() 就會在連結期
//   撞成 multiple definition，且無解。C 的慣例是靠前綴硬撐（graphics_draw、
//   log_draw），C++ 則把「前綴」提升為語言機制：完整名稱不同即可共存，
//   而且編譯器認得這個結構，於是還能巢狀分層、選擇性引入、取別名、分次擴充。
//
// 【2. 三種存取方式，以及選擇的判準】
//   ① 完全限定名稱 math_tools::PI —— 最安全，標頭檔「只能」用這種。
//   ② using 宣告   using string_tools::toUpperCase; —— 白名單，只引入點名者。
//   ③ using 指示   using namespace math_tools; —— 全開，最方便也最危險。
//   判準是「引入的名稱數量 × 作用域大小」，兩者都要壓到最小。
//   本檔第 [6] 段把 ③ 關進 { } 區塊，離開即失效，正是實務該有的紀律。
//
// 【3. 為什麼「離開區塊後又要寫完整名稱」不是麻煩，而是保護】
//   本檔輸出的第 [6] 段最後一行刻意示範了這件事：區塊內可以直接寫 PI，
//   離開後必須寫 math_tools::PI。這證明 using 的效力確實被大括號限制住了。
//   若它會外洩，那麼「把 using 關進函式」這個廣泛採用的建議就完全失效了。
//
// 【4. 匿名命名空間：檔案私有的正統做法】
//   本檔第 [5] 段的 callCount / logCall 放在 namespace { } 裡，
//   只有這個翻譯單元看得到（internal linkage）。它取代 C 風格的 static，
//   主因是能力更強：static 只能修飾變數與函式，匿名命名空間還能容納
//   「型別定義」—— class 是不能寫 static 的。
//
// 【概念補充 Concept Deep Dive】
//
// (A) ADL：即使完全不寫 using，std 仍可能參與重載解析
//   引數依賴查找規定：呼叫非限定函式時，引數型別所屬的命名空間也會被納入
//   查找範圍。這就是 std::string s; std::cout << s; 能運作的原因 ——
//   operator<< 是在 std 裡找到的，而你從未寫過 using namespace std。
//
// (B) 名稱修飾：命名空間在連結期的實體
//   本機以 nm 實測（g++ 15.2.0，Itanium ABI）：
//       math_utils::add(int,int)  → T _ZN10math_utils3addEii   （大寫＝外部連結）
//       命名空間中的 const double → r _ZN10math_utilsL2PIE     （小寫＝內部連結）
//       匿名命名空間的成員        → t _ZN12_GLOBAL__N_1L16internalFunctionEv
//   _GLOBAL__N_1 就是編譯器替匿名命名空間生成的唯一名字。
//
// (C) 命名空間範圍的 const 具有 internal linkage
//   這是 C++ 與 C 少數不相容之處。本檔 math_tools::PI 寫在標頭檔也不會
//   造成重複定義（每個 .cpp 各一份）；C 則會撞。要在標頭提供「唯一一份」
//   的變數，C++17 起請用 inline 變數。
//
// 【注意事項 Pay Attention】
//   1. ★ 標頭檔的全域範圍絕不可出現 using（宣告與指示皆然）。#include 的
//      本質是文字貼上，那一行會出現在每個引入者的全域範圍，而他們不知情。
//   2. using 指示造成的衝突，錯誤報在「使用處」而非「宣告處」。本機實測：
//      全域 int count 搭配 using namespace std 與 <algorithm>，宣告行不報錯，
//      直到使用 count 的那一行才爆 error: reference to 'count' is ambiguous。
//   3. 命名空間名稱打錯不會報錯，只會安靜地新建一個命名空間。實測錯誤訊息
//      出現在呼叫端：error: 'b' is not a member of 'mylib'; did you mean 'mylibb::b'?
//   4. 匿名命名空間絕不可放進標頭檔：每個翻譯單元各得一份「獨立」副本，
//      a.cpp 改了值 b.cpp 讀不到，而且編譯連結全部成功，只是行為不對。
//   5. 不要往 namespace std 新增宣告。除標準明文允許的樣板特化
//      （例如為自訂型別特化 std::hash）之外，一律是未定義行為。
//   6. 命名空間不是存取控制：它沒有 private，任何人都能再開啟它加東西。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】命名空間（完整講義版）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼不能在標頭檔裡寫 using namespace std;?
//     答：因為 #include 的本質是文字展開。那一行會被貼進每一個引入該標頭的
//         .cpp 的全域範圍，效力涵蓋該檔「引入之後」的所有程式碼。
//         於是別人的全域 count、size、data 突然變成歧義，而編譯錯誤指向
//         他自己的檔案，他幾乎不可能想到肇因是你的標頭。
//     追問：那寫在 .cpp 的全域範圍可以嗎？
//         → 小型單檔程式可接受，影響僅限該檔。但更好的做法是把它關進函式
//           （本檔第 [6] 段），或改用 using 宣告只引入需要的名稱。
//
// 🔥 Q2. 命名空間有執行期成本嗎？編譯後它變成什麼？
//     答：零執行期成本。它只影響編譯期的名稱查找與連結期的符號修飾。
//         本機 nm 實測：math_utils::add(int,int) 的符號是
//         _ZN10math_utils3addEii，其中 10math_utils 是命名空間名稱與其長度。
//         沒有任何額外的物件、指標或間接呼叫被產生。
//     追問：那 namespace 和 class 的 static 成員函式該怎麼選？
//         → 只是要把一批自由函式分組就用 namespace（可分次擴充、可取別名）；
//           需要存取控制、實體化或繼承，才用 class。用 class 純粹當
//           「函式容器」是 Java 帶來的習慣，在 C++ 並不必要。
//
// 🔥 Q3. 巢狀命名空間的 C++17 簡寫 namespace a::b { } 和傳統的
//        namespace a { namespace b { } } 有什麼差別？
//     答：語意完全相同，只是語法糖 —— 本檔第 [4] 段的存取方式對兩種寫法
//         一模一樣即為證明。差別只在標準版本：簡寫需要 C++17，
//         以 -std=c++14 -pedantic-errors 編譯會直接報錯（本機實測）。
//     追問：那 namespace a::inline b { } 呢？
//         → 那是 C++20 才有的（本機以 -std=c++17 -pedantic-errors 實測會報錯）。
//           inline namespace 本身是 C++11，但「在巢狀簡寫中使用 inline」
//           要到 C++20。
//
// ⚠️ 陷阱 1. 我寫了 using namespace std; 又宣告全域 int count = 0;，
//          編譯器在宣告那行沒報錯 —— 是不是代表安全？
//     答：不是。實測（g++ 15.2.0 + <algorithm>）顯示宣告行確實不報錯，
//         但只要有任何一處使用非限定的 count，那一行就會爆
//         error: reference to 'count' is ambiguous。using 指示不是把名稱
//         複製進來，而是讓查找時「也去看」std，所以衝突要等到查找發生時
//         才被偵測到。
//     為什麼會錯：把「目前編得過」當成安全證明。using 指示埋下的是延遲
//         引爆的地雷 —— 可能在你新增一行程式碼、或升級標準時才觸發
//         （C++17 新增 std::size，C++20 新增大量 ranges 名稱）。
//
// ⚠️ 陷阱 2. 為了讓每個檔案都有自己的私有計數器，我把匿名命名空間寫進標頭檔。
//          編譯連結都成功，為什麼 a.cpp 加到 10、b.cpp 讀出來還是 0？
//     答：因為「每個翻譯單元各得一份獨立副本」正是匿名命名空間的定義。
//         標頭被 include 到 N 個 .cpp，就產生 N 個彼此無關的變數。
//         沒有符號衝突，所以編譯與連結都不會有任何警告 —— 程式只是行為不對。
//     為什麼會錯：把匿名命名空間想成「加了保護的全域變數」，以為它仍是
//         「一個」變數。它改變的是「有幾份」，不是「誰能碰」。
//         標頭中要提供唯一的共享變數，C++17 起請用 inline 變數。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【本檔不加 LeetCode 範例的理由】
//   命名空間是「程式碼組織與連結可見性」的機制，不影響任何演算法的時間或
//   空間複雜度，與 LeetCode 的評量面向沒有交集。附帶一提：LeetCode 提交
//   範本裡的 using namespace std; 正是這個寫法最合理的使用場景 —— 單檔、
//   生命週期只有幾分鐘、沒有人需要維護。把同樣的習慣帶進正式專案才是問題。
//
// 【日常實務範例】
//   本檔下方的 math_tools / string_tools / company::graphics|audio 本身
//   就是「模擬真實專案結構」的實務範例（講義 4.11 節明言此意圖），
//   已充分呈現命名空間在專案中的組織方式，故不再另外添加情境。

/*
# 第 4 課：命名空間（namespace）基礎

---

## 4.1 課程目標

當程式規模變大，或使用多個函式庫時，很容易遇到**名稱衝突**的問題——不同的函式庫可能定義了相同名稱的函數或類別。C++ 的**命名空間（namespace）**機制就是為了解決這個問題而設計的。

本課將學習命名空間的概念、語法，以及如何正確使用它。

---

## 4.2 為什麼需要命名空間？

### 4.2.1 問題情境

假設你在開發一個遊戲，使用了兩個第三方函式庫：

- **圖形庫**：提供一個 `draw()` 函數來繪製圖形
- **日誌庫**：也提供一個 `draw()` 函數來「抽取」日誌記錄

當你同時使用這兩個函式庫時，編譯器無法知道你要呼叫哪一個 `draw()`！

### 4.2.2 C 語言的解決方案（前綴命名）

在 C 語言中，通常用**前綴**來避免衝突：

```c
// 圖形庫的函數
void graphics_draw();
void graphics_clear();
void graphics_init();

// 日誌庫的函數
void log_draw();
void log_write();
void log_init();
```

這種方式雖然有效，但名稱變得很長，而且容易打錯。

### 4.2.3 C++ 的解決方案（命名空間）

C++ 使用命名空間來組織程式碼：

```cpp
namespace graphics {
    void draw();
    void clear();
    void init();
}

namespace log {
    void draw();
    void write();
    void init();
}

// 使用時明確指定
graphics::draw();  // 呼叫圖形庫的 draw
log::draw();       // 呼叫日誌庫的 draw
```

---

## 4.3 命名空間的基本語法

### 4.3.1 定義命名空間

```cpp
namespace 命名空間名稱 {
    // 變數、函數、類別等
}
```

### 4.3.2 簡單範例

```cpp
#include <iostream>

// 定義命名空間 math_utils
namespace math_utils {
    const double PI = 3.14159265358979;
    
    int add(int a, int b) {
        return a + b;
    }
    
    int square(int x) {
        return x * x;
    }
}

int main() {
    // 使用命名空間中的成員，需要加上 命名空間:: 前綴
    std::cout << "PI = " << math_utils::PI << std::endl;
    std::cout << "add(3, 5) = " << math_utils::add(3, 5) << std::endl;
    std::cout << "square(4) = " << math_utils::square(4) << std::endl;
    
    return 0;
}
```

### 輸出

```
PI = 3.14159
add(3, 5) = 8
square(4) = 16
```

---

## 4.4 存取命名空間成員的三種方式

### 4.4.1 方式一：完全限定名稱（推薦）

使用 `命名空間::成員` 的完整形式：

```cpp
std::cout << "Hello" << std::endl;
std::string name = "Alice";
std::vector<int> numbers;
```

**優點**：清楚明確，不會有歧義
**缺點**：程式碼較長

### 4.4.2 方式二：using 宣告（引入特定成員）

使用 `using` 引入特定的成員：

```cpp
#include <iostream>
#include <string>

using std::cout;    // 只引入 cout
using std::endl;    // 只引入 endl
using std::string;  // 只引入 string

int main() {
    cout << "Hello" << endl;  // 可以直接使用 cout 和 endl
    string name = "Alice";    // 可以直接使用 string
    
    // 其他成員仍需完整名稱
    std::cin >> name;
    
    return 0;
}
```

**優點**：只引入需要的成員，減少名稱衝突風險
**缺點**：需要逐一列出要引入的成員

### 4.4.3 方式三：using 指令（引入整個命名空間）

使用 `using namespace` 引入整個命名空間：

```cpp
#include <iostream>
#include <string>

using namespace std;  // 引入 std 的所有成員

int main() {
    cout << "Hello" << endl;
    string name = "Alice";
    cin >> name;
    
    return 0;
}
```

**優點**：程式碼最簡潔
**缺點**：可能導致名稱衝突，不推薦在大型專案中使用

---

## 4.5 using namespace 的風險

### 4.5.1 名稱衝突範例

```cpp
#include <iostream>
#include <algorithm>  // 包含 std::count

using namespace std;

// 自己定義的 count 函數
int count(int arr[], int size) {
    return size;
}

int main() {
    int arr[] = {1, 2, 3, 4, 5};
    
    // 這裡會產生歧義！
    // 是呼叫我們的 count，還是 std::count？
    // cout << count(arr, 5) << endl;  // 編譯錯誤
    
    return 0;
}
```

### 4.5.2 最佳實踐建議

| 情況 | 建議 |
|------|------|
| 標頭檔（.h / .hpp） | **絕對不要**使用 `using namespace` |
| 小型練習程式 | 可以使用 `using namespace std;` |
| 大型專案 | 使用完全限定名稱或 `using` 宣告 |
| 函數內部 | 可以使用 `using namespace`（作用域限制在函數內） |

### 4.5.3 安全的使用方式

```cpp
#include <iostream>
#include <vector>

// 不要在全域範圍使用 using namespace

void processData() {
    // 在函數內部使用是安全的，作用域限制在這個函數內
    using namespace std;
    
    vector<int> data = {1, 2, 3, 4, 5};
    for (int x : data) {
        cout << x << " ";
    }
    cout << endl;
}

int main() {
    // 這裡仍需使用完整名稱
    std::cout << "開始處理..." << std::endl;
    processData();
    std::cout << "處理完成" << std::endl;
    
    return 0;
}
```

---

## 4.6 標準函式庫的命名空間：std

C++ 標準函式庫的所有內容都定義在 `std` 命名空間中：

| 成員 | 說明 |
|------|------|
| `std::cout` | 標準輸出 |
| `std::cin` | 標準輸入 |
| `std::endl` | 換行並清空緩衝區 |
| `std::string` | 字串類別 |
| `std::vector` | 動態陣列類別 |
| `std::map` | 關聯容器 |
| `std::sort` | 排序演算法 |
| `std::move` | 移動語義函數 |

---

## 4.7 巢狀命名空間

命名空間可以嵌套：

### 4.7.1 傳統寫法

```cpp
namespace company {
    namespace project {
        namespace module {
            void doSomething() {
                // ...
            }
        }
    }
}

// 使用時
company::project::module::doSomething();
```

### 4.7.2 C++17 簡化寫法

```cpp
namespace company::project::module {
    void doSomething() {
        // ...
    }
}

// 使用方式相同
company::project::module::doSomething();
```

---

## 4.8 命名空間別名

當命名空間名稱很長時，可以建立別名：

```cpp
#include <iostream>

namespace very_long_namespace_name {
    void greet() {
        std::cout << "Hello from long namespace!" << std::endl;
    }
}

int main() {
    // 建立別名
    namespace vln = very_long_namespace_name;
    
    // 使用別名
    vln::greet();
    
    // 原名稱仍然有效
    very_long_namespace_name::greet();
    
    return 0;
}
```

實際應用範例：

```cpp
// 標準函式庫中常見的簡化
namespace fs = std::filesystem;  // C++17

// 使用
fs::path p = "/home/user/documents";
```

---

## 4.9 匿名命名空間

沒有名稱的命名空間稱為**匿名命名空間**，其成員只在當前檔案內可見：

```cpp
#include <iostream>

// 匿名命名空間
namespace {
    int internalCounter = 0;  // 只在這個檔案內可見
    
    void internalFunction() {
        std::cout << "這是內部函數" << std::endl;
    }
}

int main() {
    internalCounter = 10;  // 可以直接使用，不需前綴
    internalFunction();
    
    return 0;
}
```

匿名命名空間的作用類似於 C 語言中的 `static`，用於限制符號的連結範圍。

### 比較

| 方式 | C 語言 | C++ |
|------|--------|-----|
| 檔案內部函數 | `static void func()` | 放在匿名命名空間中 |
| 檔案內部變數 | `static int var` | 放在匿名命名空間中 |

---

## 4.10 擴展命名空間

同一個命名空間可以在多處定義，編譯器會將它們合併：

```cpp
#include <iostream>

// 第一部分
namespace mylib {
    void funcA() {
        std::cout << "Function A" << std::endl;
    }
}

// 第二部分（擴展同一個命名空間）
namespace mylib {
    void funcB() {
        std::cout << "Function B" << std::endl;
    }
}

// 這在多檔案專案中很常見：
// math.h 定義 namespace mylib { 數學函數 }
// string.h 定義 namespace mylib { 字串函數 }
// 它們都屬於同一個 mylib 命名空間

int main() {
    mylib::funcA();
    mylib::funcB();
    
    return 0;
}
```

---

## 4.11 完整範例程式

```cpp
#include <iostream>
#include <string>
#include <cmath>

// ============================================================
// 定義多個命名空間，模擬實際專案結構
// ============================================================

// 數學工具命名空間
namespace math_tools {
    const double PI = 3.14159265358979;
    const double E = 2.71828182845905;
    
    double circleArea(double radius) {
        return PI * radius * radius;
    }
    
    double circleCircumference(double radius) {
        return 2 * PI * radius;
    }
    
    int power(int base, int exp) {
        int result = 1;
        for (int i = 0; i < exp; i++) {
            result *= base;
        }
        return result;
    }
}

// 字串工具命名空間
namespace string_tools {
    std::string toUpperCase(const std::string& str) {
        std::string result = str;
        for (char& c : result) {
            if (c >= 'a' && c <= 'z') {
                c = c - 'a' + 'A';
            }
        }
        return result;
    }
    
    std::string toLowerCase(const std::string& str) {
        std::string result = str;
        for (char& c : result) {
            if (c >= 'A' && c <= 'Z') {
                c = c - 'A' + 'a';
            }
        }
        return result;
    }
    
    std::string repeat(const std::string& str, int times) {
        std::string result;
        for (int i = 0; i < times; i++) {
            result += str;
        }
        return result;
    }
}

// 巢狀命名空間（模擬公司專案結構）
namespace company {
    namespace graphics {
        void draw() {
            std::cout << "[Graphics] 繪製圖形" << std::endl;
        }
        
        void clear() {
            std::cout << "[Graphics] 清除畫面" << std::endl;
        }
    }
    
    namespace audio {
        void play() {
            std::cout << "[Audio] 播放音效" << std::endl;
        }
        
        void stop() {
            std::cout << "[Audio] 停止播放" << std::endl;
        }
    }
}

// 匿名命名空間（檔案內部使用）
namespace {
    int callCount = 0;
    
    void logCall(const std::string& funcName) {
        callCount++;
        std::cout << "  [Log] 第 " << callCount << " 次呼叫: " 
                  << funcName << std::endl;
    }
}

// ============================================================
// 主程式
// ============================================================
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    命名空間（namespace）展示" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // --------------------------------------------------
    // 1. 使用完全限定名稱
    // --------------------------------------------------
    std::cout << "[1] 完全限定名稱存取" << std::endl;
    std::cout << "  PI = " << math_tools::PI << std::endl;
    std::cout << "  E = " << math_tools::E << std::endl;
    std::cout << "  圓面積(r=5) = " << math_tools::circleArea(5) << std::endl;
    std::cout << "  2^10 = " << math_tools::power(2, 10) << std::endl;
    std::cout << std::endl;
    
    // --------------------------------------------------
    // 2. 使用 using 宣告（引入特定成員）
    // --------------------------------------------------
    std::cout << "[2] using 宣告（引入特定成員）" << std::endl;
    
    using string_tools::toUpperCase;
    using string_tools::toLowerCase;
    
    std::string text = "Hello World";
    std::cout << "  原始: " << text << std::endl;
    std::cout << "  大寫: " << toUpperCase(text) << std::endl;
    std::cout << "  小寫: " << toLowerCase(text) << std::endl;
    
    // repeat 沒有 using，仍需完整名稱
    std::cout << "  重複: " << string_tools::repeat("Hi", 3) << std::endl;
    std::cout << std::endl;
    
    // --------------------------------------------------
    // 3. 命名空間別名
    // --------------------------------------------------
    std::cout << "[3] 命名空間別名" << std::endl;
    
    namespace mt = math_tools;
    namespace st = string_tools;
    
    std::cout << "  mt::PI = " << mt::PI << std::endl;
    std::cout << "  st::toUpperCase(\"test\") = " 
              << st::toUpperCase("test") << std::endl;
    std::cout << std::endl;
    
    // --------------------------------------------------
    // 4. 巢狀命名空間
    // --------------------------------------------------
    std::cout << "[4] 巢狀命名空間" << std::endl;
    
    company::graphics::draw();
    company::graphics::clear();
    company::audio::play();
    company::audio::stop();
    std::cout << std::endl;
    
    // 也可以使用別名簡化
    namespace gfx = company::graphics;
    namespace sfx = company::audio;
    
    std::cout << "  使用別名：" << std::endl;
    gfx::draw();
    sfx::play();
    std::cout << std::endl;
    
    // --------------------------------------------------
    // 5. 匿名命名空間（內部使用）
    // --------------------------------------------------
    std::cout << "[5] 匿名命名空間（內部計數器）" << std::endl;
    
    logCall("function1");
    logCall("function2");
    logCall("function3");
    
    std::cout << "  總呼叫次數: " << callCount << std::endl;
    std::cout << std::endl;
    
    // --------------------------------------------------
    // 6. 在區塊內使用 using namespace
    // --------------------------------------------------
    std::cout << "[6] 區塊內的 using namespace" << std::endl;
    
    {
        // 只在這個區塊內有效
        using namespace math_tools;
        using namespace string_tools;
        
        std::cout << "  在區塊內可以直接使用：" << std::endl;
        std::cout << "  PI = " << PI << std::endl;
        std::cout << "  circleArea(3) = " << circleArea(3) << std::endl;
        std::cout << "  toUpperCase(\"abc\") = " << toUpperCase("abc") << std::endl;
    }
    
    // 離開區塊後，又需要完整名稱
    std::cout << "  離開區塊後：math_tools::PI = " << math_tools::PI << std::endl;
    std::cout << std::endl;
    
    std::cout << "========================================" << std::endl;
    std::cout << "    展示完成" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
```

### 編譯與執行

```bash
g++ -std=c++17 -Wall -Wextra lesson04.cpp -o lesson04
./lesson04
```

### 執行結果

```
========================================
    命名空間（namespace）展示
========================================

[1] 完全限定名稱存取
  PI = 3.14159
  E = 2.71828
  圓面積(r=5) = 78.5398
  2^10 = 1024

[2] using 宣告（引入特定成員）
  原始: Hello World
  大寫: HELLO WORLD
  小寫: hello world
  重複: HiHiHi

[3] 命名空間別名
  mt::PI = 3.14159
  st::toUpperCase("test") = TEST

[4] 巢狀命名空間
[Graphics] 繪製圖形
[Graphics] 清除畫面
[Audio] 播放音效
[Audio] 停止播放

  使用別名：
[Graphics] 繪製圖形
[Audio] 播放音效

[5] 匿名命名空間（內部計數器）
  [Log] 第 1 次呼叫: function1
  [Log] 第 2 次呼叫: function2
  [Log] 第 3 次呼叫: function3
  總呼叫次數: 3

[6] 區塊內的 using namespace
  在區塊內可以直接使用：
  PI = 3.14159
  circleArea(3) = 28.2743
  toUpperCase("abc") = ABC
  離開區塊後：math_tools::PI = 3.14159

========================================
    展示完成
========================================
```

---

## 4.12 本課重點回顧

| 概念 | 說明 |
|------|------|
| 命名空間目的 | 避免名稱衝突，組織程式碼 |
| 定義語法 | `namespace name { ... }` |
| 存取成員 | 使用 `::` 運算子，如 `std::cout` |
| using 宣告 | `using std::cout;` 引入特定成員 |
| using 指令 | `using namespace std;` 引入整個命名空間 |
| 命名空間別名 | `namespace fs = std::filesystem;` |
| 巢狀命名空間 | `namespace a::b::c { }` (C++17) |
| 匿名命名空間 | `namespace { }` 限制在檔案內部 |
| std 命名空間 | C++ 標準函式庫的命名空間 |

---

## 4.13 最佳實踐總結

1. **標頭檔中絕對不要使用 `using namespace`**
2. **優先使用完全限定名稱**（如 `std::cout`）
3. **若要簡化，使用 `using` 宣告引入特定成員**
4. **`using namespace` 只在小範圍內使用**（如函數內部）
5. **為長命名空間建立別名**
6. **使用匿名命名空間取代 `static` 限制檔案作用域**

---

## 4.14 下一課預告

下一課我們將學習 **輸入輸出流（iostream）**，深入了解 C++ 的 `cout`、`cin`、格式化輸出等功能，這是 C++ 程式設計的基本技能。

準備好進入 **第 5 課：輸入輸出流（iostream）入門** 了嗎？
*/



#include <iostream>
#include <string>
#include <cmath>

// ============================================================
// 定義多個命名空間，模擬實際專案結構
// ============================================================

// 數學工具命名空間
namespace math_tools {
    const double PI = 3.14159265358979;
    const double E = 2.71828182845905;
    
    double circleArea(double radius) {
        return PI * radius * radius;
    }
    
    double circleCircumference(double radius) {
        return 2 * PI * radius;
    }
    
    int power(int base, int exp) {
        int result = 1;
        for (int i = 0; i < exp; i++) {
            result *= base;
        }
        return result;
    }
}

// 字串工具命名空間
namespace string_tools {
    std::string toUpperCase(const std::string& str) {
        std::string result = str;
        for (char& c : result) {
            if (c >= 'a' && c <= 'z') {
                c = c - 'a' + 'A';
            }
        }
        return result;
    }
    
    std::string toLowerCase(const std::string& str) {
        std::string result = str;
        for (char& c : result) {
            if (c >= 'A' && c <= 'Z') {
                c = c - 'A' + 'a';
            }
        }
        return result;
    }
    
    std::string repeat(const std::string& str, int times) {
        std::string result;
        for (int i = 0; i < times; i++) {
            result += str;
        }
        return result;
    }
}

// 巢狀命名空間（模擬公司專案結構）
namespace company {
    namespace graphics {
        void draw() {
            std::cout << "[Graphics] 繪製圖形" << std::endl;
        }
        
        void clear() {
            std::cout << "[Graphics] 清除畫面" << std::endl;
        }
    }
    
    namespace audio {
        void play() {
            std::cout << "[Audio] 播放音效" << std::endl;
        }
        
        void stop() {
            std::cout << "[Audio] 停止播放" << std::endl;
        }
    }
}

// 匿名命名空間（檔案內部使用）
namespace {
    int callCount = 0;
    
    void logCall(const std::string& funcName) {
        callCount++;
        std::cout << "  [Log] 第 " << callCount << " 次呼叫: " 
                  << funcName << std::endl;
    }
}

// ============================================================
// 主程式
// ============================================================
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    命名空間（namespace）展示" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // --------------------------------------------------
    // 1. 使用完全限定名稱
    // --------------------------------------------------
    std::cout << "[1] 完全限定名稱存取" << std::endl;
    std::cout << "  PI = " << math_tools::PI << std::endl;
    std::cout << "  E = " << math_tools::E << std::endl;
    std::cout << "  圓面積(r=5) = " << math_tools::circleArea(5) << std::endl;
    std::cout << "  2^10 = " << math_tools::power(2, 10) << std::endl;
    std::cout << std::endl;
    
    // --------------------------------------------------
    // 2. 使用 using 宣告（引入特定成員）
    // --------------------------------------------------
    std::cout << "[2] using 宣告（引入特定成員）" << std::endl;
    
    using string_tools::toUpperCase;
    using string_tools::toLowerCase;
    
    std::string text = "Hello World";
    std::cout << "  原始: " << text << std::endl;
    std::cout << "  大寫: " << toUpperCase(text) << std::endl;
    std::cout << "  小寫: " << toLowerCase(text) << std::endl;
    
    // repeat 沒有 using，仍需完整名稱
    std::cout << "  重複: " << string_tools::repeat("Hi", 3) << std::endl;
    std::cout << std::endl;
    
    // --------------------------------------------------
    // 3. 命名空間別名
    // --------------------------------------------------
    std::cout << "[3] 命名空間別名" << std::endl;
    
    namespace mt = math_tools;
    namespace st = string_tools;
    
    std::cout << "  mt::PI = " << mt::PI << std::endl;
    std::cout << "  st::toUpperCase(\"test\") = " 
              << st::toUpperCase("test") << std::endl;
    std::cout << std::endl;
    
    // --------------------------------------------------
    // 4. 巢狀命名空間
    // --------------------------------------------------
    std::cout << "[4] 巢狀命名空間" << std::endl;
    
    company::graphics::draw();
    company::graphics::clear();
    company::audio::play();
    company::audio::stop();
    std::cout << std::endl;
    
    // 也可以使用別名簡化
    namespace gfx = company::graphics;
    namespace sfx = company::audio;
    
    std::cout << "  使用別名：" << std::endl;
    gfx::draw();
    sfx::play();
    std::cout << std::endl;
    
    // --------------------------------------------------
    // 5. 匿名命名空間（內部使用）
    // --------------------------------------------------
    std::cout << "[5] 匿名命名空間（內部計數器）" << std::endl;
    
    logCall("function1");
    logCall("function2");
    logCall("function3");
    
    std::cout << "  總呼叫次數: " << callCount << std::endl;
    std::cout << std::endl;
    
    // --------------------------------------------------
    // 6. 在區塊內使用 using namespace
    // --------------------------------------------------
    std::cout << "[6] 區塊內的 using namespace" << std::endl;
    
    {
        // 只在這個區塊內有效
        using namespace math_tools;
        using namespace string_tools;
        
        std::cout << "  在區塊內可以直接使用：" << std::endl;
        std::cout << "  PI = " << PI << std::endl;
        std::cout << "  circleArea(3) = " << circleArea(3) << std::endl;
        std::cout << "  toUpperCase(\"abc\") = " << toUpperCase("abc") << std::endl;
    }
    
    // 離開區塊後，又需要完整名稱
    std::cout << "  離開區塊後：math_tools::PI = " << math_tools::PI << std::endl;
    std::cout << std::endl;
    
    std::cout << "========================================" << std::endl;
    std::cout << "    展示完成" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 4 課：命名空間（namespace）基礎8.cpp" -o ns8

// ─────────────────────────────────────────────────────────────────────────────
// 【輸出但書】
//  1. 本檔前段是 /* */ 講義區塊（不編譯），實際執行的是註解結束之後那份程式。
//     講義中的片段與實際程式碼高度相似但不完全相同，以下輸出來自後者。
//  2. PI = 3.14159、圓面積 78.5398、circleArea(3) = 28.2743 都是
//     std::cout 預設 6 位有效數字的結果，不是數值被截斷。
//  3. [4] 段中 [Graphics]/[Audio] 各行沒有縮排，是因為那些字串由命名空間內的
//     函式自己輸出（函式內未加前導空白），而非呼叫端排版問題。
//  4. [6] 段最後一行「離開區塊後：math_tools::PI」示範了 using 指示的效力
//     確實被大括號限制住 —— 區塊內可直接寫 PI，離開後必須寫完整名稱。
//  5. 標準版本的說法以 -pedantic-errors 實測驗證（g++ 15.2.0）：
//       -std=c++14 編 namespace a::b { }
//         → error: nested namespace definitions only available with '-std=c++17'
//       -std=c++17 編 namespace a::inline b { }
//         → error: nested inline namespace definitions only available with '-std=c++20'
//     驗證標準版本必須用 -pedantic-errors：只用 -fsyntax-only 時 GCC 會把它
//     當作擴充放行，看起來像「舊標準也支援」，實際上並不可攜。
//  6. 講義 4.7 節提到的 namespace fs = std::filesystem; 只是示意，
//     本檔實際程式碼並未使用（也未 include <filesystem>），故輸出中不會出現。
//  7. 以下為本機 g++ 15.2.0 (Ubuntu 26.04) 連續執行 3 次、逐位元組相同的結果。
// ─────────────────────────────────────────────────────────────────────────────

// === 預期輸出 ===
// ========================================
//     命名空間（namespace）展示
// ========================================
//
// [1] 完全限定名稱存取
//   PI = 3.14159
//   E = 2.71828
//   圓面積(r=5) = 78.5398
//   2^10 = 1024
//
// [2] using 宣告（引入特定成員）
//   原始: Hello World
//   大寫: HELLO WORLD
//   小寫: hello world
//   重複: HiHiHi
//
// [3] 命名空間別名
//   mt::PI = 3.14159
//   st::toUpperCase("test") = TEST
//
// [4] 巢狀命名空間
// [Graphics] 繪製圖形
// [Graphics] 清除畫面
// [Audio] 播放音效
// [Audio] 停止播放
//
//   使用別名：
// [Graphics] 繪製圖形
// [Audio] 播放音效
//
// [5] 匿名命名空間（內部計數器）
//   [Log] 第 1 次呼叫: function1
//   [Log] 第 2 次呼叫: function2
//   [Log] 第 3 次呼叫: function3
//   總呼叫次數: 3
//
// [6] 區塊內的 using namespace
//   在區塊內可以直接使用：
//   PI = 3.14159
//   circleArea(3) = 28.2743
//   toUpperCase("abc") = ABC
//   離開區塊後：math_tools::PI = 3.14159
//
// ========================================
//     展示完成
// ========================================
