// =============================================================================
//  第 16 課：vector 的迭代器操作 6  —  隨機存取迭代器與它解鎖的能力
// =============================================================================
//
// 【主題資訊 Information】
//   vector 的迭代器類別：std::random_access_iterator_tag（本機實測）
//   支援運算：*it、it->m、++/--、it+n、it-n、it+=n、it-=n、it[n]、
//             it1-it2、== != < > <= >=
//   標頭檔：<vector>；查詢類別要 <iterator>
//   複雜度：以上全部 O(1)
//   標準版本：類別標籤是 C++98；C++20 另外定義了 contiguous_iterator 概念
//
// 【詳細解釋 Explanation】
//
// 【1. 迭代器有五個等級，能力由弱到強層層疊加】
//   STL 把迭代器分級，每一級都是前一級的超集合：
//     input        只能往前走一次、唯讀（例：istream_iterator）
//     output       只能往前走一次、唯寫（例：back_insert_iterator）
//     forward      可多次走訪、可讀寫（例：forward_list）
//     bidirectional  再加 --（例：list、set、map）
//     random access  再加 +n、-n、[]、大小比較（例：vector、deque、原生指標）
//   分級的目的不是分類癖，而是「讓演算法能在編譯期挑選最佳實作」，
//   同時讓「這個演算法需要什麼能力」變成型別系統可檢查的契約。
//
// 【2. 隨機存取解鎖了什麼 —— 這才是重點】
//   (a) std::sort 只接受隨機存取迭代器。introsort 需要「任意跳到中位數位置」，
//       這對只能一步一步走的迭代器代價太高。所以 std::list 無法用 std::sort，
//       它改為提供成員函式 list::sort（走 merge sort，靠接指標而非搬資料）。
//       本機實測：對 list 呼叫 std::sort 會編譯失敗。
//   (b) 二分搜尋才有意義。std::lower_bound 對任何 forward 以上的迭代器都能用，
//       但只有隨機存取時「跳到中點」是 O(1)，整體才是真正的 O(log n)；
//       對 list 雖然比較次數仍是 O(log n)，走訪成本卻讓總體退化成 O(n)。
//   (c) std::distance 是 O(1)（直接相減）而不是 O(n)（逐步 ++）。
//   (d) 可以做「跨步走訪」：it += stride，用於取樣、分塊、平行切分。
//
// 【3. 編譯期的 tag dispatch：同一個函式名，不同的實作】
//   std::distance 的實作大致是：
//       template<class It>
//       auto distance(It first, It last) {
//           return __distance(first, last,
//                             typename iterator_traits<It>::iterator_category());
//       }
//   然後對 random_access_iterator_tag 與 input_iterator_tag 各寫一個多載，
//   靠「標籤型別」在編譯期選擇 O(1) 相減或 O(n) 迴圈。
//   這是零成本抽象的另一個典型手法 —— 分派發生在編譯期，執行期沒有 if。
//
// 【概念補充 Concept Deep Dive】
//   C++20 又加了一級：contiguous iterator（元素在記憶體中連續，
//   保證 &*(it + n) == &*it + n），vector 與 array 屬於此級，deque 不是
//   （deque 是分段連續，可隨機存取但不連續）。這一級讓演算法能安全地
//   退化成 memcpy/SIMD。
//   有趣的實測細節：在本機 g++ 15.2.0 以 -std=c++20 編譯時，
//       iterator_traits<vector<int>::iterator>::iterator_category
//   仍然是 random_access_iterator_tag（不是 contiguous_iterator_tag），
//   這是為了不破壞既有的 tag dispatch 程式碼；C++20 改用另一個成員
//   iterator_concept 來表達連續性，而概念 std::contiguous_iterator
//   對 vector 的迭代器求值為 true（本機實測）。
//   也就是說：「舊的 category 保持相容，新的能力用 concept 表達」。
//
// 【注意事項 Pay Attention】
//   1. it + n 若跨出 [begin, end] 範圍是 UB —— 注意這裡上界是 end() 本身，
//      end() 這個位置可以被算出來，只是不能解參考。
//   2. 比較兩個「來自不同容器」的迭代器（< 或 -）是 UB，即使看起來能跑。
//   3. it1 - it2 的型別是 difference_type（有號，本機為 8 位元組的 ptrdiff_t），
//      不要存進 unsigned，否則負距離會回繞成天文數字。
//   4. it[n] 是 *(it + n) 的語法糖，一樣不做邊界檢查。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】迭代器類別
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 std::sort 不能對 std::list 使用？
//     答：std::sort 要求隨機存取迭代器（introsort 需要 O(1) 跳到任意位置取
//         pivot、做分割），而 list 只提供雙向迭代器，本機實測會編譯失敗。
//         list 因此自備成員函式 sort()，用 merge sort 重接節點指標，
//         不搬移元素、也不需要隨機存取。
//     追問：那 std::find 為什麼 list 可以用？
//         → find 只需要 input iterator（往前走 + 比較），list 完全滿足。
//
// 🔥 Q2. std::distance 對 vector 和對 list 的複雜度一樣嗎？
//     答：不一樣。它用 tag dispatch 在編譯期分派：對 random_access_iterator_tag
//         直接回傳 last - first，O(1)；對其他類別則逐步 ++ 計數，O(n)。
//         同一個函式名，兩種實作，選擇發生在編譯期而非執行期。
//
// ⚠️ 陷阱. auto d = it2 - it1; 如果 it2 在 it1 前面，d 會是什麼？寫成 size_t 有什麼問題？
//     答：d 的正確型別是 difference_type（有號的 ptrdiff_t，本機 8 位元組），
//         此時會得到負數，這是正常且有意義的。但若寫成
//         std::size_t d = it2 - it1;  負值會回繞成極大的正數，
//         後續拿它當長度或迴圈上界就會嚴重越界。
//     為什麼會錯：多數人反射性地覺得「距離不會是負的，用 size_t 剛好」，
//         但迭代器相減是有方向的向量差，不是絕對距離。要絕對值請自己取 abs，
//         或直接用 auto 讓型別正確。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <iterator>
#include <algorithm>
#include <string>
#include <type_traits>

void demo_random_access() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    auto it = v.begin();

    // 1. 解參考
    std::cout << "*it = " << *it << std::endl;           // 10

    // 2. 遞增 / 遞減
    ++it;
    std::cout << "++it: " << *it << std::endl;           // 20
    --it;
    std::cout << "--it: " << *it << std::endl;           // 10

    // 3. 加減整數（隨機跳躍，O(1)）
    it = it + 3;
    std::cout << "it + 3: " << *it << std::endl;         // 40
    it = it - 2;
    std::cout << "it - 2: " << *it << std::endl;         // 20

    // 4. 下標運算（等同 *(it + 2)，一樣不做邊界檢查）
    std::cout << "it[2] = " << it[2] << std::endl;       // 40

    // 5. 迭代器相減（回傳有號的 difference_type）
    auto it1 = v.begin();
    auto it2 = v.end();
    std::cout << "距離 = " << (it2 - it1) << std::endl;  // 5
    std::cout << "反過來相減 = " << (it1 - it2) << "（有號，負值是正常的）" << std::endl;

    // 6. 比較運算
    std::cout << std::boolalpha;
    std::cout << "(begin < end) = " << (v.begin() < v.end()) << std::endl;  // true

    // 7. 編譯期查詢迭代器類別
    using IC = std::iterator_traits<std::vector<int>::iterator>::iterator_category;
    std::cout << "vector 的迭代器是 random access？ "
              << std::is_same<IC, std::random_access_iterator_tag>::value << std::endl;
    std::cout << "difference_type 位元組數 = "
              << sizeof(std::vector<int>::difference_type) << "（有號 ptrdiff_t）" << std::endl;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 704. Binary Search
//   題目：在已排序且無重複的陣列中找 target，回傳索引；不存在回傳 -1。
//   為什麼用到本主題：二分搜尋的核心是「一步跳到中點」mid = lo + (hi - lo)/2，
//         這正是隨機存取迭代器獨有的能力（it + n 為 O(1)）。
//         同一份程式碼若把容器換成 list，因為 it + n 不存在，根本編譯不過 ——
//         這就是迭代器分級要解決的問題。
//   複雜度：時間 O(log n)、空間 O(1)。
//   註：mid 刻意寫成 lo + (hi - lo) / 2 而非 (lo + hi) / 2 —— 迭代器根本不能相加，
//       這個寫法在指標/迭代器世界是唯一合法的，順帶也避開了整數溢位。
// -----------------------------------------------------------------------------
int binarySearch(const std::vector<int>& nums, int target) {
    auto lo = nums.cbegin();
    auto hi = nums.cend();                       // 半開區間 [lo, hi)

    while (lo != hi) {
        auto mid = lo + (hi - lo) / 2;           // O(1) 跳躍，只有隨機存取做得到
        if (*mid == target) {
            return static_cast<int>(mid - nums.cbegin());
        } else if (*mid < target) {
            lo = mid + 1;                        // 收縮成 [mid+1, hi)
        } else {
            hi = mid;                            // 收縮成 [lo, mid)
        }
    }
    return -1;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】感測器資料降取樣（downsampling）：每 N 筆取一筆
//   情境：感測器以 100 Hz 取樣，但前端圖表只需要 10 Hz。直接畫十萬個點又慢又醜，
//         標準做法是每 10 筆取 1 筆。
//   為什麼用到本主題：it += stride 一次跨多步是隨機存取迭代器的能力；
//         迴圈條件必須用「剩餘距離」判斷，不能寫 it != end，
//         因為跨步走訪可能一次跳過 end() 造成越界（見【注意事項】）。
// -----------------------------------------------------------------------------
std::vector<double> downsample(const std::vector<double>& src, std::ptrdiff_t stride) {
    std::vector<double> out;
    if (stride <= 0 || src.empty()) return out;

    // 用 < 而非 != ：跨步前進可能一次越過終點
    for (auto it = src.cbegin(); it < src.cend(); it += stride) {
        out.push_back(*it);
    }
    return out;
}

int main() {
    std::cout << "=== 隨機存取迭代器的完整運算 ===" << std::endl;
    demo_random_access();

    std::cout << "\n=== LeetCode 704. Binary Search ===" << std::endl;
    std::vector<int> nums = {-1, 0, 3, 5, 9, 12};
    std::cout << "找 9  → 索引 " << binarySearch(nums, 9)  << std::endl;
    std::cout << "找 2  → 索引 " << binarySearch(nums, 2)  << std::endl;
    std::cout << "找 -1 → 索引 " << binarySearch(nums, -1) << std::endl;

    std::cout << "\n=== 日常實務：感測器降取樣（每 3 筆取 1 筆）===" << std::endl;
    std::vector<double> signal = {1.0, 1.2, 1.1, 2.5, 2.6, 2.4, 3.9, 4.1, 4.0, 5.2};
    std::vector<double> ds = downsample(signal, 3);
    std::cout << "原始 " << signal.size() << " 筆 → 降取樣後 " << ds.size() << " 筆：";
    for (double d : ds) std::cout << d << " ";
    std::cout << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 16 課：vector 的迭代器操作6.cpp" -o iter6
//
// ── 輸出但書 ────────────────────────────────────────────────────────────
// difference_type 為 8 bytes 是本機 x86-64 Linux（LP64，ptrdiff_t 為 64-bit）
// 的結果，屬實作定義；在 32-bit 平台上會是 4。標準只保證它是有號整數型別。

// === 預期輸出 ===
// === 隨機存取迭代器的完整運算 ===
// *it = 10
// ++it: 20
// --it: 10
// it + 3: 40
// it - 2: 20
// it[2] = 40
// 距離 = 5
// 反過來相減 = -5（有號，負值是正常的）
// (begin < end) = true
// vector 的迭代器是 random access？ true
// difference_type 位元組數 = 8（有號 ptrdiff_t）
//
// === LeetCode 704. Binary Search ===
// 找 9  → 索引 4
// 找 2  → 索引 -1
// 找 -1 → 索引 0
//
// === 日常實務：感測器降取樣（每 3 筆取 1 筆）===
// 原始 10 筆 → 降取樣後 4 筆：1 2.5 3.9 5.2 
