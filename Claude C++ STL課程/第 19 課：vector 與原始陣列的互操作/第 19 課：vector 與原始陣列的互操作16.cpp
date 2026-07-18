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
