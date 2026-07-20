/*=============================================================================
 * 檔名：19_RAII.cpp
 * 主題：RAII (Resource Acquisition Is Initialization)
 * 適合：學完建構子、解構子、繼承後，準備接觸現代 C++ 核心觀念的人
 *
 * 【課題介紹】
 *   RAII 縮寫看起來很學術，意思其實很單純：
 *
 *       「資源 (Resource) 在物件建構時取得 (Acquisition)，
 *        並用『初始化 (Initialization)』來表示這件事；
 *        當物件解構時，資源自動被釋放。」
 *
 *   翻譯成白話：
 *       「把資源綁在某個 C++ 物件上，由它的生命週期幫你管。」
 *
 *   為什麼這麼重要？因為 C++ 沒有 GC (垃圾回收)，但物件在離開作用域、
 *   被 delete、容器被銷毀、甚至「函式拋出例外時」… 解構子都會被自動呼叫。
 *   所以只要把「釋放動作」寫在解構子裡，C++ 就會替你保證一定執行。
 *
 *   這個觀念衍生了現代 C++ 的許多明星設計：
 *     - std::unique_ptr / std::shared_ptr  → 自動 delete (第 20、21 篇)
 *     - std::lock_guard / std::scoped_lock → 自動 unlock
 *     - std::fstream                       → 自動 close
 *     - std::vector / std::string          → 自動 free
 *
 *   以前 C 語言時代要寫：
 *       FILE* f = fopen("a.txt", "r");
 *       if (!f) goto fail;
 *       // ... 使用 ...
 *       fclose(f);
 *   一旦中間 return 或拋例外，fclose 沒跑就洩漏。RAII 解決所有這類問題。
 *
 * 【RAII 三大特徵】
 *   1. 建構子 = 取得資源 (open / lock / new)
 *   2. 解構子 = 釋放資源 (close / unlock / delete)
 *   3. 物件不准被「不小心複製」(否則會釋放兩次) → 通常會把 copy ctor 設為 delete
 *      或實作正確的複製語意 (參考 Rule of 3/5/0，第 23 篇)
 *
 * 【日常實用範例】
 *   1. ScopedTimer：自動計時器，建構時開始計時、解構時印出耗時。
 *      工作上很常用：套在某段程式外面就能量速度。
 *   2. FileHandle：自動關檔的小包裝。
 *
 * 【對應 Leetcode】1480. Running Sum of 1d Array
 *   為什麼選這題：題目本身很簡單，但我們可以用它示範 RAII 的「實戰用途」 —
 *   把 ScopedTimer 套在 LC 解法外面，量它的執行時間。日常工作上想知道某段
 *   程式跑多久時，這個寫法非常常用，而且就算中途 return / 拋例外都會自動印時間。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/language/raii
 *   https://cplusplus.com/reference/chrono/
 *=============================================================================*/

/*
補充筆記：RAII
  - RAII 把資源取得放進建構子，把釋放放進解構子。
  - RAII 的價值在例外或 early return 時仍能自動清理。
  - mutex lock、file handle、memory ownership 都應優先用 RAII 類型管理。
  - RAII 的意思是 Resource Acquisition Is Initialization：資源在物件建構時取得，在物件解構時釋放。
  - RAII 把成對操作包起來，例如 open/close、lock/unlock、new/delete；使用者只要控制物件生命週期，不必手動記得釋放。
  - RAII 最重要的好處是例外安全：即使中途 throw，stack unwinding 仍會呼叫已建立物件的 destructor。
  - std::vector 管理動態陣列、std::string 管理字串記憶體、std::fstream 管理檔案、std::lock_guard 管理 mutex lock，都是 RAII。
  - RAII 類別通常不可複製或要定義清楚複製語意；例如 mutex lock 不能被複製，檔案 handle 複製也容易造成雙重關閉。
  - 如果你在程式中看到裸 new/delete、手動 close、手動 unlock，可以優先思考是否能改成 RAII 物件。
  - RAII destructor 應盡量不拋例外，因為它常在錯誤處理過程被自動呼叫。
  - RAII 不是只為記憶體而生，而是 C++ 管理任何有限資源的核心設計方法。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】RAII
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是 RAII？為什麼它能提供 exception safety？
//     答：Resource Acquisition Is Initialization —— 在 constructor 取得資源、在
//         destructor 釋放資源，把資源生命週期綁在物件生命週期上。之所以能提供
//         exception safety，是因為例外造成 stack unwinding 時，「已完整建構」的
//         物件其 destructor 一定會被呼叫（本檔範例 3 的 mayThrow 正是在示範這件事）。
//     追問：舉幾個標準庫的 RAII 型別？（std::lock_guard、std::unique_ptr、
//           std::fstream、std::vector、std::string）
//
// 🔥 Q2. RAII 型別為什麼通常要禁止 copy，或正確定義 copy？
//     答：預設的 memberwise copy 會讓兩個物件持有同一份資源 → 兩次釋放（double free
//         / 重複 close / 重複 unlock）。所以本檔的 ScopedTimer、FileHandle、
//         ScopedLock 都把 copy constructor 與 copy assignment `= delete`。
//         若資源本來就可共享或可深拷貝，才去正確定義 copy（見第 23 篇 Rule of 3/5/0）。
//     追問：那 move 呢？（多數 RAII handle 是「可移動不可複製」—— 所有權轉移是合理的）
//
// ⚠️ 陷阱. destructor 裡可以拋出例外嗎？
//     答：不該。C++11 起 destructor 預設是 `noexcept(true)`，例外逃出 destructor
//         會直接呼叫 `std::terminate`；若當下正在 stack unwinding（已有一個例外在飛），
//         同樣是 terminate。原則：destructor 內部自己 try/catch 吞掉或記錄。
//     為什麼會錯：多數人把 destructor 當成一般函式，覺得「釋放失敗就丟出去讓上層處理」。
//         但 destructor 最常被呼叫的時機正是錯誤處理過程本身，此時再丟就無路可退了。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <cstdint>
#include <chrono>      // 時間相關 std::chrono
#include <fstream>     // 檔案 std::ofstream
#include <thread>      // std::this_thread::sleep_for
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 範例 1：ScopedTimer 自動計時器
// -----------------------------------------------------------------------------
// 用法：在某個區塊一開始 ScopedTimer t("name");，離開時自動印耗時。
// 適合用來量「某段程式跑多久」 - 工作上量化效能很實用。
class ScopedTimer {
private:
    std::string label_;
    std::chrono::steady_clock::time_point start_;

public:
    explicit ScopedTimer(const std::string& label)
        : label_(label),
          start_(std::chrono::steady_clock::now()) {
        std::cout << "[計時開始] " << label_ << std::endl;
    }

    ~ScopedTimer() {
        auto end = std::chrono::steady_clock::now();
        auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_).count();
        std::cout << "[計時結束] " << label_ << " 耗時 " << ms << " ms" << std::endl;
    }

    // RAII 物件通常禁止複製，避免「同一資源被釋放兩次」的問題
    ScopedTimer(const ScopedTimer&)            = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;
};

// -----------------------------------------------------------------------------
// 範例 2：FileHandle 自動關檔
// -----------------------------------------------------------------------------
// 注意：std::ofstream 本身就是 RAII，這裡只是練習自己手做一個小包裝
class FileHandle {
private:
    std::ofstream out_;

public:
    explicit FileHandle(const std::string& path) : out_(path) {
        if (!out_) {
            std::cout << "[FileHandle] 無法開啟 " << path << std::endl;
        } else {
            std::cout << "[FileHandle] 已開啟 " << path << std::endl;
        }
    }

    ~FileHandle() {
        if (out_) {
            out_.close();      // 即便 ofstream 自己會關，這裡顯式呼叫只是教學示意
            std::cout << "[FileHandle] 已自動關閉" << std::endl;
        }
    }

    void writeln(const std::string& s) {
        if (out_) out_ << s << '\n';
    }

    FileHandle(const FileHandle&)            = delete;
    FileHandle& operator=(const FileHandle&) = delete;
};

// -----------------------------------------------------------------------------
// 範例 3：示範「即便拋出例外，解構子仍會跑」
// -----------------------------------------------------------------------------
void mayThrow(bool doThrow) {
    ScopedTimer t("mayThrow");      // 一進來就開始計時
    if (doThrow) {
        std::cout << "(故意丟出例外)" << std::endl;
        throw std::runtime_error("oops");
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

// -----------------------------------------------------------------------------
// 範例 4：對應 Leetcode 359 - Logger Rate Limiter (用 RAII 包資源)
// -----------------------------------------------------------------------------
// 把「rate limiter 的開關」用 RAII 包裝：
// 物件建構時開始計時模式、解構時自動關閉並印統計。
class RateLimiterSession {
private:
    std::string name_;
    int         allowedCount_;
    int         deniedCount_;
public:
    explicit RateLimiterSession(const std::string& name)
        : name_(name), allowedCount_(0), deniedCount_(0) {
        std::cout << "[RateLimiter] 啟動 " << name_ << std::endl;
    }

    ~RateLimiterSession() {
        // RAII：解構時自動印統計，使用者完全不必呼叫 stop()
        std::cout << "[RateLimiter] 結束 " << name_
                  << " 允許=" << allowedCount_
                  << " 拒絕=" << deniedCount_ << std::endl;
    }

    // 模擬 LC 359 的決策：返回是否可以印
    bool tryLog(int timestamp, const std::string& msg) {
        // 為了簡化，只是把 hash 結果當作通過邏輯
        bool allow = ((timestamp + static_cast<int>(msg.size())) % 2) == 0;
        if (allow) ++allowedCount_;
        else       ++deniedCount_;
        return allow;
    }
};

// -----------------------------------------------------------------------------
// 範例 5：日常實用 - Lock RAII (模擬 std::lock_guard)
// -----------------------------------------------------------------------------
// 這是 RAII 最經典的應用之一：自動 lock / unlock。
class FakeMutex {
public:
    bool locked = false;
    void lock()   { locked = true;  std::cout << "  Mutex.lock()\n"; }
    void unlock() { locked = false; std::cout << "  Mutex.unlock()\n"; }
};

class ScopedLock {
private:
    FakeMutex& m_;
public:
    explicit ScopedLock(FakeMutex& m) : m_(m) { m_.lock(); }
    ~ScopedLock() { m_.unlock(); }     // 離開作用域自動 unlock，即使 throw 也是
    ScopedLock(const ScopedLock&)            = delete;
    ScopedLock& operator=(const ScopedLock&) = delete;
};

int main() {
    std::cout << "===== 範例 1：ScopedTimer =====" << std::endl;
    {
        ScopedTimer t("計算迴圈");
        long long s = 0;
        for (int i = 0; i < 1000000; ++i) s += i;     // 跑久一點，看耗時
        // 離開大括號 → t 解構 → 自動印耗時
    }

    std::cout << "===== 範例 2：FileHandle =====" << std::endl;
    {
        FileHandle f("raii_demo.txt");
        f.writeln("hello");
        f.writeln("world");
        // 離開 → f 解構 → 自動關檔
    }

    std::cout << "===== 範例 3：拋例外時解構子仍會跑 =====" << std::endl;
    try {
        mayThrow(true);
    } catch (const std::exception& e) {
        std::cout << "main 捕到例外: " << e.what() << std::endl;
    }

    std::cout << "===== 範例 4：Leetcode 1480 用 ScopedTimer 量解法 =====" << std::endl;
    // LC 1480: Running Sum 解法。我們把 ScopedTimer 套在 in-place 解法外面，
    // 完全不用手動計時、不用煩惱「return 之前忘了 stop」之類的問題。
    {
        // ⚠️ 踩雷：0..99999 的前綴和是 4,999,950,000,遠超 INT_MAX(約 21.5 億)。
        //    用 vector<int> 會 signed overflow（UB）——而且是在跟 RAII 完全無關的
        //    地方爆掉,最難查。改用 int64_t。
        std::vector<std::int64_t> nums(100000);
        for (int i = 0; i < 100000; ++i) nums[i] = i;
        ScopedTimer t("LC1480 RunningSum");
        for (size_t i = 1; i < nums.size(); ++i) nums[i] += nums[i - 1];
        // 離開大括號 → t 解構 → 自動印耗時。中途如果有 return / throw 也一樣。
    }

    std::cout << "===== 範例 5：RateLimiterSession (LC 359 變體) =====" << std::endl;
    {
        RateLimiterSession sess("authLog");
        sess.tryLog(1, "foo");
        sess.tryLog(2, "bar");
        sess.tryLog(3, "baz");
        // 離開區塊 → sess 解構 → 自動印統計
    }

    std::cout << "===== 範例 6：ScopedLock 自動上鎖/解鎖 =====" << std::endl;
    FakeMutex m;
    {
        ScopedLock guard(m);
        std::cout << "  (臨界區操作中, locked=" << m.locked << ")\n";
        // 離開區塊 → guard 解構 → 自動 unlock
    }
    std::cout << "  locked = " << m.locked << " (預期 0)\n";
    return 0;
}

/* 預期輸出 (耗時數字依機器而異)：
 * ===== 範例 1：ScopedTimer =====
 * [計時開始] 計算迴圈
 * [計時結束] 計算迴圈 耗時 2 ms
 * ===== 範例 2：FileHandle =====
 * [FileHandle] 已開啟 raii_demo.txt
 * [FileHandle] 已自動關閉
 * ===== 範例 3：拋例外時解構子仍會跑 =====
 * [計時開始] mayThrow
 * (故意丟出例外)
 * [計時結束] mayThrow 耗時 0 ms
 * main 捕到例外: oops
 * ===== 範例 4：Leetcode 1480 用 ScopedTimer 量解法 =====
 * [計時開始] LC1480 RunningSum
 * [計時結束] LC1480 RunningSum 耗時 X ms
 * ===== 範例 5：RateLimiterSession (LC 359 變體) =====
 * [RateLimiter] 啟動 authLog
 * [RateLimiter] 結束 authLog 允許=? 拒絕=?
 * ===== 範例 6：ScopedLock 自動上鎖/解鎖 =====
 *   Mutex.lock()
 *   (臨界區操作中, locked=1)
 *   Mutex.unlock()
 *   locked = 0 (預期 0)
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. RAII = 「建構子取得資源、解構子釋放資源」，把資源綁在物件生命週期上。
 *   2. 即便拋出例外、提前 return，解構子仍會被呼叫，所以資源不會洩漏。
 *   3. RAII 物件通常 delete 掉複製建構/賦值，避免重複釋放。
 *   4. 現代 C++ 的智慧指標、lock_guard、fstream 都是 RAII 的應用。
 *
 * 【下一篇預告】
 *   20_UniquePtr.cpp
 *   std::unique_ptr — 用 RAII 包住 new/delete，幾乎讓你不用再寫 delete。
 *=============================================================================*/

// 編譯: g++ -std=c++20 -Wall -Wextra 19_RAII.cpp -o 19_RAII

// === 預期輸出 ===
// ===== 範例 1：ScopedTimer =====
// [計時開始] 計算迴圈
// [計時結束] 計算迴圈 耗時 1 ms
// ===== 範例 2：FileHandle =====
// [FileHandle] 已開啟 raii_demo.txt
// [FileHandle] 已自動關閉
// ===== 範例 3：拋例外時解構子仍會跑 =====
// [計時開始] mayThrow
// (故意丟出例外)
// [計時結束] mayThrow 耗時 0 ms
// main 捕到例外: oops
// ===== 範例 4：Leetcode 1480 用 ScopedTimer 量解法 =====
// [計時開始] LC1480 RunningSum
// [計時結束] LC1480 RunningSum 耗時 1 ms
// ===== 範例 5：RateLimiterSession (LC 359 變體) =====
// [RateLimiter] 啟動 authLog
// [RateLimiter] 結束 authLog 允許=2 拒絕=1
// ===== 範例 6：ScopedLock 自動上鎖/解鎖 =====
//   Mutex.lock()
//   (臨界區操作中, locked=1)
//   Mutex.unlock()
//   locked = 0 (預期 0)
// ⚠️ 上面的位址／執行緒 id／耗時每次執行都不同，數值僅供對照，不是固定結果。
