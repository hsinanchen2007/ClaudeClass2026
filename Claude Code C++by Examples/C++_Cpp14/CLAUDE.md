# C++14 語言特性學習專案

## 使用者需求 / 規則

> 5 條專案永久規則。

1. **永遠用繁體中文回答**。
2. **每個主題加入「簡單實用」的 LeetCode 範例**（語言特性單純可省）。
3. **每個程式只談一個主題**。
4. **範例要簡單直接**。
5. **參考來源** —
   - https://en.cppreference.com/w/cpp/14
   - https://en.cppreference.com/w/cpp/language

## 範圍

C++14 是 C++11 的「補強版」，多數是「放寬限制」與小語法增強。本專案聚焦
語言特性（generic lambda 與 init capture 已在 C++_Lambda 詳述，這裡省）。

## 專案結構

```
C++_Cpp14/
├── CLAUDE.md
├── Makefile
├── 01_auto_return_type.cpp      # 一般函式可用 auto 推導 return type
├── 02_decltype_auto.cpp         # decltype(auto) — auto 的精確版
├── 03_constexpr_relaxed.cpp     # constexpr 函式可用 if / for / 區域變數
├── 04_binary_literals.cpp       # 0b1010 二進位字面量
├── 05_digit_separator.cpp       # 1'000'000 千位分隔符
├── 06_make_unique.cpp           # std::make_unique
└── 07_deprecated_attribute.cpp  # [[deprecated]]
```

## 編譯

```bash
make
make run
make clean
```
