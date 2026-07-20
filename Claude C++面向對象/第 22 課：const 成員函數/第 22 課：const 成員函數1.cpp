// =============================================================================
//  第 22 課：const 成員函數 1  —  const 是編譯器強制執行的承諾
// =============================================================================
//
// 【主題資訊 Information】
//   語法：
//     回傳型別 函式名(參數列) const { ... }      // const 放在參數列之後
//   意義：承諾此函式不修改 *this 的任何非 mutable 成員。
//   標準版本：C++98 起即有。
//   複雜度：const 本身沒有執行期成本,是純編譯期的型別檢查。
//   標頭檔：<string>
//
// 【詳細解釋 Explanation】
//
// 【1. const 到底改變了什麼:隱含的 this 參數】
//   每個非靜態成員函式都有一個看不見的第一參數 this。
//       void use()             → this 的型別是 Potion* const
//       void printInfo() const → this 的型別是 const Potion* const
//   差別就在被指向的物件有沒有 const。因為 this 指向的是 const Potion,
//   任何 quantity_ = 0; 這種寫法都會被編譯器擋下——它等價於
//   this->quantity_ = 0;,而 this 指向 const 物件。
//   所以 const 不是註解、不是慣例,它直接改變了 this 的型別。
//
// 【2. 為什麼「編譯器強制執行」比「寫註解說明」有價值】
//   若只是在註解寫「這個函式不會改東西」,半年後有人在裡面加一行
//   quantity_--,沒有任何機制會發現。加了 const,同一行程式碼會在
//   編譯期直接失敗。const 把「口頭承諾」變成「型別系統的一部分」。
//
// 【3. 判斷該不該加 const 的實務準則】
//   函式只讀取成員、不改變物件可觀察狀態 → 加 const。
//   實作上幾乎所有 getter、所有 printXxx()、所有 isXxx()/hasXxx() 都該加。
//   本檔 use()/restock() 會改 quantity_,所以不能加,這是正確的。
//   準則的反面同樣重要:不要為了讓某個函式能被 const 物件呼叫,
//   就硬把它標成 const 再用 const_cast 繞過——那是把型別系統的保護丟掉。
//
// 【4. const 的傳染性(這才是它真正的價值)】
//   一旦某個函式接受 const Potion&,它就只能呼叫 const 成員函式;
//   那些 const 函式內部又只能呼叫其他 const 函式。於是「唯讀」這個性質
//   會沿著整條呼叫鏈自動傳播並被檢查。這也是為什麼「漏加一個 const」
//   常常會讓一整條呼叫鏈編不過——問題通常在最底層那個忘了加 const 的函式。
//
// 【概念補充 Concept Deep Dive】
//   * const 成員函式與非 const 成員函式,在編譯後是兩個不同的符號
//     (name mangling 會把 const 資格編進去),所以它們可以構成重載。
//   * const 只保證「不修改本物件的成員」,不保證函式是純函式:
//     const 成員函式照樣可以寫檔案、印訊息、修改全域變數,
//     或透過成員指標修改「別的物件」。本檔 printInfo() 就有 I/O。
//   * 若成員是指標 T* p,const 成員函式保護的是「p 這個指標本身不能改」,
//     而不是「*p 指向的內容不能改」。也就是 const 只有一層深(bitwise const),
//     這正是第 23 課 mutable 與「邏輯 const」要處理的問題。
//
// 【注意事項 Pay Attention】
//   1. const 要寫在參數列之後:void f() const;。寫在前面
//      (const void f())意思完全不同,那是在修飾回傳型別。
//   2. 宣告與定義分離時,兩邊都要寫 const,否則會被視為不同的重載而連結失敗。
//   3. const 成員函式不能呼叫同一物件的非 const 成員函式。
//   4. 建構子與解構子不能是 const —— 它們本來就在建立/銷毀物件狀態。
//   5. 加 const 的時機是「一開始就加」。等到程式碼長大再補,
//      往往會引發一連串編譯錯誤(俗稱 const 傳染),補起來很痛苦。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】const 成員函數基礎
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 成員函式後面那個 const 到底做了什麼?
//     答：它把隱含的 this 指標型別從 T* const 改成 const T* const。
//         因為 this 指向 const 物件,函式內任何對成員的寫入
//         (實際上都是 this->member = ...)都會在編譯期被拒絕。
//         這是型別系統層級的強制,不是慣例或註解。
//     追問：那它有執行期成本嗎?→ 完全沒有。const 是純編譯期資訊,
//         不會產生任何額外指令,也不會影響物件佈局。
//
// 🔥 Q2. const 成員函式是不是就代表這個函式沒有副作用?
//     答：不是。const 只保證「不修改 *this 的非 mutable 成員」。
//         它照樣可以做 I/O(本檔 printInfo 就在印東西)、修改全域變數、
//         修改靜態成員,或透過成員指標去改別的物件。
//         const 講的是「這個物件的狀態」,不是「整個程式的狀態」。
//     追問：那要怎麼表達「真的沒有副作用」?→ C++ 沒有這種語言層級保證;
//         constexpr 是「可在編譯期求值」,語意也不等於無副作用。
//
// ⚠️ 陷阱. 「這個 getter 現在能用就好,const 之後有需要再補上。」
//     答：這是實務上代價最高的決定之一。const 具有傳染性:
//         一個接受 const T& 的函式只能呼叫 const 成員函式,
//         而那些函式又只能呼叫其他 const 函式。專案長大後再補 const,
//         往往一改就是幾十個檔案連鎖編譯失敗,壓力之下最常見的
//         「解法」就是加 const_cast 硬過——等於把型別保護整個丟掉。
//     為什麼會錯：把 const 當成「事後可以加上的修飾」,
//         但它其實是介面契約的一部分,和回傳型別、參數型別同等級。
//         正確做法是一開始就對所有唯讀函式加 const。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 從缺,理由如下
//   const 正確性是 C++ 型別系統的議題,LeetCode 判題只執行程式碼、
//   不檢查介面契約,把成員函式標成 const 與否對通過與否毫無影響。
//   本課 summary.cpp 會以 705. Design HashSet 說明「查詢 vs 修改」
//   的介面切分如何自然對應到 const / 非 const,在此不重複掛題。
//
// =============================================================================

#include <iostream>
#include <string>
using namespace std;

class Potion {
private:
    string name_;
    int healAmount_;
    int quantity_;

public:
    Potion(const string& name, int heal, int qty)
        : name_(name), healAmount_(heal), quantity_(qty)
    {
    }

    // ====== const 成員函數：承諾不修改對象 ======
    // const 成員函數承諾不修改對象的狀態（成員變量）
    // 這允許在 const 對象上調用這些函數，並且編譯器會強制執行這一承諾
    const string& getName() const { return name_; }
    int getHealAmount() const { return healAmount_; }
    int getQuantity() const { return quantity_; }

    void printInfo() const {
        cout << "  " << name_ << " (回復:" << healAmount_
             << " 數量:" << quantity_ << ")" << endl;

        // 以下會編譯錯誤！const 函數不能修改成員
        // quantity_ = 0;       // 錯誤！
        // name_ = "被篡改";    // 錯誤！
    }

    // ====== 非 const 成員函數：可以修改對象 ======
    // 非 const 成員函數可以修改對象的狀態，這些函數不能在 const 對象上調用
    // 這裡的 use() 函數會修改 quantity_，所以不能是 const
    bool use() {
        if (quantity_ <= 0) {
            cout << "  " << name_ << " 已用完！" << endl;
            return false;
        }
        quantity_--;
        cout << "  使用 " << name_ << "，回復 " << healAmount_
             << " HP (剩餘:" << quantity_ << ")" << endl;
        return true;
    }

    void restock(int amount) {
        if (amount > 0) {
            quantity_ += amount;
            cout << "  補貨 " << name_ << " +" << amount
                 << " (總計:" << quantity_ << ")" << endl;
        }
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】訂單物件:唯讀報表函式為何非要 const 不可
//   情境：訂單建立後，會被丟給「列印收據」「稽核記錄」「風控檢查」等
//         多個模組。這些模組一律以 const Order& 接收 —— 它們只該讀，
//         不該改。這個約束不是靠開會約定，而是靠型別系統擋住。
//   重點：printReceipt() 之所以能成立，是因為 Order 的每一個查詢函式
//         都加了 const。只要 subtotal() 漏掉 const，printReceipt()
//         整個函式就編不過 —— 這就是 const 傳染性的實際樣貌。
// -----------------------------------------------------------------------------
class Order {
private:
    string orderId_;
    string customer_;
    int    unitPriceCents_;
    int    quantity_;
    int    discountPercent_;
    bool   paid_;

public:
    Order(const string& id, const string& customer, int unitPriceCents, int qty)
        : orderId_(id), customer_(customer), unitPriceCents_(unitPriceCents)
        , quantity_(qty), discountPercent_(0), paid_(false) {}

    // ===== 查詢介面：全部 const =====
    const string& id()       const { return orderId_; }
    const string& customer() const { return customer_; }
    int  quantity()          const { return quantity_; }
    bool isPaid()            const { return paid_; }

    // 衍生值也是查詢 → 同樣 const，而且可以呼叫其他 const 函式
    int subtotalCents() const { return unitPriceCents_ * quantity_; }
    int discountCents() const { return subtotalCents() * discountPercent_ / 100; }
    int totalCents()    const { return subtotalCents() - discountCents(); }

    // ===== 修改介面：不能是 const =====
    bool applyDiscount(int percent) {
        if (percent < 0 || percent > 50) return false;   // 規則：折扣上限 50%
        if (paid_) return false;                          // 已付款不得再改價
        discountPercent_ = percent;
        return true;
    }

    bool markPaid() {
        if (paid_) return false;
        paid_ = true;
        return true;
    }
};

// 以「元」為單位格式化分為單位的金額（避免浮點誤差，金流系統的標準做法）
static string formatMoney(int cents) {
    return to_string(cents / 100) + "." + (cents % 100 < 10 ? "0" : "") + to_string(cents % 100);
}

// 參數是 const Order& → 內部只能呼叫 const 成員函式
// 若 Order 的任一查詢函式漏了 const，這個函式會直接編譯失敗
static void printReceipt(const Order& o) {
    cout << "    訂單 " << o.id() << "  客戶：" << o.customer() << endl;
    cout << "    數量：" << o.quantity()
         << "  小計：" << formatMoney(o.subtotalCents())
         << "  折扣：-" << formatMoney(o.discountCents())
         << "  應付：" << formatMoney(o.totalCents()) << endl;
    cout << "    付款狀態：" << (o.isPaid() ? "已付款" : "未付款") << endl;
    // o.markPaid();   // ❌ 編譯錯誤：const Order& 不能呼叫非 const 成員函式
}

int main() {
    cout << "=== const 成員函數基礎 ===" << endl;

    Potion potion("治療藥水", 50, 3);

    // const 函數：只讀操作
    cout << "\n--- const 函數（只讀）---" << endl;
    potion.printInfo();
    cout << "  名稱：" << potion.getName() << endl;
    cout << "  數量：" << potion.getQuantity() << endl;

    // 非 const 函數：修改操作
    cout << "\n--- 非 const 函數（修改）---" << endl;
    potion.use();
    potion.use();
    potion.printInfo();

    // ─────────────────────────────────────────────────────────
    cout << "\n=== 日常實務：訂單與唯讀報表 ===" << endl;
    Order order("ORD-20260719-001", "陳信安", 12900, 3);   // 單價 129.00 元 x 3

    cout << "\n--- 套用折扣前 ---" << endl;
    printReceipt(order);

    cout << "\n--- 套用 15% 折扣 ---" << endl;
    cout << "    applyDiscount(15) → " << (order.applyDiscount(15) ? "成功" : "失敗") << endl;
    cout << "    applyDiscount(80) → " << (order.applyDiscount(80) ? "成功" : "失敗")
         << "（超過 50% 上限，被規則擋下）" << endl;
    printReceipt(order);

    cout << "\n--- 付款後不得再改價 ---" << endl;
    cout << "    markPaid()        → " << (order.markPaid() ? "成功" : "失敗") << endl;
    cout << "    applyDiscount(30) → " << (order.applyDiscount(30) ? "成功" : "失敗")
         << "（已付款，被規則擋下）" << endl;
    printReceipt(order);

    // const 物件：只能走查詢介面
    const Order& readOnly = order;
    cout << "\n--- 以 const 參考觀察（只能讀）---" << endl;
    cout << "    應付金額：" << formatMoney(readOnly.totalCents()) << endl;
    // readOnly.markPaid();   // ❌ 編譯錯誤

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 22 課：const 成員函數1.cpp" -o l22_1
// 執行: ./l22_1        (rc=0)

// === 預期輸出 ===
// === const 成員函數基礎 ===
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
// === 日常實務：訂單與唯讀報表 ===
//
// --- 套用折扣前 ---
//     訂單 ORD-20260719-001  客戶：陳信安
//     數量：3  小計：387.00  折扣：-0.00  應付：387.00
//     付款狀態：未付款
//
// --- 套用 15% 折扣 ---
//     applyDiscount(15) → 成功
//     applyDiscount(80) → 失敗（超過 50% 上限，被規則擋下）
//     訂單 ORD-20260719-001  客戶：陳信安
//     數量：3  小計：387.00  折扣：-58.05  應付：328.95
//     付款狀態：未付款
//
// --- 付款後不得再改價 ---
//     markPaid()        → 成功
//     applyDiscount(30) → 失敗（已付款，被規則擋下）
//     訂單 ORD-20260719-001  客戶：陳信安
//     數量：3  小計：387.00  折扣：-58.05  應付：328.95
//     付款狀態：已付款
//
// --- 以 const 參考觀察（只能讀）---
//     應付金額：328.95
