// =============================================================================
//  summary.cpp  —  第 2.7 章總結：完美轉發的完整工程樣貌與六大陷阱
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<utility>（forward）、<type_traits>（enable_if / decay / is_same）
//   完美轉發公式（C++11 起）：
//       template<class... Args>
//       void wrapper(Args&&... args) { target(std::forward<Args>(args)...); }
//   本檔用到的標準版本標記：
//       std::enable_if_t / std::decay_t / std::is_same_v 是 C++14 起的別名
//       （_t 後綴 C++14、_v 後綴 C++17；C++11 要寫 typename ...::type / ::value）。
//       本檔宣告 -std=c++17，故可直接使用 is_same_v。
//   複雜度：轉發零成本；成本全在被轉發到的建構子／函式本身。
//
// 【詳細解釋 Explanation】
//
// 【1. 完美轉發真正解決的工程問題：多載組合爆炸】
//   一個接受 n 個參數的函式，若要對每個參數分別支援左值與右值，
//   手寫多載需要 2^n 個版本。兩個參數要 4 個、三個參數要 8 個。
//   本檔的 Service 類別就是實例：它有 prefix 與 data 兩個參數，
//   手寫要四個建構子，用一個轉發建構子就全部涵蓋，而且新增參數不會爆炸。
//   這不只是「少打字」——多載爆炸會讓每個版本都需要各自維護與測試。
//
// 【2. 為什麼轉發建構子會「搶走」複製建構子（本章最重要的陷阱）】
//   template<class T> Widget(T&& name);
//   當你寫 Widget w2(w1);（w1 是非 const 的 Widget）：
//     * 複製建構子的參數是 const Widget&，需要加上 const → 需要一次資格轉換
//     * 轉發建構子可以把 T 推導成 Widget&，得到「完全精確」的 Widget&
//   多載決議偏好完全精確的匹配，於是模板贏了——編譯器會拿 Widget 去初始化
//   name_（std::string），最後報一個看起來莫名其妙的型別錯誤。
//   注意 const Widget w4; Widget w5(w4); 反而會正確走複製建構子，
//   因為此時複製建構子才是精確匹配。這種「加了 const 行為就變了」正是
//   此陷阱最難察覺的地方。
//   解法：用 enable_if 把「T 去掉參考與 cv 之後就是 Widget 自己」的情況排除。
//
// 【3. 六大陷阱的本質分類】
//   陷阱 1（搶建構子）  —— 多載決議問題，解法 enable_if / C++20 concepts
//   陷阱 2（{1,2,3}）   —— braced-init-list 沒有型別，模板推導本來就不適用
//   陷阱 3（0 / NULL）  —— 0 推導成 int 而非指標，解法一律用 nullptr
//   陷阱 4（static const 整數成員）—— odr-use 需要定義，解法 constexpr/inline
//   陷阱 5（重載函式名）—— 函式名代表一組多載，無法單獨推導型別
//   陷阱 6（位元欄位）  —— 位元欄位沒有獨立位址，無法綁定非 const 參考
//   前五個是「推導或決議」的問題，最後一個是「語言層面沒有位址」的物理限制。
//
// 【4. 陷阱 4 的正確說法（常被講錯）】
//   class Config { static const int MAX_SIZE = 100; };
//   類別內的初值只是「宣告 + 初始值」，不是「定義」。
//   只要發生 odr-use（取址、綁定到 const int&、完美轉發成 const int&），
//   就需要類別外定義 const int Config::MAX_SIZE;，否則連結錯誤。
//   注意：這與「傳值」不同——target(int n) 只需要值，屬於常數轉換，不 odr-use。
//   C++17 起 static constexpr 成員隱含 inline，就不必再寫類別外定義了。
//
// 【概念補充 Concept Deep Dive】
//   (A) enable_if 的位置：本檔用「額外模板參數帶預設值」的寫法
//         template<class T, std::enable_if_t<條件, int> = 0>
//       這是最推薦的形式。寫在回傳型別上對建構子不適用（建構子沒有回傳型別）。
//   (B) 為什麼要 std::decay_t 而不是直接 is_same_v<T, Widget>？
//       因為 T 可能被推導成 Widget&、const Widget&。decay 會去掉參考與 cv 限定，
//       讓「所有形式的 Widget 自己」都被正確排除。
//   (C) C++20 之後這件事有更好的寫法：
//         template<class T> requires (!std::same_as<std::decay_t<T>, Widget>)
//       或直接用 std::constructible_from 之類的 concept，錯誤訊息也友善得多。
//   (D) 陷阱 5 的第二種解法（lambda 包裝）通常比 static_cast 更好，
//       因為 static_cast 要求你手寫完整的函式指標型別，簽名一改就得同步改。
//
// 【注意事項 Pay Attention】
//   1. 轉發建構子務必配 enable_if（或 concepts），否則複製建構子會被搶走。
//   2. 繼承時問題更嚴重：衍生類別的複製建構子也可能被基底的轉發建構子攔截。
//   3. 重試／迴圈類的包裝器不可以轉發引數，只能以左值重複傳遞（本章範例 5 有說明）。
//   4. 位元欄位可用一元 + 產生 prvalue（+f.readable）繞過，或先複製到具名變數。
//   5. 完美轉發無法轉發 braced-init-list、無法轉發多載函式名，這是語言限制，
//      不是寫法問題——呼叫端必須提供明確型別。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】完美轉發的工程陷阱
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼「完美轉發建構子」會搶走複製建構子？怎麼解決？
//     答：對 Widget w2(w1)（w1 非 const），複製建構子要求 const Widget&，
//         需要一次 qualification conversion；而轉發建構子能把 T 推成 Widget&，
//         是完全精確匹配，於是多載決議選了模板，接著在初始化成員時型別錯誤。
//         解法：用 enable_if 排除 std::decay_t<T> 就是自身型別的情況
//         （C++20 可改用 requires / concepts）。
//     追問：那 const Widget w4; Widget w5(w4); 會怎樣？
//         → 反而正常走複製建構子，因為此時它才是精確匹配。
//           「加了 const 就正常、不加就壞掉」正是此 bug 最難查的特徵。
//
// 🔥 Q2. 為什麼 wrapper({1, 2, 3}) 無法編譯？
//     答：braced-init-list 本身沒有型別，模板參數推導對它是 non-deduced context。
//         標準只對 auto 開了特例（auto x = {1,2,3} 推成 initializer_list），
//         模板推導沒有這個特例。解法是在呼叫端寫出明確型別：
//         wrapper(std::vector<int>{1,2,3})。
//
// 🔥 Q3. 完美轉發能轉發重載函式名嗎？
//     答：不能。函式名代表「一整組多載」，在推導 T 的階段編譯器無從選擇。
//         解法一：static_cast<void(*)(int)>(process) 明確指定簽名；
//         解法二（較佳）：用 lambda 包起來 [](int x){ process(x); }，
//         簽名改變時不必同步維護型別字串。
//
// ⚠️ 陷阱. static const int MAX_SIZE = 100; 寫在類別裡就「不需要定義」——為什麼錯？
//     答：類別內的初值只是宣告加初始值，不是定義。一旦發生 odr-use
//         （取址、綁到 const int&、被完美轉發成 const int&），
//         就必須有類別外定義 const int Config::MAX_SIZE;，否則連結期報
//         undefined reference。傳值使用不會 odr-use，所以「有時能過有時不能」。
//     為什麼會錯：多數人以為「有寫 = 100 就是定義好了」，
//         把「常數值可在編譯期取用」與「這個變數在記憶體中存在」混為一談。
//         C++17 起 static constexpr 隱含 inline，才真正不必再補定義。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <utility>
#include <memory>
#include <vector>
#include <functional>
#include <type_traits>
#include <unordered_map>
#include <typeindex>
#include <algorithm>
#include <stdexcept>

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
// 參數刻意不命名：只示範選到哪個多載（命名不用會觸發 -Wunused-parameter）
void final_fn(const std::string&) { std::cout << "  最終: 左值\n"; }
void final_fn(std::string&&)      { std::cout << "  最終: 右值\n"; }

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

// 這個 fwd 只用來證明「某個呼叫寫法能不能通過推導」，故意不使用引數。
template<typename T>
void fwd(T&&) { std::cout << "  forwarded\n"; }

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 155. Min Stack
//   題目：設計一個支援 push / pop / top / getMin 的堆疊，全部 O(1)。
//   為什麼用到本主題：標準解法是在堆疊裡存 pair<值, 到此為止的最小值>。
//     每次 push 都要生出一個 pair——用 st_.emplace(val, newMin) 就是把兩個
//     引數完美轉發到 pair 的建構子、在容器記憶體上就地建構，
//     不必先做一個臨時 pair 再搬進去。這正是本章公式
//     「Args&&... + std::forward<Args>(args)...」在標準函式庫裡的成品。
//   複雜度：四個操作全部 O(1)；空間 O(n)。
// -----------------------------------------------------------------------------
class MinStack {
    // first = 這次推入的值，second = 從堆疊底到這裡為止的最小值
    std::vector<std::pair<int, int>> st_;
public:
    void push(int val) {
        int curMin = st_.empty() ? val : std::min(val, st_.back().second);
        st_.emplace_back(val, curMin);   // ★ 完美轉發到 pair<int,int> 的建構子
    }
    void pop()        { st_.pop_back(); }
    int  top()  const { return st_.back().first; }
    int  getMin() const { return st_.back().second; }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】以完美轉發實作「帶重試的 HTTP 請求送出器」
//   情境：對外部 API 送請求，失敗要重試。這裡刻意示範本章的重要邊界：
//     ★ 會重複呼叫目標的包裝器，「不可以」轉發引數 ★
//   因為第一次呼叫若把引數搬走了，第二次重試就只剩空殼。
//   正確做法是以左值重複傳遞，犧牲一點複製成本換取正確性——
//   這是「完美轉發不是到處都該用」的經典反例。
// -----------------------------------------------------------------------------
struct HttpResponse {
    int status;
    std::string body;
};

int g_attemptCounter = 0;   // 模擬前兩次失敗、第三次成功

HttpResponse sendRequest(const std::string& url, const std::string& payload) {
    ++g_attemptCounter;
    if (g_attemptCounter < 3) {
        throw std::runtime_error("connection reset");
    }
    return HttpResponse{200, "ok:" + url + ":" + payload};
}

template<typename Func, typename... Args>
auto retryCall(int maxAttempts, Func&& func, Args&... args)   // ← 注意是 Args&，不是 Args&&
    -> decltype(func(args...))
{
    for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
        try {
            std::cout << "    第 " << attempt << " 次嘗試\n";
            // 刻意「不」轉發：引數必須能被重複使用
            return func(args...);
        } catch (const std::exception& e) {
            std::cout << "      失敗: " << e.what() << "\n";
            if (attempt == maxAttempts) throw;
        }
    }
    throw std::runtime_error("unreachable");
}

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

    // ============================================================
    // 7. LeetCode 155. Min Stack（emplace_back = 完美轉發）
    // ============================================================
    std::cout << "\n=== LeetCode 155. Min Stack ===\n";
    {
        MinStack ms;
        ms.push(-2);
        ms.push(0);
        ms.push(-3);
        std::cout << "  getMin() = " << ms.getMin() << "\n";   // -3
        ms.pop();
        std::cout << "  top()    = " << ms.top() << "\n";      // 0
        std::cout << "  getMin() = " << ms.getMin() << "\n";   // -2
    }

    // ============================================================
    // 8. 日常實務：重試包裝器（示範「不該轉發」的情境）
    // ============================================================
    std::cout << "\n=== 日常實務：帶重試的請求送出器 ===\n";
    {
        std::string url = "https://api.example.com/orders";
        std::string payload = "{\"id\":1001}";
        try {
            auto resp = retryCall(5, sendRequest, url, payload);
            std::cout << "  成功: status=" << resp.status
                      << " body=" << resp.body << "\n";
        } catch (const std::exception& e) {
            std::cout << "  最終失敗: " << e.what() << "\n";
        }
        // 關鍵驗證：因為沒有轉發，引數在多次重試後仍然完好
        std::cout << "  重試後 url 仍完好: " << url << "\n";
        std::cout << "  重試後 payload 仍完好: " << payload << "\n";
        std::cout << "  → 會重複呼叫的包裝器不可轉發引數，這是完美轉發的邊界\n";
    }

    std::cout << "\n=== 重點整理 ===\n";
    std::cout << "  公式：template<typename T> void f(T&& a) { g(forward<T>(a)); }\n";
    std::cout << "  應用：工廠、建構子、包裝器、emplace_back\n";
    std::cout << "  陷阱：模板搶建構子、{} 推導、0/NULL、重載函式、位元欄位\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// 註：Flags f = {1, 1}; 只初始化了前兩個位元欄位，第三個（executable）
//     會被值初始化為 0，這是聚合初始化的規則，不是未定義行為。
//     本檔所有輸出皆為確定性內容，不含時間或位址。

// === 預期輸出 ===
// ===== 1. 泛型工廠 =====
//   Connection(example.com:8080, https)
//   Connection(localhost:3000, http)
//
// ===== 2. 完美轉發建構子 =====
//   全部左值:
//     Logger 複製
//   全部右值:
//     Logger 移動
//
// ===== 3. enable_if =====
//   Widget 模板建構子
//   Widget 複製建構子
//   Widget 移動建構子
//
// ===== 4. 多層轉發（穿越三層）=====
//   傳左值:
//   最終: 左值
//   傳右值:
//   最終: 右值
//
// ===== 5. 日誌包裝器 =====
//   [LOG] add 開始
//   [LOG] add 結束
//   結果: 30
//
// ===== 6. 完美轉發的陷阱 =====
//   forwarded
//   {1,2,3} 無法推導 → 需要明確建構
//   forwarded
//   用 nullptr 取代 0/NULL
//   forwarded
//   重載函式需要 static_cast 指定版本
//   forwarded
//   位元欄位需先複製到變數
//
// === LeetCode 155. Min Stack ===
//   getMin() = -3
//   top()    = 0
//   getMin() = -2
//
// === 日常實務：帶重試的請求送出器 ===
//     第 1 次嘗試
//       失敗: connection reset
//     第 2 次嘗試
//       失敗: connection reset
//     第 3 次嘗試
//   成功: status=200 body=ok:https://api.example.com/orders:{"id":1001}
//   重試後 url 仍完好: https://api.example.com/orders
//   重試後 payload 仍完好: {"id":1001}
//   → 會重複呼叫的包裝器不可轉發引數，這是完美轉發的邊界
//
// === 重點整理 ===
//   公式：template<typename T> void f(T&& a) { g(forward<T>(a)); }
//   應用：工廠、建構子、包裝器、emplace_back
//   陷阱：模板搶建構子、{} 推導、0/NULL、重載函式、位元欄位
