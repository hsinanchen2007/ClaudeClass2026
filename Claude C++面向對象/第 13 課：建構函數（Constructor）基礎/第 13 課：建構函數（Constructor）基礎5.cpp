// =============================================================================
//  第 13 課：建構函數（Constructor）基礎 5  —  constructor 多載（overloading）
// =============================================================================
//
// 【主題資訊 Information】
//   主題      : 同一個類別提供多個 constructor，由參數列決定用哪一個
//   規則      : 參數的「個數或型別」不同即可多載；只有回傳型別不同不算（constructor 本來就沒有回傳型別）
//   標準版本  : C++98 起；本檔提到的 delegating constructor 是 C++11
//   標頭檔    : <iostream>、<string>
//
// 【詳細解釋 Explanation】
//
// 【1. constructor 多載就是一般的函式多載】
//   編譯器在看到 Student s2("張三", 20, 3.8f); 時，會做 overload resolution：
//     (a) 找出所有名字相符、可能可行的候選（candidate functions）
//     (b) 篩掉參數個數對不上的
//     (c) 對剩下的，逐一計算每個引數需要哪種轉換，比較「誰比較不需要轉換」
//   轉換的優先順序大致是：
//       完全相符 > 提升（char→int、float→double）> 標準轉換（int→double）
//       > 使用者定義轉換（呼叫別的 constructor 或 operator T()）
//   若兩個候選「一樣好」，就是 ambiguous，編譯失敗——這不是警告，是錯誤。
//
// 【2. 本檔的隱藏成本：Student(string n, ...) 用了值傳遞】
//   Student(string n, int a, float g) { name = n; ... }
//   傳一個 std::string 進來會發生：
//       第 1 次複製：實參 → 參數 n（若實參是左值）
//       第 2 次複製：n → 成員 name（operator=）
//   兩次複製，兩次可能的堆積配置。改成 const string& 可以省掉第一次；
//   再改用初始化列表 : name(n) 可以讓第二次變成 copy constructor 而非賦值。
//   第 15 課會完整處理這個主題，本檔先埋下觀念。
//
// 【3. 多載 vs 預設參數：什麼時候該用哪一個】
//   下面兩種寫法「呼叫端看起來一樣」：
//       // A. 兩個多載
//       Student();
//       Student(string n, int a, float g);
//       // B. 一個帶預設參數的 constructor
//       Student(string n = "未命名", int a = 0, float g = 0.0f);
//   差別在於：
//     * B 比較精簡，但它讓「所有中間組合」都合法（只給 name、只給 name+age…），
//       有時那些組合並沒有意義。
//     * B 的預設值寫在標頭檔，改動會讓所有呼叫端必須重新編譯（ABI 議題）。
//     * A 可以讓不同多載執行完全不同的邏輯。
//   實務上：語意相同、只是「省略尾巴」→ 用預設參數；
//           語意不同（例如「從檔案建構」vs「從記憶體建構」）→ 用多載。
//
// 【4. C++11 的 delegating constructor：消除重複】
//   多個 constructor 常有重複的初始化程式碼。C++11 允許一個 constructor
//   把工作「委派」給同類別的另一個：
//       Student() : Student("未命名", 0, 0.0f) {}   // 委派給三參數版本
//   注意：委派時初始化列表**只能**有那一個委派呼叫，不能同時初始化其他成員。
//   本檔示範了這個寫法，可與上面的傳統寫法對照。
//
// 【概念補充 Concept Deep Dive】
//   ▍多載解析發生在編譯期，零執行期成本
//     選哪個 constructor 是編譯期決定的，最終產生的就是一個直接呼叫。
//     它跟 virtual function 的執行期分派完全是兩回事。
//
//   ▍為什麼「只有回傳型別不同」不能多載
//     因為呼叫端可以忽略回傳值（例如單獨寫一行 f();），
//     此時編譯器無從判斷你要哪一個。constructor 沒有回傳型別，自然也不適用。
//
//   ▍delegating constructor 的例外安全性
//     若被委派的 constructor 成功、而委派方的本體拋出例外，
//     那個「已經建構完成」的物件會被正確解構（destructor 會被呼叫）。
//     這與「成員已建構完成」的規則一致。
//
// 【注意事項 Pay Attention】
//   1. 多載之間若有隱式轉換牽扯，容易出現 ambiguous 錯誤——尤其是
//      同時有 (int) 和 (double) 版本時，傳 3.0f 或字面值 0 都可能兩邊一樣好。
//   2. 單參數 constructor 預設是隱式轉換的來源，常需要加 explicit（第 15 課）。
//   3. 值傳遞 std::string 會造成多餘複製；本檔保留原寫法以對照，正式碼請用 const&。
//   4. delegating constructor 不能形成循環（A 委派 B、B 又委派 A）——那是 UB。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】constructor 多載
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 多個 constructor 時，編譯器怎麼決定用哪一個？
//     答：overload resolution。先依參數個數篩出可行候選，再比較每個引數所需的
//         轉換等級（完全相符 > 提升 > 標準轉換 > 使用者定義轉換），選整體最好的。
//         若有兩個一樣好就是 ambiguous，直接編譯錯誤。整個過程都在編譯期完成。
//     追問：那 Student s(0); 為什麼有時會 ambiguous？→ 字面值 0 可以轉成 int、
//         也可以轉成 double、甚至是空指標常數，多個多載都「同樣好」時就無法決定。
//
// 🔥 Q2. 多載和預設參數該怎麼選？
//     答：語意相同、只是省略尾端參數 → 預設參數，比較精簡；
//         語意不同、初始化邏輯不同 → 多載，各自寫各自的。
//         另外預設參數寫在宣告處，改動會影響所有呼叫端的重新編譯，
//         寫函式庫時要留意這個 ABI 面向。
//     追問：兩者可以混用嗎？→ 可以，但很容易造出 ambiguous 的組合，
//         例如同時有 Student(string) 和 Student(string, int = 0)，
//         呼叫 Student("x") 時兩者皆可行 → 編譯錯誤。
//
// ⚠️ 陷阱. 用 delegating constructor 時寫成
//         Student() : Student("未命名", 0, 0.0f), age(0) {}
//     答：編譯錯誤。一旦使用委派，初始化列表中**只能**有那一個委派呼叫，
//         不可以再初始化其他成員——因為被委派的 constructor 已經負責了整個物件。
//     為什麼會錯：把委派想成「先呼叫一個 helper，再繼續初始化剩下的」。
//         實際上被委派的 constructor 一結束，物件就已經完整建構好了，
//         之後只剩委派方的「本體」可以執行。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
using namespace std;

class Student {
private:
    string name;
    int age;
    float gpa;

public:
    // 無參數建構函數（default constructor）
    Student() {
        name = "未命名";
        age = 0;
        gpa = 0.0f;
        cout << "[預設建構] 創建學生: " << name << endl;
    }

    // 帶參數的建構函數（多載）
    Student(string n, int a, float g) {
        name = n;
        age = a;
        gpa = g;
        cout << "[帶參建構] 創建學生: " << name << endl;
    }

    void print() const {
        cout << "  姓名: " << name
             << ", 年齡: " << age
             << ", GPA: " << gpa << endl;
    }
};

// 對照組：用 C++11 delegating constructor 消除重複
class StudentDelegating {
private:
    string name;
    int    age;
    float  gpa;

public:
    // 主 constructor：真正做事的只有這一個
    StudentDelegating(const string& n, int a, float g)
        : name(n), age(a), gpa(g) {
        cout << "[主建構] " << name << endl;
    }

    // 委派給主 constructor，不重複寫初始化邏輯
    StudentDelegating() : StudentDelegating("未命名", 0, 0.0f) {
        cout << "[委派建構] 委派完成後才執行本體" << endl;
    }

    void print() const {
        cout << "  姓名: " << name << ", 年齡: " << age
             << ", GPA: " << gpa << endl;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】從缺
//   理由：LeetCode 的設計題只會用單一個 constructor（題目簽名固定），
//         不存在「多個 constructor 讓編譯器挑」的情境，多載解析也無從觀察。
//         本檔的重點是編譯期的 overload resolution，屬於語言機制。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】快取項目（CacheEntry）：不同來源、不同建構方式
//   情境：快取的資料可能來自兩個地方——
//         (1) 剛從資料庫查出來（需要記錄「查詢耗時」，並標記為新鮮的）
//         (2) 從磁碟上的持久化快取載入（已知寫入時間，可能已過期）
//   這兩種來源的初始化邏輯完全不同，正是「該用多載而不是預設參數」的典型案例：
//   若硬塞成一個帶預設參數的 constructor，呼叫端會分不清自己在建哪一種。
// -----------------------------------------------------------------------------
class CacheEntry {
private:
    string key_;
    string value_;
    int    ageSeconds_;    // 這筆資料存在多久了
    bool   fromDisk_;
    int    dbCostMs_;      // 只有從資料庫來的才有意義

public:
    // 多載 1：從資料庫查詢結果建構 —— 一定是新鮮的（age = 0）
    CacheEntry(const string& key, const string& value, int dbCostMs)
        : key_(key), value_(value), ageSeconds_(0),
          fromDisk_(false), dbCostMs_(dbCostMs) {}

    // 多載 2：從磁碟快取載入 —— 帶著既有的年齡，沒有查詢成本
    CacheEntry(const string& key, const string& value, int ageSeconds, bool /*fromDiskTag*/)
        : key_(key), value_(value), ageSeconds_(ageSeconds),
          fromDisk_(true), dbCostMs_(-1) {}

    bool isStale(int ttlSeconds) const { return ageSeconds_ >= ttlSeconds; }

    void print() const {
        cout << "  key=" << key_ << ", value=" << value_
             << ", 來源=" << (fromDisk_ ? "磁碟快取" : "資料庫")
             << ", 已存在=" << ageSeconds_ << "s";
        if (dbCostMs_ >= 0) cout << ", 查詢耗時=" << dbCostMs_ << "ms";
        cout << endl;
    }
};

int main() {
    cout << "=== 使用預設建構函數 ===" << endl;
    Student s1;
    s1.print();

    cout << "\n=== 使用帶參數建構函數 ===" << endl;
    Student s2("張三", 20, 3.8f);
    s2.print();

    Student s3("李四", 22, 3.5f);
    s3.print();

    cout << "\n=== C++11 委派建構函數（注意執行順序）===" << endl;
    StudentDelegating d1;          // 先跑主建構，再跑委派方的本體
    d1.print();
    StudentDelegating d2("王五", 25, 3.9f);
    d2.print();

    cout << "\n=== 日常實務：CacheEntry 的兩種建構來源 ===" << endl;
    CacheEntry fresh("user:1001", "Alice", 35);        // 多載 1：剛從 DB 查到
    fresh.print();

    CacheEntry loaded("user:2002", "Bob", 900, true);  // 多載 2：從磁碟載入
    loaded.print();

    const int ttl = 600;
    cout << "  TTL = " << ttl << "s" << endl;
    cout << "  fresh  是否過期? " << (fresh.isStale(ttl)  ? "是" : "否") << endl;
    cout << "  loaded 是否過期? " << (loaded.isStale(ttl) ? "是" : "否") << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 13 課：建構函數（Constructor）基礎5.cpp" -o ctor5

// === 預期輸出 ===
// === 使用預設建構函數 ===
// [預設建構] 創建學生: 未命名
//   姓名: 未命名, 年齡: 0, GPA: 0
//
// === 使用帶參數建構函數 ===
// [帶參建構] 創建學生: 張三
//   姓名: 張三, 年齡: 20, GPA: 3.8
// [帶參建構] 創建學生: 李四
//   姓名: 李四, 年齡: 22, GPA: 3.5
//
// === C++11 委派建構函數（注意執行順序）===
// [主建構] 未命名
// [委派建構] 委派完成後才執行本體
//   姓名: 未命名, 年齡: 0, GPA: 0
// [主建構] 王五
//   姓名: 王五, 年齡: 25, GPA: 3.9
//
// === 日常實務：CacheEntry 的兩種建構來源 ===
//   key=user:1001, value=Alice, 來源=資料庫, 已存在=0s, 查詢耗時=35ms
//   key=user:2002, value=Bob, 來源=磁碟快取, 已存在=900s
//   TTL = 600s
//   fresh  是否過期? 否
//   loaded 是否過期? 是
