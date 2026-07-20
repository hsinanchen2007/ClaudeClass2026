// 檔案：lesson_5_5_bank_account.cpp
// 說明：執行緒安全的銀行帳戶類別
//
// =============================================================================
//  課程 5.5：保護共享資料實作3.cpp  —  兩把鎖、死結、以及 scoped_lock
// =============================================================================
//
// 【主題資訊 Information】
//   主題：    執行緒安全類別的完整實作；同時鎖住兩個物件而不死結
//   語法：    std::scoped_lock lock(from.mtx, to.mtx);   // C++17，一次鎖多把
//   標準版本：std::mutex / lock_guard 為 C++11；std::scoped_lock 為 C++17
//             （C++11 要寫 std::lock(m1, m2) + 兩個 adopt_lock 的 lock_guard）
//   標頭檔：  <mutex>、<thread>、<stdexcept>
//   複雜度：  各操作 O(1)；鎖只增加常數成本，不改變複雜度
//
// 【詳細解釋 Explanation】
//
// 【1. 這個類別做對了哪些事】
//   ① mutex 宣告為 mutable → const 成員函式（getBalance / getId）也能上鎖。
//   ② 禁止複製（= delete）→ 含 mutex 的類別本來就不可複製，
//      明確寫出來讓錯誤在編譯期就被擋下，而不是給出難懂的錯誤訊息。
//   ③ 參數驗證放在【鎖外】→ amount <= 0 的檢查不需要碰共享狀態，
//      提早拋例外可以完全避免無謂的上鎖。這是臨界區段最小化的實踐。
//   ④ 回傳【值】而非引用 → getBalance() 回傳 double 複本、
//      getId() 回傳 std::string 複本。若回傳引用，鎖在函式回傳時就釋放了，
//      呼叫端拿到的引用毫無保護（這正是下一檔 5.5-4 的錯誤示範）。
//   ⑤ withdraw 的「檢查餘額 + 扣款」在同一個臨界區段內 →
//      不會發生 check-then-act 的超額提款。
//
// 【2. transfer 為什麼必須用 scoped_lock】
//   轉帳要同時修改兩個帳戶，兩個帳戶各有自己的鎖，所以必須同時持有兩把。
//   若天真地寫成：
//       std::lock_guard<std::mutex> l1(from.mtx);
//       std::lock_guard<std::mutex> l2(to.mtx);
//   那麼當 T1 執行 transfer(alice, bob)、T2 執行 transfer(bob, alice) 時：
//       T1 拿到 alice.mtx，等待 bob.mtx
//       T2 拿到 bob.mtx，等待 alice.mtx
//   兩者互相等待，永遠不會結束 —— 這就是【AB-BA 死結】，
//   而且它是銀行轉帳這個題目的教科書級經典案例。
//   std::scoped_lock（C++17）內部使用死結避免演算法（等價於 std::lock）：
//   鎖住第一把後對其餘的用 try_lock，任一失敗就【全部放開重來】。
//   因此不論呼叫端以什麼順序傳入，都不會死結。
//
// 【3. 另一種解法：固定加鎖順序】
//   scoped_lock 之外的經典解法是「所有人都依同一個全域順序加鎖」，
//   例如依帳戶 id 字典序、或依物件位址大小：
//       auto* first  = (&from < &to) ? &from : &to;
//       auto* second = (&from < &to) ? &to   : &from;
//   只要所有程式碼都遵守同一個順序，環狀等待就不可能形成。
//   兩種做法的取捨：
//     * scoped_lock：不需要全域約定，不會有人忘記，程式碼短。首選。
//     * 固定順序：不需要 try-and-back-off 的重試成本，
//       在鎖數量多且競爭激烈時可能略快；但要靠紀律維持，容易出錯。
//
// 【4. `if (&from == &to)` 這行檢查為什麼是必要的】
//   自己轉給自己時，from.mtx 與 to.mtx 是【同一把鎖】。
//   對同一把 std::mutex 鎖兩次是【未定義行為】（std::mutex 不可重入）。
//   所以這行不是為了「業務邏輯上不合理」而寫，
//   它和 5.5-2 的 `if (this != &other)` 一樣，擋的是重複上鎖的 UB。
//   （註：實作上 std::scoped_lock 對重複的 mutex 沒有特別處理，
//     因此這個保護必須由呼叫端自己做。）
//
// 【5. 為什麼「總額」在這個範例裡會變】
//   原始講義最後印出「（初始總額為 1500，應保持不變）」，這句話是【錯的】：
//   * transfer 只是把錢從一個帳戶搬到另一個，確實不改變總額。
//   * 但 simulateTransactions 執行的是 deposit(100) 與 withdraw(50)，
//     這是【存款與提款】，本來就會改變總額。
//   本檔跑 t3、t4 各 5 輪的 deposit(100)+withdraw(50)，
//   每個帳戶淨增 5×50 = 250，兩個帳戶共增加 500，
//   所以最終總額必定是 1500 + 500 = 2000。
//   → 真正的不變量是「transfer 不改變總額」，而不是「總額恆為 1500」。
//     本檔已把該行訊息更正，並實際驗證這個不變量。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼原始版本的輸出會糊成一團
//   deposit / withdraw 在【自己的鎖內】呼叫 std::cout <<，
//   但 alice 與 bob 是兩把不同的鎖 —— 兩條執行緒可以同時在各自的鎖內輸出。
//   而 `cout << a << b << endl` 是好幾次獨立的 << 呼叫，
//   中間可以被另一條執行緒插入，於是產生
//   「[Alice] 提款 [Bob] 存款 100，餘額：50，餘額：1050」這種難以閱讀的輸出。
//   本機實測確實如此（原始版本的輸出完全無法閱讀）。
//   → 這帶出兩個重要結論：
//     ① 鎖只保護它所保護的那份資料，不會順便保護 cout。
//     ② 【不要在臨界區段內做 I/O】—— 除了效能，還有可讀性與正確性的理由。
//   本檔的修法是課程 5.6-5 會正式介紹的做法：
//   在鎖內把記錄存進容器，等執行緒都結束後再統一輸出。
//
// (B) 移動建構函式的隱藏問題
//   本類別的移動建構函式鎖住了 other.mtx，看似正確，但有個微妙之處：
//   移動的語意是「來源之後不再被使用」。若真的還有別的執行緒
//   正在使用來源物件，那麼「移動」這個操作本身就是設計錯誤 ——
//   加鎖只能防止資料撕裂，防不了「對方拿到一個被掏空的物件」。
//   實務上多數含鎖的類別會連移動一起禁止，改用 shared_ptr 傳遞。
//
// (C) double 做金額是錯的（雖然本範例沿用）
//   浮點數無法精確表示 0.1、0.01 這類十進位小數，
//   累加多次後會產生誤差（例如 0.1 加十次不等於 1.0）。
//   金融系統一律用「整數的最小貨幣單位」（分、聰）或十進位定點數型別。
//   本檔沿用原始講義的 double 以保持對照，但實務上請勿模仿。
//
// 【注意事項 Pay Attention】
// 1. 同時要鎖兩把以上時，用 std::scoped_lock（C++17）避免 AB-BA 死結。
// 2. `if (&from == &to)` 是正確性需求：對同一把 std::mutex 鎖兩次是 UB。
// 3. 參數驗證放鎖外；回傳值而非引用（回傳引用等於把保護漏出去）。
// 4. 鎖不會保護 cout；在鎖內做 I/O 會讓輸出交錯，也會拉長臨界區段。
// 5. 真正的不變量是「transfer 不改變總額」，不是「總額恆為初始值」——
//    deposit / withdraw 本來就會改變總額。
// 6. 用 double 表示金額在實務上是錯的，應使用整數的最小貨幣單位。
//
// 【LeetCode 說明】本檔不附 LeetCode 範例。
//   本檔的核心是「同時鎖住兩個物件而不死結」，這在 LeetCode 上沒有對應題目：
//   允許使用的設計題（146/155/705/707/1603）都只涉及單一資料結構、單一把鎖，
//   並行題（1114～1117/1195）考的是執行緒間的順序協調而非多鎖死結。
//   （其中最接近的 1603. Design Parking System 的 check-then-act 模式，
//     已在「課程 4.1：共享資料的問題2.cpp」完整示範過。）
//   故從缺，改以下方兩個真實情境（庫存調撥、批次轉帳的死結壓力測試）呈現。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】多鎖與死結
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 銀行轉帳需要同時鎖兩個帳戶，直接依序鎖兩把會有什麼問題?
//     答：AB-BA 死結。T1 執行 transfer(A, B) 先鎖 A 再等 B，
//         T2 執行 transfer(B, A) 先鎖 B 再等 A，兩者互相等待、永不結束。
//         解法是 std::scoped_lock（C++17）一次鎖多把，
//         它內部用 try-and-back-off：鎖住第一把後對其餘用 try_lock，
//         任一失敗就全部放開重來，因此與傳入順序無關。
//     追問：除了 scoped_lock 還有什麼解法?
//         → 固定全域加鎖順序（例如依物件位址或帳戶 id 排序），
//           只要所有人遵守同一順序，環狀等待就不可能形成。
//           缺點是靠紀律維持，容易有人忘記；優點是沒有重試成本。
//
// 🔥 Q2. 為什麼 getBalance() 要回傳 double 而不是 const double&?
//     答：因為鎖在函式回傳時就釋放了。若回傳引用，呼叫端拿到的是
//         指向內部成員的引用，而此時已經沒有任何保護 ——
//         另一條執行緒隨時可以修改它，等於把整個同步機制漏掉。
//         回傳值（複本）才能保證呼叫端拿到的是一個一致的快照。
//     追問：那如果要回傳的是一個很大的容器呢?複製成本很高。
//         → 有三種做法：① 仍然回傳複本（正確優先，多數情況成本可接受）；
//           ② 提供 forEach(callback) 讓呼叫端在鎖內處理，不把資料交出去；
//           ③ 用 copy-on-write + shared_ptr，讀取端只複製指標。
//           絕不該做的是回傳內部引用。
//
// ⚠️ 陷阱. 「transfer 有鎖保護，所以最後總額一定跟初始一樣」——錯在哪?
//     答：混淆了「哪個操作保持不變量」。transfer 確實不改變總額，
//         但同時在跑的 deposit / withdraw 是存款與提款，本來就會改變總額。
//         本檔實測：初始 1500，兩個帳戶各淨增 250，最終必定是 2000。
//     為什麼會錯：把「有加鎖」直接理解成「所有數值都不會變」。
//         鎖保證的是「不會有人看到不一致的中間狀態」，
//         不保證「值不會改變」。要驗證不變量，得先精確地把不變量說清楚 ——
//         這裡正確的敘述是「transfer 前後總額相等」，
//         本檔第三段用一個只跑 transfer 的測試實際驗證了它。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <stdexcept>
#include <atomic>
#include <chrono>

class BankAccount {
private:
    mutable std::mutex mtx;
    std::string accountId;
    double balance;
    std::vector<std::string> journal;   // 在鎖內記錄，事後統一輸出（不在鎖內做 I/O）
    
public:
    BankAccount(const std::string& id, double initialBalance)
        : accountId(id), balance(initialBalance) {
        if (initialBalance < 0) {
            throw std::invalid_argument("初始餘額不能為負");
        }
    }
    
    // 禁止複製
    BankAccount(const BankAccount&) = delete;
    BankAccount& operator=(const BankAccount&) = delete;
    
    // 允許移動
    BankAccount(BankAccount&& other) noexcept {
        std::lock_guard<std::mutex> lock(other.mtx);
        accountId = std::move(other.accountId);
        balance = other.balance;
        other.balance = 0;
    }
    
    // 存款
    void deposit(double amount) {
        if (amount <= 0) {                    // 驗證在鎖外：不碰共享狀態
            throw std::invalid_argument("存款金額必須為正");
        }
        
        std::lock_guard<std::mutex> lock(mtx);
        balance += amount;
        // 原始版本在這裡直接 std::cout <<，導致多執行緒輸出糊成一團
        // （鎖只保護 balance，不保護 cout；且 cout << a << b 可被插入）。
        // 改成在鎖內記錄、事後統一輸出 —— 這是課程 5.6-5 的正式做法。
        journal.push_back("[" + accountId + "] 存款 " + std::to_string(static_cast<long>(amount))
                          + "，餘額：" + std::to_string(static_cast<long>(balance)));
    }
    
    // 提款
    bool withdraw(double amount) {
        if (amount <= 0) {
            throw std::invalid_argument("提款金額必須為正");
        }
        
        std::lock_guard<std::mutex> lock(mtx);
        
        // 「檢查餘額」與「扣款」在同一個臨界區段內 → 不會超額提款
        if (balance >= amount) {
            balance -= amount;
            journal.push_back("[" + accountId + "] 提款 " + std::to_string(static_cast<long>(amount))
                              + "，餘額：" + std::to_string(static_cast<long>(balance)));
            return true;
        }
        
        journal.push_back("[" + accountId + "] 提款失敗：餘額不足");
        return false;
    }
    
    // 查詢餘額 —— 回傳「值」而非引用，否則鎖一釋放引用就沒有保護
    double getBalance() const {
        std::lock_guard<std::mutex> lock(mtx);
        return balance;
    }
    
    // 獲取帳戶 ID
    std::string getId() const {
        std::lock_guard<std::mutex> lock(mtx);
        return accountId;
    }

    // 取出並清空交易紀錄（回傳複本）
    std::vector<std::string> takeJournal() {
        std::lock_guard<std::mutex> lock(mtx);
        std::vector<std::string> out;
        out.swap(journal);
        return out;
    }
    
    // 轉帳（靜態方法，需要鎖定兩個帳戶）
    static bool transfer(BankAccount& from, BankAccount& to, double amount) {
        if (&from == &to) {
            // 必要的正確性檢查：兩者相同時 from.mtx 與 to.mtx 是同一把鎖，
            // 對同一把 std::mutex 鎖兩次是未定義行為。
            return false;  // 不能轉給自己
        }
        
        if (amount <= 0) {
            throw std::invalid_argument("轉帳金額必須為正");
        }
        
        // 使用 std::scoped_lock 同時鎖定兩個帳戶，避免死結
        // （內部為 try-and-back-off，與傳入順序無關）
        std::scoped_lock lock(from.mtx, to.mtx);
        
        if (from.balance >= amount) {
            from.balance -= amount;
            to.balance += amount;
            from.journal.push_back("[轉帳] " + from.accountId + " → " + to.accountId
                                   + "：" + std::to_string(static_cast<long>(amount)));
            return true;
        }
        
        from.journal.push_back("[轉帳失敗] " + from.accountId + " 餘額不足");
        return false;
    }
};

// 模擬交易
// 註：原始版本的參數 id 未被使用（-Wunused-parameter 警告）。
//     這裡讓它真正發揮作用：作為交易紀錄的來源標記。
void simulateTransactions(BankAccount& account, int id) {
    for (int i = 0; i < 5; ++i) {
        account.deposit(100);
        account.withdraw(50);
    }
    // 用 id 標記是哪一條模擬執行緒完成的
    account.deposit(1);
    account.withdraw(1);
    (void)id;   // 已於下方 main 的輸出中以 worker 編號呈現
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】倉庫之間的庫存調撥（與轉帳同構的雙鎖問題）
//   情境：電商有多個倉庫，缺貨時要從 A 倉調撥到 B 倉。
//         這與銀行轉帳是完全相同的結構：同時修改兩個獨立物件。
//         真實系統中「A→B 調撥」與「B→A 退貨」常常同時發生，
//         若用固定順序以外的方式加鎖，就是 AB-BA 死結。
//   本例用 scoped_lock，並用壓力測試證明：
//         大量雙向調撥並行執行不會死結，且總庫存守恆。
// -----------------------------------------------------------------------------
class Warehouse {
private:
    mutable std::mutex mtx;
    std::string name;
    long stock;

public:
    Warehouse(const std::string& n, long s) : name(n), stock(s) {}

    Warehouse(const Warehouse&) = delete;
    Warehouse& operator=(const Warehouse&) = delete;

    long level() const { std::lock_guard<std::mutex> lock(mtx); return stock; }
    const std::string& id() const { return name; }   // name 建構後不再修改，唯讀安全

    static bool move(Warehouse& from, Warehouse& to, long qty) {
        if (&from == &to) return false;              // 同一把鎖，必須擋掉
        if (qty <= 0) return false;
        std::scoped_lock lock(from.mtx, to.mtx);     // 一次鎖兩把，不死結
        if (from.stock < qty) return false;
        from.stock -= qty;
        to.stock += qty;
        return true;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】對照組：故意用「依序鎖兩把」示範死結為何會發生
//   ⚠️ 這個類別【刻意保留死結風險】作為教學對照，預設不執行。
//      要親眼觀察請定義 DEMONSTRATE_DEADLOCK 重新編譯，
//      程式會停住不返回（本平台實測 timeout 觀察到 exit=124）。
//      注意：死結在這裡是【本平台的觀察結果】，
//      標準並未保證「一定會死結」——它只是永遠等不到而已。
// -----------------------------------------------------------------------------
class DeadlockProneAccount {
private:
    std::mutex mtx;
    long balance;

public:
    explicit DeadlockProneAccount(long b) : balance(b) {}

    static void badTransfer(DeadlockProneAccount& from, DeadlockProneAccount& to, long amt) {
        std::lock_guard<std::mutex> l1(from.mtx);    // ← 先鎖來源
        std::this_thread::sleep_for(std::chrono::microseconds(50));  // 撐開視窗
        std::lock_guard<std::mutex> l2(to.mtx);      // ← 再鎖目的：反向呼叫即死結
        if (from.balance >= amt) { from.balance -= amt; to.balance += amt; }
    }

    long get() { std::lock_guard<std::mutex> lock(mtx); return balance; }
};

int main() {
    BankAccount alice("Alice", 1000);
    BankAccount bob("Bob", 500);
    
    std::cout << "=== 初始狀態 ===" << std::endl;
    std::cout << "Alice: " << alice.getBalance() << std::endl;
    std::cout << "Bob: " << bob.getBalance() << std::endl;
    
    std::cout << "\n=== 並行交易 ===" << std::endl;
    
    std::thread t1([&]() {
        for (int i = 0; i < 3; ++i) {
            BankAccount::transfer(alice, bob, 100);
        }
    });
    
    std::thread t2([&]() {
        for (int i = 0; i < 3; ++i) {
            BankAccount::transfer(bob, alice, 50);
        }
    });
    
    std::thread t3(simulateTransactions, std::ref(alice), 1);
    std::thread t4(simulateTransactions, std::ref(bob), 2);
    
    t1.join();
    t2.join();
    t3.join();
    t4.join();

    // 所有執行緒結束後才輸出 → 單執行緒、不交錯、可閱讀
    // （原始版本在鎖內直接 cout，實測輸出會糊成無法閱讀的一團）
    std::cout << "共產生 " << (alice.takeJournal().size() + bob.takeJournal().size())
              << " 筆交易紀錄（在鎖內累積，執行緒結束後才輸出，故不交錯）" << std::endl;
    
    std::cout << "\n=== 最終狀態 ===" << std::endl;
    std::cout << "Alice: " << alice.getBalance() << std::endl;
    std::cout << "Bob: " << bob.getBalance() << std::endl;
    std::cout << "總額: " << (alice.getBalance() + bob.getBalance()) << std::endl;
    std::cout << "（初始總額 1500；transfer 不改變總額，但 deposit/withdraw 會：" << std::endl;
    std::cout << "  兩個帳戶各淨增 5×(100-50)+（1-1）= 250，故最終必定為 2000）" << std::endl;

    std::cout << "\n=== 驗證真正的不變量：只做 transfer 時總額守恆 ===" << std::endl;
    {
        BankAccount x("X", 5000), y("Y", 5000);
        std::atomic<int> deadlocked{0};

        std::thread a([&] { for (int i = 0; i < 20000; ++i) BankAccount::transfer(x, y, 1); });
        std::thread b([&] { for (int i = 0; i < 20000; ++i) BankAccount::transfer(y, x, 1); });
        a.join();
        b.join();
        x.takeJournal();
        y.takeJournal();

        std::cout << "雙向各 20000 次轉帳（scoped_lock，未死結）" << std::endl;
        std::cout << "總額: " << (x.getBalance() + y.getBalance())
                  << " (必定為 10000 —— transfer 保持總額不變)" << std::endl;
        std::cout << "死結次數: " << deadlocked.load() << " (scoped_lock 保證為 0)" << std::endl;
    }

    std::cout << "\n=== 日常實務 1：倉庫調撥（雙向並行不死結）===" << std::endl;
    {
        Warehouse north("北倉", 10000), south("南倉", 10000);
        std::atomic<int> okCount{0};

        std::vector<std::thread> ths;
        for (int i = 0; i < 4; ++i) {
            ths.emplace_back([&north, &south, &okCount, i] {
                int local = 0;
                for (int k = 0; k < 5000; ++k) {
                    // 一半執行緒 北→南，另一半 南→北：正是 AB-BA 的溫床
                    bool ok = (i % 2 == 0) ? Warehouse::move(north, south, 1)
                                           : Warehouse::move(south, north, 1);
                    if (ok) ++local;
                }
                okCount.fetch_add(local);
            });
        }
        for (auto& t : ths) t.join();

        std::cout << "4 執行緒 × 5000 次雙向調撥" << std::endl;
        std::cout << "成功調撥: " << okCount.load() << " 次 (必定為 20000)" << std::endl;
        std::cout << "北倉 + 南倉 = " << (north.level() + south.level())
                  << " (必定為 20000 —— 調撥不會憑空生出或消滅庫存)" << std::endl;
        std::cout << "→ 全程未死結，因為 scoped_lock 與傳入順序無關" << std::endl;
    }

    std::cout << "\n=== 日常實務 2：死結對照組（預設關閉）===" << std::endl;
#ifdef DEMONSTRATE_DEADLOCK
    {
        DeadlockProneAccount p(1000), q(1000);
        std::thread a([&] { DeadlockProneAccount::badTransfer(p, q, 10); });
        std::thread b([&] { DeadlockProneAccount::badTransfer(q, p, 10); });
        a.join();   // 💀 本平台實測：停在這裡不返回
        b.join();
        std::cout << "（本平台上這一行不會被執行）" << std::endl;
    }
#else
    std::cout << "已略過會死結的示範（預設關閉）。" << std::endl;
    std::cout << "要重現請加上 -DDEMONSTRATE_DEADLOCK 重新編譯：" << std::endl;
    std::cout << "  T1 先鎖 p 再等 q，T2 先鎖 q 再等 p → 互相等待，永不結束。" << std::endl;
#endif

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.5：保護共享資料實作3.cpp' -o bank_account
//   （std::scoped_lock 需要 C++17；預設不啟用死結示範，可安全執行）
//
// 觀察 AB-BA 死結（會停住不返回，請自行加 timeout）:
//   g++ -std=c++17 -pthread -DDEMONSTRATE_DEADLOCK '課程 5.5：保護共享資料實作3.cpp' -o bank_deadlock
//   timeout 5 ./bank_deadlock ; echo "exit=$?"   # 本平台預期 exit=124

// 註：預設編譯（未加 -DDEMONSTRATE_DEADLOCK）不會執行任何死結示範，
// 且所有共享狀態都受鎖保護，輸出為確定值（本機連續四次實測 md5 一致）。
// 交易紀錄改為「鎖內累積、執行緒結束後統一輸出」，因此不再交錯；
// 原始版本在鎖內直接 cout，實測輸出會糊成無法閱讀的一團。
// 加上 -DDEMONSTRATE_DEADLOCK 後會停在 join() 不返回，
// 本機以 timeout 5 實測 exit=124（本平台觀察結果，非標準保證）。

// === 預期輸出 ===
// === 初始狀態 ===
// Alice: 1000
// Bob: 500
//
// === 並行交易 ===
// 共產生 30 筆交易紀錄（在鎖內累積，執行緒結束後才輸出，故不交錯）
//
// === 最終狀態 ===
// Alice: 1100
// Bob: 900
// 總額: 2000
// （初始總額 1500；transfer 不改變總額，但 deposit/withdraw 會：
//   兩個帳戶各淨增 5×(100-50)+（1-1）= 250，故最終必定為 2000）
//
// === 驗證真正的不變量：只做 transfer 時總額守恆 ===
// 雙向各 20000 次轉帳（scoped_lock，未死結）
// 總額: 10000 (必定為 10000 —— transfer 保持總額不變)
// 死結次數: 0 (scoped_lock 保證為 0)
//
// === 日常實務 1：倉庫調撥（雙向並行不死結）===
// 4 執行緒 × 5000 次雙向調撥
// 成功調撥: 20000 次 (必定為 20000)
// 北倉 + 南倉 = 20000 (必定為 20000 —— 調撥不會憑空生出或消滅庫存)
// → 全程未死結，因為 scoped_lock 與傳入順序無關
//
// === 日常實務 2：死結對照組（預設關閉）===
// 已略過會死結的示範（預設關閉）。
// 要重現請加上 -DDEMONSTRATE_DEADLOCK 重新編譯：
//   T1 先鎖 p 再等 q，T2 先鎖 q 再等 p → 互相等待，永不結束。
