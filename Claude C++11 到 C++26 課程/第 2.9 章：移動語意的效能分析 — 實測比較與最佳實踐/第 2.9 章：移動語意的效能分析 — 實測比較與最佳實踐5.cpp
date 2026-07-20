// =============================================================================
//  第 2.9 章 5  —  push_back(複製) vs push_back(移動) vs emplace_back
// =============================================================================
//
// 【主題資訊 Information】
//   void push_back(const T& value);   // 複製版本      [C++98]
//   void push_back(T&& value);        // 移動版本      [C++11]
//   template<class... Args>
//   reference emplace_back(Args&&... args);            [C++11；C++17 起回傳參考]
//   標頭檔  : <vector>, <string>
//   複雜度  : 三者皆為攤銷 O(1)（本檔已 reserve，故無擴容干擾）
//   實測    : libstdc++ 的 SSO 門檻為 15 字元；"Hello, World!!!!" 為 16 字元，
//             剛好超過門檻而必須配置堆積——這是本範例能呈現差異的前提。
//
// 【詳細解釋 Explanation】
//
// 【1. 三種寫法到底差在哪】
//   push_back(s)             s 是左值 → 選中 push_back(const T&) → 複製建構一份進容器
//   push_back(std::move(s))  轉成右值 → 選中 push_back(T&&)      → 移動建構進容器
//   emplace_back(args...)    完全不建立中間物件，直接用 args 在容器的記憶體上
//                            就地建構（placement new + 完美轉發）
//
//   關鍵在於 emplace_back 少了一整個「中間物件」的生命週期：
//       push_back 版：建 s（配置） → 搬進容器（複製再配置一次／移動零配置）→ 銷毀 s
//       emplace 版  ：直接在容器裡用 const char* 建構（配置一次）
//
// 【2. 為什麼要用 16 個字元的字串】
//   libstdc++ 的 SSO 門檻是 15 個字元：15 字以內直接存在 string 物件內部，
//   完全不碰堆積。若示範用 "Hi"，三種寫法的配置次數都會是 0，
//   什麼差異都看不出來。"Hello, World!!!!" 剛好 16 字元，越過門檻，
//   差異才顯現。這個門檻是實作定義的，換標準庫可能不同。
//
// 【3. 為什麼本檔不用牆鐘時間當證據】
//   原始寫法用 Timer 量三段的毫秒數，問題是：
//     (a) 耗時不可重現，受 CPU 頻率、快取、系統負載影響，每次都不同，
//         寫進「預期輸出」等於假資料；
//     (b) 三段的差距可能被記憶體配置器的行為、頁面錯誤等雜訊淹沒。
//   改用「堆積配置次數」就沒有這些問題：它是確定性的，而且直接對應
//   成本發生的機制——每一次多餘的配置就是一次 malloc 加一次字元拷貝。
//
// 【4. emplace_back 不是萬靈丹】
//   它只在「參數不是同型別物件」時才有優勢。若你已經有一個 std::string，
//   emplace_back(s) 和 push_back(s) 完全一樣（都是複製建構）。
//   另外 emplace_back 使用直接初始化，會繞過 explicit 的保護：
//       std::vector<std::unique_ptr<int>> v;
//       v.emplace_back(new int(1));   // 編得過，但中途丟例外就洩漏
//   所以「一律用 emplace_back」是過度簡化的建議。
//
// 【概念補充 Concept Deep Dive】
//   本檔刻意先 v.reserve(N)，目的是排除「擴容搬移」對計數的干擾——
//   否則量到的配置次數會混入 vector 自己成長時的配置與元素搬遷，
//   無法歸因到 push_back / emplace_back 的差異上。
//   這也呼應第 9 課的重點：量測時必須把不想量的變因先固定住。
//
// 【注意事項 Pay Attention】
// 1. SSO 門檻（15 字元）是實作定義。字串短於門檻時，三種寫法沒有差別。
// 2. emplace_back 用直接初始化，會繞過 explicit；傳裸指標給智慧指標容器
//    是已知的例外安全陷阱。
// 3. 移動之後區域變數 s 處於「有效但未指定」狀態，本檔不對其內容做斷言。
// 4. 本檔已 reserve，故不含擴容成本；未 reserve 時三者都會額外付出
//    重新配置與搬移的代價。
// 5. 量測程式必須阻止編譯器最佳化掉被測程式碼。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】push_back vs emplace_back
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. emplace_back 一定比 push_back 快嗎？
//     答：不一定。它的優勢來自「省掉中間物件」——當你傳的是建構參數
//         （例如 const char*）時，emplace_back 直接在容器記憶體上就地建構，
//         而 push_back 得先建一個 std::string 再搬進去。但如果你手上
//         已經有一個 std::string 左值，emplace_back(s) 與 push_back(s)
//         完全等價，都是複製建構。
//     追問：那為什麼不乾脆一律用 emplace_back？→ 它用直接初始化，
//         會繞過 explicit 的保護，例如
//         vector<unique_ptr<int>>::emplace_back(new int(1)) 編得過，
//         但中途丟例外就會洩漏。可讀性上 push_back 也更能表達「放一個現成的值」。
//
// 🔥 Q2. push_back(s) 和 push_back(std::move(s)) 差在哪？
//     答：多載決議的結果不同。前者 s 是左值，選中 push_back(const T&)，
//         走複製建構——對超過 SSO 門檻的字串就是一次堆積配置加字元拷貝。
//         後者轉成右值，選中 push_back(T&&)，走移動建構，零配置。
//     追問：如果 s 之後還要用呢？→ 那就不能 move。std::move 的語意是
//         「我不再需要它了」，之後再讀它的內容就是讀未指定的值。
//
// ⚠️ 陷阱. 用 "Hi" 這種短字串做上面的效能比較，量出來三種寫法幾乎一樣快，
//          於是結論「emplace_back 是迷思」。錯在哪？
//     答：錯在測資選錯了。"Hi" 只有 2 個字元，落在 SSO 範圍內，
//         整個字串直接存在物件內部、根本沒有堆積配置，
//         也就沒有任何資源需要搬移——三種寫法當然一樣。
//         必須用超過 SSO 門檻（libstdc++ 為 15 字元）的字串才測得出差異。
//     為什麼會錯：把 std::string 想成「一定會配置堆積」的型別，
//         忽略了 SSO 這個實作最佳化。這也提醒我們：任何效能結論都必須
//         先確認「你以為的成本」在測資上真的發生了。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】從缺。
//   理由：本檔主題是容器插入方式的成本差異與效能量測方法論。
//   LeetCode 判定演算法正確性，不會因為用 push_back 而非 emplace_back 失敗；
//   清單中也沒有任何一題以此為核心。硬套一題只會模糊焦點，故從缺。

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <utility>
#include <new>
#include <cstdlib>

// -----------------------------------------------------------------------------
// 堆積配置計數器：覆寫全域 operator new / delete（標準允許的替換函式）
// -----------------------------------------------------------------------------
static long g_alloc_count = 0;

void* operator new(std::size_t n) {
    ++g_alloc_count;
    void* p = std::malloc(n);
    if (!p) throw std::bad_alloc();
    return p;
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void* operator new[](std::size_t n) {
    ++g_alloc_count;
    void* p = std::malloc(n);
    if (!p) throw std::bad_alloc();
    return p;
}
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

template <typename T>
static inline void doNotOptimize(T& value) {
    asm volatile("" : "+m"(value) : : "memory");
}

// Timer 保留供參考，但輸出到 stderr——耗時每次執行都不同
class Timer {
    std::chrono::steady_clock::time_point start_;
    const char* label_;
public:
    explicit Timer(const char* label)
        : start_(std::chrono::steady_clock::now()), label_(label) {}
    ~Timer() {
        auto elapsed = std::chrono::steady_clock::now() - start_;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        std::cerr << "[stderr] " << label_ << ": " << ms << " ms（僅供參考）\n";
    }
};

int main() {
    const int N = 1000000;
    const char* TEXT = "Hello, World!!!!";   // 16 字元，剛好超過 SSO 門檻(15)

    std::cout << "=== push_back(複製) vs push_back(移動) vs emplace_back ===\n";
    std::cout << "字串 \"" << TEXT << "\" 長度 " << std::string(TEXT).size()
              << " 字元（libstdc++ SSO 門檻為 15，故必須配置堆積）\n";
    std::cout << "各執行 " << N << " 次，皆已 reserve 以排除擴容干擾\n\n";

    long a0;

    // ---- (1) push_back(左值) → 複製 ----
    a0 = g_alloc_count;
    {
        Timer t("push_back(複製)");
        std::vector<std::string> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) {
            std::string s = TEXT;      // 配置 #1：建立中間物件
            v.push_back(s);            // 配置 #2：複製建構進容器
        }
        doNotOptimize(v);
    }
    long copy_allocs = g_alloc_count - a0;

    // ---- (2) push_back(右值) → 移動 ----
    a0 = g_alloc_count;
    {
        Timer t("push_back(移動)");
        std::vector<std::string> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) {
            std::string s = TEXT;          // 配置 #1：建立中間物件
            v.push_back(std::move(s));     // 移動：0 次配置，只搬指標
        }
        doNotOptimize(v);
    }
    long move_allocs = g_alloc_count - a0;

    // ---- (3) emplace_back → 就地建構，連中間物件都沒有 ----
    a0 = g_alloc_count;
    {
        Timer t("emplace_back");
        std::vector<std::string> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) {
            v.emplace_back(TEXT);          // 配置 #1：直接在容器記憶體上建構
        }
        doNotOptimize(v);
    }
    long emplace_allocs = g_alloc_count - a0;

    std::cout << "push_back(複製) -> 堆積配置 " << copy_allocs
              << " 次（中間物件 1 次 + 複製進容器 1 次，每輪 2 次）\n";
    std::cout << "push_back(移動) -> 堆積配置 " << move_allocs
              << " 次（中間物件 1 次；移動本身 0 次）\n";
    std::cout << "emplace_back    -> 堆積配置 " << emplace_allocs
              << " 次（就地建構，連中間物件都不存在）\n";
    std::cout << "\n注意 reserve(N) 本身也各算一次配置，故三者都含這 1 次。\n";
    std::cout << "誠實解讀：移動版與 emplace 版的「配置次數相同」——\n";
    std::cout << "  複製版多出的 100 萬次配置，是因為它把字串內容真的又拷貝了一份；\n";
    std::cout << "  emplace 相對移動版省下的不是配置，而是那個中間 std::string\n";
    std::cout << "  物件的建構與解構（以及一次移動）。配置次數量不到這一項，\n";
    std::cout << "  這正是「單一指標只能證明它涵蓋的那件事」的實例。\n";

    // ---- 對照：短字串落在 SSO 範圍，三者毫無差別 ----
    std::cout << "\n=== 對照：改用 SSO 短字串 \"Hi\" ===\n";
    const int M = 100000;
    const char* SHORT = "Hi";

    a0 = g_alloc_count;
    { std::vector<std::string> v; v.reserve(M);
      for (int i = 0; i < M; ++i) { std::string s = SHORT; v.push_back(s); }
      doNotOptimize(v); }
    long s_copy = g_alloc_count - a0;

    a0 = g_alloc_count;
    { std::vector<std::string> v; v.reserve(M);
      for (int i = 0; i < M; ++i) { std::string s = SHORT; v.push_back(std::move(s)); }
      doNotOptimize(v); }
    long s_move = g_alloc_count - a0;

    a0 = g_alloc_count;
    { std::vector<std::string> v; v.reserve(M);
      for (int i = 0; i < M; ++i) { v.emplace_back(SHORT); }
      doNotOptimize(v); }
    long s_empl = g_alloc_count - a0;

    std::cout << "push_back(複製) -> " << s_copy << " 次\n";
    std::cout << "push_back(移動) -> " << s_move << " 次\n";
    std::cout << "emplace_back    -> " << s_empl << " 次\n";
    std::cout << "三者都只有 reserve 那 1 次：SSO 下字串沒碰堆積，\n";
    std::cout << "所以用短字串做這個比較，永遠測不出差異。\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 2.9 章：移動語意的效能分析 — 實測比較與最佳實踐5.cpp -o pushback_vs_emplace

// 【但書】
//   1. 耗時（Timer）輸出到 stderr，不列入下方預期輸出——每次執行都不同。
//      stdout 只放確定性的配置次數，本機連跑 5 次逐位元組相同。
//   2. SSO 門檻 15 字元是 libstdc++ 的實作定義值（本機 g++ 15.2）。
//      換標準庫時門檻不同，"Hello, World!!!!" 可能就落在 SSO 範圍內，
//      三種寫法的差異會整個消失。
//   3. 「2000001 / 1000001」中的 +1 來自 reserve(N) 自己那一次配置。

// === 預期輸出 ===
// === push_back(複製) vs push_back(移動) vs emplace_back ===
// 字串 "Hello, World!!!!" 長度 16 字元（libstdc++ SSO 門檻為 15，故必須配置堆積）
// 各執行 1000000 次，皆已 reserve 以排除擴容干擾
//
// push_back(複製) -> 堆積配置 2000001 次（中間物件 1 次 + 複製進容器 1 次，每輪 2 次）
// push_back(移動) -> 堆積配置 1000001 次（中間物件 1 次；移動本身 0 次）
// emplace_back    -> 堆積配置 1000001 次（就地建構，連中間物件都不存在）
//
// 注意 reserve(N) 本身也各算一次配置，故三者都含這 1 次。
// 誠實解讀：移動版與 emplace 版的「配置次數相同」——
//   複製版多出的 100 萬次配置，是因為它把字串內容真的又拷貝了一份；
//   emplace 相對移動版省下的不是配置，而是那個中間 std::string
//   物件的建構與解構（以及一次移動）。配置次數量不到這一項，
//   這正是「單一指標只能證明它涵蓋的那件事」的實例。
//
// === 對照：改用 SSO 短字串 "Hi" ===
// push_back(複製) -> 1 次
// push_back(移動) -> 1 次
// emplace_back    -> 1 次
// 三者都只有 reserve 那 1 次：SSO 下字串沒碰堆積，
// 所以用短字串做這個比較，永遠測不出差異。
