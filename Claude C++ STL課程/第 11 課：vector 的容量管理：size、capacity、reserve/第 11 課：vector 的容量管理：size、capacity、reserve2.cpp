// =============================================================================
//  第 11 課：vector 的容量管理：size、capacity、reserve2.cpp
//  —  reserve(n)：預先配置空間，把 n 次擴容壓成 1 次
// =============================================================================
//
// 【主題資訊 Information】
//   void reserve(size_type n);
//
//   標頭檔：<vector>
//   標準版本：C++98 起；C++20 起為 constexpr。
//   複雜度：O(size())（需要把既有元素搬到新 buffer），只在真的重新配置時付出。
//   例外：n > max_size() 丟 std::length_error；配置失敗丟 std::bad_alloc。
//         若 T 的 move constructor 沒標 noexcept，重新配置會改用 copy
//         （為了強例外保證），成本更高 —— 見【概念補充】。
//
//   標準規定的語意（[vector.capacity]）：
//     * n >  capacity()：重新配置，使 capacity() >= n。
//     * n <= capacity()：**不做任何事**（標準明文規定，不是「通常不會」）。
//     * 永遠不改變 size()，永遠不建構或銷毀任何元素。
//     * reserve 只增不減，沒有任何情況會讓 capacity 變小。
//
// 【詳細解釋 Explanation】
//
// 【1. reserve 解決什麼問題】
// 不用 reserve 連續 push_back n 次，vector 會在容量滿時反覆「配置更大的
// buffer → 搬移全部舊元素 → 釋放舊 buffer」。雖然幾何成長讓總成本仍是攤提
// O(1)，但實際付出的代價是：多次 allocator 呼叫、多次全量搬移、記憶體碎片，
// 以及最糟的一點 —— **每次重新配置都讓所有 iterator/pointer/reference 失效**。
//
// reserve(n) 把這一切壓成「開頭一次配置」：之後 n 次 push_back 全都只是
// 在既有記憶體上建構元素，零搬移、零失效。
//
// 【2. reserve 不改變 size —— 這是最常被誤用的一點】
// reserve 只動 capacity，size 仍然是 0。那塊記憶體是「已配置但未建構」的
// 原始位元組，裡面沒有任何 int 物件存在。所以：
//     std::vector<int> v;
//     v.reserve(100);
//     v[0] = 5;        // ← UB！size() 是 0，v[0] 越界
// 正確的用法只有 push_back / emplace_back（讓 size 逐步長上去）。
// 想要「配置好且可以直接用 v[i] 存取」，你要的是 resize(100) 而不是
// reserve(100)（詳見 4.cpp 與 5.cpp）。
//
// 【3. reserve 配置多少？—— 本機實測「剛好 n」】
// 標準只要求 capacity() >= n。libstdc++ 實測是**剛好配置 n**，不會四捨五入
// 到 2 的冪次：reserve(100) 後 capacity 正好是 100（本檔輸出可驗證）。
// 這個細節有實際後果：reserve(100) 後推第 101 個元素會觸發重新配置，
// 而此時 libstdc++ 會用「2 倍成長」規則跳到 200。
//
// 【4. 該 reserve 多少？】
//   * 知道精確筆數（例如檔頭寫了記錄數）→ reserve(精確值)，最理想。
//   * 只知道上界（例如「篩選結果最多和輸入一樣多」）→ reserve(上界)，
//     代價是可能多佔一點記憶體，之後可視需要 shrink_to_fit()。
//   * 完全不知道 → **不要 reserve**。讓 vector 自己的幾何成長處理就好；
//     亂猜一個小數字反而讓你既付了 reserve 的配置成本、又照樣要擴容。
//
// 【概念補充 Concept Deep Dive】
// (A) 為什麼「幾何成長」能讓 push_back 攤提 O(1)
//   假設倍率是 k（libstdc++ 是 2），從 1 成長到 n 期間，被搬移的元素總數是
//   等比級數 1 + k + k² + … + n ≈ n·k/(k-1)，是 **Θ(n)** 而不是 Θ(n²)。
//   平均分攤到 n 次 push_back，每次就是 O(1) —— 這就是「攤提」的意思。
//   若改成「每次加固定 c 個」，總搬移量會是 n²/(2c) = Θ(n²)，就崩掉了。
//   本機實測：1,000,000 次 push_back 不 reserve 共觸發 21 次重新配置、
//   搬移 1,048,575 個元素（見 3.cpp）；reserve 後是 0 次。
//
// (B) 重新配置時是 move 還是 copy？
//   vector 的 push_back 提供**強例外保證**（要嘛成功、要嘛容器不變）。搬移
//   到新 buffer 時若 move constructor 可能拋例外，搬到一半失敗就無法還原
//   ——舊 buffer 的元素已經被搬空了。所以 libstdc++ 用
//   std::move_if_noexcept：只有當 T 的 move constructor 是 noexcept
//   （或 T 不可複製）時才 move，否則退回 **copy**。
//   實務結論：自訂型別的 move constructor 一定要標 noexcept，否則
//   vector 擴容會靜默地退化成深複製，效能差幾個數量級。
//
// (C) reserve 之後 capacity 的成長規則不會「記得」你 reserve 過
//   reserve(100) 之後推到第 101 個，libstdc++ 是以「當前 capacity 的 2 倍」
//   計算（100 → 200），不是回到 128 這種 2 的冪次。成長規則永遠是相對於
//   當前 capacity，與你當初 reserve 的數字無關。
//
// 【注意事項 Pay Attention】
// 1. **reserve 後所有 iterator / pointer / reference 全部失效**（只要真的
//    發生了重新配置）。在 reserve 前取得的 &v[0]、v.begin()、v.data()
//    在 reserve 後都不可再用。
// 2. reserve 只增不減。想縮小要用 shrink_to_fit()（見 6.cpp）或 swap trick
//    （見 8.cpp）。reserve(0) 什麼也不會發生。
// 3. reserve 不改變 size，也不建構元素 → reserve 後 v[0] 是 UB。
// 4. **不要在迴圈裡每輪 reserve(size()+1)**。因為 libstdc++ 的 reserve 是
//    「剛好配置 n」，這會關掉幾何成長，讓每次插入都重新配置 → 退化成 O(n²)。
//    本機實測：推 10 個元素，逐次 reserve 觸發 10 次重新配置，
//    什麼都不做的 push_back 只觸發 5 次（見面試題陷阱 2）。
// 5. n > max_size() 丟 std::length_error（不是回傳錯誤碼、也不是靜默失敗）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】reserve
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. reserve(n) 會改變 size() 嗎？reserve 之後可以直接 v[0] = x 嗎？
//     答：不會改變 size()，reserve 只動 capacity 且不建構任何元素。
//         reserve(100) 之後 size() 仍是 0，此時 v[0] 是越界存取 → UB。
//         要填資料只能 push_back / emplace_back；想要能直接 v[i] 存取，
//         該用的是 resize(n) 而不是 reserve(n)。
//     追問：那 reserve 之後那塊記憶體裡是什麼？
//         → 已配置但**未建構**的原始位元組，沒有任何 T 物件存在。
//
// 🔥 Q2. 呼叫 reserve(n) 時 n 小於目前 capacity 會發生什麼事？
//     答：標準明文規定**什麼都不做**（不是「通常不縮」而是保證不縮）。
//         reserve 只增不減，沒有任何情況會讓 capacity 變小。
//     追問：那要怎麼縮容？
//         → shrink_to_fit()（C++11，且是非強制請求），或 C++11 前的
//           swap trick：std::vector<T>(v).swap(v)。
//
// 🔥 Q3. 為什麼 push_back 能做到「攤提 O(1)」？
//     答：因為採幾何成長（libstdc++ 實測 2×）。從 1 長到 n 的總搬移量是等比
//         級數 ≈ 2n = Θ(n)，分攤到 n 次插入就是 O(1)。若改成每次固定加 c 個，
//         總搬移量變 Θ(n²)，攤提就不成立了。
//     追問：標準有規定倍率是 2 嗎？
//         → **沒有**。標準只要求「攤提 O(1)」。libstdc++ 實測 2×、
//           MSVC 是 1.5×，都合規。
//
// ⚠️ 陷阱 1. reserve 之後，先前取得的 iterator 還能用嗎？
//     答：不能。只要真的發生重新配置，所有 iterator / pointer / reference
//         全部失效，包括 v.begin()、&v[0]、v.data()。
//     為什麼會錯：直覺認為「我只是多要空間、又沒動元素，指標應該不變」。
//         但擴容是**換一塊新記憶體再搬過去**，舊位址已經被 free 掉了。
//
// ⚠️ 陷阱 2. 「reserve 一定比較快，所以每次 push_back 前都 reserve(size()+1)」
//     答：這是效能災難。libstdc++ 的 reserve 剛好配置 n，逐次 reserve 等於
//         把幾何成長改成「每次加 1」→ 每次插入都重新配置 → 退化成 O(n²)。
//         本機實測推 10 個元素：逐次 reserve 觸發 10 次重新配置，
//         而什麼都不做的 push_back 只觸發 5 次 —— reserve 反而更慢。
//     為什麼會錯：把 reserve 當成「效能開關」，以為呼叫越多次越快。
//         reserve 的價值來自**一次就要到最終大小**；不知道總量時，
//         什麼都不做才是對的。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>

int main() {
    std::vector<int> v;

    std::cout << "=== reserve 前 ===" << std::endl;
    std::cout << "初始 - size: " << v.size()
              << ", capacity: " << v.capacity() << std::endl;

    v.reserve(100);  // 預先配置至少 100 個元素的空間

    std::cout << "\n=== reserve(100) 後 ===" << std::endl;
    std::cout << "size: " << v.size()
              << ", capacity: " << v.capacity() << std::endl;
    // size 仍是 0（reserve 不建構元素），capacity >= 100

    // 記下 reserve 後的資料位址，稍後驗證 100 次 push_back 都沒有搬移
    const int* base = v.data();

    // 現在連續 push_back 100 次都不會觸發擴容
    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
    }

    std::cout << "\n=== 100 次 push_back 後 ===" << std::endl;
    std::cout << "size: " << v.size()
              << ", capacity: " << v.capacity() << std::endl;
    // 只印「位址有沒有變」，不印位址本身（位址每次執行都不同）
    std::cout << "資料位址是否搬移過: " << (v.data() != base ? "是" : "否")
              << std::endl;

    // reserve 只增不減：n <= capacity() 時標準規定什麼都不做
    v.reserve(10);
    std::cout << "\n=== reserve(10)（小於現有 capacity）===" << std::endl;
    std::cout << "capacity: " << v.capacity() << " （不縮小）" << std::endl;

    // 推第 101 個元素 → 超出 capacity → 觸發重新配置
    v.push_back(100);
    std::cout << "\n=== 推第 101 個元素（超出 capacity）===" << std::endl;
    std::cout << "size: " << v.size()
              << ", capacity: " << v.capacity() << std::endl;
    std::cout << "資料位址是否搬移過: " << (v.data() != base ? "是" : "否")
              << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 11 課：vector 的容量管理：size、capacity、reserve2.cpp" -o demo2

// 註：以下 capacity 具體數值為 libstdc++ / GCC 15.2 實測，非標準保證。
//     標準只保證 reserve(100) 後 capacity() >= 100；libstdc++ 剛好配置 100，
//     超出後以 2 倍成長到 200，MSVC（1.5×）會得到不同數字。

// === 預期輸出 ===
// === reserve 前 ===
// 初始 - size: 0, capacity: 0
//
// === reserve(100) 後 ===
// size: 0, capacity: 100
//
// === 100 次 push_back 後 ===
// size: 100, capacity: 100
// 資料位址是否搬移過: 否
//
// === reserve(10)（小於現有 capacity）===
// capacity: 100 （不縮小）
//
// === 推第 101 個元素（超出 capacity）===
// size: 101, capacity: 200
// 資料位址是否搬移過: 是
