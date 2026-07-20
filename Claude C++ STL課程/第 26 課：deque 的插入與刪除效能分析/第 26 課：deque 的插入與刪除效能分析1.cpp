// =============================================================================
//  第 26 課：deque 的插入與刪除效能分析 1  —  兩端 O(1)、中間 O(n)，以及失效規則
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：#include <deque>
//
//   ┌─ 複雜度總表 ────────────────────────────────────────────────────────┐
//   │  push_front / push_back    攤還 O(1)                                │
//   │  pop_front  / pop_back           O(1)                               │
//   │  emplace_front / emplace_back 攤還 O(1)                             │
//   │  insert(pos, v)（中間）    O(n) —— 精確地說是 O(min(前段, 後段))    │
//   │  erase(pos)（中間）        O(n) —— 同上                             │
//   │  operator[] / at           O(1)（常數比 vector 大）                 │
//   └──────────────────────────────────────────────────────────────────────┘
//
//   ★★ 失效規則（本課最重要、面試最愛考的一段）★★
//   ┌────────────────────┬──────────────┬──────────────────────────────────┐
//   │ 操作               │ iterator     │ reference / pointer              │
//   ├────────────────────┼──────────────┼──────────────────────────────────┤
//   │ push_front/back    │ **全部失效** │ **全部保持有效**  ← 極反直覺     │
//   │ pop_front/back     │ 只有被移除的 │ 只有被移除的失效                 │
//   │ insert（中間）     │ 全部失效     │ 全部失效                         │
//   │ erase（中間）      │ 全部失效     │ 全部失效                         │
//   │ （對照）vector 擴容│ 全部失效     │ **全部失效**                     │
//   └────────────────────┴──────────────┴──────────────────────────────────┘
//   「iterator 失效但 reference 有效」是 deque 獨有的組合，vector 沒有、list 沒有。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼兩端是 O(1) 而中間是 O(n)】
// deque 是 map（指標陣列）+ 固定大小 chunk 的兩層結構。
//   兩端插入：若當前端點的 chunk 還有空位就直接放；沒空位就新配一個 chunk，
//             把指標掛到 map 的端點格。**完全不搬動既有元素** → O(1)。
//             （「攤還」是因為偶爾要重新配置 map 陣列本身，但那很少見且成本分攤下來是常數。）
//   中間插入：chunk 內部是連續的，而且整個 deque 的邏輯順序必須維持，
//             所以必須把插入點某一側的元素**整體位移一格**。搬動量 = 該側元素數。
//
// 【2. deque 的中間插入會「挑比較短的那一側搬」】
// 這是 deque 相對 vector 的實質優勢，而且它是**實作行為**（標準只要求 O(n)）。
// 本機直接讀 libstdc++ 15.2.0 原始碼 /usr/include/c++/15/bits/deque.tcc
// 的 _M_insert_aux 確認：
//     if (index < size() / 2) { push_front(...); 向前搬 }
//     else                    { push_back(...);  向後搬 }
// 意思是：
//     插在前 1/2 → 把「插入點之前」的元素往前推（搬 index 個）
//     插在後 1/2 → 把「插入點之後」的元素往後推（搬 size-index 個）
// 所以真正的成本是 **O(min(index, size - index))**，最差落在正中間 = O(n/2)。
// 對照 vector：insert 一律把「插入點之後」的全部往後搬，成本是 O(size - index)，
// 在**靠近頭部**時最慘（幾乎搬全部）。這就是本檔測到的差距來源。
//
// 【2b. 但「搬的個數少」不等於「比較快」—— 本檔實測推翻了一個常見說法】
// 很多教材寫「deque 中間插入和 vector 差不多」。本機實測結果是：
//     位置 1   ：vector ≈ 310 ms、deque ≈ 1 ms   → deque 快上百倍（如預期）
//     正中間   ：vector ≈ 125 ms、deque ≈ 420 ms → **deque 反而慢約 3.4 倍**
// 為什麼？因為複雜度只算「搬幾個元素」，沒算「搬一個元素多貴」：
//   * vector 的元素全在一塊連續記憶體 → 位移可以用 memmove 一次處理一大段，
//     編譯器/libc 會用 SIMD 向量化，每個元素的平均成本極低。
//   * deque 的元素散在多個不相鄰的 chunk → **無法跨 chunk 做 memmove**，
//     只能逐段處理並在邊界反覆換 chunk，常數大很多。
// 結論：兩者同為 O(n/2)，但 vector 的常數小得多。
// **deque 的優勢只在「靠近頭端」，不在「中間」** —— 中間插入 deque 反而更慢。
// 這也是為什麼「需要大量中間插入」時正解是 list（或重新設計資料結構），
// 而不是換成 deque。
//
// 【3. 為什麼 push_back 會讓 iterator 失效，卻不讓 reference 失效】
// 這是本課最違反直覺、也最能區分「背過」和「懂了」的一點。
//   * reference / pointer 為什麼有效：
//     元素**沒有被搬動**。新元素放進新的（或現有的）chunk，舊元素還躺在
//     原本那個 chunk 的原本那格 —— 位址完全沒變。所以指向它的指標仍然指得對。
//     本檔 main() 有實測：push 5000 次前後，同一個元素的位址一模一樣。
//   * iterator 為什麼失效：
//     deque 的 iterator **不只是一根指標**。它內部有四個欄位：
//         cur（目前元素）、first / last（所在 chunk 的邊界）、node（指回 map 的哪一格）
//     當 map 陣列本身需要重新配置（增長或重新置中）時，所有 iterator 的 node
//     欄位就指向了舊的、已釋放的 map 陣列 → 失效。
//   一句話總結：**元素沒搬，但「導航資訊」被換掉了。**
//
// 【4. 對照 vector：為什麼 vector 是「兩個都失效」】
// vector 擴容時會配置一塊更大的新緩衝區，把元素**整批搬過去**再釋放舊的。
// 元素的位址真的改變了 → 舊指標成為懸空指標 → reference / pointer / iterator 全滅。
// 本檔 main() 有 vector 的對照實測，會看到位址確實變了。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 「攤還 O(1)」的攤還在哪裡
//   push_back 絕大多數時候只是「往 chunk 裡放一格」——真正的 O(1)。
//   偶爾會踩到兩種較貴的路徑：
//     1. 當前 chunk 滿了 → 配置一個新 chunk（一次 malloc，O(chunk 大小)，
//        但不搬既有元素）
//     2. map 陣列的端點也用完了 → 重新配置 map 並把指標搬過去
//        （O(chunk 個數)，遠小於 O(元素個數)）
//   關鍵差異：vector 擴容要搬 **n 個元素**；deque 擴 map 只搬 **n/每chunk元素數
//   根指標**。以 int + libstdc++（每 chunk 128 個）計算，成本差約 128 倍，
//   而且搬指標不需要呼叫元素的移動建構子。
//
// (B) 為什麼 deque 的走訪比 vector 慢（複雜度一樣但常數差很多）
//   vector 走訪是「一根指標一直 ++」，對 CPU 的硬體預取器（prefetcher）
//   極度友善。deque 走訪每到 chunk 邊界就要跳到另一塊完全不相鄰的記憶體，
//   預取器猜不到下一個位址，容易 cache miss。
//   所以：需要大量順序走訪 + 只在尾端增長 → **永遠選 vector**。
//   deque 的價值只在「頭端也要 O(1)」這一點上。
//
// (C) 為什麼不能用 deque 的 iterator 做指標運算的假設
//   d.begin() + 5 是合法的（deque 迭代器是 random access），但它**不是**
//   「位址 + 5*sizeof(T)」。實作要先算跨了幾個 chunk 再定位。
//   絕不可寫 &d[0] + i 這種假設連續的程式碼 —— 那是未定義行為。
//
// 【注意事項 Pay Attention】
// 1. **push_back 之後不要再用舊的 iterator**（即使它「看起來還能動」）。
//    使用失效的 iterator 是未定義行為 —— 可能剛好正常、可能讀到垃圾、
//    可能崩潰，不會有固定結果。
// 2. reference / pointer 在 push_front/push_back 後仍有效，這是**標準保證**
//    （[deque.modifiers]），不是實作巧合。可以放心依賴。
// 3. 「在迴圈裡邊走訪邊 push_back」對 deque 一樣危險（iterator 會失效）。
//    正確做法是用索引 i 走訪，或先收集要加的元素、迴圈結束後再加。
// 4. erase 回傳「下一個有效元素的 iterator」，迴圈中刪除請用
//    `it = d.erase(it);` 而不是 `d.erase(it); ++it;`（後者用了失效的 it）。
// 5. 本檔的耗時數字是**本機實測**，每次執行、每台機器都不同。
//    效能結論要看「數量級關係」，不要背絕對數值。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】deque 的插入刪除效能與失效規則
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. deque 各種插入刪除的複雜度是什麼？靠近頭部插入為什麼比 vector 快？
//     答：兩端 push/pop 皆 O(1)（push 是攤還 O(1)）；中間 insert/erase 是 O(n)。
//         但 deque 的中間插入會**挑元素較少的那一側搬**，實際成本是
//         O(min(index, size-index))；vector 則一律搬「插入點之後」的全部，
//         成本 O(size-index)。所以越靠近頭部，deque 的優勢越大 ——
//         在位置 1 插入時 vector 幾乎要搬全部，deque 只搬 1 個。
//         本機實測（-O2）：位置 1 插入 10 萬次，vector ≈ 310 ms、deque ≈ 1 ms。
//     追問：那在正中間插入呢？deque 還是比較快嗎？
//         → **不是，反而更慢**。兩者都搬約 n/2，但 vector 的元素連續、
//           可用 memmove + SIMD；deque 跨不了 chunk 邊界只能逐段搬。
//           本機實測：正中間 vector ≈ 125 ms、deque ≈ 420 ms（deque 慢約 3.4 倍）。
//           複雜度相同不代表速度相同 —— 常數因子在這裡決定了勝負。
//
// 🔥 Q2. deque 的 push_back 之後，原本的 iterator 和 reference 還能用嗎？
//     答：**iterator 全部失效，但 reference / pointer 全部保持有效**。
//         原因是元素沒有被搬動（位址不變），但 deque 的 iterator 內含指回 map
//         的 node 欄位，map 重新配置後那個欄位就指向已釋放的陣列了。
//         這是標準明文保證的行為，不是實作巧合。
//     追問：vector 也是這樣嗎？
//         → 完全不是。vector 擴容會把元素整批搬到新緩衝區，位址真的改變，
//           所以 iterator、reference、pointer **全部失效**。
//           這正是 deque 與 vector 最容易被考的差異。
//
// 🔥 Q3. 為什麼 deque 說 push_back 是「攤還 O(1)」而不是「O(1)」？
//         它的攤還跟 vector 的攤還是同一回事嗎？
//     答：不是同一回事，而且 deque 便宜非常多。
//         vector 擴容要配置新緩衝區並**搬移全部 n 個元素**（呼叫 n 次移動/複製建構子）。
//         deque 只有在 map 陣列本身用完時才需要重配，而且只搬「chunk 指標」，
//         數量是 n / 每chunk元素數。以 int + libstdc++（128 個/chunk）算，
//         搬的東西少約 128 倍，且是純指標複製、不呼叫任何建構子。
//     追問：deque 有辦法像 vector 那樣先 reserve 避免重配嗎？
//         → 沒有。deque 沒有 reserve()（它不連續，沒有「容量」概念）。
//           但因為重配成本本來就低很多，實務上很少構成問題。
//
// ⚠️ 陷阱 1. 「deque 中間插入比 vector 快，因為它挑短的那側搬」——這句話對嗎？
//     答：**前半句在本機實測是錯的**。deque 確實挑較短的一側搬（搬的個數較少），
//         但複雜度只算「搬幾個」，沒算「搬一個多貴」：
//         vector 的元素連續，位移可用 memmove + SIMD 一次處理一大段；
//         deque 的元素散在不相鄰的 chunk，**無法跨 chunk memmove**，
//         只能逐段處理並在邊界換 chunk，常數大得多。
//         本檔實測（-O2，本機）：正中間插入 10 萬次 vector ≈ 125 ms、
//         deque ≈ 420 ms —— **deque 反而慢約 3.4 倍**。
//         deque 的優勢**只在靠近頭端**（位置 1：vector ≈ 310 ms、deque ≈ 1 ms）。
//     為什麼會錯：把「漸進複雜度相同或更好」直接當成「實際比較快」。
//         O(n/2) vs O(n/2) 只說明成長趨勢一樣，常數因子可以差好幾倍。
//         真的需要頻繁中間插入時，正解是換 list 或重新設計資料結構，
//         不是換成 deque。
//
// ⚠️ 陷阱 2. 下面這段程式碼哪裡錯了？
//         deque<int> d = {1,2,3};
//         int& r  = d[0];
//         auto it = d.begin();
//         d.push_back(4);
//         cout << r << *it;      // ← 問題在哪一個？
//     答：`r` 完全合法（reference 在 push_back 後仍有效，標準保證）；
//         `*it` 是**未定義行為**（iterator 已失效）。
//         很多人會反過來答，或以為「兩個都失效」（把 vector 的規則套過來）、
//         或以為「兩個都有效」（因為看到元素沒被搬動）。
//     為什麼會錯：多數人腦中的模型是「iterator 就是一根指標」。
//         對 vector 而言這大致成立，但 **deque 的 iterator 是一個含四個欄位的
//         結構**（cur / first / last / node）。元素沒搬 → reference 有效；
//         map 被換掉 → iterator 裡的 node 欄位懸空 → iterator 失效。
//         想通「iterator ≠ 指標」，這題就再也不會錯。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <deque>
#include <string>
#include <chrono>
using namespace std;
using namespace chrono;

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 239. Sliding Window Maximum
//   題目：給陣列 nums 與視窗大小 k，回傳每個滑動視窗中的最大值。
//   為什麼用到本主題：這是「單調佇列（monotonic deque）」的經典題，也是
//         deque 在演算法題中**最重要**的用途。它同時用到 deque 的兩個核心能力：
//           - 視窗左界移出 → pop_front()，O(1)
//           - 維護單調性時從尾端彈出較小值 → pop_back()，O(1)
//         這兩件事必須都是 O(1)，整體才會是 O(n)。
//         若改用 vector，pop_front 變成 O(n)，整體退化成 O(nk)。
//   做法：deque 存**索引**（不是值），並維持對應的值由大到小。
//         隊首永遠是當前視窗的最大值。
//   複雜度：時間 O(n)（每個索引最多進出各一次），空間 O(k)。
// -----------------------------------------------------------------------------
vector<int> maxSlidingWindow(const vector<int>& nums, int k) {
    vector<int> result;
    deque<int> dq;   // 存索引；對應的值保持遞減

    for (int i = 0; i < static_cast<int>(nums.size()); ++i) {
        // 1. 隊首若已滑出視窗左界就丟掉 —— 這裡需要 O(1) 的 pop_front
        if (!dq.empty() && dq.front() <= i - k) {
            dq.pop_front();
        }
        // 2. 從尾端彈掉所有「比新元素小」的（它們永遠不可能再當最大值）
        while (!dq.empty() && nums[dq.back()] <= nums[i]) {
            dq.pop_back();
        }
        dq.push_back(i);
        // 3. 視窗成形後，隊首就是答案
        if (i >= k - 1) {
            result.push_back(nums[dq.front()]);
        }
    }
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】即時串流的「延遲告警」滑動視窗（P99 監控的簡化版）
//   情境：API gateway 每收到一個請求就記錄回應時間，維護「最近 N 筆」的視窗，
//         並要能立刻回答「這個視窗內的最大延遲是多少」以觸發告警。
//         每秒上萬筆請求，所以每筆的處理成本必須是 O(1)，不能每次重掃視窗。
//   為什麼用 deque：兩個 deque 各司其職 ——
//         window_ 存原始資料（尾進頭出，都是 O(1)）；
//         maxq_   是單調佇列，隊首恆為視窗最大值（與 LeetCode 239 完全同一招）。
//   為什麼不用 vector：淘汰最舊的那筆是 pop_front，vector 要 O(n) 搬移，
//         在每秒上萬筆的情境下會直接變成瓶頸。
// -----------------------------------------------------------------------------
class LatencyWindow {
    deque<int> window_;   // 最近 N 筆的原始延遲（毫秒）
    deque<int> maxq_;     // 單調遞減佇列，front() 恆為視窗最大值
    size_t     capacity_;
    int        alertMs_;
public:
    LatencyWindow(size_t capacity, int alertMs)
        : capacity_(capacity), alertMs_(alertMs) {}

    // 回傳：本筆進來後是否應觸發告警
    bool record(int latencyMs) {
        // 淘汰最舊的一筆（若視窗已滿）
        if (window_.size() == capacity_) {
            int oldest = window_.front();
            window_.pop_front();                       // O(1)
            // 若被淘汰的正好是目前的最大值，單調佇列的隊首也要跟著移除
            if (!maxq_.empty() && maxq_.front() == oldest) {
                maxq_.pop_front();                     // O(1)
            }
        }
        window_.push_back(latencyMs);                  // O(1)
        // 維護單調遞減：比新值小的都不可能再當最大值
        while (!maxq_.empty() && maxq_.back() < latencyMs) {
            maxq_.pop_back();                          // O(1)
        }
        maxq_.push_back(latencyMs);
        return peakMs() >= alertMs_;
    }
    int    peakMs() const { return maxq_.empty() ? 0 : maxq_.front(); }
    size_t size()   const { return window_.size(); }
};

int main() {
    const int N = 100000;

    cout << "========== 一、插入位置對效能的影響（vector vs deque）==========" << endl;
    cout << "（耗時為本機實測，每次執行都不同；以 -O2 編譯）" << endl;

    // === 測試在頭部附近插入 ===
    cout << "\n--- 在位置 1 插入 " << N << " 次 ---" << endl;
    {
        vector<int> vec(100, 0);
        auto start = high_resolution_clock::now();
        for (int i = 0; i < N; i++)
            vec.insert(vec.begin() + 1, i);
        auto end = high_resolution_clock::now();
        cout << "vector: "
             << duration_cast<milliseconds>(end - start).count()
             << " ms   (每次都要把後面全部往後搬)" << endl;
    }
    {
        deque<int> dq(100, 0);
        auto start = high_resolution_clock::now();
        for (int i = 0; i < N; i++)
            dq.insert(dq.begin() + 1, i);
        auto end = high_resolution_clock::now();
        cout << "deque:  "
             << duration_cast<milliseconds>(end - start).count()
             << " ms   (挑較短的一側 → 只搬 1 個)" << endl;
    }

    // === 測試在中間插入 ===
    cout << "\n--- 在中間位置插入 " << N << " 次 ---" << endl;
    {
        vector<int> vec(100, 0);
        auto start = high_resolution_clock::now();
        for (int i = 0; i < N; i++)
            vec.insert(vec.begin() + static_cast<long long>(vec.size()) / 2, i);
        auto end = high_resolution_clock::now();
        cout << "vector: "
             << duration_cast<milliseconds>(end - start).count()
             << " ms   (搬後半段 ≈ n/2)" << endl;
    }
    {
        deque<int> dq(100, 0);
        auto start = high_resolution_clock::now();
        for (int i = 0; i < N; i++)
            dq.insert(dq.begin() + static_cast<long long>(dq.size()) / 2, i);
        auto end = high_resolution_clock::now();
        cout << "deque:  "
             << duration_cast<milliseconds>(end - start).count()
             << " ms   (也是 n/2，但不能跨 chunk memmove → 反而更慢)" << endl;
    }

    // === 測試在尾部附近插入 ===
    cout << "\n--- 在倒數第 2 個位置插入 " << N << " 次 ---" << endl;
    {
        vector<int> vec(100, 0);
        auto start = high_resolution_clock::now();
        for (int i = 0; i < N; i++)
            vec.insert(vec.end() - 1, i);
        auto end = high_resolution_clock::now();
        cout << "vector: "
             << duration_cast<milliseconds>(end - start).count()
             << " ms   (只搬 1 個，vector 的主場)" << endl;
    }
    {
        deque<int> dq(100, 0);
        auto start = high_resolution_clock::now();
        for (int i = 0; i < N; i++)
            dq.insert(dq.end() - 1, i);
        auto end = high_resolution_clock::now();
        cout << "deque:  "
             << duration_cast<milliseconds>(end - start).count()
             << " ms   (也只搬 1 個)" << endl;
    }

    cout << "\n========== 二、失效規則實測：deque 的 push_back ==========" << endl;
    {
        deque<int> d = {10, 20, 30};
        int*  p      = &d[1];          // 指向元素 20
        int   before = *p;
        cout << "  push 前：*p = " << before << "，位址 = " << static_cast<void*>(p) << endl;

        for (int i = 0; i < 5000; ++i) d.push_back(i);     // 逼它配置大量新 chunk
        for (int i = 0; i < 5000; ++i) d.push_front(i);    // 兩端都擴

        cout << "  push 10000 次後：*p = " << *p
             << "，位址 = " << static_cast<void*>(p) << endl;
        cout << "  該元素在 deque 中的新位置 d[5001] 位址 = "
             << static_cast<const void*>(&d[5001]) << endl;
        cout << "  ★ 位址完全相同 → reference / pointer **仍然有效**（標準保證）" << endl;
        cout << "  ★ 但原本的 iterator 已經失效，不可再解參考（未定義行為）" << endl;
    }

    cout << "\n========== 三、對照組：vector 擴容後連 pointer 都失效 ==========" << endl;
    {
        vector<int> v = {10, 20, 30};
        int* p = &v[1];
        cout << "  push 前：位址 = " << static_cast<void*>(p) << endl;
        for (int i = 0; i < 5000; ++i) v.push_back(i);
        cout << "  push 後：該元素位址 = " << static_cast<const void*>(&v[1]) << endl;
        cout << "  位址相同嗎? " << boolalpha << (p == &v[1]) << endl;
        cout << "  ★ vector 擴容把元素整批搬到新緩衝區 → 舊指標懸空，全部失效" << endl;
    }

    cout << "\n========== 四、LeetCode 239. Sliding Window Maximum ==========" << endl;
    {
        vector<int> nums = {1, 3, -1, -3, 5, 3, 6, 7};
        int k = 3;
        cout << "  nums = [1,3,-1,-3,5,3,6,7], k = " << k << endl;
        vector<int> ans = maxSlidingWindow(nums, k);
        cout << "  各視窗最大值 = [";
        for (size_t i = 0; i < ans.size(); ++i) {
            cout << ans[i];
            if (i + 1 < ans.size()) cout << ",";
        }
        cout << "]" << endl;
        cout << "  ★ 單調佇列讓每個索引最多進出各一次 → 整體 O(n)" << endl;
        cout << "  ★ 靠的正是 pop_front 與 pop_back 都是 O(1)" << endl;
    }

    cout << "\n========== 五、日常實務：API 延遲滑動視窗告警 ==========" << endl;
    {
        LatencyWindow lw(5, 500);   // 視窗 5 筆，峰值 >= 500ms 就告警
        const int samples[] = {120, 95, 480, 510, 130, 88, 102, 76, 91, 64};
        for (int ms : samples) {
            bool alert = lw.record(ms);
            cout << "  延遲 " << ms << "ms → 視窗 " << lw.size()
                 << " 筆, 峰值 " << lw.peakMs() << "ms"
                 << (alert ? "  🔴 告警" : "") << endl;
        }
        cout << "  ★ 510ms 那筆滑出視窗後，峰值自動降回安全值，告警解除" << endl;
        cout << "  ★ 每筆處理都是 O(1)，每秒上萬筆也撐得住" << endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -O2 -Wall -Wextra 第\ 26\ 課：deque\ 的插入與刪除效能分析1.cpp -o deque_perf1
//   （不加 -O2 也能編譯且零警告，但耗時數字會失真 —— 效能測試請務必開最佳化）

// ※ 下方耗時為**本機實測，每次執行都不同**（Dell Precision 7550 / Ubuntu 26.04 /
// （耗時為本機實測，每次執行都不同；以 -O2 編譯）

// === 預期輸出 ===
//   g++ 15.2.0 / -O2 編譯）。請只看「數量級關係」，不要當成固定值或效能保證。
//   本機連跑三次的區間：位置1 vector 310~323ms / deque 1ms；
//   中間 vector 119~135ms / deque 418~445ms。首次執行可能因 CPU 尚未升頻而偏高
//   （曾量到 1092ms 的冷啟動離群值）。位址（0x...）每次執行也都不同。
//
// ========== 一、插入位置對效能的影響（vector vs deque）==========
//
// --- 在位置 1 插入 100000 次 ---
// vector: 323 ms   (每次都要把後面全部往後搬)
// deque:  1 ms   (挑較短的一側 → 只搬 1 個)
//
// --- 在中間位置插入 100000 次 ---
// vector: 135 ms   (搬後半段 ≈ n/2)
// deque:  445 ms   (也是 n/2，但不能跨 chunk memmove → 反而更慢)
//
// --- 在倒數第 2 個位置插入 100000 次 ---
// vector: 0 ms   (只搬 1 個，vector 的主場)
// deque:  1 ms   (也只搬 1 個)
//
// ========== 二、失效規則實測：deque 的 push_back ==========
//   push 前：*p = 20，位址 = 0x5984f6d8d6b4
//   push 10000 次後：*p = 20，位址 = 0x5984f6d8d6b4
//   該元素在 deque 中的新位置 d[5001] 位址 = 0x5984f6d8d6b4
//   ★ 位址完全相同 → reference / pointer **仍然有效**（標準保證）
//   ★ 但原本的 iterator 已經失效，不可再解參考（未定義行為）
//
// ========== 三、對照組：vector 擴容後連 pointer 都失效 ==========
//   push 前：位址 = 0x5984f6d2e1c4
//   push 後：該元素位址 = 0x5984f6d335c4
//   位址相同嗎? false
//   ★ vector 擴容把元素整批搬到新緩衝區 → 舊指標懸空，全部失效
//
// ========== 四、LeetCode 239. Sliding Window Maximum ==========
//   nums = [1,3,-1,-3,5,3,6,7], k = 3
//   各視窗最大值 = [3,3,5,5,6,7]
//   ★ 單調佇列讓每個索引最多進出各一次 → 整體 O(n)
//   ★ 靠的正是 pop_front 與 pop_back 都是 O(1)
//
// ========== 五、日常實務：API 延遲滑動視窗告警 ==========
//   延遲 120ms → 視窗 1 筆, 峰值 120ms
//   延遲 95ms → 視窗 2 筆, 峰值 120ms
//   延遲 480ms → 視窗 3 筆, 峰值 480ms
//   延遲 510ms → 視窗 4 筆, 峰值 510ms  🔴 告警
//   延遲 130ms → 視窗 5 筆, 峰值 510ms  🔴 告警
//   延遲 88ms → 視窗 5 筆, 峰值 510ms  🔴 告警
//   延遲 102ms → 視窗 5 筆, 峰值 510ms  🔴 告警
//   延遲 76ms → 視窗 5 筆, 峰值 510ms  🔴 告警
//   延遲 91ms → 視窗 5 筆, 峰值 130ms
//   延遲 64ms → 視窗 5 筆, 峰值 102ms
//   ★ 510ms 那筆滑出視窗後，峰值自動降回安全值，告警解除
//   ★ 每筆處理都是 O(1)，每秒上萬筆也撐得住
