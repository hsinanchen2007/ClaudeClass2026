#include <iostream>
#include <string>
#include <vector>

class Entry {
    std::string key_;
    int value_;
public:
    Entry(const std::string& k, int v) : key_(k), value_(v) {
        std::cout << "  Entry(const string&, int)\n";
    }
    Entry(std::string&& k, int v) : key_(std::move(k)), value_(v) {
        std::cout << "  Entry(string&&, int)\n";
    }
};

int main() {
    std::vector<Entry> vec;
    vec.reserve(4);

    std::string key = "alpha";

    std::cout << "--- push_back（必須先建構，再複製或移動進容器）---\n";
    vec.push_back(Entry(key, 1));           // 先建構臨時 Entry，再移動
    vec.push_back(Entry("beta", 2));        // 同上

    std::cout << "\n--- emplace_back（直接在容器內建構）---\n";
    vec.emplace_back(key, 3);               // 完美轉發 → Entry(const string&, int)
    vec.emplace_back(std::string("delta"), 4); // 完美轉發 → Entry(string&&, int)

    return 0;
}
