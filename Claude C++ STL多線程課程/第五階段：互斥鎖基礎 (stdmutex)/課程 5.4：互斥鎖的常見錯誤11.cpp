// =============================================================================
//  課程 5.4：互斥鎖的常見錯誤11.cpp  —  AB-BA 死結：唯一「真正」的死結
// =============================================================================
//
// 【主題資訊 Information】
//   本檔示範：兩條執行緒以【相反順序】取得兩把鎖 → 循環等待 → 死結
//   標頭檔：<mutex>、<thread>、<chrono>
//   解法（C++ 標準提供）：
//       template<class L1, class L2, class... L3>
//       void std::lock(L1&, L2&, L3&...);                          // C++11
//       template<class... MutexTypes> class std::scoped_lock;      // C++17
//
//   ★ 本檔的錯誤性質（與本課其他檔案對照）：
//       * 每一次 lock() 呼叫【本身都完全合法】——不是 UB
//       * 但兩條執行緒可能形成【循環等待】——這才是教科書定義的「死結」
//       * ⚠️ 而且它【不保證發生】：若 thread1 剛好在 thread2 開始前就
//         整段跑完，程式會正常結束。這是本檔最重要的觀念。
//
// 【詳細解釋 Explanation】
//
// 【1. 這是本課唯一真正的「死結」】
//   前面幾個檔案的現象常被籠統地叫做「死結」，但精確地說：
//       檔 1  忘記解鎖 → 鎖洩漏導致的永久阻塞（沒有循環，不是死結）
//       檔 2、5 同一執行緒重複 lock → 未定義行為（沒有循環，不是死結）
//       本檔  兩執行緒相反順序取鎖 → 【循環等待】→ 這才是死結
//   死結的定義需要「兩個以上的執行緒【互相】等待對方持有的資源」。
//   本檔的情形正是如此：
//       Thread 1 持有 A，等待 B
//       Thread 2 持有 B，等待 A
//   兩者都永遠等不到 —— 形成一個長度為 2 的等待環。
//
// 【2. 為什麼「不保證發生」是關鍵觀念】
//   死結需要兩條執行緒的執行【真的交錯】：
//       T1: lock(A) ─────────→ lock(B)   ← 必須在 T2 拿到 B 之後才執行到這
//       T2:      lock(B) ────→ lock(A)
//   若 thread1 在 thread2 開始前就把 A、B 都取得並釋放完畢，
//   兩者不會交錯，程式正常印出 "完成" 並結束。
//   本檔中間的 sleep_for(10ms) 是【刻意加大】交錯機率的手段——
//   它讓「持有 A、尚未取得 B」這個危險窗口從奈秒級拉長到 10 毫秒，
//   使死結幾乎每次都發生。
//   ⚠️ 這正是死結在生產環境如此可怕的原因：
//      危險窗口在真實程式碼裡可能只有幾微秒，
//      開發與測試環境跑一萬次都不出事，
//      上線後在高負載下每天發生兩次。
//      「測不出來」不代表「不存在」。
//   本機實測（g++ 15.2、Ubuntu 26.04）：連續 5 次執行皆死結（exit=124）。
//
// 【3. 三種解法，以及它們各自打破了哪個條件】
//   回顧 Coffman 的四個必要條件（互斥、持有並等待、不可搶奪、循環等待）：
//   ┌──────────────────────────┬──────────────────┬────────────────────────┐
//   │ 解法                     │ 打破哪個條件     │ 特性                   │
//   ├──────────────────────────┼──────────────────┼────────────────────────┤
//   │ 統一鎖的取得順序         │ 循環等待         │ 最簡單，但需全域紀律   │
//   │ std::scoped_lock (C++17) │ 持有並等待       │ 最推薦，標準內建       │
//   │ try_lock + 退讓          │ 持有並等待       │ 見課 5.3；可能活鎖     │
//   └──────────────────────────┴──────────────────┴────────────────────────┘
//   【統一順序】的常見實作是依鎖的【記憶體位址】排序：
//       std::mutex* first  = (&a < &b) ? &a : &b;
//       std::mutex* second = (&a < &b) ? &b : &a;
//   這保證任何兩條執行緒對同一對鎖的取得順序必然一致，環就形成不了。
//   【scoped_lock】則是內部使用 try-and-back-off 演算法一次取得多把鎖，
//   不需要使用者維護任何順序約定——這是實務上的首選。
//
// 【4. 更大的系統怎麼防：鎖階層（lock hierarchy）】
//   當程式裡有幾十把鎖時，「統一順序」的紀律很難靠人維護。
//   業界做法是給每把鎖一個【階層編號】，並強制規定：
//       只能由高層往低層取鎖，絕不可反向。
//   例如：UI 層(3000) → 業務邏輯層(2000) → 資料層(1000)。
//   在除錯建置中，可以讓鎖自己檢查這條規則並在違規時立即報錯——
//   本檔的實務範例就實作了一個這樣的 HierarchicalMutex。
//   它的價值是把「可能今天不發生、明天才發生」的機率性死結，
//   變成「一違規就立刻爆炸」的確定性錯誤。
//
// 【概念補充 Concept Deep Dive】
//   * 怎麼診斷已經發生的死結？
//     徵狀：程式停住 + CPU 使用率接近 0%（所有執行緒都在 blocked）。
//     診斷：gdb -p <pid> 後 thread apply all bt，
//     會看到 T1 停在 lock(B)、T2 停在 lock(A)，兩份堆疊互相印證。
//     ⚠️ Linux 也可用 `cat /proc/<pid>/task/*/stack`（需權限）。
//   * ThreadSanitizer 能偵測【潛在】的鎖順序反轉：
//         g++ -std=c++17 -pthread -g -fsanitize=thread ...
//     它不需要死結真的發生——只要觀察到程式在不同時間點
//     以不同順序取得同兩把鎖，就會報 "lock-order-inversion"。
//     這對「危險窗口極短、測不出來」的死結特別有價值。
//   * ⚠️ 解鎖的順序【不影響】死結：unlock 不會阻塞，
//     怎麼解都不會形成等待。只有【取得】的順序重要。
//     很多人以為「後鎖的要先解」是為了避免死結——不是，
//     那只是為了對稱與可讀（也與 RAII 的解構順序一致）。
//
// 【注意事項 Pay Attention】
//   1. 本檔的死結【不是 UB】——每個 lock() 都合法，問題在循環等待。
//   2. 它【不保證發生】，取決於兩條執行緒是否真的交錯。
//      測試通過完全不代表沒有死結風險。
//   3. 只有【取得】鎖的順序會造成死結，解鎖順序不影響。
//   4. 需要同時持有多把鎖時，一律優先用 std::scoped_lock（C++17）。
//   5. 鎖很多時用鎖階層（lock hierarchy），並在除錯建置中強制檢查。
//   6. 用 -fsanitize=thread 可在死結尚未發生前就偵測到順序反轉。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】AB-BA 死結
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 兩條執行緒以相反順序取兩把鎖，一定會死結嗎？
//     答：不一定，這是關鍵。死結需要兩者的執行【真的交錯】——
//         T1 必須在持有 A 且尚未取得 B 時，T2 剛好已經持有 B。
//         若 T1 在 T2 開始前就整段跑完，程式完全正常。
//         本檔用 sleep(10ms) 把危險窗口人為拉長，才讓死結幾乎每次都發生。
//     追問：那為什麼生產環境的死結這麼難抓？
//           → 因為真實程式碼的危險窗口可能只有幾微秒。
//             測試環境跑一萬次都不出事，上線後在高負載、
//             執行緒更多、排程更容易交錯的情況下才爆——
//             而且往往無法在開發機重現。
//
// 🔥 Q2. 有哪些方法可以避免這種死結？各自的原理是什麼？
//     答：(1)【統一鎖的取得順序】——打破「循環等待」條件。
//         常見實作是依鎖的記憶體位址排序，保證所有執行緒對同一對鎖
//         的取得順序必然一致，環就形成不了。
//         (2)【std::scoped_lock（C++17）】——一次取得多把鎖，
//         內部用 try-and-back-off 演算法，不需要使用者維護順序約定。
//         這是實務首選。
//         (3)【try_lock + 失敗就放掉已持有的鎖】——打破「持有並等待」，
//         但要注意可能變成活鎖，需要隨機退避。
//     追問：鎖很多的大型系統怎麼辦？→ 用鎖階層（lock hierarchy）：
//           給每把鎖一個層級編號，強制只能由高層往低層取，
//           並在除錯建置中讓鎖自己檢查違規並立即報錯。
//
// ⚠️ 陷阱. 「我把解鎖順序改成跟上鎖順序相反（後鎖的先解），
//        就不會死結了。」
//     答：完全無效。死結的成因是【取得】鎖時的循環等待，
//         而 unlock() 根本不會阻塞——它不可能參與任何等待環。
//         無論以什麼順序解鎖，只要兩條執行緒【取得】的順序相反，
//         死結風險就原封不動地存在。
//     為什麼會錯：把「後進先出」這個在 RAII 與堆疊管理中普遍存在的
//         對稱性，誤當成死結的解法。
//         「後鎖的先解」確實是好習慣（與 lock_guard 的解構順序一致、
//         可讀性佳），但它的價值是【風格】，不是【正確性】。
//         真正要改的是取得順序，不是釋放順序。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔【略過】，理由如下：
//   LeetCode 的並行題（1114 / 1115 / 1116 / 1117 / 1195）都只用【一個】
//   同步點協調輸出順序，從頭到尾不需要同時持有兩把以上的鎖，
//   因此結構上不可能發生 AB-BA 死結。
//   硬掛一題只會讓人誤以為那些題目與死結有關，故從缺。
//   （多鎖的正確用法在課 5.3「非阻塞鎖定3.cpp」以銀行轉帳完整示範。）
//
// -----------------------------------------------------------------------------
// 【本檔是「刻意示範錯誤」的範例，開啟示範後程式極可能永遠停住】
//
// ⚠️ 這【不是】未定義行為，每一次 lock() 都是合法的；
//    問題出在兩個執行緒以【相反順序】取鎖，形成循環等待（AB-BA 死結）。
//
// ⚠️ 但它也【不是「保證」死結】：
//    若 thread1 剛好在 thread2 開始前就整段跑完，兩者不會交錯，
//    程式就會正常印出 "完成" 並結束。
//    中間的 sleep_for(10ms) 是【刻意加大】交錯機率的手段，
//    讓死結幾乎每次都發生，但仍屬機率問題，而非標準保證。
//    → 因此原版註解「程式永遠不會到達這裡」並不精確，已改寫。
//
//    實測（g++ 15.2、Ubuntu 26.04）：連續 5 次執行皆死結（exit=124）。
//
// 為了不讓整份課程的批次執行卡死，這個示範預設【關閉】。
// 要親眼觀察，請自行加上 -DDEMONSTRATE_UB：
//    g++ -std=c++17 -pthread -DDEMONSTRATE_UB -o lock_order '課程 5.4：互斥鎖的常見錯誤11.cpp'
//    timeout 5 ./lock_order ; echo "exit=$?"   # 預期 exit=124（逾時）
//
// ✅ 正確作法（擇一）：
//    1. 全程式統一鎖的取得順序（永遠先 A 後 B）；
//    2. 用 std::lock(mutexA, mutexB)，或 C++17 的
//       std::scoped_lock lock(mutexA, mutexB); 一次取得多把鎖，
//       它內部採用可避免死結的演算法。
// -----------------------------------------------------------------------------

#include <atomic>
#include <chrono>
#include <climits>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

std::mutex mutexA;
std::mutex mutexB;

void thread1() {
    mutexA.lock();
    std::cout << "Thread 1: locked A" << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    mutexB.lock();  // 💀 等待 Thread 2 釋放 B
    std::cout << "Thread 1: locked B" << std::endl;

    mutexB.unlock();
    mutexA.unlock();
}

void thread2() {
    mutexB.lock();  // ← 順序相反！
    std::cout << "Thread 2: locked B" << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    mutexA.lock();  // 💀 等待 Thread 1 釋放 A
    std::cout << "Thread 2: locked A" << std::endl;

    mutexA.unlock();
    mutexB.unlock();
}

// -----------------------------------------------------------------------------
// 【解法 1】統一鎖的取得順序（依記憶體位址排序）—— 打破「循環等待」
// -----------------------------------------------------------------------------
void orderedWork(std::mutex& a, std::mutex& b) {
    // 不論呼叫端傳入的順序如何，實際取鎖順序永遠一致
    std::mutex* first  = (&a < &b) ? &a : &b;
    std::mutex* second = (&a < &b) ? &b : &a;

    std::lock_guard<std::mutex> l1(*first);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    std::lock_guard<std::mutex> l2(*second);
}

// -----------------------------------------------------------------------------
// 【解法 2】std::scoped_lock（C++17）—— 打破「持有並等待」，最推薦
// -----------------------------------------------------------------------------
void scopedWork(std::mutex& a, std::mutex& b) {
    std::scoped_lock lock(a, b);      // 一次取得，內建死結避免演算法
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

// -----------------------------------------------------------------------------
// 【日常實務範例】鎖階層（lock hierarchy）：把機率性死結變成確定性錯誤
//   情境：一個有數十把鎖的系統。「統一取得順序」的紀律靠人維護不可靠——
//         新人不知道約定、重構時順序被打亂、呼叫鏈太長看不出來。
//   對策：給每把鎖一個【層級編號】，強制規定只能由高層往低層取鎖。
//         例如：UI 層(3000) → 業務邏輯層(2000) → 資料層(1000)。
//         鎖自己在 lock() 時檢查這條規則，違規就【立刻拋出例外】。
//
//   ★ 這個做法的核心價值：
//     原本的死結是機率事件（可能今天不發生、上線才發生、無法重現）；
//     加上階層檢查後，任何一次違規的取鎖【當場就爆炸】，
//     而且錯誤訊息直接指出是哪兩層違規。
//     把「難以重現的並行 bug」變成「一測就中的確定性錯誤」，
//     這是並行程式設計中極有價值的一類技巧。
//
//   實作要點：用 thread_local 記錄「這條執行緒目前持有的最低層級」。
//   thread_local 保證每條執行緒各有一份，不需要額外同步。
// -----------------------------------------------------------------------------
class HierarchicalMutex {
private:
    std::mutex          internalMutex_;
    const unsigned long hierarchyValue_;      // 本鎖的層級
    unsigned long       previousValue_ = 0;   // 記住取得本鎖前的層級

    // 每條執行緒各自的「目前持有的最低層級」，初始為最大值（尚未持有任何鎖）
    static thread_local unsigned long thisThreadHierarchyValue_;

    void checkForHierarchyViolation() const {
        if (thisThreadHierarchyValue_ <= hierarchyValue_) {
            throw std::logic_error(
                "鎖階層違規：目前持有層級 " +
                std::to_string(thisThreadHierarchyValue_) +
                "，卻嘗試取得層級 " + std::to_string(hierarchyValue_) +
                "（只能由高層往低層取）");
        }
    }

    void updateHierarchyValue() {
        previousValue_ = thisThreadHierarchyValue_;
        thisThreadHierarchyValue_ = hierarchyValue_;
    }

public:
    explicit HierarchicalMutex(unsigned long value)
        : hierarchyValue_(value) {}

    void lock() {
        checkForHierarchyViolation();    // 先檢查，違規就直接拋例外
        internalMutex_.lock();
        updateHierarchyValue();
    }

    void unlock() {
        if (thisThreadHierarchyValue_ != hierarchyValue_) {
            throw std::logic_error("解鎖順序違規");
        }
        thisThreadHierarchyValue_ = previousValue_;
        internalMutex_.unlock();
    }

    bool try_lock() {
        checkForHierarchyViolation();
        if (!internalMutex_.try_lock()) return false;
        updateHierarchyValue();
        return true;
    }
};

// 初值設為最大值，代表「尚未持有任何鎖，可以取得任何層級」
thread_local unsigned long
    HierarchicalMutex::thisThreadHierarchyValue_ = ULONG_MAX;

// 三層架構的鎖
HierarchicalMutex uiMutex(3000);
HierarchicalMutex logicMutex(2000);
HierarchicalMutex dataMutex(1000);

int main() {
#ifdef DEMONSTRATE_UB
    std::thread t1(thread1);
    std::thread t2(thread2);

    t1.join();
    t2.join();

    // 💀 一旦兩個執行緒真的交錯，就到不了這裡（實測每次都到不了）；
    //    但若排程剛好讓 thread1 先整段跑完，這行是【有可能】被印出來的。
    std::cout << "完成" << std::endl;
#else
    std::cout << "已略過極可能死結的示範（預設關閉）。" << std::endl;
    std::cout << "要重現請加上 -DDEMONSTRATE_UB 重新編譯，"
                 "並用 timeout 觀察它不會自行結束。" << std::endl;
#endif

    std::cout << "\n=== 解法 1: 統一取得順序（依記憶體位址排序）===" << std::endl;
    {
        std::mutex a, b;
        // 刻意讓兩條執行緒傳入相反的參數順序
        std::thread t1([&a, &b]() { for (int i = 0; i < 50; ++i) orderedWork(a, b); });
        std::thread t2([&a, &b]() { for (int i = 0; i < 50; ++i) orderedWork(b, a); });
        t1.join();
        t2.join();
        std::cout << "兩條執行緒以相反參數順序各跑 50 次: 全部完成，無死結"
                  << std::endl;
        std::cout << "原理: 實際取鎖順序由位址決定，永遠一致 → 形不成環"
                  << std::endl;
    }

    std::cout << "\n=== 解法 2: std::scoped_lock (C++17，推薦) ===" << std::endl;
    {
        std::mutex a, b;
        std::thread t1([&a, &b]() { for (int i = 0; i < 50; ++i) scopedWork(a, b); });
        std::thread t2([&a, &b]() { for (int i = 0; i < 50; ++i) scopedWork(b, a); });
        t1.join();
        t2.join();
        std::cout << "兩條執行緒以相反參數順序各跑 50 次: 全部完成，無死結"
                  << std::endl;
        std::cout << "原理: 一次取得多把鎖，內建 try-and-back-off 演算法"
                  << std::endl;
    }

    std::cout << "\n=== 日常實務: 鎖階層把機率性死結變成確定性錯誤 ===" << std::endl;
    {
        // ✅ 正確：由高層往低層取鎖
        try {
            std::lock_guard<HierarchicalMutex> l1(uiMutex);      // 3000
            std::lock_guard<HierarchicalMutex> l2(logicMutex);   // 2000
            std::lock_guard<HierarchicalMutex> l3(dataMutex);    // 1000
            std::cout << "由高層往低層取鎖 (3000 → 2000 → 1000): 成功" << std::endl;
        } catch (const std::logic_error& e) {
            std::cout << "非預期的違規: " << e.what() << std::endl;
        }

        // ❌ 違規：由低層往高層取鎖 → 立刻拋出例外，不會等到死結才發現
        try {
            std::lock_guard<HierarchicalMutex> l1(dataMutex);    // 1000
            std::lock_guard<HierarchicalMutex> l2(uiMutex);      // 3000 💀
            std::cout << "（這一行不會被執行）" << std::endl;
        } catch (const std::logic_error& e) {
            std::cout << "由低層往高層取鎖: 立刻被攔截" << std::endl;
            std::cout << "  錯誤訊息: " << e.what() << std::endl;
        }

        // 多執行緒驗證：全部遵守階層時，永遠不會死結
        std::atomic<int> completed{0};
        std::vector<std::thread> workers;
        for (int t = 0; t < 8; ++t) {
            workers.emplace_back([&completed]() {
                for (int i = 0; i < 200; ++i) {
                    std::lock_guard<HierarchicalMutex> l1(logicMutex);
                    std::lock_guard<HierarchicalMutex> l2(dataMutex);
                }
                completed.fetch_add(1, std::memory_order_relaxed);
            });
        }
        for (auto& t : workers) t.join();

        std::cout << "8 執行緒各取鎖 200 次（全部遵守階層），完成數: "
                  << completed.load() << "  (必須是 8)" << std::endl;
        std::cout << "價值: 把「可能上線才爆」的機率性死結，"
                     "變成「一違規就立刻拋例外」" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.4：互斥鎖的常見錯誤11.cpp' -o lock_order
// 觀察死結（會卡住，務必加 timeout）:
//   g++ -std=c++17 -Wall -Wextra -pthread -DDEMONSTRATE_UB '課程 5.4：互斥鎖的常見錯誤11.cpp' -o lock_order_ub
//   timeout 5 ./lock_order_ub ; echo "exit=$?"     # 本機實測 exit=124
// 在死結發生前就偵測順序反轉:
//   g++ -std=c++17 -pthread -g -fsanitize=thread -DDEMONSTRATE_UB '課程 5.4：互斥鎖的常見錯誤11.cpp' -o lock_order_tsan
//   （TSan 會報 lock-order-inversion，不需要死結真的發生）

// -----------------------------------------------------------------------------
// 【輸出但書】
//   1. 以下是【預設編譯】（不加 -DDEMONSTRATE_UB）的輸出。
//      加上 -DDEMONSTRATE_UB 後極可能死結；本機連續 5 次實測皆
//      timeout 逾時（exit=124）。但這【不是保證】——
//      若排程剛好讓 thread1 先整段跑完，程式會正常印出 "完成" 並結束。
//   2. 其餘輸出完全確定：兩種解法與鎖階層的檢查都不依賴排程順序，
//      多執行緒區段只驗證「全部完成」這個不變量。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// 已略過極可能死結的示範（預設關閉）。
// 要重現請加上 -DDEMONSTRATE_UB 重新編譯，並用 timeout 觀察它不會自行結束。
//
// === 解法 1: 統一取得順序（依記憶體位址排序）===
// 兩條執行緒以相反參數順序各跑 50 次: 全部完成，無死結
// 原理: 實際取鎖順序由位址決定，永遠一致 → 形不成環
//
// === 解法 2: std::scoped_lock (C++17，推薦) ===
// 兩條執行緒以相反參數順序各跑 50 次: 全部完成，無死結
// 原理: 一次取得多把鎖，內建 try-and-back-off 演算法
//
// === 日常實務: 鎖階層把機率性死結變成確定性錯誤 ===
// 由高層往低層取鎖 (3000 → 2000 → 1000): 成功
// 由低層往高層取鎖: 立刻被攔截
//   錯誤訊息: 鎖階層違規：目前持有層級 1000，卻嘗試取得層級 3000（只能由高層往低層取）
// 8 執行緒各取鎖 200 次（全部遵守階層），完成數: 8  (必須是 8)
// 價值: 把「可能上線才爆」的機率性死結，變成「一違規就立刻拋例外」
