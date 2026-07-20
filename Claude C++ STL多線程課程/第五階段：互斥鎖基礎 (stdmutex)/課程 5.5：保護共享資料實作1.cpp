// =============================================================================
//  課程 5.5：保護共享資料實作1.cpp — 把 mutex 與被保護的資料綁在同一個類別裡
// =============================================================================
//
// 【主題資訊 Information】
//   class ThreadSafeCounter {
//       mutable std::mutex mtx;   // 保護者（private）
//       int count = 0;            // 被保護者（private）
//   public:
//       void increment();                  // 每個 public 方法 = 一段完整的臨界區段
//       int  get() const;
//       int  reset();                      // 讀取＋歸零，必須「一次鎖完」的複合操作
//       bool incrementIfLessThan(int max); // check-then-act 也必須在同一把鎖內
//   };
//
//   標頭檔：<mutex>
//   標準版本：std::mutex、std::lock_guard = C++11；std::scoped_lock = C++17
//   複雜度：無競爭時 lock/unlock 是使用者空間的一次原子 CAS（不進核心，數十 ns）；
//           一旦競爭就走 futex 系統呼叫掛起 + 喚醒（微秒等級）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「mutex 要和資料綁在一起」是第一原則】
// 初學者最常見的寫法是：全域一個 std::mutex g_mtx、全域一個 int g_count，
// 然後「約定」大家存取 g_count 前要先鎖 g_mtx。這個約定沒有任何編譯期強制力：
// 只要有一個人忘記鎖，就是 data race（未定義行為），而且它不會編譯錯、
// 不會執行期報錯，只會在壓力測試或上線後偶爾算錯。
//
// 正確做法是把「保護者」和「被保護者」放進同一個類別，兩者都是 private：
//   - 外部拿不到 count，就不可能繞過鎖去改它。
//   - 外部拿不到 mtx，就不可能在外面亂鎖、亂解、或湊出 AB-BA 死結。
// 這樣「該不該鎖」不再是使用者的紀律問題，而是型別系統的保證 ——
// 把正確性從「文件上的約定」升級成「不照做就編不過」。
//
// 【2. 為什麼 mutex 要宣告成 mutable】
// get() 是邏輯上的唯讀操作，應該標成 const。但 std::mutex::lock() 是
// non-const 成員函式（它確實會改變 mutex 的內部狀態）。若 mtx 不是 mutable，
// 在 const 成員函式裡建構 lock_guard 就會編譯失敗。
//   mutable std::mutex mtx;   // ← 讓 const 成員函式也能加鎖
// 這是 mutable 少數幾個「完全正當」的用途之一：mutex、cache、統計計數器這類
// 「不影響物件對外邏輯狀態（logical constness）」的成員。
// 注意 const 在 C++ 裡從來就不等於 thread-safe，它只代表「不改變邏輯狀態」；
// 真正讓多個執行緒可以同時呼叫 const 方法的是這把鎖，不是 const 關鍵字。
//
// 【3. 為什麼要 = delete 掉複製】
// std::mutex 的複製建構與複製賦值都被標準刪除（複製一把「可能正被某執行緒
// 持有的鎖」沒有任何合理語意）。所以只要類別內含 mutex 成員，
// 編譯器合成的複製建構／複製賦值就會自動變成 deleted。
// 這裡明確寫出 = delete 是為了「意圖明示」：讓讀者一眼看出這是設計決定，
// 而且錯誤訊息會停在你這一行，而不是深入 <mutex> 內部的一堆樣板。
// 若真的需要可複製，就必須自己定義複製語意並鎖住來源 —— 見本課第 2 支檔案。
//
// 【4. 介面層級的 race condition：為什麼需要 incrementIfLessThan】
// 這是本課最重要的觀念。看這段呼叫端程式碼：
//     if (counter.get() < 100) {   // ← 鎖在這一行結束時就已經放掉了
//         counter.increment();     // ← 這裡重新拿鎖，中間別人早就改過了
//     }
// get() 和 increment() 各自都有鎖、各自都是原子的、全程沒有 data race、
// 沒有任何未定義行為 —— 但整段邏輯依然是錯的。這叫 check-then-act，
// 也就是 TOCTOU（Time-Of-Check to Time-Of-Use）。兩個執行緒可能同時看到 99，
// 於是兩個都遞增，最後變成 101，違反了「不超過 100」這個不變量（invariant）。
//
// 這裡務必分清楚兩個常被混用的名詞：
//   * data race      ：兩個執行緒未同步地存取同一記憶體、且至少一方是寫入
//                      → 標準直接判定為未定義行為（UB）。
//   * race condition ：結果取決於執行緒的相對時序。即使全程有鎖、
//                      完全沒有 UB，仍可能違反程式的不變量。
// 上面那段是 race condition 但不是 data race。修法不是「加更多鎖」，
// 而是把「檢查」和「動作」設計成同一個原子介面：
//     bool incrementIfLessThan(int max);   // 檢查與遞增在同一把鎖內完成
// 通則：不變量跨越幾個欄位、幾個步驟，臨界區段就必須涵蓋幾個步驟。
//
// 【5. get() 回傳的值天生就是「過期的」】
//     int v = counter.get();  // 這行結束、鎖放掉的瞬間，v 就可能已經不是現值
// 這不是 bug，是並行系統的本質：任何「快照」一離開臨界區段就過期。
// 所以 get() 的正確用途是監控、記錄、印報表；
// 絕不可以拿它的回傳值去決定「下一步該不該動作」（那就是第 4 點的錯誤）。
//
// 【概念補充 Concept Deep Dive】
//
// (A) lock_guard 的成本：無競爭 vs 有競爭
//   Linux 上 std::mutex 實作於 pthread_mutex_t，本質是一個 futex
//   （fast userspace mutex）。無競爭時，lock 只是對一個整數做一次原子 CAS，
//   全程在使用者空間、不進核心，成本約數十個 CPU 週期。
//   只有在「CAS 失敗＝真的有人持有」時才會呼叫 futex(FUTEX_WAIT) 進核心把
//   執行緒掛起；之後被喚醒還要付一次 context switch，成本跳到微秒等級。
//   結論：mutex 慢的通常不是 mutex 本身，是「競爭」。
//   本檔 4 個執行緒搶同一個計數器加 10 萬次，是刻意製造的高競爭情境。
//
// (B) 記憶體序：unlock 到底同步了什麼
//   標準規定 mutex::lock() 具 acquire 語意、unlock() 具 release 語意。
//   「A 執行緒的 unlock() synchronizes-with B 執行緒之後對同一 mutex 的 lock()」，
//   因此 A 在臨界區段內的所有寫入，對 B 進入臨界區段之後全部可見
//   （建立 happens-before 關係）。這保證的不只是 count 這一個變數，而是 A 在
//   解鎖前做過的每一個寫入 —— 包含它剛塞進 vector 的元素、剛配置的字串緩衝區。
//   這就是「不能因為覺得慢就把鎖拿掉」的真正原因：你拿掉的不只是互斥，
//   還有整個記憶體可見性保證。編譯器與 CPU 都不允許把臨界區段內的存取搬到鎖外
//   （單向屏障：外面的可以搬進來，裡面的不能搬出去）。
//
// (C) ++count 為什麼不是原子的
//   ++count 編譯後是 load / add / store 三步（x86-64 上即使被縮成單一
//   `add DWORD PTR [rax], 1`，沒有 lock 前綴時在多核上依然不是原子的）。
//   兩個執行緒可能都讀到 41、都寫回 42，一次遞增就這樣蒸發了。
//   這種「更新遺失（lost update）」正是 data race 最典型的後果。
//
// (D) 這個類別其實可以不用鎖
//   單一 int 的遞增，用 std::atomic<int> 搭配 fetch_add 會明顯更快
//   （無鎖、不會 context switch）。這裡用 mutex 是為了教學一致性；
//   而且一旦需求變成「同時維護 count 與 sum 兩個欄位的不變量」，
//   atomic 就不夠了 —— 多個變數的一致性只有鎖（或 CAS 迴圈）能保證。
//
// 【注意事項 Pay Attention】
// 1. mutex 與被保護資料一律 private，且放在同一個類別；不要提供 getMutex()。
// 2. 絕對不要回傳內部資料的參考、指標或 iterator（詳見本課第 4 支檔案），
//    那會讓鎖在 return 的瞬間失效，保護形同虛設。
// 3. 每個 public 方法要維持「進入前不變量成立、離開時不變量也成立」。
// 4. 複合操作（讀了再判斷、判斷了再改）必須包成單一方法，不可讓呼叫端自己組合。
// 5. const 方法要能加鎖，mutex 就必須是 mutable。
// 6. 持有鎖時不要呼叫使用者提供的回呼、不要做 I/O、不要 sleep、不要再去鎖別的鎖，
//    輕則拖垮吞吐，重則死結。
// 7. lock_guard 一定要有變數名。std::lock_guard<std::mutex>(mtx); 會被解析成
//    「宣告一個叫 mtx 的暫存物件」，當場解構＝完全沒鎖到。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】執行緒安全類別設計
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 類別裡的 mutex 為什麼要宣告成 mutable？
//     答：因為 get() 這種邏輯唯讀的方法應該標 const，但 mutex::lock() 是
//         non-const 成員函式。mutable 讓 const 成員函式也能加鎖，且不破壞
//         「邏輯常數性（logical constness）」—— 加鎖不改變物件對外的邏輯狀態。
//     追問：const 是不是就代表 thread-safe？→ 不是。const 只保證不改邏輯狀態；
//           多執行緒能同時呼叫 const 方法，靠的是裡面那把鎖。
//
// 🔥 Q2. 每個方法都有鎖了，為什麼還會有 race condition？
//     答：那是介面層級的 race。if (c.get() < 100) c.increment(); 兩個呼叫
//         各自原子，但兩者之間鎖是放掉的，這就是 check-then-act / TOCTOU。
//         正解是把檢查與動作合併成一個原子介面 incrementIfLessThan(100)。
//     追問：這算 data race 嗎？→ 不算。data race 是「未同步存取同一記憶體且
//           至少一方寫入」，屬未定義行為；這裡全程有鎖、沒有 UB，
//           但結果依賴時序，仍然違反不變量。兩者絕不可混用。
//
// ⚠️ 陷阱. 為什麼含有 std::mutex 成員的類別，複製它會編譯失敗？
//     答：std::mutex 的複製建構與複製賦值都是 deleted（複製一把可能正被
//         某執行緒持有的鎖沒有意義），因此編譯器合成的複製操作也跟著 deleted。
//     為什麼會錯：多數人以為「編譯器一定會幫我合成複製建構函式」。
//         事實是只要有任一成員不可複製，合成出來的版本就是 deleted。
//         想支援複製就得自己寫，並記得鎖住來源物件（見第 2 支檔案）。
//
// ⚠️ 陷阱. std::lock_guard<std::mutex>(mtx); 這一行有鎖到嗎？
//     答：沒有。它被解析成「宣告一個名為 mtx 的 lock_guard 區域物件」，
//         在分號處立刻解構，臨界區段長度是零。正確寫法一定要給它名字：
//         std::lock_guard<std::mutex> lock(mtx);
//     為什麼會錯：看起來像「建一個暫時物件守住這個區塊」，
//         但 C++ 的宣告語法優先，那對括號被當成初始化器的括號。
//         C++17 起可寫 std::lock_guard lock(mtx); 用 CTAD 推導，較不易寫錯。
// ═══════════════════════════════════════════════════════════════════════════

// 檔案：lesson_5_5_safe_counter.cpp
// 說明：執行緒安全的計數器類別

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <utility>

class ThreadSafeCounter {
private:
    mutable std::mutex mtx;  // mutable：允許在 const 方法中使用
    int count = 0;
    
public:
    // 建構函式
    ThreadSafeCounter() = default;
    
    // 禁止複製（互斥鎖不可複製）
    ThreadSafeCounter(const ThreadSafeCounter&) = delete;
    ThreadSafeCounter& operator=(const ThreadSafeCounter&) = delete;
    
    // 遞增
    void increment() {
        std::lock_guard<std::mutex> lock(mtx);
        ++count;
    }
    
    // 遞減
    void decrement() {
        std::lock_guard<std::mutex> lock(mtx);
        --count;
    }
    
    // 增加指定值
    void add(int value) {
        std::lock_guard<std::mutex> lock(mtx);
        count += value;
    }
    
    // 獲取當前值
    int get() const {
        std::lock_guard<std::mutex> lock(mtx);
        return count;
    }
    
    // 重設並返回舊值（原子操作）
    int reset() {
        std::lock_guard<std::mutex> lock(mtx);
        int oldValue = count;
        count = 0;
        return oldValue;
    }
    
    // 條件遞增：只有當前值小於 max 時才遞增
    bool incrementIfLessThan(int max) {
        std::lock_guard<std::mutex> lock(mtx);
        if (count < max) {
            ++count;
            return true;
        }
        return false;
    }
};

// 測試
void workerIncrement(ThreadSafeCounter& counter, int times) {
    for (int i = 0; i < times; ++i) {
        counter.increment();
    }
}

int main() {
    ThreadSafeCounter counter;
    
    const int numThreads = 4;
    const int incrementsPerThread = 100000;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(workerIncrement, std::ref(counter), incrementsPerThread);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    int expected = numThreads * incrementsPerThread;
    int actual = counter.get();
    
    std::cout << "預期值：" << expected << std::endl;
    std::cout << "實際值：" << actual << std::endl;
    std::cout << "結果：" << (expected == actual ? "✓ 正確" : "✗ 錯誤") << std::endl;
    
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 5.5：保護共享資料實作1.cpp" -o safe_counter
//   本檔可執行的部分只用到 C++11 起就有的 std::mutex / std::lock_guard,
//   以 -std=c++17 編譯零警告通過;註解中提到的 std::scoped_lock 才是 C++17 新增。

// 註 1:本檔輸出是【完全確定的】,連跑 20 次位元組完全相同。
//      4 條執行緒 × 100000 次遞增,每次都在 lock_guard 保護下完成,
//      因此總數必然是 400000,不受排程影響。
//
// 註 2:【確定的是最終數值,不是中途的觀察值。】
//      如果在 join() 之前呼叫 counter.get(),讀到的數字每次都不一樣 ——
//      那不是 bug,而是「當下真的就是那個值」。
//      mutex 保證每次讀寫都是完整的,不保證你讀到的是最終值。
//
// 註 3:incrementIfLessThan() 是本檔最值得看的設計:它把「檢查 + 遞增」
//      放進【同一個】臨界區段,所以不會出現 check-then-act 的 race condition。
//      如果拆成 if (c.get() < max) c.increment(); 兩次上鎖,
//      即使每次呼叫本身都是執行緒安全的,整體邏輯依然會錯 ——
//      這是「元件安全 ≠ 組合安全」最典型的例子。
//
// 註 4:mutex 成員宣告為 mutable,才能在 const 的 get() 內上鎖。
//      這是 mutable 少數真正正當的用途:它保護的是「邏輯上的常數性」,
//      上鎖並沒有改變物件對外可觀察的狀態。

// === 預期輸出 ===
// 預期值：400000
// 實際值：400000
// 結果：✓ 正確
