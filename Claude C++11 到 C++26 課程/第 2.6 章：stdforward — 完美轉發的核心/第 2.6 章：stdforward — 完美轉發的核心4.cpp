// =============================================================================
//  第 2.6 章 範例 4  —  泛型工廠函式：make<T>(args...) 的完美轉發
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<utility>（std::forward）、<memory>（std::unique_ptr）
//   本檔示範的樣式：
//     template<class T, class... Args>
//     std::unique_ptr<T> make(Args&&... args);
//   這正是 std::make_unique（C++14）與 std::make_shared（C++11）的骨架。
//   複雜度：轉發本身 O(1) 且零成本，實際成本由 T 的建構子決定。
//
// 【詳細解釋 Explanation】
//
// 【1. 工廠函式為什麼非要完美轉發不可】
//   工廠的職責是「代替呼叫端呼叫建構子」。既然是代替，就不該改變語意：
//   呼叫端寫 make<Person>(name, 30) 傳左值，就該複製；
//   寫 make<Person>(std::string("Bob"), 25) 傳右值，就該移動。
//   如果工廠把參數宣告成 const Args&，右值資訊在進入函式的瞬間就消失，
//   T 的移動建構子永遠不會被選中——效能損失還算小事，真正的問題是
//   「只能移動不能複製」的型別（unique_ptr、thread、future）根本傳不進去。
//
// 【2. 為什麼參數包也能轉發：Args&&... 與 forward<Args>(args)...】
//   Args&&... 是「一包轉發參考」，每個參數各自獨立推導：
//     make<Person>(name, 30)  →  Args = {std::string&, int}
//   展開時 std::forward<Args>(args)... 會逐一配對展開成
//     std::forward<std::string&>(args0), std::forward<int>(args1)
//   所以同一次呼叫裡「左值參數保持左值、右值參數保持右值」是分別成立的，
//   不是整包一起決定。這就是所謂的「混合左右值」也能正確轉發。
//
// 【3. 三種呼叫方式各自發生什麼】
//   make<Person>(name, 30)                 name 是具名變數 → 左值 → 複製建構子
//   make<Person>(std::string("Bob"), 25)   臨時物件 → 右值 → 移動建構子
//   make<Person>("Charlie", 35)            const char[8] → 需先隱式轉成 std::string，
//                                          這個轉換產生的臨時物件是右值 → 移動建構子
//   第三種常被誤答成「複製」，因為字面值看起來很像常數。關鍵在於「隱式轉換
//   造出來的是一個臨時 std::string」，而臨時物件是純右值。
//
// 【概念補充 Concept Deep Dive】
//   (A) 本檔的 make 用 new 再包進 unique_ptr，這是 C++11 沒有 make_unique 時的寫法。
//       C++14 起應直接用 std::make_unique<T>(args...)：它同樣完美轉發，
//       而且把 new 包在函式內部，避免「引數求值順序」造成的例外洩漏。
//   (B) 為什麼 make_shared 比 new + shared_ptr 好？make_shared 只配置一次記憶體
//       （控制區塊與物件相鄰），new 版本要配置兩次。這是轉發之外的額外好處。
//   (C) 完美轉發工廠無法轉發 braced-init-list：make<std::vector<int>>({1,2,3})
//       中的 {1,2,3} 沒有型別，模板推導失敗。要明確寫成 std::vector<int>{1,2,3}。
//
// 【注意事項 Pay Attention】
//   1. 參數只能轉發一次。若工廠內要用同一個參數做兩件事，只有最後一次能 forward。
//   2. 轉發不會做隱式轉換的「保底」：T 必須真的有能吃這些引數的建構子。
//   3. 回傳 std::unique_ptr<T> 時不必寫 std::move(ptr)，那會妨礙 RVO。
//   4. 被移動的來源物件處於 valid but unspecified 狀態，只保證可解構、可再賦值。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】完美轉發工廠函式
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼工廠函式的參數要寫 Args&&... 而不是 const Args&...？
//     答：const Args&... 會把所有引數一律當左值，T 的移動建構子永遠選不到；
//         而且 unique_ptr、thread 這類「只能移動」的型別根本無法傳入。
//         Args&&... 是轉發參考，配合 std::forward 才能原樣傳遞值類別。
//     追問：那寫成 Args... （傳值）行不行？
//         → 可以編譯，但每個引數都多一次複製或移動，白白付出成本。
//
// 🔥 Q2. make<Person>("Charlie", 35) 會呼叫複製建構子還是移動建構子？
//     答：移動建構子。const char* 要先隱式轉換成 std::string，這個轉換
//         產生的是「臨時 std::string」，臨時物件是純右值，因此選到 string&& 版本。
//     追問：那 Args 被推導成什麼？→ Args = {const char(&)[8], int}，
//         轉換發生在 Person 的建構子多載決議階段，不在推導階段。
//
// ⚠️ 陷阱. 「工廠內部把參數用了兩次，各自 forward 一次」為什麼危險？
//     答：第一次 forward 若解析成右值，該引數就可能已經被搬空；
//         第二次 forward 拿到的是被掏空的物件，得到空字串或空容器。
//         正確做法是只在「最後一次使用」時 forward，其餘用具名左值。
//     為什麼會錯：多數人把 forward 想成「唯讀的型別標記」，
//         但它實際上是在授權下游把資源偷走，語意上等同一次性的通行證。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <memory>
#include <utility>

class Person {
    std::string name_;
    int age_;

public:
    Person(const std::string& name, int age)
        : name_(name), age_(age) {
        std::cout << "  Person(const string&, int) 複製 name\n";
    }

    Person(std::string&& name, int age)
        : name_(std::move(name)), age_(age) {
        std::cout << "  Person(string&&, int) 移動 name\n";
    }

    void print() const {
        std::cout << "  " << name_ << ", age " << age_ << "\n";
    }
};

// 泛型工廠函式：完美轉發任意數量、任意型別的引數
template<typename T, typename... Args>
std::unique_ptr<T> make(Args&&... args) {
    return std::unique_ptr<T>(
        new T(std::forward<Args>(args)...)  // 完美轉發所有引數
    );
}

// -----------------------------------------------------------------------------
// 【日常實務範例】用同一個工廠建立不同型別的服務元件
//   情境：微服務啟動時要依設定檔組出多個元件，每個元件的建構參數不同
//         （HTTP 伺服器要 host+port，重試器要次數+間隔）。
//         若不用完美轉發，就得為每個元件各寫一支 create 函式；
//         有了 make<T>(args...)，新增元件不必動工廠一行程式碼。
// -----------------------------------------------------------------------------
class HttpServer {
    std::string host_;
    int port_;
public:
    HttpServer(const std::string& host, int port) : host_(host), port_(port) {
        std::cout << "  HttpServer(const string&) 複製 host\n";
    }
    HttpServer(std::string&& host, int port) : host_(std::move(host)), port_(port) {
        std::cout << "  HttpServer(string&&) 移動 host\n";
    }
    void print() const { std::cout << "  serving " << host_ << ":" << port_ << "\n"; }
};

int main() {
    std::string name = "Alice";

    std::cout << "--- 傳入左值 ---\n";
    auto p1 = make<Person>(name, 30);
    p1->print();

    std::cout << "\n--- 傳入右值 ---\n";
    auto p2 = make<Person>(std::string("Bob"), 25);
    p2->print();

    std::cout << "\n--- 傳入字面值 ---\n";
    auto p3 = make<Person>("Charlie", 35);  // const char* → string 隱式轉換
    p3->print();

    std::cout << "\n--- 驗證：左值來源沒有被搬空 ---\n";
    std::cout << "  name = \"" << name << "\"\n";

    std::cout << "\n=== 日常實務：同一個工廠建立不同元件 ===\n";
    std::string bindAddr = "0.0.0.0";
    auto srv1 = make<HttpServer>(bindAddr, 8080);                 // 左值 → 複製
    srv1->print();
    auto srv2 = make<HttpServer>(std::string("127.0.0.1"), 9090); // 右值 → 移動
    srv2->print();
    std::cout << "  bindAddr 仍可重複使用: \"" << bindAddr << "\"\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.6 章：stdforward — 完美轉發的核心4.cpp" -o make_factory

// 註：本檔未附 LeetCode 範例。LeetCode 的評測程式碼會直接呼叫題目指定的類別介面，
//     不會出現「泛型工廠 + 完美轉發」這種函式庫層抽象；硬套一題只會失真。
//     emplace 家族在 LeetCode 的真實用法，請見同章範例 5 與 summary.cpp。

// === 預期輸出 ===
// --- 傳入左值 ---
//   Person(const string&, int) 複製 name
//   Alice, age 30
//
// --- 傳入右值 ---
//   Person(string&&, int) 移動 name
//   Bob, age 25
//
// --- 傳入字面值 ---
//   Person(string&&, int) 移動 name
//   Charlie, age 35
//
// --- 驗證：左值來源沒有被搬空 ---
//   name = "Alice"
//
// === 日常實務：同一個工廠建立不同元件 ===
//   HttpServer(const string&) 複製 host
//   serving 0.0.0.0:8080
//   HttpServer(string&&) 移動 host
//   serving 127.0.0.1:9090
//   bindAddr 仍可重複使用: "0.0.0.0"
