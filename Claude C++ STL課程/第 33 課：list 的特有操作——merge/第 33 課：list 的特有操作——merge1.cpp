// =============================================================================
//  第 33 課：list 的特有操作 —— merge 1  —  兩條已排序串列的 O(n) 接合
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<list>
//   簽名（C++98 起；C++11 起新增 rvalue 版本）：
//       void merge(list& other);                       // (1) 用 operator<
//       void merge(list&& other);                      // (2) C++11
//       template<class Compare> void merge(list& other, Compare comp);   // (3)
//       template<class Compare> void merge(list&& other, Compare comp);  // (4) C++11
//
//   複雜度：至多 size() + other.size() - 1 次比較 → O(n + m)
//   前置條件（Precondition）：**兩條 list 都必須已依同一個排序準則排好序**。
//                            違反此前提是未定義行為（UB），不是「結果不正確」而已。
//   效果：other 的所有節點被搬進 *this，other 變成空的；
//         **不配置也不釋放任何節點，不複製也不移動任何元素**。
//   穩定性：等價元素中，原本屬於 *this 的排在 other 的前面。
//   迭代器：指向被搬移元素的迭代器、指標、參考**全部保持有效**，
//           只是它們現在屬於 *this 而不是 other。
//
// 【詳細解釋 Explanation】
//
// 【1. merge 到底做了什麼：搬節點，不搬資料】
//   一般人想像的合併是「開一條新串列，把兩邊的值比一比再抄過去」。
//   list::merge 不是這樣。它只是把 other 的節點一個一個「摘下來、接上去」：
//       比較 A 的目前節點與 B 的目前節點
//       → 若 B 的較小，就把 B 那個節點從 B 摘下，接到 A 的目前位置之前
//       → 否則 A 前進一步
//   全程只有指標重新接線，元素本身連一個 byte 都沒有被複製或移動。
//   這帶來三個重要後果：
//     (a) 即使元素是「不可複製、不可移動」的型別（例如含 const 成員、
//         或刪除了移動建構子），merge 依然可行。
//     (b) 不會拋出 bad_alloc，因為完全不配置記憶體。
//     (c) 所有既有迭代器都還活著（本檔第 8 段實測）。
//   這正是 std::vector 永遠做不到的事——vector 的元素綁在連續緩衝區上，
//   任何合併都必然涉及搬移。
//
// 【2. 為什麼前提是「必須已排序」】
//   merge 的演算法只做「一次線性掃描」：兩個指標各自從頭走，每次取較小的那個。
//   這個策略之所以正確，完全建立在「各自內部已經有序」這個假設上。
//   一旦輸入無序，演算法沒有任何機制能察覺，也不會回頭修正——
//   它會照樣走完並產生一條「看起來像結果」的串列。
//   標準把「兩邊皆已排序」列為 Requires（前置條件），違反它就是未定義行為。
//   實務上你通常會看到一條沒排好的串列，但**標準不保證任何特定結果**，
//   也不保證不會出現更糟的狀況。本檔第 4 段示範這件事，請看該段的說明。
//
// 【3. 穩定性（stability）是明文保證，不是實作巧合】
//   標準規定：若 *this 與 other 中有等價（既不小於也不大於）的元素，
//   則 *this 的元素排在 other 的元素之前。
//   這在「多來源資料合併」時非常重要——例如合併多台伺服器的 log，
//   同一毫秒的事件要維持「主機編號小的優先」這種可預測順序。
//   本檔第 3 段用帶來源標記的 Item 驗證這件事。
//
// 【4. merge vs sort vs splice：三者的分工】
//     A.splice(pos, B)  —— 把 B 整段接到 A 的 pos 位置，**不比較、不排序**，O(1)。
//     A.merge(B)        —— 兩邊都已排序，交錯接合成一條有序串列，O(n+m)。
//     A.sort()          —— 對 A 自己排序，O(n log n)，內部就是反覆用歸併。
//   如果兩邊沒排序又想要有序結果，正確做法是先各自 sort 再 merge：
//     A.sort(); B.sort(); A.merge(B);   → O(n log n + m log m + (n+m))
//   而不是 splice 之後再 sort（那也可行，複雜度 O((n+m) log(n+m))，通常較慢）。
//
// 【5. 比較器必須一致】
//   A.merge(B, comp) 中的 comp 必須跟「當初把 A 和 B 排好序時用的準則」一致。
//   用 greater<int>() 合併時，兩條串列都必須是降序。
//   混用（A 升序、B 降序）同樣違反前置條件，同樣是 UB。
//   comp 也必須是嚴格弱序（strict weak ordering）——用 <= 而非 < 會出事。
//
// 【概念補充 Concept Deep Dive】
//   ● 為什麼 std::list 要自備 merge，而不用 <algorithm> 的 std::merge？
//     std::merge 是「把兩個範圍合併到第三個輸出範圍」，它必須複製元素，
//     而且需要一塊目的地空間。list::merge 是「就地重接指標」，
//     零配置、零複製、迭代器不失效。兩者名字像，語意完全不同。
//   ● 為什麼配置器（allocator）必須相同？
//     merge 是把節點的所有權從 other 轉移給 *this。若兩者的配置器不同，
//     *this 將來釋放這些節點時會用錯的配置器 → 標準規定此情況為 UB。
//     這也是為什麼 merge 只能用在「同型別、同配置器」的兩條 list 之間。
//   ● list::sort 的內部實作就是反覆 merge：
//     libstdc++ 用的是自底向上的歸併排序，維護一組大小為 1、2、4、8… 的
//     暫存 list，不斷把小段 merge 成大段。因為 merge 不需要額外空間，
//     整個 sort 也就不需要 O(n) 的輔助陣列——這是鏈結串列排序的經典優勢。
//   ● 為什麼 merge 的比較次數上界是 n + m - 1？
//     每次比較至少確定一個元素的最終位置；當其中一條走完，另一條可以整段
//     直接接上（不必再比較）。最壞情況是兩條交錯用完，即 n + m - 1 次。
//
// 【注意事項 Pay Attention】
//   1. **未排序就 merge 是未定義行為**，不可描述成「一定會得到某個結果」。
//      本檔第 4 段的輸出只是本機這一次的實際觀察，不是規格。
//   2. merge 之後 other 保證變成空的（size() == 0），這點是明文規定。
//   3. 兩條 list 的配置器必須相等，否則是 UB。
//   4. comp 必須是嚴格弱序；寫成 `a <= b` 會破壞這個要求。
//   5. 不可對「同一條 list」自我 merge（A.merge(A)）——標準對此有特別規定，
//      實作通常直接忽略，但不要依賴這個行為。
//   6. merge 不會配置記憶體，因此不會因為記憶體不足而失敗。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::list::merge
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. list::merge 跟 <algorithm> 的 std::merge 差在哪裡？
//     答：std::merge 把兩個輸入範圍合併「複製」到第三個輸出範圍，需要目的地空間，
//         元素會被複製或移動。list::merge 是就地把 other 的節點重新接線到 *this，
//         零配置、零複製，而且所有既有迭代器、指標、參考都保持有效。
//         兩者只是名字像，語意完全不同。
//     追問：那 list::merge 的複雜度？→ 至多 n + m - 1 次比較，即 O(n + m)，
//         而且不需要任何額外記憶體。
//
// 🔥 Q2. merge 之後，原本指向 other 中某個元素的迭代器還能用嗎？
//     答：能。merge 只是把節點從一條串列摘下、接到另一條，節點本身沒有被
//         釋放也沒有被重新配置。所以迭代器、指標、參考全部保持有效，
//         只是它們現在隸屬於 *this。這是 list 相對於 vector 的核心優勢之一。
//     追問：那 other 會變成什麼？→ 保證變成空 list（size() == 0）。
//
// 🔥 Q3. 為什麼 list 有自己的 merge 和 sort，vector 卻沒有？
//     答：因為 list 能用「重接指標」在不搬動元素的前提下完成這些操作，
//         而 vector 的元素綁在連續緩衝區，任何重排都必須實際搬移資料。
//         反過來說，std::sort 需要隨機存取迭代器，list 提供不了，
//         所以 list 只能自己實作一個基於歸併的版本。這是「容器結構決定
//         哪些演算法划算」的典型例子。
//     追問：list::sort 為什麼選歸併排序？→ 歸併只需循序走訪與指標接合，
//         不需要隨機存取，也不需要額外的 O(n) 輔助空間（merge 是就地的）。
//
// ⚠️ 陷阱. 「沒排序就 merge，頂多結果亂掉，反正不會出事。」
//     答：標準把「兩邊皆已排序」列為前置條件（Requires），違反它就是
//         **未定義行為**，不是「結果不正確」這麼溫和。實務上你多半只會看到
//         一條沒排好的串列，但標準不保證任何特定結果。
//         正確做法是先各自 sort() 再 merge()。
//     為什麼會錯：把「我這次跑起來沒事」當成語言保證。UB 的危險就在於
//         它可能在換編譯器、換最佳化等級、換資料後才爆出來。
//
// ⚠️ 陷阱. 「A.merge(B) 之後，B 裡面應該還留著原來那些元素吧？」
//     答：不會。merge 是「轉移」不是「複製」，B 保證變成空的。
//         如果你還需要 B 的內容，必須在 merge 之前自己複製一份。
//     為什麼會錯：把 merge 想成 std::merge 那種「讀取兩個輸入、寫到第三處」
//         的非破壞性操作。list::merge 會清空來源，是破壞性的。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <list>
#include <vector>        // 多路合併示範用
#include <string>
#include <utility>       // std::move
#include <functional>    // greater
using namespace std;

template <typename T>
void print_list(const string& label, const list<T>& lst) {
    cout << label << " [" << lst.size() << "]: ";
    for (const auto& val : lst) cout << val << " ";
    cout << (lst.empty() ? "(空)" : "") << endl;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 21. Merge Two Sorted Lists
//   題目：給兩條已排序的鏈結串列，合併成一條仍然排序的串列。
//   為什麼用到本主題：這題就是 list::merge 的手寫版。用 std::list 表達時，
//                     一行 a.merge(b) 就是答案——而且語意完全對應：
//                     題目要求「用原節點接成新串列」，正是 merge 的重接指標行為。
// -----------------------------------------------------------------------------
list<int> mergeTwoSortedLists(list<int> a, list<int> b) {
    a.merge(b);          // O(n + m)，不配置任何節點
    return a;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 23. Merge k Sorted Lists
//   題目：給 k 條已排序的串列，合併成一條。
//   為什麼用到本主題：本檔第 7 段的「多路合併」正是這題的直觀解法。
//   複雜度提醒：像下面這樣「逐一併進第一條」是 O(k * N)（N 為總元素數），
//              因為第一條會越來越長、每次都要整條走過。
//              兩兩配對合併（分治）可降到 O(N log k)，這正是本題的考點。
// -----------------------------------------------------------------------------
// 逐一合併：簡單但較慢，O(k * N)
list<int> mergeKLists_naive(vector<list<int>> lists) {
    list<int> result;
    for (auto& l : lists) result.merge(l);
    return result;
}

// 分治兩兩合併：O(N log k)
list<int> mergeKLists_divide(vector<list<int>> lists) {
    if (lists.empty()) return {};
    while (lists.size() > 1) {
        vector<list<int>> next;
        for (size_t i = 0; i < lists.size(); i += 2) {
            if (i + 1 < lists.size()) {
                lists[i].merge(lists[i + 1]);   // 兩兩合併
            }
            next.push_back(std::move(lists[i]));
        }
        lists.swap(next);
    }
    return lists[0];
}

// -----------------------------------------------------------------------------
// 【日常實務範例】合併多台伺服器的存取日誌（依時間戳排序）
//   情境：三台 web server 各自產生已按時間排序的 log；集中分析前要合併成
//         單一時間軸。同一毫秒發生的事件，希望維持「主機編號小的排前面」
//         這種可預測的順序，方便重現與比對。
//   為什麼用 list::merge：
//     1. 每台的 log 本來就已排序 → 不必再 sort，直接 O(n+m) 交錯接合。
//     2. merge 保證穩定 → 同時間戳的事件順序可預測，不會每次跑出不同結果。
//     3. 完全不複製 log 內容（字串可能很長），只重接指標。
// -----------------------------------------------------------------------------
struct LogEntry {
    long   timestamp_ms;
    int    host_id;
    string message;
};

static void mergeServerLogs() {
    // 每台伺服器的 log 本身已按時間排序
    list<LogEntry> host1 = {
        {1717000001, 1, "GET /index.html 200"},
        {1717000005, 1, "GET /style.css 200"},
        {1717000009, 1, "POST /api/login 200"},
    };
    list<LogEntry> host2 = {
        {1717000001, 2, "GET /index.html 200"},   // 與 host1 同一毫秒
        {1717000004, 2, "GET /logo.png 404"},
        {1717000011, 2, "GET /api/items 500"},
    };
    list<LogEntry> host3 = {
        {1717000002, 3, "GET /index.html 200"},
        {1717000007, 3, "POST /api/order 201"},
    };

    auto by_time = [](const LogEntry& a, const LogEntry& b) {
        return a.timestamp_ms < b.timestamp_ms;   // 只比時間 → 同時間的順序靠穩定性決定
    };

    host1.merge(host2, by_time);
    host1.merge(host3, by_time);

    cout << "  合併後的統一時間軸：" << endl;
    for (const auto& e : host1) {
        cout << "    [" << e.timestamp_ms << "] host" << e.host_id
             << "  " << e.message << endl;
    }
    cout << "  注意 1717000001 有兩筆：host1 排在 host2 前面（merge 的穩定性保證）"
         << endl;
}

int main() {
    // ===== 1. 基本 merge（升序） =====
    cout << "===== 基本 merge（升序） =====" << endl;
    {
        list<int> A = {2, 5, 8, 10};
        list<int> B = {1, 3, 6, 7, 9};
        print_list("A", A);
        print_list("B", B);

        A.merge(B);
        print_list("merge 後 A", A);
        print_list("merge 後 B", B);   // B 保證變空
    }

    // ===== 2. 降序 merge =====
    // 注意：兩條都必須「已按降序排好」，比較器才會一致
    cout << "\n===== 降序 merge =====" << endl;
    {
        list<int> A = {10, 8, 5, 2};
        list<int> B = {9, 7, 6, 3, 1};
        print_list("A", A);
        print_list("B", B);

        A.merge(B, greater<int>());
        print_list("merge 後 A", A);
        print_list("merge 後 B", B);
    }

    // ===== 3. 穩定性驗證 =====
    cout << "\n===== 穩定性驗證 =====" << endl;
    {
        struct Item {
            string source;
            int value;
        };

        list<Item> A = {{"A", 1}, {"A", 3}, {"A", 5}};
        list<Item> B = {{"B", 1}, {"B", 3}, {"B", 4}};

        cout << "A: ";
        for (const auto& item : A) cout << item.source << item.value << " ";
        cout << endl;

        cout << "B: ";
        for (const auto& item : B) cout << item.source << item.value << " ";
        cout << endl;

        A.merge(B, [](const Item& a, const Item& b) {
            return a.value < b.value;
        });

        cout << "merge 後：";
        for (const auto& item : A) cout << item.source << item.value << " ";
        cout << endl;
        cout << "（相同值時，A 的元素排在 B 前面 → 標準保證的穩定性）" << endl;
    }

    // ===== 4. 未排序的 list 做 merge（違反前置條件 → 未定義行為） =====
    cout << "\n===== 未排序 merge（違反前置條件，UB 示範） =====" << endl;
    {
        list<int> A = {5, 2, 8};    // 未排序！
        list<int> B = {3, 9, 1};    // 未排序！
        print_list("A（未排序）", A);
        print_list("B（未排序）", B);

        // ⚠️ 標準把「兩邊皆已排序」列為前置條件（Requires）。
        //    違反它是【未定義行為】，不是「保證得到某個錯誤結果」。
        //    下面的輸出只是本機這一次執行的實際觀察，不可當成規格，
        //    換編譯器、換最佳化等級、換資料都可能不同。
        A.merge(B);
        print_list("本次觀察到的結果 ", A);
        cout << "（結果並未排序；而且這是 UB，標準不保證任何特定輸出）" << endl;
    }

    // ===== 5. 正確做法：先 sort 再 merge =====
    cout << "\n===== 正確做法：先 sort 再 merge =====" << endl;
    {
        list<int> A = {5, 2, 8};
        list<int> B = {3, 9, 1};
        print_list("A（未排序）", A);
        print_list("B（未排序）", B);

        A.sort();    // 先排序 → {2, 5, 8}
        B.sort();    // 先排序 → {1, 3, 9}
        print_list("sort 後 A  ", A);
        print_list("sort 後 B  ", B);

        A.merge(B);
        print_list("merge 後 A ", A);
        cout << "（結果正確排序，且滿足 merge 的前置條件）" << endl;
    }

    // ===== 6. 自訂物件 merge =====
    cout << "\n===== 自訂物件 merge =====" << endl;
    {
        struct Student {
            string name;
            double gpa;
        };

        // 兩班學生，各自按 GPA 降序排列
        list<Student> classA = {{"Alice", 3.9}, {"Charlie", 3.5}, {"Eve", 3.2}};
        list<Student> classB = {{"Bob", 3.8}, {"David", 3.6}, {"Frank", 3.0}};

        auto by_gpa_desc = [](const Student& a, const Student& b) {
            return a.gpa > b.gpa;
        };

        cout << "A 班：";
        for (const auto& s : classA) cout << s.name << "(" << s.gpa << ") ";
        cout << endl;

        cout << "B 班：";
        for (const auto& s : classB) cout << s.name << "(" << s.gpa << ") ";
        cout << endl;

        classA.merge(classB, by_gpa_desc);

        cout << "合併（GPA 降序）：" << endl;
        for (const auto& s : classA) {
            cout << "  " << s.name << " - GPA: " << s.gpa << endl;
        }
    }

    // ===== 7. 多路合併（merge 多個 list） =====
    cout << "\n===== 多路合併 =====" << endl;
    {
        list<int> lists[] = {
            {1, 5, 9},
            {2, 6, 10},
            {3, 7, 11},
            {4, 8, 12}
        };

        cout << "合併前：" << endl;
        for (int i = 0; i < 4; i++) {
            cout << "  list " << i << ": ";
            for (int val : lists[i]) cout << val << " ";
            cout << endl;
        }

        // 逐一合併到 lists[0]
        for (int i = 1; i < 4; i++) {
            lists[0].merge(lists[i]);
        }

        print_list("合併結果", lists[0]);
    }

    // ===== 8. merge 的迭代器穩定性 =====
    cout << "\n===== merge 的迭代器穩定性 =====" << endl;
    {
        list<int> A = {2, 5, 8};
        list<int> B = {1, 3, 6};

        auto it_5 = A.begin();
        advance(it_5, 1);         // 指向 A 的 5
        auto it_3 = B.begin();
        advance(it_3, 1);         // 指向 B 的 3

        cout << "merge 前 *it_5=" << *it_5 << " *it_3=" << *it_3 << endl;

        A.merge(B);

        // 迭代器仍然有效！節點沒被釋放，只是換了隸屬的串列
        cout << "merge 後 *it_5=" << *it_5 << " *it_3=" << *it_3 << endl;
        print_list("A", A);
        print_list("B", B);
    }

    // ===== 9. LeetCode 21 / 23 =====
    cout << "\n===== LeetCode 21. Merge Two Sorted Lists =====" << endl;
    {
        auto r = mergeTwoSortedLists({1, 2, 4}, {1, 3, 4});
        print_list("結果", r);
    }

    cout << "\n===== LeetCode 23. Merge k Sorted Lists =====" << endl;
    {
        vector<list<int>> k1 = {{1, 4, 5}, {1, 3, 4}, {2, 6}};
        auto r1 = mergeKLists_naive(k1);
        print_list("逐一合併 O(k*N)", r1);

        vector<list<int>> k2 = {{1, 4, 5}, {1, 3, 4}, {2, 6}};
        auto r2 = mergeKLists_divide(k2);
        print_list("分治合併 O(N log k)", r2);
    }

    // ===== 10. 日常實務：合併多台伺服器 log =====
    cout << "\n===== 日常實務：合併多台伺服器 log =====" << endl;
    mergeServerLogs();

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 33 課：list 的特有操作——merge1.cpp" -o lesson33

// 注意：第 4 段刻意違反 merge 的前置條件，屬於未定義行為。
//       下方該段的輸出僅為本機這一次執行的實際觀察，標準不保證任何特定結果。

// === 預期輸出 ===
// ===== 基本 merge（升序） =====
// A [4]: 2 5 8 10
// B [5]: 1 3 6 7 9
// merge 後 A [9]: 1 2 3 5 6 7 8 9 10
// merge 後 B [0]: (空)
//
// ===== 降序 merge =====
// A [4]: 10 8 5 2
// B [5]: 9 7 6 3 1
// merge 後 A [9]: 10 9 8 7 6 5 3 2 1
// merge 後 B [0]: (空)
//
// ===== 穩定性驗證 =====
// A: A1 A3 A5
// B: B1 B3 B4
// merge 後：A1 B1 A3 B3 B4 A5
// （相同值時，A 的元素排在 B 前面 → 標準保證的穩定性）
//
// ===== 未排序 merge（違反前置條件，UB 示範） =====
// A（未排序） [3]: 5 2 8
// B（未排序） [3]: 3 9 1
// 本次觀察到的結果  [6]: 3 5 2 8 9 1
// （結果並未排序；而且這是 UB，標準不保證任何特定輸出）
//
// ===== 正確做法：先 sort 再 merge =====
// A（未排序） [3]: 5 2 8
// B（未排序） [3]: 3 9 1
// sort 後 A   [3]: 2 5 8
// sort 後 B   [3]: 1 3 9
// merge 後 A  [6]: 1 2 3 5 8 9
// （結果正確排序，且滿足 merge 的前置條件）
//
// ===== 自訂物件 merge =====
// A 班：Alice(3.9) Charlie(3.5) Eve(3.2)
// B 班：Bob(3.8) David(3.6) Frank(3)
// 合併（GPA 降序）：
//   Alice - GPA: 3.9
//   Bob - GPA: 3.8
//   David - GPA: 3.6
//   Charlie - GPA: 3.5
//   Eve - GPA: 3.2
//   Frank - GPA: 3
//
// ===== 多路合併 =====
// 合併前：
//   list 0: 1 5 9
//   list 1: 2 6 10
//   list 2: 3 7 11
//   list 3: 4 8 12
// 合併結果 [12]: 1 2 3 4 5 6 7 8 9 10 11 12
//
// ===== merge 的迭代器穩定性 =====
// merge 前 *it_5=5 *it_3=3
// merge 後 *it_5=5 *it_3=3
// A [6]: 1 2 3 5 6 8
// B [0]: (空)
//
// ===== LeetCode 21. Merge Two Sorted Lists =====
// 結果 [6]: 1 1 2 3 4 4
//
// ===== LeetCode 23. Merge k Sorted Lists =====
// 逐一合併 O(k*N) [8]: 1 1 2 3 4 4 5 6
// 分治合併 O(N log k) [8]: 1 1 2 3 4 4 5 6
//
// ===== 日常實務：合併多台伺服器 log =====
//   合併後的統一時間軸：
//     [1717000001] host1  GET /index.html 200
//     [1717000001] host2  GET /index.html 200
//     [1717000002] host3  GET /index.html 200
//     [1717000004] host2  GET /logo.png 404
//     [1717000005] host1  GET /style.css 200
//     [1717000007] host3  POST /api/order 201
//     [1717000009] host1  POST /api/login 200
//     [1717000011] host2  GET /api/items 500
//   注意 1717000001 有兩筆：host1 排在 host2 前面（merge 的穩定性保證）
