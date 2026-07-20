// =============================================================================
//  第 23 課：mutable 關鍵字 4  —  mutable 的濫用:當 const 失去意義時
// =============================================================================
//
// 【主題資訊 Information】
//   語法    ： mutable <型別> <成員名>;
//   標準版本： C++98 起
//   標頭檔  ： 無(語言關鍵字);本檔的 std::max 需要 <algorithm>
//   本檔性質： 這是一份「對照組」教材。BadExample 可以編譯、可以執行,
//             但它是設計上的錯誤示範;GoodExample 才是應該學的寫法。
//
// 【詳細解釋 Explanation】
//
// 【1. 這份反面教材錯在哪裡】
//   BadExample 把 hp_ 與 gold_ 標成 mutable,於是 takeDamage() 和 spendGold()
//   都能宣告成 const。程式編得過、跑得動,但結果是:
//       const BadExample bad(...);
//       bad.takeDamage(30);      // const 物件竟然掉血
//       bad.spendGold(200);      // const 物件竟然花錢
//   const 在這裡完全沒有攔截到任何東西。它變成一個純粹的裝飾字,
//   讀程式的人看到 const 會以為物件不會變,而事實正好相反——
//   這比不寫 const 更糟,因為它主動誤導人。
//
// 【2. 判斷「該不該用 mutable」的操作型準則】
//   問一句話:把這個成員拿掉(或凍結不准改),用同一串公開呼叫去觀察,
//   回傳值序列會不會不同?
//     * viewCount_(GoodExample):凍結它,只是印出來的次數不動,
//       HP/Gold 這些真正的答案完全一樣 → 不是邏輯狀態 → 可以 mutable。
//     * hp_(BadExample):凍結它,print() 印出的血量就變了,
//       戰鬥結果也跟著變 → 是邏輯狀態 → 絕對不能 mutable。
//   簡單說:mutable 只給「拿掉之後程式只會變慢、不會變錯」的東西。
//
// 【3. GoodExample 為什麼是對的】
//   它把兩類成員分開:
//     * hp_ / gold_ 是核心資料,不加 mutable。於是 takeDamage()/spendGold()
//       必須是非 const 成員函式,編譯器就會在 const 物件上正確擋下它們
//       (本檔第 81~82 行被註解掉的兩行,取消註解就會編譯失敗)。
//     * viewCount_ 是統計用的輔助資料,加 mutable。於是 print() 可以是 const,
//       const 物件也能列印,同時仍能累計查看次數。
//   這就是 const 該有的樣子:擋住該擋的,放行該放的。
//
// 【4. 「編譯得過」不等於「設計正確」】
//   這是本檔最重要的一課。C++ 給你的工具(mutable、const_cast、reinterpret_cast、
//   friend)幾乎都能讓你繞過某層保護。編譯器不會、也無法判斷你繞得有沒有道理。
//   保護機制的價值來自於「大家都遵守」,一旦開了後門,整條防線就失效。
//
// 【概念補充 Concept Deep Dive】
//   (A) const 物件與唯讀記憶體。
//       編譯器對於「宣告為 const 且可在編譯期決定初值」的物件,有機會把它放進
//       .rodata 唯讀區段。但只要類別含有 mutable 成員,標準就允許修改該成員,
//       所以編譯器不能把這種物件整個放進唯讀區——BadExample 因此才不會當場出錯。
//       這說明 mutable 是「合法但可能被誤用」,而不是「危險到會崩潰」。
//       真正危險的是對 沒有 mutable 的 const 物件用 const_cast 寫入,那是 UB(見檔案 6)。
//
//   (B) const 成員函式與多載解析。
//       const 與非 const 成員函式可以同名多載,編譯器依物件的常數性挑選:
//           void print();        // 非 const 物件優先選這個
//           void print() const;  // const 物件只能選這個
//       這是 std::vector 能同時提供 operator[] 兩個版本(回傳 T& 與 const T&)的機制。
//
//   (C) 為什麼 GoodExample 的 hp_ 用 std::max(0, hp_ - dmg)。
//       避免血量變成負值。注意 std::max 定義在 <algorithm>,
//       原始碼只 include 了 <iostream> 與 <string> 卻能編譯,是因為 libstdc++ 的
//       標頭檔之間有間接引入(transitive include)。這是實作細節,不是標準保證——
//       換一個標準函式庫實作就可能編譯失敗。本檔已補上 <algorithm>。
//
//   (D) mutable 的另一個完全不同的意義。
//       C++11 起,mutable 也用在 lambda 上:[x]() mutable { ++x; }。
//       那是指「允許修改以值捕獲的複本」,和資料成員的 mutable 是兩回事,
//       只是共用同一個關鍵字。面試常拿這點來確認你是否真的理解而非死背。
//
// 【注意事項 Pay Attention】
//   1. mutable 成員應該盡可能少。一個類別若有超過兩三個 mutable 成員,
//      通常代表 const 的界線沒有想清楚。
//   2. 不要用 mutable 來「讓程式編得過」。編不過往往是設計在提醒你:
//      這個函式其實會改變狀態,不該宣告成 const。
//   3. 本檔的 BadExample 可以正常執行,不會崩潰,也不是 UB——
//      它只是設計錯誤。請不要把「能跑」當成「沒問題」的證據。
//   4. GoodExample 中被註解掉的兩行呼叫,取消註解會產生編譯錯誤。
//      這正是它比 BadExample 好的地方:錯誤在編譯期就被抓到。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】mutable 的濫用與 const 正確性
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 給你一個類別,你怎麼判斷某個成員該不該標成 mutable?
//     答：用「可觀察性」測試:凍結這個成員不准改,再用同一串公開呼叫觀察,
//         回傳值序列有沒有變?沒變 → 它不是邏輯狀態,可以 mutable(快取、計數器、mutex);
//         有變 → 它是邏輯狀態,絕不能 mutable。
//         本檔的 viewCount_ 通過測試,hp_ 沒有通過。
//     追問：那查看次數如果要拿來當「熱門商品排行」的依據呢?
//         → 那它就變成外部觀察得到的邏輯狀態了,就不該再是 mutable。
//         同一個成員該不該 mutable,取決於它在系統中扮演什麼角色,不是看它的型別。
//
// 🔥 Q2. BadExample 能編譯也能執行,那它到底算不算 bug?
//     答：它不是語言層級的錯誤,不是 UB,執行結果也完全確定。
//         它是設計層級的錯誤:const 對呼叫端做出了「不會改變」的承諾,實作卻違背了。
//         後果是所有讀到 const BadExample& 的人都會做出錯誤假設,
//         而編譯器再也幫不上忙。
//     追問：那要怎麼在 code review 抓出這種問題?
//         → 看到 mutable 就檢查兩件事:這個成員有沒有出現在任何 getter 的回傳值裡?
//         有沒有 const 成員函式的名字聽起來像動詞(takeDamage、spend、update)?
//         兩者任一成立就要警覺。
//
// ⚠️ 陷阱. 「const 成員函式不能修改物件,所以 const 物件在任何情況下都不會變。」
//     答：錯,而且錯兩層。第一,mutable 成員在 const 成員函式裡可以被合法修改,
//         BadExample 就是實證。第二,const 只約束「透過這個型別的介面」的存取;
//         若別處還持有非 const 的參考或指標指向同一物件,照樣可以改它。
//     為什麼會錯：多數人把 const 理解成「這塊記憶體是唯讀的」,
//         但 const 其實是「透過這條存取路徑不能寫」的性質,是綁在路徑上而不是綁在物件上。
//         唯一真正接近「唯讀記憶體」的情況,是宣告為 const 且無 mutable 成員的物件,
//         而對那種物件用 const_cast 寫入才是 UB。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <algorithm>   // std::max —— 原始碼漏了這個,靠間接引入才編得過(見概念補充 C)
#include <vector>
#include <map>
#include <mutex>
#include <thread>
using namespace std;

// ===== 濫用 mutable 的反面教材 =====
class BadExample {
private:
    string name_;
    mutable int hp_;        // ❌ 把核心數據設為 mutable
    mutable int gold_;      // ❌ 把核心數據設為 mutable

public:
    BadExample(const string& name, int hp, int gold)
        : name_(name), hp_(hp), gold_(gold) {}

    // 這些操作明明在修改對象狀態，卻偽裝成 const！
    void takeDamage(int dmg) const {   // ❌ 不應該是 const
        hp_ -= dmg;
        cout << "  " << name_ << " 受傷 HP:" << hp_ << endl;
    }

    void spendGold(int amount) const { // ❌ 不應該是 const
        gold_ -= amount;
        cout << "  花費 " << amount << " 金幣" << endl;
    }

    void print() const {
        cout << "  " << name_ << " HP:" << hp_
             << " Gold:" << gold_ << endl;
    }
};

// ===== 正確的設計 =====
class GoodExample {
private:
    string name_;
    int hp_;              // 核心數據：不用 mutable
    int gold_;            // 核心數據：不用 mutable
    mutable int viewCount_;  // ✅ 只有輔助數據用 mutable

public:
    GoodExample(const string& name, int hp, int gold)
        : name_(name), hp_(hp), gold_(gold), viewCount_(0) {}

    // 修改狀態的函數：不是 const
    void takeDamage(int dmg) {
        hp_ = max(0, hp_ - dmg);
        cout << "  " << name_ << " 受傷 HP:" << hp_ << endl;
    }

    void spendGold(int amount) {
        if (amount <= gold_) {
            gold_ -= amount;
            cout << "  花費 " << amount << " 金幣" << endl;
        }
    }

    // 只讀函數：是 const，只有 viewCount_ 用 mutable
    void print() const {
        viewCount_++;   // ✅ 合理的 mutable 用途
        cout << "  " << name_ << " HP:" << hp_
             << " Gold:" << gold_
             << " (查看次數:" << viewCount_ << ")" << endl;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔從缺,理由如下
//   本檔談的是 const 正確性與 API 設計品質,屬於「程式碼是否誤導讀者」的層面。
//   LeetCode 只驗證輸入輸出是否正確,一份把核心資料標成 mutable 的解答
//   照樣能全部 AC——也就是說,這個主題的對錯 在 LeetCode 上根本量不出來。
//   硬掛一題只會傳達錯誤訊息(彷彿設計品質可以用測資驗證),因此刻意從缺,
//   改用下方「mutable std::mutex」這個業界最常見的正當用法作結。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】mutable std::mutex —— mutable 最無爭議的正當用途
//   情境：服務內的計數器登錄表(metrics registry)。工作執行緒不斷回報事件,
//         監控執行緒定期讀取快照送到儀表板。
//   為什麼 mutex 一定要 mutable：
//         讀取快照的 snapshot() 對呼叫者是純查詢,理應宣告成 const;
//         但它必須先上鎖才能安全讀取,而 lock() 會修改 mutex 的內部狀態。
//         成員函式是 const → this 是 const T* → mutex_ 也是 const → 不能 lock()。
//         唯一乾淨的解法就是把 mutex 標成 mutable。
//         這也是為什麼「mutable 最典型的用途」在現代 C++ 幾乎都指向 mutex。
//   為什麼這不算濫用：
//         用上面的可觀察性測試——把鎖拿掉,單執行緒下的回傳值序列完全不變,
//         只是多執行緒時會有 data race。它影響的是安全性與效能,不是邏輯答案。
//   注意：本範例的輸出刻意設計成決定性的(總數 = 執行緒數 × 每緒次數),
//         不去印任何與排程有關的中間值。
// -----------------------------------------------------------------------------
class MetricsRegistry {
private:
    mutable mutex mtx_;                 // ✅ 正當的 mutable：保護用的鎖
    map<string, long long> counters_;   // 核心資料：不是 mutable

public:
    void record(const string& event, long long delta = 1) {
        lock_guard<mutex> lk(mtx_);
        counters_[event] += delta;
    }

    // 純查詢，所以是 const；靠 mutable mutex 才能在 const 函式裡上鎖
    long long valueOf(const string& event) const {
        lock_guard<mutex> lk(mtx_);
        auto it = counters_.find(event);
        return it == counters_.end() ? 0 : it->second;
    }

    // 回傳整份快照，同樣是 const
    map<string, long long> snapshot() const {
        lock_guard<mutex> lk(mtx_);
        return counters_;               // 在鎖內複製，複本交給呼叫者慢慢用
    }
};

int main() {
    cout << "=== mutable 濫用 vs 正確使用 ===" << endl;

    // 濫用的後果：const 失去意義
    cout << "\n--- 濫用 mutable ---" << endl;
    const BadExample bad("壞設計", 100, 500);
    bad.takeDamage(30);     // const 對象居然能受傷？！
    bad.spendGold(200);     // const 對象居然能花錢？！
    bad.print();
    cout << "  const 完全失去了保護作用！" << endl;

    // 正確的設計
    cout << "\n--- 正確使用 ---" << endl;
    const GoodExample good("好設計", 100, 500);
    // good.takeDamage(30);   // ❌ 編譯錯誤！正確攔截
    // good.spendGold(200);   // ❌ 編譯錯誤！正確攔截
    good.print();             // ✅ 只有查看次數變化
    good.print();

    cout << "\n=== 日常實務: mutable std::mutex（正當用途）===" << endl;
    MetricsRegistry metrics;

    const int kThreads = 4;
    const int kPerThread = 1000;
    vector<thread> workers;
    workers.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        workers.emplace_back([&metrics] {
            for (int j = 0; j < kPerThread; ++j) {
                metrics.record("http.request");
            }
        });
    }
    for (thread& t : workers) t.join();

    // 全部 join 之後才讀，輸出因此是決定性的：4 * 1000 = 4000
    const MetricsRegistry& ro = metrics;      // 用 const 參考讀取
    cout << "  http.request 總數 = " << ro.valueOf("http.request")
         << "（預期 " << kThreads * kPerThread << "）" << endl;
    cout << "  未記錄過的事件 = " << ro.valueOf("http.error") << endl;

    metrics.record("http.error", 7);
    map<string, long long> snap = ro.snapshot();
    cout << "  快照內容：" << endl;
    for (const auto& kv : snap) {
        cout << "    " << kv.first << " = " << kv.second << endl;
    }
    cout << "  （valueOf 與 snapshot 都是 const 成員函式，靠 mutable mutex 上鎖）" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread 第\ 23\ 課：mutable\ 關鍵字4.cpp -o mutable4
//   （本檔用到 std::thread，必須加 -pthread）

// 【輸出說明】
//   1. BadExample 那三行示範的是「設計錯誤」而非 UB:結果完全確定,
//      const 物件真的掉血、真的花錢——這正是它危險的地方。
//   2. GoodExample 中被註解掉的 good.takeDamage(30) / good.spendGold(200),
//      取消註解會產生編譯錯誤(本機 g++ 15.2 實測):
//        error: passing 'const GoodExample' as 'this' argument discards qualifiers
//      這是它比 BadExample 好的證據——錯誤在編譯期就被攔下。
//   3. 多執行緒段落刻意設計成決定性:4 條執行緒各記錄 1000 次,
//      全部 join 之後才讀取,所以總數必為 4000。
//      過程中誰先誰後由 OS 排程決定,但那些中間狀態本檔完全不輸出。
//      連續執行 6 次 md5 相同。
//   4. 快照內容依 std::map 的鍵排序輸出(http.error 在 http.request 之前),
//      這是 std::map 的規格保證,不是巧合。

// === 預期輸出 ===
// === mutable 濫用 vs 正確使用 ===
//
// --- 濫用 mutable ---
//   壞設計 受傷 HP:70
//   花費 200 金幣
//   壞設計 HP:70 Gold:300
//   const 完全失去了保護作用！
//
// --- 正確使用 ---
//   好設計 HP:100 Gold:500 (查看次數:1)
//   好設計 HP:100 Gold:500 (查看次數:2)
//
// === 日常實務: mutable std::mutex（正當用途）===
//   http.request 總數 = 4000（預期 4000）
//   未記錄過的事件 = 0
//   快照內容：
//     http.error = 7
//     http.request = 4000
//   （valueOf 與 snapshot 都是 const 成員函式，靠 mutable mutex 上鎖）
