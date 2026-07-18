#include <iostream>
#include <string>
using namespace std;

class Logger {
private:
    ostream& output;  // 引用成員：必須在初始化時綁定
    string prefix;

public:
    // 引用必須在宣告時綁定，所以必須用初始化列表
    // 初始化列表的語法：冒號後面跟著成員變數和對應的初始值
    // 注意：初始化列表的語法是冒號後面跟著成員變數和對應的初始值
    // 優點：效率更高，特別是對於引用成員來說，必須使用初始化列表來初始化，因為它們不能在函數體內賦值
    // Logger(ostream& os, const string& p) {
    //     output = os;  // 編譯錯誤！引用成員必須在初始化列表中初始化！
    //     prefix = p;
    // }
    Logger(ostream& os, const string& p) 
        : output(os), prefix(p) 
    { }
    
    void log(const string& message) const {
        output << "[" << prefix << "] " << message << endl;
    }
};

int main() {
    Logger consoleLogger(cout, "INFO");
    consoleLogger.log("程式啟動");
    consoleLogger.log("初始化完成");
    
    return 0;
}
