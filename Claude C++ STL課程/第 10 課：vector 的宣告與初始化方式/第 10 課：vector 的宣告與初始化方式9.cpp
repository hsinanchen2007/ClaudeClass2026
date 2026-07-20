// =============================================================================
//  第 10 課：vector 的宣告與初始化方式 9  —  本課講義 + CTAD（C++17 型別推導）
// =============================================================================
//
// 【主題資訊 Information】
//   本檔前半是第 10 課的完整講義（保存在下方區塊註解），
//   後半是可執行的 CTAD（Class Template Argument Deduction）示範。
//   CTAD 語法：
//       std::vector v = {1, 2, 3};          // 推導為 vector<int>
//       std::vector v(first, last);         // 從迭代器的 value_type 推導
//   標頭檔：<vector>
//   標準版本：**CTAD 是 C++17**。本機以 -std=c++14 -pedantic-errors 實測，
//             `std::vector v = {1,2,3};` 會被拒絕；-std=c++17 才通過。
//   注意：CTAD 推導的是「類別模板的參數」，
//         和 auto（推導變數型別）、template argument deduction
//         （推導函式模板參數）是三件不同的事。
//
// 【詳細解釋 Explanation】
//
// 【1. C++17 之前為什麼一定要寫出型別】
//   C++17 之前，函式模板可以推導參數（std::make_pair(1, 2.0) 就是靠這個），
//   但類別模板不行。所以標準函式庫得為每個類別各寫一個 make_ 工廠函式：
//       std::make_pair、std::make_tuple、std::make_shared…
//   這些函式存在的唯一理由，就是「借用函式模板的推導能力」來繞過限制。
//   C++17 的 CTAD 直接讓類別模板自己會推導，這批 make_ 函式從此
//   大多只剩相容性價值（std::make_shared 除外——它還負責合併配置）。
//
// 【2. CTAD 靠什麼推導：推導指引（deduction guides）】
//   編譯器會把類別的建構子當成一組候選函式來做重載解析。
//   `std::vector v = {1, 2, 3};` 匹配到 vector(initializer_list<T>)，
//   於是 T 推導為 int。
//   但有些情況建構子本身推不出來，例如從一對迭代器建構——
//   vector(InputIt, InputIt) 的簽章裡根本沒有 T。
//   這時就需要標準函式庫明確提供的推導指引：
//       template<class InputIt, class Alloc = ...>
//       vector(InputIt, InputIt, Alloc = Alloc())
//           -> vector<typename iterator_traits<InputIt>::value_type, Alloc>;
//   這行指引告訴編譯器「用迭代器的 value_type 當 T」。
//   本檔的 v3 就是走這條路。
//
// 【3. CTAD 最容易出意外的地方：括號種類會改變推導結果】
//   std::vector<int> src = {1, 2, 3};
//   std::vector a{src};        // 推導成什麼？
//   直覺會說 vector<vector<int>>（一個含單一元素的巢狀 vector）。
//   實際上是 vector<int>，也就是 src 的複本。
//   原因：重載解析時「複製建構子」贏過 initializer_list 建構子。
//   這個規則讓 `std::vector a{src};` 和 `std::vector a(src);` 結果相同，
//   算是避免了更大的困惑，但和「大括號一律是元素清單」的直覺相衝。
//   本檔的 main 有實測。
//
// 【4. 什麼時候該用 CTAD、什麼時候不該】
//   該用：型別從初始值一眼可知，寫出來只是噪音。
//       std::vector v = {1, 2, 3};
//       std::pair p{1, "x"};
//   不該用：型別不明顯，或推導結果可能不是你要的。
//       std::vector v = {1u, 2, 3};       // 混型別 → 直接編譯失敗（推不出唯一 T）
//       std::vector v(10, 0);             // 這是 10 個 0 還是兩個元素？寫明比較好
//   一般準則：CTAD 省的是打字，不該用來省「讀者的理解成本」。
//
// 【概念補充 Concept Deep Dive】
//
// (A) CTAD 不做隱式轉換的統一
//     std::vector v = {1, 2, 3.0};    // 編譯失敗
//     推導 T 時三個元素分別是 int, int, double，無法得到唯一的 T。
//     編譯器不會「自動升級成 double」——推導失敗就是失敗。
//     這和 auto 面對 {1, 2, 3.0} 的行為一致（也會失敗）。
//
// (B) CTAD 與 auto 的分工
//     auto v = std::vector<int>{1, 2, 3};   // auto 推導「變數」的型別
//     std::vector v = {1, 2, 3};            // CTAD 推導「模板參數」
//     兩者可以合用：auto v = std::vector{1, 2, 3};
//     但這樣寫沒有好處，反而讓型別完全消失在字面上。
//
// (C) C++20 對 CTAD 的擴充
//     C++20 起，別名模板（alias template）也支援 CTAD，
//     聚合型別（aggregate）也能推導：
//         template<class T> struct Box { T value; };
//         Box b{42};                          // C++20 起合法，推導為 Box<int>
//     C++17 時這需要自己寫推導指引。
//
// 【注意事項 Pay Attention】
//   1. CTAD 是 C++17。用 -std=c++14 編譯會直接失敗
//      （驗證方式：加 -pedantic-errors，只用 -fsyntax-only 會被
//       GCC 當成擴充放行，看起來像舊標準也支援）。
//   2. std::vector v{other_vector} 推導成「複本」不是「巢狀 vector」——
//      複製建構子在重載解析中勝過 initializer_list 建構子。
//   3. 元素型別不一致時 CTAD 直接失敗，不會自動做算術轉換。
//   4. CTAD 不能只指定部分參數。std::vector<int> v(...) 是明確指定，
//      std::vector v(...) 是全部推導，沒有中間狀態。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】CTAD 與 vector 的初始化
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. CTAD 是哪個標準版本引入的？在那之前標準函式庫怎麼繞過限制？
//     答：C++17。在那之前，類別模板無法推導參數，只有函式模板可以，
//         所以標準函式庫提供一整批 make_ 工廠函式
//         （make_pair / make_tuple / make_shared）借用函式模板的推導能力。
//         C++17 之後這些函式大多只剩相容性用途——
//         例外是 std::make_shared，它還負責把控制區塊與物件合併成一次配置。
//     追問：怎麼驗證某個語法屬於哪個標準？
//         → 用 g++ -std=c++14 -pedantic-errors 編譯看它拒不拒絕。
//           只用 -fsyntax-only 不夠，GCC 會把很多新特性當擴充放行。
//
// 🔥 Q2. `std::vector<int> src = {1,2,3}; std::vector a{src};`
//        a 的型別是什麼？
//     答：vector<int>，內容是 src 的複本（三個元素 1,2,3）。
//         不是 vector<vector<int>>。因為重載解析時複製建構子
//         勝過 initializer_list 建構子。
//     追問：那 std::vector a{src, src}; 呢？
//         → 兩個元素，型別是 vector<vector<int>>。
//           兩個引數無法匹配複製建構子，才走 initializer_list。
//
// ⚠️ 陷阱. `std::vector v = {1, 2, 3.0};` 為什麼編譯失敗？
//         明明 int 可以隱式轉成 double，編譯器自動選 double 不就好了？
//     答：CTAD 的推導不做「找一個大家都能轉過去的共同型別」這件事。
//         它把 initializer_list<T> 的 T 拿三個元素分別去推，
//         得到 int、int、double 三個互相矛盾的結果 → 推導失敗 → 編譯錯誤。
//         想要 double 就自己寫明：std::vector<double> v = {1, 2, 3.0};
//     為什麼會錯：把 CTAD 想成「智慧型別選擇」。它其實只是
//         模板參數推導，規則是「所有推導結果必須完全一致」，
//         一旦不一致就放棄，不會試圖調解。
//         auto x = {1, 2, 3.0}; 失敗的原因完全相同。
// ═══════════════════════════════════════════════════════════════════════════

/*
# 第二階段：序列容器 — vector

## 第 10 課：vector 的宣告與初始化方式

---

### 核心概念

C++ 提供了多種初始化 vector 的方式，每種都有其適用場景。理解這些方式的差異，能讓你寫出更清晰、更有效率的程式碼。

---

### 一、預設建構：空的 vector

```cpp
#include <vector>

int main() {
    std::vector<int> v1;           // 空的 vector，size=0，capacity=0
    std::vector<double> v2{};      // 同上，C++11 統一初始化語法
    std::vector<std::string> v3 = {};  // 同上
    
    return 0;
}
```

這三種寫法效果相同，建立一個不含任何元素的 vector。

---

### 二、指定大小的建構

```cpp
#include <vector>
#include <iostream>

int main() {
    // 建立含有 5 個元素的 vector，每個元素都是預設值（int 的預設值是 0）
    std::vector<int> v1(5);
    
    for (int x : v1) {
        std::cout << x << " ";  // 輸出：0 0 0 0 0
    }
    std::cout << std::endl;
    
    // 建立含有 5 個元素的 vector，每個元素都是 42
    std::vector<int> v2(5, 42);
    
    for (int x : v2) {
        std::cout << x << " ";  // 輸出：42 42 42 42 42
    }
    std::cout << std::endl;
    
    return 0;
}
```

**注意**：這裡用的是小括號 `()`，不是大括號 `{}`。這個差異很重要，稍後會詳細說明。

---

### 三、初始化串列（Initializer List）

C++11 引入的大括號初始化，可以直接列出元素：

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v1 = {1, 2, 3, 4, 5};    // 明確的初始化串列
    std::vector<int> v2{1, 2, 3, 4, 5};       // 同上，省略等號
    std::vector<int> v3({1, 2, 3, 4, 5});     // 同上，但較少人這樣寫
    
    for (int x : v1) {
        std::cout << x << " ";  // 輸出：1 2 3 4 5
    }
    std::cout << std::endl;
    
    return 0;
}
```

---

### 四、小括號 vs 大括號的陷阱

這是初學者最容易混淆的地方：

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v1(5, 10);   // 5 個元素，每個都是 10
    std::vector<int> v2{5, 10};   // 2 個元素，分別是 5 和 10
    
    std::cout << "v1 size: " << v1.size() << std::endl;  // 輸出：5
    std::cout << "v2 size: " << v2.size() << std::endl;  // 輸出：2
    
    std::cout << "v1: ";
    for (int x : v1) std::cout << x << " ";  // 輸出：10 10 10 10 10
    std::cout << std::endl;
    
    std::cout << "v2: ";
    for (int x : v2) std::cout << x << " ";  // 輸出：5 10
    std::cout << std::endl;
    
    return 0;
}
```

**規則**：
- `()` 呼叫建構子，`(5, 10)` 意思是「5 個元素，值為 10」
- `{}` 優先嘗試初始化串列，`{5, 10}` 意思是「元素是 5 和 10」

---

### 五、從其他容器或範圍初始化

```cpp
#include <vector>
#include <array>
#include <iostream>

int main() {
    // 從另一個 vector 複製
    std::vector<int> original = {1, 2, 3, 4, 5};
    std::vector<int> copy1(original);           // 複製建構
    std::vector<int> copy2 = original;          // 同上
    
    // 從迭代器範圍建構
    std::vector<int> partial(original.begin() + 1, original.begin() + 4);
    for (int x : partial) {
        std::cout << x << " ";  // 輸出：2 3 4
    }
    std::cout << std::endl;
    
    // 從 C 風格陣列建構
    int arr[] = {10, 20, 30, 40};
    std::vector<int> from_array(std::begin(arr), std::end(arr));
    // 或者：std::vector<int> from_array(arr, arr + 4);
    
    // 從 std::array 建構
    std::array<int, 3> std_arr = {100, 200, 300};
    std::vector<int> from_std_array(std_arr.begin(), std_arr.end());
    
    return 0;
}
```

---

### 六、移動建構

當來源 vector 不再需要時，可以「移動」而非「複製」：

```cpp
#include <vector>
#include <iostream>
#include <utility>  // for std::move

int main() {
    std::vector<int> source = {1, 2, 3, 4, 5};
    
    std::cout << "移動前 source size: " << source.size() << std::endl;
    
    std::vector<int> dest = std::move(source);  // 移動建構
    
    std::cout << "移動後 source size: " << source.size() << std::endl;  // 通常是 0
    std::cout << "移動後 dest size: " << dest.size() << std::endl;      // 5
    
    return 0;
}
```

移動後，`source` 處於「有效但未指定」的狀態，通常是空的。移動操作只是轉移指標的所有權，不會複製元素，效率很高。

---

### 七、使用 assign 重新初始化

已存在的 vector 可以用 `assign` 重新設定內容：

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3};
    
    // 方法一：指定數量和值
    v.assign(5, 100);
    std::cout << "assign(5, 100): ";
    for (int x : v) std::cout << x << " ";  // 輸出：100 100 100 100 100
    std::cout << std::endl;
    
    // 方法二：從初始化串列
    v.assign({10, 20, 30});
    std::cout << "assign({10, 20, 30}): ";
    for (int x : v) std::cout << x << " ";  // 輸出：10 20 30
    std::cout << std::endl;
    
    // 方法三：從迭代器範圍
    std::vector<int> other = {7, 8, 9, 10, 11};
    v.assign(other.begin() + 1, other.end() - 1);
    std::cout << "從迭代器範圍: ";
    for (int x : v) std::cout << x << " ";  // 輸出：8 9 10
    std::cout << std::endl;
    
    return 0;
}
```

---

### 八、自訂類別的 vector 初始化

```cpp
#include <vector>
#include <string>
#include <iostream>

struct Person {
    std::string name;
    int age;
    
    Person(const std::string& n, int a) : name(n), age(a) {}
};

int main() {
    // 使用初始化串列（需要隱式轉換或大括號建構）
    std::vector<Person> people = {
        {"Alice", 30},
        {"Bob", 25},
        {"Charlie", 35}
    };
    
    for (const auto& p : people) {
        std::cout << p.name << " is " << p.age << " years old." << std::endl;
    }
    
    return 0;
}
```

---

### 九、C++17 類別模板引數推導（CTAD）

C++17 開始，某些情況下可以省略模板參數：

```cpp
#include <vector>
#include <iostream>

int main() {
    // C++17 之前必須寫：std::vector<int> v = {1, 2, 3};
    // C++17 可以讓編譯器推導：
    std::vector v1 = {1, 2, 3};       // 推導為 vector<int>
    std::vector v2 = {1.5, 2.5, 3.5}; // 推導為 vector<double>
    
    // 從迭代器推導
    std::vector<int> source = {10, 20, 30};
    std::vector v3(source.begin(), source.end());  // 推導為 vector<int>
    
    return 0;
}
```

**注意**：CTAD 有時會產生意外結果，建議在型別不明顯時還是明確寫出來。

---

### 各種初始化方式對照表

| 語法 | 結果 | 說明 |
|------|------|------|
| `vector<int> v;` | 空 vector | 預設建構 |
| `vector<int> v(5);` | `{0,0,0,0,0}` | 5 個預設值元素 |
| `vector<int> v(5, 42);` | `{42,42,42,42,42}` | 5 個值為 42 的元素 |
| `vector<int> v{5, 42};` | `{5, 42}` | 2 個元素：5 和 42 |
| `vector<int> v = {1,2,3};` | `{1, 2, 3}` | 初始化串列 |
| `vector<int> v(other);` | 複製 other | 複製建構 |
| `vector<int> v(std::move(other));` | 接收 other 的資源 | 移動建構 |
| `vector<int> v(it1, it2);` | 範圍 [it1, it2) | 迭代器範圍建構 |

---

### 練習題

1. **辨識題**：以下程式碼建立的 vector 各有幾個元素？值分別是什麼？
   ```cpp
   std::vector<int> a(3);
   std::vector<int> b(3, 7);
   std::vector<int> c{3};
   std::vector<int> d{3, 7};
   std::vector<int> e(3, 7, 9);  // 這行能編譯嗎？
   ```

2. **實作題**：建立一個 `vector<double>`，包含 1.0 到 10.0 的等差數列（共 10 個元素）。用兩種不同的方式完成。

3. **思考題**：假設你有一個函數回傳 `vector<string>`，在呼叫端接收時，用 `auto result = func();` 和 `auto&& result = func();` 有什麼差異？（提示：考慮 C++17 的保證複製消除）

---

下一課我們講 **vector 的容量管理：size、capacity、reserve**，這是控制 vector 記憶體行為的關鍵。

準備好繼續嗎？
*/



#include <vector>
#include <iostream>
#include <string>
#include <list>
#include <type_traits>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔刻意不放
//   理由：CTAD 純粹是「宣告變數時少打幾個字」的語法便利，
//   不改變任何執行期行為，也不影響演算法的正確性或複雜度。
//   LeetCode 的評分只看輸入輸出，沒有一題的難點會落在型別推導上
//   （實際上 LeetCode 的函式簽章都由平台給定、型別全部寫死，
//     連用 CTAD 的機會都沒有）。
//   硬掛一題只會讓讀者誤以為 CTAD 有演算法上的意義，所以從缺。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】用 CTAD 讓「查表初始化」的宣告不再囉唆
//   情境：程式裡常有一批寫死的對照表——HTTP 狀態碼說明、
//         副檔名對 MIME type、錯誤碼對訊息。
//   為什麼用到本主題：這種表的型別從初始值一眼可知，
//     把 std::vector<std::pair<int, std::string>> 完整寫出來只是噪音。
//     CTAD 正是為這種場合設計的。
//   分寸：下面同時示範「該用」與「不該用」——狀態表用 CTAD 很清爽；
//     但需要明確控制型別（例如要 double 而初始值全寫成整數）時，
//     就必須寫明，不能依賴推導。
// -----------------------------------------------------------------------------
void demoConfigTables() {
    using namespace std::string_literals;   // 讓 "..."s 產生 std::string

    // CTAD：推導為 vector<pair<int, string>>，一眼可知，不必寫型別
    std::vector statusTable = {
        std::pair{200, "OK"s},
        std::pair{404, "Not Found"s},
        std::pair{500, "Internal Server Error"s},
    };

    std::cout << "HTTP 狀態表（CTAD 推導）:\n";
    for (const auto& [code, text] : statusTable) {     // 結構化繫結，C++17
        std::cout << "  " << code << " -> " << text << "\n";
    }

    // 反例：想要 double，但初始值都寫成整數 → CTAD 會推成 vector<int>
    std::vector wrong = {1, 2, 3};              // vector<int>
    std::vector<double> right = {1, 2, 3};      // 明確指定 → vector<double>
    std::cout << "CTAD  {1,2,3} 的元素是 double 嗎: " << std::boolalpha
              << std::is_same_v<decltype(wrong)::value_type, double> << "\n";
    std::cout << "明確寫 vector<double> 的元素是 double 嗎: "
              << std::is_same_v<decltype(right)::value_type, double> << "\n";
    std::cout << "→ 型別不是「一眼可知」時就該寫明，別依賴 CTAD\n";
}

int main() {
    // C++17 之前必須寫：std::vector<int> v = {1, 2, 3};
    // C++17 可以讓編譯器推導：
    std::vector v1 = {1, 2, 3};       // 推導為 vector<int>
    std::vector v2 = {1.5, 2.5, 3.5}; // 推導為 vector<double>

    // 從迭代器推導（靠標準函式庫提供的推導指引，取 iterator 的 value_type）
    std::vector<int> source = {10, 20, 30};
    std::vector v3(source.begin(), source.end());  // 推導為 vector<int>

    std::cout << "=== CTAD 基本推導（C++17）===\n";
    std::cout << std::boolalpha;
    std::cout << "v1 = {1, 2, 3}         推導成 vector<int>?    "
              << std::is_same_v<decltype(v1), std::vector<int>>
              << "  size=" << v1.size() << "\n";
    std::cout << "v2 = {1.5, 2.5, 3.5}   推導成 vector<double>? "
              << std::is_same_v<decltype(v2), std::vector<double>>
              << "  size=" << v2.size() << "\n";
    std::cout << "v3(begin, end)         推導成 vector<int>?    "
              << std::is_same_v<decltype(v3), std::vector<int>>
              << "  size=" << v3.size() << "\n";

    std::cout << "\n=== 從別種容器的迭代器推導 ===\n";
    {
        std::list<std::string> names = {"alice", "bob", "carol"};
        std::vector fromList(names.begin(), names.end());   // 推導為 vector<string>
        std::cout << "從 list<string> 的迭代器推導成 vector<string>? "
                  << std::is_same_v<decltype(fromList), std::vector<std::string>> << "\n";
        std::cout << "內容: ";
        for (const auto& s : fromList) std::cout << s << " ";
        std::cout << "\n";
    }

    std::cout << "\n=== 陷阱：大括號包住另一個 vector，推導出什麼？ ===\n";
    {
        std::vector<int> src = {1, 2, 3};

        std::vector a{src};        // 直覺：vector<vector<int>>？
        std::vector b(src);        // 明顯是複製建構

        std::cout << "std::vector a{src} 是 vector<int>（= src 的複本）嗎? "
                  << std::is_same_v<decltype(a), std::vector<int>> << "\n";
        std::cout << "  a.size()=" << a.size() << " 內容: ";
        for (int x : a) std::cout << x << " ";
        std::cout << "\n";
        std::cout << "  → 不是 vector<vector<int>>！複製建構子在重載解析中\n";
        std::cout << "    勝過 initializer_list 建構子，所以 a{src} 等同 a(src)。\n";
        std::cout << "std::vector b(src) 的 size=" << b.size() << "（兩者一致）\n";

        // 兩個引數無法匹配複製建構子，這時才走 initializer_list
        std::vector c{src, src};
        std::cout << "std::vector c{src, src} 是 vector<vector<int>> 嗎? "
                  << std::is_same_v<decltype(c), std::vector<std::vector<int>>> << "\n";
        std::cout << "  c.size()=" << c.size() << "（兩個元素，每個都是一整個 vector）\n";
    }

    std::cout << "\n=== 推導失敗的情況（已註解，寫出來會編譯錯誤）===\n";
    std::cout << "std::vector v = {1, 2, 3.0};   // 錯誤：推出 int/int/double，不唯一\n";
    std::cout << "CTAD 不會自動找共同型別，推導結果不一致就直接放棄。\n";
    std::cout << "要 double 請自己寫明: std::vector<double> v = {1, 2, 3.0};\n";
    {
        std::vector<double> ok = {1, 2, 3.0};   // 明確指定就沒問題
        std::cout << "明確指定的版本 size=" << ok.size() << " 首元素=" << ok[0] << "\n";
    }

    std::cout << "\n=== 日常實務：查表初始化該不該用 CTAD ===\n";
    demoConfigTables();

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 10 課：vector 的宣告與初始化方式9.cpp -o demo9
// ⚠️ 本檔必須用 C++17（或更新）—— CTAD 是 C++17 特性。
//    以 g++ -std=c++14 -pedantic-errors 編譯會被明確拒絕。

// === 預期輸出 ===
// === CTAD 基本推導（C++17）===
// v1 = {1, 2, 3}         推導成 vector<int>?    true  size=3
// v2 = {1.5, 2.5, 3.5}   推導成 vector<double>? true  size=3
// v3(begin, end)         推導成 vector<int>?    true  size=3
//
// === 從別種容器的迭代器推導 ===
// 從 list<string> 的迭代器推導成 vector<string>? true
// 內容: alice bob carol
//
// === 陷阱：大括號包住另一個 vector，推導出什麼？ ===
// std::vector a{src} 是 vector<int>（= src 的複本）嗎? true
//   a.size()=3 內容: 1 2 3
//   → 不是 vector<vector<int>>！複製建構子在重載解析中
//     勝過 initializer_list 建構子，所以 a{src} 等同 a(src)。
// std::vector b(src) 的 size=3（兩者一致）
// std::vector c{src, src} 是 vector<vector<int>> 嗎? true
//   c.size()=2（兩個元素，每個都是一整個 vector）
//
// === 推導失敗的情況（已註解，寫出來會編譯錯誤）===
// std::vector v = {1, 2, 3.0};   // 錯誤：推出 int/int/double，不唯一
// CTAD 不會自動找共同型別，推導結果不一致就直接放棄。
// 要 double 請自己寫明: std::vector<double> v = {1, 2, 3.0};
// 明確指定的版本 size=3 首元素=1
//
// === 日常實務：查表初始化該不該用 CTAD ===
// HTTP 狀態表（CTAD 推導）:
//   200 -> OK
//   404 -> Not Found
//   500 -> Internal Server Error
// CTAD  {1,2,3} 的元素是 double 嗎: false
// 明確寫 vector<double> 的元素是 double 嗎: true
// → 型別不是「一眼可知」時就該寫明，別依賴 CTAD
