// =============================================================================
//  第 11 課：vector 的容量管理：size、capacity、reserve3.cpp
//  —  reserve 的效能意義：把「多次配置 + 全量搬移」壓成一次配置
// =============================================================================
//
// 【主題資訊 Information】
//   本檔測量的不是牆鐘時間，而是**確定性的成本指標**：
//     重新配置次數（reallocation count）與被搬移的元素總數（elements moved）。
//
//   標頭檔：<vector>、<iostream>
//   標準版本：C++98 起（reserve / capacity）。
//   關鍵複雜度：
//     * push_back 攤提 O(1)（標準保證），單次最壞 O(size())（觸發重新配置時）。
//     * 不 reserve 連續插入 n 個：總搬移量 Θ(n)，但伴隨 Θ(log n) 次配置。
//     * 先 reserve(n) 再插入 n 個：0 次重新配置、0 次搬移。
//
//   為什麼不直接印執行時間？
//     牆鐘時間受 CPU 排程、頻率調節、快取狀態、其他行程干擾影響，
//     **每次執行都不同**，無法作為可重現的預期輸出。重新配置次數與搬移
//     元素數則是由成長規則決定的確定值，才是真正該教的東西。
//     （本機參考測量見檔尾【實測時間】，僅供比例參考。）
//
// 【詳細解釋 Explanation】
//
// 【1. 成本到底花在哪裡】
// 不 reserve 連續 push_back，每次容量滿時 vector 會做三件事：
//     ① 向 allocator 要一塊更大的記憶體
//     ② 把舊 buffer 的**全部**元素搬到新 buffer（move 或 copy）
//     ③ 銷毀舊元素、釋放舊 buffer
// 這三件事的成本依序是：系統配置器呼叫（可能觸發 mmap/brk）、O(size()) 的
// 搬移、以及釋放。reserve(n) 讓 ①②③ 只在開頭發生一次。
//
// 【2. 為什麼「總搬移量 Θ(n)」還是值得優化】
// 幾何成長讓總搬移量是 Θ(n)（見 2.cpp【概念補充】），所以不 reserve 也還是
// 攤提 O(1) —— 這是常被拿來說「reserve 沒必要」的論點。但實際上：
//   * 常數項差很多：本機實測 100 萬次 push_back 會多搬 1,048,575 個元素，
//     等於整份資料被多複製了一遍以上。
//   * 配置器呼叫不是免費的：21 次 malloc/free，大塊配置還可能走 mmap。
//   * **iterator 失效**：每次重新配置都讓所有 iterator/pointer/reference
//     失效。這往往比效能更致命 —— 是正確性問題，不只是速度問題。
//   * 記憶體尖峰：搬移瞬間新舊 buffer 同時存在，峰值記憶體是 1.5～3 倍。
//
// 【3. 什麼時候 reserve 幫不上忙、甚至有害】
//   * 不知道總量卻硬猜一個小數字 → 照樣擴容，還多付一次配置。
//   * 在迴圈裡每輪 reserve(size()+1) → 關掉幾何成長，退化成 O(n²)（見 2.cpp）。
//   * 元素很少（幾十個）→ 差異在雜訊等級，可讀性優先。
//   * 過度 reserve（例如猜 100 萬結果只放 10 個）→ 白白佔住記憶體，
//     此時該搭配 shrink_to_fit()（見 6.cpp；注意它是 **non-binding request**，
//     標準不保證真的縮容）。
//
// 【概念補充 Concept Deep Dive】
// (A) 21 次重新配置是怎麼來的
//   libstdc++ 從 0 開始的容量序列是 0→1→2→4→8→…（實測 2× 成長）。
//   要容納 1,000,000 個元素需要成長到 2²⁰ = 1,048,576，
//   從 1 開始翻倍 20 次，加上第一次「0 → 1」的配置，共 21 次。
//   被搬移的元素總數是 1+2+4+…+524288 = 2²⁰ - 1 = 1,048,575，
//   正好是「幾何級數和 ≈ 最終大小」這個理論結果的實證。
//
// (B) 搬移是 move 還是 copy —— 決定實際代價的關鍵
//   本檔用 int，搬移就是 memcpy，很快。但若元素是含堆積配置的型別
//   （std::string、自訂類別），且 move constructor **沒有標 noexcept**，
//   vector 為了維持強例外保證會退回 **深複製**（std::move_if_noexcept）。
//   此時「多搬一遍」就從 memcpy 變成 100 萬次堆積配置 —— 差好幾個數量級。
//   實務結論：自訂型別的 move constructor 一定要標 noexcept。
//
// (C) 為什麼倍率不是越大越好
//   倍率越大，重新配置次數越少，但記憶體浪費越多（最差情況浪費接近
//   (k-1)/k）。另有一個經典論點：倍率 < 黃金比例 φ≈1.618 時，先前釋放的
//   記憶體區塊有機會被後續配置重複利用；2× 則永遠無法重用前面所有區塊的
//   總和。這是 MSVC 選 1.5× 的理由之一。兩者都合規，因為標準只要求攤提 O(1)。
//
// 【注意事項 Pay Attention】
// 1. 本檔輸出的重新配置次數與搬移數是 libstdc++（2× 成長）實測值。
//    MSVC 用 1.5× 成長，次數會更多、單次搬移量更小 —— **標準未規定倍率**，
//    只規定 push_back 為攤提 O(1)。
// 2. 牆鐘時間**每次執行都不同**，不可寫成固定值，也不該當作單元測試斷言。
// 3. 效能結論一定要在 **-O2** 下測。-O0 的 std::vector 有大量未內聯的函式
//    呼叫，會把差異稀釋掉（本機實測 -O0 差距明顯小於 -O2）。
// 4. 只在真的量測過、且該處確實是熱點時才加 reserve。憑感覺散佈 reserve
//    是典型的過早最佳化。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】reserve 的效能意義
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 既然 push_back 已經是攤提 O(1)，為什麼還要 reserve？
//     答：攤提 O(1) 說的是**漸進**成本，沒說常數項。實測插入 100 萬個 int，
//         不 reserve 要 21 次重新配置、多搬 1,048,575 個元素（等於整份資料
//         被多複製一遍以上），還有 21 次配置器呼叫與記憶體尖峰。
//         更重要的是每次重新配置都會讓所有 iterator/pointer/reference 失效
//         —— 那是正確性問題，不只是速度問題。
//     追問：那什麼時候不該 reserve？
//         → 不知道總量時。硬猜一個小數字只會多付一次配置卻照樣擴容；
//           在迴圈裡逐次 reserve 更會退化成 O(n²)。
//
// 🔥 Q2. 插入 n 個元素、成長倍率 k，總共搬移多少元素？為什麼是攤提 O(1)？
//     答：搬移量是等比級數 1+k+k²+…+n ≈ n·k/(k-1) = Θ(n)。分攤到 n 次插入
//         就是每次 O(1)。若改成「每次固定加 c 個」，總搬移量變 Θ(n²)，
//         攤提就不成立 —— 這正是幾何成長不可或缺的原因。
//     追問：k=2 時常數是多少？
//         → n·2/(2-1) = 2n，也就是「大約整份資料被多搬一遍」，
//           與本機實測 1,048,575 ≈ 1,000,000 吻合。
//
// ⚠️ 陷阱 1. 「reserve 之後 push_back 就不會失效 iterator 了」——完整嗎？
//     答：不完整。reserve(n) 只保證在 size 長到 n 之前不失效。一旦 size
//         超過那個 n，下一次 push_back 照樣重新配置、照樣全部失效。
//     為什麼會錯：把 reserve 當成「永久免疫」，忘記它只是把失效點往後推。
//
// ⚠️ 陷阱 2. 用 -O0 測出「reserve 只快一點點，所以沒用」。
//     答：-O0 下 vector 的成員函式都沒有內聯，每次 push_back 的固定開銷
//         淹沒了重新配置的差異。效能結論必須在 -O2 以上量測。
//     為什麼會錯：拿 debug build 的數字下最佳化結論。debug build 測的是
//         「未內聯的函式呼叫成本」，不是演算法成本。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>

// -----------------------------------------------------------------------------
// 量測工具：連續插入 n 個元素，回報「重新配置次數」與「被搬移的元素總數」。
// 原理：capacity() 改變 <=> 發生了一次重新配置，而該次搬移的元素數
//       正好是「插入前的 size()」。這兩個量都由成長規則決定，完全確定性。
// -----------------------------------------------------------------------------
struct GrowthCost {
    int         reallocations;   // 重新配置次數
    long long   elementsMoved;   // 被搬移的元素總數
    std::size_t finalCapacity;   // 最終 capacity
};

GrowthCost measurePushBack(int n, bool useReserve) {
    std::vector<int> v;
    if (useReserve) {
        v.reserve(static_cast<std::size_t>(n));
    }

    GrowthCost cost{0, 0, 0};
    std::size_t lastCap = v.capacity();

    for (int i = 0; i < n; ++i) {
        // 容量已滿 → 這次 push_back 必定觸發重新配置，
        // 搬移的元素數就是目前的 size()
        if (v.size() == v.capacity()) {
            ++cost.reallocations;
            cost.elementsMoved += static_cast<long long>(v.size());
        }
        v.push_back(i);
        lastCap = v.capacity();
    }

    cost.finalCapacity = lastCap;
    return cost;
}

int main() {
    const int N = 1000000;

    std::cout << "=== 容量成長序列（前 8 次重新配置）===" << std::endl;
    {
        std::vector<int> v;
        std::size_t last = v.capacity();
        std::cout << "起始 capacity: " << last << std::endl;
        for (int i = 0; i < 200; ++i) {
            v.push_back(i);
            if (v.capacity() != last) {
                last = v.capacity();
                std::cout << "  size=" << v.size() << " -> capacity=" << last
                          << std::endl;
            }
        }
    }

    std::cout << "\n=== 插入 " << N << " 個元素的確定性成本 ===" << std::endl;

    GrowthCost without = measurePushBack(N, false);
    GrowthCost with    = measurePushBack(N, true);

    std::cout << "不使用 reserve:" << std::endl;
    std::cout << "  重新配置次數: " << without.reallocations << std::endl;
    std::cout << "  搬移元素總數: " << without.elementsMoved << std::endl;
    std::cout << "  最終 capacity: " << without.finalCapacity << std::endl;

    std::cout << "使用 reserve(" << N << "):" << std::endl;
    std::cout << "  重新配置次數: " << with.reallocations << std::endl;
    std::cout << "  搬移元素總數: " << with.elementsMoved << std::endl;
    std::cout << "  最終 capacity: " << with.finalCapacity << std::endl;

    std::cout << "\n=== 記憶體浪費比較 ===" << std::endl;
    // 不 reserve 時容量會超過需求（2 的冪次），reserve 則剛好
    std::cout << "不使用 reserve 多配置了 "
              << (without.finalCapacity - static_cast<std::size_t>(N))
              << " 個元素的空間" << std::endl;
    std::cout << "使用 reserve   多配置了 "
              << (with.finalCapacity - static_cast<std::size_t>(N))
              << " 個元素的空間" << std::endl;

    // -------------------------------------------------------------------------
    // 【日常實務範例】批次匯入 CSV：檔頭已宣告筆數 → 一次 reserve 到位
    //   真實情境：資料倉儲的匯入工具，檔頭會寫明本檔記錄數（或先用
    //   檔案大小 / 平均列長估上界）。已知筆數卻不 reserve，是最常見的
    //   「白白多搬一遍資料」浪費。
    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務: 批次匯入已知筆數 ===" << std::endl;
    {
        const int declaredRows = 50000;  // 假設檔頭宣告了 50000 筆

        GrowthCost naive = measurePushBack(declaredRows, false);
        GrowthCost smart = measurePushBack(declaredRows, true);

        std::cout << "匯入 " << declaredRows << " 筆記錄" << std::endl;
        std::cout << "  未讀檔頭直接推: 重新配置 " << naive.reallocations
                  << " 次, 搬移 " << naive.elementsMoved << " 筆" << std::endl;
        std::cout << "  依檔頭 reserve: 重新配置 " << smart.reallocations
                  << " 次, 搬移 " << smart.elementsMoved << " 筆" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 11 課：vector 的容量管理：size、capacity、reserve3.cpp" -o demo3
//   （效能結論請另用 -O2 量測；本檔輸出的是確定性計數，與最佳化等級無關）

// 【實測時間】本機 Dell Precision 7550 / GCC 15.2 / -O2，插入 100 萬個 int：
//     不使用 reserve: 約 1700–2500 微秒
//     使用 reserve:   約 1100–1190 微秒
//   同一份程式 -O0 重測：8648 / 7380 微秒（差距被未內聯的呼叫開銷稀釋）。
//   **牆鐘時間每次執行都不同**，上列僅供比例參考，不可寫進測試斷言。
//
// 註：以下重新配置次數與 capacity 為 libstdc++ / GCC 15.2（2× 成長）實測，
//     非標準保證。MSVC 採 1.5× 成長，次數與數值都會不同；標準只要求
//     push_back 為攤提 O(1)。

// === 預期輸出 ===
// === 容量成長序列（前 8 次重新配置）===
// 起始 capacity: 0
//   size=1 -> capacity=1
//   size=2 -> capacity=2
//   size=3 -> capacity=4
//   size=5 -> capacity=8
//   size=9 -> capacity=16
//   size=17 -> capacity=32
//   size=33 -> capacity=64
//   size=65 -> capacity=128
//   size=129 -> capacity=256
//
// === 插入 1000000 個元素的確定性成本 ===
// 不使用 reserve:
//   重新配置次數: 21
//   搬移元素總數: 1048575
//   最終 capacity: 1048576
// 使用 reserve(1000000):
//   重新配置次數: 0
//   搬移元素總數: 0
//   最終 capacity: 1000000
//
// === 記憶體浪費比較 ===
// 不使用 reserve 多配置了 48576 個元素的空間
// 使用 reserve   多配置了 0 個元素的空間
//
// === 日常實務: 批次匯入已知筆數 ===
// 匯入 50000 筆記錄
//   未讀檔頭直接推: 重新配置 17 次, 搬移 65535 筆
//   依檔頭 reserve: 重新配置 0 次, 搬移 0 筆
