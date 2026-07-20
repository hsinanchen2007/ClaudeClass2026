// =============================================================================
//  第 22 課：const 成員函數 3  —  const 正確性(const correctness)
// =============================================================================
//
// 【主題資訊 Information】
//   核心對照:
//     string getName()        { return name_; }   // ❌ 漏 const → const 物件不可用
//     const string& getName() const { return name_; }  // ✅ const 正確
//   標準版本:C++98 起即有;本檔另用到 C++17 的 [[maybe_unused]] 屬性。
//   複雜度:const 為編譯期概念,零執行期成本。
//   標頭檔:<string>
//
// 【詳細解釋 Explanation】
//
// 【1. 什麼叫「const 正確」】
//   一個型別是 const 正確的,意思是:凡是不改變物件可觀察狀態的成員函式,
//   全部都標了 const。它不是風格偏好,而是「這個型別能不能被正常使用」的
//   前提條件 —— 因為 C++ 生態(標準庫、多執行緒程式、大多數 API)
//   預設就是以 const& 傳遞物件。
//
// 【2. 漏一個 const 的實際代價:本檔 BadDesign 的下場】
//   BadDesign 的 getName()/getValue()/print() 都沒有加 const。
//   後果是:processBad(const BadDesign&) 這個函式裡,
//   什麼有意義的事都做不了 —— 三個查詢函式全部不能呼叫。
//   注意這不是「效能差一點」或「風格不佳」,而是整個型別在
//   const 語境下完全不可用。這就是為什麼 const 正確性被視為
//   介面設計的基本功,而不是可選的加分項。
//
// 【3. 為什麼會「傳染」】
//   假設某個底層函式漏了 const,呼叫它的中層函式就不能是 const,
//   再上層也跟著不能是 const,一路傳染到最上層。當你終於發現、
//   想從最上層補 const 時,會發現要一路補到底才編得過。
//   反過來說,只要一開始就從底層做對,整條鏈自然都是 const 正確的。
//
// 【4. 修法的順序:由內而外,而不是由外而內】
//   遇到 const 相關的連鎖編譯錯誤,正確做法是從最底層的
//   「本來就該是 const 卻沒標」的函式開始補。
//   錯誤做法是在最上層加 const_cast 把 const 拿掉 —— 那只是把
//   編譯期能抓到的錯誤,延後成執行期抓不到的錯誤。
//
// 【概念補充 Concept Deep Dive】
//   * BadDesign::getName() 回傳的是 string(值),GoodDesign 回傳
//     const string&(參考)。除了 const 正確性之外,這也順帶示範了
//     第 21 課的回傳方式取捨:前者每次呼叫都深拷貝一份字串。
//   * [[maybe_unused]] 是 C++17 引入的標準屬性,用來明確告訴編譯器
//     「這個參數刻意不使用,不要發出 -Wunused-parameter 警告」。
//     在 C++17 之前的常見做法是寫 (void)b; 或乾脆省略參數名稱。
//     本檔用它來保留參數名(讓讀者看得懂那個 b 是什麼),同時保持零警告。
//   * const 正確性也是多執行緒安全的基礎:標準庫保證
//     「同時對同一物件呼叫 const 成員函式」是安全的(不會有資料競爭),
//     前提是該型別確實遵守了 const 語意 —— 這也是 mutable 成員
//     (第 23 課)必須格外小心的原因。
//
// 【注意事項 Pay Attention】
//   1. 不要用 const_cast 來「修好」const 錯誤。若對本來就宣告為 const
//      的物件寫入,是未定義行為(UB),不保證任何特定結果。
//   2. 補 const 是介面變更。若該型別已被外部使用,補 const 只會
//      放寬可用範圍(原本能編的仍能編),因此通常是安全的變更。
//   3. 回傳型別的 const 與函式的 const 是兩件事,兩個都要想清楚:
//      const string& getName() const;
//      ~~~~~ 呼叫端不能改回傳值      ~~~~~ 這個函式不改 *this
//   4. 本檔 processBad() 只是印一行字,那正是它的重點 ——
//      對一個 const 不正確的型別,以 const& 收下之後真的無事可做。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】const 正確性
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是 const correctness?為什麼它被當成基本功而不是加分項?
//     答：凡是不改變物件可觀察狀態的成員函式都標上 const,就叫 const 正確。
//         它是基本功,因為 C++ 生態預設以 const& 傳遞物件:標準庫演算法、
//         多執行緒程式、絕大多數 API 都是如此。一個 const 不正確的型別,
//         在這些語境下等於完全不能用 —— 不是效率差,是根本無法呼叫。
//     追問：補 const 會不會破壞既有使用者?→ 幾乎不會。補 const 只會
//         放寬可呼叫的範圍,原本能編譯的程式碼仍然能編譯。
//
// 🔥 Q2. 遇到一整串 const 相關的編譯錯誤,正確的修法是什麼?
//     答：由內而外 —— 從最底層那個「本來就該是 const 卻漏標」的函式開始補,
//         一路往上,整條呼叫鏈就會自然通過。
//         絕對不要在最上層用 const_cast 把 const 拿掉硬過,
//         那是把編譯期抓得到的錯誤,換成執行期抓不到的錯誤。
//     追問：const_cast 什麼時候才是正當用途?→ 主要是為了呼叫
//         「介面忘了加 const 的舊有 C API」,且你確知它實際上不會修改資料。
//
// ⚠️ 陷阱. 「我這個 getter 只是回傳成員,加不加 const 不影響行為,先不加吧。」
//     答：行為確實不變,但「可呼叫性」變了 —— 而那才是重點。
//         漏 const 的 getter,對 const 物件、const 參考參數、
//         以及任何以 const& 收下你物件的函式而言,全部無法呼叫。
//         本檔 processBad() 就是實證:一個只差幾個 const 的類別,
//         在 const 語境下三個查詢函式一個都用不了。
//     為什麼會錯：只看「函式做了什麼」,沒看「誰能呼叫它」。
//         const 不改變函式的行為,它改變的是函式的可用範圍;
//         漏標 const 等於自願把型別的使用場景砍掉一大半。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 從缺,理由如下
//   const 正確性是介面契約議題,LeetCode 判題只執行程式碼、不檢查契約。
//   本課 summary.cpp 會以 705. Design HashSet 說明「查詢 vs 修改」
//   的介面切分如何自然對應到 const / 非 const,在此不重複掛題。
//
// =============================================================================

#include <iostream>
#include <string>
using namespace std;

// ===== 反面教材：忘記加 const =====
class BadDesign {
private:
    string name_;
    int value_;

public:
    BadDesign(const string& n, int v) : name_(n), value_(v) {}

    // 忘記加 const！這些函數明明不修改對象
    // 這會導致無法在 const 對象上調用這些函數，限制了它們的使用場景
    // 這裡的 getName()、getValue() 和 print() 函數都應該是 const 成員函數，但它們缺少 const 修飾符
    string getName() { return name_; }       // 缺少 const
    int getValue() { return value_; }        // 缺少 const
    void print() { cout << name_ << ":" << value_ << endl; } // 缺少 const
};

// ===== 正確設計：const 正確 =====
class GoodDesign {
private:
    string name_;
    int value_;

public:
    GoodDesign(const string& n, int v) : name_(n), value_(v) {}

    // 所有不修改對象的函數都加 const
    // 這允許在 const 對象上調用這些函數，並且編譯器會強制執行這一承諾
    // 這裡的 getName()、getValue() 和 print() 函數都正確地標記為 const 成員函數，表示它們不修改對象的狀態
    const string& getName() const { return name_; }
    int getValue() const { return value_; }
    void print() const { cout << name_ << ":" << value_ << endl; }

    // 只有修改對象的函數才不加 const
    // 這裡的 setValue() 函數會修改 value_，所以不能是 const
    void setValue(int v) { value_ = v; }
};

// 需要 const 引用的函數
// 這個函數接受一個 const GoodDesign&，表示它只能「看」這個對象，但不能修改它
// 這裡的 processGood() 函數接受一個 const GoodDesign&，表示它只能「看」這個對象，但不能修改它
// 這裡的 processBad() 函數接受一個 const BadDesign&，但由於 BadDesign 的成員函數沒有標記為 const，所以幾乎什麼都做不了
// [[maybe_unused]]（C++17）：明確表示「這個參數刻意不使用」，
// 讓 -Wall -Wextra 不再發出 -Wunused-parameter 警告。
// 保留參數名 b 是為了讓讀者看得懂它是什麼 —— 而它「無事可做」正是本例的重點。
void processBad([[maybe_unused]] const BadDesign& b) {
    // b.getName();   // ❌ 編譯錯誤！getName 不是 const
    // b.print();     // ❌ 編譯錯誤！print 不是 const
    cout << "  BadDesign：什麼都不能做！" << endl;
}

void processGood(const GoodDesign& g) {
    g.print();         // ✅ 完美
    cout << "  name = " << g.getName() << endl;  // ✅
    cout << "  value = " << g.getValue() << endl; // ✅
}

// -----------------------------------------------------------------------------
// 【日常實務範例】log 記錄與多個輸出格式化器
//   情境：同一筆 log 記錄要同時送給「主控台格式化器」「JSON 格式化器」
//         「告警判斷器」。這些消費端一律以 const LogRecord& 接收 ——
//         沒有任何一個該修改原始記錄。
//   重點：只要 LogRecord 是 const 正確的，要新增第 N 個消費端就毫無阻力；
//         反之若漏了 const，每加一個消費端都得回頭改型別。
//         這就是 const 正確性在真實專案裡的複利效果。
// -----------------------------------------------------------------------------
class LogRecord {
private:
    string timestamp_;
    string level_;      // DEBUG / INFO / WARN / ERROR
    string module_;
    string message_;
    int    errorCode_;

public:
    LogRecord(const string& ts, const string& lv, const string& mod,
              const string& msg, int code)
        : timestamp_(ts), level_(lv), module_(mod), message_(msg), errorCode_(code) {}

    // 查詢介面全部 const —— 這是它能被任意數量的消費端使用的前提
    const string& timestamp() const { return timestamp_; }
    const string& level()     const { return level_; }
    const string& module()    const { return module_; }
    const string& message()   const { return message_; }
    int  errorCode()          const { return errorCode_; }

    // 衍生查詢：const 函式呼叫其他 const 函式，完全合法
    bool isError()   const { return level() == "ERROR"; }
    bool needAlert() const { return isError() && errorCode() >= 500; }
};

// 消費端 1：人類可讀的主控台格式
static void formatConsole(const LogRecord& r) {
    cout << "    " << r.timestamp() << " [" << r.level() << "] "
         << r.module() << " - " << r.message();
    if (r.errorCode() != 0) cout << " (code=" << r.errorCode() << ")";
    cout << endl;
}

// 消費端 2：機器可讀的 JSON（實務上會送進 ELK / Loki 之類的系統）
static void formatJson(const LogRecord& r) {
    cout << R"(    {"ts":")" << r.timestamp()
         << R"(","level":")" << r.level()
         << R"(","module":")" << r.module()
         << R"(","code":)" << r.errorCode() << "}" << endl;
}

// 消費端 3：告警判斷（只讀，卻能做完整決策）
static void checkAlert(const LogRecord& r) {
    cout << "    告警判斷：" << (r.needAlert() ? "🔴 需要通知值班" : "無需告警") << endl;
}

int main() {
    cout << "=== const 正確性 ===" << endl;

    BadDesign bad("壞設計", 42);
    GoodDesign good("好設計", 42);

    cout << "\n--- 用 const 引用傳遞 ---" << endl;
    processBad(bad);      // 幾乎什麼都做不了
    processGood(good);    // 正常工作

    // ─────────────────────────────────────────────────────────
    cout << "\n=== 日常實務：一筆 log 餵給三個唯讀消費端 ===" << endl;

    const LogRecord errRec("2026-07-19T21:04:11Z", "ERROR", "payment",
                           "上游閘道逾時", 504);
    cout << "\n--- ERROR 記錄 ---" << endl;
    formatConsole(errRec);
    formatJson(errRec);
    checkAlert(errRec);

    const LogRecord infoRec("2026-07-19T21:04:12Z", "INFO", "auth",
                            "使用者登入成功", 0);
    cout << "\n--- INFO 記錄 ---" << endl;
    formatConsole(infoRec);
    formatJson(infoRec);
    checkAlert(infoRec);

    cout << "\n  三個消費端都只接 const LogRecord&，" << endl;
    cout << "  型別系統保證沒有任何一個會改到原始記錄。" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 22 課：const 成員函數3.cpp" -o l22_3
// 執行: ./l22_3        (rc=0)

// === 預期輸出 ===
// === const 正確性 ===
//
// --- 用 const 引用傳遞 ---
//   BadDesign：什麼都不能做！
// 好設計:42
//   name = 好設計
//   value = 42
//
// === 日常實務：一筆 log 餵給三個唯讀消費端 ===
//
// --- ERROR 記錄 ---
//     2026-07-19T21:04:11Z [ERROR] payment - 上游閘道逾時 (code=504)
//     {"ts":"2026-07-19T21:04:11Z","level":"ERROR","module":"payment","code":504}
//     告警判斷：🔴 需要通知值班
//
// --- INFO 記錄 ---
//     2026-07-19T21:04:12Z [INFO] auth - 使用者登入成功
//     {"ts":"2026-07-19T21:04:12Z","level":"INFO","module":"auth","code":0}
//     告警判斷：無需告警
//
//   三個消費端都只接 const LogRecord&，
//   型別系統保證沒有任何一個會改到原始記錄。
