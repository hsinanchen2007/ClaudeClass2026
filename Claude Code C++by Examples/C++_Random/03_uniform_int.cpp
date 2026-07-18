// =============================================================================
//  03_uniform_int.cpp  —  uniform_int_distribution
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/numeric/random/uniform_int_distribution
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、定義                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//      std::uniform_int_distribution<T> dist{a, b};   // 閉區間 [a, b]
//      T x = dist(engine);
//
//  注意：兩端「都包含」（[a, b]）；浮點數版才是 [a, b)。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、為什麼比 rand() % N 好                                │
//  └────────────────────────────────────────────────────────────┘
//
//   * 內部處理 rejection sampling — 即使 N 不能整除 engine 範圍也均勻
//   * 跨平台保證一致（同 engine + 種子 → 同序列；distribution 在 C++17
//     後實作差異仍可能造成不同實作產生不同序列，但同一實作內穩定）
//   * 表達意圖清楚：[a, b]、含上界
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、distribution 物件可以重複用                            │
//  └────────────────────────────────────────────────────────────┘
//
//  建議把 distribution 物件「保留」 — 它有內部 cache（特別是浮點分佈）；
//  每次重建會丟失 cache、且需要 reinit。
//
//      static thread_local std::mt19937 rng{...};
//      static thread_local std::uniform_int_distribution<int> dist{1, 6};
//      int v = dist(rng);
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：擲骰子 N 次統計
//   * Demo 2：抽樣抽 K 個不重複數（reservoir sampling 的概念）
//   * Demo 3：用 .param() 動態切換範圍（不重建物件）
// =============================================================================

/*
補充筆記：std::uniform_int
  - std::uniform_int 使用 <random>；現代 C++ 把亂數引擎和分佈分開，引擎產生位元，分佈把位元轉成需要的機率模型。
  - std::rand 品質與範圍有限，也常被錯誤取模造成偏差；新程式應優先使用 <random>。
  - seed 決定可重現性；測試常用固定 seed，正式隨機可用 random_device 或更完整的 seed sequence。
  - uniform_int_distribution 端點通常包含上下界，uniform_real_distribution 常見語意是 [a,b)；使用前要確認範圍。
  - shuffle 應搭配亂數引擎；不要使用 random_shuffle，因為它已被移除。
  - 加權抽樣、常態分佈、伯努力分佈等應使用對應 distribution，不要手寫容易有偏差的算法。
  - uniform_int_distribution 的上下界都是 inclusive，這和許多半開區間 API 不同。
  - 不要用 engine() % n 做均勻整數，當 engine 範圍不能被 n 整除時會有 modulo bias。
  - distribution 物件可重用；改範圍時可建立新 distribution 或使用 param_type。
*/
#include <algorithm>
#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>

// ─────────────────────────────────────────────────────────
// LeetCode 380. Insert Delete GetRandom O(1)  (難度: medium)
// 題意：設計資料結構 RandomizedSet，支援 insert/remove/getRandom 都是 O(1)。
//
// 思路：vector + unordered_map
//   - vector<int> nums 存所有元素 (連續, 方便 O(1) 隨機抽)
//   - unordered_map<int,int> pos 存「值 → 在 vector 的 index」
//   - insert：push_back 並記 pos
//   - remove：把要刪的元素跟「最後一個元素」對調，pop_back；更新 pos
//   - getRandom：uniform_int_distribution{0, size-1} 抽一個 index
//
// 為什麼適合本主題：getRandom 完美示範「對動態變化的範圍」用 uniform_int
//   來抽。範圍每次都不同 — 我們用 .param() 切換，避免重建 distribution。
// ─────────────────────────────────────────────────────────
class RandomizedSet {
public:
    RandomizedSet() : rng_(std::random_device{}()) {}

    bool insert(int val) {
        if (pos_.count(val)) return false;
        pos_[val] = (int)nums_.size();
        nums_.push_back(val);
        return true;
    }

    bool remove(int val) {
        auto it = pos_.find(val);
        if (it == pos_.end()) return false;
        int idx = it->second;
        int last = nums_.back();
        nums_[idx] = last;                            // 把最後一個搬過來
        pos_[last] = idx;
        nums_.pop_back();
        pos_.erase(it);
        return true;
    }

    int getRandom() {
        // 範圍是 [0, size-1]，每次 size 可能不同 → 用 .param() 切換
        dist_.param(decltype(dist_)::param_type{0, (int)nums_.size() - 1});
        return nums_[dist_(rng_)];
    }

private:
    std::vector<int> nums_;
    std::unordered_map<int, int> pos_;
    std::mt19937 rng_;
    std::uniform_int_distribution<int> dist_;
};

static void demo_lc380_randomized_set() {
    std::cout << "[LC380] RandomizedSet:\n";
    RandomizedSet s;
    s.insert(1); s.insert(2); s.insert(3);
    int c1 = 0, c2 = 0, c3 = 0;
    for (int i = 0; i < 30'000; ++i) {
        int v = s.getRandom();
        if (v == 1) ++c1; else if (v == 2) ++c2; else ++c3;
    }
    std::cout << "  insert 3 元素後 getRandom 30000 次：1=" << c1
              << " 2=" << c2 << " 3=" << c3 << " (期望各 ~10000)\n";
    s.remove(2);
    std::cout << "  remove(2) 後 size=" << 2 << "，下次 getRandom = " << s.getRandom() << '\n';
}

int main() {
    std::mt19937 rng{42};

    // ─────────────────────────────────────────────────────────
    // Demo 1：1..6 骰子 60000 次
    // ─────────────────────────────────────────────────────────
    {
        std::uniform_int_distribution<int> die{1, 6};
        std::vector<int> count(7, 0);
        for (int i = 0; i < 60'000; ++i) ++count[die(rng)];
        std::cout << "[Demo1] dice counts (期望 ~10000):";
        for (int v = 1; v <= 6; ++v) std::cout << ' ' << count[v];
        std::cout << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // Demo 2：從 0..N-1 抽 5 個不重複（用「打亂後取前 K」的 trick 思維；
    //         真正想做要看 07 與 08）
    // ─────────────────────────────────────────────────────────
    {
        constexpr int N = 20;
        std::vector<int> pool(N);
        for (int i = 0; i < N; ++i) pool[i] = i;
        std::shuffle(pool.begin(), pool.end(), rng);
        std::cout << "[Demo2] pick 5 from 0.." << (N-1) << ':';
        for (int i = 0; i < 5; ++i) std::cout << ' ' << pool[i];
        std::cout << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // Demo 3：.param() 切換範圍 — 不重建物件
    // ─────────────────────────────────────────────────────────
    {
        std::uniform_int_distribution<int> dist;     // 預設 [0, INT_MAX]
        dist.param(decltype(dist)::param_type{1, 100});
        std::cout << "[Demo3] [1,100]: " << dist(rng) << ' ' << dist(rng) << '\n';

        dist.param(decltype(dist)::param_type{-50, 50});
        std::cout << "[Demo3] [-50,50]: " << dist(rng) << ' ' << dist(rng) << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼是閉區間 [a, b]？
    //    A：標準明文：integer 版是 [a, b]；real 版是 [a, b)。整數結尾 b
    //       很常被需要（最大值就是 b），所以閉區間更直覺。
    //
    //  Q2：可以對 char、long long 一起用嗎？
    //    A：可以。template 參數 T 必須是 integer type；可以是 unsigned。
    //       注意 char 在標準上不算 integer type — 用 short / int 包一下。
    //
    //  Q3：重抽到死循環會發生嗎？
    //    A：不會。標準保證「在 finite expected time 內」回傳。Rejection
    //       sampling 的拒絕率隨 distribution 範圍 vs engine 範圍接近 1
    //       時趨近 0。
    //
    demo_lc380_randomized_set();
    return 0;
}
