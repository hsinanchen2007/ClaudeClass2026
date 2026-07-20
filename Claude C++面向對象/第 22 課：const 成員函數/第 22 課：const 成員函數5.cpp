// =============================================================================
//  第 22 課：const 成員函數 5  —  const 重載:同名不同 const 資格
// =============================================================================
//
// 【主題資訊 Information】
//   語法：
//     const string& getText() const;   // (1) const 物件/參考 → 選這個
//     string&       getText();         // (2) 非 const 物件   → 選這個
//   標準版本：C++98 起即有。
//   複雜度：重載解析在編譯期完成,零執行期成本。
//   標頭檔：<string>
//
// 【詳細解釋 Explanation】
//
// 【1. 兩者為何能構成重載:隱含 this 參數型別不同】
//   (1) 的 this 是 const TextBuffer* const,(2) 的 this 是 TextBuffer* const。
//   兩者參數列(含隱含 this)不同,所以是合法重載。
//   注意「只有回傳型別不同」不能構成重載,這裡真正區分它們的是 const 資格,
//   不是 const string& 與 string& 的差異。
//
// 【2. 編譯器怎麼選:看「呼叫者物件」的 const 資格,不看你想拿它做什麼】
//   buf.getText()      → buf 是非 const → 選 (2),回傳可寫參考
//   constBuf.getText() → 物件是 const   → 選 (1),回傳唯讀參考
//   ref.getText()      → ref 是 const&  → 選 (1)
//   關鍵是:即使你只想讀,只要物件本身非 const,選到的仍是非 const 版本。
//   這也是本檔輸出中「非 const 物件即使只是讀,也印出非 const 版本」的原因。
//
// 【3. 這組重載解決什麼實際問題】
//   容器類別想同時支援「唯讀存取」與「可寫存取」時,若只提供一個
//   string& getText(),const 物件就完全用不了;若只提供 const 版本,
//   則沒有人能透過它修改。提供成對重載,才能讓同一個名字在兩種語境下
//   都給出「剛好正確」的權限。std::vector 的 operator[]、at()、
//   front()、back()、begin() 全都是這個模式。
//
// 【4. 避免重複實作:讓非 const 版本呼叫 const 版本】
//   當兩個版本的邏輯較複雜且完全相同時,常見手法是:
//       string& getText() {
//           return const_cast<string&>(
//               static_cast<const TextBuffer&>(*this).getText());
//       }
//   先把 *this 轉成 const 以選到 const 版本,再把回傳值的 const 去掉。
//   這是 const_cast 少數公認正當的用途 —— 因為底層物件本來就非 const。
//   反方向(讓 const 版本呼叫非 const 版本)則絕對不可以,那會是 UB。
//
// 【概念補充 Concept Deep Dive】
//   * const 資格會被編進 mangled name,所以連結器看到的是兩個不同符號。
//   * 本檔兩個版本都會印出自己被呼叫,因此可以「用輸出證明」編譯器選了誰 ——
//     這比單純講解更有說服力,也是驗證重載解析最直接的方法。
//   * buf.getText() = "Modified!"; 之所以合法,是因為 (2) 回傳非 const
//     左值參考,可以出現在等號左邊。若不小心把兩個版本都寫成回傳
//     const string&,這行就會編譯失敗。
//
// 【注意事項 Pay Attention】
//   1. 兩個版本要放在同一個類別中,且 const 資格必須不同,否則是重複定義。
//   2. 回傳型別務必配對正確:const 版本回傳 const T&,非 const 版本回傳 T&。
//      若 const 版本誤回傳 T&,等於從 const 物件洩漏了寫入權,封裝就破了。
//   3. 非 const 版本呼叫 const 版本時,轉型方向只能是「先加 const、
//      再去掉回傳值的 const」;反過來是 UB。
//   4. 對非 const 物件而言,即使只是讀取也會選到非 const 版本 ——
//      若該版本有額外副作用(例如標記 dirty flag),要特別小心。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】const 重載
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. const string& getText() const 與 string& getText() 為什麼能共存?
//     答：因為隱含的 this 參數型別不同(const T* const vs T* const),
//         等同參數列不同,構成合法重載,const 資格也會編入 mangled name。
//         要注意真正讓它們可共存的是「函式的 const 資格」,
//         不是回傳型別差異 —— 只有回傳型別不同是不能重載的。
//     追問：編譯器依什麼決定選哪個?→ 依「呼叫者物件本身」的 const 資格,
//         而不是依你打算拿回傳值做什麼。
//
// 🔥 Q2. 兩個版本邏輯相同時,怎麼避免複製貼上?
//     答：讓「非 const 版本」呼叫「const 版本」:先把 *this 轉成 const 參考
//         以選到 const 版本,再用 const_cast 去掉回傳值的 const。
//         因為底層物件本來就不是 const,這個 const_cast 是安全的,
//         也是 const_cast 少數公認正當的用途。
//     追問：反過來讓 const 版本呼叫非 const 版本可以嗎?→ 絕對不行。
//         那需要先把 const 物件轉成非 const,若該物件本來就宣告為 const,
//         後續修改即為未定義行為。
//
// ⚠️ 陷阱. 「我只是要讀取,所以應該會選到 const 版本吧?」
//     答：不會。重載解析只看「呼叫者物件的 const 資格」,完全不看你之後
//         打算讀還是寫。對非 const 的 buf 而言,buf.getText() 一定選到
//         非 const 版本 —— 本檔的實際輸出就是證據:即使那行只是讀值,
//         印出來的仍是「[調用非 const 版本]」。
//     為什麼會錯：把重載解析想像成「編譯器會依用途挑最合適的」。
//         實際上它只做型別比對:引數(含隱含 this)的型別決定一切,
//         回傳值後續怎麼被使用完全不在考慮範圍內。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 從缺,理由如下
//   const 重載是 C++ 特有的介面設計手法,LeetCode 題目給定的方法簽章
//   通常只有單一版本,判題也不檢查 const 資格。本課 summary.cpp 會以
//   705. Design HashSet 說明查詢/修改介面切分,在此不重複掛題。
//
// =============================================================================

#include <iostream>
#include <string>
#include <vector>     // 日常實務範例的 ConfigSection 需要；明確引入，不倚賴間接引入
using namespace std;

class TextBuffer {
private:
    string content_;

public:
    TextBuffer(const string& text) : content_(text) {}

    // const 版本：返回 const 引用（只讀）
    // 在 const 成員函數中，this 的類型是 const TextBuffer* const，表示 this 是一個指向 TextBuffer 對象的常量指針，並且指向的對象也是常量（不可修改）
    // 這裡的 getText() 函數有兩個版本：一個是 const 成員函數，返回 const 引用；另一個是非 const 成員函數，返回非 const 引用
    const string& getText() const {
        cout << "  [調用 const 版本]" << endl;
        return content_;
    }

    // 非 const 版本：返回非 const 引用（可讀寫）
    // 在非 const 成員函數中，this 的類型是 TextBuffer* const，表示 this 是一個指向 TextBuffer 對象的常量指針（指針本身不可修改，但指向的對象可以修改）
    // 這裡的 getText() 函數有兩個版本：一個是 const 成員函數，返回 const 引用；另一個是非 const 成員函數，返回非 const 引用
    string& getText() {
        cout << "  [調用非 const 版本]" << endl;
        return content_;
    }

    void print() const {
        cout << "  內容：「" << content_ << "」" << endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】設定檔區段:成對 const 重載 + 非 const 版本共用 const 版本實作
//   情境：設定物件需要「查詢」與「修改」兩種存取。查詢端往往拿到的是
//         const ConfigSection&（例如驗證器、序列化器），修改端則是可寫物件。
//   重點 1：value(key) 提供成對重載，兩種語境各自拿到剛好正確的權限。
//   重點 2：非 const 版本以 const_cast 共用 const 版本的查找邏輯，
//           避免同一段搜尋程式碼寫兩遍（真實專案裡這段常常有幾十行）。
// -----------------------------------------------------------------------------
class ConfigSection {
private:
    string         name_;
    vector<string> keys_;
    vector<string> values_;
    string         missing_;   // 查無此鍵時回傳的空字串

public:
    explicit ConfigSection(const string& name) : name_(name) {}

    void put(const string& k, const string& v) {
        for (size_t i = 0; i < keys_.size(); ++i) {
            if (keys_[i] == k) { values_[i] = v; return; }
        }
        keys_.push_back(k);
        values_.push_back(v);
    }

    const string& name() const { return name_; }
    size_t size()        const { return keys_.size(); }

    // const 版本：唯一真正實作查找邏輯的地方
    const string& value(const string& k) const {
        for (size_t i = 0; i < keys_.size(); ++i) {
            if (keys_[i] == k) return values_[i];
        }
        return missing_;
    }

    // 非 const 版本：先把 *this 轉成 const 以選到上面那個版本，
    // 再把回傳值的 const 去掉。底層物件本來就非 const，所以這是安全的。
    string& value(const string& k) {
        return const_cast<string&>(
            static_cast<const ConfigSection&>(*this).value(k));
    }
};

// 驗證器：以 const& 接收 → 只可能選到 const 版本
static void validateSection(const ConfigSection& s) {
    cout << "    區段 [" << s.name() << "] 共 " << s.size() << " 個鍵" << endl;
    cout << "    host    = " << s.value("host") << endl;
    cout << "    port    = " << s.value("port") << endl;
    cout << "    unknown = [" << s.value("unknown") << "]（查無此鍵）" << endl;
    // s.value("host") = "x";   // ❌ 編譯錯誤：const 版本回傳 const string&
}

int main() {
    cout << "=== const 重載 ===" << endl;

    // 非 const 對象
    cout << "\n--- 非 const 對象 ---" << endl;
    TextBuffer buf("Hello");
    buf.getText();                   // 調用非 const 版本
    buf.getText() = "Modified!";     // 可以通過引用修改
    buf.print();

    // const 對象
    cout << "\n--- const 對象 ---" << endl;
    const TextBuffer constBuf("ReadOnly");
    constBuf.getText();              // 調用 const 版本
    // constBuf.getText() = "Hack!"; // ❌ 編譯錯誤！返回的是 const 引用
    constBuf.print();

    // const 引用
    cout << "\n--- const 引用 ---" << endl;
    const TextBuffer& ref = buf;
    ref.getText();                   // 調用 const 版本
    ref.print();

    // ─────────────────────────────────────────────────────────
    cout << "\n=== 日常實務：設定區段的成對 const 重載 ===" << endl;
    ConfigSection db("database");
    db.put("host", "10.0.0.7");
    db.put("port", "5432");
    db.put("pool", "16");

    cout << "\n--- 修改端（非 const 物件 → 非 const 版本，可寫）---" << endl;
    db.value("host") = "10.0.0.99";      // 直接透過回傳的參考改值
    cout << "    改寫 host 後：" << db.value("host") << endl;

    cout << "\n--- 驗證端（以 const& 接收 → 只能走 const 版本）---" << endl;
    validateSection(db);

    cout << "\n  非 const 版本共用了 const 版本的查找邏輯，" << endl;
    cout << "  查找程式碼只存在一份。" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 22 課：const 成員函數5.cpp" -o l22_5
// 執行: ./l22_5        (rc=0)

// === 預期輸出 ===
// === const 重載 ===
//
// --- 非 const 對象 ---
//   [調用非 const 版本]
//   [調用非 const 版本]
//   內容：「Modified!」
//
// --- const 對象 ---
//   [調用 const 版本]
//   內容：「ReadOnly」
//
// --- const 引用 ---
//   [調用 const 版本]
//   內容：「Modified!」
//
// === 日常實務：設定區段的成對 const 重載 ===
//
// --- 修改端（非 const 物件 → 非 const 版本，可寫）---
//     改寫 host 後：10.0.0.99
//
// --- 驗證端（以 const& 接收 → 只能走 const 版本）---
//     區段 [database] 共 3 個鍵
//     host    = 10.0.0.99
//     port    = 5432
//     unknown = []（查無此鍵）
//
//   非 const 版本共用了 const 版本的查找邏輯，
//   查找程式碼只存在一份。
