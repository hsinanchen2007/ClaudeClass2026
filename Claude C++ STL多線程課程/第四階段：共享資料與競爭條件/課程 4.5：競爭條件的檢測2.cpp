/*
# 第四階段：共享資料與競爭條件

## 課程 4.5：競爭條件的檢測

---

### 引言

競爭條件難以重現和除錯，幸運的是有專門的工具可以幫助我們檢測。本課介紹實用的檢測工具和技巧，特別是你已經熟悉的 ThreadSanitizer。

---

### 一、檢測工具總覽

```
┌─────────────────────────────────────────────────────────────┐
│                  競爭條件檢測工具                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  動態分析（執行時檢測）                                      │
│  • ThreadSanitizer (TSan) — 最常用                          │
│  • Helgrind (Valgrind)                                      │
│  • Intel Inspector                                          │
│                                                             │
│  靜態分析（編譯時檢測）                                      │
│  • Clang Static Analyzer                                    │
│  • Coverity                                                 │
│  • PVS-Studio                                               │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 二、ThreadSanitizer 基本使用

```cpp
// 檔案：race_demo.cpp
#include <thread>

int counter = 0;

void increment() {
    for (int i = 0; i < 1000; ++i) {
        counter++;
    }
}

int main() {
    std::thread t1(increment);
    std::thread t2(increment);
    
    t1.join();
    t2.join();
    
    return 0;
}
```

編譯與執行：

```bash
g++ -std=c++17 -fsanitize=thread -g -o race_demo race_demo.cpp -pthread
./race_demo
```

---

### 三、TSan 輸出解讀

```
==================
WARNING: ThreadSanitizer: data race (pid=12345)
  Write of size 4 at 0x000000601040 by thread T2:
    #0 increment() race_demo.cpp:7 (race_demo+0x...)
    #1 void std::__invoke_impl<...>

  Previous write of size 4 at 0x000000601040 by thread T1:
    #0 increment() race_demo.cpp:7 (race_demo+0x...)
    #1 void std::__invoke_impl<...>

  Location is global 'counter' of size 4 at 0x000000601040

SUMMARY: ThreadSanitizer: data race race_demo.cpp:7 in increment()
==================
```

關鍵資訊：
- **Write of size 4**：4 位元組的寫入操作
- **race_demo.cpp:7**：問題發生在第 7 行
- **global 'counter'**：問題變數是全域的 counter

---

### 四、TSan 常見報告類型

```
┌─────────────────────────────────────────────────────────────┐
│                  TSan 報告類型                               │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  data race                                                  │
│  → 兩個執行緒同時存取，至少一個寫入                          │
│                                                             │
│  thread leak                                                │
│  → 執行緒結束前未 join 或 detach                            │
│                                                             │
│  lock-order-inversion (potential deadlock)                  │
│  → 鎖的獲取順序不一致，可能死結                              │
│                                                             │
│  use of uninitialized mutex                                 │
│  → 使用未初始化的互斥鎖                                      │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 五、更複雜的案例

```cpp
// 檔案：complex_race.cpp
#include <iostream>
#include <thread>
#include <vector>

std::vector<int> data;

void producer() {
    for (int i = 0; i < 100; ++i) {
        data.push_back(i);
    }
}

void consumer() {
    for (int i = 0; i < 100; ++i) {
        if (!data.empty()) {
            int val = data.back();
            data.pop_back();
        }
    }
}

int main() {
    std::thread t1(producer);
    std::thread t2(consumer);
    
    t1.join();
    t2.join();
    
    return 0;
}
```

TSan 會報告 vector 內部的多處競爭。

---

### 六、TSan 的限制

```
┌─────────────────────────────────────────────────────────────┐
│                  TSan 的限制與注意事項                       │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  效能影響                                                    │
│  • 執行速度慢 5-15 倍                                        │
│  • 記憶體使用增加 5-10 倍                                    │
│                                                             │
│  檢測限制                                                    │
│  • 只能檢測實際執行到的程式碼                                │
│  • 無法檢測所有可能的交錯                                    │
│  • 需要足夠的測試覆蓋率                                      │
│                                                             │
│  使用建議                                                    │
│  • 開發和測試時使用，不用於生產環境                          │
│  • 結合壓力測試增加發現機率                                  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 七、Helgrind（Valgrind 工具）

另一個選擇，不需要重新編譯：

```bash
# 正常編譯
g++ -std=c++17 -g -o race_demo race_demo.cpp -pthread

# 使用 Helgrind 執行
valgrind --tool=helgrind ./race_demo
```

輸出：
```
==12345== Possible data race during write of size 4
==12345==    at 0x401234: increment() (race_demo.cpp:7)
==12345==  This conflicts with a previous write
==12345==    at 0x401234: increment() (race_demo.cpp:7)
```

---

### 八、TSan vs Helgrind 比較

| 特性 | TSan | Helgrind |
|------|------|----------|
| 速度 | 較快（5-15x） | 較慢（20-100x） |
| 需要重新編譯 | 是 | 否 |
| 誤報率 | 低 | 較高 |
| 支援 C++11 原子 | 完整 | 部分 |

---

### 九、手動檢測技巧

#### 技巧一：插入延遲

```cpp
void suspiciousFunction() {
    // 在可疑位置插入延遲，增加競爭發生機率
    checkCondition();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    doAction();
}
```

#### 技巧二：壓力測試

```cpp
#include <iostream>
#include <thread>
#include <vector>

void stressTest() {
    for (int trial = 0; trial < 1000; ++trial) {
        // 重設狀態
        counter = 0;
        
        std::vector<std::thread> threads;
        for (int i = 0; i < 10; ++i) {
            threads.emplace_back(increment);
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        if (counter != 10000) {
            std::cout << "競爭檢測！Trial " << trial 
                      << " counter=" << counter << std::endl;
        }
    }
}
```

---

### 十、檢測清單

```
┌─────────────────────────────────────────────────────────────┐
│               競爭條件檢測清單                               │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  □ 使用 TSan 編譯並執行測試                                 │
│  □ 確保測試覆蓋多執行緒路徑                                 │
│  □ 進行壓力測試（多次重複執行）                             │
│  □ 審查所有共享變數的存取                                   │
│  □ 檢查 Check-Then-Act 模式                                │
│  □ 檢查 Read-Modify-Write 操作                             │
│  □ 確認所有複合操作都有適當保護                             │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 十一、本課重點回顧

1. **ThreadSanitizer** 是最實用的競爭條件檢測工具
2. 使用 `-fsanitize=thread -g` 編譯
3. TSan 會報告資料競爭的位置和相關執行緒
4. TSan 會降低效能，僅用於開發和測試
5. **Helgrind** 不需重新編譯，但較慢
6. 結合**壓力測試**增加發現問題的機率

---

### 第四階段完成！

恭喜你完成了共享資料與競爭條件階段！你已經學會：

- ✅ 共享資料的問題本質
- ✅ 不變量與競爭條件的關係
- ✅ 臨界區段的識別與設計
- ✅ 常見競爭條件模式
- ✅ 使用工具檢測競爭條件

---

### 下一階段預告

**第五階段：互斥鎖基礎 (std::mutex)** 將學習如何解決這些問題：
- 課程 5.1：std::mutex 基本操作
- 課程 5.2：互斥鎖的工作原理
- 課程 5.3：try_lock() 非阻塞鎖定
- ...

---

準備好進入第五階段嗎？
*/



// 檔案：complex_race.cpp
#include <iostream>
#include <thread>
#include <vector>

std::vector<int> data;

void producer() {
    for (int i = 0; i < 100; ++i) {
        data.push_back(i);
    }
}

void consumer() {
    for (int i = 0; i < 100; ++i) {
        if (!data.empty()) {
            int val = data.back();
            data.pop_back();
        }
    }
}

int main() {
    std::thread t1(producer);
    std::thread t2(consumer);
    
    t1.join();
    t2.join();
    
    return 0;
}
