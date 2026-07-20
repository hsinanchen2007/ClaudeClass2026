// =============================================================================
//  課程 2.6：執行緒識別與資訊5.cpp  —  std::this_thread 命名空間總覽
// =============================================================================
//
// 【主題資訊 Information】
//   namespace std::this_thread {                                    // C++11
//       thread::id get_id() noexcept;
//       void       yield() noexcept;
//       template<class Rep, class Period>
//       void       sleep_for  (const chrono::duration<Rep, Period>& rel_time);
//       template<class Clock, class Duration>
//       void       sleep_until(const chrono::time_point<Clock, Duration>& abs_time);
//   }
//   標頭檔：<thread>（sleep_for / sleep_until 的時間型別來自 <chrono>）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼是「命名空間」而不是「類別的靜態成員」】
//   這四個函式作用的對象是「呼叫它的那條執行緒」，不需要任何 thread 物件。
//   主執行緒沒有對應的 std::thread 物件（沒人 new 出它），
//   但它一樣要能問「我是誰」、一樣要能睡覺——
//   如果做成 std::thread 的靜態成員，語意會很怪（哪一條 thread？）。
//   放進 this_thread 命名空間，語意就變成「對目前這條執行緒動作」，非常清楚。
//
// 【2. sleep_for vs sleep_until：相對 vs 絕對】
//     sleep_for(100ms)          →「從現在起再睡 100 毫秒」
//     sleep_until(t)            →「一直睡到時間點 t」
//   看起來 sleep_until(now() + 100ms) 等於 sleep_for(100ms)，但兩者在
//   【週期性任務】上有本質差異：
//       // 寫法 A：每輪都會累積誤差（工作耗時被加進週期）
//       while (true) { doWork(); sleep_for(1s); }        // 實際週期 = 1s + 工作時間
//       // 寫法 B：週期不漂移
//       auto next = steady_clock::now();
//       while (true) { doWork(); next += 1s; sleep_until(next); }
//   寫法 B 才是心跳、取樣、定時排程該用的形式。
//
// 【3. 為什麼 sleep_until 一定要配 steady_clock】
//   system_clock 是「牆上時鐘」，會被 NTP 校時、使用者改時間、
//   甚至夏令時間調整而【往前或往後跳】。
//   拿它當 sleep_until 的基準，遇到時鐘被往前調就會多睡幾小時。
//   steady_clock 保證單調遞增、不被調整，是所有「量時間長度／等一段時間」
//   的正確選擇。system_clock 只該用在「顯示現在幾點」與「與外界交換時間戳」。
//
// 【4. sleep 的精度：只保證「至少」，不保證「剛好」】
//   標準的措辭是「阻塞至少 rel_time」。實際喚醒會晚一點，因為：
//     * OS 排程器的時間片粒度（Linux 上與 CONFIG_HZ / 高精度計時器有關）
//     * 喚醒後還要排隊等 CPU
//   所以 sleep_for(100ms) 實測通常是 100.0x ~ 100.x ms。
//   絕不能拿 sleep 來做「精準節拍」，也不能假設它會準時到微秒。
//
// 【5. yield()：一個「提示」，不是保證】
//   yield() 告訴排程器「我現在願意讓出 CPU，去跑別人吧」。
//   標準說它是 opportunity to reschedule——【提示】而已。
//   實作可以完全忽略它；即使讓出，也可能立刻又把 CPU 排回給你
//   （如果沒有其他 runnable 的執行緒）。
//   它的正當用途是【短暫】的自旋等待：預期馬上就會等到，
//   睡下去（sleep）的 context switch 成本反而更貴。
//   如果要等的時間可能很長，正確工具是 condition_variable，不是 yield 迴圈。
//
// 【概念補充 Concept Deep Dive】
//   * sleep_for 的實作：libstdc++ 在 Linux 上走 nanosleep()/clock_nanosleep()。
//     被 signal 打斷時標準要求它繼續睡滿，所以實作內部會處理 EINTR 重試。
//   * sleep_for 傳負值 duration：標準規定等同於不睡（立即返回），不是 UB。
//   * yield() 在 Linux 上是 sched_yield()。要注意在【只有一條 runnable
//     執行緒】時它幾乎是 no-op，忙等迴圈仍會吃滿一顆 CPU。
//     x86 上更省電的做法是 _mm_pause()（PAUSE 指令），C++ 標準沒有對應 API。
//   * this_thread::get_id() 永遠回傳一個「代表某條實際執行緒」的 id，
//     不可能等於預設建構的 std::thread::id{}。
//
// 【注意事項 Pay Attention】
//   1. sleep_* 只保證【至少】睡那麼久，會超過，不會不足。
//   2. sleep_until 請用 steady_clock，別用 system_clock（會被校時影響）。
//   3. yield() 是提示，實作可忽略；別用它做長時間等待。
//   4. 這四個函式都不接受「執行緒參數」——它們只作用於呼叫者自己。
//   5. 睡眠期間【仍持有已取得的鎖】。在持鎖狀態下 sleep 是效能災難，
//      也是死結的常見來源。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::this_thread
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. sleep_for(1s) 和 sleep_until(now() + 1s) 有什麼差別？
//     答：單次呼叫幾乎等價，差別在【週期性任務】。
//         迴圈中用 sleep_for，每輪的工作耗時都會被加進週期，誤差不斷累積；
//         用 sleep_until 並讓目標時間點每輪固定 += 週期，就不會漂移。
//     追問：sleep_until 該用哪個 clock？→ steady_clock。
//           system_clock 會被 NTP／手動改時間往前往後跳，
//           拿它當基準可能一睡不醒（或完全不睡）。
//
// 🔥 Q2. sleep_for(100ms) 保證剛好睡 100 毫秒嗎？
//     答：不保證。標準只保證「至少」100 毫秒。
//         實際受排程器粒度與喚醒後的排隊影響，一定會多一點。
//         需要精準節拍的場景要用時間點推進（sleep_until）加上補償，
//         而不是期待 sleep 準時。
//     追問：那它會不會提早醒？→ 標準規定不會因為 signal 而提早返回，
//           實作必須繼續睡滿剩餘時間。
//
// ⚠️ 陷阱. 「while (!ready) std::this_thread::yield();」比
//        「while (!ready) {}」省 CPU，所以它是省電的等待方式，對嗎？
//     答：不對。yield() 只是提示排程器可以換人跑，這條執行緒【仍是 runnable】，
//         排程器隨時可能把 CPU 立刻排回給它。在沒有其他待跑執行緒時，
//         這個迴圈照樣把一顆核心吃到 100%。
//     為什麼會錯：多數人把 yield() 想成「睡一下」。它不是睡眠，
//         不會讓執行緒進入 blocked 狀態，也不會被排程器降低優先度。
//         真正讓出 CPU 的是「阻塞」——等待 condition_variable、
//         等 I/O、或 sleep_for。等待時間可能較長時，正解是 condition_variable。
// ═══════════════════════════════════════════════════════════════════════════

#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <thread>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1114. Print in Order
//   題目：三條執行緒分別呼叫 first()／second()／third()，呼叫順序任意，
//         但必須保證輸出永遠是 "firstsecondthird"。
//   為什麼用到本主題：這題的最小解法就是「自旋 + yield」——
//         用一個 atomic 記錄目前該輪到誰，還沒輪到的執行緒
//         用 std::this_thread::yield() 讓出 CPU 再檢查一次。
//         正好示範 yield() 的正當用途：預期【很快】就會等到的短暫等待。
//   ⚠️ 生產環境提醒：等待時間可能較長時，應改用 condition_variable；
//      自旋只在「幾乎立刻就會滿足」時才划算。
// -----------------------------------------------------------------------------
class Foo {
private:
    std::atomic<int> stage{1};   // 1 → 該跑 first，2 → second，3 → third

public:
    void first(const std::function<void()>& printFirst) {
        printFirst();
        stage.store(2, std::memory_order_release);
    }

    void second(const std::function<void()>& printSecond) {
        while (stage.load(std::memory_order_acquire) != 2) {
            std::this_thread::yield();   // 還沒輪到我，讓出 CPU 再看一次
        }
        printSecond();
        stage.store(3, std::memory_order_release);
    }

    void third(const std::function<void()>& printThird) {
        while (stage.load(std::memory_order_acquire) != 3) {
            std::this_thread::yield();
        }
        printThird();
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】不會漂移的心跳排程器（監控系統送 heartbeat）
//   情境：監控 agent 每 50ms 要送一次心跳。若寫成
//         while (...) { sendHeartbeat(); sleep_for(50ms); }
//         則實際週期是 50ms + 送出耗時，跑久了會愈拖愈慢，
//         後端會誤判 agent 卡住。
//   正解：用 steady_clock 的絕對時間點推進，每輪 nextTick += 週期，
//         再 sleep_until(nextTick)——工作耗時被吸收掉，長期平均週期不漂移。
// -----------------------------------------------------------------------------
int runHeartbeat(int ticks, std::chrono::milliseconds period) {
    int sent = 0;
    auto nextTick = std::chrono::steady_clock::now();

    for (int i = 0; i < ticks; ++i) {
        // ── 模擬「送出心跳」這件事本身要花的時間 ──
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        ++sent;

        nextTick += period;                        // 目標時間點固定往前推
        std::this_thread::sleep_until(nextTick);   // 睡到那個時間點，不是「再睡一段」
    }
    return sent;
}

int main() {
    std::cout << "=== 1. get_id(): 取得當前執行緒 ID ===" << std::endl;
    // 實際數值每次執行都不同、格式也是實作定義的，故只驗證可驗證的性質
    const std::thread::id myId = std::this_thread::get_id();
    std::cout << "this_thread::get_id() 不等於預設建構的 id: "
              << (myId != std::thread::id{} ? "是" : "否") << std::endl;

    std::cout << "\n=== 2. sleep_for(): 相對時間休眠 ===" << std::endl;
    {
        const auto t0 = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::steady_clock::now() - t0);
        // 標準只保證「至少」，所以只能斷言 >=，不能斷言等於
        std::cout << "要求睡 100ms，實際經過 >= 100ms: "
                  << (elapsed.count() >= 100 ? "是" : "否") << std::endl;
    }

    std::cout << "\n=== 3. sleep_until(): 絕對時間點休眠 ===" << std::endl;
    {
        const auto wakeTime = std::chrono::steady_clock::now()
                            + std::chrono::milliseconds(100);
        std::this_thread::sleep_until(wakeTime);
        std::cout << "醒來時間 >= 指定的時間點: "
                  << (std::chrono::steady_clock::now() >= wakeTime ? "是" : "否") << std::endl;

        // 已經過去的時間點：立即返回，不會倒著睡
        const auto past = std::chrono::steady_clock::now() - std::chrono::seconds(1);
        const auto t0   = std::chrono::steady_clock::now();
        std::this_thread::sleep_until(past);
        const auto spent = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - t0);
        std::cout << "對已過去的時間點 sleep_until，幾乎立即返回 (< 50ms): "
                  << (spent.count() < 50 ? "是" : "否") << std::endl;
    }

    std::cout << "\n=== 4. yield(): 讓出 CPU 時間給其他執行緒 ===" << std::endl;
    std::this_thread::yield();   // 只是提示排程器；沒有回傳值、也無從觀測是否生效
    std::cout << "yield() 已呼叫（它是提示，實作可忽略，無可觀測的回傳值）" << std::endl;

    std::cout << "\n=== LeetCode 1114. Print in Order ===" << std::endl;
    {
        Foo foo;
        // 刻意用「相反」的順序啟動執行緒，證明輸出順序由同步機制決定
        std::thread t3([&foo]() { foo.third ([]() { std::cout << "third";  }); });
        std::thread t2([&foo]() { foo.second([]() { std::cout << "second"; }); });
        std::thread t1([&foo]() { foo.first ([]() { std::cout << "first";  }); });
        t1.join();
        t2.join();
        t3.join();
        std::cout << std::endl;
    }

    std::cout << "\n=== 日常實務: 不漂移的心跳排程器 ===" << std::endl;
    {
        const auto period = std::chrono::milliseconds(50);
        const int  ticks  = 4;

        const auto t0   = std::chrono::steady_clock::now();
        const int  sent  = runHeartbeat(ticks, period);
        const auto total = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - t0);

        std::cout << "送出心跳次數: " << sent << std::endl;
        // 每輪工作 5ms + 睡到 50ms 的邊界 → 總時間貼齊 4 * 50 = 200ms，
        // 而不是 4 * (50 + 5) = 220ms。允許一點排程誤差。
        std::cout << "總耗時貼近 " << (ticks * period.count()) << "ms（未累積工作耗時）: "
                  << (total.count() >= 200 && total.count() < 220 ? "是" : "否") << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 2.6：執行緒識別與資訊5.cpp' -o this_thread_demo

// -----------------------------------------------------------------------------
// 【輸出但書】
//   1. 本檔【刻意不印出】任何實際的執行緒 id 與毫秒數——
//      id 每次執行都不同、格式是實作定義的；耗時則受排程影響。
//      改為印出可穩定驗證的【性質】（>= 門檻、是否相異）。
//   2. 「總耗時貼近 200ms」這行在系統極度忙碌時理論上可能變成「否」，
//      因為 sleep 只保證下限、排程延遲無上限。本機連續 5 次執行皆為「是」。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// === 1. get_id(): 取得當前執行緒 ID ===
// this_thread::get_id() 不等於預設建構的 id: 是
//
// === 2. sleep_for(): 相對時間休眠 ===
// 要求睡 100ms，實際經過 >= 100ms: 是
//
// === 3. sleep_until(): 絕對時間點休眠 ===
// 醒來時間 >= 指定的時間點: 是
// 對已過去的時間點 sleep_until，幾乎立即返回 (< 50ms): 是
//
// === 4. yield(): 讓出 CPU 時間給其他執行緒 ===
// yield() 已呼叫（它是提示，實作可忽略，無可觀測的回傳值）
//
// === LeetCode 1114. Print in Order ===
// firstsecondthird
//
// === 日常實務: 不漂移的心跳排程器 ===
// 送出心跳次數: 4
// 總耗時貼近 200ms（未累積工作耗時）: 是
