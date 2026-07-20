// =============================================================================
//  課程 5.4：互斥鎖的常見錯誤6.cpp  —  std::recursive_mutex（以及它的代價）
// =============================================================================
//
// 【主題資訊 Information】
//   class std::recursive_mutex;                                    // C++11，<mutex>
//       void lock();        // 同一執行緒可重複呼叫，內部維護「遞迴計數」
//       bool try_lock();
//       void unlock();      // 必須與 lock() 呼叫次數【完全配對】
//   class std::recursive_timed_mutex;                              // C++11
//   物件大小（實作定義）：本機 sizeof(std::recursive_mutex) = 40 bytes
//   ⚠️ 標準規定：同一執行緒可鎖定的次數有【上限】（unspecified），
//      超過時 lock() 會丟 std::system_error、try_lock() 會回傳 false。
//
// 【詳細解釋 Explanation】
//
// 【1. 它解決什麼問題：同一執行緒的重入】
//   std::mutex 是【不可重入】的：同一條執行緒對已持有的 mutex 再次 lock()
//   是未定義行為（前一個檔案示範了這件事）。
//   典型的觸發場景是「加了鎖的公開函式互相呼叫」：
//       void outer() { mtx.lock(); ...; inner(); ...; mtx.unlock(); }
//       void inner() { mtx.lock(); ...; mtx.unlock(); }   // 💀 UB
//   recursive_mutex 在內部多維護兩樣東西：
//       * 目前持有者的執行緒 id
//       * 一個遞迴計數（recursion count）
//   lock() 時若發現持有者就是自己，就只把計數 +1 並立刻返回；
//   unlock() 時計數 -1，直到歸零才真正釋放鎖。
//
// 【2. 「lock 幾次就要 unlock 幾次」——這是硬性要求】
//   遞迴計數必須精確歸零，鎖才會被釋放。
//       rmtx.lock();  rmtx.lock();      // 計數 = 2
//       rmtx.unlock();                  // 計數 = 1 → 鎖【仍被持有】
//   少 unlock 一次，鎖就永遠不會釋放，其他執行緒永久阻塞。
//   → 這也是為什麼即使用 recursive_mutex，也應該搭配
//     std::lock_guard<std::recursive_mutex>：讓計數的增減由 RAII 配對，
//     人類不必自己數。
//
// 【3. 為什麼多數 C++ 專家把它視為「設計警訊」】
//   recursive_mutex 能讓程式碼「跑起來」，但它通常掩蓋了真正的問題：
//     (a) 【不變量的破口】：outer() 在中途呼叫 inner() 時，
//         被保護的資料很可能正處於「不變量暫時被破壞」的中間狀態。
//         inner() 拿得到鎖，於是它在一個【不完整的狀態】上工作，
//         而且不會有任何徵兆。用 std::mutex 至少會當場爆掉。
//     (b) 【鎖的範圍變得不可知】：看到 rmtx.lock() 無法判斷
//         「我是第一層還是第三層」，也就無法推理臨界區段到底有多長。
//     (c) 【效能較差】：每次 lock 都要多比對一次執行緒 id、多維護計數。
//   → Herb Sutter 與 Butler Lampson 的經典建議：
//     需要 recursive_mutex，通常代表【該重構了】——
//     把「假設鎖已持有」的邏輯抽成私有函式，由公開函式統一上鎖。
//     下一個檔案（常見錯誤7）就是這個重構的完整示範。
//
// 【4. 那什麼時候用它才是合理的？】
//   它不是禁忌，在這些情況下是務實的選擇：
//     * 遞迴演算法要走訪一個受保護的樹狀結構，且拆解成
//       「無鎖版本 + 上鎖入口」會讓程式碼明顯更難讀；
//     * 維護既有程式碼，重構的風險高於收益（先讓它正確，再談重構）；
//     * 回呼（callback）機制：你把控制權交給使用者的函式，
//       而它可能回過頭來呼叫你的公開 API。
//   ⚠️ 但最後這一種其實有更好的解法：在【呼叫回呼之前先解鎖】，
//     避免使用者的程式碼在你持有鎖時執行——那同時也能防止死結。
//
// 【概念補充 Concept Deep Dive】
//   * 底層對應：POSIX 的 PTHREAD_MUTEX_RECURSIVE 型別。
//     glibc 的實作就是在 pthread_mutex_t 裡存 owner 與 __count 欄位，
//     這也是為什麼它的 sizeof 與 std::mutex 相同（欄位本來就在那）。
//   * 遞迴次數的上限是 unspecified 的。實作上通常是一個很大的整數
//     （glibc 用 unsigned int），正常程式碼不可能達到；
//     但標準明確規定超過時的行為是「丟 system_error」而非 UB。
//   * ⚠️ std::condition_variable【不能】搭配 recursive_mutex：
//     cv.wait() 只放開一層鎖，若你遞迴鎖了三層，等待期間仍持有兩層，
//     其他執行緒永遠拿不到鎖 → 死結。
//     condition_variable_any 語法上接受它，但同樣的邏輯問題依然存在。
//     這是 recursive_mutex 一個很少被提及卻很致命的限制。
//
// 【注意事項 Pay Attention】
//   1. lock() 與 unlock() 的次數必須【完全配對】，少一次鎖就不會釋放。
//   2. 用 std::lock_guard<std::recursive_mutex> 讓 RAII 幫你配對計數。
//   3. 遞迴深度有實作定義的上限，超過會丟 std::system_error（不是 UB）。
//   4. ⚠️ 不要與 std::condition_variable 搭配使用（wait 只放開一層）。
//   5. 「需要 recursive_mutex」通常是重構的訊號，不是解法本身。
//   6. 遞迴期間被保護的資料可能處於不變量被破壞的中間狀態，要格外小心。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::recursive_mutex
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. recursive_mutex 是怎麼做到「同一執行緒可以重複鎖定」的？
//     答：它在內部多維護「持有者的執行緒 id」與一個「遞迴計數」。
//         lock() 時先比對持有者是不是自己：是就把計數 +1 立刻返回，
//         不是就走一般的取鎖流程。unlock() 把計數 -1，
//         直到【歸零】才真正釋放鎖給其他執行緒。
//         因此 lock 與 unlock 的呼叫次數必須完全配對。
//     追問：遞迴次數有上限嗎？→ 有，但上限值是 unspecified。
//           標準規定超過上限時 lock() 丟 std::system_error、
//           try_lock() 回傳 false——是明確定義的行為，不是 UB。
//
// 🔥 Q2. 既然 recursive_mutex 這麼方便，為什麼不乾脆都用它？
//     答：三個理由。效能上它每次 lock 都要多比對執行緒 id、多維護計數；
//         更重要的是設計上——它會【掩蓋問題】：
//         外層函式在中途呼叫內層時，被保護的資料常處於「不變量暫時被破壞」
//         的中間狀態，內層卻能順利拿到鎖並在不完整的狀態上工作，
//         而且毫無徵兆。用 std::mutex 至少會當場出錯。
//         此外，看到一個 lock() 無法判斷自己在第幾層，臨界區段變得無法推理。
//     追問：那正確的做法是什麼？→ 重構：把實際邏輯抽成
//           「假設鎖已持有」的私有函式（慣例命名如 doXxxLocked()），
//           由公開函式負責統一上鎖後呼叫它。
//
// ⚠️ 陷阱. recursive_mutex 可以和 std::condition_variable 一起用嗎？
//     答：不行。cv.wait() 的機制是「在等待期間放開鎖、被喚醒後重新取得」，
//         但它只會放開【一層】。如果你已經遞迴鎖了三層，
//         wait() 期間仍然持有兩層，其他執行緒永遠拿不到鎖來改變條件、
//         也就永遠不會發出通知 → 死結。
//     為什麼會錯：以為「recursive_mutex 只是可重入的 mutex，
//         其他行為完全一樣」。它在【所有權轉移】的語意上與 mutex 不同，
//         而 condition_variable 的正確性正好建立在「wait 會完全放開鎖」
//         這個前提上。（std::condition_variable_any 在語法上接受
//         recursive_mutex，但邏輯上的死結問題依然存在。）
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔【略過】，理由如下：
//   recursive_mutex 的存在意義是處理「同一執行緒的重入」，
//   而 LeetCode 的並行題（1114 / 1115 / 1116 / 1117 / 1195）全部是
//   【不同執行緒之間】的順序協調，每條執行緒只呼叫一次自己的函式，
//   根本不存在重入情境。硬掛一題完全無法示範本檔重點，故從缺。

#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

std::recursive_mutex rmtx;  // ✓ 遞迴互斥鎖

void innerFunction() {
    rmtx.lock();  // ✓ 同一執行緒可以再次鎖定（計數 +1）
    std::cout << "Inner function" << std::endl;
    rmtx.unlock();  // 計數 -1
}

void outerFunction() {
    rmtx.lock();  // 計數 = 1
    std::cout << "Outer function" << std::endl;

    innerFunction();  // ✓ 正常運作（計數短暫變成 2）

    rmtx.unlock();  // 計數 = 0 → 鎖真正釋放
}

// -----------------------------------------------------------------------------
// 【推薦寫法】即使用 recursive_mutex，也該讓 RAII 幫你配對計數
//   手動數 lock/unlock 的次數是人類最不擅長的事之一。
// -----------------------------------------------------------------------------
int recursiveSum(int n) {
    std::lock_guard<std::recursive_mutex> lock(rmtx);   // 每層各一個 guard
    if (n <= 0) return 0;
    return n + recursiveSum(n - 1);                     // 遞迴：計數層層 +1
}   // 每一層解構時計數 -1，最外層歸零才真正釋放

// -----------------------------------------------------------------------------
// 【日常實務範例】受保護的樹狀結構：遞迴走訪
//   情境：一個檔案系統目錄樹（或組織架構圖、UI 元件樹），
//         整棵樹由一把鎖保護，需要提供「計算某節點底下的總大小」這種
//         天生就是遞迴的操作。
//   為什麼這裡 recursive_mutex 是合理的：
//         走訪本身就是遞迴的，硬拆成「無鎖遞迴 + 上鎖入口」在這個
//         小例子裡確實可行（下一檔會示範），但當樹的操作有十幾種時，
//         每一種都要維護一對 public/private 函式，維護成本很高。
//   ⚠️ 但仍要留意：遞迴期間整棵樹都被鎖住，其他執行緒完全無法存取。
//         若樹很大，應考慮更細粒度的鎖（每個節點一把）或讀寫鎖。
// -----------------------------------------------------------------------------
struct DirNode {
    std::string name;
    long        sizeBytes = 0;              // 檔案自身大小（目錄為 0）
    std::vector<DirNode> children;
};

class FileTree {
private:
    mutable std::recursive_mutex mtx_;
    DirNode root_;

    // 注意：這個函式自己也會上鎖，而且會被 totalSize() 在持鎖狀態下呼叫
    long sizeOf(const DirNode& node) const {
        std::lock_guard<std::recursive_mutex> lock(mtx_);   // 重入
        long total = node.sizeBytes;
        for (const auto& child : node.children) {
            total += sizeOf(child);                          // 遞迴 → 再次上鎖
        }
        return total;
    }

public:
    explicit FileTree(DirNode root) : root_(std::move(root)) {}

    long totalSize() const {
        std::lock_guard<std::recursive_mutex> lock(mtx_);
        return sizeOf(root_);        // 在持鎖狀態下呼叫也會上鎖的函式
    }

    int countNodes() const {
        std::lock_guard<std::recursive_mutex> lock(mtx_);
        return countNodesImpl(root_);
    }

private:
    int countNodesImpl(const DirNode& node) const {
        std::lock_guard<std::recursive_mutex> lock(mtx_);
        int n = 1;
        for (const auto& child : node.children) {
            n += countNodesImpl(child);
        }
        return n;
    }
};

int main() {
    std::cout << "=== 課程示範: recursive_mutex 允許同一執行緒重入 ===" << std::endl;
    outerFunction();  // ✓ 正常執行
    std::cout << "完成" << std::endl;

    std::cout << "\n=== 物件大小（實作定義）===" << std::endl;
    std::cout << "sizeof(std::mutex)           = " << sizeof(std::mutex)
              << " bytes" << std::endl;
    std::cout << "sizeof(std::recursive_mutex) = " << sizeof(std::recursive_mutex)
              << " bytes" << std::endl;
    std::cout << "註: 本機兩者相同——glibc 的 pthread_mutex_t 本來就有 "
                 "owner 與 count 欄位" << std::endl;

    std::cout << "\n=== 用 RAII 讓遞迴計數自動配對 ===" << std::endl;
    std::cout << "recursiveSum(100) = " << recursiveSum(100)
              << "  (預期 5050，遞迴 101 層都成功取得同一把鎖)" << std::endl;
    // 若上面任何一層漏了 unlock，這裡就會永久阻塞
    std::cout << "遞迴結束後鎖已完全釋放，可再次取得: "
              << (rmtx.try_lock() ? "是" : "否") << std::endl;
    rmtx.unlock();

    std::cout << "\n=== 日常實務: 受保護的目錄樹遞迴走訪 ===" << std::endl;
    {
        DirNode root{"/", 0, {
            DirNode{"bin", 0, {
                DirNode{"ls",   140000, {}},
                DirNode{"grep", 210000, {}},
            }},
            DirNode{"home", 0, {
                DirNode{"user", 0, {
                    DirNode{"notes.md",  4096, {}},
                    DirNode{"photo.jpg", 2500000, {}},
                }},
            }},
            DirNode{"README", 1024, {}},
        }};

        FileTree tree(std::move(root));
        std::cout << "節點總數: " << tree.countNodes()
                  << "  (預期 9)" << std::endl;
        std::cout << "總大小:   " << tree.totalSize()
                  << " bytes  (預期 2855120)" << std::endl;

        // 多執行緒同時查詢：recursive_mutex 對【不同執行緒】仍是互斥的
        std::vector<std::thread> readers;
        std::vector<long> results(4, 0);
        for (int i = 0; i < 4; ++i) {
            readers.emplace_back([&tree, &results, i]() {
                for (int k = 0; k < 200; ++k) {
                    results[i] = tree.totalSize();
                }
            });
        }
        for (auto& t : readers) t.join();

        bool allSame = true;
        for (long r : results) {
            if (r != 2855120) allSame = false;
        }
        std::cout << "4 條執行緒各查詢 200 次，結果全部一致且正確: "
                  << (allSame ? "是" : "否") << std::endl;
        std::cout << "重點: 可重入只對【同一條】執行緒成立；"
                     "對其他執行緒它仍然是互斥的" << std::endl;
    }

    std::cout << "\n=== 設計提醒 ===" << std::endl;
    std::cout << "「需要 recursive_mutex」通常是重構的訊號——" << std::endl;
    std::cout << "把邏輯抽成「假設鎖已持有」的私有函式，由公開函式統一上鎖。"
              << std::endl;
    std::cout << "完整示範見同課「常見錯誤7.cpp」。" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.4：互斥鎖的常見錯誤6.cpp' -o recursive_mutex

// -----------------------------------------------------------------------------
// 【輸出但書】
//   1. 兩個 sizeof 是【實作定義】的（本機 x86-64 / glibc / libstdc++）。
//   2. 其餘輸出完全確定：遞迴計算與樹的走訪都是純函式，
//      多執行緒區段只驗證「結果一致且正確」這個不變量。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// === 課程示範: recursive_mutex 允許同一執行緒重入 ===
// Outer function
// Inner function
// 完成
//
// === 物件大小（實作定義）===
// sizeof(std::mutex)           = 40 bytes
// sizeof(std::recursive_mutex) = 40 bytes
// 註: 本機兩者相同——glibc 的 pthread_mutex_t 本來就有 owner 與 count 欄位
//
// === 用 RAII 讓遞迴計數自動配對 ===
// recursiveSum(100) = 5050  (預期 5050，遞迴 101 層都成功取得同一把鎖)
// 遞迴結束後鎖已完全釋放，可再次取得: 是
//
// === 日常實務: 受保護的目錄樹遞迴走訪 ===
// 節點總數: 9  (預期 9)
// 總大小:   2855120 bytes  (預期 2855120)
// 4 條執行緒各查詢 200 次，結果全部一致且正確: 是
// 重點: 可重入只對【同一條】執行緒成立；對其他執行緒它仍然是互斥的
//
// === 設計提醒 ===
// 「需要 recursive_mutex」通常是重構的訊號——
// 把邏輯抽成「假設鎖已持有」的私有函式，由公開函式統一上鎖。
// 完整示範見同課「常見錯誤7.cpp」。
