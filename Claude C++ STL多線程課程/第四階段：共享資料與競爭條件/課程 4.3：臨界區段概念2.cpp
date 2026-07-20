// =============================================================================
//  課程 4.3：臨界區段概念2.cpp  —  共享資源清單：什麼算、什麼不算
// =============================================================================
//
// 【本檔性質】原始版本是一張「共享資源分類表」，但 function() 裡的三個變數
//   只宣告不使用，編譯時會產生 4 個 -Wunused 警告，也看不出三者的差異。
//   本檔保留完整分類，並讓每個變數【真的被多執行緒使用】，
//   用實際輸出證明 static 與 thread_local 的行為差異（同時消除警告）。
//   其中 worker() 對 globalCounter 的存取仍刻意不加鎖 → data race → 【UB】。
//
// 【主題資訊 Information】
//   主題：    共享資源的完整清單，以及 storage duration 與共享性的關係
//   標準版本：thread_local 為 C++11 關鍵字（在此之前只能用 __thread / TLS API）
//   標頭檔：  <thread>、<vector>、<iostream>
//   核心判準：兩條執行緒能不能取得【同一個位址】？能 → 共享；不能 → 不共享。
//
// 【詳細解釋 Explanation】
//
// 【1. 依儲存期 (storage duration) 分類】
//   C++ 有四種儲存期，共享性一目了然：
//     ① static storage duration（全域變數、static 變數、類別 static 成員）
//        → 整個程式只有一份，所有執行緒共用 →【共享】
//     ② thread storage duration（thread_local）
//        → 每條執行緒各一份，各自獨立 →【不共享】
//     ③ automatic storage duration（區域變數）
//        → 在各自的執行緒堆疊上 →【不共享】（但見【4】的例外）
//     ④ dynamic storage duration（new / make_shared 出來的物件）
//        → 物件本身在堆積上，共享與否取決於【指標有沒有被分享出去】
//   → 記住這張表，就能在 code review 時三秒判定要不要加鎖。
//
// 【2. 函式內的 static 是最容易被漏掉的共享資源】
//   void function() {
//       int localVar = 0;            // 不共享：每次呼叫、每條執行緒各一份
//       static int localStatic = 0;  // 【共享！】所有執行緒共用同一份
//       thread_local int tlVar = 0;  // 不共享：每條執行緒各一份
//   }
//   `localStatic` 寫在函式裡面，看起來很「區域」，但它是 static 儲存期，
//   整個程式只有一份。多執行緒同時 ++localStatic 就是不折不扣的 data race。
//   這是實務上極常見的漏網之魚 —— 特別是拿來做「呼叫次數統計」或
//   「lazy 初始化的快取」時。
//   （註：C++11 起 function-local static 的【初始化】本身是執行緒安全的，
//     所謂 magic static；但初始化之後的【讀寫】完全沒有保護。
//     兩者常被混為一談，見下方面試題。）
//
// 【3. 類別成員的共享性取決於「物件」而非「類別」】
//   class MyClass {
//       int memberVar;           // 共享與否 → 看是不是同一個物件被多執行緒用
//       static int staticMember; // 一定共享 → 所有物件、所有執行緒共用一份
//   };
//   * 每條執行緒各自 new 一個 MyClass → memberVar 不共享
//   * 同一個 MyClass 物件被兩條執行緒使用 → memberVar 共享
//   → 所以「這個類別是不是執行緒安全」這句話本身是不完整的，
//     必須說清楚是「同一個物件被多執行緒使用時是否安全」。
//
// 【4. 區域變數唯一會失效的情形】
//   區域變數天生不共享，但只要它的【位址】被傳出去就不再成立：
//     * 被 lambda 以 `[&]` 引用捕捉，而該 lambda 交給別的執行緒
//     * 用 std::ref(x) 傳給 std::thread
//     * 取 &x 存進共享容器
//   這三種寫法都會讓堆疊上的變數瞬間變成共享資料；
//   若原函式先回傳，還會多一個懸空引用的 UB。
//
// 【5. thread_local 的代價與適用場景】
//   thread_local 是「用空間換同步」：每條執行緒一份，完全不需要鎖。
//   適合：per-thread 的快取、亂數產生器狀態、記憶體池、錯誤碼（errno 就是 TLS）。
//   代價：① 每條執行緒都要配置一份（執行緒多時記憶體放大）
//         ② 存取比一般變數稍慢（需經由 TLS 區塊定址，本機為
//            fs 段暫存器相對定址，成本很低但非零）
//         ③ 非平凡型別的解構函式在執行緒結束時才執行，順序不易掌握
//   → 但「不需要同步」通常遠勝這些代價，是很值得優先考慮的設計。
//
// 【概念補充 Concept Deep Dive】
//
// (A) thread_local 在本機的實作方式
//   Linux/x86-64 的 ELF TLS 模型下，每條執行緒有一塊 TLS 區塊，
//   由 fs 段暫存器指向。存取 thread_local 變數編譯成
//   `mov eax, DWORD PTR fs:0xfffffffffffffffc` 之類的段相對定址 ——
//   只是多一個段前綴，沒有函式呼叫、沒有鎖，所以成本極低。
//   （相對地，POSIX 的 pthread_getspecific 是真的函式呼叫，慢得多。）
//
// (B) magic static：C++11 保證「初始化」執行緒安全，但僅止於初始化
//   static Foo& instance() { static Foo f; return f; }   // 初始化是安全的
//   編譯器會插入一個 guard variable 與必要的同步，保證 f 只被建構一次，
//   且其他執行緒會等待建構完成 —— 這就是 Meyers Singleton 在 C++11 之後
//   成為推薦寫法的原因（課程 4.4 的雙重檢查鎖定範例會對照）。
//   但【建構完成之後】對 f 的所有讀寫都毫無保護，
//   要不要加鎖是另一個完全獨立的問題。
//
// (C) 為什麼 static 區域變數比全域變數更危險
//   全域變數擺在檔案最上方，review 時一眼就看得到，容易警覺；
//   函式內的 static 藏在一百行的函式中間，長得跟區域變數幾乎一樣，
//   常常在「加個計數器看看被呼叫幾次」這種臨時 debug 需求下被加進去，
//   然後就永遠留在正式程式碼裡。
//
// 【注意事項 Pay Attention】
// 1. 函式內的 static 是共享資源，不是區域變數 —— 最常被漏掉的一種。
// 2. C++11 的 magic static 只保證「初始化」安全，不保證後續讀寫安全。
// 3. 類別成員的共享性取決於「同一個物件是否被多執行緒使用」。
// 4. 區域變數被 `[&]` / std::ref / 取位址傳出去之後就不再安全。
// 5. thread_local 用空間換掉同步，但每條執行緒各一份，執行緒多時要留意記憶體。
// 6. 本檔第一段 worker() 是 UB 示範，不可宣稱固定輸出。
//
// 【LeetCode 說明】本檔不附 LeetCode 範例。
//   本檔是一張「共享資源分類表」，重點在判定方法而非演算法；
//   允許使用的設計題（146/155/705/707/1603）與並行題（1114～1117/1195）
//   都無法誠實對應「storage duration 與共享性的關係」，故從缺，
//   改以下方兩個真實情境（per-thread 亂數種子、per-thread 記憶體池）呈現。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】共享資源的判定與 thread_local
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 函式裡的 static int 是共享資源嗎?需要加鎖嗎?
//     答：是共享資源，而且需要加鎖。它是 static 儲存期，整個程式只有一份，
//         所有執行緒共用同一個位址。它寫在函式內只影響「可見範圍(scope)」，
//         不影響「生命期(storage duration)」——兩者是完全獨立的概念。
//     追問：可是 C++11 不是保證 function-local static 執行緒安全嗎?
//         → 那是 magic static，只保證【初始化】只發生一次且其他執行緒會等待；
//           初始化完成後的讀寫完全沒有保護。兩者常被混為一談。
//
// 🔥 Q2. thread_local 和 static 差在哪?什麼時候該用 thread_local?
//     答：static 是「整個程式一份」，thread_local 是「每條執行緒一份」。
//         thread_local 用空間換掉同步 —— 因為不共享，所以完全不需要鎖。
//         適合 per-thread 快取、亂數狀態、記憶體池、errno 這類天生
//         就該各自獨立的東西。
//     追問：thread_local 有什麼代價?
//         → 每條執行緒都配置一份（執行緒多時記憶體放大）；
//           存取需經 TLS 定址（本機是 fs 段相對定址，成本很低但非零）；
//           非平凡型別的解構在執行緒結束時才跑，順序不易掌握。
//
// ⚠️ 陷阱. 「這個類別沒有 static 成員，所以它是執行緒安全的」——錯在哪?
//     答：共享性取決於【物件】而不是【類別】。只要同一個物件被兩條執行緒
//         使用，它的每個非 static 成員都是共享資料。
//         反過來說，若每條執行緒各自持有自己的物件，即使類別完全沒加鎖也安全。
//     為什麼會錯：把「執行緒安全」當成類別的固有屬性。
//         正確的問法永遠是「同一個【物件】被多執行緒使用時安不安全」，
//         這也是為什麼標準函式庫的文件都寫「不同物件的並行存取是安全的，
//         同一物件的並行存取則否」。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <string>
#include <sstream>
#include <algorithm>
#include <functional>

// ── 共享資源 ────────────────────────────────────────────────────────────────
int globalCounter = 0;                    // 全域變數           →【共享】
static int staticCounter = 0;             // 靜態變數           →【共享】
std::vector<int> sharedVector;            // 全域容器           →【共享】
std::mutex ioMtx;                         // 保護輸出用

class MyClass {
    int memberVar;                        // 若多執行緒存取同一物件 →【共享】
    static int staticMember;              // 靜態成員           →【一定共享】

public:
    explicit MyClass(int v) : memberVar(v) {}

    void method() {
        // memberVar 是否為共享資源？
        // 取決於是否多執行緒存取同一個 MyClass 物件：
        //   * 每條執行緒各自持有一個 MyClass → 不共享，這裡不需要鎖
        //   * 同一個 MyClass 被多執行緒使用   → 共享，這裡就必須鎖
        ++memberVar;
    }

    int value() const { return memberVar; }

    // 靜態成員不管怎麼用都是共享的，所以一律要保護
    static void bumpStatic() {
        static std::mutex sm;
        std::lock_guard<std::mutex> lock(sm);
        ++staticMember;
    }
    static int staticValue() { return staticMember; }
};

int MyClass::staticMember = 0;            // 靜態成員的定義

// ── 三種變數的差異（讓它們真的被使用，才看得出行為差別）──────────────────
// 回傳這一次呼叫後三個變數各自的值，用來對照 static 與 thread_local 的差異。
struct FuncCounts {
    int local;        // 每次呼叫都重置 → 永遠是 1
    int shared;       // 所有執行緒共用 → 一路累加（需要保護）
    int perThread;    // 每條執行緒一份 → 各自從 1 開始數
};

std::mutex staticMtx;

FuncCounts function() {
    int localVar = 0;                     // 不是共享資源：每次呼叫都是新的
    static int localStatic = 0;           // 是共享資源！所有執行緒共用一份
    thread_local int tlVar = 0;           // 不是共享資源：每條執行緒各一份

    ++localVar;                           // 永遠變成 1
    ++tlVar;                              // 每條執行緒各自累加，不需要鎖

    int sharedSnapshot;
    {
        std::lock_guard<std::mutex> lock(staticMtx);   // static 必須保護
        ++localStatic;
        sharedSnapshot = localStatic;
    }
    return FuncCounts{localVar, sharedSnapshot, tlVar};
}

// ── 原始的 worker（刻意不加鎖 → data race / UB）─────────────────────────────
void worker() {
    int localVar = 0;        // 不是臨界區段：區域變數

    localVar = 42;           // 不是臨界區段：只存取區域變數

    globalCounter = localVar;   // ← 臨界區段：寫入共享資料

    int temp = globalCounter;   // ← 臨界區段：讀取共享資料（如有寫入者）

    std::cout << temp;       // 可能是臨界區段：cout 也是共享資源
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】per-thread 亂數產生器：thread_local 消滅整把鎖
//   情境：模擬 / 壓測 / 蒙地卡羅要在多執行緒中大量取亂數。
//         若共用一個 std::mt19937 就必須每次上鎖，鎖會成為最大瓶頸
//         （亂數本身只要幾奈秒，鎖卻要數十奈秒 —— 同步比工作還貴）。
//   做法：每條執行緒一個 thread_local 引擎，各自播種，完全不需要同步。
//         這是實務上 thread_local 最經典、也最划算的用途。
//   注意：種子要摻入執行緒識別資訊，否則每條執行緒會產生一模一樣的序列。
// -----------------------------------------------------------------------------
unsigned nextRandom() {
    // 每條執行緒各一份狀態 → 無鎖、無競爭
    thread_local unsigned state = 0;
    thread_local bool seeded = false;
    if (!seeded) {
        // 用執行緒 id 的雜湊當種子，確保各執行緒序列不同
        state = static_cast<unsigned>(
                    std::hash<std::thread::id>{}(std::this_thread::get_id()))
                | 1u;
        seeded = true;
    }
    // xorshift32：純區域運算，不碰任何共享資料
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】per-thread 統計緩衝：先各自累積，最後才合併一次
//   情境：高頻交易 / 遊戲伺服器的即時統計，每秒數百萬次事件。
//         若每個事件都去鎖全域計數器，鎖競爭會吃掉大部分 CPU。
//   做法：每條執行緒用 thread_local 累積，執行緒收尾時才上鎖合併一次。
//         把「數百萬次上鎖」降為「每執行緒一次上鎖」。
//         （這正是課程 5.6「批次累加」優化的思想來源。）
// -----------------------------------------------------------------------------
struct GlobalTally {
    std::mutex mtx;
    long events = 0;
    long bytes = 0;

    void merge(long e, long b) {
        std::lock_guard<std::mutex> lock(mtx);   // 每條執行緒只呼叫一次
        events += e;
        bytes += b;
    }
};

void tallyWorker(GlobalTally& g, int iterations) {
    thread_local long localEvents = 0;   // 各自一份，全程無鎖
    thread_local long localBytes = 0;
    localEvents = 0;
    localBytes = 0;

    for (int i = 0; i < iterations; ++i) {
        ++localEvents;
        localBytes += 64 + (i % 8);
    }
    g.merge(localEvents, localBytes);    // 全程唯一一次上鎖
}

int main() {
    std::cout << "=== 原始 worker：globalCounter 未保護（data race / UB）===\n";
    {
        std::thread t1(worker);
        std::thread t2(worker);
        t1.join();
        t2.join();
        std::cout << "\n（上面那串數字每次執行都可能不同，且不受標準保證）\n";
    }

    std::cout << "\n=== 三種變數的實際差異（每條執行緒各呼叫 function() 三次）===\n";
    {
        std::vector<std::thread> ths;
        for (int i = 1; i <= 3; ++i) {
            ths.emplace_back([i] {
                std::ostringstream oss;
                for (int k = 0; k < 3; ++k) {
                    FuncCounts c = function();
                    oss << "  [thread " << i << "] localVar=" << c.local
                        << "  localStatic=" << c.shared
                        << "  tlVar=" << c.perThread << "\n";
                }
                std::lock_guard<std::mutex> lock(ioMtx);
                std::cout << oss.str();
            });
        }
        for (auto& t : ths) t.join();
        std::cout << "  → localVar   永遠是 1（每次呼叫都是新的區域變數）\n";
        std::cout << "  → tlVar      每條執行緒各自數到 3（thread_local，不共享）\n";
        std::cout << "  → localStatic 三條執行緒共用，最終累加到 9（static，共享！）\n";
    }

    std::cout << "\n=== 類別成員：同一物件 vs 各自的物件 ===\n";
    {
        // 各自持有自己的物件 → memberVar 不共享，不需要鎖
        std::vector<std::thread> ths;
        std::vector<int> results(4, 0);
        for (int i = 0; i < 4; ++i) {
            ths.emplace_back([&results, i] {
                MyClass own(0);                       // 每條執行緒自己的物件
                for (int k = 0; k < 1000; ++k) own.method();
                results[static_cast<size_t>(i)] = own.value();   // 各寫各的索引，不重疊
            });
        }
        for (auto& t : ths) t.join();
        std::cout << "各自持有物件時，每個 memberVar = "
                  << results[0] << " / " << results[1] << " / "
                  << results[2] << " / " << results[3]
                  << " (皆為 1000，完全不需要鎖)\n";

        // 靜態成員一定共享 → 一律要保護
        std::vector<std::thread> ths2;
        for (int i = 0; i < 4; ++i) {
            ths2.emplace_back([] { for (int k = 0; k < 1000; ++k) MyClass::bumpStatic(); });
        }
        for (auto& t : ths2) t.join();
        std::cout << "靜態成員 staticMember = " << MyClass::staticValue()
                  << " (必定為 4000，因為有鎖保護)\n";
    }

    std::cout << "\n=== 全域容器也是共享資源 ===\n";
    {
        std::mutex vecMtx;
        std::vector<std::thread> ths;
        for (int i = 0; i < 4; ++i) {
            ths.emplace_back([&vecMtx, i] {
                for (int k = 0; k < 250; ++k) {
                    std::lock_guard<std::mutex> lock(vecMtx);
                    sharedVector.push_back(i * 250 + k);
                }
            });
        }
        for (auto& t : ths) t.join();
        std::cout << "sharedVector.size() = " << sharedVector.size()
                  << " (必定為 1000)\n";
        std::cout << "staticCounter（示範用的全域 static）= " << ++staticCounter << "\n";
    }

    std::cout << "\n=== 日常實務 1：per-thread 亂數（thread_local 免鎖）===\n";
    {
        std::vector<std::thread> ths;
        std::vector<size_t> distinct(4, 0);
        for (int i = 0; i < 4; ++i) {
            ths.emplace_back([&distinct, i] {
                std::vector<unsigned> seen;
                seen.reserve(10000);
                for (int k = 0; k < 10000; ++k) seen.push_back(nextRandom());
                std::sort(seen.begin(), seen.end());
                seen.erase(std::unique(seen.begin(), seen.end()), seen.end());
                distinct[static_cast<size_t>(i)] = seen.size();
            });
        }
        for (auto& t : ths) t.join();
        std::cout << "4 條執行緒各取 10000 個亂數，各自的相異值個數: "
                  << distinct[0] << " / " << distinct[1] << " / "
                  << distinct[2] << " / " << distinct[3] << "\n";
        std::cout << "→ 全程零上鎖；每條執行緒的引擎狀態各自獨立\n";
    }

    std::cout << "\n=== 日常實務 2：per-thread 累積，收尾才合併一次 ===\n";
    {
        GlobalTally tally;
        std::vector<std::thread> ths;
        for (int i = 0; i < 8; ++i) {
            ths.emplace_back(tallyWorker, std::ref(tally), 100000);
        }
        for (auto& t : ths) t.join();
        std::cout << "8 執行緒 × 100000 事件\n";
        std::cout << "events = " << tally.events << " (必定為 800000)\n";
        std::cout << "上鎖次數: 8 次（而非 800000 次）\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 4.3：臨界區段概念2.cpp' -o critical2
//
// 偵測資料競爭（第一段 worker() 是 UB，建議跑一次）:
//   g++ -std=c++17 -fsanitize=thread -g -pthread '課程 4.3：臨界區段概念2.cpp' -o critical2_tsan

// ⚠️ 兩處內容【每次執行都不同】，下面只是本機某一次的真實實測：
//   (1) 第一段的「4242」——該段是 genuine data race → UB，不受標準保證。
//   (2)「三種變數的實際差異」九行的【出現順序】與 localStatic 的【數值分配】
//       取決於排程（例如 thread 2 可能拿到 1/3/5、thread 1 拿到 2/4/6）。
//       確定的是：localVar 恆為 1、每條執行緒的 tlVar 恆為 1→2→3、
//       localStatic 恆為 1..9 各出現一次（因為有鎖保護，不會重號或漏號）。
// 其餘各段（類別成員、全域容器、per-thread 亂數與統計）皆為確定值。

// === 預期輸出 ===
// === 原始 worker：globalCounter 未保護（data race / UB）===
// 4242
// （上面那串數字每次執行都可能不同，且不受標準保證）
//
// === 三種變數的實際差異（每條執行緒各呼叫 function() 三次）===
//   [thread 2] localVar=1  localStatic=2  tlVar=1
//   [thread 2] localVar=1  localStatic=3  tlVar=2
//   [thread 2] localVar=1  localStatic=5  tlVar=3
//   [thread 1] localVar=1  localStatic=1  tlVar=1
//   [thread 1] localVar=1  localStatic=4  tlVar=2
//   [thread 1] localVar=1  localStatic=6  tlVar=3
//   [thread 3] localVar=1  localStatic=7  tlVar=1
//   [thread 3] localVar=1  localStatic=8  tlVar=2
//   [thread 3] localVar=1  localStatic=9  tlVar=3
//   → localVar   永遠是 1（每次呼叫都是新的區域變數）
//   → tlVar      每條執行緒各自數到 3（thread_local，不共享）
//   → localStatic 三條執行緒共用，最終累加到 9（static，共享！）
//
// === 類別成員：同一物件 vs 各自的物件 ===
// 各自持有物件時，每個 memberVar = 1000 / 1000 / 1000 / 1000 (皆為 1000，完全不需要鎖)
// 靜態成員 staticMember = 4000 (必定為 4000，因為有鎖保護)
//
// === 全域容器也是共享資源 ===
// sharedVector.size() = 1000 (必定為 1000)
// staticCounter（示範用的全域 static）= 1
//
// === 日常實務 1：per-thread 亂數（thread_local 免鎖）===
// 4 條執行緒各取 10000 個亂數，各自的相異值個數: 10000 / 10000 / 10000 / 10000
// → 全程零上鎖；每條執行緒的引擎狀態各自獨立
//
// === 日常實務 2：per-thread 累積，收尾才合併一次 ===
// 8 執行緒 × 100000 事件
// events = 800000 (必定為 800000)
// 上鎖次數: 8 次（而非 800000 次）
