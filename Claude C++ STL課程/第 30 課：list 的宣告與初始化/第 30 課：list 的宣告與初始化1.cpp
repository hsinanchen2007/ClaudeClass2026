// =============================================================================
//  第 30 課：list 的宣告與初始化1.cpp  —  八種建構路徑的逐一實作
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<list>
//   本檔逐一示範 std::list 的建構與 assign：
//     預設 / 填充(n) / 填充(n,v) / initializer_list / 範圍 / 複製 / 移動 / assign
//   複雜度：預設與移動 O(1)；其餘皆 O(元素個數)
//   輔助函式：std::advance(it, n) — list 的迭代器是 BidirectionalIterator，
//             不支援 it + n，跳躍必須逐步走，成本 O(n)
//
// 【詳細解釋 Explanation】
//
// 【1. 範圍建構是這八種裡最重要的一種】
//   list<int> l(first, last) 接受任何 InputIterator 對，所以來源可以是
//   陣列、vector、另一個 list 的子區間，甚至 istream_iterator。
//   本檔第 4 節同時示範了三種來源，重點在於：list 不在乎你從哪裡來，
//   它只要求「能走、能解參考」。這正是 STL 迭代器抽象的價值——
//   容器與資料來源被徹底解耦。
//
// 【2. 「部分範圍」為什麼要用 advance 而不是 it + 1】
//   list 的迭代器是 BidirectionalIterator，只有 ++ / --，沒有 + n。
//   這不是 STL 偷懶，而是誠實：list 節點散在 heap 各處，
//   「跳 3 格」本來就只能一步一步走。若標準硬是提供 it + 3，
//   使用者會誤以為它像 vector 一樣是 O(1)。
//   std::advance 把這件事講明白：它會依迭代器分類選擇實作，
//   對 RandomAccess 是一次加法，對 Bidirectional 是迴圈。
//
// 【3. 深複製的證明方式】
//   第 5 節先複製、再對複本 push_back，然後印出原始容器確認沒變。
//   這是驗證「值語意（value semantics）」的標準手法。
//   STL 容器全部是值語意——複製就是真的複製一份資料，
//   不像 Java 的 ArrayList 賦值只是複製參考。
//
// 【4. assign 的三個重載對應三種來源】
//   assign(n, v) / assign(initializer_list) / assign(first, last)
//   三者都會先「概念上清空」再填入，但實作會盡量重用既有節點，
//   而不是全部釋放再重新配置。
//
// 【概念補充 Concept Deep Dive】
//   * list 的迭代器不會因為「其他元素被插入／刪除」而失效——
//     這是節點式容器的核心保證，也是本檔第 8 節能安全建巢狀結構的原因。
//     vector 沒有這個保證（重新配置會讓全部迭代器失效）。
//   * 本機實測（實作定義）：sizeof(std::list<int>) = 24 bytes，
//     每個節點也是 24 bytes（next 8 + prev 8 + int 4 + padding 4）。
//   * begin() == end() 是判斷空容器的通用寫法，
//     但 empty() 更清楚、且對所有容器都保證 O(1)。
//
// 【注意事項 Pay Attention】
//   1. list<int> lst2(5) 建立 5 個 0，不是預留 5 格——list 沒有 capacity。
//   2. (5,1) 與 {5,1} 結果不同，見下方面試題。
//   3. advance() 會就地修改迭代器（回傳 void）；
//      想要「不改原迭代器」請用 std::next(it, n)（C++11）。
//   4. 移動後的來源本機實測為空，但標準層面請只當它「可重新賦值」。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】list 的建構與迭代器跳躍
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 std::list 的迭代器不支援 it + 3，而 vector 可以？
//     答：因為迭代器分類不同。list 是 BidirectionalIterator，
//         vector 是 RandomAccessIterator。list 的節點散在 heap 各處，
//         位址之間沒有算術關係，「跳 3 格」只能一步步走 O(n)；
//         標準不提供 + n 是為了不讓使用者誤以為它是 O(1)。
//     追問：那 std::advance(it, 3) 為什麼可以？
//         → 它用 tag dispatch 依 iterator_category 選實作：
//           RandomAccess 走一次加法 O(1)，Bidirectional 走迴圈 O(n)。
//           語法統一了，但成本沒有被隱藏——複雜度仍取決於迭代器分類。
//
// 🔥 Q2. list<int> a(5, 1); 和 list<int> b{5, 1}; 各是什麼？
//     答：a 是五個 1（呼叫 (count, value)）；b 是兩個元素 5 和 1
//         （呼叫 initializer_list）。只要大括號內元素能轉成
//         initializer_list 的元素型別，該版本就優先。
//     追問：list<string> c{5, "STL"} 呢？
//         → 五個 "STL"。5 轉不成 string，initializer_list 版本不可行，
//           於是退回 (count, value)。「{} 一定走 initializer_list」是錯的。
//
// ⚠️ 陷阱. 判斷 list 是否為空，寫 lst.size() == 0 有什麼問題？
//     答：對 C++11 之後的 std::list 沒有效能問題（size() 已是 O(1)）。
//         但這個習慣在 C++03 的 list 上是 O(n)，而且對
//         std::forward_list 根本不成立——它沒有 size()。
//         通用且永遠 O(1) 的寫法是 empty()。
//     為什麼會錯：把「這個容器現在剛好是 O(1)」當成「所有容器都一樣」。
//         empty() 存在的理由就是提供跨容器一致的常數時間保證。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <list>
#include <vector>
#include <string>
using namespace std;

// 輔助函數：印出 list 內容
template <typename T>
void print_list(const string& label, const list<T>& lst) {
    cout << label << " [size=" << lst.size() << "]: ";
    for (const auto& val : lst) {
        cout << val << " ";
    }
    cout << endl;
}

int main() {
    cout << "===== 1. 預設建構（空 list）=====" << endl;
    list<int> lst1;
    print_list("lst1", lst1);
    cout << "begin() == end()? " << (lst1.begin() == lst1.end() ? "是" : "否") << endl;

    cout << "\n===== 2. 填充建構 =====" << endl;
    list<int> lst2(5);        // 5 個預設值
    print_list("lst2(5)     ", lst2);

    list<int> lst3(5, 42);    // 5 個 42
    print_list("lst3(5, 42) ", lst3);

    list<string> lst4(3, "STL");
    print_list("lst4(3,STL) ", lst4);

    cout << "\n===== 3. 初始化列表建構 =====" << endl;
    list<int> lst5 = {10, 20, 30, 40, 50};
    print_list("lst5{...}   ", lst5);

    cout << "\n===== 4. 範圍建構 =====" << endl;
    // 從陣列
    int arr[] = {100, 200, 300};
    list<int> lst6(begin(arr), end(arr));
    print_list("從陣列      ", lst6);

    // 從 vector
    vector<int> vec = {7, 8, 9};
    list<int> lst7(vec.begin(), vec.end());
    print_list("從 vector   ", lst7);

    // 從另一個 list 的部分範圍
    auto it_start = lst5.begin();
    auto it_end = lst5.begin();
    advance(it_start, 1);  // 指向 20
    advance(it_end, 4);    // 指向 50
    list<int> lst8(it_start, it_end);
    print_list("部分範圍    ", lst8);  // {20, 30, 40}

    cout << "\n===== 5. 複製建構 =====" << endl;
    list<int> lst9(lst5);  // 深複製
    print_list("原始 lst5   ", lst5);
    print_list("複製 lst9   ", lst9);

    lst9.push_back(60);
    print_list("lst9 加 60  ", lst9);
    print_list("lst5 不變   ", lst5);  // 確認深複製

    cout << "\n===== 6. 移動建構 =====" << endl;
    list<int> src = {1, 2, 3, 4, 5};
    print_list("移動前 src  ", src);

    list<int> dst(std::move(src));
    print_list("移動後 dst  ", dst);
    print_list("移動後 src  ", src);  // 空的

    cout << "\n===== 7. assign 重新指定 =====" << endl;
    list<int> lst10 = {1, 2, 3};
    print_list("assign 前   ", lst10);

    lst10.assign(4, 99);
    print_list("assign(4,99)", lst10);

    lst10.assign({10, 20, 30, 40, 50});
    print_list("assign{...} ", lst10);

    lst10.assign(vec.begin(), vec.end());
    print_list("assign(vec) ", lst10);

    cout << "\n===== 8. 特殊情境 =====" << endl;

    // 小心：圓括號 vs 花括號
    list<int> a(5, 1);    // 5 個 1 → {1, 1, 1, 1, 1}
    list<int> b{5, 1};    // 初始化列表 → {5, 1}
    print_list("(5, 1) 填充 ", a);
    print_list("{5, 1} 列表 ", b);

    // 巢狀 list
    list<list<int>> nested;
    nested.push_back({1, 2, 3});
    nested.push_back({4, 5});
    nested.push_back({6, 7, 8, 9});
    cout << "\n巢狀 list:" << endl;
    int row = 0;
    for (const auto& inner : nested) {
        cout << "  row " << row++ << ": ";
        for (int val : inner) cout << val << " ";
        cout << endl;
    }

    cout << "\n===== 9. empty() vs size()==0 =====" << endl;
    cout << "lst1.empty()      = " << boolalpha << lst1.empty() << endl;
    cout << "lst5.empty()      = " << lst5.empty() << endl;
    cout << "（empty() 對所有容器都保證 O(1)，forward_list 甚至沒有 size()）" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 30 課：list 的宣告與初始化1.cpp" -o lesson30

// === 預期輸出 ===
// ===== 1. 預設建構（空 list）=====
// lst1 [size=0]:
// begin() == end()? 是
//
// ===== 2. 填充建構 =====
// lst2(5)      [size=5]: 0 0 0 0 0
// lst3(5, 42)  [size=5]: 42 42 42 42 42
// lst4(3,STL)  [size=3]: STL STL STL
//
// ===== 3. 初始化列表建構 =====
// lst5{...}    [size=5]: 10 20 30 40 50
//
// ===== 4. 範圍建構 =====
// 從陣列       [size=3]: 100 200 300
// 從 vector    [size=3]: 7 8 9
// 部分範圍     [size=3]: 20 30 40
//
// ===== 5. 複製建構 =====
// 原始 lst5    [size=5]: 10 20 30 40 50
// 複製 lst9    [size=5]: 10 20 30 40 50
// lst9 加 60   [size=6]: 10 20 30 40 50 60
// lst5 不變    [size=5]: 10 20 30 40 50
//
// ===== 6. 移動建構 =====
// 移動前 src   [size=5]: 1 2 3 4 5
// 移動後 dst   [size=5]: 1 2 3 4 5
// 移動後 src   [size=0]:
//
// ===== 7. assign 重新指定 =====
// assign 前    [size=3]: 1 2 3
// assign(4,99) [size=4]: 99 99 99 99
// assign{...}  [size=5]: 10 20 30 40 50
// assign(vec)  [size=3]: 7 8 9
//
// ===== 8. 特殊情境 =====
// (5, 1) 填充  [size=5]: 1 1 1 1 1
// {5, 1} 列表  [size=2]: 5 1
//
// 巢狀 list:
//   row 0: 1 2 3
//   row 1: 4 5
//   row 2: 6 7 8 9
//
// ===== 9. empty() vs size()==0 =====
// lst1.empty()      = true
// lst5.empty()      = false
// （empty() 對所有容器都保證 O(1)，forward_list 甚至沒有 size()）
