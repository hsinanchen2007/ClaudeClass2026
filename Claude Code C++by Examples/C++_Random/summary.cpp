/*
================================================================================
【C++_Random/summary.cpp】

本目錄主題：<random>（不要再用 rand()）

<random> 的設計是「引擎 engine + 分佈 distribution」：
  - engine：產生均勻的亂數位元（例如 mt19937）
  - distribution：把引擎輸出映射成你要的數學分佈（uniform_int, normal...）

重點：
  - rand() 品質/範圍/可移植性很差，且難以正確產生某範圍均勻分佈
  - 使用 std::random_device 做 seed（或用固定 seed 做可重現測試）
  - 分佈物件應該重用（保持狀態/避免重建成本）

本 summary 原則：
  - 不加入 題庫 類範例
  - C++17 可編譯

編譯：
  g++ -std=c++17 -Wall -Wextra summary.cpp -o summary && ./summary
================================================================================
*/

/*
補充筆記：C++_Random/C++_Random summary
  - 如果兩個範例看起來都能完成同一件事，優先比較它們是否擁有資料、是否配置記憶體、是否改變輸入。
  - C++_Random/C++_Random summary 使用 <random>；現代 C++ 把亂數引擎和分佈分開，引擎產生位元，分佈把位元轉成需要的機率模型。
  - std::rand 品質與範圍有限，也常被錯誤取模造成偏差；新程式應優先使用 <random>。
  - seed 決定可重現性；測試常用固定 seed，正式隨機可用 random_device 或更完整的 seed sequence。
  - uniform_int_distribution 端點通常包含上下界，uniform_real_distribution 常見語意是 [a,b)；使用前要確認範圍。
  - shuffle 應搭配亂數引擎；不要使用 random_shuffle，因為它已被移除。
  - 加權抽樣、常態分佈、伯努力分佈等應使用對應 distribution，不要手寫容易有偏差的算法。
  - 這個 summary.cpp 只做章節整理，不新增題庫題解；需要實作練習時回到各主題檔。
  - C++_Random/C++_Random summary 的複習方式是把 API 依用途分組，再比較輸入條件、輸出語意、失敗狀態和複雜度。
  - 初學複習 summary 時，不要只背函式名稱；要能說出何時該用、何時不該用、和相近工具差在哪裡。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】<random> 總複習
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 用一句話說明 <random> 的正確使用姿勢
//     答：用 std::random_device 取種子（必要時用 seed_seq 餵多個熵值），用 mt19937 之類
//         的 engine 產生位元，用 distribution 塑形；engine 與 distribution 都只建立
//         一次（static 或 thread_local）重複使用，不要放在函式裡每次重建。多執行緒
//         一律用 thread_local，因為 operator() 會修改狀態、共用即 data race。
//     追問：兩個最常考的區間陷阱？（uniform_int_distribution 是閉區間 [a, b]，
//           uniform_real_distribution 是半開區間 [a, b)）
//
// 🔥 Q2. 什麼是 Reservoir Sampling（水塘抽樣）？
//     答：從「長度未知或無法全部載入記憶體」的資料流中等機率抽取 k 個樣本。k = 1 的
//         版本：走訪到第 i 個元素時，以 1/i 的機率用它取代目前保留的樣本。可用歸納法
//         證明每個元素最終被選中的機率都是 1/n。空間 O(k)、單次掃描 O(n)。
//     追問：k > 1 怎麼做？（前 k 個直接放入水塘；第 i 個（i > k）以 k/i 的機率隨機
//           替換掉水塘中的其中一個）
//
// ⚠️ 陷阱. 「我印出來看起來很亂，所以我的隨機演算法沒問題」——錯在哪？
//     答：肉眼看不出統計偏差。modulo bias、寫錯範圍的 Fisher-Yates、重建 engine 導致
//         的重複序列，輸出通常都「看起來很亂」。驗收隨機演算法必須靠統計：對小規模
//         輸入跑上百萬次，統計各結果的頻率是否符合理論分布，必要時做卡方檢定。
//     為什麼會錯：把「無法預測下一個」與「機率均勻」混為一談。前者是感受，後者是
//         可量化、可檢驗的數學性質，兩者完全不同。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

static void demo_seed_and_engine() {
    std::cout << "\n[demo_seed_and_engine]\n";

    // (1) 固定 seed：結果可重現（適合測試/除錯）
    std::mt19937 rng1(12345);
    std::cout << "  mt19937(12345) first=" << rng1() << "\n";

    // (2) random_device：嘗試提供「非決定性」seed（不保證真硬體亂數）
    std::random_device rd;
    std::mt19937 rng2(rd());
    std::cout << "  mt19937(rd()) first=" << rng2() << "\n";
}

static void demo_uniform_int_and_real() {
    std::cout << "\n[demo_uniform_int_and_real]\n";
    std::mt19937 rng(42);

    std::uniform_int_distribution<int> dist_i(1, 6);   // 骰子
    std::uniform_real_distribution<double> dist_d(0.0, 1.0);

    std::cout << "  dice:";
    for (int k = 0; k < 10; ++k) std::cout << ' ' << dist_i(rng);
    std::cout << "\n";

    std::cout << "  uniform[0,1):";
    for (int k = 0; k < 5; ++k) std::cout << ' ' << dist_d(rng);
    std::cout << "\n";
}

static void demo_normal_distribution() {
    std::cout << "\n[demo_normal_distribution]\n";
    std::mt19937 rng(7);
    std::normal_distribution<double> norm(0.0, 1.0); // mean=0, stddev=1

    std::cout << "  N(0,1) samples:";
    for (int i = 0; i < 5; ++i) std::cout << ' ' << norm(rng);
    std::cout << "\n";
}

static void demo_shuffle() {
    std::cout << "\n[demo_shuffle]\n";
    std::mt19937 rng(1);
    std::vector<int> v(10);
    std::iota(v.begin(), v.end(), 1);

    std::shuffle(v.begin(), v.end(), rng);
    std::cout << "  shuffled:";
    for (int x : v) std::cout << ' ' << x;
    std::cout << "\n";
}

static void demo_why_not_rand_note() {
    std::cout << "\n[demo_why_not_rand_note]\n";
    std::cout << "  rand() 常見問題：範圍小、品質差、共用全域序列(鎖競爭)、取模偏差。\n";
    std::cout << "  要範圍 [a,b] 均勻：請用 uniform_int_distribution。\n";
}

int main() {
    demo_seed_and_engine();
    demo_uniform_int_and_real();
    demo_normal_distribution();
    demo_shuffle();
    demo_why_not_rand_note();

    std::cout << "\n[done]\n";
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
//
// [demo_seed_and_engine]
//   mt19937(12345) first=3992670690
//   mt19937(rd()) first=2778428117
//
// [demo_uniform_int_and_real]
//   dice: 3 5 6 2 5 5 4 4 1 3
//   uniform[0,1): 0.0999749 0.459249 0.333709 0.142867 0.650888
//
// [demo_normal_distribution]
//   N(0,1) samples: -0.720144 -1.08467 -0.0371002 0.399463 -1.09344
//
// [demo_shuffle]
//   shuffled: 10 1 3 6 8 5 7 4 2 9
//
// [demo_why_not_rand_note]
//   rand() 常見問題：範圍小、品質差、共用全域序列(鎖競爭)、取模偏差。
//   要範圍 [a,b] 均勻：請用 uniform_int_distribution。
//
// [done]
