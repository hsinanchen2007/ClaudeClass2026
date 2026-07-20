// =============================================================================
//  第 38 課  —  std::forward_list：為「省一個指標」而生的單向鏈結串列
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<forward_list>
//   標準版本：**C++11 新增**（是唯一在 C++11 才加入的序列容器）。
//   迭代器類別：Forward Iterator（只能 ++，不能 --，不能 +n）
//   關鍵成員（注意全部是 *_after 系列）：
//     iterator before_begin() noexcept;              // 「第一個元素之前」的虛擬位置
//     iterator insert_after(const_iterator pos, const T& v);
//     iterator erase_after (const_iterator pos);
//     void     push_front(const T& v) / pop_front();
//     void     sort() / reverse() / unique() / remove(v) / remove_if(p) / merge(other)
//     void     splice_after(const_iterator pos, forward_list& other);
//   **刻意缺席的成員**（不是遺漏，是設計）：
//     size()、back()、push_back()、pop_back()、operator[]、rbegin()、rend()
//   複雜度：push_front / pop_front / insert_after / erase_after 皆 O(1)；
//           sort O(N log N)；remove / unique / reverse / distance 皆 O(N)。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼標準要多加一個「閹割版的 list」】
//   forward_list 的設計目標只有一個：**在空間與時間上都不輸給手寫的 C 單向串列**。
//   C++ 的核心原則是「不為你沒用到的東西付錢」，而 std::list 為了雙向走訪，
//   每個節點都要多存一個 prev 指標：
//       list<int>         節點 = prev(8) + next(8) + int(4) + padding(4) = 24 bytes
//       forward_list<int> 節點 =           next(8) + int(4) + padding(4) = 16 bytes
//   （以上為本機 x86-64 / libstdc++ 的實測值，屬實作定義。）
//   對百萬節點就是 24MB vs 16MB —— 省下三分之一。
//   如果你根本不需要往回走，那個 prev 指標就是純粹的浪費。
//
// 【2. 為什麼沒有 size()：這是最有爭議也最能體現設計哲學的決定】
//   要提供 O(1) 的 size()，容器就得存一個 size_t 成員（8 bytes）並在每次
//   插入/刪除時維護它。對「只有一個 head 指標」的 forward_list 而言，
//   這等於讓容器物件本身大一倍，且讓 splice_after 從 O(1) 退化成 O(N)
//   （因為要數出搬了幾個元素才能更新兩邊的 size）。
//   標準委員會的選擇是：**寧可不提供，也不要讓不需要它的人付錢**。
//   需要元素個數時自己算：
//       auto n = std::distance(fl.begin(), fl.end());   // O(N)，明確標示成本
//   這個 API 設計的訊息很清楚：「這是 O(N)，你確定要嗎？」
//   （對比之下，C++11 起 std::list::size() 被規定必須是 O(1)，
//     因為 list 已經夠肥了，多存一個 size 相對划算。）
//
// 【3. 為什麼所有操作都是 *_after：單向串列的必然結果】
//   要在單向串列中「於 pos 之前插入」，必須先找到 pos 的**前一個**節點
//   才能改它的 next —— 而單向串列沒有 prev，只能從頭走一遍，O(N)。
//   於是標準乾脆把介面改成「在 pos **之後**插入」，這樣只要改 pos->next，O(1)。
//   代價是「在最前面插入」變得無處著力 —— 因此有了 before_begin()。
//
// 【4. before_begin() 是什麼：一個不可解參考的虛擬位置】
//   before_begin() 回傳「第一個元素之前」的迭代器，用來讓
//   insert_after(before_begin(), v) 能在頭端插入。
//   它有兩條鐵律：
//     (a) **絕不可解參考**（*fl.before_begin() 是未定義行為）——
//         它指向的是一個 dummy 頭節點，裡面沒有有效的元素
//     (b) 它與 end() 完全不同，且 ++before_begin() == begin()
//   這是半開區間 [begin, end) 之外，STL 少數需要「區間之前」概念的地方。
//
// 【概念補充 Concept Deep Dive】
//   forward_list 真的比 list 快嗎？答案比直覺複雜。
//   **空間**：確定省 1/3（16 vs 24 bytes/節點，本機實測）。
//   **時間**：不一定更快，甚至常常一樣或更慢：
//     - 走訪：兩者都是 pointer chasing，都會 cache miss。
//       forward_list 節點較小 → 同一條 cache line 塞得下更多節點的機率略高，
//       但因為節點各自配置、位址分散，這個優勢在實務上往往被稀釋。
//     - 插入/刪除：兩者都是 O(1)（前提是已握有正確位置的迭代器）。
//     - 但 forward_list 少了 prev，某些操作反而更麻煩：
//       想刪除「it 指向的元素」必須握有它的**前一個**迭代器，
//       否則只能從頭再走一遍 O(N)。list 則直接 erase(it) 即可。
//   結論：選 forward_list 的理由主要是**記憶體**（節點數以百萬計、
//   或嵌入式環境），而不是速度。若不確定，list 幾乎總是更好用；
//   而如果連續性可以接受，vector 通常又比兩者都快得多。
//
// 【注意事項 Pay Attention】
//   1. **沒有 size()** —— 要算個數請用 std::distance(begin, end)，這是 O(N)。
//   2. **沒有 back() / push_back() / pop_back()** —— 尾端操作需要走到最後，O(N)。
//   3. **沒有 rbegin() / rend()** —— Forward Iterator 不能倒著走。
//   4. *fl.before_begin() 是未定義行為，它只能當作 insert_after / erase_after 的位置參數。
//   5. erase_after(pos) 刪的是 **pos 的下一個**，不是 pos 本身 ——
//      這是最容易寫錯的地方（想刪第一個要寫 erase_after(before_begin())）。
//   6. unique() 只移除**相鄰**的重複元素，未排序的資料要先 sort()。
//   7. 迭代器失效規則與 list 相同：除了被刪的節點，其餘全部保持有效。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::forward_list
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. forward_list 為什麼不提供 size()？
//     答：因為要 O(1) 的 size() 就得在容器內存一個計數器並隨時維護，
//         這會讓容器物件變大，還會讓 splice_after 從 O(1) 退化成 O(N)
//         （得先數出搬移了幾個元素）。forward_list 的存在目的就是
//         「在空間與時間上不輸手寫 C 單向串列」，所以標準選擇不提供，
//         需要時請自己用 std::distance()，成本 O(N) 一目了然。
//     追問：那 std::list 的 size() 是 O(1) 還是 O(N)？
//           → C++11 起標準**要求**必須是 O(1)（C++03 時代兩種實作都存在）。
//             因為 list 節點已經有 prev/next 兩個指標了，
//             再多存一個 size 相對划算，取捨結論與 forward_list 相反。
//
// 🔥 Q2. 為什麼 forward_list 的插入刪除都叫 insert_after / erase_after？
//     答：單向串列只有 next 指標。要「在 pos 之前插入」得先找到 pos 的前一個節點，
//         而那需要從頭走一遍，O(N)。改成「在 pos 之後插入」則只要改 pos->next，
//         O(1)。標準因此把整套介面設計成 *_after，讓複雜度保證是誠實的。
//     追問：那要在最前面插入怎麼辦？
//           → 用 before_begin()：insert_after(fl.before_begin(), v)。
//             它是一個指向 dummy 頭節點的虛擬位置，**絕不可解參考**，
//             只能當作 insert_after / erase_after 的位置參數。
//
// ⚠️ 陷阱. auto it = fl.begin(); fl.erase_after(it); —— 這行刪掉了哪個元素？
//     答：刪掉的是**第二個**元素，不是第一個。
//         erase_after(pos) 的語意是「刪除 pos 的下一個」，pos 本身完好無損。
//         要刪除第一個元素必須寫 fl.erase_after(fl.before_begin())，
//         或直接用 fl.pop_front()。
//     為什麼會錯：其他容器的 erase(it) 都是「刪掉 it 指的那個」，
//         直覺會把 erase_after 也讀成同樣語意。
//         這個 off-by-one 特別陰險 —— 程式不會崩潰，只是安靜地刪錯元素，
//         而且在「刪除條件恰好連續成立」的測試資料下常常看不出來。
//         記憶法：函式名結尾的 _after 就是在提醒你「動的是後面那個」。
// ═══════════════════════════════════════════════════════════════════════════

// lesson38_forward_list_intro.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -o lesson38 lesson38_forward_list_intro.cpp

#include <iostream>
#include <forward_list>
#include <list>
#include <vector>
#include <string>
#include <iterator>    // std::distance
using namespace std;

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 707. Design Linked List
//   題目：實作一個單向鏈結串列，支援 get(index) / addAtHead / addAtTail /
//         addAtIndex / deleteAtIndex。
//   為什麼用到本主題：這題要求的正是 forward_list 的語意，
//         而用 forward_list 實作能把本課的每個重點都用上：
//           - addAtHead     → push_front（O(1)）
//           - addAtIndex(0) → insert_after(before_begin())，示範 before_begin 的用途
//           - addAtTail     → 必須走到最後，**O(N)** —— 正好說明
//                             「為什麼 forward_list 不提供 push_back」
//           - deleteAtIndex → erase_after(前一個)，示範 _after 語意的 off-by-one
//           - get / 長度檢查 → std::distance，示範「沒有 size() 的代價」
//         換句話說，這題就是一份 forward_list 設計取捨的實戰說明書。
//   複雜度：addAtHead O(1)；其餘皆 O(index) 或 O(N)。
// -----------------------------------------------------------------------------
class MyLinkedList {
    forward_list<int> data_;
    int               size_ = 0;   // 自己維護長度，正是因為 forward_list 沒有 size()

public:
    MyLinkedList() = default;

    int get(int index) {
        if (index < 0 || index >= size_) return -1;
        auto it = data_.begin();
        advance(it, index);        // Forward Iterator 只能一步一步走，O(index)
        return *it;
    }

    void addAtHead(int val) {
        data_.push_front(val);     // O(1) —— forward_list 最擅長的操作
        ++size_;
    }

    void addAtTail(int val) {
        // 沒有 push_back：必須走到最後一個節點，O(N)
        auto pos = data_.before_begin();
        for (auto it = data_.begin(); it != data_.end(); ++it) pos = it;
        data_.insert_after(pos, val);
        ++size_;
    }

    void addAtIndex(int index, int val) {
        if (index < 0 || index > size_) return;
        // 走到「第 index 個的前一個」；index==0 時就是 before_begin()
        auto pos = data_.before_begin();
        for (int i = 0; i < index; ++i) ++pos;
        data_.insert_after(pos, val);
        ++size_;
    }

    void deleteAtIndex(int index) {
        if (index < 0 || index >= size_) return;
        // 同樣要握有「前一個」才能刪 —— 這是單向串列的本質限制
        auto pos = data_.before_begin();
        for (int i = 0; i < index; ++i) ++pos;
        data_.erase_after(pos);    // 刪的是 pos 的下一個
        --size_;
    }

    // 教學用：印出目前內容
    string dump() const {
        string s;
        for (int v : data_) s += to_string(v) + " ";
        return s.empty() ? "(空)" : s;
    }
    int size() const { return size_; }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】雜湊表的分離鏈結（separate chaining）桶
//   情境：自己實作 hash map 時，每個桶需要一條「衝突鏈」。
//         桶的數量可能上百萬個，而每條鏈通常只有 0~2 個元素。
//   為什麼用到本主題：這是 forward_list 最正當的用途，理由有三：
//     (1) 桶只需要單向走訪（找到就回傳），不需要往回走
//     (2) 節點省下 prev 指標 → 百萬桶就是省下數 MB
//     (3) 插入一律在頭端（push_front，O(1)），符合「最近插入最可能被查」的直覺
//   這也正是 libstdc++ 的 std::unordered_map 內部採用單向串列的原因。
// -----------------------------------------------------------------------------
class SimpleHashSet {
    static const size_t BUCKETS = 8;
    forward_list<string> buckets_[BUCKETS];

    size_t bucketOf(const string& key) const {
        size_t h = 0;
        for (char c : key) h = h * 131 + static_cast<unsigned char>(c);
        return h % BUCKETS;
    }

public:
    void insert(const string& key) {
        auto& chain = buckets_[bucketOf(key)];
        // 先查是否已存在（單向走訪就夠）
        for (const string& s : chain) if (s == key) return;
        chain.push_front(key);          // O(1)，插在鏈頭
    }

    bool contains(const string& key) const {
        const auto& chain = buckets_[bucketOf(key)];
        for (const string& s : chain) if (s == key) return true;
        return false;
    }

    void erase(const string& key) {
        auto& chain = buckets_[bucketOf(key)];
        // 單向串列刪除：必須握有「前一個」，所以從 before_begin 開始走
        auto prev_it = chain.before_begin();
        for (auto it = chain.begin(); it != chain.end(); ++it) {
            if (*it == key) { chain.erase_after(prev_it); return; }
            prev_it = it;
        }
    }

    void dumpBuckets() const {
        for (size_t i = 0; i < BUCKETS; ++i) {
            size_t n = static_cast<size_t>(
                distance(buckets_[i].begin(), buckets_[i].end()));
            if (n == 0) continue;
            cout << "    bucket[" << i << "] (" << n << "): ";
            for (const string& s : buckets_[i]) cout << s << " ";
            cout << endl;
        }
    }
};

template <typename T>
void print_flist(const string& label, const forward_list<T>& flst) {
    cout << label << ": ";
    for (const auto& val : flst) cout << val << " ";
    // 注意：不能用 flst.size()
    cout << "(元素數: " << distance(flst.begin(), flst.end()) << ")" << endl;
}

int main() {
    // ===== 1. 初始化方式 =====
    cout << "===== 初始化方式 =====" << endl;
    {
        forward_list<int> fl1;                        // 空
        forward_list<int> fl2(5, 42);                 // 5 個 42
        forward_list<int> fl3 = {10, 20, 30, 40, 50}; // 初始化列表
        forward_list<int> fl4(fl3);                   // 複製建構
        forward_list<int> fl5(fl3.begin(), fl3.end()); // 範圍建構

        print_flist("fl1（空）    ", fl1);
        print_flist("fl2（填充）  ", fl2);
        print_flist("fl3（列表）  ", fl3);
        print_flist("fl4（複製）  ", fl4);
        print_flist("fl5（範圍）  ", fl5);
    }

    // ===== 2. 元素存取 =====
    cout << "\n===== 元素存取 =====" << endl;
    {
        forward_list<int> flst = {10, 20, 30};

        cout << "front(): " << flst.front() << endl;
        // cout << flst.back();     // ✗ 編譯錯誤！
        // cout << flst[0];         // ✗ 編譯錯誤！
        // cout << flst.size();     // ✗ 編譯錯誤！
        cout << "empty(): " << (flst.empty() ? "是" : "否") << endl;
    }

    // ===== 3. push_front / pop_front =====
    cout << "\n===== push_front / pop_front =====" << endl;
    {
        forward_list<int> flst;

        flst.push_front(30);
        flst.push_front(20);
        flst.push_front(10);
        print_flist("push_front x3", flst);   // 10 20 30

        flst.pop_front();
        print_flist("pop_front    ", flst);   // 20 30

        // push_back 不存在！
        // flst.push_back(40);    // ✗ 編譯錯誤
    }

    // ===== 4. before_begin 的用法 =====
    cout << "\n===== before_begin =====" << endl;
    {
        forward_list<int> flst = {20, 30, 40};
        print_flist("初始      ", flst);

        // 在最前面插入（用 before_begin）
        flst.insert_after(flst.before_begin(), 10);
        print_flist("頭端插入  ", flst);   // 10 20 30 40

        // 用 emplace_after 在頭端建構
        flst.emplace_after(flst.before_begin(), 5);
        print_flist("頭端emplace", flst);   // 5 10 20 30 40
    }

    // ===== 5. insert_after 各種重載 =====
    cout << "\n===== insert_after =====" << endl;
    {
        forward_list<int> flst = {10, 30, 50};
        print_flist("初始            ", flst);

        // 5a. 在第一個元素（10）之後插入 20
        auto it = flst.begin();    // 指向 10
        flst.insert_after(it, 20);
        print_flist("insert_after(20)", flst);  // 10 20 30 50

        // 5b. 插入多個相同值
        it = flst.begin();
        advance(it, 3);           // 指向 50
        flst.insert_after(it, 3, 60);
        print_flist("insert_after x3 ", flst);  // 10 20 30 50 60 60 60

        // 5c. 插入初始化列表
        flst.insert_after(flst.before_begin(), {1, 2, 3});
        print_flist("insert_after{}  ", flst);  // 1 2 3 10 20 30 50 60 60 60
    }

    // ===== 6. erase_after =====
    cout << "\n===== erase_after =====" << endl;
    {
        forward_list<int> flst = {10, 20, 30, 40, 50, 60};
        print_flist("初始             ", flst);

        // 6a. 刪除第一個元素之後的元素（= 刪除 20）
        flst.erase_after(flst.begin());
        print_flist("erase_after(begin)", flst);  // 10 30 40 50 60

        // 6b. 刪除範圍 (pos, last)
        // 刪除 30 和 40（即 begin 之後到 50 之前）
        auto it = flst.begin();      // 指向 10
        auto last = it;
        advance(last, 3);            // 指向 50
        flst.erase_after(it, last);
        print_flist("erase_after範圍  ", flst);  // 10 50 60

        // 6c. 刪除第一個元素（用 before_begin）
        flst.erase_after(flst.before_begin());
        print_flist("刪除第一個       ", flst);  // 50 60
    }

    // ===== 7. 特有成員函數 =====
    cout << "\n===== 特有成員函數 =====" << endl;
    {
        // sort
        forward_list<int> flst = {5, 3, 8, 1, 9, 2};
        print_flist("排序前  ", flst);
        flst.sort();
        print_flist("sort 後 ", flst);

        // reverse
        flst.reverse();
        print_flist("reverse ", flst);

        // unique（需要先排序）
        forward_list<int> flst2 = {1, 1, 2, 3, 3, 3, 4};
        flst2.unique();
        print_flist("unique  ", flst2);

        // remove
        forward_list<int> flst3 = {1, 2, 3, 2, 4, 2, 5};
        flst3.remove(2);
        print_flist("remove(2)", flst3);

        // remove_if
        forward_list<int> flst4 = {1, 2, 3, 4, 5, 6, 7, 8};
        flst4.remove_if([](int x) { return x % 2 == 0; });
        print_flist("remove偶數", flst4);

        // merge
        forward_list<int> a = {1, 3, 5};
        forward_list<int> b = {2, 4, 6};
        a.merge(b);
        print_flist("merge   ", a);
        print_flist("b(空了) ", b);
    }

    // ===== 8. 遍歷方式 =====
    cout << "\n===== 遍歷方式 =====" << endl;
    {
        forward_list<int> flst = {10, 20, 30, 40, 50};

        // 方式 1：範圍 for
        cout << "範圍 for：";
        for (int val : flst) cout << val << " ";
        cout << endl;

        // 方式 2：迭代器
        cout << "迭代器：  ";
        for (auto it = flst.begin(); it != flst.end(); ++it) {
            cout << *it << " ";
        }
        cout << endl;

        // ✗ 不能反向遍歷！
        // for (auto rit = flst.rbegin(); rit != flst.rend(); ++rit)
        //     → 編譯錯誤！forward_list 沒有 rbegin/rend

        // ✗ 不能用 --it
        // auto it = flst.end();
        // --it;    → 編譯錯誤！Forward Iterator 不支援 --
    }

    // ===== 9. 記憶體對比 =====
    cout << "\n===== 記憶體對比 =====" << endl;
    {
        cout << "sizeof(forward_list<int>): " << sizeof(forward_list<int>) << " bytes" << endl;
        cout << "sizeof(list<int>):         " << sizeof(list<int>) << " bytes" << endl;
        cout << "sizeof(vector<int>):       " << sizeof(vector<int>) << " bytes" << endl;

        // 估算 10 萬個 int 的記憶體
        const int N = 100000;

        // forward_list: 每個節點 = next(8) + int(4) + padding(4) = 16 bytes
        // list:         每個節點 = prev(8) + next(8) + int(4) + padding(4) = 24 bytes
        // vector:       N × 4 bytes（連續）

        cout << "\n" << N << " 個 int 的估算記憶體：" << endl;
        cout << "  forward_list: ~" << (N * 16) / 1024 << " KB" << endl;
        cout << "  list:         ~" << (N * 24) / 1024 << " KB" << endl;
        cout << "  vector:       ~" << (N * 4) / 1024 << " KB" << endl;
    }

    // ===== 10. 迭代器失效規則 =====
    cout << "\n===== 迭代器失效規則 =====" << endl;
    {
        forward_list<int> flst = {10, 20, 30, 40, 50};

        auto it20 = next(flst.begin());    // 指向 20
        auto it40 = next(it20, 2);         // 指向 40

        cout << "操作前: *it20=" << *it20 << " *it40=" << *it40 << endl;

        // insert_after 不影響任何迭代器
        flst.insert_after(flst.begin(), 15);

        cout << "insert後: *it20=" << *it20 << " *it40=" << *it40 << endl;

        // push_front 不影響任何迭代器
        flst.push_front(5);

        cout << "push後: *it20=" << *it20 << " *it40=" << *it40 << endl;

        // erase_after 只影響被刪的
        auto it_before_30 = next(flst.begin(), 3);  // 指向 20
        flst.erase_after(it_before_30);              // 刪除 30

        cout << "erase後: *it20=" << *it20 << " *it40=" << *it40 << endl;

        print_flist("最終結果", flst);
        cout << "→ 規則和 list 完全一致：除了被刪的，其餘全有效" << endl;
    }

    // ===== 11. erase_after 的 off-by-one（最常見的錯誤）=====
    cout << "\n===== erase_after 刪的是「下一個」 =====" << endl;
    {
        forward_list<int> a = {10, 20, 30, 40};
        a.erase_after(a.begin());              // 刪掉 begin 的下一個 = 20
        print_flist("erase_after(begin())    ", a);
        cout << "  → 刪掉的是 20（第二個），不是 10" << endl;

        forward_list<int> b = {10, 20, 30, 40};
        b.erase_after(b.before_begin());       // 這才是刪第一個
        print_flist("erase_after(before_begin)", b);
        cout << "  → 想刪第一個，位置要傳 before_begin()（或直接 pop_front()）" << endl;
    }

    // ===== 12. before_begin 不可解參考 =====
    cout << "\n===== before_begin() 的性質 =====" << endl;
    {
        forward_list<int> f = {1, 2, 3};
        cout << "  ++before_begin() == begin()? "
             << (next(f.before_begin()) == f.begin() ? "是" : "否") << endl;
        cout << "  *before_begin() 是未定義行為 —— 本檔刻意不執行" << endl;
        cout << "  它只能當作 insert_after / erase_after 的位置參數" << endl;
    }

    // ===== 13. LeetCode 707 =====
    cout << "\n===== LeetCode 707. Design Linked List =====" << endl;
    {
        MyLinkedList ll;
        ll.addAtHead(1);          cout << "  addAtHead(1)     → " << ll.dump() << endl;
        ll.addAtTail(3);          cout << "  addAtTail(3)     → " << ll.dump() << endl;
        ll.addAtIndex(1, 2);      cout << "  addAtIndex(1,2)  → " << ll.dump() << endl;
        cout << "  get(1)           = " << ll.get(1) << endl;
        ll.deleteAtIndex(1);      cout << "  deleteAtIndex(1) → " << ll.dump() << endl;
        cout << "  get(1)           = " << ll.get(1) << endl;
        cout << "  get(99)          = " << ll.get(99) << "  （越界回傳 -1）" << endl;
        cout << "  → addAtHead 是 O(1)，但 addAtTail 必須走到底 O(N)：" << endl;
        cout << "    這正是 forward_list 不提供 push_back 的原因" << endl;
    }

    // ===== 14. 實務：雜湊表的分離鏈結桶 =====
    cout << "\n===== 日常實務：雜湊表的分離鏈結（separate chaining）=====" << endl;
    {
        SimpleHashSet hs;
        for (const char* k : {"apple", "banana", "cherry", "date",
                              "elderberry", "fig", "grape"}) {
            hs.insert(k);
        }
        cout << "  插入 7 個字串後，各桶內容：" << endl;
        hs.dumpBuckets();

        cout << "  contains(\"cherry\") = " << (hs.contains("cherry") ? "true" : "false") << endl;
        cout << "  contains(\"kiwi\")   = " << (hs.contains("kiwi") ? "true" : "false") << endl;

        hs.erase("cherry");
        cout << "  erase(\"cherry\") 之後 contains = "
             << (hs.contains("cherry") ? "true" : "false") << endl;
        cout << "  → 每個桶只需單向走訪，省下 prev 指標；" << endl;
        cout << "    百萬個桶時這就是數 MB 的差別（libstdc++ 的 unordered_map 正是這樣做）"
             << endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 38 課：forward_list 單向鏈結串列介紹1.cpp -o lesson38

// === 預期輸出 ===
// ===== 初始化方式 =====
// fl1（空）    : (元素數: 0)
// fl2（填充）  : 42 42 42 42 42 (元素數: 5)
// fl3（列表）  : 10 20 30 40 50 (元素數: 5)
// fl4（複製）  : 10 20 30 40 50 (元素數: 5)
// fl5（範圍）  : 10 20 30 40 50 (元素數: 5)
//
// ===== 元素存取 =====
// front(): 10
// empty(): 否
//
// ===== push_front / pop_front =====
// push_front x3: 10 20 30 (元素數: 3)
// pop_front    : 20 30 (元素數: 2)
//
// ===== before_begin =====
// 初始      : 20 30 40 (元素數: 3)
// 頭端插入  : 10 20 30 40 (元素數: 4)
// 頭端emplace: 5 10 20 30 40 (元素數: 5)
//
// ===== insert_after =====
// 初始            : 10 30 50 (元素數: 3)
// insert_after(20): 10 20 30 50 (元素數: 4)
// insert_after x3 : 10 20 30 50 60 60 60 (元素數: 7)
// insert_after{}  : 1 2 3 10 20 30 50 60 60 60 (元素數: 10)
//
// ===== erase_after =====
// 初始             : 10 20 30 40 50 60 (元素數: 6)
// erase_after(begin): 10 30 40 50 60 (元素數: 5)
// erase_after範圍  : 10 50 60 (元素數: 3)
// 刪除第一個       : 50 60 (元素數: 2)
//
// ===== 特有成員函數 =====
// 排序前  : 5 3 8 1 9 2 (元素數: 6)
// sort 後 : 1 2 3 5 8 9 (元素數: 6)
// reverse : 9 8 5 3 2 1 (元素數: 6)
// unique  : 1 2 3 4 (元素數: 4)
// remove(2): 1 3 4 5 (元素數: 4)
// remove偶數: 1 3 5 7 (元素數: 4)
// merge   : 1 2 3 4 5 6 (元素數: 6)
// b(空了) : (元素數: 0)
//
// ===== 遍歷方式 =====
// 範圍 for：10 20 30 40 50
// 迭代器：  10 20 30 40 50
//
// ===== 記憶體對比 =====
// sizeof(forward_list<int>): 8 bytes
// sizeof(list<int>):         24 bytes
// sizeof(vector<int>):       24 bytes
//
// 100000 個 int 的估算記憶體：
//   forward_list: ~1562 KB
//   list:         ~2343 KB
//   vector:       ~390 KB
//
// ===== 迭代器失效規則 =====
// 操作前: *it20=20 *it40=40
// insert後: *it20=20 *it40=40
// push後: *it20=20 *it40=40
// erase後: *it20=20 *it40=40
// 最終結果: 5 10 15 20 40 50 (元素數: 6)
// → 規則和 list 完全一致：除了被刪的，其餘全有效
//
// ===== erase_after 刪的是「下一個」 =====
// erase_after(begin())    : 10 30 40 (元素數: 3)
//   → 刪掉的是 20（第二個），不是 10
// erase_after(before_begin): 20 30 40 (元素數: 3)
//   → 想刪第一個，位置要傳 before_begin()（或直接 pop_front()）
//
// ===== before_begin() 的性質 =====
//   ++before_begin() == begin()? 是
//   *before_begin() 是未定義行為 —— 本檔刻意不執行
//   它只能當作 insert_after / erase_after 的位置參數
//
// ===== LeetCode 707. Design Linked List =====
//   addAtHead(1)     → 1
//   addAtTail(3)     → 1 3
//   addAtIndex(1,2)  → 1 2 3
//   get(1)           = 2
//   deleteAtIndex(1) → 1 3
//   get(1)           = 3
//   get(99)          = -1  （越界回傳 -1）
//   → addAtHead 是 O(1)，但 addAtTail 必須走到底 O(N)：
//     這正是 forward_list 不提供 push_back 的原因
//
// ===== 日常實務：雜湊表的分離鏈結（separate chaining）=====
//   插入 7 個字串後，各桶內容：
//     bucket[0] (1): fig
//     bucket[1] (1): cherry
//     bucket[2] (1): apple
//     bucket[3] (1): grape
//     bucket[4] (1): elderberry
//     bucket[5] (1): banana
//     bucket[6] (1): date
//   contains("cherry") = true
//   contains("kiwi")   = false
//   erase("cherry") 之後 contains = false
//   → 每個桶只需單向走訪，省下 prev 指標；
//     百萬個桶時這就是數 MB 的差別（libstdc++ 的 unordered_map 正是這樣做）
