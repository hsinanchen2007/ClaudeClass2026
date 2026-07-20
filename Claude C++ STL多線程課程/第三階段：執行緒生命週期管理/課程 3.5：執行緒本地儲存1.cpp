// =============================================================================
//  課程 3.5：執行緒本地儲存1.cpp  —  thread_local 入門：每執行緒一份的計數器
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  thread_local T name = initializer;              // C++11 起
//   thread_local 是 storage-class-specifier（儲存類別指定符），
//   和 static / extern / mutable 同一類；它**不是**型別修飾詞（不像 const/volatile）。
//   因此 `thread_local int x;` 的型別就是 int，不是什麼「thread_local int」。
//
//   可以出現的位置（只有三種）：
//     1. namespace scope（全域變數）           ← 本檔用的就是這種
//     2. block scope（函式內區域變數，可搭配 static/extern，static 是多餘的）
//     3. class 的 static data member（非 static 成員不行）
//   標頭檔：不需要（它是語言關鍵字，不是函式庫設施）；本檔的 std::thread 需要 <thread>。
//   存取複雜度：O(1)。
//   標準版本：C++11。本機用 -std=c++03 編譯 `thread_local int x;` 會得到
//             error: 'thread_local' does not name a type（實測 g++ 15.2.0）。
//
// 【詳細解釋 Explanation】
//
// 【1. 一份 vs 每執行緒一份 —— 這就是全部的重點】
//   把 counter 宣告成三種樣子，差別是「有幾份實體」：
//     int counter;               → 整個程式 1 份，所有執行緒搶同一個
//     static int counter;        → 整個程式 1 份（static 只是改連結性/範圍，不是份數）
//     thread_local int counter;  → 有幾條執行緒就有幾份，互不相干
//   本檔開了 2 條執行緒，所以記憶體中有 3 份 counter：t1 一份、t2 一份、
//   主執行緒也有一份（雖然主執行緒沒去碰它）。兩條執行緒各自從 0 數到 3，
//   所以輸出是「A:1,2,3」和「B:1,2,3」，**不是**接力數到 6。
//
// 【2. 為什麼不需要 mutex】
//   資料競爭（data race）的定義是「兩個以上執行緒未同步地存取**同一個**記憶體位置，
//   且至少一方是寫入」。thread_local 直接把「同一個記憶體位置」這個前提拿掉了 ——
//   t1 寫的是 t1 的 counter，t2 寫的是 t2 的 counter，位址根本不同。
//   沒有共享，就沒有競爭，也就不需要任何同步。
//   這是 thread_local 的核心價值：它不是「比較快的鎖」，而是**讓鎖變得不必要**。
//
// 【3. 什麼時候初始化】
//   `thread_local int counter = 0;` 用的是常數初始化（constant initialization），
//   在執行緒的 TLS 區塊建立時就已經是 0，沒有執行期成本。
//   若初始化式需要執行程式碼（例如 thread_local std::string s = f();），
//   那叫動態初始化，會延遲到該執行緒**第一次 odr-use 它**時才跑（見檔 7 的實測）。
//
// 【概念補充 Concept Deep Dive】
//   Linux/x86-64 的 ELF TLS 是這樣做的：每條執行緒有一塊自己的 TLS block，
//   位址放在 %fs 段暫存器指向的執行緒指標（thread pointer）。存取一個 thread_local
//   變數 = 「%fs 的值 + 這個變數在 TLS block 中的固定偏移量」。
//   本機把本檔這種寫法（變數就在執行檔內）編譯出來，實測是**一條指令**：
//       movl  %fs:counter@tpoff, %eax      ← @tpoff = thread pointer offset
//   對照一般全域變數是 `movl counter(%rip), %eax`，同樣一條指令。
//   所以在執行檔裡，thread_local 的讀寫成本和普通全域變數幾乎一樣
//   （本機實測 1.212 ns/op vs 1.276 ns/op，差距在雜訊內；每次執行都不同）。
//   真正變慢的是動態函式庫的情形，那會走 __tls_get_addr 函式呼叫 —— 見檔 7。
//   TLS block 由 pthread_create 在建立執行緒時一併配置，執行緒結束時一併釋放。
//
// 【注意事項 Pay Attention】
//   1. 每條執行緒一份 → 執行緒很多時記憶體會等比放大。thread_local 一個 1MB 的
//      緩衝區、開 100 條執行緒，就是 100MB。
//   2. thread_local 變數**無法被別條執行緒讀到**。要彙總結果必須自己收集
//      （典型作法：執行緒結束前把自己的值加進一個有鎖/atomic 的總計）。
//   3. 主執行緒也有自己的一份，別忘了它也算一條執行緒。
//   4. 輸出交錯：本檔沒有鎖保護 std::cout，兩條執行緒的訊息**可能**互相穿插。
//      這不是 data race（見下方陷阱題），而是輸出順序不確定。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】thread_local 基本語意
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. thread_local 和 static 差在哪裡？
//     答：差在「份數」，不是差在生命週期長短。static 讓變數在整個程式中只有一份，
//         所有執行緒共用；thread_local 讓每條執行緒各有一份獨立實體。
//         兩者都是儲存期（storage duration）的指定：static 是靜態儲存期，
//         thread_local 是執行緒儲存期。
//     追問：那 `static thread_local int x;` 寫在函式裡是什麼意思？
//         → 在 block scope，thread_local 已經隱含靜態儲存期，static 是**多餘的**
//           （合法但不改變任何語意，本機以 -pedantic-errors 實測可編譯）。
//
// 🔥 Q2. 這支程式的 counter 沒有任何 mutex，為什麼不算 data race？
//     答：data race 的成立要件是「多執行緒未同步存取**同一記憶體位置**且至少一方寫入」。
//         thread_local 讓 t1 與 t2 存取的是兩個不同位址的物件，共享這個前提不成立，
//         所以連競爭的機會都沒有，不需要同步。
//     追問：那把 counter 改成 static 會怎樣？
//         → 立刻變成貨真價實的 data race（++ 是讀-改-寫三步），是 UB，
//           最終印出的值不保證是 6，也不保證是任何特定數字。
//
// ⚠️ 陷阱. 既然完全不需要同步，為什麼實測輸出還是可能交錯成一團？
//     答：因為 counter 不共享，但 **std::cout 是共享的**。標準保證對同步過的
//         標準串流物件做並行格式化輸出**不會產生 data race**（[iostream.objects]），
//         但**完全不保證**兩個執行緒的字元不會互相穿插——
//         `cout << a << b << c` 是三次獨立呼叫，中間可以被切換走。
//     為什麼會錯：多數人把「沒有 data race」直接等於「輸出會整齊」。
//         這正是 data race 與 race condition 的分野：
//         這裡**沒有 data race**（沒有 UB），但**有 race condition**
//         （結果取決於時序，是邏輯層面的不確定）。要輸出不交錯得自己加鎖，
//         或用 C++20 的 std::osyncstream。
//         本檔 1 的實測剛好沒交錯，但同課的檔 3、5、6 就實際印出交錯結果。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>

thread_local int counter = 0;  // 每個執行緒有獨立的 counter

void increment(const std::string& name) {
    for (int i = 0; i < 3; ++i) {
        ++counter;
        std::cout << name << ": counter = " << counter << std::endl;
    }
}

int main() {
    std::thread t1(increment, "Thread A");
    std::thread t2(increment, "Thread B");
    
    t1.join();
    t2.join();
    
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.5：執行緒本地儲存1.cpp" -o tls1

// 註 1:本檔輸出【每次執行都不同】,而且變化程度比直覺大得多。
//      本機實測連跑 60 次,得到 12 種相異輸出;其中
//        • 37/60(約 62%)是下方這種「兩條執行緒的第一行互相插入」的形式
//        • 只有  6/60(約 10%)是六行都完整、完全不交錯
//      下方預期輸出取【出現次數最多】的那一種,不是唯一正確答案。
//
// 註 2:為什麼會「行中間」被插斷?因為
//        std::cout << name << ": counter = " << counter << std::endl;
//      是【四次獨立】的 operator<< 呼叫,不是一次原子操作。
//      C++11 起標準只保證 cout 本身不會因並行存取而資料損毀
//      ([iostream.objects] 的 race-free 要求),但【完全不保證】
//      一整條 << 鏈中間不會被其他執行緒插進來。
//      要輸出不交錯,必須自己加鎖,或先組成單一 std::string 再一次輸出。
//
// 註 3:【確定不變的部分】是本課真正的重點:無論怎麼交錯,
//      每條執行緒讀到的 counter 序列永遠是 1 → 2 → 3。
//      兩條執行緒各有一份獨立的 counter,從未出現 4、5、6,
//      也從未出現任何一條執行緒讀到對方的值。這正是 thread_local 的保證。
//      會抖動的只有「字元印出來的順序」,不是「數值」。

// === 預期輸出 ===
// （以下為 60 次實測中出現 37 次的多數形式；交錯位置每次可能不同）
// Thread A: counter = Thread B: counter = 11
//
// Thread A: counter = 2
// Thread A: counter = 3
// Thread B: counter = 2
// Thread B: counter = 3
//
// （6/60 的執行會得到完全不交錯的版本，如下）
// Thread A: counter = 1
// Thread A: counter = 2
// Thread A: counter = 3
// Thread B: counter = 1
// Thread B: counter = 2
// Thread B: counter = 3
