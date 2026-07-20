// =============================================================================
//  第 22 課：const 成員函數 6  —  const 呼叫鏈與衍生查詢
// =============================================================================
//
// 【主題資訊 Information】
//   呼叫方向規則:
//     const 成員函式  →  只能呼叫 const 成員函式        (✅ 單向)
//     非 const 成員函式 → const 與非 const 都能呼叫       (✅ 雙向)
//   標準版本：C++98 起即有;本檔用到 std::to_string(C++11)。
//   複雜度：本檔各查詢皆 O(1)。
//   標頭檔：<string>(std::max 實際屬 <algorithm>,見注意事項)
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 const 只能往 const 呼叫】
//   在 const 成員函式中,this 是 const T* const。呼叫另一個成員函式時,
//   等於要把這個 const 指標當成該函式的隱含 this 傳過去 ——
//   而非 const 成員函式要求的是 T* const。const 無法隱式轉成非 const,
//   所以呼叫直接不成立。
//   反過來,非 const 成員函式的 this 是 T*,要傳給需要 const T* 的函式
//   完全沒問題(加上 const 永遠安全),因此非 const 可以呼叫 const。
//   一句話:const 是單向可傳染的,方向是「非 const → const」。
//
// 【2. 衍生查詢(derived query)為什麼特別適合 const】
//   getHpPercent()、getStatusText() 都不是單純回傳成員,
//   而是「由其他成員算出來的值」。這類函式最容易被誤標成非 const ——
//   因為它們裡面有邏輯、有計算,直覺上「好像做了什麼事」。
//   但判準只有一個:它有沒有改變物件的可觀察狀態?沒有,就該是 const。
//   本檔 getStatusText() 內部組字串、比大小、串接文字,卻仍然是 const,
//   正是這個判準的最佳示範。
//
// 【3. 一層一層堆疊的查詢:const 讓組合變得安全】
//   printFullStatus() const → 呼叫 getStatusText() const
//                           → 呼叫 getName()/getLevel()/getHpPercent() const
//                           → getHpPercent() 又呼叫 getHp()/getMaxHp() const
//   整條鏈四層深,全部是 const。因為每一層都保證不改狀態,
//   所以整條鏈可以安全地在 const 物件上執行。
//   若最底層的 getHp() 漏了 const,這四層會一起垮掉 ——
//   這就是第 3 號檔案講的「由內而外修 const」的實際情境。
//
// 【4. 非 const 函式呼叫 const 函式:takeDamage() 的寫法】
//   takeDamage() 會改 hp_,所以不能是 const;但它內部呼叫了
//   getName() 與 getStatusText() 這兩個 const 函式,完全合法。
//   實務上這是很常見的組合:修改狀態之後,立刻用唯讀查詢產生 log 訊息。
//
// 【概念補充 Concept Deep Dive】
//   * getStatusText() 回傳 std::string(值)而非 const&,這是正確的 ——
//     它回傳的是「臨時算出來的新字串」,沒有對應的成員可以參考。
//     若硬寫成 const string& 回傳區域變數,會產生懸空參考(UB)。
//     這是回傳 const& 的重要邊界:只有「已存在的成員」才能回傳參考。
//   * getHpPercent() 先 static_cast<double> 再相除,是為了避免整數除法
//     把 60/200 算成 0。這與 const 無關,但是同樣常見的計算錯誤。
//   * 本檔 hero.takeDamage(120) 後 hp_ 為 80,80/200 = 40% → [受傷];
//     再扣 60 後為 20,20/200 = 10% → [瀕死]。分段門檻在 getStatusText()
//     中以 > 50 / > 20 / > 0 判斷,注意 10% 落在「> 0」那一段。
//
// 【注意事項 Pay Attention】
//   1. std::max 定義在 <algorithm>;本檔僅 include <string> 仍能編譯,
//      是 libstdc++ 標頭間接引入的結果,並非標準保證。嚴謹寫法應明確引入。
//   2. 衍生查詢容易被誤標成非 const。判準是「有沒有改變可觀察狀態」,
//      不是「裡面有沒有做運算」。
//   3. 回傳計算結果時要回傳「值」,不能回傳區域變數的參考(懸空 → UB)。
//   4. const 函式若需要快取計算結果(避免重複運算),就會撞上
//      「邏輯 const」問題 —— 那正是第 23 課 mutable 要解決的。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】const 呼叫鏈
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. const 成員函式能不能呼叫非 const 成員函式?反過來呢?
//     答：不能;反過來可以。因為 const 成員函式的 this 是 const T* const,
//         而非 const 成員函式需要 T* const —— const 無法隱式轉成非 const,
//         呼叫不成立。反方向則是加上 const,永遠安全。
//         所以 const 是單向傳染的:非 const → const 可以,反之不行。
//     追問：那 const 函式真的需要改狀態時怎麼辦?→ 若改的是「不影響
//         外界可觀察狀態」的東西(快取、統計計數),可用 mutable 成員,
//         這是第 23 課的主題;否則就代表這個函式本來就不該是 const。
//
// 🔥 Q2. 一個函式裡有一堆計算與字串組裝,它還能是 const 嗎?
//     答：能,而且通常就該是。判準是「有沒有改變物件的可觀察狀態」,
//         不是「裡面有沒有做事」。本檔 getStatusText() 組字串、比大小、
//         串接文字,但從頭到尾只讀成員、不寫成員,所以是標準的 const 函式。
//     追問：那它為什麼回傳 string 而不是 const string&?→ 因為它回傳的是
//         臨時算出來的新字串,沒有對應成員可參考;回傳區域變數的參考
//         會產生懸空參考,屬未定義行為。
//
// ⚠️ 陷阱. 「getHpPercent() 裡面有除法運算,算是在『做事』,應該不能加 const 吧?」
//     答：可以加,而且必須加。const 管的是「物件狀態有沒有被改變」,
//         跟函式內部忙不忙、有沒有運算完全無關。它讀 hp_ 與 maxHp_、
//         算出一個新的 double 回傳,一個成員都沒動 —— 這正是 const 的定義。
//         若因為「裡面有運算」就不加 const,printFullStatus()、
//         getStatusText() 整條呼叫鏈都會跟著不能是 const,
//         最後連 const 物件都印不出狀態。
//     為什麼會錯：把 const 理解成「這個函式很單純/很輕量」,
//         用「複雜度」而非「是否修改狀態」當判準。
//         實際上再長再複雜的函式,只要不寫成員,就該是 const。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 從缺,理由如下
//   const 呼叫鏈是型別系統議題,LeetCode 判題不檢查成員函式的 const 資格。
//   本課 summary.cpp 會以 705. Design HashSet 說明查詢/修改介面切分
//   如何自然對應到 const / 非 const,在此不重複掛題。
//
// =============================================================================

#include <iostream>
#include <string>
using namespace std;

class GameCharacter {
private:
    string name_;
    int hp_;
    int maxHp_;
    int level_;

public:
    GameCharacter(const string& name, int maxHp, int level)
        : name_(name), hp_(maxHp), maxHp_(maxHp), level_(level)
    {
    }

    // ====== const 函數群 ======
    const string& getName() const { return name_; }
    int getHp() const { return hp_; }
    int getMaxHp() const { return maxHp_; }
    int getLevel() const { return level_; }

    double getHpPercent() const {
        // const 函數可以調用其他 const 函數 ✅
        return (static_cast<double>(getHp()) / getMaxHp()) * 100.0;
    }

    string getStatusText() const {
        // 可以調用 getName(), getHpPercent() — 都是 const ✅
        string status = getName() + " Lv." + to_string(getLevel());
        double pct = getHpPercent();

        if (pct > 50.0)
            status += " [健康]";
        else if (pct > 20.0)
            status += " [受傷]";
        else if (pct > 0.0)
            status += " [瀕死]";
        else
            status += " [死亡]";

        return status;
    }

    void printFullStatus() const {
        // const 函數可以調用其他 const 函數
        cout << "  " << getStatusText() << endl;
        cout << "  HP: " << getHp() << "/" << getMaxHp()
             << " (" << getHpPercent() << "%)" << endl;

        // 不能調用非 const 函數：
        // takeDamage(10);  // ❌ 編譯錯誤！
    }

    // ====== 非 const 函數 ======
    void takeDamage(int dmg) {
        hp_ = max(0, hp_ - dmg);
        // 非 const 函數可以調用 const 函數 ✅
        cout << "  " << getName() << " 受傷！" << getStatusText() << endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】服務健康度:四層深的 const 衍生查詢
//   情境：監控系統要從「請求總數 / 失敗數 / 累計延遲」這三個原始計數，
//         推導出成功率、平均延遲、SLA 是否達標、健康度等級。
//   重點：每一層都是「由更底層的 const 查詢算出來的」，
//         整條鏈全部 const，因此可以安全地在 const 快照上執行 ——
//         監控系統正是典型的「拿到一份快照就只准讀」的場景。
// -----------------------------------------------------------------------------
class ServiceHealth {
private:
    string serviceName_;
    long   totalRequests_;
    long   failedRequests_;
    long   totalLatencyMs_;

public:
    explicit ServiceHealth(const string& name)
        : serviceName_(name), totalRequests_(0), failedRequests_(0), totalLatencyMs_(0) {}

    // 修改介面：會改狀態 → 不能是 const
    void record(bool success, long latencyMs) {
        ++totalRequests_;
        if (!success) ++failedRequests_;
        totalLatencyMs_ += latencyMs;
    }

    // 第 1 層：原始計數
    const string& name()     const { return serviceName_; }
    long totalRequests()     const { return totalRequests_; }
    long failedRequests()    const { return failedRequests_; }

    // 第 2 層：由第 1 層算出
    double successRate() const {
        if (totalRequests() == 0) return 100.0;
        return 100.0 * static_cast<double>(totalRequests() - failedRequests())
                     / static_cast<double>(totalRequests());
    }
    double avgLatencyMs() const {
        if (totalRequests() == 0) return 0.0;
        return static_cast<double>(totalLatencyMs_) / static_cast<double>(totalRequests());
    }

    // 第 3 層：由第 2 層算出（SLA：成功率 ≥ 99%，平均延遲 ≤ 200ms）
    bool meetsSla() const { return successRate() >= 99.0 && avgLatencyMs() <= 200.0; }

    // 第 4 層：由第 3 層與第 2 層算出
    string healthGrade() const {
        if (totalRequests() == 0)  return "無資料";
        if (meetsSla())            return "健康";
        if (successRate() >= 95.0) return "降級";
        return "異常";
    }
};

// 監控報表：以 const& 接收 → 整條四層查詢鏈全部走 const 版本
static void printHealthReport(const ServiceHealth& h) {
    cout << "    服務：" << h.name()
         << "  請求數：" << h.totalRequests()
         << "  失敗數：" << h.failedRequests() << endl;
    cout << "    成功率：" << h.successRate() << "%"
         << "  平均延遲：" << h.avgLatencyMs() << "ms" << endl;
    cout << "    SLA 達標？" << (h.meetsSla() ? "是" : "否")
         << "  健康度：" << h.healthGrade() << endl;
    // h.record(true, 10);   // ❌ 編譯錯誤：const& 不能呼叫非 const 成員函式
}

int main() {
    cout << "=== const 函數調用鏈 ===" << endl;

    GameCharacter hero("戰士", 200, 5);

    cout << "\n--- 初始狀態 ---" << endl;
    hero.printFullStatus();

    cout << "\n--- 受傷後 ---" << endl;
    hero.takeDamage(120);
    hero.printFullStatus();

    hero.takeDamage(60);
    hero.printFullStatus();

    // ─────────────────────────────────────────────────────────
    cout << "\n=== 日常實務：服務健康度（四層 const 衍生查詢）===" << endl;

    ServiceHealth api("payment-api");
    // 100 筆請求，其中 3 筆失敗；延遲刻意做出差異
    for (int i = 0; i < 100; ++i) {
        bool ok = (i % 34 != 0);          // i = 0, 34, 68 → 三筆失敗
        api.record(ok, ok ? 120 : 900);   // 失敗的那幾筆延遲特別高
    }

    cout << "\n--- 成功率 97%（未達 99% SLA）---" << endl;
    printHealthReport(api);

    ServiceHealth healthy("auth-api");
    for (int i = 0; i < 100; ++i) {
        api.totalRequests();              // 只是示範 const 查詢隨時可呼叫
        healthy.record(true, 80);
    }
    cout << "\n--- 全部成功、延遲 80ms（達標）---" << endl;
    printHealthReport(healthy);

    // const 快照：整條四層查詢鏈依然可用
    const ServiceHealth& snapshot = healthy;
    cout << "\n--- 以 const 快照觀察 ---" << endl;
    cout << "    健康度：" << snapshot.healthGrade() << endl;
    // snapshot.record(false, 100);   // ❌ 編譯錯誤

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 22 課：const 成員函數6.cpp" -o l22_6
// 執行: ./l22_6        (rc=0)

// === 預期輸出 ===
// === const 函數調用鏈 ===
//
// --- 初始狀態 ---
//   戰士 Lv.5 [健康]
//   HP: 200/200 (100%)
//
// --- 受傷後 ---
//   戰士 受傷！戰士 Lv.5 [受傷]
//   戰士 Lv.5 [受傷]
//   HP: 80/200 (40%)
//   戰士 受傷！戰士 Lv.5 [瀕死]
//   戰士 Lv.5 [瀕死]
//   HP: 20/200 (10%)
//
// === 日常實務：服務健康度（四層 const 衍生查詢）===
//
// --- 成功率 97%（未達 99% SLA）---
//     服務：payment-api  請求數：100  失敗數：3
//     成功率：97%  平均延遲：143.4ms
//     SLA 達標？否  健康度：降級
//
// --- 全部成功、延遲 80ms（達標）---
//     服務：auth-api  請求數：100  失敗數：0
//     成功率：100%  平均延遲：80ms
//     SLA 達標？是  健康度：健康
//
// --- 以 const 快照觀察 ---
//     健康度：健康
