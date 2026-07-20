// =============================================================================
//  課程 2.7：執行緒的移動語意 5  —  std::vector<std::thread>：容器管理執行緒
// =============================================================================
//
// 【主題資訊 Information】
//   容器：std::vector<std::thread>      標頭檔：<vector> <thread>
//   標準：C++11 起（容器支援 move-only 元素）
//
//   兩種放入方式：
//     v.push_back(std::thread(f));   // 建立暫存 thread → 移動進容器（1 次移動）
//     v.emplace_back(f);             // 在容器記憶體上**直接建構**（0 次移動）
//     v.push_back(std::move(t));     // 把既有具名 thread 移進容器
//     v.push_back(t);                // ✗ 編譯錯誤：複製被 delete
//
//   容器對 move-only 型別的要求：
//     元素只需 MoveInsertable 即可放入 vector；**不需要**可複製。
//     但 v.resize(n)、v.insert(pos, n, val) 這類需要複製的操作用不了。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 vector 裝得下 move-only 的 thread】
// C++11 之前的 STL 容器要求元素必須 CopyConstructible，所以 vector<thread>
// 在 C++03 的世界是不可能的。C++11 把要求放寬成「依操作而定」：
//   * push_back(T&&) / emplace_back(...) 只要求 MoveInsertable
//   * 重新配置（reallocation）只要求可移動
//   * 只有真正需要複製的操作（如 resize 的填值版本）才要求 CopyInsertable
// 所以 vector<thread> 完全合法，只是你會發現某些成員函式用不了 ——
// 那不是 bug，是型別要求沒被滿足，編譯器會明確告訴你。
//
// 【2. 重新配置時發生什麼：為什麼「移動必須 noexcept」是關鍵】
// vector 容量不足時要配置更大的緩衝區，再把舊元素搬過去。搬的時候用
// std::move_if_noexcept 決定策略：
//
//   若移動建構是 noexcept        → 用移動（快，而且中途不可能失敗）
//   若移動可能丟例外 且 可複製   → 用複製（保住強例外保證：搬到一半失敗
//                                  還能把舊緩衝區原封不動留著）
//   若移動可能丟例外 且 不可複製 → 只能用移動，強例外保證降級為基本保證
//
// std::thread 的移動建構子是 noexcept（本機實測
// is_nothrow_move_constructible_v<std::thread> == true），所以落在第一種：
// vector 放心用移動，而且 push_back 仍維持強例外保證。
//
// 這件事為什麼重要？想像若移動可能丟例外：vector 搬到第 3 個執行緒握把時
// 丟例外，此時舊緩衝區有 2 個已被清空的握把、新緩衝區有 2 個有效握把，
// 而且 vector 正在解構中 —— 這批執行緒會**沒有任何人能 join**。
// noexcept 讓這個災難在型別系統層級被排除。
//
// 【3. push_back vs emplace_back：差在哪、什麼時候真的有差】
//     v.push_back(std::thread([]{...}));   // ① 建構暫存 thread
//                                          // ② 移動建構到容器內
//                                          // ③ 暫存物件解構（此時已 non-joinable）
//     v.emplace_back([]{...});             // ① 用 lambda 直接在容器記憶體上
//                                          //    建構 thread —— 沒有暫存、沒有移動
//
// 對 thread 而言差距很小（移動只是搬 8 bytes），但 emplace_back 語意更直接：
// 「用這些參數就地造一個」。而且 emplace_back 可以直接吃執行緒函式的參數：
//     v.emplace_back(worker, id, config);   // 等價於 std::thread(worker, id, config)
//
// **反直覺提醒**：emplace_back 不是永遠比較好。它使用**直接初始化**，
// 會繞過 explicit 檢查，偶爾會意外選中你不想要的建構子。
// 對已經有現成物件要放進去的情況，push_back(std::move(t)) 反而更清楚。
//
// 【4. reserve：不只是效能，對執行緒容器更是可讀性問題】
// 迴圈裡 push_back 會經歷多次重新配置（本機實測 capacity 變化為
// 1 → 2 → 4 → 4 → 8），每次都要把已存在的握把全部搬一遍。
// 對 thread 而言成本仍然很低（沒有系統呼叫，只是搬 8 bytes），
// 但 reserve(n) 讓意圖更明確、也避免不必要的搬移：
//     threads.reserve(N);   // 一次配置到位
// 注意：reserve **不會**影響已啟動的執行緒 —— 搬的是握把，不是執行緒。
//
// 【5. 收尾：一定要走完整個容器 join】
// 最常見的錯誤是「只 join 了一部分」或「迴圈中途 break」。
// vector 解構時會解構每個元素，任何一個還 joinable 的 thread
// 都會觸發 std::terminate()。所以收尾迴圈必須無條件走完：
//     for (auto& t : threads) if (t.joinable()) t.join();
// 加上 joinable() 檢查是好習慣：容器中可能混有已被移走的空握把。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼一定要用 auto& 而不是 auto
//     for (auto t : threads) t.join();   // ✗ 編譯錯誤
// auto 會嘗試**複製**元素 → 複製建構子被 delete → 編譯失敗。
// 這裡編譯器幫了大忙：換成別的型別（例如 vector<string>），
// 寫 auto 只是靜默地多一次複製，效能損失還不會被發現。
// **對容器迴圈一律優先寫 auto&（或 const auto&）** 是通則。
//
// (B) 迭代器失效：thread 容器特別容易踩
// push_back 觸發重新配置後，先前取得的 iterator/reference/pointer 全部失效。
// 若你寫了：
//     auto& first = threads[0];
//     threads.push_back(...);   // 可能重新配置
//     first.join();             // ✗ 懸空參考 → UB
// 這是真正的 UB（不是丟例外）。解法：reserve 到位，或用索引而非參考。
//
// (C) 為什麼不用 std::array<std::thread, N>
// 可以用，但 array 的元素在建立時就必須全部預設建構（變成 N 個空握把），
// 之後再逐一移動賦值進去。因為賦值目標是空的（non-joinable），
// 所以不會觸發 terminate，是安全的。但 vector 語意更貼近
// 「動態決定要開幾條」的實際需求。
//
// (D) 真正的 thread pool 與本檔的差別
// 本檔示範的是「一個任務一條執行緒」（fork-join）。真正的 thread pool 是
// 「固定 N 條長命執行緒 + 一個工作佇列」，避免反覆建立/銷毀執行緒的成本
// （在 Linux 上一次 clone() 大約數十微秒，加上預設 8MB stack 的虛擬記憶體）。
// 兩者共通的地方是：worker 執行緒的握把都存在 vector<thread> 裡，
// 而且都靠移動語意才能放進容器。本檔實務範例示範的是後者的骨架。
//
// 【注意事項 Pay Attention】
// 1. for (auto& t : threads) —— 必須是參考，寫 auto 會編譯錯誤（複製被 delete）。
// 2. 容器解構前必須把每個元素 join 或 detach，否則 terminate。
// 3. 多條執行緒同時對 std::cout 輸出，**文字會交錯**。這是 race condition
//    （順序不保證），**不是 data race** —— 標準保證對同步過的標準串流
//    並行輸出不產生 data race（[iostream.objects]），程式沒有 UB。
//    要有序輸出必須自己加 mutex，或各自算完再由主執行緒統一印。
// 4. lambda 捕捉迴圈變數請用**值** [i]。若寫 [&i] 捕捉參考，i 在迴圈結束後
//    就消滅了，執行緒讀到懸空參考 → UB。
// 5. 執行緒的**啟動順序**不等於**執行順序**，也不等於**完成順序**。
//    本檔輸出的數字順序每次都不同，這是正常的。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector<std::thread>
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::thread 不能複製，為什麼可以放進 std::vector？
//     答：C++11 起容器的元素要求依操作而定 —— push_back(T&&) 與
//         emplace_back 只需要 MoveInsertable，不需要 CopyInsertable。
//         vector 重新配置時也是用移動搬元素。只有真正需要複製的操作
//         （如 resize 的填值版本）才會編譯失敗。
//     追問：那 vector 重新配置時，會不會有執行緒被搬丟？
//           → 不會。搬的是 8 bytes 的握把，執行緒本身完全不受影響；
//             而且移動建構是 noexcept，搬運途中不可能失敗。
//
// 🔥 Q2. for (auto t : threads) t.join(); 為什麼編譯不過？
//     答：auto 推導成 std::thread（值），range-for 會嘗試從元素**複製**
//         建構 t，而複製建構子被 = delete → 編譯錯誤。
//         必須寫 for (auto& t : threads)。
//     追問：如果真的想在迴圈裡取走元素呢？
//           → for (auto&& t : threads) 仍是綁定，不會取走；
//             要取走得明確寫 std::thread taken = std::move(threads[i]);
//
// ⚠️ 陷阱. 「push_back 時 vector 重新配置，會把 thread 複製一份，
//            所以要先 reserve 才安全」
//     答：前半錯、結論對但理由錯。重新配置**不會**複製（複製被 delete，
//         真要複製會編譯失敗），它用的是移動，而且因為移動是 noexcept，
//         整個過程安全且維持強例外保證。reserve 的價值是避免多次重新配置
//         與**避免迭代器/參考失效**，不是「避免複製執行緒」。
//     為什麼會錯：把 vector 對一般型別的行為（可能複製）直接套到 move-only
//         型別上。真正該擔心的是**先前取得的參考在 push_back 後失效**，
//         那才是實際會造成 UB 的問題。
//
// ⚠️ 陷阱 2. 「emplace_back 一定比 push_back 快，所以永遠用 emplace_back」
//     答：對 thread 而言差別是「省下一次搬 8 bytes」，實務上量不出來。
//         而且 emplace_back 用的是**直接初始化**，會繞過 explicit 檢查，
//         有時會意外選中非預期的建構子。已有現成物件時，
//         push_back(std::move(t)) 語意更清楚。
//     為什麼會錯：把「少一次移動」當成必然的效能勝利，
//         但對握把型別，移動成本本來就趨近於零。
// ═══════════════════════════════════════════════════════════════════════════

#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>
#include <vector>

// -----------------------------------------------------------------------------
// 【日常實務範例】固定大小 thread pool：vector<std::thread> 管理 worker
// 情境：影像縮圖服務。若每個請求都 new 一條執行緒，尖峰時會爆出上千條
//       執行緒（每條預設 8MB stack 的虛擬位址空間 + 每次 clone() 的成本）。
//       正解是固定開 N 條長命 worker，共用一個工作佇列。
// 移動語意在哪裡用到：
//   * workers_.emplace_back(...) 把 worker 就地建構進 vector；
//   * vector 擴充時用移動搬握把（noexcept，安全）；
//   * 整個 pool 因為持有 vector<thread> 而自動變成 move-only。
// 同步重點（本課雖不深入，但這是正確可用的骨架）：
//   * mutex 保護佇列，condition_variable 讓 worker 沒事時睡著不空轉；
//   * stop_ 旗標 + notify_all 讓 shutdown 能喚醒所有人結束；
//   * 輸出先組成完整字串再一次 << 出去，避免文字交錯。
// -----------------------------------------------------------------------------
class ThreadPool {
    std::vector<std::thread>          workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex                        mtx_;
    std::condition_variable           cv_;
    bool                              stop_ = false;

public:
    explicit ThreadPool(unsigned n) {
        workers_.reserve(n);                    // 一次配置到位，不再重新配置
        for (unsigned i = 0; i < n; ++i) {
            // emplace_back：直接在容器記憶體上建構 thread，沒有暫存物件
            workers_.emplace_back([this, i]() { workerLoop(i); });
        }
    }

    // 持有 vector<thread> → 自動 move-only（複製被成員的 delete 傳染）
    ThreadPool(const ThreadPool&)            = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void submit(std::function<void()> job) {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            tasks_.push(std::move(job));
        }
        cv_.notify_one();
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            stop_ = true;
        }
        cv_.notify_all();                       // 喚醒所有睡著的 worker
        for (auto& w : workers_) {              // 必須 auto&，auto 會編譯錯誤
            if (w.joinable()) w.join();
        }
        workers_.clear();
    }

    ~ThreadPool() { shutdown(); }               // RAII：絕不讓 joinable 的握把解構

private:
    void workerLoop(unsigned id) {
        for (;;) {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lk(mtx_);
                cv_.wait(lk, [this] { return stop_ || !tasks_.empty(); });
                if (stop_ && tasks_.empty()) return;
                job = std::move(tasks_.front());
                tasks_.pop();
            }
            std::ostringstream oss;             // 先組好整行，避免輸出交錯
            oss << "  [worker " << id << "] 處理一個任務\n";
            std::cout << oss.str();
            job();
        }
    }
};

int main() {
    std::cout << "=== 原始示範：vector<thread> 建立與 join ===\n";
    std::vector<std::thread> threads;
    threads.reserve(5);                 // 避免重新配置，也避免參考失效

    // 建立多個執行緒
    for (int i = 0; i < 5; ++i) {
        threads.push_back(std::thread([i]() {   // [i] 以值捕捉：必要！
            std::cout << "執行緒 " << i << std::endl;
        }));
        // 或使用 emplace_back（就地建構，不產生暫存物件）
        // threads.emplace_back([i]() { ... });
    }

    // 等待所有執行緒完成（必須 auto&；寫 auto 會因複製被 delete 而編譯失敗）
    for (auto& t : threads) {
        t.join();
    }
    std::cout << "全部 join 完成，容器大小 = " << threads.size() << "\n";

    std::cout << "\n=== capacity 成長：每次重新配置都會「移動」握把 ===\n";
    std::vector<std::thread> grow;
    std::cout << "初始 capacity = " << grow.capacity() << "\n";
    for (int i = 0; i < 5; ++i) {
        grow.emplace_back([]() {});     // 空工作，只看容器行為
        std::cout << "  放入第 " << (i + 1) << " 個後 capacity = "
                  << grow.capacity() << "\n";
    }
    for (auto& t : grow) t.join();
    std::cout << "（移動的是 8 bytes 握把，執行緒本身完全不受影響）\n";

    std::cout << "\n=== 實務：固定大小 thread pool ===\n";
    {
        std::atomic<int> done{0};
        ThreadPool pool(3);             // 3 條長命 worker
        for (int i = 0; i < 6; ++i) {
            pool.submit([&done]() { done.fetch_add(1, std::memory_order_relaxed); });
        }
        pool.shutdown();                // 等所有任務做完並 join 全部 worker
        // shutdown 已 join 全部 worker，此處讀取 done 沒有並行存取問題
        std::cout << "  完成任務數 = " << done.load(std::memory_order_relaxed) << "\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.7：執行緒的移動語意5.cpp" -o move5

// 註：以下為某次實際執行結果。**每次執行都不同**：

// === 預期輸出 ===
//   * "執行緒 0..4" 的**順序每次都不一樣**（啟動順序 ≠ 執行順序），
//     而且文字可能在同一行中間交錯（race condition，非 data race）。
//   * thread pool 中哪個 worker 接到哪個任務、各接幾個，完全看排程；
//     只有「完成任務數 = 6」是確定的（由 atomic 保證）。
//   * capacity 成長序列 1→2→4→4→8 是 libstdc++ 的成長策略（實作定義，
//     常見為 2 倍成長；MSVC 用 1.5 倍，數列會不同）。
//
// === 原始示範：vector<thread> 建立與 join ===
// 執行緒 執行緒 01
// 執行緒 2
//
// 執行緒 3
// 執行緒 4
// 全部 join 完成，容器大小 = 5
//
//   ↑ 第 2～4 行就是**實際跑出來的交錯**：執行緒 0 印完 "執行緒 " 之後
//     被執行緒 1 插隊，於是變成 "執行緒 執行緒 01"，換行也跟著跑位。
//     這是 race condition（順序不保證），不是 data race（無 UB）。
//
// === capacity 成長：每次重新配置都會「移動」握把 ===
// 初始 capacity = 0
//   放入第 1 個後 capacity = 1
//   放入第 2 個後 capacity = 2
//   放入第 3 個後 capacity = 4
//   放入第 4 個後 capacity = 4
//   放入第 5 個後 capacity = 8
// （移動的是 8 bytes 握把，執行緒本身完全不受影響）
//
// === 實務：固定大小 thread pool ===
//   [worker 2] 處理一個任務
//   [worker 0] 處理一個任務
//   [worker 0] 處理一個任務
//   [worker 1] 處理一個任務
//   [worker 2] 處理一個任務
//   [worker 0] 處理一個任務
//   完成任務數 = 6
//
//   ↑ 另一次執行的同一段是「worker 1 一個人接走全部 6 個任務」——
//     完全合法：worker 拿到鎖後若佇列還有工作就繼續拿，
//     沒有任何機制保證任務會平均分配。只有「完成任務數 = 6」是確定的。
