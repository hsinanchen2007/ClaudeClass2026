#include <iostream>
#include <string>
using namespace std;

class Logger {
private:
    string module_;
    inline static int logCount_ = 0;
    inline static bool enabled_ = true;

public:
    Logger(const string& module) : module_(module) {}

    // ====== 靜態函數 ======
    // 靜態函數只能訪問靜態成員，不能訪問非靜態成員（因為它們不屬於任何實例）❌
    // 靜態函數可以修改靜態變數，這些變數對所有實例共享 ✅
    // 這些函數提供了對日誌系統的全局控制，例如啟用/停用日誌、獲取已記錄的日誌數量等
    // 靜態函數可以直接通過類名調用，不需要實例化對象 ✅
    // 靜態函數可以調用其他靜態函數 ✅
    static void enable() { enabled_ = true; }
    static void disable() { enabled_ = false; }
    static bool isEnabled() { return enabled_; }
    static int getLogCount() { return logCount_; }

    // 靜態函數可以調用其他靜態函數 ✅
    // 這個函數用於顯示當前日誌系統的狀態和已記錄的日誌數量
    // 注意：這裡不能訪問 module_，因為它是非靜態成員，與任何特定實例相關聯 ❌
    // 這個函數展示了靜態函數如何提供全局信息，而不依賴於任何特定的 Logger 實例
    // 這裡的輸出會顯示日誌系統是否啟用，以及已經記錄了多少條日誌，這些信息對所有 Logger 實例都是共享的
    static void printSystemInfo() {
        cout << "  [系統] 日誌 " << (isEnabled() ? "啟用" : "停用")
             << "，已記錄 " << getLogCount() << " 條" << endl;
    }

    // ====== 非靜態函數 ======
    // 非靜態函數可以訪問非靜態成員，因為它們屬於特定的實例 ✅
    // 非靜態函數也可以訪問靜態成員，因為靜態成員對所有實例共享 ✅
    // 非靜態函數提供了對特定 Logger 實例的操作，例如記錄消息、顯示警告等
    // 非靜態函數可以直接訪問同一類中的靜態函數和靜態變數，這些成員對所有實例都是共享的 ✅
    // 非靜態函數需要通過實例化對象來調用，因為它們依賴於特定的實例狀態 ✅
    // 非靜態函數可以調用靜態函數 ✅
    void log(const string& message) const {
        if (!isEnabled()) return;    // 調用靜態函數 ✅
        logCount_++;                 // 訪問靜態變數 ✅
        cout << "  [" << module_ << "] " << message   // 訪問非靜態成員 ✅
             << " (#" << logCount_ << ")" << endl;
    }

    void warn(const string& message) const {
        if (!isEnabled()) return;
        logCount_++;
        cout << "  ⚠ [" << module_ << "] " << message
             << " (#" << logCount_ << ")" << endl;
    }
};

int main() {
    cout << "=== 靜態與非靜態的互動 ===" << endl;

    Logger gameLog("遊戲");
    Logger netLog("網路");

    Logger::printSystemInfo();

    cout << "\n--- 正常記錄 ---" << endl;
    gameLog.log("遊戲啟動");
    netLog.log("連接伺服器");
    gameLog.log("載入地圖");
    netLog.warn("延遲過高");

    Logger::printSystemInfo();

    // 關閉日誌
    cout << "\n--- 關閉日誌 ---" << endl;
    Logger::disable();
    gameLog.log("這條不會顯示");
    netLog.log("這條也不會顯示");
    Logger::printSystemInfo();

    // 重新開啟
    cout << "\n--- 重新開啟 ---" << endl;
    Logger::enable();
    gameLog.log("日誌恢復");
    Logger::printSystemInfo();

    return 0;
}
