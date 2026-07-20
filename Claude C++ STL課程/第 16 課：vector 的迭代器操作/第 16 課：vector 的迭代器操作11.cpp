// =============================================================================
//  第 16 課-11：std::advance —— 「原地」移動迭代器的通用工具
// =============================================================================
//
// 【主題資訊 Information】
//   template<class InputIt, class Distance>
//   constexpr void std::advance(InputIt& it, Distance n);   // constexpr 自 C++17
//   回傳型別：void（它修改的是傳進去的迭代器本身）
//   標準版本：C++98 起；C++17 加上 constexpr
//   複雜度：random access iterator → O(1)；其他 → O(|n|)
//   標頭檔：<iterator>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼需要 advance —— 泛型程式碼不能寫 it + n】
//   vector 的迭代器支援 it + 3，但 list 的迭代器不支援。
//   如果你寫的是模板函式，事先不知道拿到的是哪種容器的迭代器，
//   直接寫 it + n 會在 list 上編譯失敗。
//   std::advance 存在的意義就是提供一個「所有迭代器類別都能用」的介面：
//   對能跳的就跳、不能跳的就一步一步走。
//   換句話說，advance 是為了「可移植性」而不是為了「方便」。
//
// 【2. 它如何依迭代器類別選擇實作 —— tag dispatch】
//   標準庫用 iterator_traits<It>::iterator_category 取出迭代器類別標籤，
//   再用重載決議挑選對應版本（這個技巧叫 tag dispatch）：
//     random_access_iterator_tag → it += n;              // O(1)，一步到位
//     bidirectional_iterator_tag → n>0 時 ++ n 次，n<0 時 -- |n| 次
//     input_iterator_tag         → 只能 ++ n 次，n 為負數是 UB
//   所以同一行 std::advance(it, 5)，在 vector 上是一次指標加法，
//   在 list 上是走五次指標鏈——寫法一樣，成本天差地遠。
//
// 【3. advance 為什麼回傳 void】
//   這是 C++98 時代的設計：它就地修改迭代器，語意上是「把 it 移動 n 步」。
//   缺點是不能寫成一行運算式（沒有回傳值可以接），
//   所以 C++11 才補上 std::next / std::prev 這兩個「回傳新迭代器」的版本。
//   兩者的分工是：要改變原迭代器用 advance，要保留原迭代器用 next/prev。
//
// 【4. 負數步數的前提條件】
//   n 為負數時，迭代器至少要是 bidirectional（雙向）。
//   vector 是 random access，當然可以；forward_list 是 forward iterator，
//   傳負數是 undefined behavior。
//   本檔用 vector 示範，advance(it, -1) 是合法且 O(1) 的。
//
// 【5. 越界是呼叫端的責任】
//   advance 不做任何範圍檢查。把迭代器移出 [begin, end] 之外
//   （注意 end 本身是合法的「位置」，但不可解參考）就是 UB。
//   標準庫的立場一貫如此：不為你沒要求的檢查付出執行期成本。
//
// 【概念補充 Concept Deep Dive】
//   ▸ 五種迭代器類別由弱到強
//     input（單次讀取、只能前進）
//       → forward（可重複走訪）
//         → bidirectional（可 --，list/set/map）
//           → random access（可 +n、-n、[]、比大小，vector/deque/array）
//             → contiguous（C++20 新增，保證記憶體連續，vector/array/string）
//     每一級都包含前一級的能力。advance / next / prev / distance
//     這組工具的成本，完全取決於你手上的迭代器落在哪一級。
//   ▸ 為什麼 vector 的 advance 是 O(1)
//     vector 的迭代器實質上就是一根指標（或包著指標的薄封裝），
//     it += n 就是一次位址算術：new_addr = old_addr + n * sizeof(T)。
//     CPU 一道指令就能算完，和 n 多大完全無關。
//   ▸ 和 distance 的對稱關係
//     distance(a, b) 回答「從 a 走到 b 要幾步」，advance(it, n) 執行
//     「從 it 走 n 步」。兩者的複雜度規則一模一樣：
//     random access 為 O(1)，其餘為 O(n)。
//
// 【注意事項 Pay Attention】
//   1. advance 沒有回傳值，寫 auto x = std::advance(it, 3); 會編譯失敗。
//   2. 它會修改傳入的迭代器本身；要保留原值請改用 std::next。
//   3. 負數 n 需要 bidirectional 以上的迭代器；forward iterator 傳負數是 UB。
//   4. 不做邊界檢查，移出容器範圍是 UB（end() 本身是合法位置，但不可解參考）。
//   5. 在 list/map 上呼叫是 O(n)，寫在迴圈裡很容易不小心變成 O(n²)。
//   6. n 的型別是 Distance，對 vector 建議用 std::ptrdiff_t（有號）而非 size_t。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::advance
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 已經有 it + n 了，為什麼還需要 std::advance？
//     答：it + n 只有 random access iterator 才支援。
//         在泛型程式碼中你可能拿到 list 或 set 的 bidirectional iterator，
//         那時 it + n 直接編譯失敗。std::advance 用 tag dispatch
//         依迭代器類別選擇實作：能跳的一步到位，不能跳的逐步 ++。
//         它換來的是可移植性，不是效能。
//     追問：那 advance 在 list 上的複雜度是多少？
//         → O(|n|)，因為只能沿著指標鏈一個一個走。
//           寫在迴圈裡就會變成 O(n²)，這是很常見的效能地雷。
//
// 🔥 Q2. std::advance 和 std::next 差在哪？什麼時候該用哪一個？
//     答：advance 就地修改傳入的迭代器、回傳 void；
//         next 回傳一個新迭代器、原迭代器不動（C++11 才加入）。
//         要往前推進手上這個迭代器用 advance；
//         要在不破壞原迭代器的前提下取得「後面第 n 個位置」用 next。
//     追問：next 內部是怎麼實作的？
//         → 複製一份迭代器，對副本呼叫 advance，再回傳副本。
//           所以 next 的複雜度和 advance 完全相同。
//
// ⚠️ 陷阱. auto it2 = std::advance(it, 3); 為什麼編譯不過？
//     答：因為 std::advance 的回傳型別是 void，不能拿來初始化變數。
//         正確寫法是 std::advance(it, 3); 然後直接用 it；
//         或者改用 auto it2 = std::next(it, 3);
//     為什麼會錯：把 advance 想成了「回傳移動後結果」的純函式。
//         它其實是 C++98 時代的「命令式」設計——動作與取值分離，
//         advance 只負責動作。next/prev 是 C++11 才補上的取值版本。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>  // std::min
#include <iostream>
#include <iterator>   // std::advance, std::distance, std::next
#include <list>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【日常實務範例】查詢結果分頁（pagination）
//   情境：搜尋結果共 N 筆，前端要第 page 頁、每頁 pageSize 筆。
//   為什麼用本主題：跳到第 page 頁的起點，就是把迭代器 advance
//         (page-1)*pageSize 步；而且必須小心「最後一頁不足一頁」的邊界，
//         不能盲目 advance 導致越界 UB。
//   這個函式刻意寫成模板，示範 advance 的真正價值——
//   同一份程式碼對 vector（O(1) 跳躍）與 list（O(n) 逐步）都能運作。
// -----------------------------------------------------------------------------
template <typename Container>
void printPage(const Container& c, std::size_t page, std::size_t pageSize) {
    const std::ptrdiff_t total = std::distance(c.begin(), c.end());
    std::ptrdiff_t offset = static_cast<std::ptrdiff_t>((page - 1) * pageSize);

    if (offset >= total) {                 // 邊界：這一頁根本不存在
        std::cout << "  第 " << page << " 頁：（無資料）" << std::endl;
        return;
    }

    auto it = c.begin();
    std::advance(it, offset);              // 跳到本頁起點

    // 本頁實際筆數＝min(pageSize, 剩餘筆數)，避免 advance 越界
    std::ptrdiff_t remain = total - offset;
    std::ptrdiff_t take = std::min(static_cast<std::ptrdiff_t>(pageSize), remain);

    std::cout << "  第 " << page << " 頁：";
    for (std::ptrdiff_t i = 0; i < take; ++i, ++it) std::cout << *it << " ";
    std::cout << std::endl;
}

int main() {
    std::cout << "=== 一、advance 的基本用法（vector，O(1)）===" << std::endl;
    std::vector<int> v = {10, 20, 30, 40, 50};

    auto it = v.begin();
    std::cout << "*it = " << *it << std::endl;

    std::advance(it, 3);          // 前進 3 步
    std::cout << "advance(it, 3): " << *it << std::endl;

    std::advance(it, -1);         // 後退 1 步（需要 bidirectional 以上）
    std::cout << "advance(it, -1): " << *it << std::endl;

    std::cout << "\n=== 二、advance 修改原迭代器，next 不會 ===" << std::endl;
    auto a = v.begin();
    auto b = v.begin();
    std::advance(a, 2);                   // a 本身被改掉
    auto bNext = std::next(b, 2);         // b 不動，另外得到一個新迭代器
    std::cout << "advance 後 *a      = " << *a     << "（a 已被移動）" << std::endl;
    std::cout << "next    後 *b      = " << *b     << "（b 原封不動）" << std::endl;
    std::cout << "next    的回傳 *bNext = " << *bNext << std::endl;

    std::cout << "\n=== 三、同一份泛型程式碼跑在 vector 與 list 上 ===" << std::endl;
    std::cout << "vector（random access，advance 為 O(1)）:" << std::endl;
    std::vector<std::string> vs = {"a", "b", "c", "d", "e", "f", "g"};
    printPage(vs, 1, 3);
    printPage(vs, 3, 3);
    printPage(vs, 4, 3);

    std::cout << "list（bidirectional，advance 為 O(n)）:" << std::endl;
    std::list<std::string> ls(vs.begin(), vs.end());
    printPage(ls, 1, 3);
    printPage(ls, 3, 3);
    printPage(ls, 4, 3);

    std::cout << "\n=== 四、advance 與 distance 的對稱性 ===" << std::endl;
    auto p = v.begin();
    std::advance(p, 4);
    std::cout << "從 begin 走 4 步後，distance(begin, p) = "
              << std::distance(v.begin(), p) << std::endl;
    std::advance(p, 1);
    std::cout << "再走 1 步，p == v.end() ? "
              << std::boolalpha << (p == v.end())
              << "（end 是合法位置，但不可解參考）" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 16 課：vector 的迭代器操作11.cpp" -o advance_demo
//
// 【本檔未附 LeetCode 範例的理由】
//   std::advance 解決的是「泛型程式碼中如何移動任意類別的迭代器」，
//   屬於介面可移植性問題。LeetCode 的題目一律給定 vector 或 string，
//   直接寫 it + n 或索引即可，硬套 advance 只會讓解法變得繞路而不自然。
//   與其湊一題不相關的，不如把版面留給上面的分頁實務範例——
//   那才是 advance 在真實程式碼中真正被用到的樣子。

// === 預期輸出 ===
// === 一、advance 的基本用法（vector，O(1)）===
// *it = 10
// advance(it, 3): 40
// advance(it, -1): 30
//
// === 二、advance 修改原迭代器，next 不會 ===
// advance 後 *a      = 30（a 已被移動）
// next    後 *b      = 10（b 原封不動）
// next    的回傳 *bNext = 30
//
// === 三、同一份泛型程式碼跑在 vector 與 list 上 ===
// vector（random access，advance 為 O(1)）:
//   第 1 頁：a b c
//   第 3 頁：g
//   第 4 頁：（無資料）
// list（bidirectional，advance 為 O(n)）:
//   第 1 頁：a b c
//   第 3 頁：g
//   第 4 頁：（無資料）
//
// === 四、advance 與 distance 的對稱性 ===
// 從 begin 走 4 步後，distance(begin, p) = 4
// 再走 1 步，p == v.end() ? true（end 是合法位置，但不可解參考）
