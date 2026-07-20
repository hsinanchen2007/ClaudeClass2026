// =============================================================================
//  第 14 課：vector 元素插入：insert、emplace1.cpp
//    —  insert 最基本形式：在迭代器位置「之前」插入單一元素
// =============================================================================
//
// 【主題資訊 Information】
//   iterator insert(const_iterator pos, const T& value);  // (1) copy 插入
//   iterator insert(const_iterator pos, T&& value);       // (2) move 插入,C++11
//   標頭檔: <vector>
//   標準版本: C++98 起提供;C++11 起參數改 const_iterator 並「開始有回傳值」
//   回傳: 指向「新插入元素」的 iterator
//   複雜度: O(std::distance(pos, end())) 的元素搬移
//           + 若容量不足另有 O(size()) 的 reallocation
//           → 尾端插入 amortized O(1);開頭/中間插入 O(n)
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼參數是 iterator 而不是索引】
// vector 支援隨機存取,用索引本來也行。但 STL 的設計哲學是「容器 × 演算法
// 以 iterator 解耦」—— list、set、map 都沒有索引,只有 iterator。insert 統一
// 收 iterator,`std::lower_bound`、`std::find` 的回傳值就能直接餵進來:
//     v.insert(std::lower_bound(v.begin(), v.end(), x), x);   // 維持排序插入
// 若 insert 只收索引,每次都得多寫一個 `- v.begin()` 把 iterator 轉回索引。
//
// 【2. 為什麼是插入「之前」而不是「之後」】
// 這是為了讓 `v.end()` 有意義。若語意定為「插入之後」,就永遠無法插到第 0 個
// 元素之前;而 end() 是「最後一個元素的下一個」的哨兵位置,它「之後」並不存在。
// 定義成「之前」,兩個邊界都自然成立:
//     v.insert(v.begin(), x)  → 插到最前面
//     v.insert(v.end(),   x)  → 插到最後面(等同 push_back)
// 這也讓 [first, last) 半開區間的慣例貫穿整個 STL。
//
// 【3. 插入實際是怎麼發生的:搬移,不是「塞進去」】
// vector 的元素在記憶體中是一段連續空間,中間沒有洞可以塞。所以
// `v.insert(v.begin()+2, 100)` 實際做的是:
//     1) 確認 capacity 夠 —— 不夠就先 reallocate(配新空間、搬全部元素、釋放舊的)
//     2) 把 [pos, end) 整段往後移一格(必須從後往前搬,否則會蓋掉還沒搬的元素)
//     3) 在空出來的 pos 位置建構新元素
//     4) size() + 1
// 「離 end() 越遠,要搬的元素越多」—— 這就是中間插入 O(n) 的來源。
//
// 【概念補充 Concept Deep Dive】
// 記憶體佈局(以 v = {1,2,3,4,5},capacity=8 為例):
//     插入前: [1][2][3][4][5][ ][ ][ ]      size=5
//                   ↑ pos = begin()+2
//     搬移後: [1][2][ ][3][4][5][ ][ ]      3,4,5 各往後一格
//     建構後: [1][2][100][3][4][5][ ][ ]    size=6
// libstdc++ 對 trivially copyable 的型別(如 int)會把搬移退化成一次 memmove;
// 對有自訂 move constructor 的型別則逐一 move-construct / move-assign。
// 也就是說「O(n)」的常數項對 int 很小,對 std::string 這種要逐一 move 的型別大得多。
//
// 【注意事項 Pay Attention】
// 1. 插入後所有 iterator/pointer/reference 都可能失效:
//      * 若發生 reallocation(新 size > capacity)→ 全部失效
//      * 若沒有 reallocation → 插入點「及其之後」失效,之前的仍有效
//    正確做法永遠是接住 insert 的回傳值,不要沿用舊 iterator。
// 2. `v.insert(v.begin(), x)` 放在迴圈裡是 O(n²) 陷阱(見本課第 7、9 個檔案)。
// 3. pos 必須是「這個 vector」的有效 iterator(可以是 end());
//    傳別的容器的 iterator 是 UB,編譯器不會擋。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector::insert 基本語意
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. v.insert(pos, value) 把 value 插在 pos 之前還是之後?回傳什麼?
//     答：插在 pos「之前」。回傳指向「新插入元素」的 iterator。
//         定義成「之前」才能讓 v.end() 有意義(end 之後不存在),
//         也才能用 v.insert(v.begin(), x) 插到最前面。
//     追問：那 v.insert(v.end(), x) 等於什麼?→ 等於 push_back(x)。
//
// 🔥 Q2. 在 vector 開頭插入的複雜度是多少?為什麼不是 O(1)?
//     答：O(n)。vector 元素在記憶體中連續存放、中間沒有空洞,
//         要在前面挪出位置就得把後面 n 個元素整段往後搬一格。
//         只有尾端插入才是 amortized O(1)(不必搬任何元素)。
//     追問：需要頻繁在頭部插入怎麼辦?→ 改用 std::deque(頭尾都 amortized O(1)),
//         或反向 push_back 到最後再 std::reverse 一次。
//
// ⚠️ 陷阱. 這段程式碼為什麼是 UB?
//         auto it = v.begin() + 2;
//         v.insert(it, 10);
//         v.insert(it, 20);   // ← 問題在這
//     答：第一次 insert 之後 it 已經失效。若發生 reallocation,整塊記憶體換了
//         位置,it 變成懸空指標;即使沒有 reallocation,插入點之後的 iterator
//         也一律失效。第二次拿它來 insert 就是 UB。
//     為什麼會錯：多數人腦中把 iterator 想成「第 2 個位置」這種邏輯座標,
//         但 vector 的 iterator 實際上就是一根裸指標,它綁的是記憶體位址,
//         不是邏輯索引。正解是 `it = v.insert(it, 10);` 接住回傳值。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5};

    // insert 需要一個迭代器指定插入位置
    // 新元素會插入在該迭代器「之前」

    // 在開頭插入 → 後面 5 個元素全部往後搬一格,O(n)
    v.insert(v.begin(), 0);
    // v: {0, 1, 2, 3, 4, 5}

    // 在第三個位置插入（索引 2）→ 需搬移其後 4 個元素
    v.insert(v.begin() + 2, 100);
    // v: {0, 1, 100, 2, 3, 4, 5}

    // 在結尾插入（等同於 push_back）→ 不必搬移任何元素,amortized O(1)
    v.insert(v.end(), 6);
    // v: {0, 1, 100, 2, 3, 4, 5, 6}

    for (int x : v) {
        std::cout << x << " ";
    }
    std::cout << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 14 課：vector 元素插入：insert、emplace1.cpp" -o insert1

// === 預期輸出 ===
// 0 1 100 2 3 4 5 6 
