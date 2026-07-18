# C++ IOStream 學習專案

## 使用者需求 / 規則

> 5 條專案永久規則。

1. **永遠用繁體中文回答**。
2. **每個主題加入「簡單實用」的範例**，含 LeetCode 或工作上會用到的小工具。
3. **每個程式只談一個主題**。
4. **範例要簡單直接**。
5. **參考來源** —
   - https://en.cppreference.com/w/cpp/io
   - https://cplusplus.com/reference/iolibrary/

## 專案結構

```
C++_IOStream/
├── CLAUDE.md
├── Makefile
├── 01_overview.cpp           # iostream 體系：streams、buffers、locale
├── 02_cout_cin.cpp           # 基本輸入輸出、operator<< / >>
├── 03_manipulators.cpp       # setw / setfill / fixed / hex / boolalpha / setprecision
├── 04_sync_with_stdio.cpp    # 加速 IO 的姿勢（競賽、批次處理）
├── 05_fstream_text.cpp       # ifstream / ofstream 文字檔讀寫
├── 06_fstream_binary.cpp     # 二進位檔案 read/write，POD 序列化
├── 07_stringstream.cpp       # istringstream / ostringstream 字串解析與組裝
├── 08_getline.cpp            # getline 行讀取與常見陷阱
├── 09_error_state.cpp        # eof / fail / bad / good，正確判斷讀取結束
└── 10_practical_log.cpp      # 工作小工具：簡單 logger
```

## 速查表

| 想做什麼 | 用什麼 |
|---|---|
| 標準輸出/輸入 | `std::cout` / `std::cin` |
| 錯誤輸出（不被 buffer） | `std::cerr` |
| 文字檔讀寫 | `std::ifstream` / `std::ofstream` |
| 二進位檔讀寫 | `fstream + std::ios::binary` |
| 字串當 stream 處理 | `std::istringstream` / `std::ostringstream` |
| 整行讀取 | `std::getline(stream, str)` |
| 加速 IO | `std::ios::sync_with_stdio(false); std::cin.tie(nullptr);` |
| 浮點精度 | `std::setprecision(N) << std::fixed` |
| 對齊 | `std::setw(N) << std::left/right` |

## 編譯

```bash
make
make run
make clean
```
