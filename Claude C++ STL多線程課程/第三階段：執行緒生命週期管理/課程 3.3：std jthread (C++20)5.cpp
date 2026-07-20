// =============================================================================
//  課程 3.3：std::jthread (C++20) — 第 5 部分：stop_callback 停止回調
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<stop_token>、<thread>
//   標準版本：C++20
//
//     template<class Callback>
//     class stop_callback {
//         template<class C>
//         explicit stop_callback(const std::stop_token& st, C&& cb);  // 註冊
//         ~stop_callback();                                           // 解除註冊
//         stop_callback(const stop_callback&) = delete;               // 不可複製/移動
//     };
//     // C++17 起有 CTAD，所以可以直接寫 std::stop_callback cb(token, lambda);
//     // 不必寫成 std::stop_callback<decltype(lambda)>。
//
//   複雜度：註冊/解除註冊為 O(1)（插入/移除一個侵入式串列節點）。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼需要回調：輪詢解不了阻塞】
//   前面幾個部分都是用「輪詢」的方式取消：
//       while (!st.stop_requested()) { ...做一小段工作... }
//   這個模式有個致命前提：執行緒必須【定期回到迴圈頂端】。
//   但如果執行緒正阻塞在
//       * accept() 等待新連線
//       * read() 等待資料
//       * condition_variable::wait() 等待通知
//   它根本不會回到迴圈頂端，旗標設了也沒人看。
//
//   stop_callback 解決的正是這件事：把「停止時要做什麼」註冊成回調，
//   由 request_stop() 的呼叫端【同步】執行。於是你可以在回調裡
//   close(fd)、cv.notify_all()、寫入 eventfd —— 主動把阻塞中的執行緒踹醒。
//
// 【2. 執行時機與執行者：這是最容易搞錯的地方】
//   標準規定（[stopcallback.cons]）：
//     * 若註冊時停止【尚未】被請求 → 回調存起來，
//       之後由呼叫 request_stop() 的【那條執行緒】同步執行。
//     * 若註冊時停止【已經】被請求 → 回調在 stop_callback 的
//       【建構子裡】立刻執行，執行者是建構它的那條執行緒。
//   換句話說：回調【不保證】在哪一條執行緒上跑。
//   這對回調內容有嚴格要求 —— 它必須是執行緒安全的，
//   而且不能假設自己跑在工作執行緒上。
//
// 【3. RAII 的解除註冊，以及它的阻塞語意】
//   ~stop_callback() 會把回調從共享狀態的串列中移除。
//   關鍵細節：若解構時該回調【正在別的執行緒上執行】，
//   解構子會【阻塞等它執行完】才返回。
//   這個保證非常重要：它確保「解構子返回之後，回調絕對不會再被呼叫，
//   也絕對不在執行中」，於是回調捕獲的物件可以安全銷毀。
//   代價是：如果回調自己去 join 那條正在解構 stop_callback 的執行緒，就會死結。
//
// 【4. 為什麼不可複製也不可移動】
//   stop_callback 在共享狀態中是以侵入式串列節點的形式存在，
//   節點裡存的是「指向這個 stop_callback 物件的指標」。
//   若允許移動，指標就會指向舊位置 → 懸空。
//   標準因此直接刪除複製與移動建構子。
//   要延後建立就用 std::optional<std::stop_callback>，或用指標持有。
//
// 【概念補充 Concept Deep Dive】
//   * 回調串列的資料結構：libstdc++ 用侵入式雙向串列掛在 _Stop_state_t 上，
//     所以註冊與解除都是 O(1)，且不需要額外的堆積配置 ——
//     節點就內嵌在 stop_callback 物件自己身上。
//
//   * 多個回調的執行順序【未指定】。標準沒有規定 LIFO 或 FIFO，
//     所以不要寫出依賴回調順序的邏輯。要有順序就自己在單一回調裡串起來。
//
//   * 回調中拋出例外會怎樣？
//     標準規定：若回調以例外結束，會呼叫 std::terminate()。
//     理由與解構子相同 —— 回調是在 request_stop() 的路徑上同步執行的，
//     那裡沒有合理的地方可以處理例外。所以回調內部必須自己 catch 乾淨。
//
//   * 與 stop_requested() 輪詢的成本比較：
//     輪詢是每圈一次 atomic load（極便宜但有延遲）；
//     回調是零輪詢成本但註冊/解除各要一次帶鎖的串列操作。
//     兩者互補而非互斥 —— 實務上常常同時用：
//     用回調踹醒阻塞，用輪詢決定何時退出迴圈。
//
// 【注意事項 Pay Attention】
//   1. 回調可能在【任何一條執行緒】上執行（取決於誰呼叫 request_stop()，
//      或註冊時是否已經 requested）。回調內容必須執行緒安全。
//   2. 回調中【絕對不要】做耗時工作或取得可能被長期持有的鎖 ——
//      它是同步執行的，會拖住 request_stop() 的呼叫端。
//   3. 回調中拋出例外 → std::terminate()。務必自己 catch。
//   4. stop_callback 不可複製、不可移動。需要延後建立請用 std::optional。
//   5. 本檔輸出中「工作中...」的行數與時間相關，每次執行都可能不同。
//   6. 本檔必須用 -std=c++20（已用 -pedantic-errors 實測驗證）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】stop_callback
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. stop_callback 的回調在哪一條執行緒上執行？
//     答：不保證。若註冊時尚未請求停止，回調由【呼叫 request_stop() 的那條
//         執行緒】同步執行；若註冊時已經請求過，回調在 stop_callback 的
//         【建構子裡立刻】執行，執行者是建構它的執行緒。
//         因此回調內容必須是執行緒安全的，且不能假設自己在工作執行緒上跑。
//     追問：那回調裡可以做耗時工作嗎？
//         → 不應該。它會同步拖住 request_stop() 的呼叫端。
//           回調應該只做「踹醒」這類極短的動作，例如 notify_all() 或 close(fd)。
//
// 🔥 Q2. 已經有 stop_requested() 輪詢了，為什麼還需要 stop_callback？
//     答：輪詢的前提是執行緒會定期回到迴圈頂端。若執行緒阻塞在 accept()、
//         read() 或 cv.wait() 上，它永遠不會去檢查旗標，設了也沒用。
//         stop_callback 讓你在停止發生的當下主動出手 ——
//         關閉 fd、notify_all()、寫 eventfd，把阻塞中的執行緒踹醒。
//     追問：C++20 有沒有更直接的做法？
//         → 有。std::condition_variable_any 新增了
//           wait(lock, stop_token, pred) 多載，內部就是用 stop_callback
//           實作的，等待時可被 request_stop() 直接喚醒。
//
// ⚠️ 陷阱. 「~stop_callback() 只是把回調從串列拿掉，所以它一定很快」——錯在哪？
//     答：解構子在「回調正於另一條執行緒執行中」時，會【阻塞等待該回調執行完】
//         才返回。這是標準刻意給的保證，目的是讓「解構子返回後，
//         回調絕不會再被呼叫、也不在執行中」成立，回調捕獲的物件才能安全銷毀。
//     為什麼會錯：把解除註冊想成單純的串列移除（O(1) 就結束）。
//         實際上它含有一個同步點。真正的危險後果是死結：
//         若回調內部去 join()（或等待）那條正在解構 stop_callback 的執行緒，
//         兩邊互等就鎖死了。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔從缺
//   stop_callback 解決的是「如何叫醒阻塞中的執行緒」這個作業系統層面的問題。
//   LeetCode 並行題（1114/1115/1116/1117/1195）中的執行緒只會阻塞在
//   題目自己的同步原語上，且必然會被同題的其他執行緒喚醒，
//   從來不需要外部取消。故本檔沒有真正對應的題目，從缺。

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <stop_token>
#include <string>
#include <thread>

// -----------------------------------------------------------------------------
// 【基本示範】註冊停止回調
// -----------------------------------------------------------------------------
void demoBasicCallback() {
    std::jthread jt([](std::stop_token stoken) {
        // 註冊停止時的回調
        std::stop_callback callback(stoken, []() {
            std::cout << "停止回調被觸發！" << std::endl;
        });

        while (!stoken.stop_requested()) {
            std::cout << "工作中..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }

        std::cout << "執行緒結束" << std::endl;
    });  // ← callback 在 lambda 結束時解構、自動解除註冊

    std::this_thread::sleep_for(std::chrono::seconds(1));
    jt.request_stop();
}

// -----------------------------------------------------------------------------
// 【對照示範】註冊時「已經」請求停止 → 回調在建構子裡立刻執行
// -----------------------------------------------------------------------------
void demoLateRegistration() {
    std::stop_source src;
    src.request_stop();   // 先請求停止

    std::cout << "  即將建立 stop_callback（此時 stop 已被請求）..." << std::endl;
    std::stop_callback cb(src.get_token(), []() {
        std::cout << "  → 回調在【建構子內】就立刻執行了" << std::endl;
    });
    std::cout << "  stop_callback 建構完成" << std::endl;
}

// -----------------------------------------------------------------------------
// 【對照示範】回調由「呼叫 request_stop() 的那條執行緒」執行
// -----------------------------------------------------------------------------
void demoWhichThreadRuns() {
    std::stop_source src;
    const auto mainId = std::this_thread::get_id();

    std::stop_callback cb(src.get_token(), [mainId]() {
        bool onMain = (std::this_thread::get_id() == mainId);
        std::cout << "  回調執行於: " << (onMain ? "main 執行緒" : "其他執行緒")
                  << std::endl;
    });

    std::cout << "  由 main 直接呼叫 request_stop():" << std::endl;
    src.request_stop();   // ← 回調在 main 上同步執行
}

// -----------------------------------------------------------------------------
// 【日常實務範例】可即時中斷的任務佇列 worker（回調 + condition_variable）
//   情境：典型的執行緒池 worker —— 佇列空的時候阻塞在 cv.wait() 上等新任務。
//         關機時如果只設 stop 旗標，worker 還卡在 wait() 裡，
//         沒有人 notify 它就永遠不會醒來，解構子會永久阻塞。
//   為何用 stop_callback：把 cv.notify_all() 註冊成停止回調。
//         request_stop() 的瞬間回調就被執行 → worker 被喚醒 →
//         重新檢查述詞發現要停止 → 乾淨退出。
//         這是「輪詢解不了阻塞」的標準解法，也是 condition_variable_any
//         的 token 多載內部所做的事。
// -----------------------------------------------------------------------------
class TaskQueue {
public:
    void push(const std::string& task) {
        {
            std::lock_guard lock(mtx_);
            tasks_.push(task);
        }
        cv_.notify_one();
    }

    void runWorker(std::stop_token st) {
        // 關鍵：把「喚醒所有等待者」註冊成停止回調
        std::stop_callback wakeUp(st, [this]() {
            std::cout << "  [callback] 停止請求送達 → notify_all() 踹醒 worker"
                      << std::endl;
            cv_.notify_all();
        });

        while (true) {
            std::unique_lock lock(mtx_);
            // 沒有回調的話，這裡在佇列空時會永久阻塞
            cv_.wait(lock, [&] { return !tasks_.empty() || st.stop_requested(); });

            if (st.stop_requested() && tasks_.empty()) {
                std::cout << "  [worker] 佇列已清空且收到停止請求，退出" << std::endl;
                return;
            }

            std::string task = tasks_.front();
            tasks_.pop();
            lock.unlock();

            std::cout << "  [worker] 處理任務: " << task << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
        }
    }

private:
    std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<std::string> tasks_;
};

int main() {
    std::cout << "=== 基本示範：註冊停止回調 ===" << std::endl;
    demoBasicCallback();

    std::cout << "\n=== 註冊時已請求停止 → 回調立刻在建構子執行 ===" << std::endl;
    demoLateRegistration();

    std::cout << "\n=== 回調由誰執行？ ===" << std::endl;
    demoWhichThreadRuns();

    std::cout << "\n=== 日常實務：可即時中斷的任務佇列 worker ===" << std::endl;
    {
        TaskQueue q;
        std::jthread worker([&q](std::stop_token st) { q.runWorker(st); });

        q.push("resize-image-001");
        q.push("resize-image-002");
        std::this_thread::sleep_for(std::chrono::milliseconds(150));

        std::cout << "  main: 佇列已空，worker 正阻塞在 cv.wait() 上" << std::endl;
        std::cout << "  main: 送出停止請求" << std::endl;
        worker.request_stop();
    }  // worker 解構 → join（因為有回調踹醒，這裡不會卡住）
    std::cout << "  main: worker 已回收（沒有卡在 wait 上）" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra -pthread "課程 3.3：std jthread (C++20)5.cpp" -o jthread5
//   注意：本檔【必須】用 -std=c++20（std::jthread / std::stop_callback 為 C++20 新增，
//   已用 -std=c++17 -pedantic-errors 實測確認編譯失敗）。

// 註:
//   ⚠️ 「工作中...」的行數取決於執行緒排程，【每次執行都不同】。
//   實務範例中 worker 與 main 的輸出交錯順序也可能改變。

// === 預期輸出 ===
// === 基本示範：註冊停止回調 ===
// 工作中...
// 工作中...
// 工作中...
// 工作中...
// 停止回調被觸發！
// 執行緒結束
//
// === 註冊時已請求停止 → 回調立刻在建構子執行 ===
//   即將建立 stop_callback（此時 stop 已被請求）...
//   → 回調在【建構子內】就立刻執行了
//   stop_callback 建構完成
//
// === 回調由誰執行？ ===
//   由 main 直接呼叫 request_stop():
//   回調執行於: main 執行緒
//
// === 日常實務：可即時中斷的任務佇列 worker ===
//   [worker] 處理任務: resize-image-001
//   [worker] 處理任務: resize-image-002
//   main: 佇列已空，worker 正阻塞在 cv.wait() 上
//   main: 送出停止請求
//   [callback] 停止請求送達 → notify_all() 踹醒 worker
//   [worker] 佇列已清空且收到停止請求，退出
//   main: worker 已回收（沒有卡在 wait 上）
