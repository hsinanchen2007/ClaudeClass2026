#include <iostream>
#include <string>
using namespace std;

class Logger {
public:
    string name = "APP";

    // level 有預設值 "INFO", 這裡的 level 參數有一個預設值 "INFO"，這意味著如果在調用 log 函數時沒有提供 
    // level 參數，則會自動使用 "INFO" 作為默認值。
    void log(const string& message, const string& level = "INFO") {
        cout << "[" << level << "] " << name << ": " << message << endl;
    }

    // repeat 有預設值 1, ch 有預設值 '-', 這裡的 divider 函數有兩個參數 ch 和 repeat，分別有預設值 '-' 
    // 和 40。這意味著如果在調用 divider 函數時沒有提供這些參數，則會使用這些預設值。
    void divider(char ch = '-', int repeat = 40) {
        for (int i = 0; i < repeat; i++) {
            cout << ch;
        }
        cout << endl;
    }
};

int main() {
    Logger logger;
    logger.name = "MyApp";

    logger.divider();                  // 使用全部預設值
    logger.log("程式啟動");            // level 使用預設值 "INFO"
    logger.log("連線失敗", "ERROR");   // 指定 level
    logger.log("嘗試重連", "WARN");
    logger.divider('=', 40);           // 指定 ch，使用預設 repeat
    logger.log("重連成功");
    logger.divider('*', 20);           // 全部指定

    return 0;
}
