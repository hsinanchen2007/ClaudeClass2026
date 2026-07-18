# C++ Exception 學習專案

## 使用者需求 / 規則

> 5 條專案永久規則。

1. **永遠用繁體中文回答**。
2. **每個主題加入「簡單實用」的範例**。
3. **每個程式只談一個主題**。
4. **範例要簡單直接**。
5. **參考來源** —
   - https://en.cppreference.com/w/cpp/error/exception
   - https://en.cppreference.com/w/cpp/language/exceptions

## 專案結構

```
C++_Exception/
├── CLAUDE.md
├── Makefile
├── 01_basics.cpp                # try / throw / catch 基本機制
├── 02_standard_exceptions.cpp   # std::exception 階層、logic/runtime_error
├── 03_what_to_throw.cpp         # 該丟什麼物件，不該 throw int / string
├── 04_catch_by_ref.cpp          # 為什麼 catch by const& — slicing 與多型
├── 05_noexcept.cpp              # noexcept、move / 容器策略
├── 06_raii_safety.cpp           # RAII + stack unwinding 確保資源釋放
├── 07_function_try_block.cpp    # 建構子 function-try-block
├── 08_nested_exception.cpp      # std::throw_with_nested / rethrow_if_nested
└── 09_pitfalls.cpp              # destructor 丟例外、catch(...) 等坑
```

## 速查表

| 想做什麼 | 用什麼 |
|---|---|
| 丟例外 | `throw std::runtime_error{"msg"}` |
| 接住所有 | `catch (...)`（最後手段，少用） |
| 接住已知型 | `catch (const std::exception& e)` |
| 看訊息 | `e.what()` |
| 重丟 | `throw;`（保留型別資訊） |
| 不會丟 | 函式加 `noexcept` |
| 多層內外因 | `std::throw_with_nested` |

## 編譯

```bash
make
make run
make clean
```
