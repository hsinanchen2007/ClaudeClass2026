// =============================================================================
//  第 35 課：list 的特有操作——unique、remove、remove_if1.cpp
//    —  真刪節點的成員函式，與只會搬移的同名演算法
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔: #include <list>
//
//     remove(value)          移除所有 == value 的元素        O(n)
//     remove_if(pred)        移除所有 pred 為 true 的元素     O(n)
//     unique()               移除**相鄰**重複                O(n)
//     unique(binary_pred)    以 pred 判定「相鄰視為重複」      O(n)
//
//   ★ 三者都**真正摘除節點並釋放記憶體**，size() 會變小。
//   ★ 都**不使其他 iterator 失效**（只有被刪的那些失效）。
//   ★ 回傳型別（本機 g++ 15.2 以 -pedantic-errors 實測確認）：
//       C++17 以前：void  ← **本檔採用**，故不取回傳值
//       C++20 起  ：size_type（移除個數）
//     本檔以 -std=c++17 編譯，示範「沒有回傳值時要怎麼知道刪了幾個」
//     （比較前後 size()）。要直接取得個數請見 summary.cpp（-std=c++20）。
//
// 【詳細解釋 Explanation】
//
// 【1. 成員函式 vs 同名演算法:本課最重要的對比】
//   std::remove（<algorithm>）只拿得到 iterator，**沒有能力改變容器大小**。
//   它把要保留的元素往前搬，回傳新的**邏輯尾端**，尾端之後是
//   「有效但未指定」的值，size() 完全沒變。必須自己補上 erase：
//       v.erase(std::remove(v.begin(), v.end(), 3), v.end());   // erase-remove idiom
//   list::remove（成員函式）是容器的一部分，直接把節點從鏈上摘掉並釋放，
//   size() 真的變小。
//   對 list 而言成員版還更快：std::remove 會逐個**賦值搬移元素**，
//   而 list::remove 只改指標，一個元素都不搬。
//   → 通則:**容器提供同名成員函式時，優先用成員版**。
//
// 【2. unique 只折疊「相鄰」重複】
//   {1,1,2,3,3,3,2,2,1}.unique() → {1,2,3,2,1}
//   1 和 2 依然存在，因為它們不相鄰。只看相鄰才能保證 O(n)。
//   要全域去重必須**先 sort** 讓相同值聚在一起：
//       lst.sort();     // O(n log n)，成員版只改指標
//       lst.unique();   // O(n)
//   順序不可顛倒——先 unique 再 sort 無法消除原本不相鄰的重複。
//
// 【概念補充 Concept Deep Dive】
// 為什麼演算法不能刪除元素？因為 STL 刻意讓「演算法」與「容器」解耦：
// iterator 只提供存取與移動，不知道背後是哪個容器、也沒有容器的指標。
// 這個設計讓同一份 std::remove 能作用在 vector / deque / 原生陣列上，
// 代價就是它無法縮小容器。理解這點，erase-remove idiom 就不必死背。
//
// 【注意事項 Pay Attention】
//  1. unique 只移除**相鄰**重複；全域去重必須先 sort。
//  2. std::remove 不會縮小容器，必須配 erase；成員 remove 才是真刪。
//  3. std::remove 新邏輯尾端之後的元素是「有效但未指定」，不可假設其內容。
//  4. 這些操作不使其他 iterator 失效，只有被刪的那些失效。
//  5. 判定式不可在執行期間修改該容器（UB）。
//  6. C++17 以前這些成員函式回傳 void，不能寫 `auto n = lst.remove(x);`。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】list::unique / remove / remove_if
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. lst.remove(3) 跟 std::remove(lst.begin(), lst.end(), 3) 差在哪?
//     答：成員版**真的摘除節點並釋放記憶體**，size() 變小。
//         演算法版只把保留的元素往前搬、回傳新的邏輯尾端，**size() 不變**，
//         必須再 erase(newEnd, end()) 才算刪掉（erase-remove idiom）。
//         對 list 來說成員版還更快:它只改指標，不搬任何元素的值。
//     追問：為什麼演算法做不到真刪?→ 它只有 iterator，不知道背後是哪個
//         容器，也拿不到容器物件。這個解耦正是它能通用於各種容器的原因。
//
// ⚠️ 陷阱. lst.unique() 之後為什麼還看得到重複的值?
//     答：unique 只折疊**相鄰**重複。{1,1,2,3,3,3,2,2,1} → {1,2,3,2,1}，
//         不相鄰的 1、2 都保留。要全域去重必須先 sort 再 unique。
//     為什麼會錯：把函式名讀成「去重」。它的契約其實是
//         「使相鄰元素唯一」——正確性**依賴輸入已排序**這個前提。
//         也因此順序不可顛倒:先 unique 再 sort 是無效的。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <list>
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>
#include <cmath>
#include <cctype>

template <typename C>
void print(const std::string& label, const C& c) {
    std::cout << "  " << label << " [" << c.size() << "]:";
    for (const auto& v : c) std::cout << " " << v;
    std::cout << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 83. Remove Duplicates from Sorted List
//   題目：已排序的鏈結串列，讓每個值只出現一次。
//   為什麼用到本主題：輸入**已排序**，重複必然相鄰——這正是 unique() 的
//     前提條件，一行解決。這題也反證了「unique 只處理相鄰」是合理設計：
//     在已排序資料上，相鄰重複就等於全部重複。
// -----------------------------------------------------------------------------
std::list<int> deleteDuplicates(std::list<int> head) {
    head.unique();
    return head;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】監控告警去重:清掉已解除的告警，並摺疊連續重複的訊息
//   情境：告警系統收到一串事件，要先移除已解除（resolved）的項目，
//         再把「連續重複」的同一則告警摺疊成一筆，避免通知洗版。
//   為什麼用 remove_if + unique：
//     remove_if 一次遍歷真正刪除已解除的節點；
//     unique 只折疊連續重複——不相鄰的同樣告警代表它**又發生了一次**，
//     必須保留，所以這裡刻意**不**先 sort（先 sort 會破壞時間順序）。
// -----------------------------------------------------------------------------
struct Alert {
    std::string msg;
    bool        resolved;
};
std::ostream& operator<<(std::ostream& os, const Alert& a) { return os << a.msg; }

std::list<Alert> condenseAlerts(std::list<Alert> alerts) {
    alerts.remove_if([](const Alert& a) { return a.resolved; });   // 真正刪除節點
    alerts.unique([](const Alert& a, const Alert& b) {             // 只折疊連續重複
        return a.msg == b.msg;
    });
    return alerts;
}

int main() {
    std::cout << "=== 1. remove:移除所有等於某值的元素 ===\n";
    {
        std::list<int> lst = {1,3,2,3,4,3,5,3};
        print("原始         ", lst);
        std::size_t before = lst.size();
        lst.remove(3);                       // C++17:回傳 void，只能比較前後 size
        print("remove(3)    ", lst);
        std::cout << "  移除了 " << (before - lst.size())
                  << " 個（C++17 無回傳值，只能自行比較 size）\n";
    }
    {
        std::list<std::string> words = {"apple","banana","apple","cherry","apple"};
        print("原始         ", words);
        words.remove("apple");
        print("remove(apple)", words);
    }

    std::cout << "\n=== 2. remove_if:依判定式移除 ===\n";
    {
        std::list<int> lst = {1,2,3,4,5,6,7,8,9,10};
        print("原始         ", lst);
        lst.remove_if([](int x) { return x % 2 == 0; });
        print("移除偶數     ", lst);
    }
    {
        struct Student { std::string name; double gpa; };
        std::list<Student> students = {
            {"Alice",3.8},{"Bob",2.1},{"Charlie",3.5},
            {"David",1.9},{"Eve",3.9},{"Frank",2.5}
        };
        students.remove_if([](const Student& s) { return s.gpa < 2.5; });
        std::cout << "  移除 GPA<2.5 後留下:";
        for (const auto& s : students) std::cout << " " << s.name;
        std::cout << "\n";
    }

    std::cout << "\n=== 3. unique:只折疊「相鄰」重複 ===\n";
    {
        std::list<int> lst = {1,1,2,3,3,3,2,2,1};
        print("原始         ", lst);
        lst.unique();
        print("unique()     ", lst);
        std::cout << "  ★ 不相鄰的 1、2 仍在——unique 只看相鄰\n";
    }

    std::cout << "\n=== 4. sort + unique = 全域去重（順序不可顛倒）===\n";
    {
        std::list<int> lst = {3,1,4,1,5,9,2,6,5,3,5};
        print("原始         ", lst);
        lst.sort();     print("sort()       ", lst);
        lst.unique();   print("unique()     ", lst);
    }
    {
        std::list<int> bad = {3,1,4,1,5,9,2,6,5,3,5};
        bad.unique();   // 先 unique
        bad.sort();     // 再 sort ← 錯誤順序
        print("先unique後sort", bad);
        std::cout << "  ★ 重複的 1、3、5 仍在 → 證明順序顛倒無效\n";
    }

    std::cout << "\n=== 5. unique 的自訂二元判定式 ===\n";
    {
        std::list<double> lst = {1.0,1.1,1.2,2.5,2.6,3.0,3.3,3.4};
        print("原始         ", lst);
        lst.unique([](double a, double b) { return std::fabs(a - b) < 0.5; });
        print("差<0.5視為重複", lst);
        std::cout << "  ★ 比的是「前一個保留下來的」與目前元素，非原始前鄰\n";
    }
    {
        std::list<std::string> words = {"Hello","hello","HELLO","World","world","Foo"};
        print("原始         ", words);
        words.unique([](const std::string& a, const std::string& b) {
            if (a.size() != b.size()) return false;
            for (std::size_t i = 0; i < a.size(); ++i)
                if (std::tolower(static_cast<unsigned char>(a[i])) !=
                    std::tolower(static_cast<unsigned char>(b[i]))) return false;
            return true;
        });
        print("忽略大小寫   ", words);
    }

    std::cout << "\n=== 6. 成員 remove vs 演算法 std::remove ===\n";
    {
        std::list<int> lst = {1,3,2,3,4,3,5};
        std::cout << "  【成員】list::remove(3):size " << lst.size();
        lst.remove(3);
        std::cout << " → " << lst.size() << "（真的變小）\n";
        print("    結果       ", lst);

        std::vector<int> v = {1,3,2,3,4,3,5};
        std::cout << "  【演算法】std::remove:size " << v.size();
        auto newEnd = std::remove(v.begin(), v.end(), 3);
        std::cout << " → " << v.size() << "（完全沒變！）\n";
        std::cout << "    邏輯長度只有 " << (newEnd - v.begin()) << "，有效範圍:";
        for (auto it = v.begin(); it != newEnd; ++it) std::cout << " " << *it;
        std::cout << "\n";
        std::cout << "    ★ newEnd 之後是「有效但未指定」的值，不可假設也不可用\n";
        v.erase(newEnd, v.end());               // erase-remove idiom 第二步
        std::cout << "    補 erase 後 size=" << v.size() << "（這才算真刪）\n";
        print("    結果       ", v);
    }

    std::cout << "\n=== 7. 迭代器穩定性:只有被刪的失效 ===\n";
    {
        std::list<int> lst = {10,20,30,20,40,20,50};
        auto it10 = lst.begin();
        auto it40 = std::next(lst.begin(), 4);
        std::cout << "  remove 前 *it10=" << *it10 << " *it40=" << *it40 << "\n";
        lst.remove(20);
        std::cout << "  remove 後 *it10=" << *it10 << " *it40=" << *it40
                  << " → 未被刪的仍有效\n";
        print("  結果       ", lst);
    }

    std::cout << "\n=== 8. LeetCode 83. Remove Duplicates from Sorted List ===\n";
    print("原始（已排序）", std::list<int>{1,1,2,3,3});
    print("結果         ", deleteDuplicates({1,1,2,3,3}));
    print("原始（已排序）", std::list<int>{1,1,2});
    print("結果         ", deleteDuplicates({1,1,2}));

    std::cout << "\n=== 9. 實務:告警去重與摺疊 ===\n";
    {
        std::list<Alert> alerts = {
            {"CPU 使用率 > 90%", false},
            {"CPU 使用率 > 90%", false},
            {"磁碟空間不足",     true},      // 已解除
            {"CPU 使用率 > 90%", false},
            {"記憶體不足",       false},
            {"記憶體不足",       false},
            {"連線數過高",       true}       // 已解除
        };
        std::cout << "  原始 " << alerts.size() << " 筆:\n";
        for (const auto& a : alerts)
            std::cout << "    | " << a.msg << (a.resolved ? "  [已解除]" : "") << "\n";
        auto out = condenseAlerts(alerts);
        std::cout << "  處理後 " << out.size() << " 筆:\n";
        for (const auto& a : out) std::cout << "    | " << a.msg << "\n";
        std::cout << "  ★ 第 3 筆 CPU 告警與前兩筆之間隔了「磁碟空間不足」，\n";
        std::cout << "    但那筆已解除被 remove_if 刪掉 → CPU 告警變成相鄰而被摺疊\n";
    }

    std::cout << "\n=== 10. 組合使用:清理數據 ===\n";
    {
        std::list<int> data = {-1,5,3,-2,5,0,3,8,-1,5,0,3,7};
        print("原始         ", data);
        data.remove_if([](int x) { return x < 0; });
        print("移除負數     ", data);
        data.remove(0);
        print("移除 0       ", data);
        data.sort();
        print("sort         ", data);
        data.unique();
        print("unique       ", data);
    }
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 35 課：list 的特有操作——unique、remove、remove_if1.cpp" -o list_unique
//   ★ 本檔刻意用 C++17:此標準下 remove/remove_if/unique 回傳 void，
//     所以示範「以前後 size() 相減」得知移除個數。
//     C++20 起可直接取回傳值（見同目錄 summary.cpp）。

// === 預期輸出 ===
// === 1. remove:移除所有等於某值的元素 ===
//   原始          [8]: 1 3 2 3 4 3 5 3
//   remove(3)     [4]: 1 2 4 5
//   移除了 4 個（C++17 無回傳值，只能自行比較 size）
//   原始          [5]: apple banana apple cherry apple
//   remove(apple) [2]: banana cherry
//
// === 2. remove_if:依判定式移除 ===
//   原始          [10]: 1 2 3 4 5 6 7 8 9 10
//   移除偶數      [5]: 1 3 5 7 9
//   移除 GPA<2.5 後留下: Alice Charlie Eve Frank
//
// === 3. unique:只折疊「相鄰」重複 ===
//   原始          [9]: 1 1 2 3 3 3 2 2 1
//   unique()      [5]: 1 2 3 2 1
//   ★ 不相鄰的 1、2 仍在——unique 只看相鄰
//
// === 4. sort + unique = 全域去重（順序不可顛倒）===
//   原始          [11]: 3 1 4 1 5 9 2 6 5 3 5
//   sort()        [11]: 1 1 2 3 3 4 5 5 5 6 9
//   unique()      [7]: 1 2 3 4 5 6 9
//   先unique後sort [11]: 1 1 2 3 3 4 5 5 5 6 9
//   ★ 重複的 1、3、5 仍在 → 證明順序顛倒無效
//
// === 5. unique 的自訂二元判定式 ===
//   原始          [8]: 1 1.1 1.2 2.5 2.6 3 3.3 3.4
//   差<0.5視為重複 [3]: 1 2.5 3
//   ★ 比的是「前一個保留下來的」與目前元素，非原始前鄰
//   原始          [6]: Hello hello HELLO World world Foo
//   忽略大小寫    [3]: Hello World Foo
//
// === 6. 成員 remove vs 演算法 std::remove ===
//   【成員】list::remove(3):size 7 → 4（真的變小）
//       結果        [4]: 1 2 4 5
//   【演算法】std::remove:size 7 → 7（完全沒變！）
//     邏輯長度只有 4，有效範圍: 1 2 4 5
//     ★ newEnd 之後是「有效但未指定」的值，不可假設也不可用
//     補 erase 後 size=4（這才算真刪）
//       結果        [4]: 1 2 4 5
//
// === 7. 迭代器穩定性:只有被刪的失效 ===
//   remove 前 *it10=10 *it40=40
//   remove 後 *it10=10 *it40=40 → 未被刪的仍有效
//     結果        [4]: 10 30 40 50
//
// === 8. LeetCode 83. Remove Duplicates from Sorted List ===
//   原始（已排序） [5]: 1 1 2 3 3
//   結果          [3]: 1 2 3
//   原始（已排序） [3]: 1 1 2
//   結果          [2]: 1 2
//
// === 9. 實務:告警去重與摺疊 ===
//   原始 7 筆:
//     | CPU 使用率 > 90%
//     | CPU 使用率 > 90%
//     | 磁碟空間不足  [已解除]
//     | CPU 使用率 > 90%
//     | 記憶體不足
//     | 記憶體不足
//     | 連線數過高  [已解除]
//   處理後 2 筆:
//     | CPU 使用率 > 90%
//     | 記憶體不足
//   ★ 第 3 筆 CPU 告警與前兩筆之間隔了「磁碟空間不足」，
//     但那筆已解除被 remove_if 刪掉 → CPU 告警變成相鄰而被摺疊
//
// === 10. 組合使用:清理數據 ===
//   原始          [13]: -1 5 3 -2 5 0 3 8 -1 5 0 3 7
//   移除負數      [10]: 5 3 5 0 3 8 5 0 3 7
//   移除 0        [8]: 5 3 5 3 8 5 3 7
//   sort          [8]: 3 3 3 5 5 5 7 8
//   unique        [4]: 3 5 7 8
