// ============================================================
// std::swap
// 分類 (Category): Modifying / Utilities (修改型 / 通用工具)
// 標頭檔 (Header):  <utility> (主要), <algorithm> (轉介)
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/swap
//   * https://en.cppreference.com/w/cpp/utility/swap
//   * https://cplusplus.com/reference/utility/swap/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::swap 解的問題很基本:
//
//   「把兩個物件的值互換。」
//
// 預設實作概念上是這樣 (C++11 起會走 move):
//
//   T tmp = std::move(a);
//   a     = std::move(b);
//   b     = std::move(tmp);
//
// 也就是「3 次 move」 — 對基本型別等於 3 次拷貝。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、容器特化 — STL 最大的「O(1) swap」優勢                  │
// └────────────────────────────────────────────────────────────┘
//
// 對 STL 容器 (vector、string、map ...),std::swap 有「特化」 —
// 只交換內部三個指標 (data、size、capacity),完全不碰元素。
//
//   std::vector<int> a(1'000'000), b(2'000'000);
//   std::swap(a, b);   // O(1)!不是 O(N)。
//
// 因此「double-buffer」、「rotate buffers」這類設計,
// 容器 swap 是極度高效的選擇。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、ADL (Argument-Dependent Lookup) 與「using std::swap」 │
// └────────────────────────────────────────────────────────────┘
//
// 寫泛型程式時,要交換兩個物件,標準慣用法是:
//
//   using std::swap;   // 引入 std::swap 作為候選者
//   swap(a, b);        // 依 ADL,優先找 a/b 的 namespace 中的 swap;
//                      // 找不到才 fallback 到 std::swap
//
// 為什麼?因為很多型別 (例如自訂類別) 提供「friend swap」 —
// 它通常更快。透過 ADL 找到自訂版本就能享用這份效能。
//
// 切記:不要寫 `std::swap(a, b);` — 這會「寫死」用 std::swap,
// 跳過 ADL 找不到使用者的客製化版本。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、自訂類別的 swap 實作 (custom swap idiom)               │
// └────────────────────────────────────────────────────────────┘
//
// 為自訂類別提供 swap 的標準方式是「friend in-class」:
//
//   struct T {
//       Foo a; Bar b;
//       friend void swap(T& x, T& y) noexcept {
//           using std::swap;
//           swap(x.a, y.a);
//           swap(x.b, y.b);
//       }
//   };
//
// 這樣寫的好處:
//   * ADL 能找到。
//   * 標記 noexcept,讓「copy-and-swap idiom」更安全。
//   * 內部用 ADL 對成員 swap,各成員都能用最佳實作。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   // 通用版 (C++11 起 noexcept-aware)
//   template <class T>
//   void swap(T& a, T& b);
//
//   // 陣列版 (C++11 起,逐元素 swap)
//   template <class T2, std::size_t N>
//   void swap(T2 (&a)[N], T2 (&b)[N]);
//
//   * C++20 起為 constexpr。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   通用版:   3 次 move-assignment
//   容器特化: O(1) — 僅交換內部指標
//   陣列版:   O(N)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 寫泛型程式請用「using std::swap; swap(a, b);」慣用法。
//   2. 對自訂類別要提供 friend swap 並標 noexcept。
//   3. 自交換 (swap(a, a)) 對某些自訂類型可能 UB,小心。
//   4. 想交換「迭代器指向」的內容請用 std::iter_swap。
//
// ============================================================

/*
補充筆記：std::swap
  - swap 交換兩個物件的狀態；對容器通常是常數時間交換內部資源。
  - 寫泛型程式時常先 using std::swap，再呼叫 swap(a,b) 讓 ADL 找自訂版本。
  - swap 是否 noexcept 會影響某些容器與演算法的例外安全策略。
  - std::swap 會改變目標範圍內容或元素順序；呼叫前要確認輸出範圍空間足夠、iterator 有效。
  - copy/transform/generate/fill 寫入目的地時不會自動擴容，除非你使用 back_inserter 這類 insert iterator。
  - remove/unique 不會真的縮短容器，只把保留元素移到前面並回傳新邏輯結尾；還要搭配 erase 才會改變 size。
  - reverse/rotate/swap_ranges 會改變元素位置；若外部保存索引或 iterator，要重新確認是否仍指向預期元素。
  - move algorithm 會把元素搬到目的地，來源仍有效但值可能改變；後續只能重新指定或安全銷毀。
  - shuffle/sample 需要亂數引擎；不要每次呼叫都用同一個固定種子，除非你刻意要可重現測試結果。
*/

// ===========================================================================
// 【面試題】std::swap
// ---------------------------------------------------------------------------
// 🔥 Q1. 標準容器的 swap 複雜度是多少?
//     答:O(1)。因為只交換內部的指標、size、capacity 等成員,元素本身完全不動。
//         唯一的例外是 std::array——它的元素實際存在物件內部,沒有指標可換,
//         所以是 O(n),逐元素 swap。
//     追問:那 std::array 為什麼不能做到 O(1)?
//
// 🔥 Q2. 容器 swap 之後,原本的 iterator / reference 會失效嗎?
//     答:不會失效,但「換了主人」。C++11 明確規定 swap 不會使 iterator、reference、
//         pointer 失效,它們仍指向原本那些元素——只是那些元素現在屬於另一個容器了。
//         例外是 end() iterator,它不保證有效。這也是 swap trick
//         (用 vector<T>(v).swap(v) 收縮 capacity)能成立的基礎。
//
// Q3. 通用版 std::swap 是怎麼實作的?成本多少?
//     答:C++11 起用 move:一次 move construction 加兩次 move assignment,共 3 次移動,
//         並且是 noexcept-aware(能從型別的 move 操作推導出 noexcept)。
//         C++11 之前用 copy,對重型物件昂貴得多。
//
// ⚠️ 陷阱. 自訂型別要怎麼寫才能被泛型程式碼正確 swap?
//     答:提供成員 swap,再在「同一個 namespace」提供自由函式 swap 呼叫它。使用端要寫
//         using std::swap; swap(a, b); 讓 ADL 挑到你的版本。
//     為什麼會錯:很多人直接寫 std::swap(a, b),那就寫死了通用版、永遠找不到你的版本;
//         也有人想在 namespace std 裡加 overload——標準只允許有限度的特化,
//         自行加 overload 是 UB。
// ===========================================================================

#include <algorithm>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: 基本型別 ---
    int a = 1, b = 2;
    std::swap(a, b);
    std::cout << "a=" << a << ", b=" << b << '\n';

    // --- 範例 2: 字串 ---
    std::string s1 = "alpha", s2 = "beta";
    std::swap(s1, s2);
    std::cout << "s1=" << s1 << ", s2=" << s2 << '\n';

    // --- 範例 3: 容器 (O(1) — 只交換內部指標) ---
    std::vector<int> v1{1, 2, 3};
    std::vector<int> v2{9, 8};
    std::swap(v1, v2);
    std::cout << "v1: "; for (int x : v1) std::cout << x << ' '; std::cout << '\n';
    std::cout << "v2: "; for (int x : v2) std::cout << x << ' '; std::cout << '\n';

    // --- 範例 4: ADL 慣用法 (在泛型函式中) ---
    auto adl_swap = [](auto& x, auto& y){
        using std::swap;     // 引入 std::swap 作為候選者
        swap(x, y);          // ADL: 優先找 x/y 所在 namespace 中的 swap
    };
    int p = 5, q = 10;
    adl_swap(p, q);
    std::cout << "after adl_swap: p=" << p << ", q=" << q << '\n';

    // --- 範例 5: 陣列 swap (逐元素) ---
    int arr1[3] = {1, 2, 3};
    int arr2[3] = {7, 8, 9};
    std::swap(arr1, arr2);
    std::cout << "arr1: " << arr1[0] << ' ' << arr1[1] << ' ' << arr1[2] << '\n';

    // === 實務範例 ===
    void practical_double_buffer_swap();
    void practical_custom_swap_idiom();
    void leetcode_2149_rearrange_sign_swap();
    void practical_swap_active_inactive_user();
    practical_double_buffer_swap();
    practical_custom_swap_idiom();
    leetcode_2149_rearrange_sign_swap();
    practical_swap_active_inactive_user();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================
//
// std::swap 在 LeetCode 通常是「演算法的內部步驟」(sort、partition、
// reverse 等都用得到),題目本身鮮少直接考它。所以這裡專注實務情境。

// ----------------------------------------------------------------
// 實務範例 1:大型 vector 的 O(1) 交換 (double-buffer)
// ----------------------------------------------------------------
// 場景:double-buffer 架構 — 一個 buffer 用於讀,另一個用於寫;
//      每幀結束後需要快速將兩個 buffer 互換。
//
// 為什麼用 std::swap:
//   std::swap 對 vector 有「特化」,只搬內部三個指標,O(1) 完成。
//   即使 buffer 有百萬筆資料,也是常數時間 — 完全沒拷貝開銷。
void practical_double_buffer_swap() {
    std::vector<int> read_buf(1000, 0);
    std::vector<int> write_buf(1000, 1);
    auto* read_data_before = read_buf.data();
    std::swap(read_buf, write_buf);
    auto* read_data_after = read_buf.data();
    std::cout << "Buffer swap: read_buf[0]=" << read_buf[0]
              << ", write_buf[0]=" << write_buf[0]
              << ", pointer changed=" << std::boolalpha
              << (read_data_before != read_data_after) << '\n';
}

// ----------------------------------------------------------------
// 實務範例 2:自訂類別提供 friend swap (copy-and-swap idiom 的基礎)
// ----------------------------------------------------------------
// 場景:撰寫自訂類別時提供 friend swap,讓 ADL 能找到並用於
//      copy-and-swap idiom (一種異常安全的賦值寫法)。
struct Box {
    std::string name;
    std::vector<int> values;
    friend void swap(Box& a, Box& b) noexcept {
        using std::swap;
        swap(a.name,   b.name);
        swap(a.values, b.values);
    }
};

void practical_custom_swap_idiom() {
    Box x{"X", {1, 2, 3}};
    Box y{"Y", {9, 8}};
    swap(x, y);   // ADL 找到 friend swap
    std::cout << "Box swap: x.name=" << x.name
              << ", y.name=" << y.name
              << ", x.values.size=" << x.values.size() << '\n';
}

// ----------------------------------------------------------------
// LeetCode 2149 概念:重排陣列使正負交替 (Rearrange Array by Sign)
// 難度: medium
// ----------------------------------------------------------------
// 題目簡化:給陣列 nums (正負數量相等),重排使正負數交替,
//          且保留各自相對順序。
//          這裡示範用 swap 做「就地兩兩配對」的版本 (簡化作法)。
//
// 為什麼用 std::swap:
//   雙指針掃描,把不在正確「奇偶位置」的相鄰元素互換。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_2149_rearrange_sign_swap() {
    std::vector<int> nums{3, 1, -2, -5, 2, -4};
    int n = nums.size();
    int pos_idx = 0, neg_idx = 1;
    std::vector<int> ans(n);
    for (int x : nums) {
        if (x > 0) { ans[pos_idx] = x; pos_idx += 2; }
        else       { ans[neg_idx] = x; neg_idx += 2; }
    }
    // 示範使用 swap:把第一對 (位置 0 和 1) 互換 (作為 demo)
    std::swap(ans[0], ans[1]);   // 只是示範 swap
    std::swap(ans[0], ans[1]);   // 換回來
    std::cout << "LC2149:";
    for (int x : ans) std::cout << ' ' << x;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:把「active user」與「inactive user」的 vector 互換指標
// ----------------------------------------------------------------
// 場景:後台維護兩個大型 vector — active_users / inactive_users。
//      每次「重設活躍狀態」時要把 inactive_users 內所有人變成 active。
//      若直接拷貝資料代價大;用 std::swap 對 vector 互換 (O(1)),
//      原 active 變空 (等待 GC) 再用 clear() 清空。
void practical_swap_active_inactive_user() {
    std::vector<std::string> active{"u1", "u2", "u3"};
    std::vector<std::string> inactive{"u100", "u101", "u102", "u103"};
    std::swap(active, inactive);
    inactive.clear();
    std::cout << "after swap: active size=" << active.size()
              << ", inactive size=" << inactive.size() << '\n';
}

// 編譯: g++ -std=c++20 -Wall -Wextra swap.cpp -o swap

// === 預期輸出 ===
// a=2, b=1
// s1=beta, s2=alpha
// v1: 9 8
// v2: 1 2 3
// after adl_swap: p=10, q=5
// arr1: 7 8 9
// Buffer swap: read_buf[0]=1, write_buf[0]=0, pointer changed=true
// Box swap: x.name=Y, y.name=X, x.values.size=2
// LC2149: 3 -2 1 -5 2 -4
// after swap: active size=4, inactive size=0
