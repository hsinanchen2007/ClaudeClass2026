#include <iostream>
#include <string>
using namespace std;

class GameManager {
private:
    int score_;
    int level_;
    string currentMap_;

    // 私有建構函數：外部不能創建
    // 這個類別只能有一個實例，通過靜態函數 getInstance() 獲取
    // 這是一種常見的設計模式，稱為 "singleton"（單例模式）
    // 在第一次調用 getInstance() 時，靜態局部變數 instance 會被創建並初始化
    // 之後每次調用 getInstance() 都會返回同一個 instance，確保全局只有一個 GameManager 對象
    // 單例模式適合用於管理遊戲狀態、配置、資源等全局共享的對象
    // 在遊戲開發中，GameManager 通常負責管理遊戲的整體狀態，例如分數、關卡、當前地圖等
    // 這個類別的設計確保了遊戲中只有一個 GameManager 實例，避免了多個實例之間的狀態不一致問題
    // 單例模式的實現方式有很多種，這裡使用了 C++11 以後的 "Meyers' Singleton" 實現方式，利用了靜態局部變數的特性來確保線程安全和唯一性
    // 在 C++11 以後，靜態局部變數的初始化是線程安全的，因此這種實現方式不需要額外的鎖來保護 instance 的創建
    // 這個建構函數是私有的，外部無法直接創建 GameManager 對象
    GameManager() : score_(0), level_(1), currentMap_("新手村") {
        cout << "  [GameManager 初始化]" << endl;
    }

public:
    // 禁止拷貝
    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;

    // 靜態函數：獲取唯一實例
    // 這個函數會返回 GameManager 的唯一實例，確保全局只有一個 GameManager 對象
    // 這個函數是靜態的，因為它不依賴於任何實例，可以直接通過類別名調用
    // 第一次調用 getInstance() 時，靜態局部變數 instance 會被創建並初始化
    // 之後每次調用 getInstance() 都會返回同一個 instance，確保全局只有一個 GameManager 對象
    // 單例模式適合用於管理遊戲狀態、配置、資源等全局共享的對象
    static GameManager& getInstance() {
        static GameManager instance;   // 靜態局部變數，只初始化一次
        return instance;
    }

    // 普通成員函數
    // 這些函數操作 GameManager 的狀態，例如增加分數、進入下一關、設置當前地圖等
    // 這些函數不是靜態的，因為它們需要操作 GameManager 的實例變數
    // 這些函數可以通過 getInstance() 獲取的實例來調用，例如 GameManager::getInstance().addScore(100);
    // 這些函數提供了對 GameManager 狀態的操作接口，讓遊戲邏輯可以通過這些函數來修改遊戲狀態，例如增加分數、進入下一關、設置當前地圖等
    // 這些函數的實現非常簡單，主要是修改 GameManager 的成員變數並輸出當前狀態的訊息
    void addScore(int points) {
        score_ += points;
        cout << "  得分 +" << points << " (總分:" << score_ << ")" << endl;
    }

    void nextLevel() {
        level_++;
        cout << "  進入第 " << level_ << " 關" << endl;
    }

    void setMap(const string& map) { currentMap_ = map; }

    void printStatus() const {
        cout << "  [遊戲狀態] 地圖:" << currentMap_
             << " 關卡:" << level_ << " 分數:" << score_ << endl;
    }
};

int main() {
    cout << "=== 單例存取點（靜態函數）===" << endl;

    // 通過靜態函數獲取唯一實例
    cout << "\n--- 第一次存取 ---" << endl;
    GameManager::getInstance().printStatus();

    cout << "\n--- 遊戲進行 ---" << endl;
    GameManager::getInstance().addScore(100);
    GameManager::getInstance().addScore(250);
    GameManager::getInstance().nextLevel();
    GameManager::getInstance().setMap("暗黑森林");

    cout << "\n--- 查看狀態 ---" << endl;
    GameManager::getInstance().printStatus();

    // 驗證是同一個實例
    cout << "\n--- 驗證唯一性 ---" << endl;
    GameManager& ref1 = GameManager::getInstance();
    GameManager& ref2 = GameManager::getInstance();
    cout << "  ref1 地址：" << &ref1 << endl;
    cout << "  ref2 地址：" << &ref2 << endl;
    cout << "  是同一個對象：" << (&ref1 == &ref2 ? "是" : "否") << endl;

    return 0;
}
