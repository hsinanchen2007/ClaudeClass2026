#include <iostream>
#include <thread>

class Database {
public:
    Database() { std::cout << "Database 初始化" << std::endl; }
    void query() { std::cout << "查詢中..." << std::endl; }
};

Database* db = nullptr;

void initAndUse() {
    if (db == nullptr) {           // 執行緒 A 檢查
        db = new Database();       // 執行緒 A 和 B 都可能執行這行！
    }
    db->query();
}

int main() {
    std::thread t1(initAndUse);
    std::thread t2(initAndUse);
    
    t1.join();
    t2.join();
    
    delete db;
    return 0;
}
