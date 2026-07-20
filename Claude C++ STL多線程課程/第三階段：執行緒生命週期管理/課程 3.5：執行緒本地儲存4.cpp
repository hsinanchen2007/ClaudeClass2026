// =============================================================================
//  課程 3.5：執行緒本地儲存4.cpp  —  thread_local:每個執行緒一份的錯誤碼
// =============================================================================
//
// 【主題資訊 Information】
//   語法      : thread_local T name;
//   標準版本  : C++11(關鍵字);C++20 起可搭配 constinit 保證常數初始化
//   標頭檔    : 不需要 —— thread_local 是語言關鍵字,不是函式庫設施
//   儲存期    : thread storage duration(執行緒儲存期)
//   可宣告於  : namespace 範圍、類別的 static 資料成員、函式內的區域變數
//   生命週期  : 執行緒開始時建立 → 執行緒結束時銷毀,每個執行緒各自一份
//   存取成本  : 平凡型別幾乎為零(見【概念補充】的實測反組譯)
//
// 【詳細解釋 Explanation】
//
// 【1. thread_local 解決什麼問題】
// 多執行緒程式最常見的共享狀態衝突,不是來自「刻意共享的資料」,而是來自
// 「當年單執行緒時代隨手寫的全域變數」。典型代表就是 C 的 errno:
//
//     int errno;                       // 假設是單純全域變數
//     if (fopen(...) == NULL) {
//         printf("%d\n", errno);       // 兩個執行緒同時失敗 → 互相覆寫
//     }
//
// 錯誤碼有個本質特性:它描述的是「呼叫它的那個執行緒剛剛做了什麼」,
// 語意上根本就屬於執行緒,而不屬於整個行程。thread_local 把這個語意
// 直接寫進型別系統 —— 每個執行緒讀寫的是自己的那一份,天生沒有 data race,
// 也不需要任何 mutex。
//
// 這正是本檔示範的模式:setError()/getError() 看起來在操作「全域變數」,
// 但每個執行緒看到的其實是不同的物件。
//
// 【2. 真正的 errno 就是這樣做的】
// 現代 glibc 的 errno 不是全域變數,而是一個巨集:
//     #define errno (*__errno_location())
// __errno_location() 回傳呼叫端執行緒專屬的位址。換句話說,C 標準函式庫
// 早在 C++11 之前就用「執行緒區域儲存」解決了這個問題,只是實作藏在函式後面。
// thread_local 把這個技巧變成語言層級、任何人都能用的設施。
//
// 【3. 四種儲存期的完整座標】
//   static    儲存期:程式開始 → 程式結束        全程式一份
//   thread    儲存期:執行緒開始 → 執行緒結束    每執行緒一份   ← 本課
//   automatic 儲存期:進入區塊 → 離開區塊        每次呼叫一份
//   dynamic   儲存期:new → delete               由程式設計師決定
//
// 記憶法:thread_local 是「把 static 的『一份』從行程降級成執行緒」。
//
// 【4. 初始化與銷毀的時機】
//   * 常數初始化(如本檔 int lastError = 0):在執行緒啟動前就備妥,零成本。
//   * 動態初始化(需要跑建構子的型別):標準規定「在該執行緒中第一次
//     odr-use 之前」完成 —— 也就是惰性初始化,不是執行緒一啟動就跑。
//     這條規則非常重要,本課第 7 個範例檔會示範「從沒被用到 → 從沒被建構」。
//   * 銷毀:執行緒結束時,以「與建構相反的順序」銷毀該執行緒的所有
//     thread_local 物件,時機與 C 的 pthread_key 解構器相同。
//
// 【5. thread_local vs 傳參數 vs context 物件】
//   thread_local 的本質是「隱式傳遞的參數」。優點是不必改動整條呼叫鏈的
//   簽名(想想 setError 被 30 層深的函式呼叫);缺點也正是如此 —— 資料流
//   變成隱式的,不看實作就不知道函式依賴了什麼。
//   實務準則:
//     * 錯誤碼、log 的 trace id、per-thread 快取/緩衝區 → 適合 thread_local
//     * 業務資料、需要被測試替換的相依 → 老實用參數或 context 物件傳
//
// 【概念補充 Concept Deep Dive】
//
// (A) 記憶體佈局:TLS 區塊掛在執行緒控制區(TCB)上
//   x86-64 Linux 上,每個執行緒有一塊自己的 TLS 區段,%fs 暫存器指向它。
//   存取 thread_local 變數 = 「%fs + 該變數的固定位移」。
//
// (B) 本機實測(GCC 15.2.0, x86-64, -O2, 變數在主執行檔內)
//   thread_local int a; 的讀取,反組譯後只有一道指令:
//       movl  %fs:a@tpoff, %eax
//   也就是說「平凡型別的 thread_local 讀取」和「一般全域變數讀取」
//   成本幾乎相同,沒有函式呼叫、沒有鎖。這就是本檔能安心把 lastError
//   放在熱路徑上的理由。
//
//   但換成需要跑建構子的型別(如 thread_local std::string),每次存取都會
//   多一道初始化守衛檢查:
//       cmpb  $0, %fs:__tls_guard@tpoff
//   編譯器還會另外產生一個 __tls_init 函式。這就是「動態初始化的
//   thread_local 不是零成本」的具體來源。
//
// (C) 四種 TLS 存取模型(由連結方式決定,不是由你選)
//   local-exec    : 主執行檔內的 thread_local,直接 %fs+常數位移(最快)
//   initial-exec  : 程式啟動時就載入的 .so 內的變數
//   local-dynamic : 同一模組內多個 TLS 變數共用一次位址解析
//   global-dynamic: dlopen() 進來的 .so,需要呼叫 __tls_get_addr()(最慢)
//   結論:同一段程式碼搬進動態載入的外掛,TLS 存取成本會變高。
//
// (D) 為什麼 thread_local 不需要 mutex
//   因為根本沒有共享。mutex 解決的是「多個執行緒存取同一個物件」,
//   而 thread_local 從源頭就讓每個執行緒有各自的物件 —— 這是
//   「消除共享」而非「保護共享」,是效能最好的並行策略。
//
// 【注意事項 Pay Attention】
// 1. thread_local 物件的位址在不同執行緒是不同的;把 &lastError 傳給
//    另一個執行緒,對方拿到的是「你的那一份」的位址,執行緒結束後即懸空。
// 2. 每個執行緒一份 → 執行緒很多時記憶體會乘上去。放大物件(例如 1MB 緩衝區)
//    在數百個執行緒的伺服器上,就是數百 MB。
// 3. thread_local 物件的解構子在執行緒結束時執行;若解構子裡再存取
//    「另一個已被銷毀的 thread_local」,是未定義行為。
// 4. detach 的執行緒若在行程結束時仍在跑,其 thread_local 解構子不保證被執行。
// 5. 函式內的 thread_local 隱含 static 的意思(不需要也不能寫成 auto)。
// 6. std::cout 的並行輸出:標準保證不會產生 data race,但「不保證整行不被
//    切開」。本檔因此用一個 mutex 保護輸出,否則會看到兩行字互相穿插
//    ——那是輸出交錯,不是 thread_local 失效,別誤判。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】thread_local 執行緒本地儲存
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. thread_local 變數什麼時候建立、什麼時候銷毀?
//     答：每個執行緒各有一份。常數初始化的在執行緒開始執行前就備妥;
//         需要跑建構子的則是惰性的 —— 標準只保證「在該執行緒第一次
//         odr-use 之前」完成。銷毀在執行緒結束時,順序與建構相反。
//     追問：如果某個執行緒從頭到尾都沒碰過這個變數呢?
//         → 那它在那個執行緒裡就可能從來不會被建構,解構子自然也不會執行。
//           (本課第 7 個範例檔正是這個情況,實測完全沒有建構訊息。)
//           但粒度要講清楚:GCC 是整個翻譯單元共用一個 __tls_guard,
//           碰了同檔案裡任何一個,同檔案的全部就會一起建構。
//
// 🔥 Q2. thread_local 需要加 mutex 保護嗎?為什麼?
//     答：不需要。mutex 是用來保護「被多個執行緒共享的物件」,
//         而 thread_local 讓每個執行緒擁有各自的物件,根本沒有共享,
//         也就沒有 data race。這是「消除共享」而不是「保護共享」。
//     追問：那它完全零成本嗎?
//         → 平凡型別接近零(本機實測是單一道 movl %fs:...@tpoff);
//           但需要動態初始化的型別,每次存取都會多一次初始化守衛檢查,
//           而且從 dlopen 的 .so 存取還要走 __tls_get_addr()。
//
// ⚠️ 陷阱. 「thread_local 就是每個執行緒一份,所以把它的位址傳給別的
//         執行緒用,也還是安全的。」哪裡錯了?
//     答：錯得很徹底。thread_local 保證的是「透過名字存取時,拿到的是
//         自己那一份」,一旦你取出位址並交給別的執行緒,對方就是在直接
//         存取你的那一份 —— 共享回來了,data race 也回來了。
//         更糟的是,你的執行緒一結束,那份物件就被銷毀,對方手上是懸空指標。
//     為什麼會錯：把 thread_local 想成一種「魔法保護」,以為它保護的是
//         物件本身;實際上它只是決定了「名字 → 哪一份物件」的對應關係,
//         保護力在取址的瞬間就結束了。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// 保護 std::cout,避免多執行緒輸出互相切開(見【注意事項】6)
std::mutex g_ioMutex;

// -----------------------------------------------------------------------------
// 原始示範:每個執行緒一份的錯誤碼
// -----------------------------------------------------------------------------
thread_local int lastError = 0;

void setError(int code) {
    lastError = code;
}

int getError() {
    return lastError;
}

void worker(int id) {
    setError(id * 100);
    std::lock_guard<std::mutex> lock(g_ioMutex);
    std::cout << "Thread " << id << " error: " << getError() << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】仿 errno 的執行緒區域錯誤狀態(設定檔解析器)
//   情境: 解析器的每個階段都可能失敗,但我們不想讓每個函式都回傳
//         error code、也不想在 30 層呼叫鏈上多傳一個 out 參數。
//         用 thread_local 存「本執行緒最後一次錯誤」,呼叫端隨時可查。
//   為什麼用本主題: 錯誤狀態天生屬於「執行這件事的那個執行緒」。
// -----------------------------------------------------------------------------
enum class ParseError { None = 0, EmptyLine, MissingEquals, EmptyKey };

thread_local ParseError t_lastParseError = ParseError::None;

const char* parseErrorText(ParseError e) {
    switch (e) {
        case ParseError::None:          return "OK";
        case ParseError::EmptyLine:     return "空行";
        case ParseError::MissingEquals: return "缺少 '=' 分隔符";
        case ParseError::EmptyKey:      return "鍵名為空";
    }
    return "未知錯誤";
}

// 解析 "key=value";失敗時回傳 false 並設定 thread_local 錯誤碼
bool parseConfigLine(const std::string& line, std::string& key, std::string& value) {
    t_lastParseError = ParseError::None;

    if (line.empty()) {
        t_lastParseError = ParseError::EmptyLine;
        return false;
    }
    std::size_t eq = line.find('=');
    if (eq == std::string::npos) {
        t_lastParseError = ParseError::MissingEquals;
        return false;
    }
    if (eq == 0) {
        t_lastParseError = ParseError::EmptyKey;
        return false;
    }
    key   = line.substr(0, eq);
    value = line.substr(eq + 1);
    return true;
}

void configWorker(int id, const std::vector<std::string>& lines) {
    int ok = 0;
    std::string lastFailure;

    for (const std::string& line : lines) {
        std::string k, v;
        if (parseConfigLine(line, k, v)) {
            ++ok;
        } else {
            // 注意:這裡讀到的一定是「本執行緒」剛剛設下的錯誤碼,
            // 即使另一個執行緒同時也在解析、也在寫它自己的那一份。
            lastFailure = std::string("[") + line + "] " + parseErrorText(t_lastParseError);
        }
    }

    std::lock_guard<std::mutex> lock(g_ioMutex);
    std::cout << "  解析器 #" << id << " 成功 " << ok << " 行,最後一個錯誤: "
              << lastFailure << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】每執行緒的請求追蹤 ID(伺服器 log 關聯)
//   情境: 一個請求進來由某個 worker thread 處理,期間會呼叫十幾個模組。
//         我們希望所有 log 都自動帶上同一個 trace id,才能在 log 系統裡
//         把同一個請求的所有紀錄串起來 —— 但不想改每個函式的簽名。
//   為什麼用本主題: trace id 的作用域正好就是「這條執行緒正在處理的請求」。
// -----------------------------------------------------------------------------
thread_local std::string t_traceId = "-";

void logLine(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_ioMutex);
    std::cout << "  [trace=" << t_traceId << "] " << msg << std::endl;
}

// 深層模組:完全不知道 trace id 的存在,卻能印出正確的 id
void chargeCreditCard() { logLine("扣款完成"); }
void sendReceipt()      { logLine("寄出收據"); }

void handleRequest(const std::string& traceId) {
    t_traceId = traceId;          // 只在進入點設定一次
    logLine("開始處理訂單");
    chargeCreditCard();
    sendReceipt();
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔從缺,理由如下
//   LeetCode 的並行題(1114 Print in Order、1115 Print FooBar Alternately、
//   1116 Print Zero Even Odd、1117 Building H2O、1195 Fizz Buzz Multithreaded)
//   考的全是「多執行緒之間如何同步」,需要的是共享狀態 + mutex/條件變數/號誌。
//   而 thread_local 的用途恰恰相反 —— 它是用來「消除共享」的。
//   在那些題目裡放 thread_local,反而會讓執行緒彼此看不到對方的進度,
//   直接把題目做錯。硬湊一題不如誠實從缺,改以上面兩個真實情境示範。
// -----------------------------------------------------------------------------

int main() {
    std::cout << "=== 原始示範:每執行緒一份的錯誤碼 ===" << std::endl;
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    t1.join();
    t2.join();

    std::cout << "\n=== 主執行緒有自己的那一份(從未被 worker 影響) ===" << std::endl;
    std::cout << "主執行緒 lastError = " << getError()
              << "  (worker 寫的 100/200 完全沒有外洩過來)" << std::endl;

    std::cout << "\n=== 實務 1:執行緒區域的設定檔解析錯誤碼 ===" << std::endl;
    std::vector<std::string> fileA = {"host=127.0.0.1", "port 8080", "debug=true"};
    std::vector<std::string> fileB = {"=novalue", "timeout=30"};
    std::thread p1(configWorker, 1, std::cref(fileA));
    std::thread p2(configWorker, 2, std::cref(fileB));
    p1.join();
    p2.join();

    std::cout << "\n=== 實務 2:每執行緒的請求追蹤 ID ===" << std::endl;
    std::thread r1(handleRequest, "req-1001");
    std::thread r2(handleRequest, "req-1002");
    r1.join();
    r2.join();
    std::cout << "主執行緒的 t_traceId 仍是預設值: " << t_traceId << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.5：執行緒本地儲存4.cpp" -o tls_error

// 注意:以下為某一次實際執行的結果。兩個執行緒之間「哪一行先印出」
//       每次執行都可能不同(例如 Thread 2 可能排在 Thread 1 之前);
//       但每一行的「內容」是確定的 —— 這正是 thread_local 的保證。

// === 預期輸出 ===
// === 原始示範:每執行緒一份的錯誤碼 ===
// Thread 1 error: 100
// Thread 2 error: 200
//
// === 主執行緒有自己的那一份(從未被 worker 影響) ===
// 主執行緒 lastError = 0  (worker 寫的 100/200 完全沒有外洩過來)
//
// === 實務 1:執行緒區域的設定檔解析錯誤碼 ===
//   解析器 #1 成功 2 行,最後一個錯誤: [port 8080] 缺少 '=' 分隔符
//   解析器 #2 成功 1 行,最後一個錯誤: [=novalue] 鍵名為空
//
// === 實務 2:每執行緒的請求追蹤 ID ===
//   [trace=req-1001] 開始處理訂單
//   [trace=req-1001] 扣款完成
//   [trace=req-1001] 寄出收據
//   [trace=req-1002] 開始處理訂單
//   [trace=req-1002] 扣款完成
//   [trace=req-1002] 寄出收據
// 主執行緒的 t_traceId 仍是預設值: -
