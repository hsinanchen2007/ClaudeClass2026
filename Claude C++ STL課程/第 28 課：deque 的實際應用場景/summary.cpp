// ============================================================
// 第 28 課 總結：deque 的實際應用場景
// 編譯：g++ -std=c++17 -o summary summary.cpp
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

#include <iostream>
#include <deque>
#include <vector>
#include <string>
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

    return 0;
}
