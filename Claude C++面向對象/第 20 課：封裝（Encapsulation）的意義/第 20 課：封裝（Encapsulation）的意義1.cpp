// =============================================================================
//  第 20 課：封裝的意義 1  —  沒有封裝時，「不變式」為什麼一定會被破壞
// =============================================================================
//
// 【主題資訊 Information】
//   語法       ：struct 的成員預設為 public；class 的成員預設為 private
//   標準版本   ：C++98 起；本檔的 std::clamp 對照寫法需要 C++17
//   標頭檔     ：無（語言層級的存取控制）
//   核心概念   ：class invariant（類別不變式）
//   執行期成本 ：零。存取控制完全在編譯期檢查，不產生任何執行期指令
//
// 【詳細解釋 Explanation】
//   （本檔是「反面教材」，示範沒有封裝會發生什麼；
//     正面的做法請見同一課第 2 檔，兩檔刻意互補、不重複內容。）
//
// 【1. 什麼是「不變式（invariant）」——封裝真正要保護的東西】
//   不變式是「這個物件在任何外部可觀察的時刻，都必須成立的條件」。
//   以本檔的角色為例，合理的不變式至少有：
//       0 <= hp <= maxHp        // 血量不能超過上限，也不能是負的
//       maxHp > 0               // 最大血量必須是正數
//       attack > 0              // 攻擊力必須是正數
//       0 <= exp < 升到下一級所需經驗
//       level >= 1
//   注意這些條件「跨越多個成員」——hp 與 maxHp 的關係、level 與 exp 的關係。
//   這正是關鍵：單一成員自己看不出對錯，hp = 99999 本身是個合法的 int，
//   只有把它跟 maxHp 放在一起看才知道是錯的。
//   而 struct 的 public 成員讓每個欄位都能被「單獨」修改，
//   於是任何跨欄位的條件都無法被保證。
//
// 【2. 為什麼「全部 public」不是「比較有彈性」而是「無法維護」】
//   初學時很容易覺得 public 只是少打幾個字、比較方便。真正的代價在於：
//     * 責任被推給每一個呼叫端。原本「保證 hp 不超過 maxHp」是一個地方的事，
//       現在變成專案裡「每一處寫 hero.hp = ...」的地方都要記得檢查。
//       只要有一處忘記，整個系統的假設就破了。
//     * 無法定位錯誤來源。當你發現 hp 是 -500 時，兇手可能是三個月前
//       某個模組寫的一行程式碼。你必須搜尋整個專案裡所有寫入 hp 的地方。
//       若 hp 是 private，兇手一定在 class 內部那幾百行裡。
//       這是封裝最實際的價值：把「可能出錯的範圍」從整個專案縮小到一個檔案。
//     * 無法演進。哪天要把 hp 從 int 改成 long long，或改成用百分比儲存，
//       所有直接碰 hero.hp 的程式碼全部要改。
//
// 【3. struct 與 class 在 C++ 裡的唯一差別】
//   很多人以為 struct 是 C 的東西、class 才是 C++ 的。實際上在 C++ 裡，
//   兩者「只有兩個預設值」不同：
//       struct：成員預設 public，繼承時預設 public
//       class ：成員預設 private，繼承時預設 private
//   除此之外完全相同——struct 可以有建構子、解構子、成員函式、
//   虛擬函式、繼承、樣板，全部都可以。
//   所以選哪一個是「意圖宣告」而非能力差異，慣例是：
//       * 純粹的資料聚合、沒有不變式要維護 → 用 struct
//         （例如 std::pair、一個純粹的 3D 座標點）
//       * 有不變式要維護、有行為 → 用 class
//   本檔的 CharacterBad 用 struct 是「誠實」的：它確實沒有維護任何不變式。
//   問題不在於用了 struct，而在於「這種資料本來就需要不變式」。
//
// 【4. 封裝不等於「加上 getter/setter」】
//   這是最常見的誤解，重要到必須單獨講。把成員改成 private，
//   然後機械式地替每個成員加一組 getX()/setX()：
//       private: int hp_;
//       public:  int  getHp() const { return hp_; }
//                void setHp(int v) { hp_ = v; }      // ← 完全沒有檢查
//   這跟直接 public 在「保護能力」上是「完全等價」的——任何人還是可以
//   把 hp 設成 -500，只是多打了 setHp() 幾個字。
//   真正的封裝是「用有意義的操作取代直接設值」：
//       takeDamage(30)   // 而不是 setHp(getHp() - 30)
//       heal(40)         // 而不是 setHp(getHp() + 40)
//   因為只有 takeDamage 這種「動詞」才知道要套用什麼規則
//   （不能低於 0、死亡時要觸發事件）。setHp 這種「名詞 + 設值」
//   根本不知道呼叫端想幹嘛，也就無從檢查。
//
// 【概念補充 Concept Deep Dive】
//   * 存取控制是「編譯期」的，不是「執行期」的安全機制。
//     private 成員在記憶體裡就躺在那裡，用指標運算或 reinterpret_cast
//     一樣碰得到（那是未定義行為，但編譯器攔不住）。
//     封裝防的是「意外」與「維護成本」，不是防惡意攻擊。
//   * 存取控制不影響物件佈局，也不影響 sizeof。把 public 改成 private
//     不會讓物件變大或變小，也不產生任何額外指令——它是純粹的編譯期概念。
//     （注意：若成員的「宣告順序」被打散在不同存取區段中，
//       標準只保證同一存取區段內的成員依宣告順序遞增，
//       不同區段之間的相對順序是實作定義的。實務上主流編譯器仍照宣告順序排。）
//   * C 語言其實也能做封裝，做法是 opaque pointer（不完整型別）：
//     標頭檔只宣告 struct Character;（不給定義），使用者只能拿到指標，
//     碰不到欄位。代價是必須動態配置、無法直接放在 stack 上。
//     C++ 的 private 是把同樣的概念做進語言裡，且沒有這些代價。
//
// 【注意事項 Pay Attention】
//   1. struct 與 class 在 C++ 只差預設存取權限，其餘能力完全相同。
//   2. 加了 getter/setter 但沒有檢查，等於沒有封裝，只是多繞一層。
//   3. 存取控制是編譯期概念，不能當成執行期的安全防護。
//   4. 不變式常常跨越多個成員，這正是「單獨設值」無法保證它的原因。
//   5. 本檔的 CharacterBad 刻意不做任何保護，是反面教材，不要照抄。
//   6. 回傳 private 成員的「非 const 參考或指標」會讓封裝破功
//      （呼叫端可以直接改），等於把 private 又打開了。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】封裝與不變式
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++ 的 struct 和 class 有什麼差別？
//     答：只有兩個預設值不同——struct 的成員與繼承預設是 public，
//         class 預設是 private。除此之外能力完全相同：struct 一樣可以有
//         建構子、解構子、虛擬函式、繼承、樣板。
//         所以選用哪一個是在宣告意圖：純資料聚合用 struct，
//         有不變式要維護的用 class。
//     追問：那 C 的 struct 跟 C++ 的 struct 一樣嗎？→ 不一樣。C 的 struct
//         只能有資料成員，不能有成員函式、存取控制或繼承。
//         C++ 的 struct 是完整的 class，只是預設 public。
//
// 🔥 Q2. 什麼是 class invariant？為什麼 public 成員無法維護它？
//     答：不變式是「物件在任何外部可觀察時刻都必須成立的條件」，
//         例如 0 <= hp <= maxHp。關鍵在於它通常「跨越多個成員」——
//         單看 hp = 99999 是個合法的 int，只有跟 maxHp 一起看才知道錯。
//         public 成員允許每個欄位被單獨修改，物件無從得知有人動了它，
//         也就沒有任何時機可以檢查跨欄位的條件。
//     追問：那什麼時候「不需要」不變式？→ 純粹的資料傳輸物件（DTO）、
//         數學上的點/向量這類「任意組合都合法」的型別，
//         此時用 struct + public 反而更清楚，硬加 getter/setter 是雜訊。
//
// ⚠️ 陷阱. 把成員改成 private，再加上 getter/setter，這樣就算封裝了嗎？
//     答：不算。如果 setHp(int v) { hp_ = v; } 沒有任何檢查，
//         它的保護能力跟直接 public「完全等價」——別人照樣能設成 -500，
//         只是多打了幾個字。這種寫法只是把「欄位」換成「兩個函式」，
//         沒有換來任何保證。
//         真正的封裝是用「動詞」取代「設值」：提供 takeDamage(30) 而不是
//         setHp(getHp() - 30)，因為只有 takeDamage 知道該套用什麼規則。
//     為什麼會錯：很多教材把封裝教成「成員變數要 private，
//         然後每個都配一組 get/set」，變成機械式的樣板動作。
//         但封裝的目的是「維護不變式」，不是「讓成員變數不可見」。
//         判準很簡單：如果你的 setter 全都沒有檢查，
//         那你只是寫了一堆沒有用的程式碼。
//
// 註：本檔是「沒有封裝會出什麼事」的反面示範，適合對照而非解題。
//     LeetCode 的設計類題目（155 Min Stack、146 LRU Cache 等）
//     示範的是「正確封裝」，放在同一課的第 2、3 檔更貼切，此處不重複。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
using namespace std;

// ===== 沒有封裝的角色（全部 public）=====
// 這個結構體沒有任何封裝，所有成員都是 public 的，外部程式可以隨意修改它們，導致各種災難。
struct CharacterBad {
    string name;
    int hp;         // 生命值
    int maxHp;      // 最大生命值
    int attack;     // 攻擊力
    int level;      // 等級
    int exp;        // 經驗值
    int gold;       // 金幣
};

// -----------------------------------------------------------------------------
// 一個「檢查不變式」的外部函式——注意它只能事後稽核，無法事前阻止
//
// 這正是沒有封裝的結構性問題：物件本身無從得知有人動了它，
// 我們只能在事後拿一個外部檢查器去掃，而且還得記得呼叫它。
// -----------------------------------------------------------------------------
vector<string> checkInvariants(const CharacterBad& c) {
    vector<string> broken;
    if (c.maxHp <= 0)            broken.push_back("maxHp 必須大於 0");
    if (c.hp < 0)                broken.push_back("hp 不可為負數");
    if (c.hp > c.maxHp)          broken.push_back("hp 不可超過 maxHp");
    if (c.attack <= 0)           broken.push_back("attack 必須大於 0");
    if (c.level < 1)             broken.push_back("level 至少為 1");
    if (c.exp < 0)               broken.push_back("exp 不可為負數");
    if (c.gold < 0)              broken.push_back("gold 不可為負數");
    return broken;
}

void reportInvariants(const string& stage, const CharacterBad& c) {
    const vector<string> broken = checkInvariants(c);
    cout << "  [稽核] " << stage << "：";
    if (broken.empty()) {
        cout << "全部不變式成立" << endl;
    } else {
        cout << "違反 " << broken.size() << " 項 →";
        for (size_t i = 0; i < broken.size(); ++i) {
            cout << (i ? "、" : " ") << broken[i];
        }
        cout << endl;
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例】沒有封裝的連線設定：一個真實會上線的 bug
//
// 情境：一個 HTTP client 的設定物件。全部欄位 public，於是任何模組都能
//   在任何時刻改任何欄位——包含在連線「已經建立之後」才改 timeout，
//   或把 port 設成不合法的值。
// 為何用到本主題：這比遊戲角色更貼近日常。真正的痛點不是「有人惡意亂設」，
//   而是「兩個模組各自合理地修改了設定，合起來變成不合法狀態」，
//   而且沒有任何一行程式碼有機會發現。
// -----------------------------------------------------------------------------
struct HttpConfigBad {
    string host;
    int    port;
    int    timeoutMs;
    int    maxRetries;
    bool   useTls;
};

string describeConfig(const HttpConfigBad& c) {
    return (c.useTls ? string("https://") : string("http://"))
         + c.host + ":" + to_string(c.port)
         + " timeout=" + to_string(c.timeoutMs) + "ms"
         + " retries=" + to_string(c.maxRetries);
}

vector<string> checkConfig(const HttpConfigBad& c) {
    vector<string> bad;
    if (c.host.empty())                   bad.push_back("host 不可為空");
    if (c.port < 1 || c.port > 65535)     bad.push_back("port 必須在 1..65535");
    if (c.timeoutMs <= 0)                 bad.push_back("timeoutMs 必須為正");
    if (c.maxRetries < 0)                 bad.push_back("maxRetries 不可為負");
    // 跨欄位的不變式：TLS 的預設埠慣例
    if (c.useTls && c.port == 80)         bad.push_back("啟用 TLS 卻用 port 80（跨欄位矛盾）");
    return bad;
}

int main() {
    cout << "=== 沒有封裝的災難 ===" << endl;

    CharacterBad hero;
    hero.name = "勇者";
    hero.hp = 100;
    hero.maxHp = 100;
    hero.attack = 25;
    hero.level = 1;
    hero.exp = 0;
    hero.gold = 50;

    cout << hero.name << " HP:" << hero.hp << "/" << hero.maxHp << endl;
    reportInvariants("初始狀態", hero);

    // 災難 1：生命值可以超過最大值
    hero.hp = 99999;
    cout << "超量回血：HP = " << hero.hp << "/" << hero.maxHp << endl;

    // 災難 2：生命值可以是負數
    hero.hp = -500;
    cout << "負數 HP：HP = " << hero.hp << "/" << hero.maxHp << endl;

    // 災難 3：最大生命值可以被設成 0
    hero.maxHp = 0;
    cout << "最大HP歸零：HP = " << hero.hp << "/" << hero.maxHp << endl;

    // 災難 4：攻擊力可以隨便改
    hero.attack = 999999;
    cout << "作弊攻擊力：" << hero.attack << endl;

    // 災難 5：等級與經驗不一致
    hero.level = 99;
    hero.exp = 0;    // 99 級但經驗值是 0？
    cout << "等級 " << hero.level << "，經驗 " << hero.exp << endl;

    // 災難 6：金幣可以無限刷
    hero.gold = 2147483647;
    cout << "無限金幣：" << hero.gold << endl;

    cout << "\n所有數據都被任意篡改，毫無防護！" << endl;

    cout << "\n=== 事後稽核：破壞已經發生了 ===" << endl;
    reportInvariants("六個災難之後", hero);
    cout << "  → 注意：稽核只能「事後」發現，無法「事前」阻止。" << endl;
    cout << "    而且必須有人記得呼叫它——沒人呼叫就永遠不會被發現。" << endl;

    cout << "\n=== 為什麼難以除錯：兇手可能在任何地方 ===" << endl;
    {
        // 模擬三個不同模組各自「合理地」修改角色
        CharacterBad c;
        c.name = "法師"; c.hp = 80; c.maxHp = 80;
        c.attack = 15; c.level = 1; c.exp = 0; c.gold = 0;
        reportInvariants("模組執行前", c);

        // 模組 A：戰鬥系統扣血（看起來完全合理）
        c.hp -= 50;
        // 模組 B：陷阱系統再扣血（自己看也完全合理）
        c.hp -= 45;
        // 模組 C：讀取狀態準備顯示——此時才發現 hp 是負的
        reportInvariants("三個模組執行後", c);
        cout << "  目前 HP = " << c.hp << endl;
        cout << "  → 模組 A 與 B 各自都「沒有錯」，錯的是沒有人負責維護不變式。" << endl;
        cout << "    若 hp 是 private 且只能透過 takeDamage() 修改，" << endl;
        cout << "    夾在 0 的邏輯只需要寫一次，而且不可能被繞過。" << endl;
    }

    cout << "\n=== 日常實務：沒有封裝的連線設定 ===" << endl;
    {
        HttpConfigBad cfg;
        cfg.host = "api.example.com";
        cfg.port = 443;
        cfg.timeoutMs = 3000;
        cfg.maxRetries = 3;
        cfg.useTls = true;
        cout << "  初始設定：" << describeConfig(cfg) << endl;

        vector<string> bad = checkConfig(cfg);
        cout << "  檢查結果：" << (bad.empty() ? "合法" : "不合法") << endl;

        // 模組 A：「我們要改用測試環境」——只改了 host 跟 port，忘了 TLS
        cfg.host = "test.internal";
        cfg.port = 80;
        // 模組 B：「逾時太短了，加長一點」——但單位寫錯，以為是秒
        cfg.timeoutMs = -1;

        cout << "  兩個模組各自修改後：" << describeConfig(cfg) << endl;
        bad = checkConfig(cfg);
        cout << "  檢查結果：違反 " << bad.size() << " 項" << endl;
        for (const string& b : bad) cout << "    - " << b << endl;
        cout << "  → 兩處修改單獨看都很合理，合起來卻產生了不合法狀態。" << endl;
        cout << "    這正是「沒有任何一個地方負責維護整體一致性」的後果。" << endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 20 課：封裝（Encapsulation）的意義1.cpp" -o enc1
//
// 說明：
//   1. 本檔是「反面教材」：CharacterBad 與 HttpConfigBad 刻意不做任何保護，
//      用來示範沒有封裝的後果。正確的做法請看同一課第 2 檔。
//   2. 2147483647 是 32-bit int 的最大值（本機 x86-64 上 sizeof(int) == 4）。
//      再加 1 會是有號整數溢位，屬未定義行為，本檔刻意不示範。
//   3. 本檔所有輸出都是確定的：沒有用到亂數、時間或位址，
//      每次執行結果完全相同。

// === 預期輸出 ===
// === 沒有封裝的災難 ===
// 勇者 HP:100/100
//   [稽核] 初始狀態：全部不變式成立
// 超量回血：HP = 99999/100
// 負數 HP：HP = -500/100
// 最大HP歸零：HP = -500/0
// 作弊攻擊力：999999
// 等級 99，經驗 0
// 無限金幣：2147483647
//
// 所有數據都被任意篡改，毫無防護！
//
// === 事後稽核：破壞已經發生了 ===
//   [稽核] 六個災難之後：違反 2 項 → maxHp 必須大於 0、hp 不可為負數
//   → 注意：稽核只能「事後」發現，無法「事前」阻止。
//     而且必須有人記得呼叫它——沒人呼叫就永遠不會被發現。
//
// === 為什麼難以除錯：兇手可能在任何地方 ===
//   [稽核] 模組執行前：全部不變式成立
//   [稽核] 三個模組執行後：違反 1 項 → hp 不可為負數
//   目前 HP = -15
//   → 模組 A 與 B 各自都「沒有錯」，錯的是沒有人負責維護不變式。
//     若 hp 是 private 且只能透過 takeDamage() 修改，
//     夾在 0 的邏輯只需要寫一次，而且不可能被繞過。
//
// === 日常實務：沒有封裝的連線設定 ===
//   初始設定：https://api.example.com:443 timeout=3000ms retries=3
//   檢查結果：合法
//   兩個模組各自修改後：https://test.internal:80 timeout=-1ms retries=3
//   檢查結果：違反 2 項
//     - timeoutMs 必須為正
//     - 啟用 TLS 卻用 port 80（跨欄位矛盾）
//   → 兩處修改單獨看都很合理，合起來卻產生了不合法狀態。
//     這正是「沒有任何一個地方負責維護整體一致性」的後果。
