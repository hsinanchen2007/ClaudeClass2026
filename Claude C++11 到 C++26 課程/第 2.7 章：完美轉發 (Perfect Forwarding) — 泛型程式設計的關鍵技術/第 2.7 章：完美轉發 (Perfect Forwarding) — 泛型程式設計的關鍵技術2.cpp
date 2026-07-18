#include <iostream>
#include <string>
#include <vector>
#include <utility>

class Logger {
    std::string prefix_;

public:
    Logger(const std::string& p) : prefix_(p) {
        std::cout << "  Logger: 複製 prefix\n";
    }
    Logger(std::string&& p) : prefix_(std::move(p)) {
        std::cout << "  Logger: 移動 prefix\n";
    }

    void log(const std::string& msg) const {
        std::cout << "  [" << prefix_ << "] " << msg << "\n";
    }
};

class Service {
    Logger logger_;
    std::vector<int> data_;

public:
    // 不用完美轉發：需要寫多個建構子
    // Service(const std::string& prefix, const std::vector<int>& data)
    //     : logger_(prefix), data_(data) {}
    // Service(std::string&& prefix, std::vector<int>&& data)
    //     : logger_(std::move(prefix)), data_(std::move(data)) {}
    // Service(const std::string& prefix, std::vector<int>&& data) ...
    // Service(std::string&& prefix, const std::vector<int>& data) ...
    // → 4 個建構子！

    // 用完美轉發：一個搞定
    template<typename S, typename V>
    Service(S&& prefix, V&& data)
        : logger_(std::forward<S>(prefix))
        , data_(std::forward<V>(data))
    {}

    void run() const {
        logger_.log("Service started with " +
                    std::to_string(data_.size()) + " items");
    }
};

int main() {
    std::string name = "MyApp";
    std::vector<int> nums = {1, 2, 3, 4, 5};

    std::cout << "--- 全部左值 ---\n";
    Service s1(name, nums);
    s1.run();

    std::cout << "\n--- 全部右值 ---\n";
    Service s2(std::string("Worker"), std::vector<int>{10, 20, 30});
    s2.run();

    std::cout << "\n--- 混合 ---\n";
    Service s3(name, std::vector<int>{100, 200});
    s3.run();

    return 0;
}
