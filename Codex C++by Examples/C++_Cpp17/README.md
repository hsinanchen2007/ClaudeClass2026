# C++17：用型別表達狀態

C++17 的實用核心是 vocabulary types：<code>optional</code>、<code>variant</code>、<code>string_view</code>，以及讓結構化資料更容易使用的 structured binding。它們把過去靠特殊值、union 或 out parameter 表達的狀態，搬進型別系統。

## optional：有值或沒有值

適合「沒有結果是正常情況」，例如查找不到、設定未提供。它不負責攜帶詳細錯誤原因；需要原因時應使用 error code、exception，或 C++23 的 expected。

## variant：有限集合中的其中一種

<code>variant&lt;A, B&gt;</code> 是 tagged union。它知道目前 active alternative，並負責正確析構。使用 <code>visit</code> 時，visitor 應覆蓋所有 alternatives。

## string_view：非擁有 view

它只保存 pointer 與長度，不延長來源生命週期。不要從 temporary string 回傳 string_view，也不要在來源 reallocate/destroy 後使用舊 view。

## structured binding 與 if initializer

structured binding 讓 pair、tuple 與 aggregate 拆解更清楚；是否複製取決於前面的 <code>auto</code>、<code>auto&amp;</code> 或 <code>const auto&amp;</code>。if initializer 可把 iterator 或 lock 的 scope 限制在條件區塊。

## 範例索引

本章共 14 個可獨立編譯執行的 `.cpp`；`summary.cpp` 是面試前速查。

<details>
<summary>展開全部檔案</summary>

- [`01_structured_bindings.cpp`](01_structured_bindings.cpp)
- [`02_if_with_init.cpp`](02_if_with_init.cpp)
- [`03_switch_with_init.cpp`](03_switch_with_init.cpp)
- [`04_if_constexpr.cpp`](04_if_constexpr.cpp)
- [`05_inline_variables.cpp`](05_inline_variables.cpp)
- [`06_ctad.cpp`](06_ctad.cpp)
- [`07_nested_namespace.cpp`](07_nested_namespace.cpp)
- [`08_nodiscard.cpp`](08_nodiscard.cpp)
- [`09_maybe_unused.cpp`](09_maybe_unused.cpp)
- [`10_fallthrough.cpp`](10_fallthrough.cpp)
- [`11_string_view.cpp`](11_string_view.cpp)
- [`12_byte.cpp`](12_byte.cpp)
- [`13_invoke.cpp`](13_invoke.cpp)
- [`summary.cpp`](summary.cpp)

</details>

## 練習

1. 將 optional 查找改成回傳 reference_wrapper，避免複製大型物件。
2. 為 variant 新增一種事件，讓編譯器指出 visitor 尚未處理的位置。
3. 寫一個只在呼叫期間使用 string_view 的 parser，並說明來源壽命要求。
