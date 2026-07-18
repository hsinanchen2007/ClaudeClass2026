// ============================================================
// 第 2.7 章 總結：完美轉發（Perfect Forwarding）— 泛型程式設計的關鍵技術
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【完美轉發公式】
//   template<typename... Args>
//   void wrapper(Args&&... args) {
//       target(std::forward<Args>(args)...);
//   }
//   T&& + std::forward<T> = 保持原始值類別
//
// 【實際應用場景】
//   1. 泛型工廠：make<T>(args...) → 完美轉發到建構子
//   2. 完美轉發建構子：避免 2^n 個建構子組合
//   3. 日誌/計時/重試包裝器
//   4. 成員函式轉發器
//   5. 事件系統：emit<Event>(args...) → 直接建構事件
//
// 【完美轉發的五大陷阱】
//   1. 模板建構子搶走複製/移動建構子
//      → 解法：enable_if 排除自身型別
//   2. braced-init-list {1,2,3} 無法推導型別
//      → 解法：明確建構 vector<int>{1,2,3}
//   3. 0 和 NULL 不能當指標轉發
//      → 解法：用 nullptr
//   4. 靜態 const 整數成員可能連結錯誤
//      → 解法：用 constexpr（C++17 隱含 inline）
//   5. 重載函式無法推導
//      → 解法：static_cast 或 lambda 包裝
//   6. 位元欄位不能取址
//      → 解法：先複製到普通變數
// ============================================================

#include <iostream>
#include <string>
#include <utility>
#include <memory>
#include <vector>
#include <functional>
#include <type_traits>
#include <unordered_map>
#include <typeindex>

// ============================================================
// 1. 泛型工廠函式
// ============================================================
class Connection {
    std::string host_;
    int port_;
    std::string protocol_;
public:
    template<typename H, typename P>
    Connection(H&& h, int p, P&& proto)
        : host_(std::forward<H>(h)), port_(p), protocol_(std::forward<P>(proto)) {
        std::cout << "  Connection(" << host_ << ":" << port_
                  << ", " << protocol_ << ")\n";
    }
};

template<typename T, typename... Args>
std::unique_ptr<T> make(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// ============================================================
// 2. 完美轉發建構子（避免 2^n 個版本）
// ============================================================
class Logger {
    std::string prefix_;
public:
    Logger(const std::string& p) : prefix_(p) { std::cout << "    Logger 複製\n"; }
    Logger(std::string&& p) : prefix_(std::move(p)) { std::cout << "    Logger 移動\n"; }
    void log(const std::string& msg) const { std::cout << "    [" << prefix_ << "] " << msg << "\n"; }
};

class Service {
    Logger logger_;
    std::vector<int> data_;
public:
    // 一個模板建構子取代 4 個（2 參數 × 左右值 = 4 種組合）
    template<typename S, typename V>
    Service(S&& prefix, V&& data)
        : logger_(std::forward<S>(prefix))
        , data_(std::forward<V>(data)) {}
    void run() const { logger_.log("Service running"); }
};

// ============================================================
// 3. enable_if 解決模板搶走複製建構子
// ============================================================
class Widget {
    std::string name_;
public:
    // 用 enable_if 排除 Widget 型別本身
    template<typename T,
             std::enable_if_t<!std::is_same_v<std::decay_t<T>, Widget>, int> = 0>
    Widget(T&& name) : name_(std::forward<T>(name)) {
        std::cout << "  Widget 模板建構子\n";
    }

    Widget(const Widget& o) : name_(o.name_) { std::cout << "  Widget 複製建構子\n"; }
    Widget(Widget&& o) noexcept : name_(std::move(o.name_)) { std::cout << "  Widget 移動建構子\n"; }

    const std::string& name() const { return name_; }
};

// ============================================================
// 4. 日誌包裝器
// ============================================================
template<typename Func, typename... Args>
auto logged_call(const std::string& tag, Func&& func, Args&&... args)
    -> decltype(std::forward<Func>(func)(std::forward<Args>(args)...))
{
    std::cout << "  [LOG] " << tag << " 開始\n";
    auto result = std::forward<Func>(func)(std::forward<Args>(args)...);
    std::cout << "  [LOG] " << tag << " 結束\n";
    return result;
}

// ============================================================
// 5. 多層轉發
// ============================================================
void final_fn(const std::string& s) { std::cout << "  最終: 左值\n"; }
void final_fn(std::string&& s)      { std::cout << "  最終: 右值\n"; }

template<typename T>
void layer3(T&& a) { final_fn(std::forward<T>(a)); }
template<typename T>
void layer2(T&& a) { layer3(std::forward<T>(a)); }
template<typename T>
void layer1(T&& a) { layer2(std::forward<T>(a)); }

// ============================================================
// 6. 重載函式轉發陷阱
// ============================================================
void process(int x)    { std::cout << "  process(int): " << x << "\n"; }
void process(double x) { std::cout << "  process(double): " << x << "\n"; }

template<typename T>
void fwd(T&& arg) { std::cout << "  forwarded\n"; }

int main() {
    // ============================================================
    // 1. 泛型工廠
    // ============================================================
    std::cout << "===== 1. 泛型工廠 =====\n";
    std::string host = "example.com";
    auto c1 = make<Connection>(host, 8080, std::string("https"));  // 混合左右值
    auto c2 = make<Connection>(std::string("localhost"), 3000, std::string("http"));
    std::cout << "\n";

    // ============================================================
    // 2. 完美轉發建構子
    // ============================================================
    std::cout << "===== 2. 完美轉發建構子 =====\n";
    std::string name = "MyApp";
    std::vector<int> nums = {1, 2, 3};

    std::cout << "  全部左值:\n";
    Service s1(name, nums);

    std::cout << "  全部右值:\n";
    Service s2(std::string("Worker"), std::vector<int>{10, 20, 30});
    std::cout << "\n";

    // ============================================================
    // 3. enable_if 防止模板搶走複製建構子
    // ============================================================
    std::cout << "===== 3. enable_if =====\n";
    Widget w1("Hello");        // 模板建構子 ✅
    Widget w2(w1);             // 複製建構子 ✅（不被模板搶走）
    Widget w3(std::move(w1));  // 移動建構子 ✅
    std::cout << "\n";

    // ============================================================
    // 4. 多層轉發
    // ============================================================
    std::cout << "===== 4. 多層轉發（穿越三層）=====\n";
    std::string s = "test";
    std::cout << "  傳左值:\n";
    layer1(s);
    std::cout << "  傳右值:\n";
    layer1(std::string("tmp"));
    std::cout << "\n";

    // ============================================================
    // 5. 日誌包裝器
    // ============================================================
    std::cout << "===== 5. 日誌包裝器 =====\n";
    auto result = logged_call("add",
        [](int a, int b) { return a + b; }, 10, 20);
    std::cout << "  結果: " << result << "\n\n";

    // ============================================================
    // 6. 五大陷阱
    // ============================================================
    std::cout << "===== 6. 完美轉發的陷阱 =====\n";

    // 陷阱 2：{1,2,3} 不能推導
    // fwd({1,2,3});  // ❌ 編譯錯誤
    fwd(std::vector<int>{1, 2, 3});  // ✅ 明確建構
    std::cout << "  {1,2,3} 無法推導 → 需要明確建構\n";

    // 陷阱 3：0/NULL 不能當指標轉發
    // fwd(NULL);  // 可能錯誤
    fwd(nullptr);  // ✅
    std::cout << "  用 nullptr 取代 0/NULL\n";

    // 陷阱 5：重載函式不能推導
    // fwd(process);  // ❌ 哪個 process？
    fwd(static_cast<void(*)(int)>(process));  // ✅ 指定版本
    std::cout << "  重載函式需要 static_cast 指定版本\n";

    // 陷阱 6：位元欄位不能取址
    struct Flags { unsigned int readable : 1; unsigned int writable : 1; };
    Flags f = {1, 1};
    // fwd(f.readable);  // ❌ 位元欄位不能取址
    unsigned int r = f.readable;
    fwd(r);  // ✅ 先複製到普通變數
    std::cout << "  位元欄位需先複製到變數\n";

    std::cout << "\n=== 重點整理 ===\n";
    std::cout << "  公式：template<typename T> void f(T&& a) { g(forward<T>(a)); }\n";
    std::cout << "  應用：工廠、建構子、包裝器、emplace_back\n";
    std::cout << "  陷阱：模板搶建構子、{} 推導、0/NULL、重載函式、位元欄位\n";

    return 0;
}
