// =============================================================================
//  第 18 課-14：實務決策指南 —— 什麼時候可以用 vector<bool>，什麼時候絕對不要
// =============================================================================
//
// 【主題資訊 Information】
//   本檔上半部是第 18 課的完整講義（保留在下方的大註解區塊中），
//   下半部是可執行的「四種存法對照 + 決策流程」示範。
//   四個候選方案的能力矩陣：
//                        vector<bool>  deque<bool>  vector<uint8_t>  bitset<N>
//     記憶體 (N 個旗標)     N/8 bytes    N bytes       N bytes        N/8 bytes
//     取得 T& / T*             ✗            ✓             ✓             ✗
//     data()（交給 C API）     ✗            ✗             ✓             ✗
//     泛型模板相容             ✗            ✓             ✓            不適用
//     位元運算 (& | ^ ~)       ✗            ✗           需自己寫          ✓
//     大小可執行期決定         ✓            ✓             ✓             ✗
//   標準版本：全部 C++98 起（uint8_t 需 <cstdint>，C++11 起納入標準）
//   標頭檔：<vector>、<deque>、<bitset>、<cstdint>
//
// 【詳細解釋 Explanation】
//
// 【1. 先問三個問題，答案就出來了】
//   (Q1) 大小是編譯期就固定的嗎？
//        是 → 而且需要位元運算 → std::bitset<N>（最佳）
//   (Q2) 需要把整塊資料交給 C API 嗎（memcpy / fwrite / send）？
//        是 → vector<uint8_t>（唯一選擇；必要時自己做位元打包）
//   (Q3) 這份資料會被傳進泛型模板、或需要 T& / T* 嗎？
//        是 → 避開 vector<bool>；用 vector<uint8_t>，
//             若同時需要頻繁頭尾增刪才考慮 deque<bool>
//   三題都是「否」，而且記憶體真的是瓶頸 → 這時 vector<bool> 才是合理選擇。
//
// 【2. 用 vector<bool> 時必須遵守的三條紀律】
//   (a) 存取一律明確寫 bool，絕不用 auto
//         bool v = flags[i];      ✓ 獨立副本
//         auto v = flags[i];      ✗ 得到代理物件，會跟著容器變動
//   (b) 不要試圖取位址或引用——編譯器會擋，別用 auto&& 硬繞過去，
//       除非你很清楚自己在操作代理物件。
//   (c) 不要把它傳進你沒讀過原始碼的模板。
//       模板內部只要有一行 T& elem = v[i]; 就會炸，
//       而錯誤訊息會指向模板內部，離你的呼叫點很遠。
//
// 【3. 為什麼「省 8 倍記憶體」常常是假議題】
//   一百萬個旗標，vector<bool> 約 125 KB、vector<uint8_t> 約 1 MB，
//   差約 875 KB。在一台有 16 GB 記憶體的機器上，這個差距通常不值得
//   換來前面五個陷阱。
//   真正該用位元壓縮的是「億級以上」的規模，
//   或記憶體極度受限的嵌入式環境——而後者往往連 STL 都不用。
//   更關鍵的是：如果只是要省記憶體，用 vector<uint8_t> 自己位元打包
//   可以同時保有 data()，比 vector<bool> 全面更好。
//
// 【4. 這一課真正要學到的東西】
//   表面上是 vector<bool> 的五個陷阱，本質上是三個更通用的觀念：
//     (a) 代理物件（proxy object）——當底層表示與介面型別不一致時，
//         標準庫用代理來架橋，代價是型別不再直觀。
//         這個模式在 std::bitset、Eigen、表達式模板中大量出現。
//     (b) 特化不該改變介面語意——改變實作可以，改變契約會毀掉泛型。
//     (c) auto 推導的是「運算式的型別」，不是「你以為的那個型別」。
//   這三點的適用範圍遠超過 vector<bool> 本身。
//
// 【概念補充 Concept Deep Dive】
//   ▸ 為什麼標準至今不移除它
//     ABI 與原始碼相容性。已有大量既存程式碼依賴 vector<bool>，
//     移除它會讓那些程式碼無法編譯。
//     Herb Sutter 曾提案移除，最終未通過；
//     現在的共識是「它應該叫 bit_vector」——
//     錯在名字讓人以為它滿足 vector 的泛型契約，不在位元壓縮本身。
//   ▸ 一個判斷「這個容器有沒有被特化」的通用技巧
//     檢查 std::is_same_v<decltype(v[0]), T&>。
//     對正常容器成立，對 vector<bool> 不成立。
//     這比直接判斷 T 是否為 bool 更貼近問題本質——
//     問題出在 operator[] 的回傳型別，不在元素型別。
//   ▸ 多執行緒下的額外風險
//     vector<bool> 相鄰元素共用同一個字組，
//     兩個執行緒分別寫 v[0] 與 v[1] 會對同一個 word 做讀改寫，
//     形成 data race。而對 vector<int> 寫不同元素則完全安全。
//     這一點在並行程式中特別致命，卻很少被提及。
//
// 【注意事項 Pay Attention】
//   1. 用 vector<bool> 時，存取一律明確寫 bool，絕不用 auto。
//   2. 不要把 vector<bool> 傳進你沒讀過原始碼的泛型模板。
//   3. 需要 data() 就只能用 vector<uint8_t>（或自己位元打包）。
//   4. 大小編譯期固定 + 需要位元運算 → std::bitset<N> 是更好的選擇。
//   5. vector<bool> 相鄰元素共用字組，多執行緒寫入不同元素也會有 data race。
//   6. 「省 8 倍記憶體」在多數應用中不值得換來這些陷阱；
//      真要省，用 vector<uint8_t> 自己打包可同時保有 data()。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector<bool> 的實務決策
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 專案裡要存一百萬個布林旗標，你會怎麼選容器？
//     答：先問三個問題。(1) 大小編譯期固定且需要位元運算 → std::bitset；
//         (2) 需要整塊交給 C API → vector<uint8_t>（必要時自己位元打包）；
//         (3) 會被傳進泛型模板或需要 T&/T* → 避開 vector<bool>。
//         三題皆否、且記憶體確實是瓶頸，才選 vector<bool>。
//         一百萬個旗標的差距只有約 875 KB，多數情況下
//         不值得換來那些陷阱。
//     追問：如果既要省記憶體又要能交給 C API 呢？
//         → 用 vector<uint8_t> 自己做位元打包（每 byte 存 8 個旗標）。
//           這樣壓縮格式就是輸出格式，data() 照樣可用，
//           比 vector<bool> 全面更好。
//
// 🔥 Q2. 這一課除了「別用 vector<bool>」，還有什麼更通用的收穫？
//     答：三個。(1) 代理物件模式——當底層表示與介面型別不一致時，
//         標準庫用代理架橋，代價是型別不再直觀（bitset、Eigen 都用這招）；
//         (2) 特化可以改變實作，但不該改變介面語意，否則會毀掉泛型；
//         (3) auto 推導的是「運算式的型別」，不是「你以為的那個型別」。
//     追問：怎麼在程式中判斷一個容器有沒有被特化？
//         → 檢查 std::is_same_v<decltype(v[0]), T&>。
//           這比判斷 T 是否為 bool 更貼近問題本質，
//           因為問題出在 operator[] 的回傳型別，不在元素型別。
//
// ⚠️ 陷阱. 「vector<bool> 的問題我都知道了，但我這份資料只會被
//          兩個執行緒各自寫入不同的索引，應該沒有並行問題吧？」
//     答：有，而且很嚴重。vector<bool> 把相鄰的多個元素壓在同一個字組裡，
//         執行緒 A 寫 v[0]、執行緒 B 寫 v[1] 時，
//         兩者都在對「同一個 word」做讀改寫，這是貨真價實的 data race
//         （undefined behavior），ThreadSanitizer 會直接報出來。
//         換成 vector<int> 或 vector<uint8_t> 就完全沒有這個問題——
//         標準明文保證「同時修改容器的不同元素」是安全的，
//         而 vector<bool> 正是這條保證的例外。
//     為什麼會錯：以為「不同索引 = 不同記憶體位置」。
//         對所有正常容器這都成立，唯獨 vector<bool> 因為位元壓縮，
//         多個邏輯元素實際共用同一個實體位置。
// ═══════════════════════════════════════════════════════════════════════════

/*
# 第 18 課：vector\<bool> 的特殊性與陷阱

## 一、一個看似正常的容器

如果你按照前面學的 vector 知識，寫出這樣的程式碼，看起來完全合理：

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<bool> vb = {true, false, true, true, false};

    for (bool b : vb) {
        std::cout << b << " ";
    }
    std::cout << std::endl;

    return 0;
}
```

**輸出：**
```
1 0 1 1 0
```

一切正常。但當你開始深入使用，就會發現 `vector<bool>` 跟其他 `vector<T>` **根本不是同一種東西**。

---

## 二、vector\<bool> 是一個特化版本

C++ 標準對 `vector<bool>` 做了**模板特化（template specialization）**。普通的 `vector<T>` 每個元素佔一個 `T` 的空間，但 `vector<bool>` 用**每個位元（bit）存一個 bool**，而不是每個位元組（byte）。

### 2.1 記憶體節省的動機

```cpp
#include <iostream>
#include <vector>

int main() {
    const int N = 1000;

    // 普通的 bool 陣列：每個 bool 佔 1 byte
    bool arr[N];
    std::cout << "bool[1000] 大小：" << sizeof(arr) << " bytes" << std::endl;

    // vector<char> 模擬存 bool：每個也佔 1 byte
    std::vector<char> vc(N);
    std::cout << "vector<char>(1000) 容量佔用：約 "
              << vc.capacity() * sizeof(char) << " bytes" << std::endl;

    // vector<bool>：每個 bool 只佔 1 bit
    std::vector<bool> vb(N);
    std::cout << "vector<bool>(1000) 容量佔用：約 "
              << vb.capacity() / 8 << " bytes" << std::endl;

    return 0;
}
```

**輸出：**
```
bool[1000] 大小：1000 bytes
vector<char>(1000) 容量佔用：約 1000 bytes
vector<bool>(1000) 容量佔用：約 128 bytes
```

記憶體節省了大約 **8 倍**。在需要存儲大量布林值的場景（例如篩法求質數、位元圖），這是有意義的。

### 2.2 位元壓縮的內部結構

```
普通 vector<int> 的記憶體佈局（每個元素獨立佔空間）：
┌──────┬──────┬──────┬──────┐
│  10  │  20  │  30  │  40  │  每個佔 4 bytes
└──────┴──────┴──────┴──────┘

vector<bool> 的記憶體佈局（位元壓縮）：
┌─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┐
│1│0│1│1│0│0│1│0│1│1│0│1│0│0│1│1│  一個 byte 存 8 個 bool
└─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┘
 ↑ 第0個       ↑ 第7個
```

---

## 三、核心問題：operator[] 不回傳 bool&

這是 `vector<bool>` 最根本的問題。在普通 vector 中：

```cpp
std::vector<int> vi = {10, 20, 30};
int& ref = vi[0];    // 取得真正的引用，指向記憶體中的 int
ref = 99;            // 直接修改記憶體
```

但在 `vector<bool>` 中，一個 bool 只佔 1 個位元，而 C++ 中**不可能有指向單一位元的引用或指標**。最小的可定址單位是 1 byte。

所以 `vector<bool>::operator[]` 回傳的不是 `bool&`，而是一個叫做 `vector<bool>::reference` 的**代理物件（proxy object）**：

```cpp
#include <iostream>
#include <vector>
#include <typeinfo>

int main() {
    std::vector<int> vi = {1, 2, 3};
    std::vector<bool> vb = {true, false, true};

    // vector<int>::operator[] 回傳 int&
    auto ri = vi[0];   // auto 推導為 int
    std::cout << "vi[0] 的型別：" << typeid(ri).name() << std::endl;

    // vector<bool>::operator[] 回傳 reference（代理物件）
    auto rb = vb[0];   // auto 推導為 vector<bool>::reference，不是 bool！
    std::cout << "vb[0] 的型別：" << typeid(rb).name() << std::endl;

    // 大小也不同
    std::cout << "sizeof(ri) = " << sizeof(ri) << std::endl;
    std::cout << "sizeof(rb) = " << sizeof(rb) << std::endl;

    return 0;
}
```

**輸出（GCC，型別名稱經 demangle）：**
```
vi[0] 的型別：i
vb[0] 的型別：St14_Bit_reference
sizeof(ri) = 4
sizeof(rb) = 16
```

`auto rb = vb[0]` 得到的不是 `bool`，而是一個 16 bytes 的代理物件！

---

## 四、代理物件帶來的陷阱

### 4.1 陷阱一：auto 推導出錯誤型別

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<bool> vb = {true, false, true};

    // 陷阱：auto 推導為 proxy，不是 bool
    auto val = vb[0];   // 型別是 vector<bool>::reference

    vb[0] = false;      // 修改原容器

    // val 是代理物件，它「連結」到 vb 的內部
    // 所以 val 也跟著變了！
    std::cout << "val = " << val << std::endl;  // 0（false），不是預期的 1（true）

    // 正確做法：明確宣告為 bool
    bool val2 = vb[1];  // 強制轉型為 bool，取得獨立的副本
    vb[1] = true;
    std::cout << "val2 = " << val2 << std::endl;  // 0（false），不受影響

    return 0;
}
```

**輸出：**
```
val = 0
val2 = 0
```

### 4.2 陷阱二：無法取得元素的指標

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> vi = {10, 20, 30};
    int* pi = &vi[0];  // 完全合法，取得元素的位址
    std::cout << "*pi = " << *pi << std::endl;

    std::vector<bool> vb = {true, false, true};
    // bool* pb = &vb[0];  // 編譯錯誤！
    // vb[0] 回傳的是代理物件，不是 bool
    // 你不能對代理物件取位址來得到 bool*

    return 0;
}
```

**編譯錯誤訊息（GCC）：**
```
error: cannot convert 'std::vector<bool>::reference*' to 'bool*'
```

### 4.3 陷阱三：無法綁定到 bool&

```cpp
#include <iostream>
#include <vector>

void set_true(bool& b) {
    b = true;
}

int main() {
    std::vector<int> vi = {0, 0, 0};
    int& ref = vi[0];  // OK
    ref = 42;

    std::vector<bool> vb = {false, false, false};
    // bool& ref_b = vb[0];  // 編譯錯誤！
    // vb[0] 是代理物件，不能綁定到 bool&

    // set_true(vb[0]);  // 編譯錯誤！
    // 函數參數是 bool&，不接受代理物件

    return 0;
}
```

### 4.4 陷阱四：data() 不可用

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> vi = {10, 20, 30};
    int* data_i = vi.data();  // OK，取得底層陣列指標
    std::cout << "vi.data()[0] = " << data_i[0] << std::endl;

    std::vector<bool> vb = {true, false, true};
    // bool* data_b = vb.data();  // 編譯錯誤！
    // vector<bool> 沒有 data() 成員函數
    // 因為底層不是 bool 陣列，而是位元壓縮的結構

    return 0;
}
```

### 4.5 陷阱五：與模板程式碼的衝突

這是最隱蔽的問題。一段完美泛型的程式碼，碰到 `vector<bool>` 就壞了：

```cpp
#include <iostream>
#include <vector>

template <typename T>
void process(std::vector<T>& v) {
    // 對每個元素取引用來修改
    for (size_t i = 0; i < v.size(); ++i) {
        T& elem = v[i];   // 對 vector<int> OK
                           // 對 vector<bool> 編譯錯誤！
        // ... 處理 elem ...
    }
}

int main() {
    std::vector<int> vi = {1, 2, 3};
    process(vi);   // OK

    std::vector<bool> vb = {true, false, true};
    // process(vb);  // 編譯錯誤！T& elem = v[i] 對 bool 行不通

    return 0;
}
```

---

## 五、vector\<bool> 的特有操作

雖然有很多陷阱，`vector<bool>` 也提供了一個其他 vector 沒有的操作：

### 5.1 flip()：翻轉位元

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<bool> vb = {true, false, true, true, false};

    std::cout << "翻轉前：";
    for (bool b : vb) std::cout << b;
    std::cout << std::endl;

    // 翻轉單一元素（透過代理物件的 flip）
    vb[0].flip();
    std::cout << "vb[0].flip()：";
    for (bool b : vb) std::cout << b;
    std::cout << std::endl;

    // 翻轉全部（使用 flip 成員函數）
    vb.flip();
    std::cout << "vb.flip()：";
    for (bool b : vb) std::cout << b;
    std::cout << std::endl;

    return 0;
}
```

**輸出：**
```
翻轉前：10110
vb[0].flip()：00110
vb.flip()：11001
```

---

## 六、替代方案

既然 `vector<bool>` 有這麼多問題，實務上該怎麼辦？

### 6.1 方案一：std::deque\<bool>

`deque<bool>` 沒有被特化，行為完全正常：

```cpp
#include <iostream>
#include <deque>

int main() {
    std::deque<bool> db = {true, false, true};

    // 正常的 bool 引用
    bool& ref = db[0];
    ref = false;
    std::cout << "db[0] = " << db[0] << std::endl;  // 0

    // 可以取位址
    bool* ptr = &db[1];
    std::cout << "*ptr = " << *ptr << std::endl;     // 0

    return 0;
}
```

**輸出：**
```
db[0] = 0
*ptr = 0
```

### 6.2 方案二：vector\<char> 或 vector\<uint8_t>

用整數型別代替 bool：

```cpp
#include <iostream>
#include <vector>
#include <cstdint>

int main() {
    // 用 char 或 uint8_t 代替 bool
    std::vector<uint8_t> vb = {1, 0, 1, 1, 0};

    uint8_t& ref = vb[0];  // 正常的引用
    uint8_t* ptr = &vb[0]; // 正常的指標
    uint8_t* data = vb.data(); // data() 可用

    // 與 C API 互操作也沒問題
    std::cout << "data[2] = " << static_cast<int>(data[2]) << std::endl;

    return 0;
}
```

**輸出：**
```
data[2] = 1
```

### 6.3 方案三：std::bitset（固定大小）

如果大小在編譯期已知，`bitset` 是更好的選擇：

```cpp
#include <iostream>
#include <bitset>

int main() {
    // 大小必須是編譯期常數
    std::bitset<8> bs;

    bs[0] = 1;
    bs[3] = 1;
    bs[7] = 1;

    std::cout << "bs = " << bs << std::endl;
    std::cout << "count（1 的個數）= " << bs.count() << std::endl;

    bs.flip();
    std::cout << "flip 後：" << bs << std::endl;

    // 位元運算
    std::bitset<8> bs2("11001100");
    std::cout << "bs AND bs2 = " << (bs & bs2) << std::endl;

    return 0;
}
```

**輸出：**
```
bs = 10001001
count（1 的個數）= 3
flip 後：01110110
bs AND bs2 = 01000100
```

### 6.4 方案比較總覽

| 方案 | 記憶體效率 | 標準容器行為 | 大小彈性 | 位元運算 |
|------|-----------|-------------|---------|---------|
| `vector<bool>` | 高（1 bit/bool） | 不完整 | 動態 | 有 flip |
| `deque<bool>` | 低（1 byte/bool） | 完整 | 動態 | 無 |
| `vector<char>` | 低（1 byte/bool） | 完整 | 動態 | 無 |
| `std::bitset<N>` | 高（1 bit/bool） | 非容器 | 固定 | 完整 |
| `boost::dynamic_bitset` | 高（1 bit/bool） | 部分 | 動態 | 完整 |

---

## 七、為什麼這是一個「歷史錯誤」？

`vector<bool>` 的特化被許多 C++ 專家認為是標準庫中的設計失誤。以下是幾位重要人物的觀點：

**Scott Meyers**（《Effective STL》作者）在 Item 18 中明確寫道：

> 避免使用 `vector<bool>`。

他的理由是 `vector<bool>` 不滿足 STL 容器的基本要求。標準規定，如果 `c` 是一個 `T` 物件的容器，且 `c` 支援 `operator[]`，那麼 `T* p = &c[0]` 必須合法。`vector<bool>` 違反了這條規則。

**Herb Sutter**（前 C++ 標準委員會主席）也發表過類似觀點，指出 `vector<bool>` 是一個位元壓縮的容器，戴著 vector 的面具，卻無法完全履行 vector 的契約。

問題在於，當年（1998 年制定 C++98 標準時）委員會認為位元壓縮是一個值得的優化，所以直接特化了 `vector<bool>`。但事後證明，這個決定帶來的相容性問題遠超過記憶體節省的好處。

C++ 標準委員會後來也承認了這個問題。在一份文件（N2050）中，他們探討了取消這個特化的可能性，但因為向後相容性的考量，至今仍未改變。

---

## 八、實務建議

```cpp
#include <iostream>
#include <vector>
#include <cstdint>
#include <deque>

int main() {
    // 1. 需要大量布林值且只做簡單讀寫 → vector<bool> 可以接受
    std::vector<bool> flags(1'000'000, false);
    flags[42] = true;
    // 注意：不要用 auto，明確寫 bool
    bool val = flags[42];  // OK
    // auto val2 = flags[42];  // 危險！得到代理物件

    // 2. 需要標準容器行為（取引用、指標、傳給模板）→ 避免 vector<bool>
    std::vector<uint8_t> safe_flags(1000, 0);
    uint8_t& ref = safe_flags[0];  // 正常引用
    uint8_t* ptr = safe_flags.data();  // 正常指標

    // 3. 在泛型程式碼中，永遠不要假設 vector<T> 的行為是統一的
    // 如果你寫的模板可能被 bool 實例化，要特別注意

    return 0;
}
```

**核心原則：**

- 寫 `vector<bool>` 時，**永遠不要用 `auto` 接收 `operator[]` 的結果**，明確寫 `bool`
- 如果你需要完整的容器語意（取引用、取指標、傳給期望 `T&` 的函數），**不要用 `vector<bool>`**
- 在模板程式碼中，如果 `T` 可能是 `bool`，要考慮 `vector<bool>` 的特殊行為
- 如果你真的需要位元層級的壓縮和操作，`bitset` 或 `boost::dynamic_bitset` 是更好的選擇

---

## 九、本課重點回顧

1. **`vector<bool>` 是模板特化**：每個 bool 只佔 1 bit，節省 8 倍記憶體
2. **`operator[]` 回傳代理物件**：不是 `bool&`，而是 `vector<bool>::reference`
3. **五大陷阱**：`auto` 推導錯誤、無法取指標、無法綁定 `bool&`、`data()` 不可用、破壞模板泛型性
4. **特有操作**：`flip()` 可翻轉位元
5. **替代方案**：`deque<bool>`、`vector<char>`、`std::bitset`
6. **歷史教訓**：被廣泛認為是 C++ 標準庫的設計失誤
7. **實務原則**：存取時明確寫 `bool` 而非 `auto`，需要完整容器行為時避免使用

---

## 課後練習

**練習一：代理物件觀察**
建立 `vector<bool> vb = {true, true, false, true}`。分別用 `auto` 和 `bool` 接收 `vb[0]` 的值，然後修改 `vb[0] = false`，觀察兩個變數的值是否不同。解釋原因。

**練習二：替代方案比較**
寫一個程式，分別用 `vector<bool>` 和 `vector<uint8_t>` 建立一百萬個布林值，比較兩者的記憶體使用量（用 `capacity()` 和元素大小計算）。

**練習三：泛型陷阱**
寫一個模板函數 `template<typename T> void set_first(std::vector<T>& v, const T& val)`，把 vector 的第一個元素設為 `val`。用 `vector<int>` 和 `vector<bool>` 分別呼叫，觀察哪個能編譯通過、哪個不行。然後修改模板函數，讓兩者都能正常運作。

---

準備好了就告訴我，我們進入第 19 課：**vector 與原始陣列的互操作**。那一課會教你如何在 vector 和 C 風格陣列之間安全地轉換資料。
*/



#include <bitset>
#include <cstdint>
#include <deque>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// -----------------------------------------------------------------------------
// 通用偵測：這個容器的 operator[] 是否回傳真正的 T&？
// 這比「T 是不是 bool」更貼近問題本質——問題出在回傳型別，不在元素型別。
// -----------------------------------------------------------------------------
template <typename Container, typename T>
constexpr bool yieldsRealReference() {
    return std::is_same_v<decltype(std::declval<Container&>()[0]), T&>;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】依需求挑選旗標容器的四種實作
//   情境：同一份「一百萬個使用者是否啟用某功能」的資料，
//         在四種不同的下游需求下，應該選擇不同的儲存方式。
//   本範例把決策流程寫成可執行的對照，讓取捨變得具體可量測。
// -----------------------------------------------------------------------------

// 需求 A：只做讀寫、記憶體是瓶頸 → vector<bool> 可以接受（但要守紀律）
std::size_t countEnabledBitPacked(const std::vector<bool>& flags) {
    std::size_t c = 0;
    for (std::size_t i = 0; i < flags.size(); ++i) {
        bool v = flags[i];      // 紀律：明確寫 bool，不要用 auto
        if (v) ++c;
    }
    return c;
}

// 需求 B：要交給 C API → vector<uint8_t>（唯一選擇）
extern "C" std::size_t c_count_nonzero(const std::uint8_t* p, std::size_t n) {
    std::size_t c = 0;
    for (std::size_t i = 0; i < n; ++i) c += (p[i] != 0);
    return c;
}

// 需求 C：要傳進泛型模板 → 必須用 auto&&，或乾脆避開 vector<bool>
template <typename Container>
std::size_t countTruthyGeneric(Container& c) {
    std::size_t n = 0;
    for (auto&& x : c) {        // auto&& 才能同時相容 vector<bool>
        if (x) ++n;
    }
    return n;
}

// 需求 D：大小編譯期固定 + 需要位元運算 → std::bitset
using FeatureSet = std::bitset<8>;

int main() {
    std::cout << std::boolalpha;

    std::cout << "=== 一、四種存法的能力對照（編譯期偵測）===" << std::endl;
    std::cout << "vector<int>     operator[] 回傳真正的 int&    : "
              << yieldsRealReference<std::vector<int>, int>() << std::endl;
    std::cout << "vector<uint8_t> operator[] 回傳真正的 uint8_t&: "
              << yieldsRealReference<std::vector<std::uint8_t>, std::uint8_t>() << std::endl;
    std::cout << "deque<bool>     operator[] 回傳真正的 bool&   : "
              << yieldsRealReference<std::deque<bool>, bool>() << std::endl;
    std::cout << "vector<bool>    operator[] 回傳真正的 bool&   : "
              << yieldsRealReference<std::vector<bool>, bool>()
              << "  ← 唯一的例外" << std::endl;

    std::cout << "\n=== 二、需求 A：只做讀寫、記憶體是瓶頸 ===" << std::endl;
    std::vector<bool> flags(1'000'000, false);
    flags[42] = true;
    for (std::size_t i = 0; i < flags.size(); i += 1000) flags[i] = true;

    bool val = flags[42];              // 紀律：明確寫 bool，不用 auto
    // auto val2 = flags[42];          // 危險！得到代理物件，會跟著容器變動
    std::cout << "flags[42] = " << val << std::endl;
    std::cout << "啟用數 = " << countEnabledBitPacked(flags) << std::endl;
    std::cout << "記憶體約 " << (flags.capacity() / 8 / 1024) << " KB" << std::endl;

    std::cout << "\n=== 三、需求 B：要交給 C API ===" << std::endl;
    std::vector<std::uint8_t> safe_flags(1000, 0);
    std::uint8_t& ref = safe_flags[0];        // 正常引用
    ref = 1;
    std::uint8_t* ptr = safe_flags.data();    // 正常指標，data() 可用
    for (std::size_t i = 0; i < safe_flags.size(); i += 4) safe_flags[i] = 1;

    std::cout << "safe_flags[0]（透過 ref 設定）= " << static_cast<int>(*ptr) << std::endl;
    std::cout << "交給 C 函式庫統計 = "
              << c_count_nonzero(safe_flags.data(), safe_flags.size()) << std::endl;
    std::cout << "記憶體約 " << safe_flags.capacity() << " bytes"
              << "（同樣筆數會是 vector<bool> 的 8 倍，但 data() 可用）" << std::endl;

    std::cout << "\n=== 四、需求 C：要傳進泛型模板 ===" << std::endl;
    std::vector<int>  vi = {1, 0, 3, 0, 5};
    std::vector<bool> vb = {true, false, true, true};
    std::deque<bool>  db = {true, true, false};
    std::cout << "同一個模板套在 vector<int>  → " << countTruthyGeneric(vi) << std::endl;
    std::cout << "同一個模板套在 vector<bool> → " << countTruthyGeneric(vb)
              << "（靠 auto&& 才過得了）" << std::endl;
    std::cout << "同一個模板套在 deque<bool>  → " << countTruthyGeneric(db) << std::endl;

    std::cout << "\n=== 五、需求 D：編譯期固定大小 + 位元運算 ===" << std::endl;
    FeatureSet enabled("00001101");
    FeatureSet beta   ("00110000");
    std::cout << "已啟用   = " << enabled << std::endl;
    std::cout << "beta     = " << beta << std::endl;
    std::cout << "合併     = " << (enabled | beta) << std::endl;
    std::cout << "共同項   = " << (enabled & beta) << std::endl;
    std::cout << "反向     = " << (~enabled) << std::endl;
    std::cout << "啟用數   = " << enabled.count()
              << "（vector<bool> 完全沒有這些位元運算子）" << std::endl;

    std::cout << "\n=== 六、決策流程總結 ===" << std::endl;
    std::cout << "Q1 大小編譯期固定 + 需要位元運算？ → std::bitset<N>" << std::endl;
    std::cout << "Q2 需要 data() 交給 C API？        → vector<uint8_t>" << std::endl;
    std::cout << "Q3 會進泛型模板 / 需要 T&、T*？    → 避開 vector<bool>" << std::endl;
    std::cout << "三題皆否 + 記憶體真的是瓶頸        → 才用 vector<bool>" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 18 課：vectorbool 的特殊性與陷阱14.cpp" -o vb_decision
//   註：本檔使用 std::is_same_v 與 constexpr 函式，需要 C++17（或更新）。
//
// 【關於下方預期輸出的但書】
//   記憶體的 KB 數是約略值：vector<bool> 的 capacity()/8 未計入
//   字組對齊的向上取整，vector<uint8_t> 的 capacity() 也未計入
//   配置器的額外開銷。兩者僅供量級比較，皆屬實作定義
//   （本機為 x86-64 / GCC 15.2 / libstdc++）。
//
// 【本檔未附 LeetCode 範例的理由】
//   本檔是整課的決策指南，主題是「依需求選擇容器」這個工程判斷。
//   LeetCode 的題目一律直接給定 vector，不存在容器選型的空間，
//   硬套一題無法呈現重點。與位元運算真正相關的
//   LeetCode 191（Number of 1 Bits）已放在同課的 std::bitset 範例（13.cpp），
//   在此重複只會稀釋本檔的主軸。

// === 預期輸出 ===
// === 一、四種存法的能力對照（編譯期偵測）===
// vector<int>     operator[] 回傳真正的 int&    : true
// vector<uint8_t> operator[] 回傳真正的 uint8_t&: true
// deque<bool>     operator[] 回傳真正的 bool&   : true
// vector<bool>    operator[] 回傳真正的 bool&   : false  ← 唯一的例外
//
// === 二、需求 A：只做讀寫、記憶體是瓶頸 ===
// flags[42] = true
// 啟用數 = 1001
// 記憶體約 122 KB
//
// === 三、需求 B：要交給 C API ===
// safe_flags[0]（透過 ref 設定）= 1
// 交給 C 函式庫統計 = 250
// 記憶體約 1000 bytes（同樣筆數會是 vector<bool> 的 8 倍，但 data() 可用）
//
// === 四、需求 C：要傳進泛型模板 ===
// 同一個模板套在 vector<int>  → 3
// 同一個模板套在 vector<bool> → 3（靠 auto&& 才過得了）
// 同一個模板套在 deque<bool>  → 2
//
// === 五、需求 D：編譯期固定大小 + 位元運算 ===
// 已啟用   = 00001101
// beta     = 00110000
// 合併     = 00111101
// 共同項   = 00000000
// 反向     = 11110010
// 啟用數   = 3（vector<bool> 完全沒有這些位元運算子）
//
// === 六、決策流程總結 ===
// Q1 大小編譯期固定 + 需要位元運算？ → std::bitset<N>
// Q2 需要 data() 交給 C API？        → vector<uint8_t>
// Q3 會進泛型模板 / 需要 T&、T*？    → 避開 vector<bool>
// 三題皆否 + 記憶體真的是瓶頸        → 才用 vector<bool>
