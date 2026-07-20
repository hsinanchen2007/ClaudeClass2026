// 檔案：lesson_2_1_first_thread.cpp

// =============================================================================
//  課程 2.1：第一個多執行緒程式1.cpp  —  建構即啟動、join() 收工
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：  #include <thread>
//   標準版本：std::thread 於 C++11 引入（本檔語法 C++11 起皆可編譯）
//   核心簽名：
//     template<class F, class... Args>
//     explicit thread(F&& f, Args&&... args);  // 建構 = 立即啟動，沒有 start()
//     void join();                             // 阻塞呼叫端，直到該執行緒跑完
//     bool joinable() const noexcept;          // 是否仍持有一條尚未被收回的執行緒
//     ~thread();                               // 若解構時仍 joinable → std::terminate()
//   複雜度／成本：
//     建立一條 thread ≈ 10–100 µs（Linux NPTL：clone() + 預設 8 MB stack 的 VMA 保留）
//     join() 本身 O(1)；真正的成本是「等待時間」+ 一次 futex 喚醒（~1–3 µs）
//   編譯：g++ -std=c++17 -Wall -Wextra -pthread
//
// 【詳細解釋 Explanation】
//
// 【1. 建構即啟動 —— std::thread 沒有 start()】
//   Java 的 Thread 要先 new 再 .start()，C# 也是；std::thread 刻意不這樣設計。
//   `std::thread t(sayHello);` 這一行執行完，sayHello 已經是「可被排程的狀態」，
//   甚至可能在下一行程式碼被執行前就跑完了。
//   為什麼不給 start()？因為「已建構但還沒啟動」是一個多餘且危險的狀態：
//   使用者可能忘記呼叫 start()，而編譯器無法幫你檢查。標準的選擇是讓
//   std::thread 這個物件的「存在」本身就代表「有一條執行緒正在跑（或已跑完）」，
//   物件不變式（invariant）因此非常單純：joinable() 為真 ⇔ 我還欠這條執行緒一次收屍。
//
//   要留意「啟動」不等於「開始執行」：建構函式回傳時，新執行緒只是進入 OS 的
//   runnable queue，何時真的拿到 CPU 由排程器決定（可能是下一奈秒，也可能是
//   100 µs 後）。因此「建立順序」≠「執行順序」，這是後面所有 race condition 的根源。
//
// 【2. join() 到底做了什麼】
//   join() 有三個作用，缺一不可：
//     (a) 阻塞：呼叫端（這裡是 main）停下來，等 t 代表的執行緒執行完畢。
//     (b) 回收：釋放該執行緒的核心／函式庫資源（Linux 上等同 pthread_join，
//         不 join 也不 detach 的執行緒結束後會留下類似 zombie 的記錄）。
//     (c) 建立 happens-before：這是最常被忽略、卻最重要的一點——
//         新執行緒中「所有」的動作，都 happens-before join() 回傳之後的動作。
//         所以 join() 之後讀取該執行緒寫過的資料，不需要額外的 mutex 或 atomic，
//         而且保證讀得到最新值（見【概念補充】(C)）。
//   join() 之後 t.joinable() 變成 false —— 這個物件不再擁有任何執行緒。
//
// 【3. 為什麼本例的輸出順序是「確定的」】
//   多執行緒的輸出通常不確定，但本檔是例外，輸出一定是：
//       Hello from thread!
//       Back in main
//   原因不是運氣，而是 join() 的 happens-before 保證（第 2 點 (c)）：
//   "Back in main" 這行寫在 t.join() 之後，而 sayHello() 的全部工作
//   happens-before join() 回傳，所以兩者之間有嚴格順序，不可能顛倒、不可能交錯。
//   請把這點和第 6 個檔案（兩條執行緒同時印 A / B）對照——那裡沒有任何
//   同步關係介於兩條 worker 之間，所以順序才會每次不同。
//
// 【概念補充 Concept Deep Dive】
//   (A) OS 層面發生了什麼
//       Linux 的 glibc/NPTL 採 1:1 模型：一個 std::thread 對應一個核心可見的
//       task，經由 clone(CLONE_VM|CLONE_THREAD|CLONE_SIGHAND|...) 建立。
//       新執行緒與 main 共享同一個 virtual address space（heap、全域變數、
//       程式碼段、fd table 都共用），但各自擁有 stack 與暫存器狀態。
//       這正是 thread 比 fork() 便宜約一個數量級的原因：不必複製 page table。
//
//   (B) 為什麼要 -pthread（不是 -lpthread）
//       -pthread 同時做兩件事：定義 _REENTRANT 巨集（影響標頭檔中的宣告與某些
//       巨集的執行緒安全版本），並連結執行緒函式庫。glibc 2.34 起 pthread 已併入
//       libc，光靠 -lpthread 甚至可能「看起來也能編過」，但仍應寫 -pthread，
//       因為它是編譯階段與連結階段都正確的可攜寫法。漏掉它在某些平台會得到
//       連結錯誤，或更糟——執行期才炸（std::system_error）。
//
//   (C) happens-before 不只是「先後」
//       happens-before 是 C++ 記憶體模型中的**偏序關係**，它同時約束兩件事：
//       編譯器不得把跨越該邊界的存取重排，以及硬體必須讓寫入對另一方可見
//       （在 x86 上通常對應一個 acquire/release 語意，ARM 上則會產生實際的
//       記憶體屏障指令）。「t 先跑完」只是時間上的事實，若沒有 join() 這種
//       同步操作建立 happens-before，另一條執行緒仍可能讀到過期的值。
//
//   (D) std::endl vs '\n'
//       本檔用 std::endl，它 = 輸出 '\n' + flush。在多執行緒教材裡 flush 有實際
//       意義：它讓輸出立刻離開使用者空間緩衝區，避免程式崩潰時遺失（第 4 個檔
//       的 abort 示範正是靠這點）。但在效能敏感的迴圈中，每行 flush 是很貴的
//       系統呼叫，正式程式碼應優先用 '\n'。
//
// 【注意事項 Pay Attention】
//   1. join() 只能呼叫一次。對已經 join 過（joinable() == false）的物件再次 join，
//      會拋出 std::system_error（errc::invalid_argument）；若無人接住例外，
//      最終仍是 std::terminate()。
//   2. 解構時仍 joinable 的 std::thread 會呼叫 std::terminate() —— 這是**標準規定**
//      的行為，不是未定義行為，一定會發生（詳見第 4 個檔案）。
//   3. std::thread 不可複製（copy constructor 被 delete），只能 move。它是獨佔式
//      的資源持有者，語意上接近 unique_ptr。
//   4. std::cout 在 C++11 之後保證「不會產生 data race」（對標準串流物件的並行
//      操作不是 UB），但**不保證不交錯**：兩條執行緒各印一行，字元仍可能穿插。
//      要保證整行完整，需自備 mutex 或使用 C++20 的 std::osyncstream。
//   5. 執行緒的預設 stack 大小是**實作定義**的：Linux/glibc 通常 8 MB（可用
//      `ulimit -s` 查詢），musl 約 128 KB，Windows 預設 1 MB。std::thread 沒有
//      提供標準方式調整，需要時只能走 pthread_attr_t 等平台 API。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::thread 的建立與 join()
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::thread 為什麼沒有 start()？建構函式回傳時執行緒已經在跑了嗎？
//     答：建構即啟動，回傳時該執行緒已處於 runnable 狀態（可能已跑完，也可能
//         還沒拿到 CPU）。不提供 start() 是為了消滅「已建構但未啟動」這個容易
//         忘記處理的中間狀態，讓物件不變式維持單純：物件存在 ⇔ 執行緒存在。
//     追問：那「已啟動」是否代表「已開始執行」？→ 不是，何時取得 CPU 由 OS
//           排程器決定，所以建立順序不等於執行順序。
//
// 🔥 Q2. join() 除了「等它跑完」還做了什麼？
//     答：還有兩件事：回收該執行緒的資源（等同 pthread_join，避免類 zombie 殘留），
//         以及建立 happens-before —— 該執行緒內的所有寫入，對 join() 回傳後的
//         程式碼都保證可見。所以 join() 之後讀取其結果不需要額外同步。
//     追問：那如果我只是 sleep 夠久、確定它跑完了，可以不 join 嗎？→ 不行。
//           時間上的「跑完」不建立 happens-before，可能讀到過期值；而且物件仍
//           joinable，解構時照樣 terminate。
//
// ⚠️ 陷阱. 本檔輸出「Hello from thread!」一定在「Back in main」之前嗎？
//     答：是，一定。這是本課少數「順序確定」的例子——因為 "Back in main" 寫在
//         t.join() 之後，join() 的 happens-before 保證讓兩者有嚴格順序。
//     為什麼會錯：很多人反射性回答「多執行緒都不確定」，把「非確定性」當成
//         多執行緒的公理。真正的規則是：**沒有同步關係時才不確定**。join()、
//         mutex、atomic 都會建立順序保證，這正是我們寫同步程式碼的目的。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>

void sayHello() {
    std::cout << "Hello from thread!" << std::endl;
}

int main() {
    std::thread t(sayHello);  // 建立並啟動執行緒
    t.join();                 // 等待執行緒結束
    
    std::cout << "Back in main" << std::endl;
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread 課程\ 2.1：第一個多執行緒程式1.cpp -o first_thread

// ✅ 本檔的輸出順序是**確定的**（不是每次都不同）：
//    順序才會每次執行都不同。

// === 預期輸出 ===
// Hello from thread!
// Back in main
//
//    "Back in main" 寫在 t.join() 之後，join() 建立的 happens-before 關係
//    保證 sayHello() 的全部動作都排在它前面。
//    請與第 6 個檔案（兩條執行緒同時印 A / B）對照——那裡沒有同步關係，
// 實測 exit code = 0。
