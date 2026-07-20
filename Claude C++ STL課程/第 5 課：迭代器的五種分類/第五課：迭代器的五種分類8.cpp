// =============================================================================
//  第五課：迭代器的五種分類 8  —  演算法對迭代器的「最低要求」決定誰能用
// =============================================================================
//
// 【主題資訊 Information】
//   本檔主題：<algorithm> 裡每個演算法都對迭代器有明確的**最低能力要求**，
//             這個要求決定了它能不能用在某個容器上。
//   三個代表：
//       std::find     最低要求 Input Iterator         → 所有容器都能用
//       std::reverse  最低要求 Bidirectional Iterator → forward_list 不能用
//       std::sort     最低要求 Random Access Iterator → 只有 vector/deque/array 能用
//   標頭檔：<algorithm>
//   標準版本：三者皆 C++98；C++17 加了平行版（execution policy），
//             C++20 另有 std::ranges:: 版本並改用 concept 表達同樣的要求。
//
// 【詳細解釋 Explanation】
//
// 【1. 「能不能用」不是看標頭檔，是看迭代器能力】
//   初學者常有的模型是：「<algorithm> 是一組通用工具，對所有容器一視同仁」。
//   這是錯的。每個演算法的模板參數名稱其實就寫著它的要求：
//       template<class InputIt,  class T>  InputIt find(InputIt, InputIt, const T&);
//       template<class BidirIt>            void   reverse(BidirIt, BidirIt);
//       template<class RandomIt>           void   sort(RandomIt, RandomIt);
//   參數名 InputIt / BidirIt / RandomIt 就是文件。
//   C++20 的 ranges 版本更直接，把要求寫成 concept，違反時錯誤訊息也清楚得多。
//
// 【2. 為什麼 sort 需要 Random Access】
//   std::sort 通常是 introsort（quicksort + heapsort + insertion sort 的混合）。
//   quicksort 的 partition 需要「從兩端往中間夾」並隨時交換任意兩個位置；
//   heapsort 需要用 2i+1 / 2i+2 算出子節點位置；
//   兩者都要求 O(1) 跳到任意位置。鏈結串列做不到，
//   硬要支援的話每次跳躍變 O(n)，整體從 O(n log n) 退化成 O(n² log n)，
//   比不提供還糟。
//
//   所以 list / forward_list 各自提供 **成員函式** sort()：
//       lst.sort();      flst.sort();
//   它們走的是為鏈結串列量身訂做的合併排序——只改指標、不搬資料、
//   不需要隨機存取、也不需要額外緩衝區。
//   注意這裡的取捨方向：**不是「效率不如 std::sort」那麼簡單**。
//   對鏈結串列而言 list::sort 反而是最合適的演算法（O(n log n) 且不搬元素）；
//   真正的差距在於 vector 的連續記憶體對 CPU 快取友善，
//   同樣是 O(n log n)，vector + std::sort 的常數小得多。
//
// 【3. 為什麼 reverse 需要 Bidirectional】
//   std::reverse 的標準做法是「頭尾兩個迭代器往中間夾，邊走邊 swap」：
//       while (first != last && first != --last) std::iter_swap(first++, last);
//   那個 --last 就是 Bidirectional 的要求來源。
//   forward_list 不能後退，所以 std::reverse 用不了；
//   它同樣提供了成員函式 flst.reverse()，
//   實作方式完全不同——直接把每個節點的 next 指標反向重接，
//   一趟 O(n)，不需要後退。
//
// 【4. 這個設計哲學：能力分級讓錯誤在編譯期就被擋下】
//   把 std::sort(lst.begin(), lst.end()) 寫出來，**編譯就會失敗**。
//   這是刻意的：如果標準讓它「可以編譯但很慢」，
//   效能問題就會潛伏到上線後才爆炸。
//   迭代器分類的真正價值不只是分類本身，
//   而是讓「這個資料結構不適合這個演算法」變成一個編譯期錯誤。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 各演算法最低要求速查（常見的）
//     Input          : find、count、accumulate、equal、for_each、copy（來源端）
//     Output         : copy（目的端）、fill_n、generate_n
//     Forward        : unique、remove、replace、search、lower_bound（見下）
//     Bidirectional  : reverse、next_permutation、copy_backward、stable_partition
//     Random Access  : sort、stable_sort、nth_element、shuffle、make_heap、
//                      partial_sort、binary_search 的「真 O(log n)」
//
// (B) lower_bound 是個容易誤判的例子
//     它的最低要求只是 Forward，所以在 list 上**可以編譯也可以跑**。
//     但複雜度規定是「比較次數 O(log n)、迭代器前進次數 O(n)」——
//     在 list 上總成本是 O(n)，比線性掃還慢。
//     這說明「能編譯」不等於「該用」，第 7 檔有實測。
//
// (C) std::remove / std::unique 不會真的刪除元素
//     它們只要求 Forward Iterator，所以不能改變容器大小
//     （改大小是容器的能力，不是迭代器的）。
//     它們的做法是把要保留的元素往前搬，回傳「新的邏輯結尾」，
//     後面的元素變成未指定的值。真正刪除要靠容器自己：
//         v.erase(std::remove(v.begin(), v.end(), x), v.end());   // erase-remove 慣用法
//     C++20 起有更直白的 std::erase / std::erase_if 可以一步到位。
//
// (D) C++20 ranges 把要求寫成 concept
//     std::ranges::sort(lst) 會直接告訴你
//     「list 不滿足 random_access_range」，
//     而不是丟出一大串模板實例化的錯誤。這是 concept 最實際的好處之一。
//
// 【注意事項 Pay Attention】
//   1. std::sort 不是穩定排序（相同鍵的相對順序可能改變）。
//      要穩定請用 std::stable_sort；list::sort **是**穩定的。
//   2. lst.sort() 與 std::sort 名字像但完全是兩回事，別混用。
//   3. std::remove / std::unique 不改變容器大小，一定要搭配 erase。
//   4. std::unique 只移除**相鄰**的重複元素，用之前通常要先排序。
//   5. forward_list 沒有 rbegin()、不能用 std::reverse，
//      但有成員函式 reverse()（直接反接指標）。
//   6. 對已排序資料，std::sort 沒有特別優化；
//      若資料多半有序可考慮 std::stable_sort 或先檢查 is_sorted。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】演算法的迭代器要求
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 std::sort 不能用在 std::list？list 不是也能排序嗎？
//     答：std::sort 要求 Random Access Iterator，list 只有 Bidirectional，
//         所以**編譯就會失敗**。原因是 introsort 的 partition 與 heapify
//         都需要 O(1) 跳到任意位置，鏈結串列做不到。
//         list 提供成員函式 lst.sort()，走的是為鏈結串列設計的合併排序，
//         只改指標不搬資料，複雜度一樣是 O(n log n)，而且是穩定排序。
//     追問：那 list::sort 是不是比 std::sort 慢？
//         → 同樣是 O(n log n)，但 vector 的連續記憶體對 CPU 快取友善得多，
//           實務上 vector + std::sort 的常數小很多。
//           差距來自記憶體佈局，不是演算法本身。
//
// 🔥 Q2. std::remove 為什麼不會真的把元素刪掉？
//     答：因為它只拿到迭代器，而迭代器沒有「改變容器大小」的能力——
//         那是容器才有的權力。remove 的做法是把要保留的元素往前搬，
//         回傳新的邏輯結尾，尾端變成未指定的值。
//         真正刪除要用 erase-remove 慣用法：
//             v.erase(std::remove(v.begin(), v.end(), x), v.end());
//     追問：C++20 有更好的寫法嗎？
//         → 有，std::erase(v, x) 和 std::erase_if(v, pred) 一步到位。
//
// ⚠️ 陷阱. std::unique 可以幫我把 vector 裡所有重複元素去掉，對吧？
//     答：不對，有兩個陷阱疊在一起。
//         (1) unique 只移除**相鄰**的重複元素。{1,2,1} 跑完還是 {1,2,1}，
//             因為兩個 1 不相鄰。要全域去重必須**先排序**。
//         (2) 它不改變容器大小，只回傳新的邏輯結尾，
//             後面的元素是未指定的值。
//         正確寫法是：
//             std::sort(v.begin(), v.end());
//             v.erase(std::unique(v.begin(), v.end()), v.end());
//     為什麼會錯：把 unique 想成「去重函式」。它的真實語意是
//         「壓縮連續重複段」（等同 Unix 的 uniq 指令，也要先 sort 才準）。
//         名字造成的誤解，加上「演算法不能改容器大小」這個
//         迭代器抽象的根本限制，兩者疊在一起就成了經典錯誤。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <list>
#include <forward_list>
#include <string>
#include <algorithm>
#include <iterator>
#include <cctype>      // std::isspace / std::tolower（引數必須先轉 unsigned char）

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 26. Remove Duplicates from Sorted Array
//   題目：給一個已排序的陣列，原地移除重複元素，回傳新的長度；
//         前 k 個元素必須是去重後的結果。
//   為什麼用到本主題：這題就是 std::unique 的語意本身。
//     它完美示範了本檔的核心觀念——
//       * unique 只要求 Forward Iterator，所以不需要隨機存取
//       * 它**不能**改變容器大小（迭代器沒這個權力），
//         只能回傳「新的邏輯結尾」，剛好就是題目要的 k
//     所以這題用 STL 解幾乎是一行，而題目要求「回傳新長度」
//     正是因為 C 風格陣列同樣無法改變大小，兩者的限制是同一個。
// -----------------------------------------------------------------------------
int removeDuplicates(std::vector<int>& nums) {
    // 輸入已排序，所以重複元素必定相鄰，unique 直接適用
    auto newEnd = std::unique(nums.begin(), nums.end());
    return static_cast<int>(newEnd - nums.begin());   // 新的邏輯長度 k
}

// 手寫版：說明 unique 內部在做什麼（雙指標往前搬）
int removeDuplicatesManual(std::vector<int>& nums) {
    if (nums.empty()) return 0;
    std::size_t write = 1;
    for (std::size_t read = 1; read < nums.size(); ++read) {
        if (nums[read] != nums[write - 1]) nums[write++] = nums[read];
    }
    return static_cast<int>(write);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】郵件群發前，把收件者清單去重並排序
//   情境：系統從三個來源（訂閱表、活動報名、手動輸入）收集 email，
//         合併後一定有重複。群發前必須去重，否則同一個人收到兩封，
//         而且會拉高退信率、影響寄件信譽。
//   為什麼用到本主題：這正是「sort + unique + erase」三部曲的教科書場景，
//     每一步都對應本檔的一個迭代器要求：
//       sort   → Random Access（所以收件者清單要放 vector，不能放 list）
//       unique → Forward（只壓縮相鄰重複，所以一定要先 sort）
//       erase  → 容器的能力，演算法給不了
//   附帶示範：email 比對通常要不分大小寫，所以正規化要在去重之前做。
// -----------------------------------------------------------------------------
std::string normalizeEmail(std::string e) {
    // 去頭尾空白
    const auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    e.erase(e.begin(), std::find_if(e.begin(), e.end(), notSpace));
    e.erase(std::find_if(e.rbegin(), e.rend(), notSpace).base(), e.end());
    // 轉小寫（注意：<cctype> 的函式必須先轉成 unsigned char，否則是 UB）
    std::transform(e.begin(), e.end(), e.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return e;
}

std::vector<std::string> dedupRecipients(std::vector<std::string> raw) {
    for (auto& e : raw) e = normalizeEmail(e);

    // 去掉空字串（erase-remove 慣用法）
    raw.erase(std::remove(raw.begin(), raw.end(), std::string{}), raw.end());

    std::sort(raw.begin(), raw.end());                       // 需要 Random Access
    raw.erase(std::unique(raw.begin(), raw.end()), raw.end()); // unique 只壓相鄰
    return raw;
}

int main() {
    std::vector<int> vec = {5, 2, 8, 1, 9};
    std::list<int> lst = {5, 2, 8, 1, 9};
    std::forward_list<int> flst = {5, 2, 8, 1, 9};

    // find：只需要 Input Iterator
    // 所有容器都能用
    std::cout << "=== find（需要 Input Iterator）===" << std::endl;

    auto v_it = std::find(vec.begin(), vec.end(), 8);
    auto l_it = std::find(lst.begin(), lst.end(), 8);
    auto f_it = std::find(flst.begin(), flst.end(), 8);

    std::cout << "vector: " << (v_it != vec.end() ? "找到" : "沒找到") << std::endl;
    std::cout << "list: " << (l_it != lst.end() ? "找到" : "沒找到") << std::endl;
    std::cout << "forward_list: " << (f_it != flst.end() ? "找到" : "沒找到") << std::endl;

    // reverse：需要 Bidirectional Iterator
    // forward_list 不能用！
    std::cout << "\n=== reverse（需要 Bidirectional Iterator）===" << std::endl;

    std::reverse(vec.begin(), vec.end());
    std::reverse(lst.begin(), lst.end());
    // std::reverse(flst.begin(), flst.end());  // 編譯錯誤！內部需要 --last

    std::cout << "vector 反轉: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "list 反轉: ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;

    // forward_list 有自己的成員函式：直接把 next 指標反接，一趟 O(n)
    flst.reverse();
    std::cout << "forward_list 反轉（成員函式 flst.reverse()）: ";
    for (int n : flst) std::cout << n << " ";
    std::cout << std::endl;

    // sort：需要 Random Access Iterator
    // 只有 vector 能用 std::sort！
    std::cout << "\n=== sort（需要 Random Access Iterator）===" << std::endl;

    std::sort(vec.begin(), vec.end());
    // std::sort(lst.begin(), lst.end());   // 編譯錯誤！partition 需要 O(1) 跳躍
    // std::sort(flst.begin(), flst.end()); // 編譯錯誤！

    // list 和 forward_list 有自己的 sort 成員函數
    // （為鏈結串列設計的合併排序：只改指標、不搬資料、而且是穩定排序）
    lst.sort();
    flst.sort();

    std::cout << "vector 排序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "list 排序: ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "forward_list 排序: ";
    for (int n : flst) std::cout << n << " ";
    std::cout << std::endl;

    // -------------------------------------------------------------------------
    std::cout << "\n=== 為什麼「能編譯」不等於「該用」 ===" << std::endl;
    {
        std::cout << "std::find        最低要求 Input         → 三種容器都能用" << std::endl;
        std::cout << "std::reverse     最低要求 Bidirectional → forward_list 編譯失敗" << std::endl;
        std::cout << "std::sort        最低要求 Random Access → list 也編譯失敗" << std::endl;
        std::cout << "std::lower_bound 最低要求 Forward       → list 能編譯，"
                  << "但會退化成 O(n)" << std::endl;
        std::cout << "→ 前三個由編譯器擋下；最後一個編譯器擋不了，要靠人判斷。" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== std::remove / std::unique 不會改變容器大小 ===" << std::endl;
    {
        std::vector<int> v = {1, 2, 2, 3, 2, 4};
        std::cout << "原始 v = ";
        for (int n : v) std::cout << n << " ";
        std::cout << " (size=" << v.size() << ")" << std::endl;

        auto newEnd = std::remove(v.begin(), v.end(), 2);   // 只搬，不刪
        std::cout << "std::remove(v, 2) 之後 size 仍是 " << v.size()
                  << "，邏輯長度是 " << (newEnd - v.begin()) << std::endl;
        std::cout << "  [begin, newEnd) = ";
        for (auto it = v.begin(); it != newEnd; ++it) std::cout << *it << " ";
        std::cout << std::endl;
        std::cout << "  [newEnd, end) 的值是未指定的，不可依賴" << std::endl;

        v.erase(newEnd, v.end());                            // 這一步才真的刪
        std::cout << "erase 之後 v = ";
        for (int n : v) std::cout << n << " ";
        std::cout << " (size=" << v.size() << ")" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 陷阱：unique 只壓縮「相鄰」重複 ===" << std::endl;
    {
        std::vector<int> a = {1, 2, 1, 2, 1};      // 沒排序
        std::vector<int> b = a;                    // 排序後再 unique

        a.erase(std::unique(a.begin(), a.end()), a.end());

        std::sort(b.begin(), b.end());
        b.erase(std::unique(b.begin(), b.end()), b.end());

        std::cout << "沒先排序就 unique: ";
        for (int n : a) std::cout << n << " ";
        std::cout << " ← 還有重複！" << std::endl;

        std::cout << "先 sort 再 unique  : ";
        for (int n : b) std::cout << n << " ";
        std::cout << " ← 正確去重" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== LeetCode 26. Remove Duplicates from Sorted Array ===" << std::endl;
    {
        std::vector<int> n1 = {1, 1, 2};
        int k1 = removeDuplicates(n1);
        std::cout << "[1,1,2] -> k=" << k1 << ", 前 k 個 = ";
        for (int i = 0; i < k1; ++i) std::cout << n1[i] << " ";
        std::cout << std::endl;

        std::vector<int> n2 = {0,0,1,1,1,2,2,3,3,4};
        int k2 = removeDuplicates(n2);
        std::cout << "[0,0,1,1,1,2,2,3,3,4] -> k=" << k2 << ", 前 k 個 = ";
        for (int i = 0; i < k2; ++i) std::cout << n2[i] << " ";
        std::cout << std::endl;

        std::vector<int> n3 = {0,0,1,1,1,2,2,3,3,4};
        int k3 = removeDuplicatesManual(n3);
        std::cout << "手寫雙指標版 k=" << k3 << ", 前 k 個 = ";
        for (int i = 0; i < k3; ++i) std::cout << n3[i] << " ";
        std::cout << "  ← 與 std::unique 結果一致" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務：群發前的收件者去重 ===" << std::endl;
    {
        std::vector<std::string> raw = {
            "Alice@Example.com",
            "  bob@example.com  ",
            "alice@example.com",
            "carol@example.com",
            "",
            "BOB@EXAMPLE.COM",
            "dave@example.com",
        };

        std::cout << "原始清單 " << raw.size() << " 筆：" << std::endl;
        for (const auto& e : raw) std::cout << "  [" << e << "]" << std::endl;

        auto clean = dedupRecipients(raw);

        std::cout << "去重後 " << clean.size() << " 筆：" << std::endl;
        for (const auto& e : clean) std::cout << "  " << e << std::endl;
        std::cout << "→ sort（Random Access）+ unique（Forward）+ erase（容器）" << std::endl;
        std::cout << "  三步各自對應不同層級的能力，缺一不可。" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第五課：迭代器的五種分類8.cpp -o demo8

// === 預期輸出 ===
// === find（需要 Input Iterator）===
// vector: 找到
// list: 找到
// forward_list: 找到
//
// === reverse（需要 Bidirectional Iterator）===
// vector 反轉: 9 1 8 2 5
// list 反轉: 9 1 8 2 5
// forward_list 反轉（成員函式 flst.reverse()）: 9 1 8 2 5
//
// === sort（需要 Random Access Iterator）===
// vector 排序: 1 2 5 8 9
// list 排序: 1 2 5 8 9
// forward_list 排序: 1 2 5 8 9
//
// === 為什麼「能編譯」不等於「該用」 ===
// std::find        最低要求 Input         → 三種容器都能用
// std::reverse     最低要求 Bidirectional → forward_list 編譯失敗
// std::sort        最低要求 Random Access → list 也編譯失敗
// std::lower_bound 最低要求 Forward       → list 能編譯，但會退化成 O(n)
// → 前三個由編譯器擋下；最後一個編譯器擋不了，要靠人判斷。
//
// === std::remove / std::unique 不會改變容器大小 ===
// 原始 v = 1 2 2 3 2 4  (size=6)
// std::remove(v, 2) 之後 size 仍是 6，邏輯長度是 3
//   [begin, newEnd) = 1 3 4
//   [newEnd, end) 的值是未指定的，不可依賴
// erase 之後 v = 1 3 4  (size=3)
//
// === 陷阱：unique 只壓縮「相鄰」重複 ===
// 沒先排序就 unique: 1 2 1 2 1  ← 還有重複！
// 先 sort 再 unique  : 1 2  ← 正確去重
//
// === LeetCode 26. Remove Duplicates from Sorted Array ===
// [1,1,2] -> k=2, 前 k 個 = 1 2
// [0,0,1,1,1,2,2,3,3,4] -> k=5, 前 k 個 = 0 1 2 3 4
// 手寫雙指標版 k=5, 前 k 個 = 0 1 2 3 4   ← 與 std::unique 結果一致
//
// === 日常實務：群發前的收件者去重 ===
// 原始清單 7 筆：
//   [Alice@Example.com]
//   [  bob@example.com  ]
//   [alice@example.com]
//   [carol@example.com]
//   []
//   [BOB@EXAMPLE.COM]
//   [dave@example.com]
// 去重後 4 筆：
//   alice@example.com
//   bob@example.com
//   carol@example.com
//   dave@example.com
// → sort（Random Access）+ unique（Forward）+ erase（容器）
//   三步各自對應不同層級的能力，缺一不可。
