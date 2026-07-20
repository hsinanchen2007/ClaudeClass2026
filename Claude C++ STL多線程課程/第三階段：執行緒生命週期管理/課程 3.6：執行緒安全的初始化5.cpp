// =============================================================================
//  課程 3.6：執行緒安全的初始化5.cpp  —  Magic Static:語言內建的執行緒安全初始化
// =============================================================================
//
// 【主題資訊 Information】
//   語法      : static T instance;        // 寫在函式內的區域 static 變數
//   標準版本  : C++11 起「保證執行緒安全」([stmt.dcl]/4);C++98 沒有這個保證
//   標頭檔    : 不需要 —— 這是語言規則,不是函式庫設施
//   俗名      : Magic Static / Meyers Singleton(Scott Meyers 推廣)
//   實作      : 編譯器產生 guard 變數 + __cxa_guard_acquire/release(本機實測)
//   解構      : 保證在程式結束時執行,順序與建構相反
//
// 【詳細解釋 Explanation】
//
// 【1. 標準到底保證了什麼】
// C++11 [stmt.dcl]/4 規定:當控制流程「並行地」進入一個需要動態初始化的
// 區域 static 變數的宣告時,並行的執行必須「等待」該初始化完成。
// 換句話說,底下這三行程式碼就已經是完整、正確的執行緒安全惰性單例:
//
//     static Singleton& getInstance() {
//         static Singleton instance;   // 只會被初始化一次,而且其他人會等
//         return instance;
//     }
//
// 不需要 once_flag、不需要 mutex、不需要 atomic。這是 C++11 送給大家的禮物 ——
// 在此之前,同樣的效果必須自己用雙重檢查鎖定寫,而且幾乎人人寫錯。
//
// 【2. 相對 call_once 版本的三個優勢】
//   (a) 更短:三行搞定,沒有 static 成員要在類別外定義,不會忘記那一行。
//   (b) 物件與初始化綁在一起:不會出現「旗標設好了但指標是 nullptr」
//       這種狀態不一致的可能。
//   (c) **保證解構**:instance 是有靜態儲存期的具名物件,程式結束時
//       一定會呼叫解構子,順序與建構相反。對照課程 3.6/4 的
//       new 版單例 —— 那個永遠不會被解構。
//       如果解構子有必要副作用(flush 檔案、關閉連線),這一點是決定性的。
//
// 【3. 什麼時候仍該用 call_once】
//   * 初始化的目標不是「單一個物件」,而是好幾樣東西(設定多個全域、
//     註冊多個 handler)。
//   * 初始化需要參數,而參數要由呼叫端傳入。
//   * 需要重試語意(初始化失敗後讓下一個執行緒再試)——
//     magic static 在建構子拋例外時也有重試語意,但寫法上比較難控制。
//   一般情況下,「單一物件的惰性初始化」優先選 magic static。
//
// 【4. 代價與限制】
//   * 每次呼叫 getInstance() 都會檢查一次 guard 變數(見【概念補充】A)。
//     這在絕大多數情況下微不足道,但它不是零成本。
//   * 解構順序仍可能出問題:若另一個 static 物件的解構子想使用這個單例,
//     而它已經先被銷毀,就是存取已銷毀物件(static destruction order fiasco)。
//     這是 magic static 唯一比「永不解構」版本脆弱的地方。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 編譯器實際產生了什麼(本機實測)
//   GCC 15.2.0 / x86-64 / -O2 下,把 static S s; 放進函式編譯後可見:
//       _ZGVZ3getvE1s              ← guard 變數(名稱以 _ZGV 開頭)
//       __cxa_guard_acquire        ← 嘗試取得初始化權
//       __cxa_guard_release        ← 初始化成功,標記完成
//       __cxa_guard_abort          ← 建構子拋例外時回復狀態
//   快速路徑只是讀 guard 變數的第一個位元組;只有第一次(或並行競爭時)
//   才會真的呼叫 __cxa_guard_acquire 進入慢速路徑。
//   這也說明了為什麼建構子拋例外後「下次還會再試」——
//   __cxa_guard_abort 會把 guard 還原成未初始化狀態。
//
// (B) -fno-threadsafe-statics
//   GCC/Clang 提供這個選項關掉上述保護。本機實測加上它之後,
//   組譯輸出裡的 __cxa_guard_* 呼叫數量歸零。
//   它存在的理由是嵌入式/單執行緒環境要省下那點空間與時間 ——
//   但在多執行緒程式裡開啟它,等於把 C++11 的保證整個拿掉,
//   magic static 會退回 C++98 那種有競態的行為。這是個危險選項。
//
// (C) 為什麼 C++98 沒有這個保證
//   C++98 根本沒有記憶體模型,標準裡沒有「執行緒」這個概念,
//   自然無法規定並行進入宣告時該怎麼辦。當年各家編譯器行為不一:
//   GCC 其實早就實作了執行緒安全版本,MSVC 則直到 VS2015 才支援。
//   所以在很舊的程式碼裡看到手寫的雙重檢查鎖定,不是作者愛炫技,
//   而是當年真的沒有別的選擇。
//
// 【注意事項 Pay Attention】
// 1. 這個保證是 C++11 才有的;跨足非常舊的編譯器(如 VS2013 以前)不成立。
// 2. -fno-threadsafe-statics 會關掉這個保證,多執行緒程式不可使用。
// 3. 保證的仍然只有「初始化」。物件建好後,會修改狀態的方法仍需自己同步。
// 4. 解構順序問題依然存在:別讓其他 static 物件的解構子依賴這個單例。
// 5. 建構子若拋例外,該次初始化不算完成,下次呼叫會再試一次。
//    若它每次都拋,就會每次都重試。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Magic Static(函式內 static 區域變數)
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 下面這個單例在多執行緒下安全嗎?為什麼?
//        static Singleton& getInstance() { static Singleton s; return s; }
//     答：C++11 起是安全的。標準規定並行進入一個需要動態初始化的區域
//         static 變數宣告時,其他執行緒必須等待初始化完成。
//         編譯器會產生 guard 變數與 __cxa_guard_acquire/release 來實現。
//     追問：C++98 呢?
//         → 沒有這個保證。C++98 連記憶體模型都沒有。當年 GCC 已自行實作,
//           但 MSVC 要到 VS2015 才支援,所以舊程式碼常見手寫雙重檢查鎖定。
//
// 🔥 Q2. magic static 和 call_once 版單例,最大的實務差別是什麼?
//     答：解構。magic static 的物件有靜態儲存期,程式結束時保證被解構,
//         順序與建構相反;而 new 出來的 call_once 版單例通常永遠不解構。
//         若解構子要 flush 檔案或送出關閉訊息,這個差別是決定性的。
//     追問：那什麼時候還是該用 call_once?
//         → 當要初始化的不是單一物件、或初始化需要呼叫端傳入參數時。
//
// ⚠️ 陷阱. 「既然 magic static 保證會被解構,那我在別的全域物件的解構子裡
//         使用這個單例,一定沒問題。」哪裡錯了?
//     答：不保證。全域/static 物件的解構順序是「與建構順序相反」,
//         而跨翻譯單元的建構順序本身就是未指定的。若那個全域物件的
//         解構子執行時,單例已經先被銷毀,你就是在使用已銷毀的物件 ——
//         未定義行為。這就是 static destruction order fiasco。
//     為什麼會錯：把「保證會被解構」誤讀成「保證在我之後才被解構」。
//         前者只承諾「會發生」,完全沒有承諾「順序」。
//         需要確定順序時,可用 nifty counter,或反過來刻意讓單例永不解構。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// 保護 std::cout,避免多執行緒輸出互相切開
std::mutex g_ioMutex;

void say(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_ioMutex);
    std::cout << msg << std::endl;
}

// -----------------------------------------------------------------------------
// 原始示範:magic static 單例
// -----------------------------------------------------------------------------
class Singleton {
    Singleton() { say("  Singleton 建立"); }

public:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

    static Singleton& getInstance() {
        static Singleton instance;  // C++11 保證執行緒安全
        return instance;
    }

    void doSomething() { say("  工作中"); }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】會被保證 flush 的稽核日誌(magic static 相對 new 版的關鍵優勢)
//   情境: 稽核日誌會先在記憶體裡累積,程式結束時必須寫入磁碟。
//         若用課程 3.6/4 那種 new 出來、永不解構的單例,
//         這個 flush 永遠不會發生 —— 資料就這樣靜靜地掉了。
//         改用 magic static,解構子保證會在程式結束時執行。
//   為什麼用本主題: 直接示範【詳細解釋 2】(c) 的實務後果,
//                   這是選擇 magic static 最有力的理由。
// -----------------------------------------------------------------------------
class AuditLog {
    std::mutex               mtx_;      // 保護 entries_,初始化安全 ≠ 使用安全
    std::vector<std::string> entries_;

    AuditLog() { say("    [audit] 稽核日誌啟動"); }

public:
    AuditLog(const AuditLog&) = delete;
    AuditLog& operator=(const AuditLog&) = delete;

    ~AuditLog() {
        // 這裡保證會被執行 —— 這正是選 magic static 的理由
        std::cout << "    [audit] 程式結束,flush " << entries_.size()
                  << " 筆稽核紀錄到磁碟" << std::endl;
    }

    static AuditLog& get() {
        static AuditLog instance;
        return instance;
    }

    void record(const std::string& what) {
        std::lock_guard<std::mutex> lock(mtx_);
        entries_.push_back(what);
    }

    std::size_t size() {
        std::lock_guard<std::mutex> lock(mtx_);
        return entries_.size();
    }
};

void auditWorker(int id, int times) {
    for (int i = 0; i < times; ++i) {
        AuditLog::get().record("thread " + std::to_string(id) + " 動作 " +
                               std::to_string(i));
    }
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔從缺,理由如下
//   LeetCode 的並行題(1114 Print in Order、1115 Print FooBar Alternately、
//   1116 Print Zero Even Odd、1117 Building H2O、1195 Fizz Buzz Multithreaded)
//   考的是執行緒間的交替與順序控制;magic static 解決的則是
//   「一個物件如何只被初始化一次」,兩者是不同的問題。
//   把單例塞進那些題目反而會讓多組測資之間互相污染(狀態殘留)。
//   因此誠實從缺,改以上面的稽核日誌情境示範最關鍵的解構保證差異。
// -----------------------------------------------------------------------------

int main() {
    std::cout << "=== 原始示範:兩條執行緒同時取得單例 ===" << std::endl;
    std::thread t1([]() { Singleton::getInstance().doSomething(); });
    std::thread t2([]() { Singleton::getInstance().doSomething(); });
    t1.join();
    t2.join();
    std::cout << "  ↑「Singleton 建立」只出現一次" << std::endl;

    std::cout << "\n=== 驗證是同一個物件 ===" << std::endl;
    std::cout << "  兩次 getInstance() 位址相同? " << std::boolalpha
              << (&Singleton::getInstance() == &Singleton::getInstance())
              << std::endl;

    std::cout << "\n=== 實務:4 條執行緒各寫 250 筆稽核紀錄 ===" << std::endl;
    std::vector<std::thread> pool;
    for (int i = 1; i <= 4; ++i) pool.emplace_back(auditWorker, i, 250);
    for (std::thread& t : pool) t.join();
    std::cout << "  累積筆數 = " << AuditLog::get().size() << " (期望 1000)"
              << std::endl;

    std::cout << "\n=== main 即將結束 ===" << std::endl;
    std::cout << "(下面那行 flush 訊息由解構子印出,發生在 main 回傳之後 ——"
                 "這正是 magic static 保證解構的證據)" << std::endl;
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.6：執行緒安全的初始化5.cpp" -o once_magic

// 注意:以下為某一次實際執行的結果。
//   * 兩條執行緒印出「工作中」的先後順序每次執行都可能不同。
//   * 不變的是:「Singleton 建立」恰好一行、稽核累積筆數恰好 1000,
//     且最後那行 flush 訊息一定出現在「=== main 即將結束 ===」之後。

// === 預期輸出 ===
// === 原始示範:兩條執行緒同時取得單例 ===
//   Singleton 建立
//   工作中
//   工作中
//   ↑「Singleton 建立」只出現一次
//
// === 驗證是同一個物件 ===
//   兩次 getInstance() 位址相同? true
//
// === 實務:4 條執行緒各寫 250 筆稽核紀錄 ===
//     [audit] 稽核日誌啟動
//   累積筆數 = 1000 (期望 1000)
//
// === main 即將結束 ===
// (下面那行 flush 訊息由解構子印出,發生在 main 回傳之後 ——這正是 magic static 保證解構的證據)
//     [audit] 程式結束,flush 1000 筆稽核紀錄到磁碟
