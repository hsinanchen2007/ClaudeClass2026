// =============================================================================
//  課程 2.4：join() 與 detach()4.cpp  —  joinable():它問的到底是什麼
// =============================================================================
//
// 【主題資訊 Information】
//   簽名      : bool joinable() const noexcept;
//   標準版本  : C++11
//   標頭檔    : <thread>
//   語意      : 這個 thread 物件目前是否「關聯著一條尚未被回收的執行緒」
//   noexcept  : 是 —— 它只讀取內部的執行緒 handle,不會失敗
//   複雜度    : O(1),實作上通常就是比較 handle 是否為預設值
//
// 【詳細解釋 Explanation】
//
// 【1. joinable() 問的是「所有權」,不是「還在不在跑」】
// 這是最重要、也最常被誤解的一點。
// joinable() == true 的意思是:**這個物件還握著一條執行緒的所有權,
// 你還欠它一次 join() 或 detach()。**
// 它完全不告訴你那條執行緒的執行狀態 —— 執行緒可能還在跑、
// 可能早就跑完了,只要沒有人 join 或 detach 過它,joinable() 就是 true。
//
// 本檔的輸出正好示範這件事:第二次查詢時執行緒早就印完 "Working" 了,
// 但只有在 join() 之後,joinable() 才變成 false。
// 讓它變 false 的是 join() 這個動作,不是「執行緒跑完了」。
//
// 【2. 什麼情況下 joinable() 是 false】
//   * 預設建構的 std::thread(沒有關聯任何執行緒)
//   * 已經 join() 過
//   * 已經 detach() 過
//   * 被 std::move 走了(所有權轉移給別人,自己變成空殼)
// 這四種情況在課程 2.5 會完整示範。共同點都是「你不再欠它什麼」。
//
// 【3. 為什麼這個判斷這麼重要】
// 因為 std::thread 的三個關鍵操作都以它為前置條件:
//   * join()   :joinable() 必須為 true,否則丟 std::system_error
//   * detach() :同上
//   * 解構子   :joinable() 必須為 false,否則呼叫 std::terminate()
// 換句話說,joinable() 是 std::thread 整個生命週期的核心不變量。
// 寫「安全的執行緒管理」時,幾乎所有防禦性檢查都圍繞著它。
//
// 【4. 為什麼直接印出來是 1 和 0】
// std::cout << t.joinable() 印出的是 1 或 0,不是 true / false ——
// 因為 bool 在沒有特別設定時,會以整數形式輸出。
// 想印出 true/false 要加 std::boolalpha:
//     std::cout << std::boolalpha << t.joinable();
// 這個格式旗標是「黏著」的:設定之後會一直影響同一個串流的後續輸出,
// 直到你用 std::noboolalpha 改回來。本檔兩種都示範了。
//
// 【概念補充 Concept Deep Dive】
//
// (A) joinable() 的實作有多輕
//   libstdc++ 的實作大致是:
//       bool joinable() const noexcept { return !(_M_id == id()); }
//   也就是比較內部的執行緒 id 是不是預設值(代表「沒有關聯」)。
//   沒有系統呼叫、沒有鎖 —— 所以它是 noexcept,也可以放心地頻繁呼叫。
//
// (B) 為什麼「執行緒跑完了」不會自動讓它變 false
//   因為執行緒結束後,它的返回狀態與資源仍需要有人回收
//   (POSIX 上就是 pthread_join 的職責)。在有人 join 之前,
//   那條執行緒處於「已結束但未回收」的狀態 —— 相當於行程世界裡的殭屍行程。
//   std::thread 物件仍然握著它,所以仍然 joinable()。
//
// (C) 這也解釋了「忘記 join 會 terminate」的合理性
//   若解構時 joinable() 仍為 true,代表有一條執行緒的資源沒有人負責回收。
//   標準不願意默默地幫你選擇(自動 join 會讓解構子可能阻塞很久,
//   自動 detach 會讓執行緒去存取正在被銷毀的資料),所以選擇當場中止,
//   讓錯誤明確暴露(見本課第 1 個範例檔的詳細討論)。
//
// 【注意事項 Pay Attention】
// 1. joinable() 回報的是所有權狀態,不是執行狀態。
//    執行緒跑完了但沒 join,它仍然是 true。
// 2. 想知道「執行緒做完了沒」,std::thread 沒有提供這個功能 ——
//    請用 std::future(async)、atomic 旗標或條件變數自己實作。
// 3. 直接輸出會印 1/0;要 true/false 請加 std::boolalpha,
//    且注意這個旗標會持續影響同一個串流。
// 4. 對 joinable() == false 的物件呼叫 join()/detach() 會丟 std::system_error。
// 5. 解構時 joinable() 必須是 false,否則 std::terminate()。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】joinable() 的真正語意
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 一條執行緒已經執行完畢了,對應的 std::thread 物件的
//        joinable() 會是什麼?
//     答：仍然是 true。joinable() 問的是「這個物件還握著執行緒的所有權嗎」,
//         而不是「執行緒還在跑嗎」。執行緒結束後資源仍待回收
//         (類似殭屍行程),要等 join() 或 detach() 之後,
//         joinable() 才會變成 false。
//     追問：那要怎麼知道執行緒做完了沒?
//         → std::thread 不提供這個能力。需要的話請用 std::async 取得
//           future 並用 wait_for(0s) 查詢,或自己用 atomic 旗標
//           / 條件變數回報進度。
//
// ⚠️ 陷阱. 「std::cout << t.joinable() 印出 1,那不就是有 bug 嗎?
//         它回傳的明明是 bool,應該印 true 才對。」
//     答：沒有 bug。bool 在 iostream 預設是以整數形式輸出的,
//         true 印成 1、false 印成 0。要印出文字得加 std::boolalpha。
//         而且要注意 boolalpha 是「黏著」的格式旗標 ——
//         設定之後會持續影響同一個串流的所有後續 bool 輸出,
//         直到用 std::noboolalpha 關掉。
//     為什麼會錯：把「型別是 bool」和「輸出成 true/false」混為一談。
//         iostream 的輸出格式由串流的格式旗標決定,不是由型別決定。
//         同樣的道理也出現在浮點數精度、進位制等地方 ——
//         這些旗標都是黏著的,在共用同一個串流時特別容易互相影響。
// ═══════════════════════════════════════════════════════════════════════════

#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

// 保護 std::cout,避免多執行緒輸出互相切開
std::mutex g_ioMutex;

void say(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_ioMutex);
    std::cout << msg << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】可重複使用的工作執行緒欄位:每次啟動前先確認前一個已結束
//   情境: 一個匯出功能的按鈕,使用者可能連按好幾次。物件裡有一個
//         std::thread 成員,每次啟動新任務前必須先確認前一條已經收乾淨,
//         否則對「還 joinable 的」thread 做 move-assign 會直接 terminate。
//   為什麼用本主題: 這正是 joinable() 在真實程式中最主要的用途 ——
//         維護 std::thread 生命週期的不變量。
//   ⚠️ 重要:對一個仍 joinable() 的 std::thread 做 move assignment,
//      標準規定會呼叫 std::terminate()(它會先「丟棄」原本持有的執行緒)。
//      所以底下 start() 裡的那次 join() 不是可選的禮貌,而是必要的。
//      (對照:C++20 的 std::jthread 的 move assignment 會先
//       request_stop() 並 join(),不會中止 —— 兩者規則不同,別混用記憶。)
// -----------------------------------------------------------------------------
class ExportTask {
    std::thread worker_;
    int         runCount_ = 0;

public:
    void start(const std::string& fileName) {
        // 關鍵:前一條若還握在手上,必須先收乾淨
        if (worker_.joinable()) {
            say("    (偵測到前一個匯出仍未回收 → 先 join)");
            worker_.join();
        }

        ++runCount_;
        int id = runCount_;
        worker_ = std::thread([fileName, id]() {     // 值捕獲,不借用外部變數
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            say("    [匯出 #" + std::to_string(id) + "] " + fileName + " 完成");
        });
    }

    ~ExportTask() {
        if (worker_.joinable()) {
            worker_.join();       // RAII:解構前一定收乾淨,避免 terminate
        }
    }

    ExportTask() = default;
    ExportTask(const ExportTask&) = delete;
    ExportTask& operator=(const ExportTask&) = delete;
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔從缺,理由如下
//   joinable() 處理的是 std::thread 物件的所有權狀態,屬於 C++ 執行緒
//   生命週期管理的細節。LeetCode 的並行題(1114 Print in Order、
//   1115 Print FooBar Alternately、1116 Print Zero Even Odd、
//   1117 Building H2O、1195 Fizz Buzz Multithreaded)由評測框架建立與回收
//   執行緒,參賽者只實作被呼叫的成員函式,根本接觸不到 thread 物件,
//   自然也用不到 joinable()。硬湊不如從缺,改以上面的實務情境示範。
// -----------------------------------------------------------------------------

int main() {
    std::cout << "=== 原始示範:join 前後的 joinable() ===" << std::endl;
    {
        std::thread t([]() {
            say("  Working");
        });

        std::cout << "  joinable: " << t.joinable() << "  (印成 1,因為 bool 預設以整數輸出)" << std::endl;

        t.join();

        std::cout << "  joinable: " << t.joinable() << "  (現在是 0)" << std::endl;
    }

    std::cout << "\n=== 加上 std::boolalpha 就印成 true/false ===" << std::endl;
    {
        std::thread t([]() {});
        std::cout << std::boolalpha;
        std::cout << "  join 前: " << t.joinable() << std::endl;
        t.join();
        std::cout << "  join 後: " << t.joinable() << std::endl;
        std::cout << std::noboolalpha;   // 記得關掉,否則會影響後續輸出
    }

    std::cout << "\n=== 關鍵觀念:執行緒「跑完」不會讓 joinable 變 false ===" << std::endl;
    {
        std::thread t([]() {
            say("  (執行緒瞬間就做完事了)");
        });

        // 給它充足的時間確定已經執行完畢
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::cout << "  執行緒早已跑完,但 joinable() 仍是 " << std::boolalpha
                  << t.joinable() << std::noboolalpha << std::endl;
        std::cout << "  ↑ 因為還沒有人回收它 —— joinable 問的是所有權,"
                     "不是執行狀態" << std::endl;

        t.join();
        std::cout << "  join() 之後才變成 " << std::boolalpha << t.joinable()
                  << std::noboolalpha << "  (是 join 這個動作讓它改變的)"
                  << std::endl;
    }

    std::cout << "\n=== 實務:可重複啟動的匯出任務 ===" << std::endl;
    {
        ExportTask task;
        task.start("report-2024-01.csv");
        task.start("report-2024-02.csv");   // 會先 join 前一個
        task.start("report-2024-03.csv");
        // 解構時自動 join 最後一個
    }
    std::cout << "  ↑ 每次啟動前檢查 joinable(),"
                 "避免對仍持有執行緒的 thread 做 move-assign 而 terminate" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.4：join() 與 detach()4.cpp" -o joinable_demo

// 注意:以下為某一次實際執行的結果,每次執行都相同 ——
//       所有 joinable() 的查詢都發生在明確的同步點前後,
//       實務段的三次匯出也因為「啟動前先 join 前一個」而被強制序列化。

// === 預期輸出 ===
// === 原始示範:join 前後的 joinable() ===
//   joinable: 1  (印成 1,因為 bool 預設以整數輸出)
//   Working
//   joinable: 0  (現在是 0)
//
// === 加上 std::boolalpha 就印成 true/false ===
//   join 前: true
//   join 後: false
//
// === 關鍵觀念:執行緒「跑完」不會讓 joinable 變 false ===
//   (執行緒瞬間就做完事了)
//   執行緒早已跑完,但 joinable() 仍是 true
//   ↑ 因為還沒有人回收它 —— joinable 問的是所有權,不是執行狀態
//   join() 之後才變成 false  (是 join 這個動作讓它改變的)
//
// === 實務:可重複啟動的匯出任務 ===
//     (偵測到前一個匯出仍未回收 → 先 join)
//     [匯出 #1] report-2024-01.csv 完成
//     (偵測到前一個匯出仍未回收 → 先 join)
//     [匯出 #2] report-2024-02.csv 完成
//     [匯出 #3] report-2024-03.csv 完成
//   ↑ 每次啟動前檢查 joinable(),避免對仍持有執行緒的 thread 做 move-assign 而 terminate
