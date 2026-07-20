// =============================================================================
//  第 13 課：vector 元素新增：push_back、emplace_back4.cpp
//    —  多參數建構：emplace_back 真正無可取代的場景
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<vector>
//   template<class... Args>
//   reference emplace_back(Args&&... args);   // C++17 起回傳 reference
//   複雜度：攤銷 O(1)
//
//   當元素型別的建構子需要**兩個以上**參數時，push_back 沒有辦法只交零件——
//   它的參數型別是 T，你一定得先組出一個完整的 T。emplace_back 則可以把
//   任意數量的建構子參數直接交出去。
//
// 【詳細解釋 Explanation】
//
// 【1. 三種寫法，三種成本】
//   people.emplace_back("Alice", 30, 165.5);
//       把三個零件轉發進去，就地呼叫 Person(const string&, int, double)。
//       只有一次建構。
//
//   people.push_back(Person("Bob", 25, 175.0));
//       ① 先在堆疊上建構 Person("Bob",...)
//       ② 這是 rvalue → 移動建構進 vector
//       ③ 臨時物件解構
//       兩次建構動作（一般 + 移動）。
//
//   people.push_back({"Charlie", 35, 180.0});
//       大括號看起來很像「就地建構」，但它其實只是
//       **用 list-initialization 造一個臨時 Person**，然後照樣移動進去。
//       成本與上一種完全相同。這是很多人以為「加了大括號就會就地建構」
//       的誤解來源。
//
// 【2. 為什麼 push_back({...}) 不等於 emplace_back(...)】
// push_back 的簽章是 push_back(const T&) 或 push_back(T&&)。
// 你寫 {"Charlie", 35, 180.0} 時，編譯器必須先把這個 braced-init-list
// 轉換成一個 T（也就是 Person）才能傳進去——這個轉換就是「建構一個臨時物件」。
// emplace_back 的簽章是 emplace_back(Args&&...)，三個引數各自獨立轉發，
// 從頭到尾沒有 Person 這個型別的臨時物件存在過。
//
// 【3. 但大括號有一個 emplace_back 沒有的優點：窄化檢查】
// list-initialization 會做 narrowing 檢查：
//     people.push_back({"Dave", 40.7, 180.0});   // ✗ 編譯失敗：double → int 窄化
//     people.emplace_back("Dave", 40.7, 180.0);  // ✓ 編過！age 靜默變成 40
// emplace_back 走的是 direct-init，**不做窄化檢查**。
// 所以「效能較好」與「型別較安全」在這裡是互相衝突的取捨，不是單方面的勝出。
//
// 【概念補充 Concept Deep Dive】
// 為什麼 Person 的移動建構子要標 noexcept？
// 本例 reserve(3) 剛好裝滿，不會擴容，所以看不出差別。但如果 push 超過 3 個，
// vector 擴容時會用 std::move_if_noexcept 決定怎麼搬既有元素：
//     * move constructor 是 noexcept → 用 move（string 只搬指標，快）
//     * move constructor 可能拋例外  → **退化成 copy**（string 整段深複製，慢）
// 原因是強例外保證：搬到一半拋例外時，copy 沒有破壞來源、可以回滾，
// move 卻已經把來源掏空、回不去了。標準庫因此寧可選擇比較慢但安全的 copy。
//
// 實務結論：任何自訂型別，只要 move constructor 真的不會拋例外，
// 就一定要標 noexcept。忘了標不會有任何警告，只會在擴容時默默變慢。
//
// 【注意事項 Pay Attention】
// 1. push_back({a, b, c}) **不是**就地建構，它一樣會造臨時物件再移動。
// 2. emplace_back 不做 narrowing 檢查；push_back({...}) 會做。
//    要型別安全就用大括號，要效能就用 emplace_back，這是取捨。
// 3. Person 的複製建構子印出的是 other.name（複製完成後兩邊都有值）；
//    移動建構子印出的 name 是**移動後的自己**，來源已被掏空。
// 4. move constructor 一定要標 noexcept，否則擴容退化成深複製。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】多參數建構與 emplace_back
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼情況下 emplace_back 一定比 push_back 有優勢？
//     答：元素型別的建構子需要兩個以上參數、而你手上只有那些零散的參數時。
//         push_back 的參數型別是 T，你被迫先組出一個完整的 T（臨時物件），
//         emplace_back 則可把參數直接轉發給建構子，省下臨時物件的
//         建構、移動與解構。
//     追問：那如果建構子只有一個參數呢？
//         → 仍然有優勢（省臨時物件），但前提是你傳的是「建構子參數」
//           而不是「已經造好的 T」。傳現成的 T 時兩者等價。
//
// ⚠️ 陷阱 Q2. people.push_back({"Charlie", 35, 180.0}) 是就地建構嗎？
//     答：不是。大括號只是 list-initialization 的語法，編譯器必須先把它
//         轉換成一個 Person 臨時物件，才能傳給 push_back(T&&)，
//         接著再移動進 vector。成本與 push_back(Person(...)) 完全相同，
//         實測輸出同樣是「建構 + 移動」兩行。
//     為什麼會錯：把大括號的簡潔語法誤讀成「直接在容器裡建構」。
//         判斷標準很簡單——看函式的參數型別：push_back 收的是 T，
//         所以無論你怎麼寫，那個 T 都必須先存在。
//
// ⚠️ 陷阱 Q3. 既然 emplace_back 較快，多參數時是不是永遠該用它？
//     答：效能上是，但要知道你放棄了什麼：push_back({...}) 的 list-init
//         會做 **narrowing 檢查**，把 40.7 傳給 int age 會編譯失敗；
//         emplace_back("Dave", 40.7, 180.0) 用 direct-init，**不檢查**，
//         age 會靜默變成 40，錯誤延後到執行期才被發現。
//     為什麼會錯：只比較效能，沒注意到兩者的初始化語意根本不同
//         （copy-init/list-init vs direct-init）。
//         參數多又型別相近時（int/double/size_t 混在一起），
//         這個「靜默截斷」的風險是真實存在的。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <string>
#include <algorithm>

struct Person {
    std::string name;
    int age;
    double height;

    Person(const std::string& n, int a, double h)
        : name(n), age(a), height(h) {
        std::cout << "建構 Person: " << name << std::endl;
    }

    Person(const Person& other)
        : name(other.name), age(other.age), height(other.height) {
        std::cout << "複製 Person: " << name << std::endl;
    }

    // noexcept：擴容時才會用 move 而非退化成 copy
    Person(Person&& other) noexcept
        : name(std::move(other.name)), age(other.age), height(other.height) {
        std::cout << "移動 Person: " << name << std::endl;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 56. Merge Intervals
//   題目：給一組區間 [start, end]，合併所有重疊的區間後回傳。
//   為什麼用到本主題：合併結果每一項都是「由 start、end 兩個零件組成的新區間」，
//                     正是多參數 emplace_back 的典型場景——
//                     emplace_back(s, e) 直接就地建構，
//                     不必先寫出 push_back(vector<int>{s, e}) 造臨時物件。
// -----------------------------------------------------------------------------
struct Interval {
    int start;
    int end;
    Interval(int s, int e) : start(s), end(e) {}
};

std::vector<Interval> mergeIntervals(std::vector<Interval> intervals) {
    std::vector<Interval> merged;
    if (intervals.empty()) return merged;

    // 依起點排序，之後只需一次線性掃描
    std::sort(intervals.begin(), intervals.end(),
              [](const Interval& a, const Interval& b) {
                  return a.start < b.start;
              });

    merged.reserve(intervals.size());       // 最壞情況完全不重疊
    merged.emplace_back(intervals[0].start, intervals[0].end);

    for (size_t i = 1; i < intervals.size(); ++i) {
        if (intervals[i].start <= merged.back().end) {
            // 有重疊 → 把最後一段的終點往右延伸
            merged.back().end = std::max(merged.back().end, intervals[i].end);
        } else {
            // 不重疊 → 用兩個零件就地建構新區間
            merged.emplace_back(intervals[i].start, intervals[i].end);
        }
    }
    return merged;
}

int main() {
    std::vector<Person> people;
    people.reserve(3);   // 剛好裝滿 3 個，過程中不會擴容

    std::cout << "=== emplace_back（三個零件直接就地建構）===" << std::endl;
    people.emplace_back("Alice", 30, 165.5);   // 一次建構

    std::cout << "\n=== push_back(Person(...))（先造臨時物件）===" << std::endl;
    people.push_back(Person("Bob", 25, 175.0));  // 建構 + 移動

    std::cout << "\n=== push_back({...})（大括號也是先造臨時物件！）===" << std::endl;
    people.push_back({"Charlie", 35, 180.0});    // 建構 + 移動，成本同上

    std::cout << "\n=== 三人資料 ===" << std::endl;
    for (const Person& p : people) {
        std::cout << "  " << p.name << " age=" << p.age
                  << " height=" << p.height << std::endl;
    }

    std::cout << "\n=== LeetCode 56. Merge Intervals ===" << std::endl;
    std::vector<Interval> input{{1, 3}, {2, 6}, {8, 10}, {15, 18}};
    std::cout << "  輸入: [1,3] [2,6] [8,10] [15,18]" << std::endl;
    std::cout << "  合併: ";
    for (const Interval& iv : mergeIntervals(input)) {
        std::cout << "[" << iv.start << "," << iv.end << "] ";
    }
    std::cout << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 13 課：vector 元素新增：push_back、emplace_back4.cpp" -o multi_arg_emplace

// === 預期輸出 ===
// === emplace_back（三個零件直接就地建構）===
// 建構 Person: Alice
// 
// === push_back(Person(...))（先造臨時物件）===
// 建構 Person: Bob
// 移動 Person: Bob
// 
// === push_back({...})（大括號也是先造臨時物件！）===
// 建構 Person: Charlie
// 移動 Person: Charlie
// 
// === 三人資料 ===
//   Alice age=30 height=165.5
//   Bob age=25 height=175
//   Charlie age=35 height=180
// 
// === LeetCode 56. Merge Intervals ===
//   輸入: [1,3] [2,6] [8,10] [15,18]
//   合併: [1,6] [8,10] [15,18] 
