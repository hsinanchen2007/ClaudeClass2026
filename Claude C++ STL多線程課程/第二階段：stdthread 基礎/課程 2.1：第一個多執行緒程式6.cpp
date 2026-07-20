/*
# 第二階段：std::thread 基礎

## 課程 2.1：第一個多執行緒程式

---

### 引言

從這一課開始，我們正式動手撰寫多執行緒程式。`std::thread` 是 C++ 多執行緒程式設計的核心類別，掌握它是一切的起點。

---

### 一、std::thread 基本概念

`std::thread` 代表一個執行緒物件。建立它時，執行緒就開始執行。

```
┌─────────────────────────────────────────┐
│           std::thread 生命週期          │
├─────────────────────────────────────────┤
│                                         │
│  建構 ──► 執行中 ──► join() 或 detach() │
│                                         │
│  • 建構時立即開始執行                    │
│  • 必須在解構前呼叫 join() 或 detach()  │
│  • 否則程式會呼叫 std::terminate()      │
│                                         │
└─────────────────────────────────────────┘
```

---

### 二、最簡單的多執行緒程式

```cpp
// 檔案：lesson_2_1_first_thread.cpp

#include <iostream>
#include <thread>

void sayHello() {
    std::cout << "Hello from thread!" << std::endl;
}

int main() {
    std::thread t(sayHello);  // 建立並啟動執行緒
    t.join();                 // 等待執行緒結束
    
    std::cout << "Back in main" << std::endl;
    return 0;
}
```

```bash
g++ -std=c++17 -pthread -o lesson_2_1 lesson_2_1_first_thread.cpp
./lesson_2_1
```

輸出：
```
Hello from thread!
Back in main
```

---

### 三、join() vs detach()

#### join() — 等待執行緒結束

```cpp
#include <iostream>
#include <thread>

void work() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Work done!" << std::endl;
}

int main() {
    std::thread t(work);
    std::cout << "Waiting..." << std::endl;
    t.join();  // 阻塞，直到 t 完成
    std::cout << "Thread finished" << std::endl;
    return 0;
}
```

輸出：
```
Waiting...
Work done!
Thread finished
```

#### detach() — 分離執行緒

```cpp
#include <iostream>
#include <thread>

void background() {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "Background task" << std::endl;  // 可能不會印出！
}

int main() {
    std::thread t(background);
    t.detach();  // 分離，不等待
    
    std::cout << "Main exits" << std::endl;
    return 0;  // 主程式結束，背景執行緒可能還沒完成
}
```

輸出（不確定）：
```
Main exits
```

**警告**：detach 後的執行緒在主程式結束時會被強制終止，要小心使用。

---

### 四、忘記 join/detach 的後果

```cpp
#include <iostream>
#include <thread>

void work() {
    std::cout << "Working..." << std::endl;
}

int main() {
    std::thread t(work);
    // 忘記呼叫 join() 或 detach()
    return 0;  // 程式崩潰！呼叫 std::terminate()
}
```

執行結果：
```
terminate called without an active exception
Aborted (core dumped)
```

⚠️ 補充（本機實測，GCC 15.2.0）：**程式必定中止**（exit code 134 / SIGABRT）——
這是標準 [thread.thread.destr] 規定的行為，不是未定義行為。
但 `Working...` **不一定**會印出來：main 建立執行緒後立刻 `return`，
新執行緒是否來得及被排程執行是不確定的。本機這次就**沒有**印出 `Working...`。
兩種結果都合法，別把它當成保證的一部分。

（另注意：收集這種會中止的程式的輸出時要用 `stdbuf -o0` 關掉緩衝，
否則緩衝區裡尚未沖出的內容會在 abort 時遺失，讓你誤判執行進度。）

---

### 五、使用 Lambda 建立執行緒

Lambda 是建立執行緒最簡潔的方式：

```cpp
#include <iostream>
#include <thread>

int main() {
    std::thread t([]() {
        std::cout << "Hello from lambda!" << std::endl;
    });
    
    t.join();
    return 0;
}
```

---

### 六、多個執行緒同時運行

```cpp
// 檔案：lesson_2_1_multiple.cpp

#include <iostream>
#include <thread>

int main() {
    std::thread t1([]() {
        for (int i = 0; i < 3; ++i)
            std::cout << "A";
    });
    
    std::thread t2([]() {
        for (int i = 0; i < 3; ++i)
            std::cout << "B";
    });
    
    t1.join();
    t2.join();
    
    std::cout << std::endl;
    return 0;
}
```

輸出（順序不確定）：
```
AABBAB
```

每次執行結果可能不同，這就是多執行緒的非確定性。

---

### 七、關鍵要點圖解

```
┌──────────────────────────────────────────────────────────┐
│                   std::thread 使用要點                    │
├──────────────────────────────────────────────────────────┤
│                                                          │
│  ✓ 建構時傳入可呼叫物件（函式、Lambda、函式物件）          │
│                                                          │
│  ✓ 執行緒建立後立即開始執行                               │
│                                                          │
│  ✓ 必須呼叫 join() 或 detach()，否則程式崩潰             │
│                                                          │
│  ✓ join() = 等待結束，detach() = 放手不管                │
│                                                          │
│  ✓ 多執行緒的輸出順序是不確定的                           │
│                                                          │
└──────────────────────────────────────────────────────────┘
```

---

### 八、本課重點回顧

1. `std::thread` 建構時傳入函式，執行緒立即開始執行
2. `join()` 會阻塞等待執行緒結束
3. `detach()` 讓執行緒在背景獨立運行
4. 必須在 thread 物件解構前呼叫 join() 或 detach()
5. Lambda 是建立執行緒最方便的方式
6. 多執行緒的執行順序是不確定的

---

### 下一課預告

在 **課程 2.2：執行緒函式的多種形式** 中，我們將學習：
- 一般函式
- Lambda 表達式
- 成員函式
- 函式物件（Functor）

---

準備好繼續嗎？
*/

// =============================================================================
//  課程 2.1：第一個多執行緒程式6.cpp  —  多執行緒的非確定性(本課總整理)
// =============================================================================
//
// 【主題資訊 Information】
//   類別      : std::thread —— 一個「執行緒的所有權 handle」
//   標準版本  : C++11;C++20 加入自動 join 的 std::jthread
//   標頭檔    : <thread>
//   建構      : 傳入任何可呼叫物件,執行緒「立即」開始執行
//   必要動作  : 解構前必須 join() 或 detach(),否則 std::terminate()
//   本機實測  : std::thread::hardware_concurrency() = 16
//
// 【詳細解釋 Explanation】
//
// 【1. 本檔要傳達的核心概念:非確定性】
// 前面的範例都刻意設計成輸出固定,好讓你先掌握語法。本檔正好相反 ——
// 它讓兩條執行緒同時印 "A" 和 "B",目的就是讓你親眼看到:
// **同一支程式、同一台機器,每次執行的輸出可能不一樣。**
//
// 這不是 bug,而是多執行緒的本質。作業系統排程器決定哪條執行緒何時
// 拿到 CPU、執行多久,這個決定受到當下系統負載、核心數、快取狀態、
// 甚至其他行程的影響。你的程式對此沒有任何控制權,也不該假設任何順序。
//
// 【2. 「非確定性」與「錯誤」的界線在哪】
// 初學者最常見的誤解是把兩者混為一談。正確的區分方式是:
//   * 輸出順序不固定 → 這是「非確定性」,是正常的。
//   * 同一條執行緒內部的邏輯出錯、資料被寫壞、算出來的總和不對
//     → 這才是「錯誤」,通常來自資料競爭(data race)。
// 判準:如果你的程式的「正確性」依賴於某個特定的執行順序,
// 而你沒有用任何同步機制去保證它,那它就是錯的 ——
// 即使它在你的機器上跑一百次都剛好對。
//
// 【3. 為什麼本機輸出常常是整齊的 AAABBB】
// 實測本機多次執行,最常見的結果是 "AAABBB" 而不是講義寫的 "AABBAB"。
// 原因是每條執行緒的工作量太小(只印三個字元),
// 通常在時間片用完之前就整段跑完了,根本沒機會被切開。
// ⚠️ 但這絕不是保證。工作量變大、系統變忙、換一台核心數不同的機器,
//    交錯就會出現。「我測過都沒問題」在多執行緒領域是最危險的一句話。
//
// 【4. std::cout 的並行輸出:不會壞掉,但會交錯】
// 標準保證對同步過的標準串流做並行的格式化輸出不會產生 data race
// (不會毀損串流狀態、不會崩潰),但「不保證一次輸出敘述的內容是不可分割的」。
// 所以 std::cout << "A" << "B"; 這兩次插入之間,別的執行緒完全可能插進來。
// 想要整行完整,唯一的方法是自己用 mutex 保護,或先組好整個字串再一次輸出。
//
// 【概念補充 Concept Deep Dive】
//
// (A) hardware_concurrency() 的真正意義
//   本機實測回傳 16。它代表「硬體能真正並行執行的執行緒數」的提示值,
//   通常是邏輯核心數。要注意三件事:
//     * 它只是「提示」,標準允許回傳 0 表示無法判斷。
//     * 它不知道 cgroup/容器的 CPU 限制,在 Kubernetes 裡常常高估 ——
//       容器只分到 2 核,它仍可能回報宿主機的 16。
//     * 建立超過這個數量的執行緒不會出錯,只是會增加上下文切換成本。
//
// (B) 執行緒不是免費的
//   每條 std::thread 對應一個作業系統執行緒,預設堆疊在 Linux 上通常是 8 MB
//   的虛擬位址空間(實際用多少才配多少實體記憶體)。建立與銷毀都要進核心,
//   成本遠高於一次函式呼叫。這就是「執行緒池」存在的理由:
//   建立一次、重複使用,而不是每個任務都開一條新執行緒。
//
// (C) 為什麼建構 thread 後執行緒「立即」開始
//   std::thread 沒有提供「先建立、稍後啟動」的介面。這是刻意的設計:
//   一個 std::thread 物件的存在,就代表「有一條執行緒正被它擁有」,
//   沒有中間的曖昧狀態。要延後執行,做法是在執行緒函式一開始就等待
//   某個條件變數,而不是延後建立。
//
// 【注意事項 Pay Attention】
// 1. 永遠不要假設執行緒的執行順序,除非你用了同步機制明確保證它。
// 2. 「測過很多次都對」不能證明多執行緒程式正確 ——
//    競態條件可能一年才出現一次,而且通常在正式環境的高負載下。
// 3. std::cout 並行輸出不會崩潰,但會交錯;要整行完整必須自己加鎖。
// 4. 建立執行緒後一定要 join() 或 detach(),否則解構時 std::terminate()。
// 5. hardware_concurrency() 只是提示,可能回傳 0,也可能無視容器的 CPU 限制。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::thread 基礎與非確定性
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 兩條執行緒分別印 "A" 和 "B",輸出順序由什麼決定?
//     答：由作業系統排程器決定,程式無法控制,也不該依賴。
//         標準對執行緒之間的執行順序沒有任何保證。
//         要保證順序,必須自己用同步機制(join、mutex + 條件變數、
//         atomic、號誌)明確建立 happens-before 關係。
//     追問：那為什麼我在自己電腦上跑很多次,輸出都是整齊的 AAABBB?
//         → 因為每條執行緒的工作量太小,通常在時間片用完前就跑完了。
//           工作量變大、系統變忙、或換一台機器,交錯就會出現。
//           「測過都沒問題」在多執行緒領域從來不是正確性的證明。
//
// 🔥 Q2. 多執行緒同時對 std::cout 輸出,會不會造成未定義行為?
//     答：不會。標準保證對同步過的標準串流做並行格式化輸出不會產生
//         data race,串流狀態不會被毀損。但它「不保證」一次輸出敘述
//         是不可分割的,所以不同執行緒的字元會交錯。
//     追問：怎麼讓一整行不被切開?
//         → 用一把 mutex 保護輸出,或先把整行組成一個 std::string
//           再用單一次 << 送出(後者能大幅縮小競爭視窗,但嚴格說仍非原子)。
//
// ⚠️ 陷阱. 「我的程式跑了一千次,結果都正確,所以它是執行緒安全的。」
//     答：這什麼都證明不了。競態條件的觸發需要特定的時序,
//         而你在開發機的輕負載下幾乎不可能碰到那個時序視窗。
//         真實案例中,競態常常在正式環境跑了好幾個月之後,
//         某次流量尖峰時才第一次出現 —— 然後極難重現。
//     為什麼會錯：把測試當成證明。測試只能證明「有 bug」,
//         不能證明「沒有 bug」,這一點在並行程式上被放大到極致。
//         正確的做法是從設計上論證同步的正確性,並用
//         ThreadSanitizer(g++ -fsanitize=thread)這類工具主動偵測競態。
// ═══════════════════════════════════════════════════════════════════════════

#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1114. Print in Order
//   題目：三條執行緒分別呼叫同一個物件的 first()、second()、third(),
//         呼叫的先後順序是任意的;請確保實際印出的順序一定是 first→second→third。
//   為什麼用到本主題：這題正是本課「執行緒順序不確定」的直接應對 ——
//         它逼你承認「呼叫順序無法控制」,然後用同步機制把想要的順序
//         明確地建立出來,而不是祈禱排程器幫你。
//   作法：用一個 stage_ 變數表示「現在輪到第幾棒」,
//         後面的棒次在條件變數上等待,直到前一棒把 stage_ 推進並通知。
//   注意：條件變數是課程後段的主題,這裡先用它示範標準解法。
//         cv_.wait 傳入 predicate 的形式可同時處理 spurious wakeup(偽喚醒),
//         這是必須養成的寫法 —— 不要用沒有 predicate 的 wait。
// -----------------------------------------------------------------------------
class Foo {
    std::mutex              m_;
    std::condition_variable cv_;
    int                     stage_ = 1;   // 1=該印 first,2=該印 second,3=該印 third

public:
    void first(std::function<void()> printFirst) {
        std::unique_lock<std::mutex> lk(m_);
        printFirst();
        stage_ = 2;
        cv_.notify_all();
    }

    void second(std::function<void()> printSecond) {
        std::unique_lock<std::mutex> lk(m_);
        cv_.wait(lk, [this] { return stage_ == 2; });   // 等到輪到我
        printSecond();
        stage_ = 3;
        cv_.notify_all();
    }

    void third(std::function<void()> printThird) {
        std::unique_lock<std::mutex> lk(m_);
        cv_.wait(lk, [this] { return stage_ == 3; });
        printThird();
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】把「交錯的輸出」修好:先組字串,再一次送出
//   情境: 多執行緒的 log 最惱人的問題就是行與行互相穿插,變得無法閱讀。
//         這裡並排示範兩種寫法,讓你直接看到差別。
//   為什麼用本主題: 對應【詳細解釋 4】—— std::cout 的並行輸出不會壞,
//                   但會交錯,而這是有標準做法可以解決的。
// -----------------------------------------------------------------------------
std::mutex g_logMutex;

// 壞寫法:多次 << 之間可能被其他執行緒插入
void badLog(int id, int seq) {
    std::cout << "[thread " << id << "] 訊息 " << seq << "\n";
}

// 好寫法:先組好整行,再在鎖的保護下一次輸出
void goodLog(int id, int seq) {
    std::string line = "[thread " + std::to_string(id) + "] 訊息 " +
                       std::to_string(seq) + "\n";
    std::lock_guard<std::mutex> lock(g_logMutex);
    std::cout << line;
}

int main() {
    std::cout << "=== 原始示範:兩條執行緒同時印 A 與 B ===" << std::endl;
    {
        std::thread t1([]() {
            for (int i = 0; i < 3; ++i) std::cout << "A";
        });
        std::thread t2([]() {
            for (int i = 0; i < 3; ++i) std::cout << "B";
        });
        t1.join();
        t2.join();
        std::cout << std::endl;
        std::cout << "  ↑ 這一行每次執行都可能不同(AAABBB / ABABAB / BBBAAA …)"
                  << std::endl;
    }

    std::cout << "\n=== 本機硬體資訊 ===" << std::endl;
    std::cout << "  hardware_concurrency() = " << std::thread::hardware_concurrency()
              << " (提示值,可能回傳 0,也可能無視容器的 CPU 限制)" << std::endl;

    std::cout << "\n=== LeetCode 1114. Print in Order ===" << std::endl;
    {
        Foo foo;
        // 刻意用「相反」的順序建立執行緒,證明輸出順序不受建立順序影響
        std::thread c([&foo]() { foo.third ([] { std::cout << "third\n";  }); });
        std::thread b([&foo]() { foo.second([] { std::cout << "second\n"; }); });
        std::thread a([&foo]() { foo.first ([] { std::cout << "first\n";  }); });
        c.join();
        b.join();
        a.join();
        std::cout << "  ↑ 即使 third 的執行緒最先建立,輸出仍固定是 first→second→third"
                  << std::endl;
    }

    std::cout << "\n=== 實務:交錯的 log vs 加鎖後的 log ===" << std::endl;
    std::cout << "-- 好寫法(先組字串 + 上鎖,每行完整) --" << std::endl;
    {
        std::vector<std::thread> pool;
        for (int id = 1; id <= 3; ++id) {
            pool.emplace_back([id]() {
                for (int s = 1; s <= 2; ++s) goodLog(id, s);
            });
        }
        for (std::thread& t : pool) t.join();
    }
    std::cout << "  ↑ 每一行都完整,不會被切開(行的先後順序仍然不確定)"
              << std::endl;
    std::cout << "  (badLog() 也已定義在上方供對照,它用多次 << 輸出,"
                 "在高負載下就可能出現半行穿插)" << std::endl;
    if (false) badLog(0, 0);   // 僅為避免 -Wunused-function,不實際執行

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.1：第一個多執行緒程式6.cpp" -o first_thread

// 注意:以下為某一次實際執行的結果。
//   * 第一段的 AAABBB 每次執行都可能不同 —— 那正是本檔要示範的重點。
//   * hardware_concurrency() = 16 是本機的值,換機器就不同。
//   * 實務段各行的先後順序每次執行都可能不同,但每一行都會是完整的。
//   * 唯一每次都固定的是 LeetCode 1114 那段:first→second→third,
//     因為那是用條件變數「明確保證」出來的,不是碰運氣。

// === 預期輸出 ===
// === 原始示範:兩條執行緒同時印 A 與 B ===
// AAABBB
//   ↑ 這一行每次執行都可能不同(AAABBB / ABABAB / BBBAAA …)
//
// === 本機硬體資訊 ===
//   hardware_concurrency() = 16 (提示值,可能回傳 0,也可能無視容器的 CPU 限制)
//
// === LeetCode 1114. Print in Order ===
// first
// second
// third
//   ↑ 即使 third 的執行緒最先建立,輸出仍固定是 first→second→third
//
// === 實務:交錯的 log vs 加鎖後的 log ===
// -- 好寫法(先組字串 + 上鎖,每行完整) --
// [thread 3] 訊息 1
// [thread 3] 訊息 2
// [thread 1] 訊息 1
// [thread 1] 訊息 2
// [thread 2] 訊息 1
// [thread 2] 訊息 2
//   ↑ 每一行都完整,不會被切開(行的先後順序仍然不確定)
//   (badLog() 也已定義在上方供對照,它用多次 << 輸出,在高負載下就可能出現半行穿插)
