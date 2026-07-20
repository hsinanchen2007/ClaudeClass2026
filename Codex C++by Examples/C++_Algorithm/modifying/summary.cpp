/*
 * Modifying Algorithms：面試前完整速查章
 * =======================================
 *
 * 一、核心模型
 * STL 演算法只看 iterator 範圍，不知道背後是不是 vector/list/array，因此大多只能
 * 指定、交換或搬移元素，不能替容器改 size。remove/unique 回「邏輯新尾端」，仍需
 * erase；copy/fill/transform 的目的端必須已有空間，或使用 inserter。
 *
 * 二、API 選擇表
 * copy/copy_if       保留來源，複製全部/符合條件，O(N)
 * copy_n             從起點精確複製 N 筆；N 不可超過來源有效範圍
 * copy_backward      由尾端往前複製，適合把重疊範圍向右搬
 * move               消耗來源值，來源 valid but unspecified，O(N)
 * fill/fill_n        每格固定值；`_n` 從起點處理指定筆數
 * generate/generate_n 每格呼叫 generator；有狀態 generator 的呼叫次數即契約
 * transform          每格由輸入計算輸出，O(N)
 * replace            原地替換；replace_copy(_if) 保留來源並寫到另一目的端
 * remove             穩定壓縮被保留元素，回 new_end，O(N)
 * unique             壓縮相鄰重複，回 new_end，O(N)
 * reverse            原地反轉；reverse_copy 保留來源並反向輸出，O(N)
 * swap/iter_swap     交換物件/iterator 所指元素，通常 O(1)
 * swap_ranges        逐一交換兩範圍，O(N)
 * shuffle            無偏隨機排列；sample 無放回抽 n 筆，O(N)
 *
 * 三、容量與 iterator 生命週期
 * reserve 只增加 capacity，不建立元素；copy 到 reserve-only vector.begin() 仍錯。
 * resize 才建立可指定元素；back_inserter 則呼叫 push_back。push_back 可能 reallocate，
 * 使 vector 的 iterator/reference/pointer 全失效。演算法回傳 iterator 必須在容器後續
 * 修改前使用或轉成 index。
 *
 * 四、重疊規則
 * 向左搬重疊範圍可用 copy/move；向右搬用 copy_backward/move_backward。不要假設
 * implementation 會自動 memmove。transform 只安全支援明確允許的原地同起點，
 * 危險 partial overlap 應使用暫存或正確方向演算法。
 *
 * 五、所有權
 * std::move(x) 是 cast，不會自行搬任何東西；range std::move 才逐項 move-assign。
 * moved-from 物件只能保證 valid、可析構/重新指定；不得 assert 其內容為空。
 * const 物件通常不能真正 move，因得到 const T&&，可能退回 copy。
 *
 * 六、erase-remove / erase-unique
 * `v.erase(std::remove(...),v.end())` 真正縮小 vector；C++20 std::erase 更簡潔。
 * unique 只移除相鄰重複；全域去重可 sort+unique（改順序）或 seen set（多空間）。
 * [new_end,end) 的值 unspecified，不可用來判斷刪除了什麼。
 *
 * 七、隨機演算法
 * mt19937 適合模擬/測試，不是密碼學 RNG。固定 seed 便於重現，但 shuffle/sample
 * 的確切結果不保證跨標準庫一致；測試 permutation、大小、唯一性與來源成員即可。
 * sample 是無放回抽樣；shuffle 改整體順序。
 *
 * 八、易錯陷阱清單
 * - 忘記目的容量；把 reserve 當 resize。
 * - remove/unique 後忘記 erase。
 * - moved-from 內容當成保證為空。
 * - copy 危險重疊；end iterator 被解參考。
 * - rotate 的 middle 算反；空 vector 對 size 取 modulo。
 * - shuffle 測試硬綁特定排列；mt19937 用於安全 token。
 * - `_n` 的 count 超過有效輸入/輸出範圍；copy 目的區未先建立元素。
 * - reverse_copy 的目的範圍與來源重疊；reverse UTF-8 bytes 破壞文字。
 * - replace 改到排序 key 後忘了 sorted invariant 已失效。
 */

/*
==============================================================================
【面試深挖：Modifying Algorithms】

A1｜erase-remove idiom 為何分兩步？
答：`remove` 只能重新排列 range，把保留元素移到前面並回傳 new logical end；generic
algorithm 不知道 container 如何縮小。容器的 erase 才真正銷毀尾段。C++20 可用 `std::erase`。

A2｜`unique` 能移除所有重複值嗎？
答：只能消除相鄰等價元素，且同樣不縮容器。要全域去重，常先 sort 再 unique/erase；
若不可改順序，可用 hash set 記錄 seen，時間與空間取捨不同。

A3｜`copy` 遇到重疊 range 怎麼辦？
答：目的起點落在來源內且向右覆蓋時應用 copy_backward；向左可用 copy。契約不符時不是
「可能慢」，而是結果不受保證。trivially copyable raw storage 可由 memmove 處理重疊。

A4｜`transform` 可以 in-place 嗎？
答：unary transform 的 output 可等於 input begin，典型 in-place 合法；但 operation 不可
使目前或其他 iterator invalid，且任意偏移重疊不自動安全。binary 版本也須檢查兩個 input range。

A5｜`std::move` algorithm 與 `std::move(x)` 是同一件事嗎？
答：前者把 range 元素逐一 move-assign 到目的；後者只是 cast 成 xvalue。兩者都不保證
來源變成空，只保證 moved-from object 可析構與重新賦值，具體狀態依型別契約。

A6｜`swap_ranges` 可處理重疊區間嗎？
答：不可依賴；兩個 range 重疊違反前置條件。需要在同一 range 內重排時應用 rotate、
reverse 或明確暫存，而不是拿 swap_ranges 猜測交換次序。

A7｜`rotate` 的回傳 iterator 有何用？
答：回傳原 first 元素旋轉後的位置，可把兩段重新定位。它能實作 stable insertion、
移動區塊與陣列 rotation；不是只有 LeetCode「旋轉陣列」。

A8｜`shuffle` 與舊 `random_shuffle` 的差別？
答：shuffle 明確接受 UniformRandomBitGenerator，能控制 seed 與測試重現；
random_shuffle 使用不透明亂數來源，C++14 deprecated、C++17 removed。

A9｜`fill`、`generate`、`iota` 如何選？
答：相同值用 fill；每次呼叫 generator 產值用 generate；連續遞增序列用 iota。
generator 若捕獲 mutable state，在 parallel algorithm 下還要考慮 data race。

A10｜stable 與不 stable 的修改為何是工程問題？
答：例如 partition/shuffle 前後是否要保留同 key 原順序會影響可重現性與 UI/API 契約。
不要只看 Big-O；先寫出順序是否屬於 observable behavior。
==============================================================================
*/

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

// LeetCode 2460：Apply Operations to an Array。
// 先合併相鄰相同值並設 0，再用 stable remove/填零完成 move-zeroes。
std::vector<int> leetcode_apply_operations(std::vector<int> nums) {
    for (std::size_t i = 0; i + 1U < nums.size(); ++i) {
        if (nums[i] == nums[i + 1U]) {
            const long long doubled = static_cast<long long>(nums[i]) * 2LL;
            if (doubled < std::numeric_limits<int>::min() ||
                doubled > std::numeric_limits<int>::max()) {
                throw std::overflow_error("merged value does not fit int");
            }
            nums[i] = static_cast<int>(doubled);
            nums[i + 1U] = 0;
        }
    }
    const auto logical_end = std::remove(nums.begin(), nums.end(), 0);
    std::fill(logical_end, nums.end(), 0);
    return nums;
}

struct RawEvent {
    std::string type;
    bool internal;
};

// 實務整合：過濾內部事件、正規化字串、壓縮相鄰重複；保留輸入不變。
std::vector<std::string> practical_public_event_pipeline(
    const std::vector<RawEvent>& input) {
    std::vector<RawEvent> public_events;
    std::copy_if(input.begin(), input.end(), std::back_inserter(public_events),
                 [](const RawEvent& event) { return !event.internal; });

    std::vector<std::string> names(public_events.size());
    std::transform(public_events.begin(), public_events.end(), names.begin(),
                   [](const RawEvent& event) {
                       std::string name = event.type;
                       std::transform(name.begin(), name.end(), name.begin(),
                                      [](unsigned char ch) {
                                          if (ch >= static_cast<unsigned char>('A') &&
                                              ch <= static_cast<unsigned char>('Z')) {
                                              return static_cast<char>(
                                                  ch - static_cast<unsigned char>('A') +
                                                  static_cast<unsigned char>('a'));
                                          }
                                          return static_cast<char>(ch);
                                      });
                       return name;
                   });
    names.erase(std::unique(names.begin(), names.end()), names.end());
    return names;
}

int main() {
    assert((leetcode_apply_operations({1, 2, 2, 1, 1, 0}) ==
            std::vector<int>{1, 4, 2, 0, 0, 0}));
    assert((leetcode_apply_operations({0, 1}) == std::vector<int>{1, 0}));
    bool overflow_rejected = false;
    try {
        static_cast<void>(leetcode_apply_operations(
            {std::numeric_limits<int>::max(), std::numeric_limits<int>::max()}));
    } catch (const std::overflow_error&) {
        overflow_rejected = true;
    }
    assert(overflow_rejected);

    const std::vector<RawEvent> input{{"LOGIN", false},
                                      {"debug", true},
                                      {"login", false},
                                      {"VIEW", false}};
    assert((practical_public_event_pipeline(input) ==
            std::vector<std::string>{"login", "view"}));
    assert(input.size() == 4U && input[0].type == "LOGIN");
    std::cout << "Modifying algorithms 整合複習完成\n";
}

/*
 * 九、面試 Q&A
 * Q: remove 為何不直接縮小 vector？
 * A: 演算法只收到 iterator，無法知道容器型別或呼叫其 erase。
 * Q: copy 與 move 如何選？
 * A: 來源仍要保留用 copy；所有權可消耗且型別 move 有收益才 move。
 * Q: transform 可否做 running sum？
 * A: 不應依賴 callable 執行順序；使用 partial_sum/inclusive_scan。
 * Q: reverse 與 rotate 差別？
 * A: reverse 顛倒每個元素順序；rotate 交換兩段位置且段內順序不變。
 * Q: unique 是否全域去重？
 * A: 否，只壓縮相鄰等價元素。
 * Q: 為何 generic code 寫 using std::swap; swap(a,b)？
 * A: 保留標準 fallback，同時允許 ADL 找到型別專屬高效 swap。
 *
 * 十、複雜度與選擇
 * 本章幾乎全是線性 O(N)，但常數與資料搬移成本不同。大型 move-only 元素、cache
 * locality、配置次數、predicate/comparator 成本都可能主導。先 reserve 可降低配置，
 * 但不能取代 resize。若只需 view，不要急著複製/反轉，ranges view 可能更合適。
 *
 * 十一、例外安全
 * 中途 assignment/generator/predicate 丟例外，多數演算法不 rollback，範圍可能部分
 * 修改。需要 all-or-nothing 時先寫到 staging container，成功後 noexcept swap 發布。
 * predicate 應盡量純函式；一邊遍歷一邊修改同容器結構通常使 iterator 失效。
 *
 * 十二、測試清單
 * 空、單值、全部符合/不符合、重複值、奇偶長度、重疊邊界、move-only 元素、
 * 目的容量不足的拒絕路徑、固定 seed 但不綁排列、例外中途狀態都要思考。
 *
 * 十三、練習
 * 1. 用 stable_partition 改寫 public event 過濾，比較是否值得修改輸入。
 * 2. 對 UTF-8 name 使用真正 Unicode case folding，而不是 ASCII-only lambda。
 * 3. 將 pipeline 改為 staging + swap，模擬 transform 中途丟例外。
 * 4. 實作 LC189 三次 reverse 與 std::rotate 版本，跑 property test 比較。
 * 5. 以 ranges::copy/filter_view 重寫唯讀管線，評估 lifetime 陷阱。
 *
 * 最後速記：先確認「改值還是改 size、保留來源還是消耗來源、目的是否有空間、
 * 是否重疊、是否要求穩定順序」，答案會直接決定該用哪個 modifying algorithm。
 * LeetCode 範例用 LC2460 串接 remove/fill；實務範例用事件匯出管線串接
 * copy_if/transform/unique，兩者都在 main 以可執行 assertion 驗證。
 */
