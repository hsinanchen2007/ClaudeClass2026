// =============================================================================
//  第 23 課：deque 的雙端操作 2  —  push_back vs emplace_back：省下了什麼
// =============================================================================
//
// 【主題資訊 Information】
//   void push_back(const T& value);      // 複製建構一個元素
//   void push_back(T&& value);           // 移動建構一個元素（C++11）
//   template <class... Args>
//   reference emplace_back(Args&&... args);   // 就地建構（C++11；C++17 起回傳 reference）
//
//   標頭檔：<vector>、<deque>、<string>
//   複雜度：兩者皆為攤銷 O(1)；差別在「每次操作內部做了幾次建構」
//   本檔標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 兩者到底差在哪一步】
//   vec.push_back(string("Hello"))：
//       ① 先用 "Hello" 建構一個**臨時 string 物件**（一次建構 + 可能一次 heap 配置）
//       ② 把臨時物件**移動**進容器（一次移動建構）
//       ③ 臨時物件解構
//     → 總共 1 次建構 + 1 次移動 + 1 次解構
//
//   vec.emplace_back("Hello")：
//       ① 把 const char* 直接轉發給 string 的建構子，
//          **在容器的記憶體上就地建構**
//     → 總共 1 次建構，沒有臨時物件、沒有移動、沒有多餘的解構
//
//   省下的就是那一次移動與一次解構。對 std::string 而言，移動雖然便宜
//   （只搬指標），但**不是免費**；累積 100 萬次就看得出來。
//
// 【2. 為什麼「省一次移動」有時很有感、有時完全沒感】
//   關鍵在元素型別：
//     * 移動很貴（例如某些自訂型別沒有移動建構子 → 退回深複製）→ 差距巨大
//     * 移動很便宜（std::string 移動只搬 3 個欄位）→ 差距小但存在
//     * 平凡型別（int、double）→ 移動就是複製一個暫存器值，差距幾乎為零
//   本檔用「建構／移動次數」的計數證明差異的**本質**，
//   而不是只給一個隨機器浮動的耗時數字。
//
// 【3. emplace_back 不是永遠比較好】
//   當你手上**已經有一個 T 物件**時：
//       v.push_back(existing);              // 一次複製
//       v.emplace_back(existing);           // 一樣是一次複製（轉發給複製建構子）
//   兩者完全相同，此時 push_back 語意更清楚。
//   而且 emplace_back 會啟用 explicit 建構子、可能選到你沒預期的重載
//   （見第 20 課 5.cpp 的 vv.emplace_back(5, 0) 陷阱）。
//
// 【4. 對 deque 一樣成立】
//   本檔的原始程式碼用 vector 做示範，但 deque 也有 emplace_back /
//   emplace_front，語意完全相同——就地建構、省掉臨時物件。
//   deque 甚至更常受惠：它不會像 vector 那樣重新配置搬移全部元素，
//   所以「每次插入的固定成本」佔比更高，省下的那次移動相對更明顯。
//
// 【概念補充 Concept Deep Dive】
//   ● 為什麼原版程式的 stdout 不可重現
//     原始碼把耗時（ms）直接印到 stdout。耗時受 CPU 頻率調節、快取狀態、
//     其他行程干擾影響，每次執行都不同，不能當作教材的「預期輸出」。
//     本檔的處理方式：
//       耗時 → std::cerr（保留，供你臨場觀察）
//       建構／移動次數 → std::cout（可重現，當作教材證據）
//
//   ● 完美轉發（perfect forwarding）
//     emplace_back 的簽名是 template <class... Args> emplace_back(Args&&... args)。
//     Args&& 在模板推導下是「轉發參考」（forwarding reference），
//     搭配 std::forward<Args>(args)... 就能保留每個參數的值類別
//     （左值還是右值、const 還是非 const），原封不動交給 T 的建構子。
//     這是 C++11 讓「就地建構」成為可能的語言機制。
//
//   ● SSO 讓短字串的差距變小
//     libstdc++ 的 std::string 對長度 ≤ 15 位元組的字串使用 SSO，
//     不配置 heap。此時「臨時物件」的成本只是複製 32 位元組的本體，
//     差距會比長字串小很多。本檔的測試字串長度 36，超過門檻，
//     所以每個臨時物件都真的配置了一次 heap。
//
// 【注意事項 Pay Attention】
//   1. emplace_back 只有在「參數是建構原料」時才省得到；
//      傳入現成的 T 物件時兩者完全相同。
//   2. emplace_back 會啟用 explicit 建構子，可能讓本該編譯失敗的程式通過。
//   3. 耗時測量必須註明最佳化等級；-O0 與 -O2 的結論可能完全不同。
//   4. SSO 門檻 15 是本機 libstdc++ 的實作值，非標準保證。
//   5. 本檔 stderr 的耗時每次執行都不同，屬正常現象。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】push_back vs emplace_back
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. v.push_back(std::string("hi")) 和 v.emplace_back("hi")
//        內部各做了哪些事？
//     答：push_back 版：先建構一個臨時 string → 移動進容器 → 臨時物件解構，
//         共 1 建構 + 1 移動 + 1 解構。
//         emplace_back 版：把 const char* 直接轉發給 string 的建構子，
//         **在容器記憶體上就地建構**，只有 1 次建構。
//         省下的是那一次移動與一次解構。
//     追問：對 vector<int> 也有差嗎？→ 幾乎沒有。int 的「移動」就是複製一個
//         暫存器值，省下的成本趨近於零，此時選哪個純看可讀性。
//
// 🔥 Q2. emplace_back 靠什麼機制把參數原封不動交給建構子？
//     答：可變參數模板 + 轉發參考 + std::forward。
//         簽名是 template <class... Args> emplace_back(Args&&... args)，
//         Args&& 在模板推導下是轉發參考，內部再用 std::forward<Args>(args)...
//         保留每個參數的值類別（左值/右值、const/非 const），
//         這樣才能選到 T 「正確的那個」建構子。
//     追問：如果不用 std::forward 會怎樣？→ args 在函式內部都是具名的左值，
//         全部會以左值傳遞，右值參數就會退化成複製而非移動。
//
// ⚠️ 陷阱. 「emplace_back 比較快，所以我把程式裡所有 push_back 都改成 emplace_back」
//     答：對「已經有現成物件」的呼叫沒有任何效果（兩者都是一次複製／移動），
//         但你付出了三個代價：語意變模糊、explicit 保護被繞過、
//         以及可能選到非預期的建構子重載（例如
//         vector<vector<int>>::emplace_back(5, 0) 會建出「5 個 0」而不是 {5,0}）。
//     為什麼會錯：把「A 在某些情況下比 B 快」誤讀成「A 永遠該取代 B」。
//         正確的判準是**參數的語意**：傳「建構原料」用 emplace_back，
//         傳「一個完整的 T」用 push_back。效能只是這個判準的副產品。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <deque>
#include <chrono>
#include <string>
using namespace std;
using namespace chrono;

// -----------------------------------------------------------------------------
// 儀器型別：記錄「從 const char* 建構」「複製建構」「移動建構」各幾次。
// 用計數當證據，結果每次執行完全相同，可驗證、可與理論值對照。
// -----------------------------------------------------------------------------
struct Probe {
    static long ctorFromRaw;   // 從原料（const char*）建構
    static long copies;
    static long moves;
    static long dtors;
    static void reset() { ctorFromRaw = copies = moves = dtors = 0; }

    string s;

    explicit Probe(const char* raw) : s(raw) { ++ctorFromRaw; }
    Probe(const Probe& o) : s(o.s) { ++copies; }
    Probe(Probe&& o) noexcept : s(std::move(o.s)) { ++moves; }
    Probe& operator=(const Probe& o) { s = o.s; ++copies; return *this; }
    Probe& operator=(Probe&& o) noexcept { s = std::move(o.s); ++moves; return *this; }
    ~Probe() { ++dtors; }
};
long Probe::ctorFromRaw = 0;
long Probe::copies = 0;
long Probe::moves = 0;
long Probe::dtors = 0;

// -----------------------------------------------------------------------------
// 【日常實務範例】批次建立資料庫連線描述物件
//   情境：服務啟動時要依設定建立一批連線描述（host、port、逾時秒數）。
//         這些物件是「用幾個參數組出來的」，正是 emplace_back 的主場：
//         直接把參數交給建構子，不需要先在外面組出一個臨時物件再搬進去。
// -----------------------------------------------------------------------------
struct ConnSpec {
    string host;
    int port;
    int timeoutSec;

    ConnSpec(string h, int p, int t)
        : host(std::move(h)), port(p), timeoutSec(t) {}
};

deque<ConnSpec> buildConnectionPool() {
    deque<ConnSpec> pool;
    // 參數就是「建構原料」→ emplace_back 恰如其分，零臨時物件
    pool.emplace_back("db-primary.internal", 5432, 30);
    pool.emplace_back("db-replica-1.internal", 5432, 15);
    pool.emplace_back("db-replica-2.internal", 5432, 15);
    pool.emplace_back("cache.internal", 6379, 5);
    return pool;
}

// 註：本檔不附 LeetCode 範例。push_back 與 emplace_back 的差異是
//     「物件建構成本」議題，LeetCode 判題只看回傳值，量不到建構次數，
//     也沒有對應題型；硬掛一題只會模糊本檔重點。

int main() {
    cout << "=== 1. 可重現證據：各自做了幾次建構 / 移動 / 解構 ===" << endl;
    const int SMALL = 5;

    Probe::reset();
    {
        vector<Probe> v;
        v.reserve(SMALL);
        for (int i = 0; i < SMALL; i++) {
            v.push_back(Probe("Hello, World! This is a test."));   // 先建臨時物件
        }
        cout << "push_back(臨時物件) x" << SMALL << "：" << endl;
        cout << "  從原料建構 " << Probe::ctorFromRaw
             << " 次、複製 " << Probe::copies
             << " 次、移動 " << Probe::moves
             << " 次、解構 " << Probe::dtors << " 次" << endl;
    }

    Probe::reset();
    {
        vector<Probe> v;
        v.reserve(SMALL);
        for (int i = 0; i < SMALL; i++) {
            v.emplace_back("Hello, World! This is a test.");       // 就地建構
        }
        cout << "emplace_back x" << SMALL << "：" << endl;
        cout << "  從原料建構 " << Probe::ctorFromRaw
             << " 次、複製 " << Probe::copies
             << " 次、移動 " << Probe::moves
             << " 次、解構 " << Probe::dtors << " 次" << endl;
    }
    cout << "→ push_back 版每次多一次移動建構與一次臨時物件解構；" << endl;
    cout << "  emplace_back 版完全沒有臨時物件。" << endl;
    cout << "（這組數字每次執行完全相同，不像耗時會浮動）" << endl;

    cout << "\n=== 2. 已有現成物件時，兩者完全相同 ===" << endl;
    Probe::reset();
    {
        Probe existing("already constructed");
        vector<Probe> v;
        v.reserve(4);
        v.push_back(existing);        // 複製
        v.emplace_back(existing);     // 一樣是複製（轉發給複製建構子）
        cout << "傳入現成物件：push_back 與 emplace_back 各一次 → 複製共 "
             << Probe::copies << " 次、移動 " << Probe::moves << " 次" << endl;
        cout << "→ 兩者行為完全一致，此時 push_back 語意更清楚" << endl;
    }

    cout << "\n=== 3. 大量測試的實際耗時（結果印在 stderr）===" << endl;
    const int N = 1000000;
    const char* TEXT = "Hello, World! This is a test string.";
    cout << "測試字串長度 " << string(TEXT).size()
         << "（超過本機 libstdc++ 的 SSO 門檻 15，每個臨時物件都要配置 heap）" << endl;

    size_t sizeA = 0, sizeB = 0;

    // 測試 push_back（臨時物件）
    {
        vector<string> vec;
        vec.reserve(N);
        auto start = high_resolution_clock::now();

        for (int i = 0; i < N; i++) {
            vec.push_back(string(TEXT));
        }

        auto end = high_resolution_clock::now();
        auto ms = duration_cast<milliseconds>(end - start).count();
        cerr << "  [計時] push_back(臨時物件): " << ms << " ms" << endl;
        sizeA = vec.size();
    }

    // 測試 emplace_back
    {
        vector<string> vec;
        vec.reserve(N);
        auto start = high_resolution_clock::now();

        for (int i = 0; i < N; i++) {
            vec.emplace_back(TEXT);
        }

        auto end = high_resolution_clock::now();
        auto ms = duration_cast<milliseconds>(end - start).count();
        cerr << "  [計時] emplace_back:       " << ms << " ms" << endl;
        sizeB = vec.size();
    }

    cout << "兩者都放入 " << sizeA << " 個元素（結果一致："
         << boolalpha << (sizeA == sizeB) << "）" << endl;
    cout << "耗時已印到 stderr——每次執行都不同，故不列入預期輸出。" << endl;
    cout << "⚠️ 且此處差距不大：std::string 的移動很便宜（只搬幾個欄位），" << endl;
    cout << "   省下的一次移動在 -O2 下常被雜訊掩蓋。真正的證據請看第 1 節的計數。" << endl;
    cout << "只看 stdout 請用：./demo23_2 2>/dev/null" << endl;

    cout << "\n=== 4. deque 也有 emplace_front / emplace_back ===" << endl;
    deque<string> dq;
    dq.emplace_back("尾端就地建構");
    dq.emplace_front("頭端就地建構");
    cout << "deque 內容：";
    for (const auto& s : dq) cout << "[" << s << "] ";
    cout << endl;
    cout << "→ deque 兩端都支援就地建構，語意與 vector 的 emplace_back 相同" << endl;

    cout << "\n=== 日常實務：批次建立資料庫連線描述 ===" << endl;
    auto pool = buildConnectionPool();
    cout << "連線池共 " << pool.size() << " 筆：" << endl;
    for (const auto& c : pool) {
        cout << "  " << c.host << ":" << c.port
             << "（逾時 " << c.timeoutSec << " 秒）" << endl;
    }
    cout << "→ 三個參數就是 ConnSpec 的建構原料，emplace_back 直接轉發，" << endl;
    cout << "  完全不需要先在外面組出一個臨時 ConnSpec 再搬進容器。" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 23 課：deque 的雙端操作：push_front、push_back、pop_front、pop_back2.cpp" -o demo23_2
// 只看 stdout: ./demo23_2 2>/dev/null
//
// ⚠️ 但書：
//   1. 第 3 節的耗時印在 stderr，每次執行都不同（受 CPU 頻率、快取、
//      其他行程影響），故不納入下方預期輸出。
//   2. std::string 的移動成本很低，push_back 與 emplace_back 的耗時差距
//      在 -O2 下常被雜訊掩蓋；本檔以第 1 節的「建構／移動次數」為主要證據。
//   3. SSO 門檻 15 是本機 g++ 15.2 / libstdc++ 的實作值，非標準保證。

// === 預期輸出 ===
// === 1. 可重現證據：各自做了幾次建構 / 移動 / 解構 ===
// push_back(臨時物件) x5：
//   從原料建構 5 次、複製 0 次、移動 5 次、解構 5 次
// emplace_back x5：
//   從原料建構 5 次、複製 0 次、移動 0 次、解構 0 次
// → push_back 版每次多一次移動建構與一次臨時物件解構；
//   emplace_back 版完全沒有臨時物件。
// （這組數字每次執行完全相同，不像耗時會浮動）
//
// === 2. 已有現成物件時，兩者完全相同 ===
// 傳入現成物件：push_back 與 emplace_back 各一次 → 複製共 2 次、移動 0 次
// → 兩者行為完全一致，此時 push_back 語意更清楚
//
// === 3. 大量測試的實際耗時（結果印在 stderr）===
// 測試字串長度 36（超過本機 libstdc++ 的 SSO 門檻 15，每個臨時物件都要配置 heap）
// 兩者都放入 1000000 個元素（結果一致：true）
// 耗時已印到 stderr——每次執行都不同，故不列入預期輸出。
// ⚠️ 且此處差距不大：std::string 的移動很便宜（只搬幾個欄位），
//    省下的一次移動在 -O2 下常被雜訊掩蓋。真正的證據請看第 1 節的計數。
// 只看 stdout 請用：./demo23_2 2>/dev/null
//
// === 4. deque 也有 emplace_front / emplace_back ===
// deque 內容：[頭端就地建構] [尾端就地建構]
// → deque 兩端都支援就地建構，語意與 vector 的 emplace_back 相同
//
// === 日常實務：批次建立資料庫連線描述 ===
// 連線池共 4 筆：
//   db-primary.internal:5432（逾時 30 秒）
//   db-replica-1.internal:5432（逾時 15 秒）
//   db-replica-2.internal:5432（逾時 15 秒）
//   cache.internal:6379（逾時 5 秒）
// → 三個參數就是 ConnSpec 的建構原料，emplace_back 直接轉發，
//   完全不需要先在外面組出一個臨時 ConnSpec 再搬進容器。
