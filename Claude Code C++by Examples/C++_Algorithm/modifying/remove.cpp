// ============================================================
// std::remove / std::remove_if / std::remove_copy / std::remove_copy_if
// 分類 (Category): Modifying sequence operations (修改型)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/remove
//   * https://cplusplus.com/reference/algorithm/remove/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 — 學 STL 一定要懂的「最大誤解」                │
// └────────────────────────────────────────────────────────────┘
//
// std::remove **不會真的把元素從容器中刪除!**
//
// 這是 C++ STL 中「最容易誤會」的設計之一。標準函式庫的 algorithm
// 不持有容器,只能透過迭代器「看到」元素 — 它根本不知道也無法去
// 改變容器大小。
//
// std::remove 實際做的是:
//
//   1. 從頭掃描範圍,把「想保留的元素」往前搬。
//   2. 回傳一個迭代器,指向「保留段的結尾的下一個位置」。
//   3. 「結尾之後」的元素是「廢值 (residue)」 — 標準說它們處於
//      「valid but unspecified」狀態,通常是被 swap/move 過去的舊值。
//   4. 容器的 size() 不變!
//
// 範例:
//
//   原本 v = {1, 2, 3, 2, 5, 2, 7}
//   呼叫 std::remove(v.begin(), v.end(), 2)
//   結果 v = {1, 3, 5, 7, ?, ?, ?}     ← 前 4 個是答案,後 3 個是廢值
//   回傳的 it 指向 v[4]                ← 這是「邏輯結尾」
//
// 想真的把後段廢值刪掉,必須再呼叫容器的 erase:
//
//   v.erase(it, v.end());
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、erase-remove idiom — 一定要記住的慣用法                 │
// └────────────────────────────────────────────────────────────┘
//
//   v.erase(std::remove(v.begin(), v.end(), value), v.end());
//   v.erase(std::remove_if(v.begin(), v.end(), pred), v.end());
//
// 這是 C++ 幾十年來最經典的「移除元素」寫法。每個 C++ 工程師都該背下來。
//
// C++20 起新增了 std::erase / std::erase_if,可一行解決:
//
//   std::erase(v, value);
//   std::erase_if(v, pred);
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、為什麼設計成這樣?                                     │
// └────────────────────────────────────────────────────────────┘
//
// 因為 STL 演算法是「容器無關」的 — 只透過迭代器運作,不知道容器型別。
// vector、deque、list、原生陣列、string ... 各有不同的「erase」方法,
// 所以 algorithm 只能負責「重排」,「真正刪除」交給容器自己。
//
// 這個設計讓 std::remove 對「原生陣列」也能用 (反正陣列也沒得 erase),
// 但對 vector 這種有大小的容器就需要使用者自己補一刀 erase。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、四個成員的差別                                         │
// └────────────────────────────────────────────────────────────┘
//
//   ┌──────────────────────┬───────────────────────────────────┐
//   │ remove               │ 原地搬移,移除「等於 value」      │
//   │ remove_if            │ 原地搬移,移除「述詞為 true」      │
//   │ remove_copy          │ 拷貝過濾結果到輸出,原範圍不變    │
//   │ remove_copy_if       │ 拷貝過濾結果到輸出,述詞版         │
//   └──────────────────────┴───────────────────────────────────┘
//
// _copy 版本不會改原範圍,適合「想保留原資料 + 同時要過濾後的版本」。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class FwdIt, class T>
//   FwdIt remove(FwdIt first, FwdIt last, const T& value);
//
//   template <class FwdIt, class UnaryPred>
//   FwdIt remove_if(FwdIt first, FwdIt last, UnaryPred p);
//
//   template <class InputIt, class OutputIt, class T>
//   OutputIt remove_copy(InputIt first, InputIt last, OutputIt d_first,
//                        const T& value);
//
//   template <class InputIt, class OutputIt, class UnaryPred>
//   OutputIt remove_copy_if(InputIt first, InputIt last, OutputIt d_first,
//                           UnaryPred p);
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間: O(N) 次比較或述詞呼叫;搬移亦 O(N)。
//   空間: O(1) (in-place 版);_copy 版另需 O(N) 輸出空間。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 千萬記得:remove 不會改容器大小!想真刪除要配合 erase。
//   2. 對 std::list,有更好的成員 list::remove() (O(N) 但無 swap)。
//   3. 對 std::set / std::map,這些 algorithm 不適用 — 用容器自己的 erase。
//   4. C++20 的 std::erase / std::erase_if 是更現代的寫法。
//
// ============================================================

/*
補充筆記：std::remove
  - std::remove 不會刪除容器元素；它把要保留的元素往前搬，回傳新的 logical end。
  - erase-remove idiom 的 erase 才會真正縮小 vector/string/deque 的 size。
  - remove 後 new_end 到 old end 之間的值仍存在但內容不應依賴，通常立刻 erase。
  - std::remove 會改變目標範圍內容或元素順序；呼叫前要確認輸出範圍空間足夠、iterator 有效。
  - copy/transform/generate/fill 寫入目的地時不會自動擴容，除非你使用 back_inserter 這類 insert iterator。
  - remove/unique 不會真的縮短容器，只把保留元素移到前面並回傳新邏輯結尾；還要搭配 erase 才會改變 size。
  - reverse/rotate/swap_ranges 會改變元素位置；若外部保存索引或 iterator，要重新確認是否仍指向預期元素。
  - move algorithm 會把元素搬到目的地，來源仍有效但值可能改變；後續只能重新指定或安全銷毀。
  - shuffle/sample 需要亂數引擎；不要每次呼叫都用同一個固定種子，除非你刻意要可重現測試結果。
*/
#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> v{1, 2, 3, 2, 5, 2, 7};

    // --- 範例 1: erase-remove idiom (最常見寫法,務必背下來) ---
    auto new_end = std::remove(v.begin(), v.end(), 2);
    std::cout << "size before erase = " << v.size()
              << ", logical size = " << (new_end - v.begin()) << '\n';
    v.erase(new_end, v.end());
    std::cout << "after erase: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 2: remove_if + erase (述詞版) ---
    std::vector<int> w{1, 2, 3, 4, 5, 6};
    w.erase(std::remove_if(w.begin(), w.end(),
                           [](int x){ return x % 2 == 0; }),
            w.end());
    std::cout << "removed evens: ";
    for (int x : w) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 3: remove_copy (原範圍不變) ---
    std::vector<int> a{1, 2, 3, 2, 4};
    std::vector<int> b;
    std::remove_copy(a.begin(), a.end(), std::back_inserter(b), 2);
    std::cout << "remove_copy: ";
    for (int x : b) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 4: 不 erase 直接觀察「廢值」殘留現象 ---
    std::vector<int> c{1, 2, 3, 2};
    auto e = std::remove(c.begin(), c.end(), 2);
    std::cout << "without erase, full vec: ";
    for (int x : c) std::cout << x << ' ';
    std::cout << "(logical end at index " << (e - c.begin()) << ")\n";

    // === LeetCode / 實務範例 ===
    void leetcode_27_remove_element();
    void leetcode_283_move_zeroes();
    void practical_filter_invalid_orders();
    void leetcode_1346_remove_doubles_concept();
    void practical_drop_blank_lines();
    leetcode_27_remove_element();
    leetcode_283_move_zeroes();
    practical_filter_invalid_orders();
    leetcode_1346_remove_doubles_concept();
    practical_drop_blank_lines();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 27: 移除元素 (Remove Element)
// ----------------------------------------------------------------
// 題目:給陣列 nums 與整數 val,原地移除所有等於 val 的元素,
//      回傳新長度 k。題目只要求「前 k 個位置」是保留結果。
//
// 為什麼用 std::remove:
//   題目「前 k 個是答案,後段不在乎」的設計就是為了 std::remove —
//   remove 的行為剛好就是「把保留的元素往前搬,回傳邏輯結尾」。
//
// 解法步驟:
//   1. std::remove(nums.begin(), nums.end(), val) 取得新邏輯結尾 it。
//   2. k = it - begin。題目不要求真刪除,直接回傳 k。
//
// 複雜度:時間 O(n),空間 O(1)。
void leetcode_27_remove_element() {
    std::vector<int> nums{3, 2, 2, 3};
    int val = 3;
    auto new_end = std::remove(nums.begin(), nums.end(), val);
    int k = static_cast<int>(new_end - nums.begin());
    std::cout << "LC27: k=" << k << ", nums[0..k)=";
    for (auto it = nums.begin(); it != new_end; ++it) std::cout << *it << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 283: 移動零 (Move Zeroes)
// ----------------------------------------------------------------
// 題目:給陣列 nums,原地將所有 0 搬到最後,且其餘非零元素「保持原相對順序」。
//
// 為什麼用 std::remove + std::fill:
//   * std::remove 的核心動作就是「保留元素前移,且保留原順序」 —
//     正好是題目要求。
//   * 移除後,原本的「零」沒有真的消失,只是被當廢值留在尾段。
//     再用 std::fill 把尾段全填 0 即可。
//
// 解法步驟:
//   1. it = std::remove(nums, 0)。  → 前段是非零、原順序保留。
//   2. std::fill(it, end, 0)。      → 把尾段填回 0。
//
// 複雜度:時間 O(n),空間 O(1)。
void leetcode_283_move_zeroes() {
    std::vector<int> nums{0, 1, 0, 3, 12};
    auto new_end = std::remove(nums.begin(), nums.end(), 0);
    std::fill(new_end, nums.end(), 0);
    std::cout << "LC283: ";
    for (int x : nums) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:過濾掉「無效訂單」(amount <= 0)
// ----------------------------------------------------------------
// 場景:後端拿到一份訂單金額清單,要清掉所有 <= 0 的無效訂單。
//
// 為什麼用 erase + remove_if 慣用法:
//   一行表達「過濾並真正刪除」,是 C++ 最經典的寫法。
void practical_filter_invalid_orders() {
    std::vector<int> amounts{120, 0, 35, -10, 50, 0, 200};
    amounts.erase(
        std::remove_if(amounts.begin(), amounts.end(),
                       [](int x){ return x <= 0; }),
        amounts.end());
    std::cout << "Valid orders: ";
    for (int x : amounts) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1346 概念:刪除「其值的兩倍也存在」的元素
// ----------------------------------------------------------------
// 題目簡化:給陣列 arr,移除所有「2*x 也存在於 arr」的 x。
//          這裡示範 remove_if + lambda 的用法。
//
// 為什麼用 std::remove_if:
//   過濾條件牽涉「整個陣列」的查詢時,把它包進 lambda;
//   remove_if 把不符合條件 (即「不被刪除」) 的元素往前搬。
//
// 複雜度:時間 O(n²) (因 lambda 內查詢);空間 O(1)。
void leetcode_1346_remove_doubles_concept() {
    std::vector<int> arr{2, 4, 5, 8, 10};
    auto end_it = std::remove_if(arr.begin(), arr.end(), [&](int x) {
        // 若 2x 也在陣列中,標記為要移除
        return std::find(arr.begin(), arr.end(), 2 * x) != arr.end()
            && x != 0;   // 避免 0 自己對自己
    });
    arr.erase(end_it, arr.end());
    std::cout << "LC1346-rm:";
    for (int x : arr) std::cout << ' ' << x;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:刪除文字檔的「空白行」
// ----------------------------------------------------------------
// 場景:讀入一份檔案的 vector<string> 表示每一行,要過濾掉空白行
//      (空字串或只有 whitespace)。
//      erase-remove_if 慣用法,一行表達。
void practical_drop_blank_lines() {
    std::vector<std::string> lines{
        "hello", "", "world", "   ", "foo", "bar"
    };
    lines.erase(
        std::remove_if(lines.begin(), lines.end(), [](const std::string& s) {
            for (char c : s) if (c != ' ' && c != '\t') return false;
            return true;   // 整行只有 whitespace
        }),
        lines.end());
    std::cout << "kept lines:";
    for (auto& s : lines) std::cout << " [" << s << "]";
    std::cout << '\n';
}

// === 預期輸出 (Expected output) ===
// size before erase = 7, logical size = 4
// after erase: 1 3 5 7
// removed evens: 1 3 5
// remove_copy: 1 3 4
// without erase, full vec: 1 3 ? ? (logical end at index 2)
//   (注意 "?" 處的值由實作決定,通常是被 swap 過去的舊值)
// LC27: k=2, nums[0..k)=2 2
// LC283: 1 3 12 0 0
// Valid orders: 120 35 50 200
// LC1346-rm: 8 10
// kept lines: [hello] [world] [foo] [bar]
