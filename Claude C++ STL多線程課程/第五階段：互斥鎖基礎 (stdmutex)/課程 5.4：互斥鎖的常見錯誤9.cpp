// 檔案：lesson_5_4_exception_unsafe.cpp
// 說明：例外導致 unlock 未執行
//
// 【本檔是「刻意示範錯誤」的範例，開啟示範後程式會停住，不會自己結束】
//
// ⚠️ 錯誤性質：riskyOperation(-5) 拋出例外時跳過了 unlock()，
//    鎖仍被【主執行緒自己】持有；接著 riskyOperation(20) 又在
//    【同一個執行緒】對 std::mutex 呼叫 lock() → 這是【未定義行為】，
//    不是標準保證的「死結」。
//
//    實測（g++ 15.2 / glibc、Ubuntu 26.04）：印出 "開始處理 20" 之前
//    就永久停住，程式不會自行終止（timeout 觀察到 exit=124）。
//    這是本平台的實作結果，不是標準保證的結果。
//
// 為了不讓整份課程的批次執行卡死，這個示範預設【關閉】。
// 要親眼觀察，請自行加上 -DDEMONSTRATE_UB：
//    g++ -std=c++17 -pthread -DDEMONSTRATE_UB -o exception_unsafe '課程 5.4：互斥鎖的常見錯誤9.cpp'
//    timeout 5 ./exception_unsafe ; echo "exit=$?"   # 本平台預期 exit=124
//
// ✅ 正確作法：用 std::lock_guard —— 堆疊展開時解構函式一定會執行，
//    例外再怎麼拋，鎖都會被釋放（這就是 RAII 的價值）。

// =============================================================================
//  【教科書段落】為什麼 RAII 不是「風格偏好」，而是正確性的必要條件
// =============================================================================
//
// 【主題資訊 Information】
//   主題：    例外安全 (exception safety) 與 std::lock_guard
//   語法：    std::lock_guard<std::mutex> lock(mtx);   // 建構即鎖，解構即解
//   標準版本：std::mutex / std::lock_guard 為 C++11；std::scoped_lock 為 C++17
//   標頭檔：  <mutex>、<stdexcept>
//   關鍵規則：同一條執行緒對已持有的 std::mutex 再次 lock() → 【未定義行為】
//             （不是標準保證的死結；std::recursive_mutex 才允許重入）
//
// 【詳細解釋 Explanation】
//
// 【1. 手動 lock/unlock 為什麼「一定」會出事】
//   riskyOperation() 的結構是：
//       mtx.lock();
//       ... 中間可能 throw ...
//       mtx.unlock();          // ← 例外一拋，這一行永遠不會執行
//   問題不在於作者粗心，而在於【控制流的離開點不只一個】：
//     * 中途 return（多個離開點，總有一個會被漏掉）
//     * 拋出例外（堆疊展開直接跳過後面所有敘述）
//     * 後人維護時在中間插入一個 return
//   只要函式有 N 個離開點，手動配對就需要 N 個 unlock 都不漏。
//   這是「靠紀律維持的正確性」，而紀律在程式碼演化中必定會失效。
//
// 【2. RAII 為什麼能根治】
//   lock_guard 是自動儲存期物件，C++ 保證：
//   【物件離開作用域時，解構函式必定被呼叫】——
//   包含正常 return、break、goto，以及例外造成的堆疊展開 (stack unwinding)。
//   於是「解鎖」從一個「要記得執行的敘述」，
//   變成「由語言機制保證會發生的事件」。
//   把上面的函式改寫成：
//       {
//           std::lock_guard<std::mutex> lock(mtx);
//           ... 中間隨便怎麼 throw ...
//       }   // ← 不論怎麼離開，這裡一定解鎖
//   錯誤就從「可能發生」變成「不可能發生」。
//
// 【3. 為什麼「鎖沒釋放」的後果比想像中嚴重】
//   直覺會以為「頂多下次鎖不到，卡住而已」。實際上：
//     * 若是【別的執行緒】來鎖 → 它會永遠阻塞（本機實測會停住不返回）。
//     * 若是【同一條執行緒】再鎖（本檔的情形）→ 這是【未定義行為】，
//       不是「保證死結」。標準對 std::mutex 的規定是
//       「呼叫端執行緒不得已持有該 mutex」，違反即 UB。
//   而且在真實服務中，一條卡住的執行緒通常會連鎖反應：
//   執行緒池被耗盡 → 新請求排隊 → 逾時 → 上游重試 → 雪崩。
//   一個沒解開的鎖足以拖垮整個服務。
//
// 【4. 「那我在 catch 裡 unlock 不就好了」為什麼不行】
//   try { mtx.lock(); ...; mtx.unlock(); }
//   catch (...) { mtx.unlock(); throw; }
//   這樣寫確實能運作，但：
//     * 解鎖邏輯散在兩處，任何一處改動都要同步維護
//     * 若例外發生在 lock() 之前，catch 裡的 unlock 就變成「解一把沒鎖的鎖」→ UB
//     * 巢狀資源時會迅速演變成金字塔式的 try/catch
//   這正是 C++ 引入 RAII 的原因：把「清理」綁定到物件生命週期，
//   而不是綁定到控制流的每一條路徑。
//
// 【5. 例外安全的三個等級（Abrahams 保證）】
//   * 基本保證 (basic)：拋例外後物件仍處於合法狀態，無資源洩漏。
//   * 強保證 (strong)：拋例外後狀態完全不變，如同沒呼叫過（交易語意）。
//   * 不拋保證 (nothrow)：絕不拋出，通常標記 noexcept。
//   用了 lock_guard，「鎖一定會釋放」屬於【基本保證】的一部分。
//   但要注意：基本保證【不代表】資料是一致的 ——
//   若例外發生在「不變量被破壞到一半」時，鎖是解開了，
//   但別人拿到鎖後會看到破壞狀態。要達到強保證，
//   還需要「先在暫存區完成所有修改，最後用 noexcept 的 swap 一次生效」。
//   本檔第三段用一個轉帳的例子示範這個差別。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 std::mutex 不設計成可重入
//   可重入（recursive）需要額外記錄「擁有者 id + 遞迴計數」，
//   每次 lock/unlock 都要比對與增減，比 std::mutex 貴。
//   更重要的是設計上的理由：需要重入通常代表
//   「同一個不變量的保護範圍沒有想清楚」——
//   函式 A 持鎖時呼叫了也要鎖的函式 B，往往表示
//   應該把「不加鎖的內部版本」抽出來給 A 用。
//   Herb Sutter 有句名言：recursive_mutex 多半是設計問題的止痛藥。
//
// (B) 堆疊展開時解構函式若再拋例外會怎樣
//   若在例外展開途中，某個解構函式又拋出例外，
//   程式會呼叫 std::terminate()（這是【標準保證】的行為，不是 UB）。
//   這就是為什麼解構函式預設是 noexcept，也是為什麼
//   「不要在解構函式裡做可能失敗的事」是鐵律。
//   lock_guard 的解構只呼叫 unlock()（noexcept），所以絕對安全。
//
// (C) 本檔為什麼用 #ifdef 把 UB 關掉
//   讓整份課程可以批次編譯與執行而不會卡死。
//   這也示範了處理「示範用的危險程式碼」的正確做法：
//   保留它、說清楚它，但預設不執行 —— 而不是刪掉或假裝沒事。
//
// 【注意事項 Pay Attention】
// 1. 同一條執行緒對已持有的 std::mutex 再 lock() 是 UB，不是保證死結。
// 2. 手動 lock/unlock 在「多個離開點 + 例外」下必定失效，這是結構問題非紀律問題。
// 3. 在 catch 裡補 unlock 不是解法：邏輯分散，且例外若發生在 lock 前會變成解錯鎖。
// 4. lock_guard 保證「鎖會釋放」，但【不保證】資料一致；
//    不變量破壞到一半就拋例外，別人仍會看到壞掉的狀態。
// 5. 解構函式在堆疊展開中再拋例外 → std::terminate()（標準保證的行為）。
// 6. 需要重入請用 std::recursive_mutex，但先想想是不是設計有問題。
//
// 【LeetCode 說明】本檔不附 LeetCode 範例。
//   本檔主題是例外安全與 RAII，屬於 C++ 資源管理的語言機制；
//   允許使用的設計題（146/155/705/707/1603）與並行題（1114～1117/1195）
//   都不涉及例外處理，沒有一題在考「拋例外時鎖有沒有釋放」。
//   硬湊只會失焦，故從缺，改以下方兩個真實情境
//   （交易的強例外保證、連線池的 RAII 歸還）呈現。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】例外安全與 RAII
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼一定要用 lock_guard，而不是手動 lock() / unlock()?
//     答：因為函式的離開點不只一個 —— 中途 return、拋出例外、
//         後人維護時插入的 return，任何一條路徑漏掉 unlock，
//         鎖就永遠不會釋放。lock_guard 是自動儲存期物件，
//         C++ 保證它離開作用域時解構必定執行，包含例外造成的堆疊展開，
//         於是「解鎖」從「要記得寫的敘述」變成「語言保證的事件」。
//     追問：那我在 catch 區塊裡補 unlock 不行嗎?
//         → 能跑但不該這樣寫：解鎖邏輯散在兩處難以維護；
//           而且若例外發生在 lock() 之前，catch 裡的 unlock
//           就變成「解一把沒鎖的鎖」，那本身又是 UB。
//
// 🔥 Q2. 同一條執行緒對同一個 std::mutex 連續 lock() 兩次會怎樣?
//     答：未定義行為。標準規定呼叫 lock() 時該執行緒不得已持有這把 mutex。
//         很多人會說「死結」，但死結是【本平台觀察到的現象】而非標準保證 ——
//         本機（g++ 15.2 / glibc）實測是永久停住（timeout 觀察到 exit=124）。
//         需要重入語意請改用 std::recursive_mutex。
//     追問：那為什麼標準不讓 std::mutex 直接支援重入?
//         → 成本（要記錄擁有者與遞迴計數）與設計考量：
//           需要重入通常代表保護範圍沒設計好，
//           正解多半是把「不加鎖的內部版本」抽出來給持鎖者呼叫。
//
// ⚠️ 陷阱. 「我改用 lock_guard 了，所以我的函式是例外安全的」——錯在哪?
//     答：lock_guard 只保證【鎖會被釋放】，不保證【資料是一致的】。
//         若例外剛好發生在「不變量被破壞到一半」的時候，
//         鎖確實解開了，但下一個拿到鎖的人會看到壞掉的狀態 ——
//         而且因為鎖正常釋放，這個問題不會卡住，只會靜靜地擴散。
//     為什麼會錯：把「無資源洩漏」等同於「例外安全」。
//         Abrahams 的三個等級中，lock_guard 給的是【基本保證】；
//         要拿到【強保證】（狀態完全不變，如同沒呼叫過），
//         必須先在暫存區完成所有修改，最後用 noexcept 的 swap 一次生效。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <mutex>
#include <stdexcept>
#include <vector>
#include <string>

std::mutex mtx;

void riskyOperation(int value) {
    mtx.lock();
    
    std::cout << "開始處理 " << value << std::endl;
    
    if (value < 0) {
        throw std::invalid_argument("值不能為負數");  // 💀 例外拋出
        // unlock() 永遠不會執行！
    }
    
    std::cout << "處理完成" << std::endl;
    mtx.unlock();
}

// -----------------------------------------------------------------------------
// 【正確版】用 lock_guard 改寫 riskyOperation
//   一模一樣的邏輯，一模一樣的例外，但鎖必定被釋放 ——
//   所以後續呼叫完全正常，程式不會停住。
// -----------------------------------------------------------------------------
std::mutex safeMtx;

void safeOperation(int value) {
    std::lock_guard<std::mutex> lock(safeMtx);   // 建構即鎖

    std::cout << "開始處理 " << value << std::endl;

    if (value < 0) {
        throw std::invalid_argument("值不能為負數");
        // 堆疊展開時 lock 的解構必定執行 → 鎖一定被釋放
    }

    std::cout << "處理完成" << std::endl;
}                                                 // ← 正常路徑也在此解鎖

// -----------------------------------------------------------------------------
// 【日常實務範例 1】交易的強例外保證：光有 RAII 還不夠
//   情境：轉帳要扣款、入帳、寫交易紀錄三步。若在中途拋例外
//         （餘額不足、風控攔截、寫 log 失敗），
//         用了 lock_guard 鎖確實會釋放，但帳目可能停在
//         「已扣款、未入帳」的破壞狀態 —— 錢憑空消失。
//   基本保證（BasicAccount）：不洩漏資源，但狀態可能已被部分修改。
//   強保證（StrongAccount）：先在區域變數上算出完整的新狀態，
//         確認不會再失敗之後，才用 noexcept 的動作一次生效。
//         這是資料庫交易 (transaction) 在應用層的對應做法。
// -----------------------------------------------------------------------------
class BasicAccount {
private:
    mutable std::mutex mtx;
    long a = 1000;
    long b = 1000;
    std::vector<std::string> journal;

public:
    // 基本保證：鎖會釋放，但可能停在「已扣款、未入帳」
    void transferBasic(long amount, bool simulateFailure) {
        std::lock_guard<std::mutex> lock(mtx);
        a -= amount;                                   // ← 破壞期開始
        if (simulateFailure) {
            throw std::runtime_error("風控攔截");       // ← 停在破壞狀態！
        }
        b += amount;                                   // ← 破壞期結束
        journal.push_back("transfer " + std::to_string(amount));
    }

    long total() const { std::lock_guard<std::mutex> lock(mtx); return a + b; }
};

class StrongAccount {
private:
    mutable std::mutex mtx;
    long a = 1000;
    long b = 1000;
    std::vector<std::string> journal;

public:
    // 強保證：所有可能失敗的動作都在「生效」之前完成
    void transferStrong(long amount, bool simulateFailure) {
        std::lock_guard<std::mutex> lock(mtx);

        // ① 在區域變數上算出完整的新狀態（此時還沒動到任何成員）
        long newA = a - amount;
        long newB = b + amount;
        std::vector<std::string> newJournal = journal;          // 可能拋 bad_alloc
        newJournal.push_back("transfer " + std::to_string(amount));  // 可能拋

        // ② 所有可能失敗的檢查都放在生效之前
        if (simulateFailure) {
            throw std::runtime_error("風控攔截");   // 此時成員完全沒被改過
        }
        if (newA < 0) {
            throw std::runtime_error("餘額不足");
        }

        // ③ 生效：以下全部是 noexcept 的動作，不可能失敗到一半
        a = newA;
        b = newB;
        journal.swap(newJournal);      // vector::swap 是 noexcept
    }

    long total() const { std::lock_guard<std::mutex> lock(mtx); return a + b; }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】連線池：借出的連線必須在任何路徑上都歸還
//   情境：從池子借一條資料庫連線，執行查詢，歸還。
//         查詢可能拋例外（SQL 錯誤、連線中斷、逾時），
//         手寫 release() 一定會在某條路徑上被漏掉，
//         結果是連線池被慢慢耗盡 —— 這是線上服務最典型的「慢性死亡」：
//         剛上線好好的，跑幾小時後所有請求開始逾時。
//   正解：RAII 守衛，讓歸還由解構函式保證。
// -----------------------------------------------------------------------------
class ConnectionPool {
private:
    mutable std::mutex mtx;
    std::vector<std::string> available;
    int leaked = 0;

public:
    ConnectionPool() {
        for (int i = 0; i < 4; ++i) available.push_back("conn-" + std::to_string(i));
    }

    std::string acquire() {
        std::lock_guard<std::mutex> lock(mtx);
        if (available.empty()) return "";
        std::string c = available.back();
        available.pop_back();
        return c;
    }

    void release(const std::string& c) {
        std::lock_guard<std::mutex> lock(mtx);
        available.push_back(c);
    }

    size_t idle() const { std::lock_guard<std::mutex> lock(mtx); return available.size(); }
    int leakedCount() const { std::lock_guard<std::mutex> lock(mtx); return leaked; }
};

// RAII 守衛：解構必定歸還，與 lock_guard 完全相同的設計
class PooledConnection {
private:
    ConnectionPool& pool;
    std::string conn;

public:
    explicit PooledConnection(ConnectionPool& p) : pool(p), conn(p.acquire()) {}
    ~PooledConnection() { if (!conn.empty()) pool.release(conn); }

    PooledConnection(const PooledConnection&) = delete;
    PooledConnection& operator=(const PooledConnection&) = delete;

    bool valid() const { return !conn.empty(); }
    const std::string& name() const { return conn; }
};

// 這個查詢有三條離開點，其中一條是拋例外 —— 連線都必定歸還
void runQuery(ConnectionPool& pool, int id) {
    PooledConnection c(pool);
    if (!c.valid()) return;                       // 離開點 1
    if (id % 7 == 0) {
        throw std::runtime_error("SQL error");    // 離開點 2（堆疊展開時歸還）
    }
    // ... 正常執行查詢 ...
                                                  // 離開點 3
}

int main() {
    try {
        riskyOperation(10);   // OK
        riskyOperation(-5);   // 💀 拋出例外，鎖沒釋放
    } catch (const std::exception& e) {
        std::cout << "捕獲例外：" << e.what() << std::endl;
    }
    
    // 此時 mtx 仍處於鎖定狀態，而且持有者就是本執行緒！
    std::cout << "嘗試再次操作..." << std::endl;

#ifdef DEMONSTRATE_UB
    // 💀 同一執行緒對已持有的 std::mutex 再 lock() → 未定義行為。
    //    本平台實測：停在這裡，連 "開始處理 20" 都印不出來。
    riskyOperation(20);

    std::cout << "（本平台上這一行不會被執行）" << std::endl;
#else
    std::cout << "已略過會卡住的第三次呼叫（預設關閉）。" << std::endl;
    std::cout << "要重現請加上 -DDEMONSTRATE_UB 重新編譯。" << std::endl;
#endif

    std::cout << "\n=== 正確版：改用 lock_guard，鎖必定釋放 ===" << std::endl;
    try {
        safeOperation(10);    // OK
        safeOperation(-5);    // 拋例外，但 lock_guard 的解構會釋放鎖
    } catch (const std::exception& e) {
        std::cout << "捕獲例外：" << e.what() << std::endl;
    }
    // 關鍵差異：這裡不會卡住，因為鎖在堆疊展開時已被釋放
    safeOperation(20);
    std::cout << "→ 第三次呼叫正常完成，證明鎖確實被釋放了" << std::endl;

    std::cout << "\n=== 日常實務 1：基本保證 vs 強例外保證 ===" << std::endl;
    {
        BasicAccount basic;
        std::cout << "轉帳前 總額: " << basic.total() << std::endl;
        try {
            basic.transferBasic(300, true);   // 在扣款後、入帳前拋例外
        } catch (const std::exception& e) {
            std::cout << "捕獲例外：" << e.what() << std::endl;
        }
        std::cout << "基本保證 轉帳後 總額: " << basic.total()
                  << "  ← 錢憑空消失了 300（鎖有釋放，但狀態壞了）" << std::endl;

        StrongAccount strong;
        std::cout << "轉帳前 總額: " << strong.total() << std::endl;
        try {
            strong.transferStrong(300, true);
        } catch (const std::exception& e) {
            std::cout << "捕獲例外：" << e.what() << std::endl;
        }
        std::cout << "強保證   轉帳後 總額: " << strong.total()
                  << "  ← 完全沒變，如同從未呼叫過" << std::endl;

        // 強保證版本在成功時當然也要正確
        strong.transferStrong(300, false);
        std::cout << "強保證   成功轉帳後 總額: " << strong.total()
                  << " (守恆)" << std::endl;
    }

    std::cout << "\n=== 日常實務 2：連線池的 RAII 歸還 ===" << std::endl;
    {
        ConnectionPool pool;
        std::cout << "初始閒置連線: " << pool.idle() << std::endl;

        int errors = 0;
        for (int i = 1; i <= 100; ++i) {
            try {
                runQuery(pool, i);
            } catch (const std::exception&) {
                ++errors;    // 有些查詢會拋例外
            }
        }

        std::cout << "執行 100 次查詢，其中 " << errors << " 次拋出例外" << std::endl;
        std::cout << "結束後閒置連線: " << pool.idle()
                  << " (必定回到 4 —— RAII 保證每條路徑都歸還)" << std::endl;
        std::cout << "→ 若改成手寫 release()，拋例外那 " << errors
                  << " 次的連線就會永遠洩漏，" << std::endl;
        std::cout << "  池子被慢慢耗盡，服務跑幾小時後開始全面逾時。" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.4：互斥鎖的常見錯誤9.cpp' -o exception_unsafe
//   （預設不啟用 UB 示範，可安全執行）
//
// 觀察 UB（會停住不返回，請自行加 timeout）:
//   g++ -std=c++17 -pthread -DDEMONSTRATE_UB '課程 5.4：互斥鎖的常見錯誤9.cpp' -o exception_ub
//   timeout 5 ./exception_ub ; echo "exit=$?"   # 本平台預期 exit=124

// 註：預設編譯（未加 -DDEMONSTRATE_UB）不會執行任何 UB，
// 輸出為確定值，每次執行完全相同（本機連續三次實測 md5 一致）。
// 加上 -DDEMONSTRATE_UB 後會停在「嘗試再次操作...」之後不返回，
// 本機以 timeout 5 實測 exit=124（這是本平台的實作結果，不是標準保證）。

// === 預期輸出 ===
// 開始處理 10
// 處理完成
// 開始處理 -5
// 捕獲例外：值不能為負數
// 嘗試再次操作...
// 已略過會卡住的第三次呼叫（預設關閉）。
// 要重現請加上 -DDEMONSTRATE_UB 重新編譯。
//
// === 正確版：改用 lock_guard，鎖必定釋放 ===
// 開始處理 10
// 處理完成
// 開始處理 -5
// 捕獲例外：值不能為負數
// 開始處理 20
// 處理完成
// → 第三次呼叫正常完成，證明鎖確實被釋放了
//
// === 日常實務 1：基本保證 vs 強例外保證 ===
// 轉帳前 總額: 2000
// 捕獲例外：風控攔截
// 基本保證 轉帳後 總額: 1700  ← 錢憑空消失了 300（鎖有釋放，但狀態壞了）
// 轉帳前 總額: 2000
// 捕獲例外：風控攔截
// 強保證   轉帳後 總額: 2000  ← 完全沒變，如同從未呼叫過
// 強保證   成功轉帳後 總額: 2000 (守恆)
//
// === 日常實務 2：連線池的 RAII 歸還 ===
// 初始閒置連線: 4
// 執行 100 次查詢，其中 14 次拋出例外
// 結束後閒置連線: 4 (必定回到 4 —— RAII 保證每條路徑都歸還)
// → 若改成手寫 release()，拋例外那 14 次的連線就會永遠洩漏，
//   池子被慢慢耗盡，服務跑幾小時後開始全面逾時。
