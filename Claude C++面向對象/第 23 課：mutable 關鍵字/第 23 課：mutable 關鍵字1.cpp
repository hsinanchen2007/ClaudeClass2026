// =============================================================================
//  第 23 課：mutable 關鍵字 1  —  邏輯 const 與 bitwise const
// =============================================================================
//
// 【主題資訊 Information】
//   語法：
//     mutable 型別 成員名;      // 該成員即使在 const 成員函式中也可修改
//   限制：mutable 不可用於 static 成員、const 成員、或參考型別成員。
//   標準版本：C++98 起即有(C++11 起也可用於 lambda 的 mutable 修飾)。
//   複雜度：不影響複雜度;mutable 不改變物件佈局,也沒有執行期成本。
//   標頭檔：<string>
//
// 【詳細解釋 Explanation】
//
// 【1. 兩種 const:bitwise const vs 邏輯 const】
//   * bitwise const(位元層級):物件的每一個位元組都不准變。
//     這是編譯器實際強制執行的規則。
//   * 邏輯 const(logical const):物件「對外表現出來的狀態」不變,
//     內部可以有一些不影響觀察結果的變動(快取、統計、鎖)。
//   這兩者大多數時候一致,但並非總是。mutable 存在的理由,
//   就是讓程式設計者能表達「這個成員不屬於物件的邏輯狀態」。
//   本檔 inspectCount_ 正是如此:被查看幾次,不改變這隻怪物是什麼。
//
// 【2. 三個典型的正當用途】
//   (a) 快取 / 記憶化:昂貴的計算結果存下來,下次直接用。
//       物件的邏輯值沒變,只是算得比較快。
//   (b) 統計 / 診斷:存取次數、命中率、最後存取時間。
//   (c) 同步原語:mutable std::mutex,讓 const 成員函式也能上鎖 ——
//       這是標準庫與實務中最常見、也最無爭議的用途。
//   共同點:這些成員都不是「這個物件是什麼」的一部分。
//
// 【3. 判準:拿掉它,物件的相等性會改變嗎?】
//   若兩隻怪物名字、HP、攻擊力都一樣,只是被查看次數不同,
//   它們算不算同一隻怪物?當然算 —— 所以 inspectCount_ 不屬於邏輯狀態,
//   標成 mutable 是合理的。
//   反過來,若你想把 hp_ 標成 mutable 好讓 const 函式能扣血,
//   那就是濫用:HP 顯然是怪物邏輯狀態的一部分。
//   一句話:mutable 用來表達「這個欄位不參與物件的身分認定」。
//
// 【4. mutable 的代價:它讓 const 不再是執行緒安全的保證】
//   標準庫保證「同時對同一物件呼叫 const 成員函式」不會有資料競爭,
//   但那是對「遵守 const 語意」的型別而言。
//   本檔的 printInfo() const 內部有 inspectCount_++,這是個
//   非原子的讀-改-寫。若多個執行緒同時呼叫它,就是資料競爭 ——
//   而資料競爭是未定義行為,不保證任何特定結果(不是「數字算錯」而已)。
//   正確做法是把計數器改成 std::atomic<int>,或以 mutable mutex 保護。
//
// 【概念補充 Concept Deep Dive】
//   * mutable 不影響物件佈局,也不影響 sizeof;它純粹是編譯期的
//     「豁免標記」,告訴編譯器對這個成員不要施加 const 檢查。
//   * const 物件的 mutable 成員也能改 —— 本檔 dragon 宣告為
//     const Monster,inspectCount_ 仍然正常遞增。這是合法的,
//     因為 mutable 成員在標準上就被排除在 const 保護之外。
//     注意這與「const_cast 掉 const 再修改」完全不同:後者若對象
//     本來就宣告為 const,是未定義行為;mutable 則是標準明確允許的。
//   * mutable 也可用在 lambda:[x]() mutable { ++x; } 表示
//     「可以修改按值捕獲的副本」。那是不同的語意,別混淆。
//
// 【注意事項 Pay Attention】
//   1. mutable 不能用於 static 成員、const 成員、參考成員。
//   2. mutable 會削弱 const 的保證:const 成員函式不再等於
//      「這個物件的位元組完全不變」。
//   3. 多執行緒下,mutable 成員的修改必須自行同步
//      (std::atomic 或 mutex),否則是資料競爭 → 未定義行為。
//   4. 不要用 mutable 來規避「這個函式本來就該是非 const」的事實。
//      若修改的是邏輯狀態,正確做法是把函式改成非 const。
//   5. 本檔 std::max 實際定義在 <algorithm>;僅 include <string>
//      仍能編譯是 libstdc++ 間接引入的結果,並非標準保證。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】mutable 與邏輯 const
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. mutable 是什麼?什麼情況該用它?
//     答：mutable 讓某個成員豁免於 const 檢查 —— 即使在 const 成員函式中、
//         即使物件本身宣告為 const,該成員仍可被修改。
//         正當用途有三類:快取(記憶化計算結果)、統計/診斷(存取次數)、
//         同步原語(mutable std::mutex 讓 const 函式也能上鎖)。
//         共同點是:這些成員都不屬於「物件的邏輯狀態」。
//     追問：判準是什麼?→ 拿掉這個成員,兩個物件的相等性判斷會不會改變?
//         不會 → 它不屬於邏輯狀態,可以 mutable;會 → 就不該 mutable。
//
// 🔥 Q2. bitwise const 與 logical const 差在哪?
//     答：bitwise const 是「每個位元組都不變」,這是編譯器實際檢查的;
//         logical const 是「對外可觀察的狀態不變」,這是設計者的意圖。
//         兩者通常一致,但快取、統計、加鎖這些情況會不一致 ——
//         此時 mutable 就是用來告訴編譯器「這裡的差異是刻意的」。
//     追問：C++ 的 const 預設是哪一種?→ 編譯器強制的是 bitwise const
//         (且只有一層深);mutable 是唯一能在語言層面表達
//         「我要的是 logical const」的機制。
//
// ⚠️ 陷阱. 「const 成員函式是唯讀的,所以多執行緒同時呼叫一定安全。」
//     答：只有在型別確實遵守 const 語意時才成立。一旦有了 mutable 成員,
//         const 成員函式就可能在寫入。本檔 printInfo() const 裡的
//         inspectCount_++ 是非原子的讀-改-寫;多執行緒同時呼叫就是
//         資料競爭,而資料競爭屬未定義行為 —— 不保證任何特定結果,
//         不能假設「頂多就是數字少算幾次」。
//     為什麼會錯：把「標準庫保證 const 成員函式可並行呼叫」記成
//         「const 就等於執行緒安全」。實際上那個保證的前提,
//         正是型別自己要維持 const 語意;mutable 一出現,
//         維持這個前提的責任就回到你身上(改用 std::atomic 或 mutex)。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 從缺,理由如下
//   mutable 是 C++ 特有的 const 豁免機制,LeetCode 判題不檢查
//   成員的 const 資格,也沒有題目以「快取是否標 mutable」計分。
//   本課 summary.cpp 會以「昂貴計算的記憶化快取」完整示範 mutable
//   最主要的實務用途,在此不硬掛不相關的題目。
//
// =============================================================================

#include <iostream>
#include <string>
#include <vector>     // 日常實務範例的 DnsCache 需要；明確引入，不倚賴間接引入
using namespace std;

class Monster {
private:
    string name_;
    int hp_;
    int attack_;
    mutable int inspectCount_;   // mutable：const 函數也能改它
                                 // 這裡用來記錄被查看了多少次, 邏輯上不影響怪物狀態

public:
    Monster(const string& name, int hp, int atk)
        : name_(name), hp_(hp), attack_(atk), inspectCount_(0)
    {
    }

    // const 函數——邏輯上只讀, 但可以修改 inspectCount_
    void printInfo() const {
        inspectCount_++;   // ✅ 可以修改！因為是 mutable
        cout << "  " << name_ << " [HP:" << hp_ << " ATK:" << attack_
             << "] (被查看了 " << inspectCount_ << " 次)" << endl;
    }

    const string& getName() const { return name_; }
    int getHp() const { return hp_; }

    // 查看被檢查了幾次, 這個函數也是 const 的，因為它不修改怪物的狀態
    int getInspectCount() const { return inspectCount_; }

    // 非 const：實際修改怪物狀態, 這裡不允許在 const 對象上調用
    void takeDamage(int dmg) {
        hp_ = max(0, hp_ - dmg);
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】DNS 解析快取:const 查詢 + mutable 記憶化
//   情境：resolve() 在邏輯上是「純查詢」—— 同一個網域名永遠對應同一個 IP，
//         呼叫它不改變這份對照表的內容。但每次查詢都重跑一次昂貴的
//         解析流程太浪費，所以把上次結果快取起來。
//   關鍵：快取命中與否，完全不影響 resolve() 的回傳值 ——
//         這正是「邏輯 const」的定義，也是 mutable 最主流的用途。
//   注意：cacheHits_/cacheMisses_ 是統計欄位，同樣不屬於邏輯狀態。
//         本範例為單執行緒；多執行緒下這些 mutable 成員必須自行同步。
// -----------------------------------------------------------------------------
class DnsCache {
private:
    // 真正的邏輯狀態：網域 → IP 的對照表
    vector<string> hosts_;
    vector<string> ips_;

    // 以下三個都不是邏輯狀態，只是加速與觀測用 → mutable
    mutable string lastQuery_;     // 上次查詢的網域（快取鍵）
    mutable string lastResult_;    // 上次查詢的結果（快取值）
    mutable int    cacheHits_;
    mutable int    cacheMisses_;

    // 模擬「昂貴」的查表流程
    string slowLookup(const string& host) const {
        for (size_t i = 0; i < hosts_.size(); ++i) {
            if (hosts_[i] == host) return ips_[i];
        }
        return "0.0.0.0";
    }

public:
    DnsCache() : cacheHits_(0), cacheMisses_(0) {}

    // 建表：真的改變邏輯狀態 → 不能是 const
    void addRecord(const string& host, const string& ip) {
        hosts_.push_back(host);
        ips_.push_back(ip);
        lastQuery_.clear();       // 表變了，快取失效
        lastResult_.clear();
    }

    // 純查詢 → const。內部寫快取與統計，靠 mutable 豁免
    const string& resolve(const string& host) const {
        if (!lastQuery_.empty() && lastQuery_ == host) {
            ++cacheHits_;         // ✅ mutable，const 函式中可寫
            return lastResult_;
        }
        ++cacheMisses_;
        lastQuery_  = host;
        lastResult_ = slowLookup(host);
        return lastResult_;
    }

    // 觀測介面：同樣是 const
    int cacheHits()   const { return cacheHits_; }
    int cacheMisses() const { return cacheMisses_; }
    size_t recordCount() const { return hosts_.size(); }
};

// 以 const& 接收 —— 呼叫端完全看不出來內部有沒有命中快取，
// 這正是「邏輯 const」對外的表現：結果一致，只有速度不同。
static void lookupAll(const DnsCache& dns, const vector<string>& queries) {
    for (const auto& q : queries) {
        cout << "    " << q << " → " << dns.resolve(q) << endl;
    }
}

int main() {
    cout << "=== mutable 基本用法 ===" << endl;

    const Monster dragon("火龍", 500, 60);  // const 對象！

    // 可以調用 const 函數
    dragon.printInfo();
    dragon.printInfo();
    dragon.printInfo();

    cout << "  總共被查看：" << dragon.getInspectCount() << " 次" << endl;

    // dragon.takeDamage(10);  // ❌ 編譯錯誤！const 對象

    // ─────────────────────────────────────────────────────────
    cout << "\n=== 日常實務：DNS 快取（const 查詢 + mutable 記憶化）===" << endl;

    DnsCache dns;
    dns.addRecord("api.example.com", "10.0.0.11");
    dns.addRecord("cdn.example.com", "10.0.0.22");
    dns.addRecord("db.internal",     "10.0.0.33");
    cout << "  已載入 " << dns.recordCount() << " 筆記錄" << endl;

    // 整份查詢都走 const& —— 呼叫端無從得知內部有沒有寫入快取
    const DnsCache& readOnly = dns;

    cout << "\n--- 連續查詢（含重複）---" << endl;
    vector<string> queries = {
        "api.example.com",   // miss（第一次）
        "api.example.com",   // hit （與上次相同）
        "api.example.com",   // hit
        "cdn.example.com",   // miss（鍵換了）
        "nope.example.com"   // miss（查無 → 0.0.0.0）
    };
    lookupAll(readOnly, queries);

    cout << "\n--- 快取統計（同樣由 const 介面取得）---" << endl;
    cout << "    命中：" << readOnly.cacheHits()
         << "  未命中：" << readOnly.cacheMisses() << endl;
    cout << "  ↑ 這些寫入全發生在 const 成員函式內，靠 mutable 豁免；" << endl;
    cout << "    回傳的 IP 完全不受快取狀態影響，這就是邏輯 const。" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 23 課：mutable 關鍵字1.cpp" -o l23_1
// 執行: ./l23_1        (rc=0)

// === 預期輸出 ===
// === mutable 基本用法 ===
//   火龍 [HP:500 ATK:60] (被查看了 1 次)
//   火龍 [HP:500 ATK:60] (被查看了 2 次)
//   火龍 [HP:500 ATK:60] (被查看了 3 次)
//   總共被查看：3 次
//
// === 日常實務：DNS 快取（const 查詢 + mutable 記憶化）===
//   已載入 3 筆記錄
//
// --- 連續查詢（含重複）---
//     api.example.com → 10.0.0.11
//     api.example.com → 10.0.0.11
//     api.example.com → 10.0.0.11
//     cdn.example.com → 10.0.0.22
//     nope.example.com → 0.0.0.0
//
// --- 快取統計（同樣由 const 介面取得）---
//     命中：2  未命中：3
//   ↑ 這些寫入全發生在 const 成員函式內，靠 mutable 豁免；
//     回傳的 IP 完全不受快取狀態影響，這就是邏輯 const。
