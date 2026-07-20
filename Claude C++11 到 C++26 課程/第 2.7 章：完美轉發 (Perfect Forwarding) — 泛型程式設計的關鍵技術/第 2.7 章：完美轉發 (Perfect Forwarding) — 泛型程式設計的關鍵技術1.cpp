// =============================================================================
//  第 2.7 章 - 1  —  變參工廠函式：用一個 make_connection 涵蓋所有引數組合
// =============================================================================
//
// 【主題資訊 Information】
//   forwarding reference（轉發引用，又稱 universal reference）:
//       template<typename T> void f(T&& x);        // T 需被「推導」才算
//   std::forward:
//       template<class T> constexpr T&& forward(std::remove_reference_t<T>& t) noexcept;
//   變參模板（variadic template）:
//       template<typename... Args> void f(Args&&... args);
//       展開：f(std::forward<Args>(args)...)
//
//   標準版本：forwarding reference / std::forward / variadic template 皆為 C++11。
//             std::make_unique 為 C++14；emplace_back 回傳 reference 為 C++17
//             （C++11/14 回傳 void）。本檔以 C++17 編譯。
//   標頭檔：<utility>（std::forward）、<memory>（unique_ptr）、<vector>
//   複雜度：完美轉發本身是編譯期機制，執行期零額外成本（不產生中介物件）。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼工廠函式非要完美轉發不可】
//   工廠函式的職責是「把引數原封不動交給建構子」。它自己不使用引數、
//   不改變引數，只是個傳聲筒。問題在於：C++ 的參數傳遞方式會「洗掉」
//   引數原本的性質。
//
//   假設不用完美轉發，只能三選一：
//     (a) void make(T v)          → 一律複製一次，右值也白白複製
//     (b) void make(const T& v)   → 不複製，但整個 const 化：
//                                    傳進來的右值再也移動不了，
//                                    建構子只能走「複製」那條路
//     (c) void make(T& v)         → 連字面值 "abc" 這種右值都收不了
//
//   (b) 是最常見的折衷，也是最常見的效能漏洞：呼叫端明明給了
//   std::string("localhost") 這個馬上要死掉的臨時物件，卻因為中途被綁成
//   const&，到了 host_ 的初始化只能複製，白白多配置一次記憶體。
//
//   完美轉發要達成的是：**引數是左值就以左值抵達、是右值就以右值抵達，
//   const 與否也原樣保留**——「完美」二字指的就是這個「原樣」。
//
// 【2. 為什麼是 T&& 而不是別的寫法】
//   T&& 在「T 需要被推導」的位置上，不是右值引用，而是 forwarding reference。
//   它是標準特別為此開的一個後門（[temp.deduct.call]/3）：
//
//     * 傳左值 X 進來 → T 被推導成 X&   （注意：帶引用！）
//     * 傳右值 X 進來 → T 被推導成 X    （不帶引用）
//
//   這一步是整個機制的關鍵：**T 自己記住了「原本是左值還是右值」**。
//   接著再靠 reference collapsing 把它還原（見【概念補充】）。
//
//   要成為 forwarding reference，形式必須「剛好」是 T&&：
//     template<class T> void f(T&& x);          // ✅ 是
//     template<class T> void f(const T&& x);    // ❌ 不是（多了 const）
//     template<class T> void f(std::vector<T>&& x); // ❌ 不是（T 不在頂層）
//     void f(auto&& x);                          // ✅ 是（C++20 簡寫函式模板）
//   類別模板的成員函式也常踩雷：
//     template<class T> struct V { void push(T&& x); };   // ❌ 這是右值引用！
//     T 在 V 實例化時就定了，push 沒有自己的推導，所以 T&& 就是純右值引用。
//
// 【3. std::forward 到底做了什麼】
//   它只是一個「有條件的 static_cast」，沒有任何執行期動作：
//       forward<X&>(t)  → 回傳 X&   （左值）
//       forward<X>(t)   → 回傳 X&&  （右值）
//   注意它一定要顯式寫模板引數 std::forward<Args>(args)。理由是：
//   具名參數 args 本身**永遠是左值**（有名字的東西都是左值），
//   光靠推導無法知道它「原本」是什麼，只能從 T 這個型別把資訊撈回來。
//
// 【4. 變參模板：把 N 個參數一起轉發】
//   Args&&... args 宣告了一個參數包。展開時的寫法是：
//       std::forward<Args>(args)...
//   省略號在**整個 pattern 後面**，代表「對每一組 (Args_i, args_i) 都套用
//   一次 std::forward，再用逗號串起來」。展開後等價於：
//       std::forward<Args0>(args0), std::forward<Args1>(args1), ...
//   常見錯誤是寫成 std::forward<Args...>(args...)——那是把整包塞進一次呼叫，
//   語意完全不同。
//
// 【概念補充 Concept Deep Dive】
//
// ── reference collapsing：完美轉發真正的引擎 ──────────────────────────
//   C++ 不允許你手寫「引用的引用」，但模板推導與 typedef 會產生它。
//   標準規定用以下規則把它「摺疊」掉（[dcl.ref]/6）——
//   口訣：**只要有一個左值引用，結果就是左值引用；全是右值引用才是右值引用。**
//
//        T& &   →  T&        T&& &  →  T&
//        T& &&  →  T&        T&& && →  T&&
//
//   把它套進 make_connection(Args&&... args) 走一遍：
//
//     呼叫 make_connection(host)              host 是 std::string 左值
//       → Args 推導為 std::string&
//       → 參數型別 Args&& = std::string& &&  --摺疊--> std::string&
//       → std::forward<std::string&> 回傳 std::string&（左值）
//       → Connection 的 host_ 走「複製」
//
//     呼叫 make_connection(std::string("localhost"))   引數是右值
//       → Args 推導為 std::string（不帶引用）
//       → 參數型別 Args&& = std::string&&（本來就是右值引用，不用摺疊）
//       → std::forward<std::string> 回傳 std::string&&（右值）
//       → Connection 的 host_ 走「移動」
//
//   所以「左值→左值、右值→右值」不是編譯器的魔法，而是
//   「推導把資訊存進 T」＋「摺疊規則把資訊還原」兩步的必然結果。
//
// ── emplace_back：把完美轉發用到極致 ────────────────────────────────
//   push_back(const T&) / push_back(T&&) 收的是「一個已經造好的 T」，
//   所以呼叫端得先造出臨時物件，再複製或移動進容器。
//   emplace_back(Args&&...) 收的是「造一個 T 所需要的原料」，
//   把原料完美轉發到 **元素自己的建構子**，直接在容器的記憶體上建構：
//
//       pool.push_back(Session("alice", 1));   // 建構臨時 → 移動 → 臨時解構
//       pool.emplace_back("alice", 1);         // 就地建構，完全沒有臨時物件
//
//   省下的不只是一次移動，還有臨時物件的解構。對「移動成本不為零」的型別
//   （例如內含 std::string、或根本不可移動的型別）差別更明顯。
//   本檔 main() 會用計數器把這件事真的數出來。
//
// ── 為什麼工廠一定要 forwarding 才能支援「不可複製」的型別 ──────────
//   std::unique_ptr 不可複製、只可移動。若工廠寫成 make(const T&)，
//   unique_ptr 連傳都傳不進去。只有完美轉發能讓 move-only 型別穿過工廠。
//
// 【注意事項 Pay Attention】
//   1. 轉發過的引數要當成「已經被搬走」看待。被移動後的物件處於
//      **valid but unspecified**（有效但未指定）狀態——可以安全解構、
//      可以重新賦值，但不可假設它還留著原本的值，也不可假設它一定是空的。
//   2. 一個參數只能轉發一次。轉發兩次等於允許被搬兩次，第二次拿到的
//      內容不確定。迴圈中重複使用的引數千萬別 forward（見本章第 5 檔）。
//   3. std::forward 不加模板引數就沒有意義；而對「具名右值引用參數」
//      要用 std::move，兩者不可混用。
//   4. 完美轉發不是萬能：它無法轉發 brace-init-list（{1,2,3} 沒有型別，
//      無法推導）、無法轉發 0/NULL 當空指標（會推成 int），
//      這類情況要在呼叫端寫明 std::initializer_list 或 nullptr。
//   5. 轉發建構子是 greedy overload，會搶走複製建構——見本章第 3、4 檔。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】完美轉發與變參工廠
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 完美轉發到底解決什麼問題？為什麼不能一律用 const T&？
//     答：const T& 雖然避免了複製，卻把「右值」這個資訊抹掉了。引數一旦被綁成
//         const&，下游就只剩複製這條路，原本可以移動的臨時物件被迫複製。
//         而 T& 則收不了右值，T（傳值）又多一次複製。
//         只有 T&& + std::forward 能讓左值以左值、右值以右值原樣抵達下游。
//     追問：那為什麼不乾脆多寫幾個重載？→ N 個參數要 2^N 個重載，
//           兩個參數就 4 個、三個就 8 個，組合爆炸；而且 move-only 型別
//           （unique_ptr）根本沒有 const& 版可寫。
//
// 🔥 Q2. T&& 什麼時候是右值引用，什麼時候是 forwarding reference？
//     答：只有當 T 是**本函式自己要推導**的模板參數、且形式剛好是 T&& 時，
//         才是 forwarding reference。加了 const、包了一層（vector<T>&&）、
//         或 T 早在類別實例化時就定了（類別模板的成員函式），
//         都只是普通的右值引用。
//     追問：auto&& 算哪一種？→ 算 forwarding reference，它走的是同一套推導規則；
//           C++20 起 void f(auto&& x) 這種簡寫函式模板也是。
//
// 🔥 Q3. emplace_back 和 push_back 差在哪？是不是永遠該用 emplace_back？
//     答：push_back 收「已建好的物件」，呼叫端得先造臨時物件再移動進去；
//         emplace_back 收「建構元素所需的引數」，完美轉發到元素建構子，
//         直接在容器記憶體就地建構，省掉臨時物件與那一次移動。
//         但不是永遠該用：emplace_back 會呼叫 **explicit** 建構子，
//         v.emplace_back(10) 對 vector<Foo> 會意外呼叫 explicit Foo(int)，
//         push_back 反而會擋下來。語意上想「放一個現成的物件」就用 push_back。
//     追問：emplace_back 的回傳值是什麼？→ C++11/14 回傳 void，
//           **C++17 起改回傳 reference**（指向新元素），才能寫
//           auto& s = v.emplace_back(...);。
//
// ⚠️ 陷阱. 下面這個「泛型記錄器」為什麼是錯的？
//         template<class T> void log_twice(T&& x) {
//             sink(std::forward<T>(x));
//             sink(std::forward<T>(x));   // ← 問題在這
//         }
//     答：同一個引數被轉發了兩次。當呼叫端傳入右值時，第一次 forward 會把
//         內容搬走，第二次 sink 收到的物件處於 valid but unspecified 狀態，
//         不能假設它還留著原值。正確做法是只在**最後一次使用**時才 forward，
//         前面幾次用具名的左值 x。
//     為什麼會錯：多數人把 std::forward 當成「一個安全的傳遞動作」，
//         以為它只是換個寫法傳參數。實際上它是把「可以被搬走」的許可交出去；
//         許可只能給一次。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class Connection {
    std::string host_;
    int port_;
    std::string protocol_;
public:
    template<typename H, typename P>
    Connection(H&& h, int p, P&& proto)
        : host_(std::forward<H>(h)), port_(p), protocol_(std::forward<P>(proto)) {
        std::cout << "  [建構] host=" << host_ << " port=" << port_
                  << " proto=" << protocol_ << "\n";
    }
    void print() const {
        std::cout << "  Connection(" << host_ << ":" << port_
                  << ", " << protocol_ << ")\n";
    }
};

// 一個函式涵蓋所有情況
template<typename... Args>
std::unique_ptr<Connection> make_connection(Args&&... args) {
    return std::unique_ptr<Connection>(
        new Connection(std::forward<Args>(args)...)
    );
}

// -----------------------------------------------------------------------------
// 【日常實務範例】連線池：用 emplace_back 就地建構，避免每次建立 session
//                 都多付一次「臨時物件 + 移動 + 解構」的代價
//
// 情境：伺服器維護一個 session pool。每個 Session 內含 std::string（會配置
//       記憶體），數量一多，多出來的臨時物件與移動就是實打實的成本。
//       這裡用靜態計數器把「push_back 比 emplace_back 多做了什麼」數出來。
//
// 注意：pool.reserve() 是必要的。若不先 reserve，vector 擴容時搬移既有元素
//       也會計入 move 次數，就分不清是 push_back 造成的還是擴容造成的。
// -----------------------------------------------------------------------------
struct Session {
    std::string user_;
    int id_;

    // 這三個計數器刻意用「類別外定義」的非 constexpr static 成員，
    // 取位址/繫結參考都不會有 odr-use 連結問題。
    static int ctors;
    static int copies;
    static int moves;

    Session(std::string u, int id) : user_(std::move(u)), id_(id) { ++ctors; }
    Session(const Session& o) : user_(o.user_), id_(o.id_) { ++copies; }
    Session(Session&& o) noexcept : user_(std::move(o.user_)), id_(o.id_) { ++moves; }

    Session& operator=(const Session&) = delete;
    Session& operator=(Session&&) = delete;
};
int Session::ctors  = 0;
int Session::copies = 0;
int Session::moves  = 0;

static void reset_counters() {
    Session::ctors = Session::copies = Session::moves = 0;
}

static void report(const char* tag) {
    std::cout << "  " << tag
              << " → 一般建構 " << Session::ctors
              << " 次、複製 "   << Session::copies
              << " 次、移動 "   << Session::moves << " 次\n";
}

int main() {
    std::string host = "example.com";
    std::string proto = "https";

    std::cout << "=== 全部左值 ===\n";
    auto c1 = make_connection(host, 8080, proto);
    c1->print();

    std::cout << "\n=== 全部右值 ===\n";
    auto c2 = make_connection(std::string("localhost"), 3000, std::string("http"));
    c2->print();

    std::cout << "\n=== 混合 ===\n";
    auto c3 = make_connection(host, 443, std::string("wss"));
    c3->print();

    // 轉發過後，host 與 proto 都還在（上面三次都是以左值轉發，走複製）
    std::cout << "\n=== 以左值轉發後，來源仍然完好 ===\n";
    std::cout << "  host=" << host << " proto=" << proto << "\n";

    std::cout << "\n=== 日常實務：push_back vs emplace_back ===\n";
    {
        std::vector<Session> pool;
        pool.reserve(4);           // 排除擴容造成的移動干擾

        reset_counters();
        pool.push_back(Session("alice", 1));   // 先造臨時物件，再移動進容器
        report("push_back(Session(\"alice\", 1))");

        reset_counters();
        pool.emplace_back("bob", 2);           // 直接在容器記憶體上建構
        report("emplace_back(\"bob\", 2)      ");

        // C++17 起 emplace_back 回傳新元素的 reference（C++11/14 回傳 void）
        auto& latest = pool.emplace_back("carol", 3);
        std::cout << "  emplace_back 回傳的 reference（C++17 起）: "
                  << latest.user_ << " (id=" << latest.id_ << ")\n";

        std::cout << "  pool 內共 " << pool.size() << " 個 session\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.7 章：完美轉發 (Perfect Forwarding) — 泛型程式設計的關鍵技術1.cpp" -o pf1

// === 預期輸出 ===
// === 全部左值 ===
//   [建構] host=example.com port=8080 proto=https
//   Connection(example.com:8080, https)
//
// === 全部右值 ===
//   [建構] host=localhost port=3000 proto=http
//   Connection(localhost:3000, http)
//
// === 混合 ===
//   [建構] host=example.com port=443 proto=wss
//   Connection(example.com:443, wss)
//
// === 以左值轉發後，來源仍然完好 ===
//   host=example.com proto=https
//
// === 日常實務：push_back vs emplace_back ===
//   push_back(Session("alice", 1)) → 一般建構 1 次、複製 0 次、移動 1 次
//   emplace_back("bob", 2)       → 一般建構 1 次、複製 0 次、移動 0 次
//   emplace_back 回傳的 reference（C++17 起）: carol (id=3)
//   pool 內共 3 個 session
