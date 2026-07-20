# C++11：現代 C++ 的共同起點

C++11 不只是增加語法糖；它把「資源由物件生命週期管理」正式推到日常寫法的中心。後續 C++14 到 C++26 大多是在這個模型上補強。

## 本章目標

1. 用 <code>auto</code>、range-for、<code>nullptr</code> 與 scoped enum 降低型別錯誤。
2. 理解 move 是「允許搬走資源」，不是保證不配置、不拷貝。
3. 用 smart pointer、mutex、future 表達所有權與同步，而不是靠註解約定。

## 三個核心模型

### 型別推導不等於動態型別

<code>auto</code> 仍在編譯期決定唯一型別。它會像 template 參數推導一樣通常移除頂層 const 與 reference；需要 reference 時要明寫 <code>auto&amp;</code> 或 <code>const auto&amp;</code>。

### move 不會自己移動

<code>std::move</code> 只是轉成可被 move overload 接受的 xvalue。真正是否搬移，由目標型別的 move constructor/assignment 決定。被 move-from 的標準庫物件仍然有效，但值通常未指定；可以析構、重新賦值，不能假設仍有原內容。

### 執行緒共享資料必須同步

兩條執行緒同時存取同一個非 atomic 物件，且至少一方寫入、又沒有 happens-before 關係，就是 data race，程式行為未定義。<code>join()</code> 只等待結束，不會修復執行期間的競爭。

## 範例索引

本章共 16 個可獨立編譯執行的 `.cpp`；`summary.cpp` 是面試前速查。

<details>
<summary>展開全部檔案</summary>

- [`01_auto.cpp`](01_auto.cpp)
- [`02_decltype.cpp`](02_decltype.cpp)
- [`03_nullptr.cpp`](03_nullptr.cpp)
- [`04_range_based_for.cpp`](04_range_based_for.cpp)
- [`05_brace_init.cpp`](05_brace_init.cpp)
- [`06_initializer_list.cpp`](06_initializer_list.cpp)
- [`07_default_delete.cpp`](07_default_delete.cpp)
- [`08_override_final.cpp`](08_override_final.cpp)
- [`09_constexpr.cpp`](09_constexpr.cpp)
- [`10_static_assert.cpp`](10_static_assert.cpp)
- [`11_type_alias.cpp`](11_type_alias.cpp)
- [`12_enum_class.cpp`](12_enum_class.cpp)
- [`13_user_defined_literals.cpp`](13_user_defined_literals.cpp)
- [`14_raw_string.cpp`](14_raw_string.cpp)
- [`15_trailing_return.cpp`](15_trailing_return.cpp)
- [`summary.cpp`](summary.cpp)

</details>

## 練習

1. 把第一個範例中的 range-for 改成 value、reference、const reference，比較結果。
2. 為 move-only 類別加入計數器，觀察 vector 擴容時是否呼叫 move。
3. 把第三個範例移除 mutex 後用 ThreadSanitizer 執行，只作實驗，不保留錯誤版本。
