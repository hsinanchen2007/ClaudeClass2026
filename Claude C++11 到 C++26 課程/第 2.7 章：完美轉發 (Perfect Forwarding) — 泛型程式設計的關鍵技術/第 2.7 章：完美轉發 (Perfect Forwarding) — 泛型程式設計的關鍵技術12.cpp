// =============================================================================
//  第 2.7 章 範例 12  —  事件匯流排：完美轉發在真實框架中的樣子
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<utility>（forward）、<typeindex>（type_index）、<functional>（function）、
//           <unordered_map>、<vector>
//   核心 API：
//       template<class EventType, class Func> void on(Func&& handler);
//       template<class EventType, class... Args> void emit(Args&&... args);
//   emit 把引數完美轉發到 EventType 的建構子，事件物件在 emit 內部就地建構，
//   呼叫端完全不需要自己 new 或組裝事件物件。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼事件系統是完美轉發的完美舞台】
//   事件系統的介面必須同時滿足三件事：
//     * 型別安全：ClickEvent 的處理器不能收到 MessageEvent
//     * 任意參數：不同事件的欄位數量與型別都不同
//     * 零額外複製：事件可能每秒發射上萬次
//   emit<ClickEvent>(100, 200, std::string("left")) 一行同時做到三件事：
//   EventType 決定型別、Args&&... 吃下任意參數、forward 保證右值被移動。
//   若沒有完美轉發，就得為每種事件寫一支專用的 emit，或退回
//   「先建好事件再傳進來」——後者對右值多一次搬移，對左值多一次複製。
//
// 【2. 型別擦除：handlers_ 為什麼是 map<type_index, vector<function<void(const Event&)>>>】
//   不同事件型別的處理器簽名都不同（void(const ClickEvent&) vs
//   void(const MessageEvent&)），無法存進同一個容器。
//   解法是把它們統一包成 std::function<void(const Event&)>：
//       [h = std::forward<Func>(handler)](const Event& e) {
//           h(static_cast<const EventType&>(e));
//       }
//   外層統一簽名，內層在 lambda 裡把基底參考轉回具體型別。
//   這個轉型之所以安全，是因為 emit 只會把處理器交給「用同一個 type_index
//   註冊」的事件——型別的正確性由 typeid(EventType) 這把鑰匙保證。
//
// 【3. lambda 的初始化捕捉 [h = std::forward<Func>(handler)] 是關鍵】
//   這是 C++14 的 init-capture。它讓處理器被「移動」進 lambda，
//   而不是複製。若寫成 [handler]（複製捕捉），有捕捉狀態的處理器
//   （例如捕捉了大型 buffer 的 lambda）每註冊一次就複製一次。
//   註：C++11 沒有 init-capture，只能複製捕捉，這是 C++14 的實質改進。
//
// 【4. 為什麼 emit 內部是「先建構事件物件、再逐一呼叫處理器」】
//   EventType event(std::forward<Args>(args)...);
//   引數只能轉發一次，而處理器可能有很多個。
//   所以正確做法是：轉發一次、建構出一個事件物件，然後把「這個物件的
//   const 參考」傳給每一個處理器。若試圖對每個處理器都轉發原始引數，
//   第二個處理器就會拿到被搬空的資料——正是本章範例 5 講的重試陷阱。
//
// 【概念補充 Concept Deep Dive】
//   (A) std::type_index 是 std::type_info 的可複製、可比較、可雜湊包裝，
//       專門為了「當 map 的鍵」而存在（type_info 本身不可複製）。
//   (B) 本檔的 handler 用 static_cast 而非 dynamic_cast，是因為型別
//       正確性已由 type_index 保證，static_cast 沒有執行期成本。
//       若要防禦性程式設計，可改用 dynamic_cast 並檢查 nullptr。
//   (C) 事件基類有 virtual ~Event() = default，這是必要的：
//       雖然本檔的事件都在堆疊上，但一旦有人透過 Event* 刪除衍生物件，
//       沒有虛擬解構子就是未定義行為。
//
// 【注意事項 Pay Attention】
//   1. emit 內只能轉發一次；多個處理器共用「已建構好的事件物件」。
//   2. 註冊處理器時用 init-capture 移動，避免大型閉包被複製。
//   3. static_cast 到具體事件型別的安全性，完全依賴 type_index 的正確配對；
//      若手動把處理器塞進錯誤的桶子，就是未定義行為。
//   4. 本檔的 EventBus 不是執行緒安全的；多執行緒環境需要自行加鎖，
//      或改用無鎖的訂閱者快照。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】事件系統中的完美轉發
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. emit<ClickEvent>(100, 200, std::string("left")) 中，
//        字串是被複製還是被移動？為什麼？
//     答：被移動。std::string("left") 是臨時物件（純右值），
//         Args 推導為 std::string，forward<std::string> 折疊成右值參考，
//         於是 ClickEvent 的轉發建構子把它 move 進 button 成員，零複製。
//     追問：那 bus.emit<ClickEvent>(50, 75, button)（button 是具名變數）呢？
//         → Args 推導為 std::string&，折疊回左值，走複製；button 保持完好。
//
// 🔥 Q2. 為什麼 emit 是「先建構事件物件、再餵給每個處理器」，
//        而不是對每個處理器分別轉發引數？
//     答：因為引數只能轉發一次。若對每個處理器都轉發，第一個處理器
//         可能已把資料搬走，第二個就拿到空殼——與重試包裝器是同一個陷阱。
//         建構一次事件物件、再以 const Event& 分送，才是正確做法。
//
// 🔥 Q3. handlers_ 為什麼要用 std::type_index 當鍵？直接用字串不行嗎？
//     答：type_index 由編譯器保證唯一且不會拼錯，比較是 O(1) 的位址或
//         字串比對，而且不需要維護任何命名規則。用字串則要自己保證
//         「發射端與註冊端拼法一致」，重構改名時無法靠編譯器檢查。
//
// ⚠️ 陷阱. 處理器裡的 static_cast<const EventType&>(e) 看起來很危險，
//          為什麼它其實是安全的？反過來說，什麼情況下會不安全？
//     答：安全是因為這個 lambda 只會被存進 handlers_[typeid(EventType)] 這個桶子，
//         而 emit<T> 也只會去 handlers_[typeid(T)] 取處理器——
//         型別配對由 type_index 這把鑰匙保證，轉型必定正確。
//         不安全的情況：若有人繞過 on<T>() 直接把 lambda 塞進別的桶子，
//         或用同一個 type_index 註冊不同型別的處理器，就會以錯誤型別
//         解讀物件，屬於未定義行為。
//     為什麼會錯：大家看到 static_cast 向下轉型就直覺認為該用 dynamic_cast。
//         但這裡的型別不變性是由「資料結構的設計」維持的，不是靠執行期檢查；
//         這是一種常見的取捨——用封裝換取零成本。前提是封裝不能被繞過。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <utility>
#include <memory>
#include <typeindex>

// ===== 事件基類 =====
struct Event {
    virtual ~Event() = default;
    virtual std::string name() const = 0;
};

// ===== 具體事件型別 =====
struct ClickEvent : Event {
    int x, y;
    std::string button;

    // 完美轉發建構子
    template<typename S>
    ClickEvent(int x, int y, S&& btn)
        : x(x), y(y), button(std::forward<S>(btn)) {}

    std::string name() const override { return "ClickEvent"; }
};

struct MessageEvent : Event {
    std::string sender;
    std::string content;

    template<typename S1, typename S2>
    MessageEvent(S1&& s, S2&& c)
        : sender(std::forward<S1>(s))
        , content(std::forward<S2>(c)) {}

    std::string name() const override { return "MessageEvent"; }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】訂單付款事件
//   情境：電商後端在訂單付款成功後，需要同時觸發「寄確認信」「寫稽核紀錄」
//     「更新營收統計」三件互不相干的事。事件驅動架構讓這三者各自獨立訂閱，
//     新增一種後續處理時不必修改付款流程本身。
//   金額用整數分（cents）而非浮點數，這是金流系統的標準做法——
//   浮點數無法精確表示 0.1 這類十進位小數，會累積誤差。
// -----------------------------------------------------------------------------
struct OrderPaidEvent : Event {
    std::string orderId;
    std::string customer;
    long long   amountCents;

    template<typename S1, typename S2>
    OrderPaidEvent(S1&& id, S2&& cust, long long cents)
        : orderId(std::forward<S1>(id))
        , customer(std::forward<S2>(cust))
        , amountCents(cents) {}

    std::string name() const override { return "OrderPaidEvent"; }
};

// ===== 事件匯流排 =====
class EventBus {
    using Handler = std::function<void(const Event&)>;
    std::unordered_map<std::type_index, std::vector<Handler>> handlers_;

public:
    // 註冊處理器
    template<typename EventType, typename Func>
    void on(Func&& handler) {
        handlers_[typeid(EventType)].push_back(
            [h = std::forward<Func>(handler)](const Event& e) {
                h(static_cast<const EventType&>(e));
            }
        );
    }

    // 發射事件：完美轉發引數，直接在內部建構事件
    template<typename EventType, typename... Args>
    void emit(Args&&... args) {
        // 完美轉發到事件的建構子
        EventType event(std::forward<Args>(args)...);

        std::cout << "發射 " << event.name() << "\n";

        auto it = handlers_.find(typeid(EventType));
        if (it != handlers_.end()) {
            for (auto& handler : it->second) {
                handler(event);
            }
        }
    }
};

int main() {
    EventBus bus;

    // 註冊處理器
    bus.on<ClickEvent>([](const ClickEvent& e) {
        std::cout << "  處理點擊: (" << e.x << "," << e.y
                  << ") button=" << e.button << "\n";
    });

    bus.on<MessageEvent>([](const MessageEvent& e) {
        std::cout << "  處理訊息: [" << e.sender << "] " << e.content << "\n";
    });

    // 發射事件——引數直接完美轉發到建構子
    std::cout << "--- 右值引數 ---\n";
    bus.emit<ClickEvent>(100, 200, std::string("left"));
    bus.emit<MessageEvent>(std::string("Alice"), std::string("Hello!"));

    std::cout << "\n--- 左值引數 ---\n";
    std::string button = "right";
    std::string sender = "Bob";
    std::string content = "World!";
    bus.emit<ClickEvent>(50, 75, button);       // 複製 button
    bus.emit<MessageEvent>(sender, content);     // 複製 sender 和 content

    std::cout << "\n--- 混合引數 ---\n";
    bus.emit<MessageEvent>(sender, std::string("Goodbye!"));  // 複製 sender，移動 content

    // 驗證左值沒被破壞
    std::cout << "\n--- 驗證 ---\n";
    std::cout << "button  = \"" << button  << "\"\n";
    std::cout << "sender  = \"" << sender  << "\"\n";
    std::cout << "content = \"" << content << "\"\n";

    // ============================================================
    // 多個處理器共用同一個事件物件（引數只轉發一次）
    // ============================================================
    std::cout << "\n--- 同一事件註冊多個處理器 ---\n";
    bus.on<MessageEvent>([](const MessageEvent& e) {
        std::cout << "  第二個處理器也收到完整內容: [" << e.sender << "] "
                  << e.content << "\n";
    });
    bus.emit<MessageEvent>(std::string("Carol"), std::string("Multi-handler"));
    std::cout << "  ↑ 兩個處理器都拿到完整資料：因為 emit 只轉發一次、\n";
    std::cout << "     建構出一個事件物件，再以 const& 分送給每個處理器\n";

    // ============================================================
    // 日常實務：訂單狀態變更的事件驅動處理
    // ============================================================
    std::cout << "\n=== 日常實務：訂單事件驅動 ===\n";
    {
        EventBus orderBus;

        // 同一個事件，三個獨立的訂閱者（發信、寫稽核 log、更新統計）
        orderBus.on<OrderPaidEvent>([](const OrderPaidEvent& e) {
            std::cout << "  [通知] 寄出付款確認信給 " << e.customer
                      << "（訂單 " << e.orderId << "）\n";
        });
        orderBus.on<OrderPaidEvent>([](const OrderPaidEvent& e) {
            std::cout << "  [稽核] 記錄付款: order=" << e.orderId
                      << " amount=" << e.amountCents << " cents\n";
        });
        orderBus.on<OrderPaidEvent>([](const OrderPaidEvent& e) {
            std::cout << "  [統計] 營收累加 " << e.amountCents << " cents\n";
        });

        std::string customer = "alice@example.com";
        std::cout << "  發射事件（orderId 為右值 → 移動，customer 為左值 → 複製）:\n";
        orderBus.emit<OrderPaidEvent>(std::string("ORD-20260719-0042"),
                                      customer, 129900);

        std::cout << "  呼叫端的 customer 仍完好: " << customer << "\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.7 章：完美轉發 (Perfect Forwarding) — 泛型程式設計的關鍵技術12.cpp" -o event_bus

// 註：本檔未附 LeetCode 範例。事件匯流排是應用架構層的設計，
//     依賴型別擦除與 RTTI（type_index），LeetCode 題目沒有對應情境；
//     硬套一題只會失真。
//
// 註：本檔所有輸出皆為確定性內容。處理器的呼叫順序等同註冊順序，
//     因為它們存放在 std::vector 中依序走訪。

// === 預期輸出 ===
// --- 右值引數 ---
// 發射 ClickEvent
//   處理點擊: (100,200) button=left
// 發射 MessageEvent
//   處理訊息: [Alice] Hello!
//
// --- 左值引數 ---
// 發射 ClickEvent
//   處理點擊: (50,75) button=right
// 發射 MessageEvent
//   處理訊息: [Bob] World!
//
// --- 混合引數 ---
// 發射 MessageEvent
//   處理訊息: [Bob] Goodbye!
//
// --- 驗證 ---
// button  = "right"
// sender  = "Bob"
// content = "World!"
//
// --- 同一事件註冊多個處理器 ---
// 發射 MessageEvent
//   處理訊息: [Carol] Multi-handler
//   第二個處理器也收到完整內容: [Carol] Multi-handler
//   ↑ 兩個處理器都拿到完整資料：因為 emit 只轉發一次、
//      建構出一個事件物件，再以 const& 分送給每個處理器
//
// === 日常實務：訂單事件驅動 ===
//   發射事件（orderId 為右值 → 移動，customer 為左值 → 複製）:
// 發射 OrderPaidEvent
//   [通知] 寄出付款確認信給 alice@example.com（訂單 ORD-20260719-0042）
//   [稽核] 記錄付款: order=ORD-20260719-0042 amount=129900 cents
//   [統計] 營收累加 129900 cents
//   呼叫端的 customer 仍完好: alice@example.com
