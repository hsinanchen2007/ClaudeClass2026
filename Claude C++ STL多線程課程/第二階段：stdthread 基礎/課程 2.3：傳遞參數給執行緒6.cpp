// =============================================================================
//  課程 2.3：傳遞參數給執行緒6.cpp  —  把「只能移動」的資源交給執行緒
// =============================================================================
//
// 【主題資訊 Information】
//   語法      : std::thread t(func, std::move(ptr));
//   標準版本  : C++11(std::thread、std::unique_ptr、移動語意)
//   標頭檔    : <thread>、<memory>、<utility>(std::move)
//   關鍵規則  : std::thread 對參數做 decay-copy;對只能移動的型別,
//               這個「複製」會以移動的形式完成 —— 但前提是你傳的是右值
//   移動之後  : 原本的 unique_ptr 變成 nullptr,所有權已轉移給執行緒
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼一定要寫 std::move】
//     auto ptr = std::make_unique<int>(42);
//     std::thread t(process, ptr);              // ✗ 編譯失敗
//     std::thread t(process, std::move(ptr));   // ✅
//
// std::unique_ptr 刻意刪除了複製建構子 —— 這正是它「獨佔所有權」語意的實現:
// 如果能複製,就會有兩個 unique_ptr 指向同一塊記憶體,兩個都會 delete 它,
// 造成 double free。
// 而 std::thread 需要把參數「弄一份」存進執行緒的內部儲存。
// 對可複製的型別它就複製;對只能移動的型別,它會移動 ——
// 但移動只能從右值進行,所以你必須用 std::move 把 ptr 轉成右值,
// 明確表達「我放棄這個資源的所有權」。
//
// 【2. std::move 其實什麼都沒搬】
// 這是最常見的誤解。std::move 只是一個型別轉換 ——
// 它把左值轉成右值參考,不產生任何機器碼,也不搬動任何位元組。
// 真正搬東西的是「移動建構子」:unique_ptr 的移動建構子會把來源的
// 內部指標複製過來,然後把來源設成 nullptr。
// 所以 std::move(ptr) 之後 ptr 變成 nullptr,不是因為 std::move,
// 而是因為 unique_ptr 的移動建構子這樣寫。
//
// 【3. 移動之後,原本的 ptr 處於什麼狀態】
// 標準對「被移動走的物件」的通則是:處於「有效但未指定」的狀態 ——
// 你可以安全地銷毀它、可以重新賦值,但不該假設它的值。
// 不過 std::unique_ptr 是少數有「明確保證」的型別:
// 移動之後它保證等於 nullptr。所以本檔可以放心地斷言 ptr == nullptr,
// 這是標準保證,不是實作巧合。
// ⚠️ 但這個保證不能套用到其他型別:被移動走的 std::string 或
//    std::vector 是「有效但未指定」,不保證是空的(雖然主流實作通常是)。
//
// 【4. 這個模式解決了什麼真實問題】
// 「把一份資源完整交給背景執行緒」是 detach 唯一安全的做法。
// 對照本課第 5 個範例檔的懸空 char*:那裡的問題是執行緒「借用」了
// 呼叫端的東西;而這裡是「拿走」——所有權轉移之後,呼叫端再也碰不到它,
// 也就不可能發生「呼叫端先銷毀」的競爭。
// 用型別系統來消滅一整類 bug,這是現代 C++ 最漂亮的地方之一。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 unique_ptr 的移動幾乎沒有成本
//   unique_ptr 內部通常就只有一個裸指標(對預設的 deleter 而言,
//   本機實測 sizeof(std::unique_ptr<int>) 等於 sizeof(int*))。
//   移動 = 複製那個指標 + 把來源設為 nullptr,兩道指令的事,
//   完全不碰堆積上的物件。這就是為什麼「移動」通常比「複製」快得多。
//
// (B) 函式參數該用 unique_ptr<T> 還是 unique_ptr<T>&&
//   本例的 process(std::unique_ptr<int> ptr) 按值接收,是慣用做法:
//   它在簽名上就宣告了「我會拿走所有權」,呼叫端不寫 std::move 就編譯不過,
//   意圖無法被誤解。用右值參考 && 也可行,但按值更清楚也更安全。
//
// (C) 什麼時候該用 shared_ptr 而不是 unique_ptr
//   unique_ptr 適合「交出去就不再需要」;若呼叫端自己也還要用,
//   或有多條執行緒都要用同一份資源,就該用 shared_ptr ——
//   複製它會增加參考計數,每個持有者都能安全使用,
//   最後一個離開的才釋放(見本課第 4 個範例檔的實務範例)。
//
// 【注意事項 Pay Attention】
// 1. 只能移動的型別必須用 std::move 傳入,否則編譯失敗。
// 2. std::move 只是型別轉換,真正搬東西的是移動建構子。
// 3. 移動之後 unique_ptr 保證是 nullptr(這是標準對 unique_ptr 的明確保證);
//    但「被移動走的物件是空的」不能套用到所有型別。
// 4. 移動之後絕對不要再解參考原本的 ptr —— 那是解參考 nullptr,UB。
// 5. 若呼叫端自己還需要那份資源,就不該用 unique_ptr 移動,改用 shared_ptr。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】把 move-only 資源傳給執行緒
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼把 std::unique_ptr 傳給 std::thread 一定要寫 std::move?
//     答：unique_ptr 刪除了複製建構子(否則會有兩個指標 delete 同一塊記憶體),
//         而 std::thread 必須把參數「弄一份」存進執行緒內部。
//         對只能移動的型別,它會用移動完成,但移動只能從右值進行,
//         所以必須用 std::move 把左值轉成右值,明確表達放棄所有權。
//     追問：std::move 到底做了什麼?
//         → 什麼都沒搬。它只是個型別轉換,把左值轉成右值參考,不產生機器碼。
//           真正搬東西的是 unique_ptr 的移動建構子 ——
//           它複製內部指標,然後把來源設成 nullptr。
//
// 🔥 Q2. std::move(ptr) 之後,ptr 的值是什麼?
//     答：對 std::unique_ptr 而言,標準明確保證移動後等於 nullptr,
//         可以放心斷言。但這是 unique_ptr 的特別保證 ——
//         標準對一般型別的通則只是「有效但未指定的狀態」,
//         被移動走的 std::string / std::vector 不保證是空的。
//     追問：那被移動走的物件還能做什麼?
//         → 可以安全地銷毀,也可以重新賦值。但不該讀取它的值,
//           更不該解參考(對 unique_ptr 來說那是解參考 nullptr)。
//
// ⚠️ 陷阱. 「我把 unique_ptr move 進執行緒了,那我在主執行緒
//         只是『看一下』它有沒有值,應該沒關係吧?」
//     答：「看一下」本身是安全的 —— ptr 移動後保證是 nullptr,
//         讀這個值完全合法。真正的錯誤是「以為它還有值而去用它」:
//         *ptr 或 ptr->foo() 都是解參考 nullptr,未定義行為。
//         而且危險的是,編譯器不會給你任何警告。
//     為什麼會錯：把「移動」想像成「複製了一份給對方,我這邊也還在」。
//         移動的語意是「轉移」,不是「分享」—— 交出去之後你手上就沒有了。
//         真的需要雙方都能用,那從一開始就該選 shared_ptr,
//         用型別把「共享」這個意圖表達出來。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

// 保護 std::cout,避免多執行緒輸出互相切開
std::mutex g_ioMutex;

void say(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_ioMutex);
    std::cout << msg << std::endl;
}

// 按值接收 → 簽名上就宣告「我會拿走所有權」
void process(std::unique_ptr<int> ptr) {
    say("  Value: " + std::to_string(*ptr));
}   // ptr 在這裡銷毀 → 記憶體由執行緒釋放

// -----------------------------------------------------------------------------
// 【日常實務範例】把整批待寫入的資料交給背景寫檔執行緒
//   情境: 主執行緒收集了一批 log,要交給背景執行緒批次寫入磁碟,
//         自己立刻回去服務下一個請求。這裡「交出去就不再需要」的語意
//         用 unique_ptr 表達最精確 —— 移動之後主執行緒連碰都碰不到,
//         從型別上就杜絕了「兩邊同時寫同一塊資料」的競爭。
//   為什麼用本主題: 這是【詳細解釋 4】所說「用所有權轉移消滅整類 bug」
//                   的真實樣貌,也是 detach 背景任務唯一安全的資料傳遞方式。
// -----------------------------------------------------------------------------
struct LogBatch {
    std::vector<std::string> lines;

    ~LogBatch() {
        say("      (LogBatch 銷毀 —— 由背景執行緒負責,主執行緒早就交出去了)");
    }
};

void flushToDisk(std::unique_ptr<LogBatch> batch, int batchId) {
    say("    [寫檔] 批次 #" + std::to_string(batchId) + " 共 " +
        std::to_string(batch->lines.size()) + " 行");
    for (const std::string& l : batch->lines) {
        say("      · " + l);
    }
}   // batch 在此銷毀

void collectAndFlush(int batchId) {
    auto batch = std::make_unique<LogBatch>();
    batch->lines.push_back("GET /api/users 200");
    batch->lines.push_back("POST /api/orders 201");
    batch->lines.push_back("GET /api/health 200");

    say("    主執行緒交出前:batch " + std::string(batch ? "有效" : "已為空"));

    std::thread t(flushToDisk, std::move(batch), batchId);

    // 移動之後,主執行緒手上的 batch 保證是 nullptr —— 想誤用也用不了
    say("    主執行緒交出後:batch " + std::string(batch ? "有效" : "已為空(nullptr)"));

    t.join();
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔從缺,理由如下
//   本檔的主題是「所有權轉移與移動語意」,是 C++ 的資源管理設計問題。
//   LeetCode 的並行題(1114 Print in Order、1115 Print FooBar Alternately、
//   1116 Print Zero Even Odd、1117 Building H2O、1195 Fizz Buzz Multithreaded)
//   由評測框架建立執行緒並管理物件生命週期,參賽者既不傳參數也不管所有權,
//   考點完全在同步原語上,用不到 std::move。
//   本課第 7 個範例檔會示範真正對應得上的 1116,此處從缺以免失焦。
// -----------------------------------------------------------------------------

int main() {
    std::cout << "=== 原始示範:把 unique_ptr 移動進執行緒 ===" << std::endl;
    {
        auto ptr = std::make_unique<int>(42);
        std::cout << "  移動前 ptr " << (ptr ? "有效" : "為空") << std::endl;

        std::thread t(process, std::move(ptr));   // 必須 move
        t.join();

        // 標準保證:被移動走的 unique_ptr 等於 nullptr
        std::cout << "  移動後 ptr " << (ptr ? "有效" : "為空(nullptr,標準保證)")
                  << std::endl;
        std::cout << "  ↑ 讀它的值是合法的;但解參考 *ptr 就是 UB" << std::endl;
    }

    std::cout << "\n=== 對照:不寫 std::move 會怎樣 ===" << std::endl;
    std::cout << "  std::thread t(process, ptr);   // ✗ 編譯失敗" << std::endl;
    std::cout << "  因為 unique_ptr 的複製建構子被刪除了(= delete),"
                 "而 thread 必須把參數弄一份存起來。" << std::endl;
    std::cout << "  (該行未寫進本檔,否則整支程式無法編譯)" << std::endl;

    std::cout << "\n=== unique_ptr 的大小(實作定義) ===" << std::endl;
    std::cout << "  sizeof(std::unique_ptr<int>) = " << sizeof(std::unique_ptr<int>)
              << ",sizeof(int*) = " << sizeof(int*)
              << "  → 預設 deleter 下沒有額外開銷" << std::endl;

    std::cout << "\n=== 實務:把整批 log 交給背景寫檔執行緒 ===" << std::endl;
    collectAndFlush(7);

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.3：傳遞參數給執行緒6.cpp" -o move_to_thread

// 注意:以下為某一次實際執行的結果,每次執行都相同 ——
//       因為所有跨執行緒的觀察都發生在 join() 之後,
//       而 sizeof 與「移動後為 nullptr」都是編譯期/標準層級的保證。
//       (sizeof(std::unique_ptr<int>) = 8 是本機 x86-64 的實作定義值,
//        在 32 位元平台上會是 4。)

// === 預期輸出 ===
// === 原始示範:把 unique_ptr 移動進執行緒 ===
//   移動前 ptr 有效
//   Value: 42
//   移動後 ptr 為空(nullptr,標準保證)
//   ↑ 讀它的值是合法的;但解參考 *ptr 就是 UB
//
// === 對照:不寫 std::move 會怎樣 ===
//   std::thread t(process, ptr);   // ✗ 編譯失敗
//   因為 unique_ptr 的複製建構子被刪除了(= delete),而 thread 必須把參數弄一份存起來。
//   (該行未寫進本檔,否則整支程式無法編譯)
//
// === unique_ptr 的大小(實作定義) ===
//   sizeof(std::unique_ptr<int>) = 8,sizeof(int*) = 8  → 預設 deleter 下沒有額外開銷
//
// === 實務:把整批 log 交給背景寫檔執行緒 ===
//     主執行緒交出前:batch 有效
//     主執行緒交出後:batch 已為空(nullptr)
//     [寫檔] 批次 #7 共 3 行
//       · GET /api/users 200
//       · POST /api/orders 201
//       · GET /api/health 200
//       (LogBatch 銷毀 —— 由背景執行緒負責,主執行緒早就交出去了)
