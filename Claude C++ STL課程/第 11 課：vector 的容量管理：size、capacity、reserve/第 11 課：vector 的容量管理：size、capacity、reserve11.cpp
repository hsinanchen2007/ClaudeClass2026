// =============================================================================
//  第 11 課：vector 的容量管理：size、capacity、reserve11.cpp
//  —  本課完整講義（下方 /* ... */ 區塊為授課本體）
// =============================================================================
//
// 【主題資訊 Information】
//   本課涵蓋的全部介面（皆定義於 <vector>）：
//
//     size_type size()      const noexcept;   // 已建構的元素數         O(1)
//     size_type capacity()  const noexcept;   // 目前 buffer 容得下幾個  O(1)
//     bool      empty()     const noexcept;   // size() == 0            O(1)
//     size_type max_size()  const noexcept;   // 理論上界（實作定義）    O(1)
//     void      reserve(size_type n);         // 只增容，不改 size      O(size())
//     void      resize(size_type n);          // 改 size，新元素 value-init
//     void      resize(size_type n, const T& val);  // 新元素複製自 val
//     void      shrink_to_fit();              // **non-binding request**
//     void      clear() noexcept;             // 清元素，capacity 不變
//
//   標準版本：
//     * size / capacity / empty / max_size / reserve / resize / clear：C++98 起
//     * **shrink_to_fit：C++11 起**（C++98 只能用 swap trick）
//     * C++11 起補上 noexcept；C++20 起上述全部為 constexpr
//     * 單參數 resize(n) 對新元素 value-initialize 是 C++11 起的語意
//       （C++98 簽名為 resize(n, T val = T())，要求 T 可複製）
//
//   核心不變量：0 <= size() <= capacity() <= max_size()
//
//   ★ 三個最關鍵、也最常被講錯的正確性重點 ★
//   1. **成長倍率是實作定義**。標準只要求 push_back 為 **amortized O(1)**，
//      從未規定 2× 或 1.5×。libstdc++ 實測為 2×、MSVC 為 1.5×，兩者都合規。
//      凡是 capacity 的具體數值，一律是「實測值，非標準保證」。
//   2. **shrink_to_fit() 是 non-binding request**，標準明文允許實作完全
//      忽略它。**絕不可寫成「一定會把 capacity 降到 size」。**
//      本機 libstdc++ 實測會縮，但那是實作行為。
//   3. **reserve(n) 不改變 size、不建構元素**；n <= capacity() 時標準規定
//      **不做任何事**；n > max_size() 丟 std::length_error。
//      reserve 之後用 v[i] 存取是 UB —— 那塊記憶體上沒有任何物件。
//
//   逐一主題的深入說明與可執行示範，請見同目錄的
//   1.cpp（size/capacity/empty）、2.cpp（reserve）、3.cpp（重新配置成本）、
//   4.cpp（resize）、5.cpp（reserve vs resize）、6.cpp（shrink_to_fit）、
//   7.cpp（max_size）、8.cpp（clear / swap trick）、9.cpp（讀取已知筆數）、
//   10.cpp（緩衝區配置），以及 summary.cpp（總複習）。
//
//   註：下方講義本體中的 capacity 數值屬示意；實際數值以各示範檔的
//       實測輸出為準（例如講義寫 shrink 前 capacity=1000 是正確的，
//       但「resize(8) 之後 capacity 是 8」這類推測請以 4.cpp 實測為準
//       —— libstdc++ 的 resize 走幾何成長 max(2*size, n)，實際是 10）。
//
// 【詳細解釋 Explanation】
//   ★ 逐節說明見下方講義本體，此處補三個貫穿本課的主軸：
//
//   【1. size 與 capacity 是兩件不同的事】
//      size() 是「已經建構好幾個物件」，capacity() 是「配置的記憶體放得下幾個」。
//      兩者之間那段空間**已配置但沒有任何合法物件**，所以 v[size()] 是 UB。
//      理解這一點，才會明白為什麼 reserve 不能拿來「先撐大再用索引寫入」。
//
//   【2. reserve 的價值是把多次 reallocation 壓成一次】
//      不 reserve 而連續 push_back n 次，會經歷 log₂(n) 次重新配置，
//      每次都要搬移全部既有元素，總搬移量約 2n；先 reserve(n) 則一次配置到位、
//      零搬移。資料筆數已知時（讀檔、查詢結果、批次匯入）這是最划算的優化。
//
//   【3. 容量只增不減，除非你明確要求】
//      clear() 只銷毀元素、**不歸還記憶體**（capacity 不變），這對「反覆重用的
//      緩衝區」是優點：下一輪 push_back 不必再配置。但若你是真的想放掉記憶體，
//      就得用 shrink_to_fit（非強制）或 swap trick（可靠）。
//
// 【概念補充 Concept Deep Dive】
//   為什麼幾何成長才能達到攤銷 O(1)？
//   若每次只加固定量 k，n 次 push_back 的總搬移量是 1+2+…+(n/k) ≈ O(n²/k)，
//   仍是平方級；改成每次乘以固定倍率 r（>1），總搬移量是等比級數
//   n + n/r + n/r² + … ≈ n·r/(r-1)，是 **O(n)**，攤銷到每次就是常數。
//   倍率取多少是取捨：r 越大重新配置次數越少但浪費越多空間；
//   r=2 實作簡單（libstdc++），r=1.5 則讓釋放出的舊區塊有機會被後續配置重用
//   （MSVC）。兩者都符合標準 —— 標準只要求攤銷 O(1)，不規定 r。
//
//   swap trick 為何可靠：
//     std::vector<T>(v).swap(v);
//   先用 v 複製建構一個**剛好 size 大小**的臨時 vector（複製建構不會預留多餘容量），
//   再與 v 交換內部指標，最後臨時物件析構時帶走舊的大 buffer。
//   因為它靠的是「複製建構的容量行為」而非「請求實作縮容」，所以不像
//   shrink_to_fit 那樣可被忽略。C++11 之前這是唯一的縮容手段。
//
// 【注意事項 Pay Attention】
//   1. **成長倍率與任何具體 capacity 數值都是實作定義**（libstdc++ 實測 2×、
//      MSVC 1.5×）。標準只規定 push_back 攤銷 O(1)，不要據此寫測試或斷言。
//   2. **shrink_to_fit() 是 non-binding request**，標準允許實作完全忽略；
//      絕不可敘述成「一定會把 capacity 降到 size」。需要保證時用 swap trick。
//   3. reserve(n) 只增不減：n <= capacity() 時標準規定不做任何事，
//      **不會**縮容；n > max_size() 丟 std::length_error。
//   4. reserve 不改變 size、不建構元素 —— reserve 後用 v[i] 讀寫是 UB。
//      要能用索引存取請用 resize。
//   5. 任何導致 reallocation 的操作（push_back 超容、reserve、resize 變大）
//      都會使**所有** iterator / pointer / reference 失效，含 end()。
//   6. clear() 銷毀元素但 capacity 不變；想同時放掉記憶體要另外處理。
//
// =============================================================================

/*
# 第二階段：序列容器 — vector

## 第 11 課：vector 的容量管理：size、capacity、reserve

---

### 核心概念

上一課我們學了如何建立 vector，這一課要深入理解如何**控制**它的記憶體行為。掌握容量管理，是寫出高效能程式的關鍵。

---

### size 與 capacity 的區別（複習）

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5};
    
    std::cout << "size: " << v.size() << std::endl;         // 5（實際元素數量）
    std::cout << "capacity: " << v.capacity() << std::endl; // >= 5（配置的空間）
    std::cout << "empty: " << v.empty() << std::endl;       // 0（false）
    
    return 0;
}
```

記住這個關係：**size ≤ capacity**，永遠成立。

---

### 一、reserve：預先配置空間

當你知道大約需要多少元素時，用 `reserve` 預先配置空間可以避免多次擴容：

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v;
    
    std::cout << "初始 - size: " << v.size() 
              << ", capacity: " << v.capacity() << std::endl;
    
    v.reserve(100);  // 預先配置至少 100 個元素的空間
    
    std::cout << "reserve(100) 後 - size: " << v.size() 
              << ", capacity: " << v.capacity() << std::endl;
    // size 仍是 0，capacity >= 100
    
    // 現在連續 push_back 100 次都不會觸發擴容
    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
    }
    
    std::cout << "100 次 push_back 後 - size: " << v.size() 
              << ", capacity: " << v.capacity() << std::endl;
    
    return 0;
}
```

**重點**：`reserve` 只改變 capacity，不改變 size，也不會建立任何元素。

---

### 二、效能差異實測

```cpp
#include <vector>
#include <iostream>
#include <chrono>

int main() {
    const int N = 1000000;
    
    // 不使用 reserve
    auto start1 = std::chrono::high_resolution_clock::now();
    {
        std::vector<int> v1;
        for (int i = 0; i < N; ++i) {
            v1.push_back(i);
        }
    }
    auto end1 = std::chrono::high_resolution_clock::now();
    
    // 使用 reserve
    auto start2 = std::chrono::high_resolution_clock::now();
    {
        std::vector<int> v2;
        v2.reserve(N);
        for (int i = 0; i < N; ++i) {
            v2.push_back(i);
        }
    }
    auto end2 = std::chrono::high_resolution_clock::now();
    
    auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(end1 - start1);
    auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(end2 - start2);
    
    std::cout << "不使用 reserve: " << duration1.count() << " 微秒" << std::endl;
    std::cout << "使用 reserve:   " << duration2.count() << " 微秒" << std::endl;
    
    return 0;
}
```

你會發現使用 `reserve` 的版本明顯快很多，尤其當元素是需要複製的複雜物件時。

---

### 三、resize：改變實際大小

`resize` 和 `reserve` 不同，它會**實際改變元素數量**：

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5};
    std::cout << "初始: size=" << v.size() << std::endl;  // 5
    
    // 擴大：新元素用預設值（0）填充
    v.resize(8);
    std::cout << "resize(8): ";
    for (int x : v) std::cout << x << " ";  // 1 2 3 4 5 0 0 0
    std::cout << std::endl;
    
    // 擴大並指定填充值
    v.resize(10, 99);
    std::cout << "resize(10, 99): ";
    for (int x : v) std::cout << x << " ";  // 1 2 3 4 5 0 0 0 99 99
    std::cout << std::endl;
    
    // 縮小：多餘的元素被銷毀
    v.resize(3);
    std::cout << "resize(3): ";
    for (int x : v) std::cout << x << " ";  // 1 2 3
    std::cout << std::endl;
    
    // 注意：縮小後 capacity 通常不變
    std::cout << "縮小後 capacity: " << v.capacity() << std::endl;  // 仍然 >= 10
    
    return 0;
}
```

---

### 四、reserve vs resize 對照

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v1, v2;
    
    v1.reserve(5);
    v2.resize(5);
    
    std::cout << "reserve(5): size=" << v1.size() 
              << ", capacity=" << v1.capacity() << std::endl;
    // size=0, capacity>=5
    
    std::cout << "resize(5):  size=" << v2.size() 
              << ", capacity=" << v2.capacity() << std::endl;
    // size=5, capacity>=5
    
    // reserve 後不能直接用 operator[]
    // v1[0] = 10;  // 未定義行為！size 是 0
    
    // resize 後可以直接用 operator[]
    v2[0] = 10;  // 合法，因為 size 是 5
    
    return 0;
}
```

| 函數 | 改變 size | 改變 capacity | 建立元素 |
|------|-----------|---------------|----------|
| `reserve(n)` | 否 | 是（若 n > capacity） | 否 |
| `resize(n)` | 是 | 可能（若 n > capacity） | 是（擴大時） |

---

### 五、shrink_to_fit：釋放多餘空間

C++11 引入的 `shrink_to_fit` 可以請求釋放多餘的記憶體：

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v;
    v.reserve(1000);
    
    for (int i = 0; i < 10; ++i) {
        v.push_back(i);
    }
    
    std::cout << "shrink 前: size=" << v.size() 
              << ", capacity=" << v.capacity() << std::endl;
    // size=10, capacity=1000
    
    v.shrink_to_fit();
    
    std::cout << "shrink 後: size=" << v.size() 
              << ", capacity=" << v.capacity() << std::endl;
    // size=10, capacity 約等於 10（不保證）
    
    return 0;
}
```

**注意**：`shrink_to_fit` 是**非強制性的請求**，編譯器可以忽略它。實務上大多數實作會照做。

---

### 六、max_size：理論最大容量

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v;
    
    std::cout << "max_size: " << v.max_size() << std::endl;
    
    // 在 64 位元系統上，這個數字通常非常大
    // 例如：4611686018427387903（約 4.6 × 10^18）
    
    return 0;
}
```

這是理論上的最大值，實際上受限於可用記憶體。

---

### 七、清空 vector 的方式比較

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5};
    v.reserve(100);
    
    std::cout << "初始: size=" << v.size() 
              << ", capacity=" << v.capacity() << std::endl;
    
    // 方法一：clear（保留 capacity）
    v.clear();
    std::cout << "clear(): size=" << v.size() 
              << ", capacity=" << v.capacity() << std::endl;
    // size=0, capacity 不變
    
    // 重新填入資料
    v = {1, 2, 3, 4, 5};
    
    // 方法二：swap 技巧（C++11 前常用，釋放記憶體）
    std::vector<int>().swap(v);
    std::cout << "swap 技巧: size=" << v.size() 
              << ", capacity=" << v.capacity() << std::endl;
    // size=0, capacity=0
    
    // 重新填入資料
    v = {1, 2, 3, 4, 5};
    v.reserve(100);
    
    // 方法三：clear + shrink_to_fit（C++11 後推薦）
    v.clear();
    v.shrink_to_fit();
    std::cout << "clear + shrink: size=" << v.size() 
              << ", capacity=" << v.capacity() << std::endl;
    // size=0, capacity 約為 0
    
    return 0;
}
```

---

### 八、實際應用場景

#### 場景一：讀取已知大小的資料

```cpp
#include <vector>
#include <iostream>

int main() {
    int n;
    std::cout << "請輸入資料筆數: ";
    std::cin >> n;
    
    std::vector<int> data;
    data.reserve(n);  // 預先配置，避免擴容
    
    std::cout << "請輸入 " << n << " 筆整數: ";
    for (int i = 0; i < n; ++i) {
        int x;
        std::cin >> x;
        data.push_back(x);
    }
    
    // 處理資料...
    
    return 0;
}
```

#### 場景二：建立固定大小的緩衝區

```cpp
#include <vector>
#include <cstdint>

int main() {
    // 建立 1MB 的緩衝區
    std::vector<uint8_t> buffer(1024 * 1024);  // resize，直接可用
    
    // 或者只配置空間，稍後填入
    std::vector<uint8_t> buffer2;
    buffer2.reserve(1024 * 1024);  // 只 reserve，size 仍為 0
    
    return 0;
}
```

#### 場景三：收集不確定數量的結果

```cpp
#include <vector>
#include <iostream>

std::vector<int> find_even_numbers(const std::vector<int>& input) {
    std::vector<int> result;
    
    // 最多有 input.size() 個偶數，所以這是合理的上界
    result.reserve(input.size());
    
    for (int x : input) {
        if (x % 2 == 0) {
            result.push_back(x);
        }
    }
    
    // 可選：釋放多餘空間
    result.shrink_to_fit();
    
    return result;
}

int main() {
    std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto evens = find_even_numbers(nums);
    
    for (int x : evens) {
        std::cout << x << " ";  // 2 4 6 8 10
    }
    std::cout << std::endl;
    
    return 0;
}
```

---

### 容量管理函數一覽

| 函數 | 說明 |
|------|------|
| `size()` | 回傳目前元素數量 |
| `capacity()` | 回傳目前配置的空間可容納的元素數量 |
| `empty()` | 回傳 size() == 0 |
| `max_size()` | 回傳理論最大容量 |
| `reserve(n)` | 確保 capacity >= n，不改變 size |
| `resize(n)` | 改變 size 為 n，必要時擴大 capacity |
| `resize(n, val)` | 同上，新元素用 val 填充 |
| `shrink_to_fit()` | 請求釋放多餘的 capacity |
| `clear()` | 移除所有元素，size 變 0，capacity 不變 |

---

### 練習題

1. **預測題**：以下程式碼執行後，v 的 size 和 capacity 分別是多少？
   ```cpp
   std::vector<int> v;
   v.reserve(10);
   v.resize(5);
   v.push_back(100);
   v.resize(3);
   v.shrink_to_fit();
   ```

2. **實作題**：寫一個函數，接收一個 `vector<int>`，回傳一個新的 vector，只包含原本的奇數。要求：使用 reserve 優化效能。

3. **思考題**：為什麼 `shrink_to_fit` 被設計成「非強制性請求」而不是保證執行？（提示：考慮記憶體配置器的限制）

4. **除錯題**：以下程式碼有什麼問題？
   ```cpp
   std::vector<int> v;
   v.reserve(100);
   for (int i = 0; i < 100; ++i) {
       v[i] = i;  // 這裡有問題嗎？
   }
   ```

---

下一課我們講 **vector 元素存取：operator[]、at、front、back**，學習各種存取元素的方式及其安全性差異。

準備好繼續嗎？
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector 容量管理（本課總整理）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. size() 和 capacity() 差在哪？為什麼 vector 要同時提供兩個？
//     答：size() 是「已建構的元素個數」，capacity() 是「這塊 buffer 放得下
//         幾個」，永遠 size() <= capacity()。多配的尾巴讓 push_back 在沒滿
//         之前免於重新配置與搬移，push_back 才能是 amortized O(1)；
//         若每次只配剛好夠，n 次插入會退化成 O(n²)。
//     追問：兩者中間那段記憶體裡有什麼？
//         → 未建構的原始記憶體（raw storage），沒有任何 T 物件存在，碰它是 UB。
//
// 🔥 Q2. reserve(n) 會改變 size 嗎？之後可以直接 v[0] = x 嗎？
//     答：不會。reserve 只動 capacity、不建構任何元素，size 仍是 0，
//         此時 v[0] 是越界存取 → UB。要填資料只能 push_back / emplace_back；
//         想要能直接索引存取，該用的是 resize(n)。
//     追問：n <= capacity() 時 reserve 做什麼？
//         → 標準規定**什麼都不做**。reserve 只增不減，不會縮容。
//
// 🔥 Q3. push_back 為什麼是 amortized O(1)？標準有規定成長倍率嗎？
//     答：因為採幾何成長。倍率 k 時總搬移量是等比級數 ≈ n·k/(k-1) = Θ(n)，
//         分攤到 n 次插入就是 O(1)。**標準只要求 amortized O(1)，
//         從未規定倍率**：libstdc++ 實測 2×、MSVC 是 1.5×，兩者都合規。
//     追問：改成「每次固定加 c 個」會怎樣？
//         → 總搬移量變 Θ(n²)，攤提不再成立 —— 幾何成長是不可或缺的。
//
// 🔥 Q4. shrink_to_fit() 保證把 capacity 降到 size 嗎？
//     答：**不保證**。標準明文寫它是 **non-binding request**，實作可以完全
//         忽略。libstdc++ 實測會照做，但那是實作行為，不可寫成斷言。
//     追問：為什麼標準要留這個彈性？又要怎麼「保證」縮容？
//         → 縮容需要配置新 buffer，記憶體不足時硬性要求就得丟 bad_alloc，
//           「想省記憶體卻因記憶體不足而失敗」太荒謬；某些 allocator
//           也根本沒有縮小的概念。需要保證時用 swap trick：
//           std::vector<T>(v).swap(v)，它一定會重新配置。
//
// ⚠️ 陷阱 1. clear() 之後 capacity 會變 0 嗎？
//     答：不會。clear() 只解構元素、把 size 設為 0，capacity 完全不變。
//         這是刻意設計 —— 清空多半是為了重複使用，保留 buffer 讓下一輪
//         填入免於重新配置。想真的釋放要 clear() + shrink_to_fit()，
//         或 std::vector<T>().swap(v)。
//     為什麼會錯：把「清空」等同於「還記憶體」。兩者在 vector 是分開的操作。
//
// ⚠️ 陷阱 2. reserve / resize / shrink_to_fit 之後，舊 iterator 還能用嗎？
//     答：只要真的發生重新配置（含 shrink_to_fit 成功縮容），所有
//         iterator / pointer / reference **全部失效**。clear() 即使位址
//         沒變也會使它們失效，因為元素已被解構。
//     為什麼會錯：以為「只是多要空間」或「只是變小」不會動到既有元素。
//         實際上那是換一塊新記憶體再整批搬過去。
//
// ⚠️ 陷阱 3. 「reserve 一定比較快」——什麼時候反而更慢？
//     答：在迴圈裡每輪 reserve(size()+1) 時。libstdc++ 的 reserve 剛好配置
//         n，逐次 reserve 等於把幾何成長改成「每次加 1」→ 每次插入都重新
//         配置 → O(n²)。本機實測推 10 個元素：逐次 reserve 觸發 10 次
//         重新配置，什麼都不做的 push_back 只要 5 次。
//     為什麼會錯：把 reserve 當成「效能開關」，以為呼叫越多越快。
//         它的價值來自**一次就要到最終大小**；不知道總量時什麼都不做才對。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>

std::vector<int> find_even_numbers(const std::vector<int>& input) {
    std::vector<int> result;
    
    // 最多有 input.size() 個偶數，所以這是合理的上界
    result.reserve(input.size());
    
    for (int x : input) {
        if (x % 2 == 0) {
            result.push_back(x);
        }
    }
    
    // 可選：釋放多餘空間
    result.shrink_to_fit();
    
    return result;
}

// -----------------------------------------------------------------------------
// 【練習題 2 解答】只保留奇數，並用 reserve 優化
//   與 find_even_numbers 同一個模式：以「最多和輸入一樣多」為保守上界，
//   一次 reserve 到位，收集完再視需要 shrink_to_fit。
// -----------------------------------------------------------------------------
std::vector<int> find_odd_numbers(const std::vector<int>& input) {
    std::vector<int> result;
    result.reserve(input.size());   // 保守上界：最多全部都是奇數

    for (int x : input) {
        if (x % 2 != 0) {
            result.push_back(x);
        }
    }

    result.shrink_to_fit();         // 非強制請求，libstdc++ 實測會縮
    return result;
}

int main() {
    std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto evens = find_even_numbers(nums);

    for (int x : evens) {
        std::cout << x << " ";  // 2 4 6 8 10
    }
    std::cout << std::endl;

    // =========================================================================
    // 講義【練習題】解答（以下數值皆為本機實跑取得）
    // =========================================================================

    std::cout << "\n=== 練習題 1 解答：逐步追蹤 size 與 capacity ===" << std::endl;
    {
        std::vector<int> v;
        std::cout << "起始          : size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;

        v.reserve(10);      // 只增容，size 不變
        std::cout << "reserve(10)   : size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;

        v.resize(5);        // 建構 5 個 0；capacity 已夠，不重新配置
        std::cout << "resize(5)     : size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;

        v.push_back(100);   // 仍在容量內，不擴容
        std::cout << "push_back(100): size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;

        v.resize(3);        // 銷毀尾端元素，capacity **不變**
        std::cout << "resize(3)     : size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;

        v.shrink_to_fit();  // non-binding request；libstdc++ 實測縮到 size
        std::cout << "shrink_to_fit : size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;

        std::cout << "答案: size=3, capacity=3"
                  << "（capacity 為 libstdc++ 實測，shrink_to_fit 不保證縮容）"
                  << std::endl;
    }

    std::cout << "\n=== 練習題 2 解答：用 reserve 篩出奇數 ===" << std::endl;
    {
        std::vector<int> odds = find_odd_numbers(nums);
        std::cout << "奇數: [";
        for (std::size_t i = 0; i < odds.size(); ++i) {
            if (i != 0) std::cout << ' ';
            std::cout << odds[i];
        }
        std::cout << "]  size=" << odds.size()
                  << ", capacity=" << odds.capacity() << std::endl;
    }

    std::cout << "\n=== 練習題 3 解答：shrink_to_fit 為何是非強制請求 ==="
              << std::endl;
    std::cout << "縮容必須配置一塊新的小 buffer 再把元素搬過去。若此時記憶體不足,"
              << std::endl;
    std::cout << "硬性要求就得丟 bad_alloc ——「想省記憶體」卻因記憶體不足而失敗"
              << "並不合理;" << std::endl;
    std::cout << "而且記憶體池之類的 allocator 根本沒有『縮小』這個概念。"
              << std::endl;
    std::cout << "所以標準讓使用者表達意圖, 是否照辦由實作決定。" << std::endl;

    std::cout << "\n=== 練習題 4 解答：reserve 之後用 v[i] 是 UB ===" << std::endl;
    {
        std::vector<int> v;
        v.reserve(100);
        std::cout << "reserve(100) 後 size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;
        std::cout << "原程式碼寫 v[i] = i, 但 size 是 0 → 全部越界, 是 UB。"
                  << std::endl;
        std::cout << "（那塊記憶體只是已配置、未建構, 上面沒有任何 int 物件）"
                  << std::endl;

        for (int i = 0; i < 100; ++i) v.push_back(i);   // 修法一
        std::cout << "修法一 push_back : size=" << v.size()
                  << ", capacity=" << v.capacity()
                  << ", v[99]=" << v[99] << std::endl;

        std::vector<int> w;
        w.resize(100);                                  // 修法二
        for (int i = 0; i < 100; ++i) w[i] = i;         // 此時索引存取才合法
        std::cout << "修法二 resize    : size=" << w.size()
                  << ", capacity=" << w.capacity()
                  << ", w[99]=" << w[99] << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 11 課：vector 的容量管理：size、capacity、reserve11.cpp" -o demo11

// 註：以下 capacity 數值為 libstdc++ / GCC 15.2 實測，**非標準保證**。
//     標準只要求 push_back 為 amortized O(1)，未規定成長倍率（libstdc++ 2×、
//     MSVC 1.5×）；shrink_to_fit 是 non-binding request，允許實作忽略。

// === 預期輸出 ===
// 2 4 6 8 10
//
// === 練習題 1 解答：逐步追蹤 size 與 capacity ===
// 起始          : size=0, capacity=0
// reserve(10)   : size=0, capacity=10
// resize(5)     : size=5, capacity=10
// push_back(100): size=6, capacity=10
// resize(3)     : size=3, capacity=10
// shrink_to_fit : size=3, capacity=3
// 答案: size=3, capacity=3（capacity 為 libstdc++ 實測，shrink_to_fit 不保證縮容）
//
// === 練習題 2 解答：用 reserve 篩出奇數 ===
// 奇數: [1 3 5 7 9]  size=5, capacity=5
//
// === 練習題 3 解答：shrink_to_fit 為何是非強制請求 ===
// 縮容必須配置一塊新的小 buffer 再把元素搬過去。若此時記憶體不足,
// 硬性要求就得丟 bad_alloc ——「想省記憶體」卻因記憶體不足而失敗並不合理;
// 而且記憶體池之類的 allocator 根本沒有『縮小』這個概念。
// 所以標準讓使用者表達意圖, 是否照辦由實作決定。
//
// === 練習題 4 解答：reserve 之後用 v[i] 是 UB ===
// reserve(100) 後 size=0, capacity=100
// 原程式碼寫 v[i] = i, 但 size 是 0 → 全部越界, 是 UB。
// （那塊記憶體只是已配置、未建構, 上面沒有任何 int 物件）
// 修法一 push_back : size=100, capacity=100, v[99]=99
// 修法二 resize    : size=100, capacity=100, w[99]=99

