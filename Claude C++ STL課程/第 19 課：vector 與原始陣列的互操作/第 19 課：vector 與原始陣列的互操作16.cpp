/*
# 第 19 課：vector 與原始陣列的互操作

## 一、為什麼需要互操作？

在現實的 C++ 開發中，你不可能活在一個純 STL 的世界裡。你會遇到：

- **C 語言函式庫**：OpenGL、POSIX API、SQLite 等，參數都是原始指標
- **舊有程式碼（Legacy Code）**：大量用 C 風格陣列寫的函數
- **效能敏感的底層操作**：直接操作記憶體的場景
- **第三方函式庫**：很多函式庫的介面設計為接收指標和長度

所以，理解 vector 和原始陣列之間如何安全轉換，是非常實用的技能。

---

## 二、vector 的記憶體保證

C++ 標準對 vector 有一個關鍵保證（自 C++03 起）：

> vector 的元素存放在**連續的記憶體**中。也就是說，對於一個 `vector<T> v`，如果 `v.size() > 0`，那麼 `&v[0]` 到 `&v[v.size()-1]` 的記憶體是連續的，就像一個 C 風格陣列。

這個保證讓 vector 可以直接和期望 `T*` 的 C 函數互通。

---

## 三、從 vector 到原始指標

### 3.1 使用 data()（C++11，推薦）

```cpp
#include <iostream>
#include <vector>

// 模擬一個 C 風格的函式庫函數
void c_library_function(const int* arr, int size) {
    std::cout << "C 函數收到：";
    for (int i = 0; i < size; ++i) {
        std::cout << arr[i] << " ";
    }
    std::cout << std::endl;
}

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // data() 回傳底層陣列的指標
    // 如果 vector 是 const，回傳 const T*
    // 如果 vector 非 const，回傳 T*
    int* ptr = v.data();
    const int* cptr = v.data();  // 隱式轉為 const

    std::cout << "v.data() = " << ptr << std::endl;
    std::cout << "v.data()[0] = " << ptr[0] << std::endl;
    std::cout << "v.data()[4] = " << ptr[4] << std::endl;

    // 傳給 C 函數
    c_library_function(v.data(), v.size());

    return 0;
}
```

**輸出：**
```
v.data() = 0x55b3a4c5deb0
v.data()[0] = 10
v.data()[4] = 50
C 函數收到：10 20 30 40 50
```

### 3.2 使用 &v[0]（C++03 風格）

在 C++11 之前，沒有 `data()` 成員函數，常見的寫法是 `&v[0]`：

```cpp
#include <iostream>
#include <vector>

void c_library_function(const int* arr, int size) {
    for (int i = 0; i < size; ++i) {
        std::cout << arr[i] << " ";
    }
    std::cout << std::endl;
}

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // C++03 風格：取第一個元素的位址
    int* ptr = &v[0];
    c_library_function(ptr, v.size());

    // 注意：空 vector 時 &v[0] 是未定義行為！
    std::vector<int> empty_v;
    // int* bad_ptr = &empty_v[0];  // 危險！v[0] 本身就越界了

    // data() 在空 vector 時回傳有效（但不可解參考）的指標或 nullptr
    int* safe_ptr = empty_v.data();  // 安全
    std::cout << "empty.data() = " << safe_ptr << std::endl;

    return 0;
}
```

**輸出：**
```
10 20 30 40 50
empty.data() = 0
```

> **結論：** 永遠優先使用 `data()`，不要用 `&v[0]`。`data()` 在空 vector 時也是安全的。

### 3.3 空 vector 的處理

```cpp
#include <iostream>
#include <vector>

void process(const double* arr, size_t size) {
    if (size == 0) {
        std::cout << "空陣列，不處理" << std::endl;
        return;
    }
    for (size_t i = 0; i < size; ++i) {
        std::cout << arr[i] << " ";
    }
    std::cout << std::endl;
}

int main() {
    std::vector<double> v;  // 空的

    // 安全的寫法：data() + size()
    process(v.data(), v.size());

    v = {1.1, 2.2, 3.3};
    process(v.data(), v.size());

    return 0;
}
```

**輸出：**
```
空陣列，不處理
1.1 2.2 3.3
```

---

## 四、讓 C 函數寫入 vector

有些 C 函數需要你提供一塊緩衝區，它會把資料寫進去。vector 可以完美擔任這個角色：

### 4.1 已知輸出大小

```cpp
#include <iostream>
#include <vector>
#include <cstring>  // memcpy

// 模擬一個 C 函數：把結果寫入 buffer
void fill_buffer(int* buffer, int count) {
    for (int i = 0; i < count; ++i) {
        buffer[i] = (i + 1) * 100;
    }
}

int main() {
    int count = 5;

    // 步驟 1：建立足夠大小的 vector
    std::vector<int> v(count);  // 5 個元素，初始為 0

    // 步驟 2：讓 C 函數直接寫入 vector 的底層記憶體
    fill_buffer(v.data(), count);

    // 步驟 3：vector 已經擁有資料了
    std::cout << "結果：";
    for (int x : v) {
        std::cout << x << " ";
    }
    std::cout << std::endl;

    return 0;
}
```

**輸出：**
```
結果：100 200 300 400 500
```

### 4.2 兩階段查詢模式

很多 C API 使用「先查大小，再填資料」的模式：

```cpp
#include <iostream>
#include <vector>

// 模擬一個 C API：第一次呼叫取得大小，第二次呼叫填入資料
// 回傳值是實際需要的大小
int get_system_data(int* buffer, int buffer_size) {
    // 假設系統有 7 筆資料
    const int data[] = {11, 22, 33, 44, 55, 66, 77};
    const int data_count = 7;

    if (buffer == nullptr || buffer_size < data_count) {
        // 告訴呼叫者需要多大的空間
        return data_count;
    }

    // 填入資料
    for (int i = 0; i < data_count; ++i) {
        buffer[i] = data[i];
    }
    return data_count;
}

int main() {
    // 第一階段：查詢需要多少空間
    int needed = get_system_data(nullptr, 0);
    std::cout << "需要 " << needed << " 個元素的空間" << std::endl;

    // 第二階段：配置空間並取得資料
    std::vector<int> v(needed);
    get_system_data(v.data(), v.size());

    std::cout << "取得的資料：";
    for (int x : v) {
        std::cout << x << " ";
    }
    std::cout << std::endl;

    return 0;
}
```

**輸出：**
```
需要 7 個元素的空間
取得的資料：11 22 33 44 55 66 77
```

### 4.3 使用 resize + data 而非 reserve + data

這是一個容易犯的錯誤：

```cpp
#include <iostream>
#include <vector>

void fill_data(int* buffer, int count) {
    for (int i = 0; i < count; ++i) {
        buffer[i] = i * 10;
    }
}

int main() {
    // ===== 錯誤做法：reserve =====
    {
        std::vector<int> v;
        v.reserve(5);  // 只分配記憶體，size 仍然是 0

        // 雖然記憶體已分配，但 vector 認為 size 是 0
        fill_data(v.data(), 5);  // 寫入「未被 vector 管理」的記憶體

        std::cout << "reserve 後 size = " << v.size() << std::endl;
        // v.size() 是 0，所以用 range-for 遍歷什麼都看不到
        for (int x : v) {
            std::cout << x << " ";  // 不會執行
        }
        std::cout << "（沒有輸出）" << std::endl;
    }

    // ===== 正確做法：resize =====
    {
        std::vector<int> v;
        v.resize(5);  // 分配記憶體且 size 變成 5

        fill_data(v.data(), 5);

        std::cout << "resize 後 size = " << v.size() << std::endl;
        for (int x : v) {
            std::cout << x << " ";  // 正常遍歷
        }
        std::cout << std::endl;
    }

    return 0;
}
```

**輸出：**
```
reserve 後 size = 0
（沒有輸出）
resize 後 size = 5
0 10 20 30 40
```

> **核心區別：**
> - `reserve(n)`：配置記憶體但 `size()` 不變，元素不存在
> - `resize(n)`：配置記憶體且 `size()` 變為 n，元素被建構出來

---

## 五、從原始陣列到 vector

### 5.1 建構時複製

```cpp
#include <iostream>
#include <vector>

int main() {
    // C 風格陣列
    int arr[] = {10, 20, 30, 40, 50};
    int arr_size = sizeof(arr) / sizeof(arr[0]);  // 5

    // 方法一：用指標範圍建構
    std::vector<int> v1(arr, arr + arr_size);

    // 方法二：用 std::begin / std::end（C++11）
    std::vector<int> v2(std::begin(arr), std::end(arr));

    std::cout << "v1: ";
    for (int x : v1) std::cout << x << " ";
    std::cout << std::endl;

    std::cout << "v2: ";
    for (int x : v2) std::cout << x << " ";
    std::cout << std::endl;

    return 0;
}
```

**輸出：**
```
v1: 10 20 30 40 50
v2: 10 20 30 40 50
```

### 5.2 複製部分陣列

```cpp
#include <iostream>
#include <vector>

int main() {
    int arr[] = {10, 20, 30, 40, 50, 60, 70, 80};

    // 只複製索引 2~5 的元素（30, 40, 50, 60）
    std::vector<int> v(arr + 2, arr + 6);

    std::cout << "部分複製：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;

    return 0;
}
```

**輸出：**
```
部分複製：30 40 50 60
```

### 5.3 用 assign 替換現有 vector 的內容

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {1, 2, 3};

    int arr[] = {100, 200, 300, 400};

    // assign 會清除原有內容，用新資料取代
    v.assign(std::begin(arr), std::end(arr));

    std::cout << "assign 後：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;

    return 0;
}
```

**輸出：**
```
assign 後：100 200 300 400
```

### 5.4 從動態配置的陣列複製

```cpp
#include <iostream>
#include <vector>

int main() {
    // 模擬從 C 函數收到動態配置的陣列
    int size = 5;
    int* dynamic_arr = new int[size];
    for (int i = 0; i < size; ++i) {
        dynamic_arr[i] = (i + 1) * 11;
    }

    // 複製到 vector，接管資料的生命週期
    std::vector<int> v(dynamic_arr, dynamic_arr + size);

    // 釋放原始陣列（vector 有自己的副本）
    delete[] dynamic_arr;
    dynamic_arr = nullptr;

    // vector 的資料獨立存在
    std::cout << "vector 內容：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;

    return 0;
}
```

**輸出：**
```
vector 內容：11 22 33 44 55
```

> **注意：** vector 建構時會**複製**資料，不是接管指標的所有權。所以原始的 `new[]` 記憶體仍需要 `delete[]`。

---

## 六、實際應用場景

### 6.1 與 C 標準函式庫互操作

```cpp
#include <iostream>
#include <vector>
#include <cstring>   // memcpy, memset
#include <cstdlib>   // qsort
#include <algorithm>  // std::sort（對比用）

// qsort 的比較函數
int compare_int(const void* a, const void* b) {
    return (*(const int*)a) - (*(const int*)b);
}

int main() {
    // ===== memcpy：在 vector 之間複製原始記憶體 =====
    std::vector<int> src = {50, 30, 10, 40, 20};
    std::vector<int> dst(src.size());

    std::memcpy(dst.data(), src.data(), src.size() * sizeof(int));

    std::cout << "memcpy 結果：";
    for (int x : dst) std::cout << x << " ";
    std::cout << std::endl;

    // ===== memset：批量設定記憶體 =====
    std::vector<int> zeros(5);
    std::memset(zeros.data(), 0, zeros.size() * sizeof(int));

    std::cout << "memset 結果：";
    for (int x : zeros) std::cout << x << " ";
    std::cout << std::endl;

    // ===== qsort：使用 C 的排序函數 =====
    std::vector<int> to_sort = {50, 30, 10, 40, 20};
    std::qsort(to_sort.data(), to_sort.size(), sizeof(int), compare_int);

    std::cout << "qsort 結果：";
    for (int x : to_sort) std::cout << x << " ";
    std::cout << std::endl;

    return 0;
}
```

**輸出：**
```
memcpy 結果：50 30 10 40 20
memset 結果：0 0 0 0 0
qsort 結果：10 20 30 40 50
```

> **提醒：** `memcpy` 和 `memset` 只對 **POD 型別**（Plain Old Data，如 int、double、C 結構體）安全。對含有建構子、虛擬函數的物件使用這些函數是未定義行為。對非 POD 型別，應使用 `std::copy` 等 STL 演算法。

### 6.2 與檔案 I/O 互操作

```cpp
#include <iostream>
#include <fstream>
#include <vector>

int main() {
    // ===== 寫入二進位檔 =====
    {
        std::vector<int> data = {100, 200, 300, 400, 500};

        std::ofstream ofs("data.bin", std::ios::binary);
        if (ofs) {
            // 直接把 vector 的底層記憶體寫入檔案
            ofs.write(reinterpret_cast<const char*>(data.data()),
                      data.size() * sizeof(int));
            std::cout << "寫入 " << data.size() << " 個 int 到 data.bin" << std::endl;
        }
    }

    // ===== 讀取二進位檔 =====
    {
        std::ifstream ifs("data.bin", std::ios::binary);
        if (ifs) {
            // 先取得檔案大小
            ifs.seekg(0, std::ios::end);
            size_t file_size = ifs.tellg();
            ifs.seekg(0, std::ios::beg);

            // 計算元素數量
            size_t count = file_size / sizeof(int);

            // 配置 vector 並讀入
            std::vector<int> data(count);
            ifs.read(reinterpret_cast<char*>(data.data()),
                     count * sizeof(int));

            std::cout << "從 data.bin 讀取：";
            for (int x : data) std::cout << x << " ";
            std::cout << std::endl;
        }
    }

    return 0;
}
```

**輸出：**
```
寫入 5 個 int 到 data.bin
從 data.bin 讀取：100 200 300 400 500
```

### 6.3 與作業系統 API 互操作

```cpp
#include <iostream>
#include <vector>
#include <cstring>

// 模擬一個作業系統 API（類似 Windows 的 GetComputerName 或 POSIX 的 read）
// 把字串寫入 buffer，回傳寫入的位元組數
int os_get_hostname(char* buffer, int buffer_size) {
    const char* hostname = "my-workstation";
    int len = std::strlen(hostname);

    if (buffer_size < len + 1) {
        return -1;  // 緩衝區太小
    }

    std::strcpy(buffer, hostname);
    return len;
}

int main() {
    // 用 vector<char> 當作動態緩衝區
    std::vector<char> buffer(256, '\0');

    int len = os_get_hostname(buffer.data(), buffer.size());

    if (len > 0) {
        // 可以直接用 buffer.data() 當作 C 字串
        std::cout << "主機名稱：" << buffer.data() << std::endl;
        std::cout << "長度：" << len << std::endl;

        // 也可以轉成 std::string
        std::string hostname(buffer.data(), len);
        std::cout << "std::string：" << hostname << std::endl;
    }

    return 0;
}
```

**輸出：**
```
主機名稱：my-workstation
長度：14
std::string：my-workstation
```

---

## 七、注意事項與常見錯誤

### 7.1 指標的生命週期

```cpp
#include <iostream>
#include <vector>

int main() {
    int* dangerous_ptr = nullptr;

    {
        std::vector<int> v = {10, 20, 30};
        dangerous_ptr = v.data();
        std::cout << "在作用域內：" << dangerous_ptr[0] << std::endl;  // 10，OK
    }
    // v 已經被銷毀，底層記憶體已釋放

    // dangerous_ptr 現在是懸空指標（dangling pointer）
    // std::cout << dangerous_ptr[0] << std::endl;  // 未定義行為！

    return 0;
}
```

### 7.2 重新配置後指標失效

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {10, 20, 30};
    int* ptr = v.data();

    std::cout << "push_back 前：ptr[0] = " << ptr[0] << std::endl;

    // push_back 可能觸發重新配置
    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
    }

    // ptr 可能已經失效！（和迭代器失效同理）
    // std::cout << "push_back 後：ptr[0] = " << ptr[0] << std::endl;  // 危險！

    // 正確：重新取得指標
    ptr = v.data();
    std::cout << "重新取得後：ptr[0] = " << ptr[0] << std::endl;

    return 0;
}
```

### 7.3 不要混用 vector 管理和手動管理

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {10, 20, 30};

    // 絕對不要這樣做！
    // delete[] v.data();   // 災難！vector 的解構子還會再釋放一次 → 雙重釋放
    // free(v.data());      // 同樣災難！

    // 也不要嘗試用 realloc 來「幫」vector 擴容
    // vector 有自己的記憶體管理機制，不要干涉

    return 0;
}
```

---

## 八、互操作的安全守則

把前面的內容整理成一套守則：

| 守則 | 說明 |
|------|------|
| 用 `data()` 不用 `&v[0]` | `data()` 在空 vector 時安全 |
| 用 `resize` 不用 `reserve` 來準備緩衝區 | `reserve` 不改變 `size()`，資料不被 vector 管理 |
| 傳指標時一定搭配大小 | `v.data()` + `v.size()` 成對出現 |
| 注意指標生命週期 | vector 銷毀或重新配置後，指標失效 |
| POD 型別才能用 `memcpy`/`memset` | 非 POD 型別用 `std::copy` 等 STL 演算法 |
| 不要手動釋放 `data()` 的記憶體 | vector 自行管理記憶體的配置與釋放 |
| 不要持有指標跨越修改操作 | 任何可能觸發重新配置的操作都讓指標失效 |

---

## 九、本課重點回顧

1. **vector 的連續記憶體保證**：底層就是一塊 C 風格陣列，可以直接和 C API 互通
2. **`data()` 是取得底層指標的標準方式**：比 `&v[0]` 安全，空 vector 也可用
3. **讓 C 函數寫入 vector**：用 `resize()` 預先配置空間，再傳 `data()` 給 C 函數
4. **從陣列建構 vector**：用指標範圍 `vector<int>(arr, arr + n)` 或 `std::begin/end`
5. **實務場景**：二進位檔案讀寫、作業系統 API 呼叫、C 函式庫互操作
6. **安全守則**：注意指標的生命週期、不要混用管理機制、POD 限制

---

## 課後練習

**練習一：C 函數互操作**
寫一個 C 風格函數 `void double_values(int* arr, int size)`，把陣列中每個元素乘以 2。然後建立一個 `vector<int> v = {3, 7, 2, 9, 5}`，用 `data()` 傳給這個函數，最後印出 vector 的結果。

**練習二：二進位讀寫**
建立一個 `vector<double> data = {3.14, 2.718, 1.414, 1.732}`，將它以二進位格式寫入檔案 `doubles.bin`，然後讀回另一個 vector 並驗證數值一致。

**練習三：兩階段查詢**
模擬一個 C API：`int get_primes(int* buffer, int buffer_size, int max_val)`，功能是找出小於 `max_val` 的所有質數，寫入 buffer，回傳質數個數。如果 buffer 為 `nullptr` 或空間不足，只回傳需要的個數。用 vector 搭配兩階段呼叫來取得所有小於 50 的質數。

---

準備好了就告訴我，我們進入第 20 課：**vector 效能分析與最佳實踐**，那是 vector 系列的最後一課，會把所有效能相關的知識做一個總整理。
*/

// =============================================================================
//  第 19 課 講義附錄  —  記憶體所有權：絕對不要手動釋放 vector 的緩衝區
// =============================================================================
//
// 【主題資訊 Information】
//   T* vector<T>::data() noexcept;                  // C++11 起
//   標頭檔：<vector>
//   複雜度：data() 為 O(1)
//   本檔標準：C++17
//   本檔性質：**危險示範**。所有會造成 double free 的敘述都刻意保持註解狀態，
//             檔案本身可正常編譯執行。
//
// 【詳細解釋 Explanation】
//
// 【1. 所有權（ownership）才是核心概念】
//   前面整份講義講的都是「怎麼把 vector 的緩衝區交給 C API 使用」。
//   但「使用」和「擁有」是兩件完全不同的事：
//       使用（borrow）：讀寫那塊記憶體 —— 完全合法，這正是 data() 的用途
//       擁有（own）   ：決定它何時被釋放 —— **永遠屬於 vector，不可轉移**
//   delete[] / free() 是「行使所有權」的動作。對一塊你只是借來用的記憶體
//   行使所有權，就是 double free。
//
// 【2. 為什麼 delete[] v.data() 是災難】
//   執行順序會變成：
//       ① 你呼叫 delete[]，那塊記憶體被歸還給配置器
//       ② v 離開作用域，解構子執行，它以為自己還擁有那塊記憶體，
//          於是**再釋放一次**
//   同一塊記憶體被釋放兩次，這是典型的 double free：
//   它會破壞配置器內部的 free list 結構，後果無法預測。
//   glibc 通常會偵測到並印出 "double free or corruption" 然後 abort，
//   但**這只是實作的善意行為，不是標準保證**——標準對 double free
//   的定義就是 undefined behavior，不保證任何特定結果。
//
// 【3. 連配對都不對】
//   即使拋開 double free 不談，delete[] 與 free() 在這裡本來就用錯了：
//     * vector 是透過 **allocator**（預設 std::allocator）配置記憶體的，
//       內部走的是 ::operator new，對應的釋放動作是
//       allocator.deallocate() → ::operator delete。
//     * delete[] 對應的是 new[]，它還會嘗試呼叫元素的解構子並讀取
//       陣列長度的 cookie —— 而那塊記憶體根本沒有 new[] 寫下的 cookie。
//     * free() 對應的是 malloc()，與 operator new 未必是同一套機制。
//   配對錯誤本身就是 UB，與 double free 是兩個獨立的錯誤。
//
// 【4. realloc 也不行】
//   realloc 假設記憶體來自 malloc 家族，而且它可能把資料搬到新位址並
//   釋放舊的——但 vector 內部仍保存著舊指標，完全不知情。
//   之後 vector 的任何操作都會操作到已釋放的記憶體。
//   要擴容請用 vector 自己的 reserve() / resize()，這才是它的設計方式。
//
// 【5. 那什麼時候需要「轉移所有權」】
//   有時你確實想把資料交給一個「會自己釋放」的 C API。正確做法有兩種：
//     (a) 複製一份給對方：malloc 一塊新的、memcpy 過去，對方負責 free。
//     (b) 改用能表達所有權轉移的型別，例如 std::unique_ptr<T[]> 搭配
//         release()，明確把所有權交出去。
//   關鍵是：**所有權的轉移必須是顯式的、雙方都知情的**，
//   而不是偷偷拿走別人的緩衝區。
//
// 【概念補充 Concept Deep Dive】
//   ● 為什麼編譯器不擋
//     data() 的回傳型別就是 T*，一個普通的裸指標。裸指標在型別系統裡
//     不帶任何所有權資訊——這正是裸指標最大的問題，也是現代 C++ 用
//     unique_ptr / shared_ptr 取代它的原因：那些型別把所有權寫進了型別系統，
//     編譯器才有機會幫你檢查。
//
//   ● 怎麼抓這類錯誤
//     -fsanitize=address 能在 double free 發生的當下報錯，
//     並印出「這塊記憶體第一次在哪釋放、第二次在哪釋放」。
//     Valgrind 也能抓。⚠️ 但工具沒報錯不等於程式正確——
//     它只檢查實際執行到的路徑，而且不同工具的偵測能力並不相同
//     （例如某些洩漏 LeakSanitizer 會漏，Valgrind 才抓得到）。
//
//   ● RAII 的一體兩面
//     vector 的價值就在於「配置與釋放都由它負責」，你不必寫任何釋放程式碼。
//     代價是你必須放棄對那塊記憶體的所有權主張。
//     這不是限制，而是 RAII 的定義：資源的生命週期綁定物件的生命週期。
//
// 【注意事項 Pay Attention】
//   1. 對 v.data() 呼叫 delete[] / delete / free / realloc 全部是 UB，
//      **不保證崩潰、不保證出現任何特定錯誤訊息**。
//      glibc 常見的 "double free or corruption" 是實作的好意，不是語言保證。
//   2. 不要把 v.data() 傳給任何「會自行釋放這塊記憶體」的 API。
//      傳給只讀寫、不釋放的 API 才是安全的。
//   3. 要擴容用 reserve() / resize()，不要試圖用 realloc 介入。
//   4. 需要真正轉移所有權時，請複製一份，或改用 unique_ptr<T[]>::release()。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector 緩衝區的所有權
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::vector<int> v = {1,2,3}; delete[] v.data(); 會發生什麼事？
//     答：未定義行為，而且是兩個獨立的錯誤疊在一起：
//         ① double free —— v 解構時會再釋放一次同一塊記憶體；
//         ② 配對錯誤 —— vector 走的是 allocator（::operator new），
//            delete[] 對應的是 new[]，兩者機制不同。
//     追問：那實際跑起來會怎樣？→ **不能回答任何特定結果**。
//         glibc 常會偵測到並 abort，但那是實作的善意，不是標準保證。
//         把 UB 講成固定行為，正是面試官在這題想抓的。
//
// 🔥 Q2. 我想把 vector 的資料交給一個「會自己 free」的 C API，怎麼做才對？
//     答：不能直接給 data()。正確做法是複製一份對方能安全釋放的記憶體：
//         用 malloc 配置、memcpy 過去，再把新指標交給它；
//         或者一開始就改用 std::unique_ptr<T[]>，用 release() 顯式交出所有權。
//     追問：為什麼不能直接給？→ 因為所有權沒有轉移。
//         vector 仍認為自己擁有那塊記憶體，解構時會再釋放一次。
//         「誰配置、誰釋放」是 C 介面設計的鐵律。
//
// ⚠️ 陷阱. 「我跑過了，delete[] v.data() 沒有崩潰，所以應該沒問題」
//     答：這正是 UB 最危險的性質。沒有立刻崩潰的原因可能是：
//         配置器把那塊記憶體放回 free list 卻還沒被重用，
//         於是第二次釋放「看起來」也成功了。
//         但 free list 的結構已經被破壞，崩潰會發生在**很久以後、
//         完全無關的另一次配置**上，讓你完全找不到真兇。
//     為什麼會錯：把「沒有觀察到錯誤」當成「沒有錯誤」。
//         UB 的定義是「標準不對行為做任何規定」——它包含了
//         「這次剛好正常」這個可能性，而那恰恰是最壞的情況，
//         因為它讓錯誤逃過測試、直到上線才爆炸。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <cstdlib>    // std::malloc / std::free
#include <cstring>    // std::memcpy
#include <memory>     // std::unique_ptr
#include <numeric>    // std::iota

// -----------------------------------------------------------------------------
// 模擬一個「會自己釋放記憶體」的 C API。
// 這種介面在真實世界很常見（例如某些函式庫的 take_ownership 系列）。
// 關鍵：它拿到指標之後會呼叫 free()，所以你給它的必須是 malloc 來的、
//       而且之後你自己不能再碰。
// -----------------------------------------------------------------------------
void c_api_takes_ownership(int* data, int count) {
    long long sum = 0;
    for (int i = 0; i < count; ++i) sum += data[i];
    std::cout << "  C API 收到 " << count << " 個元素，總和 = " << sum
              << "，現在由它負責釋放\n";
    std::free(data);          // ← 這就是為什麼不能直接餵 v.data()
}

// -----------------------------------------------------------------------------
// 對照組：只讀寫、不釋放的 C API —— 這種才可以安全地餵 v.data()
// -----------------------------------------------------------------------------
void c_api_borrows_only(const int* data, int count) {
    long long sum = 0;
    for (int i = 0; i < count; ++i) sum += data[i];
    std::cout << "  C API 借用 " << count << " 個元素，總和 = " << sum
              << "，不碰記憶體的生命週期\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】把 vector 的內容安全地交給「會自行 free」的 C 函式庫
//   情境：影像 / 音訊 / 壓縮函式庫常見的介面是
//         「你給我一塊 malloc 的緩衝區，我用完自己 free」。
//         此時唯一正確的做法就是複製一份給它，vector 保留自己那份。
// -----------------------------------------------------------------------------
void hand_off_to_c_library(const std::vector<int>& v) {
    if (v.empty()) return;

    // 複製一份 C 函式庫能安全釋放的記憶體
    int* owned = static_cast<int*>(std::malloc(v.size() * sizeof(int)));
    if (owned == nullptr) {
        std::cout << "  配置失敗\n";
        return;
    }
    std::memcpy(owned, v.data(), v.size() * sizeof(int));

    // 所有權明確轉移給 C API（它會 free 掉 owned）
    c_api_takes_ownership(owned, static_cast<int>(v.size()));
    // 注意：這裡之後絕不能再碰 owned
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 66. Plus One
//   題目：用陣列表示一個非負整數（每格一位數），對它加一，回傳結果陣列。
//   為什麼用到本主題：這題正好示範「讓 vector 自己管理記憶體」的價值——
//     當進位一路傳到最高位（例如 999 + 1 = 1000），結果會比輸入**長一格**。
//     用原始陣列就得自己 malloc/realloc/free 並處理失敗路徑；
//     用 vector 只要 insert 一個 1，記憶體管理全部自動完成，
//     而且離開函式時保證不洩漏——這就是本課「不要手動介入」的正面示範。
// -----------------------------------------------------------------------------
std::vector<int> plusOne(std::vector<int> digits) {
    for (int i = static_cast<int>(digits.size()) - 1; i >= 0; --i) {
        if (digits[static_cast<std::size_t>(i)] < 9) {
            ++digits[static_cast<std::size_t>(i)];
            return digits;                       // 沒有進位，直接結束
        }
        digits[static_cast<std::size_t>(i)] = 0; // 這一位進位
    }
    // 全部都是 9 → 結果多一位，vector 自動擴容
    digits.insert(digits.begin(), 1);
    return digits;
}

int main() {
    std::cout << "=== 1. 絕對不要手動釋放 vector 的緩衝區 ===\n";
    std::vector<int> v = {10, 20, 30};

    // 絕對不要這樣做！
    // delete[] v.data();   // 災難！vector 的解構子還會再釋放一次 → 雙重釋放
    // free(v.data());      // 同樣災難！

    // 也不要嘗試用 realloc 來「幫」vector 擴容
    // vector 有自己的記憶體管理機制，不要干涉

    std::cout << "v 的內容：";
    for (int x : v) std::cout << x << " ";
    std::cout << "(size=" << v.size() << ")\n";
    std::cout << "上面三行危險敘述全部保持註解狀態——\n";
    std::cout << "它們是未定義行為，不保證崩潰、也不保證任何特定錯誤訊息。\n";

    std::cout << "\n=== 2. 借用 vs 擁有：兩種 C API 的正確用法 ===\n";
    std::cout << "(a) 只借用的 API → 可以直接傳 v.data()：\n";
    c_api_borrows_only(v.data(), static_cast<int>(v.size()));

    std::cout << "(b) 會自行釋放的 API → 必須複製一份給它：\n";
    hand_off_to_c_library(v);

    std::cout << "v 仍然完好無損：";
    for (int x : v) std::cout << x << " ";
    std::cout << "(size=" << v.size() << ")\n";

    std::cout << "\n=== 3. 要擴容請用 vector 自己的方法 ===\n";
    std::vector<int> grow = {1, 2, 3};
    std::cout << "擴容前：size=" << grow.size()
              << ", capacity=" << grow.capacity() << "\n";
    grow.reserve(100);                 // ✅ 正確：交給 vector
    // realloc(grow.data(), ...);      // ❌ 絕對不行：vector 仍持有舊指標
    std::cout << "reserve(100) 後：size=" << grow.size()
              << ", capacity=" << grow.capacity() << "\n";
    grow.resize(10, 0);
    std::cout << "resize(10, 0) 後：size=" << grow.size()
              << ", capacity=" << grow.capacity() << "\n";

    std::cout << "\n=== 4. 真的需要轉移所有權時：用 unique_ptr<T[]> ===\n";
    {
        auto owned = std::make_unique<int[]>(5);
        std::iota(owned.get(), owned.get() + 5, 100);
        std::cout << "unique_ptr<int[]> 內容：";
        for (int i = 0; i < 5; ++i) std::cout << owned[i] << " ";
        std::cout << "\n";
        std::cout << "需要交出所有權時可呼叫 owned.release()，\n";
        std::cout << "型別系統會清楚記錄「所有權已經不在我這裡」——\n";
        std::cout << "這正是裸指標做不到、而 vector 也刻意不提供的事。\n";
    }   // 沒有 release，unique_ptr 自動釋放

    std::cout << "\n=== LeetCode 66. Plus One ===\n";
    auto show = [](const char* label, const std::vector<int>& d) {
        std::cout << "  " << label;
        for (int x : d) std::cout << x;
        std::cout << "\n";
    };
    show("123 + 1 = ", plusOne({1, 2, 3}));
    show("129 + 1 = ", plusOne({1, 2, 9}));
    show("999 + 1 = ", plusOne({9, 9, 9}));      // 結果多一位，vector 自動處理
    show("9 + 1   = ", plusOne({9}));
    std::cout << "  → 999+1 的結果比輸入長一格；用原始陣列得自己 realloc 並\n";
    std::cout << "    處理失敗路徑，用 vector 只要 insert，記憶體全自動。\n";

    std::cout << "\n=== 5. 總結：借用可以，擁有不行 ===\n";
    std::cout << "可以做的：把 v.data() 傳給只讀寫的 C API\n";
    std::cout << "可以做的：用 resize() 先配置空間，再讓 C API 寫進去\n";
    std::cout << "不可以做：delete[] / free / realloc 任何來自 data() 的指標\n";
    std::cout << "不可以做：把 data() 交給會自行釋放的 API\n";
    std::cout << "要轉移所有權：複製一份，或改用 unique_ptr<T[]>::release()\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 19 課：vector 與原始陣列的互操作16.cpp" -o demo16
// 抓 UB: g++ -std=c++17 -Wall -Wextra -fsanitize=address -g "第 19 課：vector 與原始陣列的互操作16.cpp" -o demo16_asan
//
// ⚠️ 但書：
//   本檔中所有 delete[] / free / realloc 對 v.data() 的敘述都保持註解狀態。
//   取消註解會造成未定義行為——不保證崩潰、不保證出現任何特定訊息，
//   也不保證每次執行結果相同。下方預期輸出對應的是「保持註解」的正常版本。

// === 預期輸出 ===
// === 1. 絕對不要手動釋放 vector 的緩衝區 ===
// v 的內容：10 20 30 (size=3)
// 上面三行危險敘述全部保持註解狀態——
// 它們是未定義行為，不保證崩潰、也不保證任何特定錯誤訊息。
//
// === 2. 借用 vs 擁有：兩種 C API 的正確用法 ===
// (a) 只借用的 API → 可以直接傳 v.data()：
//   C API 借用 3 個元素，總和 = 60，不碰記憶體的生命週期
// (b) 會自行釋放的 API → 必須複製一份給它：
//   C API 收到 3 個元素，總和 = 60，現在由它負責釋放
// v 仍然完好無損：10 20 30 (size=3)
//
// === 3. 要擴容請用 vector 自己的方法 ===
// 擴容前：size=3, capacity=3
// reserve(100) 後：size=3, capacity=100
// resize(10, 0) 後：size=10, capacity=100
//
// === 4. 真的需要轉移所有權時：用 unique_ptr<T[]> ===
// unique_ptr<int[]> 內容：100 101 102 103 104
// 需要交出所有權時可呼叫 owned.release()，
// 型別系統會清楚記錄「所有權已經不在我這裡」——
// 這正是裸指標做不到、而 vector 也刻意不提供的事。
//
// === LeetCode 66. Plus One ===
//   123 + 1 = 124
//   129 + 1 = 130
//   999 + 1 = 1000
//   9 + 1   = 10
//   → 999+1 的結果比輸入長一格；用原始陣列得自己 realloc 並
//     處理失敗路徑，用 vector 只要 insert，記憶體全自動。
//
// === 5. 總結：借用可以，擁有不行 ===
// 可以做的：把 v.data() 傳給只讀寫的 C API
// 可以做的：用 resize() 先配置空間，再讓 C API 寫進去
// 不可以做：delete[] / free / realloc 任何來自 data() 的指標
// 不可以做：把 data() 交給會自行釋放的 API
// 要轉移所有權：複製一份，或改用 unique_ptr<T[]>::release()
