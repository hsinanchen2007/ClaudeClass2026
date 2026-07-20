/*
 * 第 18 章：tag dispatch
 *
 * tag 是沒有資料、只代表分類的型別。公開函式先推導 tag，再 overload 到不同實作。
 * 標準 iterator category 便是經典案例：random-access 可直接跳躍，forward iterator
 * 只能逐步前進。C++17 後很多情況可用 if constexpr，tag dispatch 仍常見於舊 API。
 */

/*
 * 【離線教材：模型與取捨】
 * 1. Tag 是通常不含資料的型別；建立 Tag{} 幾乎零成本，作用是參與 overload resolution。
 * 2. 公開 wrapper 負責萃取 iterator_category，實作 overload 則各自承擔可用操作的契約。
 * 3. iterator tag 形成繼承階層：random_access 是 bidirectional/forward/input 的更強能力。
 * 4. fallback 應接 input_iterator_tag，而非萬用 Category，才能讓衍生 tag 選到最強可行 overload。
 * 5. random-access 路徑使用 iterator+n，時間 O(1)；input/forward 路徑逐次 ++，時間 O(n)。
 * 6. 兩條路徑額外空間皆 O(1)，且 dispatch 本身沒有 virtual call 或 runtime type test。
 * 7. `advance_n` 沒有接收 end sentinel，前置條件是至少還有 count 個可前進位置。
 * 8. 超出有效範圍會形成無效 iterator，之後解參照可能是 UB；泛用 API 應另收 sentinel。
 * 9. count 也必須可表示成 iterator 的 difference_type，size_t 轉窄不是自動安全檢查。
 * 10. Tag dispatch 適合既有 overload 架構與可擴充分類；單一函式少量分支可選 if constexpr。
 * 11. 若策略由設定檔在 runtime 決定，應用 enum/variant/virtual，而不是為每個值實體化模板。
 * 12. 每組 Iterator 與 DeliveryTag 會產生 specialization，可能增加 code size 與 compile time。
 * 13. 空 tag 本身不保存資源，沒有生命週期負擔；iterator 仍只借用其來源 container。
 * 14. container 一旦銷毀或依規則使 iterator 失效，dispatch 回傳的 iterator 也立即失效。
 * 15. LeetCode middle 使用快慢 iterator，時間 O(n)、空間 O(1)，偶數長度回傳第二個中點。
 * 16. 本例回 optional<value_type> 以涵蓋空 range，並複製結果，避免把 iterator/reference 帶出去。
 * 17. 因而 LeetCode 泛型契約還要求 value_type 可複製；原題 ListNode* 則由串列管理生命週期。
 * 18. practical_send 按值取得 message 所有權，再 move 給實作；回傳 string 自己擁有內容。
 * 19. 診斷若選錯 overload，先檢查 category 的繼承與轉換排名，不要只看 tag 名稱相似度。
 * 20. 面試追問：tag dispatch 和 policy template 差在哪？前者靠 overload 分派，後者常把行為封裝成型別。
 * 21. 面試追問：為何不直接 std::advance？正式程式應優先標準演算法，本例用來拆解其經典機制。
 */

#include <cassert>
#include <forward_list>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

template <typename Iterator>
Iterator advance_n_impl(Iterator iterator, std::size_t count, std::random_access_iterator_tag) {
    return iterator + static_cast<typename std::iterator_traits<Iterator>::difference_type>(count);
}

template <typename Iterator>
Iterator advance_n_impl(Iterator iterator, std::size_t count, std::input_iterator_tag) {
    while (count > 0U) {
        ++iterator;
        --count;
    }
    return iterator;
}

template <typename Iterator>
Iterator advance_n(Iterator iterator, std::size_t count) {
    using Category = typename std::iterator_traits<Iterator>::iterator_category;
    return advance_n_impl(iterator, count, Category{});
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 876. Middle of the Linked List（鏈結串列的中間節點）
// 題目：找串列中點，偶數長度回第二個中點；[1,2,3,4,5,6] 得 4，空泛化輸入回無值。
// 為何使用本章主題：本泛型版讀取 iterator_category tag 並驗證至少是 forward iterator，
// 但沒有依 tag overload 分派；它是相鄰的 category 教學改寫，且回值而非原題節點指標。
// 思路：空 range 回 nullopt；slow/fast 同起點；fast 每輪兩步、slow 一步，最後複製 slow 值。
// 複雜度：時間 O(N)、額外空間 O(1)，N 是元素數；optional 內另保存一份中間元素。
// 易錯點：需要可多趟走訪的 forward iterator；value_type 必須可複製，偶數長度取第二個中點。
// -----------------------------------------------------------------------------
template <typename ForwardRange>
auto leetcode_middle(const ForwardRange& values)
    -> std::optional<typename ForwardRange::value_type> {
    using Iterator = decltype(values.begin());
    using Category = typename std::iterator_traits<Iterator>::iterator_category;
    static_assert(std::is_base_of_v<std::forward_iterator_tag, Category>,
                  "middle 演算法需要可多趟走訪的 forward iterator");

    auto slow = values.begin();
    auto fast = values.begin();
    const auto end = values.end();
    if (fast == end) {
        return std::nullopt;
    }
    while (fast != end) {
        ++fast;
        if (fast == end) {
            break;
        }
        ++fast;
        ++slow;
    }
    return *slow;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】訊息立即送出或緩衝排隊
// 情境：同一訊息 API 依編譯期部署模式回報 queued 或 sent，不在熱路徑做 runtime policy 判斷。
// 為何使用本章主題：BufferedTag/ImmediateTag 由 overload resolution 選 send_impl，公開模板只建立 tag；
// 相較 virtual dispatch 沒有間接呼叫，但部署時不能對同一物件動態切換策略。
// 設計：兩個 tag 各對應一個 impl；practical_send 按值取得 message；move 給選中的實作並加狀態前綴。
// 成本：分派 O(1) 且通常完全消除，字串建構 O(L)，L 是訊息長度。
// 上線注意：範例只回字串，真正 buffered 路徑需處理容量、持久化與失敗；訊息內容也要避免洩密。
// -----------------------------------------------------------------------------
struct BufferedTag {};
struct ImmediateTag {};

std::string send_impl(std::string message, BufferedTag) {
    return "queued:" + std::move(message);
}

std::string send_impl(std::string message, ImmediateTag) {
    return "sent:" + std::move(message);
}

template <typename DeliveryTag>
std::string practical_send(std::string message) {
    return send_impl(std::move(message), DeliveryTag{});
}

int main() {
    const std::vector<int> vector_values{10, 20, 30, 40};
    assert(advance_n(vector_values.begin(), 0U) == vector_values.begin());
    assert(*advance_n(vector_values.begin(), 2U) == 30);

    const std::forward_list<int> list_values{1, 2, 3, 4, 5, 6};
    assert(*advance_n(list_values.begin(), 3U) == 4);
    assert(leetcode_middle(list_values) == 4);
    const std::forward_list<int> odd_values{1, 2, 3, 4, 5};
    assert(leetcode_middle(odd_values) == 3);
    const std::forward_list<int> empty_values;
    assert(!leetcode_middle(empty_values).has_value());

    assert(practical_send<BufferedTag>("audit") == "queued:audit");
    assert(practical_send<ImmediateTag>("alert") == "sent:alert");
    assert(practical_send<ImmediateTag>("") == "sent:");

    std::cout << "tag dispatch 測試完成\n";
}

/*
 * 【複雜度】vector advance O(1)，forward_list advance O(n)；介面相同但成本不同。
 * 【陷阱】input_iterator_tag 等類別有繼承關係；overload 設計不當可能選到非預期版本。
 * 【面試】tag dispatch 與 virtual dispatch 差異？前者編譯期選 overload，後者執行期選 virtual。
 * 【練習】為 contiguous iterator 加專屬 tag 路徑，並解釋它與 random-access 的關係。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '18_tag_dispatch.cpp' -o '/tmp/codex_cpp_C_Template_18_tag_dispatch' && '/tmp/codex_cpp_C_Template_18_tag_dispatch'
//
// === 預期輸出（節錄）===
// tag dispatch 測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
