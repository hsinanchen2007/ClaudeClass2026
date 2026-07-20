// =============================================================================
//  課程 2.4：join() 與 detach()3.cpp  —  detach + 參考捕獲 = 懸空
// =============================================================================
//
// 【本檔含一個「真正的未定義行為(UB)」示範,預設不執行 —— 見下方說明】
//
// 【主題資訊 Information】
//   問題語法  : std::thread t([&localVar]{ ... });  t.detach();
//   標準版本  : C++11
//   標頭檔    : <thread>、<chrono>
//   錯誤分類  : 真正的未定義行為 —— 存取已結束生命週期的自動儲存期物件
//               (不是標準規定的中止,因此結果無法預測、不可斷言)
//   本機觀察  : 曾印出 0(而非 42);但這只是某一次的觀察,不是規律
//
// 【詳細解釋 Explanation】
//
// 【1. 這個 bug 的結構】
//     void dangerous() {
//         int localVar = 42;
//         std::thread t([&localVar]{ ...; std::cout << localVar; });
//         t.detach();
//     }   // ← localVar 在這裡被銷毀
//
// 三件事湊在一起才會出事,少一件都不會:
//   (a) lambda 用「參考」捕獲了區域變數 → 閉包裡存的是位址,不是值;
//   (b) 呼叫了 detach() → 執行緒的壽命與 dangerous() 完全脫鉤;
//   (c) 執行緒真正讀取 localVar 的時機,晚於 dangerous() 返回。
// 於是執行緒讀的是一塊已經歸還的堆疊記憶體 —— 未定義行為。
//
// 本例的 sleep_for(100ms) 更是把 (c) 從「可能」變成「幾乎必然」:
// 它保證執行緒在讀取之前先睡 100 毫秒,而 dangerous() 早就返回了。
// (原始範例刻意這樣寫,就是為了讓這個 bug 穩定地暴露出來。)
//
// 【2. 為什麼印出來的是 0 而不是 42,也不是崩潰】
// 這正是 UB 最難教的地方。那塊堆疊記憶體在 dangerous() 返回後
// 並沒有「消失」—— 頁面仍然映射著,只是不再屬於任何人。
// 之後任何函式呼叫(包括 sleep_for 自己、iostream 內部)都可能
// 在那塊區域上建立新的堆疊框,把原本的 42 覆寫成別的東西。
// 本機實測讀到 0,那只是「剛好被覆寫成 0」,不是規律。
//
// ⚠️ 三種可能都合法:讀到 42(還沒被覆寫)、讀到亂數、程式崩潰。
//    正因為沒有正確答案,本檔不把任何一次的觀察寫進預期輸出。
//
// 【3. 兩種修法,以及它們的差別】
//
//   (a) 改用「值捕獲」—— 治本
//         std::thread t([localVar]{ ... });   // 複製一份進閉包
//       執行緒操作的是自己的副本,和 dangerous() 的區域變數再無關係。
//       這是 detach 情境下唯一正確的心法:**把資料完整帶走,不要借用。**
//
//   (b) 改用 join() 而不是 detach() —— 治標
//         t.join();   // 在 localVar 銷毀之前就等它跑完
//       有效,因為 join() 把執行緒的壽命綁回這個作用域。
//       ⚠️ 但它脆弱:整段程式的正確性依賴「呼叫者永遠記得 join」,
//          任何人日後改回 detach,UB 就無聲無息地回來了。
//
// 準則:**只要決定要 detach,就必須用值捕獲(或 shared_ptr),
//       絕不可用 [&] 或 [this] 捕獲區域範圍的東西。**
//
// 【4. [this] 是最容易忽略的那一個】
// 在成員函式裡寫 std::thread t([this]{ ... }); 看起來很無害,
// 但 [this] 捕獲的是指標 —— 和 [&] 一樣危險。若物件先被銷毀而
// 執行緒還在跑,就是懸空。C++17 起可以用 [*this] 複製整個物件來避開。
// 這在「物件持有一條背景執行緒」的設計中是頭號陷阱,
// 正確做法是讓解構子先停止並 join 那條執行緒(RAII)。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼編譯器不警告
//   編譯器要證明「這個執行緒會在變數銷毀後才讀它」,等於要做跨執行緒的
//   生命週期分析,一般情況下不可判定。GCC/Clang 只能在最單純的情形給
//   -Wdangling-pointer 之類的提示,對本例這種跨執行緒的則無能為力。
//   這類 bug 的偵測要靠工具,見下面 (B)。
//
// (B) 用 sanitizer 讓它現形
//   AddressSanitizer 會明確報出 stack-use-after-return:
//       g++ -std=c++17 -fsanitize=address -g -pthread -DDEMONSTRATE_UB 本檔
//   (需搭配環境變數 ASAN_OPTIONS=detect_stack_use_after_return=1,
//    新版 GCC 多半已預設開啟。)
//   ThreadSanitizer(-fsanitize=thread)則專門抓資料競爭。
//   多執行緒程式應該定期用這兩者跑一次測試 —— 它們能抓到
//   「看起來正常」的錯誤,這正是人眼與一般測試最弱的地方。
//
// (C) 這和第 5 個範例檔(懸空 char*)是同一個病
//   那裡是 std::thread 複製了一個指標,這裡是 lambda 閉包存了一個參考。
//   形式不同,本質完全一樣:**執行緒手上握著一個位址,
//   而那個位址所指的東西比執行緒先死。**
//   認得這個模式,就能一眼看出絕大多數多執行緒的生命週期 bug。
//
// 【注意事項 Pay Attention】
// 1. 這是真正的未定義行為,不可斷言它會印什麼、也不可斷言它會崩潰。
//    「測起來正常」完全不代表它是對的。
// 2. 決定 detach 就必須值捕獲或用 shared_ptr,絕不可 [&] 捕獲區域變數。
// 3. [this] 和 [&] 一樣危險;C++17 可用 [*this] 複製物件。
// 4. 用 join 來修只是把風險藏起來,不是根治。
// 5. 本檔的 UB 示範預設不執行,要看它必須加 -DDEMONSTRATE_UB 重新編譯。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】detach 與懸空的捕獲
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. lambda 用 [&] 捕獲區域變數再 detach,為什麼是錯的?
//     答：[&] 捕獲存的是位址而不是值,而 detach() 讓執行緒的壽命與
//         建立它的函式完全脫鉤。函式一返回,那個區域變數就被銷毀,
//         執行緒之後讀到的是一塊已歸還的堆疊記憶體 —— 未定義行為。
//     追問：那為什麼有時候印出來的值還是對的?
//         → 因為那塊記憶體通常還映射著,只是不再屬於任何人。
//           在它被新的堆疊框覆寫之前讀,就會「剛好」讀到舊值。
//           本機實測讀到的是 0 而不是 42,正說明它已經被覆寫了。
//
// 🔥 Q2. 這個問題怎麼修?
//     答：治本是改用值捕獲 [localVar],讓執行緒擁有自己的副本;
//         治標是把 detach 改成 join,讓執行緒在變數銷毀前跑完。
//         後者有效但脆弱 —— 正確性依賴「永遠記得 join」這個約定。
//         準則是:一旦決定 detach,所有資料都要以值或 shared_ptr 完整帶走。
//     追問：成員函式裡的 [this] 呢?
//         → 一樣危險,它捕獲的是指標。物件先銷毀就是懸空。
//           C++17 可用 [*this] 複製整個物件;更好的做法是讓解構子
//           先停止並 join 那條背景執行緒。
//
// ⚠️ 陷阱. 「這種 bug 只要 code review 仔細一點就能抓到,
//         畢竟 [&] 和 detach() 就寫在隔壁兩行。」哪裡錯了?
//     答：在教科書的 10 行範例裡確實一眼就看到。但真實程式碼中,
//         建立執行緒的地方和 detach() 常常隔了好幾層函式呼叫,
//         被捕獲的變數也可能是「某個物件的成員,而該物件由呼叫者擁有」——
//         這時要判斷生命週期,得先看懂整條呼叫鏈的所有權設計。
//         更別說 [this] 這種形式,連 & 符號都看不到。
//     為什麼會錯：把教學範例的可讀性,誤當成真實程式碼的可讀性。
//         正確的態度是不依賴人眼:用 AddressSanitizer 與 ThreadSanitizer
//         定期跑測試,並在設計層面訂下「detach 的執行緒一律不得
//         參考呼叫端資料」這種可機械檢查的規則。
// ═══════════════════════════════════════════════════════════════════════════

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

// 保護 std::cout,避免多執行緒輸出互相切開
std::mutex g_ioMutex;

void say(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_ioMutex);
    std::cout << msg << std::endl;
}

#ifdef DEMONSTRATE_UB
// -----------------------------------------------------------------------------
// ⚠️ 未定義行為示範 —— 預設「不編譯、不執行」
//    要親眼看它,請加上 -DDEMONSTRATE_UB 重新編譯(見檔尾)。
//    它的輸出本質上不可預測:可能是 42、可能是 0、可能是亂數,也可能崩潰。
//    把不可預測的結果寫進「預期輸出」是錯誤的示範,所以預設關閉。
// -----------------------------------------------------------------------------
void dangerous() {
    int localVar = 42;

    std::thread t([&localVar]() {          // ⚠️ 參考捕獲
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "  UB 示範讀到的值: " << localVar << std::endl;  // 危險!
    });

    t.detach();
}   // localVar 在這裡被銷毀,但執行緒還要再睡 100ms 才讀它
#endif

// -----------------------------------------------------------------------------
// 修法 (a):值捕獲 —— 治本,detach 情境下的唯一正解
// -----------------------------------------------------------------------------
void safeByValueCapture() {
    int localVar = 42;

    std::thread t([localVar]() {           // ✅ 複製一份進閉包
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        say("    [值捕獲] 讀到的值: " + std::to_string(localVar));
    });

    t.detach();
}   // localVar 銷毀完全不影響執行緒 —— 它有自己的副本

// -----------------------------------------------------------------------------
// 修法 (b):改用 join() —— 治標,有效但脆弱
// -----------------------------------------------------------------------------
void safeByJoin() {
    int localVar = 99;

    std::thread t([&localVar]() {          // 參考捕獲……
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        say("    [join] 讀到的值: " + std::to_string(localVar));
    });

    t.join();                              // ……但 join 保證它在變數死前跑完
}

// -----------------------------------------------------------------------------
// 【日常實務範例】物件持有背景執行緒:[this] 的陷阱與 RAII 解法
//   情境: 一個 Poller 物件在背景週期性檢查狀態。最自然的寫法是
//         std::thread t([this]{ loop(); });,但 [this] 捕獲的是指標 ——
//         若物件先被銷毀而執行緒還在跑,就是懸空存取成員。
//   為什麼用本主題: 這是【詳細解釋 4】在真實設計中的樣子,
//         也是「物件 + 背景執行緒」這個常見組合的標準解法:
//         用 atomic 旗標通知停止,並在解構子裡 join,
//         保證執行緒絕不會活得比物件久。
// -----------------------------------------------------------------------------
class Poller {
    std::string       name_;
    std::atomic<bool> running_{true};
    int               ticks_ = 0;
    std::thread       worker_;

public:
    explicit Poller(std::string name) : name_(std::move(name)) {
        // [this] 在這裡是安全的 —— 因為解構子保證會先停止並 join
        worker_ = std::thread([this]() {
            while (running_.load()) {
                ++ticks_;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            // 刻意不印 ticks_ —— 它的值取決於排程與系統負載,每次執行都不同,
            // 印出來會讓「預期輸出」變成無法比對的浮動值。
            // 這裡只回報「確實跑過而且已乾淨停止」這個確定的事實。
            say("    [" + name_ + "] 背景迴圈已停止(期間確實執行過: " +
                std::string(ticks_ > 0 ? "是" : "否") + ")");
        });
    }

    ~Poller() {
        running_.store(false);          // 先通知停止
        if (worker_.joinable()) {
            worker_.join();             // 再等它真的停下來
        }
        // 走到這裡,執行緒保證已結束 → this 絕不可能被懸空存取
    }

    // 持有執行緒的類別不應該被隨意複製
    Poller(const Poller&) = delete;
    Poller& operator=(const Poller&) = delete;
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔從缺,理由如下
//   本檔的主題是「物件生命週期與未定義行為」,而 LeetCode 無法考這個:
//   它只檢查輸出是否正確,而 UB 最典型的表現恰恰是「輸出也是對的」。
//   並行題(1114 Print in Order、1115 Print FooBar Alternately、
//   1116 Print Zero Even Odd、1117 Building H2O、1195 Fizz Buzz Multithreaded)
//   由評測框架管理執行緒與物件存活,懸空捕獲的情境根本不會出現。
//   本課第 7 個範例檔會示範真正對應得上的 1195,此處從缺。
// -----------------------------------------------------------------------------

int main() {
    std::cout << "=== 修法 (a):值捕獲(治本) ===" << std::endl;
    safeByValueCapture();
    std::this_thread::sleep_for(std::chrono::milliseconds(120));

    std::cout << "\n=== 修法 (b):改用 join()(治標,脆弱) ===" << std::endl;
    safeByJoin();

    std::cout << "\n=== 實務:物件持有背景執行緒,用 RAII 保證不懸空 ===" << std::endl;
    {
        Poller p("health-check");
        std::this_thread::sleep_for(std::chrono::milliseconds(55));
    }   // p 解構 → 通知停止 → join → 執行緒保證先於物件結束
    std::cout << "  ↑ 解構子先停止再 join,所以 [this] 永遠不會懸空" << std::endl;

    std::cout << "\n=== 未定義行為示範 ===" << std::endl;
#ifdef DEMONSTRATE_UB
    std::cout << "  (已啟用 -DDEMONSTRATE_UB,接下來的輸出無法預測)" << std::endl;
    dangerous();
    std::this_thread::sleep_for(std::chrono::seconds(1));
#else
    std::cout << "  已跳過(預設關閉)。" << std::endl;
    std::cout << "  該示範會在區域變數銷毀後才讀取它,結果無法預測 ——"
              << std::endl;
    std::cout << "  可能是 42、可能是 0、可能是亂數,也可能崩潰。" << std::endl;
    std::cout << "  正因為沒有正確答案,它不適合被寫進「預期輸出」。"
              << std::endl;
    std::cout << "  要親眼看它:g++ -std=c++17 -Wall -Wextra -pthread"
                 " -DDEMONSTRATE_UB 本檔 -o ub_demo" << std::endl;
    std::cout << "  想讓它現形:再加上 -fsanitize=address -g,"
                 "ASan 會明確報出 stack-use-after-return" << std::endl;
#endif

    return 0;
}

// 編譯(預設,安全且輸出可重現):
//   g++ -std=c++17 -Wall -Wextra -pthread "課程 2.4：join() 與 detach()3.cpp" -o detach_capture
//
// 編譯(啟用 UB 示範,輸出不可預測,僅供實驗):
//   g++ -std=c++17 -Wall -Wextra -pthread -DDEMONSTRATE_UB "課程 2.4：join() 與 detach()3.cpp" -o ub_demo
//
// 讓 UB 現形(強烈建議親自跑一次):
//   g++ -std=c++17 -fsanitize=address -g -pthread -DDEMONSTRATE_UB 本檔 -o ub_asan

// 注意:以下為「預設編譯」(未啟用 -DDEMONSTRATE_UB)的實際執行結果。
//   * 每一行每次執行都相同 —— Poller 刻意不印會隨排程浮動的迴圈次數,
//     只回報「確實執行過且已乾淨停止」這個確定的事實。
//   * 若你自行加上 -DDEMONSTRATE_UB,最後一段的輸出將無法預測 ——
//     那正是未定義行為的定義,不要把任何一次的結果當成「正確答案」。

// === 預期輸出 ===
// === 修法 (a):值捕獲(治本) ===
//     [值捕獲] 讀到的值: 42
//
// === 修法 (b):改用 join()(治標,脆弱) ===
//     [join] 讀到的值: 99
//
// === 實務:物件持有背景執行緒,用 RAII 保證不懸空 ===
//     [health-check] 背景迴圈已停止(期間確實執行過: 是)
//   ↑ 解構子先停止再 join,所以 [this] 永遠不會懸空
//
// === 未定義行為示範 ===
//   已跳過(預設關閉)。
//   該示範會在區域變數銷毀後才讀取它,結果無法預測 ——
//   可能是 42、可能是 0、可能是亂數,也可能崩潰。
//   正因為沒有正確答案,它不適合被寫進「預期輸出」。
//   要親眼看它:g++ -std=c++17 -Wall -Wextra -pthread -DDEMONSTRATE_UB 本檔 -o ub_demo
//   想讓它現形:再加上 -fsanitize=address -g,ASan 會明確報出 stack-use-after-return
