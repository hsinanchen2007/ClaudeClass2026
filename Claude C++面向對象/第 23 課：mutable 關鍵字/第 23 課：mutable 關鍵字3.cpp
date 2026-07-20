// =============================================================================
//  第 23 課：mutable 關鍵字 3  —  用 mutable 實作「延遲初始化 (lazy initialization)」
// =============================================================================
//
// 【主題資訊 Information】
//   語法    ： mutable bool ready_;  mutable std::string cache_;
//   標準版本： mutable 為 C++98;本檔提到的 std::optional 為 C++17,
//             std::once_flag / std::call_once 為 C++11
//   標頭檔  ： 無(語言關鍵字);optional 需 <optional>,call_once 需 <mutex>
//   核心概念： 把「昂貴的建構成本」從物件建立時,推遲到第一次真正被使用時
//
// 【詳細解釋 Explanation】
//
// 【1. 延遲初始化解決什麼問題】
//   本檔登記了 3 個任務,但使用者只點開其中 1 個看詳情。
//   如果在建構子就把描述字串全部生成,另外 2 份就是純浪費。
//   當「建立很多、實際使用很少」時,這個浪費會被放大——
//   例如載入一整份怪物圖鑑、一整份商品目錄、一整棵 UI 樹。
//   延遲初始化把成本從「物件個數」轉移到「實際使用個數」。
//
// 【2. 為什麼是 mutable,而不是把 getDescription() 拿掉 const】
//   getDescription() 從呼叫者的角度是純查詢:同一個 QuestLog,不論呼叫幾次、
//   什麼順序呼叫,拿到的字串永遠一樣。它符合 logical const。
//   若為了「內部要寫快取」就把 const 拿掉,代價是所有持有 const QuestLog&
//   的程式碼都不能再查詢描述了——const 會像傳染病一樣往外擴散,
//   最後大家索性都不寫 const。用 mutable 才能保住介面上的 const 承諾。
//
// 【3. 延遲初始化 vs 快取,差別在哪】
//   兩者程式碼長得幾乎一樣,但語意不同,而且差別就在「失效」:
//     * 快取(檔案 2 的 Circle):來源資料會變(setRadius),所以必須有失效邏輯。
//     * 延遲初始化(本檔):來源資料 questName_ / difficulty_ 建構後就不再改變,
//       所以描述算一次就永遠有效,不需要 invalidate()。
//   換句話說,延遲初始化是「來源不可變」這個前提下的快取特例,實作更單純也更安全。
//   本檔沒有提供任何 setter,正是這個設計的一部分——一旦加了 setName(),
//   就必須同時把 descriptionReady_ 設回 false,否則描述會與名稱不一致。
//
// 【4. 這個寫法的三個典型改良方向】
//   (a) 用 mutable std::optional<std::string>(C++17):
//       把「有沒有值」與「值是多少」合成一個成員,不會忘記同步兩個變數。
//   (b) 用 mutable std::once_flag + std::call_once(C++11):
//       多執行緒下保證只初始化一次,且其他執行緒會等到初始化完成才讀取。
//   (c) 若初始化成本極高且不一定用得到,考慮改回傳 by value 或用 shared_ptr,
//       避免整個大字串長期佔住記憶體。
//
// 【概念補充 Concept Deep Dive】
//   (A) 為什麼 generateDescription() 也必須是 const。
//       它是從 const 成員函式 getDescription() 裡呼叫的,而在 const 成員函式內,
//       this 是 const QuestLog*,只能呼叫 const 成員函式。
//       它內部之所以能寫 detailedDescription_,同樣是因為那些成員是 mutable。
//
//   (B) 回傳 const string& 的生命週期考量。
//       getDescription() 回傳的是成員的參考,不是複本,所以沒有複製成本;
//       但呼叫者必須確保 QuestLog 物件比那個參考活得久。
//       一旦寫成 const string& d = makeQuest().getDescription();
//       暫時物件在完整運算式結束時就被銷毀,d 立刻變成迷途參考(dangling reference)。
//       這種情況是 UB,不保證會崩潰,也可能默默印出看似正常的內容——
//       正因為不一定當場出錯,才特別難抓。
//
//   (C) 這個 lazy 版本 不是 thread-safe。
//       兩條執行緒同時第一次呼叫 getDescription(),會同時看到
//       descriptionReady_ == false,於是同時寫入 detailedDescription_,構成 data race。
//       更糟的是,其中一條可能讀到「另一條寫到一半」的字串。
//       修法見上面 (b) 的 std::call_once。
//
//   (D) 字串是一段段 += 累積起來的。
//       每次 += 都可能觸發重新配置。若在意這個成本,可先 reserve() 一個估計容量。
//       本例字串很短(遠小於 libstdc++ 的 SSO 門檻 15 bytes 之外的第一次配置),
//       成本可忽略,但在組裝大量文字時值得注意。
//
// 【注意事項 Pay Attention】
//   1. 只要來源資料有任何 setter,就必須同步把 ready 旗標設回 false;
//      本檔刻意不提供 setter 來迴避這個風險。
//   2. 延遲初始化把成本從建構期移到第一次使用時——那一次呼叫會變慢。
//      對延遲敏感的路徑(例如遊戲的每幀更新)反而可能要「預熱」,提前呼叫一次。
//   3. mutable 成員不參與 const 保護,所以不要把任何影響外部觀察結果的資料放進去。
//   4. 本檔在單執行緒下完全正確;多執行緒請改用 std::call_once。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】延遲初始化與 mutable
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 延遲初始化和快取,程式碼幾乎一樣,設計上有什麼差別?
//     答：差在「來源資料會不會變」。快取的來源會變,所以一定要有失效機制;
//         延遲初始化的來源在建構後就固定,算一次就永久有效,不需要 invalidate。
//         本檔不提供任何 setter,就是在用型別設計來保證這個前提。
//     追問：如果之後真的要加 setDifficulty(),要改什麼?
//         → 必須在 setter 裡把 descriptionReady_ 設回 false,
//         此時它就從「延遲初始化」升級成「快取」,兩者的維護成本差在這裡。
//
// 🔥 Q2. 為什麼 generateDescription() 這個私有函式也得宣告成 const?
//     答：它被 const 的 getDescription() 呼叫。在 const 成員函式內 this 是
//         const QuestLog*,只能呼叫 const 成員函式,否則編譯不過。
//         它能修改 detailedDescription_ 與 descriptionReady_,是因為那兩個成員是 mutable。
//     追問：可以改成 static 私有函式嗎?→ 可以,但要把需要的資料當參數傳進去、
//         把結果回傳出來,反而更清楚——static 版本不能碰 this,所以無法偷改狀態,
//         是更容易測試的設計。
//
// ⚠️ 陷阱. 「回傳 const string& 沒有複製成本,所以永遠比回傳 string 好。」
//     答：不一定。回傳參考的前提是被參考的物件活得夠久。
//         寫成 const string& d = QuestLog("x", 1).getDescription(); 時,
//         暫時的 QuestLog 在該完整運算式結束就被銷毀,d 成為迷途參考,屬於 UB。
//         (注意:const 參考延長生命週期的規則只適用於「直接綁定暫時物件」,
//          不會延長「暫時物件的成員」的壽命。)
//     為什麼會錯：多數人記住了「回傳參考避免複製」這條效能規則,
//         卻沒把生命週期一起記住。而且這種錯誤不一定當場崩潰——
//         記憶體可能還沒被覆寫,看起來一切正常,直到換個編譯器或最佳化等級才爆炸。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
using namespace std;

class QuestLog {
private:
    string questName_;
    int difficulty_;

    // 延遲初始化：詳細描述只在需要時才生成
    // mutable：即使在 const 函數中也能修改它們，因為它們只是效能優化，不影響邏輯狀態
    // descriptionReady_ 用來記錄描述是否已經生成，detailedDescription_ 存儲生成的描述
    // 這裡用 mutable 是因為即使在 const 函數中，我們也想更新描述生成的狀態，這是延遲初始化的典型用法
    mutable bool descriptionReady_;
    mutable string detailedDescription_;

    // 模擬昂貴的描述生成
    void generateDescription() const {
        cout << "    [生成詳細描述... 耗時操作]" << endl;
        detailedDescription_ = "【" + questName_ + "】\n";
        detailedDescription_ += "    難度：";
        for (int i = 0; i < difficulty_; i++) {
            detailedDescription_ += "★";
        }
        detailedDescription_ += "\n";
        detailedDescription_ += "    這是一個";
        if (difficulty_ >= 4) detailedDescription_ += "極其危險的";
        else if (difficulty_ >= 3) detailedDescription_ += "具有挑戰性的";
        else if (difficulty_ >= 2) detailedDescription_ += "需要謹慎的";
        else detailedDescription_ += "簡單的";
        detailedDescription_ += "任務。冒險者需做好充分準備。";

        descriptionReady_ = true;
    }

public:
    QuestLog(const string& name, int diff)
        : questName_(name)
        , difficulty_(diff > 0 && diff <= 5 ? diff : 1)
        , descriptionReady_(false)
    {
        cout << "  [登記任務] " << questName_ << endl;
        // 注意：不在建構時生成描述，延遲到需要時
    }

    // 簡單查詢——不需要詳細描述
    const string& getName() const { return questName_; }
    int getDifficulty() const { return difficulty_; }

    // 需要詳細描述時才觸發初始化
    const string& getDescription() const {
        if (!descriptionReady_) {
            generateDescription();   // 延遲初始化
        }
        return detailedDescription_;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔從缺,理由如下
//   延遲初始化是「物件生命週期與成本分攤」的設計技巧,不是演算法題型。
//   LeetCode 的設計題(146 LRU Cache、155 Min Stack、707 Design Linked List 等)
//   都在意的是資料結構的操作複雜度,沒有一題的評分標準會因為
//   「你有沒有延後計算」而不同。硬套一題只會模糊本檔的重點,
//   因此這裡改以一個真實的設定檔情境呈現(見下方實務範例)。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】設定檔的延遲解析(lazy parsing of a config value)
//   情境：服務啟動時讀進一份 key=value 設定,其中 allowed_origins 是一個
//         逗號分隔的白名單字串。多數請求根本用不到它,只有 CORS 檢查那條路徑需要。
//   為什麼延遲：切分字串 + 建 vector 的成本,只在真正第一次做 CORS 檢查時才付。
//         原始字串 raw_ 建構後不再變動,所以這是「延遲初始化」而非「快取」,
//         不需要失效邏輯——這正好呼應本檔上面 §3 的區分。
//   為什麼 const：isAllowed() 對呼叫者是純查詢,同樣輸入永遠同樣結果,
//         所以介面上必須維持 const,內部靠 mutable 放行。
// -----------------------------------------------------------------------------
class CorsConfig {
private:
    string raw_;                              // 例如 "https://a.com, https://b.org"

    mutable bool parsed_ = false;
    mutable vector<string> origins_;
    mutable int parseCount_ = 0;              // 證明真的只解析一次

    static string trim(const string& s) {
        size_t b = s.find_first_not_of(" \t");
        if (b == string::npos) return "";
        size_t e = s.find_last_not_of(" \t");
        return s.substr(b, e - b + 1);
    }

    void parse() const {
        origins_.clear();
        size_t pos = 0;
        while (pos <= raw_.size()) {
            size_t comma = raw_.find(',', pos);
            if (comma == string::npos) comma = raw_.size();
            string item = trim(raw_.substr(pos, comma - pos));
            if (!item.empty()) origins_.push_back(item);
            pos = comma + 1;
        }
        parsed_ = true;
        ++parseCount_;
    }

public:
    explicit CorsConfig(const string& raw) : raw_(raw) {}

    bool isAllowed(const string& origin) const {
        if (!parsed_) parse();                // 第一次呼叫才付解析成本
        for (const string& o : origins_) {
            if (o == origin) return true;
        }
        return false;
    }

    size_t originCount() const {
        if (!parsed_) parse();
        return origins_.size();
    }

    int parseCount() const { return parseCount_; }
};

int main() {
    cout << "=== 延遲初始化 ===" << endl;

    // 創建多個任務——都不生成描述
    cout << "\n--- 登記任務 ---" << endl;
    QuestLog quest1("討伐火龍", 5);
    QuestLog quest2("採集草藥", 1);
    QuestLog quest3("護送商隊", 3);

    // 只用簡單查詢——不觸發描述生成
    cout << "\n--- 簡單查詢（不觸發生成）---" << endl;
    cout << "  任務1：" << quest1.getName()
         << "（難度 " << quest1.getDifficulty() << "）" << endl;
    cout << "  任務2：" << quest2.getName()
         << "（難度 " << quest2.getDifficulty() << "）" << endl;

    // 只有查看詳細描述時才觸發生成
    cout << "\n--- 查看詳細描述（觸發生成）---" << endl;
    cout << quest1.getDescription() << endl;

    // 第二次查看——直接返回，不重複生成
    cout << "\n--- 再次查看（已生成）---" << endl;
    cout << quest1.getDescription() << endl;

    // quest2 和 quest3 的描述始終沒被生成——節省了資源
    cout << "\n--- quest2 和 quest3 從未生成描述，節省了資源 ---" << endl;

    // const 物件也能查詢：介面上的 const 承諾靠 mutable 保住了
    cout << "\n--- const 物件仍可查詢詳細描述 ---" << endl;
    const QuestLog frozen("清剿哥布林", 2);
    cout << frozen.getDescription() << endl;

    cout << "\n=== 日常實務: CORS 白名單的延遲解析 ===" << endl;
    const CorsConfig cors("https://app.example.com, https://admin.example.com ,, https://cdn.example.com");
    cout << "  建構完成（此時尚未解析）" << endl;
    cout << "  是否允許 https://app.example.com ? "
         << (cors.isAllowed("https://app.example.com") ? "是" : "否") << endl;
    cout << "  是否允許 https://evil.example.net ? "
         << (cors.isAllowed("https://evil.example.net") ? "是" : "否") << endl;
    cout << "  白名單筆數 = " << cors.originCount() << "（空項目已被略過）" << endl;
    cout << "  實際解析次數 = " << cors.parseCount() << "（多次查詢只解析一次）" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第\ 23\ 課：mutable\ 關鍵字3.cpp -o mutable3

// 【輸出說明】
//   1. 「[登記任務] 清剿哥布林」出現在 const 物件那一段,是建構子印的——
//      const 物件一樣要跑建構子,const 是建構完成之後才生效的性質。
//   2. CORS 白名單刻意放了一個空項目(",,"),輸出的 3 筆證明空項目被略過。
//   3. 本檔輸出完全決定性,連續執行 3 次 md5 相同。

// === 預期輸出 ===
// === 延遲初始化 ===
//
// --- 登記任務 ---
//   [登記任務] 討伐火龍
//   [登記任務] 採集草藥
//   [登記任務] 護送商隊
//
// --- 簡單查詢（不觸發生成）---
//   任務1：討伐火龍（難度 5）
//   任務2：採集草藥（難度 1）
//
// --- 查看詳細描述（觸發生成）---
//     [生成詳細描述... 耗時操作]
// 【討伐火龍】
//     難度：★★★★★
//     這是一個極其危險的任務。冒險者需做好充分準備。
//
// --- 再次查看（已生成）---
// 【討伐火龍】
//     難度：★★★★★
//     這是一個極其危險的任務。冒險者需做好充分準備。
//
// --- quest2 和 quest3 從未生成描述，節省了資源 ---
//
// --- const 物件仍可查詢詳細描述 ---
//   [登記任務] 清剿哥布林
//     [生成詳細描述... 耗時操作]
// 【清剿哥布林】
//     難度：★★
//     這是一個需要謹慎的任務。冒險者需做好充分準備。
//
// === 日常實務: CORS 白名單的延遲解析 ===
//   建構完成（此時尚未解析）
//   是否允許 https://app.example.com ? 是
//   是否允許 https://evil.example.net ? 否
//   白名單筆數 = 3（空項目已被略過）
//   實際解析次數 = 1（多次查詢只解析一次）
