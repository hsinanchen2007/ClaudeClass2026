// =============================================================================
//  第 28 課：deque 的實際應用場景 2  —  有深度上限的 Undo/Redo 系統
// =============================================================================
//
// 【主題資訊 Information】
//   std::deque<std::string> undoStack;   // 歷史紀錄（需要兩端操作）
//   undoStack.push_back(v);   // 新增歷史
//   undoStack.pop_back();     // 撤銷：取回最近的狀態
//   undoStack.pop_front();    // 超過上限：丟棄最舊的  ★ 這是 vector 做不到的
//
//   標頭檔：<deque>、<vector>、<string>
//   複雜度：三個操作皆為 O(1)（vector 的 pop_front 則是 O(n)）
//   本檔標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 undo 歷史必須用 deque 而不是 vector】
//   Undo 歷史需要三種操作：
//       尾端新增（做了新動作）        → vector 可以，O(1)
//       尾端取出（撤銷）              → vector 可以，O(1)
//       **頭端刪除（超過深度上限）**  → vector 是 O(n)，而且沒有 pop_front
//   前兩項 vector 完全勝任，關鍵在第三項。
//   任何有記憶體考量的編輯器都必須限制歷史深度（Photoshop 預設 20 步、
//   VS Code 也有上限），否則長時間編輯會把記憶體吃光。
//   「丟棄最舊的」正是 deque 的主場。
//
// 【2. 為什麼 undo 用 deque、redo 用 vector】
//   注意本檔的設計：undoStack 是 deque，redoStack 是 vector。
//   這不是隨意的——
//       undo 歷史：需要「尾端進出」+「頭端丟棄」→ 必須是 deque
//       redo 堆疊：只需要「尾端進出」，而且它會被 clear()、不會無限成長
//                  → vector 就夠了，而且更快（連續記憶體、快取友善）
//   **選容器要看實際需要哪些操作，不是一律用同一個。**
//   這個對比本身就是本課最實用的一課。
//
// 【3. redo 為什麼一做新動作就要清空】
//   這是 undo/redo 的通用語意，不是實作偷懶。想像：
//       A → B → C，然後 undo 兩次回到 A，再做一個新動作 D。
//       此時 B、C 這條分支已經被 D 取代，「重做」到 B 已經沒有意義。
//   若不清空，redo 會把你帶到一個與目前狀態不連貫的歷史線上。
//   （真正支援分支歷史的編輯器如 Vim 的 undo tree，需要樹狀結構而非堆疊。）
//
// 【4. 這個實作存的是「狀態」還是「動作」】
//   本檔存的是**完整狀態快照**（memento 模式），實作簡單但記憶體用量大。
//   工業級的做法通常存「動作 + 反向動作」（command 模式），
//   例如「在位置 5 插入 abc」的反向是「從位置 5 刪除 3 個字元」。
//   後者記憶體省得多，但每個動作都要寫一對正反操作。
//   選哪個取決於狀態大小：小狀態用快照，大文件用命令。
//
// 【概念補充 Concept Deep Dive】
//   ● 為什麼 currentState 要獨立於兩個堆疊之外
//     undoStack 存的是「**過去**的狀態」，redoStack 存的是「**未來**的狀態」，
//     currentState 是「現在」。三者不重疊。
//     若把現在也塞進 undoStack，每次操作都要多一次 pop/push 來維持不變量，
//     容易寫錯。分開之後，undo 的邏輯就非常乾淨：
//         redo.push(現在) → 現在 = undo.back() → undo.pop_back()
//
//   ● static const int MAX_HISTORY 的初始化
//     class 內的 static const 整數成員可以直接在宣告處給初值（C++98 起）。
//     若要取它的位址或綁定到參考，C++17 之前還需要一個 out-of-class 定義；
//     C++17 起 static constexpr 資料成員隱含 inline，不再需要。
//     本檔只讀取它的值，所以沒有這個問題。
//
//   ● 深度上限的邊界
//     「歷史超過 MAX_HISTORY 就 pop_front」意味著 undoStack 最多有
//     MAX_HISTORY 個元素，也就是最多能 undo MAX_HISTORY 次。
//     本檔設 5，第 6 次操作時最舊的「空白文件」就被丟棄，
//     所以你再也回不到最初狀態——這是有上限的必然代價。
//
// 【注意事項 Pay Attention】
//   1. undo/redo 前務必檢查 empty()；對空容器呼叫 back()/pop_back() 是 UB。
//   2. 執行新動作時必須清空 redo，否則會產生不連貫的歷史線。
//   3. 深度上限會讓最舊的狀態永久遺失——這是刻意的取捨，不是 bug。
//   4. 存完整狀態快照對大型文件很耗記憶體，工業級實作多改存「命令」。
//   5. undo 用 deque（需要 pop_front）、redo 用 vector（不需要）——
//      不要一律用同一個容器。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Undo/Redo 系統設計
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 實作有深度上限的 undo 歷史，為什麼要用 deque 而不是 vector？
//     答：因為需要「超過上限時丟棄**最舊**的紀錄」，也就是 pop_front。
//         vector 沒有 pop_front，自己實作也是 O(n)（要把全部元素往前搬）。
//         deque 的 pop_front 是攤銷 O(1)。
//         至於「新增」與「撤銷」都在尾端，vector 本來就能勝任——
//         關鍵差異只在那個頭端刪除。
//     追問：那 redo 堆疊也要用 deque 嗎？→ 不用。redo 只需要尾端進出，
//         而且會被 clear()、不會無限成長，用 vector 更快也更省。
//
// 🔥 Q2. 為什麼執行新動作時要清空 redo 堆疊？
//     答：因為歷史分支了。假設 A→B→C，undo 兩次回到 A，再做新動作 D。
//         此時 B、C 那條路徑已被 D 取代，「重做到 B」在語意上不再連貫。
//         保留 redo 會讓使用者跳到一個與目前狀態無關的歷史線上。
//     追問：有沒有辦法保留所有分支？→ 有，那需要 **undo tree**（樹狀歷史）
//         而不是兩個堆疊，Vim 和 Emacs 都有這個功能。
//         代價是介面複雜很多——使用者得自己選要走哪個分支。
//
// ⚠️ 陷阱. 「undo 就是把 currentState 丟掉、換成 undoStack 的最後一個」——
//         這樣實作為什麼會壞掉？
//     答：因為你把「現在」直接丟棄了，redo 就再也回不來。
//         正確順序是三步，缺一不可：
//             ① redoStack.push_back(currentState);   // 先把「現在」存進 redo
//             ② currentState = undoStack.back();     // 再取回「過去」
//             ③ undoStack.pop_back();                // 最後才移除
//         少了 ①，undo 之後 redo 就沒東西可回；
//         把 ② ③ 對調（先 pop 再讀 back）則會讀到錯的元素。
//     為什麼會錯：把 undo 想成「刪除一步」，但它其實是
//         「**在三個容器之間搬移狀態**」——過去、現在、未來各有其位置。
//         想清楚不變量（三者不重疊、加起來是完整歷史）才不會寫錯。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <deque>
#include <string>
#include <vector>
using namespace std;

class UndoRedoSystem {
    static const int MAX_HISTORY = 5;  // 最多保留 5 步

    deque<string> undoStack;   // 操作歷史
    vector<string> redoStack;  // 被撤銷的操作
    string currentState;

public:
    UndoRedoSystem(const string& initial) : currentState(initial) {}

    void doAction(const string& action) {
        // 新操作 → 清空 redo
        redoStack.clear();

        // 保存當前狀態到 undo 歷史
        undoStack.push_back(currentState);

        // 如果歷史超過上限，移除最舊的
        if (undoStack.size() > MAX_HISTORY) {
            undoStack.pop_front();  // ← deque 的優勢！
        }

        currentState = action;
        cout << "執行：" << action << endl;
    }

    void undo() {
        if (undoStack.empty()) {
            cout << "無法撤銷！" << endl;
            return;
        }
        // 當前狀態推入 redo
        redoStack.push_back(currentState);
        // 從 undo 歷史取出最近的狀態
        currentState = undoStack.back();
        undoStack.pop_back();
        cout << "撤銷 → 回到：" << currentState << endl;
    }

    void redo() {
        if (redoStack.empty()) {
            cout << "無法重做！" << endl;
            return;
        }
        undoStack.push_back(currentState);
        currentState = redoStack.back();
        redoStack.pop_back();
        cout << "重做 → " << currentState << endl;
    }

    void showState() const {
        cout << "當前狀態：" << currentState
             << "（歷史深度：" << undoStack.size()
             << "，可重做：" << redoStack.size() << "）" << endl;
    }

    // 查詢是否還能撤銷 / 重做（呼叫 undo/redo 前先問，避免無謂操作）
    bool canUndo() const { return !undoStack.empty(); }
    bool canRedo() const { return !redoStack.empty(); }
};


// 註：本檔不附 LeetCode 範例。Undo/Redo 是應用層的設計模式（memento/command），
//     LeetCode 沒有對應題型——最接近的 155. Min Stack 考的是「輔助堆疊」，
//     與這裡的「三容器狀態搬移」不是同一件事，已在第 27 課示範過。
//     硬掛一題只會模糊本檔重點。

int main() {
    UndoRedoSystem editor("空白文件");
    editor.showState();

    editor.doAction("輸入 Hello");
    editor.doAction("輸入 World");
    editor.doAction("設定粗體");
    editor.doAction("改字體大小");
    editor.doAction("加入圖片");
    editor.doAction("調整排版");   // 第 6 步，最舊的「空白文件」會被移除
    editor.showState();

    editor.undo();   // 回到「加入圖片」
    editor.undo();   // 回到「改字體大小」
    editor.redo();   // 重做「加入圖片」
    editor.showState();

    // ========================================================================
    // 深度上限的代價：最舊的狀態已經永久遺失
    // ========================================================================
    cout << "\n=== 深度上限的代價 ===" << endl;
    cout << "MAX_HISTORY = 5，第 6 次操作時最舊的「空白文件」已被 pop_front 丟棄。" << endl;
    cout << "所以不論按幾次 undo，都回不到最初狀態：" << endl;
    int steps = 0;
    while (editor.canUndo()) { editor.undo(); ++steps; }
    cout << "連續 undo " << steps << " 次後：" << endl;
    editor.showState();
    editor.undo();   // 已經沒有東西可以撤銷
    cout << "→ 這是「有上限」的必然代價，不是 bug。" << endl;
    cout << "  Photoshop 預設 20 步、VS Code 也有上限，都是同樣的取捨。" << endl;

    // ========================================================================
    // 新動作會清空 redo（歷史分支）
    // ========================================================================
    cout << "\n=== 新動作會清空 redo（歷史分支）===" << endl;
    UndoRedoSystem doc("草稿");
    doc.doAction("寫第一段");
    doc.doAction("寫第二段");
    doc.doAction("寫第三段");
    cout << "現在：" << endl; doc.showState();

    doc.undo();   // 回到「寫第二段」
    doc.undo();   // 回到「寫第一段」
    cout << "undo 兩次後，redo 可用嗎？ " << boolalpha << doc.canRedo() << endl;

    doc.doAction("改寫成新版本");     // ← 執行新動作
    cout << "執行新動作後，redo 還可用嗎？ " << doc.canRedo() << endl;
    cout << "→ 因為歷史分支了：「寫第二段/第三段」那條路徑已被新動作取代，" << endl;
    cout << "  重做到它們在語意上不再連貫，所以必須清空。" << endl;
    cout << "  想保留所有分支需要 undo tree（樹狀歷史），不是兩個堆疊。" << endl;

    // ========================================================================
    // 為什麼 undo 用 deque、redo 用 vector
    // ========================================================================
    cout << "\n=== 容器選擇：undo 用 deque、redo 用 vector ===" << endl;
    cout << "undo 歷史需要：尾端新增 + 尾端取出 + **頭端丟棄**（超過上限）" << endl;
    cout << "               → 頭端丟棄只有 deque 是 O(1)，vector 連 pop_front 都沒有" << endl;
    cout << "redo 堆疊需要：尾端新增 + 尾端取出（而且會被 clear、不會無限成長）" << endl;
    cout << "               → vector 就夠了，而且連續記憶體、快取更友善" << endl;
    cout << "→ 選容器要看實際需要哪些操作，不是一律用同一個。" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 28 課：deque 的實際應用場景2.cpp" -o demo28_2

// === 預期輸出 ===
// 當前狀態：空白文件（歷史深度：0，可重做：0）
// 執行：輸入 Hello
// 執行：輸入 World
// 執行：設定粗體
// 執行：改字體大小
// 執行：加入圖片
// 執行：調整排版
// 當前狀態：調整排版（歷史深度：5，可重做：0）
// 撤銷 → 回到：加入圖片
// 撤銷 → 回到：改字體大小
// 重做 → 加入圖片
// 當前狀態：加入圖片（歷史深度：4，可重做：1）
//
// === 深度上限的代價 ===
// MAX_HISTORY = 5，第 6 次操作時最舊的「空白文件」已被 pop_front 丟棄。
// 所以不論按幾次 undo，都回不到最初狀態：
// 撤銷 → 回到：改字體大小
// 撤銷 → 回到：設定粗體
// 撤銷 → 回到：輸入 World
// 撤銷 → 回到：輸入 Hello
// 連續 undo 4 次後：
// 當前狀態：輸入 Hello（歷史深度：0，可重做：5）
// 無法撤銷！
// → 這是「有上限」的必然代價，不是 bug。
//   Photoshop 預設 20 步、VS Code 也有上限，都是同樣的取捨。
//
// === 新動作會清空 redo（歷史分支）===
// 執行：寫第一段
// 執行：寫第二段
// 執行：寫第三段
// 現在：
// 當前狀態：寫第三段（歷史深度：3，可重做：0）
// 撤銷 → 回到：寫第二段
// 撤銷 → 回到：寫第一段
// undo 兩次後，redo 可用嗎？ true
// 執行：改寫成新版本
// 執行新動作後，redo 還可用嗎？ false
// → 因為歷史分支了：「寫第二段/第三段」那條路徑已被新動作取代，
//   重做到它們在語意上不再連貫，所以必須清空。
//   想保留所有分支需要 undo tree（樹狀歷史），不是兩個堆疊。
//
// === 容器選擇：undo 用 deque、redo 用 vector ===
// undo 歷史需要：尾端新增 + 尾端取出 + **頭端丟棄**（超過上限）
//                → 頭端丟棄只有 deque 是 O(1)，vector 連 pop_front 都沒有
// redo 堆疊需要：尾端新增 + 尾端取出（而且會被 clear、不會無限成長）
//                → vector 就夠了，而且連續記憶體、快取更友善
// → 選容器要看實際需要哪些操作，不是一律用同一個。
