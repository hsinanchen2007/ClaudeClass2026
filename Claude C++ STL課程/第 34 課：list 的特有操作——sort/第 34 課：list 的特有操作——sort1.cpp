// =============================================================================
//  第 34 課：list 的特有操作——sort1.cpp
//  —— 手寫 merge sort 對照 list::sort，並實測 cache 對排序效能的支配力
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<list>
//   簽名：  void sort();
//           template<class Compare> void sort(Compare comp);
//   標準保證：穩定、約 N log N 次比較、不使 iterator／reference 失效
//   標準未規定：實際演算法（libstdc++ 用 bottom-up merge sort）
//   本檔另用到：list::splice（跨 list 搬部分區間為 O(distance)）、
//               list::merge（穩定合併，兩邊須已排序）
//
// 【詳細解釋 Explanation】
//
// 【1. my_list_sort 為什麼要用「快慢指標」找中點】
//   list 沒有隨機存取，拿不到 lst.begin() + size/2。
//   快慢指標（fast 走兩步、slow 走一步）讓我們在一次走訪內找到中點，
//   而且完全不需要事先知道長度——這正是鏈結串列題目的標準手法。
//   注意本檔的迴圈刻意寫成「先 ++fast，確認沒到 end 才再 ++fast 並 ++slow」，
//   這是為了避免對 end() 再前進一次（那是未定義行為）。
//
// 【2. splice 在這裡扮演的角色：切半而不複製】
//   second_half.splice(second_half.begin(), lst, slow, lst.end());
//   這一行把 [slow, end) 整段從 lst 摘下、接到 second_half。
//   節點完全沒有被複製或移動，只是幾根指標改接。
//   代價是：因為要維護兩邊的 size()（C++11 起 size() 必須 O(1)），
//   這個「跨 list 搬移部分區間」的重載複雜度是 O(distance(first,last))。
//   也就是說遞迴每層仍是 O(n)，總計 O(n log n)，與 merge sort 相符。
//
// 【3. merge 為什麼可以原地合併】
//   list::merge 假設兩個 list 各自已排序，然後只用指標接線把它們併起來，
//   不配置任何新節點、不複製任何元素。這就是「鏈結串列適合 merge sort」
//   的關鍵：陣列版 merge sort 需要 O(n) 額外緩衝區，鏈結串列版不需要。
//   merge 也是穩定的——相等時取前一個 list 的元素。
//
// 【4. 第 6 節在驗證什麼】
//   保存排序前指向某元素的 iterator，排序後檢查它是否仍指向同一個節點。
//   結果是位址完全相同：sort 沒有搬移任何元素，只重接了 _M_next/_M_prev。
//   （位址數值本身每次執行都不同，所以本檔印的是「位址是否相同」的布林值。）
//
// 【5. 第 7 節的效能結論要怎麼讀】
//   list::sort 不搬移元素、比較次數也是 N log N，理論上不該輸。
//   但它每次比較都要解參考兩個散落在 heap 的節點 → 幾乎每次 cache miss。
//   vector 方案多付出兩次 O(n) 複製，卻能在連續記憶體上排序。
//   實測通常 vector 方案勝出，這說明現代 CPU 上
//   「記憶體存取模式」往往比「演算法漸進複雜度」更能決定實際速度。
//   注意：本節用 random_device 產生資料，每次執行的資料與耗時都不同。
//
// 【概念補充 Concept Deep Dive】
//   * 本檔的 my_list_sort 是 top-down（遞迴）merge sort，堆疊深度 O(log n)；
//     libstdc++ 內建的是 bottom-up 版本，用一組大小為 2^i 的暫存 list
//     像二進位進位一樣往上合併，不遞迴、堆疊使用 O(1)。
//     兩者複雜度相同，行為（穩定性、不失效）也相同。
//   * list<int> 每個節點在本機實測佔 24 bytes（next 8 + prev 8 + int 4
//     + padding 4，實作定義），資料只佔 1/6，這是 cache miss 的根源。
//   * lst1 == lst2 對 list 是逐元素比較 O(n)，不是比較指標。
//
// 【注意事項 Pay Attention】
//   1. std::sort(lst.begin(), lst.end()) 對 list 是編譯錯誤（迭代器分類不符），
//      不是「可以跑但很慢」。
//   2. 比較器必須滿足 strict weak ordering；寫成 <= 會導致未定義行為，
//      不要預期任何固定結果。
//   3. my_list_sort 對 size() <= 1 直接返回，這個 base case 不能放寬成
//      只檢查 empty()——理由見下方陷阱題。
//   4. 效能數字、隨機資料每次執行都不同；位址亦然。
//   5. list::merge 要求「兩邊都已排序」，否則結果未定義（不是「會自動排好」）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】手寫鏈結串列排序
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 在鏈結串列上實作 merge sort，如何找到中點？
//     答：快慢指標。fast 每次走兩步、slow 每次走一步，fast 到底時
//         slow 正好在中點。這樣只走一遍、不需要先算長度，
//         也不需要隨機存取（list 本來就沒有）。
//     追問：為什麼不先 size() / 2 再 advance？
//         → 也可以，且 C++11 起 size() 是 O(1)。但 advance 仍要走 n/2 步，
//           總成本一樣，而快慢指標對「只給 head 指標」的原生鏈結串列
//           （LeetCode 148 的情境）才是唯一可行解——那裡根本沒有 size()。
//
// 🔥 Q2. 為什麼鏈結串列的 merge sort 不需要額外 O(n) 空間，陣列版卻需要？
//     答：陣列的合併必須把結果寫到別的地方，因為原地寫會覆蓋還沒讀的元素。
//         鏈結串列的合併只是改指標接線——節點本來就散在各處，
//         重新串起來不需要任何新空間。
//     追問：那空間複雜度是多少？
//         → top-down 遞迴版是 O(log n)（遞迴堆疊）；
//           bottom-up 版（libstdc++ 用的）是 O(1)。
//           LeetCode 148 要求 O(1) 空間時，指的就是 bottom-up。
//
// ⚠️ 陷阱. my_list_sort 的 base case 若從 size() <= 1 放寬成 empty() 會怎樣？
//     答：單一元素時不會提早返回。快慢指標會讓 slow 停在 begin()，
//         splice 把整串搬到 second_half，lst 變空——接著對
//         「空 + 單元素」再遞迴，同樣的切法無限重複直到堆疊耗盡。
//     為什麼會錯：直覺認為「空的才需要提早返回」，
//         但 merge sort 的 base case 必須涵蓋「已經不可再分」的情況，
//         也就是 size() <= 1，而不只是 size() == 0。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <list>
#include <string>
#include <functional>
#include <chrono>
#include <vector>
#include <algorithm>
#include <random>
using namespace std;

template <typename T>
void print_list(const string& label, const list<T>& lst) {
    cout << label << " [" << lst.size() << "]: ";
    for (const auto& val : lst) cout << val << " ";
    cout << (lst.empty() ? "(空)" : "") << endl;
}

// ===== 手動實作歸併排序（教學用） =====
template <typename T>
void my_list_sort(list<T>& lst) {
    // 基底情況：0 或 1 個元素，已經排序
    if (lst.size() <= 1) return;

    // ① 分割：用快慢指標找中點
    list<T> second_half;
    auto slow = lst.begin();
    auto fast = lst.begin();

    // fast 每次走 2 步，slow 每次走 1 步
    while (fast != lst.end()) {
        ++fast;
        if (fast != lst.end()) {
            ++fast;
            ++slow;
        }
    }
    // slow 現在指向後半段的開頭

    // 用 splice 把後半段切出來（O(n) 因為跨 list 要計數）
    second_half.splice(second_half.begin(), lst, slow, lst.end());

    // ② 遞迴排序兩半
    my_list_sort(lst);           // 排序前半段
    my_list_sort(second_half);   // 排序後半段

    // ③ 合併（用 list::merge，原地操作）
    lst.merge(second_half);
}

int main() {
    // ===== 1. list::sort 基本用法 =====
    cout << "===== list::sort 基本用法 =====" << endl;
    {
        list<int> lst = {5, 2, 8, 1, 9, 3, 7, 4, 6};
        print_list("排序前", lst);

        lst.sort();
        print_list("排序後", lst);
    }

    // ===== 2. 降序排序 =====
    cout << "\n===== 降序排序 =====" << endl;
    {
        list<int> lst = {5, 2, 8, 1, 9, 3, 7, 4, 6};
        print_list("排序前", lst);

        lst.sort(greater<int>());
        print_list("降序後", lst);
    }

    // ===== 3. 自訂物件排序 =====
    cout << "\n===== 自訂物件排序 =====" << endl;
    {
        struct Student {
            string name;
            double gpa;
        };

        list<Student> students = {
            {"Alice", 3.5},
            {"Bob", 3.9},
            {"Charlie", 3.2},
            {"David", 3.7},
            {"Eve", 3.9}
        };

        cout << "排序前：" << endl;
        for (const auto& s : students)
            cout << "  " << s.name << " GPA: " << s.gpa << endl;

        // 按 GPA 降序排序
        students.sort([](const Student& a, const Student& b) {
            return a.gpa > b.gpa;
        });

        cout << "按 GPA 降序：" << endl;
        for (const auto& s : students)
            cout << "  " << s.name << " GPA: " << s.gpa << endl;
    }

    // ===== 4. 穩定性驗證 =====
    cout << "\n===== 穩定性驗證 =====" << endl;
    {
        struct Item {
            int key;
            string label;
        };

        list<Item> items = {
            {3, "A"}, {1, "B"}, {2, "C"},
            {1, "D"}, {3, "E"}, {2, "F"}
        };

        cout << "排序前：";
        for (const auto& item : items)
            cout << item.key << item.label << " ";
        cout << endl;

        items.sort([](const Item& a, const Item& b) {
            return a.key < b.key;
        });

        cout << "排序後：";
        for (const auto& item : items)
            cout << item.key << item.label << " ";
        cout << endl;
        cout << "（相同 key 的元素保持原始順序 → 穩定排序）" << endl;
    }

    // ===== 5. 手動實作 vs list::sort 驗證 =====
    cout << "\n===== 手動歸併排序 vs list::sort =====" << endl;
    {
        list<int> lst1 = {38, 27, 43, 3, 9, 82, 10};
        list<int> lst2 = lst1;  // 複製一份

        print_list("原始資料   ", lst1);

        my_list_sort(lst1);
        print_list("手動歸併   ", lst1);

        lst2.sort();
        print_list("list::sort ", lst2);

        cout << "兩者結果相同：" << (lst1 == lst2 ? "是" : "否") << endl;
    }

    // ===== 6. 排序的迭代器穩定性 =====
    cout << "\n===== sort 的迭代器穩定性 =====" << endl;
    {
        list<int> lst = {50, 30, 10, 40, 20};

        // 保存指向元素 30 的迭代器
        auto it = lst.begin();
        advance(it, 1);  // 指向 30
        const int* addr_before = &(*it);
        cout << "sort 前 *it = " << *it << endl;

        lst.sort();

        // 迭代器仍然有效，仍指向 30
        cout << "sort 後 *it = " << *it << endl;
        // 位址數值每次執行都不同（ASLR／heap 佈局），所以比對「是否相同」
        cout << "節點位址不變？ " << boolalpha << (addr_before == &(*it)) << endl;
        cout << "（地址相同 → 節點沒動，只是重新接線）" << endl;

        print_list("排序結果", lst);
    }

    // ===== 7. 效能比較：list::sort vs 複製到 vector 再 sort =====
    cout << "\n===== 效能比較 =====" << endl;
    {
        const int N = 500000;
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<int> dist(1, 1000000);

        // 建立隨機 list
        list<int> lst1;
        for (int i = 0; i < N; i++) {
            lst1.push_back(dist(gen));
        }
        list<int> lst2 = lst1;  // 複製一份

        // 方法 1：直接 list::sort
        auto start1 = chrono::high_resolution_clock::now();
        lst1.sort();
        auto end1 = chrono::high_resolution_clock::now();
        auto ms1 = chrono::duration_cast<chrono::milliseconds>(end1 - start1).count();

        // 方法 2：複製到 vector → sort → 複製回 list
        auto start2 = chrono::high_resolution_clock::now();
        vector<int> vec(lst2.begin(), lst2.end());
        std::sort(vec.begin(), vec.end());
        lst2.assign(vec.begin(), vec.end());
        auto end2 = chrono::high_resolution_clock::now();
        auto ms2 = chrono::duration_cast<chrono::milliseconds>(end2 - start2).count();

        cout << N << " 個元素排序（隨機資料與耗時每次執行都不同）：" << endl;
        cout << "  list::sort         : " << ms1 << " ms" << endl;
        cout << "  vector sort + 複製 : " << ms2 << " ms" << endl;

        if (ms1 > ms2) {
            cout << "  → vector 方案更快（快取效率的威力）" << endl;
        } else {
            cout << "  → list::sort 更快（少了複製的開銷）" << endl;
        }

        // 驗證兩者結果相同
        cout << "  結果一致：" << (lst1 == lst2 ? "是" : "否") << endl;
    }

    // ===== 8. 多鍵排序 =====
    cout << "\n===== 多鍵排序（利用穩定性） =====" << endl;
    {
        struct Employee {
            string department;
            string name;
            int salary;
        };

        list<Employee> employees = {
            {"工程", "Alice",   80000},
            {"行銷", "Bob",     70000},
            {"工程", "Charlie", 75000},
            {"行銷", "David",   70000},
            {"工程", "Eve",     80000},
            {"行銷", "Frank",   65000}
        };

        // 先按次要鍵排序（名字）
        employees.sort([](const Employee& a, const Employee& b) {
            return a.name < b.name;
        });

        // 再按主要鍵排序（部門）— 因為 sort 是穩定的，
        // 同部門內的名字順序會保持！
        employees.sort([](const Employee& a, const Employee& b) {
            return a.department < b.department;
        });

        cout << "按部門→名字排序：" << endl;
        for (const auto& e : employees) {
            cout << "  " << e.department << " | "
                 << e.name << " | $" << e.salary << endl;
        }
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 34 課：list 的特有操作——sort1.cpp" -o lesson34
//   （第 7 節有 50 萬筆效能比較，加 -O2 會更接近實務數字）

// 500000 個元素排序（隨機資料與耗時每次執行都不同）：

// === 預期輸出 ===
// ===== list::sort 基本用法 =====
// 排序前 [9]: 5 2 8 1 9 3 7 4 6
// 排序後 [9]: 1 2 3 4 5 6 7 8 9
//
// ===== 降序排序 =====
// 排序前 [9]: 5 2 8 1 9 3 7 4 6
// 降序後 [9]: 9 8 7 6 5 4 3 2 1
//
// ===== 自訂物件排序 =====
// 排序前：
//   Alice GPA: 3.5
//   Bob GPA: 3.9
//   Charlie GPA: 3.2
//   David GPA: 3.7
//   Eve GPA: 3.9
// 按 GPA 降序：
//   Bob GPA: 3.9
//   Eve GPA: 3.9
//   David GPA: 3.7
//   Alice GPA: 3.5
//   Charlie GPA: 3.2
//
// ===== 穩定性驗證 =====
// 排序前：3A 1B 2C 1D 3E 2F
// 排序後：1B 1D 2C 2F 3A 3E
// （相同 key 的元素保持原始順序 → 穩定排序）
//
// ===== 手動歸併排序 vs list::sort =====
// 原始資料    [7]: 38 27 43 3 9 82 10
// 手動歸併    [7]: 3 9 10 27 38 43 82
// list::sort  [7]: 3 9 10 27 38 43 82
// 兩者結果相同：是
//
// ===== sort 的迭代器穩定性 =====
// sort 前 *it = 30
// sort 後 *it = 30
// 節點位址不變？ true
// （地址相同 → 節點沒動，只是重新接線）
// 排序結果 [5]: 10 20 30 40 50
//
// ===== 效能比較 =====
//   list::sort         : 310 ms
//   vector sort + 複製 : 92 ms
//   → vector 方案更快（快取效率的威力）
//   結果一致：是
//
// ===== 多鍵排序（利用穩定性） =====
// 按部門→名字排序：
//   工程 | Alice | $80000
//   工程 | Charlie | $75000
//   工程 | Eve | $80000
//   行銷 | Bob | $70000
//   行銷 | David | $70000
//   行銷 | Frank | $65000
