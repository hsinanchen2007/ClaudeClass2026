#include <iostream>
#include <string>
using namespace std;

class GameConfig {
private:
    // C++11：直接在宣告時給預設值
    // 優點：簡潔明瞭，適合大多數情況，特別是當預設值對所有建構函數都適用時
    int screenWidth = 1280;
    int screenHeight = 720;
    bool fullscreen = false;
    string title = "My Game";
    int fps = 60;

public:
    // 預設建構函數：所有成員使用類別內的預設值
    // 注意：如果定義了其他建構函數，則預設建構函數不會自動生成，所以需要手動定義
    GameConfig() {
        cout << "[預設建構] 使用所有預設值" << endl;
    }
    
    // 部分自定義：初始化列表會覆蓋類別內預設值
    // 優點：靈活，可以只覆蓋需要改變的成員，其他成員仍然使用預設值
    GameConfig(int w, int h) 
        : screenWidth(w), screenHeight(h)  // 只覆蓋寬高
        // fullscreen、title、fps 使用類別內的預設值
    {
        cout << "[部分自定義] 自定義解析度" << endl;
    }
    
    // 完全自定義
    // 優點：完全控制所有成員的初始化，適合需要多種配置的情況
    GameConfig(int w, int h, bool fs, const string& t, int f) 
        : screenWidth(w), screenHeight(h), fullscreen(fs), 
          title(t), fps(f) 
    {
        cout << "[完全自定義] 所有參數指定" << endl;
    }

    void print() const {
        cout << "  遊戲: " << title << endl;
        cout << "  解析度: " << screenWidth << "x" << screenHeight << endl;
        cout << "  全螢幕: " << (fullscreen ? "是" : "否") << endl;
        cout << "  FPS: " << fps << endl;
    }
};

int main() {
    cout << "=== 配置 1：全部預設 ===" << endl;
    GameConfig cfg1;
    cfg1.print();
    
    cout << "\n=== 配置 2：只改解析度 ===" << endl;
    GameConfig cfg2(1920, 1080);
    cfg2.print();
    
    cout << "\n=== 配置 3：全部自定義 ===" << endl;
    GameConfig cfg3(2560, 1440, true, "RPG World", 144);
    cfg3.print();
    
    return 0;
}
