// =============================================================================
//  第 21 課 範例 1  —  getter 與 setter：不是樣板程式碼，是不變量的守門員
// =============================================================================
//
// 【主題資訊 Information】
//   語法    ：T getX() const;              // 讀取，通常標 const
//             const T& getX() const;       // 大型物件用 const 參考，避免複製
//             void setX(T value);          // 寫入，內含驗證邏輯
//   標準版本：C++98 起；本檔以 C++17 編譯
//   標頭檔  ：<iostream>、<string>
//   複雜度  ：存取本身 O(1)；標頭檔中定義的短函式通常會被 inline，零額外成本
//
// 【詳細解釋 Explanation】
//
// 【1. getter/setter 的真正目的不是「隱藏變數」】
//   很多人把 getter/setter 學成一種儀式：
//   「成員設成 private，然後每個都配一組 get/set」——
//   如果 setter 只是無條件地 `x_ = value;`，那和把成員設成 public
//   在效果上幾乎沒有差別，只是多打了很多字。
//   它們真正的價值在於：**成為維護「類別不變量」的唯一入口**。
//   不變量（invariant）是「這個物件在任何時刻都必須成立的性質」，
//   例如本檔的 Player：
//       0 <= hp_ <= maxHp_        且      name_ 不可為空
//   只要所有的修改都必須經過 setHp() / setName()，
//   這兩條規則就不可能被破壞 —— 這是 public 成員永遠做不到的。
//
// 【2. 本檔 setHp 的關鍵設計：修正而非拒絕】
//   setHp(-500) 不會讓 hp_ 變成 -500，也不會丟例外，
//   而是把它夾到合法範圍（clamp 成 0）。
//   setHp(99999) 同理夾成 maxHp_。
//   這是遊戲數值最常見的處理策略：呼叫端不必每次都先檢查上下限，
//   類別自己保證結果一定合法。
//   （另一種策略是「拒絕並回報錯誤」，本檔的 setName 走的就是這條路：
//    空字串直接拒絕、不修改。兩種都對，取決於哪一種對呼叫端比較好用。
//    重點是「必須擇一，不能讓非法值進來」。）
//
// 【3. 最重要的設計決定，其實是「不提供 setter」】
//   注意 level_ 只有 getLevel()，沒有 setLevel()。
//   這不是漏寫，而是刻意的設計：等級應該只能透過遊戲邏輯
//   （打怪獲得經驗、完成任務）提升，不該讓任何人直接指定。
//   `hero.setLevel(99);` 會直接編譯失敗 —— 在編譯期就擋下作弊與誤用。
//   這帶出一條重要原則：
//       **不要反射式地替每個成員都配一組 getter/setter。**
//       先問「外界真的需要讀嗎？真的需要寫嗎？」
//       多數欄位的答案是「可以讀，但不該任意寫」。
//
// 【4. getter 的兩個技術細節】
//   (a) 一律標 const：`int getHp() const`。
//       沒標 const 的話，const Player& 就無法呼叫它 ——
//       這會讓你的類別無法用在任何以 const 參考傳遞的場合。
//   (b) 回傳型別看大小：
//       基本型別（int）直接回傳值，複製成本比參考還低；
//       大型物件（string、vector）回傳 const 參考，避免每次呼叫都深複製。
//       注意一定要是 **const** 參考 —— 回傳非 const 參考等於把內部
//       成員的寫入權整個交出去，封裝當場破功（見下方陷阱題）。
//
// 【概念補充 Concept Deep Dive】
//   getter/setter 會不會有效能成本？在標頭檔中定義的短函式，
//   編譯器幾乎必定會 inline，最終產生的機器碼與直接存取成員相同 ——
//   所以封裝在這裡是「零成本抽象」。
//   真正的成本不在執行期，而在「介面一旦公開就難以更動」：
//   getHp() 回傳 int 這件事會被所有呼叫端依賴，
//   日後想改成浮點血量或加上暴擊減免，就得同時處理所有呼叫端。
//   這也是為什麼「不要無腦替每個成員開 getter/setter」——
//   每開一個，就等於多簽了一份長期契約。
//
//   另外值得一提：回傳 const 參考時要注意懸空風險。
//   `const string& n = makeTempPlayer().getName();`
//   —— 暫時 Player 在該完整運算式結束時就銷毀了，n 隨即懸空。
//   回傳 const 參考的 getter 本身沒錯，
//   但呼叫端若用參考接住「暫時物件的成員」就會出事；
//   用 `string n = ...` 以值接住即可避免。
//
// 【注意事項 Pay Attention】
// 1. getter 一律加 const，否則 const 物件無法呼叫。
// 2. 大型成員回傳 `const T&`；**絕對不要**回傳非 const 參考或成員指標，
//    那等於把封裝拆掉（呼叫端可以繞過所有驗證直接改）。
// 3. 只寫「無驗證的 set」等於公開成員，卻多付了維護介面的代價。
//    真的不需要驗證時，反而該考慮直接用 public 成員（或 struct）。
// 4. setter 的兩種策略（夾到合法範圍 vs 拒絕並回報）要在同一個類別內
//    保持一致的風格，否則呼叫端難以預期行為。
// 5. 本檔使用 `using namespace std;`，教學檔可接受，
//    標頭檔與大型專案應避免。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】getter 與 setter
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 如果 setter 裡面只有一行 `x_ = value;`，那它和 public 成員有什麼差別？
//     答：以「當下的行為」而言幾乎沒有差別，這也是很多人覺得 getter/setter
//         是樣板廢話的原因。它的價值在兩個地方：
//         (a) 保留了「日後加入驗證、記錄、通知」的位置，而不必改動呼叫端；
//         (b) 讓「讀」與「寫」的權限可以分開授予
//             （例如本檔的 level_ 只給讀、不給寫）。
//         但反過來說也成立：如果一個型別根本沒有不變量要維護
//         （例如單純的 Point{x, y}），那就該老實用 public 成員或 struct，
//         硬包一層 get/set 只是徒增噪音。
//     追問：那要怎麼判斷該不該封裝？
//         → 問「這個型別有沒有不變量」。有（hp 不能超過 maxHp、
//           名字不能為空）→ 封裝並由 setter 守門；
//           沒有（幾個彼此獨立、任意組合都合法的欄位）→ 直接開放。
//
// 🔥 Q2. getter 為什麼要加 const？回傳型別該用值還是參考？
//     答：加 const 是因為不加的話，`const Player&` 就無法呼叫這個 getter，
//         你的類別會無法用在任何以 const 傳遞的場合。
//         回傳型別看成員大小：基本型別（int）直接回傳值最快；
//         大型物件（string、vector）回傳 const 參考避免深複製。
//     追問：回傳 const 參考有沒有風險？
//         → 有，懸空參考。若呼叫端寫
//           `const string& n = makeTempPlayer().getName();`，
//           暫時 Player 在該完整運算式結束就銷毀，n 立刻懸空。
//           這不是 getter 的錯，但用值接住（`string n = ...`）就能避免。
//
// ⚠️ 陷阱. 「我的成員都是 private，而且全部只透過 getter 存取，
//           所以我的封裝是完整的。」
//     答：不一定。只要有任何一個 getter 回傳了「非 const 的參考或指標」，
//         封裝就已經破了。例如：
//             string& getName() { return name_; }        // ← 非 const 參考
//         呼叫端可以寫 `hero.getName() = "";`，
//         直接把 name_ 改成空字串，完全繞過 setName() 的驗證。
//         同理，回傳 `int*`、回傳內部容器的非 const 參考、
//         或回傳 .data() 指標，都會把寫入權洩漏出去。
//     為什麼會錯：把「成員是 private」當成封裝的充分條件。
//         實際上 private 只擋住了「直接用名字存取」，
//         擋不住「你自己親手把內部的參考遞出去」。
//         正確的判準是：檢查每一個 public 函式的回傳型別，
//         問「呼叫端拿到這個回傳值之後，能不能繞過驗證改到內部狀態？」
//         能 → 封裝已破。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
using namespace std;

class Player {
private:
    string name_;
    int hp_;
    int maxHp_;
    int level_;

public:
    Player(const string& name, int maxHp)
        : name_(name), hp_(maxHp), maxHp_(maxHp), level_(1)
    {
    }

    // ====== Getter：返回數據的只讀訪問 ======
    // 注意：getter 通常是 const 成員函數，保證不修改對象狀態

    // 基本型別的 getter：返回值的拷貝
    int getHp() const { return hp_; }
    int getMaxHp() const { return maxHp_; }
    int getLevel() const { return level_; }

    // 字串的 getter：返回 const 引用（避免拷貝）
    const string& getName() const { return name_; }

    // ====== Setter：帶驗證的修改 ======
    // setter 允許修改內部狀態，但通常會帶有驗證邏輯，確保對象保持有效狀態

    void setHp(int newHp) {
        // 驗證：維護不變量
        if (newHp < 0) newHp = 0;
        if (newHp > maxHp_) newHp = maxHp_;
        hp_ = newHp;
    }

    void setName(const string& newName) {
        if (newName.empty()) {
            cout << "  名字不能為空！" << endl;
            return;
        }
        name_ = newName;
    }

    // 注意：level_ 沒有 setter！
    // 等級只能通過 gainExp() 等遊戲邏輯改變，不開放直接設定

    void printStatus() const {
        cout << "  " << name_ << " Lv." << level_
             << " HP:" << hp_ << "/" << maxHp_ << endl;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1603. Design Parking System
//   題目：設計一個停車系統，建構時給定大、中、小三種車位的數量。
//         addCar(carType) 嘗試停入一輛車（1=大 2=中 3=小），
//         成功回傳 true 並佔用一個車位，沒有空位則回傳 false。
//   為什麼用到本主題：這題就是本課的直接應用 ——
//         剩餘車位數是「必須被保護的內部狀態」，其不變量是
//         「任何時刻剩餘車位數都不得小於 0」。
//         若把計數器開成 public 成員，呼叫端可以隨手寫成負數而破壞不變量；
//         把它設為 private、只透過 addCar() 這個帶驗證的修改入口操作，
//         不變量就不可能被破壞。這正是「setter 是守門員」的具體體現。
//         注意本題也示範了「不是每個成員都要有 setter」：
//         外界只需要 addCar()，完全不需要能直接指定剩餘車位數。
//   複雜度：addCar 為 O(1)，空間 O(1)
// -----------------------------------------------------------------------------
class ParkingSystem {
private:
    int slots_[4];   // 索引 1=大, 2=中, 3=小（索引 0 不用，避免 -1 位移）

public:
    ParkingSystem(int big, int medium, int small) : slots_{0, big, medium, small} {}

    // 帶驗證的修改入口：沒有空位就拒絕，保證 slots_ 永遠不會變成負數
    bool addCar(int carType) {
        if (carType < 1 || carType > 3) return false;   // 防禦非法輸入
        if (slots_[carType] <= 0) return false;         // 守住不變量
        --slots_[carType];
        return true;
    }

    // 只讀的 getter：外界可以查詢剩餘車位，但不能直接指定
    int remaining(int carType) const {
        if (carType < 1 || carType > 3) return 0;
        return slots_[carType];
    }
};

int main() {
    cout << "=== Getter 與 Setter 基本用法 ===" << endl;

    Player hero("勇者", 100);
    hero.printStatus();

    // 使用 getter 讀取
    cout << "\n--- Getter ---" << endl;
    cout << "  名字：" << hero.getName() << endl;
    cout << "  HP：" << hero.getHp() << endl;
    cout << "  等級：" << hero.getLevel() << endl;

    // 使用 setter 修改（帶驗證）
    cout << "\n--- Setter（正常）---" << endl;
    hero.setHp(60);
    hero.printStatus();

    // setter 的保護作用
    cout << "\n--- Setter（異常值被攔截）---" << endl;
    hero.setHp(-500);       // 會被修正為 0
    hero.printStatus();

    hero.setHp(99999);      // 會被修正為 maxHp
    hero.printStatus();

    hero.setName("");        // 空名字被拒絕
    hero.setName("英雄");
    hero.printStatus();

    // hero.setLevel(99);   // 編譯錯誤！沒有 setLevel
    cout << "\n--- 沒有 setter 的欄位（等級）---" << endl;
    cout << "  hero.setLevel(99); 會編譯失敗 —— 等級只能由遊戲邏輯提升" << endl;
    cout << "  目前等級（唯讀）：" << hero.getLevel() << endl;

    cout << "\n=== LeetCode 1603. Design Parking System ===" << endl;
    {
        ParkingSystem lot(1, 1, 0);   // 大 1、中 1、小 0
        cout << "  初始車位  大:" << lot.remaining(1)
             << " 中:" << lot.remaining(2)
             << " 小:" << lot.remaining(3) << endl;
        cout << "  addCar(1) 大型車 -> " << (lot.addCar(1) ? "true" : "false") << endl;
        cout << "  addCar(2) 中型車 -> " << (lot.addCar(2) ? "true" : "false") << endl;
        cout << "  addCar(3) 小型車 -> " << (lot.addCar(3) ? "true" : "false")
             << "（小車位本來就是 0）" << endl;
        cout << "  addCar(1) 再一輛 -> " << (lot.addCar(1) ? "true" : "false")
             << "（大車位已滿）" << endl;
        cout << "  剩餘車位  大:" << lot.remaining(1)
             << " 中:" << lot.remaining(2)
             << " 小:" << lot.remaining(3) << endl;
        cout << "  → 計數器全程未曾變成負數：不變量由 addCar() 這個唯一入口守住" << endl;
    }

    cout << "\n=== 日常實務：帳戶餘額不可為負 ===" << endl;
    {
        // 與 Player::setHp 相同的守門邏輯，換到金融情境
        class Account {
            long long balanceCents_ = 0;      // 以「分」為單位，避免浮點誤差
        public:
            long long balance() const { return balanceCents_; }
            bool withdraw(long long cents) {   // 帶驗證的修改入口
                if (cents <= 0) return false;               // 拒絕非法金額
                if (cents > balanceCents_) return false;    // 守住「餘額不可為負」
                balanceCents_ -= cents;
                return true;
            }
            void deposit(long long cents) {
                if (cents > 0) balanceCents_ += cents;
            }
        };

        Account acc;
        acc.deposit(10000);                       // 存入 100.00
        cout << "  存入後餘額（分）：" << acc.balance() << endl;
        cout << "  提領 30.00 -> " << (acc.withdraw(3000) ? "成功" : "失敗")
             << "，餘額 " << acc.balance() << endl;
        cout << "  提領 999.00 -> " << (acc.withdraw(99900) ? "成功" : "失敗")
             << "（超過餘額，被攔截），餘額 " << acc.balance() << endl;
        cout << "  提領 -50 -> " << (acc.withdraw(-5000) ? "成功" : "失敗")
             << "（負數金額，被攔截），餘額 " << acc.balance() << endl;
        cout << "  → 餘額欄位沒有 setter：外界只能透過 deposit/withdraw 改變它" << endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 21 課：getter 與 setter 設計模式1.cpp" -o getset1

// ── 輸出但書 ────────────────────────────────────────────────────────────
// 1. 本檔輸出完全由程式邏輯決定，逐位元組可重現（實測連跑 5 次 md5 相同）。
// 2. 判讀重點：setHp(-500) 之後 HP 顯示 0、setHp(99999) 之後顯示 100，
//    證明非法值被夾到合法範圍，不變量 0 <= hp_ <= maxHp_ 全程成立；
//    setName("") 被拒絕（印出「名字不能為空！」且名字未變）。
// 3. 文中「hero.setLevel(99); 會編譯失敗」已實測驗證，不是推測：
//    取消該行註解後，GCC 15.2.0 回報
//      error: 'class Player' has no member named 'setLevel';
//             did you mean 'getLevel'?
//    這正是「只給讀、不給寫」在編譯期就生效的證據。
// 4. LeetCode 1603 這一段的輸出與官方範例一致：
//    ParkingSystem(1, 1, 0) 依序 addCar(1)/addCar(2)/addCar(3)/addCar(1)
//    的預期結果為 true / true / false / false。
// 5. 帳戶範例以「分」為整數單位計算，刻意避開浮點誤差；
//    金融金額不應使用 double 表示。
// 6. 本機環境：GCC 15.2.0 (Ubuntu 15.2.0-16ubuntu1) / libstdc++ / x86-64。

// === 預期輸出 ===
// === Getter 與 Setter 基本用法 ===
//   勇者 Lv.1 HP:100/100
//
// --- Getter ---
//   名字：勇者
//   HP：100
//   等級：1
//
// --- Setter（正常）---
//   勇者 Lv.1 HP:60/100
//
// --- Setter（異常值被攔截）---
//   勇者 Lv.1 HP:0/100
//   勇者 Lv.1 HP:100/100
//   名字不能為空！
//   英雄 Lv.1 HP:100/100
//
// --- 沒有 setter 的欄位（等級）---
//   hero.setLevel(99); 會編譯失敗 —— 等級只能由遊戲邏輯提升
//   目前等級（唯讀）：1
//
// === LeetCode 1603. Design Parking System ===
//   初始車位  大:1 中:1 小:0
//   addCar(1) 大型車 -> true
//   addCar(2) 中型車 -> true
//   addCar(3) 小型車 -> false（小車位本來就是 0）
//   addCar(1) 再一輛 -> false（大車位已滿）
//   剩餘車位  大:0 中:0 小:0
//   → 計數器全程未曾變成負數：不變量由 addCar() 這個唯一入口守住
//
// === 日常實務：帳戶餘額不可為負 ===
//   存入後餘額（分）：10000
//   提領 30.00 -> 成功，餘額 7000
//   提領 999.00 -> 失敗（超過餘額，被攔截），餘額 7000
//   提領 -50 -> 失敗（負數金額，被攔截），餘額 7000
//   → 餘額欄位沒有 setter：外界只能透過 deposit/withdraw 改變它
