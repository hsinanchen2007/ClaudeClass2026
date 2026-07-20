// =============================================================================
//  第 24 課：deque 與 vector 的比較 1  —  用實測數字看穿兩者的效能差異
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<vector>、<deque>、<chrono>
//   標準版本：std::deque 與 std::vector 皆為 C++98；<chrono> 為 C++11
//   本檔測的四件事與其標準複雜度保證：
//     vector::push_back    攤銷 O(1)（amortized constant）
//     deque ::push_back    攤銷 O(1)
//     deque ::push_front   攤銷 O(1)   ← vector 沒有這個成員函式
//     兩者 operator[]      O(1)（常數時間，但常數大小不同）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「同樣是 O(1)」跑起來卻差這麼多】
//   複雜度符號 O(1) 只描述「成長趨勢」，它刻意丟掉常數項。兩個都是 O(1) 的操作，
//   常數可以差 3 倍、10 倍。這一課的重點就是讓你親眼看到被 O(1) 藏起來的常數。
//     - vector::operator[] 的機器碼大致是：load 基底指標 → 加上 i*sizeof(T) → load。
//       一次間接定址（one indirection）。
//     - deque::operator[] 必須先算出「元素在第幾個 chunk、chunk 內第幾格」，
//       再從 map（指標陣列）取出該 chunk 的位址，最後才讀值。
//       兩次間接定址 + 一次除法/位移 + 一次取模。
//   所以 deque 的隨機存取雖然也是 O(1)，實務上通常比 vector 慢。
//
// 【2. push_back：為什麼 deque 有時反而「比較穩」】
//   vector 滿了要重新配置：申請新記憶體 → 搬移全部既有元素 → 釋放舊記憶體。
//   單次搬移是 O(n)，但因為容量以倍數成長，攤銷下來每次 push_back 仍是 O(1)。
//   代價是「偶爾有一次很貴的操作」——對延遲敏感（latency-sensitive）的系統是問題。
//   deque 擴充時只是「再配一塊固定大小的 chunk，把它的位址寫進 map」，
//   既有元素完全不動。所以 deque 沒有那個尖峰延遲，但每次存取要付間接成本。
//   這是典型的「攤銷成本 vs 最壞單次成本」取捨。
//
// 【3. push_front：這是 deque 唯一無可取代的地方】
//   vector 根本沒有 push_front 成員函式。硬要做只能 v.insert(v.begin(), x)，
//   那要把全部元素往後搬一格 → 單次 O(n)，做 N 次就是 O(N²)。
//   一千萬次的 O(N²) 在現實中跑不完，所以本檔刻意不測 vector 的頭端插入。
//   deque 的 map 兩端都預留空間，往前長就跟往後長一樣便宜。
//
// 【4. 遍歷：cache locality 才是真正的勝負點】
//   vector 的元素在記憶體中完全連續，CPU 的硬體預取器（hardware prefetcher）
//   能準確預測下一個 cache line，而且一條 64-byte cache line 可裝 16 個 int。
//   deque 的元素分段連續：在同一個 chunk 內是連續的，跨 chunk 就跳走了。
//   每跨一個 chunk 邊界，預取器就得重新暖機一次。這就是遍歷慢的來源。
//
// 【概念補充 Concept Deep Dive】
//   ● libstdc++ 的 deque chunk 大小是「實作定義」的：
//     _GLIBCXX_DEQUE_BUF_SIZE 預設 512 bytes；每個 chunk 的元素個數是
//     sizeof(T) < 512 ? 512 / sizeof(T) : 1。
//     所以在本機（int 為 4 bytes）一個 chunk 放 128 個 int。
//     MSVC 的 STL 用 16 bytes 為基準，一個 chunk 只放 4 個 int —— 完全不同的數字，
//     這也是為什麼「deque 的效能」在不同平台上結論可能相反。
//   ● vector 的成長倍率同樣是實作定義：libstdc++／libc++ 用 2 倍，MSVC 用 1.5 倍。
//     1.5 倍的好處是舊區塊有機會被後續配置重複利用（2 倍永遠不夠用）。
//   ● 為什麼要用 high_resolution_clock？
//     它是「實作定義」的別名，libstdc++ 上等於 system_clock（可被 NTP 調整）。
//     嚴謹的 benchmark 應該用 std::chrono::steady_clock —— 它保證單調遞增。
//     本檔沿用課程原始寫法，但實務上請優先選 steady_clock。
//
// 【注意事項 Pay Attention】
//   1. 本檔輸出是「時間」，每次執行都不同。受 CPU 頻率、其他行程、記憶體壓力影響。
//      要看到穩定結論請跑多次取中位數，不要用單次數字下定論。
//   2. 未開最佳化（-O0）與開了 -O2 的結果差異極大。教學檔預設不開最佳化，
//      量到的絕對值偏大；比較「相對倍數」才有意義。
//   3. 第三段測的 vector<int> vec(N) 會把元素值初始化為 0，所以 sum 必然是 0。
//      這不是 bug，是 value-initialization 的規定行為。
//   4. deque 的 chunk 大小、vector 的成長倍率都是實作定義，換編譯器數字會變。
//   5. 一千萬個 int 約 40 MB；deque 因為 map 與 chunk 尚有額外開銷，會再多一些。
//      記憶體不足的機器請自行調小 N。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】deque vs vector
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. vector 和 deque 的 operator[] 都是 O(1)，為什麼實測 deque 比較慢？
//     答：O(1) 只保證「不隨 n 成長」，不保證常數一樣小。vector 是一次
//         「基底 + 偏移」定址；deque 要先算出 chunk 索引與 chunk 內偏移，
//         再從 map 取 chunk 位址，等於兩次間接定址加上一次除法與取模。
//         再加上跨 chunk 破壞 cache locality，實測自然慢。
//     追問：那什麼情況 deque 反而比較快？→ 需要 push_front／pop_front 時，
//         以及不希望出現「單次擴容搬移全部元素」尖峰延遲的場合。
//
// 🔥 Q2. vector 的 push_back 是攤銷 O(1)，「攤銷」到底是什麼意思？
//     答：單次操作最壞是 O(n)（要重新配置並搬移全部元素），但因為容量呈
//         等比成長，做 n 次 push_back 的總成本是 O(n)，平均下來每次 O(1)。
//         這是總成本的均攤，不是「每次都很快」的保證。
//     追問：這在什麼系統上會出事？→ 即時系統／低延遲交易系統。單次的尖峰
//         延遲才是它們在意的指標，這時要嘛先 reserve，要嘛改用 deque。
//
// ⚠️ 陷阱. 「deque 是分段的，所以它的 operator[] 是 O(n) 吧？」
//     答：不是。deque 內部有一塊「map」——一個指向各 chunk 的指標陣列。
//         給定索引可以直接算出是第幾個 chunk、chunk 內第幾格，
//         兩次定址就到，與元素個數無關，所以是不折不扣的 O(1)。
//     為什麼會錯：把 deque 想成 list 那種「必須一個一個走過去」的鏈結結構。
//         deque 是「分段連續」，不是鏈結串列，它有隨機存取迭代器。
//
// ⚠️ 陷阱. 「vector 比較快，所以什麼都用 vector 就對了。」
//     答：只要程式需要在頭端插入或刪除，vector 就是 O(n)，做 N 次變成 O(N²)，
//         這時 deque 快的不是常數而是整個複雜度等級，差距是天與地。
//     為什麼會錯：只記住了「vector cache 友善」這個結論，卻沒有先看
//         「我的存取模式是什麼」。選容器要先看操作模式，再看常數。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <deque>
#include <chrono>
using namespace std;
using namespace chrono;

// -----------------------------------------------------------------------------
// 【日常實務範例】固定長度的即時監控滑動視窗（sliding window）
//   情境：機房監控每秒收到一筆 CPU 使用率，只保留最近 N 秒用來算移動平均。
//         新資料從尾端進、最舊的資料從頭端出 —— 這正是 deque 的主場。
//   為什麼不用 vector：每秒都要 erase(begin())，那是 O(n) 搬移；
//         資料量一大，光是搬移就吃掉整個 CPU 預算。deque 的 pop_front 是 O(1)。
// -----------------------------------------------------------------------------
class CpuUsageWindow {
public:
    explicit CpuUsageWindow(size_t capacity) : cap_(capacity) {}

    void record(double usage) {
        window_.push_back(usage);          // 尾端進：O(1)
        sum_ += usage;
        if (window_.size() > cap_) {
            sum_ -= window_.front();
            window_.pop_front();           // 頭端出：O(1)，vector 做不到
        }
    }

    double movingAverage() const {
        return window_.empty() ? 0.0 : sum_ / static_cast<double>(window_.size());
    }

    size_t size() const { return window_.size(); }

private:
    deque<double> window_;
    size_t cap_;
    double sum_ = 0.0;
};

int main() {
    const int N = 10000000;  // 一千萬

    // === 測試 push_back ===
    {
        vector<int> vec;
        auto start = high_resolution_clock::now();
        for (int i = 0; i < N; i++) vec.push_back(i);
        auto end = high_resolution_clock::now();
        cout << "vector push_back: "
             << duration_cast<milliseconds>(end - start).count()
             << " ms" << endl;
    }
    {
        deque<int> dq;
        auto start = high_resolution_clock::now();
        for (int i = 0; i < N; i++) dq.push_back(i);
        auto end = high_resolution_clock::now();
        cout << "deque  push_back: "
             << duration_cast<milliseconds>(end - start).count()
             << " ms" << endl;
    }

    // === 測試 push_front ===
    {
        deque<int> dq;
        auto start = high_resolution_clock::now();
        for (int i = 0; i < N; i++) dq.push_front(i);
        auto end = high_resolution_clock::now();
        cout << "deque  push_front: "
             << duration_cast<milliseconds>(end - start).count()
             << " ms" << endl;
    }
    // vector 的 push_front 太慢，不測一千萬個
    // （v.insert(v.begin(), x) 單次 O(n)，做一千萬次是 O(N²)，現實中跑不完）

    // === 測試順序遍歷 ===
    {
        vector<int> vec(N);   // 注意：vector<int> vec(N) 會值初始化成 0，所以 sum 必為 0
        long long sum = 0;
        auto start = high_resolution_clock::now();
        for (int i = 0; i < N; i++) sum += vec[i];
        auto end = high_resolution_clock::now();
        cout << "vector 遍歷:      "
             << duration_cast<milliseconds>(end - start).count()
             << " ms (sum=" << sum << ")" << endl;
    }
    {
        deque<int> dq(N);
        long long sum = 0;
        auto start = high_resolution_clock::now();
        for (int i = 0; i < N; i++) sum += dq[i];
        auto end = high_resolution_clock::now();
        cout << "deque  遍歷:      "
             << duration_cast<milliseconds>(end - start).count()
             << " ms (sum=" << sum << ")" << endl;
    }

    // === 日常實務：CPU 使用率滑動視窗 ===
    cout << "\n=== 日常實務：5 秒滑動視窗移動平均 ===" << endl;
    CpuUsageWindow win(5);
    const double samples[] = {10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0};
    for (double s : samples) {
        win.record(s);
        cout << "  收到 " << s << "%  視窗大小=" << win.size()
             << "  移動平均=" << win.movingAverage() << "%" << endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 24 課：deque 與 vector 的比較1.cpp" -o cmp24
// 實務量測請另外加 -O2；未最佳化的絕對數字偏大，只有相對倍數有參考價值。

// 注意：下列毫秒數為本機一次實測，每次執行都不同（受 CPU 頻率、負載影響）；
//       只有「相對倍數」有參考價值，絕對值請勿當成規格。

// === 預期輸出 ===
// vector push_back: 97 ms
// deque  push_back: 85 ms
// deque  push_front: 79 ms
// vector 遍歷:      43 ms (sum=0)
// deque  遍歷:      667 ms (sum=0)
//
// === 日常實務：5 秒滑動視窗移動平均 ===
//   收到 10%  視窗大小=1  移動平均=10%
//   收到 20%  視窗大小=2  移動平均=15%
//   收到 30%  視窗大小=3  移動平均=20%
//   收到 40%  視窗大小=4  移動平均=25%
//   收到 50%  視窗大小=5  移動平均=30%
//   收到 60%  視窗大小=5  移動平均=40%
//   收到 70%  視窗大小=5  移動平均=50%
