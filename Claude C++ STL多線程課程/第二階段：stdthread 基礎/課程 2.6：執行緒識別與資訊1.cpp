// =============================================================================
//  課程 2.6：執行緒識別與資訊1.cpp
//  主題: 取得執行緒 ID —— std::this_thread::get_id() 與 t.get_id() 的差別
// =============================================================================
//
// 【主題資訊 Information】
//   namespace std::this_thread {
//       thread::id get_id() noexcept;          // C++11 — 取得「呼叫者自己」的 id
//   }
//   class thread {
//       id get_id() const noexcept;            // C++11 — 取得「這個物件所管理的執行緒」的 id
//   };
//
//   標頭檔  : <thread>
//   型別    : std::thread::id — 不透明(opaque)型別
//   複雜度  : 兩者皆 O(1) 且 noexcept(絕不丟例外、絕不阻塞)
//   本機實測: sizeof(std::thread::id) == 8 (g++ 15.2.0 / libstdc++ / x86-64)
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼要有「執行緒 id」這個東西】
// 多執行緒程式最先失去的就是「我現在在哪裡執行」這個直覺。單執行緒時,程式碼的
// 位置就代表執行的位置;一旦有了多個執行緒,同一段程式碼會被 N 個執行緒同時踩過,
// 你在 log 裡看到的每一行都可能來自不同的執行緒。thread::id 的存在目的只有一個:
// 讓你能在執行期回答「這一行是誰跑的」「這兩次呼叫是不是同一個執行緒」。
//
// 注意它的定位——它是一個「識別用的標籤」,不是控制代碼(handle)。你不能拿 id 去
// 叫醒、暫停、終止一個執行緒;要做那些事得走 native_handle()(見本課第 6 個檔案)。
// 標準把兩者刻意分開:id 是可攜、安全、唯讀的識別;native_handle 是不可攜的逃生艙。
//
// 【2. 兩種 get_id() 的根本差別:問「我是誰」 vs 問「它是誰」】
//   std::this_thread::get_id()
//       自由函式,沒有參數。回傳「執行到這一行的那個執行緒」的 id。
//       它永遠回傳一個代表真實執行緒的 id(你既然在執行,你當然存在)。
//       主執行緒也算執行緒,在 main() 裡呼叫它一樣有值。
//
//   t.get_id()
//       成員函式,問的是「thread 物件 t 目前管理著哪一個執行緒」。
//       關鍵在於 t 是一個「握把」,它有可能沒握著任何東西:
//         - 預設建構的 std::thread
//         - 已經 join() 過的
//         - 已經 detach() 過的
//         - 被 std::move() 搬走內容的
//       這四種情況 t.get_id() 都回傳「預設建構的 id」,也就是 std::thread::id{}。
//
// 換句話說,this_thread::get_id() 問的是「主體」,t.get_id() 問的是「物件所有權」。
// 初學者最常見的誤解,是以為 t.get_id() 會一直有效——實際上 t.join() 回來的那一刻,
// t 就空了,再問它 id 只會拿到「不代表任何執行緒」的預設值。
//
// 【3. 預設建構的 id:代表「不是任何執行緒」】
// std::thread::id{} 是一個合法、可比較、可 hash 的值,語意是「不指向任何執行緒」。
// 標準保證:所有 non-joinable 的 thread 物件,get_id() 都相等,且都等於 id{}。
// 這也直接給出了 joinable() 的精確定義——標準規定:
//       t.joinable()  <==>  t.get_id() != std::thread::id()
// 本機實測確認這個等價關係成立(見 main() 的「joinable 與 id 的等價」段)。
//
// 【4. 輸出格式是「實作定義」,不要對它的內容寫死任何假設】
// operator<< 對 thread::id 的輸出,標準只要求:相等的 id 印出相同文字、不相等的
// id 印出不同文字。至於長什麼樣子,完全由實作決定。
//   - libstdc++(本機):真實執行緒印出底層 pthread_t 的數值,例如 124418109994688;
//                       預設 id 則印出一整串英文字 "thread::id of a non-executing thread"。
//   - MSVC / libc++ 的格式與 libstdc++ 都不一樣。
// 所以「thread id 一定是個數字」是錯的——本機的預設 id 就不是數字。
// 任何靠 parse 這段字串來取值的程式碼,換一個編譯器就會壞掉。
//
// 【概念補充 Concept Deep Dive】
// (A) libstdc++ 裡 thread::id 到底是什麼
//     它是一個只包了一個 native_handle_type(在 Linux 上就是 pthread_t)的 struct:
//         class thread::id { native_handle_type _M_thread; ... };
//     本機實測:sizeof(thread::id) == sizeof(pthread_t) == 8,而且對同一個執行緒,
//     t.get_id() 印出來的數值與 t.native_handle() 完全相同。glibc 的 pthread_t
//     其實是該執行緒 TCB(thread control block)的位址,所以那串數字本質上是個指標值。
//
// (B) 為什麼標準要把它包成「不透明型別」而不是直接給 unsigned long
//     因為不同平台的執行緒識別根本不同型:Linux 是指標大小的 pthread_t,Windows 是
//     DWORD 的 thread id,有些嵌入式 RTOS 是小整數。若標準規定成整數,就得選一個
//     寬度並強迫所有平台轉換。包成 opaque 型別後,標準只承諾「可比較、可 hash、
//     可輸出」這組最小介面,實作愛放什麼都行——這是典型的以介面換可攜性。
//
// (C) 這組最小介面剛好夠用來當關聯容器的 key
//     標準要求 thread::id 支援 == != < <= > >=(C++20 起改由 <=> 產生),並且特化了
//     std::hash<std::thread::id>。所以 std::map<std::thread::id, X> 與
//     std::unordered_map<std::thread::id, X> 兩種都能直接用,不必自己寫比較器。
//     ——但「能當 key」不等於「能當永久 key」,原因見下一個檔案(id 會被重複使用)。
//
// 【注意事項 Pay Attention】
// 1. t.get_id() 在 join()/detach()/move 之後會變成預設 id;想留 id 就要在那之前先存起來。
// 2. 印出來的格式是實作定義,不可寫死、不可 parse、不可假設它是數字。
// 3. 不要把 id 當控制代碼——它不能用來停止或喚醒執行緒。
// 4. 兩個函式都是 noexcept,但 t.get_id() 只是讀取狀態,對「t 正在跑」的執行緒
//    沒有任何同步作用;它不會等待、也不保證那個執行緒此刻仍在執行。
// 5. 多個執行緒同時 std::cout << ... 不會有 data race(標準保證同步標準串流物件的
//    格式化輸出不產生 data race),但字元可能交錯,那是 race condition 不是 UB。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】執行緒識別 get_id()
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::this_thread::get_id() 與 t.get_id() 差在哪?
//     答：前者回傳「執行到這行的執行緒」自己的 id,永遠對應一個真實執行緒;
//         後者回傳「thread 物件 t 目前管理的執行緒」的 id,若 t 是 non-joinable
//         (預設建構/已 join/已 detach/被 move 走),則回傳預設 id std::thread::id{}。
//     追問：在子執行緒裡呼叫 t.get_id()(t 是它自己那個物件)拿得到嗎?
//         → 拿得到,但那是跨執行緒讀取 t 這個物件,若主執行緒同時 join()/move 它,
//           就是對同一物件的未同步讀寫 = data race。實務上一律用 this_thread::get_id()。
//
// 🔥 Q2. std::thread::id 印出來保證是數字嗎?
//     答：不保證。標準只要求「相等的 id 印出相同文字、不等的印出不同文字」,格式
//         完全是實作定義。本機 libstdc++ 對真實執行緒印 pthread_t 數值,但對預設
//         id 印的是英文句子 "thread::id of a non-executing thread"——根本不是數字。
//     追問：那要怎麼得到穩定的短代號?→ 自己維護 map<thread::id,int> 發序號,
//         或用 std::hash<std::thread::id> 取雜湊(但雜湊值同樣是實作定義)。
//
// ⚠️ 陷阱. 下面這段為什麼印不出你要的東西?
//         std::thread t(work);
//         t.join();
//         std::cout << "剛剛那條執行緒是 " << t.get_id();   // ← 錯
//     答：join() 之後 t 已經不再關聯任何執行緒,get_id() 回傳預設 id,
//         本機會印出 "thread::id of a non-executing thread"。
//         正確做法是 join() 之前先 auto id = t.get_id(); 存下來。
//     為什麼會錯：腦中把 t 當成「那條執行緒本身」,以為它會一直記得自己的身分。
//         實際上 std::thread 只是可移動的「所有權握把」,join() 就是交還所有權,
//         交還後握把當然是空的。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <mutex>
#include <sstream>

// -----------------------------------------------------------------------------
// 【日常實務範例】為每一行 log 打上執行緒標記
// 情境: 線上服務出事後看 log,最需要回答的第一個問題是「這幾行是不是同一條執行緒
//       跑出來的」。做法是在每行前面蓋上 this_thread::get_id()。
// 為何用 get_id: 它是唯一「不需要傳參數、在任何函式深處都能問出我是誰」的方法,
//       不必把執行緒編號一路當參數傳下去。
// 附帶示範: 用 mutex 保護 cout,避免多執行緒輸出「字元交錯」(race condition,
//       不是 data race——cout 本身標準保證不會 data race)。
// -----------------------------------------------------------------------------
std::mutex g_logMutex;

void logLine(const std::string& msg) {
    std::ostringstream oss;
    oss << "[tid " << std::this_thread::get_id() << "] " << msg << "\n";
    std::lock_guard<std::mutex> lk(g_logMutex);
    std::cout << oss.str();
}

void handleRequest(int reqId) {
    logLine("開始處理請求 #" + std::to_string(reqId));
    logLine("完成請求 #" + std::to_string(reqId));
}

int main() {
    std::cout << "=== 原始示範:this_thread::get_id() vs t.get_id() ===" << std::endl;

    // 取得主執行緒 ID
    std::cout << "主執行緒 ID: " << std::this_thread::get_id() << std::endl;

    std::thread t([]() {
        // 在執行緒內部取得自己的 ID
        std::cout << "子執行緒 ID: " << std::this_thread::get_id() << std::endl;
    });

    // 從外部取得執行緒 ID
    // 注意:這裡必須在 join() 之前問,join() 之後 t 就空了
    std::cout << "t 的 ID: " << t.get_id() << std::endl;

    t.join();

    std::cout << "\n=== join() 之後 t 已不再關聯任何執行緒 ===" << std::endl;
    std::cout << "join 後 t.get_id(): " << t.get_id() << std::endl;
    std::cout << "等於預設 id 嗎? " << (t.get_id() == std::thread::id{}) << std::endl;

    std::cout << "\n=== joinable 與 id 的等價(標準定義) ===" << std::endl;
    std::thread t2([]{});
    std::cout << "跑動中  joinable=" << t2.joinable()
              << " , (get_id()!=id{})=" << (t2.get_id() != std::thread::id{}) << std::endl;
    t2.join();
    std::cout << "join 後 joinable=" << t2.joinable()
              << " , (get_id()!=id{})=" << (t2.get_id() != std::thread::id{}) << std::endl;

    std::cout << "\n=== 預設建構的 id 印出來長什麼樣(實作定義) ===" << std::endl;
    std::cout << "std::thread::id{} = " << std::thread::id{} << std::endl;

    std::cout << "\n=== 日常實務: 帶執行緒標記的 log ===" << std::endl;
    logLine("主執行緒:服務啟動");
    std::thread w1(handleRequest, 1001);
    std::thread w2(handleRequest, 1002);
    w1.join();
    w2.join();
    logLine("主執行緒:服務結束");

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.6：執行緒識別與資訊1.cpp" -o thread_id_basic

// 以下為某次實際執行結果。thread id 的「數值」每次執行都不同,且格式為實作定義

// === 預期輸出 ===
// (本機 libstdc++ 印的是 pthread_t 數值);兩條 worker 的 log 先後順序也每次不同。
// 唯一穩定的是:t 的 ID 與子執行緒自報的 ID 相同、join 後變成預設 id、
// 以及 joinable 與 (get_id()!=id{}) 永遠一致。
//
// === 原始示範:this_thread::get_id() vs t.get_id() ===
// 主執行緒 ID: 128272783337408
// t 的 ID: 128272776033984
// 子執行緒 ID: 128272776033984
//
// === join() 之後 t 已不再關聯任何執行緒 ===
// join 後 t.get_id(): thread::id of a non-executing thread
// 等於預設 id 嗎? 1
//
// === joinable 與 id 的等價(標準定義) ===
// 跑動中  joinable=1 , (get_id()!=id{})=1
// join 後 joinable=0 , (get_id()!=id{})=0
//
// === 預設建構的 id 印出來長什麼樣(實作定義) ===
// std::thread::id{} = thread::id of a non-executing thread
//
// === 日常實務: 帶執行緒標記的 log ===
// [tid 128272783337408] 主執行緒:服務啟動
// [tid 128272776033984] 開始處理請求 #1001
// [tid 128272776033984] 完成請求 #1001
// [tid 128272767641280] 開始處理請求 #1002
// [tid 128272767641280] 完成請求 #1002
// [tid 128272783337408] 主執行緒:服務結束
//
// ★ 值得注意:上面這次執行中,worker w1 拿到的 id(128272776033984)與前面那條
//   早就 join() 完的 t 一模一樣——這不是巧合,而是「執行緒 id 會被重複使用」的
//   真實現場。t 結束並被回收後,它的 pthread_t 被下一條執行緒接手。
//   下一個檔案(課程 2.6...2.cpp)會用 20 條執行緒把這個現象放大給你看,
//   並說明為什麼「拿 id 當永久 key」是經典錯誤。
