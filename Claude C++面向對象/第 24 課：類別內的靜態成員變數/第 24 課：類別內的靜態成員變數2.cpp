#include <iostream>
#include <string>
using namespace std;

class GameConfig {
public:
    // C++17 inline static：直接在類別內定義並初始化
    // 靜態成員變數屬於類別本身，而不是任何特定的對象
    inline static int maxPlayers = 4;
    inline static double gravity = 9.8;
    inline static string version = "1.0.3";

    // const static 整數型別：即使 C++11 也可以類別內初始化
    // 這些常量用於遊戲配置，不會改變
    static const int MAX_LEVEL = 100;
    static const int MAX_ITEMS = 999;

    // C++17 inline static const 也可以用於非整數型別
    // 這裡我們定義了一個遊戲名稱常量，直接在類別內初始化
    inline static const string GAME_NAME = "冒險世界";

    static void printConfig() {
        cout << "  遊戲：" << GAME_NAME << " v" << version << endl;
        cout << "  最大玩家數：" << maxPlayers << endl;
        cout << "  重力：" << gravity << endl;
        cout << "  最大等級：" << MAX_LEVEL << endl;
        cout << "  最大物品：" << MAX_ITEMS << endl;
    }
};

// 不需要類別外定義了！（C++17）

int main() {
    cout << "=== C++17 inline static ===" << endl;

    GameConfig::printConfig();

    // 可以修改非 const 的靜態成員
    cout << "\n--- 修改配置 ---" << endl;
    GameConfig::maxPlayers = 8;
    GameConfig::gravity = 15.0;
    GameConfig::version = "2.0.0";
    GameConfig::printConfig();

    // GameConfig::MAX_LEVEL = 200;  // ❌ 編譯錯誤！const

    return 0;
}
