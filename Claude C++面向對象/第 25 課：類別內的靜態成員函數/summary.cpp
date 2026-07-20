// =============================================================================
//  第 25 課：類別內的靜態成員函數（summary）  —  沒有 this 的成員函式
// =============================================================================
//
// 【主題資訊 Information】
//   宣告:  class C { static R f(Args...); };            // 類別內加 static
//   定義:  R C::f(Args...) { ... }                      // 類別外定義時「不再寫 static」
//   呼叫:  C::f(...)                （推薦）
//          obj.f(...)              （合法但誤導，物件根本沒被用到）
//   標準版本: C++98 起即有；本檔的 Meyers Singleton 執行緒安全保證是 C++11 起
//   複雜度: 呼叫成本與一般函式相同，且少傳一個 this（少一個隱含參數）
//   標頭檔: <string> <cmath>；本檔實務範例另需 <optional>（C++17）
//
// 【詳細解釋 Explanation】
//
// 【1. 唯一的根本差異：沒有 this】
//   一般成員函式其實隱含收一個參數。概念上：
//       void Bullet::fire()          →  void fire(Bullet* this)
//       static void Bullet::stats()  →  void stats()            // 沒有 this
//   本課後面所有規則，全部都是這一件事的推論：
//     * 不需要物件就能呼叫  → 因為不必準備 this
//     * 不能存取非靜態成員  → 因為找不到「是哪個物件」
//     * 不能加 const        → const 修飾的是 *this，沒有 this 就無從修飾
//     * 不能是 virtual      → 虛擬分派要靠物件裡的 vptr，同樣需要 this
//   把「沒有 this」記牢，這四條就不必背。
//
// 【2. 為什麼 obj.f() 合法卻不推薦】
//   標準允許用物件語法呼叫靜態函式，但編譯器只是取出型別、把物件丟掉，
//   產生的機器碼與 C::f() 完全相同。
//   問題純粹在可讀性：讀者會以為這個呼叫和 obj 的狀態有關，其實無關。
//   注意物件運算式仍然「會被求值」——
//   getBullet().printStats() 裡的 getBullet() 還是會被呼叫，副作用照樣發生。
//
// 【3. 靜態函式仍然是「成員」，所以能碰 private】
//   這是它與全域函式最大的差別。
//   靜態成員函式屬於類別，享有完整的存取權限，
//   因此可以讀寫 private static 資料，也能透過參數存取「傳進來的物件」的 private。
//   本課第四節示範的就是這件事：
//       static void heal(Character& c) { c.hp_ += 10; }   // hp_ 是 private，合法
//   全域函式要做到同樣的事就得宣告 friend。
//
// 【4. 四大典型用途】
//   (a) 工廠函式（named constructor idiom）：
//       建構子不能有名字、也不能「失敗時不產生物件」。
//       靜態工廠可以取有意義的名字（Potion::createHealthPotion()），
//       也可以回傳 optional / nullptr 表示建立失敗。
//   (b) 工具函式類：純運算、不需要物件狀態（MathUtil::clamp()）。
//   (c) 單例存取點：Meyers Singleton，
//       static Logger& instance() { static Logger inst; return inst; }
//   (d) 類別層級的統計 / 查詢：讀寫 static 計數器（Bullet::printStats()）。
//
// 【5. 靜態函式 vs 全域函式 vs 命名空間函式】
//   * 全域函式：無歸屬、容易撞名、碰不到 private。
//   * 命名空間函式：有歸屬、可避免撞名，但一樣碰不到 private。
//     若功能與任何類別的內部狀態無關，命名空間其實是更輕量的選擇。
//   * 靜態成員函式：有歸屬、可碰 private、可維護類別的 static 狀態。
//   判斷準則：需要碰該類別的私有實作 → 靜態成員函式；
//             純粹的自由函式 → 命名空間。
//
// 【概念補充 Concept Deep Dive】
//   * 類別外定義時不可重複寫 static：static 是「宣告時的儲存類別說明子」，
//     寫在定義處會編譯錯誤。同理，預設引數也只寫在宣告處。
//   * 靜態成員函式沒有 cv-qualifier 與 ref-qualifier，
//     所以不能寫 static void f() const、也不能寫 static void f() &&。
//   * 靜態成員函式的位址是「普通函式指標」R(*)(Args)，
//     不是成員函式指標 R(C::*)(Args)。這是它能直接當 C API callback
//     （例如 pthread_create、qsort 的比較函式）的原因。
//   * 靜態函式可以呼叫非靜態函式 —— 但必須自己提供物件：
//       static void run(Widget& w) { w.draw(); }   // 合法
//     反之非靜態函式呼叫靜態函式永遠沒問題（它本來就不需要 this）。
//   * Meyers Singleton 的 function-local static，C++11 起保證初始化是
//     thread-safe 的（編譯器插入 guard 變數）。這是標準保證，不是實作巧合。
//
// 【注意事項 Pay Attention】
//   1. 類別外定義時不要再寫 static，否則編譯錯誤。
//   2. 靜態函式不能加 const / virtual，因為兩者修飾的都是 this。
//   3. 用 obj.f() 呼叫時，obj 的運算式仍會被求值（副作用照樣發生）。
//   4. 靜態函式操作的 static 資料在多執行緒下需自行同步。
//   5. 工具類要禁止實例化，用 = delete 建構子比 private 建構子清楚。
//   6. Singleton 的解構順序仍受靜態解構順序影響，跨物件相依要小心。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】類別內的靜態成員函數
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 靜態成員函數為什麼不能宣告成 const?
//     答：const 成員函式修飾的是隱含的 this 指標
//         （把 C* this 變成 const C* this）。
//         靜態成員函式根本沒有 this，沒有東西可以修飾，
//         所以語法上直接不允許。
//         同樣的理由，它也不能是 virtual、不能有 ref-qualifier。
//     追問：那靜態函式要怎麼保證不修改狀態?
//         → 靠設計而非語法。要嘛不碰任何 static 資料（純函式），
//         要嘛把參數宣告成 const C&，讓編譯器擋住對該物件的修改。
//
// 🔥 Q2. 靜態成員函數和全域函數差在哪裡?為什麼不乾脆用全域函數?
//     答：靜態成員函式仍是類別的成員，有三個全域函式沒有的優勢：
//         (1) 有明確歸屬，Class::func() 一看就知道出處，不會撞名；
//         (2) 可以存取該類別的 private 成員（含傳進來的物件的 private），
//             全域函式要做到得宣告成 friend；
//         (3) 可以讀寫類別的 static 狀態。
//     追問：那和命名空間裡的函式比呢?
//         → 若完全不需要碰 private 或 static 狀態，命名空間更輕量、耦合更低。
//         判準是「需不需要類別的內部實作」。
//
// 🔥 Q3. 為什麼靜態成員函數可以直接當 C API 的 callback?
//     答：因為它的位址型別是普通函式指標 R(*)(Args...)，
//         而非成員函式指標 R(C::*)(Args...)。
//         成員函式指標多帶了「要在哪個物件上呼叫」的資訊，
//         尺寸與呼叫慣例都和 C 的函式指標不相容。
//     追問：那要怎麼把物件資訊傳進 callback?
//         → 用 C API 慣有的 void* user data 參數：
//         靜態函式把它 static_cast 回物件指標再呼叫成員函式。
//         這正是 pthread_create、qsort_r 那類介面的設計原因。
//
// ⚠️ 陷阱. 「obj.staticFunc() 既然是用物件呼叫的，
//            那它應該多少和這個物件有關，至少物件不能是無效的吧?」
//     答：無關。編譯器只從運算式取出「型別」來決定呼叫哪個函式，
//         物件本身完全沒被使用，產生的機器碼與 Class::staticFunc() 相同。
//         但要注意一個真實差異：物件「運算式」仍然會被求值 ——
//         寫 makeObj().staticFunc() 時 makeObj() 照樣會執行，副作用照樣發生。
//     為什麼會錯：把「點運算子」一律理解成「對這個物件做事」。
//         實際上點運算子在這裡只負責名稱查找，
//         真正決定一切的是「這個函式有沒有 this」。
// ═══════════════════════════════════════════════════════════════════════════

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
#include <cstdint>
#include <optional>
#include <vector>
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
    // 注意：成員初始化一律依「宣告順序」執行，與初始化列表寫的順序無關。
    // 這裡讓列表順序 damage_ → type_ 與宣告順序一致；
    // 若寫成 type_(type), damage_(dmg)，g++ -Wall 會發出 -Wreorder 警告，
    // 提醒「你寫的順序不是真正的執行順序」——欄位互相依賴時這就是真 bug。
    Bullet(const string& type, int dmg)
        : damage_(dmg), type_(type)
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

// ============================================================================
//  【LeetCode 實戰範例】靜態成員函數作為「工具函式類」
//    對應第六節：純運算、不依賴任何物件狀態，正是 static 成員函式的典型用途。
//    下面兩題都是純函式：輸入決定輸出、沒有物件狀態可言，
//    寫成 static 成員函式既有歸屬、又不必先造一個沒有意義的物件。
// ============================================================================
class NumberUtil {
public:
    // 工具類不該被實例化：用 = delete 明確禁止（比 private 建構子清楚）
    NumberUtil() = delete;

    // ------------------------------------------------------------------
    // 【LeetCode 實戰範例 1】LeetCode 204. Count Primes
    //   題目：統計嚴格小於 n 的質數個數。
    //   為什麼用到本主題：這是純粹的數值運算，沒有任何物件狀態，
    //         寫成 static 工具函式最自然。
    //   解法：Sieve of Eratosthenes，時間 O(n log log n)、空間 O(n)。
    // ------------------------------------------------------------------
    static int countPrimes(int n) {
        if (n < 3) return 0;                    // 小於 3 時沒有質數（2 不算，因為要 < n）
        vector<char> isComposite(static_cast<size_t>(n), 0);
        int count = 0;
        for (int i = 2; i < n; ++i) {
            if (isComposite[static_cast<size_t>(i)]) continue;
            ++count;
            // 從 i*i 開始標記；用 long long 避免 i*i 溢位
            for (long long j = 1LL * i * i; j < n; j += i) {
                isComposite[static_cast<size_t>(j)] = 1;
            }
        }
        return count;
    }

    // ------------------------------------------------------------------
    // 【LeetCode 實戰範例 2】LeetCode 191. Number of 1 Bits
    //   題目：回傳一個無號整數的二進位表示中 '1' 的個數（Hamming weight）。
    //   為什麼用到本主題：同樣是純函式。
    //   解法：Brian Kernighan —— n &= (n - 1) 每次消掉最低位的那個 1，
    //         迴圈次數等於 1 的個數，而不是固定 32 次。
    // ------------------------------------------------------------------
    static int hammingWeight(uint32_t n) {
        int bits = 0;
        while (n) {
            n &= (n - 1);
            ++bits;
        }
        return bits;
    }
};

// ============================================================================
//  【日常實務範例】靜態工廠函式：解析可能失敗的輸入
//    情境：從設定檔讀進一行 "192.168.1.10"，要轉成 IPv4 位址物件。
//    為什麼非用靜態函式不可：
//      建構子有兩個先天限制 ——
//        (1) 不能取名字，所以無法區分 fromString / fromBytes 這類不同來源；
//        (2) 不能「失敗時不產生物件」，只能丟例外。
//      靜態工廠可以取有意義的名字，也能回傳 std::optional 表示解析失敗，
//      讓「格式錯誤」變成正常的回傳值而不是例外。
//    這個手法有個名字：named constructor idiom。
// ============================================================================
class Ipv4Address {
private:
    // 建構子設為 private：只能經由驗證過的工廠建立，杜絕未驗證的物件
    unsigned char octets_[4];
    Ipv4Address(unsigned char a, unsigned char b,
                unsigned char c, unsigned char d)
        : octets_{a, b, c, d} {}

public:
    // 靜態工廠：解析失敗回傳 nullopt，不丟例外
    static optional<Ipv4Address> fromString(const string& text) {
        int parts[4] = {0, 0, 0, 0};
        int idx = 0;
        int digits = 0;          // 本段已讀入幾位數字，用來擋空段 "1..2.3"
        long value = 0;

        for (size_t i = 0; i <= text.size(); ++i) {
            const bool atEnd = (i == text.size());
            const char ch = atEnd ? '.' : text[i];

            if (ch == '.') {
                if (digits == 0) return nullopt;      // 空的一段
                if (idx > 3)     return nullopt;      // 段數過多
                if (value > 255) return nullopt;      // 超出範圍
                parts[idx++] = static_cast<int>(value);
                value = 0;
                digits = 0;
            } else if (ch >= '0' && ch <= '9') {
                if (++digits > 3) return nullopt;     // 單段最多三位
                value = value * 10 + (ch - '0');
            } else {
                return nullopt;                       // 非法字元
            }
        }
        if (idx != 4) return nullopt;                 // 段數不足

        return Ipv4Address(static_cast<unsigned char>(parts[0]),
                           static_cast<unsigned char>(parts[1]),
                           static_cast<unsigned char>(parts[2]),
                           static_cast<unsigned char>(parts[3]));
    }

    // 另一個工廠：語義化的具名建構，建構子做不到這件事
    static Ipv4Address loopback() { return Ipv4Address(127, 0, 0, 1); }

    string toString() const {
        return to_string(octets_[0]) + "." + to_string(octets_[1]) + "." +
               to_string(octets_[2]) + "." + to_string(octets_[3]);
    }

    bool isPrivate() const {
        // RFC 1918：10/8、172.16/12、192.168/16
        if (octets_[0] == 10) return true;
        if (octets_[0] == 172 && octets_[1] >= 16 && octets_[1] <= 31) return true;
        if (octets_[0] == 192 && octets_[1] == 168) return true;
        return false;
    }
};

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
    // 這裡刻意「不印位址」：位址每次執行都不同（ASLR），
    // 印出來既無法當成穩定的預期輸出，也不是我們真正要驗證的事。
    // 要驗證的是「兩次取得的是不是同一個物件」，直接比較指標即可。
    cout << "  兩次 getInstance() 取得同一物件："
         << (&ref1 == &ref2 ? "是" : "否") << endl;
    cout << "  （位址本身每次執行都不同，故不列印）" << endl;

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

    // --------------------------------------------------
    //  LeetCode 展示：靜態工具函式
    // --------------------------------------------------
    cout << "\n【LeetCode 實戰】靜態成員函數作為工具函式類" << endl;
    cout << "------------------------------------------------------------" << endl;
    cout << "  LeetCode 204. Count Primes" << endl;
    cout << "    countPrimes(10)      = " << NumberUtil::countPrimes(10)
         << "（2,3,5,7 → 預期 4）" << endl;
    cout << "    countPrimes(2)       = " << NumberUtil::countPrimes(2)
         << "（預期 0）" << endl;
    cout << "    countPrimes(0)       = " << NumberUtil::countPrimes(0)
         << "（預期 0）" << endl;
    cout << "    countPrimes(100)     = " << NumberUtil::countPrimes(100)
         << "（預期 25）" << endl;

    cout << "\n  LeetCode 191. Number of 1 Bits" << endl;
    cout << "    hammingWeight(11)         = " << NumberUtil::hammingWeight(11u)
         << "（1011 → 預期 3）" << endl;
    cout << "    hammingWeight(128)        = " << NumberUtil::hammingWeight(128u)
         << "（預期 1）" << endl;
    cout << "    hammingWeight(4294967293) = "
         << NumberUtil::hammingWeight(4294967293u) << "（預期 31）" << endl;
    cout << "  註：兩者都不需要物件，Class::func() 直接呼叫。" << endl;

    // --------------------------------------------------
    //  日常實務展示：靜態工廠函式
    // --------------------------------------------------
    cout << "\n【日常實務】靜態工廠：解析可能失敗的輸入" << endl;
    cout << "------------------------------------------------------------" << endl;

    const string samples[] = {
        "192.168.1.10",     // 合法，私有位址
        "8.8.8.8",          // 合法，公開位址
        "10.0.0.1",         // 合法，私有位址
        "256.1.1.1",        // 非法：超過 255
        "1.2.3",            // 非法：段數不足
        "1..2.3",           // 非法：空的一段
        "1.2.3.4.5",        // 非法：段數過多
        "192.168.1.a"       // 非法：含非數字
    };

    for (const auto& s : samples) {
        auto addr = Ipv4Address::fromString(s);
        cout << "  解析 \"" << s << "\" → ";
        if (addr) {
            cout << addr->toString()
                 << (addr->isPrivate() ? "（私有位址）" : "（公開位址）") << endl;
        } else {
            cout << "格式錯誤（回傳 nullopt，不必丟例外）" << endl;
        }
    }

    cout << "  具名工廠 Ipv4Address::loopback() → "
         << Ipv4Address::loopback().toString() << endl;
    cout << "  ↑ 建構子不能取名字、也不能「失敗時不產生物件」，" << endl;
    cout << "    這兩件事正是靜態工廠函式存在的理由。" << endl;

    cout << "\n============================================================" << endl;
    cout << "  總結完畢！本課核心：靜態成員函數沒有 this 指標，" << endl;
    cout << "  只能訪問靜態成員，推薦用 Class::func() 調用。" << endl;
    cout << "  典型應用：工廠函數、工具函數類、單例模式、統計功能。" << endl;
    cout << "============================================================" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary25

// === 預期輸出 ===
// ============================================================
//   第 25 課：類別內的靜態成員函數 —— 完整總結
// ============================================================
//
// 【第一節】靜態成員函數基礎
// ------------------------------------------------------------
//   發射 普通彈（傷害:10）
//   發射 普通彈（傷害:10）
//   發射 火焰彈（傷害:25）
//   命中！普通彈 造成 10 傷害
//   命中！火焰彈 造成 25 傷害
//   發射 火焰彈（傷害:25）
//   命中！火焰彈 造成 25 傷害
//
//   --- 戰鬥統計（類別名調用，推薦）---
//   發射：4  命中：3  命中率：75%
//   --- 通過對象調用（不推薦）---
//   發射：4  命中：3  命中率：75%
//   --- 重置 ---
//   統計數據已重置
//   發射：0  命中：0  命中率：0%
//
// 【第二節 & 第四節】訪問規則 + 透過參數間接訪問
// ------------------------------------------------------------
//   --- 非靜態函數 ---
//   戰士 HP:200 (共 2 人)
//   法師 HP:120 (共 2 人)
//   --- 靜態函數 ---
//   角色總數：2
//   --- 靜態函數接收對象參數 ---
//   比較：戰士(HP:200) vs 法師(HP:120)
//   -> 戰士 更強
//
// 【第五節】工廠函數
// ------------------------------------------------------------
//   --- 預設藥水 ---
//   小型藥水 (回復:30 價格:50 金幣)
//   中型藥水 (回復:70 價格:120 金幣)
//   大型藥水 (回復:150 價格:300 金幣)
//   --- 自定義藥水 ---
//   秘製靈藥 (回復:500 價格:2000 金幣)
//
// 【第六節】工具函數類
// ------------------------------------------------------------
//   clamp(150, 0, 100) = 100
//   clamp(-50, 0, 100) = 0
//   lerp(0, 100, 0.3)  = 30
//   lerp(0, 100, 0.7)  = 70
//   distance(0,0, 3,4) = 5
//   isInRange(5,10,3)  = false
//   isInRange(8,10,3)  = true
//
// 【第七節】單例模式存取點
// ------------------------------------------------------------
//   --- 第一次存取（觸發初始化）---
//   [GameManager 初始化]
//   [遊戲狀態] 地圖:新手村 關卡:1 分數:0
//   --- 遊戲進行 ---
//   得分 +100 (總分:100)
//   得分 +250 (總分:350)
//   進入第 2 關
//   --- 查看狀態 ---
//   [遊戲狀態] 地圖:暗黑森林 關卡:2 分數:350
//   --- 驗證唯一性 ---
//   兩次 getInstance() 取得同一物件：是
//   （位址本身每次執行都不同，故不列印）
//
// 【第八節】靜態與非靜態的互動
// ------------------------------------------------------------
//   [系統] 日誌 啟用，已記錄 0 條
//   --- 正常記錄 ---
//   [遊戲] 遊戲啟動 (#1)
//   [網路] 連接伺服器 (#2)
//   [遊戲] 載入地圖 (#3)
//   [警告][網路] 延遲過高 (#4)
//   [系統] 日誌 啟用，已記錄 4 條
//   --- 關閉日誌 ---
//   [系統] 日誌 停用，已記錄 4 條
//   --- 重新開啟 ---
//   [遊戲] 日誌恢復 (#5)
//   [系統] 日誌 啟用，已記錄 5 條
//
// 【第九節】全域函數 vs 命名空間函數 vs 靜態函數
// ------------------------------------------------------------
//   全域函數：globalAdd(1, 2) = 3
//   命名空間：MathUtils::add(3, 4) = 7
//   靜態函數：Calculator::add(5, 6) = 11
//   Calculator 運算次數：3
//   （只有類別靜態函數能維護內部狀態！）
//
// 【第十節】綜合範例——遊戲成就系統
// ============================================================
//
//   === 初始化成就系統 ===
//   ========== 成就進度 ==========
//   解鎖：0 / 5
//   點數：0 / 230
//   完成率：0%
//   ===============================
//
//   === 成就列表 ===
//   [未解鎖] 初次擊殺 (10點) -- 第一次擊殺的紀念
//   [未解鎖] 屠龍者 (50點) -- 擊敗第一個Boss
//   [未解鎖] 收藏家 (30點) -- 收集 100 件物品
//   [未解鎖] 探索者 (40點) -- 發現所有區域
//   [未解鎖] 大師之路 (100點) -- 里程碑成就
//
//   === 遊戲進行中... ===
//   成就解鎖！「初次擊殺」 +10 點
//     第一次擊殺的紀念
//   成就解鎖！「屠龍者」 +50 點
//     擊敗第一個Boss
//   成就解鎖！「探索者」 +40 點
//     發現所有區域
//
//   --- 嘗試重複解鎖 ---
//   「初次擊殺」已經解鎖過了
//
//   === 最終進度 ===
//   ========== 成就進度 ==========
//   解鎖：3 / 5
//   點數：100 / 230
//   完成率：60%
//   ===============================
//
//   === 最終成就列表 ===
//   [已解鎖] 初次擊殺 (10點)
//   [已解鎖] 屠龍者 (50點)
//   [未解鎖] 收藏家 (30點) -- 收集 100 件物品
//   [已解鎖] 探索者 (40點)
//   [未解鎖] 大師之路 (100點) -- 里程碑成就
//
// 【LeetCode 實戰】靜態成員函數作為工具函式類
// ------------------------------------------------------------
//   LeetCode 204. Count Primes
//     countPrimes(10)      = 4（2,3,5,7 → 預期 4）
//     countPrimes(2)       = 0（預期 0）
//     countPrimes(0)       = 0（預期 0）
//     countPrimes(100)     = 25（預期 25）
//
//   LeetCode 191. Number of 1 Bits
//     hammingWeight(11)         = 3（1011 → 預期 3）
//     hammingWeight(128)        = 1（預期 1）
//     hammingWeight(4294967293) = 31（預期 31）
//   註：兩者都不需要物件，Class::func() 直接呼叫。
//
// 【日常實務】靜態工廠：解析可能失敗的輸入
// ------------------------------------------------------------
//   解析 "192.168.1.10" → 192.168.1.10（私有位址）
//   解析 "8.8.8.8" → 8.8.8.8（公開位址）
//   解析 "10.0.0.1" → 10.0.0.1（私有位址）
//   解析 "256.1.1.1" → 格式錯誤（回傳 nullopt，不必丟例外）
//   解析 "1.2.3" → 格式錯誤（回傳 nullopt，不必丟例外）
//   解析 "1..2.3" → 格式錯誤（回傳 nullopt，不必丟例外）
//   解析 "1.2.3.4.5" → 格式錯誤（回傳 nullopt，不必丟例外）
//   解析 "192.168.1.a" → 格式錯誤（回傳 nullopt，不必丟例外）
//   具名工廠 Ipv4Address::loopback() → 127.0.0.1
//   ↑ 建構子不能取名字、也不能「失敗時不產生物件」，
//     這兩件事正是靜態工廠函式存在的理由。
//
// ============================================================
//   總結完畢！本課核心：靜態成員函數沒有 this 指標，
//   只能訪問靜態成員，推薦用 Class::func() 調用。
//   典型應用：工廠函數、工具函數類、單例模式、統計功能。
// ============================================================
