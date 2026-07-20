// =============================================================================
//  summary.cpp  —  第 33 課總複習：list 的特有操作 merge
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<list>
//   簽名：
//       void merge(list& other);                                          // C++98
//       void merge(list&& other);                                         // C++11
//       template<class Compare> void merge(list& other, Compare comp);    // C++98
//       template<class Compare> void merge(list&& other, Compare comp);   // C++11
//
//   複雜度：至多 size() + other.size() - 1 次比較 → O(n + m)
//   前置條件：兩條 list 都必須已依「同一個」排序準則排好序。
//             違反 → 未定義行為（UB），不只是「結果不對」。
//   後置條件：other 保證變空；*this 含有全部元素且維持排序。
//   例外保證：不配置記憶體，因此不會 throw bad_alloc；
//             若比較器 comp 拋出例外，行為未定義。
//   迭代器：全部保持有效（節點只是換了隸屬的串列）。
//
// 【詳細解釋 Explanation】
//
// 【1. merge 的本質：重接指標，不是複製資料】
//   merge 不建立任何新節點，也不複製、不移動任何元素。
//   它只是把 other 的節點一個一個摘下來、接進 *this 的正確位置。
//   後果有三：
//     (a) 元素即使不可複製、不可移動，merge 依然可行。
//     (b) 不配置記憶體 → 不會因記憶體不足而失敗。
//     (c) 所有既有迭代器、指標、參考全部保持有效。
//   對照 <algorithm> 的 std::merge：那個是「讀兩個輸入範圍、寫進第三個輸出
//   範圍」，必須複製元素、需要目的地空間、原容器不變。兩者名字像，語意完全不同。
//
// 【2. 為什麼「必須已排序」是硬性前置條件】
//   merge 只做一次線性掃描：兩個游標各自從頭走，每次取較小者接上。
//   這個貪心策略的正確性完全建立在「各自內部已有序」上。
//   輸入無序時演算法沒有任何機制能察覺，它會照樣走完並吐出一條串列。
//   標準把這列為 Requires（前置條件），違反即為 UB —— 不保證任何特定輸出。
//   正確做法永遠是：A.sort(); B.sort(); A.merge(B);
//
// 【3. 穩定性是明文保證】
//   等價元素（既不小於也不大於）中，原屬 *this 的排在原屬 other 的前面。
//   這讓「多來源合併」有可預測的順序——合併多台主機的 log 時，
//   同一時間戳的事件不會每次執行跑出不同排列。
//
// 【4. merge / sort / splice 的分工】
//     A.splice(pos, B)   把 B 整段接到 pos，不比較不排序，O(1)
//     A.merge(B)         兩邊皆已排序，交錯接合成有序串列，O(n+m)
//     A.sort()           對自己排序，O(n log n)，內部反覆呼叫歸併
//   要合併兩條「未排序」的串列並得到有序結果，
//   先各自 sort 再 merge（O(n log n + m log m + n + m)）
//   通常比 splice 後整條 sort（O((n+m) log(n+m))）划算。
//
// 【5. 比較器必須前後一致且是嚴格弱序】
//   A.merge(B, comp) 的 comp 必須與「當初排序 A、B 時所用的準則」相同。
//   用 greater<int>() 合併，兩條就都必須是降序。混用即違反前置條件。
//   comp 還必須是 strict weak ordering —— 寫成 `a <= b` 會破壞這個要求，
//   在某些輸入下導致錯誤結果或更糟的狀況。
//
// 【概念補充 Concept Deep Dive】
//   ● list::sort 的實作就是反覆 merge：libstdc++ 採自底向上歸併，
//     維護一組大小 1、2、4、8… 的暫存 list，不斷把小段 merge 成大段。
//     因為 merge 本身就地且零配置，整個 sort 也不需要 O(n) 輔助陣列——
//     這是鏈結串列排序相對於陣列排序的經典優勢。
//   ● 配置器必須相等：merge 把節點的所有權從 other 移轉給 *this。
//     若兩者配置器不同，*this 日後釋放這些節點會用到錯的配置器，
//     標準因此規定此情況為 UB。
//   ● 比較次數上界 n + m - 1 的來由：每次比較至少確定一個元素的最終位置；
//     當一條走完，另一條可整段直接接上不必再比較。最壞是兩條完全交錯。
//   ● 為什麼 vector 沒有 merge 成員函式？因為 vector 的元素綁在連續緩衝區，
//     任何合併都必然搬移資料、可能重新配置、必然使迭代器失效——
//     那就不是「merge 這種操作對它划算」的結構。容器的結構決定了
//     哪些演算法值得做成成員函式，這是 STL 設計的一貫邏輯。
//
// 【注意事項 Pay Attention】
//   1. 未排序就 merge 是 UB，不可描述為「一定得到某個結果」。
//   2. merge 之後 other 保證為空；需要保留內容請事先自行複製。
//   3. 兩條 list 的配置器必須相等。
//   4. comp 必須是嚴格弱序（用 < 不要用 <=）。
//   5. merge 不配置記憶體，所以不會 throw bad_alloc。
//   6. 所有迭代器維持有效——這是 list 相對 vector 的核心優勢。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::list::merge 總複習
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. list::merge 與 std::merge（<algorithm>）差在哪？
//     答：std::merge 讀取兩個輸入範圍、把結果「複製」到第三個輸出範圍，
//         需要目的地空間，來源容器不變。list::merge 是就地重接節點指標，
//         零配置、零複製、來源被清空、所有迭代器仍然有效。
//     追問：那複雜度呢？→ 兩者都是 O(n+m) 次比較，但 list::merge 不需要
//         任何額外記憶體，std::merge 需要一塊能放下 n+m 個元素的輸出空間。
//
// 🔥 Q2. merge 之後，原本指向 other 元素的迭代器還有效嗎？
//     答：有效。節點沒有被釋放或重新配置，只是從一條串列摘下接到另一條，
//         所以迭代器、指標、參考全部存活，只是現在隸屬於 *this。
//     追問：other 變成什麼？→ 保證是空 list，size() == 0。
//
// 🔥 Q3. 要合併兩條「未排序」的 list 並得到有序結果，正確做法是什麼？
//     答：先各自 sort() 再 merge()：A.sort(); B.sort(); A.merge(B);
//         成本 O(n log n + m log m + (n+m))。
//         直接 merge 是違反前置條件的 UB；先 splice 再整條 sort 雖然正確，
//         但複雜度是 O((n+m) log(n+m))，通常較慢。
//     追問：為什麼先分開排比較快？→ 兩個較小的 log 因子相加，
//         通常小於一個大的 (n+m)log(n+m)，而且 merge 本身只要線性時間。
//
// ⚠️ 陷阱. 「merge 沒排序頂多結果亂掉，又不會怎樣。」
//     答：標準把「兩邊皆已排序」列為前置條件，違反就是未定義行為。
//         實務上多半只看到一條沒排好的串列，但標準不保證任何特定結果。
//         把 UB 當成「可預期的錯誤」是很危險的心智模型。
//     為什麼會錯：把「這次跑起來沒事」當成語言保證。UB 的風險在於它
//         可能換編譯器、換最佳化等級、換資料規模之後才顯現。
//
// ⚠️ 陷阱. 「A.merge(B) 之後，B 應該還在吧？我等一下還要用。」
//     答：不會。merge 是所有權轉移，B 保證變空。要保留請先複製一份。
//     為什麼會錯：用 std::merge（非破壞性）的心智模型去想 list::merge
//         （破壞性）。名字相同不代表語意相同。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <list>
#include <vector>
#include <string>
#include <functional>
using namespace std;

template <typename T>
void print(const string& label, const list<T>& lst) {
    cout << "  " << label << " [" << lst.size() << "]: ";
    for (const auto& v : lst) cout << v << " ";
    cout << (lst.empty() ? "(空)" : "") << endl;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 21. Merge Two Sorted Lists
//   題目：合併兩條已排序的鏈結串列，回傳一條仍然排序的串列。
//   為什麼用到本主題：這題的標準解法就是 list::merge 的手寫版——
//                     用雙游標比較、把節點接上，不配置新節點。
//                     用 std::list 表達時，a.merge(b) 一行就是答案。
// -----------------------------------------------------------------------------
list<int> mergeTwoLists(list<int> a, list<int> b) {
    a.merge(b);
    return a;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】合併多個已排序的資料分片（sharded query result）
//   情境：一張大表按使用者 ID 雜湊分成 4 個 shard，各 shard 回傳
//         「已按下單時間排序」的結果。前端要呈現統一的時間軸。
//   為什麼用 merge：各分片本來就有序，不必重新排序整批資料。
//         用分治兩兩合併是 O(N log k)，比逐一併進第一條的 O(k*N) 好；
//         而且全程不複製訂單資料（可能含長字串），只重接指標。
// -----------------------------------------------------------------------------
struct Order {
    long   ts;
    string order_id;
};

static list<Order> mergeShards(vector<list<Order>> shards) {
    auto by_ts = [](const Order& a, const Order& b) { return a.ts < b.ts; };
    if (shards.empty()) return {};
    // 分治：每輪把相鄰兩份合併，輪數為 log k
    while (shards.size() > 1) {
        vector<list<Order>> next;
        for (size_t i = 0; i < shards.size(); i += 2) {
            if (i + 1 < shards.size()) shards[i].merge(shards[i + 1], by_ts);
            next.push_back(std::move(shards[i]));
        }
        shards.swap(next);
    }
    return shards[0];
}

int main() {
    // 1. 升序 merge
    cout << "===== 升序 merge =====\n";
    {
        list<int> A = {2,5,8,10}, B = {1,3,6,7,9};
        print("A", A); print("B", B);
        A.merge(B);
        print("merge 後 A", A);
        print("merge 後 B", B);      // 保證變空
    }

    // 2. 降序 merge（兩條都必須已是降序）
    cout << "\n===== 降序 merge =====\n";
    {
        list<int> A = {10,8,5,2}, B = {9,7,6,3,1};
        A.merge(B, greater<int>());
        print("降序 A", A);
    }

    // 3. 穩定性驗證（標準明文保證：等價時 *this 的排前面）
    cout << "\n===== 穩定性 =====\n";
    {
        struct Item { string src; int val; };
        list<Item> A = {{"A",1},{"A",3},{"A",5}};
        list<Item> B = {{"B",1},{"B",3},{"B",4}};
        A.merge(B, [](const Item& a, const Item& b) { return a.val < b.val; });
        cout << "  合併：";
        for (auto& i : A) cout << i.src << i.val << " ";
        cout << "\n  （相同值時 A 排在 B 前面 → 標準保證的穩定性）\n";
    }

    // 4. 未排序 → 必須先 sort（直接 merge 是 UB）
    cout << "\n===== 先 sort 再 merge =====\n";
    {
        list<int> A = {5,2,8}, B = {3,9,1};
        A.sort(); B.sort();          // 滿足 merge 的前置條件
        A.merge(B);
        print("正確結果", A);
    }

    // 5. 多路合併
    cout << "\n===== 多路合併 =====\n";
    {
        list<int> lists[] = {{1,5,9},{2,6,10},{3,7,11},{4,8,12}};
        for (int i = 1; i < 4; i++) lists[0].merge(lists[i]);
        print("4 路合併", lists[0]);
    }

    // 6. 迭代器穩定性
    cout << "\n===== 迭代器穩定性 =====\n";
    {
        list<int> A = {2,5,8}, B = {1,3,6};
        auto it5 = next(A.begin()); auto it3 = next(B.begin());
        A.merge(B);
        cout << "  *it5=" << *it5 << " *it3=" << *it3 << " → 仍然有效\n";
    }

    // 7. 自訂物件
    cout << "\n===== 自訂物件 merge =====\n";
    {
        struct Student { string name; double gpa; };
        list<Student> cA = {{"Alice",3.9},{"Charlie",3.5},{"Eve",3.2}};
        list<Student> cB = {{"Bob",3.8},{"David",3.6},{"Frank",3.0}};
        cA.merge(cB, [](const Student& a, const Student& b) { return a.gpa > b.gpa; });
        cout << "  GPA 降序合併：\n";
        for (auto& s : cA) cout << "    " << s.name << " " << s.gpa << "\n";
    }

    // 8. LeetCode 21
    cout << "\n===== LeetCode 21. Merge Two Sorted Lists =====\n";
    {
        auto r = mergeTwoLists({1,2,4}, {1,3,4});
        print("結果", r);
    }

    // 9. 日常實務：合併分片查詢結果
    cout << "\n===== 日常實務：合併 4 個已排序的資料分片 =====\n";
    {
        vector<list<Order>> shards = {
            {{1717000001, "ORD-1001"}, {1717000009, "ORD-1009"}},
            {{1717000003, "ORD-1003"}, {1717000007, "ORD-1007"}},
            {{1717000002, "ORD-1002"}, {1717000010, "ORD-1010"}},
            {{1717000005, "ORD-1005"}, {1717000006, "ORD-1006"}},
        };
        auto all = mergeShards(std::move(shards));
        cout << "  統一時間軸（共 " << all.size() << " 筆）：\n";
        for (const auto& o : all)
            cout << "    [" << o.ts << "] " << o.order_id << "\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
// ===== 升序 merge =====
//   A [4]: 2 5 8 10
//   B [5]: 1 3 6 7 9
//   merge 後 A [9]: 1 2 3 5 6 7 8 9 10
//   merge 後 B [0]: (空)
//
// ===== 降序 merge =====
//   降序 A [9]: 10 9 8 7 6 5 3 2 1
//
// ===== 穩定性 =====
//   合併：A1 B1 A3 B3 B4 A5
//   （相同值時 A 排在 B 前面 → 標準保證的穩定性）
//
// ===== 先 sort 再 merge =====
//   正確結果 [6]: 1 2 3 5 8 9
//
// ===== 多路合併 =====
//   4 路合併 [12]: 1 2 3 4 5 6 7 8 9 10 11 12
//
// ===== 迭代器穩定性 =====
//   *it5=5 *it3=3 → 仍然有效
//
// ===== 自訂物件 merge =====
//   GPA 降序合併：
//     Alice 3.9
//     Bob 3.8
//     David 3.6
//     Charlie 3.5
//     Eve 3.2
//     Frank 3
//
// ===== LeetCode 21. Merge Two Sorted Lists =====
//   結果 [6]: 1 1 2 3 4 4
//
// ===== 日常實務：合併 4 個已排序的資料分片 =====
//   統一時間軸（共 8 筆）：
//     [1717000001] ORD-1001
//     [1717000002] ORD-1002
//     [1717000003] ORD-1003
//     [1717000005] ORD-1005
//     [1717000006] ORD-1006
//     [1717000007] ORD-1007
//     [1717000009] ORD-1009
//     [1717000010] ORD-1010
