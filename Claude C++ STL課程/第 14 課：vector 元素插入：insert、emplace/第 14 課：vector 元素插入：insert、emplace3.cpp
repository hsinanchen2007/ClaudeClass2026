// =============================================================================
//  第 14 課：vector 元素插入：insert、emplace3.cpp
//    —  fill insert：一次插入 n 個相同元素
// =============================================================================
//
// 【主題資訊 Information】
//   iterator insert(const_iterator pos, size_type count, const T& value);
//   標頭檔: <vector>
//   標準版本: C++98 起提供;C++11 起才有回傳值(原本回傳 void)
//   回傳: 指向「第一個」被插入元素的 iterator;count == 0 時回傳 pos
//   複雜度: O(count + distance(pos, end())) —— 一次搬移,不是搬 count 次
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼不是「呼叫 count 次單元素 insert」】
// 這是 fill insert 存在的全部理由。若你自己寫迴圈:
//     for (int i = 0; i < 4; ++i) v.insert(v.begin() + 1, 0);   // 壞寫法
// 每一次 insert 都要把後面的元素整段往後搬一格 → 總共 O(count × n)。
// 而 `v.insert(v.begin() + 1, 4, 0)` 只做一次規劃:先確認容量、把後段一次
// 往後搬 count 格、再在空出來的 count 格中填值 → O(count + n)。
// 資料量一大,這是「幾毫秒」與「幾秒」的差別。
//
// 【2. 容量規劃也只做一次】
// 迴圈版可能觸發多次 reallocation(每次都是配新記憶體 + 搬全部 + 釋放舊的);
// fill 版事先知道總共要多 count 個位置,最多只 reallocate 一次。
// 這條「批次操作優於逐一操作」的原則貫穿整個 vector:
// insert 的 fill / range / initializer_list 三個重載都是同一個道理。
//
// 【3. count == 0 的邊界】
// 標準規定此時什麼都不做並回傳 pos。這讓「插入數量由執行期計算」的程式碼
// 不必特別寫 if 判斷:
//     v.insert(pos, need_padding, ' ');   // need_padding 可能是 0，直接呼叫就好
//
// 【概念補充 Concept Deep Dive】
// libstdc++ 的 _M_fill_insert 大致分三種情況處理(以在中間插入為例):
//   (a) 容量夠、且「插入點之後的元素數」> count:
//       先把尾端 count 個元素 move-construct 到未初始化區,
//       再把中間那段往後 move-assign,最後在空出的位置 fill。
//   (b) 容量夠、但「插入點之後的元素數」<= count:
//       先在未初始化區補齊多出來的 value,再搬既有元素,再填剩下的。
//   (c) 容量不夠:直接配一塊新記憶體,依序 move「前段 → count 個新值 → 後段」。
// 之所以要分這麼細,是因為 C++ 必須區分「對已建構物件賦值(assign)」與
// 「在生記憶體上建構物件(construct)」—— 這是 C 的 memmove 不需要煩惱的事。
//
// 【注意事項 Pay Attention】
// 1. `v.insert(v.begin(), 3, 0)` 的第二個參數是「個數」不是「位置」。
//    與 `v.insert(v.begin() + 3, 0)`(在索引 3 插入一個 0)完全不同,極易看錯。
// 2. vector<int> 有個經典歧義:`v.insert(v.begin(), 3, 0)` 兩個參數都是 int,
//    編譯器必須在「fill 版(count, value)」與「range 版(first, last)」之間選。
//    標準要求此時選 fill 版(range 版對非 integral 的 InputIt 才成立),
//    所以結果是插入 3 個 0,不會被誤判成迭代器範圍。
// 3. value 是 const T&,會被複製 count 次。若 T 複製昂貴且 count 很大,
//    考慮改用 resize 或先 reserve 再處理。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】fill insert
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. v.insert(pos, 4, 0) 跟寫 4 次 v.insert(pos, 0) 有什麼差別?
//     答：結果的元素內容一樣,但複雜度差一個量級。fill 版只搬移一次後段、
//         最多只 reallocate 一次 → O(count + n);迴圈版每次都搬一次後段、
//         可能多次 reallocate → O(count × n)。
//     追問：那 range 版 insert(pos, first, last) 呢?→ 同樣是批次一次搬完,
//         但前提是 first/last 至少是 forward iterator(能先算出距離);
//         若只是 input iterator(如 istream_iterator),事先不知道有幾個,
//         就退化成逐一插入。
//
// ⚠️ 陷阱. 下面兩行差在哪?很多人會看錯。
//         v.insert(v.begin(), 3, 0);
//         v.insert(v.begin() + 3, 0);
//     答：第一行是「在最前面插入 3 個 0」;第二行是「在索引 3 的位置插入 1 個 0」。
//         3 在第一行是「個數」,在第二行是「位移量」。
//     為什麼會錯：把 insert 的參數順序想成 (位置, 位置, 值),
//         但實際上 fill 版的簽名是 (pos, count, value) —— 位置只有第一個參數。
//
// ⚠️ 陷阱. vector<int> v; v.insert(v.begin(), 3, 0); 會不會被當成
//     「插入 iterator 範圍 [3, 0)」而編譯錯誤?
//     答：不會。標準特別規定:當這兩個參數是 integral type 時,必須選 fill 版。
//         所以結果是插入 3 個 0。
//     為什麼會錯：知道有 range 版 insert(pos, first, last) 之後,
//         會擔心 (3, 0) 被推導成 InputIt = int 而走錯重載。
//         標準用 enable_if / 標籤分派把這個坑填掉了。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3};

    // 在位置 1 插入 4 個 0
    // 一次搬移、最多一次 reallocation → O(count + n)
    auto it = v.insert(v.begin() + 1, 4, 0);

    std::cout << "回傳的 iterator 指向第一個插入元素, 索引 = "
              << (it - v.begin()) << std::endl;

    for (int x : v) {
        std::cout << x << " ";  // 1 0 0 0 0 2 3
    }
    std::cout << std::endl;

    // count == 0：什麼都不做，直接回傳 pos，不必自己寫 if
    std::size_t padding = 0;
    v.insert(v.begin(), padding, -1);
    std::cout << "插入 0 個元素後 size = " << v.size() << std::endl;

    // 對照：同樣看起來有「3」，語意完全不同
    std::vector<int> a = {7, 8, 9};
    std::vector<int> b = {7, 8, 9};
    a.insert(a.begin(), 3, 0);      // 在最前面插入 3 個 0
    b.insert(b.begin() + 3, 0);     // 在索引 3 插入 1 個 0

    std::cout << "insert(begin(), 3, 0)  -> ";
    for (int x : a) std::cout << x << " ";
    std::cout << std::endl;

    std::cout << "insert(begin() + 3, 0) -> ";
    for (int x : b) std::cout << x << " ";
    std::cout << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 14 課：vector 元素插入：insert、emplace3.cpp" -o insert3

// === 預期輸出 ===
// 回傳的 iterator 指向第一個插入元素, 索引 = 1
// 1 0 0 0 0 2 3 
// 插入 0 個元素後 size = 7
// insert(begin(), 3, 0)  -> 0 0 0 7 8 9 
// insert(begin() + 3, 0) -> 7 8 9 0 
