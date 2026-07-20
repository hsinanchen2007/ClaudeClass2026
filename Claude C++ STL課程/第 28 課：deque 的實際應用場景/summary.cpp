// ============================================================
// 第 28 課 總結：deque 的實際應用場景
// 編譯：g++ -std=c++17 -Wall -Wextra -o summary summary.cpp
// ============================================================
// 【三大經典應用場景】
//
// 1. 滑動視窗最大值（Sliding Window Maximum）
//    用 deque 維持索引的單調遞減序列
//    頭端 = 當前視窗最大值的索引
//    插入新元素時，從尾端移除所有比它小的
//    滑動時，從頭端移除超出視窗範圍的
//    時間複雜度：O(n)（每個元素最多進出 deque 各一次）
//
// 2. Undo/Redo 系統
//    用 deque 存操作歷史（限制最大深度）
//    新操作 → push_back（歷史超過上限 → pop_front 丟棄最舊的）
//    撤銷 → pop_back + 推入 redo 堆疊
//    重做 → 從 redo 堆疊取回 + push_back
//    deque 的優勢：pop_front O(1)（vector 做不到）
//
// 3. 環形緩衝區（Ring Buffer）
//    用 deque 模擬固定大小的循環佇列
//    新增 → push_back（滿了 → pop_front 丟最舊的）
//    支援隨機存取 [i]、取最新 back()、取最舊 front()
// ============================================================
//
// 【主題資訊 Information】
//   std::deque 的四個雙端操作：push_front / push_back / pop_front / pop_back
//   全部攤銷 O(1)；operator[] 也是 O(1)（但常數比 vector 大）
//
//   標頭檔：<deque>、<vector>、<string>
//   本檔標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 三個場景的共同結構】
//   本課三個應用看似無關，其實共用同一個骨架：
//       **一端加入新資料，另一端淘汰舊資料，中間可能還要隨機存取。**
//       滑動視窗：尾端加入新索引、前端淘汰過期索引
//       Undo 歷史：尾端加入新狀態、前端淘汰超出深度的舊狀態
//       環形緩衝：尾端加入新紀錄、前端淘汰最舊的紀錄
//   只要看到這個結構，deque 幾乎一定是正解——因為它是標準容器裡
//   唯一能同時做到「兩端 O(1) 增刪」與「O(1) 隨機存取」的。
//
// 【2. 為什麼其他容器都差一項】
//       vector：兩端加入 ✓（尾端）／頭端刪除 ✗（O(n)，且沒有 pop_front）
//       list  ：兩端增刪 ✓／隨機存取 ✗（bidirectional iterator，取第 i 個要 O(n)）
//       deque ：兩端增刪 ✓／隨機存取 ✓  ← 三項全中
//   這就是 deque 在 STL 中的定位：它不是「更好的 vector」，
//   而是**專門補上「前端操作」這個缺口**的容器。
//
// 【3. 單調佇列：deque 最精妙的用法】
//   滑動視窗最大值需要「前端移除過期的」+「後端移除無用的」兩種刪除，
//   這是唯一必須用到 deque **兩端刪除**能力的場景（另外兩個應用只用到一端）。
//   關鍵洞察：若 nums[j] ≤ nums[i] 且 j < i，那麼 j 永遠被 i 壓著，
//   可以安全丟棄。剩下的元素必然單調遞減，前端就是視窗最大值。
//   總複雜度 O(n)——每個索引最多進出 deque 各一次（攤銷分析）。
//
// 【4. deque 的能力邊界在哪裡】
//   deque 強在「兩端」，不強在「中間」。
//   一旦你的需求變成「把中間某個元素搬到最前面」（例如 LRU 快取），
//   deque 就辦不到 O(1) 了——那要用 list（O(1) splice）+ unordered_map。
//   認清這條界線，比記住 deque 有哪些成員函式更重要。
//
// 【概念補充 Concept Deep Dive】
//   ● 分段連續：deque 一切能力的來源
//     deque = 中央控制器（指標陣列）+ 多塊固定大小的 chunk。
//     前端插入只需要「頭端 chunk 有空位就放，沒有就掛一塊新的」，
//     完全不搬移既有元素 → O(1)。
//     本機 libstdc++ 的 chunk 是 512 位元組（deque<int> 為 128 個元素，
//     第 23 課已用位址不連續處實測驗證）。這是實作值，標準沒有規定。
//
//   ● 代價：存取常數較大、快取局部性較差
//     operator[] 要先算「第幾塊、塊內第幾格」，是兩次間接定址；
//     vector 只要一次加法。元素分散多塊也讓硬體預取器效果變差。
//     所以 **預設仍應選 vector**，只有真的需要前端操作才換 deque。
//
//   ● 攤銷分析的直覺
//     單調佇列的內層 while 看起來會退化成 O(nk)，但每個索引
//     最多被 push 一次、pop 一次，總操作數不超過 2n。
//     「某一輪 while 跑很多次」代表「先前很多輪什麼都沒 pop」——總量守恆。
//     同課 1.cpp 用實際計數驗證過：n=10000 時 push+pop = 19987 ≤ 20000。
//
// 【注意事項 Pay Attention】
//   1. 對空 deque 呼叫 front()/back()/pop_front()/pop_back() 是 **UB**，
//      不保證崩潰、不保證任何特定值。務必先 empty() 檢查。
//   2. 單調佇列存的是**索引**不是值（要靠索引判斷是否過期）。
//   3. deque 存的是「候選者」而非「整個視窗」，長度通常遠小於 k。
//   4. push_front/push_back 使所有 iterator 失效，但 reference/pointer 仍有效。
//   5. chunk 大小 512 位元組是本機 libstdc++ 的實作值，不可依賴。
//   6. 需要「把中間元素搬到端點」時 deque 就不夠了，要改用 list + hash map。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】deque 的實際應用
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 滑動視窗最大值為什麼一定要用 deque？其他容器不行嗎？
//     答：這題需要兩種刪除同時是 O(1)——
//         前端移除「已滑出視窗」的元素，後端移除「比新元素小、永遠沒機會」的元素。
//         vector 的前端刪除是 O(n)；stack/queue 只能碰一端；
//         priority_queue 無法有效率地刪除非堆頂的過期元素。
//         **只有 deque 兩端都是 O(1)。**
//     追問：為什麼存索引不存值？→ 要判斷元素是否滑出視窗，必須知道它的位置。
//         條件 dq.front() <= i - k 就是靠索引判斷的。
//
// 🔥 Q2. 這三個應用（滑動視窗、Undo、環形緩衝）的共同結構是什麼？
//     答：**一端加入新資料，另一端淘汰舊資料**，而且經常還要隨機存取。
//         標準容器裡只有 deque 同時滿足「兩端 O(1) 增刪」與「O(1) 隨機存取」：
//         vector 缺前端刪除，list 缺隨機存取。
//         看到這個結構就該想到 deque。
//     追問：那什麼時候 deque 就不夠了？→ 需要「把**中間**的元素搬到端點」時。
//         最典型的是 LRU 快取：命中時要把該項目移到最前面。
//         deque 做不到 O(1)，要改用 list（O(1) splice）+ unordered_map。
//
// ⚠️ 陷阱. 「單調佇列裡放的是視窗內的所有元素，所以長度一定是 k」——錯在哪？
//     答：deque 裡放的是「**還有可能成為未來某個視窗最大值的候選者**」，
//         長度通常遠小於 k。任何 nums[j] ≤ nums[i] 且 j < i 的 j 都被丟棄了。
//         實測：遞減序列 [9..1]、k=5 時 deque 長度是 5（全都是候選者）；
//         遞增序列 [1..9]、k=5 時 deque 長度只有 1（每個新元素把前面全壓掉）。
//     為什麼會錯：把 deque 當成「視窗的鏡像」，
//         但它其實是「經過剪枝的候選集合」。
//         **沒有這個剪枝，就沒有 O(n)**——剪枝才是整個演算法的靈魂，
//         也是它比暴力 O(nk) 快的唯一原因。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <deque>
#include <vector>
#include <string>
#include <list>
#include <unordered_map>
using namespace std;

// ============================================================
// 應用 1：滑動視窗最大值
// ============================================================
vector<int> maxSlidingWindow(const vector<int>& nums, int k) {
    deque<int> dq;       // 存放索引，維持對應值的單調遞減
    vector<int> result;

    for (int i = 0; i < (int)nums.size(); i++) {
        // ① 移除已滑出視窗的頭端元素
        if (!dq.empty() && dq.front() <= i - k) {
            dq.pop_front();
        }

        // ② 從尾端移除所有比 nums[i] 小的（它們不可能再是最大值）
        while (!dq.empty() && nums[dq.back()] <= nums[i]) {
            dq.pop_back();
        }

        // ③ 加入當前索引
        dq.push_back(i);

        // ④ 視窗形成後，頭端就是最大值
        if (i >= k - 1) {
            result.push_back(nums[dq.front()]);
        }
    }
    return result;
}

// ============================================================
// 應用 2：Undo/Redo 系統
// ============================================================
class UndoRedoSystem {
    static const int MAX_HISTORY = 5;
    deque<string> undoStack;    // 歷史（deque 方便 pop_front 限制深度）
    vector<string> redoStack;
    string currentState;
public:
    UndoRedoSystem(const string& initial) : currentState(initial) {}

    void doAction(const string& action) {
        redoStack.clear();                         // 新操作清空 redo
        undoStack.push_back(currentState);         // 保存當前狀態
        if (undoStack.size() > MAX_HISTORY) {
            undoStack.pop_front();                 // ← deque 的優勢！O(1)
        }
        currentState = action;
        cout << "    執行：" << action << endl;
    }

    void undo() {
        if (undoStack.empty()) { cout << "    無法撤銷！\n"; return; }
        redoStack.push_back(currentState);
        currentState = undoStack.back();
        undoStack.pop_back();
        cout << "    撤銷 → 回到：" << currentState << endl;
    }

    void redo() {
        if (redoStack.empty()) { cout << "    無法重做！\n"; return; }
        undoStack.push_back(currentState);
        currentState = redoStack.back();
        redoStack.pop_back();
        cout << "    重做 → " << currentState << endl;
    }

    void showState() const {
        cout << "    當前：" << currentState
             << "（歷史深度：" << undoStack.size() << "）\n";
    }
};

// ============================================================
// 應用 3：環形緩衝區（Ring Buffer）
// ============================================================
template <typename T>
class RingBuffer {
    deque<T> buffer;
    size_t maxSize;
public:
    RingBuffer(size_t max) : maxSize(max) {}

    void push(const T& item) {
        if (buffer.size() >= maxSize) {
            buffer.pop_front();     // 丟棄最舊的
        }
        buffer.push_back(item);     // 加入最新的
    }

    const T& operator[](size_t i) const { return buffer[i]; }
    const T& latest() const { return buffer.back(); }
    const T& oldest() const { return buffer.front(); }
    size_t size() const { return buffer.size(); }
    bool full() const { return buffer.size() >= maxSize; }
};

// ============================================================
// 【LeetCode 實戰範例】LeetCode 146. LRU Cache
//   題目：固定容量的快取，get / put 都要 O(1)，滿了淘汰「最久未使用」的。
//   為什麼放在本課：它與環形緩衝同屬「固定容量 + 自動淘汰」家族，
//     但淘汰規則從「加入時間」變成「最後使用時間」，
//     於是需要「把中間元素搬到最前」——deque 做不到 O(1)。
//     這題正好標示出 deque 的能力邊界，也說明何時該換 list + hash map。
// ============================================================
class LRUCache {
    using Entry = pair<int, int>;                     // (key, value)
    list<Entry> items_;                                // 前端=最近使用
    unordered_map<int, list<Entry>::iterator> pos_;    // key → 節點位置
    size_t capacity_;
public:
    explicit LRUCache(size_t capacity) : capacity_(capacity) {}

    int get(int key) {
        auto it = pos_.find(key);
        if (it == pos_.end()) return -1;
        // splice 把節點搬到最前：O(1) 且不使 iterator 失效（list 獨有）
        items_.splice(items_.begin(), items_, it->second);
        return it->second->second;
    }

    void put(int key, int value) {
        auto it = pos_.find(key);
        if (it != pos_.end()) {
            it->second->second = value;
            items_.splice(items_.begin(), items_, it->second);
            return;
        }
        if (items_.size() >= capacity_) {              // 滿了 → 淘汰最後一個
            pos_.erase(items_.back().first);
            items_.pop_back();
        }
        items_.emplace_front(key, value);
        pos_[key] = items_.begin();
    }
};

int main() {
    // ============================================================
    // 1. 滑動視窗最大值
    // ============================================================
    cout << "===== 1. 滑動視窗最大值 =====\n";
    {
        vector<int> nums = {1, 3, -1, -3, 5, 3, 6, 7};
        int k = 3;

        cout << "  原始陣列：";
        for (int v : nums) cout << v << " ";
        cout << "\n  k = " << k << "\n";

        vector<int> res = maxSlidingWindow(nums, k);
        cout << "  各視窗最大值：";
        for (int v : res) cout << v << " ";
        cout << endl;
        // 輸出：3 3 5 5 6 7
        // 視窗 [1,3,-1]→3  [3,-1,-3]→3  [-1,-3,5]→5
        // [−3,5,3]→5  [5,3,6]→6  [3,6,7]→7
    }
    cout << "\n";

    // ============================================================
    // 2. Undo/Redo 系統
    // ============================================================
    cout << "===== 2. Undo/Redo 系統 =====\n";
    {
        UndoRedoSystem editor("空白文件");
        editor.doAction("輸入 Hello");
        editor.doAction("輸入 World");
        editor.doAction("設定粗體");
        editor.doAction("改字體大小");
        editor.doAction("加入圖片");
        editor.doAction("調整排版");  // 第 6 步，最舊的被 pop_front
        editor.showState();

        editor.undo();   // 回到「加入圖片」
        editor.undo();   // 回到「改字體大小」
        editor.redo();   // 重做「加入圖片」
        editor.showState();
    }
    cout << "\n";

    // ============================================================
    // 3. 環形緩衝區（日誌系統）
    // ============================================================
    cout << "===== 3. 環形緩衝區 =====\n";
    {
        RingBuffer<string> log(5);  // 最多保留 5 筆

        log.push("09:00 系統啟動");
        log.push("09:01 使用者登入");
        log.push("09:05 查詢資料庫");
        log.push("09:10 匯出報表");
        log.push("09:15 發送郵件");

        cout << "  --- 5 筆日誌 ---\n";
        for (size_t i = 0; i < log.size(); i++)
            cout << "    [" << i << "] " << log[i] << endl;

        // 再加入 2 筆 → 最舊的 2 筆被丟棄
        log.push("09:20 備份資料");
        log.push("09:25 系統更新");

        cout << "  --- 新增 2 筆後 ---\n";
        for (size_t i = 0; i < log.size(); i++)
            cout << "    [" << i << "] " << log[i] << endl;

        cout << "  最舊：" << log.oldest() << endl;
        cout << "  最新：" << log.latest() << endl;
    }
    cout << "\n";

    // ============================================================
    // 4. 三個應用的共同結構
    // ============================================================
    cout << "===== 4. 三個應用的共同結構 =====\n";
    cout << "  滑動視窗：尾端加入新索引、前端淘汰過期索引\n";
    cout << "  Undo 歷史：尾端加入新狀態、前端淘汰超出深度的舊狀態\n";
    cout << "  環形緩衝：尾端加入新紀錄、前端淘汰最舊的紀錄\n";
    cout << "  → 都是「一端加入、另一端淘汰」，而且常要隨機存取。\n";
    cout << "    vector 缺前端刪除，list 缺隨機存取，只有 deque 三項全中。\n\n";

    // ============================================================
    // 5. 實測：單調佇列的攤銷 O(n)
    // ============================================================
    cout << "===== 5. 實測：單調佇列的攤銷 O(n) =====\n";
    {
        vector<int> big;
        big.reserve(10000);
        for (int i = 0; i < 10000; ++i) big.push_back((i * 7919) % 1000);

        deque<int> dq;
        long pushes = 0, pops = 0;
        const int k = 100;
        for (int i = 0; i < (int)big.size(); ++i) {
            if (!dq.empty() && dq.front() <= i - k) { dq.pop_front(); ++pops; }
            while (!dq.empty() && big[dq.back()] <= big[i]) { dq.pop_back(); ++pops; }
            dq.push_back(i); ++pushes;
        }
        cout << "  n = " << big.size() << "，k = " << k << "\n";
        cout << "  push 總次數 = " << pushes << "（= n，每個索引恰好入列一次）\n";
        cout << "  pop  總次數 = " << pops << "\n";
        cout << "  push + pop  = " << (pushes + pops)
             << " ≤ 2n = " << (2 * big.size()) << " ✓\n";
        cout << "  → 內層 while 看似會退化，但每個元素最多進出各一次，總量守恆。\n";
        cout << "  （這組數字每次執行完全相同，可重現）\n\n";
    }

    // ============================================================
    // 6. deque 存的是「候選者」不是整個視窗
    // ============================================================
    cout << "===== 6. deque 存的是候選者，不是整個視窗 =====\n";
    {
        auto maxDequeLen = [](const vector<int>& a, int k) {
            deque<int> dq; size_t mx = 0;
            for (int i = 0; i < (int)a.size(); ++i) {
                if (!dq.empty() && dq.front() <= i - k) dq.pop_front();
                while (!dq.empty() && a[dq.back()] <= a[i]) dq.pop_back();
                dq.push_back(i);
                mx = max(mx, dq.size());
            }
            return mx;
        };
        cout << "  遞減序列 [9,8,...,1] k=5：deque 最大長度 = "
             << maxDequeLen({9,8,7,6,5,4,3,2,1}, 5) << "（全都是候選者）\n";
        cout << "  遞增序列 [1,2,...,9] k=5：deque 最大長度 = "
             << maxDequeLen({1,2,3,4,5,6,7,8,9}, 5) << "（每個新元素把前面全壓掉）\n";
        cout << "  → 沒有這個剪枝就沒有 O(n)——它才是演算法的靈魂。\n\n";
    }

    // ============================================================
    // LeetCode 146. LRU Cache —— deque 的能力邊界
    // ============================================================
    cout << "===== LeetCode 146. LRU Cache（deque 的能力邊界）=====\n";
    {
        LRUCache cache(2);
        cache.put(1, 1);
        cache.put(2, 2);
        cout << "  put(1,1), put(2,2) → get(1) = " << cache.get(1) << "\n";
        cache.put(3, 3);              // 淘汰最久未使用的 key 2
        cout << "  put(3,3) → get(2) = " << cache.get(2) << "（-1 = 已淘汰）\n";
        cache.put(4, 4);              // 淘汰 key 1
        cout << "  put(4,4) → get(1) = " << cache.get(1) << "（已淘汰）"
             << "，get(3) = " << cache.get(3)
             << "，get(4) = " << cache.get(4) << "\n";
        cout << "  → LRU 與環形緩衝同屬「固定容量 + 自動淘汰」家族，\n";
        cout << "    但淘汰規則不同：環形緩衝依「加入時間」，LRU 依「最後使用時間」。\n";
        cout << "    LRU 需要「把中間元素搬到最前」，deque 做不到 O(1)，\n";
        cout << "    所以改用 list（O(1) splice）+ unordered_map。\n";
        cout << "    **這正好標示出 deque 的能力邊界：強在兩端，不強在中間。**\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
// ===== 1. 滑動視窗最大值 =====
//   原始陣列：1 3 -1 -3 5 3 6 7
//   k = 3
//   各視窗最大值：3 3 5 5 6 7
//
// ===== 2. Undo/Redo 系統 =====
//     執行：輸入 Hello
//     執行：輸入 World
//     執行：設定粗體
//     執行：改字體大小
//     執行：加入圖片
//     執行：調整排版
//     當前：調整排版（歷史深度：5）
//     撤銷 → 回到：加入圖片
//     撤銷 → 回到：改字體大小
//     重做 → 加入圖片
//     當前：加入圖片（歷史深度：4）
//
// ===== 3. 環形緩衝區 =====
//   --- 5 筆日誌 ---
//     [0] 09:00 系統啟動
//     [1] 09:01 使用者登入
//     [2] 09:05 查詢資料庫
//     [3] 09:10 匯出報表
//     [4] 09:15 發送郵件
//   --- 新增 2 筆後 ---
//     [0] 09:05 查詢資料庫
//     [1] 09:10 匯出報表
//     [2] 09:15 發送郵件
//     [3] 09:20 備份資料
//     [4] 09:25 系統更新
//   最舊：09:05 查詢資料庫
//   最新：09:25 系統更新
//
// ===== 4. 三個應用的共同結構 =====
//   滑動視窗：尾端加入新索引、前端淘汰過期索引
//   Undo 歷史：尾端加入新狀態、前端淘汰超出深度的舊狀態
//   環形緩衝：尾端加入新紀錄、前端淘汰最舊的紀錄
//   → 都是「一端加入、另一端淘汰」，而且常要隨機存取。
//     vector 缺前端刪除，list 缺隨機存取，只有 deque 三項全中。
//
// ===== 5. 實測：單調佇列的攤銷 O(n) =====
//   n = 10000，k = 100
//   push 總次數 = 10000（= n，每個索引恰好入列一次）
//   pop  總次數 = 9987
//   push + pop  = 19987 ≤ 2n = 20000 ✓
//   → 內層 while 看似會退化，但每個元素最多進出各一次，總量守恆。
//   （這組數字每次執行完全相同，可重現）
//
// ===== 6. deque 存的是候選者，不是整個視窗 =====
//   遞減序列 [9,8,...,1] k=5：deque 最大長度 = 5（全都是候選者）
//   遞增序列 [1,2,...,9] k=5：deque 最大長度 = 1（每個新元素把前面全壓掉）
//   → 沒有這個剪枝就沒有 O(n)——它才是演算法的靈魂。
//
// ===== LeetCode 146. LRU Cache（deque 的能力邊界）=====
//   put(1,1), put(2,2) → get(1) = 1
//   put(3,3) → get(2) = -1（-1 = 已淘汰）
//   put(4,4) → get(1) = -1（已淘汰），get(3) = 3，get(4) = 4
//   → LRU 與環形緩衝同屬「固定容量 + 自動淘汰」家族，
//     但淘汰規則不同：環形緩衝依「加入時間」，LRU 依「最後使用時間」。
//     LRU 需要「把中間元素搬到最前」，deque 做不到 O(1)，
//     所以改用 list（O(1) splice）+ unordered_map。
//     **這正好標示出 deque 的能力邊界：強在兩端，不強在中間。**
