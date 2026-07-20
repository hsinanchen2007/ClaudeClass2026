// =============================================================================
//  第 16 課：vector 的迭代器操作 10  —  std::distance：通用的「兩點間距離」
// =============================================================================
//
// 【主題資訊 Information】
//   template<class It>
//   typename std::iterator_traits<It>::difference_type
//   distance(It first, It last);
//   標頭檔：<iterator>
//   標準版本：C++98（C++17 起加上 constexpr）
//   複雜度：隨機存取迭代器 O(1)；其他類別 O(n)
//   回傳：有號的 difference_type（本機為 8 位元組的 ptrdiff_t）
//
// 【詳細解釋 Explanation】
//
// 【1. 既然可以寫 it2 - it1，為什麼還要 std::distance】
//   因為 it2 - it1 只有「隨機存取迭代器」才有定義。對 list、set、map 的
//   雙向迭代器，減法根本不存在，寫了會編譯失敗。
//   std::distance 提供的是一個「對所有迭代器類別都成立」的通用介面：
//       vector → 內部直接相減，O(1)
//       list   → 內部逐步 ++ 計數，O(n)
//   所以寫泛型程式碼（模板函式，事先不知道會收到什麼容器）時，
//   一律用 std::distance；只有確定是 vector 且想強調 O(1) 時才直接相減。
//
// 【2. 它怎麼做到「同一個名字、兩種複雜度」—— tag dispatch】
//   實作大致長這樣：
//       template<class It>
//       auto distance(It first, It last) {
//           using cat = typename iterator_traits<It>::iterator_category;
//           return __distance(first, last, cat{});     // 傳一個「空的標籤物件」
//       }
//       // 兩個多載，靠參數型別在編譯期選擇
//       __distance(It f, It l, random_access_iterator_tag) { return l - f; }
//       __distance(It f, It l, input_iterator_tag)
//           { difference_type n = 0; while (f != l) { ++f; ++n; } return n; }
//   標籤型別是空的 struct，不佔空間、不產生執行期程式碼，
//   選擇完全在編譯期完成 —— 執行期沒有任何 if 或虛擬呼叫。
//
// 【3. 它最常見的實際用途：把迭代器換算成索引】
//   演算法（std::find、std::lower_bound…）回傳的是迭代器，
//   但你要輸出的往往是「第幾筆」。標準寫法就是：
//       auto it = std::find(v.begin(), v.end(), target);
//       if (it != v.end()) {
//           auto idx = std::distance(v.begin(), it);   // 0-based 索引
//       }
//   注意一定要先比對 != end() 再算距離，否則得到的是 size() 而不是「找不到」。
//
// 【4. 參數順序與負值】
//   distance(first, last) 算的是「從 first 走到 last 要幾步」，
//   順序寫反會得到負數（對隨機存取迭代器而言是合法且有意義的）。
//   但對非隨機存取的迭代器，順序寫反是 UB —— 它只會一直 ++ 卻永遠碰不到
//   終點，實際結果不可預期，不保證會停下來。
//
// 【概念補充 Concept Deep Dive】
//   回傳型別是有號的 difference_type 而不是 size_t，這是刻意的：
//   隨機存取迭代器的相減是「向量差」，方向有意義。若你把它存進無號型別：
//       std::size_t d = std::distance(it2, it1);   // it1 在 it2 前面
//   負值會回繞成極大的正數（本機 size_t 為 64 位元），
//   拿去當長度或迴圈上界就會嚴重越界。用 auto 或明寫
//   std::vector<int>::difference_type 才安全。
//   另外 C++20 起 std::ranges::distance 也能接受「範圍物件」本身
//   （ranges::distance(v)），對有 size() 的容器是 O(1)，比較不容易寫錯順序。
//
// 【注意事項 Pay Attention】
//   1. 兩個迭代器必須來自「同一個容器」，否則是 UB（即使跑起來好像沒事）。
//   2. 對非隨機存取迭代器，first 必須能走到 last，順序寫反是 UB。
//   3. 回傳值是有號型別，不要存進 size_t。
//   4. 對 list 呼叫 distance 是 O(n)，放在迴圈裡會意外變成 O(n²)。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::distance
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 已經有 it2 - it1 了，為什麼還需要 std::distance？
//     答：減法只有隨機存取迭代器才有定義；list、set、map 的雙向迭代器寫減法
//         會編譯失敗。std::distance 用 tag dispatch 提供統一介面：
//         隨機存取走相減 O(1)，其他類別走 ++ 計數 O(n)。
//         泛型程式碼一律用 distance，才不會綁死容器。
//     追問：那 distance 對 list 是 O(n)，會有什麼實務風險？
//         → 若寫在迴圈裡（例如每輪都算目前位置），整體會從 O(n) 惡化成 O(n²)。
//
// 🔥 Q2. 怎麼把 std::find 回傳的迭代器換成索引？要注意什麼？
//     答：auto idx = std::distance(v.begin(), it);
//         關鍵是必須先檢查 it != v.end()。若沒找到，it 就是 end()，
//         算出來的距離剛好等於 size()，看起來像個合法索引，
//         拿去 v[idx] 就是越界 UB。
//
// ⚠️ 陷阱. std::size_t d = std::distance(it2, it1); 有什麼問題？
//     答：distance 回傳的是「有號」的 difference_type。當 it1 在 it2 前面時
//         結果是負數，存進無號的 size_t 會回繞成極大正數
//         （本機 size_t 為 64 位元，-1 會變成 18446744073709551615）。
//         之後拿它當長度、迴圈上界或索引，就是大範圍越界。
//     為什麼會錯：直覺認為「距離不可能是負的，用 size_t 語意上更貼切」，
//         但迭代器相減是有方向的差值，不是絕對距離。用 auto 接就不會錯。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <iterator>  // std::distance
#include <algorithm> // std::find
#include <string>

void demo_distance() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    auto it1 = v.begin();
    auto it2 = v.begin() + 3;

    // 方法一：直接相減（僅限隨機存取迭代器）
    std::cout << "it2 - it1 = " << (it2 - it1) << std::endl;

    // 方法二：std::distance()（適用於所有迭代器類型）
    std::cout << "distance = " << std::distance(it1, it2) << std::endl;

    // 順序反過來：對隨機存取迭代器會得到負值（合法且有意義）
    std::cout << "distance(it2, it1) = " << std::distance(it2, it1)
              << "（有號，負值正常）" << std::endl;

    // 典型用途：把 find 的結果換算成索引 —— 一定要先檢查 != end()
    auto found = std::find(v.begin(), v.end(), 40);
    if (found != v.end()) {
        std::cout << "40 的索引 = " << std::distance(v.begin(), found) << std::endl;
    }
    auto missing = std::find(v.begin(), v.end(), 99);
    std::cout << "99 是否存在：" << std::boolalpha << (missing != v.end())
              << "（若不檢查就算距離，會得到 " << std::distance(v.begin(), missing)
              << " ＝ size()，看起來像合法索引但越界）" << std::endl;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 167. Two Sum II - Input Array Is Sorted
//   題目：在遞增排序的陣列中找兩個數，使其和等於 target，
//         回傳這兩個數的「1-based」索引。保證恰有一組解。
//   為什麼用到本主題：解法是左右雙指標往中間夾擠，但題目要的是「索引」，
//         而我們手上只有迭代器 —— 這正是 std::distance 的標準用途：
//         把位置換算成 0-based 索引，再 +1 轉成題目要求的 1-based。
//   複雜度：時間 O(n)、空間 O(1)。
// -----------------------------------------------------------------------------
std::vector<int> twoSum(const std::vector<int>& numbers, int target) {
    auto lo = numbers.cbegin();
    auto hi = numbers.cend() - 1;          // 指向最後一個元素

    while (lo < hi) {
        int sum = *lo + *hi;
        if (sum == target) {
            // 迭代器 → 0-based 索引 → 題目要的 1-based
            int i = static_cast<int>(std::distance(numbers.cbegin(), lo)) + 1;
            int j = static_cast<int>(std::distance(numbers.cbegin(), hi)) + 1;
            return {i, j};
        } else if (sum < target) {
            ++lo;                          // 和太小 → 左邊往右移
        } else {
            --hi;                          // 和太大 → 右邊往左移
        }
    }
    return {};
}

// -----------------------------------------------------------------------------
// 【日常實務範例】感測器監控：回報第一筆超標樣本是「第幾筆」
//   情境：溫控系統每秒取樣一次，一旦偵測到超過警戒溫度就要發警報，
//         而警報訊息必須寫清楚「第幾秒（第幾筆樣本）開始異常」，
//         值班人員才能回頭對照當時的操作紀錄。
//   為什麼用到本主題：std::find_if 只會回傳迭代器，
//         要轉成人類看得懂的樣本序號，就得用 std::distance 換算，
//         而且務必先確認不是 end()，否則會回報一個不存在的樣本編號。
// -----------------------------------------------------------------------------
struct AlarmResult {
    bool triggered;
    std::size_t sample_index;   // 0-based
    double value;
};

AlarmResult firstOverThreshold(const std::vector<double>& temps, double limit) {
    auto it = std::find_if(temps.cbegin(), temps.cend(),
                           [limit](double t) { return t > limit; });
    if (it == temps.cend()) {
        return {false, 0, 0.0};                       // 沒有超標，不可算距離當結果
    }
    std::size_t idx = static_cast<std::size_t>(std::distance(temps.cbegin(), it));
    return {true, idx, *it};
}

int main() {
    std::cout << "=== std::distance 基本用法 ===" << std::endl;
    demo_distance();

    std::cout << "\n=== LeetCode 167. Two Sum II - Input Array Is Sorted ===" << std::endl;
    std::vector<int> numbers = {2, 7, 11, 15};
    std::vector<int> ans = twoSum(numbers, 9);
    std::cout << "target=9  → [" << ans[0] << ", " << ans[1] << "]" << std::endl;
    std::vector<int> n2 = {1, 3, 4, 5, 7, 11};
    std::vector<int> a2 = twoSum(n2, 15);
    std::cout << "target=15 → [" << a2[0] << ", " << a2[1] << "]" << std::endl;

    std::cout << "\n=== 日常實務：第一筆超標樣本 ===" << std::endl;
    std::vector<double> temps = {68.2, 69.0, 70.5, 71.8, 76.4, 77.1, 72.0};
    AlarmResult r = firstOverThreshold(temps, 75.0);
    if (r.triggered) {
        std::cout << "警報：第 " << r.sample_index << " 筆樣本（第 "
                  << r.sample_index + 1 << " 秒）溫度 " << r.value << " 超過 75.0" << std::endl;
    }
    AlarmResult r2 = firstOverThreshold(temps, 90.0);
    std::cout << "門檻 90.0 是否觸發：" << std::boolalpha << r2.triggered << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 16 課：vector 的迭代器操作10.cpp" -o iter10

// === 預期輸出 ===
// === std::distance 基本用法 ===
// it2 - it1 = 3
// distance = 3
// distance(it2, it1) = -3（有號，負值正常）
// 40 的索引 = 3
// 99 是否存在：false（若不檢查就算距離，會得到 5 ＝ size()，看起來像合法索引但越界）
//
// === LeetCode 167. Two Sum II - Input Array Is Sorted ===
// target=9  → [1, 2]
// target=15 → [3, 6]
//
// === 日常實務：第一筆超標樣本 ===
// 警報：第 4 筆樣本（第 5 秒）溫度 76.4 超過 75.0
// 門檻 90.0 是否觸發：false
