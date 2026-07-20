// =============================================================================
//  第 11 課 -4  —  protected：介於 private 與 public 之間的「家族內」存取權
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  class Base { protected: /* 成員 */ };
//           class Derived : public Base { /* 這裡碰得到 Base 的 protected */ };
//   標準：  C++98 起
//   標頭檔：本例僅需 <iostream>、<string>
//   關鍵詞：protected、inheritance、fragile base class、Template Method
//
//   三種存取權的可存取者一覽：
//     ┌───────────┬──────┬──────────┬────────┬────────┐
//     │           │ 自己 │ friend   │ 衍生類 │ 外界   │
//     ├───────────┼──────┼──────────┼────────┼────────┤
//     │ public    │  ✅  │    ✅    │   ✅   │   ✅   │
//     │ protected │  ✅  │    ✅    │   ✅   │   ❌   │
//     │ private   │  ✅  │    ✅    │   ❌   │   ❌   │
//     └───────────┴──────┴──────────┴────────┴────────┘
//
// 【詳細解釋 Explanation】
//
// 【1. protected 存在的理由：讓子類別「參與實作」】
// 基底類別常常需要保留一些狀態給子類別使用，但又不想開放給全世界。
// 例如 Animal 的 species/legs：Dog 與 Spider 必須能設定它們（因為每個物種不同），
// 但外界不該能把一隻狗的腿數改成 100。protected 精確地表達了這個意圖。
//
// 【2. 關鍵限制：protected 只能透過「自己這一支」存取】
// 這是 protected 最常被誤解的規則。衍生類別 D 可以存取
//   * 自己的 protected 成員（this->species）
//   * 另一個 D 物件的 protected 成員（other.species，同型別）
// 但「不能」透過 Base 的指標／reference 存取，也不能碰兄弟類別：
//     class Dog : public Animal {
//         void f(Animal& a)  { a.species = "x"; }   // ❌ 編譯錯誤
//         void g(Spider& s)  { s.species = "x"; }   // ❌ 編譯錯誤（兄弟類別）
//         void h(Dog& d)     { d.species = "x"; }   // ✅ 同型別，合法
//     };
// 理由是型別安全：若允許 Dog 透過 Animal& 改 protected 成員，
// 它可能實際指向一隻 Spider，等於讓 Dog 破壞 Spider 的不變量。
// 本檔在 Dog 內留了 copyFrom(const Dog&)（可編譯，同型別）與兩行註解掉的
// touchBase(Animal&)／touchSibling(Spider&)（解除註解即編譯失敗）作為對照。
//
// 【3. 為什麼「protected 資料成員」被廣泛視為 code smell】
// protected 資料成員的破壞力接近 public，只是觀眾換成「所有子類別」：
//   * 你無法控制誰來繼承你的類別（任何人都可以）。
//   * 一旦子類別直接讀寫 species，這個欄位就變成對整個繼承體系的公開契約。
//   * 想改型別（string → enum）、想改語意、想刪掉它，全部子類別都會壞。
//     這正是所謂的 fragile base class problem（脆弱基底類別問題）。
// 業界共識（Effective C++、C++ Core Guidelines C.133）是：
//     protected 用於「成員函式」，資料成員維持 private。
// 子類別要改狀態，就透過基底提供的 protected 成員函式 —— 檢查點還在你手上。
// 本檔的 BetterAnimal 示範這個做法。
//
// 【4. protected 與「繼承方式」是兩件事】
// 初學者常混淆 `class D : public B` 裡的 public 和成員的 access specifier。
//   * 成員的 access specifier：決定「誰能碰這個成員」。
//   * 繼承方式（public/protected/private 繼承）：決定「基底的成員在子類別中變成什麼存取權」。
// public 繼承：public 仍 public、protected 仍 protected（is-a 關係，最常用）。
// protected 繼承：基底的 public 與 protected 在子類別中都變成 protected。
// private 繼承：兩者都變成 private（表達 implemented-in-terms-of，不是 is-a）。
// 不論哪種繼承方式，基底的 private 成員「永遠」不可被子類別存取。
//
// 【概念補充 Concept Deep Dive】
// (A) protected 成員的存取檢查在編譯期完成，執行期零成本
//     與 public/private 相同，protected 不產生任何額外指令或記憶體開銷。
//     它純粹影響「哪些程式碼被允許寫出這個名字」。
//
// (B) Template Method 模式：protected 的正當用途
//     基底提供 public 的「流程骨架」，把可變步驟開成 protected（常搭配 virtual），
//     子類別只覆寫步驟、不改流程。這樣基底仍掌控整體順序與前後置處理。
//     本檔的 ReportGenerator 就是這個模式的最小示範。
//
// (C) protected 建構函式：讓類別「只能被繼承，不能被直接實例化」
//     若一個基底類別不打算被直接建立（但也還不到抽象類別的程度），
//     可以把建構函式設為 protected —— 子類別仍能在初始化列表中呼叫它，
//     但外界寫 `Base b;` 會編譯失敗。
//
// (D) 存取權與虛擬函式是正交的
//     一個 virtual 函式可以是 private，而子類別「仍然能覆寫它」
//     （覆寫檢查的是簽名，不是存取權），只是子類別不能「呼叫」它。
//     這是 NVI（Non-Virtual Interface）慣用法的基礎：
//     public 非虛擬函式定流程，private virtual 讓子類別客製細節。
//
// 【注意事項 Pay Attention】
// 1. protected 只能透過「自己的型別」存取，不能透過 Base& 或兄弟類別。
// 2. protected 資料成員 ≈ 對所有子類別 public，優先改用 protected 成員函式。
// 3. 基底的 private 成員，任何繼承方式下子類別都碰不到。
// 4. 「成員的 access specifier」與「繼承方式」是兩個不同的概念，別混淆。
// 5. 存取權不影響 virtual 覆寫：private virtual 一樣可以被子類別覆寫。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】protected 與繼承
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. protected 和 private 差在哪？為什麼多數指南建議「資料成員不要用 protected」？
//     答：protected 額外開放給衍生類別。但因為「誰能繼承你」是你控制不了的，
//         protected 資料成員等於對整個（開放的）繼承體系公開，
//         之後改型別／改語意／刪除都會震到所有子類別 —— 這就是
//         fragile base class problem。建議 protected 只用於成員函式，
//         資料維持 private，子類別透過受控的 protected 函式修改狀態。
//     追問：那子類別要怎麼設定基底的狀態？
//         → 透過基底提供的 protected setter（可含驗證），或在建構函式初始化列表
//           呼叫基底建構函式。
//
// 🔥 Q2. 子類別 Dog 能不能透過 Animal& 存取 Animal 的 protected 成員？
//     答：不能。protected 的規則是「只能透過自己的型別（或其衍生型別）存取」。
//         `void f(Animal& a) { a.species = "x"; }` 寫在 Dog 裡會編譯失敗。
//         `void h(Dog& d) { d.species = "x"; }` 才合法。
//     追問：為什麼要有這條限制？
//         → 型別安全。否則 Dog 可以拿一個實際指向 Spider 的 Animal&，
//           去改寫 Spider 的內部狀態，破壞它的不變量。
//
// ⚠️ 陷阱. 「把成員設成 protected 就等於保留了封裝，只是多開放給子類別」——錯在哪？
//     答：錯在低估了「子類別」這個群體的大小與不可控性。private 的觀眾是
//         「我自己寫的這個 class」，數量固定、你完全掌控；protected 的觀眾是
//         「所有現在與未來繼承它的類別」，包含別人寫的、別的專案寫的。
//         從「可以安全修改」的角度看，protected 資料成員比較接近 public 而非 private。
//     為什麼會錯：多數人腦中把三種存取權想成一條「開放程度」的均勻刻度
//         （private < protected < public），於是以為 protected 大約在中間。
//         但真正該問的是「這個決定凍結了多少我改不動的程式碼」，
//         而在開放繼承下，protected 凍結的量級與 public 相當。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
using namespace std;

// -----------------------------------------------------------------------------
// 基礎示範：protected 資料成員（教學用；實務上建議改用下方 BetterAnimal 的做法）
// -----------------------------------------------------------------------------
class Animal {
protected:
    string species = "未知";   // 外界不能存取，但子類可以
    int legs = 0;

public:
    void showInfo() const {
        cout << "  " << species << ", " << legs << " 條腿" << endl;
    }
};

class Dog : public Animal {
public:
    void setup() {
        species = "犬科";   // 子類可以存取 protected
        legs = 4;
    }

    // 同型別物件的 protected 成員：合法
    void copyFrom(const Dog& other) {
        species = other.species;   // ✅ 同為 Dog
        legs = other.legs;
    }

    // 下面兩個若解除註解都會編譯失敗，示範 protected 的核心限制：
    // void touchBase(Animal& a)     { a.species = "x"; }  // ❌ 不能透過 Base&
    // void touchSibling(Spider& s)  { s.species = "x"; }  // ❌ 不能碰兄弟類別
};

class Spider : public Animal {
public:
    void setup() {
        species = "蛛形綱"; // 子類可以存取 protected
        legs = 8;
    }
};

// -----------------------------------------------------------------------------
// 改良版：資料 private，只開放受控的 protected 成員函式
//   子類別仍能設定狀態，但必須經過驗證 —— 檢查點留在基底手上。
// -----------------------------------------------------------------------------
class BetterAnimal {
private:
    string m_species = "未知";
    int    m_legs = 0;

protected:
    // protected 成員函式：子類別的正式管道，含驗證
    void setSpecies(const string& s) {
        m_species = s.empty() ? "未知" : s;
    }
    void setLegs(int n) {
        if (n < 0)   n = 0;
        if (n > 100) n = 100;      // 沒有動物有 100 條以上的腿
        m_legs = n;
    }

public:
    void showInfo() const {
        cout << "  " << m_species << ", " << m_legs << " 條腿" << endl;
    }
};

class Centipede : public BetterAnimal {
public:
    void setup() {
        setSpecies("唇足綱");
        setLegs(354);              // 超出上限，被基底的驗證夾成 100
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】Template Method 模式：報表產生器
//   情境：公司所有報表都要遵守同一套流程
//     ① 印表頭（含公司名與報表名）② 印內容 ③ 印頁尾（含免責聲明）
//   流程順序不可被子類別更動（合規要求），但「內容」每種報表都不同。
//   做法：public generate() 定死流程骨架（非虛擬），
//        把可變的一步開成 protected virtual 讓子類別覆寫。
//   這是 protected 的正當用途：開放的是「參與實作的權利」，不是「內部狀態」。
// -----------------------------------------------------------------------------
class ReportGenerator {
private:
    string m_company = "Acme Corp";     // private：子類別不該直接碰

protected:
    // 開放給子類別覆寫的步驟（protected virtual）
    virtual void renderBody() const = 0;

    // 開放給子類別「讀取」的受控管道，而非直接暴露欄位
    const string& company() const { return m_company; }

public:
    virtual ~ReportGenerator() = default;

    // 非虛擬的流程骨架：子類別改不了順序
    void generate(const string& title) const {
        cout << "  ┌── " << m_company << " │ " << title << " ──" << endl;
        renderBody();                              // ← 唯一的變異點
        cout << "  └── 本報表僅供內部使用 ──" << endl;
    }
};

class SalesReport : public ReportGenerator {
protected:
    void renderBody() const override {
        cout << "  │ Q3 營收： $1,240,000" << endl;
        cout << "  │ 年增率： +18.4%" << endl;
    }
};

class InventoryReport : public ReportGenerator {
protected:
    void renderBody() const override {
        cout << "  │ 庫存品項： 1,832 項" << endl;
        cout << "  │ 低水位警示： 27 項" << endl;
        cout << "  │ 資料來源： " << company() << " WMS" << endl;  // 透過受控管道
    }
};

int main() {
    cout << "=== protected：子類別可以存取，外界不行 ===" << endl;
    Dog d;
    d.setup();
    d.showInfo();

    Spider s;
    s.setup();
    s.showInfo();

    // 下面兩行若解除註解會編譯失敗
    // d.species = "test";  // ❌ error: 'species' is protected
    // d.legs = 100;        // ❌ error: 'legs' is protected
    cout << "  （d.species = \"test\"; 會編譯失敗：'species' is protected）" << endl;

    cout << "\n=== protected 只能透過「自己的型別」存取 ===" << endl;
    Dog d2;
    d2.copyFrom(d);          // ✅ Dog 存取另一個 Dog 的 protected
    d2.showInfo();
    cout << "  → copyFrom(const Dog&) 合法；若參數型別改成 Animal& 就編譯失敗。" << endl;

    cout << "\n=== 改良版：資料 private + protected 成員函式（含驗證） ===" << endl;
    Centipede c;
    c.setup();               // 要求 354 條腿
    c.showInfo();            // 被基底驗證夾成 100
    cout << "  → 子類別要求 354 條腿，被基底的 setLegs() 夾成 100。" << endl;
    cout << "     資料是 private，這個檢查點繞不過去。" << endl;

    cout << "\n=== 日常實務：Template Method（流程固定、內容可換） ===" << endl;
    SalesReport sales;
    sales.generate("業績月報");

    InventoryReport inv;
    inv.generate("庫存週報");
    cout << "  → 兩份報表格式一致：表頭表尾由基底掌控，子類別只換內容。" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 11 課：存取修飾符——public、private、protected4.cpp" -o access4

// === 預期輸出 ===
// === protected：子類別可以存取，外界不行 ===
//   犬科, 4 條腿
//   蛛形綱, 8 條腿
//   （d.species = "test"; 會編譯失敗：'species' is protected）
//
// === protected 只能透過「自己的型別」存取 ===
//   犬科, 4 條腿
//   → copyFrom(const Dog&) 合法；若參數型別改成 Animal& 就編譯失敗。
//
// === 改良版：資料 private + protected 成員函式（含驗證） ===
//   唇足綱, 100 條腿
//   → 子類別要求 354 條腿，被基底的 setLegs() 夾成 100。
//      資料是 private，這個檢查點繞不過去。
//
// === 日常實務：Template Method（流程固定、內容可換） ===
//   ┌── Acme Corp │ 業績月報 ──
//   │ Q3 營收： $1,240,000
//   │ 年增率： +18.4%
//   └── 本報表僅供內部使用 ──
//   ┌── Acme Corp │ 庫存週報 ──
//   │ 庫存品項： 1,832 項
//   │ 低水位警示： 27 項
//   │ 資料來源： Acme Corp WMS
//   └── 本報表僅供內部使用 ──
//   → 兩份報表格式一致：表頭表尾由基底掌控，子類別只換內容。
