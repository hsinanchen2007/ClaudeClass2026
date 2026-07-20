// =============================================================================
//  課程 4.4：資料競爭範例分析2.cpp  —  Read-Modify-Write：counter++ 的三個步驟
// =============================================================================
//
// 【本檔是「刻意示範錯誤」的範例，不要照抄到實際專案】
//   兩條執行緒對同一個非 atomic 的 counter 並行遞增，無任何同步
//   → genuine data race → 【未定義行為 (UB)】。
//   本檔【不會】宣稱「一定小於 20000」——UB 沒有固定結果。
//
// 【主題資訊 Information】
//   主題：    Read-Modify-Write (RMW) 競爭模式；五大競爭模式之一
//   語法：    counter++;   // 看似一步，實為 load / add / store 三步
//   標準版本：std::thread、std::atomic 皆為 C++11
//   標頭檔：  <thread>、<atomic>、<mutex>
//   複雜度：  每執行緒 O(N)；重點在正確性
//   偵測工具：g++ -fsanitize=thread -g -pthread
//
// 【詳細解釋 Explanation】
//
// 【1. counter++ 的三個步驟（本機實測組譯碼）】
//   本機 g++ 15.2 以 -O0 對 `counter++` 產生（x86-64，Intel 語法）：
//       mov  eax, DWORD PTR counter[rip]   ; ① load  ：讀進暫存器
//       add  eax, 1                        ; ② modify：暫存器 +1
//       mov  DWORD PTR counter[rip], eax   ; ③ store ：寫回記憶體
//   注意第三道指令【沒有 lock 前綴】，所以整段不是原子操作。
//   兩條執行緒若在 ① 與 ③ 之間交錯，就會「兩次 +1 只長 1」——
//   這就是 lost update（更新遺失）。
//
// 【2. 交錯的具體時序】
//       時間   執行緒 A            執行緒 B            counter（記憶體）
//       ─────────────────────────────────────────────────────────────
//        1     load  → eax=0                            0
//        2                        load  → eax=0         0
//        3     add   → eax=1                            0
//        4                        add   → eax=1         0
//        5     store eax(1)                             1
//        6                        store eax(1)          1
//       兩次遞增，counter 只變成 1。遺失的更新數量取決於交錯的次數，
//       所以「實際值」介於「單條執行緒的份額」到「完整總和」之間都有可能。
//
// 【3. 三種修法與它們的成本】
//   (a) std::atomic<int> + fetch_add
//         counter.fetch_add(1, std::memory_order_relaxed);
//       → 編譯成單一 `lock add` 指令（本機 -O2 實測），
//         整個 RMW 由硬體保證不可分割。最快，也最貼近本問題的本質。
//         計數器場景可用 relaxed 順序（只要求原子性，不要求跨變數的順序）。
//   (b) std::mutex + lock_guard
//         → 通用但較重（本機無競爭時約 13~25 ns，見課程 5.6）。
//           當「要保護的不只是一個變數」時才需要它。
//   (c) 每執行緒本地累加，最後合併一次
//         → 最快。把 N 次同步降為「每執行緒一次」。
//           本檔第四段示範，也是課程 5.6 的核心優化手法。
//   → 選擇原則：單一變數的計數用 (a)；跨變數的不變量用 (b)；
//     高頻累加且可延後合併用 (c)。
//
// 【4. 為什麼 atomic 在這裡夠用、在 4.2 卻不夠用】
//   課程 4.2-3 的結論是「atomic 修不好跨變數的不變量」，這裡卻說 atomic 夠用，
//   兩者並不矛盾，差別在於【不變量橫跨幾個變數】：
//     * 本檔：不變量只涉及 counter 一個變數，
//       而「+1」這整個動作可以用單一原子指令完成 → atomic 完全足夠。
//     * 4.2-3：不變量是 A + B == 2000，橫跨兩個變數，
//       沒有任何單一原子指令能同時改兩個位置 → 必須用鎖。
//   → 判準：你能不能把「整個操作」壓縮成一個原子動作？
//     能就用 atomic，不能就用鎖。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 -O2 反而「看起來正確」
//   開啟最佳化後，編譯器可能把整個迴圈摺疊成
//       add DWORD PTR counter[rip], 10000
//   一道指令。競爭視窗窄到幾乎撞不到，於是每次都印出 20000。
//   但這道指令【沒有 lock 前綴】，仍非原子操作，data race 與 UB 依然存在。
//   → 本檔最重要的教訓：不能用「跑幾次答案對」來證明沒有 data race。
//
// (B) lock add 與 fetch_add 的關係
//   x86-64 上 `counter.fetch_add(1)` 通常編譯成 `lock add`（或 `lock xadd`
//   若需要回傳舊值）。lock 前綴讓 CPU 在該指令期間鎖定快取行
//   （現代 CPU 用快取一致性協定 MESI 實現，不是真的鎖匯流排），
//   保證 load-modify-store 三步不被打斷。
//   代價是該快取行必須進入 Exclusive 狀態，多核心競爭同一個計數器時
//   會產生 cache line ping-pong，這是 atomic 計數器的主要成本來源。
//
// (C) false sharing：為什麼「各自的計數器」也可能很慢
//   若把每條執行緒的計數器放在相鄰的陣列元素（相距 4 或 8 bytes），
//   它們會落在同一條 cache line（本機為 64 bytes），
//   即使邏輯上互不相干，硬體仍會因為快取一致性協定而互相拖累。
//   解法是把各自的計數器對齊到 cache line
//   （C++17 的 std::hardware_destructive_interference_size）。
//   本檔第四段的「本地累加」用區域變數，天生落在各自的堆疊上，沒有這個問題。
//
// 【注意事項 Pay Attention】
// 1. 本檔是 UB 示範：不可說「一定小於 20000」，也不可說「一定等於某值」。
// 2. -O2 下常常「剛好正確」，那不是修好了，只是競爭視窗變窄。
// 3. counter++ 與 ++counter 在這裡沒有差別，兩者都是 RMW，都不是原子的。
// 4. atomic 夠不夠用，取決於「整個操作能否壓縮成一個原子動作」。
// 5. relaxed 順序只保證原子性，不建立跨變數的順序關係；
//    純計數可以用，用它同步其他資料則不行。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Read-Modify-Write 與 atomic
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. counter++ 為什麼不是原子操作?
//     答：它是 load / add / store 三道指令（本機 -O0 實測組譯確認），
//         寫回的指令沒有 lock 前綴。兩條執行緒在 load 與 store 之間交錯，
//         就會「兩次 +1 只長 1」，也就是 lost update。
//     追問：那 ++counter 呢?會不會比較好?
//         → 完全一樣。前置與後置只差在回傳值，都是 RMW，都不是原子的。
//
// 🔥 Q2. 用 std::atomic<int> 修這個 bug，該用哪種 memory order?
//     答：純計數器可以用 memory_order_relaxed —— 它保證這次 RMW 的原子性，
//         但不建立與其他變數之間的順序關係，因此最快。
//         如果這個計數器同時被用來當「資料就緒」的旗標，
//         那就必須用 release/acquire，否則別人可能讀到還沒寫完的資料。
//     追問：relaxed 會讓計數變少嗎?
//         → 不會。原子性與記憶體順序是兩件獨立的事：
//           relaxed 仍然保證每次 fetch_add 都不會遺失，
//           它放寬的只是「這次操作與其他記憶體操作之間的可見順序」。
//
// ⚠️ 陷阱. 「我加了 -O2，結果每次都印 20000，看來最佳化把問題修好了」——錯在哪?
//     答：-O2 把整個迴圈摺疊成單一 `add` 指令，競爭視窗窄到撞不到，
//         但那道指令沒有 lock 前綴，仍不是原子操作，UB 依然存在。
//         換平台、換負載、換編譯器版本隨時會現形。
//     為什麼會錯：把「觀察不到錯誤」當成「錯誤不存在」。
//         最佳化只改變了問題出現的機率，沒有改變程式的正確性；
//         判定要用 ThreadSanitizer，而不是看輸出對不對。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <string>
#include <functional>
#include <condition_variable>

// -----------------------------------------------------------------------------
// 【錯誤示範】無同步的 counter++ —— 本課主角
// -----------------------------------------------------------------------------
int counter = 0;

void increment() {
    // 這三步不是原子的！
    // 1. 讀取 counter
    // 2. +1
    // 3. 寫回
    for (int i = 0; i < 10000; ++i) {
        counter++;  // 非原子操作
    }
}

// -----------------------------------------------------------------------------
// 【修法 a】std::atomic：整個 RMW 由硬體保證不可分割
// -----------------------------------------------------------------------------
std::atomic<int> atomicCounter{0};

void incrementAtomic() {
    for (int i = 0; i < 10000; ++i) {
        // 純計數，不需要建立跨變數的順序關係 → relaxed 即可，且最快
        atomicCounter.fetch_add(1, std::memory_order_relaxed);
    }
}

// -----------------------------------------------------------------------------
// 【修法 b】std::mutex：通用但較重，適合「要保護的不只一個變數」時
// -----------------------------------------------------------------------------
int mutexCounter = 0;
std::mutex counterMtx;

void incrementMutex() {
    for (int i = 0; i < 10000; ++i) {
        std::lock_guard<std::mutex> lock(counterMtx);
        ++mutexCounter;
    }
}

// -----------------------------------------------------------------------------
// 【修法 c】本地累加 + 合併一次：把 10000 次同步降為 1 次
// -----------------------------------------------------------------------------
std::atomic<long> mergedCounter{0};

void incrementLocal() {
    long local = 0;                    // 區域變數，全程無同步
    for (int i = 0; i < 10000; ++i) {
        ++local;
    }
    mergedCounter.fetch_add(local, std::memory_order_relaxed);   // 只同步一次
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1195. Fizz Buzz Multithreaded
//   題目：四條執行緒分別負責印 "fizz"（3 的倍數）、"buzz"（5 的倍數）、
//         "fizzbuzz"（15 的倍數）與數字本身，必須產生正確的 FizzBuzz 序列。
//   為什麼用到本主題：這題的核心就是一個共享計數器 current，
//         四條執行緒都要對它做 read-modify-write（讀值判斷、印出、遞增）。
//         若像本檔開頭那樣裸寫 current++，會同時踩到兩個錯誤：
//           ① RMW 的 lost update（數字被跳過或重複）
//           ② check-then-act（讀到的值在印出前已被別人改掉）
//         正解是把「判斷 + 印出 + 遞增」整段放進同一個臨界區段，
//         並用 condition_variable 讓不該輪到的執行緒等待。
//         → 這正是「atomic 不夠用、必須用鎖」的實例：
//           要保護的不是單一變數，而是「判斷與遞增之間的一致性」。
// -----------------------------------------------------------------------------
class FizzBuzz {
private:
    // 【注意】成員初始化順序依「宣告順序」，與初始化列表順序無關。
    std::mutex mtx;
    std::condition_variable cv;
    int n;
    int current = 1;

public:
    explicit FizzBuzz(int nn) : n(nn) {}

    // 只有 3 的倍數（且非 15 的倍數）才輪到它
    void fizz(const std::function<void()>& printFizz) {
        while (true) {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] {
                return current > n || (current % 3 == 0 && current % 5 != 0);
            });
            if (current > n) break;
            printFizz();
            ++current;                    // 判斷與遞增同在鎖內
            cv.notify_all();
        }
        cv.notify_all();
    }

    void buzz(const std::function<void()>& printBuzz) {
        while (true) {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] {
                return current > n || (current % 5 == 0 && current % 3 != 0);
            });
            if (current > n) break;
            printBuzz();
            ++current;
            cv.notify_all();
        }
        cv.notify_all();
    }

    void fizzbuzz(const std::function<void()>& printFizzBuzz) {
        while (true) {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] {
                return current > n || (current % 15 == 0);
            });
            if (current > n) break;
            printFizzBuzz();
            ++current;
            cv.notify_all();
        }
        cv.notify_all();
    }

    void number(const std::function<void(int)>& printNumber) {
        while (true) {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] {
                return current > n || (current % 3 != 0 && current % 5 != 0);
            });
            if (current > n) break;
            printNumber(current);
            ++current;
            cv.notify_all();
        }
        cv.notify_all();
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】API 請求計數與速率限制
//   情境：閘道器要統計每個 API 的呼叫次數，並在超過配額時擋下請求。
//   兩個常見錯誤：
//     ① 用裸 int 計數 → lost update，統計數字長期偏低，
//        計費系統少收錢、容量規劃低估。
//     ② 用 atomic 但寫成「先 load 檢查配額、再 fetch_add」
//        → check-then-act，高併發下會放行超過配額的請求。
//   正解：純統計用 atomic fetch_add（夠用且最快）；
//         但「檢查配額 + 遞增」必須是一個原子動作 ——
//         用 compare_exchange 迴圈（本例）或直接用鎖。
// -----------------------------------------------------------------------------
class RateLimiter {
private:
    std::atomic<int> used{0};
    const int quota;

public:
    explicit RateLimiter(int q) : quota(q) {}

    // ✗ 錯誤寫法（僅供對照，不呼叫）：
    //     if (used.load() < quota) { used.fetch_add(1); return true; }
    //   load 與 fetch_add 之間有縫隙，多條執行緒可能同時通過檢查。

    // ✓ 正確：用 CAS 迴圈把「檢查 + 遞增」合併成一次不可分割的嘗試
    bool tryAcquire() {
        int cur = used.load(std::memory_order_relaxed);
        while (cur < quota) {
            // 只有在 used 仍等於 cur 時才寫入 cur+1；
            // 否則把最新值寫回 cur，重試。
            if (used.compare_exchange_weak(cur, cur + 1,
                                           std::memory_order_relaxed,
                                           std::memory_order_relaxed)) {
                return true;
            }
            // compare_exchange_weak 失敗時已把最新值放進 cur，直接進入下一輪
        }
        return false;   // 配額已滿
    }

    int usedCount() const { return used.load(std::memory_order_relaxed); }
};

int main() {
    std::cout << "=== 錯誤示範：無同步的 counter++（data race / UB）===\n";
    {
        std::thread t1(increment);
        std::thread t2(increment);
        t1.join();
        t2.join();
        // 預期 20000，實際可能更小
        std::cout << "counter = " << counter << "\n";
        std::cout << "（此數值每次執行都可能不同；等於 20000 也不代表程式正確）\n";
    }

    std::cout << "\n=== 修法 a：std::atomic + fetch_add(relaxed) ===\n";
    {
        std::thread t1(incrementAtomic);
        std::thread t2(incrementAtomic);
        t1.join();
        t2.join();
        std::cout << "atomicCounter = " << atomicCounter.load() << " (必定為 20000)\n";
    }

    std::cout << "\n=== 修法 b：std::mutex ===\n";
    {
        std::thread t1(incrementMutex);
        std::thread t2(incrementMutex);
        t1.join();
        t2.join();
        std::cout << "mutexCounter = " << mutexCounter << " (必定為 20000)\n";
    }

    std::cout << "\n=== 修法 c：本地累加 + 合併一次（同步次數 20000 → 2）===\n";
    {
        std::thread t1(incrementLocal);
        std::thread t2(incrementLocal);
        t1.join();
        t2.join();
        std::cout << "mergedCounter = " << mergedCounter.load() << " (必定為 20000)\n";
    }

    std::cout << "\n=== LeetCode 1195. Fizz Buzz Multithreaded (n = 15) ===\n";
    {
        FizzBuzz fb(15);
        std::string out;
        std::mutex outMtx;
        auto emit = [&out, &outMtx](const std::string& s) {
            std::lock_guard<std::mutex> lock(outMtx);
            out += s;
            out += " ";
        };

        std::thread a([&] { fb.fizz    ([&] { emit("fizz");     }); });
        std::thread b([&] { fb.buzz    ([&] { emit("buzz");     }); });
        std::thread c([&] { fb.fizzbuzz([&] { emit("fizzbuzz"); }); });
        std::thread d([&] { fb.number  ([&](int x) { emit(std::to_string(x)); }); });
        a.join(); b.join(); c.join(); d.join();

        std::cout << out << "\n";
        std::cout << "→ 四條執行緒交替寫入同一個 current，順序仍完全正確\n";
    }

    std::cout << "\n=== 日常實務：速率限制（CAS 迴圈避免 check-then-act）===\n";
    {
        RateLimiter limiter(1000);
        std::vector<std::thread> ths;
        std::atomic<int> granted{0};

        for (int i = 0; i < 8; ++i) {
            ths.emplace_back([&] {
                int local = 0;
                for (int k = 0; k < 500; ++k) {
                    if (limiter.tryAcquire()) ++local;
                }
                granted.fetch_add(local);
            });
        }
        for (auto& t : ths) t.join();

        std::cout << "8 執行緒共嘗試 4000 次，配額 1000\n";
        std::cout << "放行次數: " << granted.load() << " (必定 = 1000，不會超發)\n";
        std::cout << "計數器值: " << limiter.usedCount() << " (必定 = 1000)\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 4.4：資料競爭範例分析2.cpp' -o race2
//
// 觀察最佳化的影響（第一段在 -O2 下常常「剛好正確」，但 UB 依然存在）:
//   g++ -std=c++17 -O2 -Wall -Wextra -pthread '課程 4.4：資料競爭範例分析2.cpp' -o race2_opt
//
// 偵測資料競爭（唯一可靠的判定方式）:
//   g++ -std=c++17 -fsanitize=thread -g -pthread '課程 4.4：資料競爭範例分析2.cpp' -o race2_tsan

// ⚠️ 只有第一行的 counter 值【每次執行都不同】——該段是 genuine data race → UB。
// 本機（Ubuntu 26.04 / g++ 15.2.0 / 16 核心）以檔頭指令（未指定 -O，即 -O0）
// 連續多次實測分別出現過 15424 / 17249 / 19700 / 18668 / 16697 / 20000 等值。
// 下面這一次剛好印出 20000 —— 這正好示範了本檔的核心教訓：
// 「跑對」完全不代表沒有 data race，UB 依然存在，只是這次沒撞上競爭視窗。
// 改用 -O2 則常常剛好印出 20000 —— 那不是修好了，只是競爭視窗變窄，UB 依然存在。
// 其餘各段（三種修法、LeetCode 1195、速率限制）都有同步保護，為確定值；
// 其中 1195 的 FizzBuzz 序列必定完全正確，與四條執行緒的排程無關。

// === 預期輸出 ===
// === 錯誤示範：無同步的 counter++（data race / UB）===
// counter = 20000
// （此數值每次執行都可能不同；等於 20000 也不代表程式正確）
//
// === 修法 a：std::atomic + fetch_add(relaxed) ===
// atomicCounter = 20000 (必定為 20000)
//
// === 修法 b：std::mutex ===
// mutexCounter = 20000 (必定為 20000)
//
// === 修法 c：本地累加 + 合併一次（同步次數 20000 → 2）===
// mergedCounter = 20000 (必定為 20000)
//
// === LeetCode 1195. Fizz Buzz Multithreaded (n = 15) ===
// 1 2 fizz 4 buzz fizz 7 8 fizz buzz 11 fizz 13 14 fizzbuzz 
// → 四條執行緒交替寫入同一個 current，順序仍完全正確
//
// === 日常實務：速率限制（CAS 迴圈避免 check-then-act）===
// 8 執行緒共嘗試 4000 次，配額 1000
// 放行次數: 1000 (必定 = 1000，不會超發)
// 計數器值: 1000 (必定 = 1000)
