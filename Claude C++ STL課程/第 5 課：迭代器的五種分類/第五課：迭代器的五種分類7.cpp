// =============================================================================
//  第五課：迭代器的五種分類 7  —  Random Access Iterator（隨機存取迭代器）
// =============================================================================
//
// 【主題資訊 Information】
//   類別標籤：std::random_access_iterator_tag
//   代表容器：std::vector、std::deque、std::array、std::string、原生指標
//   支援操作：Bidirectional 的全部，再加上
//       it + n / it - n / it += n / it -= n     （O(1) 跳躍）
//       it1 - it2                                （距離，回傳 difference_type）
//       it[n]                                    （等同 *(it + n)）
//       it1 < it2、<=、>、>=                      （位置比大小）
//   標頭檔：<vector>、<deque>、<array>、<iterator>
//   標準版本：C++98 起即有；C++20 另外細分出 contiguous_iterator。
//   移動 n 步的複雜度：O(1)
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼只有這一類能 O(1) 跳躍】
//   Random Access 的所有新能力都建立在同一個前提上：
//   **從第 i 個元素的位置，可以用「算術」直接算出第 j 個元素的位置。**
//   vector 的元素在記憶體中連續排列，所以
//       第 n 個元素的位址 = 起始位址 + n * sizeof(T)
//   一次乘法加一次加法，與 n 多大無關 —— 這就是 O(1)。
//   鏈結串列辦不到，因為節點位址由 allocator 決定，彼此毫無關係。
//
//   有了這個前提，it1 - it2 也才有意義：
//       距離 = (位址1 - 位址2) / sizeof(T)
//   同樣是 O(1)。在 list 上要算距離只能一步步數，是 O(n)。
//
// 【2. it1 < it2 為什麼只有這一類才有】
//   「比大小」問的是「誰在前面」。對連續記憶體，比位址就是比順序。
//   對鏈結串列，兩個節點的位址大小和它們在串列中的先後**完全無關**
//   （節點是分別 new 出來的，位址順序取決於 allocator，不是插入順序）。
//   所以標準只在 Random Access 這一級提供比較運算子——
//   在其他容器上這個問題根本沒有可以 O(1) 回答的定義。
//   Bidirectional 只能用 == / != 判斷「是不是同一個位置」。
//
// 【3. deque 的迭代器是 Random Access，但它不是連續記憶體】
//   這是很多人搞混的一點。deque 的內部是「一組固定大小的區塊 + 一張索引表」，
//   本機 libstdc++ 實測每塊為 512 bytes（元素大時改為每塊 1 個元素；
//   這是實作定義的數值，不是標準規定）。
//   要取第 n 個元素，deque 的迭代器得先算「在第幾塊、塊內第幾個」，
//   再查索引表拿到那塊的位址 —— 還是 O(1)，只是常數比 vector 大。
//   所以 deque 的迭代器**滿足 Random Access**，卻**不是 contiguous**：
//       &v[0] + n == &v[n]      對 vector 成立
//       &d[0] + n == &d[n]      對 deque 不成立（跨塊就斷了）
//   C++20 新增 contiguous_iterator 正是為了把這個差別講清楚，
//   因為「能不能拿 &*it 當 C 陣列指標傳給 API」取決於連續性，而非隨機存取性。
//
// 【4. 有了 Random Access 才真正解鎖的演算法】
//       std::sort            需要隨機跳躍做 partition
//       std::nth_element     同上
//       std::binary_search / lower_bound / upper_bound
//                            —— 見下面 (B)，這裡有個經典誤解
//       std::random_shuffle / std::shuffle
//       std::make_heap / push_heap / pop_heap
//   這些在 list 上全部不能用（list 只能用自己的成員函式 sort/merge）。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 原生指標就是最原始的 Random Access Iterator
//     int arr[5]; int* p = arr;
//     p + 3、p[2]、p2 - p1、p1 < p2 —— 全部合法。
//     STL 的迭代器設計本來就是「把指標的介面抽象化」，
//     所以原生指標天生滿足 Random Access（也滿足 C++20 的 contiguous_iterator）。
//     這也是為什麼 std::sort(arr, arr + 5) 直接就能用。
//
// (B) 「二分搜尋一定是 O(log n)」是錯的 —— 要看迭代器類別
//     std::lower_bound 的介面只要求 Forward Iterator，所以它**可以**用在 std::list。
//     但它的複雜度規定是：
//         比較次數 O(log n)，迭代器前進次數 O(n)。
//     在 vector 上，每次「跳到中點」是 O(1)，總成本 O(log n)。
//     在 list 上，每次跳中點得一步步走，總成本退化成 O(n)。
//     所以在 list 上做二分搜尋，比從頭線性掃還慢（多了 log n 次無謂比較）。
//     結論：**二分搜尋的 O(log n) 是 Random Access 換來的，不是演算法自帶的。**
//
// (C) difference_type 是有號的，不是 size_t
//     it1 - it2 的回傳型別是 iterator_traits<It>::difference_type，
//     對 vector 而言通常是 ptrdiff_t（有號）。
//     這是刻意的：距離可以是負的（it2 在 it1 後面時）。
//     這也是為什麼 std::distance 回傳有號型別，
//     和 container.size() 回傳無號的 size_type 不同 ——
//     兩者混用比較時會觸發 -Wsign-compare 警告。
//
// (D) end() 可以參與算術，但不可解參考
//     v.end() - 1、v.end() > it 都合法（end 是合法的「尾後位置」）。
//     但 *v.end() 是未定義行為。
//     另外 v.end() + 1 也是 UB —— 合法範圍只到「尾後一個」為止。
//
// 【注意事項 Pay Attention】
//   1. *v.end() 是未定義行為；v.end() + 1 同樣是 UB。
//   2. 空容器上 v.begin() + 1、v.end() - 1 都是 UB，要先檢查 empty()。
//   3. it[n] 不做邊界檢查，越界是 UB（和 vector::operator[] 一樣）。
//      要檢查請用 v.at(i)。
//   4. vector 的 push_back 若觸發重新配置，**所有**迭代器/指標/參考立即失效。
//      deque 的 push_back/push_front 會讓迭代器失效，但**指向元素的參考仍有效**
//      （因為既有區塊不搬動）——這是 deque 少見但重要的性質。
//   5. deque 是 Random Access 但不是 contiguous，不可假設 &d[0] 是一整塊陣列。
//   6. it1 - it2 的型別是有號的 difference_type，和 size() 比較會有號/無號警告。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Random Access Iterator
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼只有 Random Access Iterator 支援 it1 < it2？
//        其他類別為什麼連比大小都不行？
//     答：因為「誰在前面」這個問題，只有在元素位置可以用算術描述時
//         才有 O(1) 的答案。vector 的元素連續，比位址就是比順序。
//         鏈結串列的節點是分別配置的，位址大小和串列中的先後**完全無關**，
//         沒有任何辦法 O(1) 回答。所以標準乾脆不提供，
//         其他類別只有 == / != 用來判斷「是不是同一個位置」。
//     追問：那 deque 呢？它不是連續記憶體，為什麼還能比大小？
//         → deque 是「區塊陣列 + 索引表」，迭代器內部存著
//           「第幾塊、塊內第幾個」，比較這組座標仍是 O(1)。
//           它滿足 Random Access，但不滿足 C++20 的 contiguous_iterator。
//
// 🔥 Q2. std::lower_bound 可以用在 std::list 嗎？複雜度是多少？
//     答：可以用（它的介面只要求 Forward Iterator），但複雜度會退化。
//         標準的規定是「比較次數 O(log n)、迭代器前進次數 O(n)」。
//         在 vector 上跳中點是 O(1)，總成本 O(log n)；
//         在 list 上跳中點要一步步走，總成本 O(n)——
//         比從頭線性掃還慢，因為多做了 log n 次無謂比較。
//     追問：所以二分搜尋的 O(log n) 是誰給的？
//         → Random Access Iterator 給的，不是演算法自帶的。
//           資料結構決定了演算法能跑多快。
//
// ⚠️ 陷阱. deque 的迭代器是 Random Access，所以 &d[0] 可以當成
//         一整塊陣列傳給 C API（例如 memcpy 或 read()）吧？
//     答：不行。Random Access 保證的是「O(1) 跳到第 n 個」，
//         **不保證元素在記憶體中連續**。
//         deque 是一組固定大小區塊（本機 libstdc++ 實測 512 bytes 一塊，
//         實作定義），&d[0] + n 一旦跨過區塊邊界就指向不相干的記憶體。
//         要連續請用 vector 或 array，並用 data() 取指標；
//         C++20 起可以用 std::contiguous_iterator concept 在編譯期擋下這種誤用。
//     為什麼會錯：把「隨機存取」和「連續記憶體」當成同一件事。
//         C++17 以前標準確實沒有明確區分這兩個概念，
//         contiguous_iterator 就是 C++20 為了消除這個混淆才加的。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <deque>
#include <list>
#include <string>
#include <algorithm>
#include <iterator>
#include <type_traits>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 349. Intersection of Two Arrays
//   題目：給兩個陣列，回傳它們的交集（結果中每個元素只出現一次）。
//   為什麼用到本主題：這題的標準解法「先排序、再雙指標（或二分搜尋）」
//     從頭到尾都踩在 Random Access Iterator 上：
//       * std::sort 要求 Random Access（list 連編譯都不過）
//       * std::lower_bound 要在 vector 上才真的是 O(log n)
//       * 雙指標推進、算距離、比較位置，全都靠隨機存取
//     換成 std::list，sort 直接編譯失敗，改用 list::sort 之後
//     lower_bound 又會退化成 O(n) —— 正好示範
//     「資料結構決定演算法能跑多快」。
// -----------------------------------------------------------------------------
std::vector<int> intersection(std::vector<int> a, std::vector<int> b) {
    std::sort(a.begin(), a.end());          // 要求 Random Access
    std::sort(b.begin(), b.end());

    std::vector<int> out;
    auto ia = a.begin(), ib = b.begin();
    while (ia != a.end() && ib != b.end()) {
        if (*ia < *ib)      ++ia;
        else if (*ib < *ia) ++ib;
        else {
            if (out.empty() || out.back() != *ia) out.push_back(*ia);
            ++ia; ++ib;
        }
    }
    return out;
}

// 同一題的二分搜尋版：對 a 排序後，用 lower_bound 逐一查 b 的元素
// 在 vector 上是 O(m log n)；同樣的程式碼邏輯若資料放在 list，
// lower_bound 會退化成 O(m * n)。
std::vector<int> intersectionByBinarySearch(std::vector<int> a,
                                            const std::vector<int>& b) {
    std::sort(a.begin(), a.end());
    a.erase(std::unique(a.begin(), a.end()), a.end());

    std::vector<int> out;
    for (int x : b) {
        auto it = std::lower_bound(a.begin(), a.end(), x);   // O(log n)：靠隨機存取
        if (it != a.end() && *it == x) {
            if (std::find(out.begin(), out.end(), x) == out.end()) out.push_back(x);
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】在排序好的時間序列上，取出某個時間區間的資料點
//   情境：監控系統把 (timestamp, value) 依時間存在 vector 裡，
//         前端要畫「10:00 到 10:30 之間」的圖。資料可能有幾十萬點，
//         不可能每次都線性掃。
//   為什麼用到本主題：lower_bound / upper_bound 在 vector 上是真正的 O(log n)，
//     而且回傳的兩個迭代器可以直接相減得到「區間內有幾筆」（O(1)），
//     也可以直接 vector(first, last) 一次複製出來。
//     這三件事（二分、相減、範圍建構）全都是 Random Access 才有的。
// -----------------------------------------------------------------------------
struct Sample {
    int    tsSeconds;   // 從當日 00:00 起算的秒數
    double value;
};

// 取出 [fromSec, toSec) 區間的樣本
std::vector<Sample> queryRange(const std::vector<Sample>& series,
                               int fromSec, int toSec) {
    auto byTime = [](const Sample& s, int t) { return s.tsSeconds < t; };
    auto timeBy = [](int t, const Sample& s) { return t < s.tsSeconds; };

    auto first = std::lower_bound(series.begin(), series.end(), fromSec, byTime);
    auto last  = std::upper_bound(series.begin(), series.end(), toSec - 1, timeBy);

    return std::vector<Sample>(first, last);   // 範圍建構，一次搞定
}

// 區間內有幾筆：只要相減，不必真的複製資料（O(1)）
std::ptrdiff_t countRange(const std::vector<Sample>& series,
                          int fromSec, int toSec) {
    auto byTime = [](const Sample& s, int t) { return s.tsSeconds < t; };
    auto timeBy = [](int t, const Sample& s) { return t < s.tsSeconds; };

    auto first = std::lower_bound(series.begin(), series.end(), fromSec, byTime);
    auto last  = std::upper_bound(series.begin(), series.end(), toSec - 1, timeBy);
    return last - first;            // Random Access 才能這樣寫
}

int main() {
    std::vector<int> vec = {10, 20, 30, 40, 50};

    std::cout << "=== Random Access Iterator 完整功能 ===" << std::endl;

    auto it = vec.begin();

    // 基本操作
    std::cout << "*it = " << *it << std::endl;

    // 前進後退
    ++it;
    std::cout << "++it: *it = " << *it << std::endl;
    --it;
    std::cout << "--it: *it = " << *it << std::endl;

    // 跳躍！O(1)，因為 vector 的元素連續，位址可以直接算出來
    it = it + 3;
    std::cout << "it + 3: *it = " << *it << std::endl;

    it = it - 2;
    std::cout << "it - 2: *it = " << *it << std::endl;

    // 下標存取（等同 *(it + n)，不做邊界檢查）
    it = vec.begin();
    std::cout << "it[2] = " << it[2] << std::endl;
    std::cout << "it[4] = " << it[4] << std::endl;

    // 計算距離
    auto begin = vec.begin();
    auto end = vec.end();
    std::cout << "end - begin = " << (end - begin) << std::endl;

    // 比較大小
    auto mid = vec.begin() + 2;
    std::cout << "begin < mid? " << (begin < mid ? "是" : "否") << std::endl;
    std::cout << "end > mid? " << (end > mid ? "是" : "否") << std::endl;

    // -------------------------------------------------------------------------
    std::cout << "\n=== 為什麼能 O(1) 跳躍：位址就是算出來的 ===" << std::endl;
    {
        std::cout << "sizeof(int) = " << sizeof(int) << " bytes" << std::endl;
        std::cout << "&vec[0] 與 &vec[3] 的位元組差 = "
                  << (reinterpret_cast<const char*>(&vec[3])
                      - reinterpret_cast<const char*>(&vec[0]))
                  << " = 3 * sizeof(int)" << std::endl;
        std::cout << "（實際位址每次執行都不同，這裡只印差值）" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== difference_type 是有號的 ===" << std::endl;
    {
        auto d1 = vec.end() - vec.begin();
        auto d2 = vec.begin() - vec.end();      // 可以是負的
        std::cout << "end - begin = " << d1 << std::endl;
        std::cout << "begin - end = " << d2 << "（有號，可以是負數）" << std::endl;
        std::cout << "difference_type 是有號型別嗎? " << std::boolalpha
                  << std::is_signed<std::vector<int>::difference_type>::value
                  << std::endl;
        std::cout << "size_type 是無號型別嗎? "
                  << std::is_unsigned<std::vector<int>::size_type>::value
                  << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== deque：是 Random Access，但不是 contiguous ===" << std::endl;
    {
        std::deque<int> dq = {1, 2, 3, 4, 5};
        auto dit = dq.begin();
        dit += 3;                                   // O(1) 跳躍，合法
        std::cout << "deque 支援 it += 3: " << *dit << std::endl;
        std::cout << "deque 支援 end - begin: " << (dq.end() - dq.begin())
                  << std::endl;

        std::vector<int> v = {1, 2, 3, 4, 5};
        bool vecContiguous = (&v[0] + 4 == &v[4]);
        std::cout << "vector: &v[0] + 4 == &v[4] ? " << std::boolalpha
                  << vecContiguous << "（連續）" << std::endl;
        std::cout << "deque : 小 deque 可能剛好落在同一區塊，"
                  << "但跨區塊就不成立 —— 不可依賴" << std::endl;
        std::cout << "→ 要連續請用 vector/array 的 data()；"
                  << "deque 沒有 data() 就是這個原因" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 二分搜尋的 O(log n) 是 Random Access 換來的 ===" << std::endl;
    {
        // 用「比較次數」而非計時來呈現，結果可重現
        std::vector<int> v(1024);
        for (int i = 0; i < 1024; ++i) v[i] = i * 2;
        std::list<int> l(v.begin(), v.end());

        int cmpVec = 0, cmpList = 0;
        auto cntVec  = [&cmpVec ](int a, int b) { ++cmpVec;  return a < b; };
        auto cntList = [&cmpList](int a, int b) { ++cmpList; return a < b; };

        auto hitVec  = std::lower_bound(v.begin(), v.end(), 1000, cntVec);
        auto hitList = std::lower_bound(l.begin(), l.end(), 1000, cntList);

        std::cout << "兩邊都找到 1000 嗎? " << std::boolalpha
                  << (*hitVec == 1000 && *hitList == 1000) << std::endl;
        std::cout << "1024 筆資料，lower_bound 的比較次數：" << std::endl;
        std::cout << "  vector: " << cmpVec  << " 次（O(log n)）" << std::endl;
        std::cout << "  list  : " << cmpList << " 次（也是 O(log n) 次比較）"
                  << std::endl;
        std::cout << "→ 比較次數一樣，但 list 為了「走到中點」必須一步步前進，" << std::endl;
        std::cout << "  總迭代器移動是 O(n)，所以實際上比線性掃還慢。" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== LeetCode 349. Intersection of Two Arrays ===" << std::endl;
    {
        auto show = [](const std::vector<int>& v) {
            std::cout << "[";
            for (std::size_t i = 0; i < v.size(); ++i) {
                if (i) std::cout << ",";
                std::cout << v[i];
            }
            std::cout << "]";
        };

        std::vector<int> a = {1, 2, 2, 1}, b = {2, 2};
        std::cout << "雙指標版 [1,2,2,1] ∩ [2,2] = ";
        show(intersection(a, b));
        std::cout << std::endl;

        std::vector<int> c = {4, 9, 5}, d = {9, 4, 9, 8, 4};
        std::cout << "雙指標版 [4,9,5] ∩ [9,4,9,8,4] = ";
        show(intersection(c, d));
        std::cout << std::endl;

        std::cout << "二分版   [4,9,5] ∩ [9,4,9,8,4] = ";
        show(intersectionByBinarySearch(c, d));
        std::cout << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務：時間序列的區間查詢 ===" << std::endl;
    {
        // 每 5 分鐘一筆，從 09:00 到 11:00
        std::vector<Sample> series;
        for (int t = 9 * 3600; t <= 11 * 3600; t += 300) {
            series.push_back({t, 20.0 + (t / 300) % 7});
        }
        std::cout << "資料點總數: " << series.size()
                  << "（09:00~11:00，每 5 分鐘一筆）" << std::endl;

        const int from = 10 * 3600;          // 10:00
        const int to   = 10 * 3600 + 1800;   // 10:30

        std::cout << "10:00~10:30 有幾筆（只相減，不複製）: "
                  << countRange(series, from, to) << std::endl;

        auto win = queryRange(series, from, to);
        std::cout << "取出的資料點: ";
        for (const auto& s : win) {
            int hh = s.tsSeconds / 3600, mm = (s.tsSeconds % 3600) / 60;
            std::cout << hh << ":" << (mm < 10 ? "0" : "") << mm << " ";
        }
        std::cout << std::endl;
        std::cout << "→ 二分定位 O(log n) + 相減算筆數 O(1) + 範圍建構，" << std::endl;
        std::cout << "  三件事全靠 Random Access Iterator。" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第五課：迭代器的五種分類7.cpp -o demo7

// === 預期輸出 ===
// === Random Access Iterator 完整功能 ===
// *it = 10
// ++it: *it = 20
// --it: *it = 10
// it + 3: *it = 40
// it - 2: *it = 20
// it[2] = 30
// it[4] = 50
// end - begin = 5
// begin < mid? 是
// end > mid? 是
//
// === 為什麼能 O(1) 跳躍：位址就是算出來的 ===
// sizeof(int) = 4 bytes
// &vec[0] 與 &vec[3] 的位元組差 = 12 = 3 * sizeof(int)
// （實際位址每次執行都不同，這裡只印差值）
//
// === difference_type 是有號的 ===
// end - begin = 5
// begin - end = -5（有號，可以是負數）
// difference_type 是有號型別嗎? true
// size_type 是無號型別嗎? true
//
// === deque：是 Random Access，但不是 contiguous ===
// deque 支援 it += 3: 4
// deque 支援 end - begin: 5
// vector: &v[0] + 4 == &v[4] ? true（連續）
// deque : 小 deque 可能剛好落在同一區塊，但跨區塊就不成立 —— 不可依賴
// → 要連續請用 vector/array 的 data()；deque 沒有 data() 就是這個原因
//
// === 二分搜尋的 O(log n) 是 Random Access 換來的 ===
// 兩邊都找到 1000 嗎? true
// 1024 筆資料，lower_bound 的比較次數：
//   vector: 10 次（O(log n)）
//   list  : 10 次（也是 O(log n) 次比較）
// → 比較次數一樣，但 list 為了「走到中點」必須一步步前進，
//   總迭代器移動是 O(n)，所以實際上比線性掃還慢。
//
// === LeetCode 349. Intersection of Two Arrays ===
// 雙指標版 [1,2,2,1] ∩ [2,2] = [2]
// 雙指標版 [4,9,5] ∩ [9,4,9,8,4] = [4,9]
// 二分版   [4,9,5] ∩ [9,4,9,8,4] = [4,9]
//
// === 日常實務：時間序列的區間查詢 ===
// 資料點總數: 25（09:00~11:00，每 5 分鐘一筆）
// 10:00~10:30 有幾筆（只相減，不複製）: 6
// 取出的資料點: 10:00 10:05 10:10 10:15 10:20 10:25
// → 二分定位 O(log n) + 相減算筆數 O(1) + 範圍建構，
//   三件事全靠 Random Access Iterator。
