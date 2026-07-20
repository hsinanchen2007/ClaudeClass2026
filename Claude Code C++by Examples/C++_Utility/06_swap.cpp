/*
================================================================================
主題:std::swap —— 交換兩個物件的值
標準:C++98 起(C++11 後改良為使用 move,大幅提速)
標頭:<utility>(C++11 起)、<algorithm>(C++98 時期)
參考:https://en.cppreference.com/w/cpp/algorithm/swap
================================================================================

【一、課題介紹】
  std::swap(a, b) 把兩個變數的值互換。聽起來很基本,但它其實是 C++ 中
  一個重要的可重載點(customization point):

  1. STL 容器與演算法(如 std::sort、std::reverse、std::iter_swap)會
     大量呼叫 swap,所以你的型別若有「O(n) 的拷貝」但「O(1) 的 swap」
     (例如交換指標),透過自訂 swap 就能讓 STL 演算法直接受惠。

  2. 「Copy-and-swap idiom」是寫出強例外保證(strong exception guarantee)
     的 operator= 經典手法。

【二、觀念解釋】
  1. 預設行為:std::swap 用三步驟交換:
       T tmp = std::move(a);
       a     = std::move(b);
       b     = std::move(tmp);
     C++11 起預設用 move,所以對 std::string、std::vector 這種「移動成本低」
     的型別,swap 是 O(1)(只搬指標)。

  2. STL 容器的成員 .swap():例如 std::vector<int>::swap 是 O(1),
     比拷貝快很多;std::swap 對 vector 的特化也會走 .swap() 路徑。

  3. ADL 與「兩步 swap 慣用法」:對自訂型別寫 swap 時,正確寫法是
       using std::swap;
       swap(x, y);
     先 using std::swap,讓 std::swap 進入候選;再寫無命名空間的 swap(x,y),
     這樣 ADL(Argument-Dependent Lookup)就能挑到使用者自訂的 swap;
     若沒有自訂版,則退回 std::swap。

  4. 自訂類別最佳實作:加一個無拋例外(noexcept)、O(1) 的成員 swap,
     再提供同命名空間的非成員 swap(讓 ADL 找得到)。

【三、常見陷阱】
  - swap(a, a) 自交換的情境,自己寫 swap 時要避免「先清空 *this 再從 self 搬」
    的錯誤;用「複製暫存 + move」可避免。
  - 不要在 std 命名空間裡塞一個自訂型別的 swap 重載(那是「特化」,規則繁瑣);
    把 swap 放在「自訂型別所在的命名空間」即可,ADL 會自動找到。

【四、與其他 utility 的比較】
  - vs std::exchange:swap 是「a 進 b、b 進 a」;exchange 是「把舊值取出、
    填入新值」(等同 swap 的「半邊」)。
  - vs std::move:move 不真的搬,只是把右值身分賦予物件;swap 是「真的交換」。

【五、Leetcode 對應題目】
  題號:344. Reverse String(反轉字串)
  難度:Easy
  連結:https://leetcode.com/problems/reverse-string/
  題目大意:就地反轉一個 vector<char>。
  選用理由:雙指標 + std::swap 是這題的最直觀解法,正好示範 swap 的用法。

【六、日常工作實用範例】
  情境:Copy-and-swap 寫法的 operator= —— 一個 buffer 類別示範強例外安全。
================================================================================
*/

/*
補充筆記：std::swap
  - swap 交換兩個物件的狀態；對容器通常是常數時間交換內部資源。
  - 寫泛型程式時常先 using std::swap，再呼叫 swap(a,b) 讓 ADL 找自訂版本。
  - swap 是否 noexcept 會影響某些容器與演算法的例外安全策略。
  - std::swap 屬於 utility 類工具；這些型別與函式常用來表達小型資料組合、可選值、型別安全聯集或值類別轉換。
  - pair/tuple 適合簡短聚合結果，但欄位語意複雜時應定義具名 struct，避免 first/second 或 get<0> 難讀。
  - optional 表示可能沒有值，使用前要檢查 has_value 或使用 value_or；value() 在無值時會丟例外。
  - variant 表示多選一型別，應用 visit 或 holds_alternative/get_if 安全存取目前替代項。
  - any 提供執行期任意型別保存，但取回需要知道正確型別；過度使用會失去靜態型別檢查優勢。
  - std::move/std::forward/std::exchange/as_const 都是表達意圖的工具；它們本身不一定搬移或複製資料。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::swap 與 ADL 兩步式慣用法
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::swap 的預設實作是什麼？為什麼要寫 using std::swap;？
//     答：預設實作是三次移動：T tmp = std::move(a); a = std::move(b); b = std::move(tmp);
//     兩步式（two-step）慣用法是先 using std::swap; 引入標準版當後備，再以未限定的
//     swap(a, b) 呼叫，讓 ADL 先找到使用者為自己的型別提供的高效版本（常常只是交換
//     指標）。若寫死 std::swap(a, b) 就繞過了 ADL，永遠用不到那個自訂版本。
//     追問：為什麼不建議特化 std::swap？（函式模板不能偏特化，而全特化又限制多；正解是
//     在自己的 namespace 提供 swap 重載，讓 ADL 找得到）
//
// 🔥 Q2. 為什麼 swap「必須」是 noexcept？
//     答：因為 swap 是回滾機制本身。copy-and-swap 的強例外保證論證建立在「最後那一步
//     不可能失敗」之上：若 swap 會拋例外，資料已部分交換，既沒完成也回不去。骨牌效應是
//     swap 非 noexcept → move assignment 可能無法標 noexcept →
//     is_nothrow_move_constructible 為 false → vector 擴容退回複製 → 效能損失。
//     追問：std::swap 本身是 noexcept 嗎？（條件式：noexcept(is_nothrow_move_constructible_v<T>
//     && is_nothrow_move_assignable_v<T>)）
//
// ⚠️ 陷阱. 自己寫的 swap 需要配置記憶體，標 noexcept 只是形式問題嗎？
//     答：不是。真正的 swap 應該只交換指標或 POD 成員，本質上不可能失敗；若你的 swap
//     需要配置記憶體，那是類別設計有問題的訊號（該用 pimpl，或讓成員自己是 RAII 型別），
//     而不是「把 noexcept 拿掉就好」。
//     為什麼會錯：把 noexcept 當成一個可有可無的標註，忽略它是整個例外安全機制的地基。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <utility>
#include <vector>
#include <string>
#include <algorithm>   // std::iter_swap, std::swap_ranges

// ---------------------------------------------------------------------------
// 範例 1:基本用法 —— 交換兩個 int / string
// ---------------------------------------------------------------------------
void demo_basic() {
    std::cout << "[demo_basic]\n";
    int a = 1, b = 2;
    std::swap(a, b);
    std::cout << "  a=" << a << ", b=" << b << "\n";

    std::string s1 = "hello", s2 = "world";
    std::swap(s1, s2);                            // O(1):只搬內部指標
    std::cout << "  s1=" << s1 << ", s2=" << s2 << "\n";
}

// ---------------------------------------------------------------------------
// 範例 1.5:swap 的「家族成員」—— 陣列、迭代器、整段範圍
//
// std::swap 不只交換單一變數,它在 STL 裡還有兩個常被忽略的好夥伴:
//
//   - std::swap(T(&a)[N], T(&b)[N])  (C++11 起)
//       對「同型別、同大小」的兩個陣列做逐元素交換。
//
//   - std::iter_swap(it1, it2)  (位於 <algorithm>)
//       透過迭代器交換它們指到的元素;對 list / map 等不能用索引的容器特別好用。
//
//   - std::swap_ranges(first1, last1, first2)  (位於 <algorithm>)
//       把 [first1, last1) 與從 first2 起的等長範圍逐一交換,
//       回傳 first2 + (last1 - first1)。
// ---------------------------------------------------------------------------
void demo_swap_family() {
    std::cout << "[demo_swap_family]\n";

    // (a) 陣列整體 swap(C++11):兩個 int[3] 的內容互換
    int A[3] = {1, 2, 3};
    int B[3] = {7, 8, 9};
    std::swap(A, B);
    std::cout << "  array swap: A={" << A[0] << "," << A[1] << "," << A[2] << "}\n";

    // (b) iter_swap:用迭代器交換兩個元素(這裡交換 vector 的頭尾)
    std::vector<int> v = {10, 20, 30, 40, 50};
    std::iter_swap(v.begin(), v.end() - 1);
    std::cout << "  iter_swap : v.front=" << v.front() << ", v.back=" << v.back() << "\n";

    // (c) swap_ranges:把 v[0..3) 與 w[0..3) 整段交換
    std::vector<int> w = {100, 200, 300, 400, 500};
    std::swap_ranges(v.begin(), v.begin() + 3, w.begin());
    std::cout << "  swap_ranges: v[0..2]=" << v[0] << "," << v[1] << "," << v[2] << "\n";
    std::cout << "               w[0..2]=" << w[0] << "," << w[1] << "," << w[2] << "\n";
}

// ---------------------------------------------------------------------------
// 範例 2:Leetcode #344 Reverse String —— 雙指標 + swap
//
// 解題思路:
//   左右各放一個指標,往中間靠,兩端字元用 std::swap 互換,
//   直到 lo >= hi 為止。
//
// 時間複雜度:O(n),空間複雜度:O(1)。
// ---------------------------------------------------------------------------
void reverseString(std::vector<char>& s) {
    int lo = 0, hi = static_cast<int>(s.size()) - 1;
    while (lo < hi) {
        std::swap(s[lo], s[hi]);                  // 直接呼叫 std::swap
        ++lo;
        --hi;
    }
}

void demo_leetcode_reverse() {
    std::cout << "[demo_leetcode_reverse]\n";
    std::vector<char> s = {'h','e','l','l','o'};
    reverseString(s);
    std::cout << "  reversed = ";
    for (char c : s) std::cout << c;
    std::cout << "\n";
}

// ---------------------------------------------------------------------------
// 範例 3:日常工作實用範例 —— Copy-and-swap idiom
//
// 情境:有一個自訂的 Buffer 類別,管理一段 heap 配置的記憶體。
//       要正確寫 operator=,「複製來源 + swap」這個慣用法可以同時:
//         (a) 處理自指派(self assignment)
//         (b) 提供強例外保證:若拷貝失敗,*this 完全不變
//
// 寫法重點:
//   1. 提供成員 swap(noexcept、O(1))。
//   2. operator= 接「按值傳遞」的參數 —— 編譯器會幫忙做拷貝/移動,
//      然後我們只需要 swap。
// ---------------------------------------------------------------------------
class Buffer {
public:
    explicit Buffer(std::size_t n = 0) : size_(n), data_(n ? new int[n] : nullptr) {}
    ~Buffer() { delete[] data_; }

    Buffer(const Buffer& o) : size_(o.size_), data_(size_ ? new int[size_] : nullptr) {
        for (std::size_t i = 0; i < size_; ++i) data_[i] = o.data_[i];
    }

    // 成員 swap:O(1)、noexcept
    void swap(Buffer& o) noexcept {
        using std::swap;                          // 「兩步 swap」慣用法
        swap(size_, o.size_);
        swap(data_, o.data_);
    }

    // operator= 用「按值傳遞 + swap」一行搞定
    Buffer& operator=(Buffer o) noexcept {        // 注意是「值傳」
        swap(o);
        return *this;
    }

    std::size_t size() const { return size_; }
private:
    std::size_t size_;
    int* data_;
};

// 同命名空間的非成員 swap,讓 ADL 能找到(自訂型別應提供這個)
void swap(Buffer& a, Buffer& b) noexcept { a.swap(b); }

void demo_practical_copy_and_swap() {
    std::cout << "[demo_practical_copy_and_swap]\n";
    Buffer x(3), y(7);
    std::cout << "  before: x.size=" << x.size() << ", y.size=" << y.size() << "\n";
    x = y;                                        // 走 Copy-and-swap
    std::cout << "  after : x.size=" << x.size() << ", y.size=" << y.size() << "\n";

    // 也可直接 swap
    Buffer p(2), q(9);
    using std::swap;
    swap(p, q);                                   // ADL 會找到上面那個自由函式 swap
    std::cout << "  swap  : p.size=" << p.size() << ", q.size=" << q.size() << "\n";
}

// ---------------------------------------------------------------------------
// 範例 4 (額外):Leetcode #283 Move Zeroes —— 雙指標 + swap
// 題目:就地把 vector<int> 中的 0 全部移到末尾,其餘元素保持相對順序。
//
// 解題思路:slow 指針指「下一個非零元素該放的位置」,fast 跑全陣列。
//   遇到非零就 swap 到 slow 位置,slow++。
// 時間 O(n),空間 O(1)。
// ---------------------------------------------------------------------------
void moveZeroes(std::vector<int>& nums) {
    int slow = 0;
    for (int fast = 0; fast < (int)nums.size(); ++fast) {
        if (nums[fast] != 0) {
            std::swap(nums[slow], nums[fast]);
            ++slow;
        }
    }
}

void demo_leetcode_move_zeroes() {
    std::cout << "[demo_leetcode_move_zeroes]\n";
    std::vector<int> v{0, 1, 0, 3, 12};
    moveZeroes(v);
    std::cout << "  after: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << "\n";
}

int main() {
    demo_basic();
    demo_swap_family();
    demo_leetcode_reverse();
    demo_leetcode_move_zeroes();
    demo_practical_copy_and_swap();
    return 0;
}

/*
================================================================================
編譯與執行:
    g++ -std=c++17 -Wall -Wextra 06_swap.cpp -o 06_swap && ./06_swap
================================================================================
*/
