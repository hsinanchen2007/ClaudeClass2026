// =============================================================================
//  第 11 課：vector 的容量管理：size、capacity、reserve1.cpp
//  —  size() / capacity() / empty()：三個最容易被混為一談的觀測函式
// =============================================================================
//
// 【主題資訊 Information】
//   size_type size()     const noexcept;   // 目前「有幾個已建構的元素」
//   size_type capacity() const noexcept;   // 目前「這塊記憶體最多放得下幾個」
//   bool      empty()    const noexcept;   // 等價於 size() == 0
//
//   標頭檔：<vector>
//   標準版本：三者 C++98 就有；C++11 起補上 noexcept；
//             C++20 起三者都是 constexpr（可在編譯期求值）。
//   複雜度：全部 O(1)（都只是指標相減，見【概念補充】）。
//   不變量：0 <= size() <= capacity() <= max_size()，永遠成立。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼需要「兩個大小」？】
// vector 的合約是「連續記憶體 + 尾端插入攤提 O(1)」。這兩件事互相衝突：
// 連續記憶體代表元素必須排在同一塊 buffer 上，而 buffer 一旦滿了就得換一塊
// 更大的、把舊資料搬過去。如果每次 push_back 都只配「剛好夠」的空間，那每次
// 插入都要重新配置 + 搬移 N 個元素 → n 次插入會退化成 O(n²)。
//
// 解法是「多要一點」：配置的空間（capacity）刻意大於實際用量（size），多出
// 來的尾巴是還沒建構任何物件的原始記憶體（raw storage）。push_back 時只要
// size < capacity，就只是在既有空間上建構一個元素、size++ 而已，完全不需要
// 配置或搬移。這就是為什麼 vector 必須同時暴露兩個「大小」。
//
// 【2. size() 是「元素數」，不是「位元組數」】
// 初學者最常見的誤解是把 size() 當成記憶體大小。size() 回傳的是元素個數，
// 要換算成位元組必須自己乘 sizeof(T)：
//     v.size()     * sizeof(T)      // 已使用（已建構）的位元組
//     v.capacity() * sizeof(T)      // 已配置（含未建構尾巴）的位元組
// 這一點和 std::string 一致 —— string 的 size() 是 byte 數而非「人眼看到的
// 字元數」，只因為 char 剛好 1 byte，兩者數值相等才容易產生錯覺。
//
// 【3. empty() 為什麼不寫成 size() == 0？】
// 對 vector 而言兩者確實等價、也一樣快。但 empty() 是所有容器的共同介面，
// 而 size() 不是 —— C++11 之前 std::list::size() 允許是 O(n)（實作可以不
// 快取長度），所以「判斷容器是否為空」唯一可攜且恆為 O(1) 的寫法一直是
// empty()。養成用 empty() 的習慣，日後換容器才不會突然掉進 O(n)。
// 另外 empty() 語意更直白，`if (v.empty())` 比 `if (v.size() == 0)` 更接近人話。
//
// 【4. capacity() 的實際數值是「實作定義」】
// 標準只規定 capacity() >= size()，以及 push_back 必須是「攤提 O(1)」。
// 標準從未規定成長倍率是多少，也沒規定初始 capacity 必須是 0。
// 本檔輸出中的 capacity 具體數字是 libstdc++（GCC 15.2）實測值，換成 MSVC
// （成長倍率 1.5×）或 libc++ 會得到不同數字 —— 這不是 bug。
// 因此：**永遠不要把 capacity() 的具體值寫進程式邏輯或單元測試的斷言。**
//
// 【概念補充 Concept Deep Dive】
// libstdc++ 的 std::vector 內部就只有三根指標（本機實測 sizeof 為 24 bytes）：
//
//     _M_start           _M_finish              _M_end_of_storage
//        │                   │                          │
//        ▼                   ▼                          ▼
//      ┌───┬───┬───┬───┬───┬───────────────────────────┐
//      │ 1 │ 2 │ 3 │ 4 │ 5 │    未建構的原始記憶體      │
//      └───┴───┴───┴───┴───────────────────────────────┘
//        └──── size() = 5 ───┘
//        └────────── capacity()（假設為 8）────────────┘
//
//   size()     ≡ _M_finish          - _M_start
//   capacity() ≡ _M_end_of_storage  - _M_start
//   empty()    ≡ _M_finish == _M_start
//
// 所以三者都只是指標相減（編譯器再除以 sizeof(T)，對 2 的冪次通常優化成位移），
// 這就是「O(1)」的來源 —— 它們不走訪、也不查表，而是純算術。
// 也因為 capacity 的尾巴是**未建構**的原始記憶體，對它做 operator[] 存取是
// UB，而不是「讀到 0」（詳見 5.cpp）。
//
// 【注意事項 Pay Attention】
// 1. 空 vector 的 capacity() 不保證是 0。libstdc++ 預設建構是 0，
//    但標準未規定，其他實作可能預先配置。
// 2. 本檔輸出的 capacity=5 是 libstdc++ 實測：由 initializer_list 建構時
//    剛好配置「正好夠用」的大小，不是四捨五入到 2 的冪次。
// 3. empty() 在 C++20 起標了 [[nodiscard]] —— 把 `v.empty();` 單獨寫一行
//    （本意想清空卻打錯字）會被編譯器警告。想清空要用 v.clear()。
// 4. size() 回傳無號的 size_type。`v.size() - 1` 在 v 為空時不是 -1，
//    而是巨大的正數 → 拿去當索引就是越界 UB。倒著跑迴圈要特別小心。
// 5. 三者都是觀測函式，不修改容器，也不會使任何 iterator 失效。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】size / capacity / empty
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. size() 和 capacity() 差在哪？為什麼 vector 需要同時提供兩個？
//     答：size() 是「已建構的元素個數」，capacity() 是「這塊 buffer 放得下
//         幾個元素」，永遠 size() <= capacity()。多配的尾巴讓 push_back 在
//         沒滿之前不需要重新配置與搬移，push_back 才能是攤提 O(1)；
//         若每次都只配剛好夠，n 次 push_back 會退化成 O(n²)。
//     追問：capacity 和 size 之間那段記憶體裡有什麼？
//         → 未建構的原始記憶體（raw storage），沒有任何 T 物件存在，
//           碰它就是 UB。
//
// 🔥 Q2. 這三個函式的複雜度各是多少？為什麼？
//     答：全部 O(1)。vector 內部只有三根指標（_M_start / _M_finish /
//         _M_end_of_storage），size 與 capacity 都只是指標相減，
//         empty 是兩根指標比較，都不需要走訪任何元素。
//     追問：那 std::list::size() 呢？
//         → C++11 起標準要求所有容器的 size() 都是 O(1)（list 必須快取長度）；
//           C++11 之前 list::size() 允許 O(n)，這正是「判空一律用 empty()」
//           這個習慣的歷史由來。
//
// ⚠️ 陷阱 1. 「空 vector 的 capacity() 一定是 0」——對嗎？
//     答：不對。標準只保證 size() <= capacity()，沒有規定預設建構後
//         capacity 必須是 0。libstdc++ 實測是 0，但這是實作細節，
//         不可寫進斷言。
//     為什麼會錯：把某一個實作的觀測結果當成標準保證。凡是 capacity 的
//         「具體數值」，標準幾乎都沒有規定。
//
// ⚠️ 陷阱 2. `for (size_t i = 0; i <= v.size() - 1; ++i)` 在 v 為空時會怎樣？
//     答：v.size() 是無號型別，0 - 1 會 wrap around 成 SIZE_MAX，條件恆真
//         → 迴圈用巨大索引存取 → 越界，是 UB，不是「跑 0 次」。
//     為什麼會錯：腦中把 size() 當成有號 int，以為 0-1 是 -1、迴圈自然不執行。
//         正確寫法是 `i < v.size()`，或直接用 range-for。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5};

    std::cout << "=== 基本觀測 ===" << std::endl;
    std::cout << "size: " << v.size() << std::endl;          // 5（實際元素數量）
    std::cout << "capacity: " << v.capacity() << std::endl;  // >= 5（配置的空間）
    std::cout << "empty: " << v.empty() << std::endl;        // 0（false）

    std::cout << "\n=== 空 vector ===" << std::endl;
    std::vector<int> e;
    std::cout << "size: " << e.size() << std::endl;
    std::cout << "capacity: " << e.capacity() << std::endl;
    std::cout << "empty: " << e.empty() << std::endl;        // 1（true）

    std::cout << "\n=== 換算成位元組 ===" << std::endl;
    // size() 是「元素數」不是「位元組數」，要自己乘 sizeof(T)
    std::cout << "已使用: " << v.size() * sizeof(int) << " bytes" << std::endl;
    std::cout << "已配置: " << v.capacity() * sizeof(int) << " bytes" << std::endl;

    std::cout << "\n=== 不變量 size <= capacity ===" << std::endl;
    std::cout << "size <= capacity: " << (v.size() <= v.capacity()) << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 11 課：vector 的容量管理：size、capacity、reserve1.cpp" -o demo1

// 註：以下 capacity 具體數值為 libstdc++ / GCC 15.2 實測，非標準保證；
//     換 MSVC（1.5× 成長）或 libc++ 可能得到不同數字。

// === 預期輸出 ===
// === 基本觀測 ===
// size: 5
// capacity: 5
// empty: 0
//
// === 空 vector ===
// size: 0
// capacity: 0
// empty: 1
//
// === 換算成位元組 ===
// 已使用: 20 bytes
// 已配置: 20 bytes
//
// === 不變量 size <= capacity ===
// size <= capacity: 1
