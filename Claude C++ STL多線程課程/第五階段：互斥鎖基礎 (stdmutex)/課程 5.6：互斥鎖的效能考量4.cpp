//
// =============================================================================
//  課程 5.6：互斥鎖的效能考量4.cpp  —  把計算移出鎖，效能差 4 倍以上
// =============================================================================
//
// 【主題資訊 Information】
//   主題：    臨界區段最小化；三種寫法（鎖內算 / 鎖外算 / 批次累加）的量化比較
//   標準版本：std::thread / std::chrono / lambda 為 C++11
//   標頭檔：  <mutex>、<chrono>、<cmath>
//   本機環境：Ubuntu 26.04 / g++ 15.2.0 / 16 核心
//   ⚠️ 所有時間數字都是本機實測值，每次執行都不同，非標準保證。
//
// 【詳細解釋 Explanation】
//
// 【1. 三種寫法的差別只有「鎖住多少東西」】
//   三個函式做的計算完全相同（同樣呼叫 expensiveComputation 同樣多次），
//   唯一的差別是鎖的範圍：
//     * badApproach   ：lock → 計算 → 累加 → unlock（計算在鎖內）
//     * goodApproach  ：計算 → lock → 累加 → unlock（計算移到鎖外）
//     * betterApproach：全部計算 → lock → 累加一次 → unlock（只鎖一次）
//   → 這是效能優化中少見的「純賺」：程式碼幾乎沒變複雜，
//     正確性完全相同，效能卻差好幾倍。
//
// 【2. 為什麼 bad 這麼慢：它把並行變成序列】
//   badApproach 在鎖內做 expensiveComputation（1000 次三角函數）。
//   由於臨界區段是序列化的，4 條執行緒的計算【一個一個排隊做】——
//   等於完全退化成單執行緒，還要外加搶鎖的成本。
//   依 Amdahl 定律，序列比例 p ≈ 100% → 加速上限 1×。
//   → 這正是「鎖的真正成本不在指令，在失去的並行度」最直接的證據。
//
// 【3. good vs better：從 N 次上鎖降到 1 次】
//   goodApproach 已經把計算移出鎖，4 條執行緒的計算可以真正並行，
//   所以相對 bad 有數倍的提升。
//   betterApproach 更進一步：連「累加」都先在區域變數做，
//   整條執行緒只在最後上鎖一次。
//   上鎖次數從 250 次（每執行緒的工作量）降到 1 次。
//   本機實測 good 與 better 的差距不大 ——
//   因為此例的計算量（每次約數微秒）遠大於鎖的開銷（十幾奈秒），
//   鎖已經不是瓶頸了。
//   → 這帶出一個重要判斷：【當鎖的佔比已經很低時，再優化鎖沒有意義】。
//     betterApproach 的價值要在「計算很輕、上鎖很頻繁」時才會顯現
//     （見 5.6-2 的本地累加實測：107 ms → 0.39 ms）。
//
// 【4. 什麼東西「絕對不該」放在鎖內】
//   依照代價由大到小：
//     ① I/O：檔案、網路、資料庫 —— 微秒到毫秒等級，可能還會阻塞（見 5.6-5）
//     ② 記憶體配置：new / vector 擴容 —— 可能觸發系統呼叫，且 malloc 自己也有鎖
//     ③ 使用者提供的回呼：你不知道它會做什麼、會不會反過來鎖同一把鎖（→ UB）
//     ④ 純計算：本檔的情形
//     ⑤ 睡眠：絕對不可以，等於把鎖扣住不放
//   → 檢查清單：進入臨界區段前先問「這一行【必須】在鎖裡嗎？」
//     只有「讀寫共享資料」這一件事是必須的。
//
// 【5. 本檔的一個實作細節：為什麼要重設 result】
//   runTest 每次都把全域 result 歸零再測，否則三次測試會互相累加。
//   注意 result 是全域變數且被多執行緒寫入 ——
//   它在三個 approach 中都有鎖保護，所以沒有 data race；
//   但 runTest 中的 `result = 0;` 是在【沒有執行緒在跑】的時候做的，
//   所以也安全。這種「只在單執行緒階段存取共享變數」的模式很常見，
//   但要確保真的沒有執行緒還在跑（本檔靠 join 保證）。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼浮點數累加的結果每次可能有微小差異
//   result 是 double，而浮點加法【不滿足結合律】：
//   (a+b)+c 不一定等於 a+(b+c)。三種寫法的累加順序不同
//   （bad/good 是逐項加、better 是先加成本地和再加總），
//   而且多執行緒的合併順序每次都不同，
//   所以最後的 result 可能在最低有效位上有差異。
//   → 這是浮點運算的本質，不是 bug。需要可重現的結果時，
//     要固定累加順序（例如各執行緒的部分和先排序再相加）
//     或改用整數/定點數。
//
// (B) 這個模式的名字：reduce / fold
//   betterApproach 就是平行程式設計中的 reduction（歸約）：
//   把資料切塊 → 各自算出部分結果 → 最後合併。
//   這是所有平行框架的基本模式（OpenMP 的 reduction 子句、
//   std::reduce（C++17）、MapReduce 的 Reduce 階段）。
//   C++17 起可以直接用 std::reduce + 平行執行策略：
//       std::reduce(std::execution::par, v.begin(), v.end(), 0.0);
//   （需要連結 TBB；本檔手寫是為了讓機制透明。）
//
// (C) 為什麼 expensiveComputation 用三角函數
//   sin/cos 是無法被編譯器輕易最佳化掉的真實計算
//   （不像 ++counter 可能被摺疊成一次加法），
//   所以能穩定地製造出「有份量的臨界區段」。
//   如果用簡單的整數運算，-O2 可能把整個迴圈消除，
//   三種寫法就會量出幾乎相同的時間，看不出差異。
//
// 【注意事項 Pay Attention】
// 1. 只有「讀寫共享資料」必須在鎖內；計算、格式化、配置一律移到鎖外。
// 2. 在鎖內做計算會讓多執行緒退化成序列執行 —— 加再多核心也沒用。
// 3. 絕對不要在鎖內做 I/O、記憶體配置、睡眠，或呼叫使用者的回呼。
// 4. 當鎖的佔比已經很低時，再優化鎖沒有意義；先量測再決定。
// 5. 浮點累加不滿足結合律，多執行緒下最低有效位可能每次不同 —— 這不是 bug。
// 6. 所有時間數字每次執行都不同，檔尾的預期輸出只是某一次的實測。
//
// 【LeetCode 說明】本檔不附 LeetCode 範例。
//   本檔是「臨界區段大小對效能的影響」的量化實驗，沒有可判題的演算法；
//   允許使用的設計題（146/155/705/707/1603）與並行題（1114～1117/1195）
//   都不涉及效能量測。硬湊只會失焦，故從缺，
//   改以下方兩個真實情境（影像處理的平行歸約、報表聚合）呈現。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】臨界區段最小化
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼「在鎖內做計算」會讓多執行緒完全失去意義?
//     答：臨界區段是序列化的 —— 同一時間只有一條執行緒能執行。
//         把耗時計算放進鎖內，等於讓所有執行緒的計算排隊一個一個做，
//         退化成單執行緒，還要外加搶鎖與上下文切換的成本。
//         依 Amdahl 定律，序列比例 p ≈ 100% 時加速上限就是 1×。
//     追問：那哪些東西才「必須」在鎖內?
//         → 只有「讀寫共享資料」這一件事。計算、格式化、記憶體配置、
//           I/O、日誌全部都應該移到鎖外。
//
// 🔥 Q2. 「批次累加」為什麼比「鎖外計算」更好?什麼時候差別才明顯?
//     答：批次累加把上鎖次數從「每次操作一次」降到「每條執行緒一次」，
//         直接把序列比例壓到接近零。
//         但差別的大小取決於【鎖的開銷佔總工作量的比例】：
//         本檔每次計算約數微秒、鎖只要十幾奈秒，所以 good 與 better
//         差距不大；若計算只是一次 ++（約 1 ns），
//         差距會非常誇張（見 5.6-2：107 ms → 0.39 ms）。
//     追問：批次累加有什麼代價?
//         → 結果有延遲（合併前看不到中間值）、
//           執行緒中途崩潰會遺失它的本地累計、
//           以及浮點累加順序改變導致最低有效位可能不同。
//
// ⚠️ 陷阱. 「我把計算移到鎖外了，所以這段程式碼現在是最佳的」——還可能漏了什麼?
//     答：漏了「鎖內是否還有隱藏的慢動作」。常見的隱形殺手包括：
//         ① 記憶體配置（push_back 擴容、字串串接）——
//            malloc 自己內部就有鎖，等於巢狀上鎖
//         ② 使用者提供的回呼 —— 你無法控制它做什麼
//         ③ 日誌輸出 —— 看起來很輕，實際上是 I/O
//         ④ 例外的建構與拋出
//     為什麼會錯：把「計算」狹義理解成「算數運算」，
//         卻沒發現一行 `logs.push_back(msg)` 同時包含了字串複製與可能的
//         記憶體重新配置。判斷的方式不是看程式碼長度，
//         而是問「這一行最壞情況要多久」——
//         任何可能超過幾百奈秒的東西都該移出臨界區段。
// ═══════════════════════════════════════════════════════════════════════════
// 檔案：lesson_5_6_minimize_critical_section.cpp
// 說明：減少臨界區段大小提升效能

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <cmath>
#include <iomanip>
#include <map>
#include <string>

std::mutex mtx;
double result = 0;

// 耗時計算（模擬複雜運算）
double expensiveComputation(int input) {
    double sum = 0;
    for (int i = 0; i < 1000; ++i) {
        sum += std::sin(input + i) * std::cos(input - i);
    }
    return sum;
}

// 差的做法：在鎖內進行計算
void badApproach(int start, int count) {
    for (int i = start; i < start + count; ++i) {
        std::lock_guard<std::mutex> lock(mtx);  // 鎖定
        double computed = expensiveComputation(i);  // 💀 在鎖內計算！
        result += computed;
    }  // 解鎖
}

// 好的做法：在鎖外進行計算
void goodApproach(int start, int count) {
    for (int i = start; i < start + count; ++i) {
        double computed = expensiveComputation(i);  // ✓ 在鎖外計算
        
        std::lock_guard<std::mutex> lock(mtx);  // 鎖定
        result += computed;  // 只鎖定必要的部分
    }  // 解鎖
}

// 更好的做法：批次累加
void betterApproach(int start, int count) {
    double localSum = 0;  // 本地累加
    
    for (int i = start; i < start + count; ++i) {
        localSum += expensiveComputation(i);  // 完全不需要鎖
    }
    
    std::lock_guard<std::mutex> lock(mtx);  // 只鎖定一次
    result += localSum;
}

template<typename Func>
double runTest(Func&& func, int numThreads, int totalWork) {
    result = 0;
    int workPerThread = totalWork / numThreads;
    
    std::vector<std::thread> threads;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(func, i * workPerThread, workPerThread);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    return std::chrono::duration<double, std::milli>(end - start).count();
}


// -----------------------------------------------------------------------------
// 【日常實務範例 1】影像處理：把每個像素的計算移出鎖（平行歸約）
//   情境：計算一張影像的直方圖 —— 每個像素要算出所屬的 bin，然後累加。
//   天真寫法：對每個像素都 lock → 算 bin → ++histogram[bin] → unlock。
//         結果是 800 萬個像素全部排隊，多核心完全沒用上。
//   正解：每條執行緒維護自己的本地直方圖（完全無鎖），
//         處理完自己那一段之後，才上一次鎖把本地直方圖合併進全域。
//         上鎖次數從「像素數」降到「執行緒數」。
//   這是影像處理、統計、MapReduce 的標準模式。
// -----------------------------------------------------------------------------
static const int HIST_BINS = 256;

struct Histogram {
    std::mutex mtx;
    std::vector<long> bins;
    Histogram() : bins(HIST_BINS, 0) {}
};

// 模擬像素的計算（實際可能是色彩空間轉換、gamma 校正等）
inline int pixelToBin(int pixel) {
    int v = (pixel * 37 + 11) % 251;
    return v % HIST_BINS;
}

// ✗ 每個像素都上鎖
void histogramPerPixelLock(Histogram& h, const std::vector<int>& pixels,
                           size_t begin, size_t end) {
    for (size_t i = begin; i < end; ++i) {
        std::lock_guard<std::mutex> lock(h.mtx);
        ++h.bins[static_cast<size_t>(pixelToBin(pixels[i]))];
    }
}

// ✓ 本地直方圖 + 最後合併一次
void histogramLocalMerge(Histogram& h, const std::vector<int>& pixels,
                         size_t begin, size_t end) {
    std::vector<long> local(HIST_BINS, 0);           // 全程無鎖
    for (size_t i = begin; i < end; ++i) {
        ++local[static_cast<size_t>(pixelToBin(pixels[i]))];
    }
    std::lock_guard<std::mutex> lock(h.mtx);         // 整條執行緒只上鎖一次
    for (int b = 0; b < HIST_BINS; ++b) h.bins[static_cast<size_t>(b)] += local[static_cast<size_t>(b)];
}

template<typename Fn>
double runHistogram(Fn fn, const std::vector<int>& pixels, int numThreads, long& checksum) {
    Histogram h;
    std::vector<std::thread> threads;
    size_t chunk = pixels.size() / static_cast<size_t>(numThreads);

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < numThreads; ++i) {
        size_t b = chunk * static_cast<size_t>(i);
        size_t e = (i == numThreads - 1) ? pixels.size() : b + chunk;
        threads.emplace_back([&h, &pixels, b, e, fn] { fn(h, pixels, b, e); });
    }
    for (auto& t : threads) t.join();
    auto end = std::chrono::steady_clock::now();

    checksum = 0;
    for (long v : h.bins) checksum += v;
    return std::chrono::duration<double, std::milli>(end - start).count();
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】報表聚合：鎖內的「隱藏慢動作」
//   情境：把大量交易記錄依類別聚合。看起來已經「只在鎖內做累加」，
//         但實際上鎖內還藏著兩個慢動作：
//           ① std::string 的建構與比較（key 查找）
//           ② std::map 可能的節點配置（new）—— malloc 內部也有鎖
//   正解：在鎖外先把 key 轉成整數索引（或先在本地 map 聚合），
//         鎖內只做純粹的數值累加。
// -----------------------------------------------------------------------------
struct ReportAggregator {
    std::mutex mtx;
    std::vector<long> totals;     // 用索引取代字串 key
    explicit ReportAggregator(size_t categories) : totals(categories, 0) {}
};

// ✗ 鎖內做字串處理與 map 配置
void aggregateSlow(ReportAggregator& agg, std::map<std::string, long>& shared,
                   const std::vector<int>& records, size_t begin, size_t end) {
    for (size_t i = begin; i < end; ++i) {
        std::lock_guard<std::mutex> lock(agg.mtx);
        std::string key = "category-" + std::to_string(records[i] % 8);  // 鎖內建字串！
        shared[key] += records[i];                                        // 鎖內可能 new！
    }
}

// ✓ 鎖外算索引、本地聚合，鎖內只做數值累加
void aggregateFast(ReportAggregator& agg, const std::vector<int>& records,
                   size_t begin, size_t end) {
    std::vector<long> local(agg.totals.size(), 0);
    for (size_t i = begin; i < end; ++i) {
        local[static_cast<size_t>(records[i] % 8)] += records[i];   // 全程無鎖
    }
    std::lock_guard<std::mutex> lock(agg.mtx);
    for (size_t k = 0; k < local.size(); ++k) agg.totals[k] += local[k];
}

int main() {
    const int totalWork = 1000;
    const int numThreads = 4;
    
    std::cout << "=== 臨界區段大小對效能的影響 ===" << std::endl;
    std::cout << "執行緒數：" << numThreads << std::endl;
    std::cout << "總工作量：" << totalWork << std::endl << std::endl;
    
    std::cout << std::fixed << std::setprecision(2);
    
    double badTime = runTest(badApproach, numThreads, totalWork);
    std::cout << "差的做法（鎖內計算）：" << badTime << " ms" << std::endl;
    
    double goodTime = runTest(goodApproach, numThreads, totalWork);
    std::cout << "好的做法（鎖外計算）：" << goodTime << " ms" << std::endl;
    
    double betterTime = runTest(betterApproach, numThreads, totalWork);
    std::cout << "更好做法（批次累加）：" << betterTime << " ms" << std::endl;
    
    std::cout << std::endl;
    std::cout << "加速比（差 vs 好）：" << badTime / goodTime << "x" << std::endl;
    std::cout << "加速比（差 vs 更好）：" << badTime / betterTime << "x" << std::endl;
    std::cout << std::endl;
    std::cout << "→ 三者做的計算完全相同，差別只在「鎖住多少東西」。" << std::endl;
    std::cout << "  bad 把計算放進鎖內 → 4 條執行緒的計算排隊做 → 退化成單執行緒。" << std::endl;
    std::cout << "  good 與 better 差距不大，因為此例的計算量遠大於鎖的開銷，" << std::endl;
    std::cout << "  鎖已經不是瓶頸了（計算很輕時差距才會誇張，見 5.6-2）。" << std::endl;

    std::cout << "\n=== 日常實務 1：影像直方圖（每像素上鎖 vs 本地合併）===" << std::endl;
    {
        std::vector<int> pixels(2000000);
        for (size_t i = 0; i < pixels.size(); ++i) pixels[i] = static_cast<int>(i % 4096);

        long sum1 = 0, sum2 = 0;
        double tPerPixel = runHistogram(histogramPerPixelLock, pixels, 4, sum1);
        double tLocal    = runHistogram(histogramLocalMerge,   pixels, 4, sum2);

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "200 萬像素，4 條執行緒" << std::endl;
        std::cout << "每像素上鎖  ：" << tPerPixel << " ms（上鎖 2000000 次）" << std::endl;
        std::cout << "本地合併    ：" << tLocal    << " ms（上鎖 4 次）" << std::endl;
        std::cout << "兩者結果相同：" << (sum1 == sum2 ? "是" : "否")
                  << "（各為 " << sum1 << " 與 " << sum2 << "）" << std::endl;
        std::cout << "→ 正確性完全相同，效能差距來自上鎖次數" << std::endl;
    }

    std::cout << "\n=== 日常實務 2：鎖內的隱藏慢動作（字串與記憶體配置）===" << std::endl;
    {
        std::vector<int> records(400000);
        for (size_t i = 0; i < records.size(); ++i) records[i] = static_cast<int>(i % 1000);

        // 慢版：鎖內建字串 + map 配置
        ReportAggregator aggSlow(8);
        std::map<std::string, long> sharedMap;
        std::vector<std::thread> ths1;
        size_t chunk = records.size() / 4;
        auto s1 = std::chrono::steady_clock::now();
        for (int i = 0; i < 4; ++i) {
            size_t b = chunk * static_cast<size_t>(i);
            size_t e = (i == 3) ? records.size() : b + chunk;
            ths1.emplace_back([&aggSlow, &sharedMap, &records, b, e] {
                aggregateSlow(aggSlow, sharedMap, records, b, e);
            });
        }
        for (auto& t : ths1) t.join();
        auto e1 = std::chrono::steady_clock::now();
        double tSlow = std::chrono::duration<double, std::milli>(e1 - s1).count();

        // 快版：鎖外算索引 + 本地聚合
        ReportAggregator aggFast(8);
        std::vector<std::thread> ths2;
        auto s2 = std::chrono::steady_clock::now();
        for (int i = 0; i < 4; ++i) {
            size_t b = chunk * static_cast<size_t>(i);
            size_t e = (i == 3) ? records.size() : b + chunk;
            ths2.emplace_back([&aggFast, &records, b, e] {
                aggregateFast(aggFast, records, b, e);
            });
        }
        for (auto& t : ths2) t.join();
        auto e2 = std::chrono::steady_clock::now();
        double tFast = std::chrono::duration<double, std::milli>(e2 - s2).count();

        long slowTotal = 0;
        for (const auto& kv : sharedMap) slowTotal += kv.second;
        long fastTotal = 0;
        for (long v : aggFast.totals) fastTotal += v;

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "40 萬筆記錄，4 條執行緒" << std::endl;
        std::cout << "鎖內建字串 + map 配置：" << tSlow << " ms" << std::endl;
        std::cout << "鎖外算索引 + 本地聚合：" << tFast << " ms" << std::endl;
        std::cout << "兩者總和相同：" << (slowTotal == fastTotal ? "是" : "否")
                  << "（各為 " << slowTotal << " 與 " << fastTotal << "）" << std::endl;
        std::cout << "→ 「只在鎖內做累加」還不夠 ——" << std::endl;
        std::cout << "  字串建構與 map 的節點配置都是藏在鎖裡的慢動作。" << std::endl;
    }
    
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.6：互斥鎖的效能考量4.cpp' -o minimize_critical_section

// ⚠️ 本檔的【時間數字與加速比每次執行都不同】（效能量測，受頻率調節、排程、
// 負載影響），下面貼的是本機（Ubuntu 26.04 / g++ 15.2.0 / 16 核心）某一次的實測。
// 穩定成立的是【趨勢】：鎖內做計算最慢；把計算移出鎖有數倍提升；
// 上鎖次數從百萬級降到執行緒數時差距最誇張（本次實測 246 ms → 5.8 ms、439 ms → 1.4 ms）。
//
// 「兩者結果相同」與各項總和（2000000、199800000）則是確定值 ——
// 那是正確性驗證：效能改善不能改變結果。
// 註：第一段的 result 是 double，浮點加法不滿足結合律，
//     多執行緒下最低有效位可能每次略有不同，這不是 bug。

// === 預期輸出 ===
// === 臨界區段大小對效能的影響 ===
// 執行緒數：4
// 總工作量：1000
//
// 差的做法（鎖內計算）：26.36 ms
// 好的做法（鎖外計算）：6.39 ms
// 更好做法（批次累加）：6.64 ms
//
// 加速比（差 vs 好）：4.13x
// 加速比（差 vs 更好）：3.97x
//
// → 三者做的計算完全相同，差別只在「鎖住多少東西」。
//   bad 把計算放進鎖內 → 4 條執行緒的計算排隊做 → 退化成單執行緒。
//   good 與 better 差距不大，因為此例的計算量遠大於鎖的開銷，
//   鎖已經不是瓶頸了（計算很輕時差距才會誇張，見 5.6-2）。
//
// === 日常實務 1：影像直方圖（每像素上鎖 vs 本地合併）===
// 200 萬像素，4 條執行緒
// 每像素上鎖  ：262.42 ms（上鎖 2000000 次）
// 本地合併    ：5.81 ms（上鎖 4 次）
// 兩者結果相同：是（各為 2000000 與 2000000）
// → 正確性完全相同，效能差距來自上鎖次數
//
// === 日常實務 2：鎖內的隱藏慢動作（字串與記憶體配置）===
// 40 萬筆記錄，4 條執行緒
// 鎖內建字串 + map 配置：411.33 ms
// 鎖外算索引 + 本地聚合：1.45 ms
// 兩者總和相同：是（各為 199800000 與 199800000）
// → 「只在鎖內做累加」還不夠 ——
//   字串建構與 map 的節點配置都是藏在鎖裡的慢動作。
