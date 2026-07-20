// =============================================================================
//  第 20 課：vector 效能分析與最佳實踐 7  —  移動語意與 RVO/NRVO
// =============================================================================
//
// 【主題資訊 Information】
//   vector(vector&& other) noexcept;                    // 移動建構（C++11）
//   vector& operator=(vector&& other) noexcept(...);    // 移動賦值（C++11）
//   std::move(x)  →  <utility>，只是「轉型成右值參考」，本身不搬任何東西
//
//   標頭檔：<vector>、<string>、<utility>
//   複雜度：vector 的移動是 O(1)（只搬三個指標）；複製是 O(n)
//   本檔標準：C++17（保證複製省略 guaranteed copy elision 是 C++17 才入標準）
//
// 【詳細解釋 Explanation】
//
// 【1. vector 的移動為什麼是 O(1)】
//   一個 std::vector 物件本體其實只有三個指標（libstdc++ 的
//   _M_start / _M_finish / _M_end_of_storage，共 24 位元組），
//   真正的元素在 heap 上。移動就是：
//       ① 把這三個指標複製給新物件
//       ② 把來源的三個指標設成 nullptr（讓它解構時不會釋放那塊記憶體）
//   完全不碰元素本身。所以無論 vector 裡有 3 個還是 3 億個元素，
//   移動的成本都一樣——這就是「O(1) 且與 n 無關」的意思。
//
// 【2. RVO / NRVO：比移動更好的「根本不搬」】
//   generate_data() 回傳一個區域變數。你可能以為會發生「建構 → 移動 → 解構」，
//   但編譯器其實會做具名回傳值優化（NRVO）：
//   **直接把 result 建構在呼叫端的目標位置上**，一次移動都不發生。
//     * RVO（回傳臨時物件，如 return std::vector<int>{1,2,3}）：
//       C++17 起是**標準強制**的（guaranteed copy elision），不是最佳化。
//     * NRVO（回傳具名區域變數，如本檔的 return result）：
//       標準**允許但不強制**，g++/clang 在 -O0 也普遍會做，但不保證。
//   本檔第 1 節會用計數器實測本機到底有沒有發生 NRVO。
//
// 【3. 為什麼不要寫 return std::move(result)】
//   這是很常見的「好心辦壞事」。加上 std::move 之後：
//       * 回傳型別變成右值參考運算式 → **NRVO 被禁用**（編譯器不能再省略）
//       * 於是強制發生一次移動建構，反而比原本的「零次」還慢
//   g++ 甚至有 -Wpessimizing-move 專門警告這種寫法。
//   規則很簡單：**回傳區域變數時就直接 return 它，什麼都不要加。**
//
// 【4. moved-from 狀態：合法但未指定】
//   std::move(v1) 之後，v1 仍然是一個合法物件——你可以對它賦值、
//   可以呼叫 clear()、可以解構。但它的**內容是「未指定」的**。
//   對 libstdc++ 的 vector，實測結果是變成空的（size()==0），
//   但標準沒有這個保證。安全的用法只有兩種：
//       ① 不再讀它，直接讓它解構
//       ② 先重新賦值（v1 = something）再使用
//   絕對不要寫「反正 move 完就是空的」這種依賴實作的程式。
//
// 【概念補充 Concept Deep Dive】
//   ● std::move 本身不移動任何東西
//     它只是一個 static_cast<T&&>，是編譯期的型別轉換，執行期零成本。
//     真正做搬移工作的是**被選中的那個移動建構子／移動賦值運算子**。
//     所以 std::move 一個 const 物件會靜默退回複製（因為 const T&& 無法
//     繫結到移動建構子的 T&& 參數，只能繫結到 const T&）——沒有任何警告。
//
//   ● noexcept 為什麼關鍵
//     vector 擴容時，只有元素的移動建構子是 noexcept 才會使用移動；
//     否則為了維持強例外保證，會退回複製。這就是
//     「自訂型別的移動建構子務必標 noexcept」的真正原因。
//     可用 std::is_nothrow_move_constructible_v<T> 檢查。
//
//   ● 移動賦值 vs 移動建構
//     移動**建構**：目標物件還不存在 → 直接接管指標。
//     移動**賦值**：目標物件已存在 → 要先釋放自己原有的記憶體，再接管。
//     所以移動賦值多一次 deallocate，但仍是 O(1)（相對於元素數量）。
//
// 【注意事項 Pay Attention】
//   1. moved-from 的 vector 內容是**未指定**的。libstdc++ 實測為空，
//      但不可依賴——這是實作行為，不是標準保證。
//   2. return std::move(localVar) 會**關掉 NRVO**，讓效能變差。不要寫。
//   3. std::move 一個 const 物件會靜默退回複製，且不會有任何警告。
//   4. 移動不是「一定比較快」——對 std::array<int, 1000> 這種沒有動態配置的
//      型別，移動和複製完全一樣（都是逐元素複製）。
//   5. 本檔的 NRVO 觀測結果來自本機 g++ 15.2；不同編譯器或不同最佳化等級
//      可能不同，這是實作允許的。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】移動語意與回傳值優化
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::vector 的移動建構為什麼是 O(1)？
//     答：vector 物件本體只有三個指標（起點、終點、容量尾端），
//         元素在 heap 上。移動只是複製這三個指標並把來源設為 nullptr，
//         完全不碰元素。所以不論裝 3 個還是 3 億個元素，成本都一樣。
//     追問：那 std::array 的移動也是 O(1) 嗎？→ 不是。array 的元素就在物件本體內，
//         沒有可以「接管」的外部記憶體，移動只能逐元素搬，是 O(n)，
//         和複製完全一樣。
//
// 🔥 Q2. return std::move(localVar) 有什麼問題？
//     答：它會**禁用 NRVO**。原本編譯器可以把區域變數直接建構在呼叫端的
//         目標位置（零次搬移），加了 std::move 之後回傳的是右值參考運算式，
//         省略條件不再成立，強制產生一次移動建構——比原本更慢。
//         g++ 有 -Wpessimizing-move 專門警告這種寫法。
//     追問：那什麼時候該在 return 寫 std::move？→ 當回傳的**不是**區域變數時，
//         例如回傳成員變數 return std::move(this->data_)，
//         或回傳函式參數 return std::move(param)——這些本來就不適用 NRVO。
//
// ⚠️ 陷阱. 「std::move 之後原物件一定變空，所以可以拿 size()==0 來判斷」
//     答：錯。標準只說 moved-from 物件處於「合法但未指定（valid but unspecified）」
//         的狀態。libstdc++ 的 vector 實測是變成空的，但這是實作選擇；
//         換一個標準庫、或換一個型別（例如某些 std::string 的 SSO 實作，
//         短字串 move 完內容可能還在）結果就不同。
//     為什麼會錯：把「我在這台機器上觀察到的行為」當成「語言的保證」。
//         正確心態是：moved-from 物件只能做兩件事——重新賦值，或直接讓它解構。
//         任何「讀取它的內容」的程式碼都是在賭實作細節。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include <type_traits>

// -----------------------------------------------------------------------------
// 儀器型別：計算複製 / 移動各發生幾次。
// 用「計數」而非「計時」當證據，結果每次執行完全相同、可驗證。
// -----------------------------------------------------------------------------
struct Tracked {
    static int copies;
    static int moves;
    static void reset() { copies = 0; moves = 0; }

    std::string name;

    explicit Tracked(std::string n = "") : name(std::move(n)) {}
    Tracked(const Tracked& o) : name(o.name) { ++copies; }
    Tracked(Tracked&& o) noexcept : name(std::move(o.name)) { ++moves; }
    Tracked& operator=(const Tracked& o) { name = o.name; ++copies; return *this; }
    Tracked& operator=(Tracked&& o) noexcept { name = std::move(o.name); ++moves; return *this; }
};
int Tracked::copies = 0;
int Tracked::moves = 0;

std::vector<std::string> generate_data() {
    std::vector<std::string> result;
    result.reserve(3);
    result.emplace_back("alpha");
    result.emplace_back("beta");
    result.emplace_back("gamma");
    return result;  // NRVO（具名回傳值優化）通常會消除拷貝
}

// -----------------------------------------------------------------------------
// 觀察 NRVO 是否發生。
//
// ⚠️ 重要：這裡刻意回傳「單一 Tracked 物件」而不是 vector<Tracked>。
//    原因是我第一版真的寫成回傳 vector<Tracked>，實測結果兩種寫法
//    **都是複製 0 次、移動 0 次**，完全分不出差異——因為 vector 自己的
//    移動建構只搬三個指標，根本不碰元素，元素計數器當然看不到。
//    要觀察「回傳值本身有沒有多一次移動」，被觀測的物件就必須是
//    那個帶計數器的型別本身。這是一個「測量推翻了初稿假設」的實例。
// -----------------------------------------------------------------------------
Tracked makeTrackedNRVO() {
    Tracked result("payload");
    return result;                 // ✅ 直接 return：允許 NRVO
}

// ❌ 反例：多此一舉的 std::move，會禁用 NRVO。
//    g++ 會以 -Wpessimizing-move 警告（這正是本節要示範的重點），
//    此處刻意保留反例，並「局部」關閉該警告讓檔案仍能無警告編譯。
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpessimizing-move"
Tracked makeTrackedPessimized() {
    Tracked result("payload");
    return std::move(result);      // ❌ 禁用 NRVO，強迫一次移動建構
}
#pragma GCC diagnostic pop

// -----------------------------------------------------------------------------
// 【日常實務範例】資料管線：每個階段「接手」上一階段的結果
//   情境：ETL / 影像處理 / 編譯器 pass 這類管線，每一階段都產生一份新資料
//         並交給下一階段。若每一階段都複製，n 個階段就複製 n 次；
//         用移動語意「接手」則全程零複製，資料只有一份從頭走到尾。
//   關鍵寫法：參數收值（sink parameter）+ 呼叫端 std::move + 直接 return。
// -----------------------------------------------------------------------------
std::vector<std::string> stageTrim(std::vector<std::string> lines) {
    for (auto& s : lines) {
        std::size_t b = s.find_first_not_of(" \t");
        std::size_t e = s.find_last_not_of(" \t");
        s = (b == std::string::npos) ? std::string{} : s.substr(b, e - b + 1);
    }
    return lines;                  // 直接 return，讓編譯器省略
}

std::vector<std::string> stageDropEmpty(std::vector<std::string> lines) {
    std::vector<std::string> out;
    out.reserve(lines.size());
    for (auto& s : lines) {
        if (!s.empty()) out.push_back(std::move(s));   // 移動，不複製字串
    }
    return out;
}

std::vector<std::string> stageAddPrefix(std::vector<std::string> lines,
                                        const std::string& prefix) {
    for (auto& s : lines) s.insert(0, prefix);
    return lines;
}

// 註：本檔不附 LeetCode 範例。移動語意與 RVO 屬於「物件生命週期與成本」議題，
//     LeetCode 只驗證回傳值正確與否，量不到搬移次數，也不會考這個設計選擇。

int main() {
    std::cout << "=== 1. 原始示範：NRVO 與移動賦值 ===\n";
    // 情況一：函數回傳值（通常 NRVO 會優化，不需要手動 move）
    std::vector<std::string> v1 = generate_data();

    // 情況二：明確轉移一個已存在的 vector
    std::vector<std::string> v2;
    v2 = std::move(v1);
    // v1 現在是「合法但未指定狀態」（通常是空的）

    std::cout << "v1.size() = " << v1.size() << std::endl;   // 0
    std::cout << "v2.size() = " << v2.size() << std::endl;   // 3

    for (const auto& s : v2) {
        std::cout << s << " ";
    }
    std::cout << std::endl;
    std::cout << "⚠️ v1.size() 顯示 0 是 libstdc++ 的實測結果；\n";
    std::cout << "   標準只保證 moved-from 是「合法但未指定」，不保證一定是 0。\n";

    std::cout << "\n=== 2. 實測：NRVO 到底有沒有發生（用計數，非計時）===\n";
    Tracked::reset();
    {
        Tracked got = makeTrackedNRVO();
        std::cout << "直接 return result       → 複製 " << Tracked::copies
                  << " 次、移動 " << Tracked::moves << " 次"
                  << "（內容 " << got.name << "）\n";
    }

    Tracked::reset();
    {
        Tracked got = makeTrackedPessimized();
        std::cout << "return std::move(result) → 複製 " << Tracked::copies
                  << " 次、移動 " << Tracked::moves << " 次"
                  << "（內容 " << got.name << "）\n";
    }
    std::cout << "→ 直接 return：NRVO 生效，物件直接建構在呼叫端，零次搬移。\n";
    std::cout << "  加上 std::move：省略條件被破壞，多出一次移動建構。\n";
    std::cout << "  這就是「好心加 std::move 反而變慢」的實證，\n";
    std::cout << "  g++ 也會用 -Wpessimizing-move 主動警告這種寫法。\n";
    std::cout << "\n  📌 附帶一課：若把被觀測型別換成 vector<Tracked>，兩種寫法\n";
    std::cout << "     都會顯示 0 次複製 0 次移動——因為 vector 的移動只搬指標、\n";
    std::cout << "     不碰元素，元素計數器根本觀測不到。選對觀測對象很重要。\n";

    std::cout << "\n=== 3. 移動是 O(1)：與元素數量完全無關 ===\n";
    std::vector<int> huge(5'000'000, 7);
    const int* dataBefore = huge.data();
    std::size_t sizeBefore = huge.size();

    std::vector<int> taken = std::move(huge);      // 500 萬個元素，仍是 O(1)

    std::cout << "移動前 size = " << sizeBefore << "\n";
    std::cout << "移動後 taken.size() = " << taken.size() << "\n";
    std::cout << "底層緩衝區是否為同一塊？ " << std::boolalpha
              << (taken.data() == dataBefore)
              << " ← true 代表記憶體被「接管」，沒有複製任何元素\n";
    std::cout << "（500 萬個 int = 約 "
              << (sizeBefore * sizeof(int) / 1024 / 1024)
              << " MB，移動卻只搬了 3 個指標）\n";

    std::cout << "\n=== 4. std::move 對 const 物件會靜默退回複製 ===\n";
    Tracked::reset();
    {
        const Tracked src("const-source");
        Tracked dst = std::move(src);              // 看起來是移動，其實是複製！
        std::cout << "std::move(const 物件) → 複製 " << Tracked::copies
                  << " 次、移動 " << Tracked::moves << " 次\n";
        std::cout << "dst.name = " << dst.name
                  << "（來源是 const，移動建構子繫結不了，靜默退回複製）\n";
    }

    Tracked::reset();
    {
        Tracked src("mutable-source");
        Tracked dst = std::move(src);              // 這次才是真的移動
        std::cout << "std::move(非 const 物件) → 複製 " << Tracked::copies
                  << " 次、移動 " << Tracked::moves << " 次\n";
        std::cout << "dst.name = " << dst.name << "\n";
    }

    std::cout << "\n=== 5. noexcept 決定擴容時是移動還是複製 ===\n";
    std::cout << "Tracked 的移動建構子是 noexcept 嗎？ "
              << std::is_nothrow_move_constructible_v<Tracked> << "\n";
    std::cout << "std::string 呢？ "
              << std::is_nothrow_move_constructible_v<std::string> << "\n";
    std::cout << "→ 若為 false，vector 擴容時會為了強例外保證而改用複製，\n";
    std::cout << "  效能可能因此大幅下降——這是自訂型別最容易忽略的一點。\n";

    std::cout << "\n=== 日常實務：三階段資料管線，全程零字串複製 ===\n";
    std::vector<std::string> raw = {
        "  ERROR: disk full  ",
        "   ",
        "  WARN: high latency",
        "",
        "INFO: started  "
    };
    std::cout << "原始 " << raw.size() << " 行\n";

    // 每一階段都用 std::move 交棒，資料只有一份
    auto trimmed  = stageTrim(std::move(raw));
    auto nonEmpty = stageDropEmpty(std::move(trimmed));
    auto final    = stageAddPrefix(std::move(nonEmpty), "[log] ");

    std::cout << "處理後 " << final.size() << " 行：\n";
    for (const auto& s : final) std::cout << "  " << s << "\n";
    std::cout << "→ 三個階段之間沒有任何一次 vector 或 string 的深複製，\n";
    std::cout << "  每一步都是「接管上一步的緩衝區」。\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 20 課：vector 效能分析與最佳實踐7.cpp" -o demo7
//
// ⚠️ 但書：
//   1. 第 1 節的 v1.size() = 0 是 libstdc++ 對 moved-from vector 的實測結果，
//      標準只保證「合法但未指定」，不保證為 0。
//   2. 第 2 節的計數反映本機 g++ 15.2 的省略行為；NRVO 標準允許但不強制，
//      其他編譯器或最佳化等級可能不同。

// === 預期輸出 ===
// === 1. 原始示範：NRVO 與移動賦值 ===
// v1.size() = 0
// v2.size() = 3
// alpha beta gamma
// ⚠️ v1.size() 顯示 0 是 libstdc++ 的實測結果；
//    標準只保證 moved-from 是「合法但未指定」，不保證一定是 0。
//
// === 2. 實測：NRVO 到底有沒有發生（用計數，非計時）===
// 直接 return result       → 複製 0 次、移動 0 次（內容 payload）
// return std::move(result) → 複製 0 次、移動 1 次（內容 payload）
// → 直接 return：NRVO 生效，物件直接建構在呼叫端，零次搬移。
//   加上 std::move：省略條件被破壞，多出一次移動建構。
//   這就是「好心加 std::move 反而變慢」的實證，
//   g++ 也會用 -Wpessimizing-move 主動警告這種寫法。
//
//   📌 附帶一課：若把被觀測型別換成 vector<Tracked>，兩種寫法
//      都會顯示 0 次複製 0 次移動——因為 vector 的移動只搬指標、
//      不碰元素，元素計數器根本觀測不到。選對觀測對象很重要。
//
// === 3. 移動是 O(1)：與元素數量完全無關 ===
// 移動前 size = 5000000
// 移動後 taken.size() = 5000000
// 底層緩衝區是否為同一塊？ true ← true 代表記憶體被「接管」，沒有複製任何元素
// （500 萬個 int = 約 19 MB，移動卻只搬了 3 個指標）
//
// === 4. std::move 對 const 物件會靜默退回複製 ===
// std::move(const 物件) → 複製 1 次、移動 0 次
// dst.name = const-source（來源是 const，移動建構子繫結不了，靜默退回複製）
// std::move(非 const 物件) → 複製 0 次、移動 1 次
// dst.name = mutable-source
//
// === 5. noexcept 決定擴容時是移動還是複製 ===
// Tracked 的移動建構子是 noexcept 嗎？ true
// std::string 呢？ true
// → 若為 false，vector 擴容時會為了強例外保證而改用複製，
//   效能可能因此大幅下降——這是自訂型別最容易忽略的一點。
//
// === 日常實務：三階段資料管線，全程零字串複製 ===
// 原始 5 行
// 處理後 3 行：
//   [log] ERROR: disk full
//   [log] WARN: high latency
//   [log] INFO: started
// → 三個階段之間沒有任何一次 vector 或 string 的深複製，
//   每一步都是「接管上一步的緩衝區」。
