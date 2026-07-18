#include <iostream>
#include <string>
using namespace std;

class Config {
private:
    int maxConnections;
public:
    Config() : maxConnections(100) {
        cout << "  [Config] 初始化" << endl;
    }
    int getMax() const { return maxConnections; }
};

class ConnectionPool {
private:
    int poolSize;
public:
    ConnectionPool(int size) : poolSize(size) {
        cout << "  [ConnectionPool] 初始化: " << poolSize << endl;
    }
    int getMax() const { return poolSize; }
};

// 用函數包裝，保證初始化順序
Config& getConfig() {
    static Config config;    // 第一次調用時建構
    return config;
}

ConnectionPool& getPool() {
    // getConfig() 保證在使用前就已初始化
    static ConnectionPool pool(getConfig().getMax());
    return pool;
}

int main() {
    cout << "=== 安全的初始化順序 ===" << endl;
    cout << "  Pool size = " << getPool().getMax() << endl;  
    // 假設 ConnectionPool 也有 getMax() 的話
    
    getPool();   // 第二次調用，不會重複初始化
    return 0;
}
// 這段程式碼展示了如何使用函數包裝全域對象來確保初始化順序：
// - Config 和 ConnectionPool 都是全域對象，但它們被包裝在函數中，使用 static 局部變數來確保它們在第一次調用時才被初始化。
// - getConfig() 會在第一次調用時建構 Config 對象，然後 getPool() 會在第一次調用時建構 ConnectionPool 對象，
// 並且依賴於已經初始化的 Config 對象，這樣就避免了初始化順序的問題。
