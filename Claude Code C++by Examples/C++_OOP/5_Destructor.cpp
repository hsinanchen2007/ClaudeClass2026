/*=============================================================================
 * 檔名：5_Destructor.cpp
 * 主題：解構子 (Destructor) - 物件死亡時自動執行的清理函式
 * 適合：已會建構子，想知道「物件結束時會發生什麼事」的初學者
 *
 * 【課題介紹】
 *   建構子處理「物件出生時」的事，那物件結束、被丟棄時呢？
 *   答案就是：解構子 (Destructor)。
 *
 *       「解構子是另一個特殊的成員函式，物件被銷毀前 C++ 會自動呼叫它。」
 *
 *   什麼時候會被呼叫？
 *     1. 區域物件 (local object)：離開所在的 { ... } 區塊時。
 *     2. 用 new 配置的物件：在你呼叫 delete 時。
 *     3. 容器內的物件：容器被銷毀時，每個元素的解構子都會被呼叫。
 *     4. 程式結束時：全域物件 (global object) 也會自動解構。
 *
 *   為什麼重要？因為很多資源 (檔案、網路連線、鎖、heap 記憶體) 都需要
 *   「用完一定要釋放」，否則會洩漏。寫在解構子裡 → C++ 自動幫你呼叫，
 *   不會忘記。這個概念之後會變成第 19 篇的主題：RAII。
 *
 * 【解構子的規則】
 *   1. 函式名 = ~ClassName    (波浪號 + 類別名)
 *   2. 不寫回傳型別、也不能吃任何參數，所以不能多載 (一個類別只有一個解構子)。
 *   3. 你不寫的話，編譯器會幫你產一個什麼都不做的預設解構子。
 *   4. 銷毀順序：與建構順序相反 (後建後死 → LIFO)。
 *
 * 【建構/解構順序示意】
 *   {
 *       A a;     // 建構 a
 *       B b;     // 建構 b
 *       C c;     // 建構 c
 *   }            // 離開區塊 → 先解構 c，再 b，再 a (反序)
 *
 * 【日常實用範例】
 *   工作上很常需要「打開資源 → 用 → 一定要關起來」的模式：
 *     - 打開檔案、log → 一定要關
 *     - 拿到 mutex 鎖 → 一定要釋放
 *     - 連到資料庫 → 一定要 disconnect
 *   把開資源寫在建構子、關資源寫在解構子，就能讓 C++ 自動幫你管。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/language/destructor
 *   https://cplusplus.com/doc/tutorial/classes2/   (Destructors)
 *=============================================================================*/

/*
補充筆記：Destructor
  - Destructor 這類 OOP 範例要追蹤物件狀態：建構後是否有效、操作後是否仍符合類別承諾。
  - 如果類別擁有資源，就要檢查 destructor、copy、move 是否表達同一套所有權規則。
  - 繼承、friend、static、operator overload 都應服務於清楚的物件語意，而不是只展示語法。
  - destructor 是物件生命週期的結束點，名稱是 ~ClassName()，沒有參數也沒有回傳型別；它會在 automatic object 離開 scope 時自動呼叫。
  - 解構子的主要責任是釋放物件擁有的資源，例如動態記憶體、檔案、mutex lock、socket；如果物件沒有直接擁有資源，通常不需要手寫解構子。
  - C++ 的解構順序和建構順序相反：先跑類別自己的解構子本體，再依成員宣告的反順序解構成員，再解構 base class。
  - 解構子不應拋出例外；若 stack unwinding 時又有解構子拋例外，程式可能呼叫 std::terminate。
  - delete 指標時會呼叫指向物件的解構子並釋放記憶體；只呼叫解構子不等於釋放配置取得的 storage，這兩件事概念上要分清楚。
  - 手寫 destructor 往往代表類別管理資源，因此也要檢查 copy constructor 和 copy assignment 是否會造成 double delete 或淺拷貝問題。
  - 現代 C++ 優先把資源交給 std::unique_ptr、std::vector、std::string、std::fstream 等 RAII 成員，讓編譯器產生的解構子就足夠。
  - 觀察解構子輸出時要記得暫時物件、容器元素、例外離開 scope 都會觸發解構；不要只用 main 結尾才會解構的直覺理解。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】解構子（Destructor）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. destructor 什麼時候被呼叫？順序如何？
//     答：區域物件離開 scope、對動態物件 delete、容器或母物件被銷毀、程式結束時的
//     全域物件；例外造成的 stack unwinding 也會呼叫「已完整建構」物件的 destructor —
//     這正是 RAII 能提供 exception safety 的原因。
//     順序是建構的完全反序：自己的本體 → 成員（宣告的反序）→ base class。
//     追問：什麼時候「需要」自己手寫 destructor？（類別直接持有裸資源時；而一旦手寫了，
//     copy constructor 與 copy assignment 通常也要一起處理 — 這就是 Rule of Three）
//
// 🔥 Q2. exception 從 destructor 拋出去會怎樣？
//     答：C++11 起 destructor 預設是 noexcept，exception 逃出去會直接呼叫 std::terminate；
//     若又發生在 stack unwinding 過程中（已有一個 exception 在飛）同樣是 terminate。
//     原則：destructor 絕不外拋，內部要 try/catch 吞掉或記錄。
//
// Q3. 在 destructor 裡呼叫 virtual function 會發生什麼？
//     答：與 constructor 對稱 — 進到 Base 的 destructor 時，derived 的部分已經解構完畢，
//     動態型別退回 Base，呼叫到的是 Base 的版本。呼叫 pure virtual 函式則是 UB。
//
// ⚠️ 陷阱. 用 `new[]` 配置、卻用 `delete` 釋放會怎樣？
//     答：undefined behavior。`new`/`delete` 與 `new[]`/`delete[]` 必須成對使用。
//     同理，`std::unique_ptr<T>` 不能拿來管理 `new[]` 的結果，要用 `std::unique_ptr<T[]>`。
//     為什麼會錯：以為「反正都是把那塊記憶體還回去」；實際上陣列版必須知道元素個數才能
//     逐一呼叫 destructor，兩者走的是不同的釋放路徑。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <fstream>     // std::ofstream 寫檔
#include <string>

// -----------------------------------------------------------------------------
// 範例 1：最簡單的解構子 - 看它什麼時候被呼叫
// -----------------------------------------------------------------------------
class Loud {
private:
    std::string name_;
public:
    Loud(const std::string& n) : name_(n) {        // 初始化列表，第 7 篇詳述
        std::cout << "[Loud] " << name_ << " 出生" << std::endl;
    }
    ~Loud() {                                       // 解構子：~ClassName，無參數無回傳
        std::cout << "[Loud] " << name_ << " 死亡" << std::endl;
    }
};

// -----------------------------------------------------------------------------
// 範例 2：日常實用 - 自動關檔的 LogWriter
// -----------------------------------------------------------------------------
// 沒有解構子的話，我們得：
//     std::ofstream f("log.txt");
//     f << "...";
//     f.close();        // 萬一忘記寫這行就糟糕
// 把它包成一個類別，建構時開檔、解構時關檔，使用者就完全不用煩惱關檔的事。
class LogWriter {
private:
    std::ofstream file_;
    std::string   path_;

public:
    explicit LogWriter(const std::string& path) : path_(path) {
        // explicit 是讓建構子「不被隱式轉換呼叫」，避免 LogWriter w = "x.txt"; 這種意外
        file_.open(path_);
        if (file_.is_open())
            std::cout << "[LogWriter] 開啟 " << path_ << std::endl;
        else
            std::cout << "[LogWriter] 開檔失敗: " << path_ << std::endl;
    }

    ~LogWriter() {
        if (file_.is_open()) {
            file_.close();
            std::cout << "[LogWriter] 自動關閉 " << path_ << std::endl;
        }
    }

    void write(const std::string& msg) {
        if (file_.is_open()) file_ << msg << '\n';
    }
};

// -----------------------------------------------------------------------------
// 範例 3：示範「建構/解構順序相反」
// -----------------------------------------------------------------------------
void demoOrder() {
    std::cout << "--- 進入 demoOrder() ---" << std::endl;
    Loud a("A");
    Loud b("B");
    Loud c("C");
    std::cout << "--- 即將離開 demoOrder() ---" << std::endl;
    // 離開時：C → B → A 反序解構
}

// -----------------------------------------------------------------------------
// 範例 5：對應 Leetcode 1603 - Design Parking System (用解構子展示「離場結算」)
// -----------------------------------------------------------------------------
// 把 ParkingSystem 加上解構子：當物件被銷毀時印出「最終剩餘車位」報表。
// 工作上很實用 — 例如 服務結束時要列出統計資訊。
class ParkingLot {
private:
    int big_;
    int medium_;
    int small_;
    int totalIn_;             // 累計成功停入幾輛

public:
    ParkingLot(int b, int m, int s) : big_(b), medium_(m), small_(s), totalIn_(0) {
        std::cout << "[ParkingLot] 開幕，車位 b=" << big_
                  << " m=" << medium_ << " s=" << small_ << std::endl;
    }

    ~ParkingLot() {
        // 解構時自動印出統計結果
        std::cout << "[ParkingLot] 打烊，總共停入 " << totalIn_ << " 輛車"
                  << "，剩餘: b=" << big_ << " m=" << medium_ << " s=" << small_ << std::endl;
    }

    bool addCar(int type) {
        if (type == 1 && big_ > 0)    { --big_;    ++totalIn_; return true; }
        if (type == 2 && medium_ > 0) { --medium_; ++totalIn_; return true; }
        if (type == 3 && small_ > 0)  { --small_;  ++totalIn_; return true; }
        return false;
    }
};

// -----------------------------------------------------------------------------
// 範例 6：日常實用 - DBConnection 自動斷線
// -----------------------------------------------------------------------------
// 連到資料庫之後，無論程式怎麼結束 (return, throw) 都必須斷線；
// 寫在解構子裡是最保險的方式。
class DBConnection {
private:
    std::string host_;
    bool        connected_;

public:
    explicit DBConnection(const std::string& host) : host_(host), connected_(true) {
        std::cout << "[DB] 已連線到 " << host_ << std::endl;
    }

    ~DBConnection() {
        if (connected_) {
            std::cout << "[DB] 自動斷線: " << host_ << std::endl;
            connected_ = false;
        }
    }

    void query(const std::string& sql) {
        std::cout << "[DB] 執行: " << sql << std::endl;
    }
};

int main() {
    std::cout << "===== 範例 1+3：解構順序 =====" << std::endl;
    demoOrder();           // 在這個函式裡會看到 LIFO 解構順序

    std::cout << "===== 範例 2：LogWriter 自動關檔 =====" << std::endl;
    {
        // 用一個額外的 { } 區塊製造「離開時自動解構」的時機
        LogWriter lw("demo_log.txt");
        lw.write("Hello");
        lw.write("World");
        // 離開這個 { } → lw 解構 → 檔案自動關閉
    }
    std::cout << "離開區塊，檔案已自動關閉，請看到上面 [LogWriter] 自動關閉 訊息。"
              << std::endl;

    std::cout << "===== 範例 4：用 new/delete 控制生命週期 =====" << std::endl;
    Loud* p = new Loud("Heap-Object");      // 在 heap 上配置
    std::cout << "(p 還沒 delete，所以還沒解構)" << std::endl;
    delete p;                                // 手動釋放 → 解構子在這時被呼叫
    // ※ 用 new/delete 容易忘記釋放 → 第 20 篇會學 unique_ptr 自動管理。

    std::cout << "===== 範例 5：ParkingLot (LC1603) 解構時印報表 =====" << std::endl;
    {
        ParkingLot lot(1, 1, 2);
        lot.addCar(1);
        lot.addCar(3);
        lot.addCar(2);
        lot.addCar(1);   // big 已滿 → false
        // 離開大括號 → lot 解構 → 自動印報表
    }

    std::cout << "===== 範例 6：DBConnection 自動斷線 =====" << std::endl;
    {
        DBConnection db("db.example.com:5432");
        db.query("SELECT * FROM users WHERE id=1");
        // 離開區塊 → 自動斷線
    }

    std::cout << "===== main 即將結束 =====" << std::endl;
    return 0;
}

/* 預期輸出（demo_log.txt 會被建立，內容為 "Hello\nWorld\n"）:
 * ===== 範例 1+3：解構順序 =====
 * --- 進入 demoOrder() ---
 * [Loud] A 出生
 * [Loud] B 出生
 * [Loud] C 出生
 * --- 即將離開 demoOrder() ---
 * [Loud] C 死亡
 * [Loud] B 死亡
 * [Loud] A 死亡
 * ===== 範例 2：LogWriter 自動關檔 =====
 * [LogWriter] 開啟 demo_log.txt
 * [LogWriter] 自動關閉 demo_log.txt
 * 離開區塊，檔案已自動關閉，請看到上面 [LogWriter] 自動關閉 訊息。
 * ===== 範例 4：用 new/delete 控制生命週期 =====
 * [Loud] Heap-Object 出生
 * (p 還沒 delete，所以還沒解構)
 * [Loud] Heap-Object 死亡
 * ===== 範例 5：ParkingLot (LC1603) 解構時印報表 =====
 * [ParkingLot] 開幕，車位 b=1 m=1 s=2
 * [ParkingLot] 打烊，總共停入 3 輛車，剩餘: b=0 m=0 s=1
 * ===== 範例 6：DBConnection 自動斷線 =====
 * [DB] 已連線到 db.example.com:5432
 * [DB] 執行: SELECT * FROM users WHERE id=1
 * [DB] 自動斷線: db.example.com:5432
 * ===== main 即將結束 =====
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. 解構子名 ~ClassName，無參數無回傳，每個類別最多一個。
 *   2. 物件離開作用域、被 delete、容器銷毀時，解構子自動被呼叫。
 *   3. 一群物件的解構順序與建構順序「相反」(LIFO)。
 *   4. 用建構子拿資源、解構子放資源，使用者就不會忘記清理 → RAII 的雛形。
 *
 * 【下一篇預告】
 *   6_ThisPointer.cpp
 *   this 指標 — 物件「指向自己」的方法，
 *   並用 Leetcode 1768. Merge Strings Alternately 練習。
 *=============================================================================*/
