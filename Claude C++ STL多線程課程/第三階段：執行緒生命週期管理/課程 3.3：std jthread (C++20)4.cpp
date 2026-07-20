// =============================================================================
//  課程 3.3：std::jthread (C++20) — 第 4 部分：stop_source 與 stop_token 的取得
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<stop_token>、<thread>
//   標準版本：C++20
//
//     std::stop_source jthread::get_stop_source()       noexcept;  // 回傳【值】
//     std::stop_token  jthread::get_stop_token()  const noexcept;  // 回傳【值】
//     bool             jthread::request_stop()          noexcept;
//
//   ⚠️ 兩者都是【回傳值（by value）】，不是回傳 reference。
//      寫成 std::stop_source& s = jt.get_stop_source(); 會編譯失敗，
//      因為 non-const lvalue reference 不能綁定到 prvalue。
//      正確寫法是用 auto 或直接用值接。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼回傳值而不是 reference】
//   stop_source 與 stop_token 都是【把手（handle）型別】——
//   它們本身很小（通常就是一個指向共享停止狀態的指標），
//   複製它們只是把引用計數 +1，不會複製底層狀態。
//   這與 shared_ptr 的設計哲學完全一致：把手可以自由複製，狀態只有一份。
//
//   回傳值的好處是安全：即使原本的 jthread 已經解構了，
//   你手上那份 stop_source / stop_token 仍然有效（共享狀態被引用計數保護著），
//   不會變成 dangling reference。若回傳 reference，jthread 一死就懸空了。
//
// 【2. 三種發出停止請求的方式，效果完全相同】
//     (a) jt.request_stop();                    // 直接透過 jthread
//     (b) auto src = jt.get_stop_source();
//         src.request_stop();                   // 透過取得的 source
//     (c) std::stop_source src;                 // 自己建立 source
//         std::jthread jt2(f, src.get_token()); // 手動把 token 傳給 thread
//         src.request_stop();
//   三者操作的都是同一塊共享停止狀態（前兩者），或你自己管理的狀態（第三者）。
//   選哪一種取決於「誰應該有權力叫停」：
//     * 只有擁有 jthread 的人能叫停 → 用 (a)
//     * 要把「叫停權」交給別的模組（但不交出執行緒本身）→ 用 (b)，傳 source 過去
//     * 要一個 source 同時控制多條執行緒 → 用 (c)
//
// 【3. source 與 token 的權責分離】
//   這是本檔最重要的設計觀念：
//     * stop_source 有 request_stop() → 是【控制端】的憑證
//     * stop_token  沒有 request_stop() → 是【工作端】的憑證，唯讀
//   把 token 傳給工作執行緒、把 source 留給控制端，型別系統就替你保證了
//   「工作執行緒不可能自己叫停自己以外的東西」。
//   這比「傳一個 atomic<bool>& 進去，大家都能寫」安全得多 ——
//   後者任何人都能改，靠的是紀律；前者靠的是型別。
//
// 【4. 一個 source 控制多條執行緒（fan-out 取消）】
//   共享停止狀態可以被任意多個 token 觀察。所以
//       std::stop_source src;
//       for (int i = 0; i < N; ++i)
//           workers.emplace_back(work, src.get_token());
//       src.request_stop();   // ← 一次叫停全部 N 條
//   這是實作「優雅關機（graceful shutdown）」最乾淨的方式。
//   注意此時要用 std::thread 或手動傳 token 給 jthread，
//   因為 jthread 自動注入的是【它自己的】token，不是你的 src 的。
//
// 【概念補充 Concept Deep Dive】
//   * 共享狀態的實體：libstdc++ 中是一個堆積配置的 _Stop_state_t，
//     內含 atomic 的旗標與回調串列，以及兩個引用計數
//     （一個算 source 數、一個算總把手數）。
//     source 計數歸零且未曾請求停止時，stop_possible() 會轉為 false ——
//     因為再也沒有人有能力發出請求了。
//
//   * sizeof：stop_source 與 stop_token 在 libstdc++ 上都是一個指標大小
//     （x86-64 為 8 bytes）。這是【實作定義】的數值，標準未規定。
//     本檔會實際印出來驗證。
//
//   * 為什麼 get_stop_token() 是 const 而 get_stop_source() 不是：
//     取得唯讀的觀察端不改變 jthread 的邏輯狀態，所以是 const；
//     取得可寫的控制端則被視為交出了修改能力，標準因此不標 const。
//
//   * 預設建構的 std::stop_token 沒有關聯任何共享狀態：
//     stop_requested() 恆為 false、stop_possible() 恆為 false。
//     這是「這個工作不支援取消」的合法表達方式，不是錯誤狀態。
//
// 【注意事項 Pay Attention】
//   1. 【最常見的編譯錯誤】把回傳值當 reference 接：
//        std::stop_source& s = jt.get_stop_source();   // ✗ 編譯失敗
//        auto s = jt.get_stop_source();                // ✓ 正確
//        const std::stop_source& s = jt.get_stop_source(); // 可編譯（const ref
//                                                          // 可延長 prvalue 壽命）
//                                                          // 但沒有理由這樣寫
//   2. 對 jthread 呼叫 detach() 之後，解構子不再 join，
//      但你先前取得的 stop_source / stop_token 仍然有效。
//   3. 停止請求不可逆。沒有辦法把 stop_requested() 從 true 改回 false。
//   4. 本檔必須用 -std=c++20（已用 -pedantic-errors 實測驗證）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】stop_source / stop_token 的取得與所有權
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. get_stop_source() 回傳值還是 reference？寫錯會怎樣？
//     答：回傳【值】。寫成 std::stop_source& s = jt.get_stop_source();
//         會編譯失敗，因為 non-const lvalue reference 綁不到 prvalue。
//         正確用 auto 接。回傳值是刻意設計：stop_source 是引用計數的把手，
//         複製成本極低，而且 jthread 死掉後你手上的把手依然有效。
//     追問：既然回傳值，那複製一份 source 會不會讓停止狀態變成兩份？
//         → 不會。所有複製出來的把手共用同一塊堆積狀態，
//           就像 shared_ptr 一樣，任何一個 request_stop() 全部都看得到。
//
// 🔥 Q2. stop_source 和 stop_token 差在哪？為什麼要分成兩個型別？
//     答：source 有 request_stop()（控制端），token 沒有（工作端唯讀）。
//         分成兩個型別是為了用型別系統落實權責分離：
//         把 token 傳給工作執行緒，它在型別上就【不可能】發出停止請求。
//         若共用一個型別或傳 atomic<bool>&，就只能靠寫程式的人自律。
//     追問：怎麼讓一個 source 同時叫停十條執行緒？
//         → 自己建 stop_source，把 src.get_token() 分別傳給每條執行緒，
//           呼叫一次 src.request_stop() 就全部收到。這是優雅關機的標準寫法。
//
// ⚠️ 陷阱. 自己建了 stop_source 並把 token 傳給 jthread 的 lambda，
//         結果呼叫 jt.request_stop() 卻沒有任何反應，為什麼？
//     答：因為那是【兩塊不同的共享狀態】。jt.request_stop() 觸發的是
//         jthread【自己內建】的 stop_source；而你的 lambda 檢查的是
//         你手動捕獲進去的那個 token，它屬於你自己建的 source。
//         兩者毫無關聯，當然互不影響。
//     為什麼會錯：多數人以為 jthread 會「認得」lambda 參數列以外的 token，
//         但 jthread 只會在 callable 的【第一個參數】是 stop_token 時
//         注入自己的 token。手動捕獲的 token 對 jthread 而言是不透明的資料。
//         修法：要嘛統一用 jthread 內建的（讓 lambda 收第一個參數），
//         要嘛統一用自己的 source（那就別呼叫 jt.request_stop()）。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔從缺
//   本檔談的是取消機制的所有權模型（誰有權叫停、把手如何傳遞）。
//   LeetCode 並行題（1114/1115/1116/1117/1195）的執行緒都是跑完固定次數
//   就結束，題目框架也不提供取消入口，完全沒有這個維度。故從缺。

#include <atomic>
#include <chrono>
#include <iostream>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

// -----------------------------------------------------------------------------
// 【基本示範】從 jthread 取得 source 與 token
// -----------------------------------------------------------------------------
void demoGetSourceAndToken() {
    std::jthread jt([](std::stop_token stoken) {
        while (!stoken.stop_requested()) {
            std::cout << "Running..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
        std::cout << "工作執行緒結束" << std::endl;
    });

    // 取得 stop_source
    // ⚠️ 更正：get_stop_source() 是【回傳值】,不是回傳 reference,
    //    寫成 std::stop_source& 會編譯失敗（non-const lvalue ref 綁不到 prvalue）。
    //    直接用 auto 接就好——stop_source 本身就是共享狀態的把手,複製不影響語意。
    auto source = jt.get_stop_source();

    // 取得 stop_token（唯讀觀察端）
    std::stop_token token = jt.get_stop_token();

    std::cout << "  取得後 stop_possible = " << std::boolalpha
              << token.stop_possible() << std::endl;
    std::cout << "  取得後 stop_requested = " << token.stop_requested() << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 兩種方式都可以請求停止（操作的是同一塊共享狀態）
    // source.request_stop();
    jt.request_stop();

    std::cout << "  請求後 stop_requested = " << token.stop_requested()
              << "（同一塊共享狀態，透過 token 也看得到）" << std::endl;
}

// -----------------------------------------------------------------------------
// 【對照示範】把手大小（實作定義）
// -----------------------------------------------------------------------------
void demoHandleSizes() {
    std::cout << "  sizeof(std::stop_source) = " << sizeof(std::stop_source)
              << " bytes" << std::endl;
    std::cout << "  sizeof(std::stop_token)  = " << sizeof(std::stop_token)
              << " bytes" << std::endl;
    std::cout << "  （實作定義：libstdc++ 上就是一個指向共享狀態的指標）"
              << std::endl;

    std::stop_token empty;  // 預設建構：沒有關聯共享狀態
    std::cout << "  預設建構的 token: stop_possible = " << std::boolalpha
              << empty.stop_possible() << ", stop_requested = "
              << empty.stop_requested() << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】優雅關機：一個 stop_source 同時叫停整個 worker 池
//   情境：HTTP 服務收到 SIGTERM 後要「優雅關機」——停止接受新請求，
//         讓所有 worker 把手上的請求處理完再退出，而不是硬砍。
//   為何自己建 stop_source：需要【一個開關控制 N 條執行緒】。
//         jthread 內建的 token 是每條執行緒各一份，做不到 fan-out；
//         自建 source 把同一個 token 分發給所有 worker，一次 request_stop()
//         就能讓全體收到，這是實務上最常見的關機模式。
// -----------------------------------------------------------------------------
class WorkerPool {
public:
    explicit WorkerPool(int n) {
        for (int i = 0; i < n; ++i) {
            // 注意：這裡傳的是【我們自己的】 source 產生的 token，
            //       不是 jthread 自動注入的那一個。
            workers_.emplace_back([this, i, tok = shutdown_.get_token()]() {
                serve(i, tok);
            });
        }
    }

    void shutdown() {
        std::cout << "  [pool] 收到 SIGTERM，廣播停止請求給所有 worker" << std::endl;
        shutdown_.request_stop();   // ← 一次叫停全部
    }

    int totalHandled() const { return handled_.load(); }

private:
    void serve(int id, std::stop_token tok) {
        while (!tok.stop_requested()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(60));
            handled_.fetch_add(1, std::memory_order_relaxed);
        }
        std::cout << "  [worker " << id << "] 手上請求已處理完，安全退出"
                  << std::endl;
    }

    std::stop_source shutdown_;
    std::atomic<int> handled_{0};
    std::vector<std::jthread> workers_;   // ← 宣告在最後：解構時最先 join
};

int main() {
    std::cout << "=== 基本示範：取得 source 與 token ===" << std::endl;
    demoGetSourceAndToken();

    std::cout << "\n=== 把手大小與預設建構的 token ===" << std::endl;
    demoHandleSizes();

    std::cout << "\n=== 日常實務：一個 source 廣播關機給整個 worker 池 ===" << std::endl;
    {
        WorkerPool pool(3);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        pool.shutdown();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "  [pool] 關機期間共處理 " << pool.totalHandled()
                  << " 個請求（每次執行都不同）" << std::endl;
    }  // pool 解構 → workers_ 逐一 join
    std::cout << "  main: 所有 worker 已回收，程序可以安全退出" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra -pthread "課程 3.3：std jthread (C++20)4.cpp" -o jthread4
//   注意：本檔【必須】用 -std=c++20（std::jthread / std::stop_source / std::stop_token
//   為 C++20 新增，已用 -std=c++17 -pedantic-errors 實測確認編譯失敗）。

// 註:
//   ⚠️ "Running..." 的行數、已處理請求數、worker 退出順序都取決於執行緒排程，
//   【每次執行都不同】。sizeof 為實作定義數值。

//   [pool] 關機期間共處理 12 個請求（每次執行都不同）

// === 預期輸出 ===
// === 基本示範：取得 source 與 token ===
//   取得後 stop_possible = true
//   取得後 stop_requested = false
// Running...
// Running...
// Running...
// Running...
//   請求後 stop_requested = true（同一塊共享狀態，透過 token 也看得到）
// 工作執行緒結束
//
// === 把手大小與預設建構的 token ===
//   sizeof(std::stop_source) = 8 bytes
//   sizeof(std::stop_token)  = 8 bytes
//   （實作定義：libstdc++ 上就是一個指向共享狀態的指標）
//   預設建構的 token: stop_possible = false, stop_requested = false
//
// === 日常實務：一個 source 廣播關機給整個 worker 池 ===
//   [pool] 收到 SIGTERM，廣播停止請求給所有 worker
//   [worker 0] 手上請求已處理完，安全退出
//   [worker 1] 手上請求已處理完，安全退出
//   [worker 2] 手上請求已處理完，安全退出
//   main: 所有 worker 已回收，程序可以安全退出
