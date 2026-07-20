// =============================================================================
//  第 12 課：vector 元素存取 11  —  本課講義 + 參考/指標失效
// =============================================================================
//
// 【主題資訊 Information】
//   本檔前半是第 12 課的完整講義（保存在下方區塊註解），
//   後半示範 vector 最重要的一條安全規則：
//     【push_back 若造成重新配置，所有指向元素的
//       reference / pointer / iterator 全部失效。】
//   相關成員（皆 <vector>，皆 C++98）：
//     reference operator[](size_type);   回傳參考 → 可取位址
//     reference front();                 回傳參考
//     reference back();                  回傳參考
//     T*        data();                  C++11，回傳底層陣列首位址
//     void      reserve(size_type);      預先配置，之後不再擴容
//   標準的判準：只要 capacity 沒有改變，push_back 就不會使
//     「被 push 之前既有元素」的 reference/pointer 失效
//     （end() iterator 一律失效）。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼參考會失效：vector 的記憶體是「一整塊」】
//   vector 保證元素連續存放。當空間不夠時，它沒辦法「就地長大」——
//   相鄰的記憶體可能被別人佔用了。它只能：
//     ① 向配置器要一塊【更大的新記憶體】
//     ② 把舊元素逐一移動（或複製）到新位置
//     ③ 銷毀舊元素、釋放舊記憶體
//   於是所有原本指向舊位置的參考、指標、迭代器，
//   全部指向了一塊【已經被釋放的記憶體】。
//   它們不會變成 nullptr，也不會自動更新——它們只是靜靜地變成懸空指標。
//
// 【2. 這是「未定義行為」，不是「讀到舊值」】
//   很多人以為失效後讀到的是舊資料。實際上是未定義行為：
//   那塊記憶體已經還給配置器，可能被下一次配置重用、
//   可能被寫入配置器自己的中繼資料、也可能仍然「看起來正常」。
//   最危險的正是最後一種——測試時完全正常，上線後某天突然出錯。
//   本檔【不會】示範讀取失效參考的結果，因為那不存在「正確答案」，
//   把任何一次執行的結果寫進教材都是誤導。
//   本檔改為用一個完全合法的方式證明它確實失效：
//   在操作【之前】把位址轉成 uintptr_t（整數），操作後比較整數。
//   這樣全程沒有使用任何失效的指標值。
//
// 【3. 什麼時候「不會」失效】
//   標準給的判準很明確：capacity 沒變就不會重新配置。
//   所以只要事先 reserve 足夠的容量，後續的 push_back 就不會
//   使既有元素的 reference/pointer 失效。
//   但要注意兩個例外：
//     * end() iterator 每次 push_back 都會失效（它本來就指向尾後）。
//     * insert / erase 到中間位置時，即使不重新配置，
//       被搬動的那些元素的參考也會指向不同的值。
//   「reserve 就完全安全」只在「只做 push_back、且不超過容量」時成立。
//
// 【4. 正確的做法：存索引，不要存指標】
//   若你需要長期記住「某個元素」，存 index 而不是 pointer/reference：
//       size_t idx = 3;          // 安全：push_back 不會改變 index 的意義
//       T* p = &v[3];            // 危險：任何擴容都會讓它失效
//   索引的代價是每次要多算一次位址（v.data() + idx），
//   但它對擴容免疫。只有在「確定容器不會改動」的短暫區間內，
//   才適合持有指標或參考。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼移動元素時 noexcept 這麼重要
//     擴容時 vector 要把舊元素搬到新記憶體。它面臨一個選擇：
//     用移動還是複製？若元素的移動建構子【不是 noexcept】，
//     vector 會選擇【複製】——因為若搬到一半拋出例外，
//     移動已經破壞了來源，無法回復到原狀（違反強例外保證）；
//     複製則可以直接丟掉新記憶體、原資料完好。
//     所以自訂型別的移動建構子一定要標 noexcept，
//     否則 vector 擴容會默默退化成深複製。
//
// (B) data() 與 &v[0] 的差別
//     兩者對非空 vector 完全等價。差別在【空 vector】：
//       v.data()  —— 合法，回傳的指標可能是 nullptr，但呼叫本身沒問題
//       &v[0]     —— 未定義行為，因為 v[0] 本身就越界了
//     所以要取底層指標傳給 C API 時，一律用 data()。
//
// (C) 迭代器失效與參考失效不完全相同
//     以 vector 而言，重新配置時兩者一起失效。
//     但別的容器不一樣——例如 unordered_map 在 rehash 時，
//     所有【迭代器】失效，但【參考與指標永遠不失效】
//     （因為節點本身沒有搬家，只是重新掛到不同的 bucket）。
//     這個差異是容器選型的重要依據：需要長期持有元素位址時，
//     node-based 容器（list / map / unordered_map）比 vector 安全得多。
//
// 【注意事項 Pay Attention】
//   1. 重新配置後使用舊的 reference/pointer/iterator 是未定義行為，
//      不是「讀到舊值」。不要依賴任何一次執行的觀察結果。
//   2. 判準是 capacity 有沒有變，不是 size。
//      reserve 足夠容量後，push_back 不會使既有元素的參考失效。
//   3. end() iterator 每次 push_back 都會失效，即使沒有重新配置。
//   4. 長期記住某個元素請存 index，不要存 pointer。
//   5. 自訂型別的移動建構子要標 noexcept，否則 vector 擴容會改用複製。
//   6. 取底層指標請用 data() 而非 &v[0]（空 vector 時後者是 UB）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector 的參考/指標失效
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. `int& ref = v[0]; v.push_back(1);` 之後 ref 還能用嗎？
//     答：不一定，取決於這次 push_back 有沒有造成重新配置。
//         若 size < capacity，沒有重新配置，ref 仍然有效；
//         若容量滿了，vector 會配置新記憶體、搬移元素、釋放舊記憶體，
//         ref 就變成懸空參考，使用它是未定義行為。
//         由於呼叫端通常無法確定當下的 capacity，
//         實務上的規則是「push_back 之後就當作全部失效」。
//     追問：怎麼讓它保證有效？
//         → 事先 v.reserve(最終大小)。只要後續 push_back 不超過該容量，
//           就不會重新配置，既有元素的 reference/pointer 都保持有效。
//
// 🔥 Q2. vector 擴容時，元素是被移動還是被複製？
//     答：取決於元素的移動建構子有沒有標 noexcept。
//         標了 noexcept → 用移動（快）。
//         沒標（或會拋例外）→ vector 選擇【複製】。
//         原因是強例外保證：若搬到一半拋出例外，移動已經破壞了來源、
//         無法回復；複製則可以直接丟棄新記憶體，原資料完好無損。
//     追問：這在實務上有多大影響？
//         → 很大。一個含 std::string 的結構，忘記標 noexcept
//           會讓每次擴容都變成全部元素的深複製。
//           自訂移動建構子時標 noexcept 幾乎是必做的事。
//
// ⚠️ 陷阱. 「我測過了，push_back 之後那個舊參考還是印出正確的值，
//         所以在我的情況下是安全的。」——這個推論錯在哪？
//     答：錯在把「未定義行為的一次觀察」當成「保證」。
//         那次剛好正確，可能是因為 ① 那次沒有觸發重新配置，
//         或 ② 舊記憶體還沒被配置器重用，內容碰巧沒被覆蓋。
//         兩者都不是保證：資料量、編譯器版本、最佳化等級、
//         甚至同一支程式的不同執行，都可能改變結果。
//         這類 bug 的典型症狀就是「開發環境正常、上線偶發崩潰」。
//     為什麼會錯：把測試當成證明。測試只能證明「存在一次成功的執行」，
//         無法證明「不存在失敗的執行」。對 UB 而言，
//         唯一可靠的方法是從程式碼層面確保它不會發生
//         （用 index、或先 reserve）。
//         想在執行期抓出來，可以用 -fsanitize=address 重新編譯，
//         AddressSanitizer 會直接報 heap-use-after-free。
// ═══════════════════════════════════════════════════════════════════════════

/*
# 第二階段：序列容器 — vector

## 第 12 課：vector 元素存取：operator[]、at、front、back

---

### 核心概念

vector 提供了多種存取元素的方式，它們在**語法**、**效能**和**安全性**上各有不同。選擇正確的存取方式，能讓程式碼既安全又高效。

---

### 一、operator[]：快速但不檢查邊界

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};
    
    // 讀取
    std::cout << "v[0] = " << v[0] << std::endl;  // 10
    std::cout << "v[2] = " << v[2] << std::endl;  // 30
    
    // 修改
    v[1] = 200;
    std::cout << "修改後 v[1] = " << v[1] << std::endl;  // 200
    
    return 0;
}
```

**特性**：
- 不進行邊界檢查
- 存取越界是**未定義行為**（Undefined Behavior）
- 效能最佳，與原生陣列相同

---

### 二、越界存取的危險

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {10, 20, 30};
    
    // 危險！越界存取
    // 編譯器不會報錯，但這是未定義行為
    std::cout << v[10] << std::endl;   // 可能印出垃圾值，可能當機
    
    v[10] = 999;  // 可能破壞其他記憶體，導致難以追蹤的 bug
    
    return 0;
}
```

未定義行為的可怕之處在於：它**可能看起來正常運作**，但在其他環境、其他時間點突然出錯。

---

### 三、at()：安全的邊界檢查

```cpp
#include <vector>
#include <iostream>
#include <stdexcept>

int main() {
    std::vector<int> v = {10, 20, 30};
    
    // 正常存取
    std::cout << "v.at(0) = " << v.at(0) << std::endl;  // 10
    std::cout << "v.at(2) = " << v.at(2) << std::endl;  // 30
    
    // 越界存取會拋出例外
    try {
        std::cout << v.at(10) << std::endl;
    }
    catch (const std::out_of_range& e) {
        std::cout << "捕捉到例外: " << e.what() << std::endl;
    }
    
    return 0;
}
```

**特性**：
- 進行邊界檢查
- 越界時拋出 `std::out_of_range` 例外
- 比 `operator[]` 稍慢（多一次比較）

---

### 四、front() 與 back()：存取首尾元素

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};
    
    // 讀取首尾
    std::cout << "front: " << v.front() << std::endl;  // 10
    std::cout << "back:  " << v.back() << std::endl;   // 50
    
    // 等價於
    std::cout << "v[0]:          " << v[0] << std::endl;
    std::cout << "v[size()-1]:   " << v[v.size() - 1] << std::endl;
    
    // 修改首尾
    v.front() = 100;
    v.back() = 500;
    
    for (int x : v) {
        std::cout << x << " ";  // 100 20 30 40 500
    }
    std::cout << std::endl;
    
    return 0;
}
```

**注意**：對空的 vector 呼叫 `front()` 或 `back()` 是未定義行為！

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> empty_vec;
    
    // 危險！未定義行為
    // std::cout << empty_vec.front() << std::endl;
    // std::cout << empty_vec.back() << std::endl;
    
    // 安全的做法：先檢查
    if (!empty_vec.empty()) {
        std::cout << empty_vec.front() << std::endl;
    }
    
    return 0;
}
```

---

### 五、data()：取得原始指標

```cpp
#include <vector>
#include <iostream>
#include <cstring>  // for memcpy

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};
    
    // 取得指向內部陣列的指標
    int* ptr = v.data();
    
    std::cout << "ptr[0] = " << ptr[0] << std::endl;  // 10
    std::cout << "ptr[2] = " << ptr[2] << std::endl;  // 30
    
    // 用指標修改
    ptr[1] = 200;
    std::cout << "v[1] = " << v[1] << std::endl;  // 200
    
    // 實用場景：與 C 函數互動
    // 例如 memcpy、C 風格 API 等
    int dest[5];
    std::memcpy(dest, v.data(), v.size() * sizeof(int));
    
    return 0;
}
```

**注意**：空的 vector 呼叫 `data()` 可能回傳 nullptr 或有效指標（C++11 後保證合法，但不應解引用）。

---

### 六、const 正確性

當 vector 是 const 時，存取方法回傳的也是 const 參考：

```cpp
#include <vector>
#include <iostream>

void print_first(const std::vector<int>& v) {
    if (!v.empty()) {
        std::cout << v[0] << std::endl;      // OK，讀取
        std::cout << v.at(0) << std::endl;   // OK，讀取
        std::cout << v.front() << std::endl; // OK，讀取
        
        // v[0] = 100;      // 編譯錯誤！不能修改
        // v.front() = 100; // 編譯錯誤！不能修改
    }
    
    const int* ptr = v.data();  // 回傳 const int*
    // ptr[0] = 100;  // 編譯錯誤！
}

int main() {
    std::vector<int> v = {1, 2, 3};
    print_first(v);
    return 0;
}
```

---

### 七、效能比較

```cpp
#include <vector>
#include <iostream>
#include <chrono>

int main() {
    const int N = 100000000;
    std::vector<int> v(1000);
    
    // 填入資料
    for (int i = 0; i < 1000; ++i) {
        v[i] = i;
    }
    
    long long sum1 = 0, sum2 = 0;
    
    // 測試 operator[]
    auto start1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < N; ++i) {
        sum1 += v[i % 1000];
    }
    auto end1 = std::chrono::high_resolution_clock::now();
    
    // 測試 at()
    auto start2 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < N; ++i) {
        sum2 += v.at(i % 1000);
    }
    auto end2 = std::chrono::high_resolution_clock::now();
    
    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);
    
    std::cout << "operator[]: " << duration1.count() << " ms" << std::endl;
    std::cout << "at():       " << duration2.count() << " ms" << std::endl;
    
    // 防止編譯器優化掉
    std::cout << "sums: " << sum1 << ", " << sum2 << std::endl;
    
    return 0;
}
```

在 Release 模式下，`at()` 通常比 `operator[]` 慢一些，但在現代編譯器優化下差異可能很小。

---

### 八、何時用哪個？

| 方法 | 適用場景 |
|------|----------|
| `operator[]` | 確定索引有效時；效能關鍵的內部迴圈 |
| `at()` | 索引來自外部輸入；需要例外處理的場合 |
| `front()` | 需要存取第一個元素，且確定非空 |
| `back()` | 需要存取最後一個元素，且確定非空 |
| `data()` | 需要與 C API 互動；需要原始指標操作 |

---

### 九、安全存取的封裝

你可以自己封裝一個更安全的存取方式：

```cpp
#include <vector>
#include <iostream>
#include <optional>

template <typename T>
std::optional<T> safe_get(const std::vector<T>& v, size_t index) {
    if (index < v.size()) {
        return v[index];
    }
    return std::nullopt;
}

int main() {
    std::vector<int> v = {10, 20, 30};
    
    // 安全存取，不會拋例外也不會未定義行為
    if (auto val = safe_get(v, 1)) {
        std::cout << "v[1] = " << *val << std::endl;  // 20
    }
    
    if (auto val = safe_get(v, 10)) {
        std::cout << "v[10] = " << *val << std::endl;
    } else {
        std::cout << "索引 10 無效" << std::endl;
    }
    
    return 0;
}
```

---

### 十、常見錯誤與陷阱

#### 陷阱一：用 size_t 迴圈的溢位問題

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {10, 20, 30};
    
    // 危險！當 v.size() 是 0 時，v.size() - 1 會溢位成超大數字
    // for (size_t i = v.size() - 1; i >= 0; --i) {  // 永遠為真！
    //     std::cout << v[i] << std::endl;
    // }
    
    // 安全的反向遍歷方式
    for (size_t i = v.size(); i > 0; --i) {
        std::cout << v[i - 1] << " ";  // 30 20 10
    }
    std::cout << std::endl;
    
    // 或者使用反向迭代器（第 16 課會講）
    
    return 0;
}
```

#### 陷阱二：存取後 vector 被修改

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {10, 20, 30};
    
    int& ref = v[0];          // 取得參考
    int* ptr = &v[0];         // 取得指標
    
    std::cout << "ref = " << ref << std::endl;  // 10
    
    v.push_back(40);          // 可能導致擴容！
    v.push_back(50);
    v.push_back(60);
    
    // 危險！如果發生擴容，ref 和 ptr 都已失效
    // std::cout << "ref = " << ref << std::endl;  // 未定義行為
    // std::cout << "*ptr = " << *ptr << std::endl; // 未定義行為
    
    return 0;
}
```

**規則**：任何可能導致擴容的操作都會使現有的指標、參考、迭代器失效。

---

### 存取方法一覽表

| 方法 | 回傳型別 | 邊界檢查 | 空容器行為 |
|------|----------|----------|------------|
| `v[i]` | `T&` / `const T&` | 否 | 未定義行為 |
| `v.at(i)` | `T&` / `const T&` | 是 | 拋出例外 |
| `v.front()` | `T&` / `const T&` | 否 | 未定義行為 |
| `v.back()` | `T&` / `const T&` | 否 | 未定義行為 |
| `v.data()` | `T*` / `const T*` | 否 | 回傳合法指標（不可解引用）|

---

### 練習題

1. **改錯題**：找出以下程式碼的問題並修正：
   ```cpp
   std::vector<int> v;
   for (int i = 0; i < 10; ++i) {
       v[i] = i * i;
   }
   ```

2. **實作題**：寫一個函數 `double safe_average(const std::vector<double>& v)`，計算平均值。如果 vector 為空，回傳 0.0。使用適當的存取方式。

3. **思考題**：以下兩種寫法哪個比較好？為什麼？
   ```cpp
   // 寫法 A
   if (v.size() > 0) {
       process(v[0]);
   }
   
   // 寫法 B
   if (!v.empty()) {
       process(v.front());
   }
   ```

4. **追蹤題**：以下程式碼的輸出是什麼？
   ```cpp
   std::vector<int> v = {1, 2, 3, 4, 5};
   int& a = v.front();
   int& b = v.back();
   a = 10;
   b = 50;
   v[2] = v.front() + v.back();
   for (int x : v) std::cout << x << " ";
   ```

---

下一課我們講 **vector 元素新增：push_back、emplace_back**，深入理解兩者的差異與適用場景。

準備好繼續嗎？
*/



#include <vector>
#include <iostream>
#include <string>
#include <map>
#include <cstdint>   // uintptr_t

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔刻意不放
//   理由：LeetCode 的解法幾乎都是「先把 vector 建好、再唯讀地掃過去」，
//   不會在持有元素參考的同時對容器做 push_back。
//   失效問題屬於「長生命週期的物件管理」，是應用程式的議題，
//   不是演算法題的議題。硬掛一題會讓讀者以為這是個演算法技巧，
//   反而模糊了它真正的性質：一條記憶體安全規則。
//   下方的實務範例才是這個坑真實出現的地方。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】訂單索引：為什麼快取要存 index 而不是指標
//   情境：系統維護一份訂單清單，同時建立「訂單編號 -> 該筆訂單」的快取，
//         讓查詢不必每次線性掃描。訂單會持續進來（push_back）。
//   為什麼用到本主題：這是失效問題在真實程式中最常見的形態。
//     直覺會把快取寫成 map<int, Order*>，但只要 vector 一擴容，
//     所有快取起來的指標就全部懸空——而且問題通常在資料量
//     超過某個門檻後才第一次出現，極難重現。
//   下方 SafeIndex 存的是 size_t 索引，對擴容完全免疫。
//   代價：每次查詢多一次 orders_[idx] 的位址計算，這幾乎免費。
// -----------------------------------------------------------------------------
struct Order {
    int         id;
    std::string customer;
    long long   amountCents;
};

class OrderBook {
public:
    // 新增訂單。注意：這可能觸發擴容，任何外部持有的元素指標都會失效。
    void add(int id, const std::string& customer, long long cents) {
        index_[static_cast<size_t>(id)] = orders_.size();   // 存「索引」
        orders_.push_back({id, customer, cents});
    }

    // 查詢：用索引還原位置。擴容再多次都不影響索引的意義。
    const Order* find(int id) const {
        auto it = index_.find(static_cast<size_t>(id));
        if (it == index_.end()) return nullptr;
        return &orders_[it->second];      // 現場取位址，短暫使用
    }

    size_t size()     const { return orders_.size(); }
    size_t capacity() const { return orders_.capacity(); }

    // 把底層陣列的起始位址轉成整數回傳。
    // 回傳 uintptr_t 而非指標，是為了讓呼叫端能安全地「比較前後是否搬家」
    // 而不必持有任何可能已經失效的指標值。
    uintptr_t dataAddress() const {
        return reinterpret_cast<uintptr_t>(orders_.data());
    }

private:
    std::vector<Order> orders_;
    std::map<size_t, size_t> index_;      // id -> orders_ 的索引
};

int main() {
    std::vector<int> v = {10, 20, 30};

    std::cout << "=== 原始示範：取得參考與指標 ===\n";
    std::cout << "初始 size=" << v.size() << " capacity=" << v.capacity() << "\n";

    int& ref = v[0];          // 取得參考
    int* ptr = &v[0];         // 取得指標

    std::cout << "ref = " << ref << std::endl;  // 10
    std::cout << "*ptr = " << *ptr << std::endl;

    // 關鍵技巧：在任何可能失效的操作【之前】，把位址轉成整數存起來。
    // 之後只比較整數，全程不使用任何可能已失效的指標值。
    const uintptr_t addrBefore = reinterpret_cast<uintptr_t>(v.data());
    const size_t    capBefore  = v.capacity();

    v.push_back(40);          // 可能導致擴容！
    v.push_back(50);
    v.push_back(60);

    const uintptr_t addrAfter = reinterpret_cast<uintptr_t>(v.data());
    const size_t    capAfter  = v.capacity();

    // 危險！如果發生擴容，ref 和 ptr 都已失效
    // std::cout << "ref = " << ref << std::endl;  // 未定義行為
    // std::cout << "*ptr = " << *ptr << std::endl; // 未定義行為

    std::cout << "\n=== 用「位址是否改變」證明失效（不觸碰失效的指標）===\n";
    std::cout << "push_back 前: capacity=" << capBefore << "\n";
    std::cout << "push_back 後: capacity=" << capAfter << " size=" << v.size() << "\n";
    std::cout << "底層陣列是否搬家: " << std::boolalpha << (addrBefore != addrAfter) << "\n";
    std::cout << "→ 搬家了，代表舊記憶體已被釋放；ref 與 ptr 現在都是懸空的。\n";
    std::cout << "  本檔【不會】去讀它們——那是未定義行為，\n";
    std::cout << "  任何一次執行的結果都不能當成「答案」寫進教材。\n";
    std::cout << "  （上面全程只比較 uintptr_t 整數，沒有使用失效的指標值）\n";

    std::cout << "\n=== 對照：先 reserve 就不會失效 ===\n";
    {
        std::vector<int> w;
        w.reserve(10);                     // 一次配置足夠容量
        w.push_back(1);
        w.push_back(2);
        w.push_back(3);

        const uintptr_t a0 = reinterpret_cast<uintptr_t>(w.data());
        const size_t    c0 = w.capacity();

        int& r = w[0];                     // 取得參考
        std::cout << "reserve(10) 後 capacity=" << c0 << "\n";
        std::cout << "取得 w[0] 的參考，值 = " << r << "\n";

        w.push_back(4);
        w.push_back(5);
        w.push_back(6);
        w.push_back(7);

        const uintptr_t a1 = reinterpret_cast<uintptr_t>(w.data());
        std::cout << "又 push 了 4 個之後 size=" << w.size()
                  << " capacity=" << w.capacity() << "\n";
        std::cout << "底層陣列是否搬家: " << (a0 != a1) << "\n";
        std::cout << "capacity 沒變 → 沒有重新配置 → 參考仍然有效，可以安全讀取：\n";
        std::cout << "  r = " << r << "（合法，因為這次沒有發生重新配置）\n";
        r = 999;                            // 也可以安全地寫
        std::cout << "  透過參考改成 999 後，w[0] = " << w[0] << "\n";
    }

    std::cout << "\n=== 判準是 capacity 不是 size：逐步觀察 ===\n";
    {
        std::vector<int> u;
        u.reserve(4);
        uintptr_t prev = reinterpret_cast<uintptr_t>(u.data());
        std::cout << "起始 capacity=" << u.capacity() << "\n";
        for (int i = 1; i <= 6; ++i) {
            u.push_back(i);
            uintptr_t now = reinterpret_cast<uintptr_t>(u.data());
            std::cout << "  push " << i << " -> size=" << u.size()
                      << " capacity=" << u.capacity()
                      << " 是否搬家=" << (now != prev)
                      << (now != prev ? "  ← 此刻所有既有參考失效" : "")
                      << "\n";
            prev = now;
        }
        std::cout << "→ 只有 capacity 改變的那一次才失效，其餘 push_back 都安全。\n";
        std::cout << "  （但 end() iterator 每次 push_back 都失效，即使沒搬家）\n";
    }

    std::cout << "\n=== data() 與 &v[0] 的差別（空 vector）===\n";
    {
        std::vector<int> empty;
        std::cout << "空 vector 呼叫 data(): 合法，回傳值可能是 nullptr\n";
        std::cout << "  data() == nullptr: " << (empty.data() == nullptr) << "\n";
        std::cout << "空 vector 寫 &v[0]  : 未定義行為（v[0] 本身已越界）\n";
        std::cout << "→ 傳底層指標給 C API 時一律用 data()。\n";
    }

    std::cout << "\n=== 日常實務：訂單快取存 index 而非指標 ===\n";
    {
        OrderBook book;
        uintptr_t addr0 = book.dataAddress();

        book.add(1001, "Alice", 120000);
        book.add(1002, "Bob",    45000);
        book.add(1003, "Carol",  98000);

        // 先查一次，確認可用
        if (const Order* o = book.find(1002)) {
            std::cout << "查詢 1002 -> " << o->customer
                      << " $" << (o->amountCents / 100) << "\n";
        }

        // 大量新增，一定會觸發多次擴容
        for (int i = 0; i < 100; ++i) {
            book.add(2000 + i, "bulk", 1000);
        }
        uintptr_t addr1 = book.dataAddress();

        std::cout << "新增 100 筆後 size=" << book.size()
                  << " capacity=" << book.capacity() << "\n";
        std::cout << "底層陣列是否搬過家: " << (addr0 != addr1) << "\n";

        // 索引式快取完全不受影響
        if (const Order* o = book.find(1002)) {
            std::cout << "擴容後再查 1002 -> " << o->customer
                      << " $" << (o->amountCents / 100) << "  ← 仍然正確\n";
        }
        if (const Order* o = book.find(1001)) {
            std::cout << "擴容後再查 1001 -> " << o->customer
                      << " $" << (o->amountCents / 100) << "  ← 仍然正確\n";
        }
        std::cout << "→ 若快取存的是 Order*，這時全部都是懸空指標了。\n";
        std::cout << "  存 size_t 索引則對擴容完全免疫，代價只是多一次位址計算。\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 12 課：vector 元素存取11.cpp -o demo11
// 想在執行期抓出「使用失效指標」，可用 AddressSanitizer 重編：
//        g++ -std=c++17 -fsanitize=address -g <本檔> -o demo11_asan
// 它會在真的踩到已釋放記憶體時直接報 heap-use-after-free。

// === 預期輸出 ===
// === 原始示範：取得參考與指標 ===
// 初始 size=3 capacity=3
// ref = 10
// *ptr = 10
//
// === 用「位址是否改變」證明失效（不觸碰失效的指標）===
// push_back 前: capacity=3
// push_back 後: capacity=6 size=6
// 底層陣列是否搬家: true
// → 搬家了，代表舊記憶體已被釋放；ref 與 ptr 現在都是懸空的。
//   本檔【不會】去讀它們——那是未定義行為，
//   任何一次執行的結果都不能當成「答案」寫進教材。
//   （上面全程只比較 uintptr_t 整數，沒有使用失效的指標值）
//
// === 對照：先 reserve 就不會失效 ===
// reserve(10) 後 capacity=10
// 取得 w[0] 的參考，值 = 1
// 又 push 了 4 個之後 size=7 capacity=10
// 底層陣列是否搬家: false
// capacity 沒變 → 沒有重新配置 → 參考仍然有效，可以安全讀取：
//   r = 1（合法，因為這次沒有發生重新配置）
//   透過參考改成 999 後，w[0] = 999
//
// === 判準是 capacity 不是 size：逐步觀察 ===
// 起始 capacity=4
//   push 1 -> size=1 capacity=4 是否搬家=false
//   push 2 -> size=2 capacity=4 是否搬家=false
//   push 3 -> size=3 capacity=4 是否搬家=false
//   push 4 -> size=4 capacity=4 是否搬家=false
//   push 5 -> size=5 capacity=8 是否搬家=true  ← 此刻所有既有參考失效
//   push 6 -> size=6 capacity=8 是否搬家=false
// → 只有 capacity 改變的那一次才失效，其餘 push_back 都安全。
//   （但 end() iterator 每次 push_back 都失效，即使沒搬家）
//
// === data() 與 &v[0] 的差別（空 vector）===
// 空 vector 呼叫 data(): 合法，回傳值可能是 nullptr
//   data() == nullptr: true
// 空 vector 寫 &v[0]  : 未定義行為（v[0] 本身已越界）
// → 傳底層指標給 C API 時一律用 data()。
//
// === 日常實務：訂單快取存 index 而非指標 ===
// 查詢 1002 -> Bob $450
// 新增 100 筆後 size=103 capacity=128
// 底層陣列是否搬過家: true
// 擴容後再查 1002 -> Bob $450  ← 仍然正確
// 擴容後再查 1001 -> Alice $1200  ← 仍然正確
// → 若快取存的是 Order*，這時全部都是懸空指標了。
//   存 size_t 索引則對擴容完全免疫，代價只是多一次位址計算。
