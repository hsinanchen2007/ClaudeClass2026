/*
================================================================================
【C++_Algorithm/modifying/summary.cpp】
本章末總整理 — 修改型序列演算法 (Modifying Sequence Operations)
================================================================================

本檔僅為章末教科書式總整理。依規則 8,本檔不包含任何 LeetCode 題目。

參考:
  - https://en.cppreference.com/w/cpp/algorithm#Modifying_sequence_operations
  - https://cplusplus.com/reference/algorithm/

--------------------------------------------------------------------------------
一、本章涵蓋的 API 分組
--------------------------------------------------------------------------------
  搬運 / 寫入:
    copy / copy_if / copy_n / copy_backward
    move / move_backward
    fill / fill_n
    generate / generate_n
    transform                    (一輸入 → 一輸出 或 兩輸入 → 一輸出)
    iter_swap / swap_ranges
    sample              (C++17 — 隨機取樣)
    shuffle             (C++11+,在 random 章節)
  改值:
    replace / replace_if / replace_copy / replace_copy_if
  反序與旋轉:
    reverse / reverse_copy
    rotate / rotate_copy
  「移除」與「去重」(都是邏輯移除,不會縮小容器):
    remove / remove_if / remove_copy / remove_copy_if
    unique / unique_copy

--------------------------------------------------------------------------------
二、重要觀念:演算法不擁有容器
--------------------------------------------------------------------------------
  ★ 標準演算法只透過 iterator 讀寫;它不知道容器有多大、能不能擴張。
  ★ 所以:
    1. 寫入型演算法 (copy / transform 等) 必須確保目的端「已經有足夠空間」。
       - 用 back_inserter / front_inserter / inserter 來「自動長大」。
       - 或先 resize() 目的端容器。
    2. 「移除型」演算法 (remove / unique) 不會真的刪除元素,只是「邏輯刪除」:
       把要保留的元素往前壓,回傳新尾端 new_end;
       要真正移除須搭配 container.erase(new_end, end()) — 即 erase-remove idiom。
       C++20 起也可用 std::erase(v, value) / std::erase_if(v, pred) 一行解決。

--------------------------------------------------------------------------------
三、erase-remove idiom (必背)
--------------------------------------------------------------------------------
  傳統寫法:
      auto it = std::remove(v.begin(), v.end(), 2);
      v.erase(it, v.end());

  C++20 簡化:
      std::erase(v, 2);              // 移除所有等於 2 的元素
      std::erase_if(v, [](int x){ return x > 10; });

  ★ 為什麼 remove 不直接刪?
    因為 std::remove 只持有 iterator,沒有容器的引用,無法呼叫 erase。
    這是 STL「演算法與容器分離」設計理念的代價。

--------------------------------------------------------------------------------
四、複雜度速查
--------------------------------------------------------------------------------
  API               時間              迭代器       備註
  ────────────────  ────────────────  ──────────   ─────────────────────
  copy / copy_n     O(N)              Input→Output
  copy_if           O(N)              同上          C++11+
  move              O(N)              同 copy      C++11+,內部 std::move(*it)
  transform         O(N)              同上          一元/二元 functor
  fill / fill_n     O(N)              Forward/Out   寫入相同值
  generate          O(N)              Forward       靠 functor 提供值
  replace(_if)      O(N)              Forward       in-place 改值
  remove(_if)       O(N)              Forward       邏輯刪除
  unique            O(N)              Forward       只去「相鄰」重複
  reverse           O(N)              Bidirectional
  rotate            O(N)              Forward
  swap_ranges       O(N)              Forward
  iter_swap         O(1)              —
  sample            O(N) 或 O(N*log N) C++17

--------------------------------------------------------------------------------
五、unique 重點 — 只去「相鄰」重複
--------------------------------------------------------------------------------
  std::unique 只把「相鄰相同」的元素合併:
      {1,1,2,2,2,3,1,1}  → unique 後變 {1,2,3,1, ?, ?, ?, ?}
                            (尾端是未定義垃圾值)
  ★ 若要「全域去重」,先 std::sort 再 unique:
      std::sort(v.begin(), v.end());
      v.erase(std::unique(v.begin(), v.end()), v.end());

--------------------------------------------------------------------------------
六、copy_backward / move_backward / reverse_copy / rotate_copy
--------------------------------------------------------------------------------
  - copy 預設「從前往後」;當目的範圍與來源重疊且目的在來源之後,
    必須改用 copy_backward 才不會邊讀邊覆寫。
  - 一般獨立記憶體區段,兩者結果一樣;只有重疊區段才有差別。

--------------------------------------------------------------------------------
七、transform 兩種版本
--------------------------------------------------------------------------------
  一元 (unary):
      std::transform(in.begin(), in.end(), out.begin(),
                     [](int x){ return x * x; });
  二元 (binary):兩個輸入 range 合併
      std::transform(a.begin(), a.end(), b.begin(), out.begin(),
                     std::plus<>{});
  - 目的可以與來源相同 (in-place),只要記憶體相同地址。

--------------------------------------------------------------------------------
八、Iterator 失效規則 (Iterator Invalidation)
--------------------------------------------------------------------------------
  - vector::erase / resize / reserve / push_back (觸發重新配置) → 全失效。
  - list::erase → 只失效被刪元素的 iterator。
  - std::remove 本身不失效任何 iterator (沒有重新配置),但「[new_end, end())
    區域的內容已是 unspecified」。

  ★ 經驗法則:對 vector 做修改後,所有舊 iterator 視為失效,重新取得。

--------------------------------------------------------------------------------
九、常見陷阱 (Common Pitfalls)
--------------------------------------------------------------------------------
  1. 用 copy 寫入空 vector → UB (沒空間)。
     對策:用 std::back_inserter(out) 或先 out.resize(n)。

  2. 把 std::remove 當成真的刪除 → bug。記得搭配 erase。

  3. unique 不排序就用 → 只去相鄰重複,留下別處還有的重複。

  4. transform 兩 range 長度不同 → 預設只走到第一個 range 結束;
     若第二個比第一個短會越界 UB。

  5. fill 寫到唯讀範圍 (例如 const_iterator) → 編譯錯。

  6. std::move(算法) ≠ std::move(rvalue cast)。
     std::move(first, last, d_first) 是搬運演算法;
     std::move(x) 是把 x 轉成右值參考,功能不同,不要混淆。

  7. rotate 的回傳值常被忽略:
       回傳新位置 (= first + (last - middle)),指向「原本 first 元素的新位置」。
       許多人沒用到這個資訊,但對串接後續操作很有用。

  8. sample (C++17) 需要 ForwardIterator 來源 + 隨機引擎;
     用法:
       std::sample(in.begin(), in.end(), out_iter, k, rng);

--------------------------------------------------------------------------------
十、工作場景 (Daily Work Use Cases)
--------------------------------------------------------------------------------
  - 過濾無效訂單 → erase + remove_if (或 std::erase_if)。
  - log 壓縮:連續重複訊息只保留一份 → unique。
  - 把 vector 中所有負數歸 0 → replace_if。
  - 把 ID 對應到 token → transform。
  - 反向顯示事件清單 → reverse。
  - 把第一筆移到尾端 (VIP 任務插隊的反向操作) → rotate。
  - 在資料集隨機取樣作測試 → sample。
================================================================================
*/

// ===========================================================================
// 【面試題】修改型序列演算法(Modifying Sequence Operations)家族總覽
// ---------------------------------------------------------------------------
// 🔥 Q1. 為什麼 STL 演算法「不能」改變容器大小?
//     答:因為演算法只拿到 iterator,拿不到容器本身。iterator 能讀寫既有元素,
//         但配置/釋放空間、改變 size 只有容器的成員函式做得到。這是 STL
//         「演算法與容器分離」設計的直接代價,也是本章一半陷阱的共同根源。
//         兩個後果:寫入型演算法不會替你擴容;移除型演算法不會真的縮短容器。
//     追問:那 back_inserter 為什麼就可以?
//         (答:它是包著容器參考的 output iterator,寫入時實際呼叫的是 push_back)
//
// 🔥 Q2. 哪些演算法需要「目的端已有足夠空間」?寫不夠會怎樣?
//     答:所有會寫到 d_first / out 的:copy 家族、move 家族、transform、
//         fill / fill_n、generate / generate_n、replace_copy、reverse_copy、
//         rotate_copy、unique_copy、sample。空間不足就是 UB(buffer overrun),
//         不會有例外也不會有錯誤訊息。對策:先 resize(),或用 back_inserter /
//         inserter 這類 insert iterator。注意 reserve() 不算——它只給 capacity,
//         size 仍是 0。
//
// 🔥 Q3. 「移除型」演算法的統一慣用法是什麼?
//     答:remove / remove_if / unique 都只做「邏輯移除」——把要保留的元素往前壓,
//         回傳新的邏輯終點,size() 不變,尾端元素是 valid but unspecified。
//         必須搭配 erase-remove idiom 才會真的縮短:
//           v.erase(std::remove(v.begin(), v.end(), value), v.end());
//           v.erase(std::unique(v.begin(), v.end()), v.end());
//         C++20 起可直接用 std::erase(v, value) / std::erase_if(v, pred)。
//
// ⚠️ 陷阱. 這一章有哪幾個「名字會騙人」的演算法?
//     答:三個。(1) remove 不會 remove;(2) <algorithm> 的 std::move 是搬運演算法,
//         和 <utility> 的型別轉換 std::move 同名不同物;(3) unique 不是「全域去重」,
//         只處理相鄰重複,不先 sort 就會留下別處的重複值。
//     為什麼會錯:這些名字描述的是「演算法對區間做了什麼」,
//         而不是「使用者以為的容器操作」。
//
// Q4. 通用演算法和容器成員函式同名時,該選哪個?
//     答:一律優先用成員函式。list::remove / list::unique / list::reverse / list::sort
//         操作的是節點,真的會改變 size 或不搬移元素,效率與語意都更好;
//         關聯容器(set/map)則根本不該用這些通用演算法,要用自己的 erase。
// ===========================================================================

#include <algorithm>
#include <iostream>
#include <numeric>
#include <vector>

static void print(const std::vector<int>& v, const char* label) {
    std::cout << label << ":";
    for (int x : v) std::cout << ' ' << x;
    std::cout << "\n";
}

// -----------------------------------------------------------------------------
// 示範區 (Demonstration)
// -----------------------------------------------------------------------------
static void demo_transform_replace() {
    std::cout << "\n[demo_transform_replace]\n";
    std::vector<int> v(5);
    std::iota(v.begin(), v.end(), 1);  // 1..5

    // 目的端先 resize,確保 transform 寫入有空間
    std::vector<int> squares;
    squares.resize(v.size());
    std::transform(v.begin(), v.end(), squares.begin(),
                   [](int x) { return x * x; });
    print(squares, "  squares");

    // in-place 修改:把大於 10 的改成 0
    std::replace_if(squares.begin(), squares.end(),
                    [](int x) { return x > 10; }, 0);
    print(squares, "  replace_if(x>10 ->0)");
}

static void demo_remove_erase() {
    std::cout << "\n[demo_remove_erase]\n";
    std::vector<int> v{1, 2, 3, 2, 4, 2, 5};
    print(v, "  before");

    auto new_end = std::remove(v.begin(), v.end(), 2);
    // 注意:v 內容已被覆寫,但 size 沒變!尾端是「未定義垃圾值」。
    print(v, "  after remove (size 不變,尾端為垃圾)");

    v.erase(new_end, v.end());
    print(v, "  after erase (真的縮小了)");
}

static void demo_reverse_rotate_unique() {
    std::cout << "\n[demo_reverse_rotate_unique]\n";
    std::vector<int> v{1, 2, 3, 4, 5};
    std::reverse(v.begin(), v.end());
    print(v, "  reverse");

    // rotate:把 [begin, begin+2) 轉到尾端
    std::rotate(v.begin(), v.begin() + 2, v.end());
    print(v, "  rotate(begin+2)");

    // unique:只去相鄰重複 (此處資料相鄰是排好的)
    std::vector<int> u{1, 1, 2, 2, 2, 3};
    auto end2 = std::unique(u.begin(), u.end());
    u.erase(end2, u.end());
    print(u, "  unique+erase");
}

int main() {
    demo_transform_replace();
    demo_remove_erase();
    demo_reverse_rotate_unique();
    std::cout << "\n[done]\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary && ./summary
