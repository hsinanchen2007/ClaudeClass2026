// =============================================================================
//  第 17 課-8：erase-remove 慣用法 —— 把 O(n²) 的刪除壓成 O(n)
// =============================================================================
//
// 【主題資訊 Information】
//   template<class ForwardIt, class UnaryPredicate>
//   ForwardIt std::remove_if(ForwardIt first, ForwardIt last, UnaryPredicate p);
//   iterator  vector<T>::erase(const_iterator first, const_iterator last);
//   標準版本：C++98 起；C++20 起可用 std::erase_if(v, pred) 一行完成
//   複雜度：remove_if 對每個元素恰好套用一次述詞、最多一次移動 → O(n)
//           erase(範圍) 只解構尾段、不搬移 → O(刪除個數)
//   標頭檔：<algorithm>、<vector>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼逐一 erase 是 O(n²)】
//   vector 的元素連續存放，刪掉中間一個就必須把它後面的所有元素
//   往前搬一格。刪 k 個元素、每次平均搬 n/2 格 → 總搬移量約 k·n/2。
//   刪掉一半（k = n/2）時就是 n²/4 次搬移。
//   n = 100000 時約 25 億次——這不是理論上的擔憂，是實際會讓
//   請求逾時的效能事故。
//
// 【2. erase-remove 為什麼能壓到 O(n)】
//   關鍵在「只掃一遍、每個保留元素只搬一次」。
//   remove_if 用快慢雙指標：read 一路往前掃，
//   遇到要保留的元素就寫到 write 的位置、write 再前進一格。
//   於是每個保留元素最多被搬動一次，總搬移量 ≤ n。
//   最後 erase(new_end, end()) 砍掉尾段——因為砍的是尾巴，
//   沒有任何元素需要往前補位，這一步只有解構成本。
//   合計 O(n)，而且對 CPU 快取極友善（單向循序存取）。
//
// 【3. remove_if 不改變容器大小，這是必然而非疏漏】
//   <algorithm> 裡的演算法只透過迭代器工作，而迭代器不持有容器，
//   拿不到 size、也叫不到容器的成員函式。
//   所以 remove_if 能做的只有重排元素 + 回傳「保留區到哪為止」。
//   真正縮短容器只有 vector::erase 做得到。
//   這個「演算法與容器分離」是 STL 的核心設計，
//   代價就是刪除必須拆成兩步——名字取得不好，但邏輯是自洽的。
//
// 【4. erase-remove 同樣不會歸還記憶體】
//   erase 只降低 size，capacity 原封不動。
//   一個曾經裝過一百萬筆的 vector，用 erase-remove 清到剩十筆之後，
//   仍然佔著一百萬筆的記憶體。要真的歸還必須另外呼叫 shrink_to_fit()。
//   這是刻意的：避免「刪一個縮一次、加一個又長回來」的反覆配置。
//
// 【5. 什麼時候「不該」用 erase-remove】
//   當刪除時還需要對被刪元素做事（釋放資源、寫稽核日誌、通知外部）時。
//   因為 remove_if 是用後方元素「覆蓋」掉被刪元素，
//   等你拿到 new_end 時，那些元素的內容早就沒了。
//   那種情況只能用逐一 erase（見同課 7.cpp），接受 O(n²) 的代價。
//
// 【概念補充 Concept Deep Dive】
//   ▸ remove_if 的內部只有五行
//       ForwardIt result = first;
//       for (; first != last; ++first)
//           if (!p(*first)) *result++ = std::move(*first);
//       return result;
//     這正是 LeetCode 上大量「原地移除」題要你手寫的快慢雙指標。
//     看懂它，就同時看懂了 remove_if 為什麼是 stable（保留元素相對順序不變）、
//     為什麼是 O(n)、以及為什麼尾段內容不可依賴。
//   ▸ 尾段是 valid but unspecified，不是 indeterminate
//     被 move 走的元素仍是合法物件（可解構、可賦值），
//     只是內容不保證。讀它不是 undefined behavior，
//     但也不該對讀到什麼有任何期待——對 int 可能看到舊值，
//     對 std::string 可能看到空字串。
//   ▸ 為什麼 std::list 有自己的 remove_if 成員函式
//     list 可以只改指標就摘掉節點，成員版是 O(n) 且完全不搬移元素值。
//     用通用演算法反而要做 move-assign，白白付出搬資料的成本。
//     這也是「容器有同名成員函式時優先用成員版」這條準則的由來。
//
// 【注意事項 Pay Attention】
//   1. remove_if 不改變 size；忘了接 erase 就等於什麼都沒刪。
//   2. [new_end, end()) 是 valid but unspecified，內容不可依賴。
//   3. erase-remove 不歸還記憶體，capacity 不變；需要時另呼叫 shrink_to_fit()。
//   4. 要在刪除時處理被刪元素，就不能用 erase-remove（元素已被覆蓋）。
//   5. list/forward_list 請用成員函式 remove_if，效率較好。
//   6. C++20 起優先用 std::erase_if(v, pred)，它還會回傳刪除個數。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】erase-remove 慣用法
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼刪除 vector 中一半的元素，逐一 erase 是 O(n²)
//        而 erase-remove 是 O(n)？
//     答：逐一 erase 時，每刪一個元素都要把它後面的所有元素往前搬一格，
//         刪 k 個就搬 k 次、每次最多 n 格 → O(n²)。
//         erase-remove 只掃一遍，用快慢雙指標把保留元素依序寫到前段，
//         每個元素最多搬一次 → O(n)；最後對尾段做一次區間 erase，
//         因為砍的是尾巴，不需要任何補位搬移。
//     追問：那 erase-remove 之後記憶體有變少嗎？
//         → 沒有。erase 只降低 size，capacity 完全不變。
//           要真正歸還記憶體必須再呼叫 shrink_to_fit()。
//
// 🔥 Q2. std::remove_if 之後，[new_end, end()) 這段裡面是什麼？
//     答：處於 valid but unspecified state（有效但未指定）的物件。
//         它們是合法可解構、可賦值的物件，讀取它們不是 UB，
//         但標準不保證內容。對 int 可能看到原本的尾端數值，
//         對 std::string 可能是被 move 空的字串——完全取決於實作與型別。
//     追問：那可以直接對那段元素賦新值來重複使用嗎？
//         → 可以。valid 的意思就是「所有不需要前置條件的操作都合法」，
//           賦值正是其中之一。不可以做的是「讀取後假設內容」。
//
// ⚠️ 陷阱. 寫了 std::remove_if(v.begin(), v.end(), pred); 之後
//          印出 v，發現元素一個都沒少——remove_if 壞掉了嗎？
//     答：沒壞。remove_if 從來不刪除元素，它只把保留的元素往前搬
//         並回傳新的邏輯結尾。真正縮短容器必須接
//         v.erase(new_end, v.end())。少了這一行，size 當然不變。
//     為什麼會錯：被函式名稱誤導。它叫 remove，實際做的卻是 partition。
//         根本原因是演算法只拿得到迭代器、拿不到容器，
//         物理上就不可能改變容器大小。
//         叫它 move_unwanted_to_the_back 會誠實得多。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 27. Remove Element
//   題目：原地移除陣列中所有等於 val 的元素，回傳新長度。
//   為什麼用到本主題：這題就是 erase-remove 的最小化版本。
//         第 15 課用「逐一 erase」解過同一題（O(n²)）；
//         這裡改用 erase-remove，正好對照出兩者的複雜度差異——
//         而 LeetCode 官方期待的快慢雙指標解法，
//         其實就是 std::remove 的內部實作。
// -----------------------------------------------------------------------------
int removeElement(std::vector<int>& nums, int val) {
    auto newEnd = std::remove(nums.begin(), nums.end(), val);
    nums.erase(newEnd, nums.end());
    return static_cast<int>(nums.size());
}

// 手寫版：把 std::remove 的內部攤開，就是 LeetCode 的標準解
int removeElementManual(std::vector<int>& nums, int val) {
    std::size_t write = 0;
    for (std::size_t read = 0; read < nums.size(); ++read) {
        if (nums[read] != val) {
            nums[write++] = nums[read];      // 保留的才寫回去
        }
    }
    nums.resize(write);
    return static_cast<int>(write);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】伺服器 access log 過濾：一次剔除健康檢查與靜態資源請求
//   情境：分析流量前，要先把 /healthz 探針、/favicon.ico、
//         以及靜態資源請求濾掉，否則 QPS 與熱門路徑統計會嚴重失真。
//   為什麼用本主題：這是最典型的「大量資料 + 條件批次刪除 +
//         被刪的東西不需要善後」場景，正是 erase-remove 的主場。
//         資料量動輒數十萬筆，逐一 erase 的 O(n²) 完全不可行。
// -----------------------------------------------------------------------------
struct AccessLog {
    std::string path;
    int         status;
};

std::size_t filterNoise(std::vector<AccessLog>& logs) {
    std::size_t before = logs.size();
    auto newEnd = std::remove_if(
        logs.begin(), logs.end(),
        [](const AccessLog& e) {
            return e.path == "/healthz" ||
                   e.path == "/favicon.ico" ||
                   e.path.rfind("/static/", 0) == 0;   // 以 /static/ 開頭
        });
    logs.erase(newEnd, logs.end());
    return before - logs.size();
}

int main() {
    std::cout << "=== 一、erase-remove 兩步驟拆解 ===" << std::endl;
    std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8};
    std::cout << "原始     : ";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;

    // 第一步：remove_if 只重排，回傳新的邏輯結尾
    auto new_end = std::remove_if(v.begin(), v.end(),
                                  [](int x) { return x % 2 == 0; });
    std::cout << "remove_if 後 size = " << v.size()
              << "（完全沒變），邏輯長度 = " << (new_end - v.begin()) << std::endl;
    std::cout << "保留區   : ";
    for (auto it = v.begin(); it != new_end; ++it) std::cout << *it << " ";
    std::cout << "  ← 尾段為 valid but unspecified，故不印出" << std::endl;

    // 第二步：erase 才真正縮短容器
    v.erase(new_end, v.end());
    std::cout << "erase 後 : ";
    for (int x : v) std::cout << x << " ";
    std::cout << "  size = " << v.size() << std::endl;

    std::cout << "\n=== 二、erase-remove 不會歸還記憶體 ===" << std::endl;
    std::vector<int> big(1000);
    for (int i = 0; i < 1000; ++i) big[static_cast<std::size_t>(i)] = i;
    std::cout << "初始       size=" << big.size() << ", capacity=" << big.capacity() << std::endl;
    auto e = std::remove_if(big.begin(), big.end(), [](int x) { return x % 10 != 0; });
    big.erase(e, big.end());
    std::cout << "erase-remove 後 size=" << big.size()
              << ", capacity=" << big.capacity() << "  ← capacity 沒動" << std::endl;
    big.shrink_to_fit();
    std::cout << "shrink 後  size=" << big.size() << ", capacity=" << big.capacity() << std::endl;

    std::cout << "\n=== 三、LeetCode 27. Remove Element ===" << std::endl;
    std::vector<int> nums1 = {3, 2, 2, 3};
    std::cout << "erase-remove 版 {3,2,2,3} val=3 → 長度 "
              << removeElement(nums1, 3) << ", 內容: ";
    for (int x : nums1) std::cout << x << " ";
    std::cout << std::endl;

    std::vector<int> nums2 = {0, 1, 2, 2, 3, 0, 4, 2};
    std::cout << "手寫雙指標版 {0,1,2,2,3,0,4,2} val=2 → 長度 "
              << removeElementManual(nums2, 2) << ", 內容: ";
    for (int x : nums2) std::cout << x << " ";
    std::cout << std::endl;

    std::cout << "\n=== 四、日常實務：過濾 access log 雜訊 ===" << std::endl;
    std::vector<AccessLog> logs = {
        {"/api/orders", 200},   {"/healthz", 200},        {"/api/users", 200},
        {"/static/app.js", 200},{"/favicon.ico", 404},    {"/api/orders", 500},
        {"/healthz", 200},      {"/static/logo.png", 200},{"/api/login", 401}
    };
    std::cout << "過濾前筆數: " << logs.size() << std::endl;
    std::size_t removed = filterNoise(logs);
    std::cout << "剔除雜訊  : " << removed << " 筆" << std::endl;
    std::cout << "保留的請求: ";
    for (const AccessLog& e2 : logs) std::cout << e2.path << "(" << e2.status << ") ";
    std::cout << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 17 課：vector 的記憶體重新配置機制8.cpp" -o erase_remove
//
// 【關於下方預期輸出的但書】
//   第二段的 capacity 數值屬「實作定義」（本機 GCC 15.2 / libstdc++）：
//   vector<int> big(1000) 的初始 capacity 恰為 1000，
//   shrink_to_fit() 後縮為 100。其他實作的數值可能不同，
//   而且 shrink_to_fit 在標準上只是非強制性請求，允許完全不動作。
//   本檔刻意不印出 remove_if 之後的尾段內容——
//   那是 valid but unspecified，印出來會誤導成「可以依賴的結果」。

// === 預期輸出 ===
// === 一、erase-remove 兩步驟拆解 ===
// 原始     : 1 2 3 4 5 6 7 8
// remove_if 後 size = 8（完全沒變），邏輯長度 = 4
// 保留區   : 1 3 5 7   ← 尾段為 valid but unspecified，故不印出
// erase 後 : 1 3 5 7   size = 4
//
// === 二、erase-remove 不會歸還記憶體 ===
// 初始       size=1000, capacity=1000
// erase-remove 後 size=100, capacity=1000  ← capacity 沒動
// shrink 後  size=100, capacity=100
//
// === 三、LeetCode 27. Remove Element ===
// erase-remove 版 {3,2,2,3} val=3 → 長度 2, 內容: 2 2
// 手寫雙指標版 {0,1,2,2,3,0,4,2} val=2 → 長度 5, 內容: 0 1 3 0 4
//
// === 四、日常實務：過濾 access log 雜訊 ===
// 過濾前筆數: 9
// 剔除雜訊  : 5 筆
// 保留的請求: /api/orders(200) /api/users(200) /api/orders(500) /api/login(401)
