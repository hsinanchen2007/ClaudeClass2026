// =============================================================================
//  第 17 課：解構函數 6  —  忘記 delete：解構函數永遠不會被呼叫
// =============================================================================
//
// 【主題資訊 Information】
//   主題：動態配置物件若沒有 delete，解構函數不會執行 → 資源洩漏
//   標準版本：C++98 起即有；std::unique_ptr 為 C++11、std::make_unique 為 C++14
//   複雜度：O(1)
//   標頭檔：<memory>（智慧指標）
//   本檔性質：**含刻意保留的記憶體洩漏示範**（資源 #2），
//             用來讓你親眼看到「少了一行解構訊息」。詳見下方說明。
//
// 【詳細解釋 Explanation】
//
// 【1. 這個 bug 的形狀：沉默】
//   記憶體洩漏最麻煩的地方是**它不會報錯**。程式照跑、結果照對、
//   測試照過，只有長時間執行的服務會慢慢吃掉記憶體，
//   幾天後才因為記憶體不足而倒掉——而那時已經很難回推是哪一行造成的。
//   本檔把這個沉默視覺化：資源 #2 的建構訊息有印出來，
//   但**解構訊息永遠不會出現**。少掉的那一行就是洩漏的證據。
//
// 【2. 為什麼「程式結束時作業系統會回收」不能當成藉口】
//   這句話本身沒錯：行程結束時，作業系統會回收它的整個記憶體空間。
//   但它有三個嚴重問題：
//     (a) **長時間執行的程式根本不會結束**。伺服器、守護行程、遊戲主迴圈
//         一跑就是好幾個月，洩漏會持續累積。
//     (b) **記憶體不是唯一的資源**。檔案描述元、socket、資料庫連線、
//         鎖、GPU 緩衝區都有限，而且有些（如資料庫連線）由遠端持有，
//         你的行程結束不代表對方會馬上釋放。
//     (c) **解構函數的副作用不見了**。若解構函數負責寫入尚未存檔的資料、
//         送出結束通知、更新統計，這些邏輯都會被跳過——洩漏的不只是記憶體，
//         而是「該做的事沒做」。
//
// 【3. 三個層次的解法】
//   ● 層次一（不推薦）：小心一點，記得每個 new 都配一個 delete
//     問題：每一條 return、每一個例外路徑都要記得，人一定會漏。
//   ● 層次二（好）：讓物件放在堆疊上，用作用域管理（本檔的資源 #3）
//     只要不是非得動態配置，這永遠是最簡單的答案。
//   ● 層次三（最佳）：非得動態配置時，用**智慧指標**
//         auto p = std::make_unique<Resource>(4);   // C++14
//     unique_ptr 的解構函數會替你 delete，等於把動態儲存期
//     重新納入 RAII 的保護。本檔的資源 #4 示範這個做法。
//
// 【4. unique_ptr 與 shared_ptr 怎麼選】
//   ● std::unique_ptr：**獨佔**所有權，不可複製、只能移動。
//     零額外開銷（大小通常等同裸指標），應該是你的**預設選擇**。
//   ● std::shared_ptr：**共享**所有權，內部有參考計數，
//     最後一個持有者消失時才釋放。有額外的空間與同步成本，
//     而且循環參考會造成洩漏（需要 std::weak_ptr 打破）。
//   經驗法則：先用 unique_ptr，只有在確實需要多方共享生命週期時才換 shared_ptr。
//
// 【5. 怎麼在實務上抓到洩漏（本機實測有個值得記住的意外）】
//   不要靠肉眼，要用工具。但工具之間有差異，本檔正好是個好例子。
//   本機（g++ 15.2 / glibc）對本檔實測：
//     ● **Valgrind 抓到了**：
//           valgrind --leak-check=full --show-leak-kinds=all ./demo6
//       回報 “4 bytes in 1 blocks are definitely lost”，正是資源 #2。
//     ● **AddressSanitizer（LeakSanitizer）這次沒有回報**：
//           g++ -fsanitize=address -g ... && ./demo6
//       程式結束時沒有任何洩漏報告。
//   為什麼會這樣？因為 LeakSanitizer 判斷的是**可達性**：它在程式結束時
//   掃描堆疊與暫存器，若還找得到指向該區塊的指標，就視為「仍可達」而不報告。
//   本檔的 r2 是 main 的區域變數，程式結束時它的值可能仍殘留在堆疊上，
//   於是那塊記憶體看起來「還有人指著」，就被略過了。
//   而 Valgrind 用的是更嚴格的判準，因此判定為 definitely lost。
//
//   要記住的結論有兩個：
//     (1) **工具沒報，不代表沒洩漏**。這與第 16 課 7.cpp 的教訓一致：
//         偵測工具是輔助，不是正確性的證明。
//     (2) 上述「LSan 沒報」是本機這次建置的觀察，會隨編譯器版本、
//         最佳化等級、堆疊佈局而變，**不是保證的行為**。
//         換個環境它可能就報出來了。
//
// 【概念補充 Concept Deep Dive】
//
//   ● 洩漏的是什麼
//     new Resource(2) 做了兩件事：配置記憶體、在該記憶體上建構物件。
//     沒有 delete 就是兩件事都沒還：記憶體沒歸還，解構函數也沒執行。
//
//   ● 指標被覆寫也會造成洩漏
//     Resource* p = new Resource(1);
//     p = new Resource(2);      // 第一塊再也找不到 → 洩漏
//     這種「還有指標變數，但已經指到別處」的情況比忘記 delete 更難發現。
//
//   ● delete 之後指標並不會變成 nullptr
//     delete p; 之後 p 仍然保有原本的位址，變成**懸空指標**（dangling）。
//     再次解參考或再次 delete 都是未定義行為。
//     舊習慣是 delete p; p = nullptr;，但更好的答案是根本不要用裸的
//     new/delete——用智慧指標，它會把這些細節處理掉。
//
//   ● 為什麼 unique_ptr 不可複製
//     因為「獨佔所有權」在語意上就不允許兩份。它只支援移動
//     （std::move），移動之後來源會變成 nullptr，
//     所有權明確地轉交出去，不可能有兩個人都以為自己該負責釋放。
//
// 【注意事項 Pay Attention】
//   1. 本檔的資源 #2 是**刻意保留的洩漏**，用來示範症狀，請勿模仿。
//   2. 「反正程式結束會回收」不成立：長跑程式、非記憶體資源、
//      解構函數的副作用，三者都會出問題。
//   3. delete 之後指標不會自動變 nullptr，仍是懸空指標。
//   4. 需要動態配置時請用 std::unique_ptr（預設）或 std::shared_ptr（需共享時）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】記憶體洩漏與智慧指標
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 忘記 delete 會發生什麼事？「反正程式結束作業系統會回收」對嗎？
//     答：那塊記憶體不會歸還，而且該物件的**解構函數永遠不會被呼叫**。
//         「作業系統會回收」對短命的程式勉強成立，但三個情況會出事：
//         長時間執行的服務根本不會結束、記憶體以外的資源（檔案描述元、
//         連線、鎖）有限且可能由遠端持有、以及解構函數裡該做的事
//         （存檔、送通知、更新統計）全部被跳過。
//     追問：怎麼在開發階段抓到洩漏？
//         → 用 Valgrind（--leak-check=full）或 AddressSanitizer
//           （-fsanitize=address）。但要知道兩者判準不同：LeakSanitizer
//           以「可達性」判斷，若指標值仍殘留在堆疊上就可能不報告；
//           本檔實測 Valgrind 回報 definitely lost，LSan 這次卻沒報。
//           工具沒報不等於沒洩漏。
//
// 🔥 Q2. unique_ptr 和 shared_ptr 差在哪？預設該用哪個？
//     答：unique_ptr 是獨佔所有權，不可複製只能移動，通常沒有額外開銷；
//         shared_ptr 是共享所有權，內部維護參考計數，有空間與同步成本，
//         而且循環參考會造成洩漏（需要 weak_ptr 打破）。
//         預設應該用 unique_ptr，只有確實需要多方共享生命週期才用 shared_ptr。
//     追問：為什麼 unique_ptr 不能複製？
//         → 因為「獨佔」的語意不允許存在兩份所有權。它只支援 std::move，
//           移動後來源變成 nullptr，責任歸屬永遠明確。
//
// ⚠️ 陷阱. 我寫了 delete p; 之後又檢查 if (p != nullptr) 才使用，這樣安全嗎？
//     答：不安全。delete 之後 p **仍然保有原本的位址**，不會自動變成 nullptr，
//         所以那個檢查一定會通過，然後你就解參考了一塊已經歸還的記憶體——
//         未定義行為。重複 delete 同一個指標同樣是未定義行為。
//     為什麼會錯：以為 delete 會「清理指標變數本身」。實際上 delete 處理的是
//         指標**指向的物件**，指標變數只是個存位址的普通變數，
//         delete 不會去動它。這也是為什麼現代 C++ 建議完全不要碰裸的
//         new/delete —— 智慧指標會把所有權與指標狀態一起管好。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>
using namespace std;

class Resource {
private:
    int id;

public:
    explicit Resource(int i) : id(i) {
        cout << "  [建構] 資源 #" << id << endl;
    }

    ~Resource() {
        cout << "  [解構] 資源 #" << id << endl;
    }

    int getId() const { return id; }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】連線池：以 unique_ptr 持有物件，轉手時用 move 明確交接
//   情境：服務啟動時預先建立數條連線放進池子，取用時把所有權「借出」，
//         用完再還回來。連線是有限資源，任何一條漏掉都會慢慢耗盡池子。
//   重點：
//     ● 用 unique_ptr 持有 → 池子解構時所有連線自動關閉，不必手寫迴圈 delete
//     ● 借出／歸還用 std::move 明確轉移所有權 → 不可能有兩個人以為自己該釋放
//     ● 借出期間池子裡那一格是 nullptr，狀態一目了然
// -----------------------------------------------------------------------------
class Connection {
private:
    int id_;
public:
    explicit Connection(int id) : id_(id) {
        cout << "    [開啟] 連線 #" << id_ << endl;
    }
    ~Connection() {
        cout << "    [關閉] 連線 #" << id_ << endl;
    }
    void send(const string& msg) const {
        cout << "    連線 #" << id_ << " 送出: " << msg << endl;
    }
};

class ConnectionPool {
private:
    vector<unique_ptr<Connection>> pool_;

public:
    explicit ConnectionPool(int n) {
        for (int i = 1; i <= n; ++i) {
            pool_.push_back(make_unique<Connection>(i));
        }
    }
    // 不需要寫解構函數：pool_ 解構時每個 unique_ptr 都會自動關閉連線
    // 這就是 Rule of Zero

    // 借出一條連線（所有權轉移給呼叫端）
    unique_ptr<Connection> acquire() {
        for (auto& slot : pool_) {
            if (slot) return std::move(slot);   // 轉移後 slot 變成 nullptr
        }
        return nullptr;                          // 池子空了
    }

    // 歸還（所有權轉移回池子）
    void release(unique_ptr<Connection> conn) {
        for (auto& slot : pool_) {
            if (!slot) { slot = std::move(conn); return; }
        }
    }

    void status() const {
        cout << "    池況: ";
        for (const auto& slot : pool_) cout << (slot ? "[可用]" : "[借出]");
        cout << endl;
    }
};

int main() {
    cout << "=== 正確使用 delete ===" << endl;
    Resource* r1 = new Resource(1);
    cout << "  使用資源 #" << r1->getId() << endl;
    delete r1;    // 解構函數被呼叫，資源被釋放

    cout << "\n=== 忘記 delete（刻意保留的洩漏示範）===" << endl;
    Resource* r2 = new Resource(2);
    cout << "  使用資源 #" << r2->getId() << endl;
    // 這裡故意沒有 delete r2;
    //   → 解構函數永遠不會被呼叫
    //   → 這塊記憶體永遠不會被釋放
    //   → 注意輸出中「找不到」資源 #2 的解構訊息，那就是洩漏的證據
    cout << "  ↑ 沒有 delete，所以下面不會出現「[解構] 資源 #2」" << endl;

    cout << "\n=== 局部物件（自動管理，推薦）===" << endl;
    {
        Resource r3(3);
        cout << "  使用資源 #" << r3.getId() << endl;
        // 不需要 delete，離開作用域自動解構
    }

    cout << "\n=== 智慧指標（需要動態配置時的正解）===" << endl;
    {
        auto r4 = make_unique<Resource>(4);
        cout << "  使用資源 #" << r4->getId() << endl;
        // 不需要 delete，unique_ptr 的解構函數會處理
    }

    cout << "\n=== 日常實務：連線池（unique_ptr + move）===" << endl;
    {
        ConnectionPool pool(3);
        pool.status();

        auto conn = pool.acquire();       // 借出一條
        pool.status();
        if (conn) conn->send("SELECT 1");

        pool.release(std::move(conn));    // 明確歸還所有權
        pool.status();

        cout << "    --- 離開作用域，池子解構 ---" << endl;
    }

    cout << "\n=== main() 結束 ===" << endl;
    return 0;
    // r1 已在上面被解構
    // r2 指向的記憶體洩漏了，解構函數從未執行
    // r3、r4 與連線池的所有連線都已自動釋放
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 17 課：解構函數（Destructor）6.cpp" -o demo6
//
// 用 Valgrind 實際抓出洩漏（本機實測回報 "4 bytes definitely lost"）:
//   g++ -std=c++17 -g "第 17 課：解構函數（Destructor）6.cpp" -o demo6
//   valgrind --leak-check=full --show-leak-kinds=all ./demo6
//
// 用 AddressSanitizer（注意：本機實測這次**沒有**回報，理由見上方【5】）:
//   g++ -std=c++17 -Wall -Wextra -fsanitize=address -g "第 17 課：解構函數（Destructor）6.cpp" -o demo6
//   ./demo6
//
// ※ 重要說明（放在預期輸出標記之前）：
//   本檔含**刻意保留**的記憶體洩漏（資源 #2），用來示範「沒有 delete 就
//   沒有解構」。請比對下方輸出：資源 #1、#3、#4 都有對應的 [解構] 訊息，
//   唯獨資源 #2 沒有——那一行的缺席就是洩漏的證據。
//   下方預期輸出是以一般編譯（不帶 sanitizer）取得，內容完全確定。
//   若改用 Valgrind 或 sanitizer 執行，會另外印出工具自己的報告
//   （含隨機化的位址與行程編號），那些不在預期輸出範圍內。

// === 預期輸出 ===
// === 正確使用 delete ===
//   [建構] 資源 #1
//   使用資源 #1
//   [解構] 資源 #1
//
// === 忘記 delete（刻意保留的洩漏示範）===
//   [建構] 資源 #2
//   使用資源 #2
//   ↑ 沒有 delete，所以下面不會出現「[解構] 資源 #2」
//
// === 局部物件（自動管理，推薦）===
//   [建構] 資源 #3
//   使用資源 #3
//   [解構] 資源 #3
//
// === 智慧指標（需要動態配置時的正解）===
//   [建構] 資源 #4
//   使用資源 #4
//   [解構] 資源 #4
//
// === 日常實務：連線池（unique_ptr + move）===
//     [開啟] 連線 #1
//     [開啟] 連線 #2
//     [開啟] 連線 #3
//     池況: [可用][可用][可用]
//     池況: [借出][可用][可用]
//     連線 #1 送出: SELECT 1
//     池況: [可用][可用][可用]
//     --- 離開作用域，池子解構 ---
//     [關閉] 連線 #1
//     [關閉] 連線 #2
//     [關閉] 連線 #3
//
// === main() 結束 ===
