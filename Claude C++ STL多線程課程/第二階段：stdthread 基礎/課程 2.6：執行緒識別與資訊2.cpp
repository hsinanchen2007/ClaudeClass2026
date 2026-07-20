// =============================================================================
//  課程 2.6：執行緒識別與資訊2.cpp
//  主題: std::thread::id 的比較語意,以及「id 會被重複使用」這個經典陷阱
// =============================================================================
//
// 【主題資訊 Information】
//   class thread::id {
//   public:
//       id() noexcept;                       // 預設建構 = 「不代表任何執行緒」
//   };
//   // C++11 起:
//   bool operator==(thread::id, thread::id) noexcept;
//   bool operator!=/< /<=/> />=(thread::id, thread::id) noexcept;   // C++20 起由 <=> 取代
//   // C++20 起:
//   strong_ordering operator<=>(thread::id, thread::id) noexcept;
//   template<> struct hash<thread::id>;      // 標準保證有特化
//   template<class charT, class traits>
//   basic_ostream<charT,traits>& operator<<(basic_ostream<charT,traits>&, thread::id);
//
//   標頭檔: <thread>
//   複雜度: 全部 O(1) 且 noexcept
//   本機實測: C++17 模式下六個關係運算子都可用;C++20 模式下 <=> 亦可用
//             (以 -std=c++17 -pedantic-errors 與 -std=c++20 -pedantic-errors 各驗過)
//
// 【詳細解釋 Explanation】
//
// 【1. thread::id 提供的是一組「剛好夠用」的介面】
// 標準對 thread::id 的承諾非常克制,只有三件事:
//   (a) 可比較相等   —— 讓你回答「這兩次是不是同一條執行緒」
//   (b) 可全序比較   —— 讓它能當 std::map 的 key(順序本身無意義,只求穩定)
//   (c) 可 hash、可輸出 —— 讓它能當 unordered_map 的 key、能寫進 log
// 特別注意 (b):< 定義了一個全序,但這個順序「不代表任何意義」——它不是建立時間
// 順序,也不是優先權順序。標準只保證它是個一致的全序,好讓關聯容器能運作。
// 拿 id 的大小去推論「誰先被建立」是沒有根據的。
//
// 【2. 相等比較的精確語意】
// 兩個 id 相等,代表它們指涉同一個執行緒,或者「兩者都不指涉任何執行緒」。
// 因此下面兩種寫法都是慣用法:
//       if (std::this_thread::get_id() == mainThreadId) ...   // 我是不是主執行緒
//       if (t.get_id() == std::thread::id{}) ...              // t 是不是空的
// 後者等價於 !t.joinable() —— 標準就是這樣定義 joinable() 的。
//
// 【3. ★ 核心陷阱:thread::id 會被重複使用 ★】
// 標準明文允許:一個執行緒結束、而且已經不能再被 join 之後,實作「可以」把它的
// thread::id 值拿給之後建立的新執行緒用。也就是說:
//       id 在「同一時刻」是唯一的,但在「整個程式生命週期」中不是唯一的。
//
// 本機實測(見 main() 的示範):連續建立 20 條執行緒,每條都 join() 完才建下一條,
// 結果 20 條執行緒總共只出現 1 個不同的 id —— 同一個值被重複用了 20 次。
// 對照組:讓 20 條執行緒同時存活,則得到 20 個相異的 id。
// (這是 libstdc++/glibc 的實作行為:pthread_t 是 TCB 位址,執行緒結束後其堆疊與
//  TCB 進入快取被下一條重複利用。重複使用是標準「允許」的,不是標準「保證」的,
//  次數與時機都不可依賴。)
//
// 這個特性直接推翻一個很自然的想法:
//       std::map<std::thread::id, Stats> g_stats;    // 想長期記錄每條執行緒的統計
// 只要執行緒有生有死(thread pool 重建 worker、短命任務執行緒),舊執行緒的統計
// 就會被新執行緒「繼承」,數字全部串味。而且它不會 crash、不會有任何警告,
// 只是靜靜地給你錯的數字——這是最難抓的那種 bug。
//
// 正確做法:
//   (a) 只在「執行緒確定活著」的期間用 id 當 key(例如整批 join 之前收集完統計);
//   (b) 需要長期身分,就自己發一個永不回收的序號(見下面實務範例 2 的 workerId);
//   (c) 或用 thread_local 讓每條執行緒自己攜帶身分與統計,結束時再匯總出去。
//
// 【4. 這個檔案原始碼裡藏著的另一個細節:全域 id 的初始化時機】
// mainThreadId 是全域變數,在 main() 執行前就被預設建構成「不代表任何執行緒」。
// 程式在 main() 的第一行才把它設成真正的主執行緒 id。這意味著:任何在 main()
// 之前執行的程式碼(其他翻譯單元的全域物件建構子)若呼叫 checkThread(),
// 都會因為 mainThreadId 還是預設值而被判成「子執行緒」——即使它就是主執行緒跑的。
// 這是靜態初始化順序問題(static initialization order fiasco)在多執行緒主題下的變形。
// 穩健的寫法是用函式內的 static,或直接在建構時就抓取(見實務範例 1)。
//
// 【概念補充 Concept Deep Dive】
// (A) 為什麼 libstdc++ 的 id 會被重用得這麼徹底
//     glibc 的 pthread_t 是執行緒描述區塊(TCB)的位址,而 TCB 與執行緒堆疊是一起
//     配置的。pthread_join() 回收之後,那塊堆疊會進入 glibc 的 stack cache,
//     下一次 pthread_create() 若要的大小相符就直接取用 —— 位址相同,於是 pthread_t
//     相同,於是 thread::id 相同。這也解釋了為什麼「依序建立/join」幾乎必然重用:
//     每次都拿回剛剛釋放的同一塊。
//
// (B) C++11 的六個關係運算子 vs C++20 的 <=>
//     C++11~17:標準列出 == != < <= > >= 六個自由函式。
//     C++20   :改為只列 operator== 與 operator<=>(回傳 strong_ordering),
//               另外四個由編譯器從 <=> 自動改寫產生。對使用者而言寫法完全不變,
//               本機在 -std=c++17 與 -std=c++20 下六個運算子都能編過。
//               差別在於:C++20 起你可以直接寫 (a <=> b) 拿到三向比較結果。
//
// (C) std::hash<thread::id> 的值也是實作定義
//     標準只保證特化存在、且相等的 id 有相同雜湊。雜湊值本身不可攜、不該持久化,
//     更不該拿去當跨行程或跨執行的識別碼。
//
// 【注意事項 Pay Attention】
// 1. ★ id 只在同一時刻唯一,不是永久唯一;絕不可當長期身分或資料庫主鍵。
// 2. < 的順序沒有語意,不代表建立先後,只是為了能放進 std::map。
// 3. 跨執行緒讀寫同一個 std::thread::id 變數(例如這裡的全域 mainThreadId)
//    仍然需要同步。本檔之所以安全,是因為主執行緒「先寫完」才建立子執行緒,
//    thread 的建構本身就建立了 happens-before(見注意事項 4)。
// 4. std::thread 建構子對新執行緒有同步保證:建構子的呼叫(以及此前的所有寫入)
//    happens-before 新執行緒開始執行。所以 main() 裡先設定 mainThreadId、
//    再 std::thread t(checkThread),子執行緒必定看得到已設定好的值,不是 data race。
// 5. 印出來的格式是實作定義,不要 parse(見前一個檔案)。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】thread::id 的比較與重複使用
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::thread::id 是唯一的嗎?
//     答：只在「同一時刻」唯一。標準明文允許實作把已結束、且不可再 join 的執行緒
//         的 id 重新配給新執行緒。本機實測:連續建立並 join 20 條執行緒,
//         總共只出現 1 個相異 id;但 20 條同時存活時則有 20 個相異 id。
//     追問：那 std::map<thread::id, Stats> 能用嗎?
//         → 只能用在「這些執行緒都還活著」的區間內。跨越執行緒生死就會張冠李戴,
//           要長期身分請自己發序號或用 thread_local。
//
// 🔥 Q2. thread::id 為什麼要支援 operator< ?排序有什麼意義?
//     答：純粹為了能當 std::map / std::set 的 key。標準只保證它是一致的全序,
//         順序本身沒有任何語意——不代表建立時間、不代表優先權。
//     追問：那 unordered_map 呢?→ 也可以,標準保證有 std::hash<thread::id> 特化。
//
// ⚠️ 陷阱. 這段「每條執行緒的處理量統計」為什麼數字會錯?
//         std::map<std::thread::id, long> g_count;      // 全域
//         void work(){ g_count[std::this_thread::get_id()]++; }   // 每條 worker 跑完就結束
//         // 主程式反覆建立 worker、跑完 join、再建下一批……最後印出 g_count
//     答：先不論 map 未加鎖本身就是 data race(UB);就算加了鎖,數字仍然會錯——
//         因為前一批 worker 結束後,它的 id 會被下一批 worker 重複使用,
//         兩批不同執行緒的計數被累加進同一個 key,看起來就像「有幾條執行緒特別忙」。
//     為什麼會錯：腦中把 thread::id 想成「像身分證字號一樣終身唯一」。
//         實際上它比較像「座位號碼」——你離席之後,號碼會給下一個人。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <vector>
#include <set>
#include <map>
#include <mutex>
#include <atomic>

std::thread::id mainThreadId;

void checkThread() {
    if (std::this_thread::get_id() == mainThreadId) {
        std::cout << "這是主執行緒" << std::endl;
    } else {
        std::cout << "這是子執行緒" << std::endl;
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】「這個函式只准在主執行緒呼叫」的執行期檢查
// 情境: GUI 框架(Qt/GTK/Cocoa)、OpenGL context、以及很多 C 函式庫都規定某些 API
//       只能在建立它的那條執行緒上呼叫,誤用的後果通常是隨機當掉而非明確報錯。
//       在這類 API 的包裝層加一道 id 比對,能把「幾週後隨機崩潰」變成「當場報錯」。
// 為何用 thread::id: 這是標準唯一可攜的「我現在在哪條執行緒」判斷方式。
// 改良點: 用函式內 static 抓取,避免全域變數的靜態初始化順序問題。
// -----------------------------------------------------------------------------
const std::thread::id& uiThreadId() {
    // 第一次呼叫(必須由主執行緒在啟動時呼叫一次)就把身分釘住
    static const std::thread::id id = std::this_thread::get_id();
    return id;
}

bool onUiThread() {
    return std::this_thread::get_id() == uiThreadId();
}

void updateWidget(const char* what) {
    if (!onUiThread()) {
        std::cout << "  [拒絕] " << what << " 只能在 UI 執行緒呼叫,已擋下" << std::endl;
        return;
    }
    std::cout << "  [允許] " << what << " 在 UI 執行緒執行" << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】用 map<thread::id, 統計> 聚合各執行緒工作量 —— 以及它的界線
// 情境: 想知道 thread pool 裡各 worker 分到多少工作,判斷負載是否平均。
// 正確用法: 在「所有 worker 都還活著」的期間累積,全部 join 之前/當下收集完畢。
// 界線示範: 同時示範「跨越執行緒生死」時,同一個 id 會把兩批 worker 的統計混在一起。
// 注意: map 是共享狀態,必須加鎖;不加鎖的並行 map 寫入是 data race = UB。
// -----------------------------------------------------------------------------
std::mutex g_statMutex;
std::map<std::thread::id, long> g_workDone;

void poolWorker(int items) {
    for (int i = 0; i < items; ++i) {
        std::lock_guard<std::mutex> lk(g_statMutex);
        ++g_workDone[std::this_thread::get_id()];
    }
}

int main() {
    mainThreadId = std::this_thread::get_id();
    uiThreadId();  // 在主執行緒釘住 UI 執行緒身分

    std::cout << "=== 原始示範:比較 id 判斷身分 ===" << std::endl;
    checkThread();  // 在主執行緒呼叫

    std::thread t(checkThread);  // 在子執行緒呼叫
    t.join();

    // -------------------------------------------------------------------------
    std::cout << "\n=== 預設 id 的語意:non-joinable 的 thread 都相等 ===" << std::endl;
    std::thread empty1;                 // 預設建構
    std::thread empty2([]{});
    empty2.join();                      // join 後也變成 non-joinable
    std::cout << "預設建構 == join 過的? "
              << (empty1.get_id() == empty2.get_id()) << std::endl;
    std::cout << "兩者都 == thread::id{}? "
              << (empty1.get_id() == std::thread::id{}
                  && empty2.get_id() == std::thread::id{}) << std::endl;

    // -------------------------------------------------------------------------
    std::cout << "\n=== ★ 陷阱實證:thread::id 會被重複使用 ★ ===" << std::endl;
    {
        std::set<std::thread::id> sequentialIds;
        for (int i = 0; i < 20; ++i) {
            std::thread w([]{});
            sequentialIds.insert(w.get_id());
            w.join();                    // 每條都收完才建下一條
        }
        std::cout << "依序建立並 join 20 條 -> 相異 id 數 = "
                  << sequentialIds.size() << std::endl;

        std::set<std::thread::id> concurrentIds;
        std::vector<std::thread> alive;
        alive.reserve(20);
        std::atomic<bool> release{false};
        for (int i = 0; i < 20; ++i) {
            alive.emplace_back([&release]{
                while (!release) std::this_thread::yield();   // 撐著不結束
            });
        }
        for (auto& th : alive) concurrentIds.insert(th.get_id());
        release = true;
        for (auto& th : alive) th.join();
        std::cout << "20 條同時存活       -> 相異 id 數 = "
                  << concurrentIds.size() << std::endl;
        std::cout << "結論:id 只在同一時刻唯一,不是永久唯一" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務 1: 只准在 UI 執行緒呼叫的 API ===" << std::endl;
    updateWidget("setText()");                      // 主執行緒 -> 允許
    std::thread bg([]{ updateWidget("setText()"); });  // 背景執行緒 -> 擋下
    bg.join();

    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務 2: 各 worker 工作量統計(所有 worker 同時存活) ===" << std::endl;
    {
        std::vector<std::thread> pool;
        pool.emplace_back(poolWorker, 5);
        pool.emplace_back(poolWorker, 3);
        pool.emplace_back(poolWorker, 7);
        for (auto& w : pool) w.join();

        long total = 0;
        for (const auto& kv : g_workDone) total += kv.second;
        std::cout << "map 中的 key 數 = " << g_workDone.size()
                  << " ,總工作量 = " << total << " (預期 3 個 key / 共 15)" << std::endl;
    }

    std::cout << "\n=== 界線示範: 跨越執行緒生死後,統計會被張冠李戴 ===" << std::endl;
    {
        g_workDone.clear();
        // 第一批:跑完就結束
        std::thread a(poolWorker, 4);
        a.join();
        // 第二批:很可能拿到第一批剛釋放的 id
        std::thread b(poolWorker, 6);
        b.join();

        std::cout << "兩批各自獨立的 worker,map 中卻只有 " << g_workDone.size()
                  << " 個 key" << std::endl;
        for (const auto& kv : g_workDone) {
            std::cout << "  某個 id 的累計工作量 = " << kv.second
                      << "  (若為 10,代表兩批被合併成同一個 key)" << std::endl;
        }
        std::cout << "→ 這就是為什麼 thread::id 不能當長期身分" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.6：執行緒識別與資訊2.cpp" -o thread_id_compare

// === 預期輸出 ===
// 以下為某次實際執行結果。
// * 「相異 id 數」與最後的「張冠李戴」示範依賴實作的 id 重用行為:標準只「允許」
//   重用、不「保證」重用,因此在別的平台/實作上數字可能不同(例如相異 id 數 = 20、
//   兩批各自成 key)。本機 libstdc++/glibc 上重用非常穩定地發生。
// * 「這是主執行緒/子執行緒」與 UI 執行緒擋下與否則是確定的,每次都一樣。
//
// === 原始示範:比較 id 判斷身分 ===
// 這是主執行緒
// 這是子執行緒
//
// === 預設 id 的語意:non-joinable 的 thread 都相等 ===
// 預設建構 == join 過的? 1
// 兩者都 == thread::id{}? 1
//
// === ★ 陷阱實證:thread::id 會被重複使用 ★ ===
// 依序建立並 join 20 條 -> 相異 id 數 = 1
// 20 條同時存活       -> 相異 id 數 = 20
// 結論:id 只在同一時刻唯一,不是永久唯一
//
// === 日常實務 1: 只准在 UI 執行緒呼叫的 API ===
//   [允許] setText() 在 UI 執行緒執行
//   [拒絕] setText() 只能在 UI 執行緒呼叫,已擋下
//
// === 日常實務 2: 各 worker 工作量統計(所有 worker 同時存活) ===
// map 中的 key 數 = 3 ,總工作量 = 15 (預期 3 個 key / 共 15)
//
// === 界線示範: 跨越執行緒生死後,統計會被張冠李戴 ===
// 兩批各自獨立的 worker,map 中卻只有 1 個 key
//   某個 id 的累計工作量 = 10  (若為 10,代表兩批被合併成同一個 key)
// → 這就是為什麼 thread::id 不能當長期身分
