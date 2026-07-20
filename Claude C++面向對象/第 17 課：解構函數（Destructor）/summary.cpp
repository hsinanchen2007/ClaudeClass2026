// =============================================================================
//  第 17 課：解構函數（Destructor）—— 總複習
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  ~ClassName();     // 無回傳型別、無參數、不可重載，一個類別只有一個
//   標準版本：C++98 起即有；C++11 起解構函數**預設為 noexcept**，
//             智慧指標（unique_ptr/shared_ptr）為 C++11、make_unique 為 C++14
//   複雜度：取決於要釋放的資源
//   標頭檔：本檔用到 <string>、<chrono>、<fstream>、<memory>、<vector>
//
// 【詳細解釋 Explanation】
//
// 【1. 解構函數到底是什麼】
//   最直觀的說法是「釋放資源的地方」，但更準確的說法是：
//   **「這個物件的生命週期結束時，一定要做的事」**。
//   資源釋放只是其中最常見的一種；計時結算、日誌配對、狀態還原、
//   交易回復，全都適用同一個機制。
//   它之所以強大，是因為呼叫時機由**編譯器保證**，不是由你記得。
//
// 【2. 五種呼叫時機】
//     (a) 區域物件離開作用域（最常見）
//     (b) 對 new 出來的物件呼叫 delete
//     (c) 臨時物件在完整運算式結束時
//     (d) 全域／static 物件在 main 結束之後
//     (e) 容器元素被移除，或容器本身被銷毀
//   另外還有一種很重要的情形：**拋出例外導致堆疊展開時**，
//   作用域內已建構完成的物件都會被逐一解構。這正是 RAII 相對於
//   手動釋放最大的優勢——手寫的 close()／free() 會被例外直接跳過。
//
// 【3. 解構順序：一律是建構順序的完全反序】
//   ● 區域物件：後建立的先解構（LIFO，因為在堆疊上）
//   ● 類別成員：依宣告順序建構，依宣告的**反序**解構
//   ● 繼承體系：先解構衍生類別（含其成員），最後才解構基底
//   理由都一樣：後建立者可能依賴先建立者，反序拆除才能保證
//   「依賴別人的先走、被依賴的後走」。
//
// 【4. RAII：C++ 最核心的慣用法】
//   Resource Acquisition Is Initialization——建構時取得資源，解構時釋放。
//   把資源的生命週期綁定到物件的生命週期，於是：
//       {
//           LockGuard g(mtx);      // 取得
//           doWork();              // 中間不論 return 或拋例外
//       }                          // 都保證釋放
//   標準函式庫大量採用這個模式：std::string、std::vector、std::fstream、
//   std::lock_guard、std::unique_ptr 全都是 RAII 型別。
//
// 【5. Rule of Three / Five / Zero】
//   ● **Rule of Three**：若你需要自訂「解構函數、複製建構函數、
//     複製指派運算子」其中任何一個，通常三個都需要。
//     因為需要自訂解構，代表這個類別自己管理資源；
//     而編譯器生成的複製只做淺複製，會讓兩個物件持有同一份資源，
//     最後重複釋放。
//   ● **Rule of Five**（C++11）：再加上移動建構與移動指派。
//   ● **Rule of Zero**（最理想）：讓成員自己管理資源
//     （std::vector、std::string、std::unique_ptr），
//     你的類別就一個特殊成員函數都不用寫。
//   實務上的優先順序是 Rule of Zero > Rule of Five > Rule of Three。
//
// 【6. 解構函數的三條硬規則】
//   (1) **不可以讓例外逃出**。C++11 起預設 noexcept，例外逃出直接
//       std::terminate。清理可能失敗時，在裡面攔下並記錄，
//       另外提供明確的 close()／commit() 讓呼叫端在正常流程處理錯誤。
//   (2) **不要手動呼叫**。除了搭配 placement new 的進階場景，
//       手動呼叫之後物件離開作用域還會再解構一次 → 重複解構，未定義行為。
//   (3) **多型刪除必須有 virtual 解構函數**。若會透過基底指標
//       delete 衍生物件，基底的解構函數沒宣告 virtual 就是未定義行為
//       （實務症狀通常是衍生類別的部分沒被清理）。
//
// 【概念補充 Concept Deep Dive】
//
//   ● 編譯器自動生成的解構函數做了什麼
//     它會依反序解構每個成員與基底，但**不會**對裸指標成員呼叫 delete。
//     編譯器無從得知那個指標是「擁有」還是「只是借看」，
//     所以它什麼都不做——這就是裸指標成員必須自己寫解構函數的原因，
//     也是應該改用 unique_ptr 的原因（它明確表達了所有權）。
//
//   ● 「解構函數一定會被呼叫」的前提
//     前提是「正常結束生命週期」。以下情況不會執行解構函數：
//       - std::abort()、std::_Exit()、行程被強制終止
//       - std::exit() 會解構全域／static 物件，但**不會**解構堆疊上的區域物件
//       - 建構函數拋出例外時，該物件的解構函數不會被呼叫
//         （但已初始化完成的成員仍會正常解構）
//
//   ● 洩漏偵測工具的判準不同（本課 6.cpp 有實測）
//     Valgrind 與 LeakSanitizer 對同一份程式碼可能給出不同結論：
//     LeakSanitizer 以**可達性**判斷，若指標值仍殘留在堆疊上就可能不報告。
//     本課 6.cpp 實測：Valgrind 回報 definitely lost，LSan 卻沒報。
//     結論是**工具沒報不等於沒問題**，設計上的正確性才是根本。
//
//   ● 量測經過時間要用 steady_clock
//     std::chrono::steady_clock 保證單調遞增，不受系統校時影響；
//     system_clock 是牆上時間、可能被往回調；
//     high_resolution_clock 在標準中只是別名，實際指向誰是**實作定義**
//     （libstdc++ 上是 system_clock）。本檔的 ScopedTimer 已改用 steady_clock。
//
// 【注意事項 Pay Attention】
//   1. 絕不可讓例外離開解構函數（C++11 起預設 noexcept）。
//   2. 打算多型使用（基底指標刪除衍生物件）時，基底解構函數必須 virtual。
//   3. new 配 delete、new[] 配 delete[]，交叉使用是未定義行為。
//   4. delete 之後指標不會自動變 nullptr，仍是懸空指標。
//   5. 自訂解構函數時，一併考慮複製／移動（Rule of Three／Five）；
//      更好的做法是改用會自我管理的成員，一個都不用寫（Rule of Zero）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】解構函數
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是 RAII？為什麼說它比手動釋放可靠？
//     答：RAII 把資源的生命週期綁定到物件的生命週期——建構時取得，
//         解構時釋放。可靠的原因是語言**保證**物件離開作用域時呼叫解構函數，
//         包含拋出例外導致堆疊展開的情況；手動釋放則必須在每一條離開路徑
//         （每個 return、每個錯誤分支、每個例外）都記得，漏一條就洩漏。
//     追問：RAII 有保證不了的情況嗎？
//         → 有。std::abort()、std::_Exit()、行程被強制終止時解構函數不會執行；
//           std::exit() 也不會解構堆疊上的區域物件。
//
// 🔥 Q2. 什麼是 Rule of Three？C++11 之後有什麼變化？
//     答：只要類別需要自訂解構函數、複製建構函數、複製指派運算子其中之一，
//         通常三個都需要——因為需要自訂解構代表它自己管理資源，
//         而編譯器生成的淺複製會導致重複釋放。C++11 加上移動建構與
//         移動指派成為 Rule of Five；最理想的是 Rule of Zero：
//         讓成員（vector、unique_ptr）自己管理，五個都不用寫。
//     追問：編譯器自動生成的解構函數會 delete 裸指標成員嗎？
//         → 不會。它只解構各成員與基底；裸指標是不是「擁有」該記憶體，
//           編譯器無從得知，所以什麼都不做。
//
// 🔥 Q3. 為什麼透過基底指標 delete 衍生物件時，基底解構函數要宣告 virtual？
//     答：因為 delete 會依**靜態型別**決定呼叫哪個解構函數。基底解構函數
//         若非 virtual，只會呼叫基底的版本，衍生類別新增的成員不會被清理，
//         標準規定此情況為未定義行為。宣告 virtual 之後，
//         delete 會透過虛擬表下派到實際型別的解構函數，再逐層往上解構。
//     追問：那是不是所有類別都該加 virtual 解構函數？
//         → 不是。virtual 會讓物件多出虛擬表指標、且失去 trivially
//           destructible 的性質。只有「打算被當成多型基底使用」的類別才需要。
//
// ⚠️ 陷阱 1. 解構函數裡可以拋出例外嗎？反正我會在外面 catch。
//     答：不可以。C++11 起解構函數預設 noexcept，例外逃出會直接
//         std::terminate，外面的 catch 根本沒機會執行。
//         更根本的原因是：解構可能發生在堆疊展開的過程中，
//         此時若再拋出第二個例外，語言無法決定該傳播哪一個，只能終止程式。
//     為什麼會錯：把解構函數當成一般函數，以為錯誤可以照常往外拋。
//         正確做法是解構函數只做「盡力而為」的清理並記錄錯誤，
//         另外提供明確的 close()／commit() 讓呼叫端在正常流程中處理。
//
// ⚠️ 陷阱 2. 忘記 delete 沒關係，程式結束時作業系統會全部回收。
//     答：這對短命的程式勉強成立，但三個情況會出事：
//         長時間執行的服務根本不會結束、記憶體以外的資源
//         （檔案描述元、連線、鎖）有限且可能由遠端持有、
//         以及解構函數裡該做的事（存檔、送通知、更新統計）全部被跳過。
//     為什麼會錯：把「洩漏」只理解成「記憶體沒還」。
//         實際上洩漏的是「該執行的清理邏輯沒有執行」，那可能包含資料。
//
// ⚠️ 陷阱 3. 我 delete p; 之後有檢查 if (p != nullptr) 才使用，這樣安全嗎？
//     答：不安全。delete 之後 p 仍保有原本的位址，不會自動變成 nullptr，
//         那個檢查一定會通過，接著就解參考了已歸還的記憶體——未定義行為。
//     為什麼會錯：以為 delete 會清理指標變數本身。delete 處理的是指標
//         **指向的物件**，指標變數只是個存位址的普通變數。
//         這也是應該改用智慧指標的理由：它把所有權與指標狀態一起管好。
// ═══════════════════════════════════════════════════════════════════════════
//
// VERIFY_STDOUT_ONLY
//   ↑ 此標記告訴驗證工具：本檔的預期輸出只涵蓋 stdout。
//     ScopedTimer 量到的耗時數字每次執行都不同，因此寫到 stderr，
//     不列為固定預期輸出（作法與本課 5.cpp 一致）。

/*
 * ================================================================
 * 【第 17 課：解構函數（Destructor）】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -Wall -Wextra -o summary summary.cpp
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
#include <memory>
#include <vector>
#include <cstdio>     // std::remove（清理示範產生的檔案）
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
    // 量測經過時間要用 steady_clock（單調遞增，不受系統校時影響）。
    // high_resolution_clock 只是別名，實際指向誰是實作定義，不適合量測區間。
    chrono::steady_clock::time_point startTime;

public:
    // 用初始化列表一步到位（第 16 課的作法），不在函數體內賦值
    explicit ScopedTimer(const string& name)
        : taskName(name)
        , startTime(chrono::steady_clock::now())
    {
        cout << "  [計時開始] " << taskName << endl;
    }

    ~ScopedTimer() {
        auto endTime = chrono::steady_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(
            endTime - startTime
        ).count();
        // 流程訊息 → stdout（可重現）
        cout << "  [計時結束] " << taskName << "（耗時數字見 stderr）" << endl;
        // 量測數字 → stderr（每次執行都不同，屬診斷資訊，不污染正式輸出）
        cerr << "  [計時] " << taskName << " 耗時: " << duration << " ms" << endl;
    }
};

void simulateWork() {
    ScopedTimer timer("模擬工作");

    // 模擬一些耗時操作
    // 用 long long：累加到約 5×10^13，int 會溢位（signed overflow 是未定義行為）
    volatile long long sum = 0;  // volatile 防止編譯器優化掉
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

// ================================================================
// 【LeetCode 實戰範例】LeetCode 707. Design Linked List
// ================================================================
// 題目：自行設計單向鏈結串列，支援 get / addAtHead / addAtTail /
//       addAtIndex / deleteAtIndex。
//
// 為什麼用到本主題：
//   這是少數「非寫解構函數不可」的經典題型。每個節點都是 new 出來的，
//   若沒有解構函數逐一走訪並 delete，整條串列就是一整串洩漏。
//   而且它同時逼你面對 Rule of Three：
//     ● 自訂了解構函數 → 編譯器生成的淺複製會讓兩份物件共用同一批節點
//       → 兩者解構時重複釋放（未定義行為）
//     ● 本範例的處理方式是**明確停用複製**（= delete），
//       讓任何複製嘗試在編譯期就被擋下，而不是留下執行期地雷
//   解構函數用**迴圈**而非遞迴，是因為長串列遞迴會把呼叫堆疊用光。
//
// 複雜度：get/addAtIndex/deleteAtIndex 為 O(index)，addAtHead 為 O(1)，
//         addAtTail 為 O(n)（本實作未維護 tail 指標）；解構為 O(n)。
class MyLinkedList {
private:
    struct Node {
        int val;
        Node* next;
        Node(int v, Node* n = nullptr) : val(v), next(n) { }
    };

    Node* head_ = nullptr;   // NSDMI：保證起始為空串列，不是不定值
    int   size_ = 0;

public:
    MyLinkedList() = default;

    // 解構函數：逐一釋放所有節點。用迴圈而非遞迴，避免長串列爆堆疊。
    ~MyLinkedList() {
        Node* cur = head_;
        while (cur) {
            Node* next = cur->next;   // 先存起來，delete 之後就讀不到了
            delete cur;
            cur = next;
        }
        head_ = nullptr;
        size_ = 0;
    }

    // Rule of Three：自訂了解構函數，複製就必須處理。
    // 這裡選擇明確停用，讓錯誤在編譯期就被擋下。
    MyLinkedList(const MyLinkedList&) = delete;
    MyLinkedList& operator=(const MyLinkedList&) = delete;

    int get(int index) const {
        if (index < 0 || index >= size_) return -1;
        const Node* cur = head_;
        for (int i = 0; i < index; ++i) cur = cur->next;
        return cur->val;
    }

    void addAtHead(int val) {
        head_ = new Node(val, head_);
        ++size_;
    }

    void addAtTail(int val) { addAtIndex(size_, val); }

    void addAtIndex(int index, int val) {
        if (index > size_) return;          // 超過長度就不插入
        if (index <= 0) { addAtHead(val); return; }

        Node* prev = head_;
        for (int i = 0; i < index - 1; ++i) prev = prev->next;
        prev->next = new Node(val, prev->next);
        ++size_;
    }

    void deleteAtIndex(int index) {
        if (index < 0 || index >= size_) return;

        Node* victim = nullptr;
        if (index == 0) {
            victim = head_;
            head_ = head_->next;
        } else {
            Node* prev = head_;
            for (int i = 0; i < index - 1; ++i) prev = prev->next;
            victim = prev->next;
            prev->next = victim->next;
        }
        delete victim;                       // 唯一擁有者，負責釋放
        --size_;
    }

    void print(const string& tag) const {
        cout << "  " << tag << " [";
        for (const Node* cur = head_; cur; cur = cur->next) {
            if (cur != head_) cout << ", ";
            cout << cur->val;
        }
        cout << "]  (size=" << size_ << ")" << endl;
    }
};

// ================================================================
// 【日常實務範例】背景工作佇列：解構函數負責「排空並收尾」
// ================================================================
// 情境：服務關閉時，背景佇列裡可能還有沒處理完的工作。
//       如果解構函數只是把記憶體釋放掉，這些工作就無聲無息地消失了——
//       洩漏的不是記憶體，而是「該做的事沒做」。
// 重點：
//   ● 解構函數不只釋放資源，還要完成「這個物件消失前必須做完的事」
//   ● 解構函數**絕不可拋出例外**，所以用 try/catch 把每一項包起來，
//     單一工作失敗不能拖垮整個收尾流程
//   ● 提供明確的 flush()，讓呼叫端可以在正常流程中處理錯誤；
//     解構函數只是最後的安全網
class TaskQueue {
private:
    string name_;
    vector<string> pending_;
    bool flushed_ = false;

public:
    explicit TaskQueue(const string& name) : name_(name) { }

    void submit(const string& task) { pending_.push_back(task); }

    // 明確的收尾入口：呼叫端可在正常流程中檢查結果
    void flush() {
        cout << "    [" << name_ << "] 開始排空 " << pending_.size() << " 筆工作" << endl;
        for (const auto& t : pending_) {
            cout << "      處理: " << t << endl;
        }
        pending_.clear();
        flushed_ = true;
    }

    // 解構函數：最後的安全網。絕不拋出例外。
    ~TaskQueue() {
        if (flushed_) {
            cout << "    [" << name_ << "] 已正常排空，解構無事可做" << endl;
            return;
        }
        try {
            cout << "    [" << name_ << "] 未呼叫 flush()，解構時緊急排空 "
                 << pending_.size() << " 筆" << endl;
            for (const auto& t : pending_) {
                cout << "      緊急處理: " << t << endl;
            }
        } catch (...) {
            // 解構函數中絕不可讓例外逃出（C++11 起預設 noexcept）
            // 這裡只能盡力而為，不能往外拋
        }
    }
};

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
    // 【11】LeetCode 707. Design Linked List（非寫解構函數不可）
    // ----------------------------------------------------------
    cout << "\n【11】LeetCode 707. Design Linked List" << endl;
    cout << "  每個節點都是 new 出來的 → 解構函數必須逐一 delete" << endl;
    {
        MyLinkedList list;
        list.addAtHead(1);
        list.addAtTail(3);
        list.addAtIndex(1, 2);      // 變成 1 → 2 → 3
        list.print("插入後:");
        cout << "  get(1) = " << list.get(1) << endl;
        list.deleteAtIndex(1);      // 變成 1 → 3
        list.print("刪除後:");
        cout << "  get(1) = " << list.get(1) << endl;
        cout << "  get(9) = " << list.get(9) << "  （超出範圍回 -1）" << endl;
        // MyLinkedList copy = list;   // 編譯錯誤：複製已被 = delete 停用
        cout << "  --- 離開作用域，解構函數逐一釋放所有節點 ---" << endl;
    }

    // ----------------------------------------------------------
    // 【12】日常實務：背景工作佇列（解構函數負責收尾）
    // ----------------------------------------------------------
    cout << "\n【12】日常實務：背景工作佇列（解構不只釋放，還要收尾）" << endl;
    cout << "  情況 A：正常呼叫 flush()" << endl;
    {
        TaskQueue q("寄信佇列");
        q.submit("送出訂單確認信 #1001");
        q.submit("送出訂單確認信 #1002");
        q.flush();
    }
    cout << "  情況 B：忘記呼叫 flush()，靠解構函數當安全網" << endl;
    {
        TaskQueue q("稽核佇列");
        q.submit("寫入稽核紀錄 #A");
        q.submit("寫入稽核紀錄 #B");
        // 沒有呼叫 flush()
    }

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

    // 清理本次示範產生的檔案，讓重複執行的結果一致、也不留下垃圾
    std::remove("test_output.txt");

    return 0;
}
// main() 結束後：
// - localObj、a、d 在這裡被解構（d 先解構，a 後解構，localObj 最後 → LIFO）
// - globalObj 在所有局部對象解構之後被解構（最晚建構的全域對象最先解構）

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary
// 只看流程（隱藏耗時診斷）:  ./summary 2>/dev/null
//
// ※ 重要說明（放在預期輸出標記之前）：
//   1. 下方預期輸出**只涵蓋 stdout**。ScopedTimer 量到的毫秒數會寫到
//      **stderr**，因為那個數字每次執行、每台機器都不同，不能列為固定預期值。
//   2. 本程式會在目前工作目錄暫時建立 test_output.txt，並在結束前刪除，
//      因此重複執行的輸出一致、也不會留下檔案。
//   3. 本檔以 -Wall -Wextra 編譯無任何警告，且不含未定義行為。

// === 預期輸出 ===
//   [建構] 全域物件
// =============================================
//    第 17 課：解構函數（Destructor）總複習
// =============================================
//
// 【1】解構函數基本語法 & 解構順序（LIFO）
// 建構順序: A → B，解構順序: B → A（後進先出）
//   [建構] 物件A 被創建了
//   [建構] 物件B 被創建了
//   --- 即將離開區塊 ---
//   [解構] 物件B 被銷毀了
//   [解構] 物件A 被銷毀了
//
// 【2】解構函數的調用時機
// （全域物件已在 main() 之前建構，見最上方輸出）
//
//   --- 局部對象 ---
//   [建構] 局部物件
//
//   --- 進入區塊 ---
//   [建構] 區塊物件
//   --- 離開區塊 ---
//   [解構] 區塊物件
//   --- 區塊已結束 ---
//
//   --- 進入 testFunction ---
//   [建構] 函數局部物件
//   --- 離開 testFunction ---
//   [解構] 函數局部物件
//
//   --- 動態對象 ---
//   [建構] 動態物件
//   --- 手動 delete ---
//   [解構] 動態物件
//
// 【3】資源管理：動態記憶體（DynamicArray）
//   [建構] 分配了 5 個 int 的記憶體
//   [10, 20, 30, 0, 0]
//   --- 即將離開區塊 ---
//   [解構] 釋放了 5 個 int 的記憶體
//   --- 記憶體已自動釋放 ---
//
// 【4】資源管理：檔案資源（FileWriter）
//   [建構] 打開檔案: test_output.txt
//   寫入: 第一行
//   寫入: 第二行
//   寫入: 第三行
//   --- 即將離開區塊 ---
//   [解構] 關閉檔案: test_output.txt
//   --- 檔案已自動關閉 ---
//
// 【5】資源管理：自動計時器（ScopedTimer / RAII）
//   [計時開始] 模擬工作
//   計算完成
//   [計時結束] 模擬工作（耗時數字見 stderr）
//
// 【6】忘記 delete 的後果：記憶體洩漏
//   --- 正確使用 delete ---
//   [建構] 資源 #1
//   [解構] 資源 #1
//   --- 忘記 delete（記憶體洩漏！）---
//   [建構] 資源 #2
//   --- 局部對象（自動管理）---
//   [建構] 資源 #3
//   [解構] 資源 #3
//   注意：資源 #2 的解構函數從未被調用 → 記憶體洩漏！
//
// 【7】編譯器自動生成的解構函數
//   AutoDestructor：
//     - string name  → 編譯器自動調用 string::~string()
//     - int value    → 基本型別，不做任何事
//   NeedCustomDestructor：
//     - int* data    → 裸指標！編譯器不會 delete！
//     - 必須手動寫解構函數：~NeedCustomDestructor() { delete[] data; }
//   兩個對象都已正確清理
//
// 【8】解構函數中不要拋出異常
//   [建構] SafeCleanup: 資料庫連接
//   [解構] SafeCleanup: 資料庫連接 安全清理
//   正確做法：解構函數內部 try-catch，不讓異常傳播
//   錯誤做法：~Bad() { throw runtime_error("..."); }
//   → 如果在 stack unwinding 中拋出，程式會 terminate()！
//
// 【9】建構函數 vs 解構函數 對照表
//   ┌──────────┬────────────────┬────────────────┐
//   │ 特徵      │ 建構函數        │ 解構函數        │
//   ├──────────┼────────────────┼────────────────┤
//   │ 名稱      │ ClassName()     │ ~ClassName()    │
//   │ 返回值    │ 無              │ 無              │
//   │ 參數      │ 可以有(可重載)  │ 不能有(不可重載)│
//   │ 數量      │ 可以有多個      │ 只能有一個      │
//   │ 調用時機  │ 對象創建時      │ 對象銷毀時      │
//   │ 職責      │ 獲取資源/初始化 │ 釋放資源/清理   │
//   └──────────┴────────────────┴────────────────┘
//
// 【10】static 成員追蹤存活物件數量（LifeCycle）
//   [建構] Alpha (目前存活: 1 個)
//   [建構] Beta (目前存活: 2 個)
//   [建構] Charlie (目前存活: 3 個)
//
//   --- 區塊內：存活 3 個 ---
//
//   [解構] Charlie (目前存活: 2 個)
//   [解構] Beta (目前存活: 1 個)
//
//   --- 區塊外：存活 1 個 ---
//
//   [建構] Delta (目前存活: 2 個)
//
//   --- 即將離開 LifeCycle 區段 ---
//
// 【11】綜合範例：Session 管理 DatabaseConnection
// （展示 delete chain：Session 解構 → delete 成員 → 成員解構）
//
// --- 建立工作階段 ---
//   [連接 #1] 已建立: mysql://localhost:3306/mydb
//   [連接 #2] 已建立: redis://localhost:6379
//   [Session] UserService 啟動
//
// --- 執行工作 ---
//   [連接 #1] 執行: SELECT * FROM users
//   [連接 #2] 執行: GET user:1001
//   [連接 #1] 執行: UPDATE users SET active = 1
//
// --- 工作完成，即將離開區塊 ---
//   [Session] UserService 關閉中...
//   [連接 #2] 已斷開: redis://localhost:6379
//   [連接 #1] 已斷開: mysql://localhost:3306/mydb
//   [Session] UserService 已完全關閉
//
// --- 所有資源已自動清理 ---
//
// 【11】LeetCode 707. Design Linked List
//   每個節點都是 new 出來的 → 解構函數必須逐一 delete
//   插入後: [1, 2, 3]  (size=3)
//   get(1) = 2
//   刪除後: [1, 3]  (size=2)
//   get(1) = 3
//   get(9) = -1  （超出範圍回 -1）
//   --- 離開作用域，解構函數逐一釋放所有節點 ---
//
// 【12】日常實務：背景工作佇列（解構不只釋放，還要收尾）
//   情況 A：正常呼叫 flush()
//     [寄信佇列] 開始排空 2 筆工作
//       處理: 送出訂單確認信 #1001
//       處理: 送出訂單確認信 #1002
//     [寄信佇列] 已正常排空，解構無事可做
//   情況 B：忘記呼叫 flush()，靠解構函數當安全網
//     [稽核佇列] 未呼叫 flush()，解構時緊急排空 2 筆
//       緊急處理: 寫入稽核紀錄 #A
//       緊急處理: 寫入稽核紀錄 #B
//
// =============================================
// 本課重點回顧：
//   1. 解構函數語法：~ClassName()，無返回值，無參數，不能重載
//   2. 解構順序與建構順序相反（LIFO：後建構先解構）
//   3. 五種調用時機：全域/局部/區塊/函數返回/delete
//   4. RAII 模式：建構獲取資源 → 解構釋放資源（自動管理）
//   5. 忘記 delete → 解構函數不會被調用 → 記憶體洩漏
//   6. 編譯器自動生成的解構函數不會 delete 裸指標
//   7. 解構函數中不要拋出異常（用 try-catch 包住）
//   8. 建構函數可重載多個 vs 解構函數只能有一個
//   9. static 成員可追蹤存活物件數量
//   10. 裸指標成員 → 必須手動寫解構函數 delete
// =============================================
//   [解構] Delta (目前存活: 1 個)
//   [解構] Alpha (目前存活: 0 個)
//   [解構] 局部物件
//   [解構] 全域物件
