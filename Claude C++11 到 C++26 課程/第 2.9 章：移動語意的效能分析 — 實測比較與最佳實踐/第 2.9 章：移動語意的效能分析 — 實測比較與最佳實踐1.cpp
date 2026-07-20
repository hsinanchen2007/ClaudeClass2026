// =============================================================================
//  第 2.9 章 範例 1  —  string 複製 vs 移動：為什麼移動是 O(1)
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<string>、<utility>（move）、<chrono>（計時）
//   複雜度對比（這是本章的全部重點）：
//     複製 std::string：O(n) —— 配置新緩衝區 + memcpy n 個位元組
//     移動 std::string：O(1) —— 搬三個欄位（指標、長度、容量），與 n 無關
//   std::string 的移動建構子自 C++11 起為 noexcept。
//
// 【詳細解釋 Explanation】
//
// 【1. 移動為什麼是 O(1)：看清 std::string 的內部佈局】
//   libstdc++ 的 std::string 內部大致是：
//       char*       ptr_;          // 指向堆積緩衝區（或指向自己的 SSO 區）
//       size_type   size_;         // 目前長度
//       union { char sso_[16]; size_type capacity_; };
//   複製時必須：配置一塊新的堆積記憶體 → memcpy 全部內容 → 設定欄位。
//   移動時只需要：把 ptr_/size_/capacity_ 三個欄位搬過來 → 把來源設成空。
//   **移動搬的是「指向資料的憑證」，不是資料本身。**
//   所以字串越長，兩者差距越大；而移動的成本從頭到尾都是固定的幾個賦值。
//
// 【2. 本檔的測量方式：為什麼「移動」那組要先複製一份】
//   移動會破壞來源，所以不能在迴圈裡反覆移動同一個 source
//   （第二圈開始 source 就是空的，量到的是「移動空字串」，毫無意義）。
//   因此移動組每圈都先 std::string temp = source;（一次複製）再移動它。
//   這代表：
//       複製組每圈 = 1 次複製
//       移動組每圈 = 1 次複製 + 1 次移動
//   移動組做的事其實「更多」，卻仍然只慢一點點——這正好證明移動幾乎免費。
//   ★ 這個測量設計常被誤解成「移動比複製慢」，要看清楚它多做了一次複製。
//
// 【3. 為什麼耗時輸出改到 stderr】
//   牆鐘時間每次執行都不同（受 CPU 頻率、快取狀態、系統排程影響），
//   把它寫進 stdout 會讓「預期輸出」永遠對不上。
//   本檔已把 Timer 改為輸出到 std::cerr，並在 stdout 印出
//   **確定性的操作計數**（配置次數、複製的位元組數）——
//   這些數字每次執行完全相同，可以直接寫成單元測試的斷言。
//   這是效能測試的通則：**能用計數驗證的，就不要用時間驗證。**
//
// 【4. 成員初始化順序（本檔原本有 -Wreorder 警告，已修正）】
//   Timer 的成員宣告順序是 start_ 在前、label_ 在後，
//   但初始化列表原本寫成 label_(label), start_(now())。
//   成員的初始化「一律依宣告順序」執行，與初始化列表的書寫順序無關，
//   g++ 會以 -Wreorder 警告（-Wall 已包含）。
//   本檔已把初始化列表改成與宣告順序一致。
//   在本例中兩者沒有相依關係所以沒有實際 bug，但若寫成
//       Timer(const char* l) : label_(l), start_(computeFrom(label_)) {}
//   而 start_ 宣告在前，start_ 就會讀到尚未初始化的 label_——那才是真正的災難。
//
// 【概念補充 Concept Deep Dive】
//   (A) SSO（Small String Optimization）：libstdc++ 的門檻是 15 個字元
//       （sizeof(std::string) 為 32 bytes，其中 16 bytes 是 SSO 緩衝區，
//       扣掉結尾的 '\0' 可存 15 個字元）——這是實作定義的值。
//       短字串完全存在物件內部、不碰堆積，此時移動只能逐位元組複製那 16 bytes，
//       和複製幾乎沒有差別（見本章範例 7）。本檔用 10000 字元正是為了避開 SSO。
//   (B) 開啟最佳化後的觀察：本檔的迴圈結果沒有被使用，
//       -O2 下編譯器可能把整段迴圈刪掉（dead code elimination），量到接近 0 ms。
//       這也是為什麼「用計數而非計時」更可靠——計數器有副作用，不會被最佳化掉。
//   (C) 真正嚴謹的微效能測量應使用 Google Benchmark 之類的框架，
//       它會自動處理暖機、重複次數、以及防止編譯器最佳化掉待測程式碼。
//
// 【注意事項 Pay Attention】
//   1. 不可在迴圈中反覆移動同一個來源——第二圈起它已是空殼。
//   2. 耗時數字受機器、負載、最佳化等級影響，本檔的 stderr 輸出僅供參考，
//      不同機器上的絕對數值必然不同。
//   3. 移動組每圈多做一次複製，直接比較兩組的絕對時間會低估移動的優勢。
//   4. 成員初始化順序看宣告順序（-Wreorder），不是初始化列表順序。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】string 的複製與移動成本
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 std::string 的移動是 O(1)，複製是 O(n)？
//     答：string 內部持有「指標 + 長度 + 容量」三個欄位，實際字元資料在堆積上。
//         複製必須配置新緩衝區再 memcpy n 個位元組，成本隨長度線性成長；
//         移動只需把那三個欄位搬過來、把來源設空，成本固定與長度無關。
//         移動搬的是「指向資料的憑證」，不是資料本身。
//     追問：那為什麼短字串移動起來沒有比較快？
//         → 因為 SSO：短字串直接存在物件內部（libstdc++ 門檻是 15 字元），
//           根本沒有堆積指標可偷，移動只能逐位元組複製那塊小緩衝區。
//
// 🔥 Q2. 這個測試中，「移動」那一組為什麼每圈要先複製一份 temp？
//     答：因為移動會破壞來源。若直接在迴圈裡移動 source，
//         第二圈開始 source 已經是空字串，量到的是「移動空字串」，毫無意義。
//         代價是移動組每圈變成「1 次複製 + 1 次移動」，比複製組多做事——
//         它仍然只慢一點點，反而證明了移動本身幾乎免費。
//
// ⚠️ 陷阱. 「這個 benchmark 顯示移動組比複製組慢，所以 std::move 沒用」——錯在哪？
//     答：兩組做的事情根本不同。複製組每圈 1 次複製；
//         移動組每圈 1 次複製 + 1 次移動。移動組多做了一整件事還能跑得差不多，
//         正說明移動的成本趨近於零。要公平比較，該比的是
//         「複製 n 次」對「複製 1 次後移動 n 次」。
//     為什麼會錯：看 benchmark 只看最後的數字，沒有先確認兩組
//         「做的工作量是否相同」。這是效能測試最常見的方法論錯誤——
//         數字本身不會騙人，但拿它回答錯誤的問題就會得到錯誤的結論。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <chrono>

class Timer {
    // ⚠️ 成員初始化順序依「宣告順序」：start_ 在前、label_ 在後。
    //    初始化列表必須寫成相同順序，否則觸發 -Wreorder 警告。
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

// -----------------------------------------------------------------------------
// 確定性的驗證：用「計數」取代「計時」
//   自訂 allocator 太複雜，這裡改用一個最小的可觀測型別：
//   Payload 模擬「持有堆積資源」的類別，並統計複製與移動各發生幾次、
//   總共搬運了多少位元組。這些數字每次執行完全相同。
// -----------------------------------------------------------------------------
struct Stats {
    long long copies = 0;          // 複製建構次數
    long long moves = 0;           // 移動建構次數
    long long bytesCopied = 0;     // 因複製而實際搬運的位元組數
    long long bytesMoved = 0;      // 因移動而實際搬運的位元組數（永遠是固定欄位大小）
};
Stats g_stats;

class Payload {
    std::string data_;
public:
    explicit Payload(std::size_t n) : data_(n, 'x') {}

    Payload(const Payload& o) : data_(o.data_) {          // O(n)
        ++g_stats.copies;
        g_stats.bytesCopied += static_cast<long long>(o.data_.size());
    }

    Payload(Payload&& o) noexcept : data_(std::move(o.data_)) {   // O(1)
        ++g_stats.moves;
        // 移動搬的是固定大小的欄位，與內容長度無關
        g_stats.bytesMoved += static_cast<long long>(sizeof(std::string));
    }

    std::size_t size() const { return data_.size(); }
};

int main() {
    const int N = 2000000;
    std::string source(10000, 'x');  // 10000 字元

    // ── 計時部分：結果輸出到 stderr（每次執行都不同）──
    {
        Timer t("複製");
        for (int i = 0; i < N; ++i) {
            std::string copy = source;
            (void)copy;
        }
    }

    {
        Timer t("移動");
        for (int i = 0; i < N; ++i) {
            std::string temp = source;            // 先複製一份供移動
            std::string moved = std::move(temp);  // 移動
            (void)moved;
        }
    }

    // ── 確定性部分：結果輸出到 stdout（每次執行完全相同）──
    std::cout << "=== 確定性驗證：複製與移動各搬運了多少位元組 ===\n";
    {
        const int M = 1000;                 // 次數小一點，因為這裡只看計數不看時間
        const std::size_t LEN = 10000;

        g_stats = Stats{};
        {
            Payload src(LEN);
            for (int i = 0; i < M; ++i) {
                Payload copy = src;          // 複製建構
                (void)copy;
            }
        }
        std::cout << "  複製 " << M << " 次長度 " << LEN << " 的物件:\n";
        std::cout << "    複製次數 = " << g_stats.copies << "\n";
        std::cout << "    實際搬運位元組 = " << g_stats.bytesCopied << "\n";

        g_stats = Stats{};
        {
            for (int i = 0; i < M; ++i) {
                Payload tmp(LEN);            // 建構（不計入複製/移動）
                Payload moved = std::move(tmp);
                (void)moved;
            }
        }
        std::cout << "  移動 " << M << " 次同樣長度的物件:\n";
        std::cout << "    移動次數 = " << g_stats.moves << "\n";
        std::cout << "    實際搬運位元組 = " << g_stats.bytesMoved
                  << "（= " << M << " × sizeof(std::string) = "
                  << sizeof(std::string) << " bytes）\n";

        std::cout << "  → 複製搬運量與長度成正比；移動搬運量是固定的欄位大小\n";
        std::cout << "  → 這就是 O(n) 與 O(1) 的差別，而且此結論與機器無關\n";
    }

    std::cout << "\n（耗時數字每次執行都不同，已輸出到 stderr，不列入預期輸出）\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.9 章：移動語意的效能分析 — 實測比較與最佳實踐1.cpp" -o move_perf1
// 觀察耗時（stderr）: ./move_perf1 2>&1 >/dev/null

// 註：本檔未附 LeetCode 範例。效能量測是工程方法論，
//     LeetCode 只判斷是否通過時限，不提供逐項成本觀測；硬套一題只會失真。
//
// 註：sizeof(std::string) 為「實作定義」的值，本機（g++ 15.2 / libstdc++ /
//     x86-64）為 32 bytes。其他實作（例如 MSVC 的 STL 為 40 bytes）不同。
//     下方輸出中的「實際搬運位元組」因此也是實作定義的數值。
//
// 註：耗時輸出已導向 stderr，不出現在下方的 stdout 預期輸出中。

// === 預期輸出 ===
// === 確定性驗證：複製與移動各搬運了多少位元組 ===
//   複製 1000 次長度 10000 的物件:
//     複製次數 = 1000
//     實際搬運位元組 = 10000000
//   移動 1000 次同樣長度的物件:
//     移動次數 = 1000
//     實際搬運位元組 = 32000（= 1000 × sizeof(std::string) = 32 bytes）
//   → 複製搬運量與長度成正比；移動搬運量是固定的欄位大小
//   → 這就是 O(n) 與 O(1) 的差別，而且此結論與機器無關
//
// （耗時數字每次執行都不同，已輸出到 stderr，不列入預期輸出）
