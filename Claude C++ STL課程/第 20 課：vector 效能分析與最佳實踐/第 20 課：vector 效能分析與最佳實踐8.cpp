// =============================================================================
//  第 20 課：vector 效能分析與最佳實踐 8  —  range-for 的複製陷阱
// =============================================================================
//
// 【主題資訊 Information】
//   for (T x : container)          // 每次迭代「複製」一個元素
//   for (const T& x : container)   // 唯讀，零複製            ← 唯讀時的標準寫法
//   for (T& x : container)         // 可修改容器內的元素
//   for (auto&& x : container)     // 泛型轉發，寫模板時最保險
//
//   標頭檔：<vector>、<string>
//   複雜度：複製版每次迭代多一次元素複製（對 std::string 可能含 heap 配置）
//   本檔標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. range-for 展開後長什麼樣】
//   for (T x : data) { ... }  在編譯器眼中大致等於：
//       auto&& __range = data;
//       auto __b = __range.begin();  auto __e = __range.end();
//       for (; __b != __e; ++__b) {
//           T x = *__b;              // ★ 關鍵在這一行
//           ...
//       }
//   當你寫 T x（值）時，那一行就是一次完整的複製建構；
//   寫 const T& x 時，它只是繫結一個參考，什麼都不做。
//   對 std::string 而言，「一次複製」可能包含一次 heap 配置 + memcpy + 之後的釋放。
//
// 【2. 為什麼 std::string 特別痛】
//   libstdc++ 的 std::string 有 SSO（Small String Optimization）：
//   長度 **≤ 15 位元組**的字串直接存在物件本體內（本機 sizeof(std::string) == 32），
//   不需要 heap 配置。超過 15 就必須配置 heap。
//   所以：
//       短字串（≤15）複製 → 只是複製 32 位元組，很快
//       長字串（>15）複製 → operator new + memcpy + 之後 operator delete
//   本檔的測試字串長度 44，遠超 SSO 門檻，所以每次迭代都是真的配置記憶體。
//   ⚠️ SSO 門檻 15 是 libstdc++ 的實作值，MSVC 的 STL 門檻是 15，
//      但 sizeof 與內部佈局不同——這是實作定義，不是標準規定。
//
// 【3. 什麼時候「該」用值】
//   * 你就是要一份可以隨意改的副本，且不想影響容器內容。
//   * 元素是 int / double / 指標這種小型別——此時值反而更好
//     （少一層間接定址，且沒有 aliasing 疑慮）。
//   對 int 寫 const int& 不會比較快，甚至可能略慢，屬於過度優化。
//
// 【4. auto&& 的用途】
//   寫泛型程式（模板）時，容器可能回傳 proxy 物件而不是真正的參考，
//   最典型的就是 std::vector<bool> ——它的 operator* 回傳的是
//   std::vector<bool>::reference 這個 proxy，不是 bool&。
//   此時 for (bool& b : vb) 會編譯失敗，而 for (auto&& b : vb) 可以。
//   所以模板程式碼一律建議寫 auto&&。
//
// 【概念補充 Concept Deep Dive】
//   ● 為什麼編譯器不幫你自動改成參考
//     因為值與參考的**語意不同**：值版本的 x 是獨立副本，
//     修改它不會影響容器；參考版本會。編譯器必須忠實保留你寫的語意。
//     只有在它能證明「完全觀察不到差異」時才可能省略，而只要元素型別的
//     複製建構子有任何副作用（std::string 的 operator new 就是副作用），
//     就不能省略。
//
//   ● 為什麼本檔用「配置次數」而不是「耗時」當證據
//     耗時受 CPU 頻率、快取、其他行程影響，每次執行都不同，
//     而且會隨最佳化等級劇烈變化，不適合寫進預期輸出。
//     本檔改用計數 allocator 數「operator new 被呼叫幾次」——
//     這是可重現、可驗證、且直接反映問題本質的證據。耗時則印到 stderr。
//
//   ● -Wreorder：本檔修正過的一個真實警告
//     原版 Timer 的成員宣告順序（start_、label_）與初始化列表的書寫順序
//     （label_、start_）不一致。C++ 規定成員一律依**宣告順序**初始化，
//     g++ -Wall 會以 -Wreorder 警告。本檔已把宣告順序調整為一致。
//
// 【注意事項 Pay Attention】
//   1. for (std::string s : data) 每次迭代都複製一次，編譯器**不會**警告。
//      這是最常見、也最容易被忽略的效能問題之一。
//   2. 唯讀就用 const auto&；要改元素用 auto&；寫模板用 auto&&。
//   3. 對 int / double / 指標等小型別，直接用值即可，加 const& 沒有好處。
//   4. for (auto x : data) 的 auto 會**推導成值型別**，同樣會複製——
//      很多人以為 auto 會自動變成參考，這是錯的。
//   5. SSO 門檻 15 是本機 libstdc++ 的實作值，不是標準保證。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】range-for 的元素型別選擇
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. for (std::string s : vec) 和 for (const std::string& s : vec)
//        差在哪？
//     答：前者每次迭代都複製建構一個 string；若字串超過 SSO 門檻
//         （本機 libstdc++ 是 15 位元組），每次複製都含一次 heap 配置與釋放。
//         後者只繫結參考，零成本。唯讀情境一律用 const&。
//     追問：那對 vector<int> 呢？→ 用值就好。int 只有 4 位元組，
//         直接放暫存器比多一層間接定址更快，寫 const int& 是過度優化。
//
// 🔥 Q2. for (auto x : vec) 會複製嗎？
//     答：會。auto 在這裡推導出的是**值型別**，等同 for (T x : vec)。
//         要避免複製必須明確寫 const auto& 或 auto&。
//         「用了 auto 就會自動最佳化」是常見的誤解。
//     追問：那 auto&& 什麼時候用？→ 寫模板時。有些容器（如 vector<bool>）
//         的迭代器解參考回傳的是 proxy 物件而非真參考，
//         auto& 會編譯失敗，auto&& 才能同時接住真參考與 proxy。
//
// ⚠️ 陷阱. 「我全部都寫 const auto&，這樣一定最快」——什麼時候反而錯？
//     答：兩種情況。(a) 小型別（int、指標）：多一層間接定址，
//         而且編譯器可能因為無法排除別名（aliasing）而放棄向量化。
//         (b) **迴圈內本來就需要一份可修改的副本**：寫 const auto& 之後
//         還是得在迴圈內複製一次，反而多繞一圈。
//     為什麼會錯：把「const& 避免複製」這條規則當成無條件的教條，
//         而不是「複製成本高時才需要避免」的手段。
//         真正的判準是「元素複製貴不貴」與「這個迴圈需不需要副本」。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <cstdlib>   // std::malloc / std::free（自訂 operator new 用）
#include <new>       // std::bad_alloc

// -----------------------------------------------------------------------------
// RAII 計時器。
// 【修正 1】成員宣告順序改成與初始化列表一致（原版觸發 -Wreorder 警告）。
// 【修正 2】結果印到 std::cerr——耗時每次執行都不同，不應混進 stdout。
// -----------------------------------------------------------------------------
struct Timer {
    std::string label_;                                     // 先宣告 → 先初始化
    std::chrono::high_resolution_clock::time_point start_;  // 後宣告 → 後初始化

    explicit Timer(const std::string& label)
        : label_(label),
          start_(std::chrono::high_resolution_clock::now()) {}

    ~Timer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
        std::cerr << "  [計時] " << label_ << ": " << us << " μs" << std::endl;
    }
};

// -----------------------------------------------------------------------------
// 可重現的證據：數 operator new / delete 被呼叫幾次。
// 這直接反映「複製字串時到底有沒有配置記憶體」，比耗時穩定得多。
// -----------------------------------------------------------------------------
struct HeapStats {
    static long news;
    static long deletes;
    static bool tracking;
    static void reset() { news = 0; deletes = 0; }
};
long HeapStats::news = 0;
long HeapStats::deletes = 0;
bool HeapStats::tracking = false;

void* operator new(std::size_t sz) {
    if (HeapStats::tracking) ++HeapStats::news;
    void* p = std::malloc(sz ? sz : 1);
    if (!p) throw std::bad_alloc();
    return p;
}
void operator delete(void* p) noexcept {
    if (HeapStats::tracking && p) ++HeapStats::deletes;
    std::free(p);
}
void operator delete(void* p, std::size_t) noexcept {
    if (HeapStats::tracking && p) ++HeapStats::deletes;
    std::free(p);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】掃描 access log，統計各狀態碼的位元組總量
//   情境：一份存在記憶體中的 access log（每行是一個字串），
//         要跑好幾種統計。這種「同一份大資料掃很多次」的程式，
//         迴圈變數寫成值就會把整份 log 反覆複製一遍，
//         而 profiler 上只會看到 operator new 很忙，不容易一眼看出根因。
// -----------------------------------------------------------------------------
struct LogSummary {
    std::size_t lines = 0;
    std::size_t totalBytes = 0;
    std::size_t errorLines = 0;
};

LogSummary summarize(const std::vector<std::string>& log) {
    LogSummary s;
    for (const std::string& line : log) {        // ✅ const& ：零複製
        ++s.lines;
        s.totalBytes += line.size();
        if (line.find(" 500 ") != std::string::npos ||
            line.find(" 503 ") != std::string::npos) {
            ++s.errorLines;
        }
    }
    return s;
}

// 註：本檔不附 LeetCode 範例。迴圈變數該用值還是參考，是效能與 API 品味議題，
//     LeetCode 只驗證答案正確與否，量不到這個差異，也不會針對它出題。

int main() {
    std::cout << "=== 1. 可重現證據：迴圈中發生幾次 heap 配置 ===\n";
    const std::size_t N = 20000;
    const std::string longStr = "this_is_a_reasonably_long_string_for_testing";  // 長度 44
    std::cout << "字串長度 = " << longStr.size()
              << "，本機 libstdc++ 的 SSO 門檻 = 15 → 超過，必須配置 heap\n";
    std::cout << "sizeof(std::string) = " << sizeof(std::string)
              << " 位元組（實作定義）\n";
    std::cout << "元素個數 = " << N << "\n\n";

    std::vector<std::string> data(N, longStr);

    // (a) 值：每次迭代複製一個 string
    HeapStats::reset();
    HeapStats::tracking = true;
    long long totalA = 0;
    for (std::string s : data) {
        totalA += static_cast<long long>(s.size());
    }
    HeapStats::tracking = false;
    long newsByValue = HeapStats::news;
    long delsByValue = HeapStats::deletes;

    // (b) const 參考：零複製
    HeapStats::reset();
    HeapStats::tracking = true;
    long long totalB = 0;
    for (const std::string& s : data) {
        totalB += static_cast<long long>(s.size());
    }
    HeapStats::tracking = false;
    long newsByRef = HeapStats::news;
    long delsByRef = HeapStats::deletes;

    std::cout << "for (std::string s : data)        → new " << newsByValue
              << " 次、delete " << delsByValue << " 次\n";
    std::cout << "for (const std::string& s : data) → new " << newsByRef
              << " 次、delete " << delsByRef << " 次\n";
    std::cout << "兩者計算結果相同：" << std::boolalpha << (totalA == totalB)
              << "（總位元組 " << totalA << "）\n";
    std::cout << "→ 值版本每個元素配置並釋放一次記憶體，共 " << newsByValue
              << " 次；參考版本一次都沒有。\n";
    std::cout << "（這組數字每次執行完全相同，不像耗時會浮動）\n";

    std::cout << "\n=== 2. 短字串（SSO 範圍內）就不會配置 ===\n";
    std::vector<std::string> shortData(N, "short");   // 長度 5，在 SSO 內
    HeapStats::reset();
    HeapStats::tracking = true;
    long long totalC = 0;
    for (std::string s : shortData) {                 // 一樣是值傳遞
        totalC += static_cast<long long>(s.size());
    }
    HeapStats::tracking = false;
    std::cout << "字串長度 5（≤ SSO 門檻 15）、同樣用值迭代 → new "
              << HeapStats::news << " 次\n";
    std::cout << "→ 複製仍然發生（32 位元組的物件本體），但不需要 heap 配置，\n";
    std::cout << "  所以成本低很多。這說明「值傳遞的代價」高度取決於元素型別。\n";

    std::cout << "\n=== 3. 原始示範：實際計時（結果印在 stderr）===\n";
    std::vector<std::string> big(100'000, longStr);

    // 拷貝遍歷（每次迭代都拷貝一個 string）
    long long t1 = 0;
    {
        Timer t("for (string s : data)");
        for (std::string s : big) {
            t1 += static_cast<long long>(s.size());
        }
    }

    // const 引用遍歷（零拷貝）
    long long t2 = 0;
    {
        Timer t("for (const string& s : data)");
        for (const std::string& s : big) {
            t2 += static_cast<long long>(s.size());
        }
    }

    std::cout << "兩種寫法結果一致：" << std::boolalpha << (t1 == t2) << "\n";
    std::cout << "耗時已印到 stderr（每次執行都不同，故不列入預期輸出）。\n";
    std::cout << "只看 stdout 請用：./demo8 2>/dev/null\n";

    std::cout << "\n=== 4. auto 不會自動變成參考 ===\n";
    HeapStats::reset();
    HeapStats::tracking = true;
    for (auto s : data) { (void)s.size(); }           // auto → 推導成值型別！
    HeapStats::tracking = false;
    long newsAuto = HeapStats::news;

    HeapStats::reset();
    HeapStats::tracking = true;
    for (const auto& s : data) { (void)s.size(); }    // const auto& → 零複製
    HeapStats::tracking = false;

    std::cout << "for (auto s : data)       → new " << newsAuto << " 次（仍然複製！）\n";
    std::cout << "for (const auto& s : data)→ new " << HeapStats::news << " 次\n";
    std::cout << "→ 「用了 auto 就會自動最佳化」是常見誤解，auto 推導的是值型別。\n";

    std::cout << "\n=== 日常實務：access log 統計（零複製掃描）===\n";
    std::vector<std::string> accessLog = {
        R"(10.0.0.1 - - [19/Jul/2026:08:00:01] "GET /api/users HTTP/1.1" 200 1043)",
        R"(10.0.0.7 - - [19/Jul/2026:08:00:02] "POST /api/order HTTP/1.1" 500 87)",
        R"(10.0.0.3 - - [19/Jul/2026:08:00:04] "GET /static/app.js HTTP/1.1" 200 20481)",
        R"(10.0.0.9 - - [19/Jul/2026:08:00:05] "GET /api/health HTTP/1.1" 503 42)",
        R"(10.0.0.2 - - [19/Jul/2026:08:00:07] "GET /index.html HTTP/1.1" 200 5120)"
    };

    HeapStats::reset();
    HeapStats::tracking = true;
    LogSummary s = summarize(accessLog);
    HeapStats::tracking = false;

    std::cout << "掃描 " << s.lines << " 行\n";
    std::cout << "總位元組（行長度總和）：" << s.totalBytes << "\n";
    std::cout << "5xx 錯誤行數：" << s.errorLines << "\n";
    std::cout << "掃描過程 heap 配置次數：" << HeapStats::news
              << "（全程 const&，一次都沒配置）\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 20 課：vector 效能分析與最佳實踐8.cpp" -o demo8
// 只看 stdout: ./demo8 2>/dev/null
//
// ⚠️ 但書：
//   1. 第 3 節的耗時印在 stderr，每次執行都不同（受 CPU 頻率、快取、
//      其他行程影響），故不納入下方預期輸出。
//   2. SSO 門檻 15、sizeof(std::string)==32 是本機 g++ 15.2 / libstdc++
//      的實作值，非標準保證。
//   3. 本檔全域覆寫 operator new/delete 僅為教學計數用途，
//      正式專案請改用 profiler 或 heaptrack 等工具。

// === 預期輸出 ===
// === 1. 可重現證據：迴圈中發生幾次 heap 配置 ===
// 字串長度 = 44，本機 libstdc++ 的 SSO 門檻 = 15 → 超過，必須配置 heap
// sizeof(std::string) = 32 位元組（實作定義）
// 元素個數 = 20000
//
// for (std::string s : data)        → new 20000 次、delete 20000 次
// for (const std::string& s : data) → new 0 次、delete 0 次
// 兩者計算結果相同：true（總位元組 880000）
// → 值版本每個元素配置並釋放一次記憶體，共 20000 次；參考版本一次都沒有。
// （這組數字每次執行完全相同，不像耗時會浮動）
//
// === 2. 短字串（SSO 範圍內）就不會配置 ===
// 字串長度 5（≤ SSO 門檻 15）、同樣用值迭代 → new 0 次
// → 複製仍然發生（32 位元組的物件本體），但不需要 heap 配置，
//   所以成本低很多。這說明「值傳遞的代價」高度取決於元素型別。
//
// === 3. 原始示範：實際計時（結果印在 stderr）===
// 兩種寫法結果一致：true
// 耗時已印到 stderr（每次執行都不同，故不列入預期輸出）。
// 只看 stdout 請用：./demo8 2>/dev/null
//
// === 4. auto 不會自動變成參考 ===
// for (auto s : data)       → new 20000 次（仍然複製！）
// for (const auto& s : data)→ new 0 次
// → 「用了 auto 就會自動最佳化」是常見誤解，auto 推導的是值型別。
//
// === 日常實務：access log 統計（零複製掃描）===
// 掃描 5 行
// 總位元組（行長度總和）：354
// 5xx 錯誤行數：2
// 掃描過程 heap 配置次數：0（全程 const&，一次都沒配置）
