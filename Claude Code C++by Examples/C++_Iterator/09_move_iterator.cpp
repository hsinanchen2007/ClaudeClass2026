// =============================================================================
//  09_move_iterator.cpp  —  std::move_iterator (C++11)
// =============================================================================
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼這個 iterator 存在?                              │
//  └────────────────────────────────────────────────────────────┘
//
//  C++11 引入「右值參考 (rvalue reference, T&&)」與「移動語意 (move semantics)」
//  的目的:對於擁有「外部資源」(heap-allocated buffer 等) 的物件,
//  搬位置時不必把資源整份「複製」一份再把舊的銷毀,而是把資源「掏」過去 —
//  舊物件變成「空殼」(moved-from state)。對 std::string、std::vector、
//  std::unique_ptr 等,move 通常是 O(1)、copy 是 O(n)。
//
//  問題來了:STL 演算法 (如 std::copy) 與容器的 range-constructor
//  「預設都走 copy」,因為它們對 *it 取出的東西當左值看待:
//
//      *dst = *src;   // 兩個都是左值 → 走 copy assignment
//
//  那要怎麼讓既有演算法走 move 而不重寫一份?
//
//  → std::move_iterator 是「Iterator Adaptor」,把底層 iterator 的 *it
//    從「左值」(T&) 包成「右值參考」(T&&)。一旦目的端拿到的是 T&&,
//    overload resolution 就會自動選 move 版本 (move-ctor / move-assign):
//
//      *dst = std::move(*src);  // ← 等同於 std::move_iterator 的效果
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、底層機制 — *it 的回傳型別                              │
//  └────────────────────────────────────────────────────────────┘
//
//  * 一般 iterator 的 reference 通常是 T&  (左值參考)
//  * move_iterator 把 reference 重新定義為 T&& (右值參考)
//
//  也就是說,std::move_iterator<It>::reference == std::iterator_traits<It>::value_type&&
//
//  解參考時的實作(概念上):
//      decltype(auto) operator*() const { return std::move(*base_); }
//
//  iterator_category 跟底層 It 一樣 (random_access、bidirectional 都行)。
//  所以 move_iterator 是「能力等同底層 + 解參考改成 rvalue」的薄包裝。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、何時使用 (When to use)                                 │
//  └────────────────────────────────────────────────────────────┘
//
//  * 「我已經不要原容器了,要把元素整批搬走」 → 用 move_iterator。
//  * 把 vector<string> 搬到另一個 vector<string>:
//      std::vector<std::string> dst(
//          std::make_move_iterator(src.begin()),
//          std::make_move_iterator(src.end()));
//  * 把 list<unique_ptr<T>> 倒進 vector<unique_ptr<T>>:
//      unique_ptr 不能 copy,只能 move → 必須用 move_iterator。
//  * 把資料從 thread-local buffer 移到 shared buffer:
//      避免不必要的記憶體拷貝。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、何時「不要」用 (When not to use)                       │
//  └────────────────────────────────────────────────────────────┘
//
//  * 來源容器之後還要繼續用 → 不要 move,會把元素掏空。
//  * 元素是 trivially-copyable (int / double / POD) → move 等同 copy,
//    沒效率差異,但會讓程式碼意圖混淆,通常不必加。
//  * 元素若沒有 move-ctor (或是 = delete),move_iterator 會 fallback
//    到 copy (因為 T&& 可以繫結到 const T& 參數)。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 五、Pitfalls (陷阱與注意事項)                              │
//  └────────────────────────────────────────────────────────────┘
//
//   1. Moved-from 物件「合法但內容未定」:
//      * 對 std::string 而言,實作面 moved-from 通常變空字串,
//        但「標準」只保證它是 valid (能 destroy、能 assign 新值),
//        不保證為空。所以 src 元素之後唯一安全的操作是「指派新值」或「銷毀」。
//   2. src 的 size() 不變:
//      * move_iterator 不會把元素從 src 移除,只把「內容掏空」。
//        若想真正釋放,還要呼叫 src.clear() 或讓 src 離開 scope。
//   3. 對 const 元素 move 沒用:
//      * std::move(const T&) 得到 const T&&,而 const T&& 不能繫結到
//        T&& 參數,於是 fallback 成 copy。對 vector<const string> 沒效。
//   4. move_iterator 不解決「演算法是否安全」的問題 — 例如
//      std::sort + move_iterator 沒有意義 (sort 會 swap 元素,不是 move 走)。
//   5. 與 std::move (演算法) 的差別:
//      * std::move(first, last, dst)  ← 演算法版,單純把 [first, last) move 到 dst
//      * std::make_move_iterator(it)  ← iterator 版,黏在 iterator 上,
//                                       適用於任何吃 iterator 的演算法 / ctor。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 六、簽章 (Signatures)                                      │
//  └────────────────────────────────────────────────────────────┘
//
//      template <class Iter> class move_iterator;        // C++11
//
//      template <class Iter>
//      move_iterator<Iter> make_move_iterator(Iter it);  // C++11 工廠函式
//
//      // 主要成員:
//      reference  operator*()  const;   // 回傳 std::move(*base())
//      Iter       base()       const;   // 取出底層 iterator
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 七、參考連結 (References)                                  │
//  └────────────────────────────────────────────────────────────┘
//
//    https://en.cppreference.com/w/cpp/iterator/move_iterator        — class
//    https://en.cppreference.com/w/cpp/iterator/make_move_iterator   — 工廠
//    https://en.cppreference.com/w/cpp/algorithm/move                — std::move 演算法版
//    https://en.cppreference.com/w/cpp/utility/move                  — std::move (cast)
//    https://cplusplus.com/reference/iterator/move_iterator/         — 簡明
// =============================================================================

/*
補充筆記：move_iterator
  - move_iterator 的核心是「位置」與「走訪能力」；iterator 不擁有元素，只是通往容器元素的操作介面。
  - 不同 iterator category 能力不同：input 只能讀一次，forward 可多次走訪，bidirectional 可退後，random access 可做加減與索引。
  - end iterator 是哨兵位置，不能解參考；所有 [begin, end) 半開區間都依賴這個規則。
  - 容器修改可能讓 iterator 失效；vector reallocation、erase、insert 的規則和 list/map 不同，不能通用直覺。
  - insert iterator、stream iterator、move iterator 都是在改變演算法如何讀寫元素，而不是改變演算法本身。
  - 自訂 iterator 要讓 value_type、reference、difference_type、iterator_category 等 traits 能被演算法理解。
  - move_iterator 解參考時產生右值，讓 copy 類演算法改成搬移元素。
  - 搬移後來源元素仍在容器中，但值通常不可再當原內容使用。
  - make_move_iterator 常用於把一段 unique_ptr 或大型 string 搬到另一個容器。
*/
#include <iostream>
#include <iterator>
#include <vector>
#include <list>
#include <string>
#include <memory>
#include <algorithm>

int main() {
    // -----------------------------------------------------------------------
    // 範例 1:把 vector<string> 用 move 搬到另一個 vector
    //
    // 觀察重點:
    //   * dst 的 ctor 接受兩個 InputIterator,本來會走 copy。
    //   * make_move_iterator 包了之後,*it 變 rvalue ref → 走 move-ctor。
    //   * src 的元素內容被掏空 (字串通常變空,但標準不保證)。
    //   * src.size() 仍是 3 — move_iterator 不刪除元素,只移走內容。
    // -----------------------------------------------------------------------
    std::vector<std::string> src = {
        "hello, this is a long string A",
        "hello, this is a long string B",
        "hello, this is a long string C",
    };

    std::vector<std::string> dst(
        std::make_move_iterator(src.begin()),
        std::make_move_iterator(src.end())
    );

    std::cout << "dst (搬完):\n";
    for (auto& s : dst) std::cout << "  [" << s << "]\n";

    std::cout << "src (被掏空, size 不變 = " << src.size() << "):\n";
    for (auto& s : src) std::cout << "  [" << s << "] (空字串或實作定義)\n";

    // -----------------------------------------------------------------------
    // 範例 2:和 std::copy 搭配 — 等同 std::move (演算法版)
    //   std::copy(make_move_iterator(a.begin()), make_move_iterator(a.end()), dst)
    //   ≡ std::move(a.begin(), a.end(), dst)
    // -----------------------------------------------------------------------
    std::vector<std::string> a = {"x1", "x2", "x3"};
    std::vector<std::string> b;
    b.reserve(a.size());

    std::copy(std::make_move_iterator(a.begin()),
              std::make_move_iterator(a.end()),
              std::back_inserter(b));

    std::cout << "b after move-copy: ";
    for (auto& s : b) std::cout << '[' << s << "] ";
    std::cout << '\n';

    // -----------------------------------------------------------------------
    // 範例 3:move_iterator 對「不可複製」型別 (unique_ptr) 必不可少
    //   * std::unique_ptr 沒有 copy-ctor,只有 move-ctor。
    //   * 直接用 std::copy 會編譯錯誤 (call to deleted copy ctor)。
    //   * 用 make_move_iterator 包起來就 OK。
    // -----------------------------------------------------------------------
    std::vector<std::unique_ptr<int>> ups;
    ups.push_back(std::make_unique<int>(10));
    ups.push_back(std::make_unique<int>(20));
    ups.push_back(std::make_unique<int>(30));

    std::vector<std::unique_ptr<int>> ups_dst(
        std::make_move_iterator(ups.begin()),
        std::make_move_iterator(ups.end())
    );
    std::cout << "unique_ptr 搬家後 dst: ";
    for (auto& p : ups_dst) std::cout << *p << ' ';   // 10 20 30
    std::cout << '\n';
    std::cout << "原 ups 第一個是否為 nullptr? "
              << std::boolalpha << (ups[0] == nullptr) << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_1470_shuffle_the_array();
    void practical_collect_logs_from_threads();
    void leetcode_2418_sort_people_by_height();
    void practical_move_unique_ptrs();
    leetcode_1470_shuffle_the_array();
    practical_collect_logs_from_threads();
    leetcode_2418_sort_people_by_height();
    practical_move_unique_ptrs();

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:std::move_iterator 跟 std::move (utility cast) 跟 std::move (algorithm) 三者怎麼分?
    //    A:std::move (utility,<utility>) 是「型別轉換」,把左值 cast 成右值參考,
    //      O(1) 不做任何搬動。std::move (algorithm,<algorithm>) 是「演算法」,
    //      把 [first, last) 的元素逐一 move 到 dst,等同 std::copy + move_iterator。
    //      std::move_iterator 是「iterator adaptor」,黏在 iterator 上把 *it 變
    //      右值,讓任何吃 iterator 的演算法或 ctor 自動走 move 路徑。
    //
    //  Q2:什麼時候非用 move_iterator 不可?
    //    A:當元素是「不可複製」型別 (例如 std::unique_ptr、std::thread、std::ifstream)
    //      時,std::copy 會編譯失敗 (call to deleted copy ctor)。這時必須用
    //      make_move_iterator 包起來,演算法才會選 move-ctor。對 string、vector
    //      這類「可複製但 move 較便宜」的型別,則是「效能優化」(把 O(n) 變 O(1))。
    //
    //  Q3:用 move_iterator 搬完後,來源容器能不能繼續用?
    //    A:來源容器仍然存活、size() 不變,但每個元素都進入「moved-from」狀態 —
    //      合法但內容未定義 (對 string 通常變空字串,但標準不保證)。安全的後續
    //      操作只有「指派新值」、「銷毀」或呼叫「無前置條件的成員」(如 size())。
    //      不要再讀元素內容。要徹底回收請呼叫 src.clear() 或讓 src 離開 scope。
    //

    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 1470: Shuffle the Array (洗牌陣列)
// ----------------------------------------------------------------
// 題目:給定 nums = [x1,x2,..,xn, y1,y2,..,yn] (長度 2n),
//      回傳 [x1,y1, x2,y2, ..., xn,yn]。
//
// 為什麼這題用 move_iterator 適合示範:
//   * 雖然原題是整數 (int 上 move == copy,沒效能差),這裡示範
//     若元素是 string / 大型物件,用 std::move 把元素「搬」到答案陣列,
//     避免兩次 copy。
//   * push_back(std::move(x)) 是最常見的「就地 move 到 vector 尾端」慣用語,
//     等價於 *back_inserter = std::move(x)。
//
// 解題思路:
//   * 走 i = 0..n-1,先放第 i 個 (前半),再放第 i+n 個 (後半)。
//   * 用 std::move(nums[i]) 把元素轉為 rvalue,觸發 vector 的 move-ctor。
//
// 複雜度:時間 O(n);空間 O(n) (答案陣列)。
void leetcode_1470_shuffle_the_array() {
    auto shuffle = [](std::vector<std::string> nums, int n) {
        std::vector<std::string> ans;
        ans.reserve(2 * n);
        for (int i = 0; i < n; ++i) {
            ans.push_back(std::move(nums[i]));         // move 第 i 個 (前半)
            ans.push_back(std::move(nums[i + n]));     // move 第 i+n 個 (後半)
        }
        return ans;
    };

    std::vector<std::string> nums = {"a1","a2","a3","b1","b2","b3"};
    auto ans = shuffle(nums, 3);
    std::cout << "LC1470 shuffle: ";
    for (auto& s : ans) std::cout << s << ' ';        // a1 b1 a2 b2 a3 b3
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:把多個 thread-local 訊息 buffer 合併到中央 log
// ----------------------------------------------------------------
// 場景:
//   每個 worker thread 有自己的 std::vector<std::string> 暫存 log。
//   收尾時要把它們搬到中央 log,且 worker 不再使用自己的 buffer。
//   每筆 log 是長字串,如果用 copy,代價是 O(總 byte 數);
//   用 move_iterator,代價是 O(條數) (每條只是搬指標)。
//
// 重點:
//   * 用 vector::insert 接受 move_iterator,直接 append 整段。
//   * 搬完後 worker.clear() 釋放 size。
void practical_collect_logs_from_threads() {
    std::vector<std::string> worker1 = {
        "[w1] 2026-05-04 10:00:01 INFO  startup",
        "[w1] 2026-05-04 10:00:02 WARN  retry",
    };
    std::vector<std::string> worker2 = {
        "[w2] 2026-05-04 10:00:03 INFO  job done",
        "[w2] 2026-05-04 10:00:04 ERROR timeout",
    };

    std::vector<std::string> central;
    // insert + move_iterator:把 worker1/worker2 的字串內容「搬」進 central
    central.insert(central.end(),
                   std::make_move_iterator(worker1.begin()),
                   std::make_move_iterator(worker1.end()));
    central.insert(central.end(),
                   std::make_move_iterator(worker2.begin()),
                   std::make_move_iterator(worker2.end()));
    worker1.clear();
    worker2.clear();

    std::cout << "central log size = " << central.size() << " (期望 4)\n";
    std::cout << "central[0] = " << central.front() << '\n';
    std::cout << "central[3] = " << central.back()  << '\n';
}

// ----------------------------------------------------------------
// LeetCode 2418: Sort the People (依身高排序名字)
// ----------------------------------------------------------------
// 題目:給 names (string) 與 heights (int) 兩個對應陣列,把 names 依 heights 降序排列。
// 為什麼用 move_iterator:排好序之後要把 names 從中間結構 (vector<pair<int,string>>)
// 搬到結果陣列。每個 name 是 string,move 比 copy 便宜得多 — 1000 個長字串用 move
// 是搬指標,O(1) 一個;copy 是逐字元拷貝,可能 O(每串長度)。
void leetcode_2418_sort_people_by_height() {
    std::vector<std::string> names{"Mary","John","Emma"};
    std::vector<int>         heights{180, 165, 170};
    // 1) 配對
    std::vector<std::pair<int,std::string>> people;
    people.reserve(names.size());
    for (size_t i = 0; i < names.size(); ++i)
        people.emplace_back(heights[i], std::move(names[i]));
    // 2) 依 height 降序
    std::sort(people.begin(), people.end(),
              [](auto& a, auto& b){ return a.first > b.first; });
    // 3) 把名字 move 出來
    std::vector<std::string> sorted_names;
    sorted_names.reserve(people.size());
    for (auto& [h, n] : people) sorted_names.push_back(std::move(n));
    std::cout << "LC2418 sort by height desc: ";
    for (auto& n : sorted_names) std::cout << n << ' ';
    std::cout << "(期望 Mary Emma John)\n";
}

// ----------------------------------------------------------------
// 實務範例:把 worker 暫存的 unique_ptr 任務全部搬到主執行緒佇列
// ----------------------------------------------------------------
// 場景:Thread pool 每個 worker 自己存 vector<unique_ptr<Task>>,結尾時要把所有
// 任務「移交」給主執行緒。unique_ptr 不可拷貝、只可 move,因此一定要用
// move_iterator 才能整段塞進主佇列。
void practical_move_unique_ptrs() {
    struct Task { int id; };
    std::vector<std::unique_ptr<Task>> worker;
    worker.push_back(std::make_unique<Task>(Task{101}));
    worker.push_back(std::make_unique<Task>(Task{102}));
    worker.push_back(std::make_unique<Task>(Task{103}));

    std::vector<std::unique_ptr<Task>> central;
    central.insert(central.end(),
                   std::make_move_iterator(worker.begin()),
                   std::make_move_iterator(worker.end()));
    worker.clear();
    std::cout << "central tasks: ";
    for (auto& p : central) std::cout << p->id << ' ';
    std::cout << "(期望 101 102 103)\n";
}

// === 預期輸出 (Expected output) ===
// dst (搬完):
//   [hello, this is a long string A]
//   [hello, this is a long string B]
//   [hello, this is a long string C]
// src (被掏空, size 不變 = 3):
//   [] (空字串或實作定義)
//   [] (空字串或實作定義)
//   [] (空字串或實作定義)
// b after move-copy: [x1] [x2] [x3]
// unique_ptr 搬家後 dst: 10 20 30
// 原 ups 第一個是否為 nullptr? true
// LC1470 shuffle: a1 b1 a2 b2 a3 b3
// central log size = 4 (期望 4)
// central[0] = [w1] 2026-05-04 10:00:01 INFO  startup
// central[3] = [w2] 2026-05-04 10:00:04 ERROR timeout
