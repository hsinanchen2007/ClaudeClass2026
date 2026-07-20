// =============================================================================
//  第 16 課：vector 的迭代器操作 2  —  begin() / end() 與半開區間 [begin, end)
// =============================================================================
//
// 【主題資訊 Information】
//   iterator       begin() noexcept;        // 指向第一個元素
//   iterator       end()   noexcept;        // 指向「最後一個元素之後」的哨兵
//   const_iterator begin() const noexcept;  // 對 const vector 自動回傳唯讀版
//   標頭檔：<vector>
//   標準版本：C++98（noexcept 標註為 C++11 加上）
//   複雜度：兩者皆 O(1)
//   回傳：一對迭代器，共同界定半開區間 [begin(), end())
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼是半開區間，而不是 [first, last] 兩端都含】
//   這是 STL 全域一致的約定，好處有四個，每一個都很實際：
//     (a) 空區間可以表示。閉區間 [first, last] 無法表達「什麼都沒有」——
//         你找不到一組合法的 first/last 讓它代表空集合。半開區間只要
//         first == last 就是空，因此空 vector 天然滿足 begin() == end()，
//         所有 for 迴圈一次都不跑，不需要任何特例判斷。
//     (b) 元素個數 = last - first，沒有惱人的 +1。閉區間得寫 last - first + 1，
//         這正是無數 off-by-one bug 的來源。
//     (c) 區間可以無縫切割。[a, b) 與 [b, c) 接起來剛好是 [a, c)，中間那個 b
//         不會被算兩次。分頁、分塊、平行切分都靠這個性質。
//     (d) 迴圈終止條件統一寫 it != end，不必區分「走到最後一個」與「走過頭」。
//
// 【2. end() 為什麼不可解參考】
//   end() 指向的是「最後一個元素的下一個位址」。標準保證這個位址可以被
//   「計算與比較」，但不保證那裡有物件 —— 它可能剛好是配置區塊的邊界之外。
//   因此 *v.end() 是未定義行為（UB）。UB 的意思是「標準不規範會發生什麼」，
//   可能讀到垃圾值、可能剛好讀到別的資料、也可能觸發保護錯誤，
//   不能假設它一定會 crash，更不能假設它一定印出某個值。
//   註：位址本身可以合法算出來，所以 v.end() 本身、v.end() - 1 都是合法的，
//       只有「解參考 end()」不合法。
//
// 【3. begin() 的 const 多載】
//   begin() 有兩個多載：非 const 版回傳 iterator，const 版回傳 const_iterator。
//   所以對 const vector& 呼叫 begin() 會自動得到唯讀迭代器，這是型別系統
//   幫你擋下誤寫的第一道防線（第 4 個檔案會談 cbegin() 為何仍有必要）。
//
// 【概念補充 Concept Deep Dive】
//   vector 內部通常就是三個指標（libstdc++ 的 _Vector_impl）：
//       _M_start           → begin()
//       _M_finish          → end()
//       _M_end_of_storage  → begin() + capacity()
//   所以 begin()/end() 只是回傳成員指標的包裝，size() 其實是
//   _M_finish - _M_start 算出來的，capacity() 是 _M_end_of_storage - _M_start。
//   這解釋了兩件事：
//     * 為什麼 size() 是 O(1) —— 它是減法，不是走訪計數。
//     * 為什麼 push_back 觸發重新配置後，三個指標全部改指新緩衝區，
//       導致「所有」既有迭代器失效（見第 14 個檔案）。
//
// 【注意事項 Pay Attention】
//   1. *v.end() 是 UB；v.end() - 1 才是最後一個元素（但空 vector 不適用）。
//   2. 對空 vector，begin() == end()，*begin() 一樣是 UB。
//   3. 比較「來自不同容器」的兩個迭代器（it1 != it2、it1 < it2）是 UB，
//      即使跑起來好像沒事。迴圈條件務必拿同一個容器的 end() 來比。
//   4. 迴圈條件用 != 而不是 <：對 vector 兩者都能編譯，但 != 是對所有迭代器
//      類別都成立的通用寫法（list 的迭代器就不支援 <）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】半開區間與 end()
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 STL 統一採用半開區間 [begin, end)？講三個理由。
//     答：(1) 能表示空區間（first == last），空容器不需要特例；
//         (2) 元素個數就是 last - first，不必 +1，減少 off-by-one；
//         (3) 區間可無縫相接：[a,b) + [b,c) = [a,c)，分割不重不漏。
//     追問：那 end() 指向的記憶體到底有沒有東西？
//         → 標準只保證該位址可以被計算與比較，不保證有物件，所以不可解參考。
//
// 🔥 Q2. 為什麼迴圈條件寫 it != v.end() 而不是 it < v.end()？
//     答：對 vector 兩者都可編譯且結果相同，因為它是隨機存取迭代器。
//         但 != 對所有迭代器類別都成立，< 只有隨機存取迭代器才有；
//         寫 != 的程式碼日後把容器換成 list 仍能編譯。
//     追問：那什麼時候反而該用 < ？
//         → 當迴圈內可能讓迭代器一次前進多步（it += 2）時，用 != 可能整個
//           跳過終點造成越界，這種情況要改用 <。
//
// ⚠️ 陷阱. 下面這段程式碼錯在哪裡？
//         for (auto it = v.begin(); it != v.end(); ++it)
//             if (*it < 0) v.push_back(0);      // ← 邊走邊 push_back
//     答：push_back 可能觸發重新配置，使 it 與 v.end() 全部失效，
//         之後的 ++it 與比較都是 UB。而且新增的元素會讓迴圈可能永不終止。
//     為什麼會錯：多數人以為「只要不刪除元素就安全」，但 vector 的危險來自
//         「重新配置」而非刪除；新增同樣會搬家。正確做法是先收集要新增的
//         內容到另一個 vector，迴圈結束後再合併。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>

void demo_begin_end() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // begin() 指向第一個元素
    // end()   指向最後一個元素的「下一個位置」（past-the-end）
    std::vector<int>::iterator first = v.begin();
    std::vector<int>::iterator last  = v.end();

    std::cout << "*first = " << *first << std::endl;
    // *last 是未定義行為！end() 不指向任何有效元素。
    // 但 last 本身可以合法參與運算 —— 例如相減得到元素個數：
    std::cout << "last - first = " << (last - first)
              << "（= size()，半開區間不必 +1）" << std::endl;
    std::cout << "*(last - 1) = " << *(last - 1) << "（最後一個元素）" << std::endl;

    // 典型的遍歷方式
    for (std::vector<int>::iterator it = v.begin(); it != v.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // 空容器：半開區間讓它天然安全，不需要任何特例判斷
    std::vector<int> empty_v;
    std::cout << std::boolalpha
              << "空 vector 的 begin() == end()：" << (empty_v.begin() == empty_v.end())
              << "，迴圈執行 0 次" << std::endl;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 26. Remove Duplicates from Sorted Array
//   題目：就地移除已排序陣列中的重複元素，回傳新長度 k；前 k 個位置必須是
//         去重後的結果。
//   為什麼用到本主題：這題的標準解法就是維護一個「半開區間 [begin, write)」
//         代表已確定的結果區，write 正是這個區間的 past-the-end 位置。
//         每接受一個新元素就 ++write，區間自然長大 —— 完全是 end() 的語意。
//   複雜度：時間 O(n)、空間 O(1)。
// -----------------------------------------------------------------------------
int removeDuplicates(std::vector<int>& nums) {
    if (nums.empty()) return 0;

    auto write = nums.begin();          // [nums.begin(), write] 已確定，write 指向下一個待寫位置
    for (auto read = nums.begin() + 1; read != nums.end(); ++read) {
        if (*read != *write) {          // 與結果區最後一個不同 → 是新值
            ++write;
            *write = *read;
        }
    }
    // write 目前指向結果區的最後一個元素，長度要 +1
    return static_cast<int>(write - nums.begin()) + 1;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】log 檔的分頁輸出（用半開區間切分批次）
//   情境：伺服器把數萬行 log 傳到前端時要分頁，每頁 N 行。
//   為什麼用到本主題：分頁就是把 [begin, end) 切成一串互不重疊的子區間
//         [p0,p1)、[p1,p2)、…，半開區間讓「接縫處那一行」不會被印兩次；
//         最後一頁不足 N 行時，只要讓 end 取 min 就自然收尾，不必特判。
// -----------------------------------------------------------------------------
void printLogPage(const std::vector<std::string>& logs, std::size_t page, std::size_t per_page) {
    const std::size_t total = logs.size();
    std::size_t from = page * per_page;
    if (from >= total) {                                  // 超出範圍 → 空頁
        std::cout << "  (第 " << page << " 頁：無資料)" << std::endl;
        return;
    }
    std::size_t to = from + per_page;
    if (to > total) to = total;                           // 最後一頁自動截短

    auto b = logs.begin() + static_cast<std::ptrdiff_t>(from);
    auto e = logs.begin() + static_cast<std::ptrdiff_t>(to);

    std::cout << "  第 " << page << " 頁（" << (e - b) << " 行）：" << std::endl;
    for (auto it = b; it != e; ++it) {
        std::cout << "    " << *it << std::endl;
    }
}

int main() {
    std::cout << "=== begin() / end() 與半開區間 ===" << std::endl;
    demo_begin_end();

    std::cout << "\n=== LeetCode 26. Remove Duplicates from Sorted Array ===" << std::endl;
    std::vector<int> nums = {0, 0, 1, 1, 1, 2, 2, 3, 3, 4};
    int k = removeDuplicates(nums);
    std::cout << "新長度 k = " << k << "，前 k 個元素：";
    for (int i = 0; i < k; ++i) std::cout << nums[i] << " ";
    std::cout << std::endl;

    std::cout << "\n=== 日常實務：log 分頁（每頁 3 行）===" << std::endl;
    std::vector<std::string> logs = {
        "10:00:01 [INFO]  service started",
        "10:00:04 [WARN]  cache miss rate 62%",
        "10:00:09 [ERROR] db connect timeout",
        "10:00:11 [INFO]  retry succeeded",
        "10:00:15 [ERROR] disk usage 91%",
        "10:00:20 [INFO]  heartbeat ok",
        "10:00:26 [WARN]  slow query 1200ms"
    };
    printLogPage(logs, 0, 3);
    printLogPage(logs, 2, 3);   // 最後一頁只有 1 行
    printLogPage(logs, 5, 3);   // 超出範圍

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 16 課：vector 的迭代器操作2.cpp" -o iter2

// === 預期輸出 ===
// === begin() / end() 與半開區間 ===
// *first = 10
// last - first = 5（= size()，半開區間不必 +1）
// *(last - 1) = 50（最後一個元素）
// 10 20 30 40 50 
// 空 vector 的 begin() == end()：true，迴圈執行 0 次
//
// === LeetCode 26. Remove Duplicates from Sorted Array ===
// 新長度 k = 5，前 k 個元素：0 1 2 3 4 
//
// === 日常實務：log 分頁（每頁 3 行）===
//   第 0 頁（3 行）：
//     10:00:01 [INFO]  service started
//     10:00:04 [WARN]  cache miss rate 62%
//     10:00:09 [ERROR] db connect timeout
//   第 2 頁（1 行）：
//     10:00:26 [WARN]  slow query 1200ms
//   (第 5 頁：無資料)
