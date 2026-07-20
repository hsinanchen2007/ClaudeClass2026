// =============================================================================
//  第 15 課：帶參數的建構函數 12  —  綜合範例：explicit、預設參數、窄化防護
// =============================================================================
//
// 【主題資訊 Information】
//   涵蓋      : const& 傳參、預設參數、explicit、成員初始化列表、大括號防窄化
//   標準版本  : 主體 C++98／C++11；本檔用到的 unordered_map 與 list splice 為 C++11
//   標頭檔    : <iostream>、<string>、<list>、<unordered_map>
//   命名風格  : Google C++ Style（成員加底線後綴 name_）
//
// 【詳細解釋 Explanation】
//
// 【1. GameCharacter 為什麼這樣設計】
//       explicit GameCharacter(const string& name,
//                              const string& classType = "戰士",
//                              int level = 1)
//   這一行同時用上了本課四個重點：
//     * explicit —— 擋掉 GameCharacter g = "小明"; 這種隱式轉換。
//       角色不是字串，這個轉換沒有意義。
//     * const string& —— 避免值傳遞造成的多餘複製。
//     * 預設參數 —— 只有 name 是必需的，其餘給合理預設。
//     * 成員初始化列表 —— name_(name) 直接建構，而非先預設建構再賦值。
//
// 【2. 為什麼參數順序是 name → classType → level】
//   預設參數只能從右往左省略，所以參數順序等於「必要性的排序」。
//   name 沒有合理預設 → 放最左；classType 次之；level 幾乎都是 1 → 放最右。
//   若順序寫反（level 在前），呼叫端想指定職業就必須連 level 一起寫，
//   預設參數的便利性就消失了。這是設計 API 時實際要想的事。
//
// 【3. 本檔中「先初始化列表、再在本體算衍生屬性」的模式】
//   name_、classType_、level_ 用初始化列表設定；
//   hp_、mp_、attackPower_ 則在本體內依 classType_ 計算。
//   這是合理且常見的作法——衍生值需要先有輸入才能算。
//   但要注意兩件事：
//     (a) 本體內用到的 classType_ 必須**已經**初始化完成。
//         這一點成立，因為初始化列表在本體之前執行。
//     (b) 成員的初始化順序是**宣告順序**，不是初始化列表的書寫順序。
//         若你在初始化列表中用 level_ 去算 hp_，而 hp_ 宣告在 level_ 之前，
//         就會讀到尚未初始化的 level_。本檔把衍生計算放本體正好避開這個坑。
//
// 【4. Coordinate 示範的兩件事】
//     * explicit 對「多參數 constructor」也有意義（C++11 起）：
//       它擋掉的是 copy-list-initialization，也就是 Coordinate c = {50, 60};
//       但 Coordinate c{30, 40};（direct-list-init）仍然合法。
//     * 大括號禁止窄化轉換：Coordinate c{3.7, 4.2}; 編譯失敗，
//       而 Coordinate c(3.7, 4.2); 會靜靜地把 3.7 截斷成 3。
//       這是 {} 比 () 安全的最具體理由。
//
// 【概念補充 Concept Deep Dive】
//   ▍窄化轉換（narrowing conversion）的精確定義
//     指「目的型別無法表示來源型別所有值」的隱式轉換，包括
//     double→int、int→char、long→int、以及 int→float（可能損失精度）。
//     標準規定 {} 初始化中出現窄化即 ill-formed。
//     例外：來源是**編譯期常數且值可被精確表示**時允許——
//         char c{65};              // 合法（65 放得進 char）
//         int n = 300; char c{n};  // 標準上 ill-formed（n 不是常數運算式）
//
//     ⚠️ 但 GCC 的預設診斷等級並不一致，本機 GCC 15.2 實測：
//         Coordinate c{3.7, 4.2};  → **error**（預設就是硬錯誤）
//         int n = 300; char c{n};  → 預設只是 **warning** [-Wnarrowing]，
//                                    加上 -pedantic-errors 才變成 error
//     所以「{} 一定擋得住窄化」這句話對 GCC 預設設定並不完全成立。
//     要讓標準規則被確實執行，編譯時請加 -pedantic-errors。
//
//   ▍為什麼 explicit 對多參數 constructor 也有用
//     C++11 起 {} 讓多參數 constructor 也能參與「隱式轉換」：
//         void draw(Coordinate);
//         draw({1, 2});          // 沒有 explicit 時合法
//     加了 explicit 之後這行就失效，必須寫 draw(Coordinate{1, 2})。
//     本檔實測驗證了這個差異。
//
//   ▍陣列中使用帶參 constructor
//     GameCharacter party[3] = { GameCharacter(...), ... };
//     C++17 起保證 copy elision，這些 prvalue 直接就地初始化陣列元素，
//     不產生暫時物件，也不需要 copy constructor。
//
// 【注意事項 Pay Attention】
//   1. 成員的初始化順序是宣告順序，不是初始化列表的書寫順序（-Wreorder 會警告）。
//   2. explicit 擋掉 copy-list-init（= {…}）與隱式轉換，但不擋 direct-init（{…} 或 (…)）。
//   3. {} 禁止窄化、() 允許——這是選 {} 的最實際理由；但 GCC 對部分窄化
//      預設只給警告，要加 -pedantic-errors 才完全依標準辦事。
//   4. 預設參數的順序等於必要性排序；設計 API 時要刻意安排。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】explicit、預設參數與窄化
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. explicit 對多參數的 constructor 有意義嗎？
//     答：有（C++11 起）。因為 {} 讓多參數 constructor 也能當隱式轉換來源：
//         沒有 explicit 時 draw({1, 2}); 與 Coordinate c = {1, 2}; 都合法。
//         加上 explicit 之後這兩種寫法失效，但 Coordinate c{1, 2};
//         與 Coordinate c(1, 2);（direct-initialization）仍然可用。
//     追問：那 explicit 到底擋的是什麼？→ 擋的是 copy-initialization 這個情境
//         （= 右側、函式引數、return 值），不是 direct-initialization。
//
// 🔥 Q2. Coordinate c(3.7, 4.2); 和 Coordinate c{3.7, 4.2}; 差在哪？
//     答：前者合法，double 被截斷成 int（3 和 4），沒有任何警告；
//         後者是**編譯錯誤**，因為 {} 禁止窄化轉換。
//         這是大括號初始化比小括號安全的最具體理由——
//         它把一個靜默的資料損失變成編譯期錯誤。
//     追問：所有窄化在 {} 中都會被擋嗎？→ 標準上是，但 GCC 預設不一致：
//         double→int（如 Coordinate c{3.7,4.2}）預設就是 error，
//         而 int→char（int n=300; char c{n};）預設只是 -Wnarrowing 警告，
//         要加 -pedantic-errors 才會變成 error。本機 GCC 15.2 實測如此。
//
// ⚠️ 陷阱. 成員初始化列表寫成 : hp_(level_ * 20), level_(level)，
//         但 hp_ 宣告在 level_ 前面。
//     答：hp_ 會用到**尚未初始化**的 level_，得到不確定的結果。
//         因為成員的實際初始化順序永遠是**宣告順序**，不是你在初始化列表中
//         書寫的順序。GCC/Clang 會給 -Wreorder 警告，但那只在順序不一致時提醒，
//         不會告訴你「你讀到了未初始化的值」。
//     為什麼會錯：以為初始化列表是「由上往下依序執行的程式碼」。
//         它其實只是「為每個成員指定初始化方式」的表格，執行順序由宣告順序決定。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <list>
#include <unordered_map>
using namespace std;

// ============================================================
// 遊戲角色類別：展示帶參建構函數的各種技巧
// ============================================================
class GameCharacter {
private:
    string name_;         // Google 風格：底線後綴
    string classType_;    // 職業
    int level_;
    int hp_;
    int mp_;
    double attackPower_;

public:
    // 建構函數 1：完整建構（explicit + const 引用 + 預設參數 + 初始化列表）
    // 參數順序 = 必要性排序：name 必填 → classType 常改 → level 幾乎都是 1
    explicit GameCharacter(const string& name,
                           const string& classType = "戰士",
                           int level = 1)
        : name_(name), classType_(classType), level_(level)
    {
        // 衍生屬性在本體計算：此時初始化列表已執行完，classType_、level_ 都可用
        if (classType_ == "戰士") {
            hp_ = 150 + level_ * 20;
            mp_ = 30 + level_ * 5;
            attackPower_ = 15.0 + level_ * 3.0;
        } else if (classType_ == "法師") {
            hp_ = 80 + level_ * 10;
            mp_ = 100 + level_ * 15;
            attackPower_ = 25.0 + level_ * 5.0;
        } else if (classType_ == "弓箭手") {
            hp_ = 100 + level_ * 12;
            mp_ = 50 + level_ * 8;
            attackPower_ = 20.0 + level_ * 4.0;
        } else {
            // 未知職業，使用平均值
            hp_ = 100 + level_ * 15;
            mp_ = 50 + level_ * 10;
            attackPower_ = 15.0 + level_ * 3.5;
        }

        cout << "  [創建角色] " << name_ << " - " << classType_
             << " (Lv." << level_ << ")" << endl;
    }

    void printStatus() const {
        cout << "  ┌──────────────────────────┐" << endl;
        cout << "  │ " << name_ << " [" << classType_ << "]" << endl;
        cout << "  │ 等級: " << level_ << endl;
        cout << "  │ HP: " << hp_ << "  MP: " << mp_ << endl;
        cout << "  │ 攻擊力: " << attackPower_ << endl;
        cout << "  └──────────────────────────┘" << endl;
    }
};

// ============================================================
// 座標類別：展示 explicit 和窄化防護
// ============================================================
class Coordinate {
private:
    int x_, y_;

public:
    // explicit 對多參數 constructor 同樣有效（C++11 起）：
    // 它擋掉 Coordinate c = {1,2}; 與 draw({1,2});，但不擋 Coordinate c{1,2};
    explicit Coordinate(int x, int y) : x_(x), y_(y) { }

    void print() const {
        cout << "  (" << x_ << ", " << y_ << ")" << endl;
    }
};

// 用來示範 explicit 擋掉了什麼
static void drawAt(const Coordinate& c) {
    cout << "  drawAt 收到 → ";
    c.print();
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 146. LRU Cache
//   題目：設計一個容量固定的 LRU（Least Recently Used）快取，
//         get 與 put 都要 O(1)。容量滿時淘汰最久未使用的項目。
//   為什麼用到本主題：題目簽名 LRUCache(int capacity) 正是一個
//         **帶參數的建構函數**——容量必須在建構時決定，之後不可改變。
//         這是本課最貼切的設計題：capacity 沒有合理的預設值
//         （0 容量的快取毫無意義），所以它必須是必填參數，
//         而且應該用 explicit 避免 LRUCache c = 100; 這種可疑寫法。
//   關鍵想法：list 維持使用順序（最前 = 最近使用），
//         unordered_map 把 key 對應到 list 節點的 iterator，
//         使「把某個節點移到最前面」變成 O(1) 的 splice。
//   複雜度：get / put 皆為 O(1) 平均；空間 O(capacity)。
// -----------------------------------------------------------------------------
class LRUCache {
private:
    int capacity_;
    // list 存 (key, value)，最前面是最近使用的
    list<pair<int, int>> items_;
    // key → 在 items_ 中的位置，讓我們能 O(1) 找到並搬移節點
    unordered_map<int, list<pair<int, int>>::iterator> index_;

public:
    // 帶參建構：capacity 沒有合理預設值，所以是必填；
    // explicit 避免 LRUCache c = 100; 這種語意不明的寫法
    explicit LRUCache(int capacity) : capacity_(capacity) {
        // 容量至少為 1，否則快取沒有意義
        if (capacity_ < 1) capacity_ = 1;
    }

    int get(int key) {
        auto it = index_.find(key);
        if (it == index_.end()) return -1;
        // 命中：把該節點搬到最前面（splice 不會使 iterator 失效）
        items_.splice(items_.begin(), items_, it->second);
        return it->second->second;
    }

    void put(int key, int value) {
        auto it = index_.find(key);
        if (it != index_.end()) {
            it->second->second = value;                       // 更新值
            items_.splice(items_.begin(), items_, it->second); // 並移到最前
            return;
        }
        if (static_cast<int>(items_.size()) >= capacity_) {
            // 淘汰最久未使用的（list 最後一個）
            auto last = items_.back();
            index_.erase(last.first);
            items_.pop_back();
        }
        items_.emplace_front(key, value);
        index_[key] = items_.begin();
    }

    // 教學用：印出目前的使用順序（最近使用在前）
    void dump() const {
        cout << "    快取內容(最近使用在前): ";
        for (const auto& kv : items_) cout << "(" << kv.first << "," << kv.second << ") ";
        cout << endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】限流器（RateLimiter）：建構時決定的參數就該是必填
//   情境：API 閘道對每個用戶端限流。「每秒幾次」與「突發容量」
//         是這個物件存在的前提——沒有這兩個數字，限流器不知道要做什麼。
//   設計：兩個參數都必填、用 explicit、並在 constructor 中驗證，
//         讓「建構成功」等於「這個限流器一定是可用的」。
//   對照 LRUCache：兩者都是「參數在建構時決定、之後不可變」的典型，
//         這正是帶參 constructor 最核心的用途——建立不可變的組態。
// -----------------------------------------------------------------------------
class RateLimiter {
private:
    int    ratePerSec_;
    int    burst_;
    double tokens_;

public:
    explicit RateLimiter(int ratePerSec, int burst)
        : ratePerSec_(ratePerSec < 1 ? 1 : ratePerSec),
          burst_(burst < 1 ? 1 : burst),
          tokens_(burst < 1 ? 1 : burst)     // 一開始給滿，允許突發
    {}

    // 模擬經過 elapsedSec 秒後嘗試取得一個 token
    bool allow(double elapsedSec) {
        tokens_ += elapsedSec * ratePerSec_;
        if (tokens_ > burst_) tokens_ = burst_;       // 上限是 burst
        if (tokens_ >= 1.0) { tokens_ -= 1.0; return true; }
        return false;
    }

    void print() const {
        cout << "    限流器: " << ratePerSec_ << " req/s, burst=" << burst_
             << ", 目前 tokens=" << tokens_ << endl;
    }
};

int main() {
    cout << "==========================================" << endl;
    cout << "   第 15 課：帶參數的建構函數 綜合範例" << endl;
    cout << "==========================================" << endl;

    // --- 使用不同數量的參數 ---
    cout << "\n[1] 不同參數數量（預設參數從右往左省略）" << endl;
    GameCharacter hero1("勇者小明");                    // 預設職業和等級
    GameCharacter hero2("暗影法師", "法師");            // 預設等級
    GameCharacter hero3("神射手", "弓箭手", 10);        // 全部指定

    cout << "\n[2] 角色狀態" << endl;
    hero1.printStatus();
    hero2.printStatus();
    hero3.printStatus();

    // --- explicit 的效果 ---
    cout << "\n[3] Coordinate (explicit)" << endl;
    Coordinate c1(10, 20);         // OK：direct-initialization
    Coordinate c2{30, 40};         // OK：direct-list-initialization
    // Coordinate c3 = {50, 60};   // 錯誤！explicit 禁止 copy-list-initialization
    // drawAt({1, 2});             // 錯誤！同樣是 copy-list-initialization
    c1.print();
    c2.print();
    cout << "  Coordinate c3 = {50, 60};  → 編譯失敗（explicit 擋下）" << endl;
    cout << "  drawAt({1, 2});            → 編譯失敗（同上）" << endl;
    drawAt(Coordinate{1, 2});      // 明確寫出型別就可以
    cout << "  drawAt(Coordinate{1, 2});  → OK，意圖清楚" << endl;

    // --- 大括號防窄化 ---
    cout << "\n[4] 窄化轉換測試" << endl;
    Coordinate c4(3, 4);      // OK
    // Coordinate c5{3.7, 4.2};  // 編譯錯誤！double → int 是窄化
    Coordinate c6{int(3.7), int(4.2)};  // OK：明確轉換
    c4.print();
    c6.print();
    cout << "  Coordinate c5{3.7, 4.2};   → 編譯失敗（{} 禁止窄化）" << endl;
    cout << "  Coordinate c5(3.7, 4.2);   → 卻能編譯，靜靜截斷成 (3, 4)" << endl;
    cout << "  ↑ 這就是 {} 比 () 安全的最具體理由" << endl;
    cout << "  註：GCC 對窄化的預設診斷不一致——double→int 預設是 error，" << endl;
    cout << "      int→char（非常數）預設只是 -Wnarrowing 警告；" << endl;
    cout << "      加上 -pedantic-errors 才完全依標準辦事（本機 GCC 15.2 實測）。" << endl;

    // --- 陣列中使用帶參建構函數 ---
    cout << "\n[5] 陣列中的帶參建構" << endl;
    GameCharacter party[3] = {
        GameCharacter("坦克", "戰士", 5),
        GameCharacter("治癒者", "法師", 3),
        GameCharacter("輸出手", "弓箭手", 7)
    };

    cout << "\n隊伍成員：" << endl;
    for (int i = 0; i < 3; i++) {
        party[i].printStatus();
    }

    // --- LeetCode 146. LRU Cache ---
    cout << "\n[6] LeetCode 146. LRU Cache（capacity 由建構函數決定）" << endl;
    LRUCache cache(2);             // 容量必填、explicit → 意圖明確
    cache.put(1, 1);
    cache.put(2, 2);
    cache.dump();
    cout << "    get(1) = " << cache.get(1) << "   (期望 1)" << endl;
    cache.dump();
    cache.put(3, 3);               // 容量滿 → 淘汰最久未使用的 key 2
    cout << "    put(3,3) 後（應淘汰 key 2）:" << endl;
    cache.dump();
    cout << "    get(2) = " << cache.get(2) << "  (期望 -1，已被淘汰)" << endl;
    cache.put(4, 4);               // 再淘汰 key 1
    cout << "    put(4,4) 後（應淘汰 key 1）:" << endl;
    cache.dump();
    cout << "    get(1) = " << cache.get(1) << "  (期望 -1)" << endl;
    cout << "    get(3) = " << cache.get(3) << "   (期望 3)" << endl;
    cout << "    get(4) = " << cache.get(4) << "   (期望 4)" << endl;

    // --- 日常實務：限流器 ---
    cout << "\n[7] 日常實務：限流器（參數在建構時決定，之後不可變）" << endl;
    RateLimiter limiter(2, 3);     // 每秒 2 次，突發容量 3
    limiter.print();
    cout << "    連續請求（不等待）:" << endl;
    for (int i = 1; i <= 5; ++i) {
        bool ok = limiter.allow(0.0);
        cout << "      第 " << i << " 次: " << (ok ? "通過" : "被限流") << endl;
    }
    cout << "    等待 1 秒後（補充 2 個 token）:" << endl;
    for (int i = 1; i <= 3; ++i) {
        bool ok = limiter.allow(i == 1 ? 1.0 : 0.0);
        cout << "      第 " << i << " 次: " << (ok ? "通過" : "被限流") << endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 15 課：帶參數的建構函數12.cpp" -o param12
//   （建議另加 -pedantic-errors，讓標準規定的窄化規則被確實執行）


// === 預期輸出 ===
// ==========================================
//    第 15 課：帶參數的建構函數 綜合範例
// ==========================================
//
// [1] 不同參數數量（預設參數從右往左省略）
//   [創建角色] 勇者小明 - 戰士 (Lv.1)
//   [創建角色] 暗影法師 - 法師 (Lv.1)
//   [創建角色] 神射手 - 弓箭手 (Lv.10)
//
// [2] 角色狀態
//   ┌──────────────────────────┐
//   │ 勇者小明 [戰士]
//   │ 等級: 1
//   │ HP: 170  MP: 35
//   │ 攻擊力: 18
//   └──────────────────────────┘
//   ┌──────────────────────────┐
//   │ 暗影法師 [法師]
//   │ 等級: 1
//   │ HP: 90  MP: 115
//   │ 攻擊力: 30
//   └──────────────────────────┘
//   ┌──────────────────────────┐
//   │ 神射手 [弓箭手]
//   │ 等級: 10
//   │ HP: 220  MP: 130
//   │ 攻擊力: 60
//   └──────────────────────────┘
//
// [3] Coordinate (explicit)
//   (10, 20)
//   (30, 40)
//   Coordinate c3 = {50, 60};  → 編譯失敗（explicit 擋下）
//   drawAt({1, 2});            → 編譯失敗（同上）
//   drawAt 收到 →   (1, 2)
//   drawAt(Coordinate{1, 2});  → OK，意圖清楚
//
// [4] 窄化轉換測試
//   (3, 4)
//   (3, 4)
//   Coordinate c5{3.7, 4.2};   → 編譯失敗（{} 禁止窄化）
//   Coordinate c5(3.7, 4.2);   → 卻能編譯，靜靜截斷成 (3, 4)
//   ↑ 這就是 {} 比 () 安全的最具體理由
//   註：GCC 對窄化的預設診斷不一致——double→int 預設是 error，
//       int→char（非常數）預設只是 -Wnarrowing 警告；
//       加上 -pedantic-errors 才完全依標準辦事（本機 GCC 15.2 實測）。
//
// [5] 陣列中的帶參建構
//   [創建角色] 坦克 - 戰士 (Lv.5)
//   [創建角色] 治癒者 - 法師 (Lv.3)
//   [創建角色] 輸出手 - 弓箭手 (Lv.7)
//
// 隊伍成員：
//   ┌──────────────────────────┐
//   │ 坦克 [戰士]
//   │ 等級: 5
//   │ HP: 250  MP: 55
//   │ 攻擊力: 30
//   └──────────────────────────┘
//   ┌──────────────────────────┐
//   │ 治癒者 [法師]
//   │ 等級: 3
//   │ HP: 110  MP: 145
//   │ 攻擊力: 40
//   └──────────────────────────┘
//   ┌──────────────────────────┐
//   │ 輸出手 [弓箭手]
//   │ 等級: 7
//   │ HP: 184  MP: 106
//   │ 攻擊力: 48
//   └──────────────────────────┘
//
// [6] LeetCode 146. LRU Cache（capacity 由建構函數決定）
//     快取內容(最近使用在前): (2,2) (1,1) 
//     get(1) = 1   (期望 1)
//     快取內容(最近使用在前): (1,1) (2,2) 
//     put(3,3) 後（應淘汰 key 2）:
//     快取內容(最近使用在前): (3,3) (1,1) 
//     get(2) = -1  (期望 -1，已被淘汰)
//     put(4,4) 後（應淘汰 key 1）:
//     快取內容(最近使用在前): (4,4) (3,3) 
//     get(1) = -1  (期望 -1)
//     get(3) = 3   (期望 3)
//     get(4) = 4   (期望 4)
//
// [7] 日常實務：限流器（參數在建構時決定，之後不可變）
//     限流器: 2 req/s, burst=3, 目前 tokens=3
//     連續請求（不等待）:
//       第 1 次: 通過
//       第 2 次: 通過
//       第 3 次: 通過
//       第 4 次: 被限流
//       第 5 次: 被限流
//     等待 1 秒後（補充 2 個 token）:
//       第 1 次: 通過
//       第 2 次: 通過
//       第 3 次: 被限流
