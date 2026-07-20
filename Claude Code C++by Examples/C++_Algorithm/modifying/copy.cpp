// ============================================================
// std::copy / std::copy_if / std::copy_n / std::copy_backward
// 分類 (Category): Modifying sequence operations (修改型)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/copy
//   * https://en.cppreference.com/w/cpp/algorithm/copy_n
//   * https://en.cppreference.com/w/cpp/algorithm/copy_backward
//   * https://cplusplus.com/reference/algorithm/copy/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::copy 系列要解決的問題就是「把一段資料搬到另一個地方」 —
// 這是程式設計裡最基礎、也是被頻繁呼叫的動作。STL 把它們整理成
// 四個「家族成員」,各自處理不同情境:
//
//   ┌──────────────────┬──────────────────────────────────────┐
//   │ copy             │ 全部複製 (來源範圍 → 目的端)         │
//   │ copy_if          │ 過濾 + 複製 (符合述詞才複製) C++11   │
//   │ copy_n           │ 複製前 N 個 (用個數而非 end iterator)│
//   │ copy_backward    │ 由後往前複製 (處理重疊區段向後位移) │
//   └──────────────────┴──────────────────────────────────────┘
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼有 copy_backward? (重疊區段的方向問題)          │
// └────────────────────────────────────────────────────────────┘
//
// 當「來源」和「目的」位於同一個容器,而且兩者「重疊」時,
// 複製方向選錯就會出問題。看這個例子:
//
//   原:  [A, B, C, D, E, _, _]    要把前 5 個搬到 index 2 位置
//   想要結果: [A, B, A, B, C, D, E]
//
// 如果用 std::copy 從前往後寫:
//   step1: dst[2] = src[0] → [A, B, A, D, E, _, _]   (D 被 A 蓋掉了!)
//   step2: dst[3] = src[1] → [A, B, A, B, E, _, _]   (E 被 B 蓋掉了!)
//   ...結果錯
//
// 這時候要用 copy_backward — 從尾巴往前寫:
//   step1: dst[6] = src[4] → [A, B, C, D, E, _, E]
//   step2: dst[5] = src[3] → [A, B, C, D, E, D, E]
//   ...一路往前都不會誤覆蓋
//
// 「前移用 copy,後移用 copy_backward」這個口訣要記住。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、目的端的容量誰負責?                                   │
// └────────────────────────────────────────────────────────────┘
//
// std::copy 不會幫你「擴大」目的端容量 — 你要先確保 dst 有足夠空間,
// 否則就是踩記憶體 (UB)。常見兩種寫法:
//
//   1. 預先 resize:   dst.resize(src.size()); std::copy(...);
//   2. 用插入迭代器: std::copy(..., std::back_inserter(dst));
//
// `back_inserter` 把每次 `*it = value` 翻譯成 `dst.push_back(value)`,
// dst 自動長大。配合 std::copy 是最常見的「拷貝到 vector 末尾」寫法。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class InputIt, class OutputIt>
//   OutputIt copy(InputIt first, InputIt last, OutputIt d_first);
//
//   template <class InputIt, class OutputIt, class UnaryPred>
//   OutputIt copy_if(InputIt first, InputIt last, OutputIt d_first, UnaryPred p);
//
//   template <class InputIt, class Size, class OutputIt>
//   OutputIt copy_n(InputIt first, Size n, OutputIt d_first);
//
//   template <class BidirIt1, class BidirIt2>
//   BidirIt2 copy_backward(BidirIt1 first, BidirIt1 last, BidirIt2 d_last);
//
//   * C++20 起為 constexpr。
//   * C++17 起有執行策略多載。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、回傳值 (Return value)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   * copy / copy_if / copy_n  → d_first 加上「實際寫入個數」的迭代器
//   * copy_backward            → d_last 減去複製長度 (即「最前面」寫入位置)
//
// 回傳值常用於串接 — 例如 copy_if 後得到 it,可以用 it 做為下一次寫入的起點。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間: O(n) — 線性次數的指派
//   空間: O(1)
//   對 trivially copyable 型別,實作通常會 fall back 到 memmove,
//   執行速度極快。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 目的端容量必須足夠,或用 back_inserter。
//   2. 重疊區段:前移 → copy;後移 → copy_backward。
//   3. copy_if 不會「移除」原元素,它只是把符合的拷貝出去 —
//      要原地過濾請用 std::remove_if。
//   4. copy_n 對 n <= 0 不寫入,直接回傳 d_first。
//
// ============================================================

/*
補充筆記：std::copy
  - std::copy 假設目的地範圍有足夠空間；如果目的容器是空的，應使用 back_inserter。
  - 來源與目的重疊時 copy 不一定安全；往右重疊通常應使用 copy_backward。
  - copy 是值複製；若元素拷貝成本高，請確認是否應改用 move 或保存指標/handle。
  - std::copy 會改變目標範圍內容或元素順序；呼叫前要確認輸出範圍空間足夠、iterator 有效。
  - copy/transform/generate/fill 寫入目的地時不會自動擴容，除非你使用 back_inserter 這類 insert iterator。
  - remove/unique 不會真的縮短容器，只把保留元素移到前面並回傳新邏輯結尾；還要搭配 erase 才會改變 size。
  - reverse/rotate/swap_ranges 會改變元素位置；若外部保存索引或 iterator，要重新確認是否仍指向預期元素。
  - move algorithm 會把元素搬到目的地，來源仍有效但值可能改變；後續只能重新指定或安全銷毀。
  - shuffle/sample 需要亂數引擎；不要每次呼叫都用同一個固定種子，除非你刻意要可重現測試結果。
*/

// ===========================================================================
// 【面試題】std::copy / copy_if / copy_n / copy_backward
// ---------------------------------------------------------------------------
// 🔥 Q1. std::copy 到一個空的 vector 會發生什麼事?
//     答:UB(buffer overrun)。copy 只會往 output iterator 寫,它不知道也碰不到容器,
//         不可能幫你 resize。空 vector 的 begin() == end(),寫入第一個元素就已越界。
//         正解:先 dst.resize(src.size())、或用 std::back_inserter(dst)、
//         或直接 dst.assign(src.begin(), src.end()) / dst = src。
//     追問:那我先 reserve() 不就有空間了嗎?
//
// ⚠️ 陷阱. reserve() 為什麼不夠?
//     答:reserve() 只增加 capacity,size() 仍然是 0,begin() 到 end() 仍是空區間。
//         std::copy 是透過 iterator 寫入既有元素,不是 push_back,所以照樣越界。
//         要用 resize()(真的建構出元素)或改用 insert iterator。
//     為什麼會錯:多數人把「有記憶體」等同於「有元素」。capacity 是配置好的原始空間,
//         size 才是已建構元素的個數;iterator 區間由 size 決定,與 capacity 無關。
//
// 🔥 Q2. 為什麼需要 copy_backward?重疊區間的規則是什麼?
//     答:std::copy 由前往後寫,要求 d_first 不在 [first, last) 之內,否則還沒讀到的
//         來源元素會先被覆蓋。當來源與目的重疊「且目的在來源後方」時,必須改用
//         copy_backward 由後往前寫(它反過來要求 d_last 不在 (first, last] 內)。
//         口訣:往前搬用 copy,往後搬用 copy_backward。
//
// Q3. copy_n 和 copy 差在哪?什麼時候一定得用 copy_n?
//     答:copy 用 [first, last) 兩個 iterator 描述範圍;copy_n 用「起點 + 個數」。
//         當你只有起點和數量、拿不到 last(例如來源是 input iterator/串流),或是
//         只想搬前 N 個時就用 copy_n。copy_if 則是 C++11 才加入的過濾版。
// ===========================================================================

#include <algorithm>
#include <iostream>
#include <iterator>   // back_inserter, ostream_iterator
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> src{1, 2, 3, 4, 5};

    // --- 範例 1: copy 到既有大小的目的容器 ---
    std::vector<int> dst1(5);
    std::copy(src.begin(), src.end(), dst1.begin());
    std::cout << "copy:        ";
    for (int x : dst1) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 2: copy 配合 back_inserter (動態加入) ---
    std::vector<int> dst2;
    std::copy(src.begin(), src.end(), std::back_inserter(dst2));
    std::cout << "back_insert: ";
    std::copy(dst2.begin(), dst2.end(),
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << '\n';

    // --- 範例 3: copy_if — 只拷貝偶數 (過濾 + 拷貝) ---
    std::vector<int> evens;
    std::copy_if(src.begin(), src.end(), std::back_inserter(evens),
                 [](int x){ return x % 2 == 0; });
    std::cout << "copy_if(even): ";
    for (int x : evens) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 4: copy_n — 拷貝前 3 個 ---
    std::vector<int> first3(3);
    std::copy_n(src.begin(), 3, first3.begin());
    std::cout << "copy_n(3):   ";
    for (int x : first3) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 5: copy_backward — 在同容器中向後位移 (避免覆蓋) ---
    //  原: 1 2 3 4 5 _ _   →   1 2 1 2 3 4 5  (前 5 個被搬到尾部)
    std::vector<int> v{1, 2, 3, 4, 5, 0, 0};
    std::copy_backward(v.begin(), v.begin() + 5, v.end());
    std::cout << "copy_backward: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_88_merge_sorted_array();
    void practical_slice_backup();
    void leetcode_1089_duplicate_zeros();
    void practical_clone_config_object();
    leetcode_88_merge_sorted_array();
    practical_slice_backup();
    leetcode_1089_duplicate_zeros();
    practical_clone_config_object();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 88: 合併兩個有序陣列 (Merge Sorted Array)
// ----------------------------------------------------------------
// 題目:nums1 大小為 m+n,前 m 個是有效資料;nums2 大小為 n。
//      把 nums2 合併進 nums1,使整體保持非遞減排序 (in-place)。
//
// 為什麼用 std::copy:
//   題目給 nums1 後段預留了 n 個空位 — 把 nums2 完整 copy 過去
//   是最直觀的步驟,然後排序整體即可。
//
// 解法步驟:
//   1. std::copy(nums2 全部, nums1 從 index m 開始)。
//   2. std::sort 整個 nums1。
//
// 複雜度:時間 O((m+n) log(m+n));空間 O(1)。
//        (進階解法可雙指針從尾合併,O(m+n);這裡示範 std::copy。)
void leetcode_88_merge_sorted_array() {
    std::vector<int> nums1{1, 2, 3, 0, 0, 0};
    std::vector<int> nums2{2, 5, 6};
    int m = 3, n = 3;
    std::copy(nums2.begin(), nums2.end(), nums1.begin() + m);
    std::sort(nums1.begin(), nums1.end());
    std::cout << "LC88: ";
    for (int x : nums1) std::cout << x << ' ';
    std::cout << '\n';
    (void)n;
}

// ----------------------------------------------------------------
// 實務範例:備份某段切片 (slice backup)
// ----------------------------------------------------------------
// 場景:在做高風險批次運算前,先備份原始資料的某段區間,
//      以便發生錯誤時可快速還原。
//
// 為什麼用 std::copy + back_inserter:
//   不必預先計算切片長度、也不必預 resize backup,程式碼最簡潔。
void practical_slice_backup() {
    std::vector<int> data{10, 20, 30, 40, 50, 60, 70, 80};
    std::vector<int> backup;
    std::copy(data.begin() + 2, data.begin() + 6, std::back_inserter(backup));
    std::cout << "Backup slice: ";
    for (int x : backup) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1089: 複寫零 (Duplicate Zeros)
// ----------------------------------------------------------------
// 題目:給長度固定的陣列 arr,把每個 0 「複製一份」,其後元素往右擠,
//      超出長度的元素直接丟棄。原地修改。
//
// 為什麼用 std::copy_backward:
//   經典「向後位移」場景 — 把 [i..end) 整段往後挪一格再寫入第二個 0。
//   copy_backward 是處理「向右移動,避免覆蓋」的標準工具。
//
// 複雜度:時間 O(n²) (最壞每個 0 都觸發位移);空間 O(1)。
void leetcode_1089_duplicate_zeros() {
    std::vector<int> arr{1, 0, 2, 3, 0, 4, 5, 0};
    int n = arr.size();
    for (int i = 0; i < n; ++i) {
        if (arr[i] == 0 && i + 1 < n) {
            // 從 i+1 ~ end-1 向後位移一格
            std::copy_backward(arr.begin() + i + 1,
                               arr.begin() + n - 1,
                               arr.begin() + n);
            arr[i + 1] = 0;
            ++i;   // 跳過剛剛新增的 0
        }
    }
    std::cout << "LC1089:";
    for (int x : arr) std::cout << ' ' << x;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:深拷貝 (clone) 設定物件
// ----------------------------------------------------------------
// 場景:服務啟動時讀入「預設設定」,執行時若使用者要客製,
//      先 deep copy 一份再修改 — 不污染原始預設。
//      std::copy 對 POD struct 陣列就是 element-wise copy。
void practical_clone_config_object() {
    struct Setting { int key; int value; };
    std::vector<Setting> defaults{{1, 100}, {2, 200}, {3, 300}};
    std::vector<Setting> user_copy;
    std::copy(defaults.begin(), defaults.end(), std::back_inserter(user_copy));
    user_copy[1].value = 999;   // 改使用者副本
    std::cout << "default[1]=" << defaults[1].value
              << " user[1]=" << user_copy[1].value << '\n';
}

// 編譯: g++ -std=c++20 -Wall -Wextra copy.cpp -o copy

// === 預期輸出 ===
// copy:        1 2 3 4 5
// back_insert: 1 2 3 4 5
// copy_if(even): 2 4
// copy_n(3):   1 2 3
// copy_backward: 1 2 1 2 3 4 5
// LC88: 1 2 2 3 5 6
// Backup slice: 30 40 50 60
// LC1089: 1 0 0 2 3 0 0 4
// default[1]=200 user[1]=999
