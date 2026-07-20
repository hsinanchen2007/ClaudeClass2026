// =============================================================================
//  課程 5.4：互斥鎖的常見錯誤14.cpp  —  互斥鎖最佳實踐總整理
// =============================================================================
//
// 【主題資訊 Information】
//   本檔是課程 5.4 的收尾：把前面十餘個「錯誤示範」歸納成一組可直接套用的
//   實踐守則，並用一個執行緒安全的類別完整示範。
//   涉及的標準設施（全部 <mutex>）：
//       std::mutex                      // C++11，基本互斥鎖
//       std::lock_guard                 // C++11，作用域 RAII 鎖（預設選擇）
//       std::unique_lock                // C++11，需要彈性時
//       std::scoped_lock                // C++17，同時鎖多把
//       std::recursive_mutex            // C++11，可重入（通常是重構訊號）
//       mutable                         // 讓 const 方法也能上鎖
//
// 【詳細解釋 Explanation】
//
// 【1. 六條守則，以及它們各自對應本課的哪個錯誤】
//   ┌────┬──────────────────────────┬──────────────────────────────────┐
//   │ #  │ 守則                     │ 對應的錯誤示範                   │
//   ├────┼──────────────────────────┼──────────────────────────────────┤
//   │ 1  │ 一律用 RAII，不手寫      │ 檔 1 忘記 unlock（永久阻塞）     │
//   │    │ lock/unlock              │ 檔 2 提前 return（→ UB）         │
//   │ 2  │ 臨界區段最小化           │ 檔 13 範圍太大（並行度歸零）     │
//   │ 3  │ 但不可小到拆散不變量     │ 檔 12 範圍太小（TOCTOU）         │
//   │ 4  │ const 方法也要保護       │ 需搭配 mutable mutex             │
//   │ 5  │ 例外安全靠設計，不只靠   │ 檔 10（RAII 只給基本保證）       │
//   │    │ RAII                     │                                  │
//   │ 6  │ 多把鎖用 scoped_lock     │ 檔 11 AB-BA 死結                 │
//   └────┴──────────────────────────┴──────────────────────────────────┘
//   ⚠️ 特別注意守則 2 與 3 【互相拉扯】：
//      臨界區段要盡量小（效能），但不能小到把「檢查」與「動作」拆開（正確性）。
//      唯一的判準是：**臨界區段應恰好涵蓋「不變量從被破壞到被修復」的期間**，
//      不多也不少。正確性優先於效能。
//
// 【2. 為什麼「封裝」是最重要的一條】
//   前面所有錯誤都有一個共同前提：使用者必須自己記得上鎖。
//   最根本的解法是【不要讓使用者碰到鎖】——
//   把 mutex 設為 private，只暴露已經保證原子的操作。
//       class SafeCounter {
//           mutable std::mutex mtx;      // private：外界看不到
//           int count = 0;
//       public:
//           void increment();            // 每個操作自己保證原子
//           int  getAndReset();
//       };
//   ⚠️ 但要記得課 5.1 的警告：
//   「每個方法都加鎖」【不等於】「使用這個類別的程式碼就安全」。
//   若使用者需要「檢查後再動作」，就必須由類別提供【組合好的】原子操作
//   （如本檔的 getAndReset、addIfBelow），而不是讓使用者在外面拼。
//
// 【3. mutable mutex：const 正確性與執行緒安全的交會點】
//   一個 const 方法在語意上「不改變物件的邏輯狀態」，但它仍需要上鎖
//   （因為別的執行緒可能正在寫）。而 lock() 是非 const 的成員函式，
//   所以 mutex 必須宣告成 mutable：
//       mutable std::mutex mtx;
//   這正是 mutable 最正當的用途：
//   「這個成員的改變不影響物件對外的邏輯狀態」。
//   ⚠️ 若忘了加 mutable，編譯器會在 const 方法裡報錯——
//   這是少數「編譯器會幫你抓到」的執行緒安全問題。
//
// 【4. 不可複製：mutex 本身就禁止了】
//   含有 std::mutex 成員的類別，其複製建構與複製指派會被
//   【隱式刪除】（因為 mutex 不可複製）。這通常正是你要的——
//   複製一個帶鎖的物件在語意上很可疑：新物件該不該持有鎖？
//   若真的需要複製，必須自己寫複製建構子，
//   在其中鎖住來源物件再複製資料（而不是複製鎖本身）：
//       SafeCounter(const SafeCounter& other) {
//           std::lock_guard<std::mutex> lock(other.mtx);
//           count = other.count;
//       }
//   ⚠️ 注意：複製【兩個】這種物件時（operator= 的自我賦值與雙鎖問題），
//   要用 std::scoped_lock 同時鎖住兩者，否則兩個物件互相賦值會死結。
//
// 【概念補充 Concept Deep Dive】
//   * 本檔的 SafeCounter 展示了「執行緒安全類別」的標準骨架，
//     但它有一個刻意保留的限制值得討論：所有操作共用【一把】鎖。
//     這在操作都很短時完全正確；若某些操作很慢，
//     就會拖累所有其他操作（見檔 13 的討論）。
//     進階做法是依資料切分成多把鎖，但那會引入死結風險——
//     所以原則永遠是【先粗後細，量測後再優化】。
//   * 「執行緒安全」不是一個二元屬性，而是一份【契約】。
//     說一個類別 thread-safe 時，必須講清楚保證到什麼程度：
//       - 每個方法各自原子？（本檔的 SafeCounter）
//       - 多個方法的組合也原子？（需要額外的複合介面）
//       - const 方法可以並行？（需要 shared_mutex）
//     沒有講清楚的「thread-safe」是誤導。
//   * 效能的第一原則其實是【先問需不需要鎖】：
//     單純的計數器用 std::atomic 就好，完全不需要 mutex（見課 5.2）。
//
// 【注意事項 Pay Attention】
//   1. mutex 一律 private；不要讓使用者自己管鎖。
//   2. const 方法要上鎖 → mutex 必須是 mutable。
//   3. 「每個方法原子」不等於「組合起來原子」，複合操作要另外提供介面。
//   4. 臨界區段大小的唯一判準是「不變量的生命週期」，不是行數。
//   5. RAII 保證鎖被釋放，【不保證】資料一致——例外安全要另外設計。
//   6. 含 mutex 的類別預設不可複製；真要複製必須自己處理鎖。
//   7. 能用 std::atomic 解決的，就不要用鎖。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】執行緒安全類別的設計
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 設計一個 thread-safe 的類別時，mutex 該放 public 還是 private？
//        為什麼？
//     答：一律 private。把鎖暴露出去等於把「記得上鎖」的責任丟給使用者，
//         而本課所有錯誤示範的共同前提就是「人類會忘記」。
//         正確做法是把 mutex 設為 private，只暴露【已經保證原子】的操作；
//         使用者根本碰不到鎖，也就不可能忘記或用錯順序。
//     追問：那使用者需要「檢查後再動作」怎麼辦？
//           → 由類別提供組合好的原子介面（如 getAndReset()、
//             tryPop(T&)、addIfBelow(n, limit)），
//             而不是要求使用者在外面自己拼 —— 那會變成 TOCTOU 錯誤。
//
// 🔥 Q2. 為什麼 thread-safe 類別的 mutex 成員常常宣告成 mutable？
//     答：因為 const 方法（如 get()）在語意上不改變物件的邏輯狀態，
//         但仍然需要上鎖——別的執行緒可能正在寫。
//         而 std::mutex::lock() 是非 const 的成員函式，
//         在 const 方法裡無法對非 mutable 的成員呼叫。
//         mutable 正是為這種情況設計的：
//         「這個成員的改變不影響物件對外的邏輯狀態」。
//     追問：忘了加 mutable 會怎樣？→ 編譯錯誤。
//           這是少數編譯器會幫你抓到的執行緒安全問題，
//           大多數（data race、死結、TOCTOU）它都無能為力。
//
// ⚠️ 陷阱. 「我的類別每個 public 方法都用 lock_guard 保護了，
//        所以它是 thread-safe 的，使用者可以放心用。」
//     答：只說對了一半。每個方法【各自】是原子的沒錯，
//         但使用者把兩個方法【組合】起來用時，中間有空隙：
//             if (counter.get() < 100) counter.increment();   // 💀 TOCTOU
//         兩條執行緒可能同時看到 99，然後都遞增，變成 101。
//         「每個方法原子」與「使用它的程式碼正確」是兩件事。
//     為什麼會錯：把 thread-safe 當成一個二元屬性
//         （安全 / 不安全），而它其實是一份【需要說清楚範圍的契約】。
//         正確的做法是：辨識出使用者真正需要的複合操作，
//         直接提供對應的原子介面（例如 incrementIfBelow(limit)），
//         而不是宣稱「我每個方法都加鎖了」就把責任交出去。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔【略過】，理由如下：
//   本檔是整課的【守則總整理】，不是單一技術點的示範。
//   本課已在對應的檔案中用真實題目完整示範過設計題：
//       常見錯誤3.cpp  → LeetCode 155. Min Stack（RAII 鎖）
//       常見錯誤7.cpp  → LeetCode 707. Design Linked List（重構避免重入）
//       課 5.1 基本操作3.cpp → LeetCode 146. LRU Cache（複合操作原子性）
//       課 5.3 非阻塞鎖定5.cpp → LeetCode 705. Design HashSet（非阻塞查詢）
//   在總整理檔再掛一題只會重複，故從缺。

/*
# 第五階段：互斥鎖基礎 (std::mutex)

## 課程 5.4：互斥鎖的常見錯誤

---

### 引言

互斥鎖看似簡單，只有 `lock()` 和 `unlock()` 兩個主要操作，但實際開發中卻充滿陷阱。本課將系統性地分析最常見的錯誤，並學習如何避免它們。

---

### 一、錯誤總覽

```
┌─────────────────────────────────────────────────────────────┐
│                  互斥鎖常見錯誤                              │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. 忘記解鎖（Forgetting to Unlock）                        │
│     → 其他執行緒永遠等待                                    │
│                                                             │
│  2. 重複鎖定（Double Locking）                              │
│     → 同一執行緒對同一 mutex 呼叫兩次 lock()                │
│                                                             │
│  3. 解鎖未鎖定的互斥鎖（Unlocking Unowned Mutex）           │
│     → 未定義行為                                            │
│                                                             │
│  4. 例外導致未解鎖（Exception Without Unlock）              │
│     → 例外拋出後 unlock() 未執行                            │
│                                                             │
│  5. 鎖定順序不一致（Inconsistent Lock Ordering）            │
│     → 導致死結                                              │
│                                                             │
│  6. 保護範圍錯誤（Incorrect Protection Scope）              │
│     → 臨界區段設計不當                                      │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 二、錯誤 1：忘記解鎖

這是最常見也最危險的錯誤。

#### 錯誤範例

```cpp
// 檔案：lesson_5_4_forget_unlock.cpp
// 說明：忘記解鎖的錯誤示範

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

std::mutex mtx;
int counter = 0;

void buggyIncrement() {
    mtx.lock();
    ++counter;
    std::cout << "Counter = " << counter << std::endl;
    
    // 💀 忘記 unlock()！
    // mtx.unlock();
}

int main() {
    std::thread t1(buggyIncrement);
    t1.join();
    
    std::cout << "執行緒 1 完成" << std::endl;
    
    std::thread t2(buggyIncrement);  // 💀 永遠卡住！
    
    // 設定超時檢測
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "程式卡住了！（這行可能不會執行）" << std::endl;
    
    t2.join();  // 永遠不會返回
    
    return 0;
}
```

#### 更隱蔽的情況：早期返回

```cpp
// 檔案：lesson_5_4_early_return.cpp
// 說明：早期返回導致忘記解鎖

#include <iostream>
#include <mutex>

std::mutex mtx;

int getValue(int input) {
    mtx.lock();
    
    if (input < 0) {
        std::cout << "無效輸入" << std::endl;
        return -1;  // 💀 提前返回，沒有 unlock！
    }
    
    if (input == 0) {
        std::cout << "零值" << std::endl;
        return 0;   // 💀 又一個提前返回！
    }
    
    int result = input * 2;
    mtx.unlock();
    return result;
}

int main() {
    std::cout << getValue(10) << std::endl;   // OK
    std::cout << getValue(-5) << std::endl;   // 💀 鎖沒釋放
    std::cout << getValue(20) << std::endl;   // 💀 永遠卡住
    
    return 0;
}
```

#### 正確做法：使用 RAII

```cpp
// 檔案：lesson_5_4_raii_fix.cpp
// 說明：使用 lock_guard 自動管理鎖

#include <iostream>
#include <mutex>

std::mutex mtx;

int getValueSafe(int input) {
    std::lock_guard<std::mutex> lock(mtx);  // ✓ RAII
    
    if (input < 0) {
        std::cout << "無效輸入" << std::endl;
        return -1;  // ✓ lock_guard 解構時自動 unlock
    }
    
    if (input == 0) {
        std::cout << "零值" << std::endl;
        return 0;   // ✓ 同樣會自動 unlock
    }
    
    return input * 2;
}  // ✓ 函式結束，lock_guard 解構，自動 unlock

int main() {
    std::cout << getValueSafe(10) << std::endl;
    std::cout << getValueSafe(-5) << std::endl;
    std::cout << getValueSafe(20) << std::endl;  // ✓ 正常執行
    
    return 0;
}
```

---

### 三、錯誤 2：重複鎖定

同一執行緒對同一個 `std::mutex` 呼叫兩次 `lock()`。

#### 錯誤範例

```cpp
// 檔案：lesson_5_4_double_lock.cpp
// 說明：重複鎖定的錯誤

#include <iostream>
#include <mutex>

std::mutex mtx;

void outerFunction();
void innerFunction();

void innerFunction() {
    mtx.lock();  // 💀 已經被 outerFunction 鎖定了！
    std::cout << "Inner function" << std::endl;
    mtx.unlock();
}

void outerFunction() {
    mtx.lock();
    std::cout << "Outer function" << std::endl;
    
    innerFunction();  // 💀 呼叫另一個也需要鎖的函式
    
    mtx.unlock();
}

int main() {
    outerFunction();  // 💀 死結！（或未定義行為）
    return 0;
}
```

```
┌─────────────────────────────────────────────────────────────┐
│                   重複鎖定的後果                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  std::mutex：未定義行為（通常是死結）                        │
│                                                             │
│  outerFunction()                                            │
│       │                                                     │
│       ▼                                                     │
│  mtx.lock()  ← 成功                                         │
│       │                                                     │
│       ▼                                                     │
│  innerFunction()                                            │
│       │                                                     │
│       ▼                                                     │
│  mtx.lock()  ← 💀 同一執行緒再次嘗試鎖定                     │
│       │          已經持有的鎖                               │
│       ▼                                                     │
│  永遠等待自己釋放鎖（死結）                                  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

#### 解決方案 1：使用 std::recursive_mutex

```cpp
// 檔案：lesson_5_4_recursive_mutex.cpp
// 說明：使用遞迴互斥鎖解決重複鎖定

#include <iostream>
#include <mutex>

std::recursive_mutex rmtx;  // ✓ 遞迴互斥鎖

void innerFunction() {
    rmtx.lock();  // ✓ 同一執行緒可以再次鎖定
    std::cout << "Inner function" << std::endl;
    rmtx.unlock();
}

void outerFunction() {
    rmtx.lock();
    std::cout << "Outer function" << std::endl;
    
    innerFunction();  // ✓ 正常運作
    
    rmtx.unlock();
}

int main() {
    outerFunction();  // ✓ 正常執行
    std::cout << "完成" << std::endl;
    return 0;
}
```

#### 解決方案 2：重構程式碼（更好的做法）

```cpp
// 檔案：lesson_5_4_refactor.cpp
// 說明：透過重構避免重複鎖定

#include <iostream>
#include <mutex>

std::mutex mtx;

// 內部實作（假設鎖已被持有，不加鎖）
void innerFunctionImpl() {
    std::cout << "Inner function" << std::endl;
}

// 公開介面（需要鎖）
void innerFunction() {
    std::lock_guard<std::mutex> lock(mtx);
    innerFunctionImpl();
}

void outerFunction() {
    std::lock_guard<std::mutex> lock(mtx);
    std::cout << "Outer function" << std::endl;
    
    innerFunctionImpl();  // ✓ 呼叫不加鎖的版本
}

int main() {
    outerFunction();
    std::cout << "完成" << std::endl;
    return 0;
}
```

---

### 四、錯誤 3：解鎖未鎖定的互斥鎖

```cpp
// 檔案：lesson_5_4_unlock_unowned.cpp
// 說明：解鎖未鎖定或不屬於自己的互斥鎖

#include <iostream>
#include <thread>
#include <mutex>

std::mutex mtx;

void wrongUnlock() {
    // 沒有 lock()
    mtx.unlock();  // 💀 未定義行為！
}

void wrongThread() {
    mtx.lock();
    
    std::thread t([&]() {
        mtx.unlock();  // 💀 在不同執行緒解鎖！未定義行為！
    });
    t.join();
}

int main() {
    // 這些都是錯誤的用法
    // wrongUnlock();
    // wrongThread();
    
    std::cout << "這些函式都有問題，不要這樣做！" << std::endl;
    
    return 0;
}
```

```
┌─────────────────────────────────────────────────────────────┐
│              互斥鎖的所有權規則                              │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  規則 1：誰鎖定，誰解鎖                                      │
│  ────────────────────────                                   │
│  執行緒 A 呼叫 lock()，只有執行緒 A 能呼叫 unlock()          │
│                                                             │
│  規則 2：先鎖定，後解鎖                                      │
│  ────────────────────────                                   │
│  必須先成功呼叫 lock() 或 try_lock()，才能呼叫 unlock()      │
│                                                             │
│  規則 3：配對原則                                            │
│  ────────────────                                           │
│  每個成功的 lock()/try_lock() 必須有且只有一個 unlock()      │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 五、錯誤 4：例外導致未解鎖

這是 C++ 特有的問題，也是 RAII 如此重要的原因。

#### 錯誤範例

```cpp
// 檔案：lesson_5_4_exception_unsafe.cpp
// 說明：例外導致 unlock 未執行

#include <iostream>
#include <mutex>
#include <stdexcept>

std::mutex mtx;

void riskyOperation(int value) {
    mtx.lock();
    
    std::cout << "開始處理 " << value << std::endl;
    
    if (value < 0) {
        throw std::invalid_argument("值不能為負數");  // 💀 例外拋出
        // unlock() 永遠不會執行！
    }
    
    std::cout << "處理完成" << std::endl;
    mtx.unlock();
}

int main() {
    try {
        riskyOperation(10);   // OK
        riskyOperation(-5);   // 💀 拋出例外，鎖沒釋放
    } catch (const std::exception& e) {
        std::cout << "捕獲例外：" << e.what() << std::endl;
    }
    
    // 此時 mtx 仍處於鎖定狀態！
    std::cout << "嘗試再次操作..." << std::endl;
    riskyOperation(20);  // 💀 死結！
    
    return 0;
}
```

#### 正確做法：使用 RAII

```cpp
// 檔案：lesson_5_4_exception_safe.cpp
// 說明：使用 RAII 確保例外安全

#include <iostream>
#include <mutex>
#include <stdexcept>

std::mutex mtx;

void safeOperation(int value) {
    std::lock_guard<std::mutex> lock(mtx);  // ✓ RAII
    
    std::cout << "開始處理 " << value << std::endl;
    
    if (value < 0) {
        throw std::invalid_argument("值不能為負數");
        // ✓ 例外拋出時，lock_guard 解構，自動 unlock
    }
    
    std::cout << "處理完成" << std::endl;
}  // ✓ 正常返回時也會自動 unlock

int main() {
    try {
        safeOperation(10);
        safeOperation(-5);  // 拋出例外
    } catch (const std::exception& e) {
        std::cout << "捕獲例外：" << e.what() << std::endl;
    }
    
    // ✓ 鎖已被正確釋放
    std::cout << "嘗試再次操作..." << std::endl;
    safeOperation(20);  // ✓ 正常執行
    
    return 0;
}
```

#### 輸出

```
開始處理 10
處理完成
開始處理 -5
捕獲例外：值不能為負數
嘗試再次操作...
開始處理 20
處理完成
```

---

### 六、錯誤 5：鎖定順序不一致

這會導致死結，我們在第八階段會詳細討論。

```cpp
// 檔案：lesson_5_4_lock_order.cpp
// 說明：鎖定順序不一致導致死結

#include <iostream>
#include <thread>
#include <mutex>

std::mutex mutexA;
std::mutex mutexB;

void thread1() {
    mutexA.lock();
    std::cout << "Thread 1: locked A" << std::endl;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    mutexB.lock();  // 💀 等待 Thread 2 釋放 B
    std::cout << "Thread 1: locked B" << std::endl;
    
    mutexB.unlock();
    mutexA.unlock();
}

void thread2() {
    mutexB.lock();  // ← 順序相反！
    std::cout << "Thread 2: locked B" << std::endl;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    mutexA.lock();  // 💀 等待 Thread 1 釋放 A
    std::cout << "Thread 2: locked A" << std::endl;
    
    mutexA.unlock();
    mutexB.unlock();
}

int main() {
    std::thread t1(thread1);
    std::thread t2(thread2);
    
    t1.join();
    t2.join();
    
    // 💀 程式永遠不會到達這裡
    std::cout << "完成" << std::endl;
    
    return 0;
}
```

```
┌─────────────────────────────────────────────────────────────┐
│                    死結形成過程                              │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  時間   Thread 1              Thread 2                      │
│  ────   ────────              ────────                      │
│   T1    lock(A) ✓                                           │
│   T2                          lock(B) ✓                     │
│   T3    lock(B) 等待...                                     │
│   T4                          lock(A) 等待...               │
│                                                             │
│         Thread 1 等待 B       Thread 2 等待 A               │
│         B 被 Thread 2 持有    A 被 Thread 1 持有            │
│                                                             │
│                    💀 死結！                                 │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

#### 解決方案：統一鎖定順序

```cpp
// 總是先鎖 A，再鎖 B
void thread1() {
    std::lock_guard<std::mutex> lockA(mutexA);
    std::lock_guard<std::mutex> lockB(mutexB);
    // 工作...
}

void thread2() {
    std::lock_guard<std::mutex> lockA(mutexA);  // ✓ 同樣的順序
    std::lock_guard<std::mutex> lockB(mutexB);
    // 工作...
}
```

---

### 七、錯誤 6：保護範圍錯誤

#### 錯誤 6a：保護範圍太小

```cpp
// 檔案：lesson_5_4_scope_too_small.cpp
// 說明：保護範圍太小，仍有競爭條件

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>

std::mutex mtx;
std::vector<int> data;

void unsafeAppend(int value) {
    // 錯誤：檢查和操作分開保護
    
    mtx.lock();
    bool shouldAppend = (data.size() < 10);
    mtx.unlock();
    
    // 💀 此時其他執行緒可能已經改變了 data.size()！
    
    if (shouldAppend) {
        mtx.lock();
        data.push_back(value);  // 💀 可能超過限制！
        mtx.unlock();
    }
}

void safeAppend(int value) {
    // 正確：整個操作在同一個臨界區段
    std::lock_guard<std::mutex> lock(mtx);
    
    if (data.size() < 10) {
        data.push_back(value);  // ✓ 安全
    }
}
```

#### 錯誤 6b：保護範圍太大

```cpp
// 檔案：lesson_5_4_scope_too_large.cpp
// 說明：保護範圍太大，降低效能

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

std::mutex mtx;
int result = 0;

void inefficientWork(int input) {
    std::lock_guard<std::mutex> lock(mtx);  // 💀 整個函式都被鎖住
    
    // 這部分不需要鎖
    int temp = input * input;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // 模擬耗時計算
    temp += input;
    
    // 只有這裡需要鎖
    result += temp;
}

void efficientWork(int input) {
    // 不需要鎖的部分
    int temp = input * input;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    temp += input;
    
    // 只鎖必要的部分
    {
        std::lock_guard<std::mutex> lock(mtx);
        result += temp;  // ✓ 最小化臨界區段
    }
}

int main() {
    auto start = std::chrono::steady_clock::now();
    
    std::thread t1(inefficientWork, 10);
    std::thread t2(inefficientWork, 20);
    t1.join();
    t2.join();
    
    auto mid = std::chrono::steady_clock::now();
    
    result = 0;
    
    std::thread t3(efficientWork, 10);
    std::thread t4(efficientWork, 20);
    t3.join();
    t4.join();
    
    auto end = std::chrono::steady_clock::now();
    
    auto inefficient_time = std::chrono::duration_cast<std::chrono::milliseconds>(mid - start);
    auto efficient_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - mid);
    
    std::cout << "低效版本：" << inefficient_time.count() << " ms" << std::endl;
    std::cout << "高效版本：" << efficient_time.count() << " ms" << std::endl;
    
    return 0;
}
```

#### 輸出

```
低效版本：200 ms   （串行執行）
高效版本：100 ms   （並行執行）
```

---

### 八、錯誤檢查清單

```
┌─────────────────────────────────────────────────────────────┐
│               互斥鎖使用檢查清單                             │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  □ 每個 lock() 都有對應的 unlock()                          │
│                                                             │
│  □ 所有提前返回的路徑都會釋放鎖                              │
│    → 使用 RAII（lock_guard/unique_lock）                    │
│                                                             │
│  □ 例外拋出時鎖會被正確釋放                                  │
│    → 使用 RAII                                              │
│                                                             │
│  □ 沒有在同一執行緒重複鎖定 std::mutex                      │
│    → 如需要，使用 recursive_mutex 或重構                    │
│                                                             │
│  □ 只在持有鎖的執行緒呼叫 unlock()                          │
│                                                             │
│  □ 多個互斥鎖的鎖定順序一致                                  │
│    → 或使用 std::lock() / std::scoped_lock                  │
│                                                             │
│  □ 臨界區段盡可能小                                          │
│    → 只包含必要的共享資料存取                                │
│                                                             │
│  □ 臨界區段內沒有阻塞操作                                    │
│    → 避免 I/O、sleep、等待其他資源                          │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 九、最佳實踐總結

```cpp
// 檔案：lesson_5_4_best_practices.cpp
// 說明：互斥鎖最佳實踐範例

#include <iostream>
#include <mutex>
#include <vector>
#include <stdexcept>

class SafeCounter {
private:
    mutable std::mutex mtx;  // mutable 允許在 const 方法中鎖定
    int count = 0;
    
public:
    // ✓ 最佳實踐 1：使用 RAII
    void increment() {
        std::lock_guard<std::mutex> lock(mtx);
        ++count;
    }
    
    // ✓ 最佳實踐 2：最小化臨界區段
    int getAndReset() {
        int result;
        {
            std::lock_guard<std::mutex> lock(mtx);
            result = count;
            count = 0;
        }  // 鎖在這裡釋放
        
        // 後續處理不需要鎖
        return result;
    }
    
    // ✓ 最佳實踐 3：const 方法也要保護
    int get() const {
        std::lock_guard<std::mutex> lock(mtx);
        return count;
    }
    
    // ✓ 最佳實踐 4：例外安全
    void riskyOperation(int value) {
        std::lock_guard<std::mutex> lock(mtx);
        
        if (value < 0) {
            throw std::invalid_argument("負值");
            // 例外拋出時 lock_guard 自動解鎖
        }
        
        count += value;
    }
};

int main() {
    SafeCounter counter;
    
    counter.increment();
    counter.increment();
    std::cout << "Count: " << counter.get() << std::endl;
    
    try {
        counter.riskyOperation(-1);
    } catch (...) {
        std::cout << "例外被捕獲，鎖已正確釋放" << std::endl;
    }
    
    counter.increment();  // 正常運作
    std::cout << "Final count: " << counter.get() << std::endl;
    
    return 0;
}
```

---

### 十、本課重點回顧

| 錯誤類型 | 後果 | 解決方案 |
|----------|------|----------|
| 忘記解鎖 | 其他執行緒永遠等待 | 使用 RAII |
| 重複鎖定 | 死結/未定義行為 | recursive_mutex 或重構 |
| 解鎖未持有的鎖 | 未定義行為 | 遵守所有權規則 |
| 例外導致未解鎖 | 鎖洩漏 | 使用 RAII |
| 鎖定順序不一致 | 死結 | 統一順序或 std::lock |
| 保護範圍錯誤 | 競爭條件或效能差 | 仔細設計臨界區段 |

**核心建議：永遠使用 RAII（lock_guard 或 unique_lock）來管理鎖！**

---

### 下一課預告

在 **課程 5.5：保護共享資料實作** 中，我們將：
- 實作一個完整的執行緒安全計數器類別
- 實作一個執行緒安全的銀行帳戶類別
- 學習如何設計執行緒安全的介面

---

準備好繼續嗎？
*/



// 檔案：lesson_5_4_best_practices.cpp
// 說明：互斥鎖最佳實踐範例

#include <atomic>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

class SafeCounter {
private:
    mutable std::mutex mtx;  // mutable 允許在 const 方法中鎖定
    int count = 0;

public:
    // ✓ 最佳實踐 1：使用 RAII
    void increment() {
        std::lock_guard<std::mutex> lock(mtx);
        ++count;
    }

    // ✓ 最佳實踐 2：最小化臨界區段
    int getAndReset() {
        int result;
        {
            std::lock_guard<std::mutex> lock(mtx);
            result = count;
            count = 0;
        }  // 鎖在這裡釋放

        // 後續處理不需要鎖
        return result;
    }

    // ✓ 最佳實踐 3：const 方法也要保護
    int get() const {
        std::lock_guard<std::mutex> lock(mtx);
        return count;
    }

    // ✓ 最佳實踐 4：例外安全
    void riskyOperation(int value) {
        std::lock_guard<std::mutex> lock(mtx);

        if (value < 0) {
            throw std::invalid_argument("負值");
            // 例外拋出時 lock_guard 自動解鎖
        }

        count += value;
    }

    // ✓ 最佳實踐 5：提供【組合好的】原子操作
    //   使用者若自己寫 if (c.get() < limit) c.increment();
    //   就會是 TOCTOU 錯誤（見常見錯誤12）。
    //   正解是由類別直接提供這個複合操作，讓檢查與動作在同一個臨界區段。
    bool incrementIfBelow(int limit) {
        std::lock_guard<std::mutex> lock(mtx);
        if (count >= limit) return false;
        ++count;
        return true;
    }

    // ✓ 最佳實踐 6：真的需要複製時，鎖住來源再複製【資料】（不是複製鎖）
    SafeCounter() = default;

    SafeCounter(const SafeCounter& other) {
        std::lock_guard<std::mutex> lock(other.mtx);
        count = other.count;
    }

    SafeCounter& operator=(const SafeCounter& other) {
        if (this == &other) return *this;             // 自我賦值防護
        // ⚠️ 兩個物件都要鎖 → 必須用 scoped_lock，
        //    否則 a = b 與 b = a 同時發生就是 AB-BA 死結
        std::scoped_lock lock(mtx, other.mtx);
        count = other.count;
        return *this;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】指標註冊表（metrics registry）：把六條守則一次用上
//   情境：服務要記錄各種指標（請求數、錯誤數、各端點的計數），
//         由大量工作執行緒並行更新，另有監控執行緒定期匯出快照。
//   本例逐條對應守則：
//     1. 全部用 lock_guard，沒有一處手寫 unlock
//     2. 匯出快照時先複製到區域變數，格式化在鎖外做（臨界區段最小）
//     3. 但 recordRequest 的「累加 + 更新最大值」在同一個臨界區段
//        （不變量不可拆散）
//     4. snapshot() 是 const 方法 → mutex 宣告為 mutable
//     5. 可能拋例外的字串配置全部在鎖外（例外安全）
//     6. 提供 recordIfUnderQuota() 這種組合好的原子操作，
//        而不是讓使用者自己 if (count < quota) 再 record
// -----------------------------------------------------------------------------
class MetricsRegistry {
public:
    struct Snapshot {
        long requests = 0;
        long errors   = 0;
        long maxLatencyMs = 0;
    };

private:
    mutable std::mutex mtx_;          // 守則 4：const 方法要能上鎖
    long requests_ = 0;
    long errors_   = 0;
    long maxLatencyMs_ = 0;

public:
    // 守則 3：三個欄位的更新必須是一個整體，不可為了「縮小臨界區段」而拆開
    void recordRequest(long latencyMs, bool failed) {
        std::lock_guard<std::mutex> lock(mtx_);      // 守則 1：RAII
        ++requests_;
        if (failed) ++errors_;
        if (latencyMs > maxLatencyMs_) maxLatencyMs_ = latencyMs;
    }

    // 守則 6：組合好的原子操作。使用者若自己寫
    //   if (m.snapshot().requests < quota) m.recordRequest(...)
    // 就會是 TOCTOU 錯誤。
    bool recordIfUnderQuota(long latencyMs, bool failed, long quota) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (requests_ >= quota) return false;
        ++requests_;
        if (failed) ++errors_;
        if (latencyMs > maxLatencyMs_) maxLatencyMs_ = latencyMs;
        return true;
    }

    // 守則 2 + 4：const 方法，且只在鎖內複製資料
    Snapshot snapshot() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return Snapshot{requests_, errors_, maxLatencyMs_};
    }

    // 守則 2 + 5：字串格式化（可能拋 bad_alloc）在【鎖外】進行
    std::string exportText() const {
        const Snapshot snap = snapshot();            // 短臨界區段，取完就放
        // ── 以下完全不持有鎖 ──
        return "requests=" + std::to_string(snap.requests) +
               " errors="  + std::to_string(snap.errors) +
               " max_latency_ms=" + std::to_string(snap.maxLatencyMs);
    }
};

int main() {
    std::cout << "=== 課程示範: SafeCounter 最佳實踐 ===" << std::endl;
    SafeCounter counter;

    counter.increment();
    counter.increment();
    std::cout << "Count: " << counter.get() << std::endl;

    try {
        counter.riskyOperation(-1);
    } catch (...) {
        std::cout << "例外被捕獲，鎖已正確釋放" << std::endl;
    }

    counter.increment();  // 正常運作
    std::cout << "Final count: " << counter.get() << std::endl;

    std::cout << "\n=== 守則 5: 提供組合好的原子操作 ===" << std::endl;
    {
        SafeCounter limited;
        // 16 條執行緒搶著遞增，但上限是 100
        std::vector<std::thread> workers;
        std::atomic<int> succeeded{0};
        for (int t = 0; t < 16; ++t) {
            workers.emplace_back([&limited, &succeeded]() {
                for (int i = 0; i < 50; ++i) {
                    if (limited.incrementIfBelow(100)) {
                        succeeded.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            });
        }
        for (auto& t : workers) t.join();

        std::cout << "16 執行緒共嘗試 800 次，上限 100" << std::endl;
        std::cout << "最終計數: " << limited.get()
                  << "  (必須恰好是 100，絕不超過)" << std::endl;
        std::cout << "成功次數: " << succeeded.load()
                  << "  (必須等於最終計數)" << std::endl;
        std::cout << "說明: 若讓使用者自己寫 if (get() < 100) increment()，"
                     "就會超過上限（TOCTOU）" << std::endl;
    }

    std::cout << "\n=== 守則 6: 含 mutex 的類別如何正確複製 ===" << std::endl;
    {
        SafeCounter original;
        for (int i = 0; i < 42; ++i) original.increment();

        SafeCounter copied(original);        // 複製建構：鎖住來源再複製資料
        std::cout << "複製建構後: 原件 = " << original.get()
                  << ", 複本 = " << copied.get() << "  (預期都是 42)" << std::endl;

        SafeCounter assigned;
        assigned = original;                 // 複製指派：用 scoped_lock 鎖住兩者
        std::cout << "複製指派後: 複本 = " << assigned.get()
                  << "  (預期 42)" << std::endl;
        std::cout << "說明: 複製的是【資料】不是鎖；"
                     "指派要用 scoped_lock 同時鎖住兩個物件以免死結" << std::endl;
    }

    std::cout << "\n=== 日常實務: 指標註冊表（六條守則一次用上）===" << std::endl;
    {
        MetricsRegistry metrics;

        // 8 條工作執行緒並行記錄
        std::vector<std::thread> workers;
        for (int t = 0; t < 8; ++t) {
            workers.emplace_back([&metrics, t]() {
                for (int i = 1; i <= 1000; ++i) {
                    const long latency = static_cast<long>(t + 1) * i % 500;
                    metrics.recordRequest(latency, i % 100 == 0);
                }
            });
        }

        // 監控執行緒同時匯出快照（const 方法，靠 mutable mutex 上鎖）
        std::atomic<bool> stop{false};
        std::thread monitor([&metrics, &stop]() {
            while (!stop.load(std::memory_order_acquire)) {
                const std::string text = metrics.exportText();
                (void)text;   // 實際服務會寫入監控系統
            }
        });

        for (auto& t : workers) t.join();
        stop.store(true, std::memory_order_release);
        monitor.join();

        const MetricsRegistry::Snapshot snap = metrics.snapshot();
        std::cout << "8 執行緒各記錄 1000 筆" << std::endl;
        std::cout << "總請求數: " << snap.requests
                  << "  (必須是 8000，一筆都不能少)" << std::endl;
        std::cout << "錯誤數:   " << snap.errors
                  << "  (必須是 80——每 100 筆有 1 筆失敗)" << std::endl;
        std::cout << "監控執行緒同時匯出快照，未干擾工作執行緒的正確性: 是"
                  << std::endl;

        // 配額版本
        MetricsRegistry quota;
        std::vector<std::thread> quotaWorkers;
        for (int t = 0; t < 8; ++t) {
            quotaWorkers.emplace_back([&quota]() {
                for (int i = 0; i < 500; ++i) {
                    quota.recordIfUnderQuota(10, false, 1000);
                }
            });
        }
        for (auto& t : quotaWorkers) t.join();
        std::cout << "配額版: 8 執行緒共嘗試 4000 筆，配額 1000，實際記錄: "
                  << quota.snapshot().requests << "  (必須恰好 1000)" << std::endl;
    }

    std::cout << "\n=== 六條守則總結 ===" << std::endl;
    std::cout << "1. 一律用 RAII（lock_guard / unique_lock / scoped_lock）" << std::endl;
    std::cout << "2. 臨界區段最小化，耗時工作與 I/O 放在鎖外" << std::endl;
    std::cout << "3. 但不可小到把「檢查」與「動作」拆開（TOCTOU）" << std::endl;
    std::cout << "4. const 方法也要保護 → mutex 宣告為 mutable" << std::endl;
    std::cout << "5. RAII 只給基本保證；資料一致性要另外設計" << std::endl;
    std::cout << "6. 多把鎖用 scoped_lock；mutex 一律 private" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.4：互斥鎖的常見錯誤14.cpp' -o best_practices

// -----------------------------------------------------------------------------
// 【輸出但書】
//   本檔輸出完全確定：所有多執行緒區段都只驗證不變量
//   （總數、上限、配額），不依賴任何排程順序。
//   「哪一條執行緒搶到配額」每次都不同，但總量必然相同。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// === 課程示範: SafeCounter 最佳實踐 ===
// Count: 2
// 例外被捕獲，鎖已正確釋放
// Final count: 3
//
// === 守則 5: 提供組合好的原子操作 ===
// 16 執行緒共嘗試 800 次，上限 100
// 最終計數: 100  (必須恰好是 100，絕不超過)
// 成功次數: 100  (必須等於最終計數)
// 說明: 若讓使用者自己寫 if (get() < 100) increment()，就會超過上限（TOCTOU）
//
// === 守則 6: 含 mutex 的類別如何正確複製 ===
// 複製建構後: 原件 = 42, 複本 = 42  (預期都是 42)
// 複製指派後: 複本 = 42  (預期 42)
// 說明: 複製的是【資料】不是鎖；指派要用 scoped_lock 同時鎖住兩個物件以免死結
//
// === 日常實務: 指標註冊表（六條守則一次用上）===
// 8 執行緒各記錄 1000 筆
// 總請求數: 8000  (必須是 8000，一筆都不能少)
// 錯誤數:   80  (必須是 80——每 100 筆有 1 筆失敗)
// 監控執行緒同時匯出快照，未干擾工作執行緒的正確性: 是
// 配額版: 8 執行緒共嘗試 4000 筆，配額 1000，實際記錄: 1000  (必須恰好 1000)
//
// === 六條守則總結 ===
// 1. 一律用 RAII（lock_guard / unique_lock / scoped_lock）
// 2. 臨界區段最小化，耗時工作與 I/O 放在鎖外
// 3. 但不可小到把「檢查」與「動作」拆開（TOCTOU）
// 4. const 方法也要保護 → mutex 宣告為 mutable
// 5. RAII 只給基本保證；資料一致性要另外設計
// 6. 多把鎖用 scoped_lock；mutex 一律 private
