// =============================================================================
//  第三課：STL 的六大組件概覽 5  —  正交性：同一個演算法套用在不同容器
// =============================================================================
//
// 【主題資訊 Information】
//   概念：STL 的「正交性（orthogonality）」—— 容器與演算法互不相識，
//         靠迭代器這個共同介面接合，因此 M 個容器 × N 個演算法只需 M+N 份程式碼。
//   標準版本：template 函式模板為 C++98；本檔用到的 print_container 若改寫成
//             泛型 lambda（auto 參數）則是 C++14，寫成 template 函式則 C++98 即可。
//   複雜度：std::find 對所有容器都是 O(N)；
//           std::sort 僅適用 Random Access（O(N log N)），
//           list::sort 是專為鏈結串列設計的歸併排序（O(N log N)，只重接指標）。
//   標頭檔：<vector> <list> <deque> <algorithm> <string>
//
// 【詳細解釋 Explanation】
//
// 【1. 正交性到底省下了什麼】
//   假設 STL 沒有迭代器這一層，那麼每個演算法都得為每種容器各寫一份：
//       sort_vector / sort_list / sort_deque / find_vector / find_list / ...
//   以 STL 現有規模（十幾個容器 × 上百個演算法）計算，那是上千個函式，
//   而且每加一個新容器就要補齊全部演算法。
//   有了迭代器之後：容器只負責「能產生符合某類別要求的迭代器」，
//   演算法只負責「對某類別的迭代器工作」，兩邊各自演化。
//   你自己寫的容器只要提供合格的 begin()/end()，立刻能用全部 STL 演算法 ——
//   這才是正交性真正的價值。
//
// 【2. 為什麼 std::find 三種容器都能用，std::sort 卻不行】
//   差別在演算法「要求」的迭代器類別：
//       std::find → 只需要 Input Iterator（能 *it、++it、比較）
//                   vector / list / deque 全部滿足 → 都能用
//       std::sort → 需要 Random Access Iterator（要 it+n、it-it、it[n]）
//                   vector / deque 滿足；list 只有 Bidirectional → 編譯錯誤
//   這不是 STL 偷懶，而是誠實：quicksort 必須能在 O(1) 內跳到任意位置取樞紐、
//   做分割，這在鏈結串列上做不到。
//
// 【3. 成員版 sort 與泛型 sort 的分工】
//   list 提供 lst.sort() 成員函式，用歸併排序：
//     - 不移動元素本身，只重接 prev/next 指標 → 對「移動成本高」的元素型別反而有利
//     - 迭代器/參考在排序後仍然有效（只是所在位置變了）—— 這是 std::sort 給不了的保證
//     - 天生穩定（stable）
//   一般規則：**容器有提供同名成員函式時，優先用成員版**。
//   同樣的道理也適用於 lst.remove()（O(N) 一次搞定）與
//   std::remove + erase（對 list 要搬移元素，較差）。
//
// 【4. 為什麼 deque 可以用 std::sort】
//   deque 是「分段連續」：一組固定大小的區塊（本機 libstdc++ 為 512 bytes 一塊）
//   加上一張索引表。它的迭代器內含 4 個指標（cur / first / last / node），
//   operator+ 能靠除法與取餘算出目標區塊與偏移，因此仍是 O(1) 隨機存取，
//   滿足 Random Access Iterator 的要求。
//   但它**不是**連續記憶體（C++20 的 contiguous_iterator 不成立），
//   所以 &deq[0] 不能當成整段陣列的指標傳給 C API。
//
// 【概念補充 Concept Deep Dive】
//   模板是怎麼做到「一份程式碼、多種容器」的？靠的是編譯期實例化（instantiation），
//   而不是執行期多型：
//     - std::find(vec.begin(), ...) 和 std::find(lst.begin(), ...) 會產生
//       兩份**不同的**機器碼，各自針對該迭代器型別最佳化。
//     - 因此沒有虛擬函式呼叫、沒有間接跳躍，++it 可以完全 inline 展開。
//   代價是：(a) 二進位檔變大（code bloat）、(b) 編譯變慢、
//   (c) 型別錯誤時的錯誤訊息極長（C++20 的 concepts 就是為了改善這點）。
//   這是 STL 選擇「編譯期多型」而非「執行期多型（如 Java 的 Collection 介面）」
//   的核心取捨：以編譯時間與程式碼大小，換取零額外執行期成本。
//
// 【注意事項 Pay Attention】
//   1. std::sort(lst.begin(), lst.end()) 對 list 是**編譯錯誤**，不是跑得慢。
//   2. 容器有同名成員函式時優先用成員版：lst.sort() / lst.remove() / lst.unique()
//      都比泛型版對 list 更有效率，且提供更強的迭代器有效性保證。
//   3. 不要把不同容器的迭代器混著用（find(vec.begin(), lst.end(), x)）——
//      這通常編得過（型別可能剛好相容）但行為是未定義的。
//   4. deque 是 Random Access 但非連續記憶體；別把 &deq[0] 當陣列首指標用。
//   5. 演算法在「已排序」前提下才成立的（binary_search / set_intersection /
//      unique），呼叫前務必確認前提，否則結果無意義且不會有任何警告。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】容器與演算法的正交性
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. STL 的「正交設計」是什麼？它解決了什麼問題？
//     答：容器只管儲存、演算法只管運算，兩者不互相認識，靠迭代器接合。
//         結果是 M 個容器與 N 個演算法只需 M+N 份程式碼而非 M×N，
//         而且自訂容器只要提供合格的 begin()/end() 就能免費用上所有 STL 演算法。
//     追問：那六大組件裡的「配置器（allocator）」在這個架構中扮演什麼角色？
//           → 它把「記憶體從哪來」也抽出來，讓同一個容器可以換用記憶體池、
//             共享記憶體等策略而不必改容器實作 —— 是同一種解耦思路的延伸。
//
// 🔥 Q2. 為什麼 std::find 可以用在 list，std::sort 卻不行？
//     答：因為兩者要求的迭代器類別不同。find 只要 Input Iterator（*、++、比較），
//         list 滿足；sort 需要 Random Access Iterator（+n、-、[]），
//         list 只有 Bidirectional，所以編譯失敗。
//         list 改用成員函式 lst.sort()（歸併排序，只重接指標）。
//     追問：list::sort 相對 std::sort 還有什麼額外好處？
//           → 它是穩定排序，而且排序後既有的迭代器與參考仍然有效
//             （元素沒被搬動，只是節點重新串接），std::sort 沒有這個保證。
//
// ⚠️ 陷阱. 「既然 STL 演算法是泛型的，那 std::sort 對 list 應該只是慢一點吧？」
//     答：不是慢一點，是**根本編譯不過**。std::sort 的實作裡有 it + n、
//         it1 - it2 這類運算，list::iterator 沒有這些運算子，
//         樣板實例化時就會失敗。錯誤訊息通常很長（指向 <bits/stl_algo.h> 內部），
//         但根因就是這個。
//     為什麼會錯：把 C++ 樣板想成 Python 的 duck typing —— 以為「跑起來再說」。
//         實際上樣板是編譯期展開的，缺少任何一個用到的運算子都是編譯錯誤。
//         C++20 的 concepts 就是為了把這種錯誤訊息變回人話。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <list>
#include <deque>
#include <string>
#include <algorithm>

template <typename Container>
void print_container(const std::string& name, const Container& c) {
    std::cout << name << ": ";
    for (const auto& elem : c) std::cout << elem << " ";
    std::cout << std::endl;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】不強加。
//   理由：本檔的主題是「同一個演算法能跨容器套用」這個**架構層面**的性質，
//         而 LeetCode 題目一律指定 vector 作為輸入型別，
//         不會出現「同一份解法要同時吃 vector 與 list」的情境。
//         硬套一題只會讓讀者以為正交性是解題技巧，反而模糊了重點。
//         真正需要正交性的是下面那個「多來源資料清洗」的實務範例 ——
//         同一份驗證邏輯要吃三種不同來源的容器。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】跨資料來源的統一查詢（正交性的實際價值）
//   情境：一個訂單系統的資料來自三個地方：
//         - 記憶體快取用 vector<string>（連續、查詢快）
//         - 待處理佇列用 deque<string>（要能兩端進出）
//         - 稽核紀錄用 list<string>（要頻繁在中間插入且迭代器不能失效）
//   需求：寫「一個」函式檢查某筆訂單編號是否存在於任一來源。
//   為什麼用到本主題：因為演算法只認迭代器，這個函式可以寫成 template
//                     並同時服務三種容器 —— 這就是正交性在實務上的兌現。
// -----------------------------------------------------------------------------
template <typename Container>
bool containsOrder(const Container& source, const std::string& order_id) {
    return std::find(source.begin(), source.end(), order_id) != source.end();
}

// 同一份「統計符合條件筆數」的邏輯，也能跨容器共用
template <typename Container, typename Pred>
int countMatching(const Container& source, Pred pred) {
    return static_cast<int>(std::count_if(source.begin(), source.end(), pred));
}

int main() {
    std::vector<int> vec = {3, 1, 4, 1, 5};
    std::list<int> lst = {9, 2, 6, 5, 3};
    std::deque<int> deq = {5, 8, 9, 7, 9};

    // 同一個 sort 演算法用在 vector 和 deque
    std::sort(vec.begin(), vec.end());
    std::sort(deq.begin(), deq.end());

    // list 有自己的 sort（因為 std::sort 需要 Random Access Iterator）
    lst.sort();

    print_container("vector", vec);
    print_container("list", lst);
    print_container("deque", deq);

    // 同一個 find 演算法用在所有容器
    std::cout << "\n尋找元素 5：" << std::endl;

    auto vit = std::find(vec.begin(), vec.end(), 5);
    std::cout << "  vector: " << (vit != vec.end() ? "找到" : "沒找到") << std::endl;

    auto lit = std::find(lst.begin(), lst.end(), 5);
    std::cout << "  list: " << (lit != lst.end() ? "找到" : "沒找到") << std::endl;

    auto dit = std::find(deq.begin(), deq.end(), 5);
    std::cout << "  deque: " << (dit != deq.end() ? "找到" : "沒找到") << std::endl;

    // 演算法的「迭代器類別要求」決定誰能用
    std::cout << "\n=== 誰能用哪個演算法 ===" << std::endl;
    std::cout << "  std::find   需要 Input Iterator        → vector/list/deque 皆可"
              << std::endl;
    std::cout << "  std::count  需要 Input Iterator        → vector/list/deque 皆可"
              << std::endl;
    std::cout << "  std::reverse需要 Bidirectional         → vector/list/deque 皆可"
              << std::endl;
    std::cout << "  std::sort   需要 Random Access         → 只有 vector/deque；"
                 "list 用 lst.sort()" << std::endl;

    // reverse：Bidirectional 就夠，三種容器都能用
    std::reverse(vec.begin(), vec.end());
    std::reverse(lst.begin(), lst.end());
    std::reverse(deq.begin(), deq.end());
    std::cout << "\n三種容器都做 std::reverse 之後：" << std::endl;
    print_container("  vector", vec);
    print_container("  list  ", lst);
    print_container("  deque ", deq);

    std::cout << "\n=== 日常實務：跨三種資料來源的統一查詢 ===" << std::endl;
    std::vector<std::string> cache      = {"ORD-1001", "ORD-1002", "ORD-1003"};
    std::deque<std::string>  pending    = {"ORD-2001", "ORD-2002"};
    std::list<std::string>   audit_log  = {"ORD-1002", "ORD-3001", "ORD-3002"};

    const std::string target = "ORD-1002";
    std::cout << "查詢 " << target << "：" << std::endl;
    std::cout << "  vector 快取    : " << (containsOrder(cache, target) ? "有" : "無") << std::endl;
    std::cout << "  deque  待處理  : " << (containsOrder(pending, target) ? "有" : "無") << std::endl;
    std::cout << "  list   稽核紀錄: " << (containsOrder(audit_log, target) ? "有" : "無") << std::endl;

    // 同一份條件，三種容器共用
    auto is_order_1000s = [](const std::string& id) { return id.rfind("ORD-1", 0) == 0; };
    std::cout << "字首為 ORD-1 的筆數：" << std::endl;
    std::cout << "  vector: " << countMatching(cache, is_order_1000s) << std::endl;
    std::cout << "  deque : " << countMatching(pending, is_order_1000s) << std::endl;
    std::cout << "  list  : " << countMatching(audit_log, is_order_1000s) << std::endl;
    std::cout << "→ 一份 template 程式碼服務三種容器，這就是正交性的價值" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第三課：STL 的六大組件概覽5.cpp -o demo5

// === 預期輸出 ===
// vector: 1 1 3 4 5
// list: 2 3 5 6 9
// deque: 5 7 8 9 9
//
// 尋找元素 5：
//   vector: 找到
//   list: 找到
//   deque: 找到
//
// === 誰能用哪個演算法 ===
//   std::find   需要 Input Iterator        → vector/list/deque 皆可
//   std::count  需要 Input Iterator        → vector/list/deque 皆可
//   std::reverse需要 Bidirectional         → vector/list/deque 皆可
//   std::sort   需要 Random Access         → 只有 vector/deque；list 用 lst.sort()
//
// 三種容器都做 std::reverse 之後：
//   vector: 5 4 3 1 1
//   list  : 9 6 5 3 2
//   deque : 9 9 8 7 5
//
// === 日常實務：跨三種資料來源的統一查詢 ===
// 查詢 ORD-1002：
//   vector 快取    : 有
//   deque  待處理  : 無
//   list   稽核紀錄: 有
// 字首為 ORD-1 的筆數：
//   vector: 3
//   deque : 0
//   list  : 1
// → 一份 template 程式碼服務三種容器，這就是正交性的價值
