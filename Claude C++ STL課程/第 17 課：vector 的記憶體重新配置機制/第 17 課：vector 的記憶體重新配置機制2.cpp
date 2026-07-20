// =============================================================================
//  第 17 課：vector 的記憶體重新配置機制 2  —  成長倍率與均攤 O(1) 的數學
// =============================================================================
//
// 【主題資訊 Information】
//   void push_back(const T& value);   // C++98
//   void push_back(T&& value);        // C++11
//
//   標頭檔  ：<vector>
//   複雜度  ：均攤（amortized）O(1)；單次最壞 O(n)（該次剛好要重新配置）
//   標準依據：C++ 標準只規定「均攤常數時間」，
//             【從未規定成長倍率是多少】——倍率完全是實作定義。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼一定要「等比」成長，不能「等差」成長】
//   假設改成每次滿了就 +k 格（等差）：
//     容量走 k, 2k, 3k, ... 要放到 n 個元素得重新配置 n/k 次，
//     第 i 次搬 i*k 個元素，總搬移量 = k(1+2+...+n/k) ≈ n^2/(2k) = O(n^2)。
//   不論 k 開多大，都只是把常數變小，階數仍是平方——push_back 的
//   均攤 O(1) 承諾直接破產。
//   改成等比（每次乘以 g > 1）：
//     容量走 1, g, g^2, ... 要放到 n 個元素只需 log_g(n) 次重新配置，
//     總搬移量 = 1 + g + g^2 + ... + n ≈ n * g/(g-1) = O(n)。
//   n 次 push_back 總成本 O(n) ⇒ 每次均攤 O(1)。這就是「等比」的全部理由。
//
// 【2. 均攤分析：用「記帳法」直觀理解】
//   把 push_back 的定價訂成「3 個代幣」（g = 2 的情形）：
//     1 個代幣付這次「把新元素放進去」的成本，
//     2 個代幣存起來貼在這個新元素身上。
//   當容量從 m 漲到 2m 時，要搬 m 個舊元素。這 m 個元素中，
//   有 m/2 個是「上次擴容之後才加進來的」，每個身上都存了 2 個代幣，
//   剛好 m/2 * 2 = m 個代幣，足以支付這次全部的搬移。
//   → 帳戶永遠不會透支 ⇒ 每次 push_back 的均攤成本是常數（3 個代幣）。
//
// 【3. 為什麼有些實作選 1.5 而不是 2】
//   兩者都滿足均攤 O(1)，差別在「記憶體行為」與「浪費率」：
//     * 倍率 2（libstdc++ / libc++）：重新配置次數最少、最省 CPU，
//       但最壞情況會浪費將近 50% 的容量（剛擴容完 size 只有 capacity 的一半）。
//     * 倍率 1.5（MSVC）：浪費較少、擴容較平滑，代價是重新配置較頻繁。
//   另有一個常被提到的理論性質：
//       倍率 2 時，新容量 2m 恆「大於」先前所有已釋放區塊的總和
//       (1 + 2 + 4 + ... + m = 2m - 1 < 2m)，
//       所以就算 allocator 把先前釋放的空間全部合併，也永遠塞不下新區塊，
//       配置器只能一路往前找新位置。
//       倍率小於黃金比例 φ ≈ 1.618 時，
//       先前釋放區塊的總和終究會追上所需大小，理論上舊空間有機會被重複利用。
//   注意這是「配置器行為的理論論證」，實際會不會發生取決於 allocator 的
//   實作與當下的 heap 狀態，不是標準保證，也不該當成選型的唯一依據。
//
// 【4. 從 0 開始的第一次成長】
//   等比成長遇到 0 會卡住（0 * 2 還是 0），所以每個實作都得特判第一次。
//   本機 libstdc++ 實測是 0 → 1 → 2 → 4 → 8 ...；
//   有些實作第一次直接給比較大的值（例如一次給 8）以減少小 vector 的
//   配置次數。這同樣是【實作定義】。
//
// 【概念補充 Concept Deep Dive】
//   libstdc++ 的 _M_check_len 大致是：
//       新長度 = 舊容量 + max(舊容量, 需要新增的個數)
//   舊容量非 0 時就是「舊容量 * 2」，這解釋了本機看到的 2 倍數列。
//   但它同時說明了一個常被忽略的事實：
//       一次 insert 很多個元素時，新容量不是「舊容量 * 2」，
//       而是「至少能裝下所需元素」的大小。
//   例如對 capacity 4、size 4 的 vector 一次 insert 100 個元素，
//   新容量會直接跳到能容納 104 個的大小，而不是 8。
//   另外 max_size()（本機 vector<int> 實測為 2305843009213693951）是
//   理論上限，超過會丟 std::length_error，實務上會先撞到實體記憶體。
//
// 【注意事項 Pay Attention】
//   1. 【實作定義】成長倍率、初始容量、以及下方輸出中的每一個 capacity
//      數字，都來自本機 GCC 15.2 / libstdc++。MSVC 會看到 1.5 倍的數列，
//      不要把這些數字寫進跨平台的單元測試。
//   2. 「倍率」只是觀察值，程式邏輯絕不可依賴它。要保證容量請用 reserve()。
//   3. 均攤 O(1) 是「n 次操作的平均」，不是「每次都很快」。
//      對延遲敏感的即時系統（遊戲主迴圈、音訊回呼），那一次 O(n) 的
//      搬移可能就是掉幀來源——這種場合要事先 reserve 或改用固定容量容器。
//   4. 本檔用了 double 除法算倍率，顯示出來的「2」是浮點格式化的結果，
//      不代表倍率一定是精確整數。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】成長倍率與均攤複雜度
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 push_back 可以宣稱均攤 O(1)？請證明。
//     答：因為容量是「等比」成長。以倍率 2 為例，插入 n 個元素只會發生
//         約 log2(n) 次重新配置，總搬移量為 1+2+4+...+n < 2n = O(n)，
//         分攤到 n 次操作就是每次 O(1)。
//         若改成等差（每次 +k），總搬移量是 O(n^2)，均攤就退化成 O(n)。
//     追問：那單次 push_back 的最壞複雜度是多少？
//         → O(n)，就是剛好觸發重新配置那一次。均攤 ≠ 每次都快。
//
// 🔥 Q2. vector 的成長倍率是 2 嗎？
//     答：標準沒有規定，這是【實作定義】。libstdc++ 與 libc++ 實測是 2 倍，
//         MSVC 是 1.5 倍。標準只要求「均攤常數時間」，任何 > 1 的倍率都合格。
//     追問：為什麼 MSVC 選 1.5？→ 記憶體浪費較少、擴容較平滑；且倍率小於
//         黃金比例 φ≈1.618 時，先前釋放的區塊總和有機會足夠容納新區塊，
//         理論上讓 allocator 有重複利用的餘地（倍率 2 則永遠追不上）。
//
// ⚠️ 陷阱. 「我插入 1000 個元素，所以最後 capacity 會是 1000」——錯在哪？
//     答：capacity 是實作依成長策略決定的，本機實測會是 1024（2 的冪次），
//         多出 24 格。只有 reserve(1000) 或 shrink_to_fit() 才會讓
//         capacity 貼近 1000。
//     為什麼會錯：把 capacity 想成「我用了多少」。capacity 是「配置了多少」，
//         那是 size 的事；兩者相等純屬巧合。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <iomanip>

// -----------------------------------------------------------------------------
// 【日常實務範例】依「預估筆數」決定要不要 reserve
//   情境：批次匯入系統讀進一個檔案，檔頭寫著「本批約 N 筆」。
//         這個估計值可能不準，所以我們要決定：
//           - 估計可信（誤差在容忍範圍）→ reserve，一次到位
//           - 估計離譜地大               → 不要盲目 reserve，避免一次吃掉大量記憶體
//   關聯：直接對應本檔的成長策略——不 reserve 就得付 log(n) 次重新配置。
// -----------------------------------------------------------------------------
size_t plan_capacity(size_t estimated_rows, size_t hard_cap) {
    if (estimated_rows == 0) return 0;                 // 沒有估計值就交給成長策略
    if (estimated_rows > hard_cap) return hard_cap;    // 估計離譜，只先要一個安全上限
    return estimated_rows;                             // 估計可信，一次到位
}

int main() {
    std::cout << "=== 一、容量成長序列（本機 libstdc++，實作定義）===" << std::endl;

    std::vector<int> v;
    size_t prev_cap = 0;
    long   total_moved = 0;   // 累計被搬移的元素個數

    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
        if (v.capacity() != prev_cap) {
            if (prev_cap > 0) {
                double ratio = static_cast<double>(v.capacity()) / prev_cap;
                std::cout << "  " << prev_cap << " -> " << v.capacity()
                          << " (倍率: " << ratio << ")" << std::endl;
                total_moved += static_cast<long>(prev_cap);  // 這次搬了 prev_cap 個
            }
            prev_cap = v.capacity();
        }
    }

    std::cout << "\n=== 二、均攤成本的實證 ===" << std::endl;
    std::cout << "push_back 次數 n = " << v.size() << std::endl;
    std::cout << "總搬移元素數     = " << total_moved << std::endl;
    std::cout << "每次均攤搬移     = "
              << std::fixed << std::setprecision(3)
              << static_cast<double>(total_moved) / static_cast<double>(v.size())
              << " 個 (< 常數 ⇒ 均攤 O(1))" << std::endl;
    std::cout << std::defaultfloat;

    std::cout << "\n=== 三、等比 vs 等差：總搬移量的階數差異（純數學模擬）===" << std::endl;
    // 註：欄寬用 setw 對純 ASCII 的數字欄，標題另外用固定字串印，
    //     避免中文字在 setw 下按 byte 計寬而錯位。
    std::cout << "  n          geometric(g=2)   arithmetic(+16)" << std::endl;
    std::cout << "             等比總搬移量     等差總搬移量" << std::endl;
    for (long n : {1000L, 10000L, 100000L}) {
        // 等比：1 + 2 + 4 + ... < 2n
        long geo = 0;
        for (long c = 1; c < n; c *= 2) geo += c;
        // 等差：每次 +16，第 i 次搬 16*i 個
        long ari = 0;
        for (long c = 16; c < n; c += 16) ari += c;
        std::cout << "  " << std::left << std::setw(11) << n
                  << std::setw(17) << geo
                  << std::setw(17) << ari << std::endl;
    }
    std::cout << "→ n 放大 10 倍時：等比的搬移量約放大 10 倍（線性 O(n)），"
              << std::endl;
    std::cout << "  等差的搬移量約放大 100 倍（平方 O(n^2)）。" << std::endl;

    std::cout << "\n=== 四、一次插入多個元素時，成長不是單純的 2 倍 ===" << std::endl;
    std::vector<int> w(4);                 // size=4
    w.reserve(4);                          // capacity 至少 4
    std::cout << "插入前： size=" << w.size() << ", capacity=" << w.capacity() << std::endl;
    std::vector<int> batch(100, 7);
    w.insert(w.end(), batch.begin(), batch.end());   // 一次插入 100 個
    std::cout << "一次插入 100 個後： size=" << w.size()
              << ", capacity=" << w.capacity()
              << "（不是 8，而是至少能裝下所需元素）" << std::endl;

    std::cout << "\n=== 五、日常實務：依預估筆數規劃容量 ===" << std::endl;
    const size_t HARD_CAP = 1000000;   // 一次最多先要 100 萬格，避免估計失控
    struct Case { const char* name; size_t est; };
    for (const Case& c : { Case{"檔頭宣告 5000 筆（可信）",       5000UL},
                           Case{"檔頭宣告 20 億筆（顯然有問題）", 2000000000UL},
                           Case{"檔頭沒寫筆數",                   0UL} }) {
        size_t plan = plan_capacity(c.est, HARD_CAP);
        std::vector<std::string> rows;
        if (plan) rows.reserve(plan);
        std::cout << std::left << std::setw(30) << c.name
                  << " → reserve(" << plan << ")"
                  << "，實得 capacity=" << rows.capacity() << std::endl;
    }
    std::cout << "注意：reserve(n) 之後 capacity 恰好是 n（不會再往上湊 2 的冪次），"
              << std::endl;
    std::cout << "      這是 reserve 與「自然成長」最明顯的差別。" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 17 課：vector 的記憶體重新配置機制2.cpp" -o growth_ratio

// 註：下方所有 capacity 數字均為本機 GCC 15.2 / libstdc++ 的【實作定義】結果
//     （2 倍成長）；在 MSVC 上會看到 1.5 倍的數列，數值不同屬正常。

// === 預期輸出 ===
// === 一、容量成長序列（本機 libstdc++，實作定義）===
//   1 -> 2 (倍率: 2)
//   2 -> 4 (倍率: 2)
//   4 -> 8 (倍率: 2)
//   8 -> 16 (倍率: 2)
//   16 -> 32 (倍率: 2)
//   32 -> 64 (倍率: 2)
//   64 -> 128 (倍率: 2)
//
// === 二、均攤成本的實證 ===
// push_back 次數 n = 100
// 總搬移元素數     = 127
// 每次均攤搬移     = 1.270 個 (< 常數 ⇒ 均攤 O(1))
//
// === 三、等比 vs 等差：總搬移量的階數差異（純數學模擬）===
//   n          geometric(g=2)   arithmetic(+16)
//              等比總搬移量     等差總搬移量
//   1000       1023             31248
//   10000      16383            3120000
//   100000     131071           312450000
// → n 放大 10 倍時：等比的搬移量約放大 10 倍（線性 O(n)），
//   等差的搬移量約放大 100 倍（平方 O(n^2)）。
//
// === 四、一次插入多個元素時，成長不是單純的 2 倍 ===
// 插入前： size=4, capacity=4
// 一次插入 100 個後： size=104, capacity=104（不是 8，而是至少能裝下所需元素）
//
// === 五、日常實務：依預估筆數規劃容量 ===
// 檔頭宣告 5000 筆（可信） → reserve(5000)，實得 capacity=5000
// 檔頭宣告 20 億筆（顯然有問題） → reserve(1000000)，實得 capacity=1000000
// 檔頭沒寫筆數             → reserve(0)，實得 capacity=0
// 注意：reserve(n) 之後 capacity 恰好是 n（不會再往上湊 2 的冪次），
//       這是 reserve 與「自然成長」最明顯的差別。
