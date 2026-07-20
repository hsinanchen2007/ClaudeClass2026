// =============================================================================
//  summary.cpp  —  list::sort：為什麼鏈結串列需要自己的排序演算法
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<list>
//   簽名：
//     void sort();                              // 用 operator<
//     template<class Compare> void sort(Compare comp);
//
//   標準保證（C++17 [list.ops]）：
//     * 穩定（stable）——相等元素的相對順序不變
//     * 約 N log N 次比較
//     * 不使指向元素的 iterator／reference／pointer 失效
//       （元素在容器中的「順序」改變，但節點本身不搬家）
//   標準「沒有」保證：
//     * 使用哪一種演算法。libstdc++ 實作為 bottom-up merge sort，
//       但那是實作選擇，不是標準要求——只是穩定 + N log N + 不失效
//       這三個條件加起來，實務上幾乎只剩 merge sort 可選。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 std::sort 不能用在 list 上——這是型別系統擋下來的】
//   std::sort 的實作是 introsort（quicksort + heapsort + insertion sort），
//   三者都需要「隨機存取」：
//     * quicksort 要取中位數樞紐 → 需要 it + n/2
//     * heapsort  要算子節點位置 → 需要 it + 2*i + 1
//     * 分割時要從兩端往中間夾  → 需要 it1 - it2 算距離
//   list 的迭代器是 BidirectionalIterator，只有 ++ / --。
//   所以 std::sort(lst.begin(), lst.end()) 不是「執行慢」，
//   而是「根本編譯不過」——這是 STL 用型別把演算法需求寫進介面的示範：
//   錯誤在編譯期就被擋住，不會變成執行期的效能地雷。
//
// 【2. merge sort 為什麼特別適合鏈結串列】
//   一般在陣列上，merge sort 的致命傷是「合併需要額外 O(n) 空間」。
//   但在鏈結串列上，合併完全不需要額外空間——只要改指標接線：
//       while (a && b) { 取較小者接到結果尾端; 該串前進; }
//   於是 merge sort 在 list 上同時擁有：
//     O(n log n) 時間、O(log n) 遞迴堆疊（bottom-up 實作甚至 O(1)）、穩定、
//     且不需要移動任何元素。
//   反過來，quicksort 在鏈結串列上很難做好（樞紐選擇退化、無法雙向夾擊），
//   所以「陣列用 quicksort、鏈結串列用 merge sort」是資料結構課的經典結論，
//   STL 的 std::sort 與 list::sort 正好各站一邊。
//
// 【3. 穩定性不是附贈的，它是可以拿來用的工具】
//   因為 sort 保證穩定，就能做「多鍵排序」而不必寫複雜的比較函式：
//       先用次要鍵排 → 再用主要鍵排
//   第二次排序不會打亂第一次的結果（相同主鍵維持原相對順序），
//   於是兩次單鍵排序 == 一次雙鍵排序。本檔第 5 節示範了這個技巧。
//   代價是 O(2 n log n) 而非 O(n log n)，但可讀性好很多，
//   而且鍵的數量增加時（三鍵、四鍵）優勢更明顯。
//
// 【4. 迭代器不失效的實際價值】
//   排序前拿到的 iterator，排序後仍指向「同一個元素」（而不是同一個位置）。
//   本檔第 4 節印出位址驗證：排序前後 &(*it) 相同。
//   這讓「其他模組長期持有元素 handle」的設計成為可能——
//   排序只是重新接線，handle 不會突然指到別人身上。
//   vector 完全沒有這個性質：std::sort 是搬移元素的值，
//   排序後同一個 iterator 指向的是「同一個位置的不同元素」。
//
// 【5. 效能：為什麼「複製到 vector 排完再搬回來」常常更快】
//   list::sort 不搬移元素，聽起來很省，但它每一次比較都要
//   解參考兩個散落在 heap 各處的節點 → 幾乎每次都是 cache miss。
//   而 vector 方案雖然多了兩次 O(n) 複製，但排序過程完全在
//   連續記憶體上進行，預取器（prefetcher）能完全發揮。
//   本檔第 6 節實測 50 萬個 int（實測值每次執行都不同，且與
//   機器、編譯最佳化等級高度相關）。
//   結論不是「list::sort 沒用」，而是：
//     * 元素小、只是要排序 → 搬到 vector 划算
//     * 元素大（複製昂貴）或必須保持 iterator 有效 → list::sort 才是對的
//
// 【概念補充 Concept Deep Dive】
//   * libstdc++ 的 list::sort 採 bottom-up merge sort：
//     維護一組「大小為 2^i 的已排序暫存 list」，每次取一個節點往上合併，
//     進位的方式很像二進位加法。這種寫法沒有遞迴，堆疊使用是 O(1)。
//   * 「不失效」與「不移動元素」是同一件事的兩面：
//     節點的位址就是元素的位址，sort 只改 _M_next / _M_prev。
//   * 比較次數約 N log N，但實際執行時間主要由 cache miss 主導，
//     所以「比較次數」在這裡是很差的效能預測指標。
//
// 【注意事項 Pay Attention】
//   1. std::sort(lst.begin(), lst.end()) 編譯錯誤，不是效能問題。
//   2. 比較器必須是 strict weak ordering：
//      用 <= 而不是 < 會違反嚴格弱序，導致 UB（不同實作可能有不同表現，
//      不保證任何特定結果）。
//   3. sort 不使 iterator 失效，但元素順序改變——
//      「排序後 begin() 指向的還是同一個元素」是錯的。
//   4. 本檔的效能數字與位址每次執行都不同，僅供量級參考。
//   5. 演算法是實作選擇，面試時說「標準規定用 merge sort」是錯的；
//      正確說法是「標準規定穩定 + 約 N log N，libstdc++ 用 merge sort」。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】list::sort
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼不能對 std::list 用 std::sort？
//     答：std::sort 要求 RandomAccessIterator（需要 it+n、it-n、it1-it2），
//         因為它的 introsort 實作要選樞紐、算堆積子節點位置、雙向夾擊。
//         list 的迭代器只是 BidirectionalIterator，
//         所以這是「編譯期就被型別擋下來」，不是執行慢。
//     追問：那 list::sort 用什麼演算法？
//         → 標準只規定「穩定 + 約 N log N 次比較 + 不使 iterator 失效」，
//           沒規定演算法。libstdc++ 用 bottom-up merge sort；
//           merge sort 在鏈結串列上合併不需額外空間，是天生的好配。
//
// 🔥 Q2. list::sort 之後，先前保存的 iterator 還能用嗎？
//     答：能。標準保證 sort 不使 iterator／reference／pointer 失效，
//         因為它只重接 _M_next/_M_prev，節點本身不搬家。
//         本檔實測排序前後 &(*it) 位址相同。
//     追問：那 vector 呢？
//         → 完全相反。std::sort 是搬移「值」，iterator 仍然有效
//           （沒有重新配置的話），但它指向的是同一個「位置」，
//           該位置上的元素已經換人了。這個差別是節點式容器
//           與連續容器最本質的分野之一。
//
// ⚠️ 陷阱. 想按「部門，再按薪水」排序，寫兩次 sort 時哪個先？
//     答：先排次要鍵（薪水），再排主要鍵（部門）。順序反了就沒用。
//     為什麼會錯：直覺會照閱讀順序「先部門再薪水」寫下去，
//         但第二次排序會重排整個容器，只有穩定性能保住
//         「相同主鍵之間的既有順序」——所以次要鍵必須先建立好，
//         再由主要鍵覆蓋上去。記法：越重要的鍵，越晚排。
//
// ⚠️ 陷阱 2. 比較器寫成 return a.gpa >= b.gpa; 有什麼問題？
//     答：違反 strict weak ordering（相等元素會同時「小於」對方），
//         行為未定義。不要預期任何固定結果——它可能看似正常、
//         可能排錯、也可能在其他實作或其他資料量下崩潰。
//     為什麼會錯：把比較器當成「回答誰該排前面」的布林問題，
//         而不是當成數學上的嚴格弱序關係。正確寫法一律用 < 或 >。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <list>
#include <vector>
#include <string>
#include <functional>
#include <algorithm>
#include <chrono>
#include <random>
using namespace std;

template <typename T>
void print(const string& label, const list<T>& lst) {
    cout << "  " << label << ": ";
    for (const auto& v : lst) cout << v << " ";
    cout << endl;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 148. Sort List
//   題目：給定鏈結串列的 head，回傳排序後的串列，要求 O(n log n) 時間、
//         O(1) 額外空間（常數空間版本即 bottom-up merge sort）。
//   為什麼用到本主題：這題就是「手寫 list::sort」。
//         它之所以指定 merge sort，正是【詳細解釋 2】說的原因——
//         鏈結串列的合併只需改指標、不需要額外陣列，
//         而 quicksort 在鏈結串列上選不到好樞紐。
//   以下用 STL 的 list 重現同一個演算法骨架（切半 → 遞迴 → merge），
//   對照 libstdc++ 內部在做的事。
// -----------------------------------------------------------------------------
void sortListMergeSort(list<int>& lst) {
    if (lst.size() <= 1) return;

    // ① 快慢指標找中點（LeetCode 148 的標準切半手法）
    list<int> right;
    auto slow = lst.begin();
    auto fast = lst.begin();
    while (fast != lst.end()) {
        ++fast;
        if (fast != lst.end()) { ++fast; ++slow; }
    }

    // ② splice 把後半段整段搬走——只改指標，不複製元素
    right.splice(right.begin(), lst, slow, lst.end());

    // ③ 遞迴排序兩半
    sortListMergeSort(lst);
    sortListMergeSort(right);

    // ④ 原地合併（穩定）
    lst.merge(right);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】任務排程：依優先度排序，但同優先度必須維持先到先服務
//   情境：工作佇列裡每個任務有 priority（數字越小越優先）與到達序號。
//   關鍵需求是「公平性」：同優先度的任務不可以因為排序而被插隊。
//   這正是穩定排序的定義，所以完全不需要在比較器裡加上「再比到達時間」
//   這種容易寫錯的次要鍵——list::sort 的穩定性直接保證 FIFO。
//   用 list 而非 vector 的理由：排程器會在執行期插入／取消任務，
//   而其他模組持有指向任務的 iterator 當 handle。
// -----------------------------------------------------------------------------
struct Task {
    string  name;
    int     priority;   // 越小越優先
};

void scheduleByPriority(list<Task>& q) {
    // 只比 priority；同 priority 的相對順序由「穩定性」保證＝到達順序
    q.sort([](const Task& a, const Task& b) { return a.priority < b.priority; });
}

int main() {
    // 1. 基本排序
    cout << "===== 升序 / 降序 =====\n";
    {
        list<int> lst = {5,2,8,1,9,3,7,4,6};
        print("原始  ", lst);
        lst.sort();
        print("升序  ", lst);
        lst.sort(greater<int>());
        print("降序  ", lst);
    }

    // 2. 自訂物件排序
    cout << "\n===== 自訂物件排序 =====\n";
    {
        struct Student { string name; double gpa; };
        list<Student> students = {
            {"Alice",3.5},{"Bob",3.9},{"Charlie",3.2},
            {"David",3.7},{"Eve",3.9}
        };
        students.sort([](const Student& a, const Student& b) {
            return a.gpa > b.gpa;
        });
        for (auto& s : students)
            cout << "  " << s.name << " GPA:" << s.gpa << "\n";
    }

    // 3. 穩定性驗證
    cout << "\n===== 穩定性 =====\n";
    {
        struct Item { int key; string label; };
        list<Item> items = {
            {3,"A"},{1,"B"},{2,"C"},{1,"D"},{3,"E"},{2,"F"}
        };
        items.sort([](const Item& a, const Item& b) { return a.key < b.key; });
        cout << "  ";
        for (auto& i : items) cout << i.key << i.label << " ";
        cout << "\n  （相同 key 保持原始順序 → 穩定）\n";
    }

    // 4. 迭代器穩定性
    cout << "\n===== 迭代器穩定性 =====\n";
    {
        list<int> lst = {50,30,10,40,20};
        auto it = next(lst.begin());
        const int*  before_addr = &(*it);
        const int   before_val  = *it;
        lst.sort();
        // 位址本身每次執行都不同（ASLR／heap 佈局），所以印「是否相同」而非數值
        cout << "  sort 前 *it=" << before_val << "，sort 後 *it=" << *it << "\n";
        cout << "  節點位址不變？ " << boolalpha << (before_addr == &(*it)) << "\n";
        cout << "  （標準保證 sort 不使 iterator/reference 失效：只重接指標）\n";
    }

    // 5. 多鍵排序（利用穩定性）
    cout << "\n===== 多鍵排序 =====\n";
    {
        struct Emp { string dept; string name; };
        list<Emp> emps = {
            {"工程","Alice"},{"行銷","Bob"},{"工程","Charlie"},
            {"行銷","David"},{"工程","Eve"}
        };
        emps.sort([](const Emp& a, const Emp& b) { return a.name < b.name; });
        emps.sort([](const Emp& a, const Emp& b) { return a.dept < b.dept; });
        // 穩定：同部門內名字順序保持
        for (auto& e : emps) cout << "  " << e.dept << " | " << e.name << "\n";
    }

    // 6. 效能比較
    cout << "\n===== 效能比較 =====\n";
    {
        const int N = 500000;
        mt19937 gen(42);
        list<int> lst1;
        for (int i = 0; i < N; i++) lst1.push_back(gen() % 1000000);
        list<int> lst2 = lst1;

        auto t1 = chrono::high_resolution_clock::now();
        lst1.sort();
        auto t2 = chrono::high_resolution_clock::now();

        auto t3 = chrono::high_resolution_clock::now();
        vector<int> vec(lst2.begin(), lst2.end());
        sort(vec.begin(), vec.end());
        lst2.assign(vec.begin(), vec.end());
        auto t4 = chrono::high_resolution_clock::now();

        auto ms1 = chrono::duration_cast<chrono::milliseconds>(t2 - t1).count();
        auto ms2 = chrono::duration_cast<chrono::milliseconds>(t4 - t3).count();
        cout << "  list::sort:         " << ms1 << " ms\n";
        cout << "  vector sort+複製:   " << ms2 << " ms\n";
        cout << "  （耗時每次執行都不同，且受機器與最佳化等級影響，僅供量級參考）\n";
        cout << "  兩者結果一致：" << boolalpha << (lst1 == lst2) << "\n";
    }

    // 7. LeetCode 148：手寫 merge sort，對照 list::sort
    cout << "\n===== LeetCode 148. Sort List =====\n";
    {
        list<int> a = {38, 27, 43, 3, 9, 82, 10};
        list<int> b = a;
        print("原始      ", a);
        sortListMergeSort(a);
        print("手寫 merge", a);
        b.sort();
        print("list::sort", b);
        cout << "  結果相同：" << boolalpha << (a == b) << "\n";
    }

    // 8. 實務：任務排程的公平性
    cout << "\n===== 實務：優先度排程 + 同優先度 FIFO =====\n";
    {
        list<Task> q = {
            {"備份",     2}, {"付款",   1}, {"寄信",   2},
            {"對帳",     1}, {"清快取", 3}, {"退款",   1}
        };
        cout << "  到達順序: ";
        for (const auto& t : q) cout << t.name << "(" << t.priority << ") ";
        cout << "\n";

        scheduleByPriority(q);

        cout << "  排程結果: ";
        for (const auto& t : q) cout << t.name << "(" << t.priority << ") ";
        cout << "\n  （同優先度維持到達順序：付款→對帳→退款，穩定排序保證公平）\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -O2 -Wall -Wextra summary.cpp -o summary
//   （-O2 是因為第 6 節有 50 萬筆的效能比較；教學用途不加 -O2 也能編譯執行）

//   （耗時每次執行都不同，且受機器與最佳化等級影響，僅供量級參考）

// === 預期輸出 ===
// ===== 升序 / 降序 =====
//   原始  : 5 2 8 1 9 3 7 4 6
//   升序  : 1 2 3 4 5 6 7 8 9
//   降序  : 9 8 7 6 5 4 3 2 1
//
// ===== 自訂物件排序 =====
//   Bob GPA:3.9
//   Eve GPA:3.9
//   David GPA:3.7
//   Alice GPA:3.5
//   Charlie GPA:3.2
//
// ===== 穩定性 =====
//   1B 1D 2C 2F 3A 3E
//   （相同 key 保持原始順序 → 穩定）
//
// ===== 迭代器穩定性 =====
//   sort 前 *it=30，sort 後 *it=30
//   節點位址不變？ true
//   （標準保證 sort 不使 iterator/reference 失效：只重接指標）
//
// ===== 多鍵排序 =====
//   工程 | Alice
//   工程 | Charlie
//   工程 | Eve
//   行銷 | Bob
//   行銷 | David
//
// ===== 效能比較 =====
//   list::sort:         93 ms
//   vector sort+複製:   29 ms
//   兩者結果一致：true
//
// ===== LeetCode 148. Sort List =====
//   原始      : 38 27 43 3 9 82 10
//   手寫 merge: 3 9 10 27 38 43 82
//   list::sort: 3 9 10 27 38 43 82
//   結果相同：true
//
// ===== 實務：優先度排程 + 同優先度 FIFO =====
//   到達順序: 備份(2) 付款(1) 寄信(2) 對帳(1) 清快取(3) 退款(1)
//   排程結果: 付款(1) 對帳(1) 退款(1) 備份(2) 寄信(2) 清快取(3)
//   （同優先度維持到達順序：付款→對帳→退款，穩定排序保證公平）
