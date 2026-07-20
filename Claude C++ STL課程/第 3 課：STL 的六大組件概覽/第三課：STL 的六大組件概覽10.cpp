// =============================================================================
//  第三課：STL 的六大組件概覽 10  —  迭代器配接器：reverse / inserter / stream
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<iterator>
//   三大類迭代器配接器：
//     反向    std::reverse_iterator<It>      由 rbegin()/rend() 取得
//     插入    std::back_insert_iterator<C>   由 std::back_inserter(c) 取得（→ push_back）
//             std::front_insert_iterator<C>  由 std::front_inserter(c) 取得（→ push_front）
//             std::insert_iterator<C>        由 std::inserter(c, pos) 取得（→ insert）
//     串流    std::ostream_iterator<T>       寫入輸出串流
//             std::istream_iterator<T>       從輸入串流讀取
//   標準版本：以上皆 C++98；C++11 新增 cbegin/cend/crbegin/crend、
//             std::begin/std::end 自由函式；C++14 新增 std::make_reverse_iterator。
//   複雜度：三者的每次操作都是 O(1)（插入型的攤銷 O(1)）。
//
// 【詳細解釋 Explanation】
//
// 【1. 配接器的統一精神：把「別的東西」偽裝成迭代器】
//   STL 演算法只認得迭代器介面（*、++、比較）。
//   於是只要把任何行為包裝成符合這個介面的物件，演算法就能為你所用：
//     - 想反向走訪 → 包一個「++ 時實際往回走」的迭代器
//     - 想寫進容器尾端 → 包一個「賦值時實際呼叫 push_back」的迭代器
//     - 想直接印到螢幕 → 包一個「賦值時實際 operator<<」的迭代器
//   這就是為什麼 std::copy 一個演算法就能同時做到「複製到容器」「印到 cout」
//   「寫進檔案」—— 差別只在你傳哪個輸出迭代器。
//
// 【2. reverse_iterator 的「差一位」設計（最容易搞混的地方）】
//   反向迭代器內部存的是一個正向迭代器 current，而解參考時回傳的是**前一個**元素：
//       T& operator*() const { It tmp = current; return *--tmp; }
//   也就是說：&*rit == &*(rit.base() - 1)。
//   為什麼要這樣繞？因為半開區間 [begin, end) 的兩端必須對應：
//       rbegin() 的 base() 是 end()   → 解參考得到最後一個元素
//       rend()   的 base() 是 begin() → 這是「past-the-end」，不可解參考
//   如果不差這一位，rend() 就得指向 begin() 的前一個位置 ——
//   而「陣列第一個元素之前的位址」在 C++ 中根本不是合法的指標值。
//   這個設計因此不是怪癖，而是為了避免未定義行為的必然選擇。
//
//   實務影響：拿 rit.base() 去 erase 時記得減一：
//       v.erase(std::next(rit).base());   // 刪掉 rit 指向的那個元素
//
// 【3. back_inserter 為什麼能讓 copy 自動長大】
//   std::copy 本身完全不會改變容器大小 —— 它只是 *out = *in; ++out。
//   把 out 換成 back_insert_iterator 之後：
//       back_insert_iterator& operator=(const T& v) { c->push_back(v); return *this; }
//       back_insert_iterator& operator*()  { return *this; }   // 回傳自己
//       back_insert_iterator& operator++() { return *this; }   // 什麼都不做
//   於是 *out = value 這個動作被轉譯成 push_back(value)，容器自然就長大了。
//   注意 operator* 和 operator++ 都是空操作、回傳自己 —— 這是很聰明的小把戲。
//
// 【4. ostream_iterator 的分隔符只加在「後面」】
//   std::ostream_iterator<int>(std::cout, " ") 會在**每個**元素後面加分隔符，
//   所以輸出結尾一定多一個空格：「1 2 3 」。
//   要做到「只在中間加逗號」，C++ 標準到 C++20 才提供
//   std::ranges::views::join_with；在此之前只能自己寫迴圈或用
//   experimental::ostream_joiner（非標準）。
//
// 【概念補充 Concept Deep Dive】
//   三種插入迭代器的取捨：
//     back_inserter(c)   → push_back，要求容器有 push_back
//                          vector / deque / list / string 可用；set / forward_list 不行
//     front_inserter(c)  → push_front，要求有 push_front
//                          deque / list / forward_list 可用；**vector 不行**
//                          注意它會讓結果順序**反過來**（每個新元素都插到最前面）
//     inserter(c, pos)   → insert(pos, v)，最通用；關聯容器（set/map）只能用這個
//                          對 set 而言 pos 只是「提示」，實際位置仍由排序決定
//   效能提醒：copy + back_inserter 無法預知元素個數，可能經歷多次重新配置。
//   若已知數量，先 reserve 再用 back_inserter，或改用 resize + 一般迭代器。
//
// 【注意事項 Pay Attention】
//   1. rend() 不可解參考（它是反向的 past-the-end）。
//   2. rit.base() 指向的是 rit 所指元素的**下一個**位置，兩者差一位。
//   3. vector 沒有 push_front，因此不能用 front_inserter（編譯錯誤）。
//   4. ostream_iterator 的分隔符會加在每個元素之後，包含最後一個。
//   5. 用 back_inserter 時目的地容器不必先 resize；反之若用一般迭代器當輸出，
//      目的地**必須**已有足夠元素，否則是未定義行為。
//   6. istream_iterator 是 Input Iterator：只能單次走訪，不能回頭，
//      而且預設建構的 istream_iterator 就是「串流結束」的哨兵。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】迭代器配接器
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::copy 本身不會改變容器大小，那 back_inserter 是怎麼讓它「長大」的？
//     答：back_inserter 回傳一個 back_insert_iterator，它把 operator= 多載成
//         「呼叫容器的 push_back」，而 operator* 與 operator++ 都只是回傳自己
//         （空操作）。於是 copy 內部的 *out = *in 實際執行的是 c.push_back(*in)。
//         copy 完全不知道發生了什麼，它只是照著迭代器介面操作。
//     追問：那對 std::set 要用哪個？
//           → std::inserter(s, s.begin())。set 沒有 push_back，
//             而且插入位置由排序決定，傳入的 pos 只是效能提示。
//
// 🔥 Q2. reverse_iterator 的 base() 回傳什麼？為什麼會「差一位」？
//     答：base() 回傳它內部持有的正向迭代器，而 *rit 等於 *(base() - 1)。
//         原因是半開區間必須對應：rbegin().base() == end()（解參考得到最後元素），
//         rend().base() == begin()。若不差這一位，rend() 就得指向
//         begin() 之前的位置，而那不是合法的指標值 —— 會是未定義行為。
//     追問：那要用反向迭代器刪除某個元素，該怎麼寫？
//           → v.erase(std::next(rit).base())。直接寫 v.erase(rit.base())
//             會刪到**後面一個**元素。
//
// ⚠️ 陷阱. std::vector<int> dst; std::copy(src.begin(), src.end(), dst.begin());
//          為什麼會出事？加了 dst.reserve(src.size()) 之後就對了嗎？
//     答：兩者都不對。dst 是空的，dst.begin() == dst.end()，寫入即越界 → 未定義行為。
//         reserve **不會**解決問題：它只配置 capacity，size 仍是 0，
//         元素還沒被建構，dst.begin() 依然等於 dst.end()。
//         正解是 std::back_inserter(dst)（讓寫入變成 push_back），
//         或先 dst.resize(src.size()) 讓元素真的存在。
//     為什麼會錯：把 capacity 和 size 混為一談，
//         以為「配置了空間」就等於「有元素可以寫」。
//         迭代器指向的是**已建構的元素**，不是原始記憶體。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <deque>
#include <set>
#include <string>
#include <sstream>
#include <iterator>
#include <algorithm>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 66. Plus One
//   題目：用陣列表示一個大整數（每格一位數，最高位在前），對它加一並回傳結果。
//         例如 [1,2,3] → [1,2,4]；[9,9] → [1,0,0]。
//   為什麼用到本主題：加法必須從**最低位**（陣列尾端）往前處理進位，
//         這正是反向迭代器 rbegin()/rend() 的定義用途。
//         最後若最高位仍有進位（全 9 的情形），還要在最前面插入 1 ——
//         這裡示範 insert(begin(), 1)。
//   複雜度：時間 O(N)、空間 O(1)（全 9 時 O(N)，因為要多一位）。
// -----------------------------------------------------------------------------
std::vector<int> plusOne(std::vector<int> digits) {
    // 用反向迭代器從最低位往高位走
    for (auto rit = digits.rbegin(); rit != digits.rend(); ++rit) {
        if (*rit < 9) {
            ++(*rit);
            return digits;          // 沒有進位，直接結束
        }
        *rit = 0;                   // 9 + 1 = 10，本位歸零，繼續往前進位
    }
    // 走完全部仍有進位 → 原本是 999...9，結果是 1000...0
    digits.insert(digits.begin(), 1);
    return digits;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】把查詢結果輸出成 CSV 一行（含去重與排序）
//   情境：報表服務要把「本次查詢命中的標籤」輸出成一行 CSV 供下游匯入，
//         標籤來源有重複，且要求字典序輸出。
//   為什麼用到本主題：
//     - std::inserter 把 copy 的結果丟進 std::set（自動去重 + 排序），
//       這是 set 唯一能用的插入迭代器
//     - std::ostream_iterator 直接把結果串到 ostringstream，不需手寫迴圈
//     - 並示範 ostream_iterator 的「尾端會多一個分隔符」該怎麼處理
// -----------------------------------------------------------------------------
std::string tagsToCsv(const std::vector<std::string>& raw_tags) {
    // set 沒有 push_back，只能用 inserter；順帶完成去重與排序
    std::set<std::string> uniq;
    std::copy(raw_tags.begin(), raw_tags.end(),
              std::inserter(uniq, uniq.begin()));

    if (uniq.empty()) return "";

    std::ostringstream oss;
    // 先輸出前 n-1 個（每個後面帶逗號），最後一個單獨輸出，避免尾逗號
    std::copy(uniq.begin(), std::prev(uniq.end()),
              std::ostream_iterator<std::string>(oss, ","));
    oss << *std::prev(uniq.end());
    return oss.str();
}

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5};

    // reverse_iterator：反向迭代
    std::cout << "反向迭代: ";
    for (auto rit = vec.rbegin(); rit != vec.rend(); ++rit) {
        std::cout << *rit << " ";
    }
    std::cout << std::endl;

    // back_inserter：自動在尾端插入
    std::vector<int> src = {10, 20, 30};
    std::vector<int> dest;

    std::copy(src.begin(), src.end(), std::back_inserter(dest));
    std::cout << "back_inserter 結果: ";
    for (int n : dest) std::cout << n << " ";
    std::cout << std::endl;

    // ostream_iterator：輸出到串流
    std::cout << "ostream_iterator: ";
    std::copy(vec.begin(), vec.end(),
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;

    // reverse_iterator 的 base() 差一位
    std::cout << "\n=== reverse_iterator 與 base() 的關係 ===" << std::endl;
    auto rit = vec.rbegin();                 // 指向最後一個元素 5
    std::cout << "  *rit            = " << *rit << std::endl;
    std::cout << "  *(rit.base()-1) = " << *(rit.base() - 1) << "  ← 兩者相同" << std::endl;
    std::cout << "  rit.base()==end()? " << (rit.base() == vec.end() ? "是" : "否") << std::endl;

    // 用反向迭代器刪除：記得 next(rit).base()
    std::vector<int> del = {1, 2, 3, 4, 5};
    auto r3 = std::find(del.rbegin(), del.rend(), 3);   // 從後面找 3
    del.erase(std::next(r3).base());                     // 正確刪掉 3
    std::cout << "  用反向迭代器刪掉 3 之後: ";
    for (int n : del) std::cout << n << " ";
    std::cout << std::endl;

    // 三種插入迭代器的差異
    std::cout << "\n=== 三種插入迭代器 ===" << std::endl;
    std::vector<int> source = {1, 2, 3};

    std::vector<int> b;
    std::copy(source.begin(), source.end(), std::back_inserter(b));
    std::cout << "  back_inserter (vector) : ";
    for (int n : b) std::cout << n << " ";
    std::cout << "  ← 保持原順序" << std::endl;

    std::deque<int> f;   // vector 沒有 push_front，這裡必須用 deque
    std::copy(source.begin(), source.end(), std::front_inserter(f));
    std::cout << "  front_inserter (deque) : ";
    for (int n : f) std::cout << n << " ";
    std::cout << "  ← 順序被反轉！" << std::endl;

    std::set<int> s;
    std::copy(source.begin(), source.end(), std::inserter(s, s.begin()));
    std::cout << "  inserter (set)         : ";
    for (int n : s) std::cout << n << " ";
    std::cout << "  ← set 唯一可用的插入迭代器" << std::endl;

    std::cout << "\n=== LeetCode 66. Plus One ===" << std::endl;
    auto show = [](const char* label, const std::vector<int>& v) {
        std::cout << "  " << label;
        for (int n : v) std::cout << n;
        std::cout << std::endl;
    };
    show("[1,2,3]   + 1 = ", plusOne({1, 2, 3}));
    show("[4,3,2,1] + 1 = ", plusOne({4, 3, 2, 1}));
    show("[9]       + 1 = ", plusOne({9}));
    show("[9,9,9]   + 1 = ", plusOne({9, 9, 9}));

    std::cout << "\n=== 日常實務：標籤輸出成 CSV（去重 + 排序）===" << std::endl;
    std::vector<std::string> tags = {
        "cpp", "stl", "iterator", "cpp", "algorithm", "stl", "container"
    };
    std::cout << "  原始 " << tags.size() << " 個標籤（含重複）" << std::endl;
    std::cout << "  CSV: " << tagsToCsv(tags) << std::endl;
    std::cout << "  （注意結尾沒有多餘逗號 —— ostream_iterator 預設會多一個，"
                 "此處特別處理過）" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第三課：STL 的六大組件概覽10.cpp -o demo10

// === 預期輸出 ===
// 反向迭代: 5 4 3 2 1
// back_inserter 結果: 10 20 30
// ostream_iterator: 1 2 3 4 5
//
// === reverse_iterator 與 base() 的關係 ===
//   *rit            = 5
//   *(rit.base()-1) = 5  ← 兩者相同
//   rit.base()==end()? 是
//   用反向迭代器刪掉 3 之後: 1 2 4 5
//
// === 三種插入迭代器 ===
//   back_inserter (vector) : 1 2 3   ← 保持原順序
//   front_inserter (deque) : 3 2 1   ← 順序被反轉！
//   inserter (set)         : 1 2 3   ← set 唯一可用的插入迭代器
//
// === LeetCode 66. Plus One ===
//   [1,2,3]   + 1 = 124
//   [4,3,2,1] + 1 = 4322
//   [9]       + 1 = 10
//   [9,9,9]   + 1 = 1000
//
// === 日常實務：標籤輸出成 CSV（去重 + 排序）===
//   原始 7 個標籤（含重複）
//   CSV: algorithm,container,cpp,iterator,stl
//   （注意結尾沒有多餘逗號 —— ostream_iterator 預設會多一個，此處特別處理過）
