// =============================================================================
//  第 28 課：deque 的實際應用場景 1  —  單調佇列與滑動視窗最大值
// =============================================================================
//
// 【主題資訊 Information】
//   std::deque<int> dq;   // 存「索引」，維持對應值單調遞減
//   dq.front();  dq.back();  dq.push_back();  dq.pop_front();  dq.pop_back();
//
//   標頭檔：<deque>、<vector>
//   複雜度：時間 O(n)（每個索引最多入列一次、出列一次）；空間 O(k)
//   本檔標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼這題非用 deque 不可】
//   滑動視窗最大值需要同時做兩件事：
//       (a) 從**前端**移除「已經滑出視窗」的元素
//       (b) 從**後端**移除「比新元素小、永遠不可能成為最大值」的元素
//   一個容器要同時支援兩端的 O(1) 刪除——這正是 deque 的定義。
//   用 vector 的話 (a) 是 O(n)；用 stack 只能碰一端；用 queue 只能從前端刪。
//   **只有 deque 同時滿足 (a) 與 (b)**，這是「資料結構決定演算法」的教科書案例。
//
// 【2. 單調佇列（monotonic deque）的核心洞察】
//   關鍵問題：什麼樣的元素「永遠不可能成為未來任何視窗的最大值」？
//   答案：若 nums[j] ≤ nums[i] 且 j < i（j 比較舊、值又比較小），
//         那麼只要 j 還在視窗內，i 一定也在（因為 i 比較新）。
//         此時 j 永遠被 i 壓著，**永無出頭之日** → 可以安全丟棄。
//   所以我們在加入 i 之前，把後端所有 ≤ nums[i] 的都 pop_back 掉。
//   剩下的元素必然由前到後**單調遞減**，前端就是當前視窗的最大值。
//
// 【3. 為什麼存「索引」而不是「值」】
//   因為要判斷「這個元素滑出視窗了沒」，必須知道它的位置。
//   條件 dq.front() <= i - k 就是「前端那個索引已經落在視窗 [i-k+1, i] 之外」。
//   若只存值就無從判斷。這是所有滑動視窗題的共同技巧。
//
// 【4. 為什麼總複雜度是 O(n) 而不是 O(nk)】
//   看起來內層有個 while 迴圈，好像會退化。但關鍵在**攤銷分析**：
//   每個索引最多被 push_back 一次、最多被 pop 一次（不論從前端或後端）。
//   所以整個過程的 push/pop 總次數不超過 2n，攤銷下來每次迭代 O(1)。
//   內層 while 跑很多次的那一輪，代表之前有很多輪什麼都沒 pop——總量守恆。
//   本檔第 4 節會用實際的操作次數計數驗證這一點。
//
// 【概念補充 Concept Deep Dive】
//   ● 為什麼移除過期元素用 if 而不是 while
//     因為每次迭代 i 只前進一格，最多只有一個元素會滑出視窗。
//     所以 if 就夠了。寫成 while 也正確（只是多一次判斷），
//     但用 if 更精確地表達了「每次最多掉一個」這個不變量。
//
//   ● 單調遞減 vs 單調遞增
//     求視窗最大值 → 維持單調遞減（前端最大）
//     求視窗最小值 → 維持單調遞增（前端最小），只要把 <= 改成 >=
//     這是同一個模式的兩種變形。
//
//   ● pop_back 的條件用 <= 還是 <
//     用 <=（相等時也彈出）可以讓 deque 更短。
//     用 < 也正確，但會保留重複值、佔用更多空間。
//     兩者答案相同，因為相等的值誰當最大值都一樣。
//
//   ● 與其他解法的比較
//     暴力法：每個視窗掃一遍 → O(nk)
//     大頂堆（priority_queue）：O(n log k)，還要處理「刪除過期元素」的麻煩
//     單調 deque：O(n)，且常數很小
//     這題是少數「最優解也很好寫」的題目。
//
// 【注意事項 Pay Attention】
//   1. deque 存的是**索引**不是值；判斷過期與取值都靠索引。
//   2. 對空 deque 呼叫 front()/back() 是 UB——每個存取前都要先檢查 empty()。
//   3. k 必須 ≥ 1 且 ≤ nums.size()，否則要另外處理（本檔加了防呆）。
//   4. 結果的個數是 n - k + 1，不是 n。
//   5. 這個模式只適用於「視窗大小固定」；視窗可變時要用其他方法。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】單調佇列與滑動視窗
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 滑動視窗最大值為什麼要用 deque？用 priority_queue 不行嗎？
//     答：這題需要「前端移除過期元素」+「後端移除無用元素」兩種操作，
//         只有 deque 兩端都是 O(1)。
//         priority_queue 也能解，但它無法有效率地刪除「非堆頂的過期元素」，
//         得額外用 lazy deletion 或存 (值, 索引) 配對再檢查，
//         複雜度變成 O(n log k)，程式碼也更繁瑣。deque 解法是 O(n)。
//     追問：為什麼 deque 存索引不存值？→ 要判斷元素是否已滑出視窗，
//         必須知道它的位置。條件 dq.front() <= i - k 就是靠索引判斷的。
//
// 🔥 Q2. 內層有 while 迴圈，為什麼總複雜度還是 O(n)？
//     答：攤銷分析。每個索引在整個過程中最多被 push_back 一次、
//         最多被 pop 一次（前端或後端），所以總操作數不超過 2n。
//         內層 while 跑很多次的那一輪，代表先前有很多輪什麼都沒 pop——
//         總量是守恆的，攤銷下來每次迭代是 O(1)。
//     追問：怎麼驗證？→ 加個計數器數 push/pop 總次數，
//         會發現它始終不超過 2n（本檔第 4 節有實測）。
//
// ⚠️ 陷阱. 「單調佇列裡放的是視窗內的所有元素」——這個理解錯在哪？
//     答：完全錯。deque 裡放的是「**還有可能成為未來某個視窗最大值**的候選者」，
//         數量通常遠少於 k。
//         任何 nums[j] ≤ nums[i] 且 j < i 的 j 都被丟棄了，
//         因為只要 j 在視窗內、i 就一定也在，j 永遠被壓著。
//         例如 nums = [5,4,3,2,1]、k=5，deque 裡會有 5 個元素（嚴格遞減）；
//         但 nums = [1,2,3,4,5]、k=5，deque 裡最後只剩 1 個。
//     為什麼會錯：把 deque 當成「視窗的鏡像」，
//         而它其實是「經過剪枝的候選集合」。
//         沒有這個剪枝，就沒有 O(n)——這才是整個演算法的靈魂。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <deque>
#include <string>
using namespace std;

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 239. Sliding Window Maximum
//   題目：給定陣列 nums 與視窗大小 k，視窗從左滑到右，
//         每次回報視窗內的最大值。回傳所有視窗最大值組成的陣列。
//   為什麼用到本主題：這個函式**就是** LeetCode 239 的標準最優解。
//         它也是 deque 存在意義的最佳說明——需要同時從兩端 O(1) 刪除，
//         這是其他任何標準容器都做不到的。
//   複雜度：時間 O(n)、空間 O(k)。
// -----------------------------------------------------------------------------
vector<int> maxSlidingWindow(const vector<int>& nums, int k) {
    deque<int> dq;       // 存放索引，維持對應值的單調遞減
    vector<int> result;

    // 防呆：k 不合法時直接回傳空結果，避免後面的索引運算出錯
    if (k <= 0 || nums.empty() || k > static_cast<int>(nums.size())) return result;
    result.reserve(nums.size() - static_cast<size_t>(k) + 1);

    for (int i = 0; i < (int)nums.size(); i++) {
        // ① 移除已經滑出視窗的頭端元素
        //    用 if 就夠：i 每次只前進一格，最多只有一個元素會過期
        if (!dq.empty() && dq.front() <= i - k) {
            dq.pop_front();
        }

        // ② 從尾端移除所有比 nums[i] 小的元素
        //    因為它們不可能再成為任何未來視窗的最大值
        //    （它們比 i 舊、值又比 i 小 → 只要它們在視窗內，i 一定也在）
        while (!dq.empty() && nums[dq.back()] <= nums[i]) {
            dq.pop_back();
        }

        // ③ 把當前索引加入尾端
        dq.push_back(i);

        // ④ 當視窗已經形成（i >= k-1），記錄最大值
        //    此時 deque 前端必定是視窗內最大值的索引
        if (i >= k - 1) {
            result.push_back(nums[dq.front()]);
        }
    }
    return result;
}

// 同一個演算法，但額外統計 deque 的操作次數，用來驗證攤銷 O(n)
struct WindowStats {
    vector<int> result;
    long pushes = 0;
    long pops = 0;
    size_t maxDequeSize = 0;
};

WindowStats maxSlidingWindowInstrumented(const vector<int>& nums, int k) {
    WindowStats st;
    deque<int> dq;
    if (k <= 0 || nums.empty() || k > static_cast<int>(nums.size())) return st;

    for (int i = 0; i < (int)nums.size(); i++) {
        if (!dq.empty() && dq.front() <= i - k) { dq.pop_front(); ++st.pops; }
        while (!dq.empty() && nums[dq.back()] <= nums[i]) { dq.pop_back(); ++st.pops; }
        dq.push_back(i); ++st.pushes;
        st.maxDequeSize = max(st.maxDequeSize, dq.size());
        if (i >= k - 1) st.result.push_back(nums[dq.front()]);
    }
    return st;
}

// 對照組：暴力法 O(n*k)，用來驗證單調佇列版的答案正確
vector<int> maxSlidingWindowBruteForce(const vector<int>& nums, int k) {
    vector<int> result;
    if (k <= 0 || nums.empty() || k > static_cast<int>(nums.size())) return result;
    for (size_t i = 0; i + static_cast<size_t>(k) <= nums.size(); ++i) {
        int m = nums[i];
        for (int j = 1; j < k; ++j) m = max(m, nums[i + static_cast<size_t>(j)]);
        result.push_back(m);
    }
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】伺服器監控：最近 N 秒的尖峰延遲告警
//   情境：監控系統每秒收到一筆 API 延遲。告警規則是
//         「最近 60 秒內的**最高**延遲若超過門檻就發警報」。
//         每秒都要重算一次「最近 N 秒的最大值」——
//         若每次都重掃整個視窗就是 O(N)，一天 86400 秒就是 86400×N 次比較。
//         用單調佇列，每秒的攤銷成本是 O(1)。
//   這是滑動視窗最大值在真實系統中最常見的樣貌。
// -----------------------------------------------------------------------------
class PeakLatencyMonitor {
    deque<pair<int, int>> mono_;   // (時間戳, 延遲值)，維持延遲單調遞減
    int windowSec_;
    int threshold_;
public:
    PeakLatencyMonitor(int windowSec, int thresholdMs)
        : windowSec_(windowSec), threshold_(thresholdMs) {}

    // 回傳 (目前視窗峰值, 是否觸發告警)
    pair<int, bool> record(int timestamp, int latencyMs) {
        // ① 移除滑出時間視窗的
        while (!mono_.empty() && mono_.front().first <= timestamp - windowSec_) {
            mono_.pop_front();
        }
        // ② 移除不可能再成為峰值的
        while (!mono_.empty() && mono_.back().second <= latencyMs) {
            mono_.pop_back();
        }
        // ③ 加入新樣本
        mono_.push_back({timestamp, latencyMs});

        int peak = mono_.front().second;   // 前端就是視窗峰值
        return {peak, peak > threshold_};
    }

    size_t candidates() const { return mono_.size(); }
};

int main() {
    cout << "=== 1. LeetCode 239. Sliding Window Maximum ===" << endl;
    vector<int> nums = {1, 3, -1, -3, 5, 3, 6, 7};
    int k = 3;

    cout << "原始陣列：";
    for (int v : nums) cout << v << " ";
    cout << "，k = " << k << endl;

    vector<int> res = maxSlidingWindow(nums, k);
    cout << "各視窗最大值：";
    for (int val : res) cout << val << " ";
    cout << endl;
    // 輸出：3 3 5 5 6 7
    cout << "視窗個數 = " << res.size()
         << "（= n - k + 1 = " << nums.size() << " - " << k << " + 1）" << endl;

    cout << "\n=== 2. 逐視窗對照 ===" << endl;
    for (size_t i = 0; i + static_cast<size_t>(k) <= nums.size(); ++i) {
        cout << "  [";
        for (int j = 0; j < k; ++j) {
            cout << nums[i + static_cast<size_t>(j)] << (j + 1 < k ? "," : "");
        }
        cout << "] → " << res[i] << endl;
    }

    cout << "\n=== 3. 與暴力法交叉驗證答案 ===" << endl;
    vector<int> brute = maxSlidingWindowBruteForce(nums, k);
    bool same = (brute == res);
    cout << "單調佇列 O(n) 與暴力法 O(n*k) 結果相同？ "
         << boolalpha << same << endl;
    // 再用一組較刁鑽的資料驗證
    vector<int> tricky = {9, 8, 7, 6, 5, 4, 3, 2, 1};
    bool same2 = (maxSlidingWindow(tricky, 4) == maxSlidingWindowBruteForce(tricky, 4));
    vector<int> ascending = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    bool same3 = (maxSlidingWindow(ascending, 4) == maxSlidingWindowBruteForce(ascending, 4));
    cout << "遞減序列驗證：" << same2 << "，遞增序列驗證：" << same3 << endl;

    cout << "\n=== 4. 實測攤銷 O(n)：deque 的操作總次數 ===" << endl;
    // 造一組較大的資料
    vector<int> big;
    big.reserve(10000);
    for (int i = 0; i < 10000; ++i) big.push_back((i * 7919) % 1000);   // 偽隨機但確定
    WindowStats st = maxSlidingWindowInstrumented(big, 100);
    cout << "n = " << big.size() << "，k = 100" << endl;
    cout << "  push_back 總次數 = " << st.pushes << "（= n，每個索引恰好入列一次）" << endl;
    cout << "  pop 總次數       = " << st.pops << endl;
    cout << "  push + pop       = " << (st.pushes + st.pops)
         << " ≤ 2n = " << (2 * big.size()) << " ✓" << endl;
    cout << "  → 這就是攤銷 O(n) 的直接證據：內層 while 看似會退化，" << endl;
    cout << "    但每個元素最多進出各一次，總量守恆。" << endl;
    cout << "（這組數字每次執行完全相同，可重現）" << endl;

    cout << "\n=== 5. deque 裡放的是「候選者」，不是整個視窗 ===" << endl;
    cout << "  最大 deque 長度 = " << st.maxDequeSize
         << "（視窗大小 k = 100）" << endl;
    WindowStats desc = maxSlidingWindowInstrumented({9,8,7,6,5,4,3,2,1}, 5);
    WindowStats asc  = maxSlidingWindowInstrumented({1,2,3,4,5,6,7,8,9}, 5);
    cout << "  遞減序列 [9..1] k=5：deque 最大長度 = " << desc.maxDequeSize
         << "（全部都是候選者）" << endl;
    cout << "  遞增序列 [1..9] k=5：deque 最大長度 = " << asc.maxDequeSize
         << "（每個新元素都把前面全部壓掉）" << endl;
    cout << "  → deque 存的是「還有可能成為未來最大值的候選者」，" << endl;
    cout << "    不是視窗的鏡像。這個剪枝才是 O(n) 的來源。" << endl;

    cout << "\n=== 日常實務：最近 N 秒的尖峰延遲告警 ===" << endl;
    PeakLatencyMonitor mon(5, 200);        // 5 秒視窗、門檻 200ms
    struct { int ts; int ms; } samples[] = {
        {1, 45}, {2, 60}, {3, 350}, {4, 55}, {5, 40},
        {6, 38}, {7, 42}, {8, 51}, {9, 47}, {10, 260}
    };
    for (auto& s : samples) {
        auto [peak, alert] = mon.record(s.ts, s.ms);
        cout << "  t=" << s.ts << "s 延遲 " << s.ms << "ms"
             << " → 視窗峰值 " << peak << "ms"
             << (alert ? "  🔴 告警！" : "")
             << "（候選者 " << mon.candidates() << " 個）" << endl;
    }
    cout << "  → 注意 t=3 的 350ms 在 t=8 之後就滑出視窗，告警自動解除；" << endl;
    cout << "    每秒的攤銷成本是 O(1)，不是 O(視窗長度)。" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 28 課：deque 的實際應用場景1.cpp" -o demo28_1

// === 預期輸出 ===
// === 1. LeetCode 239. Sliding Window Maximum ===
// 原始陣列：1 3 -1 -3 5 3 6 7 ，k = 3
// 各視窗最大值：3 3 5 5 6 7
// 視窗個數 = 6（= n - k + 1 = 8 - 3 + 1）
//
// === 2. 逐視窗對照 ===
//   [1,3,-1] → 3
//   [3,-1,-3] → 3
//   [-1,-3,5] → 5
//   [-3,5,3] → 5
//   [5,3,6] → 6
//   [3,6,7] → 7
//
// === 3. 與暴力法交叉驗證答案 ===
// 單調佇列 O(n) 與暴力法 O(n*k) 結果相同？ true
// 遞減序列驗證：true，遞增序列驗證：true
//
// === 4. 實測攤銷 O(n)：deque 的操作總次數 ===
// n = 10000，k = 100
//   push_back 總次數 = 10000（= n，每個索引恰好入列一次）
//   pop 總次數       = 9987
//   push + pop       = 19987 ≤ 2n = 20000 ✓
//   → 這就是攤銷 O(n) 的直接證據：內層 while 看似會退化，
//     但每個元素最多進出各一次，總量守恆。
// （這組數字每次執行完全相同，可重現）
//
// === 5. deque 裡放的是「候選者」，不是整個視窗 ===
//   最大 deque 長度 = 14（視窗大小 k = 100）
//   遞減序列 [9..1] k=5：deque 最大長度 = 5（全部都是候選者）
//   遞增序列 [1..9] k=5：deque 最大長度 = 1（每個新元素都把前面全部壓掉）
//   → deque 存的是「還有可能成為未來最大值的候選者」，
//     不是視窗的鏡像。這個剪枝才是 O(n) 的來源。
//
// === 日常實務：最近 N 秒的尖峰延遲告警 ===
//   t=1s 延遲 45ms → 視窗峰值 45ms（候選者 1 個）
//   t=2s 延遲 60ms → 視窗峰值 60ms（候選者 1 個）
//   t=3s 延遲 350ms → 視窗峰值 350ms  🔴 告警！（候選者 1 個）
//   t=4s 延遲 55ms → 視窗峰值 350ms  🔴 告警！（候選者 2 個）
//   t=5s 延遲 40ms → 視窗峰值 350ms  🔴 告警！（候選者 3 個）
//   t=6s 延遲 38ms → 視窗峰值 350ms  🔴 告警！（候選者 4 個）
//   t=7s 延遲 42ms → 視窗峰值 350ms  🔴 告警！（候選者 3 個）
//   t=8s 延遲 51ms → 視窗峰值 55ms（候選者 2 個）
//   t=9s 延遲 47ms → 視窗峰值 51ms（候選者 2 個）
//   t=10s 延遲 260ms → 視窗峰值 260ms  🔴 告警！（候選者 1 個）
//   → 注意 t=3 的 350ms 在 t=8 之後就滑出視窗，告警自動解除；
//     每秒的攤銷成本是 O(1)，不是 O(視窗長度)。
