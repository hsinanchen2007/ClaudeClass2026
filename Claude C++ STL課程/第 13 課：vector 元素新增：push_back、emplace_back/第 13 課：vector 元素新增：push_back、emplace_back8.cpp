// =============================================================================
//  第 13 課：vector 元素新增：push_back、emplace_back8.cpp
//    —  破解迷思：「emplace_back 永遠比較快」是錯的
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<vector>
//   void      push_back(const T&);        // lvalue → 複製建構
//   void      push_back(T&&);             // rvalue → 移動建構
//   template<class... Args>
//   reference emplace_back(Args&&...);    // 完美轉發 → 就地建構
//   複雜度：皆為攤銷 O(1)
//
//   本檔用一個會計數的型別，把「兩者到底各做了幾次複製／移動／建構」
//   直接數出來，而不是憑印象爭論誰比較快。
//
// 【詳細解釋 Explanation】
//
// 【1. 迷思從哪裡來】
// 教學文章常說「emplace_back 就地建構，避免了複製」，很多人因此推論出
// 「emplace_back 永遠比 push_back 快，應該全面取代它」。
// 前半句沒錯，後半句是嚴重的過度推廣。
//
// 【2. 為什麼傳現成物件時兩者完全等價】
// 關鍵在 std::forward 的語意——它保留參數的**值類別 (value category)**：
//     std::string s = "Hello";
//     v.emplace_back(s);
// s 是 lvalue，Args 被推導成 std::string&，std::forward 轉發出來還是 lvalue，
// 最後在 vector 內部呼叫的是：
//     ::new (ptr) std::string(s);      // ← 這是複製建構子
// 跟 push_back(s) 做的事一模一樣。同理：
//     v.emplace_back(std::move(s));    // 轉發 rvalue → 移動建構子
//     v.push_back(std::move(s));       // 同樣是移動建構子
// 兩者在最佳化後通常生成完全相同的機器碼。
//
// 「就地建構」省掉的是**臨時物件**，不是「複製」這個動作本身。
// 你傳一個 lvalue 進去，本來就沒有臨時物件可以省。
//
// 【3. 那什麼時候真的有差】
// 只有當「push_back 被迫先造一個臨時 T」時：
//     v.push_back("Hello");      // const char* 不是 string
//                                //  → 造臨時 string → 移動進去 → 臨時物件解構
//     v.emplace_back("Hello");   // 直接在容器內用 string(const char*) 建構
// 本檔實測這組會看到明顯差異：push_back 多一次建構與一次移動。
//
// 一句話總結分界線：
//     手上是「成品」→ 兩者等價
//     手上是「零件」→ emplace_back 較優
//
// 【4. 為什麼「等價時」仍該選 push_back】
// 既然效能相同，就該用其他標準來選，而 push_back 在兩方面都更好：
//   (a) 意圖明確：push_back(s) 一眼看出「放一個現成物件進去」。
//   (b) 型別安全：emplace_back 走 direct-init，會繞過 explicit、
//       也不做 narrowing 檢查（見本課第 7 個範例檔），
//       參數打錯比較容易悄悄編過。
// 選 push_back 等於不花任何代價換到可讀性與安全性。
//
// 【概念補充 Concept Deep Dive】
// 這裡有一個很多人忽略的反例：**emplace_back 有時反而讓程式變差**。
// （注意：這裡講的不是「執行速度一定比較慢」，而是它讓昂貴或錯誤的操作
//   在程式碼上變得不顯眼——這是可讀性與可審查性的代價。）
//
// 最典型的例子是「emplace_back 讓你不小心建構了昂貴的物件」：
//     v.emplace_back(1000000, 'x');   // 就地建構一百萬字元的 string
// 這行看起來很無害，但它真的會配置 1MB。用 push_back 你至少會先寫出
// std::string(1000000, 'x')，昂貴之處一目了然。
//
// 另一個實務上的量測陷阱：如果你寫 benchmark 比較兩者，卻沒有先 reserve，
// 那你量到的其實是「reallocation 的成本」，它會完全淹沒 push/emplace 之間
// 那一次移動的差異。本檔所有測量都先 reserve，就是為了排除這個變因。
//
// 【注意事項 Pay Attention】
// 1. 傳現成同型別物件時，emplace_back **沒有**比較快，兩者完全等價。
// 2. emplace_back 的優勢只在「省掉臨時物件」，不在「避免複製」。
// 3. 量測兩者差異前一定要先 reserve，否則測到的是 reallocation 成本。
// 4. 本檔的計數是「建構子被呼叫的次數」，這是語意層面的事實；
//    最終效能還受編譯器最佳化（如 copy elision）影響。
// 5. 被 std::move 走的物件處於 valid but unspecified 狀態，不要再讀其值。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】emplace_back 不一定比較快
// ───────────────────────────────────────────────────────────────────────────
// ⚠️ 陷阱 Q1. 「emplace_back 一定比 push_back 快，所以專案應該全面改用」——對嗎？
//     答：錯。當傳入的已經是現成的同型別物件時，兩者**完全等價**：
//             std::string s = "Hello";
//             v.push_back(s);      // 複製建構
//             v.emplace_back(s);   // 也是複製建構！
//         本檔實測兩者的複製次數都是 1、移動次數都是 0。
//         emplace_back 只有在「原本需要先造一個臨時物件」時才勝出。
//     為什麼會錯：腦中的模型是「emplace = 就地建構 = 不會複製」，
//         漏掉了 std::forward 會**保留值類別**。傳 lvalue 進去，
//         轉發出來仍是 lvalue，最後呼叫的依然是複製建構子。
//         就地建構省的是「臨時物件」，不是「複製」本身。
//
// 🔥 Q2. 請具體說明 v.push_back("Hello") 與 v.emplace_back("Hello")
//         對 vector<std::string> 的差別。
//     答：push_back 的參數型別是 std::string，"Hello" 是 const char*，
//         編譯器得先呼叫 string(const char*) 造一個臨時 string，
//         再把它移動進容器，最後解構臨時物件——共 1 次建構 + 1 次移動。
//         emplace_back 把 const char* 完美轉發進去，直接在容器的記憶體上
//         呼叫 string(const char*)——只有 1 次建構，沒有移動、沒有臨時物件。
//     追問：這個差別在實務上重要嗎？
//         → 視型別而定。短字串因為 SSO 幾乎沒差；
//           但若 T 的移動成本高、或迴圈跑數百萬次，差距就會浮現。
//
// 🔥 Q3. 如果兩者效能相同，該用哪一個？為什麼？
//     答：用 push_back。效能既然相同，就該用其他標準決定，而 push_back
//         在兩方面更好：意圖明確（一看就知道是放現成物件），
//         以及型別安全（emplace_back 走 direct-init，會繞過 explicit、
//         不做 narrowing 檢查，參數打錯容易悄悄編過）。
//     追問：那什麼時候該堅持用 emplace_back？
//         → 元素建構子需要多個參數、或你手上只有零件而非成品時。
//
// ⚠️ 陷阱 Q4. 有沒有 emplace_back 反而讓程式變差的情況？
//     答：有。emplace_back(1000000, 'x') 會就地建構一個一百萬字元的 string，
//         配置 1MB 記憶體，但這行看起來人畜無害；
//         寫成 push_back(std::string(1000000, 'x')) 反而讓昂貴之處一目了然。
//         此外 emplace_back 繞過 explicit 與窄化檢查，
//         也會讓型別錯誤更晚才被發現。
//     為什麼會錯：只從「省一次移動」的角度看 emplace_back，
//         忽略了它同時降低了程式碼的**可讀性與可審查性**。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <string>
#include <iomanip>

// 會計數的字串包裝：用來精確數出各種呼叫做了幾次什麼
struct Counted {
    std::string data;

    static int ctorFromLiteral;   // 從 const char* 建構（就地建構的證據）
    static int copyCtor;          // 複製建構
    static int moveCtor;          // 移動建構

    Counted(const char* s) : data(s) { ++ctorFromLiteral; }
    Counted(const Counted& o) : data(o.data) { ++copyCtor; }
    Counted(Counted&& o) noexcept : data(std::move(o.data)) { ++moveCtor; }

    static void reset() { ctorFromLiteral = copyCtor = moveCtor = 0; }
    static void report(const std::string& label) {
        std::cout << "    " << std::left << std::setw(26) << label
                  << " 從字面量建構=" << ctorFromLiteral
                  << "  複製=" << copyCtor
                  << "  移動=" << moveCtor << std::endl;
    }
};

int Counted::ctorFromLiteral = 0;
int Counted::copyCtor = 0;
int Counted::moveCtor = 0;

// -----------------------------------------------------------------------------
// 【日常實務範例】監控系統的事件收集器
//   情境：服務執行期間持續收集事件，每筆事件由「時間戳 + 等級 + 訊息」組成。
//   為什麼兩種都用得上，而且各有道理：
//     * 事件是從三個零件現場組出來的 → emplace_back（省掉臨時 Event）
//     * 已經有一筆現成 Event 要轉存到另一個容器 → push_back（意圖清楚，
//       而且此時 emplace_back 完全沒有優勢）
//   這正是本檔的主題：用場景決定，而不是無腦選 emplace_back。
// -----------------------------------------------------------------------------
struct Event {
    long timestamp;
    std::string level;
    std::string message;

    Event(long ts, std::string lv, std::string msg)
        : timestamp(ts), level(std::move(lv)), message(std::move(msg)) {}
};

std::vector<Event> collectEvents() {
    std::vector<Event> events;
    events.reserve(4);

    // 零件現場組裝 → emplace_back 有優勢
    events.emplace_back(1721390102L, "INFO", "worker pool started");
    events.emplace_back(1721390144L, "WARN", "queue depth 812");
    events.emplace_back(1721390161L, "ERROR", "upstream timeout after 30s");
    return events;
}

std::vector<Event> filterErrors(const std::vector<Event>& all) {
    std::vector<Event> errors;
    for (const Event& e : all) {
        if (e.level == "ERROR") {
            // 已經是現成的 Event → push_back 意圖清楚，
            // 這裡改寫成 emplace_back(e) 效果完全相同，沒有任何好處
            errors.push_back(e);
        }
    }
    return errors;
}

int main() {
    std::cout << "=== 情境 A：傳現成的 lvalue（兩者完全等價）===" << std::endl;
    {
        std::vector<Counted> v;
        v.reserve(10);                    // 先 reserve，排除 reallocation 干擾
        Counted s("Hello");               // 這次建構不算進比較（先重設計數）

        Counted::reset();
        v.push_back(s);                   // 複製
        Counted::report("push_back(s)");

        Counted::reset();
        v.emplace_back(s);                // 也是複製！
        Counted::report("emplace_back(s)");
        std::cout << "    → 結論：完全相同，各 1 次複製" << std::endl;
    }

    std::cout << "\n=== 情境 B：傳 std::move 過的 rvalue（兩者完全等價）===" << std::endl;
    {
        std::vector<Counted> v;
        v.reserve(10);
        Counted a("World");
        Counted b("World");

        Counted::reset();
        v.push_back(std::move(a));        // 移動
        Counted::report("push_back(move(a))");

        Counted::reset();
        v.emplace_back(std::move(b));     // 也是移動
        Counted::report("emplace_back(move(b))");
        std::cout << "    → 結論：完全相同，各 1 次移動" << std::endl;
    }

    std::cout << "\n=== 情境 C：從字面量建立（emplace_back 才真的較優）===" << std::endl;
    {
        std::vector<Counted> v;
        v.reserve(10);

        Counted::reset();
        v.push_back("Foo");               // 造臨時 Counted → 移動進容器
        Counted::report("push_back(\"Foo\")");

        Counted::reset();
        v.emplace_back("Bar");            // 就地建構，無臨時物件
        Counted::report("emplace_back(\"Bar\")");
        std::cout << "    → 結論：push_back 多了 1 次移動（那個臨時物件）"
                  << std::endl;
    }

    std::cout << "\n=== 總結 ===" << std::endl;
    std::cout << "  手上是「成品」(現成物件) → 兩者等價，選 push_back（語意清楚）"
              << std::endl;
    std::cout << "  手上是「零件」(建構參數) → emplace_back 較優（省臨時物件）"
              << std::endl;

    std::cout << "\n=== 日常實務：監控事件收集 ===" << std::endl;
    std::vector<Event> all = collectEvents();      // 零件組裝 → emplace_back
    std::cout << "  收集到 " << all.size() << " 筆事件:" << std::endl;
    for (const Event& e : all) {
        std::cout << "    [" << e.level << "] " << e.message << std::endl;
    }

    std::vector<Event> errs = filterErrors(all);   // 轉存現成物件 → push_back
    std::cout << "  其中 ERROR 級別 " << errs.size() << " 筆:" << std::endl;
    for (const Event& e : errs) {
        std::cout << "    ts=" << e.timestamp << " " << e.message << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 13 課：vector 元素新增：push_back、emplace_back8.cpp" -o not_always_faster

// === 預期輸出 ===
// === 情境 A：傳現成的 lvalue（兩者完全等價）===
//     push_back(s)               從字面量建構=0  複製=1  移動=0
//     emplace_back(s)            從字面量建構=0  複製=1  移動=0
//     → 結論：完全相同，各 1 次複製
// 
// === 情境 B：傳 std::move 過的 rvalue（兩者完全等價）===
//     push_back(move(a))         從字面量建構=0  複製=0  移動=1
//     emplace_back(move(b))      從字面量建構=0  複製=0  移動=1
//     → 結論：完全相同，各 1 次移動
// 
// === 情境 C：從字面量建立（emplace_back 才真的較優）===
//     push_back("Foo")           從字面量建構=1  複製=0  移動=1
//     emplace_back("Bar")        從字面量建構=1  複製=0  移動=0
//     → 結論：push_back 多了 1 次移動（那個臨時物件）
// 
// === 總結 ===
//   手上是「成品」(現成物件) → 兩者等價，選 push_back（語意清楚）
//   手上是「零件」(建構參數) → emplace_back 較優（省臨時物件）
// 
// === 日常實務：監控事件收集 ===
//   收集到 3 筆事件:
//     [INFO] worker pool started
//     [WARN] queue depth 812
//     [ERROR] upstream timeout after 30s
//   其中 ERROR 級別 1 筆:
//     ts=1721390161 upstream timeout after 30s
