// =============================================================================
//  第 22 課：const 成員函數 —— summary.cpp（本課教科書級總整理）
// =============================================================================
//
// 【主題資訊 Information】
//   語法：
//     回傳型別 函式名(參數列) const;                  // const 成員函式
//     const T& f() const;   T& f();                   // 成對 const 重載
//     void g(const T& obj);                           // const 參考參數
//   語意：const 承諾「不修改 *this 的任何非 mutable 成員」,
//         實作方式是把隱含的 this 從 T* const 變成 const T* const。
//   標準版本：const 成員函式為 C++98;本檔另用到 C++11 的 to_string,
//             以及 C++17 的 [[maybe_unused]]。
//   複雜度：const 為純編譯期資訊,零執行期成本、不影響物件佈局。
//   標頭檔：<iostream>、<string>、<vector>
//
// 【詳細解釋 Explanation】
//
// 【1. const 成員函式的本質:改變 this 的型別】
//   非靜態成員函式都有一個隱含的第一參數 this:
//       void modify()        → Demo* const       this
//       void inspect() const → const Demo* const this
//   函式尾端的 const 加的是「底層 const」(指向的物件是 const);
//   而「頂層 const」(this 本身不可重新指派)在所有成員函式中都存在。
//   因此函式內寫 value_(等同 this->value_)會是 const 左值,無法賦值。
//   結論:const 不是註解、不是慣例,它是型別系統的一部分。
//
// 【2. const 物件、const 參考、const 參數:同一件事的三種面貌】
//   const T obj;    → 這個物件終其一生只露出 const 介面
//   const T& ref;   → 透過這個別名只看得到 const 介面
//   void f(const T&)→ 最常用的傳參方式:不拷貝、不修改、接受面最廣
//   綁定方向是單向的:T → const T& 合法(加 const 永遠安全),
//   const T → T& 不合法。這個單向性是整個 const 正確性的基礎。
//
// 【3. const 正確性(const correctness):不是風格,是可用性】
//   凡不改變可觀察狀態的成員函式都標 const,叫做 const 正確。
//   它之所以是基本功而非加分項,是因為 C++ 生態預設以 const& 傳遞物件。
//   一個漏了 const 的型別,在 const 語境下不是「慢一點」,是「完全不能用」。
//   而且 const 具有傳染性:底層漏標,上層全部跟著不能是 const。
//   修法一律「由內而外」,絕不是在外層加 const_cast 硬過。
//
// 【4. const 呼叫鏈:單向可傳染】
//   const 成員函式 → 只能呼叫 const 成員函式
//   非 const 成員函式 → const 與非 const 都能呼叫
//   原因同 (2):this 的 const 資格無法隱式脫去。
//   衍生查詢(由其他成員算出來的值)最容易被誤標成非 const ——
//   判準是「有沒有改變可觀察狀態」,不是「裡面有沒有做運算」。
//
// 【5. const 重載:同名、同參數,靠 const 資格分派】
//   const T& at(i) const;   // const 物件 → 唯讀
//   T&       at(i);         // 非 const 物件 → 可寫
//   兩者因隱含 this 型別不同而構成合法重載(只有回傳型別不同是不行的)。
//   編譯器依「呼叫者物件本身的 const 資格」選擇,與你打算讀還是寫無關。
//   要避免重複實作,讓非 const 版本呼叫 const 版本,再 const_cast 去掉
//   回傳值的 const;反方向絕對不可以。
//
// 【6. 淺層 const(bitwise const):const 只有一層深】
//   若成員是 T* p,const 成員函式保護的是「p 本身不能改」,
//   而不是「*p 指向的內容不能改」。也就是說:
//       void f() const { *ptr_ = 42; }   // 完全合法!
//   這是 const 最常見的認知落差,也是「邏輯 const」與 mutable
//   (第 23 課)要處理的問題。
//
// 【概念補充 Concept Deep Dive】
//   * const 資格會被編入 mangled name,所以 const 版與非 const 版在
//     連結器眼中是兩個不同符號 —— 這是它們能重載的底層原因。
//   * const 成員函式仍可做 I/O、改全域變數、改靜態成員,或透過成員指標
//     改別的物件。const 講的是「這個物件的狀態」,不是「整個程式的狀態」,
//     更不等於「純函式」。
//   * 標準庫保證:同時對同一物件呼叫 const 成員函式是不會有資料競爭的。
//     這個保證的前提是該型別確實遵守 const 語意 —— 這也是 mutable 成員
//     必須格外小心的原因(它可能讓「唯讀」操作實際上在寫入)。
//   * const_cast 的正當用途只有兩個:(a) 呼叫忘了加 const 的舊 C API;
//     (b) 非 const 版本共用 const 版本的實作。對「本來就宣告為 const
//     的物件」寫入,是未定義行為(UB),不保證任何特定結果。
//
// 【注意事項 Pay Attention】
//   1. const 寫在參數列之後;宣告與定義分離時兩邊都要寫。
//   2. 建構子與解構子不可為 const。
//   3. 回傳計算結果要回傳「值」,不可回傳區域變數的參考(懸空 → UB)。
//   4. const 只有一層深:成員指標所指向的內容不受保護。
//   5. 加 const 的正確時機是一開始就加;事後補會引發連鎖編譯錯誤。
//   6. 對非 const 物件而言,即使只是讀取也會選到非 const 重載版本。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】const 成員函數(本課總整理)
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 成員函式後面的 const 到底做了什麼?有執行期成本嗎?
//     答：它把隱含的 this 從 T* const 變成 const T* const,
//         使函式內對成員的任何寫入(本質是 this->member = ...)
//         在編譯期就被拒絕。完全沒有執行期成本 ——
//         const 是純編譯期資訊,不產生任何額外指令,也不影響物件佈局。
//     追問：那 const 物件呼叫非 const 函式,執行時會怎樣?
//         → 不會執行到那一步,它根本編譯不過。所有 const 檢查都在編譯期。
//
// 🔥 Q2. const 成員函式能不能修改成員?請把話說完整。
//     答：不能修改「非 mutable 的直接成員」。但有兩個重要例外:
//         (a) mutable 成員可以改(第 23 課主題);
//         (b) 若成員是指標 T* p,const 只保護 p 本身,
//             *p 指向的內容照樣能改 —— 這就是淺層 const。
//         所以 const 成員函式並不等於「這個函式什麼都不會改」。
//     追問：那要怎麼保護指標指向的內容?→ 把成員宣告成 const T* p,
//         或使用能正確傳播 const 的包裝型別(如 std::experimental::propagate_const)。
//
// 🔥 Q3. const 版本與非 const 版本邏輯完全相同時,怎麼避免重複?
//     答：讓非 const 版本呼叫 const 版本:先把 *this 轉成 const 參考以選到
//         const 版本,再用 const_cast 去掉回傳值的 const。因為底層物件
//         本來就不是 const,這個轉型是安全的,也是 const_cast 公認的
//         正當用途之一。反過來(const 版本呼叫非 const 版本)絕對不行。
//     追問：為什麼反方向不行?→ 那需要先把 const 物件轉成非 const;
//         若該物件本來就宣告為 const,後續修改即為未定義行為。
//
// ⚠️ 陷阱 1. 「const 成員函式保證這個函式不會改到任何東西。」
//     答：不保證。它只保證不改「本物件的非 mutable 直接成員」。
//         const 成員函式照樣可以:做 I/O、改全域變數、改 static 成員、
//         改 mutable 成員,以及透過成員指標修改別的物件的內容。
//     為什麼會錯：把 const 讀成「純函式 / 無副作用」。
//         實際上它的範圍嚴格限定在「*this 的直接成員」這一層,
//         其他任何東西都不在它的保護傘下。
//
// ⚠️ 陷阱 2. 「這個 getter 只是回傳成員,加不加 const 都能跑,先不加。」
//     答：行為確實不變,但「誰能呼叫它」變了 —— 而那才是重點。
//         漏 const 的 getter 對 const 物件、const& 參數、
//         以及任何以 const& 收下你物件的函式全部無法呼叫。
//         由於 const 會傳染,事後要補往往牽連數十個檔案,
//         最後常被迫用 const_cast 硬過,等於把型別保護整個丟掉。
//     為什麼會錯：只看「函式做了什麼」,沒看「函式的可用範圍」。
//         const 不改變行為,它決定的是這個型別能不能在 C++ 生態中正常使用。
// ═══════════════════════════════════════════════════════════════════════════

/*
 * ============================================================================
 *  第 22 課：const 成員函數 — 綜合總結
 * ============================================================================
 *
 *  本檔案涵蓋第 22 課的所有核心概念，閱讀本檔案即可完整複習，
 *  無需回頭查看其他 cpp 檔案。
 *
 *  目錄：
 *    第一節：const 成員函數基礎 — 什麼是 const 成員函數？語法與強制力
 *    第二節：const 對象的限制 — const 對象只能調用 const 成員函數
 *    第三節：const 正確性 — 好設計 vs 壞設計的對比
 *    第四節：this 指標與 const — 底層原理解析
 *    第五節：const 重載 — 同名函數的 const / 非 const 版本共存
 *    第六節：const 函數調用鏈 — const 函數之間的互相調用規則
 *    第七節：實際應用 — const 引用參數在真實場景中的威力
 *    第八節：常見錯誤與陷阱 — 淺層 const、忘記加 const 等
 *    第九節：const 正確性檢查清單 — 寫完函數後的自我檢查
 *
 * ============================================================================
 */

#include <iostream>
#include <string>
#include <vector>
using namespace std;

/* ============================================================================
 *  第一節：const 成員函數基礎
 * ============================================================================
 *
 *  const 成員函數的核心概念：
 *    - 在函數參數列表的右括號「之後」、函數體的「之前」加上 const 關鍵字
 *    - 語法：  返回類型 函數名(參數) const { ... }
 *    - 語義：  承諾「不修改任何成員變數」，編譯器會強制執行這個承諾
 *    - 如果在 const 函數體內嘗試修改成員變數，編譯器會報錯
 *
 *  何時應該使用 const：
 *    - 所有 getter 函數（getName, getHp 等）
 *    - 所有只讀/查詢函數（printInfo, isAlive 等）
 *    - 任何不修改對象狀態的函數
 *
 *  何時不應該使用 const：
 *    - 會修改成員變數的函數（setter、use、takeDamage 等）
 *
 * ============================================================================
 */

// 示範類別：藥水 — 展示 const 與非 const 成員函數的基本區別
class Potion {
private:
    string name_;       // 藥水名稱
    int healAmount_;    // 回復量
    int quantity_;      // 持有數量

public:
    Potion(const string& name, int heal, int qty)
        : name_(name), healAmount_(heal), quantity_(qty)
    {
    }

    // ------ const 成員函數：承諾不修改對象 ------
    // 這些函數只讀取成員變數，不修改它們，因此標記為 const
    // 加了 const 後，這些函數可以在 const 對象上被調用
    const string& getName() const { return name_; }         // 返回 const 引用，避免拷貝
    int getHealAmount() const { return healAmount_; }       // 返回值拷貝，不需要 const 修飾返回值
    int getQuantity() const { return quantity_; }

    void printInfo() const {
        cout << "  " << name_ << " (回復:" << healAmount_
             << " 數量:" << quantity_ << ")" << endl;

        // 以下會編譯錯誤！const 函數不能修改成員
        // quantity_ = 0;       // 錯誤！不能在 const 函數中修改成員變數
        // name_ = "被篡改";    // 錯誤！不能在 const 函數中修改成員變數
    }

    // ------ 非 const 成員函數：可以修改對象 ------
    // 這些函數會修改成員變數 quantity_，所以不能標記為 const
    bool use() {
        if (quantity_ <= 0) {
            cout << "  " << name_ << " 已用完！" << endl;
            return false;
        }
        quantity_--;    // 修改了成員變數，因此本函數不能是 const
        cout << "  使用 " << name_ << "，回復 " << healAmount_
             << " HP (剩餘:" << quantity_ << ")" << endl;
        return true;
    }

    void restock(int amount) {
        if (amount > 0) {
            quantity_ += amount;    // 修改了成員變數
            cout << "  補貨 " << name_ << " +" << amount
                 << " (總計:" << quantity_ << ")" << endl;
        }
    }
};

/* ============================================================================
 *  第二節：const 對象的限制
 * ============================================================================
 *
 *  核心規則：
 *    非 const 對象 → 可以調用 const 和非 const 成員函數
 *    const 對象    → 只能調用 const 成員函數，不能調用非 const 成員函數
 *
 *  比喻：
 *    普通人（非 const）→ 可以「看」也可以「碰」展品
 *    參觀者（const）   → 只能「看」，不能「碰」
 *
 *  const 引用參數：
 *    函數參數使用 const 引用（const T&）是非常常見的用法
 *    表示函數內部「只看不碰」，不會修改傳入的對象
 *    在函數體內只能調用該對象的 const 成員函數
 *
 * ============================================================================
 */

// 示範類別：盾牌 — 展示 const 對象的限制
class Shield {
private:
    string name_;
    int defense_;
    int durability_;

public:
    Shield(const string& name, int def, int dur)
        : name_(name), defense_(def), durability_(dur)
    {
    }

    // const 成員函數 — const 對象和非 const 對象都能調用
    const string& getName() const { return name_; }
    int getDefense() const { return defense_; }
    int getDurability() const { return durability_; }

    void printShieldInfo() const {
        cout << "  " << name_ << " [防禦:" << defense_
             << " 耐久:" << durability_ << "]" << endl;
    }

    // 非 const 成員函數 — 只有非 const 對象能調用
    void takeDamage(int dmg) {
        durability_ -= dmg;
        if (durability_ < 0) durability_ = 0;
        cout << "  " << name_ << " 耐久 -" << dmg
             << " (剩餘:" << durability_ << ")" << endl;
    }

    void repair() {
        durability_ = 100;
        cout << "  " << name_ << " 修復完成" << endl;
    }
};

// 接收 const 引用的函數 — 模擬「只看不碰」
// 參數類型是 const Shield&，表示只能讀取盾牌資料，不能修改
void inspectShield(const Shield& s) {
    cout << "\n--- 檢查盾牌（const 引用）---" << endl;

    // 可以調用 const 成員函數
    s.printShieldInfo();
    cout << "  防禦力：" << s.getDefense() << endl;
    cout << "  耐久度：" << s.getDurability() << endl;

    // 不能調用非 const 成員函數 — 因為參數是 const 引用
    // s.takeDamage(10);   // 編譯錯誤！
    // s.repair();          // 編譯錯誤！
}

/* ============================================================================
 *  第三節：const 正確性（const correctness）
 * ============================================================================
 *
 *  const 正確性是 C++ 中非常重要的設計原則：
 *    「所有不修改對象狀態的成員函數，都應該標記為 const」
 *
 *  忘記加 const 的後果：
 *    當別人用 const 引用接收你的對象時，連 getter 都不能調用
 *    這是非常常見的 C++ 新手錯誤
 *
 *  以下用 BadDesign vs GoodDesign 對比說明
 *
 * ============================================================================
 */

// 反面教材：忘記加 const
class BadDesign {
private:
    string name_;
    int value_;

public:
    BadDesign(const string& n, int v) : name_(n), value_(v) {}

    // 這些函數明明不修改對象，卻忘記加 const！
    // 導致：無法在 const 對象或 const 引用上調用這些函數
    string getName() { return name_; }                                  // 缺少 const
    int getValue() { return value_; }                                   // 缺少 const
    void printBad() { cout << name_ << ":" << value_ << endl; }        // 缺少 const
};

// 正確設計：const 正確
class GoodDesign {
private:
    string name_;
    int value_;

public:
    GoodDesign(const string& n, int v) : name_(n), value_(v) {}

    // 所有不修改對象的函數都正確標記為 const
    const string& getName() const { return name_; }
    int getValue() const { return value_; }
    void printGood() const { cout << name_ << ":" << value_ << endl; }

    // 只有修改對象的函數才不加 const
    void setValue(int v) { value_ = v; }
};

// 用 const 引用接收 — 對比效果
// [[maybe_unused]]（C++17）：明確表示「這個參數刻意不使用」，
// 讓 -Wall -Wextra 不再發出 -Wunused-parameter 警告。
// 保留參數名 b 是為了讓讀者看得懂它是什麼 —— 而它「無事可做」正是本例的重點。
void processBad([[maybe_unused]] const BadDesign& b) {
    // b.getName();   // 編譯錯誤！getName 不是 const
    // b.printBad();  // 編譯錯誤！printBad 不是 const
    cout << "  BadDesign：什麼都不能做！" << endl;
}

void processGood(const GoodDesign& g) {
    g.printGood();                                     // 完美運作
    cout << "  name = " << g.getName() << endl;        // 完美運作
    cout << "  value = " << g.getValue() << endl;      // 完美運作
}

/* ============================================================================
 *  第四節：this 指標與 const — 底層原理
 * ============================================================================
 *
 *  每個成員函數都有一個隱含的 this 指標。const 改變的就是 this 的類型：
 *
 *  普通成員函數：
 *    void setHp(int hp)
 *    隱含參數：Player* const this
 *    → this 指標本身是 const（不能指向別的對象）
 *    → 但 this 指向的內容可以修改
 *
 *  const 成員函數：
 *    int getHp() const
 *    隱含參數：const Player* const this
 *    → this 指標本身是 const
 *    → this 指向的內容「也是 const」（不能修改！）
 *
 *  圖解：
 *    普通成員函數的 this：
 *      this --> [ name_ | hp_ | maxHp_ ]
 *               可讀寫   可讀寫  可讀寫
 *
 *    const 成員函數的 this：
 *      this --> [ name_ | hp_ | maxHp_ ]
 *               只讀     只讀    只讀
 *
 * ============================================================================
 */

// 示範類別：Demo — 展示 this 指標在 const 與非 const 函數中的差異
class Demo {
private:
    int value_;

public:
    Demo(int v) : value_(v) {}

    // 普通成員函數：this 的類型是 Demo* const
    // 可以通過 this 修改成員變數
    void modify() {
        this->value_ = 999;        // 可以修改，因為 this 指向的內容非 const
        cout << "  modify(): value_ = " << value_ << endl;
    }

    // const 成員函數：this 的類型是 const Demo* const
    // 不能通過 this 修改成員變數
    void inspect() const {
        // this->value_ = 999;     // 編譯錯誤！this 指向的內容是 const
        cout << "  inspect(): value_ = " << value_ << endl;
    }
};

/* ============================================================================
 *  第五節：const 重載（const overloading）
 * ============================================================================
 *
 *  同一個函數可以同時有 const 和非 const 兩個版本，
 *  編譯器會根據對象是否為 const 來選擇調用哪個版本。
 *
 *  選擇規則：
 *    對象/引用是 const     → 調用 const 版本
 *    對象/引用是非 const   → 調用非 const 版本（如果有的話）
 *                          → 如果沒有非 const 版本，也會調用 const 版本
 *
 *  常見用途：
 *    std::vector 的 operator[] 就有兩個版本：
 *      const 版本返回 const 引用（只讀）
 *      非 const 版本返回普通引用（可讀寫）
 *
 * ============================================================================
 */

// 示範類別：TextBuffer — 展示 const 重載
class TextBuffer {
private:
    string content_;

public:
    TextBuffer(const string& text) : content_(text) {}

    // const 版本：返回 const 引用（只讀）
    // 當 const 對象或 const 引用調用時，編譯器選擇此版本
    const string& getText() const {
        cout << "  [調用 const 版本]" << endl;
        return content_;
    }

    // 非 const 版本：返回非 const 引用（可讀寫）
    // 當非 const 對象調用時，編譯器優先選擇此版本
    // 返回非 const 引用，允許調用者直接修改 content_
    string& getText() {
        cout << "  [調用非 const 版本]" << endl;
        return content_;
    }

    void print() const {
        cout << "  內容：「" << content_ << "」" << endl;
    }
};

/* ============================================================================
 *  第六節：const 函數調用鏈
 * ============================================================================
 *
 *  調用規則：
 *    const 函數 --> const 函數      可以
 *    const 函數 --> 非 const 函數   不行（編譯錯誤）
 *    非 const 函數 --> const 函數   可以
 *    非 const 函數 --> 非 const 函數 可以
 *
 *  簡記：
 *    const 函數只能調用其他 const 函數
 *    非 const 函數可以調用所有函數
 *
 * ============================================================================
 */

// 示範類別：GameCharacter — 展示 const 函數之間的調用鏈
class GameCharacter {
private:
    string name_;
    int hp_;
    int maxHp_;
    int level_;

public:
    GameCharacter(const string& name, int maxHp, int level)
        : name_(name), hp_(maxHp), maxHp_(maxHp), level_(level)
    {
    }

    // ------ const 函數群 ------
    const string& getName() const { return name_; }
    int getHp() const { return hp_; }
    int getMaxHp() const { return maxHp_; }
    int getLevel() const { return level_; }

    // const 函數可以調用其他 const 函數（getHp, getMaxHp 都是 const）
    double getHpPercent() const {
        return (static_cast<double>(getHp()) / getMaxHp()) * 100.0;
    }

    // const 函數可以調用其他 const 函數（getName, getLevel, getHpPercent 都是 const）
    string getStatusText() const {
        string status = getName() + " Lv." + to_string(getLevel());
        double pct = getHpPercent();

        if (pct > 50.0)
            status += " [健康]";
        else if (pct > 20.0)
            status += " [受傷]";
        else if (pct > 0.0)
            status += " [瀕死]";
        else
            status += " [死亡]";

        return status;
    }

    void printFullStatus() const {
        // const 函數調用其他 const 函數 — 完全合法
        cout << "  " << getStatusText() << endl;
        cout << "  HP: " << getHp() << "/" << getMaxHp()
             << " (" << getHpPercent() << "%)" << endl;

        // 不能調用非 const 函數：
        // takeDamage(10);  // 編譯錯誤！const 函數不能調用非 const 函數
    }

    // ------ 非 const 函數 ------
    void takeDamage(int dmg) {
        hp_ = max(0, hp_ - dmg);
        // 非 const 函數可以調用 const 函數 — 完全合法
        cout << "  " << getName() << " 受傷！" << getStatusText() << endl;
    }
};

/* ============================================================================
 *  第七節：實際應用 — const 引用參數的威力
 * ============================================================================
 *
 *  const 成員函數最大的實際價值：
 *    讓你的類別可以安全地通過 const 引用傳遞
 *
 *  常見的接收 const 引用的場景：
 *    1. 分析函數 — 只需要讀取對象資料
 *    2. 比較函數 — 比較兩個對象
 *    3. 列表顯示 — 遍歷容器中的元素
 *    4. 搜尋函數 — 在容器中查找特定元素
 *
 *  如果成員函數沒有標記 const，以上所有場景都會編譯失敗！
 *
 * ============================================================================
 */

// 示範類別：Monster — 展示 const 引用的實際應用
class Monster {
private:
    string name_;
    int hp_;
    int attack_;
    string element_;

public:
    Monster(const string& name, int hp, int atk, const string& elem)
        : name_(name), hp_(hp), attack_(atk), element_(elem)
    {
    }

    // 全部是 const — 讓 Monster 可以安全地通過 const 引用傳遞
    const string& getName() const { return name_; }
    int getHp() const { return hp_; }
    int getAttack() const { return attack_; }
    const string& getElement() const { return element_; }
    bool isAlive() const { return hp_ > 0; }

    void printInfo() const {
        cout << "  " << name_ << " [" << element_ << "] HP:"
             << hp_ << " ATK:" << attack_ << endl;
    }

    // 非 const：會修改狀態
    void takeDamage(int dmg) {
        hp_ = max(0, hp_ - dmg);
    }
};

// 場景 1：分析函數 — 通過 const 引用只讀取資料
void analyzeMonster(const Monster& m) {
    cout << "  分析 " << m.getName() << "：" << endl;
    cout << "    屬性：" << m.getElement() << endl;
    cout << "    威脅等級：";
    if (m.getAttack() >= 50) cout << "高";
    else if (m.getAttack() >= 30) cout << "中";
    else cout << "低";
    cout << endl;
}

// 場景 2：比較函數 — 通過 const 引用比較兩個對象
void compareMonsters(const Monster& a, const Monster& b) {
    cout << "  比較 " << a.getName() << " vs " << b.getName() << "：";
    if (a.getAttack() > b.getAttack())
        cout << a.getName() << " 更強！" << endl;
    else if (a.getAttack() < b.getAttack())
        cout << b.getName() << " 更強！" << endl;
    else
        cout << "不分上下！" << endl;
}

// 場景 3：列表顯示 — 通過 const 引用遍歷容器
void showMonsterList(const vector<Monster>& monsters) {
    cout << "  怪物圖鑑（共 " << monsters.size() << " 隻）：" << endl;
    for (const auto& m : monsters) {
        cout << "    ";
        m.printInfo();  // printInfo 必須是 const 才能在這裡調用
    }
}

// 場景 4：搜尋函數 — 通過 const 引用在容器中查找
const Monster* findByElement(const vector<Monster>& monsters,
                              const string& element)
{
    for (const auto& m : monsters) {
        if (m.getElement() == element) {
            return &m;
        }
    }
    return nullptr;
}

/* ============================================================================
 *  第八節：常見錯誤與陷阱
 * ============================================================================
 *
 *  陷阱 1：在 const 函數中意外修改成員
 *    → 編譯器會直接報錯，這是最容易發現的錯誤
 *
 *  陷阱 2：通過指標間接修改（淺層 const）
 *    → const 只保護對象的直接成員，不保護指標指向的間接數據
 *    → 例如：data_ 指標本身不能改，但 *data_ 指向的內容居然可以改！
 *    → 這是 C++ const 的重要限制：它是「淺層的」（shallow const）
 *
 *    圖解：
 *      const 保護的範圍：
 *      +---------------------------------+
 *      | 對象本身                         |
 *      |  +----------+                   |
 *      |  | data_ ---+--> [堆上的 int]   |  <-- const 不保護這裡！
 *      |  | (指標)   |    (指向的內容)    |
 *      |  +----------+                   |
 *      |   ^ const 保護這裡              |
 *      +---------------------------------+
 *
 *  陷阱 3：忘記給 getter 加 const
 *    → 導致用 const 引用接收對象時，連 getter 都無法調用
 *    → 這是最常見的 C++ 新手錯誤
 *
 * ============================================================================
 */

// 示範陷阱 2：淺層 const — 指標間接修改
class ShallowConstDemo {
private:
    int* data_;      // 指向堆上的數據

public:
    ShallowConstDemo(int val) : data_(new int(val)) {}
    ~ShallowConstDemo() { delete data_; }

    // 禁止拷貝（簡化示範）
    ShallowConstDemo(const ShallowConstDemo&) = delete;
    ShallowConstDemo& operator=(const ShallowConstDemo&) = delete;

    // 危險！const 只保護指標本身（data_），不保護指向的內容（*data_）
    void sneakyModify() const {
        // data_ = nullptr;  // 編譯錯誤：不能修改指標本身
        *data_ = 999;        // 居然可以！const 不保護指標指向的數據
    }

    int getValue() const { return *data_; }
};

/* ============================================================================
 *  第九節：const 正確性檢查清單
 * ============================================================================
 *
 *  寫完一個成員函數後，問自己：
 *
 *    1. 這個函數會修改任何成員變數嗎？
 *       → 否 → 加 const
 *       → 是 → 不加 const
 *
 *    2. 這個函數調用的其他成員函數都是 const 嗎？
 *       → 如果你的函數是 const，它只能調用 const 函數
 *
 *    3. 返回值類型正確嗎？
 *       → const 函數返回成員的引用 → 必須是 const 引用
 *       → 返回值（拷貝）→ 不需要 const
 *
 *    4. 有沒有通過指標間接修改數據？
 *       → const 不保護指標指向的內容，需要自律
 *
 *  養成習慣：
 *    寫完一個成員函數，如果它不修改任何成員，立刻加上 const
 *
 * ============================================================================
 */

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 705. Design HashSet
//   題目：不使用任何內建雜湊表，自行實作 MyHashSet，支援
//         add(key) / remove(key) / contains(key)。
//   為什麼用到本主題：這題的三個方法天然分成兩類 ——
//         add / remove 會改變容器狀態  → 非 const 成員函式
//         contains     只查詢、不改狀態 → 應為 const 成員函式
//   LeetCode 給的骨架通常把 contains 寫成非 const（判題不在意），
//   但那正是「漏 const」的典型案例：一旦 MyHashSet 被以
//   const MyHashSet& 傳進任何函式，contains() 就無法呼叫，
//   這個容器等於廢掉一半。本檔刻意把它寫成 const 正確的版本，
//   並用 countPresent() 證明「只有 const 正確才能在 const 語境下使用」。
//   作法：鏈結法（separate chaining）—— 固定 769 個 bucket（質數可降低碰撞），
//         每個 bucket 是一條 vector。
//   複雜度：平均 O(1)（負載因子低時），最壞 O(n)；空間 O(n + 桶數)。
// -----------------------------------------------------------------------------
class MyHashSet {
private:
    static const int BUCKETS = 769;      // 取質數，讓 key % BUCKETS 分布較均勻
    vector<vector<int>> table_;

    // 私有輔助函式也要 const —— 否則 const 版的 contains() 無法呼叫它
    int bucketOf(int key) const { return key % BUCKETS; }

public:
    MyHashSet() : table_(BUCKETS) {}

    // 會改變狀態 → 不能是 const
    void add(int key) {
        vector<int>& bucket = table_[bucketOf(key)];
        for (int v : bucket) {
            if (v == key) return;        // 已存在，集合不放重複值
        }
        bucket.push_back(key);
    }

    // 會改變狀態 → 不能是 const
    void remove(int key) {
        vector<int>& bucket = table_[bucketOf(key)];
        for (size_t i = 0; i < bucket.size(); ++i) {
            if (bucket[i] == key) {
                bucket[i] = bucket.back();   // 與最後一個交換再彈出，O(1) 移除
                bucket.pop_back();
                return;
            }
        }
    }

    // 純查詢 → 必須是 const，這是本檔的重點
    bool contains(int key) const {
        const vector<int>& bucket = table_[bucketOf(key)];
        for (int v : bucket) {
            if (v == key) return true;
        }
        return false;
    }

    // 衍生查詢：同樣 const，且呼叫其他 const 成員
    size_t size() const {
        size_t n = 0;
        for (const auto& bucket : table_) n += bucket.size();
        return n;
    }
};

// 以 const& 接收 → 只有在 contains() / size() 是 const 時才編得過。
// 若照 LeetCode 骨架把 contains() 寫成非 const，這個函式整段都無法編譯。
static int countPresent(const MyHashSet& s, const vector<int>& queries) {
    int hit = 0;
    for (int q : queries) {
        if (s.contains(q)) ++hit;
    }
    return hit;
}

/* ============================================================================
 *  主程式：依序展示每個概念
 * ============================================================================
 */

int main() {

    // ========================================================================
    //  第一節示範：const 成員函數基礎
    // ========================================================================
    cout << "============================================================" << endl;
    cout << "  第一節：const 成員函數基礎" << endl;
    cout << "============================================================" << endl;

    Potion potion("治療藥水", 50, 3);

    // const 函數：只讀操作 — 不修改對象
    cout << "\n--- const 函數（只讀）---" << endl;
    potion.printInfo();                                // const 函數
    cout << "  名稱：" << potion.getName() << endl;    // const 函數
    cout << "  數量：" << potion.getQuantity() << endl; // const 函數

    // 非 const 函數：修改操作 — 會改變 quantity_
    cout << "\n--- 非 const 函數（修改）---" << endl;
    potion.use();       // 非 const 函數，quantity_ 從 3 變 2
    potion.use();       // 非 const 函數，quantity_ 從 2 變 1
    potion.printInfo(); // 確認數量已變為 1

    // ========================================================================
    //  第二節示範：const 對象的限制
    // ========================================================================
    cout << "\n============================================================" << endl;
    cout << "  第二節：const 對象的限制" << endl;
    cout << "============================================================" << endl;

    // 非 const 對象：所有函數都能調用
    cout << "\n--- 非 const 對象 ---" << endl;
    Shield shield("鐵盾", 40, 100);
    shield.printShieldInfo();       // const 函數 — 可以調用
    shield.takeDamage(20);          // 非 const 函數 — 可以調用
    shield.repair();                // 非 const 函數 — 可以調用

    // const 對象：只能調用 const 函數
    cout << "\n--- const 對象 ---" << endl;
    const Shield legendaryShield("傳說之盾", 100, 999);
    legendaryShield.printShieldInfo();      // const 函數 — 可以調用
    legendaryShield.getDefense();           // const 函數 — 可以調用
    // legendaryShield.takeDamage(10);      // 編譯錯誤！const 對象不能調用非 const 函數
    // legendaryShield.repair();            // 編譯錯誤！const 對象不能調用非 const 函數

    // const 引用參數：函數內部只能「看」不能「碰」
    inspectShield(shield);

    // ========================================================================
    //  第三節示範：const 正確性（好設計 vs 壞設計）
    // ========================================================================
    cout << "\n============================================================" << endl;
    cout << "  第三節：const 正確性（好設計 vs 壞設計）" << endl;
    cout << "============================================================" << endl;

    BadDesign bad("壞設計", 42);
    GoodDesign good("好設計", 42);

    cout << "\n--- 用 const 引用傳遞 ---" << endl;
    processBad(bad);      // 幾乎什麼都做不了，因為缺少 const
    processGood(good);    // 正常工作，因為所有 getter 都有 const

    // ========================================================================
    //  第四節示範：this 指標與 const
    // ========================================================================
    cout << "\n============================================================" << endl;
    cout << "  第四節：this 指標與 const（底層原理）" << endl;
    cout << "============================================================" << endl;

    Demo d(42);
    d.inspect();    // const 函數：this 類型是 const Demo* const，只能讀
    d.modify();     // 非 const 函數：this 類型是 Demo* const，可以讀寫
    d.inspect();    // 再次查看：值已從 42 變為 999

    // ========================================================================
    //  第五節示範：const 重載
    // ========================================================================
    cout << "\n============================================================" << endl;
    cout << "  第五節：const 重載" << endl;
    cout << "============================================================" << endl;

    // 非 const 對象 → 調用非 const 版本的 getText()
    cout << "\n--- 非 const 對象 ---" << endl;
    TextBuffer buf("Hello");
    buf.getText();                   // 調用非 const 版本
    buf.getText() = "Modified!";     // 非 const 版本返回非 const 引用，可以修改
    buf.print();                     // 確認內容已變為 "Modified!"

    // const 對象 → 調用 const 版本的 getText()
    cout << "\n--- const 對象 ---" << endl;
    const TextBuffer constBuf("ReadOnly");
    constBuf.getText();              // 調用 const 版本
    // constBuf.getText() = "Hack!"; // 編譯錯誤！const 版本返回 const 引用，不能修改
    constBuf.print();

    // const 引用 → 即使原始對象非 const，通過 const 引用也只能調用 const 版本
    cout << "\n--- const 引用 ---" << endl;
    const TextBuffer& ref = buf;
    ref.getText();                   // 調用 const 版本（因為 ref 是 const 引用）
    ref.print();

    // ========================================================================
    //  第六節示範：const 函數調用鏈
    // ========================================================================
    cout << "\n============================================================" << endl;
    cout << "  第六節：const 函數調用鏈" << endl;
    cout << "============================================================" << endl;

    GameCharacter hero("戰士", 200, 5);

    // printFullStatus 是 const 函數
    // 它內部調用 getStatusText(), getHp(), getMaxHp(), getHpPercent() — 都是 const
    // const 函數可以調用其他 const 函數
    cout << "\n--- 初始狀態 ---" << endl;
    hero.printFullStatus();

    // takeDamage 是非 const 函數
    // 它內部調用 getName(), getStatusText() — 都是 const
    // 非 const 函數可以調用 const 函數
    cout << "\n--- 受傷後 ---" << endl;
    hero.takeDamage(120);
    hero.printFullStatus();

    hero.takeDamage(60);
    hero.printFullStatus();

    // ========================================================================
    //  第七節示範：const 引用參數的實際應用
    // ========================================================================
    cout << "\n============================================================" << endl;
    cout << "  第七節：const 引用參數的實際應用" << endl;
    cout << "============================================================" << endl;

    vector<Monster> monsters = {
        Monster("火龍", 500, 60, "火"),
        Monster("冰狼", 300, 40, "冰"),
        Monster("雷鷹", 200, 55, "雷"),
        Monster("土蟲", 800, 20, "土")
    };

    // 場景 1：列表顯示 — 通過 const vector<Monster>& 傳遞
    cout << "\n--- 怪物列表 ---" << endl;
    showMonsterList(monsters);

    // 場景 2：分析函數 — 通過 const Monster& 傳遞
    cout << "\n--- 分析怪物 ---" << endl;
    analyzeMonster(monsters[0]);
    analyzeMonster(monsters[3]);

    // 場景 3：比較函數 — 兩個 const Monster& 傳遞
    cout << "\n--- 比較怪物 ---" << endl;
    compareMonsters(monsters[0], monsters[2]);
    compareMonsters(monsters[1], monsters[3]);

    // 場景 4：搜尋函數 — 返回 const Monster* 指標
    cout << "\n--- 搜尋怪物 ---" << endl;
    const Monster* found = findByElement(monsters, "雷");
    if (found) {
        cout << "  找到：";
        found->printInfo();     // printInfo 必須是 const 才能通過 const 指標調用
    }

    // ========================================================================
    //  第八節示範：淺層 const 陷阱
    // ========================================================================
    cout << "\n============================================================" << endl;
    cout << "  第八節：淺層 const 陷阱" << endl;
    cout << "============================================================" << endl;

    ShallowConstDemo scd(42);
    cout << "  修改前：" << scd.getValue() << endl;
    scd.sneakyModify();     // 在 const 函數中通過指標間接修改了數據！
    cout << "  修改後：" << scd.getValue() << endl;
    cout << "  （注意：const 函數居然成功修改了數據，因為 const 是淺層的）" << endl;

    // ========================================================================
    //  本課重點回顧
    // ========================================================================
    cout << "\n============================================================" << endl;
    cout << "  本課重點回顧" << endl;
    cout << "============================================================" << endl;
    cout << endl;
    cout << "  1. const 成員函數：承諾不修改任何成員變數，編譯器強制執行" << endl;
    cout << "  2. 語法位置：const 放在參數列表 ) 之後、{ 之前" << endl;
    cout << "  3. const 對象的限制：只能調用 const 成員函數" << endl;
    cout << "  4. const 正確性：所有不修改對象的函數都應標記 const" << endl;
    cout << "  5. this 指標：const 函數中 this 是 const T* const" << endl;
    cout << "  6. const 重載：同一函數的 const 和非 const 版本可共存" << endl;
    cout << "  7. 調用鏈：const 函數只能調用 const 函數" << endl;
    cout << "  8. 實際價值：讓類別可以安全地通過 const 引用傳遞" << endl;
    cout << "  9. 淺層 const：只保護直接成員，不保護指標指向的內容" << endl;
    cout << "  10. 養成習慣：不修改成員的函數，立刻加 const" << endl;
    cout << endl;

    // ========================================================================
    //  LeetCode 705. Design HashSet — 查詢 / 修改介面切分與 const
    // ========================================================================
    cout << "============================================================" << endl;
    cout << "  LeetCode 705. Design HashSet" << endl;
    cout << "============================================================" << endl;

    MyHashSet hs;
    // LeetCode 官方範例
    hs.add(1);
    hs.add(2);
    cout << "\n  add(1), add(2)" << endl;
    cout << "  contains(1) = " << (hs.contains(1) ? "true" : "false")
         << "   （預期 true）"  << endl;
    cout << "  contains(3) = " << (hs.contains(3) ? "true" : "false")
         << "  （預期 false）" << endl;

    hs.add(2);
    cout << "  add(2) 再一次（集合不放重複值）" << endl;
    cout << "  contains(2) = " << (hs.contains(2) ? "true" : "false")
         << "   （預期 true）"  << endl;
    cout << "  目前元素數 = " << hs.size() << "   （預期 2）" << endl;

    hs.remove(2);
    cout << "  remove(2)" << endl;
    cout << "  contains(2) = " << (hs.contains(2) ? "true" : "false")
         << "  （預期 false）" << endl;
    cout << "  目前元素數 = " << hs.size() << "   （預期 1）" << endl;

    // 關鍵示範：以 const& 傳遞。只有 contains()/size() 是 const 才編得過
    hs.add(100);
    hs.add(769);   // 769 % 769 == 0，與 key=0 落在同一個 bucket（示範碰撞）
    hs.add(0);
    const MyHashSet& roSet = hs;
    vector<int> queries = {1, 2, 3, 100, 769, 0};
    cout << "\n  以 const MyHashSet& 查詢 {1,2,3,100,769,0}：" << endl;
    cout << "  命中數 = " << countPresent(roSet, queries)
         << "   （預期 4：1、100、769、0）" << endl;
    cout << "  碰撞驗證：769 與 0 落在同一 bucket，仍能各自正確查到" << endl;
    cout << "  ↑ 這段之所以能編譯，全靠 contains() 與 size() 標了 const" << endl;
    cout << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "summary.cpp" -o l22_sum
// 執行: ./l22_sum        (rc=0)

// === 預期輸出 ===
// ============================================================
//   第一節：const 成員函數基礎
// ============================================================
//
// --- const 函數（只讀）---
//   治療藥水 (回復:50 數量:3)
//   名稱：治療藥水
//   數量：3
//
// --- 非 const 函數（修改）---
//   使用 治療藥水，回復 50 HP (剩餘:2)
//   使用 治療藥水，回復 50 HP (剩餘:1)
//   治療藥水 (回復:50 數量:1)
//
// ============================================================
//   第二節：const 對象的限制
// ============================================================
//
// --- 非 const 對象 ---
//   鐵盾 [防禦:40 耐久:100]
//   鐵盾 耐久 -20 (剩餘:80)
//   鐵盾 修復完成
//
// --- const 對象 ---
//   傳說之盾 [防禦:100 耐久:999]
//
// --- 檢查盾牌（const 引用）---
//   鐵盾 [防禦:40 耐久:100]
//   防禦力：40
//   耐久度：100
//
// ============================================================
//   第三節：const 正確性（好設計 vs 壞設計）
// ============================================================
//
// --- 用 const 引用傳遞 ---
//   BadDesign：什麼都不能做！
// 好設計:42
//   name = 好設計
//   value = 42
//
// ============================================================
//   第四節：this 指標與 const（底層原理）
// ============================================================
//   inspect(): value_ = 42
//   modify(): value_ = 999
//   inspect(): value_ = 999
//
// ============================================================
//   第五節：const 重載
// ============================================================
//
// --- 非 const 對象 ---
//   [調用非 const 版本]
//   [調用非 const 版本]
//   內容：「Modified!」
//
// --- const 對象 ---
//   [調用 const 版本]
//   內容：「ReadOnly」
//
// --- const 引用 ---
//   [調用 const 版本]
//   內容：「Modified!」
//
// ============================================================
//   第六節：const 函數調用鏈
// ============================================================
//
// --- 初始狀態 ---
//   戰士 Lv.5 [健康]
//   HP: 200/200 (100%)
//
// --- 受傷後 ---
//   戰士 受傷！戰士 Lv.5 [受傷]
//   戰士 Lv.5 [受傷]
//   HP: 80/200 (40%)
//   戰士 受傷！戰士 Lv.5 [瀕死]
//   戰士 Lv.5 [瀕死]
//   HP: 20/200 (10%)
//
// ============================================================
//   第七節：const 引用參數的實際應用
// ============================================================
//
// --- 怪物列表 ---
//   怪物圖鑑（共 4 隻）：
//       火龍 [火] HP:500 ATK:60
//       冰狼 [冰] HP:300 ATK:40
//       雷鷹 [雷] HP:200 ATK:55
//       土蟲 [土] HP:800 ATK:20
//
// --- 分析怪物 ---
//   分析 火龍：
//     屬性：火
//     威脅等級：高
//   分析 土蟲：
//     屬性：土
//     威脅等級：低
//
// --- 比較怪物 ---
//   比較 火龍 vs 雷鷹：火龍 更強！
//   比較 冰狼 vs 土蟲：冰狼 更強！
//
// --- 搜尋怪物 ---
//   找到：  雷鷹 [雷] HP:200 ATK:55
//
// ============================================================
//   第八節：淺層 const 陷阱
// ============================================================
//   修改前：42
//   修改後：999
//   （注意：const 函數居然成功修改了數據，因為 const 是淺層的）
//
// ============================================================
//   本課重點回顧
// ============================================================
//
//   1. const 成員函數：承諾不修改任何成員變數，編譯器強制執行
//   2. 語法位置：const 放在參數列表 ) 之後、{ 之前
//   3. const 對象的限制：只能調用 const 成員函數
//   4. const 正確性：所有不修改對象的函數都應標記 const
//   5. this 指標：const 函數中 this 是 const T* const
//   6. const 重載：同一函數的 const 和非 const 版本可共存
//   7. 調用鏈：const 函數只能調用 const 函數
//   8. 實際價值：讓類別可以安全地通過 const 引用傳遞
//   9. 淺層 const：只保護直接成員，不保護指標指向的內容
//   10. 養成習慣：不修改成員的函數，立刻加 const
//
// ============================================================
//   LeetCode 705. Design HashSet
// ============================================================
//
//   add(1), add(2)
//   contains(1) = true   （預期 true）
//   contains(3) = false  （預期 false）
//   add(2) 再一次（集合不放重複值）
//   contains(2) = true   （預期 true）
//   目前元素數 = 2   （預期 2）
//   remove(2)
//   contains(2) = false  （預期 false）
//   目前元素數 = 1   （預期 1）
//
//   以 const MyHashSet& 查詢 {1,2,3,100,769,0}：
//   命中數 = 4   （預期 4：1、100、769、0）
//   碰撞驗證：769 與 0 落在同一 bucket，仍能各自正確查到
//   ↑ 這段之所以能編譯，全靠 contains() 與 size() 標了 const
//
