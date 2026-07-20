// =============================================================================
//  第七課：演算法（Algorithm）與容器的分離設計14.cpp
//    —  數值演算法 <numeric>：accumulate / iota / partial_sum /
//       adjacent_difference / inner_product
// =============================================================================
//
// 【主題資訊 Information】
//   T     accumulate         (InputIt f, InputIt l, T init);                 // C++98
//   T     accumulate         (InputIt f, InputIt l, T init, BinaryOp op);    // C++98
//   void  iota               (FwdIt f, FwdIt l, T value);                    // C++11 ★
//   OutIt partial_sum        (InputIt f, InputIt l, OutIt d);                // C++98
//   OutIt adjacent_difference(InputIt f, InputIt l, OutIt d);                // C++98
//   T     inner_product      (InputIt1 f1, InputIt1 l1, InputIt2 f2, T init);// C++98
//
//   C++17 新增：std::reduce（可平行化的 accumulate）、
//               std::transform_reduce、std::inclusive_scan / exclusive_scan
//
//   標準版本：多數為 C++98；**std::iota 是 C++11 新增**
//   迭代器需求：多數為 Input Iterator；iota 需要 Forward Iterator
//   複雜度：全部 O(N)
//   標頭檔：**<numeric>**（注意不是 <algorithm>）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼數值演算法自成一個標頭檔】
// <algorithm> 裡的演算法多半只做「搬移、比較、篩選」，不對元素做算術運算；
// <numeric> 則專門處理「把一串值歸納成數值」或「數值序列之間的轉換」。
// 這個分類不只是整理，也反映了需求差異：數值演算法對型別的要求更強
// （要能相加、相乘），而且**累加型別的選擇會直接影響正確性**（見下一點）。
//
// 【2. accumulate 最經典的坑：init 的型別決定累加型別】
//     std::vector<double> v = {1.5, 2.5, 3.5};
//     auto sum = std::accumulate(v.begin(), v.end(), 0);      // ✗ 得到 7，不是 7.5
// 為什麼？因為 accumulate 的回傳型別由 **init 的型別 T** 決定，不是由元素型別決定。
// 傳 0（int）進去，內部累加器就是 int，每次加法的結果都被截斷成整數。
// 正確寫法是讓 init 的型別與需求一致：
//     std::accumulate(v.begin(), v.end(), 0.0);               // ✓ double，得到 7.5
// 同樣的坑也發生在整數溢位上：
//     std::vector<int> big = {2000000000, 2000000000};
//     std::accumulate(big.begin(), big.end(), 0);             // ✗ int 溢位（未定義行為）
//     std::accumulate(big.begin(), big.end(), 0LL);           // ✓ long long
// **口訣：init 不只是起始值，它同時宣告了「用什麼型別做運算」。**
//
// 【3. partial_sum 與 adjacent_difference 是一對互逆運算】
//     partial_sum        : {1,2,3,4,5} → {1,3,6,10,15}   （前綴和）
//     adjacent_difference: {1,3,6,10,15} → {1,2,3,4,5}   （差分，還原回去）
// 注意兩者的**第一個元素都是原封不動複製**的：
//   * partial_sum 的第一個輸出就是第一個輸入（前 1 項的和）
//   * adjacent_difference 的第一個輸出也是第一個輸入（沒有前一項可減）
// 這個設計讓兩者嚴格互逆，是離散微積分的基本操作。
// 前綴和的實務價值極高：**預先算好一次 O(N)，之後任意區間和都是 O(1)**
//     區間 [i, j] 的和 = prefix[j] - prefix[i-1]
// 這正是 LeetCode 303 的解法，也是所有「區間查詢」問題的基礎。
//
// 【4. inner_product 的名字取窄了：它其實是「兩序列歸納」的通用工具】
// 預設行為是「逐項相乘再全部相加」（數學上的內積／點積），
// 但它有四參數版本可以換掉這兩個運算：
//     inner_product(f1, l1, f2, init, op1, op2)
//     // op1 取代「加總」，op2 取代「相乘」
// 例如「計算兩個序列有幾個位置的值不同」（漢明距離）：
//     inner_product(a.begin(), a.end(), b.begin(), 0,
//                   std::plus<int>(),                        // 累加
//                   [](int x, int y){ return x != y; });     // 逐項比較
// 與 transform 的差別在於：transform 產生一個**新序列**，
// inner_product 則直接歸納成**單一值**，不需要中間容器。
// 注意它同樣**只給第二個範圍的起點**，第二個序列必須夠長（同二元 transform）。
//
// 【5. C++17 的 reduce：為什麼要另外開一個名字】
// std::reduce 看起來只是 accumulate 的翻版，差別在於：
//   * accumulate **保證由左至右依序累加**，所以不能平行化
//   * reduce **不保證順序與分組方式**，因此可以加上執行策略平行執行
// 代價是 reduce 要求運算必須**滿足結合律與交換律**。
// 浮點數加法**不滿足結合律**（(a+b)+c 與 a+(b+c) 可能得到不同結果），
// 所以對 double 用平行 reduce 時，結果可能每次略有差異——
// 這在財務計算中要特別小心。
//
// 【概念補充 Concept Deep Dive】
//
// (A) accumulate 的 op 不可修改元素、不可使結果依賴呼叫順序
//   標準要求 op 不得有副作用、不得使迭代器失效。
//   雖然 accumulate 保證由左至右，但仍不應在 op 內修改被走訪的容器。
//
// (B) iota 的名字來自 APL 語言
//   APL 的 ι（iota）運算子產生連續整數序列，STL 沿用了這個名字。
//   它填的是 value、++value、++++value…，所以**任何支援 ++ 的型別都能用**，
//   不限於整數——指標、iterator 也可以（例如產生一組指向各元素的指標）。
//
// (C) 為什麼 partial_sum 允許 d_first == first（就地運算）
//   標準明確允許輸出範圍與輸入範圍相同，因為它是「逐項往後累積」，
//   讀取當前元素後才寫入同一位置，不會踩到還沒讀的資料。
//   這與 transform 的就地轉換是同樣的道理（見第 7 個檔案）。
//
// 【注意事項 Pay Attention】
// 1. **標頭檔是 <numeric>**，不是 <algorithm>。忘了 include 會編譯失敗。
// 2. **accumulate 的 init 型別決定累加型別**：
//    對 double 序列傳 0（int）會被截斷；大數列傳 0 可能整數溢位（未定義行為）。
//    要寫 0.0 或 0LL。
// 3. std::multiplies / std::plus 等函式物件在 **<functional>**，需另外 include。
// 4. inner_product **只給第二個範圍的起點**，第二個序列必須至少一樣長，
//    否則讀取越界（同二元 transform，沒有安全的四參數版）。
// 5. **std::iota 是 C++11**；-std=c++98 編不過。
// 6. partial_sum / adjacent_difference 的第一個輸出元素是原值的複製。
// 7. C++17 的 std::reduce 不保證順序，要求運算滿足結合律；
//    浮點數加法不滿足結合律，平行化時結果可能有微小差異。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】數值演算法
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::accumulate(v.begin(), v.end(), 0) 對 vector<double>{1.5, 2.5, 3.5}
//        的結果是多少？
//     答：**7**，不是 7.5。因為 accumulate 的累加型別由 **init 的型別**決定，
//         傳 0 是 int，內部累加器就是 int，每次加法都被截斷。
//         正確寫法是傳 0.0，讓累加器成為 double。
//     追問：這個坑還有什麼變形？→ 整數溢位。對一串大 int 傳 0 進去，
//         累加器是 int，超過範圍就是未定義行為；應該傳 0LL 讓累加器是 long long。
//         口訣：**init 不只是起始值，它同時宣告了運算型別**。
//
// 🔥 Q2. partial_sum 和 adjacent_difference 有什麼關係？前綴和有什麼實務價值？
//     答：兩者互為逆運算。partial_sum 把 {1,2,3,4,5} 變成 {1,3,6,10,15}（前綴和），
//         adjacent_difference 再把它還原回 {1,2,3,4,5}（差分）。
//         前綴和的價值在於：**預先計算一次 O(N)，之後任意區間和都是 O(1)**——
//         區間 [i,j] 的和 = prefix[j] - prefix[i-1]。
//         所有「多次區間查詢」的問題都建立在這個技巧上。
//     追問：兩者的第一個輸出元素是什麼？→ 都是第一個輸入元素的複製。
//         partial_sum 的第一項就是前 1 項的和；adjacent_difference 的第一項
//         沒有前一項可減，所以原封不動。這個設計讓兩者嚴格互逆。
//
// ⚠️ 陷阱. std::accumulate 和 C++17 的 std::reduce 可以互換嗎？
//     答：不能無條件互換。accumulate **保證由左至右依序累加**；
//         reduce **不保證順序與分組**，所以才能平行化，但也因此要求運算
//         必須滿足結合律與交換律。關鍵在於：**浮點數加法不滿足結合律**
//         （(a+b)+c 與 a+(b+c) 的捨入誤差不同），
//         所以對 double 用平行 reduce，每次執行的結果可能有微小差異。
//     為什麼會錯：大家看到 reduce 的簽名幾乎和 accumulate 一樣，
//         就以為只是「比較新、比較快」的版本而直接替換。
//         在財務或科學計算中，這種不可重現的微小差異可能是嚴重問題。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <numeric>
#include <algorithm>
#include <functional>   // std::multiplies / std::plus

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 303. Range Sum Query - Immutable
//   題目：給定不變的整數陣列，多次查詢區間 [left, right] 的元素和。
//   為什麼用到本主題：**這題就是 partial_sum 的定義用途**。
//         建構時用 partial_sum 算好前綴和（一次 O(N)），
//         之後每次查詢只要一次減法，O(1) 完成。
//         若每次查詢都重新累加，M 次查詢就是 O(M*N)，會超時。
//   複雜度：建構 O(N)，每次查詢 O(1)。
// -----------------------------------------------------------------------------
class NumArray {
public:
    explicit NumArray(const std::vector<int>& nums) : prefix_(nums.size()) {
        std::partial_sum(nums.begin(), nums.end(), prefix_.begin());
    }

    int sumRange(int left, int right) const {
        // 區間 [left, right] 的和 = prefix[right] - prefix[left-1]
        if (left == 0) return prefix_[static_cast<std::size_t>(right)];
        return prefix_[static_cast<std::size_t>(right)] -
               prefix_[static_cast<std::size_t>(left - 1)];
    }

private:
    std::vector<int> prefix_;
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 1732. Find the Highest Altitude
//   題目：單車手從高度 0 出發，gain[i] 是第 i 點到第 i+1 點的高度變化，
//         求整趟路程中的最高高度。
//   為什麼用到本主題：把「變化量」還原成「實際高度」正是**前綴和**，
//         再取最大值即可。這題示範 partial_sum 與 max_element 的組合，
//         也對照出它與 adjacent_difference 的互逆關係
//         （gain 本身就是高度序列的差分）。
//   複雜度：O(N)。
// -----------------------------------------------------------------------------
int largestAltitude(const std::vector<int>& gain) {
    std::vector<int> altitude(gain.size());
    std::partial_sum(gain.begin(), gain.end(), altitude.begin());
    // 起點高度 0 也要納入比較（可能全程都在下坡）
    int highest = 0;
    if (!altitude.empty()) {
        highest = std::max(0, *std::max_element(altitude.begin(), altitude.end()));
    }
    return highest;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】流量計費：從累計計數器還原每分鐘用量並結算
//   情境：網路介面的 byte 計數器是**單調遞增的累計值**（例如 /proc/net/dev），
//         監控系統每分鐘取樣一次。要產出報表必須：
//         (a) 用 adjacent_difference 從累計值還原「每分鐘實際用量」
//         (b) 用 accumulate 加總得到總流量（注意型別要用 long long 防溢位）
//         (c) 用 inner_product 依各時段不同費率計算總費用（尖峰/離峰）
//   為什麼用到本主題：這是 <numeric> 三個主力函式在真實場景中的完整協作，
//         也示範了 accumulate 的 init 型別為何必須慎選。
// -----------------------------------------------------------------------------
void billingReport(const std::vector<long long>& cumulativeBytes,
                   const std::vector<double>& ratePerMB) {
    if (cumulativeBytes.empty()) return;

    // (a) 累計值 → 每分鐘用量（差分）
    std::vector<long long> perMinute(cumulativeBytes.size());
    std::adjacent_difference(cumulativeBytes.begin(), cumulativeBytes.end(),
                             perMinute.begin());
    // 第一項是原值的複製（沒有前一項可減），代表取樣起點的基準值，不計入用量
    perMinute[0] = 0;

    std::cout << "  每分鐘用量(bytes): ";
    for (long long b : perMinute) std::cout << b << " ";
    std::cout << std::endl;

    // (b) 總流量：★ init 必須是 0LL，用 0 會讓累加器變成 int 而可能溢位
    long long total = std::accumulate(perMinute.begin(), perMinute.end(), 0LL);
    std::cout << "  總流量: " << total << " bytes ("
              << std::fixed << std::setprecision(2)
              << static_cast<double>(total) / (1024.0 * 1024.0) << " MB)" << std::endl;

    // (c) 依各分鐘不同費率計算總費用：逐項相乘再加總 = inner_product
    if (ratePerMB.size() >= perMinute.size()) {
        std::vector<double> mb(perMinute.size());
        std::transform(perMinute.begin(), perMinute.end(), mb.begin(),
                       [](long long b) { return static_cast<double>(b) / (1024.0 * 1024.0); });
        double cost = std::inner_product(mb.begin(), mb.end(), ratePerMB.begin(), 0.0);
        std::cout << "  總費用: " << std::fixed << std::setprecision(4)
                  << cost << " 元 (各分鐘費率不同，用 inner_product 一次算完)" << std::endl;
    }
}

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5};

    // accumulate：累加, 這裡計算 vec 中所有元素的總和
    std::cout << "=== accumulate ===" << std::endl;
    int sum = std::accumulate(vec.begin(), vec.end(), 0);
    std::cout << "總和: " << sum << std::endl;

    // accumulate 可以自訂運算, 這裡計算 vec 中所有元素的乘積
    int product = std::accumulate(vec.begin(), vec.end(), 1,
        std::multiplies<int>());
    std::cout << "乘積: " << product << std::endl;

    // ★ init 的型別決定累加型別 —— 最經典的坑
    std::cout << "\n=== 陷阱：init 型別決定累加型別 ===" << std::endl;
    std::vector<double> prices = {1.5, 2.5, 3.5};
    auto wrong = std::accumulate(prices.begin(), prices.end(), 0);     // int 累加器
    auto right = std::accumulate(prices.begin(), prices.end(), 0.0);   // double 累加器
    std::cout << "{1.5, 2.5, 3.5} 用 init=0   (int)   : " << wrong
              << "  ← 每次加法都被截斷" << std::endl;
    std::cout << "{1.5, 2.5, 3.5} 用 init=0.0 (double): " << right
              << "  ← 正確" << std::endl;

    // iota：填充遞增序列, 這裡填充一個從 1 開始的遞增序列到 seq 中
    // ★ std::iota 是 C++11 新增
    std::cout << "\n=== iota ===" << std::endl;
    std::vector<int> seq(10);
    std::iota(seq.begin(), seq.end(), 1);  // 從 1 開始
    std::cout << "iota: ";
    for (int n : seq) std::cout << n << " ";
    std::cout << std::endl;

    // partial_sum：部分和, 這裡計算 vec 中元素的部分和，結果存儲在 partial 中
    std::cout << "\n=== partial_sum ===" << std::endl;
    std::vector<int> partial(vec.size());
    std::partial_sum(vec.begin(), vec.end(), partial.begin());
    std::cout << "部分和: ";
    for (int n : partial) std::cout << n << " ";
    std::cout << std::endl;

    // adjacent_difference：相鄰差, 這裡計算 data 中相鄰元素的差，結果存儲在 diff 中
    std::cout << "\n=== adjacent_difference ===" << std::endl;
    std::vector<int> data = {1, 3, 6, 10, 15};
    std::vector<int> diff(data.size());
    std::adjacent_difference(data.begin(), data.end(), diff.begin());
    std::cout << "相鄰差: ";
    for (int n : diff) std::cout << n << " ";
    std::cout << std::endl;

    // ★ 兩者互為逆運算
    std::cout << "\n=== partial_sum 與 adjacent_difference 互逆 ===" << std::endl;
    std::vector<int> original = {1, 2, 3, 4, 5};
    std::vector<int> summed(original.size());
    std::vector<int> restored(original.size());
    std::partial_sum(original.begin(), original.end(), summed.begin());
    std::adjacent_difference(summed.begin(), summed.end(), restored.begin());
    std::cout << "原始:     ";
    for (int n : original) std::cout << n << " ";
    std::cout << std::endl;
    std::cout << "前綴和:   ";
    for (int n : summed) std::cout << n << " ";
    std::cout << std::endl;
    std::cout << "再差分:   ";
    for (int n : restored) std::cout << n << " ";
    std::cout << "  ← 完全還原" << std::endl;

    // inner_product：內積, 這裡計算 v1 和 v2 的內積（點積）
    std::cout << "\n=== inner_product ===" << std::endl;
    std::vector<int> v1 = {1, 2, 3};
    std::vector<int> v2 = {4, 5, 6};
    int dot = std::inner_product(v1.begin(), v1.end(), v2.begin(), 0);
    std::cout << "內積: " << dot << " (1*4 + 2*5 + 3*6 = 32)" << std::endl;

    // ★ 四參數版：換掉「加總」與「相乘」，做漢明距離
    std::cout << "\n=== inner_product 換運算：計算兩序列有幾處不同 ===" << std::endl;
    std::vector<int> a = {1, 2, 3, 4, 5};
    std::vector<int> b = {1, 9, 3, 9, 5};
    int hamming = std::inner_product(a.begin(), a.end(), b.begin(), 0,
                                     std::plus<int>(),
                                     [](int x, int y) { return x != y ? 1 : 0; });
    std::cout << "{1,2,3,4,5} vs {1,9,3,9,5} 相異處: " << hamming << " 個" << std::endl;

    std::cout << "\n=== LeetCode 303. Range Sum Query - Immutable ===" << std::endl;
    NumArray na({-2, 0, 3, -5, 2, -1});
    std::cout << "sumRange(0, 2) = " << na.sumRange(0, 2) << std::endl;
    std::cout << "sumRange(2, 5) = " << na.sumRange(2, 5) << std::endl;
    std::cout << "sumRange(0, 5) = " << na.sumRange(0, 5) << std::endl;

    std::cout << "\n=== LeetCode 1732. Find the Highest Altitude ===" << std::endl;
    std::cout << "[-5,1,5,0,-7]   -> " << largestAltitude({-5, 1, 5, 0, -7}) << std::endl;
    std::cout << "[-4,-3,-2,-1,4,3,2] -> " << largestAltitude({-4, -3, -2, -1, 4, 3, 2}) << std::endl;

    std::cout << "\n=== 日常實務：流量計費報表 ===" << std::endl;
    // 累計 byte 計數器（單調遞增），每分鐘取樣一次
    std::vector<long long> counter = {1000000, 3500000, 8200000, 9000000, 15000000};
    // 各分鐘費率（尖峰時段較貴）
    std::vector<double> rates = {0.01, 0.01, 0.05, 0.05, 0.01};
    billingReport(counter, rates);

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第七課：演算法（Algorithm）與容器的分離設計14.cpp -o demo14

// === 預期輸出 ===
// === accumulate ===
// 總和: 15
// 乘積: 120
//
// === 陷阱：init 型別決定累加型別 ===
// {1.5, 2.5, 3.5} 用 init=0   (int)   : 6  ← 每次加法都被截斷
// {1.5, 2.5, 3.5} 用 init=0.0 (double): 7.5  ← 正確
//
// === iota ===
// iota: 1 2 3 4 5 6 7 8 9 10 
//
// === partial_sum ===
// 部分和: 1 3 6 10 15 
//
// === adjacent_difference ===
// 相鄰差: 1 2 3 4 5 
//
// === partial_sum 與 adjacent_difference 互逆 ===
// 原始:     1 2 3 4 5 
// 前綴和:   1 3 6 10 15 
// 再差分:   1 2 3 4 5   ← 完全還原
//
// === inner_product ===
// 內積: 32 (1*4 + 2*5 + 3*6 = 32)
//
// === inner_product 換運算：計算兩序列有幾處不同 ===
// {1,2,3,4,5} vs {1,9,3,9,5} 相異處: 2 個
//
// === LeetCode 303. Range Sum Query - Immutable ===
// sumRange(0, 2) = 1
// sumRange(2, 5) = -1
// sumRange(0, 5) = -3
//
// === LeetCode 1732. Find the Highest Altitude ===
// [-5,1,5,0,-7]   -> 1
// [-4,-3,-2,-1,4,3,2] -> 0
//
// === 日常實務：流量計費報表 ===
//   每分鐘用量(bytes): 0 2500000 4700000 800000 6000000 
//   總流量: 14000000 bytes (13.35 MB)
//   總費用: 0.3433 元 (各分鐘費率不同，用 inner_product 一次算完)
