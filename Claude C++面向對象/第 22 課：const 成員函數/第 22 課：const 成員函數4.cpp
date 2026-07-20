// =============================================================================
//  第 22 課：const 成員函數 4  —  const 如何改變 this 的型別
// =============================================================================
//
// 【主題資訊 Information】
//   this 的型別對照(以 class Demo 為例):
//     void modify()        → this 的型別是       Demo* const
//     void inspect() const → this 的型別是 const Demo* const
//   標準版本：C++98 起即有。
//   複雜度：純編譯期資訊,零執行期成本。
//   標頭檔：<iostream>
//
// 【詳細解釋 Explanation】
//
// 【1. 兩個 const,位置不同,意思完全不同】
//   const Demo* const this
//   ~~~~~             ← 第一個 const:所指向的物件是 const(不能改成員)
//               ~~~~~ ← 第二個 const:指標本身是 const(不能讓 this 指向別處)
//   第二個 const 在「所有」成員函式中都存在,不論有沒有寫 const ——
//   this 永遠不可被重新指派,你不能寫 this = &other。
//   成員函式後面那個 const,加上去的是「第一個」const。
//
// 【2. 從右往左讀指標宣告】
//   讀 const Demo* const 的口訣是從右往左:
//     const        →「常數的」
//     *            →「指標,指向」
//     const Demo   →「常數 Demo」
//   合起來:「一個常數指標,指向一個常數 Demo」。
//   對照 Demo* const:「一個常數指標,指向一個(可變的) Demo」。
//   這個讀法對所有 C/C++ 指標宣告都適用,值得記起來。
//
// 【3. 為什麼寫 value_ = 999 會被擋下】
//   在成員函式裡寫 value_,編譯器實際上把它解讀成 this->value_。
//   在 const 成員函式中,this 是 const Demo* const,
//   因此 this->value_ 是個 const int 左值 —— 對它賦值自然不合法。
//   換句話說,擋下你的不是什麼特別的「const 檢查」,
//   而是最基本的「不能對 const 物件賦值」規則。
//
// 【4. this 是什麼、不是什麼】
//   * 它是關鍵字,不是變數,不能取位址、不能重新指派。
//   * 它只存在於「非靜態」成員函式中;靜態成員函式沒有 this
//     (那是第 25 課的主題)。
//   * 它是 prvalue(純右值),型別如上;在多數 ABI 上,
//     它以隱含的第一個參數傳遞給成員函式。
//
// 【概念補充 Concept Deep Dive】
//   * 顯式寫 this->value_ 與直接寫 value_ 產生完全相同的機器碼;
//     顯式寫法的價值在可讀性,以及在樣板繼承中「依賴名稱查找」
//     必須靠 this-> 才找得到基底類別成員的情況。
//   * const 資格會被編進 mangled name。可用 nm 或 c++filt 觀察到
//     inspect() const 與 modify() 是兩個不同的符號 —— 這也是
//     兩者可以構成重載的底層原因。
//   * 因為 this 只是隱含參數,呼叫 const 與非 const 成員函式在
//     組譯層面沒有任何差異;const 完全不影響程式碼產生。
//
// 【注意事項 Pay Attention】
//   1. this 不可被賦值(this = ... 一律編譯錯誤),因為它永遠是頂層 const。
//   2. const 成員函式裡,this 指向 const 物件,故也不能把 this 傳給
//      需要 Demo* 的函式;只能傳給接受 const Demo* 的函式。
//   3. 在建構子中 this 已經可用,但物件尚未完全建構完成;
//      在建構子/解構子中呼叫虛擬函式不會分派到衍生類別 —— 那是後續課程主題。
//   4. 對已解構物件使用 this(例如把 this 存起來留到之後用)是
//      未定義行為(UB),不保證任何特定結果。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】this 指標與 const
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. const 成員函式裡的 this 是什麼型別?非 const 的呢?
//     答：const 成員函式中是 const T* const,非 const 成員函式中是 T* const。
//         兩者都有「頂層 const」(指標本身不可重新指派,所以不能寫 this = ...);
//         差別在於前者多了「底層 const」,使得 this->member 成為 const 左值,
//         任何寫入都會編譯失敗。
//     追問：那 static 成員函式裡的 this 是什麼?→ 沒有 this。
//         靜態成員函式不繫結到任何物件,這正是它不能存取非靜態成員的原因。
//
// 🔥 Q2. 為什麼 const 成員函式和非 const 成員函式可以同名重載?
//     答：因為隱含的 this 參數型別不同(const T* const vs T* const),
//         等於參數列不同,構成合法重載。const 資格也會被編入 mangled name,
//         所以連結器看到的是兩個不同符號。編譯器依「呼叫者物件是否為 const」
//         決定選哪一個。
//     追問：兩個版本內容重複怎麼辦?→ 常見手法是讓非 const 版本呼叫
//         const 版本,再用 const_cast 去掉回傳值的 const
//         (這是 const_cast 少數公認正當的用途之一)。
//
// ⚠️ 陷阱. 「成員函式後面的 const,是不是代表 this 指標本身不能改?」
//     答：不是。this 指標本身在「所有」成員函式中都不能改 ——
//         不論有沒有寫 const,this 永遠是頂層 const(T* const),
//         你在哪裡都不能寫 this = &other。
//         函式後面那個 const 加上的是「底層 const」,
//         它管的是「不能透過 this 修改成員」,和指標本身能不能改無關。
//     為什麼會錯：看到宣告裡有兩個 const,直覺把函式尾端那個 const
//         對應到「指標本身」。實際上尾端 const 加的是最靠近型別的那一個
//         (const Demo*),另一個是所有成員函式本來就有的。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 從缺,理由如下
//   this 的型別是語言機制,不是演算法;LeetCode 無法檢驗這個知識點。
//   this 指標的完整應用(鏈式呼叫、自我賦值檢查、回傳 *this)
//   是第 26 課的主題,該課會有對應的實作示範。
//
// =============================================================================

#include <iostream>
using namespace std;

class Demo {
private:
    int value_;

public:
    Demo(int v) : value_(v) {}

    // 普通成員函數
    // 在普通成員函數中，this 的類型是 Demo* const，表示 this 是一個指向 Demo 對象的常量指針（指針本身不可修改，但指向的對象可以修改）
    // 這裡的 modify() 函數是非 const 成員函數，可以修改對象的狀態（成員變量），因此 this 的類型是 Demo* const
    // 這裡的 inspect() 函數是 const 成員函數，承諾不修改對象的狀態，因此 this 的類型是 const Demo* const
    void modify() {
        // this 的類型是 Demo* const
        this->value_ = 999;        // ✅ 可以修改
        cout << "  modify(): value_ = " << value_ << endl;
    }

    // const 成員函數
    // 在 const 成員函數中，this 的類型是 const Demo* const，表示 this 是一個指向 Demo 對象的常量指針，並且指向的對象也是常量（不可修改）
    // 這裡的 inspect() 函數是 const 成員函數，承諾不修改對象的狀態，因此 this 的類型是 const Demo* const
    void inspect() const {
        // this 的類型是 const Demo* const
        // this->value_ = 999;     // ❌ 編譯錯誤！
        cout << "  inspect(): value_ = " << value_ << endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】環形緩衝區:用 const 資格決定「誰能拿到寫入權」
//   情境：網路收封包的環形緩衝區。寫入端需要可寫的 slot，
//         監控/統計端只該讀。兩者呼叫同一個名字 slot(i)，
//         由「呼叫者本身是不是 const」自動選到正確的版本。
//   這正是 this 型別差異的實際用途：同名、同參數，
//   靠隱含的 this 參數型別分派出唯讀版與可寫版。
// -----------------------------------------------------------------------------
class RingBuffer {
private:
    static const int CAP = 4;
    int   data_[CAP];
    int   count_;

public:
    RingBuffer() : data_{0, 0, 0, 0}, count_(0) {}

    // 非 const 版本：this 是 RingBuffer* const → 回傳可寫參考
    int& slot(int i) {
        cout << "    [非 const 版本 → 可寫]" << endl;
        return data_[i % CAP];
    }

    // const 版本：this 是 const RingBuffer* const → 只能回傳唯讀參考
    const int& slot(int i) const {
        cout << "    [const 版本 → 唯讀]" << endl;
        return data_[i % CAP];
    }

    void push(int v) {
        // 成員函式內部寫 data_ 等同 this->data_
        this->data_[count_ % CAP] = v;
        ++count_;
    }

    int count() const { return count_; }

    int sum() const {
        int s = 0;
        // const 成員函式裡只能讀，且只能呼叫其他 const 成員函式
        for (int i = 0; i < CAP; ++i) s += data_[i];
        return s;
    }
};

// 統計端：以 const& 接收 → 內部一律走 const 版本
static void reportStats(const RingBuffer& rb) {
    cout << "    已寫入次數：" << rb.count() << "  目前總和：" << rb.sum() << endl;
    int v = rb.slot(0);                                // 必然選到 const 版本
    cout << "    讀 slot(0)：" << v << endl;
}

int main() {
    cout << "=== this 指標與 const ===" << endl;

    Demo d(42);
    d.inspect();    // const 函數
    d.modify();     // 非 const 函數
    d.inspect();    // 再次查看

    // ─────────────────────────────────────────────────────────
    cout << "\n=== 日常實務：環形緩衝區的 const 重載分派 ===" << endl;
    RingBuffer rb;
    rb.push(10);
    rb.push(20);
    rb.push(30);

    cout << "\n--- 寫入端（非 const 物件）---" << endl;
    rb.slot(0) = 99;              // 選到非 const 版本，可以寫入
    // 注意：slot() 自己會印一行字，所以先把值取出來再印。
    // 若直接寫成 cout << "..." << rb.slot(0)，slot() 的訊息會夾在
    // 串鏈中間，輸出交錯難讀 —— 這是「有副作用的函式別放進 << 串鏈」的實例。
    int written = rb.slot(0);
    cout << "    寫入後 slot(0) = " << written << endl;

    cout << "\n--- 統計端（以 const& 接收）---" << endl;
    reportStats(rb);

    cout << "\n--- const 物件只看得到 const 版本 ---" << endl;
    const RingBuffer& ro = rb;
    int readOnlyVal = ro.slot(1);
    cout << "    ro.slot(1) = " << readOnlyVal << endl;
    // ro.slot(1) = 5;   // ❌ 編譯錯誤：const 版本回傳 const int&

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 22 課：const 成員函數4.cpp" -o l22_4
// 執行: ./l22_4        (rc=0)

// === 預期輸出 ===
// === this 指標與 const ===
//   inspect(): value_ = 42
//   modify(): value_ = 999
//   inspect(): value_ = 999
//
// === 日常實務：環形緩衝區的 const 重載分派 ===
//
// --- 寫入端（非 const 物件）---
//     [非 const 版本 → 可寫]
//     [非 const 版本 → 可寫]
//     寫入後 slot(0) = 99
//
// --- 統計端（以 const& 接收）---
//     已寫入次數：3  目前總和：149
//     [const 版本 → 唯讀]
//     讀 slot(0)：99
//
// --- const 物件只看得到 const 版本 ---
//     [const 版本 → 唯讀]
//     ro.slot(1) = 20
