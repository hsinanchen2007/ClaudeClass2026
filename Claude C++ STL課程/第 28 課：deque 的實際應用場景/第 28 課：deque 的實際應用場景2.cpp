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
             << "（歷史深度：" << undoStack.size() << "）" << endl;
    }
};

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

    return 0;
}
