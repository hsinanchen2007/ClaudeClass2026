// =============================================================================
//  第 10 課：vector 的宣告與初始化方式 8  —  自訂類別的 vector 初始化
// =============================================================================
//
// 【主題資訊 Information】
//   vector(std::initializer_list<T> init, const Allocator& = Allocator());
//   標頭檔：<vector>；std::initializer_list 本身在 <initializer_list>
//           （被 <vector> 間接引入，但明確使用時應自行 include）。
//   標準版本：C++11。
//   複雜度：O(N) 次「複製建構」——注意是複製，不是移動。原因見下。
//   本檔的關鍵事實：
//     std::initializer_list<T> 的元素型別是 const T，
//     所以【永遠無法從中移動】，只能複製。
//
// 【詳細解釋 Explanation】
//
// 【1. `{"Alice", 30}` 是怎麼變成一個 Person 的】
//   寫下
//       std::vector<Person> people = { {"Alice", 30}, {"Bob", 25} };
//   時發生了兩層轉換：
//     ① 外層的 { … } 建立一個 std::initializer_list<Person>，
//        長度為 2，是一個【編譯期就配置好的唯讀陣列】。
//     ② 內層的 {"Alice", 30} 用來建構那個陣列裡的每個 Person。
//        因為 Person 有 Person(const std::string&, int) 這個建構子，
//        且它不是 explicit，所以大括號可以走這個建構子做「複製列表初始化」。
//   如果把 Person 的建構子加上 explicit，第 ② 步就會編譯失敗——
//   你必須改寫成 { Person("Alice", 30), Person("Bob", 25) }。
//
// 【2. 這個寫法有一個藏得很深的效能陷阱】
//   std::initializer_list<T> 的底層是 `const T*` 加一個長度。
//   注意那個 const。vector 的 initializer_list 建構子拿到的是
//   const Person&，所以它只能呼叫【複製建構子】，
//   即使 Person 有完美的移動建構子也用不上。
//   對於元素是 std::string、std::vector 這種「持有堆積資源」的型別，
//   這代表每個元素都要做一次深複製。
//   本檔的 main 用一個會計數的 Tracked 型別把這件事實測出來。
//   要避開它，只能改用 push_back / emplace_back / reserve + emplace_back。
//
// 【3. emplace_back 為什麼能省掉那次複製】
//   emplace_back 是可變參數模板，它把你給的參數【原封不動轉發】
//   到元素的建構子，直接在 vector 的記憶體上就地建構（placement new）：
//       people.emplace_back("Alice", 30);   // 只建構一次，零複製
//   對比
//       people.push_back(Person("Alice", 30));  // 建構一個暫存 + 移動進去
//   前者省掉暫存物件，後者至少省掉深複製（因為暫存是右值，能移動）。
//   而 initializer_list 版本兩者都省不掉。
//
// 【4. 為什麼 Person 需要自己寫建構子】
//   本檔的 Person 寫了 Person(const std::string&, int)。
//   一旦你宣告了任何建構子，編譯器就【不再】提供預設建構子，
//   於是 std::vector<Person> v(3); 會編譯失敗（它需要預設建構 3 個元素）。
//   反過來，若 Person 是純粹的聚合（沒有任何使用者宣告的建構子、
//   沒有 private 成員、沒有虛擬函式），就能用聚合初始化，
//   連建構子都不必寫——本檔的 main 有對照示範。
//
// 【概念補充 Concept Deep Dive】
//
// (A) initializer_list 的儲存期
//     它背後的陣列是一個「暫存物件」，生命週期只到完整運算式結束
//     （若綁到具名的 initializer_list 變數則延長到該變數的生命週期）。
//     所以下面這種寫法是懸空的：
//         std::initializer_list<int> bad() { return {1, 2, 3}; }  // 危險
//     回傳後底層陣列已經死了。initializer_list 只該當參數用，
//     不該當回傳值或成員存起來。
//
// (B) 為什麼 initializer_list 建構子的優先權特別高
//     只要類別有 initializer_list 建構子，大括號初始化就會【優先】
//     選它，甚至不惜做窄化以外的次佳轉換。這就是
//         std::vector<int> v(5, 10);   // 五個 10
//         std::vector<int> v{5, 10};   // 兩個元素 5 和 10
//     差這麼多的原因。這個「大括號偏心」規則是 C++11 為了讓
//     `{...}` 直覺地表示「元素清單」而刻意設計的。
//
// (C) 聚合初始化 vs 建構子（C++17 / C++20 的變化）
//     struct P { std::string name; int age; };   // 聚合，沒有使用者建構子
//     P p{"Alice", 30};                           // 聚合初始化，直接寫成員
//     C++20 起還能用指定初始化（designated initializer）：
//     P p{.name = "Alice", .age = 30};            // C++20
//     聚合的好處是不必寫建構子、支援 vector<P> v(3)（成員各自值初始化）。
//     代價是失去「建構時就檢查不變式」的機會。
//
// 【注意事項 Pay Attention】
//   1. initializer_list 一定是複製，不是移動。元素含堆積資源時，
//      大量資料請改用 reserve + emplace_back。
//   2. 一旦自訂了任何建構子，預設建構子就消失了，
//      vector<Person> v(3); 會編譯失敗。需要的話寫 Person() = default;。
//   3. 建構子加 explicit 之後，{"Alice", 30} 這種內層大括號會失效。
//   4. 別把 initializer_list 存起來或當回傳值——它背後的陣列是暫存物件。
//   5. 只要類別有 initializer_list 建構子，大括號就會優先選它。
//      要明確呼叫其他建構子請改用小括號。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】自訂類別的 vector 初始化
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. `std::vector<Person> v = {{"Alice",30}, {"Bob",25}};`
//        每個 Person 被建構了幾次？有沒有移動？
//     答：每個元素至少兩次操作：先在 initializer_list 的底層陣列上
//         建構一個 Person，再【複製建構】進 vector 的記憶體。
//         完全不會發生移動——因為 initializer_list 的元素型別是 const T，
//         const 物件無法被移動走資源。
//     追問：那要怎麼避免這次複製？
//         → 用 v.reserve(n) 之後 v.emplace_back("Alice", 30)，
//           參數直接完美轉發到元素建構子，就地建構，零複製零移動。
//
// 🔥 Q2. 為什麼 `std::vector<Person> v(3);` 在 Person 自訂建構子後
//        會編譯失敗？
//     答：vector<T>(n) 這個建構子要求 T 是「可預設建構」的
//         （它要建立 n 個值初始化的元素）。而一旦類別宣告了任何
//         使用者定義的建構子，編譯器就不再自動生成預設建構子。
//         解法是明確寫 Person() = default;（且所有成員都能預設建構）。
//     追問：如果 Person 只有 explicit 建構子，vector<Person> 還能用嗎？
//         → 能，但初始化寫法受限：不能用 {{"Alice",30}} 的內層大括號，
//           必須明確寫出 Person("Alice", 30)。
//
// ⚠️ 陷阱. 「我的元素型別有 noexcept 的移動建構子，所以用
//         `vector<T> v = {a, b, c};` 初始化時 STL 會自動用移動，很有效率」
//         ——這個推論錯在哪？
//     答：錯得很徹底。std::initializer_list<T> 的元素型別是 const T，
//         const 物件不可能被移動（移動要修改來源）。所以無論你的移動
//         建構子多完美、標不標 noexcept，這條路徑一律走複製建構。
//         元素是 std::string、std::vector 時就是實實在在的深複製。
//     為什麼會錯：把「編譯器會盡量用移動」這個一般直覺，
//         套用到一個型別上明確禁止移動的場合。
//         移動語意的前提是「來源是可修改的右值」，
//         而 initializer_list 給的是 const 左值——前提根本不成立。
//         本檔的 main 用計數器實測，會看到 0 次移動、N 次複製。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <string>
#include <iostream>

struct Person {
    std::string name;
    int age;

    Person(const std::string& n, int a) : name(n), age(a) {}
};

// -----------------------------------------------------------------------------
// 用來實測「initializer_list 到底是複製還是移動」的計數型別。
// 三個靜態計數器分別記錄：一般建構、複製建構、移動建構。
// -----------------------------------------------------------------------------
struct Tracked {
    std::string payload;
    static int ctor;
    static int copy;
    static int move;

    explicit Tracked(std::string p) : payload(std::move(p)) { ++ctor; }
    Tracked(const Tracked& o) : payload(o.payload) { ++copy; }
    Tracked(Tracked&& o) noexcept : payload(std::move(o.payload)) { ++move; }
    Tracked& operator=(const Tracked&) = default;
    Tracked& operator=(Tracked&&) = default;
    ~Tracked() = default;

    static void reset() { ctor = copy = move = 0; }
    static void report(const char* label) {
        std::cout << label
                  << " 一般建構=" << ctor
                  << " 複製建構=" << copy
                  << " 移動建構=" << move << "\n";
    }
};
int Tracked::ctor = 0;
int Tracked::copy = 0;
int Tracked::move = 0;

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔刻意不放
//   理由：本檔的主題是「自訂型別怎麼被建構進 vector」，關心的是
//   複製 / 移動 / 就地建構的次數，屬於資源管理議題。
//   LeetCode 的輸入一律是內建型別的陣列（int / string），
//   建構成本不是任何一題的考點，硬套只會離題。
//   真正會被這件事咬到的是下面那種「載入設定 / 建物件清單」的實務程式。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】從設定檔載入後端伺服器清單
//   情境：反向代理 / 負載平衡器啟動時，讀 config 把每台後端的
//         host、port、權重建成一份清單。
//   為什麼用到本主題：這正是「自訂類別 + vector」最常見的樣子。
//     而且它示範了正確的建構姿勢：先 reserve（筆數已知），
//     再用 emplace_back 就地建構——完全不產生暫存物件，
//     不做任何一次深複製。對比用 initializer_list 一次列舉，
//     後者在筆數多、成員含字串時會付出可觀的複製成本。
// -----------------------------------------------------------------------------
struct Backend {
    std::string host;
    int         port;
    int         weight;

    Backend(std::string h, int p, int w)
        : host(std::move(h)), port(p), weight(w) {}
};

std::vector<Backend> loadBackends(const std::vector<std::string>& rawLines) {
    std::vector<Backend> out;
    out.reserve(rawLines.size());          // 筆數已知 → 一次配置到位

    for (const auto& line : rawLines) {
        // 格式： host:port:weight
        size_t c1 = line.find(':');
        if (c1 == std::string::npos) continue;
        size_t c2 = line.find(':', c1 + 1);
        if (c2 == std::string::npos) continue;

        std::string host = line.substr(0, c1);
        int port   = std::stoi(line.substr(c1 + 1, c2 - c1 - 1));
        int weight = std::stoi(line.substr(c2 + 1));

        // emplace_back：參數完美轉發到 Backend 的建構子，就地建構
        out.emplace_back(std::move(host), port, weight);
    }
    return out;
}

int main() {
    std::cout << "=== 原始示範：initializer_list 建構 vector<Person> ===\n";

    // 使用初始化串列（需要隱式轉換或大括號建構）
    std::vector<Person> people = {
        {"Alice", 30},
        {"Bob", 25},
        {"Charlie", 35}
    };

    for (const auto& p : people) {
        std::cout << p.name << " is " << p.age << " years old." << std::endl;
    }

    std::cout << "\n=== 實測：initializer_list 是複製還是移動？ ===\n";
    {
        Tracked::reset();
        std::vector<Tracked> a = { Tracked("x"), Tracked("y"), Tracked("z") };
        Tracked::report("initializer_list :");
        std::cout << "  → 移動建構是 0 次。initializer_list 的元素型別是 const T，\n";
        std::cout << "    const 物件無法被移動，所以只能走複製建構。\n";
        std::cout << "    （size=" << a.size() << "）\n";
    }
    {
        Tracked::reset();
        std::vector<Tracked> b;
        b.reserve(3);                       // 先配置，避免擴容時的額外搬移
        b.emplace_back("x");
        b.emplace_back("y");
        b.emplace_back("z");
        Tracked::report("reserve+emplace  :");
        std::cout << "  → 零複製、零移動：參數直接轉發，在 vector 的記憶體上就地建構。\n";
        std::cout << "    （size=" << b.size() << "）\n";
    }
    {
        Tracked::reset();
        std::vector<Tracked> c;
        c.reserve(3);
        c.push_back(Tracked("x"));          // 建暫存 → 移動進去
        c.push_back(Tracked("y"));
        c.push_back(Tracked("z"));
        Tracked::report("reserve+push_back:");
        std::cout << "  → 暫存物件是右值，所以走移動建構（比複製便宜，但仍多一步）。\n";
        std::cout << "    （size=" << c.size() << "）\n";
    }

    std::cout << "\n=== 聚合型別：連建構子都不必寫 ===\n";
    {
        // 沒有任何使用者宣告的建構子 → 聚合型別 → 可用聚合初始化
        struct Point { int x; int y; };
        std::vector<Point> pts = {{1, 2}, {3, 4}, {5, 6}};
        std::cout << "vector<Point> 內容: ";
        for (const auto& p : pts) std::cout << "(" << p.x << "," << p.y << ") ";
        std::cout << "\n";

        // 聚合型別可以預設建構，所以 vector<Point>(3) 合法
        std::vector<Point> zeros(3);
        std::cout << "vector<Point>(3) 內容: ";
        for (const auto& p : zeros) std::cout << "(" << p.x << "," << p.y << ") ";
        std::cout << "  ← 成員被值初始化為 0\n";
        std::cout << "對比 Person 有自訂建構子 → vector<Person>(3) 會編譯失敗\n";
    }

    std::cout << "\n=== 日常實務：從設定檔載入後端伺服器清單 ===\n";
    {
        std::vector<std::string> config = {
            "10.0.0.11:8080:5",
            "10.0.0.12:8080:3",
            "10.0.0.13:9090:1",
            "bad-line-without-colons",       // 格式錯誤，應被略過
            "10.0.0.14:8443:2",
        };

        auto backends = loadBackends(config);
        std::cout << "設定共 " << config.size() << " 行，成功解析 "
                  << backends.size() << " 台後端：\n";

        int totalWeight = 0;
        for (const auto& b : backends) {
            std::cout << "  " << b.host << ":" << b.port
                      << "  weight=" << b.weight << "\n";
            totalWeight += b.weight;
        }
        std::cout << "權重總和: " << totalWeight << "\n";
        std::cout << "vector capacity=" << backends.capacity()
                  << "（reserve 了 " << config.size() << "，只配置一次）\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 10 課：vector 的宣告與初始化方式8.cpp -o demo8

// === 預期輸出 ===
// === 原始示範：initializer_list 建構 vector<Person> ===
// Alice is 30 years old.
// Bob is 25 years old.
// Charlie is 35 years old.
//
// === 實測：initializer_list 是複製還是移動？ ===
// initializer_list : 一般建構=3 複製建構=3 移動建構=0
//   → 移動建構是 0 次。initializer_list 的元素型別是 const T，
//     const 物件無法被移動，所以只能走複製建構。
//     （size=3）
// reserve+emplace  : 一般建構=3 複製建構=0 移動建構=0
//   → 零複製、零移動：參數直接轉發，在 vector 的記憶體上就地建構。
//     （size=3）
// reserve+push_back: 一般建構=3 複製建構=0 移動建構=3
//   → 暫存物件是右值，所以走移動建構（比複製便宜，但仍多一步）。
//     （size=3）
//
// === 聚合型別：連建構子都不必寫 ===
// vector<Point> 內容: (1,2) (3,4) (5,6)
// vector<Point>(3) 內容: (0,0) (0,0) (0,0)   ← 成員被值初始化為 0
// 對比 Person 有自訂建構子 → vector<Person>(3) 會編譯失敗
//
// === 日常實務：從設定檔載入後端伺服器清單 ===
// 設定共 5 行，成功解析 4 台後端：
//   10.0.0.11:8080  weight=5
//   10.0.0.12:8080  weight=3
//   10.0.0.13:9090  weight=1
//   10.0.0.14:8443  weight=2
// 權重總和: 11
// vector capacity=5（reserve 了 5，只配置一次）
