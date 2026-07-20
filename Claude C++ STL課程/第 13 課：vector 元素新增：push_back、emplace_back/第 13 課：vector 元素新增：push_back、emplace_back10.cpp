// =============================================================================
//  第 13 課：vector 元素新增：push_back、emplace_back10.cpp
//    —  本課完整講義（含對照表與練習題）＋ 強例外保證可執行範例
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<vector>
//
//   void push_back(const T& value);                 // (1) C++03 起，複製
//   void push_back(T&& value);                      // (2) C++11 起，移動
//
//   template<class... Args>
//   void      emplace_back(Args&&... args);         // C++11 / C++14：回傳 void
//   template<class... Args>
//   reference emplace_back(Args&&... args);         // C++17 起：回傳新元素參考
//
//   複雜度：兩者皆為**攤銷 O(1)**（amortized constant）。
//           單次呼叫最壞是 O(n)——觸發 reallocation 時要搬走全部既有元素；
//           但成長是等比級數，連續 n 次的總成本是 O(n)，攤平後每次是常數。
//
//   成長倍率：**實作定義，標準未規定**。
//           libstdc++ / libc++ = 2 倍（本機實測 capacity 序列 1,2,4,8,16,32,64）；
//           MSVC STL = 1.5 倍。標準只規定攤銷常數複雜度。
//           任何依賴具體 capacity 數值的程式碼都不可攜。
//
//   例外保證：strong exception guarantee（強例外保證）——
//           操作失敗時 vector 保持呼叫前的狀態。
//           但這個保證有前提：元素的 move constructor 必須是 noexcept，
//           否則 reallocation 會透過 std::move_if_noexcept **退化成 copy**。
//           詳見本檔尾端的【面試題】Q3 與可執行的 MayThrow 範例。
//
//   失效規則：一旦發生 reallocation，該 vector 的**所有** iterator / pointer /
//           reference（含 end()）全部失效；未 reallocation 時既有元素的
//           iterator 仍有效，但 end() 一定失效。
//
// 【本檔結構說明】
//   緊接在下方的區塊是本課的**完整講義**（九個章節 + 兩張對照表 + 練習題），
//   請直接閱讀。講義之後接的是可實際編譯執行的「強例外保證」示範程式，
//   以及檔尾的【面試題】與【日常實務範例】。
//
// 【詳細解釋 Explanation】
//   ★ 逐節說明在下方講義本體，此處只補三個貫穿全課的主軸：
//
//   【1. 攤銷 O(1) 到底在說什麼】
//      單次 push_back 最壞是 O(n)（要重新配置並搬走全部元素），
//      但因為容量是**成倍**成長，昂貴的那一次會被之後大量便宜的 push_back 分攤，
//      n 次 push_back 的總成本是 O(n)、平均每次 O(1)。
//      標準規定的是這個**攤銷**上界，**不是**每次都 O(1)，
//      也**沒有**規定倍率要是多少。
//
//   【2. emplace_back 的價值不在「比較快」，在「不需要先有物件」】
//      當你手上已經是一個同型別物件時，push_back 與 emplace_back 都要做一次
//      copy/move，效能幾乎相同 ——「emplace_back 一定比較快」是錯誤的通說。
//      真正的差別是 emplace_back 把參數**完美轉發**給建構子做直接初始化，
//      因此能就地建構、能呼叫 explicit 建構子、也不做 narrowing 檢查。
//
//   【3. 為什麼「移動建構子請標 noexcept」是硬性建議】
//      reallocation 時 vector 要把舊元素搬到新空間。若搬到一半拋例外，
//      舊空間已被破壞、新空間不完整，就無法回復原狀 —— 強例外保證會失守。
//      所以 vector 透過 std::move_if_noexcept 判斷：move 若不是 noexcept，
//      寧可**退化成 copy**（慢但可回復）。這就是漏標 noexcept 會讓效能悄悄
//      掉一個量級的原因，而且編譯器不會警告你。
//
// 【概念補充 Concept Deep Dive】
//   一次觸發成長的 push_back，實際順序是：
//     1) 配置一塊更大的原始記憶體（只配置，尚未建構任何物件）
//     2) 在新空間的尾端**先**就地建構新元素（placement new）
//     3) 把舊元素逐一 move（或 copy，見上）到新空間
//     4) 反向銷毀舊元素、釋放舊記憶體
//   第 2 步排在第 3 步之前很關鍵：這讓 v.push_back(v[0]) 這種
//   **自我參照**插入也能正確運作 —— 新元素在舊資料被破壞前就已完成建構。
//   另外注意 capacity 是「已配置但尚未建構物件」的空間，
//   所以 size() 與 capacity() 之間那段記憶體裡並沒有合法物件，
//   直接存取 v[size()] 是未定義行為。
//
// 【注意事項 Pay Attention】
//   1. 成長倍率、以及任何具體 capacity 數值都是**實作定義**
//      （libstdc++ 實測 2×、MSVC 為 1.5×），標準只規定攤銷 O(1)。
//      不要寫出依賴特定 capacity 數值的程式碼或測試。
//   2. 失效規則：發生 reallocation → 所有 iterator/pointer/reference 全失效；
//      未 reallocation → 既有元素的仍有效，但 end() 一定失效。
//   3. emplace_back 的回傳型別：**C++11/14 為 void，C++17 起為 reference**
//      （本機以 -pedantic-errors 實測確認）。
//   4. emplace_back 不做 narrowing 檢查、可呼叫 explicit 建構子，
//      參數寫錯有機會安靜編譯通過，要靠 code review 補回這層檢查。
//   5. 若元素型別的 move constructor 未標 noexcept，reallocation 會退化成 copy；
//      這是效能問題，不是正確性問題，且不會有任何編譯警告。
//
// =============================================================================

/*
# 第二階段：序列容器 — vector

## 第 13 課：vector 元素新增：push_back、emplace_back

---

### 核心概念

在 vector 尾端新增元素是最常見的操作。C++11 之後有兩種主要方式：`push_back` 和 `emplace_back`。理解它們的差異，能幫助你寫出更高效的程式碼。

---

### 一、push_back：複製或移動元素進入容器

```cpp
#include <vector>
#include <iostream>
#include <string>

int main() {
    std::vector<int> v;
    
    // 基本用法
    v.push_back(10);
    v.push_back(20);
    v.push_back(30);
    
    for (int x : v) {
        std::cout << x << " ";  // 10 20 30
    }
    std::cout << std::endl;
    
    // 對於字串
    std::vector<std::string> words;
    
    std::string hello = "Hello";
    words.push_back(hello);           // 複製 hello 進入 vector
    words.push_back("World");         // 從字面量建立臨時物件，然後移動進入
    words.push_back(std::move(hello)); // 移動 hello 進入 vector
    
    std::cout << "hello 變成: \"" << hello << "\"" << std::endl;  // 可能是空的
    
    for (const auto& w : words) {
        std::cout << w << " ";  // Hello World Hello
    }
    std::cout << std::endl;
    
    return 0;
}
```

---

### 二、觀察 push_back 的複製與移動

```cpp
#include <vector>
#include <iostream>

class Tracker {
public:
    int id;
    
    Tracker(int i) : id(i) {
        std::cout << "建構 Tracker(" << id << ")" << std::endl;
    }
    
    Tracker(const Tracker& other) : id(other.id) {
        std::cout << "複製建構 Tracker(" << id << ")" << std::endl;
    }
    
    Tracker(Tracker&& other) noexcept : id(other.id) {
        std::cout << "移動建構 Tracker(" << id << ")" << std::endl;
    }
    
    ~Tracker() {
        std::cout << "銷毀 Tracker(" << id << ")" << std::endl;
    }
};

int main() {
    std::vector<Tracker> v;
    v.reserve(5);  // 預留空間，避免擴容干擾觀察
    
    std::cout << "=== 從左值 push_back ===" << std::endl;
    Tracker t1(1);
    v.push_back(t1);  // 複製
    
    std::cout << "\n=== 從右值 push_back ===" << std::endl;
    v.push_back(Tracker(2));  // 建構臨時物件，然後移動
    
    std::cout << "\n=== 用 std::move push_back ===" << std::endl;
    Tracker t3(3);
    v.push_back(std::move(t3));  // 移動
    
    std::cout << "\n=== 程式結束 ===" << std::endl;
    return 0;
}
```

輸出：
```
=== 從左值 push_back ===
建構 Tracker(1)
複製建構 Tracker(1)

=== 從右值 push_back ===
建構 Tracker(2)
移動建構 Tracker(2)
銷毀 Tracker(2)

=== 用 std::move push_back ===
建構 Tracker(3)
移動建構 Tracker(3)

=== 程式結束 ===
銷毀 Tracker(3)
銷毀 Tracker(1)
銷毀 Tracker(3)
銷毀 Tracker(2)
銷毀 Tracker(1)
```

---

### 三、emplace_back：就地建構

`emplace_back` 直接在 vector 的記憶體空間內建構物件，**不需要先建立再複製/移動**：

```cpp
#include <vector>
#include <iostream>

class Tracker {
public:
    int id;
    
    Tracker(int i) : id(i) {
        std::cout << "建構 Tracker(" << id << ")" << std::endl;
    }
    
    Tracker(const Tracker& other) : id(other.id) {
        std::cout << "複製建構 Tracker(" << id << ")" << std::endl;
    }
    
    Tracker(Tracker&& other) noexcept : id(other.id) {
        std::cout << "移動建構 Tracker(" << id << ")" << std::endl;
    }
    
    ~Tracker() {
        std::cout << "銷毀 Tracker(" << id << ")" << std::endl;
    }
};

int main() {
    std::vector<Tracker> v;
    v.reserve(3);
    
    std::cout << "=== emplace_back ===" << std::endl;
    v.emplace_back(1);  // 直接在 vector 內部建構，只有一次建構
    
    std::cout << "\n=== 對比 push_back ===" << std::endl;
    v.push_back(Tracker(2));  // 建構 + 移動 + 銷毀臨時物件
    
    std::cout << "\n=== 程式結束 ===" << std::endl;
    return 0;
}
```

輸出：
```
=== emplace_back ===
建構 Tracker(1)

=== 對比 push_back ===
建構 Tracker(2)
移動建構 Tracker(2)
銷毀 Tracker(2)

=== 程式結束 ===
銷毀 Tracker(2)
銷毀 Tracker(1)
```

可以看到 `emplace_back` 只有一次建構，沒有額外的移動和銷毀。

---

### 四、多參數建構的優勢

`emplace_back` 可以接受任意數量的參數，直接傳給元素的建構子：

```cpp
#include <vector>
#include <iostream>
#include <string>

struct Person {
    std::string name;
    int age;
    double height;
    
    Person(const std::string& n, int a, double h)
        : name(n), age(a), height(h) {
        std::cout << "建構 Person: " << name << std::endl;
    }
    
    Person(const Person& other)
        : name(other.name), age(other.age), height(other.height) {
        std::cout << "複製 Person: " << name << std::endl;
    }
    
    Person(Person&& other) noexcept
        : name(std::move(other.name)), age(other.age), height(other.height) {
        std::cout << "移動 Person: " << name << std::endl;
    }
};

int main() {
    std::vector<Person> people;
    people.reserve(3);
    
    std::cout << "=== 使用 emplace_back ===" << std::endl;
    people.emplace_back("Alice", 30, 165.5);  // 直接建構
    
    std::cout << "\n=== 使用 push_back ===" << std::endl;
    people.push_back(Person("Bob", 25, 175.0));  // 建構 + 移動
    
    std::cout << "\n=== 使用大括號 push_back ===" << std::endl;
    people.push_back({"Charlie", 35, 180.0});  // 建構 + 移動
    
    std::cout << "\n=== 結束 ===" << std::endl;
    return 0;
}
```

---

### 五、C++17 的回傳值改進

C++17 之後，`emplace_back` 會回傳新元素的參考：

```cpp
#include <vector>
#include <iostream>
#include <string>

struct Item {
    std::string name;
    int quantity;
    
    Item(const std::string& n, int q) : name(n), quantity(q) {}
};

int main() {
    std::vector<Item> items;
    items.reserve(3);
    
    // C++17：emplace_back 回傳新元素的參考
    Item& apple = items.emplace_back("Apple", 10);
    apple.quantity += 5;  // 直接修改
    
    // 鏈式操作
    items.emplace_back("Banana", 20).quantity *= 2;
    
    for (const auto& item : items) {
        std::cout << item.name << ": " << item.quantity << std::endl;
    }
    // Apple: 15
    // Banana: 40
    
    return 0;
}
```

**注意**：`push_back` 沒有回傳值（回傳 void）。

---

### 六、什麼時候用哪個？

```cpp
#include <vector>
#include <string>

int main() {
    std::vector<std::string> v;
    v.reserve(10);
    
    // 情況 1：已有物件，需要保留原物件
    std::string s1 = "Hello";
    v.push_back(s1);  // 複製，s1 仍然有效
    // 這種情況 emplace_back(s1) 效果相同
    
    // 情況 2：已有物件，不再需要原物件
    std::string s2 = "World";
    v.push_back(std::move(s2));  // 移動
    // 這種情況 emplace_back(std::move(s2)) 效果相同
    
    // 情況 3：從字面量或臨時值
    v.push_back("Foo");     // 建構臨時 string，然後移動
    v.emplace_back("Bar");  // 直接就地建構，較優
    
    // 情況 4：需要多個參數建構
    // push_back 需要先建構物件
    // emplace_back 可以直接傳參數
    
    return 0;
}
```

---

### 七、emplace_back 的陷阱

#### 陷阱一：隱式轉換可能不如預期

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<std::vector<int>> vv;
    
    // 想要加入一個含有 5 個元素的 vector
    // 錯誤理解：emplace_back(5) 會建立 vector<int>(5)，即 5 個 0
    vv.emplace_back(5);
    
    std::cout << "內層 size: " << vv[0].size() << std::endl;  // 5
    std::cout << "內層內容: ";
    for (int x : vv[0]) std::cout << x << " ";  // 0 0 0 0 0
    std::cout << std::endl;
    
    // 如果想要一個元素是 5 的 vector？
    vv.push_back({5});  // 這樣才對
    
    std::cout << "第二個內層 size: " << vv[1].size() << std::endl;  // 1
    std::cout << "第二個內層內容: ";
    for (int x : vv[1]) std::cout << x << " ";  // 5
    std::cout << std::endl;
    
    return 0;
}
```

#### 陷阱二：emplace_back 不一定更快

```cpp
#include <vector>
#include <string>

int main() {
    std::vector<std::string> v;
    v.reserve(10);
    
    std::string s = "Hello";
    
    // 這兩個效能幾乎相同
    v.push_back(s);     // 複製
    v.emplace_back(s);  // 也是複製！因為 s 是左值
    
    // 這兩個效能幾乎相同
    v.push_back(std::move(s));     // 移動
    v.emplace_back(std::move(s));  // 也是移動
    
    return 0;
}
```

當傳入的是**已存在的物件**時，兩者效能相同。`emplace_back` 的優勢在於可以**省略臨時物件的建構**。

---

### 八、完美轉發的運作原理（進階）

`emplace_back` 使用完美轉發（perfect forwarding）將參數傳給建構子：

```cpp
#include <vector>
#include <iostream>
#include <string>

struct Widget {
    std::string data;
    
    Widget(const std::string& s) : data(s) {
        std::cout << "從 const string& 建構" << std::endl;
    }
    
    Widget(std::string&& s) : data(std::move(s)) {
        std::cout << "從 string&& 建構" << std::endl;
    }
};

int main() {
    std::vector<Widget> v;
    v.reserve(5);
    
    std::string s = "Hello";
    
    std::cout << "emplace_back(s):" << std::endl;
    v.emplace_back(s);  // 呼叫 const string& 版本
    
    std::cout << "\nemplace_back(std::move(s)):" << std::endl;
    v.emplace_back(std::move(s));  // 呼叫 string&& 版本
    
    std::cout << "\nemplace_back(\"World\"):" << std::endl;
    v.emplace_back("World");  // 呼叫 string&& 版本（從字面量建構臨時 string）
    
    return 0;
}
```

---

### 九、例外安全性

`push_back` 和 `emplace_back` 都提供強例外保證（strong exception guarantee）：如果操作失敗，vector 保持原狀。

```cpp
#include <vector>
#include <iostream>
#include <stdexcept>

struct MayThrow {
    int value;
    static int count;
    
    MayThrow(int v) : value(v) {
        ++count;
        if (count == 3) {
            throw std::runtime_error("第三個物件建構失敗");
        }
        std::cout << "建構 MayThrow(" << value << ")" << std::endl;
    }
    
    MayThrow(const MayThrow& other) : value(other.value) {
        std::cout << "複製 MayThrow(" << value << ")" << std::endl;
    }
    
    MayThrow(MayThrow&& other) noexcept : value(other.value) {
        std::cout << "移動 MayThrow(" << value << ")" << std::endl;
    }
};

int MayThrow::count = 0;

int main() {
    std::vector<MayThrow> v;
    v.reserve(5);
    
    try {
        v.emplace_back(1);
        v.emplace_back(2);
        v.emplace_back(3);  // 這裡會拋出例外
        v.emplace_back(4);
    }
    catch (const std::exception& e) {
        std::cout << "捕捉例外: " << e.what() << std::endl;
    }
    
    std::cout << "vector 大小: " << v.size() << std::endl;  // 2
    
    return 0;
}
```

---

### push_back vs emplace_back 對照表

| 特性 | push_back | emplace_back |
|------|-----------|--------------|
| 參數 | 接受物件 | 接受建構子參數 |
| 運作方式 | 複製或移動物件進入 | 就地建構物件 |
| 適用場景 | 已有物件要加入 | 需要建構新物件 |
| 回傳值 | void | T&（C++17） |
| 效能 | 左值複製，右值移動 | 避免臨時物件 |

---

### 使用建議

| 場景 | 建議 |
|------|------|
| 加入已存在的左值物件 | `push_back` 或 `emplace_back` 皆可 |
| 加入已存在物件且不再需要 | `push_back(std::move(obj))` |
| 從字面量或參數建構新物件 | `emplace_back(args...)` |
| 程式碼清晰度優先 | `push_back` 意圖更明確 |
| 效能關鍵路徑 | 優先考慮 `emplace_back` |

---

### 練習題

1. **預測題**：以下程式碼會輸出什麼？
   ```cpp
   std::vector<std::pair<int, int>> v;
   v.reserve(3);
   v.push_back({1, 2});
   v.emplace_back(3, 4);
   v.push_back(std::make_pair(5, 6));
   ```

2. **改寫題**：將以下程式碼改用 `emplace_back`，使效能最佳化：
   ```cpp
   struct Point { double x, y, z; Point(double a, double b, double c) : x(a), y(b), z(c) {} };
   std::vector<Point> points;
   points.push_back(Point(1.0, 2.0, 3.0));
   points.push_back(Point(4.0, 5.0, 6.0));
   ```

3. **思考題**：為什麼 `emplace_back` 回傳參考（C++17），而 `push_back` 回傳 void？加入回傳值有什麼潛在的風險？

4. **除錯題**：以下程式碼有什麼潛在問題？
   ```cpp
   std::vector<std::unique_ptr<int>> v;
   auto p = std::make_unique<int>(42);
   v.push_back(p);
   ```

---

下一課我們講 **vector 元素插入：insert、emplace**，學習在任意位置插入元素的方法。

準備好繼續嗎？
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】push_back / emplace_back 綜合（本課總整理）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. push_back 的複雜度是 O(1) 嗎？成長倍率是 2 倍嗎？
//     答：是**攤銷 O(1)**，不是每次都 O(1)——觸發 reallocation 的那一次是
//         O(n)。因為成長是等比級數，n 次操作的總搬移量是 O(n)，攤平後
//         每次是常數。倍率則是**實作定義，標準沒有規定**：
//         libstdc++ / libc++ 是 2 倍（本機實測 capacity 序列 1,2,4,8,16,32,64），
//         MSVC 是 1.5 倍。標準只要求攤銷常數複雜度。
//     追問：為什麼不能用「每次加固定值」的成長策略？
//         → 總搬移量會變成 1+2+...+n = O(n^2)，攤銷後每次仍是 O(n)，
//           等比成長才能讓總量收斂到 O(n)。
//
// ⚠️ 陷阱 Q2. 「emplace_back 永遠比 push_back 快」——這句話錯在哪？
//     答：錯在「永遠」。當傳入的已經是現成的同型別物件時，兩者**完全等價**：
//             v.push_back(s);      // 複製
//             v.emplace_back(s);   // 也是複製！s 是 lvalue
//         emplace_back 只有在「原本需要先建構一個臨時物件」時才勝出
//         （例如 emplace_back("Bar") 或多參數建構）。
//     為什麼會錯：以為「就地建構 = 不會複製」，
//         漏掉 std::forward 會**保留參數的值類別**——傳 lvalue 進去，
//         轉發出來還是 lvalue，最後呼叫的依然是複製建構子。
//         emplace_back 省的是「臨時物件」，不是「複製」本身。
//
// 🔥 Q3. 講義第九節說 push_back / emplace_back 提供強例外保證，
//         這個保證有什麼前提？
//     答：前提是元素的 **move constructor 必須是 noexcept**。
//         reallocation 搬移元素到一半若拋出例外就無法回復，
//         所以標準用 std::move_if_noexcept 決定策略：
//           * move 是 noexcept → 用 move（快）
//           * move 可能拋例外 → **退化成 copy**（慢，但 copy 不破壞來源、
//             失敗可回滾，強例外保證才守得住）
//           * 型別不可複製（如 unique_ptr）→ 只能用 move，
//             此時強例外保證降級為基本保證
//         實務結論：自訂型別的 move constructor 一定要標 noexcept，
//         忘了標會在擴容時默默從移動退化成深複製，且完全沒有警告。
//     追問：本檔的 MayThrow 為什麼 size 停在 2？
//         → 第三次 emplace_back 在**建構新元素時**就拋了例外，
//           新元素從未成功建構，size 不會遞增，前兩個元素完好無損——
//           這正是強例外保證的具體表現。
//
// 🔥 Q4. emplace_back 的回傳值是什麼？（講義第五節）
//     答：**C++17 起**回傳 reference（指向剛建構的新元素），
//         C++11 / C++14 回傳 void；push_back 從頭到尾都是 void。
//         本機以 g++ -std=c++14 -pedantic-errors 實測，
//         寫 auto& r = v.emplace_back(x) 會得到 "forming reference to void"，
//         改 -std=c++17 即通過。
//         （注意：只用 -fsyntax-only 會被 GCC 當擴充放行，結論會錯。）
//     追問：為什麼回傳 reference 而不是 iterator？
//         → 插入位置固定在尾端，位置資訊沒有價值；回傳物件本身才能寫出
//           v.emplace_back(...).member = x 這種鏈式操作。
//
// ⚠️ 陷阱 Q5. 講義練習題 4：
//         vector<unique_ptr<int>> v; auto p = make_unique<int>(42);
//         v.push_back(p);  ← 問題出在哪？
//     答：unique_ptr 不可複製，而 p 是 lvalue，push_back(p) 會選中
//         const T& 重載並嘗試複製 → **編譯失敗**（複製建構子被 delete）。
//         正解是 v.push_back(std::move(p))（或 emplace_back(std::move(p))），
//         明確把所有權轉移進容器；p 之後為 nullptr。
//     為什麼會錯：把「放進容器」直覺當成轉移所有權。
//         C++ 的預設語意是複製，要轉移就必須明確寫出 std::move。
//
// ⚠️ 陷阱 Q6. v.push_back(v[0]) 會不會因為擴容而讀到懸空參考？
//     答：不會。標準明文要求實作必須正確處理這種自我參照。
//         libstdc++ 的順序是「先在新記憶體上建構新元素 → 再搬移舊元素 →
//         最後才釋放舊記憶體」，讀 v[0] 時舊記憶體仍然活著。
//     為什麼會錯：直覺推理成「先擴容、再插入」，因而認定舊記憶體已釋放。
//         但要注意：**操作本身安全 ≠ 舊 iterator 還能用**——
//         只要發生 reallocation，先前取得的 iterator / pointer / reference
//         一律失效。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <stdexcept>
#include <string>

struct MayThrow {
    int value;
    static int count;
    
    MayThrow(int v) : value(v) {
        ++count;
        if (count == 3) {
            throw std::runtime_error("第三個物件建構失敗");
        }
        std::cout << "建構 MayThrow(" << value << ")" << std::endl;
    }
    
    MayThrow(const MayThrow& other) : value(other.value) {
        std::cout << "複製 MayThrow(" << value << ")" << std::endl;
    }
    
    MayThrow(MayThrow&& other) noexcept : value(other.value) {
        std::cout << "移動 MayThrow(" << value << ")" << std::endl;
    }
};

int MayThrow::count = 0;

// -----------------------------------------------------------------------------
// 【日常實務範例】批次匯入使用者資料時的「全有或全無」語意
//   情境：從上游系統批次匯入帳號，其中一筆資料不合法（年齡為負）。
//         業務要求是「整批要嘛全部成功、要嘛完全不生效」，
//         不允許匯入到一半留下半套資料。
//   為什麼和本主題有關：push_back / emplace_back 的**強例外保證**
//         只保證「單次呼叫」失敗時 vector 不變，
//         **不會**自動幫你回滾同一個迴圈裡先前已成功的插入。
//         所以真正的批次原子性必須自己做——這裡的作法是
//         「先匯進暫存 vector，全部成功才 swap 進正式容器」。
//         swap 是 O(1) 且 noexcept，不會在最後一步失敗，
//         這正是 copy-and-swap idiom 的核心概念。
// -----------------------------------------------------------------------------
struct Account {
    std::string userId;
    int age;

    Account(std::string id, int a) : userId(std::move(id)), age(a) {
        if (a < 0) {
            throw std::invalid_argument("年齡不可為負: " + userId);
        }
    }
};

// 回傳 true 表示整批匯入成功；失敗時 target 保持原狀完全不變
bool importBatch(std::vector<Account>& target,
                 const std::vector<std::pair<std::string, int>>& rows) {
    std::vector<Account> staging;          // 暫存區：失敗就整個丟棄
    staging.reserve(rows.size());
    try {
        for (const auto& row : rows) {
            // 兩個零件就地建構，省掉臨時 Account
            staging.emplace_back(row.first, row.second);
        }
    } catch (const std::exception& e) {
        std::cout << "    匯入中止: " << e.what() << std::endl;
        return false;                      // staging 解構，target 完全沒被碰過
    }
    target.swap(staging);                  // O(1) 且 noexcept，最後一步不會失敗
    return true;
}

int main() {
    std::vector<MayThrow> v;
    v.reserve(5);

    try {
        v.emplace_back(1);
        v.emplace_back(2);
        v.emplace_back(3);  // 這裡會拋出例外
        v.emplace_back(4);
    }
    catch (const std::exception& e) {
        std::cout << "捕捉例外: " << e.what() << std::endl;
    }

    std::cout << "vector 大小: " << v.size() << std::endl;  // 2
    // 前兩個元素完好無損、第三個從未成功建構 → 這就是強例外保證的表現。
    // 但注意：它保證的是「單次呼叫失敗時容器不變」，
    // 並不會回滾同一迴圈中先前已成功的 emplace_back。

    std::cout << "\n=== 日常實務：批次匯入的全有或全無 ===" << std::endl;
    std::vector<Account> accounts;
    accounts.emplace_back("existing-user", 30);   // 匯入前已有的既存資料
    std::cout << "  匯入前既有筆數: " << accounts.size() << std::endl;

    std::cout << "  [情境 1] 整批合法:" << std::endl;
    bool ok1 = importBatch(accounts, {{"alice", 28}, {"bob", 35}});
    std::cout << "    結果=" << std::boolalpha << ok1
              << "，目前筆數=" << accounts.size() << std::endl;

    std::cout << "  [情境 2] 中間有一筆不合法:" << std::endl;
    bool ok2 = importBatch(accounts, {{"carol", 41}, {"dave", -5}, {"eve", 22}});
    std::cout << "    結果=" << ok2
              << "，目前筆數=" << accounts.size()
              << "（維持情境 1 的結果，未被污染）" << std::endl;

    std::cout << "  最終內容: ";
    for (const Account& a : accounts) std::cout << a.userId << "(" << a.age << ") ";
    std::cout << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 13 課：vector 元素新增：push_back、emplace_back10.cpp" -o exception_safety

// === 預期輸出 ===
// 建構 MayThrow(1)
// 建構 MayThrow(2)
// 捕捉例外: 第三個物件建構失敗
// vector 大小: 2
// 
// === 日常實務：批次匯入的全有或全無 ===
//   匯入前既有筆數: 1
//   [情境 1] 整批合法:
//     結果=true，目前筆數=2
//   [情境 2] 中間有一筆不合法:
//     匯入中止: 年齡不可為負: dave
//     結果=false，目前筆數=2（維持情境 1 的結果，未被污染）
//   最終內容: alice(28) bob(35) 
