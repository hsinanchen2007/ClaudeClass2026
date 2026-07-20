// =============================================================================
//  第 2.9 章 4  —  移動與資料量的關係：為什麼「加速比」會隨 size 一直變大
// =============================================================================
//
// 【主題資訊 Information】
//   被測對象 : std::vector<int> 的複製建構 vs 移動建構
//   標頭檔   : <vector>, <utility>, <new>, <cstdlib>, <chrono>
//   複雜度   : 複製為 O(n)（一次配置 + n 個元素的拷貝）
//              移動為 O(1)（只搬 _begin / _end / _cap 三個指標）
//   實測     : sizeof(std::vector<int>) = 24 bytes（本機，實作定義）
//
// 【詳細解釋 Explanation】
//
// 【1. 本檔要證明的核心事實】
//   複製 vector 的成本與元素個數成正比；移動的成本是常數。
//   所以兩者的差距不是一個固定倍數，而是「隨資料量線性擴大」。
//   size 從 100 增加到 1,000,000（一萬倍），複製要拷貝的位元組也變成一萬倍，
//   移動卻始終只搬三個指標。這才是移動語意真正的價值所在。
//
// 【2. 原始寫法為什麼量不出這件事（重要）】
//   常見的比較是這樣寫的：
//       for (i<N) { std::vector<int> copy = source; }                  // 複製組
//       for (i<N) { std::vector<int> temp = source;                    // ← 這行是複製！
//                   std::vector<int> moved = std::move(temp); }        // 才是移動
//   「移動組」每輪其實是「一次完整複製 + 一次移動」，工作量比複製組還多。
//   於是量出來的「加速比」永遠接近甚至小於 1，完全無法呈現真實差距。
//   更糟的是，這個錯誤會讓人得出「移動語意沒什麼用」的相反結論。
//
// 【3. 本檔改用什麼證據】
//   改量「堆積配置次數」與「拷貝的位元組數」——這兩個量是確定性的，
//   可重現，而且直接對應成本發生的機制：
//       複製 vector<int>(size) → 1 次配置，size * 4 bytes 的拷貝
//       移動 vector<int>       → 0 次配置，0 bytes 拷貝
//   而且測量方式是公平的：兩組的前置成本完全相同（都先建一份 temp），
//   只在「被測操作」前後取計數快照，差額就純粹是該操作本身的成本。
//
// 【4. 牆鐘時間該怎麼看待】
//   時間仍有參考價值，但它不可重現（受 CPU 頻率調節、快取狀態、
//   其他行程干擾影響），不能寫進「預期輸出」。本檔把它輸出到 stderr，
//   stdout 只保留確定性的計數。
//
// 【概念補充 Concept Deep Dive】
//   vector 的移動之所以是 O(1)，是因為它本身只是三個指標（_begin / _end /
//   _cap，本機 sizeof 為 24 bytes）。移動就是把這三個指標搬過去、
//   再把來源清空——堆積上那塊資料完全沒有被碰到，只是換了個擁有者。
//   複製則必須向配置器要一塊同樣大的新記憶體，再把每個元素拷貝過去。
//   對 int 這種 trivially copyable 的型別，實作會用 memmove 一次搬完，
//   仍然是 O(n)；對非 trivial 的元素型別還要逐一呼叫複製建構子，更慢。
//
// 【注意事項 Pay Attention】
// 1. 「加速比」這個說法本身就有誤導性——它不是常數，會隨資料量成長。
//    正確的描述是「複製是 O(n)、移動是 O(1)」。
// 2. 量測程式必須阻止編譯器最佳化掉被測程式碼，否則量到的是空迴圈。
//    本檔用 asm volatile 記憶體屏障處理。
// 3. 配置次數雖為確定性，仍取決於標準庫實作；本機為 g++ 15.2 / libstdc++。
// 4. 移動之後來源 vector 處於「有效但未指定」狀態。實務上 libstdc++ 會讓它
//    變成空的，但那是實作行為，本檔不對它的內容做任何斷言。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】移動成本與資料量的關係
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 移動 vector 比複製快幾倍？
//     答：這個問題本身問錯了。移動是 O(1)（只搬三個指標），複製是 O(n)
//         （一次配置加上 n 個元素的拷貝），所以差距隨元素個數線性擴大，
//         沒有固定倍數。100 個元素時差距不大，一百萬個元素時差距是四個
//         數量級。正確的回答是說出兩者的複雜度，而不是給一個數字。
//     追問：那什麼時候移動不會比較快？→ 容器為空或極小時，
//         以及元素本身沒有堆積資源時（例如 SSO 短字串、int）。
//
// 🔥 Q2. 為什麼 vector 的移動可以是 O(1)？
//     答：因為 vector 物件本身只有三個指標（_begin / _end / _cap，
//         本機 sizeof 為 24 bytes）。移動就是搬走這三個指標並把來源清空，
//         堆積上的資料完全沒被碰過，只是換了擁有者。
//     追問：那 std::array 呢？→ std::array 的資料直接內嵌在物件裡，
//         沒有指標可搬，移動就是逐元素移動，仍是 O(n)。
//
// ⚠️ 陷阱. 有人量出「移動比複製還慢」，於是主張移動語意是行銷話術。
//          他的量測錯在哪？
//     答：幾乎一定是「移動組」裡混進了複製。為了準備一個可被移動的來源，
//         常見寫法會先 std::vector<int> temp = source;——那行本身就是
//         一次完整的 O(n) 複製，而且被算進了計時區間。於是移動組做的是
//         「複製 + 移動」，比複製組還多，量出來當然更慢。
//     為什麼會錯：注意力全放在「被測的那一行」，忽略了同一個計時區間裡的
//         前置準備。量測的鐵律是：計時區間內只能有你要量的那個操作，
//         前置成本必須排除在外，或在兩組之間完全一致。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】從缺。
//   理由：本檔主題是效能量測方法論與移動／複製的成本模型。LeetCode 判定的
//   是演算法正確性，不會因為多一次容器複製而失敗；清單中也沒有任何一題
//   以「資源搬移成本」為核心。硬套一題只會模糊焦點，故從缺。

#include <iostream>
#include <vector>
#include <utility>
#include <chrono>
#include <new>
#include <cstdlib>

// -----------------------------------------------------------------------------
// 堆積配置計數器：覆寫全域 operator new / delete（標準允許的替換函式）
//   用確定性的「配置次數 + 位元組數」取代不可重現的牆鐘計時。
//   C++14 起需一併提供 sized deallocation。本檔為單執行緒，未加同步。
// -----------------------------------------------------------------------------
static long g_alloc_count = 0;
static long long g_alloc_bytes = 0;

void* operator new(std::size_t n) {
    ++g_alloc_count;
    g_alloc_bytes += static_cast<long long>(n);
    void* p = std::malloc(n);
    if (!p) throw std::bad_alloc();
    return p;
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void* operator new[](std::size_t n) {
    ++g_alloc_count;
    g_alloc_bytes += static_cast<long long>(n);
    void* p = std::malloc(n);
    if (!p) throw std::bad_alloc();
    return p;
}
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

// 阻止編譯器把沒有可觀察副作用的被測程式碼整段刪掉
template <typename T>
static inline void doNotOptimize(T& value) {
    asm volatile("" : "+m"(value) : : "memory");
}

// Timer 保留供參考用，但輸出改到 stderr——耗時每次執行都不同，
// 不該進入 stdout 的預期輸出。
class Timer {
    std::chrono::steady_clock::time_point start_;
    const char* label_;
public:
    explicit Timer(const char* label)
        : start_(std::chrono::steady_clock::now()), label_(label) {}

    ~Timer() {
        auto elapsed = std::chrono::steady_clock::now() - start_;
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
        std::cerr << "[stderr] " << label_ << ": " << us << " us（僅供參考）\n";
    }
};

// 公平的量測：兩組前置成本完全相同，只在被測操作前後取快照
void bench(size_t size) {
    const int N = 2000;
    std::vector<int> source(size, 42);

    long copy_allocs = 0, move_allocs = 0;
    long long copy_bytes = 0, move_bytes = 0;

    {
        Timer t("複製組（含前置）");
        for (int i = 0; i < N; ++i) {
            std::vector<int> temp = source;              // 前置（兩組相同）
            doNotOptimize(temp);
            long a0 = g_alloc_count; long long b0 = g_alloc_bytes;
            std::vector<int> result = temp;              // ← 被測：複製建構
            doNotOptimize(result);
            copy_allocs += g_alloc_count - a0;
            copy_bytes  += g_alloc_bytes - b0;
        }
    }

    {
        Timer t("移動組（含前置）");
        for (int i = 0; i < N; ++i) {
            std::vector<int> temp = source;              // 與上面完全相同的前置
            doNotOptimize(temp);
            long a0 = g_alloc_count; long long b0 = g_alloc_bytes;
            std::vector<int> result = std::move(temp);   // ← 被測：移動建構
            doNotOptimize(result);
            move_allocs += g_alloc_count - a0;
            move_bytes  += g_alloc_bytes - b0;
        }
    }

    std::cout << "size=" << size
              << "\t複製: " << copy_allocs << " 次配置 / " << copy_bytes << " bytes"
              << "\t移動: " << move_allocs << " 次配置 / " << move_bytes << " bytes\n";
}

int main() {
    std::cout << "=== 複製 vs 移動：配置次數與位元組數隨 size 的變化 ===\n";
    std::cout << "每種 size 各執行 2000 次；兩組前置成本相同，只量被測操作本身。\n";
    std::cout << "sizeof(std::vector<int>) = " << sizeof(std::vector<int>)
              << " bytes（本機實測，實作定義）\n\n";

    bench(100);
    bench(1000);
    bench(10000);
    bench(100000);
    bench(1000000);

    std::cout << "\n複製：配置次數固定為執行次數，但位元組數隨 size 線性成長 -> O(n)\n";
    std::cout << "移動：配置次數與位元組數恆為 0，與 size 完全無關   -> O(1)\n";
    std::cout << "所以「加速比」不是常數，而是隨資料量持續擴大。\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 2.9 章：移動語意的效能分析 — 實測比較與最佳實踐4.cpp -o move_scaling

// 【但書】
//   1. 耗時（Timer）刻意輸出到 stderr，不列入下方預期輸出——它受 CPU 頻率、
//      快取狀態與系統負載影響，每次執行都不同。stdout 只放確定性的計數，
//      本機連跑 5 次逐位元組相同。
//   2. 位元組數 = 執行次數 × size × sizeof(int)（例如 2000 × 100 × 4 = 800000），
//      可自行驗算。配置次數雖為確定性，仍取決於標準庫實作
//      （本機 g++ 15.2 / libstdc++）。

// === 預期輸出 ===
// === 複製 vs 移動：配置次數與位元組數隨 size 的變化 ===
// 每種 size 各執行 2000 次；兩組前置成本相同，只量被測操作本身。
// sizeof(std::vector<int>) = 24 bytes（本機實測，實作定義）
//
// size=100	複製: 2000 次配置 / 800000 bytes	移動: 0 次配置 / 0 bytes
// size=1000	複製: 2000 次配置 / 8000000 bytes	移動: 0 次配置 / 0 bytes
// size=10000	複製: 2000 次配置 / 80000000 bytes	移動: 0 次配置 / 0 bytes
// size=100000	複製: 2000 次配置 / 800000000 bytes	移動: 0 次配置 / 0 bytes
// size=1000000	複製: 2000 次配置 / 8000000000 bytes	移動: 0 次配置 / 0 bytes
//
// 複製：配置次數固定為執行次數，但位元組數隨 size 線性成長 -> O(n)
// 移動：配置次數與位元組數恆為 0，與 size 完全無關   -> O(1)
// 所以「加速比」不是常數，而是隨資料量持續擴大。
