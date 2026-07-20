// =============================================================================
//  第七課：演算法（Algorithm）與容器的分離設計1.cpp
//    —  同一個 std::find 用在 vector / list / set / 原生陣列
// =============================================================================
//
// 【主題資訊 Information】
//   template<class InputIt, class T>
//   InputIt find(InputIt first, InputIt last, const T& value);   // <algorithm>
//
//   標準版本：C++98 起（C++20 起加上 constexpr）
//   迭代器需求：Input Iterator（最弱的一級，所有標準容器都滿足）
//   複雜度：最多 last - first 次比較，即 O(N) 線性
//   回傳：第一個等於 value 的迭代器；找不到回傳 last
//   標頭檔：<algorithm>
//
// 【詳細解釋 Explanation】
//
// 【1. 這個檔案在示範什麼「反直覺」的事】
// 直覺上「在 vector 裡找東西」和「在 list 裡找東西」是兩件不同的事：
// vector 是一塊連續記憶體，list 是一串靠指標串起來的節點，記憶體佈局天差地遠。
// 但這裡四種資料結構（vector / list / set / 原生陣列）用的是**同一份 find 程式碼**。
// 這不是巧合，是 STL 最重要的設計決策。
//
// 【2. 為什麼可以共用：find 根本不知道容器的存在】
// 看 find 的實作骨架就懂了：
//     while (first != last) { if (*first == value) return first; ++first; }
//     return last;
// 它只用到四個操作：!=、*、++、回傳。**完全沒有提到 vector、list、set。**
// 只要一個型別支援這幾個操作，find 就能運作，而這組「最小操作集合」
// 就是 Input Iterator 的需求（requirements）。
// 換句話說，find 不是對「容器」寫的，是對「概念（concept）」寫的。
//
// 【3. M×N 變成 M+N：這才是真正的價值】
// 如果不分離，M 個容器 × N 個演算法要寫 M×N 份程式碼：
//     vector::find()  list::find()  set::find()  deque::find() ...
//     vector::sort()  list::sort()  set::sort()  deque::sort() ...
// 10 個容器 × 50 個演算法 = 500 份實作，而且每加一個容器要補 50 個函式。
// 分離之後只要 M + N = 60 份：容器負責「怎麼走訪自己」（提供 iterator），
// 演算法負責「走訪之後做什麼」。兩邊各自演化，互不影響。
// 這就是 Alexander Stepanov 設計 STL 的核心：**用 iterator 當中間協定**。
//
// 【4. 為什麼原生陣列也能用：指標就是迭代器】
//     std::find(arr, arr + 5, 30);
// 這行能編譯，是因為 int* 天生滿足 Input Iterator 的所有需求：
// p != q、*p、++p 全是內建語意。Stepanov 是刻意「照著指標的樣子」
// 設計 iterator 概念的——iterator 是指標的抽象化，不是反過來。
// 所以「指標是最原始的迭代器」不是比喻，是設計上的因果關係。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 編譯期做了什麼：template 為每個型別各生一份程式碼
//   find 是 template，find(vec.begin(),...) 與 find(lst.begin(),...)
//   會實例化（instantiate）成**兩份不同的機器碼**：前者的 ++first 是
//   「指標前進一個元素」，後者是「載入 node->next」。
//   所以「共用」是**原始碼層級的共用**，不是執行期的共用——沒有虛擬函式、
//   沒有間接呼叫、沒有執行期的型別判斷。這叫 static polymorphism（靜態多型），
//   與 Java 用 interface + 動態分派的做法本質不同。
//
// (B) 零抽象成本（zero-overhead abstraction）
//   因為 iterator 通常只是包一個指標或節點指標的薄殼，且方法都可 inline，
//   編譯器最佳化後產生的程式碼與手寫迴圈幾乎相同。抽象在此不需付出代價。
//
// (C) 為什麼 set 用 std::find 是「能用但不該用」
//   std::find 只保證 O(N) 線性掃描，因為它只看得到 iterator，
//   **不知道** set 內部是排序好的樹狀結構。
//   set 自己的 s.find(30) 走樹狀搜尋是 O(log N)。
//   這是分離設計的代價：通用演算法拿不到容器的內部結構知識，
//   所以容器會另外提供「知道自己內部結構」的同名成員函式（見本課第 16 個檔案）。
//   口訣：**成員函式若存在，優先用成員函式。**
//
// 【注意事項 Pay Attention】
// 1. find 回傳的是 iterator，不是索引，也不是 bool。判斷失敗只能與 last 比較：
//      if (it != vec.end())  ← 唯一正確寫法
//    不能寫 if (it)（iterator 不保證可轉 bool），也不能與 nullptr 比。
// 2. **比較對象必須是同一個 range 的 last**。拿 vec.end() 去比 lst 的結果
//    是錯的，而且多半編譯不過（型別不同），這算幸運。
// 3. 只有 vector / array / deque 的 iterator 支援 it - begin() 求索引
//    （需要 Random Access Iterator）。list / set 要用 std::distance(begin, it)，
//    而且那是 O(N)。
// 4. 演算法**不能改變容器大小**——它只有 iterator，拿不到容器本體，
//    沒辦法呼叫 push_back / erase。這是 remove 不會真的刪除元素的根本原因
//    （見本課第 9 個檔案的 erase-remove idiom）。
// 5. set 的元素透過 iterator 取得時是 const 的（改了會破壞排序不變式），
//    所以 set 上不能用 std::sort、std::replace 這類會寫入元素的演算法。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】演算法與容器的分離設計
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 std::find 可以同時用在 vector、list、set 和原生陣列上？
//     答：因為 find 的實作只用到 !=、*、++ 這幾個操作，它接受的是「迭代器」
//         而不是「容器」。任何滿足 Input Iterator 需求的型別都能傳進去，
//         而原生指標天生就滿足這組需求。演算法與容器透過 iterator 這層
//         協定解耦，M 個容器 × N 個演算法從 M×N 份實作降為 M+N 份。
//     追問：那執行期成本呢？→ 零。template 會為每個 iterator 型別各實例化
//         一份程式碼，是編譯期的靜態多型，沒有虛擬函式與動態分派。
//
// 🔥 Q2. std::find(s.begin(), s.end(), x) 和 s.find(x)（s 是 std::set）
//        有什麼差別？該用哪個？
//     答：std::find 是 O(N) 線性掃描；set::find 是 O(log N) 樹狀搜尋。
//         差別來自「知不知道內部結構」：通用演算法只看得到 iterator，
//         不知道 set 是排序好的樹，只能一個一個比。
//         **容器有同名成員函式時一律優先用成員函式。**
//     追問：那為什麼標準不讓 std::find 自動對 set 特化？→ 因為 find 的語意是
//         「用 operator== 逐一比較」，而 set 的搜尋依據是它的比較器
//         （預設 operator< 的等價關係）。兩者判斷「相等」的依據不同，
//         自動替換會改變語意。
//
// ⚠️ 陷阱. 「STL 演算法是通用的，所以任何演算法都能用在任何容器上」——錯在哪？
//     答：錯。通用性受**迭代器分類（iterator category）**限制。
//         find 只要 Input Iterator，所以人人可用；但 std::sort 要求
//         Random Access Iterator，list 只提供 Bidirectional，
//         std::sort(lst.begin(), lst.end()) **編譯就會失敗**。
//     為什麼會錯：多數人把「泛型」理解成「無條件通用」，
//         但 STL 的泛型是「對滿足特定需求的型別通用」。
//         iterator category 就是那份需求清單（見本課第 15 個檔案）。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <list>
#include <set>
#include <string>
#include <algorithm>
#include <iterator>

// -----------------------------------------------------------------------------
// 【日常實務範例 1】掃描多個來源的「已封鎖 IP 清單」
//   情境：防火牆的封鎖名單通常來自多個來源——熱路徑用 vector（連續記憶體、掃得快），
//         動態黑名單用 list（頻繁從中間增刪），永久黑名單用 set（自動去重、已排序）。
//   為什麼用到本主題：檢查函式只吃 iterator，同一份 isBlocked 就能套用在三種
//         截然不同的容器上，不必為每種容器各寫一個版本。
//   注意：實務上對 set 應優先用 s.find()（O(log N)）；這裡刻意用通用版本，
//         示範「同一份程式碼吃各種容器」的能力。
// -----------------------------------------------------------------------------
template <typename Iter>
bool isBlocked(Iter first, Iter last, const std::string& ip) {
    return std::find(first, last, ip) != last;
}

int main() {
    std::cout << "=== 同一個 find 用在不同容器 ===" << std::endl;

    std::vector<int> vec = {10, 20, 30, 40, 50};
    std::list<int> lst = {10, 20, 30, 40, 50};
    std::set<int> s = {10, 20, 30, 40, 50};

    // std::find 只認識迭代器，不認識容器, 所以同一個 find 可以用在 vector、list、set 等不同容器上
    auto v_it = std::find(vec.begin(), vec.end(), 30);
    auto l_it = std::find(lst.begin(), lst.end(), 30);
    auto s_it = std::find(s.begin(), s.end(), 30);

    std::cout << "vector: " << (v_it != vec.end() ? "找到" : "沒找到") << std::endl;
    std::cout << "list: " << (l_it != lst.end() ? "找到" : "沒找到") << std::endl;
    std::cout << "set: " << (s_it != s.end() ? "找到" : "沒找到") << std::endl;

    // 甚至可以用在原生陣列（指標就是迭代器）, 需要提供陣列的開始和結束位置
    int arr[] = {10, 20, 30, 40, 50};
    auto a_it = std::find(arr, arr + 5, 30);
    std::cout << "array: " << (a_it != arr + 5 ? "找到" : "沒找到") << std::endl;

    // ── 只有 Random Access Iterator 才能用 it - begin() 求索引 ──
    std::cout << "\n=== 取得位置：vector 可以相減，list 不行 ===" << std::endl;
    std::cout << "vector 中 30 的索引: " << (v_it - vec.begin()) << std::endl;
    // std::cout << (l_it - lst.begin());  // 編譯錯誤！list 的 iterator 沒有 operator-
    std::cout << "list 中 30 的距離(改用 std::distance, O(N)): "
              << std::distance(lst.begin(), l_it) << std::endl;

    // ── 分離設計的代價：通用 find 不知道 set 是排序好的 ──
    std::cout << "\n=== 同樣是找 30：通用演算法 vs 成員函式 ===" << std::endl;
    auto member_it = s.find(30);          // O(log N)：走樹狀結構
    std::cout << "std::find(set) 線性掃描 O(N)  結果: " << *s_it << std::endl;
    std::cout << "set::find()   樹狀搜尋 O(logN) 結果: " << *member_it << std::endl;

    std::cout << "\n=== 日常實務：掃描已封鎖 IP 清單 ===" << std::endl;
    std::vector<std::string> hotList = {"10.0.0.5", "192.168.1.77"};
    std::list<std::string>   dynamicList = {"172.16.0.9"};
    std::set<std::string>    permanentList = {"203.0.113.1", "198.51.100.4"};

    const std::string probe1 = "192.168.1.77";
    const std::string probe2 = "203.0.113.1";
    const std::string probe3 = "8.8.8.8";

    std::cout << probe1 << " 在熱清單(vector): "
              << (isBlocked(hotList.begin(), hotList.end(), probe1) ? "封鎖" : "放行") << std::endl;
    std::cout << probe2 << " 在永久清單(set): "
              << (isBlocked(permanentList.begin(), permanentList.end(), probe2) ? "封鎖" : "放行") << std::endl;
    std::cout << probe3 << " 在動態清單(list): "
              << (isBlocked(dynamicList.begin(), dynamicList.end(), probe3) ? "封鎖" : "放行") << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第七課：演算法（Algorithm）與容器的分離設計1.cpp -o demo1

// === 預期輸出 ===
// === 同一個 find 用在不同容器 ===
// vector: 找到
// list: 找到
// set: 找到
// array: 找到
//
// === 取得位置：vector 可以相減，list 不行 ===
// vector 中 30 的索引: 2
// list 中 30 的距離(改用 std::distance, O(N)): 2
//
// === 同樣是找 30：通用演算法 vs 成員函式 ===
// std::find(set) 線性掃描 O(N)  結果: 30
// set::find()   樹狀搜尋 O(logN) 結果: 30
//
// === 日常實務：掃描已封鎖 IP 清單 ===
// 192.168.1.77 在熱清單(vector): 封鎖
// 203.0.113.1 在永久清單(set): 封鎖
// 8.8.8.8 在動態清單(list): 放行
