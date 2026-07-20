// =============================================================================
//  第 17 課-11：實測 reserve 的效能差異 —— 以及怎麼把量測做對
// =============================================================================
//
// 【主題資訊 Information】
//   std::chrono::high_resolution_clock::now()      // 取時間點
//   std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
//   標準版本：<chrono> 是 C++11 加入；
//             C++20 起建議改用 std::chrono::steady_clock（保證單調遞增）
//   標頭檔：<chrono>
//   注意：high_resolution_clock 在多數實作上就是 steady_clock 或
//         system_clock 的別名，標準並未保證它是最高精度或單調的。
//
// 【詳細解釋 Explanation】
//
// 【1. 這個實驗到底在量什麼】
//   兩段程式碼唯一的差別是有沒有 reserve(N)。
//   不用 reserve 時，插入 N 個元素會經歷約 log₂(N) 次重新配置，
//   每次都要 malloc 一塊更大的記憶體、memcpy 全部既有元素、free 舊的。
//   總搬移量約 2N 個元素。用了 reserve 就是一次配置、零搬移。
//   量到的差距，就是「重複配置 + 元素搬移」的總成本。
//
// 【2. 為什麼差距通常沒有想像中大】
//   很多人預期會差好幾倍，實測往往只差一到兩成。原因有三：
//     (a) 對 int 這種 trivially copyable 型別，搬移就是 memcpy，
//         現代 CPU 每秒可搬數十 GB，2N 次搬移攤下來很便宜；
//     (b) 倍率成長讓重新配置只有 log₂(N) 次，N=10⁷ 也才 24 次；
//     (c) 迴圈本身的 push_back（邊界檢查 + 建構 + size 遞增）
//         才是主要成本，配置只佔一小部分。
//   換成 std::string 或含堆積成員的類別，差距就會明顯放大——
//   因為那時每次搬移都是真正的建構/解構，而不只是 memcpy。
//
// 【3. 這種微效能量測的三個經典陷阱】
//   (a) 編譯器把整段程式最佳化掉：
//       若結果沒被使用，-O2 可能直接刪掉整個迴圈，量到 0 ms。
//       本檔用 v.size() 累加成 sink 變數並印出來，確保迴圈不會被消除。
//   (b) 只跑一次：
//       單次量測會被 CPU 頻率調節、快取狀態、其他行程干擾。
//       本檔跑多輪並取最小值——最小值比平均值更能代表「純計算成本」，
//       因為雜訊只會讓時間變長、不會讓它變短。
//   (c) 順序效應：
//       先跑的那一段要負責把記憶體從作業系統要進來（page fault），
//       後跑的可能撿到現成的。本檔先做一輪 warm-up 再正式量測。
//
// 【4. 為什麼結果不能寫進「預期輸出」當成固定值】
//   耗時取決於 CPU 型號、當下負載、頻率調節、記憶體狀態，
//   每次執行都不同。本檔的預期輸出只保留「確定性的結論」
//   （reserve 版本的重新配置次數為 0），
//   實際毫秒數以「每次執行都不同」標註，僅供參考。
//
// 【概念補充 Concept Deep Dive】
//   ▸ 為什麼要用 steady_clock 而不是 system_clock
//     system_clock 對應「牆上時鐘」，可能被 NTP 校時或使用者調整而倒退，
//     用它量時間差可能得到負數。
//     steady_clock 保證單調遞增，是量測間隔的正確選擇。
//     high_resolution_clock 只是個別名，標準沒保證它是哪一個——
//     這也是 C++20 之後建議直接寫 steady_clock 的原因。
//   ▸ 為什麼「取最小值」而不是「取平均」
//     量測誤差在這裡是單向的：中斷、快取失效、頻率降低
//     都只會讓時間變長。因此最小值最接近「沒有雜訊時的真實成本」。
//     平均值會被偶發的大延遲拉高，反而失真。
//   ▸ 真正嚴謹的做法
//     生產環境的效能比較應使用 Google Benchmark 這類框架，
//     它會自動決定迭代次數、處理 warm-up、做統計檢定，
//     並提供 benchmark::DoNotOptimize 防止最佳化消除。
//     本檔的手工量測只用於教學示意。
//
// 【注意事項 Pay Attention】
//   1. 耗時數值每次執行都不同，不可當成固定的預期輸出。
//   2. 量測時要防止編譯器最佳化掉受測程式碼（本檔用 sink 變數）。
//   3. 用 steady_clock 而非 system_clock；後者可能被校時而倒退。
//   4. -O0 與 -O2 的結果差異極大，比較時務必註明最佳化等級。
//   5. 對 trivially copyable 型別，reserve 的效益比直覺小得多；
//      對含堆積資源的型別才會明顯。
//   6. 單次量測沒有意義，至少要多輪取最小值並先 warm-up。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】效能量測與 reserve 的實際效益
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼量測效能時應該用 steady_clock 而不是 system_clock？
//     答：system_clock 對應牆上時鐘，可能因 NTP 校時或使用者調整
//         而向前跳或向後退，兩個時間點相減甚至可能得到負值。
//         steady_clock 保證單調遞增，專為量測時間間隔而設計。
//         high_resolution_clock 只是其中之一的別名，
//         標準未規定是哪個，所以 C++20 起建議直接寫 steady_clock。
//     追問：那什麼時候該用 system_clock？
//         → 需要「真實世界時間」時，例如寫入 log 時間戳、
//           計算到某個日期還有多久。它才能與 time_t / 日曆互轉。
//
// 🔥 Q2. reserve 對 vector<int> 的效能提升通常沒有想像中大，為什麼？
//     答：三個原因。(1) 倍率成長讓重新配置只有 log₂(N) 次，
//         N=10⁷ 也不過 24 次；(2) int 是 trivially copyable，
//         搬移就是 memcpy，現代 CPU 每秒可搬數十 GB；
//         (3) push_back 本身的成本（容量檢查、建構、size 遞增）
//         才是迴圈的主要開銷。
//     追問：那什麼情況下 reserve 的效益會很明顯？
//         → 元素型別持有堆積資源時（std::string、含 vector 成員的類別），
//           每次搬移都是真正的移動或複製建構；
//           以及對延遲敏感的系統——reserve 消除的是
//           「偶發的 O(n) 長尾延遲」，那比平均吞吐更重要。
//
// ⚠️ 陷阱. 用 -O2 量測時，某段迴圈跑出 0 ms，是不是代表它極快？
//     答：更可能是整段迴圈被編譯器最佳化掉了。
//         若迴圈的結果沒有被任何地方使用，最佳化器可以合法地
//         判定它沒有可觀察的副作用而整段刪除。
//         量到的 0 ms 是「什麼都沒執行」，不是「執行得很快」。
//     為什麼會錯：以為編譯器只會做「等價的加速」。
//         實際上 as-if 規則允許它刪掉任何不影響可觀察行為的程式碼。
//         防範方式是把結果餵給一個會被印出的變數（本檔的 sink），
//         或使用 benchmark::DoNotOptimize 這類專用屏障。
// ═══════════════════════════════════════════════════════════════════════════

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

// 用 steady_clock（保證單調遞增）量一段工作的耗時，單位毫秒。
// fn 必須回傳一個值，交給呼叫端累加進 sink，避免整段被最佳化消除。
template <typename Fn>
long long timeOnceMs(Fn&& fn, unsigned long long& sink) {
    auto start = std::chrono::steady_clock::now();
    sink += fn();
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

// 多輪取最小值：雜訊只會讓時間變長，所以最小值最接近真實成本
template <typename Fn>
long long bestOfMs(int rounds, Fn&& fn, unsigned long long& sink) {
    long long best = -1;
    for (int i = 0; i < rounds; ++i) {
        long long ms = timeOnceMs(fn, sink);
        if (best < 0 || ms < best) best = ms;
    }
    return best;
}

int main() {
    const int N = 10'000'000;
    unsigned long long sink = 0;      // 防最佳化：所有結果都累加到這裡並印出

    std::cout << "=== 一、確定性結論：重新配置次數 ===" << std::endl;
    {
        std::vector<int> v;
        int reallocs = 0;
        std::size_t prev = 0;
        for (int i = 0; i < N; ++i) {
            v.push_back(i);
            if (v.capacity() != prev) { ++reallocs; prev = v.capacity(); }
        }
        std::cout << "不用 reserve：重新配置 " << reallocs
                  << " 次，最終 capacity = " << v.capacity() << std::endl;
    }
    {
        std::vector<int> v;
        v.reserve(N);
        int reallocs = 0;
        std::size_t prev = v.capacity();
        for (int i = 0; i < N; ++i) {
            v.push_back(i);
            if (v.capacity() != prev) { ++reallocs; prev = v.capacity(); }
        }
        std::cout << "使用 reserve：重新配置 " << reallocs
                  << " 次，最終 capacity = " << v.capacity() << std::endl;
    }
    std::cout << "↑ 這兩行是確定性的，每次執行都相同" << std::endl;

    // warm-up：讓記憶體與快取先進入穩定狀態，避免第一輪吃到 page fault 成本
    {
        std::vector<int> warm;
        warm.reserve(N);
        for (int i = 0; i < N; ++i) warm.push_back(i);
        sink += warm.size();
    }

    std::cout << "\n=== 二、耗時量測（int，trivially copyable）===" << std::endl;
    long long noReserve = bestOfMs(3, [&] {
        std::vector<int> v;
        for (int i = 0; i < N; ++i) v.push_back(i);
        return static_cast<unsigned long long>(v.size());
    }, sink);

    long long withReserve = bestOfMs(3, [&] {
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) v.push_back(i);
        return static_cast<unsigned long long>(v.size());
    }, sink);

    std::cout << "不用 reserve：" << noReserve   << " ms（3 輪取最小）" << std::endl;
    std::cout << "使用 reserve：" << withReserve << " ms（3 輪取最小）" << std::endl;

    std::cout << "\n=== 三、換成 std::string，差距明顯放大 ===" << std::endl;
    const int M = 1'000'000;
    long long strNo = bestOfMs(3, [&] {
        std::vector<std::string> v;
        for (int i = 0; i < M; ++i) v.push_back("item_" + std::to_string(i));
        return static_cast<unsigned long long>(v.size());
    }, sink);

    long long strYes = bestOfMs(3, [&] {
        std::vector<std::string> v;
        v.reserve(static_cast<std::size_t>(M));
        for (int i = 0; i < M; ++i) v.push_back("item_" + std::to_string(i));
        return static_cast<unsigned long long>(v.size());
    }, sink);

    std::cout << "string 不用 reserve：" << strNo  << " ms" << std::endl;
    std::cout << "string 使用 reserve：" << strYes << " ms" << std::endl;

    std::cout << "\n（sink = " << sink << "，僅用於阻止編譯器最佳化掉上述迴圈）"
              << std::endl;
    std::cout << "【提醒】以上毫秒數每次執行都不同，取決於 CPU 型號、當下負載、"
              << "頻率調節與最佳化等級，不可當成固定結果。" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 17 課：vector 的記憶體重新配置機制11.cpp" -o reserve_bench
// 最佳化版本比較: g++ -std=c++17 -O2 -Wall -Wextra "第 17 課：vector 的記憶體重新配置機制11.cpp" -o reserve_bench_O2
//
// 【關於下方預期輸出的但書】
//   ▸ 只有「第一段的重新配置次數與 capacity」是確定性的，每次執行都相同。
//     那兩個數值本身仍屬實作定義（本機 GCC 15.2 / libstdc++，2 倍成長）。
//   ▸ 第二、三段的所有毫秒數「每次執行都不同」——它們取決於
//     CPU 型號、當下系統負載、頻率調節、記憶體狀態與最佳化等級。
//     下方數值僅為本機某一次 -O0 建置的執行結果，不具重現性，
//     不可作為驗收依據；請以「reserve 版本較快」這個相對關係為準。
//   ▸ sink 的數值是確定性的（各段元素數的總和），
//     它存在的唯一目的是阻止編譯器把受測迴圈最佳化掉。

// === 預期輸出 ===
// === 一、確定性結論：重新配置次數 ===
// 不用 reserve：重新配置 25 次，最終 capacity = 16777216
// 使用 reserve：重新配置 0 次，最終 capacity = 10000000
// ↑ 這兩行是確定性的，每次執行都相同
//
// === 二、耗時量測（int，trivially copyable）===
// 不用 reserve：96 ms（3 輪取最小）
// 使用 reserve：75 ms（3 輪取最小）
//
// === 三、換成 std::string，差距明顯放大 ===
// string 不用 reserve：477 ms
// string 使用 reserve：377 ms
//
// （sink = 76000000，僅用於阻止編譯器最佳化掉上述迴圈）
// 【提醒】以上毫秒數每次執行都不同，取決於 CPU 型號、當下負載、頻率調節與最佳化等級，不可當成固定結果。
