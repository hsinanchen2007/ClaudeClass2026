// ============================================================
// std::search_n
// 分類 (Category): Non-modifying sequence operations (非修改型)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/search_n
//   * https://cplusplus.com/reference/algorithm/search_n/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::search_n 解決一個非常具體的問題:
//
//   「在這段資料裡,『連續出現 n 次相同值』的第一段在哪?」
//
// 例如 — 在 {1,1,2,2,2,3} 中找「連續 3 個 2」,
// 就會回傳指向第一個 2 (index 2) 的迭代器。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、search vs search_n 的區別                              │
// └────────────────────────────────────────────────────────────┘
//
//   * std::search   找一段「指定的子序列」 (按順序的多個不同元素)
//   * std::search_n 找連續 n 個「相同值」 (同一個值重複 n 次)
//
// 它們解決的問題不同。需要找「ABC」用 search;需要找「000」用 search_n。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、典型應用情境                                           │
// └────────────────────────────────────────────────────────────┘
//
//   * 服務監控:連續 N 次錯誤碼 → 觸發告警
//   * 訊號處理:連續 N 個取樣低於門檻 → 視為訊號中斷
//   * 遊程編碼 (RLE):找連續相同位元的 run
//   * 文字處理:找連續 N 個空白字元 → 視為段落分隔
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class FwdIt, class Size, class T>
//   FwdIt search_n(FwdIt first, FwdIt last, Size count, const T& value);
//
//   template <class FwdIt, class Size, class T, class BinaryPred>
//   FwdIt search_n(FwdIt first, FwdIt last, Size count, const T& value,
//                  BinaryPred p);
//
//   * C++20 起為 constexpr。
//   * C++17 起有執行策略多載。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、回傳值 (Return value)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   * 找到 → 指向「連續段第一個元素」的迭代器
//   * 找不到 → last
//   * count <= 0 → first (C++20 起明確規定)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 最多 O(n) — 一般實作維持線性
//   空間: O(1)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 「連續」是關鍵。要找「總共 n 個」(不要求連續) 用 std::count。
//   2. count <= 0 時 C++20 起保證回傳 first;之前各家實作行為可能不一。
//   3. 述詞版本的語意是 p(element, value),不是兩個元素之間。
//   4. 與 search 的差異 (找子序列 vs 連續相同值) 容易混淆,要記住。
//
// ============================================================

/*
補充筆記：std::search_n
  - search_n 找連續 n 個符合指定值或 predicate 的位置。
  - n 為 0 時通常直接回傳 first，這是容易忽略的邊界。
  - 它適合偵測連續重複、空白區塊或固定長度 run。
  - std::search_n 會改變目標範圍內容或元素順序；呼叫前要確認輸出範圍空間足夠、iterator 有效。
  - copy/transform/generate/fill 寫入目的地時不會自動擴容，除非你使用 back_inserter 這類 insert iterator。
  - remove/unique 不會真的縮短容器，只把保留元素移到前面並回傳新邏輯結尾；還要搭配 erase 才會改變 size。
  - reverse/rotate/swap_ranges 會改變元素位置；若外部保存索引或 iterator，要重新確認是否仍指向預期元素。
  - move algorithm 會把元素搬到目的地，來源仍有效但值可能改變；後續只能重新指定或安全銷毀。
  - shuffle/sample 需要亂數引擎；不要每次呼叫都用同一個固定種子，除非你刻意要可重現測試結果。
*/
#include <algorithm>
#include <iostream>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> v{1, 1, 2, 2, 2, 3, 4, 5, 5, 5, 5};

    // --- 範例 1: 找連續 3 個 2 ---
    auto it = std::search_n(v.begin(), v.end(), 3, 2);
    if (it != v.end())
        std::cout << "3 consecutive 2s at index " << (it - v.begin()) << '\n';

    // --- 範例 2: 找連續 4 個 5 ---
    auto it2 = std::search_n(v.begin(), v.end(), 4, 5);
    if (it2 != v.end())
        std::cout << "4 consecutive 5s at index " << (it2 - v.begin()) << '\n';

    // --- 範例 3: 找不到 (連續 5 個 1) ---
    auto it3 = std::search_n(v.begin(), v.end(), 5, 1);
    std::cout << "5 consecutive 1s: "
              << (it3 == v.end() ? "not found" : "found") << '\n';

    // --- 範例 4: 自訂述詞 — 找連續 3 個「>= 5」的元素 ---
    std::vector<int> w{1, 2, 6, 7, 8, 4};
    auto it4 = std::search_n(w.begin(), w.end(), 3, 5,
                             [](int a, int b){ return a >= b; });
    if (it4 != w.end())
        std::cout << "3 elements >= 5 starting at index "
                  << (it4 - w.begin()) << '\n';

    // --- 範例 5: count = 0 → 回傳 first (C++20) ---
    auto it5 = std::search_n(v.begin(), v.end(), 0, 99);
    std::cout << "count=0 -> begin: "
              << (it5 == v.begin() ? "yes" : "no") << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_485_k_consecutive_ones();
    void practical_consecutive_errors();
    void leetcode_1957_delete_to_avoid_three_same();
    void practical_idle_period_detect();
    leetcode_485_k_consecutive_ones();
    practical_consecutive_errors();
    leetcode_1957_delete_to_avoid_three_same();
    practical_idle_period_detect();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 485 子題:是否存在連續 k 個 1
//                    (Max Consecutive Ones 的子問題)
// ----------------------------------------------------------------
// 題目:給二進位陣列 nums 與整數 k,判斷是否存在「連續 k 個 1」並
//      回傳第一個出現位置;不存在回傳 -1。
//
// 為什麼用 std::search_n:
//   題意 100% 對應「連續 n 個相同值」 — 直接呼叫 search_n(nums, k, 1)。
//
// 解法步驟:
//   1. std::search_n(nums.begin(), nums.end(), k, 1)
//   2. 不等於 end() → 從 begin 算 distance 取得索引。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_485_k_consecutive_ones() {
    std::vector<int> nums{1, 1, 0, 1, 1, 1, 0, 1, 1};
    int k = 3;
    auto it = std::search_n(nums.begin(), nums.end(), k, 1);
    if (it != nums.end())
        std::cout << "LC485: " << k << " ones start at index "
                  << (it - nums.begin()) << '\n';
    else
        std::cout << "LC485: no " << k << " consecutive ones\n";
}

// ----------------------------------------------------------------
// 實務範例:服務健康監控 — 偵測連續錯誤
// ----------------------------------------------------------------
// 場景:監控系統每分鐘記錄一個請求結果碼。SRE 規則是:
//      「連續 3 次 500 狀態碼」就觸發 PagerDuty 告警。
//
// 為什麼用 std::search_n:
//   直接對應「連續 n 個相同值」 — 一行就能完成,
//   不必自己寫計數重置的雙重 for 迴圈。
void practical_consecutive_errors() {
    std::vector<int> codes{200, 200, 500, 500, 500, 200, 500};
    int it_count = 3;
    int err_code = 500;
    auto it = std::search_n(codes.begin(), codes.end(), it_count, err_code);
    if (it != codes.end())
        std::cout << "alert: " << it_count << " consecutive "
                  << err_code << " starting at index "
                  << (it - codes.begin()) << '\n';
    else
        std::cout << "no consecutive errors\n";
}

// ----------------------------------------------------------------
// LeetCode 1957: 刪除字串中的字元以使其變得美觀 (簡化檢測版)
// ----------------------------------------------------------------
// 題目簡化:判斷字串中是否存在「連續 3 個相同字元」 — 美觀字串不允許這種模式。
//
// 為什麼用 std::search_n:
//   對每個可能字元 (a..z),用 search_n(3, c) 檢查是否存在連 3 個 — 即可。
//   不過更通用的做法是寫成 search_n + 自訂謂詞 (比較相鄰一致)。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_1957_delete_to_avoid_three_same() {
    std::string s = "leeetcode";
    bool has_three = false;
    for (char c = 'a'; c <= 'z'; ++c) {
        if (std::search_n(s.begin(), s.end(), 3, c) != s.end()) {
            has_three = true;
            break;
        }
    }
    std::cout << "LC1957 has 3-run: " << std::boolalpha << has_three << '\n';
}

// ----------------------------------------------------------------
// 實務範例:伺服器閒置時段偵測
// ----------------------------------------------------------------
// 場景:CPU 使用率每秒記一筆 (0 或 1 — 是否閒置),
//      偵測「連續 N 秒閒置」(可能適合啟動低功耗模式)。
void practical_idle_period_detect() {
    std::vector<int> idle{0, 0, 1, 1, 1, 1, 1, 0, 1, 1};   // 1 = idle
    int n = 4;
    auto it = std::search_n(idle.begin(), idle.end(), n, 1);
    if (it != idle.end())
        std::cout << "idle " << n << "s starts at idx="
                  << (it - idle.begin()) << '\n';
    else
        std::cout << "no idle period of " << n << "s\n";
}

// === 預期輸出 (Expected output) ===
// 3 consecutive 2s at index 2
// 4 consecutive 5s at index 7
// 5 consecutive 1s: not found
// 3 elements >= 5 starting at index 2
// count=0 -> begin: yes
// LC485: 3 ones start at index 3
// alert: 3 consecutive 500 starting at index 2
// LC1957 has 3-run: true
// idle 4s starts at idx=2
