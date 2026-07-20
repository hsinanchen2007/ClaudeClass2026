// =============================================================================
//  第 20 課：vector 效能分析與最佳實踐 6  —  值傳遞 vs const 引用
// =============================================================================
//
// 【主題資訊 Information】
//   long long sum_by_value(std::vector<int> v);          // 值傳遞：整份複製
//   long long sum_by_ref(const std::vector<int>& v);     // const 引用：零複製
//
//   標頭檔：<vector>、<numeric>（accumulate）、<chrono>（計時）
//   複雜度：值傳遞每次呼叫 O(n) 額外成本（配置 + 複製 + 釋放）；
//           const 引用為 O(1)（只傳一個位址）
//   本檔標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 值傳遞到底付出了什麼代價】
//   sum_by_value(data) 每次呼叫都會做完整的三件事：
//       ① 向 heap 要一塊 n * sizeof(int) 的新記憶體（一次 operator new）
//       ② 把 n 個元素逐一複製過去（對 int 是 memcpy，對 std::string 是 n 次深複製）
//       ③ 函式結束時解構，釋放那塊記憶體（一次 operator delete）
//   對 1,000,000 個 int，這是每次呼叫 4 MB 的配置 + 複製 + 釋放。
//   呼叫 100 次就是 400 MB 的記憶體流量——而函式其實只是「讀」而已。
//
// 【2. const 引用為什麼零成本】
//   引用在底層就是一個位址（本機 x86-64 上是 8 位元組，通常直接放暫存器）。
//   const 保證函式不會修改來源，呼叫端因此可以安心傳入自己的資料。
//   沒有配置、沒有複製、沒有釋放——成本與資料量完全無關。
//
// 【3. 什麼時候「應該」用值傳遞】
//   值傳遞不是永遠錯的。當函式**需要一份自己的副本**時，值傳遞反而是最好的寫法：
//       std::vector<int> sorted_copy(std::vector<int> v) {   // 刻意收值
//           std::sort(v.begin(), v.end());
//           return v;                                        // 直接回傳，可被移動
//       }
//   這樣寫的好處是：呼叫端傳左值就複製、傳右值（臨時物件、std::move）就**移動**，
//   一個函式同時涵蓋兩種情境，這就是所謂的 "sink parameter" 慣用法。
//   反例才是本檔要罵的：**只讀取卻收值**。
//
// 【4. 決策表】
//   只讀取                → const std::vector<T>&
//   要修改呼叫端的資料    → std::vector<T>&
//   需要自己的副本        → std::vector<T>（值傳遞，讓呼叫端決定複製或移動）
//   只讀且可能傳子區間    → std::span<const T>（C++20）
//
// 【概念補充 Concept Deep Dive】
//   ● 為什麼編譯器不能自動幫你優化掉這次複製
//     因為值傳遞是**語意的一部分**：函式拿到的是獨立副本，
//     它有權修改而不影響呼叫端。編譯器必須保證這個語意。
//     只有當它能證明「複製與否觀察不到差異」時（例如函式被 inline、
//     且沒有任何修改與位址逃逸）才可能消除——對本檔這種跨越 100 次迴圈、
//     且元素數量龐大的情況，通常無法消除。
//
//   ● 為什麼本檔的 stdout 不放耗時
//     耗時受 CPU 頻率、快取狀態、其他行程干擾影響，每次執行都不同，
//     不適合當「預期輸出」。所以本檔把**耗時印到 stderr**（給你臨場觀察），
//     把**可重現的「配置次數 / 複製元素數」印到 stdout**（當作教材證據）。
//     計數比計時更能說明複雜度差異，也不會因機器不同而失效。
//
//   ● -Wreorder：本檔修正過的一個真實警告
//     原始版本的 Timer 成員宣告順序是 start_、label_，
//     但初始化列表寫成 label_(label), start_(now())。
//     C++ 規定成員一律**依宣告順序**初始化，與初始化列表的書寫順序無關，
//     所以實際順序是 start_ 先、label_ 後——與你寫的順序不同。
//     這次剛好無害（兩者互不依賴），但若 start_ 的初始化用到 label_ 就是讀取
//     未初始化成員。g++ -Wall 會以 -Wreorder 警告，本檔已把宣告順序調成一致。
//
// 【注意事項 Pay Attention】
//   1. 「只讀卻用值傳遞」是最常見的效能 bug，而且編譯器不會警告。
//   2. const 引用不代表資料不會變——若別處有非 const 的別名同時修改它，
//      函式看到的內容仍可能改變（const 是對「你」的限制，不是對資料的保證）。
//   3. 對小型型別（int、pointer、std::string_view 這類 ≤ 兩個暫存器的）
//      值傳遞反而更快，不要無腦對所有東西加 const&。
//   4. 成員初始化順序永遠依**宣告順序**，不是初始化列表的書寫順序。
//   5. 本檔 stderr 的耗時每次執行都不同，屬於正常現象。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】參數傳遞方式的選擇
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. void f(std::vector<int> v) 和 void f(const std::vector<int>& v)
//        差在哪？各自什麼時候用？
//     答：前者每次呼叫都完整複製一份（heap 配置 + 逐元素複製 + 解構釋放），
//         成本 O(n)；後者只傳一個位址，成本 O(1)。
//         只讀取 → 用 const&；需要一份可修改的副本 → 才用值傳遞。
//     追問：值傳遞有沒有「正確」的用法？→ 有，就是 sink parameter：
//         函式本來就要拿走一份副本時收值，呼叫端傳右值即可觸發移動而非複製，
//         一個簽名同時涵蓋複製與移動兩種情境。
//
// 🔥 Q2. 那是不是所有參數都該加 const&？
//     答：不是。對 int、double、指標、std::string_view 這類「不大於兩個暫存器」
//         的型別，值傳遞可以直接放暫存器，比多一層間接定址更快，
//         而且不會有 aliasing 疑慮。const& 是給「複製成本高」的型別用的。
//     追問：怎麼判斷界線？→ 經驗法則是超過 16 位元組（兩個暫存器）或
//         擁有動態配置資源的型別就用 const&，其餘傳值。
//
// ⚠️ 陷阱. 「加了 const& 就是零成本，所以一定比較快」——什麼情況下反而更慢？
//     答：兩種情況。(a) 小型別：多一層指標間接定址，還可能因為
//         編譯器無法確定沒有別名（aliasing）而放棄某些最佳化。
//         (b) 函式內部反正要做一份副本：寫成 const& 再在函式內複製，
//         等於強制複製；寫成值傳遞則讓呼叫端能用 std::move 省掉那次複製。
//     為什麼會錯：把 const& 當成「一律更快的萬靈丹」，
//         而不是「避免大型物件複製」這個特定問題的解法。
//         真正的判準是「這個函式需不需要一份自己的資料」，
//         而不是「引用比值快」這種無條件的宣稱。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <numeric>    // std::accumulate
#include <algorithm>  // std::sort

// -----------------------------------------------------------------------------
// RAII 計時器。
// 【修正 1】成員宣告順序改成與初始化列表一致（原版觸發 -Wreorder 警告）。
// 【修正 2】結果印到 std::cerr 而非 std::cout——耗時每次執行都不同，
//           不應該混進當作教材驗證基準的 stdout。
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
        // 印到 stderr：不影響 stdout 的可重現性
        std::cerr << "  [計時] " << label_ << ": " << us << " μs" << std::endl;
    }
};

// -----------------------------------------------------------------------------
// 可重現的證據：用一個會自報行蹤的 allocator 數「配置次數與配置元素總數」。
// 比計時更能證明複雜度差異，而且每次執行結果完全相同。
// -----------------------------------------------------------------------------
struct AllocStats {
    static long allocCount;      // operator new 被呼叫幾次
    static long elemsAllocated;  // 累計配置了多少個元素
    static void reset() { allocCount = 0; elemsAllocated = 0; }
};
long AllocStats::allocCount = 0;
long AllocStats::elemsAllocated = 0;

template <typename T>
struct CountingAllocator {
    using value_type = T;

    CountingAllocator() noexcept = default;
    template <typename U>
    CountingAllocator(const CountingAllocator<U>&) noexcept {}

    T* allocate(std::size_t n) {
        ++AllocStats::allocCount;
        AllocStats::elemsAllocated += static_cast<long>(n);
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }
    void deallocate(T* p, std::size_t) noexcept { ::operator delete(p); }

    template <typename U>
    bool operator==(const CountingAllocator<U>&) const noexcept { return true; }
    template <typename U>
    bool operator!=(const CountingAllocator<U>&) const noexcept { return false; }
};

using CountedVec = std::vector<int, CountingAllocator<int>>;

// 錯誤：參數用值傳遞，每次呼叫都會拷貝整個 vector
long long sum_by_value(std::vector<int> v) {
    return std::accumulate(v.begin(), v.end(), 0LL);
}

// 正確：參數用 const 引用，零拷貝
long long sum_by_ref(const std::vector<int>& v) {
    return std::accumulate(v.begin(), v.end(), 0LL);
}

// 同樣兩個版本，但用會計數的 allocator，好取得可重現的證據
long long counted_by_value(CountedVec v) {
    return std::accumulate(v.begin(), v.end(), 0LL);
}
long long counted_by_ref(const CountedVec& v) {
    return std::accumulate(v.begin(), v.end(), 0LL);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】報表服務：同一份交易紀錄要餵給多個統計函式
//   情境：後端服務收到一批交易明細，接著要算總額、算平均、找最大值、
//         做風控檢查……每個統計函式都只是「讀」。
//         若這些函式全用值傳遞，同一份資料會被完整複製 N 次，
//         在高流量服務上這是很典型的效能問題（且 profiler 上只會看到
//         memcpy 與 operator new 很忙，不容易一眼看出根因）。
// -----------------------------------------------------------------------------
struct TxStats {
    long long total = 0;
    double average = 0.0;
    int maxAmount = 0;
    int suspicious = 0;      // 超過門檻的可疑交易筆數
};

// 全部用 const& —— 四個函式共用同一份資料，零複製
static long long calcTotal(const std::vector<int>& tx) {
    return std::accumulate(tx.begin(), tx.end(), 0LL);
}
static double calcAverage(const std::vector<int>& tx) {
    return tx.empty() ? 0.0
                      : static_cast<double>(calcTotal(tx)) / static_cast<double>(tx.size());
}
static int calcMax(const std::vector<int>& tx) {
    int m = 0;
    for (int a : tx) if (a > m) m = a;
    return m;
}
static int countSuspicious(const std::vector<int>& tx, int threshold) {
    int c = 0;
    for (int a : tx) if (a > threshold) ++c;
    return c;
}

TxStats analyze(const std::vector<int>& transactions, int threshold) {
    TxStats s;
    s.total      = calcTotal(transactions);
    s.average    = calcAverage(transactions);
    s.maxAmount  = calcMax(transactions);
    s.suspicious = countSuspicious(transactions, threshold);
    return s;
}

// 註：本檔不附 LeetCode 範例。「參數傳遞方式」是 API 設計與效能議題，
//     LeetCode 的函式簽名由平台固定給定（通常已經是 vector<int>&），
//     不會考這個選擇；硬掛一題只會模糊重點。

int main() {
    std::cout << "=== 1. 可重現證據：記憶體配置次數（用計數 allocator）===\n";
    const std::size_t N = 100000;
    const int ROUNDS = 100;

    CountedVec counted(N, 1);
    std::cout << "資料量 " << N << " 個 int，各呼叫 " << ROUNDS << " 次\n";

    // 值傳遞：每次呼叫都要配置一塊新記憶體
    AllocStats::reset();
    {
        Timer t("值傳遞（拷貝）");
        for (int i = 0; i < ROUNDS; ++i) counted_by_value(counted);
    }
    long valueAllocs = AllocStats::allocCount;
    long valueElems  = AllocStats::elemsAllocated;

    // const 引用：完全不配置
    AllocStats::reset();
    {
        Timer t("const 引用（零拷貝）");
        for (int i = 0; i < ROUNDS; ++i) counted_by_ref(counted);
    }
    long refAllocs = AllocStats::allocCount;
    long refElems  = AllocStats::elemsAllocated;

    std::cout << "值傳遞    ：配置 " << valueAllocs << " 次，共 "
              << valueElems << " 個元素\n";
    std::cout << "const 引用：配置 " << refAllocs << " 次，共 "
              << refElems << " 個元素\n";
    std::cout << "→ 值傳遞每呼叫一次就配置一次（" << ROUNDS
              << " 次呼叫 = " << valueAllocs << " 次配置），\n";
    std::cout << "   搬動了 " << (valueElems * static_cast<long>(sizeof(int)) / 1024 / 1024)
              << " MB 的資料，而這個函式其實只是「讀」而已。\n";
    std::cout << "（這組數字每次執行完全相同，不像耗時會浮動）\n";

    std::cout << "\n=== 2. 一般 vector 的實際計時（結果印在 stderr）===\n";
    std::vector<int> data(1'000'000, 1);
    long long r1 = 0, r2 = 0;

    {
        Timer t("值傳遞（拷貝）1,000,000 個 int × 100 次");
        for (int i = 0; i < 100; ++i) {
            r1 += sum_by_value(data);
        }
    }

    {
        Timer t("const 引用（零拷貝）1,000,000 個 int × 100 次");
        for (int i = 0; i < 100; ++i) {
            r2 += sum_by_ref(data);
        }
    }

    std::cout << "兩者計算結果相同：" << std::boolalpha << (r1 == r2)
              << "（值 = " << r1 << "）\n";
    std::cout << "耗時已印到 stderr——每次執行都不同，故不列入預期輸出。\n";
    std::cout << "想看計時請直接執行；想只看 stdout 請用 2>/dev/null。\n";

    std::cout << "\n=== 3. 值傳遞「正確」的用法：sink parameter ===\n";
    // 收值 → 呼叫端傳右值時會觸發移動，不是複製
    auto sortedCopy = [](std::vector<int> v) {
        std::sort(v.begin(), v.end());
        return v;
    };
    std::vector<int> raw = {5, 2, 9, 1, 7};
    auto sorted = sortedCopy(std::move(raw));    // 傳右值 → 移動，零複製
    std::cout << "排序結果：";
    for (int x : sorted) std::cout << x << " ";
    std::cout << "\n（傳 std::move(raw) → vector 的緩衝區被直接接管，沒有複製元素）\n";
    std::cout << "raw 現在處於「合法但未指定」狀態，size = " << raw.size()
              << "（libstdc++ 實測為 0，非標準保證）\n";

    std::cout << "\n=== 日常實務：交易報表的多重統計（全部共用一份資料）===\n";
    std::vector<int> transactions = {1200, 350, 89000, 4500, 120, 76000, 2300, 640};
    TxStats stats = analyze(transactions, 50000);

    std::cout << "交易筆數  ：" << transactions.size() << "\n";
    std::cout << "總金額    ：" << stats.total << "\n";
    std::cout << "平均金額  ：" << stats.average << "\n";
    std::cout << "最大單筆  ：" << stats.maxAmount << "\n";
    std::cout << "可疑交易  ：" << stats.suspicious << " 筆（門檻 50000）\n";
    std::cout << "→ 四個統計函式全用 const&，同一份資料從頭到尾只有一份，\n";
    std::cout << "  若改成值傳遞，這裡會多出 4 次完整複製（巢狀呼叫更多）。\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 20 課：vector 效能分析與最佳實踐6.cpp" -o demo6
// 只看 stdout: ./demo6 2>/dev/null
//
// ⚠️ 但書：
//   1. 「=== 2. ===」段的耗時印在 stderr，每次執行都不同（受 CPU 頻率、
//      快取、其他行程影響），故不納入下方預期輸出。
//   2. 「raw.size() = 0」是 libstdc++ 對 moved-from vector 的實測結果；
//      標準只保證「合法但未指定的狀態」，不保證一定是 0。

// === 預期輸出 ===
// === 1. 可重現證據：記憶體配置次數（用計數 allocator）===
// 資料量 100000 個 int，各呼叫 100 次
// 值傳遞    ：配置 100 次，共 10000000 個元素
// const 引用：配置 0 次，共 0 個元素
// → 值傳遞每呼叫一次就配置一次（100 次呼叫 = 100 次配置），
//    搬動了 38 MB 的資料，而這個函式其實只是「讀」而已。
// （這組數字每次執行完全相同，不像耗時會浮動）
//
// === 2. 一般 vector 的實際計時（結果印在 stderr）===
// 兩者計算結果相同：true（值 = 100000000）
// 耗時已印到 stderr——每次執行都不同，故不列入預期輸出。
// 想看計時請直接執行；想只看 stdout 請用 2>/dev/null。
//
// === 3. 值傳遞「正確」的用法：sink parameter ===
// 排序結果：1 2 5 7 9
// （傳 std::move(raw) → vector 的緩衝區被直接接管，沒有複製元素）
// raw 現在處於「合法但未指定」狀態，size = 0（libstdc++ 實測為 0，非標準保證）
//
// === 日常實務：交易報表的多重統計（全部共用一份資料）===
// 交易筆數  ：8
// 總金額    ：174110
// 平均金額  ：21763.8
// 最大單筆  ：89000
// 可疑交易  ：2 筆（門檻 50000）
// → 四個統計函式全用 const&，同一份資料從頭到尾只有一份，
//   若改成值傳遞，這裡會多出 4 次完整複製（巢狀呼叫更多）。
