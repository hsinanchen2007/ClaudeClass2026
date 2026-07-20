// =============================================================================
//  第 20 課：vector 效能分析與最佳實踐 11  —  批量填充的四種方式
// =============================================================================
//
// 【主題資訊 Information】
//   iterator insert(const_iterator pos, InputIt first, InputIt last);   // 區間插入
//   vector(InputIt first, InputIt last);                                // 區間建構
//   void assign(InputIt first, InputIt last);                           // 整批取代
//   void push_back(const T&);                                           // 逐一附加
//
//   標頭檔：<vector>
//   複雜度：四者皆為 O(n)，差別在「配置次數」與「每元素的固定開銷」
//   本檔標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 四種方式的本質差異】
//   * 逐一 push_back：n 次呼叫，每次都要檢查 size == capacity。
//     若事先 reserve，就沒有重新配置，只剩下 n 次容量檢查。
//   * 批量 insert：一次算出要插入幾個（對 forward iterator 以上），
//     必要時**一次**擴容到位，然後整段複製。只有一次容量檢查。
//   * 區間建構子：連「先建一個空 vector」都省了，直接一次配置正確大小。
//   * assign：與區間建構子類似，但作用在既有物件上，可重用已配置的記憶體。
//
// 【2. 為什麼「一次配置」這麼重要】
//   關鍵不在容量檢查那幾個 CPU 週期，而在**重新配置**。
//   沒有 reserve 的 push_back 迴圈會觸發 log₂(n) 次擴容，
//   每次擴容都要配置新記憶體 + 搬移全部既有元素 + 釋放舊記憶體。
//   本檔第 1 節用計數 allocator 實測這個差距：
//   同樣填入 n 個元素，配置次數可以從 1 次變成十幾次，
//   搬移的元素總量從 n 變成約 2n。
//
// 【3. 對平凡型別的額外優勢：memmove】
//   當元素是 trivially copyable（int、double、POD struct）時，
//   libstdc++ 的區間插入會退化成一次 std::memmove，
//   由 CPU 的 SIMD 指令一次搬多個位元組。
//   逐一 push_back 則是 n 次獨立的賦值，編譯器雖然常能向量化，
//   但仍多了 n 次的邊界檢查邏輯。這是「批量 API」除了少配置之外的第二個好處。
//
// 【4. 該用哪一個】
//   目標容器還不存在        → 區間建構子（最直接，一次到位）
//   目標容器已存在、要換內容 → assign（可重用既有 capacity）
//   目標容器已存在、要附加   → insert(v.end(), first, last)
//   資料是逐步產生的（無法一次拿到）→ reserve + push_back / emplace_back
//   簡言之：**只要來源是「一段已知的區間」，就不要用 push_back 迴圈。**
//
// 【概念補充 Concept Deep Dive】
//   ● iterator category 再次決定效能
//     insert / 區間建構子 / assign 都會先用 std::distance 算長度——
//     但這只對 forward iterator 以上成立。若來源是 input iterator
//     （例如 std::istream_iterator），長度事先算不出來，
//     實作只能退化成「邊讀邊 push_back」，於是又回到多次擴容。
//     這就是為什麼從 stdin 讀資料建 vector 時，仍然建議自己先 reserve 一個估計值。
//
//   ● insert 到「中間」是另一回事
//     insert(v.end(), ...) 只是附加，成本 O(插入量)。
//     insert(v.begin(), ...) 要把既有元素整體往後推，成本 O(n + 插入量)。
//     本檔討論的是前者；在前端批量插入請考慮 deque。
//
//   ● 為什麼本檔不用耗時當主要證據
//     這四種方法對 int 而言差距很小（都是 O(n) 且都會向量化），
//     耗時測量很容易被雜訊淹沒，甚至出現「順序不同結果就不同」的假象。
//     用「配置次數 + 搬移元素數」這種確定性計數，才能穩定呈現真正的差異。
//     耗時仍然照印，但送到 stderr，不列入預期輸出。
//
// 【注意事項 Pay Attention】
//   1. 沒有 reserve 的 push_back 迴圈會觸發多次重新配置，
//      每次都搬移全部既有元素——這是四者中唯一有「額外搬移成本」的。
//   2. reserve 之後 size 仍是 0，不要以為可以直接 v[i] = x（那是 UB）。
//   3. insert 回傳指向「第一個被插入元素」的 iterator；
//      插入後原有的 iterator 全部失效。
//   4. assign 不會縮小 capacity（見同課 9.cpp 的實測）。
//   5. 來源若是 input iterator（如 istream_iterator），
//      批量 API 的「一次配置」優勢不存在，此時請自己 reserve。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】批量填充 vector
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 已經有一個來源區間，要建出一個新 vector，
//        用 push_back 迴圈和用區間建構子差在哪？
//     答：區間建構子能先用 std::distance 算出長度，**一次**配置正確大小；
//         沒有 reserve 的 push_back 迴圈則會觸發約 log₂(n) 次擴容，
//         每次擴容都要搬移全部既有元素，額外搬移總量約為 n。
//         兩者都是 O(n)，但常數差很多。
//     追問：那 reserve + push_back 呢？→ 配置次數就一樣是 1 了，
//         差距縮到只剩「n 次容量檢查」與「能否用 memmove 整段搬」，
//         實務上已經很接近。
//
// 🔥 Q2. 為什麼區間建構子能事先知道長度？
//     答：靠 iterator category。對 forward iterator 以上，
//         std::distance 可以 O(1)（random access）或 O(n) 走訪算出長度，
//         而且**走訪之後還能從頭再來一次**。
//         input iterator 只能走一次（走過就消耗掉了），
//         所以算不出長度，實作只能退化成邊讀邊 push_back。
//     追問：舉個 input iterator 的例子？→ std::istream_iterator。
//         從 cin 建 vector 時就沒有「一次配置」的優勢，建議自己估算並 reserve。
//
// ⚠️ 陷阱. 「我已經 reserve 了，所以用 push_back 迴圈和批量 insert 完全一樣快」
//     答：不完全一樣。reserve 消除了最大的成本（重新配置），
//         但批量 insert 對 trivially copyable 型別還能退化成單次 memmove，
//         而 push_back 迴圈是 n 次獨立操作，每次都有容量檢查。
//         差距不大，但存在。
//     為什麼會錯：把「複雜度相同」誤解成「效能相同」。
//         兩者都是 O(n)，但 O(n) 只描述成長趨勢，不描述常數。
//         真正的重點其實是可讀性：來源是一段區間時，
//         寫 v.insert(v.end(), first, last) 比五行迴圈更清楚地表達了意圖。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <sstream>
#include <iterator>

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
// 可重現的證據：計數 allocator，數「配置次數」與「配置的元素總量」。
// -----------------------------------------------------------------------------
struct AllocStats {
    static long allocs;
    static long elems;
    static void reset() { allocs = 0; elems = 0; }
};
long AllocStats::allocs = 0;
long AllocStats::elems = 0;

template <typename T>
struct CountingAllocator {
    using value_type = T;
    CountingAllocator() noexcept = default;
    template <typename U> CountingAllocator(const CountingAllocator<U>&) noexcept {}

    T* allocate(std::size_t n) {
        ++AllocStats::allocs;
        AllocStats::elems += static_cast<long>(n);
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }
    void deallocate(T* p, std::size_t) noexcept { ::operator delete(p); }

    template <typename U> bool operator==(const CountingAllocator<U>&) const noexcept { return true; }
    template <typename U> bool operator!=(const CountingAllocator<U>&) const noexcept { return false; }
};

using CVec = std::vector<int, CountingAllocator<int>>;

// -----------------------------------------------------------------------------
// 【日常實務範例】合併多個分片的查詢結果
//   情境：分散式查詢把工作切成多個 shard 平行執行，每個 shard 回傳一段結果，
//         協調端要把它們合併成一份完整結果。
//         每個 shard 的結果長度已知，所以應該先算總長度 reserve，
//         再用批量 insert 逐段附加——而不是巢狀迴圈逐一 push_back。
// -----------------------------------------------------------------------------
std::vector<int> mergeShardResults(const std::vector<std::vector<int>>& shards) {
    // 步驟 1：先算總長度（這是批量思維的起點）
    std::size_t total = 0;
    for (const auto& s : shards) total += s.size();

    // 步驟 2：一次配置到位
    std::vector<int> merged;
    merged.reserve(total);

    // 步驟 3：每個 shard 用批量 insert 整段附加
    for (const auto& s : shards) {
        merged.insert(merged.end(), s.begin(), s.end());
    }
    return merged;
}

// 註：本檔不附 LeetCode 範例。「批量填充的四種寫法」是 API 選擇與常數因子議題，
//     LeetCode 判題只看回傳值是否正確，量不到配置次數的差異。
//     相關的區間建構應用已在同課 8.cpp（LeetCode 26）示範過。

int main() {
    std::cout << "=== 1. 可重現證據：各方法的記憶體配置次數 ===\n";
    const std::size_t N = 100000;
    CVec source(N, 42);
    std::cout << "來源 " << N << " 個 int\n\n";

    // (a) 沒有 reserve 的 push_back 迴圈
    AllocStats::reset();
    { CVec v; for (int x : source) v.push_back(x); }
    std::cout << "push_back（無 reserve）：配置 " << AllocStats::allocs
              << " 次，累計 " << AllocStats::elems << " 個元素的空間\n";

    // (b) reserve 之後 push_back
    AllocStats::reset();
    { CVec v; v.reserve(N); for (int x : source) v.push_back(x); }
    std::cout << "push_back（有 reserve）：配置 " << AllocStats::allocs
              << " 次，累計 " << AllocStats::elems << " 個元素的空間\n";

    // (c) 批量 insert
    AllocStats::reset();
    { CVec v; v.reserve(N); v.insert(v.end(), source.begin(), source.end()); }
    std::cout << "reserve + 批量 insert  ：配置 " << AllocStats::allocs
              << " 次，累計 " << AllocStats::elems << " 個元素的空間\n";

    // (d) 區間建構子
    AllocStats::reset();
    { CVec v(source.begin(), source.end()); }
    std::cout << "區間建構子             ：配置 " << AllocStats::allocs
              << " 次，累計 " << AllocStats::elems << " 個元素的空間\n";

    // (e) assign
    AllocStats::reset();
    { CVec v; v.assign(source.begin(), source.end()); }
    std::cout << "assign                 ：配置 " << AllocStats::allocs
              << " 次，累計 " << AllocStats::elems << " 個元素的空間\n";

    std::cout << "\n→ 沒有 reserve 的 push_back 是唯一會多次配置的；\n";
    std::cout << "  它累計要求的空間遠超過 " << N << "，多出來的都是\n";
    std::cout << "  「配置新的、搬過去、丟掉舊的」所浪費的。\n";
    std::cout << "（這組數字每次執行完全相同，比耗時穩定得多）\n";

    std::cout << "\n=== 2. 沒有 reserve 時到底重新配置了幾次 ===\n";
    std::vector<int> grow;
    std::size_t lastCap = grow.capacity();
    int reallocs = 0;
    for (std::size_t i = 0; i < N; ++i) {
        grow.push_back(1);
        if (grow.capacity() != lastCap) { ++reallocs; lastCap = grow.capacity(); }
    }
    std::cout << "填入 " << N << " 個元素，共重新配置 " << reallocs << " 次\n";
    std::cout << "最終 capacity = " << grow.capacity()
              << "（本機 libstdc++ 為 2 倍成長，實作定義）\n";

    std::cout << "\n=== 3. reserve 之後 size 仍然是 0（常見誤解）===\n";
    std::vector<int> r;
    r.reserve(100);
    std::cout << "reserve(100) → size=" << r.size()
              << ", capacity=" << r.capacity() << "\n";
    std::cout << "→ 此時寫 r[0] = 1 是 UB！那塊記憶體上還沒有元素存在。\n";
    std::cout << "  要能用索引存取必須用 resize(100)：";
    r.resize(100);
    std::cout << "size=" << r.size() << "\n";

    std::cout << "\n=== 4. input iterator 沒有「一次配置」的優勢 ===\n";
    std::istringstream iss("1 2 3 4 5 6 7 8 9 10");
    AllocStats::reset();
    // istream_iterator 是 input iterator：長度事先算不出來
    std::vector<int, CountingAllocator<int>> fromStream{
        std::istream_iterator<int>(iss), std::istream_iterator<int>()};
    std::cout << "從 istream 建立 " << fromStream.size() << " 個元素 → 配置 "
              << AllocStats::allocs << " 次\n";
    std::cout << "→ 只有 10 個元素就配置了 " << AllocStats::allocs << " 次，\n";
    std::cout << "  因為 input iterator 只能走一次，長度算不出來，\n";
    std::cout << "  實作只能退化成邊讀邊 push_back。這種來源請自己先 reserve。\n";

    std::cout << "\n=== 5. 原始四方法的實際計時（結果印在 stderr）===\n";
    const int BIG = 100'000;
    std::vector<int> src(BIG, 42);
    {
        std::vector<int> v;
        v.reserve(BIG);
        Timer t("逐一 push_back");
        for (int x : src) v.push_back(x);
    }
    {
        Timer t("批量 insert");
        std::vector<int> v;
        v.reserve(BIG);
        v.insert(v.end(), src.begin(), src.end());
    }
    {
        Timer t("範圍建構子");
        std::vector<int> v(src.begin(), src.end());
    }
    {
        Timer t("assign");
        std::vector<int> v;
        v.assign(src.begin(), src.end());
    }
    std::cout << "四種方法的耗時已印到 stderr。\n";
    std::cout << "⚠️ 這四者都是 O(n) 且差距很小，耗時排名容易被雜訊左右，\n";
    std::cout << "   不同次執行可能得到不同名次——這正是本檔改用配置次數\n";
    std::cout << "   當主要證據的原因。只看 stdout 請用：./demo11 2>/dev/null\n";

    std::cout << "\n=== 日常實務：合併多個分片的查詢結果 ===\n";
    std::vector<std::vector<int>> shards = {
        {101, 102, 103},
        {201, 202},
        {301, 302, 303, 304},
        {},                       // 某個 shard 沒有結果
        {401}
    };
    std::cout << "共 " << shards.size() << " 個 shard，各自筆數：";
    for (const auto& s : shards) std::cout << s.size() << " ";
    std::cout << "\n";

    auto merged = mergeShardResults(shards);
    std::cout << "合併後 " << merged.size() << " 筆：";
    for (int x : merged) std::cout << x << " ";
    std::cout << "\ncapacity = " << merged.capacity()
              << "（等於總筆數 → 全程只配置一次，沒有任何重新配置）\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 20 課：vector 效能分析與最佳實踐11.cpp" -o demo11
// 只看 stdout: ./demo11 2>/dev/null
//
// ⚠️ 但書：
//   1. 第 5 節的耗時印在 stderr，每次執行都不同，故不納入下方預期輸出。
//      該四種方法差距極小，耗時排名不穩定，請以第 1 節的配置次數為準。
//   2. capacity 的成長序列（2 倍）是本機 g++ 15.2 / libstdc++ 的實作行為，
//      非標準保證。

// === 預期輸出 ===
// === 1. 可重現證據：各方法的記憶體配置次數 ===
// 來源 100000 個 int
//
// push_back（無 reserve）：配置 18 次，累計 262143 個元素的空間
// push_back（有 reserve）：配置 1 次，累計 100000 個元素的空間
// reserve + 批量 insert  ：配置 1 次，累計 100000 個元素的空間
// 區間建構子             ：配置 1 次，累計 100000 個元素的空間
// assign                 ：配置 1 次，累計 100000 個元素的空間
//
// → 沒有 reserve 的 push_back 是唯一會多次配置的；
//   它累計要求的空間遠超過 100000，多出來的都是
//   「配置新的、搬過去、丟掉舊的」所浪費的。
// （這組數字每次執行完全相同，比耗時穩定得多）
//
// === 2. 沒有 reserve 時到底重新配置了幾次 ===
// 填入 100000 個元素，共重新配置 18 次
// 最終 capacity = 131072（本機 libstdc++ 為 2 倍成長，實作定義）
//
// === 3. reserve 之後 size 仍然是 0（常見誤解）===
// reserve(100) → size=0, capacity=100
// → 此時寫 r[0] = 1 是 UB！那塊記憶體上還沒有元素存在。
//   要能用索引存取必須用 resize(100)：size=100
//
// === 4. input iterator 沒有「一次配置」的優勢 ===
// 從 istream 建立 10 個元素 → 配置 5 次
// → 只有 10 個元素就配置了 5 次，
//   因為 input iterator 只能走一次，長度算不出來，
//   實作只能退化成邊讀邊 push_back。這種來源請自己先 reserve。
//
// === 5. 原始四方法的實際計時（結果印在 stderr）===
// 四種方法的耗時已印到 stderr。
// ⚠️ 這四者都是 O(n) 且差距很小，耗時排名容易被雜訊左右，
//    不同次執行可能得到不同名次——這正是本檔改用配置次數
//    當主要證據的原因。只看 stdout 請用：./demo11 2>/dev/null
//
// === 日常實務：合併多個分片的查詢結果 ===
// 共 5 個 shard，各自筆數：3 2 4 0 1
// 合併後 10 筆：101 102 103 201 202 301 302 303 304 401
// capacity = 10（等於總筆數 → 全程只配置一次，沒有任何重新配置）
