// =============================================================================
//  第五課：迭代器的五種分類 9  —  advance / distance / next / prev 與編譯期分派
// =============================================================================
//
// 【主題資訊 Information】
//   template<class It, class Distance> void advance(It& it, Distance n);      // C++98
//   template<class It> typename iterator_traits<It>::difference_type
//                      distance(It first, It last);                           // C++98
//   template<class It> It next(It it, difference_type n = 1);                 // C++11
//   template<class It> It prev(It it, difference_type n = 1);                 // C++11
//   標頭檔：<iterator>
//   複雜度：
//       Random Access → O(1)
//       其他類別      → O(n)
//   C++17 起這四個都是 constexpr；C++20 另有 std::ranges:: 版本。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼需要 advance：寫出「對所有容器都正確」的泛型程式碼】
//   如果你的函式只吃 vector，直接寫 it += n 就好。
//   但泛型演算法不知道拿到的是哪種迭代器：
//       template<class It> void f(It it) { it += 3; }   // list 傳進來就編譯失敗
//   advance 解決的正是這件事——它對外提供統一介面，
//   對內依迭代器類別選最快的實作：
//       Random Access → it += n            （一次算完，O(1)）
//       Bidirectional → n>0 就 ++，n<0 就 --（O(|n|)）
//       Input/Forward → 只能 ++，n 不可為負（O(n)）
//   所以 advance 是「泛型程式碼的正確寫法」，不是「方便的糖」。
//
// 【2. 這個選擇發生在編譯期，執行期零成本】
//   傳統實作用的是 **tag dispatch**：
//       template<class It, class D>
//       void advance_impl(It& it, D n, std::random_access_iterator_tag) { it += n; }
//       template<class It, class D>
//       void advance_impl(It& it, D n, std::bidirectional_iterator_tag) {
//           if (n >= 0) while (n--) ++it; else while (n++) --it;
//       }
//       template<class It, class D>
//       void advance(It& it, D n) {
//           advance_impl(it, n, typename std::iterator_traits<It>::iterator_category{});
//       }
//   重點在最後那個「憑空造出一個 tag 物件」——它沒有任何資料成員，
//   純粹是拿來做**重載解析**用的型別標記。編譯完全部被最佳化掉，
//   執行期沒有任何 if / 虛擬呼叫。
//   C++17 之後也可以直接寫 if constexpr，效果相同但更好讀。
//
// 【3. advance 與 next/prev 的差別：改不改原件】
//       std::advance(it, 3);        // 原地修改 it，回傳 void
//       auto j = std::next(it, 3);  // it 不動，回傳新的迭代器
//   選哪個看語意：迴圈裡要推進遊標用 advance，
//   要「看一下前面第幾個」而不想弄髒目前位置用 next。
//   next/prev 是 C++11 才加的，在那之前只能寫
//       auto j = it; std::advance(j, 3);
//   多兩行且容易寫錯，這是 C++11 補上這兩個函式的原因。
//
// 【4. distance 的方向性與陷阱】
//   distance(first, last) 假設 **last 在 first 之後**（可從 first 走到 last）。
//   對 Random Access 它算的是 last - first，順序反了會得到負數；
//   對其他類別它只會一直 ++first 直到相等——
//   若 last 在 first 前面，就會走過頭、衝出 end()，那是未定義行為。
//   所以在非 Random Access 容器上，**參數順序寫反不是回傳負數，是 UB**。
//
// 【概念補充 Concept Deep Dive】
//
// (A) iterator_traits：讓「原生指標」也能參與這套機制
//     std::advance 需要問「你是哪一類迭代器」，但原生指標 int* 沒有
//     iterator_category 這個成員型別。iterator_traits 就是那層轉接：
//         template<class It> struct iterator_traits {
//             using iterator_category = typename It::iterator_category; ...
//         };
//         template<class T> struct iterator_traits<T*> {          // 針對指標的特化
//             using iterator_category = random_access_iterator_tag; ...
//         };
//     有了這個特化，std::advance(p, 3) 對 int* 也能用，
//     而且走的是 O(1) 分支。這是「特徵萃取（traits）」最經典的用法。
//
// (B) 為什麼 distance 對 list 是 O(n)，但 list::size() 是 O(1)
//     distance 只拿到兩個迭代器，它不知道容器是誰，只能一步步數。
//     size() 是容器的成員函式，可以直接讀內部維護的計數欄位。
//     C++11 起標準明確要求 list::size() 必須是 O(1)
//     （C++03 允許 O(n)，這是歷史上有名的爭議點）。
//     所以能用 size() 就別用 distance(begin, end)。
//
// (C) advance 的 n 可以是負的嗎？看類別
//     Random Access / Bidirectional → 可以（會用 -- 或 -=）
//     Input / Forward               → **不可以**，是未定義行為
//     next/prev 同理：prev 需要 Bidirectional，
//     對 forward_list 呼叫 std::prev 會編譯失敗。
//
// (D) C++17 起這四個函式都是 constexpr
//     可以在編譯期常數運算中使用（例如對 constexpr 的 std::array）。
//     這是 C++17 大規模「constexpr 化」標準函式庫的一部分。
//
// 【注意事項 Pay Attention】
//   1. distance(first, last) 的參數順序寫反，在非 Random Access 容器上是 UB，
//      不是「回傳負數」。
//   2. advance 走過 end() 是 UB，走過 begin() 之前同樣是 UB；
//      這兩個函式**都不做邊界檢查**。
//   3. 對 Input/Forward Iterator，advance 的 n 不可為負。
//   4. std::prev 需要 Bidirectional，forward_list 用不了。
//   5. 容器有 size() 就用 size()，別用 distance(begin(), end())——
//      對 list 是 O(n) vs O(1) 的差別。
//   6. distance 回傳的是有號的 difference_type，
//      拿去和 size()（無號）比較會觸發 -Wsign-compare。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】advance / distance 與 tag dispatch
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::advance 怎麼做到「對 vector 是 O(1)、對 list 是 O(n)」？
//        它是在執行期判斷容器種類嗎？
//     答：不是，是**編譯期**分派。傳統做法是 tag dispatch：
//         用 iterator_traits<It>::iterator_category 取出該迭代器的標記型別
//         （random_access_iterator_tag / bidirectional_iterator_tag …），
//         憑空建一個該型別的空物件當額外引數，讓**重載解析**去選對應版本。
//         標記型別沒有任何資料成員，編譯完全部消失，執行期零成本。
//         C++17 之後也可以用 if constexpr 寫得更直白。
//     追問：那原生指標 int* 沒有 iterator_category，怎麼辦？
//         → iterator_traits 對 T* 有偏特化，直接把 category 定成
//           random_access_iterator_tag。這就是 traits 這層轉接存在的理由。
//
// 🔥 Q2. 為什麼 std::distance 對 list 是 O(n)，但 lst.size() 是 O(1)？
//     答：distance 只拿到兩個迭代器，它根本不知道背後的容器是誰，
//         只能從 first 一路 ++ 數到 last。
//         size() 是容器的成員函式，可以直接讀內部維護的計數欄位。
//         C++11 起標準明確要求 list::size() 必須是 O(1)（C++03 允許 O(n)）。
//     追問：所以什麼時候該用 distance？
//         → 只有在算「兩個任意迭代器之間的距離」時。
//           算整個容器長度請一律用 size()。
//
// ⚠️ 陷阱. 「distance(first, last) 如果順序寫反，回傳負數就好了，
//         反正我用 abs() 取絕對值。」
//     答：這在 vector 上碰巧成立，但在 list 上是**未定義行為**。
//         對 Random Access，distance 算的是 last - first，順序反了得到負數，
//         沒事。對其他類別，它的實作是「一直 ++first 直到 first == last」——
//         若 last 在 first 前面，這個迴圈永遠等不到相等，
//         會一路衝過 end() 繼續解參考已經不屬於容器的記憶體。
//     為什麼會錯：以為 distance 是一個「計算兩點距離」的數學函式，
//         所以順序無關。實際上它是一個**走訪**動作，
//         方向錯了就是往懸崖走。同樣的道理，
//         advance 走過 end()、prev 走過 begin() 也都不會有任何檢查。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <list>
#include <forward_list>
#include <string>
#include <iterator>
#include <type_traits>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔刻意不放
//   理由：advance / distance / next / prev 是「泛型程式碼的正確寫法」，
//   解決的是「同一段程式碼要能跑在不同迭代器類別上」這個**函式庫作者**的問題。
//   LeetCode 的題目簽章由平台固定，輸入幾乎一律是 vector 或自訂鏈結串列節點，
//   型別完全寫死，根本不存在「不知道拿到哪種迭代器」的情境——
//   在 vector 上直接寫 nums[i] 或 it + n 更短更清楚。
//   硬掛一題只會讓讀者以為這組函式和演算法解題有關，反而模糊了它的真正用途。
//   要看它們真正發揮價值的地方，請看下面的實務範例
//   （同一個分頁函式要同時支援 vector 與 list）。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例 1】一個分頁（pagination）函式，同時支援 vector 與 list
//   情境：後端查詢結果要分頁回傳給前端。有些查詢結果放在 vector（隨機存取快），
//         有些放在 list（中途要頻繁插刪）。分頁邏輯只有一套，不想寫兩份。
//   為什麼用到本主題：函式模板拿到的迭代器類別不確定，
//     直接寫 first + offset 在 list 上會編譯失敗。
//     用 std::advance / std::next 就能同時支援兩者，
//     而且對 vector 自動走 O(1) 分支、對 list 走 O(n) 分支——
//     介面統一，效能各自最佳。
// -----------------------------------------------------------------------------
template <class It>
std::pair<It, It> pageRange(It first, It last, std::size_t pageIdx, std::size_t pageSize) {
    const auto total = std::distance(first, last);          // 有號 difference_type
    const auto skip  = static_cast<decltype(total)>(pageIdx * pageSize);
    if (skip >= total) return {last, last};                 // 超過範圍：回傳空區間

    It pageBegin = first;
    std::advance(pageBegin, skip);                          // 依類別自動選最快實作

    const auto remain = total - skip;
    const auto take   = std::min(remain, static_cast<decltype(total)>(pageSize));
    It pageEnd = pageBegin;
    std::advance(pageEnd, take);

    return {pageBegin, pageEnd};
}

template <class Container>
void printPage(const std::string& label, const Container& c,
               std::size_t pageIdx, std::size_t pageSize) {
    auto [b, e] = pageRange(c.begin(), c.end(), pageIdx, pageSize);
    std::cout << label << " 第 " << pageIdx << " 頁: ";
    if (b == e) {
        std::cout << "(空)";
    } else {
        for (auto it = b; it != e; ++it) std::cout << *it << " ";
    }
    std::cout << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】一個真實會發生的效能 bug：在迴圈裡呼叫 distance
//   情境：某段程式要為 list 裡每個元素印出「這是第幾筆」。
//         直覺寫法是在迴圈裡呼叫 std::distance(begin, it)——
//         看起來很自然，但每次都是 O(n)，整個迴圈變成 O(n²)。
//   為什麼用到本主題：這正是「知道 distance 的複雜度取決於迭代器類別」
//     才躲得掉的坑。同樣的程式碼放在 vector 上是 O(n)（完全沒事），
//     放到 list 上就從 O(n) 變成 O(n²)——而且測試資料少時看不出來。
//   下面用「計數器」而非計時來呈現，結果可重現。
// -----------------------------------------------------------------------------
struct StepCounter {
    long long steps = 0;
};

// 壞寫法：每輪都重新從頭數
long long countStepsBad(const std::list<int>& data) {
    StepCounter c;
    for (auto it = data.begin(); it != data.end(); ++it) {
        // distance 對 list 是 O(n)，每一輪都從 begin 走到 it
        auto idx = std::distance(data.begin(), it);
        c.steps += idx + 1;          // 模擬它實際走了幾步
    }
    return c.steps;
}

// 好寫法：自己維護索引，一趟就好
long long countStepsGood(const std::list<int>& data) {
    StepCounter c;
    std::size_t idx = 0;
    for (auto it = data.begin(); it != data.end(); ++it, ++idx) {
        c.steps += 1;                // 每個元素只碰一次
    }
    (void)idx;
    return c.steps;
}

int main() {
    // std::advance：移動迭代器
    std::cout << "=== std::advance ===" << std::endl;

    std::vector<int> vec = {10, 20, 30, 40, 50};
    std::list<int> lst = {10, 20, 30, 40, 50};

    auto vit = vec.begin();
    auto lit = lst.begin();

    // 對於 Random Access Iterator，advance 內部用 it += n（O(1)）
    std::advance(vit, 3);
    std::cout << "vector advance(it, 3): " << *vit << std::endl;

    // 對於 Bidirectional Iterator，advance 內部用 ++it 三次（O(n)）
    std::advance(lit, 3);
    std::cout << "list advance(it, 3): " << *lit << std::endl;

    // std::distance：計算距離
    std::cout << "\n=== std::distance ===" << std::endl;

    auto vbegin = vec.begin();
    auto vend = vec.end();
    std::cout << "vector distance: " << std::distance(vbegin, vend) << std::endl;

    auto lbegin = lst.begin();
    auto lend = lst.end();
    std::cout << "list distance: " << std::distance(lbegin, lend) << std::endl;

    // std::next 和 std::prev（C++11）：更方便的移動
    std::cout << "\n=== std::next / std::prev ===" << std::endl;

    auto it = vec.begin();
    auto next_it = std::next(it, 2);  // 前進 2 步，不修改 it
    auto prev_it = std::prev(vec.end(), 1);  // 後退 1 步

    std::cout << "*it = " << *it << std::endl;
    std::cout << "*next(it, 2) = " << *next_it << std::endl;
    std::cout << "*prev(end, 1) = " << *prev_it << std::endl;

    // -------------------------------------------------------------------------
    std::cout << "\n=== advance 改原件，next 不改 ===" << std::endl;
    {
        auto a = vec.begin();
        auto b = std::next(a, 2);
        std::cout << "std::next(a, 2) 之後，a 仍指向 " << *a
                  << "，b 指向 " << *b << std::endl;
        std::advance(a, 2);
        std::cout << "std::advance(a, 2) 之後，a 被改成指向 " << *a << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 編譯期分派：看得到迭代器的標記型別 ===" << std::endl;
    {
        using VCat = std::iterator_traits<std::vector<int>::iterator>::iterator_category;
        using LCat = std::iterator_traits<std::list<int>::iterator>::iterator_category;
        using FCat = std::iterator_traits<std::forward_list<int>::iterator>::iterator_category;
        using PCat = std::iterator_traits<int*>::iterator_category;   // 原生指標！

        std::cout << std::boolalpha;
        std::cout << "vector       是 random_access_iterator_tag ? "
                  << std::is_same<VCat, std::random_access_iterator_tag>::value << std::endl;
        std::cout << "list         是 bidirectional_iterator_tag ? "
                  << std::is_same<LCat, std::bidirectional_iterator_tag>::value << std::endl;
        std::cout << "forward_list 是 forward_iterator_tag       ? "
                  << std::is_same<FCat, std::forward_iterator_tag>::value << std::endl;
        std::cout << "int*（原生指標）是 random_access_iterator_tag ? "
                  << std::is_same<PCat, std::random_access_iterator_tag>::value << std::endl;
        std::cout << "→ 靠 iterator_traits 對 T* 的偏特化，原生指標也能用 advance"
                  << std::endl;

        int arr[] = {1, 2, 3, 4, 5};
        int* p = arr;
        std::advance(p, 3);                 // 對指標一樣可用，走 O(1) 分支
        std::cout << "std::advance(int* p, 3) -> *p = " << *p << std::endl;
        std::cout << "std::distance(arr, arr+5) = "
                  << std::distance(arr, arr + 5) << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== distance 是有號的，size() 是無號的 ===" << std::endl;
    {
        auto d = std::distance(lst.begin(), lst.end());
        std::cout << "std::distance(list) = " << d << "（有號，O(n)）" << std::endl;
        std::cout << "lst.size()          = " << lst.size()
                  << "（無號，C++11 起保證 O(1)）" << std::endl;
        std::cout << "→ 算整個容器長度請用 size()，別用 distance" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== forward_list 不能用 std::prev ===" << std::endl;
    {
        std::forward_list<int> f = {1, 2, 3};
        auto fit = f.begin();
        std::advance(fit, 2);               // 可以：只用到 ++
        std::cout << "forward_list advance(it, 2) = " << *fit << std::endl;
        // std::prev(fit);                  // 編譯錯誤：prev 需要 Bidirectional
        // std::advance(fit, -1);           // UB：Forward Iterator 的 n 不可為負
        std::cout << "std::prev(fit) 會編譯失敗；advance 傳負數是 UB" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務 1：同一個分頁函式支援 vector 與 list ===" << std::endl;
    {
        std::vector<std::string> vresults = {
            "doc-01", "doc-02", "doc-03", "doc-04",
            "doc-05", "doc-06", "doc-07",
        };
        std::list<std::string> lresults(vresults.begin(), vresults.end());

        for (std::size_t page = 0; page < 4; ++page) {
            printPage("vector", vresults, page, 3);
        }
        for (std::size_t page = 0; page < 4; ++page) {
            printPage("list  ", lresults, page, 3);
        }
        std::cout << "→ 同一份模板碼；對 vector 走 O(1) 分支、對 list 走 O(n) 分支"
                  << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務 2：在迴圈裡呼叫 distance 會變成 O(n²) ===" << std::endl;
    {
        std::list<int> data(1000, 7);
        auto bad  = countStepsBad(data);
        auto good = countStepsGood(data);
        std::cout << "1000 筆資料：" << std::endl;
        std::cout << "  迴圈內用 distance   總共走了 " << bad  << " 步（O(n²)）" << std::endl;
        std::cout << "  自己維護索引        總共走了 " << good << " 步（O(n)）" << std::endl;
        std::cout << "  倍數差距: " << (bad / good) << " 倍" << std::endl;
        std::cout << "→ 同樣的程式碼放在 vector 上完全沒事（distance 是 O(1)）；"
                  << std::endl;
        std::cout << "  換成 list 就爆炸，而且資料量小時測不出來。" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第五課：迭代器的五種分類9.cpp -o demo9

// === 預期輸出 ===
// === std::advance ===
// vector advance(it, 3): 40
// list advance(it, 3): 40
//
// === std::distance ===
// vector distance: 5
// list distance: 5
//
// === std::next / std::prev ===
// *it = 10
// *next(it, 2) = 30
// *prev(end, 1) = 50
//
// === advance 改原件，next 不改 ===
// std::next(a, 2) 之後，a 仍指向 10，b 指向 30
// std::advance(a, 2) 之後，a 被改成指向 30
//
// === 編譯期分派：看得到迭代器的標記型別 ===
// vector       是 random_access_iterator_tag ? true
// list         是 bidirectional_iterator_tag ? true
// forward_list 是 forward_iterator_tag       ? true
// int*（原生指標）是 random_access_iterator_tag ? true
// → 靠 iterator_traits 對 T* 的偏特化，原生指標也能用 advance
// std::advance(int* p, 3) -> *p = 4
// std::distance(arr, arr+5) = 5
//
// === distance 是有號的，size() 是無號的 ===
// std::distance(list) = 5（有號，O(n)）
// lst.size()          = 5（無號，C++11 起保證 O(1)）
// → 算整個容器長度請用 size()，別用 distance
//
// === forward_list 不能用 std::prev ===
// forward_list advance(it, 2) = 3
// std::prev(fit) 會編譯失敗；advance 傳負數是 UB
//
// === 日常實務 1：同一個分頁函式支援 vector 與 list ===
// vector 第 0 頁: doc-01 doc-02 doc-03
// vector 第 1 頁: doc-04 doc-05 doc-06
// vector 第 2 頁: doc-07
// vector 第 3 頁: (空)
// list   第 0 頁: doc-01 doc-02 doc-03
// list   第 1 頁: doc-04 doc-05 doc-06
// list   第 2 頁: doc-07
// list   第 3 頁: (空)
// → 同一份模板碼；對 vector 走 O(1) 分支、對 list 走 O(n) 分支
//
// === 日常實務 2：在迴圈裡呼叫 distance 會變成 O(n²) ===
// 1000 筆資料：
//   迴圈內用 distance   總共走了 500500 步（O(n²)）
//   自己維護索引        總共走了 1000 步（O(n)）
//   倍數差距: 500 倍
// → 同樣的程式碼放在 vector 上完全沒事（distance 是 O(1)）；
//   換成 list 就爆炸，而且資料量小時測不出來。
