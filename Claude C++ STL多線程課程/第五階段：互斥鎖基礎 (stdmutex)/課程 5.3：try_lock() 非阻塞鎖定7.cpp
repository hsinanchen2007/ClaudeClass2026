// =============================================================================
//  課程 5.3：try_lock() 非阻塞鎖定7.cpp  —  自訂逾時鎖 vs std::timed_mutex
// =============================================================================
//
// 【主題資訊 Information】
//   本檔手寫模式：
//       bool tryLockFor(std::mutex& m, std::chrono::milliseconds timeout);
//       —— 用 try_lock() + sleep 迴圈土炮出「等一段時間就放棄」
//   標準提供的正解（C++11）：
//       class std::timed_mutex;                                   // <mutex>
//           template<class Rep, class Period>
//           bool try_lock_for  (const chrono::duration<Rep,Period>& rel_time);
//           template<class Clock, class Duration>
//           bool try_lock_until(const chrono::time_point<Clock,Duration>& abs_time);
//       class std::recursive_timed_mutex;                         // C++11
//       class std::shared_timed_mutex;                            // C++14
//   標頭檔：<mutex>、<chrono>、<thread>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼需要「有逾時的鎖」】
//   lock() 是「無限期等待」，try_lock() 是「完全不等」。
//   真實系統最常見的需求其實在兩者【中間】：
//     「我可以等，但不能等太久——超過 100ms 就回報服務繁忙。」
//   這在有 SLA 的服務裡是硬需求：一個請求的總預算可能只有 200ms，
//   絕不能因為某把鎖塞住而讓它無限期掛著。
//   逾時鎖把「最壞延遲」變成可控、可預測的量。
//
// 【2. 本檔手寫版本的三個實際缺陷】
//   tryLockFor 的邏輯是「每 1ms 醒來試一次，直到逾時」。能動，但：
//     (a) 【延遲不精確】：鎖若在兩次取樣的【中間】被釋放，
//         這段最多 1ms 的空窗就白等了。取樣間隔調小則 CPU 消耗上升，
//         調大則延遲變差——這是土炮方案無法擺脫的取捨。
//     (b) 【浪費 CPU】：每次醒來都是一次完整的 context switch。
//         100ms 的等待就是約 100 次無謂的醒來，其中 99 次注定失敗。
//     (c) 【逾時判斷不精準】：sleep_for 只保證「至少」睡那麼久，
//         再加上 try_lock 本身的成本，實際逾時一定略大於設定值。
//   → std::timed_mutex 沒有這些問題：它底層用【帶逾時參數的 futex 等待】，
//     執行緒真的睡著（CPU 0%），鎖一釋放就【立刻】被喚醒，
//     不需要輪詢，延遲最低。
//
// 【3. 那為什麼標準要分成 mutex 與 timed_mutex 兩種型別？】
//   因為逾時能力【不是免費的】：timed_mutex 的實作路徑比 mutex 複雜，
//   在部分平台上還需要額外的狀態。
//   C++ 的設計哲學是「不為你沒用到的功能付出代價」
//   （zero-overhead principle），所以把逾時能力分離成獨立型別，
//   讓絕大多數不需要逾時的場合仍能用最精簡的 std::mutex。
//   （本機 x86-64 / glibc 上兩者的 sizeof 剛好相同，見程式輸出；
//     但這是【實作定義】的巧合，不可當成通則。）
//
// 【4. try_lock_for vs try_lock_until——與 sleep_for/sleep_until 同一個道理】
//     try_lock_for(100ms)              → 相對：從現在起最多等 100ms
//     try_lock_until(deadline)         → 絕對：等到某個時間點為止
//   在「一個請求有總時間預算」的場景，try_lock_until 才是對的：
//       auto deadline = steady_clock::now() + 200ms;   // 整個請求的預算
//       if (!m1.try_lock_until(deadline)) return Timeout;
//       if (!m2.try_lock_until(deadline)) return Timeout;   // 共用同一個截止時間
//   若這裡用 try_lock_for(200ms) 兩次，最壞會等 400ms，預算就爆了。
//   ⚠️ 並且務必用 steady_clock：system_clock 會被 NTP 校時往前往後跳。
//
// 【概念補充 Concept Deep Dive】
//   * 逾時【失敗】之後要做什麼，比逾時本身更重要。
//     回傳 false 卻沒有替代路徑，等於只是把「卡住」換成「靜默失敗」。
//     正確做法是：回報明確的錯誤（HTTP 503 / 熔斷器計數 +1）、
//     記錄告警，讓維運知道鎖競爭已經嚴重到影響 SLA。
//   * 逾時值的選擇是工程判斷：太短會在正常負載下誤判失敗，
//     太長則失去保護意義。實務上從 P99 延遲往上抓幾倍作為起點，
//     再依線上數據調整。
//   * 若發現需要頻繁設定鎖的逾時，通常代表【設計本身有問題】：
//     臨界區段太長、鎖的粒度太粗、或在鎖裡做了 I/O。
//     逾時是安全網，不是效能問題的解法。
//   * 本檔原始的 main 讓三條執行緒直接 cout，輸出順序每次都不同。
//     已改為收集結果後由主執行緒統一輸出，讓預期輸出可以驗證。
//
// 【注意事項 Pay Attention】
//   1. std::mutex 【沒有】try_lock_for/try_lock_until，
//      需要逾時必須改用 std::timed_mutex。
//   2. 手寫的輪詢式逾時延遲較差且浪費 CPU，能用 timed_mutex 就別自己寫。
//   3. try_lock_for 的實際等待時間一定【略大於】設定值
//      （sleep 只保證下限、還有排程延遲）。
//   4. 一個請求內取多把鎖時，用 try_lock_until 共用同一個截止時間，
//      而不是每把鎖各給一次 try_lock_for。
//   5. 時間基準請用 steady_clock，不要用 system_clock。
//   6. 逾時失敗【必須】有明確的替代路徑與可觀測性，不可靜默忽略。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】逾時鎖
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::mutex 有 try_lock_for() 嗎？沒有的話該用什麼？
//     答：沒有。std::mutex 只有 lock() / try_lock() / unlock()。
//         需要逾時要改用 std::timed_mutex（C++11），
//         它提供 try_lock_for()（相對時間）與 try_lock_until()（絕對時間點）。
//         另有 recursive_timed_mutex（C++11）與 shared_timed_mutex（C++14）。
//     追問：為什麼標準不直接把逾時加進 std::mutex？
//           → zero-overhead principle：逾時能力需要更複雜的實作路徑，
//             不該讓完全不需要它的使用者付出代價。
//
// 🔥 Q2. 用 try_lock() 加 sleep 迴圈自己實作逾時，跟 timed_mutex 差在哪？
//     答：手寫版本是【輪詢】：每隔一小段醒來試一次。
//         缺點是鎖若在兩次取樣之間被釋放就白等（延遲抖動）、
//         每次醒來都是一次 context switch（浪費 CPU）、
//         而且逾時值不精確。
//         timed_mutex 底層是帶逾時的 futex 等待：執行緒真的睡著，
//         鎖一釋放立刻被喚醒，延遲最低且不耗 CPU。
//     追問：那手寫版本什麼時候還有價值？
//           → 當每次重試之間需要做別的事時（檢查取消旗標、更新進度、
//             處理其他佇列），才需要自己控制迴圈。
//
// ⚠️ 陷阱. 一個請求要依序取得 m1、m2、m3 三把鎖，總預算 300ms。
//        寫成三次 try_lock_for(300ms) 對嗎？
//     答：不對。每次 try_lock_for 都是【從呼叫當下重新起算】300ms，
//         最壞情況會等 900ms，整整超出預算三倍。
//         正解是先算出一個絕對截止時間，三把鎖共用它：
//             auto deadline = std::chrono::steady_clock::now()
//                           + std::chrono::milliseconds(300);
//             if (!m1.try_lock_until(deadline)) return Timeout;
//             if (!m2.try_lock_until(deadline)) return Timeout;
//             if (!m3.try_lock_until(deadline)) return Timeout;
//     為什麼會錯：把「每一步的逾時」誤當成「整體的逾時」。
//         逾時預算是【整個請求】的屬性，不是單一操作的屬性——
//         這個錯誤在網路重試、RPC 呼叫鏈上同樣常見。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔【略過】，理由如下：
//   LeetCode 的並行題（1114 / 1115 / 1116 / 1117 / 1195）要求所有輸出
//   都必須完成，沒有「等太久就放棄」的語意；評測環境也沒有 SLA 概念。
//   逾時鎖的價值完全在於「可控的最壞延遲」，這在那些題目裡不存在，
//   硬掛一題會誤導使用時機，故從缺。

/*
# 第五階段：互斥鎖基礎 (std::mutex)

## 課程 5.3：try_lock() 非阻塞鎖定

---

### 引言

在前兩課中，我們學習了 `lock()` 會阻塞等待直到獲取鎖。但有時候，我們不希望執行緒傻傻地等待，而是想「試試看能不能拿到鎖，拿不到就做別的事」。這就是 `try_lock()` 的用途。

---

### 一、lock() vs try_lock()

```
┌─────────────────────────────────────────────────────────────┐
│                  lock() vs try_lock()                       │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  lock()                           try_lock()                │
│  ──────                           ──────────                │
│                                                             │
│  「我一定要拿到鎖」                「試試看能不能拿到」        │
│                                                             │
│  ┌─────────┐                     ┌─────────┐               │
│  │ lock()  │                     │try_lock│               │
│  └────┬────┘                     └────┬────┘               │
│       │                               │                     │
│       ▼                        ┌──────┴──────┐              │
│  ┌─────────┐                   │             │              │
│  │鎖可用？ │                 成功          失敗              │
│  └────┬────┘               (true)        (false)            │
│       │                       │             │               │
│  ┌────┴────┐                  ▼             ▼               │
│  │         │             進入臨界區段    立即返回             │
│ 是        否                            做其他事             │
│  │         │                                                │
│  ▼         ▼                                                │
│ 獲得鎖   阻塞等待                                            │
│         （可能很久）                                         │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

| 方法 | 行為 | 返回值 | 阻塞 |
|------|------|--------|------|
| `lock()` | 獲取鎖，失敗則等待 | 無（void） | 是 |
| `try_lock()` | 嘗試獲取鎖，失敗則立即返回 | `true`/`false` | 否 |
| `unlock()` | 釋放鎖 | 無（void） | 否 |

---

### 二、try_lock() 基本用法

```cpp
// 檔案：lesson_5_3_try_lock_basic.cpp
// 說明：try_lock() 的基本使用方式

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

std::mutex mtx;

void worker(int id) {
    // 嘗試獲取鎖
    if (mtx.try_lock()) {
        // 成功獲取鎖
        std::cout << "執行緒 " << id << "：成功獲取鎖，開始工作..." << std::endl;
        
        // 模擬工作
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::cout << "執行緒 " << id << "：工作完成，釋放鎖" << std::endl;
        mtx.unlock();  // 記得解鎖！
    } else {
        // 獲取失敗，立即返回
        std::cout << "執行緒 " << id << "：無法獲取鎖，執行其他任務" << std::endl;
    }
}

int main() {
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    std::thread t3(worker, 3);
    
    t1.join();
    t2.join();
    t3.join();
    
    return 0;
}
```

#### 編譯與執行

```bash
g++ -std=c++17 -pthread -o try_lock_basic lesson_5_3_try_lock_basic.cpp
./try_lock_basic
```

#### 可能的輸出

```
執行緒 1：成功獲取鎖，開始工作...
執行緒 2：無法獲取鎖，執行其他任務
執行緒 3：無法獲取鎖，執行其他任務
執行緒 1：工作完成，釋放鎖
```

---

### 三、重複嘗試模式

有時我們想重複嘗試，但不想無限等待：

```cpp
// 檔案：lesson_5_3_retry_pattern.cpp
// 說明：使用 try_lock() 實現有限次數的重試

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

std::mutex mtx;
int shared_data = 0;

bool tryUpdateWithRetry(int id, int maxRetries) {
    for (int attempt = 1; attempt <= maxRetries; ++attempt) {
        if (mtx.try_lock()) {
            // 成功獲取鎖
            std::cout << "執行緒 " << id << "：第 " << attempt 
                      << " 次嘗試成功" << std::endl;
            
            ++shared_data;
            
            // 模擬一些工作
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            mtx.unlock();
            return true;
        }
        
        std::cout << "執行緒 " << id << "：第 " << attempt 
                  << " 次嘗試失敗，稍後重試..." << std::endl;
        
        // 等待一小段時間再重試
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::cout << "執行緒 " << id << "：達到最大重試次數，放棄" << std::endl;
    return false;
}

int main() {
    std::thread t1(tryUpdateWithRetry, 1, 5);
    std::thread t2(tryUpdateWithRetry, 2, 5);
    
    t1.join();
    t2.join();
    
    std::cout << "最終 shared_data = " << shared_data << std::endl;
    
    return 0;
}
```

#### 可能的輸出

```
執行緒 1：第 1 次嘗試成功
執行緒 2：第 1 次嘗試失敗，稍後重試...
執行緒 2：第 2 次嘗試失敗，稍後重試...
執行緒 2：第 3 次嘗試失敗，稍後重試...
執行緒 2：第 4 次嘗試失敗，稍後重試...
執行緒 2：第 5 次嘗試失敗，稍後重試...
執行緒 1：（解鎖）
執行緒 2：第 5 次嘗試成功
最終 shared_data = 2
```

---

### 四、避免死結的應用

`try_lock()` 最重要的應用之一是**避免死結**。

#### 死結情境回顧

```
┌─────────────────────────────────────────────────────────────┐
│                      死結情境                                │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Thread A                      Thread B                     │
│  ─────────                     ─────────                    │
│  lock(mutex1);                 lock(mutex2);                │
│  lock(mutex2);  ← 等待 B       lock(mutex1);  ← 等待 A      │
│                                                             │
│              互相等待 → 死結！                               │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

#### 使用 try_lock() 避免死結

```cpp
// 檔案：lesson_5_3_avoid_deadlock.cpp
// 說明：使用 try_lock() 避免死結

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

std::mutex mutex1;
std::mutex mutex2;

void safeOperation(int id, std::mutex& first, std::mutex& second, 
                   const char* firstName, const char* secondName) {
    while (true) {
        // 先鎖定第一個互斥鎖
        first.lock();
        std::cout << "執行緒 " << id << "：獲得 " << firstName << std::endl;
        
        // 嘗試鎖定第二個互斥鎖
        if (second.try_lock()) {
            // 成功獲得兩個鎖
            std::cout << "執行緒 " << id << "：獲得 " << secondName 
                      << "，執行操作" << std::endl;
            
            // 執行需要兩個鎖的操作
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            // 釋放鎖（注意順序：後獲取的先釋放）
            second.unlock();
            first.unlock();
            
            std::cout << "執行緒 " << id << "：操作完成" << std::endl;
            return;  // 成功完成
        }
        
        // 無法獲得第二個鎖，釋放第一個鎖，避免死結
        std::cout << "執行緒 " << id << "：無法獲得 " << secondName 
                  << "，釋放 " << firstName << " 並重試" << std::endl;
        first.unlock();
        
        // 稍等一下再重試（避免活鎖）
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

int main() {
    // 兩個執行緒以不同順序請求鎖
    std::thread t1([](){ 
        safeOperation(1, mutex1, mutex2, "mutex1", "mutex2"); 
    });
    std::thread t2([](){ 
        safeOperation(2, mutex2, mutex1, "mutex2", "mutex1"); 
    });
    
    t1.join();
    t2.join();
    
    std::cout << "兩個執行緒都成功完成，沒有死結！" << std::endl;
    
    return 0;
}
```

#### 輸出

```
執行緒 1：獲得 mutex1
執行緒 2：獲得 mutex2
執行緒 1：無法獲得 mutex2，釋放 mutex1 並重試
執行緒 2：無法獲得 mutex1，釋放 mutex2 並重試
執行緒 1：獲得 mutex1
執行緒 1：獲得 mutex2，執行操作
執行緒 1：操作完成
執行緒 2：獲得 mutex2
執行緒 2：獲得 mutex1，執行操作
執行緒 2：操作完成
兩個執行緒都成功完成，沒有死結！
```

---

### 五、活鎖（Livelock）問題

```
┌─────────────────────────────────────────────────────────────┐
│                      活鎖警告！                              │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  使用 try_lock() 可能導致「活鎖」：                          │
│                                                             │
│    Thread A              Thread B                           │
│    ────────              ────────                           │
│    lock(m1)              lock(m2)                           │
│    try m2 失敗           try m1 失敗                         │
│    unlock(m1)            unlock(m2)                         │
│    lock(m1)              lock(m2)        ← 立刻重試          │
│    try m2 失敗           try m1 失敗     ← 又失敗            │
│    ...                   ...             ← 無限循環！        │
│                                                             │
│  解決方案：加入隨機延遲                                      │
│                                                             │
│    // 重試前等待隨機時間                                     │
│    std::this_thread::sleep_for(                             │
│        std::chrono::milliseconds(rand() % 10)               │
│    );                                                       │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

#### 避免活鎖的改進版本

```cpp
// 檔案：lesson_5_3_avoid_livelock.cpp
// 說明：使用隨機退避避免活鎖

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <random>

std::mutex mutex1;
std::mutex mutex2;

void robustOperation(int id) {
    // 隨機數產生器
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 10);
    
    int attempts = 0;
    const int maxAttempts = 100;
    
    while (attempts < maxAttempts) {
        ++attempts;
        
        mutex1.lock();
        
        if (mutex2.try_lock()) {
            // 成功獲得兩個鎖
            std::cout << "執行緒 " << id << "：成功（嘗試 " 
                      << attempts << " 次）" << std::endl;
            
            // 執行操作
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            mutex2.unlock();
            mutex1.unlock();
            return;
        }
        
        // 釋放並等待隨機時間（指數退避的簡化版）
        mutex1.unlock();
        
        int waitTime = dis(gen);  // 1-10 毫秒的隨機等待
        std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
    }
    
    std::cout << "執行緒 " << id << "：達到最大嘗試次數" << std::endl;
}

int main() {
    std::thread t1(robustOperation, 1);
    std::thread t2(robustOperation, 2);
    
    t1.join();
    t2.join();
    
    return 0;
}
```

---

### 六、實用場景：非阻塞資源存取

```cpp
// 檔案：lesson_5_3_non_blocking_cache.cpp
// 說明：非阻塞的快取存取模式

#include <iostream>
#include <thread>
#include <mutex>
#include <map>
#include <string>
#include <chrono>
#include <vector>

class NonBlockingCache {
private:
    std::map<std::string, std::string> cache;
    std::mutex mtx;
    
public:
    // 嘗試從快取獲取值（非阻塞）
    bool tryGet(const std::string& key, std::string& value) {
        if (mtx.try_lock()) {
            auto it = cache.find(key);
            if (it != cache.end()) {
                value = it->second;
                mtx.unlock();
                return true;
            }
            mtx.unlock();
        }
        // 無法獲取鎖或找不到 key
        return false;
    }
    
    // 嘗試設定值（非阻塞）
    bool trySet(const std::string& key, const std::string& value) {
        if (mtx.try_lock()) {
            cache[key] = value;
            mtx.unlock();
            return true;
        }
        return false;
    }
    
    // 阻塞版本（保證成功）
    void set(const std::string& key, const std::string& value) {
        mtx.lock();
        cache[key] = value;
        mtx.unlock();
    }
    
    std::string get(const std::string& key) {
        mtx.lock();
        std::string result = cache[key];
        mtx.unlock();
        return result;
    }
};

NonBlockingCache cache;

void fastReader(int id) {
    std::string value;
    
    for (int i = 0; i < 5; ++i) {
        if (cache.tryGet("data", value)) {
            std::cout << "Reader " << id << "：快速讀取成功 = " << value << std::endl;
        } else {
            std::cout << "Reader " << id << "：快取忙碌，使用預設值" << std::endl;
            // 使用預設值或其他邏輯
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void writer() {
    for (int i = 0; i < 5; ++i) {
        std::string value = "value_" + std::to_string(i);
        cache.set("data", value);
        std::cout << "Writer：寫入 " << value << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}

int main() {
    cache.set("data", "initial");
    
    std::thread w(writer);
    std::thread r1(fastReader, 1);
    std::thread r2(fastReader, 2);
    
    w.join();
    r1.join();
    r2.join();
    
    return 0;
}
```

---

### 七、try_lock() 的注意事項

```
┌─────────────────────────────────────────────────────────────┐
│                  try_lock() 注意事項                        │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. 成功時必須 unlock()                                     │
│     ─────────────────────                                   │
│     if (mtx.try_lock()) {                                   │
│         // 工作...                                          │
│         mtx.unlock();  // ← 不要忘記！                      │
│     }                                                       │
│                                                             │
│  2. 可能產生虛假失敗（Spurious Failure）                    │
│     ────────────────────────────────────                    │
│     即使鎖是空閒的，try_lock() 也可能返回 false             │
│     （實作相關，較少見）                                     │
│                                                             │
│  3. 不保證公平性                                            │
│     ──────────────                                          │
│     頻繁呼叫 try_lock() 的執行緒可能一直搶不到鎖            │
│                                                             │
│  4. 不適合替代 lock()                                       │
│     ─────────────────                                       │
│     如果一定要獲取鎖，就用 lock()                           │
│     try_lock() 是用於「可以不獲取」的場景                   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 八、try_lock() 與 RAII

手動管理 `try_lock()` 容易忘記 `unlock()`，可以搭配 `std::unique_lock` 使用：

```cpp
// 檔案：lesson_5_3_try_lock_raii.cpp
// 說明：使用 unique_lock 搭配 try_lock

#include <iostream>
#include <thread>
#include <mutex>

std::mutex mtx;
int counter = 0;

void safeIncrement(int id) {
    // std::try_to_lock 讓 unique_lock 使用 try_lock() 而非 lock()
    std::unique_lock<std::mutex> lock(mtx, std::try_to_lock);
    
    if (lock.owns_lock()) {
        // 成功獲取鎖
        ++counter;
        std::cout << "執行緒 " << id << "：成功遞增，counter = " 
                  << counter << std::endl;
        // lock 離開作用域時自動 unlock
    } else {
        // 獲取失敗
        std::cout << "執行緒 " << id << "：無法獲取鎖" << std::endl;
    }
    // 無論成功與否，都不需要手動 unlock
}

int main() {
    std::thread t1(safeIncrement, 1);
    std::thread t2(safeIncrement, 2);
    std::thread t3(safeIncrement, 3);
    
    t1.join();
    t2.join();
    t3.join();
    
    std::cout << "最終 counter = " << counter << std::endl;
    
    return 0;
}
```

---

### 九、何時使用 try_lock()

```
┌─────────────────────────────────────────────────────────────┐
│               try_lock() 適用場景                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ✓ 適合使用 try_lock() 的情況：                             │
│                                                             │
│    • 有替代方案時（如使用預設值、快取）                      │
│    • 避免死結時（嘗試多個鎖）                                │
│    • 實現超時機制時                                         │
│    • 輪詢式設計時（定期檢查資源）                            │
│    • 效能敏感且可接受偶爾失敗時                              │
│                                                             │
│  ✗ 不適合使用 try_lock() 的情況：                           │
│                                                             │
│    • 操作必須完成時                                         │
│    • 沒有合理的失敗處理邏輯時                                │
│    • 會導致活鎖時（頻繁重試）                                │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 十、完整範例：帶超時的鎖獲取

```cpp
// 檔案：lesson_5_3_try_lock_timeout.cpp
// 說明：實現自訂的超時鎖獲取

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

std::mutex mtx;

// 嘗試在指定時間內獲取鎖
bool tryLockFor(std::mutex& m, std::chrono::milliseconds timeout) {
    auto start = std::chrono::steady_clock::now();
    
    while (true) {
        if (m.try_lock()) {
            return true;  // 成功獲取
        }
        
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed >= timeout) {
            return false;  // 超時
        }
        
        // 短暫睡眠避免忙等待
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void worker(int id, int workTime) {
    std::cout << "執行緒 " << id << "：嘗試獲取鎖（超時 100ms）" << std::endl;
    
    if (tryLockFor(mtx, std::chrono::milliseconds(100))) {
        std::cout << "執行緒 " << id << "：獲得鎖，工作 " 
                  << workTime << "ms" << std::endl;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(workTime));
        
        mtx.unlock();
        std::cout << "執行緒 " << id << "：完成並釋放鎖" << std::endl;
    } else {
        std::cout << "執行緒 " << id << "：超時，放棄等待" << std::endl;
    }
}

int main() {
    // 執行緒 1 持有鎖 200ms，執行緒 2 和 3 只等待 100ms
    std::thread t1(worker, 1, 200);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    std::thread t2(worker, 2, 50);
    std::thread t3(worker, 3, 50);
    
    t1.join();
    t2.join();
    t3.join();
    
    return 0;
}
```

#### 輸出

```
執行緒 1：嘗試獲取鎖（超時 100ms）
執行緒 1：獲得鎖，工作 200ms
執行緒 2：嘗試獲取鎖（超時 100ms）
執行緒 3：嘗試獲取鎖（超時 100ms）
執行緒 2：超時，放棄等待
執行緒 3：超時，放棄等待
執行緒 1：完成並釋放鎖
```

---

### 十一、本課重點回顧

1. **try_lock() 是非阻塞的**：立即返回 `true`（成功）或 `false`（失敗）
2. **成功時必須 unlock()**：與 `lock()` 相同的責任
3. **可用於避免死結**：嘗試獲取多個鎖，失敗就釋放已持有的鎖
4. **注意活鎖問題**：加入隨機延遲避免無限重試
5. **搭配 std::unique_lock 使用更安全**：`std::try_to_lock` 標籤
6. **適用於有替代方案的場景**：不是用來替代 `lock()`

---

### 下一課預告

在 **課程 5.4：互斥鎖的常見錯誤** 中，我們將學習：
- 忘記解鎖的後果
- 重複鎖定的問題
- 在錯誤的執行緒解鎖
- 例外安全問題

這些都是實際開發中最常見的錯誤，學會避免它們非常重要！

---

準備好繼續嗎？
*/



// 檔案：lesson_5_3_try_lock_timeout.cpp
// 說明：實現自訂的超時鎖獲取

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

std::mutex mtx;

// 嘗試在指定時間內獲取鎖
bool tryLockFor(std::mutex& m, std::chrono::milliseconds timeout) {
    auto start = std::chrono::steady_clock::now();
    
    while (true) {
        if (m.try_lock()) {
            return true;  // 成功獲取
        }
        
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed >= timeout) {
            return false;  // 超時
        }
        
        // 短暫睡眠避免忙等待
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// 結果收集：直接在執行緒裡 cout 會交錯，順序每次不同，無法驗證
std::atomic<int> acquiredCount{0};
std::atomic<int> timedOutCount{0};

void worker(int id, int workTime) {
    (void)id;

    if (tryLockFor(mtx, std::chrono::milliseconds(100))) {
        std::this_thread::sleep_for(std::chrono::milliseconds(workTime));
        mtx.unlock();
        acquiredCount.fetch_add(1, std::memory_order_relaxed);
    } else {
        timedOutCount.fetch_add(1, std::memory_order_relaxed);
    }
}

// -----------------------------------------------------------------------------
// 【標準正解對照組】std::timed_mutex —— 同樣的語意，交給標準函式庫
//   差別：不輪詢、不浪費 context switch，鎖一釋放就立刻醒來。
// -----------------------------------------------------------------------------
std::timed_mutex tmtx;

std::atomic<int> tmAcquired{0};
std::atomic<int> tmTimedOut{0};

void timedWorker(int workTime) {
    if (tmtx.try_lock_for(std::chrono::milliseconds(100))) {
        std::this_thread::sleep_for(std::chrono::milliseconds(workTime));
        tmtx.unlock();
        tmAcquired.fetch_add(1, std::memory_order_relaxed);
    } else {
        tmTimedOut.fetch_add(1, std::memory_order_relaxed);
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例】請求層級的截止時間（deadline propagation）
//   情境：一個 API 請求的總預算是 150ms，處理過程中要依序取得
//         「使用者資料」與「帳務資料」兩把鎖。
//   ⚠️ 錯誤寫法：對兩把鎖各給 try_lock_for(150ms)
//              → 最壞會等 300ms，整整超出預算一倍。
//   ✅ 正確寫法：在請求開始時算出一個【絕對截止時間】，
//              兩把鎖共用它（try_lock_until）——無論在哪一步逾時，
//              整個請求的總耗時都不會超過預算。
//   這個「把 deadline 一路往下傳」的模式，在 gRPC / Go 的 context
//   等現代 RPC 框架裡是標準做法，不只適用於鎖。
// -----------------------------------------------------------------------------
enum class RequestResult { Ok, TimeoutOnUser, TimeoutOnBilling };

std::timed_mutex userMtx;
std::timed_mutex billingMtx;

RequestResult handleRequest(std::chrono::milliseconds budget) {
    // 整個請求共用同一個截止時間（用 steady_clock，不受系統校時影響）
    const auto deadline = std::chrono::steady_clock::now() + budget;

    if (!userMtx.try_lock_until(deadline)) {
        return RequestResult::TimeoutOnUser;
    }
    std::unique_lock<std::timed_mutex> userLock(userMtx, std::adopt_lock);

    if (!billingMtx.try_lock_until(deadline)) {   // 注意：同一個 deadline
        return RequestResult::TimeoutOnBilling;   // userLock 解構時自動解鎖
    }
    std::unique_lock<std::timed_mutex> billingLock(billingMtx, std::adopt_lock);

    // 兩把鎖都到手，做實際工作
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    return RequestResult::Ok;
}

int main() {
    std::cout << "=== 課程示範: 手寫 tryLockFor（輪詢式逾時）===" << std::endl;
    {
        // 執行緒 1 持有鎖 200ms，執行緒 2 和 3 只等待 100ms
        std::thread t1(worker, 1, 200);

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        std::thread t2(worker, 2, 50);
        std::thread t3(worker, 3, 50);

        t1.join();
        t2.join();
        t3.join();

        std::cout << "取得鎖的執行緒數: " << acquiredCount.load()
                  << "  (必須是 1——執行緒 1 先搶到並持有 200ms)" << std::endl;
        std::cout << "逾時放棄的執行緒數: " << timedOutCount.load()
                  << "  (必須是 2——它們只等 100ms，短於 200ms 的持有時間)"
                  << std::endl;
    }

    std::cout << "\n=== 標準正解: std::timed_mutex::try_lock_for ===" << std::endl;
    {
        std::thread t1(timedWorker, 200);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::thread t2(timedWorker, 50);
        std::thread t3(timedWorker, 50);

        t1.join();
        t2.join();
        t3.join();

        std::cout << "取得鎖的執行緒數: " << tmAcquired.load()
                  << "  (必須是 1，行為與手寫版相同)" << std::endl;
        std::cout << "逾時放棄的執行緒數: " << tmTimedOut.load()
                  << "  (必須是 2)" << std::endl;
        std::cout << "差別: timed_mutex 全程睡眠等待，不輪詢、不浪費 CPU，"
                     "且鎖一釋放就立刻醒來" << std::endl;
    }

    std::cout << "\n=== 兩種 mutex 的大小（實作定義）===" << std::endl;
    std::cout << "sizeof(std::mutex)       = " << sizeof(std::mutex) << " bytes" << std::endl;
    std::cout << "sizeof(std::timed_mutex) = " << sizeof(std::timed_mutex) << " bytes" << std::endl;
    std::cout << "註: 本機兩者相同純屬平台巧合，不可當通則；"
                 "標準把逾時能力分成獨立型別是為了 zero-overhead 原則" << std::endl;

    std::cout << "\n=== 日常實務: 請求層級的截止時間 ===" << std::endl;
    {
        // 情境 1：無競爭，兩把鎖都能立刻取得 → 成功
        {
            const auto t0 = std::chrono::steady_clock::now();
            const RequestResult r = handleRequest(std::chrono::milliseconds(150));
            const auto spent = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::steady_clock::now() - t0);
            std::cout << "無競爭時的結果: "
                      << (r == RequestResult::Ok ? "Ok" : "Timeout")
                      << "，耗時未超出 150ms 預算: "
                      << (spent.count() <= 150 ? "是" : "否") << std::endl;
        }

        // 情境 2：billingMtx 被別人長期持有 → 在第二把鎖逾時，
        //         但【整個請求】仍在預算內返回
        {
            billingMtx.lock();      // 模擬別的請求長期持有帳務鎖

            RequestResult result = RequestResult::Ok;
            long spentMs = 0;

            std::thread req([&result, &spentMs]() {
                const auto t0 = std::chrono::steady_clock::now();
                result = handleRequest(std::chrono::milliseconds(150));
                spentMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now() - t0).count();
            });

            req.join();
            billingMtx.unlock();

            std::cout << "帳務鎖被佔用時的結果: "
                      << (result == RequestResult::TimeoutOnBilling
                              ? "TimeoutOnBilling" : "其他")
                      << std::endl;
            std::cout << "整個請求仍在預算內返回: "
                      << (spentMs <= 200 ? "是" : "否")
                      << "  (若對每把鎖各給 150ms，最壞會拖到 300ms)" << std::endl;
        }
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.3：try_lock() 非阻塞鎖定7.cpp' -o try_lock_timeout

// -----------------------------------------------------------------------------
// 【輸出但書】
//   1. sizeof 的兩個數字是【實作定義】的（本機 x86-64 / glibc / libstdc++），
//      換平台或換標準函式庫實作都會不同。
//   2. 各段的「1 成功 / 2 逾時」是穩定的：持有者持有 200ms，
//      另外兩條只等 100ms，兩者差距夠大。
//   3. 實際耗時的毫秒數每次執行都不同（sleep 只保證下限、還有排程延遲），
//      故本檔只驗證「是否在預算內」這個布林條件，不列出實際毫秒數。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// === 課程示範: 手寫 tryLockFor（輪詢式逾時）===
// 取得鎖的執行緒數: 1  (必須是 1——執行緒 1 先搶到並持有 200ms)
// 逾時放棄的執行緒數: 2  (必須是 2——它們只等 100ms，短於 200ms 的持有時間)
//
// === 標準正解: std::timed_mutex::try_lock_for ===
// 取得鎖的執行緒數: 1  (必須是 1，行為與手寫版相同)
// 逾時放棄的執行緒數: 2  (必須是 2)
// 差別: timed_mutex 全程睡眠等待，不輪詢、不浪費 CPU，且鎖一釋放就立刻醒來
//
// === 兩種 mutex 的大小（實作定義）===
// sizeof(std::mutex)       = 40 bytes
// sizeof(std::timed_mutex) = 40 bytes
// 註: 本機兩者相同純屬平台巧合，不可當通則；標準把逾時能力分成獨立型別是為了 zero-overhead 原則
//
// === 日常實務: 請求層級的截止時間 ===
// 無競爭時的結果: Ok，耗時未超出 150ms 預算: 是
// 帳務鎖被佔用時的結果: TimeoutOnBilling
// 整個請求仍在預算內返回: 是  (若對每把鎖各給 150ms，最壞會拖到 300ms)
