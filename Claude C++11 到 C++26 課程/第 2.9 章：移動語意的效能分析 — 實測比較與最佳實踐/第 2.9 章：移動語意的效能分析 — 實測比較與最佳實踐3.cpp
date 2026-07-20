// =============================================================================
//  第 2.9 章 範例 3  —  vector 的複製與移動：容器移動只搬三個指標
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<vector>、<utility>（move）、<chrono>
//   複雜度：
//     複製 std::vector<T>：O(n) —— 配置新緩衝區 + 複製 n 個元素
//     移動 std::vector<T>：O(1) —— 搬三個指標，與元素數量完全無關
//   std::vector 的移動建構子自 C++11 起為 noexcept。
//   ★ 注意 vector 沒有 SSO：即使只有一個元素也會配置堆積記憶體
//     （這是它與 std::string 最重要的差別）。
//
// 【詳細解釋 Explanation】
//
// 【1. vector 的內部佈局：三個指標】
//   libstdc++ 的 std::vector 內部就是三個指標：
//       T* begin_;        // 資料起點
//       T* end_;          // 最後一個元素的下一個位置（size = end_ - begin_）
//       T* cap_end_;      // 配置區的結尾（capacity = cap_end_ - begin_）
//   因此 sizeof(std::vector<T>) 恆為 24 bytes（三個 8 bytes 指標），
//   **與元素型別和元素數量都無關**——這是實作定義的值，但反映了通用設計。
//   移動就是把這三個指標搬過去、把來源的三個指標設成 nullptr。
//   不論這個 vector 裝了 10 個還是 1000 萬個元素，移動成本完全一樣。
//
// 【2. vector 與 string 的關鍵差異：沒有 SSO】
//   std::string 對短字串有小字串最佳化（本機門檻 15 字元），
//   短字串完全存在物件內部、移動與複製沒有差別。
//   **std::vector 沒有這個機制**：只要非空，資料一定在堆積上。
//   所以「移動比複製划算」對 vector 而言幾乎永遠成立，
//   不像 string 還要先問「有沒有超過 SSO 門檻」。
//   （標準沒有禁止 vector 實作 SSO，但主流實作都沒有做，
//     因為那會破壞「swap 後 iterator 仍有效」等保證。）
//
// 【3. 為什麼複製 vector<int> 特別貴】
//   複製 100000 個 int = 400 KB 的 memcpy。
//   400 KB 遠超過本機 L1（通常 32~48 KB）與 L2 快取，
//   所以每次複製都會實際打到記憶體頻寬。
//   而移動只碰 24 bytes——連一條 cache line（64 bytes）都用不滿。
//   這就是為什麼容器越大，移動的相對優勢越明顯（見範例 4 的規模掃描）。
//
// 【4. 元素型別會影響複製成本，但不影響移動成本】
//   vector<int> 複製時可以用 memcpy（int 是 trivially copyable）。
//   vector<std::string> 複製時必須逐一呼叫 string 的複製建構子，
//   每個元素各自配置堆積記憶體——成本高得多。
//   但兩者的「移動」成本完全相同，都是搬三個指標。
//   **元素越貴，移動的優勢越大。**
//
// 【概念補充 Concept Deep Dive】
//   (A) 移動後的 vector 保證是空的嗎？
//       標準對 vector 的移動建構子只保證來源處於 valid but unspecified 狀態。
//       實務上主流實作都會留下一個空 vector（因為它把指標設為 nullptr），
//       但依賴這件事是不可攜的。要確保為空應明確呼叫 clear()。
//   (B) 為什麼 vector 的移動是 noexcept 而移動賦值需要看 allocator？
//       移動建構只是偷指標，不可能失敗；但移動賦值在 allocator 不相等
//       且不可傳播時，可能需要逐元素搬移（會配置記憶體），因而可能拋例外。
//       這就是 propagate_on_container_move_assignment 這個 allocator trait 的用途。
//   (C) 本檔的測量同樣有「移動組每圈多做一次複製」的問題，
//       原因與範例 1 相同（移動會破壞來源）。改善方法見範例 2。
//
// 【注意事項 Pay Attention】
//   1. vector 沒有 SSO，非空時資料必在堆積——這點與 string 不同。
//   2. sizeof(std::vector<T>) 為 24 bytes 是本機 libstdc++ 的實作定義值，
//      與 T 和元素數量無關。
//   3. 移動後的來源是 valid but unspecified，不保證一定為空。
//   4. 移動組每圈多做一次複製，直接比較絕對時間會低估移動的優勢。
//   5. reserve 不會改變上述結論，但能避免「成長時反覆重新配置」的額外成本。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector 的複製與移動
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼移動一個裝了 1000 萬個元素的 vector，和移動一個只有 3 個元素的
//        vector，成本完全一樣？
//     答：因為 vector 物件本身只有三個指標（begin / end / capacity_end），
//         元素資料全在堆積上。移動就是把這三個指標搬過去、來源設為 nullptr，
//         完全不碰元素。所以成本是固定的 O(1)，與元素數量無關。
//     追問：那 sizeof(std::vector<int>) 和 sizeof(std::vector<BigStruct>) 一樣嗎？
//         → 一樣，本機皆為 24 bytes。元素型別只影響堆積上的內容，不影響物件本身。
//
// 🔥 Q2. std::string 和 std::vector 在「移動優勢」上有什麼關鍵差別？
//     答：string 有 SSO（本機門檻 15 字元），短字串存在物件內部，
//         此時移動與複製幾乎沒有差別；
//         vector 沒有 SSO，只要非空資料就在堆積上，移動永遠是 O(1) 的贏面。
//         所以對 vector 而言不需要先問「夠不夠大」。
//
// ⚠️ 陷阱. 「std::move 之後那個 vector 一定是空的，我可以直接拿 size()==0 來判斷」
//          ——為什麼危險？
//     答：標準只保證移動後的來源處於 valid but unspecified 狀態，
//         可以安全解構、可以重新賦值，但「內容是什麼」不在保證範圍內。
//         主流實作確實會留下空 vector，但這是實作行為不是標準保證。
//         要確保為空，請在移動後明確呼叫 clear()。
//     為什麼會錯：大家把「實測結果一致」當成「標準保證」。
//         這兩件事的差別在換編譯器、換標準函式庫版本時才會暴露出來——
//         而那通常是專案升級時最不想遇到的驚喜。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include <chrono>

class Timer {
    // ⚠️ 成員初始化順序依「宣告順序」：start_ 在前、label_ 在後（-Wreorder）
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

// 防止結果未被使用的迴圈被整段最佳化掉
volatile std::size_t g_sink = 0;

int main() {
    const int N = 10000;
    std::vector<int> source(100000, 42);  // 10 萬個 int

    // ── 計時部分：輸出到 stderr（每次執行都不同）──
    {
        Timer t("複製 vector");
        for (int i = 0; i < N; ++i) {
            std::vector<int> copy = source;
            g_sink += copy.size();
        }
    }

    {
        Timer t("移動 vector");
        for (int i = 0; i < N; ++i) {
            std::vector<int> temp = source;
            std::vector<int> moved = std::move(temp);
            g_sink += moved.size();
        }
    }

    // ── 確定性部分：輸出到 stdout（每次執行完全相同）──
    std::cout << "=== 確定性事實一：vector 物件本身的大小與元素無關 ===\n";
    std::cout << "  sizeof(std::vector<int>)         = "
              << sizeof(std::vector<int>) << " bytes\n";
    std::cout << "  sizeof(std::vector<double>)      = "
              << sizeof(std::vector<double>) << " bytes\n";
    std::cout << "  sizeof(std::vector<std::string>) = "
              << sizeof(std::vector<std::string>) << " bytes\n";
    std::cout << "  → 都是三個指標（begin/end/capacity_end），\n";
    std::cout << "     所以移動只搬這麼多，與元素數量、元素型別都無關\n";

    std::cout << "\n=== 確定性事實二：移動搬走的資料量 vs 複製搬走的資料量 ===\n";
    {
        const std::size_t n = 100000;
        std::vector<int> v(n, 42);

        const std::size_t copyBytes = n * sizeof(int);
        const std::size_t moveBytes = sizeof(std::vector<int>);

        std::cout << "  一個含 " << n << " 個 int 的 vector:\n";
        std::cout << "    複製需搬運 " << copyBytes << " bytes（"
                  << copyBytes / 1024 << " KB，遠超過 L1/L2 快取）\n";
        std::cout << "    移動需搬運 " << moveBytes << " bytes（連一條 "
                  << "cache line 都用不滿）\n";
        std::cout << "    比值 = " << copyBytes / moveBytes << " 倍\n";
    }

    std::cout << "\n=== 確定性事實三：移動不改變元素，只轉移擁有權 ===\n";
    {
        std::vector<int> a{1, 2, 3, 4, 5};
        const int* dataBefore = a.data();      // 記住緩衝區位址（不印出來）

        std::vector<int> b = std::move(a);
        const int* dataAfter = b.data();

        std::cout << "  移動後，新容器指向的是同一塊緩衝區嗎? "
                  << (dataBefore == dataAfter ? "是" : "否") << "\n";
        std::cout << "  → 這證明沒有發生任何元素複製，只是換了擁有者\n";
        std::cout << "  b 的內容 = [";
        for (std::size_t i = 0; i < b.size(); ++i)
            std::cout << b[i] << (i + 1 < b.size() ? "," : "");
        std::cout << "]\n";
        std::cout << "  （來源 a 現在處於 valid but unspecified 狀態，\n";
        std::cout << "    本機實作留下空容器，但標準不保證，故此處不讀取它）\n";
    }

    std::cout << "\n（耗時數字每次執行都不同，已輸出到 stderr，不列入預期輸出）\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.9 章：移動語意的效能分析 — 實測比較與最佳實踐3.cpp" -o move_perf3
// 觀察耗時（stderr）: ./move_perf3 2>&1 >/dev/null

// 註：本檔未附 LeetCode 範例。容器移動成本分析是效能工程議題，
//     LeetCode 只判斷是否通過時限；硬套一題只會失真。
//
// 註：sizeof(std::vector<T>) = 24 bytes 是本機（g++ 15.2 / libstdc++ / x86-64）
//     的「實作定義」值，反映「三個指標」的內部佈局。其他實作可能不同。
//
// 註：「移動後來源是否為空」屬於 valid but unspecified，本檔刻意不讀取來源，
//     以免示範出不可攜的依賴。

// === 預期輸出 ===
// === 確定性事實一：vector 物件本身的大小與元素無關 ===
//   sizeof(std::vector<int>)         = 24 bytes
//   sizeof(std::vector<double>)      = 24 bytes
//   sizeof(std::vector<std::string>) = 24 bytes
//   → 都是三個指標（begin/end/capacity_end），
//      所以移動只搬這麼多，與元素數量、元素型別都無關
//
// === 確定性事實二：移動搬走的資料量 vs 複製搬走的資料量 ===
//   一個含 100000 個 int 的 vector:
//     複製需搬運 400000 bytes（390 KB，遠超過 L1/L2 快取）
//     移動需搬運 24 bytes（連一條 cache line 都用不滿）
//     比值 = 16666 倍
//
// === 確定性事實三：移動不改變元素，只轉移擁有權 ===
//   移動後，新容器指向的是同一塊緩衝區嗎? 是
//   → 這證明沒有發生任何元素複製，只是換了擁有者
//   b 的內容 = [1,2,3,4,5]
//   （來源 a 現在處於 valid but unspecified 狀態，
//     本機實作留下空容器，但標準不保證，故此處不讀取它）
//
// （耗時數字每次執行都不同，已輸出到 stderr，不列入預期輸出）
