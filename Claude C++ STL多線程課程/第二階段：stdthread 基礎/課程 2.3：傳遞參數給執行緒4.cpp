// =============================================================================
//  課程 2.3：傳遞參數給執行緒4.cpp  —  傳指標:為什麼它不需要 std::ref
// =============================================================================
//
// 【主題資訊 Information】
//   語法      : std::thread t(func, &value);      // 直接傳位址,不需 std::ref
//   標準版本  : C++11
//   標頭檔    : <thread>
//   關鍵差異  : 參考需要 std::ref,指標不需要 —— 因為「複製一個指標」
//               本來就會得到指向同一個東西的指標
//   風險      : 編譯器不再幫你把關生命週期,責任完全在你身上
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼指標可以直接傳,參考卻不行】
// 這個對比是理解 std::thread 參數傳遞的最佳切入點。
//
//   void modifyPtr(int* p);
//   std::thread t(modifyPtr, &value);       // ✅ 可以
//
//   void modify(int& x);
//   std::thread t(modify, value);           // ✗ 編譯失敗,需要 std::ref
//
// 差別不在 std::thread 對兩者有不同待遇 —— 它對兩者做的事完全一樣:
// 「複製參數,然後以右值傳給函式」。差別在於「複製之後還剩下什麼」:
//   * 複製一個 int* → 得到另一個 int*,仍指向同一個 int。共享關係被保留了。
//   * 複製一個 int  → 得到另一個獨立的 int。共享關係消失了,
//     而且它是右值,連綁定到 int& 都做不到 → 編譯失敗。
//
// 換句話說,指標之所以「不需要 std::ref」,是因為指標本身就是
// 「一個可以被複製的參考」—— 而這正是 std::reference_wrapper 的本質。
// std::ref 做的事,說穿了就是「把參考包裝成指標,讓它能被複製」。
//
// 【2. 那到底該用指標還是 std::ref】
// 兩者在執行期效果幾乎相同,選擇的依據是「介面語意」:
//
//   用 T*                        用 T& + std::ref
//   ────────────────────────    ────────────────────────
//   可以是 nullptr(代表「沒有」) 一定指向某個物件
//   呼叫端看得到 & 很醒目          呼叫端要記得寫 std::ref,忘了會編譯錯
//   函式內需要檢查 nullptr         不必檢查
//   可以重新指向別的物件           綁定後不可更換
//
// 實務準則:參數「可有可無」用指標;參數「一定要有」用參考。
// 而在 std::thread 的情境下,指標還有一個小優勢:
// 忘記寫 std::ref 是編譯錯誤(還算幸運),但如果函式吃的是「值」而你
// 以為它會改到原變數,那就是靜默的邏輯錯誤 —— 傳指標時這個誤會不會發生,
// 因為 & 這個符號會提醒你「我正在給出位址」。
//
// 【3. 真正的風險:生命週期】
// 傳指標時,編譯器完全不會幫你檢查那個物件活多久。
//     void spawn() {
//         int local = 1;
//         std::thread t(modifyPtr, &local);
//         t.detach();
//     }   // ✗ local 銷毀,但執行緒可能還沒執行 → 寫入已釋放的堆疊,UB
// 本檔的示範之所以安全,關鍵只有一個:t.join() 在 value 離開作用域之前。
// 這條規則對指標和 std::ref 完全一樣。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 指標被複製了,但它指的東西沒有
//   std::thread 內部大致是把參數存進一個 tuple:
//       std::tuple<void(*)(int*), int*>
//   第二個元素是 int* 的副本。副本和原本的指標是兩個不同的變數,
//   但它們存著同一個位址值,所以透過任何一個解參考,碰到的都是同一個 int。
//   這就是「淺複製」在這裡剛好是我們要的行為。
//
// (B) 為什麼這也解釋了 char[] 的坑
//   傳 char buffer[] 給執行緒時,陣列會 decay 成 char*,
//   thread 複製的是那個指標。指標本身活得好好的,但它指向的堆疊陣列
//   可能已經被銷毀 —— 這正是本課第 5 個範例檔要示範的未定義行為。
//   同樣是「複製指標」,安全與否完全取決於被指向的東西活多久。
//
// (C) 智慧指標怎麼傳
//   * std::shared_ptr:直接傳值即可。複製會增加參考計數,
//     從根本上保證物件活得比執行緒久 —— 這是最安全的做法。
//   * std::unique_ptr:不可複製,必須 std::move 進去(見本課第 6 個範例檔)。
//   在會 detach 的情境下,shared_ptr 幾乎是唯一安全的選擇。
//
// 【注意事項 Pay Attention】
// 1. 傳指標不需要 std::ref,但生命週期責任完全在你身上。
// 2. 執行緒可能在你離開作用域之後才真正開始執行 —— join 之前絕不能讓
//    被指向的物件死掉。detach 時尤其危險。
// 3. 多條執行緒透過指標存取同一物件、且有寫入時,仍需 mutex 或 atomic。
// 4. 函式若接受指標,記得處理 nullptr 的可能。
// 5. 需要跨執行緒共享物件又不確定生命週期時,優先考慮 std::shared_ptr。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】傳指標給執行緒
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 std::thread t(func, &value); 不需要 std::ref,
//        但吃參考的版本卻需要?
//     答：std::thread 對兩者做的事完全相同 —— 複製參數後以右值傳給函式。
//         差別在複製之後剩下什麼:複製一個指標仍然指向同一個物件,
//         共享關係被保留;複製一個 int 則得到獨立的副本,共享關係消失,
//         而且右值無法綁定到非 const 左值參考,直接編譯失敗。
//     追問：那 std::ref 到底做了什麼?
//         → 它把參考包裝成 reference_wrapper —— 本質上就是一個指標,
//           讓「參考」也變得可以被複製。所以指標和 std::ref 是同一個概念
//           的兩種寫法。
//
// ⚠️ 陷阱. 「傳指標給執行緒比傳參考安全,因為指標明確、不會有隱藏的
//         生命週期問題。」哪裡錯了?
//     答：完全相反。兩者的生命週期風險一模一樣 —— 指標和 reference_wrapper
//         內部都只是存了一個位址,被指向的物件一旦銷毀,兩者都是懸空。
//         而且指標「看起來明確」反而容易讓人放鬆警惕:
//         編譯器對這兩種寫法都不做任何生命週期檢查。
//     為什麼會錯：把「語法上明確」誤當成「語意上安全」。
//         & 這個符號只是讓你看見自己傳了位址,它不會、也不能告訴你
//         那個位址在執行緒真正跑到時是否還有效。
//         真正的保護只有兩種:確保 join() 在物件銷毀之前,
//         或改用 shared_ptr 讓物件的壽命由參考計數決定。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// 保護 std::cout,避免多執行緒輸出互相切開
std::mutex g_ioMutex;

void say(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_ioMutex);
    std::cout << msg << std::endl;
}

void modifyPtr(int* p) {
    if (p == nullptr) return;      // 接受指標就要處理 nullptr
    *p = 200;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】用 shared_ptr 讓「可能還沒跑完」的背景任務保持安全
//   情境: 上傳完成後要在背景寫一筆稽核紀錄。這個背景工作可能比建立它的
//         函式活得更久,若用裸指標指向區域物件,函式一返回就是懸空。
//         改用 shared_ptr:複製一份進執行緒 → 參考計數 +1 →
//         只要執行緒還沒跑完,物件就一定還活著。
//   為什麼用本主題: 這是【概念補充 C】的實作,也是「傳指標」在真實系統中
//                   唯一能安心 detach 的做法。
// -----------------------------------------------------------------------------
struct AuditRecord {
    std::string user;
    std::string action;
    long        bytes;

    ~AuditRecord() {
        say("      (AuditRecord 銷毀 —— 由最後一個持有者觸發)");
    }
};

void writeAudit(std::shared_ptr<AuditRecord> rec) {
    // 就算呼叫端早就返回了,rec 仍持有一份所有權,物件保證還活著
    say("    [稽核] " + rec->user + " 執行 " + rec->action + ",共 " +
        std::to_string(rec->bytes) + " bytes(此時參考計數 = " +
        std::to_string(rec.use_count()) + ")");
}

void handleUpload() {
    auto rec = std::make_shared<AuditRecord>();
    rec->user   = "alice";
    rec->action = "upload";
    rec->bytes  = 4096;

    say("    handleUpload 建立紀錄,參考計數 = " + std::to_string(rec.use_count()));

    // 複製 shared_ptr 進執行緒 → 計數 +1
    std::thread t(writeAudit, rec);
    t.join();      // 這裡示範用 join;即使改成 detach,物件也不會提早消失

    say("    handleUpload 即將返回,參考計數 = " + std::to_string(rec.use_count()));
}   // rec 銷毀 → 計數歸零 → AuditRecord 才真正被釋放

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔從缺,理由如下
//   本檔比較的是「指標 vs 參考」這兩種參數傳遞方式的語意差異,
//   屬於 C++ 的物件與生命週期主題。LeetCode 的並行題
//   (1114 Print in Order、1115 Print FooBar Alternately、1116 Print Zero Even Odd、
//   1117 Building H2O、1195 Fizz Buzz Multithreaded)由評測框架自行建立執行緒,
//   你無從決定參數怎麼傳,考點也全在同步原語上。
//   本課第 7 個範例檔會示範真正用得上的 1116,此處從缺。
// -----------------------------------------------------------------------------

int main() {
    std::cout << "=== 原始示範:傳指標修改原變數 ===" << std::endl;
    {
        int value = 1;
        std::cout << "  呼叫前 value = " << value << std::endl;

        std::thread t(modifyPtr, &value);   // 不需要 std::ref
        t.join();                           // join 之前 value 一定還活著

        std::cout << "  呼叫後 value = " << value << std::endl;
    }

    std::cout << "\n=== 指標 vs std::ref:兩種寫法效果相同 ===" << std::endl;
    {
        int a = 1, b = 1;
        std::thread t1(modifyPtr, &a);
        std::thread t2([](int& x) { x = 200; }, std::ref(b));
        t1.join();
        t2.join();
        std::cout << "  傳指標   → a = " << a << std::endl;
        std::cout << "  std::ref → b = " << b << std::endl;
        std::cout << "  ↑ 兩者本質相同:都是「把一個位址複製進執行緒」"
                  << std::endl;
    }

    std::cout << "\n=== 多條執行緒寫「不同」的格子:免鎖也安全 ===" << std::endl;
    {
        std::vector<int> slots(4, 0);
        std::vector<std::thread> pool;
        for (int i = 0; i < 4; ++i) {
            // 每條執行緒拿到不同的位址 → 沒有兩條寫同一處 → 沒有資料競爭
            pool.emplace_back(modifyPtr, &slots[static_cast<std::size_t>(i)]);
        }
        for (std::thread& t : pool) t.join();

        std::cout << "  slots = ";
        for (int s : slots) std::cout << s << " ";
        std::cout << " (全部被寫成 200,且不需要任何 mutex)" << std::endl;
    }

    std::cout << "\n=== 實務:用 shared_ptr 保證背景任務期間物件不死 ===" << std::endl;
    handleUpload();
    std::cout << "  ↑ 注意銷毀訊息出現在 handleUpload 返回之後 ——"
                 "物件的壽命由參考計數決定,不是由作用域" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.3：傳遞參數給執行緒4.cpp" -o pass_by_ptr

// 注意:以下為某一次實際執行的結果。
//   本檔的輸出「每次執行都相同」——所有結果都在 join() 之後才讀取,
//   而 join 建立了 happens-before 關係;實務段的參考計數變化也是確定的。
//   這再次說明:多執行緒程式的輸出並非注定混亂,同步做對了就是確定的。

// === 預期輸出 ===
// === 原始示範:傳指標修改原變數 ===
//   呼叫前 value = 1
//   呼叫後 value = 200
//
// === 指標 vs std::ref:兩種寫法效果相同 ===
//   傳指標   → a = 200
//   std::ref → b = 200
//   ↑ 兩者本質相同:都是「把一個位址複製進執行緒」
//
// === 多條執行緒寫「不同」的格子:免鎖也安全 ===
//   slots = 200 200 200 200  (全部被寫成 200,且不需要任何 mutex)
//
// === 實務:用 shared_ptr 保證背景任務期間物件不死 ===
//     handleUpload 建立紀錄,參考計數 = 1
//     [稽核] alice 執行 upload,共 4096 bytes(此時參考計數 = 2)
//     handleUpload 即將返回,參考計數 = 1
//       (AuditRecord 銷毀 —— 由最後一個持有者觸發)
//   ↑ 注意銷毀訊息出現在 handleUpload 返回之後 ——物件的壽命由參考計數決定,不是由作用域
