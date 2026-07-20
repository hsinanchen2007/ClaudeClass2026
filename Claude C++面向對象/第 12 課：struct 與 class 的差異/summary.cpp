/*
 * ================================================================
 * 【第 12 課：struct 與 class 的差異】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. struct 與 class 的唯一語法差異：預設存取修飾符
 * 2. struct 可以使用完整的 OOP 功能（private、virtual、繼承）
 * 3. 慣例用法：struct 用於純資料、class 用於完整 OOP
 * 4. struct 的繼承預設為 public，class 預設為 private
 * 5. 何時用 struct，何時用 class
 * ================================================================
 */

// =============================================================================
// 【主題資訊 Information】
// -----------------------------------------------------------------------------
//   主題：    struct 與 class 的差異 —— 以及為什麼差異這麼小卻這麼重要
//   語法：    struct X { ... };   // 成員預設 public，繼承預設 public
//             class  X { ... };   // 成員預設 private，繼承預設 private
//   標準：    C++98 起兩者即為同義；差別只有上面那兩個預設值。
//   標頭檔：  <iostream>、<string>、<cmath>、<cstdint>
//   一句話：  **它們是同一個東西，只差兩個預設值。**
//
// =============================================================================
// 【詳細解釋 Explanation】
// -----------------------------------------------------------------------------
//
// 【1. 完整的差異清單（真的只有兩條）】
//     (a) 成員的預設存取權：struct = public，class = private。
//     (b) 繼承的預設方式：  struct D : B  等同  struct D : public B
//                          class  D : B  等同  class  D : private B
//   除此之外**完全相同**。struct 可以有：
//     建構函式、解構函式、virtual 函式、純虛擬函式、繼承、多重繼承、
//     運算子多載、private/protected 成員、static 成員、friend、template……
//   「struct 只能裝資料」是 **C** 的規則，不是 C++ 的。
//
// 【2. 為什麼 (b) 比 (a) 更容易出事】
//   成員預設值寫錯，編譯器會立刻報「無法存取 private 成員」，你馬上會發現。
//   但繼承預設值寫錯**不會立刻報錯**，只會讓多型悄悄失效：
//       class Derived : Base { ... };        // 這是 private 繼承！
//       Base* p = new Derived();             // ❌ 編譯錯誤，但訊息很難懂
//   private 繼承表達的是「用 Base 實作 Derived」（has-a 的替代），
//   而非「Derived is-a Base」，所以 Derived* 無法隱式轉成 Base*。
//   ★ 準則：**永遠顯式寫出繼承方式**，不要依賴預設值。
//
// 【3. 選用準則：看「有沒有不變量」】
//   這是 C++ Core Guidelines（C.2）與 Google Style Guide 的共識：
//     用 struct → **純資料聚合**：所有成員 public、彼此獨立，
//                  任意欄位組合都是合法狀態，沒有需要維護的一致性條件。
//                  例：Point、RGB、Config、函式的多回傳值打包。
//     用 class  → **有不變量的型別**：內部狀態必須滿足某些條件
//                  （balance >= 0、index < size、fd 有效…），
//                  需要靠成員函式集中把關。
//   本檔的 RGB 是前者的好例子（三個色彩分量互相獨立）；
//   BankAccount 是後者（餘額不可為負、提款要檢查）。
//
// 【4. 為什麼「純資料就別寫 getter/setter」】
//   常見的過度設計是為 Point 這種型別寫上 getX()/setX()/getY()/setY()。
//   這毫無收益：既然任意 (x, y) 組合都合法，setter 永遠不會拒絕任何值，
//   它就只是 public 的冗長寫法，還讓程式碼多了一倍。
//   判準仍然是那句：**這個 setter 有沒有可能拒絕呼叫端？**
//   不可能 → 直接用 public 成員（也就是用 struct）。
//
// =============================================================================
// 【概念補充 Concept Deep Dive】
// -----------------------------------------------------------------------------
// (A) 關鍵字與 POD／trivial／standard-layout 完全無關
//   這是最頑固的誤解之一。這些性質只取決於型別的**內容**：
//   有沒有 virtual 函式、有沒有使用者提供的建構函式、
//   資料成員是否都在同一個 access 區塊、基底類別的性質等等。
//   用 class 寫的 { public: int x, y; } 一樣是 standard-layout 且 trivial；
//   用 struct 寫的、帶 virtual 解構函式的型別一樣兩者皆非。
//   （本課第 12 課 -1 檔案有可執行的實測。）
//
// (B) 前向宣告可以混用關鍵字，但不建議
//       class Dog;            // 前向宣告
//       struct Dog { ... };   // 定義 —— 標準上合法
//   標準允許（兩者屬同一個 class-key 家族），但 MSVC 會發 C4099 警告，
//   因為除錯資訊會記錄 class-key，不一致可能造成連結期問題。
//   實務準則：前向宣告與定義用同一個關鍵字。
//
// (C) 聚合初始化（aggregate initialization）與關鍵字無關
//   RGB red = {255, 0, 0}; 這種寫法能用，是因為 RGB 是個 **aggregate**：
//   沒有使用者提供的建構函式、沒有 private/protected 非靜態資料成員、
//   沒有 virtual 函式、沒有 virtual 基底。
//   同樣條件的 class 也一樣能聚合初始化 —— 但因為 class 預設 private，
//   若不寫 public: 就會失去 aggregate 資格。這是「預設值」造成的間接影響，
//   而非關鍵字本身的規則。
//
// (D) C 相容性
//   要與 C 互通的型別（會被 memcpy、寫入檔案、傳給 C API）
//   必須是 standard-layout。這時 struct 是慣例選擇，
//   但真正的要求是「不要有 virtual、資料成員放同一個 access 區塊」，
//   而不是「必須用 struct 關鍵字」。
//
// =============================================================================
// 【注意事項 Pay Attention】
// -----------------------------------------------------------------------------
//  1. C++ 的 struct **不是** C 的 struct，功能上與 class 完全相同。
//  2. 繼承時**永遠顯式寫** public/protected/private，別依賴預設值 ——
//     class D : B 是 private 繼承，幾乎不會是你要的意思。
//  3. POD／trivial／standard-layout 由內容決定，與關鍵字無關。
//  4. 純資料聚合不要寫 getter/setter，那是沒有收益的樣板程式碼。
//  5. 前向宣告與定義建議用同一個關鍵字（MSVC C4099）。
//  6. 選 struct 還是 class，本質是**向讀者宣告設計意圖**，不是語法選擇。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】struct 與 class
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++ 中 struct 和 class 的差別是什麼？
//     答：只有兩個：(1) 成員預設存取權 —— struct 是 public、class 是 private；
//         (2) 繼承預設方式 —— struct 預設 public 繼承、class 預設 private 繼承。
//         其他完全相同：struct 一樣能有建構函式、virtual 函式、繼承、
//         運算子多載、private 成員。
//     追問：那實務上依什麼選？
//         → 看「有沒有不變量要維護」。純資料聚合用 struct，
//           有 private 狀態需要靠成員函式維護一致性的用 class。
//           這是在向讀者宣告設計意圖。
//
// 🔥 Q2. class D : B 和 struct D : B 有什麼差別？為什麼前者危險？
//     答：前者是 private 繼承、後者是 public 繼承。
//         private 繼承表達「用 B 實作 D」而非「D is-a B」，
//         因此 D* 無法隱式轉成 B*，多型完全失效 ——
//         而錯誤訊息通常出現在很遠的地方，難以聯想到繼承方式寫錯。
//     追問：那 private 繼承什麼時候真的有用？
//         → 想複用 B 的實作但不想公開它的介面時；
//           不過多數情況用組合（把 B 當成員）更清楚，
//           只有需要覆寫 B 的 virtual 函式時才非用 private 繼承不可。
//
// 🔥 Q3. RGB red = {255, 0, 0}; 這種聚合初始化，改用 class 寫還能用嗎？
//     答：可以，但前提是要顯式寫 public:。
//         聚合初始化的條件是該型別為 aggregate ——
//         沒有使用者提供的建構函式、沒有 private/protected 非靜態資料成員、
//         沒有 virtual 函式或 virtual 基底。
//         class 預設 private，不寫 public: 就會失去 aggregate 資格。
//     追問：加了自訂建構函式之後呢？
//         → 就不再是 aggregate，聚合初始化失效，
//           只能走建構函式（C++11 起可用 {} 呼叫建構函式，但語意不同）。
//
// ⚠️ 陷阱 1. 「struct 是 POD、class 不是」——對嗎？
//     答：**錯**。POD／trivial／standard-layout 完全由型別的內容決定，
//         與關鍵字無關。用 class 寫的純資料型別一樣是 standard-layout；
//         用 struct 寫的、有 virtual 函式的型別一樣不是 POD。
//     為什麼會錯：把 C 的世界觀（struct = 純資料）套進 C++，
//         再把「純資料」與「POD」畫上等號。
//
// ⚠️ 陷阱 2. 既然只差預設值，那全部都用 struct 並顯式寫 private: 不就好了？
//     答：語法上完全可行，但**會誤導讀者**。
//         關鍵字在實務上承載的是設計意圖：
//         看到 struct，讀者預期「一包公開資料，沒有不變量」；
//         看到 class，讀者預期「有內部狀態要保護」。
//         寫一個帶 private 成員與驗證邏輯的 struct，
//         等於在標題寫了與內容不符的字。
//     為什麼會錯：只從編譯器的角度想「反正結果一樣」，
//         忽略了程式碼的主要讀者是**人**。
//         語言允許的事，不代表溝通上是好的。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>
using namespace std;

// ================================================================
// 重點一：唯一的語法差異 —— 預設存取修飾符
// ================================================================
// struct：成員預設為 public（C 語言相容）
// class ：成員預設為 private（OOP 安全性）
//
// 除此之外，struct 和 class 在 C++ 中「完全相同」！

struct StructExample {
    // 預設 public —— 不需要寫 public:
    int x;
    int y;

    void print() {  // 也是預設 public
        cout << "struct: x=" << x << ", y=" << y << endl;
    }
};

class ClassExample {
    // 預設 private —— 外界無法直接存取
    int x;
    int y;

public:  // 必須明確寫 public:
    ClassExample(int a, int b) : x(a), y(b) {}

    void print() {
        cout << "class: x=" << x << ", y=" << y << endl;
    }
};

// ================================================================
// 重點二：struct 可以使用完整的 OOP 功能
// ================================================================
// struct 不只是「資料容器」，它和 class 完全一樣強大
// 可以有：建構函數、解構函數、private/protected、virtual、繼承

struct Point {
private:              // struct 也可以有 private！
    double x_, y_;

public:
    // 建構函數
    Point(double x = 0, double y = 0) : x_(x), y_(y) {}

    // Getter
    double x() const { return x_; }
    double y() const { return y_; }

    // 成員函數
    double distanceTo(const Point& other) const {
        double dx = x_ - other.x_;
        double dy = y_ - other.y_;
        return sqrt(dx*dx + dy*dy);
    }

    // 虛函數（struct 也可以有！）
    virtual void describe() const {
        cout << "Point(" << x_ << ", " << y_ << ")" << endl;
    }

    virtual ~Point() {}
};

// struct 也可以繼承（預設 public 繼承）
struct ColorPoint : Point {       // struct 繼承預設是 public
    string color;

    ColorPoint(double x, double y, const string& c)
        : Point(x, y), color(c) {}

    void describe() const override {
        cout << color << "色的 Point(" << x() << ", " << y() << ")" << endl;
    }
};

// ================================================================
// 重點三：struct 與 class 的繼承預設差異
// ================================================================
// struct 繼承預設：public
// class  繼承預設：private
//
// struct Child : Base     等同於  struct Child : public Base
// class  Child : Base     等同於  class  Child : private Base

struct BaseStruct {
    int value = 42;
};

struct DerivedStruct : BaseStruct {  // 預設 public 繼承
    // value 可直接存取
};

class BaseClass {
public:
    int value = 42;
};

class DerivedClassPrivate : BaseClass {  // 預設 private 繼承
    // value 在外部無法存取（已變成 private）
};

// ================================================================
// 重點四：慣例用法（Convention）
// ================================================================
// 雖然語法上幾乎相同，業界有約定俗成的用法：
//
// 用 struct 的情況：
//   - 純資料集合（Plain Old Data, POD）
//   - 沒有或很少有函數
//   - 所有成員公開
//   - C 語言介面相容的資料結構
//
// 用 class 的情況：
//   - 有封裝（private 成員 + public 介面）
//   - 有繼承和多型
//   - 有複雜的建構/解構邏輯
//   - 完整的 OOP 設計

// 典型 struct 用法：純資料容器
struct RGB {
    uint8_t r, g, b;  // 全部 public，無需保護
};

struct Config {
    string hostname;
    int port;
    bool useSSL;
    int timeout;
};

// 典型 class 用法：有封裝的實體
class BankAccount {
private:
    string owner;
    double balance;
    string accountNumber;

public:
    BankAccount(const string& owner, const string& accNum)
        : owner(owner), balance(0.0), accountNumber(accNum) {}

    // 受保護的操作，帶有驗證
    bool deposit(double amount) {
        if (amount <= 0) return false;
        balance += amount;
        return true;
    }

    bool withdraw(double amount) {
        if (amount <= 0 || amount > balance) return false;
        balance -= amount;
        return true;
    }

    double getBalance() const { return balance; }
    const string& getOwner() const { return owner; }
};

// ================================================================
// 重點五：完整對照表
// ================================================================
//
// ┌─────────────────┬──────────────┬──────────────┐
// │ 特性             │ struct        │ class         │
// ├─────────────────┼──────────────┼──────────────┤
// │ 預設存取修飾符   │ public        │ private       │
// │ 預設繼承方式     │ public        │ private       │
// │ 可有建構函數     │ ✓             │ ✓             │
// │ 可有解構函數     │ ✓             │ ✓             │
// │ 可有 private     │ ✓             │ ✓             │
// │ 可有 virtual     │ ✓             │ ✓             │
// │ 可以繼承         │ ✓             │ ✓             │
// │ 慣例用途         │ 純資料 POD   │ 完整 OOP      │
// └─────────────────┴──────────────┴──────────────┘


// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 56. Merge Intervals
//   題目：給一組區間 [start, end]，合併所有重疊的區間並回傳結果。
//   為什麼用到本主題：Interval 是**純資料聚合**的教科書範例 ——
//         start 與 end 彼此獨立、任意數值組合在解題過程中都合法、
//         沒有需要類別自己維護的不變量，所以用 **struct** 最恰當。
//         這題也順帶示範聚合初始化 Interval{1, 3} 的簡潔：
//         若寫成 class 並加上 getter/setter，程式碼會長一倍卻毫無收益。
//   複雜度：排序 O(n log n) + 掃描 O(n) = O(n log n)。
// -----------------------------------------------------------------------------
struct Interval {          // 純資料 → struct，成員直接公開
    int start = 0;
    int end   = 0;
};

vector<Interval> mergeIntervals(vector<Interval> intervals) {
    if (intervals.empty()) return {};

    // 依起點排序：之後只需一次線性掃描
    sort(intervals.begin(), intervals.end(),
         [](const Interval& a, const Interval& b) { return a.start < b.start; });

    vector<Interval> merged;
    merged.push_back(intervals[0]);

    for (size_t i = 1; i < intervals.size(); ++i) {
        Interval& last = merged.back();
        if (intervals[i].start <= last.end) {
            // 有重疊 → 把結尾往後延（取較大者，因為可能被完全包含）
            last.end = max(last.end, intervals[i].end);
        } else {
            merged.push_back(intervals[i]);   // 不重疊 → 開新區間
        }
    }
    return merged;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】HTTP 回應處理：一個 struct + 一個 class，各司其職
//   情境：呼叫外部 API 後要處理回應。這裡剛好需要兩種型別：
//     (1) HttpResponse —— **純資料聚合**：status、body、耗時三個欄位
//         彼此獨立，任意組合都是「一個合法的回應」（連 500 + 空 body 都合法）。
//         沒有不變量 → **用 struct**，直接聚合初始化，不寫 getter/setter。
//     (2) RetryPolicy  —— **有不變量**：重試次數不可為負、
//         退避時間必須遞增、且不可超過上限。
//         這些條件必須由型別自己保證 → **用 class**，成員 private。
//   為什麼用到本主題：這是「有沒有不變量」這條判準最貼近日常的應用。
//         同一個功能裡兩種型別並存，正好對照出各自的適用場合。
// -----------------------------------------------------------------------------
struct HttpResponse {          // 純資料 → struct
    int    status  = 0;
    string body;
    double elapsedMs = 0.0;

    // struct 一樣可以有成員函式（這正是本課重點）——
    // 這裡只是唯讀的便利查詢，沒有維護任何不變量
    bool ok() const { return status >= 200 && status < 300; }
};

class RetryPolicy {            // 有不變量 → class
public:
    // 工廠函式：參數不合理就無法建立，不變量從源頭得保
    static RetryPolicy makeDefault() { return RetryPolicy(3, 100, 5000); }

    // 參數不合理就退回預設值，呼叫端永遠拿到一個滿足不變量的物件
    static RetryPolicy make(int maxRetries, int baseDelayMs, int maxDelayMs) {
        if (maxRetries < 0 || baseDelayMs <= 0 || maxDelayMs < baseDelayMs) {
            cout << "    (參數不合理，退回預設策略)" << endl;
            return makeDefault();
        }
        return RetryPolicy(maxRetries, baseDelayMs, maxDelayMs);
    }

    // 指數退避：第 n 次重試等待 base * 2^n，但夾在 maxDelay 以內
    int delayForAttempt(int attempt) const {
        if (attempt < 0) return m_baseDelayMs;
        long long d = m_baseDelayMs;
        for (int i = 0; i < attempt && d < m_maxDelayMs; ++i) d *= 2;
        return static_cast<int>(d < m_maxDelayMs ? d : m_maxDelayMs);
    }

    bool shouldRetry(const HttpResponse& r, int attemptsSoFar) const {
        if (attemptsSoFar >= m_maxRetries) return false;
        // 5xx 與 429 值得重試；4xx（除 429）重試也沒用
        return r.status >= 500 || r.status == 429;
    }

    int maxRetries() const { return m_maxRetries; }

private:
    RetryPolicy(int maxRetries, int baseDelayMs, int maxDelayMs)
        : m_maxRetries(maxRetries), m_baseDelayMs(baseDelayMs), m_maxDelayMs(maxDelayMs) {}

    int m_maxRetries  = 0;
    int m_baseDelayMs = 0;
    int m_maxDelayMs  = 0;
};

int main() {
    cout << "===========================================" << endl;
    cout << "   第 12 課：struct 與 class 的差異展示" << endl;
    cout << "===========================================" << endl;

    // --- 預設存取修飾符差異 ---
    cout << "\n【預設存取修飾符】" << endl;

    StructExample se;
    se.x = 10;   // OK：struct 預設 public
    se.y = 20;
    se.print();

    ClassExample ce(30, 40);
    // ce.x = 30;  // 錯誤！class 預設 private
    ce.print();

    // --- struct 完整 OOP 示範 ---
    cout << "\n【struct 也能用 OOP】" << endl;

    Point p1(3.0, 4.0);
    Point p2(0.0, 0.0);
    p1.describe();
    cout << "p1 到原點的距離：" << p1.distanceTo(p2) << endl;

    ColorPoint cp(1.0, 2.0, "紅");
    cp.describe();  // 多型：呼叫 ColorPoint::describe()

    // --- 繼承預設差異 ---
    cout << "\n【繼承預設存取差異】" << endl;

    DerivedStruct ds;
    ds.value = 99;   // OK：public 繼承，value 仍是 public
    cout << "struct 繼承，value = " << ds.value << endl;

    DerivedClassPrivate dcp;
    // dcp.value = 99;  // 錯誤！private 繼承，value 變成 private
    cout << "class 預設 private 繼承，外部無法存取 value" << endl;

    // --- 慣例用法 ---
    cout << "\n【慣例用法對比】" << endl;

    RGB red = {255, 0, 0};  // struct：直接初始化，簡潔
    cout << "RGB: (" << (int)red.r << ", " << (int)red.g << ", " << (int)red.b << ")" << endl;

    BankAccount account("王小明", "ACC-001");
    account.deposit(1000.0);
    account.withdraw(250.0);
    cout << account.getOwner() << " 的餘額：" << account.getBalance() << endl;
    bool ok = account.withdraw(2000.0);  // 超出餘額，失敗
    cout << "嘗試提取 2000：" << (ok ? "成功" : "失敗") << endl;

    cout << "\n=== 結論：struct 和 class 幾乎相同 ===" << endl;
    cout << "唯一差異：預設存取修飾符（struct=public，class=private）" << endl;
    cout << "慣例：struct 用於 POD 資料，class 用於 OOP 設計" << endl;


    cout << "\n【LeetCode 56. Merge Intervals】" << endl;
    vector<Interval> iv{{1,3},{2,6},{8,10},{15,18}};
    for (const Interval& x : mergeIntervals(iv))
        cout << "  [" << x.start << "," << x.end << "]";
    cout << endl;
    vector<Interval> iv2{{1,4},{4,5}};        // 剛好相接也要合併
    for (const Interval& x : mergeIntervals(iv2))
        cout << "  [" << x.start << "," << x.end << "]";
    cout << "   <- 端點相接也算重疊" << endl;
    vector<Interval> iv3{{1,4},{2,3}};        // 完全包含
    for (const Interval& x : mergeIntervals(iv3))
        cout << "  [" << x.start << "," << x.end << "]";
    cout << "   <- 被完全包含的區間會被吸收" << endl;

    cout << "\n【日常實務：struct 放資料、class 管規則】" << endl;
    HttpResponse r1{200, "{\"ok\":true}", 42.5};      // struct 聚合初始化，簡潔
    cout << "  回應 status=" << r1.status << " ok()=" << boolalpha << r1.ok()
         << " 耗時=" << r1.elapsedMs << "ms" << endl;

    HttpResponse r2{503, "", 1200.0};
    cout << "  回應 status=" << r2.status << " ok()=" << r2.ok() << endl;

    RetryPolicy policy = RetryPolicy::makeDefault();
    cout << "  最大重試次數: " << policy.maxRetries() << endl;
    cout << "  503 要重試嗎? " << policy.shouldRetry(r2, 0) << endl;
    cout << "  200 要重試嗎? " << policy.shouldRetry(r1, 0) << endl;
    HttpResponse r404{404, "not found", 15.0};
    cout << "  404 要重試嗎? " << policy.shouldRetry(r404, 0) << "  (4xx 重試沒意義)" << endl;
    RetryPolicy bad = RetryPolicy::make(-1, 0, 10);   // 參數不合理
    cout << "  不合理參數建立後的 maxRetries: " << bad.maxRetries() << "  (退回預設)" << endl;
    cout << "  退避時間(指數成長，上限 5000ms):";
    for (int i = 0; i < 8; ++i) cout << " " << policy.delayForAttempt(i);
    cout << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
// ===========================================
//    第 12 課：struct 與 class 的差異展示
// ===========================================
// 
// 【預設存取修飾符】
// struct: x=10, y=20
// class: x=30, y=40
// 
// 【struct 也能用 OOP】
// Point(3, 4)
// p1 到原點的距離：5
// 紅色的 Point(1, 2)
// 
// 【繼承預設存取差異】
// struct 繼承，value = 99
// class 預設 private 繼承，外部無法存取 value
// 
// 【慣例用法對比】
// RGB: (255, 0, 0)
// 王小明 的餘額：750
// 嘗試提取 2000：失敗
// 
// === 結論：struct 和 class 幾乎相同 ===
// 唯一差異：預設存取修飾符（struct=public，class=private）
// 慣例：struct 用於 POD 資料，class 用於 OOP 設計
// 
// 【LeetCode 56. Merge Intervals】
//   [1,6]  [8,10]  [15,18]
//   [1,5]   <- 端點相接也算重疊
//   [1,4]   <- 被完全包含的區間會被吸收
// 
// 【日常實務：struct 放資料、class 管規則】
//   回應 status=200 ok()=true 耗時=42.5ms
//   回應 status=503 ok()=false
//   最大重試次數: 3
//   503 要重試嗎? true
//   200 要重試嗎? false
//   404 要重試嗎? false  (4xx 重試沒意義)
//     (參數不合理，退回預設策略)
//   不合理參數建立後的 maxRetries: 3  (退回預設)
//   退避時間(指數成長，上限 5000ms): 100 200 400 800 1600 3200 5000 5000
