/*
 * ================================================================
 * 【第 17 課：解構函數（Destructor）】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. 解構函數的語法規則：~ClassName()，無返回值，無參數，不能重載
 * 2. 解構順序與建構順序相反（LIFO，後建構先解構）
 * 3. 解構函數的調用時機：全域/局部/區塊/函數/動態對象
 * 4. 資源管理實際用途：動態記憶體、檔案資源、自動計時器（RAII）
 * 5. 忘記 delete 的後果：記憶體洩漏
 * 6. 編譯器自動生成的解構函數：調用成員的解構函數，但不會 delete 裸指標
 * 7. 解構函數中不要拋出異常
 * 8. 建構函數 vs 解構函數對照表
 * 9. static 成員追蹤存活物件數量（LifeCycle 範例）
 * 10. 綜合範例：Session 管理 DatabaseConnection（delete chain）
 * ================================================================
 */

#include <iostream>
#include <string>
#include <chrono>
#include <fstream>
using namespace std;

// ================================================================
// 重點一：解構函數的語法規則
// ================================================================
// ┌─────────────────┬──────────────────────────────────────────┐
// │ 特徵             │ 說明                                      │
// ├─────────────────┼──────────────────────────────────────────┤
// │ 函數名           │ ~ClassName()（波浪號 + 類別名）            │
// │ 返回值           │ 沒有返回值，連 void 都不寫                │
// │ 參數             │ 不能有參數                                │
// │ 數量             │ 每個類別只能有一個（不能重載）            │
// │ 調用時機         │ 對象被銷毀時自動調用                      │
// └─────────────────┴──────────────────────────────────────────┘
//
// 建構函數和解構函數是一對：
//   建構函數  →  對象誕生時自動調用  →  獲取資源、初始化
//   解構函數  →  對象死亡時自動調用  →  釋放資源、清理

// ================================================================
// 重點二：基本範例 —— 解構順序與建構順序相反（LIFO）
// ================================================================
// 先建構的後解構，後建構的先解構，像堆疊（stack）一樣後進先出。

class SimpleObject {
private:
    string name;

public:
    SimpleObject(const string& n) : name(n) {
        cout << "  [建構] " << name << " 被創建了" << endl;
    }

    ~SimpleObject() {
        cout << "  [解構] " << name << " 被銷毀了" << endl;
    }
};

// ================================================================
// 重點三：解構函數的五種調用時機
// ================================================================
// ┌──────────────┬───────────────────────────────────────────┐
// │ 對象類型      │ 解構時機                                   │
// ├──────────────┼───────────────────────────────────────────┤
// │ 全域對象      │ 程式結束時（main() 返回之後）               │
// │ 局部對象      │ 離開所在作用域時                            │
// │ 區塊內對象    │ 離開區塊的 } 時                             │
// │ 函數內對象    │ 函數返回時                                  │
// │ 動態對象      │ delete 時（不 delete 就不會解構！）          │
// └──────────────┴───────────────────────────────────────────┘

class Tracker {
private:
    string name;

public:
    Tracker(const string& n) : name(n) {
        cout << "  [建構] " << name << endl;
    }

    ~Tracker() {
        cout << "  [解構] " << name << endl;
    }
};

// 全域對象：在 main() 之前建構，在 main() 返回之後解構
Tracker globalObj("全域物件");

// 函數內對象：函數返回時解構
void testFunction() {
    cout << "\n  --- 進入 testFunction ---" << endl;
    Tracker funcObj("函數局部物件");
    cout << "  --- 離開 testFunction ---" << endl;
}  // funcObj 在這裡被解構

// ================================================================
// 重點四：資源管理實際用途 —— 場景 1：管理動態記憶體
// ================================================================
// 建構時 new[] 分配記憶體，解構時 delete[] 釋放記憶體。
// 離開作用域時自動釋放，不需要手動 free/delete。

class DynamicArray {
private:
    int* data;
    int size;

public:
    DynamicArray(int sz) : size(sz) {
        data = new int[size];           // 建構時分配記憶體
        for (int i = 0; i < size; i++) {
            data[i] = 0;
        }
        cout << "  [建構] 分配了 " << size << " 個 int 的記憶體" << endl;
    }

    ~DynamicArray() {
        delete[] data;                  // 解構時釋放記憶體，避免記憶體洩漏
        cout << "  [解構] 釋放了 " << size << " 個 int 的記憶體" << endl;
    }

    void set(int index, int value) {
        if (index >= 0 && index < size) {
            data[index] = value;
        }
    }

    int get(int index) const {
        if (index >= 0 && index < size) {
            return data[index];
        }
        return -1;
    }

    void print() const {
        cout << "  [";
        for (int i = 0; i < size; i++) {
            if (i > 0) cout << ", ";
            cout << data[i];
        }
        cout << "]" << endl;
    }
};

// ================================================================
// 重點四續：場景 2：管理檔案資源
// ================================================================
// 建構時 open 打開檔案，解構時 close 關閉檔案。
// RAII 模式：資源獲取即初始化，資源釋放即解構。

class FileWriter {
private:
    ofstream file;
    string filename;

public:
    FileWriter(const string& fname) : filename(fname) {
        file.open(filename);
        if (file.is_open()) {
            cout << "  [建構] 打開檔案: " << filename << endl;
        } else {
            cout << "  [建構] 無法打開檔案: " << filename << endl;
        }
    }

    ~FileWriter() {
        if (file.is_open()) {
            file.close();               // 自動關閉檔案
            cout << "  [解構] 關閉檔案: " << filename << endl;
        }
    }

    void writeLine(const string& text) {
        if (file.is_open()) {
            file << text << endl;
            cout << "  寫入: " << text << endl;
        }
    }
};

// ================================================================
// 重點四續：場景 3：自動計時器（RAII 模式）
// ================================================================
// 建構時記錄開始時間，解構時計算並印出耗時。
// 這就是 RAII（Resource Acquisition Is Initialization）模式的經典應用。

class ScopedTimer {
private:
    string taskName;
    chrono::high_resolution_clock::time_point startTime;

public:
    ScopedTimer(const string& name) : taskName(name) {
        startTime = chrono::high_resolution_clock::now();
        cout << "  [計時開始] " << taskName << endl;
    }

    ~ScopedTimer() {
        auto endTime = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(
            endTime - startTime
        ).count();
        cout << "  [計時結束] " << taskName
             << " 耗時: " << duration << " ms" << endl;
    }
};

void simulateWork() {
    ScopedTimer timer("模擬工作");

    // 模擬一些耗時操作
    volatile int sum = 0;  // volatile 防止編譯器優化掉
    for (int i = 0; i < 10000000; i++) {
        sum += i;
    }
    cout << "  計算完成" << endl;

    // timer 在函數返回時自動解構，印出耗時
}

// ================================================================
// 重點五：忘記 delete 的後果 —— 記憶體洩漏
// ================================================================
// 動態對象（new 出來的）如果忘記 delete，解構函數永遠不會被調用，
// 該記憶體永遠不會被釋放，造成記憶體洩漏。
// 教訓：盡量使用局部對象（棧上），它們離開作用域自動解構。
//        如果必須用 new，記得 delete——或使用智能指標（第 94-96 課）。

class Resource {
private:
    int id;

public:
    Resource(int i) : id(i) {
        cout << "  [建構] 資源 #" << id << endl;
    }

    ~Resource() {
        cout << "  [解構] 資源 #" << id << endl;
    }
};

// ================================================================
// 重點六：編譯器自動生成的解構函數
// ================================================================
// 如果不寫解構函數，編譯器會自動生成一個。
// 自動生成的解構函數行為：
// ┌────────────────┬────────────────────────────────────────┐
// │ 成員類型        │ 自動生成的解構函數行為                    │
// ├────────────────┼────────────────────────────────────────┤
// │ 基本型別(int等) │ 不做任何事                                │
// │ 類別型別(string)│ 調用該成員的解構函數                      │
// │ 裸指標(int*)    │ 不做任何事！不會 delete！                  │
// └────────────────┴────────────────────────────────────────┘
//
// 重要：如果類別有裸指標成員指向 new 出來的記憶體，
//       你必須自己寫解構函數來 delete，編譯器不會幫你！

class AutoDestructor {
public:
    string name;        // string 有自己的解構函數 → 自動調用
    int value;          // 基本型別 → 不需要清理
    // 沒有定義解構函數 → 編譯器自動生成
    // 自動生成的解構函數會調用 name 的 string::~string()
    // value 是 int，不做任何事
};

class NeedCustomDestructor {
private:
    int* data;          // 裸指標！編譯器不會幫你 delete！

public:
    NeedCustomDestructor(int size) {
        data = new int[size];  // 動態分配
    }

    // 必須手動寫解構函數來釋放記憶體！
    ~NeedCustomDestructor() {
        delete[] data;         // 如果不寫這行，記憶體永遠不會被釋放
    }
};

// ================================================================
// 重點七：解構函數中不要拋出異常
// ================================================================
// 如果解構函數拋出異常，而它是在另一個異常的堆疊展開（stack unwinding）
// 過程中被調用的，程式會直接呼叫 std::terminate() 終止！
// 正確做法：在解構函數內部 try-catch，不讓異常傳播出去。

class SafeCleanup {
private:
    string name;

public:
    SafeCleanup(const string& n) : name(n) {
        cout << "  [建構] SafeCleanup: " << name << endl;
    }

    ~SafeCleanup() {
        try {
            // 模擬可能失敗的清理操作
            cout << "  [解構] SafeCleanup: " << name << " 安全清理" << endl;
        } catch (...) {
            // 記錄錯誤，但不要讓異常傳播出去！
            cerr << "  [錯誤] 清理 " << name << " 時發生異常，已忽略" << endl;
        }
    }
};

// ================================================================
// 重點八：建構函數 vs 解構函數 對照表
// ================================================================
// ┌──────────────┬─────────────────────┬─────────────────────┐
// │ 特徵          │ 建構函數             │ 解構函數             │
// ├──────────────┼─────────────────────┼─────────────────────┤
// │ 名稱          │ ClassName()          │ ~ClassName()         │
// │ 返回值        │ 無                   │ 無                   │
// │ 參數          │ 可以有（支援重載）   │ 不能有（不能重載）   │
// │ 數量          │ 可以有多個           │ 只能有一個           │
// │ 調用時機      │ 對象創建時           │ 對象銷毀時           │
// │ 職責          │ 獲取資源、初始化     │ 釋放資源、清理       │
// │ 自動生成      │ 無自定義時自動生成   │ 無自定義時自動生成   │
// │ 自動生成行為  │ 調用成員的預設建構   │ 調用成員的解構函數   │
// └──────────────┴─────────────────────┴─────────────────────┘

// ================================================================
// 重點九：static 成員追蹤存活物件數量（LifeCycle 範例）
// ================================================================
// 使用 static 成員變數 count 追蹤當前存活的物件數量。
// 建構時 count++，解構時 count--，隨時知道有多少物件存活。

class LifeCycle {
private:
    string name;
    static int count;   // 追蹤當前存活的物件數量

public:
    LifeCycle(const string& n) : name(n) {
        count++;
        cout << "  [建構] " << name
             << " (目前存活: " << count << " 個)" << endl;
    }

    ~LifeCycle() {
        cout << "  [解構] " << name
             << " (目前存活: " << count - 1 << " 個)" << endl;
        count--;
    }

    static int getCount() { return count; }
};

int LifeCycle::count = 0;   // 靜態成員初始化

// ================================================================
// 重點十：綜合範例 —— Session 管理 DatabaseConnection
// ================================================================
// Session 類別持有兩個 DatabaseConnection 的裸指標（new 出來的）。
// Session 的解構函數負責 delete 這兩個指標 → 觸發 DatabaseConnection 的解構。
// 這展示了 delete chain（解構鏈）的概念。

class DatabaseConnection {
private:
    string connectionString;
    int connectionId;
    bool isConnected;
    static int nextId;

public:
    DatabaseConnection(const string& connStr)
        : connectionString(connStr),
          connectionId(nextId++),
          isConnected(false)
    {
        isConnected = true;
        cout << "  [連接 #" << connectionId << "] 已建立: "
             << connectionString << endl;
    }

    ~DatabaseConnection() {
        if (isConnected) {
            isConnected = false;        // 自動斷開連接
            cout << "  [連接 #" << connectionId << "] 已斷開: "
                 << connectionString << endl;
        }
    }

    void query(const string& sql) const {
        if (isConnected) {
            cout << "  [連接 #" << connectionId << "] 執行: " << sql << endl;
        } else {
            cout << "  [連接 #" << connectionId << "] 錯誤: 未連接！" << endl;
        }
    }

    int getId() const { return connectionId; }
};

int DatabaseConnection::nextId = 1;

class Session {
private:
    string sessionName;
    DatabaseConnection* mainDb;     // 裸指標 → 解構函數必須 delete
    DatabaseConnection* cacheDb;    // 裸指標 → 解構函數必須 delete

public:
    Session(const string& name,
            const string& mainConn,
            const string& cacheConn)
        : sessionName(name),
          mainDb(new DatabaseConnection(mainConn)),
          cacheDb(new DatabaseConnection(cacheConn))
    {
        cout << "  [Session] " << sessionName << " 啟動" << endl;
    }

    ~Session() {
        cout << "  [Session] " << sessionName << " 關閉中..." << endl;
        delete cacheDb;     // 先關閉快取連接 → 觸發 DatabaseConnection 解構
        delete mainDb;      // 再關閉主連接   → 觸發 DatabaseConnection 解構
        cout << "  [Session] " << sessionName << " 已完全關閉" << endl;
    }

    void doWork() const {
        mainDb->query("SELECT * FROM users");
        cacheDb->query("GET user:1001");
        mainDb->query("UPDATE users SET active = 1");
    }
};

// ================================================================
// main() —— 逐一展示所有重點
// ================================================================

int main() {
    cout << "=============================================" << endl;
    cout << "   第 17 課：解構函數（Destructor）總複習" << endl;
    cout << "=============================================" << endl;

    // ----------------------------------------------------------
    // 【1】解構函數基本語法 & 解構順序（LIFO）
    // ----------------------------------------------------------
    cout << "\n【1】解構函數基本語法 & 解構順序（LIFO）" << endl;
    cout << "建構順序: A → B，解構順序: B → A（後進先出）" << endl;
    {
        SimpleObject a("物件A");
        SimpleObject b("物件B");
        cout << "  --- 即將離開區塊 ---" << endl;
    }
    // 輸出：先解構 B，再解構 A

    // ----------------------------------------------------------
    // 【2】解構函數的調用時機
    // ----------------------------------------------------------
    cout << "\n【2】解構函數的調用時機" << endl;
    cout << "（全域物件已在 main() 之前建構，見最上方輸出）" << endl;

    // 局部對象
    cout << "\n  --- 局部對象 ---" << endl;
    Tracker localObj("局部物件");

    // 區塊內對象
    {
        cout << "\n  --- 進入區塊 ---" << endl;
        Tracker blockObj("區塊物件");
        cout << "  --- 離開區塊 ---" << endl;
    }  // blockObj 在這裡被解構
    cout << "  --- 區塊已結束 ---" << endl;

    // 函數內對象
    testFunction();

    // 動態對象
    cout << "\n  --- 動態對象 ---" << endl;
    Tracker* heapObj = new Tracker("動態物件");
    cout << "  --- 手動 delete ---" << endl;
    delete heapObj;     // 解構函數在 delete 時調用

    // ----------------------------------------------------------
    // 【3】資源管理：動態記憶體（DynamicArray）
    // ----------------------------------------------------------
    cout << "\n【3】資源管理：動態記憶體（DynamicArray）" << endl;
    {
        DynamicArray arr(5);
        arr.set(0, 10);
        arr.set(1, 20);
        arr.set(2, 30);
        arr.print();
        cout << "  --- 即將離開區塊 ---" << endl;
    }
    // arr 離開作用域，解構函數自動釋放記憶體，不需要手動 delete
    cout << "  --- 記憶體已自動釋放 ---" << endl;

    // ----------------------------------------------------------
    // 【4】資源管理：檔案資源（FileWriter）
    // ----------------------------------------------------------
    cout << "\n【4】資源管理：檔案資源（FileWriter）" << endl;
    {
        FileWriter writer("test_output.txt");
        writer.writeLine("第一行");
        writer.writeLine("第二行");
        writer.writeLine("第三行");
        cout << "  --- 即將離開區塊 ---" << endl;
    }
    // writer 離開作用域，解構函數自動關閉檔案
    cout << "  --- 檔案已自動關閉 ---" << endl;

    // ----------------------------------------------------------
    // 【5】資源管理：自動計時器（ScopedTimer / RAII）
    // ----------------------------------------------------------
    cout << "\n【5】資源管理：自動計時器（ScopedTimer / RAII）" << endl;
    simulateWork();
    // timer 在 simulateWork() 返回時自動解構，印出耗時

    // ----------------------------------------------------------
    // 【6】忘記 delete 的後果：記憶體洩漏
    // ----------------------------------------------------------
    cout << "\n【6】忘記 delete 的後果：記憶體洩漏" << endl;

    cout << "  --- 正確使用 delete ---" << endl;
    Resource* r1 = new Resource(1);
    delete r1;          // 解構函數被調用

    cout << "  --- 忘記 delete（記憶體洩漏！）---" << endl;
    Resource* r2 = new Resource(2);
    // 注意：這裡故意不 delete r2，展示記憶體洩漏
    // 解構函數永遠不會被調用！記憶體永遠不會被釋放！
    (void)r2;           // 消除「未使用變數」警告

    cout << "  --- 局部對象（自動管理）---" << endl;
    {
        Resource r3(3);
        // 不需要 delete，離開作用域自動解構
    }
    cout << "  注意：資源 #2 的解構函數從未被調用 → 記憶體洩漏！" << endl;

    // ----------------------------------------------------------
    // 【7】編譯器自動生成的解構函數
    // ----------------------------------------------------------
    cout << "\n【7】編譯器自動生成的解構函數" << endl;
    cout << "  AutoDestructor：" << endl;
    cout << "    - string name  → 編譯器自動調用 string::~string()" << endl;
    cout << "    - int value    → 基本型別，不做任何事" << endl;
    cout << "  NeedCustomDestructor：" << endl;
    cout << "    - int* data    → 裸指標！編譯器不會 delete！" << endl;
    cout << "    - 必須手動寫解構函數：~NeedCustomDestructor() { delete[] data; }" << endl;
    {
        AutoDestructor ad;
        ad.name = "測試";
        ad.value = 42;
        // 離開作用域時，編譯器自動生成的解構函數會調用 name 的 ~string()
    }
    {
        NeedCustomDestructor ncd(10);
        // 離開作用域時，自定義的解構函數會 delete[] data
    }
    cout << "  兩個對象都已正確清理" << endl;

    // ----------------------------------------------------------
    // 【8】解構函數中不要拋出異常
    // ----------------------------------------------------------
    cout << "\n【8】解構函數中不要拋出異常" << endl;
    {
        SafeCleanup sc("資料庫連接");
        // 離開作用域時，解構函數在 try-catch 中安全清理
    }
    cout << "  正確做法：解構函數內部 try-catch，不讓異常傳播" << endl;
    cout << "  錯誤做法：~Bad() { throw runtime_error(\"...\"); }" << endl;
    cout << "  → 如果在 stack unwinding 中拋出，程式會 terminate()！" << endl;

    // ----------------------------------------------------------
    // 【9】建構函數 vs 解構函數 對照表
    // ----------------------------------------------------------
    cout << "\n【9】建構函數 vs 解構函數 對照表" << endl;
    cout << "  ┌──────────┬────────────────┬────────────────┐" << endl;
    cout << "  │ 特徵      │ 建構函數        │ 解構函數        │" << endl;
    cout << "  ├──────────┼────────────────┼────────────────┤" << endl;
    cout << "  │ 名稱      │ ClassName()     │ ~ClassName()    │" << endl;
    cout << "  │ 返回值    │ 無              │ 無              │" << endl;
    cout << "  │ 參數      │ 可以有(可重載)  │ 不能有(不可重載)│" << endl;
    cout << "  │ 數量      │ 可以有多個      │ 只能有一個      │" << endl;
    cout << "  │ 調用時機  │ 對象創建時      │ 對象銷毀時      │" << endl;
    cout << "  │ 職責      │ 獲取資源/初始化 │ 釋放資源/清理   │" << endl;
    cout << "  └──────────┴────────────────┴────────────────┘" << endl;

    // ----------------------------------------------------------
    // 【10】static 成員追蹤存活物件數量（LifeCycle）
    // ----------------------------------------------------------
    cout << "\n【10】static 成員追蹤存活物件數量（LifeCycle）" << endl;

    LifeCycle a("Alpha");

    {
        LifeCycle b("Beta");
        LifeCycle c("Charlie");

        cout << "\n  --- 區塊內：存活 "
             << LifeCycle::getCount() << " 個 ---\n" << endl;
    }
    // b 和 c 在這裡被解構（Charlie 先解構，Beta 後解構 → LIFO）

    cout << "\n  --- 區塊外：存活 "
         << LifeCycle::getCount() << " 個 ---\n" << endl;

    LifeCycle d("Delta");

    cout << "\n  --- 即將離開 LifeCycle 區段 ---" << endl;
    // 注意：a 和 d 在 main() 結束時才解構

    // ----------------------------------------------------------
    // 【11】綜合範例：Session 管理 DatabaseConnection
    // ----------------------------------------------------------
    cout << "\n【11】綜合範例：Session 管理 DatabaseConnection" << endl;
    cout << "（展示 delete chain：Session 解構 → delete 成員 → 成員解構）\n" << endl;

    cout << "--- 建立工作階段 ---" << endl;
    {
        Session session("UserService",
                        "mysql://localhost:3306/mydb",
                        "redis://localhost:6379");

        cout << "\n--- 執行工作 ---" << endl;
        session.doWork();

        cout << "\n--- 工作完成，即將離開區塊 ---" << endl;
    }
    // session 離開作用域：
    // 1. Session::~Session() 被調用
    // 2. delete cacheDb → DatabaseConnection::~DatabaseConnection() 解構
    // 3. delete mainDb  → DatabaseConnection::~DatabaseConnection() 解構

    cout << "\n--- 所有資源已自動清理 ---" << endl;

    // ----------------------------------------------------------
    // 重點回顧
    // ----------------------------------------------------------
    cout << "\n=============================================" << endl;
    cout << "本課重點回顧：" << endl;
    cout << "  1. 解構函數語法：~ClassName()，無返回值，無參數，不能重載" << endl;
    cout << "  2. 解構順序與建構順序相反（LIFO：後建構先解構）" << endl;
    cout << "  3. 五種調用時機：全域/局部/區塊/函數返回/delete" << endl;
    cout << "  4. RAII 模式：建構獲取資源 → 解構釋放資源（自動管理）" << endl;
    cout << "  5. 忘記 delete → 解構函數不會被調用 → 記憶體洩漏" << endl;
    cout << "  6. 編譯器自動生成的解構函數不會 delete 裸指標" << endl;
    cout << "  7. 解構函數中不要拋出異常（用 try-catch 包住）" << endl;
    cout << "  8. 建構函數可重載多個 vs 解構函數只能有一個" << endl;
    cout << "  9. static 成員可追蹤存活物件數量" << endl;
    cout << "  10. 裸指標成員 → 必須手動寫解構函數 delete" << endl;
    cout << "=============================================" << endl;

    return 0;
}
// main() 結束後：
// - localObj、a、d 在這裡被解構（d 先解構，a 後解構，localObj 最後 → LIFO）
// - globalObj 在所有局部對象解構之後被解構（最晚建構的全域對象最先解構）
