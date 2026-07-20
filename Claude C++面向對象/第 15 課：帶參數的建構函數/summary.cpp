// =============================================================================
//  第 15 課：帶參數的建構函數  —  總複習 summary.cpp
// =============================================================================
//
// 【主題資訊 Information】
//   涵蓋      : 傳參方式、成員同名、五種初始化語法、窄化、explicit、預設參數
//   標準版本  : 主體 C++98；{} 統一初始化與 explicit 用於多參數為 **C++11**；
//               guaranteed copy elision 為 **C++17**
//   標頭檔    : <iostream>、<string>、<vector>
//   一句話    : 帶參 constructor 的工作是「把外界給的原料，變成一個合法的物件」，
//               而本課談的全是這個轉換過程中的成本、歧義與安全性。
//
// 【詳細解釋 Explanation】
//
// 【1. 傳參方式：為什麼 const string& 幾乎總是對的】
//   值傳遞 string 會發生兩次複製：
//       Student(string n) { name = n; }
//       ① 實參 → 參數 n（若實參是左值，複製一次）
//       ② n → 成員 name（operator= 又複製一次）
//   改成 const string& 省掉 ①；再改用初始化列表 : name(n) 讓 ② 從
//   「賦值」變成「copy construction」，少一次可能的重新配置。
//   準則：
//       * 內建型別（int、double、指標）→ 直接值傳遞，反而更快
//       * class type → const T&
//       * 確定要「取得一份自己的副本」且呼叫端常傳暫時物件
//         → 值傳遞 + std::move（sink parameter 慣用法）
//
// 【2. const& 為什麼能綁定字面值與暫時物件】
//       Demo(string& s)        → Demo d("hello");   編譯失敗
//       Demo(const string& s)  → Demo d("hello");   OK
//   因為 "hello" 會先建立一個暫時的 std::string（prvalue），
//   而 non-const lvalue reference 不能綁定到暫時物件——
//   這個規則存在的理由是：允許它等於允許你「修改一個馬上就要消失的東西」，
//   那幾乎一定是 bug。const& 則會把暫時物件的生命週期延長到 reference 的作用域結束。
//   實務結論：**唯讀的參數一律用 const&**，它同時接受左值、右值、字面值。
//
// 【3. 參數與成員同名：四種解法與它們的取捨】
//   錯誤示範 name = name; 是把參數賦值給自己，成員完全沒被碰到
//   （GCC 需要 -Wshadow 才會提醒，預設不警告）。四種解法：
//       (a) this->name = name;      → 明確，但仍是「賦值」不是初始化
//       (b) 參數加前綴 _name        → 有些風格指南禁止（_ 開頭在某些情境是保留識別字）
//       (c) 成員加 m_ 或後綴 name_  → Google Style 用後綴，很多公司用 m_ 前綴
//       (d) 初始化列表 : name(name) → **最推薦**
//   (d) 之所以不衝突，是因為初始化列表的語法規定：
//       **括號外的名稱一定解析為成員，括號內的依一般作用域規則（優先是參數）**。
//   所以 : name(name) 讀作「用參數 name 初始化成員 name」，毫無歧義。
//
// 【4. 五種初始化語法與它們的差異】
//       Point p1(3.0, 4.0);          // direct-initialization
//       Point p2{5.0, 6.0};          // direct-list-initialization（禁止窄化）
//       Point p3 = Point(7.0, 8.0);  // copy-initialization（C++17 起保證省略複製）
//       Point p4 = {9.0, 10.0};      // copy-list-initialization（explicit 會擋掉）
//       Point* p5 = new Point(1, 2); // 動態配置
//   實務上的選擇準則：
//       * 一般情況用 {}：禁止窄化，且不會踩到 most vexing parse
//       * 但若類別有 initializer_list constructor，{} 會**強烈優先**選它
//         （vector<int> v{3, 1} 得到 {3,1} 而非「3 個 1」）——此時要用 ()
//
// 【5. explicit：擋在哪一層】
//   explicit 擋掉的是 **copy-initialization** 這個情境：
//       Distance d = 50.0;      // 擋
//       showDistance(200.0);    // 擋（函式引數也是 copy-init）
//       return 50.0;            // 擋（回傳值也是）
//   它**不擋** direct-initialization：
//       SafeDistance d(50.0);   // 仍可用
//       SafeDistance d{50.0};   // 仍可用
//   準則：**單一引數可呼叫的 constructor 預設就加 explicit**，
//   除非那個隱式轉換正是設計意圖（std::string 接受 const char* 就是刻意的）。
//
// 【概念補充 Concept Deep Dive】
//   ▍成員初始化順序永遠是宣告順序
//     不是初始化列表的書寫順序。若成員之間有依賴（用 A 算 B），
//     宣告順序就必須刻意安排。GCC/Clang 用 -Wreorder 提醒兩者不一致，
//     但它不會告訴你「你讀到了未初始化的成員」。
//
//   ▍窄化在 GCC 的預設診斷並不一致（本機 GCC 15.2 實測）
//     Holder h{3.14};（double→int）    → 預設就是 **error**
//     int n=300; char c{n};（int→char）→ 預設只是 **warning** [-Wnarrowing]
//     標準規定兩者都是 ill-formed；要完全依標準辦事請加 -pedantic-errors。
//
//   ▍C++17 guaranteed copy elision
//     Point p3 = Point(7.0, 8.0); 在 C++17 之前是「建立暫時物件再複製，
//     但編譯器可以省略」；C++17 起變成語言保證——prvalue 直接就地初始化目標，
//     中間根本沒有暫時物件，因此連 copy constructor 被 delete 的型別都能這樣寫。
//
// 【注意事項 Pay Attention】
//   1. name = name; 是把參數賦給自己，成員沒被初始化；預設不會有警告（需 -Wshadow）。
//   2. 成員初始化順序 = 宣告順序，不是初始化列表的書寫順序。
//   3. {} 禁止窄化，但 GCC 對部分情況預設只給警告——請加 -pedantic-errors。
//   4. explicit 擋 copy-init，不擋 direct-init。
//   5. 類別若有 initializer_list constructor，{} 會強烈優先選它。
//   6. 預設參數只能從右往左省略；virtual function 不要用預設參數（靜態繫結）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】帶參數的建構函數 綜合
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. constructor 的參數該用值傳遞還是 const&？講清楚判準。
//     答：內建型別（int、double、指標）用值傳遞，反而比傳參考快；
//         class type 用 const T&，避免呼叫時的複製。
//         若這個參數注定要被「存進成員」且呼叫端常傳暫時物件，
//         可用「值傳遞 + std::move」的 sink parameter 慣用法，
//         讓右值只發生移動、左值只發生一次複製。
//     追問：為什麼 const& 能接字面值而 T& 不行？→ 字面值會產生暫時物件（prvalue），
//         non-const lvalue reference 不能綁定暫時物件；允許它等於允許你修改
//         一個馬上要消失的東西。const& 還會把暫時物件的生命週期延長到自己的作用域。
//
// 🔥 Q2. : name(name) 這種「括號外成員、括號內參數」為什麼不會衝突？
//     答：因為初始化列表的語法有明確規定：**括號外的名稱一定解析為成員**，
//         括號內的依一般作用域規則（參數優先）。所以它明確讀作
//         「用參數 name 初始化成員 name」。這也是四種同名解法中最推薦的一種——
//         既不用改命名風格，又是真正的「初始化」而非「賦值」。
//     追問：那 constructor 本體裡寫 name = name; 呢？→ 兩個 name 都是參數，
//         成員完全沒被碰到。而且預設不會有任何警告，要開 -Wshadow 才提醒。
//
// 🔥 Q3. explicit 到底擋掉了什麼？什麼時候不該加？
//     答：擋掉 copy-initialization：T x = v;、傳函式引數、return 值。
//         不擋 direct-initialization：T x(v);、T x{v};。
//         單一引數可呼叫的 constructor 預設就該加，
//         除非那個隱式轉換本來就是設計意圖——例如 std::string 接受
//         const char*、std::function 接受 lambda，那些是刻意保留的便利性。
//     追問：多參數的 constructor 加 explicit 有意義嗎？→ 有（C++11 起）。
//         因為 {} 讓多參數也能參與隱式轉換：draw({1,2}) 在沒有 explicit 時合法。
//
// ⚠️ 陷阱. 成員宣告順序是 x, y，卻寫成 : y(b), x(y * 2)，會發生什麼？
//     答：x 會先被初始化（因為它宣告在前），此時 y **尚未初始化**，
//         於是 x 拿到一個不確定的值。編譯器會給 -Wreorder 警告說順序不一致，
//         但那個警告的措辭不會告訴你「你讀到了未初始化的成員」。
//     為什麼會錯：把初始化列表當成「由左到右依序執行的敘述」。
//         它只是「為每個成員指定初始化方式」的對照表，
//         真正的執行順序永遠由**宣告順序**決定。
// ═══════════════════════════════════════════════════════════════════════════

/*
 * ================================================================
 * 【第 15 課：帶參數的建構函數】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. 值傳遞 vs const 引用傳遞（推薦 const 引用）
 * 2. const 引用可以綁定字面值和臨時對象（普通引用不行）
 * 3. 參數名與成員變數同名的問題及四種解決方案
 * 4. 初始化列表中同名也不衝突
 * 5. 帶參建構函數的五種調用語法
 * 6. 大括號初始化禁止窄化轉換（Narrowing Conversion）
 * 7. 單參數建構函數的隱式轉換 & explicit 關鍵字
 * 8. 預設參數的進階用法
 * ================================================================
 */

#include <iostream>
#include <string>
using namespace std;

// ================================================================
// 重點一：值傳遞 vs const 引用傳遞
// ================================================================
// 對於基本型別（int、double、bool 等）：直接值傳遞即可，開銷很小。
// 對於類別型別（string、vector 等）：推薦 const 引用傳遞，避免不必要的複製。
//
// ┌──────────────────┬─────────────────────────────────────┐
// │ 傳遞方式          │ 說明                                 │
// ├──────────────────┼─────────────────────────────────────┤
// │ 值傳遞 string n   │ 呼叫時複製一次 + 賦值時又複製一次     │
// │ const string& n   │ 不複製，只在賦值給成員時複製一次（推薦）│
// │ string& n         │ 不能接受字面值和臨時對象（不推薦）     │
// └──────────────────┴─────────────────────────────────────┘

class StudentValue {
private:
    string name;
    int age;

public:
    // 值傳遞：每次調用都複製 string
    StudentValue(string n, int a) {
        name = n;    // 這裡又複製了一次
        age = a;
    }

    void print() const {
        cout << "  [值傳遞] " << name << ", " << age << " 歲" << endl;
    }
};

class StudentRef {
private:
    string name;
    int age;

public:
    // const 引用傳遞（推薦）：不複製 string，只傳遞引用
    StudentRef(const string& n, int a) {
        name = n;    // 只在這裡複製一次
        age = a;
    }

    void print() const {
        cout << "  [const引用] " << name << ", " << age << " 歲" << endl;
    }
};

// ================================================================
// 重點二：const 引用可以綁定字面值和臨時對象
// ================================================================
// 普通引用（string&）不能綁定到字面值 "hello" 或臨時對象 string("temp")
// const 引用（const string&）可以！
//
// Demo(string& s)       → Demo d("hello");     // 編譯錯誤！
// Demo(const string& s) → Demo d("hello");     // OK！

class Demo {
public:
    string data;
    Demo(const string& s) { data = s; }
};

// ================================================================
// 重點三：參數名與成員變數同名的問題
// ================================================================
// 錯誤示範：
//   BadExample(string name, int age) {
//       name = name;   // 把參數 name 賦給參數 name 自己！成員沒被修改！
//       age = age;     // 同理！
//   }

// --- 解決方案 1：使用 this 指針 ---
class Solution_This {
private:
    string name;
    int age;

public:
    Solution_This(string name, int age) {
        this->name = name;   // this->name 是成員，name 是參數
        this->age = age;
    }

    void print() const {
        cout << "  [this] " << name << ", " << age << " 歲" << endl;
    }
};

// --- 解決方案 2A：參數加底線前綴 ---
class Solution_UnderscorePrefix {
private:
    string name;
    int age;

public:
    Solution_UnderscorePrefix(const string& _name, int _age) {
        name = _name;
        age = _age;
    }
};

// --- 解決方案 2B：成員變數加 m_ 前綴（很多公司採用）---
class Solution_MPrefix {
private:
    string m_name;
    int m_age;

public:
    Solution_MPrefix(const string& name, int age) {
        m_name = name;
        m_age = age;
    }
};

// --- 解決方案 2C：成員變數加底線後綴（Google C++ 風格指南）---
class Solution_Suffix {
private:
    string name_;
    int age_;

public:
    Solution_Suffix(const string& name, int age) {
        name_ = name;
        age_ = age;
    }
};

// --- 解決方案 3：使用初始化列表（最推薦！）---
// 初始化列表中，括號外是成員變數，括號內是參數，即使同名也不衝突！
class Solution_InitList {
private:
    string name;
    int age;

public:
    Solution_InitList(const string& name, int age)
        : name(name), age(age) { }   // 簡潔且不衝突！

    void print() const {
        cout << "  [初始化列表] " << name << ", " << age << " 歲" << endl;
    }
};

// ================================================================
// 重點四：帶參建構函數的五種調用語法
// ================================================================
//   語法 1：直接初始化（括號）     Point p1(3.0, 4.0);
//   語法 2：統一初始化（大括號）   Point p2{5.0, 6.0};
//   語法 3：拷貝初始化（等號）     Point p3 = Point(7.0, 8.0);
//   語法 4：等號 + 大括號          Point p4 = {9.0, 10.0};
//   語法 5：動態分配               Point* p5 = new Point(1.0, 2.0);

class Point {
private:
    double x, y;

public:
    Point(double x, double y) : x(x), y(y) {
        cout << "  建構 Point(" << x << ", " << y << ")" << endl;
    }

    void print() const {
        cout << "  (" << x << ", " << y << ")" << endl;
    }
};

// ================================================================
// 重點五：大括號初始化禁止窄化轉換
// ================================================================
// 小括號允許窄化轉換（如 double → int，截斷為整數部分）
// 大括號禁止窄化轉換（會產生編譯錯誤）
//
// Holder h1(3.14);     // OK：double → int，截斷為 3
// Holder h2{3.14};     // 編譯錯誤！禁止窄化
// Holder h3{int(3.14)};// OK：明確轉換

class Holder {
public:
    int value;
    Holder(int v) : value(v) { }
};

// ================================================================
// 重點六：單參數建構函數的隱式轉換 & explicit
// ================================================================
// 單參數建構函數默認允許隱式轉換：
//   Distance d = 50.0;       // double 隱式轉換為 Distance
//   showDistance(200.0);      // 函數參數中也會隱式轉換
//
// 用 explicit 禁止隱式轉換：
//   explicit SafeDistance(double m) : meters(m) { }
//   SafeDistance d = 50.0;    // 編譯錯誤！
//   SafeDistance d(50.0);     // OK：直接初始化

class Distance {
private:
    double meters;

public:
    // 沒有 explicit → 允許隱式轉換
    Distance(double m) : meters(m) {
        cout << "  建構 Distance: " << meters << " m" << endl;
    }

    void print() const {
        cout << "  距離: " << meters << " 公尺" << endl;
    }
};

void showDistance(Distance d) {
    d.print();
}

class SafeDistance {
private:
    double meters;

public:
    // explicit → 禁止隱式轉換
    explicit SafeDistance(double m) : meters(m) {
        cout << "  建構 SafeDistance: " << meters << " m" << endl;
    }

    void print() const {
        cout << "  距離: " << meters << " 公尺" << endl;
    }
};

void showSafeDistance(SafeDistance d) {
    d.print();
}

// ================================================================
// 重點七：預設參數的進階用法
// ================================================================
// 預設參數必須從右到左依次提供，不能跳過中間的參數。
// 一個典型用法：只有第一個參數是必需的，其餘都有合理預設值。

class HttpRequest {
private:
    string url;
    string method;
    int timeout;
    bool followRedirect;

public:
    // 只有 url 是必需的
    HttpRequest(const string& url,
                const string& method = "GET",
                int timeout = 30,
                bool followRedirect = true)
        : url(url), method(method), timeout(timeout),
          followRedirect(followRedirect) { }

    void print() const {
        cout << "  [" << method << "] " << url
             << " (timeout=" << timeout << "s"
             << ", redirect=" << (followRedirect ? "是" : "否")
             << ")" << endl;
    }
};

// ================================================================
// 綜合範例：遊戲角色
// ================================================================

class GameCharacter {
private:
    string name_;         // Google 風格：底線後綴
    string classType_;
    int level_;
    int hp_;
    int mp_;
    double attackPower_;

public:
    // explicit + const 引用 + 預設參數
    explicit GameCharacter(const string& name,
                           const string& classType = "戰士",
                           int level = 1)
        : name_(name), classType_(classType), level_(level)
    {
        // 根據職業設定基礎屬性
        if (classType_ == "戰士") {
            hp_ = 150 + level_ * 20;
            mp_ = 30 + level_ * 5;
            attackPower_ = 15.0 + level_ * 3.0;
        } else if (classType_ == "法師") {
            hp_ = 80 + level_ * 10;
            mp_ = 100 + level_ * 15;
            attackPower_ = 25.0 + level_ * 5.0;
        } else if (classType_ == "弓箭手") {
            hp_ = 100 + level_ * 12;
            mp_ = 50 + level_ * 8;
            attackPower_ = 20.0 + level_ * 4.0;
        } else {
            hp_ = 100 + level_ * 15;
            mp_ = 50 + level_ * 10;
            attackPower_ = 15.0 + level_ * 3.5;
        }
    }

    void printStatus() const {
        cout << "  ┌──────────────────────────┐" << endl;
        cout << "  │ " << name_ << " [" << classType_ << "]" << endl;
        cout << "  │ 等級: " << level_ << endl;
        cout << "  │ HP: " << hp_ << "  MP: " << mp_ << endl;
        cout << "  │ 攻擊力: " << attackPower_ << endl;
        cout << "  └──────────────────────────┘" << endl;
    }
};

// ================================================================
// 座標類別：展示 explicit 和窄化防護
// ================================================================
class Coordinate {
private:
    int x_, y_;

public:
    explicit Coordinate(int x, int y) : x_(x), y_(y) { }

    void print() const {
        cout << "  (" << x_ << ", " << y_ << ")" << endl;
    }
};


// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 707. Design Linked List
//   題目：實作單向鏈結串列，支援 get / addAtHead / addAtTail / addAtIndex / deleteAtIndex。
//   為什麼用到本主題：這題的節點型別 Node 是「帶參數的 constructor」最單純也最典型的
//         用途——Node(int val, Node* next) 一次把兩個欄位設好，
//         用初始化列表 : val_(val), next_(next) 直接建構而非先預設再賦值。
//         鏈結串列的節點若沒在建構時就接好指標，很容易留下懸空的 next_，
//         這正是「讓物件一出生就合法」的價值。
//   複雜度：get / addAtIndex / deleteAtIndex 為 O(n)，addAtHead 為 O(1)。
// -----------------------------------------------------------------------------
class MyLinkedList {
private:
    struct Node {
        int   val_;
        Node* next_;
        // 帶參 constructor + 初始化列表：next 預設為 nullptr，避免懸空指標
        Node(int val, Node* next = nullptr) : val_(val), next_(next) {}
    };

    Node* head_ = nullptr;
    int   size_ = 0;

public:
    MyLinkedList() = default;

    ~MyLinkedList() {                       // 自己配置就自己釋放
        while (head_) { Node* nx = head_->next_; delete head_; head_ = nx; }
    }
    // 這個示範類別持有裸指標，故禁止複製，避免 double free（Rule of Three）
    MyLinkedList(const MyLinkedList&)            = delete;
    MyLinkedList& operator=(const MyLinkedList&) = delete;

    int get(int index) const {
        if (index < 0 || index >= size_) return -1;
        const Node* p = head_;
        for (int i = 0; i < index; ++i) p = p->next_;
        return p->val_;
    }

    void addAtHead(int val) {
        head_ = new Node(val, head_);       // 建構時就把 next 接好
        ++size_;
    }

    void addAtTail(int val) { addAtIndex(size_, val); }

    void addAtIndex(int index, int val) {
        if (index > size_ || index < 0) return;
        if (index == 0) { addAtHead(val); return; }
        Node* prev = head_;
        for (int i = 0; i < index - 1; ++i) prev = prev->next_;
        prev->next_ = new Node(val, prev->next_);
        ++size_;
    }

    void deleteAtIndex(int index) {
        if (index < 0 || index >= size_) return;
        if (index == 0) { Node* d = head_; head_ = head_->next_; delete d; --size_; return; }
        Node* prev = head_;
        for (int i = 0; i < index - 1; ++i) prev = prev->next_;
        Node* d = prev->next_;
        prev->next_ = d->next_;
        delete d;
        --size_;
    }

    void dump() const {
        cout << "    [";
        for (const Node* p = head_; p; p = p->next_)
            cout << p->val_ << (p->next_ ? " -> " : "");
        cout << "]  size=" << size_ << endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】不可變的稽核事件（AuditEvent）
//   情境：稽核日誌一旦寫下就不該再被修改——這是合規要求，不是風格偏好。
//   作法：所有成員宣告為 const，於是它們**只能**在成員初始化列表初始化
//         （const 成員無法在 constructor 本體賦值，那會直接編譯失敗）。
//         這讓「不可變」從一個口頭約定，變成編譯器強制的規則。
//   附帶效果：有 const 成員的類別無法被 assign（copy assignment 會被隱式刪除），
//         所以它也不能放進需要 assign 的容器操作（例如 vector::erase 中間元素後的搬移）。
//         這是「不可變」要付出的代價，設計時要意識到。
// -----------------------------------------------------------------------------
class AuditEvent {
private:
    const long   timestamp_;      // const 成員 → 只能用初始化列表
    const string actor_;
    const string action_;
    const string target_;

public:
    AuditEvent(long ts, const string& actor,
               const string& action, const string& target)
        : timestamp_(ts), actor_(actor), action_(action), target_(target) {}
        // 若改成本體內 timestamp_ = ts; → 編譯失敗（assignment of read-only member）

    void print() const {
        cout << "    [" << timestamp_ << "] " << actor_
             << " " << action_ << " " << target_ << endl;
    }
};

int main() {
    cout << "=============================================" << endl;
    cout << "   第 15 課：帶參數的建構函數" << endl;
    cout << "=============================================" << endl;

    // --- 重點一：值傳遞 vs const 引用 ---
    cout << "\n【1】值傳遞 vs const 引用傳遞" << endl;
    string myName = "張三";
    StudentValue sv(myName, 20);     // 值傳遞：複製兩次
    sv.print();
    StudentRef sr(myName, 20);       // const 引用：只複製一次
    sr.print();

    // --- 重點二：const 引用可綁定字面值 ---
    cout << "\n【2】const 引用可綁定字面值和臨時對象" << endl;
    Demo d1("直接傳字串");            // const 引用 OK！
    Demo d2(string("臨時字串"));      // const 引用 OK！
    cout << "  d1: " << d1.data << endl;
    cout << "  d2: " << d2.data << endl;

    // --- 重點三：參數名與成員同名的解決方案 ---
    cout << "\n【3】參數名與成員同名的解決方案" << endl;

    Solution_This st("王五", 25);
    st.print();

    Solution_InitList si("趙六", 30);
    si.print();

    cout << "  四種命名風格：" << endl;
    cout << "    A: 參數加底線前綴  → _name, _age" << endl;
    cout << "    B: 成員加 m_ 前綴  → m_name, m_age" << endl;
    cout << "    C: 成員加底線後綴  → name_, age_ (Google 風格)" << endl;
    cout << "    D: 使用 this 指針  → this->name = name" << endl;
    cout << "    最推薦：初始化列表  → : name(name), age(age)" << endl;

    // --- 重點四：五種調用語法 ---
    cout << "\n【4】帶參建構函數的五種調用語法" << endl;

    cout << "語法 1：直接初始化 Point p1(3.0, 4.0);" << endl;
    Point p1(3.0, 4.0);
    p1.print();

    cout << "語法 2：統一初始化 Point p2{5.0, 6.0};" << endl;
    Point p2{5.0, 6.0};
    p2.print();

    cout << "語法 3：拷貝初始化 Point p3 = Point(7.0, 8.0);" << endl;
    Point p3 = Point(7.0, 8.0);
    p3.print();

    cout << "語法 4：等號+大括號 Point p4 = {9.0, 10.0};" << endl;
    Point p4 = {9.0, 10.0};
    p4.print();

    cout << "語法 5：動態分配 Point* p5 = new Point(1.0, 2.0);" << endl;
    Point* p5 = new Point(1.0, 2.0);
    p5->print();
    delete p5;

    // --- 重點五：大括號禁止窄化轉換 ---
    cout << "\n【5】大括號初始化禁止窄化轉換" << endl;
    double pi = 3.14;
    Holder h1(pi);        // OK：double → int，截斷為 3
    // Holder h2{pi};     // 編譯錯誤！大括號禁止窄化
    Holder h3{3};          // OK：int → int，沒有窄化
    cout << "  h1.value = " << h1.value << " (double 3.14 截斷為 3)" << endl;
    cout << "  h3.value = " << h3.value << " (int 3，無窄化)" << endl;

    // --- 重點六：隱式轉換 & explicit ---
    cout << "\n【6】隱式轉換 & explicit" << endl;

    cout << "Distance（允許隱式轉換）：" << endl;
    Distance dist1(100.0);
    dist1.print();
    Distance dist2 = 50.0;     // 隱式轉換！（copy-initialization）
    dist2.print();             // 實際用它一下，避免 -Wunused-but-set-variable
    showDistance(200.0);        // 函數參數也是 copy-init → 同樣會隱式轉換

    cout << "SafeDistance（explicit 禁止隱式轉換）：" << endl;
    SafeDistance sd1(100.0);              // OK：直接初始化
    // SafeDistance sd2 = 50.0;           // 編譯錯誤！explicit 禁止
    // showSafeDistance(200.0);           // 編譯錯誤！explicit 禁止
    showSafeDistance(SafeDistance(200.0)); // OK：明確轉換

    // --- 重點七：預設參數 ---
    cout << "\n【7】預設參數的進階用法" << endl;
    HttpRequest r1("https://example.com");
    r1.print();

    HttpRequest r2("https://api.example.com/data", "POST");
    r2.print();

    HttpRequest r3("https://slow-server.com", "GET", 120);
    r3.print();

    HttpRequest r4("https://redirect.com", "GET", 10, false);
    r4.print();

    // --- 綜合範例：遊戲角色 ---
    cout << "\n【8】綜合範例：遊戲角色" << endl;
    GameCharacter hero1("勇者小明");                   // 預設職業和等級
    GameCharacter hero2("暗影法師", "法師");           // 預設等級
    GameCharacter hero3("神射手", "弓箭手", 10);       // 全部指定

    hero1.printStatus();
    hero2.printStatus();
    hero3.printStatus();

    // --- explicit + 窄化防護 ---
    cout << "\n【9】Coordinate (explicit + 窄化防護)" << endl;
    Coordinate c1(10, 20);         // OK：直接初始化
    Coordinate c2{30, 40};         // OK：大括號初始化
    // Coordinate c3 = {50, 60};   // 錯誤！explicit 禁止拷貝列表初始化
    // Coordinate c4{3.7, 4.2};    // 錯誤！double→int 是窄化
    Coordinate c5{int(3.7), int(4.2)};  // OK：明確轉換
    c1.print();
    c2.print();
    c5.print();

    // --- 陣列中使用帶參建構函數 ---
    cout << "\n【10】陣列中使用帶參建構函數" << endl;
    GameCharacter party[3] = {
        GameCharacter("坦克", "戰士", 5),
        GameCharacter("治癒者", "法師", 3),
        GameCharacter("輸出手", "弓箭手", 7)
    };
    for (int i = 0; i < 3; i++) {
        party[i].printStatus();
    }

    // --- 重點回顧 ---
    cout << "\n=============================================" << endl;
    cout << "本課重點回顧：" << endl;
    cout << "  1. 類別型別參數推薦用 const 引用傳遞，避免不必要的複製" << endl;
    cout << "  2. const 引用可以綁定字面值和臨時對象" << endl;
    cout << "  3. 參數同名問題：推薦用初始化列表 : name(name) 解決" << endl;
    cout << "  4. 五種調用語法：()、{}、= T()、= {}、new T()" << endl;
    cout << "  5. 大括號 {} 禁止窄化轉換，比小括號更安全" << endl;
    cout << "  6. explicit 禁止單參數建構的隱式轉換" << endl;
    cout << "  7. 預設參數從右到左提供，讓 API 更靈活" << endl;
    cout << "  8. Google 風格：成員加底線後綴 name_、age_" << endl;
    cout << "=============================================" << endl;

    // --- LeetCode 707. Design Linked List ---
    cout << "\n【11】LeetCode 707. Design Linked List" << endl;
    MyLinkedList list;
    list.addAtHead(1);
    list.addAtTail(3);
    list.addAtIndex(1, 2);       // 變成 1 -> 2 -> 3
    list.dump();
    cout << "    get(1) = " << list.get(1) << "   (期望 2)" << endl;
    list.deleteAtIndex(1);       // 變成 1 -> 3
    list.dump();
    cout << "    get(1) = " << list.get(1) << "   (期望 3)" << endl;
    cout << "    get(9) = " << list.get(9) << "  (超出範圍，期望 -1)" << endl;
    cout << "    節點的 Node(val, next) 讓指標在建構時就接好，不會留下懸空的 next_" << endl;

    // --- 日常實務：不可變的稽核事件 ---
    cout << "\n【12】日常實務：const 成員只能用初始化列表" << endl;
    AuditEvent e1(1700000000, "alice", "DELETE", "orders/1001");
    AuditEvent e2(1700000042, "bob",   "UPDATE", "users/77");
    e1.print();
    e2.print();
    cout << "    所有成員都是 const → 寫成 timestamp_ = ts; 會編譯失敗：" << endl;
    cout << "      error: assignment of read-only member 'AuditEvent::timestamp_'" << endl;
    cout << "    「不可變」因此成為編譯器強制的規則，而不是口頭約定。" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary
//   （建議另加 -pedantic-errors，讓標準規定的窄化規則被確實執行）

// === 預期輸出 ===
// =============================================
//    第 15 課：帶參數的建構函數
// =============================================
//
// 【1】值傳遞 vs const 引用傳遞
//   [值傳遞] 張三, 20 歲
//   [const引用] 張三, 20 歲
//
// 【2】const 引用可綁定字面值和臨時對象
//   d1: 直接傳字串
//   d2: 臨時字串
//
// 【3】參數名與成員同名的解決方案
//   [this] 王五, 25 歲
//   [初始化列表] 趙六, 30 歲
//   四種命名風格：
//     A: 參數加底線前綴  → _name, _age
//     B: 成員加 m_ 前綴  → m_name, m_age
//     C: 成員加底線後綴  → name_, age_ (Google 風格)
//     D: 使用 this 指針  → this->name = name
//     最推薦：初始化列表  → : name(name), age(age)
//
// 【4】帶參建構函數的五種調用語法
// 語法 1：直接初始化 Point p1(3.0, 4.0);
//   建構 Point(3, 4)
//   (3, 4)
// 語法 2：統一初始化 Point p2{5.0, 6.0};
//   建構 Point(5, 6)
//   (5, 6)
// 語法 3：拷貝初始化 Point p3 = Point(7.0, 8.0);
//   建構 Point(7, 8)
//   (7, 8)
// 語法 4：等號+大括號 Point p4 = {9.0, 10.0};
//   建構 Point(9, 10)
//   (9, 10)
// 語法 5：動態分配 Point* p5 = new Point(1.0, 2.0);
//   建構 Point(1, 2)
//   (1, 2)
//
// 【5】大括號初始化禁止窄化轉換
//   h1.value = 3 (double 3.14 截斷為 3)
//   h3.value = 3 (int 3，無窄化)
//
// 【6】隱式轉換 & explicit
// Distance（允許隱式轉換）：
//   建構 Distance: 100 m
//   距離: 100 公尺
//   建構 Distance: 50 m
//   距離: 50 公尺
//   建構 Distance: 200 m
//   距離: 200 公尺
// SafeDistance（explicit 禁止隱式轉換）：
//   建構 SafeDistance: 100 m
//   建構 SafeDistance: 200 m
//   距離: 200 公尺
//
// 【7】預設參數的進階用法
//   [GET] https://example.com (timeout=30s, redirect=是)
//   [POST] https://api.example.com/data (timeout=30s, redirect=是)
//   [GET] https://slow-server.com (timeout=120s, redirect=是)
//   [GET] https://redirect.com (timeout=10s, redirect=否)
//
// 【8】綜合範例：遊戲角色
//   ┌──────────────────────────┐
//   │ 勇者小明 [戰士]
//   │ 等級: 1
//   │ HP: 170  MP: 35
//   │ 攻擊力: 18
//   └──────────────────────────┘
//   ┌──────────────────────────┐
//   │ 暗影法師 [法師]
//   │ 等級: 1
//   │ HP: 90  MP: 115
//   │ 攻擊力: 30
//   └──────────────────────────┘
//   ┌──────────────────────────┐
//   │ 神射手 [弓箭手]
//   │ 等級: 10
//   │ HP: 220  MP: 130
//   │ 攻擊力: 60
//   └──────────────────────────┘
//
// 【9】Coordinate (explicit + 窄化防護)
//   (10, 20)
//   (30, 40)
//   (3, 4)
//
// 【10】陣列中使用帶參建構函數
//   ┌──────────────────────────┐
//   │ 坦克 [戰士]
//   │ 等級: 5
//   │ HP: 250  MP: 55
//   │ 攻擊力: 30
//   └──────────────────────────┘
//   ┌──────────────────────────┐
//   │ 治癒者 [法師]
//   │ 等級: 3
//   │ HP: 110  MP: 145
//   │ 攻擊力: 40
//   └──────────────────────────┘
//   ┌──────────────────────────┐
//   │ 輸出手 [弓箭手]
//   │ 等級: 7
//   │ HP: 184  MP: 106
//   │ 攻擊力: 48
//   └──────────────────────────┘
//
// =============================================
// 本課重點回顧：
//   1. 類別型別參數推薦用 const 引用傳遞，避免不必要的複製
//   2. const 引用可以綁定字面值和臨時對象
//   3. 參數同名問題：推薦用初始化列表 : name(name) 解決
//   4. 五種調用語法：()、{}、= T()、= {}、new T()
//   5. 大括號 {} 禁止窄化轉換，比小括號更安全
//   6. explicit 禁止單參數建構的隱式轉換
//   7. 預設參數從右到左提供，讓 API 更靈活
//   8. Google 風格：成員加底線後綴 name_、age_
// =============================================
//
// 【11】LeetCode 707. Design Linked List
//     [1 -> 2 -> 3]  size=3
//     get(1) = 2   (期望 2)
//     [1 -> 3]  size=2
//     get(1) = 3   (期望 3)
//     get(9) = -1  (超出範圍，期望 -1)
//     節點的 Node(val, next) 讓指標在建構時就接好，不會留下懸空的 next_
//
// 【12】日常實務：const 成員只能用初始化列表
//     [1700000000] alice DELETE orders/1001
//     [1700000042] bob UPDATE users/77
//     所有成員都是 const → 寫成 timestamp_ = ts; 會編譯失敗：
//       error: assignment of read-only member 'AuditEvent::timestamp_'
//     「不可變」因此成為編譯器強制的規則，而不是口頭約定。
