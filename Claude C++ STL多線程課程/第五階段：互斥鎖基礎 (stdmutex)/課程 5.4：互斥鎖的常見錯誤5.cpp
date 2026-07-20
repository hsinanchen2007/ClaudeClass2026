// =============================================================================
//  課程 5.4：互斥鎖的常見錯誤5.cpp  —  重複鎖定：std::mutex 為何不可重入
// =============================================================================
//
// 【主題資訊 Information】
//   本檔示範：outerFunction() 持有 mtx → 呼叫 innerFunction()
//             → innerFunction() 又對【同一把】mtx 呼叫 lock()
//             → 同一執行緒重複鎖定 → 【未定義行為】
//   標頭檔：<mutex>
//   標準依據：std::mutex 的 lock() 要求「呼叫執行緒尚未持有該 mutex」，
//             違反此前提的行為未定義（std::mutex 不可重入 / non-recursive）。
//   相關檔案：常見錯誤6（recursive_mutex 解法）、
//             常見錯誤7（重構解法，較推薦）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼標準要讓 std::mutex 不可重入】
//   直覺上「可重入」比較方便，為什麼標準的預設是不可重入？三個理由：
//     (a) 【效能】：可重入需要在每次 lock 時多做一件事——
//         讀取目前的執行緒 id 並與持有者比對，還要維護遞迴計數。
//         那是每一次上鎖都要付的成本，即使 99.99% 的程式碼根本不重入。
//         C++ 的原則是「不為你沒用到的功能付費」。
//     (b) 【安全】：重入其實會【掩蓋設計問題】。
//         outerFunction 在中途呼叫 innerFunction 時，被保護的資料
//         很可能正處於「不變量暫時被破壞」的中間狀態
//         （例如兩個必須同步更新的欄位只改了一個）。
//         若鎖可重入，innerFunction 會順利拿到鎖，然後在一個
//         【不完整的狀態】上工作——而且完全沒有徵兆。
//         不可重入至少會讓問題當場浮現。
//     (c) 【語意清晰】：不可重入時，看到 lock() 就知道
//         「這裡是臨界區段的起點」。可重入時你無法判斷自己在第幾層，
//         臨界區段的實際範圍變得無法推理。
//
// 【2. 為什麼是「UB」而不是「保證死結」——這是本檔的核心觀念】
//   標準只寫「未定義行為」，因為不同實作可以有不同反應：
//     * glibc 的 PTHREAD_MUTEX_NORMAL（std::mutex 走這條）→ 實際上卡死
//     * PTHREAD_MUTEX_ERRORCHECK → 回傳 EDEADLK 錯誤碼
//     * PTHREAD_MUTEX_RECURSIVE  → 遞迴計數 +1，正常繼續
//   標準不想把任何一種寫死，所以統一規定為 UB。
//   ⚠️ 因此，正確的說法是：
//       ✗ 「同一執行緒重複 lock 會死結」          ← 把觀察當成保證
//       ✓ 「同一執行緒重複 lock 是未定義行為；
//           在本平台實測的結果是永久停住」        ← 精確
//   本機實測（g++ 15.2 / glibc、Ubuntu 26.04）：印出 "Outer function" 後
//   永久停住（timeout 觀察到 exit=124）。這是【本平台的實作結果】。
//
// 【3. 這個錯誤在真實程式碼裡怎麼長出來的】
//   幾乎沒有人會刻意寫出「連續 lock 兩次」。它幾乎總是【間接】發生：
//       公開函式 A（上鎖）→ 呼叫 → 公開函式 B（也上鎖）  💀
//   而且往往是【重構之後才出現】的：
//     * 原本 A 直接內嵌了那段邏輯，後來有人把它抽成公開函式 B；
//     * 原本 B 不需要鎖，後來 B 被別處直接呼叫，於是有人幫 B 加了鎖；
//     * A 呼叫的其實是 C，而 C 內部呼叫了 B——呼叫鏈長到沒人發現。
//   ⚠️ 最後一種最可怕：從 A 的程式碼完全看不出它會間接重入，
//      必須追蹤整條呼叫鏈才能發現。
//
// 【4. 兩種解法，以及該選哪一個】
//   ┌────────────────────────┬──────────────────────┬─────────────────────┐
//   │ 解法                   │ 做法                 │ 適用時機            │
//   ├────────────────────────┼──────────────────────┼─────────────────────┤
//   │ 重構（推薦）           │ 抽出不上鎖的私有實作 │ 重入點只有少數幾處  │
//   │ （見常見錯誤7）        │ 由公開函式統一上鎖   │ 臨界區段要能推理    │
//   ├────────────────────────┼──────────────────────┼─────────────────────┤
//   │ std::recursive_mutex   │ 換一種鎖             │ 重入是瀰漫全類別的  │
//   │ （見常見錯誤6）        │                      │ 模式（如遞迴走訪）  │
//   └────────────────────────┴──────────────────────┴─────────────────────┘
//   預設選【重構】。recursive_mutex 讓程式碼「跑起來」，
//   但它同時也把上面第 1(b) 點的隱患一起留下了。
//
// 【概念補充 Concept Deep Dive】
//   * 怎麼在測試階段就抓到這個錯誤？
//     ThreadSanitizer 對此有專門的偵測：
//         g++ -std=c++17 -pthread -g -fsanitize=thread ...
//     它會直接報 "double lock of a mutex" 並列出【兩次上鎖的堆疊】——
//     對於前面提到的「長呼叫鏈間接重入」，這是最有效的工具，
//     因為它直接告訴你兩個上鎖點分別在哪。
//   * Valgrind 的 helgrind 工具也能偵測，但速度較慢。
//   * 為什麼 glibc 的預設 mutex 不做這個檢查？
//     檢查「持有者是不是自己」需要額外讀取並比對執行緒 id，
//     那是每次 lock 的固定成本。標準選擇讓正確的程式碼跑最快，
//     把錯誤交給 sanitizer 在測試階段處理。
//   * ⚠️ 特別注意：這個 UB 與「常見錯誤1」的永久阻塞【不同】。
//     檔 1 是別的執行緒來取鎖 → 標準保證阻塞（定義明確）；
//     本檔是同一執行緒 → 標準什麼都不保證。
//     症狀可能一樣，性質完全不同。
//
// 【注意事項 Pay Attention】
//   1. std::mutex 【不可重入】：同一執行緒重複 lock() 是 UB，
//      不是「保證死結」。
//   2. 本機實測會永久停住，但那是【實作結果】，不是標準承諾。
//   3. 這個錯誤幾乎總是間接發生（公開函式互相呼叫），
//      而且常在重構後才出現。
//   4. 用 -fsanitize=thread 可在測試階段直接抓出，並顯示兩次上鎖的堆疊。
//   5. 優先用重構解決（見常見錯誤7）；recursive_mutex 是次選。
//   6. 對已持有的 std::mutex 呼叫 try_lock() 同樣是 UB。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::mutex 的不可重入性
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 std::mutex 設計成不可重入？可重入不是比較方便嗎？
//     答：三個理由。效能上，可重入需要每次 lock 都比對執行緒 id
//         並維護遞迴計數，那是所有使用者都要付的成本，
//         但絕大多數程式碼根本不重入。
//         安全上，可重入會【掩蓋設計問題】——外層函式在中途呼叫內層時，
//         被保護的資料常處於「不變量暫時被破壞」的狀態，
//         內層卻能順利取得鎖並在不完整的狀態上工作，毫無徵兆。
//         語意上，不可重入時看到 lock() 就知道臨界區段從這裡開始；
//         可重入時你無法判斷自己在第幾層。
//     追問：那真的需要重入時怎麼辦？→ 優先重構：把邏輯抽成
//           「假設鎖已持有」的私有函式，由公開函式統一上鎖。
//           重入是瀰漫全類別的模式時，才考慮 std::recursive_mutex。
//
// 🔥 Q2. 同一執行緒對 std::mutex 連續 lock 兩次，會發生什麼？
//     答：標準規定為【未定義行為】，沒有承諾任何特定結果。
//         實際行為取決於實作：glibc 的預設 mutex 會卡死；
//         PTHREAD_MUTEX_ERRORCHECK 會回傳 EDEADLK；
//         PTHREAD_MUTEX_RECURSIVE 則正常遞增計數。
//         正因為各實作可以合理地有不同反應，標準才規定為 UB。
//     追問：怎麼在上線前抓到？→ 用 -fsanitize=thread 編譯，
//           TSan 會報 "double lock of a mutex" 並列出兩次上鎖的堆疊，
//           對於長呼叫鏈導致的間接重入特別有用。
//
// ⚠️ 陷阱. 「這段程式碼會死結（deadlock）。」——用來描述本檔的情況，對嗎？
//     答：不精確，有兩處問題。
//         第一，死結的定義是【兩個以上】的執行緒【互相】等待對方的資源，
//         形成循環等待。本檔只有一條執行緒、一把鎖，沒有任何循環。
//         第二，更關鍵的是：這是【未定義行為】，
//         標準根本沒有保證它會卡住。說「會死結」等於把
//         「本機的觀察結果」升格成「語言的承諾」。
//     為什麼會錯：看到程式停住就叫它死結，是很自然的直覺。
//         但在 C++ 裡，「這段程式碼會發生 X」與「這段程式碼是 UB，
//         而本平台的表現是 X」是兩個完全不同層次的陳述——
//         前者可以依賴，後者不能。能分清楚這兩者，
//         是判斷一個人是否真正理解 UB 的關鍵。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔【略過】，理由如下：
//   本檔示範的是「同一執行緒重複鎖定」這個錯誤模式與它的 UB 性質。
//   LeetCode 的並行題（1114 / 1115 / 1116 / 1117 / 1195）中，
//   每條執行緒只呼叫一次自己負責的函式，不存在巢狀呼叫或重入情境。
//   硬掛一題完全無法示範本檔重點，故從缺。
//   （重構解法在同課「常見錯誤7.cpp」以 LeetCode 707 完整示範。）
//
// -----------------------------------------------------------------------------
// 【本檔是「刻意示範錯誤」的範例，開啟示範後程式會停住，不會自己結束】
//
// ⚠️ 精確地說，這【不是「死結」，而是未定義行為】：
//    outerFunction() 先鎖住 mtx，接著呼叫 innerFunction()，
//    innerFunction() 又在【同一個執行緒】對 mtx 呼叫 lock()。
//    std::mutex 不可重入（non-recursive），對已被自己持有的 mutex
//    再次 lock() 依標準即為未定義行為 —— 標準並未保證「一定卡住」。
//
//    實測（g++ 15.2 / glibc、Ubuntu 26.04）：印出 "Outer function" 之後
//    永久停住，不會自行終止（timeout 觀察到 exit=124）。
//    這是本平台的實作結果，不是標準保證的結果。
//
// 為了不讓整份課程的批次執行卡死，這個示範預設【關閉】。
// 要親眼觀察，請自行加上 -DDEMONSTRATE_UB：
//    g++ -std=c++17 -pthread -DDEMONSTRATE_UB -o double_lock '課程 5.4：互斥鎖的常見錯誤5.cpp'
//    timeout 5 ./double_lock ; echo "exit=$?"   # 本平台預期 exit=124
//
// ✅ 正確作法（擇一）：
//    1. 重新設計：把需要鎖的部分抽成「呼叫前已持有鎖」的私有函式，
//       由外層統一上鎖（最推薦，鎖的範圍最清楚）；
//    2. 真的需要同一執行緒重入時，改用 std::recursive_mutex。
// -----------------------------------------------------------------------------

#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

std::mutex mtx;

void outerFunction();
void innerFunction();

void innerFunction() {
    mtx.lock();  // 💀 mtx 已被【同一執行緒】的 outerFunction 鎖定 → 未定義行為
    std::cout << "Inner function" << std::endl;
    mtx.unlock();
}

void outerFunction() {
    mtx.lock();
    std::cout << "Outer function" << std::endl;

    innerFunction();  // 💀 呼叫另一個也需要鎖的函式

    mtx.unlock();
}

// -----------------------------------------------------------------------------
// 【解法 1：重構】把邏輯抽成不上鎖的私有實作（推薦，見常見錯誤7）
// -----------------------------------------------------------------------------
std::mutex fixedMtx;

// 前提：呼叫者【必須】已持有 fixedMtx
void innerImpl() {
    std::cout << "Inner function（不上鎖版本）" << std::endl;
}

// 公開介面：負責上鎖
void innerFixed() {
    std::lock_guard<std::mutex> lock(fixedMtx);
    innerImpl();
}

void outerFixed() {
    std::lock_guard<std::mutex> lock(fixedMtx);
    std::cout << "Outer function（重構後）" << std::endl;
    innerImpl();          // ✓ 呼叫不上鎖的版本，不會重入
}

// -----------------------------------------------------------------------------
// 【解法 2：recursive_mutex】換一種鎖（次選，見常見錯誤6）
// -----------------------------------------------------------------------------
std::recursive_mutex rmtx;

void innerRecursive() {
    std::lock_guard<std::recursive_mutex> lock(rmtx);   // ✓ 可重入
    std::cout << "Inner function（recursive_mutex）" << std::endl;
}

void outerRecursive() {
    std::lock_guard<std::recursive_mutex> lock(rmtx);
    std::cout << "Outer function（recursive_mutex）" << std::endl;
    innerRecursive();     // ✓ 同一執行緒可再次鎖定
}

// -----------------------------------------------------------------------------
// 【日常實務範例】重構之後才「長出來」的間接重入
//   情境：一個購物車服務。原本 addItem() 內嵌了「重算總金額」的邏輯。
//         後來有人把重算抽成公開的 recalculate()（因為結帳頁也要用），
//         並且——很合理地——幫它加了鎖。
//         於是 addItem() 呼叫 recalculate() 就變成了間接重入。
//   ⚠️ 這個 bug 的可怕之處：兩個函式【各自看都完全正確】，
//      只有把它們放在一起看才會發現問題。
//      而且若呼叫鏈更長（addItem → applyDiscount → recalculate），
//      從 addItem 的程式碼完全看不出它會重入。
//   本例示範正確的重構結果：所有邏輯在不上鎖的 Impl 裡，
//   公開函式只負責上鎖，彼此不互相呼叫。
// -----------------------------------------------------------------------------
class ShoppingCart {
private:
    mutable std::mutex mtx_;
    std::vector<long> prices_;
    long total_ = 0;

    // 前提：已持有 mtx_。所有真正的邏輯都放在這一層。
    void recalculateImpl() {
        total_ = 0;
        for (long p : prices_) total_ += p;
    }

    // 前提：已持有 mtx_
    void addItemImpl(long price) {
        prices_.push_back(price);
        recalculateImpl();        // ✓ 呼叫 Impl，不是公開的 recalculate()
    }

public:
    // ── 公開介面：只負責上鎖 + 轉呼叫，彼此【不可】互相呼叫 ──
    void addItem(long price) {
        std::lock_guard<std::mutex> lock(mtx_);
        addItemImpl(price);
    }

    void recalculate() {
        std::lock_guard<std::mutex> lock(mtx_);
        recalculateImpl();
    }

    // 複合操作：重構的額外好處——可以做成一個原子操作
    void addBatch(const std::vector<long>& items) {
        std::lock_guard<std::mutex> lock(mtx_);   // 整批只上一次鎖
        for (long p : items) {
            prices_.push_back(p);
        }
        recalculateImpl();                        // 最後統一重算一次
    }

    long total() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return total_;
    }

    std::size_t itemCount() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return prices_.size();
    }
};

int main() {
#ifdef DEMONSTRATE_UB
    outerFunction();  // 💀 未定義行為；本平台實測會停在 innerFunction 的 lock()
    std::cout << "（本平台上這一行不會被執行）" << std::endl;
#else
    std::cout << "已略過會卡住的重複鎖定示範（預設關閉）。" << std::endl;
    std::cout << "要重現請加上 -DDEMONSTRATE_UB 重新編譯。" << std::endl;
#endif

    std::cout << "\n=== 解法 1: 重構成「不上鎖的私有實作」（推薦）===" << std::endl;
    outerFixed();
    innerFixed();     // 單獨呼叫公開介面也正常
    std::cout << "重構後鎖仍可正常取得: "
              << (fixedMtx.try_lock() ? "是" : "否") << std::endl;
    fixedMtx.unlock();

    std::cout << "\n=== 解法 2: 改用 std::recursive_mutex（次選）===" << std::endl;
    outerRecursive();
    std::cout << "recursive_mutex 允許同一執行緒重入，但會掩蓋設計問題"
              << std::endl;

    std::cout << "\n=== 為什麼說「UB」而不是「死結」 ===" << std::endl;
    std::cout << "死結的定義是【兩個以上】執行緒【互相】等待，形成循環。" << std::endl;
    std::cout << "本檔只有一條執行緒、一把鎖，沒有任何循環。" << std::endl;
    std::cout << "更關鍵的是：標準規定這是【未定義行為】，"
                 "根本沒保證它會卡住——" << std::endl;
    std::cout << "本機實測停住，是【實作結果】，不是【語言承諾】。" << std::endl;

    std::cout << "\n=== 日常實務: 重構後才長出來的間接重入 ===" << std::endl;
    {
        ShoppingCart cart;
        cart.addItem(1200);
        cart.addItem(350);
        cart.addItem(89);
        std::cout << "加入 3 件商品後總金額: " << cart.total()
                  << "  (預期 1639)" << std::endl;

        cart.recalculate();      // 單獨呼叫公開介面也正常
        std::cout << "單獨呼叫 recalculate() 後總金額: " << cart.total()
                  << "  (預期 1639，不會重入)" << std::endl;

        cart.addBatch({100, 200, 300});
        std::cout << "批次加入 3 件後總金額: " << cart.total()
                  << "  (預期 2239)" << std::endl;
        std::cout << "商品件數: " << cart.itemCount() << "  (預期 6)" << std::endl;

        // 多執行緒驗證：大量交錯呼叫也不會重入或資料不一致
        ShoppingCart shared;
        std::vector<std::thread> workers;
        for (int t = 0; t < 8; ++t) {
            workers.emplace_back([&shared]() {
                for (int i = 0; i < 500; ++i) {
                    shared.addItem(10);
                    shared.recalculate();
                }
            });
        }
        for (auto& t : workers) t.join();

        std::cout << "8 執行緒各加 500 件（每件 10 元）後總金額: "
                  << shared.total() << "  (必須是 40000)" << std::endl;
        std::cout << "重點: 公開函式彼此不互相呼叫，所以永遠不會重入"
                  << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.4：互斥鎖的常見錯誤5.cpp' -o double_lock
// 觀察 UB（本平台會卡住，務必加 timeout）:
//   g++ -std=c++17 -Wall -Wextra -pthread -DDEMONSTRATE_UB '課程 5.4：互斥鎖的常見錯誤5.cpp' -o double_lock_ub
//   timeout 5 ./double_lock_ub ; echo "exit=$?"      # 本機實測 exit=124
// 讓 ThreadSanitizer 指出「double lock」並顯示兩次上鎖的堆疊:
//   g++ -std=c++17 -pthread -g -fsanitize=thread -DDEMONSTRATE_UB '課程 5.4：互斥鎖的常見錯誤5.cpp' -o double_lock_tsan

// -----------------------------------------------------------------------------
// 【輸出但書】
//   1. 以下是【預設編譯】（不加 -DDEMONSTRATE_UB）的輸出。
//      加上 -DDEMONSTRATE_UB 後 outerFunction() 會觸發【未定義行為】；
//      本機實測是印出 "Outer function" 後永久停住
//      （timeout 5 觀察到 exit=124），但這是本平台的實作結果，
//      【不是】標準保證——換平台可能是別的症狀。
//   2. 其餘輸出完全確定：兩種解法與實務範例都不依賴排程順序，
//      多執行緒區段只驗證「總金額」這個不變量。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// 已略過會卡住的重複鎖定示範（預設關閉）。
// 要重現請加上 -DDEMONSTRATE_UB 重新編譯。
//
// === 解法 1: 重構成「不上鎖的私有實作」（推薦）===
// Outer function（重構後）
// Inner function（不上鎖版本）
// Inner function（不上鎖版本）
// 重構後鎖仍可正常取得: 是
//
// === 解法 2: 改用 std::recursive_mutex（次選）===
// Outer function（recursive_mutex）
// Inner function（recursive_mutex）
// recursive_mutex 允許同一執行緒重入，但會掩蓋設計問題
//
// === 為什麼說「UB」而不是「死結」 ===
// 死結的定義是【兩個以上】執行緒【互相】等待，形成循環。
// 本檔只有一條執行緒、一把鎖，沒有任何循環。
// 更關鍵的是：標準規定這是【未定義行為】，根本沒保證它會卡住——
// 本機實測停住，是【實作結果】，不是【語言承諾】。
//
// === 日常實務: 重構後才長出來的間接重入 ===
// 加入 3 件商品後總金額: 1639  (預期 1639)
// 單獨呼叫 recalculate() 後總金額: 1639  (預期 1639，不會重入)
// 批次加入 3 件後總金額: 2239  (預期 2239)
// 商品件數: 6  (預期 6)
// 8 執行緒各加 500 件（每件 10 元）後總金額: 40000  (必須是 40000)
// 重點: 公開函式彼此不互相呼叫，所以永遠不會重入
