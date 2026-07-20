// =============================================================================
//  第 14 課：vector 元素插入：insert、emplace5.cpp
//    —  initializer_list insert：用 {a, b, c} 直接插入一串字面值
// =============================================================================
//
// 【主題資訊 Information】
//   iterator insert(const_iterator pos, std::initializer_list<T> ilist);
//   標頭檔: <vector>(std::initializer_list 定義在 <initializer_list>,
//                     但 <vector> 保證會間接引入)
//   標準版本: C++11
//   回傳: 指向第一個被插入元素的 iterator;ilist 為空時回傳 pos
//   複雜度: O(ilist.size() + distance(pos, end()))
//
// 【詳細解釋 Explanation】
//
// 【1. 這個重載其實是 range 版的語法糖】
// `v.insert(pos, {10, 20, 30})` 等價於
// `v.insert(pos, ilist.begin(), ilist.end())`。它存在的理由純粹是可讀性:
// 想插入一串字面值時,不必先宣告一個暫時容器再取它的 begin/end。
// 效能特性與 range 版完全相同 —— initializer_list 的 iterator 是 const T*,
// 屬於 random access,所以能事先算出數量、一次搬移。
//
// 【2. initializer_list 的元素是 const,而且不擁有記憶體】
// std::initializer_list<T> 內部只是「指向一塊 const T 陣列的指標 + 長度」。
// 那塊陣列的生命期只到「包含它的完整運算式」結束為止。這帶來兩個後果:
//   * 元素永遠是 const → 只能複製進 vector,**不能 move**。
//     對 vector<std::string> 用 insert(pos, {s1, s2}) 會發生兩次「複製」,
//     即使你寫 std::move(s1) 也一樣(因為 list 的元素型別是 const string)。
//   * 不能保存起來事後使用 —— 存下 initializer_list 再跨行使用是懸空參考。
//
// 【3. 什麼時候該用它、什麼時候不該】
//   該用:插入的是「寫死在程式碼裡的少量字面值」,例如預設設定、測試資料。
//   不該用:元素是需要搬移的重型物件(改用 range 版 + std::make_move_iterator),
//           或數量由執行期決定(改用 fill / range 版)。
//
// 【概念補充 Concept Deep Dive】
// `{10, 20, 30}` 在編譯期會被放進唯讀資料段(或堆疊上的暫時陣列),
// 然後建構出一個只有兩個成員的輕量物件傳給 insert:
//     struct initializer_list<T> { const T* _M_array; size_type _M_len; };
// 所以「傳 initializer_list」本身幾乎沒有成本(等同傳兩個機器字),
// 真正的成本在於把那 n 個元素**逐一複製**進 vector。
// 這也解釋了為何它無法 move:來源是 const,move 會退化成 copy。
//
// 【注意事項 Pay Attention】
// 1. 大括號與圓括號的差異在 vector 的建構子上是有名的坑:
//      std::vector<int> a(3, 0);   // 3 個 0            → {0, 0, 0}
//      std::vector<int> b{3, 0};   // initializer_list  → {3, 0}
//    insert 沒有這個歧義(第一個參數一定是 iterator),但同一個直覺要記住。
// 2. initializer_list 不能 move,重型元素請改用
//    `std::make_move_iterator` 搭配 range 版。
// 3. 空的 `{}` 會插入 0 個元素並回傳 pos,不是插入一個預設建構的元素。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】initializer_list insert
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. v.insert(pos, {1, 2, 3}) 跟 v.insert(pos, first, last) 有什麼差別?
//     答：語意與效能完全相同,前者只是後者的語法糖(內部就是拿 ilist 的
//         begin/end 去呼叫 range 版)。差別純粹在可讀性:插入寫死的字面值時
//         不必先造一個暫存容器。
//     追問：那 initializer_list 有什麼是 range 版做得到、它做不到的?
//         → move。initializer_list 的元素型別是 const T,永遠只能複製;
//         range 版可以搭配 std::make_move_iterator 把元素搬進來。
//
// ⚠️ 陷阱. vector<std::string> v; v.insert(v.begin(), {std::move(s1), std::move(s2)});
//     這樣寫有把 s1、s2 搬進去嗎?
//     答：沒有。initializer_list 的元素是 const std::string,
//         move 會因為無法繫結到 string&& 而退化成 copy,s1/s2 內容原封不動。
//         真要 move 得用:
//         std::string tmp[] = {std::move(s1), std::move(s2)};
//         v.insert(v.begin(), std::make_move_iterator(std::begin(tmp)),
//                             std::make_move_iterator(std::end(tmp)));
//     為什麼會錯：以為 std::move 是「一定會搬移」的指令。它其實只是個
//         static_cast<T&&>,搬不搬得成要看目標型別接不接受 rvalue;
//         元素是 const 時就一律退回 copy,而且編譯器不會給任何警告。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <string>
#include <iterator>

int main() {
    std::vector<int> v = {1, 2, 3};

    // 使用初始化串列插入多個元素（本質上就是 range insert 的語法糖）
    v.insert(v.begin() + 1, {10, 20, 30});

    for (int x : v) {
        std::cout << x << " ";  // 1 10 20 30 2 3
    }
    std::cout << std::endl;

    // 空的 {} 插入 0 個元素，不是插入一個預設值
    v.insert(v.begin(), {});
    std::cout << "插入 {} 之後 size = " << v.size() << std::endl;

    // ── initializer_list 無法 move：示範正確的搬移寫法 ──
    std::vector<std::string> names;
    std::string a = "Alice";
    std::string b = "Bob";

    // 用陣列 + make_move_iterator 才是真正的搬移
    std::string tmp[] = {std::move(a), std::move(b)};
    names.insert(names.begin(),
                 std::make_move_iterator(std::begin(tmp)),
                 std::make_move_iterator(std::end(tmp)));

    std::cout << "搬移後 names: ";
    for (const auto& n : names) std::cout << n << " ";
    std::cout << std::endl;

    // 被搬移的來源處於「有效但未指定 (valid but unspecified)」狀態。
    // 標準【沒有】規定它會變成空字串——下面這行是 libstdc++ 的實測結果，
    // 不是可移植的保證。實務上搬移後只能對它做「賦新值」或「解構」，
    // 絕不能讀它原本的內容。
    std::cout << "tmp[0] 被搬移後是否為空 (libstdc++ 實測, 非標準保證): "
              << (tmp[0].empty() ? "是" : "否") << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 14 課：vector 元素插入：insert、emplace5.cpp" -o insert5

// tmp[0] 被搬移後是否為空 (libstdc++ 實測, 非標準保證): 是

// === 預期輸出 ===
// 1 10 20 30 2 3 
// 插入 {} 之後 size = 6
// 搬移後 names: Alice Bob 
