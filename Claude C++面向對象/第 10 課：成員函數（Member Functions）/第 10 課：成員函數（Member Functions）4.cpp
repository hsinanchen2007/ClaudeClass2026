// =============================================================================
//  第 10 課：成員函數 4  —  預設引數（Default Arguments）
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  void f(T a, U b = 預設值);      // 預設值寫在**宣告**處
//   標準：  C++98 起即有。
//   標頭檔：<iostream>、<string>
//   核心規則：預設引數只能從**右往左**連續給；一旦某個參數有預設值，
//             它右邊的所有參數都必須也有預設值。
//
// 【詳細解釋 Explanation】
//
// 【1. 預設引數是「呼叫端補參數」，不是「另一個函式」】
//   這是最關鍵的認知。log(msg, level = "INFO") 從頭到尾**只有一個函式**。
//   當你寫 logger.log("啟動") 時，是**編譯器在呼叫端**幫你把
//   "INFO" 填進去，實際產生的呼叫是 log("啟動", "INFO")。
//   對比重載：重載是真的存在多個函式、多個符號；
//   預設引數只有一個符號，差別在呼叫端的引數填充。
//   ★ 這個區別會直接導致下面【面試題】的 virtual 陷阱。
//
// 【2. 為什麼只能從右往左給】
//   因為 C++ 的引數是**位置**對應的，沒有具名引數（named arguments）。
//   若允許 void f(int a = 1, int b)，那 f(5) 中的 5 要算 a 還是 b？無解。
//   所以標準規定有預設值的參數必須是尾端連續的一段。
//   本檔 divider(char ch = '-', int repeat = 40) 兩個都有預設值，
//   於是可以呼叫 divider()、divider('=')、divider('*', 20) 三種形式；
//   但**不能**只指定 repeat 而讓 ch 用預設 —— 這正是 C++ 沒有具名引數的代價。
//   繞道做法：改用重載，或（現代做法）傳一個 options struct 進去。
//
// 【3. 預設值寫在宣告還是定義？】
//   規則：**寫在宣告處**（通常是 header 裡的類內宣告）。
//   同一個 translation unit 內不可以在宣告與定義**重複**寫同一個預設值，
//   否則是編譯錯誤。若類內只宣告、類外定義，預設值只能放類內那份。
//
// 【4. 預設值在什麼時候求值】
//   預設引數運算式是在**每次呼叫時**求值，不是在編譯期算好存起來。
//   所以 void f(int t = currentTime()) 每次呼叫都會重新叫一次 currentTime()。
//   但要注意它是在**呼叫端的 scope** 求值 —— 因此預設值運算式
//   不能引用其他參數，也不能用非 static 的成員變數。
//
// 【概念補充 Concept Deep Dive】
//   預設引數與重載在「呼叫端看起來一樣」，但編譯產物完全不同：
//     - 預設引數：一個符號。改預設值 → **必須重編所有呼叫端**，
//       因為填值的動作發生在呼叫端。若只重編了函式庫而沒重編使用者程式，
//       舊的預設值會殘留 —— 這是共享函式庫（.so）的 ABI 陷阱之一。
//     - 重載：多個符號，各自獨立。
//   這也是為什麼許多函式庫（如 Qt）在公開 API 上對預設引數相當保守。
//
//   另一個實務細節：預設引數不參與**函式型別**。
//   void f(int, int = 0) 的型別仍然是 void(int, int)，
//   所以取函式指標時必須提供全部引數，不能靠預設值：
//       void (*p)(int) = &f;   // ❌ 型別不合，編譯錯誤
//
// 【注意事項 Pay Attention】
//   1. 預設引數 + 重載極易 ambiguous：
//      同時有 f(int) 與 f(int, int = 0)，呼叫 f(1) 兩者皆可行 → 編譯錯誤。
//   2. **virtual 函式的預設引數依「靜態型別」決定**，
//      而函式本體依「動態型別」決定 —— 兩者會對不起來（見面試題陷阱）。
//      實務準則：virtual 函式不要用預設引數。
//   3. 預設值是每次呼叫求值，別放有副作用或昂貴的運算式。
//   4. 修改公開 API 的預設值等同於**行為變更**，且需要所有呼叫端重編才生效。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】預設引數
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 預設引數和函數重載有什麼本質差別？
//     答：預設引數只有**一個函式、一個符號**，由編譯器在呼叫端補上引數；
//         重載是**多個各自獨立的函式**，編譯期依型別決議挑一個。
//         因此改預設值需要重編所有呼叫端，改重載則不必。
//     追問：兩者可以並用嗎？
//         → 可以但很危險，極易造成 ambiguous：
//           f(int) 與 f(int, int = 0) 並存時，f(1) 就無法決議。
//
// 🔥 Q2. 預設引數為什麼只能從右邊開始給？想只指定後面的參數怎麼辦？
//     答：因為引數是位置對應的，C++ 沒有具名引數，
//         若中間允許缺省就無法判斷實際引數對應到哪個參數。
//         想跳著指定只能改用重載，或傳一個 options struct／builder。
//     追問：那 divider(int repeat) 這種「只想指定次數」的需求呢？
//         → 加一個重載 divider(int repeat) 即可，
//           但要小心它與 divider(char, int) 之間的隱式轉換是否會 ambiguous。
//
// ⚠️ 陷阱. 基底類別的 virtual f(int x = 10)，衍生類別覆寫成 f(int x = 20)，
//         透過 Base* 指向 Derived 物件呼叫 p->f()，會用哪個預設值？
//     答：用 **Base 的 10**，但執行的是 **Derived 的函式本體**。
//         因為預設引數在編譯期依**靜態型別**（Base*）填入，
//         而虛擬呼叫在執行期依**動態型別**（Derived）分派 —— 兩者分屬不同階段。
//     為什麼會錯：大家預期「既然叫到 Derived 的版本，就整組都用 Derived 的」，
//         把預設引數也想成是函式的一部分。但它其實是呼叫端的語法糖，
//         根本沒進到 vtable。準則：**virtual 函式不要給預設引數**。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
using namespace std;

class Logger {
public:
    string name = "APP";

    // level 有預設值 "INFO", 這裡的 level 參數有一個預設值 "INFO"，這意味著如果在調用 log 函數時沒有提供 
    // level 參數，則會自動使用 "INFO" 作為默認值。
    void log(const string& message, const string& level = "INFO") {
        cout << "[" << level << "] " << name << ": " << message << endl;
    }

    // repeat 有預設值 1, ch 有預設值 '-', 這裡的 divider 函數有兩個參數 ch 和 repeat，分別有預設值 '-' 
    // 和 40。這意味著如果在調用 divider 函數時沒有提供這些參數，則會使用這些預設值。
    void divider(char ch = '-', int repeat = 40) {
        for (int i = 0; i < repeat; i++) {
            cout << ch;
        }
        cout << endl;
    }
};

// -----------------------------------------------------------------------------
// 【可執行示範】virtual + 預設引數的陷阱（實測，不是推論）
//   Base::speak(int n = 1)，Derived::speak(int n = 3)。
//   透過 Base* 呼叫 p->speak() 時：
//     - 預設值 n 由**靜態型別 Base*** 決定 → 填入 1
//     - 函式本體由**動態型別 Derived** 決定 → 執行 Derived 的版本
//   於是印出「Derived 說 1 次」這種兩邊各半的結果。
// -----------------------------------------------------------------------------
class Base {
public:
    virtual ~Base() = default;
    virtual void speak(int times = 1) {
        cout << "  Base::speak，times=" << times << endl;
    }
};

class Derived : public Base {
public:
    void speak(int times = 3) override {
        cout << "  Derived::speak，times=" << times << endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】為什麼真實專案常用 options struct 取代一長串預設引數
//   情境：發 HTTP 請求，可調的東西有 timeout、重試次數、是否驗證 TLS、User-Agent。
//   若用預設引數寫成
//       request(url, timeout = 30, retries = 3, verifyTls = true, ua = "app/1.0")
//   就會遇到本檔講的限制：**想只改最後的 ua，前面三個全都得手動再寫一次**，
//   而且呼叫端出現 request(u, 30, 3, true, "bot/2.0") 這種
//   「一排看不出誰是誰」的 magic value。
//   解法：把選項打包成一個 struct，用 designated initializer（C++20）
//         或直接逐項指定，只填想改的欄位。
//   ★ 這裡用 C++17 相容寫法（先建 struct 再改欄位），
//     避免依賴 C++20 的 designated initializer。
// -----------------------------------------------------------------------------
struct RequestOptions {
    int    timeoutSec = 30;
    int    retries    = 3;
    bool   verifyTls  = true;
    string userAgent  = "app/1.0";
};

void sendRequest(const string& url, const RequestOptions& opt = RequestOptions{}) {
    cout << "GET " << url << endl;
    cout << "   timeout=" << opt.timeoutSec << "s"
         << ", retries=" << opt.retries
         << ", verifyTls=" << (opt.verifyTls ? "true" : "false")
         << ", UA=" << opt.userAgent << endl;
}

int main() {
    cout << "=== 基本：預設引數的三種呼叫形式 ===" << endl;
    Logger logger;
    logger.name = "MyApp";

    logger.divider();                  // 使用全部預設值
    logger.log("程式啟動");            // level 使用預設值 "INFO"
    logger.log("連線失敗", "ERROR");   // 指定 level
    logger.log("嘗試重連", "WARN");
    logger.divider('=', 40);           // 指定 ch，使用預設 repeat
    logger.log("重連成功");
    logger.divider('*', 20);           // 全部指定

    cout << "\n=== 陷阱：virtual 函式的預設引數看靜態型別 ===" << endl;
    Derived d;
    Base* p = &d;
    cout << "透過 Base* 呼叫 p->speak():" << endl;
    p->speak();          // ⚠️ 本體是 Derived 的，預設值卻是 Base 的 1
    cout << "直接用 Derived 物件呼叫 d.speak():" << endl;
    d.speak();           // 靜態型別就是 Derived → 預設值 3
    cout << "→ 同一個函式本體，預設值卻不同：這就是不該混用兩者的原因" << endl;

    cout << "\n=== 日常實務：options struct 取代長串預設引數 ===" << endl;
    sendRequest("https://api.example.com/health");        // 全部用預設

    RequestOptions opt;                 // 只想改 userAgent，其餘留預設
    opt.userAgent = "healthcheck-bot/2.0";
    sendRequest("https://api.example.com/health", opt);

    RequestOptions slow;                // 內網服務：拉長 timeout、關掉 TLS 驗證
    slow.timeoutSec = 120;
    slow.verifyTls  = false;
    sendRequest("https://internal.lan/report", slow);

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 10 課：成員函數（Member Functions）4.cpp" -o member4

// === 預期輸出 ===
// === 基本：預設引數的三種呼叫形式 ===
// ----------------------------------------
// [INFO] MyApp: 程式啟動
// [ERROR] MyApp: 連線失敗
// [WARN] MyApp: 嘗試重連
// ========================================
// [INFO] MyApp: 重連成功
// ********************
//
// === 陷阱：virtual 函式的預設引數看靜態型別 ===
// 透過 Base* 呼叫 p->speak():
//   Derived::speak，times=1
// 直接用 Derived 物件呼叫 d.speak():
//   Derived::speak，times=3
// → 同一個函式本體，預設值卻不同：這就是不該混用兩者的原因
//
// === 日常實務：options struct 取代長串預設引數 ===
// GET https://api.example.com/health
//    timeout=30s, retries=3, verifyTls=true, UA=app/1.0
// GET https://api.example.com/health
//    timeout=30s, retries=3, verifyTls=true, UA=healthcheck-bot/2.0
// GET https://internal.lan/report
//    timeout=120s, retries=3, verifyTls=false, UA=app/1.0
