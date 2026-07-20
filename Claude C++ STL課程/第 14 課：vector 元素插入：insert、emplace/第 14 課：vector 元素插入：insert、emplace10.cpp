// =============================================================================
//  第 14 課：vector 元素插入：insert、emplace10.cpp
//    —  維持排序的插入：lower_bound + insert
// =============================================================================
//
// 【主題資訊 Information】
//   ForwardIt std::lower_bound(ForwardIt first, ForwardIt last, const T& value);
//   iterator  std::vector<T>::insert(const_iterator pos, const T& value);
//   標頭檔: <algorithm>(lower_bound)、<vector>(insert)
//   標準版本: 兩者皆 C++98
//   複雜度: lower_bound 比較次數 O(log n),但 insert 搬移 O(n)
//           → **整體仍是 O(n)**,二分搜尋並不能讓插入變快
//
// 【詳細解釋 Explanation】
//
// 【1. lower_bound 找的是「第一個不小於 value 的位置」】
// 這正是「保持排序插入」需要的位置。用 upper_bound 也能維持排序,
// 差別在**相等元素的插入位置**:
//     lower_bound → 插在所有相等元素**之前**
//     upper_bound → 插在所有相等元素**之後**(等同穩定排序的行為)
// 若元素帶有「先到先服務」的語意(例如同分數依報名順序),
// 應該用 upper_bound 才能維持先後關係。
//
// 【2. 為什麼二分搜尋救不了 O(n)】
// 這是本檔最重要的觀念。整個操作分兩步:
//     找位置:O(log n) 次比較   ← 很快
//     插進去:O(n) 次元素搬移   ← 瓶頸在這
// 總複雜度取兩者較大者,還是 O(n)。lower_bound 省下的是**比較次數**
// (對比較昂貴的型別如 std::string 有實際意義),不是搬移次數。
// 常見誤解是「用了二分搜尋所以是 O(log n)」—— 面試很愛考這一點。
//
// 【3. 什麼時候「排序 vector」仍然是對的選擇】
// 明知插入是 O(n),為什麼實務上還常用排序 vector 而不是 std::set?
//   * 記憶體連續 → 查詢時 cache 命中率遠高於節點分散的紅黑樹,
//     實測上 n 不大(數千以內)時,排序 vector 的查詢往往比 set 更快。
//   * 記憶體開銷小 → set 每個節點要額外存 3 根指標 + 顏色。
//   * 若使用模式是「建構期大量插入、之後幾乎只查詢」,
//     正解是**全部 push_back 完再 sort 一次**(O(n log n)),
//     而不是逐一 sorted_insert(O(n²))。
// 反過來說,若是「持續高頻插入且要隨時保持有序」,才該用 std::set / std::multiset。
//
// 【概念補充 Concept Deep Dive】
// lower_bound 對 random access iterator 是真正的二分搜尋(O(log n) 次比較、
// O(log n) 次 iterator 移動);對 forward iterator(如 std::list)雖然仍只做
// O(log n) 次**比較**,但因為 iterator 只能一步步走,總移動次數退化成 O(n)。
// 這就是為什麼 std::list 有自己的成員函式 sort 而不用 std::sort,
// 也是為什麼「在 list 上做二分搜尋」沒有意義。
//
// 【注意事項 Pay Attention】
// 1. lower_bound 要求範圍**已依同一準則排序**。沒排序就呼叫是 UB,
//    而且不會有任何警告 —— 它會回傳一個看似合理但錯誤的位置。
// 2. 自訂比較器時,insert 與 lower_bound 必須用**同一個**比較器,
//    否則排序不變式會被破壞。
// 3. 批次插入多個元素時,不要迴圈 sorted_insert(O(k×n));
//    改成 push_back 全部 → sort 一次,或用 std::inplace_merge 合併兩段有序區間。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】排序插入
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 用 lower_bound 找位置再 insert,整體複雜度是多少?
//     答：O(n)。lower_bound 只要 O(log n) 次比較,但 insert 仍要搬移
//         插入點之後的所有元素,是 O(n)。總複雜度取較大者 → O(n)。
//         二分搜尋省的是「比較次數」,不是「搬移次數」。
//     追問：那 lower_bound 還有什麼價值?→ 當比較成本很高時(例如比較長字串、
//         或自訂的昂貴比較器),把 O(n) 次比較降成 O(log n) 次仍然很值得;
//         而且搬移對 trivially copyable 型別可退化成一次 memmove,常數極小。
//
// 🔥 Q2. 要維持一個持續有序的集合,排序 vector 和 std::set 怎麼選?
//     答：看使用模式。「建構期大量插入、之後主要查詢」→ 排序 vector 勝
//         (記憶體連續、cache 友善、開銷小),而且應該全部 push_back 完
//         再 sort 一次 O(n log n),不要逐一插入 O(n²)。
//         「持續高頻插入且隨時要有序」→ std::set,插入 O(log n)。
//     追問：n 多大時 set 才會贏?→ 沒有固定答案,要實測。但因為 cache 效應,
//         門檻常常比理論預期高很多(數千個元素上下),小資料量時 vector 幾乎總是贏。
//
// ⚠️ 陷阱. sorted_insert 用 lower_bound 還是 upper_bound?有差嗎?
//     答：兩者都能維持排序,差別在**相等元素**的落點:lower_bound 插在
//         所有相等元素之前,upper_bound 插在之後。若元素帶有先後語意
//         (同分數依報名順序、同優先權依到達時間),必須用 upper_bound
//         才能維持「先到先排前面」。
//     為什麼會錯：只檢查「結果有沒有排序」,兩種寫法都通過,
//         於是以為可以互換。等到需要穩定順序時才發現行為不同。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <algorithm>
#include <string>

void sorted_insert(std::vector<int>& v, int value) {
    // 找到第一個大於等於 value 的位置：O(log n) 次比較
    auto pos = std::lower_bound(v.begin(), v.end(), value);
    // 但插入仍需搬移後段：O(n)。整體是 O(n)，不是 O(log n)
    v.insert(pos, value);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 57. Insert Interval
// 題目：給一組已排序且互不重疊的區間,插入一個新區間,必要時合併重疊部分。
// 為什麼用到本主題：先用 lower_bound 找到新區間該插入的位置、insert 進去,
//                   再把重疊的相鄰區間合併。這是「排序插入」最典型的應用。
// 複雜度：插入 O(n) + 合併掃描 O(n) → O(n)。
// -----------------------------------------------------------------------------
std::vector<std::vector<int>> insertInterval(std::vector<std::vector<int>> intervals,
                                             const std::vector<int>& newInterval) {
    // 依區間起點找到插入位置（維持起點排序）
    auto pos = std::lower_bound(
        intervals.begin(), intervals.end(), newInterval,
        [](const std::vector<int>& a, const std::vector<int>& b) {
            return a[0] < b[0];
        });
    intervals.insert(pos, newInterval);

    // 合併重疊區間
    std::vector<std::vector<int>> merged;
    for (const auto& iv : intervals) {
        if (!merged.empty() && iv[0] <= merged.back()[1]) {
            merged.back()[1] = std::max(merged.back()[1], iv[1]);
        } else {
            merged.push_back(iv);
        }
    }
    return merged;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】排行榜：依分數由高到低維持有序，同分者維持報名先後
// 情境：遊戲/考試排行榜每收到一筆新成績就要插入正確位置並立刻顯示。
// 關鍵：同分時要「先到的排前面」→ 必須用 upper_bound（插在相等元素之後），
//       用 lower_bound 會讓後來者插到同分者前面，破壞先後順序。
// -----------------------------------------------------------------------------
struct Entry {
    std::string name;
    int         score;
};

void leaderboard_insert(std::vector<Entry>& board, const Entry& e) {
    auto pos = std::upper_bound(
        board.begin(), board.end(), e,
        [](const Entry& a, const Entry& b) {
            return a.score > b.score;   // 由高到低
        });
    board.insert(pos, e);
}

int main() {
    std::cout << "=== 基本排序插入 ===" << std::endl;
    std::vector<int> v = {1, 3, 5, 7, 9};

    sorted_insert(v, 4);
    sorted_insert(v, 0);
    sorted_insert(v, 10);

    for (int x : v) {
        std::cout << x << " ";  // 0 1 3 4 5 7 9 10
    }
    std::cout << std::endl;

    std::cout << "\n=== LeetCode 57. Insert Interval ===" << std::endl;
    auto r1 = insertInterval({{1, 3}, {6, 9}}, {2, 5});
    for (const auto& iv : r1) std::cout << "[" << iv[0] << "," << iv[1] << "] ";
    std::cout << std::endl;

    auto r2 = insertInterval({{1, 2}, {3, 5}, {6, 7}, {8, 10}, {12, 16}}, {4, 8});
    for (const auto& iv : r2) std::cout << "[" << iv[0] << "," << iv[1] << "] ";
    std::cout << std::endl;

    std::cout << "\n=== 日常實務: 排行榜 (同分維持先後) ===" << std::endl;
    std::vector<Entry> board;
    leaderboard_insert(board, {"Alice", 95});
    leaderboard_insert(board, {"Bob", 87});
    leaderboard_insert(board, {"Carol", 95});   // 與 Alice 同分，應排在 Alice 之後
    leaderboard_insert(board, {"Dave", 99});
    leaderboard_insert(board, {"Eve", 87});     // 與 Bob 同分，應排在 Bob 之後

    for (const auto& e : board) {
        std::cout << e.name << " " << e.score << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 14 課：vector 元素插入：insert、emplace10.cpp" -o insert10

// === 預期輸出 ===
// === 基本排序插入 ===
// 0 1 3 4 5 7 9 10 
//
// === LeetCode 57. Insert Interval ===
// [1,5] [6,9] 
// [1,2] [3,10] [12,16] 
//
// === 日常實務: 排行榜 (同分維持先後) ===
// Dave 99
// Alice 95
// Carol 95
// Bob 87
// Eve 87
