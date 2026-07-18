#include <iostream>
#include <string>
using namespace std;

class QueryBuilder {
private:
    string table_;
    string conditions_;
    string orderBy_;
    int limit_;

public:
    QueryBuilder() : limit_(0) {}

    // 每個函數返回 *this（自身的引用）
    QueryBuilder& from(const string& table) {
        table_ = table;
        return *this;    // *this 解引用 this 指標，得到對象本身
    }

    QueryBuilder& where(const string& condition) {
        if (!conditions_.empty()) conditions_ += " AND ";
        conditions_ += condition;
        return *this;
    }

    QueryBuilder& orderBy(const string& field) {
        orderBy_ = field;
        return *this;
    }

    QueryBuilder& limit(int n) {
        limit_ = (n > 0) ? n : 0;
        return *this;
    }

    string build() const {
        string sql = "SELECT * FROM " + table_;
        if (!conditions_.empty()) sql += " WHERE " + conditions_;
        if (!orderBy_.empty()) sql += " ORDER BY " + orderBy_;
        if (limit_ > 0) sql += " LIMIT " + to_string(limit_);
        return sql;
    }
};

int main() {
    cout << "=== 場景二：鏈式調用 ===" << endl;

    // 鏈式調用的原理：
    // query.from("players")  → 返回 query 自身的引用
    //       .where("lv > 10")→ 在返回的引用上繼續調用 → 返回 query 引用
    //       .orderBy("lv")   → 繼續...
    //       .limit(20)       → 繼續...
    //       .build()         → 最後產出 SQL

    string sql = QueryBuilder()
        .from("players")
        .where("level > 10")
        .where("hp > 0")
        .orderBy("level DESC")
        .limit(20)
        .build();

    cout << "  " << sql << endl;

    cout << "\n--- 另一個查詢 ---" << endl;
    string sql2 = QueryBuilder()
        .from("items")
        .where("rarity = 'legendary'")
        .where("price < 10000")
        .limit(5)
        .build();

    cout << "  " << sql2 << endl;

    return 0;
}
