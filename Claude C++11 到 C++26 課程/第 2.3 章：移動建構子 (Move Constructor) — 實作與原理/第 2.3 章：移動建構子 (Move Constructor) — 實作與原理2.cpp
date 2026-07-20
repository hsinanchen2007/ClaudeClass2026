// =============================================================================
//  第 2.3 章 #2  —  為什麼移動建構子一定要標 noexcept（vector 擴容實測）
// =============================================================================
//
// 【主題資訊 Information】
//   關鍵字：noexcept                          （C++11）
//   關鍵函式：std::move_if_noexcept<T>(T& x)  （C++11，<utility>）
//   檢測 trait：std::is_nothrow_move_constructible<T>  （C++11，<type_traits>）
//   本檔重點：同一段 vector 擴容程式碼，只因為少寫一個 noexcept，
//             就從 O(1) 的移動**靜默退回** O(n) 的複製。
//
// 【詳細解釋 Explanation】
//
// 【1. vector 擴容時的兩難：搬到一半拋例外怎麼辦】
// std::vector 的 push_back／emplace_back 在容量不足時要做三件事：
//   ① 配置一塊更大的新緩衝區
//   ② 把舊緩衝區的 N 個元素「搬」到新緩衝區
//   ③ 解構舊元素、釋放舊緩衝區
// 標準要求 push_back 提供 **strong exception guarantee**（強例外保證）：
// 若過程中拋出例外，vector 必須維持在呼叫前的狀態，就像什麼都沒發生。
//
// 現在看第 ② 步。假設用「移動」搬，搬到第 5 個時拋了例外：
//   * 前 4 個元素**已經被掏空**了（它們的資源被偷到新緩衝區）
//   * 新緩衝區只有 4 個有效元素，第 5 個是半成品
//   * 想回滾？把新的搬回舊的 —— 但「搬回去」這個動作本身也可能再拋例外
//   → **無法回滾**，強例外保證破功。
//
// 如果用「複製」搬，搬到第 5 個時拋例外：
//   * 舊緩衝區**完好無損**（複製不會動到來源）
//   * 只要把新緩衝區已建好的 4 個解構掉、釋放新緩衝區，就完全回到原狀
//   → **可以乾淨回滾**，強例外保證成立。
//
// 結論：移動比較快，但**不可回滾**；複製比較慢，但**可以回滾**。
// 只有在「移動保證不會拋例外」時，才能安心選快的那條路。
//
// 【2. std::move_if_noexcept：標準庫怎麼做這個選擇】
// vector 內部不是直接寫 std::move(elem)，而是 std::move_if_noexcept(elem)。
// 它的判斷邏輯（概念上）是：
//
//     若 T 的移動建構子是 noexcept  → 回傳 T&&      → 走移動（快）
//     否則若 T 可以複製建構         → 回傳 const T& → 走複製（慢但安全）
//     否則（不可複製，只好賭）      → 回傳 T&&      → 走移動
//
// 最後一條很重要：如果型別**根本不能複製**（例如 move-only 的 unique_ptr 包裝），
// 那就沒有安全退路了，標準庫只能移動 —— 這時**強例外保證會降級成基本保證**。
// 所以「不可複製」不是免死金牌，該標 noexcept 還是要標。
//
// 本檔實測（見預期輸出）：兩個類別的內容完全一樣，只差一個 noexcept，
// 擴容時一個印 [移動]、一個印 [複製]。這不是編譯器的隨機決定，
// 而是 move_if_noexcept 依 is_nothrow_move_constructible 做出的必然選擇。
//
// 【3. 為什麼「靜默退回複製」特別危險】
// 少寫 noexcept **不會編譯錯誤，不會警告，不會執行期報錯**。
// 程式完全正確，只是慢了一個數量級 —— 而且是在 vector 擴容這種
// 「資料量越大越常發生」的路徑上慢。這種 bug 只能靠 profiler 或
// static_assert 抓，是實務上最常見的「隱形效能地雷」之一。
// 防禦寫法：
//     static_assert(std::is_nothrow_move_constructible<MyType>::value,
//                   "MyType 的移動建構子必須是 noexcept");
//
// 【概念補充 Concept Deep Dive】
//
// (A) 移動建構子憑什麼能保證 noexcept
//   因為它做的事情只有「搬幾個純量欄位 + 把來源歸零」，完全不配置記憶體、
//   不呼叫可能拋例外的操作。相對地，複製建構子必須 new，new 可能丟
//   std::bad_alloc，所以複製建構子**天生就不能**標 noexcept。
//   這是移動與複製在例外安全上的本質差異，不是風格選擇。
//   反面案例：如果你的移動建構子裡寫了 new（例如「移動時順便配一塊小 buffer」），
//   它就真的可能拋例外，這時**不可以**騙人標 noexcept —— 標了而實際拋出，
//   會直接呼叫 std::terminate，程式當場死掉，比慢更糟。
//
// (B) noexcept 不只影響 vector
//   凡是需要強例外保證又要搬移元素的地方都吃這一套：
//   vector 的 reserve／resize／insert、deque 的擴充、
//   std::swap 的 noexcept 推導、以及各種容器的 reallocation。
//
// (C) = default 產生的移動建構子會自動推導 noexcept
//   若你寫 `T(T&&) = default;`，編譯器會依「所有成員與基底的移動操作是否
//   全部 noexcept」自動決定這個 default 版本是不是 noexcept。
//   成員都是 int／指標／std::string 這類 nothrow-movable 的型別時，
//   自動推導出來的就是 noexcept —— 這是 Rule of Zero 的隱藏好處：
//   什麼都不寫，反而最容易拿到正確的 noexcept。
//
// (D) 成員初始化順序（與本章共通的地雷）
//   本檔的類別只有一個 int 成員所以看不出來，但只要類別有「長度 + 指標」
//   這種互相依賴的成員，就必須讓**宣告順序**與依賴順序一致 ——
//   成員一律照宣告順序初始化，與初始化列表寫法無關。詳見本章 #1 與 summary。
//
// 【注意事項 Pay Attention】
// 1. noexcept 是**承諾**不是**檢查**。標了 noexcept 卻真的拋出例外時，
//    不會傳播出去，而是直接呼叫 std::terminate 終止程式。
//    只在「確定不會拋」時才標。
// 2. 本檔 v1/v2 都先 reserve(2)，所以前兩次 emplace_back 不會有任何搬移輸出；
//    第三次才超過容量、觸發 reallocation。這是為了讓輸出乾淨地只呈現擴容行為。
// 3. 擴容後的新容量（本機 libstdc++ 是 2 倍成長，2 → 4）屬**實作定義**，
//    標準只保證攤還 O(1)，不同標準庫（MSVC 用 1.5 倍）數字不同。
// 4. 被移動後的來源處於「有效但未指定」狀態。本檔自己寫了 o.id = -1，
//    所以我們知道是 -1；但這是**這個類別的自訂行為**，不是通則。
// 5. 移動建構子的 noexcept 要標在**宣告**上；分離定義時兩邊都要一致。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】noexcept 與 move_if_noexcept
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼移動建構子一定要標 noexcept？
//     答：因為 std::vector 擴容時必須提供 strong exception guarantee。若用移動
//         搬到一半拋例外，前面的元素已被掏空、舊緩衝區已破壞，**無法回滾**；
//         用複製則舊緩衝區完好，可以乾淨回滾。所以 vector 用
//         std::move_if_noexcept：只有移動建構子標了 noexcept 才移動，
//         **否則靜默退回複製**。少一個 noexcept，O(1) 就變成 O(n)。
//     追問：這個退化會有編譯錯誤或警告嗎？→ 完全沒有。程式正確，只是慢一個
//         數量級，只能靠 profiler 或
//         static_assert(std::is_nothrow_move_constructible<T>::value) 抓。
//
// 🔥 Q2. std::move 和 std::move_if_noexcept 差在哪？
//     答：std::move 是**無條件**轉成 rvalue（static_cast<T&&>）。
//         std::move_if_noexcept 是**有條件**的：移動建構子是 noexcept 才回傳
//         T&&（走移動），否則回傳 const T&（走複製）。前者是使用者用的，
//         後者是標準庫在需要例外安全的地方用的。
//     追問：型別不可複製時 move_if_noexcept 怎麼辦？→ 沒有安全退路，仍回傳 T&&
//         走移動；此時 vector 的強例外保證會降級成基本保證。
//
// 🔥 Q3. 複製建構子可以標 noexcept 嗎？
//     答：通常不行。複製建構子要 new 一塊新記憶體，new 可能丟 std::bad_alloc，
//         所以天生就會拋例外。這正是移動與複製在例外安全上的本質差異：
//         移動只搬指標、不配置記憶體，才有資格保證 nothrow。
//     追問：那什麼複製建構子可以 noexcept？→ 成員全是純量或固定大小陣列、
//         完全不做動態配置的類別（例如只有幾個 int 的 POD-like 型別）。
//
// ⚠️ 陷阱. 我的類別寫了 `T(T&&)`（沒標 noexcept），但 is_move_constructible<T>
//         回傳 true，這樣 vector 擴容應該就會用移動了吧？
//     答：不會。is_move_constructible 只回答「能不能用 rvalue 建構」，
//         而 `const T&` 也吃得下 rvalue，所以有複製建構子時它**恆為 true**，
//         看不出有沒有真正的移動建構子。vector 看的是
//         **is_nothrow_move_constructible**（本檔中 WithoutNoexcept 為 false）。
//     為什麼會錯：把 is_move_constructible 當成「有沒有移動建構子」的偵測器。
//         它其實是「可否從 rvalue 建構」，複製建構子也滿足這個條件。
//         要驗證有沒有「真正會被 vector 採用的移動」，一律看 nothrow 版本。
//         （本章 #4 檔用 trait 把這個差異印出來對照。）
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <utility>
#include <type_traits>

class WithNoexcept {
public:
    int id;
    WithNoexcept(int i) : id(i) {}

    WithNoexcept(const WithNoexcept& o) : id(o.id) {
        std::cout << "  WithNoexcept [複製] id=" << id << "\n";
    }

    WithNoexcept(WithNoexcept&& o) noexcept : id(o.id) {
        o.id = -1;
        std::cout << "  WithNoexcept [移動] id=" << id << "\n";
    }
};

class WithoutNoexcept {
public:
    int id;
    WithoutNoexcept(int i) : id(i) {}

    WithoutNoexcept(const WithoutNoexcept& o) : id(o.id) {
        std::cout << "  WithoutNoexcept [複製] id=" << id << "\n";
    }

    // 注意：沒有 noexcept！
    WithoutNoexcept(WithoutNoexcept&& o) : id(o.id) {
        o.id = -1;
        std::cout << "  WithoutNoexcept [移動] id=" << id << "\n";
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】連線池物件：只能移動、不能複製，且必須保證 nothrow move
//   情境：連線池裡放的是「已建立的 TCP 連線」。連線是獨佔資源（一條 socket
//         不能有兩個擁有者），所以複製要 = delete，只保留移動。
//   為什麼用到本主題：池子通常用 std::vector<Connection> 存放，擴容時要搬移
//         元素。Connection 不可複製 → move_if_noexcept 沒有複製退路，
//         **只能移動**；此時若移動建構子會拋例外，強例外保證就沒了。
//         所以 move-only 型別**更需要**確保 noexcept，並用 static_assert 釘住。
//   注意：這裡用 fd 整數模擬 socket；真實程式碼會在解構子呼叫 ::close(fd)。
// -----------------------------------------------------------------------------
class Connection {
    int fd_;                 // socket file descriptor，-1 代表「不持有連線」
    std::string endpoint_;

public:
    Connection(int fd, std::string endpoint)
        : fd_(fd), endpoint_(std::move(endpoint)) {}

    ~Connection() {
        if (fd_ >= 0) {
            // ::close(fd_);   // 真實場景會關閉 socket
            std::cout << "    [關閉連線] fd=" << fd_ << " → " << endpoint_ << "\n";
        }
    }

    // 連線是獨佔資源：禁止複製
    Connection(const Connection&)            = delete;
    Connection& operator=(const Connection&) = delete;

    // 只保留移動；string 的移動是 noexcept，int 也是，故整體可保證 noexcept
    Connection(Connection&& o) noexcept
        : fd_(o.fd_), endpoint_(std::move(o.endpoint_)) {
        o.fd_ = -1;                     // 來源放棄 fd，解構時才不會誤關
    }

    Connection& operator=(Connection&& o) noexcept {
        if (this != &o) {
            if (fd_ >= 0) { /* ::close(fd_); */ }
            fd_ = o.fd_;
            endpoint_ = std::move(o.endpoint_);
            o.fd_ = -1;
        }
        return *this;
    }

    int fd() const { return fd_; }
    const std::string& endpoint() const { return endpoint_; }
};

// move-only 型別沒有複製退路，更要把 noexcept 釘死
static_assert(std::is_nothrow_move_constructible<Connection>::value,
              "Connection 的移動建構子必須是 noexcept，否則 vector 擴容"
              "無法提供強例外保證");
static_assert(!std::is_copy_constructible<Connection>::value,
              "Connection 必須是 move-only");

int main() {
    std::cout << std::boolalpha;

    std::cout << "=== 有 noexcept 的類別 ===\n";
    std::vector<WithNoexcept> v1;
    v1.reserve(2);  // 先保留 2 個空間
    v1.emplace_back(1);
    v1.emplace_back(2);
    std::cout << "-- 觸發擴容 --\n";
    v1.emplace_back(3);  // 超過容量，需要擴容搬移舊元素

    std::cout << "\n=== 沒有 noexcept 的類別 ===\n";
    std::vector<WithoutNoexcept> v2;
    v2.reserve(2);
    v2.emplace_back(1);
    v2.emplace_back(2);
    std::cout << "-- 觸發擴容 --\n";
    v2.emplace_back(3);  // 擴容時會退回用複製！

    // ── 用 trait 印出「vector 依據什麼做決定」──────────────────
    std::cout << "\n=== vector 的判斷依據（is_nothrow_move_constructible）===\n";
    std::cout << "  WithNoexcept    : is_move_constructible        = "
              << std::is_move_constructible<WithNoexcept>::value << "\n";
    std::cout << "  WithNoexcept    : is_nothrow_move_constructible= "
              << std::is_nothrow_move_constructible<WithNoexcept>::value
              << "   ← 擴容走【移動】\n";
    std::cout << "  WithoutNoexcept : is_move_constructible        = "
              << std::is_move_constructible<WithoutNoexcept>::value
              << "   ← 這個 true 具有誤導性\n";
    std::cout << "  WithoutNoexcept : is_nothrow_move_constructible= "
              << std::is_nothrow_move_constructible<WithoutNoexcept>::value
              << "  ← 擴容退回【複製】\n";

    // ── move_if_noexcept 實際回傳什麼型別 ──────────────────────
    std::cout << "\n=== std::move_if_noexcept 的選擇 ===\n";
    WithNoexcept    yes(7);
    WithoutNoexcept no(8);
    std::cout << "  move_if_noexcept(WithNoexcept)    回傳 rvalue ref? "
              << std::is_rvalue_reference<
                     decltype(std::move_if_noexcept(yes))>::value
              << "  → 走移動\n";
    std::cout << "  move_if_noexcept(WithoutNoexcept) 回傳 rvalue ref? "
              << std::is_rvalue_reference<
                     decltype(std::move_if_noexcept(no))>::value
              << " → 退回複製（回傳的是 const&）\n";

    // ── 實務：連線池 ─────────────────────────────────────────
    std::cout << "\n=== 日常實務：連線池（move-only + noexcept）===\n";
    std::vector<Connection> pool;
    pool.reserve(2);
    pool.emplace_back(101, "db-primary:5432");
    pool.emplace_back(102, "db-replica:5432");
    std::cout << "  池中連線數=" << pool.size()
              << "，容量=" << pool.capacity() << "\n";

    std::cout << "  -- 加入第 3 條，觸發擴容（無複製退路，只能移動）--\n";
    pool.emplace_back(103, "cache:6379");
    std::cout << "  擴容後仍持有 " << pool.size() << " 條連線：\n";
    for (const auto& c : pool) {
        std::cout << "    fd=" << c.fd() << " → " << c.endpoint() << "\n";
    }

    std::cout << "  -- 把一條連線交給別處（明確移動）--\n";
    Connection borrowed = std::move(pool.back());
    std::cout << "    borrowed fd=" << borrowed.fd() << "\n";
    std::cout << "    來源已放棄 fd（-1 代表不再持有）: "
              << pool.back().fd() << "\n";

    std::cout << "\n=== 離開 main（解構，只有仍持有 fd 的會關閉）===\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.3 章：移動建構子 (Move Constructor) — 實作與原理2.cpp" -o move_ctor2

// === 預期輸出 ===
// === 有 noexcept 的類別 ===
// -- 觸發擴容 --
//   WithNoexcept [移動] id=1
//   WithNoexcept [移動] id=2
//
// === 沒有 noexcept 的類別 ===
// -- 觸發擴容 --
//   WithoutNoexcept [複製] id=1
//   WithoutNoexcept [複製] id=2
//
// === vector 的判斷依據（is_nothrow_move_constructible）===
//   WithNoexcept    : is_move_constructible        = true
//   WithNoexcept    : is_nothrow_move_constructible= true   ← 擴容走【移動】
//   WithoutNoexcept : is_move_constructible        = true   ← 這個 true 具有誤導性
//   WithoutNoexcept : is_nothrow_move_constructible= false  ← 擴容退回【複製】
//
// === std::move_if_noexcept 的選擇 ===
//   move_if_noexcept(WithNoexcept)    回傳 rvalue ref? true  → 走移動
//   move_if_noexcept(WithoutNoexcept) 回傳 rvalue ref? false → 退回複製（回傳的是 const&）
//
// === 日常實務：連線池（move-only + noexcept）===
//   池中連線數=2，容量=2
//   -- 加入第 3 條，觸發擴容（無複製退路，只能移動）--
//   擴容後仍持有 3 條連線：
//     fd=101 → db-primary:5432
//     fd=102 → db-replica:5432
//     fd=103 → cache:6379
//   -- 把一條連線交給別處（明確移動）--
//     borrowed fd=103
//     來源已放棄 fd（-1 代表不再持有）: -1
//
// === 離開 main（解構，只有仍持有 fd 的會關閉）===
//     [關閉連線] fd=103 → cache:6379
//     [關閉連線] fd=101 → db-primary:5432
//     [關閉連線] fd=102 → db-replica:5432
