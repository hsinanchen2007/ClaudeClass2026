// =============================================================================
//  第 14 課：vector 元素插入：insert、emplace7.cpp
//    —  插入成本實測：迴圈中在頭部 insert 的 O(n²) 陷阱
// =============================================================================
//
// 【主題資訊 Information】
//   單次 insert 複雜度: O(distance(pos, end())) 的元素搬移
//                        + 若容量不足另有 O(size()) 的 reallocation
//   標頭檔: <vector>
//   本檔用「元素搬移次數」而非計時來量化成本 —— 計時會受 CPU 排程、
//   turbo、cache 狀態影響而每次不同;搬移次數則是完全確定、可重現的。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「迴圈 + 頭部 insert」是 O(n²)】
// 每次 `v.insert(v.begin(), x)` 都必須把目前所有元素往後搬一格。
// 第 1 次搬 0 個、第 2 次搬 1 個、…、第 N 次搬 N-1 個,總搬移次數是
//     0 + 1 + 2 + … + (N-1) = N(N-1)/2
// 這是標準的等差級數,量級 O(N²)。N = 2000 時約 200 萬次搬移;
// N 放大 10 倍到 20000,搬移次數會放大 100 倍到 2 億次。
// 本檔實測 N = 2000 得到 2001000 次 —— 正好是 N(N-1)/2 = 1999000
// 再加上 N = 2000 次「把暫時物件搬進插入位置」。
//
// 【2. reserve 救不了這個問題】
// 很多人以為先 `v.reserve(N)` 就沒事了。reserve 只消除 **reallocation**,
// 完全不影響 **元素搬移** —— 洞還是得挪出來。本檔的測試全都先 reserve(N),
// 結果仍然是 O(N²)。這是最常見的誤解之一。
//
// 【3. 三種正解】
//   (a) 若順序可以事後再調:改成 push_back(尾端 O(1)) 最後 std::reverse 一次。
//       總成本 O(N) + O(N) = O(N)。
//   (b) 若真的需要頻繁在頭部插入:改用 std::deque(頭尾都 amortized O(1))。
//   (c) 若是「一次要插入很多個」:用 range / fill 版一次插入,
//       只搬移一次後段,而不是每個元素搬一次。
//
// 【4. 順帶看清楚 push_back 與 emplace_back 的差別】
// 本檔第三組測試顯示:push_back(Tracked(i)) 有 N 次搬移(把暫時物件搬進去),
// emplace_back(i) 是 0 次 —— 參數被完美轉發、直接在容器記憶體上建構,
// 從頭到尾沒有第二個物件存在過。這就是就地建構的真正意義。
//
// 【概念補充 Concept Deep Dive】
// 為什麼用「搬移次數」比「毫秒」更適合教學?
//   * 可重現:同樣的程式碼在任何機器上跑都是同一個數字,能直接對照理論公式。
//   * 可歸因:毫秒混合了 cache miss、分支預測、記憶體頻寬等因素,
//     看到「慢 100 倍」也說不清是哪來的。
//   * 但要注意:搬移次數不等於實際耗時。libstdc++ 對 trivially copyable 的
//     型別(如 int)會把整段搬移退化成一次 memmove,由 SIMD 一次處理多個元素,
//     實際耗時遠低於「搬移次數 × 單次成本」的直覺估計。
//     本檔用有自訂 copy/move 的 Tracked 型別,才能逐次計數。
//
// 【注意事項 Pay Attention】
// 1. 下面印出的搬移次數是 **libstdc++ 實測值**。標準只規定複雜度的量級,
//    沒有規定確切的搬移次數,不同實作(libc++、MSVC STL)可能略有差異。
// 2. 本檔刻意不印耗時。原本以 std::chrono 計時的版本每次執行結果都不同,
//    無法作為可驗證的預期輸出。要自行計時請用 -O2 編譯並多跑幾次取中位數。
// 3. O(n²) 在 N 很小(幾十、幾百)時完全無所謂。別為了理論複雜度
//    把可讀性好的程式碼改成難懂的版本 —— 先量測,再優化。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】插入的效能陷阱
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. for (i = 0; i < N; ++i) v.insert(v.begin(), i); 的複雜度是多少?
//     答：O(N²)。第 k 次插入要把既有的 k 個元素往後搬一格,
//         總搬移次數 = 0 + 1 + … + (N-1) = N(N-1)/2。
//         本檔 N = 2000 實測共 2001000 次元素搬移。
//     追問：怎麼改成 O(N)?→ 改用 push_back 累積、最後 std::reverse 一次;
//         或改用 std::deque 的 push_front;或一次用 range insert 插完。
//
// ⚠️ 陷阱. 「先 v.reserve(N) 就能把上面那個迴圈變成 O(N)」—— 對嗎?
//     答：不對。reserve 只避免 reallocation(重新配置 + 搬移全部元素),
//         但完全不影響「為了空出插入位置而搬移後段元素」的成本。
//         本檔三組測試全都先 reserve(N),頭部插入依然是 O(N²)。
//     為什麼會錯：把 vector 的兩種成本混為一談。
//         reallocation 是「容量不夠時換一塊更大的記憶體」;
//         元素搬移是「連續儲存為了插入而必須挪位」。
//         reserve 只解決前者,而頭部插入的瓶頸完全在後者。
//
// 🔥 Q2. push_back(T(x)) 與 emplace_back(x) 在搬移次數上差多少?
//     答：push_back 需要 1 次搬移(把呼叫端建好的暫時物件搬進容器),
//         emplace_back 是 0 次(參數完美轉發、直接在容器記憶體上建構)。
//         本檔 N = 2000 實測:push_back 2000 次、emplace_back 0 次。
//     追問：那對 vector<int> 有差嗎?→ 幾乎沒有。int 的「搬移」就是一次
//         暫存器複製,編譯器最佳化後兩者產生的機器碼通常完全相同。
//         差異只在建構成本高的型別上才看得出來。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <algorithm>

// 會自我計數的元素型別：每次 copy/move 建構或賦值都 +1
struct Tracked {
    int v;
    static long long moves;

    explicit Tracked(int x = 0) : v(x) {}
    Tracked(const Tracked& o) : v(o.v) { ++moves; }
    Tracked(Tracked&& o) noexcept : v(o.v) { ++moves; }
    Tracked& operator=(const Tracked& o) { v = o.v; ++moves; return *this; }
    Tracked& operator=(Tracked&& o) noexcept { v = o.v; ++moves; return *this; }
};
long long Tracked::moves = 0;

int main() {
    const int N = 2000;

    std::cout << "=== 元素搬移次數比較 (N = " << N << ") ===" << std::endl;

    // (1) 在頭部插入：每次都要把既有元素整段往後搬 → O(N²)
    {
        std::vector<Tracked> v;
        v.reserve(N);              // 注意：已經 reserve，仍然是 O(N²)
        Tracked::moves = 0;
        for (int i = 0; i < N; ++i) {
            v.insert(v.begin(), Tracked(i));
        }
        std::cout << "頭部 insert   搬移次數: " << Tracked::moves << std::endl;
    }

    // (2) 在尾端 push_back：不搬移既有元素，只搬入暫時物件 → O(N)
    {
        std::vector<Tracked> v;
        v.reserve(N);
        Tracked::moves = 0;
        for (int i = 0; i < N; ++i) {
            v.push_back(Tracked(i));
        }
        std::cout << "尾端 push_back 搬移次數: " << Tracked::moves << std::endl;
    }

    // (3) 尾端 emplace_back：就地建構，完全沒有搬移
    {
        std::vector<Tracked> v;
        v.reserve(N);
        Tracked::moves = 0;
        for (int i = 0; i < N; ++i) {
            v.emplace_back(i);
        }
        std::cout << "尾端 emplace_back 搬移次數: " << Tracked::moves << std::endl;
    }

    std::cout << "\n理論值 N(N-1)/2 = " << static_cast<long long>(N) * (N - 1) / 2
              << "  (頭部插入還要再加 N 次「把暫時物件搬進插入位置」)" << std::endl;
    std::cout << "以上為 libstdc++ 實測值；標準只規定複雜度量級，未規定確切搬移次數"
              << std::endl;

    // ── 正解示範：需要「反序建立」時，用 push_back + reverse 取代頭部 insert ──
    std::cout << "\n=== 正解：push_back + reverse ===" << std::endl;
    {
        std::vector<Tracked> v;
        v.reserve(N);
        Tracked::moves = 0;
        for (int i = 0; i < N; ++i) {
            v.emplace_back(i);          // 尾端累積：O(N)
        }
        std::reverse(v.begin(), v.end());  // 一次反轉：O(N)
        std::cout << "push_back + reverse 搬移次數: " << Tracked::moves
                  << "  (結果與頭部插入完全相同，但成本從 O(N²) 降為 O(N))"
                  << std::endl;
        std::cout << "驗證首尾: v.front()=" << v.front().v
                  << ", v.back()=" << v.back().v << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 14 課：vector 元素插入：insert、emplace7.cpp" -o insert7

// 以上為 libstdc++ 實測值；標準只規定複雜度量級，未規定確切搬移次數

// === 預期輸出 ===
// === 元素搬移次數比較 (N = 2000) ===
// 頭部 insert   搬移次數: 2001000
// 尾端 push_back 搬移次數: 2000
// 尾端 emplace_back 搬移次數: 0
//
// 理論值 N(N-1)/2 = 1999000  (頭部插入還要再加 N 次「把暫時物件搬進插入位置」)
//
// === 正解：push_back + reverse ===
// push_back + reverse 搬移次數: 3000  (結果與頭部插入完全相同，但成本從 O(N²) 降為 O(N))
// 驗證首尾: v.front()=1999, v.back()=0
