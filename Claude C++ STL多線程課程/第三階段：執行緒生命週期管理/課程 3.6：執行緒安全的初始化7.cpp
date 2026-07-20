/*
# 第三階段：執行緒生命週期管理

## 課程 3.6：執行緒安全的初始化

---

### 引言

在多執行緒環境中，某些資源只應該被初始化一次（例如單例物件、全域配置）。本課學習如何確保初始化只發生一次，即使多個執行緒同時嘗試初始化。

---

### 一、問題：不安全的初始化

```cpp
#include <iostream>
#include <thread>

class Database {
public:
    Database() { std::cout << "Database 初始化" << std::endl; }
    void query() { std::cout << "查詢中..." << std::endl; }
};

Database* db = nullptr;

void initAndUse() {
    if (db == nullptr) {           // 執行緒 A 檢查
        db = new Database();       // 執行緒 A 和 B 都可能執行這行！
    }
    db->query();
}

int main() {
    std::thread t1(initAndUse);
    std::thread t2(initAndUse);
    
    t1.join();
    t2.join();
    
    delete db;
    return 0;
}
```

可能輸出：
```
Database 初始化
Database 初始化
查詢中...
查詢中...
```

Database 被初始化了兩次！這是競爭條件。

---

### 二、解決方案：std::call_once

```cpp
#include <iostream>
#include <thread>
#include <mutex>

class Database {
public:
    Database() { std::cout << "Database 初始化" << std::endl; }
    void query() { std::cout << "查詢中..." << std::endl; }
};

Database* db = nullptr;
std::once_flag initFlag;

void initDatabase() {
    db = new Database();
}

void initAndUse() {
    std::call_once(initFlag, initDatabase);
    db->query();
}

int main() {
    std::thread t1(initAndUse);
    std::thread t2(initAndUse);
    std::thread t3(initAndUse);
    
    t1.join();
    t2.join();
    t3.join();
    
    delete db;
    return 0;
}
```

輸出：
```
Database 初始化
查詢中...
查詢中...
查詢中...
```

不管多少執行緒，Database 只初始化一次。

---

### 三、call_once 工作原理

```
┌─────────────────────────────────────────────────────────────┐
│                std::call_once 機制                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  std::once_flag flag;                                       │
│  → 記錄是否已經執行過                                        │
│                                                             │
│  std::call_once(flag, func);                                │
│  → 第一個到達的執行緒執行 func                               │
│  → 其他執行緒等待直到 func 完成                              │
│  → 之後的呼叫直接跳過（flag 已設定）                         │
│                                                             │
│  如果 func 拋出例外：                                        │
│  → flag 不會被設定                                          │
│  → 下一個執行緒會再次嘗試執行                                │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 四、使用 Lambda

更簡潔的寫法：

```cpp
#include <iostream>
#include <thread>
#include <mutex>

class Database {
public:
    Database() { std::cout << "Database 初始化" << std::endl; }
    void query() { std::cout << "查詢中..." << std::endl; }
};

Database* db = nullptr;
std::once_flag initFlag;

void initAndUse() {
    std::call_once(initFlag, []() {
        db = new Database();
    });
    db->query();
}

int main() {
    std::thread t1(initAndUse);
    std::thread t2(initAndUse);
    
    t1.join();
    t2.join();
    
    delete db;
    return 0;
}
```

---

### 五、執行緒安全的單例模式

#### 方法一：使用 call_once

```cpp
#include <iostream>
#include <thread>
#include <mutex>

class Singleton {
    static Singleton* instance;
    static std::once_flag initFlag;
    
    Singleton() { std::cout << "Singleton 建立" << std::endl; }
    
public:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    
    static Singleton& getInstance() {
        std::call_once(initFlag, []() {
            instance = new Singleton();
        });
        return *instance;
    }
    
    void doSomething() { std::cout << "工作中" << std::endl; }
};

Singleton* Singleton::instance = nullptr;
std::once_flag Singleton::initFlag;

int main() {
    std::thread t1([]() { Singleton::getInstance().doSomething(); });
    std::thread t2([]() { Singleton::getInstance().doSomething(); });
    
    t1.join();
    t2.join();
    
    return 0;
}
```

---

#### 方法二：使用 static 區域變數（更簡潔，C++11 保證安全）

```cpp
#include <iostream>
#include <thread>

class Singleton {
    Singleton() { std::cout << "Singleton 建立" << std::endl; }
    
public:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    
    static Singleton& getInstance() {
        static Singleton instance;  // C++11 保證執行緒安全
        return instance;
    }
    
    void doSomething() { std::cout << "工作中" << std::endl; }
};

int main() {
    std::thread t1([]() { Singleton::getInstance().doSomething(); });
    std::thread t2([]() { Singleton::getInstance().doSomething(); });
    
    t1.join();
    t2.join();
    
    return 0;
}
```

C++11 標準保證：區域 static 變數的初始化是執行緒安全的。

---

### 六、兩種方法比較

```
┌─────────────────────────────────────────────────────────────┐
│            call_once vs static 區域變數                     │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  std::call_once                static 區域變數              │
│  ──────────────                ─────────────────            │
│  • 明確控制初始化時機          • 更簡潔                     │
│  • 可用於非建構函式的初始化    • 只能用於建構函式            │
│  • 需要 once_flag              • 不需要額外變數              │
│  • 較靈活                      • C++11 自動保證安全          │
│                                                             │
│  建議：單例優先用 static 區域變數                            │
│        複雜初始化用 call_once                               │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 七、call_once 與例外

如果初始化函式拋出例外，下一個執行緒會重新嘗試：

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>

std::once_flag flag;
std::atomic<int> attempt{0};

void mayFail() {
    int current = ++attempt;
    std::cout << "嘗試 #" << current << std::endl;
    
    if (current < 3) {
        throw std::runtime_error("初始化失敗");
    }
    
    std::cout << "初始化成功！" << std::endl;
}

void worker() {
    try {
        std::call_once(flag, mayFail);
        std::cout << "繼續執行" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "捕獲例外: " << e.what() << std::endl;
    }
}

int main() {
    std::thread t1(worker);
    std::thread t2(worker);
    std::thread t3(worker);
    std::thread t4(worker);
    
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    
    return 0;
}
```

可能輸出：
```
嘗試 #1
捕獲例外: 初始化失敗
嘗試 #2
捕獲例外: 初始化失敗
嘗試 #3
初始化成功！
繼續執行
繼續執行
```

---

### 八、類別成員的延遲初始化

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <memory>

class Service {
    mutable std::once_flag cacheFlag;
    mutable std::unique_ptr<std::string> cache;
    
    void initCache() const {
        std::cout << "初始化快取..." << std::endl;
        cache = std::make_unique<std::string>("快取資料");
    }
    
public:
    const std::string& getCache() const {
        std::call_once(cacheFlag, &Service::initCache, this);
        return *cache;
    }
};

int main() {
    Service service;
    
    std::thread t1([&]() { std::cout << service.getCache() << std::endl; });
    std::thread t2([&]() { std::cout << service.getCache() << std::endl; });
    
    t1.join();
    t2.join();
    
    return 0;
}
```

---

### 九、本課重點回顧

1. 多執行緒環境下的初始化可能發生競爭條件
2. `std::call_once` 確保函式只執行一次
3. `std::once_flag` 記錄是否已執行
4. 如果初始化拋出例外，下一個執行緒會重新嘗試
5. C++11 保證區域 static 變數初始化是執行緒安全的
6. 單例模式優先使用區域 static 變數實作

---

### 第三階段完成！

恭喜你完成了執行緒生命週期管理階段！你已經學會：

- ✅ RAII 執行緒管理
- ✅ 執行緒守衛類別設計
- ✅ std::jthread 與 stop_token
- ✅ 執行緒例外處理
- ✅ thread_local 儲存
- ✅ std::call_once 安全初始化

---

### 下一階段預告

**第四階段：共享資料與競爭條件** 將深入探討：
- 課程 4.1：共享資料的問題
- 課程 4.2：不變量與競爭條件
- 課程 4.3：臨界區段概念
- ...

---

準備好進入第四階段嗎？
*/

// =============================================================================
//  課程 3.6：執行緒安全的初始化7.cpp  —  類別成員的延遲初始化(本課總整理)
// =============================================================================
//
// 【主題資訊 Information】
//   簽名      : void call_once(std::once_flag& flag, Callable&& f, Args&&... args);
//   標準版本  : C++11
//   標頭檔    : <mutex>
//   本檔重點  : 把 once_flag 放進「類別成員」,做每個物件各自的延遲初始化
//   關鍵語法  : std::call_once(cacheFlag, &Service::initCache, this);
//               —— 成員函式指標 + 物件指標,是 INVOKE 的合法形式
//   關鍵限制  : 含 once_flag 的類別自動變成「不可複製、不可移動」
//
// 【詳細解釋 Explanation】
//
// 【1. 從「全域只做一次」到「每個物件各做一次」】
// 前面幾個範例的 once_flag 都是全域或 static 的 —— 保護的是
// 「整個程式只初始化一次」。本檔把 once_flag 變成「非 static 的成員變數」,
// 語意就完全不同了:每個 Service 物件各自有一個旗標,
// 因此每個物件的快取各自被初始化一次。
// 有 100 個 Service 物件,就會有 100 次初始化 —— 但每個物件只有一次,
// 而且面對多執行緒同時存取同一個物件時是安全的。
//
// 這是實務上非常常見的模式:每個連線物件、每個 session、每個資料表的
// handler,各自持有一份需要延遲建立的昂貴資源。
//
// 【2. 成員函式指標的呼叫形式】
//     std::call_once(cacheFlag, &Service::initCache, this);
// call_once 的第二個參數只要能被 INVOKE 就行,而「成員函式指標 + 物件指標」
// 正是 INVOKE 的合法形式之一,等價於 (this->*&Service::initCache)()。
// 也可以改用 lambda:
//     std::call_once(cacheFlag, [this]{ initCache(); });
// 兩者效果相同。lambda 版可讀性通常較好,成員指標版則少一層閉包。
//
// 【3. mutable 與 const 成員函式:邏輯常數性】
// getCache() 宣告為 const,代表「從呼叫者的角度看,這個物件沒有改變」——
// 但它內部確實修改了 cache 與 cacheFlag。這種「物理上改了、邏輯上沒改」
// 的情況,正是 mutable 的用途:
//     mutable std::once_flag cacheFlag;
//     mutable std::unique_ptr<std::string> cache;
// 快取是最典型的例子:查詢一個值不應該讓物件「變得不同」,
// 即使內部偷偷把結果存了起來。
// ⚠️ 但要注意:mutable 只是解除編譯器的 const 檢查,它不會讓操作變成
//    執行緒安全。這裡之所以安全,是因為 call_once 提供了同步,
//    而不是因為寫了 mutable。
//
// 【4. once_flag 讓類別失去複製與移動能力】
// std::once_flag 的複製建構子與複製賦值都被刪除了(而且它沒有移動建構子)。
// 因此任何「以值持有 once_flag」的類別,編譯器都無法產生預設的複製/移動函式 ——
// 整個類別自動變成不可複製、不可移動。
// 本檔用 static_assert 把這件事在編譯期驗證出來。
//
// 這個限制常常在意想不到的地方咬人:
//     std::vector<Service> services;
//     services.push_back(Service{});     // ✗ 編譯錯誤:無法複製/移動
// 解法有三:
//   (a) 改存 std::vector<std::unique_ptr<Service>>(最常見)
//   (b) 用 services.emplace_back() 就地建構 —— 但 vector 擴容時仍需搬移,
//       所以還要搭配 reserve() 才可行,脆弱,不建議
//   (c) 改用其他機制(如函式內 static、或自己用 atomic 寫)
//
// 【5. 各種做法的總整理】
//   函式內 static(magic static):單一全域物件的惰性初始化 → 首選,最簡潔,
//                                 而且保證解構。
//   全域/static once_flag       :要初始化的不只一個物件、或需要參數 → 用它。
//   成員 once_flag              :每個物件各自一份延遲資源 → 本檔的做法。
//                                 代價是類別不可複製/移動。
//   自己寫 atomic + 雙重檢查     :極端效能需求且你確定自己懂記憶體序才用。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 once_flag 不可複製是「正確」的設計
//   旗標代表「這件事做過了沒」。若允許複製,新物件會連同「已完成」的狀態
//   一起被複製過去 —— 但新物件的 cache 其實還是空的(或指向同一份資料),
//   結果就是「系統認為初始化過了,實際上沒有」。這正是課程 3.6/6 說的
//   最危險狀態。標準直接在型別層面禁止,讓這種錯誤根本寫不出來。
//
// (B) 這個模式的記憶體序保證
//   和全域版一樣:initCache() 的完成 happens-before 所有其他
//   call_once(cacheFlag, ...) 的返回。所以第二條執行緒從 getCache()
//   拿到的 *cache,保證是完整建構好的字串,不會看到半初始化的 unique_ptr。
//
// (C) 和「建構子裡直接初始化」的取捨
//   最簡單的做法當然是把 cache 直接在建構子裡填好,完全不需要 call_once。
//   選擇延遲初始化的理由只有一個:那份資源很貴,而且不一定會被用到。
//   若 Service 物件建立後幾乎一定會呼叫 getCache(),延遲初始化只是
//   徒增複雜度與每次呼叫的檢查成本 —— 直接在建構子做完更好。
//
// 【注意事項 Pay Attention】
// 1. 成員 once_flag ⇒ 每個物件各一次,不是全程式一次。別搞混。
// 2. 含 once_flag 的類別不可複製、不可移動,放不進 std::vector<T>。
// 3. mutable 只解除 const 檢查,不提供執行緒安全;安全來自 call_once。
// 4. 初始化函式拋例外 ⇒ 該物件的旗標不算完成,下次會重試(見第 6 個範例檔)。
// 5. call_once 只保護初始化;之後若有會修改狀態的操作,仍需自己同步。
// 6. 延遲初始化不是免費的,每次呼叫都要檢查一次。資源不貴就別用。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】類別成員的延遲初始化
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 把 once_flag 從全域變數改成「非 static 的類別成員」,語意有什麼不同?
//     答：完全不同。全域/static 的旗標保證「整個程式只初始化一次」;
//         非 static 的成員旗標則是「每個物件各自初始化一次」。
//         有 100 個物件就會有 100 次初始化,但每個物件只有一次,
//         且對同一物件的並行存取是安全的。
//     追問：那什麼時候該用哪一種?
//         → 資源是全程式共用的(設定、logger、連線池)用全域;
//           資源屬於個別物件的(每個 session 的快取、每個連線的緩衝區)
//           用成員旗標。
//
// 🔥 Q2. 為什麼含有 std::once_flag 成員的類別不能放進 std::vector?
//     答：once_flag 的複製建構子與複製賦值被刪除,也沒有移動建構子,
//         所以外層類別的複製/移動函式無法被隱式產生,整個類別變成
//         不可複製、不可移動。而 vector 的 push_back 與擴容都需要
//         複製或移動元素,因此編譯不過。
//     追問：實務上怎麼解?
//         → 最常見的是改存 std::vector<std::unique_ptr<Service>>,
//           讓容器搬移的是指標而不是物件本身。
//
// ⚠️ 陷阱. 「getCache() 是 const 成員函式,又用了 mutable,
//         所以它是 const 而且執行緒安全的。」哪裡錯了?
//     答：兩個概念被混在一起了。const 與 mutable 都只是編譯期的存取控制,
//         和執行緒安全完全無關 —— mutable 甚至是在「解除」限制。
//         這段程式碼之所以執行緒安全,唯一的原因是 std::call_once 提供了
//         互斥與記憶體序保證。把 call_once 換成
//         if (!cache) cache = make_unique<...>(); 一樣可以編譯,
//         const 與 mutable 都沒變,但它立刻就是資料競爭了。
//     為什麼會錯：把「const 代表唯讀,唯讀就沒有競爭」這個直覺,
//         套到了「內部其實會寫入」的 mutable 成員上。
//         const 成員函式若碰了 mutable 成員,就是在寫入 ——
//         而多執行緒同時寫入同一個位置,就需要同步,不管簽名上寫了什麼。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>

// 保護 std::cout,避免多執行緒輸出互相切開
std::mutex g_ioMutex;

void say(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_ioMutex);
    std::cout << msg << std::endl;
}

// -----------------------------------------------------------------------------
// 原始示範:每個 Service 物件各自延遲初始化自己的快取
// -----------------------------------------------------------------------------
class Service {
    mutable std::once_flag cacheFlag;
    mutable std::unique_ptr<std::string> cache;

    void initCache() const {
        say("  初始化快取...");
        cache = std::make_unique<std::string>("快取資料");
    }

public:
    const std::string& getCache() const {
        // 成員函式指標 + this,是 INVOKE 的合法形式
        std::call_once(cacheFlag, &Service::initCache, this);
        return *cache;
    }
};

// 【4】用編譯期斷言證明:含 once_flag 的類別不可複製、不可移動
static_assert(!std::is_copy_constructible_v<Service>,
              "含 once_flag 的類別不可複製");
static_assert(!std::is_move_constructible_v<Service>,
              "含 once_flag 的類別不可移動");

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 146. LRU Cache
//   題目：設計一個容量固定的 LRU(Least Recently Used)快取,
//         get(key) 與 put(key, value) 都必須是 O(1);
//         容量滿了要淘汰「最久沒被使用」的那一筆。
//   為什麼用到本主題：146 本身是單執行緒的資料結構設計題,
//         但這種快取在真實系統裡幾乎一定是「整個行程共用一份、
//         由多條 worker thread 同時存取、而且要延遲建立」的物件 ——
//         那正好就是本課的三個主題疊在一起:
//           (1) 用 magic static / call_once 做執行緒安全的延遲初始化;
//           (2) 快取本身的 get/put 會修改狀態,必須另外用 mutex 保護
//               (呼應本課反覆強調的「初始化安全 ≠ 使用安全」);
//           (3) 快取物件含有 mutex,同樣不可複製 —— 和 once_flag 一樣的限制。
//   複雜度：get 與 put 皆為 O(1);空間 O(capacity)。
// -----------------------------------------------------------------------------
class LRUCache {
    int capacity_;
    // list 存 (key, value),最前面是最近使用過的
    std::list<std::pair<int, int>> items_;
    // key → 該筆在 list 中的位置,讓 get/put 都能 O(1) 找到並搬移
    std::unordered_map<int, std::list<std::pair<int, int>>::iterator> index_;

public:
    explicit LRUCache(int capacity) : capacity_(capacity) {}

    int get(int key) {
        auto it = index_.find(key);
        if (it == index_.end()) return -1;
        // 命中 → 搬到最前面代表「最近使用」;splice 不會使 iterator 失效
        items_.splice(items_.begin(), items_, it->second);
        return it->second->second;
    }

    void put(int key, int value) {
        auto it = index_.find(key);
        if (it != index_.end()) {
            it->second->second = value;                        // 更新值
            items_.splice(items_.begin(), items_, it->second);  // 搬到最前
            return;
        }
        if (static_cast<int>(items_.size()) == capacity_) {
            // 淘汰最久沒用的那一筆(list 的最後一個)
            index_.erase(items_.back().first);
            items_.pop_back();
        }
        items_.emplace_front(key, value);
        index_[key] = items_.begin();
    }
};

// 把 146 的資料結構包成「執行緒安全 + 延遲初始化」的行程級共用快取
class SharedUserCache {
    mutable std::once_flag           initFlag_;
    mutable std::unique_ptr<LRUCache> cache_;
    mutable std::mutex               mtx_;   // 保護 get/put:初始化安全 ≠ 使用安全

    void init() const {
        say("    [cache] 建立容量 2 的 LRU 快取(只會出現一次)");
        cache_ = std::make_unique<LRUCache>(2);
    }

    LRUCache& raw() const {
        std::call_once(initFlag_, &SharedUserCache::init, this);
        return *cache_;
    }

public:
    void put(int k, int v) const {
        LRUCache& c = raw();                       // 初始化由 call_once 保護
        std::lock_guard<std::mutex> lock(mtx_);    // 使用由 mutex 保護
        c.put(k, v);
    }

    int get(int k) const {
        LRUCache& c = raw();
        std::lock_guard<std::mutex> lock(mtx_);
        return c.get(k);
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】每個資料庫連線各自延遲準備自己的 prepared statement
//   情境: 連線池裡有多個 Connection 物件,每個連線第一次執行某個查詢時,
//         才向伺服器準備(prepare)那份 SQL。這份 prepared statement
//         綁定在「那一條連線」上,不能跨連線共用 ——
//         所以旗標必須是「成員」而不是全域的。
//   為什麼用本主題: 這正是【詳細解釋 1】所說「每個物件各做一次」的
//                   典型真實案例,也說明了為什麼不能圖方便用 static 旗標。
// -----------------------------------------------------------------------------
class Connection {
    int                     id_;
    mutable std::once_flag  prepFlag_;
    mutable std::string     stmt_;

    void prepare() const {
        say("    [conn " + std::to_string(id_) + "] 準備 prepared statement");
        stmt_ = "PREPARED(SELECT * FROM users WHERE id=?)";
    }

public:
    explicit Connection(int id) : id_(id) {}

    const std::string& query() const {
        std::call_once(prepFlag_, &Connection::prepare, this);
        return stmt_;
    }

    int id() const { return id_; }
};

int main() {
    std::cout << "=== 原始示範:兩條執行緒存取同一個 Service ===" << std::endl;
    Service service;
    std::thread t1([&]() { say("  " + service.getCache()); });
    std::thread t2([&]() { say("  " + service.getCache()); });
    t1.join();
    t2.join();
    std::cout << "  ↑「初始化快取...」只出現一次" << std::endl;

    std::cout << "\n=== 成員旗標 = 每個物件各初始化一次 ===" << std::endl;
    Service a, b;
    a.getCache();
    b.getCache();
    std::cout << "  ↑ 兩個不同的 Service 物件 → 各自初始化一次(共兩行)"
              << std::endl;

    std::cout << "\n=== LeetCode 146. LRU Cache(單執行緒行為驗證) ==="
              << std::endl;
    LRUCache lru(2);
    lru.put(1, 1);
    lru.put(2, 2);
    std::cout << "  get(1) = " << lru.get(1) << "  (期望 1)" << std::endl;
    lru.put(3, 3);   // 容量滿 → 淘汰最久沒用的 key 2
    std::cout << "  get(2) = " << lru.get(2) << "  (期望 -1,已被淘汰)"
              << std::endl;
    lru.put(4, 4);   // 淘汰 key 1
    std::cout << "  get(1) = " << lru.get(1) << "  (期望 -1,已被淘汰)"
              << std::endl;
    std::cout << "  get(3) = " << lru.get(3) << "  (期望 3)" << std::endl;
    std::cout << "  get(4) = " << lru.get(4) << "  (期望 4)" << std::endl;

    std::cout << "\n=== 把 146 包成執行緒安全 + 延遲初始化的共用快取 ==="
              << std::endl;
    SharedUserCache shared;
    std::vector<std::thread> pool;
    for (int i = 0; i < 4; ++i) {
        pool.emplace_back([&shared, i]() {
            shared.put(i, i * 100);
            shared.get(i);
        });
    }
    for (std::thread& t : pool) t.join();
    std::cout << "  ↑ 4 條執行緒同時使用,「建立 LRU 快取」只出現一次"
              << std::endl;

    std::cout << "\n=== 實務:每條連線各自準備 prepared statement ==="
              << std::endl;
    std::vector<std::unique_ptr<Connection>> conns;   // 注意:必須存指標
    conns.push_back(std::make_unique<Connection>(1));
    conns.push_back(std::make_unique<Connection>(2));
    for (const std::unique_ptr<Connection>& c : conns) {
        c->query();
        c->query();   // 第二次不會再 prepare
    }
    std::cout << "  ↑ 兩條連線各 prepare 一次;每條連線的第二次查詢都沒有重複準備"
              << std::endl;
    std::cout << "  (conns 用 vector<unique_ptr<Connection>> 而非 vector<Connection>,"
                 "因為含 once_flag 的類別不可複製/移動)" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.6：執行緒安全的初始化7.cpp" -o once_member

// 注意:以下為某一次實際執行的結果。
//   * 多執行緒各段的行序每次執行都可能不同。
//   * 但關鍵不變量每次都成立:同一個 Service 的「初始化快取...」恰好一次、
//     兩個不同 Service 各一次、「[cache] 建立容量 2 的 LRU 快取」恰好一次、
//     每條 Connection 的 prepare 各恰好一次。
//   * LeetCode 146 那一段是單執行緒的,輸出完全確定。

// === 預期輸出 ===
// === 原始示範:兩條執行緒存取同一個 Service ===
//   初始化快取...
//   快取資料
//   快取資料
//   ↑「初始化快取...」只出現一次
//
// === 成員旗標 = 每個物件各初始化一次 ===
//   初始化快取...
//   初始化快取...
//   ↑ 兩個不同的 Service 物件 → 各自初始化一次(共兩行)
//
// === LeetCode 146. LRU Cache(單執行緒行為驗證) ===
//   get(1) = 1  (期望 1)
//   get(2) = -1  (期望 -1,已被淘汰)
//   get(1) = -1  (期望 -1,已被淘汰)
//   get(3) = 3  (期望 3)
//   get(4) = 4  (期望 4)
//
// === 把 146 包成執行緒安全 + 延遲初始化的共用快取 ===
//     [cache] 建立容量 2 的 LRU 快取(只會出現一次)
//   ↑ 4 條執行緒同時使用,「建立 LRU 快取」只出現一次
//
// === 實務:每條連線各自準備 prepared statement ===
//     [conn 1] 準備 prepared statement
//     [conn 2] 準備 prepared statement
//   ↑ 兩條連線各 prepare 一次;每條連線的第二次查詢都沒有重複準備
//   (conns 用 vector<unique_ptr<Connection>> 而非 vector<Connection>,因為含 once_flag 的類別不可複製/移動)
