// =============================================================================
//  第三課：STL 的六大組件概覽 2  —  遍歷容器的三種寫法（迭代器 / auto / 範圍 for）
// =============================================================================
//
// 【主題資訊 Information】
//   語法：
//     for (std::vector<int>::iterator it = v.begin(); it != v.end(); ++it)  // 完整型別
//     for (auto it = v.begin(); it != v.end(); ++it)                        // C++11 auto
//     for (int n : v)                                                       // C++11 範圍 for
//   標準版本：auto 型別推導、範圍 for 皆為 C++11；
//             v.cbegin()/cend() 也是 C++11；C++17 起範圍 for 允許 begin/end 型別不同。
//   複雜度：三種寫法完全相同 —— 走訪 N 個元素 O(N)，每步 O(1)。
//   標頭檔：<vector>（容器自帶 begin/end）；<iterator> 提供 std::begin/std::end。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 STL 要用「迭代器」而不是索引】
//   用 for (int i = 0; i < v.size(); ++i) v[i] 只對「連續記憶體 + 有 operator[]」的容器
//   成立。std::list 是鏈結串列，沒有 operator[]；std::map 是紅黑樹，索引更無意義。
//   迭代器把「怎麼走到下一個元素」這件事封裝進型別本身：
//     - vector::iterator 的 ++ 就是指標 +1
//     - list::iterator   的 ++ 是 node = node->next
//     - map::iterator    的 ++ 是中序走訪紅黑樹的後繼節點
//   對呼叫端而言介面完全一樣（*it、++it、!=），這就是 STL 能用「同一個 std::find」
//   處理所有容器的關鍵。
//
// 【2. 半開區間 [begin, end) 的設計】
//   end() 指向「最後一個元素的下一個位置」，不是最後一個元素。這樣設計的好處：
//     - 空容器天然表示為 begin() == end()，不必特判
//     - 迴圈條件一律寫 it != end()，不需要 <=（list 迭代器也沒有 <）
//     - 元素個數 = end() - begin()（隨機存取迭代器）
//   代價：*end() 是未定義行為，絕不能解參考。
//
// 【3. 為什麼推薦 ++it 而不是 it++】
//   後置 ++ 依語意必須「先保存舊值、前進、回傳舊值」，所以要多複製一份迭代器：
//     iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }
//   對 vector::iterator（本質是指標）編譯器在 -O2 幾乎一定會把沒用到的 tmp 最佳化掉，
//   兩者產生相同機器碼；但對「肥迭代器」（例如 deque::iterator 內含 4 個指標、
//   或某些偵錯模式的 checked iterator）複製成本是真的。
//   養成寫 ++it 的習慣：零成本、且在泛型程式碼中永遠不會比較差。
//
// 【4. 範圍 for 只是語法糖】
//   for (int n : v) { ... } 被編譯器展開成大致如下（C++17 起的規則）：
//     {
//         auto&& __range = v;                 // 綁定到 v，延長暫時物件壽命
//         auto __begin = __range.begin();
//         auto __end   = __range.end();       // C++17 起 __end 型別可與 __begin 不同
//         for (; __begin != __end; ++__begin) {
//             int n = *__begin;               // 注意：這裡是「複製」
//             ...
//         }
//     }
//   關鍵在最後那行：宣告成 int n 會複製元素。對 int 無所謂，
//   對 std::string / 大型結構就是每輪一次深複製。要點：
//     for (const auto& x : v)  唯讀，不複製 ← 預設就該這樣寫
//     for (auto& x : v)        要修改元素
//     for (auto x : v)         真的想要一份副本時才用
//
// 【概念補充 Concept Deep Dive】
//   vector<int>::iterator 在 libstdc++ 中是 __gnu_cxx::__normal_iterator<int*, vector<int>>
//   —— 一個只包住 int* 的薄包裝（sizeof 等於一個指標，本機實測 8 bytes）。
//   包一層而不直接 typedef int* 的理由有二：
//     1) 型別安全：讓 vector<int>::iterator 與 int* 是不同型別，避免使用者把裸指標
//        誤傳進只接受該容器迭代器的介面，也讓多載解析能區分。
//     2) 可換實作：debug 模式（_GLIBCXX_DEBUG）可換成會做邊界檢查的 iterator，
//        而使用者程式碼一行都不用改。
//   因為只包一個指標且成員函式全是 inline，-O2 下三種寫法會編出幾乎相同的機器碼；
//   「用迭代器比較慢」是誤解。
//
// 【注意事項 Pay Attention】
//   1. 迴圈中修改容器（push_back / erase）可能使迭代器失效，範圍 for 也一樣會中招，
//      因為它內部就是持有 __begin/__end。要邊走邊刪請用 it = v.erase(it) 的寫法。
//   2. *v.end() 是未定義行為；不要因為「印出來好像是某個數字」就以為它可讀。
//   3. for (int n : v) 對非平凡型別會複製，請預設寫 const auto&。
//   4. 對 const 容器，v.begin() 會回傳 const_iterator；若只是要唯讀語意，
//      明確寫 cbegin()/cend() 更能表達意圖，也避免不小心呼叫到寫入版多載。
//   5. auto it = v.begin() 推導出的是 iterator 而非 const_iterator —— auto 只做型別推導，
//      不會幫你加上 const。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】容器遍歷與迭代器基礎
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 範圍 for（range-based for）背後做了什麼？它要求容器提供什麼？
//     答：編譯器展開成「取 begin()/end() → 迴圈 ++ → 解參考」的普通迭代器迴圈。
//         要求是能取得 begin/end：成員函式 v.begin() 或 ADL 找得到的自由函式
//         begin(v) 二擇一即可（C 陣列由編譯器特別處理）。
//         所以任何自訂類別只要提供 begin()/end() 就能用範圍 for。
//     追問：迭代器最少要實作哪些運算子？→ operator*、operator++（前置）、
//           以及能判斷結束的 operator!=（C++17 起 end 可以是不同型別的 sentinel）。
//
// 🔥 Q2. ++it 和 it++ 有什麼差別？實務上該用哪個？
//     答：後置版必須回傳「遞增前」的副本，因此語意上多一次迭代器複製。
//         對 vector::iterator（薄包裝指標）最佳化後通常沒有差別，
//         但對複雜迭代器（如 deque::iterator）複製成本是真的。
//         習慣寫 ++it：不會更差，在泛型程式碼中可能更好。
//     追問：為什麼後置 ++ 的宣告要多一個沒名字的 int 參數？
//           → 那是純粹用來區分前置/後置多載的 dummy 參數，呼叫時不會傳值。
//
// ⚠️ 陷阱. for (auto x : vec) 和 for (auto& x : vec) 只差一個 & ，會有什麼實質差別？
//     答：auto x 每輪都「複製」一份元素；對 vector<std::string> 就是每輪一次
//         字串複製（可能含堆積配置）。而且對 x 的修改只改到副本，
//         原容器完全不受影響 —— 這常被誤以為是「修改沒生效」的 bug。
//     為什麼會錯：多數人把範圍 for 讀成「x 就是容器裡的那個元素」，
//         但展開後那行其實是 int n = *__begin; ——一次貨真價實的初始化複製。
//         唯讀請寫 const auto&，要改請寫 auto&。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1480. Running Sum of 1d Array
//   題目：給定陣列 nums，回傳 runningSum，其中 runningSum[i] = nums[0]+...+nums[i]。
//   為什麼用到本主題：這是「一次線性走訪 + 累加」最純粹的形式。用範圍 for 搭配
//                     const auto& 讀取來源、用 push_back 寫入結果，正好示範
//                     「唯讀遍歷」與「邊走邊產生輸出」兩種基本模式。
//   複雜度：時間 O(N)、額外空間 O(N)（不算輸出則為 O(1)）。
// -----------------------------------------------------------------------------
std::vector<int> runningSum(const std::vector<int>& nums) {
    std::vector<int> result;
    result.reserve(nums.size());  // 先配足容量，避免走訪中反覆重新配置
    int sum = 0;
    for (const int& n : nums) {   // 唯讀遍歷：用 const 參考不複製
        sum += n;
        result.push_back(sum);
    }
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】計算 API 回應時間的平均值與超標筆數（SLA 監控）
//   情境：監控系統每分鐘收到一批 API 回應時間（毫秒），要算出平均值，
//         並統計超過 SLA 門檻（200ms）的筆數，用來決定是否發警報。
//   為什麼用到本主題：這是最典型的「單次線性走訪同時累計多個統計量」，
//                     一輪範圍 for 就能同時完成，不需要走訪兩次。
// -----------------------------------------------------------------------------
struct LatencyReport {
    double average_ms = 0.0;
    int    over_sla   = 0;
};

LatencyReport analyzeLatency(const std::vector<int>& samples_ms, int sla_ms) {
    LatencyReport report;
    if (samples_ms.empty()) return report;   // 防止除以零

    long long total = 0;
    for (const int& ms : samples_ms) {
        total += ms;
        if (ms > sla_ms) ++report.over_sla;
    }
    report.average_ms = static_cast<double>(total) / static_cast<double>(samples_ms.size());
    return report;
}

int main() {
    std::vector<int> vec = {10, 20, 30, 40, 50};

    std::cout << "=== 三種遍歷寫法 ===" << std::endl;

    // 方法一：使用迭代器遍歷
    std::cout << "使用迭代器: ";
    for (std::vector<int>::iterator it = vec.begin(); it != vec.end(); ++it) {
        std::cout << *it << " ";  // *it 取得迭代器指向的值
    }
    std::cout << std::endl;

    // 方法二：使用 auto 簡化
    std::cout << "使用 auto: ";
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // 方法三：範圍 for（底層也是迭代器）
    std::cout << "範圍 for: ";
    for (int n : vec) {
        std::cout << n << " ";
    }
    std::cout << std::endl;

    // 補充：唯讀請用 const 參考，要修改請用參考
    std::cout << "\n=== 值 vs 參考 ===" << std::endl;
    std::vector<std::string> names = {"alice", "bob"};

    for (auto n : names) n += "_MODIFIED";        // 改的是副本，原容器不變
    std::cout << "for (auto n : names) 之後: ";
    for (const auto& n : names) std::cout << n << " ";
    std::cout << std::endl;

    for (auto& n : names) n += "_MODIFIED";       // 改的是元素本身
    std::cout << "for (auto& n : names) 之後: ";
    for (const auto& n : names) std::cout << n << " ";
    std::cout << std::endl;

    // 迭代器本身有多大（本機 g++ 15.2 / libstdc++ 實測；其他實作可能不同）
    std::cout << "\n=== 迭代器大小（實作定義）===" << std::endl;
    std::cout << "sizeof(int*)                    = " << sizeof(int*) << " bytes" << std::endl;
    std::cout << "sizeof(std::vector<int>::iterator) = "
              << sizeof(std::vector<int>::iterator) << " bytes" << std::endl;

    std::cout << "\n=== LeetCode 1480. Running Sum of 1d Array ===" << std::endl;
    std::vector<int> nums = {1, 2, 3, 4};
    std::cout << "nums       = 1 2 3 4" << std::endl;
    std::cout << "runningSum = ";
    for (int n : runningSum(nums)) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "\n=== 日常實務：API 延遲 SLA 監控 ===" << std::endl;
    std::vector<int> latency = {120, 95, 240, 180, 310, 150};
    LatencyReport rep = analyzeLatency(latency, 200);
    std::cout << "樣本數: " << latency.size() << std::endl;
    std::cout << "平均延遲: " << rep.average_ms << " ms" << std::endl;
    std::cout << "超過 200ms 的筆數: " << rep.over_sla << std::endl;

    return 0;
}

// 注意：sizeof(std::vector<int>::iterator) 是「實作定義」的值。
//       以下 8 bytes 為本機 g++ 15.2 / libstdc++ / x86-64 實測結果，
//       其他編譯器、其他平台或開啟 _GLIBCXX_DEBUG 時都可能不同。

// 編譯: g++ -std=c++17 -Wall -Wextra 第三課：STL 的六大組件概覽2.cpp -o demo2

// === 預期輸出 ===
// === 三種遍歷寫法 ===
// 使用迭代器: 10 20 30 40 50
// 使用 auto: 10 20 30 40 50
// 範圍 for: 10 20 30 40 50
//
// === 值 vs 參考 ===
// for (auto n : names) 之後: alice bob
// for (auto& n : names) 之後: alice_MODIFIED bob_MODIFIED
//
// === 迭代器大小（實作定義）===
// sizeof(int*)                    = 8 bytes
// sizeof(std::vector<int>::iterator) = 8 bytes
//
// === LeetCode 1480. Running Sum of 1d Array ===
// nums       = 1 2 3 4
// runningSum = 1 3 6 10
//
// === 日常實務：API 延遲 SLA 監控 ===
// 樣本數: 6
// 平均延遲: 182.5 ms
// 超過 200ms 的筆數: 2
