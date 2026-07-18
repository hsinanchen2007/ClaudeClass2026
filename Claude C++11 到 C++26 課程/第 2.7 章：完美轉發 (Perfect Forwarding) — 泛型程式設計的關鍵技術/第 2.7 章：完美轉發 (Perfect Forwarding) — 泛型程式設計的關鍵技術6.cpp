#include <iostream>
#include <string>
#include <utility>
#include <functional>

class Database {
public:
    std::string query(const std::string& sql) {
        std::cout << "  DB::query(const string&)\n";
        return "result of: " + sql;
    }

    void insert(std::string&& data) {
        std::cout << "  DB::insert(string&&): " << data << "\n";
    }

    void update(const std::string& key, std::string&& value) {
        std::cout << "  DB::update(\"" << key << "\", \"" << value << "\")\n";
    }
};

// 泛型的成員函式呼叫轉發器
template<typename Obj, typename MemFunc, typename... Args>
auto invoke_member(Obj&& obj, MemFunc func, Args&&... args)
    -> decltype((std::forward<Obj>(obj).*func)(std::forward<Args>(args)...))
{
    return (std::forward<Obj>(obj).*func)(std::forward<Args>(args)...);
}

int main() {
    Database db;
    std::string sql = "SELECT * FROM users";

    auto result = invoke_member(db, &Database::query, sql);
    std::cout << "  " << result << "\n\n";

    invoke_member(db, &Database::insert, std::string("new record"));

    std::cout << "\n";
    std::string key = "user_1";
    invoke_member(db, &Database::update, key, std::string("updated value"));

    return 0;
}
