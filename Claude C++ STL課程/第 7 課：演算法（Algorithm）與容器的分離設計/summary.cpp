// =============================================================================
//  summary.cpp  —  第 7 課總複習：演算法（Algorithm）與容器的分離設計
// =============================================================================
//
// 【主題資訊 Information】
//   核心概念：STL 把「資料怎麼存」（容器）與「資料怎麼處理」（演算法）拆開，
//             中間以「迭代器（iterator）」作為唯一協定。
//
//   三大元件與職責：
//     容器 Container  → 負責儲存與生命週期管理，提供 begin() / end()
//     迭代器 Iterator → 抽象化的「位置」，提供 *、++、（視分類）--、+n…
//     演算法 Algorithm→ 只接受迭代器，不認識容器
//
//   本課涵蓋的演算法家族（皆在 <algorithm>，數值類在 <numeric>）：
//     搜尋   find / find_if / find_if_not / adjacent_find
//     計數   count / count_if / all_of / any_of / none_of
//     複製   copy / copy_if / copy_n / copy_backward
//     轉換   transform / for_each / replace / replace_if
//     填值   fill / fill_n / generate / generate_n
//     移除   remove / remove_if / unique          ★ 皆需搭配 erase
//     重排   reverse / rotate / partition / stable_partition
//     排序   sort / stable_sort / partial_sort / nth_element
//     二分   binary_search / lower_bound / upper_bound / equal_range
//     極值   min_element / max_element / minmax_element
//     數值   accumulate / iota / partial_sum / adjacent_difference / inner_product
//
//   標準版本：核心為 C++98；C++11 補 all_of/any_of/none_of/copy_if/find_if_not/iota；
//             C++17 加執行策略與 reduce / for_each_n；
//             C++20 加 <ranges>、std::erase / erase_if、constexpr 化
//   標頭檔：<algorithm>、<numeric>、<iterator>、<functional>
//
// 【詳細解釋 Explanation】
//
// 【1. 分離設計要解決的問題：M×N 爆炸】
// 若容器各自實作演算法，M 個容器 × N 個演算法要寫 M×N 份程式碼：
//     vector::sort()  list::sort()  deque::sort()  set::sort() ...
//     vector::find()  list::find()  deque::find()  set::find() ...
// 10 個容器 × 50 個演算法 = 500 份實作，而且**每新增一個容器要補 50 個函式**，
// 每新增一個演算法要改 10 個容器。維護成本隨兩者相乘而爆炸。
// STL 的解法是引入中間層：
//     容器 ──提供──> 迭代器 <──接受── 演算法
// 於是只需要 M + N = 60 份實作，兩邊可以獨立演化。
// **你自己寫的容器只要提供符合規範的 iterator，就能立刻使用全部 STL 演算法；
//   你自己寫的演算法只要接受 iterator，就能立刻適用全部 STL 容器。**
// 這種「靠共同協定達成組合爆炸的收斂」是本課最重要的設計思想。
//
// 【2. 迭代器為什麼能當協定：它是指標的抽象化】
// 看 std::find 的實作骨架：
//     while (first != last) { if (*first == value) return first; ++first; }
//     return last;
// 只用到 !=、*、++ 三個操作，完全沒有提到任何容器型別。
// Stepanov 設計 STL 時是**刻意照著「指標」的樣子**定義迭代器需求的，
// 所以原生指標 int* 天生就是合法的迭代器——
// 「指標是最原始的迭代器」不是比喻，是設計上的因果關係。
//
// 【3. 五種迭代器分類：能力遞增的階梯】
//     Input          *it（讀）、++、!=            單次通過
//     Forward        Input + 可重複走訪、可寫入
//     Bidirectional  Forward + --                 可往回走
//     Random Access  Bidirectional + +n、-n、[]、< O(1) 跳躍
//     Contiguous     Random Access + 記憶體連續    C++20 新增
//   容器提供的分類：
//     vector/array/deque → Random Access（vector/array 於 C++20 起為 Contiguous）
//     list/set/map       → Bidirectional
//     forward_list/unordered_* → Forward
// **演算法宣告需求、容器宣告能力，兩者在編譯期對帳**——
// 這就是 std::sort 對 list 會編譯失敗的原因（sort 要 Random Access）。
//
// 【4. 分離設計的三個代價（本課最重要的實務知識）】
//
//   代價一：**演算法無法改變容器大小** → remove / unique 不會真的刪除
//     演算法只有兩個 iterator，拿不到容器本體，無法呼叫 erase。
//     所以 std::remove 只能「把要保留的往前搬」並回傳新的邏輯結尾：
//         v.erase(std::remove(v.begin(), v.end(), val), v.end());   // erase-remove idiom
//     C++20 補上 std::erase(v, val) / std::erase_if(v, pred) 直接吃容器，一行解決。
//
//   代價二：**演算法看不到容器的內部結構** → 同名成員函式往往更快或語意不同
//     std::find 對 set 是 O(N)（逐一比較），set::find 是 O(log N)（樹狀搜尋）。
//     list::remove 是真刪除，std::remove 只搬移。
//     **準則：同名時一律優先用成員函式。**
//
//   代價三：**iterator 分類限制了演算法的適用範圍**
//     編譯失敗型：std::sort 對 list（需 Random Access）
//     默默變慢型：std::lower_bound 對 list（能編譯，但總時間退化成 O(N)）
//     後者更危險，因為程式會跑出正確答案，效能問題要到資料量變大才浮現。
//
// 【5. 演算法的五種參數形式（認得形式就能猜出簽名）】
//     (1) 只有範圍      f(first, last)             sort, reverse
//     (2) 範圍 + 值     f(first, last, value)      count, find, fill
//     (3) 範圍 + 謂詞   f(first, last, pred)       count_if, find_if
//     (4) 範圍 + 輸出   f(first, last, d_first)    copy, transform
//     (5) 兩個範圍      f(f1, l1, f2)              equal, mismatch
//   命名慣例：
//     _if    改用謂詞判斷           （find → find_if）
//     _copy  不就地修改、輸出到別處 （remove → remove_copy）
//     _n     用個數取代終點         （copy → copy_n）
//   為什麼用命名而非多載來區分「值」與「謂詞」？因為當元素本身就是可呼叫物件時
//   （如 vector<function<bool(int)>>），兩者在型別上無法區分，多載會產生歧義。
//
// 【6. 半開區間 [first, last) 的三個理由】
//   (a) 空範圍自然表示成 first == last，不需特例；
//       閉區間要表示空範圍得寫 last = first - 1，對首元素會產生無效位址。
//   (b) 長度直接是 last - first，不必 +1，off-by-one 錯誤少一半。
//   (c) 迴圈條件只需 !=，不需要 <，因此非隨機存取的 iterator 也能用同一套演算法。
//   這也是 end() 指向「最後一個元素的下一個位置」而非最後一個元素的原因。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 靜態多型：零抽象成本的來源
//   find(vec.begin(),...) 與 find(lst.begin(),...) 會實例化成**兩份不同的機器碼**：
//   前者的 ++ 是「指標前進一個元素」，後者是「載入 node->next」。
//   所以「共用」是**原始碼層級的共用**，不是執行期的共用——
//   沒有虛擬函式、沒有間接呼叫、沒有執行期型別判斷。
//   這與 Java 用 interface + 動態分派的做法本質不同，代價是編譯時間與程式碼體積。
//
// (B) 標籤分派（tag dispatch）：依能力自動選最佳實作
//   std::advance 對 Input Iterator 用 while(n--) ++it（O(n)），
//   對 Random Access Iterator 用 it += n（O(1)），由 iterator_category 在編譯期選擇。
//   std::distance、std::rotate、std::reverse 都用同樣手法。
//   這是分離設計的隱藏紅利：**演算法可以針對能力更強的 iterator 偷偷加速**。
//
// (C) 謂詞必須是純函式
//   標準**不保證**謂詞的呼叫次數與順序（find_if 會提早結束、
//   transform 不保證順序、平行版本更不保證）。
//   唯一例外是 for_each 與 generate：標準明文保證依序呼叫恰好一次。
//
// (D) C++20 ranges：分離設計的下一步
//   std::ranges::sort(v) 直接吃容器，省去 begin()/end()；
//   views 還能組合出惰性求值的管線（filter | transform）。
//   但底層仍是 iterator——ranges 是在其上加了一層更好用的介面，不是取代它。
//
// 【注意事項 Pay Attention】
// 1. **remove / unique 不會真的刪除**，必須搭配 erase（C++20 可用 std::erase_if）。
// 2. **同名時優先用成員函式**：set::find、list::sort / remove / unique / splice。
// 3. **std::sort 需要 Random Access**：list / forward_list / set 編譯失敗。
// 4. **比較器必須是嚴格弱序**（用 < 不用 <=），否則是未定義行為。
// 5. **輸出型演算法不檢查目的地容量**；大小未知時用 std::back_inserter。
// 6. **二分搜尋家族要求範圍已依同一準則排序**，且自訂比較器必須一併傳入。
// 7. **accumulate 的 init 型別決定累加型別**：對 double 傳 0 會被截斷。
// 8. 要「是非題」用 any_of / all_of / none_of（提早結束），
//    不要用 count_if(...) > 0（一定掃完）。
// 9. 對 char 序列使用 toupper / tolower **必須先轉 unsigned char**，
//    否則對非 ASCII 位元組是未定義行為。
// 10. nth_element / partial_sort 留下的「未指定順序」區域不可依賴。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】演算法與容器的分離設計（第 7 課總覽）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 請說明 STL 為什麼要把演算法和容器分開，這樣設計解決了什麼問題？
//     答：解決 M×N 爆炸。若每個容器各自實作所有演算法，M 個容器 × N 個演算法
//         需要 M×N 份程式碼，且新增任一方都要改動另一方全部。
//         STL 引入 iterator 作為中間協定，容器只負責提供 iterator、
//         演算法只接受 iterator，實作量降為 M+N，兩邊可獨立演化。
//         而且使用者自訂的容器只要提供合規的 iterator，就能立刻用上全部 STL 演算法。
//     追問：這樣的抽象有執行期成本嗎？→ 沒有。template 會為每個 iterator 型別
//         各實例化一份程式碼，是編譯期的靜態多型，沒有虛擬函式與動態分派，
//         最佳化後與手寫迴圈相當。代價是編譯時間與程式碼體積。
//
// 🔥 Q2. std::remove 為什麼不會真的刪除元素？正確的刪除寫法是什麼？
//     答：因為演算法只拿到兩個 iterator，**拿不到容器本體**。
//         刪除要改變 size、可能要釋放記憶體，那是容器成員函式的職責，
//         remove 沒有能力呼叫 vector::erase，甚至不知道這段範圍屬於哪個容器。
//         它只能把要保留的元素往前搬並回傳新的邏輯結尾，正確寫法是：
//             v.erase(std::remove(v.begin(), v.end(), val), v.end());
//         C++20 起可直接寫 std::erase(v, val)。
//     追問：remove 之後尾端那些元素的值是什麼？→ **valid but unspecified**。
//         C++11 起 remove 用 move assignment 搬移，尾端是被 move 走的殘骸。
//         對 int 實務上看得到舊值，對 std::string 通常已變成空字串，
//         標準不保證任何特定值，絕不可依賴。
//
// 🔥 Q3. std::find(s.begin(), s.end(), x) 和 s.find(x)（s 為 set）差在哪？
//        該用哪個？這反映了什麼設計取捨？
//     答：std::find 是 O(N) 逐一比較，set::find 是 O(log N) 樹狀搜尋。
//         應該用成員函式。這正反映分離設計的代價：通用演算法只看得到 iterator，
//         **看不到容器的內部結構**，不知道 set 是排序好的樹，只能一個一個比。
//         所以標準在容器上另外提供了同名的成員函式。
//         **準則：同名時一律優先用成員函式。**
//     追問：list::remove 和 std::remove 也是這樣嗎？→ 更嚴重，那不只是效能差異，
//         而是**語意不同**：list::remove 是真刪除（size 會變），
//         std::remove 只搬移（size 不變）。用錯會得到錯誤結果，不只是慢。
//
// 🔥 Q4. 為什麼 std::sort 不能用在 std::list 上？list 要怎麼排序？
//     答：std::sort 要求 **Random Access Iterator**，因為 introsort 需要選 pivot、
//         計算中點、用索引算 heap 父子節點，這些都要 O(1) 隨機存取。
//         list 只提供 Bidirectional Iterator，其 iterator 沒有定義 operator+/-，
//         **編譯期就會失敗**（不是執行期變慢）。
//         list 要用成員函式 lst.sort()，它用 merge sort 只重新串接節點指標。
//     追問：list::sort 有什麼 std::sort 沒有的優點？→ 元素本身完全不移動，
//         所以排序後既有的 iterator / reference / pointer 仍指向同一個元素、
//         依然有效；而且它是 merge sort，**保證穩定**。
//
// ⚠️ 陷阱 1. 「STL 演算法是泛型的，所以任何演算法都能用在任何容器上」——錯在哪？
//     答：錯。泛型受**迭代器分類**限制。find 只要 Input Iterator 所以人人可用；
//         sort 要 Random Access，list 就編譯失敗。更麻煩的是「能編譯但會變慢」
//         的情況：std::lower_bound 只要 Forward Iterator，對 list 能編譯、
//         比較次數也是 O(log N)，但每次跳到中點都要走 k 次 ++，
//         **總時間退化成 O(N)**，比直接 std::find 還慢。
//     為什麼會錯：多數人把「泛型」理解成「無條件通用」。
//         STL 的泛型是「對**滿足特定需求**的型別通用」，
//         而且「能編譯」只保證語法可用，**不保證複雜度符合預期**。
//
// ⚠️ 陷阱 2. 這段程式碼有兩個錯誤，分別是什麼？
//        std::vector<int> dest;
//        std::copy(src.begin(), src.end(), dest.begin());
//        std::sort(dest.begin(), dest.end(), [](int a, int b){ return a <= b; });
//     答：(1) dest 是空的，copy 不會讓容器自動成長，這是**緩衝區溢位**
//             （未定義行為）。要嘛先 dest.resize(src.size())，
//             要嘛改用 std::back_inserter(dest)。
//         (2) 比較器用了 <=，**違反嚴格弱序的非自反性**（comp(a,a) 必須為 false），
//             也是未定義行為；相等元素會被判定成互相小於對方，
//             排序內部的邊界判斷失效可能導致記憶體越界。應該用 <。
//     為什麼會錯：兩者都是「小資料測試會通過」的陷阱——
//         空 vector 可能因 capacity 剛好而沒當場崩潰；
//         <= 的比較器在 16 個元素以下走 insertion sort 不會觸發越界。
//         資料量變大才爆炸，而且不一定每次都爆。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <array>
#include <string>
#include <algorithm>
#include <numeric>
#include <functional>
#include <iterator>
#include <cctype>
using namespace std;

// ================================================================
// 重點一：分離設計哲學
// ================================================================
// STL 的核心設計：演算法（algorithm）獨立於容器（container）
// 演算法不直接操作容器，而是透過「迭代器」操作資料
//
// 傳統方式：每種資料結構有自己的一套操作函數（大量重複）
// STL 方式：一個演算法 + 任何容器的迭代器 = 通用操作
//
// 好處：
//   - N 個容器 × M 個演算法 = 只需要 N+M 個實作（不是 N×M）
//   - 新增容器或演算法不影響另一邊

// ================================================================
// 重點二：同一個演算法用於不同容器
// ================================================================
// std::find 只需要「開始迭代器」和「結束迭代器」
// 可以用在 vector、list、set、array、甚至原生陣列

void demoFindUniversal() {
    cout << "\n【同一個 find 用在不同容器】" << endl;

    vector<int> vec = {10, 20, 30, 40, 50};
    list<int> lst = {10, 20, 30, 40, 50};
    set<int> s = {10, 20, 30, 40, 50};
    int arr[] = {10, 20, 30, 40, 50};

    // 完全相同的 std::find 介面
    auto v_it = find(vec.begin(), vec.end(), 30);
    auto l_it = find(lst.begin(), lst.end(), 30);
    auto s_it = find(s.begin(), s.end(), 30);
    auto a_it = find(arr, arr + 5, 30);     // 原生陣列：指標就是迭代器

    cout << "在 vector 找到 30: " << (v_it != vec.end() ? "✓" : "✗") << endl;
    cout << "在 list 找到 30:   " << (l_it != lst.end() ? "✓" : "✗") << endl;
    cout << "在 set 找到 30:    " << (s_it != s.end() ? "✓" : "✗") << endl;
    cout << "在 array 找到 30:  " << (a_it != arr + 5 ? "✓" : "✗") << endl;

    // 但對 set 應優先用成員函式：O(log N) 而非 O(N)
    cout << "（set 應改用 s.find(30)：O(log N)，通用版是 O(N)）" << endl;
}

// ================================================================
// 重點三：排序演算法 —— std::sort
// ================================================================
// sort 要求「隨機存取迭代器」（vector、array、deque）
// list 沒有隨機存取迭代器，需用自己的 list::sort()

void demoSort() {
    cout << "\n【std::sort 排序】" << endl;

    vector<int> v = {5, 2, 8, 1, 9, 3};

    // 預設：升序（小到大）
    sort(v.begin(), v.end());
    cout << "升序: ";
    for (int n : v) cout << n << " ";
    cout << endl;

    // 自定義：降序（大到小）
    sort(v.begin(), v.end(), greater<int>());
    cout << "降序: ";
    for (int n : v) cout << n << " ";
    cout << endl;

    // 使用 lambda 自定義排序規則（★ 用 < 不用 <=，否則違反嚴格弱序）
    vector<string> words = {"banana", "apple", "cherry", "date"};
    sort(words.begin(), words.end(), [](const string& a, const string& b) {
        return a.length() < b.length();  // 按字串長度升序
    });
    cout << "按長度排序: ";
    for (const string& w : words) cout << w << " ";
    cout << endl;

    // list 不能用 std::sort（編譯失敗），要用成員函式
    list<int> lst = {5, 2, 8, 1, 9};
    lst.sort();                    // merge sort，只改節點指標
    cout << "list 用成員函式 sort: ";
    for (int n : lst) cout << n << " ";
    cout << endl;
}

// ================================================================
// 重點四：計數演算法 —— std::count / std::count_if
// ================================================================
// count(begin, end, value)：計算等於 value 的元素數量
// count_if(begin, end, pred)：計算滿足謂詞的元素數量

void demoCount() {
    cout << "\n【std::count / count_if】" << endl;

    vector<int> v = {1, 3, 5, 2, 4, 3, 6, 3, 8};

    // 計算特定值的數量
    int threes = count(v.begin(), v.end(), 3);
    cout << "3 出現了 " << threes << " 次" << endl;

    // 計算滿足條件的數量（謂詞）
    int evens = count_if(v.begin(), v.end(), [](int n) {
        return n % 2 == 0;  // 偶數
    });
    cout << "偶數有 " << evens << " 個" << endl;

    int gt4 = count_if(v.begin(), v.end(), [](int n) {
        return n > 4;
    });
    cout << "大於 4 的有 " << gt4 << " 個" << endl;

    // ★ 只想知道「有沒有」時，用 any_of 而非 count_if(...) > 0
    bool hasGt4 = any_of(v.begin(), v.end(), [](int n) { return n > 4; });
    cout << "有大於 4 的嗎: " << (hasGt4 ? "有" : "沒有")
         << "（用 any_of：找到就停；count_if 一定掃完）" << endl;
}

// ================================================================
// 重點五：轉換演算法 —— std::transform
// ================================================================
// transform：對每個元素套用函數，結果寫入目標位置
// transform(srcBegin, srcEnd, destBegin, func)
// transform(src1Begin, src1End, src2Begin, destBegin, binaryFunc)

void demoTransform() {
    cout << "\n【std::transform】" << endl;

    vector<int> v = {1, 2, 3, 4, 5};

    // 每個元素乘以 2，結果存到新 vector（★ 目的地要先配置好空間）
    vector<int> doubled(v.size());
    transform(v.begin(), v.end(), doubled.begin(), [](int n) {
        return n * 2;
    });
    cout << "乘以 2: ";
    for (int n : doubled) cout << n << " ";
    cout << endl;

    // 就地修改（原地轉換）—— 標準明確允許 d_first == first
    transform(v.begin(), v.end(), v.begin(), [](int n) {
        return n * n;  // 平方
    });
    cout << "平方後: ";
    for (int n : v) cout << n << " ";
    cout << endl;

    // 字串轉大寫
    // ★ 必須先轉 unsigned char：char 在 x86 Linux 是有號的，
    //   把負值傳給 <cctype> 的 toupper 是未定義行為
    string s = "hello world";
    transform(s.begin(), s.end(), s.begin(),
              [](unsigned char c) { return static_cast<char>(toupper(c)); });
    cout << "轉大寫: " << s << endl;
}

// ================================================================
// 重點六：for_each —— 對每個元素執行操作
// ================================================================
// for_each(begin, end, func)：對每個元素呼叫 func
// 注意：不會修改元素（除非 func 透過參考修改）
// C++11 後的 range-based for 更簡潔，但 for_each 可以傳回函數物件

void demoForEach() {
    cout << "\n【std::for_each】" << endl;

    vector<int> v = {10, 20, 30, 40, 50};

    // 簡單的 for_each
    cout << "印出每個元素: ";
    for_each(v.begin(), v.end(), [](int n) {
        cout << n << " ";
    });
    cout << endl;

    // for_each 配合有狀態的 lambda
    int sum = 0;
    for_each(v.begin(), v.end(), [&sum](int n) {
        sum += n;  // 捕獲 sum 的參考
    });
    cout << "總和: " << sum << endl;

    // ★ for_each 唯一不可取代之處：它會回傳函式物件
    struct Counter {
        int over25 = 0;
        void operator()(int n) { if (n > 25) ++over25; }
    };
    Counter c = for_each(v.begin(), v.end(), Counter());
    cout << "大於 25 的個數（由回傳的函式物件取得）: " << c.over25 << endl;
}

// ================================================================
// 重點七：搜尋與查找演算法
// ================================================================
// find_if(begin, end, pred)：找到第一個滿足謂詞的元素
// any_of / all_of / none_of：條件判斷

void demoSearch() {
    cout << "\n【搜尋與條件判斷】" << endl;

    vector<int> v = {1, 3, 5, 7, 8, 11, 13};

    // find_if：找第一個偶數
    auto it = find_if(v.begin(), v.end(), [](int n) {
        return n % 2 == 0;
    });
    if (it != v.end()) {
        cout << "第一個偶數: " << *it << endl;
    }

    // any_of：是否有任何元素滿足條件
    bool hasNeg = any_of(v.begin(), v.end(), [](int n) { return n < 0; });
    cout << "有負數: " << (hasNeg ? "是" : "否") << endl;

    // all_of：是否所有元素滿足條件
    bool allPos = all_of(v.begin(), v.end(), [](int n) { return n > 0; });
    cout << "都是正數: " << (allPos ? "是" : "否") << endl;

    // none_of：是否沒有元素滿足條件
    bool noneGt100 = none_of(v.begin(), v.end(), [](int n) { return n > 100; });
    cout << "沒有大於 100: " << (noneGt100 ? "是" : "否") << endl;

    // ★ 空範圍語意（面試常考）
    vector<int> emptyVec;
    cout << "空範圍 all_of: "
         << (all_of(emptyVec.begin(), emptyVec.end(), [](int n) { return n > 0; }) ? "true" : "false")
         << "（vacuous truth：沒有反例即為真）" << endl;
}

// ================================================================
// 重點八：數值演算法 —— accumulate, iota
// ================================================================
// accumulate：累積計算（求和、乘積等）
// iota：填入連續值

void demoNumeric() {
    cout << "\n【數值演算法 <numeric>】" << endl;

    vector<int> v(5);
    iota(v.begin(), v.end(), 1);  // 填入 1, 2, 3, 4, 5
    cout << "iota 填入: ";
    for (int n : v) cout << n << " ";
    cout << endl;

    // 求和
    int sum = accumulate(v.begin(), v.end(), 0);
    cout << "總和: " << sum << endl;

    // 乘積（初始值設為 1，使用 multiplies）
    int product = accumulate(v.begin(), v.end(), 1, multiplies<int>());
    cout << "乘積 (5!): " << product << endl;

    // ★ init 的型別決定累加型別 —— 最經典的坑
    vector<double> prices = {1.5, 2.5, 3.5};
    cout << "double 序列用 init=0   (int)   : "
         << accumulate(prices.begin(), prices.end(), 0) << "（被截斷！）" << endl;
    cout << "double 序列用 init=0.0 (double): "
         << accumulate(prices.begin(), prices.end(), 0.0) << "（正確）" << endl;
}

// ================================================================
// 重點九：移除類演算法不會真的刪除
// ================================================================

void demoRemoveIdiom() {
    cout << "\n【erase-remove idiom：分離設計最重要的後果】" << endl;

    vector<int> v = {1, 2, 3, 2, 4, 2, 5};
    cout << "原始 size = " << v.size() << endl;

    // 只呼叫 remove：size 完全不變
    vector<int> onlyRemove = v;
    (void)remove(onlyRemove.begin(), onlyRemove.end(), 2);   // 刻意丟棄回傳值
    cout << "只呼叫 remove 後 size = " << onlyRemove.size() << "（什麼都沒刪掉）" << endl;

    // 完整的 erase-remove idiom
    vector<int> full = v;
    full.erase(remove(full.begin(), full.end(), 2), full.end());
    cout << "erase-remove 後 size = " << full.size() << "，內容: ";
    for (int n : full) cout << n << " ";
    cout << endl;

    // list 有自己的 remove，是真正的刪除
    list<int> lst = {1, 2, 3, 2, 4, 2, 5};
    lst.remove(2);
    cout << "list::remove(2) 後 size = " << lst.size() << "（成員函式是真刪除）" << endl;
}

// ================================================================
// 重點十：常用演算法快速參考
// ================================================================
//
// ┌──────────────────┬──────────────────────────────────────────┐
// │ 演算法            │ 說明                                      │
// ├──────────────────┼──────────────────────────────────────────┤
// │ find             │ 找第一個等於 value 的元素                  │
// │ find_if          │ 找第一個滿足謂詞的元素                     │
// │ sort             │ 排序（需隨機存取迭代器）                    │
// │ stable_sort      │ 穩定排序（相等元素保持原來順序）            │
// │ count            │ 計算等於 value 的元素數                    │
// │ count_if         │ 計算滿足謂詞的元素數                       │
// │ transform        │ 套用函數轉換每個元素                        │
// │ for_each         │ 對每個元素執行操作                          │
// │ any_of           │ 是否有任意元素滿足條件                      │
// │ all_of           │ 是否所有元素滿足條件                        │
// │ none_of          │ 是否沒有元素滿足條件                        │
// │ accumulate       │ 累積計算（求和等）                          │
// │ iota             │ 填入連續遞增值                              │
// │ max_element      │ 找最大元素的迭代器                          │
// │ min_element      │ 找最小元素的迭代器                          │
// │ reverse          │ 反轉範圍                                    │
// │ unique           │ 移除相鄰重複元素（需搭配 erase）            │
// │ remove_if        │ 邏輯移除滿足條件的元素（需搭配 erase）      │
// └──────────────────┴──────────────────────────────────────────┘

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 27. Remove Element
//   題目：就地移除所有等於 val 的元素，回傳剩餘個數 k，前 k 個即為結果。
//   為什麼用到本主題：這題的規格**就是 std::remove 的合約**——
//         把要保留的搬到前面、回傳新的邏輯結尾、不要求處理尾端。
//         是理解「remove 為何不刪除」最好的練習。
//   複雜度：O(N)。
// -----------------------------------------------------------------------------
int removeElement(vector<int>& nums, int val) {
    return static_cast<int>(remove(nums.begin(), nums.end(), val) - nums.begin());
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 35. Search Insert Position
//   題目：在已排序陣列找 target 的索引；不存在則回傳它該插入的位置。
//   為什麼用到本主題：**這正是 std::lower_bound 的定義**——
//         「第一個 >= target 的位置」同時就是元素位置與插入點。
//   複雜度：O(log N)。
// -----------------------------------------------------------------------------
int searchInsert(const vector<int>& nums, int target) {
    return static_cast<int>(lower_bound(nums.begin(), nums.end(), target) - nums.begin());
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 3】LeetCode 215. Kth Largest Element in an Array
//   題目：回傳陣列中第 k 大的元素。
//   為什麼用到本主題：只需要**一個值**，不需要整個陣列有序，
//         正是 nth_element 的定義（平均 O(N)，優於排序的 O(N log N)）。
// -----------------------------------------------------------------------------
int findKthLargest(vector<int> nums, int k) {
    auto target = nums.begin() + (static_cast<int>(nums.size()) - k);
    nth_element(nums.begin(), target, nums.end());
    return *target;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】伺服器 log 分析管線（一次用上本課大半演算法）
//   情境：每天處理一批存取紀錄，要產出營運報表：
//         過濾出錯誤請求、依耗時排名、統計各狀態碼、計算總流量、去重使用者清單。
//   為什麼用到本主題：真實的資料處理幾乎不會只用一個演算法，
//         而是「過濾 → 轉換 → 排序 → 歸納」串成管線。
//         本例刻意示範各步驟該選哪個演算法，以及為什麼。
// -----------------------------------------------------------------------------
struct AccessLog {
    string path;
    int status;
    int latencyMs;
    long long bytes;
    string userId;
};

void analyzeAccessLogs(const vector<AccessLog>& logs) {
    // ① 過濾：目的地大小未知 → copy_if + back_inserter
    vector<AccessLog> errors;
    copy_if(logs.begin(), logs.end(), back_inserter(errors),
            [](const AccessLog& e) { return e.status >= 400; });
    cout << "  錯誤請求: " << errors.size() << " 筆" << endl;

    // ② 是非題 → any_of（找到就停），不要用 count_if(...) > 0
    bool hasServerError = any_of(logs.begin(), logs.end(),
                                 [](const AccessLog& e) { return e.status >= 500; });
    cout << "  有伺服器錯誤(5xx): " << (hasServerError ? "是" : "否") << endl;

    // ③ 要數量 → count_if
    auto slowCount = count_if(logs.begin(), logs.end(),
                              [](const AccessLog& e) { return e.latencyMs > 500; });
    cout << "  逾時(>500ms)請求: " << slowCount << " 筆" << endl;

    // ④ 只要前 3 名排行榜 → partial_sort（不必替全部資料排序）
    vector<AccessLog> ranked = logs;
    const size_t topN = min<size_t>(3, ranked.size());
    partial_sort(ranked.begin(), ranked.begin() + static_cast<ptrdiff_t>(topN), ranked.end(),
                 [](const AccessLog& a, const AccessLog& b) {
                     return a.latencyMs > b.latencyMs;
                 });
    cout << "  最慢的 " << topN << " 個請求:" << endl;
    for (size_t i = 0; i < topN; ++i) {
        cout << "    " << (i + 1) << ". " << ranked[i].path
             << "  " << ranked[i].latencyMs << "ms" << endl;
    }

    // ⑤ 歸納成單一數值 → accumulate（★ init 用 0LL 防溢位）
    long long totalBytes = accumulate(logs.begin(), logs.end(), 0LL,
                                      [](long long acc, const AccessLog& e) {
                                          return acc + e.bytes;
                                      });
    cout << "  總傳輸量: " << totalBytes << " bytes" << endl;

    // ⑥ 去重 → 先 transform 抽出欄位，再 sort + unique + erase
    vector<string> users;
    users.reserve(logs.size());
    transform(logs.begin(), logs.end(), back_inserter(users),
              [](const AccessLog& e) { return e.userId; });
    sort(users.begin(), users.end());                              // 讓相同的相鄰
    users.erase(unique(users.begin(), users.end()), users.end());  // unique 也要 erase
    cout << "  不重複使用者: " << users.size() << " 人 (";
    for (const auto& u : users) cout << u << " ";
    cout << ")" << endl;

    // ⑦ 統計各狀態碼 → map 計數（成員函式，不用通用演算法）
    map<int, int> statusCount;
    for (const auto& e : logs) ++statusCount[e.status];
    cout << "  狀態碼分布: ";
    for (const auto& kv : statusCount) cout << kv.first << "×" << kv.second << " ";
    cout << endl;
}

int main() {
    cout << "============================================" << endl;
    cout << "   第 7 課：演算法與容器的分離設計展示" << endl;
    cout << "============================================" << endl;

    demoFindUniversal();
    demoSort();
    demoCount();
    demoTransform();
    demoForEach();
    demoSearch();
    demoNumeric();
    demoRemoveIdiom();

    cout << "\n=== LeetCode 27. Remove Element ===" << endl;
    vector<int> lc27 = {0, 1, 2, 2, 3, 0, 4, 2};
    int k = removeElement(lc27, 2);
    cout << "[0,1,2,2,3,0,4,2] 移除 2 -> k=" << k << ", 前 k 個: ";
    for (int i = 0; i < k; ++i) cout << lc27[i] << " ";
    cout << endl;

    cout << "\n=== LeetCode 35. Search Insert Position ===" << endl;
    cout << "[1,3,5,6] 找 5 -> " << searchInsert({1, 3, 5, 6}, 5) << endl;
    cout << "[1,3,5,6] 找 2 -> " << searchInsert({1, 3, 5, 6}, 2) << endl;

    cout << "\n=== LeetCode 215. Kth Largest Element in an Array ===" << endl;
    cout << "[3,2,1,5,6,4], k=2 -> " << findKthLargest({3, 2, 1, 5, 6, 4}, 2) << endl;

    cout << "\n=== 日常實務：伺服器 log 分析管線 ===" << endl;
    analyzeAccessLogs({
        {"/api/orders",  200, 820,  15000, "u1001"},
        {"/api/users",   200,  95,   3200, "u1002"},
        {"/api/search",  500, 1240,   180, "u1001"},
        {"/api/login",   401, 150,     90, "u1003"},
        {"/api/report",  200, 670,  98000, "u1002"},
        {"/api/health",  200,  12,     40, "u1001"}
    });

    cout << "\n============================================" << endl;
    cout << " 設計哲學：演算法 + 迭代器 = 與容器無關的通用操作" << endl;
    cout << "============================================" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
// ============================================
//    第 7 課：演算法與容器的分離設計展示
// ============================================
//
// 【同一個 find 用在不同容器】
// 在 vector 找到 30: ✓
// 在 list 找到 30:   ✓
// 在 set 找到 30:    ✓
// 在 array 找到 30:  ✓
// （set 應改用 s.find(30)：O(log N)，通用版是 O(N)）
//
// 【std::sort 排序】
// 升序: 1 2 3 5 8 9 
// 降序: 9 8 5 3 2 1 
// 按長度排序: date apple banana cherry 
// list 用成員函式 sort: 1 2 5 8 9 
//
// 【std::count / count_if】
// 3 出現了 3 次
// 偶數有 4 個
// 大於 4 的有 3 個
// 有大於 4 的嗎: 有（用 any_of：找到就停；count_if 一定掃完）
//
// 【std::transform】
// 乘以 2: 2 4 6 8 10 
// 平方後: 1 4 9 16 25 
// 轉大寫: HELLO WORLD
//
// 【std::for_each】
// 印出每個元素: 10 20 30 40 50 
// 總和: 150
// 大於 25 的個數（由回傳的函式物件取得）: 3
//
// 【搜尋與條件判斷】
// 第一個偶數: 8
// 有負數: 否
// 都是正數: 是
// 沒有大於 100: 是
// 空範圍 all_of: true（vacuous truth：沒有反例即為真）
//
// 【數值演算法 <numeric>】
// iota 填入: 1 2 3 4 5 
// 總和: 15
// 乘積 (5!): 120
// double 序列用 init=0   (int)   : 6（被截斷！）
// double 序列用 init=0.0 (double): 7.5（正確）
//
// 【erase-remove idiom：分離設計最重要的後果】
// 原始 size = 7
// 只呼叫 remove 後 size = 7（什麼都沒刪掉）
// erase-remove 後 size = 4，內容: 1 3 4 5 
// list::remove(2) 後 size = 4（成員函式是真刪除）
//
// === LeetCode 27. Remove Element ===
// [0,1,2,2,3,0,4,2] 移除 2 -> k=5, 前 k 個: 0 1 3 0 4 
//
// === LeetCode 35. Search Insert Position ===
// [1,3,5,6] 找 5 -> 2
// [1,3,5,6] 找 2 -> 1
//
// === LeetCode 215. Kth Largest Element in an Array ===
// [3,2,1,5,6,4], k=2 -> 5
//
// === 日常實務：伺服器 log 分析管線 ===
//   錯誤請求: 2 筆
//   有伺服器錯誤(5xx): 是
//   逾時(>500ms)請求: 3 筆
//   最慢的 3 個請求:
//     1. /api/search  1240ms
//     2. /api/orders  820ms
//     3. /api/report  670ms
//   總傳輸量: 116510 bytes
//   不重複使用者: 3 人 (u1001 u1002 u1003 )
//   狀態碼分布: 200×4 401×1 500×1 
//
// ============================================
//  設計哲學：演算法 + 迭代器 = 與容器無關的通用操作
// ============================================
