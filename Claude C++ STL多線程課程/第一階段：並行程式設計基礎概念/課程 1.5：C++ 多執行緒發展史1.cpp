// =============================================================================
//  課程 1.5：C++ 多執行緒發展史1.cpp  —  C++11 之前：POSIX Threads 的世界
// =============================================================================
//
// 【主題資訊 Information】
//   int pthread_create(pthread_t* thread, const pthread_attr_t* attr,
//                      void* (*start_routine)(void*), void* arg);
//   int pthread_join  (pthread_t thread, void** retval);
//
//   標準版本：**不是 C++ 標準**。這是 POSIX.1-2001 (IEEE Std 1003.1) 的 C API。
//             C++98/C++03 標準完全沒有執行緒的概念 —— 見【詳細解釋 1】。
//   標頭檔：  <pthread.h>（POSIX 系統；Windows 需改用 <windows.h> 的 CreateThread）
//   回傳值：  成功回傳 0，失敗回傳「錯誤碼本身」（EAGAIN / EINVAL / EPERM…）。
//             注意：pthread_* 函式**不設定 errno**，這是它與多數 POSIX 呼叫的差別。
//   編譯：    必須加 -pthread（不只是 -lpthread，見【詳細解釋 4】）
//
// 【詳細解釋 Explanation】
//
// 【1. C++98/03 為什麼「沒有」多執行緒 —— 比你想的更嚴重】
// 常見說法是「C++98 沒有提供執行緒函式庫，要自己用 pthread」。這只講了一半。
// 真正的問題是：C++98/03 的抽象機器模型裡，**只存在一條執行流**。標準沒有定義
// 「兩條執行緒同時存取同一個物件」會發生什麼事 —— 不是定義成 UB，而是根本
// 「沒有詞彙可以描述這件事」。後果是：
//   * 編譯器最佳化只需保證「單執行緒的可觀察行為不變」。它可以合法地把
//     一次寫入拆成兩次、可以把不該搬的寫入搬進迴圈、可以對相鄰的 bitfield
//     做 read-modify-write 而意外覆寫隔壁欄位。
//   * 因此嚴格來說，「用 pthread 寫的 C++98 程式」在 C++98 標準下**沒有任何保證**。
//     它能跑，靠的是編譯器與 pthread 實作之間的「君子協定」，不是標準。
// 這正是 C++11 最重要的貢獻：先給出**記憶體模型**（memory model），
// 定義 happens-before、data race 與 UB，`std::thread` 才有意義。
// 換句話說 —— C++11 的核心不是 std::thread 那個類別，而是它底下那套記憶體模型。
//
// 【2. void* 介面：型別安全的代價】
// start_routine 的型別被固定成 `void* (*)(void*)`。要傳任何東西都得：
//     傳入：取位址 → 隱式轉成 void*
//     取出：手動 (int*)arg 轉回來 ← 編譯器完全不檢查這一步
// 若你轉錯型別，編譯器不會有任何抱怨，執行期直接是 UB。而且：
//   * 只能傳「一個」參數。要傳多個必須自己包一個 struct 並管理其生命週期。
//   * 參數是**以指標傳遞**的，呼叫端必須保證該物件活得比執行緒久
//     （本範例的 value 在 main 的堆疊上，因為有 pthread_join 才安全）。
// 對照 C++11：`std::thread t(f, 42, "abc", obj)` 是變參模板，型別安全、
// 參數預設**複製**進執行緒自己的儲存區，生命週期問題大幅減少。
//
// 【3. pthread_create 的四個參數】
//   thread        [out] 執行緒識別碼（不透明型別，不保證是整數，不可 printf %d）
//   attr          [in]  屬性：堆疊大小、detach 狀態、排程政策；NULL = 預設
//   start_routine [in]  進入點；回傳的 void* 由 pthread_join 的第二參數取回
//   arg           [in]  唯一的參數
//
// 【4. -pthread 與 -lpthread 的差別（面試常考）】
//   -lpthread 只是「連結 libpthread」。
//   -pthread  同時做兩件事：連結，**並且**定義 _REENTRANT / 開啟編譯器的
//             執行緒感知模式（影響 errno 的 TLS 展開、某些平台的例外處理表）。
//   正確做法一律是 -pthread，且編譯與連結兩階段都要加。
//   （glibc 2.34 起 libpthread 已併入 libc，但 -pthread 仍是正確寫法。）
//
// 【概念補充 Concept Deep Dive】
// (A) Linux 上 pthread_create 實際做了什麼
//     glibc/NPTL 是 **1:1 模型**：一個 pthread 對應一個 kernel 排程實體，
//     底層走 clone(CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND |
//     CLONE_THREAD | CLONE_SETTLS | ...)。CLONE_VM 表示共用同一份
//     address space（heap、全域變數、程式碼段都共享），CLONE_THREAD 讓它
//     與建立者屬於同一個 thread group（因此 getpid() 相同）。
//     每條執行緒**各自擁有**：stack（Linux 預設 8 MB，由 ulimit -s 決定）、
//     暫存器狀態、TLS 區塊、以及自己的 errno。
// (B) 為什麼 errno 不會在多執行緒下互相踩到
//     因為它其實不是變數，而是被定義成 `*__errno_location()` 這樣的巨集，
//     每條執行緒回傳自己 TLS 裡的位址。這是 _REENTRANT 年代留下的設計。
// (C) 為什麼 pthread_* 回傳錯誤碼而不是設 errno
//     正因為 errno 本身需要 TLS 支援。pthread 是「提供執行緒」的那一層，
//     不能反過來依賴「已經有執行緒支援」的 errno，這是分層上的必然選擇。
//
// 【注意事項 Pay Attention】
// 1. **回傳值一定要檢查**。原始教材範例（與多數教科書一樣）忽略了
//    pthread_create 的回傳值；資源不足時它會回傳 EAGAIN，此時 thread 這個
//    識別碼是未初始化的，後續 pthread_join 就是 UB。本檔已補上檢查。
// 2. pthread_t 是**不透明型別**，不保證是整數，不可以用 %d 印，也不可以
//    用 == 比較（要用 pthread_equal）。Linux 上剛好是 unsigned long，
//    但這是**實作定義**，不可移植。
// 3. 執行緒函式**不可以回傳指向自己區域變數的指標** —— 該執行緒結束時
//    它的堆疊就失效了，pthread_join 取回的會是懸空指標（UB）。
// 4. 未 join 也未 detach 的執行緒結束後，其資源不會被回收（類似殭屍程序），
//    這是舊程式碼常見的資源洩漏來源。
// 5. 這段程式碼**無法跨平台**：Windows 沒有 pthread，要改寫成 CreateThread /
//    WaitForSingleObject。這正是 C++11 要解決的核心痛點。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】C++11 之前的多執行緒 / POSIX Threads
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++11 為多執行緒帶來的最重要東西是什麼？是 std::thread 嗎？
//     答：不是。最重要的是**記憶體模型**（memory model）。C++98/03 的抽象機器
//         只有一條執行流，標準沒有詞彙描述「並行存取同一物件」，因此編譯器
//         最佳化只需維持單執行緒語意。C++11 先定義了 happens-before、
//         data race 與 UB，std::thread / std::atomic 才有可依附的語意基礎。
//     追問：那 C++98 + pthread 寫出來的程式到底算不算對？
//         → 標準上沒有保證，實務上靠編譯器與 pthread 實作的協定；
//           C++11 之後才真正「有標準可依」。
//
// 🔥 Q2. 編譯多執行緒程式為什麼要加 -pthread，跟 -lpthread 差在哪？
//     答：-lpthread 只做連結；-pthread 同時定義 _REENTRANT 並開啟編譯器的
//         執行緒感知模式（影響 errno 的 TLS 展開與部分平台的例外處理表），
//         而且編譯與連結兩階段都應該加。一律用 -pthread。
//     追問：glibc 2.34 之後 libpthread 已併進 libc，那還需要加嗎？
//         → 需要。連結面向雖已無差別，但 -pthread 的「編譯期」語意仍在。
//
// ⚠️ 陷阱. pthread_create 失敗時，可以用 errno 判斷原因嗎？
//     答：不行。pthread_* 系列**直接回傳錯誤碼**（EAGAIN、EINVAL…），
//         成功回傳 0，而且**不會設定 errno**。要寫
//         `int rc = pthread_create(...); if (rc != 0) { /* rc 就是錯誤碼 */ }`。
//     為什麼會錯：多數 POSIX 呼叫（open、read、write）確實是「回傳 -1 並設
//         errno」，很多人就把這個模式套到 pthread 上。但 errno 本身要靠 TLS
//         才能在多執行緒下正確運作，而 pthread 正是提供執行緒的那一層，
//         不能反過來依賴它 —— 所以它只能走回傳值。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode】從缺。
//   本課主題是「歷史脈絡與 POSIX C API」，LeetCode 並行題（1114/1115/1116/
//   1117/1195）考的都是同步邏輯，與本課主題對不上，故不強加。
// =============================================================================

// C++11 之前的 POSIX 多執行緒寫法
#include <pthread.h>
#include <stdio.h>
#include <string.h>

void* threadFunction(void* arg) {
    int* value = (int*)arg;   // ← 型別完全靠人工維護，編譯器不檢查
    printf("執行緒收到值: %d\n", *value);
    return NULL;
}

// 示範「用 void* 取回結果」——舊式 API 唯一的回傳管道。
// 注意：回傳的指標必須指向比執行緒活得久的儲存區（此處用 static），
//       絕不可回傳指向自己堆疊上區域變數的指標。
static int g_result;

void* squareFunction(void* arg) {
    int* value = (int*)arg;
    g_result = (*value) * (*value);
    return &g_result;         // 呼叫端用 pthread_join 的第二參數取回
}

int main() {
    printf("=== 段 1：原始的 POSIX 執行緒寫法 ===\n");
    pthread_t thread;
    int value = 42;

    // 建立執行緒（補上回傳值檢查：pthread_* 回傳錯誤碼，不設 errno）
    int rc = pthread_create(&thread, NULL, threadFunction, &value);
    if (rc != 0) {
        fprintf(stderr, "pthread_create 失敗: %s\n", strerror(rc));
        return 1;
    }

    // 等待執行緒結束
    pthread_join(thread, NULL);

    printf("\n=== 段 2：用 void* 取回回傳值（型別不安全的代價）===\n");
    pthread_t t2;
    int n = 7;
    rc = pthread_create(&t2, NULL, squareFunction, &n);
    if (rc != 0) {
        fprintf(stderr, "pthread_create 失敗: %s\n", strerror(rc));
        return 1;
    }

    void* raw = NULL;
    pthread_join(t2, &raw);          // 取回 void*
    int* got = (int*)raw;            // ← 這一步轉錯型別編譯器不會警告
    printf("%d 的平方 = %d\n", n, *got);
    printf("對照 C++11：std::future<int> 直接給你型別正確的 int，無須轉型\n");

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 1.5：C++ 多執行緒發展史1.cpp" -o lesson_1_5a

// === 預期輸出 ===
// === 段 1：原始的 POSIX 執行緒寫法 ===
// 執行緒收到值: 42
//
// === 段 2：用 void* 取回回傳值（型別不安全的代價）===
// 7 的平方 = 49
// 對照 C++11：std::future<int> 直接給你型別正確的 int，無須轉型
//
// ── 關於「非決定性」的說明 ────────────────────────────────────────────────
// 本檔輸出是**決定性的**（每次執行都相同）：兩段都用 pthread_join 等到執行緒
// 結束才往下走，主執行緒與子執行緒不會同時輸出，因此沒有任何交錯。
// 若把 pthread_join 拿掉改成同時跑兩條，輸出就會開始交錯 —— 那才是常態。
// exit code = 0（本機實測）。
