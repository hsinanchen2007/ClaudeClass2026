# C++14：讓泛型程式更自然

C++14 是一個「補齊 C++11 使用體驗」的版本。重點不是全新的程式模型，而是讓 lambda、型別推導與所有權工具更適合日常使用。

## 本章目標

- 用 generic lambda 對不同型別重用同一個短行為。
- 用 init-capture 把資料安全地搬進 closure。
- 用 <code>std::make_unique</code> 在一個 expression 中完成配置與所有權建立。
- 認識 variable template 與函式回傳型別推導。

## Generic lambda

lambda 參數寫成 <code>auto</code> 時，closure 的 call operator 會成為 function template。它適合短小、只在呼叫點附近使用的泛型行為；需要清楚介面、文件或多個 overload 時，普通 function object 往往更好。

## Init-capture 與生命週期

<code>[name = expression]</code> 會在建立 lambda 時初始化 closure member。若 expression 使用 <code>std::move</code>，可把 move-only 資源交給 lambda。這比用 reference capture 丟進延後執行的 callback 更安全，因為 closure 自己擁有資料。

## make_unique

<code>make_unique&lt;T&gt;(args...)</code> 把配置與建構包在一起，避免裸 <code>new</code> 暫時暴露。除非需要特殊 deleter，應優先使用它。

## 範例索引

本章共 8 個可獨立編譯執行的 `.cpp`；`summary.cpp` 是面試前速查。

<details>
<summary>展開全部檔案</summary>

- [`01_auto_return_type.cpp`](01_auto_return_type.cpp)
- [`02_decltype_auto.cpp`](02_decltype_auto.cpp)
- [`03_constexpr_relaxed.cpp`](03_constexpr_relaxed.cpp)
- [`04_binary_literals.cpp`](04_binary_literals.cpp)
- [`05_digit_separator.cpp`](05_digit_separator.cpp)
- [`06_make_unique.cpp`](06_make_unique.cpp)
- [`07_deprecated_attribute.cpp`](07_deprecated_attribute.cpp)
- [`summary.cpp`](summary.cpp)

</details>

## 練習

1. 將 generic lambda 改成接受兩種不同型別，觀察回傳型別。
2. 建立一個擁有 vector 的 lambda，移動 lambda 後再執行。
3. 為 variable template 加上 <code>long double</code> 特化。
