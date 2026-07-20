// =============================================================================
//  第 11 課：vector 的容量管理：size、capacity、reserve4.cpp
//  —  resize(n)：真正改變元素數量（會建構、會銷毀）
// =============================================================================
//
// 【主題資訊 Information】
//   void resize(size_type n);                       // 新元素 value-initialize
//   void resize(size_type n, const value_type& val);// 新元素複製自 val
//
//   標頭檔：<vector>
//   標準版本：C++98 起（C++11 把單參數版本從「用 val 複製」改成
//             value-initialize，見【注意事項 4】）；C++20 起為 constexpr。
//   複雜度：O(|n - size()|)（建構或銷毀的元素數）；
//           若需要擴大 capacity，再加 O(size()) 的搬移。
//   例外：n > max_size() 丟 std::length_error；配置失敗丟 std::bad_alloc。
//
//   標準規定的語意（[vector.capacity]）：
//     * n <  size()：銷毀尾端多餘元素，size 變 n。**capacity 不變**。
//     * n >  size()：在尾端追加 (n - size()) 個新元素，必要時擴大 capacity。
//     * n == size()：什麼也不做。
//
// 【詳細解釋 Explanation】
//
// 【1. resize 和 reserve 的本質差異：誰動 size】
// 一句話：**reserve 準備「空間」，resize 準備「元素」**。
//   reserve(n)：只確保 capacity >= n，size 不變，不建構任何物件。
//               那塊記憶體是未建構的原始位元組，碰它是 UB。
//   resize(n) ：真的把 size 變成 n。擴大時**建構**新元素（所以之後可以直接
//               用 v[i] 存取），縮小時**銷毀**尾端元素（呼叫解構子）。
// 這也決定了兩者的搭配用法：reserve 之後只能 push_back；resize 之後可以
// 直接 operator[] 賦值。
//
// 【2. 擴大時新元素是什麼值】
// 單參數 resize(n) 對新元素做 **value-initialization**：
//   * 內建型別（int、double、指標）→ 歸零。所以本檔補上的是 0 而不是垃圾值。
//   * 有 user-provided 預設建構子的類別 → 呼叫該建構子。
//   * 沒有 user-provided 建構子的聚合類別 → 成員逐一 value-initialize（歸零）。
// 雙參數 resize(n, val) 則是把 val **複製**給每個新元素。
// 注意這和 `new int[n]`（default-initialize，內建型別是未定值）不同 ——
// vector 的 resize 對 int 保證給 0，這是標準行為不是實作巧合。
//
// 【3. 縮小時發生什麼】
// resize(3) 會對第 4 個之後的元素**依序呼叫解構子**（對 int 是 no-op，但對
// std::string、檔案 handle 這類 RAII 型別就是真的釋放資源），然後把 size
// 改成 3。**capacity 完全不變** —— 記憶體不會還給作業系統。
// 這是刻意的設計：縮小往往只是暫時的，保留 buffer 讓後續再長回去時免配置。
// 真要釋放記憶體必須額外呼叫 shrink_to_fit()（見 6.cpp）。
//
// 【4. resize 擴大時的 capacity 成長是幾何的（和 reserve 不同！）】
// 這是本檔最容易被忽略的一點。libstdc++ 實測：
//   * reserve(n)  → 剛好配置 n（見 2.cpp：reserve(100) 得到 capacity 100）
//   * resize(n)   → 走 push_back 那套幾何成長：new_cap = max(2 * size, n)
// 所以本檔中 capacity 5 的 vector 呼叫 resize(8)，得到的 capacity 不是 8
// 而是 **10**（= max(2×5, 8)）。接著 resize(10, 99) 剛好塞滿，capacity 仍是 10。
// 理解這點才能解釋本檔輸出中那個「為什麼是 10 不是 8」的疑問。
//
// 【概念補充 Concept Deep Dive】
// (A) 為什麼縮小不釋放記憶體
//   釋放意味著「配置新的小 buffer + 搬移 + 釋放舊的」—— 縮小反而要付出
//   完整的重新配置成本，非常反直覺。而且多數程式縮小後很快又會長回去
//   （例如每輪清空重用的緩衝區）。所以標準把「釋放」獨立成 shrink_to_fit()，
//   讓使用者明確表達意圖，而不是讓 resize 偷偷做昂貴的事。
//
// (B) resize 對元素的操作是「在尾端」，不是「重新建構全部」
//   resize(10) 不會動到前 5 個既有元素 —— 它們原地不動（除非發生重新配置
//   而被搬移）。新元素只在 [old_size, n) 這段被建構。所以 resize 不是
//   「重設容器」，它是「調整尾端」。想全部換值要用 assign() 或 fill()。
//
// (C) 為什麼 resize 用幾何成長而 reserve 用精確配置
//   語意不同：reserve 是使用者**明確宣告**「我要 n 這麼多」，實作照辦最尊重
//   意圖、也最省記憶體。resize 則常被放在迴圈裡逐步擴張（例如
//   `v.resize(v.size() + chunk)`），若精確配置就會退化成 O(n²) ——
//   所以它沿用 push_back 的幾何成長來保護使用者。
//
// 【注意事項 Pay Attention】
// 1. resize 縮小**不會**降低 capacity。本檔實測：resize(3) 之後 capacity
//    仍是 10。要釋放記憶體得另外呼叫 shrink_to_fit()。
// 2. resize 擴大若觸發重新配置，**所有 iterator/pointer/reference 失效**。
//    即使沒有重新配置，指向被銷毀元素（縮小時）的 iterator 也失效。
// 3. 本檔 capacity 的具體數值（5 → 10）是 libstdc++ 實測；標準只保證
//    capacity() >= size()。MSVC 用 1.5× 成長會得到不同數字。
// 4. **版本差異**：C++98 的單參數版本簽名是 `resize(n, T val = T())`，
//    新元素是「複製一個 value-initialized 的暫時物件」；C++11 起改成獨立的
//    `resize(n)` 並對新元素直接 value-initialize。對只可移動不可複製的型別
//    （如 std::unique_ptr）只有 C++11 之後的版本能用。
// 5. resize 要求 T 是 DefaultInsertable（單參數版）或 CopyInsertable
//    （雙參數版）。沒有預設建構子的型別不能用 resize(n)，只能用 resize(n, val)。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】resize
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. resize(n) 和 reserve(n) 差在哪？
//     答：reserve 只確保 capacity >= n，**不改變 size、不建構元素**，之後只能
//         用 push_back 填入；resize 真的把 size 變成 n，擴大時建構新元素
//         （之後可直接 v[i] 存取）、縮小時銷毀尾端元素。
//         一句話：reserve 準備空間，resize 準備元素。
//     追問：兩者擴大 capacity 的規則一樣嗎？
//         → 不一樣。libstdc++ 實測 reserve 剛好配置 n，resize 則走幾何成長
//           max(2*size, n) —— 所以 capacity 5 的 vector resize(8) 會得到 10。
//
// 🔥 Q2. resize 縮小之後，capacity 會跟著變小嗎？
//     答：不會，標準規定 capacity 不因縮小而減少。resize(3) 只是銷毀尾端
//         元素、把 size 設為 3，記憶體仍然握在手上。要真的釋放必須呼叫
//         shrink_to_fit()（而它還是非強制請求）。
//     追問：為什麼標準要這樣設計？
//         → 釋放需要「配置小 buffer + 搬移 + 釋放舊的」，縮小反而付出完整
//           重新配置成本；而且多數情境縮小後很快又長回去，保留 buffer 更划算。
//
// ⚠️ 陷阱 1. `std::vector<int> v; v.resize(5);` 之後 v[0] 的值是多少？
//     答：0。單參數 resize 對新元素做 value-initialization，內建型別會歸零。
//         這是標準保證，不是「剛好記憶體是乾淨的」。
//     為什麼會錯：拿 `new int[5]`（default-initialize，內建型別是未定值）
//         的直覺套到 resize 上，以為裡面是垃圾值。兩者初始化語意不同。
//
// ⚠️ 陷阱 2. 想清空 vector 卻寫 `v.resize(0)` 和 `v.clear()`，有差嗎？
//     答：對 size 與 capacity 的效果完全相同（都是 size 歸 0、capacity 不變、
//         元素都被解構）。差別只在可讀性 —— clear() 明確表達意圖。
//         但兩者都**不會**釋放記憶體，這才是多數人真正搞錯的地方。
//     為什麼會錯：以為「清空」等於「還記憶體」。想釋放要 clear() +
//         shrink_to_fit()，或用 swap trick（見 8.cpp）。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <string>

// 印出目前狀態的小工具
static void dump(const std::string& label, const std::vector<int>& v) {
    std::cout << label << " size=" << v.size()
              << ", capacity=" << v.capacity() << ", 內容: [";
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i != 0) std::cout << ' ';
        std::cout << v[i];
    }
    std::cout << "]" << std::endl;
}

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5};

    std::cout << "=== 初始狀態 ===" << std::endl;
    dump("初始:", v);

    std::cout << "\n=== 擴大：新元素 value-initialize（int 歸 0）===" << std::endl;
    v.resize(8);
    dump("resize(8):", v);
    // 注意 capacity 是 10 不是 8：resize 走幾何成長 max(2*size, n) = max(10, 8)

    std::cout << "\n=== 擴大並指定填充值 ===" << std::endl;
    v.resize(10, 99);
    dump("resize(10, 99):", v);

    std::cout << "\n=== 縮小：多餘元素被銷毀，capacity 不變 ===" << std::endl;
    v.resize(3);
    dump("resize(3):", v);
    std::cout << "縮小後 capacity 仍是 " << v.capacity()
              << "（記憶體沒有還給系統）" << std::endl;

    std::cout << "\n=== resize(0) 等價於 clear()：capacity 依然不變 ===" << std::endl;
    v.resize(0);
    dump("resize(0):", v);

    std::cout << "\n=== 對照：reserve 不建構元素，size 不變 ===" << std::endl;
    std::vector<int> r;
    r.reserve(8);
    std::cout << "reserve(8): size=" << r.size()
              << ", capacity=" << r.capacity()
              << "（size 仍是 0，r[0] 是 UB）" << std::endl;
    std::vector<int> s;
    s.resize(8);
    std::cout << "resize(8):  size=" << s.size()
              << ", capacity=" << s.capacity()
              << "（size 是 8，s[0] 合法且為 0）" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 11 課：vector 的容量管理：size、capacity、reserve4.cpp" -o demo4

// 註：以下 capacity 具體數值為 libstdc++ / GCC 15.2 實測，非標準保證。
//     resize 擴大時 libstdc++ 用 new_cap = max(2 * size, n)，所以
//     capacity 5 的 vector 呼叫 resize(8) 得到 10 而不是 8；
//     MSVC（1.5× 成長）會得到不同數字。標準只保證 capacity() >= size()。

// === 預期輸出 ===
// === 初始狀態 ===
// 初始: size=5, capacity=5, 內容: [1 2 3 4 5]
//
// === 擴大：新元素 value-initialize（int 歸 0）===
// resize(8): size=8, capacity=10, 內容: [1 2 3 4 5 0 0 0]
//
// === 擴大並指定填充值 ===
// resize(10, 99): size=10, capacity=10, 內容: [1 2 3 4 5 0 0 0 99 99]
//
// === 縮小：多餘元素被銷毀，capacity 不變 ===
// resize(3): size=3, capacity=10, 內容: [1 2 3]
// 縮小後 capacity 仍是 10（記憶體沒有還給系統）
//
// === resize(0) 等價於 clear()：capacity 依然不變 ===
// resize(0): size=0, capacity=10, 內容: []
//
// === 對照：reserve 不建構元素，size 不變 ===
// reserve(8): size=0, capacity=8（size 仍是 0，r[0] 是 UB）
// resize(8):  size=8, capacity=8（size 是 8，s[0] 合法且為 0）
