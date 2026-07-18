#include <iostream>
#include <thread>
#include <mutex>

class Database {
public:
    Database() { std::cout << "Database 初始化" << std::endl; }
    void query() { std::cout << "查詢中..." << std::endl; }
};

Database* db = nullptr;
std::once_flag initFlag;

void initDatabase() {
    db = new Database();
}

void initAndUse() {
    std::call_once(initFlag, initDatabase);
    db->query();
}

int main() {
    std::thread t1(initAndUse);
    std::thread t2(initAndUse);
    std::thread t3(initAndUse);
    
    t1.join();
    t2.join();
    t3.join();
    
    delete db;
    return 0;
}
