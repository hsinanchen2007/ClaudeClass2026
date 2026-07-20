// =============================================================================
//  第 14 課：vector 元素插入：insert、emplace8.cpp
//    —  插入時的迭代器失效 (iterator invalidation) 規則
// =============================================================================
//
// 【主題資訊 Information】
//   標準依據: [vector.modifiers] —— insert / emplace 的失效規則
//     * 若插入造成 size() > 舊 capacity()(即發生 reallocation):
//         → **所有** iterator、reference、pointer 全部失效
//     * 若沒有 reallocation:
//         → 插入點**及其之後**的 iterator/reference/pointer 失效
//         → 插入點**之前**的仍然有效
//   標頭檔: <vector>
//   標準版本: C++98 起(規則本身未曾改變)
//
// 【詳細解釋 Explanation】
//
// 【1. 兩種失效,成因完全不同】
//   (a) reallocation 造成的失效:
//       容量不夠 → 配置一塊全新的記憶體 → 把舊元素搬過去 → 釋放舊記憶體。
//       舊 iterator 存的是**舊記憶體的位址**,那塊記憶體已經被 free 了,
//       它現在是懸空指標。連 `v.begin()` 之前取得的都失效。
//   (b) 元素搬移造成的失效:
//       容量夠,記憶體沒換,但插入點之後的元素全部往後挪了一格。
//       舊 iterator 位址還在同一塊合法記憶體裡,**不會 crash**,
//       但它指向的已經是「別的元素」了 —— 這比 crash 更危險,因為程式
//       會安靜地算出錯誤結果。
//
// 【2. 為什麼「插入點之前」的 iterator 在沒有 reallocation 時仍有效】
// 因為那些元素完全沒有被碰過。vector 只搬移 [pos, end) 這一段,
// [begin, pos) 原封不動,位址與內容都沒變。這是標準明文給的保證,
// 不是實作巧合。
//
// 【3. 實務結論:一律當成「全部失效」】
// 上面的規則雖然精確,但你在寫程式時**通常無法預知會不會 reallocate**
// (要知道 capacity 與 size 的確切關係)。因此業界通用做法是:
//     任何修改容器大小的操作之後,舊 iterator 一律視為失效。
// 要跨越修改保存位置,只有兩個可靠手段:
//     * 接住 insert/erase 的回傳值
//     * 把 iterator 降級成 index(整數不會因記憶體搬家而失效)
//
// 【概念補充 Concept Deep Dive】
// vector<int> 的 iterator 在 release 建置下通常就是 `int*` 本身
// (libstdc++ 用 __normal_iterator 包一層,但那只是型別安全的包裝,
//  裡面就是一根裸指標,零額外成本)。
// 這解釋了一切:
//   * 為什麼 reallocation 之後 iterator 會懸空 → 因為它就是指標
//   * 為什麼失效的 iterator 用起來「有時候看起來沒事」→ 因為那塊記憶體
//     可能還沒被還給 OS,讀它不會立刻 segfault,但值已經不對了
//   * 為什麼 `it + 1` 這麼快 → 因為就是指標算術
//
// 除錯技巧:libstdc++ 提供 `-D_GLIBCXX_DEBUG`,會把 iterator 換成
// 有檢查能力的版本,在使用失效 iterator 時直接中止並印出診斷訊息。
// 開發階段值得打開(執行速度會明顯變慢,不要用在正式環境)。
//
// 【注意事項 Pay Attention】
// 1. 使用已失效的 iterator 是 **UB**。它可能看起來正常、可能印出垃圾值、
//    可能崩潰 —— 行為不可預測,而且會隨最佳化等級與編譯器版本改變。
//    絕不能寫「它一定會崩潰」或「它一定印出 X」。
// 2. reference 與 pointer 的失效規則和 iterator 完全相同。
//    存了 `int& r = v[0];` 之後再 insert,r 一樣可能懸空。
// 3. 失效規則各容器不同,別混用直覺:
//      * vector : 插入可能全失效(如上)
//      * deque  : 插入使**所有 iterator** 失效,但插在頭尾時 reference 仍有效
//      * list   : 插入**不會**使任何 iterator/reference 失效
//    這是選容器時的重要考量,不只是效能問題。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector 插入的迭代器失效
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 完整說明 vector::insert 之後哪些 iterator 會失效。
//     答：分兩種情況。若插入導致 size() 超過原本的 capacity() 而發生
//         reallocation,則**所有** iterator、reference、pointer 全部失效
//         (記憶體整塊換了位置)。若沒有 reallocation,則只有**插入點及其之後**
//         的失效(那些元素被往後挪了),插入點之前的仍然有效。
//     追問：實務上你怎麼處理?→ 一律當成全部失效。因為呼叫端通常無法預知
//         會不會 reallocate,唯二可靠的做法是接住 insert 的回傳值,
//         或把位置存成 index。
//
// 🔥 Q2. 為什麼 list 插入不會使 iterator 失效,vector 卻會?
//     答：list 是節點式容器,每個元素獨立配置,插入只是改幾個 next/prev 指標,
//         既有節點的位址完全不變。vector 是連續儲存,插入必須挪動後段元素,
//         容量不足時甚至要整塊換記憶體,所以位址會變。
//     追問：那 deque 呢?→ deque 插入會使**所有 iterator** 失效(它的
//         iterator 要記錄分段陣列的位置),但在頭尾插入時 **reference 仍有效**
//         (既有元素本身沒被搬動)。這是 vector/list 之外的第三種模式。
//
// ⚠️ 陷阱. 這段程式碼「跑起來好像沒問題」,為什麼仍然是嚴重的 bug?
//         std::vector<int> v = {1, 2, 3, 4, 5};   // 本機 libstdc++ 實測 capacity 剛好 5
//                                                 //（標準未規定此值，此處只為讓範例必定擴容）
//         auto it = v.begin() + 2;                 // 指向 3
//         v.insert(v.begin(), 0);                  // 觸發 reallocation
//         std::cout << *it;                        // ← 這行
//     答：insert 之後 it 已經懸空(舊記憶體已釋放),讀它是 UB。
//         它可能印出 2、可能印出垃圾、可能崩潰,行為不可預測,
//         而且會隨編譯器版本與最佳化等級改變。
//     為什麼會錯：測試時「印出來的值看起來合理」就以為程式正確。
//         但 free 過的記憶體通常還沒還給 OS,內容可能一時未被覆寫 ——
//         這是最典型的「今天能跑、上線就爆」的 UB。
//         用 -D_GLIBCXX_DEBUG 或 AddressSanitizer 可以把它揪出來。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5};

    auto it = v.begin() + 2;                            // 指向 3
    std::cout << "插入前 *it = " << *it << std::endl;   // 3

    // 在 it 之前插入元素
    auto new_it = v.insert(it, 100);

    // it 現在已失效！不應該再使用
    // std::cout << *it << std::endl;  // 未定義行為，值不可預測

    // 應該使用 insert 回傳的新迭代器
    std::cout << "new_it 指向: " << *new_it << std::endl;          // 100
    std::cout << "new_it + 1 指向: " << *(new_it + 1) << std::endl; // 3

    // ── 觀察「有沒有 reallocation」對失效範圍的影響 ──
    // 用 capacity 是否改變來判斷是否發生了 reallocation
    std::cout << "\n=== 是否發生 reallocation ===" << std::endl;
    {
        std::vector<int> a = {1, 2, 3};
        a.reserve(10);                       // 容量充足
        std::size_t cap_before = a.capacity();
        a.insert(a.begin() + 1, 99);
        std::cout << "容量充足時插入, capacity 是否改變: "
                  << (a.capacity() == cap_before ? "否 (無 reallocation)"
                                                 : "是 (發生 reallocation)")
                  << std::endl;
    }
    {
        std::vector<int> b = {1, 2, 3};
        b.shrink_to_fit();                   // 讓 capacity 貼齊 size
        std::size_t cap_before = b.capacity();
        b.insert(b.begin() + 1, 99);         // 容量不足 → 必然 reallocate
        std::cout << "容量不足時插入, capacity 是否改變: "
                  << (b.capacity() == cap_before ? "否 (無 reallocation)"
                                                 : "是 (發生 reallocation)")
                  << std::endl;
    }

    // ── 安全做法：把位置存成 index，整數不會因記憶體搬家而失效 ──
    std::cout << "\n=== 用 index 取代 iterator ===" << std::endl;
    std::vector<int> c = {10, 20, 30};
    std::size_t keep = 1;                    // 記住「索引 1」這個邏輯位置
    c.insert(c.begin(), 5);                  // 不論有沒有 reallocate…
    c.insert(c.begin(), 1);                  // …index 都還是可以重建 iterator
    std::cout << "原本索引 1 的元素現在往後移了 2 格, 值 = "
              << c[keep + 2] << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 14 課：vector 元素插入：insert、emplace8.cpp" -o insert8

// === 預期輸出 ===
// 插入前 *it = 3
// new_it 指向: 100
// new_it + 1 指向: 3
//
// === 是否發生 reallocation ===
// 容量充足時插入, capacity 是否改變: 否 (無 reallocation)
// 容量不足時插入, capacity 是否改變: 是 (發生 reallocation)
//
// === 用 index 取代 iterator ===
// 原本索引 1 的元素現在往後移了 2 格, 值 = 20
