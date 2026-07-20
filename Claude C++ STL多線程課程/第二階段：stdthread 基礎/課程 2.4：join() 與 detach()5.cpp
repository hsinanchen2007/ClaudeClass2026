// =============================================================================
//  課程 2.4：join() 與 detach()5.cpp  —  防禦性的 joinable() 檢查
// =============================================================================
//
// 【主題資訊 Information】
//   慣用法    : if (t.joinable()) { t.join(); }
//   標準版本  : C++11
//   標頭檔    : <thread>
//   目的      : 避免對 non-joinable 的 thread 呼叫 join()/detach()
//               而丟出 std::system_error
//   重要前提  : 這個檢查只在「單一執行緒管理該 thread 物件」時才可靠
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼需要這個檢查】
// join() 與 detach() 都要求 joinable() 為 true,否則丟出 std::system_error
// (錯誤碼 invalid_argument)。在執行緒函式裡沒接住的例外會導致
// std::terminate() —— 也就是程式當場中止(見本課第 6 個範例檔)。
// 因此在「不確定目前狀態」的地方,先問一句 joinable() 是標準做法。
//
// 【2. 什麼時候「不確定」,什麼時候其實很確定】
// 這個判斷值得說清楚,因為到處亂加檢查反而會掩蓋設計問題:
//
//   不需要檢查(狀態確定):
//     std::thread t(work);
//     t.join();               // 剛建立,必定 joinable
//
//   需要檢查(狀態不確定):
//     * 條件式建立:if (cond) t = std::thread(work);  之後不確定有沒有建
//     * 可能被 move 走:所有權可能已經轉移出去
//     * 迴圈中重複使用同一個 thread 變數
//     * 類別的解構子:不知道使用者之前對它做過什麼
//
// 本檔第一段的示範屬於「剛建立,必定 joinable」——那個 if 其實可以省略。
// 保留它是為了示範慣用法,以及示範第二次呼叫時它如何保護你。
//
// 【3. 第二次呼叫為什麼是安全的】
//     if (t.joinable()) { t.join(); }    // 第一次:進入,join
//     if (t.joinable()) { t.join(); }    // 第二次:joinable 已是 false,跳過
// 若沒有這個檢查,第二次 join() 會丟 std::system_error 而使程式中止。
// 這個「重複呼叫也安全」的性質,正是把它包進 RAII 類別的基礎 ——
// 解構子可以無腦呼叫,不必擔心使用者是不是已經自己 join 過了。
//
// 【4. ⚠️ 這個慣用法的重要限制:它不是執行緒安全的】
// 這是本檔最需要強調的一點。
//     if (t.joinable()) {    // ← 檢查
//         t.join();          // ← 使用
//     }
// 如果有「兩條執行緒」同時對同一個 t 做這件事,就是典型的 TOCTOU
// (Time-Of-Check to Time-Of-Use)競態:兩條都通過檢查,
// 然後兩條都去 join —— 其中一條會丟 std::system_error。
// 更糟的是,std::thread 的成員函式本身並沒有承諾可以被並行呼叫。
//
// 正確的心法是:**一個 std::thread 物件應該只由一個擁有者管理。**
// 需要多方協調時,要在更上層用 mutex 保護整個「檢查 + 操作」,
// 或改用 std::future / 執行緒池這類本來就設計成可共享的抽象。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼標準選擇「丟例外」而不是「靜默忽略」
//   對 non-joinable 的 thread 呼叫 join(),幾乎一定代表程式的邏輯有誤 ——
//   你以為手上有一條執行緒,但其實沒有。靜默忽略會讓這個誤解一直存在
//   (例如你以為已經等到工作完成,其實根本沒等)。丟例外能讓錯誤及早浮現。
//
// (B) 例外若沒接住,為什麼是 terminate 而不是往上傳
//   例外無法跨越執行緒邊界傳播。執行緒的進入點函式沒有「上一層」可以
//   接住例外,標準只好規定:例外逸出執行緒函式就呼叫 std::terminate()。
//   想把例外帶回主執行緒,要用 std::promise/std::future 或 std::exception_ptr。
//
// (C) C++20 的 std::jthread 讓大部分檢查變得不必要
//   jthread 的解構子會自動 request_stop() 並 join(),
//   所以「忘記 join」不再是錯誤,RAII 守衛類別也不必自己寫。
//   若專案能用 C++20,這是預設選擇。
//   ⚠️ 但要注意兩者的 move assignment 規則不同:
//      對仍 joinable 的 std::thread 做 move-assign 會 terminate;
//      而 std::jthread 會先 request_stop() + join(),然後正常存活。
//
// 【注意事項 Pay Attention】
// 1. 「檢查後再操作」只在單一擁有者的前提下可靠;多執行緒共用同一個
//    thread 物件時是 TOCTOU 競態。
// 2. 剛建立的 thread 必定 joinable,那裡的檢查是多餘的 ——
//    到處亂加檢查會掩蓋「你其實不清楚自己的所有權設計」這個問題。
// 3. 對 non-joinable 呼叫 join()/detach() 丟 std::system_error;
//    在執行緒函式中沒接住會導致 std::terminate()。
// 4. RAII 守衛類別的解構子應該用這個慣用法,才能容忍使用者已手動 join。
// 5. C++20 的 std::jthread 讓多數情況不再需要手寫這些檢查。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】joinable() 防禦性檢查
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. if (t.joinable()) t.join(); 這個慣用法解決了什麼問題?
//        什麼時候其實不需要它?
//     答：它避免對 non-joinable 的 thread 呼叫 join() 而丟出
//         std::system_error(在執行緒函式中沒接住就會 terminate)。
//         但剛建立的 thread 必定 joinable,那裡的檢查是多餘的。
//         真正需要它的是狀態不確定的場合:條件式建立、可能被 move 走、
//         迴圈中重複使用,以及類別的解構子。
//     追問：為什麼 RAII 守衛的解構子一定要加這個檢查?
//         → 因為使用者可能已經自己手動 join 過了。加了檢查,
//           解構子才能無腦呼叫而不會出錯 —— 這是它能被安全重複呼叫的基礎。
//
// ⚠️ 陷阱. 「我在每個 join() 前面都加上 if (t.joinable()),
//         這樣就是執行緒安全的了。」哪裡錯了?
//     答：完全不是。這個寫法只在「單一擁有者」的前提下有效。
//         若兩條執行緒同時對同一個 t 執行這段程式,兩條都可能通過檢查、
//         然後都去 join —— 這是典型的 TOCTOU 競態,其中一條會丟
//         std::system_error。而且 std::thread 的成員函式本來就沒有
//         承諾可以被並行呼叫。
//     為什麼會錯：把「加了檢查」等同於「安全」。檢查與使用之間永遠存在
//         時間差,只要中間可能被別人插隊,檢查就形同虛設。
//         正確的原則是所有權:一個 std::thread 物件只該由一個擁有者管理;
//         真的需要多方協調,就得用 mutex 保護整段「檢查 + 操作」,
//         或改用 std::future、執行緒池這類本來就設計成可共享的抽象。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

// 保護 std::cout,避免多執行緒輸出互相切開
std::mutex g_ioMutex;

void say(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_ioMutex);
    std::cout << msg << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】ThreadGuard:把「檢查 + join」包成 RAII
//   情境: 這是《C++ Concurrency in Action》書中的經典守衛類別,
//         也是 C++20 之前管理執行緒生命週期的標準做法。
//         重點在於:即使中途 return 或丟出例外,解構子仍會在
//         堆疊展開時被呼叫,join 因此不會被跳過。
//   為什麼用本主題: 解構子裡的 if (joinable()) 正是本檔的慣用法 ——
//         它讓守衛可以容忍「使用者已經自己 join 過」的情況。
// -----------------------------------------------------------------------------
class ThreadGuard {
    std::thread t_;
public:
    explicit ThreadGuard(std::thread t) : t_(std::move(t)) {}

    ~ThreadGuard() {
        if (t_.joinable()) {   // ← 本檔的慣用法:容忍已被手動 join 的情況
            t_.join();
        }
    }

    // 允許使用者提早手動 join;之後解構子會自動跳過
    void join() {
        if (t_.joinable()) {
            t_.join();
        }
    }

    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;
};

// 示範:即使中途丟出例外,ThreadGuard 仍會 join
void workWithException() {
    ThreadGuard guard{std::thread([]() {
        say("    [背景] 任務開始");
    })};

    say("    [主線] 即將丟出例外");
    throw std::runtime_error("模擬的錯誤");
    // 這行後面的 join 永遠執行不到 —— 但 guard 的解構子會在展開時被呼叫
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔從缺,理由如下
//   本檔的主題是 std::thread 物件的所有權管理與防禦性檢查,
//   屬於 C++ 資源管理的設計問題。LeetCode 的並行題
//   (1114 Print in Order、1115 Print FooBar Alternately、1116 Print Zero Even Odd、
//   1117 Building H2O、1195 Fizz Buzz Multithreaded)由評測框架負責建立與
//   回收執行緒,參賽者只實作被呼叫的成員函式,完全接觸不到 thread 物件,
//   自然也用不到 joinable()。硬湊不如從缺,改以 ThreadGuard 這個
//   業界真正在用的 RAII 模式示範。
// -----------------------------------------------------------------------------

int main() {
    std::cout << "=== 原始示範:重複呼叫也安全 ===" << std::endl;
    {
        std::thread t([]() {
            say("  Task");
        });

        // 安全的寫法(此處其實必定 joinable,檢查是為了示範慣用法)
        if (t.joinable()) {
            t.join();
            std::cout << "  第一次:joinable 為 true → 執行了 join()" << std::endl;
        }

        // 再次呼叫是安全的(因為有檢查)
        if (t.joinable()) {
            t.join();  // 不會執行
            std::cout << "  第二次:這行不會被執行" << std::endl;
        } else {
            std::cout << "  第二次:joinable 已是 false → 安全地跳過"
                      << std::endl;
        }
        std::cout << "  ↑ 少了這個檢查,第二次 join() 會丟 std::system_error"
                  << std::endl;
    }

    std::cout << "\n=== 狀態不確定的場合:條件式建立 ===" << std::endl;
    {
        for (bool shouldRun : {true, false}) {
            std::thread t;   // 預設建構 → joinable() == false

            if (shouldRun) {
                t = std::thread([]() { say("    (條件成立,執行緒有跑)"); });
            }

            // 這裡就是「真正需要」檢查的地方:t 可能從未被賦值
            if (t.joinable()) {
                t.join();
                std::cout << "    shouldRun=" << std::boolalpha << shouldRun
                          << " → 有執行緒,已 join" << std::endl;
            } else {
                std::cout << "    shouldRun=" << std::boolalpha << shouldRun
                          << " → 沒有執行緒,跳過(若無檢查,這裡會丟例外)"
                          << std::endl;
            }
        }
        std::cout << std::noboolalpha;
    }

    std::cout << "\n=== 實務:ThreadGuard 在例外展開時仍能 join ===" << std::endl;
    {
        try {
            workWithException();
        } catch (const std::exception& e) {
            std::cout << "    [主線] 接住例外: " << e.what() << std::endl;
        }
        std::cout << "    ↑ 背景執行緒已被守衛的解構子 join,"
                     "沒有發生 terminate" << std::endl;
    }

    std::cout << "\n=== 提醒:這個慣用法不是執行緒安全的 ===" << std::endl;
    std::cout << "  if (t.joinable()) t.join();  在多條執行緒同時操作"
                 "同一個 t 時是 TOCTOU 競態。" << std::endl;
    std::cout << "  原則:一個 std::thread 物件只該由一個擁有者管理。"
              << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.4：join() 與 detach()5.cpp" -o joinable_guard

// 注意:以下為某一次實際執行的結果,每次執行都相同 ——
//       所有 join 都是同步點,例外展開的順序也是語言保證的。
//       (實務段「[背景] 任務開始」與「[主線] 即將丟出例外」兩行的
//        相對順序理論上可能互換,因為它們分屬兩條執行緒;
//        本機觀察到的是主線先印。)

// === 預期輸出 ===
// === 原始示範:重複呼叫也安全 ===
//   Task
//   第一次:joinable 為 true → 執行了 join()
//   第二次:joinable 已是 false → 安全地跳過
//   ↑ 少了這個檢查,第二次 join() 會丟 std::system_error
//
// === 狀態不確定的場合:條件式建立 ===
//     (條件成立,執行緒有跑)
//     shouldRun=true → 有執行緒,已 join
//     shouldRun=false → 沒有執行緒,跳過(若無檢查,這裡會丟例外)
//
// === 實務:ThreadGuard 在例外展開時仍能 join ===
//     [主線] 即將丟出例外
//     [背景] 任務開始
//     [主線] 接住例外: 模擬的錯誤
//     ↑ 背景執行緒已被守衛的解構子 join,沒有發生 terminate
//
// === 提醒:這個慣用法不是執行緒安全的 ===
//   if (t.joinable()) t.join();  在多條執行緒同時操作同一個 t 時是 TOCTOU 競態。
//   原則:一個 std::thread 物件只該由一個擁有者管理。
