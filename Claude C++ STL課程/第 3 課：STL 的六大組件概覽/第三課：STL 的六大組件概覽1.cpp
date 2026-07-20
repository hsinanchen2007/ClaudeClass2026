// =============================================================================
//  第 3 課：STL 的六大組件概覽 1  —  四種容器的第一次見面
// =============================================================================
//
// 【主題資訊 Information】
//   std::vector<T>          // 序列容器，連續記憶體，隨機存取 O(1)
//   std::list<T>            // 序列容器，雙向鏈結，任意位置插入刪除 O(1)
//   std::set<T>             // 關聯容器，自動排序、不重複，查找 O(log n)
//   std::map<K, V>          // 關聯容器，鍵值對，依 key 排序，查找 O(log n)
//
//   標頭檔：<vector>、<list>、<set>、<map>
//   本檔標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 STL 要有這麼多容器】
//   因為**沒有一種資料結構在所有操作上都最快**。
//   容器的差異不是「好壞」，而是「把成本放在哪裡」：
//       vector：把成本放在「中間插入刪除」，換來極快的隨機存取與快取局部性
//       list  ：把成本放在「隨機存取」，換來任意位置 O(1) 的插入刪除
//       set   ：把成本放在「每次操作 O(log n)」，換來自動排序與去重
//       map   ：同 set，但每個元素多帶一個 value
//   選容器的正確思路是**先列出你最常做的操作**，再挑那個把成本
//   放在你「很少做的事」上的容器。
//
// 【2. 序列容器 vs 關聯容器：根本差異在「誰決定順序」】
//   序列容器（vector、list、deque）：**你**決定順序。
//       push_back 放尾巴、insert 放指定位置——元素的位置由你的操作決定。
//   關聯容器（set、map）：**容器**決定順序。
//       你只管 insert，它依 key 自動排到該去的位置。
//   這就是為什麼 set 沒有 push_back——「放到尾巴」這個概念對它毫無意義。
//
// 【3. set 的兩個自動行為】
//   本檔的 std::set<int> s = {30, 10, 20, 10, 30}; 展示了兩件事：
//       自動排序：輸出是 10 20 30，不是插入順序
//       自動去重：重複的 10 和 30 被忽略，size 是 3 不是 5
//   兩者都源自它的底層實作——紅黑樹（一種自平衡二元搜尋樹）。
//   樹的中序走訪天然有序；插入時若發現 key 已存在就不插入。
//   ⚠️ 標準只規定 set 的複雜度是 O(log n)，沒有規定必須用紅黑樹，
//      但實務上所有主流實作都是。
//
// 【4. map 的 operator[] 有個重要的副作用】
//   ages["Alice"] = 25; 看起來只是賦值，實際上：
//       若 "Alice" 不存在 → **先插入一個 value 為預設值的元素**，再賦值
//   這代表 operator[] 會**修改容器**，所以它不是 const 成員函式，
//   const map 無法使用。更危險的是查詢時誤用：
//       if (ages["Bob"] == 30)     // 若 Bob 不存在，這行會「新增」一個 Bob！
//   查詢請用 find() 或 count()（C++20 起有更好讀的 contains()）。
//
// 【概念補充 Concept Deep Dive】
//   ● 為什麼 list 有 push_front 而 vector 沒有
//     vector 保證元素連續且從索引 0 開始，前端插入要把全部元素往後推 → O(n)。
//     標準刻意不提供 vector::push_front，就是為了讓「這很貴」變得顯眼——
//     你必須寫 v.insert(v.begin(), x)，那個 insert 就是一個提醒。
//     list 的節點各自獨立，前端插入只是改幾個指標 → O(1)。
//
//   ● 為什麼 map 走訪出來是排序的
//     map 底層是紅黑樹，它的 iterator 做的是**中序走訪**（in-order traversal），
//     而二元搜尋樹的中序走訪必然由小到大。
//     所以本檔的 map 走訪會依 "Alice" < "Bob" < "Charlie" 的字典序輸出，
//     與插入順序無關。
//     若你要「保持插入順序」，map 幫不了你——那需要額外的
//     vector<key> 記錄順序，或改用 unordered_map + 自建順序清單。
//
//   ● std::pair 與結構化綁定
//     map 的元素型別是 std::pair<const Key, Value>。
//     注意 Key 是 **const** 的——因為改 key 會破壞樹的排序不變量。
//     C++17 起可以用結構化綁定寫得更好讀：
//         for (const auto& [name, age] : ages) { ... }
//     本檔兩種寫法都示範。
//
// 【注意事項 Pay Attention】
//   1. map 的 operator[] 在 key 不存在時會**新增元素**；查詢請用 find/count。
//   2. set 與 map 的 key 是 const，不能透過 iterator 修改（會破壞排序）。
//   3. set/map 走訪的順序由 key 決定，**不是插入順序**。
//   4. vector 沒有 push_front；那是刻意的設計，提醒你前端插入是 O(n)。
//   5. 元素順序不重要、只要快速查找時，unordered_set/unordered_map
//      的平均 O(1) 比 set/map 的 O(log n) 更好——但它們不排序。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】STL 容器的選擇
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. vector、list、set、map 各自把成本放在哪裡？怎麼選？
//     答：沒有全能的容器，差別在「成本放在哪」：
//         vector → 隨機存取 O(1)、尾端插入攤銷 O(1)，但中間插刪 O(n)
//         list   → 任意位置插刪 O(1)（要先有 iterator），但沒有隨機存取
//         set    → 自動排序去重、查找 O(log n)，但每次操作都有樹的常數成本
//         map    → 同 set，多帶一個 value
//         選法：先列出**最常做的操作**，挑那個把成本放在你少做的事情上的容器。
//     追問：預設該選哪個？→ vector。它的快取局部性讓它在小到中等規模時
//         常常打敗理論上更適合的容器；有具體理由再換，換完要實測。
//
// 🔥 Q2. std::set 為什麼會自動排序又自動去重？
//     答：底層是紅黑樹（自平衡二元搜尋樹）。
//         排序來自「iterator 做中序走訪」——BST 的中序走訪必然由小到大。
//         去重來自「插入時發現 key 已存在就不插入」。
//         標準只規定 O(log n) 的複雜度，沒有強制紅黑樹，
//         但所有主流實作都是。
//     追問：那 unordered_set 呢？→ 底層是雜湊表，平均 O(1) 查找，
//         但**完全不排序**。要順序用 set，只要快用 unordered_set。
//
// ⚠️ 陷阱. 想檢查 map 裡有沒有某個 key，寫成
//         if (ages["Bob"] == 30) { ... }
//         這行有什麼問題？
//     答：若 "Bob" 不存在，operator[] 會**先插入一個 value 為 0 的 Bob**，
//         再拿它跟 30 比較。你的「查詢」動作把容器改大了一格。
//         在迴圈裡反覆這樣查，map 會不斷長大，而且完全沒有警告。
//         正確寫法：
//             auto it = ages.find("Bob");
//             if (it != ages.end() && it->second == 30) { ... }
//         或 C++20 的 ages.contains("Bob")。
//     為什麼會錯：把 operator[] 想成「唯讀的查詢」，
//         但它的定義是「回傳該 key 對應的 value 的參考，**必要時先建立它**」。
//         這也是為什麼 operator[] 不是 const 成員函式、const map 不能用它——
//         型別系統其實已經在提示你了，只是大部分人沒注意到。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <string>
#include <algorithm>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 217. Contains Duplicate
//   題目：判斷陣列中是否存在重複元素。
//   為什麼用到本主題：這題就是在考「set 會自動去重」這個性質。
//     把全部元素倒進 set，若 set 的大小小於原陣列，就代表有重複被吃掉了。
//     一行判斷，複雜度 O(n log n)。
// -----------------------------------------------------------------------------
bool containsDuplicate(const std::vector<int>& nums) {
    std::set<int> unique(nums.begin(), nums.end());   // 區間建構 + 自動去重
    return unique.size() < nums.size();
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 349. Intersection of Two Arrays
//   題目：回傳兩個陣列的交集，結果中每個元素只能出現一次。
//   為什麼用到本主題：「元素唯一」+「需要快速查找」正是 set 的兩個核心性質。
//     把 nums1 轉成 set 供 O(log n) 查找，再掃 nums2 收集命中的元素；
//     結果也放 set，自動處理「只能出現一次」的要求。
// -----------------------------------------------------------------------------
std::vector<int> intersection(const std::vector<int>& nums1,
                              const std::vector<int>& nums2) {
    std::set<int> pool(nums1.begin(), nums1.end());
    std::set<int> hits;
    for (int x : nums2) {
        if (pool.count(x) > 0) hits.insert(x);        // count 而非 operator[]
    }
    return std::vector<int>(hits.begin(), hits.end());
}

// -----------------------------------------------------------------------------
// 【日常實務範例】伺服器 access log 的錯誤碼統計
//   情境：統計一份 log 裡各 HTTP 狀態碼出現幾次，並找出「不重複的來源 IP」。
//   這個小任務同時用到三種容器，剛好展示各自的定位：
//       vector<string>  → 原始 log 行（順序就是時間順序，由我們決定）
//       map<int, int>   → 狀態碼 → 次數（自動依狀態碼排序，報表好看）
//       set<string>     → 不重複的來源 IP（自動去重 + 排序）
//   注意計數用的是 ++counts[code]——這裡 operator[] 的「不存在就建立
//   （value 預設為 0）」行為正好是我們要的，是它少數的正當用法。
// -----------------------------------------------------------------------------
struct LogReport {
    std::map<int, int> statusCounts;      // 狀態碼 → 次數（自動排序）
    std::set<std::string> uniqueIps;      // 不重複來源 IP（自動去重排序）
    int total = 0;
};

LogReport analyzeLog(const std::vector<std::string>& lines) {
    LogReport r;
    for (const std::string& line : lines) {
        // 格式假設：<ip> <method> <path> <status>
        std::size_t sp1 = line.find(' ');
        std::size_t sp3 = line.rfind(' ');
        if (sp1 == std::string::npos || sp3 == std::string::npos || sp3 <= sp1) continue;

        std::string ip = line.substr(0, sp1);
        int status = 0;
        try {
            status = std::stoi(line.substr(sp3 + 1));
        } catch (const std::exception&) {
            continue;                     // 格式壞掉的行直接跳過
        }

        r.uniqueIps.insert(ip);           // set：重複的 IP 自動被忽略
        ++r.statusCounts[status];         // map：operator[] 不存在時建立 0，正好合用
        ++r.total;
    }
    return r;
}

int main() {
    std::cout << "=== 1. 四種容器的基本樣貌 ===\n";
    // 序列容器：vector
    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::cout << "vector: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    // 序列容器：list
    std::list<int> lst = {10, 20, 30};
    lst.push_front(5);  // list 可以高效地在前端插入
    std::cout << "list: ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;

    // 關聯容器：set（自動排序、不重複）
    std::set<int> s = {30, 10, 20, 10, 30};  // 重複的會被忽略
    std::cout << "set: ";
    for (int n : s) std::cout << n << " ";
    std::cout << std::endl;

    // 關聯容器：map（鍵值對）
    std::map<std::string, int> ages;
    ages["Alice"] = 25;
    ages["Bob"] = 30;
    ages["Charlie"] = 35;
    std::cout << "map: ";
    for (const auto& pair : ages) {
        std::cout << pair.first << "=" << pair.second << " ";
    }
    std::cout << std::endl;

    std::cout << "\n=== 2. set 的兩個自動行為 ===\n";
    std::cout << "插入順序是 {30, 10, 20, 10, 30}，共 5 個值\n";
    std::cout << "set 內容：";
    for (int n : s) std::cout << n << " ";
    std::cout << "→ size = " << s.size() << "\n";
    std::cout << "  自動排序：輸出由小到大，不是插入順序\n";
    std::cout << "  自動去重：重複的 10 與 30 被忽略，5 個值只剩 " << s.size() << " 個\n";
    std::cout << "  兩者都來自底層的紅黑樹（標準只規定 O(log n)，未強制實作方式）\n";

    std::cout << "\n=== 3. map 走訪依 key 排序，不是插入順序 ===\n";
    std::map<std::string, int> scores;
    scores["Zoe"] = 91;      // 先插入 Z
    scores["Adam"] = 78;     // 再插入 A
    scores["Mia"] = 85;
    std::cout << "插入順序：Zoe → Adam → Mia\n";
    std::cout << "走訪順序：";
    // C++17 結構化綁定，比 pair.first / pair.second 好讀
    for (const auto& [name, score] : scores) {
        std::cout << name << "(" << score << ") ";
    }
    std::cout << "\n→ 依 key 的字典序輸出，與插入順序無關\n";

    std::cout << "\n=== 4. ⚠️ map::operator[] 會「新增」元素 ===\n";
    std::map<std::string, int> m;
    m["exists"] = 1;
    std::cout << "初始 size = " << m.size() << "\n";

    // 危險：用 operator[] 查詢一個不存在的 key
    int v = m["missing"];                       // ← 這行會新增一個元素！
    std::cout << "讀取 m[\"missing\"] 得到 " << v
              << "，但 size 變成 " << m.size() << " ← 容器被改大了！\n";

    // 正確：用 find 查詢
    auto it = m.find("also_missing");
    std::cout << "用 find(\"also_missing\")：" 
              << (it == m.end() ? "找不到" : "找到")
              << "，size 仍是 " << m.size() << " ← 沒有副作用\n";
    std::cout << "  另可用 count()（回傳 0 或 1）；C++20 起有更好讀的 contains()\n";
    std::cout << "  這也是為什麼 operator[] 不是 const 成員函式——const map 不能用它\n";

    std::cout << "\n=== 5. 為什麼 vector 沒有 push_front ===\n";
    std::cout << "list.push_front(5) → O(1)，只改幾個指標\n";
    std::cout << "vector 沒有 push_front 這個函式，必須寫 v.insert(v.begin(), x)\n";
    std::vector<int> pv = {1, 2, 3};
    pv.insert(pv.begin(), 0);
    std::cout << "insert(begin(), 0) 後：";
    for (int n : pv) std::cout << n << " ";
    std::cout << "→ O(n)，全部元素往後推一格\n";
    std::cout << "  標準刻意不提供 vector::push_front，就是要讓「這很貴」變得顯眼\n";

    std::cout << "\n=== LeetCode 217. Contains Duplicate ===\n";
    std::cout << "  [1,2,3,1]     → " << std::boolalpha
              << containsDuplicate({1, 2, 3, 1}) << "\n";
    std::cout << "  [1,2,3,4]     → " << containsDuplicate({1, 2, 3, 4}) << "\n";
    std::cout << "  [1,1,1,3,3,4] → " << containsDuplicate({1, 1, 1, 3, 3, 4}) << "\n";
    std::cout << "  → 直接利用 set 的自動去重：size 變小就代表有重複\n";

    std::cout << "\n=== LeetCode 349. Intersection of Two Arrays ===\n";
    auto show = [](const char* label, const std::vector<int>& r) {
        std::cout << "  " << label << "[";
        for (std::size_t i = 0; i < r.size(); ++i)
            std::cout << r[i] << (i + 1 < r.size() ? "," : "");
        std::cout << "]\n";
    };
    show("[1,2,2,1] ∩ [2,2]       = ", intersection({1, 2, 2, 1}, {2, 2}));
    show("[4,9,5] ∩ [9,4,9,8,4]   = ", intersection({4, 9, 5}, {9, 4, 9, 8, 4}));
    std::cout << "  → set 同時解決「快速查找」與「結果不重複」兩個需求\n";

    std::cout << "\n=== 日常實務：access log 統計 ===\n";
    std::vector<std::string> log = {
        "10.0.0.1 GET /api/users 200",
        "10.0.0.7 POST /api/order 500",
        "10.0.0.1 GET /static/app.js 200",
        "10.0.0.3 GET /api/health 503",
        "10.0.0.7 GET /index.html 200",
        "10.0.0.1 POST /api/login 401",
        "壞掉的行",
        "10.0.0.9 GET /api/users 200"
    };

    LogReport rep = analyzeLog(log);
    std::cout << "有效紀錄 " << rep.total << " 筆（格式壞掉的行已跳過）\n";
    std::cout << "狀態碼統計（map 自動依狀態碼排序）：\n";
    for (const auto& [code, count] : rep.statusCounts) {
        std::cout << "  " << code << " → " << count << " 次\n";
    }
    std::cout << "不重複來源 IP（set 自動去重 + 排序）共 "
              << rep.uniqueIps.size() << " 個：";
    for (const std::string& ip : rep.uniqueIps) std::cout << ip << " ";
    std::cout << "\n→ 三種容器各司其職：vector 保留時間順序、\n";
    std::cout << "  map 做有序計數、set 做去重。這就是「選對容器」的實際樣貌。\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第三課：STL 的六大組件概覽1.cpp" -o demo3_1

// === 預期輸出 ===
// === 1. 四種容器的基本樣貌 ===
// vector: 1 2 3 4 5
// list: 5 10 20 30
// set: 10 20 30
// map: Alice=25 Bob=30 Charlie=35
//
// === 2. set 的兩個自動行為 ===
// 插入順序是 {30, 10, 20, 10, 30}，共 5 個值
// set 內容：10 20 30 → size = 3
//   自動排序：輸出由小到大，不是插入順序
//   自動去重：重複的 10 與 30 被忽略，5 個值只剩 3 個
//   兩者都來自底層的紅黑樹（標準只規定 O(log n)，未強制實作方式）
//
// === 3. map 走訪依 key 排序，不是插入順序 ===
// 插入順序：Zoe → Adam → Mia
// 走訪順序：Adam(78) Mia(85) Zoe(91)
// → 依 key 的字典序輸出，與插入順序無關
//
// === 4. ⚠️ map::operator[] 會「新增」元素 ===
// 初始 size = 1
// 讀取 m["missing"] 得到 0，但 size 變成 2 ← 容器被改大了！
// 用 find("also_missing")：找不到，size 仍是 2 ← 沒有副作用
//   另可用 count()（回傳 0 或 1）；C++20 起有更好讀的 contains()
//   這也是為什麼 operator[] 不是 const 成員函式——const map 不能用它
//
// === 5. 為什麼 vector 沒有 push_front ===
// list.push_front(5) → O(1)，只改幾個指標
// vector 沒有 push_front 這個函式，必須寫 v.insert(v.begin(), x)
// insert(begin(), 0) 後：0 1 2 3 → O(n)，全部元素往後推一格
//   標準刻意不提供 vector::push_front，就是要讓「這很貴」變得顯眼
//
// === LeetCode 217. Contains Duplicate ===
//   [1,2,3,1]     → true
//   [1,2,3,4]     → false
//   [1,1,1,3,3,4] → true
//   → 直接利用 set 的自動去重：size 變小就代表有重複
//
// === LeetCode 349. Intersection of Two Arrays ===
//   [1,2,2,1] ∩ [2,2]       = [2]
//   [4,9,5] ∩ [9,4,9,8,4]   = [4,9]
//   → set 同時解決「快速查找」與「結果不重複」兩個需求
//
// === 日常實務：access log 統計 ===
// 有效紀錄 7 筆（格式壞掉的行已跳過）
// 狀態碼統計（map 自動依狀態碼排序）：
//   200 → 4 次
//   401 → 1 次
//   500 → 1 次
//   503 → 1 次
// 不重複來源 IP（set 自動去重 + 排序）共 4 個：10.0.0.1 10.0.0.3 10.0.0.7 10.0.0.9
// → 三種容器各司其職：vector 保留時間順序、
//   map 做有序計數、set 做去重。這就是「選對容器」的實際樣貌。
