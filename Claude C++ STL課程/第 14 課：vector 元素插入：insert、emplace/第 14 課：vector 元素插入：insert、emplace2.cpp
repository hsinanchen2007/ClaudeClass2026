// =============================================================================
//  第 14 課：vector 元素插入：insert、emplace2.cpp
//    —  insert 的回傳值：唯一還活著的那根 iterator
// =============================================================================
//
// 【主題資訊 Information】
//   iterator insert(const_iterator pos, const T& value);
//   標頭檔: <vector>
//   標準版本: 回傳值是 C++11 才加的(C++98 的單元素 insert 回傳 iterator,
//             但 fill / range 版本回傳 void;C++11 統一成全部回傳 iterator)
//   回傳: 指向「第一個被插入元素」的 iterator
//   複雜度: O(distance(pos, end())) + 可能的 reallocation
//
// 【詳細解釋 Explanation】
//
// 【1. 回傳值為什麼重要:它是插入後唯一保證有效的 iterator】
// insert 之後,你手上的舊 iterator 全部處於「可能已失效」狀態:
//     * 有 reallocation → 全部失效(整塊記憶體換位置)
//     * 無 reallocation → 插入點及其之後失效
// 但標準保證:insert **回傳的** iterator 一定指向新插入的元素、一定有效。
// 這讓「插入 → 繼續在附近操作」變成安全的連鎖動作,不必重新 begin() + 算偏移。
//
// 【2. 為什麼是「第一個」被插入的元素】
// 單元素版本只插一個,「第一個」就是它本身。但 fill / range / initializer_list
// 版本會一次插入多個,標準統一規定回傳「第一個」:
//     auto it = v.insert(pos, {10, 20, 30});   // it 指向 10
//     // 想拿到最後一個 → it + 2;想拿到插入區段的尾後位置 → it + 3
// 選「第一個」而不是「最後一個」,是因為插入區段的起點加上已知的插入數量就能
// 推出所有位置,反過來則需要往前算、比較容易寫錯。
//
// 【3. C++98 → C++11 的介面統一】
// C++98 時 fill 版與 range 版的 insert 回傳 void,想知道插到哪只能自己先記
// `auto idx = pos - v.begin();` 再用 `v.begin() + idx` 還原。C++11 把全部
// 重載統一成回傳 iterator,才有現在這種可鏈接的寫法。
//
// 【概念補充 Concept Deep Dive】
// 為什麼回傳值一定有效?因為 libstdc++ 內部本來就知道插入點在新緩衝區的偏移量:
//     const size_type idx = pos - begin();     // 先把 iterator 轉成「偏移量」
//     ... 可能 reallocate、搬移 ...
//     return begin() + idx;                    // 用新的 begin() 重建 iterator
// 關鍵在於「偏移量(整數)不會因為記憶體搬家而失效,指標會」。
// 這也直接給了你自救的通用手法:凡是要跨越可能 reallocate 的操作保存位置,
// 就把 iterator 降級成 index,事後再升級回 iterator。
//
// 【注意事項 Pay Attention】
// 1. 回傳的 iterator 只在「下一次修改容器之前」有效。再 insert 一次,
//    它一樣會失效 —— 要嘛再接住新的回傳值,要嘛改用 index。
// 2. `it - v.begin()` 的型別是 difference_type(有號),不是 size_type。
//    拿來跟 size() 直接比較會觸發 signed/unsigned 比較警告。
// 3. 別假設回傳值等於你傳進去的 pos。沒有 reallocation 時位址碰巧相同,
//    有 reallocation 時完全不同 —— 不能依賴這個巧合。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】insert 的回傳值
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. insert 回傳什麼?插入多個元素時回傳哪一個?
//     答：回傳指向「第一個被插入元素」的 iterator。fill 版
//         insert(pos, 3, x)、range 版、initializer_list 版都一樣是第一個。
//         想拿到最後一個插入元素就自己 + (n-1),尾後位置就 + n。
//     追問：為什麼要選第一個?→ 有了起點與插入數量就能推出全部位置;
//         若回傳最後一個,要往回算反而容易差一格。
//
// 🔥 Q2. 為什麼說 insert 的回傳值是「插入後唯一保證有效的 iterator」?
//     答：insert 可能觸發 reallocation 讓全部 iterator 失效;即使沒有
//         reallocation,插入點及其之後的 iterator 也失效。標準明確保證
//         回傳值指向新元素且有效,所以它是安全接續操作的唯一起點。
//     追問：那插入點「之前」的 iterator 呢?→ 只在沒有 reallocation 時仍有效;
//         由於你通常無法預知會不會 reallocate,實務上一律當成失效處理。
//
// ⚠️ 陷阱. 有人為了「省事」這樣寫,為什麼不可靠?
//         auto it = v.insert(v.begin() + 1, 100);
//         v.insert(v.begin() + 1, 99);      // 直接重算,不用 it
//         std::cout << *it;                 // ← 這行
//     答：第二次 insert 之後 it 又失效了,印出來是 UB。重算 begin()+1 本身沒錯,
//         錯在後面又用了上一輪留下的 it。
//     為什麼會錯：腦中認為「it 指向 100 這個值」,但 iterator 綁的是位址不是值。
//         100 這個元素被往後搬了一格,而 it 還停在原本的位址上。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3};

    // insert 回傳指向「新插入元素」的 iterator
    auto it = v.insert(v.begin() + 1, 100);

    std::cout << "插入的值: " << *it << std::endl;                    // 100
    std::cout << "插入位置的索引: " << (it - v.begin()) << std::endl;  // 1

    // 可以利用回傳值繼續操作：在 100 之前再插入 99
    // 注意這裡「用完就丟」——這行之後 it 本身也失效了
    v.insert(it, 99);

    for (int x : v) {
        std::cout << x << " ";  // 1 99 100 2 3
    }
    std::cout << std::endl;

    // ── 通用自救手法：跨越可能 reallocate 的操作時，把 iterator 降級成 index ──
    std::vector<int> w = {5, 6, 7};
    auto        pos = w.begin() + 1;
    std::size_t idx = static_cast<std::size_t>(pos - w.begin());  // 存偏移量

    w.reserve(100);   // 這裡一定 reallocate，pos 已經失效，但 idx 還是對的

    w.insert(w.begin() + static_cast<std::ptrdiff_t>(idx), 55);   // 用 index 重建
    std::cout << "index 重建後: ";
    for (int x : w) {
        std::cout << x << " ";
    }
    std::cout << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 14 課：vector 元素插入：insert、emplace2.cpp" -o insert2

// === 預期輸出 ===
// 插入的值: 100
// 插入位置的索引: 1
// 1 99 100 2 3 
// index 重建後: 5 55 6 7 
