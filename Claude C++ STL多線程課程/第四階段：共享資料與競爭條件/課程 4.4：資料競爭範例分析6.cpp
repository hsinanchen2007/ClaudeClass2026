// =============================================================================
//  課程 4.4：資料競爭範例分析6.cpp  —  部分更新：讀到「從未存在過」的組合
// =============================================================================
//
// 【本檔是「刻意示範錯誤」的範例，不要照抄到實際專案】
//   writer 逐欄位改寫 person，reader 同時逐欄位讀取，兩者沒有任何同步
//   → genuine data race → 【未定義行為 (UB)】。
//   本檔【不會】宣稱「一定會印出 Jane Doe 30」——UB 沒有固定結果。
//
// 【檔案結構】上半部是課程 4.4 的完整講義（包在 /* ... */ 裡），
//   下半部是可執行的錯誤示範。本段是為整份檔案補上的教科書導讀。
//
// 【主題資訊 Information】
//   主題：    部分更新 (partial update)；五大競爭模式之五
//   語法：    person.firstName = "Jane";   // ← 三個欄位分開賦值
//             person.lastName  = "Smith";  //   中間任何一刻都可能被觀察
//             person.age       = 25;
//   標準版本：std::thread 為 C++11
//   標頭檔：  <thread>、<string>、<mutex>
//   偵測工具：g++ -fsanitize=thread -g -pthread
//
// 【詳細解釋 Explanation】
//
// 【1. 這個錯誤的特別之處：讀到的是「從未存在過」的狀態】
//   原始資料是 {"John", "Doe", 30}，寫入端要改成 {"Jane", "Smith", 25}。
//   這兩個都是合法的人。但因為三個欄位分開賦值，
//   reader 可能讀到 {"Jane", "Doe", 30} —— 一個【從來沒有存在過】的人。
//   前面幾種競爭模式的後果多半是「數值算錯」或「用到舊資料」，
//   這裡的後果是【邏輯上根本不可能的狀態】。
//   在真實系統裡，這種資料往往會通過所有的驗證檢查
//   （每個欄位單獨看都合法），然後一路傳到下游造成難以追查的錯誤。
//
// 【2. 為什麼 std::string 讓問題更嚴重】
//   `person.firstName = "Jane"` 不是一次原子的指標交換，而是：
//     ① 可能配置新的堆積記憶體（若新字串比容量長）
//     ② 複製字元內容
//     ③ 更新長度欄位
//     ④ 可能釋放舊的緩衝區
//   （本機 libstdc++ 的 std::string 有 SSO —— 短字串最佳化：
//     長度 ≤ 15 bytes 直接存在物件內部，不配置堆積。
//     "Jane"/"Smith" 都在此範圍內，所以本例不會配置記憶體；
//     但這是【實作定義】的門檻，MSVC 是 15、libc++ 是 22。）
//   即使有 SSO 不配置記憶體，欄位仍是逐位元組複製的 ——
//   reader 完全可能讀到「寫到一半」的字串內容（撕裂的字串），
//   那不只是「舊值或新值」，而是任何位元組組合，
//   甚至可能是長度欄位與內容不一致的損毀狀態。
//
// 【3. 為什麼 atomic 在這裡幾乎無解】
//   有人會想「那把每個欄位都變成 atomic」。這行不通，原因有二：
//     ① std::atomic<std::string> 不是 lock-free（字串太大），
//        實作內部仍然用鎖，等於繞了一圈回到 mutex。
//     ② 就算三個欄位各自原子，reader 仍是三次獨立的讀取，
//        中間 writer 可以整個跑完 —— 混搭問題原封不動
//        （這與課程 4.2-3 的結論完全一致：
//        原子化每個零件，不會讓組裝過程變原子）。
//   → 要讓「三個欄位」成為一個不可分割的觀察單位，只有兩條路：
//     用鎖，或把整個物件當成一個不可變的值來替換（見【4】）。
//
// 【4. 兩種正確作法】
//   (a) 用鎖保護整組欄位
//         寫入：{ lock; firstName=...; lastName=...; age=...; }
//         讀取：{ lock; return Person{firstName, lastName, age}; }
//       注意讀取端要回傳【複本】而非引用，否則鎖一釋放就沒有保護。
//   (b) 不可變物件 + 指標交換（copy-on-write）
//         寫入：建一個【全新】的 Person，然後一次換掉 shared_ptr。
//         讀取：複製一個 shared_ptr，之後完全無鎖地讀整份。
//       這個做法的優勢是讀取端幾乎零成本，適合讀多寫少；
//       而且因為物件本身永不修改，不可能讀到中間狀態。
//   本檔下方兩者都示範，並用併發測試驗證「混搭次數必定為 0」。
//
// 【5. 這個模式在真實系統中的樣子】
//   * 使用者資料更新（姓名 + 地址 + 電話分開寫）
//   * 訂單狀態機（狀態 + 時間戳 + 操作者分開寫）→
//     可能讀到「已出貨但出貨時間是 null」
//   * 幾何座標（x, y, z 分開寫）→ 物件瞬間出現在不存在的位置
//   * 連線資訊（host + port 分開寫）→ 連到錯誤的機器（見 4.2-2）
//   共同特徵是：每個欄位單獨看都合法，組合起來卻不合法 ——
//   所以【下游的驗證檢查抓不到】。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼結構體的賦值也不是原子的
//   即使寫成 `person = Person{"Jane", "Smith", 25};`（整個結構體一次賦值），
//   編譯器產生的仍然是逐欄位的複製（對含 std::string 的類別是逐成員複製建構）。
//   x86-64 上只有 ≤ 16 bytes 且對齊的資料【有機會】用單一指令搬移，
//   但那也不是標準保證的原子性 —— 沒有 lock 前綴就不是原子操作。
//   → 「一行程式碼」與「一個原子操作」是完全不同的概念。
//
// (B) 什麼樣的型別才可能 lock-free
//   std::atomic<T> 要求 T 是 trivially copyable。
//   即使符合，是否 lock-free 仍取決於大小與平台：
//   本機 x86-64 上 ≤ 8 bytes 通常 lock-free，16 bytes 需要 cmpxchg16b。
//   可以用 std::atomic<T>::is_always_lock_free（C++17）在編譯期查詢。
//   含 std::string 的 Person 根本不是 trivially copyable，
//   連放進 std::atomic 都不合法。
//
// (C) 為什麼「不可變 + 指標交換」是現代並行的主流做法
//   它把「多個欄位的一致性」問題，轉化成「一個指標的原子交換」問題 ——
//   而後者是硬體天生支援的。讀取端不需要鎖、不會被寫入端阻塞，
//   寫入端也不會被讀取端阻塞。代價是每次更新都要配置一份新物件。
//   這個思想貫穿了 RCU（Linux 核心）、Clojure 的 persistent data structure、
//   React 的 immutable state，以及 C++ 的 shared_ptr + copy-on-write。
//
// 【注意事項 Pay Attention】
// 1. 本檔是 UB 示範：不可說「一定印出 Jane Doe 30」或任何固定結果。
// 2. 部分更新的後果是「讀到從未存在過的組合」，而且每個欄位單獨看都合法，
//    所以下游的驗證檢查抓不到。
// 3. 把每個欄位改成 atomic 修不好 —— 混搭問題出在「三次獨立的讀取」之間。
// 4. std::string 的賦值不是原子的；即使有 SSO 不配置記憶體，
//    仍可能讀到撕裂的內容（SSO 門檻 15 bytes 是本機 libstdc++ 的實作定義值）。
// 5. 讀取端要回傳整份【複本】，回傳引用等於把保護漏到鎖外。
// 6. 「不可變物件 + 指標交換」讓讀取端零同步，適合讀多寫少的場景。
//
// 【LeetCode 說明】本檔不附 LeetCode 範例。
//   本檔的主題是「多欄位物件的一致性更新」，屬於資料建模與同步設計；
//   允許使用的設計題（146/155/705/707/1603）操作的都是單一資料結構、
//   且在單執行緒下判題，不會遇到欄位混搭；
//   並行題（1114～1117/1195）考的是執行緒間的順序協調。
//   硬湊只會失焦，故從缺，改以下方兩個真實情境
//   （訂單狀態機、使用者個人資料更新）呈現。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】部分更新與物件一致性
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 一個物件有三個欄位，更新時逐一賦值，多執行緒下會有什麼問題?
//     答：讀取端可能讀到「混搭」的組合 —— 例如新的 firstName
//         配上舊的 lastName，一個從來沒有存在過的狀態。
//         最危險的是每個欄位單獨看都合法，
//         所以下游的驗證檢查完全抓不到，錯誤資料會一路傳下去。
//     追問：把每個欄位都改成 std::atomic 可以嗎?
//         → 不行。① std::atomic<std::string> 不是 lock-free，
//           內部仍用鎖；② 就算各自原子，讀取端仍是三次獨立的讀取，
//           中間寫入端可以整個跑完，混搭問題原封不動。
//
// 🔥 Q2. 除了加鎖，還有什麼方式能保證讀到一致的物件?
//     答：不可變物件 + 指標交換（copy-on-write）。
//         更新時建一份【全新】的物件，然後用一次原子的指標交換發佈；
//         讀取端複製一個 shared_ptr 之後，那份物件在它手上絕不會變，
//         之後的所有存取完全無鎖。
//         這把「多欄位的一致性」轉化成「一個指標的原子交換」——
//         而後者是硬體天生支援的。
//     追問：這個做法的代價是什麼?
//         → 每次更新都要配置一份新物件（寫入成本較高），
//           而且舊版本要等最後一個讀者放手才會回收。
//           所以它適合【讀多寫少】；寫入頻繁時用鎖反而較好。
//
// ⚠️ 陷阱. 「我把整個物件一次賦值 person = Person{...}，這樣就是一個動作了吧?」
//     答：不是。編譯器產生的仍然是逐欄位的複製 ——
//         對含 std::string 的類別是逐成員的複製指派，
//         中間任何一刻都可能被另一條執行緒觀察到。
//         x86-64 上只有 ≤ 16 bytes 且對齊的資料【有機會】用單一指令搬移，
//         但沒有 lock 前綴就不是原子操作，標準也不保證。
//     為什麼會錯：把「一行原始碼」等同於「一個不可分割的動作」。
//         原子性的單位是硬體指令（且需要 lock 前綴或適當的記憶體序），
//         不是原始碼的行數。判斷一個操作是否原子，
//         唯一可靠的方式是看它有沒有透過 std::atomic 或鎖來保證。
// =============================================================================

/*
# 第四階段：共享資料與競爭條件

## 課程 4.4：資料競爭範例分析

---

### 引言

本課透過多個實際案例，分析資料競爭如何發生，以及如何識別程式碼中潛在的競爭條件。

---

### 一、案例一：Check-Then-Act 競爭

最常見的競爭模式：先檢查條件，再根據結果行動。

```cpp
#include <iostream>
#include <thread>
#include <map>

std::map<int, std::string> cache;

// 危險！Check-Then-Act 競爭
std::string getValue(int key) {
    if (cache.find(key) == cache.end()) {  // 檢查
        // ← 另一執行緒可能在此插入相同 key！
        cache[key] = "computed_" + std::to_string(key);  // 行動
    }
    return cache[key];
}

int main() {
    std::thread t1([]() { getValue(1); });
    std::thread t2([]() { getValue(1); });
    
    t1.join();
    t2.join();
    
    return 0;
}
```

**問題**：兩個執行緒都可能認為 key 不存在，然後都嘗試插入。

---

### 二、案例二：Read-Modify-Write 競爭

讀取、修改、寫回的複合操作。

```cpp
#include <iostream>
#include <thread>

int counter = 0;

void increment() {
    // 這三步不是原子的！
    // 1. 讀取 counter
    // 2. +1
    // 3. 寫回
    for (int i = 0; i < 10000; ++i) {
        counter++;  // 非原子操作
    }
}

int main() {
    std::thread t1(increment);
    std::thread t2(increment);
    
    t1.join();
    t2.join();
    
    // 預期 20000，實際可能更小
    std::cout << "counter = " << counter << std::endl;
    
    return 0;
}
```

---

### 三、案例三：複合操作競爭

看似獨立的操作，實際上有關聯。

```cpp
#include <iostream>
#include <thread>
#include <vector>

std::vector<int> vec;

void unsafeAppend(int value) {
    if (vec.size() < 10) {        // 檢查
        // ← 另一執行緒可能在此改變 size！
        vec.push_back(value);      // 可能超過限制
    }
}

void unsafeAccess() {
    if (!vec.empty()) {            // 檢查
        // ← 另一執行緒可能清空 vec！
        int last = vec.back();     // 可能崩潰
        std::cout << last << std::endl;
    }
}
```

---

### 四、案例四：迭代器失效

```cpp
#include <iostream>
#include <thread>
#include <vector>

std::vector<int> data = {1, 2, 3, 4, 5};

void reader() {
    for (auto it = data.begin(); it != data.end(); ++it) {
        // ← 另一執行緒可能修改 data，導致迭代器失效！
        std::cout << *it << " ";
    }
}

void writer() {
    data.push_back(6);  // 可能導致重新配置，所有迭代器失效
}

int main() {
    std::thread t1(reader);
    std::thread t2(writer);
    
    t1.join();
    t2.join();
    
    return 0;
}
```

---

### 五、案例五：延遲初始化

```cpp
#include <iostream>
#include <thread>

class Singleton {
    static Singleton* instance;
    
public:
    // 危險！雙重檢查鎖定的錯誤實作
    static Singleton* getInstance() {
        if (instance == nullptr) {          // 第一次檢查
            // ← 多執行緒可能同時通過這裡！
            instance = new Singleton();     // 可能建立多個實例
        }
        return instance;
    }
};

Singleton* Singleton::instance = nullptr;
```

---

### 六、案例六：共享物件的部分更新

```cpp
#include <iostream>
#include <thread>
#include <string>

struct Person {
    std::string firstName;
    std::string lastName;
    int age;
};

Person person{"John", "Doe", 30};

void writer() {
    // 更新不是原子的
    person.firstName = "Jane";
    // ← 此刻資料不一致！
    person.lastName = "Smith";
    person.age = 25;
}

void reader() {
    // 可能讀到 "Jane Doe 30" 這種不一致狀態
    std::cout << person.firstName << " " 
              << person.lastName << " " 
              << person.age << std::endl;
}
```

---

### 七、競爭條件識別清單

```
┌─────────────────────────────────────────────────────────────┐
│                 競爭條件警示信號                             │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ⚠️ if (condition) { action }                              │
│     條件和行動之間狀態可能改變                               │
│                                                             │
│  ⚠️ variable++, variable += x                              │
│     Read-Modify-Write 不是原子的                            │
│                                                             │
│  ⚠️ 讀取多個相關變數                                        │
│     可能讀到不一致的組合                                     │
│                                                             │
│  ⚠️ 迭代容器時修改容器                                      │
│     迭代器可能失效                                          │
│                                                             │
│  ⚠️ 物件狀態的部分更新                                      │
│     可能讀到中間狀態                                        │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 八、如何發現競爭條件

#### 方法一：程式碼審查

問自己：
- 這個變數是否被多執行緒存取？
- 有沒有寫入操作？
- 操作是否原子？

#### 方法二：使用工具

```bash
# ThreadSanitizer（你已熟悉）
g++ -fsanitize=thread -g -o program program.cpp
./program
```

#### 方法三：壓力測試

```cpp
// 大量重複執行，增加發現問題的機會
for (int i = 0; i < 10000; ++i) {
    runTest();
}
```

---

### 九、ThreadSanitizer 輸出範例

```cpp
// 編譯：g++ -fsanitize=thread -g -o race race.cpp -pthread

#include <thread>

int counter = 0;

int main() {
    std::thread t1([]() { counter++; });
    std::thread t2([]() { counter++; });
    
    t1.join();
    t2.join();
    
    return 0;
}
```

TSan 輸出：
```
WARNING: ThreadSanitizer: data race (pid=12345)
  Write of size 4 at 0x000000601040 by thread T2:
    #0 main::$_1::operator()() race.cpp:8
    
  Previous write of size 4 at 0x000000601040 by thread T1:
    #0 main::$_0::operator()() race.cpp:7
    
  Location is global 'counter' of size 4 at 0x000000601040
```

---

### 十、競爭條件的特性

```
┌─────────────────────────────────────────────────────────────┐
│               競爭條件為何難以除錯                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. 非確定性                                                 │
│     同樣的程式碼，每次執行結果可能不同                       │
│                                                             │
│  2. 時機敏感                                                 │
│     只在特定時序下發生                                       │
│                                                             │
│  3. 難以重現                                                 │
│     加上 printf 除錯可能改變時序，問題消失                   │
│                                                             │
│  4. 環境依賴                                                 │
│     在某台機器正常，換台機器就出錯                           │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 十一、本課重點回顧

1. **Check-Then-Act**：檢查和行動之間狀態可能改變
2. **Read-Modify-Write**：複合操作不是原子的
3. **複合操作**：多個相關操作必須一起保護
4. **迭代器失效**：修改容器時迭代器可能無效
5. **部分更新**：物件可能處於不一致狀態
6. **TSan** 是發現競爭條件的有力工具

---

### 下一課預告

在 **課程 4.5：競爭條件的檢測** 中，我們將深入學習：
- ThreadSanitizer 的詳細使用
- 靜態分析工具
- 除錯策略與技巧

---

準備好繼續嗎？
*/



#include <iostream>
#include <thread>
#include <string>
#include <mutex>
#include <memory>
#include <atomic>
#include <vector>

struct Person {
    std::string firstName;
    std::string lastName;
    int age;
};

Person person{"John", "Doe", 30};

void writer() {
    // 更新不是原子的
    person.firstName = "Jane";
    // ← 此刻資料不一致！
    person.lastName = "Smith";
    person.age = 25;
}

void reader() {
    // 可能讀到 "Jane Doe 30" 這種不一致狀態
    std::cout << person.firstName << " " 
              << person.lastName << " " 
              << person.age << std::endl;
}

// -----------------------------------------------------------------------------
// 【正確作法 a】用鎖保護整組欄位
// -----------------------------------------------------------------------------
struct PersonData {
    std::string firstName;
    std::string lastName;
    int age;
};

class LockedPerson {
private:
    mutable std::mutex mtx;
    PersonData data;

public:
    LockedPerson() : data{"John", "Doe", 30} {}

    // 三個欄位一起改，破壞期整段在鎖內
    void update(const std::string& first, const std::string& last, int age) {
        std::lock_guard<std::mutex> lock(mtx);
        data.firstName = first;
        data.lastName = last;
        data.age = age;
    }

    // 回傳整份【複本】—— 回傳引用的話鎖一釋放就沒有保護
    PersonData snapshot() const {
        std::lock_guard<std::mutex> lock(mtx);
        return data;
    }
};

// -----------------------------------------------------------------------------
// 【正確作法 b】不可變物件 + 指標交換（copy-on-write）
//   物件本身永不修改；更新是「換掉整份」。讀取端幾乎零成本。
// -----------------------------------------------------------------------------
class ImmutablePerson {
private:
    mutable std::mutex mtx;
    std::shared_ptr<const PersonData> current;

public:
    ImmutablePerson() {
        current = std::make_shared<const PersonData>(PersonData{"John", "Doe", 30});
    }

    void update(const std::string& first, const std::string& last, int age) {
        auto next = std::make_shared<const PersonData>(PersonData{first, last, age});
        std::lock_guard<std::mutex> lock(mtx);
        current = next;              // 一次指標交換，不可能有中間狀態
    }

    // 讀取端只在複製 shared_ptr 的瞬間持鎖，之後完全無鎖
    std::shared_ptr<const PersonData> get() const {
        std::lock_guard<std::mutex> lock(mtx);
        return current;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 1】訂單狀態機：狀態、時間戳、操作者必須一起更新
//   情境：訂單出貨時要同時寫入「狀態=已出貨」「出貨時間」「出貨人員」。
//         若分開寫，查詢端可能讀到「狀態已出貨、但出貨時間還是 0」——
//         一個不可能存在的訂單。這種資料通過所有欄位層級的驗證，
//         然後在下游的報表、對帳、客服系統裡引發一連串莫名其妙的問題。
//   正解：整組欄位一次更新、一次讀取（本例用鎖）。
// -----------------------------------------------------------------------------
struct OrderState {
    int status;            // 0=待處理, 1=已付款, 2=已出貨
    long shippedAtMs;      // 出貨時間；status<2 時必須為 0
    std::string operatorId;

    // 不變量：status==2 ⟺ shippedAtMs>0 且 operatorId 非空
    bool consistent() const {
        if (status == 2) return shippedAtMs > 0 && !operatorId.empty();
        return shippedAtMs == 0 && operatorId.empty();
    }
};

class Order {
private:
    mutable std::mutex mtx;
    OrderState state;

public:
    Order() : state{0, 0, ""} {}

    void markShipped(long timestampMs, const std::string& op) {
        std::lock_guard<std::mutex> lock(mtx);
        state.status = 2;                 // ← 破壞期開始
        state.shippedAtMs = timestampMs;
        state.operatorId = op;            // ← 破壞期結束
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mtx);
        state.status = 0;
        state.shippedAtMs = 0;
        state.operatorId.clear();
    }

    OrderState snapshot() const {
        std::lock_guard<std::mutex> lock(mtx);
        return state;                     // 整份複本，必定一致
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】使用者個人資料：用不可變快照供大量讀取
//   情境：個人資料被每個請求讀取（頭像、暱稱、權限等級），
//         但一天只更新幾次。用鎖的話，每個請求都要排隊。
//   正解：copy-on-write —— 讀取端拿到 shared_ptr<const Profile> 之後
//         完全無鎖，寫入端建新物件並交換指標。
//         讀多寫少的場景中，這個模式的吞吐量遠勝於加鎖。
// -----------------------------------------------------------------------------
struct Profile {
    std::string nickname;
    std::string avatarUrl;
    int level;

    // 不變量：暱稱與頭像必須來自同一次更新（本例用命名慣例驗證）
    bool consistent() const {
        return nickname.size() == avatarUrl.size()
            && level == static_cast<int>(nickname.size());
    }
};

class ProfileStore {
private:
    mutable std::mutex mtx;
    std::shared_ptr<const Profile> current;

public:
    ProfileStore() {
        current = std::make_shared<const Profile>(Profile{"aaa", "111", 3});
    }

    void publish(const std::string& nick, const std::string& avatar, int level) {
        auto next = std::make_shared<const Profile>(Profile{nick, avatar, level});
        std::lock_guard<std::mutex> lock(mtx);
        current = next;
    }

    std::shared_ptr<const Profile> get() const {
        std::lock_guard<std::mutex> lock(mtx);
        return current;
    }
};

int main() {
    std::cout << "=== 錯誤示範：逐欄位更新（data race / UB）===\n";
    std::thread t1(writer);
    std::thread t2(reader);
    
    t1.join();
    t2.join();
    std::cout << "（上面那一行的三個欄位每次執行都可能不同，且不受標準保證；\n";
    std::cout << "  可能讀到 Jane Doe 30 這種從未存在過的組合）\n";

    std::cout << "\n=== 正確作法 a：用鎖保護整組欄位 ===\n";
    {
        LockedPerson lp;
        std::atomic<int> mixed{0};
        std::atomic<bool> stop{false};

        std::thread w([&lp, &stop] {
            for (int i = 0; i < 20000; ++i) {
                if (i % 2 == 0) lp.update("Jane", "Smith", 25);
                else            lp.update("John", "Doe", 30);
            }
            stop.store(true);
        });
        std::thread r([&lp, &mixed, &stop] {
            while (!stop.load()) {
                PersonData p = lp.snapshot();
                bool ok = (p.firstName == "Jane" && p.lastName == "Smith" && p.age == 25)
                       || (p.firstName == "John" && p.lastName == "Doe"   && p.age == 30);
                if (!ok) mixed.fetch_add(1);
            }
        });
        w.join();
        r.join();

        std::cout << "寫入 20000 次，讀取端持續取快照\n";
        std::cout << "讀到欄位混搭的次數: " << mixed.load() << "（必定為 0）\n";
    }

    std::cout << "\n=== 正確作法 b：不可變物件 + 指標交換 ===\n";
    {
        ImmutablePerson ip;
        std::atomic<int> mixed{0};
        std::atomic<bool> stop{false};

        std::thread w([&ip, &stop] {
            for (int i = 0; i < 20000; ++i) {
                if (i % 2 == 0) ip.update("Jane", "Smith", 25);
                else            ip.update("John", "Doe", 30);
            }
            stop.store(true);
        });
        std::thread r([&ip, &mixed, &stop] {
            while (!stop.load()) {
                auto p = ip.get();          // 之後完全無鎖地讀整份
                bool ok = (p->firstName == "Jane" && p->lastName == "Smith" && p->age == 25)
                       || (p->firstName == "John" && p->lastName == "Doe"   && p->age == 30);
                if (!ok) mixed.fetch_add(1);
            }
        });
        w.join();
        r.join();

        std::cout << "寫入 20000 次，讀取端持續取快照\n";
        std::cout << "讀到欄位混搭的次數: " << mixed.load() << "（必定為 0）\n";
        std::cout << "→ 物件本身永不修改，讀者手上那份絕不會變\n";
    }

    std::cout << "\n=== 日常實務 1：訂單狀態機的一致性 ===\n";
    {
        Order order;
        std::atomic<int> bad{0};
        std::atomic<bool> stop{false};

        std::thread w([&order, &stop] {
            for (int i = 0; i < 20000; ++i) {
                order.markShipped(1700000000000L + i, "staff-007");
                order.reset();
            }
            stop.store(true);
        });
        std::thread q([&order, &bad, &stop] {
            while (!stop.load()) {
                OrderState s = order.snapshot();
                if (!s.consistent()) bad.fetch_add(1);
            }
        });
        w.join();
        q.join();

        std::cout << "出貨/重設各 20000 次，查詢端持續檢查不變量\n";
        std::cout << "讀到「已出貨但無出貨時間」的次數: " << bad.load()
                  << "（必定為 0）\n";
        std::cout << "→ 若分開寫入，這種訂單會通過所有欄位層級的驗證，\n";
        std::cout << "  然後在報表與對帳系統裡引發莫名其妙的問題。\n";
    }

    std::cout << "\n=== 日常實務 2：個人資料的無鎖讀取（讀多寫少）===\n";
    {
        ProfileStore store;
        std::atomic<int> bad{0};
        std::atomic<long> reads{0};
        std::atomic<bool> stop{false};
        std::vector<std::thread> readers;

        for (int i = 0; i < 4; ++i) {
            readers.emplace_back([&store, &bad, &reads, &stop] {
                long n = 0;
                while (!stop.load(std::memory_order_relaxed)) {
                    auto p = store.get();
                    if (!p->consistent()) bad.fetch_add(1);
                    ++n;
                }
                reads.fetch_add(n);
            });
        }
        std::thread w([&store, &stop] {
            for (int i = 1; i <= 2000; ++i) {
                std::string s(static_cast<size_t>(i % 8 + 1), 'a');
                std::string a(static_cast<size_t>(i % 8 + 1), '1');
                store.publish(s, a, static_cast<int>(s.size()));
            }
            stop.store(true);
        });
        w.join();
        for (auto& t : readers) t.join();

        std::cout << "寫入 2000 次，4 條讀取端持續讀取\n";
        std::cout << "讀到不一致資料的次數: " << bad.load() << "（必定為 0）\n";
        std::cout << "→ 讀取端只在複製 shared_ptr 時持鎖，之後零同步；\n";
        std::cout << "  總讀取次數每次執行都不同（取決於 CPU 速度與排程）。\n";
    }
    
    return 0;
}
// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 4.4：資料競爭範例分析6.cpp' -o race6
//
// 偵測資料競爭（第一段是 UB，唯一可靠的判定方式）:
//   g++ -std=c++17 -fsanitize=thread -g -pthread '課程 4.4：資料競爭範例分析6.cpp' -o race6_tsan

// ⚠️ 只有第一段的那一行【每次執行都可能不同】——該段是 genuine data race → UB。
// 本機（Ubuntu 26.04 / g++ 15.2.0 / 16 核心）多次實測看過
// 「Jane Smith 25」（已完成）與「John Doe 30」（尚未開始）；
// 理論上也可能出現「Jane Doe 30」這種從未存在過的混搭組合，
// 或撕裂的字串內容 —— 標準不保證任何結果。
// 下面貼的是其中一次的真實實測。
//
// 其餘各段（鎖保護、不可變物件、訂單狀態機、個人資料）的
// 「次數: 0」都是確定值：那是不變量的驗證，每次執行都必定為 0。

// === 預期輸出 ===
// === 錯誤示範：逐欄位更新（data race / UB）===
// Jane Smith 25
// （上面那一行的三個欄位每次執行都可能不同，且不受標準保證；
//   可能讀到 Jane Doe 30 這種從未存在過的組合）
//
// === 正確作法 a：用鎖保護整組欄位 ===
// 寫入 20000 次，讀取端持續取快照
// 讀到欄位混搭的次數: 0（必定為 0）
//
// === 正確作法 b：不可變物件 + 指標交換 ===
// 寫入 20000 次，讀取端持續取快照
// 讀到欄位混搭的次數: 0（必定為 0）
// → 物件本身永不修改，讀者手上那份絕不會變
//
// === 日常實務 1：訂單狀態機的一致性 ===
// 出貨/重設各 20000 次，查詢端持續檢查不變量
// 讀到「已出貨但無出貨時間」的次數: 0（必定為 0）
// → 若分開寫入，這種訂單會通過所有欄位層級的驗證，
//   然後在報表與對帳系統裡引發莫名其妙的問題。
//
// === 日常實務 2：個人資料的無鎖讀取（讀多寫少）===
// 寫入 2000 次，4 條讀取端持續讀取
// 讀到不一致資料的次數: 0（必定為 0）
// → 讀取端只在複製 shared_ptr 時持鎖，之後零同步；
//   總讀取次數每次執行都不同（取決於 CPU 速度與排程）。
