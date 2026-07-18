/*=============================================================================
 * 檔名：9_AssignmentOperator.cpp
 * 主題：賦值運算子 operator= - 對「已存在物件」做指派
 * 適合：學完複製建構子，想搞清楚「= 的兩種情況」的人
 *
 * 【課題介紹】
 *   有兩個非常容易混淆的場景：
 *
 *       Foo a("初始化");
 *       Foo b = a;     // (1) 用 a「建構」b           → 呼叫複製建構子
 *       Foo c;
 *       c = a;         // (2) 把 a「指派」給已存在的 c → 呼叫賦值運算子 operator=
 *
 *   差別在哪？
 *     (1) 物件 b 還沒誕生，要從 a 這個既有物件出生。
 *     (2) 物件 c 已經存在 (而且可能已經持有某些資源)，
 *         現在我們要「丟掉它原本的內容、改成 a 的內容」。
 *
 *   因此 operator= 比複製建構子多了一個重要步驟：
 *
 *       「先釋放自己原本擁有的資源，再放新內容進來。」
 *
 * 【標準形式】
 *   ClassName& operator=(const ClassName& other) {
 *       if (this == &other) return *this;     // (a) 自我賦值防護
 *       // (b) 釋放舊資源
 *       // (c) 配置 + 複製新資源
 *       return *this;                         // (d) 回傳 *this 以支援連鎖賦值 a=b=c
 *   }
 *
 *   為什麼回傳型別是 ClassName& (參考)？
 *     - 內建型別也支援 a = b = c (右結合)，自訂型別要做到一樣的效果，
 *       就要回傳「同一個物件」的參考。
 *
 * 【為什麼要做「自我賦值防護 (Self-assignment Check)」？】
 *   想像：
 *       Foo x;
 *       Foo* p = &x;
 *       x = *p;      // 等同 x = x，常見於別名 (alias) 場景
 *
 *   如果你忘記檢查，直接做「先釋放自己的資源，再從 other 複製」，
 *   那你會先把自己的資料砍了，再去複製「已經沒了」的資料 → 災難。
 *
 *   所以第一行通常是：if (this == &other) return *this;
 *
 * 【更現代的寫法：copy-and-swap (簡介)】
 *   有一種被認為「簡潔且異常安全 (exception-safe)」的寫法：
 *       ClassName& operator=(ClassName other) {   // 注意這裡用值傳入 → 自動複製
 *           swap(*this, other);
 *           return *this;
 *       }
 *   每次呼叫都會先做一份完整副本 (透過參數)，再 swap 進來，
 *   舊資源跟著區域副本離開時被解構。本篇先看「傳統寫法」，這個之後再深入。
 *
 * 【日常實用範例】
 *   延續第 8 篇的 MyString，把 operator= 寫好，理解：
 *     1. 自我賦值防護
 *     2. 釋放舊資源 + 深拷貝新資源
 *     3. 回傳 *this 支援連鎖賦值
 *
 * 【對應 Leetcode】1480. Running Sum of 1d Array
 *   為什麼選這題：物件「賦值」與「複製建構」最大的差別在於「賦值前物件已經存在」。
 *   我們做兩個累加器，分別累加不同的數，再用 a = b 的方式把 b 的累計狀態整個搬到 a，
 *   觀察 a 原本的狀態被丟棄、變成跟 b 一模一樣（但兩物件記憶體獨立）。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/language/copy_assignment
 *   https://cplusplus.com/doc/tutorial/classes2/
 *=============================================================================*/

/*
補充筆記：AssignmentOperator
  - AssignmentOperator 這類 OOP 範例要追蹤物件狀態：建構後是否有效、操作後是否仍符合類別承諾。
  - 如果類別擁有資源，就要檢查 destructor、copy、move 是否表達同一套所有權規則。
  - 繼承、friend、static、operator overload 都應服務於清楚的物件語意，而不是只展示語法。
  - copy assignment operator 用於「已存在的物件」接收另一個物件的值，典型形式是 Class& operator=(const Class& rhs)。
  - 它和 copy constructor 最大差別是目標物件已經持有舊資源，所以必須先處理舊狀態，再複製新狀態。
  - 要處理自我賦值，例如 a = a；如果先 delete 自己的資源，再從 rhs 複製，而 rhs 正好是自己，就會讀到已釋放資料。
  - 回傳 Class& 並回傳 *this，可支援 a = b = c 這種鏈式賦值；若回傳 void 會破壞內建型別賦值的一致語意。
  - 強例外安全常用 copy-and-swap：先複製 rhs 到暫時物件，成功後交換資源；若複製失敗，原物件保持原狀。
  - 若成員都是 std::string、std::vector、智慧指標等 RAII 型別，預設賦值通常比手寫安全。
  - move assignment 要處理右值資源轉移；一旦類別手寫 copy/destructor，就要思考 move 是否也該明確定義或刪除。
  - 賦值運算子應維持類別不變條件，不能只把每個欄位機械式複製而忽略欄位之間的關係。
*/
#include <iostream>
#include <cstring>
#include <vector>
#include <string>

class MyString {
private:
    char*  data_;
    size_t size_;

public:
    MyString(const char* s = "") : data_(nullptr), size_(std::strlen(s)) {
        data_ = new char[size_ + 1];
        std::strcpy(data_, s);
    }

    // 複製建構子 (第 8 篇學過)
    MyString(const MyString& other) : data_(new char[other.size_ + 1]), size_(other.size_) {
        std::strcpy(data_, other.data_);
    }

    // ===== 本篇重點：複製賦值運算子 (Copy Assignment Operator) =====
    MyString& operator=(const MyString& other) {
        std::cout << "[op=] 從 \"" << other.data_ << "\" 賦值給 \"" << data_ << "\"\n";

        // (a) 自我賦值防護：a = a 這種情況直接什麼都不做
        if (this == &other) {
            std::cout << "  └ 自我賦值，直接 return\n";
            return *this;
        }

        // (b) 釋放舊資源
        delete[] data_;

        // (c) 配置新空間，複製新內容
        size_ = other.size_;
        data_ = new char[size_ + 1];
        std::strcpy(data_, other.data_);

        // (d) 回傳 *this，這樣 a = b = c 可以連鎖工作
        return *this;
    }

    ~MyString() { delete[] data_; }

    const char* c_str() const { return data_; }
};

int main() {
    std::cout << "----- 賦值 vs 複製建構 -----" << std::endl;
    MyString a("apple");
    MyString b("banana");

    MyString c = a;       // 呼叫複製建構子，c 還沒存在
    std::cout << "c = " << c.c_str() << std::endl;

    b = a;                // 呼叫 operator=，b 已存在
    std::cout << "b = " << b.c_str() << std::endl;

    std::cout << "----- 自我賦值 -----" << std::endl;
    a = a;                // 應該被守住，不會崩潰
    std::cout << "a = " << a.c_str() << std::endl;

    std::cout << "----- 連鎖賦值 a = b = c -----" << std::endl;
    MyString x("xxx"), y("yyy"), z("zzz");
    x = y = z;            // 等同於 x = (y = z); 因為 = 是右結合
    std::cout << "x = " << x.c_str() << std::endl;
    std::cout << "y = " << y.c_str() << std::endl;
    std::cout << "z = " << z.c_str() << std::endl;

    std::cout << "----- 範例 4：Leetcode 1480 - 賦值整顆累加器 -----" << std::endl;
    // 兩個累加器各自累加不同的東西，然後用 a = b 把 b 的狀態整個覆蓋過去 a。
    // 因為成員都是 std::vector / int (RAII 型別)，編譯器產生的預設賦值就是深拷貝，
    // 所以 a 把舊狀態完全丟掉，變成跟 b 等價但仍然是獨立的物件。
    class RunningSum {
    public:
        std::vector<int> history;
        int total = 0;
        int add(int x) { total += x; history.push_back(total); return total; }
    };
    RunningSum r1; r1.add(1); r1.add(2);        // r1.total = 3, history = {1,3}
    RunningSum r2; r2.add(100); r2.add(200);    // r2.total = 300, history = {100,300}
    r1 = r2;                                     // 賦值：r1 的舊狀態 (3) 被丟棄，成為 r2 的副本
    r1.add(50);                                  // 之後改 r1 不會影響 r2
    std::cout << "r1.total = " << r1.total << " (應為 350)\n";
    std::cout << "r2.total = " << r2.total << " (應為 300，未受 r1 影響)\n";

    std::cout << "----- 範例 5：Leetcode 225 用兩個 queue 做 stack -----" << std::endl;
    // 題目簡述：用 queue 模擬 stack (LIFO)，支援 push/pop/top/empty。
    // 重點：成員都是 RAII 容器 (vector 模擬 queue)，預設賦值即可整顆狀態複製。
    class MyStack {
    public:
        std::vector<int> data;       // 簡化：用 vector 當底層 queue
        void push(int x) {
            // 把 x 放 front：先把舊資料放後面，新元素放最前
            std::vector<int> nd{ x };
            for (int v : data) nd.push_back(v);
            data = std::move(nd);    // 這裡也用到「賦值」(移動賦值，第 22 篇詳述)
        }
        int  pop() {
            int v = data.front();
            data.erase(data.begin());
            return v;
        }
        int  top() const { return data.front(); }
        bool empty() const { return data.empty(); }
    };

    MyStack st;
    st.push(1); st.push(2); st.push(3);
    MyStack st2;
    st2 = st;                        // 整個 stack 賦值 (深拷貝)
    std::cout << "st2.pop = " << st2.pop() << " (預期 3)" << std::endl;
    std::cout << "st.top  = " << st.top()  << " (仍為 3，未受影響)" << std::endl;

    std::cout << "----- 範例 6：日常實用 - Config 配置物件覆蓋 -----" << std::endl;
    // 工作上常見：用「另一份預設配置」整個覆蓋目前配置。
    class AppConfig {
    public:
        std::string theme  = "light";
        int         volume = 50;
        std::string lang   = "zh-TW";
    };
    AppConfig cfg;
    AppConfig defaults;       // 預設值
    defaults.theme = "dark"; defaults.volume = 80; defaults.lang = "en-US";
    cfg = defaults;           // 用預設值覆蓋
    std::cout << "cfg: " << cfg.theme << "/" << cfg.volume << "/" << cfg.lang << std::endl;
    return 0;
}

/* 預期輸出（順序固定，內容確切）：
 * ----- 賦值 vs 複製建構 -----
 * c = apple
 * [op=] 從 "apple" 賦值給 "banana"
 * b = apple
 * ----- 自我賦值 -----
 * [op=] 從 "apple" 賦值給 "apple"
 *   └ 自我賦值，直接 return
 * a = apple
 * ----- 連鎖賦值 a = b = c -----
 * [op=] 從 "zzz" 賦值給 "yyy"
 * [op=] 從 "zzz" 賦值給 "xxx"
 * x = zzz
 * y = zzz
 * z = zzz
 * ----- 範例 4：Leetcode 1480 - 賦值整顆累加器 -----
 * r1.total = 350 (應為 350)
 * r2.total = 300 (應為 300，未受 r1 影響)
 * ----- 範例 5：Leetcode 225 用兩個 queue 做 stack -----
 * st2.pop = 3 (預期 3)
 * st.top  = 3 (仍為 3，未受影響)
 * ----- 範例 6：日常實用 - Config 配置物件覆蓋 -----
 * cfg: dark/80/en-US
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. operator= 用於「已存在物件」之間的指派；複製建構子用於「物件誕生」。
 *   2. 標準四步驟：
 *        (a) 自我賦值防護 if (this == &other) return *this;
 *        (b) 釋放舊資源
 *        (c) 配置並複製新資源
 *        (d) return *this 支援連鎖
 *   3. 回傳型別寫成 ClassName& (reference)，效能與語意都對。
 *   4. 進階：copy-and-swap idiom 可以一次解決自我賦值與例外安全，
 *      但需要先理解第 22 篇的 swap / move semantics，先記得有這東西即可。
 *
 * 【下一篇預告】
 *   10_ConstMember.cpp
 *   const 成員函式 — 「我保證不改物件」的承諾，
 *   以及 const 物件、const 參數一起搭配的觀念。
 *=============================================================================*/
