// =============================================================================
//  第 21 課：getter 與 setter 設計模式 3  —  危險的 getter:洩漏內部控制權
// =============================================================================
//
// 【主題資訊 Information】
//   對照組:
//     int&            getBalanceDangerous();   // 洩漏寫入權 → 封裝失效
//     int             getBalance() const;      // 只給值
//     bool deposit(int amount);                // 帶驗證與稽核的唯一修改入口
//   標準版本:C++98 起即有;本檔以 C++17 編譯。
//   複雜度:所有 getter 皆 O(1);deposit/withdraw 因寫 log 為攤銷 O(1)。
//   標頭檔:<string>、<vector>
//
// 【詳細解釋 Explanation】
//
// 【1. 什麼是類別不變式(class invariant)】
//   不變式是「這個物件在任何外界可觀察的時刻,都必須成立的性質」。
//   本檔 BankAccount 的不變式至少有三條:
//       (a) balance_ >= 0                       —— 餘額不得為負
//       (b) 每一次餘額變動,都對應 transactionLog_ 裡的一筆記錄
//       (c) log 只能追加,不得被刪改(稽核軌跡)
//   建構子負責「建立」不變式,每個 public 成員函式負責「維持」不變式。
//   deposit()/withdraw() 正是這樣寫的:先驗證,再改餘額,再補記錄。
//
// 【2. 一個非 const 參考,就讓上面三條全部失效】
//   int& getBalanceDangerous() { return balance_; }
//   這行等於對外宣告:「balance_ 的寫入權,誰要誰拿。」
//   於是 account.getBalanceDangerous() = 999999; 可以:
//       * 跳過 amount <= 0 的驗證          → 破壞 (a)
//       * 不留下任何交易記錄                → 破壞 (b)
//   而 getLogDangerous().clear() 更直接把稽核軌跡刪掉 → 破壞 (c)。
//   注意這些程式碼「完全合法、零警告、零 UB」。它不是記憶體錯誤,
//   是設計錯誤——編譯器不會救你,只有 API 設計能救你。
//
// 【3. 為什麼說「private 只擋名字,不擋資料」】
//   存取控制(access control)發生在編譯期的名稱查找階段:編譯器檢查的是
//   「你有沒有資格寫出 balance_ 這個名字」。它從不追蹤「這個參考指向哪裡」。
//   所以只要類別自己把參考交出去,存取控制就完全繞過了——這不是漏洞,
//   是標準明訂的語意。C++ 的封裝從來就是「約定 + 自律」,不是強制隔離。
//
// 【4. 正確的替代方案:命令式 API】
//   不要問「我要怎麼讓外界改 balance_」,要問「外界到底想做什麼事」。
//   答案是「存款」與「提款」,不是「設定餘額」。於是介面自然變成
//   deposit()/withdraw(),驗證與稽核就有地方掛。這就是下一支檔案
//   (第 4 號)要談的「用行為取代 setter」。
//
// 【概念補充 Concept Deep Dive】
//   * 回傳 int& 與回傳 int,在組譯層面差別是「回傳位址」vs「回傳值」。
//     前者讓呼叫端得到一個可寫的左值(lvalue),因此 f() = 999 才會合法。
//     一個函式呼叫能出現在等號左邊,正是「它回傳了非 const 左值參考」的訊號。
//   * 若把 getLogDangerous() 改回 const&,account.getLogDangerous().clear()
//     會在編譯期就失敗(clear() 不是 const 成員函式)。也就是說,const
//     正確性是有能力在編譯期擋下這類竄改的——只要你不要主動放棄它。
//   * 稽核軌跡(audit trail)在金融系統中通常另有「只可追加」的儲存層保證,
//     但在物件層級,第一道防線就是不要提供任何可刪改 log 的介面。
//
// 【注意事項 Pay Attention】
//   1. 本檔的「竄改」示範完全是良好定義(well-defined)的行為,不是 UB。
//      它示範的是封裝被破壞,不是記憶體錯誤,兩者別混為一談。
//   2. 回傳 int& 之後,呼叫端也可以長期保存這個參考;之後帳戶物件被解構,
//      該參考即懸空,再使用才會變成 UB。
//   3. 不要用「只有我們團隊會用,不會亂改」說服自己保留危險 getter——
//      六個月後的你就是那個「外部使用者」。
//   4. 真的需要批次修改內部容器時,提供 applyXxx(const std::function<>&)
//      或回傳 const& + 專用的 mutating 方法,而不是把容器整個交出去。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】封裝與不變式
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 成員是 private,為什麼還會被外部改掉?
//     答：因為存取控制只作用在「名字」上,不作用在「資料」上。
//         private 阻止外界寫出 balance_ 這個識別字,但只要類別自己回傳
//         int&,外界就拿到了指向該成員的可寫左值,存取控制完全繞過。
//         這是標準明訂的語意,不是編譯器漏洞。
//     追問：那要怎麼在編譯期擋下來?→ 回傳 const&(或回傳值)。
//         此時對它呼叫任何非 const 成員函式都會編譯失敗。
//
// 🔥 Q2. 什麼是類別不變式?誰負責建立、誰負責維持?
//     答：不變式是物件在任何外界可觀察時刻都必須成立的性質,例如
//         「餘額非負」「每筆餘額變動都有對應 log」。建構子負責建立它,
//         每個 public 成員函式負責在回傳前把它維持住。
//         private 資料 + 受控介面,就是為了把「可能破壞不變式的程式碼」
//         收斂到類別內部這一小塊,便於檢查與測試。
//     追問：成員函式執行到一半時不變式可以暫時不成立嗎?→ 可以,
//         只要在函式回傳前(以及任何可能被外界觀察到之前)恢復即可。
//
// ⚠️ 陷阱. 「getBalanceDangerous() = 999999 沒有警告也不會崩潰,那它應該還好吧?」
//     答：不好,而且問題比崩潰更嚴重。這段程式碼是完全合法、良好定義的,
//         正因為如此,編譯器、sanitizer、valgrind 全都不會報任何東西。
//         它造成的是「資料悄悄變成不合法狀態」——餘額憑空多出來、
//         稽核記錄對不上,而錯誤會在很久以後、很遠的地方才爆出來。
//     為什麼會錯：把「安全」等同於「不會 crash / 沒有 UB」。
//         記憶體安全與設計正確性是兩個獨立維度;一個沒有任何 UB 的程式,
//         照樣可以把帳算錯。封裝防的是後者。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
using namespace std;

class BankAccount {
private:
    string owner_;
    int balance_;
    vector<string> transactionLog_;

public:
    BankAccount(const string& owner, int initial)
        : owner_(owner), balance_(initial)
    {
        transactionLog_.push_back("開戶：" + to_string(initial));
    }

    // ===== 安全的 getter =====
    // 注意：這些 getter 都是 const 成員函數，保證不修改對象狀態
    int getBalance() const { return balance_; }
    const string& getOwner() const { return owner_; }
    const vector<string>& getLog() const { return transactionLog_; }

    // ===== 危險的 getter（反面教材）=====
    // 注意：這些 getter 返回非 const 引用，允許外部直接修改內部狀態，極易導致錯誤！
    // 這樣的設計完全破壞了封裝，外部可以繞過所有驗證邏輯，直接竄改內部數據！
    int& getBalanceDangerous() { return balance_; }
    vector<string>& getLogDangerous() { return transactionLog_; }

    // ===== 正確的修改介面 =====
    // 注意：這些方法帶有驗證邏輯，確保對象保持有效狀態，並且記錄所有操作
    bool deposit(int amount) {
        if (amount <= 0) return false;
        balance_ += amount;
        transactionLog_.push_back("存入：" + to_string(amount));
        return true;
    }

    bool withdraw(int amount) {
        if (amount <= 0 || amount > balance_) return false;
        balance_ -= amount;
        transactionLog_.push_back("取出：" + to_string(amount));
        return true;
    }

    void printStatement() const {
        cout << "  帳戶：" << owner_ << endl;
        cout << "  餘額：" << balance_ << endl;
        cout << "  交易記錄：" << endl;
        for (const auto& log : transactionLog_) {
            cout << "    " << log << endl;
        }
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 155. Min Stack
//   題目:設計一支堆疊,支援 push / pop / top,並能在 O(1) 取得目前最小值。
//   為什麼用到本主題:MinStack 的正確性完全建立在一條不變式上——
//       「minStack_ 的頂端,永遠等於 data_ 中所有元素的最小值」。
//   這條不變式只由 push()/pop() 成對維護。如果為了「方便」而多開一個
//       vector<int>& getData() { return data_; }
//   外界就能直接對 data_ push/pop,minStack_ 卻不會跟著更新,
//   getMin() 從此回傳錯誤答案——而且不會崩潰、不會有任何警告。
//   這正是本檔要示範的:洩漏非 const 參考 = 讓不變式失去唯一守門人。
//   複雜度:四個操作皆為 O(1);空間 O(n)。
// -----------------------------------------------------------------------------
class MinStack {
private:
    vector<int> data_;
    vector<int> minStack_;   // 與 data_ 同步:每 push 一次就記錄「至今最小值」

public:
    void push(int val) {
        data_.push_back(val);
        // 不變式維護:新的最小值 = min(舊最小值, val)
        if (minStack_.empty() || val <= minStack_.back()) {
            minStack_.push_back(val);
        } else {
            minStack_.push_back(minStack_.back());
        }
    }

    void pop() {
        if (data_.empty()) return;
        data_.pop_back();
        minStack_.pop_back();   // 必須成對彈出，否則不變式立刻壞掉
    }

    int top()    const { return data_.back(); }
    int getMin() const { return minStack_.back(); }

    // 只提供唯讀視圖:外界看得到大小，但拿不到寫入權
    size_t size() const { return data_.size(); }

    // 反面示範(刻意保留為註解):
    // vector<int>& getData() { return data_; }
    // ↑ 一旦提供，外界 getData().push_back(x) 會讓 minStack_ 失去同步，
    //   getMin() 開始回傳錯誤答案，卻沒有任何編譯錯誤或執行期警告。
};

// -----------------------------------------------------------------------------
// 【日常實務範例】會議室座位預訂:守住 available_ + reserved_ == total_
//   情境:訂位系統最常見的 bug 就是「總數對不上」——賣超或憑空少票。
//   這裡的不變式非常具體,而且可以在每次操作後實際驗證。
//   reserve()/cancel() 是唯一入口,兩者都先驗證再改狀態。
//   若改成回傳 int& 讓外界直接改 available_,總數立刻對不上。
// -----------------------------------------------------------------------------
class SeatBooking {
private:
    string roomName_;
    int total_;
    int reserved_;

public:
    SeatBooking(const string& room, int total)
        : roomName_(room), total_(total), reserved_(0) {}

    bool reserve(int n) {
        if (n <= 0 || reserved_ + n > total_) return false;   // 擋下賣超
        reserved_ += n;
        return true;
    }

    bool cancel(int n) {
        if (n <= 0 || n > reserved_) return false;            // 擋下退超
        reserved_ -= n;
        return true;
    }

    int  total()     const { return total_; }
    int  reserved()  const { return reserved_; }
    int  available() const { return total_ - reserved_; }
    const string& room() const { return roomName_; }

    // 不變式自我檢查:任何時刻都必須成立
    bool invariantHolds() const {
        return reserved_ >= 0 && reserved_ <= total_
            && available() + reserved_ == total_;
    }
};

int main() {
    cout << "=== 危險的 getter 示範 ===" << endl;

    BankAccount account("陳信安", 1000);

    // 正常操作
    cout << "\n--- 正常操作 ---" << endl;
    account.deposit(500);
    account.withdraw(200);
    account.printStatement();

    // 使用危險的 getter 繞過所有保護！
    // 注意：這樣做會直接修改內部狀態，完全繞過 deposit() 和 withdraw() 的驗證邏輯，極易導致錯誤！
    cout << "\n--- 使用危險的 getter ---" << endl;

    // 直接修改餘額，沒有驗證，沒有記錄！
    // 注意：這樣做會直接修改內部狀態，完全繞過 deposit() 和 withdraw() 的驗證邏輯，極易導致錯誤！
    account.getBalanceDangerous() = 999999;
    cout << "  餘額被直接竄改為：" << account.getBalance() << endl;

    // 直接竄改交易記錄！
    // 注意：這樣做會直接修改內部狀態，完全繞過所有驗證邏輯，極易導致錯誤！
    account.getLogDangerous().clear();
    account.getLogDangerous().push_back("一切正常，沒有異常");

    cout << "\n--- 竄改後的帳戶 ---" << endl;
    account.printStatement();
    cout << "  ↑ 餘額憑空多出來，交易記錄卻對不上：不變式已被破壞" << endl;

    // ─────────────────────────────────────────────────────────
    cout << "\n=== LeetCode 155. Min Stack（不變式由 push/pop 成對維護）===" << endl;
    MinStack ms;
    ms.push(-2);
    ms.push(0);
    ms.push(-3);
    cout << "  push(-2), push(0), push(-3)" << endl;
    cout << "  getMin() = " << ms.getMin() << "   （預期 -3）" << endl;
    ms.pop();
    cout << "  pop() 之後" << endl;
    cout << "  top()    = " << ms.top()    << "   （預期 0）" << endl;
    cout << "  getMin() = " << ms.getMin() << "   （預期 -2）" << endl;
    cout << "  目前元素數：" << ms.size() << endl;

    // ─────────────────────────────────────────────────────────
    cout << "\n=== 日常實務：會議室座位預訂（守住總數不變式）===" << endl;
    SeatBooking room("101 會議室", 20);
    cout << "  " << room.room() << " 總座位：" << room.total() << endl;

    cout << "  預訂 8 位：" << (room.reserve(8)  ? "成功" : "失敗") << endl;
    cout << "  預訂 15 位：" << (room.reserve(15) ? "成功" : "失敗")
         << "（會超賣，應被擋下）" << endl;
    cout << "  取消 30 位：" << (room.cancel(30)  ? "成功" : "失敗")
         << "（退超，應被擋下）" << endl;
    cout << "  取消 3 位：" << (room.cancel(3)   ? "成功" : "失敗") << endl;

    cout << "  已預訂：" << room.reserved()
         << "  剩餘：" << room.available()
         << "  總數：" << room.total() << endl;
    cout << "  不變式成立？" << (room.invariantHolds() ? "是" : "否") << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 21 課：getter 與 setter 設計模式3.cpp" -o l21_3
// 執行: ./l21_3        (rc=0)

// === 預期輸出 ===
// === 危險的 getter 示範 ===
//
// --- 正常操作 ---
//   帳戶：陳信安
//   餘額：1300
//   交易記錄：
//     開戶：1000
//     存入：500
//     取出：200
//
// --- 使用危險的 getter ---
//   餘額被直接竄改為：999999
//
// --- 竄改後的帳戶 ---
//   帳戶：陳信安
//   餘額：999999
//   交易記錄：
//     一切正常，沒有異常
//   ↑ 餘額憑空多出來，交易記錄卻對不上：不變式已被破壞
//
// === LeetCode 155. Min Stack（不變式由 push/pop 成對維護）===
//   push(-2), push(0), push(-3)
//   getMin() = -3   （預期 -3）
//   pop() 之後
//   top()    = 0   （預期 0）
//   getMin() = -2   （預期 -2）
//   目前元素數：2
//
// === 日常實務：會議室座位預訂（守住總數不變式）===
//   101 會議室 總座位：20
//   預訂 8 位：成功
//   預訂 15 位：失敗（會超賣，應被擋下）
//   取消 30 位：失敗（退超，應被擋下）
//   取消 3 位：成功
//   已預訂：5  剩餘：15  總數：20
//   不變式成立？是
