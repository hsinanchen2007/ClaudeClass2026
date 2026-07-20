// =============================================================================
//  第 11 課 -8  —  access specifier 可以重複出現：區塊式而非一次性
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  class X { public: ... private: ... public: ... };
//   標準：  C++98 起即有；重複出現的 access specifier 一直都合法。
//   標頭檔：<iostream>
//   預設值：class 的預設存取權是 private；struct 是 public（第 12 課主題）。
//
// 【詳細解釋 Explanation】
//
// 【1. access specifier 是「從這裡開始」，不是「這個區塊」】
//   很多人以為 public: 後面必須把所有公開成員寫完，之後不能再回頭。
//   實際上 access specifier 的語意是**切換目前的存取模式**，
//   從它出現的那一行起，直到下一個 specifier 或類別結束為止。
//   所以 public → private → public → private → public 交替出現完全合法，
//   而且不限次數。本檔就是這樣寫的，能正常編譯執行。
//
// 【2. 那該不該這樣寫？】
//   語法允許，不代表是好風格。實務上幾乎所有 coding style（Google、LLVM、
//   Core Guidelines）都建議**每種存取權只出現一次、並依固定順序排列**：
//       public → protected → private
//   理由是可讀性：讀者想知道「這個類別對外提供什麼」時，
//   只要看最上面那一段就好，不必掃完整個類別確認有沒有漏掉第二個 public 區塊。
//   反例就是本檔 —— 你必須讀到最後一行才敢說 func3 也是公開的。
//
//   ★ 唯一常見的例外：把 private 成員變數放在最下面（而非最上面），
//     讓讀者先看到介面、後看到實作細節。這仍然只用了一組 specifier。
//
// 【3. 為什麼順序不影響成員的可見性】
//   func2() 定義在 data1 之後，卻也能存取 data1；
//   而 func1() 定義在 data1 之前 —— 如果它要用 data1 也完全沒問題。
//   原因是成員函式的**本體**在完整類別定義之後才解析
//   （complete-class context），所以類別內所有成員彼此可見，
//   與書寫順序無關。access specifier 管的是「誰能存取」，
//   不是「什麼時候看得到」。
//
// 【概念補充 Concept Deep Dive】
//   access specifier 是**純編譯期**概念，不產生任何執行期成本 ——
//   private 成員與 public 成員在記憶體中沒有任何區別，
//   編譯後的機器碼裡也沒有「檢查權限」這種指令。
//   它擋的是「誤用」，不是「惡意存取」：
//   用指標算術或 reinterpret_cast 一樣讀得到 private 成員的位元組
//   （雖然那是 UB）。所以 private **不是安全機制**。
//
//   另一個實務上很重要、卻幾乎沒人提的細節：
//   **把資料成員拆散在多個 access specifier 區塊，會讓型別失去 standard-layout**。
//   標準明定 standard-layout 的條件之一是「所有非靜態資料成員
//   都宣告在同一個 access specifier 區塊」。本檔的 data1 與 data2
//   分屬兩個不同的 private 區塊，因此 Demo **不是** standard-layout type。
//   這不是理論問題，有三個可觀測的後果：
//     (a) offsetof 對非 standard-layout 型別是條件式支援（不保證可用）；
//     (b) 無法安全地與 C 的 struct 對應（跨語言／序列化／檔案格式全受影響）；
//     (c) 「第一個成員的位址等於物件位址」這條保證失效，
//         因此常見的 reinterpret_cast 到首成員型別的技巧不再合法。
//   順序方面，跨區塊時編譯器也可自由重排
//   （本機 GCC 15.2 實測未重排，但那是實作選擇，不可依賴）。
//   下方 demoLayoutImpact() 把這件事實際測給你看。
//
// 【注意事項 Pay Attention】
//   1. 重複的 access specifier 合法，但會傷可讀性，實務上避免。
//   2. class 預設 private、struct 預設 public；忘了寫 public: 是初學者
//      「為什麼我的成員函式叫不到」最常見的原因。
//   3. 跨 access specifier 區塊的資料成員排列順序是實作定義的，
//      會影響 standard-layout 與 offsetof 的可用性。
//   4. private 只擋編譯期誤用，不是安全邊界，別拿它保護機密資料。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】access specifier 的作用範圍
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 一個類別裡可以有幾個 public: 區塊？
//     答：不限次數。access specifier 的語意是「從這裡開始切換存取模式」，
//         直到下一個 specifier 或類別結束為止，所以可以任意交替出現。
//         本檔有三個 public 區塊與兩個 private 區塊，完全合法。
//     追問：那實務上建議怎麼寫？
//         → 每種只出現一次，依 public → protected → private 排列，
//           讓讀者看最上面就知道對外介面有哪些，不必掃完整個類別。
//
// 🔥 Q2. func2() 寫在 data1 後面才存取 data1，若寫在前面還能存取嗎？
//     答：可以。成員函式的本體在完整類別定義之後才解析
//         （complete-class context），因此類別內所有成員彼此可見，
//         與書寫順序無關。access specifier 決定的是「誰能存取」，
//         不是「何時看得到」。
//     追問：那什麼東西會受書寫順序影響？
//         → 成員的**初始化順序**（依宣告順序，不依初始化列的順序）、
//           成員的**型別**與**預設引數**都必須先宣告後使用。
//
// ⚠️ 陷阱. 把資料成員拆進兩個不同的 access specifier 區塊，只是風格問題嗎？
//     答：**不只是風格，會改變型別的性質**。
//         標準規定：所有非靜態資料成員都在**同一個** access specifier 區塊，
//         是 standard-layout 的必要條件之一。一旦拆開，該型別就**不再是**
//         standard-layout type —— 本機實測 std::is_standard_layout 由 true 變 false
//         （見下方 demoLayoutImpact 的可執行驗證）。
//         後果是：offsetof 不再有保證、與 C 結構體的相容性失去標準背書、
//         「第一個成員的位址等於物件位址」這條保證也不再成立。
//         此外，跨區塊時成員的相對順序本身也是實作定義的
//         （本機 GCC 15.2 實測**沒有**重排，但那是實作選擇，不可依賴）。
//     為什麼會錯：大家把 access specifier 純粹當成「權限標籤」，
//         以為它不影響記憶體；也把「宣告順序 = 記憶體順序」當成鐵律。
//         但標準只在同一區塊內給順序保證，
//         而 standard-layout 的喪失更是可觀測、可測量的實質後果。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
using namespace std;

class Demo {
public:
    void func1() { cout << "func1" << endl; }

private:
    int data1 = 0;

public:     // 第二個 public 區塊 —— 完全合法
    void func2() { cout << "func2, data1=" << data1 << endl; }

private:    // 第二個 private 區塊
    int data2 = 0;

public:     // 第三個 public 區塊
    void func3() {
        cout << "func3, data1=" << data1
             << ", data2=" << data2 << endl;
    }
};

// -----------------------------------------------------------------------------
// 【可執行示範】拆散 access 區塊 → 型別失去 standard-layout
//   這是「重複 access specifier」唯一可測量的實質代價。
// -----------------------------------------------------------------------------
struct Scattered {          // 資料成員橫跨兩個 access 區塊
public:
    int a = 0;
private:
    int b = 0;
public:
    int c = 0;
};

struct SingleBlock {        // 所有資料成員在同一區塊
public:
    int a = 0;
    int b = 0;
    int c = 0;
};

void demoLayoutImpact() {
    cout << "  Scattered   (資料成員跨 2 個區塊): is_standard_layout = "
         << boolalpha << is_standard_layout<Scattered>::value
         << ", sizeof = " << sizeof(Scattered) << endl;
    cout << "  SingleBlock (資料成員同一區塊)  : is_standard_layout = "
         << is_standard_layout<SingleBlock>::value
         << ", sizeof = " << sizeof(SingleBlock) << endl;
    cout << "  ★ 兩者 sizeof 相同，但只有 SingleBlock 能安全地與 C struct 對應" << endl;
    cout << "    （sizeof 值為本機 x86-64 實測，屬實作定義）" << endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】設定物件：把介面與實作細節分成清楚的兩段
//   情境：資料庫連線設定。它同時有「對外的操作」與「內部的驗證狀態」，
//         正好示範實務上該怎麼排 access specifier。
//   為什麼用到本主題：對照本檔那種 public/private 交錯五次的寫法，
//         這裡採用業界通行的排法 ——
//           public   段：對外介面，讀者第一眼就看完
//           private  段：實作細節與資料成員，全部集中在一處
//         好處有二：(a) 可讀性；(b) 資料成員同區塊，
//         保留了 standard-layout 相關的性質與明確的排列順序。
// -----------------------------------------------------------------------------
class DbConfig {
public:
    // ── 對外介面：讀者看這一段就夠 ──────────────────────────
    DbConfig(string host, int port)
        : m_host(std::move(host)), m_port(port) {
        normalize();
    }

    bool isValid() const { return m_valid; }

    string dsn() const {
        if (!m_valid) return "<invalid>";
        return m_host + ":" + to_string(m_port);
    }

    string diagnosis() const { return m_reason; }

private:
    // ── 實作細節：資料成員全部集中在同一個 private 區塊 ──────
    void normalize() {
        if (m_host.empty()) {
            m_valid = false;
            m_reason = "host 不可為空";
            return;
        }
        if (m_port <= 0 || m_port > 65535) {
            m_valid = false;
            m_reason = "port 必須落在 1~65535，收到 " + to_string(m_port);
            return;
        }
        m_valid  = true;
        m_reason = "ok";
    }

    string m_host;
    int    m_port  = 0;
    bool   m_valid = false;
    string m_reason;
};

int main() {
    cout << "=== 基本：三個 public 區塊都合法 ===" << endl;
    Demo d;
    d.func1();    // ✅
    d.func2();    // ✅
    d.func3();    // ✅
    // d.data1;   // ❌ private
    // d.data2;   // ❌ private

    cout << "\n=== 拆散 access 區塊的實質代價 ===" << endl;
    demoLayoutImpact();

    cout << "\n=== 日常實務：設定物件的標準排法 ===" << endl;
    DbConfig good("db.internal.lan", 5432);
    cout << "  good  -> valid=" << boolalpha << good.isValid()
         << ", dsn=" << good.dsn() << ", 診斷=" << good.diagnosis() << endl;

    DbConfig badPort("db.internal.lan", 70000);
    cout << "  badPort -> valid=" << badPort.isValid()
         << ", dsn=" << badPort.dsn() << ", 診斷=" << badPort.diagnosis() << endl;

    DbConfig noHost("", 5432);
    cout << "  noHost  -> valid=" << noHost.isValid()
         << ", dsn=" << noHost.dsn() << ", 診斷=" << noHost.diagnosis() << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 11 課：存取修飾符——public、private、protected8.cpp" -o access8

// === 預期輸出 ===
// === 基本：三個 public 區塊都合法 ===
// func1
// func2, data1=0
// func3, data1=0, data2=0
// 
// === 拆散 access 區塊的實質代價 ===
//   Scattered   (資料成員跨 2 個區塊): is_standard_layout = false, sizeof = 12
//   SingleBlock (資料成員同一區塊)  : is_standard_layout = true, sizeof = 12
//   ★ 兩者 sizeof 相同，但只有 SingleBlock 能安全地與 C struct 對應
//     （sizeof 值為本機 x86-64 實測，屬實作定義）
// 
// === 日常實務：設定物件的標準排法 ===
//   good  -> valid=true, dsn=db.internal.lan:5432, 診斷=ok
//   badPort -> valid=false, dsn=<invalid>, 診斷=port 必須落在 1~65535，收到 70000
//   noHost  -> valid=false, dsn=<invalid>, 診斷=host 不可為空
