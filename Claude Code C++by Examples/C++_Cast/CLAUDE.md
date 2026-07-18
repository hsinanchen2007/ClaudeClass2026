# C++ Cast 學習專案

## 使用者需求 / 規則

> 5 條專案永久規則。

1. **永遠用繁體中文回答**。
2. **每個主題加入「簡單實用」的範例**。
3. **每個程式只談一個主題**。一個 `.cpp` 聚焦一種 cast。
4. **範例要簡單直接**。
5. **參考來源** —
   - https://en.cppreference.com/w/cpp/language/cast
   - https://en.cppreference.com/w/cpp/language/static_cast
   - https://en.cppreference.com/w/cpp/language/dynamic_cast
   - https://en.cppreference.com/w/cpp/language/const_cast
   - https://en.cppreference.com/w/cpp/language/reinterpret_cast
   - https://en.cppreference.com/w/cpp/numeric/bit_cast

## 專案結構

```
C++_Cast/
├── CLAUDE.md
├── Makefile
├── 01_overview.cpp           # 五種 cast 速覽 + 為何不用 C-style cast
├── 02_static_cast.cpp        # static_cast：數值轉換、上下轉型、void* 轉換
├── 03_dynamic_cast.cpp       # dynamic_cast：RTTI 安全向下轉型
├── 04_const_cast.cpp         # const_cast：移除 const，何時可用、UB 邊界
├── 05_reinterpret_cast.cpp   # reinterpret_cast：重新解釋 bit pattern
├── 06_bit_cast_cpp20.cpp     # std::bit_cast：reinterpret 的安全版
├── 07_implicit_explicit.cpp  # 隱式轉換、explicit 關鍵字
└── 08_pitfalls.cpp           # 常見陷阱：narrow conversion、object slicing
```

## 速查表

| Cast              | 用途                              | 安全性 |
|-------------------|----------------------------------|-------|
| `static_cast`     | 顯式轉換（數值、void*、上下轉型）| 中    |
| `dynamic_cast`    | 多型階層中安全向下轉型            | 高    |
| `const_cast`      | 移除 / 加上 `const` `volatile`    | 危險（修改原本是 const 的物件 = UB） |
| `reinterpret_cast`| 重新解釋 bit pattern              | 極危險（容易違反 strict aliasing）|
| `bit_cast` (C++20)| 同 reinterpret 但安全且 constexpr | 高（要 trivially_copyable） |
| C-style `(T)x`    | 編譯器嘗試上面所有 — 不要用       | 低（看不出意圖） |

## 編譯

```bash
make
make run
make clean
```
