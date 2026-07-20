// =============================================================================
//  第五課：迭代器的五種分類 10  —  iterator_traits：在編譯期查出迭代器類別
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<iterator>（iterator_traits 與五個 tag）、<type_traits>（is_same_v）
//   核心設施：
//     template <class It> struct iterator_traits {
//         using difference_type   = typename It::difference_type;
//         using value_type        = typename It::value_type;
//         using pointer           = typename It::pointer;
//         using reference         = typename It::reference;
//         using iterator_category = typename It::iterator_category;
//     };
//     // 另有針對 T* 與 const T* 的特化，讓原生指標也能被查詢
//   五個 tag 型別（**它們之間有繼承關係**）：
//     struct input_iterator_tag {};
//     struct output_iterator_tag {};
//     struct forward_iterator_tag       : input_iterator_tag {};
//     struct bidirectional_iterator_tag : forward_iterator_tag {};
//     struct random_access_iterator_tag : bidirectional_iterator_tag {};
//     // C++20 新增：struct contiguous_iterator_tag : random_access_iterator_tag {};
//   標準版本：iterator_traits 自 C++98；is_same_v 是 C++17（is_same 是 C++11）；
//             **if constexpr 是 C++17**（本檔用到，故必須以 -std=c++17 以上編譯）。
//   複雜度：全部在編譯期完成，執行期零成本。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼需要 iterator_traits 這一層間接】
//   如果只想拿到「這個迭代器指向什麼型別」，直覺寫法是：
//       typename It::value_type
//   但這對**原生指標**完全行不通 —— int* 沒有巢狀型別 int*::value_type。
//   而原生指標偏偏是最重要的迭代器之一（C 陣列、vector 的底層）。
//   iterator_traits 用「特化」解決了這個問題：
//       template <class T> struct iterator_traits<T*> {
//           using value_type        = T;
//           using iterator_category = random_access_iterator_tag;
//           ...
//       };
//   於是泛型程式碼一律寫 `typename std::iterator_traits<It>::value_type`，
//   對類別型迭代器與原生指標都成立。
//   這是「**加一層間接解決所有問題**」這句名言在 STL 中的教科書案例。
//
// 【2. tag 的繼承關係才是關鍵（而不是那五個空類別本身）】
//   注意 forward_iterator_tag **繼承自** input_iterator_tag，依此類推。
//   這個繼承鏈讓「多載解析」自動實現了「能力足夠即可」的語意：
//       void impl(It, distance, random_access_iterator_tag);   // O(1) 版
//       void impl(It, distance, input_iterator_tag);           // O(n) 版
//       // 呼叫 impl(it, n, forward_iterator_tag{})
//       // → 沒有精確匹配，但 forward 繼承自 input → 選中 O(n) 版
//   若五個 tag 是彼此無關的空類別，就得為每一種各寫一個多載。
//   有了繼承，只要為「有實質差異的層級」提供實作即可。
//   這正是 std::advance / std::distance 的實作方式（見本課 summary.cpp §9）。
//
// 【3. 標籤分派 vs if constexpr：兩個時代的寫法】
//   本檔用的是 C++17 的 if constexpr：
//       if constexpr (std::is_same_v<category, std::random_access_iterator_tag>) { ... }
//   優點是全部寫在一個函式裡、好讀；**未被選中的分支不會被實例化**，
//   所以可以放「對其他分類編不過」的程式碼。
//   C++17 之前只能用「標籤分派（tag dispatch）」——
//   靠多載解析選擇，也就是上面 §2 的寫法。
//   兩者都是編譯期決策、執行期零成本；差別只在可讀性與相容性。
//   （注意：if constexpr 用 is_same_v 做**精確比對**時，
//     會失去 tag 繼承帶來的「能力足夠即可」語意 ——
//     所以本檔的判斷必須把五種都列出來，不能只寫兩個分支。）
//
// 【4. 為什麼 vector<int>::iterator 不是 contiguous_iterator_tag】
//   本檔在 C++17 下查詢 vector 的迭代器，得到的是 random_access_iterator_tag。
//   contiguous_iterator_tag 是 **C++20** 才加入的，而且即使在 C++20 下，
//   libstdc++ 的 iterator_traits<vector<int>::iterator>::iterator_category
//   仍然回報 random_access_iterator_tag（為了 ABI 相容）——
//   要判斷「是否連續」必須改用 C++20 的 concept `std::contiguous_iterator<It>`。
//   這是「舊的 tag 系統」與「新的 concept 系統」並存的實際後果。
//
// 【概念補充 Concept Deep Dive】
//   為什麼 std::advance(it, n) 對 vector 是 O(1)、對 list 是 O(n)，
//   而使用者完全不必知道？
//   libstdc++ 的實作大致是：
//       template <class It, class Dist>
//       void advance(It& it, Dist n) {
//           __advance(it, n, typename iterator_traits<It>::iterator_category());
//       }
//       // 三個多載：
//       __advance(It& it, Dist n, random_access_iterator_tag) { it += n; }           // O(1)
//       __advance(It& it, Dist n, bidirectional_iterator_tag) { n>0? ++ : --; }      // O(n)，可負
//       __advance(It& it, Dist n, input_iterator_tag)         { while (n--) ++it; }  // O(n)，n≥0
//   關鍵在最後一個參數：它是一個**臨時建構的空 tag 物件**，
//   唯一的用途是讓編譯器據以選擇多載。
//   由於 tag 是空類別、且整個決策在編譯期完成，
//   最佳化後這個參數會完全消失 —— 執行期沒有任何額外成本。
//   這個「用型別攜帶編譯期資訊」的手法在現代 C++ 中無所不在
//   （std::true_type / false_type、std::in_place_t、std::nothrow_t 都是同一招）。
//
// 【注意事項 Pay Attention】
//   1. 自訂迭代器若沒有提供五個 typedef，iterator_traits 查不到 →
//      無法用於任何 STL 演算法（範圍 for 則不受影響）。
//   2. 舊式寫法「繼承 std::iterator<...>」在 **C++17 已 deprecated**，
//      請直接在類別內宣告五個 using。
//   3. if constexpr 是 C++17；本檔必須用 -std=c++17 以上編譯。
//   4. 用 is_same_v 做精確比對會失去 tag 繼承的「能力足夠即可」語意，
//      必須列舉全部分類；若要「至少是某等級」請改用 std::is_base_of_v。
//   5. contiguous_iterator_tag 是 C++20；且 libstdc++ 為了 ABI 相容，
//      vector 迭代器的 category 仍回報 random_access_iterator_tag。
//   6. output_iterator_tag **不在**繼承鏈上（它與 input 並列），
//      判斷時要單獨處理。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】iterator_traits 與標籤分派
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼需要 std::iterator_traits？直接寫 It::value_type 不行嗎？
//     答：不行，因為**原生指標沒有巢狀型別** —— int*::value_type 不存在，
//         而指標是最重要的迭代器之一（C 陣列、vector 底層）。
//         iterator_traits 提供了針對 T* 與 const T* 的特化，
//         讓泛型程式碼統一寫 iterator_traits<It>::value_type 就能同時支援兩者。
//         這是「多加一層間接」解決問題的經典案例。
//     追問：那自訂迭代器要怎麼讓 iterator_traits 查得到？
//           → 在類別內宣告五個 using：iterator_category、value_type、
//             difference_type、pointer、reference。
//             舊式的「繼承 std::iterator<>」在 C++17 已 deprecated，別再用。
//
// 🔥 Q2. std::advance 對 vector 是 O(1)、對 list 是 O(n)，它怎麼知道該用哪個？
//     答：標籤分派（tag dispatch）。advance 內部把
//         `typename iterator_traits<It>::iterator_category()` 這個**空 tag 物件**
//         當額外參數傳給內部多載，由編譯器在編譯期選中對應版本：
//         random_access 版用 `it += n`，input/bidirectional 版用迴圈。
//         tag 是空類別、決策在編譯期完成，最佳化後執行期零成本。
//     追問：為什麼五個 tag 之間要有繼承關係？
//           → 讓多載解析自動實現「能力足夠即可」。
//             forward_iterator_tag 繼承 input_iterator_tag，
//             所以傳 forward 時若沒有精確匹配，會自動退而選中 input 版。
//             這樣只需為「有實質差異」的層級寫實作，不必五種都寫。
//
// ⚠️ 陷阱. 想寫一個「只接受 Random Access 以上」的泛型函式，
//          於是寫了 static_assert(std::is_same_v<cat, std::random_access_iterator_tag>)
//          —— 這樣有什麼問題？
//     答：is_same_v 是**精確比對**，會拒絕所有「比 Random Access 更強」的分類。
//         在 C++20 下，若某個迭代器的 category 是 contiguous_iterator_tag
//         （它繼承自 random_access_iterator_tag，能力只多不少），
//         這個 static_assert 會失敗 —— 明明完全符合需求卻被擋下。
//         正確寫法是用 is_base_of_v（利用 tag 的繼承關係）：
//             static_assert(std::is_base_of_v<std::random_access_iterator_tag, cat>);
//         或在 C++20 直接用 concept：`requires std::random_access_iterator<It>`。
//     為什麼會錯：忽略了「tag 之間有繼承關係」這個設計重點。
//         tag 不只是五個標記，它們構成一個**能力的偏序**；
//         判斷「夠不夠強」要用 is_base_of（是否為其祖先），而不是 is_same。
//         本檔為了「印出精確分類名稱」而使用 is_same_v，那是正確的用途 ——
//         但拿它做「能力檢查」就錯了。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <iterator>
#include <vector>
#include <list>
#include <forward_list>
#include <set>
#include <deque>
#include <string>
#include <type_traits>

// 查詢迭代器類別
template <typename Iterator>
void print_iterator_category(const std::string& name) {
    using category = typename std::iterator_traits<Iterator>::iterator_category;

    std::cout << name << " 的迭代器類別: ";

    if constexpr (std::is_same_v<category, std::input_iterator_tag>) {
        std::cout << "Input Iterator";
    } else if constexpr (std::is_same_v<category, std::output_iterator_tag>) {
        std::cout << "Output Iterator";
    } else if constexpr (std::is_same_v<category, std::forward_iterator_tag>) {
        std::cout << "Forward Iterator";
    } else if constexpr (std::is_same_v<category, std::bidirectional_iterator_tag>) {
        std::cout << "Bidirectional Iterator";
    } else if constexpr (std::is_same_v<category, std::random_access_iterator_tag>) {
        std::cout << "Random Access Iterator";
    }

    std::cout << std::endl;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】不強加。
//   理由：iterator_traits 是**寫泛型函式庫**時才會用到的設施 ——
//         它解決的是「我的演算法要同時支援 vector、list 與原生指標」這種問題。
//         LeetCode 的輸入型別是固定的（幾乎總是 vector 或 string），
//         寫解法時完全不需要在編譯期查詢迭代器分類；
//         真那樣寫只會讓程式碼變複雜而毫無收穫。
//         這個技能真正的用途是下面實務範例展示的「寫一個對所有容器都最佳的工具函式」。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】寫一個「對每種容器都用最佳策略」的工具函式
//   情境：團隊需要一個 middleElement(first, last) 取中間元素的工具，
//         要能吃 vector / deque / list / set / 原生陣列，
//         而且對每種容器都要用**該容器能做到的最快方式**。
//   為什麼用到本主題：這正是 iterator_traits 的天職 ——
//         在編譯期查出迭代器分類，選擇不同實作：
//           Random Access → first + (last - first) / 2      O(1)
//           其他          → 先 distance 再 advance          O(n)
//         呼叫端完全不必知道這件事，寫法一模一樣。
//         這也順帶示範標籤分派與 if constexpr 兩種寫法的差異。
// -----------------------------------------------------------------------------

// --- 寫法 A：標籤分派（C++98 起可用，STL 內部就是這樣寫的）---
template <typename It>
It middleImpl(It first, It last, std::random_access_iterator_tag) {
    std::cout << "      [策略] Random Access → 直接算位址，O(1)" << std::endl;
    return first + (last - first) / 2;
}

template <typename It>
It middleImpl(It first, It last, std::input_iterator_tag) {
    std::cout << "      [策略] 非 Random Access → distance + advance，O(n)" << std::endl;
    auto n = std::distance(first, last);
    std::advance(first, n / 2);
    return first;
}

template <typename It>
It middleByTagDispatch(It first, It last) {
    return middleImpl(first, last,
                      typename std::iterator_traits<It>::iterator_category());
}

// --- 寫法 B：if constexpr（C++17，同一個函式內決策）---
template <typename It>
It middleByIfConstexpr(It first, It last) {
    using cat = typename std::iterator_traits<It>::iterator_category;
    // 注意這裡用 is_base_of_v 而非 is_same_v ——
    // 「至少是 Random Access」才是我們要的語意（含未來更強的分類）
    if constexpr (std::is_base_of_v<std::random_access_iterator_tag, cat>) {
        return first + (last - first) / 2;
    } else {
        auto n = std::distance(first, last);
        std::advance(first, n / 2);
        return first;
    }
}

int main() {
    print_iterator_category<std::vector<int>::iterator>("vector");
    print_iterator_category<std::list<int>::iterator>("list");
    print_iterator_category<std::forward_list<int>::iterator>("forward_list");
    print_iterator_category<int*>("原生指標");
    print_iterator_category<std::istream_iterator<int>>("istream_iterator");
    print_iterator_category<std::ostream_iterator<int>>("ostream_iterator");

    // 補齊其餘常見容器
    std::cout << "\n=== 其他容器 ===" << std::endl;
    print_iterator_category<std::deque<int>::iterator>("deque");
    print_iterator_category<std::set<int>::iterator>("set");
    print_iterator_category<std::string::iterator>("string");
    print_iterator_category<const int*>("const 原生指標");

    // iterator_traits 也能查出 value_type
    std::cout << "\n=== iterator_traits 查得到的不只 category ===" << std::endl;
    std::cout << "  vector<int>::iterator 的 value_type 是 int? "
              << (std::is_same_v<
                      std::iterator_traits<std::vector<int>::iterator>::value_type,
                      int> ? "是" : "否") << std::endl;
    std::cout << "  int* 的 value_type 是 int? "
              << (std::is_same_v<std::iterator_traits<int*>::value_type, int>
                  ? "是" : "否")
              << "   ← 靠 T* 的特化才辦得到" << std::endl;

    // tag 之間的繼承關係
    std::cout << "\n=== tag 的繼承關係（能力的偏序）===" << std::endl;
    std::cout << "  forward       繼承自 input?         "
              << (std::is_base_of_v<std::input_iterator_tag,
                                    std::forward_iterator_tag> ? "是" : "否") << std::endl;
    std::cout << "  bidirectional 繼承自 forward?       "
              << (std::is_base_of_v<std::forward_iterator_tag,
                                    std::bidirectional_iterator_tag> ? "是" : "否") << std::endl;
    std::cout << "  random_access 繼承自 bidirectional? "
              << (std::is_base_of_v<std::bidirectional_iterator_tag,
                                    std::random_access_iterator_tag> ? "是" : "否") << std::endl;
    std::cout << "  output        繼承自 input?         "
              << (std::is_base_of_v<std::input_iterator_tag,
                                    std::output_iterator_tag> ? "是" : "否")
              << "  ← Output 與 Input 是並列的，不在同一條鏈上" << std::endl;

    // 「至少是 Random Access」該用 is_base_of 而非 is_same
    std::cout << "\n=== 能力檢查要用 is_base_of，不是 is_same ===" << std::endl;
    {
        using vec_cat = std::iterator_traits<std::vector<int>::iterator>::iterator_category;
        std::cout << "  vector 的 category 精確等於 random_access_iterator_tag? "
                  << (std::is_same_v<vec_cat, std::random_access_iterator_tag>
                      ? "是" : "否") << std::endl;
        std::cout << "  vector 的 category「至少是」random_access? "
                  << (std::is_base_of_v<std::random_access_iterator_tag, vec_cat>
                      ? "是" : "否") << std::endl;
        std::cout << "  → 目前兩者都成立；但若未來換成能力更強的分類"
                     "（如 C++20 contiguous），" << std::endl;
        std::cout << "    只有 is_base_of 的寫法仍然正確" << std::endl;
    }

    std::cout << "\n=== 日常實務：對每種容器都選最佳策略 ===" << std::endl;
    {
        std::vector<int> v = {10, 20, 30, 40, 50};
        std::list<int>   l = {10, 20, 30, 40, 50};
        int arr[] = {10, 20, 30, 40, 50};

        // 注意：先呼叫再印，否則函式內部的 [策略] 訊息會插進 << 串鏈中間
        std::cout << "  [標籤分派] vector:" << std::endl;
        auto mv = middleByTagDispatch(v.begin(), v.end());
        std::cout << "      中間元素 = " << *mv << std::endl;

        std::cout << "  [標籤分派] list:" << std::endl;
        auto ml = middleByTagDispatch(l.begin(), l.end());
        std::cout << "      中間元素 = " << *ml << std::endl;

        std::cout << "  [標籤分派] 原生陣列:" << std::endl;
        auto ma = middleByTagDispatch(std::begin(arr), std::end(arr));
        std::cout << "      中間元素 = " << *ma << std::endl;

        std::cout << "  [if constexpr 版本] 三種容器結果相同: "
                  << *middleByIfConstexpr(v.begin(), v.end()) << " "
                  << *middleByIfConstexpr(l.begin(), l.end()) << " "
                  << *middleByIfConstexpr(std::begin(arr), std::end(arr)) << std::endl;
        std::cout << "  → 呼叫端寫法完全一樣，最佳策略在編譯期就選好了"
                  << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第五課：迭代器的五種分類10.cpp -o demo10

// === 預期輸出 ===
// vector 的迭代器類別: Random Access Iterator
// list 的迭代器類別: Bidirectional Iterator
// forward_list 的迭代器類別: Forward Iterator
// 原生指標 的迭代器類別: Random Access Iterator
// istream_iterator 的迭代器類別: Input Iterator
// ostream_iterator 的迭代器類別: Output Iterator
//
// === 其他容器 ===
// deque 的迭代器類別: Random Access Iterator
// set 的迭代器類別: Bidirectional Iterator
// string 的迭代器類別: Random Access Iterator
// const 原生指標 的迭代器類別: Random Access Iterator
//
// === iterator_traits 查得到的不只 category ===
//   vector<int>::iterator 的 value_type 是 int? 是
//   int* 的 value_type 是 int? 是   ← 靠 T* 的特化才辦得到
//
// === tag 的繼承關係（能力的偏序）===
//   forward       繼承自 input?         是
//   bidirectional 繼承自 forward?       是
//   random_access 繼承自 bidirectional? 是
//   output        繼承自 input?         否  ← Output 與 Input 是並列的，不在同一條鏈上
//
// === 能力檢查要用 is_base_of，不是 is_same ===
//   vector 的 category 精確等於 random_access_iterator_tag? 是
//   vector 的 category「至少是」random_access? 是
//   → 目前兩者都成立；但若未來換成能力更強的分類（如 C++20 contiguous），
//     只有 is_base_of 的寫法仍然正確
//
// === 日常實務：對每種容器都選最佳策略 ===
//   [標籤分派] vector:
//       [策略] Random Access → 直接算位址，O(1)
//       中間元素 = 30
//   [標籤分派] list:
//       [策略] 非 Random Access → distance + advance，O(n)
//       中間元素 = 30
//   [標籤分派] 原生陣列:
//       [策略] Random Access → 直接算位址，O(1)
//       中間元素 = 30
//   [if constexpr 版本] 三種容器結果相同: 30 30 30
//   → 呼叫端寫法完全一樣，最佳策略在編譯期就選好了
