// =============================================================================
//  課程 2.4：join() 與 detach()1.cpp  —  join():等待、同步、與所有權回收
// =============================================================================
//
// 【主題資訊 Information】
//   簽名      : void join();
//   標準版本  : C++11
//   標頭檔    : <thread>
//   前置條件  : joinable() 必須為 true,否則丟出 std::system_error
//   效果      : 阻塞呼叫端,直到目標執行緒結束;之後該 thread 物件
//               不再 joinable()
//   同步保證  : 目標執行緒中的所有操作 happens-before join() 的返回
//
// 【詳細解釋 Explanation】
//
// 【1. join() 做的其實是三件事,不只是「等待」】
// 大多數人只記得第一件,但另外兩件在實務上同樣關鍵:
//
//   (a) 阻塞等待 —— 呼叫端停在這裡,直到目標執行緒的函式返回。
//
//   (b) 建立同步關係 —— 這是最容易被忽略的一件。標準保證目標執行緒中
//       的所有操作 happens-before join() 的返回。也就是說,
//       join() 之後,你「保證」看得到那條執行緒寫入的所有資料。
//       少了這個保證,對方寫的值可能還留在該核心的 store buffer 或
//       暫存器裡,你根本讀不到 —— 這在弱記憶體序的架構(如 ARM)上
//       是真實會發生的。
//
//   (c) 回收資源 —— 執行緒結束後,它的堆疊與核心層的結構要有人回收。
//       join() 負責這件事。這也是為什麼「執行緒已經跑完」但沒 join,
//       物件依然是 joinable() 的:資源還沒被回收。
//
// 【2. 本檔輸出順序為什麼是完全確定的】
// 這個範例的輸出永遠是 1、2、3、4,沒有任何隨機性。原因正是 join():
//     std::cout << "2. 呼叫 join()..." ;   // 一定在 join 之前
//     t.join();                             // 阻塞 2 秒
//     std::cout << "4. join() 返回..." ;    // 一定在執行緒結束之後
// 執行緒印的 "3." 被夾在 join() 的等待期間,所以必然落在 2 和 4 之間。
// 這示範了一個重要觀念:**多執行緒程式的輸出不必然混亂 ——
// 只要用同步機制建立了順序,結果就是確定的。**
//
// 【3. join() 是一次性的】
// 呼叫過一次之後,thread 物件就不再 joinable();再呼叫一次會丟出
// std::system_error(見本課第 6 個範例檔,那是會讓程式中止的示範)。
// 這個設計反映的是「所有權」概念:一個 std::thread 物件代表對某條
// 執行緒的獨佔所有權,join() 就是把它交還回去 —— 交還過的東西
// 不能再交還一次。
//
// 【4. join() 的成本與風險】
// join() 會阻塞,而且沒有逾時版本 —— 標準沒有提供 try_join_for()。
// 若目標執行緒因為某些原因永遠不結束(死結、無窮迴圈、等待永遠不來的
// 網路封包),呼叫 join() 的執行緒就永遠卡住。
// 需要逾時控制時,實務做法是改用 std::async + std::future::wait_for(),
// 或自己用條件變數 + 旗標實作可中斷的等待。
//
// 【概念補充 Concept Deep Dive】
//
// (A) join() 底層做了什麼
//   在 Linux 上 libstdc++ 的 join() 基本上就是 pthread_join()。
//   它讓呼叫端執行緒進入睡眠(不是忙碌等待,不燒 CPU),
//   由核心在目標執行緒結束時將它喚醒,並回收目標的資源。
//
// (B) 為什麼「等待」順便就提供了記憶體同步
//   要讓 A 執行緒看到 B 執行緒寫的東西,需要一個「同步點」讓兩者的
//   記憶體視圖對齊。join() 天然就是這樣一個點:B 結束、A 才繼續,
//   兩者之間有明確的先後。標準把這件事寫成正式保證,
//   於是「join 之後讀結果」成為最簡單、最不容易寫錯的跨執行緒溝通方式。
//
// (C) 為什麼沒有 join_for(timeout)
//   委員會的立場是:如果你需要「等一下下,不行就算了」,那你要的其實
//   不是執行緒的生命週期管理,而是「任務的結果」——那正是
//   std::future 的職責。std::thread 刻意保持成低階、單純的所有權 handle。
//
// 【注意事項 Pay Attention】
// 1. join() 只能呼叫一次;第二次會丟 std::system_error。不確定狀態時
//    先用 joinable() 檢查(見課程 2.5)。
// 2. join() 沒有逾時版本,目標執行緒不結束就會永遠卡住。
//    需要逾時請改用 std::async + future.wait_for()。
// 3. 在持有 mutex 的情況下呼叫 join(),而目標執行緒又要搶那把鎖 → 死結。
// 4. join() 提供的 happens-before 保證,是「join 後讀結果」正確的唯一理由;
//    不要以為「它跑完了所以我看得到」。
// 5. 每個 thread 物件解構前都必須 join() 或 detach(),否則 std::terminate()。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】join() 的語意
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. join() 除了「等執行緒結束」還做了什麼?
//     答：還有兩件重要的事。第一是建立同步關係:標準保證目標執行緒中
//         的所有操作 happens-before join() 的返回,所以 join 之後你
//         保證看得到它寫的所有資料。第二是回收資源:執行緒的堆疊與
//         核心結構由 join() 負責回收,這也是為什麼「已經跑完但沒 join」
//         的 thread 物件仍然是 joinable()。
//     追問：如果沒有那個 happens-before 保證會怎樣?
//         → 對方寫的值可能還在該核心的 store buffer 或暫存器裡,
//           你讀到的是舊值。在 x86 上不容易觀察到,但在 ARM 這類
//           弱記憶體序架構上是真實會發生的錯誤。
//
// 🔥 Q2. join() 有逾時版本嗎?如果目標執行緒卡住了怎麼辦?
//     答：沒有。標準沒有提供 try_join_for(),join() 會無限期阻塞。
//         若目標執行緒死結或無窮迴圈,呼叫 join() 的執行緒就永遠卡住。
//         需要逾時控制時,實務上改用 std::async + std::future::wait_for(),
//         或自己用條件變數加旗標實作可中斷的等待。
//     追問：委員會為什麼不加?
//         → 因為「等一下下,不行就算了」要的其實是「任務的結果」,
//           那是 std::future 的職責。std::thread 刻意維持成低階、
//           單純的執行緒所有權 handle。
//
// ⚠️ 陷阱. 「多執行緒程式的輸出順序本來就不可預測,所以這個範例
//         印出 1234 只是剛好而已。」哪裡錯了?
//     答：這個範例的輸出是「保證」的,不是碰巧。"2." 印在 join() 之前、
//         "4." 印在 join() 返回之後,而執行緒的 "3." 必定發生在
//         join() 返回之前(否則 join 就還沒返回)。順序被 join()
//         的語意鎖死了,跑一萬次都一樣。
//     為什麼會錯：把「沒有同步時順序不確定」過度推廣成「多執行緒一律不確定」。
//         實際上正好相反:寫並行程式的工作,就是用同步機制把
//         「需要確定」的部分確定下來,只讓「不需要確定」的部分自由。
//         如果一支多執行緒程式的輸出完全不可預測,那通常代表它缺少同步,
//         而不是它「本來就該這樣」。
// ═══════════════════════════════════════════════════════════════════════════

#include <chrono>
#include <iostream>
#include <mutex>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

// 保護 std::cout,避免多執行緒輸出互相切開
std::mutex g_ioMutex;

void say(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_ioMutex);
    std::cout << msg << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】分批平行處理後彙總 —— join() 當作最單純的屏障(barrier)
//   情境: 影像處理管線要把一張大圖切成數塊,各執行緒獨立處理,
//         全部完成後才能合成輸出。「全部完成後」這個要求,
//         用 join() 就能達成,不需要任何進階同步工具。
//   為什麼用本主題: 這是 join() 最常見也最有價值的用法 ——
//         它同時給了你「等待」與「保證看得到結果」兩件事,
//         所以彙總階段可以直接讀取各執行緒寫入的資料,完全不需要鎖。
// -----------------------------------------------------------------------------
struct TileResult {
    int  tileId    = 0;
    long brightness = 0;
};

void processTile(int tileId, const std::vector<int>& pixels, TileResult* out) {
    long sum = 0;
    for (int p : pixels) sum += p;
    out->tileId     = tileId;
    out->brightness = sum / static_cast<long>(pixels.size());
    say("    [tile " + std::to_string(tileId) + "] 處理完成");
}

int main() {
    std::cout << "=== 原始示範:join() 的阻塞與順序保證 ===" << std::endl;
    std::cout << "1. 建立執行緒" << std::endl;

    std::thread t([]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "3. 執行緒完成" << std::endl;
    });

    std::cout << "2. 呼叫 join(),開始等待..." << std::endl;
    t.join();  // 在這裡阻塞 2 秒

    std::cout << "4. join() 返回,繼續執行" << std::endl;
    std::cout << "  ↑ 1→2→3→4 的順序是 join() 保證的,跑一萬次都一樣"
              << std::endl;

    std::cout << "\n=== join() 之後 joinable() 變成 false ===" << std::endl;
    std::cout << "  join 後 t.joinable() = " << std::boolalpha << t.joinable()
              << "  (所有權已交還,不能也不該再 join 一次)" << std::endl;

    std::cout << "\n=== 實務:join() 當屏障,全部完成才彙總 ===" << std::endl;
    {
        // 準備 4 塊「影像區塊」
        std::vector<std::vector<int>> tiles = {
            {10, 20, 30, 40},
            {50, 60, 70, 80},
            {90, 100, 110, 120},
            {130, 140, 150, 160},
        };

        std::vector<TileResult> results(tiles.size());
        std::vector<std::thread> pool;

        for (std::size_t i = 0; i < tiles.size(); ++i) {
            pool.emplace_back(processTile, static_cast<int>(i),
                              std::cref(tiles[i]), &results[i]);
        }

        // 屏障:等全部做完。join 同時保證我們看得到它們寫的結果
        for (std::thread& th : pool) th.join();

        std::cout << "  ── 全部完成,開始彙總(此處免鎖也安全) ──" << std::endl;
        long total = 0;
        for (const TileResult& r : results) {
            std::cout << "    tile " << r.tileId << " 平均亮度 = "
                      << r.brightness << std::endl;
            total += r.brightness;
        }
        std::cout << "  整體平均亮度 = "
                  << total / static_cast<long>(results.size()) << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.4：join() 與 detach()1.cpp" -o join_demo
// (執行需約 2 秒 —— 那正是 join() 在等待背景執行緒睡完 2 秒)

// 注意:以下為某一次實際執行的結果。
//   * 實務段四行 [tile N] 處理完成 的先後順序每次執行都可能不同
//     (它們是真正並行的)。
//   * 但第一段的 1→2→3→4、以及彙總段的所有數字,每次執行都完全相同 ——
//     因為那些順序與可見性都是 join() 明確保證的。

// === 預期輸出 ===
// === 原始示範:join() 的阻塞與順序保證 ===
// 1. 建立執行緒
// 2. 呼叫 join(),開始等待...
// 3. 執行緒完成
// 4. join() 返回,繼續執行
//   ↑ 1→2→3→4 的順序是 join() 保證的,跑一萬次都一樣
//
// === join() 之後 joinable() 變成 false ===
//   join 後 t.joinable() = false  (所有權已交還,不能也不該再 join 一次)
//
// === 實務:join() 當屏障,全部完成才彙總 ===
//     [tile 0] 處理完成
//     [tile 2] 處理完成
//     [tile 1] 處理完成
//     [tile 3] 處理完成
//   ── 全部完成,開始彙總(此處免鎖也安全) ──
//     tile 0 平均亮度 = 25
//     tile 1 平均亮度 = 65
//     tile 2 平均亮度 = 105
//     tile 3 平均亮度 = 145
//   整體平均亮度 = 85
