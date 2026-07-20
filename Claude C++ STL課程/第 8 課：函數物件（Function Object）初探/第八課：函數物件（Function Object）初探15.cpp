// =============================================================================
//  第八課 15  —  std::function：把「任何可呼叫物件」統一成同一個型別
// =============================================================================
//
// 【主題資訊 Information】
//   模板    : template<class R, class... Args> class std::function<R(Args...)>
//   標準版本: C++11（C++17 起支援 CTAD；C++23 另有 std::move_only_function）
//   標頭檔  : <functional>
//   複雜度  : operator() 為 O(1)，但含一次無法 inline 的間接呼叫
//   實測    : sizeof(std::function<int(int,int)>) = 32 bytes（本機 libstdc++，
//             實作定義；標準未規定大小）
//
// 【詳細解釋 Explanation】
//
// 【1. std::function 解決的問題：型別不統一】
//   普通函數、functor、lambda 都能被 () 呼叫，但它們的「型別」毫無關係：
//       int add(int,int);                       → int(*)(int,int)
//       class Multiplier { ... };               → Multiplier
//       [](int a,int b){ return a-b; };          → 某個 unnamed closure type
//   於是問題來了：std::vector<???> 要怎麼同時裝這三種東西？
//   std::function 的答案是「型別抹除（type erasure）」：對外只看簽名
//   int(int,int)，對內把實際物件藏起來，用虛擬分派呼叫它。
//   本檔的 std::vector<std::function<int(int,int)>> 就是這個能力的展示。
//
// 【2. 型別抹除是怎麼做到的（概念版實作）】
//   內部大致等價於：
//       struct CallableBase { virtual int call(int,int) = 0; virtual ~CallableBase(); };
//       template<class F> struct Callable : CallableBase {
//           F f;
//           int call(int a,int b) override { return f(a,b); }
//       };
//       // std::function 持有 CallableBase*（或 SBO 緩衝區）
//   所以每次呼叫都要經過一次虛擬（或等價的函數指標）分派——
//   這正是它「無法 inline」的根本原因。
//
// 【3. 三個必須知道的成本】
//   (a) 間接呼叫：通常無法 inline，也會擋住迴圈最佳化。
//   (b) 可能配置堆積：libstdc++ 有 small buffer optimization，
//       小的 closure 直接存在 std::function 內部（本機 sizeof 為 32 bytes）；
//       捕獲太多而放不下時就會 new 一塊記憶體。
//   (c) 空狀態：預設建構的 std::function 不持有任何目標，
//       此時呼叫它，標準保證丟出 std::bad_function_call
//       （這是明確定義的例外行為，不是 UB）。
//
// 【4. 什麼時候「不該」用 std::function】
//   如果呼叫端能寫成模板，就別用 std::function：
//       template<class F> void apply(F f);      // 好：型別保留，可 inline
//       void apply(std::function<int(int)> f);  // 差：多一層抹除
//   std::function 真正該出場的場合只有兩種：
//     * 要存進容器或類別成員（型別必須統一）；
//     * 要在執行期切換行為（callback、事件處理、狀態機）。
//   本檔的 operations 向量正是第一種。
//
// 【概念補充 Concept Deep Dive】
//   std::function 要求目標是 CopyConstructible。這代表「只能搬移不能複製」
//   的 closure（例如捕獲了 std::unique_ptr）放不進去——這是 C++11 的
//   一個長年痛點。C++23 加入 std::move_only_function 才正式補上。
//   另外，std::function 沒有 operator==（無法比較兩個 function 是否是
//   同一個目標），所以「註冊 callback 後要取消註冊」通常得自己配一個 id，
//   不能直接拿 function 去比對移除。
//
// 【注意事項 Pay Attention】
// 1. 空的 std::function 被呼叫會丟 std::bad_function_call。呼叫前可用
//    if (f) 檢查（operator bool），本檔會實際示範這個例外。
// 2. std::function 的 sizeof 與是否配置堆積都是「實作定義」，
//    不同標準庫（libstdc++／libc++／MSVC）數值不同，不可寫死在程式邏輯裡。
// 3. 熱路徑（每秒數百萬次呼叫的迴圈）避免使用 std::function，
//    改用模板參數保留具體型別。
// 4. 本檔除法 lambda [](int a,int b){ return a/b; } 對 b == 0 是 UB。
//    示範中 b 固定為 4，實務上務必先檢查除數。
// 5. std::function 儲存的若是捕獲參考的 lambda，懸空風險完全不會因為
//    包了一層而消失——反而因為 function 常被存起來延後呼叫而更危險。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::function
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::function 是怎麼做到「同一個型別裝下函數、functor、lambda」的？
//     答：型別抹除。它對外只暴露簽名 R(Args...)，對內用一個多型的持有者
//         （概念上是 virtual call）包住實際物件。代價是每次呼叫都要經過
//         一次間接分派，通常無法 inline。
//     追問：那它會不會配置記憶體？→ 視實作而定。libstdc++ 有 small buffer
//         optimization，小 closure 直接內嵌（本機 sizeof 為 32 bytes），
//         太大才 new。這是實作定義的行為。
//
// 🔥 Q2. 什麼時候該用 std::function、什麼時候不該？
//     答：需要「型別統一」時才用——存進容器、當類別成員、當 callback、
//         執行期切換行為。如果呼叫端可以寫成 template<class F>，
//         就該保留具體型別讓編譯器 inline，不要平白付出抹除的代價。
//     追問：為什麼模板版本比較快？→ 模板實例化後呼叫目標由型別決定、
//         編譯期固定，可完全 inline；std::function 把資訊從型別搬回值，
//         只能在執行期查表分派。
//
// ⚠️ 陷阱. std::function<int(int,int)> f;  然後直接 f(3, 5) 會發生什麼？
//     答：丟出 std::bad_function_call。注意這「不是」UB，而是標準明確
//         規定的例外，行為完全可預測、可用 try/catch 攔截。
//     為什麼會錯：很多人把它想成「空指標解參考」而預期崩潰或未定義行為，
//         於是要嘛以為不能測試、要嘛以為一定 segfault。實際上 std::function
//         有明確的空狀態語意（operator bool 可檢查、呼叫則丟例外），
//         這是它與裸函數指標的重要差別。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】從缺。
//   理由：std::function 的價值在於「執行期切換行為」與「型別統一以便存放」，
//   這是系統設計層面的需求。LeetCode 解法是單一固定演算法，用 std::function
//   只會憑空增加一層無法 inline 的間接呼叫，屬於反面示範。
//   本清單中的設計題（146 LRU、155 Min Stack 等）核心也在資料結構而非
//   可呼叫物件的抹除，故從缺，改以實務範例呈現它真正的使用時機。

#include <iostream>
#include <functional>
#include <vector>
#include <string>

// 普通函數
int add(int a, int b) {
    return a + b;
}

// 函數物件
class Multiplier {
public:
    int operator()(int a, int b) const {
        return a * b;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】訂單事件的處理器註冊表（event handler registry）
//   場景：訂單系統在「建立／付款／出貨」等事件發生時，要通知所有已註冊的
//         處理器（寄信、寫稽核 log、更新報表…）。處理器由不同模組提供，
//         有的是普通函數、有的是帶設定的 functor、有的是就地寫的 lambda。
//   為什麼用 std::function：這些處理器型別完全不同，卻必須存進同一個容器。
//     這正是型別抹除唯一無可取代的場景——也是願意付出間接呼叫成本的理由
//     （事件通知一秒最多幾千次，不是熱路徑）。
//   注意：這裡用 id 來取消註冊，而非拿 std::function 去比對——
//     std::function 沒有 operator==，無法比較兩個目標是否相同。
// -----------------------------------------------------------------------------
class EventBus {
private:
    // 用 (id, handler) 配對，才有辦法取消註冊
    std::vector<std::pair<int, std::function<void(const std::string&)>>> handlers_;
    int next_id_ = 1;
public:
    int subscribe(std::function<void(const std::string&)> h) {
        handlers_.emplace_back(next_id_, std::move(h));
        return next_id_++;
    }

    void unsubscribe(int id) {
        for (size_t i = 0; i < handlers_.size(); ++i) {
            if (handlers_[i].first == id) {
                handlers_.erase(handlers_.begin() + static_cast<long>(i));
                return;
            }
        }
    }

    void publish(const std::string& event) const {
        for (const auto& h : handlers_) {
            if (h.second) h.second(event);   // operator bool 檢查，避免空目標
        }
    }
};

// 供 EventBus 使用的普通函數處理器
void auditLog(const std::string& event) {
    std::cout << "  [audit] 記錄事件: " << event << "\n";
}

// 帶設定的 functor 處理器
class MailNotifier {
private:
    std::string to_;
public:
    explicit MailNotifier(std::string to) : to_(std::move(to)) {}
    void operator()(const std::string& event) const {
        std::cout << "  [mail ] 寄給 " << to_ << ": " << event << "\n";
    }
};

int main() {
    // std::function 可以儲存各種可呼叫物件
    std::function<int(int, int)> func;
    
    // 儲存普通函數
    func = add;
    std::cout << "普通函數: func(3, 5) = " << func(3, 5) << std::endl;
    
    // 儲存函數物件
    func = Multiplier();
    std::cout << "函數物件: func(3, 5) = " << func(3, 5) << std::endl;
    
    // 儲存 Lambda
    func = [](int a, int b) { return a - b; };
    std::cout << "Lambda: func(3, 5) = " << func(3, 5) << std::endl;
    
    // 實際應用：儲存多個操作
    std::cout << "\n=== 儲存多個操作 ===" << std::endl;
    std::vector<std::function<int(int, int)>> operations;
    
    operations.push_back(add);
    operations.push_back(Multiplier());
    operations.push_back([](int a, int b) { return a - b; });
    operations.push_back([](int a, int b) { return a / b; });
    
    int a = 20, b = 4;
    std::vector<std::string> names = {"加", "乘", "減", "除"};
    
    for (size_t i = 0; i < operations.size(); ++i) {
        std::cout << a << " " << names[i] << " " << b << " = "
                  << operations[i](a, b) << std::endl;
    }

    // ---- 空的 std::function：明確定義的例外，不是 UB ----
    std::cout << "\n=== 空的 std::function ===" << std::endl;
    std::function<int(int, int)> empty_fn;
    std::cout << "empty_fn 是否有目標: " << std::boolalpha
              << static_cast<bool>(empty_fn) << std::noboolalpha << std::endl;
    try {
        empty_fn(3, 5);           // 標準保證丟 std::bad_function_call
        std::cout << "不會走到這裡" << std::endl;
    } catch (const std::bad_function_call& e) {
        std::cout << "攔截到例外: " << e.what() << std::endl;
    }

    // ---- sizeof：實作定義 ----
    std::cout << "sizeof(std::function<int(int,int)>) = "
              << sizeof(std::function<int(int, int)>)
              << " bytes（本機 libstdc++ 實測，實作定義）" << std::endl;

    // ---- 日常實務：事件處理器註冊表 ----
    std::cout << "\n=== 日常實務: 訂單事件註冊表 ===" << std::endl;
    EventBus bus;
    bus.subscribe(auditLog);                          // 普通函數
    bus.subscribe(MailNotifier("ops@example.com"));   // 帶狀態的 functor
    int report_id = bus.subscribe(                    // 就地寫的 lambda
        [](const std::string& ev) {
            std::cout << "  [report] 更新報表計數: " << ev << "\n";
        });

    std::cout << "publish(order.created):" << std::endl;
    bus.publish("order.created");

    bus.unsubscribe(report_id);
    std::cout << "取消 report 訂閱後 publish(order.paid):" << std::endl;
    bus.publish("order.paid");

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第八課：函數物件（Function Object）初探15.cpp -o std_function

// 【但書】sizeof(std::function<int(int,int)>) = 32 bytes 是本機 g++ 15.2 /
//   libstdc++ 的實作定義結果。標準未規定該型別大小，libc++ 與 MSVC 的數值
//   不同，換平台重新編譯時這一行輸出會變。

// === 預期輸出 ===
// 普通函數: func(3, 5) = 8
// 函數物件: func(3, 5) = 15
// Lambda: func(3, 5) = -2
//
// === 儲存多個操作 ===
// 20 加 4 = 24
// 20 乘 4 = 80
// 20 減 4 = 16
// 20 除 4 = 5
//
// === 空的 std::function ===
// empty_fn 是否有目標: false
// 攔截到例外: bad_function_call
// sizeof(std::function<int(int,int)>) = 32 bytes（本機 libstdc++ 實測，實作定義）
//
// === 日常實務: 訂單事件註冊表 ===
// publish(order.created):
//   [audit] 記錄事件: order.created
//   [mail ] 寄給 ops@example.com: order.created
//   [report] 更新報表計數: order.created
// 取消 report 訂閱後 publish(order.paid):
//   [audit] 記錄事件: order.paid
//   [mail ] 寄給 ops@example.com: order.paid
