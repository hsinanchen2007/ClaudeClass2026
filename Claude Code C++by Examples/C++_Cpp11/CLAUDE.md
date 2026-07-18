# C++11 語言特性學習專案

## 使用者需求 / 規則

> 5 條專案永久規則。

1. **永遠用繁體中文回答**。
2. **每個主題加入「簡單實用」的 LeetCode 範例**（語言特性單純的主題可省）。
3. **每個程式只談一個主題**。
4. **範例要簡單直接**。
5. **參考來源** —
   - https://en.cppreference.com/w/cpp/11
   - https://en.cppreference.com/w/cpp/language

## 範圍

本專案只放「**C++11 引入的語言特性**」 — 跟標準函式庫相關（lambda、chrono、smart pointer、thread、initializer_list 等）已有獨立專題，這裡不重複。每個檔案聚焦一個關鍵字／一個語法。

## 專案結構

```
C++_Cpp11/
├── CLAUDE.md
├── Makefile
├── 01_auto.cpp                  # auto 型別推導
├── 02_decltype.cpp              # decltype 取運算式型別
├── 03_nullptr.cpp               # nullptr 取代 NULL / 0
├── 04_range_based_for.cpp       # for(auto& x : c)
├── 05_brace_init.cpp            # {} 統一初始化、防 narrow conversion
├── 06_initializer_list.cpp      # std::initializer_list
├── 07_default_delete.cpp        # = default / = delete
├── 08_override_final.cpp        # override / final
├── 09_constexpr.cpp             # 編譯期常數函式 (C++11 限制版)
├── 10_static_assert.cpp         # 編譯期斷言
├── 11_type_alias.cpp            # using 取代 typedef
├── 12_enum_class.cpp            # 強型別 enum
├── 13_user_defined_literals.cpp # operator"" 自訂字面量
├── 14_raw_string.cpp            # R"(...)" 原始字串
└── 15_trailing_return.cpp       # auto f() -> T
```

## 編譯

```bash
make
make run
make clean
```
