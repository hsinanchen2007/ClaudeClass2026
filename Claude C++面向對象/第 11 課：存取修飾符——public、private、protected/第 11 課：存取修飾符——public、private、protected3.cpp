// =============================================================================
//  第 11 課 -3  —  private：只有自己人能碰的內部狀態
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  class X { private: /* 成員 */ };   // class 的預設存取權即 private
//   標準：  C++98 起
//   標頭檔：本例僅需 <iostream>、<string>
//   關鍵詞：private、information hiding、access control、friend
//
//   private 成員的可存取範圍精確地說是：
//     (1) 該類別自己的成員函式（含 static 成員函式）
//     (2) 該類別的 friend 函式／friend 類別
//   —— 就這兩個。連衍生類別都不行（那是 protected 的工作）。
//
// 【詳細解釋 Explanation】
//
// 【1. 「同一個類別」而非「同一個物件」】
// 這是最常被誤解的一點。private 的粒度是「類別」，不是「物件實例」。
// 也就是說，Safe 的成員函式可以存取「任何一個 Safe 物件」的 private 成員，
// 不只是 this 指向的那一個：
//     bool sameMoney(const Safe& other) const {
//         return money == other.money;    // ✅ 合法！可以碰 other 的 private
//     }
// 這正是拷貝建構函式、operator== 之類的函式能運作的原因 —— 否則
// `Safe(const Safe& other) : money(other.money) {}` 會編譯不過。
// 本檔的 hasSameBalance() 實際示範了這一點。
//
// 【2. private 保護的是「誰能寫這行程式碼」，不是「記憶體」】
// 本例的 password 是 private，看起來很「安全」，但要說清楚：
//   * 它擋得住：別的程式碼不小心寫 mySafe.password = "" 或誤讀密碼。
//   * 它擋不住：任何人拿到編譯後的 binary，用 strings 就能看到 "secret123"，
//     因為那是個字串常數，明文躺在執行檔裡。
// 換句話說，private 是「編譯期的名稱存取檢查」，不是加密、不是權限系統。
// 真實系統絕不會這樣存密碼 —— 正確做法是只存 salted hash，
// 比對時 hash 使用者輸入再比對 hash，明文永遠不落地。
// 本檔的 SecureVault 示範了這個修正方向（用簡化的示範用 hash，非正式密碼學）。
//
// 【3. 為什麼 unlock() 回傳 bool 而不是回傳密碼】
// 介面設計的關鍵：unlock() 對外承諾的是「驗證」這個動作，
// 而不是「把秘密交出去」。若寫成 `string getPassword() const`，
// private 就白設了 —— 你只是換了個更長的方式把它公開。
// 判準：public 介面應該回答「你能做什麼」，而不是「我藏了什麼」。
//
// 【4. private 成員函式：藏起實作步驟】
// private 不只用在資料。把「只有內部才需要的步驟」設成 private 成員函式，
// 可以讓 public 介面保持精簡，也讓你隨時能改寫、合併、刪除那些步驟
// 而不影響任何呼叫端。本檔的 SecureVault::hashOf() 與 recordAttempt()
// 就是這類函式：它們是實作細節，不是對外承諾。
//
// 【概念補充 Concept Deep Dive】
// (A) access control 發生在「存取檢查」階段，晚於「名稱查找」與「多載決議」
//     這個順序有實際後果：一個 private 的多載函式仍然會參與多載決議，
//     若它勝出，才在最後一步報「is private」錯誤 —— 編譯器不會退而求其次
//     去挑一個 public 的多載。所以把某個多載設成 private，
//     不等於「讓外界改用另一個多載」，而是「讓外界直接編譯失敗」。
//
// (B) private 成員仍然佔記憶體、仍然參與 sizeof 與物件佈局
//     access specifier 完全不影響物件大小與成員是否存在，
//     只影響「哪些程式碼被允許寫出它的名字」。編譯後的機器碼與 public 版本相同。
//
// (C) 想要「外部函式也能碰 private」時，正式管道是 friend
//     常見於 operator<< 這種必須是非成員的運算子：
//         friend std::ostream& operator<<(std::ostream&, const Safe&);
//     friend 是「類別主動授權」，寫在類別內部，所以封裝仍然由類別自己掌控
//     —— 外部無法自行宣稱是某個類別的 friend。
//
// (D) 用 reinterpret_cast 硬讀 private 記憶體是 UB
//     這種「示範 private 可以被繞過」的把戲在網路上很常見，
//     但它踩的是 undefined behavior，標準不保證任何結果，
//     不同編譯器／最佳化等級下的行為都可能不同。本檔不做這種示範。
//
// 【注意事項 Pay Attention】
// 1. private 的粒度是「類別」：同類別的其他物件也碰得到，這不是 bug。
// 2. private 不是資安機制。密碼明文放進 private 成員，binary 裡照樣看得到。
// 3. 別把 private 成員配一個 getter 就當成封裝 —— 那等於沒設 private。
// 4. private 成員函式一樣參與多載決議，勝出後才報存取錯誤。
// 5. class 的預設存取權就是 private，寫不寫 `private:` 只是可讀性差別。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】private 與資訊隱藏
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 一個類別的成員函式，能不能存取「另一個同型別物件」的 private 成員？
//     答：可以。private 的粒度是類別而非物件實例。
//         所以 `bool eq(const Safe& o) const { return money == o.money; }` 完全合法。
//         這正是 copy constructor、operator== 得以實作的前提。
//     追問：那不同型別呢？例如 Safe 能碰 Vault 的 private 嗎？
//         → 不行，除非 Vault 明確把 Safe 宣告為 friend。
//
// 🔥 Q2. private 和 protected 差在哪？什麼時候該用哪個？
//     答：private 只有自己類別 + friend 能碰；protected 額外開放給衍生類別。
//         預設應該用 private；只有當你「刻意設計這個成員給子類別使用」時
//         才用 protected —— 因為 protected 等於把它變成「對所有子類別的公開契約」，
//         之後想改就會影響所有繼承者。
//     追問：protected 資料成員為什麼常被視為 code smell？
//         → 它破壞封裝的程度接近 public：任何人只要繼承你的類別就能碰它，
//           而你無法控制誰來繼承。多數指南建議 protected 只用於成員函式。
//
// ⚠️ 陷阱. 「把密碼設成 private，別人就拿不到了」——錯在哪？
//     答：private 是編譯期的名稱存取檢查，不產生任何執行期保護。
//         "secret123" 是字串常數，直接存在執行檔的資料段裡，
//         對 binary 執行 strings 就能看到，跟它是 public 還是 private 完全無關。
//     為什麼會錯：把「存取控制（access control）」誤當成「存取保護（protection）」。
//         前者是給編譯器看的設計宣告，後者需要作業系統權限、加密、或硬體隔離。
//         正確做法是根本不存明文：只存 salted hash，比對 hash 而非比對密碼。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
using namespace std;

// -----------------------------------------------------------------------------
// 基礎示範：private 資料 + public 驗證介面
// -----------------------------------------------------------------------------
class Safe {
private:
    string password = "secret123";
    double money = 50000.0;

public:
    bool unlock(const string& input) {
        if (input == password) {    // 類別內部可以存取 private
            cout << "  保險箱已開啟，裡面有 $" << money << endl;
            return true;
        } else {
            cout << "  密碼錯誤！" << endl;
            return false;
        }
    }

    // 重點示範：private 的粒度是「類別」不是「物件」
    // 這裡碰的是 other 的 private 成員，完全合法。
    bool hasSameBalance(const Safe& other) const {
        return money == other.money;   // ✅ 可以讀 other.money
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】使用者帳號驗證：private 成員函式 + 不儲存明文密碼
//   情境：登入模組的真實需求
//     (a) 絕不儲存明文密碼 —— 只存 hash（此處用簡化示範用 hash，非正式密碼學）
//     (b) 連續失敗要鎖定帳號，且鎖定計數不能被外部竄改
//     (c) hash 演算法屬實作細節，未來要換成 bcrypt/argon2 時呼叫端不該受影響
//   做法：hash 與計數全 private；hashOf()/recordAttempt() 是 private 成員函式。
//   ⚠️ 教學用途：真實系統請用 libsodium / bcrypt / argon2，絕不要自己寫 hash。
// -----------------------------------------------------------------------------
class SecureVault {
private:
    string m_userHash;             // 只存 hash，明文從不落地
    int    m_failCount = 0;
    bool   m_locked = false;
    static const int kMaxFail = 3;

    // private 成員函式：實作細節，未來可整支換掉而不影響呼叫端
    static unsigned long hashOf(const string& s) {
        unsigned long h = 5381;                 // djb2（示範用，非密碼學安全）
        for (char c : s) h = h * 33u + static_cast<unsigned char>(c);
        return h;
    }

    void recordAttempt(bool ok) {
        if (ok) {
            m_failCount = 0;
        } else if (++m_failCount >= kMaxFail) {
            m_locked = true;
        }
    }

public:
    explicit SecureVault(const string& initialPassword)
        : m_userHash(to_string(hashOf(initialPassword))) {}

    bool login(const string& attempt) {
        if (m_locked) {
            cout << "  [鎖定] 帳號已因連續失敗被鎖定，請聯繫管理員" << endl;
            return false;
        }
        bool ok = (to_string(hashOf(attempt)) == m_userHash);
        recordAttempt(ok);
        cout << "  [登入] " << (ok ? "成功" : "失敗")
             << "（累計失敗 " << m_failCount << "/" << kMaxFail << "）" << endl;
        return ok;
    }

    bool isLocked() const { return m_locked; }
    // 刻意不提供 getPassword()／getHash() —— 那會讓 private 形同虛設
};

int main() {
    cout << "=== private 資料 + public 驗證介面 ===" << endl;
    Safe mySafe;
    mySafe.unlock("wrong");      // 呼叫 public 函數
    mySafe.unlock("secret123");  // 呼叫 public 函數

    // 下面兩行若解除註解會編譯失敗（錯誤被擋在編譯期）
    // mySafe.password;          // ❌ error: 'password' is private
    // mySafe.money = 0;         // ❌ error: 'money' is private
    cout << "  （mySafe.password 會編譯失敗：'password' is private）" << endl;

    cout << "\n=== private 的粒度是「類別」不是「物件」 ===" << endl;
    Safe another;
    cout << "  兩個保險箱餘額相同？ "
         << (mySafe.hasSameBalance(another) ? "是" : "否") << endl;
    cout << "  → hasSameBalance 讀取了 other.money（別的物件的 private），合法。" << endl;

    cout << "\n=== 日常實務：不存明文密碼 + 失敗鎖定 ===" << endl;
    SecureVault vault("Tr0ub4dor&3");
    vault.login("wrong1");
    vault.login("wrong2");
    vault.login("Tr0ub4dor&3");   // 成功後失敗計數歸零
    vault.login("wrong3");
    vault.login("wrong4");
    vault.login("wrong5");        // 第三次失敗 → 鎖定
    vault.login("Tr0ub4dor&3");   // 已鎖定，即使密碼正確也拒絕
    cout << "  最終鎖定狀態： " << (vault.isLocked() ? "已鎖定" : "正常") << endl;

    cout << "\n=== 重要提醒 ===" << endl;
    cout << "  private 是編譯期的存取檢查，不是加密。" << endl;
    cout << "  Safe 裡的 \"secret123\" 是字串常數，對 binary 執行 strings 即可看到。" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 11 課：存取修飾符——public、private、protected3.cpp" -o access3

// === 預期輸出 ===
// === private 資料 + public 驗證介面 ===
//   密碼錯誤！
//   保險箱已開啟，裡面有 $50000
//   （mySafe.password 會編譯失敗：'password' is private）
//
// === private 的粒度是「類別」不是「物件」 ===
//   兩個保險箱餘額相同？ 是
//   → hasSameBalance 讀取了 other.money（別的物件的 private），合法。
//
// === 日常實務：不存明文密碼 + 失敗鎖定 ===
//   [登入] 失敗（累計失敗 1/3）
//   [登入] 失敗（累計失敗 2/3）
//   [登入] 成功（累計失敗 0/3）
//   [登入] 失敗（累計失敗 1/3）
//   [登入] 失敗（累計失敗 2/3）
//   [登入] 失敗（累計失敗 3/3）
//   [鎖定] 帳號已因連續失敗被鎖定，請聯繫管理員
//   最終鎖定狀態： 已鎖定
//
// === 重要提醒 ===
//   private 是編譯期的存取檢查，不是加密。
//   Safe 裡的 "secret123" 是字串常數，對 binary 執行 strings 即可看到。
