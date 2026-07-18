/*=============================================================================
 * 檔名：8_CopyConstructor.cpp
 * 主題：複製建構子 (Copy Constructor) - 物件被「複製」時發生的事
 * 適合：已會建構子，想知道「物件 = 另一個物件」這行背後的故事的人
 *
 * 【課題介紹】
 *   想像你有一個物件 a，現在做了：
 *
 *       Foo b = a;     // 用 a 來建立 b
 *       Foo c(a);      // 同上，另一種寫法
 *
 *   這時候 b 跟 c 「不是」剛被預設建構出來、再被指派的；它們是
 *   一誕生就帶著 a 的資料一起出現的。背後呼叫的就是「複製建構子」。
 *
 *       Foo(const Foo& other);     ← 複製建構子的標準形式
 *
 *   - 參數型別是「const ClassName&」(常見參考)，
 *     之所以是 reference 而不是 value，是為了「避免無限遞迴」：
 *     如果參數用 value 傳入，那參數本身又要做一次 copy → 又呼叫 copy ctor → 死循環。
 *
 * 【什麼時候會被呼叫？】
 *   1. 用一個物件去建構另一個物件：       Foo b = a;       Foo c(a);
 *   2. 物件被「按值傳入」函式：           void f(Foo x);   f(a);
 *   3. 物件被「按值回傳」 (在某些情境下，現代 C++ 多半會被 RVO 優化掉)
 *   4. 標準容器內複製元素 (例如 push_back 一個物件)
 *
 *   注意「賦值 (assignment)」：
 *       Foo b;        // 預設建構 b
 *       b = a;        // ← 這是「賦值」，呼叫的是「賦值運算子」(下一篇 9)
 *   不是複製建構子！只有「物件誕生那一刻」用另一個物件來填它，才是複製建構。
 *
 * 【預設複製建構子做什麼？】
 *   你不寫的話，編譯器會自動幫你產生一個「member-wise copy」(逐成員複製) 版本。
 *   對 int / double 是直接複製值；對指標只複製「地址」 → 這就是淺拷貝陷阱來源。
 *
 *   - 淺拷貝 (Shallow copy)：複製指標位址，原物件與副本指向同一塊資源。
 *   - 深拷貝 (Deep copy)：另外配置一塊資源，把資料整份複製過去，互不影響。
 *
 *   用原始指標 (raw pointer) 自己管 heap 記憶體時，淺拷貝會踩雷：
 *     - 兩個物件解構時會 delete 同一塊記憶體 → 雙重釋放 (double free)，當機
 *     - 改一個物件的內容會影響另一個 → 邏輯錯誤
 *
 *   ※ 用 std::string / std::vector 這類「會自己管理資源的型別」時，
 *     預設的逐成員複製就已經是深拷貝了，不用煩惱。
 *
 * 【日常實用範例】
 *   寫一個極簡的 MyString，內部用 char* 管自己的記憶體，
 *   分別示範「沒寫」與「有寫」複製建構子兩種版本，理解差別。
 *
 * 【對應 Leetcode】1480. Running Sum of 1d Array
 *   為什麼選這題：很適合示範「把一個有狀態的物件複製出去」會發生什麼事。
 *   我們把累加器寫成類別，然後複製它，觀察兩個物件的狀態是「獨立」還是「共享」。
 *   結論：成員都是 int / std::vector 等 RAII 型別 → 預設 copy ctor 就會深拷貝，
 *   兩物件的狀態完全獨立 (這是 Rule of Zero 的好處，第 23 篇會深入)。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/language/copy_constructor
 *   https://cplusplus.com/doc/tutorial/classes2/
 *=============================================================================*/

/*
補充筆記：CopyConstructor
  - CopyConstructor 這類 OOP 範例要追蹤物件狀態：建構後是否有效、操作後是否仍符合類別承諾。
  - 如果類別擁有資源，就要檢查 destructor、copy、move 是否表達同一套所有權規則。
  - 繼承、friend、static、operator overload 都應服務於清楚的物件語意，而不是只展示語法。
  - copy constructor 形式通常是 Class(const Class& other)，用既有物件建立新物件；它不是賦值，因為目標物件此時還沒存在。
  - 若所有成員都能正確複製，編譯器產生的 copy constructor 通常足夠；手寫版本多半是因為類別擁有裸資源或需要特殊語意。
  - 淺拷貝會只複製指標值，兩個物件指向同一塊資源；若兩者都在解構時 delete，會造成 double delete。
  - 深拷貝會配置新資源並複製內容，讓兩個物件互相獨立；代價是較慢，但符合「值語意」直覺。
  - 參數必須用 reference，通常加 const；若寫成 Class other 會為了傳參數又呼叫 copy constructor，造成無窮遞迴。
  - copy elision 和 return value optimization 可能讓你看不到複製發生；不要靠印出 copy constructor 次數理解語意，因為最佳化會改變觀察結果。
  - 含有 std::unique_ptr 的類別預設不可複製，這是所有權語意的保護；若要複製，必須明確定義要深拷貝什麼。
  - 寫 copy constructor 時也要一起檢查 copy assignment 和 destructor，這就是 Rule of Three 的來源。
*/
#include <iostream>
#include <cstring>     // strlen, strcpy
#include <vector>
#include <string>

// -----------------------------------------------------------------------------
// 範例 1：用 std::string 包起來的 Person - 預設複製建構子就 OK
// -----------------------------------------------------------------------------
class Person {
private:
    std::string name_;
    int         age_;
public:
    Person(const std::string& n, int a) : name_(n), age_(a) {}
    void show() const {
        std::cout << "Person(" << name_ << ", " << age_ << ")" << std::endl;
    }
    void rename(const std::string& n) { name_ = n; }
};

// -----------------------------------------------------------------------------
// 範例 2：自己管 char* 的 MyString - 看「淺拷貝」會出什麼事
// -----------------------------------------------------------------------------
class MyString {
private:
    char* data_;     // 指向 heap 上的字串緩衝區 (由本物件擁有)
    size_t size_;

public:
    // 一般建構子
    MyString(const char* s = "") {
        size_ = std::strlen(s);
        data_ = new char[size_ + 1];     // +1 是給結尾的 '\0'
        std::strcpy(data_, s);
        std::cout << "[ctor] 建立 \"" << data_ << "\" 位址=" << (void*)data_ << "\n";
    }

    // 自己寫的「深拷貝」複製建構子
    // 把參數的 data_ 內容複製到自己新配置的緩衝區
    MyString(const MyString& other)
        : data_(new char[other.size_ + 1]), size_(other.size_) {
        std::strcpy(data_, other.data_);
        std::cout << "[copy] 深拷貝 \"" << data_ << "\" 位址=" << (void*)data_ << "\n";
    }

    ~MyString() {
        std::cout << "[dtor] 釋放 \"" << (data_ ? data_ : "") << "\" 位址=" << (void*)data_ << "\n";
        delete[] data_;
    }

    void append(char c) {
        char* nd = new char[size_ + 2];
        std::strcpy(nd, data_);
        nd[size_]   = c;
        nd[size_+1] = '\0';
        delete[] data_;
        data_ = nd;
        ++size_;
    }

    const char* c_str() const { return data_; }
};

// -----------------------------------------------------------------------------
// 一個輔助函式，用來示範「按值傳入」會呼叫複製建構子
// -----------------------------------------------------------------------------
void takeByValue(MyString s) {
    std::cout << "(takeByValue) 收到: " << s.c_str() << std::endl;
}   // 這裡 s 解構，會釋放它自己擁有的緩衝區 (深拷貝才安全)

int main() {
    std::cout << "----- 範例 1：Person 預設複製建構子 -----" << std::endl;
    Person a("Alice", 30);
    Person b = a;             // 用 a 來建構 b → 呼叫複製建構子 (預設版本)
    b.rename("Alice2");       // 改 b 不會影響 a，因為 std::string 自己會深拷貝
    a.show();
    b.show();

    std::cout << "----- 範例 2：MyString 深拷貝 -----" << std::endl;
    MyString s1("hello");
    MyString s2 = s1;         // 深拷貝：兩個物件持有「不同位址」的兩塊記憶體
    s2.append('!');           // 改 s2 不會動到 s1
    std::cout << "s1 = " << s1.c_str() << std::endl;   // hello
    std::cout << "s2 = " << s2.c_str() << std::endl;   // hello!

    std::cout << "----- 範例 3：按值傳入會呼叫複製建構子 -----" << std::endl;
    takeByValue(s1);          // 進入函式時複製一份
    std::cout << "(回到 main 後 s1 仍可用) s1 = " << s1.c_str() << std::endl;

    std::cout << "----- 範例 4：Leetcode 1480 - 累加器物件被複製 -----" << std::endl;
    // 目的：觀察「複製一個有狀態的物件」之後，兩個物件的狀態是否獨立。
    // RunningSum 這個類別所有成員都是 std::vector / int (都是 RAII 型別)，
    // 所以沒寫 copy ctor 也沒事，編譯器產生的逐成員複製就是「深拷貝」。
    class RunningSum {
    public:
        std::vector<int> history;     // 紀錄每次累加後的結果
        int total = 0;                // 目前累計值
        int add(int x) {
            total += x;
            history.push_back(total);
            return total;
        }
    };
    RunningSum acc;
    acc.add(1); acc.add(2); acc.add(3);   // history = {1,3,6}, total = 6

    RunningSum acc2 = acc;                // ← 呼叫預設複製建構子 (深拷貝)
    acc2.add(10);                          // 改 acc2 不會影響 acc
    std::cout << "acc.total = " << acc.total << " (應為 6)\n";
    std::cout << "acc2.total = " << acc2.total << " (應為 16，加了 10)\n";
    std::cout << "acc.history.size = " << acc.history.size()
              << "，acc2.history.size = " << acc2.history.size() << "\n";

    std::cout << "----- 範例 5：Leetcode 155 Min Stack (複製要安全)  難度: medium -----" << std::endl;
    // 題目簡述：設計支援 push/pop/top/getMin 都 O(1) 的 stack。
    // 重點：每個 push 同時把「目前最小值」也疊進另一個 stack，
    //       pop 時兩邊一起 pop，這樣 getMin 永遠 O(1)。
    // 由於只用了 std::vector (RAII)，預設 copy ctor 直接深拷貝，超安全。
    class MinStack {
    public:
        std::vector<int> data;
        std::vector<int> mins;
        void push(int x) {
            data.push_back(x);
            // 若是第一個元素或 x ≤ 目前 min，min 也要更新
            if (mins.empty() || x <= mins.back()) mins.push_back(x);
            else                                  mins.push_back(mins.back());
        }
        void pop() { data.pop_back(); mins.pop_back(); }
        int  top() const { return data.back(); }
        int  getMin() const { return mins.back(); }
    };

    MinStack ms;
    ms.push(-2); ms.push(0); ms.push(-3);
    std::cout << "getMin = " << ms.getMin() << " (預期 -3)" << std::endl;

    // 複製整個 MinStack，兩份狀態互不干擾
    MinStack ms2 = ms;
    ms2.pop();                                 // 只動 ms2
    std::cout << "ms.getMin  = " << ms.getMin()  << " (仍是 -3)" << std::endl;
    std::cout << "ms2.top    = " << ms2.top()    << " (預期 0)"  << std::endl;
    std::cout << "ms2.getMin = " << ms2.getMin() << " (預期 -2)" << std::endl;

    std::cout << "----- 範例 6：日常實用 - Snapshot 快照備份 -----" << std::endl;
    // 工作上常需要把目前物件「複製一份」做為備份/還原點。
    // 因為成員是 std::vector，預設複製建構就是深拷貝。
    class Editor {
    public:
        std::vector<std::string> buffer;
        void type(const std::string& s) { buffer.push_back(s); }
        size_t lines() const { return buffer.size(); }
    };

    Editor ed;
    ed.type("hello"); ed.type("world");
    Editor snapshot = ed;                     // 拍快照 (複製建構)
    ed.type("more");                          // 改原本的不影響 snapshot
    std::cout << "ed lines = " << ed.lines()
              << ", snapshot lines = " << snapshot.lines() << std::endl;

    /*
     * 學習小提醒：
     *   如果把 MyString 的「複製建構子」整個拿掉，編譯器會幫我們產生一個
     *   「member-wise copy」版本，那就是淺拷貝 → s1 跟 s2 的 data_ 指向
     *   同一塊記憶體，當兩者解構時會 delete 同一塊兩次 → 程式崩潰。
     *
     *   現代 C++ 的解法是：
     *     (a) 用 std::string / std::vector 這種會自己管資源的型別當成員。
     *     (b) 或寫成 RAII + 自訂複製建構/賦值/解構 (Rule of 3，第 23 篇)。
     */
    return 0;
}

/* 預期輸出（位址數值會因執行而異）：
 * ----- 範例 1：Person 預設複製建構子 -----
 * Person(Alice, 30)
 * Person(Alice2, 30)
 * ----- 範例 2：MyString 深拷貝 -----
 * [ctor] 建立 "hello" 位址=0x...
 * [copy] 深拷貝 "hello" 位址=0x...
 * s1 = hello
 * s2 = hello!
 * ----- 範例 3：按值傳入會呼叫複製建構子 -----
 * [copy] 深拷貝 "hello" 位址=0x...
 * (takeByValue) 收到: hello
 * [dtor] 釋放 "hello" 位址=0x...
 * (回到 main 後 s1 仍可用) s1 = hello
 * ----- 範例 4：Leetcode 1480 - 累加器物件被複製 -----
 * acc.total = 6 (應為 6)
 * acc2.total = 16 (應為 16，加了 10)
 * acc.history.size = 3，acc2.history.size = 4
 * ----- 範例 5：Leetcode 155 Min Stack (複製要安全)  難度: medium -----
 * getMin = -3 (預期 -3)
 * ms.getMin  = -3 (仍是 -3)
 * ms2.top    = 0 (預期 0)
 * ms2.getMin = -2 (預期 -2)
 * ----- 範例 6：日常實用 - Snapshot 快照備份 -----
 * ed lines = 3, snapshot lines = 2
 * [dtor] 釋放 "hello!" 位址=0x...
 * [dtor] 釋放 "hello" 位址=0x...
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. 複製建構子簽名：ClassName(const ClassName& other)
 *      參數一定要用 const reference，避免無限遞迴。
 *   2. 它在「物件誕生時用另一個物件去填它」時被呼叫；
 *      = 在「已存在物件被另一個賦值」時，呼叫的是賦值運算子，下一篇學。
 *   3. 不寫 → 編譯器自動產生 member-wise copy (淺拷貝)。
 *   4. 成員若是 raw pointer 自己管 heap，需要自己寫深拷貝，
 *      否則會出現雙重釋放、互相干擾等災難。
 *   5. 用 std::string / std::vector / 智慧指標當成員，能讓你大多時候不用自寫。
 *
 * 【下一篇預告】
 *   9_AssignmentOperator.cpp
 *   賦值運算子 operator= — 對「已存在的物件」做指派時的特殊處理，
 *   重點是「自我賦值防護」與「資源釋放再配置」的順序。
 *=============================================================================*/
