// =====================================================================
// 主題 04: std::enable_shared_from_this  (從成員函式安全取得自身的 shared_ptr)
// =====================================================================
// 參考來源:
//   https://en.cppreference.com/w/cpp/memory/enable_shared_from_this
//   https://cplusplus.com/reference/memory/enable_shared_from_this/
//
// ---------------------------------------------------------------------
// 一、為什麼會需要這個東西?
// ---------------------------------------------------------------------
// 想像一個情境:
//   - 你的物件 (例如 Session) 是用 shared_ptr 管理的;
//   - 在它自己的成員函式裡, 你要把「自己」傳給別人 (例如 callback、
//     非同步任務、容器、訂閱清單);
//   - 但你只有 this (一個原生指標), 沒有 shared_ptr。
//
// 直覺寫法:
//      void Session::register_self() {
//          some_list.push_back(std::shared_ptr<Session>(this));   // ❌ 災難
//      }
//   這麼寫會建立一個「全新的 control block」, 與外部已有的
//   shared_ptr 毫無關聯, 結果會 double free。
//
// 正確寫法: 讓你的 class 繼承 std::enable_shared_from_this<T>,
// 然後使用 shared_from_this() 取得「與外部共享同一個 control block」
// 的 shared_ptr。
//
// ---------------------------------------------------------------------
// 二、它的運作原理
// ---------------------------------------------------------------------
// std::enable_shared_from_this<T> 內部有一個 std::weak_ptr<T> 成員。
// 當你「第一次」用 shared_ptr<T>(new T(...)) 或 std::make_shared<T>(...)
// 建立 T 物件時, shared_ptr 的建構子會偵測 T 是否繼承自
// enable_shared_from_this, 若是, 就把這個 weak_ptr 內部初始化指向
// 該物件對應的 control block。
//
// 之後 shared_from_this() 就是把這個內部 weak_ptr 升級 (lock) 成
// shared_ptr 回傳, 因此「擁有相同 control block」, 不會 double free。
//
// ---------------------------------------------------------------------
// 三、基本用法
// ---------------------------------------------------------------------
//   class Foo : public std::enable_shared_from_this<Foo> {
//   public:
//       std::shared_ptr<Foo>      ptr_to_self()       { return shared_from_this(); }
//       std::weak_ptr<Foo>        weak_to_self()      { return weak_from_this(); }
//   };
//
//   auto sp = std::make_shared<Foo>();
//   auto sp2 = sp->ptr_to_self();           // OK, 與 sp 共享 control block
//
// 補充:
//   - shared_from_this()  在「物件還沒被 shared_ptr 管理」時呼叫,
//     會丟出 std::bad_weak_ptr (C++17 起為定義行為)。
//   - weak_from_this()    (C++17) 同樣回傳內部 weak_ptr, 但不會丟例外,
//     若無 control block 就回傳一個 expired 的 weak_ptr。
//
// ---------------------------------------------------------------------
// 四、何時該用?
// ---------------------------------------------------------------------
// 1. 非同步任務: 一個物件啟動 lambda / std::async, 該 lambda 必須
//    確保物件還活著 -> capture shared_from_this()。
// 2. 觀察者 / 回呼 / 訊號槽: 把自己註冊到別的容器或事件源。
// 3. Builder pattern: 鏈式 API 回傳 shared_ptr<This> 而非 raw this。
// 4. 樹/圖的雙向連結, 子節點需要把「自己」傳出去。
//
// ---------------------------------------------------------------------
// 五、常見陷阱
// ---------------------------------------------------------------------
// 1. ❌ 在「建構子裡」呼叫 shared_from_this():
//      此時 shared_ptr 都還沒建立, 內部 weak_ptr 為空 -> bad_weak_ptr。
//      解法: 提供 static factory 函式 (如下方範例 1), 在物件建好之後
//            (確認已被 shared_ptr 管理) 才呼叫。
//
// 2. ❌ 物件不是用 shared_ptr 管理 (例如放在 stack 上或 unique_ptr):
//      也會丟 bad_weak_ptr。
//
// 3. ❌ 多重繼承時用錯型別: 應確保 enable_shared_from_this<T> 的 T
//      就是真正繼承的型別。
//
// 4. 注意: 過度使用 shared_from_this 可能讓物件壽命被「無意間」延長。
//    若 callback 內只是「想觀察, 不延壽」, 應 capture weak_from_this(),
//    使用時再 lock()。
//
// =====================================================================

/*
補充筆記：std::enable_shared_from_this
  - std::enable_shared_from_this 屬於智慧指標；先判斷所有權是獨占、共享，還是只是觀察。
  - unique_ptr 表達獨占所有權，不可複製但可 move；shared_ptr 表達共享所有權，weak_ptr 打破循環引用。
  - make_unique/make_shared 通常比裸 new 更安全；它們把配置和建構包在一起，降低例外中途洩漏風險。
  - get() 只取得借用指標，不轉移所有權；不要 delete get() 回傳值。
  - shared_ptr 的 control block 很重要；不要用同一裸指標建立多個 shared_ptr。
  - custom deleter 用於 FILE*、C API handle、特殊釋放函式；deleter 型別也會影響 unique_ptr 型別大小。
  - enable_shared_from_this 讓物件在成員函式中安全取得指向自己的 shared_ptr，但前提是物件已由 shared_ptr 管理。
  - 在建構子中呼叫 shared_from_this 通常會失敗，因為 control block 尚未把 weak_this 接好。
  - 不要同時用裸 this 建立新的 shared_ptr；這會產生第二個 control block 並導致 double delete。
*/
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <functional>

// =====================================================================
// 範例 (1): 最基本 - shared_from_this 的正確用法
// =====================================================================
// 觀念: 用 static factory 確保物件一定由 shared_ptr 管理, 之後
//       任何成員函式內都可安全 shared_from_this()。
class Widget : public std::enable_shared_from_this<Widget> {
    std::string name_;
    // 建構子設為 private, 強迫使用者只能透過 create() 建立。
    explicit Widget(std::string n) : name_(std::move(n)) {}
public:
    static std::shared_ptr<Widget> create(std::string n) {
        // 用 new + 私有建構子; make_shared 需要 public 建構子, 兩者擇一即可。
        return std::shared_ptr<Widget>(new Widget(std::move(n)));
    }

    // 安全取得自身 shared_ptr (與外部共享 control block)
    std::shared_ptr<Widget> get_self() {
        return shared_from_this();
    }

    const std::string& name() const { return name_; }
};

void example_basic() {
    std::cout << "--- 範例 1: 基本用法 ---\n";
    auto w  = Widget::create("alpha");
    auto w2 = w->get_self();                            // 取得自身指標
    std::cout << "w  use_count = " << w.use_count()  << "\n"; // 2
    std::cout << "w2 use_count = " << w2.use_count() << "\n"; // 2
    std::cout << "same object?  " << (w.get() == w2.get()) << "\n"; // 1
}

// =====================================================================
// 範例 (2): 錯誤示範 - 為什麼不能 shared_ptr<T>(this)
// =====================================================================
// 觀念: 直接用 this 包 shared_ptr 會建立第二個 control block,
//       兩個 control block 會各自嘗試 delete -> double free。
//       此處只示範「為什麼壞」, 並對比 shared_from_this 的正確做法。
class Bad {
public:
    // 模擬「錯誤」做法 (這段在實務中千萬不要寫!!)
    std::shared_ptr<Bad> wrong_self_ptr() {
        return std::shared_ptr<Bad>(this);              // ❌ 製造第二個 control block
    }
};

void example_bad_demo() {
    std::cout << "--- 範例 2: 錯誤示範 (僅展示原理, 不執行 double free) ---\n";
    auto b = std::make_shared<Bad>();
    std::cout << "b.use_count() = " << b.use_count() << "\n"; // 1
    // 不真的呼叫 b->wrong_self_ptr(), 因為呼叫了之後一旦兩邊都解構,
    // 會 double free。這裡只是讓讀者理解錯誤模式。
    std::cout << "(若呼叫 wrong_self_ptr 並讓兩端離開 scope, 程式會崩潰)\n";
    std::cout << "正確做法: 繼承 enable_shared_from_this 並用 shared_from_this()。\n";
}

// =====================================================================
// 範例 (3): 非同步 callback - 工作中最常見的用法
// =====================================================================
// 觀念: 把「自己」交給 callback 容器, 但要保證 callback 觸發時物件
//       還活著。lambda capture shared_from_this() 即可延長壽命。
class AsyncJob : public std::enable_shared_from_this<AsyncJob> {
    std::string id_;
public:
    explicit AsyncJob(std::string id) : id_(std::move(id)) {}

    // 把任務排程出去 (這裡用 vector<function> 模擬「事件迴圈」)
    void schedule(std::vector<std::function<void()>>& queue) {
        // capture shared_from_this(), 這樣即使原本的 owner 釋放了,
        // 任務真正執行時, AsyncJob 物件仍存在。
        auto self = shared_from_this();
        queue.push_back([self]() {
            std::cout << "  執行 AsyncJob[" << self->id_ << "]\n";
        });
    }
};

void example_async_callback() {
    std::cout << "--- 範例 3: 非同步 callback ---\n";
    std::vector<std::function<void()>> queue;
    {
        auto job = std::make_shared<AsyncJob>("download");
        job->schedule(queue);
        std::cout << "schedule 完, job.use_count = " << job.use_count() << "\n"; // 2
    } // 這裡原本的 job 離開 scope, 但 lambda 內還有 shared_ptr, 物件不會死

    std::cout << "原 owner 釋放後, 開始執行 queue:\n";
    for (auto& f : queue) f();
    queue.clear();                                      // 此時最後一個 ref 才消失
}

// =====================================================================
// 範例 (4): 觀察者模式 - 弱觀察, 不延長壽命
// =====================================================================
// 觀念: 與範例 3 相反。Observer 不希望被 Subject 抓著不放,
//       所以 Subject 持有 weak_ptr, 必要時用 weak_from_this() 取得。
class EventBus;                                          // 前向宣告

class Listener : public std::enable_shared_from_this<Listener> {
    std::string name_;
public:
    explicit Listener(std::string n) : name_(std::move(n)) {}
    void attach_to(EventBus& bus);
    void on_event(int v) const {
        std::cout << "  Listener[" << name_ << "] 收到 event " << v << "\n";
    }
};

class EventBus {
    std::vector<std::weak_ptr<Listener>> listeners_;     // 弱持有, 不延壽
public:
    void subscribe(std::weak_ptr<Listener> w) { listeners_.push_back(std::move(w)); }
    void publish(int v) {
        for (auto& w : listeners_) {
            if (auto sp = w.lock()) sp->on_event(v);
        }
    }
};

void Listener::attach_to(EventBus& bus) {
    // 這裡要交「自己」給 bus, 用 weak_from_this() 取得不延壽的觀察者
    bus.subscribe(weak_from_this());
}

void example_observer_with_weak_from_this() {
    std::cout << "--- 範例 4: weak_from_this 的觀察者 ---\n";
    EventBus bus;
    auto a = std::make_shared<Listener>("A");
    a->attach_to(bus);
    {
        auto b = std::make_shared<Listener>("B");
        b->attach_to(bus);
        bus.publish(1);                                  // A, B 都收到
    } // b 離開 scope, 因 bus 只持 weak, b 自動釋放
    bus.publish(2);                                      // 只有 A 收到
}

// =====================================================================
// 範例 (5): Builder / 鏈式 API
// =====================================================================
// 觀念: 鏈式設定常見於 builder 物件, 回傳 shared_ptr<Self> 可以
//       讓使用者持續持有, 不用擔心 raw pointer 失效。
class QueryBuilder : public std::enable_shared_from_this<QueryBuilder> {
    std::string sql_ = "SELECT *";
    QueryBuilder() = default;                           // 私有
public:
    static std::shared_ptr<QueryBuilder> create() {
        return std::shared_ptr<QueryBuilder>(new QueryBuilder());
    }
    std::shared_ptr<QueryBuilder> from(const std::string& tbl) {
        sql_ += " FROM " + tbl;
        return shared_from_this();
    }
    std::shared_ptr<QueryBuilder> where(const std::string& cond) {
        sql_ += " WHERE " + cond;
        return shared_from_this();
    }
    const std::string& build() const { return sql_; }
};

void example_builder() {
    std::cout << "--- 範例 5: 鏈式 Builder ---\n";
    auto q = QueryBuilder::create()
                 ->from("users")
                 ->where("id = 1");
    std::cout << "SQL = " << q->build() << "\n";
}

// =====================================================================
// 範例 6 (實用): 訂閱者註冊到事件中心並可隨時退訂
// =====================================================================
// 觀念: 物件「自己」想把自己加進別人的訂閱表, 又不延長壽命 -> weak_from_this。
//       這是 enable_shared_from_this 「最日常」的工程情境之一。
// =====================================================================
class EventCenter {
    std::vector<std::weak_ptr<class Worker>> subs_;
public:
    void subscribe(std::weak_ptr<class Worker> w) { subs_.push_back(std::move(w)); }
    void fire(int code);
};

class Worker : public std::enable_shared_from_this<Worker> {
    int id_;
public:
    explicit Worker(int id) : id_(id) {}
    void join(EventCenter& c) { c.subscribe(weak_from_this()); }
    int id() const { return id_; }
    void handle(int code) const {
        std::cout << "  Worker[" << id_ << "] handle code=" << code << "\n";
    }
};

void EventCenter::fire(int code) {
    for (auto& w : subs_) if (auto sp = w.lock()) sp->handle(code);
}

void example_worker_subscribe() {
    std::cout << "--- 範例 6 (實用): Worker 訂閱事件中心 ---\n";
    EventCenter center;
    auto w1 = std::make_shared<Worker>(1);
    w1->join(center);
    {
        auto w2 = std::make_shared<Worker>(2);
        w2->join(center);
        center.fire(100);                    // Worker 1, 2 都收到
    }
    center.fire(200);                        // 只有 Worker 1 收到 (w2 已過期)
}

// =====================================================================
// (本主題沒有特別合適的 Leetcode 對應題)
// 原因: Leetcode 函式呼叫多為單次、無回呼、無生命週期問題,
//       而 enable_shared_from_this 主要解決「物件把自己傳出去」的場景,
//       這在實務工程 (網路服務、async I/O、UI 事件) 才常見。
//
// 替代方案: 上面 5 個範例都是工作中真實會遇到的情境, 涵蓋:
//   - 基本用法
//   - 為什麼不能 shared_ptr<T>(this) (錯誤示範)
//   - 非同步 callback (capture shared_from_this 延壽)
//   - 觀察者模式 (capture weak_from_this 不延壽)
//   - Builder 鏈式 API
// =====================================================================

// =====================================================================
// main
// =====================================================================
int main() {
    example_basic();
    example_bad_demo();
    example_async_callback();
    example_observer_with_weak_from_this();
    example_builder();
    example_worker_subscribe();

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼建構子裡呼叫 shared_from_this() 會丟 bad_weak_ptr?
    //    A：enable_shared_from_this 內部那個 weak_ptr 是在「外部 shared_ptr
    //       建好之後」才被填入。建構子執行時, 外部 shared_ptr 的 ctor
    //       還沒結束, 內部 weak_ptr 仍是空, lock() 失敗 → 丟例外。解法
    //       是 static factory: 物件先 new 完, 再交給 shared_ptr ctor,
    //       之後才呼叫成員函式。
    //
    //  Q2：什麼時候 capture shared_from_this(), 什麼時候 capture
    //       weak_from_this()?
    //    A：「callback 必須執行完才能放手 (例如非同步 I/O)」 → capture
    //       shared_from_this 延壽。「callback 想觀察, 但物件死了就應
    //       放棄 (例如事件訂閱、定時器)」 → capture weak_from_this,
    //       觸發時 lock() 失敗就跳過, 避免無意間延長壽命造成洩漏。
    //
    //  Q3：可以從非 public 繼承 enable_shared_from_this 嗎?
    //    A：可以, 但通常選 public 繼承因為它本來就是教學/工具用基底,
    //       沒有 protected/private 的好理由。多重繼承時要確保 T 是
    //       enable_shared_from_this<T> 真正的最派生型別 (CRTP), 否則
    //       shared_from_this() 回傳的型別不對, 編譯或執行期都會出錯。
    //
    return 0;
}
