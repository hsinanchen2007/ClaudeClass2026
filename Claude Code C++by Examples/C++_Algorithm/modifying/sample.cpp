// ============================================================
// std::sample   (C++17 起)
// 分類 (Category): Modifying sequence operations (修改型)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/sample
//   * https://cplusplus.com/reference/algorithm/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::sample 解的問題:
//
//   「從一個母體 [first, last) 中,均勻、無放回地抽取最多 n 個元素。」
//
// 「無放回」(without replacement) 是抽樣理論的關鍵概念:
//   * 有放回:每次抽完放回去,可能抽到同一個元素多次。
//   * 無放回:抽完不放回,每個元素最多被抽中一次。
//
// std::sample 是「無放回」抽樣 — 想要「有放回」抽樣請改用
// std::uniform_int_distribution 自己抽索引。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼是「Reservoir Sampling」?                       │
// └────────────────────────────────────────────────────────────┘
//
// std::sample 對 InputIterator 來源使用「水庫抽樣 (Reservoir Sampling)」 —
// 一種「不需事先知道母體大小」的演算法,O(N) 時間、O(n) 空間,
// 對串流資料 (例如從 socket 一筆一筆讀進來) 也能均勻取樣。
//
// 這就是它的厲害之處 — 即使你只能「逐個讀取」資料,也能用一次掃描
// 就拿到均勻樣本,不必先把全部讀進記憶體再隨機選。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、輸出順序的保證                                         │
// └────────────────────────────────────────────────────────────┘
//
// 標準對輸出順序有兩個情境:
//
//   * 來源是 ForwardIterator → 輸出元素「保留原相對順序」(穩定)
//   * 來源是 InputIterator    → 輸出順序「未指定」
//
// 對 vector / array 等支援 ForwardIterator 的容器,輸出會穩定,
// 不會把原本的順序打亂。要打亂順序請另外呼叫 std::shuffle。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class PopulationIt, class SampleIt,
//             class Distance, class URBG>
//   SampleIt sample(PopulationIt first, PopulationIt last,
//                   SampleIt out, Distance n, URBG&& g);
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、參數與回傳值                                           │
// └────────────────────────────────────────────────────────────┘
//
//   first, last : 母體範圍
//   out         : 輸出迭代器 (可用 back_inserter 動態擴充)
//   n           : 想抽取的個數;若 n > 母體大小,實際會抽出母體個數
//   g           : URBG (Uniform Random Bit Generator),例 std::mt19937
//
//   回傳: 寫入結束位置的下一個 (out + min(n, distance(first, last)))
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間: O(N) — 一次掃描母體
//   空間: O(n) — 抽出的元素
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 「無放回」 — 要有放回請自己用 uniform_int_distribution。
//   2. 若 n 超過母體大小,實際只抽出母體個數,不會錯誤也不會重複。
//   3. 使用前要建立 PRNG (例如 std::mt19937),固定 seed 可重現結果。
//   4. 對應的「打亂順序」是 std::shuffle,別搞混。
//
// ============================================================

/*
補充筆記：std::sample
  - sample 從輸入範圍隨機抽取 n 個元素，輸出到另一個範圍。
  - 它需要隨機引擎；不要用 rand 取代標準引擎。
  - 抽樣結果數量不會超過輸入大小，輸出 iterator 要能接收結果。
  - std::sample 會改變目標範圍內容或元素順序；呼叫前要確認輸出範圍空間足夠、iterator 有效。
  - copy/transform/generate/fill 寫入目的地時不會自動擴容，除非你使用 back_inserter 這類 insert iterator。
  - remove/unique 不會真的縮短容器，只把保留元素移到前面並回傳新邏輯結尾；還要搭配 erase 才會改變 size。
  - reverse/rotate/swap_ranges 會改變元素位置；若外部保存索引或 iterator，要重新確認是否仍指向預期元素。
  - move algorithm 會把元素搬到目的地，來源仍有效但值可能改變；後續只能重新指定或安全銷毀。
  - shuffle/sample 需要亂數引擎；不要每次呼叫都用同一個固定種子，除非你刻意要可重現測試結果。
*/

// ===========================================================================
// 【面試題】std::sample(C++17)
// ---------------------------------------------------------------------------
// 🔥 Q1. std::sample 是哪個標準加入的?解決什麼問題?
//     答:C++17。從母體 [first, last) 中「均勻、無放回(without replacement)」地抽出
//         最多 n 個元素寫到輸出。無放回代表每個元素最多被抽中一次;要「有放回」抽樣
//         得自己用 std::uniform_int_distribution 抽索引。
//         回傳值是輸出端寫入結束的下一個位置。
//
// 🔥 Q2. 輸出元素的順序有保證嗎?
//     答:看來源的 iterator 類別。來源滿足 ForwardIterator 時,被選中的元素會「保留原本
//         的相對順序」(穩定);來源只是 InputIterator 時,輸出順序未指定。
//         所以 sample 不等於 shuffle——想同時打亂,得抽完再呼叫一次 std::shuffle。
//     追問:那來源只有 InputIterator 時,對輸出 iterator 有額外要求嗎?
//         (答:有,此時輸出必須是 RandomAccessIterator)
//
// Q3. 為什麼 reservoir sampling 很重要?
//     答:因為它不需要事先知道母體大小,一次掃描、O(n) 額外空間就能得到均勻樣本
//         (n 為取樣個數)。對「只能逐筆讀取」的串流資料(socket、檔案流),
//         不必先把全部載入記憶體再隨機挑,這是它最有價值的場景。
//
// ⚠️ 陷阱. n 傳得比母體還大會發生什麼事?
//     答:不是錯誤,也不會重複抽——實際只會抽出母體個數,也就是
//         min(n, distance(first, last)) 個。所以千萬別假設「回傳位置 == out + n」,
//         要用回傳的 iterator 判斷實際寫了幾個。
//     為什麼會錯:大家習慣其他語言的抽樣函式在 n 過大時丟例外,於是不去檢查回傳值。
// ===========================================================================

#include <algorithm>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> pop{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::mt19937 rng(123);

    // --- 範例 1: 抽 4 個 (順序保留,因為 vector 是 ForwardIterator) ---
    std::vector<int> picked;
    std::sample(pop.begin(), pop.end(),
                std::back_inserter(picked), 4, rng);
    std::cout << "sampled 4: ";
    for (int x : picked) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 2: 抽 0 個 → 空 ---
    std::vector<int> none;
    std::sample(pop.begin(), pop.end(),
                std::back_inserter(none), 0, rng);
    std::cout << "sample n=0 size = " << none.size() << '\n';

    // --- 範例 3: n 大於母體 → 抽出全部 (不會錯誤) ---
    std::vector<int> all;
    std::sample(pop.begin(), pop.end(),
                std::back_inserter(all), 100, rng);
    std::cout << "sample n=100 size = " << all.size() << '\n';

    // --- 範例 4: 字元序列抽樣 ---
    std::string alpha = "abcdefghijklmnopqrstuvwxyz";
    std::string out;
    std::sample(alpha.begin(), alpha.end(),
                std::back_inserter(out), 5, rng);
    std::cout << "5 random letters: " << out << '\n';

    // === 實務範例 ===
    void practical_ab_test_sampling();
    void practical_string_sketch();
    void leetcode_398_random_pick_index_concept();
    void practical_lottery_draw();
    practical_ab_test_sampling();
    practical_string_sketch();
    leetcode_398_random_pick_index_concept();
    practical_lottery_draw();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================
//
// std::sample 在 LeetCode 上題目少見,但實務上極度有用 (A/B 測試、
// 隨機抽查、資料 sketch ...)。

// ----------------------------------------------------------------
// 實務範例 1:A/B 測試 — 從用戶池中無放回抽 K 個
// ----------------------------------------------------------------
// 場景:後端有 N 個用戶 ID,要均勻、無放回地抽 K 個進入 A 組。
//
// 為什麼用 std::sample:
//   * 「無放回」「均勻」「O(N) 一次完成」 — 完全對應實務需求。
//   * vector 是 ForwardIterator,輸出順序保留,易於後續處理。
void practical_ab_test_sampling() {
    std::vector<int> users;
    for (int i = 1; i <= 20; ++i) users.push_back(i);
    std::mt19937 rng(42);
    std::vector<int> group_a;
    std::sample(users.begin(), users.end(),
                std::back_inserter(group_a), 5, rng);
    std::cout << "A/B test group A (size=" << group_a.size() << "): ";
    for (int u : group_a) std::cout << u << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例 2:大字串「字元抽樣」做 sketch
// ----------------------------------------------------------------
// 場景:對大型輸入做摘要 (sketch),常用「均勻抽樣 K 個位置」當代表,
//      用於後續比對相似度等。
//
// 為什麼用 std::sample:
//   * 母體可能很大,無需先全載進記憶體 — Reservoir 演算法很省。
//   * 輸出是「均勻無偏」的代表性樣本。
void practical_string_sketch() {
    std::string corpus = "the quick brown fox jumps over the lazy dog";
    std::mt19937 rng(7);
    std::string sketch;
    std::sample(corpus.begin(), corpus.end(),
                std::back_inserter(sketch), 8, rng);
    std::cout << "Sketch (8 chars, size=" << sketch.size() << "): "
              << sketch << '\n';
}

// ----------------------------------------------------------------
// LeetCode 398 概念:隨機從 target 索引中挑一個 (Random Pick Index)
// 難度: medium
// ----------------------------------------------------------------
// 題目簡化:給陣列 nums 與 target,在所有「nums[i] == target」的索引中,
//          均勻隨機回傳一個。
//
// 為什麼用 std::sample (n = 1):
//   先收集所有 target 索引,再用 sample 抽 1 個。
//   或用 Reservoir 在 O(n) 直接 sample 1 個 (LC 398 最佳解)。
//   這裡示範前者,展示 sample 的 n = 1 應用。
//
// 複雜度:時間 O(n);空間 O(k) 收集索引。
void leetcode_398_random_pick_index_concept() {
    std::vector<int> nums{1, 2, 3, 3, 3};
    int target = 3;
    std::vector<int> idx;
    for (size_t i = 0; i < nums.size(); ++i)
        if (nums[i] == target) idx.push_back(i);
    std::mt19937 rng(777);
    std::vector<int> picked;
    std::sample(idx.begin(), idx.end(), std::back_inserter(picked), 1, rng);
    std::cout << "LC398 picked index=" << picked.front() << '\n';
}

// ----------------------------------------------------------------
// 實務範例:從會員清單抽出抽獎中獎者
// ----------------------------------------------------------------
// 場景:活動結束抽 3 名得獎者,要求「無放回、機率均等」。
//      std::sample 一行解決,seed 可記錄供事後驗證公平性。
void practical_lottery_draw() {
    std::vector<std::string> members{
        "Alice", "Bob", "Cathy", "David", "Eve", "Frank", "Grace", "Henry"
    };
    std::mt19937 rng(2026);
    std::vector<std::string> winners;
    std::sample(members.begin(), members.end(),
                std::back_inserter(winners), 3, rng);
    std::cout << "winners (n=" << winners.size() << "):";
    for (auto& w : winners) std::cout << " " << w;
    std::cout << '\n';
}

// === 預期輸出 (Expected output) ===
// sampled 4: ...   (mt19937(123) 對 1..10 的抽樣,實際數字隨實作)
// sample n=0 size = 0
// sample n=100 size = 10
// 5 random letters: ...
// A/B test group A (size=5): ...
// Sketch (8 chars, size=8): ...
// LC398 picked index= ...  (隨機;落在 2/3/4 之一)
// winners (n=3): ...        (mt19937(2026) 的固定結果)
