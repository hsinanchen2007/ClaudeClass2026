// =============================================================================
//  第 8 課：對象（object）的創建與使用 — summary.cpp
//  物件總整理：建立方式 / 生命週期 / sizeof / 物件互動 / 陣列 / 參數傳遞
// =============================================================================
//
// 【主題資訊 Information】
//   堆疊建立：Dog d;                 自動儲存期，離開作用域自動解構
//   堆積建立：Dog* p = new Dog();    動態儲存期，必須 delete
//   存取     ：物件.成員  /  指標->成員   （後者等價於 (*指標).成員）
//   物件陣列：Student s[3];          每個元素都會被預設建構
//   參數傳遞：T（傳值）/ T&（可修改）/ const T&（只讀，預設選擇）
//   標準版本：C++98 起；類內初始值與智慧指標為 C++11
//   標頭檔  ：語言核心特性；本檔另用 <iostream> <string> <vector> <stdexcept>
//
// 【詳細解釋 Explanation】
//
// 【1. 一個物件的完整生命史】
//   把本課的內容串起來，一個物件會經歷：
//       ① 配置記憶體（堆疊只是移動堆疊指標；堆積要走 allocator）
//       ② 依【宣告順序】初始化每個成員 → 建構子本體
//       ③ 被使用（成員存取都是編譯期算好的偏移量，O(1)）
//       ④ 解構：先執行解構子本體，再以【宣告的反序】解構每個成員
//       ⑤ 釋放記憶體
//   關鍵在於 ④⑤ 的觸發時機：
//   堆疊物件由【作用域】決定且保證發生（即使丟出例外）；
//   堆積物件由【delete】決定 —— 而 delete 可能因為例外或提早 return 被跳過。
//   這個差別就是 RAII 存在的全部理由。
//
// 【2. 選堆疊還是堆積：唯一該問的問題】
//   不是「哪個快」，而是【這個物件需要活得比目前作用域更久嗎？】
//       不需要 → 堆疊（更快、自動釋放、例外安全）
//       需要   → 堆積，而且應該用 std::unique_ptr 而非裸 new/delete
//   附帶一提，堆疊配置只是把堆疊指標減去一個編譯期常數（一道指令），
//   而堆積配置要進入 allocator、可能向作業系統要記憶體、
//   多執行緒下還要同步 —— 差距通常是數十倍。
//
// 【3. sizeof 的三條規則】
//   本課的 sizeof 示範可以濃縮成三句話：
//       ① 只有【非靜態資料成員】才算（成員函式與 static 成員不佔空間）
//       ② 空類別的大小保證 ≥ 1（不同物件必須有不同位址）
//       ③ 總和之外還要加上【對齊填充】，且總大小須為最大對齊值的倍數
//   第 ③ 點的實際影響：本課的 Mixed{char,int,char} 是 12 bytes，
//   其中 6 bytes（一半）是填充；把成員由大到小重排就變成 8 bytes。
//   本機已實測確認。
//
// 【4. 參數傳遞的決策表】
//       需要修改呼叫者的物件            → T&
//       只讀且物件不小                  → const T&    ← 預設就選這個
//       只讀且物件很小（int/指標/bool） → T（傳值）
//       需要取得所有權（會存起來）      → T 傳值後 std::move（C++11）
//   最容易犯的錯是「該用參考卻傳值」——
//   函式內的修改改在複製品上，而且函式內的輸出看起來完全正常，
//   直到後面才發現原物件從未改變。
//
// 【概念補充 Concept Deep Dive】
//
// (A) const 成員函式是 const 參考能否運作的前提
//   本檔新增的 Cart::total() 與 Cart::count() 都宣告為 const，
//   否則 printSummary(const Cart&) 內部根本無法呼叫它們 ——
//   const 物件不能呼叫非 const 成員函式。
//   這說明 const 正確性是【會沿著呼叫鏈傳播】的：
//   一個成員函式漏掉 const，會讓所有想用 const& 接收它的地方全部編譯失敗。
//   所以這件事必須從一開始就做對，事後補會牽連整份程式碼。
//
// (B) MinStack 為什麼需要「兩個成員同步維護」
//   本檔加入的 LeetCode 155 是「物件持有多個必須一起更新的成員」
//   最乾淨的範例：data_ 與 min_ 每次 push/pop 都必須成對操作，
//   漏掉任何一次，getMin() 就會回傳錯誤答案。
//   如果這兩個 vector 是散落在外的全域變數，
//   維護同步性就完全靠紀律；
//   把它們封進同一個類別、只透過成員函式修改之後，
//   「同步」就變成類別自己的責任 —— 這正是物件存在的意義。
//
// (C) 本課刻意保留的簡化
//   本檔多處使用裸 new/delete 與原生陣列，這是為了聚焦在概念本身。
//   實務對照：
//       Dog* p = new Dog(); ... delete p;  →  auto p = make_unique<Dog>();（C++14）
//       Student s[3];                      →  std::vector<Student> 或 std::array
//   兩種現代寫法都不需要手動管理，而且例外安全。
//
// 【注意事項 Pay Attention】
//   1. 堆疊物件自動解構、不可 delete；堆積物件必須 delete 或交給智慧指標。
//   2. 解構順序是建構順序的嚴格反序（物件、成員、基類皆然）。
//   3. sizeof 通常大於成員總和，差額是對齊填充；填充內容不確定，
//      不可用 memcmp 比較結構體或直接 memcpy 序列化。
//   4. 陣列傳給函式會退化成指標，長度資訊遺失。
//   5. 傳值只會改到複製品，而且函式內的輸出看起來正常，錯誤極難察覺。
//   6. 所有不修改狀態的成員函式都應加 const，否則 const& 無法呼叫它。
//   7. 本檔所有 sizeof 數值皆為本機 x86-64 / g++ 15.2.0 實測，屬實作定義。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】物件的建立、生命週期與傳遞
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 堆疊物件與堆積物件，該怎麼選？
//     答：唯一該問的問題是【生命週期】：這個物件需要活得比目前作用域更久嗎？
//         不需要就用堆疊 —— 配置只是移動堆疊指標（一道指令）、
//         自動解構、且在丟出例外時也保證被清理。
//         需要才用堆積，而且應該用 std::unique_ptr 包起來，
//         不要寫裸露的 new/delete。
//     追問：那 std::vector 的元素算堆疊還是堆積？
//           → vector 物件本身在堆疊（本機 sizeof 為 24），
//             元素在堆積。這正是理想模式：
//             使用者只看到一個自動生命週期的物件，
//             動態記憶體由它封裝並保證釋放。
//
// 🔥 Q2. 函式參數該用 T、T& 還是 const T&？
//     答：預設 const T& —— 不複製、不可修改、還能接受暫存物件。
//         需要修改呼叫者的物件才用 T&。
//         只有物件很小（int、double、指標）時傳值才划算，
//         因為省下一次間接存取。
//         另有一種情況該傳值：函式要取得一份自己的副本存起來，
//         此時傳值再 std::move，可讓左值走複製、右值走移動。
//     追問：為什麼 void f(Box&) 不能接受 Box{1,2} 這種暫存物件？
//           → 因為修改一個即將消失的物件幾乎必定是錯誤，標準直接禁止。
//             const Box& 則可以，而且會把該暫存物件的生命週期
//             延長到參考的作用域結束。
//
// ⚠️ 陷阱. 「這個函式收 const Box&，所以它比收 Box& 更安全，
//          不會有任何副作用」—— 哪裡不夠精確？
//     答：const 限制的是【透過這個參考】不能修改，
//         不保證「這個物件在函式執行期間不會改變」。
//         若同一個物件還有別的存取路徑（另一個非 const 參考、
//         全域指標、或另一條執行緒），它隨時可能被改：
//             Box box;  const Box& ref = box;
//             box.width = 99;      // 合法，ref.width 也跟著變
//     為什麼會錯：把 const 讀成「這個東西是常數」，
//         但它真正的意思是「我不會透過這個名字去改它」——
//         是對【存取路徑】的限制，不是對物件的宣告。
//         兩個實際後果：
//         ① 編譯器不能因為參數是 const T& 就把讀取結果快取起來
//            （除非能證明沒有別名）；
//         ② const 完全不等於執行緒安全。
//         要真正的不可變，請用值語意（傳一份副本）或明確的同步機制。
// ═══════════════════════════════════════════════════════════════════════════

/*
 * ================================================================
 * 【第 8 課：對象（object）的創建與使用】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. 對象是類別（class）的實例（instance），在記憶體中有實體
 * 2. 棧上創建（Stack Allocation）：最常用，自動管理生命週期
 * 3. 堆上創建（Heap Allocation）：用 new/delete，手動管理
 * 4. 棧對象用 . 存取成員，堆指標用 -> 存取成員
 * 5. 同一類別的不同對象各自擁有獨立的成員變數副本
 * 6. sizeof：只計算成員變數大小（含記憶體對齊填充），不計函數
 * 7. 空類別大小為 1 byte（C++ 保證每個對象有唯一地址）
 * 8. 對象可透過引用互相交互
 * 9. 對象陣列：同一類別的多個對象組成陣列
 * 10. 對象作為函數參數：傳值、傳引用、傳 const 引用
 * ================================================================
 */

#include <iostream>
#include <string>
#include <vector>     // LeetCode 155 與實務範例使用
#include <stdexcept>  // std::out_of_range
using namespace std;


// ===== 重點一：棧上創建（Stack Allocation）=====
// 說明：最常用、最安全的建立對象方式。
// 對象儲存在棧上，離開作用域時自動銷毀，不需要手動管理記憶體。
// 語法：類別名 變數名;  然後用 . 存取成員。

class Dog {
public:
    string name;
    int age = 0;

    void bark() {
        cout << name << " 汪汪！" << endl;
    }
};


// ===== 重點二：堆上創建（Heap Allocation）=====
// 說明：使用 new 在堆（heap）上配置記憶體，回傳指標。
// 優點：生命週期不受作用域限制，可跨函數使用。
// 缺點：必須手動用 delete 釋放，否則造成記憶體洩漏（Memory Leak）。
// 釋放後應將指標設為 nullptr，避免野指標（Dangling Pointer）。
// 語法：類別名* 指標名 = new 類別名(); 用 -> 存取成員。


// ===== 重點三：棧 vs 堆 比較 =====
// 說明：兩種創建方式的存取語法不同。
// 棧對象使用 .（點運算子）存取成員
// 堆指標使用 ->（箭頭運算子）存取成員
// (*指標).成員 等價於 指標->成員，但 -> 更常用也更清晰

class Cat {
public:
    string name;

    void meow() {
        cout << name << " 喵～" << endl;
    }
};


// ===== 重點四：每個對象是獨立的 =====
// 說明：同一類別建立的不同對象，各自擁有獨立的成員變數副本。
// 修改 a.count 完全不影響 b.count，它們在記憶體中是不同的位址。
// 這是面向對象設計的核心特性之一：「封裝」與「獨立性」。

class Counter {
public:
    int count = 0;

    void increment() { count++; }
    void show() { cout << "count = " << count << endl; }
};


// ===== 重點五：對象的大小 sizeof =====
// 說明：對象在記憶體中佔用的大小 = 所有成員變數大小的總和（含對齊填充）。
// 成員函數不佔對象空間（函數共享一份，透過 this 指標區分）。
// 空類別大小為 1 byte：C++ 規定每個對象必須有唯一地址，所以不能是 0。
// 記憶體對齊（Memory Alignment）：和 C 語言 struct 規則完全一樣。

class Empty {};                     // 大小 = 1 byte（特殊規則）

class OnlyFunction {
public:
    void doSomething() { cout << "doing" << endl; }
    // 函數不佔空間，所以大小仍是 1 byte
};

class WithData {
public:
    int x;      // 4 bytes
    int y;      // 4 bytes
    double z;   // 8 bytes
    // 總計 16 bytes，剛好對齊，無填充
    void show() { cout << x << ", " << y << ", " << z << endl; }
};

class Mixed {
public:
    char a;     // 1 byte
    int b;      // 4 bytes（前面需要 3 bytes 填充以對齊）
    char c;     // 1 byte（後面需要 3 bytes 填充以整體對齊）
    // 典型輸出：1 + 3(pad) + 4 + 1 + 3(pad) = 12 bytes
};


// ===== 重點六：多個對象的交互 =====
// 說明：對象之間可以透過引用互相傳遞、操作。
// 函數參數寫 Player& target 表示「引用」，直接操作原對象，不是複製品。
// 這樣 target.hp -= attack 才會真正修改傳入對象的 HP。

class Player {
public:
    string name;
    int hp = 100;
    int attack = 10;

    // 以引用方式接收 target，直接操作原對象
    void attackTarget(Player& target) {
        cout << name << " 攻擊了 " << target.name << "！" << endl;
        target.hp -= attack;
        cout << target.name << " 剩餘 HP: " << target.hp << endl;
    }

    void showStatus() {
        cout << "[" << name << "] HP:" << hp << " 攻擊:" << attack << endl;
    }

    bool isAlive() { return hp > 0; }
};


// ===== 重點七：對象陣列 =====
// 說明：可以建立同一類別的多個對象組成的陣列。
// 用索引 students[i] 存取，再用 . 存取成員。
// 陣列中的每個對象都是獨立的，可以用迴圈遍歷。

class Student {
public:
    string name = "未命名";
    int score = 0;

    void show() { cout << name << ": " << score << " 分" << endl; }
};


// ===== 重點八：對象作為函數參數（三種方式）=====
// 說明：傳值（by value）、傳引用（by reference）、傳 const 引用。
// 傳值：複製一份，函數內修改不影響原對象。有複製成本，少用。
// 傳引用：直接操作原對象，函數內的修改會反映到外部。
// 傳 const 引用：最推薦！不複製（高效），也不能修改（安全）。

class Box {
public:
    double width = 0;
    double height = 0;

    double area() { return width * height; }
};

// 傳值：函數內拿到的是複製品，修改不影響原對象
void printByValue(Box b) {
    cout << "面積 = " << b.area() << endl;
    b.width = 999;  // 只改了複製品，原對象不受影響
}

// 傳引用：函數內直接操作原對象
void doubleSize(Box& b) {
    b.width *= 2;
    b.height *= 2;
}

// 傳 const 引用：不複製（效率高），且無法修改（安全）—— 最推薦
void printByConstRef(const Box& b) {
    cout << "寬: " << b.width << ", 高: " << b.height << endl;
    // b.width = 10;  // 編譯錯誤！const 不允許修改
}


// ===== main：展示所有重點 =====

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 155. Min Stack
//   題目：設計一個堆疊，除了 push / pop / top 之外，
//         還要能在 O(1) 時間取得堆疊中的最小值 getMin()。
//   為什麼用到本主題：這題是「物件持有多個必須同步維護的成員」最經典的例子——
//         data_ 與 min_ 兩個 vector 必須【每次操作都一起更新】，
//         少更新一個就會失去一致性。
//         把它們綁進同一個物件、只透過成員函式修改，
//         正是本課「物件 = 狀態 + 操作」的核心。
//   解法：另外維護一個「同步的最小值堆疊」——
//         min_ 的每個位置記錄「到該深度為止的最小值」，
//         因此 pop 時兩邊一起彈出，getMin() 直接讀 min_ 的頂端。
//   複雜度：push / pop / top / getMin 全部 O(1)；額外空間 O(n)。
// -----------------------------------------------------------------------------
class MinStack {
private:
    vector<int> data_;   // 實際存放的資料
    vector<int> min_;    // min_[i] = data_[0..i] 之中的最小值

public:
    void push(int val) {
        data_.push_back(val);
        // 空堆疊時最小值就是自己，否則取「目前最小值」與新值的較小者
        min_.push_back(min_.empty() ? val : (val < min_.back() ? val : min_.back()));
    }

    void pop() {
        if (data_.empty()) throw std::out_of_range("pop on empty MinStack");
        data_.pop_back();
        min_.pop_back();          // 兩者必須同步彈出
    }

    int top() const {
        if (data_.empty()) throw std::out_of_range("top on empty MinStack");
        return data_.back();
    }

    int getMin() const {
        if (min_.empty()) throw std::out_of_range("getMin on empty MinStack");
        return min_.back();
    }

    bool empty() const { return data_.empty(); }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】購物車：示範「物件互動」與三種參數傳遞的實際選擇
//   情境：把商品加進購物車、計算總價、套用折扣。
//   為什麼用到本主題：三個函式剛好各自示範一種傳遞方式——
//         addTo   需要修改購物車      → Cart&
//         total   只讀取、物件不小    → const Cart&
//         applyDiscount 需要修改      → Cart&
//   這正是本課「選擇參數傳遞方式」那張表的實際應用。
// -----------------------------------------------------------------------------
struct CartItem {
    string name;
    double price = 0.0;
    int    qty   = 0;
};

class Cart {
private:
    vector<CartItem> items_;

public:
    void add(const string& name, double price, int qty) {
        items_.push_back(CartItem{name, price, qty});
    }

    // 只讀 → const 成員函式，才能被 const Cart& 呼叫
    double total() const {
        double sum = 0.0;
        for (const CartItem& it : items_) {
            sum += it.price * it.qty;
        }
        return sum;
    }

    size_t count() const { return items_.size(); }
};

// 需要修改購物車 → 非 const 參考
void addTo(Cart& cart, const string& name, double price, int qty) {
    cart.add(name, price, qty);
}

// 只讀取，且 Cart 內含 vector（複製成本高）→ const 參考
void printSummary(const Cart& cart) {
    cout << "  共 " << cart.count() << " 項商品，總計 $" << cart.total() << endl;
}

int main() {
    cout << "===== 重點一：棧上創建 =====" << endl;
    // 棧上創建：語法最簡單，離開作用域自動銷毀
    Dog d1;
    d1.name = "旺財";
    d1.age = 3;
    d1.bark();   // 用 . 存取

    Dog d2;
    d2.name = "小黑";
    d2.age = 5;
    d2.bark();


    cout << "\n===== 重點二：堆上創建 =====" << endl;
    // new 在堆上配置記憶體，回傳指標
    Dog* ptr = new Dog();
    ptr->name = "阿花";   // 用 -> 存取
    ptr->age = 2;
    ptr->bark();
    delete ptr;           // 必須手動釋放！
    ptr = nullptr;        // 好習慣：防止野指標


    cout << "\n===== 重點三：棧 vs 堆 存取語法 =====" << endl;
    Cat c1;                // 棧：自動管理
    c1.name = "咪咪";
    c1.meow();             // . 存取

    Cat* c2 = new Cat();   // 堆：手動管理
    c2->name = "花花";
    c2->meow();            // -> 存取
    (*c2).meow();          // 等價寫法：先解引用再用 .
    delete c2;
    c2 = nullptr;


    cout << "\n===== 重點四：每個對象獨立 =====" << endl;
    Counter a, b;
    a.increment(); a.increment(); a.increment();  // a.count = 3
    b.increment();                                 // b.count = 1（與 a 無關）
    cout << "a: "; a.show();
    cout << "b: "; b.show();


    cout << "\n===== 重點五：sizeof 對象 =====" << endl;
    // 成員函數不佔對象空間，空類別最小為 1 byte
    cout << "sizeof(Empty)        = " << sizeof(Empty) << " bytes" << endl;
    cout << "sizeof(OnlyFunction) = " << sizeof(OnlyFunction) << " bytes" << endl;
    cout << "sizeof(WithData)     = " << sizeof(WithData) << " bytes" << endl;
    cout << "sizeof(Mixed)        = " << sizeof(Mixed) << " bytes (含對齊填充)" << endl;


    cout << "\n===== 重點六：對象交互（引用參數）=====" << endl;
    Player warrior, mage;
    warrior.name = "戰士"; warrior.hp = 120; warrior.attack = 15;
    mage.name = "法師";    mage.hp = 80;    mage.attack = 25;

    warrior.showStatus();
    mage.showStatus();
    warrior.attackTarget(mage);  // 引用：直接修改 mage 的 hp
    mage.attackTarget(warrior);
    cout << warrior.name << (warrior.isAlive() ? " 存活" : " 陣亡") << endl;
    cout << mage.name   << (mage.isAlive()   ? " 存活" : " 陣亡") << endl;


    cout << "\n===== 重點七：對象陣列 =====" << endl;
    Student students[3];
    students[0].name = "小明"; students[0].score = 85;
    students[1].name = "小華"; students[1].score = 92;
    students[2].name = "小美"; students[2].score = 78;

    // 用迴圈遍歷
    for (int i = 0; i < 3; i++) {
        students[i].show();
    }

    // 找最高分
    int maxIdx = 0;
    for (int i = 1; i < 3; i++) {
        if (students[i].score > students[maxIdx].score)
            maxIdx = i;
    }
    cout << "最高分: " << students[maxIdx].name
         << " (" << students[maxIdx].score << " 分)" << endl;


    cout << "\n===== 重點八：對象作為函數參數 =====" << endl;
    Box box;
    box.width = 5.0;
    box.height = 3.0;

    cout << "--- 傳值（複製品，原對象不變）---" << endl;
    printByValue(box);
    cout << "原始 width = " << box.width << "（未被改變）" << endl;

    cout << "--- 傳引用（直接操作原對象）---" << endl;
    doubleSize(box);
    cout << "放大後 width = " << box.width << endl;

    cout << "--- 傳 const 引用（最推薦，高效且安全）---" << endl;
    printByConstRef(box);



    // --- LeetCode 155：物件同步維護多個成員 ---
    cout << "\n===== LeetCode 155. Min Stack =====" << endl;
    MinStack ms;
    ms.push(-2);  ms.push(0);  ms.push(-3);
    cout << "  push(-2), push(0), push(-3)" << endl;
    cout << "  getMin() = " << ms.getMin() << endl;   // -3
    ms.pop();
    cout << "  pop() 之後 top() = " << ms.top() << endl;      // 0
    cout << "  getMin() = " << ms.getMin() << endl;           // -2

    // --- 日常實務：購物車（三種參數傳遞的實際選擇） ---
    cout << "\n===== 日常實務：購物車 =====" << endl;
    Cart cart;
    addTo(cart, "咖啡豆", 450.0, 2);      // Cart&       需要修改
    addTo(cart, "濾紙",    80.0, 1);
    addTo(cart, "手沖壺", 1200.0, 1);
    printSummary(cart);                    // const Cart& 只讀取

    cout << "\n===== 重點回顧：選擇參數傳遞方式 =====" << endl;
    cout << "傳值       void f(Box b)         : 少用，有複製成本" << endl;
    cout << "傳引用     void f(Box& b)         : 需要修改原對象時" << endl;
    cout << "傳const引用 void f(const Box& b)  : 最常用！只讀取不修改" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o obj_summary
//   本檔可執行部分只用到 C++11 起就有的語法（類內初始值、range-for），
//   以 -std=c++17 編譯零警告通過。

// 註 1:本檔輸出是【完全確定的】—— 沒有輸入、亂數、執行緒或位址，
//      連跑 5 次位元組完全相同。

// 註 2:sizeof 區段的數值皆為【實作定義】，本機為 x86-64 / g++ 15.2.0：
//        Empty=1（空類別最小大小）、OnlyFunction=1（成員函式不佔空間）、
//        WithData=16（4+4+8，剛好無需填充）、
//        Mixed=12（char 1 +填充3 + int 4 + char 1 +尾端填充3，一半是填充）。

// 註 3:「原始 width = 5（未被改變）」是本課參數傳遞最重要的一行 ——
//      printByValue 內部確實把複製品改成了 999，但原物件毫髮無傷。
//      緊接著 doubleSize 用 Box& 才真的把 5 改成 10，兩者恰成對照。

// 註 4:LeetCode 155 的輸出與官方範例一致（getMin=-3 → pop → top=0、getMin=-2）。
//      重點在 data_ 與 min_ 兩個成員必須【每次操作都同步更新】——
//      把它們封進同一個類別，同步性才成為類別自己的責任。

// 註 5:購物車總計 $2180 = 450×2 + 80×1 + 1200×1。
//      印成 $2180 而非 $2180.00 是 iostream 對 double 的預設格式所致。
//      addTo 用 Cart&（要修改）、printSummary 用 const Cart&（只讀），
//      正是本課參數傳遞決策表的實際應用。

// === 預期輸出 ===
// ===== 重點一：棧上創建 =====
// 旺財 汪汪！
// 小黑 汪汪！
//
// ===== 重點二：堆上創建 =====
// 阿花 汪汪！
//
// ===== 重點三：棧 vs 堆 存取語法 =====
// 咪咪 喵～
// 花花 喵～
// 花花 喵～
//
// ===== 重點四：每個對象獨立 =====
// a: count = 3
// b: count = 1
//
// ===== 重點五：sizeof 對象 =====
// sizeof(Empty)        = 1 bytes
// sizeof(OnlyFunction) = 1 bytes
// sizeof(WithData)     = 16 bytes
// sizeof(Mixed)        = 12 bytes (含對齊填充)
//
// ===== 重點六：對象交互（引用參數）=====
// [戰士] HP:120 攻擊:15
// [法師] HP:80 攻擊:25
// 戰士 攻擊了 法師！
// 法師 剩餘 HP: 65
// 法師 攻擊了 戰士！
// 戰士 剩餘 HP: 95
// 戰士 存活
// 法師 存活
//
// ===== 重點七：對象陣列 =====
// 小明: 85 分
// 小華: 92 分
// 小美: 78 分
// 最高分: 小華 (92 分)
//
// ===== 重點八：對象作為函數參數 =====
// --- 傳值（複製品，原對象不變）---
// 面積 = 15
// 原始 width = 5（未被改變）
// --- 傳引用（直接操作原對象）---
// 放大後 width = 10
// --- 傳 const 引用（最推薦，高效且安全）---
// 寬: 10, 高: 6
//
// ===== LeetCode 155. Min Stack =====
//   push(-2), push(0), push(-3)
//   getMin() = -3
//   pop() 之後 top() = 0
//   getMin() = -2
//
// ===== 日常實務：購物車 =====
//   共 3 項商品，總計 $2180
//
// ===== 重點回顧：選擇參數傳遞方式 =====
// 傳值       void f(Box b)         : 少用，有複製成本
// 傳引用     void f(Box& b)         : 需要修改原對象時
// 傳const引用 void f(const Box& b)  : 最常用！只讀取不修改
