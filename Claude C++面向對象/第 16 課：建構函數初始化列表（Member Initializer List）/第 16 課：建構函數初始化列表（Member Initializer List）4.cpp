// =============================================================================
//  第 16 課：初始化列表 4  —  必用場景之三：沒有預設建構函數的成員
// =============================================================================
//
// 【主題資訊 Information】
//   情境：成員的型別只提供帶參建構函數，沒有無參數版本
//   語法：  class Car { Engine engine; public: Car(int hp) : engine(hp, "汽油") {} };
//   標準版本：C++98 起即有
//   複雜度：O(1)；成員直接就地建構，不產生臨時物件
//   標頭檔：<string>
//
// 【詳細解釋 Explanation】
//
// 【1. 「預設建構函數什麼時候會消失」】
//   這是很多人踩到的第一個坑：
//     ● 你**完全沒寫**任何建構函數 → 編譯器自動生成一個預設建構函數
//     ● 你只要寫了**任何一個**建構函數（哪怕是帶參的）
//       → 編譯器**不再**自動生成預設建構函數
//   本檔的 Engine 只寫了 Engine(int, const string&)，
//   所以 Engine 從此沒有無參數建構函數，Engine e; 會直接編譯失敗。
//
// 【2. 為什麼函數體內賦值救不了】
//   錯誤寫法：
//       Car(const string& b, int hp, const string& fuel) {
//           brand = b;
//           engine = Engine(hp, fuel);   // 編譯錯誤
//       }
//   問題不在等號右邊——Engine(hp, fuel) 本身完全合法。
//   問題在於：**進入函數體之前，成員 engine 就必須已經被初始化完成**。
//   編譯器此時會試著呼叫 Engine 的預設建構函數，但根本沒有這個東西：
//       error: no matching function for call to 'Engine::Engine()'
//   也就是說，程式在跑到你那行賦值之前就已經編不過了。
//
// 【3. 正確寫法：在初始化列表把參數轉交給成員的建構函數】
//       Car(const string& b, int hp, const string& fuel)
//           : brand(b), engine(hp, fuel)
//   engine(hp, fuel) 的意思是「直接用這兩個引數呼叫 Engine 的建構函數，
//   就地把 engine 建出來」。沒有預設建構、沒有臨時物件、沒有賦值——
//   一步到位，而且這是唯一可行的寫法。
//
// 【4. 建構順序：由內而外，解構則相反】
//   建立一個 Car 時的順序是：
//       brand 建構 → engine 建構 → Car 的函數體執行
//   解構時則完全相反：
//       Car 的解構函數體 → engine 解構 → brand 解構
//   「成員先建好，外層才開始工作；外層先收工，成員才拆除」——
//   這保證了函數體執行期間所有成員都處於有效狀態。
//   本檔的輸出可以直接看到「引擎建構」印在「汽車建構」之前。
//
// 【5. 這其實是「組合（composition）」的基本功】
//   Car 內含一個 Engine，這是 has-a 關係（組合），不是 is-a（繼承）。
//   組合是 OOP 中比繼承更常用、耦合更低的複用方式。
//   而只要被組合的型別沒有預設建構函數，初始化列表就是必需品。
//   反過來說，**刻意不提供預設建構函數**是一種設計手段：
//   它強迫使用者在建立物件時就給齊必要資訊，讓「半成品物件」無法存在。
//
// 【概念補充 Concept Deep Dive】
//
//   ● 三種讓成員「沒有預設建構函數」的情況
//     (a) 只定義了帶參建構函數（本檔的 Engine）
//     (b) 明確寫了 Engine() = delete;
//     (c) 型別本身就沒有，例如參考成員、const 成員（前兩個檔案）
//     不論哪一種，解法都一樣：在初始化列表指定怎麼建。
//
//   ● 如果真的需要「延後決定」怎麼辦
//     有時你在建構 Car 時還不知道該裝什麼引擎。此時不要硬加預設建構函數
//     （那會製造出無效狀態），而應該改變成員型別：
//         std::optional<Engine> engine_;    // C++17：可以是「還沒有」
//         std::unique_ptr<Engine> engine_;  // 用指標，可為 nullptr、可更換
//     這兩種寫法都誠實地把「可能還沒有」表達在型別上。
//
//   ● 為什麼寫了帶參建構函數就會失去預設建構函數
//     語言的假設是：既然你開始自訂建構方式，就代表這個型別有「必要的
//     初始化資訊」，編譯器不應該再擅自提供一個什麼都不設定的版本。
//     若你確實兩者都要，可以明確寫回來：Engine() = default;
//
//   ● 成員初始化列表 vs 委派建構函數
//     若多個建構函數的初始化邏輯重複，C++11 起可以委派：
//         Car(const string& b) : Car(b, 150, "汽油") { }
//     注意一旦委派給另一個建構函數，該初始化列表就不能再初始化其他成員。
//
// 【注意事項 Pay Attention】
//   1. 成員的建構永遠早於外層建構函數的函數體；函數體內只能賦值。
//   2. 只要寫了任何建構函數，預設建構函數就消失；需要就寫 = default。
//   3. 成員初始化順序看宣告順序，不看初始化列表的書寫順序。
//   4. 「還沒決定」的成員請用 std::optional 或智慧指標表達，
//      不要為此硬加一個會產生無效狀態的預設建構函數。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】沒有預設建構函數的成員
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 類別成員的型別沒有預設建構函數時，為什麼不能在建構函數本體內賦值？
//     答：因為所有成員必須在「進入函數體之前」就完成初始化。編譯器此時會
//         嘗試呼叫該成員的預設建構函數，但它不存在，於是在跑到你那行賦值
//         之前就已經編譯失敗（no matching function for call to Engine::Engine()）。
//         唯一解法是在初始化列表指定要用哪個建構函數。
//     追問：什麼情況會讓一個類別失去預設建構函數？
//         → 只要自己寫了任何一個建構函數，編譯器就不再自動生成預設版本；
//           也可能是被明確 = delete，或含有參考／const 成員。
//
// 🔥 Q2. Car 內含 Engine，建構與解構的順序分別是什麼？
//     答：建構是「成員先、外層後」：brand → engine → Car 函數體。
//         解構完全相反：Car 解構函數體 → engine → brand。
//         這保證外層函數體執行期間，所有成員都已建好且尚未被拆除。
//     追問：成員之間的順序由什麼決定？
//         → 由**宣告順序**決定，與初始化列表的書寫順序無關。
//
// ⚠️ 陷阱. 成員沒有預設建構函數很麻煩，那我幫它加一個預設建構函數就好了吧？
//     答：這通常是錯誤的修法。「沒有預設建構函數」往往是設計上的**刻意選擇**：
//         它保證 Engine 一旦存在就一定有馬力與燃料類型，不會有半成品。
//         硬加一個預設建構函數等於製造出一個「合法但無意義」的狀態，
//         把編譯期就能擋下的錯誤延後成執行期的資料異常。
//     為什麼會錯：把編譯錯誤當成「阻礙」而不是「訊號」。編譯器其實在說
//         「你還沒決定這個成員怎麼建」，正確回應是去初始化列表補上，
//         或若真的可能沒有，就改用 std::optional／智慧指標誠實表達。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <optional>
#include <string>
using namespace std;

class Engine {
private:
    int horsepower;
    string fuelType;

public:
    // Engine 只有帶參建構函數 → 編譯器不再提供預設建構函數
    // Engine e;  會編譯失敗：no matching function for call to 'Engine::Engine()'
    Engine(int hp, const string& fuel)
        : horsepower(hp), fuelType(fuel)
    {
        cout << "  引擎建構: " << hp << " 馬力, " << fuel << endl;
    }

    ~Engine() {
        cout << "  引擎解構: " << horsepower << " HP" << endl;
    }

    void print() const {
        cout << "  引擎: " << horsepower << " HP (" << fuelType << ")" << endl;
    }
};

class Car {
private:
    string brand;
    Engine engine;   // Engine 沒有預設建構函數！

public:
    // 錯誤寫法：
    //   Car(const string& b, int hp, const string& fuel) {
    //       brand = b;
    //       engine = Engine(hp, fuel);
    //       // 編譯錯誤：進函數體前就要初始化 engine，但它沒有預設建構函數
    //   }

    // 正確寫法：在初始化列表把參數轉交給 Engine 的建構函數
    Car(const string& b, int hp, const string& fuel)
        : brand(b), engine(hp, fuel)
    {
        cout << "  汽車建構: " << brand << endl;
    }

    ~Car() {
        cout << "  汽車解構: " << brand << endl;
    }

    void print() const {
        cout << "  品牌: " << brand << endl;
        engine.print();
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】資料庫連線池：連線物件刻意不提供預設建構函數
//   情境：DbConnection 一旦存在就必須是「已設定好連線字串」的有效連線，
//         不允許出現「連線物件存在但不知道要連哪裡」的半成品狀態。
//   重點一：Pool 用初始化列表把設定轉交給 DbConnection 的建構函數。
//   重點二：真實情況常有「尚未連線」的階段——這時不要硬加預設建構函數，
//           而是用 std::optional<DbConnection>（C++17）誠實表達「可能還沒有」。
// -----------------------------------------------------------------------------
class DbConnection {
private:
    string dsn_;
    int poolSize_;

public:
    DbConnection(const string& dsn, int poolSize)
        : dsn_(dsn), poolSize_(poolSize)
    {
        cout << "    建立連線: " << dsn_ << " (pool=" << poolSize_ << ")" << endl;
    }

    void query(const string& sql) const {
        cout << "    [" << dsn_ << "] 執行: " << sql << endl;
    }
};

class ReportService {
private:
    DbConnection primary_;                 // 一定要有 → 直接用值成員
    std::optional<DbConnection> replica_;  // 可能沒有 → 用 optional 表達

public:
    // 只有主要連線
    ReportService(const string& dsn, int poolSize)
        : primary_(dsn, poolSize), replica_(std::nullopt)
    { }

    // 主要 + 唯讀副本
    ReportService(const string& dsn, int poolSize, const string& replicaDsn)
        : primary_(dsn, poolSize)
        , replica_(std::in_place, replicaDsn, poolSize)  // 就地建構，不必先有預設值
    { }

    void runReport() const {
        // 有副本就走副本，減輕主庫負擔
        if (replica_.has_value()) {
            replica_->query("SELECT * FROM orders  -- 走唯讀副本");
        } else {
            primary_.query("SELECT * FROM orders  -- 無副本，走主庫");
        }
    }
};

int main() {
    cout << "=== 建構順序：成員先，外層後 ===" << endl;
    {
        Car car("BMW", 300, "汽油");
        cout << endl;
        car.print();
        cout << "\n--- 離開作用域，開始解構（順序相反）---" << endl;
    }

    cout << "\n=== 日常實務：只有主要連線 ===" << endl;
    {
        ReportService svc("postgres://primary/appdb", 8);
        svc.runReport();
    }

    cout << "\n=== 日常實務：主要 + 唯讀副本 ===" << endl;
    {
        ReportService svc("postgres://primary/appdb", 8,
                          "postgres://replica/appdb");
        svc.runReport();
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 16 課：建構函數初始化列表（Member Initializer List）4.cpp" -o demo4
//   （本檔用到 std::optional 與 std::in_place，兩者皆為 C++17）

// === 預期輸出 ===
// === 建構順序：成員先，外層後 ===
//   引擎建構: 300 馬力, 汽油
//   汽車建構: BMW
//
//   品牌: BMW
//   引擎: 300 HP (汽油)
//
// --- 離開作用域，開始解構（順序相反）---
//   汽車解構: BMW
//   引擎解構: 300 HP
//
// === 日常實務：只有主要連線 ===
//     建立連線: postgres://primary/appdb (pool=8)
//     [postgres://primary/appdb] 執行: SELECT * FROM orders  -- 無副本，走主庫
//
// === 日常實務：主要 + 唯讀副本 ===
//     建立連線: postgres://primary/appdb (pool=8)
//     建立連線: postgres://replica/appdb (pool=8)
//     [postgres://replica/appdb] 執行: SELECT * FROM orders  -- 走唯讀副本
