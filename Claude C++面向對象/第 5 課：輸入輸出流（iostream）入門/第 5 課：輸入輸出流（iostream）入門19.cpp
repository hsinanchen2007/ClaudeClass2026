// =============================================================================
//  第 5 課：輸入輸出流（iostream）入門19.cpp  —  本課總整合：iostream 完整功能展示
// =============================================================================
//
//  ⚠️ 檔案結構說明：本檔【前半段（第 1~968 行）是一個巨大的 /* */ 註解區塊】，
//     裡面是本課的完整 Markdown 講義，含有十幾段獨立的 int main() 範例。
//     那些程式碼【不會被編譯】。真正會編譯執行的程式從第 971 行的
//     #include 開始。用工具搜尋本檔的函式時，請注意同名的
//     printSeparator / main 各出現兩次（一次在講義裡、一次是真實程式碼），
//     要以【最後一次出現】為準。
//
// 【主題資訊 Information】
//   本檔整合第 5 課全部主題，可視為 iostream 的一站式參考：
//     std::cin / std::cout / std::cerr / std::clog     — 四個標準串流物件
//     std::getline(is, str)                            — 讀整行（<string>）
//     cin.fail() / eof() / clear() / ignore()          — 錯誤偵測與復原
//     std::fixed / scientific / defaultfloat           — 浮點格式（defaultfloat 為 C++11）
//     std::setprecision / setw / setfill               — <iomanip> 帶參數 manipulator
//     std::left / right / boolalpha / noboolalpha      — 對齊與布林顯示
//     std::dec / hex / oct / showbase / noshowbase     — 整數進位制
//   標準版本：本檔可執行部分只用到 C++11 起就有的設施
//             （std::defaultfloat 是唯一的 C++11 相依項；其餘皆 C++98）
//   標頭檔  ：<iostream>、<iomanip>、<string>、<limits>
//
// 【詳細解釋 Explanation】
//
// 【1. readInt / readDouble：把「安全讀取」封裝成可重用的函式】
//   這是本檔在工程上最有價值的部分。兩個函式都做了三件事：
//     ① 迴圈重試，直到讀到合法值
//     ② 區分【格式錯誤】與【EOF】—— 後者必須有出口，否則無限空轉
//     ③ 成功後也呼叫 ignore()，把該行剩下的內容清乾淨
//   第 ③ 點特別值得注意：即使讀取成功，緩衝區裡仍留著換行符
//   （以及使用者可能多打的字）。先清掉，下一次無論是 >> 還是 getline
//   都能從乾淨的狀態開始 —— 這讓函式變成【可組合】的。
//   對照 main 裡的用法：getline 讀名字之後，緊接著就是 readInt，
//   兩者能無縫銜接正是因為每個函式都負責清理自己的殘留。
//
// 【2. 這兩個函式仍然有一個真實的瑕疵（本檔刻意保留）】
//   readInt 內的 int value; 【沒有初始值】，而 EOF 分支直接 return value。
//   問題出在 operator>> 的 sentry：
//     • sentry 成功但解析失敗（讀到 "abc"）→ num_get 執行 → C++11 保證寫入 0
//     • sentry 失敗（跳空白時碰到 EOF，或 eofbit 早已立起）
//       → num_get 【根本沒被呼叫】→ value 完全沒被寫入
//   本機用染色探針（先把 value 設成 -12345）實測，證實了後者：
//       輸入耗盡時 readInt 回傳的正是 -12345（染色值原封不動）
//   在原始程式中 value 是未初始化的，所以【回傳它是未定義行為】。
//   → 這就是為什麼下方「執行」那一行必須餵滿三筆輸入。
//   → 正確修法只要一個字：int value = 0;
//
// 【3. 為什麼提示字串會全部擠在同一行】
//   用管線餵輸入時，三次讀取之間沒有任何等待，
//   於是「請輸入你的名字: 」「請輸入你的年齡: 」「請輸入你的身高(cm): 」
//   會連續印出。互動執行時則是一問一答。
//   輸出的【順序】兩者完全相同，只有【時間間隔】不同。
//
// 【4. 第 3 節表格的中文對齊為什麼是歪的】
//   這是本檔最好的「非預期教材」。輸出中的商品表格：
//       商品         單價    數量  小計
//       蘋果         35.00     3       105.00
//   欄位並沒有真的對齊。原因是 std::setw 計算的是【char 的個數】，
//   而在 UTF-8 下一個中文字佔 3 個 char。
//   「商品」是 2 個中文字 = 6 個 char，setw(15) 只補了 9 個空格，
//   但終端上它只顯示 4 格寬 —— 於是視覺上短了 5 格。
//   標準庫【沒有】提供「顯示寬度」的概念，
//   要正確對齊含中文的表格必須自己計算 East Asian Width。
//   （這不是本檔的 bug，而是 iostream 的真實限制，值得記住。）
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 bigNum 預設印成 1.23457e+06
//   預設的 defaultfloat 模式下 precision 是【有效位數】，預設值 6。
//   1234567.89 有 9 位整數部分，超過 6 位有效數字 →
//   自動切換成科學記號。這正是報表輸出必須明確指定 std::fixed 的理由。
//
// (B) smallNum 在 fixed + setprecision(2) 下印成 0.00
//   0.000012345 取到小數點後 2 位就是 0.00 —— 資訊完全遺失。
//   同一個數值在 scientific 模式下是 1.234e-05，資訊保留了。
//   這說明【沒有一種格式適合所有數量級】：
//   固定小數適合金額，科學記號適合物理量，
//   而 defaultfloat 是兩者的自動切換。
//
// (C) 格式狀態的清理紀律
//   本檔每一節結束都會還原格式（defaultfloat / setprecision(6) /
//   dec / noshowbase / setfill(' ') / noboolalpha）。
//   這是良好習慣：這些旗標都是【黏性】的，掛在 std::cout 這個
//   全域物件上，不還原就會污染後續所有輸出，
//   而且出問題的地方可能離設定處很遠。
//   C++20 的 std::format 從根本解決了這個問題（它不碰串流狀態）。
//
// 【注意事項 Pay Attention】
//   1. 本檔前 968 行是註解區塊，裡面的 main() 不會被編譯。
//      搜尋函式定義時請以【最後一次出現】為準。
//   2. 執行時【必須餵滿三筆輸入】（名字、年齡、身高）。
//      輸入不足會讓 readInt/readDouble 回傳未初始化的變數 → UB。
//      這是本檔刻意保留的瑕疵，修法是 int value = 0;。
//   3. cerr 與 clog 都輸出到 stderr（fd 2），只有 cout 走 stdout（fd 1）。
//      用 2>/dev/null 執行會少掉兩行。
//   4. 第 3 節的中文表格對齊是歪的 —— setw 以 char 數計算，
//      UTF-8 中文字佔 3 個 char。這是 iostream 的真實限制，不是筆誤。
//   5. 所有 manipulator 除了 setw 之外都是黏性的，用完記得還原。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】iostream 綜合
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. readInt 這種「安全讀取」函式，為什麼在【成功】的路徑上也要呼叫 ignore()？
//     答：因為 >> 成功讀到數字後會停在換行符【之前】，把 '\n' 留在緩衝區。
//         若不清掉，下一次若是 getline 就會立刻讀到空字串。
//         成功路徑也清理，函式才是【可組合】的 ——
//         呼叫者不必知道它內部用了 >> 還是 getline，
//         也不必記得自己補一次 ignore。這是封裝的價值。
//     追問：那如果使用者輸入 "25 abc" 呢？
//           → >> 讀到 25 成功，ignore 會把同一行剩下的 " abc" 一起丟掉。
//             這是刻意的設計：一行只取一個值，多餘的視為雜訊。
//
// 🔥 Q2. 這支程式的表格輸出中文對不齊，是 setw 壞了嗎？該怎麼修？
//     答：不是壞了。setw 計算的是 char 的個數，
//         而 UTF-8 下一個中文字佔 3 個 char，
//         所以「商品」（6 個 char）被當成 6 格，實際只顯示 4 格寬。
//         標準庫沒有「顯示寬度」的概念，必須自己依 East Asian Width
//         計算每個字元的顯示寬度再手動補空格。
//     追問：改用 std::wstring 或 char32_t 能解決嗎？
//           → 只解決一半。改用 wchar_t 後「商品」變成 2 個字元，
//             但顯示寬度仍是 4 —— 全形字佔 2 格。
//             字元數與顯示寬度是兩件不同的事，換型別不會自動處理。
//
// ⚠️ 陷阱. readInt 已經寫了 EOF 出口、看起來很嚴謹，
//          為什麼餵不足的輸入還是會出問題？
//     答：因為 EOF 出口 return 的是【未初始化的 value】。
//         很多人以為「C++11 起 >> 失敗會把目標設為 0」能兜底，
//         但那個保證只在 num_get 真的被呼叫時成立；
//         EOF 造成的是 sentry 階段就失敗，num_get 根本沒執行，
//         value 從未被寫入。本機染色探針（value = -12345）實測，
//         輸入耗盡時函式回傳的正是 -12345。
//     為什麼會錯：把「有處理 EOF」等同於「EOF 情況下是安全的」。
//         實際上這個函式【偵測】到了 EOF，卻在【回應】EOF 時
//         回傳了一個沒有意義的值 —— 偵測與處理是兩回事。
//         正確做法有三種：宣告時 int value = 0;（最省事）、
//         回傳 std::optional<int>（C++17，最誠實）、
//         或直接丟例外讓呼叫者決定。
// ═══════════════════════════════════════════════════════════════════════════

/*
# 第 5 課：輸入輸出流（iostream）入門

---

## 5.1 課程目標

輸入輸出是程式與使用者互動的基本方式。C++ 提供了**流（stream）**的概念來處理輸入輸出，比 C 語言的 `printf`/`scanf` 更加類型安全且易於擴展。

本課將深入學習 C++ 的 iostream 函式庫，包括基本輸入輸出、格式化控制，以及常見的使用技巧。

---

## 5.2 什麼是「流」？

**流（Stream）**是一個抽象概念，代表資料的流動：

| 流類型 | 方向 | 說明 |
|--------|------|------|
| 輸入流 | 外部 → 程式 | 從鍵盤、檔案讀取資料 |
| 輸出流 | 程式 → 外部 | 輸出到螢幕、檔案 |

想像流就像水管：資料像水一樣從一端流向另一端。

---

## 5.3 標準輸入輸出流物件

C++ 在 `<iostream>` 標頭檔中定義了四個標準流物件：

| 物件 | 類型 | 說明 | 對應 C 語言 |
|------|------|------|-------------|
| `std::cout` | `ostream` | 標準輸出（螢幕） | `stdout` |
| `std::cin` | `istream` | 標準輸入（鍵盤） | `stdin` |
| `std::cerr` | `ostream` | 標準錯誤（無緩衝） | `stderr` |
| `std::clog` | `ostream` | 標準日誌（有緩衝） | `stderr` |

---

## 5.4 基本輸出：std::cout

### 5.4.1 輸出運算子 

`<<` 稱為**插入運算子（insertion operator）**，將資料「插入」到輸出流中：

```cpp
#include <iostream>

int main() {
    std::cout << "Hello, World!";
    std::cout << std::endl;  // 換行並清空緩衝區
    
    return 0;
}
```

### 5.4.2 鏈式輸出

`<<` 運算子可以連續使用：

```cpp
#include <iostream>

int main() {
    int age = 25;
    double height = 175.5;
    
    // 鏈式輸出
    std::cout << "年齡: " << age << " 歲，身高: " << height << " cm" << std::endl;
    
    return 0;
}
```

輸出：
```
年齡: 25 歲，身高: 175.5 cm
```

### 5.4.3 自動類型識別

與 C 語言的 `printf` 不同，`cout` 會自動識別資料類型：

```cpp
#include <iostream>

int main() {
    int i = 42;
    double d = 3.14159;
    char c = 'A';
    const char* s = "Hello";
    bool b = true;
    
    // 不需要格式符，自動識別類型
    std::cout << "int: " << i << std::endl;
    std::cout << "double: " << d << std::endl;
    std::cout << "char: " << c << std::endl;
    std::cout << "string: " << s << std::endl;
    std::cout << "bool: " << b << std::endl;  // 輸出 1
    
    return 0;
}
```

輸出：
```
int: 42
double: 3.14159
char: A
string: Hello
bool: 1
```

---

## 5.5 換行方式比較

### 5.5.1 三種換行方式

```cpp
#include <iostream>

int main() {
    // 方式一：std::endl（換行 + 清空緩衝區）
    std::cout << "Line 1" << std::endl;
    
    // 方式二：'\n'（只換行，不清空緩衝區）
    std::cout << "Line 2\n";
    
    // 方式三：std::flush（只清空緩衝區，不換行）
    std::cout << "Line 3" << std::flush;
    std::cout << " continued" << std::endl;
    
    return 0;
}
```

### 5.5.2 效能考量

| 方式 | 行為 | 效能 | 使用時機 |
|------|------|------|----------|
| `std::endl` | 換行 + 清空緩衝區 | 較慢 | 需要立即顯示時 |
| `'\n'` | 只換行 | 較快 | 大量輸出時 |
| `std::flush` | 只清空緩衝區 | - | 需要立即顯示但不換行 |

> **建議**：一般情況下使用 `'\n'` 就足夠了，只有在需要確保立即顯示時才用 `std::endl`。

---

## 5.6 基本輸入：std::cin

### 5.6.1 提取運算子 >>

`>>` 稱為**提取運算子（extraction operator）**，從輸入流「提取」資料：

```cpp
#include <iostream>

int main() {
    int age;
    
    std::cout << "請輸入你的年齡: ";
    std::cin >> age;
    
    std::cout << "你的年齡是: " << age << " 歲" << std::endl;
    
    return 0;
}
```

### 5.6.2 連續輸入多個值

```cpp
#include <iostream>

int main() {
    int a, b, c;
    
    std::cout << "請輸入三個整數（用空格分隔）: ";
    std::cin >> a >> b >> c;
    
    std::cout << "你輸入的是: " << a << ", " << b << ", " << c << std::endl;
    std::cout << "總和: " << (a + b + c) << std::endl;
    
    return 0;
}
```

輸入：
```
10 20 30
```

輸出：
```
你輸入的是: 10, 20, 30
總和: 60
```

### 5.6.3 輸入不同類型

```cpp
#include <iostream>
#include <string>

int main() {
    std::string name;
    int age;
    double height;
    
    std::cout << "請輸入姓名: ";
    std::cin >> name;
    
    std::cout << "請輸入年齡: ";
    std::cin >> age;
    
    std::cout << "請輸入身高(cm): ";
    std::cin >> height;
    
    std::cout << "\n=== 個人資料 ===" << std::endl;
    std::cout << "姓名: " << name << std::endl;
    std::cout << "年齡: " << age << " 歲" << std::endl;
    std::cout << "身高: " << height << " cm" << std::endl;
    
    return 0;
}
```

---

## 5.7 cin 的特性與陷阱

### 5.7.1 空白字元處理

`>>` 運算子會跳過空白字元（空格、Tab、換行），並在遇到空白時停止讀取：

```cpp
#include <iostream>
#include <string>

int main() {
    std::string word;
    
    std::cout << "請輸入一段文字: ";
    std::cin >> word;
    
    std::cout << "你輸入的是: [" << word << "]" << std::endl;
    
    return 0;
}
```

輸入：
```
Hello World
```

輸出：
```
你輸入的是: [Hello]
```

> **注意**：只讀取了 "Hello"，"World" 還留在緩衝區中！

### 5.7.2 讀取整行：std::getline

要讀取包含空格的整行文字，使用 `std::getline`：

```cpp
#include <iostream>
#include <string>

int main() {
    std::string fullLine;
    
    std::cout << "請輸入一段文字: ";
    std::getline(std::cin, fullLine);
    
    std::cout << "你輸入的是: [" << fullLine << "]" << std::endl;
    
    return 0;
}
```

輸入：
```
Hello World
```

輸出：
```
你輸入的是: [Hello World]
```

### 5.7.3 混合使用 cin 和 getline 的陷阱

```cpp
#include <iostream>
#include <string>

int main() {
    int age;
    std::string name;
    
    std::cout << "請輸入年齡: ";
    std::cin >> age;
    
    std::cout << "請輸入姓名: ";
    std::getline(std::cin, name);  // 問題！會讀到空字串
    
    std::cout << "年齡: " << age << std::endl;
    std::cout << "姓名: [" << name << "]" << std::endl;
    
    return 0;
}
```

**問題原因**：`cin >> age` 讀取數字後，換行符 `\n` 還留在緩衝區。`getline` 讀到這個換行符就認為結束了。

### 5.7.4 解決方案

```cpp
#include <iostream>
#include <string>
#include <limits>

int main() {
    int age;
    std::string name;
    
    std::cout << "請輸入年齡: ";
    std::cin >> age;
    
    // 解決方案一：忽略緩衝區中剩餘的字元直到換行
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    // 解決方案二（簡化版）：忽略一個字元
    // std::cin.ignore();
    
    std::cout << "請輸入姓名: ";
    std::getline(std::cin, name);
    
    std::cout << "年齡: " << age << std::endl;
    std::cout << "姓名: [" << name << "]" << std::endl;
    
    return 0;
}
```

---

## 5.8 輸入錯誤處理

### 5.8.1 檢查輸入狀態

```cpp
#include <iostream>
#include <limits>

int main() {
    int number;
    
    std::cout << "請輸入一個整數: ";
    std::cin >> number;
    
    // 檢查輸入是否成功
    if (std::cin.fail()) {
        std::cout << "輸入錯誤！不是有效的整數。" << std::endl;
        
        // 清除錯誤狀態
        std::cin.clear();
        
        // 忽略錯誤的輸入
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    } else {
        std::cout << "你輸入的是: " << number << std::endl;
    }
    
    return 0;
}
```

### 5.8.2 輸入驗證迴圈

```cpp
#include <iostream>
#include <limits>

int main() {
    int number;
    
    while (true) {
        std::cout << "請輸入一個正整數: ";
        std::cin >> number;
        
        if (std::cin.fail()) {
            // ⚠️ EOF 也會讓 fail() 為 true，但 clear()+ignore() 救不了它——
            //    沒有新資料進來，這個驗證迴圈會【無限空轉】（用管線或 Ctrl-D 即可重現）。
            if (std::cin.eof()) {
                std::cout << "\n輸入結束（EOF）。" << std::endl;
                break;
            }
            // 輸入類型錯誤
            std::cout << "錯誤：請輸入數字！" << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        } else if (number <= 0) {
            // 數值範圍錯誤
            std::cout << "錯誤：必須是正整數！" << std::endl;
        } else {
            // 輸入正確
            break;
        }
    }
    
    std::cout << "你輸入的正整數是: " << number << std::endl;
    
    return 0;
}
```

---

## 5.9 流狀態標誌

| 標誌 | 方法 | 說明 |
|------|------|------|
| goodbit | `good()` | 一切正常 |
| eofbit | `eof()` | 到達檔案結尾 |
| failbit | `fail()` | 操作失敗（可恢復） |
| badbit | `bad()` | 嚴重錯誤（不可恢復） |

```cpp
#include <iostream>

int main() {
    int x;
    
    std::cin >> x;
    
    std::cout << "good: " << std::cin.good() << std::endl;
    std::cout << "eof: " << std::cin.eof() << std::endl;
    std::cout << "fail: " << std::cin.fail() << std::endl;
    std::cout << "bad: " << std::cin.bad() << std::endl;
    
    return 0;
}
```

---

## 5.10 格式化輸出

### 5.10.1 使用 iomanip 標頭檔

```cpp
#include <iostream>
#include <iomanip>  // 格式化控制器

int main() {
    double pi = 3.14159265358979;
    int num = 42;
    
    // 設定小數精度
    std::cout << "預設: " << pi << std::endl;
    std::cout << "精度3: " << std::setprecision(3) << pi << std::endl;
    std::cout << "精度8: " << std::setprecision(8) << pi << std::endl;
    
    // fixed 固定小數點表示法
    std::cout << "fixed精度2: " << std::fixed << std::setprecision(2) 
              << pi << std::endl;
    
    return 0;
}
```

輸出：
```
預設: 3.14159
精度3: 3.14
精度8: 3.1415927
fixed精度2: 3.14
```

### 5.10.2 常用格式化控制器

| 控制器 | 說明 | 範例 |
|--------|------|------|
| `std::setw(n)` | 設定欄位寬度 | `setw(10)` |
| `std::setprecision(n)` | 設定精度 | `setprecision(3)` |
| `std::fixed` | 固定小數點格式 | `3.14` |
| `std::scientific` | 科學記號格式 | `3.14e+00` |
| `std::setfill(c)` | 設定填充字元 | `setfill('0')` |
| `std::left` | 靠左對齊 | |
| `std::right` | 靠右對齊（預設） | |
| `std::boolalpha` | 布林值顯示 true/false | |
| `std::hex` | 十六進位 | |
| `std::oct` | 八進位 | |
| `std::dec` | 十進位（預設） | |

### 5.10.3 欄位寬度與對齊

```cpp
#include <iostream>
#include <iomanip>

int main() {
    std::cout << "=== 欄位寬度與對齊 ===" << std::endl;
    
    // setw 只對下一個輸出有效
    std::cout << "[" << std::setw(10) << 42 << "]" << std::endl;
    std::cout << "[" << std::setw(10) << "Hello" << "]" << std::endl;
    
    // 靠左對齊
    std::cout << "[" << std::left << std::setw(10) << 42 << "]" << std::endl;
    std::cout << "[" << std::left << std::setw(10) << "Hello" << "]" << std::endl;
    
    // 使用填充字元
    std::cout << "[" << std::right << std::setfill('0') << std::setw(6) 
              << 42 << "]" << std::endl;
    
    // 恢復預設填充字元
    std::cout << std::setfill(' ');
    
    return 0;
}
```

輸出：
```
=== 欄位寬度與對齊 ===
[        42]
[     Hello]
[42        ]
[Hello     ]
[000042]
```

### 5.10.4 數值進位顯示

```cpp
#include <iostream>
#include <iomanip>

int main() {
    int num = 255;
    
    std::cout << "十進位: " << std::dec << num << std::endl;
    std::cout << "十六進位: " << std::hex << num << std::endl;
    std::cout << "八進位: " << std::oct << num << std::endl;
    
    // 顯示進位前綴
    std::cout << std::showbase;
    std::cout << "十進位: " << std::dec << num << std::endl;
    std::cout << "十六進位: " << std::hex << num << std::endl;
    std::cout << "八進位: " << std::oct << num << std::endl;
    
    // 恢復預設
    std::cout << std::dec << std::noshowbase;
    
    return 0;
}
```

輸出：
```
十進位: 255
十六進位: ff
八進位: 377
十進位: 255
十六進位: 0xff
八進位: 0377
```

---

## 5.11 cerr 與 clog

### 5.11.1 標準錯誤輸出

```cpp
#include <iostream>

int main() {
    // 正常輸出
    std::cout << "這是正常訊息" << std::endl;
    
    // 錯誤輸出（無緩衝，立即顯示）
    std::cerr << "這是錯誤訊息" << std::endl;
    
    // 日誌輸出（有緩衝）
    std::clog << "這是日誌訊息" << std::endl;
    
    return 0;
}
```

### 5.11.2 用途差異

| 流 | 用途 | 緩衝 |
|------|------|------|
| `cout` | 正常程式輸出 | 有 |
| `cerr` | 錯誤訊息（需立即顯示） | 無 |
| `clog` | 日誌記錄 | 有 |

在命令列中，可以分別重導向：

```bash
./program > output.txt 2> errors.txt
```

---

## 5.12 完整範例程式

```cpp
#include <iostream>
#include <iomanip>
#include <string>
#include <limits>

// ============================================================
// iostream 完整功能展示
// ============================================================

// 安全讀取整數
int readInt(const std::string& prompt) {
    int value;
    while (true) {
        std::cout << prompt;
        std::cin >> value;
        
        if (std::cin.fail()) {
            // ⚠️ EOF 也會讓 fail() 為 true，但 clear()+ignore() 救不了它——
            //    沒有新資料進來，這個驗證迴圈會【無限空轉】（用管線或 Ctrl-D 即可重現）。
            if (std::cin.eof()) {
                std::cout << "\n輸入結束（EOF）。" << std::endl;
                return value;   // 沒有更多輸入，回傳目前值
            }
            std::cout << "  錯誤：請輸入有效的整數！" << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        } else {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }
    }
}

// 安全讀取浮點數
double readDouble(const std::string& prompt) {
    double value;
    while (true) {
        std::cout << prompt;
        std::cin >> value;
        
        if (std::cin.fail()) {
            // ⚠️ EOF 也會讓 fail() 為 true，但 clear()+ignore() 救不了它——
            //    沒有新資料進來，這個驗證迴圈會【無限空轉】（用管線或 Ctrl-D 即可重現）。
            if (std::cin.eof()) {
                std::cout << "\n輸入結束（EOF）。" << std::endl;
                return value;   // 沒有更多輸入，回傳目前值
            }
            std::cout << "  錯誤：請輸入有效的數字！" << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        } else {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }
    }
}

// 顯示分隔線
void printSeparator(const std::string& title) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  " << title << std::endl;
    std::cout << "========================================" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    iostream 完整功能展示" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // --------------------------------------------------
    // 1. 基本輸入輸出
    // --------------------------------------------------
    printSeparator("1. 基本輸入輸出");
    
    std::string name;
    std::cout << "請輸入你的名字: ";
    std::getline(std::cin, name);
    
    int age = readInt("請輸入你的年齡: ");
    double height = readDouble("請輸入你的身高(cm): ");
    
    std::cout << "\n你好，" << name << "！" << std::endl;
    std::cout << "你今年 " << age << " 歲，身高 " << height << " cm" << std::endl;
    
    // --------------------------------------------------
    // 2. 數值格式化
    // --------------------------------------------------
    printSeparator("2. 數值格式化");
    
    double pi = 3.14159265358979;
    double bigNum = 1234567.89;
    double smallNum = 0.000012345;
    
    std::cout << "預設輸出:" << std::endl;
    std::cout << "  pi = " << pi << std::endl;
    std::cout << "  bigNum = " << bigNum << std::endl;
    std::cout << "  smallNum = " << smallNum << std::endl;
    
    std::cout << "\nfixed + setprecision(2):" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  pi = " << pi << std::endl;
    std::cout << "  bigNum = " << bigNum << std::endl;
    std::cout << "  smallNum = " << smallNum << std::endl;
    
    std::cout << "\nscientific + setprecision(3):" << std::endl;
    std::cout << std::scientific << std::setprecision(3);
    std::cout << "  pi = " << pi << std::endl;
    std::cout << "  bigNum = " << bigNum << std::endl;
    std::cout << "  smallNum = " << smallNum << std::endl;
    
    // 恢復預設
    std::cout << std::defaultfloat << std::setprecision(6);
    
    // --------------------------------------------------
    // 3. 表格輸出
    // --------------------------------------------------
    printSeparator("3. 表格輸出");
    
    std::cout << std::left;
    std::cout << std::setw(15) << "商品" 
              << std::setw(10) << "單價" 
              << std::setw(8) << "數量" 
              << std::setw(12) << "小計" << std::endl;
    std::cout << std::string(45, '-') << std::endl;
    
    std::cout << std::fixed << std::setprecision(2);
    
    std::cout << std::setw(15) << "蘋果" 
              << std::setw(10) << 35.0
              << std::setw(8) << 3
              << std::setw(12) << 35.0 * 3 << std::endl;
              
    std::cout << std::setw(15) << "香蕉" 
              << std::setw(10) << 25.5
              << std::setw(8) << 5
              << std::setw(12) << 25.5 * 5 << std::endl;
              
    std::cout << std::setw(15) << "橘子" 
              << std::setw(10) << 40.0
              << std::setw(8) << 2
              << std::setw(12) << 40.0 * 2 << std::endl;
    
    std::cout << std::string(45, '-') << std::endl;
    std::cout << std::setw(33) << "總計: " 
              << std::setw(12) << (35.0*3 + 25.5*5 + 40.0*2) << std::endl;
    
    // --------------------------------------------------
    // 4. 進位轉換
    // --------------------------------------------------
    printSeparator("4. 進位轉換");
    
    int value = 255;
    
    std::cout << std::dec << std::noshowbase;
    std::cout << "數值 " << value << " 的不同進位表示：" << std::endl;
    std::cout << "  十進位:   " << std::dec << value << std::endl;
    std::cout << "  十六進位: " << std::hex << value << std::endl;
    std::cout << "  八進位:   " << std::oct << value << std::endl;
    
    std::cout << "\n帶前綴顯示：" << std::endl;
    std::cout << std::showbase;
    std::cout << "  十進位:   " << std::dec << value << std::endl;
    std::cout << "  十六進位: " << std::hex << value << std::endl;
    std::cout << "  八進位:   " << std::oct << value << std::endl;
    
    // 恢復預設
    std::cout << std::dec << std::noshowbase;
    
    // --------------------------------------------------
    // 5. 布林值與對齊
    // --------------------------------------------------
    printSeparator("5. 布林值與對齊");
    
    bool trueVal = true;
    bool falseVal = false;
    
    std::cout << "預設布林輸出:" << std::endl;
    std::cout << "  true = " << trueVal << ", false = " << falseVal << std::endl;
    
    std::cout << "boolalpha 布林輸出:" << std::endl;
    std::cout << std::boolalpha;
    std::cout << "  true = " << trueVal << ", false = " << falseVal << std::endl;
    
    std::cout << "\n對齊示範:" << std::endl;
    std::cout << "  靠右: [" << std::right << std::setw(10) << 42 << "]" << std::endl;
    std::cout << "  靠左: [" << std::left << std::setw(10) << 42 << "]" << std::endl;
    std::cout << "  補零: [" << std::right << std::setfill('0') << std::setw(6) 
              << 42 << "]" << std::endl;
    
    // 恢復預設
    std::cout << std::setfill(' ') << std::noboolalpha;
    
    // --------------------------------------------------
    // 6. 錯誤輸出
    // --------------------------------------------------
    printSeparator("6. 標準輸出流比較");
    
    std::cout << "cout: 標準輸出（有緩衝）" << std::endl;
    std::cerr << "cerr: 標準錯誤（無緩衝）" << std::endl;
    std::clog << "clog: 日誌輸出（有緩衝）" << std::endl;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "    展示完成" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
```

### 編譯與執行

```bash
g++ -std=c++17 -Wall -Wextra lesson05.cpp -o lesson05
./lesson05
```

### 執行結果

```
========================================
    iostream 完整功能展示
========================================

========================================
  1. 基本輸入輸出
========================================
請輸入你的名字: 陳信安
請輸入你的年齡: 25
請輸入你的身高(cm): 175.5

你好，陳信安！
你今年 25 歲，身高 175.5 cm

========================================
  2. 數值格式化
========================================
預設輸出:
  pi = 3.14159
  bigNum = 1.23457e+06
  smallNum = 1.2345e-05

fixed + setprecision(2):
  pi = 3.14
  bigNum = 1234567.89
  smallNum = 0.00

scientific + setprecision(3):
  pi = 3.142e+00
  bigNum = 1.235e+06
  smallNum = 1.235e-05

========================================
  3. 表格輸出
========================================
商品           單價      數量    小計        
---------------------------------------------
蘋果           35.00     3       105.00      
香蕉           25.50     5       127.50      
橘子           40.00     2       80.00       
---------------------------------------------
                         總計: 312.50      

========================================
  4. 進位轉換
========================================
數值 255 的不同進位表示：
  十進位:   255
  十六進位: ff
  八進位:   377

帶前綴顯示：
  十進位:   255
  十六進位: 0xff
  八進位:   0377

========================================
  5. 布林值與對齊
========================================
預設布林輸出:
  true = 1, false = 0
boolalpha 布林輸出:
  true = true, false = false

對齊示範:
  靠右: [        42]
  靠左: [42        ]
  補零: [000042]

========================================
  6. 標準輸出流比較
========================================
cout: 標準輸出（有緩衝）
cerr: 標準錯誤（無緩衝）
clog: 日誌輸出（有緩衝）

========================================
    展示完成
========================================
```

---

## 5.13 iostream vs printf/scanf 比較

| 特性 | printf/scanf | iostream |
|------|--------------|----------|
| 類型安全 | 需手動指定格式符 | 自動識別類型 |
| 可擴展性 | 無法用於自定義類型 | 可重載 << 和 >> |
| 效能 | 通常較快 | 稍慢（但差異微小） |
| 國際化 | 較困難 | 支援 locale |
| 錯誤檢測 | 運行時錯誤 | 部分編譯時檢測 |
| 程式風格 | C 風格 | C++ 風格 |

---

## 5.14 本課重點回顧

| 概念 | 說明 |
|------|------|
| cout | 標準輸出，使用 `<<` 運算子 |
| cin | 標準輸入，使用 `>>` 運算子 |
| endl vs \n | `endl` 會清空緩衝區，`\n` 只換行 |
| getline | 讀取整行（包含空格） |
| cin.ignore() | 忽略緩衝區中的字元 |
| cin.fail() | 檢查輸入是否失敗 |
| cin.clear() | 清除錯誤狀態 |
| iomanip | 格式化輸出的控制器 |
| cerr / clog | 錯誤和日誌輸出流 |

---

## 5.15 第一階段完成！

恭喜你完成了**第一階段：從 C 到 C++ 的過渡**！

你已經學習了：
- C++ 的歷史與設計哲學
- C 與 C++ 的關鍵差異
- 編譯環境設置
- 命名空間
- 輸入輸出流

現在你已經具備了開始學習 C++ 面向對象程式設計的基礎。

---

## 5.16 下一課預告

下一課我們將正式進入**第二階段：類別與對象基礎**，首先探討**什麼是面向對象？核心思想介紹**，為後續的類別學習打下思想基礎。

準備好進入 **第 6 課：什麼是面向對象？核心思想介紹** 了嗎？
*/



#include <iostream>
#include <iomanip>
#include <string>
#include <limits>

// ============================================================
// iostream 完整功能展示
// ============================================================

// 安全讀取整數
int readInt(const std::string& prompt) {
    int value;
    while (true) {
        std::cout << prompt;
        std::cin >> value;
        
        if (std::cin.fail()) {
            // ⚠️ EOF 也會讓 fail() 為 true，但 clear()+ignore() 救不了它——
            //    沒有新資料進來，這個驗證迴圈會【無限空轉】（用管線或 Ctrl-D 即可重現）。
            if (std::cin.eof()) {
                std::cout << "\n輸入結束（EOF）。" << std::endl;
                return value;   // 沒有更多輸入，回傳目前值
            }
            std::cout << "  錯誤：請輸入有效的整數！" << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        } else {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }
    }
}

// 安全讀取浮點數
double readDouble(const std::string& prompt) {
    double value;
    while (true) {
        std::cout << prompt;
        std::cin >> value;
        
        if (std::cin.fail()) {
            // ⚠️ EOF 也會讓 fail() 為 true，但 clear()+ignore() 救不了它——
            //    沒有新資料進來，這個驗證迴圈會【無限空轉】（用管線或 Ctrl-D 即可重現）。
            if (std::cin.eof()) {
                std::cout << "\n輸入結束（EOF）。" << std::endl;
                return value;   // 沒有更多輸入，回傳目前值
            }
            std::cout << "  錯誤：請輸入有效的數字！" << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        } else {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }
    }
}

// 顯示分隔線
void printSeparator(const std::string& title) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  " << title << std::endl;
    std::cout << "========================================" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    iostream 完整功能展示" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // --------------------------------------------------
    // 1. 基本輸入輸出
    // --------------------------------------------------
    printSeparator("1. 基本輸入輸出");
    
    std::string name;
    std::cout << "請輸入你的名字: ";
    std::getline(std::cin, name);
    
    int age = readInt("請輸入你的年齡: ");
    double height = readDouble("請輸入你的身高(cm): ");
    
    std::cout << "\n你好，" << name << "！" << std::endl;
    std::cout << "你今年 " << age << " 歲，身高 " << height << " cm" << std::endl;
    
    // --------------------------------------------------
    // 2. 數值格式化
    // --------------------------------------------------
    printSeparator("2. 數值格式化");
    
    double pi = 3.14159265358979;
    double bigNum = 1234567.89;
    double smallNum = 0.000012345;
    
    std::cout << "預設輸出:" << std::endl;
    std::cout << "  pi = " << pi << std::endl;
    std::cout << "  bigNum = " << bigNum << std::endl;
    std::cout << "  smallNum = " << smallNum << std::endl;
    
    std::cout << "\nfixed + setprecision(2):" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  pi = " << pi << std::endl;
    std::cout << "  bigNum = " << bigNum << std::endl;
    std::cout << "  smallNum = " << smallNum << std::endl;
    
    std::cout << "\nscientific + setprecision(3):" << std::endl;
    std::cout << std::scientific << std::setprecision(3);
    std::cout << "  pi = " << pi << std::endl;
    std::cout << "  bigNum = " << bigNum << std::endl;
    std::cout << "  smallNum = " << smallNum << std::endl;
    
    // 恢復預設
    std::cout << std::defaultfloat << std::setprecision(6);
    
    // --------------------------------------------------
    // 3. 表格輸出
    // --------------------------------------------------
    printSeparator("3. 表格輸出");
    
    std::cout << std::left;
    std::cout << std::setw(15) << "商品" 
              << std::setw(10) << "單價" 
              << std::setw(8) << "數量" 
              << std::setw(12) << "小計" << std::endl;
    std::cout << std::string(45, '-') << std::endl;
    
    std::cout << std::fixed << std::setprecision(2);
    
    std::cout << std::setw(15) << "蘋果" 
              << std::setw(10) << 35.0
              << std::setw(8) << 3
              << std::setw(12) << 35.0 * 3 << std::endl;
              
    std::cout << std::setw(15) << "香蕉" 
              << std::setw(10) << 25.5
              << std::setw(8) << 5
              << std::setw(12) << 25.5 * 5 << std::endl;
              
    std::cout << std::setw(15) << "橘子" 
              << std::setw(10) << 40.0
              << std::setw(8) << 2
              << std::setw(12) << 40.0 * 2 << std::endl;
    
    std::cout << std::string(45, '-') << std::endl;
    std::cout << std::setw(33) << "總計: " 
              << std::setw(12) << (35.0*3 + 25.5*5 + 40.0*2) << std::endl;
    
    // --------------------------------------------------
    // 4. 進位轉換
    // --------------------------------------------------
    printSeparator("4. 進位轉換");
    
    int value = 255;
    
    std::cout << std::dec << std::noshowbase;
    std::cout << "數值 " << value << " 的不同進位表示：" << std::endl;
    std::cout << "  十進位:   " << std::dec << value << std::endl;
    std::cout << "  十六進位: " << std::hex << value << std::endl;
    std::cout << "  八進位:   " << std::oct << value << std::endl;
    
    std::cout << "\n帶前綴顯示：" << std::endl;
    std::cout << std::showbase;
    std::cout << "  十進位:   " << std::dec << value << std::endl;
    std::cout << "  十六進位: " << std::hex << value << std::endl;
    std::cout << "  八進位:   " << std::oct << value << std::endl;
    
    // 恢復預設
    std::cout << std::dec << std::noshowbase;
    
    // --------------------------------------------------
    // 5. 布林值與對齊
    // --------------------------------------------------
    printSeparator("5. 布林值與對齊");
    
    bool trueVal = true;
    bool falseVal = false;
    
    std::cout << "預設布林輸出:" << std::endl;
    std::cout << "  true = " << trueVal << ", false = " << falseVal << std::endl;
    
    std::cout << "boolalpha 布林輸出:" << std::endl;
    std::cout << std::boolalpha;
    std::cout << "  true = " << trueVal << ", false = " << falseVal << std::endl;
    
    std::cout << "\n對齊示範:" << std::endl;
    std::cout << "  靠右: [" << std::right << std::setw(10) << 42 << "]" << std::endl;
    std::cout << "  靠左: [" << std::left << std::setw(10) << 42 << "]" << std::endl;
    std::cout << "  補零: [" << std::right << std::setfill('0') << std::setw(6) 
              << 42 << "]" << std::endl;
    
    // 恢復預設
    std::cout << std::setfill(' ') << std::noboolalpha;
    
    // --------------------------------------------------
    // 6. 錯誤輸出
    // --------------------------------------------------
    printSeparator("6. 標準輸出流比較");
    
    std::cout << "cout: 標準輸出（有緩衝）" << std::endl;
    std::cerr << "cerr: 標準錯誤（無緩衝）" << std::endl;
    std::clog << "clog: 日誌輸出（有緩衝）" << std::endl;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "    展示完成" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 5 課：輸入輸出流（iostream）入門19.cpp" -o io19
// 執行: printf '小明\n25\n172.5\n' | ./io19 2>&1
//       ⚠️【必須餵滿三筆輸入】—— 輸入不足會讓 readInt / readDouble
//         回傳未初始化的變數（見檔頭【2.】的染色探針實測），屬未定義行為。

// 註 1:本檔前 968 行是 /* */ 註解區塊（本課 Markdown 講義），
//      裡面十幾個 int main() 都【不會被編譯】。真正的程式從第 971 行開始。

// 註 2:下方輸出以 2>&1 合併 stdout 與 stderr 取得。
//      第 6 節的 cerr / clog 兩行走的是 stderr（fd 2），
//      若用 2>/dev/null 執行就會少掉那兩行。

// 註 3:本檔輸出在【餵固定輸入】的前提下是完全確定的 ——
//      連跑 30 次（含 2>&1 合併與只取 stdout 兩種方式）
//      位元組完全相同。三個串流的相對順序之所以穩定，
//      是因為那三行都用了 std::endl（每行都強制 flush），
//      而且 cerr 被 tie 到 cout。若改成 "\\n" 就不保證順序了。

// 註 4:第 3 節的商品表格【中文欄位是對不齊的】—— 這不是筆誤。
//      std::setw 以 char 個數計算，UTF-8 下一個中文字佔 3 個 char，
//      所以「蘋果」被當成 6 格、實際只顯示 4 格寬。
//      這是 iostream 的真實限制，標準庫沒有「顯示寬度」的概念。

// 註 5:第 6 節印出的「cerr: 標準錯誤（無緩衝）」是原始教材的措辭。
//      更精確的說法是 cerr 設定了 unitbuf 旗標（每次輸出後自動 flush），
//      而不是嚴格意義的無緩衝；同課 18 號檔有本機 flags() 的實測數據。

// === 預期輸出 ===
// ========================================
//     iostream 完整功能展示
// ========================================
//
// ========================================
//   1. 基本輸入輸出
// ========================================
// 請輸入你的名字: 請輸入你的年齡: 請輸入你的身高(cm):
// 你好，小明！
// 你今年 25 歲，身高 172.5 cm
//
// ========================================
//   2. 數值格式化
// ========================================
// 預設輸出:
//   pi = 3.14159
//   bigNum = 1.23457e+06
//   smallNum = 1.2345e-05
//
// fixed + setprecision(2):
//   pi = 3.14
//   bigNum = 1234567.89
//   smallNum = 0.00
//
// scientific + setprecision(3):
//   pi = 3.142e+00
//   bigNum = 1.235e+06
//   smallNum = 1.234e-05
//
// ========================================
//   3. 表格輸出
// ========================================
// 商品         單價    數量  小計
// ---------------------------------------------
// 蘋果         35.00     3       105.00
// 香蕉         25.50     5       127.50
// 橘子         40.00     2       80.00
// ---------------------------------------------
// 總計:                          312.50
//
// ========================================
//   4. 進位轉換
// ========================================
// 數值 255 的不同進位表示：
//   十進位:   255
//   十六進位: ff
//   八進位:   377
//
// 帶前綴顯示：
//   十進位:   255
//   十六進位: 0xff
//   八進位:   0377
//
// ========================================
//   5. 布林值與對齊
// ========================================
// 預設布林輸出:
//   true = 1, false = 0
// boolalpha 布林輸出:
//   true = true, false = false
//
// 對齊示範:
//   靠右: [        42]
//   靠左: [42        ]
//   補零: [000042]
//
// ========================================
//   6. 標準輸出流比較
// ========================================
// cout: 標準輸出（有緩衝）
// cerr: 標準錯誤（無緩衝）
// clog: 日誌輸出（有緩衝）
//
// ========================================
//     展示完成
// ========================================
