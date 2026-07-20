// =============================================================================
//  第 26 課：this 指標 4  —  把自己交出去（傳遞 this 與 *this）
// =============================================================================
//
// 【主題資訊 Information】
//   傳指標:  f(this);     // 型別 C*，可為 null 的語意、可被儲存
//   傳參考:  f(*this);    // 型別 C&，不可為 null，語意上「一定有東西」
//   傳 const 參考: 只讀用途，f(const C&)
//   標準版本: C++98
//   標頭檔: <string>
//
// 【詳細解釋 Explanation】
//
// 【1. this 與 *this 的選擇，其實是介面設計問題】
//   兩者指的是同一個物件，差別在「你想給對方什麼承諾」：
//     * 傳 this（指標）：對方可以存起來、可以檢查是否為 null、
//       也隱含「這個物件可能不存在」的語意。註冊到某個管理器通常用這種。
//     * 傳 *this（參考）：對方不能存 null，語意是「這裡一定有一個物件」。
//       只是暫時借用（呼叫期間讀一讀）通常用這種。
//   本檔 registerPlayer(this) 用指標（系統要長期持有），
//   logAction(*this, ...) 用 const 參考（只在呼叫期間讀取），
//   這個分工正是實務上的慣例。
//
// 【2. 在建構子裡傳 this 出去 —— 本檔最重要的細節】
//   Player 的建構子在「函式體」裡呼叫 registerPlayer(this)。
//   這能運作是因為：成員初始化列表（name_、hp_）在函式體之前就已完成，
//   所以 registerPlayer 讀 p->getName() 讀到的是有效的資料。
//   但這個寫法在三種情況下會出事：
//     (a) 若在「初始化列表」階段就把 this 傳出去，
//         此時成員尚未初始化，對方讀到的是未初始化的資料。
//     (b) 若 Player 有衍生類別，基底建構子執行時衍生部分還不存在 ——
//         此時透過 this 呼叫 virtual 函式不會分派到衍生版本
//         （建構期間物件的動態型別就是當前正在建構的那個類別）。
//     (c) 若對方把指標存起來，而 Player 之後被銷毀，
//         那個指標就成了懸空指標 —— 生命期必須有人負責。
//   本檔沒有解構時的反註冊（unregisterPlayer），
//   這在真實系統裡就是一個資源洩漏／懸空指標的來源。
//
// 【3. 為什麼 attack 收 Player&，而 logAction 收 const Player&】
//   attack 要修改對方（扣血），所以收非 const 參考；
//   logAction 只讀取，所以收 const 參考。
//   這個 const 標註不只是禮貌 —— 它讓編譯器幫你保證
//   「記錄日誌不會改到玩家狀態」，是文件也是檢查。
//
// 【概念補充 Concept Deep Dive】
//   * 前向宣告（class Player;）讓 registerPlayer 的宣告可以先寫在類別之前。
//     但只有宣告不夠：函式的「定義」裡用到 p->getName()，
//     必須等 Player 完整定義之後才能寫，所以本檔把實作放在類別後面。
//     這是標頭檔／實作檔分離的縮影。
//   * *this 的型別在 const 成員函式中是 const C&，
//     所以 const 成員函式無法把 *this 傳給收 C& 的函式 —— const 會被正確傳播。
//   * 傳 this 給外部並被長期持有時，現代作法是用 std::shared_ptr +
//     std::enable_shared_from_this，讓對方拿到的是能延長生命期的
//     shared_ptr 而非裸指標。直接傳 this 給非同步 callback 是常見的崩潰來源。
//   * 對已結束生命期的物件的指標，連「比較」都是實作定義的行為
//     （[basic.stc]），所以懸空指標不該被用來做任何事，包括判斷相等。
//
// 【注意事項 Pay Attention】
//   1. 在建構子的初始化列表階段傳 this 出去，成員尚未初始化。
//   2. 建構／解構期間透過 this 呼叫 virtual 函式，不會分派到衍生類別。
//   3. 對方若長期持有裸 this，物件銷毀後就是懸空指標 —— 需要反註冊機制。
//   4. 非同步／跨執行緒的 callback 不要傳裸 this，用 shared_from_this。
//   5. const 成員函式中 *this 是 const C&，無法傳給需要修改的介面。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】傳遞 this 與 *this
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼時候傳 this、什麼時候傳 *this?
//     答：兩者指同一個物件，差別在傳達的語意與對方能做什麼。
//         傳 this（C*）：對方可以儲存、可以檢查 null，
//         適合「需要長期持有」的註冊類介面。
//         傳 *this（C&）：不可能是 null，語意是「一定有一個物件」，
//         適合「只在呼叫期間借用」的場合。只讀時再加 const。
//     追問：那 const 成員函式裡傳 *this 會怎樣?
//         → 型別是 const C&，只能傳給接受 const 參考的函式。
//         const 會正確傳播，這正是我們要的。
//
// 🔥 Q2. 在建構子裡把 this 傳給外部系統，有什麼風險?
//     答：三個。第一，若在成員初始化列表階段就傳出去，
//         成員還沒初始化，對方讀到的是未初始化的資料。
//         第二，若有衍生類別，基底建構子執行時衍生部分尚未存在，
//         此時透過 this 呼叫 virtual 函式不會分派到衍生版本。
//         第三，對方若把裸指標存起來，物件銷毀後就成了懸空指標。
//     追問：本檔的寫法安全嗎?
//         → 就「讀取成員」而言是安全的 ——
//         registerPlayer 在建構子「函式體」被呼叫，
//         而成員初始化列表已經先完成了。
//         但它沒有解構時的反註冊，所以第三個風險仍然存在。
//
// ⚠️ 陷阱. 「在建構子裡呼叫 virtual 函式，會跑到衍生類別覆寫的版本 ——
//            畢竟 this 指的就是那個衍生物件。」
//     答：不會。建構期間，物件的「動態型別」就是當前正在建構的那個類別。
//         基底類別的建構子執行時，衍生類別的部分尚未初始化，
//         所以標準規定此時的虛擬分派只會找到基底自己的版本。
//         這不是編譯器沒最佳化，而是刻意如此 ——
//         否則就會呼叫到「操作尚未初始化資料」的衍生函式。
//         解構期間同理，順序反過來。
//     為什麼會錯：把 this 的「靜態型別」與「動態型別」混為一談，
//         並且假設物件從建構子第一行開始就已經是完整的衍生物件。
//         實際上物件是「由內而外」逐層建構的，
//         在基底建構子執行的那一刻，衍生的那一層還不存在。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 從缺，理由如下
//   「把自己註冊到外部系統」是物件生命期與所有權的設計議題，
//   LeetCode 判題只驗演算法輸入輸出，不涉及跨物件的持有關係。
//   本檔改以「觀察者模式的註冊／反註冊」實務範例，
//   補上本檔原本缺少的那一半 —— 解構時的反註冊。
//
// =============================================================================

#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Player;  // 前向聲明

// 外部系統：接收 Player 的引用或指標
void registerPlayer(Player* p);
void logAction(const Player& p, const string& action);

class Player {
private:
    string name_;
    int hp_;

public:
    Player(const string& name, int hp)
        : name_(name), hp_(hp)
    {
        // 在建構時把自身註冊到系統
        registerPlayer(this);     // 傳遞 this 指標
    }

    void attack(Player& target) {
        // 把自身的引用傳給日誌系統
        logAction(*this, "攻擊了 " + target.getName());
        target.takeDamage(20);
    }

    void takeDamage(int dmg) {
        hp_ -= dmg;
        logAction(*this, "受到 " + to_string(dmg) + " 傷害");
    }

    const string& getName() const { return name_; }
    int getHp() const { return hp_; }
};

// 外部函數的實現
// 註：刻意不印出 p 的數值位址 —— 位址每次執行都不同（ASLR），
//     無法當成穩定的預期輸出。這裡改印「已收到非空指標」，
//     那才是註冊這個動作真正在乎的事。
void registerPlayer(Player* p) {
    cout << "  [系統] 註冊玩家：" << p->getName()
         << " (收到有效指標:" << (p != nullptr ? "是" : "否") << ")" << endl;
}

void logAction(const Player& p, const string& action) {
    cout << "  [日誌] " << p.getName() << " " << action
         << " (HP:" << p.getHp() << ")" << endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】觀察者模式：註冊 + 反註冊（補上本檔缺少的那一半）
//   情境：多個 UI 面板都要在「溫度變化」時更新自己。
//         每個面板在建構時把 this 註冊進事件中心，
//         在解構時把自己移除 —— 這個「移除」正是上面的 Player 沒做的事。
//   為什麼重要：事件中心持有的是裸指標。
//         若面板被銷毀卻沒有反註冊，事件中心下次發送通知時
//         就會透過懸空指標呼叫成員函式。
//   本範例刻意讓一個面板先離開作用域，用計數證明它確實被移除了。
// -----------------------------------------------------------------------------
class TemperatureSensor;

class Observer {
public:
    virtual ~Observer() = default;
    virtual void onTemperature(int celsius) = 0;
    virtual string name() const = 0;
};

class TemperatureSensor {
private:
    vector<Observer*> observers_;     // 持有裸指標，生命期由對方負責

public:
    void subscribe(Observer* o) {
        observers_.push_back(o);
        cout << "    [事件中心] " << o->name()
             << " 已訂閱（目前訂閱者 " << observers_.size() << " 個）" << endl;
    }

    // 反註冊：物件銷毀前一定要呼叫，否則就留下懸空指標
    void unsubscribe(Observer* o) {
        for (size_t i = 0; i < observers_.size(); ++i) {
            if (observers_[i] == o) {          // 比較同一性，不是比較值
                observers_.erase(observers_.begin() + static_cast<long>(i));
                cout << "    [事件中心] " << o->name()
                     << " 已取消訂閱（剩餘 " << observers_.size() << " 個）" << endl;
                return;
            }
        }
    }

    void publish(int celsius) {
        cout << "    [事件中心] 發布溫度 " << celsius
             << "°C 給 " << observers_.size() << " 個訂閱者" << endl;
        for (Observer* o : observers_) o->onTemperature(celsius);
    }
};

class DashboardPanel : public Observer {
private:
    string   title_;
    TemperatureSensor* sensor_;     // 記住要向誰反註冊

public:
    // 建構子：把 this 註冊出去。
    // 安全的前提是成員初始化列表已完成 —— title_ 此時已可讀取。
    DashboardPanel(const string& title, TemperatureSensor* sensor)
        : title_(title), sensor_(sensor) {
        sensor_->subscribe(this);
    }

    // 解構子：務必反註冊，否則事件中心會留下懸空指標
    ~DashboardPanel() override {
        sensor_->unsubscribe(this);
    }

    void onTemperature(int celsius) override {
        cout << "      [" << title_ << "] 更新顯示：" << celsius << "°C" << endl;
    }

    string name() const override { return title_; }
};

int main() {
    cout << "=== 場景三：傳遞 this ===" << endl;

    cout << "\n--- 創建玩家 ---" << endl;
    Player warrior("戰士", 200);
    Player mage("法師", 120);

    cout << "\n--- 戰鬥 ---" << endl;
    warrior.attack(mage);
    mage.attack(warrior);

    cout << "\n  註：registerPlayer(this) 寫在建構子的「函式體」，" << endl;
    cout << "      此時成員初始化列表已完成，所以讀 name_ 是安全的。" << endl;
    cout << "      但本檔沒有解構時的反註冊 —— 系統會留著已銷毀物件的指標。" << endl;
    cout << "      下面的觀察者範例補上這一半。" << endl;

    cout << "\n=== 日常實務：觀察者的註冊與反註冊 ===" << endl;
    {
        TemperatureSensor sensor;

        cout << "  建立兩個常駐面板：" << endl;
        DashboardPanel main_("主控台", &sensor);
        DashboardPanel chart("趨勢圖", &sensor);

        cout << "  第一次發布：" << endl;
        sensor.publish(26);

        {
            cout << "  建立一個暫時的彈出視窗（離開作用域就會銷毀）：" << endl;
            DashboardPanel popup("彈出視窗", &sensor);

            cout << "  第二次發布（三個訂閱者都會收到）：" << endl;
            sensor.publish(31);

            cout << "  彈出視窗即將離開作用域..." << endl;
        }
        // popup 已解構，且它的解構子做了反註冊

        cout << "  第三次發布（彈出視窗已反註冊，只剩兩個）：" << endl;
        sensor.publish(28);

        cout << "  ↑ 若 DashboardPanel 的解構子沒有呼叫 unsubscribe，" << endl;
        cout << "    事件中心會留著已銷毀物件的指標，" << endl;
        cout << "    第三次發布就會透過懸空指標呼叫成員函式。" << endl;
        cout << "    這正是「把 this 交出去」必須配套的責任。" << endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 26 課：this 指標4.cpp -o this_ptr4

// === 預期輸出 ===
// === 場景三：傳遞 this ===
//
// --- 創建玩家 ---
//   [系統] 註冊玩家：戰士 (收到有效指標:是)
//   [系統] 註冊玩家：法師 (收到有效指標:是)
//
// --- 戰鬥 ---
//   [日誌] 戰士 攻擊了 法師 (HP:200)
//   [日誌] 法師 受到 20 傷害 (HP:100)
//   [日誌] 法師 攻擊了 戰士 (HP:100)
//   [日誌] 戰士 受到 20 傷害 (HP:180)
//
//   註：registerPlayer(this) 寫在建構子的「函式體」，
//       此時成員初始化列表已完成，所以讀 name_ 是安全的。
//       但本檔沒有解構時的反註冊 —— 系統會留著已銷毀物件的指標。
//       下面的觀察者範例補上這一半。
//
// === 日常實務：觀察者的註冊與反註冊 ===
//   建立兩個常駐面板：
//     [事件中心] 主控台 已訂閱（目前訂閱者 1 個）
//     [事件中心] 趨勢圖 已訂閱（目前訂閱者 2 個）
//   第一次發布：
//     [事件中心] 發布溫度 26°C 給 2 個訂閱者
//       [主控台] 更新顯示：26°C
//       [趨勢圖] 更新顯示：26°C
//   建立一個暫時的彈出視窗（離開作用域就會銷毀）：
//     [事件中心] 彈出視窗 已訂閱（目前訂閱者 3 個）
//   第二次發布（三個訂閱者都會收到）：
//     [事件中心] 發布溫度 31°C 給 3 個訂閱者
//       [主控台] 更新顯示：31°C
//       [趨勢圖] 更新顯示：31°C
//       [彈出視窗] 更新顯示：31°C
//   彈出視窗即將離開作用域...
//     [事件中心] 彈出視窗 已取消訂閱（剩餘 2 個）
//   第三次發布（彈出視窗已反註冊，只剩兩個）：
//     [事件中心] 發布溫度 28°C 給 2 個訂閱者
//       [主控台] 更新顯示：28°C
//       [趨勢圖] 更新顯示：28°C
//   ↑ 若 DashboardPanel 的解構子沒有呼叫 unsubscribe，
//     事件中心會留著已銷毀物件的指標，
//     第三次發布就會透過懸空指標呼叫成員函式。
//     這正是「把 this 交出去」必須配套的責任。
//     [事件中心] 趨勢圖 已取消訂閱（剩餘 1 個）
//     [事件中心] 主控台 已取消訂閱（剩餘 0 個）
