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

    return 0;
}
