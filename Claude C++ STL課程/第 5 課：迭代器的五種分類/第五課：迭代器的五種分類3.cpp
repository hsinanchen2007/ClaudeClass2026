// =============================================================================
//  第五課：迭代器的五種分類 3  —  Output Iterator：ostream_iterator 與 back_inserter
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<iterator>
//   兩個代表：
//     std::ostream_iterator<T>(os, delim)   寫入輸出串流，每個元素後接 delim
//     std::back_inserter(container)         回傳 back_insert_iterator，寫入 = push_back
//     std::front_inserter(container)        寫入 = push_front（vector 不能用）
//     std::inserter(container, pos)         寫入 = insert(pos, v)（set/map 唯一選擇）
//   迭代器類別：**Output Iterator**（與 Input 並列，能力互不包含）
//     可以：*it = value（**只寫**）、++it、可複製
//     不行：讀取（int x = *it 無意義）、--it、it + n、比較大小
//   標準版本：C++98；back_inserter / front_inserter / inserter 皆自 C++98。
//   複雜度：每次寫入 O(1)（back_inserter 為攤銷 O(1)，可能觸發重新配置）。
//
// 【詳細解釋 Explanation】
//
// 【1. 「只寫」的迭代器有什麼用】
//   STL 演算法（copy / transform / copy_if / merge / set_union…）的最後一個參數
//   通常叫 `d_first` —— 它只被用來「寫入」，從不讀取。
//   把這個位置抽象成 Output Iterator，就換來了驚人的彈性：
//       std::copy(v.begin(), v.end(), dest.begin());                   // 寫進容器
//       std::copy(v.begin(), v.end(), std::back_inserter(dest));       // 自動 push_back
//       std::copy(v.begin(), v.end(), std::ostream_iterator<int>(cout, " "));  // 印到螢幕
//       std::copy(v.begin(), v.end(), std::ostream_iterator<int>(file, "\n"));  // 寫進檔案
//   **同一個 std::copy**，換一個輸出迭代器就做完全不同的事。
//   這是配接器模式（adapter pattern）在 STL 中最漂亮的展現。
//
// 【2. back_inserter 的實作只有三個函式（而且都很怪）】
//       back_insert_iterator& operator=(const T& v) { c->push_back(v); return *this; }
//       back_insert_iterator& operator*()  { return *this; }   // 回傳自己
//       back_insert_iterator& operator++() { return *this; }   // 什麼都不做
//   `*out = value` 這個運算式因此被轉譯成 `c->push_back(value)`：
//   operator* 回傳自己（所以 *out 還是那個迭代器物件），
//   接著 operator= 才是真正做事的地方。
//   operator++ 是空操作，因為「下一個寫入位置」對 push_back 而言不需要追蹤。
//   這種「解參考回傳自己」的技巧值得記住 ——
//   它讓一個不是指標的東西，能完美偽裝成迭代器介面。
//
// 【3. 為什麼需要 back_inserter：copy 不會讓容器長大】
//   std::copy 的實作核心就是 `*d_first = *first; ++d_first;`，
//   完全沒有任何「改變容器大小」的動作 —— 它根本拿不到容器。
//   所以直接寫 std::copy(src.begin(), src.end(), dest.begin()) 時，
//   dest **必須已經有足夠的元素**，否則就是越界寫入（未定義行為）。
//   back_inserter 把「寫入」轉譯成「push_back」，容器才會自動長大。
//
// 【4. 三種插入迭代器的選擇】
//     back_inserter(c)   → push_back；vector/deque/list/string 可用，set 不行
//     front_inserter(c)  → push_front；deque/list/forward_list 可用，
//                          **vector 不行**（沒有 push_front）；
//                          且結果順序會**反轉**（每個新元素都插到最前面）
//     inserter(c, pos)   → insert(pos, v)；最通用，關聯容器（set/map）唯一選擇
//                          對 set 而言 pos 只是效能提示，實際位置由排序決定
//
// 【概念補充 Concept Deep Dive】
//   ostream_iterator 的分隔符是「後綴」而非「分隔」，這個設計常讓人困擾：
//       std::copy(v.begin(), v.end(), std::ostream_iterator<int>(cout, ", "));
//       // 輸出 "1, 2, 3, "  ← 結尾多一個逗號
//   為什麼標準不做成「只在中間加」？因為 Output Iterator 是**無狀態的單向寫入**——
//   它在寫入某個元素時，根本不知道自己是不是最後一個
//   （沒有 end 可以比較，也不允許回頭）。
//   要做到「只在中間加分隔符」，就必須先知道總數或有能力回溯，
//   那已經超出 Output Iterator 的能力範圍了。
//   標準直到 **C++20 才提供 std::views::join_with** 正式解決這件事；
//   在此之前的作法是：手寫迴圈、或先輸出 n-1 個再單獨輸出最後一個
//   （本檔的實務範例即示範後者）。
//
// 【注意事項 Pay Attention】
//   1. Output Iterator **不能讀取**：int x = *out 沒有意義（雖然可能編得過）。
//   2. 對每個位置只能寫入一次，且必須寫入後才 ++（單次走訪）。
//   3. std::copy 不會讓容器長大；目的地必須已有元素，或使用 back_inserter。
//   4. reserve() **不能**取代 resize()：reserve 只配置 capacity，size 仍是 0，
//      begin() 依然等於 end()，直接寫入仍是未定義行為。
//   5. ostream_iterator 的分隔符加在**每個**元素之後，包含最後一個。
//   6. vector 沒有 push_front，因此不能用 front_inserter（編譯錯誤）。
//   7. set / map 只能用 inserter，不能用 back_inserter / front_inserter。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Output Iterator
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::copy 本身不改變容器大小，back_inserter 是怎麼讓它「長大」的？
//     答：back_inserter 回傳一個 back_insert_iterator，它把 operator= 多載成
//         「呼叫容器的 push_back」，而 operator* 與 operator++ 都只是回傳自己
//         （空操作）。於是 copy 內部的 `*d_first = *first` 實際執行的是
//         `c.push_back(*first)`。copy 完全不知道發生了什麼，
//         它只是照著迭代器介面操作 —— 這就是配接器的精髓。
//     追問：那對 std::set 該用哪一個？
//           → std::inserter(s, s.begin())。set 沒有 push_back，
//             而且插入位置由排序決定，傳入的 pos 只是效能提示。
//
// 🔥 Q2. Output Iterator 為什麼與 Input Iterator 並列，而不是誰比誰弱？
//     答：因為兩者的能力**互不包含**：Input 只能讀不能寫，Output 只能寫不能讀。
//         五種分類的階層圖開頭是分叉的，兩者之上才匯流成 Forward ——
//         Forward 是第一個同時能讀能寫、且保證多次走訪的分類。
//     追問：那 back_inserter 為什麼不能是更強的分類？
//           → 它連「目前寫到哪」都沒有概念（operator++ 是空操作），
//             更談不上回頭或跳躍。它的能力剛好就是 Output Iterator 的定義。
//
// ⚠️ 陷阱. std::vector<int> dst;
//          dst.reserve(src.size());                      // 我已經配置空間了
//          std::copy(src.begin(), src.end(), dst.begin());
//          —— 為什麼這樣還是錯的？
//     答：因為 reserve 改變的是 **capacity**，不是 **size**。
//         reserve 之後 dst.size() 仍然是 0，元素還沒被建構，
//         dst.begin() 依然等於 dst.end() —— 往那裡寫入就是越界寫入，未定義行為。
//         而且症狀往往不是立刻崩潰：資料可能真的被寫進了已配置的記憶體，
//         看起來「好像成功了」，但 dst.size() 仍是 0，之後走訪什麼都印不出來。
//     為什麼會錯：把 capacity 與 size 混為一談，
//         以為「配置了空間」就等於「有元素可以寫」。
//         迭代器指向的是**已建構的元素**，不是原始記憶體。
//         正解是 dst.resize(src.size())（真的建構元素），
//         或用 std::back_inserter(dst)（讓寫入變成 push_back）——
//         後者還能保留 reserve 的效能好處。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <iterator>
#include <vector>
#include <deque>
#include <set>
#include <string>
#include <sstream>
#include <algorithm>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】不強加。
//   理由：Output Iterator 在 LeetCode 上只會以「解法中的一個零件」出現
//         （例如用 back_inserter 收集結果），從來不是題目本身考的東西。
//         同一課的 summary.cpp 已用 LeetCode 26 示範
//         「演算法對迭代器分類的要求」這個真正的主題；
//         在這裡再掛一題只是重複，反而模糊了「輸出端也能被抽象」的重點。
//         真正需要理解 Output Iterator 的場合是「同一份資料要輸出到不同去處」——
//         那正是下面實務範例的情境。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】報表匯出：同一份資料，三種輸出去處
//   情境：查詢結果要能同時 (1) 印到終端機給人看、(2) 寫成 CSV 給下游匯入、
//         (3) 存進記憶體容器供後續運算。
//   為什麼用到本主題：這三件事若各寫一個函式，就是三份幾乎一樣的迴圈。
//         用 Output Iterator 抽象之後，**一個 template 函式**通吃 ——
//         呼叫端只要傳不同的輸出迭代器。
//         這正是「把輸出端也抽象化」帶來的實際價值。
// -----------------------------------------------------------------------------
struct SalesRecord {
    std::string region;
    int         amount;
};

// 一份邏輯，任何輸出去處都適用（只要求 Output Iterator）
template <typename OutIt>
OutIt exportHighSales(const std::vector<SalesRecord>& data, int threshold, OutIt out) {
    for (const SalesRecord& r : data) {
        if (r.amount >= threshold) {
            *out = r.region + "," + std::to_string(r.amount);
            ++out;
        }
    }
    return out;   // 依慣例回傳「寫到哪裡」，讓呼叫端可以接續
}

int main() {
    std::vector<int> numbers = {10, 20, 30, 40, 50};

    // ostream_iterator 是 Output Iterator
    std::cout << "=== ostream_iterator ===" << std::endl;
    std::cout << "輸出: ";

    // ostream_iterator 讓你可以「寫入」到輸出串流
    std::copy(numbers.begin(), numbers.end(),
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;

    // back_inserter 也是 Output Iterator
    std::cout << "\n=== back_inserter ===" << std::endl;
    std::vector<int> source = {1, 2, 3};
    std::vector<int> dest;

    // back_inserter 讓你可以「寫入」到 vector 尾端
    std::copy(source.begin(), source.end(),
              std::back_inserter(dest));

    std::cout << "dest: ";
    for (int n : dest) std::cout << n << " ";
    std::cout << std::endl;

    // 為什麼一定要 back_inserter：copy 不會讓容器長大
    std::cout << "\n=== 為什麼不能直接傳 dest.begin() ===" << std::endl;
    {
        std::vector<int> a;
        a.reserve(source.size());          // 只配置 capacity，size 仍是 0
        std::cout << "  reserve(3) 後: size=" << a.size()
                  << " capacity=" << a.capacity() << std::endl;
        std::cout << "  → begin()==end()? " << (a.begin() == a.end() ? "是" : "否")
                  << "  所以寫入 begin() 是越界（本檔刻意不執行）" << std::endl;

        std::vector<int> b;
        b.resize(source.size());           // 真的建構了 3 個元素
        std::copy(source.begin(), source.end(), b.begin());   // 這樣才安全
        std::cout << "  resize(3) + copy 到 begin(): ";
        for (int n : b) std::cout << n << " ";
        std::cout << "  (size=" << b.size() << ")" << std::endl;
    }

    // 三種插入迭代器的差異
    std::cout << "\n=== 三種插入迭代器 ===" << std::endl;
    {
        std::vector<int> vb;
        std::copy(source.begin(), source.end(), std::back_inserter(vb));
        std::cout << "  back_inserter (vector): ";
        for (int n : vb) std::cout << n << " ";
        std::cout << "  ← 保持原順序" << std::endl;

        std::deque<int> df;                // vector 沒有 push_front，只能用 deque
        std::copy(source.begin(), source.end(), std::front_inserter(df));
        std::cout << "  front_inserter (deque): ";
        for (int n : df) std::cout << n << " ";
        std::cout << "  ← 順序被反轉！" << std::endl;

        std::set<int> si;
        std::copy(source.begin(), source.end(), std::inserter(si, si.begin()));
        std::cout << "  inserter (set)        : ";
        for (int n : si) std::cout << n << " ";
        std::cout << "  ← set 唯一可用的插入迭代器" << std::endl;
    }

    // ostream_iterator 的分隔符是「後綴」
    std::cout << "\n=== 分隔符是後綴，不是分隔 ===" << std::endl;
    {
        std::ostringstream oss;
        std::copy(source.begin(), source.end(),
                  std::ostream_iterator<int>(oss, ", "));
        std::cout << "  直接用 ostream_iterator: [" << oss.str() << "]" << std::endl;
        std::cout << "  → 結尾多了一個逗號（Output Iterator 不知道自己是不是最後一個）"
                  << std::endl;

        std::ostringstream oss2;
        if (!source.empty()) {
            std::copy(source.begin(), std::prev(source.end()),
                      std::ostream_iterator<int>(oss2, ", "));
            oss2 << source.back();
        }
        std::cout << "  先輸出 n-1 個再補最後一個: [" << oss2.str() << "]" << std::endl;
    }

    std::cout << "\n=== 日常實務：一份邏輯，三種輸出去處 ===" << std::endl;
    std::vector<SalesRecord> sales = {
        {"north",  12000}, {"south",  4300}, {"east",  25800},
        {"west",    9100}, {"center", 18700},
    };
    const int threshold = 10000;

    // (1) 直接印到終端機
    std::cout << "  [輸出到終端機]" << std::endl;
    std::cout << "    ";
    exportHighSales(sales, threshold,
                    std::ostream_iterator<std::string>(std::cout, " | "));
    std::cout << std::endl;

    // (2) 寫成 CSV 文字（實務上換成 std::ofstream 即可）
    std::ostringstream csv;
    exportHighSales(sales, threshold,
                    std::ostream_iterator<std::string>(csv, "\n"));
    std::cout << "  [輸出成 CSV]" << std::endl;
    {
        std::istringstream lines(csv.str());
        std::string line;
        while (std::getline(lines, line)) std::cout << "    " << line << std::endl;
    }

    // (3) 收集進容器供後續運算
    std::vector<std::string> collected;
    exportHighSales(sales, threshold, std::back_inserter(collected));
    std::cout << "  [收集進 vector] 共 " << collected.size() << " 筆" << std::endl;
    std::cout << "  → 同一個 exportHighSales，換個輸出迭代器就換了去處" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第五課：迭代器的五種分類3.cpp -o demo3

// === 預期輸出 ===
// === ostream_iterator ===
// 輸出: 10 20 30 40 50
//
// === back_inserter ===
// dest: 1 2 3
//
// === 為什麼不能直接傳 dest.begin() ===
//   reserve(3) 後: size=0 capacity=3
//   → begin()==end()? 是  所以寫入 begin() 是越界（本檔刻意不執行）
//   resize(3) + copy 到 begin(): 1 2 3   (size=3)
//
// === 三種插入迭代器 ===
//   back_inserter (vector): 1 2 3   ← 保持原順序
//   front_inserter (deque): 3 2 1   ← 順序被反轉！
//   inserter (set)        : 1 2 3   ← set 唯一可用的插入迭代器
//
// === 分隔符是後綴，不是分隔 ===
//   直接用 ostream_iterator: [1, 2, 3, ]
//   → 結尾多了一個逗號（Output Iterator 不知道自己是不是最後一個）
//   先輸出 n-1 個再補最後一個: [1, 2, 3]
//
// === 日常實務：一份邏輯，三種輸出去處 ===
//   [輸出到終端機]
//     north,12000 | east,25800 | center,18700 |
//   [輸出成 CSV]
//     north,12000
//     east,25800
//     center,18700
//   [收集進 vector] 共 3 筆
//   → 同一個 exportHighSales，換個輸出迭代器就換了去處
