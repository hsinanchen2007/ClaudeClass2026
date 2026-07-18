// ============================================================
// std::is_partitioned   (C++11 起)
// 分類 (Category): Partitioning operations (分割演算法)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/is_partitioned
//   * https://cplusplus.com/reference/algorithm/is_partitioned/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、什麼是「分割 (Partition)」?                            │
// └────────────────────────────────────────────────────────────┘
//
// 「分割」這個詞在 STL 裡有非常具體的意義:
//
//   給定一個述詞 p,如果範圍裡所有「滿足 p」的元素都排在
//   所有「不滿足 p」的元素之前,我們就說這個範圍「依 p 已分割」。
//
// 範例:
//   {2, 4, 6, 1, 3, 5}  依「是偶數」 → 已分割 (前段全偶,後段全奇)
//   {2, 1, 4, 3}        依「是偶數」 → 未分割 (中間奇偶交錯)
//
// 注意:
//   * 「分割」不等於「排序」 — {6, 4, 2, 5, 3, 1} 也是「依偶數分割」 (前段全偶)
//   * 兩段內部的順序「不在乎」,只在乎「滿足者全在前」這條規則。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、std::is_partitioned 做的事                              │
// └────────────────────────────────────────────────────────────┘
//
// 它就是回答一個問題:
//
//   「這段資料是不是『依 p 已分割』?」
//
// 回傳 true 或 false。空範圍視為「已分割」(vacuous true)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、為什麼這個函式有用?                                   │
// └────────────────────────────────────────────────────────────┘
//
// 主要應用:
//   * 「assert / 驗證」 — 對某個分割演算法的結果做 sanity check。
//   * 用於 partition_point 的「前提條件」確認 — partition_point
//     要求輸入必須已分割,否則行為未定義。
//   * 偵測資料是否已是「想要的分組形狀」(例:active 在前、inactive 在後)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class InputIt, class UnaryPred>
//   bool is_partitioned(InputIt first, InputIt last, UnaryPred p);
//
//   * C++20 起為 constexpr。
//   * C++17 起有執行策略多載。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 最多 N 次述詞呼叫 (找到反例會短路)
//   空間: O(1)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 「已分割」不等於「已排序」 — 別誤把它當排序檢查。
//   2. 空範圍永遠回 true。
//   3. 全部都滿足或全部都不滿足,都算已分割 (邊界自然滿足)。
//   4. 想找分割點,呼叫 partition_point (要求已分割才能用)。
//
// ============================================================

/*
補充筆記：std::is_partitioned
  - std::is_partitioned 依 predicate 把資料分成 true 區和 false 區；partition 後同一區內相對順序不一定保留。
  - stable_partition 會保留相對順序，但通常需要更多移動或額外空間；是否穩定要依需求決定。
  - partition_point 只能用在已 partitioned 的範圍；若資料沒有先分區，結果不可靠。
  - predicate 應對同一元素回傳穩定結果；若 predicate 依外部可變狀態改變，分區結果很難推理。
  - partition_copy 需要兩個輸出範圍，分別接 true 和 false 元素；目的地容量仍由呼叫者負責。
  - 分區不是排序；它只保證條件分界，不保證每一區內元素大小順序。
  - std::is_partitioned 的 predicate 就是分界線定義；先決定哪些元素應放在 true 區，再看函式是否保留相對順序。
  - std::is_partitioned 完成後資料通常只保證被分成兩段，不保證每段內已排序；把 partition 當 sort 使用會得到錯誤假設。
  - std::is_partitioned 若回傳 iterator，它通常代表 true 區結尾或第一個 false 位置；使用前要把這個位置當成半開區間邊界理解。
*/
#include <algorithm>
#include <iostream>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    auto is_even = [](int x){ return x % 2 == 0; };

    // --- 範例 1: 已分割 (前段全偶,後段全奇) ---
    std::vector<int> a{2, 4, 6, 1, 3, 5};
    std::cout << std::boolalpha
              << "a partitioned by even: "
              << std::is_partitioned(a.begin(), a.end(), is_even) << '\n';

    // --- 範例 2: 未分割 (奇偶交錯) ---
    std::vector<int> b{2, 1, 4, 3};
    std::cout << "b partitioned by even: "
              << std::is_partitioned(b.begin(), b.end(), is_even) << '\n';

    // --- 範例 3: 空範圍 → true (vacuous truth) ---
    std::vector<int> e;
    std::cout << "empty partitioned: "
              << std::is_partitioned(e.begin(), e.end(), is_even) << '\n';

    // --- 範例 4: 全部都滿足述詞 — 仍視為已分割 ---
    std::vector<int> all_even{2, 4, 6};
    std::cout << "all even: "
              << std::is_partitioned(all_even.begin(), all_even.end(), is_even) << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_2149_rearrange_array_check();
    void practical_active_users_check();
    void practical_validate_sorted_data();
    void leetcode_2sort_colors_check();
    void practical_inbox_unread_first_check();
    leetcode_2149_rearrange_array_check();
    practical_active_users_check();
    practical_validate_sorted_data();
    leetcode_2sort_colors_check();
    practical_inbox_unread_first_check();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 子題:重排後驗證「正在前、負在後」(LC 2149 衍生驗證)
// ----------------------------------------------------------------
// 題目簡化:把陣列重排為「正數在前、負數在後」後,驗證確實已分區。
//
// 為什麼用 std::is_partitioned:
//   做完一輪重排,要寫 assert 確認「形狀」是不是符合預期 — 一行就解決。
//
// 複雜度:時間 O(n),空間 O(1)。
void leetcode_2149_rearrange_array_check() {
    std::vector<int> nums{3, 1, 5, 2, -2, -5, -3, -1};
    bool ok = std::is_partitioned(nums.begin(), nums.end(),
                                  [](int x){ return x >= 0; });
    std::cout << "LC2149 partitioned (pos|neg): " << std::boolalpha << ok << '\n';
}

// ----------------------------------------------------------------
// 實務範例 1:檢查使用者列表「啟用中在前」
// ----------------------------------------------------------------
// 場景:後端 API 回傳使用者列表,前端要求 active 排在前面以便 UI 顯示。
//      上線前用 is_partitioned 做一次 sanity check。
void practical_active_users_check() {
    struct User { int id; bool active; };
    std::vector<User> users{
        {1, true}, {2, true}, {3, true}, {4, false}, {5, false}
    };
    bool grouped = std::is_partitioned(users.begin(), users.end(),
                                       [](const User& u){ return u.active; });
    std::cout << "Practical active-first: " << std::boolalpha << grouped << '\n';
}

// ----------------------------------------------------------------
// 實務範例 2:驗證資料庫匯出檔的「未過期 vs 過期」順序
// ----------------------------------------------------------------
// 場景:ETL 流程要求未過期紀錄在前、過期在後。is_partitioned 一行驗證。
void practical_validate_sorted_data() {
    std::vector<int> ttl_days{30, 15, 7, 1, -1, -10, -30};
    bool ok = std::is_partitioned(ttl_days.begin(), ttl_days.end(),
                                  [](int d){ return d > 0; });
    std::cout << "Practical TTL valid first: " << std::boolalpha << ok << '\n';
}

// ----------------------------------------------------------------
// LeetCode 75 概念:Sort Colors 後驗證「0 在前」(is_partitioned 用法)
// ----------------------------------------------------------------
// 題目:LC 75 要把 0, 1, 2 排序;若只要求「0 在前、其餘在後」,可用
//      is_partitioned 做 sanity check。
//
// 為什麼用 std::is_partitioned:
//   做完一輪 partition 後,用 is_partitioned 確認結果是預期形狀。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_2sort_colors_check() {
    std::vector<int> nums{0, 0, 1, 2, 1, 2};
    bool zeros_first = std::is_partitioned(nums.begin(), nums.end(),
                                           [](int x){ return x == 0; });
    std::cout << "LC75 zeros-first: " << std::boolalpha << zeros_first << '\n';
}

// ----------------------------------------------------------------
// 實務範例:Inbox 未讀郵件「未讀在前」驗證
// ----------------------------------------------------------------
// 場景:郵件列表預設「未讀在前」,UI 載入後檢查順序是否正確。
void practical_inbox_unread_first_check() {
    struct Mail { std::string subj; bool unread; };
    std::vector<Mail> mails{
        {"Subj1", true}, {"Subj2", true}, {"Subj3", false}, {"Subj4", false}
    };
    bool ok = std::is_partitioned(mails.begin(), mails.end(),
                                  [](const Mail& m){ return m.unread; });
    std::cout << "inbox order ok: " << std::boolalpha << ok << '\n';
}

// === 預期輸出 (Expected output) ===
// a partitioned by even: true
// b partitioned by even: false
// empty partitioned: true
// all even: true
// LC2149 partitioned (pos|neg): true
// Practical active-first: true
// Practical TTL valid first: true
// LC75 zeros-first: true
// inbox order ok: true
