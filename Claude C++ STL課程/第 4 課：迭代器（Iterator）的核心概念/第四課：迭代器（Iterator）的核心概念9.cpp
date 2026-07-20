// =============================================================================
//  第四課：迭代器的核心概念 9  —  三種安全刪除法：erase 迴圈 / erase-remove / 成員函式
// =============================================================================
//
// 【主題資訊 Information】
//   簽名：
//     ForwardIt std::remove   (ForwardIt first, ForwardIt last, const T& value);
//     ForwardIt std::remove_if(ForwardIt first, ForwardIt last, UnaryPred p);
//     void      list<T>::remove(const T& value);
//     iterator  vector<T>::erase(const_iterator first, const_iterator last);
//   標準版本：remove / remove_if 自 C++98；
//             **C++20 新增 std::erase(cont, val) 與 std::erase_if(cont, pred)**
//             —— 一行取代 erase-remove 慣用法（在 <vector>、<list> 等標頭內宣告）。
//             C++11 起 list::remove 回傳 void；C++20 起改回傳被移除的元素個數。
//   複雜度：
//     方法一（逐一 erase）  vector: O(N×K)、list: O(N)
//     方法二（erase-remove）vector: O(N)
//     方法三（list::remove）list: O(N)，且不搬移元素、不使其他迭代器失效
//
// 【詳細解釋 Explanation】
//
// 【1. std::remove 為什麼「移除」了卻不會變短】
//   這是 STL 最有名的命名陷阱。std::remove 是**演算法**，
//   而演算法只透過迭代器工作 —— 它拿不到容器本身，自然無法改變 size。
//   它實際做的是「壓實（compaction）」：
//       輸入:  1 2 3 2 4 2 5      要移除 2
//       過程:  把不等於 2 的元素依序往前搬
//       結果:  1 3 4 5 | 2 4 2 5   ← '|' 是回傳的 new_end
//               ^^^^^^   ^^^^^^^^
//               有效區    「有效但未指定」的殘留區
//   關鍵：new_end 之後的元素處於 *valid but unspecified* 狀態
//   （C++11 起因為用了移動語意，那些位置可能是被移動走的空殼）。
//   你可以安全地解構或賦值給它們，但不該讀取它們的值。
//   要真正縮短容器，必須自己呼叫成員函式 erase(new_end, end())。
//
// 【2. 為什麼不叫 std::compact？】
//   歷史因素。remove 是 C++98 從 SGI STL 繼承的命名，
//   當時的設計哲學是「演算法只操作區間、不觸碰容器」，
//   命名者認為在區間語意下「移除」就是「把它移到你不會再看的地方」。
//   這個決定造成了三十年的困惑，最終在 **C++20 由 std::erase / std::erase_if 收拾**：
//       std::erase(v, 2);                              // 一行完成
//       std::erase_if(v, [](int n){ return n % 2; });
//   新程式碼在 C++20 以上請直接用這兩個。
//
// 【3. 三種方法的適用時機】
//   方法一：erase 回傳值迴圈
//     優點 —— 唯一適用於**所有**容器（vector/list/deque/set/map）的通用寫法；
//             條件可以依賴位置或外部狀態（不只是元素值）。
//     缺點 —— 對 vector 每次刪除都 O(N) 搬移 → 總共 O(N×K)。
//   方法二：erase-remove 慣用法
//     優點 —— vector/deque 上是 O(N)，只走一趟。
//     缺點 —— 不能用於 set/map（它們不允許重排元素）；
//             會移動元素，因此**所有**迭代器與參考都失效。
//   方法三：容器成員函式 remove / remove_if
//     只有 list / forward_list 提供。它直接解開節點並釋放，
//     完全不搬移元素 → **除了被刪的節點，其餘迭代器/參考全部保持有效**。
//     對 list 一定要用這個，不要用 erase-remove。
//
// 【4. 對 set / map 只能用方法一】
//   關聯容器的元素順序由比較器決定，不能任意重排，
//   因此 std::remove 對它們沒有意義（而且元素是 const 的，根本不能被賦值）。
//   正確寫法是 erase 回傳值迴圈；C++11 起 set/map 的 erase 也會回傳下一個迭代器。
//   C++20 起可用 std::erase_if(m, pred)。
//
// 【概念補充 Concept Deep Dive】
//   「有效但未指定（valid but unspecified）」是標準的專有名詞，值得精確理解：
//     - **有效**：物件仍處於合法狀態，可以安全地解構、賦新值、呼叫無前置條件的成員
//       （例如 str.size()、v.clear()）。
//     - **未指定**：具體內容沒有保證。可能是原值、可能是空字串、可能是別的東西。
//   被移動走的物件（moved-from object）也是這個狀態。
//   所以下面這段是合法但無意義的：
//       auto it = std::remove(v.begin(), v.end(), 2);
//       std::cout << *it;     // 合法（不是 UB），但印出什麼沒有保證
//   而這段才是錯的（真正的 UB）：
//       v.erase(it, v.end());
//       std::cout << *it;     // it 已失效 → UB
//   區分這兩者是理解 STL 記憶體語意的一道分水嶺。
//
// 【注意事項 Pay Attention】
//   1. std::remove **不改變 size**，必須配合 erase 才真的刪除。
//   2. remove 之後 [new_end, end()) 的元素是「有效但未指定」，不要讀它們的值。
//   3. list 請用成員函式 lst.remove(val)：O(N)、不搬移元素、
//      其餘迭代器全部保持有效；用 erase-remove 反而更差。
//   4. set / map 不能用 erase-remove（元素不可重排且為 const），
//      只能用 erase 回傳值迴圈或 C++20 的 std::erase_if。
//   5. C++20 起優先用 std::erase(c, v) / std::erase_if(c, p)，一行且不會寫錯。
//   6. remove_if 的述詞不應有副作用，也不應修改元素。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】三種刪除方式與 erase-remove 慣用法
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::remove 為什麼不會讓容器變短？它到底做了什麼？
//     答：因為它是演算法，只拿到一對迭代器，碰不到容器本身，所以改不了 size。
//         它做的是「壓實」：把不需移除的元素依序往前搬，
//         回傳新的邏輯尾端 new_end。[new_end, end()) 之間的元素
//         處於「有效但未指定」狀態。要真的縮短必須自己呼叫
//         v.erase(new_end, v.end())，這就是 erase-remove 慣用法。
//     追問：C++20 有沒有更好的寫法？
//           → 有，std::erase(v, val) 與 std::erase_if(v, pred)，
//             一行完成且不可能寫錯，是新程式碼的首選。
//
// 🔥 Q2. 對 std::list 該用 erase-remove 還是 lst.remove()？為什麼？
//     答：一定用成員函式 lst.remove()。原因有二：
//         (1) 效能 —— list 是節點式的，成員版直接解開節點並釋放；
//             erase-remove 則會逐一「賦值搬移」元素內容，對 list 毫無意義且更慢。
//         (2) 迭代器有效性 —— 成員版只讓被刪節點的迭代器失效，其餘全部有效；
//             erase-remove 因為搬移了元素內容，會破壞其他迭代器指向的語意。
//     追問：這個「優先用成員函式」的原則還適用在哪裡？
//           → lst.sort()（std::sort 對 list 根本編不過）、
//             set/map 的 s.find()（O(log N) vs std::find 的 O(N)）、
//             以及 lst.unique() / lst.merge()。
//
// ⚠️ 陷阱. auto it = std::remove(v.begin(), v.end(), 2);
//          std::cout << *it << "\n";     —— 這行是不是未定義行為？
//     答：**不是** UB，但輸出沒有任何保證。
//         [new_end, end()) 的元素是「有效但未指定（valid but unspecified）」：
//         物件仍合法存在（可解構、可賦值），只是內容不保證是什麼
//         （C++11 起因為使用移動語意，那裡可能是被移動走的空殼）。
//         真正的 UB 是在 v.erase(it, v.end()) **之後**再解參考 it。
//     為什麼會錯：把「未指定」和「未定義」混為一談。
//         兩者差別很大：未指定是「值不保證」但程式仍有良好定義；
//         未定義是「整個程式的行為都不再有任何保證」。
//         這也是為什麼標準會特地為 moved-from 物件定義出這個中間狀態 ——
//         讓它可以被安全地解構與重新賦值，而不是變成一顆地雷。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <list>
#include <map>
#include <string>
#include <algorithm>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 26. Remove Duplicates from Sorted Array
//   題目：對已排序陣列就地去重，回傳新長度 k，前 k 個位置為去重結果。
//   為什麼用到本主題：std::unique 與 std::remove 是完全同一套機制 ——
//         它也只做「壓實」而不改變 size，回傳新的邏輯尾端。
//         因此這題正是 erase-unique 慣用法（erase-remove 的孿生兄弟）的教科書應用，
//         也再次印證「演算法改不了容器大小」這條規則。
//   複雜度：時間 O(N)、空間 O(1)。
//   前提：陣列必須**已排序** —— unique 只移除「相鄰」的重複，
//         未排序的資料會得到錯誤結果且不會有任何警告。
// -----------------------------------------------------------------------------
int removeDuplicates(std::vector<int>& nums) {
    auto new_end = std::unique(nums.begin(), nums.end());   // 只壓實，size 不變
    int k = static_cast<int>(new_end - nums.begin());
    nums.erase(new_end, nums.end());                        // 這步才真的縮短
    return k;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】三種容器的黑名單清理（示範各自的正確做法）
//   情境：內容審核系統要把命中黑名單的項目從三種資料結構中清掉：
//         - vector<string> 待發佈佇列（要求最快）
//         - list<string>   編輯中的草稿鏈（要求其他迭代器不失效）
//         - map<string,int> 詞頻統計（關聯容器，不能重排）
//   為什麼用到本主題：這三種容器**各自的正確刪除方式都不一樣**，
//         正是本檔三種方法的實際分工。用錯方法不是風格問題，
//         而是效能差一個數量級、或根本編譯不過。
// -----------------------------------------------------------------------------
bool isBlocked(const std::string& word, const std::vector<std::string>& blacklist) {
    return std::find(blacklist.begin(), blacklist.end(), word) != blacklist.end();
}

// (1) vector → erase-remove：O(N)，最快
void cleanQueue(std::vector<std::string>& queue,
                const std::vector<std::string>& blacklist) {
    queue.erase(std::remove_if(queue.begin(), queue.end(),
                               [&blacklist](const std::string& w) {
                                   return isBlocked(w, blacklist);
                               }),
                queue.end());
}

// (2) list → 成員函式 remove_if：O(N)，且不搬移元素、其餘迭代器保持有效
void cleanDrafts(std::list<std::string>& drafts,
                 const std::vector<std::string>& blacklist) {
    drafts.remove_if([&blacklist](const std::string& w) {
        return isBlocked(w, blacklist);
    });
}

// (3) map → erase 回傳值迴圈（關聯容器不能用 erase-remove）
void cleanFrequency(std::map<std::string, int>& freq,
                    const std::vector<std::string>& blacklist) {
    for (auto it = freq.begin(); it != freq.end(); /* 不在這裡 ++ */) {
        if (isBlocked(it->first, blacklist)) {
            it = freq.erase(it);   // C++11 起 map::erase 也回傳下一個迭代器
        } else {
            ++it;
        }
    }
}

int main() {
    // 方法一：使用 erase 的回傳值（適用於所有容器）
    std::cout << "=== 方法一：erase 回傳值 ===" << std::endl;
    std::vector<int> vec1 = {1, 2, 3, 2, 4, 2, 5};

    for (auto it = vec1.begin(); it != vec1.end(); ) {
        if (*it == 2) {
            it = vec1.erase(it);
        } else {
            ++it;
        }
    }

    for (int n : vec1) std::cout << n << " ";
    std::cout << std::endl;

    // 方法二：使用 erase-remove 慣用法（適用於 vector）
    std::cout << "\n=== 方法二：erase-remove 慣用法 ===" << std::endl;
    std::vector<int> vec2 = {1, 2, 3, 2, 4, 2, 5};

    vec2.erase(
        std::remove(vec2.begin(), vec2.end(), 2),
        vec2.end()
    );

    for (int n : vec2) std::cout << n << " ";
    std::cout << std::endl;

    // 方法三：對於 list，使用成員函數 remove（更高效）
    std::cout << "\n=== 方法三：list::remove ===" << std::endl;
    std::list<int> lst = {1, 2, 3, 2, 4, 2, 5};

    lst.remove(2);  // list 有自己的 remove 成員函數

    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;

    // 拆解 erase-remove：看清楚 remove 到底做了什麼
    std::cout << "\n=== 拆解：remove 只壓實，不改 size ===" << std::endl;
    std::vector<int> demo = {1, 2, 3, 2, 4, 2, 5};
    std::cout << "  原始       size=" << demo.size() << "  內容: ";
    for (int n : demo) std::cout << n << " ";
    std::cout << std::endl;

    auto new_end = std::remove(demo.begin(), demo.end(), 2);
    std::cout << "  remove 後  size=" << demo.size() << "  ← 完全沒變" << std::endl;
    std::cout << "  有效區 [begin, new_end) 共 " << (new_end - demo.begin())
              << " 個: ";
    for (auto it = demo.begin(); it != new_end; ++it) std::cout << *it << " ";
    std::cout << std::endl;
    std::cout << "  殘留區 [new_end, end()) 共 " << (demo.end() - new_end)
              << " 個: 「有效但未指定」，不該讀取其值" << std::endl;

    demo.erase(new_end, demo.end());
    std::cout << "  erase 後   size=" << demo.size() << "  內容: ";
    for (int n : demo) std::cout << n << " ";
    std::cout << std::endl;

    // list::remove 不使其他迭代器失效
    std::cout << "\n=== list::remove 的迭代器有效性 ===" << std::endl;
    std::list<int> keep = {10, 20, 30, 40, 50};
    auto it10 = keep.begin();
    auto it50 = std::prev(keep.end());
    std::cout << "  刪除 30 之前: *it10=" << *it10 << " *it50=" << *it50 << std::endl;
    keep.remove(30);
    std::cout << "  刪除 30 之後: *it10=" << *it10 << " *it50=" << *it50
              << "  ← 兩者仍然有效" << std::endl;

    std::cout << "\n=== LeetCode 26. Remove Duplicates from Sorted Array ===" << std::endl;
    std::vector<int> n1 = {1, 1, 2};
    int k1 = removeDuplicates(n1);
    std::cout << "  [1,1,2]               → k=" << k1 << "，內容 = ";
    for (int n : n1) std::cout << n << " ";
    std::cout << std::endl;

    std::vector<int> n2 = {0, 0, 1, 1, 1, 2, 2, 3, 3, 4};
    int k2 = removeDuplicates(n2);
    std::cout << "  [0,0,1,1,1,2,2,3,3,4] → k=" << k2 << "，內容 = ";
    for (int n : n2) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "\n=== 日常實務：三種容器的黑名單清理 ===" << std::endl;
    std::vector<std::string> blacklist = {"spam", "scam", "phish"};

    std::vector<std::string> queue = {"hello", "spam", "world", "scam", "news"};
    std::list<std::string>   drafts = {"draft1", "phish", "draft2", "spam"};
    std::map<std::string, int> freq = {
        {"hello", 12}, {"spam", 340}, {"world", 7}, {"phish", 88}
    };

    std::cout << "  清理前: queue=" << queue.size()
              << " drafts=" << drafts.size()
              << " freq=" << freq.size() << std::endl;

    cleanQueue(queue, blacklist);         // vector → erase-remove
    cleanDrafts(drafts, blacklist);       // list   → 成員函式
    cleanFrequency(freq, blacklist);      // map    → erase 回傳值迴圈

    std::cout << "  清理後: queue=" << queue.size()
              << " drafts=" << drafts.size()
              << " freq=" << freq.size() << std::endl;

    std::cout << "  queue : ";
    for (const std::string& s : queue) std::cout << s << " ";
    std::cout << std::endl;
    std::cout << "  drafts: ";
    for (const std::string& s : drafts) std::cout << s << " ";
    std::cout << std::endl;
    std::cout << "  freq  : ";
    for (const auto& kv : freq) std::cout << kv.first << "(" << kv.second << ") ";
    std::cout << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第四課：迭代器（Iterator）的核心概念9.cpp -o demo9

// === 預期輸出 ===
// === 方法一：erase 回傳值 ===
// 1 3 4 5
//
// === 方法二：erase-remove 慣用法 ===
// 1 3 4 5
//
// === 方法三：list::remove ===
// 1 3 4 5
//
// === 拆解：remove 只壓實，不改 size ===
//   原始       size=7  內容: 1 2 3 2 4 2 5
//   remove 後  size=7  ← 完全沒變
//   有效區 [begin, new_end) 共 4 個: 1 3 4 5
//   殘留區 [new_end, end()) 共 3 個: 「有效但未指定」，不該讀取其值
//   erase 後   size=4  內容: 1 3 4 5
//
// === list::remove 的迭代器有效性 ===
//   刪除 30 之前: *it10=10 *it50=50
//   刪除 30 之後: *it10=10 *it50=50  ← 兩者仍然有效
//
// === LeetCode 26. Remove Duplicates from Sorted Array ===
//   [1,1,2]               → k=2，內容 = 1 2
//   [0,0,1,1,1,2,2,3,3,4] → k=5，內容 = 0 1 2 3 4
//
// === 日常實務：三種容器的黑名單清理 ===
//   清理前: queue=5 drafts=4 freq=4
//   清理後: queue=3 drafts=2 freq=2
//   queue : hello world news
//   drafts: draft1 draft2
//   freq  : hello(12) world(7)
