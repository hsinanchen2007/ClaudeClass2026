// =============================================================================
//  第 26 課：this 指標8.cpp  —  誤區一：在建構函數中把 this 洩漏出去
// =============================================================================
//
// 【主題資訊 Information】
//   議題：constructor 執行期間，物件尚未「完全成形」，此時把 this 交給外界
//         （註冊到容器、傳給執行緒、傳給回呼、丟給 shared_ptr）是典型的
//         「escaping this / leaking this from constructor」缺陷。
//   標準：物件的生命週期自「建構函數執行完畢」才開始（[basic.life]）。
//         在建構期間，物件已有儲存空間與 this，但衍生類別部分尚未初始化。
//   標頭檔：本檔用到 <memory>（示範 shared_ptr 相關陷阱）、<vector>、<string>
//   複雜度：與主題無關（這是正確性議題，不是效能議題）
//
// 【詳細解釋 Explanation】
//
// 【1. 建構函數執行時，物件到底「完成到哪裡」】
//   建構一個 Derived 物件的實際順序是：
//       ① 配置儲存空間（this 在此刻就已經是有效位址）
//       ② 依序建構 base class 子物件（Base 的建構函數在此執行）
//       ③ 依**宣告順序**建構 Derived 的非靜態資料成員
//       ④ 執行 Derived 建構函數的函式主體
//       ⑤ 到此為止，物件的生命週期才正式開始
//   所以在 ② 的當下：this 位址有效、Base 的成員有效，
//   但 **Derived 的成員還是未初始化的原始記憶體**，vptr 也還指向 Base 的 vtable。
//
//   這就是「this 有效 ≠ 物件可用」。位址早在第一行程式碼執行前就確定了，
//   但它指向的內容要到最後一刻才完整。
//
// 【2. 洩漏 this 的三種真實災難】
//   (a) 註冊到觀察者 / 事件匯流排
//       Base 建構函數裡寫 bus.subscribe(this)；若匯流排在建構尚未結束時就派送事件，
//       呼叫到的 virtual 函式會**解析成 Base 的版本**（見下一點），
//       而該版本若碰到 Derived 的成員就是未定義行為。
//   (b) 丟給執行緒
//       建構函數裡 std::thread t(&Worker::loop, this)；新執行緒可能在
//       建構函數還沒返回時就開始跑，讀到半成品物件 → data race + 讀未初始化記憶體。
//   (c) 交給 shared_ptr
//       建構函數裡 std::shared_ptr<T>(this) 會建立**第二個獨立的控制區塊**，
//       導致同一物件被兩組 reference count 管理，最終雙重釋放。
//       正確作法是繼承 std::enable_shared_from_this 並在建構**之後**
//       才呼叫 shared_from_this()（在建構函數內呼叫會丟 std::bad_weak_ptr）。
//
// 【3. 建構函數中呼叫 virtual 函式：這不是 UB，而是「解析成當前類別的版本」】
//   很多人以為「在建構函數呼叫虛擬函式是未定義行為」。要講精確：
//     * 呼叫一般的 virtual 函式：**行為明確定義**——動態型別在建構期間就是
//       「當前正在建構的那個類別」，因此呼叫到的是 Base 的版本，不是 Derived 的覆寫。
//       這正是本檔可以安全示範的原因，輸出完全決定性。
//     * 若該函式是 **pure virtual** 且沒有定義：那才是未定義行為
//       （實務上多半以 "pure virtual method called" 終止程式，但標準未保證）。
//   結論：危險之處不在於「會不會爆」，而在於**它安靜地叫錯了函式**——
//   你以為多型會生效，實際上被靜默降級成 Base 的行為。
//
// 【4. 正確的替代方案：兩階段初始化 / 工廠函式】
//   讓建構函數只負責「把自己準備好」，把「對外註冊」搬到建構完成之後：
//       static std::shared_ptr<Widget> create(...) {
//           auto p = std::shared_ptr<Widget>(new Widget(...));  // ① 完整建構
//           bus.subscribe(p);                                   // ② 建構完才註冊
//           return p;
//       }
//   把建構函數設為 private / protected，強制所有人走工廠函式，
//   就能從型別層面杜絕「有人忘記呼叫 init()」。
//   本檔尾端的【日常實務範例】完整示範這個模式。
//
// 【概念補充 Concept Deep Dive】
//   * vptr 的動態更新：在 Itanium C++ ABI 下，Base 的建構函數會先把 vptr 設成
//     Base 的 vtable；等 Derived 建構函數開始執行時再改寫成 Derived 的 vtable。
//     「建構期間 virtual 解析成當前類別」不是編譯器特別開的後門，
//     而是 vptr 在那個時間點的實際值就是如此。解構時順序反過來，
//     所以在 Base 解構函數裡呼叫 virtual 同樣只會叫到 Base 版本。
//   * 建構函數丟出例外時，**已完成的成員與 base 會被反向解構，但該物件自己的
//     解構函數不會被呼叫**（因為生命週期從未開始）。若你已經把 this 註冊出去，
//     外界就會持有一個永遠不會被解構、卻已經失效的指標。
//   * enable_shared_from_this 的 weak_ptr 是在 shared_ptr 接管物件時才被填入，
//     這也是為什麼在建構函數內呼叫 shared_from_this() 必然失敗
//     （C++17 起明確規定丟 std::bad_weak_ptr）。
//
// 【注意事項 Pay Attention】
//   1. 「this 在建構函數中已是有效位址」是對的；「物件已可用」是錯的。兩者要分開。
//   2. 在建構函數呼叫非純虛擬函式**不是 UB**，是「解析成當前類別版本」；
//      呼叫沒有定義的純虛擬函式才是 UB。不要把兩者混為一談。
//   3. 建構函數裡絕不要做 shared_ptr<T>(this)；要用 shared_from_this，
//      而且只能在物件已被 shared_ptr 接管之後呼叫。
//   4. 同樣的道理適用於解構函數：一旦進入 ~Base()，Derived 部分已經沒了，
//      此時 virtual 只會叫到 Base 版本，且務必先把自己從所有註冊處移除。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】建構函數中的 this
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 在建構函數裡呼叫 virtual 函式會發生什麼事？
//     答：行為是明確定義的——動態型別在建構期間就是「當前正在建構的類別」，
//         所以呼叫到的是**當前類別的版本**，衍生類別的覆寫不會生效。
//         底層原因是 vptr 在 Base 建構函數執行時還指向 Base 的 vtable。
//     追問：那什麼情況才是未定義行為？
//         → 呼叫的是沒有定義的 pure virtual 函式時。實務上常見
//           "pure virtual method called" 後終止，但標準並未保證任何特定結果。
//
// 🔥 Q2. 為什麼建構函數裡不能寫 std::shared_ptr<T>(this)?
//     答：那會建立一個**全新的控制區塊**。若外部也用 shared_ptr 管理同一物件，
//         就會有兩組互不知情的 reference count，各自歸零時各釋放一次 → double free。
//         正確作法是繼承 std::enable_shared_from_this，並在物件已被 shared_ptr
//         接管之後才呼叫 shared_from_this()。
//     追問：在建構函數裡直接呼叫 shared_from_this() 行不行？
//         → 不行。此時內部的 weak_ptr 尚未被填入，C++17 起明確規定會丟
//           std::bad_weak_ptr。
//
// ⚠️ 陷阱1. 「建構函數裡的 this 還是空的／還沒配置，所以不能用。」
//     答：不對。儲存空間在建構函數執行前就已配置，this 從第一行起就是有效位址，
//         甚至可以合法地拿來初始化自己的成員。危險的不是 this 本身，
//         而是「把它交給外界，讓外界在物件尚未完整時就使用它」。
//     為什麼會錯：把「位址有效」和「物件建構完成」混為一談。
//         C++ 明確區分這兩個時間點：位址在配置後即有效，
//         但生命週期要到建構函數正常返回才開始。
//
// ⚠️ 陷阱2. 「建構函數裡開一條 thread 跑自己的成員函數，反正物件馬上就好了。」
//     答：新執行緒可能在建構函數返回前就被排程執行，讀到尚未初始化的
//         衍生類別成員，這是 data race 加上讀取未初始化記憶體。
//         而且若建構函數之後丟出例外，物件根本不會誕生，執行緒卻已經在跑了。
//     為什麼會錯：以為「建構函數很短，執行緒不可能那麼快跑起來」——
//         這是把競態條件當成機率問題。競態的正確性不能靠時序僥倖。
//
// ⚠️ 陷阱3. 「建構函數丟例外沒關係，解構函數會幫我收拾。」
//     答：不會。物件的生命週期尚未開始，它自己的解構函數**不會**被呼叫；
//         只有已建構完成的成員與 base 子物件會被反向解構。
//         若你已在建構函數中把 this 註冊到外部容器，那個懸空指標就留在那裡了。
//     為什麼會錯：以為「建構函數執行過了就算物件存在」。
//         實際上必須「正常返回」才算數。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <memory>
#include <string>
#include <vector>
using namespace std;

class Dangerous {
private:
    int value_;

public:
    Dangerous(int v) : value_(v) {
        // ⚠ 在建構函數中，對象還沒完全初始化
        // 如果把 this 傳給外部，外部可能會使用未初始化的部分

        cout << "  建構中... value_ = " << value_ << endl;

        // 原始版本這裡印的是 this 的位址。位址受 ASLR 影響每次執行都不同，
        // 不能寫進「預期輸出」，因此改印決定性的事實：
        // this 在建構函數中已經是有效位址，而且指向的就是我們正在建構的物件。
        cout << "  this 已是有效位址？ " << boolalpha << (this != nullptr)
             << "（但物件尚未完全就緒）" << endl;

        // 這裡把 this 傳出去是危險的：
        // someGlobalFunction(this);  // ⚠ 對象可能還沒完全建構
    }

    // 供呼叫端在不列印位址的前提下驗證身分
    bool isSameObject(const Dangerous* p) const { return this == p; }
};

// -----------------------------------------------------------------------------
// 示範一：建構期間的 virtual 解析
//
// 注意：這**不是**未定義行為。標準規定建構期間物件的動態型別就是
// 「正在建構的那個類別」，因此 Base 建構函數中呼叫 describe()
// 一定叫到 Base::describe()，輸出完全決定性。
// 危險之處在於它「安靜地叫錯函式」，而不是它會崩潰。
// -----------------------------------------------------------------------------
class Widget {
public:
    Widget() {
        cout << "  Widget() 建構中，呼叫 describe() → ";
        describe();          // 解析成 Widget::describe()，即使真正在建構 Button
    }
    virtual ~Widget() = default;
    virtual void describe() const { cout << "Widget（基底版本）" << endl; }
};

class Button : public Widget {
private:
    string label_;           // 在 Widget() 執行時，這個成員還沒被建構
public:
    Button() : label_("送出") {
        cout << "  Button() 建構完成，呼叫 describe() → ";
        describe();          // 此時 vptr 已更新，叫到 Button::describe()
    }
    void describe() const override {
        // 若在 Widget() 期間叫到這裡並讀 label_，就是讀未初始化記憶體（UB）。
        // 本檔不會走到那條路徑——因為建構期間根本不會分派到這個覆寫。
        cout << "Button（衍生版本，label=" << label_ << "）" << endl;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 從缺，並說明原因
//
// 本檔主題是「物件生命週期的起點、建構期間的 virtual 分派、以及把 this
// 洩漏給外部所造成的缺陷」。這是 C++ 物件模型與資源管理的議題。
// LeetCode 判題環境只會建構一次 Solution 或 design 類別，接著呼叫其方法；
// 它既不會在建構期間回呼你的物件，也不會用 shared_ptr 接管你的物件，
// 更不會檢查你是否在建構函數中註冊了 this。指定清單中的 design 類題
// （146 LRU Cache、155 Min Stack、707 Design Linked List…）也完全不觸及此主題。
// 硬套一題只會製造「這個知識是刷題技巧」的錯誤印象。依規格「寧缺勿濫」，此處從缺。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】感測器節點註冊到事件匯流排 —— 用工廠函式取代建構函數註冊
//
// 情境：IoT 閘道器上跑著數十個 SensorNode，每個節點要向 EventBus 訂閱
//       「取樣時脈」事件。最直覺的寫法是在建構函數裡 bus.subscribe(this)，
//       這正是真實專案裡最常見的 leaking-this 缺陷：
//       匯流排可能在建構尚未完成時就派送第一個 tick，
//       此時衍生類別的成員（校正參數、緩衝區）都還沒準備好。
//
// 正確作法（本範例）：
//   1. 建構函數設為 private，任何人都無法直接 new。
//   2. 提供 static 工廠函式 create()：先完整建構，**再**訂閱。
//   3. 用 shared_ptr 管理生命週期，匯流排持有 weak_ptr，
//      避免匯流排延長節點壽命，也避免節點死後留下懸空指標。
// -----------------------------------------------------------------------------
class EventBus;

class SensorNode : public std::enable_shared_from_this<SensorNode> {
private:
    string name_;
    double calibration_;
    int    ticks_ = 0;

    // 私有建構函數：強制所有人走 create()，從型別層面杜絕「忘記兩階段初始化」
    SensorNode(string name, double calibration)
        : name_(std::move(name)), calibration_(calibration) {
        cout << "    [建構] " << name_ << " 校正值=" << calibration_ << endl;
        // 這裡刻意「不」呼叫 bus.subscribe(shared_from_this())：
        //   (1) 物件尚未完全就緒；
        //   (2) 此時內部 weak_ptr 尚未填入，shared_from_this() 會丟 bad_weak_ptr。
    }

public:
    static std::shared_ptr<SensorNode> create(EventBus& bus, string name, double calibration);

    void onTick() {
        ++ticks_;
        cout << "    [" << name_ << "] tick #" << ticks_
             << "，讀值=" << (calibration_ * ticks_) << endl;
    }

    const string& name() const { return name_; }
};

class EventBus {
private:
    vector<std::weak_ptr<SensorNode>> subs_;   // weak_ptr：不延長訂閱者壽命
public:
    void subscribe(const std::shared_ptr<SensorNode>& n) {
        cout << "    [訂閱] " << n->name() << "（此時物件已完全建構）" << endl;
        subs_.push_back(n);
    }
    void tick() {
        for (auto& w : subs_) {
            if (auto n = w.lock()) n->onTick();   // 節點已消失就自動略過
        }
    }
    size_t aliveCount() const {
        size_t c = 0;
        for (const auto& w : subs_) if (!w.expired()) ++c;
        return c;
    }
};

std::shared_ptr<SensorNode> SensorNode::create(EventBus& bus, string name, double calibration) {
    // ① 先完整建構（建構函數返回 ⇒ 生命週期正式開始）
    auto node = std::shared_ptr<SensorNode>(new SensorNode(std::move(name), calibration));
    // ② 物件被 shared_ptr 接管後，此時才可安全地把「自己」交出去
    bus.subscribe(node);
    return node;
}

int main() {
    cout << "=== 誤區一：建構中洩漏 this ===" << endl;
    Dangerous d(42);
    cout << "  建構完成，現在使用 this 才安全" << endl;
    cout << "  驗證建構期間的 this 就是 &d ？ " << boolalpha << d.isSameObject(&d) << endl;

    cout << "\n=== 建構期間的 virtual 解析（標準保證，非 UB）===" << endl;
    Button b;
    cout << "  建構完成後再呼叫 describe() → ";
    b.describe();
    cout << "  結論：建構期間多型被靜默降級為基底版本" << endl;

    cout << "\n=== 日常實務：工廠函式取代建構函數註冊 ===" << endl;
    EventBus bus;
    auto temp = SensorNode::create(bus, "temp-01", 0.5);
    auto humi = SensorNode::create(bus, "humi-01", 2.0);
    cout << "  存活訂閱者 = " << bus.aliveCount() << endl;

    cout << "  --- 第 1 次 tick ---" << endl;
    bus.tick();

    humi.reset();                       // 節點下線；weak_ptr 自動失效
    cout << "  humi-01 下線後，存活訂閱者 = " << bus.aliveCount() << endl;

    cout << "  --- 第 2 次 tick ---" << endl;
    bus.tick();

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 26 課：this 指標8.cpp" -o this8

// 注意事項（輸出相關）：
//   * 本檔完全不列印指標位址（ASLR 使其每次執行都不同），
//     改以布林值（this == &d）驗證身分，輸出因此完全決定性。
//   * 「Widget() 建構中呼叫 describe() → Widget（基底版本）」是
//     **標準保證**的結果，不是實作巧合，也不是未定義行為：
//     建構期間物件的動態型別就是當前正在建構的類別。
//   * 本檔刻意**不執行**任何真正的危險路徑（不在建構函數中註冊 this、
//     不在建構函數中呼叫 shared_from_this、不從建構函數啟動執行緒），
//     那些是未定義行為，不能拿來當示範。危害以「叫錯函式」這個
//     可安全觀察的形式呈現。
//   * SensorNode 的讀值 = 校正值 × tick 次數，全部為決定性算術；
//     temp-01 兩次 tick 得 0.5、1，humi-01 一次 tick 得 2。
//   * 0.5 與 2 以 double 印出時會顯示為 0.5 與 2（iostream 預設 6 位有效數字，
//     尾隨零不顯示）。

// === 預期輸出 ===
// === 誤區一：建構中洩漏 this ===
//   建構中... value_ = 42
//   this 已是有效位址？ true（但物件尚未完全就緒）
//   建構完成，現在使用 this 才安全
//   驗證建構期間的 this 就是 &d ？ true
//
// === 建構期間的 virtual 解析（標準保證，非 UB）===
//   Widget() 建構中，呼叫 describe() → Widget（基底版本）
//   Button() 建構完成，呼叫 describe() → Button（衍生版本，label=送出）
//   建構完成後再呼叫 describe() → Button（衍生版本，label=送出）
//   結論：建構期間多型被靜默降級為基底版本
//
// === 日常實務：工廠函式取代建構函數註冊 ===
//     [建構] temp-01 校正值=0.5
//     [訂閱] temp-01（此時物件已完全建構）
//     [建構] humi-01 校正值=2
//     [訂閱] humi-01（此時物件已完全建構）
//   存活訂閱者 = 2
//   --- 第 1 次 tick ---
//     [temp-01] tick #1，讀值=0.5
//     [humi-01] tick #1，讀值=2
//   humi-01 下線後，存活訂閱者 = 1
//   --- 第 2 次 tick ---
//     [temp-01] tick #2，讀值=1
