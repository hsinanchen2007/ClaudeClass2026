// =============================================================================
//  第 13 課：建構函數（Constructor）基礎  —  總複習
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  class X {
//           public:
//               X();                          // 預設建構函式
//               X(T a, U b) : m_a(a), m_b(b) {}   // 帶參 + 成員初始化列表
//           };
//   規則：  函式名 = 類別名；無回傳型別；物件建立時自動呼叫；可多載
//   標準版本：C++98 起即有建構函式與初始化列表；
//             委派建構函式（delegating ctor）與 NSDMI 需 C++11；
//             本檔的 std::optional 需 C++17
//   標頭檔：<string>、<vector>、<optional>
//
// 【詳細解釋 Explanation】
//
// 【1. 建構函式解決的根本問題：消滅「已建立但未初始化」的空窗期】
//   C 的模式把「配置記憶體」與「填入有意義的值」拆成兩步：
//       Student s;              // 記憶體有了，內容不確定
//       student_init(&s, ...);  // 到這裡才合法
//   兩步之間的任何使用都是 UB，而編譯器完全不會提醒。
//   建構函式讓兩者合而為一 —— 只要物件誕生，初始化就一定發生，
//   不論它建在堆疊、heap、陣列裡，還是當成別的類別的成員。
//   這是從「靠紀律」升級到「靠型別系統」的關鍵一步。
//
// 【2. 三條語法規則背後的原因】
//   * 函式名 = 類別名：編譯器辨識建構函式的唯一依據（沒有專用關鍵字）
//   * 無回傳型別：它的「回傳值」就是物件本身，由語言直接處理。
//     寫成 `void Student()` 會變成一個剛好同名的普通成員函式，
//     物件建立時不會被呼叫 —— 這個錯誤非常隱蔽。
//   * 自動呼叫：這是它全部價值的來源，你無法忘記也無法跳過。
//
// 【3. 初始化列表 vs 本體內賦值 —— 本課最重要的實作細節】
//       Student() : name("未命名") {}   // 初始化：直接以目標值建構，一次到位
//       Student() { name = "未命名"; }  // 賦值：先預設建構，再賦值，做了兩次
//   對 std::string / std::vector 這類型別是實際的效能差異；
//   而 const 成員、參考成員、無預設建構函式的類別成員「只能」用初始化列表，
//   在本體內賦值會直接編譯失敗。
//   ⚠️ 初始化順序由「宣告順序」決定，與列表書寫順序無關 —— 見下方陷阱題。
//
// 【4. 四種建構時機（= 四種儲存期）】
//   * 全域／static：main() 之前建構，程式結束後解構
//   * 區域物件：執行到該行時建構，離開作用域時解構
//   * 區塊內物件：同上，但作用域是那對大括號
//   * new 出來的：new 時建構、delete 時解構 —— 只有這種要你自己負責
//   同一作用域內，解構順序嚴格反向（LIFO），因為後者可能依賴前者。
//
// 【5. 多載與驗證】
//   建構函式可多載，依參數個數與型別挑選。一旦你寫了任何建構函式，
//   編譯器就不再自動生成預設建構函式（需要就自己補，或用 = default，第 14 課）。
//   驗證應集中在「完整版」建構函式，其餘版本委派過去 ——
//   重複的驗證邏輯必然會在日後修改時產生不一致。
//
// 【概念補充 Concept Deep Dive】
//   * 建構函式本體執行「之前」，所有成員都已完成初始化。因此本體內的 `=`
//     一律是賦值運算子，不是初始化 —— 這解釋了為何 const 成員在那裡會失敗。
//   * 建構函式不能是 virtual：虛擬分派需要 vptr，而 vptr 正是在建構過程中
//     才被設定好的。要「依型別建立不同物件」請用工廠模式。
//   * 建構函式丟出例外時，該物件的解構函式不會被呼叫（它從未完成建構），
//     但已完成建構的成員會被正常解構。所以在建構函式中 new 的裸指標會洩漏，
//     正解是讓成員本身就是 RAII 型別。
//   * 委派建構函式（C++11）的執行順序：先完整跑完目標建構函式，
//     再跑委派方的本體。物件在目標完成時即算建構完成。
//
// 【注意事項 Pay Attention】
//   1. Most Vexing Parse：`Student s();` 是「宣告一個回傳 Student 的函式」，
//      不是建立物件。正確寫法是 `Student s;` 或 `Student s{};`。
//   2. 成員初始化順序 = 宣告順序，不是初始化列表的順序。務必讓兩者一致，
//      並把 -Wreorder（-Wall 已含）的警告當成錯誤處理。
//   3. 建構函式絕不可寫回傳型別，寫了就不再是建構函式。
//   4. 定義任何建構函式後，預設建構函式即消失，需要就自己補回。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】建構函數基礎
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 建構函式與一般成員函式有什麼不同？
//     答：① 名稱必須與類別同名；② 沒有回傳型別；③ 物件建立時自動呼叫，
//         無法手動略過；④ 可以有成員初始化列表；⑤ 不能是 virtual、不能是 const。
//         它的意義在於保證「物件存在」與「物件合法」同時發生。
//     追問：能不能手動呼叫建構函式？→ 一般不行。但可以用 placement new
//           在既有記憶體上明確建構，那是自訂記憶體池等場景的進階用法。
//
// 🔥 Q2. 初始化列表和在建構函式本體內賦值，差別在哪？
//     答：列表是「初始化」（成員直接以目標值建構，一次到位）；
//         本體內是「賦值」（成員已先被預設建構，再被賦一次值，共兩次工作）。
//         對有建構成本的型別是實際效能差異；而 const 成員、參考成員、
//         無預設建構函式的成員「只能」用初始化列表。
//     追問：初始化順序由誰決定？→ 由成員的宣告順序決定，與列表書寫順序無關。
//
// 🔥 Q3. 什麼是 Most Vexing Parse？
//     答：`Student s();` 看起來像「用預設建構函式建立物件」，
//         實際上會被解析成「宣告一個名為 s、無參數、回傳 Student 的函式」。
//         之後對 s 取用成員就會出現看不懂的編譯錯誤。
//     追問：怎麼避免？→ 用 `Student s;`（不加括號）或 C++11 的
//           大括號初始化 `Student s{};`，後者不會有這個歧義。
//
// ⚠️ 陷阱. 下面這段程式為什麼危險？
//         class Buffer {
//             char*  m_data;      // 先宣告
//             size_t m_len;       // 後宣告
//         public:
//             Buffer(const char* s) : m_len(strlen(s)), m_data(new char[m_len + 1]) {}
//         };
//     答：成員的初始化順序由「宣告順序」決定，所以實際上是先執行
//         m_data(new char[m_len + 1])，此時 m_len 尚未初始化，
//         配置大小取決於一個不確定的值 —— 可能配置失敗、也可能配置出
//         遠小於需要的空間而在後續寫入時越界。
//     為什麼會錯：直覺以為「初始化列表怎麼寫就怎麼執行」。
//         標準固定用宣告順序，是為了讓解構能嚴格反向進行。
//         正解是讓宣告順序與使用順序一致（先宣告 m_len），
//         並且永遠不要忽略 GCC 的 -Wreorder 警告。
// ═══════════════════════════════════════════════════════════════════════════

/*
 * ================================================================
 * 【第 13 課：建構函數（Constructor）基礎】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. 建構函數的定義：對象創建時自動調用的特殊成員函數
 * 2. 建構函數的語法規則：函數名=類別名、無返回值、自動調用
 * 3. 對比 C 語言的手動初始化 vs C++ 建構函數的自動初始化
 * 4. 建構函數的四種調用時機（全域、局部、區塊、動態）
 * 5. 帶參數的建構函數
 * 6. 建構函數重載（Overloading）
 * 7. 建構函數中的資料驗證
 * 8. 常見陷阱：返回值、Most Vexing Parse、預設建構消失
 * ================================================================
 */

#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>   // std::invalid_argument（實務範例用）
using namespace std;

// ================================================================
// 重點一：為什麼需要建構函數？
// ================================================================
// 沒有建構函數時，基本型別成員（int、float 等）不會自動初始化，
// 它們的值是未定義的「垃圾值」。
// string 等類別型別因為有自己的預設建構函數，會初始化為空字串。
//
// C 語言的問題：初始化和對象創建是分離的，容易忘記調用初始化函數。
// C++ 的建構函數把這兩步合為一步，保證對象一旦創建就已被正確初始化。

// --- C 語言風格（問題示範）---
// typedef struct {
//     char name[50];
//     int age;
// } Student_C;
// void student_init(Student_C* s, const char* name, int age) {
//     strcpy(s->name, name);
//     s->age = age;
// }
// // 問題：如果忘記調用 student_init()，成員就是垃圾值，編譯器不會警告！

// ================================================================
// 重點二：建構函數的語法規則
// ================================================================
// ┌─────────────────┬──────────────────────────────────┐
// │ 特徵             │ 說明                              │
// ├─────────────────┼──────────────────────────────────┤
// │ 函數名           │ 必須與類別名完全相同              │
// │ 返回值           │ 沒有返回值，連 void 都不寫        │
// │ 調用時機         │ 對象創建時自動調用                │
// │ 可以重載         │ 一個類別可以有多個建構函數        │
// │ 存取權限         │ 通常是 public（否則外界無法建構）  │
// └─────────────────┴──────────────────────────────────┘

class Student {
private:
    string name;
    int age;
    float gpa;

public:
    // 建構函數：函數名 = 類別名，沒有返回值（連 void 都不寫）
    Student() {
        cout << ">>> 預設建構函數被調用 <<<" << endl;
        name = "未命名";
        age = 0;
        gpa = 0.0f;
    }

    // 帶參數的建構函數（重載）
    Student(string n, int a, float g) {
        name = n;
        age = a;
        gpa = g;
        cout << ">>> 帶參建構函數被調用: " << name << " <<<" << endl;
    }

    void print() const {
        cout << "  姓名: " << name
             << ", 年齡: " << age
             << ", GPA: " << gpa << endl;
    }
};

// ================================================================
// 重點三：建構函數的四種調用時機
// ================================================================
// ┌──────────────┬───────────────────────────────┐
// │ 對象類型      │ 建構時機                       │
// ├──────────────┼───────────────────────────────┤
// │ 全域對象      │ main() 之前                    │
// │ 局部對象      │ 執行到宣告語句時                │
// │ 區塊內對象    │ 進入區塊，執行到宣告時          │
// │ 動態對象      │ new 運算子執行時                │
// └──────────────┴───────────────────────────────┘

class Box {
private:
    string label;

public:
    Box() {
        label = "空盒子";
        cout << "建構函數: 創建了 [" << label << "]" << endl;
    }
    string getLabel() const { return label; }
};

// ================================================================
// 重點四：建構函數重載（Overloading）
// ================================================================
// 和普通函數一樣，建構函數支援重載——可以有多個建構函數，只要參數列表不同。
// 編譯器根據傳入的參數數量和型別，自動匹配最合適的建構函數（重載解析）。

class Rectangle {
private:
    double width;
    double height;
    string color;

public:
    // 建構函數 1：無參數 → 預設矩形
    Rectangle() {
        width = 1.0;
        height = 1.0;
        color = "白色";
    }

    // 建構函數 2：正方形（只給一個邊長）
    Rectangle(double side) {
        width = side;
        height = side;
        color = "白色";
    }

    // 建構函數 3：指定寬高
    Rectangle(double w, double h) {
        width = w;
        height = h;
        color = "白色";
    }

    // 建構函數 4：指定寬高和顏色
    Rectangle(double w, double h, string c) {
        width = w;
        height = h;
        color = c;
    }

    void print() const {
        cout << "  " << color << " 矩形: "
             << width << " x " << height
             << ", 面積 = " << width * height << endl;
    }
};

// ================================================================
// 重點五：建構函數中的資料驗證
// ================================================================
// 建構函數不僅用來賦值，還可以加入驗證邏輯，
// 確保對象從一開始就處於合法狀態。
// 建構函數是防止非法對象存在的「第一道防線」。

class BankAccount {
private:
    string owner;
    double balance;
    string accountId;

public:
    BankAccount(string ownerName, double initialBalance, string id) {
        // 驗證帳戶名
        if (ownerName.empty()) {
            cout << "  警告：帳戶名不能為空，使用預設值" << endl;
            owner = "未知";
        } else {
            owner = ownerName;
        }

        // 驗證初始餘額
        if (initialBalance < 0) {
            cout << "  警告：初始餘額不能為負數，設為 0" << endl;
            balance = 0.0;
        } else {
            balance = initialBalance;
        }

        // 驗證帳戶 ID
        if (id.length() != 10) {
            cout << "  警告：帳戶 ID 必須為 10 位，使用預設 ID" << endl;
            accountId = "0000000000";
        } else {
            accountId = id;
        }
    }

    void print() const {
        cout << "  帳戶: " << accountId
             << ", 戶主: " << owner
             << ", 餘額: $" << balance << endl;
    }
};

// ================================================================
// 重點六：常見錯誤與陷阱
// ================================================================
//
// 陷阱 1：不要給建構函數寫返回值
//   void Bad() { }   // 錯誤！這會變成一個普通成員函數
//
// 陷阱 2：Most Vexing Parse（最令人困擾的解析）
//   Student s();     // 錯誤！這是「函數宣告」，不是對象創建！
//   Student s;       // 正確：調用預設建構函數
//   Student s{};     // 正確（C++11）：統一初始化語法
//
// 陷阱 3：定義了帶參建構函數後，預設建構函數就消失了
//   class Point {
//   public:
//       Point(int x, int y) { ... }  // 定義了帶參建構
//   };
//   Point p;         // 編譯錯誤！沒有預設建構函數！
//   // 原因：只要定義了「任何一個」建構函數，編譯器就不再自動生成預設建構函數

// ================================================================
// 重點七：綜合範例 —— Car 類別
// ================================================================

class Car {
private:
    string brand;
    string model;
    int year;
    double mileage;

public:
    // 建構函數 1：完全預設
    Car() {
        brand = "未知";
        model = "未知";
        year = 2024;
        mileage = 0.0;
        cout << "  [建構] 創建預設汽車" << endl;
    }

    // 建構函數 2：只指定品牌和型號
    Car(string b, string m) {
        brand = b;
        model = m;
        year = 2024;
        mileage = 0.0;
        cout << "  [建構] 創建新車: " << brand << " " << model << endl;
    }

    // 建構函數 3：完整指定（帶驗證）
    Car(string b, string m, int y, double mi) {
        brand = b;
        model = m;

        // 年份驗證（1886 年是汽車發明的年份）
        if (y < 1886 || y > 2025) {
            cout << "    警告：年份不合法，使用 2024" << endl;
            year = 2024;
        } else {
            year = y;
        }

        // 里程驗證
        if (mi < 0) {
            cout << "    警告：里程不能為負，設為 0" << endl;
            mileage = 0.0;
        } else {
            mileage = mi;
        }

        cout << "  [建構] 創建二手車: " << brand << " " << model
             << " (" << year << ")" << endl;
    }

    void print() const {
        cout << "  " << year << " " << brand << " " << model
             << ", 里程: " << mileage << " km" << endl;
    }
};

// =============================================================================
// 【LeetCode 實戰範例】LeetCode 1603. Design Parking System
//   題目：設計一個停車場系統。建構子接收大／中／小三種車位的數量，
//         addCar(carType) 嘗試停入一輛車，成功回傳 true、車位滿了回傳 false。
//   為什麼用到本主題：這題本身就是一道「建構函式練習題」——
//         建構函式的職責正是「建立不變條件」（0 <= 已停 <= 容量），
//         而 addCar() 則負責在每次修改時維持它。
//         車位數必須是 private：若公開，任何人都能繞過檢查把它改成負數。
//         這正是本課「建構函式 = 物件合法性的第一道防線」的最小完整示範。
//   複雜度：建構 O(1)、addCar O(1)。
// =============================================================================
class ParkingSystem {
public:
    // 建構函式在此建立不變條件：容量確定且非負
    ParkingSystem(int big, int medium, int small) {
        // 防禦性驗證：LeetCode 保證輸入合法，但真實系統不該假設這件事
        m_slots[0] = (big    > 0) ? big    : 0;
        m_slots[1] = (medium > 0) ? medium : 0;
        m_slots[2] = (small  > 0) ? small  : 0;
    }

    // carType: 1=大, 2=中, 3=小
    bool addCar(int carType) {
        if (carType < 1 || carType > 3) return false;   // 非法車種直接拒絕
        int idx = carType - 1;
        if (m_slots[idx] <= 0) return false;            // 車位已滿，維持不變條件
        --m_slots[idx];
        return true;
    }

    int remaining(int carType) const {
        if (carType < 1 || carType > 3) return 0;
        return m_slots[carType - 1];
    }

private:
    int m_slots[3] = {0, 0, 0};   // private：外界無法繞過 addCar 直接改動
};

// =============================================================================
// 【日常實務範例】資料庫連線池設定（建構函式集中驗證 + 委派）
//   情境：服務啟動時從設定檔建立連線池。這是建構函式驗證的典型戰場 ——
//   設定值來自外部（YAML／環境變數），完全不可信任：
//     * poolSize 可能是 0 或負數 → 服務永遠拿不到連線，卡死且難以診斷
//     * timeout 可能過小 → 正常查詢全部逾時
//     * host 可能是空字串 → 連線時才爆炸，但那時已離事發點很遠
//   正確做法是在建構當下就擋掉，讓「服務啟動失敗並印出明確原因」，
//   而不是「服務起來了但行為詭異」。後者的除錯成本高出好幾個數量級。
//   同時示範委派建構函式：驗證只寫一份，所有建立路徑都會經過。
// =============================================================================
class DbPoolConfig {
public:
    // 完整版：唯一做驗證的地方（single source of truth）
    DbPoolConfig(const std::string& host, int port, int poolSize, int timeoutMs)
        : m_host(host), m_port(port), m_poolSize(poolSize), m_timeoutMs(timeoutMs) {
        if (m_host.empty())
            throw std::invalid_argument("DB host 不可為空");
        if (m_port <= 0 || m_port > 65535)
            throw std::invalid_argument("DB port 超出合法範圍: " + std::to_string(m_port));
        if (m_poolSize <= 0)
            throw std::invalid_argument("連線池大小必須為正數: " + std::to_string(m_poolSize));
        if (m_timeoutMs < 100)
            throw std::invalid_argument("逾時過短，至少 100ms: " + std::to_string(m_timeoutMs));
    }

    // 常用預設值版本：委派過去，不重複任何驗證邏輯
    DbPoolConfig(const std::string& host, int port)
        : DbPoolConfig(host, port, 10, 3000) {}

    void describe() const {
        cout << "    " << m_host << ":" << m_port
             << " (pool=" << m_poolSize << ", timeout=" << m_timeoutMs << "ms)" << endl;
    }

private:
    std::string m_host;      // 宣告順序 = 初始化順序
    int         m_port;
    int         m_poolSize;
    int         m_timeoutMs;
};

int main() {
    cout << "=============================================" << endl;
    cout << "   第 13 課：建構函數（Constructor）基礎" << endl;
    cout << "=============================================" << endl;

    // --- 重點一 & 二：基本建構函數 ---
    cout << "\n【1】基本建構函數 & 帶參建構函數" << endl;

    Student s1;                        // 調用預設建構函數
    s1.print();

    Student s2("張三", 20, 3.8f);      // 調用帶參建構函數
    s2.print();

    Student s3("李四", 22, 3.5f);
    s3.print();

    // --- 重點三：建構函數的調用時機 ---
    cout << "\n【2】建構函數的調用時機" << endl;
    cout << "(全域對象在 main 之前建構，此處示範局部、區塊、動態)" << endl;

    Box localBox;                       // 局部對象：執行到這一行時建構

    {
        Box blockBox;                   // 區塊內對象：進入區塊時建構
        cout << "  區塊內..." << endl;
    }  // blockBox 在這裡離開作用域

    Box* heapBox = new Box();           // 動態對象：new 的時候建構
    delete heapBox;                     // 記得釋放

    // --- 重點四：建構函數重載 ---
    cout << "\n【3】建構函數重載" << endl;

    Rectangle r1;                       // 建構函數 1：無參數
    r1.print();

    Rectangle r2(5.0);                  // 建構函數 2：正方形
    r2.print();

    Rectangle r3(4.0, 6.0);            // 建構函數 3：指定寬高
    r3.print();

    Rectangle r4(3.0, 7.0, "紅色");     // 建構函數 4：指定寬高和顏色
    r4.print();

    // --- 重點五：建構函數中的資料驗證 ---
    cout << "\n【4】建構函數中的資料驗證" << endl;

    cout << "正常創建：" << endl;
    BankAccount a1("王五", 10000.0, "1234567890");
    a1.print();

    cout << "非法數據：" << endl;
    BankAccount a2("", -500.0, "123");
    a2.print();

    // --- 重點七：綜合範例 Car ---
    cout << "\n【5】綜合範例：Car 類別" << endl;

    Car c1;
    c1.print();

    Car c2("Toyota", "Camry");
    c2.print();

    Car c3("BMW", "M3", 2020, 35000.5);
    c3.print();

    Car c4("時光機", "DeLorean", 1800, -100.0);  // 測試非法數據
    c4.print();

    // --- 陷阱提醒 ---
    cout << "\n【6】常見陷阱提醒" << endl;
    cout << "  陷阱1: void Student() { } → 這不是建構函數，是普通函數！" << endl;
    cout << "  陷阱2: Student s(); → 這是函數宣告（Most Vexing Parse）！" << endl;
    cout << "         正確寫法: Student s; 或 Student s{};" << endl;
    cout << "  陷阱3: 定義了帶參建構函數後，預設建構函數消失！" << endl;
    cout << "         需手動加回，或使用 = default（下一課）" << endl;

    // --- LeetCode 1603. Design Parking System ---
    cout << "\n=== LeetCode 1603. Design Parking System ===" << endl;
    ParkingSystem lot(1, 1, 0);      // 官方範例：大 1、中 1、小 0
    cout << "  建構後容量: 大=" << lot.remaining(1)
         << " 中=" << lot.remaining(2)
         << " 小=" << lot.remaining(3) << endl;
    cout << "  addCar(1) 大型車 -> " << (lot.addCar(1) ? "true" : "false")
         << "（預期 true）" << endl;
    cout << "  addCar(2) 中型車 -> " << (lot.addCar(2) ? "true" : "false")
         << "（預期 true）" << endl;
    cout << "  addCar(3) 小型車 -> " << (lot.addCar(3) ? "true" : "false")
         << "（預期 false，小車位為 0）" << endl;
    cout << "  addCar(1) 再一台 -> " << (lot.addCar(1) ? "true" : "false")
         << "（預期 false，大車位已滿）" << endl;
    cout << "  ↑ 建構函式建立不變條件，addCar 維持它；車位是 private，" << endl;
    cout << "    外界無法繞過檢查直接把剩餘數改成負數。" << endl;

    // --- 實務：資料庫連線池設定 ---
    cout << "\n=== 實務：連線池設定的建構期驗證 ===" << endl;
    try {
        DbPoolConfig ok1("db.internal", 5432, 20, 5000);
        cout << "  ✓ 完整設定建立成功:" << endl;
        ok1.describe();

        DbPoolConfig ok2("cache.internal", 6379);   // 委派版本，套用預設值
        cout << "  ✓ 簡化設定（委派給完整版）建立成功:" << endl;
        ok2.describe();
    } catch (const std::invalid_argument& e) {
        cout << "  不該失敗: " << e.what() << endl;
    }

    // 每一種非法設定都在建構當下就被擋下，而不是等到連線時才爆炸
    const char* badHosts[] = {"", "db.internal", "db.internal", "db.internal"};
    const int   badPorts[] = {5432, 99999, 5432, 5432};
    const int   badPools[] = {10, 10, 0, 10};
    const int   badTimeos[] = {3000, 3000, 3000, 50};
    for (int i = 0; i < 4; ++i) {
        try {
            DbPoolConfig bad(badHosts[i], badPorts[i], badPools[i], badTimeos[i]);
            cout << "  不該成功: ";
            bad.describe();
        } catch (const std::invalid_argument& e) {
            cout << "  ✗ 啟動時即攔截: " << e.what() << endl;
        }
    }
    cout << "  ↑ 服務「啟動失敗並說明原因」，遠優於「起來了但行為詭異」" << endl;

    // --- 重點回顧 ---
    cout << "\n=============================================" << endl;
    cout << "本課重點回顧：" << endl;
    cout << "  1. 建構函數 = 對象創建時自動調用的特殊成員函數" << endl;
    cout << "  2. 命名規則：函數名=類別名，沒有返回值" << endl;
    cout << "  3. 四種調用時機：全域/局部/區塊/動態(new)" << endl;
    cout << "  4. 可以重載：多個建構函數，參數不同" << endl;
    cout << "  5. 資料驗證：建構函數是確保對象合法性的第一道防線" << endl;
    cout << "  6. Most Vexing Parse: Student s() 是函數宣告！" << endl;
    cout << "  7. 定義任何建構函數後，預設建構函數不再自動生成" << endl;
    cout << "=============================================" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
// =============================================
//    第 13 課：建構函數（Constructor）基礎
// =============================================
//
// 【1】基本建構函數 & 帶參建構函數
// >>> 預設建構函數被調用 <<<
//   姓名: 未命名, 年齡: 0, GPA: 0
// >>> 帶參建構函數被調用: 張三 <<<
//   姓名: 張三, 年齡: 20, GPA: 3.8
// >>> 帶參建構函數被調用: 李四 <<<
//   姓名: 李四, 年齡: 22, GPA: 3.5
//
// 【2】建構函數的調用時機
// (全域對象在 main 之前建構，此處示範局部、區塊、動態)
// 建構函數: 創建了 [空盒子]
// 建構函數: 創建了 [空盒子]
//   區塊內...
// 建構函數: 創建了 [空盒子]
//
// 【3】建構函數重載
//   白色 矩形: 1 x 1, 面積 = 1
//   白色 矩形: 5 x 5, 面積 = 25
//   白色 矩形: 4 x 6, 面積 = 24
//   紅色 矩形: 3 x 7, 面積 = 21
//
// 【4】建構函數中的資料驗證
// 正常創建：
//   帳戶: 1234567890, 戶主: 王五, 餘額: $10000
// 非法數據：
//   警告：帳戶名不能為空，使用預設值
//   警告：初始餘額不能為負數，設為 0
//   警告：帳戶 ID 必須為 10 位，使用預設 ID
//   帳戶: 0000000000, 戶主: 未知, 餘額: $0
//
// 【5】綜合範例：Car 類別
//   [建構] 創建預設汽車
//   2024 未知 未知, 里程: 0 km
//   [建構] 創建新車: Toyota Camry
//   2024 Toyota Camry, 里程: 0 km
//   [建構] 創建二手車: BMW M3 (2020)
//   2020 BMW M3, 里程: 35000.5 km
//     警告：年份不合法，使用 2024
//     警告：里程不能為負，設為 0
//   [建構] 創建二手車: 時光機 DeLorean (2024)
//   2024 時光機 DeLorean, 里程: 0 km
//
// 【6】常見陷阱提醒
//   陷阱1: void Student() { } → 這不是建構函數，是普通函數！
//   陷阱2: Student s(); → 這是函數宣告（Most Vexing Parse）！
//          正確寫法: Student s; 或 Student s{};
//   陷阱3: 定義了帶參建構函數後，預設建構函數消失！
//          需手動加回，或使用 = default（下一課）
//
// === LeetCode 1603. Design Parking System ===
//   建構後容量: 大=1 中=1 小=0
//   addCar(1) 大型車 -> true（預期 true）
//   addCar(2) 中型車 -> true（預期 true）
//   addCar(3) 小型車 -> false（預期 false，小車位為 0）
//   addCar(1) 再一台 -> false（預期 false，大車位已滿）
//   ↑ 建構函式建立不變條件，addCar 維持它；車位是 private，
//     外界無法繞過檢查直接把剩餘數改成負數。
//
// === 實務：連線池設定的建構期驗證 ===
//   ✓ 完整設定建立成功:
//     db.internal:5432 (pool=20, timeout=5000ms)
//   ✓ 簡化設定（委派給完整版）建立成功:
//     cache.internal:6379 (pool=10, timeout=3000ms)
//   ✗ 啟動時即攔截: DB host 不可為空
//   ✗ 啟動時即攔截: DB port 超出合法範圍: 99999
//   ✗ 啟動時即攔截: 連線池大小必須為正數: 0
//   ✗ 啟動時即攔截: 逾時過短，至少 100ms: 50
//   ↑ 服務「啟動失敗並說明原因」，遠優於「起來了但行為詭異」
//
// =============================================
// 本課重點回顧：
//   1. 建構函數 = 對象創建時自動調用的特殊成員函數
//   2. 命名規則：函數名=類別名，沒有返回值
//   3. 四種調用時機：全域/局部/區塊/動態(new)
//   4. 可以重載：多個建構函數，參數不同
//   5. 資料驗證：建構函數是確保對象合法性的第一道防線
//   6. Most Vexing Parse: Student s() 是函數宣告！
//   7. 定義任何建構函數後，預設建構函數不再自動生成
// =============================================
