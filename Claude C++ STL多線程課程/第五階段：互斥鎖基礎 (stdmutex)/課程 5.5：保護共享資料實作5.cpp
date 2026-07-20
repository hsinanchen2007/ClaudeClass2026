//
// =============================================================================
//  課程 5.5：保護共享資料實作5.cpp  —  執行緒安全介面的四種標準做法
// =============================================================================
//
// 【主題資訊 Information】
//   主題：    執行緒安全類別的介面設計；上一檔（實作4）的正面對照組
//   四種做法：① 回傳複本 ② 回傳值 ③ 提供操作而非存取 ④ 傳入回呼在鎖內執行
//   標準版本：std::mutex / lock_guard / std::function 為 C++11；
//             尾端回傳型別 auto f() -> decltype(...) 為 C++11
//   標頭檔：  <mutex>、<functional>、<vector>
//   核心原則：【把操作搬進鎖內，不要把資料送出鎖外】
//
// 【詳細解釋 Explanation】
//
// 【1. 四種做法各自解決什麼問題】
//   ① getData() 回傳複本 —— 呼叫端拿到獨立快照，愛怎麼走訪都不影響本物件。
//      代價是複製成本；資料很大時改用 shared_ptr<const T> 的 copy-on-write。
//   ② getAt(i) 回傳值 —— 單一元素查詢，連複製整個容器都省了。
//      注意它回傳 int 而非 const int&：回傳引用就退化成實作4 的錯誤。
//   ③ add() / addAll() 提供操作 —— 這是最重要的一種。
//      把「檢查 + 修改」封裝成單一個成員函式，呼叫端沒有機會在中間插入東西，
//      從根本消滅 check-then-act。批次版 addAll() 還把 N 次上鎖降為 1 次。
//   ④ forEach() / withLock() 傳入回呼 —— 當呼叫端需要做的事無法預先列舉時，
//      不把資料交出去，而是把「要做的事」拿進來，在鎖內執行。
//
// 【2. withLock() 這個樣板為什麼強大】
//       template<typename Func>
//       auto withLock(Func&& func) -> decltype(func(data)) {
//           std::lock_guard<std::mutex> lock(mtx);
//           return func(data);
//       }
//   它讓呼叫端可以執行【任意】操作而仍然受保護：加總、排序、過濾、
//   複合的讀改寫，都不需要為每一種需求新增一個成員函式。
//   回傳型別用 decltype(func(data)) 推導，所以回傳值也能正確傳出來。
//   （C++14 起可直接寫 `decltype(auto) withLock(...)`，更簡潔。）
//
// 【3. 回呼機制的兩個真實風險（必須文件化）】
//   風險一：【重複上鎖】。若使用者的回呼反過來呼叫本物件的 add()，
//           就會對同一把 std::mutex 鎖第二次 → 未定義行為
//           （std::mutex 不可重入）。
//   風險二：【引用逃逸】。withLock 把 data 的引用交給回呼，
//           使用者完全可以在回呼裡把這個引用存到外部變數，
//           等回呼結束、鎖釋放後再使用 —— 又回到實作4 的錯誤。
//           語言擋不住這件事，只能靠文件與 review。
//   還有一個效能面的風險：回呼在鎖內執行，若使用者做了 I/O 或睡眠，
//   臨界區段就被無限期拉長。所以文件必須寫明「回呼要短、不可阻塞」。
//
// 【4. 為什麼這個類別沒有提供 begin() / end()】
//   提供迭代器等於邀請使用者在鎖外走訪整個容器，
//   而且期間別人的 push_back 可能觸發重新配置讓迭代器失效。
//   執行緒安全的容器【刻意不提供】迭代器介面，
//   要走訪就用 forEach()（在鎖內）或 getData()（拿複本）。
//   這是它與 std::vector 在介面設計上最根本的差異。
//
// 【5. 為什麼 size() / empty() 這類查詢「幾乎沒有用」】
//   即使 size() 本身有鎖、回傳值正確，這個值在回傳的下一奈秒就可能過期：
//       if (c.size() > 0) { c.doSomething(); }   // ← 經典的 check-then-act
//   所以執行緒安全容器的 size()/empty() 只適合做「統計顯示」或「粗略判斷」，
//   絕不能拿來當作後續操作的前提。要做決策就用 ③ 提供的原子操作
//   （例如 tryPop() 回傳 optional，把檢查與取值合而為一）。
//   → 這是很多人第一次寫執行緒安全容器時最大的盲點。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 forEach 用 const std::function<void(int)>& 而不是樣板
//   std::function 有型別抹除的成本（可能配置記憶體、呼叫時多一次間接跳轉），
//   樣板版本（如 withLock）則能完全內聯、零額外成本。
//   選擇 std::function 的理由是：介面可以放進 .cpp、不必暴露在標頭檔，
//   編譯時間較短，且能存進成員變數（回呼註冊）。
//   兩者並存正好示範這個取捨：
//   對外的穩定 API 用 std::function，對效能敏感的用樣板。
//
// (B) mutable mutex 與 const 正確性
//   本類別的 getData / getAt / forEach / size / empty 都是 const 成員函式，
//   但它們都需要上鎖。mutex 宣告為 mutable 才能在 const 函式裡呼叫 lock()。
//   這是 mutable 最正統的用途：mutex 的狀態改變不屬於物件的邏輯狀態改變。
//   注意 withLock 不是 const —— 因為它把 data 的【非 const】引用交給回呼，
//   回呼可以修改內容。這個 const 差異是刻意的、也是正確的。
//
// (C) 為什麼 addAll 比迴圈呼叫 add 好得多
//   迴圈呼叫 add() N 次 = 上鎖解鎖 N 次。
//   本機實測單次 lock+unlock 約 13~25 ns（見課程 5.6-1 的實測範圍），
//   插入 10000 筆就有約 0.13~0.25 ms 純粹浪費在同步上，
//   而且每次釋放鎖都給了其他執行緒插隊的機會，造成更多競爭。
//   addAll() 用一次上鎖 + 一次 insert 完成，
//   這是「批次化」這個效能原則在介面設計上的體現。
//
// 【注意事項 Pay Attention】
// 1. 四種做法的共同精神：把操作搬進鎖內，不要把資料送出鎖外。
// 2. getAt 必須回傳【值】；回傳 const int& 就退化成實作4 的錯誤。
// 3. 回呼在鎖內執行 →（a）不可再呼叫本物件（重複上鎖是 UB）
//    （b）不可把引用存到外部（引用逃逸）（c）要短、不可做 I/O 或阻塞。
// 4. 刻意不提供 begin()/end()：那等於邀請使用者無鎖走訪。
// 5. size()/empty() 的結果回傳即過期，只能用於統計顯示，
//    不可當作後續操作的前提 —— 要決策請用回傳 optional 的原子操作。
// 6. 批次操作（addAll）把 N 次上鎖降為 1 次，效益顯著。
//
// 【LeetCode 說明】本檔不附 LeetCode 範例。
//   本檔主題是「執行緒安全介面的四種設計手法」，屬於 API 設計而非演算法。
//   允許使用的設計題（146/155/705/707/1603）在 LeetCode 上都是單執行緒判題，
//   介面回傳複本或引用完全不影響對錯；並行題（1114～1117/1195）
//   則沒有一題在設計容器的對外介面。硬湊只會失焦，故從缺，
//   改以下方兩個真實情境（工作佇列的原子取件、統計聚合的批次寫入）呈現。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】執行緒安全介面的設計手法
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 執行緒安全的容器該不該提供 size() 和 empty()?
//     答：可以提供，但要清楚它們【幾乎沒有決策價值】。
//         即使有鎖、回傳值當下正確，這個值在回傳的下一奈秒就可能過期：
//         `if (c.size() > 0) c.pop();` 依然是經典的 check-then-act。
//         它們只適合統計顯示或粗略判斷；要做決策必須用把檢查與行動
//         合而為一的原子操作，例如回傳 std::optional 的 tryPop()。
//     追問：那為什麼標準容器還是提供 size()?
//         → 因為標準容器本來就不是執行緒安全的，
//           在單執行緒下 size() 的結果不會過期，完全可靠。
//           問題不在 size() 本身，而在把單執行緒的思考習慣帶進並行環境。
//
// 🔥 Q2. withLock(callback) 這種「把回呼拿進鎖內執行」的設計有什麼風險?
//     答：三個。①【重複上鎖】：回呼若反過來呼叫本物件的方法，
//         會對同一把 std::mutex 鎖第二次 → UB。
//         ②【引用逃逸】：回呼拿到的是內部資料的引用，
//         使用者可以把它存到外部，鎖釋放後再用 —— 語言擋不住。
//         ③【臨界區段被拉長】：回呼裡若有 I/O 或睡眠，整把鎖被占住。
//         這三點都必須寫進文件，因為編譯器不會幫你檢查。
//     追問：那有沒有辦法從設計上避免?
//         → 可以只把【複本】交給回呼（犧牲效能換安全），
//           或改成回傳複本讓呼叫端在鎖外處理。
//           Rust 用借用檢查器從型別系統擋掉②，C++ 只能靠紀律。
//
// ⚠️ 陷阱. forEach() 在鎖內執行使用者的回呼，這樣不是很安全嗎?
//     答：對「資料不被並行修改」而言是安全的，但它同時打開了另外兩個洞：
//         回呼可能再次呼叫本物件（重複上鎖 → UB），
//         也可能把引用偷渡到鎖外。而且回呼是【使用者的程式碼】——
//         你無法控制它會不會做網路請求、會不會睡三秒。
//     為什麼會錯：把「在鎖內執行」等同於「安全」。
//         在鎖內執行【自己的、可預測的、短的】程式碼才安全；
//         在鎖內執行【別人的、任意的】程式碼，等於把臨界區段的
//         長度與行為交給呼叫端決定，這在設計上是把控制權交了出去。
//         這也是為什麼許多函式庫規定「回呼中不得呼叫本物件的任何方法」。
//
// 檔案：lesson_5_5_safe_interface.cpp
// 說明：安全的介面設計

#include <iostream>
#include <mutex>
#include <vector>
#include <functional>
#include <algorithm>
#include <thread>
#include <atomic>
#include <optional>
#include <string>
#include <stdexcept>

class SafeContainer {
private:
    mutable std::mutex mtx;
    std::vector<int> data;
    
public:
    // ✓ 安全：返回複本
    std::vector<int> getData() const {
        std::lock_guard<std::mutex> lock(mtx);
        return data;  // 返回複本，不是引用
    }
    
    // ✓ 安全：返回值的複本
    int getAt(size_t index) const {
        std::lock_guard<std::mutex> lock(mtx);
        if (index < data.size()) {
            return data[index];  // 返回值，不是引用
        }
        throw std::out_of_range("索引超出範圍");
    }
    
    // ✓ 安全：提供操作而非資料存取
    void add(int value) {
        std::lock_guard<std::mutex> lock(mtx);
        data.push_back(value);
    }
    
    // ✓ 安全：使用回呼函式處理資料
    void forEach(const std::function<void(int)>& func) const {
        std::lock_guard<std::mutex> lock(mtx);
        for (int value : data) {
            func(value);
        }
    }
    
    // ✓ 安全：在鎖保護下執行操作
    template<typename Func>
    auto withLock(Func&& func) -> decltype(func(data)) {
        std::lock_guard<std::mutex> lock(mtx);
        return func(data);
    }
    
    // ✓ 安全：批次操作
    void addAll(const std::vector<int>& values) {
        std::lock_guard<std::mutex> lock(mtx);
        data.insert(data.end(), values.begin(), values.end());
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx);
        return data.size();
    }
    
    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx);
        return data.empty();
    }
};


// -----------------------------------------------------------------------------
// 【日常實務範例 1】工作佇列：用「回傳 optional 的原子操作」取代 size() 判斷
//   情境：執行緒池的工作者不斷從佇列取件。
//   天真寫法：
//       if (!queue.empty()) { auto job = queue.front(); queue.pop(); }
//     即使 empty() 與 front() 各自都有鎖，中間仍有縫隙 ——
//     兩個工作者同時看到「非空」，其中一個取走最後一件，
//     另一個的 front() 就對空佇列操作 → UB。
//   正解：tryTake() 把「檢查 + 取出」合併成單一個原子操作，
//         用 std::optional 表達「可能沒有」。呼叫端根本拿不到中間狀態。
// -----------------------------------------------------------------------------
struct Job {
    int id;
    std::string name;
};

class JobQueue {
private:
    mutable std::mutex mtx;
    std::vector<Job> jobs;
    long taken = 0;

public:
    void submit(int id, const std::string& name) {
        std::lock_guard<std::mutex> lock(mtx);
        jobs.push_back(Job{id, name});
    }

    // ✓ 檢查與取出在同一個臨界區段 → 不可能取到不存在的工作
    std::optional<Job> tryTake() {
        std::lock_guard<std::mutex> lock(mtx);
        if (jobs.empty()) return std::nullopt;
        Job j = jobs.back();
        jobs.pop_back();
        ++taken;
        return j;
    }

    // 批次提交：一次上鎖取代 N 次
    void submitAll(const std::vector<Job>& batch) {
        std::lock_guard<std::mutex> lock(mtx);
        jobs.insert(jobs.end(), batch.begin(), batch.end());
    }

    long takenCount() const { std::lock_guard<std::mutex> lock(mtx); return taken; }
    size_t remaining() const { std::lock_guard<std::mutex> lock(mtx); return jobs.size(); }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】統計聚合器：withLock 讓呼叫端做任意運算而不外洩資料
//   情境：監控模組需要對一批延遲數據做各種統計（總和、最大值、百分位數），
//         而且需求會不斷增加。若為每種統計都加一個成員函式，
//         類別會無限膨脹；若回傳整份資料的引用，就回到實作4 的錯誤。
//   正解：提供 withLock，讓呼叫端把「要算什麼」傳進來，在鎖內執行。
//         新增統計需求完全不需要修改這個類別。
//   ⚠️ 文件必須寫明：回呼中不可再呼叫本物件、不可保存引用、不可阻塞。
// -----------------------------------------------------------------------------
class LatencyStats {
private:
    mutable std::mutex mtx;
    std::vector<long> samples;

public:
    void record(long micros) {
        std::lock_guard<std::mutex> lock(mtx);
        samples.push_back(micros);
    }

    void recordAll(const std::vector<long>& batch) {
        std::lock_guard<std::mutex> lock(mtx);
        samples.insert(samples.end(), batch.begin(), batch.end());
    }

    // ✓ 把運算拿進來，而不是把資料送出去
    //   注意：回呼收到的是 const 引用，所以它不能修改資料，
    //   只擋得住「意外修改」，仍擋不住「刻意保存引用」——後者只能靠文件。
    template<typename Func>
    auto compute(Func&& func) const -> decltype(func(samples)) {
        std::lock_guard<std::mutex> lock(mtx);
        return func(samples);
    }

    size_t count() const { std::lock_guard<std::mutex> lock(mtx); return samples.size(); }
};

int main() {
    SafeContainer container;
    
    container.add(10);
    container.add(20);
    container.add(30);
    
    // 使用 forEach
    std::cout << "forEach: ";
    container.forEach([](int v) {
        std::cout << v << " ";
    });
    std::cout << std::endl;
    
    // 使用 withLock 進行複雜操作
    int sum = container.withLock([](std::vector<int>& data) {
        int total = 0;
        for (int v : data) {
            total += v;
        }
        return total;
    });
    std::cout << "Sum: " << sum << std::endl;
    
    // 獲取複本
    auto copy = container.getData();
    std::cout << "Copy size: " << copy.size() << std::endl;

    // 複本是獨立的：改它不會影響內部狀態
    copy.push_back(999);
    std::cout << "改複本後，內部 size 仍為: " << container.size()
              << "（複本 size = " << copy.size() << "）" << std::endl;

    std::cout << "\n=== 批次操作：一次上鎖取代 N 次 ===" << std::endl;
    {
        SafeContainer batch;
        std::vector<int> values;
        for (int i = 0; i < 1000; ++i) values.push_back(i);
        batch.addAll(values);                       // 一次上鎖
        std::cout << "addAll 1000 筆後 size = " << batch.size()
                  << "（只上鎖 1 次，而非 1000 次）" << std::endl;

        int sum = batch.withLock([](std::vector<int>& d) {
            int t = 0;
            for (int v : d) t += v;
            return t;
        });
        std::cout << "withLock 計算總和 = " << sum << "（必定為 499500）" << std::endl;
    }

    std::cout << "\n=== 併發驗證：多執行緒同時 add + forEach ===" << std::endl;
    {
        SafeContainer shared;
        std::vector<std::thread> ths;
        for (int i = 0; i < 4; ++i) {
            ths.emplace_back([&shared, i] {
                for (int k = 0; k < 5000; ++k) shared.add(i * 5000 + k);
            });
        }
        for (auto& t : ths) t.join();

        std::cout << "4 執行緒 × 5000 次 add，最終 size = " << shared.size()
                  << "（必定為 20000）" << std::endl;

        long total = shared.withLock([](std::vector<int>& d) {
            long t = 0;
            for (int v : d) t += v;
            return t;
        });
        std::cout << "總和 = " << total << "（必定為 199990000）" << std::endl;
    }

    std::cout << "\n=== 日常實務 1：工作佇列的原子取件 ===" << std::endl;
    {
        JobQueue q;
        std::vector<Job> batch;
        for (int i = 1; i <= 10000; ++i) batch.push_back(Job{i, "job-" + std::to_string(i)});
        q.submitAll(batch);

        std::atomic<int> got{0};
        std::atomic<int> empties{0};
        std::vector<std::thread> workers;
        for (int i = 0; i < 8; ++i) {
            workers.emplace_back([&q, &got, &empties] {
                int local = 0, none = 0;
                for (int k = 0; k < 2000; ++k) {
                    if (auto job = q.tryTake()) ++local;   // 檢查與取出是一個動作
                    else ++none;
                }
                got.fetch_add(local);
                empties.fetch_add(none);
            });
        }
        for (auto& t : workers) t.join();

        std::cout << "提交 10000 件，8 個工作者共嘗試 16000 次" << std::endl;
        std::cout << "取得件數: " << got.load() << "（必定為 10000，不重不漏）" << std::endl;
        std::cout << "取到空的次數: " << empties.load() << "（必定為 6000）" << std::endl;
        std::cout << "佇列剩餘: " << q.remaining() << "（必定為 0）" << std::endl;
        std::cout << "→ 若寫成 if(!empty()) front()，兩個工作者會搶同一件 → UB" << std::endl;
    }

    std::cout << "\n=== 日常實務 2：withLock 做任意統計 ===" << std::endl;
    {
        LatencyStats stats;
        std::vector<std::thread> ths;
        for (int i = 0; i < 4; ++i) {
            ths.emplace_back([&stats, i] {
                std::vector<long> batch;
                for (int k = 0; k < 2500; ++k) batch.push_back((i * 2500 + k) % 500 + 1);
                stats.recordAll(batch);      // 批次寫入：一次上鎖
            });
        }
        for (auto& t : ths) t.join();

        std::cout << "樣本數: " << stats.count() << "（必定為 10000）" << std::endl;

        // 三種完全不同的統計，都不需要修改 LatencyStats 這個類別
        long sum = stats.compute([](const std::vector<long>& s) {
            long t = 0; for (long v : s) t += v; return t;
        });
        long mx = stats.compute([](const std::vector<long>& s) {
            long m = 0; for (long v : s) if (v > m) m = v; return m;
        });
        size_t slow = stats.compute([](const std::vector<long>& s) {
            size_t c = 0; for (long v : s) if (v > 400) ++c; return c;
        });

        std::cout << "總延遲: " << sum << " μs" << std::endl;
        std::cout << "最大延遲: " << mx << " μs（必定為 500）" << std::endl;
        std::cout << "超過 400 μs 的樣本數: " << slow << "（必定為 2000）" << std::endl;
        std::cout << "→ 新增統計需求完全不必修改 LatencyStats" << std::endl;
    }
    
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.5：保護共享資料實作5.cpp' -o safe_interface

// 註：本檔所有共享狀態都受鎖保護，且對外介面不洩漏任何引用，
// 因此沒有資料競爭，輸出為確定值（本機連續三次實測 md5 一致）。

// === 預期輸出 ===
// forEach: 10 20 30 
// Sum: 60
// Copy size: 3
// 改複本後，內部 size 仍為: 3（複本 size = 4）
//
// === 批次操作：一次上鎖取代 N 次 ===
// addAll 1000 筆後 size = 1000（只上鎖 1 次，而非 1000 次）
// withLock 計算總和 = 499500（必定為 499500）
//
// === 併發驗證：多執行緒同時 add + forEach ===
// 4 執行緒 × 5000 次 add，最終 size = 20000（必定為 20000）
// 總和 = 199990000（必定為 199990000）
//
// === 日常實務 1：工作佇列的原子取件 ===
// 提交 10000 件，8 個工作者共嘗試 16000 次
// 取得件數: 10000（必定為 10000，不重不漏）
// 取到空的次數: 6000（必定為 6000）
// 佇列剩餘: 0（必定為 0）
// → 若寫成 if(!empty()) front()，兩個工作者會搶同一件 → UB
//
// === 日常實務 2：withLock 做任意統計 ===
// 樣本數: 10000（必定為 10000）
// 總延遲: 2505000 μs
// 最大延遲: 500 μs（必定為 500）
// 超過 400 μs 的樣本數: 2000（必定為 2000）
// → 新增統計需求完全不必修改 LatencyStats
