// =============================================================================
//  課程 2.2：執行緒函式的多種形式1.cpp  —  自由函式（Free Function）作為執行緒入口
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：#include <thread>
//   標準版本：C++11 起（std::thread 本身是 C++11 引入）
//   建構子簽名（C++11，C++20 起加上 constexpr 相關措辭調整）：
//     template<class F, class... Args>
//     explicit thread(F&& f, Args&&... args);
//   語法：std::thread t(函式名);          // 函式名會 decay 成函式指標
//         t.join();                        // 等待該執行緒結束
//   複雜度：建立一條 OS 執行緒的成本遠高於一次函式呼叫。Linux 上 pthread_create
//           典型是「數十微秒」等級，且每條執行緒預設保留 8 MB 的 stack 位址空間
//           （`ulimit -s` 可查，本機為 8192 KB；那是虛擬位址保留，不是立刻吃掉
//           8 MB 實體記憶體）。因此「一個小任務開一條執行緒」通常不划算。
//   連結：慣例加 -pthread。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼建構子是 template，而不是收 void(*)()】
//   最直覺的設計是讓 std::thread 只收一個 void(*)() 函式指標。但那樣就只能傳
//   「無參數的自由函式」——lambda、functor、成員函式全部進不來。標準改用
//   可變參數樣板（variadic template），讓建構子接受「任何可呼叫物件（Callable）
//   + 任意多個參數」，於是這四種形式都能用同一個建構子：
//     std::thread t(free_func);                 // 本檔主題
//     std::thread t([]{ ... });                 // lambda（檔案 2）
//     std::thread t(&C::method, &obj);          // 成員函式（檔案 3）
//     std::thread t{Functor{}};                 // functor（檔案 4、5）
//   代價是錯誤訊息會變得很長（樣板展開），以及多了 most vexing parse 的坑
//   （見檔案 4）。
//
// 【2. 函式名 → 函式指標：function-to-pointer decay】
//   寫 std::thread t(greet); 時，greet 這個「函式左值」會發生
//   function-to-pointer conversion，變成 void(*)() 型別的指標，再被複製進
//   執行緒的儲存區。這代表：
//     * 傳進去的是「位址」，不是函式的複本——函式本體在 .text 區段，全程有效。
//     * 自由函式沒有狀態、沒有生命週期問題，所以是四種形式裡最安全的一種。
//       （lambda 有捕獲、functor 有成員、成員函式有 this，都要顧生命週期。）
//
// 【3. decay-copy：建構子把東西「複製」到新執行緒】
//   標準規定 std::thread 建構子會對可呼叫物件與每個參數做 decay-copy
//   （C++11/14 用語是 DECAY_COPY，C++20 改寫為 auto(...) 的等價措辭），
//   把複本存進新執行緒的儲存區，再以 INVOKE 語意在新執行緒中呼叫。
//   「decay」的意思是套用 std::decay：去掉 reference、去掉 const/volatile、
//   陣列與函式退化成指標。所以：
//     * 傳陣列 → 變成指標；傳函式名 → 變成函式指標（本檔情況）。
//     * 傳左值物件 → 呼叫「複製建構子」複製一份（不是綁引用！）。
//     * 想要真的傳引用，必須用 std::ref（見檔案 3 的實測）。
//   本檔傳的是函式指標，複製一個指標是零風險的，所以看不出差別；
//   但這個規則在檔案 3、4、5 會變成關鍵。
//
// 【4. 執行緒什麼時候開始跑？——建構完就開始，不是 join 才開始】
//   這是初學者最常誤解的一點。std::thread 建構子回傳時，新執行緒「可能已經
//   在跑了，也可能還沒被排程」——兩者都合法。join() 不是「啟動」，而是
//   「阻塞呼叫端，直到那條執行緒跑完」。所以本檔的輸出順序是確定的
//   （只有一條子執行緒，main 在 join 上等它），但只要有兩條以上，
//   交錯順序就不再保證（見檔案 2）。
//
// 【5. join() 為什麼是必要的】
//   std::thread 物件解構時，如果它仍然 joinable（還綁著一條沒被 join、
//   也沒被 detach 的 OS 執行緒），標準規定呼叫 std::terminate()。
//   這是「標準保證」，不是未定義行為，也不是「可能會」——本機實測
//   exit code 134（SIGABRT），訊息 "terminate called without an active exception"。
//   設計理由：讓「忘記處理執行緒」立刻炸給你看，而不是預設偷偷 detach
//   造成更難查的 dangling reference。詳見課程 2.5（joinable 狀態檢查）。
//
// 【概念補充 Concept Deep Dive】
//
// (A) std::thread 物件 vs OS 執行緒：handle 與實體是分開的
//   本機實測 sizeof(std::thread) == 8——它裡面基本上只有一個 pthread_t
//   （在 glibc 上是 unsigned long）。也就是說 std::thread 只是個「握把」，
//   真正的執行緒（核心的 task_struct、使用者態的 TCB、stack）在別的地方。
//   這解釋了兩件事：
//     * std::thread 可以 move、不可以 copy——複製一個握把會導致兩個物件
//       都以為自己擁有同一條執行緒，join 兩次即為錯誤。
//     * move 走之後原物件變成「空握把」，joinable() 變 false（課程 2.5）。
//
// (B) 底層發生什麼：從 std::thread 到 pthread_create
//   libstdc++ 的流程大致是：
//     1. 把可呼叫物件與參數 decay-copy 進一個 heap 上配置的 _State 物件。
//     2. 呼叫 pthread_create，入口點是一個 extern "C" 的跳板函式，
//        參數就是那個 _State 指標。
//     3. 新執行緒在跳板裡以 INVOKE 語意呼叫可呼叫物件，結束後銷毀 _State。
//   注意第 1 步是 heap 配置——這是「開執行緒不便宜」的原因之一，
//   也是為什麼實務上用 thread pool 重複利用執行緒，而不是每個任務開一條。
//
// (C) -pthread 到底做了什麼
//   -pthread 不等於 -lpthread：它同時（a）定義 _REENTRANT 巨集、
//   （b）連結執行緒函式庫、（c）在某些平台調整 code generation。
//   實測補充：本機 glibc 2.43，而 glibc 自 2.34 起已把 libpthread 併入 libc，
//   所以「不加 -pthread 也能連結成功」——本檔實測未加 -pthread 仍可編譯執行。
//   但這是平台細節，不是可攜保證；舊 glibc 或其他平台會出現連結錯誤或
//   執行期詭異行為。**一律加 -pthread** 才是正確習慣。
//
// (D) 回傳值去哪了
//   執行緒函式的回傳值會被直接丟棄——std::thread 沒有提供取回的管道。
//   要拿回結果請用 std::future / std::packaged_task / std::async（第五階段），
//   或自己用「傳入輸出參數 + 同步機制」。
//
// 【注意事項 Pay Attention】
//   1. 執行緒函式若讓例外逃出去，會呼叫 std::terminate()——這是標準保證。
//      本機實測 exit code 134，訊息含 "terminate called after throwing an
//      instance of 'std::runtime_error'"。例外「不會」自動傳播回 join() 的
//      那一端；要跨執行緒傳遞例外必須用 std::exception_ptr 或 std::future。
//   2. joinable 的 std::thread 解構 → std::terminate()（標準保證，見上）。
//      每條執行緒都必須恰好被 join() 或 detach() 一次。
//   3. join() 只能呼叫一次。對已經 join 過的物件再 join，會丟
//      std::system_error（errc::invalid_argument）。
//   4. 自由函式的位址全程有效，所以本檔沒有生命週期問題；但一旦改用 lambda
//      捕獲區域變數、或傳物件指標，就要確認被指向的東西活得比執行緒久。
//   5. hardware_concurrency() 是「提示」不是保證：本機實測回傳 16，
//      但標準允許回傳 0（表示無法判斷）。不可拿來當除數而不檢查。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】自由函式作為執行緒入口
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::thread t(greet); 這行執行完，greet 開始跑了嗎？
//     答：不保證。建構子回傳時，新執行緒可能已在跑、也可能還沒被排程，
//         兩者都合法。join() 的作用是「阻塞呼叫端直到它跑完」，
//         不是「啟動它」。想要確定的先後順序，必須用同步原語，
//         不能靠「我先建構所以它先跑」這種直覺。
//     追問：那 join() 和 detach() 差在哪？→ join 阻塞等待並回收資源；
//         detach 切斷關聯讓它自生自滅，之後你再也無法等它、也拿不回結果。
//
// 🔥 Q2. 為什麼 std::thread 的建構子要寫成可變參數樣板，而不是收函式指標？
//     答：收 void(*)() 就只能傳無參數自由函式，lambda（每個都是獨一無二的
//         匿名型別）、functor、成員函式全部進不來。用 variadic template
//         才能一個建構子吃下所有 Callable + 任意參數，並以 INVOKE 語意呼叫。
//     追問：代價是什麼？→ 樣板錯誤訊息很長，而且多了 most vexing parse
//         的坑（std::thread t(Task()); 會被當成函式宣告，見檔案 4）。
//
// ⚠️ 陷阱. 「函式執行緒裡的例外，可以在 join() 那邊用 try/catch 接到。」
//     答：錯。例外逃出執行緒函式時，直接呼叫 std::terminate() 讓整個程式中止
//         （標準保證，本機實測 exit=134），根本走不到 join() 那一行的 catch。
//         要跨執行緒傳例外，得在執行緒內部 catch 起來存成
//         std::exception_ptr，再由主執行緒 rethrow；或改用 std::future，
//         它會把例外存進共享狀態，在 get() 時重新丟出。
//     為什麼會錯：腦中把「執行緒」想成「函式呼叫」，以為呼叫堆疊是連續的。
//         實際上新執行緒有自己獨立的呼叫堆疊，main 的堆疊框根本不在它的
//         unwinding 路徑上，沒有任何 handler 可以接。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 原始範例：最基本的形式——把自由函式名直接交給 std::thread
// -----------------------------------------------------------------------------
void greet() {
    std::cout << "Hello!" << std::endl;
}

// -----------------------------------------------------------------------------
// 帶參數的自由函式：參數同樣被 decay-copy 進新執行緒的儲存區
// -----------------------------------------------------------------------------
void greetWithName(const std::string& name, int times) {
    for (int i = 0; i < times; ++i) {
        std::cout << "  Hello, " << name << "! (" << (i + 1) << ")" << std::endl;
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例】服務啟動時，用背景執行緒預熱唯讀快取
//   情境：Web 服務啟動後要立刻對外服務，但字典檔／設定檔載入要數百毫秒。
//   作法：主執行緒繼續完成監聽埠綁定，背景執行緒同時把設定載進快取，
//         最後 join 起來確保「對外開放前」快取已就緒。
//   為什麼用自由函式：這個載入邏輯是無狀態的純函式，沒有捕獲需求，
//         用自由函式最直接，也不會有生命週期問題（函式本體全程有效）。
//   注意：out 參數由呼叫端持有，其生命週期涵蓋整個執行緒 → 安全。
// -----------------------------------------------------------------------------
void loadConfigCache(const std::string& source, std::vector<std::string>* out) {
    // 實務上這裡是讀檔／打 DB；此處以固定資料模擬，讓輸出可重現
    static const char* kKeys[] = {"listen_port", "max_conn", "log_level"};
    for (const char* k : kKeys) {
        out->push_back(source + ":" + k);
    }
    std::cout << "  [warmup] 從 " << source << " 載入 " << out->size() << " 筆設定" << std::endl;
}

int main() {
    std::cout << "=== 基本形式：自由函式 ===" << std::endl;
    std::thread t(greet);
    t.join();

    std::cout << "\n=== 帶參數的自由函式 ===" << std::endl;
    // 參數被 decay-copy：這裡的字面值先轉成 std::string 複本存進執行緒儲存區
    std::thread t2(greetWithName, "Alice", 2);
    t2.join();

    std::cout << "\n=== std::thread 是 handle，不是執行緒本身 ===" << std::endl;
    std::cout << "sizeof(std::thread) = " << sizeof(std::thread)
              << " byte（本機實測，通常就是一個 pthread_t）" << std::endl;
    std::cout << "hardware_concurrency = " << std::thread::hardware_concurrency()
              << "（實作定義；標準允許回傳 0 表示無法判斷）" << std::endl;

    std::cout << "\n=== 日常實務：背景預熱設定快取 ===" << std::endl;
    std::vector<std::string> cache;
    std::thread warmup(loadConfigCache, "etc/app.conf", &cache);
    std::cout << "  [main] 同時進行：綁定監聽埠…" << std::endl;
    warmup.join();   // 對外開放前，確保快取已就緒
    std::cout << "  [main] 快取就緒，共 " << cache.size() << " 筆，首筆 = "
              << cache.front() << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.2：執行緒函式的多種形式1.cpp" -o callable1

// 註：最後一段裡 [main] 與 [warmup] 兩行的先後順序「每次執行都不同」

// === 預期輸出 ===
// （sizeof 與 hardware_concurrency 為實作定義值，換機器會不同；
//   本機為 x86-64 / GCC 15.2 / glibc 2.43，16 邏輯核心）
//
// === 基本形式：自由函式 ===
// Hello!
//
// === 帶參數的自由函式 ===
//   Hello, Alice! (1)
//   Hello, Alice! (2)
//
// === std::thread 是 handle，不是執行緒本身 ===
// sizeof(std::thread) = 8 byte（本機實測，通常就是一個 pthread_t）
// hardware_concurrency = 16（實作定義；標準允許回傳 0 表示無法判斷）
//
// === 日常實務：背景預熱設定快取 ===
//   [main] 同時進行：綁定監聽埠…
//   [warmup] 從 etc/app.conf 載入 3 筆設定
//   [main] 快取就緒，共 3 筆，首筆 = etc/app.conf:listen_port
//
//     （兩條執行緒並行輸出，且未加鎖）。其餘各段只有單一子執行緒且被 join
//     擋住，順序是確定的。
