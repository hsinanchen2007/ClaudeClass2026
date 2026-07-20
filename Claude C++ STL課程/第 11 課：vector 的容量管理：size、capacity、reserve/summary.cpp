// =============================================================================
//  summary.cpp  —  第 11 課「vector 的容量管理」總複習
//                  size / capacity / reserve / resize / shrink_to_fit
// =============================================================================
//
// 【主題資訊 Information】
//   全部介面皆定義於 <vector>：
//
//     size_type size()      const noexcept;         // 已建構的元素數
//     size_type capacity()  const noexcept;         // buffer 容得下幾個元素
//     bool      empty()     const noexcept;         // size() == 0
//     size_type max_size()  const noexcept;         // 理論上界（實作定義）
//     void      reserve(size_type n);               // 只增容，不改 size
//     void      resize (size_type n);               // 改 size，新元素 value-init
//     void      resize (size_type n, const T& val); // 新元素複製自 val
//     void      shrink_to_fit();                    // **non-binding request**
//     void      clear() noexcept;                   // 清元素，capacity 不變
//     T*        data()   noexcept;                  // 取得連續記憶體位址
//
//   標準版本：
//     * size / capacity / empty / max_size / reserve / resize / clear：C++98 起
//     * **shrink_to_fit 與 data()：C++11 起**（C++98 只能用 swap trick 縮容）
//     * C++11 起補上 noexcept；C++20 起上述全部為 constexpr
//     * 單參數 resize(n) 對新元素 value-initialize 是 C++11 起的語意
//       （C++98 簽名為 resize(n, T val = T())，要求 T 可複製）
//
//   複雜度：
//     size / capacity / empty / max_size → O(1)（純指標算術，見【概念補充 A】）
//     reserve(n)      → 只在 n > capacity() 時付出 O(size()) 搬移
//     resize(n)       → O(|n - size()|)，加上可能的 O(size()) 搬移
//     shrink_to_fit() → 最壞 O(size())
//     clear()         → O(size())（逐一解構；trivially destructible 型別近 O(1)）
//     push_back       → **amortized O(1)**（標準保證），單次最壞 O(size())
//
//   例外：reserve/resize 的 n > max_size() → std::length_error；
//         配置失敗 → std::bad_alloc。
//
//   核心不變量：0 <= size() <= capacity() <= max_size()
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 vector 要有「兩個大小」】
// vector 的合約是「連續記憶體 + 尾端插入 amortized O(1)」，這兩者天生衝突：
// 連續代表元素得排在同一塊 buffer 上，buffer 滿了就得換一塊更大的、把資料
// 搬過去。若每次只配「剛好夠」，n 次 push_back 會退化成 O(n²)。
// 解法是刻意多配：capacity > size，多出來的尾巴是**未建構的原始記憶體**。
// 只要 size < capacity，push_back 就只是在既有空間上建構元素，零搬移。
// 這就是 size 與 capacity 必須分開的根本原因，也是本課所有規則的源頭。
//
// 【2. 成長倍率是實作定義 —— 本課最常被講錯的地方】
// 標準只要求 push_back 為 **amortized O(1)**，**從未規定倍率是 2× 或 1.5×**。
//   * libstdc++（本機 GCC 15.2）實測：1 → 2 → 4 → 8 → 16 → 32 → 64（2×）
//   * MSVC：1.5×
// 兩者都合規。所以**任何 capacity 的具體數值都是「實測值，非標準保證」**，
// 不可寫進程式邏輯或單元測試斷言。
//
// 為什麼幾何成長就能攤提？倍率 k 時，從 1 長到 n 的總搬移量是等比級數
// 1 + k + k² + … + n ≈ n·k/(k-1) = Θ(n)，分攤到 n 次插入即 O(1)。
// 若改成「每次固定加 c 個」，總搬移量是 Θ(n²)，攤提就不成立了。
//
// 【3. reserve vs resize：準備「空間」還是準備「元素」】
//   reserve(n)：只確保 capacity >= n。**不改變 size、不建構任何元素。**
//               標準規定 n <= capacity() 時**什麼都不做**（只增不減）。
//               之後只能用 push_back / emplace_back 填入；用 v[i] 是 **UB**。
//   resize(n) ：真的把 size 變成 n。擴大時建構新元素（value-initialize，
//               int 歸 0），縮小時銷毀尾端元素。之後可直接 v[i] 存取。
//
// 兩者擴大 capacity 的規則也不同（實作定義，但差異很實際）：
//   libstdc++ 實測 reserve(n) **剛好配置 n**；resize(n) 走幾何成長
//   max(2 * size, n)。所以 capacity 5 的 vector 呼叫 resize(8) 得到的
//   capacity 是 **10** 而不是 8。
//
// 【4. 縮容三種手段，只有一種有保證】
//   clear()                    → size 歸 0，**capacity 完全不變**（保留重用）
//   shrink_to_fit()            → **non-binding request**，標準允許實作忽略
//   std::vector<T>().swap(v)   → **保證**重新配置：清空且釋放（C++98 唯一手段）
//   std::vector<T>(v).swap(v)  → **保證**重新配置：保留內容、capacity 縮到剛好
//
// 特別強調：**shrink_to_fit 不保證縮容**。標準原文用的是 "non-binding
// request"。libstdc++ 實測會照做，但那是實作行為。需要保證時用 swap trick。
//
// 【5. 什麼時候該 reserve、什麼時候不該】
//   該用：已知精確筆數（檔頭宣告的 record count、長度前綴訊息、OJ 的 N）；
//         或已知合理上界（篩選結果最多和輸入一樣多）。
//   不該用：完全不知道總量時 —— 亂猜一個小數字只會多付一次配置卻照樣擴容。
//   **絕對不要**：在迴圈裡每輪 reserve(size()+1)。因為 libstdc++ 的 reserve
//         剛好配置 n，這等於把幾何成長改成「每次加 1」→ 退化成 O(n²)。
//         本機實測推 10 個元素：逐次 reserve 觸發 10 次重新配置，
//         什麼都不做的 push_back 只要 5 次 —— reserve 反而更慢。
//
// 【概念補充 Concept Deep Dive】
// (A) vector 的記憶體佈局（libstdc++，本機實測 sizeof 為 24 bytes）
//     內部就只有三根指標：
//
//       _M_start          _M_finish            _M_end_of_storage
//          │                  │                        │
//          ▼                  ▼                        ▼
//        ┌───┬───┬───┬───┬───┬─────────────────────────┐
//        │ 1 │ 2 │ 3 │ 4 │ 5 │   未建構的原始記憶體      │
//        └───┴───┴───┴───┴───┴─────────────────────────┘
//          └──── size() = 5 ──┘
//          └────────── capacity()（假設 8）────────────┘
//
//     size()     ≡ _M_finish         - _M_start
//     capacity() ≡ _M_end_of_storage - _M_start
//     empty()    ≡ _M_finish == _M_start
//     三者都只是指標相減，這就是 O(1) 的來源。也因為尾巴是**未建構**的
//     記憶體，reserve 之後 v[i] 是 UB，而不是「讀到 0」。
//
// (B) 重新配置時是 move 還是 copy —— 決定實際代價的關鍵
//     push_back 提供**強例外保證**。搬移到新 buffer 時若 move constructor
//     可能拋例外，搬到一半失敗就無法還原，所以 libstdc++ 用
//     std::move_if_noexcept：只有 T 的 move constructor 是 noexcept
//     （或 T 不可複製）時才 move，否則退回 **copy**。
//     實務結論：**自訂型別的 move constructor 一定要標 noexcept**，
//     否則 vector 擴容會靜默退化成深複製，差好幾個數量級。
//
// (C) swap trick 為什麼有效
//     std::vector<T>(v).swap(v) 拆成三步：① 用 v 複製建構一個暫時 vector
//     （複製建構只需容納 size() 個元素，實測 capacity == size）；
//     ② swap 交換內部三根指標，O(1)，不搬移元素；③ 敘述結束時暫時物件
//     解構，帶走原本那塊過大的 buffer。
//     兩個變體差很多：`vector<T>()` 是**清空**、`vector<T>(v)` 是**縮容**。
//
// (D) max_size 是型別上界，不是可用記憶體
//     libstdc++ 實測公式為 PTRDIFF_MAX / sizeof(T)，所以**隨元素型別而變**：
//     vector<char> 是 9223372036854775807、vector<int> 是 2305843009213693951。
//     用 PTRDIFF_MAX 而非 SIZE_MAX 是因為兩個 iterator 相減的結果型別是
//     有號的 difference_type，超過就會有號溢位（UB）。
//     實務上一定先撞 std::bad_alloc；max_size 的用途是區分「數量本身不合法」
//     （length_error）與「記憶體不夠」（bad_alloc）。
//
// 【注意事項 Pay Attention】
// 1. **reserve 不改變 size** → reserve 後用 v[i] 是 **UB**，只能 push_back。
// 2. **shrink_to_fit 是 non-binding request**，標準不保證 capacity 會下降。
//    絕不可寫 assert(v.capacity() == v.size())。需要保證時用 swap trick。
// 3. **clear() 不釋放記憶體**，capacity 維持原值；且即使 data() 位址不變，
//    所有 iterator/pointer/reference 仍然失效（元素已被解構）。
// 4. **任何重新配置都會使全部 iterator/pointer/reference 失效** ——
//    包含 reserve 擴容、resize 擴容、push_back 觸發擴容、shrink_to_fit 成功縮容。
// 5. **成長倍率與所有 capacity 具體數值皆為實作定義。** 本檔輸出的數字是
//    libstdc++ / GCC 15.2 實測；MSVC（1.5×）會不同。
// 6. resize 縮小**不會**降低 capacity；resize(0) 與 clear() 效果相同。
// 7. reserve/resize 的 n > max_size() 丟 **std::length_error**（不是 bad_alloc）。
//    小心無號下溢：v.reserve(v.size() - 1) 在空 vector 上是 SIZE_MAX。
// 8. 傳給 C API 的長度要用 **size()** 不是 capacity()；寫進 capacity 區域是 UB。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector 容量管理（本課總整理）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. size() 和 capacity() 差在哪？為什麼 vector 需要同時提供兩個？
//     答：size() 是「已建構的元素個數」，capacity() 是「這塊 buffer 放得下
//         幾個」，永遠 size() <= capacity()。多配的尾巴讓 push_back 在沒滿
//         之前免於重新配置與搬移，才能做到 amortized O(1)；若每次只配剛好夠，
//         n 次插入會退化成 O(n²)。
//     追問：兩者中間那段記憶體裡有什麼？
//         → 未建構的原始記憶體（raw storage），沒有任何 T 物件存在，碰它是 UB。
//
// 🔥 Q2. push_back 為什麼是 amortized O(1)？標準有規定成長倍率嗎？
//     答：因為採幾何成長。倍率 k 時總搬移量是等比級數 ≈ n·k/(k-1) = Θ(n)，
//         分攤到 n 次插入就是 O(1)。**標準只要求 amortized O(1)，從未規定
//         倍率**：libstdc++ 實測 2×、MSVC 1.5×，兩者都合規。
//     追問：改成「每次固定加 c 個」會怎樣？
//         → 總搬移量變 Θ(n²)，攤提不再成立 —— 幾何成長是不可或缺的。
//
// 🔥 Q3. reserve(n) 和 resize(n) 差在哪？reserve 之後可以 v[0] = x 嗎？
//     答：reserve 只確保 capacity >= n，**不改變 size、不建構元素**；
//         resize 真的把 size 變成 n（擴大時建構、縮小時銷毀）。
//         reserve(100) 之後 size() 仍是 0，v[0] 是越界存取 → **UB**；
//         要填資料只能 push_back，想索引存取就得改用 resize。
//     追問：n <= capacity() 時 reserve 做什麼？
//         → 標準規定**什麼都不做**。reserve 只增不減，不會縮容。
//
// 🔥 Q4. shrink_to_fit() 保證把 capacity 降到 size 嗎？
//     答：**不保證**。標準明文寫它是 **non-binding request**，實作可以完全
//         忽略。libstdc++ 實測會照做，但那是實作行為，不可寫成斷言。
//     追問：為什麼標準要留這個彈性？要怎麼「保證」縮容？
//         → 縮容需要配置新 buffer；記憶體不足時硬性要求就得丟 bad_alloc，
//           「想省記憶體卻因記憶體不足而失敗」不合理；記憶體池之類的
//           allocator 也沒有縮小的概念。需要保證時用 swap trick：
//           std::vector<T>(v).swap(v)，它一定會重新配置。
//
// 🔥 Q5. clear() 之後 capacity 會變 0 嗎？要怎麼真的釋放記憶體？
//     答：不會。clear() 只解構元素、把 size 設為 0，capacity 完全不變 ——
//         這是刻意設計，清空多半是為了重複使用，保留 buffer 讓下一輪免配置。
//         想釋放要 clear() + shrink_to_fit()（非強制請求），
//         或 std::vector<T>().swap(v)（保證釋放）。
//     追問：clear() 之後舊 iterator 還能用嗎？
//         → 不能。即使 data() 位址不變，元素已被解構，那些指標指向的物件
//           生命期已結束，讀取是 UB。
//
// ⚠️ 陷阱 1. 「reserve 一定比較快，所以每次 push_back 前都 reserve(size()+1)」
//     答：這是效能災難。libstdc++ 的 reserve 剛好配置 n，逐次 reserve 等於
//         把幾何成長改成「每次加 1」→ 每次插入都重新配置 → O(n²)。
//         本機實測推 10 個元素：逐次 reserve 觸發 10 次重新配置，
//         什麼都不做的 push_back 只要 5 次 —— reserve 反而更慢。
//     為什麼會錯：把 reserve 當成「效能開關」，以為呼叫越多次越快。
//         它的價值來自**一次就要到最終大小**；不知道總量時什麼都不做才對。
//
// ⚠️ 陷阱 2. reserve / resize / shrink_to_fit 之後，先前的 iterator 還能用嗎？
//     答：不能。只要真的發生重新配置（含 shrink_to_fit 成功縮容），所有
//         iterator / pointer / reference **全部失效**，包括 v.begin()、
//         &v[0]、v.data()。
//     為什麼會錯：以為「只是多要空間」或「只是變小」不會動到既有元素。
//         實際上那是換一塊新記憶體再整批搬過去，舊位址已被釋放。
//
// ⚠️ 陷阱 3. `std::vector<int> v; v.resize(5);` 之後 v[0] 是垃圾值嗎？
//     答：不是，是 0。單參數 resize 對新元素做 value-initialization，
//         內建型別保證歸零。這是標準行為，不是「剛好記憶體是乾淨的」。
//     為什麼會錯：拿 `new int[5]`（default-initialize，內建型別是未定值）
//         的直覺套到 resize 上。兩者的初始化語意不同。
//
// ⚠️ 陷阱 4. 迴圈裡每輪都寫 `v.clear(); v.shrink_to_fit();` —— 好嗎？
//     答：這是效能反模式。若下一輪還要填回差不多的資料量，shrink_to_fit
//         把 buffer 還掉，下一輪又得從頭幾何成長回去 —— 每輪白付一次縮容
//         O(n) 加一整串重新配置。這種情境**只用 clear()** 才對。
//     為什麼會錯：以為「盡快釋放記憶體」永遠是美德，忽略 capacity 被保留
//         正是為了讓重複使用免於重新配置。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <cstdint>
#include <string>
#include <stdexcept>
#include <utility>   // std::move

// ===== 重點一：size / capacity / empty / max_size =====
// size()     → 目前實際存放的元素數量
// capacity() → 目前配置的空間可容納的元素數量（>= size）
// empty()    → 等價於 size() == 0
// max_size() → 理論最大容量（實作定義，libstdc++ 為 PTRDIFF_MAX / sizeof(T)）
//
// 不變量：size() <= capacity() 永遠成立

void demo_size_capacity_empty() {
    std::cout << "\n===== 重點一：size / capacity / empty =====\n";

    std::vector<int> v = {1, 2, 3, 4, 5};

    std::cout << "size:     " << v.size()     << std::endl;  // 5（實際元素數量）
    std::cout << "capacity: " << v.capacity() << std::endl;  // >= 5（配置的空間）
    std::cout << "empty:    " << v.empty()    << std::endl;  // 0（false）

    std::vector<int> empty_v;
    std::cout << "空 vector size: "  << empty_v.size()  << std::endl;  // 0
    std::cout << "空 vector empty: " << empty_v.empty() << std::endl;  // 1

    // max_size 隨元素型別而變（libstdc++ = PTRDIFF_MAX / sizeof(T)）
    std::cout << "vector<int>  max_size: " << v.max_size() << std::endl;
    std::cout << "vector<char> max_size: "
              << std::vector<char>().max_size() << std::endl;
}

// ===== 重點二：reserve(n) — 預先配置空間 =====
// reserve 只改變 capacity，不改變 size，也不建立任何元素。
//   - 若 n >  capacity()：重新配置，capacity 至少變為 n
//   - 若 n <= capacity()：**標準規定什麼都不做**（只增不減）
//
// 重要警告：reserve 後 size 仍為 0，不能用 operator[] 直接存取！
//   v.reserve(100);
//   v[0] = 5;  // 未定義行為！size 是 0

void demo_reserve() {
    std::cout << "\n===== 重點二：reserve 預先配置空間 =====\n";

    std::vector<int> v;
    std::cout << "初始 - size: " << v.size()
              << ", capacity: " << v.capacity() << std::endl;

    v.reserve(100);  // 預先配置至少 100 個元素的空間

    std::cout << "reserve(100) 後 - size: " << v.size()
              << ", capacity: " << v.capacity() << std::endl;
    // size 仍是 0，capacity >= 100

    // 記下位址以驗證「100 次 push_back 都沒搬移」。
    // 只印「是否搬移」而不印位址本身 —— 位址每次執行都不同。
    const int* base = v.data();

    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
    }

    std::cout << "100 次 push_back 後 - size: " << v.size()
              << ", capacity: " << v.capacity() << std::endl;
    std::cout << "資料是否搬移過: " << (v.data() != base ? "是" : "否")
              << std::endl;

    // reserve 只增不減
    v.reserve(10);
    std::cout << "reserve(10) 後 capacity: " << v.capacity()
              << "（不縮減）" << std::endl;

    // 超出 capacity 才觸發重新配置（libstdc++ 實測 2× 成長：100 → 200）
    v.push_back(100);
    std::cout << "推第 101 個後 capacity: " << v.capacity() << std::endl;
}

// ===== 重點三：resize(n) — 改變實際元素數量 =====
// resize(n)      → size 改為 n；擴大時新元素 value-initialize（int 為 0）
// resize(n, val) → size 改為 n；擴大時新元素複製自 val
// 縮小時：多餘元素被銷毀（呼叫解構子），但 **capacity 不變**

void demo_resize() {
    std::cout << "\n===== 重點三：resize 改變元素數量 =====\n";

    std::vector<int> v = {1, 2, 3, 4, 5};
    std::cout << "初始 size: " << v.size()
              << ", capacity: " << v.capacity() << std::endl;

    // 擴大：新元素 value-initialize（int 歸 0）
    v.resize(8);
    std::cout << "resize(8): ";
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i != 0) std::cout << ' ';
        std::cout << v[i];
    }
    std::cout << "  (capacity=" << v.capacity()
              << "，注意不是 8：resize 走幾何成長 max(2*size, n))" << std::endl;

    // 擴大並指定填充值
    v.resize(10, 99);
    std::cout << "resize(10, 99): ";
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i != 0) std::cout << ' ';
        std::cout << v[i];
    }
    std::cout << std::endl;

    // 縮小：多餘元素被銷毀，capacity 不變
    v.resize(3);
    std::cout << "resize(3): ";
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i != 0) std::cout << ' ';
        std::cout << v[i];
    }
    std::cout << std::endl;
    std::cout << "縮小後 capacity: " << v.capacity()
              << "（記憶體沒有還給系統）" << std::endl;
}

// ===== 重點四：reserve vs resize 關鍵差異 =====
// | 函數        | 改變 size | 改變 capacity          | 建立元素     |
// |------------|-----------|------------------------|--------------|
// | reserve(n) | 否        | 是（若 n > capacity）  | 否           |
// | resize(n)  | 是        | 可能（若 n > capacity）| 是（擴大時） |
//
// reserve：知道大約要多少，之後用 push_back 填入
// resize ：需要固定大小、可直接用 operator[] 存取

void demo_reserve_vs_resize() {
    std::cout << "\n===== 重點四：reserve vs resize 對照 =====\n";

    std::vector<int> v1, v2;
    v1.reserve(5);
    v2.resize(5);

    std::cout << "reserve(5): size=" << v1.size()
              << ", capacity=" << v1.capacity() << std::endl;  // size=0
    std::cout << "resize(5):  size=" << v2.size()
              << ", capacity=" << v2.capacity() << std::endl;  // size=5

    // reserve 後不能用 operator[]（UB！）—— 改用 at() 安全地證明 size 是 0
    try {
        v1.at(0) = 10;
        std::cout << "at(0) 成功（不應該發生）" << std::endl;
    } catch (const std::out_of_range&) {
        std::cout << "v1.at(0) 丟出 std::out_of_range → size 確實是 0"
                  << std::endl;
        std::cout << "（寫成 v1[0] = 10 則是 UB，不保證會被偵測到）"
                  << std::endl;
    }

    // resize 後可以直接用 operator[]
    v2[0] = 10;  // 合法，因為 size 是 5
    std::cout << "resize 後 v2[0] = " << v2[0] << std::endl;

    // 正確使用 reserve 的方式：push_back
    v1.push_back(99);
    std::cout << "reserve 後 push_back，v1[0] = " << v1[0] << std::endl;
}

// ===== 重點五：shrink_to_fit — 請求釋放多餘空間 =====
// C++11 引入。標準原文是 **non-binding request** —— 實作可以完全忽略。
// 絕不可寫成「一定會把 capacity 降到 size」。

void demo_shrink_to_fit() {
    std::cout << "\n===== 重點五：shrink_to_fit 釋放多餘空間 =====\n";

    std::vector<int> v;
    v.reserve(1000);
    for (int i = 0; i < 10; ++i) {
        v.push_back(i);
    }

    std::cout << "shrink 前: size=" << v.size()
              << ", capacity=" << v.capacity() << std::endl;
    std::cout << "浪費了 " << (v.capacity() - v.size()) * sizeof(int)
              << " bytes" << std::endl;

    v.shrink_to_fit();

    std::cout << "shrink 後: size=" << v.size()
              << ", capacity=" << v.capacity() << std::endl;
    std::cout << "註: capacity 真的下降是 libstdc++ 實測結果；"
              << "標準規定 shrink_to_fit 是 non-binding request，允許實作忽略。"
              << std::endl;
}

// ===== 重點六：清空 vector 的三種方式 =====
// 方法一：clear()               → size=0，capacity **不變**（保留記憶體重用）
// 方法二：swap 技巧             → size=0，capacity=0（**保證**釋放，C++98 慣用法）
// 方法三：clear + shrink_to_fit → size=0，capacity 通常 0（但是非強制請求）
//
// 選擇建議：
//   之後還要重新填入大量資料 → clear()（避免重新配置）
//   確定不再需要這些記憶體   → clear + shrink_to_fit；需要保證時用 swap trick

void demo_clearing_methods() {
    std::cout << "\n===== 重點六：清空 vector 的方式比較 =====\n";

    // --- 方法一：clear（保留 capacity）---
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        v.reserve(100);
        std::cout << "clear 前:  size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;
        v.clear();
        std::cout << "clear 後:  size=" << v.size()
                  << ", capacity=" << v.capacity()
                  << "（capacity 不變）" << std::endl;
    }

    // --- 方法二：swap 技巧（保證釋放記憶體）---
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        v.reserve(100);
        std::cout << "swap 前:   size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;
        std::vector<int>().swap(v);  // 用空的臨時 vector 和 v 交換
        std::cout << "swap 後:   size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;
    }

    // --- 方法三：clear + shrink_to_fit（C++11 之後）---
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        v.reserve(100);
        std::cout << "c+s 前:    size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;
        v.clear();
        v.shrink_to_fit();
        std::cout << "c+s 後:    size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;
    }

    // --- 變體：只縮容、保留內容 ---
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        v.reserve(100);
        std::vector<int>(v).swap(v);   // 複製建構 → capacity 剛好等於 size
        std::cout << "縮容變體:  size=" << v.size()
                  << ", capacity=" << v.capacity()
                  << "（內容保留）" << std::endl;
    }
}

// ===== 重點七：確定性成本比較 — 有無 reserve 的差異 =====
// 這裡刻意**不印執行時間**：牆鐘時間受 CPU 排程、頻率調節、快取狀態影響，
// **每次執行都不同**，無法作為可重現的教材輸出。改用由成長規則決定的
// 確定值：重新配置次數與被搬移的元素總數。
// （本機 -O2 的參考測量見檔尾【實測時間】。）

struct GrowthCost {
    int       reallocations;
    long long elementsMoved;
};

static GrowthCost measurePushBack(int n, bool useReserve) {
    std::vector<int> v;
    if (useReserve) v.reserve(static_cast<std::size_t>(n));

    GrowthCost cost{0, 0};
    for (int i = 0; i < n; ++i) {
        // 容量已滿 → 這次 push_back 必定重新配置，搬移量就是目前的 size()
        if (v.size() == v.capacity()) {
            ++cost.reallocations;
            cost.elementsMoved += static_cast<long long>(v.size());
        }
        v.push_back(i);
    }
    return cost;
}

void demo_performance() {
    std::cout << "\n===== 重點七：確定性成本比較（reserve vs 無 reserve）=====\n";

    const int N = 1000000;
    GrowthCost without = measurePushBack(N, false);
    GrowthCost with    = measurePushBack(N, true);

    std::cout << "插入 " << N << " 個 int：" << std::endl;
    std::cout << "  不使用 reserve: 重新配置 " << without.reallocations
              << " 次, 搬移 " << without.elementsMoved << " 個元素"
              << std::endl;
    std::cout << "  使用 reserve  : 重新配置 " << with.reallocations
              << " 次, 搬移 " << with.elementsMoved << " 個元素" << std::endl;
    std::cout << "  → 多搬的元素數約等於整份資料再複製一遍"
              << "（等比級數和 ≈ 2n 的實證）" << std::endl;

    // 反例：在迴圈裡逐次 reserve，會關掉幾何成長 → 比不 reserve 還慢
    std::vector<int> bad;
    int badRealloc = 0;
    std::size_t lastCap = bad.capacity();
    for (int i = 0; i < 10; ++i) {
        bad.reserve(bad.size() + 1);   // 反模式！
        bad.push_back(i);
        if (bad.capacity() != lastCap) { ++badRealloc; lastCap = bad.capacity(); }
    }
    GrowthCost plain = measurePushBack(10, false);
    std::cout << "\n反模式對照（只推 10 個元素）：" << std::endl;
    std::cout << "  每輪 reserve(size()+1): 重新配置 " << badRealloc << " 次"
              << std::endl;
    std::cout << "  什麼都不做的 push_back: 重新配置 " << plain.reallocations
              << " 次  ← reserve 反而更慢" << std::endl;
}

// ===== 重點八：實際應用場景 =====

// 場景：收集不確定數量的結果 —— 以保守上界 reserve，完成後視需要 shrink
std::vector<int> find_even_numbers(const std::vector<int>& input) {
    std::vector<int> result;
    result.reserve(input.size());  // 最多有 input.size() 個偶數（保守上界）

    for (int x : input) {
        if (x % 2 == 0) {
            result.push_back(x);
        }
    }

    result.shrink_to_fit();  // 釋放多餘空間（非強制請求）
    return result;
}

void demo_practical_usage() {
    std::cout << "\n===== 重點八：實際應用場景 =====\n";

    // 場景一：固定大小緩衝區 → 用建構子（會 value-initialize，可直接索引）
    std::vector<uint8_t> buffer(1024);
    std::cout << "buffer(1024): size=" << buffer.size()
              << ", capacity=" << buffer.capacity()
              << "（已歸零，可直接 buffer[i] 存取）" << std::endl;

    // 場景二：只配置空間，稍後 push_back 填入
    std::vector<uint8_t> buffer2;
    buffer2.reserve(1024);
    std::cout << "buffer2.reserve(1024): size=" << buffer2.size()
              << ", capacity=" << buffer2.capacity()
              << "（size 為 0，buffer2[i] 是 UB）" << std::endl;

    // 場景三：篩選結果
    std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::vector<int> evens = find_even_numbers(nums);
    std::cout << "偶數結果: [";
    for (std::size_t i = 0; i < evens.size(); ++i) {
        if (i != 0) std::cout << ' ';
        std::cout << evens[i];
    }
    std::cout << "]  size=" << evens.size()
              << ", capacity=" << evens.capacity() << std::endl;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 118. Pascal's Triangle
//   題目：給定 numRows，回傳巴斯卡三角形的前 numRows 列。
//   為什麼用到本主題：輸出大小**完全已知** —— 外層剛好 numRows 列，
//     第 i 列剛好 i+1 個元素。這是 reserve 的教科書級適用場景：
//     外層 reserve(numRows)、每列 reserve(i+1)，全程零重新配置。
//   複雜度：時間 O(numRows²)，空間 O(numRows²)（即輸出大小）。
// -----------------------------------------------------------------------------
std::vector<std::vector<int>> generatePascalTriangle(int numRows) {
    std::vector<std::vector<int>> triangle;
    if (numRows <= 0) return triangle;

    triangle.reserve(static_cast<std::size_t>(numRows));  // 列數已知

    for (int i = 0; i < numRows; ++i) {
        std::vector<int> row;
        row.reserve(static_cast<std::size_t>(i) + 1);     // 本列長度已知

        for (int j = 0; j <= i; ++j) {
            if (j == 0 || j == i) {
                row.push_back(1);
            } else {
                row.push_back(triangle[i - 1][j - 1] + triangle[i - 1][j]);
            }
        }
        triangle.push_back(std::move(row));
    }
    return triangle;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 54. Spiral Matrix
//   題目：以螺旋順序回傳 m x n 矩陣的所有元素。
//   為什麼用到本主題：結果長度**精確等於 m * n**，在讀第一個元素之前就知道。
//     reserve(m * n) 讓整趟走訪零重新配置、零搬移 —— 正是「已知精確筆數」
//     這個模式（對照 9.cpp 的檔頭宣告筆數）。
//   複雜度：時間 O(m*n)，空間 O(1)（不計輸出）。
// -----------------------------------------------------------------------------
std::vector<int> spiralOrder(const std::vector<std::vector<int>>& matrix) {
    std::vector<int> result;
    if (matrix.empty() || matrix[0].empty()) return result;

    const int rows = static_cast<int>(matrix.size());
    const int cols = static_cast<int>(matrix[0].size());
    result.reserve(static_cast<std::size_t>(rows) * cols);  // 精確大小已知

    int top = 0, bottom = rows - 1, left = 0, right = cols - 1;
    while (top <= bottom && left <= right) {
        for (int j = left; j <= right; ++j)   result.push_back(matrix[top][j]);
        ++top;
        for (int i = top; i <= bottom; ++i)   result.push_back(matrix[i][right]);
        --right;
        if (top <= bottom) {
            for (int j = right; j >= left; --j) result.push_back(matrix[bottom][j]);
            --bottom;
        }
        if (left <= right) {
            for (int i = bottom; i >= top; --i) result.push_back(matrix[i][left]);
            ++left;
        }
    }
    return result;
}

void demo_leetcode() {
    std::cout << "\n===== LeetCode 實戰 =====\n";

    std::cout << "--- LeetCode 118. Pascal's Triangle (numRows=5) ---"
              << std::endl;
    std::vector<std::vector<int>> tri = generatePascalTriangle(5);
    for (const std::vector<int>& row : tri) {
        std::cout << "  [";
        for (std::size_t i = 0; i < row.size(); ++i) {
            if (i != 0) std::cout << ' ';
            std::cout << row[i];
        }
        std::cout << "]  size=" << row.size()
                  << ", capacity=" << row.capacity() << std::endl;
    }
    std::cout << "  每列 capacity 都剛好等於長度 → 全程零重新配置" << std::endl;

    std::cout << "--- LeetCode 54. Spiral Matrix (3x4) ---" << std::endl;
    std::vector<std::vector<int>> matrix = {
        {1,  2,  3,  4},
        {5,  6,  7,  8},
        {9, 10, 11, 12}
    };
    std::vector<int> spiral = spiralOrder(matrix);
    std::cout << "  [";
    for (std::size_t i = 0; i < spiral.size(); ++i) {
        if (i != 0) std::cout << ' ';
        std::cout << spiral[i];
    }
    std::cout << "]" << std::endl;
    std::cout << "  size=" << spiral.size()
              << ", capacity=" << spiral.capacity()
              << " → reserve(m*n) 精確命中" << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】log 批次緩衝區：每輪 clear() 重複使用同一塊記憶體
//   情境：日誌收集器每滿 N 筆就 flush，然後清空繼續收。
//   刻意**只用 clear()、不用 shrink_to_fit()** —— 保留 capacity 讓後續
//   每一輪都免於重新配置，正是這個模式的重點。
// -----------------------------------------------------------------------------
void demo_log_buffer() {
    std::cout << "\n===== 日常實務 1：log buffer 重複使用 =====\n";

    std::vector<std::string> logBuffer;
    const int batchSize  = 200;
    const int batchCount = 5;
    int reallocations = 0;
    std::size_t lastCap = logBuffer.capacity();

    for (int batch = 0; batch < batchCount; ++batch) {
        for (int i = 0; i < batchSize; ++i) {
            logBuffer.push_back("2026-07-20 10:00:00 [INFO] request handled");
            if (logBuffer.capacity() != lastCap) {
                ++reallocations;
                lastCap = logBuffer.capacity();
            }
        }
        // flush 到磁碟（此處省略），然後清空但**保留 capacity**
        logBuffer.clear();
    }

    std::cout << "處理 " << batchCount << " 批 x " << batchSize
              << " 筆，全程只重新配置 " << reallocations << " 次" << std::endl;
    std::cout << "結束時 size=" << logBuffer.size()
              << ", capacity=" << logBuffer.capacity()
              << "（capacity 留給下一輪用）" << std::endl;
    std::cout << "若每輪都 shrink_to_fit()，第 2 批之後每批都要重新成長一次。"
              << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】組裝長度前綴封包：已知最終大小 → 一次 reserve 到位
//   格式：[4 bytes 長度][payload]，常見於自訂 TCP 協定。
// -----------------------------------------------------------------------------
std::vector<uint8_t> buildPacket(const std::string& payload) {
    std::vector<uint8_t> packet;
    packet.reserve(4 + payload.size());   // 最終大小完全已知

    const uint32_t len = static_cast<uint32_t>(payload.size());
    packet.push_back(static_cast<uint8_t>((len >> 24) & 0xFF));
    packet.push_back(static_cast<uint8_t>((len >> 16) & 0xFF));
    packet.push_back(static_cast<uint8_t>((len >> 8)  & 0xFF));
    packet.push_back(static_cast<uint8_t>( len        & 0xFF));

    packet.insert(packet.end(), payload.begin(), payload.end());
    return packet;
}

void demo_packet() {
    std::cout << "\n===== 日常實務 2：組裝長度前綴封包 =====\n";

    const std::string payload = "GET /metrics HTTP/1.1";
    std::vector<uint8_t> packet = buildPacket(payload);

    std::cout << "payload 長度: " << payload.size() << " bytes" << std::endl;
    std::cout << "封包 size=" << packet.size()
              << ", capacity=" << packet.capacity()
              << "（一次配置到位，過程零重新配置）" << std::endl;
    std::cout << "長度前綴解回 = " << static_cast<int>(packet[3]) << std::endl;
}

// ===== 容量管理函數一覽表 =====
// | 函數            | 說明                                               |
// |-----------------|----------------------------------------------------|
// | size()          | 回傳目前元素數量                                    |
// | capacity()      | 回傳目前配置空間可容納的元素數量                     |
// | empty()         | 回傳 size() == 0                                    |
// | max_size()      | 回傳理論最大容量（實作定義）                         |
// | reserve(n)      | 確保 capacity >= n，不改變 size；n <= cap 時不做事    |
// | resize(n)       | 改變 size 為 n，必要時擴大 capacity                  |
// | resize(n, val)  | 同上，新元素用 val 填充                              |
// | shrink_to_fit() | **非強制請求**釋放多餘 capacity（C++11）             |
// | clear()         | 移除所有元素，size 變 0，**capacity 不變**           |

int main() {
    std::cout << "====================================================\n";
    std::cout << " 第11課：vector 的容量管理 — 總複習\n";
    std::cout << "====================================================\n";

    demo_size_capacity_empty();
    demo_reserve();
    demo_resize();
    demo_reserve_vs_resize();
    demo_shrink_to_fit();
    demo_clearing_methods();
    demo_performance();
    demo_practical_usage();
    demo_leetcode();
    demo_log_buffer();
    demo_packet();

    std::cout << "\n====================================================\n";
    std::cout << " 複習完畢！\n";
    std::cout << "====================================================\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary
//   （效能結論請另用 -O2 量測；本檔輸出的是確定性計數，與最佳化等級無關）

// 【實測時間】本機 Dell Precision 7550 / GCC 15.2 / -O2，插入 100 萬個 int：
//     不使用 reserve: 約 1700–2500 微秒
//     使用 reserve:   約 1100–1190 微秒
//   同一份程式 -O0 重測：約 8648 / 7380 微秒（差距被未內聯的呼叫開銷稀釋）。
//   **牆鐘時間每次執行都不同**，上列僅供比例參考，不可寫進測試斷言。
//
// 註：以下所有 capacity 數值皆為 libstdc++ / GCC 15.2 實測，**非標準保證**。
//     標準只要求 push_back 為 amortized O(1)，未規定成長倍率
//     （libstdc++ 實測 2×、MSVC 為 1.5×）；shrink_to_fit 是 non-binding
//     request，允許實作完全忽略；max_size 為 PTRDIFF_MAX / sizeof(T)，
//     隨元素型別與實作而異。

// === 預期輸出 ===
// ====================================================
//  第11課：vector 的容量管理 — 總複習
// ====================================================
//
// ===== 重點一：size / capacity / empty =====
// size:     5
// capacity: 5
// empty:    0
// 空 vector size: 0
// 空 vector empty: 1
// vector<int>  max_size: 2305843009213693951
// vector<char> max_size: 9223372036854775807
//
// ===== 重點二：reserve 預先配置空間 =====
// 初始 - size: 0, capacity: 0
// reserve(100) 後 - size: 0, capacity: 100
// 100 次 push_back 後 - size: 100, capacity: 100
// 資料是否搬移過: 否
// reserve(10) 後 capacity: 100（不縮減）
// 推第 101 個後 capacity: 200
//
// ===== 重點三：resize 改變元素數量 =====
// 初始 size: 5, capacity: 5
// resize(8): 1 2 3 4 5 0 0 0  (capacity=10，注意不是 8：resize 走幾何成長 max(2*size, n))
// resize(10, 99): 1 2 3 4 5 0 0 0 99 99
// resize(3): 1 2 3
// 縮小後 capacity: 10（記憶體沒有還給系統）
//
// ===== 重點四：reserve vs resize 對照 =====
// reserve(5): size=0, capacity=5
// resize(5):  size=5, capacity=5
// v1.at(0) 丟出 std::out_of_range → size 確實是 0
// （寫成 v1[0] = 10 則是 UB，不保證會被偵測到）
// resize 後 v2[0] = 10
// reserve 後 push_back，v1[0] = 99
//
// ===== 重點五：shrink_to_fit 釋放多餘空間 =====
// shrink 前: size=10, capacity=1000
// 浪費了 3960 bytes
// shrink 後: size=10, capacity=10
// 註: capacity 真的下降是 libstdc++ 實測結果；標準規定 shrink_to_fit 是 non-binding request，允許實作忽略。
//
// ===== 重點六：清空 vector 的方式比較 =====
// clear 前:  size=5, capacity=100
// clear 後:  size=0, capacity=100（capacity 不變）
// swap 前:   size=5, capacity=100
// swap 後:   size=0, capacity=0
// c+s 前:    size=5, capacity=100
// c+s 後:    size=0, capacity=0
// 縮容變體:  size=5, capacity=5（內容保留）
//
// ===== 重點七：確定性成本比較（reserve vs 無 reserve）=====
// 插入 1000000 個 int：
//   不使用 reserve: 重新配置 21 次, 搬移 1048575 個元素
//   使用 reserve  : 重新配置 0 次, 搬移 0 個元素
//   → 多搬的元素數約等於整份資料再複製一遍（等比級數和 ≈ 2n 的實證）
//
// 反模式對照（只推 10 個元素）：
//   每輪 reserve(size()+1): 重新配置 10 次
//   什麼都不做的 push_back: 重新配置 5 次  ← reserve 反而更慢
//
// ===== 重點八：實際應用場景 =====
// buffer(1024): size=1024, capacity=1024（已歸零，可直接 buffer[i] 存取）
// buffer2.reserve(1024): size=0, capacity=1024（size 為 0，buffer2[i] 是 UB）
// 偶數結果: [2 4 6 8 10]  size=5, capacity=5
//
// ===== LeetCode 實戰 =====
// --- LeetCode 118. Pascal's Triangle (numRows=5) ---
//   [1]  size=1, capacity=1
//   [1 1]  size=2, capacity=2
//   [1 2 1]  size=3, capacity=3
//   [1 3 3 1]  size=4, capacity=4
//   [1 4 6 4 1]  size=5, capacity=5
//   每列 capacity 都剛好等於長度 → 全程零重新配置
// --- LeetCode 54. Spiral Matrix (3x4) ---
//   [1 2 3 4 8 12 11 10 9 5 6 7]
//   size=12, capacity=12 → reserve(m*n) 精確命中
//
// ===== 日常實務 1：log buffer 重複使用 =====
// 處理 5 批 x 200 筆，全程只重新配置 9 次
// 結束時 size=0, capacity=256（capacity 留給下一輪用）
// 若每輪都 shrink_to_fit()，第 2 批之後每批都要重新成長一次。
//
// ===== 日常實務 2：組裝長度前綴封包 =====
// payload 長度: 21 bytes
// 封包 size=25, capacity=25（一次配置到位，過程零重新配置）
// 長度前綴解回 = 21
//
// ====================================================
//  複習完畢！
// ====================================================
