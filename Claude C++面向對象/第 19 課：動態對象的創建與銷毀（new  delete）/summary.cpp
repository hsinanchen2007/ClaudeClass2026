// =============================================================================
//  summary.cpp  —  第 19 課總複習：動態對象的創建與銷毀（new / delete）
// =============================================================================
//
// 【主題資訊 Information】
//   語法    ：T* p = new T(args);      delete p;
//             T* a = new T[n];         delete[] a;
//             T* p = new(std::nothrow) T;   // 失敗回傳 nullptr
//   標準版本：new/delete 為 C++98；unique_ptr 為 C++11；make_unique 為 C++14
//   標頭檔  ：<new>（bad_alloc、nothrow）、<memory>（unique_ptr、make_unique）
//   複雜度  ：單次配置通常視為 O(1)，但配置器可能需要向 OS 要記憶體而變慢
//
// 【詳細解釋 Explanation】
//
// 【1. new 到底做了幾件事（這是 new 與 malloc 的根本差異）】
//   new T(args) 是「兩步驟」：
//     (1) 呼叫 operator new 配置 sizeof(T) 位元組的原始記憶體；
//     (2) 在那塊記憶體上呼叫 T 的建構函數。
//   delete p 則是反過來的兩步驟：
//     (1) 呼叫 p 所指物件的解構函數；
//     (2) 呼叫 operator delete 歸還記憶體。
//   malloc/free 只做上述的配置與歸還，「完全不碰建構與解構」。
//   所以對有建構函數的類別使用 malloc，會得到一塊未初始化的記憶體，
//   把它當成物件使用是未定義行為。這也是 new 回傳 T* 而 malloc 回傳 void*
//   （需要轉型）的原因 —— new 知道自己在建立什麼型別。
//
// 【2. 為什麼 new[] 一定要配 delete[]】
//   delete[] 需要知道「陣列裡有幾個元素」才能逐一呼叫解構函數。
//   實作通常會在配置的區塊前面偷偷多存一個元素個數（cookie），
//   delete[] 從那裡讀取。若對 new[] 的結果使用 delete，
//   解構函數只會被呼叫一次，而且歸還記憶體時的起始位址也可能不對
//   —— 這是未定義行為，不可寫成「一定會崩潰」。
//   反過來對 new 的結果用 delete[] 同樣是未定義行為。
//   本課的結論很簡單：兩者必須嚴格配對，而最好的辦法是根本不要手寫它們。
//
// 【3. 失敗處理：例外優先，nothrow 是特例】
//   一般的 new 失敗時擲出 std::bad_alloc，成功時保證回傳非空指標
//   （所以檢查 nullptr 是死碼）。
//   `new(std::nothrow)` 則退回 C 風格：失敗回傳 nullptr，需自行檢查。
//   選擇原則：預設用會擲例外的版本；只有在不能使用例外的環境
//   （嵌入式、核心、關閉例外的建置）才用 nothrow。
//
// 【4. 洩漏的四種場景，以及為什麼 RAII 是唯一可靠的解法】
//   忘記 delete／覆蓋指標／提前 return／例外中斷 ——
//   後兩者的共通點是「離開函式的路徑不只一條」，
//   而手寫的 delete 只守得住你有寫到的那一條。
//   RAII 把釋放動作交給區域物件的解構函數，
//   而編譯器保證每一條離開 scope 的路徑（含例外展開）都會執行解構函數。
//   你不可能漏掉任何一條 —— 因為那不是你在寫的程式碼。
//   最關鍵的一點是：洩漏的物件，它的解構函數永遠不會被呼叫。
//   若那個解構函數負責 flush 檔案、commit 交易、釋放鎖，
//   這些副作用就全部消失了。洩漏從來不只是「記憶體變少」。
//
// 【5. 現代 C++ 的結論：不要手寫 new/delete】
//   用 std::make_unique<T>(args...) 取代 new，
//   讓 unique_ptr 在解構時自動 delete。
//   成本是零（sizeof 與裸指標相同、存取會被 inline），
//   換到的是「不可能忘記釋放」與例外安全。
//   需要動態陣列時，std::vector 通常比 unique_ptr<T[]> 更合適。
//
// 【概念補充 Concept Deep Dive】
//   operator new 與 new 運算式是兩個不同層次的東西：
//     * `new T` 是「new 運算式」—— 語言結構，負責配置 + 建構。
//     * `operator new(size_t)` 是「配置函式」—— 一個可以被替換或多載的函式。
//   這個分離讓你能自訂記憶體來源（記憶體池、共享記憶體、對齊配置），
//   卻不必改動任何 `new T` 的呼叫端。
//   placement new（`new (ptr) T(args)`）則是只做第二步：
//   在既有的記憶體上建構物件。它必須以「手動呼叫解構函數」
//   （`p->~T();`）收尾，而「不能」使用 delete ——
//   因為那塊記憶體不是 operator new 配置的。
//   std::vector 內部正是用這一組機制，才能讓 capacity 大於 size：
//   多出來的那段記憶體已配置，但上面還沒有任何物件存在。
//
// 【注意事項 Pay Attention】
// 1. new/delete 與 new[]/delete[] 混用是未定義行為，
//    標準不保證任何特定結果，不可描述成「一定會崩潰」。
//    本檔對此只以文字說明，不實際執行。
// 2. double free 與 use-after-free 同樣是未定義行為，本檔亦不執行。
// 3. delete 空指標是合法且無效果的，`if (p) delete p;` 中的判斷是多餘的。
// 4. 本檔「重點七」刻意保留四處記憶體洩漏作為錯誤示範，
//    程式仍可正常編譯執行；請對照輸出中沒有配對 [-] 的 [+]。
// 5. Linux 的記憶體超額配置（overcommit）會讓「new 成功」不等於
//    「這些記憶體真的都能用」；真正用不到時是 OOM killer 出手，
//    不會擲出 bad_alloc。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】new / delete
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. new 和 malloc 有什麼差別？
//     答：最本質的差別是 new 會呼叫建構函數、delete 會呼叫解構函數，
//         而 malloc/free 只負責配置與歸還原始記憶體，完全不碰物件的生死。
//         其次：new 回傳正確型別的指標（malloc 回傳 void* 需轉型）、
//         new 失敗擲出 bad_alloc（malloc 回傳 NULL）、
//         new 的大小由型別推導（malloc 要自己算 sizeof）。
//     追問：那可以 new 之後用 free，或 malloc 之後用 delete 嗎？
//         → 不行，都是未定義行為。free 不會呼叫解構函數；
//           而 delete 會對一塊從未被建構過的記憶體呼叫解構函數。
//           兩者的配置器內部結構也不保證相容。
//
// 🔥 Q2. 為什麼 new[] 一定要用 delete[] 釋放？
//     答：因為 delete[] 需要知道元素個數，才能對每個元素呼叫解構函數。
//         實作通常在配置區塊前面多存一個計數（cookie），delete[] 從那裡讀。
//         用 delete 釋放 new[] 的結果，解構函數只會被呼叫一次，
//         其餘元素的解構被跳過，歸還記憶體的起始位址也可能不對 ——
//         未定義行為。
//     追問：如果元素是 int 這種沒有解構函數的型別，混用是不是就沒事了？
//         → 實務上「可能」不會出事，但仍然是未定義行為，標準不保證。
//           而且這種「看起來沒事」的程式碼一旦哪天把 int 換成有解構函數的
//           類別，就會立刻變成真正的錯誤。不要依賴它。
//
// ⚠️ 陷阱. 「我在每個函式的結尾都認真寫了 delete，所以不會有記憶體洩漏。」
//     答：不成立。只要函式中間有任何一條 return，或是有任何一行可能擲出例外
//         （包括 new 本身、以及幾乎所有標準函式庫呼叫），
//         結尾那個 delete 就會被跳過。你守住的只是「正常路徑」這一條。
//     為什麼會錯：把函式想成「從上到下走一遍」的直線，
//         忽略了 return 與例外會造成多重離開路徑。
//         正確的心智模型是：函式有很多個出口，而你只在最後一個出口放了清潔工。
//         RAII 的價值就在於它把清潔工綁在「離開 scope」這個動作上，
//         不論從哪個出口走都會執行 —— 這正是本課從裸 new/delete
//         一路推導到 unique_ptr 的原因。
// ═══════════════════════════════════════════════════════════════════════════
//
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔不附
//   理由：本課主題是「動態記憶體的配置、釋放與所有權」，屬於資源管理機制。
//   LeetCode 評測演算法的輸入輸出正確性，單次執行結束即由 OS 回收，
//   不會因為用 new/delete 還是 unique_ptr 而有不同答案。
//   依規格「寧缺勿濫」從缺——本檔重點十的「武器工廠 WeaponFactory」
//   本身就是動態物件所有權管理的真實情境示範。
// -----------------------------------------------------------------------------

/*
 * ================================================================
 * 【第 19 課：動態對象的創建與銷毀（new / delete）】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. new 與 delete 的基本語法（基本型別 & 類別物件）
 * 2. new vs malloc 的本質差異（new = 分配 + 建構；malloc = 只分配）
 * 3. new[] 與 delete[]：動態陣列（基本型別 & 類別物件）
 * 4. new/delete 與 new[]/delete[] 不能混用（未定義行為）
 * 5. new 失敗時的處理（try-catch bad_alloc / nothrow nullptr）
 * 6. delete nullptr 是安全的，double delete 是未定義行為
 * 7. 記憶體洩漏的四種常見場景（忘記/覆蓋/提前return/異常）
 * 8. 解決方案：用局部對象管理動態記憶體（RAII：MyString 範例）
 * 9. 現代 C++ 建議：unique_ptr / make_unique 取代裸 new/delete
 * 10. 綜合範例：武器工廠 WeaponFactory
 * ================================================================
 */

#include <iostream>
#include <string>
#include <new>       // bad_alloc, nothrow
#include <memory>    // unique_ptr, make_unique
using namespace std;

// ================================================================
// 重點一：new 與 delete 的基本語法
// ================================================================
// new 做兩件事：① 分配記憶體  ② 調用建構函數
// delete 做兩件事：① 調用解構函數  ② 釋放記憶體
//
// 基本型別：
//   int* p1 = new int;        // 未初始化（垃圾值）
//   int* p2 = new int(42);    // 小括號初始化
//   int* p3 = new int{100};   // C++11 大括號初始化
//   delete p1;                // 釋放記憶體
//
// 類別物件：
//   Hero* h = new Hero("勇者", 10);  // 分配記憶體 + 調用建構函數
//   delete h;                         // 調用解構函數 + 釋放記憶體
//   h = nullptr;                      // 好習慣：避免懸空指標

class Hero {
private:
    string name;
    int level;

public:
    Hero(const string& n, int lv) : name(n), level(lv) {
        cout << "  [建構] " << name << " Lv." << level << endl;
    }

    ~Hero() {
        cout << "  [解構] " << name << " Lv." << level << endl;
    }

    void print() const {
        cout << "  " << name << " (Lv." << level << ")" << endl;
    }
};

// ================================================================
// 重點二：new vs malloc 的本質差異
// ================================================================
// ┌───────────┬──────────────────────────┬─────────────────────┐
// │           │ new / delete             │ malloc / free        │
// ├───────────┼──────────────────────────┼─────────────────────┤
// │ 分配記憶體 │ 是                       │ 是                   │
// │ 調用建構   │ 是                       │ 否！                 │
// │ 調用解構   │ 是（delete 時）          │ 否！（free 時）      │
// │ 返回型別   │ 正確的指標型別            │ void*（需強轉）      │
// │ 失敗行為   │ 拋出 bad_alloc           │ 返回 NULL            │
// └───────────┴──────────────────────────┴─────────────────────┘
// 結論：在 C++ 中，永遠不要用 malloc/free 管理類別物件！

class Widget {
private:
    string name;
    int* data;

public:
    Widget(const string& n) : name(n) {
        data = new int[10];   // 建構函數內部也可以用 new 分配資源
        cout << "  [建構] " << name << "：分配了內部資源" << endl;
    }

    ~Widget() {
        delete[] data;        // 解構函數負責釋放內部資源
        cout << "  [解構] " << name << "：釋放了內部資源" << endl;
    }
};

// ================================================================
// 重點三：new[] 與 delete[]：動態陣列
// ================================================================
// 基本型別陣列：
//   int* a = new int[5];                    // 未初始化
//   int* b = new int[5]();                  // 值初始化（全 0）
//   int* c = new int[5]{10,20,30,40,50};   // C++11 初始化列表
//   delete[] a;                             // 必須用 delete[]
//
// 類別物件陣列：
//   Soldier* s = new Soldier[3];   // 調用 3 次預設建構函數
//   delete[] s;                    // 調用 3 次解構函數 + 釋放記憶體

class Soldier {
private:
    int id;
    static int nextId;   // 靜態成員：所有物件共用的編號計數器

public:
    Soldier() : id(nextId++) {
        cout << "  [建構] 士兵 #" << id << endl;
    }

    ~Soldier() {
        cout << "  [解構] 士兵 #" << id << endl;
    }

    void report() const {
        cout << "  士兵 #" << id << " 報到！" << endl;
    }
};

int Soldier::nextId = 1;   // 靜態成員在類外初始化

// ================================================================
// 重點四：new/delete 與 new[]/delete[] 不能混用
// ================================================================
// ┌────────────────┬─────────────┬──────────────────┐
// │ 分配方式        │ 釋放方式     │ 結果              │
// ├────────────────┼─────────────┼──────────────────┤
// │ new T          │ delete p    │ 正確              │
// │ new T[n]       │ delete[] p  │ 正確              │
// │ new T[n]       │ delete p    │ 未定義行為！       │
// │ new T          │ delete[] p  │ 未定義行為！       │
// └────────────────┴─────────────┴──────────────────┘

class Item {
private:
    int id;
    static int nextItemId;

public:
    Item() : id(nextItemId++) {
        cout << "  [+] Item #" << id << endl;
    }

    ~Item() {
        cout << "  [-] Item #" << id << endl;
    }
};

int Item::nextItemId = 1;

// ================================================================
// 重點七：記憶體洩漏的四種常見場景
// ================================================================
// 場景 1：忘記 delete
// 場景 2：覆蓋指標（丟失原地址）
// 場景 3：提前 return（跳過 delete）
// 場景 4：異常中斷（跳過 delete）

class Resource {
private:
    string name;

public:
    Resource(const string& n) : name(n) {
        cout << "  [+] " << name << endl;
    }

    ~Resource() {
        cout << "  [-] " << name << endl;
    }
};

// --- 場景 1：忘記 delete ---
void leak1_forget_delete() {
    cout << "\n  --- 洩漏 1：忘記 delete ---" << endl;
    Resource* r = new Resource("被遺忘的資源");
    (void)r;   // 僅為避免 -Wunused-variable；刻意「不」delete，這就是洩漏本身
    // 忘記 delete r;
    // r 指向的記憶體永遠無法釋放！
}

// --- 場景 2：覆蓋指標 ---
void leak2_overwrite_pointer() {
    cout << "\n  --- 洩漏 2：覆蓋指標 ---" << endl;
    Resource* r = new Resource("第一個資源");
    r = new Resource("第二個資源");   // 第一個資源的地址丟失了！
    delete r;   // 只釋放了第二個，第一個永遠洩漏
}

// --- 場景 3：提前返回 ---
void leak3_early_return() {
    cout << "\n  --- 洩漏 3：提前返回 ---" << endl;
    Resource* r = new Resource("可能洩漏的資源");

    bool error = true;   // 模擬錯誤
    if (error) {
        cout << "  發生錯誤，提前返回！" << endl;
        return;           // 直接返回，忘記 delete！
    }

    delete r;   // 這行永遠不會執行
}

// --- 場景 4：異常中斷 ---
void leak4_exception() {
    cout << "\n  --- 洩漏 4：異常中斷 ---" << endl;
    Resource* r = new Resource("異常中洩漏的資源");

    throw runtime_error("模擬異常");   // 異常拋出，跳過了 delete

    delete r;   // 這行永遠不會執行
}

// ================================================================
// 重點八：RAII 解決方案——用局部對象管理動態記憶體
// ================================================================
// RAII = Resource Acquisition Is Initialization
// 核心思想：在建構函數中獲取資源，在解構函數中釋放資源。
// 不管函數怎麼離開（正常返回、提前返回、異常），解構函數都會被自動調用，
// 所以資源一定會被正確釋放，不會洩漏。

class MyString {
private:
    char* data;
    int length;

public:
    // 建構：分配記憶體，複製字串
    MyString(const char* str = "") {
        length = 0;
        while (str[length] != '\0') length++;

        data = new char[length + 1];   // +1 給 '\0'
        for (int i = 0; i <= length; i++) {
            data[i] = str[i];
        }

        cout << "  [建構] MyString: \"" << data << "\" (長度: "
             << length << ")" << endl;
    }

    // 解構：自動釋放記憶體，不會洩漏
    ~MyString() {
        cout << "  [解構] MyString: \"" << data << "\"" << endl;
        delete[] data;     // 自動清理！
        data = nullptr;
    }

    void print() const {
        cout << "  \"" << data << "\"" << endl;
    }

    int getLength() const { return length; }
};

// 不管怎麼離開這個函數，MyString 都會自動清理
void safeFunction(bool earlyReturn) {
    MyString greeting("Hello, C++!");

    if (earlyReturn) {
        cout << "  提前返回..." << endl;
        return;   // greeting 自動解構，記憶體自動釋放
    }

    greeting.print();
    // greeting 在函數結束時自動解構，不管是正常結束還是提前返回
}

// ================================================================
// 重點九：現代 C++ 建議——unique_ptr / make_unique
// ================================================================
// unique_ptr 是 C++11 引入的智能指標，它把 RAII 進一步封裝：
//   - make_unique<T>(args...) 自動調用 new
//   - unique_ptr 離開作用域時自動調用 delete
//   - 不需要手動 delete，異常安全
//   - 幾乎零開銷（和裸指標一樣快）

class Monster {
private:
    string name;

public:
    Monster(const string& n) : name(n) {
        cout << "  [+] " << name << " 出現" << endl;
    }

    ~Monster() {
        cout << "  [-] " << name << " 消失" << endl;
    }

    void roar() const {
        cout << "  " << name << " 吼叫！" << endl;
    }
};

// ================================================================
// 重點十：綜合範例——武器工廠 WeaponFactory
// ================================================================

class Weapon {
protected:
    string name;
    int damage;
    int* durability;   // 動態分配的耐久度

public:
    Weapon(const string& n, int dmg, int dur)
        : name(n), damage(dmg)
    {
        durability = new int(dur);   // 動態分配
        cout << "  [鍛造] " << name
             << " (攻擊:" << damage
             << ", 耐久:" << *durability << ")" << endl;
    }

    ~Weapon() {
        cout << "  [銷毀] " << name << endl;
        delete durability;           // 記得釋放！
    }

    void use() {
        if (*durability > 0) {
            (*durability) -= 10;
            cout << "  使用 " << name
                 << " 攻擊！(耐久: " << *durability << ")" << endl;
        } else {
            cout << "  " << name << " 已損壞！" << endl;
        }
    }

    void print() const {
        cout << "  " << name
             << " [攻擊:" << damage
             << " 耐久:" << *durability << "]" << endl;
    }
};

class WeaponFactory {
public:
    // 工廠方法：根據類型創建武器（裸 new 版本）
    static Weapon* create(const string& type) {
        if (type == "劍") {
            return new Weapon("鐵劍", 25, 100);
        } else if (type == "弓") {
            return new Weapon("長弓", 20, 80);
        } else if (type == "杖") {
            return new Weapon("法杖", 35, 60);
        } else {
            return new Weapon("木棍", 5, 200);
        }
    }

    // 現代版工廠方法：返回 unique_ptr，自動管理記憶體
    static unique_ptr<Weapon> createSafe(const string& type) {
        if (type == "劍") {
            return make_unique<Weapon>("鐵劍", 25, 100);
        } else if (type == "弓") {
            return make_unique<Weapon>("長弓", 20, 80);
        } else if (type == "杖") {
            return make_unique<Weapon>("法杖", 35, 60);
        } else {
            return make_unique<Weapon>("木棍", 5, 200);
        }
    }
};

// ================================================================
// main()：逐一展示所有重點
// ================================================================
int main() {
    cout << "=============================================" << endl;
    cout << "   第 19 課：動態對象的創建與銷毀（new / delete）" << endl;
    cout << "=============================================" << endl;

    // ============================================================
    // 【1】new 與 delete 的基本語法
    // ============================================================
    cout << "\n【1】new 與 delete 的基本語法" << endl;
    {
        // --- 基本型別 ---
        cout << "\n--- 基本型別 ---" << endl;
        int* p1 = new int;          // 分配一個 int（未初始化，可能是垃圾值）
        int* p2 = new int(42);      // 分配一個 int 並初始化為 42
        int* p3 = new int{100};     // C++11 大括號初始化

        // ⚠️ 刻意「不」印出 *p1 的值：p1 指向的 int 未被初始化，
        //    其值是不確定的（indeterminate）。對非 unsigned char 的型別而言，
        //    讀取不確定值本身就是未定義行為，而且每次執行結果都不同，
        //    教材也無法貼出穩定的預期輸出。這裡只陳述事實，不讀取它。
        cout << "  *p1 = (未初始化，值不確定，刻意不讀取)" << endl;
        cout << "  *p2 = " << *p2 << endl;
        cout << "  *p3 = " << *p3 << endl;

        delete p1;   // 釋放記憶體
        delete p2;
        delete p3;

        // --- 類別物件 ---
        cout << "\n--- 類別物件 ---" << endl;
        Hero* hero = new Hero("勇者", 10);   // new = 分配記憶體 + 調用建構函數
        hero->print();
        delete hero;                          // delete = 調用解構函數 + 釋放記憶體
        hero = nullptr;                       // 好習慣：避免懸空指標（dangling pointer）
    }

    // ============================================================
    // 【2】new vs malloc 的本質差異
    // ============================================================
    cout << "\n【2】new vs malloc 的本質差異" << endl;
    {
        // 正確：使用 new，會調用建構函數，內部資源被正確初始化
        cout << "\n--- 使用 new（正確）---" << endl;
        Widget* w1 = new Widget("正確的 Widget");
        delete w1;   // 解構函數被調用，內部資源被釋放

        // 錯誤示範（概念說明，不實際執行，因為會崩潰）
        cout << "\n--- 如果用 malloc（概念說明）---" << endl;
        cout << "  Widget* w2 = (Widget*)malloc(sizeof(Widget));" << endl;
        cout << "  // 記憶體分配了，但建構函數沒被調用！" << endl;
        cout << "  // name 沒被初始化，data 沒被分配" << endl;
        cout << "  // 使用 w2 會導致未定義行為！" << endl;
        cout << "  // free(w2) 也不會調用解構函數，data 洩漏！" << endl;
        cout << "  結論：C++ 中永遠不要用 malloc/free 管理類別物件" << endl;
    }

    // ============================================================
    // 【3】new[] 與 delete[]：動態陣列
    // ============================================================
    cout << "\n【3】new[] 與 delete[]：動態陣列" << endl;
    {
        // --- 基本型別陣列 ---
        cout << "\n--- 基本型別陣列 ---" << endl;
        int* nums  = new int[5];                       // 5 個 int，未初始化
        int* zeros = new int[5]();                     // 5 個 int，值初始化為 0
        int* init  = new int[5]{10, 20, 30, 40, 50};  // C++11 初始化列表

        // ⚠️ 同上：new int[5] 不會初始化元素，其值不確定。
        //    這裡不讀取它們，改以「有沒有被初始化」這個事實來對照下面兩行。
        cout << "  nums:  (5 個未初始化的 int，值不確定，刻意不讀取)" << endl;

        cout << "  zeros: ";
        for (int i = 0; i < 5; i++) cout << zeros[i] << " ";
        cout << endl;

        cout << "  init:  ";
        for (int i = 0; i < 5; i++) cout << init[i] << " ";
        cout << endl;

        delete[] nums;    // 必須用 delete[] 對應 new[]
        delete[] zeros;
        delete[] init;

        // --- 類別物件陣列 ---
        cout << "\n--- 類別物件陣列 ---" << endl;
        cout << "  創建 3 個士兵：" << endl;
        Soldier* squad = new Soldier[3];   // 調用 3 次預設建構函數

        cout << "\n  點名：" << endl;
        for (int i = 0; i < 3; i++) {
            squad[i].report();
        }

        cout << "\n  解散：" << endl;
        delete[] squad;   // 調用 3 次解構函數（逆序），然後釋放記憶體
    }

    // ============================================================
    // 【4】new/delete 與 new[]/delete[] 不能混用
    // ============================================================
    cout << "\n【4】new/delete 與 new[]/delete[] 不能混用" << endl;
    {
        // 正確配對：new 配 delete
        cout << "\n--- 正確配對 ---" << endl;
        Item* single = new Item;
        delete single;

        // 正確配對：new[] 配 delete[]
        Item* array = new Item[3];
        delete[] array;

        // 錯誤配對（千萬不要這樣做！僅用文字說明）
        cout << "\n--- 錯誤配對（千萬不要這樣做！）---" << endl;
        cout << "  // Item* p = new Item[3];" << endl;
        cout << "  // delete p;      <- 錯誤！應該用 delete[]" << endl;
        cout << "  // 後果：只解構第一個元素，其餘洩漏，或直接崩潰（未定義行為）" << endl;

        cout << "\n  // Item* q = new Item;" << endl;
        cout << "  // delete[] q;    <- 錯誤！應該用 delete" << endl;
        cout << "  // 後果：未定義行為，可能崩潰" << endl;
    }

    // ============================================================
    // 【5】new 失敗時的處理
    // ============================================================
    cout << "\n【5】new 失敗時的處理" << endl;
    {
        // --- 方式 1：try-catch 捕獲 bad_alloc 異常（標準方式）---
        cout << "\n--- 方式 1：try-catch ---" << endl;
        try {
            int* p = new int[100];   // 合理大小，正常成功
            cout << "  分配成功！" << endl;
            delete[] p;
        } catch (const bad_alloc& e) {
            cout << "  記憶體分配失敗: " << e.what() << endl;
        }

        // --- 方式 2：nothrow 版本（返回 nullptr）---
        cout << "\n--- 方式 2：nothrow ---" << endl;
        int* p2 = new(nothrow) int[100];   // 失敗時返回 nullptr，不拋異常
        if (p2 == nullptr) {
            cout << "  記憶體分配失敗（返回 nullptr）" << endl;
        } else {
            cout << "  分配成功！" << endl;
            delete[] p2;
        }

        // --- 實際的分配失敗示範 ---
        cout << "\n--- 嘗試分配超大記憶體（約 8TB）---" << endl;
        try {
            size_t hugeSize = 1000000000000ULL;   // 一定會失敗
            int* huge = new int[hugeSize];
            delete[] huge;   // 不會執行到這裡
        } catch (const bad_alloc& e) {
            cout << "  預期中的失敗: " << e.what() << endl;
        }
    }

    // ============================================================
    // 【6】delete nullptr 是安全的，double delete 是未定義行為
    // ============================================================
    cout << "\n【6】delete nullptr 與 double delete" << endl;
    {
        // delete nullptr 是安全的
        int* p = nullptr;
        delete p;          // 完全安全！不會崩潰，也不會有任何效果
        cout << "  delete nullptr 是安全的" << endl;

        int* arr = nullptr;
        delete[] arr;      // 也是安全的
        cout << "  delete[] nullptr 也是安全的" << endl;

        // double delete 是未定義行為（僅文字說明，不實際執行）
        cout << "\n  // int* q = new int(42);" << endl;
        cout << "  // delete q;   <- 第一次 OK" << endl;
        cout << "  // delete q;   <- 第二次：未定義行為！可能崩潰！" << endl;

        // 好習慣：delete 後設為 nullptr
        int* safe = new int(42);
        delete safe;
        safe = nullptr;    // 設為 nullptr
        delete safe;       // 再次 delete 也安全（因為是 nullptr）
        cout << "\n  好習慣：delete 後設為 nullptr，避免 double delete" << endl;
    }

    // ============================================================
    // 【7】記憶體洩漏的四種常見場景
    // ============================================================
    cout << "\n【7】記憶體洩漏的四種常見場景" << endl;
    {
        leak1_forget_delete();       // 場景 1：忘記 delete
        leak2_overwrite_pointer();   // 場景 2：覆蓋指標
        leak3_early_return();        // 場景 3：提前 return

        try {
            leak4_exception();       // 場景 4：異常中斷
        } catch (const exception& e) {
            cout << "  捕獲異常: " << e.what() << endl;
        }

        cout << "\n  注意：以上有多個資源只有 [+] 沒有 [-]，代表它們洩漏了！" << endl;
    }

    // ============================================================
    // 【8】RAII 解決方案：用局部對象管理動態記憶體（MyString）
    // ============================================================
    cout << "\n【8】RAII 解決方案：用局部對象管理動態記憶體" << endl;
    {
        // 正常流程：函數結束時自動解構
        cout << "\n--- 正常流程 ---" << endl;
        safeFunction(false);

        // 提前返回：提前返回時也自動解構，不會洩漏
        cout << "\n--- 提前返回 ---" << endl;
        safeFunction(true);

        // 異常安全：異常傳播時堆疊展開，自動解構
        cout << "\n--- 異常安全 ---" << endl;
        try {
            MyString msg("即將拋出異常");
            throw runtime_error("boom!");
            // msg 在異常傳播過程中自動解構（堆疊展開）
        } catch (...) {
            cout << "  異常已捕獲，記憶體已自動清理" << endl;
        }

        cout << "\n  RAII 的威力：不管正常返回、提前返回還是異常，資源都被正確釋放！" << endl;
    }

    // ============================================================
    // 【9】現代 C++ 建議：unique_ptr / make_unique
    // ============================================================
    cout << "\n【9】現代 C++ 建議：unique_ptr / make_unique" << endl;
    {
        // --- 舊方式：裸 new/delete ---
        cout << "\n--- 舊方式（裸指標）---" << endl;
        {
            Monster* m = new Monster("哥布林");
            m->roar();
            delete m;       // 必須手動 delete！忘記就洩漏
        }

        // --- 新方式：unique_ptr ---
        cout << "\n--- 新方式（unique_ptr）---" << endl;
        {
            // make_unique 自動調用 new
            // unique_ptr 離開作用域時自動 delete
            unique_ptr<Monster> m = make_unique<Monster>("史萊姆");
            m->roar();
            // 不需要 delete！unique_ptr 自動處理！
        }

        // --- 異常安全 ---
        cout << "\n--- 異常安全示範 ---" << endl;
        try {
            unique_ptr<Monster> m = make_unique<Monster>("龍");
            m->roar();
            throw runtime_error("勇者逃跑了！");
            // m 在堆疊展開時自動解構，不會洩漏
        } catch (const exception& e) {
            cout << "  " << e.what() << endl;
        }

        // --- 動態陣列（unique_ptr 版本）---
        cout << "\n--- 動態陣列（unique_ptr）---" << endl;
        {
            unique_ptr<int[]> arr = make_unique<int[]>(5);
            for (int i = 0; i < 5; i++) {
                arr[i] = (i + 1) * 10;
            }

            cout << "  ";
            for (int i = 0; i < 5; i++) {
                cout << arr[i] << " ";
            }
            cout << endl;
            // 不需要 delete[]！unique_ptr 自動處理！
        }
    }

    // ============================================================
    // 【10】綜合範例：武器工廠 WeaponFactory
    // ============================================================
    cout << "\n【10】綜合範例：武器工廠 WeaponFactory" << endl;
    {
        // --- 裸指標版本：手動管理 ---
        cout << "\n=== 裸指標版本：武器鍛造 ===" << endl;
        Weapon* sword = WeaponFactory::create("劍");
        Weapon* bow   = WeaponFactory::create("弓");
        Weapon* staff = WeaponFactory::create("杖");

        cout << "\n--- 戰鬥 ---" << endl;
        sword->use();
        sword->use();
        bow->use();
        staff->use();

        cout << "\n--- 武器狀態 ---" << endl;
        sword->print();
        bow->print();
        staff->print();

        // 必須手動 delete 每一個！
        cout << "\n--- 清理武器 ---" << endl;
        delete sword;
        delete bow;
        delete staff;

        // --- 動態陣列版本：指標陣列 ---
        cout << "\n=== 批量創建（指標陣列）===" << endl;
        const int BATCH_SIZE = 3;
        Weapon** armory = new Weapon*[BATCH_SIZE];   // 指標陣列

        armory[0] = WeaponFactory::create("劍");
        armory[1] = WeaponFactory::create("弓");
        armory[2] = WeaponFactory::create("杖");

        cout << "\n  武器庫清單：" << endl;
        for (int i = 0; i < BATCH_SIZE; i++) {
            armory[i]->print();
        }

        // 必須先 delete 每個 Weapon，再 delete[] 指標陣列
        cout << "\n--- 清空武器庫 ---" << endl;
        for (int i = 0; i < BATCH_SIZE; i++) {
            delete armory[i];     // 釋放每把武器
        }
        delete[] armory;           // 釋放指標陣列本身

        // --- 現代 C++ 版本：unique_ptr 工廠 ---
        cout << "\n=== 現代 C++ 版本（unique_ptr 工廠）===" << endl;
        {
            auto safeWeapon = WeaponFactory::createSafe("劍");
            safeWeapon->use();
            safeWeapon->print();
            // 不需要 delete！離開作用域自動清理
        }
    }

    // ============================================================
    // 本課重點回顧
    // ============================================================
    cout << "\n=============================================" << endl;
    cout << "本課重點回顧：" << endl;
    cout << "  1.  new 做兩件事：分配記憶體 + 調用建構函數" << endl;
    cout << "  2.  delete 做兩件事：調用解構函數 + 釋放記憶體" << endl;
    cout << "  3.  new vs malloc：new 調用建構函數，malloc 不會" << endl;
    cout << "  4.  new[] / delete[]：陣列版本，必須配對使用" << endl;
    cout << "  5.  不能混用：new 配 delete，new[] 配 delete[]" << endl;
    cout << "  6.  new 失敗：拋 bad_alloc，或用 nothrow 返回 nullptr" << endl;
    cout << "  7.  delete nullptr 安全，double delete 是未定義行為" << endl;
    cout << "  8.  記憶體洩漏四場景：忘記/覆蓋/提前return/異常" << endl;
    cout << "  9.  RAII：用類別的解構函數自動管理動態記憶體" << endl;
    cout << "  10. 現代 C++：用 unique_ptr / make_unique 取代裸 new/delete" << endl;
    cout << "=============================================" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// ── 輸出但書 ────────────────────────────────────────────────────────────
// 1. 本檔輸出逐位元組可重現（實測連跑 10 次 md5 相同）。
// 2.【本檔曾修正的一個真問題】原始版本會印出 `new int` 與 `new int[5]`
//    未初始化的內容（標示為「可能是垃圾值」）。那些是不確定值
//    （indeterminate value），對非 unsigned char 的型別而言，
//    讀取它本身就是未定義行為；實測連跑三次也確實得到三組不同的數字，
//    根本無法作為「預期輸出」。現已改為只陳述「未初始化、刻意不讀取」，
//    教學重點（new 不會初始化基本型別）完全保留，且輸出變得穩定。
//    對照組 `new int[5]()`（值初始化為 0）與 `new int[5]{...}`
//    仍照常印出，正好凸顯三者的差異。
// 3.「重點七」刻意保留四處記憶體洩漏作為錯誤示範：
//    請找出有 [+] 卻沒有對應 [-] 的資源，那些就是漏掉的物件，
//    它們的解構函數永遠不會被執行。
// 4. new/delete 與 new[]/delete[] 混用、double free、use-after-free
//    都是未定義行為，標準不保證任何結果；本檔全程僅以文字說明，不實際執行。
// 5. 本機環境：GCC 15.2.0 (Ubuntu 15.2.0-16ubuntu1) / libstdc++ / x86-64。

// === 預期輸出 ===
// =============================================
//    第 19 課：動態對象的創建與銷毀（new / delete）
// =============================================
//
// 【1】new 與 delete 的基本語法
//
// --- 基本型別 ---
//   *p1 = (未初始化，值不確定，刻意不讀取)
//   *p2 = 42
//   *p3 = 100
//
// --- 類別物件 ---
//   [建構] 勇者 Lv.10
//   勇者 (Lv.10)
//   [解構] 勇者 Lv.10
//
// 【2】new vs malloc 的本質差異
//
// --- 使用 new（正確）---
//   [建構] 正確的 Widget：分配了內部資源
//   [解構] 正確的 Widget：釋放了內部資源
//
// --- 如果用 malloc（概念說明）---
//   Widget* w2 = (Widget*)malloc(sizeof(Widget));
//   // 記憶體分配了，但建構函數沒被調用！
//   // name 沒被初始化，data 沒被分配
//   // 使用 w2 會導致未定義行為！
//   // free(w2) 也不會調用解構函數，data 洩漏！
//   結論：C++ 中永遠不要用 malloc/free 管理類別物件
//
// 【3】new[] 與 delete[]：動態陣列
//
// --- 基本型別陣列 ---
//   nums:  (5 個未初始化的 int，值不確定，刻意不讀取)
//   zeros: 0 0 0 0 0 
//   init:  10 20 30 40 50 
//
// --- 類別物件陣列 ---
//   創建 3 個士兵：
//   [建構] 士兵 #1
//   [建構] 士兵 #2
//   [建構] 士兵 #3
//
//   點名：
//   士兵 #1 報到！
//   士兵 #2 報到！
//   士兵 #3 報到！
//
//   解散：
//   [解構] 士兵 #3
//   [解構] 士兵 #2
//   [解構] 士兵 #1
//
// 【4】new/delete 與 new[]/delete[] 不能混用
//
// --- 正確配對 ---
//   [+] Item #1
//   [-] Item #1
//   [+] Item #2
//   [+] Item #3
//   [+] Item #4
//   [-] Item #4
//   [-] Item #3
//   [-] Item #2
//
// --- 錯誤配對（千萬不要這樣做！）---
//   // Item* p = new Item[3];
//   // delete p;      <- 錯誤！應該用 delete[]
//   // 後果：只解構第一個元素，其餘洩漏，或直接崩潰（未定義行為）
//
//   // Item* q = new Item;
//   // delete[] q;    <- 錯誤！應該用 delete
//   // 後果：未定義行為，可能崩潰
//
// 【5】new 失敗時的處理
//
// --- 方式 1：try-catch ---
//   分配成功！
//
// --- 方式 2：nothrow ---
//   分配成功！
//
// --- 嘗試分配超大記憶體（約 8TB）---
//   預期中的失敗: std::bad_alloc
//
// 【6】delete nullptr 與 double delete
//   delete nullptr 是安全的
//   delete[] nullptr 也是安全的
//
//   // int* q = new int(42);
//   // delete q;   <- 第一次 OK
//   // delete q;   <- 第二次：未定義行為！可能崩潰！
//
//   好習慣：delete 後設為 nullptr，避免 double delete
//
// 【7】記憶體洩漏的四種常見場景
//
//   --- 洩漏 1：忘記 delete ---
//   [+] 被遺忘的資源
//
//   --- 洩漏 2：覆蓋指標 ---
//   [+] 第一個資源
//   [+] 第二個資源
//   [-] 第二個資源
//
//   --- 洩漏 3：提前返回 ---
//   [+] 可能洩漏的資源
//   發生錯誤，提前返回！
//
//   --- 洩漏 4：異常中斷 ---
//   [+] 異常中洩漏的資源
//   捕獲異常: 模擬異常
//
//   注意：以上有多個資源只有 [+] 沒有 [-]，代表它們洩漏了！
//
// 【8】RAII 解決方案：用局部對象管理動態記憶體
//
// --- 正常流程 ---
//   [建構] MyString: "Hello, C++!" (長度: 11)
//   "Hello, C++!"
//   [解構] MyString: "Hello, C++!"
//
// --- 提前返回 ---
//   [建構] MyString: "Hello, C++!" (長度: 11)
//   提前返回...
//   [解構] MyString: "Hello, C++!"
//
// --- 異常安全 ---
//   [建構] MyString: "即將拋出異常" (長度: 18)
//   [解構] MyString: "即將拋出異常"
//   異常已捕獲，記憶體已自動清理
//
//   RAII 的威力：不管正常返回、提前返回還是異常，資源都被正確釋放！
//
// 【9】現代 C++ 建議：unique_ptr / make_unique
//
// --- 舊方式（裸指標）---
//   [+] 哥布林 出現
//   哥布林 吼叫！
//   [-] 哥布林 消失
//
// --- 新方式（unique_ptr）---
//   [+] 史萊姆 出現
//   史萊姆 吼叫！
//   [-] 史萊姆 消失
//
// --- 異常安全示範 ---
//   [+] 龍 出現
//   龍 吼叫！
//   [-] 龍 消失
//   勇者逃跑了！
//
// --- 動態陣列（unique_ptr）---
//   10 20 30 40 50 
//
// 【10】綜合範例：武器工廠 WeaponFactory
//
// === 裸指標版本：武器鍛造 ===
//   [鍛造] 鐵劍 (攻擊:25, 耐久:100)
//   [鍛造] 長弓 (攻擊:20, 耐久:80)
//   [鍛造] 法杖 (攻擊:35, 耐久:60)
//
// --- 戰鬥 ---
//   使用 鐵劍 攻擊！(耐久: 90)
//   使用 鐵劍 攻擊！(耐久: 80)
//   使用 長弓 攻擊！(耐久: 70)
//   使用 法杖 攻擊！(耐久: 50)
//
// --- 武器狀態 ---
//   鐵劍 [攻擊:25 耐久:80]
//   長弓 [攻擊:20 耐久:70]
//   法杖 [攻擊:35 耐久:50]
//
// --- 清理武器 ---
//   [銷毀] 鐵劍
//   [銷毀] 長弓
//   [銷毀] 法杖
//
// === 批量創建（指標陣列）===
//   [鍛造] 鐵劍 (攻擊:25, 耐久:100)
//   [鍛造] 長弓 (攻擊:20, 耐久:80)
//   [鍛造] 法杖 (攻擊:35, 耐久:60)
//
//   武器庫清單：
//   鐵劍 [攻擊:25 耐久:100]
//   長弓 [攻擊:20 耐久:80]
//   法杖 [攻擊:35 耐久:60]
//
// --- 清空武器庫 ---
//   [銷毀] 鐵劍
//   [銷毀] 長弓
//   [銷毀] 法杖
//
// === 現代 C++ 版本（unique_ptr 工廠）===
//   [鍛造] 鐵劍 (攻擊:25, 耐久:100)
//   使用 鐵劍 攻擊！(耐久: 90)
//   鐵劍 [攻擊:25 耐久:90]
//   [銷毀] 鐵劍
//
// =============================================
// 本課重點回顧：
//   1.  new 做兩件事：分配記憶體 + 調用建構函數
//   2.  delete 做兩件事：調用解構函數 + 釋放記憶體
//   3.  new vs malloc：new 調用建構函數，malloc 不會
//   4.  new[] / delete[]：陣列版本，必須配對使用
//   5.  不能混用：new 配 delete，new[] 配 delete[]
//   6.  new 失敗：拋 bad_alloc，或用 nothrow 返回 nullptr
//   7.  delete nullptr 安全，double delete 是未定義行為
//   8.  記憶體洩漏四場景：忘記/覆蓋/提前return/異常
//   9.  RAII：用類別的解構函數自動管理動態記憶體
//   10. 現代 C++：用 unique_ptr / make_unique 取代裸 new/delete
// =============================================
