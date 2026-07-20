// ### 十、效能比較
// =============================================================================
//  第 15 課：vector 元素刪除 12  —  O(n²) vs O(n)：把差距量出來
// =============================================================================
//
// 【主題資訊 Information】
//   本檔比較兩種「刪除所有偶數」的做法：
//     方法 1  迴圈逐一 erase          O(n²)
//     方法 2  Erase-Remove 慣用法      O(n)
//   相關 API：
//     iterator erase(const_iterator);              <vector>   O(n)
//     ForwardIt remove_if(first, last, pred);      <algorithm> O(n)
//   計時：std::chrono::steady_clock（C++11）
//   ⚠️ 耗時走 stderr，可重現的「操作次數」走 stdout。
//      理由：時間受 CPU 頻率與系統負載影響，每次都不同；
//      而「搬移了幾次元素」是完全確定的，才適合當預期輸出。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼逐一 erase 是 O(n²)】
//   單次 erase(pos) 是 O(n)：pos 之後的元素全部要往前搬一格。
//   若要刪掉 n/2 個元素，就要做 n/2 次這樣的搬移：
//       總搬移量 ≈ (n-1) + (n-2) + … ≈ n²/4
//   注意這不是「常數比較大」，而是【漸進複雜度不同】。
//   n 變成 10 倍，時間就變成大約 100 倍。
//
// 【2. 為什麼 erase-remove 是 O(n)】
//   remove_if 用兩個指標做一次線性掃描：
//     一個讀（掃過每個元素），一個寫（指向下一個保留位置）
//   保留的元素往前搬一次就定位，之後不會再被碰到。
//   所以每個元素【最多被搬移一次】，總搬移量是 O(n)。
//   最後的 erase(newEnd, end()) 因為 last 就是 end()，
//   後面沒有元素需要搬移，只要解構尾段。
//
// 【3. 這個差距在什麼規模開始有感】
//   n = 1000    ：n² 是 100 萬，仍在微秒等級，感覺不出來
//   n = 100000  ：n² 是 100 億，開始要數秒
//   n = 1000000 ：n² 是 10^12，實務上等於當掉
//   典型的災難模式是：開發時用幾百筆測資，一切正常；
//   上線後資料成長到十萬筆，某個功能突然「卡住」。
//   這類效能 bug 幾乎不可能靠單元測試發現，
//   因為它在小資料下是正確且快速的。
//
// 【4. 本檔用「操作次數」而非「時間」當主要證據】
//   時間會浮動，不同機器差好幾倍，也無法寫進預期輸出。
//   但「元素被搬移了幾次」是資料與演算法決定的，完全確定。
//   本檔用一個會計數賦值次數的型別，把兩種做法的搬移次數
//   實際數出來——那才是複雜度差異的直接證據。
//   時間仍然量測，但輸出到 stderr 當輔助參考。
//
// 【概念補充 Concept Deep Dive】
//
// (A) remove_if 的內部：讀寫雙指標
//     概念上等價於：
//         auto write = first;
//         for (auto read = first; read != last; ++read)
//             if (!pred(*read)) *write++ = std::move(*read);
//         return write;
//     這就是為什麼它是「穩定」的（保留元素維持原順序），
//     也是為什麼每個元素最多被搬一次。
//     注意 read == write 時 libstdc++ 會跳過自我賦值，
//     所以「前段完全不刪」時搬移次數會低於元素總數。
//
// (B) 為什麼不能靠「刪除的元素比較少」來救 O(n²)
//     逐一 erase 的成本是「刪除次數 × 剩餘元素數」。
//     只刪少數幾個（例如 5 個）確實還好，因為 k 很小。
//     但只要 k 與 n 同階（刪掉一半），就是 O(n²)。
//     判準是「k 會不會隨資料量成長」，不是「k 現在是多少」。
//
// (C) 更快的路：放棄保序
//     若不需要維持順序，swap-and-pop 可以讓【單一元素】的刪除
//     變成 O(1)（把最後一個搬過來再 pop_back）。
//     批次刪除時，unstable 版本的分割也比 remove_if 少搬一些資料。
//     這是典型的「用順序保證換效能」取捨，見本課第 13 檔。
//
// 【注意事項 Pay Attention】
//   1. 逐一 erase 是 O(n²)，不是「稍微慢一點」。差的是漸進複雜度。
//   2. 這類 bug 在小測資下完全測不出來——正確、而且很快。
//   3. 只刪少數幾個元素時逐一 erase 是可以接受的；
//      判準是「刪除數量會不會隨資料量一起成長」。
//   4. 效能量測要在 -O2 下進行，且不能讓結果被最佳化掉。
//   5. 本檔的耗時每次執行都不同，故走 stderr，不列入預期輸出。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】刪除操作的複雜度
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 在迴圈裡逐一 erase 刪掉 vector 的一半元素，複雜度是多少？
//     答：O(n²)。單次 erase 是 O(n)（要搬移後半段），
//         做 n/2 次就是 (n-1)+(n-2)+… ≈ n²/4。
//         這不是常數項偏大，而是漸進複雜度不同——
//         資料量變 10 倍，時間變約 100 倍。
//         正解是 erase-remove 慣用法（O(n)）或 C++20 的 std::erase_if。
//     追問：什麼情況下逐一 erase 是可以接受的？
//         → 當刪除數量 k 是常數、不隨 n 成長時（例如「刪掉那 3 筆」）。
//           成本是 O(k·n)，k 固定就等於 O(n)。
//           判準是「k 會不會跟著資料量一起長」。
//
// 🔥 Q2. erase-remove 為什麼能做到 O(n)？
//     答：remove_if 用讀寫雙指標做一次線性掃描——
//         讀指標掃過每個元素，寫指標指向下一個保留位置，
//         保留的元素往前搬一次就定位，之後不再被碰。
//         所以每個元素最多被搬移一次，總計 O(n)。
//         最後的 erase(newEnd, end()) 因為 last 就是 end()，
//         後面沒有元素要搬，只需解構尾段。
//     追問：那它為什麼是穩定的（保留原順序）？
//         → 因為讀指標從頭到尾單向掃描，寫入順序就是原始順序。
//           這是標準明文保證的。
//
// ⚠️ 陷阱. 「我用 1000 筆測資量過，逐一 erase 只花不到 1 毫秒，
//         完全沒有效能問題。」——這個結論的問題在哪？
//     答：問題在測資規模掩蓋了複雜度。1000 筆時 n² 是 100 萬，
//         現代 CPU 幾毫秒內就跑完，看起來毫無問題。
//         但複雜度是 O(n²)：資料成長到 10 萬筆時，
//         運算量變成 100 億——同一段程式碼會從「毫秒」變成「數秒」。
//         這正是最難發現的效能 bug：它在開發環境完全正常，
//         而且結果永遠是【正確的】，單元測試也全綠。
//         直到上線後資料自然成長，某天突然開始逾時。
//     為什麼會錯：把「在這個規模下夠快」當成「這段程式碼沒問題」。
//         效能評估要看的是【成長曲線】而不是單點的絕對值。
//         正確的做法是：量兩個以上的規模（例如 n 與 2n），
//         看時間是變 2 倍還是 4 倍——那才看得出複雜度。
//         本檔的 main 就是用兩種規模來呈現這條曲線。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <algorithm>
#include <chrono>

// -----------------------------------------------------------------------------
// 計數用型別：記錄「賦值」被呼叫幾次（= 元素被搬移的次數）。
// 這是複雜度差異最直接、而且完全可重現的證據。
// -----------------------------------------------------------------------------
struct Counted {
    int value;
    static long long assigns;
    Counted(int v = 0) : value(v) {}
    Counted(const Counted&) = default;
    Counted(Counted&&) noexcept = default;
    Counted& operator=(const Counted& o) { value = o.value; ++assigns; return *this; }
    Counted& operator=(Counted&& o) noexcept { value = o.value; ++assigns; return *this; }
};
long long Counted::assigns = 0;

static bool isEven(const Counted& c) { return c.value % 2 == 0; }

// 方法 1：迴圈逐一 erase —— O(n²)
long long countMovesLoopErase(int n) {
    std::vector<Counted> v;
    v.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) v.emplace_back(i);

    Counted::assigns = 0;
    for (auto it = v.begin(); it != v.end(); ) {
        if (isEven(*it)) it = v.erase(it);
        else             ++it;
    }
    return Counted::assigns;
}

// 方法 2：Erase-Remove 慣用法 —— O(n)
long long countMovesEraseRemove(int n) {
    std::vector<Counted> v;
    v.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) v.emplace_back(i);

    Counted::assigns = 0;
    v.erase(std::remove_if(v.begin(), v.end(), isEven), v.end());
    return Counted::assigns;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】訂單系統的「取消單清理」——一個真實的 O(n²) 災難
//   情境：電商後台每晚清理當日的取消訂單。開發時用測試環境的
//         幾百筆資料，跑得飛快；上線半年後訂單累積到數十萬筆，
//         這支排程開始跑不完，最後拖垮整個夜間批次。
//   為什麼用到本主題：這是 O(n²) 效能 bug 最典型的生命週期——
//     ① 結果永遠正確，測試不會失敗
//     ② 小資料下很快，開發期完全無感
//     ③ 資料自然成長到某個門檻後突然爆炸
//   下方兩個函式的【輸出結果完全相同】，只有複雜度不同。
//   這正是這類 bug 難以察覺的原因。
// -----------------------------------------------------------------------------
struct Order {
    int  id;
    bool cancelled;
};

// 災難版：O(n²)
void purgeCancelledSlow(std::vector<Order>& orders) {
    for (auto it = orders.begin(); it != orders.end(); ) {
        if (it->cancelled) it = orders.erase(it);
        else               ++it;
    }
}

// 正確版：O(n)
void purgeCancelledFast(std::vector<Order>& orders) {
    orders.erase(std::remove_if(orders.begin(), orders.end(),
                                [](const Order& o) { return o.cancelled; }),
                 orders.end());
}

int main() {
    const int N = 100000;

    // ---- 計時（非決定性，走 stderr）----
    auto start1 = std::chrono::steady_clock::now();
    {
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) v.push_back(i);

        for (auto it = v.begin(); it != v.end(); ) {
            if (*it % 2 == 0) {
                it = v.erase(it);
            } else {
                ++it;
            }
        }
    }
    auto end1 = std::chrono::steady_clock::now();

    auto start2 = std::chrono::steady_clock::now();
    {
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) v.push_back(i);

        v.erase(std::remove_if(v.begin(), v.end(),
                               [](int x) { return x % 2 == 0; }),
                v.end());
    }
    auto end2 = std::chrono::steady_clock::now();

    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);

    std::cerr << "[計時 | 每次執行都不同，故不列入預期輸出]\n";
    std::cerr << "  逐一 erase:      " << duration1.count() << " ms\n";
    std::cerr << "  Erase-Remove:    " << duration2.count() << " ms\n";

    // ---- 操作次數（完全確定，走 stdout）----
    std::cout << "=== 元素搬移次數（完全可重現，不受機器影響）===\n";
    std::cout << "測試：從 n 個元素中刪除所有偶數（剛好一半）\n\n";

    struct Row { int n; long long loop; long long er; };
    Row rows[] = {
        {1000,  countMovesLoopErase(1000),  countMovesEraseRemove(1000)},
        {2000,  countMovesLoopErase(2000),  countMovesEraseRemove(2000)},
        {4000,  countMovesLoopErase(4000),  countMovesEraseRemove(4000)},
        {8000,  countMovesLoopErase(8000),  countMovesEraseRemove(8000)},
    };

    std::cout << "     n | 逐一 erase 搬移次數 | Erase-Remove 搬移次數 | 倍數\n";
    std::cout << "-------+---------------------+-----------------------+------\n";
    for (const auto& r : rows) {
        std::cout.width(6); std::cout << r.n << " | ";
        std::cout.width(19); std::cout << r.loop << " | ";
        std::cout.width(21); std::cout << r.er << " | ";
        std::cout << (r.er > 0 ? r.loop / r.er : 0) << "x\n";
    }

    std::cout << "\n關鍵在「成長曲線」而不是單一數字：\n";
    std::cout << "  n 每加倍，逐一 erase 的搬移次數變約 "
              << (rows[0].loop > 0 ? static_cast<double>(rows[1].loop) / rows[0].loop : 0)
              << " 倍  → O(n²)\n";
    std::cout << "  n 每加倍，Erase-Remove 的搬移次數變約 "
              << (rows[0].er > 0 ? static_cast<double>(rows[1].er) / rows[0].er : 0)
              << " 倍  → O(n)\n";
    std::cout << "→ 這就是為什麼小測資測不出問題：n=1000 時差 "
              << (rows[0].er > 0 ? rows[0].loop / rows[0].er : 0)
              << " 倍還在毫秒等級，\n";
    std::cout << "  但差距本身會隨 n 一起成長。\n";

    std::cout << "\n=== 為什麼 Erase-Remove 的次數不等於 n ===\n";
    std::cout << "n=8000 時它只搬了 " << rows[3].er << " 次，不是 8000 次。\n";
    std::cout << "因為 remove_if 用讀寫雙指標，讀寫位置相同時會跳過自我賦值——\n";
    std::cout << "前面那些「還沒刪到任何東西」的元素完全不必搬。\n";
    std::cout << "實際搬移的是「第一個被刪元素之後、需要往前補位」的那些。\n";

    std::cout << "\n=== 日常實務：訂單系統的取消單清理 ===\n";
    {
        auto makeOrders = [](int n) {
            std::vector<Order> v;
            v.reserve(static_cast<size_t>(n));
            for (int i = 0; i < n; ++i) v.push_back({i, i % 3 == 0});
            return v;
        };

        auto slow = makeOrders(2000);
        auto fast = makeOrders(2000);
        purgeCancelledSlow(slow);
        purgeCancelledFast(fast);

        std::cout << "2000 筆訂單，其中 " << (2000 / 3 + 1) << " 筆已取消\n";
        std::cout << "災難版 O(n²) 剩餘: " << slow.size() << " 筆\n";
        std::cout << "正確版 O(n)  剩餘: " << fast.size() << " 筆\n";

        bool identical = (slow.size() == fast.size());
        for (size_t i = 0; identical && i < slow.size(); ++i) {
            if (slow[i].id != fast[i].id) identical = false;
        }
        std::cout << "兩者結果完全相同: " << std::boolalpha << identical << "\n";
        std::cout << "→ 這正是這類 bug 難以察覺的原因：\n";
        std::cout << "  輸出永遠正確，單元測試全綠，只有在資料量成長後\n";
        std::cout << "  才會以「排程跑不完」的形式爆發出來。\n";
        std::cout << "  程式碼審查時看到迴圈裡的 erase，就該直接提出來。\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 15 課：vector 元素刪除12.cpp -o demo12
// 要量測真實的時間差距請開最佳化:
//        g++ -std=c++17 -O2 -Wall -Wextra <本檔> -o demo12
// （stdout 的「搬移次數」不受最佳化等級影響，兩種設定下完全相同）

// === 預期輸出 ===
// === 元素搬移次數（完全可重現，不受機器影響）===
// 測試：從 n 個元素中刪除所有偶數（剛好一半）
//
//      n | 逐一 erase 搬移次數 | Erase-Remove 搬移次數 | 倍數
// -------+---------------------+-----------------------+------
//   1000 |              250000 |                   500 | 500x
//   2000 |             1000000 |                  1000 | 1000x
//   4000 |             4000000 |                  2000 | 2000x
//   8000 |            16000000 |                  4000 | 4000x
//
// 關鍵在「成長曲線」而不是單一數字：
//   n 每加倍，逐一 erase 的搬移次數變約 4 倍  → O(n²)
//   n 每加倍，Erase-Remove 的搬移次數變約 2 倍  → O(n)
// → 這就是為什麼小測資測不出問題：n=1000 時差 500 倍還在毫秒等級，
//   但差距本身會隨 n 一起成長。
//
// === 為什麼 Erase-Remove 的次數不等於 n ===
// n=8000 時它只搬了 4000 次，不是 8000 次。
// 因為 remove_if 用讀寫雙指標，讀寫位置相同時會跳過自我賦值——
// 前面那些「還沒刪到任何東西」的元素完全不必搬。
// 實際搬移的是「第一個被刪元素之後、需要往前補位」的那些。
//
// === 日常實務：訂單系統的取消單清理 ===
// 2000 筆訂單，其中 667 筆已取消
// 災難版 O(n²) 剩餘: 1333 筆
// 正確版 O(n)  剩餘: 1333 筆
// 兩者結果完全相同: true
// → 這正是這類 bug 難以察覺的原因：
//   輸出永遠正確，單元測試全綠，只有在資料量成長後
//   才會以「排程跑不完」的形式爆發出來。
//   程式碼審查時看到迴圈裡的 erase，就該直接提出來。
