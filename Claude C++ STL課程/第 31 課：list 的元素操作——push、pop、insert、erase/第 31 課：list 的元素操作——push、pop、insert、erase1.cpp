// =============================================================================
//  第 31 課：list 的元素操作——push、pop、insert、erase1.cpp
//    —  四組核心操作，以及「為什麼 list 的 iterator 幾乎不會失效」
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔: #include <list>
//   結構:   雙向鏈結串列，每個節點含 prev / next / value
//
//   push_back / push_front / pop_back / pop_front     O(1)
//   emplace_back / emplace_front                      O(1)（原地建構）
//   insert(pos, ...)                                  O(1)  ← 已持有 pos 時
//   erase(pos) / erase(first, last)                   O(1) / O(範圍長度)
//   size() / empty()                                  O(1)（C++11 起要求 O(1)）
//   ★ 沒有 operator[]；取第 k 個要 std::advance / std::next，成本 O(k)
//
//   回傳值:
//     insert(pos, v) → 指向**新元素**的 iterator
//     erase(pos)     → 指向**被刪元素的下一個**的 iterator
//
// 【詳細解釋 Explanation】
//
// 【1. O(1) 的前提常被略過】
// 「list 插刪 O(1)、vector O(n)」這句話少了前半句：**前提是你已經持有
// 那個位置的 iterator**。若只知道值或第 k 個，得先 advance 走過去（O(n)），
// 總成本並沒有比 vector 好；而 vector 的搬移是連續 memmove，常數極小。
// list 真正勝出的場景是「iterator 早就在手上」（例如 LRU cache 用 hash map
// 存 iterator）、元素昂貴或不可搬移、以及需要 iterator 穩定保證的時候。
//
// 【2. 失效規則:list 是 STL 中最寬鬆的】
//   insert → **不使任何** iterator / reference / pointer 失效
//   erase  → **只使被刪元素**的 iterator / reference 失效
// 原因是節點各自獨立配置，插刪只改相鄰節點的 prev/next 指標，
// 既有節點的位址從未改變。對照 vector（擴容搬移，iterator 與 reference
// 一起失效）與 deque（頭尾插入 iterator 全失效、reference 仍有效），
// list 的保證最強——這正是它存在的主要理由。
//
// 【3. 迴圈刪除的正確寫法】
//     for (auto it = lst.begin(); it != lst.end(); ) {   // 第三段留空
//         if (要刪) it = lst.erase(it);                  // erase 已幫你前進
//         else      ++it;
//     }
// 若在第三段又寫 ++it，刪除後會多跳一格而漏掉元素。
//
// 【概念補充 Concept Deep Dive】
// list 的節點代價很高：本機 std::list<int> 每節點 24 bytes
// （prev 8 + next 8 + int 4 + padding 4），存 4 bytes 資料付 24 bytes，
// 而且每個節點都是一次獨立的 heap 配置 → 節點散落各處、cache 不友善。
// 小型元素、以遍歷為主的場景請用 vector。
// ★ 24 bytes 為 libstdc++ 64-bit 實測值，非標準規定。
//
// 【注意事項 Pay Attention】
//  1. insert/erase 的 O(1) 以「已持有 iterator」為前提，找位置仍是 O(n)。
//  2. 迴圈刪除寫 `it = lst.erase(it)`，for 第三段留空。
//  3. erase(it++) 對 list 合法、對 vector 是 UB（見面試題）。
//  4. 沒有 operator[]；不能用 std::sort（bidirectional iterator），
//     要排序請用成員函式 lst.sort()。
//  5. 對空 list 呼叫 front()/back()/pop_front()/pop_back() 是 UB。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】list 的元素操作
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 list 的 insert 不會使任何 iterator 失效?
//     答：因為節點各自獨立配置在堆積上，插入只是把新節點的 prev/next 接進
//         既有的兩個節點之間——**既有節點的位址完全沒有改變**，
//         所以指向它們的 iterator / reference / pointer 全部繼續有效。
//         vector 之所以會失效，是因為擴容必須把元素整批搬到新的記憶體。
//     追問：那 erase 呢?→ 只有**被刪除的那個元素**的 iterator/reference
//         失效（它的節點被釋放了），其餘不受影響。
//
// 🔥 Q2. 迴圈中一邊走訪一邊刪除，正確寫法是什麼?為什麼 erase 要回傳 iterator?
//     答：寫成 `for (auto it = lst.begin(); it != lst.end(); )`，
//         刪除時 `it = lst.erase(it)`，不刪時才 `++it`；for 第三段留空。
//         erase 必須回傳下一個 iterator，是因為被刪元素的 iterator 一定失效，
//         若不回傳，呼叫端就沒有任何合法方式繼續往下走。
//     追問：寫成 for(...; ++it) 再在裡面 erase 會怎樣?→ 刪除後 erase 回傳的
//         位置又被 ++it 前進一次，等於每刪一個就跳過一個，結果漏刪。
//
// ⚠️ 陷阱. lst.erase(it++) 對 list 可行嗎?對 vector 呢?
//     答：對 **list 可行**：it++ 先讓 it 前進到下一個節點，再把舊 iterator
//         傳給 erase；erase 只讓被刪的那個失效，it 指的節點毫髮無傷。
//         對 **vector 是 UB**：vector 的 erase 會讓刪除點**之後**所有
//         iterator 失效，而前進後的 it 正好就在那個範圍內。
//     為什麼會錯：把「erase(it++) 是經典慣用法」或「erase(it++) 一律錯」
//         當成通則，其實兩者都只對某一個容器成立。正解是**先確認這是哪個
//         容器的失效規則**。實務上統一寫 `it = lst.erase(it)` 最安全。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <list>
#include <string>
#include <iterator>

template <typename T>
void print(const std::string& label, const std::list<T>& lst) {
    std::cout << "  " << label << " [" << lst.size() << "]:";
    for (const auto& v : lst) std::cout << " " << v;
    std::cout << "\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】待辦事項清單:在指定位置插隊、依條件批次移除
//   情境：任務排程器維護一串待辦工作，可在某個工作之前插隊，
//         也要能一次清掉所有已完成的項目。
//   為什麼用 list：插隊與移除都在已知位置上進行（O(1)），
//     而且移除某些工作**不會讓其他工作的 iterator 失效**——
//     所以外部持有的「目前執行到哪」的游標不會壞掉。
// -----------------------------------------------------------------------------
struct Task {
    std::string name;
    bool        done;
};
std::ostream& operator<<(std::ostream& os, const Task& t) {
    return os << t.name << (t.done ? "(完成)" : "(待辦)");
}

int removeFinished(std::list<Task>& tasks) {
    int n = 0;
    for (auto it = tasks.begin(); it != tasks.end(); ) {
        if (it->done) { it = tasks.erase(it); ++n; }   // erase 回傳下一個
        else            ++it;
    }
    return n;
}

int main() {
    std::cout << "=== 1. 頭尾操作 push / pop ===\n";
    std::list<int> lst;
    lst.push_back(10); lst.push_back(20); lst.push_back(30);
    print("push_back ×3 ", lst);
    lst.push_front(5); lst.push_front(1);
    print("push_front ×2", lst);
    lst.pop_back();
    print("pop_back     ", lst);
    lst.pop_front();
    print("pop_front    ", lst);
    std::cout << "  front=" << lst.front() << " back=" << lst.back() << "\n";

    std::cout << "\n=== 2. emplace:直接在節點上建構 ===\n";
    std::list<std::pair<std::string, int>> scores;
    scores.emplace_back("Alice", 95);
    scores.emplace_back("Bob", 88);
    scores.emplace_front("Charlie", 92);      // 放到最前面
    for (const auto& [name, s] : scores) std::cout << "  " << name << ": " << s << "\n";

    std::cout << "\n=== 3. insert 的重載（回傳指向新元素的 iterator）===\n";
    std::list<int> l2 = {10, 30, 50};
    print("初始         ", l2);
    auto pos = std::next(l2.begin());          // 指向 30
    auto ret = l2.insert(pos, 20);             // 在 30 之前插入
    print("insert(30前,20)", l2);
    std::cout << "  回傳→" << *ret << "，原 iterator 仍指向 " << *pos
              << "（未失效）\n";
    l2.insert(l2.end(), 3, 99);                // 尾端插入 3 個 99
    print("insert 3×99  ", l2);
    l2.insert(l2.begin(), {-2, -1, 0});        // 頭端插入初始化列表
    print("insert{-2,-1,0}", l2);

    std::cout << "\n=== 4. erase:單一與範圍 ===\n";
    std::list<int> l3 = {10, 20, 30, 40, 50, 60, 70};
    print("初始         ", l3);
    auto nxt = l3.erase(std::next(l3.begin(), 2));   // 刪 30
    print("erase(30)    ", l3);
    std::cout << "  回傳→" << *nxt << "（被刪元素的下一個）\n";
    auto f = std::next(l3.begin(), 1);               // 指向 20
    auto l = std::next(l3.begin(), 3);               // 指向 60
    l3.erase(f, l);                                  // 刪 [20,60) = 20,40,50
    print("erase[20,60) ", l3);

    std::cout << "\n=== 5. 迭代器穩定性:list 最寬鬆 ===\n";
    std::list<int> l4 = {100, 200, 300, 400, 500};
    auto it200 = std::next(l4.begin(), 1);
    auto it300 = std::next(l4.begin(), 2);
    auto it400 = std::next(l4.begin(), 3);
    std::cout << "  插入前 *it200=" << *it200 << " *it400=" << *it400 << "\n";
    l4.insert(it300, 250);                           // 在中間插入
    std::cout << "  插入 250 後 *it200=" << *it200 << " *it300=" << *it300
              << " *it400=" << *it400 << " → 全部有效\n";
    l4.erase(it300);                                 // 只刪 300
    std::cout << "  刪除 300 後 *it200=" << *it200 << " *it400=" << *it400
              << " → 其他不受影響\n";
    print("目前內容     ", l4);

    std::cout << "\n=== 6. 迴圈中安全刪除（兩種寫法）===\n";
    std::list<int> l5 = {1,2,3,4,5,6,7,8,9,10};
    print("刪前         ", l5);
    for (auto it = l5.begin(); it != l5.end(); ) {   // 第三段留空
        if (*it % 2 == 0) it = l5.erase(it);
        else              ++it;
    }
    print("it=erase(it) ", l5);

    std::list<int> l5b = {1,2,3,4,5,6,7,8,9,10};
    for (auto it = l5b.begin(); it != l5b.end(); ) {
        if (*it % 2 == 0) l5b.erase(it++);           // 對 list 合法，對 vector 是 UB
        else              ++it;
    }
    print("erase(it++)  ", l5b);
    std::cout << "  ★ 結果相同，但前者對所有序列容器都正確，建議統一使用\n";

    std::cout << "\n=== 7. 實務:待辦清單插隊與清除已完成 ===\n";
    std::list<Task> tasks = {
        {"編譯專案", false}, {"跑單元測試", false},
        {"更新文件", true},  {"部署到測試機", false}, {"寄出週報", true}
    };
    print("原始         ", tasks);
    auto cursor = std::next(tasks.begin(), 3);       // 目前執行到「部署到測試機」
    std::cout << "  游標指向: " << *cursor << "\n";
    tasks.insert(std::next(tasks.begin()), Task{"修正編譯警告", false});  // 插隊
    std::cout << "  插隊後游標仍指向: " << *cursor << "（iterator 未失效）\n";
    int cleared = removeFinished(tasks);
    std::cout << "  清除已完成: " << cleared << " 項\n";
    std::cout << "  清除後游標仍指向: " << *cursor << "\n";
    print("結果         ", tasks);

    std::cout << "\n=== 8. resize / clear ===\n";
    std::list<int> l6 = {10,20,30,40,50};
    l6.resize(3);       print("resize(3)    ", l6);
    l6.resize(6, 99);   print("resize(6,99) ", l6);
    l6.clear();         print("clear        ", l6);
    std::cout << "  empty=" << (l6.empty() ? "是" : "否") << "\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 31 課：list 的元素操作——push、pop、insert、erase1.cpp" -o list_ops

// === 預期輸出 ===
// === 1. 頭尾操作 push / pop ===
//   push_back ×3  [3]: 10 20 30
//   push_front ×2 [5]: 1 5 10 20 30
//   pop_back      [4]: 1 5 10 20
//   pop_front     [3]: 5 10 20
//   front=5 back=20
//
// === 2. emplace:直接在節點上建構 ===
//   Charlie: 92
//   Alice: 95
//   Bob: 88
//
// === 3. insert 的重載（回傳指向新元素的 iterator）===
//   初始          [3]: 10 30 50
//   insert(30前,20) [4]: 10 20 30 50
//   回傳→20，原 iterator 仍指向 30（未失效）
//   insert 3×99   [7]: 10 20 30 50 99 99 99
//   insert{-2,-1,0} [10]: -2 -1 0 10 20 30 50 99 99 99
//
// === 4. erase:單一與範圍 ===
//   初始          [7]: 10 20 30 40 50 60 70
//   erase(30)     [6]: 10 20 40 50 60 70
//   回傳→40（被刪元素的下一個）
//   erase[20,60)  [4]: 10 50 60 70
//
// === 5. 迭代器穩定性:list 最寬鬆 ===
//   插入前 *it200=200 *it400=400
//   插入 250 後 *it200=200 *it300=300 *it400=400 → 全部有效
//   刪除 300 後 *it200=200 *it400=400 → 其他不受影響
//   目前內容      [5]: 100 200 250 400 500
//
// === 6. 迴圈中安全刪除（兩種寫法）===
//   刪前          [10]: 1 2 3 4 5 6 7 8 9 10
//   it=erase(it)  [5]: 1 3 5 7 9
//   erase(it++)   [5]: 1 3 5 7 9
//   ★ 結果相同，但前者對所有序列容器都正確，建議統一使用
//
// === 7. 實務:待辦清單插隊與清除已完成 ===
//   原始          [5]: 編譯專案(待辦) 跑單元測試(待辦) 更新文件(完成) 部署到測試機(待辦) 寄出週報(完成)
//   游標指向: 部署到測試機(待辦)
//   插隊後游標仍指向: 部署到測試機(待辦)（iterator 未失效）
//   清除已完成: 2 項
//   清除後游標仍指向: 部署到測試機(待辦)
//   結果          [4]: 編譯專案(待辦) 修正編譯警告(待辦) 跑單元測試(待辦) 部署到測試機(待辦)
//
// === 8. resize / clear ===
//   resize(3)     [3]: 10 20 30
//   resize(6,99)  [6]: 10 20 30 99 99 99
//   clear         [0]:
//   empty=是
