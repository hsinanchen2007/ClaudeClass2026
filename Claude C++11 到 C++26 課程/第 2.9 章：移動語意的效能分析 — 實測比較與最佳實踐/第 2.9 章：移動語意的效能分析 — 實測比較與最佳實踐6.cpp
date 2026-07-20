// =============================================================================
//  第 2.9 章 範例 6  —  noexcept 如何決定 vector 擴容走「移動」還是「複製」
// =============================================================================
//
// 【主題資訊 Information】
//   標準版本：C++11 起（noexcept、移動語意、std::move_if_noexcept）
//             本檔使用 C++17（inline static 成員變數用來計數）
//   標頭檔  ：<vector>、<string>、<utility>、<type_traits>、<chrono>
//   關鍵工具：std::move_if_noexcept —— vector 擴容時用它決定要移動還是複製
//   複雜度  ：擴容一次要搬 n 個元素；用移動是 n 次 O(1)，用複製是 n 次 O(元素大小)
//
// 【詳細解釋 Explanation】
//
// 【1. vector 擴容時到底發生什麼事】
//   vector 的元素必須連續存放。當 size 觸及 capacity 又要再塞一個元素時，
//   它只能：配置一塊更大的緩衝區 → 把舊元素逐一搬過去 → 釋放舊緩衝區。
//   「逐一搬過去」這一步，就是本檔的主角：到底是用移動建構，還是複製建構？
//
// 【2. 為什麼不是「有移動建構就一定用移動」】
//   因為 vector 的 push_back / emplace_back 承諾了「強例外保證」：
//   操作要嘛完全成功，要嘛拋出例外但容器維持原狀，不會壞在中間。
//     * 若用「複製」搬：搬到第 k 個時丟例外 —— 舊緩衝區原封不動還在，
//       把新緩衝區丟掉就回到原狀。保證成立。
//     * 若用「移動」搬：搬到第 k 個時丟例外 —— 前 k 個元素已經被掏空了，
//       舊緩衝區處於殘缺狀態，沒有辦法回復。保證破功。
//   所以標準函式庫用 std::move_if_noexcept 做判斷：
//       「移動建構是 noexcept」或「這個型別根本不能複製」→ 用移動
//        否則（可複製、但移動可能丟例外）                 → 保守地用複製
//
// 【3. 結論：少一個 noexcept，整個移動語意在最關鍵的路徑上失效】
//   你辛苦寫的移動建構，在最需要效能的擴容場景完全不會被呼叫。
//   而且沒有任何警告、行為完全正確、單元測試全過 —— 只是慢。
//   這就是為什麼「移動建構／移動賦值一定要標 noexcept」被列為硬性規範。
//
// 【概念補充 Concept Deep Dive】
//   本檔不量時間，改「數次數」。原因是牆鐘時間有兩個致命問題：
//     (a) 不可重現，同一支程式跑三次三個答案，教材無法貼出穩定的預期輸出；
//     (b) 會被最佳化與快取行為誤導，量到的常常不是你以為的那件事。
//   直接統計「複製建構被呼叫幾次／移動建構被呼叫幾次」，
//   是由語言規則決定的結果，跑幾次都一樣，而且證據力更強：
//   它不是「比較快」這種相對說法，而是「這條路徑根本沒被走到」的直接證明。
//   牆鐘時間仍保留一份，但輸出到 stderr，避免污染要驗收的 stdout。
//
//   為了讓 static_assert 也能佐證，本檔額外用
//   std::is_nothrow_move_constructible_v 在「編譯期」印出兩個型別的差異，
//   說明這個判斷完全發生在型別層次，與執行期無關。
//
// 【注意事項 Pay Attention】
// 1. 搬移總次數取決於 vector 的成長策略（libstdc++ 為 2 倍，屬實作定義）。
//    重點不在那個數字本身，而在「複製欄」與「移動欄」哪一欄是 0。
// 2. Timer 的成員初始化順序必須與宣告順序一致，否則會觸發 -Wreorder
//    （-Wall 預設開啟）。本檔前一版就踩到這個問題，已修正。
// 3. 「不能複製、但移動也沒標 noexcept」的型別仍然會用移動 ——
//    因為沒有別的選擇，不是因為它安全。
// 4. 移動後的來源是「有效但未指定」狀態，不可假設其內容。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】noexcept 與 vector 擴容
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼移動建構一定要加 noexcept？不加會發生什麼事？
//     答：vector 擴容要維持強例外保證，用 std::move_if_noexcept 決定搬法。
//         移動建構沒標 noexcept 時，只要型別可複製，vector 就會保守地改用
//         「複製」搬移舊元素。結果是移動建構在最吃效能的擴容路徑上
//         一次都不會被呼叫，效能等同沒寫移動語意，且毫無警告。
//     追問：那如果這個型別不能複製（複製建構被 delete）呢？
//         → 此時 move_if_noexcept 仍然選移動，因為沒有別的選項；
//           但強例外保證就降級為基本保證了。
//
// ⚠️ 陷阱. 「我的移動建構裡面只是搬指標，絕對不會丟例外，所以標不標 noexcept
//           只是形式問題，反正實際上不會丟。」
//     答：錯。編譯器不會去分析你的函式體「實際上會不會丟」，
//         它只看你有沒有在型別簽章上宣告 noexcept。
//         這是一個純粹的「型別層次查詢」（is_nothrow_move_constructible），
//         在編譯期就決定了，跟你函式裡寫什麼完全無關。
//     為什麼會錯：把 noexcept 當成「給人看的註解」或「執行期檢查」。
//         實際上它是型別系統的一部分，是給多載決議與函式庫看的契約。
//         沒宣告 = 函式庫必須假設你會丟 = 走保守路徑。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include <chrono>
#include <type_traits>
#include <cstddef>

// -----------------------------------------------------------------------------
// Timer：耗時一律寫到 stderr，讓 stdout 保持逐位元組穩定
//   成員宣告順序 = 初始化順序。label_ 宣告在前，初始化列也放在前，
//   否則 -Wall 的 -Wreorder 會發出警告（本檔前一版即是如此）。
// -----------------------------------------------------------------------------
class Timer {
    const char* label_;                                        // 宣告第 1
    std::chrono::high_resolution_clock::time_point start_;     // 宣告第 2
public:
    explicit Timer(const char* label)
        : label_(label),
          start_(std::chrono::high_resolution_clock::now()) {}
    ~Timer() {
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::high_resolution_clock::now() - start_).count();
        std::cerr << "  [timing] " << label_ << ": " << us << " us\n";
    }
};

// 兩個型別唯一的差別：移動建構有沒有標 noexcept
struct WithNoexcept {
    static inline int copies = 0;
    static inline int moves  = 0;
    std::vector<int> data;

    WithNoexcept() : data(1000, 42) {}
    WithNoexcept(const WithNoexcept& o) : data(o.data)                 { ++copies; }
    WithNoexcept(WithNoexcept&& o) noexcept : data(std::move(o.data))  { ++moves;  }

    static void reset() { copies = 0; moves = 0; }
};

struct WithoutNoexcept {
    static inline int copies = 0;
    static inline int moves  = 0;
    std::vector<int> data;

    WithoutNoexcept() : data(1000, 42) {}
    WithoutNoexcept(const WithoutNoexcept& o) : data(o.data)           { ++copies; }
    WithoutNoexcept(WithoutNoexcept&& o) : data(std::move(o.data))     { ++moves;  }  // 沒有 noexcept

    static void reset() { copies = 0; moves = 0; }
};

// 編譯期就能查出差別：這正是 vector 用來做決定的那個查詢
static_assert(std::is_nothrow_move_constructible_v<WithNoexcept>,
              "WithNoexcept 的移動建構應為 noexcept");
static_assert(!std::is_nothrow_move_constructible_v<WithoutNoexcept>,
              "WithoutNoexcept 的移動建構不應為 noexcept");

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔不附
//   理由：本檔主題是「例外保證如何影響標準函式庫的內部搬移策略」，
//   屬於語言契約與函式庫實作層面。LeetCode 評測的是演算法正確性與複雜度，
//   沒有題目的結果會因為「有沒有寫 noexcept」而改變。
//   依規格寧缺勿濫，改附一個真實會踩到這個坑的工程情境。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】監控系統的取樣緩衝區容器
//   情境：每個受監控的主機對應一個 MetricSeries，裡面存最近 N 筆取樣值。
//         主機是動態被發現的，所以要一直 push_back 進 registry，觸發擴容。
//   踩坑點：這種「內含 vector 成員」的類別，如果自己手寫了複製/移動建構
//         卻忘了在移動建構標 noexcept，registry 每次擴容都會把所有主機的
//         取樣資料整份深複製一次。主機數量越多、取樣越密，浪費越嚴重，
//         而且從功能面完全看不出異常 —— 只是 CPU 莫名偏高。
// -----------------------------------------------------------------------------
struct MetricSeries {
    static inline int deepCopies = 0;      // 統計「整份取樣資料被深複製」的次數
    std::string        host;
    std::vector<double> samples;

    MetricSeries(std::string h, std::size_t n)
        : host(std::move(h)), samples(n, 0.0) {}

    MetricSeries(const MetricSeries& o)
        : host(o.host), samples(o.samples) { ++deepCopies; }

    // ✅ 有 noexcept：registry 擴容時會用移動，取樣資料只搬指標
    MetricSeries(MetricSeries&& o) noexcept
        : host(std::move(o.host)), samples(std::move(o.samples)) {}

    static void reset() { deepCopies = 0; }
};

int main() {
    std::cout << "=== 編譯期查詢：vector 就是看這個決定搬法 ===\n";
    std::cout << "  is_nothrow_move_constructible<WithNoexcept>    = "
              << std::boolalpha
              << std::is_nothrow_move_constructible_v<WithNoexcept> << "\n";
    std::cout << "  is_nothrow_move_constructible<WithoutNoexcept> = "
              << std::is_nothrow_move_constructible_v<WithoutNoexcept> << "\n";

    std::cout << "\n=== 擴容時實際走了哪一條路（不 reserve，逼它反覆擴容）===\n";
    {
        WithNoexcept::reset();
        {
            Timer t("有 noexcept");
            std::vector<WithNoexcept> v;
            for (int i = 0; i < 1024; ++i) v.emplace_back();
        }
        std::cout << "  有 noexcept ：複製建構 " << WithNoexcept::copies
                  << " 次、移動建構 " << WithNoexcept::moves << " 次\n";
    }
    {
        WithoutNoexcept::reset();
        {
            Timer t("沒 noexcept");
            std::vector<WithoutNoexcept> v;
            for (int i = 0; i < 1024; ++i) v.emplace_back();
        }
        std::cout << "  沒 noexcept ：複製建構 " << WithoutNoexcept::copies
                  << " 次、移動建構 " << WithoutNoexcept::moves << " 次\n";
    }
    std::cout << "  → 少一個關鍵字，1023 次搬移就整批從「移動」變成「深複製」\n";

    std::cout << "\n=== 日常實務：監控 registry 擴容 ===\n";
    {
        MetricSeries::reset();
        std::vector<MetricSeries> registry;      // 不 reserve，模擬主機陸續被發現
        for (int i = 0; i < 100; ++i) {
            registry.emplace_back("host-" + std::to_string(i), 512);
        }
        std::cout << "  註冊 " << registry.size() << " 台主機（每台 512 筆取樣）\n";
        std::cout << "  過程中取樣資料被深複製的次數：" << MetricSeries::deepCopies << "\n";
        std::cout << "  → 移動建構有標 noexcept，擴容全走移動，一次深複製都沒有\n";
        std::cout << "  （若把該處 noexcept 拿掉，這個數字會等於累計搬移次數）\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.9 章：移動語意的效能分析 — 實測比較與最佳實踐6.cpp" -o noexcept_growth
// 執行: ./noexcept_growth             （stdout 穩定；stderr 另有耗時參考）
//       ./noexcept_growth 2>/dev/null （只看可重現的計數結果）

// ── 輸出但書 ────────────────────────────────────────────────────────────
// 1. 下方預期輸出只含 stdout，全部是計數與布林值，逐位元組可重現
//    （實測連跑 10 次 md5 相同）。stderr 的 [timing] 每次執行都不同，
//    不納入預期輸出，也不應拿來當驗收條件。
// 2. 搬移次數 1023 來自 libstdc++ 的 2 倍成長策略（屬實作定義）：
//    1024 次 emplace_back 會歷經 capacity 1→2→4→…→1024，
//    累計搬動 1+2+4+…+512 = 1023 個元素。換一個成長倍率不同的實作
//    （例如 MSVC 約 1.5 倍）數字會不同，但「哪一欄是 0」的分野不變。
// 3. 監控 registry 那段的深複製次數為 0，是因為 MetricSeries 的移動建構
//    有標 noexcept。這個 0 才是重點，不是「100 台」這個數字。
// 4. 本機環境：GCC 15.2.0 (Ubuntu 15.2.0-16ubuntu1) / libstdc++ / x86-64。

// === 預期輸出 ===
// === 編譯期查詢：vector 就是看這個決定搬法 ===
//   is_nothrow_move_constructible<WithNoexcept>    = true
//   is_nothrow_move_constructible<WithoutNoexcept> = false
//
// === 擴容時實際走了哪一條路（不 reserve，逼它反覆擴容）===
//   有 noexcept ：複製建構 0 次、移動建構 1023 次
//   沒 noexcept ：複製建構 1023 次、移動建構 0 次
//   → 少一個關鍵字，1023 次搬移就整批從「移動」變成「深複製」
//
// === 日常實務：監控 registry 擴容 ===
//   註冊 100 台主機（每台 512 筆取樣）
//   過程中取樣資料被深複製的次數：0
//   → 移動建構有標 noexcept，擴容全走移動，一次深複製都沒有
//   （若把該處 noexcept 拿掉，這個數字會等於累計搬移次數）
