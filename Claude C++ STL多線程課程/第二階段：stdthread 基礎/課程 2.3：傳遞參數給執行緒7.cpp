/*
# 第二階段：std::thread 基礎

## 課程 2.3：傳遞參數給執行緒

---

### 引言

當執行緒函式需要參數時，我們可以在建構 `std::thread` 時一併傳入。但這裡有一些重要的細節需要注意。

---

### 一、基本參數傳遞

參數直接跟在函式後面：

```cpp
#include <iostream>
#include <thread>

void printSum(int a, int b) {
    std::cout << a << " + " << b << " = " << (a + b) << std::endl;
}

int main() {
    std::thread t(printSum, 3, 5);
    t.join();
    return 0;
}
```

輸出：
```
3 + 5 = 8
```

---

### 二、預設行為：參數被複製

`std::thread` **總是複製**傳入的參數，即使函式期望引用：

```cpp
#include <iostream>
#include <thread>

void modify(int& x) {  // 期望引用
    x = 100;
}

int main() {
    int value = 1;
    
    std::thread t(modify, value);  // value 被複製！
    t.join();
    
    std::cout << "value = " << value << std::endl;  // 仍然是 1
    return 0;
}
```

輸出：
```
value = 1
```

`value` 沒有被修改，因為執行緒收到的是副本。

---

### 三、使用 std::ref 傳遞引用

要真正傳遞引用，必須使用 `std::ref`：

```cpp
#include <iostream>
#include <thread>

void modify(int& x) {
    x = 100;
}

int main() {
    int value = 1;
    
    std::thread t(modify, std::ref(value));  // 傳遞引用
    t.join();
    
    std::cout << "value = " << value << std::endl;  // 現在是 100
    return 0;
}
```

輸出：
```
value = 100
```

---

### 四、std::ref vs 直接傳遞

```
┌────────────────────────────────────────────────────┐
│              參數傳遞方式比較                       │
├────────────────────────────────────────────────────┤
│                                                    │
│  std::thread t(func, arg);                         │
│  → arg 被複製，執行緒操作副本                       │
│                                                    │
│  std::thread t(func, std::ref(arg));               │
│  → 傳遞引用，執行緒操作原始變數                     │
│                                                    │
│  std::thread t(func, std::cref(arg));              │
│  → 傳遞 const 引用，執行緒只能讀取                  │
│                                                    │
└────────────────────────────────────────────────────┘
```

---

### 五、傳遞指標

指標本身被複製，但指向同一位址：

```cpp
#include <iostream>
#include <thread>

void modifyPtr(int* p) {
    *p = 200;
}

int main() {
    int value = 1;
    
    std::thread t(modifyPtr, &value);
    t.join();
    
    std::cout << "value = " << value << std::endl;
    return 0;
}
```

輸出：
```
value = 200
```

---

### 六、傳遞字串的陷阱

傳遞 `const char*` 給期望 `std::string` 的函式時要小心：

```cpp
#include <iostream>
#include <thread>
#include <string>

void printStr(const std::string& s) {
    std::cout << s << std::endl;
}

void danger() {
    char buffer[] = "Hello";
    
    // 危險！buffer 可能在轉換前就被銷毀
    std::thread t(printStr, buffer);
    t.detach();
    
}  // buffer 在這裡被銷毀

void safe() {
    char buffer[] = "Hello";
    
    // 安全：明確轉換為 std::string
    std::thread t(printStr, std::string(buffer));
    t.detach();
}
```

**建議**：傳遞字串時，明確轉換為 `std::string`。

---

### 七、使用 std::move 傳遞

對於只能移動的物件（如 `std::unique_ptr`）：

```cpp
#include <iostream>
#include <thread>
#include <memory>

void process(std::unique_ptr<int> ptr) {
    std::cout << "Value: " << *ptr << std::endl;
}

int main() {
    auto ptr = std::make_unique<int>(42);
    
    std::thread t(process, std::move(ptr));  // 必須 move
    t.join();
    
    // ptr 現在是空的
    return 0;
}
```

---

### 八、參數傳遞總結表

| 情況 | 寫法 | 說明 |
|------|------|------|
| 傳值 | `thread(f, arg)` | arg 被複製 |
| 傳引用 | `thread(f, std::ref(arg))` | 傳遞原始變數的引用 |
| 傳 const 引用 | `thread(f, std::cref(arg))` | 只讀引用 |
| 傳指標 | `thread(f, &arg)` | 傳遞位址 |
| 移動語意 | `thread(f, std::move(arg))` | 轉移所有權 |

---

### 九、完整範例

```cpp
// 檔案：lesson_2_3_parameters.cpp

#include <iostream>
#include <thread>
#include <string>

void byValue(int x) {
    x = 999;  // 不影響原值
}

void byRef(int& x) {
    x = 999;  // 修改原值
}

void byString(const std::string& s) {
    std::cout << "String: " << s << std::endl;
}

int main() {
    int a = 1, b = 1;
    
    std::thread t1(byValue, a);
    std::thread t2(byRef, std::ref(b));
    std::thread t3(byString, std::string("Hello"));
    
    t1.join();
    t2.join();
    t3.join();
    
    std::cout << "a = " << a << " (unchanged)" << std::endl;
    std::cout << "b = " << b << " (modified)" << std::endl;
    
    return 0;
}
```

輸出：
```
String: Hello
a = 1 (unchanged)
b = 999 (modified)
```

---

### 十、本課重點回顧

1. `std::thread` 預設**複製**所有參數
2. 要傳遞引用，必須使用 `std::ref()` 或 `std::cref()`
3. 傳遞 C 字串時，建議先轉換為 `std::string`
4. 只能移動的物件需使用 `std::move()`
5. 使用引用時要確保變數的生命週期夠長

---

### 下一課預告

在 **課程 2.4：join() 與 detach()** 中，我們將深入探討：
- join() 的阻塞行為
- detach() 的風險與適用場景
- joinable() 狀態檢查

---

準備好繼續嗎？
*/



// 檔案：lesson_2_3_parameters.cpp

#include <iostream>
#include <thread>
#include <string>

void byValue(int x) {
    x = 999;  // 不影響原值
}

void byRef(int& x) {
    x = 999;  // 修改原值
}

void byString(const std::string& s) {
    std::cout << "String: " << s << std::endl;
}

int main() {
    int a = 1, b = 1;
    
    std::thread t1(byValue, a);
    std::thread t2(byRef, std::ref(b));
    std::thread t3(byString, std::string("Hello"));
    
    t1.join();
    t2.join();
    t3.join();
    
    std::cout << "a = " << a << " (unchanged)" << std::endl;
    std::cout << "b = " << b << " (modified)" << std::endl;
    
    return 0;
}
