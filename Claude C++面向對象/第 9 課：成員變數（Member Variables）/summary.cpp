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
