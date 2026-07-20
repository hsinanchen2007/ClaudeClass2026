// =============================================================================
//  第二課：泛型編程（Generic Programming）概念9.cpp
//   —  讓自訂型別滿足隱含介面：運算子多載與「非侵入式擴充」
// =============================================================================
//
// 【主題資訊 Information】
//   語法：
//     bool operator>(const Point& other) const;            // 成員函式形式
//     std::ostream& operator<<(std::ostream&, const Point&); // 自由函式形式
//
//   標準版本：運算子多載 C++98 起
//             （C++20 另有三向比較運算子 <=>，見【概念補充 D】）
//   標頭檔  ：<iostream>
//
//   本檔是概念8.cpp 的解答：為 Point 補上 find_max 所需的 operator>，
//   讓它得以通過模板的隱含介面檢查。
//
// 【詳細解釋 Explanation】
//
// 【1. 泛型的關鍵優勢：不必修改 find_max】
// 請注意本檔做了什麼、又沒做什麼：
//   * 做了：為 Point 加上 operator>
//   * **沒做**：碰 find_max 一根寒毛
//
// 這就是泛型編程與物件導向繼承的根本差異。若用 OO 的做法，
// 要讓 Point 能被某個演算法處理，通常得讓 Point 去繼承某個 Comparable 基底類別
// —— 那是**侵入式**（intrusive）的：你必須修改型別的定義，而且如果 Point 來自
// 第三方函式庫，你根本改不了。
//
// 泛型走的是另一條路：演算法只要求「會做某件事」。你只要讓型別會做那件事
// （提供 operator>），演算法就自動接受它。這叫**非侵入式擴充**：
//   * 對已存在的型別（甚至第三方型別），可以用**自由函式**形式的運算子多載
//     從外部補上能力，完全不必修改該型別的定義。
//   * 這正是 STL 能排序你自訂 struct 的原因 —— std::sort 寫成的那天，
//     你的型別還不存在。
//
// 【2. 成員函式 vs 自由函式：為什麼 operator<< 一定要是自由函式】
// 本檔兩個運算子刻意用了不同形式，這不是隨意選的：
//
//   operator>  寫成**成員函式**：
//       bool operator>(const Point& other) const;
//     呼叫 a > b 實際上是 a.operator>(b)，左運算元必須是 Point。
//
//   operator<< 寫成**自由函式**：
//       std::ostream& operator<<(std::ostream& os, const Point& p);
//     因為 `std::cout << p` 的左運算元是 std::ostream，不是 Point。
//     要寫成成員函式，就得把它加進 std::ostream 的定義裡 —— 那是標準庫的
//     類別，你不能修改。所以只能寫成自由函式。
//
//   一般準則：
//     * 左運算元必定是自己的型別（如 +=、[]、->）→ 成員函式
//     * 左運算元可能是別的型別（如 <<、>>）→ 必須自由函式
//     * 對稱的二元運算子（如 ==、<、+）→ **建議**自由函式，
//       因為成員函式版本不允許左運算元發生隱式轉換，會造成不對稱：
//       若 Point 能從 int 隱式建構，`p > 5` 可行但 `5 > p` 就不行。
//
// 【3. 這裡的 operator> 定義了什麼語意？】
// 本檔用「距離原點的平方和」比較：(1,2) 的值是 1+4=5，(3,4) 是 9+16=25，
// 所以 p2 > p1。刻意用平方和而不開根號，是為了避免浮點運算與精度問題 ——
// 比較大小時單調性已經足夠，這是幾何運算中常用的最佳化技巧。
//
// 但要特別注意：這個定義**不是全序**。(1,2) 與 (2,1) 的平方和都是 5，
// 兩者互不大於，卻也不相等。這種「不可比較但不相等」的關係
// 叫做 strict weak ordering（嚴格弱序），STL 的排序容器正是要求這個
// 而非全序。所以它其實可以正常用在 std::sort 上，只是相同平方和的元素
// 之間順序不保證穩定（要穩定得用 std::stable_sort）。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼回傳 std::ostream& 而不是 void
//   為了支援串接：`std::cout << p1 << p2 << std::endl;` 會被解析成
//   `((std::cout << p1) << p2) << std::endl`。每次呼叫都必須把那個
//   stream 再交出去，下一個 << 才有東西可用。回傳 void 就只能寫一個。
//
// (B) 為什麼參數是 const Point& 而非 Point
//   避免不必要的複製，同時允許傳入 const 物件與暫時物件。
//   對應地，operator> 宣告尾端的 const 表示「這個成員函式不修改自己」，
//   少了它，`const Point a; a > b;` 會編譯失敗。
//   泛型程式碼常大量使用 const 物件，漏了 const 會讓型別在很多場合突然不可用。
//
// (C) ADL：為什麼自由函式版的 operator<< 找得到
//   `std::cout << p` 時，編譯器除了在當前作用域找 operator<<，還會到
//   **引數所屬的 namespace** 去找，這叫 ADL（Argument-Dependent Lookup，
//   引數相依查找）。本檔的 Point 在全域 namespace，operator<< 也在全域，
//   所以找得到。若把 Point 放進 namespace geo，那 operator<< 也應該放進
//   同一個 namespace —— 這是運算子多載的標準做法，也是為什麼
//   「自訂型別的 swap 要放在同一個 namespace」那條慣例會成立。
//
// (D) C++20 的三向比較運算子 <=>（spaceship operator）
//   本檔只定義了 operator>。若還需要 <、<=、>=、==，C++98 的做法是逐一手寫
//   六個運算子（且必須保持彼此邏輯一致，很容易寫錯）。C++20 起可以寫：
//       auto operator<=>(const Point&) const = default;
//   編譯器就會自動生成全部比較運算子。注意 = default 版本是**逐成員字典序**
//   比較（先比 x 再比 y），與本檔「平方和」的語意不同；要自訂語意仍需手寫
//   <=> 的本體。此語法需 C++20，本檔維持 C++17 相容故未使用。
//
// 【注意事項 Pay Attention】
// 1. 只定義 operator> 並不會自動獲得 <、<=、>=（C++20 的 <=> 才有此能力）。
//    find_max 剛好只用到 >，所以本檔夠用；但若拿 Point 去餵 std::sort，
//    會失敗 —— 因為 std::sort 預設要求的是 **operator<**。
//    這再次說明「隱含介面」的要求是逐個演算法各自不同的。
// 2. 運算子多載應該符合直覺。把 operator> 定義成「回傳 x 座標較小者」
//    能編譯，但任何讀你程式碼的人（包括三個月後的你）都會被誤導。
// 3. 成員函式形式的比較運算子不允許左運算元隱式轉換，會導致 `5 > p` 與
//    `p > 5` 行為不一致。對稱運算子優先考慮自由函式形式。
// 4. 平方和比較是 strict weak ordering 而非全序：(1,2) 與 (2,1) 互不大於
//    卻也不相等。用於排序沒問題，但別誤以為「不大於就等於相等」。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】運算子多載與非侵入式擴充
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為 Point 加上 operator> 之後 find_max 就能用了 ——
//        這件事說明泛型編程和 OO 繼承有什麼根本差異？
//     答：泛型是**非侵入式**的。演算法只要求「會做某件事」，型別提供該能力
//         即可被接受，且 find_max 完全不必修改。OO 的做法通常要讓型別去繼承
//         某個介面基底類別，那是侵入式的 —— 若型別來自第三方函式庫，你根本
//         改不了。這正是 STL 能排序「作者當初不知道其存在」之型別的原因。
//     追問：完全無法修改的第三方型別怎麼辦？
//         → 用**自由函式**形式的運算子多載從外部補上，不必碰該型別的定義；
//           或在呼叫演算法時傳入自訂比較器（例如 std::sort(b, e, cmp)）。
//
// 🔥 Q2. 為什麼 operator<< 必須寫成自由函式，operator> 卻可以是成員函式？
//     答：因為運算子的**左運算元**決定了它能否是成員函式。
//         `std::cout << p` 的左運算元是 std::ostream，要寫成成員函式就得改
//         std::ostream 的定義 —— 那是標準庫的類別，改不了。
//         而 `a > b` 的左運算元是 Point，寫成成員函式沒有問題。
//     追問：那 operator<< 怎麼被找到的？
//         → 靠 ADL（引數相依查找）：編譯器會到引數所屬的 namespace 去找。
//           所以自訂型別的運算子應與型別放在同一個 namespace。
//
// ⚠️ 陷阱. 幫 Point 定義了 operator> 之後，就可以拿去 std::sort 了吧？
//     答：不行。std::sort 的預設比較用的是 **operator<**，不是 >。
//         只定義 > 對 sort 沒有幫助，仍會編譯失敗；必須另外提供 operator<，
//         或呼叫 std::sort(begin, end, cmp) 明確傳入比較器。
//     為什麼會錯：以為定義了一個比較運算子就「會比較了」。C++ 在 C++20 之前
//         六個比較運算子彼此完全獨立，定義 > 不會自動獲得 <。
//         而各個演算法要求的運算子各不相同 —— 這就是隱含介面「不可見」的
//         代價：你得逐個去查每個演算法到底要什麼。
//         （C++20 的 operator<=> 正是為了一次解決這個問題而生。）
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>

template <typename T>
T find_max(T a, T b) {
    return (a > b) ? a : b;
}

class Point {
public:
    int x, y;
    Point(int x, int y) : x(x), y(y) {}

    // 定義 > 運算子，以距離原點的平方和比較
    // 用平方和而非距離，可避免開根號的浮點運算與精度問題（單調性已足夠）
    // 尾端的 const 不可省：少了它，const Point 就不能參與比較
    bool operator>(const Point& other) const {
        return (x*x + y*y) > (other.x*other.x + other.y*other.y);
    }
};

// 為了輸出。
// 必須是自由函式：因為左運算元是 std::ostream，不是 Point。
// 回傳 std::ostream& 才能串接 cout << a << b。
std::ostream& operator<<(std::ostream& os, const Point& p) {
    return os << "(" << p.x << ", " << p.y << ")";
}

int main() {
    std::cout << "=== 補上 operator> 之後，find_max 直接就能用 ===" << std::endl;
    Point p1(1, 2), p2(3, 4);
    Point max_point = find_max(p1, p2);  // 現在可以了！
    std::cout << "max point: " << max_point << std::endl;

    std::cout << "\n=== 比較依據：距離原點的平方和 ===" << std::endl;
    std::cout << "p1 = " << p1 << " -> 1*1 + 2*2 = " << (1*1 + 2*2) << std::endl;
    std::cout << "p2 = " << p2 << " -> 3*3 + 4*4 = " << (3*3 + 4*4) << std::endl;
    std::cout << "所以 p2 較大。" << std::endl;

    std::cout << "\n=== 關鍵：find_max 一個字都沒改 ===" << std::endl;
    std::cout << "我們只擴充了 Point，演算法完全沒動 —— 這就是非侵入式擴充。"
              << std::endl;
    std::cout << "同一個 find_max 仍然服務所有舊型別：" << std::endl;
    std::cout << "  find_max(10, 20)   = " << find_max(10, 20) << std::endl;
    std::cout << "  find_max(2.5, 1.5) = " << find_max(2.5, 1.5) << std::endl;

    std::cout << "\n=== 這個序關係不是全序 ===" << std::endl;
    Point a(1, 2), b(2, 1);
    std::cout << "a = " << a << ", b = " << b
              << "（平方和都是 5）" << std::endl;
    std::cout << std::boolalpha;
    std::cout << "a > b ? " << (a > b) << "    b > a ? " << (b > a)
              << "   <- 互不大於，但兩點並不相等" << std::endl;
    std::cout << "這叫 strict weak ordering（嚴格弱序），"
                 "正是 STL 排序所要求的。" << std::endl;

    std::cout << "\n=== 但這不代表 Point 就能餵給 std::sort ===" << std::endl;
    std::cout << "std::sort 預設要求的是 operator<，不是 operator>。"
              << std::endl;
    std::cout << "只定義 > 對 sort 沒有幫助 —— 隱含介面的要求逐個演算法各不相同。"
              << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第二課：泛型編程（Generic Programming）概念9.cpp -o concept9

// === 預期輸出 ===
// === 補上 operator> 之後，find_max 直接就能用 ===
// max point: (3, 4)
//
// === 比較依據：距離原點的平方和 ===
// p1 = (1, 2) -> 1*1 + 2*2 = 5
// p2 = (3, 4) -> 3*3 + 4*4 = 25
// 所以 p2 較大。
//
// === 關鍵：find_max 一個字都沒改 ===
// 我們只擴充了 Point，演算法完全沒動 —— 這就是非侵入式擴充。
// 同一個 find_max 仍然服務所有舊型別：
//   find_max(10, 20)   = 20
//   find_max(2.5, 1.5) = 2.5
//
// === 這個序關係不是全序 ===
// a = (1, 2), b = (2, 1)（平方和都是 5）
// a > b ? false    b > a ? false   <- 互不大於，但兩點並不相等
// 這叫 strict weak ordering（嚴格弱序），正是 STL 排序所要求的。
//
// === 但這不代表 Point 就能餵給 std::sort ===
// std::sort 預設要求的是 operator<，不是 operator>。
// 只定義 > 對 sort 沒有幫助 —— 隱含介面的要求逐個演算法各不相同。
