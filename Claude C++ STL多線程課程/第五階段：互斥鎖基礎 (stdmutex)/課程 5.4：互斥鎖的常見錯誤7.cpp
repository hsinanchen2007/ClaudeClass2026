// =============================================================================
//  課程 5.4：互斥鎖的常見錯誤7.cpp  —  用重構取代 recursive_mutex
// =============================================================================
//
// 【主題資訊 Information】
//   本檔沒有新的 API，介紹的是一個【設計慣例】：
//       公開介面（public）  → 負責上鎖，然後呼叫私有實作
//       私有實作（private） → 【假設鎖已被持有】，自己完全不碰鎖
//   命名慣例（業界常見）：
//       doXxx() / xxxImpl() / xxxLocked() / xxx_nolock()
//       —— 重點是讓「這個函式不上鎖」在名字上就看得出來
//   標頭檔：<mutex>
//   相關背景：上一個檔案（常見錯誤6）示範了 recursive_mutex，
//             本檔示範不需要它的做法。
//
// 【詳細解釋 Explanation】
//
// 【1. 問題的根源：「上鎖」與「做事」被綁在同一個函式裡】
//   會需要重入，幾乎都是因為一個函式同時扛了兩個責任：
//       void innerFunction() {
//           mtx.lock();          // 責任 A：確保執行緒安全
//           doTheActualWork();   // 責任 B：真正的業務邏輯
//           mtx.unlock();
//       }
//   於是任何想重用「責任 B」的呼叫者，都被迫連「責任 A」一起吃下去。
//   當呼叫者【自己已經持有鎖】時，就撞上 std::mutex 不可重入的 UB。
//   把兩個責任拆開，問題就從根本消失了——
//   這不是繞過限制的技巧，而是把原本混在一起的關注點分離。
//
// 【2. 拆開之後，每個函式的「鎖前提」變成明確的契約】
//   重構後每個函式都有一個清楚、可被檢查的前提條件：
//       innerFunctionImpl()  —— 前提：呼叫者【必須】已持有 mtx
//       innerFunction()      —— 前提：呼叫者【必須】未持有 mtx
//   這個契約應該寫在註解裡，因為編譯器無法幫你檢查。
//   ⚠️ Clang 的 Thread Safety Analysis（-Wthread-safety）可以：
//       void innerFunctionImpl() REQUIRES(mtx);
//   加上標註後，編譯器會在你忘記上鎖時直接報錯。
//   GCC 目前沒有等價功能，所以在 GCC 專案裡只能靠命名慣例與註解。
//
// 【3. 這個重構同時解決了另外兩個問題】
//   除了避免重入，它還帶來：
//     (a) 【臨界區段變短且可預測】：鎖只在公開函式的一層存在，
//         看程式碼就能算出臨界區段有多長，不必追蹤呼叫鏈。
//     (b) 【可以組合出原子的複合操作】：
//             void moveItem(K from, K to) {
//                 std::lock_guard<std::mutex> lock(mtx);
//                 auto v = getImpl(from);      // 不上鎖
//                 eraseImpl(from);             // 不上鎖
//                 putImpl(to, v);              // 不上鎖
//             }                                // 整段是一個原子操作
//         如果每個小函式都自己上鎖，這三步之間就有空隙，
//         別的執行緒可以看到「兩邊都沒有」的中間狀態。
//         這正是本課 5.1 講的「每個方法都加鎖 ≠ 組合起來安全」的解法。
//
// 【4. 什麼時候這個重構不划算】
//   誠實地說，它有成本：每個操作變成兩個函式，介面數量翻倍。
//   當一個類別有二十種操作、而且大量互相呼叫時，
//   維護四十個函式的配對關係本身就是負擔。
//   這種情況下，recursive_mutex 是務實的取捨（見上一檔的目錄樹範例）。
//   → 判準：重入是【少數幾處】就重構；是【瀰漫全類別】的模式，
//     則要嘛接受 recursive_mutex，要嘛重新思考這個類別是不是責任太多了。
//
// 【概念補充 Concept Deep Dive】
//   * 這個模式與 Pimpl（Pointer to Implementation）不同，別混淆：
//     Pimpl 是為了隔離編譯相依、隱藏實作細節；
//     本檔的 Impl 後綴只是「不上鎖版本」的命名慣例，兩者無關。
//   * 命名的選擇很重要。xxxLocked() 這個名字其實有歧義——
//     它可以讀成「這個函式會上鎖」或「這個函式要求已上鎖」。
//     Linux kernel 用 __xxx()（雙底線）表示「內部、不加鎖」，
//     有些專案用 xxx_nolock() 更明確。
//     ⚠️ 團隊內統一即可，但務必在註解寫清楚前提條件。
//   * 更進階的做法是用型別強制契約——把「已持有鎖」的證明作為參數傳入：
//         void doWorkImpl(const std::lock_guard<std::mutex>&) { ... }
//     這樣沒有鎖就無法呼叫。缺點是參數噪音，實務上較少見，
//     但在關鍵程式碼裡值得考慮。
//
// 【注意事項 Pay Attention】
//   1. 私有實作函式【絕對不可】自己上鎖，否則重構失去意義。
//   2. 每個 Impl 函式都要在註解寫明「前提：呼叫者已持有 mtx」。
//   3. 公開函式之間【不可】互相呼叫（那又會重入）——
//      要組合就在公開函式裡呼叫多個 Impl。
//   4. 編譯器不檢查這個契約（GCC）；Clang 可用 -Wthread-safety 標註。
//   5. 操作種類非常多時，這個模式的維護成本會超過收益，
//      此時 recursive_mutex 是合理的取捨。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】以重構取代重入
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 兩個都需要鎖的公開函式互相呼叫造成重入，除了 recursive_mutex
//        還有什麼解法？哪個比較好？
//     答：把「上鎖」與「做事」拆開——業務邏輯抽成【不上鎖】的私有函式
//         （命名如 doXxxImpl / xxx_nolock），公開函式負責上鎖後呼叫它。
//         這比 recursive_mutex 好，因為：臨界區段變得可預測（只有一層）、
//         不會在「不變量暫時被破壞」的中間狀態上工作、
//         而且可以自由組合多個 Impl 成為一個原子的複合操作。
//     追問：那 recursive_mutex 完全不該用嗎？→ 不是。
//           當重入是瀰漫整個類別的模式（例如遞迴走訪樹狀結構的十幾種操作），
//           維護二十對 public/private 函式的成本可能超過收益，
//           這時 recursive_mutex 是務實的選擇。
//
// 🔥 Q2. 這個模式的「契約」是什麼？誰來保證它被遵守？
//     答：契約是「Impl 函式的呼叫者必須已持有鎖」。
//         在 GCC 下【沒有人保證】——只能靠命名慣例與註解，
//         程式碼審查是最後一道防線。
//         Clang 的 Thread Safety Analysis 提供 REQUIRES(mtx) 標註，
//         配合 -Wthread-safety 可以讓編譯器在忘記上鎖時直接報錯。
//     追問：有沒有純 C++ 的強制方式？
//           → 有一種手法是把 lock_guard 的參考當成參數傳給 Impl 函式，
//             沒有鎖就無法呼叫。代價是參數噪音，但在關鍵程式碼裡值得。
//
// ⚠️ 陷阱. 重構之後，公開函式 A 需要用到公開函式 B 的功能，
//        直接呼叫 B 就好了吧？
//     答：不行，那又回到原點了。A 已經持有鎖，B 進去又要 lock 一次
//         → 同一執行緒重複鎖定 std::mutex → 未定義行為。
//         正確做法是讓 A 呼叫 B 對應的【Impl 版本】。
//     為什麼會錯：重構完成後，很容易只記得「已經解決重入問題了」，
//         卻忘記解決方式是「公開層只有一層」這個結構性前提。
//         一旦公開函式之間開始互相呼叫，那個前提就被打破，
//         原本的問題會原封不動地回來——而且因為程式碼看起來「已經重構過」，
//         反而更難被察覺。防範方式：在公開函式的註解裡明確寫上
//         「不可被其他公開函式呼叫」。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

std::mutex mtx;

// 內部實作（假設鎖已被持有，不加鎖）
// 前提條件：呼叫者【必須】已持有 mtx
void innerFunctionImpl() {
    std::cout << "Inner function" << std::endl;
}

// 公開介面（需要鎖）
// 前提條件：呼叫者【必須】未持有 mtx；本函式不可被其他公開函式呼叫
void innerFunction() {
    std::lock_guard<std::mutex> lock(mtx);
    innerFunctionImpl();
}

void outerFunction() {
    std::lock_guard<std::mutex> lock(mtx);
    std::cout << "Outer function" << std::endl;

    innerFunctionImpl();  // ✓ 呼叫不加鎖的版本（不是 innerFunction()）
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 707. Design Linked List
//   題目：自行設計一個鏈結串列，支援 get / addAtHead / addAtTail /
//         addAtIndex / deleteAtIndex。
//   為什麼用到本主題：707 是這個重構模式的完美教材。
//         注意 addAtHead(v) 其實就是 addAtIndex(0, v)、
//         addAtTail(v) 就是 addAtIndex(size, v)——
//         公開函式之間【天然想要互相呼叫】。
//         若每個公開函式都自己上鎖，addAtHead 呼叫 addAtIndex 就會
//         同一執行緒重複鎖定 std::mutex → 未定義行為。
//         正解就是本檔的模式：所有邏輯放進不上鎖的 Impl 函式，
//         公開函式只負責上鎖 + 轉呼叫。
//         另外還示範了「組合出原子的複合操作」：
//         moveToFront() 把「刪除 + 插入」包在同一個臨界區段內，
//         中間不會被別的執行緒看到「元素不存在」的破碎狀態。
//   註：LeetCode 原題是單執行緒的；這裡做成執行緒安全版本。
// -----------------------------------------------------------------------------
class MyLinkedList {
private:
    struct Node {
        int   val;
        Node* next = nullptr;
        explicit Node(int v) : val(v) {}
    };

    mutable std::mutex mtx_;
    Node* head_ = nullptr;
    int   size_ = 0;

    // ── 以下全部是「假設鎖已被持有」的私有實作，自己不碰鎖 ──

    // 前提：已持有 mtx_
    int getImpl(int index) const {
        if (index < 0 || index >= size_) return -1;
        Node* cur = head_;
        for (int i = 0; i < index; ++i) cur = cur->next;
        return cur->val;
    }

    // 前提：已持有 mtx_
    void addAtIndexImpl(int index, int val) {
        if (index > size_) return;
        if (index < 0) index = 0;

        if (index == 0) {
            Node* node = new Node(val);
            node->next = head_;
            head_ = node;
        } else {
            Node* prev = head_;
            for (int i = 0; i < index - 1; ++i) prev = prev->next;
            Node* node = new Node(val);
            node->next = prev->next;
            prev->next = node;
        }
        ++size_;
    }

    // 前提：已持有 mtx_
    void deleteAtIndexImpl(int index) {
        if (index < 0 || index >= size_) return;

        if (index == 0) {
            Node* dead = head_;
            head_ = head_->next;
            delete dead;
        } else {
            Node* prev = head_;
            for (int i = 0; i < index - 1; ++i) prev = prev->next;
            Node* dead = prev->next;
            prev->next = dead->next;
            delete dead;
        }
        --size_;
    }

public:
    MyLinkedList() = default;

    ~MyLinkedList() {
        std::lock_guard<std::mutex> lock(mtx_);
        while (head_) {
            Node* dead = head_;
            head_ = head_->next;
            delete dead;
        }
    }

    MyLinkedList(const MyLinkedList&)            = delete;
    MyLinkedList& operator=(const MyLinkedList&) = delete;

    // ── 以下全部是公開介面：只負責上鎖 + 轉呼叫 Impl ──
    // ⚠️ 這些函式彼此【不可】互相呼叫，否則會重入

    int get(int index) const {
        std::lock_guard<std::mutex> lock(mtx_);
        return getImpl(index);
    }

    void addAtHead(int val) {
        std::lock_guard<std::mutex> lock(mtx_);
        addAtIndexImpl(0, val);           // ✓ 呼叫 Impl，不是 addAtIndex()
    }

    void addAtTail(int val) {
        std::lock_guard<std::mutex> lock(mtx_);
        addAtIndexImpl(size_, val);       // ✓ 同上
    }

    void addAtIndex(int index, int val) {
        std::lock_guard<std::mutex> lock(mtx_);
        addAtIndexImpl(index, val);
    }

    void deleteAtIndex(int index) {
        std::lock_guard<std::mutex> lock(mtx_);
        deleteAtIndexImpl(index);
    }

    int size() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return size_;
    }

    // ★ 重構帶來的額外好處：可以組合出【原子的】複合操作。
    //   若 deleteAtIndex 與 addAtHead 各自上鎖，兩者之間會有空隙，
    //   別的執行緒可能看到「這個元素憑空消失了」的中間狀態。
    void moveToFront(int index) {
        std::lock_guard<std::mutex> lock(mtx_);   // 整段只上一次鎖
        const int val = getImpl(index);
        if (val == -1) return;
        deleteAtIndexImpl(index);
        addAtIndexImpl(0, val);
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】訂單狀態機：公開 API 與內部轉移邏輯分離
//   情境：訂單有多種狀態轉移（付款、出貨、取消、退款）。
//         「取消」內部需要先「退款」再「還原庫存」——
//         而「退款」本身也是一個公開 API。
//   若每個公開 API 都自己上鎖，cancel() 呼叫 refund() 就會重入。
//   本例用同樣的模式：所有轉移邏輯放在 Impl，公開 API 只上鎖轉呼叫；
//   cancel() 因此能把「退款 + 還原庫存 + 改狀態」做成一個原子操作。
// -----------------------------------------------------------------------------
class OrderService {
public:
    enum class State { Created, Paid, Shipped, Cancelled, Refunded };

private:
    mutable std::mutex mtx_;
    State state_ = State::Created;
    long  refundedAmount_ = 0;
    int   restockedItems_ = 0;

    // 前提：已持有 mtx_
    bool refundImpl(long amount) {
        if (state_ != State::Paid && state_ != State::Shipped) return false;
        refundedAmount_ += amount;
        state_ = State::Refunded;
        return true;
    }

    // 前提：已持有 mtx_
    void restockImpl(int items) {
        restockedItems_ += items;
    }

public:
    void pay() {
        std::lock_guard<std::mutex> lock(mtx_);
        if (state_ == State::Created) state_ = State::Paid;
    }

    bool refund(long amount) {
        std::lock_guard<std::mutex> lock(mtx_);
        return refundImpl(amount);
    }

    // 取消 = 退款 + 還原庫存 + 改狀態，三件事必須是一個整體
    bool cancel(long amount, int items) {
        std::lock_guard<std::mutex> lock(mtx_);   // 只上一次鎖
        if (!refundImpl(amount)) return false;    // ✓ 呼叫 Impl，不是 refund()
        restockImpl(items);
        state_ = State::Cancelled;
        return true;
    }

    State state() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return state_;
    }

    long refunded() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return refundedAmount_;
    }

    int restocked() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return restockedItems_;
    }
};

const char* stateName(OrderService::State s) {
    switch (s) {
        case OrderService::State::Created:   return "Created";
        case OrderService::State::Paid:      return "Paid";
        case OrderService::State::Shipped:   return "Shipped";
        case OrderService::State::Cancelled: return "Cancelled";
        case OrderService::State::Refunded:  return "Refunded";
    }
    return "?";
}

int main() {
    std::cout << "=== 課程示範: 公開介面上鎖 + 私有實作不上鎖 ===" << std::endl;
    outerFunction();
    std::cout << "完成" << std::endl;
    // 證明 outerFunction 正常解鎖了：現在還能透過公開介面再取一次鎖
    innerFunction();

    std::cout << "\n=== LeetCode 707. Design Linked List ===" << std::endl;
    {
        MyLinkedList list;
        list.addAtHead(1);
        list.addAtTail(3);
        list.addAtIndex(1, 2);          // 串列變成 1 -> 2 -> 3
        std::cout << "get(1) = " << list.get(1) << "  (預期 2)" << std::endl;
        list.deleteAtIndex(1);          // 串列變成 1 -> 3
        std::cout << "get(1) = " << list.get(1) << "  (預期 3)" << std::endl;
        std::cout << "size() = " << list.size() << "  (預期 2)" << std::endl;

        // 複合操作：把索引 1 的元素搬到最前面（原子）
        list.moveToFront(1);            // 串列變成 3 -> 1
        std::cout << "moveToFront(1) 後 get(0) = " << list.get(0)
                  << "  (預期 3)" << std::endl;

        // 多執行緒壓測：8 條執行緒各插入 500 個節點
        MyLinkedList shared;
        std::vector<std::thread> workers;
        for (int t = 0; t < 8; ++t) {
            workers.emplace_back([&shared]() {
                for (int i = 0; i < 500; ++i) shared.addAtHead(i);
            });
        }
        for (auto& t : workers) t.join();
        std::cout << "8 執行緒各插入 500 個節點後 size() = " << shared.size()
                  << "  (必須是 4000)" << std::endl;
    }

    std::cout << "\n=== 日常實務: 訂單狀態機（取消 = 退款 + 還原庫存）===" << std::endl;
    {
        OrderService order;
        order.pay();
        std::cout << "付款後狀態: " << stateName(order.state())
                  << "  (預期 Paid)" << std::endl;

        const bool ok = order.cancel(1200, 3);
        std::cout << "cancel(1200, 3) = " << (ok ? "true" : "false")
                  << "  (預期 true)" << std::endl;
        std::cout << "狀態: " << stateName(order.state())
                  << "  (預期 Cancelled)" << std::endl;
        std::cout << "退款金額: " << order.refunded()
                  << "  (預期 1200)" << std::endl;
        std::cout << "還原庫存: " << order.restocked()
                  << "  (預期 3)" << std::endl;
        std::cout << "重點: cancel() 呼叫的是 refundImpl() 而非 refund()，"
                     "整段是一個原子操作" << std::endl;

        // 重複取消應該失敗（狀態已不是 Paid/Shipped）
        const bool again = order.cancel(500, 1);
        std::cout << "重複 cancel = " << (again ? "true" : "false")
                  << "  (預期 false，狀態機不允許)" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.4：互斥鎖的常見錯誤7.cpp' -o refactor_nolock

// -----------------------------------------------------------------------------
// 【輸出但書】
//   本檔輸出完全確定：所有操作都是原子的，多執行緒區段只驗證
//   「總節點數」這個不變量，不依賴任何排程順序。
//   （各執行緒插入的先後順序每次都不同，但總數必然是 4000。）
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// === 課程示範: 公開介面上鎖 + 私有實作不上鎖 ===
// Outer function
// Inner function
// 完成
// Inner function
//
// === LeetCode 707. Design Linked List ===
// get(1) = 2  (預期 2)
// get(1) = 3  (預期 3)
// size() = 2  (預期 2)
// moveToFront(1) 後 get(0) = 3  (預期 3)
// 8 執行緒各插入 500 個節點後 size() = 4000  (必須是 4000)
//
// === 日常實務: 訂單狀態機（取消 = 退款 + 還原庫存）===
// 付款後狀態: Paid  (預期 Paid)
// cancel(1200, 3) = true  (預期 true)
// 狀態: Cancelled  (預期 Cancelled)
// 退款金額: 1200  (預期 1200)
// 還原庫存: 3  (預期 3)
// 重點: cancel() 呼叫的是 refundImpl() 而非 refund()，整段是一個原子操作
// 重複 cancel = false  (預期 false，狀態機不允許)
