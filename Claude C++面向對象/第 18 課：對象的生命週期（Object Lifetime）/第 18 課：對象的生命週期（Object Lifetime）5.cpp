#include <iostream>
#include <string>
using namespace std;

// === 檔案 A 中（概念上）===
class Config {
private:
    int maxConnections;
public:
    Config() : maxConnections(100) {
        cout << "  [Config] 初始化: maxConnections = " 
             << maxConnections << endl;
    }
    int getMax() const { return maxConnections; }
};

// === 檔案 B 中（概念上）===
class ConnectionPool {
private:
    int poolSize;
public:
    // 危險！這裡依賴 config 已經被初始化
    ConnectionPool(const Config& cfg) : poolSize(cfg.getMax()) {
        cout << "  [ConnectionPool] 初始化: poolSize = " 
             << poolSize << endl;
    }
};

// 全域對象
Config config;                    // 在這個檔案中先宣告
ConnectionPool pool(config);      // 依賴 config

int main() {
    cout << "\n=== 程式開始 ===" << endl;
    cout << "  在同一個編譯單元中，順序是確定的" << endl;
    cout << "  但如果 config 和 pool 在不同的 .cpp 檔案中..." << endl;
    cout << "  初始化順序就是未定義的！" << endl;
    return 0;
}
// 這段程式碼展示了全域對象初始化順序的問題：
// - Config 和 ConnectionPool 是全域對象，且 ConnectionPool 依賴 Config。
// - 如果它們在同一個 .cpp 檔案中，則 Config 會先被初始化，然後 ConnectionPool 會使用它，這是安全的。
// - 但是，如果 Config 和 ConnectionPool 分別在不同的 .cpp 檔案中，則它們的初始化順序是未定義的，
// 可能會導致 ConnectionPool 在 Config 之前被初始化，從而使用未初始化的 Config，造成未定義行為。  