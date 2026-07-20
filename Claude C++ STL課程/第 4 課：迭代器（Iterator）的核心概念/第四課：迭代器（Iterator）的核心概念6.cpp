// =============================================================================
//  第四課：迭代器（Iterator）的核心概念 6  —  reverse_iterator 與 base() 的偏移
// =============================================================================
//
// 【主題資訊 Information】
//   取得方式：
//     c.rbegin()  / c.rend()    reverse_iterator（可讀可寫，反向）
//     c.crbegin() / c.crend()   const_reverse_iterator（唯讀，反向）
//   轉換：
//     rit.base()   → 取得對應的正向迭代器（注意有一格偏移！）
//     std::reverse_iterator<It>(it)  → 由正向迭代器建構反向迭代器
//   標準版本：reverse_iterator / rbegin / rend 是 C++98；
//             **crbegin()/crend() 是 C++11**；
//             std::make_reverse_iterator 是 C++14。
//   需求：底層迭代器至少要是 Bidirectional（支援 --）。
//         所以 vector/list/map/set 都有 rbegin()，但 forward_list 沒有。
//   複雜度：所有操作與底層迭代器同級，O(1)。
//   標頭檔：容器自身；std::reverse_iterator 在 <iterator>。
//
// 【詳細解釋 Explanation】
//
// 【1. reverse_iterator 是一個轉接器（adaptor），不是新種類的迭代器】
//   它不是容器另外實作的一套走訪邏輯，而是一個**包裝**：內部存著一個正向
//   迭代器，把所有操作反過來轉發。核心實作只有幾行：
//       reverse_iterator& operator++() { --current; return *this; }   // ++ 變成 --
//       reference operator*() const { It tmp = current; return *--tmp; } // 先退一格再取值
//   注意 operator* 那行——**它取的是「current 前一格」的值**。
//   這個「取值前先退一格」正是整個 base() 偏移的來源。
//
// 【2. 為什麼要有這個偏移？——因為 end() 不可解參考】
//   反向走訪的起點應該是「最後一個元素」，終點應該是「第一個元素的前一格」。
//   但如果直接這樣存：
//       rbegin 存 end()-1（最後一個元素）    ← 可以
//       rend   存 begin()-1（第一個的前一格）← 這是 UB！（見第 1 檔）
//   「begin 前一格」在 C++ 的物件模型裡根本不是合法的指標值。
//   於是 STL 選擇整體平移一格：
//       rbegin() 內部存 end()      ，解參考時退一格 → 得到最後一個元素
//       rend()   內部存 begin()    ，解參考時退一格 → 但永遠不會被解參考
//   這樣所有內部存的值都落在合法的 [begin, end] 範圍內，完全避開 UB。
//   **偏移不是設計瑕疵，而是為了避開 begin-1 這個 UB 的必要代價。**
//
// 【3. base() 的精確關係（面試最愛考）】
//   對任何有效的 reverse_iterator rit：
//       &*rit == &*(rit.base() - 1)
//   也就是說 **rit.base() 指向的是 rit 所指元素的「右邊一格」**。
//   具體對照（vec = {10,20,30,40,50}）：
//       rbegin()          指向 50，  rbegin().base()  == end()      （50 的右邊）
//       rbegin()+1        指向 40，  (rbegin()+1).base() == end()-1 （40 的右邊）
//       rend()            不可解參考，rend().base()    == begin()
//   實務影響最大的地方是 erase：拿到一個 reverse_iterator 想刪掉它指的元素，
//   **不能**寫 `v.erase(rit.base())`（那會刪到右邊那個），
//   要寫 `v.erase(std::next(rit).base())` 或等價的 `v.erase((rit + 1).base())`。
//
// 【4. 什麼時候該用 rbegin()，什麼時候該用別的】
//   * 需要反向走訪整個容器 → rbegin()/rend()，或 C++20 的
//     `std::ranges::reverse_view`（`for (int n : vec | std::views::reverse)`）。
//   * 只是要最後一個元素 → 用 back()，不要繞 rbegin()。
//   * 需要「反向搜尋」→ 用 std::find(v.rbegin(), v.rend(), x)，
//     這比自己寫反向迴圈安全得多（不會產生 begin-1）。
//   * forward_list、unordered_map 沒有反向迭代器（單向串列/雜湊桶無法倒退）。
//
// 【概念補充 Concept Deep Dive】
//   reverse_iterator 的 operator* 每次都要複製一份底層迭代器再 -- 再解參考：
//       reference operator*() const { Iter tmp = current; return *--tmp; }
//   對 int* 這是零成本（編譯器最佳化掉），但對肥迭代器（deque、map）
//   理論上每次解參考都多做一次遞減。**大多數情況下最佳化器會處理掉**，
//   不需要為此改寫程式碼，但在極端熱點迴圈中值得知道有這件事。
//   另外，C++20 的 std::views::reverse 在概念上更乾淨（它操作的是 range
//   而不是硬塞一個偏移進迭代器），但底層對 bidirectional range 仍是同樣機制。
//
// 【注意事項 Pay Attention】
//   1. `*rit.base()` 取到的**不是** rit 所指的元素，而是它右邊那一格。
//      對 rbegin() 而言 base() 就是 end()，解參考 end() 是 UB。
//   2. 用 reverse_iterator 做 erase 要記得 +1：
//      `v.erase(std::next(rit).base())`。寫成 `v.erase(rit.base())` 不會
//      編譯錯誤，但會刪錯元素——這是最難抓的那種 bug。
//   3. rend() 不可解參考（就像 end() 一樣）。
//   4. forward_list 與 unordered_* 容器沒有 rbegin()/rend()。
//   5. 反向迭代器與正向迭代器的 distance 方向相反：
//      `std::distance(v.rbegin(), rit)` 算的是「從尾巴數過來第幾個」。
//   6. 容器被修改後反向迭代器同樣會失效，規則與正向迭代器一致。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】reverse_iterator 與 base()
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. reverse_iterator 是怎麼實作的？為什麼 rbegin() 內部存的是 end()？
//     答：它是一個轉接器，內部包一個正向迭代器 current，把 ++ 轉成 --、
//         把 operator* 實作成「複製一份 current、先 --、再解參考」。
//         rbegin() 內部存 end()，是因為若直接存「最後一個元素」，
//         那 rend() 就得存 begin()-1——而 begin 前一格在 C++ 物件模型裡
//         是 UB。整體平移一格後，內部值永遠落在合法的 [begin, end] 內。
//     追問：所以 &*rit 跟 rit.base() 是什麼關係？
//         → &*rit == &*(rit.base() - 1)，也就是 base() 永遠指向
//           rit 所指元素的右邊一格。
//
// 🔥 Q2. 拿到一個 reverse_iterator，想刪掉它指的那個元素，怎麼寫？
//     答：不能寫 v.erase(rit.base())——那會刪到右邊那一格。
//         正確寫法是 v.erase(std::next(rit).base())，
//         或等價的 v.erase((rit + 1).base())。
//         因為 next(rit) 往左移一格，它的 base() 就剛好回到 rit 所指的元素。
//     追問：erase 之後 rit 還能用嗎？
//         → 不行，與正向迭代器同樣的失效規則：vector 的 erase 會讓刪除
//           點之後（對反向而言是「之前」）的迭代器全部失效。
//
// ⚠️ 陷阱. 下面這段想印出「最後一個元素」，為什麼是 UB？
//         std::cout << *vec.rbegin().base();
//     答：rbegin().base() 就是 end()，解參考 end() 是 UB。
//         想取最後一個元素應該寫 *vec.rbegin()（reverse_iterator 的
//         operator* 已經幫你退了一格），或者直接用 vec.back()。
//     為什麼會錯：直覺以為 base() 是「把反向迭代器轉回正向、指向同一個
//         元素」的無害轉換。實際上 base() 帶著一格偏移，這個偏移是為了
//         避開 begin-1 的 UB 而刻意設計的。凡是看到 .base()，
//         都要先在心裡問一句「我要的是它本身，還是它左邊那一格？」
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <iterator>   // std::next, std::reverse_iterator
#include <algorithm>  // std::find

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 344. Reverse String
//   題目：原地反轉字元陣列。
//   為什麼用到本主題：這裡示範用 reverse_iterator 的角度理解反轉——
//     std::reverse 的標準做法是「正向迭代器與反向迭代器相向而行」。
//     這個寫法完全不需要處理 begin-1，也不需要處理奇數長度的中間元素，
//     因為半開區間 + 反向迭代器的組合已經幫我們把邊界處理乾淨了。
//   複雜度：O(n) 時間、O(1) 額外空間。
// -----------------------------------------------------------------------------
void reverseString(std::string& s) {
    auto first = s.begin();
    auto last = s.rbegin();
    // 前後各走 n/2 步；用 size 控制次數，避免兩種迭代器互相比較
    for (std::size_t i = 0; i < s.size() / 2; ++i, ++first, ++last) {
        std::swap(*first, *last);
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例】從檔案路徑取出副檔名
//   情境：處理上傳檔案時要依副檔名分流（圖片走 CDN、文件走文件庫）。
//   為什麼用 reverse_iterator：副檔名是「最後一個 '.' 之後的部分」，
//     從尾巴往前找比從頭找再記錄最後位置更直接。
//     這裡示範 base() 的正確用法：找到反向位置後，用 base() 轉回正向
//     迭代器——由於 base() 天生偏移一格，rit.base() 剛好就指向 '.' 的
//     下一個字元，正是副檔名的起點，這次偏移反而幫了我們。
// -----------------------------------------------------------------------------
std::string fileExtension(const std::string& path) {
    // 從尾巴往前找第一個 '.'
    auto rit = std::find(path.rbegin(), path.rend(), '.');
    if (rit == path.rend()) return "";        // 沒有副檔名

    // rit 指向 '.'，rit.base() 指向 '.' 的右邊一格 = 副檔名第一個字元
    return std::string(rit.base(), path.end());
}

int main() {
    std::vector<int> vec = {10, 20, 30, 40, 50};

    // 正向遍歷
    std::cout << "=== 正反向遍歷 ===" << std::endl;
    std::cout << "正向: ";
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // 反向遍歷
    std::cout << "反向: ";
    for (auto rit = vec.rbegin(); rit != vec.rend(); ++rit) {
        std::cout << *rit << " ";
    }
    std::cout << std::endl;

    // 唯讀反向（C++11 的 crbegin/crend）
    std::cout << "唯讀反向 (crbegin/crend): ";
    for (auto rit = vec.crbegin(); rit != vec.crend(); ++rit) {
        std::cout << *rit << " ";
    }
    std::cout << std::endl;

    // base() 的一格偏移 —— 本檔最重要的觀念
    std::cout << "\n=== base() 的一格偏移 ===" << std::endl;
    auto rit = vec.rbegin();
    std::cout << "*rbegin()               = " << *rit << "（最後一個元素）" << std::endl;
    std::cout << "rbegin().base() == end()? "
              << (rit.base() == vec.end() ? "是" : "否")
              << "（所以 *rbegin().base() 是 UB，不可寫）" << std::endl;

    auto rit2 = vec.rbegin() + 1;
    std::cout << "*(rbegin()+1)           = " << *rit2 << std::endl;
    std::cout << "*((rbegin()+1).base())  = " << *(rit2.base())
              << "（base() 指向右邊一格！）" << std::endl;

    std::cout << "rend().base() == begin()? "
              << (vec.rend().base() == vec.begin() ? "是" : "否") << std::endl;

    // 驗證通式：&*rit == &*(rit.base() - 1)
    std::cout << "\n=== 驗證 &*rit == &*(rit.base()-1) ===" << std::endl;
    for (auto r = vec.rbegin(); r != vec.rend(); ++r) {
        bool same = (&*r == &*(r.base() - 1));
        std::cout << "  *rit = " << *r << " , 關係成立? " << (same ? "是" : "否") << std::endl;
    }

    // 用 reverse_iterator 刪除元素的正確寫法
    std::cout << "\n=== 用 reverse_iterator 刪元素（正確做法）===" << std::endl;
    std::vector<int> v2 = {10, 20, 30, 40, 50};
    auto r = std::find(v2.rbegin(), v2.rend(), 30);   // 從後往前找 30
    if (r != v2.rend()) {
        std::cout << "找到 " << *r << "，用 std::next(r).base() 刪除" << std::endl;
        v2.erase(std::next(r).base());   // 正確：先 +1 再 base()
    }
    std::cout << "刪除後: ";
    for (int n : v2) std::cout << n << " ";
    std::cout << std::endl;
    std::cout << "（若誤寫 v2.erase(r.base()) 會刪掉 40，也就是右邊那一格）" << std::endl;

    // 反向搜尋：找最後一個符合的元素
    std::cout << "\n=== 反向搜尋：找最後一個 20 ===" << std::endl;
    std::vector<int> v3 = {20, 10, 20, 30, 20, 40};
    auto r3 = std::find(v3.rbegin(), v3.rend(), 20);
    if (r3 != v3.rend()) {
        // 換算成正向索引：base() 已偏移一格，所以要 -1
        auto idx = (r3.base() - 1) - v3.begin();
        std::cout << "最後一個 20 的索引 = " << idx << std::endl;
        std::cout << "從尾巴數過來第 " << std::distance(v3.rbegin(), r3) << " 個（0 起算）" << std::endl;
    }

    std::cout << "\n=== LeetCode 344. Reverse String ===" << std::endl;
    std::string s = "hello";
    std::cout << "反轉前: " << s << std::endl;
    reverseString(s);
    std::cout << "反轉後: " << s << std::endl;

    std::cout << "\n=== 日常實務：取出副檔名 ===" << std::endl;
    const char* paths[] = {
        "/var/log/nginx/access.log",
        "report.2026-07-19.tar.gz",
        "/etc/hostname",
        "archive.tar."
    };
    for (const char* p : paths) {
        std::string ext = fileExtension(p);
        std::cout << "  " << p << "  ->  副檔名 [" << ext << "]" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第四課：迭代器（Iterator）的核心概念6.cpp" -o iter6

// === 預期輸出 ===
// === 正反向遍歷 ===
// 正向: 10 20 30 40 50
// 反向: 50 40 30 20 10
// 唯讀反向 (crbegin/crend): 50 40 30 20 10
//
// === base() 的一格偏移 ===
// *rbegin()               = 50（最後一個元素）
// rbegin().base() == end()? 是（所以 *rbegin().base() 是 UB，不可寫）
// *(rbegin()+1)           = 40
// *((rbegin()+1).base())  = 50（base() 指向右邊一格！）
// rend().base() == begin()? 是
//
// === 驗證 &*rit == &*(rit.base()-1) ===
//   *rit = 50 , 關係成立? 是
//   *rit = 40 , 關係成立? 是
//   *rit = 30 , 關係成立? 是
//   *rit = 20 , 關係成立? 是
//   *rit = 10 , 關係成立? 是
//
// === 用 reverse_iterator 刪元素（正確做法）===
// 找到 30，用 std::next(r).base() 刪除
// 刪除後: 10 20 40 50
// （若誤寫 v2.erase(r.base()) 會刪掉 40，也就是右邊那一格）
//
// === 反向搜尋：找最後一個 20 ===
// 最後一個 20 的索引 = 4
// 從尾巴數過來第 1 個（0 起算）
//
// === LeetCode 344. Reverse String ===
// 反轉前: hello
// 反轉後: olleh
//
// === 日常實務：取出副檔名 ===
//   /var/log/nginx/access.log  ->  副檔名 [log]
//   report.2026-07-19.tar.gz  ->  副檔名 [gz]
//   /etc/hostname  ->  副檔名 []
//   archive.tar.  ->  副檔名 []
