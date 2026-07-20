// =============================================================================
//  第 17 課-9：reserve() —— 用一次配置換掉十幾次重新配置
// =============================================================================
//
// 【主題資訊 Information】
//   void vector<T>::reserve(size_type new_cap);
//     new_cap <= capacity()  → 不做任何事
//     new_cap >  capacity()  → 配置新記憶體、搬移全部元素、釋放舊記憶體
//                              （所有 iterator / pointer / reference 失效）
//     new_cap >  max_size()  → 丟出 std::length_error
//   標準版本：C++98 起
//   複雜度：O(size())，最多一次重新配置
//   保證：reserve 之後，只要 size() 不超過 new_cap，
//         後續的 insert / push_back 都不會再重新配置
//   標頭檔：<vector>
//
// 【詳細解釋 Explanation】
//
// 【1. 倍率成長：為什麼 n 次 push_back 只需要 O(log n) 次重新配置】
//   vector 滿了不是「多要一格」，而是「容量乘上一個固定倍率」。
//   libstdc++ 與 libc++ 用 2 倍，MSVC 用 1.5 倍（皆為實作定義）。
//   容量序列因此是 1, 2, 4, 8, 16, ...，要容納 n 個元素只需
//   約 log₂(n) 次重新配置。
//   本機（GCC 15.2 / libstdc++）實測：push_back 一萬個 int
//   會經歷 1→2→4→…→16384，共 15 次重新配置。
//
// 【2. 均攤 O(1) 的數學推導】
//   以 2 倍成長為例，插入 n 個元素的總搬移次數是
//     1 + 2 + 4 + ... + n/2 + n  ≈ 2n
//   （等比級數和小於首項的兩倍）。
//   平均分攤到每次 push_back 就是常數 —— 這就是「均攤 O(1)」。
//   注意這是「均攤」不是「最壞」：某一次特定的 push_back 仍可能
//   觸發 O(n) 的搬移。對延遲敏感的即時系統（遊戲主迴圈、交易系統），
//   這個偶發的長尾延遲正是必須 reserve 的理由。
//
// 【3. 為什麼倍率成長而不是固定增量】
//   若每次只加固定 k 格，插入 n 個元素要重新配置 n/k 次，
//   總搬移量是 k + 2k + 3k + ... ≈ n²/(2k)，也就是 O(n²)。
//   倍率成長把它降到 O(n)。代價是記憶體可能浪費近一半
//   （剛擴容完 size 只有 capacity 的一半）——用空間換時間。
//
// 【4. 為什麼 2 倍不見得是最佳倍率】
//   有個經典論點：用 2 倍時，新容量恆大於之前所有已釋放區塊的總和，
//   因此永遠無法重複利用先前釋放的記憶體（1+2+4 < 8）。
//   小於黃金比例 φ≈1.618 的倍率（例如 MSVC 的 1.5）在理論上
//   可以讓配置器有機會回收再利用前面釋放的空間。
//   實務上差異取決於配置器實作，兩派都有支持者——
//   重點是知道「倍率是實作定義的，不要寫死假設」。
//
// 【5. reserve 只動 capacity，不動 size】
//   reserve(10000) 之後 size() 仍然是 0，v[0] 依然是越界存取。
//   要「同時建立元素」是 resize(n)，它會 value-initialize n 個元素、
//   size 也變成 n。混淆這兩者是很常見的錯誤：
//     v.reserve(10); v[3] = 5;    // UB！size 還是 0
//     v.resize(10);  v[3] = 5;    // OK，10 個元素都已建構
//
// 【概念補充 Concept Deep Dive】
//   ▸ 重新配置的完整五步驟
//       1) 配置一塊 new_cap 大小的新記憶體
//       2) 把舊元素逐一搬到新記憶體（移動或複製，見下）
//       3) 解構舊記憶體中的元素
//       4) 釋放舊記憶體
//       5) 更新內部三根指標
//     整個過程對使用者透明，代價是所有既有迭代器全部失效。
//   ▸ 第 2 步用移動還是複製？取決於 noexcept
//     vector 必須提供強例外保證：重新配置途中若丟出例外，
//     原容器必須維持不變。移動建構子若可能丟例外，
//     搬到一半失敗就無法還原（來源已被掏空），
//     所以標準庫用 std::move_if_noexcept：
//     只有移動建構子標了 noexcept 才用移動，否則保守地複製。
//     結論：自訂類別的移動建構子務必寫 noexcept，
//     否則 vector 擴容時會退化成複製，效能可能差好幾倍。
//   ▸ reserve 過頭的代價
//     reserve(1000000) 會立刻真的配置那塊記憶體。
//     過度預留在記憶體吃緊時反而有害，而且無法用 shrink_to_fit
//     以外的方式縮回去。合理的做法是用已知的資料量或保守估計值。
//
// 【注意事項 Pay Attention】
//   1. reserve 只改 capacity，不改 size；reserve 後索引存取仍是越界。
//   2. reserve 若造成容量改變，所有 iterator / pointer / reference 失效。
//   3. reserve 無法縮小容量；new_cap <= capacity() 時它什麼都不做。
//   4. 成長倍率是實作定義（本機 libstdc++ 實測為 2 倍，MSVC 為 1.5 倍）。
//   5. 「均攤 O(1)」不等於「每次都 O(1)」——單次 push_back 仍可能 O(n)。
//   6. 自訂類別的移動建構子要標 noexcept，否則擴容時會退化為複製。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector 的擴容與 reserve
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. vector 的 push_back 為什麼是「均攤 O(1)」？請說明推導。
//     答：因為容量按固定倍率成長。以 2 倍為例，插入 n 個元素的
//         總搬移次數是 1+2+4+…+n ≈ 2n（等比級數和小於首項兩倍），
//         平均到每次 push_back 就是常數。
//         若改成每次固定加 k 格，總搬移量會是 O(n²)。
//     追問：「均攤 O(1)」和「最壞 O(1)」差在哪？
//         → 均攤是平均意義；某一次剛好觸發重新配置的 push_back
//           仍然是 O(n)。對即時系統而言這個長尾延遲不可接受，
//           所以要事先 reserve 把它消掉。
//
// 🔥 Q2. vector 擴容搬移元素時，什麼情況下會用複製而不是移動？
//     答：當元素型別的移動建構子沒有標 noexcept 時。
//         vector 必須提供強例外保證——搬到一半若丟例外，
//         原容器要能維持不變。移動會掏空來源，失敗就無法還原，
//         所以標準庫用 std::move_if_noexcept 保守地退回複製。
//     追問：這對效能影響有多大？
//         → 對持有堆積資源的型別（string、vector、unique_ptr 成員）
//           差距是「搬一根指標」對上「深拷貝整塊資料」，
//           可能差好幾個數量級。所以自訂移動建構子一定要寫 noexcept。
//
// ⚠️ 陷阱. std::vector<int> v; v.reserve(10); v[3] = 42;
//          記憶體明明已經配置好了，為什麼這是 undefined behavior？
//     答：reserve 只改變 capacity，不改變 size——此時 v.size() 仍是 0，
//         一個元素都還沒被建構。operator[] 不做範圍檢查，
//         v[3] 存取的是「已配置但尚未建構物件」的原始記憶體，屬於 UB。
//     為什麼會錯：把 capacity 當成 size。
//         capacity 是「不必重新配置就能容納的上限」，
//         size 是「目前真的有幾個已建構的元素」。
//         要讓元素真的存在必須用 resize()（會建構元素）
//         或 push_back()（逐一建構）。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【日常實務範例】批次匯入：資料筆數已知時先把容量開好
//   情境：從資料庫分頁撈取資料，回應標頭已經告訴你這一頁有幾筆
//         （例如 HTTP 的 X-Total-Count，或 SQL 先做一次 COUNT）。
//   為什麼用本主題：這正是 reserve 最有價值的時機——
//         筆數已知就不該讓 vector 從 1 開始一路倍增。
//         除了省下十幾次配置與搬移，更重要的是
//         「事先取得的指標/迭代器不會在匯入途中失效」。
// -----------------------------------------------------------------------------
struct Record {
    int         id;
    std::string name;
};

std::vector<Record> importRecords(std::size_t expectedRows, bool useReserve) {
    std::vector<Record> rows;
    if (useReserve) {
        rows.reserve(expectedRows);   // 一次到位，後續 push_back 保證不重新配置
    }

    std::size_t reallocs = 0;
    const Record* prev = rows.data();
    for (std::size_t i = 0; i < expectedRows; ++i) {
        rows.push_back({static_cast<int>(i), "user_" + std::to_string(i)});
        if (rows.data() != prev) {    // 位址變了 = 發生過重新配置
            ++reallocs;
            prev = rows.data();
        }
    }
    std::cout << "  " << (useReserve ? "有 reserve" : "無 reserve")
              << "：重新配置 " << reallocs << " 次，最終 capacity = "
              << rows.capacity() << std::endl;
    return rows;
}

int main() {
    std::cout << "=== 一、不使用 reserve：觀察倍率成長 ===" << std::endl;
    {
        std::vector<int> v;
        int realloc_count = 0;
        std::size_t prev_cap = 0;

        for (int i = 0; i < 10000; ++i) {
            v.push_back(i);
            if (v.capacity() != prev_cap) {
                ++realloc_count;
                prev_cap = v.capacity();
            }
        }
        std::cout << "不用 reserve：重新配置 " << realloc_count
                  << " 次，最終容量 " << v.capacity() << std::endl;
    }

    std::cout << "\n=== 二、使用 reserve：一次到位 ===" << std::endl;
    {
        std::vector<int> v;
        v.reserve(10000);  // 預先配置
        int realloc_count = 0;
        std::size_t prev_cap = v.capacity();

        for (int i = 0; i < 10000; ++i) {
            v.push_back(i);
            if (v.capacity() != prev_cap) {
                ++realloc_count;
                prev_cap = v.capacity();
            }
        }
        std::cout << "使用 reserve：重新配置 " << realloc_count
                  << " 次，最終容量 " << v.capacity() << std::endl;
    }

    std::cout << "\n=== 三、容量成長的完整序列（前 40 次 push_back）===" << std::endl;
    {
        std::vector<int> v;
        std::size_t prev = 0;
        std::cout << "capacity: ";
        for (int i = 0; i < 40; ++i) {
            v.push_back(i);
            if (v.capacity() != prev) {
                std::cout << v.capacity() << " ";
                prev = v.capacity();
            }
        }
        std::cout << "\n（本機 libstdc++ 為 2 倍成長，屬實作定義；MSVC 為 1.5 倍）"
                  << std::endl;
    }

    std::cout << "\n=== 四、reserve 只改 capacity，不改 size ===" << std::endl;
    {
        std::vector<int> v;
        v.reserve(10);
        std::cout << "reserve(10) 後 size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;
        std::cout << "  → 此時 v[3] 是越界存取（UB），因為一個元素都還沒建構" << std::endl;

        std::vector<int> w;
        w.resize(10);
        std::cout << "resize(10)  後 size=" << w.size()
                  << ", capacity=" << w.capacity() << std::endl;
        std::cout << "  → 此時 w[3] 合法，值為 " << w[3]
                  << "（int 被 value-initialize 為 0）" << std::endl;
    }

    std::cout << "\n=== 五、日常實務：已知筆數的批次匯入 ===" << std::endl;
    importRecords(5000, false);
    importRecords(5000, true);

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 17 課：vector 的記憶體重新配置機制9.cpp" -o reserve_demo
//
// 【關於下方預期輸出的但書】
//   所有 capacity 數值與「重新配置次數」都是「實作定義」的觀察結果，
//   本機環境為 x86-64 / Ubuntu / GCC 15.2 / libstdc++，成長倍率為 2。
//   MSVC 採 1.5 倍成長，同一份程式碼跑出來的次數與容量都會不同。
//   標準只保證「均攤 O(1)」，從未規定倍率或任何具體容量值。
//
// 【本檔未附 LeetCode 範例的理由】
//   reserve 解決的是「記憶體配置次數」這個效能與穩定性問題，
//   屬於工程層面而非演算法層面。LeetCode 以演算法複雜度計分，
//   加不加 reserve 都不影響能否通過，硬套一題無法呈現本主題的價值。
//   真正需要 reserve 的是上面那種「筆數已知的批次匯入」，
//   以及對延遲敏感、不能容忍偶發長尾延遲的即時系統。

// === 預期輸出 ===
// === 一、不使用 reserve：觀察倍率成長 ===
// 不用 reserve：重新配置 15 次，最終容量 16384
//
// === 二、使用 reserve：一次到位 ===
// 使用 reserve：重新配置 0 次，最終容量 10000
//
// === 三、容量成長的完整序列（前 40 次 push_back）===
// capacity: 1 2 4 8 16 32 64
// （本機 libstdc++ 為 2 倍成長，屬實作定義；MSVC 為 1.5 倍）
//
// === 四、reserve 只改 capacity，不改 size ===
// reserve(10) 後 size=0, capacity=10
//   → 此時 v[3] 是越界存取（UB），因為一個元素都還沒建構
// resize(10)  後 size=10, capacity=10
//   → 此時 w[3] 合法，值為 0（int 被 value-initialize 為 0）
//
// === 五、日常實務：已知筆數的批次匯入 ===
//   無 reserve：重新配置 14 次，最終 capacity = 8192
//   有 reserve：重新配置 0 次，最終 capacity = 5000
