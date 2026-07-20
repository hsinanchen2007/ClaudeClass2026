// =============================================================================
//  第一課：STL 的歷史與設計哲學 3  —  效率優先：泛型不該比手寫慢
// =============================================================================
//
// 【主題資訊 Information】
//   本檔驗證 STL 三大設計原則中的第二條：「效率優先」。
//   量測對象：
//     std::sort  <algorithm>  —— IntroSort，C++11 起保證最壞 O(N log N)
//     ::qsort    <cstdlib>    —— C 標準函式庫，透過函式指標比較
//   計時工具：std::chrono::steady_clock <chrono>（C++11）
//   ⚠️ 本檔的「時間」輸出走 stderr，「可重現的事實」走 stdout。
//      原因見下方【注意事項】第 1 點。
//
// 【詳細解釋 Explanation】
//
// 【1. 「泛型不犧牲效能」這句話要怎麼驗證】
//   Stepanov 當年的主張是：泛型程式碼的效能應該與手寫特化程式碼相當。
//   這句話很容易被當成信仰，但它其實可以量測。最公平的對照組是
//   C 標準函式庫的 qsort——同樣是通用排序、同樣不知道元素型別、
//   演算法家族也一樣（都是 quicksort 變體）。
//   兩者唯一的結構性差異，就是「比較這件事發生在編譯期還是執行期」。
//
// 【2. 差異的真正來源：比較子能不能 inline】
//   qsort 的簽章是
//       void qsort(void* base, size_t n, size_t size,
//                  int (*cmp)(const void*, const void*));
//   cmp 是一個「執行期才知道指向誰」的函式指標。於是：
//     * 每比較一次 = 一次無法 inline 的間接呼叫（還可能造成分支預測失敗）
//     * 比較子內部要先把 void* 轉型回 int*，再解參考
//     * 交換元素時 qsort 只能用 memcpy 逐位元組搬（它不知道型別多大）
//   std::sort 的簽章是
//       template<class RandomIt, class Compare> void sort(RandomIt, RandomIt, Compare);
//   Compare 是「型別」不是「值」。編譯器在具現化時就知道比較子的完整內容，
//   於是 `a < b` 直接被 inline 成一兩道機器指令，交換用的是該型別的 move。
//   這就是為什麼同一個演算法家族，C++ 版本會比較快。
//   本機（g++ 15.2 / libstdc++ / 100 萬筆 int）實測：
//       -O2 : std::sort ≈ 51 ms, ::qsort ≈ 87 ms  → std::sort 約快 1.7 倍
//       -O0 : std::sort ≈ 153 ms, ::qsort ≈ 137 ms → 反而是 qsort 略快
//   注意第二行。關掉最佳化時 template 的比較子不會被 inline，
//   std::sort 的結構優勢完全消失，還要多付 iterator 抽象的帳。
//   「泛型零成本」是「最佳化開啟後成立」的承諾，不是無條件成立。
//   （以上為本機單次量測，會隨機器與負載變動，不是普世常數。）
//
// 【3. 為什麼要自己寫亂數產生器，而不用 rand()】
//   本檔用一個固定種子的 LCG（線性同餘產生器）產生測試資料，原因有二：
//     ① rand() 的產生序列是「實作定義」的。glibc 與 macOS 的 libc 給出的
//        序列不同，MSVC 又是另一套。用它做測資，換台機器結果就不一樣。
//     ② 教材需要可重現的輸出。自己寫的 LCG 在任何平台上都會產生
//        逐位元相同的序列，比較次數這種統計量才有討論價值。
//   （原版程式用的是 rand()，這裡刻意改掉。）
//
// 【4. 比較次數：一個可重現的效能指標】
//   時間會隨機器負載浮動，但「std::sort 對這筆資料做了幾次比較」是
//   完全確定的。我們用一個會計數的比較子把它量出來，再拿去對照理論值
//   N·log2(N)。這比計時更能說明 IntroSort 做了什麼——你會看到實測值
//   落在 N·log2(N) 附近的常數倍，這正是 O(N log N) 的實際意義。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼計時要用 steady_clock 而不是 high_resolution_clock
//     high_resolution_clock 在多數實作上只是 system_clock 或 steady_clock
//     的別名，標準並未規定是哪一個。若它剛好是 system_clock，那麼
//     NTP 校時、使用者改系統時間都會讓你量出負的耗時。
//     量測「經過多久」的正確工具是 steady_clock：它保證單調遞增。
//     （原版程式用 high_resolution_clock，這裡改掉。）
//
// (B) 編譯器最佳化會不會讓這個量測失去意義
//     會，而且很容易。若排序結果沒有被任何人使用，最佳化器可以
//     直接把整個排序刪掉（dead code elimination）。本檔的防線是：
//     排序後計算 checksum 並印出來，讓結果「被觀察到」，
//     編譯器就不能刪。這是寫 micro-benchmark 的基本功。
//
// (C) 這個量測在 -O0 和 -O2 下的意義不同
//     本課的編譯指令沒有加 -O2。在 -O0 下 inline 幾乎不會發生，
//     std::sort 的優勢會被大幅稀釋，兩者差距會比實際部署時小很多。
//     真要評估效能差距，必須在 -O2 或 -O3 下量測。
//     這也是效能量測最常見的方法學錯誤之一：拿 debug build 的數字
//     下 release 環境的結論。
//
// 【注意事項 Pay Attention】
//   1. 耗時數字每次執行都不同（受 CPU 頻率、快取、其他行程影響），
//      所以本檔把它印到 stderr，不列入下方的預期輸出。
//      stdout 只印「換幾次執行、換哪台機器都一樣」的內容。
//   2. 不要用單次量測下結論。真實的效能評估需要多次取樣、看中位數，
//      並固定 CPU 頻率。本檔的目的是示範原理，不是嚴謹的 benchmark。
//   3. 比較次數的實測值是 libstdc++ 這一版 IntroSort 的行為，
//      屬於實作定義。換 libc++ 或換版本，數字會不同，
//      但都應該落在 N·log2(N) 的常數倍附近。
//   4. qsort 的比較子若寫成 `return *(int*)a - *(int*)b;`，
//      在兩數相差超過 INT_MAX 時會整數溢位（未定義行為）。
//      本檔的 qsort 比較子刻意寫成三路比較，不用減法。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::sort 的效率從哪來
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 std::sort 通常比 C 的 qsort 快？是因為演算法比較好嗎？
//     答：不是。兩者都是 quicksort 家族。差別在比較子的繫結時機：
//         qsort 收函式指標（執行期繫結，每次比較一次間接呼叫、不能 inline，
//         搬移只能 memcpy）；std::sort 的 Compare 是 template 參數
//         （編譯期繫結，比較被 inline，搬移用 move 建構子）。
//         這是「零成本抽象」最經典的實證。
//     追問：那如果我給 std::sort 傳一個 std::function 當比較子呢？
//         → 優勢就沒了。std::function 是型別抹除的包裝，內部一樣是
//           間接呼叫，效能會掉回 qsort 等級。傳 lambda 才有 inline 的好處。
//
// 🔥 Q2. std::sort 的複雜度保證是什麼？C++98 和 C++11 有差別嗎？
//     答：C++11 起保證「最壞 O(N log N)」；C++98 只保證「平均 O(N log N)」，
//         最壞可以是 O(N²)。這個升級是因為實作普遍改用 IntroSort：
//         當 quicksort 的遞迴深度超過 2·log2(N)，就切換成 heapsort，
//         把最壞情況硬壓住。
//     追問：那 std::stable_sort 呢？
//         → 它保證 O(N log N)（記憶體足夠時），記憶體不足時退化成
//           O(N log²N)。注意它的保證方式和 sort 不同。
//
// ⚠️ 陷阱. 我在 -O0 下量出 std::sort 只比 qsort 快 10%，
//         所以「泛型零成本」是誇大其詞——這個推論哪裡錯了？
//     答：錯在拿 debug build 的數字下 release 的結論。
//         「零成本抽象」的前提是最佳化器有做事：inline、常數傳播、
//         去虛擬化。-O0 明確關掉這些，template 的比較子不會被 inline，
//         std::sort 退化成和 qsort 類似的間接呼叫模式，優勢自然消失。
//     為什麼會錯：把「抽象的成本」和「未最佳化的成本」混為一談。
//         C++ 的承諾從來不是「即使不最佳化也很快」，
//         而是「開了最佳化之後，抽象不會留下殘餘成本」。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cstdlib>   // ::qsort
#include <cmath>     // std::log2

// -----------------------------------------------------------------------------
// 可重現的偽亂數：固定種子的 LCG（數值取自 Numerical Recipes）
// 不用 rand()，因為 rand() 的序列是實作定義的，換平台就對不上。
// -----------------------------------------------------------------------------
struct Lcg {
    unsigned int state;
    explicit Lcg(unsigned int seed) : state(seed) {}
    unsigned int next() {
        state = state * 1664525u + 1013904223u;   // unsigned 溢位是良好定義的環繞
        return state;
    }
};

// qsort 用的比較子：三路比較，不用減法（避免 a-b 整數溢位的 UB）
static int cmp_int(const void* a, const void* b) {
    int x = *static_cast<const int*>(a);
    int y = *static_cast<const int*>(b);
    if (x < y) return -1;
    if (x > y) return 1;
    return 0;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】把「一批日誌時間戳」排序後找出中位數延遲
//   情境：服務每分鐘吐出數萬筆請求耗時（毫秒），SRE 要看的不是平均值
//         （會被少數極端值拉歪），而是 p50 / p95 這種百分位數。
//   為什麼用到本主題：算百分位數的標準做法就是「排序後取第 k 個」。
//     這正是 std::sort 在生產環境最常見的用途之一。
//   效能提醒：只要 p50/p95 不需要完整排序時，std::nth_element 是 O(N)，
//     比完整排序的 O(N log N) 更划算——這裡兩種都示範。
// -----------------------------------------------------------------------------
struct LatencyStats {
    int p50;
    int p95;
};

LatencyStats percentilesBySort(std::vector<int> samples) {
    std::sort(samples.begin(), samples.end());
    size_t i50 = samples.size() * 50 / 100;
    size_t i95 = samples.size() * 95 / 100;
    return {samples[i50], samples[i95]};
}

LatencyStats percentilesByNthElement(std::vector<int> samples) {
    size_t i50 = samples.size() * 50 / 100;
    size_t i95 = samples.size() * 95 / 100;
    // nth_element 只保證「第 n 個位置放對的值」，左右各自未排序 —— O(N)
    std::nth_element(samples.begin(), samples.begin() + i95, samples.end());
    int p95 = samples[i95];
    std::nth_element(samples.begin(), samples.begin() + i50, samples.begin() + i95);
    int p50 = samples[i50];
    return {p50, p95};
}

int main() {
    const int SIZE = 1000000;

    // ---- 準備完全相同的兩份測資，確保比較公平 ----
    Lcg rng(20260720u);
    std::vector<int> data(SIZE);
    for (int i = 0; i < SIZE; ++i) {
        data[i] = static_cast<int>(rng.next() >> 1);   // 取正數
    }
    std::vector<int> data_for_qsort = data;
    std::vector<int> data_for_count = data;

    std::cout << "=== 測資規模 ===\n";
    std::cout << "元素數: " << SIZE << "\n";

    // ---- 量測 std::sort ----
    auto t0 = std::chrono::steady_clock::now();
    std::sort(data.begin(), data.end());
    auto t1 = std::chrono::steady_clock::now();

    // ---- 量測 qsort（同樣的資料）----
    auto t2 = std::chrono::steady_clock::now();
    ::qsort(data_for_qsort.data(), data_for_qsort.size(), sizeof(int), cmp_int);
    auto t3 = std::chrono::steady_clock::now();

    auto ms_sort  = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    auto ms_qsort = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count();

    // 耗時是非決定性的 → 印到 stderr，不進預期輸出
    std::cerr << "[計時 | 每次執行都不同，故不列入預期輸出]\n";
    std::cerr << "  std::sort : " << ms_sort  << " ms\n";
    std::cerr << "  ::qsort   : " << ms_qsort << " ms\n";

    std::cout << "\n=== 正確性驗證（決定性）===\n";
    std::cout << "std::sort 結果已排序: " << std::boolalpha
              << std::is_sorted(data.begin(), data.end()) << "\n";
    std::cout << "::qsort   結果已排序: "
              << std::is_sorted(data_for_qsort.begin(), data_for_qsort.end()) << "\n";
    std::cout << "兩者結果完全相同: "
              << (data == data_for_qsort) << "\n";

    // ---- 比較次數：可重現的效能指標 ----
    // 用會計數的比較子。lambda 以參考捕捉計數器，
    // 因為 std::sort 會按值複製比較子，捕值的話計數會遺失。
    long long comparisons = 0;
    std::sort(data_for_count.begin(), data_for_count.end(),
              [&comparisons](int a, int b) {
                  ++comparisons;
                  return a < b;
              });

    double theoretical = static_cast<double>(SIZE) * std::log2(static_cast<double>(SIZE));

    std::cout << "\n=== IntroSort 實際做了幾次比較 ===\n";
    std::cout << "實測比較次數     : " << comparisons << "\n";
    std::cout << "理論 N*log2(N)   : " << static_cast<long long>(theoretical) << "\n";
    std::cout << "實測 / 理論 (整數百分比): "
              << (comparisons * 100 / static_cast<long long>(theoretical)) << "%\n";
    std::cout << "（比較次數是實作定義；換 libc++ 或不同版本會有出入，\n";
    std::cout << "  但都會落在 N*log2(N) 的常數倍附近，這就是 O(N log N) 的具體樣貌）\n";

    // checksum：讓排序結果「被使用」，避免最佳化器把整段排序刪掉
    long long checksum = 0;
    for (int i = 0; i < SIZE; i += 100000) checksum += data[i];
    std::cout << "\ncheckSum(每十萬筆取一個相加): " << checksum << "\n";

    // ---- 日常實務 ----
    std::cout << "\n=== 日常實務：請求延遲的 p50 / p95 ===\n";
    Lcg rng2(99u);
    std::vector<int> latency(10000);
    for (size_t i = 0; i < latency.size(); ++i) {
        // 多數請求 1~50ms，少數長尾到數百 ms（模擬真實服務的延遲分布）
        unsigned int r = rng2.next() % 100;
        latency[i] = (r < 95) ? static_cast<int>(1 + rng2.next() % 50)
                              : static_cast<int>(200 + rng2.next() % 800);
    }
    LatencyStats a = percentilesBySort(latency);
    LatencyStats b = percentilesByNthElement(latency);
    std::cout << "完整排序 O(N log N) : p50=" << a.p50 << "ms  p95=" << a.p95 << "ms\n";
    std::cout << "nth_element O(N)    : p50=" << b.p50 << "ms  p95=" << b.p95 << "ms\n";
    std::cout << "兩種算法結果一致: " << (a.p50 == b.p50 && a.p95 == b.p95) << "\n";
    std::cout << "只要百分位數，nth_element 不必把整批排完，是更省的選擇\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第一課：STL 的歷史與設計哲學3.cpp -o demo3
// 想看到真實的效能差距請加最佳化: g++ -std=c++17 -O2 -Wall -Wextra ...

// === 預期輸出 ===
// === 測資規模 ===
// 元素數: 1000000
//
// === 正確性驗證（決定性）===
// std::sort 結果已排序: true
// ::qsort   結果已排序: true
// 兩者結果完全相同: true
//
// === IntroSort 實際做了幾次比較 ===
// 實測比較次數     : 24032933
// 理論 N*log2(N)   : 19931568
// 實測 / 理論 (整數百分比): 120%
// （比較次數是實作定義；換 libc++ 或不同版本會有出入，
//   但都會落在 N*log2(N) 的常數倍附近，這就是 O(N log N) 的具體樣貌）
//
// checkSum(每十萬筆取一個相加): 9664437198
//
// === 日常實務：請求延遲的 p50 / p95 ===
// 完整排序 O(N log N) : p50=26ms  p95=50ms
// nth_element O(N)    : p50=26ms  p95=50ms
// 兩種算法結果一致: true
// 只要百分位數，nth_element 不必把整批排完，是更省的選擇
