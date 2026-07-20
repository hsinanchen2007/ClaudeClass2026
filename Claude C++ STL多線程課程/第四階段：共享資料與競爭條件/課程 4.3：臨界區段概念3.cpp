// =============================================================================
//  課程 4.3：臨界區段概念3.cpp  —  臨界區段該有多大：badPush vs goodPush
// =============================================================================
//
// 【檔案結構】上半部是課程 4.3 的完整講義（包在 /* ... */ 裡），
//   下半部是可執行的對照程式。本段是為整份檔案補上的教科書導讀。
//
// 【主題資訊 Information】
//   主題：    臨界區段最小化原則的入門實例
//   對照：    badPush（計算與輸出都在鎖內） vs goodPush（只鎖 push_back）
//   標準版本：std::thread / std::mutex / lock_guard 為 C++11
//   標頭檔：  <mutex>、<vector>、<thread>
//   延伸：    量化的效能比較見課程 5.6-4；避免鎖內 I/O 見 5.6-5
//
// 【詳細解釋 Explanation】
//
// 【1. 逐行判定：哪一行真的需要鎖】
//   void badPush(int value) {
//       std::lock_guard<std::mutex> lock(mtx);
//       int processed = value * value;        // ✗ 不需要鎖：純區域運算
//       data.push_back(processed);            // ✓ 需要鎖：修改共享容器
//       std::cout << "Added: " << processed;  // ✗ 不需要這把鎖（見【3】）
//   }
//   三行裡只有中間那行碰到共享資料。
//   goodPush 把另外兩行移到鎖外，臨界區段從三行縮成一行。
//   → 這是「最小化原則」最直白的形式：
//     【只有讀寫共享資料的那幾行必須在鎖內】。
//
// 【2. 為什麼把計算移出鎖這麼重要】
//   臨界區段是序列化的 —— 同一時間只有一條執行緒能執行。
//   把計算放進鎖內，等於讓所有執行緒的計算排隊做，
//   多核心完全派不上用場（依 Amdahl 定律，序列比例 p 決定加速上限 1/p）。
//   本例的 `value * value` 只是一道乘法，看不出差別；
//   但只要換成稍微複雜的運算，差距立刻是數倍
//   （課程 5.6-4 用 1000 次三角函數實測，差了約 4 倍）。
//   → 養成「計算一律放鎖外」的習慣，成本是零，收益隨工作量放大。
//
// 【3. 那行 cout 的微妙之處（本檔最容易被誤解的地方）】
//   goodPush 把 cout 移到鎖外，這對【mtx 保護的 data】而言是正確的：
//   cout 與 data 是兩個不相干的資源，不該用同一把鎖。
//   但要說清楚兩件事：
//     ① 多執行緒同時 cout 【不是 UB】——
//        C++11 起標準保證對標準串流物件的並行格式化輸出不構成 data race
//        （libstdc++ 在 streambuf 層有內部鎖）。
//     ② 但【輸出會交錯】：`cout << a << b` 是多次獨立操作，
//        中間可以被其他執行緒插入。本機實測本檔的輸出確實會糊成
//        「Added: 0Added: 25Added: 36Added: Added: 1...」這種樣子。
//   → 結論：cout 需要保護的理由是「輸出可讀性」而非「避免 UB」，
//     而且該用【另一把專屬的鎖】，不是共用 data 的鎖。
//     更好的做法是先組成完整字串再一次輸出（見 5.5-6）。
//
// 【4. 「不同資源用不同的鎖」是重要原則】
//   如果 badPush 用同一把 mtx 同時保護 data 與 cout，
//   那麼「只是想印個東西」的執行緒會被「正在改 data」的執行緒擋住，
//   反之亦然 —— 兩個毫不相干的操作被硬綁在一起序列化。
//   正確做法是每個獨立的資源各有自己的鎖。
//   ⚠️ 但這也帶來代價：一旦某個操作需要同時碰兩個資源，
//     就得同時持有兩把鎖 → 必須用 std::scoped_lock 避免 AB-BA 死結
//     （見課程 5.5-3）。
//
// 【5. 本檔 main 的設計限制（誠實說明）】
//   main 只呼叫了 goodPush，沒有呼叫 badPush ——
//   所以實際執行時不會看到兩者的效能差異。
//   而且 `std::cout << "Added: " << processed;` 沒有換行，
//   本機實測輸出會擠成一長串且交錯。
//   本檔在下方補上了：① 可執行的效能對照 ② 正確的輸出處理方式，
//   讓「最小化原則」從口號變成可以量測的事實。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 臨界區段短，除了效能還有兩個好處
//   ① 降低死結風險：持鎖時間越短，越不容易與其他鎖的持有期重疊。
//   ② 提高 fast path 命中率：glibc 的 mutex 在真正睡眠前會先自旋一小段，
//      臨界區段夠短的話，等待者常常在自旋期間就等到了，
//      完全避開進核心的系統呼叫與上下文切換（成本差兩三個數量級）。
//
// (B) 但「越短越好」有下限：不能短到破壞不變量
//   把一個複合操作硬拆成幾個小臨界區段，會讓別人看到中間狀態 ——
//   那就回到課程 4.2 的錯誤了。
//       ✗ { lock; a -= x; }  { lock; b += x; }   // 中間 a+b 不守恆！
//       ✓ { lock; a -= x; b += x; }              // 整段一起
//   → 正確的原則不是「越短越好」，而是
//     【剛好涵蓋不變量被破壞的那段期間，不多也不少】。
//
// (C) 為什麼 data 與 mtx 應該包成一個類別
//   本檔用兩個獨立的全域變數（std::vector<int> data; std::mutex mtx;），
//   這在教學上直觀，但工程上不好：沒有任何機制強制「存取 data 前要鎖 mtx」，
//   任何人都可以直接碰 data。
//   正式做法是把兩者封裝成一個類別，只透過成員函式存取
//   （見課程 5.5-5 的 SafeContainer）。
//   Rust 的 Mutex<T> 直接把資料放進鎖裡，從型別系統強制這件事 ——
//   C++ 只能靠設計紀律。
//
// 【注意事項 Pay Attention】
// 1. 只有讀寫共享資料的那幾行需要在鎖內；計算與 I/O 一律移到鎖外。
// 2. 不同資源要用不同的鎖；共用一把鎖會把不相干的操作硬綁在一起。
// 3. 多執行緒 cout 不是 UB（標準有保證），但輸出會交錯 ——
//    需要的是【另一把專屬的鎖】或先組字串再一次輸出。
// 4. 「越短越好」有下限：臨界區段必須完整涵蓋不變量的破壞期。
// 5. 短臨界區段還能降低死結風險、提高 mutex fast path 的命中率。
// 6. 資料與保護它的鎖應該封裝在同一個類別裡，不要放成兩個獨立的全域變數。
//
// 【LeetCode 說明】本檔不附 LeetCode 範例。
//   本檔講的是「臨界區段該畫多大」的判斷方法，屬於設計原則而非演算法；
//   允許使用的設計題（146/155/705/707/1603）在 LeetCode 上都是單執行緒判題，
//   沒有臨界區段的概念；並行題（1114～1117/1195）考的是執行緒間的順序協調。
//   （粒度與分片的實際資料結構，已在「課程 5.6：互斥鎖的效能考量3.cpp」
//     用 LeetCode 705. Design HashSet 完整示範。）
//   故此處從缺，改以下方兩個真實情境（批次寫入、指標更新與統計分離）呈現。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】臨界區段的大小
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 臨界區段是不是越小越好?
//     答：方向對，但有下限。小的好處是：減少序列化、降低死結風險、
//         提高 mutex fast path 的命中率（等待者在自旋期間就等到，不必進核心）。
//         但它【不能小到破壞不變量】——
//         若把「a -= x」與「b += x」拆成兩個臨界區段，
//         中間別人就會看到 a+b 不守恆的狀態。
//         正確原則是「剛好涵蓋不變量的破壞期，不多也不少」。
//     追問：怎麼判斷「剛好」在哪?
//         → 找出這段程式碼要維持的不變量橫跨哪些變數，
//           臨界區段就必須完整涵蓋「從第一個變數被改動」到
//           「最後一個變數改完、不變量重新成立」的整段期間。
//
// 🔥 Q2. 為什麼不該用同一把鎖同時保護資料和 cout?
//     答：因為它們是兩個不相干的資源。共用一把鎖會讓
//         「只是想印個東西」的執行緒被「正在改資料」的執行緒擋住，
//         兩個毫無關係的操作被硬綁在一起序列化，白白損失並行度。
//         每個獨立的資源應該有自己的鎖。
//     追問：那用多把鎖有什麼代價?
//         → 一旦某個操作需要同時碰兩個資源，就得同時持有兩把鎖，
//           而不同的加鎖順序會造成 AB-BA 死結。
//           解法是用 std::scoped_lock（C++17）一次鎖多把，
//           它內部的演算法與加鎖順序無關。
//
// ⚠️ 陷阱. 「我把 cout 移到鎖外了，所以輸出不會有問題」——錯在哪?
//     答：移出鎖確實正確（cout 不該用 data 的鎖保護），
//         但這【不代表輸出就正常了】。多執行緒同時 cout 雖然不是 UB
//         （C++11 起標準對標準串流物件有保證），
//         但 `cout << a << b` 是多次獨立操作，中間可被插入，
//         所以輸出會交錯。本機實測本檔就會印出
//         「Added: 0Added: 25Added: 36Added: Added: 1...」這種樣子。
//     為什麼會錯：把「不是 UB」與「行為正確」畫上等號。
//         標準保證的是「不會產生資料競爭」，
//         不是「輸出會是你想要的樣子」。
//         要讓一行輸出不被打斷，得自己加一把專屬的輸出鎖，
//         或先把整行組成一個字串再一次送出去。
// =============================================================================

/*
# 第四階段：共享資料與競爭條件

## 課程 4.3：臨界區段概念

---

### 引言

既然我們知道共享資料存取會造成問題，下一步就是識別哪些程式碼需要保護。這些需要保護的程式碼區域稱為**臨界區段（Critical Section）**。

---

### 一、臨界區段的定義

```
┌─────────────────────────────────────────────────────────────┐
│                 臨界區段（Critical Section）                 │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  定義：存取共享資源的程式碼區段                              │
│                                                             │
│  特性：                                                      │
│  • 同一時間只能有一個執行緒執行                              │
│  • 其他執行緒必須等待                                        │
│  • 應該盡量短小                                              │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 二、圖解臨界區段

```
執行緒 A        執行緒 B        執行緒 C
    │               │               │
    ▼               ▼               ▼
[一般程式碼]    [一般程式碼]    [一般程式碼]
    │               │               │
    ▼               │               │
┌────────┐          │               │
│ 臨界   │  等待    │               │
│ 區段   │◄─────────┤               │
│        │          │      等待     │
└────────┘          │◄──────────────┤
    │               ▼               │
    │          ┌────────┐           │
    │          │ 臨界   │           │
    │          │ 區段   │           │
    │          └────────┘           │
    │               │               ▼
    │               │          ┌────────┐
    │               │          │ 臨界   │
    │               │          │ 區段   │
    │               │          └────────┘
    ▼               ▼               ▼
[一般程式碼]    [一般程式碼]    [一般程式碼]
```

---

### 三、識別臨界區段

```cpp
#include <iostream>
#include <thread>

int sharedData = 0;

void worker() {
    int localVar = 0;        // 不是臨界區段：區域變數
    
    localVar = 42;           // 不是臨界區段：只存取區域變數
    
    sharedData = localVar;   // ← 臨界區段：寫入共享資料
    
    int temp = sharedData;   // ← 臨界區段：讀取共享資料（如有寫入者）
    
    std::cout << temp;       // 可能是臨界區段：cout 也是共享資源
}
```

---

### 四、臨界區段的範圍

#### 範例：太大的臨界區段（不好）

```cpp
void badExample() {
    lock();
    
    // 臨界區段開始
    int result = complexCalculation();  // 不需要鎖！
    sharedData = result;                // 這才需要鎖
    logToFile(result);                  // 不需要鎖！
    // 臨界區段結束
    
    unlock();
}
```

#### 範例：精確的臨界區段（好）

```cpp
void goodExample() {
    int result = complexCalculation();  // 在鎖外計算
    
    lock();
    sharedData = result;                // 只保護必要的部分
    unlock();
    
    logToFile(result);                  // 在鎖外記錄
}
```

---

### 五、臨界區段設計原則

```
┌─────────────────────────────────────────────────────────────┐
│                 臨界區段設計原則                             │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. 最小化原則                                               │
│     只保護真正需要保護的程式碼                               │
│                                                             │
│  2. 快進快出                                                 │
│     在臨界區段內不做耗時操作                                 │
│                                                             │
│  3. 不要巢狀                                                 │
│     避免在臨界區段內進入另一個臨界區段（易死結）             │
│                                                             │
│  4. 不要等待                                                 │
│     不要在臨界區段內等待外部事件                             │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 六、識別共享資源

```cpp
#include <iostream>
#include <thread>
#include <vector>

// 共享資源
int globalCounter = 0;                    // 全域變數
static int staticCounter = 0;             // 靜態變數
std::vector<int> sharedVector;            // 全域容器

class MyClass {
    int memberVar;                        // 若多執行緒存取同一物件
    static int staticMember;              // 靜態成員
    
public:
    void method() {
        // memberVar 是否為共享資源？
        // 取決於是否多執行緒存取同一個 MyClass 物件
    }
};

void function() {
    int localVar = 0;                     // 不是共享資源
    static int localStatic = 0;           // 是共享資源！
    thread_local int tlVar = 0;           // 不是共享資源
}
```

---

### 七、常見的共享資源

| 資源類型 | 是否共享 | 需要保護？ |
|----------|----------|------------|
| 全域變數 | 是 | 是 |
| 函式內 static 變數 | 是 | 是 |
| 類別 static 成員 | 是 | 是 |
| 堆積上的物件（共用指標）| 是 | 是 |
| 區域變數 | 否 | 否 |
| thread_local 變數 | 否 | 否 |
| 函式參數（傳值）| 否 | 否 |
| const 全域變數 | 是 | 否（唯讀）|

---

### 八、程式碼標記練習

```cpp
#include <thread>

int shared = 0;

void example(int param) {
    int local = param;           // A
    local += 10;                 // B
    shared = local;              // C
    local = shared;              // D
    int result = local * 2;      // E
    shared += result;            // F
}
```

哪些是臨界區段？

```
A: 不是（只存取區域變數和參數）
B: 不是（只存取區域變數）
C: 是（寫入共享資料）
D: 是（讀取共享資料，且有其他寫入者）
E: 不是（只存取區域變數）
F: 是（讀取 + 寫入共享資料）
```

---

### 九、臨界區段與效能

```
┌─────────────────────────────────────────────────────────────┐
│              臨界區段長度 vs 效能                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  臨界區段太長：                                              │
│  • 其他執行緒等待時間長                                      │
│  • 並行度降低                                                │
│  • 效能接近單執行緒                                          │
│                                                             │
│  臨界區段太短（分散）：                                      │
│  • 頻繁加鎖解鎖                                              │
│  • 鎖的開銷累積                                              │
│  • 可能無法保護完整操作                                      │
│                                                             │
│  平衡：保護完整的邏輯操作，但不包含無關的程式碼               │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 十、實際案例分析

```cpp
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>

std::vector<int> data;
std::mutex mtx;

// 不好：臨界區段太大
void badPush(int value) {
    std::lock_guard<std::mutex> lock(mtx);
    int processed = value * value;        // 不需要鎖
    data.push_back(processed);            // 需要鎖
    std::cout << "Added: " << processed;  // 可能不需要鎖
}

// 好：臨界區段精確
void goodPush(int value) {
    int processed = value * value;        // 鎖外計算
    
    {
        std::lock_guard<std::mutex> lock(mtx);
        data.push_back(processed);        // 只保護必要操作
    }
    
    std::cout << "Added: " << processed;  // 鎖外輸出
}
```

---

### 十一、本課重點回顧

1. **臨界區段**是存取共享資源的程式碼區域
2. 同一時間只能有一個執行緒執行臨界區段
3. 臨界區段應該**盡量短小**
4. 只保護真正需要保護的程式碼
5. 避免在臨界區段內做耗時操作
6. 區域變數和 thread_local 變數不需要保護

---

### 下一課預告

在 **課程 4.4：資料競爭範例分析** 中，我們將：
- 分析更多真實世界的資料競爭案例
- 學習如何發現潛在的競爭條件
- 探討競爭條件的除錯技巧

---

準備好繼續嗎？
*/



#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <sstream>
#include <string>
#include <chrono>
#include <iomanip>

std::vector<int> data;
std::mutex mtx;

// 不好：臨界區段太大
void badPush(int value) {
    std::lock_guard<std::mutex> lock(mtx);
    int processed = value * value;        // 不需要鎖
    data.push_back(processed);            // 需要鎖
    std::cout << "Added: " << processed;  // 可能不需要鎖
}

// 好：臨界區段精確
void goodPush(int value) {
    int processed = value * value;        // 鎖外計算
    
    {
        std::lock_guard<std::mutex> lock(mtx);
        data.push_back(processed);        // 只保護必要操作
    }
    
    std::cout << "Added: " << processed;  // 鎖外輸出
}


// -----------------------------------------------------------------------------
// 【補強版】把上面說明的三件事做出來
//   ① badPush / goodPush 的可執行效能對照
//   ② cout 用【專屬的鎖】，而不是共用 data 的鎖
//   ③ 一次輸出完整的一行，避免交錯
// -----------------------------------------------------------------------------
std::mutex outMtx;          // 專門保護輸出，與 mtx（保護 data）分開

// 模擬「稍微有份量」的計算，讓鎖內 vs 鎖外的差距看得出來
long compute(int value) {
    long h = value;
    for (int i = 0; i < 300; ++i) h = h * 31 + (i ^ value);
    return h;
}

std::mutex benchMtx;
std::vector<long> benchData;

// ✗ 計算在鎖內 → 所有執行緒的計算排隊做
void badPushBench(int value) {
    std::lock_guard<std::mutex> lock(benchMtx);
    long processed = compute(value);
    benchData.push_back(processed);
}

// ✓ 計算在鎖外 → 只有 push_back 序列化
void goodPushBench(int value) {
    long processed = compute(value);
    std::lock_guard<std::mutex> lock(benchMtx);
    benchData.push_back(processed);
}

template<typename Fn>
double runPushBench(Fn fn, int numThreads, int perThread) {
    benchData.clear();
    benchData.reserve(static_cast<size_t>(numThreads) * static_cast<size_t>(perThread));
    std::vector<std::thread> threads;

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([fn, i, perThread] {
            for (int k = 0; k < perThread; ++k) fn(i * perThread + k);
        });
    }
    for (auto& t : threads) t.join();
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】批次寫入：把 N 次上鎖降成 1 次
//   情境：解析一個檔案並把結果寫進共享容器。
//         逐筆 push_back 意味著每筆都要上鎖一次。
//   正解：先在本地容器累積，整批解析完才上一次鎖 insert 進去。
//         這同時解決「上鎖次數」與「鎖內可能觸發 vector 擴容」兩個問題。
// -----------------------------------------------------------------------------
class ResultCollector {
private:
    mutable std::mutex mtx;
    std::vector<long> results;

public:
    // ✗ 每筆都上鎖
    void addOne(long v) {
        std::lock_guard<std::mutex> lock(mtx);
        results.push_back(v);
    }

    // ✓ 整批一次上鎖
    void addBatch(const std::vector<long>& batch) {
        std::lock_guard<std::mutex> lock(mtx);
        results.insert(results.end(), batch.begin(), batch.end());
    }

    size_t size() const { std::lock_guard<std::mutex> lock(mtx); return results.size(); }
    void reset() { std::lock_guard<std::mutex> lock(mtx); results.clear(); }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】不同資源用不同的鎖
//   情境：一個服務同時維護「使用者資料」與「請求統計」。
//         兩者毫不相干，卻常被寫成共用一把大鎖 ——
//         結果是「查一下統計數字」會被「更新使用者資料」擋住。
//   正解：兩份資料各有自己的鎖，互不干擾。
//   ⚠️ 代價：若有操作需要同時碰兩者，必須用 std::scoped_lock 避免死結。
// -----------------------------------------------------------------------------
class UserService {
private:
    mutable std::mutex userMtx;      // 保護 users
    std::vector<std::string> users;

    mutable std::mutex statsMtx;     // 保護 stats，與 userMtx 完全獨立
    long requestCount = 0;

public:
    void addUser(const std::string& name) {
        std::lock_guard<std::mutex> lock(userMtx);
        users.push_back(name);
    }

    size_t userCount() const {
        std::lock_guard<std::mutex> lock(userMtx);
        return users.size();
    }

    // 這個操作完全不會被 addUser 擋住 —— 兩把鎖各自獨立
    void recordRequest() {
        std::lock_guard<std::mutex> lock(statsMtx);
        ++requestCount;
    }

    long requests() const {
        std::lock_guard<std::mutex> lock(statsMtx);
        return requestCount;
    }

    // 需要同時碰兩者時，用 scoped_lock 一次鎖兩把（不依賴加鎖順序，不會死結）
    std::pair<size_t, long> summary() const {
        std::scoped_lock lock(userMtx, statsMtx);
        return {users.size(), requestCount};
    }
};

int main() {
    std::cout << "=== 原始示範：goodPush（cout 在鎖外，輸出會交錯）===\n";
    std::thread t1([]{
        for (int i = 0; i < 5; ++i) {
            goodPush(i);
        }
    });
    
    std::thread t2([]{
        for (int i = 5; i < 10; ++i) {
            goodPush(i);
        }
    });
    
    t1.join();
    t2.join();
    std::cout << "\n→ 上面那串糊在一起的輸出，正是「cout 沒有自己的鎖」的結果。\n";
    std::cout << "  這【不是 UB】（標準對串流物件有保證），但完全無法閱讀。\n";

    std::cout << "\n=== 修正：cout 用專屬的鎖，且一次輸出完整一行 ===\n";
    {
        std::vector<std::thread> ths;
        for (int t = 0; t < 2; ++t) {
            ths.emplace_back([t]{
                for (int i = t * 5; i < t * 5 + 5; ++i) {
                    int processed = i * i;                 // 鎖外計算
                    {
                        std::lock_guard<std::mutex> lock(mtx);      // 只保護 data
                        data.push_back(processed);
                    }
                    std::ostringstream oss;                // 鎖外組字串
                    oss << "Added: " << processed << "\n";
                    {
                        std::lock_guard<std::mutex> lock(outMtx);   // 專屬的輸出鎖
                        std::cout << oss.str();            // 一次輸出完整一行
                    }
                }
            });
        }
        for (auto& t : ths) t.join();
        std::cout << "→ 每行完整不交錯；行的順序仍取決於排程（每次執行都不同）。\n";
    }

    std::cout << "\n=== 效能對照：計算在鎖內 vs 鎖外 ===\n";
    {
        runPushBench(badPushBench, 4, 5000);      // 預熱
        runPushBench(goodPushBench, 4, 5000);

        double tBad = runPushBench(badPushBench, 4, 5000);
        double tGood = runPushBench(goodPushBench, 4, 5000);

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "4 執行緒 × 5000 筆，每筆做 300 次運算\n";
        std::cout << "計算在鎖內（bad） ：" << tBad << " ms\n";
        std::cout << "計算在鎖外（good）：" << tGood << " ms\n";
        std::cout << "加速比            ：" << (tBad / tGood) << "x\n";
        std::cout << "元素數: " << benchData.size() << "（必定為 20000，正確性不變）\n";
        std::cout << "→ 兩者做的計算完全相同，差別只在「計算有沒有佔著鎖」。\n";
    }

    std::cout << "\n=== 日常實務 1：批次寫入把 N 次上鎖降成 1 次 ===\n";
    {
        ResultCollector collector;

        // 逐筆
        auto s1 = std::chrono::steady_clock::now();
        {
            std::vector<std::thread> ths;
            for (int i = 0; i < 4; ++i) {
                ths.emplace_back([&collector, i] {
                    for (int k = 0; k < 50000; ++k) collector.addOne(i * 50000 + k);
                });
            }
            for (auto& t : ths) t.join();
        }
        auto e1 = std::chrono::steady_clock::now();
        double tOne = std::chrono::duration<double, std::milli>(e1 - s1).count();
        size_t n1 = collector.size();

        collector.reset();

        // 批次
        auto s2 = std::chrono::steady_clock::now();
        {
            std::vector<std::thread> ths;
            for (int i = 0; i < 4; ++i) {
                ths.emplace_back([&collector, i] {
                    std::vector<long> local;
                    local.reserve(50000);
                    for (int k = 0; k < 50000; ++k) local.push_back(i * 50000 + k);
                    collector.addBatch(local);        // 整條執行緒只上鎖一次
                });
            }
            for (auto& t : ths) t.join();
        }
        auto e2 = std::chrono::steady_clock::now();
        double tBatch = std::chrono::duration<double, std::milli>(e2 - s2).count();

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "4 執行緒 × 50000 筆\n";
        std::cout << "逐筆上鎖（200000 次）：" << tOne << " ms\n";
        std::cout << "批次上鎖（4 次）      ：" << tBatch << " ms\n";
        std::cout << "兩者結果相同: " << (n1 == collector.size() ? "是" : "否")
                  << "（各為 " << n1 << " 與 " << collector.size() << "）\n";
    }

    std::cout << "\n=== 日常實務 2：不同資源用不同的鎖 ===\n";
    {
        UserService svc;
        std::vector<std::thread> ths;

        // 一組執行緒改使用者資料，另一組只更新統計 —— 兩者互不阻擋
        for (int i = 0; i < 2; ++i) {
            ths.emplace_back([&svc, i] {
                for (int k = 0; k < 5000; ++k) svc.addUser("user-" + std::to_string(i * 5000 + k));
            });
        }
        for (int i = 0; i < 2; ++i) {
            ths.emplace_back([&svc] {
                for (int k = 0; k < 5000; ++k) svc.recordRequest();
            });
        }
        for (auto& t : ths) t.join();

        auto sum = svc.summary();
        std::cout << "使用者數: " << sum.first << "（必定為 10000）\n";
        std::cout << "請求數  : " << sum.second << "（必定為 10000）\n";
        std::cout << "→ addUser 與 recordRequest 用不同的鎖，完全不互相阻擋；\n";
        std::cout << "  summary() 需要同時看兩者，用 scoped_lock 一次鎖兩把避免死結。\n";
    }
    
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 4.3：臨界區段概念3.cpp' -o critical3
//   （std::scoped_lock 需要 C++17）

// ⚠️ 三處內容【每次執行都不同】，下面貼的是本機某一次的真實實測：
//   (1) 第一段 goodPush 的輸出會【交錯糊在一起】——cout 沒有自己的鎖，
//       而 cout << a << b 是多次獨立操作，中間可被插入。
//       這不是 UB（C++11 起標準對串流物件有保證），但完全無法閱讀。
//       糊法每次都不同。
//   (2) 第二段修正版每行完整，但十行的【先後順序】仍取決於排程。
//   (3) 所有時間數字與加速比（效能量測，受頻率調節、排程、負載影響）。
//
// 確定值：元素數 20000、批次前後各 200000、使用者數與請求數各 10000
//         —— 這些是正確性驗證，每次執行都相同。

// === 預期輸出 ===
// === 原始示範：goodPush（cout 在鎖外，輸出會交錯）===
// Added: 0Added: 25Added: 1Added: 4Added: 36Added: 49Added: Added: 9Added: 1664Added: 81
// → 上面那串糊在一起的輸出，正是「cout 沒有自己的鎖」的結果。
//   這【不是 UB】（標準對串流物件有保證），但完全無法閱讀。
//
// === 修正：cout 用專屬的鎖，且一次輸出完整一行 ===
// Added: 0
// Added: 1
// Added: 4
// Added: 9
// Added: 16
// Added: 25
// Added: 36
// Added: 49
// Added: 64
// Added: 81
// → 每行完整不交錯；行的順序仍取決於排程（每次執行都不同）。
//
// === 效能對照：計算在鎖內 vs 鎖外 ===
// 4 執行緒 × 5000 筆，每筆做 300 次運算
// 計算在鎖內（bad） ：18.82 ms
// 計算在鎖外（good）：3.64 ms
// 加速比            ：5.17x
// 元素數: 20000（必定為 20000，正確性不變）
// → 兩者做的計算完全相同，差別只在「計算有沒有佔著鎖」。
//
// === 日常實務 1：批次寫入把 N 次上鎖降成 1 次 ===
// 4 執行緒 × 50000 筆
// 逐筆上鎖（200000 次）：19.00 ms
// 批次上鎖（4 次）      ：1.32 ms
// 兩者結果相同: 是（各為 200000 與 200000）
//
// === 日常實務 2：不同資源用不同的鎖 ===
// 使用者數: 10000（必定為 10000）
// 請求數  : 10000（必定為 10000）
// → addUser 與 recordRequest 用不同的鎖，完全不互相阻擋；
//   summary() 需要同時看兩者，用 scoped_lock 一次鎖兩把避免死結。
