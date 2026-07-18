/*
好的，信安！讓我們進入建構函數的「鏡像」。

---

# 第 17 課：解構函數（Destructor）

---

## 17.1 什麼是解構函數？

在 C 語言中，你一定寫過這樣的程式碼：

```c
// C 語言：手動管理資源
FILE* fp = fopen("data.txt", "r");
// ... 使用檔案 ...
fclose(fp);   // 必須手動關閉！忘記就會資源洩漏！

int* arr = (int*)malloc(100 * sizeof(int));
// ... 使用記憶體 ...
free(arr);    // 必須手動釋放！忘記就會記憶體洩漏！
```

C 語言的問題是：資源的釋放完全依賴程式設計師的紀律。忘記 `fclose` 或 `free`，編譯器不會提醒你。

C++ 的**解構函數（Destructor）** 解決了這個問題——它是一個在對象被銷毀時**自動調用**的特殊成員函數，負責清理該對象所持有的資源。

---

## 17.2 解構函數的語法規則

| 特徵 | 說明 |
|------|------|
| 函數名 | **`~` + 類別名**（波浪號 + 類別名） |
| 返回值 | **沒有返回值**，連 `void` 都不寫 |
| 參數 | **不能有參數** |
| 數量 | 每個類別**只能有一個**解構函數（不能重載） |
| 調用時機 | 對象被銷毀時**自動調用** |

```cpp
class MyClass {
public:
    MyClass() {
        // 建構：獲取資源
    }
    
    ~MyClass() {       // 波浪號 + 類別名
        // 解構：釋放資源
    }
};
```

建構函數和解構函數是一對：

```
建構函數  →  對象誕生時自動調用  →  獲取資源、初始化
解構函數  →  對象死亡時自動調用  →  釋放資源、清理
```

---

## 17.3 第一個解構函數範例

```cpp
#include <iostream>
#include <string>
using namespace std;

class SimpleObject {
private:
    string name;

public:
    SimpleObject(const string& n) : name(n) {
        cout << "  [建構] " << name << " 被創建了" << endl;
    }
    
    ~SimpleObject() {
        cout << "  [解構] " << name << " 被銷毀了" << endl;
    }
};

int main() {
    cout << "=== main() 開始 ===" << endl;
    
    SimpleObject a("物件A");
    SimpleObject b("物件B");
    
    cout << "=== main() 結束 ===" << endl;
    return 0;
}
// a 和 b 在這裡離開作用域，解構函數被自動調用
```

### 預期輸出

```
=== main() 開始 ===
  [建構] 物件A 被創建了
  [建構] 物件B 被創建了
=== main() 結束 ===
  [解構] 物件B 被銷毀了
  [解構] 物件A 被銷毀了
```

**關鍵觀察**：解構順序是建構順序的**反序**——先建構的後解構，後建構的先解構。這就像堆疊（stack）一樣，後進先出（LIFO）。

---

## 17.4 解構函數的調用時機

```cpp
#include <iostream>
#include <string>
using namespace std;

class Tracker {
private:
    string name;

public:
    Tracker(const string& n) : name(n) {
        cout << "  [建構] " << name << endl;
    }
    
    ~Tracker() {
        cout << "  [解構] " << name << endl;
    }
};

// ====== 全域對象 ======
Tracker globalObj("全域物件");

void testFunction() {
    cout << "\n--- 進入 testFunction ---" << endl;
    Tracker funcObj("函數局部物件");
    cout << "--- 離開 testFunction ---" << endl;
}  // funcObj 在這裡被解構

int main() {
    cout << "\n=== main() 開始 ===" << endl;
    
    // ====== 局部對象 ======
    Tracker localObj("局部物件");
    
    // ====== 區塊內對象 ======
    {
        cout << "\n--- 進入區塊 ---" << endl;
        Tracker blockObj("區塊物件");
        cout << "--- 離開區塊 ---" << endl;
    }  // blockObj 在這裡被解構
    
    cout << "\n--- 區塊已結束 ---" << endl;
    
    // ====== 函數調用 ======
    testFunction();
    
    // ====== 動態對象 ======
    cout << "\n--- 動態對象 ---" << endl;
    Tracker* heapObj = new Tracker("動態物件");
    cout << "--- 手動 delete ---" << endl;
    delete heapObj;  // 解構函數在 delete 時調用
    
    cout << "\n=== main() 結束 ===" << endl;
    return 0;
}
// localObj 在這裡被解構
// globalObj 在 main() 結束後被解構
```

### 預期輸出

```
  [建構] 全域物件

=== main() 開始 ===
  [建構] 局部物件

--- 進入區塊 ---
  [建構] 區塊物件
--- 離開區塊 ---
  [解構] 區塊物件

--- 區塊已結束 ---

--- 進入 testFunction ---
  [建構] 函數局部物件
--- 離開 testFunction ---
  [解構] 函數局部物件

--- 動態對象 ---
  [建構] 動態物件
--- 手動 delete ---
  [解構] 動態物件

=== main() 結束 ===
  [解構] 局部物件
  [解構] 全域物件
```

### 各種對象的解構時機總結

| 對象類型 | 解構時機 |
|----------|----------|
| 全域對象 | 程式結束時（`main()` 返回之後） |
| 局部對象 | 離開所在作用域時 |
| 區塊內對象 | 離開區塊的 `}` 時 |
| 函數內對象 | 函數返回時 |
| 動態對象 | `delete` 時（**不 delete 就不會解構！**） |

---

## 17.5 解構函數的實際用途：資源管理

解構函數最重要的應用就是**自動管理資源**。以下是幾個典型場景：

### 場景 1：管理動態記憶體

```cpp
#include <iostream>
#include <cstring>
using namespace std;

class DynamicArray {
private:
    int* data;
    int size;

public:
    DynamicArray(int sz) : size(sz) {
        data = new int[size];  // 建構時分配記憶體
        for (int i = 0; i < size; i++) {
            data[i] = 0;
        }
        cout << "  [建構] 分配了 " << size << " 個 int 的記憶體" << endl;
    }
    
    ~DynamicArray() {
        delete[] data;         // 解構時釋放記憶體
        cout << "  [解構] 釋放了 " << size << " 個 int 的記憶體" << endl;
    }
    
    void set(int index, int value) {
        if (index >= 0 && index < size) {
            data[index] = value;
        }
    }
    
    int get(int index) const {
        if (index >= 0 && index < size) {
            return data[index];
        }
        return -1;
    }
    
    void print() const {
        cout << "  [";
        for (int i = 0; i < size; i++) {
            if (i > 0) cout << ", ";
            cout << data[i];
        }
        cout << "]" << endl;
    }
};

int main() {
    cout << "=== 動態陣列範例 ===" << endl;
    
    {
        DynamicArray arr(5);
        arr.set(0, 10);
        arr.set(1, 20);
        arr.set(2, 30);
        arr.print();
        
        // arr 離開作用域時，解構函數自動釋放記憶體
        // 不需要手動 delete！這就是 C++ 相對於 C 的優勢
        cout << "  --- 即將離開區塊 ---" << endl;
    }
    
    cout << "  --- 記憶體已自動釋放 ---" << endl;
    return 0;
}
```

### 預期輸出

```
=== 動態陣列範例 ===
  [建構] 分配了 5 個 int 的記憶體
  [10, 20, 30, 0, 0]
  --- 即將離開區塊 ---
  [解構] 釋放了 5 個 int 的記憶體
  --- 記憶體已自動釋放 ---
```

對比 C 語言，你必須這樣寫：

```c
int* arr = malloc(5 * sizeof(int));
// ... 使用 ...
free(arr);  // 忘記就洩漏！
```

C++ 的解構函數讓 `free` 變成自動的。

---

### 場景 2：管理檔案資源

```cpp
#include <iostream>
#include <fstream>
#include <string>
using namespace std;

class FileWriter {
private:
    ofstream file;
    string filename;

public:
    FileWriter(const string& fname) : filename(fname) {
        file.open(filename);
        if (file.is_open()) {
            cout << "  [建構] 打開檔案: " << filename << endl;
        } else {
            cout << "  [建構] 無法打開檔案: " << filename << endl;
        }
    }
    
    ~FileWriter() {
        if (file.is_open()) {
            file.close();    // 自動關閉檔案
            cout << "  [解構] 關閉檔案: " << filename << endl;
        }
    }
    
    void writeLine(const string& text) {
        if (file.is_open()) {
            file << text << endl;
            cout << "  寫入: " << text << endl;
        }
    }
};

int main() {
    cout << "=== 檔案寫入範例 ===" << endl;
    
    {
        FileWriter writer("test_output.txt");
        writer.writeLine("第一行");
        writer.writeLine("第二行");
        writer.writeLine("第三行");
        
        cout << "  --- 即將離開區塊 ---" << endl;
    }
    // writer 離開作用域，解構函數自動關閉檔案
    
    cout << "  --- 檔案已自動關閉 ---" << endl;
    return 0;
}
```

### 預期輸出

```
=== 檔案寫入範例 ===
  [建構] 打開檔案: test_output.txt
  寫入: 第一行
  寫入: 第二行
  寫入: 第三行
  --- 即將離開區塊 ---
  [解構] 關閉檔案: test_output.txt
  --- 檔案已自動關閉 ---
```

---

### 場景 3：計時器（自動計算生命週期）

```cpp
#include <iostream>
#include <chrono>
#include <string>
#include <thread>
using namespace std;

class ScopedTimer {
private:
    string taskName;
    chrono::high_resolution_clock::time_point startTime;

public:
    ScopedTimer(const string& name) : taskName(name) {
        startTime = chrono::high_resolution_clock::now();
        cout << "  [計時開始] " << taskName << endl;
    }
    
    ~ScopedTimer() {
        auto endTime = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(
            endTime - startTime
        ).count();
        cout << "  [計時結束] " << taskName 
             << " 耗時: " << duration << " ms" << endl;
    }
};

void simulateWork() {
    ScopedTimer timer("模擬工作");
    
    // 模擬一些耗時操作
    int sum = 0;
    for (int i = 0; i < 100000000; i++) {
        sum += i;
    }
    cout << "  計算結果: " << sum << endl;
    
    // timer 在函數返回時自動解構，印出耗時
}

int main() {
    cout << "=== 自動計時器範例 ===" << endl;
    simulateWork();
    cout << "=== 完成 ===" << endl;
    return 0;
}
```

### 預期輸出（耗時因機器而異）

```
=== 自動計時器範例 ===
  [計時開始] 模擬工作
  計算結果: 887459712
  [計時結束] 模擬工作 耗時: 198 ms
=== 完成 ===
```

這就是所謂的 **RAII（Resource Acquisition Is Initialization）** 模式——資源的獲取在建構時完成，資源的釋放在解構時完成。我們在第 93 課會深入探討這個概念，但你現在已經看到了它的威力。

---

## 17.6 忘記 delete 的後果：記憶體洩漏

```cpp
#include <iostream>
using namespace std;

class Resource {
private:
    int id;

public:
    Resource(int i) : id(i) {
        cout << "  [建構] 資源 #" << id << endl;
    }
    
    ~Resource() {
        cout << "  [解構] 資源 #" << id << endl;
    }
};

int main() {
    cout << "=== 正確使用 delete ===" << endl;
    Resource* r1 = new Resource(1);
    delete r1;    // 解構函數被調用，資源被釋放
    
    cout << "\n=== 忘記 delete ===" << endl;
    Resource* r2 = new Resource(2);
    // 沒有 delete r2！
    // 解構函數永遠不會被調用！
    // 記憶體永遠不會被釋放！
    
    cout << "\n=== 局部對象（自動管理）===" << endl;
    {
        Resource r3(3);
        // 不需要 delete，離開作用域自動解構
    }
    
    cout << "\n=== main() 結束 ===" << endl;
    return 0;
    // r2 指向的記憶體洩漏了！
}
```

### 預期輸出

```
=== 正確使用 delete ===
  [建構] 資源 #1
  [解構] 資源 #1

=== 忘記 delete ===
  [建構] 資源 #2

=== 局部對象（自動管理）===
  [建構] 資源 #3
  [解構] 資源 #3

=== main() 結束 ===
```

注意：資源 #2 的解構函數**從來沒有被調用**。這就是記憶體洩漏。

**教訓**：盡量使用**局部對象**（棧上的對象），它們會自動解構。如果必須用 `new`，記得 `delete`——或者更好的做法是使用智能指標（第 94-96 課會學到）。

---

## 17.7 編譯器自動生成的解構函數

和建構函數一樣，如果你不寫解構函數，編譯器會幫你生成一個：

```cpp
#include <iostream>
#include <string>
#include <vector>
using namespace std;

class AutoDestructor {
public:
    string name;        // string 有自己的解構函數
    vector<int> nums;   // vector 有自己的解構函數
    int value;          // 基本型別，不需要清理
    
    // 沒有定義解構函數
    // 編譯器自動生成的解構函數會：
    // 1. 調用 name 的解構函數（string::~string）
    // 2. 調用 nums 的解構函數（vector::~vector）
    // 3. value 是 int，不做任何事
};
```

**編譯器生成的解構函數做什麼？**

| 成員類型 | 行為 |
|----------|------|
| 基本型別 | 不做任何事 |
| 類別型別 | 調用該成員的解構函數 |
| 指標（裸指標） | **不做任何事！不會 delete！** |

最後一點非常重要——如果你的類別有裸指標成員指向 `new` 出來的記憶體，**你必須自己寫解構函數來 delete**，編譯器不會幫你：

```cpp
class NeedCustomDestructor {
private:
    int* data;     // 裸指標

public:
    NeedCustomDestructor(int size) {
        data = new int[size];  // 動態分配
    }
    
    // 如果不寫解構函數，data 指向的記憶體永遠不會被釋放！
    ~NeedCustomDestructor() {
        delete[] data;  // 必須手動寫！
    }
};
```

---

## 17.8 解構函數中不要做的事

### 不要在解構函數中拋出異常

```cpp
class Bad {
public:
    ~Bad() {
        // 不要這樣做！
        // throw runtime_error("error in destructor");
        
        // 如果解構函數拋出異常，而這個解構函數是在
        // 另一個異常的堆疊展開（stack unwinding）過程中被調用的，
        // 程式會直接呼叫 std::terminate() 終止！
    }
};
```

如果解構函數中可能出錯，應該**在內部處理**，不要讓異常傳播出去：

```cpp
class SafeCleanup {
public:
    ~SafeCleanup() {
        try {
            // 可能失敗的清理操作
            // closeConnection();
        } catch (...) {
            // 記錄錯誤，但不要讓異常傳播
            cerr << "清理時發生錯誤，已忽略" << endl;
        }
    }
};
```

---

## 17.9 建構與解構的完整配對觀察

```cpp
#include <iostream>
#include <string>
using namespace std;

class LifeCycle {
private:
    string name;
    static int count;  // 追蹤當前存活的物件數量

public:
    LifeCycle(const string& n) : name(n) {
        count++;
        cout << "  [建構] " << name 
             << " (目前存活: " << count << " 個)" << endl;
    }
    
    ~LifeCycle() {
        cout << "  [解構] " << name 
             << " (目前存活: " << count - 1 << " 個)" << endl;
        count--;
    }
    
    static int getCount() { return count; }
};

int LifeCycle::count = 0;  // 靜態成員初始化

int main() {
    cout << "=============================" << endl;
    cout << "  物件生命週期觀察" << endl;
    cout << "=============================" << endl;
    
    LifeCycle a("Alpha");
    
    {
        LifeCycle b("Beta");
        LifeCycle c("Charlie");
        
        cout << "\n  --- 區塊內：存活 " 
             << LifeCycle::getCount() << " 個 ---\n" << endl;
    }
    // b 和 c 在這裡被解構
    
    cout << "\n  --- 區塊外：存活 " 
         << LifeCycle::getCount() << " 個 ---\n" << endl;
    
    LifeCycle d("Delta");
    
    cout << "\n  --- main() 即將結束 ---" << endl;
    return 0;
}
// d 和 a 在這裡被解構
```

### 預期輸出

```
=============================
  物件生命週期觀察
=============================
  [建構] Alpha (目前存活: 1 個)
  [建構] Beta (目前存活: 2 個)
  [建構] Charlie (目前存活: 3 個)

  --- 區塊內：存活 3 個 ---

  [解構] Charlie (目前存活: 2 個)
  [解構] Beta (目前存活: 1 個)

  --- 區塊外：存活 1 個 ---

  [建構] Delta (目前存活: 2 個)

  --- main() 即將結束 ---
  [解構] Delta (目前存活: 1 個)
  [解構] Alpha (目前存活: 0 個)
```

整理成時間線：

```
時間 →
         Alpha            Beta    Charlie              Delta
建構  ──|═══════════════════════════════════════════════════════|── 解構
                     建構 ─|═══════════|── 解構
                            建構 ─|════|── 解構
                                                  建構 ─|═════|── 解構
```

---

## 17.10 完整綜合範例：簡易連接池

```cpp
#include <iostream>
#include <string>
using namespace std;

// ============================================================
// 模擬資料庫連接
// ============================================================
class DatabaseConnection {
private:
    string connectionString;
    int connectionId;
    bool isConnected;
    static int nextId;

public:
    DatabaseConnection(const string& connStr) 
        : connectionString(connStr),
          connectionId(nextId++),
          isConnected(false)
    {
        // 模擬建立連接
        isConnected = true;
        cout << "  [連接 #" << connectionId << "] 已建立: " 
             << connectionString << endl;
    }
    
    ~DatabaseConnection() {
        if (isConnected) {
            // 自動斷開連接
            isConnected = false;
            cout << "  [連接 #" << connectionId << "] 已斷開: " 
                 << connectionString << endl;
        }
    }
    
    void query(const string& sql) const {
        if (isConnected) {
            cout << "  [連接 #" << connectionId << "] 執行: " << sql << endl;
        } else {
            cout << "  [連接 #" << connectionId << "] 錯誤: 未連接！" << endl;
        }
    }
    
    int getId() const { return connectionId; }
};

int DatabaseConnection::nextId = 1;

// ============================================================
// 管理多個連接的工作階段
// ============================================================
class Session {
private:
    string sessionName;
    DatabaseConnection* mainDb;
    DatabaseConnection* cacheDb;

public:
    Session(const string& name, 
            const string& mainConn, 
            const string& cacheConn)
        : sessionName(name),
          mainDb(new DatabaseConnection(mainConn)),
          cacheDb(new DatabaseConnection(cacheConn))
    {
        cout << "  [Session] " << sessionName << " 啟動" << endl;
    }
    
    ~Session() {
        cout << "  [Session] " << sessionName << " 關閉中..." << endl;
        delete cacheDb;    // 先關閉快取連接
        delete mainDb;     // 再關閉主連接
        cout << "  [Session] " << sessionName << " 已完全關閉" << endl;
    }
    
    void doWork() const {
        mainDb->query("SELECT * FROM users");
        cacheDb->query("GET user:1001");
        mainDb->query("UPDATE users SET active = 1");
    }
};

int main() {
    cout << "============================================" << endl;
    cout << "   第 17 課：解構函數 綜合範例" << endl;
    cout << "============================================" << endl;
    
    cout << "\n--- 建立工作階段 ---" << endl;
    {
        Session session("UserService", 
                        "mysql://localhost:3306/mydb",
                        "redis://localhost:6379");
        
        cout << "\n--- 執行工作 ---" << endl;
        session.doWork();
        
        cout << "\n--- 工作完成，即將離開區塊 ---" << endl;
    }
    // session 離開作用域：
    // 1. Session 的解構函數被調用
    // 2. 解構函數中 delete cacheDb → DatabaseConnection 解構
    // 3. 解構函數中 delete mainDb → DatabaseConnection 解構
    
    cout << "\n--- 所有資源已自動清理 ---" << endl;
    
    return 0;
}
```

### 編譯與執行

```bash
g++ -std=c++17 -o lesson17 lesson17.cpp
./lesson17
```

### 預期輸出

```
============================================
   第 17 課：解構函數 綜合範例
============================================

--- 建立工作階段 ---
  [連接 #1] 已建立: mysql://localhost:3306/mydb
  [連接 #2] 已建立: redis://localhost:6379
  [Session] UserService 啟動

--- 執行工作 ---
  [連接 #1] 執行: SELECT * FROM users
  [連接 #2] 執行: GET user:1001
  [連接 #1] 執行: UPDATE users SET active = 1

--- 工作完成，即將離開區塊 ---
  [Session] UserService 關閉中...
  [連接 #2] 已斷開: redis://localhost:6379
  [連接 #1] 已斷開: mysql://localhost:3306/mydb
  [Session] UserService 已完全關閉

--- 所有資源已自動清理 ---
```

可以看到，我們從來沒有手動調用任何「close」或「disconnect」函數，所有清理工作都由解構函數自動完成。

---

## 17.11 建構函數 vs 解構函數 對照表

| 特徵 | 建構函數 | 解構函數 |
|------|----------|----------|
| 名稱 | `ClassName()` | `~ClassName()` |
| 返回值 | 無 | 無 |
| 參數 | 可以有（支援重載） | **不能有**（不能重載） |
| 數量 | 可以有多個 | **只能有一個** |
| 調用時機 | 對象創建時 | 對象銷毀時 |
| 職責 | 獲取資源、初始化 | 釋放資源、清理 |
| 自動生成 | 無自定義建構時自動生成 | 無自定義解構時自動生成 |
| 自動生成的行為 | 調用成員的預設建構函數 | 調用成員的解構函數 |

---

## 17.12 本課重點回顧

| 概念 | 說明 |
|------|------|
| 解構函數語法 | `~ClassName()`，無返回值，無參數，不能重載 |
| 調用時機 | 局部對象離開作用域、`delete` 動態對象、程式結束時全域對象 |
| 解構順序 | 與建構順序**相反**（後建構的先解構） |
| 核心用途 | 釋放記憶體、關閉檔案、斷開連接等資源清理 |
| RAII 模式 | 建構獲取資源、解構釋放資源——讓資源管理變自動 |
| 編譯器生成的解構 | 調用成員的解構函數，但**不會 delete 裸指標** |
| 忘記 delete | 動態對象不 delete 就不會解構，造成記憶體洩漏 |
| 異常安全 | 解構函數中不要拋出異常 |

---

## 17.13 下一課預告

下一課我們將學習 **對象的生命週期（Object Lifetime）**——完整地理解對象從創建到銷毀的全過程，包括不同存儲類型（棧、堆、全域、靜態）的對象生命週期差異，以及生命週期相關的常見陷阱。

準備好進入 **第 18 課：對象的生命週期** 了嗎？
*/



#include <iostream>
#include <string>
using namespace std;

// ============================================================
// 模擬資料庫連接
// ============================================================
class DatabaseConnection {
private:
    string connectionString;
    int connectionId;
    bool isConnected;
    static int nextId;

public:
    DatabaseConnection(const string& connStr) 
        : connectionString(connStr),
          connectionId(nextId++),
          isConnected(false)
    {
        // 模擬建立連接
        isConnected = true;
        cout << "  [連接 #" << connectionId << "] 已建立: " 
             << connectionString << endl;
    }
    
    ~DatabaseConnection() {
        if (isConnected) {
            // 自動斷開連接
            isConnected = false;
            cout << "  [連接 #" << connectionId << "] 已斷開: " 
                 << connectionString << endl;
        }
    }
    
    void query(const string& sql) const {
        if (isConnected) {
            cout << "  [連接 #" << connectionId << "] 執行: " << sql << endl;
        } else {
            cout << "  [連接 #" << connectionId << "] 錯誤: 未連接！" << endl;
        }
    }
    
    int getId() const { return connectionId; }
};

int DatabaseConnection::nextId = 1;

// ============================================================
// 管理多個連接的工作階段
// ============================================================
class Session {
private:
    string sessionName;
    DatabaseConnection* mainDb;
    DatabaseConnection* cacheDb;

public:
    Session(const string& name, 
            const string& mainConn, 
            const string& cacheConn)
        : sessionName(name),
          mainDb(new DatabaseConnection(mainConn)),
          cacheDb(new DatabaseConnection(cacheConn))
    {
        cout << "  [Session] " << sessionName << " 啟動" << endl;
    }
    
    ~Session() {
        cout << "  [Session] " << sessionName << " 關閉中..." << endl;
        delete cacheDb;    // 先關閉快取連接
        delete mainDb;     // 再關閉主連接
        cout << "  [Session] " << sessionName << " 已完全關閉" << endl;
    }
    
    void doWork() const {
        mainDb->query("SELECT * FROM users");
        cacheDb->query("GET user:1001");
        mainDb->query("UPDATE users SET active = 1");
    }
};

int main() {
    cout << "============================================" << endl;
    cout << "   第 17 課：解構函數 綜合範例" << endl;
    cout << "============================================" << endl;
    
    cout << "\n--- 建立工作階段 ---" << endl;
    {
        Session session("UserService", 
                        "mysql://localhost:3306/mydb",
                        "redis://localhost:6379");
        
        cout << "\n--- 執行工作 ---" << endl;
        session.doWork();
        
        cout << "\n--- 工作完成，即將離開區塊 ---" << endl;
    }
    // session 離開作用域：
    // 1. Session 的解構函數被調用
    // 2. 解構函數中 delete cacheDb → DatabaseConnection 解構
    // 3. 解構函數中 delete mainDb → DatabaseConnection 解構
    
    cout << "\n--- 所有資源已自動清理 ---" << endl;
    
    return 0;
}
