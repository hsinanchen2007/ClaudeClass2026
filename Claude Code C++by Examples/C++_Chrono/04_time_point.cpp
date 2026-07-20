// =============================================================================
//  04_time_point.cpp  —  time_point 與 epoch 運算
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/chrono/time_point
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、time_point 是什麼？                                    │
//  └────────────────────────────────────────────────────────────┘
//
//  std::chrono::time_point<Clock, Duration> 表示「某個 Clock 的某一個瞬間」。
//  內部其實只存了一個「自 Clock 的 epoch 到此刻過了多久」的 duration。
//
//      time_point 的概念本質：
//          time_point = Clock::epoch() + duration
//
//  Clock 不同 → time_point 是不同型別。system_clock::time_point 跟
//  steady_clock::time_point「不能互相運算」 — 這是型別系統的保護。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、運算規則                                               │
//  └────────────────────────────────────────────────────────────┘
//
//      time_point + duration = time_point        // 「時刻平移」
//      time_point - duration = time_point
//      time_point - time_point = duration         // 「兩時刻差距」
//      time_point < time_point                    // 比較先後
//
//  time_point + time_point 沒有意義（被禁止）。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、time_since_epoch — 取出內部 duration                   │
//  └────────────────────────────────────────────────────────────┘
//
//  把 time_point 當成「向量」，time_since_epoch() 取得它相對於原點的位移：
//
//      auto tp = system_clock::now();
//      auto since = tp.time_since_epoch();
//      auto secs = duration_cast<seconds>(since).count();
//      // 對 system_clock：這個值就是 Unix timestamp（自 1970 年的秒數）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：time_point 加減 duration
//   * Demo 2：兩個 time_point 相減
//   * Demo 3：time_since_epoch 取 Unix timestamp
//   * Demo 4：用 time_point 排程「30 秒後到期」
// =============================================================================

/*
補充筆記：std::time_point
  - std::time_point 使用 <chrono> 時要分清楚 clock、duration、time_point：時鐘產生時間點，兩個時間點相減得到 duration。
  - steady_clock 適合量測耗時，因為它不會受系統時間調整影響；system_clock 適合和日曆時間或 Unix timestamp 互通。
  - duration 帶有單位型別，例如 milliseconds、seconds；duration_cast 用於明確轉換可能截斷的單位。
  - sleep_for/sleep_until 不保證精準喚醒，只保證至少睡到接近指定時間；排程器和系統負載會造成延遲。
  - benchmark 時要避免把輸出、配置或第一次初始化混在量測範圍，否則數字不代表目標程式碼成本。
  - C++20 calendar/time zone API 能表達年月日，但時區資料和平台支援要另外確認。
  - time_point 表示某個 clock 座標上的時間點；不同 clock 的 time_point 不能隨便相減。
  - time_point + duration 仍是 time_point，time_point - time_point 才是 duration。
  - 儲存時間點前要想清楚是要絕對時間、相對期限，還是單純耗時。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】time_point 與 epoch 運算
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. time_point 支援哪些運算？哪些不行？
//     答：合法：time_point ± duration → time_point、time_point - time_point →
//         duration、同一時鐘的 time_point 互相比較。不合法：time_point + time_point
//         （兩個時刻相加沒有意義，編譯錯誤）。另一個關鍵限制是「不同 Clock 的
//         time_point 不可互相比較或相減」，型別不同、編譯期就被擋下；C++20 才用
//         std::chrono::clock_cast 提供受控的轉換。
//     追問：這種設計叫什麼？（affine space：點與向量分離，與 chrono 的單位安全一脈相承）
//
// 🔥 Q2. time_since_epoch() 回傳什麼？可以拿來當時間戳嗎？
//     答：回傳「相對於該時鐘 epoch 的 duration」。只有 system_clock 的 epoch 有明確
//         意義（C++20 起標準規定為 Unix epoch，1970-01-01 UTC 且不計閏秒），可以拿來
//         算 timestamp。steady_clock 的 epoch 沒有定義意義（實務上常是開機時間），
//         拿它算日期會得到完全錯誤的結果。
//     追問：那 steady_clock 的 time_since_epoch() 有什麼用？（幾乎只用於「兩次相減」，
//           或作為不透明的識別值，不應被解讀成任何真實時刻）
//
// ⚠️ 陷阱. auto d = end - start; 的 d 是什麼型別？直接印 d.count() 會怎樣？
//     答：d 的型別是 Clock::duration，其單位是「實作定義」的（常見實作上 steady_clock
//         的 duration 是 nanoseconds，但標準未規定）。直接印 count() 得到的數字單位
//         不明，換平台後意義就變了。應該明確轉換：duration_cast<microseconds>(d).count()。
//     為什麼會錯：auto 讓人忽略了這裡其實有一個「單位」的自由度。C++20 起 operator<<
//         可直接輸出 duration 並附帶單位後綴（如 1500ms），是更安全的選擇。
// ═══════════════════════════════════════════════════════════════════════════

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>

// 前置宣告：附加範例
static void demo_simple_ttl_cache();
static void demo_uptime_seconds();

int main() {
    using namespace std::chrono;
    using namespace std::chrono_literals;

    // ─────────────────────────────────────────────────────────
    // Demo 1：時刻平移
    // ─────────────────────────────────────────────────────────
    auto now = steady_clock::now();
    auto then = now + 5min;
    auto diff = then - now;
    std::cout << "[Demo1] then - now = "
              << duration_cast<minutes>(diff).count() << " min\n";

    // ─────────────────────────────────────────────────────────
    // Demo 2：兩個 time_point 比較
    // ─────────────────────────────────────────────────────────
    auto t1 = steady_clock::now();
    auto t2 = t1 + 100ms;
    std::cout << "[Demo2] t1 < t2 ? " << std::boolalpha << (t1 < t2) << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 3：system_clock 的 time_since_epoch ≈ Unix timestamp
    // ─────────────────────────────────────────────────────────
    auto sysNow = system_clock::now();
    auto epochS = duration_cast<seconds>(sysNow.time_since_epoch()).count();
    std::cout << "[Demo3] unix timestamp = " << epochS << " seconds\n";

    // ─────────────────────────────────────────────────────────
    // Demo 4：用 time_point 表達「截止時間」
    //   絕對時間比相對時間更安全 — sleep_until 不會因為 spurious wakeup
    //   而提早結束（sleep_for 在某些 OS 會 early return）。
    // ─────────────────────────────────────────────────────────
    auto deadline = steady_clock::now() + 30s;
    auto remaining = deadline - steady_clock::now();
    std::cout << "[Demo4] until deadline: "
              << duration_cast<seconds>(remaining).count() << " seconds\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼不能 system_tp + steady_tp？
    //    A：兩者的 epoch 不同（一個是 1970，一個未指定），物理意義不一樣。
    //       讓型別系統擋下這個運算，避免你不小心混用。
    //
    //  Q2：time_point 的內部 duration 預設是什麼？
    //    A：等於 Clock::duration（每個 clock 自己定義）。如果你需要不同精度
    //       的 time_point，可以指定第二個 template 參數，例如：
    //         time_point<system_clock, milliseconds>
    //
    //  Q3：time_point 之間如何 hash / 存進 unordered_map？
    //    A：它本質就是 duration（可比、可大小排序），但 STL 沒提供
    //       std::hash 特化。要自己用 time_since_epoch().count() 當 key。
    //
    demo_simple_ttl_cache();
    demo_uptime_seconds();
    return 0;
}

// =============================================================================
//  附加 1：實用範例 — 簡易 TTL cache（key 過期就視為不存在）
// =============================================================================
//  快取常用模式：每個 entry 帶一個「過期時間 point」，查詢時若 now > expireAt
//  就回 miss。用 steady_clock 確保時鐘不會回頭、過期判斷穩定。
// =============================================================================
class TtlCache {
public:
    void put(const std::string& key, int value,
             std::chrono::milliseconds ttl) {
        auto exp = std::chrono::steady_clock::now() + ttl;
        store_[key] = {value, exp};
    }
    bool get(const std::string& key, int& out) {
        auto it = store_.find(key);
        if (it == store_.end()) return false;
        if (std::chrono::steady_clock::now() > it->second.expireAt) {
            store_.erase(it);
            return false;
        }
        out = it->second.value;
        return true;
    }
private:
    struct Entry {
        int value;
        std::chrono::steady_clock::time_point expireAt;
    };
    std::unordered_map<std::string, Entry> store_;
};
static void demo_simple_ttl_cache() {
    using namespace std::chrono_literals;
    TtlCache c;
    c.put("a", 42, 100ms);
    int v = 0;
    std::cout << "[ttl] hit a? " << c.get("a", v) << " value=" << v << '\n';
    std::this_thread::sleep_for(120ms);
    std::cout << "[ttl] (after 120ms) hit a? " << c.get("a", v) << '\n';
}

// =============================================================================
//  附加 2：實用範例 — 程式啟動以來的「uptime」
// =============================================================================
//  在 server 統計頁、debug log 常需要「程式跑多久了」。把第一次啟動的
//  time_point 存成 static，之後 now() 減去它即可。
// =============================================================================
static std::chrono::seconds uptime() {
    using namespace std::chrono;
    static const auto bootTime = steady_clock::now();
    return duration_cast<seconds>(steady_clock::now() - bootTime);
}
static void demo_uptime_seconds() {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10ms);
    std::cout << "[uptime] " << uptime().count() << " s\n";
}
