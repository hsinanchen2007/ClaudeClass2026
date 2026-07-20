// =============================================================================
//  課程 3.3：std::jthread (C++20) — 第 7 部分：綜合講義與可取消的工作執行緒池
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<thread>（std::jthread）、<stop_token>（stop_token/stop_source/stop_callback）
//   標準版本：C++20（P0660R10）
//
//     void worker(std::stop_token stoken, int id);   // 第一個參數自動接收 token
//     std::vector<std::jthread> workers;             // jthread 可移動 → 可放進容器
//     workers.emplace_back(worker, id);              // token 由 jthread 自動注入
//
//   複雜度：建立 N 條執行緒為 O(N) 次系統呼叫；
//           vector 解構時逐一 join，總耗時取決於最慢的那條執行緒。
//
// 【詳細解釋 Explanation】
//   本檔是本課的綜合講義（完整教材見下方 markdown 區塊），
//   這裡先補上三個「講義沒有明說、但實際寫程式一定會踩到」的重點。
//
// 【1. 為什麼 jthread 可以放進 std::vector，std::thread 也可以？】
//   兩者都是【可移動但不可複製】。std::vector 對元素的要求是
//   MoveInsertable（而非 CopyInsertable），所以兩者都合法。
//   關鍵在於 emplace_back 觸發擴容時，vector 需要把既有元素搬到新緩衝區：
//     * 若移動建構子標了 noexcept → 用移動，成本低且強例外保證仍成立；
//     * 若沒標 noexcept → vector 會退回用【複製】以維持強例外保證，
//       但 jthread 不可複製 → 編譯失敗。
//   std::jthread 的移動建構子標準規定為 noexcept，所以一切正常。
//   這是「為什麼移動建構子一定要標 noexcept」最實際的例子。
//
// 【2. vector 解構的順序，以及它的隱含代價】
//   ~vector 會【由後往前】逐一解構元素。所以 workers[2] 先 join，
//   然後 workers[1]，最後 workers[0] —— 而且是【序列化】的。
//   若每條執行緒收到停止請求後還要收尾 100ms，三條就是逐一等待。
//   本檔先對【全部】執行緒 request_stop() 再讓 vector 解構，
//   正是為了讓三條執行緒【並行收尾】，總時間才不會累加。
//   若寫成「stop 一條、join 一條」的迴圈，總時間會變成三倍。
//   這是實務上關閉執行緒池的黃金準則：【先全部通知，再全部等待】。
//
// 【3. 為什麼 worker 的簽名是 (stop_token, int) 而不是 (int, stop_token)】
//   jthread 只會把 token 注入到【第一個參數】。
//   若寫成 worker(int id, std::stop_token st) 並呼叫 emplace_back(worker, i)，
//   編譯器會嘗試用 (stop_token, int) 呼叫它 → 型別不合 → 編譯失敗。
//   規則很簡單：要用自動注入，stop_token 必須是第一個參數。
//
// 【概念補充 Concept Deep Dive】
//   * 每條 jthread 有【自己的】stop_source。所以本檔必須用迴圈逐一
//     request_stop()，沒辦法一次全停。若要「一個開關關全部」，
//     應改用自建的 std::stop_source 並把同一個 token 分發下去
//     （做法見本課第 4 部分的 WorkerPool 實務範例）。
//
//   * worker 印出的 count 值是【實作與排程相關】的：迴圈每圈 sleep 100ms，
//     跑約 1 秒，所以大約是 10，但實際值受排程延遲、輸出鎖競爭影響，
//     每次執行都不同，且三條執行緒之間也不保證相同。
//
//   * std::cout 的執行緒安全性：C++11 起標準保證對 std::cout 的
//     格式化輸出不會造成 data race（[iostream.objects] 有同步要求），
//     但【不保證原子性】—— 多條執行緒同時 << 時，輸出仍可能【交錯】。
//     本檔的輸出順序因此是非決定性的。要保證整行不被打斷，
//     必須自己加 mutex，或先組成單一字串再一次輸出。
//
// 【注意事項 Pay Attention】
//   1. stop_token 必須是 callable 的【第一個參數】才會被自動注入。
//   2. 關閉執行緒池請「先全部 request_stop()，再讓它們一起被 join」，
//      不要 stop 一條 join 一條，否則收尾時間會累加。
//   3. 各執行緒的 count 值與輸出交錯順序【每次執行都不同】，
//      不要在測試中斷言固定值。
//   4. std::cout 保證無 data race，但【不保證】整行輸出的原子性。
//   5. 本檔必須用 -std=c++20（已用 -pedantic-errors 實測驗證）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】jthread 執行緒池與關機順序
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::jthread 不可複製，為什麼還能放進 std::vector？
//     答：vector 對元素的要求是 MoveInsertable，不是 CopyInsertable，
//         所以「可移動但不可複製」的型別完全合法。關鍵在於擴容時：
//         若移動建構子標了 noexcept，vector 會用移動；沒標的話會退回用
//         複製以維持強例外保證，而 jthread 不可複製就會編譯失敗。
//         標準規定 jthread 的移動建構子是 noexcept，所以沒問題。
//     追問：這對自己寫的 RAII 執行緒類別有什麼啟示？
//         → 移動建構子與移動賦值一定要標 noexcept，否則放進 vector 時
//           會在擴容路徑上編譯失敗（或悄悄退化成複製）。
//
// 🔥 Q2. 關閉一個有 N 條執行緒的池子，為什麼不能寫成「stop 一條、join 一條」？
//     答：因為收尾時間會【累加】。每條執行緒收到停止請求後可能還要花時間
//         把手上的工作做完，逐一處理的話總時間是 N 倍。
//         正確做法是【先對全部 request_stop()，再統一等待】，
//         讓所有執行緒的收尾工作並行進行，總時間約等於最慢的那一條。
//     追問：本檔的 vector 是怎麼做到的？
//         → main 先用迴圈對全部 request_stop()，之後才讓 vector 解構
//           （由後往前逐一 join）。通知階段與等待階段是分開的。
//
// ⚠️ 陷阱. 把 worker 的簽名寫成 worker(int id, std::stop_token st)，
//         然後 emplace_back(worker, i)，為什麼編譯失敗？
//     答：jthread 只會把 token 注入到【第一個參數】。編譯器判斷
//         worker 是否可用 (stop_token, int) 呼叫 —— 不行，於是退而嘗試
//         用 (int) 呼叫 —— 參數個數也不對，兩條路都不通，編譯失敗。
//     為什麼會錯：直覺上以為 jthread 會「找到」參數列中的 stop_token 並填進去，
//         但它做的只是最單純的編譯期判斷：能不能把 token 當第一個引數。
//         沒有任何參數比對或重排的魔法。
// ═══════════════════════════════════════════════════════════════════════════

/*
# 第三階段：執行緒生命週期管理

## 課程 3.3：std::jthread (C++20)

---

### 引言

C++20 引入了 `std::jthread`（joining thread），它是 `std::thread` 的改良版本，內建 RAII 自動 join 和協作式取消機制。

---

### 一、std::jthread vs std::thread

```
┌─────────────────────────────────────────────────────────────┐
│            std::thread vs std::jthread                      │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  std::thread                    std::jthread                │
│  ────────────                   ─────────────               │
│  • 必須手動 join/detach         • 解構時自動 join            │
│  • 忘記會導致 terminate         • 不會忘記                   │
│  • 無取消機制                   • 內建 stop_token 取消機制   │
│  • C++11 起可用                 • C++20 起可用               │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 二、基本用法

```cpp
#include <iostream>
#include <thread>

int main() {
    std::jthread jt([]() {
        std::cout << "Hello from jthread!" << std::endl;
    });
    
    // 不需要呼叫 join()！
    // 離開作用域時自動 join
    
    return 0;
}
```

編譯（需要 C++20）：
```bash
g++ -std=c++20 -pthread -o jthread_demo jthread_demo.cpp
```

---

### 三、自動 join 的安全性

即使發生例外，也能正確處理：

```cpp
#include <iostream>
#include <thread>
#include <stdexcept>

void riskyFunction() {
    std::jthread jt([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "執行緒完成" << std::endl;
    });
    
    throw std::runtime_error("發生錯誤！");
    
    // 不需要擔心！jt 解構時會自動 join
}

int main() {
    try {
        riskyFunction();
    } catch (const std::exception& e) {
        std::cout << "捕獲: " << e.what() << std::endl;
    }
    
    return 0;
}
```

輸出：
```
執行緒完成
捕獲: 發生錯誤！
```

---

### 四、stop_token 取消機制

`std::jthread` 最強大的功能是內建的協作式取消：

```cpp
#include <iostream>
#include <thread>

int main() {
    std::jthread jt([](std::stop_token stoken) {
        int count = 0;
        while (!stoken.stop_requested()) {
            std::cout << "工作中... " << ++count << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        std::cout << "收到停止請求，結束" << std::endl;
    });
    
    // 讓執行緒跑一下
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // 請求停止
    jt.request_stop();
    
    // jt 解構時會等待執行緒結束
    return 0;
}
```

輸出：
```
工作中... 1
工作中... 2
工作中... 3
工作中... 4
工作中... 5
收到停止請求，結束
```

---

### 五、stop_token 詳解

```
┌─────────────────────────────────────────────────────────────┐
│                   stop_token 機制                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  std::stop_source   →  發出停止請求的來源                    │
│  std::stop_token    →  檢查是否有停止請求                    │
│  std::stop_callback →  停止時自動執行的回調                  │
│                                                             │
│  jthread 內部自動管理 stop_source                            │
│  執行緒函式可接收 stop_token 作為第一個參數                   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 六、手動取得 stop_source 和 stop_token

```cpp
#include <iostream>
#include <thread>

int main() {
    std::jthread jt([](std::stop_token stoken) {
        while (!stoken.stop_requested()) {
            std::cout << "Running..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    });
    
    // 取得 stop_source
    std::stop_source& source = jt.get_stop_source();
    
    // 取得 stop_token
    std::stop_token token = jt.get_stop_token();
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // 兩種方式都可以請求停止
    // source.request_stop();
    jt.request_stop();
    
    return 0;
}
```

---

### 七、stop_callback 自動回調

當停止被請求時，自動執行回調：

```cpp
#include <iostream>
#include <thread>

int main() {
    std::jthread jt([](std::stop_token stoken) {
        // 註冊停止時的回調
        std::stop_callback callback(stoken, []() {
            std::cout << "停止回調被觸發！" << std::endl;
        });
        
        while (!stoken.stop_requested()) {
            std::cout << "工作中..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
        
        std::cout << "執行緒結束" << std::endl;
    });
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    jt.request_stop();
    
    return 0;
}
```

輸出：
```
工作中...
工作中...
工作中...
停止回調被觸發！
執行緒結束
```

---

### 八、jthread 的完整介面

```cpp
class jthread {
public:
    // 建構
    jthread() noexcept;
    
    template<typename F, typename... Args>
    explicit jthread(F&& f, Args&&... args);
    
    // 解構：自動 request_stop() + join()
    ~jthread();
    
    // 移動（不可複製）
    jthread(jthread&&) noexcept;
    jthread& operator=(jthread&&) noexcept;
    
    // 停止機制
    stop_source get_stop_source() noexcept;
    stop_token get_stop_token() const noexcept;
    bool request_stop() noexcept;
    
    // 與 std::thread 相同的介面
    bool joinable() const noexcept;
    void join();
    void detach();
    id get_id() const noexcept;
    native_handle_type native_handle();
    
    static unsigned int hardware_concurrency() noexcept;
};
```

---

### 九、何時用 jthread vs thread

| 情況 | 選擇 |
|------|------|
| C++20 可用 | 優先使用 `jthread` |
| 需要取消機制 | 使用 `jthread` |
| 需要 detach | 使用 `thread` |
| 舊專案相容性 | 使用 `thread` |

---

### 十、完整範例：可取消的工作執行緒

```cpp
#include <iostream>
#include <thread>
#include <vector>

void worker(std::stop_token stoken, int id) {
    std::cout << "Worker " << id << " 啟動" << std::endl;
    
    int count = 0;
    while (!stoken.stop_requested()) {
        ++count;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "Worker " << id << " 結束，執行了 " << count << " 次" << std::endl;
}

int main() {
    std::vector<std::jthread> workers;
    
    // 建立 3 個工作執行緒
    for (int i = 0; i < 3; ++i) {
        workers.emplace_back(worker, i);
    }
    
    std::cout << "主執行緒等待 1 秒..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    std::cout << "請求所有執行緒停止" << std::endl;
    for (auto& w : workers) {
        w.request_stop();
    }
    
    // vector 解構時，每個 jthread 自動 join
    return 0;
}
```

---

### 十一、本課重點回顧

1. `std::jthread` 是 C++20 新增的執行緒類別
2. **自動 join**：解構時自動等待執行緒結束
3. **stop_token**：內建協作式取消機制
4. 執行緒函式可接收 `std::stop_token` 作為第一個參數
5. `request_stop()` 請求執行緒停止
6. `stop_callback` 可在停止時自動執行回調
7. 優先使用 `jthread`，除非需要 detach 或舊版相容

---

### 下一課預告

在 **課程 3.4：執行緒例外處理** 中，我們將學習：
- 執行緒中的例外如何處理
- 例外如何在執行緒間傳遞
- 安全的錯誤處理模式

---

準備好繼續嗎？
*/

// 【LeetCode 實戰範例】與【日常實務範例】見下方程式碼區塊。
// 本檔選用 LeetCode 1195 Fizz Buzz Multithreaded：它是「多條執行緒協調共用進度」
// 的最小完整範例，與本檔的工作執行緒池主題直接對應。

#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <stop_token>
#include <string>
#include <thread>
#include <utility>
#include <vector>

void worker(std::stop_token stoken, int id) {
    std::cout << "Worker " << id << " 啟動" << std::endl;

    int count = 0;
    while (!stoken.stop_requested()) {
        ++count;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Worker " << id << " 結束，執行了 " << count << " 次" << std::endl;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1195. Fizz Buzz Multithreaded
//   題目：四條執行緒共同輸出 1..n 的 FizzBuzz —— fizz 只印 "fizz"（3 的倍數）、
//         buzz 只印 "buzz"（5 的倍數）、fizzbuzz 只印 "fizzbuzz"（15 的倍數）、
//         number 只印其餘數字。四者必須讓最終輸出【嚴格按 1..n 的順序】。
//   為什麼用到本主題：這是「多條執行緒協調共用進度」的最小完整範例，
//         正好對應本檔的工作執行緒池主題。四條 worker 共享計數器 current_，
//         每條只在「輪到自己」時動作，其餘時間阻塞在 condition_variable 上。
//         這裡刻意用 std::jthread 建立四條執行緒：不必寫四次 join，
//         離開作用域自動回收；中途拋例外也一樣被安全 join。
//         真正提交到 LeetCode 時只需要 FizzBuzz class（框架會自己開執行緒）。
//   同步策略：單一 mutex + 單一 condition_variable + 述詞判斷。
//         述詞為「current_ > n（該收工了）或 輪到我了」，
//         用述詞版的 wait 可同時擋掉虛假喚醒（spurious wakeup）。
// -----------------------------------------------------------------------------
class FizzBuzz {
public:
    explicit FizzBuzz(int n) : n_(n) {}

    void fizz(const std::function<void()>& printFizz) {
        loop(printFizz, [](int i) { return i % 3 == 0 && i % 5 != 0; });
    }

    void buzz(const std::function<void()>& printBuzz) {
        loop(printBuzz, [](int i) { return i % 5 == 0 && i % 3 != 0; });
    }

    void fizzbuzz(const std::function<void()>& printFizzBuzz) {
        loop(printFizzBuzz, [](int i) { return i % 15 == 0; });
    }

    void number(const std::function<void(int)>& printNumber) {
        while (true) {
            std::unique_lock<std::mutex> lock(mtx_);
            cv_.wait(lock, [&] {
                return current_ > n_ || (current_ % 3 != 0 && current_ % 5 != 0);
            });
            if (current_ > n_) return;
            printNumber(current_);
            ++current_;
            cv_.notify_all();
        }
    }

private:
    // 共用的等待迴圈：符合 pred 時才輸出，然後推進計數器並喚醒其他人
    template <typename Pred>
    void loop(const std::function<void()>& print, Pred pred) {
        while (true) {
            std::unique_lock<std::mutex> lock(mtx_);
            cv_.wait(lock, [&] { return current_ > n_ || pred(current_); });
            if (current_ > n_) return;
            print();
            ++current_;
            cv_.notify_all();
        }
    }

    int n_;
    int current_ = 1;
    std::mutex mtx_;
    std::condition_variable cv_;
};

void demoFizzBuzz(int n) {
    FizzBuzz fb(n);
    std::string out;
    std::mutex outMtx;

    auto append = [&](const std::string& s) {
        std::lock_guard<std::mutex> lock(outMtx);
        out += s;
        out += ' ';
    };

    {
        // 用 jthread：四條執行緒離開作用域時自動 join，不必寫四次 join()
        std::jthread t1([&] { fb.fizz([&] { append("fizz"); }); });
        std::jthread t2([&] { fb.buzz([&] { append("buzz"); }); });
        std::jthread t3([&] { fb.fizzbuzz([&] { append("fizzbuzz"); }); });
        std::jthread t4([&] {
            fb.number([&](int i) { append(std::to_string(i)); });
        });
    }  // 四條 jthread 依序解構 → 全部 join 完成

    std::cout << "  n = " << n << " → " << out << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】固定大小的執行緒池（thread pool）
//   情境：Web 服務把上傳的圖片丟進背景佇列做縮圖。不能每張圖都開一條新執行緒
//         （建立/銷毀成本高、也無法限制並行度），而是預先開好固定數量的 worker
//         重複取用任務。這是伺服器端最常見的並行結構。
//   為何用 jthread + stop_token：
//     * 池子解構時每條 worker 自動 request_stop() + join()，不必手寫 shutdown()；
//     * stop_callback 把 notify_all() 綁在停止事件上，讓阻塞在 cv.wait() 的
//       閒置 worker 能被立刻叫醒（否則會永久卡住，解構子就死結了）；
//     * 停止後仍會把佇列中【已排入】的任務做完才退出（graceful drain），
//       這是生產環境的必要行為 —— 不能把已經接受的使用者請求丟掉。
// -----------------------------------------------------------------------------
class ThreadPool {
public:
    explicit ThreadPool(unsigned n) {
        for (unsigned i = 0; i < n; ++i) {
            workers_.emplace_back([this, i](std::stop_token st) { run(st, i); });
        }
    }

    void submit(std::string name, std::function<void()> job) {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            jobs_.emplace(std::move(name), std::move(job));
        }
        cv_.notify_one();
    }

private:
    // std::cout 保證無 data race，但【不保證整行輸出的原子性】。
    // 多條 worker 同時 << 時，輸出會像這樣被撕裂：
    //     [worker   [worker 1] 執行: thumbnail-2
    //   0] 執行: thumbnail-1
    // （這是實測到的真實輸出。）所以正式程式必須自己用鎖把「一行」框起來，
    // 或先組成單一字串再一次輸出。這裡示範前者。
    void log(const std::string& msg) {
        std::lock_guard<std::mutex> lock(coutMtx_);
        std::cout << msg << std::endl;
    }

    void run(std::stop_token st, unsigned id) {
        // 關鍵：停止時把所有閒置 worker 叫醒，否則它們會卡在 cv.wait()
        std::stop_callback wake(st, [this] { cv_.notify_all(); });

        while (true) {
            std::pair<std::string, std::function<void()>> job;
            {
                std::unique_lock<std::mutex> lock(mtx_);
                cv_.wait(lock, [&] { return !jobs_.empty() || st.stop_requested(); });

                // graceful drain：即使收到停止請求，也要先把已排入的任務做完
                if (jobs_.empty()) {
                    lock.unlock();
                    log("  [worker " + std::to_string(id) + "] 佇列已清空，退出");
                    return;
                }
                job = std::move(jobs_.front());
                jobs_.pop();
            }
            log("  [worker " + std::to_string(id) + "] 執行: " + job.first);
            job.second();
        }
    }

    std::mutex mtx_;
    std::mutex coutMtx_;      // 專門保護 std::cout，避免輸出被撕裂
    std::condition_variable cv_;
    std::queue<std::pair<std::string, std::function<void()>>> jobs_;
    std::vector<std::jthread> workers_;   // ← 宣告在最後：解構時最先被 join
};

int main() {
    std::cout << "=== 講義範例：可取消的工作執行緒池 ===" << std::endl;
    {
        std::vector<std::jthread> workers;

        // 建立 3 個工作執行緒
        for (int i = 0; i < 3; ++i) {
            workers.emplace_back(worker, i);
        }

        std::cout << "主執行緒等待 1 秒..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::cout << "請求所有執行緒停止" << std::endl;
        // 【先全部通知】—— 讓三條執行緒並行收尾，總時間不累加
        for (auto& w : workers) {
            w.request_stop();
        }
        // 【再統一等待】—— vector 解構時由後往前逐一 join
    }

    std::cout << "\n=== LeetCode 1195. Fizz Buzz Multithreaded ===" << std::endl;
    demoFizzBuzz(15);

    std::cout << "\n=== 日常實務：固定大小執行緒池（縮圖任務） ===" << std::endl;
    {
        ThreadPool pool(2);
        for (int i = 1; i <= 4; ++i) {
            pool.submit("thumbnail-" + std::to_string(i), [] {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            });
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::cout << "  main: 服務關閉，pool 即將解構" << std::endl;
    }  // pool 解構 → 每條 worker request_stop() + join()
    std::cout << "  main: 執行緒池已完全回收" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra -pthread "課程 3.3：std jthread (C++20)7.cpp" -o jthread7
//   注意：本檔【必須】用 -std=c++20（std::jthread / std::stop_token / std::stop_callback
//   為 C++20 新增，已用 -std=c++17 -pedantic-errors 實測確認會編譯失敗）。

// 註:
//   ⚠️ 第一段（講義範例）刻意保留原始寫法：worker 直接對 std::cout 輸出而【不加鎖】。
//   std::cout 保證不會有 data race，但【不保證整行的原子性】，
//   所以這一段的行序會變、甚至同一行會被撕裂。以下是實測到的真實撕裂輸出：
//   Worker 0 啟動
//   Worker 1 啟動主執行緒等待 1 秒...
//   Worker 2 啟動
//   這【不是 bug，也不是未定義行為】，而是缺少輸出鎖的必然結果。
//   第三段（執行緒池）示範了正確做法：用 coutMtx_ 把「一行」框起來，輸出就穩定了。
//   各 worker 的 count 值受排程影響，本機穩定為 10，但不保證在其他機器相同。

// === 預期輸出 ===
// === 講義範例：可取消的工作執行緒池 ===
// 主執行緒等待 1 秒...
// Worker 0 啟動
// Worker 1 啟動
// Worker 2 啟動
// 請求所有執行緒停止
// Worker 1Worker 0 結束，執行了 10 次
//  結束，執行了 10 次
// Worker 2 結束，執行了 10 次
//
// === LeetCode 1195. Fizz Buzz Multithreaded ===
//   n = 15 → 1 2 fizz 4 buzz fizz 7 8 fizz buzz 11 fizz 13 14 fizzbuzz
//
// === 日常實務：固定大小執行緒池（縮圖任務） ===
//   [worker 0] 執行: thumbnail-1
//   [worker 1] 執行: thumbnail-2
//   [worker 1] 執行: thumbnail-3
//   [worker 0] 執行: thumbnail-4
//   main: 服務關閉，pool 即將解構
//   [worker 0] 佇列已清空，退出
//   [worker 1] 佇列已清空，退出
//   main: 執行緒池已完全回收
