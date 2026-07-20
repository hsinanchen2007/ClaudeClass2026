// ============================================================
// std::rotate / std::rotate_copy
// 分類 (Category): Modifying sequence operations (修改型)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/rotate
//   * https://en.cppreference.com/w/cpp/algorithm/rotate_copy
//   * https://cplusplus.com/reference/algorithm/rotate/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::rotate 解的問題:
//
//   「指定 [first, n_first, last)。把 [n_first, last) 那段搬到最前面,
//    把 [first, n_first) 那段接在後面 — 整體像個圓環地旋轉一格。」
//
// 直觀理解:n_first 變成新的「第一個」元素。
//
// 範例 (n=5,n_first 在 index 2):
//
//   原:    [A, B, C, D, E]
//                  ↑ n_first
//   結果:  [C, D, E, A, B]
//
// 「向左移動 k 格」就是 rotate(begin, begin + k, end)。
// 「向右移動 k 格」就是 rotate(begin, end - k, end)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼 rotate 有用?                                    │
// └────────────────────────────────────────────────────────────┘
//
//   * 圓形緩衝區 (ring buffer):重新對齊頭部位置。
//   * 演算法的「組合元件」:next_permutation、insertion sort 部分都用得上。
//   * 任務佇列「優先處理某項」:把它 rotate 到頭部。
//   * 字串/陣列旋轉題:LC 189 等。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、rotate 與「三次反轉法」的等價性                        │
// └────────────────────────────────────────────────────────────┘
//
// std::rotate(begin, mid, end) 在效果上等於以下三次 reverse:
//
//   reverse(begin, mid);
//   reverse(mid,   end);
//   reverse(begin, end);
//
// (亦等價於以「反向順序」做這三次 reverse — 視 mid 位置而異)
// 這個性質讓你在沒有 std::rotate 的環境也能輕鬆實作旋轉。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class FwdIt>
//   FwdIt rotate(FwdIt first, FwdIt n_first, FwdIt last);
//
//   template <class FwdIt, class OutputIt>
//   OutputIt rotate_copy(FwdIt first, FwdIt n_first, FwdIt last,
//                        OutputIt d_first);
//
//   * C++11 起 rotate 才開始有回傳值 (之前是 void)。
//   * C++20 起為 constexpr。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、回傳值 (Return value)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   rotate      : 指向「原 first 元素旋轉後的新位置」 (即 first + (last - n_first))
//   rotate_copy : 寫入結束位置的下一個
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間: O(N) — 多數實作為 N - gcd(N, k) 次 swap
//   空間: O(1) (in-place 版)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. n_first 必須在 [first, last] 範圍內;若等於 first 或 last,什麼都不做。
//   2. 「向右 k」要先 k %= n,避免越界。
//   3. 對 std::list,有專屬旋轉技巧 (splice),效率更好。
//   4. 想保留原資料就用 rotate_copy。
//
// ============================================================

/*
補充筆記：std::rotate
  - rotate 把 middle 位置移到 range 開頭，形成循環位移。
  - 回傳值是原 first 元素旋轉後的新位置，這常被初學者忽略。
  - 它適合實作左旋陣列、把子區間搬到前面等操作。
  - std::rotate 會改變目標範圍內容或元素順序；呼叫前要確認輸出範圍空間足夠、iterator 有效。
  - copy/transform/generate/fill 寫入目的地時不會自動擴容，除非你使用 back_inserter 這類 insert iterator。
  - remove/unique 不會真的縮短容器，只把保留元素移到前面並回傳新邏輯結尾；還要搭配 erase 才會改變 size。
  - reverse/rotate/swap_ranges 會改變元素位置；若外部保存索引或 iterator，要重新確認是否仍指向預期元素。
  - move algorithm 會把元素搬到目的地，來源仍有效但值可能改變；後續只能重新指定或安全銷毀。
  - shuffle/sample 需要亂數引擎；不要每次呼叫都用同一個固定種子，除非你刻意要可重現測試結果。
*/

// ===========================================================================
// 【面試題】std::rotate / rotate_copy
// ---------------------------------------------------------------------------
// 🔥 Q1. std::rotate(first, middle, last) 做什麼?複雜度?
//     答:把 [first, middle) 與 [middle, last) 兩段交換位置(左旋),使原本 middle 指向的
//         元素成為新的起點。是 in-place 的線性演算法 O(n),不需要額外緩衝區。
//         回傳值是「原本 first 那個元素的新位置」,等於 first + (last - middle)。
//         注意回傳型別:C++98 是 void,C++11 起才改為回傳 iterator。
//     追問:middle 等於 first 或 last 時會怎樣?(答:等於什麼都不做)
//
// 🔥 Q2. rotate 和三次 reverse 是什麼關係?
//     答:rotate 可以用三次 reverse 實作:reverse 前段、reverse 後段、再 reverse 全體。
//         這是筆試常考的「陣列循環移位」標準解,同樣是 O(n) 且 in-place,
//         但實際的搬移次數比 std::rotate 的專門實作多。
//
// Q3. rotate 的經典用途有哪些?
//     答:陣列循環移位、把某個元素或某段子區間搬到指定位置、實作 stable_partition
//         的核心步驟。Sean Parent 在 "C++ Seasoning" 講「no raw loops」時,
//         就是以 rotate 與 stable_partition 當主要範例。
//
// ⚠️ 陷阱. 想「向右旋轉 k 格」,middle 該傳什麼?
//     答:std::rotate 是左旋,向右旋 k 等於向左旋 n - k,所以要傳
//         v.begin() + (n - k % n) % n。先對 k 取模是必要的,否則 k >= n 時 iterator 越界。
//     為什麼會錯:直覺會直接寫 v.begin() + k,那是向左旋 k;而且忘了取模在
//         k > size() 時就直接 UB,不會有任何錯誤訊息。
// ===========================================================================

#include <algorithm>
#include <climits>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: 向左旋轉 2 格 ---
    std::vector<int> v{1, 2, 3, 4, 5};
    auto it = std::rotate(v.begin(), v.begin() + 2, v.end());
    std::cout << "rotate left 2: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << "(原 first 現在在 index "
              << (it - v.begin()) << ")\n";

    // --- 範例 2: 向右旋轉 1 格 (用 last - 1 作為新起點) ---
    std::vector<int> w{1, 2, 3, 4, 5};
    std::rotate(w.begin(), w.end() - 1, w.end());
    std::cout << "rotate right 1: ";
    for (int x : w) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 3: rotate_copy (原容器不變) ---
    std::vector<int> src{10, 20, 30, 40, 50};
    std::vector<int> dst;
    std::rotate_copy(src.begin(), src.begin() + 3, src.end(),
                     std::back_inserter(dst));
    std::cout << "src: ";
    for (int x : src) std::cout << x << ' ';
    std::cout << '\n';
    std::cout << "rotate_copy: ";
    for (int x : dst) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 4: 邊界 — n_first == first 表示不動 ---
    std::vector<int> a{1, 2, 3};
    std::rotate(a.begin(), a.begin(), a.end());
    std::cout << "no-op rotate: ";
    for (int x : a) std::cout << x << ' ';
    std::cout << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_189_rotate_array();
    void practical_promote_task_to_front();
    void leetcode_396_rotate_function_concept();
    void practical_round_robin_scheduler();
    leetcode_189_rotate_array();
    practical_promote_task_to_front();
    leetcode_396_rotate_function_concept();
    practical_round_robin_scheduler();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 189: 旋轉陣列 (Rotate Array)
// ----------------------------------------------------------------
// 題目:給陣列 nums 與 k,將 nums 向右旋轉 k 步 (in-place,O(1) 額外空間)。
//      例如 [1,2,3,4,5,6,7], k=3 → [5,6,7,1,2,3,4]。
//
// 為什麼用 std::rotate:
//   題目就是「圓環旋轉」 — std::rotate 一行解決。
//   選 mid = end - (k % n),即「最後 k 個元素變成新開頭」。
//
// 解法步驟:
//   1. k %= n  (處理 k > n 的情況,避免越界)。
//   2. std::rotate(begin, end - k, end)。
//
// 複雜度:時間 O(n),空間 O(1)。
void leetcode_189_rotate_array() {
    std::vector<int> nums{1,2,3,4,5,6,7};
    int k = 3;
    int n = static_cast<int>(nums.size());
    k %= n;
    if (k > 0) {
        std::rotate(nums.begin(), nums.end() - k, nums.end());
    }
    std::cout << "LC189: ";
    for (int x : nums) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:任務佇列 — 把「VIP 任務」搬到隊頭
// ----------------------------------------------------------------
// 場景:任務佇列中第 i 個任務被標記為高優先級,要立刻搬到隊頭,
//      但其他任務的「相對順序」要保留 (不是「對換到隊頭」,是「插隊到隊頭」)。
//
// 為什麼用 std::rotate:
//   * 把 [v[i], v[i+1]) (一個元素) 旋轉到 begin 的位置,
//     原本 [begin, v[i]) 那段會接到後面 — 正好就是「插隊到隊頭」。
//   * 一行寫完,O(n),不必自己去搬陣列。
void practical_promote_task_to_front() {
    std::vector<int> queue{101, 102, 103, 104, 105};
    int vip_index = 3;   // 把 104 搬到最前面
    std::rotate(queue.begin(),
                queue.begin() + vip_index,
                queue.begin() + vip_index + 1);
    std::cout << "Promoted: ";
    for (int x : queue) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 396 概念:旋轉函數 (Rotate Function) — 用 rotate 暴力驗證
// 難度: medium
// ----------------------------------------------------------------
// 題目簡化:給陣列 nums (長度 n),定義 F(k) = sum(i * rotated_k[i])。
//          求 max F(k) for k = 0..n-1。
//
// 為什麼用 std::rotate (示範暴力法):
//   實際最佳解是 O(n) 數學公式,但這裡示範「逐次旋轉並計算」O(n²) 暴力法,
//   展示 std::rotate 處理「環狀逐步轉動」的直觀寫法。
//
// 複雜度:時間 O(n²);空間 O(1)。
void leetcode_396_rotate_function_concept() {
    std::vector<int> nums{4, 3, 2, 6};
    int n = nums.size();
    long long best = LLONG_MIN;
    for (int k = 0; k < n; ++k) {
        long long f = 0;
        for (int i = 0; i < n; ++i) f += static_cast<long long>(i) * nums[i];
        best = std::max(best, f);
        // 把最後一個元素 rotate 到開頭 (向右旋轉 1 步)
        std::rotate(nums.begin(), nums.end() - 1, nums.end());
    }
    std::cout << "LC396: " << best << '\n';
}

// ----------------------------------------------------------------
// 實務範例:Round-Robin 任務排程
// ----------------------------------------------------------------
// 場景:多個 worker 輪流被分配任務,每處理一輪後「隊頭移到隊尾」 —
//      正是 std::rotate(begin, begin+1, end) 的典型用法。
void practical_round_robin_scheduler() {
    std::vector<std::string> workers{"W1", "W2", "W3", "W4"};
    std::cout << "rounds:";
    for (int round = 0; round < 3; ++round) {
        std::cout << " [" << workers.front() << "]";
        std::rotate(workers.begin(), workers.begin() + 1, workers.end());
    }
    std::cout << '\n';
}

// 編譯: g++ -std=c++20 -Wall -Wextra rotate.cpp -o rotate

// === 預期輸出 ===
// rotate left 2: 3 4 5 1 2 (原 first 現在在 index 3)
// rotate right 1: 5 1 2 3 4
// src: 10 20 30 40 50
// rotate_copy: 40 50 10 20 30
// no-op rotate: 1 2 3
// LC189: 5 6 7 1 2 3 4
// Promoted: 104 101 102 103 105
// LC396: 26
// rounds: [W1] [W2] [W3]
