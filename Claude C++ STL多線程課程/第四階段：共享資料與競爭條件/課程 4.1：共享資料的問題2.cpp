// =============================================================================
//  課程 4.1：共享資料的問題2.cpp  —  check-then-act：轉帳為什麼會「憑空生錢」
// =============================================================================
//
// 【本檔是「刻意示範錯誤」的範例，不要照抄到實際專案】
//   transfer() 對 accountA / accountB 讀寫完全沒有同步 → genuine data race
//   → C++ 標準 [intro.races] 直接判為【未定義行為 (UB)】。
//   因此本檔【不會】宣稱「總額一定不是 2000」——UB 沒有固定結果。
//
// 【主題資訊 Information】
//   主題：    共享資料 (shared data) 的三種存取組合，以及 check-then-act 競爭模式
//   語法：    if (from.balance >= amount) { from.balance -= amount; to.balance += amount; }
//   標準版本：std::thread 為 C++11；C++11 才首次為 C++ 定義記憶體模型與 data race
//   標頭檔：  <thread>（std::thread）、<iostream>
//   複雜度：  每執行緒 O(N) 次轉帳；本檔重點是「正確性」而非複雜度
//   偵測工具：g++ -fsanitize=thread -g -pthread（ThreadSanitizer，最可靠）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「共享」本身不是罪，「共享 + 寫入」才是】
//   同一份資料被多條執行緒看到，安全與否只取決於有沒有人寫：
//     * 多執行緒同時「讀」          → 安全。沒有任何一方改變記憶體內容，
//                                     每次讀到的都是同一個值，不存在衝突。
//     * 一寫 + 一讀                 → 資料競爭。讀者可能讀到「寫到一半」的值。
//     * 多執行緒同時「寫」          → 資料競爭。互相覆蓋，更新遺失。
//   本檔兩條執行緒都在寫 balance，且互相讀對方的 balance → 屬於最危險的第三種。
//
// 【2. 本檔的錯誤不只是「counter++ 不是原子」那一種】
//   4.4 會講的 `counter++` 是 read-modify-write（RMW）：單一變數、三個步驟。
//   本檔則是更上一層的【check-then-act（先檢查、再行動）】：
//       if (from.balance >= amount) {   // ← 檢查（A）
//           from.balance -= amount;     // ← 行動（B）
//           to.balance   += amount;
//       }
//   即使把 balance 換成 std::atomic<int>，讓 A 和 B 各自都變成原子操作，
//   這段程式碼【仍然是錯的】：因為 A 與 B 之間有一個時間縫隙，
//   另一條執行緒可以在縫隙中把餘額改掉，使 A 的結論在 B 執行時已經過期。
//   → 這就是「原子操作不等於執行緒安全」最經典的反例，面試極常考。
//
// 【3. 「錢憑空產生」是怎麼發生的】
//   關鍵在於 `from.balance -= amount;` 與 `to.balance += amount;` 是兩個
//   獨立的 RMW，而且兩條執行緒操作的是同一組帳戶（只是方向相反）：
//       T1 讀 accountB.balance = 1000
//       T2 讀 accountB.balance = 1000      ← 兩者讀到同一個舊值
//       T1 寫 accountB.balance = 1001      （T1 是 A→B，B 收錢）
//       T2 寫 accountB.balance =  999      （T2 是 B→A，B 付錢）
//   T1 的 +1 被 T2 的計算整個覆蓋掉，帳目就不再守恆。反過來交錯的話，
//   總額也可能【變多】——這比「變少」更可怕，因為它看起來像系統送錢給你。
//
// 【4. 不變量 (invariant) 的角度】
//   這個系統的不變量是「accountA.balance + accountB.balance == 2000」。
//   轉帳這個「複合操作」必然會暫時破壞它（錢已扣、還沒入帳）。
//   單執行緒下沒關係，因為沒有人能在中途觀察；一旦多執行緒，
//   別人就可能在破壞期間讀到、甚至基於這個壞掉的狀態再做決策。
//   （這正是課程 4.2 的主題。）
//
// 【5. 正確作法】
//   把「檢查 + 兩次修改」整段包成一個不可分割的臨界區段：
//       std::mutex mtx;                       // 保護整組帳戶
//       std::lock_guard<std::mutex> lk(mtx);  // 整個 transfer() 一把鎖
//   若兩個帳戶各有自己的鎖，必須用 std::scoped_lock 一次鎖住兩把（C++17），
//   否則 A→B 與 B→A 反向加鎖會死結（見課程 5.5 的 BankAccount 範例）。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼「跑出 2000」不能當作程式沒問題的證據
//   本機實測（Ubuntu 26.04 / g++ 15.2.0 / 16 核心）連續執行本檔多次，
//   相當高比例會印出剛好 2000。原因是這個迴圈太短（各 1000 次）、
//   兩條執行緒的建立時間差常常大於整段迴圈的執行時間，
//   於是兩者根本沒有真正重疊執行 → 撞不到競爭視窗。
//   「測不出來」和「沒有錯」是兩件完全不同的事：UB 依然存在，
//   換機器、換最佳化等級、加大負載，隨時會現形。
//   → 要證明有沒有 data race，請用 ThreadSanitizer，不要用肉眼統計。
//
// (B) 為什麼 ThreadSanitizer 抓得到、而肉眼抓不到
//   TSan 不是靠「跑很多次碰運氣」，而是替每個記憶體位置維護 shadow memory 與
//   向量時鐘 (vector clock)，直接判定兩次存取之間【有沒有 happens-before 關係】。
//   只要執行過一次衝突路徑就會報警，不需要真的觀察到錯誤數值。
//   代價是記憶體約 5～10 倍、速度約 2～20 倍。
//
// (C) 這段程式碼在組譯層看到的東西
//   `from.balance -= amount` 在 -O0 下是 load / sub / store 三道指令，
//   沒有 LOCK 前綴，也就沒有任何原子性保證。即使某些平台上單一 int 的
//   load/store 恰好不會被撕裂 (tearing)，讀到「舊值」這件事仍然照樣發生。
//
// 【注意事項 Pay Attention】
// 1. 本檔是 UB 示範：任何「一定會…」的敘述都是錯的，包含「一定小於 2000」。
// 2. 把 int 換成 std::atomic<int> 並【不能】修好本檔——問題出在
//    check-then-act 的跨變數複合操作，不是單一變數的原子性。
// 3. 總額可能變多也可能變少；變多的情形在金融系統中屬於嚴重資安事故。
// 4. 執行緒建立成本（本機約數十 μs）常常大於這種小迴圈，
//    造成「測起來都正常」的假象；壓力測試請加大迴圈與執行緒數。
// 5. 正確解法是鎖住整段複合操作；若涉及兩把鎖，用 std::scoped_lock（C++17）
//    避免反向加鎖造成死結。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】共享資料與 check-then-act 競爭
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 把 balance 改成 std::atomic<int>，這段轉帳程式就安全了嗎?
//     答：不安全。atomic 只保證「單一變數的單一操作」不可分割，
//         但這裡是 check(讀 from) → act(寫 from、寫 to) 的跨變數複合操作，
//         三個原子步驟之間仍有縫隙，別人可以插進來把前提改掉。
//         正確做法是用 mutex 把整段包成一個臨界區段。
//     追問：那什麼時候 atomic 才真的夠用?
//         → 當「整個操作」本身就是單一個原子指令時，例如計數器
//           counter.fetch_add(1)，或用 compare_exchange_weak 寫 CAS 迴圈
//           把 check 與 act 合併成一次不可分割的嘗試。
//
// 🔥 Q2. 多執行緒同時讀取同一份資料需要加鎖嗎?
//     答：只要保證整個生命週期內沒有任何執行緒寫入，就不需要。
//         唯讀共享（const 資料、初始化完成後不再改的設定）是安全的，
//         這也是 immutable 設計在並行程式中特別受歡迎的原因。
//     追問：那 const 成員函式一定安全嗎?
//         → 不一定。const 只是語言層的承諾，若內部有 mutable 快取、
//           lazy 初始化或計數器，實際上仍在寫入，一樣要保護。
//
// ⚠️ 陷阱. 「我跑了 20 次總額都剛好 2000，所以這段程式沒問題」——錯在哪?
//     答：資料競爭是 UB，不是「一定會出錯」。跑對只代表這次沒撞上競爭視窗。
//         本檔迴圈很短，兩條執行緒常常根本沒有重疊執行，
//         測試環境幾乎測不出來；上線後負載一高就爆。
//     為什麼會錯：多數人腦中把並行 bug 當成「有機率出現的錯誤答案」，
//         用重複執行來否證。但 UB 的定義是「標準不再對程式的任何行為負責」，
//         包含「看起來完全正常」。判定有沒有 data race 要用工具
//         (ThreadSanitizer) 做形式化判定，不能用抽樣統計。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <map>

// -----------------------------------------------------------------------------
// 【錯誤示範】沒有任何同步的轉帳 —— 這是本課的主角
// -----------------------------------------------------------------------------
struct Account {
    int balance = 1000;
};

Account accountA, accountB;

void transfer(Account& from, Account& to, int amount) {
    if (from.balance >= amount) {   // ← check（檢查）
        from.balance -= amount;     // ← act（行動）：與 check 之間有縫隙
        to.balance += amount;
    }
}

// -----------------------------------------------------------------------------
// 【正確版】用一把鎖保護「整段」複合操作
//   注意鎖的是「整組帳戶」，不是單一變數 —— 不變量橫跨兩個變數，
//   所以臨界區段也必須橫跨兩個變數。
// -----------------------------------------------------------------------------
struct SafeBank {
    std::mutex mtx;
    int a = 1000;
    int b = 1000;

    void transfer(bool aToB, int amount) {
        std::lock_guard<std::mutex> lock(mtx);   // check 與 act 一起受保護
        int& from = aToB ? a : b;
        int& to   = aToB ? b : a;
        if (from >= amount) {
            from -= amount;
            to   += amount;
        }
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1603. Design Parking System
//   題目：停車場有 big/medium/small 三種車位，各有固定數量。
//         addCar(carType) 若還有空位就停入並回傳 true，否則回傳 false。
//   為什麼用到本主題：addCar 的形狀和本檔的 transfer 一模一樣 ——
//         「先檢查剩餘量是否足夠，再扣減」。LeetCode 的判題是單執行緒，
//         但真實停車場閘門是多執行緒的：兩台車同時刷卡讀到「剩 1 位」，
//         就會雙雙放行、停進同一格。這裡示範把 check-then-act
//         整段放進臨界區段的正確寫法。
// -----------------------------------------------------------------------------
class ParkingSystem {
private:
    // 【注意】成員初始化順序由「宣告順序」決定，與初始化列表的順序無關；
    //   這裡刻意讓宣告順序 = 初始化順序，避免 -Wreorder 類的陷阱。
    mutable std::mutex mtx;
    int slots[3];

public:
    ParkingSystem(int big, int medium, int small) : slots{big, medium, small} {}

    // carType: 1=big, 2=medium, 3=small
    bool addCar(int carType) {
        std::lock_guard<std::mutex> lock(mtx);   // 檢查與扣減必須同在鎖內
        int idx = carType - 1;
        if (idx < 0 || idx > 2) return false;
        if (slots[idx] > 0) {
            --slots[idx];
            return true;
        }
        return false;
    }

    int remaining(int carType) const {
        std::lock_guard<std::mutex> lock(mtx);
        return slots[carType - 1];
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】電商下單扣庫存（超賣事故的成因與修法）
//   情境：秒殺活動，多個下單執行緒同時對同一個 SKU 扣庫存。
//         錯誤版本先 if (stock >= qty) 再 stock -= qty，
//         高併發下會賣出超過實際庫存的數量（超賣），
//         這是電商系統最典型的 check-then-act 事故。
//   修法：把「檢查庫存 + 扣減 + 記錄訂單」整段放進同一個臨界區段。
// -----------------------------------------------------------------------------
class Inventory {
private:
    mutable std::mutex mtx;
    std::map<std::string, int> stock;
    int soldCount = 0;

public:
    void restock(const std::string& sku, int qty) {
        std::lock_guard<std::mutex> lock(mtx);
        stock[sku] += qty;
    }

    // 回傳是否下單成功。整段為一個不可分割的交易。
    bool placeOrder(const std::string& sku, int qty) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = stock.find(sku);
        if (it == stock.end() || it->second < qty) {
            return false;              // 庫存不足，拒單
        }
        it->second -= qty;             // 與上面的檢查同在鎖內 → 不會超賣
        soldCount += qty;
        return true;
    }

    int query(const std::string& sku) const {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = stock.find(sku);
        return it == stock.end() ? 0 : it->second;
    }

    int sold() const {
        std::lock_guard<std::mutex> lock(mtx);
        return soldCount;
    }
};

int main() {
    std::cout << "=== 錯誤示範：無同步的轉帳（資料競爭 / UB）===\n";
    std::thread t1([&]() {
        for (int i = 0; i < 1000; ++i) {
            transfer(accountA, accountB, 1);
        }
    });

    std::thread t2([&]() {
        for (int i = 0; i < 1000; ++i) {
            transfer(accountB, accountA, 1);
        }
    });

    t1.join();
    t2.join();

    int total = accountA.balance + accountB.balance;
    std::cout << "A: " << accountA.balance << std::endl;
    std::cout << "B: " << accountB.balance << std::endl;
    std::cout << "總額: " << total << " (應為 2000)" << std::endl;
    std::cout << "註：此數值是 UB 的產物，每次執行都可能不同，"
              << (total == 2000 ? "本次剛好守恆並不代表程式正確" : "本次已可見帳目損毀")
              << std::endl;

    std::cout << "\n=== 正確版：整段複合操作上鎖 ===\n";
    {
        SafeBank bank;
        std::thread s1([&]() { for (int i = 0; i < 100000; ++i) bank.transfer(true, 1); });
        std::thread s2([&]() { for (int i = 0; i < 100000; ++i) bank.transfer(false, 1); });
        s1.join();
        s2.join();
        // 注意：錢最後停在 A 還是 B 取決於執行緒排程（每次執行都不同），
        // 但「總額」這個不變量在任何排程下都必定成立 —— 這才是重點。
        std::cout << "總額: " << (bank.a + bank.b) << " (必定為 2000)" << std::endl;
        std::cout << "不變量是否成立: "
                  << (bank.a + bank.b == 2000 ? "是" : "否") << std::endl;
    }

    std::cout << "\n=== LeetCode 1603. Design Parking System ===\n";
    {
        ParkingSystem ps(1, 1, 0);
        std::cout << "addCar(1) = " << std::boolalpha << ps.addCar(1) << "\n";  // true
        std::cout << "addCar(2) = " << ps.addCar(2) << "\n";                    // true
        std::cout << "addCar(3) = " << ps.addCar(3) << "\n";                    // false（本來就 0）
        std::cout << "addCar(1) = " << ps.addCar(1) << "\n";                    // false（已滿）

        // 多執行緒壓力測試：8 條執行緒搶 100 個大型車位，
        // 因為 check-then-act 已受鎖保護，成功次數必定剛好等於 100。
        ParkingSystem lot(100, 0, 0);
        std::vector<std::thread> ths;
        std::mutex sumMtx;
        int granted = 0;
        for (int i = 0; i < 8; ++i) {
            ths.emplace_back([&]() {
                int local = 0;
                for (int k = 0; k < 50; ++k) {
                    if (lot.addCar(1)) ++local;
                }
                std::lock_guard<std::mutex> lock(sumMtx);
                granted += local;
            });
        }
        for (auto& th : ths) th.join();
        std::cout << "8 執行緒共搶到車位數: " << granted << " (車位總數 100，必定相等)\n";
        std::cout << "剩餘大型車位: " << lot.remaining(1) << "\n";
    }

    std::cout << "\n=== 日常實務：秒殺下單不超賣 ===\n";
    {
        Inventory inv;
        inv.restock("SKU-1001", 500);

        std::vector<std::thread> ths;
        std::mutex sumMtx;
        int okCount = 0;
        for (int i = 0; i < 8; ++i) {
            ths.emplace_back([&]() {
                int local = 0;
                for (int k = 0; k < 100; ++k) {
                    if (inv.placeOrder("SKU-1001", 1)) ++local;
                }
                std::lock_guard<std::mutex> lock(sumMtx);
                okCount += local;
            });
        }
        for (auto& th : ths) th.join();

        std::cout << "下單嘗試 800 次，庫存 500\n";
        std::cout << "成功下單: " << okCount << " 筆 (必定 = 500，不會超賣)\n";
        std::cout << "剩餘庫存: " << inv.query("SKU-1001") << "\n";
        std::cout << "已售出數: " << inv.sold() << "\n";
        std::cout << "帳實相符: " << (inv.sold() + inv.query("SKU-1001") == 500 ? "是" : "否") << "\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 4.1：共享資料的問題2.cpp' -o shared_data2
//
// 偵測資料競爭（比肉眼可靠，建議一定要跑一次）:
//   g++ -std=c++17 -fsanitize=thread -g -pthread '課程 4.1：共享資料的問題2.cpp' -o shared_data2_tsan
//   ./shared_data2_tsan   → 會在 transfer() 明確報出 data race

// ⚠️ 第一段「錯誤示範」是 genuine data race → UB，那三行的 A / B / 總額
// 【每次執行都可能不同】，下面只是本機某一次的真實實測，不是保證值。
// 本機（Ubuntu 26.04 / g++ 15.2.0 / 16 核心）多次執行常常剛好印出 2000，
// 原因是迴圈太短、兩條執行緒幾乎沒有重疊執行 —— 這正是 data race 最危險之處。
// 其餘三段（正確版、LeetCode 1603、秒殺下單）都有鎖保護，結果為確定值。

// === 預期輸出 ===
// === 錯誤示範：無同步的轉帳（資料競爭 / UB）===
// A: 1000
// B: 1000
// 總額: 2000 (應為 2000)
// 註：此數值是 UB 的產物，每次執行都可能不同，本次剛好守恆並不代表程式正確
//
// === 正確版：整段複合操作上鎖 ===
// 總額: 2000 (必定為 2000)
// 不變量是否成立: 是
//
// === LeetCode 1603. Design Parking System ===
// addCar(1) = true
// addCar(2) = true
// addCar(3) = false
// addCar(1) = false
// 8 執行緒共搶到車位數: 100 (車位總數 100，必定相等)
// 剩餘大型車位: 0
//
// === 日常實務：秒殺下單不超賣 ===
// 下單嘗試 800 次，庫存 500
// 成功下單: 500 筆 (必定 = 500，不會超賣)
// 剩餘庫存: 0
// 已售出數: 500
// 帳實相符: 是
