// =============================================================================
//  第 14 課：預設建構函數 9  —  = delete：明確禁止預設建構
// =============================================================================
//
// 【主題資訊 Information】
//   語法      : ClassName() = delete;
//   標準版本  : **C++11**（deleted function）
//   標頭檔    : <iostream>、<string>、<type_traits>
//   典型錯誤  : error: use of deleted function 'T::T()'
//   核心價值  : 把「這個操作不該存在」從註解裡的約定，變成編譯器強制的規則
//
// 【詳細解釋 Explanation】
//
// 【1. = delete 和「不寫」有什麼不同】
//   若你宣告了 DatabaseConnection(string, int)，隱式的 default constructor
//   本來就會消失。那為什麼還要多寫一行 = delete？
//   差別在**意圖的表達**與**錯誤訊息的品質**：
//       沒寫       → error: no matching function for call to 'T::T()'
//                    讀的人會想：「是漏寫了嗎？我幫他補一個好了」
//       = delete   → error: use of deleted function 'T::T()'
//                    讀的人立刻知道：「這是作者刻意禁止的」
//   前者是「碰巧沒有」，後者是「明確不准」。對維護者而言，這個差別很大。
//
// 【2. deleted function 仍然參與多載解析（關鍵機制）】
//   這是 = delete 最精妙的地方：被刪除的函式**仍然存在於多載集合中**，
//   只是一旦被選中就編譯失敗。這讓它可以用來精準地「擋掉特定的呼叫」：
//       void log(const string&);
//       void log(int) = delete;        // 禁止傳 int（避免意外的隱式轉換）
//   若改成「不宣告 log(int)」，log(42) 會透過轉換去呼叫 log(const string&)
//   （若存在可行的轉換路徑），行為完全不同。
//   本檔實測示範了這個差異。
//
// 【3. 常見用途】
//   (a) 禁止預設建構：「沒有這些資訊，這個物件不該存在」
//   (b) 禁止複製：Singleton、持有 socket/檔案 handle 的 RAII 型別
//           T(const T&) = delete;  T& operator=(const T&) = delete;
//   (c) 禁止特定的隱式轉換多載（上面的 log(int) 例子）
//   (d) 禁止在 heap 配置：static void* operator new(size_t) = delete;
//   在 C++11 之前，(b) 的作法是「宣告成 private 且不定義」，
//   錯誤訊息很難懂（連結期才報錯）。= delete 讓它變成清楚的編譯期錯誤。
//
// 【4. 什麼時候該用 = delete，什麼時候該給預設值】
//   判準還是那句話：**這個型別存在「零值／空白狀態」這個合理概念嗎？**
//       Point、Color、Counter、Metrics → 有，(0,0)／黑色／0 都說得通 → 給預設值
//       DatabaseConnection、FileHandle、Mutex、SensorChannel → 沒有
//           → 用 = delete 明確禁止
//   硬給一個「空的連線」或「沒開的檔案」當預設狀態，等於製造出
//   一個永遠要檢查 isValid() 的型別，把編譯期能擋的錯誤推遲到執行期。
//
// 【概念補充 Concept Deep Dive】
//   ▍= delete 可用於任何函式，不限特殊成員
//     它是通用的語言機制：任意函式、任意多載都能刪除。
//
//   ▍deleted 與 private 的差別
//     private 是「存取控制」——類別內部與 friend 仍然可以呼叫；
//     deleted 是「這個函式不存在可用的定義」——任何地方呼叫都失敗。
//     而且存取控制的檢查發生在多載解析**之後**，所以錯誤訊息也不同。
//
//   ▍is_default_constructible 會回報 false
//     = delete 之後，std::is_default_constructible<T>::value 是 false，
//     因此模板程式碼可以用 if constexpr 或 SFINAE 去適應它，
//     而不會在實例化時才炸開。本檔以 static_assert 驗證。
//
// 【注意事項 Pay Attention】
//   1. deleted function 仍參與多載解析——被選中才報錯，這是特性不是缺陷。
//   2. = delete 要寫在第一次宣告處；不能先宣告再於別處 delete。
//   3. 禁止複製要同時處理 copy constructor 與 copy assignment（還有 move 版本）。
//   4. 不要為了「讓它能放進容器」而移除 = delete——改用 emplace_back 或 unique_ptr。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】= delete
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 既然宣告了帶參 constructor 就沒有預設 constructor 了，為什麼還要 = delete？
//     答：為了表達意圖並改善錯誤訊息。沒寫時是「no matching function」，
//         讀起來像疏漏，維護者可能好心補一個；寫了 = delete 則是
//         「use of deleted function」，明確告訴所有人這是刻意禁止的。
//         同時 is_default_constructible<T> 會回報 false，模板程式碼可據此分支。
//     追問：C++11 之前怎麼做？→ 宣告成 private 且不提供定義，
//         錯誤要到連結期才出現且訊息晦澀；= delete 讓它變成清楚的編譯期錯誤。
//
// 🔥 Q2. deleted function 還會參與多載解析嗎？這有什麼用？
//     答：會，而且這正是它的價值。因為仍在多載集合中，可以用來精準攔截
//         特定呼叫：void log(int) = delete; 能擋掉 log(42)，
//         而「不宣告 log(int)」時 log(42) 反而會透過隱式轉換去呼叫別的多載。
//         兩者結果完全不同。
//     追問：那和 private 有什麼差別？→ private 是存取控制，類別內部與 friend
//         仍可呼叫，且檢查發生在多載解析之後；deleted 是任何地方都不可呼叫。
//
// ⚠️ 陷阱. 型別用了 = delete 禁止預設建構，結果 vector<T> v(3); 編不過，
//         於是想把 = delete 拿掉。
//     答：拿掉等於推翻原本的設計決定，讓「沒有連線資訊的連線物件」重新變得合法。
//         正解是改用 v.reserve(3) + v.emplace_back(...) 逐一提供引數，
//         或改存 std::optional<T> / std::unique_ptr<T>。
//     為什麼會錯：把編譯錯誤當成「阻礙」而不是「設計在對你說話」。
//         這個錯誤正確地指出：你要求建立一個沒有意義的物件。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>
using namespace std;

class DatabaseConnection {
private:
    string host;
    int    port;

public:
    // 明確禁止預設建構：沒有連接資訊就不該存在這個物件。
    // 這不只是「讓它編不過」，而是把設計意圖寫進程式碼：
    // 未來的維護者看到 use of deleted function，就知道這是刻意的。
    DatabaseConnection() = delete;

    DatabaseConnection(string h, int p) {
        host = h;
        port = p;
        cout << "連接到 " << host << ":" << port << endl;
    }

    void print() const {
        cout << "  資料庫連接: " << host << ":" << port << endl;
    }
};

// 編譯期事實查核
static_assert(!std::is_default_constructible<DatabaseConnection>::value,
              "= delete 之後不可預設建構，模板程式碼可據此分支");

// -----------------------------------------------------------------------------
// = delete 的第二個用途：禁止複製（RAII 型別的標準作法）
// -----------------------------------------------------------------------------
class FileHandle {
private:
    string name_;
    bool   open_ = false;

public:
    explicit FileHandle(const string& name) : name_(name), open_(true) {
        cout << "  [開啟] " << name_ << endl;
    }
    ~FileHandle() {
        if (open_) cout << "  [關閉] " << name_ << endl;
    }

    // 禁止複製：兩個物件持有同一個檔案 handle 會造成重複關閉
    FileHandle(const FileHandle&)            = delete;
    FileHandle& operator=(const FileHandle&) = delete;

    // 允許移動：所有權可以轉移
    FileHandle(FileHandle&& other) noexcept
        : name_(std::move(other.name_)), open_(other.open_) {
        other.open_ = false;                 // 來源放棄所有權
    }

    const string& name() const { return name_; }
};

// -----------------------------------------------------------------------------
// = delete 的第三個用途：攔截特定多載，避免意外的隱式轉換
// -----------------------------------------------------------------------------
void logMessage(const string& msg) {
    cout << "  [LOG] " << msg << endl;
}
// 禁止 logMessage(42) 這種呼叫：整數不是訊息，多半是傳錯參數了
void logMessage(int) = delete;

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】從缺
//   理由：= delete 表達的是「這個操作不該存在」這種 API 設計意圖，
//         由編譯器在編譯期強制執行。LeetCode 只比對執行結果，
//         不會、也無法評測「某個 constructor 有沒有被正確禁止」。
//         本課的設計題實作在 7.cpp（705 Design HashSet）與 summary.cpp。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】互斥鎖守衛（LockGuard）：不可複製是正確性的一部分
//   情境：RAII 的鎖守衛在 constructor 上鎖、destructor 解鎖。
//         若它可以被複製，兩個守衛物件會對同一把鎖各解一次——
//         第二次解鎖是 undefined behavior，而且這種 bug 只在特定時序下出現，
//         幾乎不可能靠測試找到。
//   作法：= delete 掉 copy constructor 與 copy assignment，
//         讓「寫出這個 bug」在編譯期就不可能。
//   這是 = delete 最有價值的用途：把正確性從執行期的祈禱，變成編譯期的保證。
// -----------------------------------------------------------------------------
class FakeMutex {                        // 為了輸出可重現，用一個假的鎖示範
private:
    bool locked_ = false;
    string name_;
public:
    explicit FakeMutex(const string& n) : name_(n) {}
    void lock()   { locked_ = true;  cout << "    [lock]   " << name_ << endl; }
    void unlock() { locked_ = false; cout << "    [unlock] " << name_ << endl; }
    bool locked() const { return locked_; }
};

class LockGuard {
private:
    FakeMutex& m_;
public:
    explicit LockGuard(FakeMutex& m) : m_(m) { m_.lock(); }
    ~LockGuard() { m_.unlock(); }

    // 若允許複製，離開 scope 時會解鎖兩次 → UB
    LockGuard(const LockGuard&)            = delete;
    LockGuard& operator=(const LockGuard&) = delete;
};

int main() {
    cout << "=== = delete 禁止預設建構 ===" << endl;
    // DatabaseConnection db;   // error: use of deleted function
    //                          //        'DatabaseConnection::DatabaseConnection()'
    cout << "  DatabaseConnection db;  → use of deleted function（刻意禁止，非疏漏）" << endl;
    cout << boolalpha;
    cout << "  is_default_constructible<DatabaseConnection> = "
         << std::is_default_constructible<DatabaseConnection>::value << endl;

    DatabaseConnection db("localhost", 5432);   // OK
    db.print();

    cout << "\n=== 需要多個連線時：用 emplace_back，不要移除 = delete ===" << endl;
    vector<DatabaseConnection> pool;
    pool.reserve(2);                    // reserve 不需要 default constructor
    pool.emplace_back("db-primary", 5432);
    pool.emplace_back("db-replica", 5433);
    for (const auto& c : pool) c.print();
    cout << "  （vector<DatabaseConnection> pool(2); 才會編譯失敗）" << endl;

    cout << "\n=== = delete 禁止複製：RAII 型別的標準作法 ===" << endl;
    {
        FileHandle f("data.txt");
        // FileHandle g = f;            // error: use of deleted function
        cout << "  FileHandle g = f;    → use of deleted function（避免重複關閉）" << endl;
        FileHandle moved = std::move(f);   // 移動是允許的：所有權轉移
        cout << "  移動之後由 moved 持有: " << moved.name() << endl;
    }   // 只會看到一次「關閉」

    cout << "\n=== = delete 攔截多載：deleted function 仍參與解析 ===" << endl;
    logMessage("服務啟動完成");
    // logMessage(42);                  // error: use of deleted function 'void logMessage(int)'
    cout << "  logMessage(42);      → use of deleted function" << endl;
    cout << "  若改成「不宣告 logMessage(int)」，42 反而可能透過隱式轉換" << endl;
    cout << "  被別的多載接走——結果完全不同。這正是 deleted 仍參與解析的價值。" << endl;

    cout << "\n=== 日常實務：不可複製的鎖守衛 ===" << endl;
    FakeMutex m("cache-mutex");
    {
        LockGuard g(m);
        cout << "    臨界區內，locked = " << m.locked() << endl;
        // LockGuard g2 = g;            // error: use of deleted function
        cout << "    LockGuard g2 = g; → 編譯失敗，因此「解鎖兩次」寫不出來" << endl;
    }   // 離開 scope，剛好解鎖一次
    cout << "  離開 scope 後 locked = " << m.locked() << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 14 課：預設建構函數（Default Constructor）9.cpp" -o def9

// === 預期輸出 ===
// === = delete 禁止預設建構 ===
//   DatabaseConnection db;  → use of deleted function（刻意禁止，非疏漏）
//   is_default_constructible<DatabaseConnection> = false
// 連接到 localhost:5432
//   資料庫連接: localhost:5432
//
// === 需要多個連線時：用 emplace_back，不要移除 = delete ===
// 連接到 db-primary:5432
// 連接到 db-replica:5433
//   資料庫連接: db-primary:5432
//   資料庫連接: db-replica:5433
//   （vector<DatabaseConnection> pool(2); 才會編譯失敗）
//
// === = delete 禁止複製：RAII 型別的標準作法 ===
//   [開啟] data.txt
//   FileHandle g = f;    → use of deleted function（避免重複關閉）
//   移動之後由 moved 持有: data.txt
//   [關閉] data.txt
//
// === = delete 攔截多載：deleted function 仍參與解析 ===
//   [LOG] 服務啟動完成
//   logMessage(42);      → use of deleted function
//   若改成「不宣告 logMessage(int)」，42 反而可能透過隱式轉換
//   被別的多載接走——結果完全不同。這正是 deleted 仍參與解析的價值。
//
// === 日常實務：不可複製的鎖守衛 ===
//     [lock]   cache-mutex
//     臨界區內，locked = true
//     LockGuard g2 = g; → 編譯失敗，因此「解鎖兩次」寫不出來
//     [unlock] cache-mutex
//   離開 scope 後 locked = false
