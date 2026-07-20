// =============================================================================
//  第 2.9 章 範例 2  —  把 benchmark 修對：如何公平比較複製與移動
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<string>、<utility>（move）、<chrono>
//   本檔與範例 1 的程式碼起點相同，但主題不同：
//     範例 1 講「移動為什麼是 O(1)」；
//     本檔講「這個 benchmark 本身有什麼方法論問題，以及怎麼修」。
//   這是效能工程的核心技能——量錯了，再漂亮的數字都是誤導。
//
// 【詳細解釋 Explanation】
//
// 【1. 原始 benchmark 的三個方法論缺陷】
//   缺陷 A：兩組的工作量不相等
//       複製組每圈：1 次複製
//       移動組每圈：1 次複製 + 1 次移動
//     移動組多做了一整次複製。實測本機的結果是「移動組反而比較慢」——
//     若照字面解讀就會得到「std::move 沒用」這種完全錯誤的結論。
//   缺陷 B：結果沒有被使用，可能被最佳化掉
//     std::string copy = source; 之後 copy 從未被讀取。
//     -O2 下編譯器有權把整個迴圈刪除（as-if 規則），量到接近 0 ms。
//     加了 -O0 才「看得到」時間，但那量的又不是真實的最佳化後效能。
//   缺陷 C：用牆鐘時間當唯一指標
//     受 CPU 頻率調節、快取狀態、其他行程排程影響，跑兩次就不一樣。
//
// 【2. 怎麼修：三種改法，本檔全部示範】
//   修法 1（公平比較）：讓兩組的「被測操作」次數相同。
//       複製組：準備好 N 個來源 → 各複製一次
//       移動組：準備好 N 個來源 → 各移動一次
//     兩組的前置成本相同，差異才真的來自複製 vs 移動。
//   修法 2（防最佳化）：把結果累加進一個 volatile 或外部可見的變數，
//     讓編譯器無法證明「刪掉也沒差」。本檔用累加 size() 到全域變數。
//   修法 3（確定性指標）：直接數「配置了幾次堆積記憶體」。
//     這個數字與機器、負載、最佳化等級完全無關，是最可靠的證據。
//     本檔用自訂的 operator new 計數器達成。
//
// 【3. 為什麼「配置次數」是比時間更好的指標】
//   複製 std::string 必然要配置一塊新的堆積記憶體（超過 SSO 門檻時）；
//   移動則完全不配置。所以：
//       複製 N 次 → N 次配置
//       移動 N 次 → 0 次配置
//   這是一個「結構性」的差異，不是「這台機器上比較快」的經驗觀察。
//   面試時能講出這一點，遠比背「移動比較快」有說服力。
//
// 【4. 自訂 operator new 的注意事項】
//   本檔為了計數而全域覆寫 operator new / operator delete。
//   這在教學示範中沒問題，但真實專案要小心：
//     * 必須同時覆寫對應的 delete，否則行為未定義
//     * 必須處理配置失敗（本檔以 std::bad_alloc 處理）
//     * 靜態初始化期間的配置也會被計入
//   實務上更常用的是 heaptrack、massif 等外部工具，不必修改程式碼。
//
// 【概念補充 Concept Deep Dive】
//   (A) 為什麼不能只加 -O2 就好？因為最佳化後空迴圈會被刪掉，
//       你會量到「什麼都沒做」的時間。正確做法是既開最佳化，
//       又讓結果真的被使用（本檔的累加），這樣量到的才是真實效能。
//   (B) 標準有一個特例：編譯器「被允許」省略某些複製與移動
//       （copy elision）。C++17 起，對 prvalue 初始化的省略是強制的。
//       這會讓「數建構子呼叫次數」的結果與直覺不同，
//       但不影響本檔數的「配置次數」——沒有物件就沒有配置。
//   (C) 為什麼 sizeof(std::string) 在本機是 32 bytes？
//       libstdc++ 的佈局是 指標(8) + 長度(8) + SSO 聯集(16) = 32。
//       這是實作定義的，MSVC 的 STL 為 40 bytes。
//
// 【注意事項 Pay Attention】
//   1. 比較兩組 benchmark 前，先確認它們的工作量真的相同。
//   2. 結果未被使用的迴圈可能被完全最佳化掉，量到的數字沒有意義。
//   3. 牆鐘時間不可重現；能用計數證明的結論就不要靠時間。
//   4. 全域覆寫 operator new 會影響整個程式，包含標準函式庫內部的配置。
//   5. 本檔的配置次數在同一台機器同一次建置下完全確定，
//      但不同標準函式庫實作（SSO 門檻不同）可能得到不同數字。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】效能量測的方法論
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 有人給你一個 benchmark，顯示 std::move 比複製還慢。你怎麼判斷？
//     答：先檢查兩組的工作量是否相同。常見錯誤是移動組每圈多做一次複製
//         （因為移動會破壞來源，必須先準備新的來源），
//         於是量到的是「複製+移動」對「複製」。
//         其次檢查結果有沒有被使用——沒被使用的迴圈可能整段被最佳化掉。
//     追問：那你會怎麼重新設計這個測試？
//         → 兩組都先準備好 N 個獨立來源，再各自做一次被測操作；
//           並改用「堆積配置次數」這類確定性指標佐證。
//
// 🔥 Q2. 為什麼「配置次數」比「執行時間」是更好的效能證據？
//     答：因為它是結構性的事實，與機器、負載、最佳化等級都無關。
//         複製長字串必然配置一次堆積記憶體，移動則是零次——
//         這個結論在任何機器上都成立，而時間數字換台機器就變了。
//         而且計數可以直接寫成單元測試的斷言，時間不行。
//
// ⚠️ 陷阱. 「加上 -O2 之後移動和複製的時間都變成 0 ms，
//          代表現代編譯器已經幫我們最佳化好了」——錯在哪？
//     答：0 ms 不是因為操作變快，而是因為整個迴圈被刪掉了。
//         結果從未被使用，編譯器依 as-if 規則有權移除沒有可觀察副作用的程式碼。
//         你量到的是「什麼都不做」的時間，不是複製或移動的成本。
//     為什麼會錯：把「時間變短」直接解讀成「程式變快」，
//         但 benchmark 量的其實是「編譯器決定執行什麼」。
//         正確做法是讓結果真的被使用（累加、寫入 volatile、回傳給外部），
//         迫使編譯器必須真的執行被測程式碼。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <chrono>
#include <new>
#include <cstdlib>

// -----------------------------------------------------------------------------
// 確定性指標：全域統計堆積配置次數
//   覆寫 operator new / delete 來計數。這個數字與機器、負載、
//   最佳化等級完全無關，是「複製配置、移動不配置」最直接的證據。
// -----------------------------------------------------------------------------
namespace alloc_counter {
    long long allocations = 0;
    bool      enabled = false;      // 只在測量區間內計數，避免混入無關配置
    void reset() { allocations = 0; }
}

void* operator new(std::size_t sz) {
    if (alloc_counter::enabled) ++alloc_counter::allocations;
    if (sz == 0) sz = 1;                       // 標準要求：0 大小也要回傳有效指標
    if (void* p = std::malloc(sz)) return p;
    throw std::bad_alloc();
}

void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

class Timer {
    // ⚠️ 成員初始化順序依「宣告順序」：start_ 在前、label_ 在後。
    //    初始化列表必須同序，否則觸發 -Wreorder 警告。
    std::chrono::high_resolution_clock::time_point start_;
    const char* label_;
public:
    Timer(const char* label)
        : start_(std::chrono::high_resolution_clock::now()), label_(label) {}

    ~Timer() {
        auto elapsed = std::chrono::high_resolution_clock::now() - start_;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        // ★ 耗時每次執行都不同 → 寫到 stderr，讓 stdout 保持可重現
        std::cerr << "  [計時] " << label_ << ": " << ms << " ms\n";
    }
};

// 防止編譯器把「結果沒被使用」的迴圈整段刪掉
volatile std::size_t g_sink = 0;

int main() {
    const int N = 2000000;
    std::string source(10000, 'x');  // 10000 字元

    // ==========================================================
    // 原始（有缺陷的）測法：保留下來作為對照
    //   兩組工作量不同 → 移動組每圈多做一次複製
    // ==========================================================
    std::cerr << "--- 原始測法（兩組工作量不相等，僅供對照）---\n";
    {
        Timer t("複製");
        for (int i = 0; i < N; ++i) {
            std::string copy = source;
            g_sink += copy.size();                // 防止整段被最佳化掉
        }
    }

    {
        Timer t("移動");
        for (int i = 0; i < N; ++i) {
            std::string temp = source;            // 先複製一份供移動
            std::string moved = std::move(temp);  // 移動
            g_sink += moved.size();
        }
    }

    // ==========================================================
    // 修正後的公平測法：兩組的被測操作次數相同
    // ==========================================================
    std::cerr << "--- 公平測法（兩組都先備妥來源，只測那一次操作）---\n";
    {
        const int M = 200000;

        // 兩組使用完全相同的前置作業：各準備 M 個獨立來源
        std::vector<std::string> srcA(M, std::string(1000, 'y'));
        std::vector<std::string> srcB(M, std::string(1000, 'y'));

        {
            Timer t("公平-複製");
            for (int i = 0; i < M; ++i) {
                std::string dst = srcA[i];            // 只做「複製」這一件事
                g_sink += dst.size();
            }
        }
        {
            Timer t("公平-移動");
            for (int i = 0; i < M; ++i) {
                std::string dst = std::move(srcB[i]); // 只做「移動」這一件事
                g_sink += dst.size();
            }
        }
    }

    // ==========================================================
    // 確定性證據：堆積配置次數（stdout，每次執行完全相同）
    // ==========================================================
    std::cout << "=== 確定性證據：堆積配置次數 ===\n";
    {
        const int M = 1000;

        // 先備妥來源（不計入）
        std::vector<std::string> srcA(M, std::string(1000, 'z'));
        std::vector<std::string> srcB(M, std::string(1000, 'z'));

        alloc_counter::reset();
        alloc_counter::enabled = true;
        for (int i = 0; i < M; ++i) {
            std::string dst = srcA[i];                // 複製 → 需要新緩衝區
            g_sink += dst.size();
        }
        alloc_counter::enabled = false;
        long long copyAllocs = alloc_counter::allocations;

        alloc_counter::reset();
        alloc_counter::enabled = true;
        for (int i = 0; i < M; ++i) {
            std::string dst = std::move(srcB[i]);     // 移動 → 偷指標，零配置
            g_sink += dst.size();
        }
        alloc_counter::enabled = false;
        long long moveAllocs = alloc_counter::allocations;

        std::cout << "  複製 " << M << " 個長度 1000 的字串:\n";
        std::cout << "    堆積配置次數 = " << copyAllocs << "\n";
        std::cout << "  移動 " << M << " 個同樣的字串:\n";
        std::cout << "    堆積配置次數 = " << moveAllocs << "\n";
        std::cout << "  → 複製每次都要配置新緩衝區；移動一次都不用\n";
        std::cout << "  → 這個結論與機器、負載、最佳化等級完全無關\n";
    }

    std::cout << "\n=== 方法論重點 ===\n";
    std::cout << "  1. 比較前先確認兩組工作量相同\n";
    std::cout << "  2. 讓結果真的被使用，否則迴圈會被最佳化掉\n";
    std::cout << "  3. 能用計數證明的，就不要只靠時間\n";
    std::cout << "\n（耗時數字每次執行都不同，已輸出到 stderr，不列入預期輸出）\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.9 章：移動語意的效能分析 — 實測比較與最佳實踐2.cpp" -o move_perf2
// 觀察耗時（stderr）: ./move_perf2 2>&1 >/dev/null

// 註：本檔未附 LeetCode 範例。benchmark 方法論屬於工程實務，
//     LeetCode 只回報是否通過時限，無法示範逐項成本歸因；硬套一題只會失真。
//
// 註：下方 stdout 的配置次數在本機（g++ 15.2 / libstdc++ / x86-64）為確定值。
//     其他標準函式庫實作若 SSO 門檻不同，長度 1000 的字串仍必然超過門檻，
//     故「複製 M 次 = M 次配置、移動 M 次 = 0 次配置」的結論可攜；
//     但若把長度改到 SSO 門檻以下（本機為 15 字元），兩者都會是 0 次配置。

// === 預期輸出 ===
// === 確定性證據：堆積配置次數 ===
//   複製 1000 個長度 1000 的字串:
//     堆積配置次數 = 1000
//   移動 1000 個同樣的字串:
//     堆積配置次數 = 0
//   → 複製每次都要配置新緩衝區；移動一次都不用
//   → 這個結論與機器、負載、最佳化等級完全無關
//
// === 方法論重點 ===
//   1. 比較前先確認兩組工作量相同
//   2. 讓結果真的被使用，否則迴圈會被最佳化掉
//   3. 能用計數證明的，就不要只靠時間
//
// （耗時數字每次執行都不同，已輸出到 stderr，不列入預期輸出）
