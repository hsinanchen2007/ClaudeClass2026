// ============================================================
// std::stable_partition
// 分類 (Category): Partitioning operations (分割演算法)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/stable_partition
//   * https://cplusplus.com/reference/algorithm/stable_partition/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::stable_partition 解的問題:
//
//   「跟 partition 一樣 — 把符合述詞的搬到前段、不符合的搬到後段。
//    但!兩組『內部的相對順序』要保留。」
//
// 「stable (穩定)」這個字在 STL 排序/分割演算法裡有固定意義 —
// 「相同類別的元素,在輸出中的相對順序與輸入相同」。
//
// 範例:
//
//   原:    [A1, B1, A2, B2, A3]   (A, B 為兩個類別)
//   partition (非 stable):
//          可能變成 [A2, A1, A3, B1, B2]    ← A 內部順序被打亂
//   stable_partition:
//          一定是   [A1, A2, A3, B1, B2]    ← A 內部、B 內部都保持原順序
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、什麼時候非用 stable_partition 不可?                    │
// └────────────────────────────────────────────────────────────┘
//
// 當「組內順序」攜帶資訊時:
//
//   * VIP 排到前面,但 VIP 之間要按註冊時間先後順序服務
//     (否則先註冊的 VIP 會抱怨「為何後註冊的先被服務」)
//   * 任務佇列分群,但同群內要維持 FIFO
//   * 資料按某 key 分桶,桶內要保留輸入順序 (避免破壞時序)
//   * 排序演算法的「分桶 + 合併」步驟,需要桶內穩定
//
// 不需要組內順序時,直接用 partition 更省記憶體。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、空間代價 — 為什麼 stable 比較慢?                      │
// └────────────────────────────────────────────────────────────┘
//
// 「保序」這件事在 in-place 模式下很難做到,所以標準允許 stable_partition
// 使用 O(N) 暫存記憶體。如果記憶體配置失敗,實作會 fall back 到
// O(N log N) 的「無暫存版」演算法。
//
//   * 有足夠記憶體 → 暫存所有元素分組後再回填,O(N)
//   * 記憶體不足   → 遞迴二分式合併,O(N log N)
//
// 因此「partition 換 stable_partition」是「以空間 + 一些時間 換 順序保證」。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class BidirIt, class UnaryPred>
//   BidirIt stable_partition(BidirIt first, BidirIt last, UnaryPred p);
//
//   * C++17 起有執行策略多載。
//   * 注意:需要 BidirectionalIterator (比 partition 的 ForwardIterator 嚴格)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、回傳值與複雜度                                         │
// └────────────────────────────────────────────────────────────┘
//
//   回傳: 分割點 (與 partition 同義) — 第一個不滿足 p 的位置
//
//   時間: 有暫存 → O(N) move;不足時 → O(N log N) move
//   空間: O(N) 嘗試 (退化版 O(1))
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 保序很重要 (例如 VIP 排序、log 排序) → 用 stable_partition。
//   2. 不在乎組內順序 → 用 partition 更省記憶體。
//   3. 對 std::list 等只有 BidirIt 的容器也適用。
//   4. 「兩次 stable_partition」可解三組分類 (例如 LC 75 顏色排序)。
//
// ============================================================

/*
補充筆記：std::stable_partition
  - std::stable_partition 依 predicate 把資料分成 true 區和 false 區；partition 後同一區內相對順序不一定保留。
  - stable_partition 會保留相對順序，但通常需要更多移動或額外空間；是否穩定要依需求決定。
  - partition_point 只能用在已 partitioned 的範圍；若資料沒有先分區，結果不可靠。
  - predicate 應對同一元素回傳穩定結果；若 predicate 依外部可變狀態改變，分區結果很難推理。
  - partition_copy 需要兩個輸出範圍，分別接 true 和 false 元素；目的地容量仍由呼叫者負責。
  - 分區不是排序；它只保證條件分界，不保證每一區內元素大小順序。
  - std::stable_partition 的 predicate 就是分界線定義；先決定哪些元素應放在 true 區，再看函式是否保留相對順序。
  - std::stable_partition 完成後資料通常只保證被分成兩段，不保證每段內已排序；把 partition 當 sort 使用會得到錯誤假設。
  - std::stable_partition 若回傳 iterator，它通常代表 true 區結尾或第一個 false 位置；使用前要把這個位置當成半開區間邊界理解。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::stable_partition
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. stable_partition 保證什麼？複雜度是多少？
//     答：保證兩組內部的相對順序與輸入相同——[A1,B1,A2,B2,A3] 分割後
//         一定是 [A1,A2,A3,B1,B2]，不會變成 [A2,A1,A3,...]。
//         複雜度是**條件式**的：能取得 O(N) 額外記憶體時是 O(N) 次 move；
//         配置不到時退化為 O(N log N) 次 move 的就地演算法。
//         這正是「以空間換順序保證」的典型取捨。
//     追問：什麼情境非用它不可？（組內順序攜帶資訊時——VIP 之間要按
//           註冊時間、任務佇列分群後同群仍要 FIFO、時序資料分桶）
//
// 🔥 Q2. 為什麼它需要 BidirectionalIterator，而 partition 只要 ForwardIterator？
//     答：非穩定的 partition 可以用「兩端往中間對撞、遇到就 swap」完成，
//         但那個作法本質上會打亂順序。要保序就得做「把元素依序搬到暫存
//         再回填」或遞迴式的就地合併，這兩種都需要能往回走。
//         所以 std::list（BidirIt）可以用 stable_partition，
//         而 std::forward_list（ForwardIt）只能用 partition。
//
// Q3. 為什麼不乾脆一律用 stable_partition，反正保證比較強？
//     答：因為它會嘗試配置 O(N) 暫存記憶體，而且即使成功也比 partition
//         多做搬移工作；配置失敗還會退化成 O(N log N)。在嵌入式或
//         記憶體受限環境這個代價是真實的。順序無意義時（例如後面
//         反正還要再依時戳 sort 一次）用 partition 才對。
//
// ⚠️ 陷阱. 「stable_partition 是 O(N)」這句話對嗎？
//     答：**不完整**。只有在成功取得額外記憶體時才是 O(N)；
//         標準允許實作在配置失敗時退化為 O(N log N)。答題時要把
//         「有暫存 / 無暫存」兩種情況都講出來，只講 O(N) 會被追問。
//     為什麼會錯：多數人把它和 std::sort 的「C++11 起保證 O(n log n)
//         worst case」混在一起，以為複雜度都是無條件保證。
//         stable_partition 與 stable_sort 一樣，複雜度是條件式的。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <iostream>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8, 9};

    // --- 範例 1: 把偶數放前面、保留輸入順序 ---
    auto mid = std::stable_partition(v.begin(), v.end(),
                                     [](int x){ return x % 2 == 0; });
    std::cout << "stable partition (even first): ";
    for (int x : v) std::cout << x << ' ';
    std::cout << "(boundary @ " << (mid - v.begin()) << ")\n";

    // --- 範例 2: 對結構體分組 (依 status 分群,保留 FIFO) ---
    struct Task { int id; bool done; };
    std::vector<Task> tasks{{1, false}, {2, true}, {3, false},
                            {4, true}, {5, false}, {6, true}};
    std::stable_partition(tasks.begin(), tasks.end(),
                          [](const Task& t){ return !t.done; });
    std::cout << "tasks (pending first, FIFO inside): ";
    for (auto& t : tasks)
        std::cout << "{" << t.id << "," << (t.done ? "done" : "todo") << "} ";
    std::cout << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_2161_partition_array_around_pivot();
    void leetcode_75_sort_colors_stable();
    void practical_vip_users_to_front();
    void leetcode_905_sort_by_parity_stable();
    void practical_pin_starred_emails();
    leetcode_2161_partition_array_around_pivot();
    leetcode_75_sort_colors_stable();
    practical_vip_users_to_front();
    leetcode_905_sort_by_parity_stable();
    practical_pin_starred_emails();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 2161: 依樞紐分區陣列 (Partition Array According to Given Pivot)
// ----------------------------------------------------------------
// 題目:給陣列 nums 與 pivot,要求重排成 [< pivot, == pivot, > pivot] 三段,
//      且每一段內部須保持原相對順序。
//
// 為什麼用 std::stable_partition (兩次):
//   題目明確要求「保持原相對順序」 — 普通 partition 不行。
//   * 第一次 stable_partition: < pivot 的搬到前段。
//   * 第二次 stable_partition (在剩餘段): == pivot 的搬到中段。
//   結果就是 [小, 等, 大],各段內順序保留。
//
// 複雜度:時間 O(n) (有足夠記憶體);空間 O(n)。
void leetcode_2161_partition_array_around_pivot() {
    std::vector<int> nums{9, 12, 5, 10, 14, 3, 10};
    int pivot = 10;
    auto m1 = std::stable_partition(nums.begin(), nums.end(),
                                    [pivot](int x){ return x < pivot; });
    std::stable_partition(m1, nums.end(),
                          [pivot](int x){ return x == pivot; });
    std::cout << "LC2161: ";
    for (int x : nums) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 75 變體:顏色排序 — 穩定版
// ----------------------------------------------------------------
// 題目:陣列只含 0、1、2,要求 in-place 排成 [0..., 1..., 2...]。
//      雖然 LC 75 不要求穩定,但這裡示範「兩次 stable_partition」 —
//      當 0/1/2 後面綁附加資料 (例如索引、名稱、權重) 時,
//      穩定性會變得很重要,這個寫法就是那種情境的標準解。
//
// 解法步驟 (兩次 stable_partition):
//   1. 把所有 0 stable_partition 到前段。
//   2. 在剩餘段把所有 1 stable_partition 到前段。
//   3. 結果 = [0..., 1..., 2...],且各組內順序保留。
//
// 複雜度:時間 O(n);空間 O(n)。
void leetcode_75_sort_colors_stable() {
    std::vector<int> nums{2, 0, 2, 1, 1, 0, 1, 2, 0};
    auto m1 = std::stable_partition(nums.begin(), nums.end(),
                                    [](int x){ return x == 0; });
    std::stable_partition(m1, nums.end(),
                          [](int x){ return x == 1; });
    std::cout << "LC75 stable sorted: ";
    for (int x : nums) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:VIP 使用者排到前面但保留註冊順序
// ----------------------------------------------------------------
// 場景:客服系統把 VIP 排到列表前面優先處理,但同類別內要維持
//      註冊時間順序 (FIFO) — 否則先註冊的 VIP 會抱怨「為何後註冊的先被服務」。
//
// 為什麼用 std::stable_partition:
//   「分組」+「組內保序」的雙重需求,正是 stable_partition 的招牌情境。
void practical_vip_users_to_front() {
    struct U { int reg_seq; bool vip; };
    std::vector<U> users{
        {1, false}, {2, true}, {3, false}, {4, true},
        {5, false}, {6, true}, {7, false}
    };
    std::stable_partition(users.begin(), users.end(),
                          [](const U& u){ return u.vip; });
    std::cout << "Practical VIP-first (FIFO inside): ";
    for (auto& u : users)
        std::cout << "{" << u.reg_seq << "," << (u.vip ? "VIP" : "std") << "} ";
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 905 變體:Sort Array By Parity — 穩定版
// ----------------------------------------------------------------
// 題目:把陣列中偶數放前面,奇數放後面;這裡保留組內原順序 (穩定)。
//
// 為什麼用 std::stable_partition:
//   普通 partition 不保證組內順序;若資料帶副欄位 (例如時序),
//   穩定版才能保持原始時序資訊。
//
// 複雜度:時間 O(n);空間 O(n)。
void leetcode_905_sort_by_parity_stable() {
    std::vector<int> nums{3, 1, 2, 4, 6, 5};
    std::stable_partition(nums.begin(), nums.end(),
                          [](int x){ return x % 2 == 0; });
    std::cout << "LC905 stable:";
    for (int x : nums) std::cout << ' ' << x;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:Inbox 把「已加星標」郵件 pin 到頂端 (保留時序)
// ----------------------------------------------------------------
// 場景:郵件列表中已加星標的要 pin 到頂端,但每群組內要保留原本「最新到舊」
//      的時序 — 正是 stable_partition 的招牌場景。
void practical_pin_starred_emails() {
    struct Mail { int id; bool starred; };
    std::vector<Mail> mails{
        {101, false}, {102, true}, {103, false},
        {104, true}, {105, false}, {106, true}
    };
    std::stable_partition(mails.begin(), mails.end(),
                          [](const Mail& m){ return m.starred; });
    std::cout << "pinned order:";
    for (auto& m : mails)
        std::cout << " {" << m.id << "," << (m.starred ? "*" : " ") << "}";
    std::cout << '\n';
}

// 編譯: g++ -std=c++20 -Wall -Wextra stable_partition.cpp -o stable_partition

// === 預期輸出 ===
// stable partition (even first): 2 4 6 8 1 3 5 7 9 (boundary @ 4)
// tasks (pending first, FIFO inside): {1,todo} {3,todo} {5,todo} {2,done} {4,done} {6,done}
// LC2161: 9 5 3 10 10 12 14
// LC75 stable sorted: 0 0 0 1 1 1 2 2 2
// Practical VIP-first (FIFO inside): {2,VIP} {4,VIP} {6,VIP} {1,std} {3,std} {5,std} {7,std}
// LC905 stable: 2 4 6 3 1 5
// pinned order: {102,*} {104,*} {106,*} {101, } {103, } {105, }
