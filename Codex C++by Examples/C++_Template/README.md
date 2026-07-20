# Template：從重複程式碼到可檢查的抽象

Template 的價值不是炫技，而是「讓同一個演算法適用多種型別，同時保留靜態型別檢查與最佳化機會」。好的 template 介面會清楚說明需求；C++20 concepts 正是為此而生。

## Function 與 class template

Function template 通常可由參數推導型別；class template 在 C++17 起有 CTAD，但公用 API 仍應讓型別意圖清楚。把與型別無關的程式碼移出 template，可降低編譯時間與 code bloat。

## Concepts

Concept 是具名的 compile-time predicate。它改善三件事：

1. 文件：介面直接寫出需求。
2. overload selection：不合條件的候選會被排除。
3. diagnostics：錯誤更接近呼叫點。

Concept 驗證語法與可觀察性質，不會自動證明語意。例如 <code>std::totally_ordered</code> 仍假設使用者實作符合數學關係。

## Variadic template 與 fold

Parameter pack 表示零個以上型別或值。Fold expression 將 pack 依指定 operator 展開。空 pack 是否有效取決於 operator 是否有 identity，或你是否提供初始值。

## 範例索引

本章共 29 個可獨立編譯執行的 `.cpp`；`summary.cpp` 是面試前速查。

<details>
<summary>展開全部檔案</summary>

- [`01_why_templates.cpp`](01_why_templates.cpp)
- [`02_function_template_basics.cpp`](02_function_template_basics.cpp)
- [`03_class_template_basics.cpp`](03_class_template_basics.cpp)
- [`04_template_parameters.cpp`](04_template_parameters.cpp)
- [`05_template_specialization_full.cpp`](05_template_specialization_full.cpp)
- [`06_template_specialization_partial.cpp`](06_template_specialization_partial.cpp)
- [`07_default_template_arguments.cpp`](07_default_template_arguments.cpp)
- [`08_typename_vs_class.cpp`](08_typename_vs_class.cpp)
- [`09_non_type_template_parameter.cpp`](09_non_type_template_parameter.cpp)
- [`10_template_template_parameter.cpp`](10_template_template_parameter.cpp)
- [`11_alias_template.cpp`](11_alias_template.cpp)
- [`12_variable_template.cpp`](12_variable_template.cpp)
- [`13_variadic_templates.cpp`](13_variadic_templates.cpp)
- [`14_fold_expressions.cpp`](14_fold_expressions.cpp)
- [`15_perfect_forwarding.cpp`](15_perfect_forwarding.cpp)
- [`16_sfinae.cpp`](16_sfinae.cpp)
- [`17_type_traits.cpp`](17_type_traits.cpp)
- [`18_tag_dispatch.cpp`](18_tag_dispatch.cpp)
- [`19_if_constexpr.cpp`](19_if_constexpr.cpp)
- [`20_concepts_intro.cpp`](20_concepts_intro.cpp)
- [`21_concepts_advanced.cpp`](21_concepts_advanced.cpp)
- [`22_crtp.cpp`](22_crtp.cpp)
- [`23_policy_based_design.cpp`](23_policy_based_design.cpp)
- [`24_type_erasure.cpp`](24_type_erasure.cpp)
- [`25_template_compilation_model.cpp`](25_template_compilation_model.cpp)
- [`26_template_with_lambda.cpp`](26_template_with_lambda.cpp)
- [`27_constexpr_templates.cpp`](27_constexpr_templates.cpp)
- [`28_capstone_mini_stl.cpp`](28_capstone_mini_stl.cpp)
- [`summary.cpp`](summary.cpp)

</details>

## 練習

1. 將固定容量 Stack 改成支援 <code>std::string</code> 並測試滿/空。
2. 寫一個只接受 floating-point 的平均函式。
3. 讓 variadic printer 在零個參數時仍產生合法輸出。
