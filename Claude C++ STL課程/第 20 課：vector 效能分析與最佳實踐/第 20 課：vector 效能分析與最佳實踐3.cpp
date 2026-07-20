// =============================================================================
//  第 20 課：vector 效能分析與最佳實踐 3
//  —  push_back vs emplace_back：看得見的建構／拷貝／移動軌跡
// =============================================================================
//
// 【主題資訊 Information】
//   void push_back(const T& value);              // (1) 拷貝一份進容器
//   void push_back(T&& value);                   // (2) C++11：移動一份進容器
//   template<class... Args>
//   reference emplace_back(Args&&... args);      // (3) C++11：用 args 直接原地建構
//                                                //     C++17 起回傳 reference
//
//   標準版本：push_back(T&&) 與 emplace_back 皆為 C++11；
//             emplace_back 的回傳值型別在 C++17 之前是 void
//   複雜度  ：均攤 O(1)
//   標頭檔  ：<vector>
//
// 【詳細解釋 Explanation】
//
// 【1. 兩者的根本差異：誰負責建構那個物件】
//   * push_back 收的是「一個已經存在的 T」。你得先在別的地方把它做出來，
//     push_back 再把它拷貝或移動到容器的記憶體裡。
//         v.push_back(Widget("A", 1));
//         ① 在 stack 上建構臨時 Widget
//         ② 移動建構到容器內部（有移動建構子時）
//         ③ 臨時物件解構
//   * emplace_back 收的是「建構 T 所需要的參數」。它把參數用完美轉發
//     （perfect forwarding）送到容器記憶體上，直接呼叫建構子。
//         v.emplace_back("B", 2);
//         ① 直接在容器內部建構 —— 沒有臨時物件、沒有移動、沒有解構
//
// 【2. 完美轉發是怎麼做到的】
//   emplace_back 的簽名是 template<class... Args> reference emplace_back(Args&&... args)。
//   這裡的 Args&& 是「轉發參考（forwarding reference）」，不是右值參考：
//     * 傳左值進來，Args 推導成 T&，Args&& 摺疊成 T&
//     * 傳右值進來，Args 推導成 T ，Args&& 就是 T&&
//   內部再用 std::forward<Args>(args)... 把「值類別（value category）」原封不動
//   交給 placement new：
//         ::new ((void*)ptr) T(std::forward<Args>(args)...);
//   所以「你原本傳什麼，建構子就收到什麼」——這就是「完美」的意思。
//
// 【3. push_back 的兩個重載怎麼選】
//   多數人只記得「emplace_back 比較快」，但 push_back 本身也分兩種：
//         Widget w("C", 3);
//         v.push_back(w);              // 左值 -> 走 const T&  -> 拷貝
//         v.push_back(std::move(w));   // 右值 -> 走 T&&       -> 移動
//   如果你手上已經有一個具名物件而且之後不再用它，
//   push_back(std::move(w)) 和 emplace_back(std::move(w)) 的成本是一樣的
//   （都是一次移動建構）。emplace_back 的優勢只在「你本來就要現場建一個新物件」。
//
// 【4. 什麼時候 emplace_back 其實沒有比較快】
//   * 元素是 int/double 這種 trivially copyable 的型別 -> 兩者幾乎相同
//   * 你傳進去的本來就是同型別的物件 -> 兩者都只是一次拷貝或移動
//   * 型別有便宜的移動建構子（如 std::string）-> 差距只有「省下一次移動」，
//     不是省下一次深拷貝
//   真正明顯的場合是：建構成本高、而且你手上只有「建構參數」不是「物件」。
//
// 【5. noexcept 移動建構子：重新配置時的生死線】
//   vector 重新配置時要把舊元素搬到新記憶體。它面臨一個兩難：
//     搬到一半如果丟出例外，舊的搬走了、新的沒搬完 —— 資料就毀了。
//   標準要求 push_back 提供「強例外保證（strong exception guarantee）」：
//     要嘛成功，要嘛容器維持原狀。
//   所以實作用 std::move_if_noexcept：
//     * 移動建構子標了 noexcept -> 放心用「移動」（快，而且不會半途爆炸）
//     * 移動建構子沒標 noexcept  -> 只好退回用「拷貝」（慢，但舊資料還在，
//                                   失敗時可以原樣還原）
//   結論：**自訂型別的移動建構子忘了寫 noexcept，vector 擴容時會默默退化成拷貝。**
//   這是實務上最常見、最昂貴、也最難察覺的效能地雷之一。
//   本檔第 5 段用計數器把這件事印出來（本機實測：noexcept 版 0 拷貝、
//   非 noexcept 版 0 移動全部走拷貝）。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼是「移動 7 次」而不是 5 次
//     從空 vector 連續 emplace_back 5 次，容量走 0->1->2->4->8，
//     在 size 到 1、2、4 時各發生一次重新配置，分別要搬 1、2、4 個舊元素
//     = 1 + 2 + 4 = 7 次移動／拷貝。這正是倍增論證裡那條等比級數，
//     加上 5 次原地建構，就是本機實測的完整數字。
//     -> 這也再次說明 reserve 的價值：先 reserve(5)，這 7 次全部消失。
//
// (B) emplace_back 回傳 reference 是 C++17 才有的
//     C++11/14 的 emplace_back 回傳 void，所以
//         auto& w = v.emplace_back("X", 1);
//     在 C++17 之前編不過。要相容舊標準得寫成
//         v.emplace_back("X", 1); auto& w = v.back();
//
// (C) emplace_back 會繞過 explicit 檢查
//     emplace_back 內部是直接 T(args...)，屬於 direct-initialization，
//     所以 explicit 建構子照樣叫得到；push_back 走的是隱式轉換，
//     explicit 建構子會被擋下來。這代表 emplace_back「比較寬鬆」——
//     好處是彈性，壞處是某些本來該被編譯器擋下的寫法會悄悄通過（見第 5 檔）。
//
// 【注意事項 Pay Attention】
//
// 1. 本檔第 1~4 段有預先 reserve(4)，目的是**排除重新配置的干擾**，
//    讓輸出只反映 push_back/emplace_back 本身的行為。沒有 reserve 的話，
//    輸出裡會混進擴容造成的額外移動（第 5 段刻意示範這件事）。
// 2. 移動之後的來源物件處於「合法但未指定（valid but unspecified）」狀態。
//    標準只保證它還能被解構與賦值，**不保證是空的**。
//    對 std::string/std::vector 這類容器，主流實作移動後會是空的，
//    但那是實作行為，不是標準保證，不要寫程式邏輯去依賴它。
// 3. 自訂型別的移動建構子請一律加 noexcept（只要它真的不會丟）。
//    忘記加，vector 擴容就會退化成拷貝，而且**編譯器不會給你任何警告**。
// 4. emplace_back 不是「永遠比較好」。傳入已存在的同型別物件時兩者等價；
//    而且 emplace_back 的寬鬆轉發會讓某些意外的建構子被選中。
// 5. 這裡示範用的 Widget 在建構子裡做 I/O（cout），只是為了讓過程可見；
//    真實程式的建構子不該有這種副作用。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】push_back vs emplace_back
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. v.push_back(Widget("A",1)) 和 v.emplace_back("A",1) 各發生幾次建構？
//     答：push_back 版是「一次一般建構（臨時物件）+ 一次移動建構（進容器）
//         + 一次解構（臨時物件）」；emplace_back 版只有「一次一般建構」，
//         直接在容器記憶體上原地建構，沒有臨時物件。
//     追問：如果 Widget 沒有移動建構子呢？
//         → push_back 版的第二步會退化成拷貝建構。
//
// 🔥 Q2. 已經有一個具名物件 w，push_back(w) 和 emplace_back(w) 差在哪？
//     答：幾乎沒差，兩者都是一次拷貝建構（w 是左值）。
//         emplace_back 的優勢只在「現場用參數建構新物件」的情境。
//         要省成本應該寫 push_back(std::move(w))，那才是一次移動。
//     追問：那 emplace_back(std::move(w)) 呢？
//         → 也是一次移動，和 push_back(std::move(w)) 完全等價。
//
// ⚠️ 陷阱. 移動建構子沒寫 noexcept，對 vector 有什麼影響？
//     答：vector 重新配置時會改用**拷貝**而不是移動。因為 push_back 必須提供
//         強例外保證，實作用 std::move_if_noexcept 判斷：移動可能丟例外時，
//         搬到一半失敗就無法復原，只好乖乖拷貝（原資料還在，可以還原）。
//         本機實測：同樣 5 次 emplace_back，noexcept 版 7 次移動 0 次拷貝；
//         拿掉 noexcept 後變成 0 次移動 7 次拷貝。
//     為什麼會錯：多數人以為「有寫移動建構子就一定會走移動」，
//         忽略了 noexcept 是這個決策的**唯一依據**——
//         而且退化時完全靜默，沒有警告、沒有錯誤，只有效能默默變差。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>

// -----------------------------------------------------------------------------
// 可觀察的 Widget：把每一次建構／拷貝／移動都印出來
// -----------------------------------------------------------------------------
class Widget {
    std::string name_;
    int value_;

public:
    Widget(const std::string& name, int value)
        : name_(name), value_(value) {
        std::cout << "  建構 Widget(" << name_ << ", " << value_ << ")" << std::endl;
    }

    Widget(const Widget& other)
        : name_(other.name_), value_(other.value_) {
        std::cout << "  拷貝 Widget(" << name_ << ")" << std::endl;
    }

    Widget(Widget&& other) noexcept
        : name_(std::move(other.name_)), value_(other.value_) {
        std::cout << "  移動 Widget(" << name_ << ")" << std::endl;
    }

    ~Widget() = default;
};

// -----------------------------------------------------------------------------
// 兩個只差一個 noexcept 的型別，用來量化「忘記 noexcept」的代價
// -----------------------------------------------------------------------------
struct FastMove {                       // 移動建構子有 noexcept
    std::string payload;
    static int copies;
    static int moves;

    explicit FastMove(const char* p) : payload(p) {}
    FastMove(const FastMove& o) : payload(o.payload) { ++copies; }
    FastMove(FastMove&& o) noexcept : payload(std::move(o.payload)) { ++moves; }
};
int FastMove::copies = 0;
int FastMove::moves  = 0;

struct SlowMove {                       // 完全一樣，只是移動建構子沒標 noexcept
    std::string payload;
    static int copies;
    static int moves;

    explicit SlowMove(const char* p) : payload(p) {}
    SlowMove(const SlowMove& o) : payload(o.payload) { ++copies; }
    SlowMove(SlowMove&& o) : payload(std::move(o.payload)) { ++moves; }
};
int SlowMove::copies = 0;
int SlowMove::moves  = 0;

// -----------------------------------------------------------------------------
// 【日常實務範例 1】從設定檔建立連線設定清單
//   情境：讀 ini/yaml 之後要建立一串 ConnectionConfig。
//     每筆設定的建構參數（host、port、timeout）是解析出來的散裝值，
//     不是現成的 ConnectionConfig 物件 —— 這正是 emplace_back 的主場：
//     直接把三個參數轉發到容器裡建構，不做出中間的臨時物件。
//   對照組：如果先 ConnectionConfig c{...} 再 push_back(c)，
//     就多了一次拷貝（或至少一次移動）。
// -----------------------------------------------------------------------------
struct ConnectionConfig {
    std::string host;
    int port;
    int timeoutMs;

    ConnectionConfig(std::string h, int p, int t)
        : host(std::move(h)), port(p), timeoutMs(t) {}
};

std::vector<ConnectionConfig> buildConfigs() {
    std::vector<ConnectionConfig> configs;
    configs.reserve(3);                       // 筆數已知 -> 零重新配置

    // 原地建構：三個參數直接轉發給 ConnectionConfig 的建構子
    configs.emplace_back("db.internal",    5432, 3000);
    configs.emplace_back("cache.internal", 6379, 500);
    configs.emplace_back("api.internal",   8080, 10000);

    return configs;
}

int main() {
    std::cout << "=== 1. push_back（臨時物件）===" << std::endl;
    {
        std::vector<Widget> v;
        v.reserve(4);                        // 排除重新配置的干擾
        v.push_back(Widget("A", 1));
        // 建構臨時物件 -> 移動到容器 -> 臨時物件解構
    }

    std::cout << "\n=== 2. emplace_back（原地建構）===" << std::endl;
    {
        std::vector<Widget> v;
        v.reserve(4);
        v.emplace_back("B", 2);
        // 直接在容器記憶體上建構，沒有臨時物件
    }

    std::cout << "\n=== 3. push_back（具名左值 -> 拷貝）===" << std::endl;
    {
        std::vector<Widget> v;
        v.reserve(4);
        Widget w("C", 3);
        v.push_back(w);                      // w 是左值 -> const T& -> 拷貝
    }

    std::cout << "\n=== 4. push_back（std::move -> 移動）===" << std::endl;
    {
        std::vector<Widget> v;
        v.reserve(4);
        Widget w2("D", 4);
        v.push_back(std::move(w2));          // 右值 -> T&& -> 移動
    }

    std::cout << "\n=== 5. noexcept 移動建構子的代價（重新配置時）===" << std::endl;
    {
        // 刻意「不」reserve，讓容量走 0->1->2->4->8，觸發 3 次重新配置
        {
            std::vector<FastMove> v;
            for (int i = 0; i < 5; ++i) v.emplace_back("payload");
        }
        {
            std::vector<SlowMove> v;
            for (int i = 0; i < 5; ++i) v.emplace_back("payload");
        }

        std::cout << "  移動建構子有 noexcept : 拷貝 " << FastMove::copies
                  << " 次, 移動 " << FastMove::moves << " 次" << std::endl;
        std::cout << "  移動建構子無 noexcept : 拷貝 " << SlowMove::copies
                  << " 次, 移動 " << SlowMove::moves << " 次" << std::endl;
        std::cout << "  -> 少一個 noexcept，vector 擴容就從「移動」退化成「拷貝」，"
                  << "而且完全沒有警告" << std::endl;
        std::cout << "  -> 搬移次數 1+2+4=7，正是倍增論證裡那條等比級數；"
                  << "先 reserve(5) 可讓它歸零" << std::endl;
    }

    std::cout << "\n=== 6. 日常實務：從設定檔建立連線設定 ===" << std::endl;
    {
        auto configs = buildConfigs();
        for (const auto& c : configs) {
            std::cout << "  " << c.host << ":" << c.port
                      << "  timeout=" << c.timeoutMs << "ms" << std::endl;
        }
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 20 課：vector 效能分析與最佳實踐3.cpp -o lesson20_3

// === 預期輸出 ===
// ※ 本檔沒有計時，輸出是決定性的。
// ※ 第 5 段的 7 次搬移來自本機 libstdc++ 的倍增策略（1+2+4）；
//    成長倍率屬實作定義，換成倍率 1.5 的 MSVC 這個數字會不同。
//
// === 1. push_back（臨時物件）===
//   建構 Widget(A, 1)
//   移動 Widget(A)
//
// === 2. emplace_back（原地建構）===
//   建構 Widget(B, 2)
//
// === 3. push_back（具名左值 -> 拷貝）===
//   建構 Widget(C, 3)
//   拷貝 Widget(C)
//
// === 4. push_back（std::move -> 移動）===
//   建構 Widget(D, 4)
//   移動 Widget(D)
//
// === 5. noexcept 移動建構子的代價（重新配置時）===
//   移動建構子有 noexcept : 拷貝 0 次, 移動 7 次
//   移動建構子無 noexcept : 拷貝 7 次, 移動 0 次
//   -> 少一個 noexcept，vector 擴容就從「移動」退化成「拷貝」，而且完全沒有警告
//   -> 搬移次數 1+2+4=7，正是倍增論證裡那條等比級數；先 reserve(5) 可讓它歸零
//
// === 6. 日常實務：從設定檔建立連線設定 ===
//   db.internal:5432  timeout=3000ms
//   cache.internal:6379  timeout=500ms
//   api.internal:8080  timeout=10000ms
