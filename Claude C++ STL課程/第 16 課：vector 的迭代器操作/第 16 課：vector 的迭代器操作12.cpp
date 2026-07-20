// =============================================================================
//  第 16 課-12：std::next / std::prev —— 不動到原迭代器的「取位置」工具
// =============================================================================
//
// 【主題資訊 Information】
//   template<class InputIt>
//   constexpr InputIt std::next(InputIt it,
//                    typename iterator_traits<InputIt>::difference_type n = 1);
//   template<class BidirIt>
//   constexpr BidirIt std::prev(BidirIt it,
//                    typename iterator_traits<BidirIt>::difference_type n = 1);
//   標準版本：C++11 加入；C++17 加上 constexpr
//   複雜度：random access → O(1)；其他 → O(|n|)
//   標頭檔：<iterator>
//
// 【詳細解釋 Explanation】
//
// 【1. next/prev 補上了 advance 缺的那一半】
//   C++98 只有 std::advance，它就地修改迭代器並回傳 void。
//   這代表你沒辦法把「移動」寫進一個運算式裡：
//     v.erase(std::advance(it, 2));            // 編譯失敗，advance 回傳 void
//   C++11 補上 next/prev，改成「回傳新迭代器、不動原件」的函式風格：
//     v.erase(std::next(it, 2));               // 可以了
//   兩者是刻意的互補設計，不是重複功能。
//
// 【2. 預設 n = 1 讓最常見的用法變得極短】
//   實務上九成的需求是「下一個」與「上一個」，所以 n 有預設值：
//     std::next(it)          // 等同 ++副本
//     std::prev(v.end())     // 最後一個元素——這是最常用的慣用法
//   注意 v.end() 不可解參考，但 std::prev(v.end()) 就指向最後一個元素了。
//   這比 v.end() - 1 更好，因為後者只有 random access 迭代器才能寫。
//
// 【3. next 與 prev 的迭代器類別要求不同】
//   next 只要求 InputIterator（因為往前走誰都會）；
//   prev 要求 BidirectionalIterator（因為必須支援 --）。
//   所以 forward_list 可以 next 不能 prev，這在編譯期就會被擋下來。
//   順帶一提，next 也可以傳負數 n，但那時它同樣需要雙向迭代器——
//   標準規定「n 為負時，InputIt 必須滿足 bidirectional」。
//
// 【4. 為什麼 prev(v.end()) 對空容器是 UB】
//   空容器時 begin() == end()，往前退一步就跑到 begin() 之前，
//   那是不存在的位置。任何 prev/advance 的越界都是 undefined behavior，
//   標準庫不會替你檢查。要取最後一個元素前，務必先確認 !v.empty()。
//   （v.back() 也一樣：對空 vector 呼叫 back() 是 UB。）
//
// 【5. 它們是 constexpr（C++17 起）】
//   這代表可以用在編譯期計算的情境，例如 constexpr 函式裡走訪
//   std::array。C++11/14 時代的 next/prev 不是 constexpr，
//   這是常被忽略的版本差異。
//
// 【概念補充 Concept Deep Dive】
//   ▸ next 的實作只有三行
//       template<class It>
//       It next(It it, typename iterator_traits<It>::difference_type n = 1) {
//           std::advance(it, n);   // 注意 it 是「傳值」進來的副本
//           return it;
//       }
//     關鍵就在參數是傳值：advance 改的是副本，呼叫端的原件毫髮無傷。
//     所以 next 的複雜度必然等同 advance——它沒有任何魔法。
//   ▸ 為什麼不直接寫 it + n
//     對 vector 完全可以，而且更短。next/prev 的價值同樣在泛型：
//     當你不確定拿到的是哪種迭代器時，it + n 可能編不過。
//     另外 std::prev(v.end()) 比 v.end() - 1 更能表達意圖，也更通用。
//   ▸ 與 reverse_iterator 的關係
//     取最後一個元素還有第三種寫法：*v.rbegin()。
//     rbegin() 內部就是「end() 的反向包裝」，解參考時會先退一格。
//     三種寫法（back()、*prev(end())、*rbegin()）結果相同，
//     其中 back() 最直觀，另外兩種在泛型演算法裡更常見。
//
// 【注意事項 Pay Attention】
//   1. next/prev 不修改傳入的迭代器；要修改請用 advance。
//   2. prev 需要 bidirectional 迭代器；forward_list 上呼叫會編譯失敗。
//   3. 對空容器呼叫 prev(v.end()) 是 UB，使用前先檢查 empty()。
//   4. 越界不做檢查——next(it, 巨大值) 一樣是 UB。
//   5. 在 list/map 上是 O(n)，別在迴圈裡反覆呼叫。
//   6. C++11 才有；C++17 起才是 constexpr。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::next / std::prev
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::next 和 std::advance 有什麼差別？為什麼標準庫兩個都要有？
//     答：advance(it, n) 就地修改 it、回傳 void，是 C++98 的命令式設計；
//         next(it, n) 回傳移動後的新迭代器、原 it 不變，C++11 才加入。
//         有了 next 才能把「移動」寫進運算式裡，
//         例如 v.erase(std::next(it, 2))——用 advance 寫不出來。
//     追問：next 的效能會比 advance 差嗎？
//         → 不會。next 內部就是「複製一份迭代器 → 呼叫 advance → 回傳」，
//           對 vector 這種輕量迭代器，複製成本等同複製一根指標。
//
// 🔥 Q2. 要取得 vector 最後一個元素，有哪些寫法？各有什麼取捨？
//     答：v.back()、*std::prev(v.end())、*v.rbegin()、v[v.size()-1]。
//         v.back() 最直觀，日常首選；
//         *std::prev(v.end()) 在泛型程式碼中最通用（不需要 random access）；
//         v[v.size()-1] 最危險——空容器時 size()-1 因無號回繞成天文數字。
//         四者對空容器都是 UB，必須先檢查 empty()。
//     追問：std::prev(v.end()) 對 forward_list 可以嗎？
//         → 不行，prev 要求 bidirectional iterator，
//           forward_list 只有 forward iterator，編譯期就會被擋。
//
// ⚠️ 陷阱. 已知 v 非空，寫 auto last = std::prev(v.end());
//          然後 v.push_back(x); 再用 *last，為什麼可能出事？
//     答：push_back 若觸發重新配置，整塊緩衝區會搬到新位址，
//         所有既有迭代器（包含 last）全部失效，
//         再解參考它就是 undefined behavior。
//     為什麼會錯：以為 prev/next 產生的是「某個元素的穩定代號」。
//         它其實只是一根指向當前緩衝區的指標；
//         迭代器的有效性取決於容器之後被怎麼操作，
//         和它是用 begin()、prev() 還是 next() 取得的完全無關。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <iterator>  // std::next, std::prev
#include <string>
#include <utility>   // std::swap
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 344. Reverse String
//   題目：原地反轉字元陣列，要求 O(1) 額外空間。
//   為什麼用到本主題：標準解法是「頭尾雙指標往中間夾」，
//         而 std::prev(s.end()) 正是取得「尾指標」最通用的寫法
//         （比 s.end() - 1 更能表達意圖，也不要求 random access）。
//         這裡刻意用迭代器而非索引，示範 next/prev 的實際用途。
// -----------------------------------------------------------------------------
void reverseString(std::vector<char>& s) {
    if (s.empty()) return;                  // 空容器時 prev(end()) 是 UB
    auto left  = s.begin();
    auto right = std::prev(s.end());        // 指向最後一個元素
    while (left < right) {
        std::swap(*left, *right);
        ++left;
        right = std::prev(right);           // 等同 --right，但不改變語意表達
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例】CSV 欄位輸出：最後一欄後面不加逗號
//   情境：把一列資料輸出成 CSV，欄位之間用逗號分隔，
//         但最後一欄後面不能有逗號（否則很多解析器會多讀出一個空欄）。
//   為什麼用本主題：判斷「我是不是最後一個」最乾淨的寫法就是
//         比對 std::next(it) == v.end()，比維護一個 index 計數器好讀得多。
// -----------------------------------------------------------------------------
std::string toCsvLine(const std::vector<std::string>& fields) {
    std::string out;
    for (auto it = fields.begin(); it != fields.end(); ++it) {
        out += *it;
        if (std::next(it) != fields.end()) {   // 不是最後一個才加逗號
            out += ',';
        }
    }
    return out;
}

int main() {
    std::cout << "=== 一、next 回傳新迭代器，原迭代器不變 ===" << std::endl;
    std::vector<int> v = {10, 20, 30, 40, 50};

    auto it = v.begin();
    auto it2 = std::next(it, 3);
    std::cout << "*it  = " << *it  << "（呼叫 next 後仍然不變）" << std::endl;
    std::cout << "*it2 = " << *it2 << std::endl;

    std::cout << "\n=== 二、prev 取得「前面第 n 個」位置 ===" << std::endl;
    auto it3 = std::prev(v.end());
    std::cout << "*prev(end)    = " << *it3 << "（最後一個元素）" << std::endl;
    auto it4 = std::prev(v.end(), 2);
    std::cout << "*prev(end, 2) = " << *it4 << "（倒數第二個）" << std::endl;

    std::cout << "\n=== 三、預設 n = 1 的簡寫 ===" << std::endl;
    std::cout << "*next(begin) = " << *std::next(v.begin()) << std::endl;
    std::cout << "四種取最後元素的寫法：back()=" << v.back()
              << ", *prev(end)=" << *std::prev(v.end())
              << ", *rbegin()=" << *v.rbegin()
              << ", v[size()-1]=" << v[v.size() - 1] << std::endl;

    std::cout << "\n=== 四、LeetCode 344. Reverse String ===" << std::endl;
    std::vector<char> s = {'h', 'e', 'l', 'l', 'o'};
    reverseString(s);
    std::cout << "{'h','e','l','l','o'} 反轉後: ";
    for (char c : s) std::cout << c;
    std::cout << std::endl;

    std::vector<char> s2 = {'H', 'a', 'n', 'n', 'a', 'h'};
    reverseString(s2);
    std::cout << "{'H','a','n','n','a','h'} 反轉後: ";
    for (char c : s2) std::cout << c;
    std::cout << std::endl;

    std::vector<char> s3;                    // 邊界：空容器不可 prev(end())
    reverseString(s3);
    std::cout << "空容器反轉後大小: " << s3.size() << "（已提前 return，未觸發 UB）" << std::endl;

    std::cout << "\n=== 五、日常實務：CSV 最後一欄不加逗號 ===" << std::endl;
    std::cout << "[" << toCsvLine({"id", "name", "email", "created_at"}) << "]" << std::endl;
    std::cout << "[" << toCsvLine({"單欄"}) << "]" << std::endl;
    std::cout << "[" << toCsvLine({}) << "]（空清單）" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 16 課：vector 的迭代器操作12.cpp" -o next_prev

// === 預期輸出 ===
// === 一、next 回傳新迭代器，原迭代器不變 ===
// *it  = 10（呼叫 next 後仍然不變）
// *it2 = 40
//
// === 二、prev 取得「前面第 n 個」位置 ===
// *prev(end)    = 50（最後一個元素）
// *prev(end, 2) = 40（倒數第二個）
//
// === 三、預設 n = 1 的簡寫 ===
// *next(begin) = 20
// 四種取最後元素的寫法：back()=50, *prev(end)=50, *rbegin()=50, v[size()-1]=50
//
// === 四、LeetCode 344. Reverse String ===
// {'h','e','l','l','o'} 反轉後: olleh
// {'H','a','n','n','a','h'} 反轉後: hannaH
// 空容器反轉後大小: 0（已提前 return，未觸發 UB）
//
// === 五、日常實務：CSV 最後一欄不加逗號 ===
// [id,name,email,created_at]
// [單欄]
// []（空清單）
