/*
 * ============================================================================
 *  第 20～26 課：封裝專題 —— 合併精華總結
 * ============================================================================
 *
 *  本檔案合併了第 20～26 課的所有核心概念與精華範例，
 *  範例簡單扼要，但註釋完整覆蓋所有重點。
 *  全部放在一個 main() 下，可直接編譯執行。
 *  編譯：g++ -std=c++17 summary20-26.cpp -o summary20-26
 *
 *  目錄：
 *    第 20 課：封裝（Encapsulation）的意義
 *    第 21 課：getter 與 setter 設計模式
 *    第 22 課：const 成員函數
 *    第 23 課：mutable 關鍵字
 *    第 24 課：類別內的靜態成員變數
 *    第 25 課：類別內的靜態成員函數
 *    第 26 課：this 指標
 *
 * ============================================================================
 */

// =============================================================================
//
// 【主題資訊 Information】
//   範圍：  封裝的目的（20）→ 存取介面設計（21）→ const 正確性（22）
//           → mutable 的例外（23）→ static 成員（24、25）→ this（26）
//   標準：  本檔以 C++17 為基準。涉及版本的關鍵點：
//             * inline static 資料成員 —— C++17（在此之前必須類外定義）
//             * constexpr static 資料成員隱含 inline —— C++17
//             * 區域 static 的執行緒安全初始化（magic static）—— C++11
//   標頭檔：<iostream> <string> <vector> <algorithm> <cmath>
//   關鍵詞：encapsulation、invariant、const correctness、bitwise vs logical
//           const、mutable、static storage duration、this、method chaining
//
// 【詳細解釋 Explanation】
//
// 【1. 這七課其實在講同一件事：誰有權改變狀態，以及何時】
// 表面上是七個獨立主題，實際上是一條主線：
//     第 20 課    把狀態關起來             → 決定「誰」能改
//     第 21 課    開放受控的存取介面       → 決定「怎麼」改
//     第 22 課    const 標記不改狀態的操作 → 由編譯器驗證「這個不會改」
//     第 23 課    mutable 開一個受控例外   → 「邏輯上沒改，實體上改了」
//     第 24/25 課 static 屬於類別而非物件  → 狀態的「歸屬」不是物件
//     第 26 課    this 指向被操作的那個物件 → 前面一切的執行期基礎
// 把這條線看懂，比背七組語法有用得多。
//
// 【2. const 成員函式的真正語意：bitwise const，不是 logical const】
// 這是本專題最容易誤解的一點。`void f() const` 對編譯器的意義是：
//     在這個函式裡，this 的型別是 const T* const，
//     因此不能對「非 mutable 的非靜態資料成員」賦值。
// 它檢查的是**位元層面**（bitwise const），而不是「這個物件在語意上沒變」
//（logical const）。兩者的落差造成兩個方向的問題：
//   (a) bitwise const 成立，但 logical 不 const：
//       成員是 T* 時，const 保護的是「指標本身不能改指向」，
//       **不保護它指向的內容**。所以 const 函式裡照樣能寫 *ptr = 999;
//       本檔的 ShallowConst22::sneakyModify() 正是這個示範
//       —— 這是 const 最大的漏洞。
//   (b) logical const 成立，但 bitwise 不 const：
//       快取、惰性求值、計數器、mutex 這些「改了但外界觀察不到」的欄位，
//       在 const 函式裡改會被編譯器擋下。這正是 mutable 存在的理由，
//       本檔的 Circle23 用 mutable 同時做到快取與存取計數。
// 判準（第 23 課的核心）：把該欄位拿掉，外部觀察到的行為是否完全相同？
//   是 → 適合 mutable（快取、統計、鎖）
//   否 → 不適合（那是真正的狀態，如 BadMutable23 的 hp_，屬於濫用）
//
// 【3. 為什麼「回傳非 const 引用的 getter」等於沒有封裝】
// 第 21 課點到但值得講透。比較三種 getter：
//     int         hp() const   { return hp_; }     // 安全：回傳副本
//     const Inv&  inv() const  { return inv_; }    // 安全：唯讀視圖
//     Inv&        inv()        { return inv_; }    // 危險：可寫通道
// 第三種讓呼叫端拿到可寫的 reference，於是能繞過所有驗證與副作用：
//     obj.inv().clear();      // 沒有經過任何一行你寫的檢查程式碼
// 表面上「有 getter、成員是 private」，實質上與 public 無異，而且更隱蔽
// —— 因為程式碼審查時看起來像是有封裝的。
// 檢查自己的類別時，該問的不是「成員有沒有標 private」，
// 而是「所有 public 函式的回傳型別裡，有沒有非 const 的 reference／指標指向內部」。
//
// 【4. static 成員的兩個層面：儲存期與存取權是正交的】
// static 資料成員的性質常被混為一談，實際上是兩件獨立的事：
//   * 儲存期（storage duration）：static 成員存在靜態儲存區，
//     生命週期是整個程式，**不隨物件建立或銷毀**，也不計入 sizeof。
//   * 存取權（access）：它照樣受 public/private/protected 管轄。
//     private static 成員一樣只有類別自己與 friend 碰得到。
// 所以「static 就是全域變數」是錯的 —— 它有歸屬、有存取控制、有命名空間。
// static 成員函式沒有 this，因此
//   (a) 不能存取非靜態成員（沒有物件可指）
//   (b) 不能標 const（const 修飾的是 this 指向的物件，沒有 this 就無意義）
//   (c) 也不能是 virtual（虛擬派發需要物件裡的 vptr）
//   (d) 可以用 Class::func() 呼叫，不需要任何物件存在
//
// 【5. this 的型別會隨函式的 const 限定而改變】
// 這是把第 22 與第 26 課串起來的關鍵：
//     void f()               →  this 的型別是        T* const
//     void f() const         →  this 的型別是  const T* const
//     void f() volatile      →  volatile T* const
//     static void f()        →  沒有 this
// 注意 this 本身**永遠是 const 指標**（不能改指向別的物件），
// 差別只在它指向的東西是不是 const。
// 這也解釋了為什麼 const 函式只能呼叫 const 函式：
// 傳進去的 this 已經是 const T*，非 const 函式要求 T*，型別不相容。
//
// 【概念補充 Concept Deep Dive】
// (A) 成員初始化順序永遠依「宣告順序」，不是初始化列表的順序
//     本檔的 Buffer26 保留了這個教學點（並會產生 -Wreorder 警告）：
//         int* data_;      // 先宣告
//         int  size_;      // 後宣告
//         Buffer26(int size) : size_(size), data_(new int[size]) { ... }
//     初始化列表寫成 size_ 在前，但實際執行順序是 data_ 先、size_ 後。
//     這裡之所以安全，是因為 data_(new int[size]) 用的是**參數 size**，
//     而不是尚未初始化的成員 size_。若寫成 data_(new int[size_])，
//     就會從未初始化的成員讀值 —— 那是 UB，標準不保證任何結果。
//     ⚠️ 本檔保留 -Wreorder 警告是刻意的：讓你親眼看到編譯器會替你抓出
//        「初始化列表順序與宣告順序不一致」這個危險訊號。
//        實務上正確做法是讓兩者順序一致，警告自然消失。
//
// (B) magic static：區域 static 的初始化在 C++11 起保證執行緒安全
//     單例最常見的寫法
//         static T& instance() { static T inst; return inst; }
//     在 C++11 之前，多執行緒同時首次呼叫可能重複建構（著名的 DCLP 問題）。
//     C++11 起標準保證這個初始化是執行緒安全的（俗稱 magic static），
//     編譯器會自動加上一次性的同步。這是現代單例首選此寫法的主因。
//
// (C) static 資料成員在 C++17 之前必須「類外定義」
//     C++17 之前：類內只是宣告，還要在某個 .cpp 寫 int Foo::count_ = 0;
//                 漏寫就是連結錯誤（undefined reference）。
//     C++17 起：可寫 inline static int count_ = 0; 直接在類內完成定義，
//               header-only 函式庫因此方便許多。
//     另注意 constexpr static 資料成員自 C++17 起隱含 inline，不必再類外定義
//     —— 本檔 Circle23 的 static constexpr double PI 正是靠這條規則。
//
// (D) const 多載：同名函式可以只差 const 限定
//         const char& at(size_t i) const;   // const 物件呼叫這個
//         char&       at(size_t i);         // 非 const 物件呼叫這個
//     這是標準容器普遍採用的模式，讓唯讀路徑回傳唯讀視圖、
//     可寫路徑回傳可寫視圖，兩者共用同一個名字。
//
// (E) 鏈式呼叫（method chaining）與回傳型別的選擇
//     `return *this;` 有兩種回傳型別，語意完全不同：
//         T&  chain() { ...; return *this; }   // 回傳引用：不複製，可連續呼叫
//         T   chain() { ...; return *this; }   // 回傳值：每次複製，效率差且語意怪
//     Builder 模式一律用前者。本檔的 QueryBuilder26 即為此模式。
//
// 【注意事項 Pay Attention】
// 1. const 是 bitwise 不是 logical：成員為指標時，const 函式仍可改它指向的內容。
// 2. mutable 只該用於「外界觀察不到」的欄位（快取／統計／鎖），不可用於核心狀態。
// 3. 回傳非 const 引用的 getter 等於解除封裝，比 public 成員更隱蔽。
// 4. static 成員函式沒有 this，因此不能加 const、不能是 virtual、不能存取非靜態成員。
// 5. static 資料成員不計入 sizeof；C++17 前需類外定義，否則連結錯誤。
// 6. 成員初始化依「宣告順序」，與初始化列表的書寫順序無關 —— 讓兩者一致以免踩雷。
// 7. 建構函式中把 this 傳出去有風險：此時物件尚未完全初始化，虛擬函式也還沒就位。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】封裝、const、mutable、static、this
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. const 成員函式到底承諾了什麼？它保護得了指標成員指向的內容嗎？
//     答：它承諾「不修改本物件的非 mutable 非靜態資料成員」，
//         實作上是把 this 的型別變成 const T* const。
//         它是 bitwise const，**保護不了指標指向的內容** ——
//         成員為 int* 時，const 函式不能改 data_ 本身指向哪裡，
//         但可以自由地寫 *data_ = 999。這是 const 最常被誤解的漏洞。
//     追問：那要怎麼讓指向的內容也唯讀？
//         → 把成員型別改成 const int*（指向 const），
//           或改用 std::vector 這類值語意容器，const 就會自然傳遞下去。
//
// 🔥 Q2. mutable 什麼時候該用？什麼時候是濫用？
//     答：判準是「把這個欄位拿掉，外部觀察到的行為會不會改變」。
//         不會改變 → 適合（快取結果、呼叫次數統計、mutex、惰性初始化旗標）。
//         會改變   → 那是真正的狀態，用 mutable 等於讓 const 完全失效。
//         例：Circle23 快取算好的面積是合理的；
//             BadMutable23 用 mutable 存 hp_ 並提供 const 的 takeDamage()
//             就是濫用 —— 呼叫端看到 const 卻被改了核心狀態。
//     追問：mutable 和 const_cast 哪個好？
//         → mutable。對「原本就宣告為 const 的物件」用 const_cast 去 const
//           再修改是 UB；mutable 是語言正式支援的機制，安全且意圖明確。
//
// 🔥 Q3. static 成員函式為什麼不能加 const，也不能是 virtual？
//     答：const 修飾的是隱含的 this 所指向的物件；static 成員函式沒有 this
//         （它不屬於任何物件），所以 const 沒有東西可修飾。
//         virtual 需要透過物件裡的 vptr 做執行期派發，static 函式同樣沒有物件，
//         兩者都在語法層面直接禁止。
//     追問：static 成員函式能存取 private 成員嗎？
//         → 能。存取權以類別為單位，與有沒有 this 無關；
//           只是它必須透過參數拿到物件，例如 static bool cmp(const T& a, const T& b)。
//
// ⚠️ 陷阱 1. 這個建構函式為什麼可能配置出錯誤大小的記憶體？
//         class B { int* data_; int size_;
//                   B(int n) : size_(n), data_(new int[size_]) {} };
//     答：因為成員初始化順序**只依宣告順序**，與初始化列表的書寫順序無關。
//         data_ 先宣告，所以 data_ 先初始化 —— 此時 size_ 還沒被賦值，
//         new int[size_] 讀的是未初始化的值。這是 UB，
//         標準不保證任何結果（可能配置出巨大長度、可能拋 bad_alloc、
//         也可能「看起來正常」而在別處爆炸）。
//     為什麼會錯：多數人直覺認為「初始化列表由左至右執行」，
//         把它當成一般的敘述序列。但標準明確規定順序由宣告順序決定，
//         列表只是提供初始值。編譯器的 -Wreorder 警告正是為此而生 ——
//         看到它就該把兩者順序對齊，而不是忽略。
//         （本檔 Buffer26 刻意保留此警告作為教學示範，
//           但它用的是參數 size 而非成員 size_，所以實際行為是正確的。）
//
// ⚠️ 陷阱 2. 「這個類別成員都是 private，也有 getter，所以封裝做好了」——不一定，為什麼？
//     答：要看 getter 的回傳型別。若是 `Inventory& inv() { return inv_; }`
//         這種非 const 引用，呼叫端可以寫 obj.inv().clear()，
//         完全繞過你所有的驗證與副作用 —— 封裝在此處已被解除，
//         而且比直接寫 public 更難察覺，因為表面上「有 getter」。
//     為什麼會錯：把封裝當成一個語法特徵（成員有沒有標 private）來檢查，
//         而不是當成一個資訊流問題（有沒有可寫的通道洩漏到外部）。
//         正確的自我檢查是掃過所有 public 函式的回傳型別，
//         看有沒有非 const 的 reference 或指標指向內部狀態。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>   // std::min, std::max
#include <cmath>       // sqrt, abs
using namespace std;

// ============================================================================
//  第 20 課：封裝（Encapsulation）的意義
// ============================================================================
/*
 *  封裝 = OOP 四大核心支柱之一（封裝/繼承/多型/抽象）
 *
 *  核心思想：將數據和操作數據的函數綁定在一起，控制外部對數據的訪問
 *
 *  封裝的三層防護：
 *    第一層：訪問控制 → private/protected/public，編譯器強制執行
 *    第二層：數據驗證 → setter/操作函數中檢查數據合法性
 *    第三層：不變量維護 → 確保對象始終處於合法狀態
 *
 *  不變量（Invariant）：
 *    - 對象在任何時刻都必須滿足的條件
 *    - 例如：0 <= hp <= maxHp, maxHp > 0, attack > 0
 *    - 每個 public 函數：開始前成立 → 執行中可暫時違反 → 結束後必須恢復
 *
 *  封裝的四大好處：
 *    1. 數據保護：防止外部破壞對象合法狀態
 *    2. 介面穩定：內部實現可改，public 介面不變
 *    3. 降低耦合：使用者不依賴內部細節
 *    4. 簡化使用：只需知道「做什麼」不需知道「怎麼做」
 *
 *  封裝的四個層次：
 *    1. 語法層：private/public 訪問控制
 *    2. 邏輯層：setter 中的數據驗證
 *    3. 介面層：只暴露必要操作，隱藏內部實現
 *    4. 模組層：命名空間/頭文件組織
 *
 *  設計原則：暴露最少介面
 *    - 只公開必要的操作
 *    - 不提供 setHp/setLevel 等 setter（避免繞過驗證）
 *    - private 輔助函數隱藏實現細節
 */

// 反面教材：無封裝的 struct，外部可任意篡改
struct CharacterBad {
    string name;
    int hp, maxHp, attack;
    // 六大災難：hp 超過 maxHp、hp 負數、maxHp 為 0、
    //           攻擊力隨改、等級經驗不一致、金幣無限刷
};

// 正面教材：完整封裝的 Character
class Character20 {
private:
    string name_;
    int hp_, maxHp_, attack_, level_, exp_, gold_;

    // 私有輔助函數：外部不可見（介面層封裝）
    int expToNextLevel() const { return level_ * 100; }
    void checkLevelUp() {
        while (exp_ >= expToNextLevel()) {
            exp_ -= expToNextLevel();
            level_++;
            maxHp_ += 20; hp_ = maxHp_; attack_ += 5;
            cout << "  ★ " << name_ << " 升級 Lv." << level_ << endl;
        }
    }

public:
    // 建構函數：確保初始狀態合法（防禦性檢查）
    Character20(const string& name, int maxHp, int atk)
        : name_(name)
        , hp_(maxHp > 0 ? maxHp : 100)
        , maxHp_(maxHp > 0 ? maxHp : 100)
        , attack_(atk > 0 ? atk : 10)
        , level_(1), exp_(0), gold_(0) {}

    // 受控操作：數據驗證 + 不變量維護
    void takeDamage(int dmg) {
        if (dmg <= 0) { cout << "  無效傷害" << endl; return; }
        hp_ = max(0, hp_ - dmg);  // 不變量：hp >= 0
        cout << "  " << name_ << " -" << dmg << " HP:" << hp_ << "/" << maxHp_ << endl;
        if (hp_ == 0) cout << "  " << name_ << " 倒下了！" << endl;
    }
    void heal(int amount) {
        if (amount <= 0 || hp_ == 0) return;
        hp_ = min(hp_ + amount, maxHp_);  // 不變量：hp <= maxHp
    }
    void gainExp(int amount) {
        if (amount <= 0) return;
        exp_ += amount;
        checkLevelUp();  // 自動處理升級邏輯（簡化使用）
    }

    // 只讀查詢（const）
    bool isAlive() const { return hp_ > 0; }
    const string& getName() const { return name_; }
    void printStatus() const {
        cout << "  " << name_ << " Lv." << level_ << " HP:" << hp_
             << "/" << maxHp_ << " ATK:" << attack_ << endl;
    }
};

void demo_lesson20() {
    cout << "\n╔══════════════════════════════════════════╗" << endl;
    cout << "║  第 20 課：封裝（Encapsulation）         ║" << endl;
    cout << "╚══════════════════════════════════════════╝" << endl;

    Character20 hero("勇者", 100, 25);
    hero.printStatus();

    hero.takeDamage(30);
    hero.takeDamage(-100);  // 無效傷害，被攔截
    hero.heal(20);
    hero.gainExp(150);      // 自動升級
    hero.printStatus();

    hero.takeDamage(9999);  // HP 歸零但不會變負
    hero.heal(50);          // 已死亡無法治療
}

// ============================================================================
//  第 21 課：getter 與 setter 設計模式
// ============================================================================
/*
 *  getter / setter 不是機械地為每個成員都加上，而是經過設計考量的介面！
 *
 *  getter 返回方式選擇：
 *    ┌────────────────┬──────────────────────────────────┐
 *    │ 型別           │ 建議返回方式                      │
 *    ├────────────────┼──────────────────────────────────┤
 *    │ 基本型別       │ 返回值（int getHp() const）        │
 *    │ (int/double)   │ 拷貝成本極低                      │
 *    ├────────────────┼──────────────────────────────────┤
 *    │ 大型物件       │ 返回 const 引用                    │
 *    │ (string/vector)│ const string& getName() const     │
 *    └────────────────┴──────────────────────────────────┘
 *
 *  危險的 getter（反面教材）：
 *    int& getBalanceDangerous() { return balance_; }
 *    → 返回非 const 引用，外部可直接修改內部狀態，繞過所有驗證！
 *    → 教訓：返回非 const 引用 = 把鑰匙交給外人，封裝形同虛設
 *
 *  用「行為」取代 setter（核心原則）：
 *    不提供 setHp()，而是提供 takeDamage() / heal()
 *    好處：語義清晰、自動驗證、自動觸發副作用
 *
 *  判斷原則：
 *    是否需要 getter？→ 外部有讀取的合理需求嗎？
 *    是否需要 setter？→ 能用更有意義的「行為」取代嗎？
 *    三種層級：
 *      1. 有 getter 沒 setter（外部可讀不可改）
 *      2. 沒 getter 沒 setter（純內部邏輯）
 *      3. 行為取代 setter（takeDamage/enrage）
 *
 *  命名慣例：
 *    風格 1（Java）：getHp() / setHp(int)  ← 最清晰
 *    風格 2（C++）：hp() / hp(int)          ← 重載風格
 *    風格 3（STL）：size() / length()       ← 通常沒 setter
 *    原則：項目中保持一致
 *
 *  鏈式調用（Method Chaining）：
 *    setter 返回 ClassName&，並在最後 return *this;
 *    例如：dlg.setTitle("警告").setMessage("確定？").show();
 */

// 展示危險 getter 的後果
class BankAccount21 {
private:
    int balance_;
public:
    BankAccount21(int b) : balance_(b) {}

    // 安全 getter
    int getBalance() const { return balance_; }

    // 危險 getter（反面教材）——返回非 const 引用，破壞封裝！
    int& getBalanceDangerous() { return balance_; }

    // 正確的修改介面（帶驗證）
    bool deposit(int amount) {
        if (amount <= 0) return false;
        balance_ += amount;
        return true;
    }
};

// 展示「行為取代 setter」
class Enemy21 {
private:
    string name_;
    int hp_, maxHp_, attackPower_;
    int aiState_;  // 純內部邏輯，沒 getter 沒 setter

public:
    Enemy21(const string& name, int maxHp, int atk)
        : name_(name), hp_(maxHp), maxHp_(maxHp), attackPower_(atk), aiState_(0) {}

    // 有 getter，沒 setter
    const string& getName() const { return name_; }
    int getHp() const { return hp_; }

    // 行為取代 setter：不是 setHp()，而是 takeDamage()
    void takeDamage(int dmg) {
        if (dmg <= 0) return;
        hp_ = max(0, hp_ - dmg);
        cout << "  " << name_ << " -" << dmg << " HP:" << hp_ << endl;
        if (hp_ == 0) cout << "  " << name_ << " 被擊敗！" << endl;
    }
    // 行為取代 setter：不是 setAttackPower()，而是 enrage()
    void enrage() {
        attackPower_ *= 2;
        cout << "  " << name_ << " 暴怒！ATK=" << attackPower_ << endl;
    }
};

// 展示鏈式調用
class DialogBox21 {
private:
    string title_, message_;
    int width_, height_;
public:
    DialogBox21() : title_(""), message_(""), width_(200), height_(100) {}

    // 返回自身引用 → 支持鏈式調用
    DialogBox21& setTitle(const string& t) { title_ = t; return *this; }
    DialogBox21& setMessage(const string& m) { message_ = m; return *this; }
    DialogBox21& setSize(int w, int h) {
        width_ = w > 0 ? w : 200;
        height_ = h > 0 ? h : 100;
        return *this;
    }
    void print() const {
        cout << "  [" << title_ << "] " << message_
             << " (" << width_ << "x" << height_ << ")" << endl;
    }
};

void demo_lesson21() {
    cout << "\n╔══════════════════════════════════════════╗" << endl;
    cout << "║  第 21 課：getter 與 setter 設計模式     ║" << endl;
    cout << "╚══════════════════════════════════════════╝" << endl;

    // 危險 getter 示範
    cout << "\n--- 危險 getter ---" << endl;
    BankAccount21 acc(1000);
    acc.deposit(500);
    cout << "  正常餘額：" << acc.getBalance() << endl;
    acc.getBalanceDangerous() = 999999;  // 繞過驗證直接竄改！
    cout << "  竄改後：" << acc.getBalance() << endl;
    cout << "  教訓：返回非 const 引用破壞封裝！" << endl;

    // 行為取代 setter
    cout << "\n--- 行為取代 setter ---" << endl;
    Enemy21 goblin("哥布林", 50, 15);
    goblin.takeDamage(20);  // 不是 setHp(30)
    goblin.enrage();         // 不是 setAttackPower(30)

    // 鏈式調用
    cout << "\n--- 鏈式調用 ---" << endl;
    DialogBox21 dlg;
    dlg.setTitle("警告").setMessage("確定刪除嗎？").setSize(400, 200);
    dlg.print();
}

// ============================================================================
//  第 22 課：const 成員函數
// ============================================================================
/*
 *  const 成員函數：
 *    語法：返回類型 函數名(參數) const { ... }
 *    含義：承諾不修改任何成員變數，編譯器強制執行
 *
 *  核心規則：
 *    非 const 對象 → 可調用 const 和非 const 函數
 *    const 對象    → 只能調用 const 函數
 *
 *  const 正確性（非常重要的設計原則）：
 *    所有不修改對象狀態的成員函數都應標記 const
 *    忘記加 const → const 引用接收時連 getter 都不能調用！
 *
 *  this 指標與 const：
 *    普通函數：this 類型為 T* const（可讀寫成員）
 *    const 函數：this 類型為 const T* const（只能讀取）
 *
 *  const 重載（const overloading）：
 *    同一函數可有 const 和非 const 兩個版本
 *    const 版本返回 const 引用（只讀）
 *    非 const 版本返回非 const 引用（可讀寫）
 *    編譯器根據對象是否 const 自動選擇版本
 *
 *  const 函數調用鏈：
 *    const → const       可以
 *    const → 非 const    不行（編譯錯誤）
 *    非 const → const    可以
 *    非 const → 非 const 可以
 *    簡記：const 函數只能調用 const 函數
 *
 *  淺層 const 陷阱（重要限制）：
 *    const 只保護對象的直接成員，不保護指標指向的間接數據
 *    data_ 指標本身不能改，但 *data_ 指向的內容可以改！
 *    const 是「淺層的」（shallow const）
 *
 *  const 正確性檢查清單：
 *    1. 函數會修改成員嗎？否 → 加 const
 *    2. 調用的其他函數都是 const 嗎？
 *    3. 返回成員引用？const 函數必須返回 const 引用
 *    4. 有沒有通過指標間接修改？（const 不保護）
 */

class Potion22 {
private:
    string name_;
    int healAmount_;
    int quantity_;

public:
    Potion22(const string& name, int heal, int qty)
        : name_(name), healAmount_(heal), quantity_(qty) {}

    // const 成員函數：只讀，不修改對象
    const string& getName() const { return name_; }
    int getQuantity() const { return quantity_; }
    void printInfo() const {
        cout << "  " << name_ << " 回復:" << healAmount_
             << " 數量:" << quantity_ << endl;
    }

    // 非 const 成員函數：會修改 quantity_
    bool use() {
        if (quantity_ <= 0) return false;
        quantity_--;  // 修改了成員 → 不能是 const
        cout << "  使用 " << name_ << " 剩餘:" << quantity_ << endl;
        return true;
    }
};

// const 重載示範
class TextBuffer22 {
private:
    string content_;
public:
    TextBuffer22(const string& text) : content_(text) {}

    // const 版本：const 對象調用，返回 const 引用
    const string& getText() const {
        cout << "  [const 版本]";
        return content_;
    }
    // 非 const 版本：非 const 對象調用，返回可修改引用
    string& getText() {
        cout << "  [非 const 版本]";
        return content_;
    }
    void print() const { cout << "  內容：" << content_ << endl; }
};

// 淺層 const 陷阱
class ShallowConst22 {
private:
    int* data_;
public:
    ShallowConst22(int v) : data_(new int(v)) {}
    ~ShallowConst22() { delete data_; }
    ShallowConst22(const ShallowConst22&) = delete;
    ShallowConst22& operator=(const ShallowConst22&) = delete;

    // 危險！const 函數卻能修改指標指向的數據
    void sneakyModify() const {
        // data_ = nullptr;  // 錯誤：不能改指標本身
        *data_ = 999;         // 可以！const 不保護指標指向的內容
    }
    int getValue() const { return *data_; }
};

void demo_lesson22() {
    cout << "\n╔══════════════════════════════════════════╗" << endl;
    cout << "║  第 22 課：const 成員函數                ║" << endl;
    cout << "╚══════════════════════════════════════════╝" << endl;

    // const 對象限制
    cout << "\n--- const 對象限制 ---" << endl;
    const Potion22 constPotion("聖水", 100, 5);
    constPotion.printInfo();       // const 函數 → OK
    constPotion.getQuantity();     // const 函數 → OK
    // constPotion.use();          // 編譯錯誤！const 對象不能調用非 const 函數

    Potion22 normalPotion("藥水", 50, 3);
    normalPotion.use();            // 非 const 對象 → 都可以調用
    normalPotion.printInfo();      // const 函數也能調用

    // const 重載
    cout << "\n--- const 重載 ---" << endl;
    TextBuffer22 buf("Hello");
    buf.getText() = "Modified";    // 非 const → 非 const 版本，可修改
    buf.print();

    const TextBuffer22 constBuf("ReadOnly");
    constBuf.getText();            // const → const 版本
    // constBuf.getText() = "Hack"; // 編譯錯誤！const 版本返回 const 引用
    cout << endl;

    // 淺層 const
    cout << "\n--- 淺層 const 陷阱 ---" << endl;
    ShallowConst22 sc(42);
    cout << "  修改前：" << sc.getValue() << endl;
    sc.sneakyModify();  // const 函數卻修改了指標指向的數據！
    cout << "  修改後：" << sc.getValue() << " (const 是淺層的！)" << endl;
}

// ============================================================================
//  第 23 課：mutable 關鍵字
// ============================================================================
/*
 *  問題起源：
 *    有些操作「邏輯上是只讀」，但「實作上需要修改某些內部數據」
 *    例如：快取、訪問計數器、延遲初始化
 *    沒有 mutable 就得：(a) 去掉 const  (b) 用 const_cast（危險）
 *
 *  語法：mutable 數據類型 變數名;
 *  含義：即使在 const 成員函數中，也允許修改這個變數
 *
 *  三大經典應用：
 *    1. 訪問計數器：getInfo() 記錄被查看次數
 *    2. 計算快取：getArea() 第一次計算後快取，之後直接返回
 *    3. 延遲初始化：建構時不算，需要時才算
 *
 *  判斷準則：
 *    ✅ 適合 mutable：快取、計數器、延遲初始化、互斥鎖
 *    ❌ 不適合 mutable：HP、攻擊力、等級等核心數據
 *    公式：去掉 mutable 變數，外部觀察行為是否完全相同？
 *      是 → 適合（只是少了快取/計數等內部機制）
 *      否 → 不適合（對象邏輯狀態真的不同了）
 *
 *  濫用的後果：
 *    把 HP/金幣設為 mutable → const 完全失去保護作用
 *    偽裝 const 的函數實際修改核心狀態 → 錯誤設計
 *
 *  mutable vs const_cast：
 *    mutable（推薦）：聲明時明確標記意圖，合法安全
 *    const_cast（不推薦）：隱藏修改意圖，對 const 對象是未定義行為
 *    結論：永遠用 mutable，不要用 const_cast
 */

// 綜合示範：訪問計數器 + 快取
class Circle23 {
private:
    double radius_;
    mutable int accessCount_;        // 訪問計數器（mutable）
    mutable bool areaCached_;        // 快取旗標（mutable）
    mutable double cachedArea_;      // 快取結果（mutable）
    static constexpr double PI = 3.14159265358979;

public:
    Circle23(double r) : radius_(r > 0 ? r : 1), accessCount_(0),
                         areaCached_(false), cachedArea_(0) {}

    // const 函數，但可以修改 mutable 成員
    double getArea() const {
        accessCount_++;  // mutable → const 中可修改
        if (!areaCached_) {
            cout << "  [計算面積...]" << endl;
            cachedArea_ = PI * radius_ * radius_;
            areaCached_ = true;
        } else {
            cout << "  [使用快取]" << endl;
        }
        return cachedArea_;
    }

    // setter 修改核心數據時，清除快取
    void setRadius(double r) {
        if (r <= 0) return;
        radius_ = r;
        areaCached_ = false;  // 快取失效
    }

    int getAccessCount() const { return accessCount_; }
};

// 反面教材：濫用 mutable
class BadMutable23 {
private:
    mutable int hp_;    // 錯誤！核心數據不該用 mutable
public:
    BadMutable23(int hp) : hp_(hp) {}
    // 偽裝成 const 卻修改核心狀態
    void takeDamage(int d) const { hp_ -= d; }  // 不應該是 const！
    int getHp() const { return hp_; }
};

void demo_lesson23() {
    cout << "\n╔══════════════════════════════════════════╗" << endl;
    cout << "║  第 23 課：mutable 關鍵字               ║" << endl;
    cout << "╚══════════════════════════════════════════╝" << endl;

    // 快取 + 計數器
    cout << "\n--- 快取 + 訪問計數器 ---" << endl;
    const Circle23 c(5.0);  // const 對象！
    cout << "  面積=" << c.getArea() << endl;  // 第一次計算
    cout << "  面積=" << c.getArea() << endl;  // 使用快取
    cout << "  面積=" << c.getArea() << endl;  // 使用快取
    cout << "  被訪問 " << c.getAccessCount() << " 次" << endl;

    // 濫用示範
    cout << "\n--- 濫用 mutable（反面教材）---" << endl;
    const BadMutable23 bad(100);
    bad.takeDamage(30);  // const 對象居然能受傷？！const 失效了
    cout << "  HP=" << bad.getHp() << " (const 完全失去保護！)" << endl;
}

// ============================================================================
//  第 24 課：類別內的靜態成員變數
// ============================================================================
/*
 *  核心觀念：
 *    普通成員變數：屬於「單一對象」，每個對象各有一份
 *    靜態成員變數：屬於「整個類別」，所有對象共享同一份
 *
 *  傳統語法（C++11 之前）：
 *    類別內聲明：static int totalCount_;
 *    類別外定義：int Soldier::totalCount_ = 0;  ← 真正分配記憶體
 *    缺少定義 → 連結錯誤（undefined reference）
 *
 *  C++17 inline static（推薦）：
 *    inline static int count_ = 0;  → 直接在類別內定義初始化，不需類別外定義
 *    任何型別都行（int, double, string...）
 *
 *  三種靜態常量比較：
 *    static const int X = 10;               → C++11，僅限整數型別
 *    inline static const string S = "hi";   → C++17，任何型別
 *    static constexpr double PI = 3.14;     → 編譯期常量（最高效）
 *
 *  兩種訪問方式：
 *    推薦：Class::member    → 清楚表明是類別共享數據
 *    不推薦：obj.member      → 容易誤導，看起來像對象自己的數據
 *
 *  生命週期：
 *    程式啟動 → 靜態成員初始化（main 之前）
 *    程式結束 → 靜態成員銷毀（main 之後）
 *    即使所有對象都銷毀了，靜態成員仍然存活
 *
 *  訪問控制：
 *    private 靜態成員 → 通過 public 的靜態 getter/setter 訪問
 *    靜態 setter 加驗證 → 防止非法修改
 *
 *  sizeof 特性：
 *    靜態成員不計入 sizeof(對象)
 *    存放在靜態/全域存儲區，不在對象內部
 *
 *  普通成員 vs 靜態成員：
 *    ┌────────────┬──────────────┬──────────────┐
 *    │ 特性       │ 普通成員      │ 靜態成員      │
 *    ├────────────┼──────────────┼──────────────┤
 *    │ 歸屬       │ 屬於對象      │ 屬於類別      │
 *    │ 份數       │ 每對象一份    │ 整個類別一份  │
 *    │ 計入sizeof │ 是           │ 否           │
 *    │ 生命週期   │ 與對象相同    │ 與程式相同    │
 *    │ 訪問方式   │ obj.member   │ Class::member│
 *    │ this 指標  │ 通過 this    │ 沒有 this    │
 *    └────────────┴──────────────┴──────────────┘
 */

class Soldier24 {
private:
    string name_;
    int id_;

    // C++17 inline static：直接定義初始化
    inline static int totalCount_ = 0;
    inline static int nextId_ = 1001;

public:
    Soldier24(const string& name)
        : name_(name), id_(nextId_++)
    {
        totalCount_++;
        cout << "  [入伍] " << name_ << " ID:" << id_
             << " 總人數:" << totalCount_ << endl;
    }
    ~Soldier24() {
        totalCount_--;
        cout << "  [退役] " << name_ << " 總人數:" << totalCount_ << endl;
    }

    // 公開靜態函數訪問私有靜態成員
    static int getTotalCount() { return totalCount_; }
};

// sizeof 對比 & constexpr static
class SizeDemo24 {
    int a_, b_;                           // 各 4 bytes
    inline static int shared_ = 0;        // 不計入 sizeof
    static constexpr double PI = 3.14;    // 編譯期常量，不計入 sizeof
};

void demo_lesson24() {
    cout << "\n╔══════════════════════════════════════════╗" << endl;
    cout << "║  第 24 課：類別內的靜態成員變數           ║" << endl;
    cout << "╚══════════════════════════════════════════╝" << endl;

    cout << "\n--- 對象計數器 + 自動 ID ---" << endl;
    {
        Soldier24 s1("阿強");
        Soldier24 s2("阿明");
        cout << "  目前總人數：" << Soldier24::getTotalCount() << endl;
    }
    // 離開作用域，全部解構
    cout << "  最終人數：" << Soldier24::getTotalCount() << endl;

    cout << "\n--- sizeof 不含靜態成員 ---" << endl;
    cout << "  SizeDemo24 大小：" << sizeof(SizeDemo24)
         << " bytes（只算兩個 int）" << endl;
}

// ============================================================================
//  第 25 課：類別內的靜態成員函數
// ============================================================================
/*
 *  語法：static 返回類型 函數名(參數);
 *
 *  五大特性：
 *    1. 不依賴對象，可用 Class::func() 調用
 *    2. 沒有 this 指標
 *    3. 只能訪問靜態成員（變數和函數）
 *    4. 不能訪問非靜態成員（沒有 this，不知道是哪個對象）
 *    5. 不能加 const（沒有 this，const 無意義）
 *
 *  訪問規則：
 *    ┌────────────────┬──────────┬───────────────┐
 *    │ 調用者         │ 靜態成員  │ 非靜態成員     │
 *    ├────────────────┼──────────┼───────────────┤
 *    │ 非靜態函數      │ 可以     │ 可以（有this） │
 *    │ 靜態函數        │ 可以     │ 不行（無this） │
 *    └────────────────┴──────────┴───────────────┘
 *    非靜態函數可以訪問一切；靜態函數只能訪問靜態的東西
 *
 *  透過參數間接訪問非靜態成員：
 *    靜態函數可以接收對象參數，訪問其 private 成員
 *    （因為靜態函數仍是類別的成員，有訪問 private 的權限）
 *
 *  三大經典應用：
 *    1. 工廠函數（Factory）：
 *       - 私有建構函數 + 靜態工廠方法
 *       - 函數名傳達語義：createSmall() 比 Potion(30,50) 清楚
 *    2. 工具函數類（Utility Class）：
 *       - 全部靜態函數，禁止實例化（delete 建構函數）
 *    3. 單例模式（Singleton）：
 *       - Meyers' Singleton：靜態局部變數，只初始化一次
 *       - static MyClass& getInstance() { static MyClass inst; return inst; }
 *       - C++11 後靜態局部變數初始化是線程安全的
 *
 *  靜態函數 vs 全域函數 vs 命名空間函數：
 *    全域函數：名字可能衝突，不能訪問 private → 盡量避免
 *    命名空間函數：有隔離，但不能訪問 private → 純工具用這個
 *    類別靜態函數：有歸屬，可訪問 private，可維護狀態 → 與類別相關用這個
 */

// 工廠函數示範
class Potion25 {
private:
    string name_;
    int heal_, price_;
    Potion25(const string& n, int h, int p) : name_(n), heal_(h), price_(p) {}

public:
    // 靜態工廠函數：提供命名的創建方式
    static Potion25 createSmall()  { return Potion25("小藥水", 30, 50); }
    static Potion25 createMedium() { return Potion25("中藥水", 70, 120); }
    static Potion25 createLarge()  { return Potion25("大藥水", 150, 300); }

    void printInfo() const {
        cout << "  " << name_ << " 回復:" << heal_ << " 價格:" << price_ << endl;
    }
};

// 工具函數類示範
class MathUtil25 {
public:
    static int clamp(int val, int lo, int hi) {
        return val < lo ? lo : val > hi ? hi : val;
    }
    static double distance(double x1, double y1, double x2, double y2) {
        return sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
    }
    MathUtil25() = delete;  // 禁止實例化
};

// 單例模式示範（Meyers' Singleton）
class GameManager25 {
private:
    int score_;
    GameManager25() : score_(0) { cout << "  [GameManager 初始化]" << endl; }
public:
    GameManager25(const GameManager25&) = delete;
    GameManager25& operator=(const GameManager25&) = delete;

    static GameManager25& getInstance() {
        static GameManager25 instance;  // 靜態局部變數，只初始化一次
        return instance;
    }
    void addScore(int pts) { score_ += pts; }
    int getScore() const { return score_; }
};

void demo_lesson25() {
    cout << "\n╔══════════════════════════════════════════╗" << endl;
    cout << "║  第 25 課：類別內的靜態成員函數           ║" << endl;
    cout << "╚══════════════════════════════════════════╝" << endl;

    // 工廠函數
    cout << "\n--- 工廠函數 ---" << endl;
    // Potion25 p("x", 1, 1);  // 編譯錯誤！private 建構函數
    Potion25 small = Potion25::createSmall();
    Potion25 large = Potion25::createLarge();
    small.printInfo();
    large.printInfo();

    // 工具函數類
    cout << "\n--- 工具函數類 ---" << endl;
    cout << "  clamp(150, 0, 100) = " << MathUtil25::clamp(150, 0, 100) << endl;
    cout << "  distance(0,0,3,4)  = " << MathUtil25::distance(0, 0, 3, 4) << endl;
    // MathUtil25 m;  // 編譯錯誤！建構函數 deleted

    // 單例模式
    cout << "\n--- 單例模式（Meyers' Singleton）---" << endl;
    GameManager25::getInstance().addScore(100);
    GameManager25::getInstance().addScore(250);
    cout << "  分數：" << GameManager25::getInstance().getScore() << endl;
    // 驗證唯一性
    GameManager25& r1 = GameManager25::getInstance();
    GameManager25& r2 = GameManager25::getInstance();
    cout << "  同一實例：" << (&r1 == &r2 ? "是" : "否") << endl;
}

// ============================================================================
//  第 26 課：this 指標
// ============================================================================
/*
 *  this 的本質：
 *    - 隱含的指標，指向調用成員函數的那個對象本身
 *    - hero.takeDamage(30) 實際上是 takeDamage(&hero, 30)
 *    - this 就是 &hero
 *
 *  this 的類型：
 *    普通函數 void func()       → T* const this（可讀寫成員）
 *    const 函數 void func() const → const T* const this（只能讀取）
 *    靜態函數 static void func() → 沒有 this
 *
 *  必須顯式使用 this 的六大場景：
 *    1. 參數與成員同名 → this->name = name; 消歧
 *       （建議：成員加下劃線後綴 name_ 從根本上避免同名）
 *    2. 鏈式調用 → return *this; 返回自身引用
 *    3. 傳遞 this 給外部函數 → registerPlayer(this); / logAction(*this);
 *    4. 自我賦值檢查 → if (this == &other) return *this;
 *    5. 拷貝自身 → MyClass copy = *this;
 *    6. 比較後返回自身 → return *this; 或 return other;
 *
 *  *this 解引用：
 *    this → 指標（地址）
 *    *this → 解引用，得到對象本身
 *    return *this → 返回對象的引用（配合返回類型 ClassName&）
 *
 *  常見誤區：
 *    1. 建構函數中洩漏 this → 對象尚未完全初始化，外部使用可能出錯
 *    2. 返回局部對象的指標 → 離開作用域指標懸空（dangling pointer）
 *       安全做法：return new Obj() 或 return Obj()（值返回，RVO 優化）
 *
 *  this 使用場景總結表：
 *    ┌──────────────────────┬──────────────────────┐
 *    │ 場景                 │ 用法                  │
 *    ├──────────────────────┼──────────────────────┤
 *    │ 同名消歧             │ this->name = name;    │
 *    │ 鏈式調用             │ return *this;          │
 *    │ 傳遞自身（指標）      │ func(this);           │
 *    │ 傳遞自身（引用）      │ func(*this);          │
 *    │ 自我賦值/操作檢查     │ if (this == &other)   │
 *    │ 拷貝自身             │ T copy = *this;       │
 *    │ 比較後返回自身        │ return *this;         │
 *    └──────────────────────┴──────────────────────┘
 */

// 鏈式調用（return *this）
class QueryBuilder26 {
private:
    string table_, conditions_, orderBy_;
    int limit_;
public:
    QueryBuilder26() : limit_(0) {}

    QueryBuilder26& from(const string& t) { table_ = t; return *this; }
    QueryBuilder26& where(const string& c) {
        if (!conditions_.empty()) conditions_ += " AND ";
        conditions_ += c;
        return *this;
    }
    QueryBuilder26& limit(int n) { limit_ = n > 0 ? n : 0; return *this; }

    string build() const {
        string sql = "SELECT * FROM " + table_;
        if (!conditions_.empty()) sql += " WHERE " + conditions_;
        if (limit_ > 0) sql += " LIMIT " + to_string(limit_);
        return sql;
    }
};

// 自我賦值檢查 + 同名消歧
class Buffer26 {
private:
    // ★ 宣告順序：長度在前、指標在後。
    //   成員初始化順序永遠依**宣告順序**，與初始化列的書寫順序無關；
    //   把 size_ 放前面，初始化列的順序才與實際順序一致（也消除 -Wreorder 警告）。
    int  size_;
    int* data_;
public:
    Buffer26(int size) : size_(size), data_(new int[size]) {
        for (int i = 0; i < size; i++) data_[i] = i;
    }
    ~Buffer26() { delete[] data_; }

    // 拷貝賦值：必須檢查自我賦值
    Buffer26& operator=(const Buffer26& other) {
        if (this == &other) {  // 核心：比較自身地址和另一個對象地址
            cout << "  [賦值] 自我賦值，跳過" << endl;
            return *this;
        }
        delete[] data_;
        size_ = other.size_;
        data_ = new int[size_];
        for (int i = 0; i < size_; i++) data_[i] = other.data_[i];
        cout << "  [賦值] 完成深拷貝" << endl;
        return *this;  // 支持 a = b = c 連續賦值
    }

    void print() const {
        cout << "  [";
        for (int i = 0; i < size_; i++) {
            if (i > 0) cout << ",";
            cout << data_[i];
        }
        cout << "]" << endl;
    }
};

// RPG 英雄：綜合 this 應用
class Hero26 {
private:
    string name_;
    int hp_, maxHp_, attack_;
    Hero26* leader_;

public:
    Hero26(const string& name, int maxHp, int atk)
        : name_(name), hp_(maxHp), maxHp_(maxHp), attack_(atk), leader_(nullptr) {}

    // 應用 1：leader_ = this（把自己設為隊長）
    Hero26& becomeLeader() {
        leader_ = this;
        cout << "  ★ " << name_ << " 成為隊長！" << endl;
        return *this;
    }

    // 應用 2：this == &other（防止跟隨自己）
    Hero26& follow(Hero26& other) {
        if (this == &other) {
            cout << "  " << name_ << "：不能跟隨自己" << endl;
            return *this;
        }
        leader_ = &other;
        cout << "  " << name_ << " 跟隨 " << other.name_ << endl;
        return *this;
    }

    // 應用 3：return *this（鏈式強化）
    Hero26& boostHp(int n) { maxHp_ += n; hp_ += n; return *this; }
    Hero26& boostAttack(int n) { attack_ += n; return *this; }

    // 應用 4：比較後返回 *this 或 other
    const Hero26& betterLeader(const Hero26& other) const {
        if (this == &other) return *this;
        return (maxHp_ + attack_ * 3 >= other.maxHp_ + other.attack_ * 3)
               ? *this : other;
    }

    // 應用 5：this == &target（自我治療判斷）
    void heal(Hero26& target, int amount) {
        if (this == &target) cout << "  " << name_ << " 自我治療";
        else cout << "  " << name_ << " 治療 " << target.name_;
        int actual = min(amount, target.maxHp_ - target.hp_);
        target.hp_ += actual;
        cout << " +" << actual << " HP:" << target.hp_ << "/" << target.maxHp_ << endl;
    }

    void takeDamage(int d) { hp_ = max(0, hp_ - d); }

    // 應用 6：leader_ == this（判斷自己是否隊長）
    void printStatus() const {
        cout << "  " << name_ << " HP:" << hp_ << "/" << maxHp_
             << " ATK:" << attack_;
        if (leader_ == this) cout << " (隊長)";
        else if (leader_) cout << " (跟隨:" << leader_->name_ << ")";
        cout << endl;
    }

    const string& getName() const { return name_; }
};

void demo_lesson26() {
    cout << "\n╔══════════════════════════════════════════╗" << endl;
    cout << "║  第 26 課：this 指標                     ║" << endl;
    cout << "╚══════════════════════════════════════════╝" << endl;

    // 鏈式調用
    cout << "\n--- 鏈式調用（return *this）---" << endl;
    string sql = QueryBuilder26()
        .from("players")
        .where("level > 10")
        .where("hp > 0")
        .limit(20)
        .build();
    cout << "  " << sql << endl;

    // 自我賦值檢查
    cout << "\n--- 自我賦值檢查 ---" << endl;
    Buffer26 buf(5);
    buf.print();
    buf = buf;  // this == &other → 安全跳過
    buf.print();

    // RPG 隊伍系統（綜合 this 應用）
    cout << "\n--- RPG 隊伍系統 ---" << endl;
    Hero26 warrior("亞瑟", 300, 40);
    Hero26 mage("梅林", 150, 70);
    Hero26 healer("伊蓮", 200, 20);

    // 鏈式強化
    warrior.boostHp(50).boostAttack(10);
    // 選隊長
    const Hero26& better = warrior.betterLeader(mage);
    cout << "  更適合當隊長：" << better.getName() << endl;
    // 組隊
    warrior.becomeLeader();
    mage.follow(warrior);
    healer.follow(warrior);
    warrior.follow(warrior);  // 攔截：不能跟隨自己
    // 狀態
    warrior.printStatus();
    mage.printStatus();
    healer.printStatus();
    // 治療
    warrior.takeDamage(120);
    healer.heal(warrior, 60);
    healer.heal(healer, 30);  // 自我治療
}

// ============================================================================
// 【LeetCode 實戰範例】LeetCode 155. Min Stack
// ============================================================================
//   題目：設計一個支援 push / pop / top / getMin 的堆疊，
//         且 getMin 必須是 O(1)（不是掃描整個堆疊）。
//   為什麼用到本主題：這題是「封裝一個不變量」的教科書案例。
//         不變量 (I)：minStack_.top() 永遠等於 data_ 內所有元素的最小值。
//         這條規則靠的是「每次 push/pop 都同步維護第二個堆疊」，
//         而它成立的唯一前提，就是**外界只能透過這四個函式改變狀態**。
//         若把 data_ 開成 public（或提供回傳非 const 引用的 getter），
//         任何人 push_back 一個更小的值就會讓 getMin 永久回報錯誤答案。
//         這正是第 20、21 課在講的事。
//   本主題的其他對應點：
//         * top()/getMin()/empty() 都是唯讀 → 標 const（第 22 課）
//         * 兩個 vector 是 private，只暴露操作不暴露容器（第 21 課）
//   複雜度：四個操作皆為 O(1)；空間 O(n)。
//   註：LeetCode 保證呼叫 pop/top/getMin 時堆疊非空；
//       這裡仍加上防禦性檢查，因為實務程式碼不該假設呼叫端守規矩。
// ============================================================================
class MinStack {
private:
    vector<int> data_;      // 實際資料
    vector<int> minStack_;  // 與 data_ 等長；minStack_[i] = data_[0..i] 的最小值

public:
    void push(int val) {
        data_.push_back(val);
        // 維護不變量：新的最小值 = min(舊的最小值, val)
        if (minStack_.empty()) {
            minStack_.push_back(val);
        } else {
            minStack_.push_back(std::min(minStack_.back(), val));
        }
    }

    void pop() {
        if (data_.empty()) return;      // 防禦：實務上不假設呼叫端守規矩
        data_.pop_back();
        minStack_.pop_back();           // 兩者必須同步，不變量才成立
    }

    // 唯讀操作 → const（第 22 課）
    int  top()    const { return data_.empty() ? 0 : data_.back(); }
    int  getMin() const { return minStack_.empty() ? 0 : minStack_.back(); }
    bool empty()  const { return data_.empty(); }
    size_t size() const { return data_.size(); }

    // 刻意「不」提供 vector<int>& data() —— 那會讓不變量瞬間失守
};

// ============================================================================
// 【日常實務範例】設定檔快取：mutable + static + const 的綜合應用
// ============================================================================
//   情境：服務啟動後要頻繁讀取設定值（如逾時秒數、重試次數）。
//   實務需求：
//     (a) 讀取設定是唯讀操作 → 介面應該是 const，讓 const ConfigStore& 也能查
//     (b) 解析設定值有成本（實務上可能是查資料庫或解析字串）
//         → 第一次算完就快取，之後直接回傳
//     (c) 快取本身「外界觀察不到」→ 正是 mutable 的正當用途（第 23 課）
//     (d) 全域統計「總共查詢了幾次」屬於類別層級而非物件層級 → static（第 24 課）
//     (e) 修改設定時必須讓快取失效，否則會回傳過期資料 —— 這是快取的頭號 bug
//
//   這個範例把第 22、23、24、25 課全部串起來，也是實務上最常見的
//   「logical const」場景：getTimeout() 從外部看是純查詢，
//   內部卻改了快取與計數器。
// ============================================================================
class ConfigStore {
private:
    string  m_rawTimeout;              // 原始設定值（字串，模擬從檔案讀入）
    int     m_retries;

    // ── 以下三個是 mutable：外界觀察不到，只影響效能 ──
    mutable bool m_timeoutCached = false;
    mutable int  m_cachedTimeout = 0;
    mutable int  m_readCount = 0;      // 本物件被查詢次數

    // static：屬於類別，所有物件共享，不計入 sizeof（第 24 課）
    // C++17 的 inline static，不需要類外定義
    inline static int s_totalReads = 0;

    // private static 輔助函式：解析邏輯是實作細節（第 25 課）
    static int parseSeconds(const string& s) {
        int v = 0;
        for (char c : s) {
            if (c >= '0' && c <= '9') v = v * 10 + (c - '0');
            else break;                // 遇到 "30s" 的 's' 就停
        }
        return v;
    }

public:
    ConfigStore(const string& timeout, int retries)
        : m_rawTimeout(timeout), m_retries(retries) {}

    // const 函式，但內部改了 mutable 成員 —— logical const
    int getTimeout() const {
        ++m_readCount;
        ++s_totalReads;                // static 成員在 const 函式中也可修改
                                       // （它不屬於本物件，不受 this 的 const 影響）
        if (!m_timeoutCached) {
            cout << "    [解析] 計算 timeout（僅第一次）" << endl;
            m_cachedTimeout = parseSeconds(m_rawTimeout);
            m_timeoutCached = true;
        } else {
            cout << "    [快取] 直接回傳快取值" << endl;
        }
        return m_cachedTimeout;
    }

    int  retries()   const { return m_retries; }
    int  readCount() const { return m_readCount; }

    // 修改設定 → 必須讓快取失效（快取最常見的 bug 就是忘了這一步）
    void setTimeout(const string& s) {
        m_rawTimeout = s;
        m_timeoutCached = false;       // 關鍵：不做這行就會回傳過期資料
        cout << "    [更新] timeout 設為 \"" << s << "\"，快取已失效" << endl;
    }

    // static 成員函式：沒有 this，用 Class::func() 呼叫（第 25 課）
    static int totalReads() { return s_totalReads; }
};

void demo_leetcode_and_practice() {
    cout << "\n╔══════════════════════════════════════════╗" << endl;
    cout << "║  LeetCode 155. Min Stack                 ║" << endl;
    cout << "╚══════════════════════════════════════════╝" << endl;

    MinStack st;
    st.push(-2); st.push(0); st.push(-3);
    cout << "  push(-2), push(0), push(-3)" << endl;
    cout << "  getMin() = " << st.getMin() << "   （預期 -3）" << endl;
    st.pop();
    cout << "  pop()" << endl;
    cout << "  top()    = " << st.top()    << "   （預期 0）" << endl;
    cout << "  getMin() = " << st.getMin() << "   （預期 -2）" << endl;
    cout << "  → 不變量「minStack_.top() 等於目前最小值」全程成立，" << endl;
    cout << "     因為外界只能透過 push/pop 改變狀態。" << endl;

    // 證明唯讀介面可被 const reference 呼叫（第 22 課）
    const MinStack& ref = st;
    cout << "  const MinStack& 也能呼叫 top()/getMin()/size()："
         << ref.top() << " / " << ref.getMin() << " / " << ref.size() << endl;

    cout << "\n╔══════════════════════════════════════════╗" << endl;
    cout << "║  日常實務：設定檔快取（mutable + static）║" << endl;
    cout << "╚══════════════════════════════════════════╝" << endl;

    ConfigStore cfg("30s", 3);
    // 注意：先把結果取出再印。若直接寫 cout << cfg.getTimeout()，
    //       getTimeout() 內部的 [解析]/[快取] 訊息會插進這一行的中間，
    //       輸出看起來會錯亂 —— 這是「函式有副作用又被嵌進輸出運算式」的典型後果。
    cout << "  第一次查詢：" << endl;
    int t1 = cfg.getTimeout();
    cout << "    timeout = " << t1 << " 秒" << endl;

    cout << "  第二次查詢（應走快取）：" << endl;
    int t2 = cfg.getTimeout();
    cout << "    timeout = " << t2 << " 秒" << endl;

    cout << "  更新設定後，快取必須失效：" << endl;
    cfg.setTimeout("60s");
    int t3 = cfg.getTimeout();
    cout << "    timeout = " << t3 << " 秒（重新解析）" << endl;

    // const 物件一樣能查詢 —— 這正是 getTimeout() 標 const 的價值
    const ConfigStore frozen("15s", 5);
    cout << "  const 物件也能查詢（因為 getTimeout 是 const）：" << endl;
    int t4 = frozen.getTimeout();
    cout << "    timeout = " << t4 << " 秒" << endl;

    cout << "  本物件查詢次數 cfg.readCount() = " << cfg.readCount() << endl;
    cout << "  全類別總查詢次數 ConfigStore::totalReads() = "
         << ConfigStore::totalReads() << endl;
    cout << "  → readCount 是每物件各自的；totalReads 是 static，全類別共享。" << endl;
    cout << "  → 兩者都在 const 函式中被修改：前者靠 mutable，" << endl;
    cout << "     後者因為 static 成員根本不屬於這個物件，不受 this 的 const 限制。" << endl;
}

// ============================================================================
//  主程式
// ============================================================================
int main() {
    cout << "╔══════════════════════════════════════════════════╗" << endl;
    cout << "║  第 20～26 課：封裝專題 合併精華總結             ║" << endl;
    cout << "╚══════════════════════════════════════════════════╝" << endl;

    demo_lesson20();
    demo_lesson21();
    demo_lesson22();
    demo_lesson23();
    demo_lesson24();
    demo_lesson25();
    demo_lesson26();
    demo_leetcode_and_practice();

    // ========================================================================
    //  速查表（Speed Lookup）
    // ========================================================================
    cout << "\n╔══════════════════════════════════════════════════════════════╗" << endl;
    cout << "║  第 20～26 課 速查表                                        ║" << endl;
    cout << "╠══════════════════════════════════════════════════════════════╣" << endl;
    cout << "║                                                            ║" << endl;
    cout << "║  【第 20 課：封裝】                                         ║" << endl;
    cout << "║    三層防護：訪問控制 → 數據驗證 → 不變量維護              ║" << endl;
    cout << "║    四大好處：數據保護 / 介面穩定 / 降低耦合 / 簡化使用     ║" << endl;
    cout << "║    四個層次：語法層 → 邏輯層 → 介面層 → 模組層            ║" << endl;
    cout << "║    核心目的：維護不變量，防止對象進入非法狀態              ║" << endl;
    cout << "║                                                            ║" << endl;
    cout << "║  【第 21 課：getter / setter】                              ║" << endl;
    cout << "║    基本型別 getter → 返回值    大型物件 → const 引用       ║" << endl;
    cout << "║    危險 getter → 返回非 const 引用 = 破壞封裝！            ║" << endl;
    cout << "║    核心原則：用行為取代 setter（takeDamage > setHp）        ║" << endl;
    cout << "║    鏈式調用：setter 返回 *this → 連續調用                  ║" << endl;
    cout << "║    命名風格：getX/setX（Java）或 x()/x(val)（C++重載）    ║" << endl;
    cout << "║                                                            ║" << endl;
    cout << "║  【第 22 課：const 成員函數】                               ║" << endl;
    cout << "║    語法：void func() const → 承諾不修改成員               ║" << endl;
    cout << "║    const 對象只能調用 const 函數                           ║" << endl;
    cout << "║    this 類型：普通 T* const / const 函數 const T* const    ║" << endl;
    cout << "║    const 重載：同名 const/非const 版本共存                 ║" << endl;
    cout << "║    調用鏈：const 函數只能調用 const 函數                   ║" << endl;
    cout << "║    淺層 const：不保護指標指向的內容                        ║" << endl;
    cout << "║                                                            ║" << endl;
    cout << "║  【第 23 課：mutable】                                      ║" << endl;
    cout << "║    語法：mutable int count_; → const 函數中也可修改       ║" << endl;
    cout << "║    適用：快取 / 計數器 / 延遲初始化 / 互斥鎖              ║" << endl;
    cout << "║    判斷：去掉 mutable 變數外部觀察是否一樣？是 → 適合     ║" << endl;
    cout << "║    濫用：核心數據用 mutable → const 完全失效               ║" << endl;
    cout << "║    mutable 合法安全 > const_cast 未定義行為                ║" << endl;
    cout << "║                                                            ║" << endl;
    cout << "║  【第 24 課：靜態成員變數】                                  ║" << endl;
    cout << "║    屬於類別，所有對象共享同一份                            ║" << endl;
    cout << "║    傳統：類內聲明 + 類外定義   C++17：inline static       ║" << endl;
    cout << "║    constexpr static → 編譯期常量，最高效                  ║" << endl;
    cout << "║    推薦 Class::member 訪問   不計入 sizeof                 ║" << endl;
    cout << "║    生命週期：程式啟動 → 程式結束（超過任何對象）           ║" << endl;
    cout << "║                                                            ║" << endl;
    cout << "║  【第 25 課：靜態成員函數】                                  ║" << endl;
    cout << "║    沒有 this 指標，只能訪問靜態成員                        ║" << endl;
    cout << "║    不能加 const（沒有 this → const 無意義）                ║" << endl;
    cout << "║    可接收對象參數間接訪問 private                          ║" << endl;
    cout << "║    工廠函數：私有建構 + 靜態創建方法                       ║" << endl;
    cout << "║    工具類：全靜態 + delete 建構函數                        ║" << endl;
    cout << "║    單例：static T& getInstance() { static T i; return i; }║" << endl;
    cout << "║    vs 全域函數：靜態函數有歸屬可訪問 private              ║" << endl;
    cout << "║                                                            ║" << endl;
    cout << "║  【第 26 課：this 指標】                                    ║" << endl;
    cout << "║    本質：隱含指標，指向調用函數的對象（== &obj）           ║" << endl;
    cout << "║    類型：普通 T* const / const → const T* const / 靜態無  ║" << endl;
    cout << "║    同名消歧：this->name = name;                           ║" << endl;
    cout << "║    鏈式調用：return *this;                                 ║" << endl;
    cout << "║    傳遞自身：func(this) 或 func(*this)                    ║" << endl;
    cout << "║    自我檢查：if (this == &other)                          ║" << endl;
    cout << "║    拷貝自身：T copy = *this;                              ║" << endl;
    cout << "║    建構中洩漏 this 有風險（對象尚未完全初始化）            ║" << endl;
    cout << "║                                                            ║" << endl;
    cout << "╚══════════════════════════════════════════════════════════════╝" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary20-26.cpp -o summary20-26
//   註：本檔刻意保留 Buffer26 的 -Wreorder 警告作為教學示範（見概念補充 A），
//       該警告不影響正確性 —— Buffer26 用的是參數 size 而非未初始化的成員 size_。

// === 預期輸出 ===
// ╔══════════════════════════════════════════════════╗
// ║  第 20～26 課：封裝專題 合併精華總結             ║
// ╚══════════════════════════════════════════════════╝
//
// ╔══════════════════════════════════════════╗
// ║  第 20 課：封裝（Encapsulation）         ║
// ╚══════════════════════════════════════════╝
//   勇者 Lv.1 HP:100/100 ATK:25
//   勇者 -30 HP:70/100
//   無效傷害
//   ★ 勇者 升級 Lv.2
//   勇者 Lv.2 HP:120/120 ATK:30
//   勇者 -9999 HP:0/120
//   勇者 倒下了！
//
// ╔══════════════════════════════════════════╗
// ║  第 21 課：getter 與 setter 設計模式     ║
// ╚══════════════════════════════════════════╝
//
// --- 危險 getter ---
//   正常餘額：1500
//   竄改後：999999
//   教訓：返回非 const 引用破壞封裝！
//
// --- 行為取代 setter ---
//   哥布林 -20 HP:30
//   哥布林 暴怒！ATK=30
//
// --- 鏈式調用 ---
//   [警告] 確定刪除嗎？ (400x200)
//
// ╔══════════════════════════════════════════╗
// ║  第 22 課：const 成員函數                ║
// ╚══════════════════════════════════════════╝
//
// --- const 對象限制 ---
//   聖水 回復:100 數量:5
//   使用 藥水 剩餘:2
//   藥水 回復:50 數量:2
//
// --- const 重載 ---
//   [非 const 版本]  內容：Modified
//   [const 版本]
//
// --- 淺層 const 陷阱 ---
//   修改前：42
//   修改後：999 (const 是淺層的！)
//
// ╔══════════════════════════════════════════╗
// ║  第 23 課：mutable 關鍵字               ║
// ╚══════════════════════════════════════════╝
//
// --- 快取 + 訪問計數器 ---
//   面積=  [計算面積...]
// 78.5398
//   面積=  [使用快取]
// 78.5398
//   面積=  [使用快取]
// 78.5398
//   被訪問 3 次
//
// --- 濫用 mutable（反面教材）---
//   HP=70 (const 完全失去保護！)
//
// ╔══════════════════════════════════════════╗
// ║  第 24 課：類別內的靜態成員變數           ║
// ╚══════════════════════════════════════════╝
//
// --- 對象計數器 + 自動 ID ---
//   [入伍] 阿強 ID:1001 總人數:1
//   [入伍] 阿明 ID:1002 總人數:2
//   目前總人數：2
//   [退役] 阿明 總人數:1
//   [退役] 阿強 總人數:0
//   最終人數：0
//
// --- sizeof 不含靜態成員 ---
//   SizeDemo24 大小：8 bytes（只算兩個 int）
//
// ╔══════════════════════════════════════════╗
// ║  第 25 課：類別內的靜態成員函數           ║
// ╚══════════════════════════════════════════╝
//
// --- 工廠函數 ---
//   小藥水 回復:30 價格:50
//   大藥水 回復:150 價格:300
//
// --- 工具函數類 ---
//   clamp(150, 0, 100) = 100
//   distance(0,0,3,4)  = 5
//
// --- 單例模式（Meyers' Singleton）---
//   [GameManager 初始化]
//   分數：350
//   同一實例：是
//
// ╔══════════════════════════════════════════╗
// ║  第 26 課：this 指標                     ║
// ╚══════════════════════════════════════════╝
//
// --- 鏈式調用（return *this）---
//   SELECT * FROM players WHERE level > 10 AND hp > 0 LIMIT 20
//
// --- 自我賦值檢查 ---
//   [0,1,2,3,4]
//   [賦值] 自我賦值，跳過
//   [0,1,2,3,4]
//
// --- RPG 隊伍系統 ---
//   更適合當隊長：亞瑟
//   ★ 亞瑟 成為隊長！
//   梅林 跟隨 亞瑟
//   伊蓮 跟隨 亞瑟
//   亞瑟：不能跟隨自己
//   亞瑟 HP:350/350 ATK:50 (隊長)
//   梅林 HP:150/150 ATK:70 (跟隨:亞瑟)
//   伊蓮 HP:200/200 ATK:20 (跟隨:亞瑟)
//   伊蓮 治療 亞瑟 +60 HP:290/350
//   伊蓮 自我治療 +0 HP:200/200
//
// ╔══════════════════════════════════════════╗
// ║  LeetCode 155. Min Stack                 ║
// ╚══════════════════════════════════════════╝
//   push(-2), push(0), push(-3)
//   getMin() = -3   （預期 -3）
//   pop()
//   top()    = 0   （預期 0）
//   getMin() = -2   （預期 -2）
//   → 不變量「minStack_.top() 等於目前最小值」全程成立，
//      因為外界只能透過 push/pop 改變狀態。
//   const MinStack& 也能呼叫 top()/getMin()/size()：0 / -2 / 2
//
// ╔══════════════════════════════════════════╗
// ║  日常實務：設定檔快取（mutable + static）║
// ╚══════════════════════════════════════════╝
//   第一次查詢：
//     [解析] 計算 timeout（僅第一次）
//     timeout = 30 秒
//   第二次查詢（應走快取）：
//     [快取] 直接回傳快取值
//     timeout = 30 秒
//   更新設定後，快取必須失效：
//     [更新] timeout 設為 "60s"，快取已失效
//     [解析] 計算 timeout（僅第一次）
//     timeout = 60 秒（重新解析）
//   const 物件也能查詢（因為 getTimeout 是 const）：
//     [解析] 計算 timeout（僅第一次）
//     timeout = 15 秒
//   本物件查詢次數 cfg.readCount() = 3
//   全類別總查詢次數 ConfigStore::totalReads() = 4
//   → readCount 是每物件各自的；totalReads 是 static，全類別共享。
//   → 兩者都在 const 函式中被修改：前者靠 mutable，
//      後者因為 static 成員根本不屬於這個物件，不受 this 的 const 限制。
//
// ╔══════════════════════════════════════════════════════════════╗
// ║  第 20～26 課 速查表                                        ║
// ╠══════════════════════════════════════════════════════════════╣
// ║                                                            ║
// ║  【第 20 課：封裝】                                         ║
// ║    三層防護：訪問控制 → 數據驗證 → 不變量維護              ║
// ║    四大好處：數據保護 / 介面穩定 / 降低耦合 / 簡化使用     ║
// ║    四個層次：語法層 → 邏輯層 → 介面層 → 模組層            ║
// ║    核心目的：維護不變量，防止對象進入非法狀態              ║
// ║                                                            ║
// ║  【第 21 課：getter / setter】                              ║
// ║    基本型別 getter → 返回值    大型物件 → const 引用       ║
// ║    危險 getter → 返回非 const 引用 = 破壞封裝！            ║
// ║    核心原則：用行為取代 setter（takeDamage > setHp）        ║
// ║    鏈式調用：setter 返回 *this → 連續調用                  ║
// ║    命名風格：getX/setX（Java）或 x()/x(val)（C++重載）    ║
// ║                                                            ║
// ║  【第 22 課：const 成員函數】                               ║
// ║    語法：void func() const → 承諾不修改成員               ║
// ║    const 對象只能調用 const 函數                           ║
// ║    this 類型：普通 T* const / const 函數 const T* const    ║
// ║    const 重載：同名 const/非const 版本共存                 ║
// ║    調用鏈：const 函數只能調用 const 函數                   ║
// ║    淺層 const：不保護指標指向的內容                        ║
// ║                                                            ║
// ║  【第 23 課：mutable】                                      ║
// ║    語法：mutable int count_; → const 函數中也可修改       ║
// ║    適用：快取 / 計數器 / 延遲初始化 / 互斥鎖              ║
// ║    判斷：去掉 mutable 變數外部觀察是否一樣？是 → 適合     ║
// ║    濫用：核心數據用 mutable → const 完全失效               ║
// ║    mutable 合法安全 > const_cast 未定義行為                ║
// ║                                                            ║
// ║  【第 24 課：靜態成員變數】                                  ║
// ║    屬於類別，所有對象共享同一份                            ║
// ║    傳統：類內聲明 + 類外定義   C++17：inline static       ║
// ║    constexpr static → 編譯期常量，最高效                  ║
// ║    推薦 Class::member 訪問   不計入 sizeof                 ║
// ║    生命週期：程式啟動 → 程式結束（超過任何對象）           ║
// ║                                                            ║
// ║  【第 25 課：靜態成員函數】                                  ║
// ║    沒有 this 指標，只能訪問靜態成員                        ║
// ║    不能加 const（沒有 this → const 無意義）                ║
// ║    可接收對象參數間接訪問 private                          ║
// ║    工廠函數：私有建構 + 靜態創建方法                       ║
// ║    工具類：全靜態 + delete 建構函數                        ║
// ║    單例：static T& getInstance() { static T i; return i; }║
// ║    vs 全域函數：靜態函數有歸屬可訪問 private              ║
// ║                                                            ║
// ║  【第 26 課：this 指標】                                    ║
// ║    本質：隱含指標，指向調用函數的對象（== &obj）           ║
// ║    類型：普通 T* const / const → const T* const / 靜態無  ║
// ║    同名消歧：this->name = name;                           ║
// ║    鏈式調用：return *this;                                 ║
// ║    傳遞自身：func(this) 或 func(*this)                    ║
// ║    自我檢查：if (this == &other)                          ║
// ║    拷貝自身：T copy = *this;                              ║
// ║    建構中洩漏 this 有風險（對象尚未完全初始化）            ║
// ║                                                            ║
// ╚══════════════════════════════════════════════════════════════╝
