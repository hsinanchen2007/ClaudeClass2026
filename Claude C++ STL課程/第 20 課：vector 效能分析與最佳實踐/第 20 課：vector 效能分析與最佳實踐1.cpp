// =============================================================================
//  第 20 課：vector 效能分析與最佳實踐 1  —  reserve()：消滅重新配置
// =============================================================================
//
// 【主題資訊 Information】
//   void reserve(size_type new_cap);        // 確保 capacity() >= new_cap，不改變 size()
//   void resize(size_type count);           // 改變 size()，新元素做 value-initialization
//   size_type capacity() const noexcept;    // 目前配置到的容量
//   size_type size()     const noexcept;    // 目前實際元素個數
//
//   標準版本：C++98 起即有（reserve / resize / capacity）
//   複雜度  ：reserve 需要重新配置時 O(n)，否則 O(1)；push_back 均攤 O(1)
//   標頭檔  ：<vector>；本檔另用 <chrono> 計時、<string> 標籤
//
// 【詳細解釋 Explanation】
//
// 【1. push_back 為什麼是「均攤 O(1)」而不是 O(1)】
//   單次 push_back 有兩種命運：
//     (a) capacity 還有空位 → 就地建構一個元素，O(1)。
//     (b) capacity 已經滿了 → 觸發「重新配置（reallocation）」：
//         ① 向配置器要一塊更大的記憶體
//         ② 把舊的 n 個元素逐一「移動或拷貝」到新記憶體
//         ③ 銷毀舊元素、歸還舊記憶體
//         這一次就是 O(n)。
//   所以最壞單次是 O(n)。但標準保證的是「均攤（amortized）O(1)」——
//   把「n 次 push_back 的總成本」除以 n，得到的平均成本是常數。
//
// 【2. 倍增論證（doubling argument）：為什麼均攤真的是常數】
//   關鍵在於「容量成長是乘法而不是加法」。假設每次擴容都把容量乘以 k（k > 1）：
//     插入第 N 個元素為止，發生過的重新配置容量依序是
//         1, k, k^2, k^3, …, N
//     總搬移成本 = 1 + k + k^2 + … + N
//                ≈ N * k/(k-1)          （等比級數，收斂）
//     例如 k = 2：總成本約 2N；k = 1.5：總成本約 3N。
//   重點：總成本是 N 的「常數倍」，除以 N 次操作 → 每次均攤 O(1)。
//
//   反例（為什麼不能「每次加固定量」）：若每次只加 1 個容量，
//     總搬移成本 = 1 + 2 + 3 + … + N = N(N+1)/2 = O(N^2)
//     均攤下來每次 O(N) —— 這就是為什麼成長必須是乘法。
//
// 【3. reserve() 做了什麼】
//   reserve(N) 一次把容量拉到 N，之後的 N 次 push_back 全部走上面的路線 (a)，
//   「零次重新配置、零次元素搬移」。省下來的是：
//     * 約 log2(N) 次記憶體配置／釋放（N = 100 萬時約 20 次）
//     * 約 2N 次元素的移動建構 + 解構
//   對 int 這種 trivially copyable 的型別，搬移是 memcpy 等級，省下的主要是
//   配置器往返；對 std::string、含 heap 資源的物件，省下的是大量真實建構成本。
//
// 【4. reserve 與 resize 的分工（最常被搞混）】
//     reserve(n)：只動 capacity，size() 仍是 0 → 之後必須用 push_back/emplace_back
//     resize(n) ：真的把 size() 變成 n，元素被 value-initialize（int 會是 0）
//                 → 之後可以直接用 v[i] = ... 賦值
//   寫成 `v.reserve(n); v[0] = x;` 是 **未定義行為**——容量有了，但那裡還沒有物件。
//
// 【5. 如何「正確」量測】
//   本檔的 Timer 是 RAII 計時器：建構時記時間、解構時印出耗時。注意它的成員宣告
//   順序是 label_ 在前、start_ 在後，且初始化列表也照這個順序——這讓「讀時鐘」
//   成為建構過程的最後一件事，字串複製的成本不會被算進待測區間。
//   （成員初始化「一律依宣告順序」進行，與初始化列表寫的順序無關；兩者不一致時
//     GCC 會發出 -Wreorder 警告。）
//
// 【概念補充 Concept Deep Dive】
//
// (A) 成長倍率是「實作定義」，標準沒有規定
//     標準只要求 push_back 均攤 O(1)，沒有指定倍率。實務上兩大陣營：
//       * libstdc++ (GCC)、libc++ (Clang)：倍率 2
//       * MSVC STL                        ：倍率 1.5
//     本機 libstdc++ 實測（見 main 的「成長軌跡」段）確實是
//         0 -> 1 -> 2 -> 4 -> 8 -> 16 -> … 每次剛好 x2。
//     這是**本機實測值，不是標準保證**，換編譯器就可能不同。
//
// (B) 為什麼 MSVC 選 1.5？——記憶體重複利用論證
//     倍率 2 有個性質：新配置的容量「永遠大於先前所有配置的總和」
//         1 + 2 + 4 + … + 2^(k-1) = 2^k - 1  <  2^k
//     所以已釋放的舊區塊永遠拼不出下一塊，配置器無法就地重用，堆會一路往前長。
//     倍率 1.5 則相反：累積的舊區塊在幾步之後足以覆蓋新需求
//         1 + 1.5 + 2.25 = 4.75  >  3.375
//     理論上配置器有機會重用先前釋放的空間，記憶體足跡較友善。
//     代價是重新配置次數變多（log_1.5 N > log_2 N）。
//     這是**工程取捨**，兩邊都合法，沒有誰「對」。
//
// (C) reserve 是精確的，push_back 才是倍增
//     本機實測：reserve(100) 之後 capacity() 剛好是 100（不是 128）；
//     但再 push 第 101 個元素時，容量從 100 跳到 200（對現有容量 x2）。
//     也就是說倍增是以「當下容量」為基準，不是回到 2 的冪次。
//
// 【注意事項 Pay Attention】
//
// 1. ★ 本檔輸出的耗時數字是「非決定性」的：每次執行都不同，換機器、換編譯器、
//    換系統負載都會不同。請看「相對趨勢」，不要背絕對數字。
// 2. ★ 最佳化等級會徹底改變結論。本檔預期輸出是用檔尾「編譯:」那行取得的，
//    亦即 **沒有 -O 旗標（等同 -O0）**。改用 -O2 之後：
//      * 三段的絕對耗時都會大幅下降
//      * 差距比例也會改變（-O0 下函式呼叫沒有 inline，開銷被放大）
//    要做嚴肅的效能結論，必須在 -O2/-O3 下、跑多次取中位數，並用
//    Google Benchmark 這類工具處理暖機與 dead-code elimination。
// 3. 不要把「本機實測約 N 倍」講成「一定快 N 倍」——那不是語言保證，
//    是這台機器、這組旗標、這次執行的觀測值。
// 4. reserve 不會縮小容量。reserve(小於現有 capacity 的值) 是 no-op，
//    不會釋放記憶體（要縮請用 shrink_to_fit，而且它「非強制」）。
// 5. reserve 之後 size() 仍是 0；用 v[i] 存取尚未建構的位置是未定義行為。
// 6. reserve/重新配置會使所有 iterator、reference、pointer 失效——
//    包含 data() 拿到的裸指標。
// 7. 不要「每加一個元素就 reserve(size()+1)」：那等於退化成每次加 1 的
//    線性成長，把均攤 O(1) 打回 O(N^2)。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】reserve 與均攤複雜度
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. push_back 的複雜度是多少？為什麼是「均攤」O(1)？
//     答：均攤 O(1)。單次最壞是 O(n)（要重新配置並搬移全部元素），
//         但因為容量是「乘法成長」，n 次 push_back 的總搬移成本是等比級數
//         1 + k + k^2 + … + n ≈ n*k/(k-1)，是 n 的常數倍，除以 n 次操作
//         得到常數 → 均攤 O(1)。
//     追問：如果改成每次只擴容 +1 會怎樣？
//         → 總成本變成 1+2+…+n = O(n^2)，均攤變 O(n)，這就是乘法成長的必要性。
//
// 🔥 Q2. reserve() 和 resize() 差在哪？什麼時候該用哪個？
//     答：reserve 只動 capacity、size 不變，適合「接下來要 push_back」；
//         resize 真的把 size 變成 n 並建構元素，適合「接下來要用 v[i] 賦值」
//         或要把 buffer 交給 C API 填。
//     追問：v.reserve(100); v[0] = 1; 對嗎？
//         → 不對，是未定義行為。容量有了但物件還沒建構，v[0] 並不存在。
//
// 🔥 Q3. vector 的成長倍率是 2 嗎？
//     答：標準沒有規定，這是實作定義。libstdc++/libc++ 用 2，MSVC 用 1.5。
//         本機 libstdc++ 實測是 x2（0->1->2->4->8…）。
//     追問：為什麼有人主張 1.5 比較好？
//         → 倍率 2 時，新需求永遠大於先前所有配置的總和（2^k > 2^k - 1），
//           釋放掉的舊區塊永遠拼不出下一塊，配置器無法就地重用；
//           1.5 則有機會重用，記憶體足跡較好，代價是重新配置次數變多。
//
// ⚠️ 陷阱. 「我 reserve 了，所以 iterator 之後都安全了」——對嗎？
//     答：只在「不超過 reserve 的容量」的前提下成立。一旦 size 超過那個容量，
//         照樣重新配置、照樣全體失效。而且 reserve 這個動作**本身**就會讓
//         呼叫前取得的所有 iterator/pointer/reference 立刻失效。
//     為什麼會錯：多數人把 reserve 記成「一勞永逸的保險」，
//         卻忘了它保護的是「容量之內」，而且它自己就是一次重新配置。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <algorithm>

// -----------------------------------------------------------------------------
// RAII 計時器
// 成員宣告順序 label_ -> start_，初始化列表也是同樣順序（避免 -Wreorder），
// 並讓「讀時鐘」是建構的最後一步，字串複製不計入待測區間。
// -----------------------------------------------------------------------------
struct Timer {
    std::string label_;
    std::chrono::high_resolution_clock::time_point start_;

    explicit Timer(const std::string& label)
        : label_(label),
          start_(std::chrono::high_resolution_clock::now()) {}

    ~Timer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
        std::cout << "  " << label_ << ": " << us << " us" << std::endl;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 56. Merge Intervals
//   題目：給一組區間，合併所有重疊的區間後回傳。
//   為什麼用到本主題：結果最多就是 intervals.size() 個區間（完全不重疊時），
//     所以可以在開始前 reserve(intervals.size())，讓整個合併過程
//     一次重新配置都不用做。這是 reserve 最典型的用法——
//     「上界已知，即使實際用不滿也值得先要」。
// -----------------------------------------------------------------------------
std::vector<std::vector<int>> mergeIntervals(std::vector<std::vector<int>> intervals) {
    std::vector<std::vector<int>> merged;
    if (intervals.empty()) return merged;

    std::sort(intervals.begin(), intervals.end(),
              [](const std::vector<int>& a, const std::vector<int>& b) { return a[0] < b[0]; });

    merged.reserve(intervals.size());          // 上界已知，先要好
    for (const auto& iv : intervals) {
        if (!merged.empty() && iv[0] <= merged.back()[1]) {
            merged.back()[1] = std::max(merged.back()[1], iv[1]);   // 與前一段重疊 -> 延長
        } else {
            merged.push_back(iv);                                   // 不重疊 -> 開新段
        }
    }
    return merged;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】感測器批次匯入（batch ingest）
//   情境：資料收集器每分鐘回傳一批讀數，格式是 "device_id:value"。
//     批次筆數在解析前就知道（呼叫端拿得到行數），所以先 reserve 再逐筆解析，
//     整個匯入過程只配置一次記憶體。
//   這是實務上 reserve 最有價值的位置：資料匯入、CSV 解析、network batch decode。
// -----------------------------------------------------------------------------
struct Reading {
    std::string device;
    int value;
};

std::vector<Reading> ingestBatch(const std::vector<std::string>& rawLines) {
    std::vector<Reading> out;
    out.reserve(rawLines.size());              // 筆數已知：一次到位

    for (const std::string& line : rawLines) {
        std::size_t colon = line.find(':');
        if (colon == std::string::npos) continue;          // 格式不合就跳過
        out.push_back(Reading{line.substr(0, colon),
                              std::stoi(line.substr(colon + 1))});
    }
    return out;                                 // NRVO / 移動，不會整包拷貝
}

int main() {
    const int N = 1'000'000;
    // 累加一個檢查值，讓迴圈有可觀察的副作用，避免在高最佳化等級被整段消除
    long long sink = 0;

    std::cout << "=== 1. reserve 效能比較 (N = " << N << ") ===" << std::endl;
    {
        Timer t("不用 reserve  ");
        std::vector<int> v;
        for (int i = 0; i < N; ++i) v.push_back(i);
        sink += v.back();
    }
    {
        Timer t("使用 reserve  ");
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) v.push_back(i);
        sink += v.back();
    }
    {
        Timer t("resize + 索引 ");
        std::vector<int> v(N);          // 已建構 N 個 0
        for (int i = 0; i < N; ++i) v[i] = i;
        sink += v.back();
    }
    std::cout << "  (檢查值 sink = " << sink << "，僅為防止迴圈被最佳化掉)" << std::endl;

    std::cout << "\n=== 2. 容量成長軌跡（本機 libstdc++ 實測，實作定義）===" << std::endl;
    {
        std::vector<int> v;
        std::size_t last = 0;
        int shown = 0;
        for (int i = 0; i < 100000 && shown < 12; ++i) {
            v.push_back(i);
            if (v.capacity() != last) {
                std::cout << "  size=" << v.size() << " -> capacity=" << v.capacity();
                if (last != 0) {
                    std::cout << "  (倍率 " << (double)v.capacity() / (double)last << ")";
                }
                std::cout << std::endl;
                last = v.capacity();
                ++shown;
            }
        }
        std::cout << "  -> 本機每次剛好 x2；標準未規定，MSVC 為 1.5" << std::endl;
    }

    std::cout << "\n=== 3. reserve 精確、push_back 倍增 ===" << std::endl;
    {
        std::vector<int> v;
        v.reserve(100);
        std::cout << "  reserve(100) 後 size=" << v.size()
                  << " capacity=" << v.capacity() << std::endl;
        for (int i = 0; i < 101; ++i) v.push_back(i);
        std::cout << "  push 到第 101 個後 capacity=" << v.capacity()
                  << "  (以當下容量 x2，不是回到 2 的冪次)" << std::endl;
    }

    std::cout << "\n=== 4. LeetCode 56. Merge Intervals ===" << std::endl;
    {
        auto r = mergeIntervals({{1, 3}, {2, 6}, {8, 10}, {15, 18}});
        std::cout << "  ";
        for (const auto& iv : r) std::cout << "[" << iv[0] << "," << iv[1] << "] ";
        std::cout << std::endl;

        auto r2 = mergeIntervals({{1, 4}, {4, 5}});
        std::cout << "  ";
        for (const auto& iv : r2) std::cout << "[" << iv[0] << "," << iv[1] << "] ";
        std::cout << std::endl;
    }

    std::cout << "\n=== 5. 日常實務：感測器批次匯入 ===" << std::endl;
    {
        std::vector<std::string> raw = {
            "sensor-a:23", "sensor-b:41", "壞掉的行", "sensor-c:7"
        };
        auto batch = ingestBatch(raw);
        std::cout << "  收到 " << raw.size() << " 行，解析成功 " << batch.size() << " 筆" << std::endl;
        for (const auto& r : batch) {
            std::cout << "    " << r.device << " = " << r.value << std::endl;
        }
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 20 課：vector 效能分析與最佳實踐1.cpp -o lesson20_1

// ※ 第 1 段的三個耗時數字「每次執行都不同」，換機器／換最佳化等級差異更大，

// === 預期輸出 ===
// ※ 本機實測（Ubuntu 26.04 / g++ 15.2.0 / 上面那行編譯指令，未加 -O 旗標）。
//    請只看相對趨勢，不要把數字當定值背。
//    （本機連跑 4 次的觀測範圍：不用 reserve 9001~9731、使用 reserve 7719~7873、
//      resize 6363~7138 us；順序穩定，絕對值每次都跳動。）
//
// === 1. reserve 效能比較 (N = 1000000) ===
//   不用 reserve  : 9001 us
//   使用 reserve  : 7726 us
//   resize + 索引 : 6444 us
//   (檢查值 sink = 2999997，僅為防止迴圈被最佳化掉)
//
// === 2. 容量成長軌跡（本機 libstdc++ 實測，實作定義）===
//   size=1 -> capacity=1
//   size=2 -> capacity=2  (倍率 2)
//   size=3 -> capacity=4  (倍率 2)
//   size=5 -> capacity=8  (倍率 2)
//   size=9 -> capacity=16  (倍率 2)
//   size=17 -> capacity=32  (倍率 2)
//   size=33 -> capacity=64  (倍率 2)
//   size=65 -> capacity=128  (倍率 2)
//   size=129 -> capacity=256  (倍率 2)
//   size=257 -> capacity=512  (倍率 2)
//   size=513 -> capacity=1024  (倍率 2)
//   size=1025 -> capacity=2048  (倍率 2)
//   -> 本機每次剛好 x2；標準未規定，MSVC 為 1.5
//
// === 3. reserve 精確、push_back 倍增 ===
//   reserve(100) 後 size=0 capacity=100
//   push 到第 101 個後 capacity=200  (以當下容量 x2，不是回到 2 的冪次)
//
// === 4. LeetCode 56. Merge Intervals ===
//   [1,6] [8,10] [15,18]
//   [1,5]
//
// === 5. 日常實務：感測器批次匯入 ===
//   收到 4 行，解析成功 3 筆
//     sensor-a = 23
//     sensor-b = 41
//     sensor-c = 7
