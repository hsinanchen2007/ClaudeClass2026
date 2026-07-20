// =============================================================================
//  課程 5.1：std::mutex 基本操作 (2)  —  lock() / unlock() 修好共享計數器
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<mutex>
//   class std::mutex {                                  // C++11 起
//       void lock();                                    // 阻塞直到取得
//       bool try_lock();                                // 非阻塞，允許偽失敗
//       void unlock();                                  // 釋放（只能由持有者呼叫）
//       mutex(const mutex&) = delete;                   // 不可複製
//       mutex& operator=(const mutex&) = delete;        // 也不可移動
//   };
//   本機實測：sizeof(std::mutex) = 40、alignof = 8，
//             與 sizeof(pthread_mutex_t) = 40 完全相同
//             （g++ 15.2.0 / glibc 2.43 / x86-64；**實作定義，不是標準值**）。
//
// 【詳細解釋 Explanation】
//
// 【1. mutex 同時解決「兩個」問題，不是一個】
//   多數人只記得第一個，面試就是在第二個被刷掉：
//     (a) 互斥（mutual exclusion）：同一時間只有一個執行緒在臨界區段內。
//     (b) 可見性（visibility）：離開臨界區段前寫的東西，
//         下一個進來的執行緒**保證看得到**。
//   只講 (a) 等於只答了一半。5.1-1 那個 data race 之所以是 UB，
//   不只是「遞增被覆蓋」，還包括「另一個核心可能永遠看不到你的寫入」。
//
// 【2. 精確的記憶體序語意（本課最重要的一段）】
//   標準規定：
//     • unlock() 是對該 mutex 的 **release** 操作。
//     • lock()（以及成功的 try_lock()）是對該 mutex 的 **acquire** 操作。
//   由此建立同步關係：
//
//     執行緒 A                        執行緒 B
//     ─────────                       ─────────
//     mtx.lock();
//       counter = 5;      ← A 在臨界區段內的所有寫入
//     mtx.unlock();  ──────────────►  mtx.lock();
//        (release)   synchronizes-with   (acquire)
//                                          read(counter);  ← 保證看到 5
//                                        mtx.unlock();
//
//   用標準術語一句話講清楚：
//     A 對某 mutex 的 unlock() **synchronizes-with** B 之後對**同一個** mutex
//     成功的 lock()；由此推得 A 在 unlock() 之前的所有寫入
//     **happens-before** B 在 lock() 之後的所有讀取。
//
//   三個常被忽略的細節：
//     • 「同一個 mutex」是必要條件。鎖不同的 mutex 之間沒有任何同步關係。
//     • 同步的是**整個臨界區段的所有記憶體**，不是只有你「打算保護」的那個變數。
//     • happens-before 有傳遞性，所以 A→B→C 的鎖交接會把 A 的寫入傳到 C。
//
// 【3. 為什麼 200000 這次是「保證」的】
//   有了鎖之後：
//     • 十萬次遞增被序列化，read-modify-write 不再交錯 → 沒有覆蓋。
//     • 每次 lock 的 acquire 都保證看到上一個持有者 unlock 前的最新值。
//   兩者缺一不可。只有互斥沒有可見性，你可能讀到 stale 的快取值；
//   只有可見性沒有互斥，兩邊還是會同時 read-modify-write。
//   所以 5.1-1 的輸出是 UB，本檔的輸出是**標準保證**的 200000。
//
// 【概念補充 Concept Deep Dive】
//
//   (A) 這個寫法的代價：本檔每次遞增都 lock/unlock 一次，共 20 萬次。
//       這是刻意的教學寫法，用來對照 5.1-1。實務上會把迴圈整個包進
//       一個臨界區段，或直接用 std::atomic<int>（免鎖、快一個數量級）。
//       「鎖的粒度」是 5.6 的主題。
//
//   (B) 為什麼 std::mutex 不可複製也不可移動？
//       本機實測 is_copy_constructible = 0、is_move_constructible = 0。
//       因為鎖的身分就是它的位址：等待佇列、futex word、持有者資訊
//       全都綁在那塊記憶體上。複製一把「已被鎖住」的鎖語意上無解
//       （複製品該是鎖住還是沒鎖？等待者要跟去嗎？），
//       移動同樣會讓正在等待的執行緒失去目標。所以標準直接刪掉。
//       實務推論：含有 mutex 成員的 class 預設也不可複製／移動，
//       想支援就得自己寫 copy constructor（複製資料、但各自持有新的鎖）。
//
//   (C) 手動 lock/unlock 的真正問題不是「忘記寫」，是「例外」。
//       只要臨界區段內任何一行拋出例外，unlock() 就被跳過，鎖永遠不會釋放。
//       本檔為了教學才手寫 lock/unlock；正式程式碼一律用 RAII：
//           { std::lock_guard<std::mutex> g(mtx); ++counter; }   // 出作用域必解鎖
//       本機實測 sizeof(std::lock_guard<std::mutex>) = 8（就是一個指標，零額外成本）。
//
// 【注意事項 Pay Attention】
//   1. std::mutex **不是遞迴鎖**：同一執行緒對已持有的 mutex 再次 lock() 是 UB。
//      需要遞迴請用 std::recursive_mutex（但通常代表設計該重構）。
//   2. unlock() 只能由**當前持有鎖的那個執行緒**呼叫；
//      對未鎖定的 mutex 呼叫 unlock()，或由別的執行緒 unlock，都是 UB。
//   3. lock() 可能拋出 std::system_error（例如資源不足、偵測到死結）。
//   4. sizeof(std::mutex) = 40 是**本平台實測值**，不是標準保證；
//      MSVC 上是另一個數字，不要把它寫進任何跨平台假設裡。
//   5. 鎖保護的是「不變量」不是「變數」。多個變數要一起維持一致，
//      就必須在**同一個**臨界區段內一起改。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::mutex 的互斥與記憶體語意
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. mutex 除了「一次只讓一個人進去」，還提供什麼保證？
//     答：記憶體可見性。unlock() 是 release、lock() 是 acquire；
//         A 的 unlock() synchronizes-with B 之後對同一 mutex 的 lock()，
//         因此 A 在臨界區段內的所有寫入 happens-before B 在臨界區段內的讀取。
//         只答「互斥」是不完整的 —— 沒有可見性保證，B 可能讀到過期的值。
//     追問：如果 A 和 B 鎖的是**不同**的 mutex 呢？
//         → 沒有任何同步關係，happens-before 不成立，一樣是 data race。
//
// 🔥 Q2. std::mutex 為什麼不可複製、也不可移動？
//     答：鎖的身分綁在它的記憶體位址上（等待佇列、futex word 都指向它）。
//         複製一把鎖住的鎖沒有合理語意，移動會讓等待者失去目標，
//         所以標準把 copy/move 都 = delete。
//         連帶效果：任何含 mutex 成員的類別預設也不可複製／移動。
//
// 🔥 Q3. 這支程式的 200000 是「保證」還是「通常」？和 5.1-1 差在哪？
//     答：是標準保證。5.1-1 是 data race → UB，任何輸出都不算違反標準；
//         本檔所有存取都在同一把鎖的臨界區段內，
//         既排除交錯的 read-modify-write，也建立了 happens-before 鏈。
//
// ⚠️ 陷阱. 「臨界區段裡如果拋例外，unlock() 沒執行到，鎖會自動釋放嗎？」
//     答：不會。std::mutex 沒有任何自動釋放機制，
//         連持有它的執行緒整個結束了都不會釋放（見 5.1-4 實測）。
//         鎖會永遠鎖著，後續所有 lock() 都確定地卡死。
//     為什麼會錯：直覺把 mutex 想成「跟著 scope / 跟著執行緒生命週期走」，
//         但那是 std::lock_guard 的行為，不是 std::mutex 本身的行為。
//         裸 mutex 只是一塊狀態，誰都不會幫你收尾。
// ═══════════════════════════════════════════════════════════════════════════

// 檔案：lesson_5_1_with_mutex.cpp
// 說明：使用互斥鎖保護共享資源

#include <iostream>
#include <thread>
#include <mutex>

int counter = 0;      // 共享資源
std::mutex mtx;       // 保護 counter 的互斥鎖

void increment() {
    for (int i = 0; i < 100000; ++i) {
        mtx.lock();       // 🔒 進入臨界區段前獲取鎖
        ++counter;        // 安全！一次只有一個執行緒能執行
        mtx.unlock();     // 🔓 離開臨界區段後釋放鎖
    }
}

int main() {
    std::thread t1(increment);
    std::thread t2(increment);
    
    t1.join();
    t2.join();
    
    std::cout << "預期: 200000" << std::endl;
    std::cout << "實際: " << counter << std::endl;  // 保證是 200000
    
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 5.1：stdmutex 基本操作2.cpp" -o mutex2

// 註 1:本檔輸出是【完全確定的】,連跑 25 次都得到位元組完全相同的結果。
//      這正是本課的重點:一旦 ++counter 被 mutex 保護,
//      結果就不再取決於排程 —— 不管兩條執行緒怎麼交錯,
//      每次 ++ 都是在獨佔臨界區段內完成,總數必然是 100000 × 2。
//      對照同課「沒有 mutex」的版本(4.1),那支每次跑都是不同的數字。
//
// 註 2:【確定的是結果,不是過程。】兩條執行緒實際的交錯順序、
//      誰先搶到鎖、各自被作業系統切換幾次,每次執行都不同,
//      而且完全沒有保證。mutex 保證的是「互斥」,不是「順序」或「公平」。
//      如果需要固定順序,mutex 不夠,要用 condition_variable 之類的機制。
//
// 註 3:本檔用裸 lock()/unlock() 是為了展示教學上的對照;
//      實務上一律該用 std::lock_guard / std::scoped_lock,
//      否則臨界區段內一旦 throw 或 early return,unlock() 就被跳過 → 死結。

// === 預期輸出 ===
// 預期: 200000
// 實際: 200000
