// =============================================================================
//  第 11 課：vector 的容量管理：size、capacity、reserve5.cpp
//  —  reserve vs resize 正面對照：最常見的 vector 誤用就在這裡
// =============================================================================
//
// 【主題資訊 Information】
//   void reserve(size_type n);   // 只確保 capacity >= n；size 不變；不建構元素
//   void resize (size_type n);   // 把 size 變成 n；擴大時建構、縮小時銷毀
//
//   標頭檔：<vector>
//   標準版本：兩者皆 C++98 起；C++20 起為 constexpr。
//   複雜度：reserve → 只在 n > capacity() 時付出 O(size()) 搬移；
//           resize  → O(|n - size()|)，加上可能的 O(size()) 搬移。
//
//   對照表（本課核心）：
//   ┌──────────────┬───────────┬───────────────────────┬──────────────┐
//   │ 函式          │ 改變 size │ 改變 capacity          │ 建構元素      │
//   ├──────────────┼───────────┼───────────────────────┼──────────────┤
//   │ reserve(n)   │ 否        │ 是（僅當 n > capacity）│ 否            │
//   │ resize(n)    │ 是        │ 可能（若 n > capacity）│ 是（擴大時）  │
//   └──────────────┴───────────┴───────────────────────┴──────────────┘
//
// 【詳細解釋 Explanation】
//
// 【1. 一句話記住差別】
// **reserve 準備「空間」，resize 準備「元素」。**
// reserve(5) 之後 vector 手上有一塊放得下 5 個 int 的記憶體，但裡面**沒有
// 任何 int 物件**——那是未建構的原始位元組。size() 仍是 0。
// resize(5) 之後 vector 裡真的有 5 個已建構、值為 0 的 int，size() 是 5。
//
// 【2. 為什麼 reserve 後 v[0] 是 UB，而不是「讀到 0」】
// operator[] 的合約是「i 必須 < size()」。reserve 後 size() 是 0，所以 v[0]
// 從一開始就違反了前置條件 —— 這是**契約違反**，不是「越界讀到垃圾」。
// 兩層問題：
//   (a) 形式上：標準說 i >= size() 是 UB，編譯器可以據此做任何假設與優化。
//   (b) 實質上：那塊記憶體上沒有生命期已開始的 int 物件。即使位元組是可讀的，
//       讀取一個尚未建構的物件在物件模型上仍是 UB。
// 所以「我試過會印出 0，所以沒問題」是錯的推論 —— UB 的表現可以剛好像正確，
// 這正是它最危險的地方。本檔改用 at()（會丟 std::out_of_range，行為完全確定）
// 來安全地證明 size() 真的是 0。
//
// 【3. 什麼時候用哪個】
//   用 reserve：知道大概要放多少，但要**逐一 push_back 填入**。
//              典型：讀檔案、篩選結果、收集事件。
//   用 resize ：需要一個**固定大小、可直接用索引存取**的容器。
//              典型：影像/音訊緩衝區、固定長度的查表、要傳給 C API 的 buffer。
//   組合誤用（最常見的 bug）：
//              v.reserve(n); for (i...) v[i] = f(i);   // ← UB，size 還是 0
//              正確：v.reserve(n); for (i...) v.push_back(f(i));
//              或者：v.resize(n);  for (i...) v[i] = f(i);
//
// 【4. 為什麼不乾脆都用 resize 就好？】
// 因為 resize 會**真的建構元素**，這有兩個代價：
//   * 建構成本：resize(1000000) 對 int 要寫 4MB 的 0；對 std::string 要跑
//     100 萬次預設建構子。若你接著就要覆蓋它們，這些工都白做了。
//   * 語意錯誤：resize(n) 之後 size() 就是 n，如果你接著 push_back，
//     資料會接在那 n 個預設值**後面**，變成 2n 個元素 —— 這是另一個經典 bug。
// 所以「要 push_back 就用 reserve，要索引賦值就用 resize」不只是效能考量，
// 更是正確性考量。
//
// 【概念補充 Concept Deep Dive】
// (A) 兩者擴大 capacity 的規則不同（實作定義，但差異很實際）
//   libstdc++ 實測：
//     reserve(n) → 剛好配置 n（reserve(5) 得到 capacity 5）
//     resize(n)  → 幾何成長 max(2 * size, n)
//   本檔中兩個空 vector 分別 reserve(5) 與 resize(5)，因為 size 原本是 0，
//   max(0, 5) = 5，所以兩者 capacity 剛好都是 5、看不出差異。
//   但若原本 size 是 5，resize(8) 會得到 capacity 10 而非 8（見 4.cpp）。
//
// (B) 「未建構的記憶體」在物件模型上的意義
//   C++ 的物件有生命期（lifetime）：從建構子完成開始，到解構子開始為止。
//   reserve 只是取得 storage，並未在其上開始任何物件的生命期。存取一個
//   生命期尚未開始的物件是 UB，即使那段記憶體實體上是可讀寫的。
//   這也是為什麼 vector 內部必須用 placement new（在既有記憶體上建構），
//   而不能只是 memcpy 位元組。
//
// (C) 為什麼 at() 是安全的示範方式
//   at() 的合約明確規定 i >= size() 時丟 std::out_of_range —— 這是
//   **定義良好**的行為，不是 UB。所以用 at() 可以在不觸發 UB 的前提下
//   證明「reserve 後 size 真的是 0」。教學示範不該直接跑 UB 程式碼。
//
// 【注意事項 Pay Attention】
// 1. reserve 後用 operator[] 存取是 **UB**。它可能看起來正常、可能崩潰、
//    也可能在開了最佳化後行為改變 —— 不可依賴任何觀察到的結果。
// 2. resize 後再 push_back，資料會接在既有元素**之後**。想「配置好再填」
//    卻用了 resize，很容易得到兩倍長度的 vector。
// 3. 兩者若觸發重新配置，都會使所有 iterator/pointer/reference 失效。
// 4. 本檔 capacity 具體數值為 libstdc++ 實測；標準只保證 capacity() >= n。
// 5. 想要「配置 + 可索引存取」且元素預設值無意義（例如馬上要 memcpy 覆蓋），
//    resize 的歸零成本是浪費；此時可考慮 reserve + push_back，或改用
//    std::unique_ptr<T[]> 之類更貼近需求的工具。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】reserve vs resize
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 以下程式碼有什麼問題？
//        std::vector<int> v;
//        v.reserve(100);
//        for (int i = 0; i < 100; ++i) v[i] = i;
//     答：reserve 不改變 size，此時 size() 仍是 0，v[i] 全部是越界存取 → UB。
//         那塊記憶體只是「已配置但未建構」的原始位元組，上面沒有任何 int 物件。
//         正確寫法二選一：改用 push_back(i)，或把 reserve(100) 換成 resize(100)。
//     追問：為什麼實際跑起來「好像」沒事？
//         → UB 的表現可以剛好看似正確。記憶體確實已配置所以不一定 segfault，
//           但一旦開最佳化、換編譯器或改動周邊程式碼，行為隨時會變。
//
// 🔥 Q2. reserve(n) 和 resize(n) 之後，size 和 capacity 分別是多少？
//     答：reserve(n) → size 不變（空 vector 就是 0），capacity >= n。
//         resize(n)  → size 變成 n，capacity >= n，且新元素被 value-initialize
//         （int 為 0）。
//     追問：那兩者的 capacity 一定相同嗎？
//         → 不一定。libstdc++ 實測 reserve 剛好配置 n，resize 走幾何成長
//           max(2*size, n)。從空 vector 出發兩者恰好都是 n，但從非空出發就會分歧。
//
// ⚠️ 陷阱 1. 「我要一個能放 n 個元素的 vector」——該用 reserve 還是 resize？
//     答：看你接下來怎麼填。要 push_back 就用 reserve；要 v[i] = ... 就用 resize。
//         用錯的兩種後果：reserve + v[i] 是 UB；resize + push_back 會得到
//         2n 個元素（前 n 個是預設值）。
//     為什麼會錯：把兩者都理解成「配置空間」，只看到 capacity 這一面，
//         忽略了 resize 同時**改變 size 並建構元素**。
//
// ⚠️ 陷阱 2. `v.resize(1000000)` 之後馬上覆蓋全部內容，有什麼浪費？
//     答：resize 會先把 100 萬個元素 value-initialize（對 int 是寫 4MB 的 0，
//         對 std::string 是 100 萬次預設建構），而你接著就把它們蓋掉了。
//         若資料本來就要逐一產生，reserve + push_back 可以省掉這趟初始化。
//     為什麼會錯：以為 resize 只是「把 size 這個數字改掉」，
//         沒意識到它真的會逐一建構元素。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <stdexcept>

int main() {
    std::vector<int> v1, v2;

    v1.reserve(5);   // 只要空間
    v2.resize(5);    // 要 5 個真正的元素

    std::cout << "=== reserve(5) vs resize(5) ===" << std::endl;
    std::cout << "reserve(5): size=" << v1.size()
              << ", capacity=" << v1.capacity() << std::endl;  // size=0
    std::cout << "resize(5):  size=" << v2.size()
              << ", capacity=" << v2.capacity() << std::endl;  // size=5

    std::cout << "\n=== reserve 後 size 真的是 0（用 at() 安全驗證）===" << std::endl;
    // 不直接寫 v1[0]，那是 UB。at() 越界會丟 out_of_range，行為完全確定。
    try {
        v1.at(0) = 10;
        std::cout << "at(0) 成功（不應該發生）" << std::endl;
    } catch (const std::out_of_range&) {
        std::cout << "v1.at(0) 丟出 std::out_of_range → size 確實是 0"
                  << std::endl;
        std::cout << "（若寫成 v1[0] = 10 則是 UB，不保證會被偵測到）"
                  << std::endl;
    }

    std::cout << "\n=== resize 後可以直接用 operator[] ===" << std::endl;
    v2[0] = 10;  // 合法，因為 size 是 5
    std::cout << "v2[0] = " << v2[0] << std::endl;
    std::cout << "v2 內容: [";
    for (std::size_t i = 0; i < v2.size(); ++i) {
        if (i != 0) std::cout << ' ';
        std::cout << v2[i];
    }
    std::cout << "]" << std::endl;

    std::cout << "\n=== reserve 的正確填法：push_back ===" << std::endl;
    for (int i = 0; i < 5; ++i) v1.push_back(i * 100);
    std::cout << "push_back 5 次後: size=" << v1.size()
              << ", capacity=" << v1.capacity() << std::endl;
    std::cout << "v1 內容: [";
    for (std::size_t i = 0; i < v1.size(); ++i) {
        if (i != 0) std::cout << ' ';
        std::cout << v1[i];
    }
    std::cout << "]" << std::endl;

    std::cout << "\n=== 經典誤用：resize 之後又 push_back ===" << std::endl;
    std::vector<int> wrong;
    wrong.resize(3);            // 本意是「配置 3 個位置」
    wrong.push_back(42);        // 卻接在 3 個預設值後面
    std::cout << "resize(3) + push_back(42) → size=" << wrong.size()
              << ", 內容: [";
    for (std::size_t i = 0; i < wrong.size(); ++i) {
        if (i != 0) std::cout << ' ';
        std::cout << wrong[i];
    }
    std::cout << "]  ← 多了 3 個預設值" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 11 課：vector 的容量管理：size、capacity、reserve5.cpp" -o demo5

// 註：以下 capacity 具體數值為 libstdc++ / GCC 15.2 實測，非標準保證。
//     標準只保證 reserve(5)/resize(5) 後 capacity() >= 5；
//     libstdc++ 從空 vector 出發兩者剛好都配置 5，MSVC 可能不同。

// === 預期輸出 ===
// === reserve(5) vs resize(5) ===
// reserve(5): size=0, capacity=5
// resize(5):  size=5, capacity=5
//
// === reserve 後 size 真的是 0（用 at() 安全驗證）===
// v1.at(0) 丟出 std::out_of_range → size 確實是 0
// （若寫成 v1[0] = 10 則是 UB，不保證會被偵測到）
//
// === resize 後可以直接用 operator[] ===
// v2[0] = 10
// v2 內容: [10 0 0 0 0]
//
// === reserve 的正確填法：push_back ===
// push_back 5 次後: size=5, capacity=5
// v1 內容: [0 100 200 300 400]
//
// === 經典誤用：resize 之後又 push_back ===
// resize(3) + push_back(42) → size=4, 內容: [0 0 0 42]  ← 多了 3 個預設值
