// =============================================================================
//  01_why_not_rand.cpp  —  為什麼不要用 C 的 rand()
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/numeric/random
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ rand() 有什麼問題？                                        │
//  └────────────────────────────────────────────────────────────┘
//
//  1) 品質差
//     * 多數實作週期短（POSIX 要求 RAND_MAX ≥ 32767 — 一些平台僅 15 bits）
//     * 統計性質不佳，做模擬 / 抽樣會偏
//
//  2) 用法易錯：rand() % N 偏差
//
//        int x = rand() % 6 + 1;     // 想要 1..6
//
//     如果 RAND_MAX = 32767、N = 6，32768 / 6 = 5461 餘 2，所以 0..1 比
//     2..5 多被產出一次（多了 5462 次 vs 5461 次）。對 N 是 RAND_MAX 因
//     數時剛好均勻；其他情況都偏差。
//
//  3) 全域狀態
//     rand() 內部用全域 seed。⚠️ 精確講法：POSIX 要求 rand() 必須 thread-safe，
//     glibc 也確實用內部鎖實作（本機 TSan 實測 4 執行緒並行呼叫：乾淨、無 data race）。
//     真正的問題是「共用同一條序列」＋「鎖競爭」，不是 data race；
//     多執行緒要各自獨立且可重現的序列，應每個執行緒各持一個 engine。
//
//  4) 種子能用的範圍小（只有 unsigned int）
//     真正的 cryptographic / Monte Carlo 需要 256-bit 級別的 seed entropy。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ <random> 的解法                                            │
//  └────────────────────────────────────────────────────────────┘
//
//  C++11 把「隨機數」拆成兩件事：
//
//   * Engine    — 產生「均勻分布的 raw bits」（mt19937、minstd_rand 等）
//   * Distribution — 把 raw bits 變成「想要分佈的數」(uniform_int、normal...)
//
//  使用模式：
//
//      std::mt19937 rng{std::random_device{}()};
//      std::uniform_int_distribution<int> die{1, 6};
//      int x = die(rng);
//
//  好處：
//   * Engine 品質高、跨平台
//   * Distribution 對「不可均分」做了正確處理（不用 % N 偏差）
//   * 物件化 — 不是全域，每 thread / 每 component 可有自己的 rng
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 本檔示範                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//   * 統計 rand() % 6 的偏差
//   * 統計 uniform_int_distribution 的均勻度
// =============================================================================

/*
補充筆記：std::why_not_rand
  - std::why_not_rand 使用 <random>；現代 C++ 把亂數引擎和分佈分開，引擎產生位元，分佈把位元轉成需要的機率模型。
  - std::rand 品質與範圍有限，也常被錯誤取模造成偏差；新程式應優先使用 <random>。
  - seed 決定可重現性；測試常用固定 seed，正式隨機可用 random_device 或更完整的 seed sequence。
  - uniform_int_distribution 端點通常包含上下界，uniform_real_distribution 常見語意是 [a,b)；使用前要確認範圍。
  - shuffle 應搭配亂數引擎；不要使用 random_shuffle，因為它已被移除。
  - 加權抽樣、常態分佈、伯努力分佈等應使用對應 distribution，不要手寫容易有偏差的算法。
  - std::rand 的範圍小、品質依實作而異，且常和取模一起造成分布偏差。
  - srand(time(nullptr)) 只提供秒級種子，多次快速啟動可能得到同樣序列。
  - 新程式用 <random> 可以分開控制引擎、分佈和 seed，語意比 rand 清楚。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】為什麼不該用 rand()
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼不該用 std::rand()？
//     答：五個理由。(1) 品質差：多數實作是簡單的 LCG，週期短、低位元的隨機性特別差。
//         (2) RAND_MAX 標準只保證至少 32767，範圍常常不夠用。(3) 配 % n 使用會造成
//         modulo bias。(4) 共用全域序列(POSIX 要求 thread-safe，glibc 以鎖實作，代價是競爭)。(5) 不同實作的序列完全不同，
//         不可重現也不可攜。正解是 <random> 的 engine + distribution 組合。
//     追問：那 rand() 完全不能用嗎？（非安全性、非統計用途的玩具程式可以，但既然
//           <random> 就在標準裡，沒有理由不用）
//
// 🔥 Q2. 什麼是 modulo bias？請用數字解釋
//     答：rand() % n 時，若「可能值的總數」不能被 n 整除，前面幾個餘數就會多出現一次。
//         例如 RAND_MAX 為 32767（共 32768 個值）要取 % 3：32768 = 3 × 10922 + 2，
//         所以餘數 0 與 1 各能由 10923 個原始值產生，餘數 2 只有 10922 個。範圍越大
//         偏差越明顯：同樣 32768 個值取 % 20000，0..12767 的出現機率是其餘的兩倍。
//     追問：怎麼修正？（拒絕採樣：捨棄落在「不完整區塊」的值再取模；或直接用
//           std::uniform_int_distribution，它內部已保證均勻）
//
// ⚠️ 陷阱. srand(time(NULL)) 有什麼問題？
//     答：time() 只有秒級解析度，於是同一秒內啟動的多個程序、或快速重啟的服務，會拿到
//         「完全相同」的隨機序列。這在遊戲伺服器、抽獎系統、測試框架中造成過真實事故。
//         注意改用 <random> 也救不了：std::mt19937 gen(time(nullptr)) 有一模一樣的問題。
//     為什麼會錯：大家把「每次執行都不同」當成隨機性的驗收標準，而單機手動測試時
//         兩次執行往往跨越了秒邊界，所以看起來一切正常，直到批次啟動才爆炸。正解是
//         用 std::random_device 取種子。
// ═══════════════════════════════════════════════════════════════════════════

#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

// ─────────────────────────────────────────────────────────
// LeetCode 470. Implement Rand10() Using Rand7()  (難度: medium)
// 題意：給定一個能等機率回傳 1..7 的 rand7()，請實作 rand10() (回傳 1..10)
//       且保持均勻分布。
//
// 思路：經典「拒絕取樣 (rejection sampling)」
//   呼叫 rand7() 兩次得 row, col ∈ [1,7]，組成 (row-1)*7 + col ∈ [1,49]。
//   只接受落在 [1,40] 的值；落在 [41,49] 就重抽。
//   接受後對 10 取餘 +1 → 得到等機率 1..10。
//
// 為什麼適合本主題：示範「為什麼 rand() % 10 偏差，而拒絕取樣才正確」。
//   雖然我們用 mt19937 + uniform_int 模擬 rand7()，但解法本身完全展現
//   標準 <random> 的真正威力 — 不偏差、可重現。
//
// 時間/空間：期望 O(1)、O(1)。
// ─────────────────────────────────────────────────────────
static int rand7(std::mt19937& rng) {
    std::uniform_int_distribution<int> d{1, 7};
    return d(rng);
}

static int rand10(std::mt19937& rng) {
    while (true) {
        int row = rand7(rng);
        int col = rand7(rng);
        int idx = (row - 1) * 7 + col;               // 1..49
        if (idx <= 40) return ((idx - 1) % 10) + 1;  // 1..10 均勻
        // 41..49 拒絕，重抽
    }
}

static void demo_lc470_rand10() {
    std::cout << "[LC470] rand10 distribution (期望各 ~10000):\n";
    std::mt19937 rng{2026};
    std::vector<int> cnt(11, 0);
    for (int i = 0; i < 100'000; ++i) ++cnt[rand10(rng)];
    for (int v = 1; v <= 10; ++v) std::cout << "  " << v << "=" << cnt[v] << '\n';
}

// ─────────────────────────────────────────────────────────
// 實用範例：simulate_dice — 模擬 N 顆骰子求和的次數分佈
//   工作中（遊戲 / 模擬）常見：「擲 N 顆 6 面骰，求和分佈長什麼樣？」
//   用 uniform_int_distribution 比 rand() 安全且均勻。
// ─────────────────────────────────────────────────────────
static void simulate_dice(int nDice, int trials) {
    std::mt19937 rng{42};
    std::uniform_int_distribution<int> die{1, 6};
    std::vector<int> tally(nDice * 6 + 1, 0);
    for (int t = 0; t < trials; ++t) {
        int sum = 0;
        for (int i = 0; i < nDice; ++i) sum += die(rng);
        ++tally[sum];
    }
    std::cout << "[實用] " << nDice << " 顆骰子 " << trials << " 次模擬，前 5 個總和：\n";
    int shown = 0;
    for (int s = nDice; s < (int)tally.size() && shown < 5; ++s, ++shown)
        std::cout << "  sum=" << s << " count=" << tally[s] << '\n';
}

int main() {
    constexpr int N = 600'000;

    // ─────────────────────────────────────────────────────────
    // Demo 1：rand() % 6 統計
    // ─────────────────────────────────────────────────────────
    std::vector<int> bad(7, 0);   // index 1..6 用
    std::srand(42);
    for (int i = 0; i < N; ++i) {
        ++bad[std::rand() % 6 + 1];
    }
    std::cout << "[rand() % 6] counts (期望各 100000):";
    for (int v = 1; v <= 6; ++v) std::cout << ' ' << bad[v];
    std::cout << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：uniform_int_distribution 統計
    // ─────────────────────────────────────────────────────────
    std::vector<int> good(7, 0);
    std::mt19937 rng{42};
    std::uniform_int_distribution<int> die{1, 6};
    for (int i = 0; i < N; ++i) ++good[die(rng)];
    std::cout << "[uniform_int]   counts (期望各 100000):";
    for (int v = 1; v <= 6; ++v) std::cout << ' ' << good[v];
    std::cout << '\n';

    // 對小範圍偏差肉眼可能不顯著（除非 N 極大或 RAND_MAX 小），但實作正確
    // 性差異是恆真的。

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：rand() 一定會偏差嗎？
    //    A：當 N 不能整除 (RAND_MAX + 1) 時就會偏差。許多教科書範例
    //       (rand() % 6) 都偏；只是偏量小時肉眼不易察覺。
    //
    //  Q2：std 的 distribution 怎麼避免偏差？
    //    A：實作上對「拒絕取樣」(rejection sampling) 做最佳化 — 把不能映
    //       射的「殘餘範圍」捨棄重抽，保證均勻。
    //
    //  Q3：encryption / 安全相關用 mt19937 OK 嗎？
    //    A：不行。mt19937 不是 cryptographic-secure。需要 CSPRNG 用 OS API
    //       (Linux: getrandom、Windows: BCryptGenRandom) 或 OpenSSL 等專用
    //       函式庫。
    //
    demo_lc470_rand10();
    simulate_dice(3, 60'000);
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 01_why_not_rand.cpp -o 01_why_not_rand

// === 預期輸出 ===
// [rand() % 6] counts (期望各 100000): 99986 99434 100661 99990 100184 99745
// [uniform_int]   counts (期望各 100000): 99865 99769 99869 100133 100110 100254
// [LC470] rand10 distribution (期望各 ~10000):
//   1=9935
//   2=10107
//   3=9966
//   4=9843
//   5=9952
//   6=10169
//   7=9936
//   8=10080
//   9=9892
//   10=10120
// [實用] 3 顆骰子 60000 次模擬，前 5 個總和：
//   sum=3 count=247
//   sum=4 count=825
//   sum=5 count=1590
//   sum=6 count=2914
//   sum=7 count=4112
