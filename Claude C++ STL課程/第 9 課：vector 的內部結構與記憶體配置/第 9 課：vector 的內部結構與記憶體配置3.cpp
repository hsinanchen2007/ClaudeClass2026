/*
好的，陳信安，我們繼續 STL 課程。根據大綱，現在進入：

---

# 第二階段：序列容器 — vector

## 第 9 課：vector 的內部結構與記憶體配置

---

### 核心概念

`std::vector` 是 STL 中最常用的容器，它的本質是一個**動態陣列**（dynamic array）。要真正理解 vector，必須先搞懂它在記憶體中長什麼樣子。

---

### vector 的三個關鍵指標

vector 內部維護三個指標，這是理解其所有行為的基礎：

```cpp
template <typename T, typename Allocator = std::allocator<T>>
class vector {
    // 簡化的內部結構
    T* _begin;    // 指向第一個元素
    T* _end;      // 指向最後一個元素的下一個位置
    T* _cap;      // 指向配置空間的尾端
};
```

用圖來表示：

```
已配置的記憶體區塊
┌───┬───┬───┬───┬───┬───┬───┬───┐
│ A │ B │ C │ D │ E │   │   │   │
└───┴───┴───┴───┴───┴───┴───┴───┘
↑                   ↑           ↑
_begin             _end        _cap

size() = _end - _begin = 5（實際存放的元素數量）
capacity() = _cap - _begin = 8（已配置的空間可容納的元素數量）
```

---

### 連續記憶體的保證

C++ 標準保證 vector 的元素儲存在**連續的記憶體空間**中。這個保證非常重要：

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};
    
    // 印出每個元素的位址
    for (size_t i = 0; i < v.size(); ++i) {
        std::cout << "v[" << i << "] 位址: " << &v[i] << std::endl;
    }
    
    // 因為是連續記憶體，可以取得原始指標
    int* ptr = v.data();  // 或 &v[0]
    std::cout << "第三個元素: " << ptr[2] << std::endl;  // 輸出 30
    
    return 0;
}
```

執行後你會看到位址是連續的，每個 int 相差 4 bytes（假設 sizeof(int) == 4）。

---

### 為什麼要分開 size 和 capacity？

這是 vector 效能的關鍵設計。考慮這個情境：

```cpp
std::vector<int> v;

// 連續新增 1000 個元素
for (int i = 0; i < 1000; ++i) {
    v.push_back(i);
}
```

如果每次 `push_back` 都重新配置記憶體：
- 配置 1 個元素的空間，複製 0 個舊元素
- 配置 2 個元素的空間，複製 1 個舊元素
- 配置 3 個元素的空間，複製 2 個舊元素
- ...
- 配置 1000 個元素的空間，複製 999 個舊元素

總複製次數 = 0 + 1 + 2 + ... + 999 = **499,500 次**

這太慢了。所以 vector 採用**預留空間**的策略：當空間不夠時，一次配置更多的空間（通常是原本的 1.5 倍或 2 倍）。

---

### 實際觀察 capacity 的變化

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v;
    
    size_t prev_cap = 0;
    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
        if (v.capacity() != prev_cap) {
            std::cout << "size: " << v.size() 
                      << ", capacity: " << v.capacity() << std::endl;
            prev_cap = v.capacity();
        }
    }
    
    return 0;
}
```

在不同編譯器上，你可能看到：

**MSVC（通常 1.5 倍成長）：**
```
size: 1, capacity: 1
size: 2, capacity: 2
size: 3, capacity: 3
size: 4, capacity: 4
size: 5, capacity: 6
size: 7, capacity: 9
size: 10, capacity: 13
...
```

**GCC/Clang（通常 2 倍成長）：**
```
size: 1, capacity: 1
size: 2, capacity: 2
size: 3, capacity: 4
size: 5, capacity: 8
size: 9, capacity: 16
size: 17, capacity: 32
...
```

---

### 記憶體配置的代價

當 capacity 不足時，vector 會：

1. **配置新的更大的記憶體區塊**
2. **將所有舊元素移動或複製到新空間**
3. **銷毀舊空間中的元素**
4. **釋放舊的記憶體區塊**

```cpp
#include <vector>
#include <iostream>

class Tracker {
public:
    int id;
    Tracker(int i) : id(i) { 
        std::cout << "建構 " << id << std::endl; 
    }
    Tracker(const Tracker& other) : id(other.id) { 
        std::cout << "複製 " << id << std::endl; 
    }
    Tracker(Tracker&& other) noexcept : id(other.id) { 
        std::cout << "移動 " << id << std::endl; 
    }
    ~Tracker() { 
        std::cout << "銷毀 " << id << std::endl; 
    }
};

int main() {
    std::vector<Tracker> v;
    
    std::cout << "--- 加入第 1 個 ---" << std::endl;
    v.emplace_back(1);
    
    std::cout << "--- 加入第 2 個 ---" << std::endl;
    v.emplace_back(2);
    
    std::cout << "--- 加入第 3 個 ---" << std::endl;
    v.emplace_back(3);
    
    std::cout << "--- 程式結束 ---" << std::endl;
    return 0;
}
```

執行這段程式，你會清楚看到每次擴容時的複製/移動和銷毀操作。

---

### 重點整理

| 概念 | 說明 |
|------|------|
| 連續記憶體 | vector 元素保證存放在連續空間，可用指標算術存取 |
| size | 目前實際存放的元素數量 |
| capacity | 目前配置的空間可容納的元素數量 |
| 擴容策略 | 空間不足時，配置 1.5x 或 2x 的新空間並搬移元素 |
| 擴容代價 | 需要配置新空間、搬移所有元素、釋放舊空間 |

---

### 練習題

1. **觀察題**：寫一個程式，建立一個空的 `vector<int>`，連續 push_back 50 個元素，記錄每次 capacity 變化時的值。你的編譯器採用什麼擴容策略？

2. **思考題**：如果你事先知道需要存放 1000 個元素，有什麼方法可以避免多次擴容？（提示：這會在第 11 課詳細講解）

3. **驗證題**：修改 Tracker 類別的程式，把移動建構子的 `noexcept` 拿掉，觀察輸出有什麼變化。為什麼會這樣？

---

下一課我們會講 **vector 的宣告與初始化方式**，涵蓋各種建立 vector 的方法。

有任何問題嗎？或者我們繼續第 10 課？
*/

// =============================================================================
//  第 9 課 3  —  擴容的代價，以及 noexcept 移動建構子為何是關鍵
// =============================================================================
//
// 【主題資訊 Information】
//   相關工具 : std::move_if_noexcept<T>(x)            [C++11, <utility>]
//              std::is_nothrow_move_constructible_v<T> [C++17, <type_traits>]
//   標頭檔   : <vector>, <utility>, <type_traits>
//   複雜度   : 擴容一次為 O(n)（搬移 n 個元素）；push_back 攤銷 O(1)
//   重點結論 : 移動建構子沒有標 noexcept 時，vector 擴容會退化成「複製」
//
// 【詳細解釋 Explanation】
//
// 【1. 擴容時到底發生了什麼】
//   capacity 用完時，vector 依序做四件事：
//     (1) 向配置器要一塊更大的記憶體；
//     (2) 把舊區塊的 n 個元素逐一「搬」到新區塊（移動或複製，見下）；
//     (3) 逐一呼叫舊元素的解構子；
//     (4) 釋放舊區塊。
//   本檔的 Tracker 把每一步都印出來，可以直接觀察到這個過程。
//
// 【2. 為什麼「沒有 noexcept 就不敢用移動」——強例外保證】
//   vector 的 push_back 提供「強例外保證」（strong exception guarantee）：
//   若操作中途丟出例外，容器必須維持原狀，像什麼都沒發生過。
//   現在考慮搬到一半出事：
//     * 用「複製」搬：舊區塊完好無損。丟例外時只要銷毀新區塊、
//       釋放它，vector 就回到原狀 —— 強保證達成。
//     * 用「移動」搬：舊元素已經被掏空了。丟例外時新的沒搬完、
//       舊的也回不去 —— 根本無法還原。
//   所以 vector 的策略是：只有在「移動保證不會丟例外」時才用移動。
//   這個判斷就是靠 noexcept，標準庫用 std::move_if_noexcept 實作：
//       T 的移動建構子是 noexcept  → 回傳右值 → 走移動（快）
//       否則且 T 可複製            → 回傳左值 → 走複製（慢，但安全）
//
// 【3. 這件事的實際代價有多大】
//   對持有堆積資源的型別（std::string、std::vector 成員…），
//   移動只是搬幾個指標（O(1)），複製卻要重新配置並拷貝全部內容（O(size)）。
//   忘記寫 noexcept，等於讓每次擴容從「搬指標」變成「深拷貝」，
//   在大型容器上可能是數量級的差距。
//   本檔下方會用「實際計數的複製次數 vs 移動次數」把這件事量化出來。
//
// 【4. 那我該怎麼做】
//   自訂型別的移動建構子與移動賦值，只要真的不會丟例外，就標上 noexcept。
//   多數情況它們只是搬指標與置空，本來就不該丟。可以用
//   static_assert(std::is_nothrow_move_constructible_v<MyType>) 主動把關。
//
// 【概念補充 Concept Deep Dive】
//   注意 move_if_noexcept 的完整條件是「移動不丟例外 或 型別不可複製」。
//   也就是說，若型別「只能移動不能複製」（例如成員有 std::unique_ptr），
//   即使移動建構子沒標 noexcept，vector 仍然只能用移動——因為沒有別的選擇。
//   此時強例外保證就降級為基本保證（basic guarantee）。所以「沒 noexcept
//   就一定會複製」這句話並不精確，正確說法是「可複製時才會退化成複製」。
//
//   另外，本檔輸出中 emplace_back(2) 那一段會看到「移動 1」——
//   這是舊有的第 1 個元素被搬到新區塊，不是新加入的元素在移動。
//   讀輸出時要分清楚「新元素的建構」與「舊元素的搬遷」。
//
// 【注意事項 Pay Attention】
// 1. 移動後的來源物件處於「有效但未指定」（valid but unspecified）狀態。
//    可以安全解構、可以重新賦值，但不可假設它的內容是什麼。
//    本檔 Tracker 的移動建構子沒有清空 other.id，所以解構時仍印出原值，
//    這是刻意保留原始教材寫法——實務上建議把來源置為明確的空狀態。
// 2. 「沒有 noexcept 就會複製」的前提是該型別可複製；只能移動的型別
//    仍然會用移動（見概念補充）。
// 3. 擴容會使所有迭代器／指標／參考失效。
// 4. 成長倍率是實作定義；本機 libstdc++ 為 2 倍，所以第 3 個元素加入時
//    capacity 由 2 變 4，會看到 2 個舊元素被搬遷。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】擴容代價與 noexcept
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼移動建構子一定要標 noexcept？不標會怎樣？
//     答：因為 vector 的 push_back 要提供強例外保證。搬移過程若可能丟例外，
//         用移動就無法回滾（舊元素已被掏空），所以標準庫用
//         std::move_if_noexcept 判斷：沒有 noexcept 且型別可複製時，
//         擴容會退化成「複製」。對持有堆積資源的型別，這是 O(1) 變 O(size)。
//     追問：那只能移動、不能複製的型別呢？→ 那就沒有退路，仍然用移動，
//         但強例外保證降級為基本保證。
//
// 🔥 Q2. 什麼是強例外保證？vector::push_back 為什麼要提供它？
//     答：強保證是「操作要嘛完全成功，要嘛容器維持原狀」，也就是
//         commit-or-rollback。push_back 提供它，呼叫端才能在例外發生後
//         安全地繼續使用容器，不必擔心處於半殘狀態。
//     追問：那 vector 有哪些操作只有基本保證？→ 例如帶區間的 insert、
//         以及元素型別的移動可能丟例外時的一些路徑；基本保證只承諾
//         「沒有資源洩漏、物件仍處於有效狀態」，但內容可能已被改變。
//
// ⚠️ 陷阱. 「我的類別只是包了幾個 int，移動和複製一樣快，noexcept 加不加無所謂」
//     答：對這個類別本身也許成立，但一旦它被當成成員放進別的類別，
//         或被放進需要擴容的容器，缺少 noexcept 會沿著型別往上傳染：
//         外層類別的隱式移動建構子也會變成 potentially-throwing，
//         於是整條鏈上的容器全部退化成複製。
//     為什麼會錯：只看單一類別的複製成本，忽略了 noexcept 是「會傳染的
//         型別屬性」。編譯器推導外層型別的 noexcept 時，取決於所有成員；
//         一個成員沒標，整個外層就失去資格。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】從缺。
//   理由：本檔主題是「擴容時的例外安全與移動語意」，屬於資源管理與
//   標準庫實作機制。LeetCode 使用的都是 int／string 等內建或標準型別，
//   不涉及自訂型別的 noexcept 標註，也不會因為複製或移動而判定對錯。
//   清單中沒有任何一題觸及這個主題，故從缺，改以實務範例量化 noexcept 的影響。

#include <vector>
#include <iostream>
#include <utility>
#include <type_traits>
#include <string>

class Tracker {
public:
    int id;
    Tracker(int i) : id(i) { 
        std::cout << "建構 " << id << std::endl; 
    }
    Tracker(const Tracker& other) : id(other.id) { 
        std::cout << "複製 " << id << std::endl; 
    }
    Tracker(Tracker&& other) noexcept : id(other.id) { 
        std::cout << "移動 " << id << std::endl; 
    }
    ~Tracker() { 
        std::cout << "銷毀 " << id << std::endl; 
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】為自訂型別標上 noexcept：用「計數」量化它的影響
//   場景：專案裡自己寫的資源持有型別（封包、影像框、資料庫記錄…），
//         幾乎一定會被放進 std::vector 批次處理。
//   為什麼重要：移動建構子少標一個 noexcept，vector 每次擴容就會從
//     「搬指標」退化成「深拷貝」。對持有 std::string／堆積緩衝的型別，
//     這是 O(1) 變 O(size) 的差別，而且完全不會有任何編譯警告。
//   下面兩個類別除了 noexcept 之外「完全相同」，用實際計數對照結果。
//   （刻意計數而非計時：計數可重現，且直接對應標準庫的選擇機制。）
// -----------------------------------------------------------------------------
class FastMove {          // 移動建構子有 noexcept
public:
    std::string payload;
    inline static int copies = 0;   // C++17 inline static member
    inline static int moves  = 0;

    explicit FastMove(std::string s) : payload(std::move(s)) {}
    FastMove(const FastMove& o) : payload(o.payload) { ++copies; }
    FastMove(FastMove&& o) noexcept : payload(std::move(o.payload)) { ++moves; }
    FastMove& operator=(const FastMove&) = default;
    FastMove& operator=(FastMove&&) = default;
    ~FastMove() = default;
};

class SlowMove {          // 完全相同，只差在移動建構子「沒有」noexcept
public:
    std::string payload;
    inline static int copies = 0;
    inline static int moves  = 0;

    explicit SlowMove(std::string s) : payload(std::move(s)) {}
    SlowMove(const SlowMove& o) : payload(o.payload) { ++copies; }
    SlowMove(SlowMove&& o) : payload(std::move(o.payload)) { ++moves; }
    SlowMove& operator=(const SlowMove&) = default;
    SlowMove& operator=(SlowMove&&) = default;
    ~SlowMove() = default;
};

template <typename T>
static void fillAndReport(const char* label) {
    T::copies = 0;
    T::moves  = 0;
    {
        std::vector<T> v;                 // 刻意不 reserve，讓擴容自然發生
        for (int i = 0; i < 5; ++i) {
            v.push_back(T("payload-" + std::to_string(i)));
        }
    }
    // 說明數字怎麼來的（推入 5 個元素、不 reserve）：
    //   capacity 走 1→2→4→8，三次擴容分別搬 1、2、4 個舊元素 = 7 次「搬遷」；
    //   另外 5 次 push_back(T&&) 把臨時物件搬進容器 = 5 次「插入移動」。
    //   FastMove : 7 次搬遷全走移動 + 5 次插入移動 = 移動 12、複製 0
    //   SlowMove : 7 次搬遷退化成複製 + 5 次插入移動 = 複製 7、移動 5
    std::cout << "  " << label
              << " | is_nothrow_move_constructible = " << std::boolalpha
              << std::is_nothrow_move_constructible_v<T> << std::noboolalpha
              << " | 全程複製 " << T::copies
              << " 次、移動 " << T::moves << " 次" << std::endl;
}

int main() {
    std::cout << "=== 擴容過程逐步觀察（Tracker）===" << std::endl;
    std::vector<Tracker> v;

    std::cout << "--- 加入第 1 個 ---" << std::endl;
    v.emplace_back(1);

    std::cout << "--- 加入第 2 個 ---" << std::endl;
    v.emplace_back(2);
    // 此時 capacity 由 1 變 2：舊的第 1 個元素被「移動」到新區塊，再銷毀舊的

    std::cout << "--- 加入第 3 個 ---" << std::endl;
    v.emplace_back(3);
    // capacity 由 2 變 4：舊的兩個元素都被搬遷

    std::cout << "--- 程式結束 ---" << std::endl;

    // ---- 日常實務：noexcept 的實際影響 ----
    std::cout << "\n=== 日常實務: noexcept 對擴容策略的影響 ===" << std::endl;
    std::cout << "兩個類別內容完全相同，只差移動建構子有沒有 noexcept：" << std::endl;
    fillAndReport<FastMove>("FastMove（有 noexcept）");
    fillAndReport<SlowMove>("SlowMove（無 noexcept）");
    std::cout << "結論：沒有 noexcept 時，vector 為了維持強例外保證，"
                 "擴容時改用複製。" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 9 課：vector 的內部結構與記憶體配置3.cpp -o vector_realloc_cost

// 【但書】「複製 7 次／移動 12 次」等數字取決於 libstdc++ 的 2 倍成長策略
//   （capacity 1→2→4→8）。在成長倍率不同的標準庫（如 MSVC 約 1.5 倍）上，
//   擴容次數與搬遷筆數都會不同，但「無 noexcept ⇒ 搬遷退化成複製」的結論不變。

// === 預期輸出 ===
// === 擴容過程逐步觀察（Tracker）===
// --- 加入第 1 個 ---
// 建構 1
// --- 加入第 2 個 ---
// 建構 2
// 移動 1
// 銷毀 1
// --- 加入第 3 個 ---
// 建構 3
// 移動 1
// 銷毀 1
// 移動 2
// 銷毀 2
// --- 程式結束 ---
//
// === 日常實務: noexcept 對擴容策略的影響 ===
// 兩個類別內容完全相同，只差移動建構子有沒有 noexcept：
//   FastMove（有 noexcept） | is_nothrow_move_constructible = true | 全程複製 0 次、移動 12 次
//   SlowMove（無 noexcept） | is_nothrow_move_constructible = false | 全程複製 7 次、移動 5 次
// 結論：沒有 noexcept 時，vector 為了維持強例外保證，擴容時改用複製。
// 銷毀 1
// 銷毀 2
// 銷毀 3
