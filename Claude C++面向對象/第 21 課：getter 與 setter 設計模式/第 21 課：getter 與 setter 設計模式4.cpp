// =============================================================================
//  第 21 課：getter 與 setter 設計模式 4  —  用「行為」取代 setter
// =============================================================================
//
// 【主題資訊 Information】
//   對照:
//     void setHp(int hp);                 // 貧血介面:把決定權推給呼叫端
//     void takeDamage(int damage);        // 行為介面:規則留在物件內
//   標準版本:C++98 起即有;本檔以 C++17 編譯。
//   複雜度:本檔所有成員函式皆 O(1)。
//   標頭檔:<string>(std::max 嚴格來說屬 <algorithm>,見下方注意事項)
//
// 【詳細解釋 Explanation】
//
// 【1. setter 的真正問題:它把「規則」留在呼叫端】
//   若提供 setHp(int),那麼「扣血後不得低於 0」「歸零時要判定死亡」這些規則,
//   就必須由每一個呼叫端自己記得做:
//       enemy.setHp(max(0, enemy.getHp() - dmg));
//       if (enemy.getHp() == 0) { /* 別忘了播死亡動畫 */ }
//   規則被複製到 N 個地方,只要有一處忘記,就是 bug。而且這個 bug 不會
//   出現在 Enemy 裡,你會在完全無關的檔案裡找它。
//   改成 takeDamage(dmg) 之後,規則只存在一份,而且就在資料旁邊。
//
// 【2. 「貧血模型」與「行為導向模型」】
//   只有 getter/setter、沒有行為的類別,通常被稱為貧血領域模型
//   (anemic domain model):它是個資料袋,所有邏輯散落在外部的
//   「Manager / Service / Helper」裡。這在 C++ 尤其可惜,因為
//   private + 成員函式本來就是為了把「資料 + 操作資料的規則」綁在一起。
//   判斷方法很簡單:如果每個 setter 的呼叫端都要先做一段計算或檢查,
//   那段計算就應該搬進類別,變成一個有名字的行為。
//
// 【3. 三種成員,三種對外策略(本檔完整示範)】
//   (a) name_/hp_/maxHp_ —— 外界要讀,但不該直接寫
//       → 只給 getter,不給 setter,寫入走 takeDamage()
//   (b) attackPower_     —— 外界不需要讀,也不該直接寫
//       → 兩個都不給,只透過 enrage() / attack() 間接影響
//   (c) internalId_/aiState_ —— 純內部實作細節
//       → 完全不對外;外界連它存在都不需要知道
//   關鍵心法:先問「外界需要知道什麼」,而不是「我有哪些成員」。
//
// 【4. 命名要用領域語言,不要用資料語言】
//   setHp() 是資料語言;takeDamage()、heal()、revive() 是領域語言。
//   領域語言的好處是「不合理的呼叫會自己顯得奇怪」——
//   看到 enemy.takeDamage(-50) 你會立刻覺得不對勁,
//   但 enemy.setHp(9999) 看起來只是「設一個值」而已。
//
// 【概念補充 Concept Deep Dive】
//   * 行為導向介面天然是「可稽核的」:想加上「每次受傷寫一筆戰鬥 log」,
//     只要改 takeDamage() 一處;若是 setHp(),你得找出所有呼叫端。
//   * 這也是為什麼 STL 容器沒有 setSize():size 是 push_back/erase 的
//     「結果」而不是「輸入」。允許直接設定 size 會讓容器立刻失去一致性。
//   * 本檔 internalId_ 以 rand() 初始化,但它從未被印出,所以程式輸出仍是
//     決定性的。順帶一提:C++ 標準並未規定 rand() 的數列,只保證未呼叫
//     srand() 時的行為等同 srand(1);跨實作的數值不可移植,不要拿它當
//     穩定 ID 或測試基準。
//
// 【注意事項 Pay Attention】
//   1. std::max 定義在 <algorithm>。本檔僅 include <string> 仍能編譯,
//      是因為 libstdc++ 的標頭之間有間接引入(transitive include)——
//      這是實作細節,不是標準保證,換編譯器或換版本就可能編不過。
//      嚴謹的寫法應明確 #include <algorithm>。
//   2. 「用行為取代 setter」不等於「禁止一切 setter」。純組態物件
//      (如視窗大小、逾時秒數)本來就沒有複雜不變式,setter 是合理的。
//      判準是:這個成員有沒有需要被一起維護的規則。
//   3. 行為函式仍要自己驗證輸入(本檔 takeDamage 對 damage <= 0 直接 return),
//      否則規則雖然集中了,卻依舊擋不住錯誤呼叫。
//   4. 不要為了「將來可能會用到」而預先開放 getter/setter;
//      介面一旦公開就很難收回,而未使用的介面仍是必須維護的承諾。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】用行為取代 setter
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼一般不建議對每個成員都機械式地產生 getter/setter?
//     答：因為那等於把 private 成員原封不動地公開,只是多繞一層函式呼叫,
//         封裝的實質保護是零。更嚴重的是,它把「維持不變式的責任」
//         推給了每一個呼叫端:規則被複製到 N 處,少寫一處就是 bug,
//         而且 bug 會出現在類別外面,很難追。
//     追問：那什麼時候 setter 是合理的?→ 當該成員沒有需要一起維護的規則時
//         (純組態值,如視窗寬度、逾時毫秒數),setter 完全合理。
//
// 🔥 Q2. 什麼是貧血領域模型(anemic domain model)?為什麼它是個反模式?
//     答：類別只有資料與 getter/setter,沒有任何行為,所有商業邏輯都寫在
//         外部的 Service/Manager 裡。反模式的原因是「資料與規則被拆散」:
//         同一條規則散落多處、無法在單一位置驗證,物件也無法保證自己
//         任何時刻都處於合法狀態。
//     追問：怎麼把它重構回來?→ 觀察每個 setter 的呼叫端,把呼叫前那段
//         計算與檢查搬進類別,給它一個領域上有意義的名字。
//
// ⚠️ 陷阱. 「我把 setHp() 改名成 takeDamage(),這樣就是行為導向了吧?」
//     答：不是。差別不在名字,在於「規則有沒有真的搬進來」。
//         若 takeDamage() 內部只寫 hp_ = newHp; 而下限判斷、死亡判定
//         仍留在呼叫端,那它只是換了名字的 setter,問題一個都沒解決。
//     為什麼會錯：把重構理解成改名。真正的判準是:
//         「呼叫端在呼叫這個函式的前後,還需不需要自己做額外的檢查?」
//         需要,就代表規則還沒搬完。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
using namespace std;

class Enemy {
private:
    string name_;
    int hp_;
    int maxHp_;
    int attackPower_;
    int internalId_;        // 內部 ID，外部完全不需要知道
    int aiState_;           // AI 狀態，純粹的內部邏輯

public:
    Enemy(const string& name, int maxHp, int atk)
        : name_(name), hp_(maxHp), maxHp_(maxHp)
        , attackPower_(atk)
        , internalId_(rand())  // 內部使用
        , aiState_(0)          // 內部使用
    {
    }

    // ===== 有 getter，沒有 setter =====
    // 外部需要讀取，但不能直接修改
    const string& getName() const { return name_; }
    int getHp() const { return hp_; }
    int getMaxHp() const { return maxHp_; }

    // ===== 沒有 getter，也沒有 setter =====
    // internalId_ — 外部完全不需要知道
    // aiState_    — 純粹的內部邏輯

    // ===== 用「行為」取代 setter =====
    // 不提供 setHp()，而是提供有意義的行為：
    void takeDamage(int damage) {
        if (damage <= 0) return;
        hp_ = max(0, hp_ - damage);
        cout << "  " << name_ << " 受到 " << damage
             << " 傷害 (HP:" << hp_ << "/" << maxHp_ << ")" << endl;

        if (hp_ == 0) {
            cout << "  " << name_ << " 被擊敗！" << endl;
        }
    }

    // 不提供 setAttackPower()，而是：
    void enrage() {  // 暴怒：攻擊力翻倍
        attackPower_ *= 2;
        cout << "  " << name_ << " 進入暴怒狀態！ATK="
             << attackPower_ << endl;
    }

    int attack() {
        aiState_++;   // 內部狀態更新，外部不知道
        cout << "  " << name_ << " 發動攻擊！(ATK:"
             << attackPower_ << ")" << endl;
        return attackPower_;
    }

    bool isAlive() const { return hp_ > 0; }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1603. Design Parking System
//   題目:停車場有大/中/小三種車位,建構時給定各自數量;addCar(carType)
//         在還有該類車位時停入並回傳 true,否則回傳 false。
//         carType: 1 = big, 2 = medium, 3 = small。
//   為什麼用到本主題:這題是「用行為取代 setter」的教科書範例。
//         題目要求的介面只有一個 addCar() —— 一個帶驗證的行為,
//         而不是 setBigSlots() 這種讓外界直接改剩餘車位的 setter。
//         若真的提供了 setter,呼叫端就能把剩餘車位設成負數或任意值,
//         「不得超停」這條規則便無處可掛。
//   複雜度:addCar 為 O(1);空間 O(1)。
// -----------------------------------------------------------------------------
class ParkingSystem {
private:
    int slots_[4];   // 索引 1..3 對應 big/medium/small；索引 0 不使用

public:
    ParkingSystem(int big, int medium, int small) : slots_{0, big, medium, small} {}

    // 唯一的修改入口：先驗證，再改狀態
    bool addCar(int carType) {
        if (carType < 1 || carType > 3) return false;   // 防呆：不合法車種
        if (slots_[carType] <= 0) return false;         // 規則：不得超停
        --slots_[carType];
        return true;
    }

    // 只給唯讀查詢，不給 setter
    int remaining(int carType) const {
        if (carType < 1 || carType > 3) return 0;
        return slots_[carType];
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】API 速率限制器(token bucket 的簡化版)
//   情境:每個 API key 每分鐘最多 N 次請求。這裡刻意不提供
//         setRemaining(),因為「剩餘額度」是 tryAcquire()/refill()
//         的結果,不是輸入。
//   若開放 setter,呼叫端就能自己把額度設回滿載,限流形同虛設——
//   這正是「setter 讓規則失去唯一守門人」在真實系統中的樣子。
// -----------------------------------------------------------------------------
class RateLimiter {
private:
    string  apiKey_;
    int     capacity_;    // 每個週期的額度上限
    int     remaining_;   // 本週期剩餘
    int     rejected_;    // 累計被拒次數（用於監控告警）

public:
    RateLimiter(const string& key, int capacity)
        : apiKey_(key), capacity_(capacity), remaining_(capacity), rejected_(0) {}

    // 行為：嘗試取得一個配額。規則(不得超用、要記錄被拒次數)全在這裡
    bool tryAcquire() {
        if (remaining_ <= 0) {
            ++rejected_;
            return false;
        }
        --remaining_;
        return true;
    }

    // 行為：週期重置。同樣不是 setter —— 呼叫端無法指定任意值
    void refill() { remaining_ = capacity_; }

    const string& apiKey()   const { return apiKey_; }
    int  remaining()         const { return remaining_; }
    int  rejectedCount()     const { return rejected_; }
    bool isThrottled()       const { return remaining_ == 0; }
};

int main() {
    cout << "=== 行為取代 setter ===" << endl;

    Enemy goblin("哥布林", 50, 15);
    cout << "  " << goblin.getName() << " HP:" << goblin.getHp()
         << "/" << goblin.getMaxHp() << endl;

    // 不是 goblin.setHp(30)，而是：
    goblin.takeDamage(20);

    // 不是 goblin.setAttackPower(30)，而是：
    goblin.enrage();

    // 正常攻擊
    int dmg = goblin.attack();
    cout << "  造成了 " << dmg << " 點傷害" << endl;

    // 以下都不可能做到（封裝保護）：
    // goblin.hp_ = 9999;          // 編譯錯誤
    // goblin.internalId_;         // 編譯錯誤
    // goblin.aiState_ = -1;       // 編譯錯誤

    // ─────────────────────────────────────────────────────────
    cout << "\n=== LeetCode 1603. Design Parking System ===" << endl;
    // 官方範例：ParkingSystem(1, 1, 0)
    ParkingSystem lot(1, 1, 0);
    cout << "  車位配置 big=1 medium=1 small=0" << endl;
    cout << "  addCar(1) big    → " << (lot.addCar(1) ? "true" : "false")
         << "   （預期 true）"  << endl;
    cout << "  addCar(2) medium → " << (lot.addCar(2) ? "true" : "false")
         << "   （預期 true）"  << endl;
    cout << "  addCar(3) small  → " << (lot.addCar(3) ? "true" : "false")
         << "  （預期 false，本來就沒有小車位）" << endl;
    cout << "  addCar(1) big    → " << (lot.addCar(1) ? "true" : "false")
         << "  （預期 false，大車位已滿）"       << endl;
    cout << "  剩餘 big/medium/small："
         << lot.remaining(1) << "/" << lot.remaining(2) << "/" << lot.remaining(3) << endl;

    // ─────────────────────────────────────────────────────────
    cout << "\n=== 日常實務：API 速率限制器 ===" << endl;
    RateLimiter limiter("key-9f3a", 3);
    cout << "  " << limiter.apiKey() << " 每週期額度：3" << endl;
    for (int i = 1; i <= 5; ++i) {
        bool ok = limiter.tryAcquire();
        cout << "  第 " << i << " 次請求："
             << (ok ? "放行" : "拒絕 (429)")
             << "  剩餘=" << limiter.remaining() << endl;
    }
    cout << "  已被限流？" << (limiter.isThrottled() ? "是" : "否") << endl;
    cout << "  累計被拒次數：" << limiter.rejectedCount() << endl;

    limiter.refill();
    cout << "  週期重置後剩餘：" << limiter.remaining() << endl;
    cout << "  被拒次數不會被重置（監控指標需累計）：" << limiter.rejectedCount() << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 21 課：getter 與 setter 設計模式4.cpp" -o l21_4
// 執行: ./l21_4        (rc=0)

// === 預期輸出 ===
// === 行為取代 setter ===
//   哥布林 HP:50/50
//   哥布林 受到 20 傷害 (HP:30/50)
//   哥布林 進入暴怒狀態！ATK=30
//   哥布林 發動攻擊！(ATK:30)
//   造成了 30 點傷害
//
// === LeetCode 1603. Design Parking System ===
//   車位配置 big=1 medium=1 small=0
//   addCar(1) big    → true   （預期 true）
//   addCar(2) medium → true   （預期 true）
//   addCar(3) small  → false  （預期 false，本來就沒有小車位）
//   addCar(1) big    → false  （預期 false，大車位已滿）
//   剩餘 big/medium/small：0/0/0
//
// === 日常實務：API 速率限制器 ===
//   key-9f3a 每週期額度：3
//   第 1 次請求：放行  剩餘=2
//   第 2 次請求：放行  剩餘=1
//   第 3 次請求：放行  剩餘=0
//   第 4 次請求：拒絕 (429)  剩餘=0
//   第 5 次請求：拒絕 (429)  剩餘=0
//   已被限流？是
//   累計被拒次數：2
//   週期重置後剩餘：3
//   被拒次數不會被重置（監控指標需累計）：2
