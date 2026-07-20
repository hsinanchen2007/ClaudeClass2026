// =============================================================================
//  課程 5.5：保護共享資料實作7.cpp  —  把「複合操作」設計成類別的原子方法
// =============================================================================
//
// 【檔案結構】上半部（約 950 行）是課程講義，全部以 // 註解呈現；
//   下半部是可執行的 ConfigManager 實作與測試。本段是為整份檔案補上的教科書導讀。
//
// 【主題資訊 Information】
//   主題：    執行緒安全的設定管理器；把 check-then-act 封裝成原子方法
//   關鍵方法：setIfAbsent()（原子的「若不存在則設定」）
//             compareAndSet()（原子的「比較並交換」）
//   標準版本：std::optional 為 C++17；結構化繫結 auto [k, v] 為 C++17；
//             std::mutex / lock_guard 為 C++11
//   標頭檔：  <map>、<optional>、<mutex>
//   複雜度：  std::map 各操作 O(log n)；鎖只增加常數成本
//
// 【詳細解釋 Explanation】
//
// 【1. 這個類別最重要的兩個方法，也是最容易被忽略的】
//   get / set / has / remove 這些是「顯而易見」的介面，
//   但真正體現執行緒安全設計功力的是這兩個：
//       bool setIfAbsent(key, value);                    // 原子的 check-then-act
//       bool compareAndSet(key, expected, newValue);     // 原子的 CAS
//   為什麼需要它們？因為呼叫端【無法】用 get + set 組合出同樣的效果：
//       if (!cfg.has(key)) cfg.set(key, value);   // ✗ 兩次呼叫之間有縫隙
//   即使 has() 與 set() 各自都有鎖，中間仍可以被插入 ——
//   這正是課程 4.4 的 check-then-act 模式。
//   → 結論：【呼叫端組合不出來的原子性，必須由類別自己提供】。
//     這是執行緒安全類別設計的第一原則。
//
// 【2. compareAndSet 為什麼是最強的那一個】
//   CAS（compare-and-swap）是所有無鎖演算法的基石，這裡是它的高階版本：
//   「只有在目前值仍等於我上次讀到的值時，才寫入新值」。
//   它讓呼叫端可以安全地做「讀取 → 計算 → 寫回」：
//       do {
//           old = cfg.get(key);
//           newVal = compute(old);
//       } while (!cfg.compareAndSet(key, old, newVal));
//   若中途有人改了值，compareAndSet 回傳 false，迴圈重試。
//   這解決了 RMW 競爭（課程 4.4-2），而且不需要對外暴露鎖。
//   ⚠️ 注意這個模式有 ABA 問題：若值從 A 變成 B 又變回 A，
//      CAS 會誤以為「沒有人動過」。對設定字串通常無妨，
//      但在指標或版本敏感的場景要加版本號（tagged pointer）。
//
// 【3. 為什麼 get() 回傳 std::optional 而不是 std::string】
//   「找不到」與「值是空字串」是兩件不同的事。用 optional 明確區分：
//       auto v = cfg.get("timeout");
//       if (v) { /* 有設定，值是 *v */ } else { /* 根本沒設定 */ }
//   若回傳 std::string 並用 "" 代表不存在，就無法表達「設定成空字串」。
//   同時 optional 回傳的是【值】不是引用 —— 這符合實作4 的教訓：
//   絕不把內部資料的引用交到鎖外。
//   （C++17 前的做法是回傳 bool + 輸出參數，或回傳指標並約定 nullptr。）
//
// 【4. keys() 為什麼要回傳整份複本】
//   走訪設定必須先取得所有 key。若回傳 map 的引用或迭代器，
//   呼叫端在鎖外走訪時，另一條執行緒的 set() 可能觸發 std::map 的
//   節點插入，雖然 map 的插入不會使既有迭代器失效（這點與 vector 不同），
//   但 erase() 會使被刪節點的迭代器失效，而且讀寫本身就構成 data race。
//   回傳 vector<string> 複本是唯一安全的做法。
//   ⚠️ 但要注意：拿到 keys 之後再逐一 get()，
//      中間設定可能已經改變 —— 這是【無法避免】的，
//      因為「走訪」本質上是多次觀察。需要一致快照就用 snapshot()
//      一次取得整份 map 的複本。
//
// 【5. 這個類別的效能特性與適用範圍】
//   單一 mutex 保護整個 map = 粗粒度鎖。對「設定」這種
//   【讀多寫極少、且資料量小】的場景完全合適。
//   但若是每秒數百萬次讀取的熱路徑，這把鎖會成為瓶頸，此時應改用：
//     * copy-on-write + shared_ptr<const Map>：讀取端幾乎零成本（見 5.5-2）
//     * std::shared_mutex（C++17）：多讀者可並行
//     * 分片鎖（sharded lock）：依 key 的雜湊分成 N 把鎖（見 5.6-3）
//   → 選鎖的粒度要看存取模式，不是越細越好（細粒度有自己的成本）。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 std::map 而不是 std::unordered_map
//   設定項目通常只有數十到數百筆，O(log n) 與 O(1) 的差距完全不重要；
//   而 map 有序，keys() 回傳的順序是穩定的字典序，
//   對輸出、比對、diff 都友善（本檔的最終輸出就是有序的）。
//   若真的是幾十萬筆的熱路徑快取，才該換 unordered_map。
//
// (B) 結構化繫結 `for (const auto& [key, value] : values)` 是 C++17
//   它把 std::pair<const K, V> 拆成兩個名字，比 it->first / it->second 好讀。
//   注意繫結出來的 key 是 const 的（map 的鍵不可修改），
//   所以即使寫成 auto& 也不能改 key。
//
// (C) 為什麼 setAll 用一次鎖而不是迴圈呼叫 set
//   迴圈呼叫 set() N 次 = 上鎖解鎖 N 次（本機每次約 13~25 ns，見 5.6-1），
//   而且每次釋放鎖都給了別人插隊的機會 ——
//   結果是「批次設定」在別人眼中變成一連串零散的變更，
//   中間狀態可能被觀察到。setAll 用一次鎖，讓整批變更成為
//   一個不可分割的原子操作，這同時是效能與正確性的需求。
//
// 【注意事項 Pay Attention】
// 1. 呼叫端組合不出來的原子性（check-then-act、CAS），必須由類別自己提供。
// 2. get() 回傳 optional 值而非引用；keys() 回傳複本而非迭代器。
// 3. 拿到 keys 之後再逐一 get()，中間值可能已改變 ——
//    需要一致快照請用一次取得整份 map 的方法。
// 4. compareAndSet 有 ABA 問題：值 A→B→A 時 CAS 無法察覺，
//    版本敏感的場景要加版本號。
// 5. 單一鎖保護整個 map 適合「讀多寫少、資料量小」；
//    熱路徑請改用 copy-on-write、shared_mutex 或分片鎖。
// 6. 批次操作（setAll）要用一次鎖，兼顧效能與原子性。
//
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 146. LRU Cache
//   （實作於檔案下方的 LRUCache 類別）
//   題目：設計固定容量的 LRU 快取，get 與 put 都必須是 O(1)；
//         容量滿時淘汰最久未使用的項目。
//   為什麼用到本主題：LRU Cache 是 ConfigManager 的「進階版」——
//         同樣是 key/value 儲存，但多了一條【橫跨兩個資料結構的不變量】：
//           雜湊表的每個 key 必須指向雙向串列中真實存在的節點，
//           且兩者大小必須相等。
//         get() 命中時要把節點搬到串列最前面（摘下 + 插回），
//         put() 滿了要同時從串列尾端與雜湊表移除 ——
//         這些都是多步驟的複合操作，中間不變量必然被破壞。
//         LeetCode 是單執行緒判題所以沒事；一旦多執行緒共用同一個快取，
//         就必須像 ConfigManager 那樣，把每個操作封裝成【原子方法】。
//         特別注意 get() 也會修改狀態（更新使用順序），
//         所以它【不能】是 const、也【不能】只用讀鎖 ——
//         這是 LRU 快取最常被忽略的並行陷阱。
// -----------------------------------------------------------------------------
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】執行緒安全的 key/value 容器
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 已經有 has() 和 set() 了，為什麼還需要 setIfAbsent()?
//     答：因為呼叫端【組合不出】這個原子性。
//         `if (!cfg.has(k)) cfg.set(k, v);` 即使兩個方法各自有鎖，
//         中間仍有縫隙，另一條執行緒可以插進來先設定 ——
//         這就是 check-then-act。要讓「檢查 + 設定」不可分割，
//         只能由類別在同一個臨界區段內完成。
//     追問：這個原則有沒有通則?
//         → 有：【呼叫端無法用現有方法組合出來的原子性，
//           就必須由類別自己提供成一個方法】。
//           這也是為什麼執行緒安全容器的 API 通常比單執行緒版本「更胖」：
//           tryPop、setIfAbsent、compareAndSet、getOrCompute 等等。
//
// 🔥 Q2. compareAndSet 可以用來做什麼?它有什麼已知缺陷?
//     答：它讓呼叫端能安全地做「讀取 → 計算 → 寫回」：
//         讀到舊值、算出新值、只有在值沒被別人改過時才寫入，
//         否則重試。這解決了 read-modify-write 競爭，
//         而且不需要把鎖暴露給呼叫端。
//         缺陷是【ABA 問題】：若值從 A 變 B 又變回 A，
//         CAS 會誤判「沒有人動過」。
//     追問：ABA 怎麼解?
//         → 加版本號：每次修改都遞增一個 counter，
//           CAS 時比對「值 + 版本」而非只比對值
//           （無鎖資料結構中稱為 tagged pointer 或 double-word CAS）。
//
// ⚠️ 陷阱. LRU Cache 的 get() 是唯讀操作，所以可以標成 const、
//         也可以只用讀鎖讓多個讀者並行 —— 對嗎?
//     答：不對。LRU 的 get() 命中時必須把該節點【搬到串列最前面】
//         以更新「最近使用」的順序 —— 它會修改資料結構。
//         所以它不能是 const，也不能只用 shared_lock：
//         兩個讀者同時命中不同的 key，會同時修改同一條串列的指標 → data race。
//     為什麼會錯：把「語意上的讀取」等同於「實作上的唯讀」。
//         很多快取、統計、惰性計算的 getter 都會在內部改狀態
//         （更新 LRU 順序、遞增命中計數、填充快取）。
//         判斷要不要保護，看的是【有沒有寫入記憶體】，
//         不是這個方法在概念上叫做「取得」還是「設定」。
// ═══════════════════════════════════════════════════════════════════════════

///*
//# 第五階段：互斥鎖基礎 (std::mutex)
//
//## 課程 5.5：保護共享資料實作
//
//---
//
//### 引言
//
//前幾課我們學習了 `std::mutex` 的基本操作和常見錯誤。現在，讓我們將這些知識付諸實踐，設計並實作完整的執行緒安全類別。這是多執行緒程式設計中最重要的技能之一。
//
//---
//
//### 一、執行緒安全類別的設計原則
//
//```
//┌─────────────────────────────────────────────────────────────┐
//│              執行緒安全類別設計原則                          │
//├─────────────────────────────────────────────────────────────┤
//│                                                             │
//│  1. 封裝原則                                                │
//│     → 互斥鎖和資料放在同一個類別中                          │
//│     → 外部無法直接存取資料或鎖                              │
//│                                                             │
//│  2. 最小介面原則                                            │
//│     → 只暴露必要的操作                                      │
//│     → 避免返回內部資料的指標或引用                          │
//│                                                             │
//│  3. 原子操作原則                                            │
//│     → 每個公開方法應該是原子的                              │
//│     → 檢查和操作應在同一個臨界區段                          │
//│                                                             │
//│  4. 不變量保護原則                                          │
//│     → 確保類別的不變量在任何時刻都成立                      │
//│     → 臨界區段結束前恢復不變量                              │
//│                                                             │
//└─────────────────────────────────────────────────────────────┘
//```
//
//---
//
//### 二、實作一：執行緒安全計數器
//
//從最簡單的例子開始。
//
//```cpp
//// 檔案：lesson_5_5_safe_counter.cpp
//// 說明：執行緒安全的計數器類別
//
//#include <iostream>
//#include <thread>
//#include <mutex>
//#include <vector>
//
//class ThreadSafeCounter {
//private:
//    mutable std::mutex mtx;  // mutable：允許在 const 方法中使用
//    int count = 0;
//    
//public:
//    // 建構函式
//    ThreadSafeCounter() = default;
//    
//    // 禁止複製（互斥鎖不可複製）
//    ThreadSafeCounter(const ThreadSafeCounter&) = delete;
//    ThreadSafeCounter& operator=(const ThreadSafeCounter&) = delete;
//    
//    // 遞增
//    void increment() {
//        std::lock_guard<std::mutex> lock(mtx);
//        ++count;
//    }
//    
//    // 遞減
//    void decrement() {
//        std::lock_guard<std::mutex> lock(mtx);
//        --count;
//    }
//    
//    // 增加指定值
//    void add(int value) {
//        std::lock_guard<std::mutex> lock(mtx);
//        count += value;
//    }
//    
//    // 獲取當前值
//    int get() const {
//        std::lock_guard<std::mutex> lock(mtx);
//        return count;
//    }
//    
//    // 重設並返回舊值（原子操作）
//    int reset() {
//        std::lock_guard<std::mutex> lock(mtx);
//        int oldValue = count;
//        count = 0;
//        return oldValue;
//    }
//    
//    // 條件遞增：只有當前值小於 max 時才遞增
//    bool incrementIfLessThan(int max) {
//        std::lock_guard<std::mutex> lock(mtx);
//        if (count < max) {
//            ++count;
//            return true;
//        }
//        return false;
//    }
//};
//
//// 測試
//void workerIncrement(ThreadSafeCounter& counter, int times) {
//    for (int i = 0; i < times; ++i) {
//        counter.increment();
//    }
//}
//
//int main() {
//    ThreadSafeCounter counter;
//    
//    const int numThreads = 4;
//    const int incrementsPerThread = 100000;
//    
//    std::vector<std::thread> threads;
//    
//    for (int i = 0; i < numThreads; ++i) {
//        threads.emplace_back(workerIncrement, std::ref(counter), incrementsPerThread);
//    }
//    
//    for (auto& t : threads) {
//        t.join();
//    }
//    
//    int expected = numThreads * incrementsPerThread;
//    int actual = counter.get();
//    
//    std::cout << "預期值：" << expected << std::endl;
//    std::cout << "實際值：" << actual << std::endl;
//    std::cout << "結果：" << (expected == actual ? "✓ 正確" : "✗ 錯誤") << std::endl;
//    
//    return 0;
//}
//```
//
//#### 輸出
//
//```
//預期值：400000
//實際值：400000
//結果：✓ 正確
//```
//
//---
//
//### 三、為什麼禁止複製？
//
//```
//┌─────────────────────────────────────────────────────────────┐
//│           為什麼含有 mutex 的類別應禁止複製？                │
//├─────────────────────────────────────────────────────────────┤
//│                                                             │
//│  問題 1：std::mutex 本身不可複製                            │
//│  ─────────────────────────────                              │
//│  std::mutex mtx1;                                           │
//│  std::mutex mtx2 = mtx1;  // 編譯錯誤！                     │
//│                                                             │
//│  問題 2：複製語意不明確                                     │
//│  ─────────────────────                                      │
//│  如果允許複製：                                             │
//│  • 複製時原物件被鎖定怎麼辦？                               │
//│  • 新物件應該共享鎖還是有自己的鎖？                         │
//│  • 複製後兩個物件是否獨立？                                 │
//│                                                             │
//│  解決方案                                                   │
//│  ────────                                                   │
//│  1. 禁止複製和賦值                                          │
//│     Counter(const Counter&) = delete;                       │
//│     Counter& operator=(const Counter&) = delete;            │
//│                                                             │
//│  2. 如果需要複製語意，明確定義行為                          │
//│     （見下方範例）                                           │
//│                                                             │
//└─────────────────────────────────────────────────────────────┘
//```
//
//#### 如果確實需要複製
//
//```cpp
//// 檔案：lesson_5_5_copyable_counter.cpp
//// 說明：可複製的執行緒安全計數器（明確定義複製行為）
//
//#include <iostream>
//#include <mutex>
//
//class CopyableCounter {
//private:
//    mutable std::mutex mtx;
//    int count = 0;
//    
//public:
//    CopyableCounter() = default;
//    
//    // 明確定義複製建構函式
//    CopyableCounter(const CopyableCounter& other) {
//        std::lock_guard<std::mutex> lock(other.mtx);  // 鎖定來源
//        count = other.count;  // 複製值
//        // 新物件有自己的 mutex（預設建構）
//    }
//    
//    // 明確定義複製賦值運算子
//    CopyableCounter& operator=(const CopyableCounter& other) {
//        if (this != &other) {
//            // 需要同時鎖定兩個物件，注意死結！
//            std::scoped_lock lock(mtx, other.mtx);
//            count = other.count;
//        }
//        return *this;
//    }
//    
//    void increment() {
//        std::lock_guard<std::mutex> lock(mtx);
//        ++count;
//    }
//    
//    int get() const {
//        std::lock_guard<std::mutex> lock(mtx);
//        return count;
//    }
//};
//
//int main() {
//    CopyableCounter c1;
//    c1.increment();
//    c1.increment();
//    
//    CopyableCounter c2 = c1;  // 複製
//    
//    std::cout << "c1 = " << c1.get() << std::endl;  // 2
//    std::cout << "c2 = " << c2.get() << std::endl;  // 2
//    
//    c1.increment();
//    
//    std::cout << "c1 = " << c1.get() << std::endl;  // 3
//    std::cout << "c2 = " << c2.get() << std::endl;  // 2（獨立）
//    
//    return 0;
//}
//```
//
//---
//
//### 四、實作二：執行緒安全銀行帳戶
//
//更複雜的實際案例。
//
//```cpp
//// 檔案：lesson_5_5_bank_account.cpp
//// 說明：執行緒安全的銀行帳戶類別
//
//#include <iostream>
//#include <thread>
//#include <mutex>
//#include <vector>
//#include <string>
//#include <stdexcept>
//
//class BankAccount {
//private:
//    mutable std::mutex mtx;
//    std::string accountId;
//    double balance;
//    
//public:
//    BankAccount(const std::string& id, double initialBalance)
//        : accountId(id), balance(initialBalance) {
//        if (initialBalance < 0) {
//            throw std::invalid_argument("初始餘額不能為負");
//        }
//    }
//    
//    // 禁止複製
//    BankAccount(const BankAccount&) = delete;
//    BankAccount& operator=(const BankAccount&) = delete;
//    
//    // 允許移動
//    BankAccount(BankAccount&& other) noexcept {
//        std::lock_guard<std::mutex> lock(other.mtx);
//        accountId = std::move(other.accountId);
//        balance = other.balance;
//        other.balance = 0;
//    }
//    
//    // 存款
//    void deposit(double amount) {
//        if (amount <= 0) {
//            throw std::invalid_argument("存款金額必須為正");
//        }
//        
//        std::lock_guard<std::mutex> lock(mtx);
//        balance += amount;
//        std::cout << "[" << accountId << "] 存款 " << amount 
//                  << "，餘額：" << balance << std::endl;
//    }
//    
//    // 提款
//    bool withdraw(double amount) {
//        if (amount <= 0) {
//            throw std::invalid_argument("提款金額必須為正");
//        }
//        
//        std::lock_guard<std::mutex> lock(mtx);
//        
//        if (balance >= amount) {
//            balance -= amount;
//            std::cout << "[" << accountId << "] 提款 " << amount 
//                      << "，餘額：" << balance << std::endl;
//            return true;
//        }
//        
//        std::cout << "[" << accountId << "] 提款失敗：餘額不足" << std::endl;
//        return false;
//    }
//    
//    // 查詢餘額
//    double getBalance() const {
//        std::lock_guard<std::mutex> lock(mtx);
//        return balance;
//    }
//    
//    // 獲取帳戶 ID
//    std::string getId() const {
//        std::lock_guard<std::mutex> lock(mtx);
//        return accountId;
//    }
//    
//    // 轉帳（靜態方法，需要鎖定兩個帳戶）
//    static bool transfer(BankAccount& from, BankAccount& to, double amount) {
//        if (&from == &to) {
//            return false;  // 不能轉給自己
//        }
//        
//        if (amount <= 0) {
//            throw std::invalid_argument("轉帳金額必須為正");
//        }
//        
//        // 使用 std::scoped_lock 同時鎖定兩個帳戶，避免死結
//        std::scoped_lock lock(from.mtx, to.mtx);
//        
//        if (from.balance >= amount) {
//            from.balance -= amount;
//            to.balance += amount;
//            std::cout << "[轉帳] " << from.accountId << " → " << to.accountId 
//                      << "：" << amount << std::endl;
//            return true;
//        }
//        
//        std::cout << "[轉帳失敗] " << from.accountId << " 餘額不足" << std::endl;
//        return false;
//    }
//};
//
//// 模擬交易
//void simulateTransactions(BankAccount& account, int id) {
//    for (int i = 0; i < 5; ++i) {
//        account.deposit(100);
//        account.withdraw(50);
//    }
//}
//
//int main() {
//    BankAccount alice("Alice", 1000);
//    BankAccount bob("Bob", 500);
//    
//    std::cout << "=== 初始狀態 ===" << std::endl;
//    std::cout << "Alice: " << alice.getBalance() << std::endl;
//    std::cout << "Bob: " << bob.getBalance() << std::endl;
//    
//    std::cout << "\n=== 並行交易 ===" << std::endl;
//    
//    std::thread t1([&]() {
//        for (int i = 0; i < 3; ++i) {
//            BankAccount::transfer(alice, bob, 100);
//        }
//    });
//    
//    std::thread t2([&]() {
//        for (int i = 0; i < 3; ++i) {
//            BankAccount::transfer(bob, alice, 50);
//        }
//    });
//    
//    std::thread t3(simulateTransactions, std::ref(alice), 1);
//    std::thread t4(simulateTransactions, std::ref(bob), 2);
//    
//    t1.join();
//    t2.join();
//    t3.join();
//    t4.join();
//    
//    std::cout << "\n=== 最終狀態 ===" << std::endl;
//    std::cout << "Alice: " << alice.getBalance() << std::endl;
//    std::cout << "Bob: " << bob.getBalance() << std::endl;
//    std::cout << "總額: " << (alice.getBalance() + bob.getBalance()) << std::endl;
//    std::cout << "（初始總額為 1500，應保持不變）" << std::endl;
//    
//    return 0;
//}
//```
//
//---
//
//### 五、介面設計的陷阱
//
//#### 危險：返回內部資料的引用
//
//```cpp
//// 檔案：lesson_5_5_dangerous_interface.cpp
//// 說明：危險的介面設計
//
//#include <iostream>
//#include <mutex>
//#include <vector>
//
//class DangerousContainer {
//private:
//    std::mutex mtx;
//    std::vector<int> data;
//    
//public:
//    // 💀 危險！返回內部資料的引用
//    std::vector<int>& getData() {
//        std::lock_guard<std::mutex> lock(mtx);
//        return data;  // 鎖在這裡就釋放了！
//    }
//    
//    // 💀 危險！返回內部資料的指標
//    int* getFirst() {
//        std::lock_guard<std::mutex> lock(mtx);
//        if (!data.empty()) {
//            return &data[0];  // 返回後鎖就釋放了！
//        }
//        return nullptr;
//    }
//    
//    void add(int value) {
//        std::lock_guard<std::mutex> lock(mtx);
//        data.push_back(value);
//    }
//};
//
///*
// * 問題分析：
// * 
// * Thread A:                          Thread B:
// * auto& vec = container.getData();   
// *   // 鎖已釋放！                    container.add(100);
// * vec.push_back(1);                    // 同時修改！
// *   // 💀 競爭條件！
// */
//```
//
//#### 安全：複製資料或提供安全的操作
//
//```cpp
//// 檔案：lesson_5_5_safe_interface.cpp
//// 說明：安全的介面設計
//
//#include <iostream>
//#include <mutex>
//#include <vector>
//#include <functional>
//#include <algorithm>
//
//class SafeContainer {
//private:
//    mutable std::mutex mtx;
//    std::vector<int> data;
//    
//public:
//    // ✓ 安全：返回複本
//    std::vector<int> getData() const {
//        std::lock_guard<std::mutex> lock(mtx);
//        return data;  // 返回複本，不是引用
//    }
//    
//    // ✓ 安全：返回值的複本
//    int getAt(size_t index) const {
//        std::lock_guard<std::mutex> lock(mtx);
//        if (index < data.size()) {
//            return data[index];  // 返回值，不是引用
//        }
//        throw std::out_of_range("索引超出範圍");
//    }
//    
//    // ✓ 安全：提供操作而非資料存取
//    void add(int value) {
//        std::lock_guard<std::mutex> lock(mtx);
//        data.push_back(value);
//    }
//    
//    // ✓ 安全：使用回呼函式處理資料
//    void forEach(const std::function<void(int)>& func) const {
//        std::lock_guard<std::mutex> lock(mtx);
//        for (int value : data) {
//            func(value);
//        }
//    }
//    
//    // ✓ 安全：在鎖保護下執行操作
//    template<typename Func>
//    auto withLock(Func&& func) -> decltype(func(data)) {
//        std::lock_guard<std::mutex> lock(mtx);
//        return func(data);
//    }
//    
//    // ✓ 安全：批次操作
//    void addAll(const std::vector<int>& values) {
//        std::lock_guard<std::mutex> lock(mtx);
//        data.insert(data.end(), values.begin(), values.end());
//    }
//    
//    size_t size() const {
//        std::lock_guard<std::mutex> lock(mtx);
//        return data.size();
//    }
//    
//    bool empty() const {
//        std::lock_guard<std::mutex> lock(mtx);
//        return data.empty();
//    }
//};
//
//int main() {
//    SafeContainer container;
//    
//    container.add(10);
//    container.add(20);
//    container.add(30);
//    
//    // 使用 forEach
//    std::cout << "forEach: ";
//    container.forEach([](int v) {
//        std::cout << v << " ";
//    });
//    std::cout << std::endl;
//    
//    // 使用 withLock 進行複雜操作
//    int sum = container.withLock([](std::vector<int>& data) {
//        int total = 0;
//        for (int v : data) {
//            total += v;
//        }
//        return total;
//    });
//    std::cout << "Sum: " << sum << std::endl;
//    
//    // 獲取複本
//    auto copy = container.getData();
//    std::cout << "Copy size: " << copy.size() << std::endl;
//    
//    return 0;
//}
//```
//
//---
//
//### 六、實作三：執行緒安全的 Logger
//
//實際應用中常見的日誌系統。
//
//```cpp
//// 檔案：lesson_5_5_thread_safe_logger.cpp
//// 說明：執行緒安全的日誌記錄器
//
//#include <iostream>
//#include <thread>
//#include <mutex>
//#include <string>
//#include <sstream>
//#include <chrono>
//#include <iomanip>
//#include <vector>
//#include <fstream>
//
//class ThreadSafeLogger {
//public:
//    enum class Level {
//        DEBUG,
//        INFO,
//        WARNING,
//        ERROR
//    };
//    
//private:
//    mutable std::mutex mtx;
//    std::ostream& output;
//    Level minLevel;
//    bool includeTimestamp;
//    bool includeThreadId;
//    
//    std::string levelToString(Level level) const {
//        switch (level) {
//            case Level::DEBUG:   return "DEBUG";
//            case Level::INFO:    return "INFO ";
//            case Level::WARNING: return "WARN ";
//            case Level::ERROR:   return "ERROR";
//            default:             return "?????";
//        }
//    }
//    
//    std::string getTimestamp() const {
//        auto now = std::chrono::system_clock::now();
//        auto time = std::chrono::system_clock::to_time_t(now);
//        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
//            now.time_since_epoch()) % 1000;
//        
//        std::ostringstream oss;
//        oss << std::put_time(std::localtime(&time), "%H:%M:%S");
//        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
//        return oss.str();
//    }
//    
//public:
//    ThreadSafeLogger(std::ostream& out = std::cout, 
//                     Level level = Level::DEBUG,
//                     bool timestamp = true,
//                     bool threadId = true)
//        : output(out)
//        , minLevel(level)
//        , includeTimestamp(timestamp)
//        , includeThreadId(threadId) {}
//    
//    // 禁止複製
//    ThreadSafeLogger(const ThreadSafeLogger&) = delete;
//    ThreadSafeLogger& operator=(const ThreadSafeLogger&) = delete;
//    
//    void setLevel(Level level) {
//        std::lock_guard<std::mutex> lock(mtx);
//        minLevel = level;
//    }
//    
//    void log(Level level, const std::string& message) {
//        if (level < minLevel) {
//            return;  // 過濾低級別日誌
//        }
//        
//        // 在鎖外準備訊息（減少臨界區段）
//        std::ostringstream oss;
//        
//        if (includeTimestamp) {
//            oss << "[" << getTimestamp() << "] ";
//        }
//        
//        oss << "[" << levelToString(level) << "] ";
//        
//        if (includeThreadId) {
//            oss << "[T:" << std::this_thread::get_id() << "] ";
//        }
//        
//        oss << message << std::endl;
//        
//        std::string formatted = oss.str();
//        
//        // 只在輸出時鎖定
//        std::lock_guard<std::mutex> lock(mtx);
//        output << formatted;
//        output.flush();
//    }
//    
//    void debug(const std::string& msg)   { log(Level::DEBUG, msg); }
//    void info(const std::string& msg)    { log(Level::INFO, msg); }
//    void warning(const std::string& msg) { log(Level::WARNING, msg); }
//    void error(const std::string& msg)   { log(Level::ERROR, msg); }
//};
//
//// 全域 Logger（實際專案中可能使用單例模式）
//ThreadSafeLogger logger;
//
//void worker(int id) {
//    logger.info("Worker " + std::to_string(id) + " 開始");
//    
//    for (int i = 0; i < 3; ++i) {
//        logger.debug("Worker " + std::to_string(id) + 
//                     " 執行任務 " + std::to_string(i));
//        std::this_thread::sleep_for(std::chrono::milliseconds(10));
//    }
//    
//    logger.info("Worker " + std::to_string(id) + " 結束");
//}
//
//int main() {
//    logger.info("=== 程式開始 ===");
//    
//    std::vector<std::thread> threads;
//    
//    for (int i = 1; i <= 3; ++i) {
//        threads.emplace_back(worker, i);
//    }
//    
//    for (auto& t : threads) {
//        t.join();
//    }
//    
//    logger.info("=== 程式結束 ===");
//    
//    return 0;
//}
//```
//
//#### 輸出範例
//
//```
//[14:32:15.123] [INFO ] [T:140001] === 程式開始 ===
//[14:32:15.124] [INFO ] [T:140002] Worker 1 開始
//[14:32:15.124] [INFO ] [T:140003] Worker 2 開始
//[14:32:15.124] [INFO ] [T:140004] Worker 3 開始
//[14:32:15.124] [DEBUG] [T:140002] Worker 1 執行任務 0
//[14:32:15.124] [DEBUG] [T:140003] Worker 2 執行任務 0
//[14:32:15.125] [DEBUG] [T:140004] Worker 3 執行任務 0
//...
//[14:32:15.156] [INFO ] [T:140002] Worker 1 結束
//[14:32:15.157] [INFO ] [T:140003] Worker 2 結束
//[14:32:15.157] [INFO ] [T:140004] Worker 3 結束
//[14:32:15.158] [INFO ] [T:140001] === 程式結束 ===
//```
//
//---
//
//### 七、實作四：執行緒安全的設定管理器
//
//```cpp
//// 檔案：lesson_5_5_config_manager.cpp
//// 說明：執行緒安全的設定管理器
//
//#include <iostream>
//#include <thread>
//#include <mutex>
//#include <map>
//#include <string>
//#include <optional>
//#include <vector>
//
//class ConfigManager {
//private:
//    mutable std::mutex mtx;
//    std::map<std::string, std::string> config;
//    
//public:
//    // 設定值
//    void set(const std::string& key, const std::string& value) {
//        std::lock_guard<std::mutex> lock(mtx);
//        config[key] = value;
//    }
//    
//    // 獲取值（使用 optional 表示可能不存在）
//    std::optional<std::string> get(const std::string& key) const {
//        std::lock_guard<std::mutex> lock(mtx);
//        auto it = config.find(key);
//        if (it != config.end()) {
//            return it->second;
//        }
//        return std::nullopt;
//    }
//    
//    // 獲取值，若不存在則返回預設值
//    std::string getOrDefault(const std::string& key, 
//                              const std::string& defaultValue) const {
//        std::lock_guard<std::mutex> lock(mtx);
//        auto it = config.find(key);
//        if (it != config.end()) {
//            return it->second;
//        }
//        return defaultValue;
//    }
//    
//    // 檢查是否存在
//    bool has(const std::string& key) const {
//        std::lock_guard<std::mutex> lock(mtx);
//        return config.find(key) != config.end();
//    }
//    
//    // 刪除
//    bool remove(const std::string& key) {
//        std::lock_guard<std::mutex> lock(mtx);
//        return config.erase(key) > 0;
//    }
//    
//    // 獲取所有 key
//    std::vector<std::string> keys() const {
//        std::lock_guard<std::mutex> lock(mtx);
//        std::vector<std::string> result;
//        result.reserve(config.size());
//        for (const auto& [key, value] : config) {
//            result.push_back(key);
//        }
//        return result;
//    }
//    
//    // 批次設定
//    void setAll(const std::map<std::string, std::string>& values) {
//        std::lock_guard<std::mutex> lock(mtx);
//        for (const auto& [key, value] : values) {
//            config[key] = value;
//        }
//    }
//    
//    // 原子性的「若不存在則設定」
//    bool setIfAbsent(const std::string& key, const std::string& value) {
//        std::lock_guard<std::mutex> lock(mtx);
//        if (config.find(key) == config.end()) {
//            config[key] = value;
//            return true;
//        }
//        return false;
//    }
//    
//    // 原子性的「比較並交換」
//    bool compareAndSet(const std::string& key, 
//                       const std::string& expected,
//                       const std::string& newValue) {
//        std::lock_guard<std::mutex> lock(mtx);
//        auto it = config.find(key);
//        if (it != config.end() && it->second == expected) {
//            it->second = newValue;
//            return true;
//        }
//        return false;
//    }
//    
//    size_t size() const {
//        std::lock_guard<std::mutex> lock(mtx);
//        return config.size();
//    }
//};
//
//// 測試
//void configWriter(ConfigManager& cfg, int id) {
//    for (int i = 0; i < 5; ++i) {
//        std::string key = "key_" + std::to_string(i);
//        std::string value = "writer" + std::to_string(id) + "_value" + std::to_string(i);
//        cfg.set(key, value);
//        std::this_thread::sleep_for(std::chrono::milliseconds(1));
//    }
//}
//
//void configReader(ConfigManager& cfg, int id) {
//    for (int i = 0; i < 5; ++i) {
//        std::string key = "key_" + std::to_string(i);
//        auto value = cfg.get(key);
//        if (value) {
//            std::cout << "Reader " << id << ": " << key << " = " << *value << std::endl;
//        }
//        std::this_thread::sleep_for(std::chrono::milliseconds(2));
//    }
//}
//
//int main() {
//    ConfigManager config;
//    
//    // 初始設定
//    config.set("app.name", "MyApp");
//    config.set("app.version", "1.0.0");
//    
//    std::vector<std::thread> threads;
//    
//    // 啟動寫入者
//    threads.emplace_back(configWriter, std::ref(config), 1);
//    threads.emplace_back(configWriter, std::ref(config), 2);
//    
//    // 啟動讀取者
//    threads.emplace_back(configReader, std::ref(config), 1);
//    threads.emplace_back(configReader, std::ref(config), 2);
//    
//    for (auto& t : threads) {
//        t.join();
//    }
//    
//    std::cout << "\n=== 最終設定 ===" << std::endl;
//    for (const auto& key : config.keys()) {
//        std::cout << key << " = " << config.getOrDefault(key, "") << std::endl;
//    }
//    
//    return 0;
//}
//```
//
//---
//
//### 八、設計模式：將鎖封裝在類別中
//
//```
//┌─────────────────────────────────────────────────────────────┐
//│              執行緒安全類別設計總結                          │
//├─────────────────────────────────────────────────────────────┤
//│                                                             │
//│  class ThreadSafeClass {                                    │
//│  private:                                                   │
//│      mutable std::mutex mtx;   // 1. 鎖是私有的             │
//│      Data data;                // 2. 資料是私有的           │
//│                                                             │
//│  public:                                                    │
//│      // 3. 禁止複製                                         │
//│      ThreadSafeClass(const ThreadSafeClass&) = delete;      │
//│      ThreadSafeClass& operator=(const ThreadSafeClass&) = delete;  │
//│                                                             │
//│      // 4. 所有公開方法都是原子的                           │
//│      void operation() {                                     │
//│          std::lock_guard<std::mutex> lock(mtx);             │
//│          // 操作 data                                       │
//│      }                                                      │
//│                                                             │
//│      // 5. 不返回內部資料的引用/指標                        │
//│      Data getData() const {                                 │
//│          std::lock_guard<std::mutex> lock(mtx);             │
//│          return data;  // 返回複本                          │
//│      }                                                      │
//│  };                                                         │
//│                                                             │
//└─────────────────────────────────────────────────────────────┘
//```
//
//---
//
//### 九、本課重點回顧
//
//1. **封裝原則**：鎖和資料放在同一類別，外部無法直接存取
//2. **禁止複製**：含有 mutex 的類別通常應禁止複製
//3. **mutable mutex**：允許在 const 方法中使用鎖
//4. **安全介面**：不返回內部資料的引用或指標
//5. **原子操作**：檢查和操作應在同一臨界區段
//6. **減少臨界區段**：只鎖定必要的部分
//7. **使用 scoped_lock**：同時鎖定多個互斥鎖時避免死結
//
//---
//
//### 下一課預告
//
//在 **課程 5.6：互斥鎖的效能考量** 中，我們將學習：
//- 鎖的開銷測量
//- 粗粒度鎖 vs 細粒度鎖
//- 鎖競爭的影響
//- 效能優化策略
//
//這是第五階段的最後一課，將為進入 RAII 鎖管理器做好準備！
//
//---
//
//準備好繼續嗎？
//*/
//



// 檔案：lesson_5_5_config_manager.cpp
// 說明：執行緒安全的設定管理器

#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>
#include <string>
#include <optional>
#include <vector>

class ConfigManager {
private:
    mutable std::mutex mtx;
    std::map<std::string, std::string> config;
    
public:
    // 設定值
    void set(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mtx);
        config[key] = value;
    }
    
    // 獲取值（使用 optional 表示可能不存在）
    std::optional<std::string> get(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = config.find(key);
        if (it != config.end()) {
            return it->second;
        }
        return std::nullopt;
    }
    
    // 獲取值，若不存在則返回預設值
    std::string getOrDefault(const std::string& key, 
                              const std::string& defaultValue) const {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = config.find(key);
        if (it != config.end()) {
            return it->second;
        }
        return defaultValue;
    }
    
    // 檢查是否存在
    bool has(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mtx);
        return config.find(key) != config.end();
    }
    
    // 刪除
    bool remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(mtx);
        return config.erase(key) > 0;
    }
    
    // 獲取所有 key
    std::vector<std::string> keys() const {
        std::lock_guard<std::mutex> lock(mtx);
        std::vector<std::string> result;
        result.reserve(config.size());
        for (const auto& [key, value] : config) {
            result.push_back(key);
        }
        return result;
    }
    
    // 批次設定
    void setAll(const std::map<std::string, std::string>& values) {
        std::lock_guard<std::mutex> lock(mtx);
        for (const auto& [key, value] : values) {
            config[key] = value;
        }
    }
    
    // 原子性的「若不存在則設定」
    bool setIfAbsent(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mtx);
        if (config.find(key) == config.end()) {
            config[key] = value;
            return true;
        }
        return false;
    }
    
    // 原子性的「比較並交換」
    bool compareAndSet(const std::string& key, 
                       const std::string& expected,
                       const std::string& newValue) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = config.find(key);
        if (it != config.end() && it->second == expected) {
            it->second = newValue;
            return true;
        }
        return false;
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx);
        return config.size();
    }
};


// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 146. LRU Cache
//   雙向串列 + 雜湊表，get / put 皆 O(1)；本版本加鎖成為執行緒安全。
//   不變量：① 雜湊表大小 == 串列長度
//           ② 雜湊表的每個 value 都指向串列中真實存在的節點
//   注意 get() 會修改使用順序，所以【不是 const】，也不能只用讀鎖。
// -----------------------------------------------------------------------------
class LRUCache {
private:
    struct LNode {
        int key;
        int value;
        LNode* prev = nullptr;
        LNode* next = nullptr;
        LNode(int k, int v) : key(k), value(v) {}
    };

    // 【注意】成員初始化順序依「宣告順序」，與初始化列表順序無關。
    mutable std::mutex mtx;
    std::map<int, LNode*> index;
    LNode* head = nullptr;      // 最近使用
    LNode* tail = nullptr;      // 最久未使用
    size_t capacity;
    long evictions = 0;

    // 以下私有方法都【假設呼叫者已持有鎖】——
    // 這是避免重複上鎖（UB）的標準做法：公開方法上鎖後呼叫不上鎖的私有方法。
    void detach(LNode* n) {
        if (n->prev) n->prev->next = n->next; else head = n->next;
        if (n->next) n->next->prev = n->prev; else tail = n->prev;
        n->prev = n->next = nullptr;
    }

    void pushFront(LNode* n) {
        n->next = head;
        n->prev = nullptr;
        if (head) head->prev = n;
        head = n;
        if (!tail) tail = n;
    }

public:
    explicit LRUCache(size_t cap) : capacity(cap) {}

    ~LRUCache() {
        while (head) { LNode* nxt = head->next; delete head; head = nxt; }
    }

    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;

    // 注意：不是 const —— 命中時會把節點搬到最前面
    int get(int key) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = index.find(key);
        if (it == index.end()) return -1;
        detach(it->second);         // ← 破壞期開始（節點不在串列上）
        pushFront(it->second);      // ← 破壞期結束
        return it->second->value;
    }

    void put(int key, int value) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = index.find(key);
        if (it != index.end()) {
            it->second->value = value;
            detach(it->second);
            pushFront(it->second);
            return;
        }
        if (index.size() >= capacity) {
            LNode* victim = tail;               // 淘汰最久未使用
            detach(victim);
            index.erase(victim->key);           // 兩個結構必須一起改
            delete victim;
            ++evictions;
        }
        LNode* n = new LNode(key, value);
        pushFront(n);
        index[key] = n;
    }

    // 可執行的不變量檢查：雜湊表大小必須等於串列長度
    bool invariantHolds() const {
        std::lock_guard<std::mutex> lock(mtx);
        size_t n = 0;
        for (const LNode* p = head; p; p = p->next) ++n;
        return n == index.size();
    }

    size_t size() const { std::lock_guard<std::mutex> lock(mtx); return index.size(); }
    long evicted() const { std::lock_guard<std::mutex> lock(mtx); return evictions; }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】用 compareAndSet 做無鎖重試的設定更新
//   情境：多個模組會同時調整同一個設定值（例如動態調整的執行緒池大小、
//         逐步放大的流量比例）。天真的做法是
//             int cur = std::stoi(cfg.getOrDefault("pool.size", "0"));
//             cfg.set("pool.size", std::to_string(cur + 1));
//         這是典型的 read-modify-write 競爭：兩個模組同時讀到 10，
//         都寫回 11，於是少算了一次。
//   正解：用 compareAndSet 做重試迴圈 ——
//         只有在「值仍是我讀到的那個」時才寫入，否則重讀重算。
//         這在不暴露鎖的前提下，讓呼叫端也能做出原子的複合操作。
// -----------------------------------------------------------------------------
bool incrementConfigCounter(ConfigManager& cfg, const std::string& key, int maxRetries = 10000) {
    for (int attempt = 0; attempt < maxRetries; ++attempt) {
        std::string cur = cfg.getOrDefault(key, "0");
        int next = std::stoi(cur) + 1;
        if (cfg.compareAndSet(key, cur, std::to_string(next))) {
            return true;                 // 成功：期間沒有人改過
        }
        // 失敗代表有人插隊改了值 → 重讀重算（這就是無鎖重試迴圈）
    }
    return false;
}

// 測試
void configWriter(ConfigManager& cfg, int id) {
    for (int i = 0; i < 5; ++i) {
        std::string key = "key_" + std::to_string(i);
        std::string value = "writer" + std::to_string(id) + "_value" + std::to_string(i);
        cfg.set(key, value);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// 註：原始版本在這裡直接 std::cout，兩個 reader 會互相插隊，
//     實測輸出會糊成「Reader 1: key_0 = Reader 2: key_0 = ...」這種難以閱讀的樣子
//     （鎖只保護 config，不保護 cout；而 cout << a << b 是多次獨立操作）。
//     改成先收集、由主執行緒統一輸出 —— 這是課程 5.6-5 的正式做法。
std::mutex readerOutMtx;
std::vector<std::string> readerLines;

void configReader(ConfigManager& cfg, int id) {
    std::vector<std::string> local;
    for (int i = 0; i < 5; ++i) {
        std::string key = "key_" + std::to_string(i);
        auto value = cfg.get(key);
        if (value) {
            local.push_back("Reader " + std::to_string(id) + ": " + key + " = " + *value);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    std::lock_guard<std::mutex> lock(readerOutMtx);
    for (auto& s : local) readerLines.push_back(std::move(s));
}

int main() {
    ConfigManager config;
    
    // 初始設定
    config.set("app.name", "MyApp");
    config.set("app.version", "1.0.0");
    
    std::vector<std::thread> threads;
    
    // 啟動寫入者
    threads.emplace_back(configWriter, std::ref(config), 1);
    threads.emplace_back(configWriter, std::ref(config), 2);
    
    // 啟動讀取者
    threads.emplace_back(configReader, std::ref(config), 1);
    threads.emplace_back(configReader, std::ref(config), 2);
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "讀取者共取得 " << readerLines.size()
              << " 筆設定（實際讀到 writer1 還是 writer2 的值取決於排程）" << std::endl;

    std::cout << "\n=== 最終設定 ===" << std::endl;
    for (const auto& key : config.keys()) {
        std::cout << key << " = " << config.getOrDefault(key, "") << std::endl;
    }

    std::cout << "\n=== setIfAbsent：呼叫端組合不出來的原子性 ===" << std::endl;
    {
        ConfigManager cfg;
        std::atomic<int> winners{0};
        std::vector<std::thread> ths;

        // 16 條執行緒同時嘗試「若不存在則設定」同一個 key
        for (int i = 0; i < 16; ++i) {
            ths.emplace_back([&cfg, &winners, i] {
                if (cfg.setIfAbsent("leader", "node-" + std::to_string(i))) {
                    winners.fetch_add(1);
                }
            });
        }
        for (auto& t : ths) t.join();

        std::cout << "16 條執行緒搶著設定 leader" << std::endl;
        std::cout << "成功次數: " << winners.load()
                  << "（必定為 1 —— 這正是分散式選主的基本語意）" << std::endl;
        std::cout << "最終 leader 是誰取決於排程（每次執行都可能不同），"
                     "但必定只有一個。" << std::endl;
    }

    std::cout << "\n=== compareAndSet：無鎖重試迴圈解決 RMW 競爭 ===" << std::endl;
    {
        ConfigManager cfg;
        cfg.set("counter", "0");

        std::vector<std::thread> ths;
        std::atomic<int> failed{0};
        for (int i = 0; i < 8; ++i) {
            ths.emplace_back([&cfg, &failed] {
                for (int k = 0; k < 500; ++k) {
                    if (!incrementConfigCounter(cfg, "counter")) failed.fetch_add(1);
                }
            });
        }
        for (auto& t : ths) t.join();

        std::cout << "8 執行緒 × 500 次遞增" << std::endl;
        std::cout << "counter = " << cfg.getOrDefault("counter", "?")
                  << "（必定為 4000，一次都不會少算）" << std::endl;
        std::cout << "重試次數用盡而失敗的次數: " << failed.load() << "（必定為 0）" << std::endl;
        std::cout << "→ 若改成 get + set，這裡會少算成千上百次" << std::endl;
    }

    std::cout << "\n=== LeetCode 146. LRU Cache ===" << std::endl;
    {
        LRUCache cache(2);
        cache.put(1, 1);
        cache.put(2, 2);
        std::cout << "get(1) = " << cache.get(1) << "  (預期 1)" << std::endl;
        cache.put(3, 3);            // 容量滿 → 淘汰最久未使用的 key 2
        std::cout << "get(2) = " << cache.get(2) << "  (預期 -1，已被淘汰)" << std::endl;
        cache.put(4, 4);            // 淘汰 key 1
        std::cout << "get(1) = " << cache.get(1) << "  (預期 -1，已被淘汰)" << std::endl;
        std::cout << "get(3) = " << cache.get(3) << "  (預期 3)" << std::endl;
        std::cout << "get(4) = " << cache.get(4) << "  (預期 4)" << std::endl;
        std::cout << "不變量（雜湊表大小 == 串列長度）: " << std::boolalpha
                  << cache.invariantHolds() << std::endl;

        // 多執行緒壓力測試：每個操作都是原子的，不變量恆成立
        LRUCache shared(64);
        std::vector<std::thread> ths;
        for (int i = 0; i < 8; ++i) {
            ths.emplace_back([&shared, i] {
                for (int k = 0; k < 5000; ++k) {
                    shared.put((i * 5000 + k) % 200, k);
                    shared.get((i * 7 + k) % 200);
                }
            });
        }
        for (auto& t : ths) t.join();

        std::cout << "8 執行緒 × 5000 次 put+get 後：" << std::endl;
        std::cout << "  快取大小: " << shared.size() << "（必定 = 容量 64）" << std::endl;
        std::cout << "  不變量仍成立: " << shared.invariantHolds()
                  << "（必定為 true）" << std::endl;
        std::cout << "→ get() 會搬動節點，所以它不是 const、也不能只用讀鎖" << std::endl;
    }
    
    return 0;
}
// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.5：保護共享資料實作7.cpp' -o config_manager
//   （std::optional 與結構化繫結需要 C++17）

// ⚠️「最終設定」中 key_0 ~ key_4 的值【每次執行都不同】——
// 兩個 writer 各自寫同一批 key，最後留下誰的值取決於排程。
// 本機連續三次實測就出現過 writer2 全拿、以及 writer1/writer2 混合等不同結果。
// 注意這【不是 data race】：ConfigManager 每個方法都有鎖，
// 每次寫入都是原子的、值也不會撕裂；這只是「誰最後寫」的良性競爭條件。
// 同理「最終 leader 是誰」也每次不同，但成功次數必定為 1。
// 下面貼的是本機某一次的真實實測。
//
// 其餘各段皆為確定值：compareAndSet 的 4000、LeetCode 146 的
// get 結果與不變量、壓力測試後的快取大小 64 —— 每次執行都相同。

// === 預期輸出 ===
// 讀取者共取得 10 筆設定（實際讀到 writer1 還是 writer2 的值取決於排程）
//
// === 最終設定 ===
// app.name = MyApp
// app.version = 1.0.0
// key_0 = writer2_value0
// key_1 = writer2_value1
// key_2 = writer1_value2
// key_3 = writer1_value3
// key_4 = writer1_value4
//
// === setIfAbsent：呼叫端組合不出來的原子性 ===
// 16 條執行緒搶著設定 leader
// 成功次數: 1（必定為 1 —— 這正是分散式選主的基本語意）
// 最終 leader 是誰取決於排程（每次執行都可能不同），但必定只有一個。
//
// === compareAndSet：無鎖重試迴圈解決 RMW 競爭 ===
// 8 執行緒 × 500 次遞增
// counter = 4000（必定為 4000，一次都不會少算）
// 重試次數用盡而失敗的次數: 0（必定為 0）
// → 若改成 get + set，這裡會少算成千上百次
//
// === LeetCode 146. LRU Cache ===
// get(1) = 1  (預期 1)
// get(2) = -1  (預期 -1，已被淘汰)
// get(1) = -1  (預期 -1，已被淘汰)
// get(3) = 3  (預期 3)
// get(4) = 4  (預期 4)
// 不變量（雜湊表大小 == 串列長度）: true
// 8 執行緒 × 5000 次 put+get 後：
//   快取大小: 64（必定 = 容量 64）
//   不變量仍成立: true（必定為 true）
// → get() 會搬動節點，所以它不是 const、也不能只用讀鎖
