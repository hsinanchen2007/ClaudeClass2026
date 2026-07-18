#include <iostream>
#include <string>
using namespace std;

class DatabaseConnection {
private:
    string host;
    int port;

public:
    // 明確禁止預設建構：沒有連接資訊就不能創建
    // 這裡我們使用 C++11 的 delete 功能，來禁止編譯器自動生成預設建構函數
    // 這樣做的好處是，如果有人試圖創建一個沒有提供連接資訊的 DatabaseConnection 對象，
    // 編譯器會直接報錯，提醒開發者必須提供必要的參數來初始化對象
    // delete 預設建構函數，禁止無參建構函數的使用，強制要求使用帶參建構函數來創建對象
    DatabaseConnection() = delete;
    
    DatabaseConnection(string h, int p) {
        host = h;
        port = p;
        cout << "連接到 " << host << ":" << port << endl;
    }

    void print() const {
        cout << "  資料庫連接: " << host << ":" << port << endl;
    }
};

int main() {
    // DatabaseConnection db;  // 編譯錯誤！預設建構函數被 delete 了
    // error: use of deleted function 'DatabaseConnection::DatabaseConnection()'
    
    DatabaseConnection db("localhost", 5432);  // OK
    db.print();
    
    return 0;
}
