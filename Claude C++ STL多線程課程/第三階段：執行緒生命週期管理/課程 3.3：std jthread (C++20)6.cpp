// =============================================================================
//  課程 3.3：std::jthread (C++20) — 第 6 部分：介面總覽（synopsis）
// =============================================================================
// 檔案：lesson_3_3_jthread_synopsis.cpp
// 說明：std::jthread 的介面總覽（synopsis）+ 可執行的對照示範
//
// ⚠️ 本檔更正：原版是「只有宣告、沒有 #include 也沒有 main()」的介面清單,
//    無法單獨編譯。錯誤原因是 stop_source / stop_token / id /
//    native_handle_type 都沒有限定範圍。已修正為：
//      1. 補上標頭,並把型別寫成 std::stop_source / std::stop_token;
//      2. id 與 native_handle_type 其實是 jthread 的【成員型別】,
//         標準定義為 std::thread::id 與 std::thread::native_handle_type,
//         補上這兩個 using 之後 synopsis 才與標準一致;
//      3. 放進 namespace synopsis,避免與真正的 std::jthread 混淆;
//      4. 補上使用「真正 std::jthread」的 main(),讓本檔可直接編譯執行。
//
// 【主題資訊 Information】
//   標頭檔：<thread>（std::jthread）、<stop_token>（stop_source / stop_token）
//   標準版本：C++20（提案 P0660R10「Stop Token and Joining Thread」）
//
//   完整介面（見下方 namespace synopsis 的可編譯版本）：
//     建構    jthread() noexcept;
//             template<class F, class... Args> explicit jthread(F&&, Args&&...);
//     解構    ~jthread();                     // joinable 時 request_stop() + join()
//     移動    jthread(jthread&&) noexcept;    // 可移動、不可複製
//     停止    get_stop_source() / get_stop_token() / request_stop()
//     相容    joinable() / join() / detach() / get_id() / native_handle()
//     靜態    static unsigned hardware_concurrency() noexcept;
//
// 【詳細解釋 Explanation】
//
// 【1. 什麼是 synopsis，為什麼標準用這種寫法】
//   synopsis（介面總覽）是 ISO C++ 標準本身描述型別的格式：
//   只列出宣告、不含實作，讓讀者一眼看完「這個型別能做什麼」。
//   本檔刻意把它寫成【可編譯】的形式放進 namespace synopsis，
//   好處是編譯器會替你檢查介面寫得對不對 ——
//   純註解寫錯了沒人知道，但寫成程式碼就會被編譯器抓出來。
//
//   注意這些成員函式【只有宣告、沒有定義】。這在 C++ 完全合法：
//   只要從未被呼叫（也未取址），連結器就不需要它們的定義，不會產生連結錯誤。
//   這正是「介面與實作分離」在語言層面的直接體現。
//
// 【2. jthread 與 std::thread 的介面差異】
//   相同的部分：joinable/join/detach/get_id/native_handle/hardware_concurrency
//   —— 這是刻意的，讓 std::thread 的程式碼幾乎可以逐字換成 jthread。
//
//   新增的部分（三個停止相關成員）：
//     get_stop_source() → 控制端把手，可發出停止請求
//     get_stop_token()  → 工作端把手，唯讀
//     request_stop()    → 便捷方法，等同 get_stop_source().request_stop()
//
//   語意改變的部分（只有一個，但影響巨大）：
//     ~thread()  → joinable 時呼叫 std::terminate()
//     ~jthread() → joinable 時呼叫 request_stop() 然後 join()
//
// 【3. 成員型別 id 與 native_handle_type】
//   標準規定 jthread::id 就是 std::thread::id、
//   jthread::native_handle_type 就是 std::thread::native_handle_type。
//   這代表兩者的執行緒識別碼可以互相比較、放進同一個 map 當 key。
//   native_handle() 在 Linux/libstdc++ 上回傳的是 pthread_t，
//   拿到之後就能呼叫 pthread_setaffinity_np() 之類的平台專屬 API ——
//   這是【實作定義】的行為，寫了就不可攜。
//
// 【4. hardware_concurrency() 的真正語意】
//   它回傳的是「硬體支援的並行執行緒數的【提示】」，不是保證。
//   標準明文允許在資訊不可得時回傳 0，所以正確用法一定要處理 0：
//       unsigned n = std::jthread::hardware_concurrency();
//       if (n == 0) n = 4;   // 合理的後備值
//   還有一個實務陷阱：它回報的是【整台機器】的邏輯核心數，
//   在容器（Docker/K8s）中【不會】反映 cgroup 的 CPU 限制。
//   在只分到 2 核的容器裡，它照樣回報宿主機的 64。
//   要正確取得配額必須自己讀 /sys/fs/cgroup/cpu.max（cgroup v2）。
//
// 【概念補充 Concept Deep Dive】
//   * 為什麼建構子是 explicit：避免 std::jthread jt = someLambda; 這種
//     隱式轉換寫法。建立一條 OS 執行緒是重量級副作用，
//     標準要求你明確寫出建構語法，不讓它偷偷發生。
//
//   * 為什麼沒有 const 版本的 get_stop_source()：取得可寫的控制端等於
//     交出了修改停止狀態的能力，標準因此不標 const；
//     而 get_stop_token() 取得的是唯讀觀察端，所以是 const 成員。
//
//   * jthread 沒有 swap 成員嗎？有的。標準提供成員 swap 與
//     非成員 std::swap 特化，本 synopsis 為聚焦核心概念而省略。
//     真正完整的宣告請以 cppreference 或標準文本為準。
//
//   * 移動後的狀態：被移走的 jthread 變成 non-joinable（等同預設建構），
//     解構時什麼都不做。這與 std::thread 一致。
//
// 【注意事項 Pay Attention】
//   1. namespace synopsis 裡的 class 只有宣告、【沒有定義】。
//      不要嘗試建立它的物件或呼叫它的成員 —— 會產生連結錯誤。
//      它存在的唯一目的是「用編譯器驗證介面形狀」。
//   2. hardware_concurrency() 可能回傳 0，務必處理；
//      且在容器中不反映 cgroup 限制。
//   3. native_handle() 的回傳型別與可用操作都是【實作定義】，
//      使用它就等於放棄可攜性。
//   4. 本檔輸出中「工作中...」的行數取決於排程，每次執行都可能不同。
//   5. 本檔必須用 -std=c++20（已用 -pedantic-errors 實測驗證）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】jthread 介面與 hardware_concurrency
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::jthread 相對於 std::thread，介面上多了什麼、改了什麼？
//     答：多了三個停止相關成員（get_stop_source / get_stop_token /
//         request_stop）。改的只有【解構子語意】：thread 在 joinable 時
//         呼叫 std::terminate()，jthread 則是 request_stop() 後 join()。
//         其餘介面（joinable/join/detach/get_id/native_handle）完全相同，
//         這是刻意設計，方便舊程式碼平移。
//     追問：既然 jthread 全面較優，為什麼不直接改掉 std::thread 的解構子？
//         → 會悄悄改變既有程式的行為（原本立刻 terminate 的地方變成默默
//           阻塞 join），屬於重大 ABI/語意破壞。加新型別是唯一可行路徑。
//
// 🔥 Q2. hardware_concurrency() 回傳 0 代表什麼？該怎麼處理？
//     答：代表「無法取得這項資訊」。標準明文允許回 0，它只是一個【提示】
//         而非保證。正確寫法一定要有後備值：
//             unsigned n = std::jthread::hardware_concurrency();
//             if (n == 0) n = 4;
//     追問：在 Docker 容器裡它準嗎？
//         → 不準。它回報宿主機的邏輯核心數，【不反映】cgroup 的 CPU 配額。
//           只分到 2 核的容器照樣回報 64，據此開 64 條執行緒會嚴重過度訂閱。
//           要正確取得配額得自己讀 /sys/fs/cgroup/cpu.max。
//
// ⚠️ 陷阱. 「namespace synopsis 裡的 jthread 只有宣告沒有定義，
//         這樣不是會連結錯誤嗎？」
//     答：不會。C++ 只在「函式真的被使用（odr-used）」時才要求定義。
//         本檔從未建立 synopsis::jthread 的物件、也未呼叫其成員，
//         編譯器只需要宣告就能通過語意檢查，連結器根本不會去找定義。
//     為什麼會錯：把「宣告了就必須有定義」當成通則。實際上 C++ 的
//         one-definition rule 是需求驅動的 —— 沒用到就不需要。
//         這也是標頭檔可以只放宣告、以及 pimpl / 前向宣告能運作的基礎。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔從缺
//   本檔是介面總覽（synopsis），目的是完整呈現型別的 API 形狀，
//   本身不解任何演算法問題。LeetCode 並行題需要的是 mutex /
//   condition_variable / semaphore 的協調邏輯，與「列出介面」無關。
//   實際運用 jthread 解協調問題的範例安排在本課第 7 部分（工作執行緒池）。

#include <chrono>
#include <iostream>
#include <stop_token>
#include <thread>
#include <type_traits>
#include <vector>

namespace synopsis {

// 以下【只有宣告、沒有定義】,純粹用來展示介面形狀。
// 因為從未被呼叫,所以不會產生連結錯誤。
class jthread {
public:
    // 成員型別（標準規定）
    using id = std::thread::id;
    using native_handle_type = std::thread::native_handle_type;

    // 建構
    jthread() noexcept;

    template<typename F, typename... Args>
    explicit jthread(F&& f, Args&&... args);

    // 解構：自動 request_stop() + join()
    ~jthread();

    // 移動（不可複製）
    jthread(jthread&&) noexcept;
    jthread& operator=(jthread&&) noexcept;

    // 停止機制
    std::stop_source get_stop_source() noexcept;
    std::stop_token get_stop_token() const noexcept;
    bool request_stop() noexcept;

    // 與 std::thread 相同的介面
    bool joinable() const noexcept;
    void join();
    void detach();
    id get_id() const noexcept;
    native_handle_type native_handle();

    static unsigned int hardware_concurrency() noexcept;
};

}  // namespace synopsis

// ---- 以下用「真正的 std::jthread」示範上面那些介面 ----

// -----------------------------------------------------------------------------
// 【對照示範】成員型別確實與 std::thread 共用
// -----------------------------------------------------------------------------
void demoMemberTypes() {
    static_assert(std::is_same_v<std::jthread::id, std::thread::id>,
                  "jthread::id 應等同 thread::id");
    static_assert(std::is_same_v<std::jthread::native_handle_type,
                                 std::thread::native_handle_type>,
                  "native_handle_type 應等同 thread 的版本");
    std::cout << "  static_assert 通過：jthread::id 與 native_handle_type "
                 "確實等同 std::thread 的版本" << std::endl;

    std::cout << "  jthread 可複製嗎? " << std::boolalpha
              << std::is_copy_constructible_v<std::jthread> << std::endl;
    std::cout << "  jthread 可移動嗎? " << std::boolalpha
              << std::is_move_constructible_v<std::jthread> << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】依硬體核心數決定工作執行緒數（含 0 的後備處理）
//   情境：影像處理服務啟動時要決定開幾條工作執行緒。開太少浪費 CPU、
//         開太多造成 context switch 抖動與記憶體壓力。
//   為何要小心：hardware_concurrency() 可能回 0（標準允許），
//         且在容器中【不反映】cgroup 配額。實務上一定要有後備值與上限夾制，
//         並且允許用環境變數覆寫 —— 這是所有正式服務的標準做法。
// -----------------------------------------------------------------------------
unsigned decideWorkerCount(unsigned hint, unsigned fallback, unsigned cap) {
    unsigned n = (hint == 0) ? fallback : hint;   // 標準允許回 0，必須處理
    if (n > cap) n = cap;                          // 避免過度訂閱
    return n;
}

void demoWorkerSizing() {
    unsigned hw = std::jthread::hardware_concurrency();
    std::cout << "  hardware_concurrency() = " << hw
              << "（0 代表資訊不可得；容器中不反映 cgroup 配額）" << std::endl;

    unsigned workers = decideWorkerCount(hw, 4, 8);
    std::cout << "  實際採用的 worker 數 = " << workers
              << "（後備值 4、上限 8）" << std::endl;

    // 驗證後備邏輯：模擬回傳 0 的平台
    std::cout << "  若平台回報 0 → 採用 " << decideWorkerCount(0, 4, 8)
              << "（走後備值）" << std::endl;
    std::cout << "  若平台回報 64 → 採用 " << decideWorkerCount(64, 4, 8)
              << "（被上限夾制）" << std::endl;
}

int main() {
    std::cout << "=== hardware_concurrency ===" << std::endl;
    std::cout << "hardware_concurrency = "
              << std::jthread::hardware_concurrency() << std::endl;

    std::cout << "\n=== synopsis 介面對照（成員型別 / 複製移動性質） ===" << std::endl;
    demoMemberTypes();

    std::cout << "\n=== 基本示範：真正的 std::jthread ===" << std::endl;
    {
        std::jthread jt([](std::stop_token st) {
            // jthread 會把 stop_token 當作第一個參數傳進來
            while (!st.stop_requested()) {
                std::cout << "工作中..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            std::cout << "收到停止請求,正常結束" << std::endl;
        });

        std::cout << "joinable = " << std::boolalpha << jt.joinable() << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(350));

        // 解構時會自動做這兩件事,這裡只是明確展示
        std::cout << "main: 發出停止請求" << std::endl;
        jt.request_stop();
    }  // jt 解構：自動 request_stop() + join()

    std::cout << "\n=== 日常實務：決定工作執行緒數 ===" << std::endl;
    demoWorkerSizing();

    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra -pthread "課程 3.3：std jthread (C++20)6.cpp" -o jthread6
//   注意：本檔【必須】用 -std=c++20（std::jthread / std::stop_token 為 C++20 新增，
//   已用 -std=c++17 -pedantic-errors 實測確認編譯失敗）。

// 註:
//   ⚠️ hardware_concurrency 的值依機器而異（本機為 16 核）；
//   「工作中...」的行數取決於排程，【每次執行都不同】。

// === 預期輸出 ===
// === hardware_concurrency ===
// hardware_concurrency = 16
//
// === synopsis 介面對照（成員型別 / 複製移動性質） ===
//   static_assert 通過：jthread::id 與 native_handle_type 確實等同 std::thread 的版本
//   jthread 可複製嗎? false
//   jthread 可移動嗎? true
//
// === 基本示範：真正的 std::jthread ===
// joinable = true
// 工作中...
// 工作中...
// 工作中...
// 工作中...
// main: 發出停止請求
// 收到停止請求,正常結束
//
// === 日常實務：決定工作執行緒數 ===
//   hardware_concurrency() = 16（0 代表資訊不可得；容器中不反映 cgroup 配額）
//   實際採用的 worker 數 = 8（後備值 4、上限 8）
//   若平台回報 0 → 採用 4（走後備值）
//   若平台回報 64 → 採用 8（被上限夾制）
