// =============================================================
// 02_data_race.cpp  --  多執行緒為何困難:資料競爭 (data race)
// =============================================================
//
// 本課目標:
//   1. 用自己的眼睛看到一個真實的資料競爭。
//   2. 理解為何 `counter++` *不是* 一個不可分割的原子操作。
//   3. 為下一課 (lesson 03) 做動機鋪陳 —— std::mutex 將會修好它。
//
// 編譯方式:
//     g++ -std=c++17 -O2 -pthread 02_data_race.cpp -o 02_data_race
//
// 執行方式 (請多執行幾次!):
//     ./02_data_race
//
// 預期結果:
//     兩個執行緒各自把共享的 counter 加 1 共 1,000,000 次。
//     「正確」答案應該是 2,000,000。
//     但你幾乎一定會看到比較 *小* 的數字,而且每次執行的
//     數字都 *不一樣*。這就是資料競爭。
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     資料競爭 (data race) 是什麼,為何 ++counter 不安全
// 前置課程: lesson 01
// 觀念詞彙:
//   - data race           ── 至少一個是寫、且無同步,結果為 UB
//   - undefined behavior  ── 標準允許「任何結果」,實務 = 隨機壞
//   - LOAD/ADD/STORE      ── ++counter 在 CPU 上是三步,不是一步
// 新介紹 API: 無 (本課示範問題,不引入新 API)
// 觀察重點:
//   - -O0 跑 → race 明顯,結果隨機
//   - -O2 跑 → 編譯器可能把迴圈合併成單次寫,race「藏起來」
// 何時你會踩到 data race:
//   - 任何時候多 thread 同時碰一個變數,且至少一邊在寫
// 常見錯誤:
//   - 「結果看起來大致對」就以為沒事 → 那是 UB 的偽裝
//   - 用 volatile 想解決 → volatile 不是同步原語
//   - 用更高的 -O 等級「隱藏」問題 → 在另一台機器照樣會炸
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── data race 在機器層真正的樣子
// =============================================================
//
// 1. ++counter 為什麼不是「一條指令」
//    -O0 下 `counter++` 在 x86-64 展開成三步:
//        mov  rax, [counter]     ; load
//        add  rax, 1             ; modify
//        mov  [counter], rax     ; store
//    兩條 thread 都 load 到同一個值各自 +1 再 store,後寫的覆蓋前寫的
//    → 「lost update」。這是 race 最直觀的後果。
//    -O2 編譯器常合併成 `inc [counter]` 單一 memory-RMW 指令,但這在
//    硬體上仍非原子;只有加 LOCK 前綴 (`lock inc`) 的版本才會把整條
//    cache line 鎖到自己核上。`std::atomic::fetch_add` 編出來就是這個。
//
// 2. data race ≠ race condition
//    - data race:對同一個非 atomic 變數,兩條 thread 同時存取且至少
//      一條是寫,且無同步關係 → C++ 直接判 *Undefined Behavior*。
//    - race condition:邏輯依賴執行順序的時序漏洞 (例如 check-then-act)。
//      可以沒有 data race (有用 mutex) 但仍有 race condition。
//    本課示範 data race;廣義 race condition 在 lesson 21。
//
// 3. 為什麼 volatile 不是答案 (Java/C# 腦傷警告)
//    C++ volatile 只承諾「編譯器不省略此讀寫、不重排到其他 volatile 之外」。
//    它 *不保證*:
//      - atomicity (RMW 仍可被打斷)
//      - visibility (一條 thread 寫的值另一條不一定看得到)
//      - ordering (與非 volatile 存取間沒有 happens-before)
//    對 MMIO 有用,對多執行緒同步無用。Java volatile 比 C++ 強 (有
//    acquire/release 語意);C# volatile 類似但仍弱。三家不能混用。
//
// 4. 為什麼 -O0 vs -O2 race 表現不同
//    -O0:每次 ++ 都實際 load+add+store,race 視窗大、易觸發。
//    -O2:編譯器可能把 counter 暫存在 register、hoist 整個迴圈,
//         看似「答案剛好對」── 但這不是「沒 race」,只是視窗窄到
//         在你的硬體上很少觸發。換 ARM、換負載立刻爆。
//         所以「-O2 跑過,答案對」不算驗證。
//
// 5. 順序也會變
//    現代 CPU (尤其 ARM、RISC-V) 在硬體層 out-of-order 執行,把 store
//    重排在 load 後 (store buffer)。即使每條指令看似原子,兩條 thread
//    看到的順序可不同。lesson 11 的 memory model 精準描述此現象。
//    Data race 一旦發生,連「結果是這個或那個」都不能保證 ── 標準判
//    UB,意思是 *什麼都可能發生*,包含 segfault、死迴圈、莫名其妙的值。
//
// 6. 怎麼修
//    本課不修,目的是先親眼看到 race。lesson 03 用 mutex,04 用
//    atomic,11 用 memory order 細調。預設的「不加同步」永遠錯。
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目:
//   ✗ 本課示範的是「錯誤示範」── data race 是要 *避免* 的問題,不是
//     LC 出題的方向。LC 上 *所有* concurrency 題目若你刻意不加同步,
//     都會看到本課 demo 的 lost update 現象。可以拿 lesson 30 的 Q5
//     (Bounded Blocking Queue) 把 mutex 拔掉跑跑看,印證本課觀察。
//
// 主要 API 對照 (cppreference):
//   - C++ memory model 概論              https://en.cppreference.com/w/cpp/atomic/memory_order
//   - C++ data race 定義 (§intro.races)  https://en.cppreference.com/w/cpp/language/multithread
//   - std::atomic (lesson 04 解法)       https://en.cppreference.com/w/cpp/atomic/atomic
//
// 練習建議:
//   讀完本課了解 race 長什麼樣,接著:
//     - lesson 03:用 mutex 修
//     - lesson 04:用 atomic 修
//     - lesson 14:用 ThreadSanitizer 自動偵測 race
// =============================================================

/*
補充筆記：std::data_race
  - data race 是 C++ memory model 中的 undefined behavior，不只是「答案偶爾錯」。
  - 同一記憶體位置若至少一方寫入，且沒有 happens-before 關係，就需要同步。
  - 修正方式不是加 sleep，而是使用 mutex、atomic 或改成 message passing。
  - std::data_race 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Data race：定義、UB 後果、與 race condition 的區別
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. Data race 與 Race condition 的差別？
//     答：Data race 是語言層面的定義——兩個 thread 並行存取同一記憶體位置、至少一個
//         是寫、且未以同步關係排序，結果是 UB。Race condition 是邏輯層面——程式正確性
//         取決於事件的相對時序。兩者互不蘊含：每個操作各自上鎖可消滅 data race，但
//         if (m.count(k)) m.at(k) 這種 check-then-act 仍是 race condition。
//     追問：舉一個「沒有 data race 但有 race condition」的例子？（TOCTOU、先查餘額
//           再扣款）
//
// 🔥 Q2. counter++ 為什麼不是原子的？
//     答：它是 read-modify-write 三步（載入、加一、寫回）。兩個 thread 可能同時讀到
//         同一個舊值、各自加一後寫回，一次更新就此消失。要原子必須用 std::atomic 的
//         fetch_add（單一 RMW 指令）或用 mutex 保護整段。
//     追問：加了 -O0 就會正確嗎？（不會，優化等級只影響出錯機率，不影響正確性）
//
// 🔥 Q3. 為什麼 data race 是 UB，而不是「頂多讀到舊值」？
//     答：因為編譯器在優化時被允許假設程式沒有 data race。基於這個假設它可以把變數
//         提升到暫存器、合併或重排載入與儲存、甚至產生 invented write。一旦假設被
//         打破，觀察到的行為就完全無法用「舊值／新值」來推理，包括撕裂讀寫與無窮迴圈。
//     追問：怎麼在執行期抓？（ThreadSanitizer，見 lesson 14）
//
// ⚠️ 陷阱. 兩個 thread 各自 push_back 到同一個 vector 不安全；那寫入「不同元素」呢？
//     答：寫入不同元素是安全的——標準保證對不同記憶體位置的並行存取不構成 data race。
//         唯一例外是 std::vector<bool>：它是位元壓縮的特化，相鄰元素共用同一個 byte，
//         並行寫入不同索引就是 data race。平行填值請用 std::vector<char>。
//     為什麼會錯：大家記得「容器不是 thread-safe」，卻不知道「不同元素」本來是安全的，
//         也不知道 vector<bool> 破壞了這個保證。（push_back 則永遠不安全：會同時寫
//         size，還可能 reallocation 使所有指標失效。）
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <vector>
#include <atomic>

// -------------------------------------------------------------
// 一個普通的全域 int —— 由兩個執行緒共享。
// 這就是我們刻意要破壞的變數,讓你親眼看到所謂的
// 「未定義行為 (undefined behavior)」長什麼樣子。
// -------------------------------------------------------------
long long counter = 0;

// 每個執行緒都執行這個函式。
// 它迴圈 N 次,每次做 counter++。
//
// 在 C++ 原始碼中 `counter++` 看起來只是「一步」,但 CPU
// 實際上會做 *三個* 獨立的步驟:
//
//     1. LOAD  : 從記憶體讀 counter 進暫存器
//     2. ADD   : 把暫存器內的值加 1
//     3. STORE : 把暫存器寫回記憶體
//
// 現在想像兩個執行緒同時都在 step 1:
//
//   Thread A: LOAD  counter (讀到 100)
//   Thread B: LOAD  counter (也讀到 100!)
//   Thread A: ADD   -> 101
//   Thread B: ADD   -> 101
//   Thread A: STORE 101
//   Thread B: STORE 101                   <-- 一次遞增「不見了」
//
// 我們做了兩次 counter++,但 counter 只增加 1。
// 把這放大到一百萬次,就會丟失非常多次計數。
void increment(int times)
{
    for (int i = 0; i < times; ++i) {
        ++counter;          // <-- 會引發競爭的那一行
    }
}

// -------------------------------------------------------------
// 實戰範例 1: 共享 histogram 的 race
// -------------------------------------------------------------
// 應用場景: 統計每個請求落到哪個 bucket (例如 status code 200/
// 404/500 各幾次)。N 條 worker thread 各自處理一批請求, 每筆
// 結果都要 ++histogram[code]。
//
// 這是 ++counter 的「多個格子版本」── 每格仍是 race。
// 修法 (進階版會在 lesson 03 mutex / lesson 04 atomic 看到):
//   - 每個 bucket 換成 std::atomic<long long>
//   - 或每條 thread 自己累積 local 結果, 結束後合併 (no-race 設計)
// -------------------------------------------------------------
int histogram[3] = {0, 0, 0};   // [200, 404, 500] 各幾次

void worker_histogram(const std::vector<int>& codes)
{
    for (int c : codes) {
        if      (c == 200) ++histogram[0];   // race
        else if (c == 404) ++histogram[1];   // race
        else if (c == 500) ++histogram[2];   // race
    }
}

void demo_histogram_race()
{
    std::cout << "\n[demo] histogram race\n";
    // 每條 thread 處理 100k 筆假請求, 全是 status=200
    std::vector<int> codes(100'000, 200);
    histogram[0] = histogram[1] = histogram[2] = 0;

    std::thread t1(worker_histogram, std::cref(codes));
    std::thread t2(worker_histogram, std::cref(codes));
    t1.join(); t2.join();

    std::cout << "  expected [200] = 200000, actual = " << histogram[0] << '\n';
    std::cout << "  (一樣有 lost update; 多個 bucket 不會解掉 race)\n";
}

// -------------------------------------------------------------
// 實戰範例 2: thread-local 累積再合併 (race-free 設計)
// -------------------------------------------------------------
// 解法核心: 「不共享就沒 race」── 每條 thread 用自己獨佔的
// 區域變數計數, 結束時把區域結果合進共享變數 *一次*。
//
// 共享變數仍可能要 atomic (因為合併也是兩條 thread 寫同一格),
// 但合併次數只有 N (= thread 數), 不是 N × 百萬, 所以爭用幾乎
// 消失, 速度跟單執行緒一樣快。
//
// 這是高效能多執行緒最重要的設計模式: 「分散累積, 最後合併」。
// -------------------------------------------------------------
std::atomic<long long> shared_sum{0};

void worker_localsum(int times)
{
    long long local = 0;
    for (int i = 0; i < times; ++i) ++local;   // 純 local, 無 race
    shared_sum.fetch_add(local);               // 只 atomic 一次
}

void demo_local_then_merge()
{
    std::cout << "\n[demo] local-accumulate then merge\n";
    const int N = 1'000'000;
    shared_sum.store(0);
    std::thread t1(worker_localsum, N);
    std::thread t2(worker_localsum, N);
    t1.join(); t2.join();
    std::cout << "  expected = " << (2LL * N)
              << ", actual = " << shared_sum.load() << " (完全正確)\n";
}

int main()
{
    const int N = 1'000'000;     // 每個執行緒遞增這麼多次

    // 啟動兩個執行緒,同時對 `counter` 進行轟炸。
    std::thread t1(increment, N);
    std::thread t2(increment, N);

    // 等兩個都結束 (回想 lesson 01 的規則:
    // 每個執行緒都必須被 join 或 detach)。
    t1.join();
    t2.join();

    // 數學上應該得到 2 * N。
    // 但硬體幾乎一定不同意。
    std::cout << "Expected counter = " << (2LL * N) << '\n';
    std::cout << "Actual   counter = " << counter   << '\n';
    std::cout << "Lost increments  = " << (2LL * N - counter) << '\n';

    // ---------------------------------------------------------
    // 兩個延伸示範: 多格 race vs 分散累積最後合併
    // ---------------------------------------------------------
    demo_histogram_race();
    demo_local_then_merge();

    // ---------------------------------------------------------
    // 重要觀念
    //
    // C++ 中的資料競爭屬於 *未定義行為*。意思是:
    //   - 結果可能看起來「差不多對」(只少了幾個)。
    //   - 也可能完全錯得離譜。
    //   - 甚至在你筆電上看起來正確,在生產伺服器上卻壞掉。
    //   - 開了最佳化 (-O2) 之後,編譯器被允許做出非常奇怪
    //     的事情,因為它假設你不會寫出競爭。
    //
    // 所以:不要去推論競爭「壞到什麼程度」,直接修掉它。
    // Lesson 03 會示範修法:std::mutex。
    // ---------------------------------------------------------

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：兩條 thread 同時「只讀」同一個非 atomic 變數, 算 data race 嗎?
    //    A：不算。C++ 標準的 data race 定義是「至少一個是寫且未同步」,
    //       純讀不變的記憶體完全安全 (每個 thread 各自 cache 一份, 不會
    //       衝突)。只要有任一邊寫, 即使另一邊只讀, 仍是 race。
    //
    //  Q2：為什麼 volatile 不能解 data race? 跟 Java/C# 的 volatile 差
    //       別在哪?
    //    A：C++ volatile 只承諾「編譯器不省略此讀寫」, 不保證 atomicity、
    //       不保證 visibility、不保證 ordering, 完全沒有跨執行緒的同步
    //       語意。它的設計目標是 MMIO 而非執行緒。Java volatile 帶
    //       acquire/release 語意 (像 C++ atomic 的中等強度), C# volatile
    //       類似但更弱。三家不能類比。
    //
    //  Q3：-O2 跑 data race 看起來「結果差不多對」, 是不是其實沒事?
    //    A：絕對不是。編譯器在 -O2 可能把 counter 暫存在 register、
    //       hoist 出迴圈, 變成「只寫一次」, race 視窗縮到看不見而已。
    //       同一份程式碼換到 ARM、換負載、換編譯器版本就會炸。Data race
    //       是 UB, 標準允許「任何結果」, 不能用測試「驗證它沒事」, 必須
    //       用 mutex / atomic / TSan 從根本除掉。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 02_data_race.cpp -o 02_data_race

// === 預期輸出 ===
// Expected counter = 2000000
// Actual   counter = 1914557
// Lost increments  = 85443
//
// [demo] histogram race
//   expected [200] = 200000, actual = 200000
//   (一樣有 lost update; 多個 bucket 不會解掉 race)
//
// [demo] local-accumulate then merge
//   expected = 2000000, actual = 2000000 (完全正確)
