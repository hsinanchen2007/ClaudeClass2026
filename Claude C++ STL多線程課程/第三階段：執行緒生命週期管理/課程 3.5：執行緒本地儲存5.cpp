// =============================================================================
//  課程 3.5：執行緒本地儲存5.cpp  —  thread_local 當「免鎖快取」用
// =============================================================================
//
// 【主題資訊 Information】
//   語法      : thread_local std::string cache;   // 需要動態初始化的型別
//   標準版本  : C++11
//   標頭檔    : 不需要(關鍵字);本例另需 <string>
//   儲存期    : 執行緒儲存期 —— 每執行緒一份,執行緒結束時銷毀
//   初始化時機: 該執行緒「第一次 odr-use 之前」惰性完成(不是執行緒一啟動就跑)
//   存取成本  : 每次存取多一次初始化守衛檢查(見【概念補充】實測)
//
// 【詳細解釋 Explanation】
//
// 【1. 快取為什麼特別適合 thread_local】
// 快取是典型的「共享會很痛」的資料結構:多執行緒共用一份快取,就必須用
// mutex 保護,於是每次查快取都要搶鎖 —— 快取本來是為了加速,結果鎖的爭用
// 反而變成新的瓶頸,在核心數多的機器上尤其明顯。
//
// thread_local 提供另一條路:讓每個執行緒各自持有一份快取。
//   * 完全不需要鎖,查詢就是一般的變數存取
//   * 沒有 cache line 在核心之間彈跳(false sharing 也一併消失)
// 代價是:
//   * 記憶體變成 N 份(N = 執行緒數)
//   * 執行緒 A 算過的結果,執行緒 B 享受不到,可能重複計算 N 次
//
// 【2. 這個取捨怎麼選】
//   選 thread_local 快取:計算便宜、結果小、命中率高、執行緒數可控。
//                        例如格式化用的緩衝區、小型查表、亂數引擎。
//   選共享快取 + 鎖    :計算昂貴(查資料庫、跑模型)、結果大、
//                        重算 N 次的代價遠高於一次鎖。
//   本檔示範的是前者;請注意輸出中 "Computing..." 出現了兩次 ——
//   兩個執行緒各算了一次,這正是「不共享」的代價,不是 bug。
//
// 【3. 惰性初始化:cache 什麼時候真的被建構?】
// thread_local std::string cache; 需要跑 std::string 的建構子,屬於動態初始化。
// 標準保證它在「該執行緒第一次 odr-use 之前」完成 —— 也就是第一次執行到
// cache.empty() 的那一刻,而不是執行緒剛啟動的時候。
// 推論:一個從頭到尾沒碰過 cache 的執行緒,它那一份 cache 可能從來不會被建構。
// 但要注意粒度:GCC 是「整個翻譯單元共用一個 __tls_guard」,
// 只要該執行緒碰了同一個 .cpp 裡任何一個需要動態初始化的 thread_local,
// 全部就會一起被建構(本課第 7 個範例檔有實測驗證)。
// 所以「沒用到就不會建立」不能拿來當資源管理的依據。
//
// 【4. 為什麼用 empty() 當「未初始化」的判斷】
// 本例用「字串是空的」代表「還沒算過」。這是個常見但有邊界的技巧:
// 如果計算結果本身就可能是空字串,這個判斷就會誤判成「沒算過」而不斷重算。
// 嚴謹的寫法是另外放一個旗標,或用 std::optional<std::string>:
//     thread_local std::optional<std::string> cache;   // C++17
//     if (!cache) cache = compute();
// 本檔在【日常實務範例 2】示範了 optional 版本。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 動態初始化的 thread_local 不是零成本(本機實測)
//   GCC 15.2.0 / x86-64 / -O2 下:
//     thread_local int a;              → 讀取只有一道 movl %fs:a@tpoff, %eax
//     thread_local std::string b;      → 每次存取前多一道
//                                        cmpb $0, %fs:__tls_guard@tpoff
//   編譯器還會為後者額外產生一個 __tls_init 函式。
//   也就是說:平凡型別的 thread_local 幾乎免費,而需要建構子的型別
//   每次存取都要付一次「初始化了嗎?」的檢查成本。
//   熱路徑上若真的很在意,可以改用平凡型別(如 char 陣列)自行管理。
//
// (B) 為什麼守衛檢查也是 thread_local 的
//   注意上面那道指令的 %fs: —— 連「初始化完成旗標」本身都放在 TLS 裡。
//   必須如此:每個執行緒的 cache 是不同物件,各自有各自的「建構了沒」狀態,
//   共用一個旗標會讓第二個執行緒誤以為自己那份已經建好了。
//
// (C) 和 function-local static 的對照
//     static std::string s;        // 全程式一份,C++11 起保證執行緒安全初始化
//     thread_local std::string s;  // 每執行緒一份,天生無競爭
//   兩者都是惰性初始化,但 static 的那次初始化需要真的上鎖(見課程 3.6),
//   thread_local 則只需要檢查自己那份的旗標,不必和別的執行緒協調。
//
// 【注意事項 Pay Attention】
// 1. 每執行緒一份 → 記憶體乘以執行緒數。thread_local 放大物件在
//    高併發伺服器上會非常吃記憶體。
// 2. 執行緒池的執行緒是重複使用的:thread_local 快取會跨「任務」殘留。
//    這有時是優點(快取命中),有時是嚴重的 bug(前一個使用者的資料
//    洩漏給下一個請求)。放使用者相關資料時務必在任務結束時清乾淨。
// 3. 用 empty()/0 這類「哨兵值」判斷未初始化,遇到「結果本身就是哨兵值」
//    的情況會無限重算,見【詳細解釋 4】。
// 4. thread_local 的解構子在執行緒結束時執行;detach 的執行緒若在行程
//    結束時還活著,解構子不保證會被執行。
// 5. std::cout 並行輸出不會有 data race,但不保證一整行不被切開;
//    本檔用 mutex 保護輸出,避免把「輸出交錯」誤判成「快取失效」。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】thread_local 快取
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 用 thread_local 做快取,和「共享一份快取 + mutex」相比,
//        各自的優缺點是什麼?
//     答：thread_local 完全免鎖,查詢就是普通變數存取,也沒有 cache line
//         在核心間來回的問題;代價是記憶體變成 N 份,而且同一筆結果可能
//         被 N 個執行緒各算一次。共享快取正好相反:只算一次、只存一份,
//         但每次存取都要搶鎖,核心數一多鎖就成為瓶頸。
//     追問：那什麼時候該選共享快取?
//         → 當「重算一次」的成本遠高於「搶一次鎖」時。例如結果來自資料庫
//           查詢或模型推論,重算 16 次的代價絕對比鎖貴。
//
// 🔥 Q2. thread_local std::string 在執行緒啟動的瞬間就被建構了嗎?
//     答：不是。它需要跑建構子,屬於動態初始化,標準只保證
//         「在該執行緒第一次 odr-use 之前」完成 —— 是惰性的。
//         一個從沒碰過它的執行緒,那份物件根本不會被建構。
//     追問：這個惰性有成本嗎?
//         → 有。本機實測每次存取前都會多一道 cmpb $0, %fs:__tls_guard@tpoff
//           的守衛檢查,編譯器也會多產生一個 __tls_init 函式。
//           平凡型別(如 int)則沒有這個成本。
//
// ⚠️ 陷阱. 「thread_local 快取在執行緒池裡用最安全了,反正每個執行緒
//         自己一份,不會互相干擾。」哪裡錯了?
//     答：執行緒池的執行緒是「重複使用」的。同一條執行緒會先後處理
//         使用者 A 和使用者 B 的請求,而 thread_local 的生命週期綁的是
//         「執行緒」不是「任務」—— A 留下的快取內容會原封不動被 B 讀到。
//         這在真實系統裡是會上新聞的資料外洩(A 的購物車出現在 B 的頁面)。
//     為什麼會錯：把 thread_local 的作用域誤記成「一個任務」或「一次請求」,
//         但它其實是「一條執行緒的一生」。執行緒池讓這兩者不再等價,
//         正確做法是在任務開始或結束時明確清空。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

// 保護 std::cout,避免多執行緒輸出互相切開
std::mutex g_ioMutex;

// -----------------------------------------------------------------------------
// 原始示範:每執行緒一份的計算快取
// -----------------------------------------------------------------------------
thread_local std::string cache;

std::string expensiveCompute(int id) {
    if (cache.empty()) {
        // 模擬耗時計算
        cache = "Result for thread " + std::to_string(id);
        std::lock_guard<std::mutex> lock(g_ioMutex);
        std::cout << "  Computing..." << std::endl;
    }
    return cache;
}

void worker(int id) {
    // 注意:先算完、再上鎖印出。若寫成「持有 g_ioMutex 的期間呼叫
    // expensiveCompute()」,而它內部又要鎖同一把 std::mutex,就會 self-deadlock:
    // std::mutex 不可重入,同一條執行緒重複上鎖是未定義行為(實務上就是卡死)。
    // 需要重入請用 std::recursive_mutex,但通常代表設計該調整。
    std::string first = expensiveCompute(id);
    {
        std::lock_guard<std::mutex> lock(g_ioMutex);
        std::cout << "  " << first << std::endl;
    }

    // 第二次呼叫直接命中快取,不會再出現 "Computing..."
    std::string second = expensiveCompute(id);
    std::lock_guard<std::mutex> lock(g_ioMutex);
    std::cout << "  " << second << " (來自快取)" << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】執行緒區域的格式化緩衝區(避免每次輸出都配置記憶體)
//   情境: 高頻 log 系統,每秒數十萬行。若每行都新建一個 std::string,
//         allocator 會成為熱點。做法是每個執行緒重複使用同一塊緩衝區:
//         clear() 保留 capacity,後續 append 幾乎不再配置記憶體。
//   為什麼用本主題: 緩衝區必須每執行緒一份,否則兩條執行緒會互相踩爛內容;
//                   而 thread_local 讓它免鎖又免配置。
// -----------------------------------------------------------------------------
thread_local std::string t_logBuffer;

const std::string& formatLogLine(const char* level, const std::string& msg) {
    t_logBuffer.clear();          // 保留已配置的 capacity,不釋放記憶體
    t_logBuffer += '[';
    t_logBuffer += level;
    t_logBuffer += "] ";
    t_logBuffer += msg;
    return t_logBuffer;
}

void logWorker(int id) {
    formatLogLine("INFO", "worker " + std::to_string(id) + " 啟動");
    std::size_t capAfterFirst = t_logBuffer.capacity();

    for (int i = 0; i < 100; ++i) {
        formatLogLine("DEBUG", "處理第 " + std::to_string(i) + " 筆");
    }

    std::lock_guard<std::mutex> lock(g_ioMutex);
    std::cout << "  worker " << id << " 跑完 101 行,緩衝區 capacity 由 "
              << capAfterFirst << " 變成 " << t_logBuffer.capacity()
              << "(沒有隨行數增長 → 沒有反覆配置記憶體)" << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】用 std::optional 取代「空字串代表沒算過」
//   情境: 快取的結果本身有可能是空字串(例如查不到暱稱的使用者)。
//         用 empty() 當哨兵會讓這種使用者「每次都重算」,快取形同失效。
//   為什麼用本主題: 示範【詳細解釋 4】提到的哨兵值陷阱的正解。
// -----------------------------------------------------------------------------
thread_local std::optional<std::string> t_nicknameCache;
thread_local int t_computeCount = 0;      // 記錄真的算了幾次

std::string lookupNickname(int userId) {
    if (!t_nicknameCache) {               // 只看「有沒有值」,不看值是什麼
        ++t_computeCount;
        // 假設這位使用者沒有設定暱稱 → 結果就是空字串
        t_nicknameCache = (userId == 42) ? std::string{} : std::string("Alice");
    }
    return *t_nicknameCache;
}

void nicknameWorker(int userId) {
    for (int i = 0; i < 3; ++i) {
        lookupNickname(userId);
    }
    std::lock_guard<std::mutex> lock(g_ioMutex);
    std::cout << "  user " << userId << " 查了 3 次,實際計算 "
              << t_computeCount << " 次(結果是空字串也只算一次)" << std::endl;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔從缺,理由如下
//   LeetCode 的並行題(1114 Print in Order、1115 Print FooBar Alternately、
//   1116 Print Zero Even Odd、1117 Building H2O、1195 Fizz Buzz Multithreaded)
//   全部在考「執行緒之間如何同步、如何看見彼此的進度」,靠的是共享狀態。
//   thread_local 的作用恰好相反 —— 它讓執行緒彼此看不見對方的資料,
//   套進那些題目只會讓答案錯掉。設計題(146 LRU Cache 等)雖然也叫「快取」,
//   但考的是資料結構本身而非執行緒儲存期。因此本檔誠實從缺,
//   改用上面兩個真實工程情境示範。
// -----------------------------------------------------------------------------

int main() {
    std::cout << "=== 原始示範:每執行緒一份的快取 ===" << std::endl;
    std::cout << "(注意 Computing... 出現兩次 —— 兩個執行緒各算一次,"
                 "這是「不共享」的代價)" << std::endl;
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    t1.join();
    t2.join();

    std::cout << "\n=== 主執行緒的 cache 從未被 worker 填過 ===" << std::endl;
    std::cout << "  主執行緒 cache.empty() = " << std::boolalpha << cache.empty()
              << std::endl;

    std::cout << "\n=== 實務 1:執行緒區域的格式化緩衝區 ===" << std::endl;
    std::thread l1(logWorker, 1);
    std::thread l2(logWorker, 2);
    l1.join();
    l2.join();

    std::cout << "\n=== 實務 2:用 optional 避開「空字串=沒算過」的陷阱 ===" << std::endl;
    std::thread n1(nicknameWorker, 42);   // 這位使用者的暱稱就是空字串
    n1.join();

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.5：執行緒本地儲存5.cpp" -o tls_cache

// 注意:以下為某一次實際執行的結果。
//   * 兩個 worker 之間的行序每次執行都可能不同(t2 可能整組排在 t1 之前)。
//   * 緩衝區 capacity 的實際數值是「實作定義」的(libstdc++ 的 SSO 門檻與
//     成長策略),換編譯器或標準函式庫可能不同;重點是它「不隨行數增長」。

// === 預期輸出 ===
// === 原始示範:每執行緒一份的快取 ===
// (注意 Computing... 出現兩次 —— 兩個執行緒各算一次,這是「不共享」的代價)
//   Computing...
//   Result for thread 1
//   Computing...
//   Result for thread 2
//   Result for thread 2 (來自快取)
//   Result for thread 1 (來自快取)
//
// === 主執行緒的 cache 從未被 worker 填過 ===
//   主執行緒 cache.empty() = true
//
// === 實務 1:執行緒區域的格式化緩衝區 ===
//   worker 1 跑完 101 行,緩衝區 capacity 由 30 變成 30(沒有隨行數增長 → 沒有反覆配置記憶體)
//   worker 2 跑完 101 行,緩衝區 capacity 由 30 變成 30(沒有隨行數增長 → 沒有反覆配置記憶體)
//
// === 實務 2:用 optional 避開「空字串=沒算過」的陷阱 ===
//   user 42 查了 3 次,實際計算 1 次(結果是空字串也只算一次)
