// =============================================================================
//  第 16 課：建構函數初始化列表（Member Initializer List）—— 總複習
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  Class(參數列) : 成員1(初值1), 成員2(初值2), 基底(引數) { 函數體 }
//                          └────────── 成員初始化列表 ──────────┘
//   位置：  建構函數參數列的右括號之後、函數體的左大括號之前
//   標準版本：C++98 起即有；NSDMI（類別內預設成員初始化）為 C++11、
//             委派建構函數（delegating constructor）亦為 C++11
//   複雜度：O(成員數)；相對「函數體賦值」少一次預設建構
//   標頭檔：本檔用到 <string>、<cmath>、<vector>、<list>、<unordered_map>、<algorithm>
//
// 【詳細解釋 Explanation】
//
// 【1. 一句話說清楚它是什麼】
//   初始化列表是「指定每個成員該**如何誕生**」的地方。
//   建構函數的函數體則是「物件已經誕生之後，還要多做什麼」的地方。
//   兩者的分界線非常明確：函數體的第一行執行時，所有成員都必須已經初始化完成。
//
// 【2. 初始化 vs 賦值：這是整課的地基】
//   ● 函數體內賦值（兩步）
//       (a) 進入函數體前，成員被**預設建構**
//       (b) 函數體內用 operator= 覆蓋它
//   ● 初始化列表（一步）
//       (a) 直接用你給的初值建構成員
//   用 C 的概念類比：
//       int x = 10;          ← 初始化 → 對應初始化列表
//       int y;  y = 10;      ← 賦值   → 對應函數體
//
// 【3. 四種「非用不可」的場景（本課核心）】
//   這四種成員在函數體內根本無法初始化，只有初始化列表一條路：
//     (1) const 成員        —— 誕生後不可改，只能在誕生時給值
//     (2) 參考成員 T&       —— 必須誕生時綁定，且永不可重新綁定
//     (3) 無預設建構函數的成員物件 —— 進函數體前沒有東西可以先建
//     (4) 基底類別的建構函數 —— 基底子物件必須先於衍生成員完成初始化
//   請注意：初始化列表首先是「表達初始化的唯一語法」，效能只是附帶好處。
//
// 【4. 初始化順序：由宣告順序決定，不是書寫順序】
//   非靜態成員一律依照在 class 內的**宣告順序**初始化。
//   書寫順序若與宣告順序不一致，g++ 會發出 -Wreorder 警告，
//   但編譯器**不會**調整順序——實際順序永遠是宣告順序。
//   為什麼要這樣設計？因為解構順序必須是建構順序的反序且唯一；
//   若順序由各建構函數自行決定，同一個類別的多個建構函數會產生不同順序，
//   唯一的解構函數就無從決定該用哪種反序。
//
// 【5. 順序依賴：什麼時候會變成真的 bug】
//   當某個成員的初值運算式讀取**另一個成員**時，順序才會影響正確性。
//   最惡名昭彰的樣板是：
//       char* m_data;      // 宣告在前 → 先初始化
//       size_t m_len;      // 宣告在後 → 後初始化
//       String(const char* s) : m_len(strlen(s)), m_data(new char[m_len+1])
//   m_data 先初始化，卻讀取尚未初始化的 m_len —— 未定義行為。
//   三層修法：
//     (a) 治本：讓被依賴的成員宣告在前
//     (b) 輔助：書寫順序 = 宣告順序，讓程式碼不誤導人
//     (c) 防呆：初值只依賴**建構函數參數**，徹底不碰其他成員
//   最根本的建議：能用 std::vector／std::string 就別自己 new[]。
//
// 【6. 初始化列表可以放什麼】
//   任何能產生該成員型別的**單一運算式**：算術、函數呼叫、字串串接、三元運算。
//   不能放敘述（沒有 if／for）。需要流程控制時，包成一個
//   （通常是 static private 的）輔助函數，再從初始化列表呼叫。
//
// 【7. NSDMI（C++11）與初始化列表的優先順序】
//   ● 成員有寫在初始化列表 → 用初始化列表的值，NSDMI 被忽略
//   ● 成員沒寫在初始化列表 → 用 NSDMI 的值
//   ● 兩者都沒有 → 類別型別呼叫預設建構函數；
//                  **內建型別不初始化，值不定，讀取是未定義行為**
//   重要：NSDMI 不是「先設預設值再覆蓋」，只會初始化一次。
//   建議替所有內建型別成員都寫上 NSDMI，消除整類不定值 bug。
//
// 【概念補充 Concept Deep Dive】
//
//   ● 完整的初始化順序（含繼承）
//     (1) 虛擬基底類別（依繼承圖深度優先、由左至右）
//     (2) 直接基底類別（依**繼承列表**的書寫順序）
//     (3) 非靜態成員（依**宣告順序**）
//     (4) 建構函數的函數體
//     解構則完全相反。注意 (2) 看繼承列表、(3) 看宣告順序，都不看初始化列表。
//
//   ● const 成員與參考成員的共同副作用
//     兩者都會讓編譯器隱式生成的**複製指派運算子被定義為 deleted**，
//     因為 const 不能被賦值、參考不能重新綁定。複製**建構**仍然可以。
//     連帶影響：這種類別放進 std::vector 後，任何需要賦值的操作
//     （sort、erase 造成的元素前移、resize）都會編譯失敗。
//
//   ● 建構／解構期間，虛擬函數不會下派
//     在基底建構函數執行期間，物件的動態型別就是「當前正在建構的那一層」，
//     虛擬呼叫會停在本層。因為衍生類別的成員還沒初始化，
//     下派過去就會操作未初始化的資料——這是語言刻意的保護。
//
//   ● 例外安全
//     若某個成員的初值運算式拋出例外，**已初始化完成的成員會被正常解構**，
//     但物件視為建構失敗，它自己的解構函數**不會**被呼叫。
//     因此每個資源都應各自由一個 RAII 成員持有，
//     而不是在一個建構函數裡連續 new 好幾次。
//
//   ● 委派建構函數（C++11）
//     多個建構函數的共用邏輯可以集中：
//         Weapon(int id) : Weapon(id, "無名", "普通", 1, 10.0) { }
//     注意一旦委派給另一個建構函數，該列表就不能再初始化其他成員。
//
//   ● 為什麼編譯器不會把「函數體賦值」自動最佳化成初始化列表
//     因為預設建構函數與 operator= 可能有可觀察的副作用（記憶體配置、I/O、計數）。
//     最佳化必須保持可觀察行為，所以那次多餘的預設建構是刪不掉的。
//     本課 10.cpp 以計數器實測：-O0 與 -O2 的呼叫次數完全相同。
//
// 【注意事項 Pay Attention】
//   1. 初始化順序由宣告順序決定；**移動一行成員宣告就可能默默改變行為**，
//      這是重構時的隱形風險。
//   2. -Wreorder 只比對「書寫 vs 宣告」順序，**不分析依賴關係**。
//      若宣告順序本身就錯而列表又照著寫，完全不會有警告，但 bug 依然存在。
//   3. 初值優先使用建構函數**參數**，而非其他成員。
//   4. 內建型別成員若既無 NSDMI 也未寫入初始化列表，其值為不定值，
//      讀取是未定義行為——不可描述成「垃圾值」這種固定結果。
//   5. M_PI 不是 C++ 標準的一部分（來自 POSIX／傳統 C 函式庫，
//      MSVC 需 _USE_MATH_DEFINES）。本檔以 #ifndef 保護後使用；
//      可攜寫法是自訂 constexpr，C++20 起則有 <numbers> 的 std::numbers::pi。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】建構函數初始化列表
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 初始化列表和在建構函數本體內賦值，差別是什麼？
//     答：本體內是「先預設建構成員、再用 operator= 賦值」兩步；
//         初始化列表是直接以初值建構成員，一步。對有建構成本的型別
//         （string、vector）方式 A 多做一次白工，甚至多一次配置與釋放。
//         但更關鍵的是功能差異：const 成員、參考成員、無預設建構函數的成員、
//         以及基底類別，**只能**在初始化列表初始化。
//     追問：怎麼證明成員在進函數體前就已經被建構了？
//         → 在函數體第一行印出該成員。若它已是合法的空字串，
//           代表預設建構早已完成（本課 1.cpp 即如此示範）。
//
// 🔥 Q2. 成員的初始化順序由什麼決定？為什麼要這樣設計？
//     答：由成員在 class 內的**宣告順序**決定，與初始化列表的書寫順序無關。
//         這樣設計是因為解構順序必須是建構順序的完全反序、且唯一可決定；
//         若順序隨各建構函數的書寫而變，唯一的解構函數就無法決定拆除順序。
//     追問：多重繼承時多個基底的順序呢？
//         → 由繼承列表的書寫順序決定，同樣不看初始化列表。
//
// 🔥 Q3. 下面這段程式碼有什麼問題？
//        class String {
//            char* m_data;
//            size_t m_len;
//            String(const char* s) : m_len(strlen(s)), m_data(new char[m_len+1]) {}
//        };
//     答：m_data 宣告在 m_len 之前，所以 m_data **先**初始化，
//         而它的初值 new char[m_len+1] 讀取了尚未初始化的 m_len —— 未定義行為。
//         配置大小不可預期，後續複製字串很可能造成 heap 溢位。
//     追問：怎麼修？
//         → 把 m_len 宣告在 m_data 前面（治本），或初值直接用參數
//           new char[strlen(s)+1]（防呆），最好是改用 std::string／std::vector。
//
// ⚠️ 陷阱 1. 類別裡加一個 const 成員只是「讓它不能改」，不影響別的，對嗎？
//     答：不對。const 成員會讓隱式的複製指派運算子被定義為 deleted，
//         連帶使移動指派也不可用。這種類別放進 std::vector 後，
//         sort、erase 造成的元素搬移、resize 等需要賦值的操作全部編譯失敗。
//     為什麼會錯：把 const 想成只作用在「那個欄位」。實際上它會改變
//         整個類別自動生成的特殊成員函數，影響範圍遠大於單一欄位。
//
// ⚠️ 陷阱 2. 開了 -Wall（含 -Wreorder），順序相關的 bug 就都擋得住了吧？
//     答：擋不住。-Wreorder 只比對書寫順序與宣告順序是否一致，
//         **不分析成員之間的依賴**。若宣告順序本身就錯，而初始化列表又
//         乖乖照著宣告順序寫，就完全不會有警告，bug 卻真實存在。
//     為什麼會錯：把編譯器警告當成完備的正確性檢查。警告是啟發式的輔助，
//         真正的防線是「被依賴者宣告在前」與「初值只用參數」。
//
// ⚠️ 陷阱 3. 初始化列表寫 : c(x), a(y), b(z)，所以 c 會先算好再輪到 a、b？
//     答：不對。實際順序永遠是宣告順序。若宣告是 a、b、c，
//         那 c 是最後才初始化的，不管你寫在第幾個位置。
//     為什麼會錯：把初始化列表當成「由上往下執行的敘述序列」。
//         它其實是宣告式的——你只是在指定每個成員該用什麼初始化。
// ═══════════════════════════════════════════════════════════════════════════

/*
 * ================================================================
 * 【第 16 課：建構函數初始化列表（Member Initializer List）】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -Wall -Wextra -o summary summary.cpp
 *
 * 本課重點：
 * 1. 初始化列表 vs 函數體賦值的本質區別（初始化=一步 vs 賦值=兩步）
 * 2. 必須使用初始化列表的四種情況：
 *    - const 成員變數（不能賦值只能初始化）
 *    - 引用成員變數（必須在宣告時綁定）
 *    - 沒有預設建構函數的成員物件
 *    - 基類的建構函數（繼承時）
 * 3. 初始化順序陷阱：按成員宣告順序，不是列表書寫順序
 * 4. 危險的順序依賴（DangerousOrder vs SafeOrder）
 * 5. 初始化列表中可以使用表達式（Circle 面積計算、FullName 字串串接）
 * 6. C++11 類別內預設值配合初始化列表（GameConfig 覆蓋規則）
 * 7. 效能對比：函數體賦值=預設建構+賦值（兩步），初始化列表=直接建構（一步）
 * 8. 綜合範例：RPG 武器系統（Rarity + Weapon）
 * ================================================================
 */

#include <iostream>
#include <string>
#include <cmath>
#include <vector>
#include <list>
#include <unordered_map>
#include <algorithm>
using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ================================================================
// 重點一：初始化列表 vs 函數體賦值的本質區別
// ================================================================
// 函數體內賦值 = 先預設建構（第一步），再賦值（第二步）→ 兩步
// 初始化列表   = 直接用參數建構（一步到位）→ 一步
//
// 用 C 語言的概念來理解：
//   int x = 10;    ← 初始化（宣告的同時給值）→ 初始化列表
//   int y;
//   y = 10;        ← 賦值（先宣告再給值）    → 函數體賦值
//
// ┌───────────────────────────────────────────────────────────┐
// │ 函數體內賦值（兩步）：                                      │
// │   成員預設建構 → name = ""    → name = "Hello" （賦值）    │
// │                                                           │
// │ 初始化列表（一步）：                                        │
// │   直接用參數建構 → name("Hello")  （一步到位）              │
// └───────────────────────────────────────────────────────────┘

// --- 方式 A：函數體內賦值（兩步）---
class DemoAssign {
private:
    string name;
    int value;

public:
    DemoAssign(const string& n, int v) {
        // 進入函數體之前，name 已經被預設建構為空字串（第一步）
        cout << "  [賦值方式] 賦值前 name = [" << name << "]（已被預設建構）" << endl;
        name = n;     // 賦值（第二步）
        value = v;
    }

    void print() const {
        cout << "  name = " << name << ", value = " << value << endl;
    }
};

// --- 方式 B：初始化列表（一步）---
class DemoInitList {
private:
    string name;
    int value;

public:
    DemoInitList(const string& n, int v)
        : name(n), value(v)    // 直接用 n 來建構 name，一步到位
    {
        cout << "  [初始化列表] name 直接就是 [" << name << "]（一步完成）" << endl;
    }

    void print() const {
        cout << "  name = " << name << ", value = " << value << endl;
    }
};

// ================================================================
// 重點二：必須使用初始化列表的四種情況
// ================================================================
// ┌──────────────────────────────────┬──────────────────────────────┐
// │ 情況                              │ 原因                          │
// ├──────────────────────────────────┼──────────────────────────────┤
// │ const 成員變數                    │ const 只能初始化，不能賦值    │
// │ 引用成員變數                      │ 引用必須在宣告時綁定          │
// │ 沒有預設建構函數的成員物件         │ 無法先預設建構再賦值          │
// │ 基類的建構函數（繼承時）           │ 基類必須在派生類之前建構      │
// └──────────────────────────────────┴──────────────────────────────┘

// --- 情況 1：const 成員變數 ---
// const 變數必須在宣告時就初始化，之後不能修改。
// 函數體執行時，成員已經「宣告」過了，再賦值就是修改 const → 編譯錯誤。
class Student {
private:
    const int studentId;  // const 成員：一旦初始化就不能修改
    string name;

public:
    // 錯誤寫法：
    // Student(int id, const string& n) {
    //     studentId = id;  // 編譯錯誤！不能給 const 賦值！
    //     name = n;
    // }

    // 正確寫法：使用初始化列表
    Student(int id, const string& n)
        : studentId(id), name(n)
    { }

    void print() const {
        cout << "  學號: " << studentId << ", 姓名: " << name << endl;
    }
};

// --- 情況 2：引用成員變數 ---
// 引用一旦綁定就不能改變指向，且必須在宣告時綁定。
class Logger {
private:
    ostream& output;  // 引用成員：必須在初始化時綁定
    string prefix;

public:
    // 錯誤寫法：
    // Logger(ostream& os, const string& p) {
    //     output = os;  // 編譯錯誤！引用必須在初始化列表中綁定！
    //     prefix = p;
    // }

    // 正確寫法
    Logger(ostream& os, const string& p)
        : output(os), prefix(p)
    { }

    void log(const string& message) const {
        output << "  [" << prefix << "] " << message << endl;
    }
};

// --- 情況 3：沒有預設建構函數的成員物件 ---
// 如果成員物件的類別只有帶參建構函數，沒有預設建構函數，
// 那就無法在函數體之前自動預設建構它，必須在初始化列表中提供參數。
class Engine {
private:
    int horsepower;
    string fuelType;

public:
    // Engine 只有帶參建構函數，沒有預設建構函數
    Engine(int hp, const string& fuel)
        : horsepower(hp), fuelType(fuel)
    {
        cout << "  引擎建構: " << hp << " 馬力, " << fuel << endl;
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
    // Car(const string& b, int hp, const string& fuel) {
    //     brand = b;
    //     engine = Engine(hp, fuel);  // 編譯錯誤！engine 無法預設建構！
    // }

    // 正確寫法：在初始化列表中建構 engine
    Car(const string& b, int hp, const string& fuel)
        : brand(b), engine(hp, fuel)  // 直接把參數傳給 Engine 的建構函數
    {
        cout << "  汽車建構: " << brand << endl;
    }

    void print() const {
        cout << "  品牌: " << brand << endl;
        engine.print();
    }
};

// --- 情況 4：基類的建構函數（繼承時）---
// 派生類必須在初始化列表中調用基類的建構函數。
class Animal {
private:
    string species;

public:
    Animal(const string& s) : species(s) {
        cout << "  Animal 建構: " << species << endl;
    }

    string getSpecies() const { return species; }
};

class Dog : public Animal {
private:
    string name;

public:
    // 用初始化列表調用基類建構函數
    Dog(const string& n)
        : Animal("犬科"),   // 調用基類建構函數（必須用初始化列表）
          name(n)            // 初始化自己的成員
    {
        cout << "  Dog 建構: " << name << endl;
    }

    void print() const {
        cout << "  " << name << " (" << getSpecies() << ")" << endl;
    }
};

// ================================================================
// 重點三：初始化順序陷阱
// ================================================================
// 初始化列表中成員的初始化順序不是由列表書寫順序決定的，
// 而是由成員在類別中宣告的順序決定的！
//
// 最佳實踐：初始化列表的書寫順序，永遠保持和成員宣告順序一致。
// 編譯時加 -Wall -Wextra 可以偵測順序不一致的問題。

class OrderDemo {
private:
    int a;    // 第 1 個宣告 → 第 1 個初始化
    int b;    // 第 2 個宣告 → 第 2 個初始化
    int c;    // 第 3 個宣告 → 第 3 個初始化

public:
    // 注意：初始化列表故意寫成 c, a, b 的順序
    // 但實際初始化順序仍然是 a, b, c（按宣告順序）
    OrderDemo(int val)
        : c(val + 2),     // 寫在第 1 個，但第 3 個執行
          a(val),          // 寫在第 2 個，但第 1 個執行
          b(val + 1)       // 寫在第 3 個，但第 2 個執行
    {
        cout << "  a = " << a << ", b = " << b << ", c = " << c << endl;
    }
};

// ================================================================
// 重點四：危險的順序依賴（DangerousOrder vs SafeOrder）
// ================================================================
// 當一個成員的初始化值依賴另一個成員時，宣告順序就至關重要。
// 如果順序不對，就會用到未初始化的垃圾值，造成未定義行為！

class DangerousOrder {
private:
    int length;    // 第 1 個宣告 → 先初始化
    int* data;     // 第 2 個宣告 → 後初始化

public:
    // 危險！看起來 data 用 length 分配，但 length 確實先初始化，
    // 然而如果寫反了（data 寫在 length 前面宣告），就會用垃圾值！
    // 這裡的問題是：書寫順序讓人誤以為 data 先於 length 初始化
    DangerousOrder(int len)
        : data(new int[length]),  // 用 length 來分配，但如果 length 還沒初始化就是垃圾值！
          length(len)              // 實際上 length 先宣告所以先初始化（此例碰巧安全）
    {
        // 注意：因為 length 宣告在 data 之前，所以 length 先被初始化為 len，
        // 然後 data 才用 length（此時已經是 len）分配。
        // 但這段程式碼很容易誤導人，因為書寫順序和宣告順序相反！
    }

    ~DangerousOrder() { delete[] data; }
};

class SafeOrder {
private:
    int length;    // 第 1 個宣告
    int* data;     // 第 2 個宣告

public:
    // 安全：初始化列表的書寫順序和宣告順序一致
    SafeOrder(int len)
        : length(len),            // 先初始化 length（和宣告順序一致）
          data(new int[length])   // 再用已初始化的 length 分配記憶體
    {
        cout << "  安全分配了 " << length << " 個元素" << endl;
    }

    ~SafeOrder() { delete[] data; }
};

// ================================================================
// 重點五：初始化列表中可以使用表達式
// ================================================================
// 初始化列表不僅可以直接傳參數，還可以使用任意表達式：
// 算術運算、函數調用、字串串接等都可以。

class Circle {
private:
    double radius;
    double area;
    double circumference;

public:
    // 在初始化列表中使用公式計算
    Circle(double r)
        : radius(r),
          area(M_PI * r * r),              // 面積公式
          circumference(2.0 * M_PI * r)    // 周長公式
    { }

    void print() const {
        cout << "  半徑: " << radius << endl;
        cout << "  面積: " << area << endl;
        cout << "  周長: " << circumference << endl;
    }
};

class FullName {
private:
    string firstName;
    string lastName;
    string fullName;

public:
    // 初始化列表中做字串串接
    FullName(const string& first, const string& last)
        : firstName(first),
          lastName(last),
          fullName(last + " " + first)    // 表達式：字串串接
    { }

    void print() const {
        cout << "  姓: " << lastName << ", 名: " << firstName
             << ", 全名: " << fullName << endl;
    }
};

// ================================================================
// 重點六：C++11 類別內預設值配合初始化列表
// ================================================================
// C++11 引入了類別內成員初始化（In-class Member Initializer），
// 可以在宣告成員時直接給預設值。
//
// 優先順序規則：
// ┌────────────────────────┬──────────────────────────────────┐
// │ 初始化列表中有指定      │ → 使用初始化列表的值（覆蓋預設值）│
// │ 初始化列表中沒有        │ → 使用類別內預設值                │
// │ 類別內也沒有預設值      │ → 基本型別不初始化（垃圾值），    │
// │                        │   類別型別調用預設建構函數         │
// └────────────────────────┴──────────────────────────────────┘

class GameConfig {
private:
    // C++11：直接在宣告時給預設值
    int screenWidth = 1280;
    int screenHeight = 720;
    bool fullscreen = false;
    string title = "My Game";
    int fps = 60;

public:
    // 預設建構函數：所有成員使用類別內的預設值
    GameConfig() {
        cout << "  [預設建構] 使用所有預設值" << endl;
    }

    // 部分自定義：初始化列表會覆蓋類別內預設值
    GameConfig(int w, int h)
        : screenWidth(w), screenHeight(h)  // 只覆蓋寬高
        // fullscreen、title、fps 使用類別內的預設值
    {
        cout << "  [部分自定義] 只改解析度" << endl;
    }

    // 完全自定義：初始化列表覆蓋所有預設值
    GameConfig(int w, int h, bool fs, const string& t, int f)
        : screenWidth(w), screenHeight(h), fullscreen(fs),
          title(t), fps(f)
    {
        cout << "  [完全自定義] 所有參數指定" << endl;
    }

    void print() const {
        cout << "  遊戲: " << title << endl;
        cout << "  解析度: " << screenWidth << "x" << screenHeight << endl;
        cout << "  全螢幕: " << (fullscreen ? "是" : "否") << endl;
        cout << "  FPS: " << fps << endl;
    }
};

// ================================================================
// 重點七：效能對比（函數體賦值 vs 初始化列表）
// ================================================================
// ┌──────────────┬──────────────┬─────────────────────────────┐
// │ 方式          │ 操作次數      │ 調用的函數                   │
// ├──────────────┼──────────────┼─────────────────────────────┤
// │ 函數體賦值    │ 2 次         │ 預設建構函數 + 賦值運算子     │
// │ 初始化列表    │ 1 次         │ 帶參建構函數                  │
// └──────────────┴──────────────┴─────────────────────────────┘
// 當成員是大型物件（如包含大量資料的容器），效能差異非常明顯。

class HeavyObject {
private:
    string data;

public:
    HeavyObject() {
        data = "";
        cout << "    HeavyObject 預設建構" << endl;
    }

    HeavyObject(const string& s) {
        data = s;
        cout << "    HeavyObject 帶參建構: " << s << endl;
    }

    HeavyObject& operator=(const string& s) {
        data = s;
        cout << "    HeavyObject 賦值運算子: " << s << endl;
        return *this;
    }
};

// 使用函數體賦值（兩步：預設建構 + 賦值）
class ContainerA {
private:
    HeavyObject obj;

public:
    ContainerA(const string& s) {
        // 進入函數體之前，obj 已經被預設建構了（第一步）
        cout << "    --- 開始賦值 ---" << endl;
        obj = s;    // 賦值（第二步）
        cout << "    --- 賦值完成 ---" << endl;
    }
};

// 使用初始化列表（一步：直接帶參建構）
class ContainerB {
private:
    HeavyObject obj;

public:
    ContainerB(const string& s)
        : obj(s)  // 直接帶參建構，不經過預設建構
    {
        cout << "    --- 初始化列表完成 ---" << endl;
    }
};

// ================================================================
// 重點八：綜合範例 —— RPG 武器系統
// ================================================================
// 展示初始化列表的所有技巧：
//   - const 成員（weaponId）
//   - 沒有預設建構函數的成員物件（Rarity）
//   - C++11 類別內預設值（enhanceLevel、isEquipped）
//   - 帶預設參數的建構函數
//   - 一般成員的初始化

// 武器稀有度（沒有預設建構函數 → 必須用初始化列表）
class Rarity {
private:
    string name;
    int stars;

public:
    Rarity(const string& n, int s) : name(n), stars(s) { }

    string getName() const { return name; }
    int getStars() const { return stars; }

    void print() const {
        cout << name << " (";
        for (int i = 0; i < stars; i++) cout << "*";
        cout << ")";
    }
};

// 武器類別
class Weapon {
private:
    const int weaponId;           // const 成員 → 必須用初始化列表
    string name;
    Rarity rarity;                // 無預設建構函數 → 必須用初始化列表
    double baseDamage;
    double critRate;

    // C++11 類別內預設值
    int enhanceLevel = 0;         // 強化等級，預設 0
    bool isEquipped = false;      // 是否裝備，預設否

public:
    // 建構函數：展示各種初始化方式的結合
    Weapon(int id, const string& n, const string& rarityName,
           int rarityStars, double dmg, double crit = 0.05)
        : weaponId(id),                              // const 成員
          name(n),                                    // 一般成員
          rarity(rarityName, rarityStars),            // 無預設建構的成員物件
          baseDamage(dmg),                            // 一般成員
          critRate(crit)                              // 帶預設參數
          // enhanceLevel 和 isEquipped 使用類別內預設值（不寫在列表中）
    {
        cout << "  鍛造武器: " << name << " [ID:" << weaponId << "]" << endl;
    }

    void enhance() {
        if (enhanceLevel < 15) {
            enhanceLevel++;
            baseDamage *= 1.1;  // 每次強化增加 10% 傷害
            cout << "  " << name << " 強化至 +" << enhanceLevel
                 << "（傷害: " << baseDamage << "）" << endl;
        } else {
            cout << "  " << name << " 已達最高強化等級！" << endl;
        }
    }

    void equip() { isEquipped = true; }
    void unequip() { isEquipped = false; }

    void print() const {
        cout << "  +---------------------------------+" << endl;
        cout << "  | [" << weaponId << "] " << name << endl;
        cout << "  | 稀有度: ";
        rarity.print();
        cout << endl;
        cout << "  | 傷害: " << baseDamage
             << "  暴擊率: " << (critRate * 100) << "%" << endl;
        cout << "  | 強化: +" << enhanceLevel
             << "  " << (isEquipped ? "[已裝備]" : "[未裝備]") << endl;
        cout << "  +---------------------------------+" << endl;
    }
};

// ================================================================
// 【LeetCode 實戰範例 1】LeetCode 146. LRU Cache
// ================================================================
// 題目：設計並實作一個 LRU（最近最少使用）快取。建構時給定 capacity；
//       get(key) 回傳值（不存在回 -1），put(key, value) 寫入，
//       容量滿時淘汰「最久沒被使用」的項目。get 與 put 都要 O(1)。
//
// 為什麼用到本主題：
//   (1) capacity 是建構參數，而且建好之後就不該改變 → 正是 const 成員的
//       典型場景，也就必須用初始化列表初始化。
//   (2) 兩個容器成員（list 與 unordered_map）都在初始化列表一次建好，
//       避免「先預設建構再賦值」的兩步。
//   (3) 順帶示範 const 成員的代價：LRUCache 因此不可複製指派。
//
// 資料結構：unordered_map 提供 O(1) 查找，list 維護使用順序（front = 最近使用），
//           map 的值存 list 的 iterator，因此可 O(1) 把節點搬到最前面。
// 複雜度：get / put 皆為 O(1) 平均；空間 O(capacity)。
class LRUCache {
private:
    const size_t capacity_;                                   // const 成員 → 必用初始化列表
    list<pair<int, int>> items_;                              // front 最近使用，back 最久未用
    unordered_map<int, list<pair<int, int>>::iterator> index_; // key → 節點位置

public:
    // 三個成員全部在初始化列表建好，一步到位
    explicit LRUCache(int capacity)
        : capacity_(static_cast<size_t>(capacity > 0 ? capacity : 1))
        , items_()
        , index_()
    { }

    int get(int key) {
        auto it = index_.find(key);
        if (it == index_.end()) return -1;
        // 命中：把該節點移到最前面（splice 不會使 iterator 失效）
        items_.splice(items_.begin(), items_, it->second);
        return it->second->second;
    }

    void put(int key, int value) {
        auto it = index_.find(key);
        if (it != index_.end()) {
            it->second->second = value;                       // 更新值
            items_.splice(items_.begin(), items_, it->second); // 並標記為最近使用
            return;
        }
        if (index_.size() >= capacity_) {                     // 滿了 → 淘汰最久未用
            int oldKey = items_.back().first;
            items_.pop_back();
            index_.erase(oldKey);
        }
        items_.emplace_front(key, value);
        index_[key] = items_.begin();
    }
};

// ================================================================
// 【LeetCode 實戰範例 2】LeetCode 705. Design HashSet
// ================================================================
// 題目：不使用內建雜湊表，自行實作 MyHashSet，支援 add / remove / contains。
//
// 為什麼用到本主題：
//   建構函數必須把「桶陣列」一次配置成固定大小。
//   寫成 : buckets_(kBuckets) 是在初始化列表直接建構出 769 個空 list；
//   若改成在函數體內 buckets_.resize(kBuckets)，就變成
//   「先預設建構一個空 vector，再 resize」的兩步——正是本課要避免的模式。
//
// 設計：分離鏈結法（separate chaining）。桶數取質數 769 以減少雜湊碰撞。
// 複雜度：平均 O(1)；最壞 O(n/桶數)。
class MyHashSet {
private:
    static const size_t kBuckets = 769;   // 質數，減少碰撞
    vector<list<int>> buckets_;

public:
    // 在初始化列表一次建好 769 個桶
    MyHashSet() : buckets_(kBuckets) { }

    void add(int key) {
        if (!contains(key)) buckets_[bucketOf(key)].push_back(key);
    }

    void remove(int key) {
        buckets_[bucketOf(key)].remove(key);
    }

    bool contains(int key) const {
        const list<int>& b = buckets_[bucketOf(key)];
        return find(b.begin(), b.end(), key) != b.end();
    }

private:
    // 用 unsigned 取模，避免負數取模的結果依實作而異
    static size_t bucketOf(int key) {
        return static_cast<size_t>(static_cast<unsigned int>(key)) % kBuckets;
    }
};

// ================================================================
// 【日常實務範例】服務啟動設定：把本課四種必用場景一次用上
// ================================================================
// 情境：一個後端服務啟動時要建立 ServiceContext，它必須包含
//   - 服務執行個體 ID（建立後絕不可變 → const 成員）
//   - 日誌輸出目標（一定要有、且不會中途更換 → 參考成員）
//   - 資料庫連線（只有帶參建構函數 → 無預設建構函數的成員物件）
//   - 一組有團隊共識預設值的調校參數（→ NSDMI）
// 這正好把「必須用初始化列表的四種情況」裡的三種，加上 NSDMI，
// 全部收斂在一個真實會出現的類別裡。
class DbHandle {
private:
    string dsn_;
public:
    // 只有帶參建構函數 → 沒有預設建構函數
    explicit DbHandle(const string& dsn) : dsn_(dsn) { }
    const string& dsn() const { return dsn_; }
};

class ServiceContext {
private:
    const string instanceId_;   // (1) const 成員：建立後不可變
    ostream& log_;              // (2) 參考成員：一定有、不更換
    DbHandle db_;               // (3) 無預設建構函數的成員物件

    // (4) NSDMI：團隊共識的預設值，三個建構函數共用
    int  maxConnections_ = 32;
    int  requestTimeoutMs_ = 3000;
    bool tracingEnabled_ = false;

public:
    // 標準設定：三個必用成員走初始化列表，其餘沿用 NSDMI
    ServiceContext(const string& instanceId, ostream& log, const string& dsn)
        : instanceId_(instanceId), log_(log), db_(dsn)
    { }

    // 高負載設定：額外覆蓋兩個 NSDMI 欄位
    ServiceContext(const string& instanceId, ostream& log, const string& dsn,
                   int maxConnections, bool tracing)
        : instanceId_(instanceId), log_(log), db_(dsn)
        , maxConnections_(maxConnections)
        , tracingEnabled_(tracing)
        // requestTimeoutMs_ 未指定 → 沿用 NSDMI 的 3000
    { }

    void report() const {
        log_ << "  [" << instanceId_ << "] db=" << db_.dsn()
             << ", maxConn=" << maxConnections_
             << ", timeout=" << requestTimeoutMs_ << "ms"
             << ", tracing=" << (tracingEnabled_ ? "on" : "off") << endl;
    }
};

// ================================================================
// main()：示範所有重點
// ================================================================
int main() {
    cout << "=====================================================" << endl;
    cout << "  第 16 課：建構函數初始化列表（Member Initializer List）" << endl;
    cout << "=====================================================" << endl;

    // --- 重點一：初始化列表 vs 函數體賦值 ---
    cout << "\n【1】初始化列表 vs 函數體賦值的本質區別" << endl;
    cout << "--- 函數體賦值（兩步：先預設建構，再賦值）---" << endl;
    DemoAssign d1("Hello", 42);
    d1.print();

    cout << "--- 初始化列表（一步：直接建構）---" << endl;
    DemoInitList d2("Hello", 42);
    d2.print();

    // --- 重點二：必須使用初始化列表的四種情況 ---
    cout << "\n【2】必須使用初始化列表的四種情況" << endl;

    cout << "\n情況 1：const 成員變數" << endl;
    Student s(20250001, "張三");
    s.print();
    cout << "  // const int studentId; → 只能初始化，不能賦值" << endl;

    cout << "\n情況 2：引用成員變數" << endl;
    Logger consoleLogger(cout, "INFO");
    consoleLogger.log("程式啟動");
    consoleLogger.log("初始化完成");
    cout << "  // ostream& output; → 引用必須在宣告時綁定" << endl;

    cout << "\n情況 3：沒有預設建構函數的成員物件" << endl;
    Car car("BMW", 300, "汽油");
    car.print();
    cout << "  // Engine engine; → Engine 只有帶參建構，無法預設建構" << endl;

    cout << "\n情況 4：基類的建構函數（繼承時）" << endl;
    Dog dog("旺財");
    dog.print();
    cout << "  // : Animal(\"犬科\") → 必須在初始化列表中調用基類建構" << endl;

    // --- 重點三：初始化順序陷阱 ---
    cout << "\n【3】初始化順序陷阱：按宣告順序，不是書寫順序" << endl;
    cout << "  列表書寫順序: c, a, b" << endl;
    cout << "  實際初始化順序: a, b, c（按宣告順序）" << endl;
    OrderDemo od(10);
    cout << "  結果正確（此例沒有依賴問題），但書寫順序容易誤導！" << endl;

    // --- 重點四：危險的順序依賴 ---
    cout << "\n【4】危險的順序依賴（DangerousOrder vs SafeOrder）" << endl;
    cout << "  DangerousOrder: data(new int[length]), length(len)" << endl;
    cout << "    → 書寫順序暗示 data 先用 length 分配，但 length 宣告在前，" << endl;
    cout << "      實際上 length 先初始化。書寫順序反過來，容易誤導讀者！" << endl;
    cout << "  SafeOrder: length(len), data(new int[length])" << endl;
    cout << "    → 書寫順序和宣告順序一致，清晰明瞭。" << endl;
    SafeOrder so(10);
    cout << "  最佳實踐：初始化列表的書寫順序永遠保持和宣告順序一致！" << endl;
    cout << "  編譯加 -Wall -Wextra 可偵測順序不一致的警告。" << endl;

    // --- 重點五：初始化列表中使用表達式 ---
    cout << "\n【5】初始化列表中可以使用表達式" << endl;

    cout << "--- Circle: 面積和周長用公式計算 ---" << endl;
    Circle circle(5.0);
    circle.print();

    cout << "--- FullName: 字串串接 ---" << endl;
    FullName fn("信安", "陳");
    fn.print();

    // --- 重點六：C++11 類別內預設值配合初始化列表 ---
    cout << "\n【6】C++11 類別內預設值配合初始化列表（GameConfig）" << endl;

    cout << "配置 1：全部使用預設值" << endl;
    GameConfig cfg1;
    cfg1.print();

    cout << "\n配置 2：只覆蓋解析度，其餘用預設值" << endl;
    GameConfig cfg2(1920, 1080);
    cfg2.print();

    cout << "\n配置 3：全部自定義（覆蓋所有預設值）" << endl;
    GameConfig cfg3(2560, 1440, true, "RPG World", 144);
    cfg3.print();

    cout << "\n  覆蓋規則：" << endl;
    cout << "    初始化列表有指定 → 使用列表的值" << endl;
    cout << "    初始化列表沒有   → 使用類別內預設值" << endl;
    cout << "    類別內也沒有     → 類別型別呼叫預設建構函數；" << endl;
    cout << "                       內建型別不初始化，值為不定值（讀取是未定義行為）" << endl;

    // --- 重點七：效能對比 ---
    cout << "\n【7】效能對比：函數體賦值 vs 初始化列表" << endl;

    cout << "方式 A：函數體賦值（預設建構 + 賦值 = 兩步）" << endl;
    ContainerA ca("Hello");

    cout << "\n方式 B：初始化列表（直接帶參建構 = 一步）" << endl;
    ContainerB cb("Hello");

    cout << "\n  結論：" << endl;
    cout << "    函數體賦值：預設建構函數 + 賦值運算子 = 2 次操作" << endl;
    cout << "    初始化列表：帶參建構函數 = 1 次操作" << endl;
    cout << "    對於大型物件，效能差異非常明顯！" << endl;

    // --- 重點八：綜合範例 RPG 武器系統 ---
    cout << "\n【8】綜合範例：RPG 武器系統" << endl;
    cout << "  展示：const成員 + 無預設建構成員 + 類別內預設值 + 預設參數" << endl;

    cout << "\n--- 鍛造武器 ---" << endl;
    Weapon sword(1001, "屠龍劍", "傳說", 5, 150.0, 0.15);
    Weapon bow(1002, "風之弓", "史詩", 4, 95.0, 0.25);
    Weapon staff(1003, "智慧之杖", "精良", 3, 120.0);  // 暴擊率用預設值 0.05

    cout << "\n--- 武器資訊 ---" << endl;
    sword.print();
    bow.print();
    staff.print();

    cout << "\n--- 強化武器 ---" << endl;
    sword.enhance();
    sword.enhance();
    sword.enhance();

    cout << "\n--- 裝備武器 ---" << endl;
    sword.equip();

    cout << "\n--- 最終狀態 ---" << endl;
    sword.print();

    // --- LeetCode 實戰 1：146. LRU Cache ---
    cout << "\n【9】LeetCode 146. LRU Cache（const 成員 + 容器成員一次建好）" << endl;
    {
        LRUCache cache(2);          // capacity_ 是 const 成員，只能用初始化列表
        cache.put(1, 1);
        cache.put(2, 2);
        cout << "  get(1) = " << cache.get(1) << "  （命中，1 成為最近使用）" << endl;
        cache.put(3, 3);            // 容量滿 → 淘汰最久未用的 key 2
        cout << "  get(2) = " << cache.get(2) << "  （已被淘汰，回 -1）" << endl;
        cache.put(4, 4);            // 再淘汰 key 1
        cout << "  get(1) = " << cache.get(1) << "  （已被淘汰，回 -1）" << endl;
        cout << "  get(3) = " << cache.get(3) << endl;
        cout << "  get(4) = " << cache.get(4) << endl;
    }

    // --- LeetCode 實戰 2：705. Design HashSet ---
    cout << "\n【10】LeetCode 705. Design HashSet（桶陣列在初始化列表一次配置）" << endl;
    {
        MyHashSet hs;
        hs.add(1);
        hs.add(2);
        cout << "  contains(1) = " << (hs.contains(1) ? "true" : "false") << endl;
        cout << "  contains(3) = " << (hs.contains(3) ? "true" : "false") << endl;
        hs.add(2);                  // 重複加入不應產生第二份
        hs.remove(2);
        cout << "  remove(2) 後 contains(2) = "
             << (hs.contains(2) ? "true" : "false") << endl;
        hs.add(770);                // 770 % 769 == 1，與 key 1 落在同一個桶
        cout << "  加入 770（與 1 同桶）後 contains(1) = "
             << (hs.contains(1) ? "true" : "false") << endl;
        cout << "  contains(770) = " << (hs.contains(770) ? "true" : "false") << endl;
    }

    // --- 日常實務：服務啟動設定 ---
    cout << "\n【11】日常實務：服務啟動設定（const + 參考 + 無預設建構 + NSDMI）" << endl;
    {
        ServiceContext standard("svc-a1", cout, "postgres://primary/appdb");
        ServiceContext heavy("svc-b7", cout, "postgres://primary/appdb", 128, true);
        standard.report();
        heavy.report();
        cout << "  ↑ 前者三個調校欄位全部沿用 NSDMI；" << endl;
        cout << "    後者覆蓋了 maxConn 與 tracing，timeout 仍沿用 NSDMI 的 3000" << endl;
    }

    // --- 重點回顧 ---
    cout << "\n=====================================================" << endl;
    cout << "本課重點回顧：" << endl;
    cout << "  1. 初始化列表語法: : member(value) 寫在參數列表後、函數體前" << endl;
    cout << "  2. 初始化(一步) vs 賦值(兩步)：初始化列表省去預設建構的開銷" << endl;
    cout << "  3. 四種必須使用的情況：const成員、引用成員、無預設建構成員、基類" << endl;
    cout << "  4. 初始化順序按成員宣告順序，不是列表書寫順序" << endl;
    cout << "  5. 書寫順序和宣告順序不一致 → 容易造成依賴性 bug" << endl;
    cout << "  6. 初始化列表中可以使用任意表達式（公式計算、字串串接等）" << endl;
    cout << "  7. C++11 類別內預設值：初始化列表會覆蓋預設值，未覆蓋的用預設值" << endl;
    cout << "  8. 效能：對類別型別成員，初始化列表省去預設建構+賦值的雙重開銷" << endl;
    cout << "  9. 最佳實踐：永遠使用初始化列表，書寫順序保持和宣告順序一致" << endl;
    cout << "=====================================================" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary
//
// ※ 重要說明（放在預期輸出標記之前）：
//   本檔**刻意**保留 6 個 -Wreorder 警告（OrderDemo 與 DangerousOrder），
//   因為「初始化列表的書寫順序不等於實際初始化順序」正是本課重點四、
//   重點五要教的內容。編譯時看到這些警告是預期行為，不是本檔寫壞了。
//   其餘所有類別皆無警告。
//
//   另外，本檔輸出完全確定：沒有任何地方讀取未初始化的成員，
//   DangerousOrder 也只定義而不建立物件。

// === 預期輸出 ===
// =====================================================
//   第 16 課：建構函數初始化列表（Member Initializer List）
// =====================================================
//
// 【1】初始化列表 vs 函數體賦值的本質區別
// --- 函數體賦值（兩步：先預設建構，再賦值）---
//   [賦值方式] 賦值前 name = []（已被預設建構）
//   name = Hello, value = 42
// --- 初始化列表（一步：直接建構）---
//   [初始化列表] name 直接就是 [Hello]（一步完成）
//   name = Hello, value = 42
//
// 【2】必須使用初始化列表的四種情況
//
// 情況 1：const 成員變數
//   學號: 20250001, 姓名: 張三
//   // const int studentId; → 只能初始化，不能賦值
//
// 情況 2：引用成員變數
//   [INFO] 程式啟動
//   [INFO] 初始化完成
//   // ostream& output; → 引用必須在宣告時綁定
//
// 情況 3：沒有預設建構函數的成員物件
//   引擎建構: 300 馬力, 汽油
//   汽車建構: BMW
//   品牌: BMW
//   引擎: 300 HP (汽油)
//   // Engine engine; → Engine 只有帶參建構，無法預設建構
//
// 情況 4：基類的建構函數（繼承時）
//   Animal 建構: 犬科
//   Dog 建構: 旺財
//   旺財 (犬科)
//   // : Animal("犬科") → 必須在初始化列表中調用基類建構
//
// 【3】初始化順序陷阱：按宣告順序，不是書寫順序
//   列表書寫順序: c, a, b
//   實際初始化順序: a, b, c（按宣告順序）
//   a = 10, b = 11, c = 12
//   結果正確（此例沒有依賴問題），但書寫順序容易誤導！
//
// 【4】危險的順序依賴（DangerousOrder vs SafeOrder）
//   DangerousOrder: data(new int[length]), length(len)
//     → 書寫順序暗示 data 先用 length 分配，但 length 宣告在前，
//       實際上 length 先初始化。書寫順序反過來，容易誤導讀者！
//   SafeOrder: length(len), data(new int[length])
//     → 書寫順序和宣告順序一致，清晰明瞭。
//   安全分配了 10 個元素
//   最佳實踐：初始化列表的書寫順序永遠保持和宣告順序一致！
//   編譯加 -Wall -Wextra 可偵測順序不一致的警告。
//
// 【5】初始化列表中可以使用表達式
// --- Circle: 面積和周長用公式計算 ---
//   半徑: 5
//   面積: 78.5398
//   周長: 31.4159
// --- FullName: 字串串接 ---
//   姓: 陳, 名: 信安, 全名: 陳 信安
//
// 【6】C++11 類別內預設值配合初始化列表（GameConfig）
// 配置 1：全部使用預設值
//   [預設建構] 使用所有預設值
//   遊戲: My Game
//   解析度: 1280x720
//   全螢幕: 否
//   FPS: 60
//
// 配置 2：只覆蓋解析度，其餘用預設值
//   [部分自定義] 只改解析度
//   遊戲: My Game
//   解析度: 1920x1080
//   全螢幕: 否
//   FPS: 60
//
// 配置 3：全部自定義（覆蓋所有預設值）
//   [完全自定義] 所有參數指定
//   遊戲: RPG World
//   解析度: 2560x1440
//   全螢幕: 是
//   FPS: 144
//
//   覆蓋規則：
//     初始化列表有指定 → 使用列表的值
//     初始化列表沒有   → 使用類別內預設值
//     類別內也沒有     → 類別型別呼叫預設建構函數；
//                        內建型別不初始化，值為不定值（讀取是未定義行為）
//
// 【7】效能對比：函數體賦值 vs 初始化列表
// 方式 A：函數體賦值（預設建構 + 賦值 = 兩步）
//     HeavyObject 預設建構
//     --- 開始賦值 ---
//     HeavyObject 賦值運算子: Hello
//     --- 賦值完成 ---
//
// 方式 B：初始化列表（直接帶參建構 = 一步）
//     HeavyObject 帶參建構: Hello
//     --- 初始化列表完成 ---
//
//   結論：
//     函數體賦值：預設建構函數 + 賦值運算子 = 2 次操作
//     初始化列表：帶參建構函數 = 1 次操作
//     對於大型物件，效能差異非常明顯！
//
// 【8】綜合範例：RPG 武器系統
//   展示：const成員 + 無預設建構成員 + 類別內預設值 + 預設參數
//
// --- 鍛造武器 ---
//   鍛造武器: 屠龍劍 [ID:1001]
//   鍛造武器: 風之弓 [ID:1002]
//   鍛造武器: 智慧之杖 [ID:1003]
//
// --- 武器資訊 ---
//   +---------------------------------+
//   | [1001] 屠龍劍
//   | 稀有度: 傳說 (*****)
//   | 傷害: 150  暴擊率: 15%
//   | 強化: +0  [未裝備]
//   +---------------------------------+
//   +---------------------------------+
//   | [1002] 風之弓
//   | 稀有度: 史詩 (****)
//   | 傷害: 95  暴擊率: 25%
//   | 強化: +0  [未裝備]
//   +---------------------------------+
//   +---------------------------------+
//   | [1003] 智慧之杖
//   | 稀有度: 精良 (***)
//   | 傷害: 120  暴擊率: 5%
//   | 強化: +0  [未裝備]
//   +---------------------------------+
//
// --- 強化武器 ---
//   屠龍劍 強化至 +1（傷害: 165）
//   屠龍劍 強化至 +2（傷害: 181.5）
//   屠龍劍 強化至 +3（傷害: 199.65）
//
// --- 裝備武器 ---
//
// --- 最終狀態 ---
//   +---------------------------------+
//   | [1001] 屠龍劍
//   | 稀有度: 傳說 (*****)
//   | 傷害: 199.65  暴擊率: 15%
//   | 強化: +3  [已裝備]
//   +---------------------------------+
//
// 【9】LeetCode 146. LRU Cache（const 成員 + 容器成員一次建好）
//   get(1) = 1  （命中，1 成為最近使用）
//   get(2) = -1  （已被淘汰，回 -1）
//   get(1) = -1  （已被淘汰，回 -1）
//   get(3) = 3
//   get(4) = 4
//
// 【10】LeetCode 705. Design HashSet（桶陣列在初始化列表一次配置）
//   contains(1) = true
//   contains(3) = false
//   remove(2) 後 contains(2) = false
//   加入 770（與 1 同桶）後 contains(1) = true
//   contains(770) = true
//
// 【11】日常實務：服務啟動設定（const + 參考 + 無預設建構 + NSDMI）
//   [svc-a1] db=postgres://primary/appdb, maxConn=32, timeout=3000ms, tracing=off
//   [svc-b7] db=postgres://primary/appdb, maxConn=128, timeout=3000ms, tracing=on
//   ↑ 前者三個調校欄位全部沿用 NSDMI；
//     後者覆蓋了 maxConn 與 tracing，timeout 仍沿用 NSDMI 的 3000
//
// =====================================================
// 本課重點回顧：
//   1. 初始化列表語法: : member(value) 寫在參數列表後、函數體前
//   2. 初始化(一步) vs 賦值(兩步)：初始化列表省去預設建構的開銷
//   3. 四種必須使用的情況：const成員、引用成員、無預設建構成員、基類
//   4. 初始化順序按成員宣告順序，不是列表書寫順序
//   5. 書寫順序和宣告順序不一致 → 容易造成依賴性 bug
//   6. 初始化列表中可以使用任意表達式（公式計算、字串串接等）
//   7. C++11 類別內預設值：初始化列表會覆蓋預設值，未覆蓋的用預設值
//   8. 效能：對類別型別成員，初始化列表省去預設建構+賦值的雙重開銷
//   9. 最佳實踐：永遠使用初始化列表，書寫順序保持和宣告順序一致
// =====================================================
