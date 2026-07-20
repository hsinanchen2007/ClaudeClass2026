// =============================================================================
//  第 25 課：類別內的靜態成員函數 8（綜合）  —  沒有 this 的成員函式
// =============================================================================
//
// 【主題資訊 Information】
//   宣告:  class C { static R f(Args...); };
//   定義:  R C::f(Args...) { ... }        // 類別外定義時「不再寫 static」
//   呼叫:  C::f(...)（推薦） / obj.f(...)（合法但誤導）
//   標準版本: C++98；Meyers Singleton 的執行緒安全保證是 C++11 起
//   複雜度: 與一般函式相同，且少傳一個隱含的 this 參數
//   標頭檔: <string>
//
// 【詳細解釋 Explanation】
//
// 【1. 全部規則都來自同一件事：沒有 this】
//   一般成員函式概念上隱含收一個參數：
//       void Achievement::unlock()          →  void unlock(Achievement* this)
//       static void Achievement::progress() →  void progress()          // 沒有 this
//   由此可直接推出四條規則，不必死背：
//     * 不需要物件就能呼叫  → 不必準備 this
//     * 不能存取非靜態成員  → 不知道是「哪個物件」的
//     * 不能加 const        → const 修飾的是 *this
//     * 不能是 virtual      → 虛擬分派要靠物件內的 vptr
//
// 【2. 靜態函式仍是「成員」，權限完整】
//   它碰不到的只有「隱含的那個物件」，不是「所有物件」。
//   把物件當參數傳進去，一樣能存取 private 成員：
//       static void compare(const Achievement& a, const Achievement& b);
//   這是它與全域函式最大的差別 —— 全域函式要碰 private 得宣告 friend。
//
// 【3. 四大典型用途（本檔全部出現）】
//   (a) 工廠函式：createFirst() / createMilestone()。
//       建構子不能取名字、也不能「失敗時不產生物件」，工廠兩者都能做。
//   (b) 類別層級的統計：printProgress() 讀 static 計數器，
//       不需要任何 Achievement 物件就能呼叫。
//   (c) 工具函式類：純運算，不依賴物件狀態（見下方 LeetCode 段落）。
//   (d) 單例存取點：Meyers Singleton（本課 5.cpp 有完整實測）。
//
// 【4. 累計量與即時量要分清楚】
//   本檔 totalCount_ 是「總共有幾個成就」，unlockedCount_ 是「已解鎖幾個」。
//   前者只在建構時增加，後者會隨解鎖變動。
//   unlock() 特意檢查「是否已解鎖」再遞增 ——
//   漏掉這個檢查，重複解鎖就會讓計數超過總數。
//   輸出中「嘗試重複解鎖」那段正是在驗證這道防線。
//
// 【概念補充 Concept Deep Dive】
//   * 類別外定義靜態成員函式時不可重複寫 static（那是宣告時的說明子）。
//   * 靜態成員函式的位址是普通函式指標 R(*)(Args)，
//     不是成員函式指標 R(C::*)(Args) —— 所以能直接當 C API callback，
//     也能直接交給 std::sort 當比較函式。
//   * 靜態函式讀寫的 static 資料是全域共享狀態：
//     多執行緒下 unlockedCount_++ 是非原子的讀-改-寫，屬資料競爭（未定義行為）。
//   * 靜態狀態會跨單元測試/跨測資殘留，
//     這是「單筆測資對、整批跑就錯」的常見元凶（本課 24 綜合檔有可執行示範）。
//   * 若一組函式完全不需要碰 private 或 static 狀態，
//     用命名空間比「只有靜態成員的類別」更輕量（見本課 7.cpp）。
//
// 【注意事項 Pay Attention】
//   1. 類別外定義時不要再寫 static，否則編譯錯誤。
//   2. 靜態函式不能加 const / virtual，因為兩者修飾的都是 this。
//   3. obj.f() 呼叫靜態函式時，物件運算式仍會被求值（副作用照樣發生）。
//   4. 靜態函式操作的共享狀態需自行同步（atomic 或鎖）。
//   5. 工具類要禁止實例化，用 = delete 比 private 建構子清楚。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】類別內的靜態成員函數（綜合）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 靜態成員函數為什麼不能是 const，也不能是 virtual?
//     答：兩者修飾／依賴的都是 this。
//         const 成員函式是把 C* this 變成 const C* this，
//         沒有 this 就沒有東西可以修飾；
//         virtual 的動態分派要靠物件內部的 vptr 找 vtable，
//         沒有物件就無從分派。因此語法上直接禁止這兩種寫法。
//     追問：那要怎麼做到「由子類別決定建立哪種物件」?
//         → 那是 factory method pattern，用非靜態的 virtual 函式實作，
//         與靜態工廠（named constructor idiom）是不同的機制。
//
// 🔥 Q2. 靜態成員函數能存取 private 成員嗎?和全域函數差在哪?
//     答：能。存取控制是類別層級的，只要是該類別的成員函式，
//         不論靜不靜態都有完整權限 ——
//         把物件當參數傳進來就能讀它的 private 欄位。
//         全域函式要做到同樣的事必須宣告成 friend，
//         而且沒有歸屬、容易撞名、也無法維護類別的 static 狀態。
//     追問：那和命名空間函式比呢?
//         → 若完全不需要碰 private 或 static 狀態，命名空間更輕量，
//         還能跨檔案擴充並支援 ADL。判準是「需不需要類別的內部實作」。
//
// 🔥 Q3. 為什麼工廠函式一定要是 static?
//     答：因為呼叫它的當下還沒有物件 —— 它的任務就是把物件生出來。
//         非靜態成員函式需要 this，等於要求「先有物件才能造物件」，
//         邏輯上自相矛盾。
//     追問：工廠回傳物件時會發生幾次拷貝?
//         → C++17 起是零次。return C(...) 回傳純右值，
//         標準規定直接在呼叫端的儲存空間就地建構（guaranteed copy elision）。
//
// ⚠️ 陷阱. 「obj.staticFunc() 是用物件呼叫的，
//            所以那個物件至少必須是有效的、而且會影響結果。」
//     答：都不對。編譯器只從運算式取出「型別」來決定呼叫哪個函式，
//         物件本身完全沒被使用，產生的機器碼與 Class::staticFunc() 相同，
//         結果也與那個物件的狀態無關。
//         但有一個真實差異要注意：物件「運算式」仍然會被求值 ——
//         makeObj().staticFunc() 裡的 makeObj() 照樣會執行，副作用照樣發生。
//     為什麼會錯：把點運算子一律理解成「對這個物件做事」。
//         點運算子在這裡只負責名稱查找，
//         真正決定行為的是「這個函式有沒有 this」。
//         正因為容易誤讀，才建議一律寫成 Class::func()。
// ═══════════════════════════════════════════════════════════════════════════

/*
# 第 25 課：類別內的靜態成員函數

---

## 25.1 回顧上一課的問題

上一課我們學了靜態成員變數，也用了一些靜態函數（如 `printStatistics()`）。但我們還沒正式介紹靜態成員函數的完整規則。先看一個問題：

```cpp
class Enemy {
private:
    inline static int totalCount_ = 0;
    string name_;

public:
    Enemy(const string& name) : name_(name) { totalCount_++; }

    // 問題：這個函數需要 this 嗎？
    // 它只用到 totalCount_（靜態的），完全不碰 name_（非靜態的）
    int getTotalCount() const { return totalCount_; }
};

// 使用時：
// Enemy e("哥布林");
// e.getTotalCount();   // 必須先有對象才能調用——不合理
```

`getTotalCount()` 明明只用到靜態數據，卻需要一個對象才能調用。這就是靜態成員函數要解決的問題。

---

## 25.2 靜態成員函數的語法與特性

```
語法：
  static 返回類型 函數名(參數);

特性：
  1. 不依賴任何對象，可以直接用 類別名::函數名() 調用
  2. 沒有 this 指標
  3. 只能訪問靜態成員（變數和函數）
  4. 不能訪問非靜態成員
  5. 不能加 const 修飾（因為沒有 this，const 無意義）
```

```cpp
#include <iostream>
#include <string>
using namespace std;

class Bullet {
private:
    int damage_;
    string type_;

    // 靜態成員變數
    inline static int totalFired_ = 0;
    inline static int totalHit_ = 0;

public:
    Bullet(const string& type, int dmg)
        : type_(type), damage_(dmg)
    {
    }

    // 非靜態函數：有 this，可以訪問一切
    void fire() {
        totalFired_++;
        cout << "  發射 " << type_ << "（傷害:" << damage_ << "）" << endl;
    }

    void hit() {
        totalHit_++;
        cout << "  命中！" << type_ << " 造成 " << damage_ << " 傷害" << endl;
    }

    // ====== 靜態成員函數：沒有 this ======
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

        // 以下會編譯錯誤！靜態函數沒有 this
        // cout << damage_;    // ❌ 不能訪問非靜態成員
        // cout << type_;      // ❌ 不能訪問非靜態成員
        // fire();             // ❌ 不能調用非靜態函數
    }

    static void resetStats() {
        totalFired_ = 0;
        totalHit_ = 0;
        cout << "  統計數據已重置" << endl;
    }
};

int main() {
    cout << "=== 靜態成員函數基礎 ===" << endl;

    Bullet normal("普通彈", 10);
    Bullet fire("火焰彈", 25);

    // 射擊
    normal.fire();
    normal.fire();
    fire.fire();
    normal.hit();
    fire.hit();
    fire.fire();
    fire.hit();

    // 通過類別名調用靜態函數——不需要對象！
    cout << "\n--- 戰鬥統計（類別名調用）---" << endl;
    Bullet::printStats();

    // 也可以通過對象調用（但不推薦）
    cout << "\n--- 通過對象調用（不推薦）---" << endl;
    normal.printStats();   // 和 Bullet::printStats() 完全一樣

    // 重置
    cout << "\n--- 重置 ---" << endl;
    Bullet::resetStats();
    Bullet::printStats();

    return 0;
}
```

### 預期輸出

```
=== 靜態成員函數基礎 ===
  發射 普通彈（傷害:10）
  發射 普通彈（傷害:10）
  發射 火焰彈（傷害:25）
  命中！普通彈 造成 10 傷害
  命中！火焰彈 造成 25 傷害
  發射 火焰彈（傷害:25）
  命中！火焰彈 造成 25 傷害

--- 戰鬥統計（類別名調用）---
  發射：4  命中：3  命中率：75%

--- 通過對象調用（不推薦）---
  發射：4  命中：3  命中率：75%

--- 重置 ---
  統計數據已重置
  發射：0  命中：0  命中率：0%
```

---

## 25.3 為什麼靜態函數不能有 const？

```cpp
class Demo {
public:
    // static void func() const;  // ❌ 編譯錯誤！
    static void func();           // ✅
};
```

原因：

```
const 成員函數的意思是：
  int getHp() const;
  → 等同於：int getHp(const Player* const this);
  → 承諾不通過 this 修改對象

靜態成員函數：
  static int getTotalCount();
  → 根本沒有 this 參數！
  → const 修飾的對象不存在
  → 所以 const 在這裡毫無意義
```

---

## 25.4 靜態函數 vs 非靜態函數的訪問規則

```cpp
#include <iostream>
#include <string>
using namespace std;

class Character {
private:
    string name_;               // 非靜態
    int hp_;                    // 非靜態
    inline static int count_ = 0;  // 靜態

public:
    Character(const string& name, int hp)
        : name_(name), hp_(hp)
    {
        count_++;
    }

    // ====== 非靜態函數：有 this，可以訪問一切 ======
    void printInfo() const {
        cout << "  " << name_ << " HP:" << hp_     // ✅ 非靜態成員
             << " (共 " << count_ << " 人)" << endl; // ✅ 靜態成員也行
    }

    // ====== 靜態函數：沒有 this，只能訪問靜態 ======
    static void printCount() {
        cout << "  角色總數：" << count_ << endl;  // ✅ 靜態成員

        // cout << name_;      // ❌ 哪個對象的 name_？
        // cout << hp_;        // ❌ 哪個對象的 hp_？
        // printInfo();        // ❌ 需要 this 才能調用
    }

    // 靜態函數可以接收對象參數來間接訪問非靜態成員
    static void compare(const Character& a, const Character& b) {
        cout << "  比較：" << a.name_ << "(HP:" << a.hp_ << ") vs "
             << b.name_ << "(HP:" << b.hp_ << ")" << endl;

        // 注意：這裡訪問的是「參數對象」的成員，不是通過 this
        // 因為 compare 是 Character 的成員函數，可以訪問 private
        if (a.hp_ > b.hp_)
            cout << "  → " << a.name_ << " 更強" << endl;
        else if (a.hp_ < b.hp_)
            cout << "  → " << b.name_ << " 更強" << endl;
        else
            cout << "  → 不分上下" << endl;
    }
};

int main() {
    cout << "=== 訪問規則 ===" << endl;

    Character warrior("戰士", 200);
    Character mage("法師", 120);

    cout << "\n--- 非靜態函數（需要對象）---" << endl;
    warrior.printInfo();
    mage.printInfo();

    cout << "\n--- 靜態函數（不需要對象）---" << endl;
    Character::printCount();

    cout << "\n--- 靜態函數接收對象參數 ---" << endl;
    Character::compare(warrior, mage);

    return 0;
}
```

### 預期輸出

```
=== 訪問規則 ===

--- 非靜態函數（需要對象）---
  戰士 HP:200 (共 2 人)
  法師 HP:120 (共 2 人)

--- 靜態函數（不需要對象）---
  角色總數：2

--- 靜態函數接收對象參數 ---
  比較：戰士(HP:200) vs 法師(HP:120)
  → 戰士 更強
```

完整的訪問規則圖：

```
┌──────────────────────────────────────────────────┐
│                訪問規則總表                        │
├─────────────────┬──────────────┬─────────────────┤
│ 調用者           │ 可訪問靜態？  │ 可訪問非靜態？   │
├─────────────────┼──────────────┼─────────────────┤
│ 非靜態成員函數    │ ✅ 可以      │ ✅ 可以（有this）│
│ 靜態成員函數      │ ✅ 可以      │ ❌ 不行（無this）│
│ 外部函數         │ ✅ 如果public│ ✅ 需要對象      │
└─────────────────┴──────────────┴─────────────────┘
```

---

## 25.5 靜態成員函數的典型應用

### 應用一：工廠函數（Factory Function）

靜態函數最經典的用途之一——用靜態函數創建對象：

```cpp
#include <iostream>
#include <string>
using namespace std;

class Potion {
private:
    string name_;
    int healAmount_;
    int price_;

    // 私有建構函數：外部不能直接 new
    Potion(const string& name, int heal, int price)
        : name_(name), healAmount_(heal), price_(price)
    {
    }

public:
    // ====== 靜態工廠函數：提供命名的創建方式 ======
    static Potion createSmall() {
        return Potion("小型藥水", 30, 50);
    }

    static Potion createMedium() {
        return Potion("中型藥水", 70, 120);
    }

    static Potion createLarge() {
        return Potion("大型藥水", 150, 300);
    }

    static Potion createCustom(const string& name, int heal, int price) {
        // 帶驗證的創建
        int safeHeal = (heal > 0 && heal <= 999) ? heal : 50;
        int safePrice = (price > 0 && price <= 9999) ? price : 100;
        return Potion(name, safeHeal, safePrice);
    }

    void printInfo() const {
        cout << "  " << name_ << " (回復:" << healAmount_
             << " 價格:" << price_ << " 金幣)" << endl;
    }
};

int main() {
    cout << "=== 工廠函數 ===" << endl;

    // 不能直接建構：
    // Potion p("藥水", 50, 100);  // ❌ 建構函數是 private

    // 透過靜態工廠函數創建
    cout << "\n--- 預設藥水 ---" << endl;
    Potion small = Potion::createSmall();
    Potion medium = Potion::createMedium();
    Potion large = Potion::createLarge();

    small.printInfo();
    medium.printInfo();
    large.printInfo();

    cout << "\n--- 自定義藥水 ---" << endl;
    Potion special = Potion::createCustom("秘製靈藥", 500, 2000);
    special.printInfo();

    return 0;
}
```

### 預期輸出

```
=== 工廠函數 ===

--- 預設藥水 ---
  小型藥水 (回復:30 價格:50 金幣)
  中型藥水 (回復:70 價格:120 金幣)
  大型藥水 (回復:150 價格:300 金幣)

--- 自定義藥水 ---
  秘製靈藥 (回復:500 價格:2000 金幣)
```

工廠函數的好處：

```
建構函數的限制：
  Potion(50, 100);     // 50 是什麼？100 是什麼？語義不清

工廠函數的優勢：
  Potion::createSmall();    // 明確：創建小型藥水
  Potion::createLarge();    // 明確：創建大型藥水

  1. 函數名傳達了語義
  2. 可以有多個「建構邏輯」使用相同的參數類型
  3. 可以在內部添加驗證邏輯
  4. 可以控制創建流程
```

---

### 應用二：工具函數（Utility Function）

```cpp
#include <iostream>
#include <string>
#include <cmath>
using namespace std;

class MathUtil {
public:
    // 全部是靜態函數——這個類別不需要創建對象
    static int clamp(int value, int minVal, int maxVal) {
        if (value < minVal) return minVal;
        if (value > maxVal) return maxVal;
        return value;
    }

    static double lerp(double a, double b, double t) {
        t = (t < 0.0) ? 0.0 : (t > 1.0) ? 1.0 : t;
        return a + (b - a) * t;
    }

    static double distance(double x1, double y1, double x2, double y2) {
        double dx = x2 - x1;
        double dy = y2 - y1;
        return sqrt(dx * dx + dy * dy);
    }

    static bool isInRange(double value, double center, double range) {
        return abs(value - center) <= range;
    }

    // 禁止創建對象
    MathUtil() = delete;
};

int main() {
    cout << "=== 工具函數類 ===" << endl;

    // 不需要創建對象，直接用類別名調用
    cout << "  clamp(150, 0, 100) = " << MathUtil::clamp(150, 0, 100) << endl;
    cout << "  clamp(-50, 0, 100) = " << MathUtil::clamp(-50, 0, 100) << endl;

    cout << "  lerp(0, 100, 0.3) = " << MathUtil::lerp(0, 100, 0.3) << endl;
    cout << "  lerp(0, 100, 0.7) = " << MathUtil::lerp(0, 100, 0.7) << endl;

    cout << "  distance(0,0, 3,4) = " << MathUtil::distance(0, 0, 3, 4) << endl;

    cout << "  isInRange(5, 10, 3) = "
         << (MathUtil::isInRange(5, 10, 3) ? "true" : "false") << endl;
    cout << "  isInRange(8, 10, 3) = "
         << (MathUtil::isInRange(8, 10, 3) ? "true" : "false") << endl;

    // MathUtil m;  // ❌ 編譯錯誤！建構函數被 delete

    return 0;
}
```

### 預期輸出

```
=== 工具函數類 ===
  clamp(150, 0, 100) = 100
  clamp(-50, 0, 100) = 0
  lerp(0, 100, 0.3) = 30
  lerp(0, 100, 0.7) = 70
  distance(0,0, 3,4) = 5
  isInRange(5, 10, 3) = false
  isInRange(8, 10, 3) = true
```

---

### 應用三：單例存取點（預告概念）

靜態函數也是實現「單例模式」的關鍵——保證一個類別只有一個實例：

```cpp
#include <iostream>
#include <string>
using namespace std;

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
    // 禁止拷貝
    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;

    // 靜態函數：獲取唯一實例
    static GameManager& getInstance() {
        static GameManager instance;   // 靜態局部變數，只初始化一次
        return instance;
    }

    // 普通成員函數
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

int main() {
    cout << "=== 單例存取點（靜態函數）===" << endl;

    // 通過靜態函數獲取唯一實例
    cout << "\n--- 第一次存取 ---" << endl;
    GameManager::getInstance().printStatus();

    cout << "\n--- 遊戲進行 ---" << endl;
    GameManager::getInstance().addScore(100);
    GameManager::getInstance().addScore(250);
    GameManager::getInstance().nextLevel();
    GameManager::getInstance().setMap("暗黑森林");

    cout << "\n--- 查看狀態 ---" << endl;
    GameManager::getInstance().printStatus();

    // 驗證是同一個實例
    cout << "\n--- 驗證唯一性 ---" << endl;
    GameManager& ref1 = GameManager::getInstance();
    GameManager& ref2 = GameManager::getInstance();
    cout << "  ref1 地址：" << &ref1 << endl;
    cout << "  ref2 地址：" << &ref2 << endl;
    cout << "  是同一個對象：" << (&ref1 == &ref2 ? "是" : "否") << endl;

    return 0;
}
```

### 預期輸出

```
=== 單例存取點（靜態函數）===

--- 第一次存取 ---
  [GameManager 初始化]
  [遊戲狀態] 地圖:新手村 關卡:1 分數:0

--- 遊戲進行 ---
  得分 +100 (總分:100)
  得分 +250 (總分:350)
  進入第 2 關

--- 查看狀態 ---
  [遊戲狀態] 地圖:暗黑森林 關卡:2 分數:350

--- 驗證唯一性 ---
  ref1 地址：0x7ffXXXXXXXX
  ref2 地址：0x7ffXXXXXXXX
  是同一個對象：是
```

---

## 25.6 靜態函數與非靜態函數的互相調用

```cpp
#include <iostream>
#include <string>
using namespace std;

class Logger {
private:
    string module_;
    inline static int logCount_ = 0;
    inline static bool enabled_ = true;

public:
    Logger(const string& module) : module_(module) {}

    // ====== 靜態函數 ======
    static void enable() { enabled_ = true; }
    static void disable() { enabled_ = false; }
    static bool isEnabled() { return enabled_; }
    static int getLogCount() { return logCount_; }

    // 靜態函數可以調用其他靜態函數 ✅
    static void printSystemInfo() {
        cout << "  [系統] 日誌 " << (isEnabled() ? "啟用" : "停用")
             << "，已記錄 " << getLogCount() << " 條" << endl;
    }

    // ====== 非靜態函數 ======

    // 非靜態函數可以調用靜態函數 ✅
    void log(const string& message) const {
        if (!isEnabled()) return;    // 調用靜態函數 ✅
        logCount_++;                 // 訪問靜態變數 ✅
        cout << "  [" << module_ << "] " << message   // 訪問非靜態成員 ✅
             << " (#" << logCount_ << ")" << endl;
    }

    void warn(const string& message) const {
        if (!isEnabled()) return;
        logCount_++;
        cout << "  ⚠ [" << module_ << "] " << message
             << " (#" << logCount_ << ")" << endl;
    }
};

int main() {
    cout << "=== 靜態與非靜態的互動 ===" << endl;

    Logger gameLog("遊戲");
    Logger netLog("網路");

    Logger::printSystemInfo();

    cout << "\n--- 正常記錄 ---" << endl;
    gameLog.log("遊戲啟動");
    netLog.log("連接伺服器");
    gameLog.log("載入地圖");
    netLog.warn("延遲過高");

    Logger::printSystemInfo();

    // 關閉日誌
    cout << "\n--- 關閉日誌 ---" << endl;
    Logger::disable();
    gameLog.log("這條不會顯示");
    netLog.log("這條也不會顯示");
    Logger::printSystemInfo();

    // 重新開啟
    cout << "\n--- 重新開啟 ---" << endl;
    Logger::enable();
    gameLog.log("日誌恢復");
    Logger::printSystemInfo();

    return 0;
}
```

### 預期輸出

```
=== 靜態與非靜態的互動 ===
  [系統] 日誌 啟用，已記錄 0 條

--- 正常記錄 ---
  [遊戲] 遊戲啟動 (#1)
  [網路] 連接伺服器 (#2)
  [遊戲] 載入地圖 (#3)
  ⚠ [網路] 延遲過高 (#4)
  [系統] 日誌 啟用，已記錄 4 條

--- 關閉日誌 ---
  [系統] 日誌 停用，已記錄 4 條

--- 重新開啟 ---
  [遊戲] 日誌恢復 (#5)
  [系統] 日誌 啟用，已記錄 5 條
```

調用關係圖：

```
靜態函數 → 靜態函數    ✅
靜態函數 → 靜態變數    ✅
靜態函數 → 非靜態成員  ❌ （沒有 this）
靜態函數 → 非靜態函數  ❌ （沒有 this）

非靜態函數 → 靜態函數  ✅
非靜態函數 → 靜態變數  ✅
非靜態函數 → 非靜態成員 ✅ （有 this）
非靜態函數 → 非靜態函數 ✅ （有 this）
```

---

## 25.7 靜態函數 vs 全域函數 vs 命名空間函數

你可能會想：靜態函數和全域函數有什麼差別？為什麼不直接用全域函數？

```cpp
#include <iostream>
#include <string>
using namespace std;

// ===== 方式 1：全域函數 =====
int globalAdd(int a, int b) { return a + b; }
// 問題：名字可能衝突、沒有歸屬感、不能訪問 private

// ===== 方式 2：命名空間函數 =====
namespace MathUtils {
    int add(int a, int b) { return a + b; }
}
// 較好：有命名空間隔離，但不能訪問類別的 private

// ===== 方式 3：類別的靜態函數 =====
class Calculator {
private:
    inline static int operationCount_ = 0;  // 可以有私有狀態

public:
    static int add(int a, int b) {
        operationCount_++;    // 可以維護內部狀態
        return a + b;
    }

    static int getOperationCount() { return operationCount_; }
};
// 最佳：有歸屬、可以訪問 private、可以維護狀態

int main() {
    cout << "=== 三種方式比較 ===" << endl;

    cout << "  全域函數：" << globalAdd(1, 2) << endl;
    cout << "  命名空間：" << MathUtils::add(3, 4) << endl;
    cout << "  靜態函數：" << Calculator::add(5, 6) << endl;

    Calculator::add(7, 8);
    Calculator::add(9, 10);
    cout << "  Calculator 運算次數：" << Calculator::getOperationCount() << endl;

    return 0;
}
```

### 預期輸出

```
=== 三種方式比較 ===
  全域函數：3
  命名空間：7
  靜態函數：11
  Calculator 運算次數：3
```

何時用哪種：

```
全域函數：
  → 極少使用，容易造成命名衝突
  → 只在 C 相容性需要時使用

命名空間函數：
  → 純工具函數，不需要訪問任何類別的 private
  → 如：字串處理、數學運算

類別靜態函數：
  → 與類別邏輯相關的操作
  → 需要訪問類別的 private 成員
  → 需要維護類別級別的狀態
  → 工廠函數、單例存取、統計功能
```

---

## 25.8 綜合範例：遊戲成就系統

```cpp
#include <iostream>
#include <string>
using namespace std;

class Achievement {
private:
    string name_;
    string description_;
    int points_;
    bool unlocked_;

    // 靜態：全域成就統計
    inline static int totalAchievements_ = 0;
    inline static int unlockedCount_ = 0;
    inline static int totalPoints_ = 0;
    inline static int earnedPoints_ = 0;

public:
    Achievement(const string& name, const string& desc, int points)
        : name_(name), description_(desc)
        , points_(points > 0 ? points : 10)
        , unlocked_(false)
    {
        totalAchievements_++;
        totalPoints_ += points_;
    }

    // ====== 非靜態函數 ======
    bool unlock() {
        if (unlocked_) {
            cout << "  「" << name_ << "」已經解鎖過了" << endl;
            return false;
        }
        unlocked_ = true;
        unlockedCount_++;
        earnedPoints_ += points_;
        cout << "  🏆 成就解鎖！「" << name_ << "」 +"
             << points_ << " 點" << endl;
        cout << "    " << description_ << endl;
        return true;
    }

    void printInfo() const {
        cout << "  " << (unlocked_ ? "🏆" : "🔒") << " "
             << name_ << " (" << points_ << "點)";
        if (!unlocked_) cout << " — " << description_;
        cout << endl;
    }

    bool isUnlocked() const { return unlocked_; }
    const string& getName() const { return name_; }
    int getPoints() const { return points_; }

    // ====== 靜態函數：類別級別操作 ======

    static int getTotalAchievements() { return totalAchievements_; }
    static int getUnlockedCount() { return unlockedCount_; }

    static double getCompletionRate() {
        if (totalAchievements_ == 0) return 0.0;
        return static_cast<double>(unlockedCount_) / totalAchievements_ * 100.0;
    }

    static void printProgress() {
        cout << "  ╔════════════════════════════╗" << endl;
        cout << "  ║      成 就 進 度           ║" << endl;
        cout << "  ╠════════════════════════════╣" << endl;
        cout << "  ║ 解鎖：" << unlockedCount_ << " / "
             << totalAchievements_ << endl;
        cout << "  ║ 點數：" << earnedPoints_ << " / "
             << totalPoints_ << endl;
        cout << "  ║ 完成率：" << getCompletionRate() << "%" << endl;
        cout << "  ╚════════════════════════════╝" << endl;
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

int main() {
    cout << "============================================" << endl;
    cout << "   第 25 課：靜態成員函數 綜合範例" << endl;
    cout << "============================================" << endl;

    // 使用工廠函數創建成就
    cout << "\n=== 初始化成就系統 ===" << endl;
    Achievement firstKill = Achievement::createFirst("擊殺");
    Achievement firstBoss = Achievement("屠龍者", "擊敗第一個Boss", 50);
    Achievement collector = Achievement("收藏家", "收集 100 件物品", 30);
    Achievement explorer = Achievement("探索者", "發現所有區域", 40);
    Achievement master = Achievement::createMilestone("大師之路", 100);

    // 顯示初始進度
    Achievement::printProgress();

    // 列出所有成就
    cout << "\n=== 成就列表 ===" << endl;
    firstKill.printInfo();
    firstBoss.printInfo();
    collector.printInfo();
    explorer.printInfo();
    master.printInfo();

    // 遊戲過程中解鎖
    cout << "\n=== 遊戲進行中... ===" << endl;
    firstKill.unlock();
    firstBoss.unlock();
    explorer.unlock();

    // 嘗試重複解鎖
    cout << "\n--- 嘗試重複解鎖 ---" << endl;
    firstKill.unlock();

    // 查看進度
    cout << "\n=== 當前進度 ===" << endl;
    Achievement::printProgress();

    // 最終成就列表
    cout << "\n=== 最終成就列表 ===" << endl;
    firstKill.printInfo();
    firstBoss.printInfo();
    collector.printInfo();
    explorer.printInfo();
    master.printInfo();

    return 0;
}
```

### 編譯與執行

```bash
g++ -std=c++17 -o lesson25 lesson25.cpp
./lesson25
```

### 預期輸出

```
============================================
   第 25 課：靜態成員函數 綜合範例
============================================

=== 初始化成就系統 ===
  ╔════════════════════════════╗
  ║      成 就 進 度           ║
  ╠════════════════════════════╣
  ║ 解鎖：0 / 5
  ║ 點數：0 / 230
  ║ 完成率：0%
  ╚════════════════════════════╝

=== 成就列表 ===
  🔒 初次擊殺 (10點) — 第一次擊殺的紀念
  🔒 屠龍者 (50點) — 擊敗第一個Boss
  🔒 收藏家 (30點) — 收集 100 件物品
  🔒 探索者 (40點) — 發現所有區域
  🔒 大師之路 (100點) — 里程碑成就

=== 遊戲進行中... ===
  🏆 成就解鎖！「初次擊殺」 +10 點
    第一次擊殺的紀念
  🏆 成就解鎖！「屠龍者」 +50 點
    擊敗第一個Boss
  🏆 成就解鎖！「探索者」 +40 點
    發現所有區域

--- 嘗試重複解鎖 ---
  「初次擊殺」已經解鎖過了

=== 當前進度 ===
  ╔════════════════════════════╗
  ║      成 就 進 度           ║
  ╠════════════════════════════╣
  ║ 解鎖：3 / 5
  ║ 點數：100 / 230
  ║ 完成率：60%
  ╚════════════════════════════╝

=== 最終成就列表 ===
  🏆 初次擊殺 (10點)
  🏆 屠龍者 (50點)
  🔒 收藏家 (30點) — 收集 100 件物品
  🔒 探索者 (40點)
  🔒 大師之路 (100點) — 里程碑成就
```

等等，有個小問題——探索者已經解鎖了，但最後列表中顯示的是 `🔒`？不是的，讓我再確認...實際上探索者已經 `unlock()` 了，`unlocked_` 是 `true`，所以 `printInfo` 會顯示 `🏆`。修正預期輸出：

```
  🏆 探索者 (40點)
```

---

## 25.9 靜態成員函數的完整總結

```
┌──────────────────┬─────────────────────┬──────────────────────┐
│ 特性             │ 非靜態成員函數        │ 靜態成員函數          │
├──────────────────┼─────────────────────┼──────────────────────┤
│ this 指標        │ 有                   │ 沒有                 │
│ 訪問非靜態成員    │ ✅                  │ ❌                   │
│ 訪問靜態成員      │ ✅                  │ ✅                   │
│ 需要對象調用      │ 是                   │ 否                   │
│ 可以加 const      │ ✅                  │ ❌                   │
│ 推薦調用方式      │ obj.func()          │ Class::func()        │
│ 語法標記         │ 無特殊               │ static               │
│ 典型用途         │ 操作對象數據          │ 工廠、統計、工具、單例 │
└──────────────────┴─────────────────────┴──────────────────────┘
```

---

## 25.10 本課重點回顧

| 概念 | 說明 |
|------|------|
| static 成員函數 | 屬於類別，不依賴對象，沒有 this |
| 調用方式 | 推薦 `Class::func()`，不推薦 `obj.func()` |
| 不能加 const | 沒有 this，const 無意義 |
| 只能訪問靜態成員 | 非靜態成員需要 this 才能訪問 |
| 可以接收對象參數 | 通過參數間接操作對象 |
| 工廠函數 | 靜態函數創建對象，提供語義化的建構方式 |
| 工具函數 | 純運算，不需要對象狀態 |
| 單例存取 | 通過靜態函數獲取唯一實例 |
| vs 全域函數 | 靜態函數有歸屬、可訪問 private、可維護狀態 |
| vs 命名空間函數 | 與類別相關時用靜態函數，純工具用命名空間 |

---

## 25.11 下一課預告

下一課是第四階段的最後一課——**第 26 課：this 指標**。我們將深入探討 `this` 的本質：它到底是什麼、在哪些場景必須顯式使用、鏈式調用的原理、以及 `this` 與靜態成員的關係。

準備好進入 **第 26 課：this 指標** 了嗎？
*/



#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>
using namespace std;

class Achievement {
private:
    string name_;
    string description_;
    int points_;
    bool unlocked_;

    // 靜態：全域成就統計
    // 使用 inline static 直接在類內初始化，簡化定義
    // 這些靜態成員變量用於追蹤所有成就的總數、已解鎖的數量、總點數和已獲得的點數
    // 靜態成員變量：類別級別的狀態，所有實例共享
    // 靜態成員變量示例
    inline static int totalAchievements_ = 0;
    inline static int unlockedCount_ = 0;
    inline static int totalPoints_ = 0;
    inline static int earnedPoints_ = 0;

public:
    Achievement(const string& name, const string& desc, int points)
        : name_(name), description_(desc)
        , points_(points > 0 ? points : 10)
        , unlocked_(false)
    {
        totalAchievements_++;
        totalPoints_ += points_;
    }

    // ====== 非靜態函數 ======
    // 非靜態成員函數：操作特定成就實例的行為
    // 非靜態成員函數示例
    bool unlock() {
        if (unlocked_) {
            cout << "  「" << name_ << "」已經解鎖過了" << endl;
            return false;
        }
        unlocked_ = true;
        unlockedCount_++;
        earnedPoints_ += points_;
        cout << "  🏆 成就解鎖！「" << name_ << "」 +"
             << points_ << " 點" << endl;
        cout << "    " << description_ << endl;
        return true;
    }

    void printInfo() const {
        cout << "  " << (unlocked_ ? "🏆" : "🔒") << " "
             << name_ << " (" << points_ << "點)";
        if (!unlocked_) cout << " — " << description_;
        cout << endl;
    }

    bool isUnlocked() const { return unlocked_; }
    const string& getName() const { return name_; }
    int getPoints() const { return points_; }

    // ====== 靜態函數：類別級別操作 ======
    // 靜態成員函數：操作類別級別的行為，無需實例即可調用
    // 靜態成員函數示例
    // 這些靜態成員函數提供了訪問和顯示整體成就進度的功能，並且還有一些工廠函數用於創建特定類型的成就
    // 靜態成員函數：訪問和顯示整體成就進度
    // 靜態工廠函數：創建特定類型的成就
    // 靜態成員函數示例：訪問和顯示整體成就進度

    static int getTotalAchievements() { return totalAchievements_; }
    static int getUnlockedCount() { return unlockedCount_; }

    static double getCompletionRate() {
        if (totalAchievements_ == 0) return 0.0;
        return static_cast<double>(unlockedCount_) / totalAchievements_ * 100.0;
    }

    static void printProgress() {
        cout << "  ╔════════════════════════════╗" << endl;
        cout << "  ║      成 就 進 度           ║" << endl;
        cout << "  ╠════════════════════════════╣" << endl;
        cout << "  ║ 解鎖：" << unlockedCount_ << " / "
             << totalAchievements_ << endl;
        cout << "  ║ 點數：" << earnedPoints_ << " / "
             << totalPoints_ << endl;
        cout << "  ║ 完成率：" << getCompletionRate() << "%" << endl;
        cout << "  ╚════════════════════════════╝" << endl;
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

// ============================================================================
//  【LeetCode 實戰範例】靜態成員函數作為「工具函式類」
//    這兩題的共同點：結果完全由參數決定，沒有任何物件狀態可言。
//    這正是「該不該寫成 static」的判準 ——
//    若函式體裡完全用不到 this 指向的任何成員，它就不該是非靜態的。
// ============================================================================
class ValidationUtil {
public:
    ValidationUtil() = delete;      // 純工具類：禁止實例化

    // ------------------------------------------------------------------
    // 【LeetCode 實戰範例 1】LeetCode 20. Valid Parentheses
    //   題目：判斷只含 ()[]{} 的字串是否括號配對正確。
    //   為什麼用到本主題：純函式，輸入決定輸出，不需要任何物件狀態。
    //   解法：用堆疊；遇左括號推入，遇右括號檢查堆疊頂端是否為對應左括號。
    //   關鍵邊界：字串掃完後堆疊必須為空（"(" 這種只有左括號的要判 false）。
    //   複雜度：時間 O(n)，空間 O(n)。
    // ------------------------------------------------------------------
    static bool isValidParentheses(const string& s) {
        vector<char> stack;
        for (char c : s) {
            if (c == '(' || c == '[' || c == '{') {
                stack.push_back(c);
            } else {
                if (stack.empty()) return false;      // 右括號多於左括號
                const char top = stack.back();
                if ((c == ')' && top != '(') ||
                    (c == ']' && top != '[') ||
                    (c == '}' && top != '{')) {
                    return false;                     // 類型不匹配
                }
                stack.pop_back();
            }
        }
        return stack.empty();                         // 還有沒配對的左括號就是 false
    }

    // ------------------------------------------------------------------
    // 【LeetCode 實戰範例 2】LeetCode 217. Contains Duplicate
    //   題目：判斷陣列中是否有任何值出現至少兩次。
    //   為什麼用到本主題：同樣是純函式。
    //   解法：用 unordered_set 邊插入邊檢查，一發現重複立刻回傳。
    //   複雜度：時間平均 O(n)，空間 O(n)。
    //         （若允許改動輸入，排序後檢查相鄰可做到 O(1) 額外空間）
    // ------------------------------------------------------------------
    static bool containsDuplicate(const vector<int>& nums) {
        unordered_set<int> seen;
        for (int n : nums) {
            if (!seen.insert(n).second) return true;  // insert 回傳 false 代表已存在
        }
        return false;
    }
};

// ============================================================================
//  【日常實務範例】使用者註冊表單的欄位驗證
//    情境：註冊 API 收到表單後要逐欄驗證。這些驗證規則是純粹的
//          「輸入 → 是否合法」判斷，與任何 User 物件都無關 ——
//          總不能為了驗證一個還沒建立的使用者，先建一個 User 出來。
//    因此寫成靜態工具函式最自然，也方便單獨做單元測試。
// ============================================================================
class FormValidator {
public:
    FormValidator() = delete;

    // 使用者名稱：3~16 個字元，只允許英數字與底線
    static bool isValidUsername(const string& name) {
        if (name.size() < 3 || name.size() > 16) return false;
        for (char c : name) {
            const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                            (c >= '0' && c <= '9') || c == '_';
            if (!ok) return false;
        }
        return true;
    }

    // Email：教學用的簡化規則 —— 只檢查「有一個 @、@ 前後皆非空、後段含 .」
    // 真正的 RFC 5322 遠比這複雜，實務上通常交給函式庫或直接寄驗證信
    static bool isValidEmail(const string& email) {
        const size_t at = email.find('@');
        if (at == string::npos) return false;             // 沒有 @
        if (at == 0 || at + 1 >= email.size()) return false;  // @ 在頭或尾
        if (email.find('@', at + 1) != string::npos) return false;  // 多個 @
        const size_t dot = email.find('.', at + 1);
        if (dot == string::npos || dot + 1 >= email.size()) return false;  // 網域缺 .
        return true;
    }

    // 密碼強度：至少 8 碼，且同時含有字母與數字
    static bool isStrongPassword(const string& pw) {
        if (pw.size() < 8) return false;
        bool hasAlpha = false, hasDigit = false;
        for (char c : pw) {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) hasAlpha = true;
            else if (c >= '0' && c <= '9')                        hasDigit = true;
        }
        return hasAlpha && hasDigit;
    }
};

int main() {
    cout << "============================================" << endl;
    cout << "   第 25 課：靜態成員函數 綜合範例" << endl;
    cout << "============================================" << endl;

    // 使用工廠函數創建成就
    cout << "\n=== 初始化成就系統 ===" << endl;
    Achievement firstKill = Achievement::createFirst("擊殺");
    Achievement firstBoss = Achievement("屠龍者", "擊敗第一個Boss", 50);
    Achievement collector = Achievement("收藏家", "收集 100 件物品", 30);
    Achievement explorer = Achievement("探索者", "發現所有區域", 40);
    Achievement master = Achievement::createMilestone("大師之路", 100);

    // 顯示初始進度
    Achievement::printProgress();

    // 列出所有成就
    cout << "\n=== 成就列表 ===" << endl;
    firstKill.printInfo();
    firstBoss.printInfo();
    collector.printInfo();
    explorer.printInfo();
    master.printInfo();

    // 遊戲過程中解鎖
    cout << "\n=== 遊戲進行中... ===" << endl;
    firstKill.unlock();
    firstBoss.unlock();
    explorer.unlock();

    // 嘗試重複解鎖
    cout << "\n--- 嘗試重複解鎖 ---" << endl;
    firstKill.unlock();

    // 查看進度
    cout << "\n=== 當前進度 ===" << endl;
    Achievement::printProgress();

    // 最終成就列表
    cout << "\n=== 最終成就列表 ===" << endl;
    firstKill.printInfo();
    firstBoss.printInfo();
    collector.printInfo();
    explorer.printInfo();
    master.printInfo();

    // ─────────────────────────────────────────────────────────
    // LeetCode 展示：靜態工具函式（不需要任何物件）
    // ─────────────────────────────────────────────────────────
    cout << "\n=== LeetCode 20. Valid Parentheses ===" << endl;
    {
        const string cases[] = {"()", "()[]{}", "(]", "([)]", "{[]}", "(", ""};
        for (const auto& s : cases) {
            cout << "  \"" << s << "\" → " << boolalpha
                 << ValidationUtil::isValidParentheses(s) << endl;
        }
        cout << "  註：空字串視為合法；\"(\" 因結尾堆疊非空而不合法。" << endl;
    }

    cout << "\n=== LeetCode 217. Contains Duplicate ===" << endl;
    {
        vector<vector<int>> cases{
            {1, 2, 3, 1},
            {1, 2, 3, 4},
            {1, 1, 1, 3, 3, 4, 3, 2, 4, 2},
            {}
        };
        for (const auto& v : cases) {
            cout << "  [";
            for (size_t i = 0; i < v.size(); ++i) {
                cout << v[i];
                if (i + 1 < v.size()) cout << ",";
            }
            cout << "] → " << boolalpha
                 << ValidationUtil::containsDuplicate(v) << endl;
        }
    }

    // ─────────────────────────────────────────────────────────
    // 日常實務：註冊表單驗證
    // ─────────────────────────────────────────────────────────
    cout << "\n=== 日常實務：註冊表單欄位驗證 ===" << endl;
    {
        cout << "  --- 使用者名稱（3~16 字元，僅英數與底線）---" << endl;
        const string names[] = {"hsinan", "ab", "user_2026", "bad name", "hsinan@x"};
        for (const auto& n : names) {
            cout << "    \"" << n << "\" → " << boolalpha
                 << FormValidator::isValidUsername(n) << endl;
        }

        cout << "  --- Email（教學用簡化規則）---" << endl;
        const string mails[] = {
            "user@example.com", "a@b.co", "no-at-sign", "@example.com",
            "user@", "a@@b.com", "user@localhost"
        };
        for (const auto& m : mails) {
            cout << "    \"" << m << "\" → " << boolalpha
                 << FormValidator::isValidEmail(m) << endl;
        }
        cout << "    註：user@localhost 被判為不合法，是因為簡化規則要求" << endl;
        cout << "        網域必須含有 '.'。真正的 RFC 5322 遠比這複雜，" << endl;
        cout << "        實務上應交給函式庫，或乾脆寄一封驗證信。" << endl;

        cout << "  --- 密碼強度（至少 8 碼且含字母與數字）---" << endl;
        const string pws[] = {"abc123", "abcdefgh", "12345678", "abcd1234"};
        for (const auto& p : pws) {
            cout << "    \"" << p << "\" → " << boolalpha
                 << FormValidator::isStrongPassword(p) << endl;
        }

        cout << "  ↑ 這些驗證都不需要任何 User 物件 ——" << endl;
        cout << "    總不能為了驗證一個還沒建立的使用者，先建一個出來。" << endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 25 課：類別內的靜態成員函數8.cpp -o static_func8

// === 預期輸出 ===
// ============================================
//    第 25 課：靜態成員函數 綜合範例
// ============================================
//
// === 初始化成就系統 ===
//   ╔════════════════════════════╗
//   ║      成 就 進 度           ║
//   ╠════════════════════════════╣
//   ║ 解鎖：0 / 5
//   ║ 點數：0 / 230
//   ║ 完成率：0%
//   ╚════════════════════════════╝
//
// === 成就列表 ===
//   🔒 初次擊殺 (10點) — 第一次擊殺的紀念
//   🔒 屠龍者 (50點) — 擊敗第一個Boss
//   🔒 收藏家 (30點) — 收集 100 件物品
//   🔒 探索者 (40點) — 發現所有區域
//   🔒 大師之路 (100點) — 里程碑成就
//
// === 遊戲進行中... ===
//   🏆 成就解鎖！「初次擊殺」 +10 點
//     第一次擊殺的紀念
//   🏆 成就解鎖！「屠龍者」 +50 點
//     擊敗第一個Boss
//   🏆 成就解鎖！「探索者」 +40 點
//     發現所有區域
//
// --- 嘗試重複解鎖 ---
//   「初次擊殺」已經解鎖過了
//
// === 當前進度 ===
//   ╔════════════════════════════╗
//   ║      成 就 進 度           ║
//   ╠════════════════════════════╣
//   ║ 解鎖：3 / 5
//   ║ 點數：100 / 230
//   ║ 完成率：60%
//   ╚════════════════════════════╝
//
// === 最終成就列表 ===
//   🏆 初次擊殺 (10點)
//   🏆 屠龍者 (50點)
//   🔒 收藏家 (30點) — 收集 100 件物品
//   🏆 探索者 (40點)
//   🔒 大師之路 (100點) — 里程碑成就
//
// === LeetCode 20. Valid Parentheses ===
//   "()" → true
//   "()[]{}" → true
//   "(]" → false
//   "([)]" → false
//   "{[]}" → true
//   "(" → false
//   "" → true
//   註：空字串視為合法；"(" 因結尾堆疊非空而不合法。
//
// === LeetCode 217. Contains Duplicate ===
//   [1,2,3,1] → true
//   [1,2,3,4] → false
//   [1,1,1,3,3,4,3,2,4,2] → true
//   [] → false
//
// === 日常實務：註冊表單欄位驗證 ===
//   --- 使用者名稱（3~16 字元，僅英數與底線）---
//     "hsinan" → true
//     "ab" → false
//     "user_2026" → true
//     "bad name" → false
//     "hsinan@x" → false
//   --- Email（教學用簡化規則）---
//     "user@example.com" → true
//     "a@b.co" → true
//     "no-at-sign" → false
//     "@example.com" → false
//     "user@" → false
//     "a@@b.com" → false
//     "user@localhost" → false
//     註：user@localhost 被判為不合法，是因為簡化規則要求
//         網域必須含有 '.'。真正的 RFC 5322 遠比這複雜，
//         實務上應交給函式庫，或乾脆寄一封驗證信。
//   --- 密碼強度（至少 8 碼且含字母與數字）---
//     "abc123" → false
//     "abcdefgh" → false
//     "12345678" → false
//     "abcd1234" → true
//   ↑ 這些驗證都不需要任何 User 物件 ——
//     總不能為了驗證一個還沒建立的使用者，先建一個出來。
