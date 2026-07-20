// =============================================================================
//  第七課：演算法（Algorithm）與容器的分離設計3.cpp
//    —  搜尋家族：find / find_if / find_if_not / adjacent_find
// =============================================================================
//
// 【主題資訊 Information】
//   InputIt  find        (InputIt f, InputIt l, const T& value);        // C++98
//   InputIt  find_if     (InputIt f, InputIt l, UnaryPred p);           // C++98
//   InputIt  find_if_not (InputIt f, InputIt l, UnaryPred p);           // C++11 ★
//   FwdIt    adjacent_find(FwdIt f, FwdIt l);                           // C++98
//   FwdIt    adjacent_find(FwdIt f, FwdIt l, BinaryPred p);             // C++98
//
//   標準版本：find / find_if / adjacent_find 為 C++98；**find_if_not 是 C++11 新增**
//   迭代器需求：find 系列要 Input Iterator；adjacent_find 要 Forward Iterator
//   複雜度：全部 O(N)，最多掃描一次
//   回傳：命中的迭代器；找不到一律回傳 last
//   標頭檔：<algorithm>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「找不到」要回傳 last，而不是 nullptr 或丟例外】
// 這是分離設計的直接後果。演算法只拿到 [first, last)，它**沒有任何「無效值」
// 可以回傳**——iterator 不保證能表示 null（list 的 iterator 是包一個節點指標，
// 但 vector 的可能就是裸指標，兩者沒有共同的「空值」約定）。
// 於是 STL 選了範圍內唯一一個「保證存在、且保證不是有效元素」的位置：last。
// 這個設計的好處是統一：所有搜尋演算法的失敗判斷寫法完全一樣。
// 代價是：**呼叫端一定要先檢查再解參考**，否則 *last 是未定義行為。
//
// 【2. find_if_not 為什麼要等到 C++11 才出現】
// C++98 時代的想法是「用 std::not1 包起來就好」：
//     std::find_if(f, l, std::not1(pred));
// 但 not1 要求 pred 是 Adaptable Function Object（必須自帶 argument_type
// 這類 typedef），**lambda 沒有這些 typedef**，所以 C++11 引入 lambda 後
// not1 反而配不上最常用的謂詞形式。與其要求大家寫 [](int n){ return !(n < 30); }
// （雙重否定、易讀性差），不如直接補一個 find_if_not。
// 補充：std::not1 在 C++17 被標記為 deprecated、C++20 移除；
//       現代的取代品是 C++17 的 std::not_fn。
//
// 【3. adjacent_find：唯一「同時看兩個元素」的搜尋演算法】
// find 系列一次只看一個元素，adjacent_find 則比較 *it 與 *(it+1)。
// 這使它需要 Forward Iterator（能夠儲存位置並重新走訪），
// 不能只有 Input Iterator（Input Iterator 是「單次通過」的，讀過就不保證能回頭）。
// 回傳的是**這一對的第一個**元素的迭代器，不是第二個——這點常記錯。
// 它的典型用途：
//   * 偵測排序後的重複（先 sort 再 adjacent_find，是 O(N log N) 的查重）
//   * 找出資料序列中「沒有變化」的平台區段（配合自訂二元謂詞）
//   * 找出違反單調遞增的第一個位置（謂詞寫成 a >= b）
//
// 【4. 四個演算法的選用決策】
//     要找「等於某個值」               → find
//     要找「滿足條件」                 → find_if
//     要找「不滿足條件」               → find_if_not（別用雙重否定的 find_if）
//     要找「相鄰兩個元素之間的關係」   → adjacent_find
//   注意 find_if_not(p) 等價於 find_if(!p)，但語意表達更清楚，
//   而且對「找第一個不合格的資料」這種需求，讀起來就是需求本身。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 it - vec.begin() 只在部分容器上能用
//   本檔用 (it - vec.begin()) 取得索引，這需要 Random Access Iterator
//   的 operator-。若容器換成 list，這行會**編譯失敗**，必須改成
//   std::distance(lst.begin(), it)，而且複雜度從 O(1) 變成 O(N)。
//   → 通用程式碼一律用 std::distance；它會依 iterator category
//     自動選擇 O(1) 或 O(N) 的實作（標籤分派，tag dispatch）。
//
// (B) adjacent_find 的二元謂詞參數順序
//   pred(*it, *(it+1))：第一個參數是前面的元素，第二個是後面的。
//   寫「找出下降的位置」時要注意方向：pred = [](int a, int b){ return a > b; }
//   會回傳第一個「比後面大」的元素位置。順序寫反會找到相反的東西。
//
// (C) 這些演算法都是 short-circuit（找到就停）
//   find 系列一命中就立刻回傳，不會掃完全部。所以「最壞 O(N)」是上界，
//   平均往往遠小於 N。相對地，count / count_if **一定**掃完全部，
//   因為它要算總數。若只想知道「有沒有」，用 find_if 或 any_of，
//   不要用 count_if(...) > 0——後者會白掃剩下的元素。
//
// 【注意事項 Pay Attention】
// 1. 解參考前一定要先檢查 != last，否則 *last 是未定義行為。
// 2. adjacent_find 回傳的是**相鄰對的第一個**元素的迭代器。
//    要取得第二個要自己 ++（且要先確認不是 last）。
// 3. adjacent_find 只偵測**相鄰**重複；不相鄰的重複要先排序或改用 set/map。
// 4. find_if_not 是 C++11；用 -std=c++98 會編不過。
// 5. 想知道「有沒有」而不需要位置時，用 any_of 比 find_if 語意更清楚；
//    絕不要用 count_if(...) > 0，那會白白掃完整個範圍。
// 6. 對已排序的資料找特定值，用 lower_bound / binary_search（O(log N)），
//    不要用 find（O(N)）——見本課第 13 個檔案。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】搜尋演算法家族
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 find 找不到時回傳 last，而不是回傳 nullptr 或丟例外？
//     答：因為演算法只拿到 [first, last) 兩個 iterator，iterator 型別不保證
//         存在「空值」表示法（list 的 iterator 包節點指標、vector 的可能是裸指標，
//         沒有共同的 null 約定）。last 是範圍內唯一保證存在、又保證不是有效
//         元素的位置，所以被選為統一的失敗哨兵。
//     追問：那 *find(...) 直接用會怎樣？→ 若沒命中就是對 last 解參考，
//         未定義行為。必須先寫 if (it != v.end())。
//
// 🔥 Q2. 已經有 find_if 了，為什麼 C++11 還要加 find_if_not？
//     答：C++98 的做法是 find_if(f, l, std::not1(pred))，但 not1 要求謂詞是
//         Adaptable Function Object（要有 argument_type 等 typedef），
//         而 lambda 沒有這些 typedef，配不起來。與其強迫大家寫雙重否定的
//         lambda，不如直接提供 find_if_not。not1 已於 C++17 deprecated、
//         C++20 移除，現代替代品是 C++17 的 std::not_fn。
//     追問：那 find_if_not 和 find_if 效能有差嗎？→ 沒有，兩者都是單次 O(N)
//         掃描且找到就停，差別純粹在可讀性與 API 設計。
//
// ⚠️ 陷阱. 「adjacent_find 可以用來找出容器裡所有重複的元素」——錯在哪？
//     答：錯。adjacent_find 只看**相鄰**的兩個元素。對 {1, 2, 3, 2} 它回傳
//         end()，因為兩個 2 並不相鄰。要找出所有重複，得先 std::sort
//         （讓相同值聚在一起）再 adjacent_find，或改用 set / unordered_set 記錄。
//     為什麼會錯：名字裡的 adjacent 常被略過，被當成通用的 find_duplicate。
//         同樣的誤解也發生在 std::unique 上——它同樣只處理**連續**重複，
//         所以標準用法永遠是「先 sort 再 unique」。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 217. Contains Duplicate
//   題目：給定整數陣列 nums，若任一值出現至少兩次回傳 true，全部相異回傳 false。
//   為什麼用到本主題：排序讓相同的值變成「相鄰」，正好命中 adjacent_find 的
//         適用條件——這也直接示範了「adjacent_find 只看相鄰」這個陷阱的正解。
//   複雜度：O(N log N)（排序主導），空間 O(1)（不含輸入複製）。
//   註：用 unordered_set 可做到平均 O(N)，這裡示範的是本主題的解法。
// -----------------------------------------------------------------------------
bool containsDuplicate(std::vector<int> nums) {
    std::sort(nums.begin(), nums.end());
    return std::adjacent_find(nums.begin(), nums.end()) != nums.end();
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】感測器資料品質檢查
//   情境：溫度感測器每分鐘回報一次。有兩種故障要抓：
//         (a) 出現不合理讀值（超出量測範圍）→ find_if_not 找第一個「不在合理範圍」的
//         (b) 感測器卡死：連續兩筆完全相同 → adjacent_find 抓相鄰重複
//   為什麼用到本主題：這兩種檢查剛好對應「單元素條件」與「相鄰元素關係」，
//         是 find_if_not 與 adjacent_find 各自的典型用途。
// -----------------------------------------------------------------------------
void checkSensorData(const std::vector<double>& readings) {
    // (a) 找第一個超出合理範圍(-40 ~ 85 度)的讀值
    auto bad = std::find_if_not(readings.begin(), readings.end(),
                                [](double t) { return t >= -40.0 && t <= 85.0; });
    if (bad != readings.end()) {
        std::cout << "  [WARN] 索引 " << (bad - readings.begin())
                  << " 讀值超出量測範圍: " << *bad << std::endl;
    } else {
        std::cout << "  [OK] 所有讀值都在量測範圍內" << std::endl;
    }

    // (b) 連續兩筆完全相同 → 感測器可能卡死
    auto stuck = std::adjacent_find(readings.begin(), readings.end());
    if (stuck != readings.end()) {
        std::cout << "  [WARN] 索引 " << (stuck - readings.begin())
                  << " 起連續兩筆相同 (" << *stuck << ")，感測器可能卡死" << std::endl;
    } else {
        std::cout << "  [OK] 沒有偵測到連續重複讀值" << std::endl;
    }

    // (c) 用二元謂詞找「溫度驟降超過 10 度」的位置
    auto drop = std::adjacent_find(readings.begin(), readings.end(),
                                   [](double a, double b) { return a - b > 10.0; });
    if (drop != readings.end()) {
        std::cout << "  [WARN] 索引 " << (drop - readings.begin())
                  << " 溫度驟降: " << *drop << " -> " << *(drop + 1) << std::endl;
    } else {
        std::cout << "  [OK] 沒有偵測到溫度驟降" << std::endl;
    }
}

int main() {
    std::vector<int> vec = {10, 20, 30, 40, 30, 50, 30};

    // find：找第一個等於某值的元素, 如果找不到會回傳 end() 迭代器
    std::cout << "=== find ===" << std::endl;
    auto it = std::find(vec.begin(), vec.end(), 30);
    if (it != vec.end()) {
        std::cout << "找到 30，位置: " << (it - vec.begin()) << std::endl;
    }

    // find_if：找第一個滿足條件的元素, 例如找第一個大於 25 的元素
    std::cout << "\n=== find_if ===" << std::endl;
    auto it2 = std::find_if(vec.begin(), vec.end(),
        [](int n) { return n > 25; });
    if (it2 != vec.end()) {
        std::cout << "第一個大於 25 的元素: " << *it2 << std::endl;
    }

    // find_if_not：找第一個不滿足條件的元素, 例如找第一個不小於 30 的元素
    // ★ find_if_not 是 C++11 新增；C++98 只能用 not1 包裝，但 not1 配不上 lambda
    std::cout << "\n=== find_if_not ===" << std::endl;
    auto it3 = std::find_if_not(vec.begin(), vec.end(),
        [](int n) { return n < 30; });
    if (it3 != vec.end()) {
        std::cout << "第一個不小於 30 的元素: " << *it3 << std::endl;
    }

    // adjacent_find：找連續相同的元素, 例如找第一對相鄰相同的元素
    // ★ 回傳的是「這一對的第一個」元素的迭代器
    std::cout << "\n=== adjacent_find ===" << std::endl;
    std::vector<int> v2 = {1, 2, 3, 3, 4, 5, 5, 5};
    auto it4 = std::adjacent_find(v2.begin(), v2.end());
    if (it4 != v2.end()) {
        std::cout << "第一對相鄰相同元素: " << *it4 << std::endl;
        std::cout << "它的索引: " << (it4 - v2.begin())
                  << "，下一個元素: " << *(it4 + 1) << std::endl;
    }

    // ★ 陷阱示範：adjacent_find 只看「相鄰」，不相鄰的重複抓不到
    std::cout << "\n=== 陷阱：不相鄰的重複抓不到 ===" << std::endl;
    std::vector<int> v3 = {1, 2, 3, 2};       // 兩個 2 不相鄰
    auto it5 = std::adjacent_find(v3.begin(), v3.end());
    std::cout << "{1,2,3,2} 直接 adjacent_find: "
              << (it5 == v3.end() ? "沒找到（但其實有重複！）" : "找到") << std::endl;
    std::sort(v3.begin(), v3.end());          // 先排序，讓相同值相鄰
    auto it6 = std::adjacent_find(v3.begin(), v3.end());
    std::cout << "先 sort 再 adjacent_find:   "
              << (it6 == v3.end() ? "沒找到" : "找到重複值 ") ;
    if (it6 != v3.end()) std::cout << *it6;
    std::cout << std::endl;

    std::cout << "\n=== LeetCode 217. Contains Duplicate ===" << std::endl;
    std::cout << "[1,2,3,1] -> " << (containsDuplicate({1, 2, 3, 1}) ? "true" : "false") << std::endl;
    std::cout << "[1,2,3,4] -> " << (containsDuplicate({1, 2, 3, 4}) ? "true" : "false") << std::endl;

    std::cout << "\n=== 日常實務：感測器資料品質檢查 ===" << std::endl;
    std::cout << "資料組 A (正常):" << std::endl;
    checkSensorData({21.5, 22.0, 22.4, 23.1, 22.8});
    std::cout << "資料組 B (有異常):" << std::endl;
    checkSensorData({21.5, 22.0, 22.0, 5.0, 999.0});

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第七課：演算法（Algorithm）與容器的分離設計3.cpp -o demo3

// === 預期輸出 ===
// === find ===
// 找到 30，位置: 2
//
// === find_if ===
// 第一個大於 25 的元素: 30
//
// === find_if_not ===
// 第一個不小於 30 的元素: 30
//
// === adjacent_find ===
// 第一對相鄰相同元素: 3
// 它的索引: 2，下一個元素: 3
//
// === 陷阱：不相鄰的重複抓不到 ===
// {1,2,3,2} 直接 adjacent_find: 沒找到（但其實有重複！）
// 先 sort 再 adjacent_find:   找到重複值 2
//
// === LeetCode 217. Contains Duplicate ===
// [1,2,3,1] -> true
// [1,2,3,4] -> false
//
// === 日常實務：感測器資料品質檢查 ===
// 資料組 A (正常):
//   [OK] 所有讀值都在量測範圍內
//   [OK] 沒有偵測到連續重複讀值
//   [OK] 沒有偵測到溫度驟降
// 資料組 B (有異常):
//   [WARN] 索引 4 讀值超出量測範圍: 999
//   [WARN] 索引 1 起連續兩筆相同 (22)，感測器可能卡死
//   [WARN] 索引 2 溫度驟降: 22 -> 5
