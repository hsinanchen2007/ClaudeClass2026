// =============================================================================
//  第 9 課：成員變數（Member Variables） — summary.cpp
//  成員變數總整理：型別選擇 / 容器成員 / 組合 / 陣列 / 初始化
// =============================================================================
//
//  ⚠️ 注意：本檔的「重點五」段落【刻意保留了一段未定義行為】——
//     它讀取三個未初始化的成員來示範危險性。
//     因此本檔的輸出【有三行是不確定的】，
//     下方預期輸出對那三行不列出數值（詳見檔尾說明）。
//
// 【主題資訊 Information】
//   成員變數的型別選擇：
//       內建型別   int / double / bool / char   —— 【必須】給類內初始值
//       容器型別   std::string / std::vector    —— 有預設建構子，可省略
//       陣列       double scores[5] = {};       —— 也能類內初始化（C++11）
//       物件成員   Engine engine;               —— 組合（has-a）
//   類內初始值（NSDMI）：C++11 起；只能用 = 或 {}，不可用 ()
//   標準版本：類內初始值 C++11；其餘 C++98
//   標頭檔  ：語言核心特性；本檔另用 <iostream> <string> <vector>
//
// 【詳細解釋 Explanation】
//
// 【1. 本課最重要的一條規則】
//   把整課濃縮成一句話：
//     【內建型別成員一定要給類內初始值；
//       類別型別成員可以依賴它自己的預設建構子。】
//   原因是兩者的預設初始化行為完全不同：
//       int a;              → 什麼都不做，值不確定，讀取即 UB
//       std::string name;   → 呼叫預設建構子，保證是合法的空字串
//   本檔的 Dangerous（無初始值）與 GameCharacter（全部有初始值）
//   並排放在一起，就是這條規則最直接的對照。
//
// 【2. 成員型別的選擇準則】
//       這個欄位的值域是什麼？          → 決定 int / double / bool / char
//       數量固定且已知？                → 陣列或 std::array
//       數量會變動？                    → std::vector
//       它是另一個完整的概念？          → 物件成員（組合）
//   選對型別能讓一大類錯誤在編譯期就被擋掉，
//   例如不可能把字串指派給 temperature。
//
// 【3. 組合（composition）與生命週期】
//   Car 內含 Engine 物件 —— 這是【內嵌實體】而非指標，
//   所以 sizeof(Car) 包含整個 Engine，
//   而且 engine 隨著 Car 自動建構、自動解構，
//   不需要 new/delete，不可能洩漏或懸空。
//   成員依【宣告順序】建構、【反序】解構；
//   Car 的建構子本體執行時，engine 已經完全建構好了。
//
// 【4. 「已初始化」與「有效」是兩個不同層次】
//   類內初始值保證的是「不會讀到垃圾值」（消除 UB）；
//   它【不保證】物件的狀態有意義。
//   本檔的 GameCharacter c1 叫「未命名」、HP 100 ——
//   完全合法，但那不是一個真實的角色。
//   要保證有意義，必須靠建構子（驗證參數、建立成員間的相依）。
//   兩者是互補的：
//       類內初始值 → 防未定義行為
//       建構子     → 防無意義的狀態
//
// 【概念補充 Concept Deep Dive】
//
// (A) 對齊填充讓 sizeof 大於成員總和
//   本課多處實測（皆屬實作定義，x86-64 / g++ 15.2.0）：
//       Sensor    = 24（資料只有 14 bytes，10 bytes 是填充）
//       Classroom = 88（string 32 + string 32 + vector 24）
//       Car       = 72（string 32 + Engine 40）
//       Student   = 88（string 32 + int 4 +填充4 + double[5] 40 + int 4 +填充4）
//   實用推論：成員【由大到小】宣告通常能減少填充。
//   Sensor 改成 double, int, bool, char 的順序後從 24 降到 16，
//   本機已實測確認。
//
// (B) 容器成員讓物件大小與內容量脫鉤
//   sizeof(Classroom) 永遠是 88，不管裝了 3 個還是 3000 個學生 ——
//   vector 物件本身只有三個指標，實際資料在堆積上。
//   而且本檔沒有寫任何解構子，卻不會洩漏記憶體：
//   編譯器產生的解構子會依反序呼叫每個成員的解構子。
//   這就是 RAII —— 資源釋放綁定在物件生命週期上。
//
// (C) 為什麼 C++ 不乾脆全部自動歸零
//   「不為你沒要求的東西付費」是 C/C++ 的核心哲學：
//   宣告一個 1MB 的緩衝區若強制先清空，
//   即使馬上要用資料覆蓋它，那個成本也省不掉。
//   注意【靜態儲存期】的變數（全域、static）是【保證零初始化】的，
//   因為那是程式載入時由作業系統清空 .bss 一次完成、沒有額外成本。
//   所以「會不會自動歸零」取決於【儲存期】而不是型別。
//
// 【注意事項 Pay Attention】
//   1. 本檔「重點五」段落刻意保留未定義行為，該處三行輸出不確定。
//   2. 內建型別成員不會自動歸零；string / vector 等類別型別會。
//   3. 指標成員一定要寫 = nullptr —— 未初始化的指標讓 if (ptr) 檢查失效。
//   4. 未初始化的 bool 可能持有既非 0 也非 1 的位元樣貌
//      （本機實測印出過 2、4、6、8），會讓 if(c) 與 if(!c) 失去一致性。
//   5. 類內初始值是 C++11 起的功能，只能用 = 或 {}，不可用 ()。
//   6. 全域與 static 變數【是】保證零初始化的 —— 差別在儲存期，不在型別。
//   7. 所有 sizeof 數值皆為本機 x86-64 / g++ 15.2.0 實測，屬實作定義。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】成員變數與初始化
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 哪些成員會自動初始化、哪些不會？判斷的依據是什麼？
//     答：依據是【這個型別有沒有自己的預設建構子】。
//         std::string、std::vector 等類別型別有，
//         預設初始化時會被建構成明確的空狀態，安全。
//         int、double、bool、char、指標等內建型別沒有，
//         預設初始化【什麼都不做】，值不確定、讀取即 UB。
//         所以規則是：內建型別成員一律給類內初始值，
//         類別型別成員可以依賴其預設建構子。
//     追問：那全域變數呢？
//           → 全域與 static（靜態儲存期）的變數【保證零初始化】，
//             因為那是程式載入時清空 .bss 一次完成的。
//             所以「會不會歸零」取決於儲存期而不是型別 ——
//             這也是「把變數改成 static 之後 bug 就消失了」
//             這種現象的真正原因（bug 被藏起來了，不是修好了）。
//
// 🔥 Q2. 類內初始值是哪個標準加入的？和建構子初始化列表衝突時以哪個為準？
//     答：【C++11】。以建構子初始化列表為準 ——
//         類內初始值的角色是「所有建構子共用的預設值」，
//         任何建構子若明確指定了該成員就會覆蓋它。
//         最佳實務是把共通預設值寫成類內初始值、
//         把依參數而異的放進初始化列表，兩者互補。
//     追問：初始化的順序由什麼決定？
//           → 永遠由【成員的宣告順序】決定，
//             與初始化列表的書寫順序無關。
//             因為解構必須以嚴格反序進行，
//             若順序可以隨建構子而異，編譯器就無法決定唯一正確的解構順序。
//             -Wall 包含的 -Wreorder 會在兩者不一致時警告。
//
// ⚠️ 陷阱. 「我把所有成員都加上類內初始值了，
//          所以這個類別已經完全安全、不需要寫建構子」—— 錯在哪？
//     答：類內初始值保證的是「不會讀到未初始化的垃圾值」，
//         這確實消除了 UB —— 但它【不保證狀態有意義】。
//         本檔的 GameCharacter c1 叫「未命名」、HP 100、MP 50；
//         第 9 課 8 號檔的空白 Student 是「未命名 / ID 0 / 平均 0 / 等級 F」。
//         這些都是合法的物件，卻不是任何真實存在的東西。
//     為什麼會錯：把兩個不同層次的保證混為一談 ——
//           已初始化 → 沒有 UB（類內初始值就能做到）
//           有效     → 滿足業務不變量（只有建構子能做到）
//         類內初始值無法做三件事：依參數決定初值、驗證參數、
//         建立成員之間的相依關係。
//         真正的封裝要求物件【一建立就有意義】，
//         做法是提供必要的建構子（必要時 = delete 掉預設建構子），
//         讓「半成品物件」在型別層次上就不可能存在。
//         兩者是互補而非替代：
//         類內初始值防 UB，建構子防無意義的狀態。
// ═══════════════════════════════════════════════════════════════════════════

/*
 * ================================================================
 * 【第 9 課：成員變數（Member Variables）】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. 成員變數是描述對象「狀態（state）」的資料
 * 2. 基本型別作為成員：int, double, bool, char
 * 3. 字串與容器作為成員：string, vector（會自動初始化）
 * 4. 另一個類別作為成員：組合（Composition）
 * 5. 固定大小陣列作為成員
 * 6. 未初始化的危險：基本型別不自動初始化，值是垃圾
 * 7. 哪些型別會自動初始化（class 型別），哪些不會（基本型別）
 * 8. 類內初始化（C++11 起）：最推薦的初始化方式
 * 9. 存取控制預告：public 的成員變數可以被隨意修改（第 11 課保護）
 * ================================================================
 */

#include <iostream>
#include <string>
#include <vector>
using namespace std;


// ===== 重點一：基本型別作為成員變數 =====
// 說明：int, double, bool, char 都可以作為成員變數。
// 重要！這些基本型別如果不手動初始化，其值是「垃圾值（garbage value）」。
// 建議：一律使用類內初始化（= 賦值語法）給予預設值，避免未定義行為。

class Sensor {
public:
    int id;            // 危險：未初始化，是垃圾值
    double temperature;
    bool isActive;
    char grade;

    void show() {
        cout << "ID: " << id
             << " 溫度: " << temperature
             << " 啟用: " << (isActive ? "是" : "否")
             << " 等級: " << grade << endl;
    }
};


// ===== 重點二：字串和容器作為成員變數 =====
// 說明：string 和 vector 是 C++ 類別，有預設建構函數，會自動初始化。
// string 預設為空字串 ""，vector 預設為空容器 {}。
// 因此不需要擔心它們的初始化問題——它們天生就是安全的。

class Classroom {
public:
    string teacherName;          // 自動初始化為 ""（安全）
    string roomNumber;           // 自動初始化為 ""（安全）
    vector<string> students;     // 自動初始化為空容器（安全）

    void addStudent(const string& name) {
        students.push_back(name);
    }

    void show() {
        cout << "教室: " << roomNumber << " | 老師: " << teacherName << endl;
        cout << "學生(" << students.size() << "人): ";
        for (int i = 0; i < (int)students.size(); i++) {
            cout << students[i];
            if (i < (int)students.size() - 1) cout << ", ";
        }
        cout << endl;
    }
};


// ===== 重點三：另一個類別作為成員（組合，Composition）=====
// 說明：類別可以包含另一個類別的對象作為成員變數，這稱為「組合」。
// Car「擁有」一個 Engine——Car 和 Engine 是「has-a」關係。
// 存取語法：car.engine.horsepower（一層層進入內部對象）。
// 被包含的類別（Engine）的成員函數在建立 Car 對象時也會被自動初始化。

class Engine {
public:
    int horsepower = 0;
    string fuelType = "汽油";

    void start() {
        cout << horsepower << " 匹馬力的 " << fuelType << " 引擎啟動！" << endl;
    }
};

class Car {
public:
    string brand;
    Engine engine;   // Car「擁有」一個 Engine（組合關係）

    void drive() {
        cout << brand << " 開始行駛 → ";
        engine.start();
    }
};


// ===== 重點四：固定大小陣列作為成員 =====
// 說明：固定大小的 C 風格陣列可以直接作為成員變數。
// 注意：這類陣列成員若未初始化，其元素值也是垃圾值。
// C++11 起可以使用類內初始化：double data[2][2] = {};

class Matrix2x2 {
public:
    double data[2][2];  // 固定大小二維陣列作為成員

    void set(double a, double b, double c, double d) {
        data[0][0] = a; data[0][1] = b;
        data[1][0] = c; data[1][1] = d;
    }

    void print() {
        cout << "| " << data[0][0] << "  " << data[0][1] << " |" << endl;
        cout << "| " << data[1][0] << "  " << data[1][1] << " |" << endl;
    }

    double determinant() {
        return data[0][0] * data[1][1] - data[0][1] * data[1][0];
    }
};


// ===== 重點五：未初始化的危險 =====
// 說明：基本型別（int, double, bool, char, 指標）的成員變數如果不初始化，
// 其值是「未定義（Undefined Behavior, UB）」——每次執行結果可能不同！
// 可能導致程式崩潰、計算錯誤、安全漏洞。
// 規則：
//   不自動初始化（危險）：int, double, char, bool, int* 等
//   自動初始化（安全）：string, vector, map 等 STL 類別

class Dangerous {
public:
    int x;       // 垃圾值！
    double y;    // 垃圾值！
    bool flag;   // 垃圾值！
    // int* ptr; // 野指標！千萬不要 dereference
};


// ===== 重點六：類內初始化（C++11 起）—— 最推薦！=====
// 說明：直接在成員變數宣告時給予預設值。
// 這是 C++11 引入的特性，語法簡單明瞭。
// 優點：所有對象建立時都有安全的預設值，避免遺漏初始化。
// 可以隨時覆蓋預設值（在建立對象後再賦值）。

class GameCharacter {
public:
    string name = "未命名";   // 有預設值
    int hp = 100;
    int mp = 50;
    double speed = 1.0;
    bool isAlive = true;

    void show() {
        cout << name << " | HP:" << hp << " MP:" << mp
             << " 速度:" << speed
             << " 狀態:" << (isAlive ? "存活" : "陣亡") << endl;
    }
};


// ===== 重點七：存取控制預告（第 11 課詳述）=====
// 說明：目前所有成員都是 public，外界可以隨意修改，這是不安全的。
// 例如：player.hp = -9999; 完全合法但不合理。
// 解決方案：第 11 課的 private 修飾符 + 第 21 課的 getter/setter。


// ===== 綜合實戰：學生成績管理（完整展示所有成員變數種類）=====
class Student {
public:
    // 各種型別的成員變數，全部使用類內初始化
    string name = "未命名";
    int id = 0;
    double scores[5] = {0, 0, 0, 0, 0};   // 陣列也能類內初始化
    int scoreCount = 0;

    void addScore(double score) {
        if (scoreCount >= 5) { cout << "最多5科" << endl; return; }
        if (score < 0 || score > 100) { cout << "成績需在0~100" << endl; return; }
        scores[scoreCount++] = score;
    }

    double average() {
        if (scoreCount == 0) return 0.0;
        double sum = 0;
        for (int i = 0; i < scoreCount; i++) sum += scores[i];
        return sum / scoreCount;
    }

    double highest() {
        if (scoreCount == 0) return 0.0;
        double max = scores[0];
        for (int i = 1; i < scoreCount; i++) if (scores[i] > max) max = scores[i];
        return max;
    }

    double lowest() {
        if (scoreCount == 0) return 0.0;
        double min = scores[0];
        for (int i = 1; i < scoreCount; i++) if (scores[i] < min) min = scores[i];
        return min;
    }

    void showReport() {
        cout << "=== " << name << " (ID:" << id << ") ===" << endl;
        cout << "成績: ";
        for (int i = 0; i < scoreCount; i++) {
            cout << scores[i];
            if (i < scoreCount - 1) cout << ", ";
        }
        cout << endl;
        cout << "平均:" << average() << " 最高:" << highest() << " 最低:" << lowest() << endl;
        double avg = average();
        cout << "等級: ";
        if      (avg >= 90) cout << "A(優秀)";
        else if (avg >= 80) cout << "B(良好)";
        else if (avg >= 70) cout << "C(中等)";
        else if (avg >= 60) cout << "D(及格)";
        else                cout << "F(不及格)";
        cout << endl;
    }
};


int main() {
    cout << "===== 重點一：基本型別成員（需手動初始化）=====" << endl;
    Sensor s;
    s.id = 1001;
    s.temperature = 36.5;
    s.isActive = true;
    s.grade = 'A';
    s.show();


    cout << "\n===== 重點二：string 和 vector 成員（自動初始化）=====" << endl;
    Classroom room;
    room.teacherName = "王老師";
    room.roomNumber  = "A-301";
    room.addStudent("小明");
    room.addStudent("小華");
    room.addStudent("小美");
    room.show();


    cout << "\n===== 重點三：組合（Composition）Car 包含 Engine =====" << endl;
    Car car;
    car.brand = "Toyota";
    car.engine.horsepower = 150;       // 進入內層對象設值
    car.engine.fuelType = "油電混合";   // 鏈式 . 存取
    car.drive();


    cout << "\n===== 重點四：固定大小陣列成員 =====" << endl;
    Matrix2x2 m;
    m.set(3, 7, 1, 5);
    m.print();
    cout << "行列式 = " << m.determinant() << endl;


    cout << "\n===== 重點五：未初始化的危險（示意，值為垃圾）=====" << endl;
    // 注意：以下輸出的值每次執行都可能不同，這就是「未定義行為」
    Dangerous d;
    cout << "未初始化 int x    = " << d.x    << "（垃圾值！）" << endl;
    cout << "未初始化 double y = " << d.y    << "（垃圾值！）" << endl;
    cout << "未初始化 bool flag= " << d.flag << "（垃圾值！）" << endl;
    cout << "-> 千萬不能在業務邏輯中依賴未初始化的值！" << endl;


    cout << "\n===== 重點六：類內初始化（C++11）—— 最推薦 =====" << endl;
    GameCharacter c1;           // 全部使用預設值
    c1.show();

    GameCharacter c2;
    c2.name = "戰士";           // 只覆蓋部分預設值
    c2.hp = 150;
    c2.show();


    cout << "\n===== 綜合實戰：學生成績管理 =====" << endl;
    Student s1;
    s1.name = "陳信安";
    s1.id = 2001;
    s1.addScore(92); s1.addScore(88); s1.addScore(95);
    s1.addScore(78); s1.addScore(85);
    s1.showReport();

    cout << endl;

    Student s2;   // 使用所有預設值，不設任何資料
    cout << "未設定的學生（全部使用預設值）:" << endl;
    s2.showReport();


    cout << "\n===== 成員變數初始化規則速查 =====" << endl;
    cout << "int, double, char, bool, 指標 -> 不自動初始化 -> 必須手動給值！" << endl;
    cout << "string, vector, map 等 STL  -> 自動初始化     -> 安全" << endl;
    cout << "其他 class 型別成員          -> 呼叫其預設建構 -> 安全" << endl;
    cout << "C++11 類內初始化             -> 最簡潔推薦方式" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o member_summary
//   編譯時會出現 3 個 -Wuninitialized 警告（重點五段落刻意讀取未初始化成員）。
//   這些警告【不是雜訊，而是該段落的重點】。
//   類內初始值為 C++11 起的功能。

// ⚠️ 註 1:【本檔的輸出有三行是不確定的。】
//      「重點五：未初始化的危險」段落刻意讀取三個未初始化的成員，
//      屬【未定義行為】—— 下方預期輸出對那三行不列出數值。
//      本機逐行比對 12 次執行的結果，確認【只有】那三行會變動，
//      其餘所有行每次都完全相同。

// 註 2:那三行的本機實測（僅作為「有多不可靠」的證據，不是預期結果）：
//        int x    出現過 0、4、6、8
//        double y 出現過 0、1.97626e-323、2.96439e-323、3.95253e-323
//        bool flag 出現過 2、4、6、8
//      特別注意 flag —— 它是 bool，卻【從未印出 0 或 1】。
//      bool 只佔 1 byte、合法位元樣貌只有 0 與 1，
//      這種值會讓 if (flag) 與 if (!flag) 失去一致性，
//      因為編譯器產生的程式碼假設它只會是 0 或 1。
//      這說明 UB 不只是「值不對」，而是【邏輯崩壞】。

// 註 3:對照「重點六：類內初始化」段落 —— 同樣沒有手動賦值，
//      GameCharacter c1 卻印出完全確定的「未命名 | HP:100 MP:50」。
//      「有沒有那一行 = 100」就是「有沒有 UB」的分界線。

// 註 4:本課各類別的 sizeof 實測（x86-64 / g++ 15.2.0，皆屬實作定義）：
//        Sensor=24、Classroom=88、Engine=40、Car=72、Matrix2x2=32、Student=88。
//      Sensor 的成員資料只有 14 bytes，其餘 10 bytes 全是對齊填充；
//      改成 double, int, bool, char 的宣告順序後降到 16 bytes（本機已實測）。

// 註 5:「平均: 87.6」= (92+88+95+78+85)/5 = 438/5，是精確的算術結果。
//      浮點數印成 87.6 而非 87.600000 是 iostream 預設格式所致。

// === 預期輸出 ===
// ===== 重點一：基本型別成員（需手動初始化）=====
// ID: 1001 溫度: 36.5 啟用: 是 等級: A
//
// ===== 重點二：string 和 vector 成員（自動初始化）=====
// 教室: A-301 | 老師: 王老師
// 學生(3人): 小明, 小華, 小美
//
// ===== 重點三：組合（Composition）Car 包含 Engine =====
// Toyota 開始行駛 → 150 匹馬力的 油電混合 引擎啟動！
//
// ===== 重點四：固定大小陣列成員 =====
// | 3  7 |
// | 1  5 |
// 行列式 = 8
//
// ===== 重點五：未初始化的危險（示意，值為垃圾）=====
// 未初始化 int x    = <不確定值，UB>（垃圾值！）
// 未初始化 double y = <不確定值，UB>（垃圾值！）
// 未初始化 bool flag= <不確定值，UB；實測從未印出 0 或 1>（垃圾值！）
// -> 千萬不能在業務邏輯中依賴未初始化的值！
//
// ===== 重點六：類內初始化（C++11）—— 最推薦 =====
// 未命名 | HP:100 MP:50 速度:1 狀態:存活
// 戰士 | HP:150 MP:50 速度:1 狀態:存活
//
// ===== 綜合實戰：學生成績管理 =====
// === 陳信安 (ID:2001) ===
// 成績: 92, 88, 95, 78, 85
// 平均:87.6 最高:95 最低:78
// 等級: B(良好)
//
// 未設定的學生（全部使用預設值）:
// === 未命名 (ID:0) ===
// 成績:
// 平均:0 最高:0 最低:0
// 等級: F(不及格)
//
// ===== 成員變數初始化規則速查 =====
// int, double, char, bool, 指標 -> 不自動初始化 -> 必須手動給值！
// string, vector, map 等 STL  -> 自動初始化     -> 安全
// 其他 class 型別成員          -> 呼叫其預設建構 -> 安全
// C++11 類內初始化             -> 最簡潔推薦方式
//
// （上方標示 <不確定值，UB> 的三行是未定義行為，每次執行都可能不同，
//   本機 12 次執行中該三行各出現 4 種相異結果；
//   請勿把任何一次的數值記錄為本檔的正確輸出。其餘各行皆為確定輸出。）
