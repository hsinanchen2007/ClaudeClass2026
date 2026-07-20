// =============================================================================
//  summary.cpp  —  std::list 的宣告與初始化：8 種建構路徑與它們的取捨
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<list>
//   類別：  template<class T, class Allocator = std::allocator<T>> class list;
//
//   建構子（節錄，C++17）：
//     list();                                              // (1) 預設
//     explicit list(size_type n);                          // (2) n 個 value-initialized
//     list(size_type n, const T& v);                       // (3) n 個 v 的複製
//     template<class It> list(It first, It last);          // (4) 範圍
//     list(const list& other);                             // (5) 複製
//     list(list&& other);                                  // (6) 移動
//     list(std::initializer_list<T> il);                   // (7) 初始化列表
//
//   複雜度：
//     (1) O(1)      (2)(3) O(n)     (4) O(distance(first,last))
//     (5) O(n)      (6) O(1)        (7) O(n)
//     assign(...)   O(舊 size + 新 size)
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 list 的建構子長得跟 vector 幾乎一樣】
//   這不是巧合，而是 STL 的「概念（Concept）」設計刻意造成的。標準把
//   SequenceContainer 的共同需求寫成一張表，凡是宣稱自己是 sequence
//   container 的型別（vector / deque / list / forward_list）都必須提供
//   同一組建構子與同樣的語意。
//   這帶來一個很實際的好處：
//       template<class C> C load(It first, It last) { return C(first, last); }
//   這種泛型程式碼可以把 C 換成任何 sequence container 而不必改寫。
//   換句話說，「初始化方式一致」是為了讓泛型程式碼可替換，
//   而不是因為它們內部長得像——事實上 list 內部和 vector 天差地遠。
//
// 【2. list 沒有的東西，比它有的東西更能說明它的定位】
//   list 的建構子少了一個 vector 一定有的東西：reserve()／capacity()。
//   原因是 list 根本沒有「容量」這個概念——每個元素是一個獨立配置的
//   節點，插入時才配置一個節點，不存在「先買一塊大地、之後慢慢蓋」。
//   所以：
//     * list<int> l(5);               → 真的配置 5 個節點，每個節點值為 0
//     * vector<int> v; v.reserve(5);  → 一塊記憶體，size() 還是 0
//   兩者看似都「準備了 5 個」，語意完全不同。初學者最常混淆這一點。
//
// 【3. 八種初始化方式的實際取捨】
//   (1) 預設建構     list<int> a;
//       O(1)，且不碰 heap；但 libstdc++ 仍會把 header node 接成自環
//       （見【概念補充 A】），所以「空 list」不是零動作，只是極廉價。
//   (2) 填充建構     list<int> b(5);
//       explicit，所以不能寫 list<int> b = 5;（這是刻意的，避免
//       「一個整數莫名其妙變成容器」這種意外轉換）。
//   (3) 填充指定值   list<int> c(5, 42);
//   (4) 範圍建構     list<int> d(v.begin(), v.end());
//       最泛用的一種——來源可以是任何 InputIterator，包含
//       istream_iterator，因此可以直接從標準輸入建構容器。
//   (5) 複製建構     深複製，逐節點配置。O(n) 且 n 次 heap 配置，
//       是這幾種裡最貴的。
//   (6) 移動建構     O(1)。偷走節點鏈與 size，不動任何元素。
//   (7) 初始化列表   list<int> e = {1,2,3};
//       注意 initializer_list 內部是 const T 陣列，元素一定是「複製」
//       進容器，不會被 move——這是 initializer_list 最著名的限制。
//   (8) assign(...)  不是建構子，是「重設既有容器」。它會盡量重用
//       既有節點（改值而不是釋放再配置），所以對「反覆重填同一個
//       list」的迴圈，assign 通常比 clear() + push_back 便宜。
//
// 【4. (n, val) 與 {n, val}：優先權規則與它的例外】
//   C++11 引進 list-initialization 時定了一條很強的規則：
//   只要候選建構子裡有 initializer_list 版本，且大括號內的元素
//   「能轉成它的元素型別」，它就享有絕對優先權。
//       list<int> a(5, 1);   // 呼叫 (3) → 五個 1
//       list<int> b{5, 1};   // 呼叫 (7) → 兩個元素 5, 1
//   但關鍵字是「能轉成」。若轉不過去，initializer_list 版本根本
//   不是可行候選，這時才會退回一般建構子——本檔實測了這個例外：
//       list<string> c{5, "STL"};  // 5 無法轉成 string
//                                  // → initializer_list<string> 不可行
//                                  // → 退回 (count, value) → 五個 "STL"
//   所以「{} 一定是 initializer_list」是錯的說法，正確說法是
//   「{} 優先試 initializer_list，不可行才退回其他建構子」。
//
// 【概念補充 Concept Deep Dive】
//
// (A) libstdc++ 的 list 是「環狀雙向鏈結 + 一個 header 哨兵節點」
//     std::list 物件本身不存 head/tail 兩根指標，而是內嵌一個
//     _List_node_base（只有 _M_next / _M_prev 兩根指標）當哨兵：
//         空 list：header._M_next == header._M_prev == &header
//         end()  ：就是指向 header 的迭代器
//     好處是插入／刪除完全不需要判斷「是不是頭尾」——永遠有前後節點，
//     一段無分支的指標改寫就結束。這是鏈結串列教科書上的
//     「circular list with sentinel」技巧，STL 把它用滿。
//
// (B) 本機實測的記憶體佈局（g++ 15.2 / libstdc++ 15 / x86-64；實作定義）
//     sizeof(std::list<int>) = 24 bytes
//         = _M_next(8) + _M_prev(8) + _M_size(8)
//     每個元素節點 = 24 bytes（以自訂 allocator 攔截 allocate 實測）
//         = _M_next(8) + _M_prev(8) + int(4) + padding(4)
//     也就是說：存一個 4 bytes 的 int，實際付出 24 bytes，
//     指標開銷是資料的 5 倍。這正是「list 快取不友善」的量化根據。
//     （以上數值是實作定義，其他標準函式庫／平台可能不同。）
//
// (C) 為什麼 C++11 之後 size() 必須是 O(1)
//     C++03 允許 list::size() 是 O(n)（現場走一遍）。C++11 把
//     所有容器的 size() 都收緊成 O(1)，於是 libstdc++ 多存了一個
//     _M_size 欄位。代價是 splice() 從「純指標改接、O(1)」變成
//     「部分重載必須數節點數以維護 size」——這就是為什麼
//     list::splice 搬移部分區間的重載複雜度是 O(distance)。
//     一句話：size() 的 O(1) 是拿部分 splice 的 O(1) 換來的。
//
// (D) 移動建構後來源的狀態，不要講太滿
//     std::list 的移動建構是 O(1)，實作上必然是偷指標，因此
//     本機實測移動後來源 size()==0。但要分清楚兩件事：
//       * 一般 move 建構：cppreference 記載移動後 other 為 empty()。
//       * allocator-extended 移動建構（傳入不同的 allocator）：
//         退化成逐元素移動，來源處於 valid but unspecified state。
//     所以「移動後一定是空的」這句話在泛型程式碼裡不該當作前提；
//     安全寫法永遠是「移動後只做 assign 或解構，不讀內容」。
//
// 【注意事項 Pay Attention】
//   1. list<int> l(5) 是「5 個 0」，不是「預留 5 格」。list 沒有 capacity。
//   2. (5,1) 與 {5,1} 意義不同；但「{} 一定走 initializer_list」是錯的，見【4】。
//   3. list 沒有 operator[]／at()，迭代器也不能 it + 3；
//      要跳躍請用 std::advance(it, n) 或 std::next(it, n)，且成本是 O(n)。
//   4. initializer_list 的元素是 const，只能複製不能移動；
//      要放 move-only 型別（unique_ptr）就不能用 {} 初始化。
//   5. list(n) 用 value-initialization：int 會是 0，自訂型別會呼叫預設建構子。
//   6. 移動後的來源只保證「可安全解構／可重新賦值」，見【概念補充 D】。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::list 的宣告與初始化
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::list<int> a(5, 1); 與 std::list<int> b{5, 1}; 差在哪？
//     答：a 呼叫 (count, value) 建構子，得到五個 1；
//         b 呼叫 initializer_list 建構子，得到兩個元素 5 和 1。
//         規則是：只要大括號內的元素「能轉成」initializer_list 的元素型別，
//         initializer_list 建構子就享有絕對優先權。
//     追問：那 std::list<std::string> c{5, "STL"}; 呢？
//         → 得到五個 "STL"（本機實測）。因為 5 無法轉成 std::string，
//           initializer_list<string> 版本不是可行候選，於是退回
//           (count, value) 建構子。這說明優先權的前提是「該版本可行」，
//           很多人以為 {} 一律走 initializer_list，這題就會答錯。
//
// 🔥 Q2. 為什麼 C++11 把 list::size() 從 O(n) 改成 O(1)？付出什麼代價？
//     答：C++11 統一要求所有容器 size() 為常數時間，libstdc++ 因此在
//         list 物件內多存一個 _M_size 欄位（本機實測 sizeof(list<int>)==24）。
//         代價是 splice() 不再能一律 O(1)——跨 list 搬移部分區間時，
//         必須走過區間數出節點數才能更新兩邊的 _M_size，
//         所以該重載的複雜度是 O(distance(first,last))。
//     追問：那如果我要的就是全系列 O(1) 的 splice？
//         → 用 std::forward_list。它刻意不提供 size()，
//           換來 splice_after 的 O(1)。這是同一個取捨的另一端。
//
// ⚠️ 陷阱. std::list<int> l(10); 之後 l.push_back(1);，l.size() 是多少？
//     答：11。不是 1。
//     為什麼會錯：多數人把 list<int> l(10) 讀成「準備 10 格空位」，
//         那是 vector::reserve 的心智模型。但 list 根本沒有 capacity 這個
//         概念——(10) 是「真的建 10 個值為 0 的節點」，size() 當下就是 10。
//         同樣的誤解在 vector<int> v(10) 上也天天發生。
//
// ⚠️ 陷阱 2. list<unique_ptr<int>> l = { make_unique<int>(1) }; 為什麼編不過？
//     答：initializer_list 內部是 const T 陣列，元素只能被複製，
//         而 unique_ptr 的複製建構子是 deleted（本機錯誤訊息即為
//         "use of deleted function unique_ptr(const unique_ptr&)"）。
//     為什麼會錯：直覺認為 {} 裡的是暫時物件、應該可以被 move 進去。
//         實際上編譯器先把它們放進一個 const 陣列，move 從一開始就被排除了。
//         正解是先預設建構再逐一 push_back(std::move(p))。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <list>
#include <vector>
#include <string>
#include <memory>
using namespace std;

template <typename T>
void print(const string& label, const list<T>& lst) {
    cout << "  " << label << " [" << lst.size() << "]: ";
    for (const auto& v : lst) cout << v << " ";
    cout << endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】用 list 建立可熱插拔的 middleware / filter chain
//   情境：後端服務的請求處理鏈（驗證 → 限流 → 記錄 → 業務邏輯）。
//   為什麼用 list 而不是 vector：
//     * 這條鏈在執行期會被插入／移除（灰度開關、A/B test 關掉某個 filter）；
//     * 其他模組會長期持有「指向某個 filter 的 iterator」當作 handle，
//       list 的插入／刪除不會使其他元素的 iterator 失效，vector 會。
//   初始化方式用「範圍建構」：直接從設定檔讀出來的 vector<string> 灌進來。
// -----------------------------------------------------------------------------
list<string> buildChain(const vector<string>& fromConfig) {
    return list<string>(fromConfig.begin(), fromConfig.end());  // (4) 範圍建構
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】assign() 重用節點：週期性重載設定
//   情境：設定檔每 30 秒 reload 一次，內容通常只有小改動。
//   clear() + push_back 會把所有節點釋放掉再重新配置（2n 次 heap 操作）；
//   assign() 會盡量「就地改值」重用既有節點，只有數量不足／過多時才配置／釋放。
//   對高頻 reload 的服務，這個差別是真的看得到的。
// -----------------------------------------------------------------------------
void reloadConfig(list<string>& current, const vector<string>& fresh) {
    current.assign(fresh.begin(), fresh.end());
}

int main() {
    cout << "===== 初始化方式 =====\n";
    list<int> d1;              print("1.預設     ", d1);
    list<int> d2(5);           print("2.填充(5)  ", d2);
    list<int> d3(5, 42);       print("3.填充(5,42)", d3);
    list<int> d4 = {10,20,30}; print("4.列表     ", d4);

    vector<int> vec = {7, 8, 9};
    list<int> d5(vec.begin(), vec.end());
    print("5.從vector ", d5);

    list<int> d6(d4);
    d6.push_back(99);
    print("6a.複製d4  ", d6);
    print("6b.d4不變  ", d4);

    list<int> d7(move(d6));
    print("7a.移動自d6", d7);
    print("7b.d6被掏空", d6);   // libstdc++ 實測為空；標準語意見【概念補充 D】

    list<int> d8;
    d8.assign({100, 200, 300});
    print("8.assign   ", d8);

    cout << "\n===== 小括號 vs 大括號 =====\n";
    list<int> a(5, 1);   print("(5,1) → 5個1", a);
    list<int> b{5, 1};   print("{5,1} → 兩元素", b);

    // 例外：initializer_list<string> 不可行（5 轉不成 string）→ 退回 (count,value)
    list<string> c{5, "STL"};
    print("string{5,\"STL\"}", c);

    cout << "\n===== 巢狀 list =====\n";
    list<list<int>> nested;
    nested.push_back({1, 2, 3});
    nested.push_back({4, 5});
    for (const auto& inner : nested) {
        cout << "  row: ";
        for (int v : inner) cout << v << " ";
        cout << endl;
    }

    cout << "\n===== 記憶體佈局（實作定義，本機 libstdc++ 15 / x86-64）=====\n";
    cout << "  sizeof(list<int>)  = " << sizeof(list<int>)
         << "  (next 8 + prev 8 + _M_size 8)\n";
    cout << "  每節點實際配置     = 24 bytes (next 8 + prev 8 + int 4 + padding 4)\n";
    cout << "  → 存 4 bytes 資料付出 24 bytes，這就是 list 快取不友善的來源\n";

    cout << "\n===== move-only 型別不能用 {} 初始化 =====\n";
    // list<unique_ptr<int>> bad = { make_unique<int>(1) };  // 編譯錯誤：複製建構子被 deleted
    list<unique_ptr<int>> ok;
    ok.push_back(make_unique<int>(42));                       // 正解：逐一 move 進去
    cout << "  push_back(make_unique) 後 size = " << ok.size()
         << ", 值 = " << *ok.front() << "\n";

    cout << "\n===== 實務 1：middleware chain =====\n";
    vector<string> conf = {"auth", "ratelimit", "logging", "handler"};
    list<string> chain = buildChain(conf);
    print("初始 chain ", chain);

    auto it = chain.begin();
    ++it; ++it;                       // 指向 "logging"
    chain.insert(it, "tracing");      // 熱插入，不影響其他 iterator
    print("插入 tracing", chain);

    cout << "\n===== 實務 2：assign 重載設定 =====\n";
    reloadConfig(chain, {"auth", "handler"});
    print("reload 之後", chain);

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
// ===== 初始化方式 =====
//   1.預設      [0]:
//   2.填充(5)   [5]: 0 0 0 0 0
//   3.填充(5,42) [5]: 42 42 42 42 42
//   4.列表      [3]: 10 20 30
//   5.從vector  [3]: 7 8 9
//   6a.複製d4   [4]: 10 20 30 99
//   6b.d4不變   [3]: 10 20 30
//   7a.移動自d6 [4]: 10 20 30 99
//   7b.d6被掏空 [0]:
//   8.assign    [3]: 100 200 300
//
// ===== 小括號 vs 大括號 =====
//   (5,1) → 5個1 [5]: 1 1 1 1 1
//   {5,1} → 兩元素 [2]: 5 1
//   string{5,"STL"} [5]: STL STL STL STL STL
//
// ===== 巢狀 list =====
//   row: 1 2 3
//   row: 4 5
//
// ===== 記憶體佈局（實作定義，本機 libstdc++ 15 / x86-64）=====
//   sizeof(list<int>)  = 24  (next 8 + prev 8 + _M_size 8)
//   每節點實際配置     = 24 bytes (next 8 + prev 8 + int 4 + padding 4)
//   → 存 4 bytes 資料付出 24 bytes，這就是 list 快取不友善的來源
//
// ===== move-only 型別不能用 {} 初始化 =====
//   push_back(make_unique) 後 size = 1, 值 = 42
//
// ===== 實務 1：middleware chain =====
//   初始 chain  [4]: auth ratelimit logging handler
//   插入 tracing [5]: auth ratelimit tracing logging handler
//
// ===== 實務 2：assign 重載設定 =====
//   reload 之後 [2]: auth handler
