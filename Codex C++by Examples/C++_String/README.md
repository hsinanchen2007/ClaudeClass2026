# String：所有權、view 與文字編碼

<code>std::string</code> 擁有一段連續 byte；<code>std::string_view</code> 只借用。這兩者處理的是 code units，不會自動理解 Unicode grapheme、正規化或語言規則。

## string 的保證

C++11 起元素連續，<code>data()</code> 可與需要 pointer+length 的 API 互通。修改字串可能 reallocate，使既有 pointer/reference/iterator/view 失效。不要保存 <code>c_str()</code> 後再修改字串。

## string_view 的適用範圍

適合函式只讀參數與 parser 的切片，前提是來源在使用期間仍存在且未使 buffer 失效。不適合直接保存未知 caller 傳入的 view；若要跨 scope 持有，複製成 string。

## 解析

<code>from_chars</code> 不受 locale 影響、不配置，也不丟例外；呼叫端要同時檢查 error code 和是否消耗完整輸入。<code>stoi</code> 較方便，但可能丟例外且接受部分字串。

## UTF-8

UTF-8 的 <code>size()</code> 是 byte 數，不是使用者看到的字元數。即使數出 Unicode code points，也不等於 grapheme clusters。正式文字處理需要專門 Unicode library；本章只示範 byte 與 leading byte 的界線。

## 範例索引

本章共 59 個可獨立編譯執行的 `.cpp`；`summary.cpp` 是面試前速查。

<details>
<summary>展開全部檔案</summary>

- [`append.cpp`](append.cpp)
- [`assign.cpp`](assign.cpp)
- [`at.cpp`](at.cpp)
- [`back.cpp`](back.cpp)
- [`begin_end.cpp`](begin_end.cpp)
- [`c_str.cpp`](c_str.cpp)
- [`capacity.cpp`](capacity.cpp)
- [`cctype_functions.cpp`](cctype_functions.cpp)
- [`charconv.cpp`](charconv.cpp)
- [`clear.cpp`](clear.cpp)
- [`compare.cpp`](compare.cpp)
- [`comparison_operators.cpp`](comparison_operators.cpp)
- [`constructor.cpp`](constructor.cpp)
- [`contains.cpp`](contains.cpp)
- [`copy.cpp`](copy.cpp)
- [`cstring_functions.cpp`](cstring_functions.cpp)
- [`data.cpp`](data.cpp)
- [`empty.cpp`](empty.cpp)
- [`ends_with.cpp`](ends_with.cpp)
- [`erase.cpp`](erase.cpp)
- [`erase_free.cpp`](erase_free.cpp)
- [`find.cpp`](find.cpp)
- [`find_first_not_of.cpp`](find_first_not_of.cpp)
- [`find_first_of.cpp`](find_first_of.cpp)
- [`find_last_not_of.cpp`](find_last_not_of.cpp)
- [`find_last_of.cpp`](find_last_of.cpp)
- [`format.cpp`](format.cpp)
- [`front.cpp`](front.cpp)
- [`get_allocator.cpp`](get_allocator.cpp)
- [`getline.cpp`](getline.cpp)
- [`hash.cpp`](hash.cpp)
- [`insert.cpp`](insert.cpp)
- [`io_operators.cpp`](io_operators.cpp)
- [`max_size.cpp`](max_size.cpp)
- [`numeric_conversions.cpp`](numeric_conversions.cpp)
- [`operator_assign.cpp`](operator_assign.cpp)
- [`operator_plus.cpp`](operator_plus.cpp)
- [`operator_plus_eq.cpp`](operator_plus_eq.cpp)
- [`operator_string_view.cpp`](operator_string_view.cpp)
- [`operator_subscript.cpp`](operator_subscript.cpp)
- [`pop_back.cpp`](pop_back.cpp)
- [`push_back.cpp`](push_back.cpp)
- [`range_modifiers.cpp`](range_modifiers.cpp)
- [`rbegin_rend.cpp`](rbegin_rend.cpp)
- [`regex_basics.cpp`](regex_basics.cpp)
- [`replace.cpp`](replace.cpp)
- [`reserve.cpp`](reserve.cpp)
- [`resize.cpp`](resize.cpp)
- [`resize_and_overwrite.cpp`](resize_and_overwrite.cpp)
- [`rfind.cpp`](rfind.cpp)
- [`shrink_to_fit.cpp`](shrink_to_fit.cpp)
- [`size_length.cpp`](size_length.cpp)
- [`starts_with.cpp`](starts_with.cpp)
- [`std_swap.cpp`](std_swap.cpp)
- [`string_literals.cpp`](string_literals.cpp)
- [`string_view.cpp`](string_view.cpp)
- [`substr.cpp`](substr.cpp)
- [`summary.cpp`](summary.cpp)
- [`swap.cpp`](swap.cpp)

</details>

## 練習

1. 寫一個只借用輸入的 key/value parser。
2. 讓整數 parser 拒絕前後空白，再寫一版明確允許空白。
3. 使用 ICU 或 utf8proc 比較 code point 與 grapheme cluster 計數。
