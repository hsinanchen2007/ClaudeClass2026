/*
 * ============================================================================
 *  第 25 課：類別內的靜態成員函數 —— 完整總結
 * ============================================================================
 *
 *  本檔案涵蓋第 25 課所有 8 個 .cpp 檔案的核心概念，閱讀此檔即可完整複習。
 *
 *  目錄：
 *    第一節：靜態成員函數的基礎語法與特性
 *    第二節：靜態函數 vs 非靜態函數的訪問規則
 *    第三節：靜態函數不能加 const 的原因
 *    第四節：靜態函數透過參數間接訪問非靜態成員
 *    第五節：應用一 —— 工廠函數（Factory Function）
 *    第六節：應用二 —— 工具函數類（Utility Class）
 *    第七節：應用三 —— 單例模式存取點（Singleton）
 *    第八節：靜態函數與非靜態函數的互相調用
 *    第九節：靜態函數 vs 全域函數 vs 命名空間函數
 *    第十節：綜合範例 —— 遊戲成就系統
 *    附錄：完整訪問規則總表與重點回顧
 *
 * ============================================================================
 */

#include <iostream>
#include <string>
#include <cmath>
using namespace std;

/* ============================================================================
 *  第一節：靜態成員函數的基礎語法與特性
 * ============================================================================
 *
 *  語法：
 *    static 返回類型 函數名(參數);
 *
 *  五大特性：
 *    1. 不依賴任何對象，可以直接用 類別名::函數名() 調用
 *    2. 沒有 this 指標
 *    3. 只能訪問靜態成員（變數和函數）
 *    4. 不能訪問非靜態成員（因為不知道是哪個對象的）
 *    5. 不能加 const 修飾（因為沒有 this，const 無意義）
 *
 *  為什麼需要靜態成員函數？
 *    例如一個函數只使用靜態數據（如 totalCount_），卻被宣告為非靜態，
 *    那麼調用時就必須先有一個對象，這不合理。
 *    靜態成員函數解決了這個問題：不需要對象即可調用。
 *
 *  調用方式：
 *    推薦：類別名::函數名()    例如 Bullet::printStats()
 *    不推薦：對象.函數名()      例如 obj.printStats()（雖然合法但語義不清）
 */

// --- 範例：子彈統計系統 ---
// 展示靜態成員函數的基本用法：訪問靜態變數、不能訪問非靜態成員
class Bullet {
private:
    int damage_;                            // 非靜態：每顆子彈的傷害值
    string type_;                           // 非靜態：每顆子彈的類型

    inline static int totalFired_ = 0;     // 靜態：所有子彈共享的發射計數
    inline static int totalHit_ = 0;       // 靜態：所有子彈共享的命中計數

public:
    Bullet(const string& type, int dmg)
        : type_(type), damage_(dmg)
    {
    }

    // 非靜態函數：有 this 指標，可以訪問一切（靜態 + 非靜態）
    void fire() {
        totalFired_++;                      // 訪問靜態變數 -> OK
        cout << "  發射 " << type_ << "（傷害:" << damage_ << "）" << endl;
                                            // 訪問非靜態成員 -> OK（有 this）
    }

    void hit() {
        totalHit_++;                        // 訪問靜態變數 -> OK
        cout << "  命中！" << type_ << " 造成 " << damage_ << " 傷害" << endl;
    }

    // ====== 靜態成員函數：沒有 this 指標 ======
    // 只能訪問靜態成員（totalFired_ 和 totalHit_）
    // 不能訪問非靜態成員（damage_ 和 type_）
    // 不能調用非靜態函數（fire() 和 hit()）
    static int getTotalFired() { return totalFired_; }
    static int getTotalHit() { return totalHit_; }

    static double getHitRate() {
        if (totalFired_ == 0) return 0.0;
        return static_cast<double>(totalHit_) / totalFired_ * 100.0;
    }

    static void printStats() {
        cout << "  發射：" << totalFired_
             << "  命中：" << totalHit_
             << "  命中率：" << getHitRate() << "%" << endl;

        // 以下會編譯錯誤！靜態函數沒有 this：
        // cout << damage_;    // 錯誤：不能訪問非靜態成員
        // cout << type_;      // 錯誤：不能訪問非靜態成員
        // fire();             // 錯誤：不能調用非靜態函數
    }

    static void resetStats() {
        totalFired_ = 0;
        totalHit_ = 0;
        cout << "  統計數據已重置" << endl;
    }
};

/* ============================================================================
 *  第二節：靜態函數 vs 非靜態函數的訪問規則
 * ============================================================================
 *
 *  訪問規則總表：
 *  ┌─────────────────┬──────────────┬─────────────────┐
 *  │ 調用者           │ 可訪問靜態？  │ 可訪問非靜態？   │
 *  ├─────────────────┼──────────────┼─────────────────┤
 *  │ 非靜態成員函數    │  可以        │  可以（有this）  │
 *  │ 靜態成員函數      │  可以        │  不行（無this）  │
 *  │ 外部函數         │  如果public  │  需要對象        │
 *  └─────────────────┴──────────────┴─────────────────┘
 *
 *  關鍵理解：
 *  - 非靜態函數有 this 指標，所以能訪問所有成員
 *  - 靜態函數沒有 this 指標，所以只能訪問靜態成員
 *  - 靜態函數不知道「是哪個對象」，所以無法訪問屬於特定對象的成員
 */

/* ============================================================================
 *  第三節：靜態函數不能加 const 的原因
 * ============================================================================
 *
 *  const 成員函數的本質：
 *    int getHp() const;
 *    等同於：int getHp(const Player* const this);
 *    承諾不通過 this 修改對象
 *
 *  靜態成員函數：
 *    static int getTotalCount();
 *    根本沒有 this 參數！
 *    const 修飾的對象不存在，所以 const 在這裡毫無意義
 *
 *  因此：static void func() const; 會導致編譯錯誤！
 */

/* ============================================================================
 *  第四節：靜態函數透過參數間接訪問非靜態成員
 * ============================================================================
 *
 *  雖然靜態函數沒有 this，不能直接訪問非靜態成員，
 *  但可以透過「接收對象參數」的方式間接訪問。
 *  因為靜態函數仍然是類別的成員，所以可以訪問參數對象的 private 成員。
 */

// --- 範例：角色比較 ---
class Character {
private:
    string name_;                               // 非靜態：每個角色的名字
    int hp_;                                    // 非靜態：每個角色的血量
    inline static int count_ = 0;               // 靜態：角色總數

public:
    Character(const string& name, int hp)
        : name_(name), hp_(hp)
    {
        count_++;
    }

    // 非靜態函數：有 this，可以訪問一切
    void printInfo() const {
        cout << "  " << name_ << " HP:" << hp_         // 訪問非靜態 -> OK
             << " (共 " << count_ << " 人)" << endl;    // 訪問靜態 -> 也OK
    }

    // 靜態函數：沒有 this，只能訪問靜態
    static void printCount() {
        cout << "  角色總數：" << count_ << endl;        // 訪問靜態 -> OK
        // cout << name_;      // 錯誤：哪個對象的 name_？
        // cout << hp_;        // 錯誤：哪個對象的 hp_？
        // printInfo();        // 錯誤：需要 this 才能調用
    }

    // 靜態函數透過參數間接訪問非靜態成員
    // 因為 compare 是 Character 的成員函數，所以可以訪問 private 成員
    // 注意：這裡訪問的是「參數對象」的成員，不是通過 this
    static void compare(const Character& a, const Character& b) {
        cout << "  比較：" << a.name_ << "(HP:" << a.hp_ << ") vs "
             << b.name_ << "(HP:" << b.hp_ << ")" << endl;

        if (a.hp_ > b.hp_)
            cout << "  -> " << a.name_ << " 更強" << endl;
        else if (a.hp_ < b.hp_)
            cout << "  -> " << b.name_ << " 更強" << endl;
        else
            cout << "  -> 不分上下" << endl;
    }
};

/* ============================================================================
 *  第五節：應用一 —— 工廠函數（Factory Function）
 * ============================================================================
 *
 *  靜態函數最經典的用途之一：用靜態函數創建對象。
 *
 *  工廠函數的優勢（相比直接用建構函數）：
 *    1. 函數名傳達了語義（createSmall 比 Potion(30, 50) 清楚）
 *    2. 可以有多個「建構邏輯」使用相同的參數類型
 *    3. 可以在內部添加驗證邏輯
 *    4. 可以控制創建流程
 *
 *  做法：將建構函數設為 private，強制使用者透過靜態工廠函數創建對象。
 */

// --- 範例：藥水工廠 ---
class Potion {
private:
    string name_;
    int healAmount_;
    int price_;

    // 私有建構函數：外部不能直接建構，只能透過工廠函數創建
    Potion(const string& name, int heal, int price)
        : name_(name), healAmount_(heal), price_(price)
    {
    }

public:
    // ====== 靜態工廠函數：提供命名的創建方式 ======
    // 每個工廠函數都創建一種特定類型的藥水，名稱清楚表達用途
    static Potion createSmall() {
        return Potion("小型藥水", 30, 50);
    }

    static Potion createMedium() {
        return Potion("中型藥水", 70, 120);
    }

    static Potion createLarge() {
        return Potion("大型藥水", 150, 300);
    }

    // 帶驗證的自定義創建
    static Potion createCustom(const string& name, int heal, int price) {
        int safeHeal = (heal > 0 && heal <= 999) ? heal : 50;
        int safePrice = (price > 0 && price <= 9999) ? price : 100;
        return Potion(name, safeHeal, safePrice);
    }

    void printInfo() const {
        cout << "  " << name_ << " (回復:" << healAmount_
             << " 價格:" << price_ << " 金幣)" << endl;
    }
};

/* ============================================================================
 *  第六節：應用二 —— 工具函數類（Utility Class）
 * ============================================================================
 *
 *  當一個類別全部都是靜態函數，不需要創建對象時，
 *  可以用 delete 建構函數來禁止實例化。
 *  這種設計模式稱為 "static class" 或 "utility class"。
 *
 *  適合用於：純數學運算、字串處理等不需要對象狀態的工具函數集合。
 */

// --- 範例：數學工具類 ---
class MathUtil {
public:
    // clamp：將值限制在指定範圍內
    // 例如 clamp(150, 0, 100) 返回 100（因為 150 超過上限）
    // 例如 clamp(-50, 0, 100) 返回 0（因為 -50 低於下限）
    static int clamp(int value, int minVal, int maxVal) {
        if (value < minVal) return minVal;
        if (value > maxVal) return maxVal;
        return value;
    }

    // lerp：線性插值，在 a 和 b 之間根據 t 的比例返回插值
    // 例如 lerp(0, 100, 0.3) 返回 30（在 0~100 之間 30% 的位置）
    static double lerp(double a, double b, double t) {
        t = (t < 0.0) ? 0.0 : (t > 1.0) ? 1.0 : t;
        return a + (b - a) * t;
    }

    // distance：計算兩點之間的距離（畢氏定理）
    // 例如 distance(0,0, 3,4) 返回 5
    static double distance(double x1, double y1, double x2, double y2) {
        double dx = x2 - x1;
        double dy = y2 - y1;
        return sqrt(dx * dx + dy * dy);
    }

    // isInRange：檢查 value 是否在 center 的 range 範圍內
    // 例如 isInRange(8, 10, 3) 返回 true（因為 |8-10|=2 <= 3）
    // 例如 isInRange(5, 10, 3) 返回 false（因為 |5-10|=5 > 3）
    static bool isInRange(double value, double center, double range) {
        return abs(value - center) <= range;
    }

    // 禁止創建對象——這個類別只是工具函數的集合
    MathUtil() = delete;
};

/* ============================================================================
 *  第七節：應用三 —— 單例模式存取點（Singleton）
 * ============================================================================
 *
 *  單例模式：保證一個類別只有一個實例。
 *  靜態函數是實現單例的關鍵——通過靜態函數 getInstance() 獲取唯一實例。
 *
 *  實現要點（Meyers' Singleton）：
 *    1. 私有建構函數：外部不能創建
 *    2. 刪除拷貝建構和賦值運算子：禁止複製
 *    3. 靜態函數 getInstance()：內部使用靜態局部變數，只初始化一次
 *    4. C++11 以後，靜態局部變數的初始化是線程安全的
 *
 *  適合用於：管理遊戲狀態、配置、資源等全局共享的對象。
 */

// --- 範例：遊戲管理器單例 ---
class GameManager {
private:
    int score_;
    int level_;
    string currentMap_;

    // 私有建構函數：外部不能創建
    GameManager() : score_(0), level_(1), currentMap_("新手村") {
        cout << "  [GameManager 初始化]" << endl;
    }

public:
    // 禁止拷貝和賦值
    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;

    // 靜態函數：獲取唯一實例
    // 靜態局部變數 instance 只會在第一次調用時初始化，之後每次都返回同一個
    static GameManager& getInstance() {
        static GameManager instance;    // 靜態局部變數，只初始化一次
        return instance;
    }

    // 普通成員函數（操作遊戲狀態）
    void addScore(int points) {
        score_ += points;
        cout << "  得分 +" << points << " (總分:" << score_ << ")" << endl;
    }

    void nextLevel() {
        level_++;
        cout << "  進入第 " << level_ << " 關" << endl;
    }

    void setMap(const string& map) { currentMap_ = map; }

    void printStatus() const {
        cout << "  [遊戲狀態] 地圖:" << currentMap_
             << " 關卡:" << level_ << " 分數:" << score_ << endl;
    }
};

/* ============================================================================
 *  第八節：靜態函數與非靜態函數的互相調用
 * ============================================================================
 *
 *  調用關係圖：
 *
 *  靜態函數 -> 靜態函數    OK
 *  靜態函數 -> 靜態變數    OK
 *  靜態函數 -> 非靜態成員  錯誤（沒有 this）
 *  靜態函數 -> 非靜態函數  錯誤（沒有 this）
 *
 *  非靜態函數 -> 靜態函數  OK
 *  非靜態函數 -> 靜態變數  OK
 *  非靜態函數 -> 非靜態成員 OK（有 this）
 *  非靜態函數 -> 非靜態函數 OK（有 this）
 *
 *  重點：非靜態函數可以訪問一切；靜態函數只能訪問靜態的東西。
 */

// --- 範例：日誌系統 ---
// 展示靜態函數與非靜態函數如何互相配合
class Logger {
private:
    string module_;                         // 非靜態：每個日誌器的模組名
    inline static int logCount_ = 0;       // 靜態：所有日誌器共享的計數
    inline static bool enabled_ = true;    // 靜態：全局日誌開關

public:
    Logger(const string& module) : module_(module) {}

    // ====== 靜態函數：全局控制 ======
    // 這些函數控制整個日誌系統的行為，不需要任何實例
    static void enable() { enabled_ = true; }
    static void disable() { enabled_ = false; }
    static bool isEnabled() { return enabled_; }
    static int getLogCount() { return logCount_; }

    // 靜態函數可以調用其他靜態函數
    static void printSystemInfo() {
        cout << "  [系統] 日誌 " << (isEnabled() ? "啟用" : "停用")   // 調用靜態函數 isEnabled()
             << "，已記錄 " << getLogCount() << " 條" << endl;        // 調用靜態函數 getLogCount()
        // 注意：這裡不能訪問 module_，因為它是非靜態成員
    }

    // ====== 非靜態函數：每個實例的操作 ======
    // 非靜態函數可以同時訪問靜態和非靜態的所有成員
    void log(const string& message) const {
        if (!isEnabled()) return;           // 調用靜態函數 -> OK
        logCount_++;                         // 訪問靜態變數 -> OK
        cout << "  [" << module_ << "] "    // 訪問非靜態成員 -> OK（有 this）
             << message << " (#" << logCount_ << ")" << endl;
    }

    void warn(const string& message) const {
        if (!isEnabled()) return;
        logCount_++;
        cout << "  [警告][" << module_ << "] " << message
             << " (#" << logCount_ << ")" << endl;
    }
};

/* ============================================================================
 *  第九節：靜態函數 vs 全域函數 vs 命名空間函數
 * ============================================================================
 *
 *  方式 1：全域函數
 *    int globalAdd(int a, int b) { return a + b; }
 *    缺點：名字可能衝突、沒有歸屬感、不能訪問 private
 *    極少使用，只在 C 相容性需要時使用
 *
 *  方式 2：命名空間函數
 *    namespace MathUtils { int add(int a, int b) { return a + b; } }
 *    較好：有命名空間隔離，但不能訪問類別的 private
 *    適合：純工具函數，不需要訪問任何類別的 private
 *
 *  方式 3：類別的靜態函數
 *    class Calculator { static int add(int a, int b) { ... } };
 *    最佳：有歸屬、可以訪問 private、可以維護內部狀態
 *    適合：與類別邏輯相關的操作、需要訪問 private、需要維護狀態
 *
 *  選擇指南：
 *    - 與類別邏輯相關 -> 類別靜態函數
 *    - 純工具、與類別無關 -> 命名空間函數
 *    - 全域函數 -> 盡量避免
 */

// --- 範例：三種方式比較 ---
// 全域函數
int globalAdd(int a, int b) { return a + b; }

// 命名空間函數
namespace MathUtils {
    int add(int a, int b) { return a + b; }
}

// 類別靜態函數（可以維護私有狀態）
class Calculator {
private:
    inline static int operationCount_ = 0;  // 私有狀態：記錄運算次數

public:
    static int add(int a, int b) {
        operationCount_++;                   // 可以維護內部狀態
        return a + b;
    }

    static int getOperationCount() { return operationCount_; }
};

/* ============================================================================
 *  第十節：綜合範例 —— 遊戲成就系統
 * ============================================================================
 *
 *  這個範例綜合運用了本課所有概念：
 *    - 靜態成員變數：追蹤全域成就統計（總數、已解鎖數、總點數、已獲得點數）
 *    - 非靜態成員函數：操作單個成就（解鎖、列印資訊）
 *    - 靜態成員函數：類別級操作（顯示進度、計算完成率）
 *    - 靜態工廠函數：創建預設類型的成就
 *    - 靜態函數調用靜態函數：printProgress() 調用 getCompletionRate()
 *    - 非靜態函數修改靜態變數：unlock() 修改 unlockedCount_ 和 earnedPoints_
 */

class Achievement {
private:
    string name_;               // 非靜態：每個成就的名稱
    string description_;        // 非靜態：每個成就的描述
    int points_;                // 非靜態：每個成就的點數
    bool unlocked_;             // 非靜態：是否已解鎖

    // 靜態：全域成就統計（所有 Achievement 實例共享）
    inline static int totalAchievements_ = 0;   // 成就總數
    inline static int unlockedCount_ = 0;        // 已解鎖數量
    inline static int totalPoints_ = 0;          // 所有成就的總點數
    inline static int earnedPoints_ = 0;         // 已獲得的點數

public:
    // 建構函數：每創建一個成就，更新靜態統計
    Achievement(const string& name, const string& desc, int points)
        : name_(name), description_(desc)
        , points_(points > 0 ? points : 10)     // 驗證：點數必須大於 0
        , unlocked_(false)
    {
        totalAchievements_++;                    // 靜態變數：成就總數 +1
        totalPoints_ += points_;                 // 靜態變數：總點數累加
    }

    // ====== 非靜態函數：操作單個成就 ======

    // 解鎖成就（非靜態函數修改靜態變數的典型範例）
    bool unlock() {
        if (unlocked_) {
            cout << "  「" << name_ << "」已經解鎖過了" << endl;
            return false;                        // 防止重複解鎖
        }
        unlocked_ = true;                       // 修改非靜態成員
        unlockedCount_++;                        // 修改靜態變數
        earnedPoints_ += points_;                // 修改靜態變數
        cout << "  成就解鎖！「" << name_ << "」 +"
             << points_ << " 點" << endl;
        cout << "    " << description_ << endl;
        return true;
    }

    // 列印單個成就資訊
    void printInfo() const {
        cout << "  " << (unlocked_ ? "[已解鎖]" : "[未解鎖]") << " "
             << name_ << " (" << points_ << "點)";
        if (!unlocked_) cout << " -- " << description_;
        cout << endl;
    }

    bool isUnlocked() const { return unlocked_; }
    const string& getName() const { return name_; }
    int getPoints() const { return points_; }

    // ====== 靜態函數：類別級別操作 ======

    static int getTotalAchievements() { return totalAchievements_; }
    static int getUnlockedCount() { return unlockedCount_; }

    // 靜態函數調用靜態函數
    static double getCompletionRate() {
        if (totalAchievements_ == 0) return 0.0;
        return static_cast<double>(unlockedCount_) / totalAchievements_ * 100.0;
    }

    // 顯示整體成就進度（只使用靜態成員）
    static void printProgress() {
        cout << "  ========== 成就進度 ==========" << endl;
        cout << "  解鎖：" << unlockedCount_ << " / "
             << totalAchievements_ << endl;
        cout << "  點數：" << earnedPoints_ << " / "
             << totalPoints_ << endl;
        cout << "  完成率：" << getCompletionRate() << "%" << endl;
        cout << "  ===============================" << endl;
    }

    // 靜態工廠函數：創建預設成就
    static Achievement createFirst(const string& action) {
        return Achievement(
            "初次" + action,
            "第一次" + action + "的紀念",
            10
        );
    }

    static Achievement createMilestone(const string& name, int points) {
        return Achievement(name, "里程碑成就", points);
    }
};

/* ============================================================================
 *  附錄：完整訪問規則總表與重點回顧
 * ============================================================================
 *
 *  非靜態成員函數 vs 靜態成員函數 完整對比：
 *
 *  ┌──────────────────┬─────────────────────┬──────────────────────┐
 *  │ 特性             │ 非靜態成員函數        │ 靜態成員函數          │
 *  ├──────────────────┼─────────────────────┼──────────────────────┤
 *  │ this 指標        │ 有                   │ 沒有                 │
 *  │ 訪問非靜態成員    │ OK                  │ 不行                 │
 *  │ 訪問靜態成員      │ OK                  │ OK                   │
 *  │ 需要對象調用      │ 是                   │ 否                   │
 *  │ 可以加 const      │ OK                  │ 不行                 │
 *  │ 推薦調用方式      │ obj.func()          │ Class::func()        │
 *  │ 語法標記         │ 無特殊               │ static               │
 *  │ 典型用途         │ 操作對象數據          │ 工廠、統計、工具、單例 │
 *  └──────────────────┴─────────────────────┴──────────────────────┘
 *
 *  本課重點：
 *    1. static 成員函數：屬於類別，不依賴對象，沒有 this
 *    2. 調用方式：推薦 Class::func()，不推薦 obj.func()
 *    3. 不能加 const：沒有 this，const 無意義
 *    4. 只能訪問靜態成員：非靜態成員需要 this 才能訪問
 *    5. 可以接收對象參數：透過參數間接操作對象（仍可訪問 private）
 *    6. 工廠函數：靜態函數創建對象，提供語義化的建構方式
 *    7. 工具函數類：純運算，不需要對象狀態，可用 delete 禁止實例化
 *    8. 單例存取：透過靜態函數獲取唯一實例（Meyers' Singleton）
 *    9. vs 全域函數：靜態函數有歸屬、可訪問 private、可維護狀態
 *   10. vs 命名空間函數：與類別相關時用靜態函數，純工具用命名空間
 */

// ============================================================================
//  主程式：依序展示所有概念
// ============================================================================

int main() {
    cout << "============================================================" << endl;
    cout << "  第 25 課：類別內的靜態成員函數 —— 完整總結" << endl;
    cout << "============================================================" << endl;

    // --------------------------------------------------
    //  第一節展示：靜態成員函數基礎
    // --------------------------------------------------
    cout << "\n【第一節】靜態成員函數基礎" << endl;
    cout << "------------------------------------------------------------" << endl;

    Bullet normal("普通彈", 10);
    Bullet fireBullet("火焰彈", 25);

    // 非靜態函數：需要透過對象調用
    normal.fire();
    normal.fire();
    fireBullet.fire();
    normal.hit();
    fireBullet.hit();
    fireBullet.fire();
    fireBullet.hit();

    // 靜態函數：推薦用類別名調用（不需要對象）
    cout << "\n  --- 戰鬥統計（類別名調用，推薦）---" << endl;
    Bullet::printStats();

    // 也可以用對象調用（不推薦，但合法）
    cout << "  --- 通過對象調用（不推薦）---" << endl;
    normal.printStats();    // 和 Bullet::printStats() 完全一樣

    // 重置統計
    cout << "  --- 重置 ---" << endl;
    Bullet::resetStats();
    Bullet::printStats();

    // --------------------------------------------------
    //  第二節 & 第四節展示：訪問規則 + 透過參數間接訪問
    // --------------------------------------------------
    cout << "\n【第二節 & 第四節】訪問規則 + 透過參數間接訪問" << endl;
    cout << "------------------------------------------------------------" << endl;

    Character warrior("戰士", 200);
    Character mage("法師", 120);

    // 非靜態函數（需要對象）
    cout << "  --- 非靜態函數 ---" << endl;
    warrior.printInfo();
    mage.printInfo();

    // 靜態函數（不需要對象）
    cout << "  --- 靜態函數 ---" << endl;
    Character::printCount();

    // 靜態函數接收對象參數，間接訪問非靜態的 private 成員
    cout << "  --- 靜態函數接收對象參數 ---" << endl;
    Character::compare(warrior, mage);

    // --------------------------------------------------
    //  第五節展示：工廠函數
    // --------------------------------------------------
    cout << "\n【第五節】工廠函數" << endl;
    cout << "------------------------------------------------------------" << endl;

    // 不能直接建構：Potion p("藥水", 50, 100);  -> 編譯錯誤（private 建構函數）
    // 必須透過靜態工廠函數創建
    Potion small = Potion::createSmall();
    Potion medium = Potion::createMedium();
    Potion large = Potion::createLarge();
    Potion special = Potion::createCustom("秘製靈藥", 500, 2000);

    cout << "  --- 預設藥水 ---" << endl;
    small.printInfo();
    medium.printInfo();
    large.printInfo();
    cout << "  --- 自定義藥水 ---" << endl;
    special.printInfo();

    // --------------------------------------------------
    //  第六節展示：工具函數類
    // --------------------------------------------------
    cout << "\n【第六節】工具函數類" << endl;
    cout << "------------------------------------------------------------" << endl;

    // 不需要創建對象，直接用類別名調用
    cout << "  clamp(150, 0, 100) = " << MathUtil::clamp(150, 0, 100) << endl;
    cout << "  clamp(-50, 0, 100) = " << MathUtil::clamp(-50, 0, 100) << endl;
    cout << "  lerp(0, 100, 0.3)  = " << MathUtil::lerp(0, 100, 0.3) << endl;
    cout << "  lerp(0, 100, 0.7)  = " << MathUtil::lerp(0, 100, 0.7) << endl;
    cout << "  distance(0,0, 3,4) = " << MathUtil::distance(0, 0, 3, 4) << endl;
    cout << "  isInRange(5,10,3)  = " << (MathUtil::isInRange(5, 10, 3) ? "true" : "false") << endl;
    cout << "  isInRange(8,10,3)  = " << (MathUtil::isInRange(8, 10, 3) ? "true" : "false") << endl;
    // MathUtil m;  -> 編譯錯誤！建構函數被 delete

    // --------------------------------------------------
    //  第七節展示：單例模式
    // --------------------------------------------------
    cout << "\n【第七節】單例模式存取點" << endl;
    cout << "------------------------------------------------------------" << endl;

    // 透過靜態函數獲取唯一實例
    cout << "  --- 第一次存取（觸發初始化）---" << endl;
    GameManager::getInstance().printStatus();

    cout << "  --- 遊戲進行 ---" << endl;
    GameManager::getInstance().addScore(100);
    GameManager::getInstance().addScore(250);
    GameManager::getInstance().nextLevel();
    GameManager::getInstance().setMap("暗黑森林");

    cout << "  --- 查看狀態 ---" << endl;
    GameManager::getInstance().printStatus();

    // 驗證是同一個實例
    cout << "  --- 驗證唯一性 ---" << endl;
    GameManager& ref1 = GameManager::getInstance();
    GameManager& ref2 = GameManager::getInstance();
    cout << "  ref1 地址：" << &ref1 << endl;
    cout << "  ref2 地址：" << &ref2 << endl;
    cout << "  是同一個對象：" << (&ref1 == &ref2 ? "是" : "否") << endl;

    // --------------------------------------------------
    //  第八節展示：靜態與非靜態的互動
    // --------------------------------------------------
    cout << "\n【第八節】靜態與非靜態的互動" << endl;
    cout << "------------------------------------------------------------" << endl;

    Logger gameLog("遊戲");
    Logger netLog("網路");

    Logger::printSystemInfo();          // 靜態函數：顯示系統狀態

    cout << "  --- 正常記錄 ---" << endl;
    gameLog.log("遊戲啟動");            // 非靜態函數調用靜態函數 isEnabled()
    netLog.log("連接伺服器");
    gameLog.log("載入地圖");
    netLog.warn("延遲過高");

    Logger::printSystemInfo();          // 靜態函數調用靜態函數

    // 關閉日誌（透過靜態函數控制全局開關）
    cout << "  --- 關閉日誌 ---" << endl;
    Logger::disable();
    gameLog.log("這條不會顯示");        // 因為 enabled_ == false
    netLog.log("這條也不會顯示");
    Logger::printSystemInfo();

    // 重新開啟
    cout << "  --- 重新開啟 ---" << endl;
    Logger::enable();
    gameLog.log("日誌恢復");
    Logger::printSystemInfo();

    // --------------------------------------------------
    //  第九節展示：三種方式比較
    // --------------------------------------------------
    cout << "\n【第九節】全域函數 vs 命名空間函數 vs 靜態函數" << endl;
    cout << "------------------------------------------------------------" << endl;

    cout << "  全域函數：globalAdd(1, 2) = " << globalAdd(1, 2) << endl;
    cout << "  命名空間：MathUtils::add(3, 4) = " << MathUtils::add(3, 4) << endl;
    cout << "  靜態函數：Calculator::add(5, 6) = " << Calculator::add(5, 6) << endl;

    Calculator::add(7, 8);
    Calculator::add(9, 10);
    cout << "  Calculator 運算次數：" << Calculator::getOperationCount() << endl;
    cout << "  （只有類別靜態函數能維護內部狀態！）" << endl;

    // --------------------------------------------------
    //  第十節展示：綜合範例——遊戲成就系統
    // --------------------------------------------------
    cout << "\n【第十節】綜合範例——遊戲成就系統" << endl;
    cout << "============================================================" << endl;

    // 使用工廠函數和建構函數創建成就
    cout << "\n  === 初始化成就系統 ===" << endl;
    Achievement firstKill = Achievement::createFirst("擊殺");       // 靜態工廠
    Achievement firstBoss = Achievement("屠龍者", "擊敗第一個Boss", 50);  // 建構函數
    Achievement collector = Achievement("收藏家", "收集 100 件物品", 30);
    Achievement explorer = Achievement("探索者", "發現所有區域", 40);
    Achievement master = Achievement::createMilestone("大師之路", 100);   // 靜態工廠

    // 顯示初始進度（靜態函數：不需要對象）
    Achievement::printProgress();

    // 列出所有成就（非靜態函數：需要對象）
    cout << "\n  === 成就列表 ===" << endl;
    firstKill.printInfo();
    firstBoss.printInfo();
    collector.printInfo();
    explorer.printInfo();
    master.printInfo();

    // 遊戲過程中解鎖（非靜態函數修改靜態變數）
    cout << "\n  === 遊戲進行中... ===" << endl;
    firstKill.unlock();
    firstBoss.unlock();
    explorer.unlock();

    // 嘗試重複解鎖（驗證防重複機制）
    cout << "\n  --- 嘗試重複解鎖 ---" << endl;
    firstKill.unlock();

    // 查看最終進度
    cout << "\n  === 最終進度 ===" << endl;
    Achievement::printProgress();

    // 最終成就列表
    cout << "\n  === 最終成就列表 ===" << endl;
    firstKill.printInfo();
    firstBoss.printInfo();
    collector.printInfo();
    explorer.printInfo();
    master.printInfo();

    cout << "\n============================================================" << endl;
    cout << "  總結完畢！本課核心：靜態成員函數沒有 this 指標，" << endl;
    cout << "  只能訪問靜態成員，推薦用 Class::func() 調用。" << endl;
    cout << "  典型應用：工廠函數、工具函數類、單例模式、統計功能。" << endl;
    cout << "============================================================" << endl;

    return 0;
}
