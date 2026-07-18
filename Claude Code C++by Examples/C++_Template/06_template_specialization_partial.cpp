// ============================================================================
//  06_template_specialization_partial.cpp  ──  偏特化 (Partial Specialization)
// ============================================================================
//
//  【本篇目標】
//    上一篇 (file 05) 講「完全特化」(把所有參數填死)。本篇講「偏特化」：
//    只填一部分參數，或對某種「結構性型別」(如 T*、std::vector<T>) 做特化。
//
//  【關鍵限制】
//    - Class template 可以偏特化。
//    - Function template 不可以偏特化！想偏特化 function template，請改用
//      function overloading 取代。
//
//  【常見偏特化形式】
//
//    ① 對指標偏特化：
//          template <typename T> struct IsPointer            { static const bool value = false; };
//          template <typename T> struct IsPointer<T*>        { static const bool value = true;  };
//
//    ② 對某容器偏特化 (e.g. 只匹配 std::vector<T>)：
//          template <typename T> struct Wrapper             { ... };
//          template <typename T> struct Wrapper<std::vector<T>> { ... };
//
//    ③ 多參數中只填一部分：
//          template <typename T, typename U> struct Pair    { ... };
//          template <typename T>             struct Pair<T, T> { ... };  // 兩參數相同型別
//
//  【偏特化的選擇規則】
//    編譯器會選「最特化的」版本：能匹配的版本中限制最多者勝出。例如有
//    primary、有 T*、有 const T*，呼叫 const int* 時會選 const T*。
//
//  參考：https://en.cppreference.com/cpp/language/partial_specialization
// ============================================================================

/*
補充筆記：template_specialization_partial
  - template_specialization_partial 涉及模板實例化；請先判斷哪些型別由呼叫端推導，哪些型別由程式指定。
  - 泛型程式要把型別需求寫清楚，例如可比較、可移動、可呼叫或符合某個 concept。
  - 模板技巧的價值在減少重複且保留型別安全，不是讓錯誤訊息變得更長。
  - template_specialization_partial 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/
#include <iostream>
#include <vector>
#include <list>
#include <string>

// ─── 1. 自製 is_pointer<T> ──────────────────────────────────────────────
//   標準庫 std::is_pointer 就是這樣寫的 (簡化版)。
//   primary 預設 false，凡是 T* 形狀 → 偏特化版本 → true。
template <typename T> struct is_pointer            { static constexpr bool value = false; };
template <typename T> struct is_pointer<T*>        { static constexpr bool value = true;  };
template <typename T> struct is_pointer<T* const>  { static constexpr bool value = true;  };
template <typename T> struct is_pointer<T* volatile>          { static constexpr bool value = true; };
template <typename T> struct is_pointer<T* const volatile>    { static constexpr bool value = true; };

// 變數模板簡化呼叫 (file 12 會詳講)
template <typename T>
constexpr bool is_pointer_v = is_pointer<T>::value;

// ─── 2. TypeName 用偏特化處理 vector<T> ─────────────────────────────────
//   做一個 debug 工具：給任意型別輸出名字。
//   - primary 預設 "unknown"
//   - 對 int / double 用「完全特化」
//   - 對 std::vector<T> 用「偏特化」(只填一個位置)
template <typename T>
struct TypeName { static std::string get() { return "unknown"; } };

template<> struct TypeName<int>    { static std::string get() { return "int"; } };
template<> struct TypeName<double> { static std::string get() { return "double"; } };

template <typename T>
struct TypeName<std::vector<T>> {
    static std::string get() {
        // 注意：這裡可以呼叫 TypeName<T>::get()，遞迴生出巢狀型別的名字
        return "std::vector<" + TypeName<T>::get() + ">";
    }
};

template <typename T>
struct TypeName<T*> {
    static std::string get() {
        return TypeName<T>::get() + "*";
    }
};

// ─── 3. Leetcode 21 ── Merge Two Sorted Lists (示範 pointer 偏特化的實用面) ──
//   題目：給兩個排序好的 linked list head，合併成一個排序好的 list 並回傳 head。
//   範例：l1 = 1→2→4, l2 = 1→3→4 → 1→1→2→3→4→4
//
//   為什麼放這篇？
//     LC 原題用單向 linked list；走訪節點時用「指標」串起來。如果你寫一個
//     「印 list」之類的 helper 函式，可以靠 pointer 偏特化讓 vector<int> 走
//     一條路、ListNode* 走另一條路。
//
//   時間：O(n+m)
//   空間：O(1) (in-place 串接)
//   邊界：l1 或 l2 為 nullptr → 直接回另一個。

struct ListNode {
    int val;
    ListNode* next;
    explicit ListNode(int v, ListNode* n = nullptr) : val(v), next(n) {}
};

ListNode* merge_two_lists(ListNode* l1, ListNode* l2) {
    ListNode dummy(0);          // 哨兵節點，省掉特判
    ListNode* tail = &dummy;
    while (l1 && l2) {
        if (l1->val <= l2->val) { tail->next = l1; l1 = l1->next; }
        else                    { tail->next = l2; l2 = l2->next; }
        tail = tail->next;
    }
    tail->next = l1 ? l1 : l2;  // 接上剩餘部分
    return dummy.next;
}

// ─── 4. 用偏特化做「印任何容器或 pointer」的工具 ──────────────────────────
//   - primary：直接 cout << v
//   - vector<T>：印成 [a, b, c]
//   - T*       ：先解參考再印
template <typename T>
struct Printer {
    static void print(const T& v) { std::cout << v; }
};

template <typename T>
struct Printer<std::vector<T>> {
    static void print(const std::vector<T>& v) {
        std::cout << "[";
        for (std::size_t i = 0; i < v.size(); ++i) {
            if (i) std::cout << ", ";
            Printer<T>::print(v[i]);                 // 遞迴
        }
        std::cout << "]";
    }
};

template <typename T>
struct Printer<T*> {
    static void print(T* p) {
        if (!p) { std::cout << "nullptr"; return; }
        std::cout << "*(" ;
        Printer<T>::print(*p);
        std::cout << ")";
    }
};

// 包裝成方便呼叫的函式
template <typename T> void print(const T& v) { Printer<T>::print(v); std::cout << "\n"; }

// ─── 5. Leetcode 1207 ── Unique Number of Occurrences (偏特化 Printer 應用) ─
//   難度: easy
//   題目：給整數陣列 arr，判斷「每個元素的出現次數」是否互不相同。
//   範例：[1,2,2,1,1,3] → 出現次數 {1:3, 2:2, 3:1}，三個都不同 → true
//
//   解法：先用 hash map 統計次數，再把次數放進 set，比較大小。
//
//   為什麼放在這裡？
//     結果裡會用到 std::pair<K, count>。我們順手對 std::pair<A,B> 做偏特化，
//     讓 Printer 能直接印 pair。
#include <unordered_map>
#include <unordered_set>

bool unique_occurrences(const std::vector<int>& arr) {
    std::unordered_map<int, int> cnt;
    for (int x : arr) ++cnt[x];
    std::unordered_set<int> seen;
    for (auto& kv : cnt) {
        if (!seen.insert(kv.second).second) return false;
    }
    return true;
}

template <typename A, typename B>
struct Printer<std::pair<A, B>> {
    static void print(const std::pair<A, B>& p) {
        std::cout << "(";
        Printer<A>::print(p.first);
        std::cout << ", ";
        Printer<B>::print(p.second);
        std::cout << ")";
    }
};

// ─── 6. 工作實用：DefaultValue<T>，對 T、T*、std::vector<T> 各有偏特化 ───
//   實務場景：泛型容器初始化、JSON 反序列化的 fallback 值。
template <typename T>
struct DefaultValue { static T get() { return T{}; } };

template <typename T>
struct DefaultValue<T*> { static T* get() { return nullptr; } };

template <typename T>
struct DefaultValue<std::vector<T>> {
    static std::vector<T> get() { return std::vector<T>{}; }
};

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    // (1) is_pointer
    std::cout << std::boolalpha;
    std::cout << "is_pointer<int>           = " << is_pointer_v<int>           << "\n";
    std::cout << "is_pointer<int*>          = " << is_pointer_v<int*>          << "\n";
    std::cout << "is_pointer<const int*>    = " << is_pointer_v<const int*>    << "\n";
    std::cout << "is_pointer<int* const>    = " << is_pointer_v<int* const>    << "\n";

    // (2) TypeName
    std::cout << "TypeName<int>                       = " << TypeName<int>::get() << "\n";
    std::cout << "TypeName<std::vector<int>>          = " << TypeName<std::vector<int>>::get() << "\n";
    std::cout << "TypeName<std::vector<std::vector<double>>> = "
              << TypeName<std::vector<std::vector<double>>>::get() << "\n";
    std::cout << "TypeName<int*>                      = " << TypeName<int*>::get() << "\n";

    // (3) Leetcode 21 Merge Two Sorted Lists
    ListNode a3(4), a2(2, &a3), a1(1, &a2);
    ListNode b3(4), b2(3, &b3), b1(1, &b2);
    ListNode* merged = merge_two_lists(&a1, &b1);
    std::cout << "Merged: ";
    for (ListNode* p = merged; p; p = p->next) std::cout << p->val << ' ';
    std::cout << "\n";

    // (4) Printer 偏特化
    int x = 7;
    print(x);
    print(&x);
    print(std::vector<int>{1, 2, 3});
    print(std::vector<std::vector<int>>{{1,2},{3,4,5}});

    // (5) Leetcode 1207 Unique Occurrences
    std::cout << "unique_occurrences({1,2,2,1,1,3}) = "
              << unique_occurrences({1, 2, 2, 1, 1, 3}) << "\n";
    std::cout << "unique_occurrences({1,2})         = "
              << unique_occurrences({1, 2}) << "\n";
    print(std::pair<std::string, int>{"alice", 30});

    // (6) DefaultValue<T>
    std::cout << "DefaultValue<int>            = " << DefaultValue<int>::get() << "\n";
    std::cout << "DefaultValue<int*>           = " << DefaultValue<int*>::get() << " (nullptr)\n";
    std::cout << "DefaultValue<vector<int>>... = size " << DefaultValue<std::vector<int>>::get().size() << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼 function template 不能 partial specialize？該怎麼變通？
    //    A：標準明定 function template 只允許 full specialization，不允
    //       許 partial。原因是 function 已經有 overload resolution 機制
    //       足以表達「對某類型別走另一條路」，再加上 partial specialization
    //       會讓選擇規則嚴重複雜化。變通解：(a) 直接寫 function overload；
    //       (b) 把邏輯包進 class template 並 partial specialize 該 class，
    //       function 裡呼叫 class 的 static 成員 (這是 std::iter_swap 的手法)。
    //
    //  Q2：當有多個 partial specialization 都能匹配時，誰會被選中？
    //    A：編譯器會做「partial ordering」(偏序比較)，挑出「最特化」(more
    //       specialized) 的版本。直覺判斷：限制條件最多、最具體的勝出。
    //       例如同時有 T*、const T*、int*，傳 const int* 時 const T*
    //       勝；傳 int* 時 T* 與 int* 都能匹配但 int* 較具體故勝。若兩
    //       個版本「無法比較誰更特化」，編譯器報 ambiguous 錯誤。
    //
    //  Q3：partial specialization 跟 SFINAE / concept 比，何時該用哪個？
    //    A：partial specialization 適合「依型別結構分流」(T*、std::vector<T>
    //       這種長相)，分支邏輯靜態固定、寫法直觀。SFINAE / concept 適
    //       合「依型別性質分流」(可加、有 begin()、是 integral)，更貼
    //       近能力契約。現代 C++20 寫法常選 concept，partial specialization
    //       仍是 type traits 庫的主力 (例如 std::is_pointer 內部就是)。
    //
    return 0;
}

// ============================================================================
//  【小結】
//    1. Function template 不能偏特化 → 用 overload 替代。
//    2. Class template 偏特化是常見 metaprogramming 技巧 (e.g. type traits)。
//    3. 偏特化會選「最具體」的版本；多個版本能匹配時比限制最多者。
//
//  【下一篇】
//    07_default_template_arguments.cpp ── 預設模板引數。
// ============================================================================
