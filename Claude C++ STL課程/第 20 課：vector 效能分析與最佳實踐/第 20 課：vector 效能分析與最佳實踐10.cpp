// =============================================================================
//  第 20 課：vector 效能分析與最佳實踐 10  —  swap-and-pop：O(1) 的無序刪除
// =============================================================================
//
// 【主題資訊 Information】
//   template <typename T>
//   void unstable_erase(std::vector<T>& v, size_t index);
//     作法：v[index] = std::move(v.back());  v.pop_back();
//
//   標頭檔：<vector>、<utility>（std::move）
//   複雜度：O(1)（相對地，v.erase(v.begin() + i) 是 O(n)）
//   本檔標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼一般的 erase 是 O(n)】
//   vector 的元素必須連續。刪掉索引 i 的元素後，
//   索引 i+1 到 n-1 的所有元素都要往前搬一格，才能補上那個洞：
//       [10][20][30][40][50]  刪索引 1
//       [10][30][40][50]      ← 30、40、50 各搬了一次
//   刪除位置越靠前，要搬的越多；刪第一個就是搬 n-1 個，成本 O(n)。
//
// 【2. swap-and-pop 的核心洞察】
//   「補洞」不一定要靠整體平移。只要有東西填進那個洞就行，
//   而**最後一個元素**是最便宜的來源——把它搬走不需要平移任何其他元素：
//       [10][20][30][40][50]  刪索引 1
//       [10][50][30][40]      ← 把 50 移到索引 1，然後 pop_back
//   總共只做了「一次移動賦值 + 一次 pop_back」，與 n 完全無關 → O(1)。
//
// 【3. 代價：順序被破壞】
//   這是一筆明碼標價的交易——你用「順序」換「速度」。
//   刪除後元素的相對順序改變了（50 跑到原本 20 的位置）。
//   所以只有在**順序無意義**時才能用：
//     ✅ 適用：粒子系統、遊戲實體清單、連線池、待處理工作集合、
//              任何「這是一個集合，不是一個序列」的場合
//     ❌ 不適用：使用者看得到順序的清單、時間序列、已排序的資料、
//              任何依賴索引穩定性的結構
//
// 【4. 為什麼要用 std::move】
//       v[index] = std::move(v.back());
//   若寫成 v[index] = v.back()（不加 move），對 std::string / vector
//   這類擁有 heap 資源的型別，會做一次**深複製**——明明那個元素馬上就要
//   被 pop_back 銷毀了，複製它完全是浪費。加上 std::move 之後改為移動賦值，
//   直接接管資源，成本從 O(元素大小) 降到 O(1)。
//   對 int 這種平凡型別則沒有差別（move 和 copy 都是一次暫存器搬移）。
//
// 【概念補充 Concept Deep Dive】
//   ● 名稱由來與其他語言
//     這個技巧常被稱為 "swap and pop" 或 "swap-remove"。
//     Rust 的標準函式庫直接內建了它：Vec::swap_remove(index)，
//     文件明確標示 O(1) 並警告順序會改變。C++ 標準庫沒有對應函式，
//     所以要自己寫（本檔的 unstable_erase）。
//
//   ● 自我刪除的邊界情況
//     若 index 就是最後一個元素，v[index] = std::move(v.back()) 變成
//     「自己移動賦值給自己」。標準對 self-move-assignment 只保證
//     物件處於「合法但未指定」的狀態——對 std::string 可能導致內容變空。
//     這裡因為緊接著就 pop_back 把它銷毀，實務上不會出錯，
//     但**嚴謹的寫法應該先檢查**，本檔的實作已加上這個保護。
//
//   ● 批次刪除時怎麼辦
//     若一次要刪很多個，不要迴圈呼叫 swap-and-pop（索引會一直變動，
//     容易漏刪或誤刪）。批次刪除請用 erase-remove 慣用法（見同課 9.cpp），
//     它是 O(n) 但一次處理完，且穩定。
//     判準：**單點刪除**用 swap-and-pop，**批次條件刪除**用 erase-remove。
//
//   ● 與 unordered_map 的 erase 對比
//     unordered_map 的 erase 本來就是 O(1) 平均，因為它沒有「連續」的包袱。
//     若你發現自己頻繁在 vector 中間刪除，先問：這真的該是 vector 嗎？
//     可能 unordered_set / list 才是對的資料結構。
//
// 【注意事項 Pay Attention】
//   1. **會破壞順序**——這是特性不是 bug，但必須在使用前確認順序無所謂。
//   2. 刪除後，原本指向「最後一個元素」與「被刪位置」的索引／iterator 都失效。
//   3. index 必須在範圍內；本檔實作沒有做邊界檢查（比照 operator[] 的慣例），
//      呼叫端要自己保證，否則是 UB。
//   4. 一定要寫 std::move，否則對含 heap 資源的型別會多一次深複製。
//   5. 迴圈中連續 swap-and-pop 時要小心：剛換過來的元素還沒檢查過，
//      所以刪除後**不要**遞增索引（本檔第 4 節示範正確寫法）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】swap-and-pop
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 從 vector 中間刪一個元素，怎麼做到 O(1)？代價是什麼？
//     答：把最後一個元素移動賦值到要刪的位置，再 pop_back。
//         只做一次移動 + 一次 pop_back，與元素個數無關，所以是 O(1)。
//         代價是**元素順序被破壞**——只能用在順序無意義的集合上。
//     追問：為什麼一般的 erase 做不到？→ vector 要保持元素連續，
//         erase 之後必須把後面所有元素往前搬一格填洞，那是 O(n)。
//
// 🔥 Q2. 為什麼要寫 v[i] = std::move(v.back()) 而不是 v[i] = v.back()？
//     答：v.back() 馬上就要被 pop_back 銷毀了，複製它毫無意義。
//         對 std::string / vector 這種持有 heap 資源的型別，
//         不加 std::move 會做一次深複製；加了才是移動賦值，直接接管資源。
//     追問：對 vector<int> 有差嗎？→ 沒有。int 是平凡型別，
//         移動和複製都只是一次暫存器搬移，std::move 在這裡不產生任何差異。
//
// ⚠️ 陷阱. 用 swap-and-pop 在迴圈裡刪除所有符合條件的元素，寫成
//         for (size_t i = 0; i < v.size(); ++i)
//             if (pred(v[i])) unstable_erase(v, i);
//         哪裡錯了？
//     答：刪除後索引 i 的位置換成了「原本的最後一個元素」，
//         那個元素**還沒被檢查過**，但 ++i 已經把它跳過去了 → 漏刪。
//         正確寫法是：刪除的那一輪**不要** ++i（讓下一輪重新檢查同一個位置）。
//     為什麼會錯：把 swap-and-pop 想成和 erase 一樣「元素整體往前移」，
//         於是以為下一個待檢查的元素會落在 i+1。實際上它落在 **i**。
//         這也是為什麼批次條件刪除應該直接用 erase-remove——
//         它沒有這個心智負擔，而且對「刪很多個」的情境總複雜度一樣是 O(n)。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include <algorithm>

// 把要刪除的元素和最後一個元素交換，然後 pop_back
// 時間複雜度：O(1)
template <typename T>
void unstable_erase(std::vector<T>& v, size_t index) {
    // 保護：若刪的就是最後一個，直接 pop_back，
    // 避免「自己移動賦值給自己」這種 valid-but-unspecified 的情況
    if (index + 1 != v.size()) {
        v[index] = std::move(v.back());
    }
    v.pop_back();
}

// -----------------------------------------------------------------------------
// 儀器型別：計算元素被賦值（搬移）幾次，作為可重現的複雜度證據。
// -----------------------------------------------------------------------------
struct Item {
    static long assignments;
    int v;
    Item(int x = 0) : v(x) {}
    Item(const Item&) = default;
    Item(Item&&) noexcept = default;
    Item& operator=(const Item& o) { v = o.v; ++assignments; return *this; }
    Item& operator=(Item&& o) noexcept { v = o.v; ++assignments; return *this; }
};
long Item::assignments = 0;

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 27. Remove Element
//   題目：就地移除所有等於 val 的元素，回傳剩餘長度 k。
//   為什麼用到本主題：這題的敘述明確寫著「**元素順序可以改變**」，
//     而且判題器只檢查前 k 個元素的多重集合（不看順序）——
//     這正是 swap-and-pop 的完美適用條件。
//     官方題解稱這個解法為 "two pointers, when elements to remove are rare"，
//     本質就是 swap-and-pop：把尾端元素搬來填洞。
// -----------------------------------------------------------------------------
int removeElementSwapPop(std::vector<int>& nums, int val) {
    std::size_t n = nums.size();
    std::size_t i = 0;
    while (i < n) {
        if (nums[i] == val) {
            nums[i] = nums[n - 1];   // 用最後一個填洞
            --n;                     // 邏輯尾端往前縮（實體不動，O(1)）
            // 注意：這裡刻意「不」遞增 i，因為剛換過來的元素還沒檢查
        } else {
            ++i;
        }
    }
    return static_cast<int>(n);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】遊戲引擎的粒子系統：移除生命耗盡的粒子
//   情境：每一幀有數萬個粒子，生命歸零的要立刻移除。
//         粒子彼此獨立、渲染順序無所謂，順序被打亂完全沒有影響——
//         這是 swap-and-pop 在業界最經典的應用場景。
//         若改用 erase，每幀刪除數千個粒子就是 O(n²)，直接掉幀。
// -----------------------------------------------------------------------------
struct Particle {
    int id;
    float x, y;
    int life;          // 剩餘生命幀數
};

std::size_t updateParticles(std::vector<Particle>& particles) {
    std::size_t removed = 0;
    std::size_t i = 0;
    while (i < particles.size()) {
        --particles[i].life;
        if (particles[i].life <= 0) {
            unstable_erase(particles, i);   // O(1) 移除
            ++removed;
            // 同樣不遞增 i：換過來的粒子這一幀還沒更新過
        } else {
            particles[i].x += 1.0f;
            particles[i].y += 0.5f;
            ++i;
        }
    }
    return removed;
}

int main() {
    std::cout << "=== 1. 基本示範：順序會被打亂 ===\n";
    std::vector<int> v = {10, 20, 30, 40, 50};

    std::cout << "刪除前：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;

    // 刪除索引 1（值 20）
    unstable_erase(v, 1);

    std::cout << "刪除後：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;
    // 注意：順序改變了（50 被移到索引 1 的位置）
    std::cout << "→ 50 被搬到索引 1，其餘元素完全沒有移動\n";

    std::cout << "\n=== 2. 對照組：一般 erase 會保持順序但是 O(n) ===\n";
    std::vector<int> w = {10, 20, 30, 40, 50};
    w.erase(w.begin() + 1);
    std::cout << "erase 後：";
    for (int x : w) std::cout << x << " ";
    std::cout << "（順序保持，但 30/40/50 各搬了一次）\n";

    std::cout << "\n=== 3. 可重現證據：搬移次數（計數，非計時）===\n";
    const int N = 20000;

    // (a) 一般 erase：每次都要平移後段
    Item::assignments = 0;
    {
        std::vector<Item> a;
        a.reserve(N);
        for (int i = 0; i < N; ++i) a.push_back(Item(i));
        // 反覆刪除第一個元素（最壞情況）
        for (int i = 0; i < 5000; ++i) a.erase(a.begin());
    }
    long assignErase = Item::assignments;

    // (b) swap-and-pop：每次只搬一個
    Item::assignments = 0;
    {
        std::vector<Item> b;
        b.reserve(N);
        for (int i = 0; i < N; ++i) b.push_back(Item(i));
        for (int i = 0; i < 5000; ++i) unstable_erase(b, 0);
    }
    long assignSwap = Item::assignments;

    std::cout << "從 " << N << " 個元素中刪除最前面的 5000 個：\n";
    std::cout << "  一般 erase   ：搬移 " << assignErase << " 次\n";
    std::cout << "  swap-and-pop ：搬移 " << assignSwap << " 次\n";
    if (assignSwap > 0) {
        std::cout << "  相差 " << (assignErase / assignSwap) << " 倍"
                  << "（swap-and-pop 每次固定搬 1 個，總數剛好等於刪除次數）\n";
    }

    std::cout << "\n=== 4. 迴圈中批次刪除的正確寫法 ===\n";
    std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::cout << "原始：";
    for (int x : nums) std::cout << x << " ";
    std::cout << "\n刪除所有偶數...\n";

    std::size_t i = 0;
    while (i < nums.size()) {
        if (nums[i] % 2 == 0) {
            unstable_erase(nums, i);
            // ✅ 關鍵：這裡「不」遞增 i，因為換過來的元素還沒檢查
        } else {
            ++i;
        }
    }
    std::cout << "結果：";
    for (int x : nums) std::cout << x << " ";
    std::cout << "(size=" << nums.size() << ")\n";

    // 驗證正確性：確認真的沒有偶數殘留
    bool anyEven = std::any_of(nums.begin(), nums.end(),
                               [](int x) { return x % 2 == 0; });
    std::cout << "還有偶數殘留嗎？ " << std::boolalpha << anyEven
              << "（false 代表刪乾淨了）\n";
    std::cout << "→ 若寫成 for (...; ++i) 每輪都遞增，就會漏刪換過來的元素。\n";

    std::cout << "\n=== 5. 刪除最後一個元素的邊界情況 ===\n";
    std::vector<std::string> s = {"alpha", "beta", "gamma"};
    unstable_erase(s, s.size() - 1);          // 刪最後一個
    std::cout << "刪除最後一個後：";
    for (const auto& x : s) std::cout << x << " ";
    std::cout << "\n（實作已加保護，避免 self-move-assignment 的未指定狀態）\n";

    std::cout << "\n=== LeetCode 27. Remove Element ===\n";
    std::vector<int> lc = {0, 1, 2, 2, 3, 0, 4, 2};
    std::cout << "原始：";
    for (int x : lc) std::cout << x << " ";
    std::cout << "，移除所有 2\n";
    int k = removeElementSwapPop(lc, 2);
    std::cout << "回傳 k = " << k << "，前 k 個：";
    for (int j = 0; j < k; ++j) std::cout << lc[static_cast<std::size_t>(j)] << " ";
    std::cout << "\n（題目明說順序可變、判題器不看順序 → swap-and-pop 完美適用）\n";

    std::cout << "\n=== 日常實務：粒子系統每幀更新 ===\n";
    std::vector<Particle> particles = {
        {1, 0.0f, 0.0f, 3}, {2, 1.0f, 1.0f, 1}, {3, 2.0f, 2.0f, 5},
        {4, 3.0f, 3.0f, 1}, {5, 4.0f, 4.0f, 2}, {6, 5.0f, 5.0f, 1}
    };
    std::cout << "初始粒子數：" << particles.size() << "\n";

    for (int frame = 1; frame <= 3; ++frame) {
        std::size_t dead = updateParticles(particles);
        std::cout << "第 " << frame << " 幀：移除 " << dead
                  << " 個，剩餘 " << particles.size() << " 個 → id: ";
        for (const auto& p : particles) std::cout << p.id << " ";
        std::cout << "\n";
    }
    std::cout << "→ 順序被打亂了，但粒子彼此獨立、渲染順序無所謂，\n";
    std::cout << "  換來的是每次移除都是 O(1)——數萬粒子時這是能否維持幀率的關鍵。\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 20 課：vector 效能分析與最佳實踐10.cpp" -o demo10

// === 預期輸出 ===
// === 1. 基本示範：順序會被打亂 ===
// 刪除前：10 20 30 40 50
// 刪除後：10 50 30 40
// → 50 被搬到索引 1，其餘元素完全沒有移動
//
// === 2. 對照組：一般 erase 會保持順序但是 O(n) ===
// erase 後：10 30 40 50 （順序保持，但 30/40/50 各搬了一次）
//
// === 3. 可重現證據：搬移次數（計數，非計時）===
// 從 20000 個元素中刪除最前面的 5000 個：
//   一般 erase   ：搬移 87497500 次
//   swap-and-pop ：搬移 5000 次
//   相差 17499 倍（swap-and-pop 每次固定搬 1 個，總數剛好等於刪除次數）
//
// === 4. 迴圈中批次刪除的正確寫法 ===
// 原始：1 2 3 4 5 6 7 8 9 10
// 刪除所有偶數...
// 結果：1 9 3 7 5 (size=5)
// 還有偶數殘留嗎？ false（false 代表刪乾淨了）
// → 若寫成 for (...; ++i) 每輪都遞增，就會漏刪換過來的元素。
//
// === 5. 刪除最後一個元素的邊界情況 ===
// 刪除最後一個後：alpha beta
// （實作已加保護，避免 self-move-assignment 的未指定狀態）
//
// === LeetCode 27. Remove Element ===
// 原始：0 1 2 2 3 0 4 2 ，移除所有 2
// 回傳 k = 5，前 k 個：0 1 4 0 3
// （題目明說順序可變、判題器不看順序 → swap-and-pop 完美適用）
//
// === 日常實務：粒子系統每幀更新 ===
// 初始粒子數：6
// 第 1 幀：移除 3 個，剩餘 3 個 → id: 1 5 3
// 第 2 幀：移除 1 個，剩餘 2 個 → id: 1 3
// 第 3 幀：移除 1 個，剩餘 1 個 → id: 3
// → 順序被打亂了，但粒子彼此獨立、渲染順序無所謂，
//   換來的是每次移除都是 O(1)——數萬粒子時這是能否維持幀率的關鍵。
