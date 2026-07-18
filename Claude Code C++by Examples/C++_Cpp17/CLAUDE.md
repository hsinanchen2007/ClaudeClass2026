# C++17 語言特性學習專案

## 使用者需求 / 規則

> 5 條專案永久規則。

1. **永遠用繁體中文回答**。
2. **每個主題加入「簡單實用」的 LeetCode 範例**（語言特性單純可省）。
3. **每個程式只談一個主題**。
4. **範例要簡單直接**。
5. **參考來源** —
   - https://en.cppreference.com/w/cpp/17
   - https://en.cppreference.com/w/cpp/language

## 範圍

C++17 的語言特性 — 不重複前面已有專題（filesystem 已在 C++_Filesystem，
optional/variant/any 已在 C++_Utility）。

## 專案結構

```
C++_Cpp17/
├── CLAUDE.md
├── Makefile
├── 01_structured_bindings.cpp     # auto [a, b] = ...
├── 02_if_with_init.cpp            # if (auto x = ...; cond)
├── 03_switch_with_init.cpp        # switch (auto x = ...; v)
├── 04_if_constexpr.cpp            # 編譯期 if
├── 05_inline_variables.cpp        # inline static 取代 .cpp 中的 definition
├── 06_ctad.cpp                    # Class Template Argument Deduction
├── 07_nested_namespace.cpp        # namespace A::B {}
├── 08_nodiscard.cpp               # [[nodiscard]]
├── 09_maybe_unused.cpp            # [[maybe_unused]]
├── 10_fallthrough.cpp             # [[fallthrough]] in switch
├── 11_string_view.cpp             # std::string_view (LC125)
├── 12_byte.cpp                    # std::byte
└── 13_invoke.cpp                  # std::invoke
```

## 編譯

```bash
make
make run
make clean
```
