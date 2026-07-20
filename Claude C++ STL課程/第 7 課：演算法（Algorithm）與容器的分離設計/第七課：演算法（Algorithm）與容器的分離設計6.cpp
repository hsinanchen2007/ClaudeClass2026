// =============================================================================
//  第七課：演算法（Algorithm）與容器的分離設計6.cpp
//    —  複製家族：copy / copy_if / copy_n / copy_backward 與 insert iterator
// =============================================================================
//
// 【主題資訊 Information】
//   OutIt  copy         (InputIt f, InputIt l, OutIt d_first);            // C++98
//   OutIt  copy_if      (InputIt f, InputIt l, OutIt d_first, UnaryPred); // C++11 ★
//   OutIt  copy_n       (InputIt f, Size n,    OutIt d_first);            // C++11 ★
//   BidIt2 copy_backward(BidIt1 f, BidIt1 l,   BidIt2 d_last);            // C++98
//
//   標準版本：copy / copy_backward 為 C++98；**copy_if 與 copy_n 是 C++11 新增**
//   迭代器需求：copy/copy_if/copy_n 要 Input + Output；
//               copy_backward 兩邊都要 Bidirectional
//   複雜度：全部 O(N)
//   回傳：copy 系列回傳「目的地寫入結束後的位置」；copy_backward 回傳
//         「目的地寫入起始位置」（因為它是往回寫）
//   標頭檔：<algorithm>；back_inserter 等在 <iterator>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼目的地只給起點：分離設計的代價】
// 所有 copy 系列都只接受 d_first，沒有 d_last。演算法**不會**檢查目的地夠不夠大。
// 原因回到本課主軸：演算法只拿到 iterator，它不知道那個 iterator 背後是
// vector、array 還是裸指標，也無從查詢剩餘容量。寫超過就是緩衝區溢位。
// 所以呼叫端有兩個責任二選一：
//   (a) 事先確保空間：std::vector<int> dest(src.size());   ← 本檔 dest1 的做法
//   (b) 改用 insert iterator：std::back_inserter(dest)      ← dest2/dest3 的做法
// **記住：copy 不會幫容器成長，back_inserter 才會。**
//
// 【2. back_inserter 是怎麼把「寫入」變成「push_back」的】
// std::back_insert_iterator 是個 Output Iterator 的轉接器（adapter）。
// 它的核心只有幾行：
//     back_insert_iterator& operator=(const T& value) {
//         container->push_back(value);     // 賦值 → 轉成 push_back
//         return *this;
//     }
//     back_insert_iterator& operator*()  { return *this; }   // 解參考回自己
//     back_insert_iterator& operator++() { return *this; }   // ++ 什麼都不做
// 於是演算法內部的 `*d_first++ = value;` 就被翻譯成 `container.push_back(value)`。
// 這是一個漂亮的示範：**iterator 是協定，不是資料結構的指標**——
// 只要遵守協定，寫入動作可以是任何事情（push_back、insert、印到 cout）。
// 同族還有 front_inserter（轉成 push_front，適用 list/deque，vector 不支援）
// 與 inserter(c, pos)（轉成 insert，適用 set/map）。
//
// 【3. copy_backward 存在的唯一理由：重疊區間】
// 為什麼需要「從後往前複製」？因為當來源與目的地**重疊**時，方向決定正確性。
// 想把 [1,2,3,4,5] 往右移兩格（就地）：
//     copy(v.begin(), v.begin()+3, v.begin()+2);    // ✗ 先寫的值會蓋掉還沒讀的
//     copy_backward(v.begin(), v.begin()+3, v.begin()+5);  // ✓ 從尾巴往前寫
// 規則很簡單：
//   * 目的地在來源**前面**（往左搬）→ 用 copy
//   * 目的地在來源**後面**（往右搬）→ 用 copy_backward
// 這與 C 的 memcpy（不允許重疊）vs memmove（允許重疊）是同一個問題。
// 注意 copy_backward 的第三個參數是 **d_last（結束位置）**，不是起點，
// 它從那裡往前寫——這是最常記錯的地方。
//
// 【4. copy_if 為什麼到 C++11 才有】
// C++98 只有 remove_copy_if（複製「不」滿足條件的），要複製「滿足」條件的
// 得寫雙重否定：remove_copy_if(f, l, out, not1(pred))。理由與 find_if_not 相同
// （見本課第 3 個檔案）：not1 配不上 lambda，且雙重否定難讀，因此補上 copy_if。
//
// 【概念補充 Concept Deep Dive】
//
// (A) copy 的回傳值常被忽略，但很有用
//   copy 回傳「目的地最後寫入位置的下一個」，可以直接串接：
//       auto it = std::copy(a.begin(), a.end(), dest.begin());
//       std::copy(b.begin(), b.end(), it);      // 接著往後寫 b
//   這是合併多個來源到同一個緩衝區的標準寫法。
//
// (B) 效能：copy 對 trivially copyable 型別會退化成 memmove
//   libstdc++ 對 int、char 這類 trivially copyable 且 iterator 是連續記憶體的
//   情況，會特化成一次 memmove 呼叫，而非逐元素迴圈。
//   所以 std::copy 對 POD 陣列的效能與手寫 memcpy 相當——
//   **這是實作的最佳化，不是標準要求**，但主流實作都有做。
//   反過來說，用了 back_inserter 就無法走這條快路（每次都要 push_back，
//   可能觸發重新配置），大量資料時應先 reserve。
//
// (C) back_inserter 與 reserve 搭配
//   std::vector<int> dest;
//   dest.reserve(src.size());                       // 先配置，避免多次重新配置
//   std::copy(src.begin(), src.end(), std::back_inserter(dest));
//   reserve 只改 capacity 不改 size，所以仍然要用 back_inserter；
//   若寫成 dest.resize(n) 再 copy(dest.begin())，那是另一種寫法（元素會先預設建構）。
//
// 【注意事項 Pay Attention】
// 1. **copy 不檢查目的地容量**。目的地是空 vector 卻傳 dest.begin() → 未定義行為。
// 2. 需要自動成長時用 std::back_inserter（<iterator>）；
//    vector **沒有** push_front，不能用 front_inserter。
// 3. copy_backward 的第三參數是**目的地的結束位置**，且是往前寫；
//    回傳值是實際寫入的起點。
// 4. 來源與目的地重疊時：往左搬用 copy，往右搬用 copy_backward。搞反會覆蓋資料。
// 5. copy_if / copy_n 是 C++11。
// 6. 用了 back_inserter 就無法享受 memmove 最佳化；大量資料先 reserve。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】複製家族與 insert iterator
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::copy 的目的地為什麼只給起點不給終點？這造成什麼風險、怎麼解決？
//     答：因為演算法只看得到 iterator，看不到容器本體，無從查詢剩餘容量。
//         風險是寫超過即緩衝區溢位（未定義行為），而且可能因為 capacity 剛好夠
//         而「看起來正常」。解法二選一：事先 resize 好目的地，
//         或改用 std::back_inserter 讓寫入轉成 push_back。
//     追問：back_inserter 是怎麼辦到的？→ 它是 Output Iterator 轉接器，
//         把 operator= 重載成呼叫 container->push_back，
//         而 operator* 和 operator++ 都回傳自己什麼也不做，
//         於是演算法裡的 *d++ = v 就變成了 push_back(v)。
//
// 🔥 Q2. 什麼時候必須用 copy_backward 而不能用 copy？
//     答：當來源與目的地**重疊，且目的地在來源後面**（把資料往右搬）時。
//         用 copy 會由前往後寫，先寫入的值會蓋掉還沒讀到的來源資料。
//         copy_backward 從尾端往前寫，避開覆蓋。這與 C 的 memcpy 不可重疊、
//         memmove 可重疊是同一個問題。
//     追問：往左搬呢？→ 往左搬用 copy 就對了，因為讀取位置永遠跑在寫入位置前面。
//
// ⚠️ 陷阱. std::vector<int> dest; std::copy(src.begin(), src.end(), dest.begin());
//        這段程式在你的機器上「跑起來沒事」，可以出貨嗎？
//     答：絕對不行。dest 是空的，dest.begin() == dest.end()，寫入任何元素都是
//         越界寫入，屬未定義行為。「沒事」只代表這次剛好沒踩到會崩潰的記憶體，
//         換個編譯器、換個最佳化等級、換筆資料量都可能變成當機或資料損毀。
//     為什麼會錯：多數人以為 STL 演算法會「自己處理容器成長」，
//         或以為 begin() 存在就代表可以寫。實際上空 vector 的 begin() 是合法
//         但不可寫入的位置，而演算法根本沒有能力呼叫 push_back。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 88. Merge Sorted Array
//   題目：nums1 長度 m+n（後 n 格是預留空間），nums2 長度 n，兩者皆已排序，
//         要把 nums2 就地合併進 nums1 且維持排序。
//   為什麼用到本主題：這題的標準解法正是「**從後往前寫**」——因為 nums1
//         前段還有沒讀到的資料，由前往後寫會覆蓋掉它。這就是 copy_backward
//         存在的理由（重疊區間、目的地在來源後方）的真實應用。
//   複雜度：O(m + n)。
// -----------------------------------------------------------------------------
void merge(std::vector<int>& nums1, int m, std::vector<int>& nums2, int n) {
    int i = m - 1;        // nums1 有效資料的最後一格
    int j = n - 1;        // nums2 的最後一格
    int k = m + n - 1;    // 寫入位置：從最尾端往前
    while (j >= 0) {
        if (i >= 0 && nums1[i] > nums2[j]) {
            nums1[k--] = nums1[i--];
        } else {
            nums1[k--] = nums2[j--];
        }
    }
    // 若 nums2 先耗盡，nums1 剩下的本來就在正確位置，不需搬動
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】從混合 log 中抽出錯誤行送往告警系統
//   情境：應用程式 log 各種等級混在一起，監控模組只要把 ERROR / FATAL 的行
//         挑出來送進告警佇列。事先不知道有幾筆，所以目的地必須能自動成長。
//   為什麼用到本主題：copy_if + back_inserter 是「條件過濾到未知大小容器」的
//         標準組合；先 reserve 可避免多次重新配置。
// -----------------------------------------------------------------------------
std::vector<std::string> extractAlerts(const std::vector<std::string>& logLines) {
    std::vector<std::string> alerts;
    alerts.reserve(logLines.size() / 4);   // 經驗值：錯誤行通常是少數，先粗估
    std::copy_if(logLines.begin(), logLines.end(), std::back_inserter(alerts),
                 [](const std::string& line) {
                     return line.find("[ERROR]") != std::string::npos ||
                            line.find("[FATAL]") != std::string::npos;
                 });
    return alerts;
}

int main() {
    std::vector<int> src = {1, 2, 3, 4, 5};

    // copy：複製到另一個容器, 需要確保目標容器有足夠空間
    std::cout << "=== copy ===" << std::endl;
    std::vector<int> dest1(src.size());   // ★ 必須先給空間，copy 不會幫你成長
    std::copy(src.begin(), src.end(), dest1.begin());
    std::cout << "copy 結果: ";
    for (int n : dest1) std::cout << n << " ";
    std::cout << std::endl;

    // copy_if：條件複製, 只複製滿足條件的元素
    // ★ 目的地是空的 vector，靠 back_inserter 把寫入轉成 push_back
    std::cout << "\n=== copy_if ===" << std::endl;
    std::vector<int> dest2;
    std::copy_if(src.begin(), src.end(), std::back_inserter(dest2),
        [](int n) { return n % 2 == 0; });
    std::cout << "只複製偶數: ";
    for (int n : dest2) std::cout << n << " ";
    std::cout << std::endl;

    // copy_n：複製前 n 個, 需要確保目標容器有足夠空間
    std::cout << "\n=== copy_n ===" << std::endl;
    std::vector<int> dest3;
    std::copy_n(src.begin(), 3, std::back_inserter(dest3));
    std::cout << "複製前 3 個: ";
    for (int n : dest3) std::cout << n << " ";
    std::cout << std::endl;

    // copy_backward：從後往前複製, 需要確保目標容器有足夠空間
    // ★ 第三個參數是「目的地的結束位置」，從那裡往前寫
    std::cout << "\n=== copy_backward ===" << std::endl;
    std::vector<int> dest4(7, 0);  // {0, 0, 0, 0, 0, 0, 0}
    std::copy_backward(src.begin(), src.end(), dest4.end());
    std::cout << "copy_backward: ";
    for (int n : dest4) std::cout << n << " ";
    std::cout << std::endl;

    // ★ copy_backward 的真正用途：來源與目的地重疊、且要往右搬
    std::cout << "\n=== 重疊區間：往右搬只能用 copy_backward ===" << std::endl;
    std::vector<int> wrong = {1, 2, 3, 4, 5};
    std::copy(wrong.begin(), wrong.begin() + 3, wrong.begin() + 2);  // 往右搬 2 格
    std::cout << "用 copy 往右搬 2 格:          ";
    for (int n : wrong) std::cout << n << " ";
    std::cout << "  ← 資料被覆蓋了" << std::endl;

    std::vector<int> right = {1, 2, 3, 4, 5};
    std::copy_backward(right.begin(), right.begin() + 3, right.begin() + 5);
    std::cout << "用 copy_backward 往右搬 2 格: ";
    for (int n : right) std::cout << n << " ";
    std::cout << "  ← 正確" << std::endl;

    // ★ copy 的回傳值可以串接多個來源
    std::cout << "\n=== 用回傳值串接多個來源 ===" << std::endl;
    std::vector<int> a = {1, 2}, b = {3, 4}, c = {5, 6};
    std::vector<int> merged(6);
    auto pos = std::copy(a.begin(), a.end(), merged.begin());
    pos = std::copy(b.begin(), b.end(), pos);
    std::copy(c.begin(), c.end(), pos);
    std::cout << "串接結果: ";
    for (int n : merged) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "\n=== LeetCode 88. Merge Sorted Array ===" << std::endl;
    std::vector<int> nums1 = {1, 2, 3, 0, 0, 0};
    std::vector<int> nums2 = {2, 5, 6};
    merge(nums1, 3, nums2, 3);
    std::cout << "合併結果: ";
    for (int n : nums1) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "\n=== 日常實務：抽出 log 中的告警行 ===" << std::endl;
    std::vector<std::string> logLines = {
        "2026-07-19 09:00:01 [INFO]  service started",
        "2026-07-19 09:00:05 [WARN]  cache miss rate 42%",
        "2026-07-19 09:00:09 [ERROR] db connection refused",
        "2026-07-19 09:00:12 [INFO]  retry scheduled",
        "2026-07-19 09:00:20 [FATAL] out of memory, aborting"
    };
    for (const auto& line : extractAlerts(logLines)) {
        std::cout << "  -> " << line << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第七課：演算法（Algorithm）與容器的分離設計6.cpp -o demo6

// === 預期輸出 ===
// === copy ===
// copy 結果: 1 2 3 4 5
//
// === copy_if ===
// 只複製偶數: 2 4
//
// === copy_n ===
// 複製前 3 個: 1 2 3
//
// === copy_backward ===
// copy_backward: 0 0 1 2 3 4 5
//
// === 重疊區間：往右搬只能用 copy_backward ===
// 用 copy 往右搬 2 格:          1 2 1 2 3   ← 資料被覆蓋了
// 用 copy_backward 往右搬 2 格: 1 2 1 2 3   ← 正確
//
// === 用回傳值串接多個來源 ===
// 串接結果: 1 2 3 4 5 6
//
// === LeetCode 88. Merge Sorted Array ===
// 合併結果: 1 2 2 3 5 6
//
// === 日常實務：抽出 log 中的告警行 ===
//   -> 2026-07-19 09:00:09 [ERROR] db connection refused
//   -> 2026-07-19 09:00:20 [FATAL] out of memory, aborting
