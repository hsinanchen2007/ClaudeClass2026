// =============================================================================
//  第 23 課：mutable 關鍵字 5  —  綜合應用:計數器 + 延遲初始化 + 快取一次到位
// =============================================================================
//
// 【主題資訊 Information】
//   語法    ： mutable <型別> <成員名>;
//   標準版本： C++98 起
//   標頭檔  ： 無(語言關鍵字)
//   本檔定位： 把前面三個檔案(快取、延遲初始化、正確 vs 濫用)整合成一個
//             接近真實專案的類別,示範多個 mutable 成員如何共存而不互相干擾。
//
// 【詳細解釋 Explanation】
//
// 【1. 這個類別的成員被切成兩層,這是關鍵】
//   MonsterEntry 刻意把成員分成兩個明確的區塊:
//     * 邏輯狀態(圖鑑資料):name_、element_、baseHp_、baseAttack_、rarity_
//       —— 建構後永不改變,也沒有任何 setter。它們定義了「這隻怪物是什麼」。
//     * 輔助狀態(mutable):viewCount_、detailGenerated_、detailCache_
//       —— 只影響效能與統計,不影響「這隻怪物是什麼」。
//   任何人要判斷這個類別的 mutable 用得對不對,只要問:
//   把第二區塊全部凍結,getName()/getRarity() 的答案會變嗎?不會。合格。
//
// 【2. 為什麼所有查詢函式都能是 const,而這件事很重要】
//   因為全部查詢都是 const,showEncyclopedia() 與 showDetail() 才能接受
//   const MonsterEntry& / const MonsterEntry[]。這是 const 正確性的實際回報:
//   函式簽章上寫著 const,呼叫端就知道「這個函式不會偷改我的圖鑑」,
//   而編譯器會強制執行這個承諾(對非 mutable 的成員而言)。
//   如果 printBrief() 因為要累計次數就被迫拿掉 const,
//   整條呼叫鏈的 const 都得跟著拿掉——這就是 const 傳染性的反面案例。
//
// 【3. 兩個 mutable 用途在同一個函式裡共存】
//   getDetail() 同時做了兩件事:
//       viewCount_++;                      // 計數器:每次呼叫都要動
//       if (!detailGenerated_) generateDetail();   // 延遲初始化:只做一次
//   注意這兩者的節奏不同——一個每次都變,一個只變一次。
//   把它們寫在同一個 const 函式裡完全沒問題,因為它們都不屬於邏輯狀態。
//   輸出中「炎龍王被查看 3 次,但詳細資料只生成 1 次」就是這個差異的證據。
//
// 【4. 延遲初始化在這裡為什麼安全(不需要失效邏輯)】
//   detailCache_ 的內容完全由 name_/element_/baseHp_/baseAttack_/rarity_ 決定,
//   而這五個成員沒有任何 setter,建構後就凍結了。
//   來源不變 ⇒ 快取永遠有效 ⇒ 不需要 invalidate()。
//   這正是檔案 3 講的「延遲初始化是來源不可變前提下的快取特例」。
//   反過來說,只要日後有人加上 setRarity(),就必須同時把 detailGenerated_
//   設回 false,否則星級改了、詳細資料卻還是舊的。
//
// 【概念補充 Concept Deep Dive】
//   (A) showEncyclopedia(const MonsterEntry entries[], int count) 的真相。
//       函式參數中的陣列會退化(decay)成指標,所以這個簽章實際等同於
//           void showEncyclopedia(const MonsterEntry* entries, int count);
//       中括號裡就算寫了大小也會被忽略,而且 sizeof(entries) 得到的是指標大小
//       (本機 x86-64 為 8),不是陣列大小。這就是必須額外傳 count 的原因。
//       現代寫法可改用 std::span(C++20) 或 const std::vector<MonsterEntry>&,
//       把大小與資料綁在一起,從根本避免傳錯 count。
//
//   (B) const 的位置意義。
//       const MonsterEntry* p;   // 指向 const 物件的指標:不能改 *p,可以改 p
//       MonsterEntry* const p;   // const 指標:可以改 *p,不能改 p
//       讀法是「由右往左」:第二個的 const 緊貼 p,所以是 p 本身為常數。
//
//   (C) MonsterEntry entries[COUNT] = { ... } 這一行做了什麼。
//       C++17 起,以暫時物件初始化陣列元素時會直接在目的地建構(保證的複製省略),
//       不會產生「先建暫時物件、再複製、再銷毀」的過程。
//       C++17 以前雖然編譯器多半也會最佳化掉,但那是允許的最佳化而非標準保證。
//
//   (D) 為什麼 detailCache_ 用 += 一段段接。
//       每次 += 可能觸發重新配置。libstdc++ 的 std::string 有 SSO(short string
//       optimization),本機實測門檻是 15 個 byte(含以下不配置 heap);
//       這裡的字串遠超過門檻,所以會有數次 heap 配置。
//       在意的話可先 detailCache_.reserve(256)。這是實作定義的數值,
//       其他標準函式庫(例如 MSVC 的 15、libc++ 的 22)不同。
//
// 【注意事項 Pay Attention】
//   1. 這個類別的執行緒安全性:printBrief()/getDetail() 都會寫 mutable 成員,
//      多執行緒同時呼叫是 data race(UB)。要安全需再加 mutable std::mutex。
//   2. 一旦有人為這個類別加上任何 setter,就必須重新檢視 detailGenerated_
//      的失效邏輯,否則快取會與資料不一致。
//   3. getDetail() 回傳 const string&,呼叫端不可讓它活得比 MonsterEntry 久。
//   4. mutable 成員請維持在「拿掉只會變慢、不會變錯」的範圍;
//      本檔三個 mutable 成員都通過這個測試。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】mutable 綜合應用
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 同一個 const 函式裡,viewCount_++ 和延遲生成快取,兩者性質有何不同?
//     答：節奏不同。viewCount_ 每次呼叫都變,detailGenerated_ 只在第一次變。
//         但兩者的正當性理由相同:都不屬於邏輯狀態,凍結它們不會改變
//         getName()/getRarity() 等公開查詢的答案。
//         輸出中「炎龍王查看 3 次、詳細資料只生成 1 次」就是這個差異的直接證據。
//     追問：如果產品經理說要用 viewCount_ 做「熱門怪物排行榜」呢?
//         → 那 viewCount_ 就變成外部可觀察的邏輯狀態,不該再是 mutable,
//         相關的 getter 也要重新設計。成員該不該 mutable 由它的角色決定,不由型別決定。
//
// 🔥 Q2. void f(const MonsterEntry entries[], int count) 裡,
//        為什麼一定要額外傳 count?能不能用 sizeof 算出來?
//     答：不能。函式參數中的陣列會退化成指標,這個簽章其實等同於
//         const MonsterEntry*。sizeof(entries) 得到的是指標大小(本機 8),
//         不是陣列總大小,所以算不出元素個數。
//     追問：那有沒有辦法不用傳 count?→ 有三種:
//         (1) 改用 const std::vector<MonsterEntry>&;
//         (2) C++20 的 std::span<const MonsterEntry>;
//         (3) template<size_t N> void f(const MonsterEntry (&arr)[N])
//             —— 綁定真正的陣列參考,N 由編譯器推導,不會退化。
//
// ⚠️ 陷阱. 「這些查詢函式都是 const,所以可以放心讓多條執行緒同時呼叫。」
//     答：錯。printBrief() 會寫 viewCount_,getDetail() 會寫 detailCache_ 與
//         detailGenerated_,多執行緒同時進來就是 data race,屬於 UB。
//         更麻煩的是第一次 getDetail() 的競爭:一條執行緒可能讀到
//         另一條寫到一半的字串。要安全必須加 mutable std::mutex,
//         或改用 std::call_once。
//     為什麼會錯：把 const 讀成「唯讀」。const 保證的是「不改變邏輯狀態」,
//         不是「不寫入記憶體」。標準函式庫確實有「const 成員函式應為 thread-safe」
//         的慣例,但那是對實作者的要求,編譯器不會幫你檢查,
//         自己寫的類別沒加鎖就是沒有這個性質。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
using namespace std;

class MonsterEntry {
private:
    // ====== 邏輯狀態（不可變的圖鑑資料）======
    string name_;
    string element_;
    int baseHp_;
    int baseAttack_;
    int rarity_;          // 1~5 星

    // ====== 輔助狀態（mutable）======
    mutable int viewCount_;               // 被查看次數
    mutable bool detailGenerated_;        // 詳細資料是否已生成
    mutable string detailCache_;          // 詳細資料快取

    // 私有輔助：生成詳細資料
    void generateDetail() const {
        cout << "    [生成詳細資料...]" << endl;

        detailCache_ = "=== " + name_ + " ===\n";
        detailCache_ += "  屬性：" + element_ + "\n";
        detailCache_ += "  稀有度：";
        for (int i = 0; i < rarity_; i++) detailCache_ += "★";
        detailCache_ += "\n";
        detailCache_ += "  基礎 HP：" + to_string(baseHp_) + "\n";
        detailCache_ += "  基礎 ATK：" + to_string(baseAttack_) + "\n";

        // 添加弱點資訊
        detailCache_ += "  弱點：";
        if (element_ == "火") detailCache_ += "水";
        else if (element_ == "水") detailCache_ += "雷";
        else if (element_ == "雷") detailCache_ += "土";
        else if (element_ == "土") detailCache_ += "風";
        else if (element_ == "風") detailCache_ += "火";
        else detailCache_ += "無";
        detailCache_ += "\n";

        // 添加威脅評估
        int threat = baseHp_ / 100 + baseAttack_ / 10 + rarity_;
        detailCache_ += "  威脅指數：" + to_string(threat) + "\n";

        detailGenerated_ = true;
    }

public:
    MonsterEntry(const string& name, const string& elem,
                 int hp, int atk, int rare)
        : name_(name), element_(elem)
        , baseHp_(hp), baseAttack_(atk)
        , rarity_(rare > 0 && rare <= 5 ? rare : 1)
        , viewCount_(0)
        , detailGenerated_(false)
    {
    }

    // ====== 所有查詢函數都是 const ======

    // 簡要資訊——只讀，遞增查看次數
    void printBrief() const {
        viewCount_++;
        cout << "  ";
        for (int i = 0; i < rarity_; i++) cout << "★";
        cout << " " << name_ << " [" << element_ << "]"
             << " (查看:" << viewCount_ << ")" << endl;
    }

    // 詳細資訊——延遲生成 + 快取
    const string& getDetail() const {
        viewCount_++;
        if (!detailGenerated_) {
            generateDetail();   // 延遲初始化
        }
        return detailCache_;
    }

    // 其他 getter
    const string& getName() const { return name_; }
    int getViewCount() const { return viewCount_; }
    int getRarity() const { return rarity_; }
};

// 展示圖鑑——接收 const 引用
void showEncyclopedia(const MonsterEntry entries[], int count) {
    cout << "\n  ╔═══════════════════════════╗" << endl;
    cout << "  ║     怪 物 圖 鑑           ║" << endl;
    cout << "  ╚═══════════════════════════╝" << endl;

    for (int i = 0; i < count; i++) {
        entries[i].printBrief();   // const 函數 ✅
    }
}

// 查看特定怪物詳情——接收 const 引用
void showDetail(const MonsterEntry& entry) {
    cout << "\n" << entry.getDetail();  // 延遲生成 + 快取 ✅
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔從缺,理由如下
//   本檔的重點是「一個類別如何切分邏輯狀態與輔助狀態」,屬於 API 設計問題。
//   LeetCode 的設計題(146、155、707、1603 等)評的是操作複雜度是否達標,
//   一份把所有成員都標成 mutable 的解答照樣能 AC——這個主題在 LeetCode 上量不出來。
//   同課的檔案 2 已用 LeetCode 146. LRU Cache 說明「讀取會改變邏輯狀態」
//   的對照案例,此處不重複,改以一個真實的唯讀索引情境作結。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】唯讀 log 檔的行索引:延遲建索引 + 查詢熱度統計
//   情境：維運工具載入一份已經寫死的 log 檔內容,提供「跳到第 N 行」與
//         「搜尋關鍵字」兩種查詢。使用者常常只看前幾行就關掉,
//         所以整份切行的索引不該在載入時就建好。
//   對應本檔的兩種 mutable 用途：
//     * lineOffsets_ / indexed_ ：延遲初始化（第一次查詢才切行，之後重用）
//     * queryCount_             ：計數器（每次查詢都累加，用於工具自身的使用統計）
//   為什麼安全：raw_ 沒有 setter，載入後不可變，所以索引永遠有效，不需要失效邏輯。
//   為什麼查詢函式維持 const：對呼叫者而言同樣輸入永遠同樣結果，
//         而且維運工具內部大量以 const LogView& 傳遞這個物件。
// -----------------------------------------------------------------------------
class LogView {
private:
    string raw_;                               // 整份 log 內容，載入後不再變動

    mutable bool indexed_ = false;             // 延遲初始化旗標
    mutable vector<size_t> lineOffsets_;       // 每行起始位移
    mutable int queryCount_ = 0;               // 查詢熱度統計
    mutable int buildCount_ = 0;               // 證明索引只建一次

    void buildIndex() const {
        lineOffsets_.clear();
        lineOffsets_.push_back(0);
        for (size_t i = 0; i < raw_.size(); ++i) {
            if (raw_[i] == '\n' && i + 1 < raw_.size()) {
                lineOffsets_.push_back(i + 1);
            }
        }
        indexed_ = true;
        ++buildCount_;
    }

    // 內部取行：不累計 queryCount_，避免 findFirst 掃描時把統計灌爆
    // （這種「公開版計數、內部版不計數」的成對設計，在有統計需求的類別很常見）
    string lineNoCount(size_t n) const {
        if (n >= lineOffsets_.size()) return "";
        size_t begin = lineOffsets_[n];
        size_t end = (n + 1 < lineOffsets_.size()) ? lineOffsets_[n + 1] - 1 : raw_.size();
        return raw_.substr(begin, end - begin);
    }

public:
    explicit LogView(const string& raw) : raw_(raw) {}

    size_t lineCount() const {
        if (!indexed_) buildIndex();
        return lineOffsets_.size();
    }

    // 取第 n 行（0-based）；超出範圍回傳空字串
    string line(size_t n) const {
        ++queryCount_;
        if (!indexed_) buildIndex();
        return lineNoCount(n);
    }

    // 找出第一行含有 keyword 的行號；找不到回傳 lineCount()
    // 整趟搜尋只算「一次查詢」，所以內部用 lineNoCount()
    size_t findFirst(const string& keyword) const {
        ++queryCount_;
        if (!indexed_) buildIndex();
        for (size_t i = 0; i < lineOffsets_.size(); ++i) {
            if (lineNoCount(i).find(keyword) != string::npos) return i;
        }
        return lineOffsets_.size();
    }

    int queryCount() const { return queryCount_; }
    int buildCount() const { return buildCount_; }
};

int main() {
    cout << "============================================" << endl;
    cout << "   第 23 課：mutable 綜合範例" << endl;
    cout << "============================================" << endl;

    // 創建圖鑑條目
    const int COUNT = 4;
    MonsterEntry entries[COUNT] = {
        MonsterEntry("炎龍王", "火", 800, 70, 5),
        MonsterEntry("冰霜狼", "水", 400, 45, 3),
        MonsterEntry("雷電鷹", "雷", 300, 60, 4),
        MonsterEntry("泥土蟲", "土", 600, 20, 1)
    };

    // 瀏覽圖鑑——只觸發簡要資訊
    showEncyclopedia(entries, COUNT);

    // 查看炎龍王的詳細資料——第一次觸發生成
    cout << "\n--- 查看炎龍王詳情 ---";
    showDetail(entries[0]);

    // 再次查看——使用快取
    cout << "\n--- 再次查看炎龍王詳情 ---";
    showDetail(entries[0]);

    // 查看雷電鷹——觸發生成
    cout << "\n--- 查看雷電鷹詳情 ---";
    showDetail(entries[2]);

    // 查看各怪物被查看次數
    cout << "\n--- 查看統計 ---" << endl;
    for (int i = 0; i < COUNT; i++) {
        cout << "  " << entries[i].getName()
             << "：被查看 " << entries[i].getViewCount() << " 次" << endl;
    }

    // 注意：冰霜狼和泥土蟲的詳細描述從未被生成——節省資源
    cout << "\n  冰霜狼和泥土蟲的詳細描述從未生成，節省了資源！" << endl;

    cout << "\n=== 日常實務: 唯讀 log 的延遲索引 + 查詢統計 ===" << endl;
    const LogView log(
        "2026-07-19 09:00:01 INFO  service started\n"
        "2026-07-19 09:00:05 WARN  cache miss rate high\n"
        "2026-07-19 09:01:12 ERROR upstream timeout after 30s\n"
        "2026-07-19 09:01:13 INFO  retrying request\n");

    cout << "  建構完成（此時尚未切行）" << endl;
    cout << "  總行數 = " << log.lineCount() << endl;
    cout << "  第 0 行 = " << log.line(0) << endl;
    cout << "  第 2 行 = " << log.line(2) << endl;

    size_t hit = log.findFirst("ERROR");
    cout << "  第一筆 ERROR 在第 " << hit << " 行" << endl;
    cout << "  搜尋不存在的字串 -> 回傳 " << log.findFirst("FATAL")
         << "（等於總行數，代表找不到）" << endl;

    cout << "  查詢次數 = " << log.queryCount()
         << "（line x2 + findFirst x2；findFirst 內部掃描不重複計數）" << endl;
    cout << "  索引建立次數 = " << log.buildCount() << "（只建一次）" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第\ 23\ 課：mutable\ 關鍵字5.cpp -o mutable5

// 【輸出說明】
//   1. 「炎龍王：被查看 3 次」但「[生成詳細資料...]」只出現一次 ——
//      這正是計數器(每次都動)與延遲初始化(只做一次)節奏不同的直接證據。
//   2. 冰霜狼與泥土蟲只被 printBrief() 掃到，detailCache_ 從未生成，
//      所以輸出中沒有它們的「[生成詳細資料...]」。
//   3. 威脅指數為整數除法：炎龍王 800/100 + 70/10 + 5 = 8+7+5 = 20。
//   4. 實務段的「查詢次數 = 4」是 line() 兩次 + findFirst() 兩次；
//      findFirst 內部改呼叫不計數的 lineNoCount()，所以掃描 4 行不會灌成 8。
//   5. 本檔輸出完全決定性，連續執行 3 次 md5 相同。

// === 預期輸出 ===
// ============================================
//    第 23 課：mutable 綜合範例
// ============================================
//
//   ╔═══════════════════════════╗
//   ║     怪 物 圖 鑑           ║
//   ╚═══════════════════════════╝
//   ★★★★★ 炎龍王 [火] (查看:1)
//   ★★★ 冰霜狼 [水] (查看:1)
//   ★★★★ 雷電鷹 [雷] (查看:1)
//   ★ 泥土蟲 [土] (查看:1)
//
// --- 查看炎龍王詳情 ---
//     [生成詳細資料...]
// === 炎龍王 ===
//   屬性：火
//   稀有度：★★★★★
//   基礎 HP：800
//   基礎 ATK：70
//   弱點：水
//   威脅指數：20
//
// --- 再次查看炎龍王詳情 ---
// === 炎龍王 ===
//   屬性：火
//   稀有度：★★★★★
//   基礎 HP：800
//   基礎 ATK：70
//   弱點：水
//   威脅指數：20
//
// --- 查看雷電鷹詳情 ---
//     [生成詳細資料...]
// === 雷電鷹 ===
//   屬性：雷
//   稀有度：★★★★
//   基礎 HP：300
//   基礎 ATK：60
//   弱點：土
//   威脅指數：13
//
// --- 查看統計 ---
//   炎龍王：被查看 3 次
//   冰霜狼：被查看 1 次
//   雷電鷹：被查看 2 次
//   泥土蟲：被查看 1 次
//
//   冰霜狼和泥土蟲的詳細描述從未生成，節省了資源！
//
// === 日常實務: 唯讀 log 的延遲索引 + 查詢統計 ===
//   建構完成（此時尚未切行）
//   總行數 = 4
//   第 0 行 = 2026-07-19 09:00:01 INFO  service started
//   第 2 行 = 2026-07-19 09:01:12 ERROR upstream timeout after 30s
//   第一筆 ERROR 在第 2 行
//   搜尋不存在的字串 -> 回傳 4（等於總行數，代表找不到）
//   查詢次數 = 4（line x2 + findFirst x2；findFirst 內部掃描不重複計數）
//   索引建立次數 = 1（只建一次）
