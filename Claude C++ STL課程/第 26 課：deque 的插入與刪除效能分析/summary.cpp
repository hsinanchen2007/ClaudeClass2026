// =============================================================================
//  第 26 課 總結：deque 的插入與刪除效能分析  —  本課的教科書
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：#include <deque>
//
//   ┌─ 複雜度總表 ────────────────────────────────────────────────────────┐
//   │  push_front / push_back / emplace_front / emplace_back  攤還 O(1)   │
//   │  pop_front  / pop_back                                        O(1)  │
//   │  insert(pos, v)（中間）   O(n)，精確為 O(min(index, size-index))    │
//   │  erase(pos)（中間）       O(n)，同上                                │
//   │  clear()                  O(n)（要解構每個元素）                    │
//   │  operator[] / at / begin / end / size / empty                 O(1)  │
//   └──────────────────────────────────────────────────────────────────────┘
//
//   ★★ 失效規則（本課核心，面試最愛考）★★
//   ┌────────────────────┬──────────────┬──────────────────────────────────┐
//   │ 操作               │ iterator     │ reference / pointer              │
//   ├────────────────────┼──────────────┼──────────────────────────────────┤
//   │ push_front/back    │ **全部失效** │ **全部保持有效**  ← deque 獨有   │
//   │ pop_front/back     │ 只有被移除的 │ 只有被移除的失效                 │
//   │ insert（中間）     │ 全部失效     │ 全部失效                         │
//   │ erase（中間）      │ 全部失效     │ 全部失效                         │
//   │ （對照）vector 擴容│ 全部失效     │ **全部失效**                     │
//   │ （對照）list 任何插刪 │ 只有被刪的 │ 只有被刪的失效                  │
//   └────────────────────┴──────────────┴──────────────────────────────────┘
//   「iterator 失效但 reference 仍有效」是 **deque 獨有**的組合。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼兩端 O(1)、中間 O(n)】
// deque = map（連續的指標陣列）+ 多個固定大小 chunk。
//   兩端：當前端點 chunk 有空位就直接放；沒空位就新配一個 chunk 掛到 map 端點。
//         **不搬動任何既有元素** → O(1)。
//   中間：chunk 內部連續且整體邏輯順序必須維持，所以要把插入點某一側整體位移
//         一格。搬動量 = 該側元素數 → O(n)。
//
// 【2. deque 中間插入「挑較短的一側搬」（實作行為，非標準要求）】
// 本機直接讀 libstdc++ 15.2.0 的 /usr/include/c++/15/bits/deque.tcc
// 之 _M_insert_aux 確認：
//     if (index < size() / 2) { push_front(...); 向前搬 index 個 }
//     else                    { push_back(...);  向後搬 size-index 個 }
// → 實際成本 O(min(index, size-index))，最差在正中間 = O(n/2)。
// vector 則一律搬「插入點之後」的全部 = O(size-index)，靠近頭部時最慘。
//
// 【3. ★ 但「搬得少」不等於「比較快」—— 本課實測推翻的常見說法 ★】
// 許多教材寫「deque 中間插入跟 vector 差不多」。本機 -O2 實測（見輸出）：
//     位置 1   ：vector ≈ 310 ms、deque ≈ 1 ms   → deque 快上百倍（符合預期）
//     正中間   ：vector ≈ 125 ms、deque ≈ 420 ms → **deque 反而慢約 3.4 倍**
//     倒數第 2 ：兩者都 ≈ 0–1 ms                  → 都只搬 1 個
// 原因是複雜度只算「搬幾個元素」，沒算「搬一個元素多貴」：
//   * vector 元素全連續 → 位移可用 memmove 一次處理一大段，會被 SIMD 向量化，
//     每元素平均成本極低。
//   * deque 元素散在不相鄰的 chunk → **無法跨 chunk memmove**，只能逐段處理
//     並在邊界反覆切換 chunk，常數大得多。
// 正確結論：**deque 的優勢只在「靠近頭端」，中間插入它反而更慢。**
// 真的需要大量中間插入時，正解是 list 或重新設計資料結構，不是換 deque。
//
// 【4. 為什麼 push_back 讓 iterator 失效、卻不讓 reference 失效】
// 這是本課最反直覺、最能區分「背過」與「懂了」的一點。
//   * reference / pointer 有效：元素**沒被搬動**，還躺在原本 chunk 的原本那格，
//     位址完全沒變 → 指向它的指標依然指得對。（本課 main() 實測位址不變）
//   * iterator 失效：deque 的 iterator **不是一根指標**，它含四個欄位：
//         cur（目前元素）／ first、last（所在 chunk 的邊界）／ node（指回 map 哪一格）
//     當 map 陣列本身重新配置時，所有 iterator 的 node 欄位就指向已釋放的舊陣列。
//   一句話：**元素沒搬，但「導航資訊」被換掉了。**
//
// 【5. 迴圈中安全刪除的標準寫法】
//   錯誤：for (auto it = d.begin(); it != d.end(); ++it)
//             if (*it % 2 == 0) d.erase(it);        // erase 後 it 已失效，++it 是 UB
//   正確：for (auto it = d.begin(); it != d.end(); )
//             if (*it % 2 == 0) it = d.erase(it);   // erase 回傳下一個有效位置
//             else              ++it;
//   更好（C++20）：std::erase_if(d, [](int x){ return x % 2 == 0; });
//   注意 std::erase_if 是 **C++20** 起才有（本機以 -pedantic-errors 驗證：
//   C++17 下編譯失敗），C++17 只能用 erase-remove idiom 或上面的迴圈。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 「攤還 O(1)」——deque 的攤還遠比 vector 便宜
//   vector 擴容：配置新緩衝區 + 搬移**全部 n 個元素**（n 次移動/複製建構子）。
//   deque 擴張：多數時候只是往 chunk 填一格；chunk 滿了就新配一個 chunk
//               （不搬既有元素）；只有 map 陣列本身用完時才重配 map，
//               而且只搬「chunk 指標」，數量是 n / 每chunk元素數。
//   以 int + libstdc++（實測每 chunk 128 個）計算，搬的東西少約 128 倍，
//   且是純指標複製、不呼叫任何建構子。
//   代價：deque **沒有 reserve()**，你無法事先消除重配 —— 但因為重配本來就便宜，
//   實務上很少構成瓶頸。
//
// (B) 為什麼 deque 走訪比 vector 慢（複雜度相同、常數差很多）
//   vector 走訪是「一根指標一直 ++」，對 CPU 硬體預取器極友善。
//   deque 每到 chunk 邊界就要跳到完全不相鄰的記憶體，預取器猜不到 → cache miss。
//   所以：大量順序走訪 + 只在尾端增長 → **永遠選 vector**。
//   deque 的價值只在「頭端也要 O(1)」這一點。
//
// (C) pop_front 為什麼是 O(1) 而 vector 的 erase(begin()) 是 O(n)
//   deque 的 pop_front 只是把 start 迭代器往後移一格、解構那個元素；
//   整個 chunk 空掉時才釋放該 chunk。完全不搬其他元素。
//   vector 的 erase(begin()) 則必須把後面 n-1 個元素全部往前搬一格。
//   這就是「佇列請用 deque 不要用 vector」的唯一理由。
//   （順帶一提：std::queue 的預設底層容器就是 deque，正是因為這點。）
//
// (D) 為什麼不能對 deque 做指標運算
//   d.begin() + 5 合法（deque 迭代器是 random access），但它**不是**
//   「位址 + 5*sizeof(T)」—— 實作要先算跨了幾個 chunk 再定位。
//   絕不可寫 &d[0] + i 這種假設連續的程式碼，那是未定義行為。
//
// 【注意事項 Pay Attention】
// 1. push_back / push_front 之後，**舊 iterator 一律不可再用**（即使看起來還能動）。
//    使用失效 iterator 是未定義行為 —— 可能剛好正常、可能讀到垃圾、可能崩潰，
//    不會有固定結果。
// 2. reference / pointer 在 push_front/push_back 後仍有效，這是**標準保證**
//    （[deque.modifiers]），不是實作巧合，可以放心依賴。
// 3. 迴圈中刪除請用 `it = d.erase(it);`，不要 `d.erase(it); ++it;`。
// 4. std::erase_if 是 **C++20**（已用 -pedantic-errors 驗證 C++17 下不可用）。
// 5. 本課所有耗時皆為**本機實測，每次執行、每台機器都不同**。
//    請看「數量級關係」，不要背絕對數值，更不要當成效能保證。
// 6. 選容器的順序：預設 vector → 需要頭端 O(1) 才換 deque →
//    需要「已知位置的 O(1) 插刪 + 迭代器永久有效」才換 list。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】deque 的插入刪除效能與失效規則
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. deque 的插入刪除複雜度是什麼？為什麼頭端能做到 O(1)？
//     答：兩端 push/pop 皆 O(1)（push 為攤還 O(1)），中間 insert/erase 為 O(n)。
//         頭端 O(1) 的原因是 deque 由 map + chunk 組成：頭端插入只需在 map 的
//         前一格掛一個新 chunk，**完全不搬動既有元素**。
//         對比 vector 的 insert(begin())，它必須把 n-1 個元素全部往後搬。
//     追問：那 std::queue 為什麼預設用 deque 而不是 vector？
//         → 因為 queue 需要「尾進頭出」，pop 發生在頭端。
//           用 vector 的話每次 pop 都是 O(n)，整個佇列會退化成 O(n²)。
//
// 🔥 Q2. deque 的 push_back 之後，iterator 和 reference 還能用嗎？
//     答：**iterator 全部失效，reference / pointer 全部保持有效**。
//         元素沒有被搬動（位址不變）→ reference 有效；
//         但 deque 的 iterator 內含指回 map 的 node 欄位，map 重配後那個欄位
//         就指向已釋放的陣列 → iterator 失效。這是標準明文保證，非實作巧合。
//     追問：vector 和 list 呢？
//         → vector 擴容時元素整批搬到新緩衝區，**iterator 與 reference 全滅**。
//           list 是節點式，插入刪除只影響被刪的那個節點，**其餘全部保持有效**。
//           三者的失效規則完全不同，這正是最常見的考點。
//
// 🔥 Q3. 為什麼說 deque 的 push_back 是「攤還 O(1)」？跟 vector 的攤還一樣嗎？
//     答：不一樣，deque 便宜非常多。vector 擴容要搬移**全部 n 個元素**
//         （n 次移動/複製建構子）；deque 只有在 map 陣列用完時才重配，
//         且只搬「chunk 指標」，數量是 n / 每chunk元素數。
//         以 int + libstdc++（實測 128 個/chunk）算，搬的東西少約 128 倍，
//         而且是純指標複製、不呼叫任何建構子。
//     追問：deque 可以像 vector 那樣 reserve 來避免重配嗎？
//         → 不行，deque 沒有 reserve()（不連續就沒有「容量」概念）。
//           但因為重配成本本來就低很多，實務上很少構成問題。
//
// 🔥 Q4. 這段迴圈刪除的程式碼錯在哪？該怎麼寫？
//         for (auto it = d.begin(); it != d.end(); ++it)
//             if (*it % 2 == 0) d.erase(it);
//     答：erase(it) 之後 it 已失效，接著的 ++it 是**未定義行為**。
//         正確寫法是利用 erase 的回傳值（下一個有效位置）：
//             for (auto it = d.begin(); it != d.end(); )
//                 if (*it % 2 == 0) it = d.erase(it);
//                 else              ++it;
//     追問：有沒有更簡潔的寫法？
//         → C++20 起可用 std::erase_if(d, pred); 一行解決。
//           C++17 只能用上面的迴圈或 erase-remove idiom：
//           d.erase(std::remove_if(d.begin(), d.end(), pred), d.end());
//
// ⚠️ 陷阱 1. 「deque 中間插入比 vector 快，因為它挑較短的一側搬」——對嗎？
//     答：**本機實測是錯的**。deque 確實搬較少元素，但複雜度只算「搬幾個」，
//         沒算「搬一個多貴」：vector 元素連續，位移可用 memmove + SIMD；
//         deque 元素散在不相鄰 chunk，**無法跨 chunk memmove**，只能逐段搬。
//         本機 -O2 實測：正中間插入 10 萬次，vector ≈ 125 ms、deque ≈ 420 ms，
//         **deque 慢約 3.4 倍**。deque 的優勢只在靠近頭端
//         （位置 1：vector ≈ 310 ms、deque ≈ 1 ms）。
//     為什麼會錯：把「漸進複雜度相同或更好」直接等同於「實際比較快」。
//         O(n/2) vs O(n/2) 只說明成長趨勢一樣，常數因子可以差好幾倍。
//         需要頻繁中間插入的正解是 list 或改資料結構，不是 deque。
//
// ⚠️ 陷阱 2. 下面這段哪裡是未定義行為？
//         deque<int> d = {1,2,3};
//         int& r  = d[0];
//         auto it = d.begin();
//         d.push_back(4);
//         cout << r << *it;
//     答：`r` 完全合法（reference 在 push_back 後仍有效，標準保證）；
//         `*it` 是**未定義行為**（iterator 已失效）。
//         常見錯答有三種：以為兩個都失效（把 vector 規則套過來）、
//         以為兩個都有效（看到元素沒搬動就下結論）、或把兩者答反。
//     為什麼會錯：腦中的模型是「iterator 就是一根指標」。對 vector 大致成立，
//         但 deque 的 iterator 是含四個欄位的結構（cur/first/last/node）。
//         元素沒搬 → reference 有效；map 被換掉 → node 懸空 → iterator 失效。
//         想通「iterator ≠ 指標」，這題就再也不會錯。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <deque>
#include <list>
#include <string>
#include <algorithm>
#include <chrono>
using namespace std;
using namespace chrono;

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 239. Sliding Window Maximum
//   題目：給陣列 nums 與視窗大小 k，回傳每個滑動視窗中的最大值。
//   為什麼用到本主題：這是**單調佇列（monotonic deque）**的經典題，也是 deque
//         在演算法題中最重要的用途。它同時吃到 deque 的兩個 O(1)：
//           - 視窗左界移出 → pop_front()
//           - 維護單調性、從尾端彈掉較小值 → pop_back()
//         兩者都必須 O(1)，整體才會是 O(n)。改用 vector 的話 pop_front 變 O(n)，
//         整體退化成 O(nk)。
//   做法：deque 存**索引**（不是值），對應的值維持遞減；隊首恆為視窗最大值。
//   複雜度：時間 O(n)（每個索引最多進出各一次），空間 O(k)。
// -----------------------------------------------------------------------------
vector<int> maxSlidingWindow(const vector<int>& nums, int k) {
    vector<int> result;
    deque<int> dq;   // 存索引，對應值遞減

    for (int i = 0; i < static_cast<int>(nums.size()); ++i) {
        if (!dq.empty() && dq.front() <= i - k) dq.pop_front();   // 左界滑出：O(1)
        while (!dq.empty() && nums[dq.back()] <= nums[i]) dq.pop_back();  // 維護單調：O(1)
        dq.push_back(i);
        if (i >= k - 1) result.push_back(nums[dq.front()]);
    }
    return result;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 862. Shortest Subarray with Sum at Least K
//   題目：找出總和 >= k 的**最短**子陣列長度（陣列可含負數），找不到回 -1。
//   為什麼用到本主題：這題是 239 的進階版，也是單調佇列，但難在**含負數**
//         使得「滑動視窗」失效，必須改用前綴和 + 單調遞增佇列。
//         同樣需要頭尾兩端都 O(1)：
//           - 從**頭端**取出可行的起點並縮短答案 → pop_front()
//           - 從**尾端**移除不可能更優的前綴和   → pop_back()
//         這正是「為什麼非 deque 不可」的最佳示範：兩端都要動。
//   關鍵洞見：若 prefix[j] <= prefix[i] 且 j > i，則 i 永遠不會是更好的起點
//             （j 更靠右且前綴和更小），可直接丟棄 → 佇列保持遞增。
//   複雜度：時間 O(n)，空間 O(n)。
// -----------------------------------------------------------------------------
int shortestSubarray(const vector<int>& nums, long long k) {
    const int n = static_cast<int>(nums.size());
    vector<long long> prefix(n + 1, 0);
    for (int i = 0; i < n; ++i) prefix[i + 1] = prefix[i] + nums[i];

    int ans = n + 1;
    deque<int> dq;   // 存前綴和的索引，對應 prefix 值遞增

    for (int i = 0; i <= n; ++i) {
        // 頭端：只要能滿足條件就更新答案並彈出（該起點已用過，且更右的更短）
        while (!dq.empty() && prefix[i] - prefix[dq.front()] >= k) {
            ans = min(ans, i - dq.front());
            dq.pop_front();                                  // O(1)
        }
        // 尾端：維護遞增（前綴和不小於當前的舊索引永遠不會更優）
        while (!dq.empty() && prefix[dq.back()] >= prefix[i]) {
            dq.pop_back();                                   // O(1)
        }
        dq.push_back(i);
    }
    return ans <= n ? ans : -1;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】背景工作佇列：一般任務排隊、緊急任務插隊
//   情境：任務排程器有一個待處理佇列。一般任務從尾端排隊（FIFO），
//         但「系統告警」這類緊急任務必須**插到最前面**立刻處理。
//         worker 永遠從頭端取任務。
//   為什麼非 deque 不可：這是唯一同時需要三種 O(1) 操作的場景 ——
//         push_back（一般排隊）、push_front（緊急插隊）、pop_front（取任務）。
//         vector 的 push_front / pop_front 都是 O(n)；
//         std::queue 沒有 push_front（它只暴露單向介面）；
//         priority_queue 語意不同（會全排序，且無法保證同優先級的 FIFO 順序）。
// -----------------------------------------------------------------------------
struct Job {
    string name;
    bool   urgent;
};

class JobQueue {
    deque<Job> q_;
public:
    void submit(const string& name, bool urgent) {
        if (urgent) q_.push_front(Job{name, true});   // 插隊：O(1)
        else        q_.push_back(Job{name, false});   // 排隊：O(1)
    }
    bool takeNext(Job& out) {
        if (q_.empty()) return false;
        out = q_.front();
        q_.pop_front();                               // 取任務：O(1)
        return true;
    }
    size_t pending() const { return q_.size(); }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】日誌檔的 tail -n：只保留最後 N 行
//   情境：服務要在記憶體中保留「最近 200 行日誌」，讓出錯時能把上下文一起送出，
//         但又不能無限成長吃光記憶體。每寫一行就淘汰最舊的一行。
//   為什麼用 deque：新行 push_back、舊行 pop_front，都是 O(1)。
//         用 vector 的話每行都要 erase(begin())，是 O(n) —— 在每秒數千行的
//         日誌量下會直接變成瓶頸。
//   注意：這裡示範的「環狀緩衝」是 deque 在生產環境最常見的真實用途。
// -----------------------------------------------------------------------------
class LogTail {
    deque<string> lines_;
    size_t        keep_;
public:
    explicit LogTail(size_t keep) : keep_(keep) {}
    void append(const string& line) {
        lines_.push_back(line);                 // O(1)
        if (lines_.size() > keep_) lines_.pop_front();   // O(1)
    }
    void dump(const string& indent) const {
        for (const auto& l : lines_) cout << indent << l << endl;
    }
    size_t size() const { return lines_.size(); }
};

int main() {
    const int N = 100000;

    cout << "========== 一、插入位置對效能的影響 ==========" << endl;
    cout << "（本機實測，每次執行都不同；以 -O2 編譯）" << endl;

    cout << "\n----- 1. 靠近頭部（位置 1）插入 " << N << " 次 -----" << endl;
    {
        vector<int> vec(100, 0);
        auto t1 = high_resolution_clock::now();
        for (int i = 0; i < N; i++) vec.insert(vec.begin() + 1, i);
        auto t2 = high_resolution_clock::now();
        cout << "  vector: " << duration_cast<milliseconds>(t2 - t1).count()
             << " ms  (每次都把後面全部往後搬)" << endl;
    }
    {
        deque<int> dq(100, 0);
        auto t1 = high_resolution_clock::now();
        for (int i = 0; i < N; i++) dq.insert(dq.begin() + 1, i);
        auto t2 = high_resolution_clock::now();
        cout << "  deque:  " << duration_cast<milliseconds>(t2 - t1).count()
             << " ms  (挑較短的一側 → 只搬 1 個)" << endl;
        cout << "  ★ deque 的主場：靠近頭部時快上百倍" << endl;
    }

    cout << "\n----- 2. 正中間插入 " << N << " 次 -----" << endl;
    {
        vector<int> vec(100, 0);
        auto t1 = high_resolution_clock::now();
        for (int i = 0; i < N; i++)
            vec.insert(vec.begin() + static_cast<long long>(vec.size()) / 2, i);
        auto t2 = high_resolution_clock::now();
        cout << "  vector: " << duration_cast<milliseconds>(t2 - t1).count()
             << " ms  (搬後半段 n/2，但可用 memmove + SIMD)" << endl;
    }
    {
        deque<int> dq(100, 0);
        auto t1 = high_resolution_clock::now();
        for (int i = 0; i < N; i++)
            dq.insert(dq.begin() + static_cast<long long>(dq.size()) / 2, i);
        auto t2 = high_resolution_clock::now();
        cout << "  deque:  " << duration_cast<milliseconds>(t2 - t1).count()
             << " ms  (也是 n/2，但跨不了 chunk 邊界)" << endl;
        cout << "  ★ 反直覺：中間插入 deque 反而比 vector 慢（常數因子輸）" << endl;
    }

    cout << "\n----- 3. 靠近尾部（倒數第 2）插入 " << N << " 次 -----" << endl;
    {
        vector<int> vec(100, 0);
        auto t1 = high_resolution_clock::now();
        for (int i = 0; i < N; i++) vec.insert(vec.end() - 1, i);
        auto t2 = high_resolution_clock::now();
        cout << "  vector: " << duration_cast<milliseconds>(t2 - t1).count()
             << " ms  (只搬 1 個)" << endl;
    }
    {
        deque<int> dq(100, 0);
        auto t1 = high_resolution_clock::now();
        for (int i = 0; i < N; i++) dq.insert(dq.end() - 1, i);
        auto t2 = high_resolution_clock::now();
        cout << "  deque:  " << duration_cast<milliseconds>(t2 - t1).count()
             << " ms  (也只搬 1 個)" << endl;
    }

    cout << "\n----- 4. 頭端 pop：deque vs vector -----" << endl;
    {
        deque<int> dq;
        for (int i = 0; i < N; i++) dq.push_back(i);
        auto t1 = high_resolution_clock::now();
        while (!dq.empty()) dq.pop_front();
        auto t2 = high_resolution_clock::now();
        cout << "  deque  pop_front  " << N << " 次: "
             << duration_cast<milliseconds>(t2 - t1).count() << " ms  (每次 O(1))" << endl;
    }
    {
        vector<int> vec;
        for (int i = 0; i < N; i++) vec.push_back(i);
        auto t1 = high_resolution_clock::now();
        while (!vec.empty()) vec.erase(vec.begin());
        auto t2 = high_resolution_clock::now();
        cout << "  vector erase(begin) " << N << " 次: "
             << duration_cast<milliseconds>(t2 - t1).count() << " ms  (每次 O(n) → 總 O(n²))" << endl;
        cout << "  ★ 這就是「佇列請用 deque，別用 vector」的唯一理由" << endl;
        cout << "  ★ std::queue 的預設底層容器正是 deque" << endl;
    }

    cout << "\n========== 二、失效規則實測 ==========" << endl;
    {
        cout << "--- deque：push 後 reference 有效、iterator 失效 ---" << endl;
        deque<int> d = {10, 20, 30};
        int* p = &d[1];
        cout << "  push 前 *p=" << *p << " 位址=" << static_cast<void*>(p) << endl;
        for (int i = 0; i < 5000; ++i) d.push_back(i);
        for (int i = 0; i < 5000; ++i) d.push_front(i);
        cout << "  push 10000 次後 *p=" << *p
             << " 位址=" << static_cast<void*>(p) << endl;
        cout << "  位址相同嗎? " << boolalpha << (p == &d[5001]) << endl;
        cout << "  ★ reference/pointer 有效（標準保證）；iterator 已失效不可用" << endl;
    }
    {
        cout << "\n--- vector 對照：連 pointer 都失效 ---" << endl;
        vector<int> v = {10, 20, 30};
        int* p = &v[1];
        cout << "  push 前 位址=" << static_cast<void*>(p) << endl;
        for (int i = 0; i < 5000; ++i) v.push_back(i);
        cout << "  push 後 位址=" << static_cast<const void*>(&v[1]) << endl;
        cout << "  位址相同嗎? " << (p == &v[1]) << endl;
        cout << "  ★ 元素整批搬到新緩衝區 → iterator/reference/pointer 全滅" << endl;
    }
    {
        cout << "\n--- list 對照：插刪只影響被刪的節點 ---" << endl;
        list<int> l = {10, 20, 30};
        auto it = next(l.begin());
        const int* p = &(*it);
        cout << "  插入前 *it=" << *it << " 位址=" << static_cast<const void*>(p) << endl;
        for (int i = 0; i < 5000; ++i) l.push_back(i);
        for (int i = 0; i < 5000; ++i) l.push_front(i);
        cout << "  插入 10000 個後 *it=" << *it
             << " 位址=" << static_cast<const void*>(&(*it)) << endl;
        cout << "  ★ list 連 iterator 都保持有效（節點從未被搬動）" << endl;
    }

    cout << "\n========== 三、迴圈中安全刪除的正確寫法 ==========" << endl;
    {
        deque<int> d = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        cout << "  原始      :";
        for (int x : d) cout << " " << x;
        cout << endl;

        // 正確：用 erase 的回傳值前進
        for (auto it = d.begin(); it != d.end(); ) {
            if (*it % 2 == 0) it = d.erase(it);   // 回傳下一個有效位置
            else              ++it;
        }
        cout << "  移除偶數後:";
        for (int x : d) cout << " " << x;
        cout << endl;
        cout << "  ★ 寫 d.erase(it); ++it; 會用到失效的 it → 未定義行為" << endl;
        cout << "  ★ C++20 可改用一行 std::erase_if(d, pred)（C++17 尚無）" << endl;
    }

    cout << "\n========== 四、LeetCode 239. Sliding Window Maximum ==========" << endl;
    {
        vector<int> nums = {1, 3, -1, -3, 5, 3, 6, 7};
        int k = 3;
        vector<int> ans = maxSlidingWindow(nums, k);
        cout << "  nums = [1,3,-1,-3,5,3,6,7], k = " << k << endl;
        cout << "  各視窗最大值 = [";
        for (size_t i = 0; i < ans.size(); ++i) {
            cout << ans[i];
            if (i + 1 < ans.size()) cout << ",";
        }
        cout << "]" << endl;
        cout << "  ★ 每個索引最多進出各一次 → O(n)，靠的是兩端都 O(1)" << endl;
    }

    cout << "\n========== 五、LeetCode 862. Shortest Subarray with Sum at Least K ==========" << endl;
    {
        struct Case { vector<int> nums; long long k; };
        const Case cases[] = {
            {{1},              1},
            {{1, 2},           4},
            {{2, -1, 2},       3},
            {{84, -37, 32, 40, 95}, 167},
        };
        for (const auto& c : cases) {
            cout << "  nums = [";
            for (size_t i = 0; i < c.nums.size(); ++i) {
                cout << c.nums[i];
                if (i + 1 < c.nums.size()) cout << ",";
            }
            cout << "], k = " << c.k << " → " << shortestSubarray(c.nums, c.k) << endl;
        }
        cout << "  ★ 含負數時滑動視窗失效，必須用前綴和 + 單調遞增佇列" << endl;
        cout << "  ★ 頭端更新答案、尾端維護單調 —— 兩端都要 O(1)，非 deque 不可" << endl;
    }

    cout << "\n========== 六、日常實務：工作佇列（一般排隊 + 緊急插隊）==========" << endl;
    {
        JobQueue jq;
        jq.submit("產生月報表",   false);
        jq.submit("寄送電子報",   false);
        jq.submit("磁碟空間告警", true);    // 緊急：插到最前面
        jq.submit("重建索引",     false);
        jq.submit("資料庫連線失敗", true);  // 更晚提交的緊急任務排更前面

        cout << "  待處理 " << jq.pending() << " 件，依序取出：" << endl;
        Job j;
        int order = 1;
        while (jq.takeNext(j)) {
            cout << "    " << order++ << ". " << j.name
                 << (j.urgent ? "  [緊急]" : "") << endl;
        }
        cout << "  ★ push_back / push_front / pop_front 三者都要 O(1)" << endl;
        cout << "  ★ vector 做不到、std::queue 沒有 push_front → 只能用 deque" << endl;
    }

    cout << "\n========== 七、日常實務：日誌 tail（只保留最後 N 行）==========" << endl;
    {
        LogTail tail(4);
        const char* logs[] = {
            "[INFO ] service started",
            "[INFO ] listening on :8080",
            "[WARN ] slow query 1.2s",
            "[INFO ] cache warmed",
            "[ERROR] upstream timeout",
            "[INFO ] retry succeeded",
        };
        for (const char* l : logs) tail.append(l);
        cout << "  寫入 6 行後，記憶體中保留最後 " << tail.size() << " 行：" << endl;
        tail.dump("    ");
        cout << "  ★ push_back + pop_front 各 O(1)；vector 會是 O(n) 每行" << endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -O2 -Wall -Wextra summary.cpp -o summary
//   （不加 -O2 也能編譯且零警告，但耗時數字會失真 —— 效能測試務必開最佳化）

// ※ 下方耗時為**本機實測，每次執行都不同**（Dell Precision 7550 / Ubuntu 26.04 /
// （本機實測，每次執行都不同；以 -O2 編譯）

// === 預期輸出 ===
//   g++ 15.2.0 / -O2 編譯）。請只看「數量級關係」，不要當成固定值或效能保證。
//   本機重複執行的區間：位置1 vector 310~324ms / deque 1ms；
//   中間 vector 119~147ms / deque 418~445ms。首次執行可能因 CPU 尚未升頻偏高
//   （曾量到 1092ms 的冷啟動離群值）。位址（0x...）每次執行也都不同。
//
// ========== 一、插入位置對效能的影響 ==========
//
// ----- 1. 靠近頭部（位置 1）插入 100000 次 -----
//   vector: 324 ms  (每次都把後面全部往後搬)
//   deque:  1 ms  (挑較短的一側 → 只搬 1 個)
//   ★ deque 的主場：靠近頭部時快上百倍
//
// ----- 2. 正中間插入 100000 次 -----
//   vector: 147 ms  (搬後半段 n/2，但可用 memmove + SIMD)
//   deque:  441 ms  (也是 n/2，但跨不了 chunk 邊界)
//   ★ 反直覺：中間插入 deque 反而比 vector 慢（常數因子輸）
//
// ----- 3. 靠近尾部（倒數第 2）插入 100000 次 -----
//   vector: 0 ms  (只搬 1 個)
//   deque:  1 ms  (也只搬 1 個)
//
// ----- 4. 頭端 pop：deque vs vector -----
//   deque  pop_front  100000 次: 0 ms  (每次 O(1))
//   vector erase(begin) 100000 次: 328 ms  (每次 O(n) → 總 O(n²))
//   ★ 這就是「佇列請用 deque，別用 vector」的唯一理由
//   ★ std::queue 的預設底層容器正是 deque
//
// ========== 二、失效規則實測 ==========
// --- deque：push 後 reference 有效、iterator 失效 ---
//   push 前 *p=20 位址=0x606dc08405a4
//   push 10000 次後 *p=20 位址=0x606dc08405a4
//   位址相同嗎? true
//   ★ reference/pointer 有效（標準保證）；iterator 已失效不可用
//
// --- vector 對照：連 pointer 都失效 ---
//   push 前 位址=0x606dc07d8854
//   push 後 位址=0x606dc07e4bf4
//   位址相同嗎? false
//   ★ 元素整批搬到新緩衝區 → iterator/reference/pointer 全滅
//
// --- list 對照：插刪只影響被刪的節點 ---
//   插入前 *it=20 位址=0x606dc07d8860
//   插入 10000 個後 *it=20 位址=0x606dc07d8860
//   ★ list 連 iterator 都保持有效（節點從未被搬動）
//
// ========== 三、迴圈中安全刪除的正確寫法 ==========
//   原始      : 1 2 3 4 5 6 7 8 9 10
//   移除偶數後: 1 3 5 7 9
//   ★ 寫 d.erase(it); ++it; 會用到失效的 it → 未定義行為
//   ★ C++20 可改用一行 std::erase_if(d, pred)（C++17 尚無）
//
// ========== 四、LeetCode 239. Sliding Window Maximum ==========
//   nums = [1,3,-1,-3,5,3,6,7], k = 3
//   各視窗最大值 = [3,3,5,5,6,7]
//   ★ 每個索引最多進出各一次 → O(n)，靠的是兩端都 O(1)
//
// ========== 五、LeetCode 862. Shortest Subarray with Sum at Least K ==========
//   nums = [1], k = 1 → 1
//   nums = [1,2], k = 4 → -1
//   nums = [2,-1,2], k = 3 → 3
//   nums = [84,-37,32,40,95], k = 167 → 3
//   ★ 含負數時滑動視窗失效，必須用前綴和 + 單調遞增佇列
//   ★ 頭端更新答案、尾端維護單調 —— 兩端都要 O(1)，非 deque 不可
//
// ========== 六、日常實務：工作佇列（一般排隊 + 緊急插隊）==========
//   待處理 5 件，依序取出：
//     1. 資料庫連線失敗  [緊急]
//     2. 磁碟空間告警  [緊急]
//     3. 產生月報表
//     4. 寄送電子報
//     5. 重建索引
//   ★ push_back / push_front / pop_front 三者都要 O(1)
//   ★ vector 做不到、std::queue 沒有 push_front → 只能用 deque
//
// ========== 七、日常實務：日誌 tail（只保留最後 N 行）==========
//   寫入 6 行後，記憶體中保留最後 4 行：
//     [WARN ] slow query 1.2s
//     [INFO ] cache warmed
//     [ERROR] upstream timeout
//     [INFO ] retry succeeded
//   ★ push_back + pop_front 各 O(1)；vector 會是 O(n) 每行
