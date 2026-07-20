// =============================================================================
//  02_engines.cpp  —  隨機引擎 Engine
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/numeric/random#Random_number_engines
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ Engine 是什麼？                                            │
//  └────────────────────────────────────────────────────────────┘
//
//  Engine 是一個「狀態機」 — 給定種子產生確定性的 bit 序列。每次呼叫
//  operator() 回傳一個 unsigned 整數（範圍由 Engine 決定）。
//
//  Distribution 不會自己產生 bits — 它「吃 Engine 的輸出」轉成你要的分佈。
//
//      Engine ──(raw bits)──→ Distribution ──(你的目標型別/範圍)──→ 你
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 標準提供的 Engine                                          │
//  └────────────────────────────────────────────────────────────┘
//
//      std::mt19937              32-bit Mersenne Twister，週期 2^19937 - 1
//                                品質高、速度快 — 多數場合首選
//
//      std::mt19937_64           64-bit 版，做大量隨機 64-bit 整數時更直接
//
//      std::minstd_rand          線性同餘，速度極快、狀態小、品質普通
//      std::minstd_rand0         同上，係數較舊版
//      std::ranlux24             高品質、速度慢
//      std::ranlux48
//      std::knuth_b              Knuth shuffle 變體
//
//      std::random_device        硬體 / OS 提供的「不可預測」來源
//                                — 但每次呼叫成本高、不適合熱迴圈
//                                通常只用來「為其他 Engine 產生種子」
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 經驗法則                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//   * 大多數情況：std::mt19937
//   * 需要 64-bit 隨機輸出：std::mt19937_64
//   * 純測試 / 重現：明寫種子（如 mt19937{42}）
//   * 真實場合：用 random_device 種一次（見 07_seeding）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 本檔示範                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：mt19937 產生序列、用同種子重現
//   * Demo 2：mt19937 vs mt19937_64 的範圍
//   * Demo 3：random_device 的使用 — 但成本高
// =============================================================================

/*
補充筆記：std::engines
  - std::engines 使用 <random>；現代 C++ 把亂數引擎和分佈分開，引擎產生位元，分佈把位元轉成需要的機率模型。
  - std::rand 品質與範圍有限，也常被錯誤取模造成偏差；新程式應優先使用 <random>。
  - seed 決定可重現性；測試常用固定 seed，正式隨機可用 random_device 或更完整的 seed sequence。
  - uniform_int_distribution 端點通常包含上下界，uniform_real_distribution 常見語意是 [a,b)；使用前要確認範圍。
  - shuffle 應搭配亂數引擎；不要使用 random_shuffle，因為它已被移除。
  - 加權抽樣、常態分佈、伯努力分佈等應使用對應 distribution，不要手寫容易有偏差的算法。
  - 亂數引擎是 deterministic state machine；同一 seed 會產生同一序列，這對測試很有用。
  - mt19937 狀態大但品質穩定，minstd_rand 較小較快但統計性質不同。
  - 不要在每次產生亂數時重新建立 engine，否則序列品質和效能都會變差。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】隨機引擎（engine）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. <random> 的架構是什麼？engine 與 distribution 如何分工？
//     答：分三層。(1) Seed sequence：std::random_device、std::seed_seq 負責產生種子。
//         (2) Engine（URBG）：從種子產生均勻分布的原始位元序列，如 mt19937、
//         mt19937_64、minstd_rand。(3) Distribution：把 engine 的輸出「塑形」成目標
//         分布，如 uniform_int_distribution、normal_distribution。關鍵分工是：engine
//         只負責產生高品質位元，distribution 負責消除 bias。
//     追問：為什麼不建議用 default_random_engine？（它是實作定義的別名，各家指向不同
//           的 engine，不可攜且品質不保證）
//
// 🔥 Q2. std::random_device 與 std::mt19937 該怎麼搭配？
//     答：random_device 是「非確定性」隨機源（實作通常取自作業系統的熵池），品質高但
//         很慢，而且不可 seed、不可重現。mt19937 是確定性 PRNG，週期 2^19937 − 1，
//         速度極快、統計性質好，但不具密碼學安全性——觀察足夠多的連續輸出即可還原
//         內部狀態。標準搭配是：用 random_device 產生種子，用 mt19937 產生大量數字。
//     追問：密碼學用途怎麼辦？（別用 <random>，改用專門的 CSPRNG，例如作業系統提供的
//           getrandom / /dev/urandom，或 libsodium、OpenSSL）
//
// Q3. mt19937 與 mt19937_64 差在哪？mt19937 有什麼缺點？
//     答：前者輸出 32-bit、狀態 624 個 32-bit word；後者輸出 64-bit、狀態 312 個
//         64-bit word，週期同為 2^19937 − 1。需要 64-bit 隨機值（Zobrist hashing、
//         大範圍整數）時直接用 mt19937_64，不要拼接兩個 32-bit 輸出。mt19937 的缺點：
//         狀態大（約 2.5 KB）對 cache 不友善、非密碼學安全、且未通過部分現代統計測試。
//         替代品有 PCG、xoshiro 系列，但它們不在標準庫內。
//     追問：uniform_int_distribution<int64_t> 配 32-bit 的 mt19937 可以嗎？（可以，
//           distribution 會呼叫 engine 多次組合，只是效率較差）
// ═══════════════════════════════════════════════════════════════════════════

#include <chrono>
#include <iostream>
#include <random>
#include <vector>

// ─────────────────────────────────────────────────────────
// LeetCode 398. Random Pick Index  (難度: medium)
// 題意：給定可能含重複元素的陣列 nums，pick(target) 要在所有等於 target
//       的 index 中等機率選一個回傳。
//
// 思路：reservoir sampling (蓄水池抽樣) — 一次走訪即可。
//   走訪 nums，遇到等於 target 的第 k 個 index 時，以 1/k 機率「換上」當前 index。
//   每個符合條件的位置最終被選中的機率相等 = 1 / count。
//
// 為什麼適合本主題：展示 engine 的 operator()() 直接配合 uniform_int 做隨機決策；
//   不需要先掃一遍找出所有 index 再選 — 空間 O(1)。
//
// 時間：pick O(n)；空間：O(1)。
// ─────────────────────────────────────────────────────────
class PickIndex {
public:
    explicit PickIndex(std::vector<int> nums)
        : nums_(std::move(nums)), rng_(std::random_device{}()) {}

    int pick(int target) {
        int chosen = -1, k = 0;
        for (int i = 0; i < (int)nums_.size(); ++i) {
            if (nums_[i] != target) continue;
            ++k;
            // 用 [0, k) 中抽 0 的機率 = 1/k → 接受當前 i
            std::uniform_int_distribution<int> d{0, k - 1};
            if (d(rng_) == 0) chosen = i;
        }
        return chosen;
    }

private:
    std::vector<int> nums_;
    std::mt19937 rng_;
};

static void demo_lc398_pick_index() {
    std::cout << "[LC398] reservoir-sampled pick(3):\n";
    PickIndex p({1, 2, 3, 3, 3});                    // index 2,3,4 都是 target=3
    int cnt[5] = {0};
    for (int i = 0; i < 30'000; ++i) ++cnt[p.pick(3)];
    for (int i = 2; i <= 4; ++i)
        std::cout << "  idx=" << i << " count=" << cnt[i] << " (期望 ~10000)\n";
}

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：mt19937 重現性
    // ─────────────────────────────────────────────────────────
    {
        std::mt19937 a{42};
        std::mt19937 b{42};
        std::cout << "[Demo1] same seed produces same sequence:\n  a=";
        for (int i = 0; i < 5; ++i) std::cout << ' ' << a();
        std::cout << "\n  b=";
        for (int i = 0; i < 5; ++i) std::cout << ' ' << b();
        std::cout << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // Demo 2：mt19937 vs mt19937_64 範圍
    // ─────────────────────────────────────────────────────────
    {
        std::mt19937 e32{1};
        std::mt19937_64 e64{1};
        std::cout << "[Demo2] mt19937    min=" << std::mt19937::min()
                  << " max=" << std::mt19937::max() << '\n';
        std::cout << "[Demo2] mt19937_64 min=" << std::mt19937_64::min()
                  << " max=" << std::mt19937_64::max() << '\n';
        std::cout << "[Demo2] sample 32: " << e32() << '\n';
        std::cout << "[Demo2] sample 64: " << e64() << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // Demo 3：random_device — 成本高，只用來種子
    // ─────────────────────────────────────────────────────────
    {
        std::random_device rd;
        std::cout << "[Demo3] random_device entropy = " << rd.entropy()
                  << " (0 表示「實作不能保證熵高」)\n";
        std::cout << "[Demo3] one sample: " << rd() << '\n';

        // 把 random_device 拿來「種一次」mt19937，然後用 mt19937 跑萬次
        std::mt19937 rng{rd()};
        long long sum = 0;
        for (int i = 0; i < 1'000'000; ++i) sum += rng() & 0xFF;
        std::cout << "[Demo3] sum-of-low-byte = " << sum << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：random_device 真的「真隨機」嗎？
    //    A：實作可能用硬體 (/dev/urandom、Intel rdrand)，也可能 fallback
    //       到 PRNG。標準允許「entropy() 回傳 0」表示無法保證 — 在某些
    //       MinGW 版本上 random_device 永遠 deterministic！要做密碼學用
    //       絕對要用 OS API（getrandom/BCryptGenRandom）。
    //
    //  Q2：每次都用 mt19937{rd()} OK 嗎？
    //    A：可以，但 mt19937 有 19937-bit 內部狀態，只用 32-bit 種子算「弱
    //       初始化」。需要更好的初始化用 std::seed_seq 配合多個 random_device
    //       輸出（見 07_seeding）。
    //
    //  Q3：thread_local 引擎？
    //    A：常見做法 — 每個 thread 自己一份 mt19937，避免互相鎖：
    //         thread_local std::mt19937 rng{std::random_device{}()};
    //
    demo_lc398_pick_index();
    return 0;
}
