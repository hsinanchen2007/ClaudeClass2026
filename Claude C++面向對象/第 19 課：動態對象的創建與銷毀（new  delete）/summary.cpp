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

        cout << "  *p1 = " << *p1 << " (可能是垃圾值)" << endl;
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

        cout << "  nums:  ";
        for (int i = 0; i < 5; i++) cout << nums[i] << " ";
        cout << "(可能是垃圾值)" << endl;

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
