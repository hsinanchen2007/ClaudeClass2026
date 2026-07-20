// =============================================================================
//  第 14 課：vector 元素插入：insert、emplace4.cpp
//    —  range insert：把另一個範圍 [first, last) 整段插進來
// =============================================================================
//
// 【主題資訊 Information】
//   template <class InputIt>
//   iterator insert(const_iterator pos, InputIt first, InputIt last);
//   標頭檔: <vector>
//   標準版本: C++98 起提供;C++11 起才有回傳值
//   回傳: 指向第一個被插入元素的 iterator;first == last 時回傳 pos
//   複雜度: forward iterator 以上 → O(distance(first,last) + distance(pos,end()))
//           純 input iterator     → 事先不知道有幾個,可能退化成多次搬移
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 range insert 對 iterator 類別這麼敏感】
// 批次插入要快,關鍵在「能不能事先知道要插入幾個元素」:
//   * forward iterator 以上(vector/array/list 的 iterator、原生指標):
//     可以先 std::distance(first, last) 算出 n,一次配足容量、一次搬完後段。
//   * 純 input iterator(如 std::istream_iterator):只能讀一次、無法回頭,
//     事先無從得知數量 → 實作只能邊讀邊插,可能多次搬移與 reallocation。
// 這就是為什麼「從檔案串流直接 range insert 進 vector」效能常常不如預期,
// 正解是先讀進暫存 vector,再一次 range insert。
//
// 【2. 原生陣列與指標:STL 的 iterator 概念本來就涵蓋指標】
// `v.insert(v.end(), arr, arr + 3)` 能編過,是因為 T* 完全滿足
// random access iterator 的所有需求(可解參考、可 ++、可 +n、可相減)。
// 這是 STL 刻意的設計:讓 C 陣列不必包裝就能餵給演算法與容器。
// C++11 起更建議用 `std::begin(arr)` / `std::end(arr)`,因為它對
// 「陣列」與「有 begin/end 成員的容器」都成立,寫泛型程式時不必改。
//
// 【3. 一個致命限制:[first, last) 不可以指向「同一個 vector」】
// 標準規定 range insert 的 first/last 不得是「被插入容器自身」的 iterator。
//     v.insert(v.begin(), v.begin() + 1, v.end());   // UB！
// 原因很直接:插入過程可能 reallocate 或搬移元素,你正在讀的來源區段
// 會在讀到一半時被搬走或釋放。要做這件事必須先複製出來:
//     std::vector<int> tmp(v.begin() + 1, v.end());
//     v.insert(v.begin(), tmp.begin(), tmp.end());   // 安全
// 注意這與「單元素 insert 傳自己的元素」不同 —— 那個標準要求必須正確處理
// (見 summary.cpp 的自我插入段落),但 range 版沒有這個保證。
//
// 【概念補充 Concept Deep Dive】
// libstdc++ 用 iterator_traits 取出 iterator_category,再用標籤分派選實作:
//     _M_range_insert(pos, first, last, std::input_iterator_tag)    // 逐一 insert
//     _M_range_insert(pos, first, last, std::forward_iterator_tag)  // 先算 n 再批次
// 這是 STL 最典型的「編譯期分派」手法:同一個公開介面,依型別特性在編譯期
// 選出最有效率的實作,執行期沒有任何 if 或虛擬呼叫的成本。
//
// 【注意事項 Pay Attention】
// 1. first/last 必須來自「別的」容器。指向自己是 UB(見上)。
// 2. `v.insert(v.end(), other.begin(), other.end())` 是「附加整個容器」的
//    標準寫法,比迴圈 push_back 快(一次配容量、一次搬移)。
// 3. 若來源是 rvalue 容器且之後不再使用,C++17 可用
//    `std::move(other.begin(), other.end(), std::back_inserter(v))`
//    避免逐一複製;或直接考慮 `v = std::move(other)`(若能整個接管)。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】range insert
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. v.insert(v.end(), other.begin(), other.end()) 為什麼比
//     for (x : other) v.push_back(x) 快?
//     答：range 版能先用 std::distance 算出要插入幾個元素,一次把容量配足、
//         一次完成搬移,最多只 reallocate 一次。逐一 push_back 則可能觸發
//         多次 reallocation,每次都要重新配置並搬移全部既有元素。
//     追問：什麼情況下這個優勢會消失?→ 當來源只是 input iterator
//         (例如 std::istream_iterator)時,事先算不出數量,只能退化成逐一插入。
//
// ⚠️ 陷阱. 這行為什麼是 UB?
//         v.insert(v.begin(), v.begin() + 1, v.end());
//     答：range insert 的 [first, last) 不可以是被插入容器自身的 iterator。
//         插入過程會搬移甚至 reallocate,來源區段可能在讀到一半時被搬走或釋放。
//         必須先複製到暫存容器再插入。
//     為什麼會錯：知道「單元素 insert 傳自己的元素是安全的」(標準有保證),
//         就以為 range 版也一樣。這兩者的規定不同 —— 單元素版標準要求實作
//         必須處理自我參考,range 版則直接把它列為 UB。
//
// ⚠️ 陷阱. int arr[] = {1,2,3}; v.insert(v.end(), arr, arr + 3);
//     這樣傳「指標」當 iterator 是不是取巧?
//     答：完全合法,而且是刻意設計。原生指標滿足 random access iterator 的
//         所有需求,STL 的 iterator 是一組 concept 而非某個特定型別。
//     為什麼會錯：以為 iterator 一定得是某個 class。實際上 STL 的泛型設計
//         讓 T* 天生就是最基本的 iterator 模型。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <iterator>

int main() {
    std::vector<int> v = {1, 2, 3};

    // 從另一個容器插入範圍：一次算好數量、一次搬移
    std::vector<int> source = {10, 20, 30};
    v.insert(v.begin() + 1, source.begin(), source.end());

    std::cout << "插入 vector 範圍後: ";
    for (int x : v) {
        std::cout << x << " ";  // 1 10 20 30 2 3
    }
    std::cout << std::endl;

    // 從原生陣列插入：T* 本身就是合格的 random access iterator
    int arr[] = {100, 200};
    v.insert(v.end(), std::begin(arr), std::end(arr));

    std::cout << "插入陣列後: ";
    for (int x : v) {
        std::cout << x << " ";  // 1 10 20 30 2 3 100 200
    }
    std::cout << std::endl;

    // ── 要「把自己的一段插回自己」時，必須先複製出來 ──
    // v.insert(v.begin(), v.begin() + 1, v.end());   // ← UB，絕對不要這樣寫
    std::vector<int> w = {1, 2, 3};
    std::vector<int> tmp(w.begin() + 1, w.end());     // 先複製到暫存
    w.insert(w.begin(), tmp.begin(), tmp.end());      // 再安全插入

    std::cout << "自我複製後插入: ";
    for (int x : w) {
        std::cout << x << " ";
    }
    std::cout << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 14 課：vector 元素插入：insert、emplace4.cpp" -o insert4

// === 預期輸出 ===
// 插入 vector 範圍後: 1 10 20 30 2 3 
// 插入陣列後: 1 10 20 30 2 3 100 200 
// 自我複製後插入: 2 3 1 2 3 
