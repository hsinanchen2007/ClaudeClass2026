// =============================================================================
//  第 15 課：帶參數的建構函數 11  —  預設參數：讓 API 同時好用又有彈性
// =============================================================================
//
// 【主題資訊 Information】
//   語法      : T(const string& a, int b = 30, bool c = true);
//   規則      : 預設值只能**從右往左**連續給；有預設值的參數右邊不能有沒預設值的
//   標準版本  : C++98 起
//   標頭檔    : <iostream>、<string>
//   核心價值  : 讓 90% 的呼叫端只寫必要參數，10% 的仍能完全掌控
//
// 【詳細解釋 Explanation】
//
// 【1. 參數順序就是 API 設計】
//   因為只能從右往左省略，參數順序實際上等於「必要性的排序」：
//       HttpRequest(url, method = "GET", timeout = 30, followRedirect = true)
//   url 沒有合理預設 → 必填、放最左；
//   method 最常被改 → 次左；
//   timeout 偶爾調；followRedirect 幾乎不動 → 放最右。
//   若順序寫反（followRedirect 在前），想改 method 就得連 followRedirect
//   一起寫出來，預設參數的便利性立刻歸零。
//   **設計 API 時，參數順序不是隨意的，它決定了呼叫端有多好寫。**
//
// 【2. 預設值是在「呼叫端」展開的（重要且常被忽略）】
//   預設值不是存在函式本體裡，而是編譯器在**每一個呼叫的地方**替你補上引數。
//   兩個實際後果：
//     (a) 改了預設值之後，只重新編譯函式庫是**沒有用的**——
//         所有呼叫端都必須重新編譯才會採用新值。這是函式庫 ABI 的實務議題。
//     (b) 預設值必須在呼叫點可見，所以它寫在**宣告**（標頭檔）而不是定義。
//         同一個參數不能在宣告和定義中各給一次預設值（重複定義，編譯錯誤）。
//
// 【3. virtual function 千萬不要用預設參數】
//   預設參數是**靜態繫結**（依編譯期的靜態型別決定），
//   而 virtual function 的實作是**動態繫結**（依執行期的實際型別決定）。
//   兩者混用會得到「一半 base、一半 derived」的結果：
//       struct Base    { virtual void f(int n = 10); };
//       struct Derived : Base { void f(int n = 20) override; };
//       Base* p = new Derived;
//       p->f();     // 執行 Derived::f，但 n 取的是 **Base 的 10**
//   本檔以可執行範例實測驗證了這個行為。
//   準則很簡單：virtual function 不要有預設參數。
//
// 【4. 預設參數 vs 多載，怎麼選】
//   * 語意相同、只是省略尾端參數 → 預設參數（精簡，一份實作）
//   * 語意不同、初始化邏輯不同   → 多載（各寫各的）
//   另外注意兩者混用很容易造出 ambiguous 的組合，例如同時有
//   T(string) 與 T(string, int = 0)，呼叫 T("x") 時兩者皆可行 → 編譯錯誤。
//
// 【概念補充 Concept Deep Dive】
//   ▍C++ 沒有具名引數（named arguments）
//     不能寫 HttpRequest("url", timeout = 120)。想「只指定中間某個參數」，
//     慣用作法是：
//       (a) named parameter idiom：回傳 *this 的鏈式 setter
//       (b) 傳一個 options struct（可搭配 designated initializers，C++20）
//     本檔示範 (a)。
//
//   ▍預設值可以是函式呼叫
//     void log(const string& msg, long ts = currentTime());
//     這個 currentTime() 會在**每次呼叫時**求值，不是只算一次。
//     常見誤解是以為它像 Python 的可變預設引數那樣只算一次——C++ 沒有那個坑。
//
//   ▍預設參數與 explicit
//     HttpRequest(const string& url, ...) 因為其餘參數都有預設值，
//     所以它**可以用單一引數呼叫** → 它同時是 converting constructor，
//     string 會隱式轉成 HttpRequest。若不想要，請加 explicit。
//
// 【注意事項 Pay Attention】
//   1. 預設值只能從右往左連續給；C++ 沒有具名引數，不能跳著指定。
//   2. 預設值寫在宣告處（標頭檔），不可在宣告與定義中重複給。
//   3. 改預設值需要重新編譯所有呼叫端才會生效（函式庫 ABI 注意）。
//   4. **virtual function 不要用預設參數**——靜態繫結會造成詭異結果。
//   5. 其餘參數都有預設值時，該 constructor 也是 converting constructor，
//      視情況加 explicit。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】預設參數
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 預設參數存在哪裡？改了預設值之後要做什麼才會生效？
//     答：預設值不在函式本體裡，而是編譯器在**每個呼叫端**替你補上引數。
//         因此改了預設值後，只重新編譯函式庫沒有用，
//         **所有呼叫端都必須重新編譯**才會採用新值。
//         這也是為什麼預設值要寫在宣告（標頭檔）而非定義。
//     追問：可以在宣告和定義中各給一次嗎？→ 不行，同一個參數重複給預設值是
//         編譯錯誤。慣例是只寫在標頭檔的宣告處。
//
// 🔥 Q2. 為什麼不能在 virtual function 上使用預設參數？
//     答：預設參數是靜態繫結（看指標／參考的靜態型別），
//         virtual function 是動態繫結（看物件的實際型別）。
//         用 Base* 呼叫 Derived 的 override 時，會執行 Derived 的實作，
//         卻套用 Base 宣告的預設值，得到「一半 base、一半 derived」的結果，
//         而且沒有任何警告。本檔以實測驗證。
//     追問：真的需要「每個子類別有不同預設」怎麼辦？→ 改成
//         non-virtual 的公開介面帶預設參數，內部再呼叫 pure virtual 的實作
//         （NVI，Non-Virtual Interface 慣用法）。
//
// ⚠️ 陷阱. 想「只指定 timeout」而寫 HttpRequest("url", timeout = 120)
//     答：語法錯誤。C++ 沒有具名引數（到 C++23 仍然沒有）。
//         你只能寫 HttpRequest("url", "GET", 120)，把中間的 method 一併補上。
//         想要真正的「只指定某一個」，請用鏈式 setter 或 options struct。
//     為什麼會錯：把 Python／C#／Kotlin 的具名引數習慣帶進 C++。
//         C++ 的預設參數只有「從右往左省略」這一種用法。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
using namespace std;

class HttpRequest {
private:
    string url;
    string method;
    int timeout;      // 秒
    bool followRedirect;

public:
    // 只有 url 是必需的，其他都有合理的預設值。
    // 參數順序 = 必要性排序：url（必填）→ method（常改）→ timeout → followRedirect
    // 預設參數只能從右往左依次省略，不能跳過中間的參數（C++ 沒有具名引數）。
    HttpRequest(const string& url,
                const string& method = "GET",
                int timeout = 30,
                bool followRedirect = true)
        : url(url), method(method), timeout(timeout),
          followRedirect(followRedirect)
    {
        cout << "  創建請求: " << method << " " << url << endl;
    }

    void print() const {
        cout << "  [" << method << "] " << url
             << " (timeout=" << timeout << "s"
             << ", redirect=" << (followRedirect ? "是" : "否")
             << ")" << endl;
    }
};

// -----------------------------------------------------------------------------
// 實測：virtual function + 預設參數 = 靜態繫結的陷阱
// -----------------------------------------------------------------------------
struct Base {
    virtual ~Base() = default;
    virtual void report(int n = 10) const {
        cout << "    Base::report(n=" << n << ")" << endl;
    }
};

struct Derived : Base {
    void report(int n = 20) const override {
        cout << "    Derived::report(n=" << n << ")" << endl;
    }
};

// -----------------------------------------------------------------------------
// C++ 沒有具名引數 → named parameter idiom（鏈式 setter）
// -----------------------------------------------------------------------------
class RequestBuilder {
private:
    string url_;
    string method_        = "GET";
    int    timeout_       = 30;
    bool   followRedirect_ = true;

public:
    explicit RequestBuilder(const string& url) : url_(url) {}

    RequestBuilder& method(const string& m)   { method_ = m; return *this; }
    RequestBuilder& timeout(int s)            { timeout_ = s; return *this; }
    RequestBuilder& followRedirect(bool b)    { followRedirect_ = b; return *this; }

    void print() const {
        cout << "  [" << method_ << "] " << url_
             << " (timeout=" << timeout_ << "s"
             << ", redirect=" << (followRedirect_ ? "是" : "否") << ")" << endl;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】從缺
//   理由：LeetCode 的類別簽名由題目完整指定，參數一個都不能省，
//         因此預設參數在那個環境中毫無用武之地——寫了也不會被呼叫到。
//         本檔談的是 API 人因設計（參數順序、可省略性、ABI 影響），
//         屬於工程議題。本課真正對應的設計題（146 LRU Cache）在 12.cpp，
//         summary.cpp 另有 707 Design Linked List。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】資料庫查詢介面（QueryOptions）：預設參數的真實用法
//   情境：內部的 DB 查詢函式，大多數呼叫只要「給我這張表的資料」，
//         少數需要分頁、排序、逾時控制。
//   設計要點：
//     * 預設值必須是**安全**的那一邊——limit 預設 100 而不是「全部」，
//       避免有人不小心 SELECT 整張億筆的表把服務打掛。
//     * 參數順序依「最常調整」排列，讓常見情境寫起來最短。
//   這是預設參數最有價值的用途：把「安全的預設行為」內建進 API，
//   讓最省事的寫法同時也是最安全的寫法。
// -----------------------------------------------------------------------------
class DbQuery {
private:
    string table_;
    int    limit_;
    string orderBy_;
    int    timeoutMs_;

public:
    // limit 預設 100：不給上限的查詢是線上事故的常見來源
    explicit DbQuery(const string& table,
                     int limit           = 100,
                     const string& orderBy = "id",
                     int timeoutMs       = 5000)
        : table_(table), limit_(limit), orderBy_(orderBy), timeoutMs_(timeoutMs) {}

    void explain() const {
        cout << "    SELECT * FROM " << table_
             << " ORDER BY " << orderBy_
             << " LIMIT " << limit_
             << ";  (timeout " << timeoutMs_ << "ms)" << endl;
    }
};

int main() {
    cout << "=== 只提供 URL ===" << endl;
    HttpRequest r1("https://example.com");
    r1.print();

    cout << "\n=== 指定 Method ===" << endl;
    HttpRequest r2("https://api.example.com/data", "POST");
    r2.print();

    cout << "\n=== 指定 Method 和 Timeout ===" << endl;
    HttpRequest r3("https://slow-server.com", "GET", 120);
    r3.print();

    cout << "\n=== 全部指定 ===" << endl;
    HttpRequest r4("https://redirect.com", "GET", 10, false);
    r4.print();

    cout << "\n=== 只能從右往左省略 ===" << endl;
    cout << "  想「只改 timeout」不能寫 HttpRequest(\"url\", timeout = 120)——" << endl;
    cout << "  C++ 沒有具名引數，只能把中間的 method 一併補上：" << endl;
    HttpRequest r5("https://example.com", "GET", 120);
    r5.print();

    cout << "\n=== 替代方案：named parameter idiom（鏈式 setter）===" << endl;
    RequestBuilder b("https://example.com");
    b.timeout(120);                       // 真的「只指定 timeout」
    b.print();

    cout << "\n=== 陷阱實測：virtual function + 預設參數 ===" << endl;
    Derived d;
    Base* p = &d;
    cout << "  透過 Base* 呼叫 p->report():" << endl;
    p->report();                          // 執行 Derived::report，但 n 用 Base 的 10
    cout << "  ↑ 執行的是 Derived 的實作，n 卻是 Base 宣告的 10" << endl;
    cout << "  直接用 Derived 物件呼叫 d.report():" << endl;
    d.report();                           // 這次 n 才是 20
    cout << "  ↑ 同一個物件、同一個函式，預設值卻不同——" << endl;
    cout << "    因為預設參數是靜態繫結，實作卻是動態繫結。" << endl;
    cout << "  結論：virtual function 不要用預設參數。" << endl;

    cout << "\n=== 日常實務：查詢介面的安全預設值 ===" << endl;
    cout << "  最常見的呼叫（只給表名）：" << endl;
    DbQuery q1("orders");
    q1.explain();
    cout << "  需要更多筆：" << endl;
    DbQuery q2("orders", 1000);
    q2.explain();
    cout << "  完整指定：" << endl;
    DbQuery q3("orders", 50, "created_at DESC", 2000);
    q3.explain();
    cout << "  ↑ limit 預設 100 而非「全部」——" << endl;
    cout << "    最省事的寫法同時也是最安全的寫法，這是預設參數的真正價值。" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 15 課：帶參數的建構函數11.cpp" -o param11

// === 預期輸出 ===
// === 只提供 URL ===
//   創建請求: GET https://example.com
//   [GET] https://example.com (timeout=30s, redirect=是)
//
// === 指定 Method ===
//   創建請求: POST https://api.example.com/data
//   [POST] https://api.example.com/data (timeout=30s, redirect=是)
//
// === 指定 Method 和 Timeout ===
//   創建請求: GET https://slow-server.com
//   [GET] https://slow-server.com (timeout=120s, redirect=是)
//
// === 全部指定 ===
//   創建請求: GET https://redirect.com
//   [GET] https://redirect.com (timeout=10s, redirect=否)
//
// === 只能從右往左省略 ===
//   想「只改 timeout」不能寫 HttpRequest("url", timeout = 120)——
//   C++ 沒有具名引數，只能把中間的 method 一併補上：
//   創建請求: GET https://example.com
//   [GET] https://example.com (timeout=120s, redirect=是)
//
// === 替代方案：named parameter idiom（鏈式 setter）===
//   [GET] https://example.com (timeout=120s, redirect=是)
//
// === 陷阱實測：virtual function + 預設參數 ===
//   透過 Base* 呼叫 p->report():
//     Derived::report(n=10)
//   ↑ 執行的是 Derived 的實作，n 卻是 Base 宣告的 10
//   直接用 Derived 物件呼叫 d.report():
//     Derived::report(n=20)
//   ↑ 同一個物件、同一個函式，預設值卻不同——
//     因為預設參數是靜態繫結，實作卻是動態繫結。
//   結論：virtual function 不要用預設參數。
//
// === 日常實務：查詢介面的安全預設值 ===
//   最常見的呼叫（只給表名）：
//     SELECT * FROM orders ORDER BY id LIMIT 100;  (timeout 5000ms)
//   需要更多筆：
//     SELECT * FROM orders ORDER BY id LIMIT 1000;  (timeout 5000ms)
//   完整指定：
//     SELECT * FROM orders ORDER BY created_at DESC LIMIT 50;  (timeout 2000ms)
//   ↑ limit 預設 100 而非「全部」——
//     最省事的寫法同時也是最安全的寫法，這是預設參數的真正價值。
