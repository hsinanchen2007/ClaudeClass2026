/*=============================================================================
 * 檔名：25_StlWithCustomClass.cpp
 * 主題：把自訂類別放進 STL 容器 (vector / set / map / unordered_*)
 * 適合：學完模板，要實際把 OOP 套到工作上的人
 *
 * 【課題介紹】
 *   STL 提供了大量好用的容器：vector / list / deque / set / map / unordered_set / ...。
 *   你可以裝 int、string，當然也能裝你自己寫的類別。但每種容器對「元素的型別」
 *   會有不同的要求 — 沒符合就編譯不過。本篇整理幾個常見規則：
 *
 *   ┌─────────────────────┬──────────────────────────────────────────────┐
 *   │ 容器                 │ 對元素型別的要求                               │
 *   ├─────────────────────┼──────────────────────────────────────────────┤
 *   │ vector / list /     │ 大多沒有特殊要求；要可複製或可移動。              │
 *   │ deque / array       │ (push_back 對大型物件建議移動)                  │
 *   ├─────────────────────┼──────────────────────────────────────────────┤
 *   │ set / map           │ 內部用平衡 BST，需要「strict weak ordering」。   │
 *   │                     │ → 提供 operator< 或自訂 comparator              │
 *   ├─────────────────────┼──────────────────────────────────────────────┤
 *   │ unordered_set / map │ 雜湊表，需要 hash 函式 + operator==              │
 *   │                     │ → 自訂類別要特化 std::hash 或自寫 hasher        │
 *   └─────────────────────┴──────────────────────────────────────────────┘
 *
 *   也就是說：
 *     - 若你的類別會被放進 set 或當 map 的 key → 提供 operator<。
 *     - 若會放進 unordered_set 或當 unordered_map 的 key → 提供 operator== 與 hash。
 *
 * 【對應 Leetcode】1845. Seat Reservation Manager
 *   題目簡述：
 *     有 n 個座位 (1..n)，提供：
 *       - reserve()             → 回傳「目前可用的最小座位號」並把它標為已預約
 *       - unreserve(seatNumber) → 把該座位變回可用
 *   為什麼選這題：
 *     最自然的做法就是用 std::set<int> 維護「目前可用的座位集合」 — 這正好
 *     展示了「STL 內建型別 (int) 加上 set」的標準解；不同設計者可能會挑
 *     不同容器，本檔順便比較幾種選擇。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/container
 *=============================================================================*/

/*
補充筆記：StlWithCustomClass
  - StlWithCustomClass 這類 OOP 範例要追蹤物件狀態：建構後是否有效、操作後是否仍符合類別承諾。
  - 如果類別擁有資源，就要檢查 destructor、copy、move 是否表達同一套所有權規則。
  - 繼承、friend、static、operator overload 都應服務於清楚的物件語意，而不是只展示語法。
  - 自訂類別放進 STL 容器前，要先確認它能不能 copy、move、比較、hash，取決於你要使用哪種容器和演算法。
  - std::vector<T> 在擴容時可能 move 或 copy 元素；若 T 的 move constructor noexcept，vector 較容易選擇搬移。
  - std::set/std::map 需要排序規則，operator< 或 comparator 必須符合 strict weak ordering。
  - std::unordered_set/std::unordered_map 需要 hash function 和 equality function；相等的物件必須產生相同 hash。
  - 若類別有 private 成員，operator<<、operator==、hash 等外部輔助函式需要透過 public 介面，或被宣告為 friend。
  - 容器保存的是物件值時，插入可能發生複製或搬移；保存 smart pointer 時，容器保存的是所有權或引用關係。
  - emplace 直接在容器內建構元素，能省去某些暫時物件；但若參數太複雜，push_back 明確建立物件反而更易讀。
  - 自訂類別若有不變條件，容器操作後仍要保持；例如排序 key 不應在放入 set 後被修改，否則容器內部順序會失效。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】把自訂類別放進 STL 容器
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 讓自訂 class 能放進 `std::map` / `std::unordered_map` 各需要什麼？
//     答：`std::map`／`std::set`（有序，內部平衡 BST）需要一個滿足「嚴格弱序
//         （strict weak ordering）」的比較 —— 提供 `operator<` 或自訂 comparator。
//         `std::unordered_map`／`std::unordered_set`（雜湊表）需要
//         「hash 函式（特化 std::hash 或自寫 hasher）＋ `operator==`」，
//         本檔的 PersonEq + PersonHash 就是後者。
//     追問：hash 有什麼硬性要求？（相等的物件「必須」產生相同的 hash；
//           反過來不同物件 hash 相同只是碰撞，合法但影響效能）
//
// 🔥 Q2. `std::vector` 擴容時，元素是被 copy 還是 move？
//     答：看元素的 move constructor 有沒有標 `noexcept`。vector 擴容要維持
//         strong exception guarantee，若 move 可能拋例外，搬到一半失敗就無法還原，
//         只好退回用 copy。所以自訂類別若持有資源，move 操作務必標 noexcept。
//     追問：`emplace_back` 和 `push_back` 差在哪？（emplace 直接在容器內就地建構，
//           省掉一個暫時物件；但參數一複雜反而不易讀，這時 push_back 更清楚）
//
// ⚠️ 陷阱. `operator<` 不小心寫成 `<=` 會怎樣？
//     答：違反嚴格弱序 —— 嚴格弱序要求「不可反身」，也就是 comp(a, a) 必須為 false，
//         而 `<=` 對自己會回傳 true。結果是 undefined behavior：`std::set` / `map`
//         的行為錯亂（插入重複元素、找不到明明存在的 key），嚴重時直接崩潰。
//     為什麼會錯：多數人把它想成「只是排序方向或邊界差一點」，以為頂多順序怪怪的。
//         實際上標準容器的內部不變條件建立在嚴格弱序上，一旦違反就不是「結果不對」
//         而是 UB。同理，物件放進 set 之後也絕不能修改參與比較的欄位。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <set>
#include <unordered_set>
#include <string>
#include <algorithm>     // std::sort

// -----------------------------------------------------------------------------
// 範例 1：自訂類別 Person 放進 vector，並用 std::sort 排序
// -----------------------------------------------------------------------------
struct Person {
    std::string name;
    int         age;

    // 為了能用 std::sort 預設規則，提供 operator<
    bool operator<(const Person& other) const {
        return age < other.age;     // 用 age 由小到大排
    }
};

// 也可以「不在類別內提供 operator<」，改傳一個 comparator 給 std::sort
auto byNameAsc = [](const Person& a, const Person& b) {
    return a.name < b.name;
};

// -----------------------------------------------------------------------------
// 範例 2：把 Person 當 std::set 的元素 — 必須有 operator<
// -----------------------------------------------------------------------------
// (用同一個 Person 結構即可；上面已提供 operator<)

// -----------------------------------------------------------------------------
// 範例 3：把 Person 當 unordered_set 的元素 — 需要 operator== 與 hash
// -----------------------------------------------------------------------------
struct PersonEq {
    std::string name;
    int         age;
    bool operator==(const PersonEq& o) const {
        return name == o.name && age == o.age;
    }
};

// 自寫 hash：把成員 hash 起來再合併
struct PersonHash {
    size_t operator()(const PersonEq& p) const {
        return std::hash<std::string>{}(p.name) ^ (std::hash<int>{}(p.age) << 1);
    }
};

// -----------------------------------------------------------------------------
// 範例 4：對應 Leetcode 1845 - Seat Reservation Manager
// -----------------------------------------------------------------------------
// 設計：用 std::set<int> 存「可用座位」，set 預設由小到大且支援 O(log n)
//       insert / erase / 取最小 (begin())。
class SeatManager {
private:
    std::set<int> available_;
public:
    explicit SeatManager(int n) {
        for (int i = 1; i <= n; ++i) available_.insert(i);
    }

    int reserve() {
        // *available_.begin() 取出目前最小，erase 掉它
        int seat = *available_.begin();
        available_.erase(available_.begin());
        return seat;
    }

    void unreserve(int seatNumber) {
        available_.insert(seatNumber);
    }
};

// -----------------------------------------------------------------------------
// 範例 5：對應 Leetcode 1656 - Design an Ordered Stream
// -----------------------------------------------------------------------------
// 題目簡述：插入 (idKey, value)，回傳「從 ptr 開始連續已填滿」的字串。
// 用 std::vector<std::string> 當底層儲存，是「自訂類別放進 vector」的最簡形式。
class OrderedStream {
private:
    std::vector<std::string> data_;
    int                      ptr_;
public:
    explicit OrderedStream(int n) : data_(n + 1), ptr_(1) {}

    std::vector<std::string> insert(int idKey, std::string value) {
        data_[idKey] = std::move(value);
        std::vector<std::string> out;
        while (ptr_ < static_cast<int>(data_.size()) && !data_[ptr_].empty()) {
            out.push_back(data_[ptr_]);
            ++ptr_;
        }
        return out;
    }
};

// -----------------------------------------------------------------------------
// 範例 6：日常實用 - Event 排序 (用 priority_queue 模擬)
// -----------------------------------------------------------------------------
// 工作上常見：把事件 (有時間戳記) 依時間排序處理。
struct Event {
    long        timestamp;
    std::string message;
    // 為了能放進 std::set，提供 operator<
    bool operator<(const Event& other) const { return timestamp < other.timestamp; }
};

int main() {
    std::cout << "===== 範例 1：vector + sort =====" << std::endl;
    std::vector<Person> v = {{"Alice", 30}, {"Bob", 25}, {"Charlie", 35}};
    std::sort(v.begin(), v.end());      // 用 operator< (依年齡)
    for (const auto& p : v) std::cout << "  " << p.name << "(" << p.age << ")\n";
    std::sort(v.begin(), v.end(), byNameAsc);  // 改成依姓名
    std::cout << "--- 改依名字排序 ---\n";
    for (const auto& p : v) std::cout << "  " << p.name << "(" << p.age << ")\n";

    std::cout << "===== 範例 2：set<Person> =====" << std::endl;
    std::set<Person> s = {{"Eve", 22}, {"David", 40}, {"Eve", 22}};   // 重複會被去除
    for (const auto& p : s) std::cout << "  " << p.name << "(" << p.age << ")\n";

    std::cout << "===== 範例 3：unordered_set<PersonEq> =====" << std::endl;
    std::unordered_set<PersonEq, PersonHash> us;
    us.insert({"Alice", 30});
    us.insert({"Bob",   25});
    us.insert({"Alice", 30});           // 重複；因為 operator== + hash 一樣
    std::cout << "  size = " << us.size() << "\n";   // 預期 2
    std::cout << "  contains Alice30? "
              << (us.count({"Alice", 30}) ? "yes" : "no") << "\n";

    std::cout << "===== 範例 4：Leetcode 1845 =====" << std::endl;
    SeatManager sm(5);
    std::cout << sm.reserve() << "\n";  // 1
    std::cout << sm.reserve() << "\n";  // 2
    sm.unreserve(2);
    std::cout << sm.reserve() << "\n";  // 2 (剛剛被退掉，又是最小)
    std::cout << sm.reserve() << "\n";  // 3
    std::cout << sm.reserve() << "\n";  // 4
    std::cout << sm.reserve() << "\n";  // 5
    sm.unreserve(5);
    std::cout << sm.reserve() << "\n";  // 5

    std::cout << "===== 範例 5：Leetcode 1656 OrderedStream =====" << std::endl;
    OrderedStream os(5);
    auto print = [](const std::vector<std::string>& v) {
        std::cout << "  [";
        for (const auto& s : v) std::cout << "\"" << s << "\" ";
        std::cout << "]\n";
    };
    print(os.insert(3, "ccccc"));
    print(os.insert(1, "aaaaa"));
    print(os.insert(2, "bbbbb"));
    print(os.insert(5, "eeeee"));
    print(os.insert(4, "ddddd"));

    std::cout << "===== 範例 6：set<Event> 依時間排序 =====" << std::endl;
    std::set<Event> events;
    events.insert({200, "save"});
    events.insert({100, "open"});
    events.insert({150, "edit"});
    for (const auto& e : events) {
        std::cout << "  " << e.timestamp << ": " << e.message << "\n";
    }
    return 0;
}

/* 預期輸出：
 * ===== 範例 1：vector + sort =====
 *   Bob(25)
 *   Alice(30)
 *   Charlie(35)
 * --- 改依名字排序 ---
 *   Alice(30)
 *   Bob(25)
 *   Charlie(35)
 * ===== 範例 2：set<Person> =====
 *   Eve(22)
 *   David(40)
 * ===== 範例 3：unordered_set<PersonEq> =====
 *   size = 2
 *   contains Alice30? yes
 * ===== 範例 4：Leetcode 1845 =====
 * 1
 * 2
 * 2
 * 3
 * 4
 * 5
 * 5
 * ===== 範例 5：Leetcode 1656 OrderedStream =====
 *   []
 *   ["aaaaa" ]
 *   ["bbbbb" "ccccc" ]
 *   []
 *   ["ddddd" "eeeee" ]
 * ===== 範例 6：set<Event> 依時間排序 =====
 *   100: open
 *   150: edit
 *   200: save
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. vector / list / deque：對自訂類別幾乎沒要求，可正常裝。
 *   2. set / map：用平衡 BST，需要 operator< 或自訂 comparator。
 *   3. unordered_set / unordered_map：需要 operator== 與 hash 函式。
 *   4. set 的 *begin() 取最小、*rbegin() 取最大；常用於「需要排序的線上集合」。
 *   5. 排序自訂順序：直接傳 lambda / functor 給 std::sort 比改 operator< 彈性。
 *
 * 【下一篇預告】
 *   26_Singleton.cpp
 *   單例模式 (Singleton) — 全域只有一個物件的設計，
 *   並用「Logger 紀錄器」這個工作上常見的需求來示範。
 *=============================================================================*/

// 編譯: g++ -std=c++20 -Wall -Wextra 25_StlWithCustomClass.cpp -o 25_StlWithCustomClass

// === 預期輸出 (節錄) ===
// ===== 範例 1：vector + sort =====
//   Bob(25)
//   Alice(30)
//   Charlie(35)
// --- 改依名字排序 ---
//   Alice(30)
//   Bob(25)
//   Charlie(35)
// ===== 範例 2：set<Person> =====
//   Eve(22)
//   David(40)
// ===== 範例 3：unordered_set<PersonEq> =====
//   size = 2
//   contains Alice30? yes
// ===== 範例 4：Leetcode 1845 =====
// 1
// 2
// 2
// 3
// 4
// …（後略，完整輸出共 32 行）
