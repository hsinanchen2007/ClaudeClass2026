// =============================================================================
//  summary.cpp  —  第四階段：共享資料與競爭條件（總複習・本階段教科書）
// =============================================================================
//
// 【主題資訊 Information】
//   涵蓋課程：4.1 共享資料的問題 / 4.2 不變量與競爭條件 / 4.3 臨界區段概念
//             4.4 資料競爭範例分析 / 4.5 競爭條件的檢測
//   標準版本：std::thread / std::mutex / std::atomic / 記憶體模型 皆為 C++11
//             （C++98/03 的標準裡根本沒有執行緒的概念）
//   標頭檔：  <thread>、<mutex>、<atomic>、<vector>、<map>
//   本機環境：Ubuntu 26.04 / g++ 15.2.0 / 16 核心
//   偵測工具：g++ -fsanitize=thread -g -pthread（ThreadSanitizer）
//
// 【詳細解釋 Explanation】
//
// 【1. 本階段的一句話總結】
//   【不變量的破壞是必然的；讓它不可觀察，才是同步的目的。】
//   任何多步驟的修改都會產生中間狀態 —— 這是物理事實，躲不掉。
//   單執行緒天生免費得到「沒有人能觀察中間狀態」這個保護；
//   多執行緒則必須用 mutex / atomic / 不可變設計把它買回來。
//   整個第四階段的五課，都是這句話的不同面向。
//
// 【2. 兩個必須分清楚的名詞（面試最愛考）】
//   * data race（資料競爭）：兩條以上執行緒同時存取同一記憶體位置，
//     至少一方是寫入，且兩者之間沒有 happens-before 關係。
//     → 這是 C++ 標準 [intro.races] 判定的【未定義行為】，屬「語言規則」問題。
//   * race condition（競爭條件）：程式的結果取決於執行時序的【邏輯瑕疵】。
//     → 屬「設計」問題，與語言 UB 無關。
//   兩者是【交叉】而非包含關係：
//     - 有 data race 通常也造成 race condition（4.1 / 4.4 的多數範例）
//     - 沒有 data race 也可能有 race condition：
//       每次存取都正確加鎖，卻在鎖與鎖之間做 check-then-act（4.2-3 的 atomic 對照組）
//     - 有 data race 但沒有 race condition：
//       兩條執行緒都把同一個變數寫成同一個常數 —— 結果不取決於時序，
//       但標準仍判 data race → UB
//
// 【3. 五大競爭模式（code review 的檢查清單）】
//   ① Check-Then-Act    ：if (條件) { 依賴該條件的動作 }        → 4.1 / 4.4-3
//      「檢查」與「行動」之間有縫隙，別人可以把前提改掉。
//   ② Read-Modify-Write ：counter++、x = x + 1                  → 4.4-2
//      看似一步，實為 load / modify / store 三步。
//   ③ 複合操作（跨變數） ：一個不變量橫跨多個變數，卻分開更新     → 4.2-3
//      原子化每個零件，不會讓組裝過程變原子。
//   ④ 迭代器失效        ：一邊走訪一邊修改容器                   → 4.4-4
//      後果是 use-after-free，比數值錯誤嚴重得多。
//   ⑤ 部分更新          ：物件的多個欄位分開賦值                 → 4.4-6
//      讀者看到「從未存在過」的組合，且每個欄位單獨看都合法。
//   → 這五個模式涵蓋了實務上絕大多數的並行 bug。
//     人工審查時直接拿這張清單去對，命中率極高。
//
// 【4. 判斷「該用什麼」的決策樹】
//   問：要保護的不變量橫跨幾個變數？
//     * 一個變數，且整個操作能壓縮成單一原子指令（如 ++）
//       → std::atomic（fetch_add / compare_exchange），最快
//     * 一個變數，但操作是 check-then-act
//       → atomic + compare_exchange 重試迴圈，或用鎖
//     * 多個變數
//       → 必須用 mutex（沒有任何單一原子指令能同時改多個位置）
//     * 讀多寫極少
//       → 不可變物件 + shared_ptr 指標交換（copy-on-write），讀取端零同步
//     * 資料天生可以各自獨立
//       → thread_local 或本地累加，完全不需要同步（最快的同步是不同步）
//
// 【5. 為什麼「跑幾次都對」永遠不是證據】
//   data race 一旦成立就是 UB，標準不再對程式的任何行為負責 ——
//   【包含「看起來完全正常」】。
//   本階段多個範例實測都出現「多數執行看起來正常」的情形
//   （4.1-2 常常剛好印出 2000、4.2-2 壓倒性多數印出完整的 1 2 3），
//   原因往往只是迴圈太短、執行緒建立成本大於整段工作、根本沒有真正重疊執行。
//   → 正確性是【程式的結構性質】：要嘛所有衝突存取之間都有 happens-before，
//     要嘛沒有。這需要形式化判定（ThreadSanitizer）或人工推理，
//     不能靠抽樣統計。
//
// 【概念補充 Concept Deep Dive】
//
// (A) happens-before：整個記憶體模型的核心概念
//   C++11 引入的記憶體模型，用 happens-before 關係定義「什麼時候
//   一條執行緒的寫入對另一條執行緒可見」。建立這個關係的方式包括：
//     * 同一條執行緒內的程式順序（sequenced-before）
//     * mutex 的 unlock → 之後對同一把鎖的 lock
//     * std::thread 的建構（建立前的寫入對新執行緒可見）
//     * join()（被 join 的執行緒的所有寫入對呼叫者可見）
//     * atomic 的 release 寫入 → 之後對同一變數的 acquire 讀取
//   沒有這些關係中的任何一個，兩次衝突存取就構成 data race。
//   → 這也解釋了為什麼「主執行緒初始化完 → 啟動執行緒 → join 後讀結果」
//     這個經典流程完全不需要額外同步。
//
// (B) 為什麼編譯器與 CPU 的重排讓「靠推理」變得不可靠
//   在 as-if 規則下，只要單執行緒的可觀察行為不變，
//   編譯器可以任意重排、快取到暫存器、甚至消除整段程式碼；
//   CPU 也會亂序執行與推測執行。
//   所以「我把 flag 放在資料後面寫，讀者看到 flag 就代表資料好了」
//   這種推理在沒有記憶體屏障時完全不成立。
//   本階段的 4.2-1 概念補充就是這個議題的入門。
//
// (C) 為什麼 x86-64 上的實測特別容易造成錯覺
//   x86-64 是【強記憶體模型】（TSO），硬體本身就禁止大部分的重排，
//   而且單一對齊的 int 讀寫不會撕裂。
//   所以很多在 ARM（弱記憶體模型）上會立刻爆炸的錯誤程式碼，
//   在 x86 上跑起來「好像沒事」。
//   → 在 x86 上測不出來，完全不代表程式碼在手機、伺服器 ARM、
//     或 Apple Silicon 上是安全的。這是跨平台專案最常見的災難來源。
//
// 【注意事項 Pay Attention】
// 1. 「不變量被破壞」是必然；「破壞被觀察到」才是 bug。
// 2. data race 是 UB（語言規則），race condition 是邏輯瑕疵（設計問題），兩者交叉。
// 3. 原子化每個零件，不會讓組裝過程變原子 —— 跨變數的不變量只能用鎖。
// 4. 讀取端也必須加鎖：data race 的條件是「至少一方寫入」，不是「雙方都寫」。
// 5. 「跑幾次都對」永遠不是證據；請用 ThreadSanitizer 做形式化判定。
// 6. 在 x86-64（強記憶體模型）上測不出來，不代表在 ARM 上安全。
// 7. 最快的同步是不同步：能用 thread_local、本地累加、不可變資料就別用鎖。
//
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1116. Print Zero Even Odd
//   （實作於下方 ZeroEvenOdd 類別）
//   題目：三條執行緒分別呼叫 zero() / even() / odd()，
//         必須協同輸出 "0102030405..." 這樣的序列。
//   為什麼用到本主題：這題把第四階段的三個核心概念濃縮在一起 ——
//         ① 共享狀態（計數器 current 被三條執行緒讀寫）
//         ② 不變量（「下一個該印什麼」由 current 的奇偶與階段唯一決定）
//         ③ 臨界區段（判斷輪到誰 + 印出 + 推進計數器，必須是一個原子步驟）
//         若把「判斷」與「印出」分開，就是 check-then-act：
//         兩條執行緒可能同時認為輪到自己。
//         而且光有 mutex 不夠 —— mutex 保證互斥卻不保證順序，
//         必須用 condition_variable 表達「等到輪到我」。
//         這正是第四階段（發現問題）通往第五、六階段（mutex / condition_variable）
//         的橋樑。
// -----------------------------------------------------------------------------
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】第四階段總複習
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. data race 和 race condition 有什麼不同?各舉一個例子。
//     答：data race 是「兩執行緒同時存取同一位置、至少一方寫、無 happens-before」，
//         由 C++ 標準判定為 UB，屬語言規則問題；
//         race condition 是「結果取決於執行時序」的邏輯瑕疵，屬設計問題。
//         兩者交叉：
//         * 兩者都有：無同步的 counter++
//         * 只有 race condition：每次存取都正確加鎖，
//           但在鎖與鎖之間做 check-then-act（先查餘額、放鎖、再扣款）
//         * 只有 data race：兩條執行緒把同一變數都寫成 42 ——
//           結果不取決於時序，但標準仍判 UB
//     追問：為什麼標準要把「都寫成同一個值」也判成 UB?
//         → 因為標準要保留給實作的最佳化空間，
//           而且在弱記憶體模型或非對齊存取上，撕裂仍然可能發生。
//           標準只承諾「有同步就正確」，不去區分哪些無同步的情況「碰巧安全」。
//
// 🔥 Q2. 什麼時候用 atomic、什麼時候必須用 mutex?
//     答：判準是【要保護的不變量橫跨幾個變數，以及整個操作能否壓縮成
//         單一原子動作】。
//         * 一個變數且操作本身是單一原子指令（counter.fetch_add(1)）→ atomic
//         * 一個變數但是 check-then-act → atomic + compare_exchange 重試迴圈
//         * 不變量橫跨多個變數（A + B == 2000）→ 必須用 mutex，
//           因為沒有任何單一原子指令能同時修改兩個記憶體位置
//     追問：那 std::atomic<某個大結構> 呢?
//         → 若該型別不是 lock-free（可用 is_always_lock_free 在編譯期查詢），
//           實作內部仍然是用鎖，等於繞一圈回到 mutex，
//           而且還多了「必須 trivially copyable」的限制。
//
// 🔥 Q3. 如何證明一段多執行緒程式碼沒有 data race?
//     答：不能靠執行測試。壓力測試只能提高撞上的機率，
//         永遠無法證明不存在 —— 而且 UB 的表現之一就是「看起來完全正常」。
//         正確的方式是：
//         ① ThreadSanitizer：為每個記憶體位置維護 shadow memory 與向量時鐘，
//            直接判定兩次存取之間有沒有 happens-before 關係，
//            只要執行過一次衝突路徑就報警
//         ② 人工推理：對照五大競爭模式檢查清單
//         兩者互補 —— TSan 只看得到「實際執行到」的路徑，
//         所以效果取決於測試涵蓋率。
//     追問：TSan 沒報錯代表安全嗎?
//         → 不代表，只代表「這次執行到的路徑」乾淨。
//           沒被測試涵蓋的分支它一樣看不到。
//
// ⚠️ 陷阱. 「我把所有共享變數都改成 std::atomic 了，現在應該完全沒有並行問題」——錯在哪?
//     答：atomic 消除了 data race（不再是 UB），但完全沒有消除 race condition。
//         本階段 4.2-3 的實測就證明了這點：
//         把 accountA、accountB 都改成 atomic 之後，
//         稽核端「先原子讀 A、再原子讀 B」仍然讀到總額不等於 2000 的組合
//         —— 而且次數穩定大於 0（因為沒有 UB，這個觀察是有意義的）。
//     為什麼會錯：把 atomic 當成「加上去就執行緒安全」的萬用貼紙。
//         atomic 保證的是【單一操作】的不可分割，
//         而絕大多數並行 bug 出在【多個操作之間】：
//         check 與 act 之間、讀 A 與讀 B 之間、寫欄位 1 與寫欄位 2 之間。
//         要讓「一段操作」成為原子單位，只有鎖（或把整段壓縮成一次 CAS）。
// ═══════════════════════════════════════════════════════════════════════════

/*
 * ============================================================
 * 【第四階段：共享資料與競爭條件】總複習 summary.cpp
 * ============================================================
 *
 * 編譯指令：g++ -std=c++17 -pthread -o summary summary.cpp
 * （如需 ThreadSanitizer：g++ -std=c++17 -fsanitize=thread -g -pthread -o summary summary.cpp）
 *
 * 本階段涵蓋課程：
 * - 課程 4.1：共享資料的問題
 * - 課程 4.2：不變量與競爭條件
 * - 課程 4.3：臨界區段概念
 * - 課程 4.4：資料競爭範例分析
 * - 課程 4.5：競爭條件的檢測
 *
 * ============================================================
 * 重點摘要：
 *
 * 1. 共享資料的問題：
 *    - 全域變數、靜態變數、堆積上的共享物件都可能成為競爭來源
 *    - 多執行緒同時「讀取」是安全的；只要有「寫入」就危險
 *    - ++counter 看似一步，實際是「讀取 → 修改 → 寫回」三步，可被中斷
 *    - 結果：未定義行為 (Undefined Behavior)，每次執行結果不同
 *
 * 2. 不變量與競爭條件：
 *    - 不變量（Invariant）：資料結構在任何「可觀察」時刻必須成立的條件
 *    - 複合操作會暫時破壞不變量；單執行緒沒問題，多執行緒就危險
 *    - 競爭條件的本質：某個執行緒看到了「不變量被破壞的中間狀態」
 *
 * 3. 臨界區段：
 *    - 定義：存取共享資源的程式碼區段
 *    - 同一時間只能有一個執行緒執行臨界區段
 *    - 原則：最小化原則、快進快出、不做耗時操作、避免巢狀
 *    - 區域變數和 thread_local 變數不是共享資源，不需要保護
 *
 * 4. 常見的資料競爭模式：
 *    - Check-Then-Act：檢查和行動之間狀態可能被改變
 *    - Read-Modify-Write：如 counter++ 這類複合操作
 *    - 迭代器失效：修改容器時其他執行緒的迭代器可能失效
 *    - 部分更新：物件的多個欄位更新不是原子的
 *
 * 5. 競爭條件的檢測：
 *    - ThreadSanitizer (TSan)：編譯時加 -fsanitize=thread -g
 *    - Helgrind (Valgrind)：不需重新編譯，但較慢
 *    - 壓力測試：多次重複執行增加發現機率
 *    - 人工審查：尋找 Check-Then-Act、RMW、多個相關變數存取等模式
 *
 * ============================================================
 */

#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <map>
#include <string>
#include <atomic>
#include <chrono>
#include <random>
#include <condition_variable>
#include <functional>

// ============================================================
// ===== 課程 4.1：共享資料的問題 =====
// ============================================================
//
// 核心問題：多執行緒同時存取共享資料，且至少有一個寫入操作，
// 就會產生「資料競爭」（Data Race），導致未定義行為。
//
// ++counter 的真實步驟（非原子！）：
//   1. 讀取：從記憶體讀取 counter 的值到 CPU 暫存器
//   2. 修改：在暫存器中將值 +1
//   3. 寫入：將暫存器的值寫回記憶體
//
// 交錯執行的災難範例：
//
//   時間  執行緒A               執行緒B              counter
//   ─────────────────────────────────────────────────────────
//    1   讀取 counter(0)                              0
//    2                          讀取 counter(0)       0
//    3   +1 得到 1                                    0
//    4                          +1 得到 1             0
//    5   寫回 1                                       1
//    6                          寫回 1                1    ← 兩次++卻只加了1！
//
// 共享資料的類型：
//   - 全域變數 / 靜態變數
//   - 堆積上的物件（透過指標/引用共享）
//   - 類別的靜態成員
//   - 函式內的 static 區域變數（注意！這也是共享資源）
//
// 資料競爭的 C++ 標準定義（三個條件同時滿足）：
//   1. 兩個或多個執行緒同時存取同一記憶體位置
//   2. 至少有一個是寫入操作
//   3. 沒有同步機制保護

namespace lesson_4_1 {

// 示範：無保護的計數器（有資料競爭）
int counter_unsafe = 0;  // 共享資源

void increment_unsafe() {
    for (int i = 0; i < 100000; ++i) {
        ++counter_unsafe;  // 危險！非原子操作（讀 → 改 → 寫）
    }
}

// 示範：唯讀資料（安全）
const int READ_ONLY_DATA = 42;  // 只有讀取，沒有寫入 → 安全

void safe_reader() {
    // 多執行緒同時讀取不可變資料 → 完全安全
    int local = READ_ONLY_DATA;
    (void)local;
}

// 示範：銀行轉帳的資料競爭
struct Account {
    int balance = 1000;
};

Account accountA_unsafe, accountB_unsafe;

void unsafe_transfer(Account& from, Account& to, int amount) {
    if (from.balance >= amount) {
        from.balance -= amount;  // 步驟1：扣款
        // ← 此刻若另一個執行緒讀取 from.balance，會看到不一致狀態！
        to.balance += amount;    // 步驟2：存款
    }
}

void demo_4_1() {
    std::cout << "\n===== 課程 4.1：共享資料的問題 =====\n";

    // 示範：資料競爭導致結果不正確
    counter_unsafe = 0;
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(increment_unsafe);
    }
    for (auto& t : threads) t.join();

    std::cout << "5個執行緒各遞增100000次\n";
    std::cout << "預期值：500000\n";
    std::cout << "實際值：" << counter_unsafe << " （通常小於預期，因為資料競爭）\n";

    // 示範：安全的唯讀存取
    std::thread t1(safe_reader);
    std::thread t2(safe_reader);
    t1.join();
    t2.join();
    std::cout << "唯讀資料多執行緒存取 → 安全！\n";
}

} // namespace lesson_4_1


// ============================================================
// ===== 課程 4.2：不變量與競爭條件 =====
// ============================================================
//
// 不變量（Invariant）的定義：
//   資料結構在任何「可觀察」時刻都必須滿足的條件。
//
// 範例不變量：
//   - 雙向鏈結串列：若 A.next = B，則 B.prev = A
//   - 銀行帳戶：轉帳前後，帳戶A + 帳戶B 的總金額不變
//   - 有序容器：元素始終保持排序順序
//
// 關鍵認知：
//   - 單執行緒下，不變量在操作期間可以「暫時」被破壞，沒有問題
//     因為沒有其他執行緒看到中間狀態
//   - 多執行緒下，另一個執行緒可能在不變量被破壞的「中間狀態」時存取資料
//     這就是競爭條件的危害！
//
// 競爭條件的本質：
//   執行緒A: [不變量成立] → [暫時破壞] → [恢復]
//   執行緒B:                   ↑
//                         在此時讀取 = 看到不一致的資料！

namespace lesson_4_2 {

// 示範：鏈結串列節點
struct Node {
    int data;
    Node* prev = nullptr;
    Node* next = nullptr;
    explicit Node(int d) : data(d) {}
};

// 示範：銀行帳戶不變量被破壞
struct Bank {
    int accountA = 1000;
    int accountB = 1000;
    // 不變量：accountA + accountB == 2000（總金額不變）
    std::mutex mtx;
};

Bank bank_demo;

void unsafe_transfer(int amount) {
    // 不變量暫時被破壞！
    bank_demo.accountA -= amount;
    // ← 此刻 accountA + accountB = 2000 - amount ≠ 2000！
    // 若另一個執行緒在此時讀取，就會看到錯誤的總額
    bank_demo.accountB += amount;
    // 不變量恢復
}

void audit_unsafe() {
    int total = bank_demo.accountA + bank_demo.accountB;
    if (total != 2000) {
        std::cout << "  警告！總額異常: " << total
                  << " （看到了不變量被破壞的中間狀態）\n";
    }
}

// 正確做法：用鎖保護整個複合操作，讓其他執行緒看不到中間狀態
void safe_transfer(int amount) {
    std::lock_guard<std::mutex> lock(bank_demo.mtx);
    bank_demo.accountA -= amount;
    bank_demo.accountB += amount;
    // 不變量始終對外呈現為成立狀態
}

void safe_audit() {
    std::lock_guard<std::mutex> lock(bank_demo.mtx);
    int total = bank_demo.accountA + bank_demo.accountB;
    // 保證永遠是 2000
    (void)total;
}

void demo_4_2() {
    std::cout << "\n===== 課程 4.2：不變量與競爭條件 =====\n";
    std::cout << "不變量：銀行總金額必須始終等於 2000\n";

    // 重置
    bank_demo.accountA = 1000;
    bank_demo.accountB = 1000;

    // 示範不安全版本
    std::thread t1([]{ for (int i = 0; i < 100; ++i) unsafe_transfer(1); });
    std::thread t2([]{ for (int i = 0; i < 100; ++i) audit_unsafe(); });
    t1.join();
    t2.join();

    // 示範安全版本
    bank_demo.accountA = 1000;
    bank_demo.accountB = 1000;
    std::thread t3([]{ for (int i = 0; i < 100; ++i) safe_transfer(1); });
    std::thread t4([]{ for (int i = 0; i < 100; ++i) safe_audit(); });
    t3.join();
    t4.join();
    std::cout << "使用互斥鎖後，不變量始終受到保護\n";
}

} // namespace lesson_4_2


// ============================================================
// ===== 課程 4.3：臨界區段概念 =====
// ============================================================
//
// 臨界區段（Critical Section）的定義：
//   存取共享資源的程式碼區域。
//   同一時間只能有一個執行緒執行臨界區段。
//
// 識別哪些是臨界區段：
//
//   資源類型               是否共享    是否需要保護
//   ─────────────────────────────────────────────
//   全域變數               是          是
//   函式內 static 變數     是          是（容易被忽略！）
//   類別 static 成員       是          是
//   堆積上的共享物件       是          是
//   區域變數（stack）      否          否（每個執行緒有自己的 stack）
//   thread_local 變數      否          否（每個執行緒有獨立副本）
//   函式參數（傳值）        否          否
//   const 全域變數         是          否（只有讀取，安全）
//
// 臨界區段設計四大原則：
//   1. 最小化原則：只保護真正需要保護的程式碼
//   2. 快進快出：在臨界區段內不做耗時操作（I/O、sleep、複雜計算）
//   3. 不要巢狀：避免在臨界區段內進入另一個臨界區段（死結風險）
//   4. 不要等待：不要在臨界區段內等待外部事件
//
// 程式碼標記範例：
//
//   void example(int param) {
//       int local = param;       // 不是臨界區段（只存取區域變數）
//       local += 10;             // 不是臨界區段（只存取區域變數）
//       shared = local;          // 是臨界區段（寫入共享資料）
//       local = shared;          // 是臨界區段（讀取共享資料，且有其他寫入者）
//       int result = local * 2;  // 不是臨界區段（只存取區域變數）
//       shared += result;        // 是臨界區段（讀取 + 寫入共享資料）
//   }

namespace lesson_4_3 {

int sharedData = 0;
std::vector<int> data_vec;
std::mutex mtx;

void worker_critical_section() {
    int localVar = 0;          // 不是臨界區段：區域變數，每個執行緒有自己的
    localVar = 42;             // 不是臨界區段：只存取區域變數

    // 寫入共享資料 → 臨界區段
    {
        std::lock_guard<std::mutex> lock(mtx);
        sharedData = localVar;     // 臨界區段：寫入共享資料
    }

    // 讀取共享資料 → 臨界區段（因為有其他執行緒在寫入）
    int temp;
    {
        std::lock_guard<std::mutex> lock(mtx);
        temp = sharedData;         // 臨界區段：讀取共享資料
    }
    (void)temp;
}

// 示範：臨界區段太大（不好）vs 精確（好）
std::mutex large_mtx;
int shared_result = 0;

int expensive_calc(int value) {
    // 模擬耗時計算（實際不需要鎖！）
    return value * value + value;
}

// 差的做法：臨界區段包含不必要的計算
void bad_critical_section(int value) {
    std::lock_guard<std::mutex> lock(large_mtx);
    // 以下計算完全不需要鎖，卻讓其他執行緒白白等待
    int processed = expensive_calc(value);  // 不需要鎖
    shared_result += processed;              // 需要鎖
}

// 好的做法：精確的臨界區段
void good_critical_section(int value) {
    int processed = expensive_calc(value);  // 在鎖外計算（並行！）
    {
        std::lock_guard<std::mutex> lock(large_mtx);
        shared_result += processed;          // 只保護必要的部分
    }
}

void demo_4_3() {
    std::cout << "\n===== 課程 4.3：臨界區段概念 =====\n";

    // 示範精確臨界區段
    sharedData = 0;
    std::thread t1(worker_critical_section);
    std::thread t2(worker_critical_section);
    t1.join();
    t2.join();
    std::cout << "臨界區段正確保護共享資料，結果：" << sharedData << "\n";

    // 示範好的臨界區段設計
    shared_result = 0;
    std::thread t3([]{ for (int i = 0; i < 5; ++i) good_critical_section(i); });
    std::thread t4([]{ for (int i = 5; i < 10; ++i) good_critical_section(i); });
    t3.join();
    t4.join();
    std::cout << "好的臨界區段設計結果：" << shared_result << "\n";
    std::cout << "原則：只鎖必要的部分，讓計算在鎖外並行執行\n";
}

} // namespace lesson_4_3


// ============================================================
// ===== 課程 4.4：資料競爭範例分析 =====
// ============================================================
//
// 競爭條件的五大模式（警示信號）：
//
// 模式1：Check-Then-Act（最常見）
//   if (condition) { action }
//   → 條件和行動之間，另一個執行緒可能改變狀態
//   → 例：if (cache.find(key) == cache.end()) { cache[key] = ... }
//
// 模式2：Read-Modify-Write
//   counter++, counter += x
//   → 讀取、修改、寫回這三步不是原子的
//   → 兩個執行緒可能讀到相同的舊值，都 +1，但結果只增加了 1
//
// 模式3：複合操作競爭
//   先 check size，再 push_back
//   → check 和 push_back 之間，另一個執行緒可能改變 size
//
// 模式4：迭代器失效
//   遍歷容器時，另一個執行緒修改容器（如 push_back 觸發重新配置）
//   → 所有迭代器失效，繼續使用會是未定義行為
//
// 模式5：物件的部分更新
//   更新物件的多個欄位，讀取者可能看到一半新值一半舊值的不一致狀態

namespace lesson_4_4 {

// 模式1：Check-Then-Act
std::map<int, std::string> cache;
std::mutex cache_mtx;

// 危險版本
std::string unsafe_get_value(int key) {
    if (cache.find(key) == cache.end()) {  // 檢查
        // ← 另一個執行緒可能在此插入相同的 key！
        cache[key] = "computed_" + std::to_string(key);  // 行動
    }
    return cache[key];
}

// 安全版本：把檢查和行動放在同一個臨界區段
std::string safe_get_value(int key) {
    std::lock_guard<std::mutex> lock(cache_mtx);
    if (cache.find(key) == cache.end()) {  // 檢查
        cache[key] = "computed_" + std::to_string(key);  // 行動
    }  // 這兩步是原子的，沒有執行緒能插進來
    return cache[key];
}

// 模式2：Read-Modify-Write
int counter_rmw = 0;
std::mutex counter_mtx;

void unsafe_increment_rmw() {
    for (int i = 0; i < 10000; ++i) {
        counter_rmw++;  // 危險！讀-改-寫 三步分開
    }
}

void safe_increment_rmw() {
    for (int i = 0; i < 10000; ++i) {
        std::lock_guard<std::mutex> lock(counter_mtx);
        counter_rmw++;  // 現在是原子的：鎖保證了三步一起完成
    }
}

// 模式4：迭代器失效
std::vector<int> shared_vec = {1, 2, 3, 4, 5};
std::mutex vec_mtx;

void unsafe_reader_vec() {
    // 危險！遍歷時另一個執行緒可能 push_back 導致重新配置
    for (auto it = shared_vec.begin(); it != shared_vec.end(); ++it) {
        // ← 迭代器可能已失效！
        (void)*it;
    }
}

void unsafe_writer_vec() {
    shared_vec.push_back(6);  // 可能觸發重新配置，使所有迭代器失效
}

// 模式5：物件部分更新
struct Person {
    std::string firstName;
    std::string lastName;
    int age;
};

Person person_shared{"John", "Doe", 30};
std::mutex person_mtx;

void unsafe_writer_person() {
    person_shared.firstName = "Jane";  // 步驟1
    // ← 此刻讀取者看到 "Jane Doe 30"，firstName 和 lastName 不一致！
    person_shared.lastName = "Smith";  // 步驟2
    person_shared.age = 25;            // 步驟3
}

void safe_writer_person() {
    std::lock_guard<std::mutex> lock(person_mtx);
    person_shared.firstName = "Jane";   // 三個步驟在鎖的保護下一起完成
    person_shared.lastName = "Smith";
    person_shared.age = 25;
}

void demo_4_4() {
    std::cout << "\n===== 課程 4.4：資料競爭範例分析 =====\n";
    std::cout << "五大競爭條件模式：\n";
    std::cout << "  1. Check-Then-Act：檢查和行動之間狀態可能被改變\n";
    std::cout << "  2. Read-Modify-Write：如 counter++ 不是原子操作\n";
    std::cout << "  3. 複合操作：多個相關操作必須一起保護\n";
    std::cout << "  4. 迭代器失效：修改容器時迭代器可能失效\n";
    std::cout << "  5. 部分更新：物件可能處於不一致的中間狀態\n";

    // 示範安全版本的 Check-Then-Act
    cache.clear();
    std::thread t1([]{ safe_get_value(1); });
    std::thread t2([]{ safe_get_value(1); });
    t1.join();
    t2.join();
    std::cout << "安全的 Check-Then-Act，cache[1] = " << cache[1] << "\n";

    // 示範安全版本的 RMW
    counter_rmw = 0;
    std::thread t3(safe_increment_rmw);
    std::thread t4(safe_increment_rmw);
    t3.join();
    t4.join();
    std::cout << "安全的 RMW 結果：" << counter_rmw << "（預期：20000）\n";
}

} // namespace lesson_4_4


// ============================================================
// ===== 課程 4.5：競爭條件的檢測 =====
// ============================================================
//
// 競爭條件為何難以除錯：
//   1. 非確定性：同樣的程式，每次執行結果可能不同
//   2. 時機敏感：只在特定的執行緒交錯時機發生
//   3. 難以重現：加上 printf 除錯可能改變時序，問題消失（Heisenbug）
//   4. 環境依賴：在某台機器正常，換台機器就出錯
//
// 檢測工具：
//
//   動態分析（執行時期檢測）：
//   - ThreadSanitizer (TSan)：最常用，需重新編譯
//     編譯：g++ -fsanitize=thread -g -o program program.cpp -pthread
//     執行速度慢 5-15 倍，記憶體增加 5-10 倍
//
//   - Helgrind (Valgrind)：不需重新編譯，但慢 20-100 倍
//     使用：valgrind --tool=helgrind ./program
//
//   靜態分析：
//   - Clang Static Analyzer / Coverity / PVS-Studio
//
// TSan 輸出範例（讀懂報告很重要）：
//
//   WARNING: ThreadSanitizer: data race (pid=12345)
//     Write of size 4 at 0x000000601040 by thread T2:
//       #0 increment() race.cpp:7         ← 第二個執行緒的寫入位置
//     Previous write of size 4 at 0x000000601040 by thread T1:
//       #0 increment() race.cpp:7         ← 第一個執行緒的寫入位置
//     Location is global 'counter' of size 4 at 0x000000601040
//                   ↑ 問題變數是全域 counter
//
// TSan 報告類型：
//   - data race：兩個執行緒同時存取，至少一個寫入
//   - thread leak：執行緒結束前未 join 或 detach
//   - lock-order-inversion：鎖的獲取順序不一致，可能死結
//   - use of uninitialized mutex：使用未初始化的互斥鎖
//
// TSan 的限制：
//   - 只能檢測實際執行到的程式碼路徑
//   - 需要足夠的測試覆蓋率
//   - 僅用於開發和測試，不用於生產環境
//
// 手動檢測技巧：
//   - 插入延遲：在可疑位置插入 sleep，增加競爭發生機率
//   - 壓力測試：大量重複執行，增加發現問題的機率
//   - 代碼審查：尋找 Check-Then-Act、RMW、多個相關變數存取模式
//
// 競爭條件檢測清單：
//   □ 使用 TSan 編譯並執行測試
//   □ 確保測試覆蓋多執行緒路徑
//   □ 進行壓力測試（多次重複執行）
//   □ 審查所有共享變數的存取
//   □ 檢查 Check-Then-Act 模式
//   □ 檢查 Read-Modify-Write 操作
//   □ 確認所有複合操作都有適當保護

namespace lesson_4_5 {

// 示範：壓力測試技術
int stress_counter = 0;
std::mutex stress_mtx;

// 有競爭條件的版本（用於示範）
void unsafe_increment_stress() {
    for (int i = 0; i < 1000; ++i) {
        stress_counter++;  // 競爭條件！
    }
}

// 正確版本
void safe_increment_stress() {
    for (int i = 0; i < 1000; ++i) {
        std::lock_guard<std::mutex> lock(stress_mtx);
        stress_counter++;
    }
}

// 壓力測試框架
void stress_test_safe(int numTrials, int numThreads) {
    int failures = 0;
    for (int trial = 0; trial < numTrials; ++trial) {
        stress_counter = 0;
        std::vector<std::thread> threads;
        for (int i = 0; i < numThreads; ++i) {
            threads.emplace_back(safe_increment_stress);
        }
        for (auto& t : threads) t.join();

        if (stress_counter != numThreads * 1000) {
            ++failures;
        }
    }
    std::cout << "安全版本壓力測試：" << numTrials << " 次試驗，"
              << failures << " 次失敗（應為 0）\n";
}

// 示範：在可疑位置插入延遲來增加競爭機率
void suspicious_function(int& shared, std::mutex& mtx) {
    if (mtx.try_lock()) {
        shared++;
        // 在可疑位置插入短暫延遲，放大競爭窗口用於除錯
        std::this_thread::sleep_for(std::chrono::microseconds(1));
        mtx.unlock();
    }
}

void demo_4_5() {
    std::cout << "\n===== 課程 4.5：競爭條件的檢測 =====\n";
    std::cout << "TSan 編譯：g++ -std=c++17 -fsanitize=thread -g -pthread -o prog prog.cpp\n";
    std::cout << "Helgrind：valgrind --tool=helgrind ./program\n\n";

    // 示範壓力測試
    stress_test_safe(10, 4);

    std::cout << "提示：\n";
    std::cout << "  - 競爭條件通常在高負載下才出現\n";
    std::cout << "  - TSan 是最有效的自動化工具\n";
    std::cout << "  - 代碼審查中尋找 Check-Then-Act 和 RMW 模式\n";
}

} // namespace lesson_4_5


// ============================================================
// ===== 解決方案預覽 =====
// ============================================================
//
// 本階段展示了問題，第五階段開始學習解決方案：
//
//   1. 互斥鎖（Mutex）             → 第五階段
//      確保同一時間只有一個執行緒存取資料
//
//   2. RAII 鎖管理器               → 第六階段
//      lock_guard / unique_lock / scoped_lock
//
//   3. 條件變數                    → 第七階段
//      執行緒間的協調與通知
//
//   4. 原子操作（Atomic）          → 第二十階段
//      使用硬體支援的不可分割操作
//
//   5. 避免共享                    → 設計層面
//      每個執行緒使用自己的資料副本（thread_local）
//
//   6. 不可變資料                  → 設計層面
//      資料建立後不再修改（const）


// ============================================================
// ===== 【LeetCode 實戰範例】LeetCode 1116. Print Zero Even Odd =====
// ============================================================
//   三條執行緒協同輸出 "0102030405..."：
//     zero() 印 n 個 0，每個數字前一個
//     even() 印偶數，odd() 印奇數
//   核心：「判斷輪到誰 + 印出 + 推進狀態」必須是一個不可分割的步驟，
//         而且順序要靠 condition_variable（mutex 只保證互斥，不保證順序）。
// ============================================================
class ZeroEvenOdd {
private:
    // 【注意】成員初始化順序依「宣告順序」，與初始化列表順序無關。
    std::mutex mtx;
    std::condition_variable cv;
    int n;
    int current = 1;      // 下一個要印的數字
    int turn = 0;         // 0 = 該印 0，1 = 該印奇數，2 = 該印偶數

public:
    explicit ZeroEvenOdd(int nn) : n(nn) {}

    void zero(const std::function<void(int)>& printNumber) {
        for (int i = 0; i < n; ++i) {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return turn == 0; });
            printNumber(0);
            // 印完 0 之後，依 current 的奇偶決定下一棒是誰
            turn = (current % 2 == 1) ? 1 : 2;
            cv.notify_all();
        }
    }

    void odd(const std::function<void(int)>& printNumber) {
        for (int i = 1; i <= n; i += 2) {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return turn == 1; });
            printNumber(current);      // 判斷、印出、推進 —— 都在同一個臨界區段
            ++current;
            turn = 0;
            cv.notify_all();
        }
    }

    void even(const std::function<void(int)>& printNumber) {
        for (int i = 2; i <= n; i += 2) {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return turn == 2; });
            printNumber(current);
            ++current;
            turn = 0;
            cv.notify_all();
        }
    }
};

// ============================================================
// ===== 【日常實務範例 1】訂單處理管線：五大模式的綜合防護 =====
// ============================================================
//   情境：電商下單流程要同時做四件事 ——
//         檢查庫存（check-then-act）、扣庫存（RMW）、
//         寫入訂單狀態的多個欄位（部分更新）、更新統計（RMW）。
//   這四件事的不變量橫跨多個變數，所以【必須】用一把鎖涵蓋整個交易，
//   而不是各自用 atomic。這正是第四階段所有教訓的綜合應用。
// ============================================================
struct OrderRecord {
    long orderId;
    int quantity;
    long createdAtMs;
    bool valid;
};

class OrderPipeline {
private:
    mutable std::mutex mtx;
    int stock;
    long nextOrderId = 1;
    long soldUnits = 0;
    std::vector<OrderRecord> orders;

public:
    explicit OrderPipeline(int initialStock) : stock(initialStock) {
        orders.reserve(2048);
    }

    // 整個下單流程是【一個】不可分割的交易
    bool placeOrder(int qty, long nowMs) {
        std::lock_guard<std::mutex> lock(mtx);

        if (stock < qty) return false;      // ① check
        stock -= qty;                       // ② act（RMW）
        soldUnits += qty;                   // ③ 另一個 RMW

        OrderRecord rec;                    // ④ 多欄位一起寫（避免部分更新）
        rec.orderId = nextOrderId++;
        rec.quantity = qty;
        rec.createdAtMs = nowMs;
        rec.valid = true;
        orders.push_back(rec);
        return true;
    }

    // 不變量：剩餘庫存 + 已售出 == 初始庫存，且訂單數量總和 == 已售出
    bool invariantHolds(int initialStock) const {
        std::lock_guard<std::mutex> lock(mtx);
        if (stock + soldUnits != initialStock) return false;
        long sum = 0;
        for (const auto& o : orders) sum += o.quantity;
        return sum == soldUnits;
    }

    int remaining() const { std::lock_guard<std::mutex> lock(mtx); return stock; }
    long sold() const     { std::lock_guard<std::mutex> lock(mtx); return soldUnits; }
    size_t orderCount() const { std::lock_guard<std::mutex> lock(mtx); return orders.size(); }
};

// ============================================================
// ===== 【日常實務範例 2】決策樹的實踐：三種同步策略的對照 =====
// ============================================================
//   同一個「統計請求數」的需求，用三種策略實作：
//     ① mutex        —— 通用，但每次都要上鎖
//     ② atomic       —— 單一變數的 RMW，硬體直接支援
//     ③ 本地累加     —— 完全不共享，最後合併一次
//   三者結果必定相同；差別在於同步的次數與成本。
//   這示範了「先判斷不變量橫跨幾個變數，再選工具」的決策流程。
// ============================================================
struct CounterStrategies {
    // ① mutex
    std::mutex mtx;
    long byMutex = 0;

    // ② atomic
    std::atomic<long> byAtomic{0};

    // ③ 本地累加後合併
    std::mutex mergeMtx;
    long byLocalMerge = 0;
};

// ============================================================
// ===== main() 函式：示範最重要的概念 =====
// ============================================================

int main() {
    std::cout << "============================================================\n";
    std::cout << " 第四階段：共享資料與競爭條件 - 總複習\n";
    std::cout << "============================================================\n";

    // 執行各課程的示範
    lesson_4_1::demo_4_1();
    lesson_4_2::demo_4_2();
    lesson_4_3::demo_4_3();
    lesson_4_4::demo_4_4();
    lesson_4_5::demo_4_5();

    // ─── 綜合示範：使用互斥鎖解決計數器問題 ───
    std::cout << "\n===== 綜合示範：用互斥鎖解決資料競爭 =====\n";

    {
        int safe_counter = 0;
        std::mutex mtx;

        auto safe_inc = [&]() {
            for (int i = 0; i < 100000; ++i) {
                std::lock_guard<std::mutex> lock(mtx);
                ++safe_counter;
            }
        };

        std::vector<std::thread> threads;
        for (int i = 0; i < 5; ++i) {
            threads.emplace_back(safe_inc);
        }
        for (auto& t : threads) t.join();

        std::cout << "使用互斥鎖後：預期 500000，實際 " << safe_counter
                  << " " << (safe_counter == 500000 ? "[正確]" : "[錯誤]") << "\n";
    }

    // ─── 關鍵重點總結 ───
    std::cout << "\n============================================================\n";
    std::cout << " 第四階段關鍵重點總結\n";
    std::cout << "============================================================\n";
    std::cout << "1. 共享資料 + 寫入操作 = 資料競爭風險\n";
    std::cout << "2. counter++ 不是原子操作（讀 → 改 → 寫）\n";
    std::cout << "3. 不變量：資料結構必須在任何可觀察時刻保持成立的條件\n";
    std::cout << "4. 競爭條件：某執行緒看到了不變量被破壞的中間狀態\n";
    std::cout << "5. 臨界區段：存取共享資源的程式碼，應盡量短小\n";
    std::cout << "6. Check-Then-Act 和 Read-Modify-Write 是常見的競爭模式\n";
    std::cout << "7. 使用 TSan (-fsanitize=thread) 可有效檢測競爭條件\n";
    std::cout << "8. 解決方案：互斥鎖、原子操作、避免共享、不可變資料\n";
    std::cout << "============================================================\n";

    // ─── LeetCode 1116. Print Zero Even Odd ───
    std::cout << "\n===== LeetCode 1116. Print Zero Even Odd (n = 5) =====\n";
    {
        ZeroEvenOdd zeo(5);
        std::string out;
        std::mutex outMtx;
        auto emit = [&out, &outMtx](int x) {
            std::lock_guard<std::mutex> lock(outMtx);
            out += std::to_string(x);
        };

        // 刻意用「反過來」的順序啟動，證明順序由 condition_variable 保證
        std::thread te([&] { zeo.even(emit); });
        std::thread to([&] { zeo.odd(emit); });
        std::thread tz([&] { zeo.zero(emit); });
        te.join();
        to.join();
        tz.join();

        std::cout << out << "\n";
        std::cout << "→ 即使以 even-odd-zero 的順序啟動執行緒，輸出仍必定是 0102030405\n";
        std::cout << "  mutex 保證互斥（不會同時進入），condition_variable 保證順序\n";
    }

    // ─── 日常實務 1：訂單處理管線 ───
    std::cout << "\n===== 日常實務 1：訂單處理管線（五大模式的綜合防護）=====\n";
    {
        const int initialStock = 5000;
        OrderPipeline pipeline(initialStock);
        std::vector<std::thread> threads;
        std::atomic<int> accepted{0};

        for (int i = 0; i < 8; ++i) {
            threads.emplace_back([&pipeline, &accepted, i] {
                int local = 0;
                for (int k = 0; k < 1000; ++k) {
                    if (pipeline.placeOrder(1, 1700000000000L + i * 1000 + k)) ++local;
                }
                accepted.fetch_add(local);
            });
        }
        for (auto& t : threads) t.join();

        std::cout << "8 執行緒共嘗試下單 8000 次，庫存 " << initialStock << "\n";
        std::cout << "成功下單: " << accepted.load() << " 筆（必定 = 5000，不會超賣）\n";
        std::cout << "剩餘庫存: " << pipeline.remaining() << "（必定 = 0）\n";
        std::cout << "訂單筆數: " << pipeline.orderCount() << "（必定 = 5000）\n";
        std::cout << "不變量成立: " << (pipeline.invariantHolds(initialStock) ? "是" : "否")
                  << "（庫存 + 已售 = 初始，且訂單數量總和 = 已售）\n";
        std::cout << "→ 整個下單流程是一個交易：check-then-act、兩個 RMW、\n";
        std::cout << "  多欄位寫入全部在同一個臨界區段內。\n";
    }

    // ─── 日常實務 2：三種同步策略的對照 ───
    std::cout << "\n===== 日常實務 2：三種同步策略（結果相同，成本不同）=====\n";
    {
        CounterStrategies cs;
        const int threads = 8, per = 100000;

        // ① mutex：每次都上鎖
        {
            std::vector<std::thread> ths;
            for (int i = 0; i < threads; ++i) {
                ths.emplace_back([&cs, per] {
                    for (int k = 0; k < per; ++k) {
                        std::lock_guard<std::mutex> lock(cs.mtx);
                        ++cs.byMutex;
                    }
                });
            }
            for (auto& t : ths) t.join();
        }

        // ② atomic：單一變數的 RMW
        {
            std::vector<std::thread> ths;
            for (int i = 0; i < threads; ++i) {
                ths.emplace_back([&cs, per] {
                    for (int k = 0; k < per; ++k)
                        cs.byAtomic.fetch_add(1, std::memory_order_relaxed);
                });
            }
            for (auto& t : ths) t.join();
        }

        // ③ 本地累加：完全不共享，最後合併一次
        {
            std::vector<std::thread> ths;
            for (int i = 0; i < threads; ++i) {
                ths.emplace_back([&cs, per] {
                    long local = 0;
                    for (int k = 0; k < per; ++k) ++local;
                    std::lock_guard<std::mutex> lock(cs.mergeMtx);
                    cs.byLocalMerge += local;
                });
            }
            for (auto& t : ths) t.join();
        }

        long expected = static_cast<long>(threads) * per;
        std::cout << "8 執行緒 × 100000 次遞增，預期 " << expected << "\n";
        std::cout << "① mutex      = " << cs.byMutex        << "（上鎖 800000 次）\n";
        std::cout << "② atomic     = " << cs.byAtomic.load() << "（原子操作 800000 次）\n";
        std::cout << "③ 本地累加   = " << cs.byLocalMerge   << "（上鎖 8 次）\n";
        std::cout << "三者一致: "
                  << ((cs.byMutex == expected && cs.byAtomic.load() == expected
                       && cs.byLocalMerge == expected) ? "是" : "否") << "\n";
        std::cout << "→ 決策流程：不變量只涉及一個變數且操作可壓成單一原子指令 → atomic；\n";
        std::cout << "  橫跨多個變數 → 必須用 mutex；資料天生獨立 → 本地累加最快。\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread summary.cpp -o summary
//
// 偵測資料競爭（本檔的 4.1 示範刻意保留 data race，其餘皆有保護）:
//   g++ -std=c++17 -fsanitize=thread -g -pthread summary.cpp -o summary_tsan

// ⚠️ 兩處內容【每次執行都不同】，下面貼的是本機某一次的真實實測：
//   (1) 課程 4.1 段落的「實際值」——那是刻意保留的 genuine data race → UB，
//       每次都不同，且【不保證】一定小於預期值（可能剛好等於 500000）。
//   (2) 課程 4.5 段落的壓力測試輪次描述，若排程不同也可能有差異。
//
// 其餘全部是確定值（每次執行都相同）：
//   * 互斥鎖版本的 500000 [正確]
//   * LeetCode 1116 的 0102030405（由 condition_variable 保證，與啟動順序無關）
//   * 訂單管線的 5000 / 0 / 5000 與不變量成立
//   * 三種同步策略皆為 800000、三者一致

// === 預期輸出 ===
// ============================================================
//  第四階段：共享資料與競爭條件 - 總複習
// ============================================================
//
// ===== 課程 4.1：共享資料的問題 =====
// 5個執行緒各遞增100000次
// 預期值：500000
// 實際值：120274 （通常小於預期，因為資料競爭）
// 唯讀資料多執行緒存取 → 安全！
//
// ===== 課程 4.2：不變量與競爭條件 =====
// 不變量：銀行總金額必須始終等於 2000
// 使用互斥鎖後，不變量始終受到保護
//
// ===== 課程 4.3：臨界區段概念 =====
// 臨界區段正確保護共享資料，結果：42
// 好的臨界區段設計結果：330
// 原則：只鎖必要的部分，讓計算在鎖外並行執行
//
// ===== 課程 4.4：資料競爭範例分析 =====
// 五大競爭條件模式：
//   1. Check-Then-Act：檢查和行動之間狀態可能被改變
//   2. Read-Modify-Write：如 counter++ 不是原子操作
//   3. 複合操作：多個相關操作必須一起保護
//   4. 迭代器失效：修改容器時迭代器可能失效
//   5. 部分更新：物件可能處於不一致的中間狀態
// 安全的 Check-Then-Act，cache[1] = computed_1
// 安全的 RMW 結果：20000（預期：20000）
//
// ===== 課程 4.5：競爭條件的檢測 =====
// TSan 編譯：g++ -std=c++17 -fsanitize=thread -g -pthread -o prog prog.cpp
// Helgrind：valgrind --tool=helgrind ./program
//
// 安全版本壓力測試：10 次試驗，0 次失敗（應為 0）
// 提示：
//   - 競爭條件通常在高負載下才出現
//   - TSan 是最有效的自動化工具
//   - 代碼審查中尋找 Check-Then-Act 和 RMW 模式
//
// ===== 綜合示範：用互斥鎖解決資料競爭 =====
// 使用互斥鎖後：預期 500000，實際 500000 [正確]
//
// ============================================================
//  第四階段關鍵重點總結
// ============================================================
// 1. 共享資料 + 寫入操作 = 資料競爭風險
// 2. counter++ 不是原子操作（讀 → 改 → 寫）
// 3. 不變量：資料結構必須在任何可觀察時刻保持成立的條件
// 4. 競爭條件：某執行緒看到了不變量被破壞的中間狀態
// 5. 臨界區段：存取共享資源的程式碼，應盡量短小
// 6. Check-Then-Act 和 Read-Modify-Write 是常見的競爭模式
// 7. 使用 TSan (-fsanitize=thread) 可有效檢測競爭條件
// 8. 解決方案：互斥鎖、原子操作、避免共享、不可變資料
// ============================================================
//
// ===== LeetCode 1116. Print Zero Even Odd (n = 5) =====
// 0102030405
// → 即使以 even-odd-zero 的順序啟動執行緒，輸出仍必定是 0102030405
//   mutex 保證互斥（不會同時進入），condition_variable 保證順序
//
// ===== 日常實務 1：訂單處理管線（五大模式的綜合防護）=====
// 8 執行緒共嘗試下單 8000 次，庫存 5000
// 成功下單: 5000 筆（必定 = 5000，不會超賣）
// 剩餘庫存: 0（必定 = 0）
// 訂單筆數: 5000（必定 = 5000）
// 不變量成立: 是（庫存 + 已售 = 初始，且訂單數量總和 = 已售）
// → 整個下單流程是一個交易：check-then-act、兩個 RMW、
//   多欄位寫入全部在同一個臨界區段內。
//
// ===== 日常實務 2：三種同步策略（結果相同，成本不同）=====
// 8 執行緒 × 100000 次遞增，預期 800000
// ① mutex      = 800000（上鎖 800000 次）
// ② atomic     = 800000（原子操作 800000 次）
// ③ 本地累加   = 800000（上鎖 8 次）
// 三者一致: 是
// → 決策流程：不變量只涉及一個變數且操作可壓成單一原子指令 → atomic；
//   橫跨多個變數 → 必須用 mutex；資料天生獨立 → 本地累加最快。
