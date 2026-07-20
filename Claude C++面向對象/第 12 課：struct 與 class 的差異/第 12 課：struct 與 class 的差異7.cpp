// =============================================================================
//  第 12 課：struct 與 class 的差異 7  —  綜合實戰：用 struct 裝資料、class 管狀態
// =============================================================================
//
// 【主題資訊 Information】
//   主題：本課總整理 —— struct/class 的語法差異、選用準則，以及兩者如何互相組合
//   標準版本：C++98 起 struct/class 即等價；本檔的 NSDMI 需 C++11
//   標頭檔：<string>、<cmath>、<vector>
//   本檔結構：① 下方大段課程講義（markdown）② 可執行的遊戲角色系統實作
//
// 【詳細解釋 Explanation】
//
// 【1. 本課的核心結論：語法上幾乎相同，語意上分工明確】
//   語言層面 struct 與 class 只差兩件事：成員的預設存取權、繼承的預設存取權。
//   但既然功能等價，業界就用這兩個關鍵字來「傳達設計意圖」：
//     * struct → 純資料聚合，各欄位可獨立自由變動，沒有不變條件要守
//     * class  → 有不變條件要維護，需要把資料藏起來、只開放受控的修改入口
//   本檔的遊戲角色系統正是這個分工的完整示範。
//
// 【2. 為什麼 CharacterStats / Vector2D 用 struct？】
//   CharacterStats 是「角色的先天屬性表」：maxHp、attack、defense 各自獨立，
//   任何數值組合都是合法設定（策劃想調成什麼都行），沒有跨欄位規則要守。
//   Vector2D 同理 —— 任何 (x, y) 都是合法座標。
//   這兩個型別若硬套 getter/setter，只會讓「調整數值表」這件事變得極其囉唆，
//   而且擋不掉任何錯誤。用 struct 是正確選擇。
//
// 【3. 為什麼 GameCharacter 必須用 class？】
//   它有多條不變條件，一旦被繞過遊戲就壞了：
//     * 0 <= currentHp <= stats.maxHp   （血量不可為負，也不可超過上限）
//     * 0 <= currentMp <= stats.maxMp
//     * 技能必須先檢查 MP 足夠才能扣除
//   如果 currentHp 是 public，任何一行 `player.currentHp = -5;` 都會讓後續
//   的存活判定、UI 血條、結算邏輯全部出錯，而且極難追查是誰寫壞的。
//   把它設為 private、只開放 takeDamage()/useSkill()，這些規則就由型別系統保證。
//
// 【4. 組合（composition）：class 內含 struct 是最常見的搭配】
//   GameCharacter 內部同時擁有 CharacterStats（先天屬性）與 Vector2D（位置），
//   這是典型的 has-a 關係。要注意的是：
//   把 struct 當成 private 成員之後，外界就無法直接改它 —— 封裝的保護
//   會「傳染」給被包含的成員。所以 struct 本身不封裝完全沒問題，
//   只要包住它的那一層有做好把關即可。
//
// 【5. 傷害公式為什麼要保底 1 點？】
//   actualDamage = damage - defense 若允許為負或 0，高防禦角色會變成無敵，
//   遊戲平衡直接崩潰。保底 1 點是遊戲設計上的常見手法，
//   而它之所以能被可靠地執行，正是因為傷害計算被收斂在 takeDamage() 這一個入口。
//
// 【概念補充 Concept Deep Dive】
//   * struct 當成員時是「內嵌」而非指標：CharacterStats 直接嵌在 GameCharacter
//     的記憶體裡，不需要額外配置、也沒有指標追蹤成本。這是 C++ 相對於
//     Java/C# 的重要優勢（那些語言的物件成員預設是參考）。
//   * GameCharacter 因為含有 std::string 而不再是 trivially copyable，
//     不能 memcpy；但 CharacterStats 與 Vector2D 本身仍然是（見本課第 3 檔）。
//   * moveTo() 每次呼叫都會算一次 sqrt。真實遊戲的移動系統若在熱路徑上，
//     常改用平方距離判斷是否抵達，只在真的需要正規化方向向量時才開根號。
//
// 【注意事項 Pay Attention】
//   1. 所有不修改狀態的查詢函式（length、showStatus、getters）都該標 const，
//      否則 const GameCharacter& 傳進函式後連狀態都印不出來。本檔已全部補上。
//   2. init() 是建構函式尚未教到之前的過渡寫法。它的缺陷是物件在 init() 之前
//      處於「已建構但未初始化」的空窗期。第 13 課的建構函式才是正解。
//   3. 除以距離前務必檢查 dist > 0，否則會產生 0/0 → NaN，
//      而 NaN 會靜默污染後續所有計算（NaN 參與的比較永遠為 false）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】struct 與 class 綜合
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++ 的 struct 和 class 到底差在哪？請完整回答。
//     答：語言層面只有兩點：① 成員預設存取權（struct=public、class=private）；
//         ② 繼承預設存取權（struct=public、class=private）。其餘完全等價 ——
//         struct 一樣能有建構函式、virtual、繼承、模板。實務上用它們傳達意圖：
//         純資料用 struct、需維護不變條件用 class。
//     追問：那 template<class T> 和 template<typename T> 差在哪？→ 完全等價，
//           只是歷史遺留的兩種寫法，與本題的 struct/class 是不同層面的事。
//
// 🔥 Q2. 這個遊戲角色系統中，為什麼 CharacterStats 用 struct 而 GameCharacter 用 class？
//     答：CharacterStats 的每個欄位都能獨立自由設定，任何數值組合都合法，
//         沒有不變條件 → struct。GameCharacter 有「0 <= currentHp <= maxHp」
//         等跨欄位規則必須永遠成立，若欄位公開就會被繞過 → class。
//     追問：把 struct 當 private 成員，它會變得安全嗎？→ 會。外界拿不到它，
//           所有修改都必須經過外層 class 的受控介面，保護會「傳染」下去。
//
// 🔥 Q3. class 內含 struct 成員時，記憶體是怎麼配置的？
//     答：直接內嵌，不是指標。CharacterStats 的所有欄位就躺在 GameCharacter
//         的物件記憶體內，不需額外 heap 配置、沒有間接存取成本。
//         這與 Java/C# 預設把物件成員當參考的模型完全不同。
//     追問：那 sizeof(GameCharacter) 怎麼算？→ 各成員大小加總再加對齊 padding，
//           實際值是實作定義的（本檔輸出有實測）。
//
// ⚠️ 陷阱. moveTo() 裡如果不檢查 dist > 0 會怎樣？
//     答：目標與現在位置相同時 dist 為 0，direction.x / dist 就是 0.0/0.0，
//         IEEE 754 下產生 NaN。NaN 會靜默傳染 —— 位置變 NaN、之後所有距離計算
//         都是 NaN，而且 NaN 參與的任何比較（含 == 自己）都回 false，
//         連 `if (pos.x != pos.x)` 以外的檢查都抓不到，極難除錯。
//     為什麼會錯：多數人以為除以零會 crash 或丟例外。整數除以零確實是 UB，
//         但浮點除以零在 IEEE 754 下有明確定義（產生 inf 或 NaN），程式照跑不誤，
//         錯誤會延後到很遠的地方才爆發。
// ═══════════════════════════════════════════════════════════════════════════

/*
# 第 12 課：struct 與 class 的差異

## 一、C++ 中 struct 和 class 幾乎相同

這可能會讓你意外：**在 C++ 中，struct 和 class 只有一個語法層面的差異**——預設存取修飾符不同。除此之外，struct 能做的所有事情 class 都能做，反之亦然。

```cpp
#include <iostream>
#include <string>
using namespace std;

// 用 struct 寫的「完整 OOP」
struct DogStruct {
private:
    string name;
    int age = 0;

public:
    void setName(const string& n) { name = n; }
    void setAge(int a) { age = a; }

    void bark() {
        cout << name << "（struct）汪汪！" << endl;
    }
};

// 用 class 寫的完全等價版本
class DogClass {
private:
    string name;
    int age = 0;

public:
    void setName(const string& n) { name = n; }
    void setAge(int a) { age = a; }

    void bark() {
        cout << name << "（class）汪汪！" << endl;
    }
};

int main() {
    DogStruct ds;
    ds.setName("旺財");
    ds.bark();

    DogClass dc;
    dc.setName("小黑");
    dc.bark();

    return 0;
}
```

**預期輸出：**
```
旺財（struct）汪汪！
小黑（class）汪汪！
```

功能上**完全一樣**。struct 在 C++ 中也可以有 private 成員、成員函數、繼承、虛函數——一切 class 能做的。

---

## 二、唯一的語法差異

### 差異一：預設存取修飾符

```cpp
#include <iostream>
using namespace std;

struct StructExample {
    int x = 10;    // 預設 public
    void show() { cout << "struct x = " << x << endl; }
};

class ClassExample {
    int x = 10;    // 預設 private
    void show() { cout << "class x = " << x << endl; }
};

int main() {
    StructExample s;
    cout << s.x << endl;    // ✅ public，可以存取
    s.show();               // ✅ public

    ClassExample c;
    // cout << c.x << endl; // ❌ 編譯錯誤！private
    // c.show();            // ❌ 編譯錯誤！private

    return 0;
}
```

### 差異二：預設繼承方式

這個差異要到第八階段（繼承）才會用到，這裡先做個預告：

```cpp
struct Base {
    int value = 42;
};

struct DerivedStruct : Base { };     // 預設 public 繼承
class DerivedClass : Base { };       // 預設 private 繼承
```

**完整對比表**：

| 特性 | struct | class |
|------|--------|-------|
| 預設成員存取 | `public` | `private` |
| 預設繼承方式 | `public` | `private` |
| 能否有 private 成員 | ✅ 可以 | ✅ 可以 |
| 能否有成員函數 | ✅ 可以 | ✅ 可以 |
| 能否繼承 | ✅ 可以 | ✅ 可以 |
| 能否有虛函數 | ✅ 可以 | ✅ 可以 |
| 能否用模板 | ✅ 可以 | ✅ 可以 |

**除了預設存取和預設繼承方式，沒有任何差異。**

---

## 三、C 的 struct vs C++ 的 struct

這個差異反而更大。你在 C 語言課程中用的 struct，和 C++ 的 struct 是不一樣的：

```cpp
#include <iostream>
#include <string>
using namespace std;

// ===== C 語言的 struct（在 C++ 中也能編譯）=====
// 只能有資料，不能有函數
struct C_Style {
    char name[50];
    int age;
    float score;
};
// C 中必須寫 struct C_Style s; （C++ 中可省略 struct 關鍵字）

// ===== C++ 的 struct =====
// 可以有函數、建構函數、存取控制、繼承……
struct CPP_Style {
    string name;
    int age = 0;
    float score = 0.0f;

    void show() {
        cout << name << ", " << age << " 歲, " << score << " 分" << endl;
    }

    bool isPassing() {
        return score >= 60.0f;
    }
};

int main() {
    // C 風格
    C_Style cs;
    // strcpy(cs.name, "小明");  // 需要 cstring
    cs.age = 20;
    cs.score = 85.5f;

    // C++ 風格
    CPP_Style cpps;
    cpps.name = "小明";
    cpps.age = 20;
    cpps.score = 85.5f;
    cpps.show();
    cout << "及格: " << (cpps.isPassing() ? "是" : "否") << endl;

    return 0;
}
```

**預期輸出：**
```
小明, 20 歲, 85.5 分
及格: 是
```

| 特性 | C 的 struct | C++ 的 struct |
|------|------------|--------------|
| 成員函數 | ❌ 不支援 | ✅ 支援 |
| 存取控制 | ❌ 全部公開 | ✅ public/private/protected |
| 建構/解構函數 | ❌ 不支援 | ✅ 支援 |
| 繼承 | ❌ 不支援 | ✅ 支援 |
| 宣告變數時 | 必須寫 `struct Dog d;` | 可以直接寫 `Dog d;` |
| 類內初始化 | ❌ 不支援 | ✅ 支援（C++11） |

---

## 四、什麼時候用 struct，什麼時候用 class？

這是 C++ 社群中最常見的慣例（不是強制規定，而是約定俗成）：

### 用 struct：簡單資料的集合

當一個型別主要是**一組資料的集合**，沒有複雜的行為邏輯，所有成員都是 public 時，用 `struct`：

```cpp
#include <iostream>
using namespace std;

// ✅ 適合用 struct —— 純資料集合
struct Point {
    double x = 0;
    double y = 0;
};

struct Color {
    int r = 0;
    int g = 0;
    int b = 0;
    int a = 255;
};

struct Config {
    int width = 800;
    int height = 600;
    bool fullscreen = false;
    int fps = 60;
};

struct DateInfo {
    int year = 2025;
    int month = 1;
    int day = 1;
};

int main() {
    Point p;
    p.x = 3.5;
    p.y = 7.2;
    cout << "Point(" << p.x << ", " << p.y << ")" << endl;

    Color red;
    red.r = 255;
    cout << "Color(" << red.r << ", " << red.g << ", " << red.b << ")" << endl;

    Config cfg;
    cout << cfg.width << "x" << cfg.height
         << (cfg.fullscreen ? " 全螢幕" : " 視窗") << endl;

    return 0;
}
```

**預期輸出：**
```
Point(3.5, 7.2)
Color(255, 0, 0)
800x600 視窗
```

**這些型別的共同特點**：
- 所有成員都是 public
- 沒有資料驗證需求（任何值都合理）
- 只是把幾個相關的值綁在一起
- 很少或沒有成員函數

---

### 用 class：有行為、需要保護的對象

當一個型別有**複雜行為**、需要**資料保護**、有**不變條件（invariant）**時，用 `class`：

```cpp
#include <iostream>
#include <string>
using namespace std;

// ✅ 適合用 class —— 有行為、有保護
class BankAccount {
public:
    void init(const string& name, double initial) {
        ownerName = name;
        if (initial >= 0) balance = initial;
    }

    bool deposit(double amount) {
        if (amount <= 0) return false;
        balance += amount;
        return true;
    }

    bool withdraw(double amount) {
        if (amount <= 0 || amount > balance) return false;
        balance -= amount;
        return true;
    }

    double getBalance() { return balance; }
    string getOwner() { return ownerName; }

private:
    string ownerName;
    double balance = 0.0;   // 不變條件：balance >= 0
};

int main() {
    BankAccount acc;
    acc.init("陳信安", 1000);
    acc.deposit(500);
    acc.withdraw(200);
    cout << acc.getOwner() << ": $" << acc.getBalance() << endl;
    return 0;
}
```

**預期輸出：**
```
陳信安: $1300
```

**這個型別需要 class 因為**：
- `balance` 有不變條件：不能為負數
- 修改 `balance` 必須經過驗證
- 外界不應該直接接觸內部資料

---

### struct 也能加少量函數

struct 加上一些便利函數是完全正常的，不需要因為加了函數就改成 class：

```cpp
#include <iostream>
#include <cmath>
using namespace std;

struct Point {
    double x = 0;
    double y = 0;

    // 便利函數 —— 不改變「純資料集合」的本質
    double distanceTo(const Point& other) {
        double dx = x - other.x;
        double dy = y - other.y;
        return sqrt(dx * dx + dy * dy);
    }

    void print() {
        cout << "(" << x << ", " << y << ")" << endl;
    }
};

struct Rectangle {
    double width = 0;
    double height = 0;

    double area() { return width * height; }
    double perimeter() { return 2 * (width + height); }
};

int main() {
    Point a, b;
    a.x = 0; a.y = 0;
    b.x = 3; b.y = 4;

    a.print();
    b.print();
    cout << "距離: " << a.distanceTo(b) << endl;

    Rectangle r;
    r.width = 5;
    r.height = 3;
    cout << "面積: " << r.area() << ", 周長: " << r.perimeter() << endl;

    return 0;
}
```

**預期輸出：**
```
(0, 0)
(3, 4)
距離: 5
面積: 15, 周長: 16
```

**關鍵判斷標準**：成員是不是全部 public？有沒有需要保護的不變條件？如果都是 public 且沒有不變條件，就算有函數也可以用 struct。

---

## 五、決策流程圖

```
設計一個新型別時：

所有成員都應該公開嗎？
├── 是 → 有需要保護的不變條件嗎？
│         ├── 否 → 用 struct ✅
│         │         （純資料集合、配置、座標、顏色等）
│         │
│         └── 是 → 用 class ✅
│                   （即使目前都是 public，未來可能需要保護）
│
└── 否 → 用 class ✅
          （有 private 成員、複雜行為、資料驗證）
```

---

## 六、真實專案中的 struct vs class

讓我們看一個同時使用兩者的完整範例：

```cpp
#include <iostream>
#include <string>
#include <cmath>
using namespace std;

// ===== struct：純資料 =====
struct Vector2D {
    double x = 0;
    double y = 0;

    double length() {
        return sqrt(x * x + y * y);
    }

    Vector2D add(const Vector2D& other) {
        Vector2D result;
        result.x = x + other.x;
        result.y = y + other.y;
        return result;
    }
};

struct CharacterStats {
    int maxHp = 100;
    int maxMp = 50;
    int attack = 10;
    int defense = 5;
    double speed = 1.0;
};

// ===== class：有行為和保護 =====
class GameCharacter {
public:
    void init(const string& charName, const CharacterStats& charStats) {
        name = charName;
        stats = charStats;
        currentHp = stats.maxHp;
        currentMp = stats.maxMp;
    }

    void moveTo(const Vector2D& target) {
        Vector2D direction;
        direction.x = target.x - position.x;
        direction.y = target.y - position.y;

        double dist = direction.length();
        if (dist > 0) {
            // 根據速度移動（簡化版）
            position.x += (direction.x / dist) * stats.speed;
            position.y += (direction.y / dist) * stats.speed;
        }
        cout << name << " 移動到 (" << position.x << ", " << position.y << ")" << endl;
    }

    bool takeDamage(int damage) {
        int actualDamage = damage - stats.defense;
        if (actualDamage < 1) actualDamage = 1;  // 最少受 1 點傷害

        currentHp -= actualDamage;
        if (currentHp < 0) currentHp = 0;

        cout << name << " 受到 " << actualDamage << " 點傷害"
             << "（HP: " << currentHp << "/" << stats.maxHp << "）" << endl;

        return currentHp > 0;  // 返回是否存活
    }

    bool useSkill(int mpCost) {
        if (currentMp < mpCost) {
            cout << name << ": MP 不足！" << endl;
            return false;
        }
        currentMp -= mpCost;
        cout << name << " 使用技能"
             << "（MP: " << currentMp << "/" << stats.maxMp << "）" << endl;
        return true;
    }

    void showStatus() {
        cout << "--- " << name << " ---" << endl;
        cout << "HP: " << currentHp << "/" << stats.maxHp << endl;
        cout << "MP: " << currentMp << "/" << stats.maxMp << endl;
        cout << "位置: (" << position.x << ", " << position.y << ")" << endl;
        cout << "攻擊: " << stats.attack << " / 防禦: " << stats.defense << endl;
    }

private:
    string name;
    CharacterStats stats;   // struct 作為 class 的成員
    Vector2D position;      // struct 作為 class 的成員
    int currentHp = 0;
    int currentMp = 0;
};

int main() {
    // 用 struct 定義角色屬性（純資料）
    CharacterStats warriorStats;
    warriorStats.maxHp = 150;
    warriorStats.maxMp = 30;
    warriorStats.attack = 20;
    warriorStats.defense = 12;
    warriorStats.speed = 0.8;

    CharacterStats mageStats;
    mageStats.maxHp = 80;
    mageStats.maxMp = 120;
    mageStats.attack = 8;
    mageStats.defense = 3;
    mageStats.speed = 1.2;

    // 用 class 建立角色（有行為和保護）
    GameCharacter warrior;
    warrior.init("戰士", warriorStats);

    GameCharacter mage;
    mage.init("法師", mageStats);

    // 遊戲模擬
    cout << "===== 初始狀態 =====" << endl;
    warrior.showStatus();
    cout << endl;
    mage.showStatus();

    cout << "\n===== 戰鬥 =====" << endl;
    Vector2D target;
    target.x = 5.0;
    target.y = 3.0;
    warrior.moveTo(target);

    mage.useSkill(40);           // 法師放技能
    warrior.takeDamage(35);      // 戰士受到攻擊
    warrior.takeDamage(35);      // 再次受到攻擊

    mage.useSkill(40);           // 法師再放技能
    mage.useSkill(40);           // 第三次
    mage.useSkill(40);           // MP 不足？

    cout << "\n===== 最終狀態 =====" << endl;
    warrior.showStatus();
    cout << endl;
    mage.showStatus();

    return 0;
}
```

**預期輸出：**
```
===== 初始狀態 =====
--- 戰士 ---
HP: 150/150
MP: 30/30
位置: (0, 0)
攻擊: 20 / 防禦: 12

--- 法師 ---
HP: 80/80
MP: 120/120
位置: (0, 0)
攻擊: 8 / 防禦: 3

===== 戰鬥 =====
戰士 移動到 (0.685994, 0.411597)
法師 使用技能（MP: 80/120）
戰士 受到 23 點傷害（HP: 127/150）
戰士 受到 23 點傷害（HP: 104/150）
法師 使用技能（MP: 40/120）
法師 使用技能（MP: 0/120）
法師: MP 不足！

===== 最終狀態 =====
--- 戰士 ---
HP: 104/150
MP: 30/30
位置: (0.685994, 0.411597)
攻擊: 20 / 防禦: 12

--- 法師 ---
HP: 80/80
MP: 0/120
位置: (0, 0)
攻擊: 8 / 防禦: 3
```

**設計分析**：

| 型別 | 用什麼 | 原因 |
|------|--------|------|
| `Vector2D` | struct | 純資料（x, y），所有成員公開，無不變條件 |
| `CharacterStats` | struct | 純資料配置，所有成員公開 |
| `GameCharacter` | class | 有行為邏輯、資料保護（HP 不能直接被設為負數）、不變條件（currentHp ≤ maxHp） |

---

## 七、常見誤解澄清

### 誤解 1：「struct 是 C 的東西，C++ 應該用 class」

**錯誤**。C++ 的 struct 是一等公民，在標準庫中大量使用。例如 `std::pair` 就是 struct。

### 誤解 2：「struct 不能有成員函數」

**錯誤**。C++ 的 struct 可以有成員函數、建構函數、虛函數——一切 class 能做的。

### 誤解 3：「用了 private 就必須改成 class」

**不一定**，但這是一個強烈的信號。如果你的 struct 開始需要 private，通常意味著它已經不是「簡單資料集合」了，應該改用 class。

### 誤解 4：「struct 效能比 class 好」

**完全一樣**。編譯器對 struct 和 class 的處理方式完全相同，不存在效能差異。

---

## 本課重點回顧

| 概念 | 說明 |
|------|------|
| 語法差異 | 只有預設存取（struct=public, class=private）和預設繼承方式 |
| 功能差異 | 沒有。struct 能做的 class 都能做 |
| struct 使用時機 | 純資料集合、全 public、無不變條件 |
| class 使用時機 | 有行為、有保護、有不變條件 |
| C struct vs C++ struct | C++ struct 功能遠超 C struct |
| 效能差異 | 完全沒有 |
| 實務慣例 | 看需求選擇，保持團隊一致 |

---

第二階段「類別與對象基礎」到此完成！🎉

接下來進入 **第三階段：建構與解構**，從 **第 13 課：建構函數（constructor）基礎** 開始。建構函數是建立對象時自動呼叫的特殊函數，它會取代我們目前用的 `init()` 函數，讓對象的初始化更安全、更優雅。準備好就告訴我！
*/



#include <iostream>
#include <string>
#include <cmath>
#include <vector>
#include <algorithm>
using namespace std;

// ===== struct：純資料 =====
// x 與 y 彼此獨立、任何值都是合法座標 → 沒有不變條件 → 用 struct 正確
struct Vector2D {
    double x = 0;
    double y = 0;

    // 查詢函式一律標 const：不改狀態，且讓 const Vector2D& 也能呼叫
    double length() const {
        return sqrt(x * x + y * y);
    }

    // 只需要比大小時用平方長度，可省去一次 sqrt
    double lengthSquared() const {
        return x * x + y * y;
    }

    Vector2D add(const Vector2D& other) const {
        Vector2D result;
        result.x = x + other.x;
        result.y = y + other.y;
        return result;
    }
};

struct CharacterStats {
    int maxHp = 100;
    int maxMp = 50;
    int attack = 10;
    int defense = 5;
    double speed = 1.0;
};

// ===== class：有行為和保護 =====
class GameCharacter {
public:
    void init(const string& charName, const CharacterStats& charStats) {
        name = charName;
        stats = charStats;
        currentHp = stats.maxHp;
        currentMp = stats.maxMp;
    }

    void moveTo(const Vector2D& target) {
        Vector2D direction;
        direction.x = target.x - position.x;
        direction.y = target.y - position.y;

        double dist = direction.length();
        if (dist > 0) {
            // 根據速度移動（簡化版）
            position.x += (direction.x / dist) * stats.speed;
            position.y += (direction.y / dist) * stats.speed;
        }
        cout << name << " 移動到 (" << position.x << ", " << position.y << ")" << endl;
    }

    bool takeDamage(int damage) {
        int actualDamage = damage - stats.defense;
        if (actualDamage < 1) actualDamage = 1;  // 最少受 1 點傷害

        currentHp -= actualDamage;
        if (currentHp < 0) currentHp = 0;

        cout << name << " 受到 " << actualDamage << " 點傷害"
             << "（HP: " << currentHp << "/" << stats.maxHp << "）" << endl;

        return currentHp > 0;  // 返回是否存活
    }

    bool useSkill(int mpCost) {
        if (currentMp < mpCost) {
            cout << name << ": MP 不足！" << endl;
            return false;
        }
        currentMp -= mpCost;
        cout << name << " 使用技能"
             << "（MP: " << currentMp << "/" << stats.maxMp << "）" << endl;
        return true;
    }

    // 查詢函式標 const：const GameCharacter& 傳進函式後仍能印出狀態
    void showStatus() const {
        cout << "--- " << name << " ---" << endl;
        cout << "HP: " << currentHp << "/" << stats.maxHp << endl;
        cout << "MP: " << currentMp << "/" << stats.maxMp << endl;
        cout << "位置: (" << position.x << ", " << position.y << ")" << endl;
        cout << "攻擊: " << stats.attack << " / 防禦: " << stats.defense << endl;
    }

    // 唯讀存取器：外界能看狀態，但改不了 —— 這正是 class 的價值
    bool     isAlive() const { return currentHp > 0; }
    int      getHp()   const { return currentHp; }
    string   getName() const { return name; }
    Vector2D getPos()  const { return position; }

private:
    string name;
    CharacterStats stats;   // struct 作為 class 的成員（直接內嵌，不是指標）
    Vector2D position;      // struct 作為 class 的成員
    int currentHp = 0;
    int currentMp = 0;
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1266. Minimum Time Visiting All Points
//   題目：平面上給一串點，必須「依序」走訪。每秒可以水平、垂直或斜向移動一格，
//         求走完所有點的最短秒數。
//   為什麼用到本主題：這題的輸入天生就是一串座標，正好用本檔的 Vector2D
//         這種純資料 struct 表達。核心觀察是：因為允許斜走，兩點間的最短時間
//         是「切比雪夫距離」max(|dx|, |dy|) —— 斜走能同時消耗一格 x 與一格 y，
//         所以較短的那個軸是「順便」走完的，不額外花時間。
//   複雜度：O(N) 時間、O(1) 額外空間。
// -----------------------------------------------------------------------------
int minTimeToVisitAllPoints(const vector<Vector2D>& points) {
    int total = 0;
    for (size_t i = 1; i < points.size(); ++i) {
        double dx = fabs(points[i].x - points[i - 1].x);
        double dy = fabs(points[i].y - points[i - 1].y);
        total += static_cast<int>(max(dx, dy));   // 切比雪夫距離
    }
    return total;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】戰鬥結算報表：從一群角色中統計存活狀況
//   情境：回合制遊戲每回合結束時，伺服器要產生一份戰報 —— 誰還活著、
//   總血量剩多少、誰血量最低（優先補血目標）。
//   重點在於：這個函式只需要「讀」角色狀態，因此參數用 const 參考傳入。
//   而它之所以能安全地只讀，正是因為 GameCharacter 把資料設成 private 並
//   只開放 const 存取器 —— 呼叫端就算想改也改不了，編譯器直接擋下。
//   這是封裝在多人協作專案中最實際的價值。
//   回傳型別用 struct：四個欄位彼此獨立、沒有不變條件，正是 struct 的標準用途。
// -----------------------------------------------------------------------------
struct BattleReport {
    int    aliveCount = 0;
    int    totalHp = 0;
    string weakestName;      // 血量最低的存活者，優先補血
    int    weakestHp = 0;
};

BattleReport summarizeBattle(const vector<GameCharacter>& party) {
    BattleReport report;
    bool foundAny = false;

    for (const GameCharacter& c : party) {     // const 參考：不複製、也不可修改
        if (!c.isAlive()) continue;            // 只統計存活者
        ++report.aliveCount;
        report.totalHp += c.getHp();
        if (!foundAny || c.getHp() < report.weakestHp) {
            report.weakestHp   = c.getHp();
            report.weakestName = c.getName();
            foundAny = true;
        }
    }
    return report;
}

int main() {
    // 用 struct 定義角色屬性（純資料）
    CharacterStats warriorStats;
    warriorStats.maxHp = 150;
    warriorStats.maxMp = 30;
    warriorStats.attack = 20;
    warriorStats.defense = 12;
    warriorStats.speed = 0.8;

    CharacterStats mageStats;
    mageStats.maxHp = 80;
    mageStats.maxMp = 120;
    mageStats.attack = 8;
    mageStats.defense = 3;
    mageStats.speed = 1.2;

    // 用 class 建立角色（有行為和保護）
    GameCharacter warrior;
    warrior.init("戰士", warriorStats);

    GameCharacter mage;
    mage.init("法師", mageStats);

    // 遊戲模擬
    cout << "===== 初始狀態 =====" << endl;
    warrior.showStatus();
    cout << endl;
    mage.showStatus();

    cout << "\n===== 戰鬥 =====" << endl;
    Vector2D target;
    target.x = 5.0;
    target.y = 3.0;
    warrior.moveTo(target);

    mage.useSkill(40);           // 法師放技能
    warrior.takeDamage(35);      // 戰士受到攻擊
    warrior.takeDamage(35);      // 再次受到攻擊

    mage.useSkill(40);           // 法師再放技能
    mage.useSkill(40);           // 第三次
    mage.useSkill(40);           // MP 不足？

    cout << "\n===== 最終狀態 =====" << endl;
    warrior.showStatus();
    cout << endl;
    mage.showStatus();

    // ─────────────────────────────────────────────────────────
    cout << "\n===== LeetCode 1266. Minimum Time Visiting All Points =====" << endl;
    // 官方範例 1：[[1,1],[3,4],[-1,0]] → 7
    vector<Vector2D> path1 = {{1, 1}, {3, 4}, {-1, 0}};
    cout << "  [[1,1],[3,4],[-1,0]] -> " << minTimeToVisitAllPoints(path1)
         << " 秒（預期 7）" << endl;
    // 官方範例 2：[[3,2],[-2,2]] → 5
    vector<Vector2D> path2 = {{3, 2}, {-2, 2}};
    cout << "  [[3,2],[-2,2]]       -> " << minTimeToVisitAllPoints(path2)
         << " 秒（預期 5）" << endl;

    // ─────────────────────────────────────────────────────────
    cout << "\n===== 實務：戰鬥結算報表 =====" << endl;
    vector<GameCharacter> party;
    party.push_back(warrior);
    party.push_back(mage);

    GameCharacter rogue;
    CharacterStats rogueStats;
    rogueStats.maxHp = 90;
    rogueStats.defense = 5;
    rogue.init("盜賊", rogueStats);
    rogue.takeDamage(200);          // 盜賊陣亡，驗證存活統計會排除他
    party.push_back(rogue);

    BattleReport rep = summarizeBattle(party);
    cout << "  存活人數: " << rep.aliveCount << " / " << party.size() << endl;
    cout << "  存活者總 HP: " << rep.totalHp << endl;
    cout << "  優先補血對象: " << rep.weakestName
         << "（HP " << rep.weakestHp << "）" << endl;

    // ─────────────────────────────────────────────────────────
    cout << "\n===== 封裝驗證 =====" << endl;
    // warrior.currentHp = 9999;   // ❌ 編譯錯誤：private，非法狀態無法被表達
    const GameCharacter& cref = warrior;
    cout << "  透過 const 參考唯讀查詢: " << cref.getName()
         << " HP=" << cref.getHp()
         << " 存活=" << (cref.isAlive() ? "是" : "否") << endl;
    // cref.takeDamage(1);         // ❌ 編譯錯誤：非 const 成員函式

    cout << "\n===== 記憶體佈局（實作定義，本機 GCC 15.2 x86-64）=====" << endl;
    cout << "  sizeof(Vector2D)       = " << sizeof(Vector2D) << endl;
    cout << "  sizeof(CharacterStats) = " << sizeof(CharacterStats) << endl;
    cout << "  sizeof(GameCharacter)  = " << sizeof(GameCharacter)
         << "（含內嵌的兩個 struct，非指標）" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 12 課：struct 與 class 的差異7.cpp" -o demo7

// === 預期輸出 ===
// ===== 初始狀態 =====
// --- 戰士 ---
// HP: 150/150
// MP: 30/30
// 位置: (0, 0)
// 攻擊: 20 / 防禦: 12
//
// --- 法師 ---
// HP: 80/80
// MP: 120/120
// 位置: (0, 0)
// 攻擊: 8 / 防禦: 3
//
// ===== 戰鬥 =====
// 戰士 移動到 (0.685994, 0.411597)
// 法師 使用技能（MP: 80/120）
// 戰士 受到 23 點傷害（HP: 127/150）
// 戰士 受到 23 點傷害（HP: 104/150）
// 法師 使用技能（MP: 40/120）
// 法師 使用技能（MP: 0/120）
// 法師: MP 不足！
//
// ===== 最終狀態 =====
// --- 戰士 ---
// HP: 104/150
// MP: 30/30
// 位置: (0.685994, 0.411597)
// 攻擊: 20 / 防禦: 12
//
// --- 法師 ---
// HP: 80/80
// MP: 0/120
// 位置: (0, 0)
// 攻擊: 8 / 防禦: 3
//
// ===== LeetCode 1266. Minimum Time Visiting All Points =====
//   [[1,1],[3,4],[-1,0]] -> 7 秒（預期 7）
//   [[3,2],[-2,2]]       -> 5 秒（預期 5）
//
// ===== 實務：戰鬥結算報表 =====
// 盜賊 受到 195 點傷害（HP: 0/90）
//   存活人數: 2 / 3
//   存活者總 HP: 184
//   優先補血對象: 法師（HP 80）
//
// ===== 封裝驗證 =====
//   透過 const 參考唯讀查詢: 戰士 HP=104 存活=是
//
// ===== 記憶體佈局（實作定義，本機 GCC 15.2 x86-64）=====
//   sizeof(Vector2D)       = 16
//   sizeof(CharacterStats) = 24
//   sizeof(GameCharacter)  = 80（含內嵌的兩個 struct，非指標）
