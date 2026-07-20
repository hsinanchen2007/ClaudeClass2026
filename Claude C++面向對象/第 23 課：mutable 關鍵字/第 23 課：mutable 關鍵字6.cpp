// =============================================================================
//  第 23 課：mutable 關鍵字 6  —  講義全文 + mutable vs const_cast 的關鍵差異
// =============================================================================
//
// 【主題資訊 Information】
//   本檔結構： 開頭是一大段 /* ... */ 的課程講義全文(23.1 ~ 23.11),
//             講義裡的程式碼片段是「文字」不會被編譯;
//             真正被編譯執行的只有講義結束後的 Counter 類別與 main()。
//   語法    ： mutable <型別> <成員名>;
//             const_cast<T*>(expr) / const_cast<T&>(expr)
//   標準版本： mutable 為 C++98;const_cast 亦為 C++98
//   標頭檔  ： 無(皆為語言關鍵字)
//
// 【詳細解釋 Explanation】
//
// 【1. 本檔真正要對比的兩條路】
//   同樣是「在 const 成員函式裡修改成員」,C++ 有兩種寫法,而它們的合法性天差地遠:
//     (a) mutable      —— 標準明文允許。編譯器知道這個成員可能被改,
//                          因此不會把物件放進唯讀記憶體。永遠安全。
//     (b) const_cast   —— 把 const 丟掉再寫入。是否安全 取決於物件本身是不是 const:
//                          * 物件本體非 const,只是透過 const 參考拿到 → 合法。
//                          * 物件本體就宣告為 const → 寫入是 undefined behavior。
//   本檔 main() 裡的 const Counter c; 屬於後者,所以 c.incrementBad() 是 UB。
//
// 【2. 為什麼「物件本體是不是 const」是分水嶺】
//   關鍵在於編譯器被允許做什麼假設:
//     * 對於宣告為 const 且沒有 mutable 成員的物件,編譯器可以假設它永不改變。
//       它因此可以把物件放進 .rodata(唯讀區段)、可以把讀取結果快取在暫存器、
//       可以把整個運算式在編譯期算完。
//     * 一旦你用 const_cast 偷偷寫入,這些假設就被違反了。
//       標準不保證任何後果——可能寫入成功、可能被最佳化掉、可能觸發記憶體保護。
//   反之,如果原始物件是非 const,只是某處用 const 參考觀察它,
//   那編譯器不能做上述假設,const_cast 回去寫入就是完全合法的
//   (這也是 const_cast 存在的正當理由,例如包裝舊的 C API)。
//
// 【3. 本檔的 UB 究竟會發生什麼】
//   實測(本機 g++ 15.2、無最佳化)輸出 count = 1 與 2,看起來「正常運作」。
//   但這 不是 保證,只是這次的觀察結果:
//     * const Counter c; 是自動儲存期(在 stack 上)的區域物件,
//       不會被放進唯讀區段,所以沒有觸發記憶體保護。
//     * 換成 static const Counter c; 或全域 const 物件,就可能被放進 .rodata,
//       屆時寫入的行為由平台決定。
//     * 提高最佳化等級後,編譯器也可能因為「c 是 const」而把讀取結果
//       常數摺疊,使輸出與無最佳化時不同。
//   結論:能跑不代表對。UB 最惡劣的性質就是「它常常看起來能用」。
//
// 【4. 正確的做法只有一個】
//   把 count_ 宣告成 mutable,increment() 就能名正言順地是 const 成員函式,
//   不需要任何轉型,也沒有任何 UB 疑慮。本檔在 main() 中加了 GoodCounter
//   作為對照組,兩者輸出並列,差別一目了然。
//
// 【概念補充 Concept Deep Dive】
//   (A) const_cast<Counter*>(this) 這個寫法的細節。
//       在 const 成員函式內 this 的型別是 const Counter*。
//       const_cast 把它變回 Counter*,於是 ->count_++ 得以通過編譯期檢查。
//       請注意:const_cast 只影響「編譯期的型別檢查」,它不會產生任何機器碼,
//       也不會改變物件實際存放的位置。所以它擋不住「物件真的在唯讀記憶體」的後果。
//
//   (B) 四種轉型的分工。
//       static_cast      —— 有關聯型別之間的轉換(數值、繼承體系向上/向下)
//       const_cast       —— 唯一能增減 const / volatile 的轉型
//       reinterpret_cast —— 位元層級重新解讀,幾乎都是 UB 邊緣
//       dynamic_cast     —— 多型型別的執行期安全向下轉型(需要 RTTI)
//       C 風格的 (T)x 會依序嘗試上述組合,正因為看不出意圖才不建議使用。
//
//   (C) const_cast 真正的正當用途。
//       最常見的是包裝只接受非 const 指標的舊 C API,而你確知底層物件非 const:
//           void legacy_api(char* buf);           // 其實不會修改 buf
//           void wrapper(std::string& s) {
//               legacy_api(&s[0]);                 // 現代寫法根本不需要轉型
//           }
//       另一個是 Scott Meyers 的「以非 const 版本呼叫 const 版本」慣用法,
//       用來避免兩個多載函式的實作重複。
//
//   (D) 為什麼標準不乾脆禁止對 const 物件做 const_cast 寫入。
//       因為編譯器在轉型當下無法知道原始物件的來源——
//       指標可能來自任何地方。標準的做法是:允許你寫,但把後果定為 UB,
//       責任歸於程式設計者。這是 C++「信任程式設計者」哲學的典型體現。
//
// 【注意事項 Pay Attention】
//   1. 本檔 main() 的 c.incrementBad() 是 undefined behavior。
//      實測輸出 1、2 只是本機這次的觀察結果,換編譯器、換最佳化等級、
//      把物件改成 static 或全域,結果都可能不同。請勿把它當成規格。
//      誠實補充:本機以 -O0 與 -O2 各編一次,兩者輸出「恰好相同」。
//      這不代表這樣寫是安全的——UB 的定義是標準不規範其行為,
//      而不是「一定會出錯」。實測沒踩到,只表示這次沒踩到。
//   2. 需要在 const 成員函式裡修改成員時,正解永遠是 mutable,不是 const_cast。
//   3. const_cast 只在「原始物件確定非 const」時才安全。
//   4. 講義區塊裡的程式碼不會被編譯,其中的「預期輸出」是講義說明,
//      不是本檔實際執行結果。本檔實際輸出見檔尾。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】mutable vs const_cast
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. mutable 和 const_cast 都能在 const 成員函式裡改成員,差在哪?
//     答：mutable 是標準允許的機制,編譯器知情,因此不會對該物件做
//         「永不改變」的最佳化假設,永遠安全。
//         const_cast 只是騙過編譯期的型別檢查,不改變物件的實際性質:
//         若原始物件本體宣告為 const,寫入就是 UB。
//     追問：那什麼時候 const_cast 是安全的?
//         → 當原始物件本體是非 const,只是你手上這條路徑帶了 const 的時候。
//         例如包裝一個只吃非 const 指標、但實際不寫入的舊 C API。
//
// 🔥 Q2. 這段程式跑起來輸出 1、2 完全正常,為什麼還說它是 UB?
//     答：UB 的定義是「標準不規範其行為」,不是「一定會崩潰」。
//         這裡的物件在 stack 上、又沒開最佳化,所以恰好寫得進去。
//         換成 static const 或全域 const 物件就可能落在唯讀區段,
//         開最佳化也可能因為常數摺疊而讀到舊值。
//         「這次能跑」不能證明「這樣寫沒問題」。
//     追問：那要怎麼在 CI 抓到這種問題?
//         → UBSan 對這類 const 違規的覆蓋有限,
//         比較可靠的做法是 code review 全面禁止 const_cast 寫入,
//         或以靜態分析工具(clang-tidy 的 cppcoreguidelines-pro-type-const-cast)攔截。
//
// ⚠️ 陷阱. 「const_cast 之後就變成一般物件了,想怎麼改都行。」
//     答：錯。const_cast 不產生任何機器碼,它只是把編譯器的型別檢查關掉。
//         物件實際存放在哪(stack、heap、還是唯讀區段)完全沒有改變。
//         如果它本來就在唯讀記憶體,轉型之後照樣寫不進去。
//     為什麼會錯：把轉型想像成「把東西搬到另一個型別的容器裡」。
//         實際上 const_cast 是純編譯期的標註調整,執行期什麼都沒發生——
//         這正是它危險的原因:它讓你以為問題解決了,其實只是把警報關掉。
// ═══════════════════════════════════════════════════════════════════════════

/*
# 第 23 課：mutable 關鍵字

---

## 23.1 問題的起源：const 函數中的合理修改需求

上一課我們學到：`const` 成員函數承諾不修改任何成員變數。但有些情況下，一個**邏輯上是只讀**的操作，在**實現上卻需要修改某些內部數據**：

```
場景 1：快取（Cache）
  → getResult() 第一次計算結果，之後直接返回快取
  → 邏輯上是「讀取」，但需要修改快取變數

場景 2：訪問計數
  → getInfo() 每次被調用時記錄訪問次數
  → 邏輯上是「讀取」，但需要修改計數器

場景 3：互斥鎖（Mutex）
  → getData() 需要加鎖保護線程安全
  → 邏輯上是「讀取」，但需要修改鎖的狀態

場景 4：延遲初始化（Lazy Initialization）
  → getDescription() 第一次被調用時才生成描述文字
  → 邏輯上是「讀取」，但需要修改內部狀態
```

如果沒有 `mutable`，你就被迫二選一：去掉 `const`（破壞介面承諾），或者用 `const_cast` 強制轉型（危險且醜陋）。`mutable` 提供了乾淨的解決方案。

---

## 23.2 mutable 的語法與基本用法

```
語法：
  mutable 數據類型 變數名;

含義：
  即使在 const 成員函數中，也允許修改這個變數
```

```cpp
#include <iostream>
#include <string>
using namespace std;

class Monster {
private:
    string name_;
    int hp_;
    int attack_;
    mutable int inspectCount_;   // mutable：const 函數也能改它

public:
    Monster(const string& name, int hp, int atk)
        : name_(name), hp_(hp), attack_(atk), inspectCount_(0)
    {
    }

    // const 函數——邏輯上只讀
    void printInfo() const {
        inspectCount_++;   // ✅ 可以修改！因為是 mutable
        cout << "  " << name_ << " [HP:" << hp_ << " ATK:" << attack_
             << "] (被查看了 " << inspectCount_ << " 次)" << endl;
    }

    const string& getName() const { return name_; }
    int getHp() const { return hp_; }

    // 查看被檢查了幾次
    int getInspectCount() const { return inspectCount_; }

    // 非 const：實際修改怪物狀態
    void takeDamage(int dmg) {
        hp_ = max(0, hp_ - dmg);
    }
};

int main() {
    cout << "=== mutable 基本用法 ===" << endl;

    const Monster dragon("火龍", 500, 60);  // const 對象！

    // 可以調用 const 函數
    dragon.printInfo();
    dragon.printInfo();
    dragon.printInfo();

    cout << "  總共被查看：" << dragon.getInspectCount() << " 次" << endl;

    // dragon.takeDamage(10);  // ❌ 編譯錯誤！const 對象

    return 0;
}
```

### 預期輸出

```
=== mutable 基本用法 ===
  火龍 [HP:500 ATK:60] (被查看了 1 次)
  火龍 [HP:500 ATK:60] (被查看了 2 次)
  火龍 [HP:500 ATK:60] (被查看了 3 次)
  總共被查看：3 次
```

注意：`dragon` 是 `const` 對象，但 `inspectCount_` 仍然被成功修改了——因為它是 `mutable`。

---

## 23.3 經典應用一：計算快取（Cache）

這是 `mutable` 最常見的用途。當某個計算很昂貴，你希望只算一次然後快取結果：

```cpp
#include <iostream>
#include <string>
using namespace std;

class Circle {
private:
    double radius_;

    // 快取相關——mutable 因為它們只是效能優化，不影響邏輯狀態
    mutable bool areaCached_;
    mutable double cachedArea_;
    mutable bool circumCached_;
    mutable double cachedCircum_;

    static constexpr double PI = 3.14159265358979;

public:
    Circle(double r)
        : radius_(r > 0 ? r : 1.0)
        , areaCached_(false), cachedArea_(0)
        , circumCached_(false), cachedCircum_(0)
    {
    }

    // getter：邏輯上只讀，但會更新快取
    double getArea() const {
        if (!areaCached_) {
            cout << "    [計算面積...]" << endl;
            cachedArea_ = PI * radius_ * radius_;   // ✅ mutable 可修改
            areaCached_ = true;                      // ✅ mutable 可修改
        } else {
            cout << "    [使用快取]" << endl;
        }
        return cachedArea_;
    }

    double getCircumference() const {
        if (!circumCached_) {
            cout << "    [計算周長...]" << endl;
            cachedCircum_ = 2 * PI * radius_;
            circumCached_ = true;
        } else {
            cout << "    [使用快取]" << endl;
        }
        return cachedCircum_;
    }

    // setter：修改半徑時，快取失效
    void setRadius(double r) {
        if (r <= 0) return;
        radius_ = r;
        areaCached_ = false;     // 清除快取
        circumCached_ = false;   // 清除快取
        cout << "  半徑改為 " << radius_ << "，快取已清除" << endl;
    }

    double getRadius() const { return radius_; }
};

int main() {
    cout << "=== 快取範例 ===" << endl;

    Circle c(5.0);

    // 第一次調用——觸發計算
    cout << "\n--- 第一次查詢 ---" << endl;
    cout << "  面積 = " << c.getArea() << endl;
    cout << "  周長 = " << c.getCircumference() << endl;

    // 第二次調用——使用快取
    cout << "\n--- 第二次查詢（快取命中）---" << endl;
    cout << "  面積 = " << c.getArea() << endl;
    cout << "  周長 = " << c.getCircumference() << endl;

    // 修改半徑——快取失效
    cout << "\n--- 修改半徑 ---" << endl;
    c.setRadius(10.0);

    // 再次查詢——重新計算
    cout << "\n--- 修改後查詢 ---" << endl;
    cout << "  面積 = " << c.getArea() << endl;
    cout << "  周長 = " << c.getCircumference() << endl;

    // 再次查詢——又是快取
    cout << "\n--- 再次查詢（快取命中）---" << endl;
    cout << "  面積 = " << c.getArea() << endl;

    return 0;
}
```

### 預期輸出

```
=== 快取範例 ===

--- 第一次查詢 ---
    [計算面積...]
  面積 = 78.5398
    [計算周長...]
  周長 = 31.4159

--- 第二次查詢（快取命中）---
    [使用快取]
  面積 = 78.5398
    [使用快取]
  周長 = 31.4159

--- 修改半徑 ---
  半徑改為 10，快取已清除

--- 修改後查詢 ---
    [計算面積...]
  面積 = 314.159
    [計算周長...]
  周長 = 62.8319

--- 再次查詢（快取命中）---
    [使用快取]
  面積 = 314.159
```

---

## 23.4 經典應用二：延遲初始化（Lazy Initialization）

有些數據的初始化成本很高，你希望在**真正需要時才初始化**：

```cpp
#include <iostream>
#include <string>
using namespace std;

class QuestLog {
private:
    string questName_;
    int difficulty_;

    // 延遲初始化：詳細描述只在需要時才生成
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

    return 0;
}
```

### 預期輸出

```
=== 延遲初始化 ===

--- 登記任務 ---
  [登記任務] 討伐火龍
  [登記任務] 採集草藥
  [登記任務] 護送商隊

--- 簡單查詢（不觸發生成）---
  任務1：討伐火龍（難度 5）
  任務2：採集草藥（難度 1）

--- 查看詳細描述（觸發生成）---
    [生成詳細描述... 耗時操作]
【討伐火龍】
    難度：★★★★★
    這是一個極其危險的任務。冒險者需做好充分準備。

--- 再次查看（已生成）---
【討伐火龍】
    難度：★★★★★
    這是一個極其危險的任務。冒險者需做好充分準備。

--- quest2 和 quest3 從未生成描述，節省了資源 ---
```

---

## 23.5 mutable 的判斷準則

`mutable` 不能隨便用。這是判斷是否適合使用 `mutable` 的準則：

```
適合用 mutable 的情況：
  ┌─────────────────────────────────────────────┐
  │ 修改的是「實現細節」，不是「邏輯狀態」         │
  │                                             │
  │ • 快取 / 備忘錄（Cache / Memoization）       │
  │ • 訪問計數器（Access Counter）               │
  │ • 延遲初始化（Lazy Initialization）          │
  │ • 互斥鎖（Mutex）— 多線程課程會詳細講         │
  │ • 調試/日誌用的輔助數據                      │
  └─────────────────────────────────────────────┘

不適合用 mutable 的情況：
  ┌─────────────────────────────────────────────┐
  │ 修改的是「對象的邏輯狀態」                     │
  │                                             │
  │ • HP、攻擊力、等級等核心數據                  │
  │ • 用戶名、帳戶餘額等業務數據                  │
  │ • 任何「外部可觀察到的變化」                   │
  └─────────────────────────────────────────────┘
```

核心原則：**從外部觀察者的角度，const 函數調用前後，對象「看起來」沒有變化。**

```
好的 mutable 使用：
  getArea() 調用前後：
    半徑沒變 ✅   面積結果一樣 ✅   只是內部多了快取

壞的 mutable 使用（反面教材）：
  mutable int hp_;
  void takeDamage(int d) const {   // 濫用！
      hp_ -= d;                     // 邏輯上改變了對象狀態
  }
```

---

## 23.6 反面教材：濫用 mutable

```cpp
#include <iostream>
#include <string>
using namespace std;

// ===== 濫用 mutable 的反面教材 =====
class BadExample {
private:
    string name_;
    mutable int hp_;        // ❌ 把核心數據設為 mutable
    mutable int gold_;      // ❌ 把核心數據設為 mutable

public:
    BadExample(const string& name, int hp, int gold)
        : name_(name), hp_(hp), gold_(gold) {}

    // 這些操作明明在修改對象狀態，卻偽裝成 const！
    void takeDamage(int dmg) const {   // ❌ 不應該是 const
        hp_ -= dmg;
        cout << "  " << name_ << " 受傷 HP:" << hp_ << endl;
    }

    void spendGold(int amount) const { // ❌ 不應該是 const
        gold_ -= amount;
        cout << "  花費 " << amount << " 金幣" << endl;
    }

    void print() const {
        cout << "  " << name_ << " HP:" << hp_
             << " Gold:" << gold_ << endl;
    }
};

// ===== 正確的設計 =====
class GoodExample {
private:
    string name_;
    int hp_;              // 核心數據：不用 mutable
    int gold_;            // 核心數據：不用 mutable
    mutable int viewCount_;  // ✅ 只有輔助數據用 mutable

public:
    GoodExample(const string& name, int hp, int gold)
        : name_(name), hp_(hp), gold_(gold), viewCount_(0) {}

    // 修改狀態的函數：不是 const
    void takeDamage(int dmg) {
        hp_ = max(0, hp_ - dmg);
        cout << "  " << name_ << " 受傷 HP:" << hp_ << endl;
    }

    void spendGold(int amount) {
        if (amount <= gold_) {
            gold_ -= amount;
            cout << "  花費 " << amount << " 金幣" << endl;
        }
    }

    // 只讀函數：是 const，只有 viewCount_ 用 mutable
    void print() const {
        viewCount_++;   // ✅ 合理的 mutable 用途
        cout << "  " << name_ << " HP:" << hp_
             << " Gold:" << gold_
             << " (查看次數:" << viewCount_ << ")" << endl;
    }
};

int main() {
    cout << "=== mutable 濫用 vs 正確使用 ===" << endl;

    // 濫用的後果：const 失去意義
    cout << "\n--- 濫用 mutable ---" << endl;
    const BadExample bad("壞設計", 100, 500);
    bad.takeDamage(30);     // const 對象居然能受傷？！
    bad.spendGold(200);     // const 對象居然能花錢？！
    bad.print();
    cout << "  const 完全失去了保護作用！" << endl;

    // 正確的設計
    cout << "\n--- 正確使用 ---" << endl;
    const GoodExample good("好設計", 100, 500);
    // good.takeDamage(30);   // ❌ 編譯錯誤！正確攔截
    // good.spendGold(200);   // ❌ 編譯錯誤！正確攔截
    good.print();             // ✅ 只有查看次數變化
    good.print();

    return 0;
}
```

### 預期輸出

```
=== mutable 濫用 vs 正確使用 ===

--- 濫用 mutable ---
  壞設計 受傷 HP:70
  花費 200 金幣
  壞設計 HP:70 Gold:300
  const 完全失去了保護作用！

--- 正確使用 ---
  好設計 HP:100 Gold:500 (查看次數:1)
  好設計 HP:100 Gold:500 (查看次數:2)
```

---

## 23.7 綜合範例：怪物圖鑑系統

```cpp
#include <iostream>
#include <string>
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

    return 0;
}
```

### 編譯與執行

```bash
g++ -std=c++17 -o lesson23 lesson23.cpp
./lesson23
```

### 預期輸出

```
============================================
   第 23 課：mutable 綜合範例
============================================

  ╔═══════════════════════════╗
  ║     怪 物 圖 鑑           ║
  ╚═══════════════════════════╝
  ★★★★★ 炎龍王 [火] (查看:1)
  ★★★ 冰霜狼 [水] (查看:1)
  ★★★★ 雷電鷹 [雷] (查看:1)
  ★ 泥土蟲 [土] (查看:1)

--- 查看炎龍王詳情 ---
    [生成詳細資料...]
=== 炎龍王 ===
  屬性：火
  稀有度：★★★★★
  基礎 HP：800
  基礎 ATK：70
  弱點：水
  威脅指數：20

--- 再次查看炎龍王詳情 ---
=== 炎龍王 ===
  屬性：火
  稀有度：★★★★★
  基礎 HP：800
  基礎 ATK：70
  弱點：水
  威脅指數：20

--- 查看雷電鷹詳情 ---
    [生成詳細資料...]
=== 雷電鷹 ===
  屬性：雷
  稀有度：★★★★
  基礎 HP：300
  基礎 ATK：60
  弱點：土
  威脅指數：13

--- 查看統計 ---
  炎龍王：被查看 4 次
  冰霜狼：被查看 1 次
  雷電鷹：被查看 3 次
  泥土蟲：被查看 1 次

  冰霜狼和泥土蟲的詳細描述從未生成，節省了資源！
```

---

## 23.8 mutable vs const_cast

你可能會想：「不用 `mutable`，用 `const_cast` 強制去掉 `const` 不行嗎？」

```cpp
#include <iostream>
using namespace std;

class Counter {
private:
    int count_;

public:
    Counter() : count_(0) {}

    // 方法 1：用 mutable（推薦）
    // mutable int count_;
    // void increment() const { count_++; }

    // 方法 2：用 const_cast（不推薦）
    void incrementBad() const {
        // 強制去掉 const——技術上可以編譯，但語義上是錯的
        const_cast<Counter*>(this)->count_++;
        cout << "  const_cast 方式：count = " << count_ << endl;
    }

    int getCount() const { return count_; }
};

int main() {
    cout << "=== mutable vs const_cast ===" << endl;

    const Counter c;     // const 對象
    c.incrementBad();    // 技術上能跑，但...
    c.incrementBad();

    // ⚠ 對 const 對象使用 const_cast 修改數據
    // 在標準中是「未定義行為」！
    // 編譯器可能把 const 對象放在只讀記憶體中，
    // 強制修改可能導致崩潰。

    return 0;
}
```

```
mutable vs const_cast 比較：

mutable：
  ✅ 在聲明時明確標記意圖
  ✅ 編譯器知道這個成員可以在 const 中修改
  ✅ 合法、安全、符合標準
  ✅ 其他程式員看到 mutable 就知道設計意圖

const_cast：
  ❌ 隱藏了修改意圖
  ❌ 對 const 對象使用是未定義行為
  ❌ 編譯器可能優化出問題
  ❌ 代碼維護者不知道為什麼要這樣做
```

**結論：需要在 const 函數中修改成員時，永遠使用 `mutable`，不要用 `const_cast`。**

---

## 23.9 mutable 的完整使用場景總結

```
✅ 適合 mutable 的場景          │ ❌ 不適合的場景
────────────────────────────────┼────────────────────────────
快取/備忘錄（避免重複計算）      │ HP、攻擊力等核心屬性
訪問計數器（統計用途）          │ 帳戶餘額、庫存等業務數據
延遲初始化（按需初始化）        │ 任何「外部可觀察到的變化」
互斥鎖（多線程安全）           │ 為了繞過 const 而濫用
調試/日誌輔助變數              │ 把所有成員都設為 mutable
```

判斷公式：

```
問：去掉這個 mutable 變數，從外部觀察者來看，
    對象的行為是否完全相同？

  → 是（只是少了快取/計數等內部機制）→ ✅ 適合 mutable
  → 否（對象的邏輯狀態真的不同了）    → ❌ 不適合 mutable
```

---

## 23.10 本課重點回顧

| 概念 | 說明 |
|------|------|
| mutable 語法 | `mutable int count_;` |
| mutable 含義 | 即使在 const 成員函數中也可以修改 |
| 適用場景 | 快取、計數器、延遲初始化、互斥鎖 |
| 核心原則 | 只用於「不影響邏輯狀態」的實現細節 |
| 判斷方法 | 去掉 mutable 變數，外部觀察是否一樣？ |
| vs const_cast | mutable 合法安全，const_cast 是未定義行為 |
| 濫用危險 | 把核心數據設為 mutable 會讓 const 完全失效 |
| 設計指導 | mutable 成員應該盡可能少 |

---

## 23.11 下一課預告

下一課是 **第 24 課：類別內的靜態成員變數**。我們將學習 `static` 成員——所有對象共享的數據。例如：記錄一個類別總共創建了多少個對象、全域配置參數、共用的常量表等。

準備好進入 **第 24 課：類別內的靜態成員變數** 了嗎？
*/

// ============================================================================
// ↑ 以上為講義全文（純註解，不會被編譯）
// ↓ 以下才是本檔實際編譯執行的程式碼
// ============================================================================

#include <iostream>
#include <string>
#include <map>
using namespace std;

class Counter {
private:
    int count_;

public:
    Counter() : count_(0) {}

    // 方法 1：用 mutable（推薦）
    // mutable int count_;
    // void increment() const { count_++; }
    // 這樣寫的好處是：語義清晰，編譯器知道這是特例，允許修改 mutable 成員。

    // 方法 2：用 const_cast（不推薦）
    // 這種方式強行去掉 const 限定，語義混亂，容易出錯。
    void incrementBad() const {
        // 強制去掉 const——技術上可以編譯，但語義上是錯的
        // 這裡的 const_cast 是危險的，因為它允許修改原本應該是不可變的數據。
        const_cast<Counter*>(this)->count_++;
        cout << "  const_cast 方式：count = " << count_ << endl;
    }

    int getCount() const { return count_; }
};

// ===== 對照組：正確的做法（mutable）=====
// 同樣是「在 const 成員函式裡累加計數」，但這個版本完全合法、沒有任何 UB。
// 差別只有兩處：count_ 加了 mutable、increment() 不需要任何轉型。
class GoodCounter {
private:
    mutable int count_;   // ✅ 標準允許在 const 成員函式中修改

public:
    GoodCounter() : count_(0) {}

    void increment() const {
        count_++;                      // 不需要 const_cast
        cout << "  mutable  方式：count = " << count_ << endl;
    }

    int getCount() const { return count_; }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔從缺,理由如下
//   本檔的主題是「const_cast 對 const 物件寫入為何是 UB」,
//   這是語言規則與編譯器最佳化假設的問題,不是演算法問題。
//   LeetCode 全部題目都以「輸出是否正確」評分,而 UB 的特徵恰恰是
//   「這次輸出剛好正確」——也就是說,一份含此種 UB 的解答可以全部 AC。
//   掛一題上來反而會強化「能 AC 就是對」的錯誤觀念,因此刻意從缺。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】const_cast 的正當用途：非 const 版本委派給 const 版本
//   情境：一個設定表類別要同時提供可讀寫與唯讀兩種 find。
//         兩個多載的查找邏輯完全一樣，若各寫一份，日後改了一邊忘了另一邊就會不一致。
//   慣用法（Scott Meyers, Effective C++ Item 3）：
//         讓「非 const 版本」呼叫「const 版本」，再把回傳值的 const 拿掉。
//   為什麼這裡的 const_cast 安全：
//         能呼叫到非 const 版本，代表 *this 本體就是非 const 物件；
//         我們只是把「自己剛剛臨時加上去的 const」再拿掉，
//         不是在對一個真正的 const 物件動手。這正是本檔開頭說的分水嶺。
//   方向很重要：只能「非 const 委派給 const」。反過來寫（const 版本呼叫非 const 版本）
//         就必須對一個可能真的是 const 的物件去 const，那才是危險的。
// -----------------------------------------------------------------------------
class SettingsTable {
private:
    map<string, string> kv_;

public:
    void set(const string& k, const string& v) { kv_[k] = v; }

    // const 版本：真正的實作放這裡
    const string* find(const string& key) const {
        auto it = kv_.find(key);
        return it == kv_.end() ? nullptr : &it->second;
    }

    // 非 const 版本：委派給 const 版本，避免重複實作
    string* find(const string& key) {
        // 1) static_cast 加上 const，藉此呼叫到上面的 const 版本
        // 2) const_cast 把回傳值的 const 拿掉
        // 安全性來自：能進到這個函式，就代表 *this 本體非 const
        return const_cast<string*>(static_cast<const SettingsTable&>(*this).find(key));
    }
};

int main() {
    cout << "=== mutable vs const_cast ===" << endl;

    cout << "\n--- ❌ const_cast 對 const 物件寫入（未定義行為）---" << endl;
    const Counter c;     // const 對象
    c.incrementBad();    // 技術上能跑，但這是 UB
    c.incrementBad();

    // ⚠ 對 const 對象使用 const_cast 修改數據
    // 在標準中是「未定義行為」！
    // 編譯器可能把 const 對象放在只讀記憶體中，
    // 強制修改可能導致崩潰。
    // 上面印出的 1、2 只是本機這次的觀察結果，不是標準保證的行為。

    cout << "\n--- ✅ mutable（標準允許，永遠安全）---" << endl;
    const GoodCounter g;   // 同樣是 const 對象
    g.increment();         // 完全合法：count_ 是 mutable
    g.increment();
    cout << "  最終 count = " << g.getCount() << endl;

    cout << "\n=== 日常實務: const_cast 的正當用途（非 const 委派給 const）===" << endl;
    SettingsTable cfg;
    cfg.set("log.level", "info");
    cfg.set("http.port", "8080");

    // 非 const 物件 -> 選到非 const 版本 -> 可以就地修改
    if (string* level = cfg.find("log.level")) {
        cout << "  修改前 log.level = " << *level << endl;
        *level = "debug";
        cout << "  修改後 log.level = " << *level << endl;
    }

    // const 參考 -> 只選得到 const 版本 -> 只能讀
    const SettingsTable& ro = cfg;
    if (const string* port = ro.find("http.port")) {
        cout << "  唯讀讀取 http.port = " << *port << endl;
    }
    if (ro.find("db.host") == nullptr) {
        cout << "  查詢不存在的鍵 db.host -> nullptr" << endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第\ 23\ 課：mutable\ 關鍵字6.cpp -o mutable6

// 【輸出說明 —— 重要】
//   1. 「const_cast 方式：count = 1 / 2」這兩行是 undefined behavior 的產物。
//      本機 g++ 15.2 以 -O0 與 -O2 各編譯執行一次，兩者輸出恰好相同，
//      但這不是標準保證，只是這次的觀察結果。把 const Counter c 改成
//      static const 或全域物件、或換一套編譯器，行為都可能不同。
//      請勿把這兩行當成「const_cast 可以這樣用」的證據。
//   2. 「mutable 方式：count = 1 / 2」則是標準保證的行為，任何符合標準的
//      編譯器都必須產生這個結果。
//   3. 實務段的 find() 多載示範：非 const 物件選到非 const 版本（可改），
//      const 參考只選得到 const 版本（唯讀）。查不到時回傳 nullptr。
//   4. 除了第 1 點屬於 UB 之外，本檔其餘輸出均為決定性。

// === 預期輸出 ===
// === mutable vs const_cast ===
//
// --- ❌ const_cast 對 const 物件寫入（未定義行為）---
//   const_cast 方式：count = 1
//   const_cast 方式：count = 2
//
// --- ✅ mutable（標準允許，永遠安全）---
//   mutable  方式：count = 1
//   mutable  方式：count = 2
//   最終 count = 2
//
// === 日常實務: const_cast 的正當用途（非 const 委派給 const）===
//   修改前 log.level = info
//   修改後 log.level = debug
//   唯讀讀取 http.port = 8080
//   查詢不存在的鍵 db.host -> nullptr
