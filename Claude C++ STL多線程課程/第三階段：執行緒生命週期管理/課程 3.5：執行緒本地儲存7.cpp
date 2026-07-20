/*
# 第三階段：執行緒生命週期管理

## 課程 3.5：執行緒本地儲存

---

### 引言

有時我們希望每個執行緒都有自己獨立的變數副本，而不是共享同一個變數。C++11 引入的 `thread_local` 關鍵字正是為此設計。

---

### 一、thread_local 基本概念

```
┌─────────────────────────────────────────────────────────────┐
│                    變數儲存類型比較                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  全域/static 變數        thread_local 變數                  │
│  ─────────────────      ─────────────────                   │
│  所有執行緒共享一份       每個執行緒各有一份                  │
│  需要同步保護            不需要同步（各自獨立）               │
│  程式啟動時初始化         執行緒啟動時初始化                  │
│  程式結束時銷毀          執行緒結束時銷毀                    │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 二、基本用法

```cpp
#include <iostream>
#include <thread>

thread_local int counter = 0;  // 每個執行緒有獨立的 counter

void increment(const std::string& name) {
    for (int i = 0; i < 3; ++i) {
        ++counter;
        std::cout << name << ": counter = " << counter << std::endl;
    }
}

int main() {
    std::thread t1(increment, "Thread A");
    std::thread t2(increment, "Thread B");
    
    t1.join();
    t2.join();
    
    return 0;
}
```

輸出（順序可能不同）：
```
Thread A: counter = 1
Thread A: counter = 2
Thread A: counter = 3
Thread B: counter = 1
Thread B: counter = 2
Thread B: counter = 3
```

注意：兩個執行緒的 counter 各自獨立從 0 開始。

---

### 三、對比：有無 thread_local

```cpp
#include <iostream>
#include <thread>

int globalCounter = 0;              // 共享
thread_local int localCounter = 0;  // 各自獨立

void work(const std::string& name) {
    ++globalCounter;
    ++localCounter;
    
    std::cout << name 
              << " global=" << globalCounter 
              << " local=" << localCounter << std::endl;
}

int main() {
    std::thread t1(work, "A");
    std::thread t2(work, "B");
    std::thread t3(work, "C");
    
    t1.join();
    t2.join();
    t3.join();
    
    return 0;
}
```

可能的輸出：
```
A global=1 local=1
B global=2 local=1
C global=3 local=1
```

`globalCounter` 累加，`localCounter` 每個執行緒都是 1。

---

### 四、thread_local 的位置

可以用於三種地方：

```cpp
#include <iostream>
#include <thread>

// 1. 全域變數
thread_local int global_tl = 0;

void func() {
    // 2. 函式內的 static 變數
    thread_local static int local_tl = 0;
    
    ++global_tl;
    ++local_tl;
    
    std::cout << "global_tl=" << global_tl 
              << " local_tl=" << local_tl << std::endl;
}

class MyClass {
public:
    // 3. 類別的 static 成員
    thread_local static int member_tl;
};

thread_local int MyClass::member_tl = 0;

int main() {
    std::thread t1([]() { func(); func(); });
    std::thread t2([]() { func(); func(); });
    
    t1.join();
    t2.join();
    
    return 0;
}
```

---

### 五、常見用途

#### 用途一：錯誤碼（如 errno）

```cpp
#include <iostream>
#include <thread>

thread_local int lastError = 0;

void setError(int code) {
    lastError = code;
}

int getError() {
    return lastError;
}

void worker(int id) {
    setError(id * 100);
    std::cout << "Thread " << id << " error: " << getError() << std::endl;
}

int main() {
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    
    t1.join();
    t2.join();
    
    return 0;
}
```

---

#### 用途二：快取

```cpp
#include <iostream>
#include <thread>
#include <string>

thread_local std::string cache;

std::string expensiveCompute(int id) {
    if (cache.empty()) {
        // 模擬耗時計算
        cache = "Result for thread " + std::to_string(id);
        std::cout << "Computing..." << std::endl;
    }
    return cache;
}

void worker(int id) {
    std::cout << expensiveCompute(id) << std::endl;
    std::cout << expensiveCompute(id) << std::endl;  // 使用快取
}

int main() {
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    
    t1.join();
    t2.join();
    
    return 0;
}
```

輸出：
```
Computing...
Result for thread 1
Result for thread 1
Computing...
Result for thread 2
Result for thread 2
```

---

#### 用途三：隨機數產生器

```cpp
#include <iostream>
#include <thread>
#include <random>

thread_local std::mt19937 rng{std::random_device{}()};

int randomInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

void worker(int id) {
    std::cout << "Thread " << id << ": " 
              << randomInt(1, 100) << ", "
              << randomInt(1, 100) << ", "
              << randomInt(1, 100) << std::endl;
}

int main() {
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    
    t1.join();
    t2.join();
    
    return 0;
}
```

---

### 六、生命週期

```
┌─────────────────────────────────────────────────────────────┐
│              thread_local 變數的生命週期                     │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  執行緒啟動                                                  │
│      ↓                                                      │
│  首次存取時初始化（或執行緒開始時）                           │
│      ↓                                                      │
│  執行緒執行期間持續存在                                      │
│      ↓                                                      │
│  執行緒結束時銷毀（呼叫解構函式）                             │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 七、注意事項

```cpp
#include <iostream>
#include <thread>

class Resource {
public:
    Resource() { std::cout << "Resource 建構" << std::endl; }
    ~Resource() { std::cout << "Resource 銷毀" << std::endl; }
};

thread_local Resource res;  // 每個執行緒有自己的 Resource

void worker(int id) {
    std::cout << "Thread " << id << " 使用資源" << std::endl;
}

int main() {
    std::cout << "=== 主執行緒 ===" << std::endl;
    
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    
    t1.join();
    t2.join();
    
    std::cout << "=== 結束 ===" << std::endl;
    return 0;
}
```

輸出（⚠️ 勘誤：以下是「直覺以為」的輸出，實測並非如此）：
```
=== 主執行緒 ===
Resource 建構          ← 實際上不會出現
Thread 1 使用資源
Resource 建構          ← 實際上不會出現
Thread 2 使用資源
Resource 銷毀          ← 實際上不會出現
Resource 銷毀          ← 實際上不會出現
=== 結束 ===
```

⚠️ **本機實測的真正輸出**（GCC 15.2.0 / x86-64）：
```
=== 主執行緒 ===
Thread 1 使用資源
Thread 2 使用資源
=== 結束 ===
```

**一次 `Resource 建構` 都沒有。** 原因是 `worker()` 從頭到尾沒有碰過 `res`，
而需要跑建構子的 `thread_local` 物件是**惰性初始化**的 —— 標準只保證它在
「該執行緒第一次 odr-use 之前」完成。這段程式裡 `res` 從未被使用，
所以它從未被建構，解構子自然也不會執行。

⚠️ 但這句話有個重要的但書（本檔後半實測驗證）：
「第一次 odr-use 之前」是**期限**，不是**確切時刻**——標準並不禁止實作提早建構。
GCC 的實際做法是**整個翻譯單元共用一個 `__tls_guard`**：只要該執行緒 odr-use 了
這個 .cpp 裡**任何一個**需要動態初始化的 `thread_local`，同一單元裡的**全部**
就會在那一刻一起被建構。所以正確的說法是
「該執行緒沒碰過**這個翻譯單元裡任何一個**這類變數 ⇒ 它們全都不會被建構」。

這一點推翻了上面第 26 行表格裡「執行緒啟動時初始化」的說法：
那只適用於**常數初始化**的變數（如 `thread_local int n = 0;`）。
檔案後半的可執行程式碼會把兩種情況並排跑給你看。

---

### 八、thread_local vs 其他方案

| 方案 | 優點 | 缺點 |
|------|------|------|
| thread_local | 語法簡潔、自動管理 | 無法跨執行緒存取 |
| 傳遞參數 | 明確、可控 | 需要到處傳遞 |
| 執行緒 ID map | 可跨執行緒存取 | 需要同步、較複雜 |

---

### 九、本課重點回顧

1. `thread_local` 讓每個執行緒有獨立的變數副本
2. 不需要同步，因為各執行緒存取自己的副本
3. 變數在執行緒首次存取時初始化，結束時銷毀
4. 常見用途：錯誤碼、快取、隨機數產生器
5. 可用於全域變數、函式內 static、類別 static 成員

---

### 下一課預告

在 **課程 3.6：執行緒安全的初始化** 中，我們將學習：
- `std::call_once` 確保只初始化一次
- `std::once_flag` 的使用
- 單例模式的執行緒安全實作

---

準備好繼續嗎？
*/

// =============================================================================
//  課程 3.5：執行緒本地儲存7.cpp  —  thread_local 的建構/解構時機(本課總整理)
// =============================================================================
//
// 【主題資訊 Information】
//   語法      : thread_local T name;
//   標準版本  : C++11
//   標頭檔    : 不需要 —— thread_local 是語言關鍵字
//   儲存期    : 執行緒儲存期,每執行緒一份
//   建構時機  : 常數初始化 → 執行緒開始執行前即備妥(零成本)
//               動態初始化 → 該執行緒「第一次 odr-use 之前」惰性完成
//   解構時機  : 該執行緒結束時,以與建構相反的順序銷毀
//
// 【詳細解釋 Explanation】
//
// 【1. 本檔要修正一個非常常見的誤解】
// 講義前半的表格寫著「執行緒啟動時初始化」,這句話只對了一半。
// 標準把 thread_local 的初始化分成兩類:
//
//   (a) 常數初始化(constant initialization)
//       thread_local int counter = 0;
//       值在編譯期就決定,執行緒的 TLS 區塊一配置好就是正確的值,
//       沒有任何執行期成本,也沒有「什麼時候跑」的問題。
//
//   (b) 動態初始化(dynamic initialization)
//       thread_local Resource res;        // 要跑建構子
//       標準([basic.stc.thread])只保證「在該執行緒中第一次 odr-use 之前」
//       完成。這是惰性的 —— 不是執行緒一啟動就跑。
//
// 由 (b) 直接推出一個違反直覺、但本檔實測驗證的結論:
//   **一個執行緒若從頭到尾沒有用過某個 thread_local 物件,
//     那個物件在該執行緒中就可能永遠不會被建構,解構子當然也不會執行。**
//
// 這正是講義那段範例的真實行為:worker() 只印字串、完全沒碰 res,
// 所以兩條執行緒都沒有輸出任何「Resource 建構」。
//
// 【1b. 但要小心:「不會被建構」的粒度是翻譯單元,不是單一變數】
// 「第一次 odr-use 之前」是標準給的**期限**,不是**確切時刻** ——
// 標準並不禁止實作提早建構。GCC 的實作方式是:
//   整個翻譯單元(.cpp)共用一個 __tls_guard 與一個 __tls_init 函式。
// 因此只要該執行緒 odr-use 了這個檔案裡「任何一個」需要動態初始化的
// thread_local,同一檔案裡的「全部」就會在那一刻一起被建構。
//
// 本機實測(GCC 15.2.0):
//     thread_local A a;  thread_local B b;
//     只碰 b  →  輸出 "A ctor" 與 "B ctor" 兩行
// 本檔的輸出也印證了同一件事:示範 B 的執行緒只碰了 traced,
// 但 res 與 dbConn 也一起被建構了(所以你會看到 [A] 與 >> 的訊息)。
//
// 所以嚴謹的結論是:
//   「該執行緒沒碰過**這個翻譯單元裡任何一個**需要動態初始化的 thread_local
//     ⇒ 它們全都不會被建構。」
// 想要真正的「用到才建立」,不能依賴這個惰性,要自己用
// std::optional 或指標明確控制(見【實務範例】的說明)。
//
// 【2. 為什麼標準要設計成惰性的】
// 想像一個有 50 個 thread_local 物件的大型程式(log 緩衝、連線、快取…)。
// 若規定「執行緒一啟動就全部建構」,那麼每建立一條執行緒就得跑 50 個建構子,
// 即使那條執行緒只想做一件小事、一個都用不到。惰性初始化讓「沒用到就不付錢」,
// 這和 function-local static 的惰性初始化是同一個設計哲學。
//
// 【3. 解構順序與「主執行緒」的特殊性】
//   * 一般執行緒:結束時銷毀它自己那份,順序與建構相反。
//   * 主執行緒:它那一份的解構發生在 main() 回傳「之後」,
//     和 static 物件的銷毀階段一起進行。所以你會看到解構訊息
//     出現在程式最後一行輸出的後面 —— 本檔的輸出可以直接印證。
//   * detach 的執行緒:若行程結束時它還活著,其 thread_local 解構子
//     不保證會被執行(見課程 2.4 的 detach 討論)。
//
// 【4. 這件事在工程上為什麼重要】
// 很多人用 thread_local + RAII 物件來管理「每執行緒一份的資源」,
// 例如資料庫連線、檔案 handle、記憶體池。如果誤以為「執行緒一啟動就建構」,
// 就會寫出這種程式:
//     thread_local DbConnection conn;   // 以為執行緒一開始就連上了
//     void handler() { /* 忘了用 conn,改用別的路徑 */ }
// 結果連線根本沒建立,而你在監控圖上看到連線數對不上,查半天找不到原因。
// 反過來也一樣:你以為某條執行緒會釋放資源,但它從沒碰過那個物件,
// 解構子自然不會執行,清理邏輯就靜默地被跳過了。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 編譯器如何實作惰性初始化
//   對需要動態初始化的 thread_local,GCC 會產生一個守衛旗標與初始化函式。
//   本機實測(GCC 15.2.0, x86-64, -O2)反組譯可見:
//       cmpb  $0, %fs:__tls_guard@tpoff     ← 每次存取前先問「建構了嗎?」
//   並額外產生一個 __tls_init 函式負責真正的建構。
//   關鍵在於:__tls_guard 與 __tls_init 是「整個翻譯單元一組」,不是每個變數一組。
//   本機用 nm/組譯輸出檢查含兩個 thread_local 的檔案,確實只有單一個
//   __tls_guard 與單一個 __tls_init —— 這就是「碰一個等於建構全部」的原因。
//   對照之下,常數初始化的 thread_local int 只有單純一道
//       movl  %fs:a@tpoff, %eax
//   ——沒有守衛、沒有函式呼叫。這就是 (a)(b) 兩類在機器碼層級的差別。
//
// (B) 守衛旗標本身也放在 TLS 裡
//   注意上面那道指令也是 %fs: 開頭。必須如此:每條執行緒各有一份物件,
//   也就各自需要一個「我這份建構了沒」的狀態。若共用一個旗標,
//   第二條執行緒會誤以為自己那份已經建好了,直接使用未建構的記憶體。
//
// (C) 解構是怎麼被觸發的
//   底層對應 POSIX 的 pthread_key_create 解構器機制:有動態初始化的
//   thread_local 物件在建構完成時,會把自己登記到該執行緒的清理串列,
//   執行緒結束時 runtime 逆序走訪這條串列呼叫解構子。
//   沒被建構的物件從來沒登記過,自然也不會被清理 —— 這與【1】的結論一致。
//
// 【注意事項 Pay Attention】
// 1. 「執行緒啟動時初始化」只適用於常數初始化;需要建構子的是惰性的。
// 2. 沒被用到 ⇒ 沒被建構 ⇒ 沒被解構。別把 thread_local 當成
//    「執行緒啟動掛鉤」或「執行緒結束掛鉤」來用。
// 3. 主執行緒的 thread_local 解構發生在 main() 回傳之後。
// 4. detach 的執行緒在行程結束時仍存活的話,解構子不保證執行。
// 5. thread_local 解構子裡再去存取另一個 thread_local 物件很危險 ——
//    對方可能已經被銷毀,那是未定義行為。
// 6. std::cout 並行輸出不會有 data race,但不保證整行不被切開;
//    本檔用 mutex 保護輸出。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】thread_local 的建構與解構時機
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. thread_local 物件是在執行緒啟動時建構的嗎?
//     答：要分兩種。常數初始化(thread_local int n = 0;)在執行緒開始執行前
//         就已備妥,零成本;需要跑建構子的動態初始化則是惰性的,標準只保證
//         「該執行緒第一次 odr-use 之前」完成。
//     追問：那如果某條執行緒完全沒用到它呢?
//         → 那它就可能永遠不會被建構,解構子也不會執行。本檔實測:worker()
//           沒碰 thread_local Resource,兩條執行緒一次「建構」訊息都沒印。
//           但要注意粒度:GCC 是整個翻譯單元共用一個 __tls_guard,
//           碰了同檔案裡任何一個,全部就會一起建構(本檔示範 B 實測驗證)。
//
// 🔥 Q2. 主執行緒的 thread_local 物件什麼時候解構?
//     答：在 main() 回傳之後,和 static 物件的銷毀階段一起進行。
//         所以它的解構訊息會出現在程式「最後一行輸出」的後面。
//     追問：detach 出去的執行緒呢?
//         → 若行程結束時它還在跑,標準不保證它的 thread_local 解構子會被執行。
//           需要確定清理就不要用 detach,或改用 jthread/明確的關閉協定。
//
// ⚠️ 陷阱. 「我用 thread_local 放一個 RAII 的資料庫連線,這樣每條執行緒
//         啟動時就會自動連線、結束時自動關閉,完全免管理。」哪裡錯了?
//     答：兩頭都錯。連線不是在執行緒啟動時建立的,而是在第一次真正使用
//         那個物件時才建立;若某條執行緒的程式路徑剛好沒走到它,連線
//         從頭到尾不存在,「自動關閉」也就無從發生。
//         把 thread_local 當成執行緒的建構/解構掛鉤,是設計上的誤用。
//     為什麼會錯：把 thread_local 的生命週期想成「和執行緒一樣長」,
//         但正確的說法是「最長不超過執行緒,實際起點取決於第一次使用」——
//         而且那個「第一次使用」還可能是同一個 .cpp 裡「別的」thread_local
//         被碰到所連帶觸發的(GCC 的翻譯單元共用守衛)。
//         需要「執行緒啟動就做某事」,應該把它明確寫在執行緒進入點的第一行;
//         需要「真的用到才建立」,則應該用 std::optional 或指標自己控制。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <mutex>
#include <string>
#include <thread>

// 保護 std::cout,避免多執行緒輸出互相切開
std::mutex g_ioMutex;

void say(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_ioMutex);
    std::cout << msg << std::endl;
}

// -----------------------------------------------------------------------------
// 示範 A:講義原本的程式 —— worker 沒有碰 res,所以 res 從未被建構
// -----------------------------------------------------------------------------
class Resource {
public:
    Resource()  { say("  [A] Resource 建構"); }
    ~Resource() { say("  [A] Resource 銷毀"); }
    void use()  { say("  [A] Resource 被使用"); }
};

thread_local Resource res;   // 每個執行緒有自己的 Resource ——「如果它真的被用到」

void workerNotUsing(int id) {
    // 注意:這裡完全沒有提到 res
    say("  [A] Thread " + std::to_string(id) + " 使用資源(其實沒碰 res)");
}

// -----------------------------------------------------------------------------
// 示範 B:同樣的宣告,但 worker 真的碰了它 → 建構與解構都會發生
// -----------------------------------------------------------------------------
class TracedResource {
    std::string owner_;
public:
    TracedResource() : owner_("(未指定)") { say("  [B] TracedResource 建構"); }
    ~TracedResource() { say("  [B] TracedResource 銷毀 owner=" + owner_); }
    void setOwner(const std::string& o) { owner_ = o; }
};

thread_local TracedResource traced;

void workerUsing(int id) {
    traced.setOwner("Thread " + std::to_string(id));   // ← 第一次 odr-use,就在這裡建構
    say("  [B] Thread " + std::to_string(id) + " 真的使用了資源");
}

// -----------------------------------------------------------------------------
// 示範 C:常數初始化的 thread_local 沒有「建構時機」問題
// -----------------------------------------------------------------------------
thread_local int counter = 0;    // 常數初始化:TLS 一配置好就是 0

void countWorker(int id, int times) {
    for (int i = 0; i < times; ++i) ++counter;
    say("  [C] Thread " + std::to_string(id) + " 的 counter = " + std::to_string(counter)
        + "(每條執行緒從 0 開始,互不干擾)");
}

// -----------------------------------------------------------------------------
// 【日常實務範例】每執行緒一條資料庫連線 —— 並示範它的真正建立時機
//   情境: 連線池常見的做法是「每條 worker thread 持有一條長連線」,
//         用 thread_local + RAII 管理。這裡刻意讓其中一條執行緒走
//         「快取命中」的捷徑、完全不查資料庫,來揭露一個真實世界的坑:
//         那條執行緒的連線根本不會被建立,監控上的連線數會和執行緒數對不上。
//   為什麼用本主題: 直接示範【注意事項】2 在工程上的後果。
//   ⚠️ 但請對照本檔實測輸出:Thread 2 之所以真的沒建立連線,是因為它
//      連同檔案裡其他 thread_local 也一個都沒碰。反之示範 B 的執行緒
//      只碰了 traced,連線卻也被一起建立了 —— 這正是 GCC 翻譯單元共用
//      守衛的效果。所以「不用就不建立」在真實程式裡並不可靠,
//      要精準控制資源建立時機,請改用 std::optional<DbConnection> 明確管理。
// -----------------------------------------------------------------------------
class DbConnection {
    std::string name_;
public:
    explicit DbConnection() : name_("conn") { say("    >> 建立資料庫連線"); }
    ~DbConnection() { say("    >> 關閉資料庫連線"); }
    std::string query(const std::string& sql) { return "結果(" + sql + ")"; }
};

thread_local DbConnection dbConn;

std::string fetchUser(int id, bool cacheHit) {
    if (cacheHit) {
        // 走捷徑:完全沒有碰 dbConn → 這條執行緒永遠不會建立連線
        return "來自快取的 user " + std::to_string(id);
    }
    return dbConn.query("SELECT * FROM users WHERE id=" + std::to_string(id));
}

void dbWorker(int id, bool cacheHit) {
    say("  [D] Thread " + std::to_string(id) + (cacheHit ? " (快取命中)" : " (需要查庫)"));
    std::string r = fetchUser(id, cacheHit);
    say("  [D] Thread " + std::to_string(id) + " 取得: " + r);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔從缺,理由如下
//   LeetCode 的並行題(1114 Print in Order、1115 Print FooBar Alternately、
//   1116 Print Zero Even Odd、1117 Building H2O、1195 Fizz Buzz Multithreaded)
//   考的是執行緒之間的同步與順序控制,必須靠共享狀態達成;
//   而 thread_local 的本質是「讓執行緒彼此看不見對方的資料」,
//   用進那些題目會直接讓答案錯誤。本課主題(儲存期與建構時機)
//   在 LeetCode 上沒有對應題型,硬湊一題不如誠實從缺,
//   改以上面的資料庫連線情境示範真實工程後果。
// -----------------------------------------------------------------------------

int main() {
    std::cout << "=== 主執行緒 ===" << std::endl;

    std::cout << "\n--- A:宣告了 thread_local Resource,但 worker 沒碰它 ---" << std::endl;
    std::thread a1(workerNotUsing, 1);
    std::thread a2(workerNotUsing, 2);
    a1.join();
    a2.join();
    std::cout << "  ↑ 一次「Resource 建構」都沒有 —— 沒被 odr-use 就不會建構" << std::endl;

    std::cout << "\n--- B:同樣宣告,但 worker 真的使用了它 ---" << std::endl;
    std::thread b1(workerUsing, 1);
    std::thread b2(workerUsing, 2);
    b1.join();
    b2.join();
    std::cout << "  ↑ 建構與解構都在「該執行緒自己」身上發生,解構在執行緒結束時" << std::endl;

    std::cout << "\n--- C:常數初始化的 thread_local(沒有惰性問題) ---" << std::endl;
    std::thread c1(countWorker, 1, 3);
    std::thread c2(countWorker, 2, 5);
    c1.join();
    c2.join();

    std::cout << "\n--- D:實務 —— 每執行緒一條資料庫連線 ---" << std::endl;
    std::thread d1(dbWorker, 1, false);   // 需要查庫 → 連線會被建立
    d1.join();
    std::thread d2(dbWorker, 2, true);    // 快取命中 → 連線永遠不會被建立
    d2.join();
    std::cout << "  ↑ Thread 2 從頭到尾沒有「建立資料庫連線」,"
                 "因為它沒走到用 dbConn 的路徑" << std::endl;

    std::cout << "\n=== 結束 ===" << std::endl;
    std::cout << "(主執行緒自己那份 thread_local 的解構會發生在 main 回傳「之後」,"
                 "所以下面還會有輸出)" << std::endl;
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.5：執行緒本地儲存7.cpp" -o tls_lifetime

// 注意:以下為某一次實際執行的結果。
//   * 同一組內兩條執行緒的先後順序每次執行都可能不同
//     (例如 [B] Thread 2 可能整組排在 Thread 1 之前)。
//   * 但每一段的「內容」是確定的,尤其是
//     ——[A] 完全沒有建構/解構訊息;
//     ——[D] Thread 2 沒有任何連線訊息;
//     ——主執行緒的解構訊息出現在「=== 結束 ===」之後。

// === 預期輸出 ===
// === 主執行緒 ===
//
// --- A:宣告了 thread_local Resource,但 worker 沒碰它 ---
//   [A] Thread 2 使用資源(其實沒碰 res)
//   [A] Thread 1 使用資源(其實沒碰 res)
//   ↑ 一次「Resource 建構」都沒有 —— 沒被 odr-use 就不會建構
//
// --- B:同樣宣告,但 worker 真的使用了它 ---
//   [A] Resource 建構
//   [B] TracedResource 建構
//     >> 建立資料庫連線
//   [B] Thread 2 真的使用了資源
//     >> 關閉資料庫連線
//   [B] TracedResource 銷毀 owner=Thread 2
//   [A] Resource 銷毀
//   [A] Resource 建構
//   [B] TracedResource 建構
//     >> 建立資料庫連線
//   [B] Thread 1 真的使用了資源
//     >> 關閉資料庫連線
//   [B] TracedResource 銷毀 owner=Thread 1
//   [A] Resource 銷毀
//   ↑ 建構與解構都在「該執行緒自己」身上發生,解構在執行緒結束時
//
// --- C:常數初始化的 thread_local(沒有惰性問題) ---
//   [C] Thread 1 的 counter = 3(每條執行緒從 0 開始,互不干擾)
//   [C] Thread 2 的 counter = 5(每條執行緒從 0 開始,互不干擾)
//
// --- D:實務 —— 每執行緒一條資料庫連線 ---
//   [D] Thread 1 (需要查庫)
//   [A] Resource 建構
//   [B] TracedResource 建構
//     >> 建立資料庫連線
//   [D] Thread 1 取得: 結果(SELECT * FROM users WHERE id=1)
//     >> 關閉資料庫連線
//   [B] TracedResource 銷毀 owner=(未指定)
//   [A] Resource 銷毀
//   [D] Thread 2 (快取命中)
//   [D] Thread 2 取得: 來自快取的 user 2
//   ↑ Thread 2 從頭到尾沒有「建立資料庫連線」,因為它沒走到用 dbConn 的路徑
//
// === 結束 ===
// (主執行緒自己那份 thread_local 的解構會發生在 main 回傳「之後」,所以下面還會有輸出)
