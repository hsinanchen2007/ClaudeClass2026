// =============================================================================
//  第五課：迭代器的五種分類 4  —  手動操作 ostream_iterator：*it = v 的真面目
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<iterator>
//   類別：template <class T, class CharT = char, ...> class ostream_iterator;
//   建構：std::ostream_iterator<int> out(std::cout, " ");
//         第二個參數 delim 是**每個元素之後**都會輸出的字串（可省略）。
//   迭代器類別：**Output Iterator**
//     支援：*it = value（寫入）、++it / it++、可複製
//     不支援：讀取、--it、it + n、it1 < it2
//   標準版本：C++98；C++20 起 ostream_iterator 的 delim 型別要求略有調整
//             （改用 CharT 的字串），行為不變。
//   複雜度：每次寫入 O(1) 加上串流本身的成本。
//
// 【詳細解釋 Explanation】
//
// 【1. 拆解 *out = 10 這一行到底發生什麼】
//   ostream_iterator 的核心實作只有三個成員函式：
//       ostream_iterator& operator*()  { return *this; }        // 回傳自己
//       ostream_iterator& operator++() { return *this; }        // 空操作
//       ostream_iterator& operator=(const T& v) {               // 真正做事的地方
//           *out_stream << v;
//           if (delim) *out_stream << delim;
//           return *this;
//       }
//   所以 `*out = 10;` 的執行順序是：
//       (a) *out    → 回傳 out 自己（不是什麼「元素的參考」）
//       (b) = 10    → 呼叫 operator=，把 10 送進串流並附上分隔符
//   換句話說，這裡的 `*` 和 `=` **都不是它們字面上的意思** ——
//   整個運算式只是「寫入一個值」的偽裝。
//
// 【2. 為什麼 operator++ 什麼都不做】
//   對真正的迭代器，++ 代表「移動到下一個位置」。
//   但 ostream_iterator 沒有「位置」的概念 —— 串流的寫入位置由串流自己維護，
//   每次 << 之後自動前進。所以迭代器這邊無事可做，直接回傳自己即可。
//   標準之所以還是要求它提供 operator++，純粹是為了滿足
//   Output Iterator 的介面契約，好讓 std::copy 之類的演算法能一視同仁地使用它。
//   這是「為了介面一致性而存在的空操作」的經典案例。
//
// 【3. 為什麼「不能讀取」——即使 int x = *out 可能編得過】
//   *out 回傳的是 ostream_iterator 自己，型別不是 int。
//   `int x = *out;` 因為型別不符會編譯錯誤（沒有從 ostream_iterator 到 int 的轉換）。
//   重點是概念上：**輸出串流沒有「目前的值」可以讀** ——
//   資料一旦寫出去就在串流那邊了，迭代器手上什麼都沒留。
//   這正是 Output Iterator 與 Input Iterator 能力互不包含的原因。
//
// 【4. 什麼時候該手動操作、什麼時候該用 std::copy】
//   手動 `*out = v; ++out;` 幾乎只出現在教學或自訂演算法內部。
//   日常程式碼一律用 std::copy / std::transform 把它交出去：
//       std::copy(v.begin(), v.end(), std::ostream_iterator<int>(cout, " "));
//   理由是意圖更清楚、也不會忘記 ++（雖然對 ostream_iterator 忘了也沒差 ——
//   但對 back_inserter 之外的其他 Output Iterator 可能就有差了）。
//
// 【概念補充 Concept Deep Dive】
//   「operator* 回傳自己」這個技巧叫做 **proxy（代理）物件**，
//   在 STL 中出現在好幾個地方，值得認得：
//     - ostream_iterator / back_insert_iterator：*it 回傳自己，靠 operator= 做事
//     - std::vector<bool>：operator[] 回傳一個 proxy 位元參考，而非真正的 bool&
//       （這就是為什麼 for (auto& b : vbool) 編不過，得寫 auto&&）
//     - std::map 的 operator[]：回傳 mapped_type&，但會「順手插入」不存在的鍵
//   proxy 的共同代價是「它看起來像 T，但不是 T」——
//   一旦用 auto 推導或取位址就可能出乎意料。
//   認得這個模式，遇到「明明是 bool 卻不能綁 bool&」這類怪事時就知道往哪查。
//
// 【注意事項 Pay Attention】
//   1. `*out = v` 中的 `*` 與 `=` 都不是字面意思，整體只是「寫入一個值」。
//   2. operator++ 是空操作，但仍應照寫（介面契約；換成其他 Output Iterator 就有意義）。
//   3. 分隔符加在**每個**元素之後，包含最後一個 → 結尾會多一個。
//   4. ostream_iterator 只能寫不能讀。
//   5. 綁定的串流必須活得比迭代器久 —— 它內部只存一個串流指標。
//   6. 它是 proxy 物件；用 auto 接 *out 會得到迭代器本身，不是元素。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】ostream_iterator 的運作原理
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. `*out = 10;` 這行對 ostream_iterator 做了什麼？
//     答：兩步。(a) operator* 回傳迭代器**自己**（不是元素參考）；
//         (b) operator= 被呼叫，把 10 送進串流並附上建構時給的分隔符。
//         所以整行的效果就是 `cout << 10 << " "`。
//         這裡的 `*` 與 `=` 都是被多載成「偽裝成迭代器語法」的寫入操作。
//     追問：那 operator++ 做了什麼？
//           → 什麼都沒做，直接回傳自己。因為串流的寫入位置由串流自己維護，
//             迭代器沒有「位置」可以移動。它存在純粹是為了滿足
//             Output Iterator 的介面契約，好讓 std::copy 能一視同仁地使用。
//
// 🔥 Q2. 為什麼 ostream_iterator 的分隔符會在結尾多出一個？
//     答：因為它是**後綴**而非分隔符 —— 每寫入一個元素就跟著輸出一次。
//         而 Output Iterator 在寫入時根本不知道自己是不是最後一個
//         （沒有 end 可比、不能回頭），所以做不到「只在中間加」。
//     追問：那要怎麼避免尾端多餘的分隔符？
//           → 先輸出前 n-1 個（帶分隔符），再單獨輸出最後一個；
//             或手寫迴圈；C++20 起可用 std::views::join_with。
//
// ⚠️ 陷阱. 「*out 回傳的是迭代器自己，那我寫 *out = 1; *out = 2; 不 ++，
//          是不是就會覆蓋掉第一個值？」
//     答：不會覆蓋，兩個值都會被輸出（"1 2 "）。
//         因為 ostream_iterator 根本沒有「位置」的概念 ——
//         每次 operator= 都是無條件把值 << 進串流，串流自己往後推進。
//         ++ 是空操作，加不加都一樣。
//     為什麼會錯：把它當成一般的迭代器來推理 ——
//         「不移動就會寫到同一個位置」對 vector 的迭代器成立，
//         但 ostream_iterator 只是偽裝成迭代器的**寫入動作包裝**。
//         附帶提醒：即使如此，寫程式時仍應照慣例寫 ++out ——
//         因為換成別的 Output Iterator（例如自訂的、真的有位置概念的）
//         漏掉 ++ 就會出事，而演算法內部也一律會呼叫它。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <iterator>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】不強加。
//   理由：本檔拆解的是 ostream_iterator 這個**配接器的內部機制**，
//         屬於「STL 怎麼實作出來的」層次，不是解題技巧。
//         LeetCode 從不要求你輸出格式化結果（回傳值即可），
//         更不會考 operator* 回傳自己這種實作細節。
//         這個知識真正的用途是「看懂 STL 原始碼」與「自己寫配接器」——
//         下面的實務範例即示範後者。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】自訂一個 Output Iterator：把寫入轉成「累加 + 記錄」
//   情境：要統計一條資料管線總共輸出了多少筆、總金額多少，
//         但又不想改動既有的演算法呼叫（那些 std::copy / std::transform 已經寫好了）。
//   為什麼用到本主題：只要照著 ostream_iterator 的模式寫一個自訂 Output Iterator
//         （operator* 回傳自己、operator++ 空操作、operator= 做事），
//         就能把它塞進任何 STL 演算法的輸出端，
//         在完全不動演算法程式碼的情況下攔截所有寫入。
//         這是「配接器」在真實專案中最實用的形式。
// -----------------------------------------------------------------------------
class SummingIterator {
    long long*  total_;
    std::size_t* count_;
public:
    // Output Iterator 要提供的 traits（讓 STL 演算法能識別它）
    using iterator_category = std::output_iterator_tag;
    using value_type        = void;
    using difference_type   = std::ptrdiff_t;
    using pointer           = void;
    using reference         = void;

    SummingIterator(long long* total, std::size_t* count)
        : total_(total), count_(count) {}

    SummingIterator& operator*()  { return *this; }   // 回傳自己
    SummingIterator& operator++() { return *this; }   // 空操作
    SummingIterator  operator++(int) { return *this; }

    SummingIterator& operator=(int v) {               // 真正做事的地方
        *total_ += v;
        ++(*count_);
        return *this;
    }
};

int main() {
    // ostream_iterator 是 Output Iterator
    std::ostream_iterator<int> out(std::cout, " ");
    //auto out = std::ostream_iterator<int>(std::cout, " ");

    // 可以寫入
    *out = 10;  // 輸出 "10 "
    ++out;
    *out = 20;  // 輸出 "20 "
    ++out;
    *out = 30;  // 輸出 "30 "

    std::cout << std::endl;

    // 但不能讀取！
    // int x = *out;  // 這不會如你預期的工作

    // 也不能回頭或跳躍
    // --out;  // 不支援
    // out + 1;  // 不支援

    // 實際演示：不 ++ 也不會覆蓋（因為它根本沒有「位置」）
    std::cout << "\n=== 不呼叫 ++ 會怎樣 ===" << std::endl;
    {
        std::ostringstream oss;
        std::ostream_iterator<int> o(oss, " ");
        *o = 1;          // 刻意不 ++
        *o = 2;
        *o = 3;
        std::cout << "  連續三次 *o = v（完全不 ++）: [" << oss.str() << "]" << std::endl;
        std::cout << "  → 三個值都輸出了，沒有覆蓋 ——"
                     "ostream_iterator 沒有「位置」的概念" << std::endl;
        std::cout << "  （但仍應照慣例寫 ++，因為其他 Output Iterator 可能真的需要）"
                  << std::endl;
    }

    // 分隔符是後綴
    std::cout << "\n=== 分隔符加在每個元素之後 ===" << std::endl;
    {
        std::ostringstream oss;
        std::vector<int> v = {1, 2, 3};
        std::copy(v.begin(), v.end(), std::ostream_iterator<int>(oss, "-"));
        std::cout << "  delim=\"-\" 的結果: [" << oss.str() << "]  ← 結尾也有一個"
                  << std::endl;
    }

    // 沒有 delim 的版本
    std::cout << "\n=== 省略 delim ===" << std::endl;
    {
        std::ostringstream oss;
        std::vector<int> v = {1, 2, 3};
        std::copy(v.begin(), v.end(), std::ostream_iterator<int>(oss));
        std::cout << "  不給 delim: [" << oss.str() << "]" << std::endl;
    }

    // 交給 std::copy 才是日常寫法
    std::cout << "\n=== 日常寫法：交給 std::copy ===" << std::endl;
    {
        std::vector<std::string> names = {"alice", "bob", "carol"};
        std::cout << "  ";
        std::copy(names.begin(), names.end(),
                  std::ostream_iterator<std::string>(std::cout, ", "));
        std::cout << std::endl;
    }

    std::cout << "\n=== 日常實務：自訂 Output Iterator 攔截寫入 ===" << std::endl;
    {
        std::vector<int> orders = {1200, 340, 5600, 890, 2300};

        long long   total = 0;
        std::size_t count = 0;

        // 完全不改動演算法呼叫，只換掉輸出迭代器
        std::copy_if(orders.begin(), orders.end(),
                     SummingIterator(&total, &count),
                     [](int amount) { return amount >= 1000; });

        std::cout << "  訂單: 1200 340 5600 890 2300（門檻 1000）" << std::endl;
        std::cout << "  攔截到 " << count << " 筆，總金額 " << total << std::endl;

        // 換成 transform 也一樣通用
        long long   doubled_total = 0;
        std::size_t doubled_count = 0;
        std::transform(orders.begin(), orders.end(),
                       SummingIterator(&doubled_total, &doubled_count),
                       [](int amount) { return amount * 2; });
        std::cout << "  全部加倍後: " << doubled_count << " 筆，總金額 "
                  << doubled_total << std::endl;
        std::cout << "  → 同一個自訂迭代器，能插進任何演算法的輸出端" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第五課：迭代器的五種分類4.cpp -o demo4

// === 預期輸出 ===
// 10 20 30
//
// === 不呼叫 ++ 會怎樣 ===
//   連續三次 *o = v（完全不 ++）: [1 2 3 ]
//   → 三個值都輸出了，沒有覆蓋 ——ostream_iterator 沒有「位置」的概念
//   （但仍應照慣例寫 ++，因為其他 Output Iterator 可能真的需要）
//
// === 分隔符加在每個元素之後 ===
//   delim="-" 的結果: [1-2-3-]  ← 結尾也有一個
//
// === 省略 delim ===
//   不給 delim: [123]
//
// === 日常寫法：交給 std::copy ===
//   alice, bob, carol,
//
// === 日常實務：自訂 Output Iterator 攔截寫入 ===
//   訂單: 1200 340 5600 890 2300（門檻 1000）
//   攔截到 3 筆，總金額 9100
//   全部加倍後: 5 筆，總金額 20660
//   → 同一個自訂迭代器，能插進任何演算法的輸出端
