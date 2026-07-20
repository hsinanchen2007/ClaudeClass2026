// =============================================================================
//  課程 5.1：stdmutex 基本操作4.cpp  —  忘記 unlock() 的後果（刻意示範錯誤）
// =============================================================================
//
// 【主題資訊 Information】
//   class std::mutex;                                       // C++11，<mutex>
//       void lock();      // 已被【其他執行緒】持有時：阻塞等待（標準保證）
//       bool try_lock();  // 非阻塞，失敗回 false
//       void unlock();    // 只能由【持有它的那條執行緒】呼叫
//   標頭檔：<mutex>
//   關鍵性質：std::mutex 沒有「擁有者自動釋放」機制——
//             持有鎖的執行緒結束時，鎖【不會】被自動解開。
//
// 【詳細解釋 Explanation】
//
// 【1. 本檔的錯誤【不是】未定義行為，請先分清楚】
//   這點非常重要，面試也常考。本檔的情境是：
//       t1 取得 mtx → 提前 return，沒有 unlock → t1 結束
//       t2（另一條執行緒）呼叫 mtx.lock()
//   對一把【已被別的執行緒持有】的 mutex 呼叫 lock()，
//   依標準就是「阻塞等待直到取得」——這是完全良好定義的行為。
//   問題在於這把鎖永遠不會被釋放，所以 t2 會【確定地、永久地】卡住。
//   → 分類：這是「標準保證的永久阻塞」，不是 UB。
//   對比一下，同課其他檔案示範的是【真正的 UB】：
//       同一條執行緒對已持有的 std::mutex 再次 lock()（不可重入）→ UB。
//   兩者症狀看起來都是「程式停住」，但性質完全不同：
//   前者標準保證會卡住；後者標準什麼都不保證，換平台可能是別的症狀。
//
// 【2. 為什麼執行緒結束不會自動解鎖】
//   std::mutex 是一個普通的物件，它的生命週期跟著它所在的儲存區走
//   （本檔是全域變數），跟哪條執行緒鎖了它毫無關係。
//   標準只要求「unlock() 必須由目前持有它的執行緒呼叫」，
//   從來沒有承諾「執行緒結束時幫你解開」。
//   作業系統層面也是如此：pthread_mutex 預設不是 robust mutex，
//   持有者死亡後鎖就永遠是鎖住狀態（Linux 有 PTHREAD_MUTEX_ROBUST
//   可讓下一個等待者拿到 EOWNERDEAD 並自行修復，但 std::mutex 不暴露這個能力）。
//
// 【3. 為什麼「早期 return」是最常見的肇因】
//   本檔的 badFunction 只有一個 if 就漏掉解鎖。真實程式碼更難防：
//     * 函式有五、六條 return 路徑，新人加第七條時忘了 unlock；
//     * 中間某個呼叫拋出例外，整段直接跳走，unlock() 根本沒機會執行；
//     * 有人在臨界區段中間加了 continue / break / goto。
//   人類靠紀律記得解鎖是不可靠的，這正是 RAII 存在的理由：
//   把「解鎖」綁在物件解構上，由編譯器保證所有離開路徑都會執行。
//
// 【4. 正確寫法】
//       void goodFunction() {
//           std::lock_guard<std::mutex> lock(mtx);   // 建構即 lock
//           if (...) return;                          // 解構自動 unlock
//           throw std::runtime_error("x");            // 解構同樣自動 unlock
//       }                                             // 正常結束也自動 unlock
//   lock_guard 沒有任何額外狀態，最佳化後與手寫 lock/unlock 的成本相同，
//   但把「忘記解鎖」這個 bug 類別從程式碼中徹底移除。
//
// 【概念補充 Concept Deep Dive】
//   * 為什麼這個 bug 特別難除錯：程式沒有 crash、沒有錯誤訊息、
//     沒有任何日誌，就只是「不動了」。事後看到的現象是
//     「服務 hang 住、CPU 使用率 0%」——因為所有執行緒都阻塞在鎖上，
//     一顆 CPU 都沒在燒。這與「無窮迴圈導致的 100% CPU」是完全相反的徵狀。
//   * 診斷手段：對卡住的行程下 `gdb -p <pid>` 再 `thread apply all bt`，
//     或直接 `eu-stack -p <pid>`，就會看到所有執行緒的堆疊都停在
//     __lll_lock_wait / pthread_mutex_lock。
//     ThreadSanitizer（-fsanitize=thread）對這種「鎖洩漏」也有幫助。
//   * 本檔用 -DDEMONSTRATE_UB 把危險示範關在編譯開關後面，
//     這樣整份課程可以被批次編譯執行而不會卡死。
//     （沿用課程既有的巨集名稱；嚴格說本檔示範的不是 UB 而是永久阻塞，
//       這一點已在上面第 1 點說明清楚。）
//
// 【注意事項 Pay Attention】
//   1. 本檔的永久阻塞是【標準保證】的行為，不是 UB——別把兩者混為一談。
//   2. 執行緒結束【不會】自動釋放它持有的 std::mutex。
//   3. unlock() 只能由持有者呼叫；由別條執行緒 unlock 是 UB。
//   4. 對未鎖定的 mutex 呼叫 unlock() 也是 UB。
//   5. 正式程式碼一律用 lock_guard / unique_lock / scoped_lock，
//      不要手寫 lock/unlock 配對。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】忘記 unlock 與 RAII
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 執行緒 A 鎖住 mutex 之後就結束了（沒有 unlock），
//        執行緒 B 再去 lock() 這把鎖，會發生什麼？這是 UB 嗎？
//     答：B 會永久阻塞，而且這【不是】UB。對已被其他執行緒持有的 mutex
//         呼叫 lock() 就是「阻塞等待」，標準定義得很清楚；
//         只是這把鎖永遠不會被釋放，所以等待永遠不會結束。
//         std::mutex 不會因為持有者執行緒結束而自動解鎖。
//     追問：那什麼情況才是 UB？→ 同一條執行緒對已持有的 std::mutex
//           再次 lock()（不可重入）、對未持有的 mutex 呼叫 unlock()、
//           由非持有者 unlock()——這三種都是 UB。
//
// 🔥 Q2. 服務在生產環境「hang 住」了，CPU 使用率接近 0%。
//        你會先懷疑什麼？怎麼確認？
//     答：CPU 0% 代表沒有執行緒在跑，幾乎可以確定是「全部阻塞」——
//         死結、鎖洩漏（本檔的情況）、或等待永遠不會到來的 I/O。
//         確認方式：對行程取堆疊（gdb -p + thread apply all bt，
//         或 eu-stack -p），看是不是所有執行緒都停在 pthread_mutex_lock。
//     追問：如果 CPU 是 100% 呢？→ 那方向相反，要懷疑忙等迴圈、
//           活鎖（livelock）、或無窮迴圈——執行緒都是 runnable 的。
//
// ⚠️ 陷阱. 「我在函式最後有寫 mtx.unlock()，所以一定會解鎖。」
//     答：只有在「執行流程真的走到那一行」時才成立。
//         中間任何一條 return、break、goto，或任何一個會拋出例外的呼叫
//         （包括 new、std::vector::push_back 的重新配置、
//           std::string 的配置、被呼叫函式內部的 throw），
//         都會讓那一行被跳過。
//     為什麼會錯：多數人把函式想成「由上往下一定會執行到底」的直線，
//         忽略了例外是一條【看不見的分支】——原始碼裡沒有任何字元
//         標示它會從這裡跳走。這正是 RAII 要解決的核心問題。
// ═══════════════════════════════════════════════════════════════════════════

/*
好的，讓我先查看我們之前的課程進度和大綱。找到了課程大綱。讓我再確認第五階段的完整內容：很好，我確認了課程進度。你已經完成了第四階段（共享資料與競爭條件），現在正式開始第五階段。

---

# 第五階段：互斥鎖基礎 (std::mutex)

## 課程 5.1：std::mutex 基本操作

---

### 引言

在第四階段，我們深入了解了競爭條件的危害。現在，我們要學習解決這個問題的第一個武器：**互斥鎖（Mutex）**。Mutex 是 **Mut**ual **Ex**clusion 的縮寫，意思是「互相排斥」，確保同一時間只有一個執行緒能存取共享資源。

---

### 一、std::mutex 基本概念

```
┌─────────────────────────────────────────────────────────────┐
│                    互斥鎖（Mutex）概念                       │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  想像一間只能容納一人的更衣室：                               │
│                                                             │
│    ┌──────────┐                                             │
│    │  更衣室  │  ← 共享資源（如：全域變數）                   │
│    │  🔒門鎖  │  ← 互斥鎖                                    │
│    └──────────┘                                             │
│                                                             │
│  規則：                                                      │
│  • 進入前必須鎖門（lock）                                    │
│  • 離開後必須開門（unlock）                                  │
│  • 門鎖著時，其他人必須在外等待                              │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 二、std::mutex 的三個基本操作

```cpp
#include <mutex>

std::mutex mtx;  // 建立互斥鎖

mtx.lock();      // 🔒 獲取鎖（若已被鎖定，則阻塞等待）
// ... 臨界區段：存取共享資源 ...
mtx.unlock();    // 🔓 釋放鎖
```

| 方法 | 功能 | 阻塞行為 |
|------|------|----------|
| `lock()` | 獲取鎖 | 若鎖已被持有，**阻塞等待**直到獲取成功 |
| `unlock()` | 釋放鎖 | 非阻塞 |
| `try_lock()` | 嘗試獲取鎖 | **非阻塞**，立即返回成功或失敗 |

---

### 三、解決競爭條件：完整範例

#### 問題回顧（無保護的程式碼）

```cpp
// 檔案：lesson_5_1_no_mutex.cpp
// 說明：沒有互斥鎖保護的計數器（有競爭條件）

#include <iostream>
#include <thread>

int counter = 0;  // 共享資源

void increment() {
    for (int i = 0; i < 100000; ++i) {
        ++counter;  // 危險！非原子操作
    }
}

int main() {
    std::thread t1(increment);
    std::thread t2(increment);
    
    t1.join();
    t2.join();
    
    std::cout << "預期: 200000" << std::endl;
    std::cout << "實際: " << counter << std::endl;  // 通常小於 200000
    
    return 0;
}
```

#### 編譯與執行

```bash
g++ -std=c++17 -pthread -o no_mutex lesson_5_1_no_mutex.cpp
./no_mutex
```

#### 可能的輸出

```
預期: 200000
實際: 143287
```

---

#### 解決方案（使用 std::mutex）

```cpp
// 檔案：lesson_5_1_with_mutex.cpp
// 說明：使用互斥鎖保護共享資源

#include <iostream>
#include <thread>
#include <mutex>

int counter = 0;      // 共享資源
std::mutex mtx;       // 保護 counter 的互斥鎖

void increment() {
    for (int i = 0; i < 100000; ++i) {
        mtx.lock();       // 🔒 進入臨界區段前獲取鎖
        ++counter;        // 安全！一次只有一個執行緒能執行
        mtx.unlock();     // 🔓 離開臨界區段後釋放鎖
    }
}

int main() {
    std::thread t1(increment);
    std::thread t2(increment);
    
    t1.join();
    t2.join();
    
    std::cout << "預期: 200000" << std::endl;
    std::cout << "實際: " << counter << std::endl;  // 保證是 200000
    
    return 0;
}
```

#### 編譯與執行

```bash
g++ -std=c++17 -pthread -o with_mutex lesson_5_1_with_mutex.cpp
./with_mutex
```

#### 輸出

```
預期: 200000
實際: 200000
```

---

### 四、執行流程圖解

```
時間軸 ──────────────────────────────────────────────────────────►

執行緒 1:   lock()─────[++counter]─────unlock()   lock()─────...
                                                    │
                                                    │ 獲取鎖
                                                    ▼
執行緒 2:   lock()  ════════等待════════════════  [++counter]─unlock()
              │                                      ▲
              │ 嘗試獲取鎖，但被阻塞                   │
              └──────────────────────────────────────┘
                    執行緒 1 釋放鎖後，執行緒 2 獲取鎖

════════ 表示阻塞等待
─────── 表示持有鎖執行
```

---

### 五、多個操作的保護

當臨界區段包含多個操作時：

```cpp
// 檔案：lesson_5_1_multiple_ops.cpp
// 說明：保護多個相關操作

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>

std::vector<int> data;
std::mutex mtx;

void addAndPrint(int value) {
    mtx.lock();
    
    // 以下三個操作必須作為一個整體執行
    data.push_back(value);                    // 操作 1：新增元素
    std::cout << "Added: " << value;          // 操作 2：輸出訊息
    std::cout << ", Size: " << data.size();   // 操作 3：輸出大小
    std::cout << std::endl;
    
    mtx.unlock();
}

int main() {
    std::thread t1([]() {
        for (int i = 0; i < 5; ++i) {
            addAndPrint(i);
        }
    });
    
    std::thread t2([]() {
        for (int i = 100; i < 105; ++i) {
            addAndPrint(i);
        }
    });
    
    t1.join();
    t2.join();
    
    return 0;
}
```

#### 輸出（順序可能不同，但每行是完整的）

```
Added: 0, Size: 1
Added: 100, Size: 2
Added: 1, Size: 3
Added: 101, Size: 4
Added: 2, Size: 5
...
```

若沒有互斥鎖，輸出可能會交錯混亂：

```
Added: 0Added: 100, Size: , Size: 21
```

---

### 六、lock() 與 unlock() 的配對規則

```
┌─────────────────────────────────────────────────────────────┐
│                    配對規則（重要！）                        │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ✓ 正確：每個 lock() 都有對應的 unlock()                    │
│                                                             │
│      mtx.lock();                                            │
│      // 工作                                                │
│      mtx.unlock();  // ✓ 配對                               │
│                                                             │
│  ✗ 錯誤 1：忘記 unlock()                                    │
│                                                             │
│      mtx.lock();                                            │
│      // 工作                                                │
│      // 沒有 unlock() → 其他執行緒永遠等待（死結）           │
│                                                             │
│  ✗ 錯誤 2：重複 lock()（同一執行緒）                        │
│                                                             │
│      mtx.lock();                                            │
│      mtx.lock();  // 💀 未定義行為！（標準 mutex 不支援）    │
│                                                             │
│  ✗ 錯誤 3：unlock() 未鎖定的互斥鎖                          │
│                                                             │
│      mtx.unlock();  // 💀 未定義行為！                       │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 七、忘記 unlock() 的危險

```cpp
// 檔案：lesson_5_1_forget_unlock.cpp
// 說明：忘記 unlock() 導致的問題（示範用，不要在實際專案這樣寫）

#include <iostream>
#include <thread>
#include <mutex>

std::mutex mtx;

void badFunction() {
    mtx.lock();
    std::cout << "Thread " << std::this_thread::get_id() << " acquired lock\n";
    
    // 假設這裡發生早期返回或拋出例外
    if (true) {
        return;  // 💀 忘記 unlock()！
    }
    
    mtx.unlock();  // 永遠不會執行
}

int main() {
    std::thread t1(badFunction);
    t1.join();
    
    std::cout << "Thread 1 finished\n";
    
    std::thread t2(badFunction);  // 💀 永遠卡住！因為鎖沒被釋放
    t2.join();
    
    std::cout << "Thread 2 finished\n";  // 永遠不會執行
    
    return 0;
}
```

這就是為什麼我們需要 RAII 鎖管理器（下一階段會學習）。

---

### 八、std::mutex 的特性

```
┌─────────────────────────────────────────────────────────────┐
│                   std::mutex 特性                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  • 不可複製、不可移動                                        │
│    std::mutex mtx1;                                         │
│    std::mutex mtx2 = mtx1;  // ✗ 編譯錯誤                   │
│                                                             │
│  • 不支援遞迴鎖定                                           │
│    同一執行緒 lock() 兩次 → 未定義行為                       │
│    （需要遞迴鎖定時，使用 std::recursive_mutex）             │
│                                                             │
│  • lock() 可能拋出例外                                      │
│    std::system_error（如：資源不足）                        │
│                                                             │
│  • 預設建構，無需參數                                        │
│    std::mutex mtx;  // 即可使用                             │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 九、本課重點回顧

1. **std::mutex** 是最基本的同步機制，確保臨界區段的互斥存取
2. 三個基本操作：`lock()`、`unlock()`、`try_lock()`
3. `lock()` 會阻塞等待直到獲取鎖
4. **每個 `lock()` 必須有對應的 `unlock()`**
5. 忘記 `unlock()` 會導致其他執行緒永遠等待
6. std::mutex 不支援同一執行緒重複鎖定

---

### 十、練習題

**練習 1**：修改以下程式碼，使用 std::mutex 解決競爭條件

```cpp
#include <iostream>
#include <thread>

int balance = 1000;

void withdraw(int amount) {
    if (balance >= amount) {
        balance -= amount;
        std::cout << "Withdrew: " << amount << ", Balance: " << balance << std::endl;
    }
}

int main() {
    std::thread t1(withdraw, 800);
    std::thread t2(withdraw, 800);
    
    t1.join();
    t2.join();
    
    std::cout << "Final balance: " << balance << std::endl;
    return 0;
}
```

---

### 下一課預告

在 **課程 5.2：互斥鎖的工作原理** 中，我們將深入了解：
- 互斥鎖在作業系統層面如何運作
- 自旋鎖 vs 睡眠鎖
- 鎖的獲取與釋放過程

---

準備好繼續嗎？
*/



// 檔案：lesson_5_1_forget_unlock.cpp
// 說明：忘記 unlock() 導致的問題（示範用，不要在實際專案這樣寫）
//
// 【本檔是「刻意示範錯誤」的範例，開啟示範後程式會永遠停住，不會自己結束】
//
// ⚠️ 這【不是】未定義行為：mtx 被 t1 鎖住後從未解鎖，
//    t2 是【另一個執行緒】，對已鎖住的 mutex 呼叫 lock() 依標準就是阻塞等待；
//    而這把鎖永遠不會被釋放（std::mutex 不會因持有者執行緒結束而自動解鎖），
//    所以 t2 會確定地、永久地卡在 lock()。
//
//    實測（g++ 15.2、Ubuntu 26.04）：印出 "Thread 1 finished" 後即停住，
//    程式不會自行終止（timeout 觀察到 exit=124）。
//
// 為了不讓整份課程的批次執行卡死，這個示範預設【關閉】。
// 要親眼觀察，請自行加上 -DDEMONSTRATE_UB：
//    g++ -std=c++17 -pthread -DDEMONSTRATE_UB -o forget_unlock '課程 5.1：stdmutex 基本操作4.cpp'
//    timeout 5 ./forget_unlock ; echo "exit=$?"   # 預期 exit=124（逾時）
//
// ✅ 正確作法：用 std::lock_guard（RAII），
//    不論是早期 return 還是拋出例外，解構函式都會保證解鎖。

#include <chrono>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

std::mutex mtx;

void badFunction() {
    mtx.lock();
    // 註：原版這裡印的是 std::this_thread::get_id()。
    //     那個值每次執行都不同、格式又是實作定義的，無法列入預期輸出，
    //     而且它跟本檔要示範的「忘記解鎖」毫無關係，故改印固定標籤。
    std::cout << "Thread A acquired lock\n";

    // 假設這裡發生早期返回或拋出例外
    if (true) {
        return;  // 💀 忘記 unlock()！
    }

    mtx.unlock();  // 永遠不會執行
}

// -----------------------------------------------------------------------------
// 【正確對照組】同樣的早期 return，用 lock_guard 就不可能漏解鎖
// -----------------------------------------------------------------------------
std::mutex goodMtx;

bool goodFunction(int input) {
    std::lock_guard<std::mutex> lock(goodMtx);   // 建構即 lock

    if (input < 0) {
        return false;      // ✓ 解構自動 unlock
    }
    if (input == 0) {
        throw std::invalid_argument("input 不可為 0");   // ✓ 例外路徑也自動 unlock
    }
    return true;           // ✓ 正常路徑同樣自動 unlock
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔【略過】，理由如下：
//   本檔示範的是「鎖洩漏導致永久阻塞」這個【錯誤模式】，
//   而 LeetCode 的並行題（1114 / 1115 / 1116 / 1117 / 1195）全部都是
//   「寫出正確的同步」的題目，評測系統會直接判逾時，
//   沒有任何一題在考「忘記解鎖會怎樣」。
//   硬掛一題只會模糊本檔的重點，故從缺。
//   （lock_guard 的正面應用已在同課「基本操作3」以 LeetCode 146 示範。）
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】連線池 acquire/release —— 鎖洩漏在真實服務中長什麼樣
//   情境：資料庫連線池用一把 mutex 保護可用連線清單。
//         acquire() 裡若有一條錯誤路徑忘了解鎖（例如「池已耗盡」時直接 return），
//         第一次觸發那條路徑之後，整個服務的所有 DB 操作就全部永久卡死——
//         而且不會有任何錯誤訊息，只會看到「服務沒回應、CPU 0%」。
//   本例示範正確版本：用 RAII 保證每條路徑都解鎖，
//         並用一個 RAII 的連線代理（ConnectionLease）保證連線一定被歸還。
// -----------------------------------------------------------------------------
class ConnectionPool {
private:
    std::mutex mtx_;
    std::vector<std::string> idle_;
    int leakedAttempts_ = 0;

public:
    explicit ConnectionPool(int n) {
        for (int i = 0; i < n; ++i) {
            idle_.push_back("conn-" + std::to_string(i));
        }
    }

    // 取得一條連線；池空時回傳空字串。
    // ✓ 每一條 return 路徑都由 lock_guard 保證解鎖
    std::string tryAcquire() {
        std::lock_guard<std::mutex> lock(mtx_);

        if (idle_.empty()) {
            ++leakedAttempts_;
            return "";        // ← 真實 bug 常發生在這種「例外狀況」的提前 return
        }

        std::string conn = idle_.back();
        idle_.pop_back();
        return conn;
    }

    void release(const std::string& conn) {
        std::lock_guard<std::mutex> lock(mtx_);
        idle_.push_back(conn);
    }

    std::size_t idleCount() {
        std::lock_guard<std::mutex> lock(mtx_);
        return idle_.size();
    }

    int exhaustedCount() {
        std::lock_guard<std::mutex> lock(mtx_);
        return leakedAttempts_;
    }
};

// RAII 代理：離開作用域一定歸還連線，即使中途拋出例外
class ConnectionLease {
private:
    ConnectionPool& pool_;
    std::string     conn_;

public:
    ConnectionLease(ConnectionPool& pool, std::string conn)
        : pool_(pool), conn_(std::move(conn)) {}

    ~ConnectionLease() {
        if (!conn_.empty()) pool_.release(conn_);
    }

    ConnectionLease(const ConnectionLease&)            = delete;
    ConnectionLease& operator=(const ConnectionLease&) = delete;

    bool valid() const { return !conn_.empty(); }
};

int main() {
    std::thread t1(badFunction);
    t1.join();

    std::cout << "Thread 1 finished\n";

#ifdef DEMONSTRATE_UB
    std::thread t2(badFunction);  // 💀 因為鎖沒被釋放，t2 會永遠卡在 lock()
    t2.join();                    // 💀 永遠不會返回

    std::cout << "Thread 2 finished\n";  // 永遠不會執行
#else
    std::cout << "已略過會永久卡住的第二個執行緒（預設關閉）。\n";
    std::cout << "要重現請加上 -DDEMONSTRATE_UB 重新編譯，"
                 "並用 timeout 觀察它不會自行結束。\n";
#endif

    // ── 正確對照組：三條離開路徑都會自動解鎖 ──
    std::cout << "\n=== 正確對照組: lock_guard 的三條離開路徑 ===\n";
    std::cout << "goodFunction(5)  正常返回 -> " << (goodFunction(5) ? "true" : "false") << "\n";
    std::cout << "goodFunction(-1) 提前返回 -> " << (goodFunction(-1) ? "true" : "false") << "\n";
    try {
        goodFunction(0);
    } catch (const std::invalid_argument& e) {
        std::cout << "goodFunction(0)  拋出例外 -> " << e.what() << "\n";
    }
    // 若上面任何一條路徑漏了解鎖，下面這行就會永久卡住
    std::cout << "三條路徑後仍可再次取得同一把鎖 -> "
              << (goodFunction(7) ? "是" : "否") << "\n";

    // ── 日常實務：連線池 ──
    std::cout << "\n=== 日常實務: 連線池 acquire/release ===\n";
    {
        ConnectionPool pool(4);          // 只有 4 條連線
        std::vector<std::thread> workers;
        std::vector<int> served(8, 0);

        for (int w = 0; w < 8; ++w) {    // 8 條 worker 搶 4 條連線
            workers.emplace_back([&pool, &served, w]() {
                for (int i = 0; i < 50; ++i) {
                    // 池暫時空了就重試，直到真的拿到連線為止
                    //（不能用 continue 跳過這一輪，否則完成次數會變成不確定）
                    std::string conn;
                    while ((conn = pool.tryAcquire()).empty()) {
                        std::this_thread::sleep_for(std::chrono::microseconds(50));
                    }
                    ConnectionLease lease(pool, conn);   // RAII：離開必歸還
                    if (lease.valid()) ++served[w];
                }
            });
        }
        for (auto& t : workers) t.join();

        int total = 0;
        for (int c : served) total += c;

        std::cout << "8 條 worker 各完成 50 次請求，總計: " << total << " 次\n";
        std::cout << "全部歸還後閒置連線數: " << pool.idleCount()
                  << "  (必須回到 4，一條都不能漏)\n";
        // 註：pool.exhaustedCount()（撞到池耗盡而重試的次數）完全取決於排程，
        //     每次執行都不同，故刻意不列入輸出，只驗證上面兩個不變量。
        std::cout << "池耗盡重試次數: 每次執行都不同，不列為預期輸出\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.1：stdmutex 基本操作4.cpp' -o forget_unlock
// 觀察永久阻塞（會卡住，請務必加 timeout）:
//   g++ -std=c++17 -Wall -Wextra -pthread -DDEMONSTRATE_UB '課程 5.1：stdmutex 基本操作4.cpp' -o forget_unlock_ub
//   timeout 5 ./forget_unlock_ub ; echo "exit=$?"     # 本機實測 exit=124（逾時）

// -----------------------------------------------------------------------------
// 【輸出但書】
//   1. 以下是【預設編譯】（不加 -DDEMONSTRATE_UB）的輸出。
//      加上 -DDEMONSTRATE_UB 之後程式會停在 t2.join() 永不返回，
//      本機實測 timeout 5 觀察到 exit=124。
//   2. 連線池的「池耗盡重試次數」完全由排程決定，每次執行都不同，
//      故刻意不印出數值；只驗證「總完成 400 次」與「連線全部歸還」兩個不變量。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// Thread A acquired lock
// Thread 1 finished
// 已略過會永久卡住的第二個執行緒（預設關閉）。
// 要重現請加上 -DDEMONSTRATE_UB 重新編譯，並用 timeout 觀察它不會自行結束。
//
// === 正確對照組: lock_guard 的三條離開路徑 ===
// goodFunction(5)  正常返回 -> true
// goodFunction(-1) 提前返回 -> false
// goodFunction(0)  拋出例外 -> input 不可為 0
// 三條路徑後仍可再次取得同一把鎖 -> 是
//
// === 日常實務: 連線池 acquire/release ===
// 8 條 worker 各完成 50 次請求，總計: 400 次
// 全部歸還後閒置連線數: 4  (必須回到 4，一條都不能漏)
// 池耗盡重試次數: 每次執行都不同，不列為預期輸出
