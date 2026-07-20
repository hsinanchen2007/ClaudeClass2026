// =============================================================================
//  第 2.5 章 4  —  std::move 與所有權轉移：unique_ptr 為何非它不可
// =============================================================================
//
// 【主題資訊 Information】
//   std::unique_ptr<T>            獨佔所有權的智慧指標  [C++11, <memory>]
//   std::make_unique<T>(args...)                        [C++14, <memory>]
//   std::move(x)                                        [C++11, <utility>]
//   標頭檔  : <memory>, <utility>
//   複雜度  : 移動為 O(1)（只搬一個指標）
//   實測    : sizeof(std::unique_ptr<int>) = 8 bytes（本機，與裸指標同大小）
//
// 【詳細解釋 Explanation】
//
// 【1. unique_ptr 用型別系統表達「獨佔」】
//   unique_ptr 刻意把複製建構子與複製賦值 delete 掉，只留下移動版本。
//   這不是限制，而是把「同一份資源不能有兩個擁有者」這個規則
//   從「靠開發者自律」升級成「編譯器強制檢查」。
//   於是 take_ownership(p); 直接編譯失敗——錯誤在編譯期就被擋下，
//   而不是等到執行期變成 double free。
//
// 【2. 為什麼一定要寫 std::move，不能讓編譯器自動幫忙】
//   因為所有權轉移是「有後果」的操作：轉移之後呼叫端就不再擁有它了。
//   C++ 的設計哲學是「代價昂貴或有副作用的事必須寫出來」。
//   強制寫 std::move 讓程式碼自我說明：讀到這一行就知道
//   「p 的所有權在這裡交出去了，之後不能再用」。
//   如果編譯器默默轉移，這種所有權變化就會變成隱形的。
//
// 【3. 移動之後 p 是什麼？——這裡有明確保證】
//   一般型別 moved-from 是「有效但未指定」，但 unique_ptr 是例外：
//   標準明文規定它的移動建構子與移動賦值會讓來源變成 nullptr
//   （[unique.ptr.single.ctor]：post-condition 為 u.get() == nullptr）。
//   所以 p == nullptr 是「有保證」的，可以安全地檢查，也可以寫進測試。
//   這與 std::string 的 moved-from 狀態完全不同，別混為一談。
//
// 【4. 為什麼 create() 的 return 不需要寫 std::move】
//   return std::make_unique<int>(42); 回傳的本來就是純右值（prvalue），
//   直接就地建構在回傳值裡（C++17 起這屬於保證的複製省略）。
//   即使是回傳具名區域變數 return local;，編譯器也會先嘗試 NRVO，
//   失敗才退而使用移動——寫成 return std::move(local); 反而會關掉 NRVO，
//   多做一次移動。所以：回傳區域變數時千萬不要加 std::move。
//
// 【概念補充 Concept Deep Dive】
//   take_ownership 的參數宣告成「傳值」的 std::unique_ptr<int>，
//   這是刻意的：這種寫法叫 sink parameter（匯入參數），
//   在型別層面就宣告了「我會拿走這個東西」。呼叫端因此被迫寫 std::move，
//   一看就知道所有權交出去了。
//   對照組是 const unique_ptr<T>&（只借閱、不拿走）——但更好的做法是
//   直接收 T* 或 T&：函式若不管理生命週期，就不該要求對方一定得用
//   unique_ptr 持有。這是「介面應該只表達自己真正需要的東西」的原則。
//
// 【注意事項 Pay Attention】
// 1. 被移動之後的 unique_ptr 保證是 nullptr，解參考它就是解參考空指標（UB）。
//    這裡的重點是：「p 變成 nullptr」有保證，但「解參考 nullptr」仍然是 UB。
// 2. unique_ptr 不可複製，這是型別設計，不是限制；需要共享請改用
//    std::shared_ptr（但要先確認真的需要共享，shared_ptr 有引用計數成本）。
// 3. 回傳區域變數時不要寫 return std::move(local);——會抑制 NRVO。
// 4. 優先用 std::make_unique 而非 new：它是例外安全的，也不用寫兩次型別。
// 5. sizeof(unique_ptr<T>) 在使用預設 deleter 時等於裸指標（本機實測 8 bytes）；
//    帶有狀態的自訂 deleter 會讓它變大。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】所有權轉移與 unique_ptr
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼把 unique_ptr 傳給函式一定要寫 std::move？
//     答：unique_ptr 的複製建構子被 delete，只有移動版本。傳值參數需要
//         建構一個新的 unique_ptr，只能走移動，所以必須明確把左值轉成右值。
//         更深層的理由是可讀性：所有權轉移有後果，C++ 要求它必須寫出來，
//         讓讀程式的人一眼看見「這裡交出去了」。
//     追問：那 sizeof(unique_ptr<int>) 是多少？→ 用預設 deleter 時等於
//         裸指標（本機實測 8 bytes），是零額外開銷的抽象；
//         帶狀態的自訂 deleter 才會增加大小。
//
// 🔥 Q2. unique_ptr 被移動之後是什麼狀態？和 std::string 一樣嗎？
//     答：不一樣。一般型別 moved-from 是「有效但未指定」，
//         但標準對 unique_ptr 有明確規定：移動後來源的 get() == nullptr。
//         所以檢查 p == nullptr 是有保證的行為，可以寫進測試。
//     追問：那可以解參考它嗎？→ 不行。它保證是 nullptr，
//         而解參考 nullptr 依然是 UB。
//
// ⚠️ 陷阱. std::unique_ptr<int> f() { auto p = make_unique<int>(1); return std::move(p); }
//          「加了 std::move 應該更快」——錯在哪？
//     答：正好相反，反而更慢。直接 return p; 時編譯器會先嘗試 NRVO，
//         把 p 直接建構在回傳值的位置，完全不搬移。寫成 return std::move(p);
//         之後回傳運算式變成右值參考而不是具名物件，NRVO 失去適用資格，
//         只能實際做一次移動建構。
//     為什麼會錯：把 std::move 理解成「加速用的最佳化提示」。
//         它其實只是型別轉換，而且在這個位置會擋掉一個更強的最佳化。
//         正確的直覺是：std::move 是用來「表達意圖」，不是用來「加速」。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】從缺。
//   理由：本檔主題是資源所有權的移轉語意。LeetCode 的樹／鏈結串列題雖然
//   會用到指標，但一律使用裸指標（TreeNode*、ListNode*）且不要求釋放，
//   完全不涉及所有權模型。清單中的設計題（146、707 等）核心也在資料結構
//   本身而非智慧指標。硬套一題會傳達「LeetCode 該用 unique_ptr」的錯誤印象，
//   故從缺，改以實務範例呈現所有權轉移真正的使用場景。

#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <utility>

std::unique_ptr<int> create() {
    // 回傳純右值，不需要（也不應該）寫 std::move
    return std::make_unique<int>(42);
}

void take_ownership(std::unique_ptr<int> ptr) {
    // sink parameter：型別本身就宣告了「我會拿走它」
    std::cout << "take_ownership 收到值: " << *ptr << "\n";
}   // ptr 在此解構，記憶體自動釋放

// -----------------------------------------------------------------------------
// 【日常實務範例】連線資源的所有權交接（工廠 → 佇列 → 工作者）
//   場景：伺服器接受連線後建立一個 Connection（持有 socket、緩衝區等資源），
//         先放進待處理佇列，之後由工作執行緒取出處理完畢並關閉。
//   為什麼用 unique_ptr + std::move：整條鏈上「同一時間只能有一個擁有者」。
//     用裸指標的話，誰該 delete、什麼時候 delete 全靠註解和自律，
//     一旦出錯就是 double free 或洩漏。改用 unique_ptr 之後，
//     所有權轉移必須寫出 std::move，編譯器會擋掉任何意外的複製，
//     而且不管正常結束或中途丟例外，資源都保證被釋放（RAII）。
// -----------------------------------------------------------------------------
class Connection {
private:
    std::string peer_;
    bool open_ = true;
public:
    explicit Connection(std::string peer) : peer_(std::move(peer)) {
        std::cout << "  [open ] 建立連線 " << peer_ << "\n";
    }
    ~Connection() {
        if (open_) std::cout << "  [close] 釋放連線 " << peer_ << "\n";
    }
    // 明確禁止複製：一條連線不可能有兩個擁有者
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    const std::string& peer() const { return peer_; }
};

// 工廠：回傳所有權（純右值，不需要 std::move）
static std::unique_ptr<Connection> accept(const std::string& peer) {
    return std::make_unique<Connection>(peer);
}

// 工作者：sink parameter，收下所有權並在結束時釋放
static void handle(std::unique_ptr<Connection> conn) {
    std::cout << "  [work ] 處理來自 " << conn->peer() << " 的請求\n";
}   // conn 解構 → 連線自動關閉

int main() {
    std::cout << "=== 基本所有權轉移 ===\n";
    auto p = create();
    std::cout << "轉移前 p 是否持有資源: " << std::boolalpha
              << static_cast<bool>(p) << "\n";

    // take_ownership(p);            // 錯誤！unique_ptr 不可複製
    take_ownership(std::move(p));    // OK：明確轉移所有權

    // 標準保證：被移動後的 unique_ptr 一定是 nullptr。
    // 這裡印布林而不是指標值——指標值每次執行都不同，也沒有教學意義。
    std::cout << "轉移後 p == nullptr ? " << (p == nullptr)
              << "（標準明文保證，非實作巧合）\n";
    std::cout << std::noboolalpha;
    std::cout << "sizeof(std::unique_ptr<int>) = " << sizeof(std::unique_ptr<int>)
              << " bytes，sizeof(int*) = " << sizeof(int*)
              << " bytes（預設 deleter 下零額外開銷）\n";

    std::cout << "\n=== 日常實務: 連線所有權交接 ===\n";
    std::vector<std::unique_ptr<Connection>> pending;

    // 工廠產生 → 移入佇列
    pending.push_back(accept("10.0.0.11:52344"));
    pending.push_back(accept("10.0.0.12:41880"));

    std::cout << "佇列中待處理連線: " << pending.size() << "\n";

    // 佇列 → 工作者：所有權再次移交，之後佇列裡的元素變成 nullptr
    for (auto& slot : pending) {
        handle(std::move(slot));
    }

    int empty_slots = 0;
    for (const auto& slot : pending) {
        if (slot == nullptr) ++empty_slots;
    }
    std::cout << "移交後佇列中已為 nullptr 的槽位: " << empty_slots
              << " / " << pending.size() << "\n";
    std::cout << "全程沒有手動 delete，資源仍在正確時機釋放。\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 2.5 章：stdmove — 強制轉換為右值4.cpp -o move_ownership

// 【但書】輸出中一律印「布林」而非指標值：指標的實際位址受 ASLR 影響、
//   每次執行都不同，且對教學沒有意義。「移動後 p == nullptr」是標準對
//   unique_ptr 的明文保證（不同於 std::string 的「有效但未指定」），
//   因此這個 true 是可依賴的結果。

// === 預期輸出 ===
// === 基本所有權轉移 ===
// 轉移前 p 是否持有資源: true
// take_ownership 收到值: 42
// 轉移後 p == nullptr ? true（標準明文保證，非實作巧合）
// sizeof(std::unique_ptr<int>) = 8 bytes，sizeof(int*) = 8 bytes（預設 deleter 下零額外開銷）
//
// === 日常實務: 連線所有權交接 ===
//   [open ] 建立連線 10.0.0.11:52344
//   [open ] 建立連線 10.0.0.12:41880
// 佇列中待處理連線: 2
//   [work ] 處理來自 10.0.0.11:52344 的請求
//   [close] 釋放連線 10.0.0.11:52344
//   [work ] 處理來自 10.0.0.12:41880 的請求
//   [close] 釋放連線 10.0.0.12:41880
// 移交後佇列中已為 nullptr 的槽位: 2 / 2
// 全程沒有手動 delete，資源仍在正確時機釋放。
