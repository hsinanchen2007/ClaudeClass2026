// =============================================================================
//  第四課：迭代器（Iterator）的核心概念 2  —  迭代器的四個基本操作
// =============================================================================
//
// 【主題資訊 Information】
//   核心操作（Input Iterator 起就有）：
//     *it            解參考，取得所指元素
//     ++it           前置遞增，回傳 iterator&
//     it++           後置遞增，回傳 iterator（舊值的副本）
//     it1 == it2 / it1 != it2   位置比較
//   型別寫法：
//     std::vector<int>::iterator it = vec.begin();   // C++98 完整寫法
//     auto it = vec.begin();                          // C++11 起
//   標準版本：以上全部是 C++98；auto 推導是 C++11。
//   複雜度：四個操作對所有標準容器都是 O(1)（攤銷）。
//   標頭檔：<vector>（容器自身）；泛用操作在 <iterator>。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼只需要這四個操作就夠了】
//   STL 演算法絕大多數只依賴「取值、前進、比較是否到終點」。
//   把需求壓到最小，換來的是最大的適用範圍：一個只支援這四個操作的
//   迭代器（Input Iterator）就能跑 std::find、std::count、std::for_each。
//   這是介面設計上的「最小充分集」原則——要求得越少，能接的型別越多。
//   例如 std::istream_iterator（從輸入串流讀資料）根本無法倒退、無法比大小，
//   但它照樣能用 std::copy 把整個 stdin 讀進 vector。
//
// 【2. ++it 與 it++ 的真正差別（不是風格問題）】
//   兩者的語意規定是：
//     前置 ++it：先前進，回傳「前進後的自己」的參考 → iterator&
//     後置 it++：先把自己複製一份保存，再前進，回傳「前進前的副本」→ iterator
//   後置版本的典型實作長這樣：
//       iterator operator++(int) {
//           iterator tmp = *this;   // ← 多出來的複製
//           ++(*this);
//           return tmp;             // ← 回傳的是舊值副本
//       }
//   對 int* 來說這個複製是免費的（編譯器最佳化掉）。但對
//   std::map::iterator 這種內部持有節點指標、甚至更肥的自訂迭代器，
//   每次迴圈都白白複製一份。**所以慣例一律寫 `++it`**——不是因為它比較快
//   多少，而是因為它「不可能比較慢」，且表達了「我不需要舊值」的意圖。
//
// 【3. `*it++` 是怎麼被解析的】
//   後置 ++ 的優先序高於 *，所以 `*it++` 等價於 `*(it++)`：
//     1) it++ 回傳「前進前的副本」（指向舊位置）
//     2) it 本身已經前進了
//     3) * 作用在那份舊副本上 → 取到的是**舊位置**的值
//   這就是本檔輸出「*it++ = 20」之後「現在 *it = 30」的原因：
//   印出來的是舊位置的 20，而 it 已經停在 30。
//   這個慣用法在手寫串流讀取時很常見（`*out++ = value;`），
//   但混在複雜運算式裡極易誤讀，一般程式碼建議拆成兩行。
//
// 【4. 為什麼比較只保證 == 和 !=】
//   `<`、`>` 只有 Random Access Iterator 才有。原因是「順序」對非連續
//   結構沒有意義：std::list 的第 3 個節點在記憶體位址上可能比第 1 個小。
//   而 `==` / `!=` 對任何迭代器都有定義——它們比的是「是否指向同一位置」，
//   這在任何資料結構上都成立。
//   實務結論：迴圈終止條件永遠寫 `it != end`，不要寫 `it < end`。
//
// 【概念補充 Concept Deep Dive】
//   迭代器是**值語意（value semantics）**的物件，不是參考。
//     auto a = vec.begin();
//     auto b = a;      // b 是獨立副本
//     ++b;             // a 不受影響
//   本機 libstdc++ 的 vector<int>::iterator 內部就只有一個 int* 成員，
//   sizeof 等同指標大小（**實作定義**，本檔會實際印出來驗證）。
//   因為它這麼小又可平凡複製，**慣例是傳值（by value），不要傳 const&**。
//   相對地，std::map::iterator 內含紅黑樹節點指標，++ 要做中序後繼走訪；
//   std::deque::iterator 更肥，通常含 4 個指標（目前元素、目前區塊首、
//   區塊尾、區塊表位置），這也是為什麼 deque 的迭代器遞增遠比 vector 貴。
//
// 【注意事項 Pay Attention】
//   1. `*vec.end()` 是 UB。end() 是「最後一個元素的下一個位置」，
//      上面沒有物件。UB 不保證特定行為，不要期待它一定崩潰或一定印出垃圾值。
//   2. 空容器的 begin() == end()，所以 `*vec.begin()` 對空容器同樣是 UB。
//      用 `vec.front()` 前一定要先確認 `!vec.empty()`。
//   3. `it++` 在迴圈裡不是錯誤，只是可能多一次複製；但把 `it++` 與其他
//      對 it 的存取寫在同一個運算式裡（如 `*it = *it++;`）就會踩到
//      求值順序問題，C++17 之前甚至是 UB。
//   4. 這裡把區域變數命名為 begin / end 會遮蔽 std::begin / std::end。
//      本檔沿用原教材命名以維持對照，實務上建議改名為 first / last。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】迭代器基本操作
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 STL 慣例一律寫 ++it 而不是 it++？
//     答：後置遞增必須先複製一份自身、前進、再回傳舊副本，因此比前置版本
//         多一次複製與一次建構。對 int* 這種平凡型別編譯器會最佳化掉，
//         但對 map/deque 這類肥迭代器就是實際成本。寫 ++it 永遠不會比較慢，
//         而且表達了「我不需要舊值」的意圖。
//     追問：那什麼時候真的需要 it++？
//         → 需要「取舊值同時前進」的場合，典型是輸出迭代器慣用法
//           `*out++ = value;`，或 erase 舊式寫法 `container.erase(it++);`
//           （對 map/list 有效：先算出舊迭代器傳給 erase，it 已安全移到下一個）。
//
// 🔥 Q2. `*it++` 取到的是哪一個元素的值？
//     答：舊位置的值。後置 ++ 優先序高於 *，所以等價於 `*(it++)`；
//         it++ 回傳前進「之前」的副本，* 作用在該副本上。副作用是 it
//         本身已經指到下一個元素。本檔實測：*it++ 印出 20，之後 *it 是 30。
//     追問：`*++it` 呢？→ 前置版本先前進再回傳自己，所以取到的是新位置的值。
//
// ⚠️ 陷阱. 迴圈條件寫成 `for (auto it = v.begin(); it < v.end(); ++it)`
//         對 vector 能編過，為什麼仍然是壞習慣？
//     答：`<` 只有 Random Access Iterator 才提供。這段程式碼對 vector
//         能編譯，但只要把容器換成 std::list 或 std::map 就會編譯失敗，
//         因為它們的迭代器沒有 operator<。
//     為什麼會錯：把迭代器想成「索引」或「位址」，於是覺得比大小理所當然。
//         但迭代器的通用契約只保證 == 和 !=——比較的是「是否同一位置」，
//         不是「誰在前面」。對鏈結串列而言，節點的記憶體位址順序與邏輯
//         順序完全無關，`<` 根本無法定義。正確寫法永遠是 `it != v.end()`。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>

int main() {
    std::vector<int> vec = {10, 20, 30, 40, 50};

    std::cout << "=== 迭代器基本操作 ===" << std::endl;

    // 取得起始迭代器（C++98 完整型別寫法，用來對照 auto）
    std::vector<int>::iterator it = vec.begin();

    // *it：解參考
    std::cout << "*it = " << *it << std::endl;

    // ++it：前置前進，回傳 iterator&
    ++it;
    std::cout << "++it 後，*it = " << *it << std::endl;

    // it++：後置前進 —— 等價於 *(it++)，取到的是「前進前」的舊值
    std::cout << "*it++ = " << *it++ << std::endl;  // 先取值，再前進
    std::cout << "現在 *it = " << *it << std::endl; // it 已經走到下一個

    // 比較
    std::cout << "\n=== 迭代器比較 ===" << std::endl;
    auto begin = vec.begin();
    auto end = vec.end();

    std::cout << "begin == end? " << (begin == end ? "是" : "否") << std::endl;
    std::cout << "begin != end? " << (begin != end ? "是" : "否") << std::endl;

    // 迭代器是值語意：複製出來的副本彼此獨立
    std::cout << "\n=== 迭代器是值語意（value semantics）===" << std::endl;
    auto a = vec.begin();
    auto b = a;      // 獨立副本
    ++b;             // 只動 b
    std::cout << "*a = " << *a << "（a 不受 ++b 影響）" << std::endl;
    std::cout << "*b = " << *b << std::endl;

    // 迭代器有多大？（實作定義，本機 libstdc++ 內部只包一個指標）
    std::cout << "\n=== 迭代器大小（實作定義）===" << std::endl;
    std::cout << "sizeof(vector<int>::iterator) = " << sizeof(decltype(vec.begin()))
              << " bytes" << std::endl;
    std::cout << "sizeof(int*)                  = " << sizeof(int*)
              << " bytes（本機兩者相同：內部就是包一個指標）" << std::endl;

    // 空容器：begin() 就等於 end()
    std::cout << "\n=== 空容器 ===" << std::endl;
    std::vector<int> empty_vec;
    std::cout << "空容器 begin == end? "
              << (empty_vec.begin() == empty_vec.end() ? "是" : "否")
              << "（所以 *empty_vec.begin() 是 UB）" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第四課：迭代器（Iterator）的核心概念2.cpp" -o iter2

// === 預期輸出 ===
// === 迭代器基本操作 ===
// *it = 10
// ++it 後，*it = 20
// *it++ = 20
// 現在 *it = 30
//
// === 迭代器比較 ===
// begin == end? 否
// begin != end? 是
//
// === 迭代器是值語意（value semantics）===
// *a = 10（a 不受 ++b 影響）
// *b = 20
//
// === 迭代器大小（實作定義）===
// sizeof(vector<int>::iterator) = 8 bytes
// sizeof(int*)                  = 8 bytes（本機兩者相同：內部就是包一個指標）
//
// === 空容器 ===
// 空容器 begin == end? 是（所以 *empty_vec.begin() 是 UB）
