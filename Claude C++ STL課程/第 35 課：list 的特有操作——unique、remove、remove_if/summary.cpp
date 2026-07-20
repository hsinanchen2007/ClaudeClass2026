// =============================================================================
//  summary.cpp  —  list 的特有操作：unique / remove / remove_if
//                  以及「成員函式 vs 同名演算法」這組最經典的對比
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔: #include <list>
//
//   成員函式（本課主角）——真正刪除節點、釋放記憶體：
//     remove(const T& value)              移除所有 == value 的元素      O(n)
//     remove_if(Predicate pred)           移除所有 pred 為 true 的元素  O(n)
//     unique()                            移除**相鄰**重複             O(n)
//     unique(BinaryPredicate pred)        以 pred 判定「相鄰視為重複」  O(n)
//
//   ★ 回傳型別隨標準版本改變（本機 g++ 15.2 以 -pedantic-errors 實測確認）：
//       C++17 以前：void
//       C++20 起  ：size_type（回傳**被移除的元素個數**）
//     本檔要示範回傳值，所以編譯行使用 -std=c++20。
//     若你的專案仍是 C++17，這些函式不能拿回傳值，只能呼叫。
//
//   ★ 全部都**不使其他 iterator 失效**——只有被刪除元素本身的失效。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 list 要自己做一套?——這是本課的核心問題】
// <algorithm> 已經有 std::remove / std::remove_if / std::unique 了，
// 為什麼 list 還要提供同名的成員函式？因為兩者做的是**完全不同的事**：
//
//   std::remove（演算法）：
//     它只拿得到 iterator，**沒有能力改變容器的大小**——演算法不知道自己
//     操作的是 vector、array 還是 deque，更不可能去呼叫容器的 erase。
//     所以它做的是「搬移」：把不該刪的元素往前擠，回傳一個
//     **新的邏輯尾端**，尾端之後的元素處於「有效但未指定」的狀態。
//     容器的 size() **完全沒變**。
//
//   list::remove（成員函式）：
//     它是容器的一部分，知道自己的節點結構，可以直接把節點從鏈上摘下來
//     並釋放記憶體。**真正刪除**，size() 會變小。
//
// 這就是著名的 erase-remove idiom 存在的理由：
//     v.erase(std::remove(v.begin(), v.end(), 3), v.end());
//              └──── 搬移，回傳新邏輯尾端 ────┘  └─ 真正刪除 ─┘
// 只寫 std::remove 而忘了 erase，是 C++ 最常見的 bug 之一：
// 程式看起來「刪掉了」，size() 卻沒變，尾端還躺著未指定的殘值。
// （C++20 起可以用 std::erase(v, 3) / std::erase_if(v, pred) 一行解決。）
//
// 【2. 對 list 而言，成員版還比較快】
// 即使你正確地寫了 erase-remove idiom，對 list 來說仍是下策：
//   * std::remove 會**搬移元素的值**（對 list 就是逐個賦值），
//     而 list 的元素可能是昂貴的 string / 大型 struct。
//   * list::remove 只改指標，一個元素都不搬。
// 所以規則是：**容器有同名成員函式時，優先用成員函式**。
// 同樣的原則也適用於 lst.sort()（vs std::sort，後者根本無法編譯）。
//
// 【3. unique 只移除「相鄰」重複——最常見的誤用】
// list::unique 的實作是「走一遍，比較目前元素與前一個，相同就刪」。
// 它**只看相鄰**，因為只有這樣才能保證 O(n)。
//     {1,1,2,3,3,3,2,2,1}.unique()  →  {1,2,3,2,1}
//                                        ↑ 1 和 2 都還在，只是不再相鄰重複
// 要做「全域去重」必須**先排序**讓相同的值聚在一起：
//     lst.sort();      // O(n log n)，list 的成員版，只改指標
//     lst.unique();    // O(n)
// 這也解釋了為什麼標準把它命名為 unique 而不是 deduplicate：
// 它的契約是「使相鄰元素唯一」，不是「使容器內的值唯一」。
// std::unique 演算法也是完全一樣的語意（並同樣只搬移、不刪除）。
//
// 【4. remove_if 的判定式與副作用】
// remove_if 接受一元判定式，回傳 true 就刪：
//     lst.remove_if([](const Order& o) { return o.status == Cancelled; });
// 判定式**可以有狀態**（例如「只刪前 3 個符合的」），但要注意：
//   * 標準未規定判定式被呼叫的次數與順序保證到什麼程度，
//     有狀態的判定式雖然在實務上可用（實作就是單純走一遍），
//     但語意上較脆弱；能用無狀態就用無狀態。
//   * 判定式**不可以修改容器**（在遍歷途中插刪自己 → UB）。
//
// 【5. 為什麼這些操作不使其他 iterator 失效】
// 因為 list 刪除節點只是把該節點從鏈上摘下、釋放它，
// 其他節點的位址完全沒動。所以：
//     只有**被刪除元素**的 iterator/reference/pointer 失效，其餘全部有效。
// 這讓「一邊持有外部索引、一邊批次清理」成為安全操作——
// 對 vector 而言同樣的事會讓所有 iterator 全部失效。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼演算法不能改變容器大小（設計層面）
//   STL 的核心設計是「演算法透過 iterator 操作資料，與容器解耦」。
//   iterator 只提供「存取與移動」，**不提供「刪除」**——它根本不知道
//   自己背後是哪個容器，也沒有指向容器物件的指標。
//   這個解耦讓同一份 std::remove 能作用在 vector / deque / array /
//   原生陣列 / 甚至自訂容器上，代價就是它無法真的刪除元素。
//   理解這一點，erase-remove idiom 就不再是需要死背的咒語。
//
// (B) 三個函式的複雜度都是 O(n)，但「比較次數」不同
//   remove(v)     ：每個元素做一次 == 比較
//   remove_if(p)  ：每個元素呼叫一次判定式
//   unique()      ：每個元素與**前一個保留下來的**元素比一次
//   都只走一遍，沒有巢狀迴圈。若先 sort 再 unique，總成本由 sort 主導
//   （O(n log n)）。
//
// (C) unique 的自訂判定式是「二元」的
//   unique(pred) 的 pred 接收**兩個**元素（前一個、目前這個），
//   回傳 true 表示「視為重複」。注意它不必是真正的相等關係：
//   例如 |a-b| < 0.5 這種「近似」判定就不具遞移性，
//   結果會取決於掃描順序（每次比的是「前一個保留下來的」而非原始前鄰）。
//   本檔範例 5 用實際輸出展示這個效果。
//
// (D) C++20 的回傳值有什麼用
//   回傳「移除了幾個」讓呼叫端可以直接記錄或判斷：
//       if (auto n = cache.remove_if(isExpired); n > 0)
//           log("清除了 " + std::to_string(n) + " 筆過期資料");
//   在 C++17 以前只能自己先數一遍（多走一次 O(n)）或比較前後 size()。
//
// 【注意事項 Pay Attention】
//  1. **unique 只移除相鄰重複**；要全域去重必須先 sort。
//  2. **成員 remove ≠ 演算法 std::remove**：前者真刪、後者只搬移，
//     後者必須配 erase（erase-remove idiom）才會真的變小。
//  3. 容器有同名成員函式時**優先用成員版**（對 list 是只改指標，不搬元素）。
//  4. 回傳型別:C++17 以前是 void，**C++20 起才回傳移除個數**。
//     用回傳值請確認編譯標準（本檔用 -std=c++20）。
//  5. 這些操作**不使其他 iterator 失效**，只有被刪的那些失效。
//  6. 判定式不可在執行期間修改該容器（UB）；盡量使用無狀態判定式。
//  7. remove 比較的是 `==`；浮點數直接用 remove(0.1) 幾乎不會如你所願，
//     請改用 remove_if 搭配容差判斷。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】list::unique / remove / remove_if
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::list 的成員 remove()，跟 <algorithm> 的 std::remove()，差在哪裡?
//     答：成員 remove **真的把節點從鏈上摘掉並釋放**，size() 會變小。
//         std::remove 是演算法，只拿得到 iterator、**無法改變容器大小**，
//         它把要保留的元素往前搬，回傳新的邏輯尾端，尾端之後的元素是
//         「有效但未指定」的值，size() 完全沒變 —— 必須自己再呼叫
//         erase(newEnd, end()) 才算真的刪掉，這就是 erase-remove idiom。
//     追問：為什麼演算法不能自己刪?→ 因為 iterator 只提供存取與移動，
//         不知道背後是哪個容器、也沒有容器的指標。這個解耦正是同一份
//         std::remove 能作用在 vector/deque/原生陣列上的原因。
//
// 🔥 Q2. lst.unique() 之後為什麼還有重複的值?
//     答：因為 unique 只移除**相鄰**的重複（這樣才能保證 O(n)）。
//         {1,1,2,3,3,3,2,2,1} → {1,2,3,2,1}，1 和 2 都還在。
//         要全域去重必須先讓相同值聚在一起：lst.sort(); lst.unique();
//     追問：為什麼不直接設計成全域去重?→ 那需要排序或雜湊，
//         不可能是 O(n) 且無額外記憶體。標準把「排序」的決定權留給你，
//         函式名 unique 的契約本來就是「使相鄰元素唯一」。
//
// 🔥 Q3. 已知要對 list 移除某個值，你會用成員版還是 erase-remove idiom?為什麼?
//     答：成員版 lst.remove(v)。即使 erase-remove idiom 寫對了，
//         std::remove 仍會**逐個賦值搬移元素**（元素若是 string 或大型
//         struct 就是實實在在的複製成本）；而 list::remove 只改指標，
//         一個元素都不搬。通則是:**容器提供同名成員函式時優先用成員版**。
//     追問：還有哪些同類例子?→ lst.sort()（std::sort 對 list 根本無法編譯）、
//         lst.reverse()、lst.merge()，以及 map/set 的 find（成員版 O(log n)，
//         std::find 是 O(n)）。
//
// ⚠️ 陷阱. 這段程式為什麼沒有真的刪掉元素?
//        std::vector<int> v = {1,3,2,3,4};
//        std::remove(v.begin(), v.end(), 3);
//        std::cout << v.size();      // 印出 5，不是 3
//     答：std::remove 只把保留的元素往前搬並回傳新邏輯尾端，
//         **完全沒有改變容器大小**。這裡連回傳值都丟掉了，
//         等於做了一次白工搬移。正確寫法：
//             v.erase(std::remove(v.begin(), v.end(), 3), v.end());
//         或 C++20 的 std::erase(v, 3);
//     為什麼會錯：名字叫 remove，直覺就以為「移除」＝「刪掉」。
//         實際上它的語意是「把不要的擠到後面去」。
//         **尾端殘留的值是「有效但未指定」的**，不可以假設它們是什麼
//         （不同實作、不同型別結果都可能不同），更不能拿來用。
//
// ⚠️ 陷阱2. 「先 unique 再 sort」跟「先 sort 再 unique」有差嗎?
//     答：差很多。unique 只看相鄰，先 unique 只會折疊原本就相鄰的重複，
//         之後再 sort 只是把倖存的重複值排在一起 —— **重複依然存在**。
//         正確順序是 sort 之後才 unique。本檔範例 4 有實際輸出佐證。
//     為什麼會錯：把兩個操作看成可交換的「清理步驟」，
//         忽略了 unique 的正確性**依賴輸入已排序**這個前提。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <list>
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>
#include <cmath>
#include <cctype>

// -----------------------------------------------------------------------------
// 工具:印出容器內容（前置空白，避免行尾多餘空白）
// -----------------------------------------------------------------------------
template <typename C>
void print(const std::string& label, const C& c) {
    std::cout << "  " << label << " [" << c.size() << "]:";
    for (const auto& v : c) std::cout << " " << v;
    std::cout << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 203. Remove Linked List Elements
//   題目：刪除鏈結串列中所有等於 val 的節點。
//   為什麼用到本主題：這正是 list::remove(val) 的定義本身——
//     一行解決，而且不需要處理「刪除頭節點」的特例（原生解法要用 dummy head）。
// -----------------------------------------------------------------------------
std::list<int> removeElements(std::list<int> head, int val) {
    head.remove(val);          // 真正摘除節點；C++20 回傳移除個數
    return head;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 83. Remove Duplicates from Sorted List
//   題目：已排序的鏈結串列，讓每個值只出現一次。
//   為什麼用到本主題：**輸入已排序**，所以重複必然相鄰 ——
//     這正是 list::unique() 的前提條件，一行即可。
//     這題也反過來說明了「unique 為何只處理相鄰」是合理的設計：
//     在已排序的資料上，相鄰重複就等於全部重複。
// -----------------------------------------------------------------------------
std::list<int> deleteDuplicates(std::list<int> head) {
    head.unique();             // 輸入已排序 → 相鄰去重即全域去重
    return head;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 3】LeetCode 27. Remove Element
//   題目：原地移除陣列中所有等於 val 的元素，回傳剩餘元素個數 k；
//         前 k 個位置必須是保留下來的元素，k 之後的內容不重要。
//   為什麼用到本主題：這題的規格**就是 std::remove 的語意**——
//     「把要保留的往前搬，回傳新的邏輯尾端，後面的不管」。
//     LeetCode 只檢查前 k 個，正好對應 std::remove 不縮小容器的行為。
//     拿它跟 list::remove 對照，最能看清「演算法 vs 成員函式」的差別。
// -----------------------------------------------------------------------------
int removeElement(std::vector<int>& nums, int val) {
    auto newEnd = std::remove(nums.begin(), nums.end(), val);
    return static_cast<int>(newEnd - nums.begin());   // k；容器大小並未改變
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 4】LeetCode 26. Remove Duplicates from Sorted Array
//   題目：已排序陣列原地去重，回傳新長度 k。
//   為什麼用到本主題：std::unique 與 list::unique 語意相同（只折疊相鄰重複），
//     差別同樣是「演算法只搬移、成員函式真刪除」。
//     這題輸入已排序，所以相鄰去重就是完整去重。
// -----------------------------------------------------------------------------
int removeDuplicates(std::vector<int>& nums) {
    auto newEnd = std::unique(nums.begin(), nums.end());
    return static_cast<int>(newEnd - nums.begin());
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】log 摺疊:壓縮連續重複的訊息
//   情境：服務出問題時常會在一秒內狂噴同一行錯誤。監控系統在顯示前
//         會把**連續重複**的行摺疊成一行，避免洗版。
//   為什麼用 unique：要保留的正是「事件發生的順序」，
//     只有連續重複才該摺疊——不相鄰的同樣訊息代表它「又發生了一次」，
//     必須留著。這是 unique「只看相鄰」剛好正確的典型場景，
//     也說明了為何這裡**不可以**先 sort。
// -----------------------------------------------------------------------------
std::list<std::string> foldRepeatedLogs(std::list<std::string> lines) {
    lines.unique();            // 只折疊連續重複，保留時間順序
    return lines;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】設定檔清理:移除註解/空行，並清掉重複的鍵
//   情境：載入 ini 風格設定檔，要先丟掉註解與空行，
//         再讓同一個鍵只保留一筆（後面重複定義的視為冗餘）。
//   為什麼用 remove_if + unique：
//     remove_if 一次遍歷刪掉所有註解與空行（真正釋放節點）；
//     去除重複鍵則需要「同鍵視為重複」的自訂判定式，
//     且因為要跨越不相鄰的重複，必須先依鍵排序（sort + unique）。
// -----------------------------------------------------------------------------
std::string keyOf(const std::string& line) {
    auto eq = line.find('=');
    return eq == std::string::npos ? line : line.substr(0, eq);
}

std::list<std::string> cleanConfig(std::list<std::string> lines) {
    // 1) 移除註解行與空行（真正刪除節點）
    lines.remove_if([](const std::string& s) {
        return s.empty() || s[0] == '#';
    });
    // 2) 依鍵排序，讓同鍵聚在一起（list 的成員 sort，只改指標）
    lines.sort([](const std::string& a, const std::string& b) {
        return keyOf(a) < keyOf(b);
    });
    // 3) 同鍵只留一筆（unique 的自訂二元判定式）
    lines.unique([](const std::string& a, const std::string& b) {
        return keyOf(a) == keyOf(b);
    });
    return lines;
}

int main() {
    // =========================================================================
    std::cout << "=== 1. remove:移除所有等於某值的元素 ===\n";
    // =========================================================================
    {
        std::list<int> lst = {1,3,2,3,4,3,5,3};
        print("原始         ", lst);
        auto n = lst.remove(3);                  // C++20 回傳移除個數
        print("remove(3)    ", lst);
        std::cout << "  回傳移除個數 = " << n << "（C++20 起才有回傳值）\n";
    }
    {
        std::list<std::string> words = {"apple","banana","apple","cherry","apple"};
        print("原始         ", words);
        auto n = words.remove("apple");
        print("remove(apple)", words);
        std::cout << "  移除 " << n << " 個\n";
    }

    // =========================================================================
    std::cout << "\n=== 2. remove_if:依判定式移除 ===\n";
    // =========================================================================
    {
        std::list<int> lst = {1,2,3,4,5,6,7,8,9,10};
        auto n = lst.remove_if([](int x) { return x % 2 == 0; });
        print("移除偶數     ", lst);
        std::cout << "  移除 " << n << " 個\n";
    }
    {
        struct Student { std::string name; double gpa; };
        std::list<Student> students = {
            {"Alice",3.8},{"Bob",2.1},{"Charlie",3.5},{"David",1.9},{"Eve",3.9}
        };
        auto n = students.remove_if([](const Student& s) { return s.gpa < 2.5; });
        std::cout << "  移除 GPA<2.5 後留下:";
        for (const auto& s : students) std::cout << " " << s.name;
        std::cout << "（移除 " << n << " 人）\n";
    }

    // =========================================================================
    std::cout << "\n=== 3. unique:只移除「相鄰」重複（最常誤解之處）===\n";
    // =========================================================================
    {
        std::list<int> lst = {1,1,2,3,3,3,2,2,1};
        print("原始         ", lst);
        auto n = lst.unique();
        print("unique()     ", lst);
        std::cout << "  移除 " << n << " 個\n";
        std::cout << "  ★ 1 和 2 都還在！unique 只折疊**相鄰**重複\n";
    }

    // =========================================================================
    std::cout << "\n=== 4. sort + unique = 真正的全域去重（順序不可顛倒）===\n";
    // =========================================================================
    {
        std::list<int> lst = {3,1,4,1,5,9,2,6,5,3,5};
        print("原始         ", lst);
        lst.sort();     print("sort()       ", lst);
        lst.unique();   print("unique()     ", lst);
    }
    {
        // 反例:順序顛倒的實際後果
        std::list<int> bad = {3,1,4,1,5,9,2,6,5,3,5};
        bad.unique();   // 先 unique（只有相鄰的會被折疊）
        bad.sort();     // 再 sort
        print("先unique後sort", bad);
        std::cout << "  ★ 重複的 1、3、5 仍在 → 證明順序錯誤會失敗\n";
    }

    // =========================================================================
    std::cout << "\n=== 5. unique 的自訂二元判定式 ===\n";
    // =========================================================================
    {
        std::list<double> lst = {1.0,1.1,1.2,2.5,2.6,3.0,3.3,3.4};
        print("原始         ", lst);
        lst.unique([](double a, double b) { return std::fabs(a - b) < 0.5; });
        print("差<0.5視為重複", lst);
        std::cout << "  ★ 判定式比的是「前一個**保留下來的**元素」與目前元素，\n";
        std::cout << "    近似判定不具遞移性，結果會受掃描順序影響\n";
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

    // =========================================================================
    std::cout << "\n=== 6. 成員 remove vs 演算法 std::remove（本課重點）===\n";
    // =========================================================================
    {
        std::list<int> lst = {1,3,2,3,4,3,5};
        std::cout << "  【成員函式】list::remove(3)\n";
        std::cout << "    移除前 size=" << lst.size();
        lst.remove(3);
        std::cout << " → 移除後 size=" << lst.size() << "（真的變小了）\n";
        print("    結果       ", lst);

        std::vector<int> v = {1,3,2,3,4,3,5};
        std::cout << "  【演算法】std::remove(v.begin(), v.end(), 3)\n";
        std::cout << "    移除前 size=" << v.size();
        auto newEnd = std::remove(v.begin(), v.end(), 3);
        std::cout << " → 呼叫後 size=" << v.size() << "（完全沒變！）\n";
        std::cout << "    但邏輯長度只有 " << (newEnd - v.begin()) << "，有效範圍是:";
        for (auto it = v.begin(); it != newEnd; ++it) std::cout << " " << *it;
        std::cout << "\n";
        std::cout << "    ★ newEnd 之後的元素是「有效但未指定」的值，\n";
        std::cout << "      不可假設其內容，也不可使用（故此處不印出）\n";
        v.erase(newEnd, v.end());          // erase-remove idiom 的第二步
        std::cout << "    補上 erase 後 size=" << v.size() << "（這才算真的刪掉）\n";
        print("    結果       ", v);
    }

    // =========================================================================
    std::cout << "\n=== 7. 迭代器穩定性:只有被刪的失效 ===\n";
    // =========================================================================
    {
        std::list<int> lst = {10,20,30,20,40,20,50};
        auto it10 = lst.begin();
        auto it40 = std::next(lst.begin(), 4);
        std::cout << "  remove 前 *it10=" << *it10 << " *it40=" << *it40 << "\n";
        lst.remove(20);                        // 刪掉所有 20
        std::cout << "  remove 後 *it10=" << *it10 << " *it40=" << *it40
                  << " → 未被刪的 iterator 全部仍有效\n";
        print("  結果       ", lst);
    }

    // =========================================================================
    std::cout << "\n=== 8. LeetCode 203. Remove Linked List Elements ===\n";
    // =========================================================================
    print("原始 val=6   ", std::list<int>{1,2,6,3,4,5,6});
    print("結果         ", removeElements({1,2,6,3,4,5,6}, 6));
    print("全等 val=7   ", removeElements({7,7,7,7}, 7));

    // =========================================================================
    std::cout << "\n=== 9. LeetCode 83. Remove Duplicates from Sorted List ===\n";
    // =========================================================================
    print("原始（已排序）", std::list<int>{1,1,2,3,3});
    print("結果         ", deleteDuplicates({1,1,2,3,3}));
    print("原始（已排序）", std::list<int>{1,1,2});
    print("結果         ", deleteDuplicates({1,1,2}));
    std::cout << "  ★ 輸入已排序 → 相鄰去重即全域去重，unique() 一行解決\n";

    // =========================================================================
    std::cout << "\n=== 10. LeetCode 27. Remove Element（std::remove 的語意）===\n";
    // =========================================================================
    {
        std::vector<int> nums = {3,2,2,3};
        int k = removeElement(nums, 3);
        std::cout << "  nums=[3,2,2,3] val=3 → k=" << k << "，前 k 個為:";
        for (int i = 0; i < k; ++i) std::cout << " " << nums[i];
        std::cout << "\n  容器 size 仍是 " << nums.size()
                  << " → 這正是 std::remove「不縮小容器」的證明\n";
    }

    // =========================================================================
    std::cout << "\n=== 11. LeetCode 26. Remove Duplicates from Sorted Array ===\n";
    // =========================================================================
    {
        std::vector<int> nums = {0,0,1,1,1,2,2,3,3,4};
        int k = removeDuplicates(nums);
        std::cout << "  去重後 k=" << k << "，前 k 個為:";
        for (int i = 0; i < k; ++i) std::cout << " " << nums[i];
        std::cout << "\n";
    }

    // =========================================================================
    std::cout << "\n=== 12. 實務:摺疊連續重複的 log 行 ===\n";
    // =========================================================================
    {
        std::list<std::string> logs = {
            "連線逾時 db-primary", "連線逾時 db-primary", "連線逾時 db-primary",
            "切換至 db-replica",
            "連線逾時 db-primary",          // 又發生一次，不相鄰 → 必須保留
            "健康檢查通過"
        };
        std::cout << "  原始 " << logs.size() << " 行:\n";
        for (const auto& s : logs) std::cout << "    | " << s << "\n";
        auto folded = foldRepeatedLogs(logs);
        std::cout << "  摺疊後 " << folded.size() << " 行:\n";
        for (const auto& s : folded) std::cout << "    | " << s << "\n";
        std::cout << "  ★ 第 5 行的「連線逾時」保留了——它是新的一次事件，\n";
        std::cout << "    這裡刻意**不**先 sort，否則會破壞時間順序\n";
    }

    // =========================================================================
    std::cout << "\n=== 13. 實務:設定檔清理（去註解 + 同鍵去重）===\n";
    // =========================================================================
    {
        std::list<std::string> cfg = {
            "# 資料庫設定",
            "host=10.0.0.1",
            "",
            "port=5432",
            "# 這行是舊的",
            "host=192.168.1.1",          // 重複的 host 鍵
            "timeout=30",
            "port=5433"                  // 重複的 port 鍵
        };
        std::cout << "  原始 " << cfg.size() << " 行\n";
        auto cleaned = cleanConfig(cfg);
        std::cout << "  清理後 " << cleaned.size() << " 行:\n";
        for (const auto& s : cleaned) std::cout << "    | " << s << "\n";
        std::cout << "  ★ 這裡必須 sort + unique:同鍵在原檔中並不相鄰\n";
    }

    // =========================================================================
    std::cout << "\n=== 重點整理 ===\n";
    // =========================================================================
    std::cout << "  1. 成員 remove/remove_if/unique:真正摘除節點，size() 變小\n";
    std::cout << "  2. 演算法 std::remove/std::unique:只搬移，回傳新邏輯尾端，\n";
    std::cout << "     必須配 erase 才真的刪除（erase-remove idiom）\n";
    std::cout << "  3. 演算法無法刪除，是因為 iterator 與容器刻意解耦\n";
    std::cout << "  4. unique 只折疊**相鄰**重複 → 全域去重必須先 sort\n";
    std::cout << "  5. 容器有同名成員函式時優先用成員版（list 只改指標不搬元素）\n";
    std::cout << "  6. 三者皆 O(n)，且不使其他 iterator 失效\n";
    std::cout << "  7. 回傳型別:C++17 以前 void，C++20 起回傳移除個數\n";
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra summary.cpp -o summary
//   ★ 本檔示範 remove/remove_if/unique 的**回傳值（移除個數）**，
//     該回傳值是 C++20 才加入的；在 C++17 以前這些函式回傳 void，
//     以 -std=c++17 編譯本檔會失敗（已用 -pedantic-errors 實測確認）。

// === 預期輸出 ===
// === 1. remove:移除所有等於某值的元素 ===
//   原始          [8]: 1 3 2 3 4 3 5 3
//   remove(3)     [4]: 1 2 4 5
//   回傳移除個數 = 4（C++20 起才有回傳值）
//   原始          [5]: apple banana apple cherry apple
//   remove(apple) [2]: banana cherry
//   移除 3 個
//
// === 2. remove_if:依判定式移除 ===
//   移除偶數      [5]: 1 3 5 7 9
//   移除 5 個
//   移除 GPA<2.5 後留下: Alice Charlie Eve（移除 2 人）
//
// === 3. unique:只移除「相鄰」重複（最常誤解之處）===
//   原始          [9]: 1 1 2 3 3 3 2 2 1
//   unique()      [5]: 1 2 3 2 1
//   移除 4 個
//   ★ 1 和 2 都還在！unique 只折疊**相鄰**重複
//
// === 4. sort + unique = 真正的全域去重（順序不可顛倒）===
//   原始          [11]: 3 1 4 1 5 9 2 6 5 3 5
//   sort()        [11]: 1 1 2 3 3 4 5 5 5 6 9
//   unique()      [7]: 1 2 3 4 5 6 9
//   先unique後sort [11]: 1 1 2 3 3 4 5 5 5 6 9
//   ★ 重複的 1、3、5 仍在 → 證明順序錯誤會失敗
//
// === 5. unique 的自訂二元判定式 ===
//   原始          [8]: 1 1.1 1.2 2.5 2.6 3 3.3 3.4
//   差<0.5視為重複 [3]: 1 2.5 3
//   ★ 判定式比的是「前一個**保留下來的**元素」與目前元素，
//     近似判定不具遞移性，結果會受掃描順序影響
//   原始          [6]: Hello hello HELLO World world Foo
//   忽略大小寫    [3]: Hello World Foo
//
// === 6. 成員 remove vs 演算法 std::remove（本課重點）===
//   【成員函式】list::remove(3)
//     移除前 size=7 → 移除後 size=4（真的變小了）
//       結果        [4]: 1 2 4 5
//   【演算法】std::remove(v.begin(), v.end(), 3)
//     移除前 size=7 → 呼叫後 size=7（完全沒變！）
//     但邏輯長度只有 4，有效範圍是: 1 2 4 5
//     ★ newEnd 之後的元素是「有效但未指定」的值，
//       不可假設其內容，也不可使用（故此處不印出）
//     補上 erase 後 size=4（這才算真的刪掉）
//       結果        [4]: 1 2 4 5
//
// === 7. 迭代器穩定性:只有被刪的失效 ===
//   remove 前 *it10=10 *it40=40
//   remove 後 *it10=10 *it40=40 → 未被刪的 iterator 全部仍有效
//     結果        [4]: 10 30 40 50
//
// === 8. LeetCode 203. Remove Linked List Elements ===
//   原始 val=6    [7]: 1 2 6 3 4 5 6
//   結果          [5]: 1 2 3 4 5
//   全等 val=7    [0]:
//
// === 9. LeetCode 83. Remove Duplicates from Sorted List ===
//   原始（已排序） [5]: 1 1 2 3 3
//   結果          [3]: 1 2 3
//   原始（已排序） [3]: 1 1 2
//   結果          [2]: 1 2
//   ★ 輸入已排序 → 相鄰去重即全域去重，unique() 一行解決
//
// === 10. LeetCode 27. Remove Element（std::remove 的語意）===
//   nums=[3,2,2,3] val=3 → k=2，前 k 個為: 2 2
//   容器 size 仍是 4 → 這正是 std::remove「不縮小容器」的證明
//
// === 11. LeetCode 26. Remove Duplicates from Sorted Array ===
//   去重後 k=5，前 k 個為: 0 1 2 3 4
//
// === 12. 實務:摺疊連續重複的 log 行 ===
//   原始 6 行:
//     | 連線逾時 db-primary
//     | 連線逾時 db-primary
//     | 連線逾時 db-primary
//     | 切換至 db-replica
//     | 連線逾時 db-primary
//     | 健康檢查通過
//   摺疊後 4 行:
//     | 連線逾時 db-primary
//     | 切換至 db-replica
//     | 連線逾時 db-primary
//     | 健康檢查通過
//   ★ 第 5 行的「連線逾時」保留了——它是新的一次事件，
//     這裡刻意**不**先 sort，否則會破壞時間順序
//
// === 13. 實務:設定檔清理（去註解 + 同鍵去重）===
//   原始 8 行
//   清理後 3 行:
//     | host=10.0.0.1
//     | port=5432
//     | timeout=30
//   ★ 這裡必須 sort + unique:同鍵在原檔中並不相鄰
//
// === 重點整理 ===
//   1. 成員 remove/remove_if/unique:真正摘除節點，size() 變小
//   2. 演算法 std::remove/std::unique:只搬移，回傳新邏輯尾端，
//      必須配 erase 才真的刪除（erase-remove idiom）
//   3. 演算法無法刪除，是因為 iterator 與容器刻意解耦
//   4. unique 只折疊**相鄰**重複 → 全域去重必須先 sort
//   5. 容器有同名成員函式時優先用成員版（list 只改指標不搬元素）
//   6. 三者皆 O(n)，且不使其他 iterator 失效
//   7. 回傳型別:C++17 以前 void，C++20 起回傳移除個數
