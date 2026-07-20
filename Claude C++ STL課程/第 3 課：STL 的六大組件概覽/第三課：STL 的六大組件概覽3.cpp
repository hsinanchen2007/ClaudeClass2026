// =============================================================================
//  第三課：STL 的六大組件概覽 3  —  容器結構決定迭代器能力（Random Access vs Bidirectional）
// =============================================================================
//
// 【主題資訊 Information】
//   概念：迭代器分五類，能力由弱到強
//     Input / Output → Forward → Bidirectional → Random Access
//   vector<T>::iterator  = Random Access Iterator（++ -- +n -n [] < 距離）
//   list<T>::iterator    = Bidirectional Iterator（只有 ++ --）
//   標準版本：五分類自 C++98 即存在；C++20 另加入 contiguous_iterator_tag
//             （vector/array/string 的迭代器同時滿足「連續」這個更強的保證）。
//   複雜度：vector 迭代器 +n 是 O(1)；list 想前進 n 步只能 ++ n 次，O(n)。
//   標頭檔：<vector>、<list>、<iterator>（std::advance / std::next / std::distance）
//
// 【詳細解釋 Explanation】
//
// 【1. 迭代器能力不是「設計選擇」，是資料結構的必然結果】
//   vector 的元素放在一塊連續記憶體，所以「第 i 個元素」的位址可以直接算出來：
//       base + i * sizeof(T)
//   一次乘加就到，因此 it + 3、it[2] 都是 O(1) —— 支援它們沒有任何代價。
//
//   list 是雙向鏈結串列，每個節點各自 new 出來、散落在堆積各處，
//   節點之間只靠 prev/next 指標相連。要找「往後第 3 個」，除了老老實實走
//   node->next->next->next 之外沒有捷徑。
//   如果 STL 硬要提供 lit + 3，那就是一個看起來 O(1) 實際上 O(n) 的介面 ——
//   使用者會在迴圈裡寫出 O(n²) 卻毫無察覺。
//
//   所以 STL 的決定是：**不提供做不到的操作，讓它變成編譯錯誤**。
//   這是 STL 設計中很重要的一條原則：介面的複雜度保證必須是誠實的。
//
// 【2. 「編譯錯誤」比「跑得慢」好】
//   lit + 3 在 list 上是編譯錯誤，不是執行期慢。這代表：
//     - 效能問題在編譯期就被抓出來，而不是上線後才發現
//     - 換容器時編譯器會直接告訴你哪些程式碼不再成立
//   同理 std::sort(lst.begin(), lst.end()) 也編不過（sort 需要 Random Access），
//   而 list 提供 lst.sort() 成員函式（歸併排序，專為鏈結串列設計）。
//
// 【3. 需要「前進 n 步」但又要泛型時：std::advance / std::next】
//   直接寫 it + n 會把程式碼綁死在 Random Access 容器上。改用：
//       std::advance(it, n);        // 就地移動；回傳 void
//       auto it2 = std::next(it, n); // 回傳新迭代器，原 it 不動（C++11）
//   這兩個函式會用 iterator_traits 查出迭代器類別，在編譯期分派到最佳實作：
//     Random Access → it += n     （O(1)）
//     Bidirectional → 迴圈 ++/--  （O(n)，支援負的 n）
//     Forward       → 迴圈 ++     （O(n)，n 必須非負）
//   複雜度仍然是 O(n)，advance 不會變魔術；它換來的是「同一份程式碼能編過所有容器」。
//
// 【概念補充 Concept Deep Dive】
//   為什麼 vector 走訪通常比 list 快很多，即使兩者都是 O(n)？
//   關鍵在快取（cache）而非大 O：
//     - 現代 CPU 一次從記憶體抓的是一整條 cache line（x86-64 上是 64 bytes）。
//       vector<int> 走訪一次 cache miss 可以帶回 16 個 int，接下來 15 次存取全部命中。
//     - list 的節點是各自配置的，位址不連續。走訪時每跳一個節點就可能是一次
//       cache miss；而且必須先讀到 next 指標才知道下一個位址，CPU 的硬體預取器
//       也難以提前抓取（pointer chasing）。
//     - list 每個節點還要多存 prev/next 兩個指標：本機 x86-64 上
//       list<int> 節點 = 8(prev) + 8(next) + 4(int) + 4(padding) = 24 bytes，
//       相對於 vector 的 4 bytes，記憶體流量是 6 倍。
//   結論：大 O 相同時，記憶體佈局往往才是決勝點。
//
// 【注意事項 Pay Attention】
//   1. list 迭代器沒有 operator+、operator[]、operator<；誤用是編譯錯誤（好事）。
//   2. std::advance / std::next 對 list 仍是 O(n)，只是介面統一，不是效能魔法。
//   3. std::distance 對 Random Access 是 O(1)，對 list 是 O(n)：
//      在迴圈裡呼叫 distance 是常見的意外 O(n²)。
//   4. vector 的 push_back 可能重新配置記憶體 → 所有迭代器/指標/參考全部失效；
//      list 的插入則不會使任何既有迭代器失效。能力強不等於比較安全。
//   5. 「Random Access」與 C++20 的「contiguous」不同：deque 是 Random Access
//      但**不是**連續記憶體，&*it 不能當成可以整段 memcpy 的陣列指標。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】迭代器能力與容器結構
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 std::list 的迭代器不支援 it + 3 和 it[2]？
//     答：list 是鏈結串列，節點位址不連續，「往後第 3 個」只能沿 next 走三次，
//         本質是 O(n)。STL 的原則是不提供「複雜度會騙人」的介面 ——
//         與其給一個 O(n) 的 operator+ 讓人誤以為 O(1)，不如讓它編譯失敗。
//     追問：那我真的需要前進 n 步怎麼辦？
//           → std::advance(it, n) 或 std::next(it, n)。它們用 iterator_traits
//             在編譯期分派：vector 走 += n，list 走迴圈 ++，複雜度誠實反映。
//
// 🔥 Q2. 為什麼 std::sort 不能用在 std::list 上？list 要怎麼排序？
//     答：std::sort 是 introsort（quicksort + heapsort + insertion sort），
//         需要隨機存取來取中位數樞紐與做分割，因此要求 Random Access Iterator。
//         list 只有 Bidirectional，故編譯錯誤。list 提供成員函式 lst.sort()，
//         用歸併排序（merge sort）實作 —— 只需重接指標，不必移動元素。
//     追問：list::sort 為什麼不用 quicksort？
//           → quicksort 依賴隨機存取做分割；而歸併排序對鏈結串列反而更自然，
//             且不需要額外陣列空間（只重接 next/prev），還能保證穩定與 O(n log n)。
//
// ⚠️ 陷阱. 「list 的插入是 O(1)，所以在 list 中間插入一定比 vector 快」——哪裡不對？
//     答：list 的插入本身確實是 O(1)，但前提是**你已經握有那個位置的迭代器**。
//         如果你得先走到中間（std::advance(it, n/2)），那一步就是 O(n)，
//         而且是 cache 極不友善的 pointer chasing。vector 的 insert 雖是 O(n)，
//         但它做的是一段連續記憶體的 memmove —— 硬體跑得極快。
//         實測上，元素數量在數千以內、或元素是小型 POD 時，vector 經常仍然勝出。
//     為什麼會錯：把大 O 當成效能的全部，忽略了常數項與記憶體階層。
//         O(1) 的 pointer chasing 可能比 O(n) 的 memmove 還慢。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <iterator>   // std::advance, std::next, std::distance
#include <algorithm>  // std::sort, std::unique

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 26. Remove Duplicates from Sorted Array
//   題目：對已排序陣列就地去除重複元素，回傳新長度 k，且前 k 個位置為去重後結果。
//   為什麼用到本主題：這題的標準解法是「快慢雙指標」——而雙指標之所以成立，
//                     正是因為 vector 的迭代器是 Random Access：
//                     可以用 nums[slow] 直接定位、可以用 (it - begin) 算索引。
//                     若容器換成 list，這個解法一行都不成立（沒有 [] 也沒有 -），
//                     必須改用 lst.unique()。這正是「容器結構決定演算法選擇」。
//   複雜度：時間 O(N)、空間 O(1)。
// -----------------------------------------------------------------------------
int removeDuplicates(std::vector<int>& nums) {
    if (nums.empty()) return 0;
    std::size_t slow = 0;                          // 慢指標：已確定去重的最後位置
    for (std::size_t fast = 1; fast < nums.size(); ++fast) {
        if (nums[fast] != nums[slow]) {            // [] 存取 = Random Access 才有
            ++slow;
            nums[slow] = nums[fast];
        }
    }
    return static_cast<int>(slow + 1);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】播放清單：取「目前歌曲的前一首 / 後一首」
//   情境：音樂播放器的播放清單需要頻繁在中間插入（加入下一首播放）與刪除，
//         而且使用者按「上一首」時要能往回走 —— 這正是 list 的典型場景。
//   為什麼用到本主題：Bidirectional Iterator 的 ++/-- 剛好對應「下一首/上一首」，
//                     且 list 插入刪除不會使其他迭代器失效，
//                     「目前播放中」這個迭代器可以安全長期持有。
// -----------------------------------------------------------------------------
void showNeighbors(const std::list<std::string>& playlist,
                   std::list<std::string>::const_iterator current) {
    std::cout << "  上一首: "
              << (current == playlist.begin() ? std::string("(已是第一首)")
                                              : *std::prev(current))
              << std::endl;
    std::cout << "  播放中: " << *current << std::endl;
    auto nxt = std::next(current);
    std::cout << "  下一首: "
              << (nxt == playlist.end() ? std::string("(已是最後一首)") : *nxt)
              << std::endl;
}

int main() {
    // vector 有 Random Access Iterator
    std::vector<int> vec = {10, 20, 30, 40, 50};
    auto vit = vec.begin();

    std::cout << "vector 迭代器可以：" << std::endl;
    std::cout << "  vit[2] = " << vit[2] << std::endl;      // 隨機存取
    std::cout << "  *(vit + 3) = " << *(vit + 3) << std::endl;  // 算術運算

    // list 只有 Bidirectional Iterator
    std::list<int> lst = {10, 20, 30, 40, 50};
    auto lit = lst.begin();

    std::cout << "\nlist 迭代器可以：" << std::endl;
    ++lit;  // 前進
    std::cout << "  ++lit → *lit = " << *lit << std::endl;
    --lit;  // 後退
    std::cout << "  --lit → *lit = " << *lit << std::endl;

    // 但 list 迭代器不能：
    // lit[2];      // 編譯錯誤！
    // lit + 3;     // 編譯錯誤！

    // 泛型寫法：advance / next 兩種容器都適用（但複雜度不同）
    std::cout << "\n=== std::advance / std::next（泛型前進）===" << std::endl;
    auto v2 = vec.begin();
    std::advance(v2, 3);                 // vector: 內部 += 3，O(1)
    std::cout << "  vector advance(3) → " << *v2 << "  (O(1))" << std::endl;

    auto l2 = lst.begin();
    std::advance(l2, 3);                 // list: 內部 ++ 三次，O(n)
    std::cout << "  list   advance(3) → " << *l2 << "  (O(n))" << std::endl;

    std::cout << "  distance(begin,end) vector = "
              << std::distance(vec.begin(), vec.end()) << "  (O(1))" << std::endl;
    std::cout << "  distance(begin,end) list   = "
              << std::distance(lst.begin(), lst.end()) << "  (O(n))" << std::endl;

    // 排序：sort 需要 Random Access，list 只能用成員函式
    std::cout << "\n=== 排序的分工 ===" << std::endl;
    std::vector<int> v3 = {5, 3, 1, 4, 2};
    std::sort(v3.begin(), v3.end());                  // OK：Random Access
    std::cout << "  std::sort(vector): ";
    for (int n : v3) std::cout << n << " ";
    std::cout << std::endl;

    std::list<int> l3 = {5, 3, 1, 4, 2};
    // std::sort(l3.begin(), l3.end());               // 編譯錯誤！只有 Bidirectional
    l3.sort();                                        // list 自己的歸併排序
    std::cout << "  list::sort(list)  : ";
    for (int n : l3) std::cout << n << " ";
    std::cout << std::endl;

    // 節點大小對比（實作定義，見檔尾說明）
    std::cout << "\n=== 記憶體佈局差異（實作定義）===" << std::endl;
    std::cout << "  vector<int> 每元素          = " << sizeof(int) << " bytes（連續）" << std::endl;
    std::cout << "  list<int>   每節點（估算）  = "
              << sizeof(void*) * 2 + sizeof(int) << " bytes + padding → 24 bytes（分散）"
              << std::endl;

    std::cout << "\n=== LeetCode 26. Remove Duplicates from Sorted Array ===" << std::endl;
    std::vector<int> nums = {0, 0, 1, 1, 1, 2, 2, 3, 3, 4};
    int k = removeDuplicates(nums);
    std::cout << "輸入 : 0 0 1 1 1 2 2 3 3 4" << std::endl;
    std::cout << "回傳 k = " << k << "，前 k 個元素 = ";
    for (int i = 0; i < k; ++i) std::cout << nums[i] << " ";
    std::cout << std::endl;
    std::cout << "（同樣需求在 list 上要改寫成 lst.unique()，因為沒有 [] 可用）" << std::endl;

    std::cout << "\n=== 日常實務：播放清單的上一首／下一首 ===" << std::endl;
    std::list<std::string> playlist = {"Intro", "Sunrise", "Nocturne", "Outro"};
    auto current = std::next(playlist.begin(), 2);   // 播放中：Nocturne
    showNeighbors(playlist, current);

    // list 的關鍵優勢：插入不會使 current 失效
    playlist.insert(current, "Interlude");           // 在 Nocturne 前插入
    std::cout << "  -- 在播放中那首前面插入一首之後 --" << std::endl;
    showNeighbors(playlist, current);                // current 仍然有效！

    return 0;
}

// 注意：list<int> 節點大小 24 bytes 是本機 g++ 15.2 / libstdc++ / x86-64 的實測值，
//       由 prev(8) + next(8) + int(4) + padding(4) 構成。
//       節點大小與 padding 皆為實作定義，其他平台或標準函式庫可能不同。

// 編譯: g++ -std=c++17 -Wall -Wextra 第三課：STL 的六大組件概覽3.cpp -o demo3

// === 預期輸出 ===
// vector 迭代器可以：
//   vit[2] = 30
//   *(vit + 3) = 40
//
// list 迭代器可以：
//   ++lit → *lit = 20
//   --lit → *lit = 10
//
// === std::advance / std::next（泛型前進）===
//   vector advance(3) → 40  (O(1))
//   list   advance(3) → 40  (O(n))
//   distance(begin,end) vector = 5  (O(1))
//   distance(begin,end) list   = 5  (O(n))
//
// === 排序的分工 ===
//   std::sort(vector): 1 2 3 4 5
//   list::sort(list)  : 1 2 3 4 5
//
// === 記憶體佈局差異（實作定義）===
//   vector<int> 每元素          = 4 bytes（連續）
//   list<int>   每節點（估算）  = 20 bytes + padding → 24 bytes（分散）
//
// === LeetCode 26. Remove Duplicates from Sorted Array ===
// 輸入 : 0 0 1 1 1 2 2 3 3 4
// 回傳 k = 5，前 k 個元素 = 0 1 2 3 4
// （同樣需求在 list 上要改寫成 lst.unique()，因為沒有 [] 可用）
//
// === 日常實務：播放清單的上一首／下一首 ===
//   上一首: Sunrise
//   播放中: Nocturne
//   下一首: Outro
//   -- 在播放中那首前面插入一首之後 --
//   上一首: Interlude
//   播放中: Nocturne
//   下一首: Outro
