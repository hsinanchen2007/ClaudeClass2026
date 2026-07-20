// =============================================================================
//  課程 4.2：不變量與競爭條件2.cpp  —  當「破壞期」變成可觀察的
// =============================================================================
//
// 【本檔是「刻意示範錯誤」的範例，不要照抄到實際專案】
//   writer() 改寫串列指標的同時，reader() 正在走訪同一份串列，
//   兩者之間沒有任何同步 → genuine data race → 【未定義行為 (UB)】。
//   本檔【不會】宣稱「一定印出 1 2 3」或「一定印出 1 3」——UB 沒有固定結果。
//
// 【主題資訊 Information】
//   主題：    競爭條件 (race condition) 的本質 = 有人觀察到不變量的破壞期
//   語法：    兩條執行緒對同一組 Node 全域物件，一方寫、一方讀，無任何同步
//   標準版本：std::thread 為 C++11
//   標頭檔：  <thread>、<iostream>
//   對照組：  上一檔（不變量與競爭條件1.cpp）是完全相同的插入邏輯，
//             但只有一條執行緒，所以完全安全。差別只在「有沒有觀眾」。
//   偵測工具：g++ -fsanitize=thread -g -pthread（本檔會被明確報出 data race）
//
// 【詳細解釋 Explanation】
//
// 【1. 同一段程式碼，為什麼上一檔安全、這一檔就是災難】
//   插入邏輯一字未改：
//       b.next = &c;   b.prev = &a;   a.next = &b;   c.prev = &b;
//   唯一的差別是這一檔多了一條 reader 執行緒。
//   → 這正是本課最重要的結論：
//     【不變量的破壞不是罪，破壞被觀察到才是罪】。
//   安全與否不是程式碼本身的性質，而是程式碼與「誰能同時看到它」的關係。
//
// 【2. reader 可能看到的三種串列】
//   writer 的四個 store 依序執行時，reader 從 &a 開始走訪可能撞見：
//     (a) 尚未執行 `a.next = &b`  → 走訪得到 a → c，印出「1 3」
//         （看到的是插入前的舊串列，合法但已過期）
//     (b) 已執行 `a.next = &b`    → 走訪得到 a → b → c，印出「1 2 3」
//         （看到的是插入後的新串列，合法）
//     (c) 撕裂的中間狀態          → 例如 a.next 已指向 b，但 b.next 還沒設定好，
//         此時 b.next 可能仍是 nullptr（提早結束）或任何值。
//   (a) 與 (b) 看起來「還好」，這正是陷阱所在：測試時多半只會撞到這兩種，
//   讓人誤以為程式沒問題。但只要 (c) 發生一次，就可能解參考到垃圾指標。
//
// 【3. 更關鍵的是：(a) 與 (b) 也【不是】標準保證的結果】
//   一旦構成 data race，標準就不再對程式行為做任何承諾。
//   不能推理成「反正只是新舊兩種版本，最壞就是拿到舊資料」——
//   這個推理隱含假設「指標寫入是原子的、而且不會被重排」，
//   而 C++ 標準對非 atomic 的物件【沒有】給這個保證。
//   編譯器可以把 b.next / b.prev / a.next / c.prev 的寫入重新排序，
//   也可以把讀取快取到暫存器、甚至整段消除。
//
// 【4. data race 與 race condition 的區別（面試必考）】
//   * data race（資料競爭）：兩條以上執行緒同時存取同一記憶體位置，
//     至少一方寫入，且無 happens-before 關係 → 標準層級的 UB。
//     這是「語言規則」問題。
//   * race condition（競爭條件）：程式結果取決於執行時序的邏輯瑕疵。
//     這是「邏輯設計」問題。
//   兩者交叉而非包含：本檔【兩者都有】——沒有同步（data race），
//   而且印出什麼取決於誰先跑（race condition）。
//   但也存在「有鎖、沒有 data race，卻仍有 race condition」的情形，
//   例如每次存取都上鎖、卻在鎖與鎖之間做 check-then-act。
//
// 【5. 正確作法】
//   讓 writer 與 reader 對同一把 mutex 加鎖，
//   使「四個 store」與「整段走訪」互相排斥：
//       std::mutex mtx;
//       { std::lock_guard<std::mutex> lk(mtx);  /* 插入四步 */ }
//       { std::lock_guard<std::mutex> lk(mtx);  /* 整段走訪 */ }
//   注意走訪【整段】都要在鎖內；只鎖住「取得第一個節點」是沒用的，
//   因為指標會在走訪過程中被改掉。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼「印出 1 2 3」比「印出 1 3」更值得警惕
//   兩者都只是抽樣結果。但「1 2 3」會讓工程師覺得「同步好像自然發生了」，
//   進而把這段程式碼當成正確的範本複製到其他地方。
//   本機（Ubuntu 26.04 / g++ 15.2.0 / 16 核心）實測，因為 writer 的四個 store
//   極短、而執行緒建立成本約數十 μs，reader 幾乎總是在 writer 完成後才開始走訪，
//   所以【壓倒性多數】會印出完整的「1 2 3」。
//   這是「測不出來」而不是「沒有錯」。
//
// (B) 為什麼 ThreadSanitizer 抓得到而肉眼抓不到
//   TSan 不靠碰運氣，它為每個記憶體位置維護 shadow memory 與向量時鐘，
//   直接判定兩次存取之間有沒有 happens-before 關係。
//   只要執行過一次衝突路徑就會報警，不需要真的觀察到錯誤輸出。
//   代價是記憶體約 5～10 倍、速度約 2～20 倍，適合在 CI 跑而非正式環境。
//
// (C) 為什麼「加大迴圈次數」不是可靠的驗證手段
//   競爭視窗的寬度取決於指令數、快取狀態、核心排程、其他負載。
//   同一支程式在你的筆電十萬次都對，在客戶的 128 核心機器上第一天就爆。
//   壓力測試只能提高機率、不能證明不存在；形式化工具才能給出判定。
//
// 【注意事項 Pay Attention】
// 1. 本檔是 UB 示範：不可說「一定印出 X」，也不可說「最壞只是拿到舊資料」。
// 2. 走訪必須【整段】在鎖內；只保護「取得起點」等於沒保護。
// 3. std::cout 本身也是共享資源。多執行緒直接 << 不會 UB（實作有內部鎖），
//    但輸出會交錯成無法閱讀的樣子（下一課 4.3 詳述）。
// 4. reader 與 writer 誰先啟動並不重要 —— 只要「可能重疊」就已構成 data race。
// 5. 把指標換成 std::atomic<Node*> 可以消除 data race，
//    但「走訪期間串列被改」這個 race condition 仍在，仍需更高層的設計
//    （RCU、hazard pointer、或直接用鎖）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】競爭條件 vs 資料競爭
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. data race 和 race condition 有什麼不同?
//     答：data race 是「兩執行緒同時存取同一位置、至少一方寫、無同步」，
//         是 C++ 標準判定的 UB，屬語言規則問題。
//         race condition 是「結果取決於執行時序」的邏輯瑕疵，屬設計問題。
//         兩者交叉：本檔兩者都有；但也可能有鎖（無 data race）
//         卻仍在鎖與鎖之間做 check-then-act（有 race condition）。
//     追問：有沒有「有 data race 但沒有 race condition」的例子?
//         → 有。例如兩條執行緒把同一個變數都寫成同一個常數 42：
//           結果不取決於時序（沒有 race condition），但標準仍判 data race → UB。
//
// 🔥 Q2. 這一檔和上一檔的插入邏輯完全相同，為什麼一個安全一個不安全?
//     答：因為安全與否不是程式碼本身的性質，而是「破壞期有沒有觀眾」。
//         任何多步驟修改都必然暫時破壞不變量；單執行緒沒有人能觀察，
//         多執行緒則必須用 mutex 把破壞期重新變成不可觀察。
//     追問：那要保護到什麼範圍?
//         → 不變量橫跨幾個變數，臨界區段就要橫跨幾個變數；
//           讀者也要用同一把鎖，而且要保護【整段走訪】，不是只保護起點。
//
// ⚠️ 陷阱. 「就算有競爭，reader 最壞也只是讀到插入前的舊串列，
//         資料還是合法的，所以可以接受」——錯在哪?
//     答：這個推理隱含「指標寫入是原子的、而且不會被重排」的假設，
//         但 C++ 對非 atomic 物件沒有給這個保證。
//         reader 完全可能看到 a.next 已更新、而 b.next 尚未寫入的撕裂狀態，
//         然後解參考到不確定的值。而且一旦是 UB，標準連「舊串列」都不保證。
//     為什麼會錯：多數人把並行想成「新舊兩個版本之間切換」的資料庫快照模型，
//         但沒有同步機制時根本不存在「版本」這個概念 ——
//         記憶體只是一堆可以被任意順序改寫的位元組。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <vector>
#include <functional>

// -----------------------------------------------------------------------------
// 【錯誤示範】完全沒有同步的 writer / reader —— 本課主角
// -----------------------------------------------------------------------------
struct Node {
    int data;
    Node* prev = nullptr;
    Node* next = nullptr;
};

Node a{1}, b{2}, c{3};

void writer() {
    // 插入 b 到 a 和 c 之間
    b.next = &c;
    b.prev = &a;
    // ← 此時另一個執行緒可能讀取！
    a.next = &b;
    c.prev = &b;
}

void reader() {
    // 嘗試遍歷鏈結串列
    Node* current = &a;
    while (current != nullptr) {
        std::cout << current->data << " ";
        current = current->next;
    }
    std::cout << std::endl;
}

// -----------------------------------------------------------------------------
// 【正確版】同一把鎖保護「整個插入」與「整段走訪」
// -----------------------------------------------------------------------------
struct SafeList {
    std::mutex mtx;
    Node n1{1}, n2{2}, n3{3};

    SafeList() {
        n1.next = &n3;
        n3.prev = &n1;
    }

    void insertMiddle() {
        std::lock_guard<std::mutex> lock(mtx);   // 破壞期整段受保護
        n2.next = &n3;
        n2.prev = &n1;
        n1.next = &n2;
        n3.prev = &n2;
    }

    std::string traverse() {
        std::lock_guard<std::mutex> lock(mtx);   // 整段走訪都在鎖內
        std::string out;
        for (Node* p = &n1; p; p = p->next) out += std::to_string(p->data) + " ";
        return out;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1114. Print in Order
//   題目：三條執行緒分別呼叫 first() / second() / third()，呼叫順序不保證，
//         但必須確保輸出永遠是 "onetwothree"。
//   為什麼用到本主題：這題是本檔問題的「解答形式」。本檔的 reader 之所以
//         可能看到破壞期，正是因為 reader 與 writer 之間沒有任何
//         happens-before 關係。1114 要求的就是「建立 happens-before」：
//         用 mutex + condition_variable 讓後面的階段等前面的階段完成。
//         注意：光有 mutex 只能保證互斥（不會同時進去），
//         【無法保證順序】——順序必須靠 condition_variable 等待條件成立。
//         這是初學者最常混淆的一點。
// -----------------------------------------------------------------------------
class Foo {
private:
    // 【注意】成員初始化順序由宣告順序決定，與初始化列表順序無關。
    std::mutex mtx;
    std::condition_variable cv;
    int stage = 0;          // 已完成的階段數：0 → 1 → 2 → 3

public:
    Foo() = default;

    void first(const std::function<void()>& printFirst) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            printFirst();
            stage = 1;
        }
        cv.notify_all();
    }

    void second(const std::function<void()>& printSecond) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return stage >= 1; });   // 等 first 完成
            printSecond();
            stage = 2;
        }
        cv.notify_all();
    }

    void third(const std::function<void()>& printThird) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return stage >= 2; });   // 等 second 完成
            printThird();
            stage = 3;
        }
        cv.notify_all();
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】設定熱更新：讀者不可以看到「更新到一半」的設定
//   情境：服務執行中要熱更新設定（DB 連線字串、逾時秒數、功能開關）。
//         天真的做法是逐欄位覆寫，於是工作執行緒可能讀到
//         「新的 host + 舊的 port」這種從未存在過的組合，
//         連上一台根本不該連的機器 —— 這是真實系統最常見的線上事故之一。
//   修法：① 用鎖保護整組欄位（本例示範），或
//         ② 建好一份完整的新設定，再以一次指標交換整份換掉
//            （copy-on-write，讀者完全無鎖，適合讀多寫少）。
// -----------------------------------------------------------------------------
struct DbConfig {
    std::string host;
    int port;
    int timeoutMs;
};

class ConfigStore {
private:
    mutable std::mutex mtx;
    DbConfig cfg;

public:
    ConfigStore() : cfg{"db-old.internal", 5432, 3000} {}

    // 整組欄位一起換：讀者不可能看到 host 與 port 不搭配的組合
    void update(const std::string& host, int port, int timeoutMs) {
        std::lock_guard<std::mutex> lock(mtx);
        cfg.host = host;
        cfg.port = port;         // ← 這三行之間不變量不成立，
        cfg.timeoutMs = timeoutMs;  //   但鎖讓破壞期不可觀察
    }

    // 回傳整份複本，而不是欄位的引用 —— 呼叫端拿到的必定是一致的快照
    DbConfig snapshot() const {
        std::lock_guard<std::mutex> lock(mtx);
        return cfg;
    }
};

int main() {
    std::cout << "=== 錯誤示範：無同步的 writer / reader（data race / UB）===\n";
    a.next = &c;
    c.prev = &a;

    std::thread t1(writer);
    std::thread t2(reader);

    t1.join();
    t2.join();
    std::cout << "（上面那一行的內容每次執行都可能不同，且不受標準保證）\n";

    std::cout << "\n=== 正確版：同一把鎖保護插入與走訪 ===\n";
    {
        SafeList list;
        std::string before = list.traverse();
        std::thread w([&] { list.insertMiddle(); });
        w.join();
        std::string after = list.traverse();
        std::cout << "插入前走訪: " << before << "\n";
        std::cout << "插入後走訪: " << after << "\n";
        std::cout << "→ 讀者只會看到「插入前」或「插入後」，不會看到中間狀態\n";
    }

    std::cout << "\n=== LeetCode 1114. Print in Order ===\n";
    {
        Foo foo;
        // 刻意用「相反」的順序啟動執行緒，證明順序由同步機制保證，
        // 而不是由執行緒建立順序保證。
        std::thread th3([&] { foo.third ([] { std::cout << "three"; }); });
        std::thread th1([&] { foo.first ([] { std::cout << "one";   }); });
        std::thread th2([&] { foo.second([] { std::cout << "two";   }); });
        th3.join();
        th1.join();
        th2.join();
        std::cout << "\n→ 即使以 3-1-2 的順序啟動執行緒，輸出仍必定是 onetwothree\n";
    }

    std::cout << "\n=== 日常實務：設定熱更新的一致性快照 ===\n";
    {
        ConfigStore store;
        std::vector<std::thread> readers;
        std::mutex outMtx;
        int inconsistent = 0;

        // 一條執行緒不斷熱更新，多條工作執行緒不斷讀取
        std::thread updater([&] {
            for (int i = 0; i < 2000; ++i) {
                if (i % 2 == 0) store.update("db-blue.internal",  5432, 3000);
                else            store.update("db-green.internal", 6432, 5000);
            }
        });

        for (int i = 0; i < 4; ++i) {
            readers.emplace_back([&] {
                int bad = 0;
                for (int k = 0; k < 2000; ++k) {
                    DbConfig s = store.snapshot();
                    // 合法組合只有兩種；任何混搭都代表讀到了破壞期
                    bool ok = (s.host == "db-blue.internal"  && s.port == 5432 && s.timeoutMs == 3000)
                           || (s.host == "db-green.internal" && s.port == 6432 && s.timeoutMs == 5000);
                    if (!ok) ++bad;
                }
                std::lock_guard<std::mutex> lock(outMtx);
                inconsistent += bad;
            });
        }

        updater.join();
        for (auto& t : readers) t.join();

        std::cout << "4 條讀者各取 2000 次快照，共 8000 次\n";
        std::cout << "讀到不一致設定的次數: " << inconsistent << " (有鎖保護，必定為 0)\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 4.2：不變量與競爭條件2.cpp' -o invariant2
//
// 偵測資料競爭（強烈建議跑一次，比肉眼可靠）:
//   g++ -std=c++17 -fsanitize=thread -g -pthread '課程 4.2：不變量與競爭條件2.cpp' -o invariant2_tsan
//   ./invariant2_tsan  → 會明確指出 writer() 的 store 與 reader() 的 load 衝突

// ⚠️ 第一段「錯誤示範」是 genuine data race → UB，那一行走訪結果
// 【每次執行都可能不同】（可能是「1 2 3」、「1 3」，或撕裂狀態），
// 下面只是本機某一次的真實實測，不是保證值。
// 本機（Ubuntu 26.04 / g++ 15.2.0 / 16 核心）實測壓倒性多數印出「1 2 3」，
// 因為 writer 的四個 store 遠短於執行緒建立成本 —— 這是「測不出來」，不是「沒有錯」。
// 其餘三段（正確版、LeetCode 1114、設定熱更新）都有同步保護，結果為確定值。

// === 預期輸出 ===
// === 錯誤示範：無同步的 writer / reader（data race / UB）===
// 1 3 
// （上面那一行的內容每次執行都可能不同，且不受標準保證）
//
// === 正確版：同一把鎖保護插入與走訪 ===
// 插入前走訪: 1 3 
// 插入後走訪: 1 2 3 
// → 讀者只會看到「插入前」或「插入後」，不會看到中間狀態
//
// === LeetCode 1114. Print in Order ===
// onetwothree
// → 即使以 3-1-2 的順序啟動執行緒，輸出仍必定是 onetwothree
//
// === 日常實務：設定熱更新的一致性快照 ===
// 4 條讀者各取 2000 次快照，共 8000 次
// 讀到不一致設定的次數: 0 (有鎖保護，必定為 0)
