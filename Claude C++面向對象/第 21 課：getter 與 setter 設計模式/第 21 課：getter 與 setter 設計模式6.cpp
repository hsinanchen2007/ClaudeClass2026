#include <iostream>
#include <string>
using namespace std;

class DialogBox {
private:
    string title_;
    string message_;
    int width_;
    int height_;
    bool visible_;

public:
    DialogBox()
        : title_("未命名"), message_(""), width_(200), height_(100)
        , visible_(false)
    {
    }

    // setter 返回自身引用，支持鏈式調用
    // 注意：這種設計允許在一行中連續調用多個 setter，提升代碼的流暢性和可讀性
    // 例如：dlg.setTitle("警告").setMessage("確定要刪除嗎？").setSize(400, 200).show();
    // 注意：這些 setter 都返回 DialogBox&，允許鏈式調用
    // 注意：這些 setter 都帶有驗證邏輯，確保對象保持有效狀態
    DialogBox& setTitle(const string& title) {
        title_ = title;
        return *this;       // 返回自身, 支持鏈式調用
    }

    DialogBox& setMessage(const string& msg) {
        message_ = msg;
        return *this;       // 返回自身, 支持鏈式調用
    }

    DialogBox& setSize(int w, int h) {
        width_ = (w > 0) ? w : 200;
        height_ = (h > 0) ? h : 100;
        return *this;       // 返回自身, 支持鏈式調用
    }

    DialogBox& show() {
        visible_ = true;
        return *this;       // 返回自身, 支持鏈式調用
    }

    void print() const {
        cout << "  ┌";
        for (int i = 0; i < 30; i++) cout << "─";
        cout << "┐" << endl;
        cout << "  │ [" << title_ << "]" << endl;
        cout << "  │ " << message_ << endl;
        cout << "  │ Size: " << width_ << "x" << height_ << endl;
        cout << "  │ Visible: " << (visible_ ? "Yes" : "No") << endl;
        cout << "  └";
        for (int i = 0; i < 30; i++) cout << "─";
        cout << "┘" << endl;
    }
};

int main() {
    cout << "=== 鏈式調用 ===" << endl;

    // 傳統方式：一行一行設定
    cout << "\n--- 傳統方式 ---" << endl;
    DialogBox dlg1;
    dlg1.setTitle("警告");
    dlg1.setMessage("你確定要刪除嗎？");
    dlg1.setSize(400, 200);
    dlg1.show();
    dlg1.print();

    // 鏈式調用：一氣呵成
    cout << "\n--- 鏈式調用 ---" << endl;
    DialogBox dlg2;
    dlg2.setTitle("歡迎")
        .setMessage("歡迎回到遊戲世界！")
        .setSize(500, 250)
        .show();
    dlg2.print();

    return 0;
}
