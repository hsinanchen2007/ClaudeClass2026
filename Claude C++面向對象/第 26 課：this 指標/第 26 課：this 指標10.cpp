// =============================================================================
//  第 26 課 -10  —  *this 的四種典型用法：回傳自身、拷貝自身、比對身分、傳出自身
// =============================================================================
//
// 【主題資訊 Information】
//   this 的型別  ： 非 const 成員函式中為 ClassName* const
//                   const 成員函式中為 const ClassName* const
//   *this 的型別 ： ClassName&（或 const ClassName&），是個左值
//   本檔涵蓋的四種用法：
//       1. return *this;        → 回傳自身參考，支援鏈式呼叫
//       2. Score copy = *this;  → 以自身為來源拷貝一份，做「不修改原物件」的運算
//       3. this == &other       → 比對「是不是同一個物件」（身分，而非內容）
//       4. printFunc(*this);    → 把自身當引數傳給外部函式
//   標準版本   ： 全部自 C++98 即可用
//   標頭檔     ： <string>
//
// 【詳細解釋 Explanation】
//
// 【1. 用法一：return *this —— 鏈式呼叫】
//   addPoints() 回傳 Score&，所以 s.addPoints(50).addPoints(30).addPoints(20)
//   三次都作用在同一個 s 上，最後 100+50+30+20 = 200。
//   關鍵在回傳型別有 &；少了它就會變成每步都在改暫時副本
//   （本課 -3 檔用計數實測過這個差別）。
//
// 【2. 用法二：Score copy = *this —— 拷貝自身】
//   doubled() 標了 const，代表「不修改原物件」。
//   要產出一個「翻倍後的新分數」，做法是先拷貝自己再改副本：
//       Score copy = *this;   // 呼叫拷貝建構函數
//       copy.points_ *= 2;
//       return copy;          // 符合 NRVO 條件，可能完全不產生額外拷貝
//   這是 const 成員函式產出「衍生結果」的標準寫法，
//   也是 operator+ 這類非變動運算子的實作骨架。
//   注意 return copy; 不要寫成 return std::move(copy);——
//   那會擋掉 NRVO，反而多一次移動（見第 28 課 -2 與第 31 課 -4）。
//
// 【3. 用法三：this == &other —— 比的是身分，不是內容】
//   isHigherThan() 開頭的 if (this == &other) return false;
//   問的是「other 是不是就是我自己」。
//   注意這是「指標相等」比較，比的是記憶體位址（身分），
//   不是 points_ 相不相等（內容）。兩個分數同為 200 的不同物件，
//   this == &other 仍然是 false。
//   這個 idiom 正是拷貝賦值與移動賦值的「自我賦值防護」所用的同一招：
//       Widget& operator=(const Widget& o) {
//           if (this != &o) { ... }      // 見第 29 課、第 33 課
//           return *this;
//       }
//   在移動賦值裡它尤其關鍵：少了這道防護，
//   自我賦值會先 delete 自己的資源、再從「已釋放的自己」偷取 → 未定義行為。
//
// 【4. 用法四：printFunc(*this) —— 把自身傳出去】
//   成員函式內把 *this 當引數傳給外部函式，
//   接收端宣告成 const Score& 就不會有拷貝。
//   常見於「把自己註冊到觀察者清單」「把自己交給序列化函式」等場景。
//   若接收端寫成 Score（傳值），就會多一次拷貝 —— 這也是 const& 幾乎永遠
//   優於傳值的原因（除非接收端本來就要一份自己的副本）。
//
// 【概念補充 Concept Deep Dive】
//
// (A) this 是隱藏參數，不佔物件空間
//   sizeof(Score) 只算 playerName_ 與 points_，不包含 this。
//   成員函式在機器碼層面就是「多收一個物件位址參數的普通函式」，
//   常見 ABI（System V AMD64）用 rdi 暫存器傳遞它。
//   所以「物件呼叫方法」只是語法包裝，執行期並沒有神秘的綁定。
//
// (B) 為什麼 doubled() 能標 const，而 addPoints() 不行
//   const 成員函式中 this 是 const Score* const，
//   *this 因而是 const Score&，不能用來修改成員。
//   doubled() 只讀自己、改的是副本，所以可以標 const；
//   addPoints() 直接改 points_，標了 const 會編譯失敗。
//   實務守則：不修改狀態的成員函式一律標 const，
//   否則 const 物件與 const& 參數就沒辦法呼叫它。
//
// (C) this == &other 為什麼比 *this == other 更適合做身分檢查
//   *this == other 需要類別有定義 operator==，比的是「內容相等」，
//   而且可能有自訂的比較語義（例如忽略大小寫）。
//   自我賦值防護要問的是「這兩個名字是不是同一塊記憶體」，
//   只有位址比較答得準。內容相同但位址不同的兩個物件，
//   賦值仍必須照常執行。
//
// (D) 函式指標 vs std::function／模板
//   printVia() 收的是裸函式指標 void(*)(const Score&)，
//   這代表它「不能」接受有捕獲的 lambda 或函式物件。
//   現代寫法會改成模板參數或 std::function<void(const Score&)>，
//   前者零額外成本、後者有型別抹除的開銷。
//   本檔沿用原始寫法以聚焦在 *this 的傳遞，不是在推薦這個介面設計。
//
// 【注意事項 Pay Attention】
//   1. this == &other 比的是位址（身分），不是內容。
//      內容相同的兩個不同物件，這個判斷仍為 false —— 而那正是我們要的。
//   2. const 成員函式中 *this 是 const ClassName&，
//      不能用它回傳非 const 的鏈，也不能拿來修改成員。
//   3. 拷貝自身時 return copy; 不要加 std::move —— 會擋掉 NRVO。
//   4. 把 *this 傳給外部函式時，接收端用 const& 才不會多一次拷貝。
//   5. 本檔的 Score 成員是 std::string 與 int，會自己管理資源，
//      因此不需要寫五法則（Rule of Zero）。
//      若成員換成裸指標，第 34 課的五法則就必須全部補齊。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】*this 的用法與身分比較
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. if (this == &other) 這行在做什麼？為什麼賦值運算子裡常看到它？
//     答：它比較「兩個名字是不是指向同一個物件」，也就是身分檢查
//         （比位址，不是比內容）。在拷貝賦值與移動賦值裡，
//         它是自我賦值防護：a = a 這種情況若不擋，
//         典型的「先釋放自己的資源、再從來源複製／偷取」流程
//         就會從已經釋放掉的記憶體讀取。
//     追問：拷貝賦值與移動賦值，哪一個少了它比較危險？
//         → 移動賦值。它會 delete 自己的資源再從來源偷指標，
//           自我賦值時來源就是自己，等於從剛釋放的記憶體取值 → 未定義行為。
//           拷貝賦值若採「先配置新的、成功後才釋放舊的」寫法，
//           即使沒有這道防護也仍然正確（只是多做一次白工）。
//
// 🔥 Q2. const 成員函式裡的 this 和 *this 分別是什麼型別？有什麼實際影響？
//     答：this 是 const ClassName* const，*this 是 const ClassName&。
//         影響有兩個：不能用來修改任何非 mutable 成員；
//         也不能把 *this 回傳給要求 ClassName& 的鏈式呼叫。
//         所以本檔的 doubled() 可以標 const（只讀自己、改副本），
//         addPoints() 不行（直接改成員）。
//     追問：那 const 物件呼叫非 const 成員函式會怎樣？
//         → 編譯錯誤。這正是「不修改狀態就標 const」的理由：
//           少標一個 const，這個方法對所有 const 物件與 const& 參數都失效。
//
// ⚠️ 陷阱. 「兩個物件的內容一樣，this == &other 應該就會成立。」
//     答：不成立。this == &other 比的是記憶體位址。
//         本檔的 Score other("對手", 150) 與 s 內容不同固然是 false；
//         但就算另外造一個分數同為 200 的物件，答案仍然是 false，
//         因為它們是兩塊不同的記憶體。
//         要比內容必須自己定義 operator== 並用 *this == other。
//     為什麼會錯：把「相等」和「同一個」混為一談。
//         賦值運算子的自我防護要問的正是「同一個」——
//         內容碰巧相同的兩個物件，賦值仍然必須照常執行。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔從缺，理由如下
//   *this 的用法屬於語言機制與類別介面設計，
//   LeetCode 判的是演算法輸入輸出，評測系統只會建立一個實例並呼叫指定方法，
//   既不會拷貝你的物件，也碰不到自我賦值或鏈式呼叫。
//   即使是設計類題目（155. Min Stack、1603. Design Parking System），
//   方法簽名都由題目固定，無從示範 return *this。
//   硬掛題號只會誤導，故從缺；改以「帳戶轉帳的自我轉帳防護」
//   這個真實情境呈現 this == &other 的價值。
//
// -----------------------------------------------------------------------------

#include <iostream>
#include <string>
using namespace std;

class Score {
private:
    string playerName_;
    int points_;

public:
    Score(const string& name, int pts)
        : playerName_(name), points_(pts)
    {
    }

    // 用法 1：返回自身引用（鏈式調用）
    // ★ 回傳型別是 Score&（有 &），所以三次 addPoints 作用在同一個物件上
    Score& addPoints(int pts) {
        points_ += pts;
        return *this;    // *this 是 Score&
    }

    // 用法 2：返回自身的拷貝（不修改原物件）
    // ★ 標了 const → this 是 const Score* const，只能讀自己、改副本
    Score doubled() const {
        Score copy = *this;     // 拷貝自身（呼叫拷貝建構函數）
        copy.points_ *= 2;
        return copy;            // 符合 NRVO 條件；別寫成 return std::move(copy);
    }

    // 用法 3：比較自身與另一個對象
    bool isHigherThan(const Score& other) const {
        // 先檢查是不是和自己比 —— 比的是「位址（身分）」不是「內容」
        // 這正是拷貝／移動賦值裡自我賦值防護所用的同一個 idiom
        if (this == &other) return false;
        return points_ > other.points_;
    }

    // 用法 4：把自身傳給外部函數
    void printVia(void (*printFunc)(const Score&)) const {
        printFunc(*this);    // 把自身傳出去；接收端用 const& 故無拷貝
    }

    const string& getName() const { return playerName_; }
    int getPoints() const { return points_; }
};

// 外部的列印函數
void fancyPrint(const Score& s) {
    cout << "  ★ " << s.getName() << "：" << s.getPoints() << " 分 ★" << endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】帳戶轉帳的「自我轉帳」防護
//   情境：金流服務提供 transferTo(dest, amount)。使用者介面上可能因為
//         下拉選單預設值相同，讓來源與目的地是同一個帳戶；
//         批次作業也可能因資料重複而產生 A→A 的轉帳指令。
//   為什麼用到本主題：若不先用 this == &dest 擋下來，
//         「先扣自己、再加給對方」的流程作用在同一個物件上，
//         結果雖然金額碰巧不變，但中間狀態已經出現過餘額不足的判斷，
//         而且若改成「先檢查再扣款」的順序還可能憑空產生或消滅金額。
//         這與賦值運算子的自我賦值防護是完全相同的問題與解法。
//   注意：這裡比的是物件身分（位址），不是帳號字串相等 ——
//         同一個帳戶必然是同一個物件時，位址比較最直接也最可靠。
// -----------------------------------------------------------------------------
class Account {
private:
    string id_;
    long   balanceCents_;

public:
    Account(const string& id, long balanceCents)
        : id_(id), balanceCents_(balanceCents) {}

    // 回傳是否成功
    bool transferTo(Account& dest, long amountCents) {
        // ★ 與 operator= 的 if (this != &other) 是同一個 idiom
        if (this == &dest) {
            cout << "  [拒絕] 來源與目的地是同一個帳戶（" << id_ << "），不執行轉帳" << endl;
            return false;
        }
        if (amountCents <= 0 || amountCents > balanceCents_) {
            cout << "  [拒絕] 金額不合法或餘額不足" << endl;
            return false;
        }
        balanceCents_      -= amountCents;
        dest.balanceCents_ += amountCents;
        cout << "  [成功] " << id_ << " → " << dest.id_
             << " 金額 " << amountCents << " 分" << endl;
        return true;
    }

    const string& id() const { return id_; }
    long balance() const { return balanceCents_; }
};

int main() {
    cout << "=== *this 的各種用法 ===" << endl;

    Score s("陳信安", 100);

    // 用法 1：鏈式調用
    cout << "\n--- 鏈式加分 ---" << endl;
    s.addPoints(50).addPoints(30).addPoints(20);
    cout << "  總分：" << s.getPoints() << endl;

    // 用法 2：拷貝自身
    cout << "\n--- 翻倍（不影響原物件）---" << endl;
    Score doubled = s.doubled();
    cout << "  原始：" << s.getPoints() << endl;
    cout << "  翻倍：" << doubled.getPoints() << endl;

    // 用法 3：比較
    cout << "\n--- 比較 ---" << endl;
    Score other("對手", 150);
    cout << "  " << s.getName() << " 高於 " << other.getName() << "？ "
         << (s.isHigherThan(other) ? "是" : "否") << endl;
    cout << "  和自己比？ "
         << (s.isHigherThan(s) ? "是" : "否") << endl;

    // 用法 3b：內容相同但不是同一個物件 → 身分比較仍是 false
    Score twin("分身", s.getPoints());
    cout << "  另造一個同分物件，和它比？ "
         << (s.isHigherThan(twin) ? "是" : "否")
         << "（分數相同故不高於；但兩者是不同物件，身分檢查不會攔下）" << endl;

    // 用法 4：傳給外部函數
    cout << "\n--- 傳遞 *this ---" << endl;
    s.printVia(fancyPrint);

    // 日常實務：自我轉帳防護
    cout << "\n=== 日常實務：帳戶轉帳的自我轉帳防護 ===" << endl;
    Account a("ACC-001", 100000);
    Account b("ACC-002", 5000);

    cout << "  轉帳前：" << a.id() << "=" << a.balance()
         << "  " << b.id() << "=" << b.balance() << endl;

    a.transferTo(b, 25000);      // 正常轉帳
    a.transferTo(a, 25000);      // 自我轉帳 → 被 this == &dest 擋下

    cout << "  轉帳後：" << a.id() << "=" << a.balance()
         << "  " << b.id() << "=" << b.balance() << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -Wreorder "第 26 課：this 指標10.cpp" -o lesson26j

// ── 輸出說明（讀之前先看這裡）────────────────────────────────────────────
// * 本檔輸出完全決定性：只有整數與字串運算，沒有位址、沒有耗時、
//   沒有執行緒（本機實測連跑 5 次逐位元組相同）。
// * 刻意「不列印任何原始位址」：位址會因 ASLR 每次執行都不同。
//   身分比較的結果以布林值（是／否）呈現，這才是可重現的證據。
// * 「和自己比？ 否」是 this == &other 攔下的結果；
//   下一行「另造一個同分物件」則說明它比的是位址不是內容 ——
//   twin 的分數與 s 相同，但兩者是不同物件，身分檢查不會攔截，
//   回答「否」是因為 200 > 200 不成立，而非被 this == &other 擋下。
// * 帳戶金額以「分」為單位的整數表示，不用浮點數 ——
//   這是金流系統的常規做法，避免二進位浮點數無法精確表示 0.1 這類值。
// * 自我轉帳被擋下後餘額不變，可對照轉帳前後兩行確認。
// ─────────────────────────────────────────────────────────────────────────

// === 預期輸出 ===
// === *this 的各種用法 ===
//
// --- 鏈式加分 ---
//   總分：200
//
// --- 翻倍（不影響原物件）---
//   原始：200
//   翻倍：400
//
// --- 比較 ---
//   陳信安 高於 對手？ 是
//   和自己比？ 否
//   另造一個同分物件，和它比？ 否（分數相同故不高於；但兩者是不同物件，身分檢查不會攔下）
//
// --- 傳遞 *this ---
//   ★ 陳信安：200 分 ★
//
// === 日常實務：帳戶轉帳的自我轉帳防護 ===
//   轉帳前：ACC-001=100000  ACC-002=5000
//   [成功] ACC-001 → ACC-002 金額 25000 分
//   [拒絕] 來源與目的地是同一個帳戶（ACC-001），不執行轉帳
//   轉帳後：ACC-001=75000  ACC-002=30000
