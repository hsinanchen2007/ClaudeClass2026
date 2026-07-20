// =============================================================================
//  課程 3.6：執行緒安全的初始化3.cpp  —  用 lambda 當 call_once 的初始化函式
// =============================================================================
//
// 【主題資訊 Information】
//   簽名      : void call_once(std::once_flag& flag, Callable&& f, Args&&... args);
//   標準版本  : C++11
//   標頭檔    : <mutex>
//   本檔重點  : 把初始化邏輯直接寫成 lambda,而不是另外命名一個函式
//   可呼叫物件: 一般函式、函式指標、lambda、functor、成員函式指標皆可
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼多數人會選 lambda 而不是具名函式】
// 上一個範例把初始化寫成獨立的 initDatabase()。那樣沒有錯,但有兩個小缺點:
//   * 那個函式只會被 call_once 用到一次,卻污染了整個命名空間 ——
//     別人看到它會以為可以直接呼叫(而直接呼叫正好會繞過保護)。
//   * 初始化邏輯和它保護的那個旗標分隔在兩個地方,閱讀時要來回跳。
// 寫成 lambda 就沒有這兩個問題:邏輯就在使用它的那一行旁邊,
// 而且是匿名的,沒有人能不小心繞過 call_once 直接呼叫它。
//
// 【2. lambda 讓「捕獲區域變數」變得可能】
// 具名函式只能透過全域變數或參數取得外部資訊;lambda 可以直接捕獲。
// 這在初始化需要用到「呼叫端才知道的資訊」時特別方便:
//     std::call_once(flag, [&]{ conn = connect(hostFromArgs, portFromArgs); });
// 不過捕獲也帶來生命週期問題,見【注意事項】2。
//
// 【3. call_once 對可呼叫物件的要求】
// 只要能被 INVOKE 的東西都可以:一般函式、函式指標、lambda、functor、
// 甚至成員函式指標 + 物件(本課第 7 個範例檔示範這種寫法)。
// 傳進去的額外參數會被「完美轉發」,所以移動語意也能正常運作。
//
// 【4. 這個範例真正示範的不變量】
// 兩條執行緒同時呼叫 initAndUse(),但輸出中「Database 初始化」只會出現一次,
// 「查詢中...」出現兩次。而且更重要的是:第二條執行緒的 db->query()
// 保證看到的是「已經完整建構好」的物件 —— 它會被阻塞到初始化完成為止,
// 不是「看到旗標已設就直接往下跑」。
//
// 【概念補充 Concept Deep Dive】
//
// (A) lambda 傳給 call_once 時發生了什麼
//   無捕獲的 lambda 會被轉換成一個沒有狀態的閉包物件;有捕獲的則是一個
//   含有捕獲成員的匿名類別物件。call_once 以轉發參考接收它,
//   不會複製到堆積上,所以這裡沒有額外的配置成本。
//
// (B) 為什麼 lambda 不能寫成「捕獲後回傳值」
//   call_once 的回傳型別是 void,初始化函式的回傳值會被丟棄。
//   想「取得初始化結果」的正確做法是讓 lambda 寫進外部變數(如本例的 db),
//   或直接改用函式內 static(magic static,見本課第 5 個範例檔)。
//
// 【注意事項 Pay Attention】
// 1. once_flag 必須是所有執行緒共用的「同一個」物件;
//    寫成區域變數等於每條執行緒各有一份旗標,保護完全失效。
// 2. lambda 若以參考捕獲區域變數,必須確保 call_once 執行期間那些變數還活著。
//    本例的 lambda 只碰全域的 db,沒有這個問題。
// 3. lambda 內拋出例外 ⇒ 旗標不算完成,下一個執行緒會再試(見第 6 個範例檔)。
// 4. 別在 lambda 內對同一個 once_flag 再呼叫 call_once —— 死結。
// 5. 本例用裸指標 + new,最後要手動 delete;實務建議 unique_ptr 或 magic static。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】call_once 搭配 lambda
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. call_once 的第二個參數可以傳哪些東西?
//     答：任何可被 INVOKE 的可呼叫物件 —— 一般函式、函式指標、lambda、
//         functor,以及「成員函式指標 + 物件指標」的組合。額外參數會被
//         完美轉發,所以連只能移動的參數也能傳。
//     追問：用 lambda 相對具名函式好在哪?
//         → 邏輯就寫在旗標旁邊、不污染命名空間,而且因為它是匿名的,
//           不會有人不小心直接呼叫那個初始化函式而繞過 call_once。
//
// ⚠️ 陷阱. 「兩條執行緒同時呼叫 call_once,第二條看到旗標已經設好,
//         就會直接往下執行 db->query()。」哪裡錯了?
//     答：錯在「直接往下執行」。當第一條執行緒正在跑初始化時,第二條
//         不是「看到已設就走人」,而是會被「阻塞等待」,直到初始化真正
//         完成才返回。這正是 call_once 和「檢查一個 bool 旗標」的根本差別:
//         bool 只能表達「做完了沒」,而 once_flag 還能表達「正在做」,
//         其他執行緒才知道該等。
//     為什麼會錯：把 once_flag 想成一個布林值。若它真的只是布林值,
//         第二條執行緒就會在物件還沒建構完時拿去用 —— 那正是雙重檢查
//         鎖定的經典 bug。
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
// 原始示範:初始化邏輯直接寫成 lambda
// -----------------------------------------------------------------------------
class Database {
public:
    Database() { say("  Database 初始化"); }
    void query() { say("  查詢中..."); }
};

Database* db = nullptr;
std::once_flag initFlag;

void initAndUse() {
    std::call_once(initFlag, []() {
        db = new Database();
    });
    // 返回時 db 保證已完整建構(不是「旗標已設」而已)
    db->query();
}

// -----------------------------------------------------------------------------
// 【日常實務範例】延遲建立連線池,並用 lambda 捕獲呼叫端提供的設定
//   情境: 連線池的大小要由設定決定,而設定只有 main() 知道。
//         用 lambda 捕獲就不必為了傳參數而多開一個全域變數。
//         同時示範一個真實世界的重要細節:只有「第一個到達的執行緒」
//         提供的參數會生效,後到者的參數會被靜默忽略 ——
//         這是 call_once 語意的必然結果,不是 bug,但很容易讓人誤會。
//   為什麼用本主題: 直接對應【詳細解釋 2】的捕獲能力與它的邊界。
// -----------------------------------------------------------------------------
struct ConnectionPool {
    int  size = 0;
    int  createdBy = 0;      // 記錄是哪條執行緒真的建立了它
};

ConnectionPool g_pool;
std::once_flag g_poolFlag;

ConnectionPool& acquirePool(int wantedSize, int threadId) {
    std::call_once(g_poolFlag, [wantedSize, threadId]() {
        g_pool.size      = wantedSize;
        g_pool.createdBy = threadId;
        say("    [pool] 由執行緒 " + std::to_string(threadId) +
            " 建立,大小 " + std::to_string(wantedSize));
    });
    return g_pool;
}

void poolWorker(int threadId, int wantedSize) {
    ConnectionPool& p = acquirePool(wantedSize, threadId);
    say("    [執行緒 " + std::to_string(threadId) + "] 要求大小 " +
        std::to_string(wantedSize) + ",實際取得大小 " + std::to_string(p.size));
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔從缺,理由如下
//   LeetCode 的並行題(1114 Print in Order、1115 Print FooBar Alternately、
//   1116 Print Zero Even Odd、1117 Building H2O、1195 Fizz Buzz Multithreaded)
//   考的是「反覆交替執行的順序控制」,call_once 的語意卻是「整個程式只做一次」,
//   放進那些題目反而會讓後續回合全部被跳過。硬湊不如從缺,
//   改以上面的連線池情境示範 lambda 捕獲的實際用途與邊界。
// -----------------------------------------------------------------------------

int main() {
    std::cout << "=== 原始示範:兩條執行緒同時初始化 ===" << std::endl;
    std::thread t1(initAndUse);
    std::thread t2(initAndUse);
    t1.join();
    t2.join();
    std::cout << "  ↑「Database 初始化」一次,「查詢中...」兩次" << std::endl;

    std::cout << "\n=== 實務:lambda 捕獲設定值建立連線池 ===" << std::endl;
    std::vector<std::thread> pool;
    pool.emplace_back(poolWorker, 1, 10);
    pool.emplace_back(poolWorker, 2, 99);
    pool.emplace_back(poolWorker, 3, 50);
    for (std::thread& t : pool) t.join();
    std::cout << "  最終連線池大小 = " << g_pool.size
              << ",由執行緒 " << g_pool.createdBy << " 建立" << std::endl;
    std::cout << "  ↑ 後到的執行緒即使要求不同大小,也只會拿到既有的那個池"
              << std::endl;

    delete db;
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.6：執行緒安全的初始化3.cpp" -o once_lambda

// 注意:以下為某一次實際執行的結果。
//   * 各執行緒的先後順序每次執行都不同,因此「連線池由哪條執行緒建立、
//     最終大小是 10 / 99 / 50 中的哪一個」每次執行都可能不一樣 ——
//     三種結果都是合法的,這正是「只有第一個到達者生效」的體現。
//   * 不變的是:「[pool] 由執行緒 … 建立」永遠只出現一行,
//     且三條執行緒印出的「實際取得大小」必定彼此相同。

// === 預期輸出 ===
// === 原始示範:兩條執行緒同時初始化 ===
//   Database 初始化
//   查詢中...
//   查詢中...
//   ↑「Database 初始化」一次,「查詢中...」兩次
//
// === 實務:lambda 捕獲設定值建立連線池 ===
//     [pool] 由執行緒 1 建立,大小 10
//     [執行緒 1] 要求大小 10,實際取得大小 10
//     [執行緒 2] 要求大小 99,實際取得大小 10
//     [執行緒 3] 要求大小 50,實際取得大小 10
//   最終連線池大小 = 10,由執行緒 1 建立
//   ↑ 後到的執行緒即使要求不同大小,也只會拿到既有的那個池
