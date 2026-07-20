// =============================================================================
//  第 20 課：vector 效能分析與最佳實踐 9  —  erase-remove 慣用法
// =============================================================================
//
// 【主題資訊 Information】
//   ForwardIt std::remove_if(ForwardIt first, ForwardIt last, Pred p);  // <algorithm>
//   iterator  vector::erase(const_iterator first, const_iterator last);
//   template <class T, class Alloc, class Pred>
//   typename vector<T,Alloc>::size_type
//   std::erase_if(vector<T,Alloc>& c, Pred pred);                       // C++20
//
//   標頭檔：<algorithm>、<vector>
//   複雜度：erase-remove 為 O(n)；逐一 erase 為 O(n²)
//   本檔標準：**C++20**（因為用到 std::erase_if；已用 -pedantic-errors 驗證
//             此函式在 -std=c++17 下不存在）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼逐一 erase 是 O(n²)】
//   vector 的元素必須連續。erase 掉中間某個元素後，後面所有元素都要往前搬一格：
//       erase(v.begin() + k)  →  搬移 (n - k - 1) 個元素，成本 O(n)
//   若要刪掉 n/2 個元素，就是 n/2 次 O(n) 的搬移 → 總共 O(n²)。
//   本檔第 1 節用「元素被搬移的次數」實測這個差距，數字完全可重現。
//
// 【2. remove_if 其實不刪任何東西】
//   這是整個慣用法最反直覺的地方。std::remove_if **不能**改變容器大小——
//   它只拿到一對 iterator，根本碰不到容器物件本身。它做的是：
//       把「要保留」的元素，往前搬到陣列的前段（用移動賦值），
//       然後回傳一個 iterator，指向「保留區的尾端」。
//   保留區之後那一段，內容是「合法但未指定」（通常是被移走後的空殼）。
//   所以 remove_if 之後 v.size() **完全沒變**，你必須自己呼叫 erase 把尾巴切掉：
//       v.erase(std::remove_if(v.begin(), v.end(), pred), v.end());
//   這就是為什麼叫「erase-remove 慣用法」——兩個動作缺一不可。
//
// 【3. 為什麼這樣就變成 O(n)】
//   remove_if 用的是「讀寫雙指標」單趟掃描：
//       read 指標從頭走到尾（n 次）
//       write 指標只在遇到要保留的元素時前進
//   每個元素最多被搬移一次，總共 O(n)。
//   最後的 erase(尾巴) 只需要銷毀尾段元素並調整 size，也是 O(n)。
//   相較之下，逐一 erase 讓同一個元素被反覆往前搬很多次，這才是 O(n²) 的來源。
//
// 【4. C++20 的 std::erase_if】
//   C++20 把這個慣用法包成一個函式，直接對容器操作：
//       std::erase_if(v, pred);          // 回傳被刪除的元素個數
//   它不只更短，也消除了「忘記寫 erase」這個經典 bug。
//   ⚠️ 這是 C++20 才有的（已用 -pedantic-errors 驗證 C++17 下編譯失敗），
//      所以本檔的編譯指令是 -std=c++20。
//   對應的還有 std::erase(v, value)（依值刪除）。
//
// 【概念補充 Concept Deep Dive】
//   ● remove_if 之後的「尾段」到底是什麼
//     標準說那些元素處於「合法但未指定（valid but unspecified）」的狀態。
//     對 int 這種平凡型別，實作通常就是原本的舊值沒被覆蓋；
//     對 std::string，被移走後通常變成空字串。
//     **不可以讀取這些值並依賴它們**——這正是必須立刻 erase 掉的原因。
//
//   ● 為什麼 remove_if 不能直接刪
//     因為演算法只認 iterator，不認容器。std::remove_if 對
//     vector、deque、array、甚至原始陣列都能用——它不知道自己面對的是什麼，
//     自然也無法呼叫容器的 erase。這是 STL「演算法與容器解耦」設計的必然結果：
//     換來泛用性，代價就是這個兩段式慣用法。
//
//   ● list 的 remove_if 是另一回事
//     std::list 有自己的成員函式 lst.remove_if(pred)，那個是**真的會刪除**的，
//     而且因為 list 刪節點是 O(1)，總複雜度 O(n)、不搬移任何元素。
//     別把 std::remove_if（演算法）和 list::remove_if（成員函式）搞混。
//
//   ● 穩定性
//     erase-remove 是**穩定**的：保留下來的元素維持原本的相對順序。
//     若你不需要維持順序，還有更快的 swap-and-pop（見同課 10.cpp），
//     單次刪除是 O(1)。
//
// 【注意事項 Pay Attention】
//   1. **只寫 remove_if 不寫 erase 是經典 bug**：容器大小完全沒變，
//      尾巴留著一堆未指定的殘值，而且編譯器不會警告。
//   2. remove_if 回傳的 iterator 是「新的邏輯結尾」，要搭配 v.end() 一起 erase。
//   3. erase-remove 之後 capacity **不會**縮小；要歸還記憶體得額外 shrink_to_fit()。
//   4. std::erase_if 是 C++20；C++17 專案只能用 erase-remove 慣用法。
//   5. 對 std::list / forward_list 請用它們自己的成員函式 remove_if，效率更好。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】erase-remove 慣用法
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::remove_if 會把元素從 vector 裡刪掉嗎？
//     答：不會。它只拿到一對 iterator，碰不到容器本身，因此無法改變 size。
//         它把要保留的元素往前搬，回傳「新邏輯結尾」的 iterator。
//         必須再呼叫 v.erase(那個 iterator, v.end()) 才真的縮小容器。
//     追問：只寫 remove_if 不寫 erase 會怎樣？→ size() 完全沒變，
//         尾段留著「合法但未指定」的殘值。程式不會崩潰，但邏輯已經錯了，
//         而且編譯器不會有任何警告——這是很難抓的 bug。
//
// 🔥 Q2. 為什麼逐一 erase 是 O(n²)，而 erase-remove 是 O(n)？
//     答：vector 元素連續，erase 中間元素要把後面全部往前搬，單次 O(n)；
//         刪 n/2 個就是 O(n²)。remove_if 用讀寫雙指標單趟掃描，
//         每個元素最多搬一次，總共 O(n)，最後只做一次尾段 erase。
//     追問：有沒有更快的？→ 若不需要維持順序，用 swap-and-pop
//         （把要刪的和最後一個交換再 pop_back），單次刪除是 O(1)。
//
// ⚠️ 陷阱. 在 for 迴圈裡邊走邊 erase，寫成
//         for (auto it = v.begin(); it != v.end(); ++it)
//             if (pred(*it)) v.erase(it);
//         哪裡錯了？
//     答：erase 之後 it 已經失效，接著 ++it 是未定義行為。
//         正確寫法是接住回傳值：it = v.erase(it);，且該分支**不要**再 ++it
//         （erase 回傳的已經是下一個元素的位置，再 ++ 就會跳過一個）。
//     為什麼會錯：把 vector 的 iterator 想像成「指向某個編號的元素」，
//         以為刪掉一個之後它會自動指到下一個。實際上 erase 之後
//         該位置以後的所有 iterator 全部失效，it 這個變數已經不能再用了。
//         這也是為什麼即使寫對了，這個 O(n²) 迴圈仍然應該換成 erase-remove。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <string>

// -----------------------------------------------------------------------------
// RAII 計時器。
// 【修正 1】成員宣告順序改成與初始化列表一致（原版觸發 -Wreorder 警告）。
// 【修正 2】結果印到 std::cerr——耗時每次執行都不同，不應混進 stdout。
// -----------------------------------------------------------------------------
struct Timer {
    std::string label_;                                     // 先宣告 → 先初始化
    std::chrono::high_resolution_clock::time_point start_;  // 後宣告 → 後初始化

    explicit Timer(const std::string& label)
        : label_(label),
          start_(std::chrono::high_resolution_clock::now()) {}

    ~Timer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
        std::cerr << "  [計時] " << label_ << ": " << us << " μs" << std::endl;
    }
};

// -----------------------------------------------------------------------------
// 儀器型別：計算「元素被搬移（賦值）了幾次」。
// 用計數而非計時當證據——它精確反映複雜度，且每次執行完全相同。
// -----------------------------------------------------------------------------
struct Item {
    static long assignments;
    int v;
    Item(int x = 0) : v(x) {}
    Item(const Item&) = default;
    Item& operator=(const Item& o) { v = o.v; ++assignments; return *this; }
    Item& operator=(Item&& o) noexcept { v = o.v; ++assignments; return *this; }
    Item(Item&&) noexcept = default;
};
long Item::assignments = 0;

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 27. Remove Element
//   題目：就地移除 nums 中所有等於 val 的元素，回傳剩餘長度 k。
//   為什麼用到本主題：這題就是 erase-remove 的教科書案例。
//     LeetCode 只要求回傳 k（前 k 個是答案），所以連 erase 都可以省略，
//     直接回傳 std::remove 的位置——這正好凸顯「remove 不刪東西，
//     只是把保留元素往前搬並回報新結尾」這個核心觀念。
// -----------------------------------------------------------------------------
int removeElement(std::vector<int>& nums, int val) {
    auto newEnd = std::remove(nums.begin(), nums.end(), val);
    return static_cast<int>(newEnd - nums.begin());   // 這就是 k
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 26. Remove Duplicates from Sorted Array
//   題目：已排序陣列就地去重，回傳新長度。
//   為什麼用到本主題：std::unique 和 std::remove_if 是同一個家族——
//     它們都「不刪除、只前移並回傳新結尾」。同樣要搭配 erase 才會真的縮小容器。
// -----------------------------------------------------------------------------
int removeDuplicates(std::vector<int>& nums) {
    auto newEnd = std::unique(nums.begin(), nums.end());
    return static_cast<int>(newEnd - nums.begin());
}

// -----------------------------------------------------------------------------
// 【日常實務範例】連線池清理：移除所有已斷線 / 逾時的連線
//   情境：伺服器維護一個連線池，每輪心跳後要把失效的連線清掉。
//         連線池可能有上萬筆，若用「逐一 erase」就是 O(n²)，
//         在高連線數下會直接吃掉 CPU。這是 erase-remove 最典型的實務場景。
// -----------------------------------------------------------------------------
struct Connection {
    int id;
    std::string state;      // "active" / "closed" / "timeout"
    int idleSeconds;
};

std::size_t reapDeadConnections(std::vector<Connection>& pool, int maxIdle) {
    // C++20：一行完成，且回傳被移除的筆數
    return std::erase_if(pool, [maxIdle](const Connection& c) {
        return c.state != "active" || c.idleSeconds > maxIdle;
    });
}

int main() {
    std::cout << "=== 1. 可重現證據：元素被搬移幾次（計數，非計時）===\n";
    const int N = 20000;
    std::cout << "資料量 " << N << " 個元素，刪除其中所有偶數（約一半）\n\n";

    // 方法一：逐一 erase → O(n²)
    Item::assignments = 0;
    {
        std::vector<Item> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) v.push_back(Item(i));

        for (auto it = v.begin(); it != v.end(); ) {
            if (it->v % 2 == 0) {
                it = v.erase(it);      // 每次 erase 都搬移後面的元素
            } else {
                ++it;
            }
        }
    }
    long assignErase = Item::assignments;

    // 方法二：erase-remove → O(n)
    Item::assignments = 0;
    {
        std::vector<Item> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) v.push_back(Item(i));

        v.erase(std::remove_if(v.begin(), v.end(),
                               [](const Item& x) { return x.v % 2 == 0; }),
                v.end());
    }
    long assignRemove = Item::assignments;

    std::cout << "逐一 erase   ：搬移 " << assignErase << " 次\n";
    std::cout << "erase-remove ：搬移 " << assignRemove << " 次\n";
    if (assignRemove > 0) {
        std::cout << "→ 相差 " << (assignErase / assignRemove)
                  << " 倍，且差距會隨 N 線性放大（O(n²) vs O(n)）\n";
    }
    std::cout << "（這組數字每次執行完全相同，比耗時更能說明複雜度差異）\n";

    std::cout << "\n=== 2. 關鍵觀念：remove_if 不會改變 size ===\n";
    std::vector<int> demo = {1, 2, 3, 4, 5, 6, 7, 8};
    std::cout << "原始      ：";
    for (int x : demo) std::cout << x << " ";
    std::cout << "(size=" << demo.size() << ")\n";

    auto newEnd = std::remove_if(demo.begin(), demo.end(),
                                 [](int x) { return x % 2 == 0; });
    std::cout << "remove_if 後：size = " << demo.size()
              << " ← 完全沒變！\n";
    std::cout << "保留區 [begin, newEnd) ：";
    for (auto it = demo.begin(); it != newEnd; ++it) std::cout << *it << " ";
    std::cout << "(共 " << (newEnd - demo.begin()) << " 個)\n";
    std::cout << "尾段 [newEnd, end) 的內容是「合法但未指定」，不可依賴其值\n";

    demo.erase(newEnd, demo.end());          // 這一步才真的刪掉
    std::cout << "再 erase 後 ：";
    for (int x : demo) std::cout << x << " ";
    std::cout << "(size=" << demo.size() << ")\n";

    std::cout << "\n=== 3. C++20 std::erase_if：一行搞定 ===\n";
    std::vector<int> c20 = {1, 2, 3, 4, 5, 6, 7, 8};
    auto removed = std::erase_if(c20, [](int x) { return x % 2 == 0; });
    std::cout << "std::erase_if 回傳被刪除的個數 = " << removed << "\n";
    std::cout << "結果：";
    for (int x : c20) std::cout << x << " ";
    std::cout << "(size=" << c20.size() << ")\n";
    std::cout << "（已用 -pedantic-errors 驗證：此函式在 -std=c++17 下不存在）\n";

    std::cout << "\n=== 4. erase-remove 不會縮小 capacity ===\n";
    std::vector<int> cap(1000);
    std::cout << "初始       ：size=" << cap.size()
              << " capacity=" << cap.capacity() << "\n";
    std::erase_if(cap, [](int) { return true; });     // 全刪
    std::cout << "全部刪除後 ：size=" << cap.size()
              << " capacity=" << cap.capacity() << " ← capacity 沒變\n";
    cap.shrink_to_fit();
    std::cout << "shrink_to_fit 後：capacity=" << cap.capacity() << "\n";

    std::cout << "\n=== 5. 原始三方法的實際計時（結果印在 stderr）===\n";
    const int BIG = 100'000;
    {
        std::vector<int> v;
        v.reserve(BIG);
        for (int i = 0; i < BIG; ++i) v.push_back(i);
        Timer t("逐一 erase");
        for (auto it = v.begin(); it != v.end(); ) {
            if (*it % 2 == 0) it = v.erase(it);
            else ++it;
        }
    }
    {
        std::vector<int> v;
        v.reserve(BIG);
        for (int i = 0; i < BIG; ++i) v.push_back(i);
        Timer t("erase-remove");
        v.erase(std::remove_if(v.begin(), v.end(),
                               [](int x) { return x % 2 == 0; }),
                v.end());
    }
    {
        std::vector<int> v;
        v.reserve(BIG);
        for (int i = 0; i < BIG; ++i) v.push_back(i);
        Timer t("std::erase_if (C++20)");
        std::erase_if(v, [](int x) { return x % 2 == 0; });
    }
    std::cout << "三種方法的耗時已印到 stderr（每次執行都不同，不列入預期輸出）\n";
    std::cout << "只看 stdout 請用：./demo9 2>/dev/null\n";

    std::cout << "\n=== LeetCode 27. Remove Element ===\n";
    std::vector<int> nums1 = {0, 1, 2, 2, 3, 0, 4, 2};
    int k1 = removeElement(nums1, 2);
    std::cout << "移除所有 2 → k = " << k1 << "，前 k 個：";
    for (int i = 0; i < k1; ++i) std::cout << nums1[static_cast<std::size_t>(i)] << " ";
    std::cout << "\n（注意：k 之後的內容未指定，判題器不會檢查）\n";

    std::cout << "\n=== LeetCode 26. Remove Duplicates from Sorted Array ===\n";
    std::vector<int> nums2 = {0, 0, 1, 1, 1, 2, 2, 3, 3, 4};
    int k2 = removeDuplicates(nums2);
    std::cout << "去重 → k = " << k2 << "，前 k 個：";
    for (int i = 0; i < k2; ++i) std::cout << nums2[static_cast<std::size_t>(i)] << " ";
    std::cout << "\n（std::unique 與 std::remove 同一家族：只前移、不刪除）\n";

    std::cout << "\n=== 日常實務：連線池清理 ===\n";
    std::vector<Connection> pool = {
        {101, "active",  3},
        {102, "closed",  0},
        {103, "active", 95},
        {104, "timeout", 60},
        {105, "active", 12},
        {106, "closed", 30},
        {107, "active",  1}
    };
    std::cout << "清理前 " << pool.size() << " 條連線\n";
    std::size_t reaped = reapDeadConnections(pool, 90);   // 閒置超過 90 秒也砍
    std::cout << "清理掉 " << reaped << " 條（closed / timeout / 閒置 > 90 秒）\n";
    std::cout << "剩餘 " << pool.size() << " 條：";
    for (const auto& c : pool) std::cout << c.id << " ";
    std::cout << "\n→ 若改用逐一 erase，上萬條連線時就是 O(n²)，"
                 "每輪心跳都會吃掉大量 CPU。\n";

    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra "第 20 課：vector 效能分析與最佳實踐9.cpp" -o demo9
// 只看 stdout: ./demo9 2>/dev/null
//
// ⚠️ 但書：
//   1. 本檔需要 C++20（std::erase_if）。已用 -pedantic-errors 驗證：
//      -std=c++17 下會出現 "'erase_if' is not a member of 'std'"。
//   2. 第 5 節的耗時印在 stderr，每次執行都不同，故不納入下方預期輸出。
//   3. remove_if 之後尾段元素為「合法但未指定」，本檔不讀取其值。

// === 預期輸出 ===
// === 1. 可重現證據：元素被搬移幾次（計數，非計時）===
// 資料量 20000 個元素，刪除其中所有偶數（約一半）
//
// 逐一 erase   ：搬移 100000000 次
// erase-remove ：搬移 10000 次
// → 相差 10000 倍，且差距會隨 N 線性放大（O(n²) vs O(n)）
// （這組數字每次執行完全相同，比耗時更能說明複雜度差異）
//
// === 2. 關鍵觀念：remove_if 不會改變 size ===
// 原始      ：1 2 3 4 5 6 7 8 (size=8)
// remove_if 後：size = 8 ← 完全沒變！
// 保留區 [begin, newEnd) ：1 3 5 7 (共 4 個)
// 尾段 [newEnd, end) 的內容是「合法但未指定」，不可依賴其值
// 再 erase 後 ：1 3 5 7 (size=4)
//
// === 3. C++20 std::erase_if：一行搞定 ===
// std::erase_if 回傳被刪除的個數 = 4
// 結果：1 3 5 7 (size=4)
// （已用 -pedantic-errors 驗證：此函式在 -std=c++17 下不存在）
//
// === 4. erase-remove 不會縮小 capacity ===
// 初始       ：size=1000 capacity=1000
// 全部刪除後 ：size=0 capacity=1000 ← capacity 沒變
// shrink_to_fit 後：capacity=0
//
// === 5. 原始三方法的實際計時（結果印在 stderr）===
// 三種方法的耗時已印到 stderr（每次執行都不同，不列入預期輸出）
// 只看 stdout 請用：./demo9 2>/dev/null
//
// === LeetCode 27. Remove Element ===
// 移除所有 2 → k = 5，前 k 個：0 1 3 0 4
// （注意：k 之後的內容未指定，判題器不會檢查）
//
// === LeetCode 26. Remove Duplicates from Sorted Array ===
// 去重 → k = 5，前 k 個：0 1 2 3 4
// （std::unique 與 std::remove 同一家族：只前移、不刪除）
//
// === 日常實務：連線池清理 ===
// 清理前 7 條連線
// 清理掉 4 條（closed / timeout / 閒置 > 90 秒）
// 剩餘 3 條：101 105 107
// → 若改用逐一 erase，上萬條連線時就是 O(n²)，每輪心跳都會吃掉大量 CPU。
