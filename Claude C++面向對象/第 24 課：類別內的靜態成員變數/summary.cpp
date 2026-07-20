// =============================================================================
//  summary.cpp  —  第 24 課總複習：類別內的靜態成員變數
// =============================================================================
//
// 【主題資訊 Information】
//   語法（傳統，C++98）：
//     class C { static int count; };     // 類別內：宣告
//     int C::count = 0;                  // 類別外：定義（必須有，否則連結失敗）
//   語法（C++17 起）：
//     class C { inline static int count = 0; };   // 就地定義，不必再寫類別外那行
//     class C { static constexpr int kMax = 100; }; // constexpr static 隱含 inline
//   標頭檔：本檔僅需 <iostream>、<string>、<vector>
//   複雜度：存取 O(1)（就是一個全域變數的存取）
//
// 【詳細解釋 Explanation】
//
// 【1. 靜態成員到底是什麼】
//   一句話：它是一個「被放進類別命名空間、並受類別存取控制保護的全域變數」。
//   它不屬於任何物件實例 —— 所有實例共用同一份，
//   而且即使一個物件都沒有建立，它依然存在。
//   這解釋了本課幾乎所有的特性：
//     * 為什麼可以用 C::count 存取（不需要物件）；
//     * 為什麼 sizeof(物件) 不包含它（它不在物件裡）；
//     * 為什麼它的生命週期是靜態儲存期（程式開始到結束）；
//     * 為什麼靜態成員函式沒有 this（它不隸屬於任何實例）。
//
// 【2. 為什麼 C++17 之前必須在類別外「再寫一次」】
//   類別定義通常放在標頭檔，會被多個 .cpp 引入。
//   若類別內的 `static int count;` 就直接配置儲存空間，
//   每個引入該標頭的 translation unit 都會產生一份定義 → 重複定義錯誤。
//   所以標準把它拆成兩件事：
//       類別內 = 宣告（告訴編譯器有這個東西，但不配置空間）
//       類別外 = 定義（真正配置那唯一一份儲存空間，只能出現在一個 .cpp）
//   忘了寫類別外那一行，編譯會過（宣告足夠通過編譯），
//   但連結時會出現 undefined reference —— 這是初學者最常見的連結錯誤之一。
//   C++17 的 inline static 解決了這個麻煩：inline 允許多個 TU 出現相同定義，
//   由連結器合併成一份，因此可以直接就地初始化，不必再寫類別外那行。
//
// 【3. sizeof 為什麼不變】
//   本檔第六節實測：有靜態成員的類別與沒有的類別，sizeof 完全相同。
//   因為靜態成員存放在靜態儲存區（.data/.bss），不在物件的記憶體佈局裡。
//   建立一百萬個物件，靜態成員也還是只有一份。
//   （順帶一提：空類別的 sizeof 不會是 0 而是 1，
//    因為標準要求不同物件必須有不同位址。）
//
// 【4. constexpr static 與 inline static 的差別】
//   * `static constexpr int kMax = 100;`
//     編譯期常量，可用於陣列大小、模板參數、static_assert。
//     C++17 起 constexpr static 資料成員隱含就是 inline，
//     所以同樣不需要類別外定義。
//   * `inline static int counter = 0;`
//     一般變數，可修改，只是免去了類別外定義。
//   選擇原則：值在編譯期就固定且不會改 → constexpr static；
//   需要在執行期變動（計數器、快取）→ inline static。
//
// 【概念補充 Concept Deep Dive】
//   靜態成員與存取控制是「正交」的兩件事：
//   private static 仍然是 private —— 外界不能存取，
//   但類別自己的成員函式（含靜態成員函式）可以。
//   這正是「用 private static 計數器 + public static getter」
//   這個常見組合的基礎：資料受保護，只開放唯讀查詢。
//
//   初始化順序需要特別當心：類別的靜態成員屬於靜態儲存期，
//   跨 translation unit 的初始化順序是「未指定」的
//   （static initialization order fiasco）。
//   若某個 TU 的靜態成員在初始化時依賴另一個 TU 的靜態成員，
//   可能讀到尚未初始化的值。
//   標準解法是把它包進函式內的 static 區域變數（Meyers Singleton），
//   讓初始化延後到第一次使用，順序因而確定，
//   而且 C++11 起還附帶執行緒安全保證。
//
//   多執行緒下另有一點：靜態成員的 ++／-- 並非原子操作。
//   本課的計數器範例都是單執行緒；
//   真要跨執行緒統計必須改用 std::atomic<int>，否則就是資料競爭。
//
// 【注意事項 Pay Attention】
// 1. C++17 之前，類別外的定義不可省略，否則連結期 undefined reference。
//    C++17 起可用 inline static 就地定義解決。
// 2. 靜態成員不佔物件空間，sizeof 不會因為它而變大。
// 3. 靜態成員函式沒有 this，因此不能存取非靜態成員，也不能標 const。
// 4. private static 仍受存取控制保護；「靜態」與「公開」是兩回事。
// 5. 跨 TU 的靜態初始化順序未指定，不可互相依賴。
// 6. 靜態成員的遞增／遞減在多執行緒下非原子操作，需要 std::atomic。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】靜態成員變數
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 C++17 之前，static 成員變數一定要在類別外再定義一次？
//     答：因為類別定義通常在標頭檔，會被多個 .cpp 引入。
//         若類別內就直接配置儲存空間，每個 translation unit 都會產生
//         一份定義，造成重複定義。所以標準把它拆成
//         「類別內宣告（不配置空間）」與「類別外定義（配置唯一那一份）」。
//         漏寫類別外那行時，編譯會通過，但連結期會出現 undefined reference。
//     追問：C++17 的 inline static 為什麼就可以就地初始化？
//         → inline 的語意正是「允許多個 TU 出現相同定義，由連結器合併成一份」。
//           因此不再有重複定義問題。附帶一提，
//           constexpr static 資料成員在 C++17 起隱含就是 inline。
//
// 🔥 Q2. 類別加了一個 static 成員，sizeof 會變大嗎？
//     答：不會。靜態成員存放在靜態儲存區，不屬於物件的記憶體佈局，
//         所有實例共用同一份。本檔第六節有實測：
//         有 static 成員與沒有的類別，sizeof 完全相同（本機皆為 8 bytes）。
//     追問：那 static 成員函式呢？會影響 sizeof 嗎？
//         → 不會，任何成員函式都不會影響 sizeof（函式不存在物件裡）。
//           唯一會讓物件變大的是「虛擬函式」——
//           那會加入 vptr，通常使 sizeof 增加一個指標的大小。
//
// ⚠️ 陷阱. 「static 成員是所有物件共用的，
//           所以在多執行緒環境下拿它當計數器很方便，也很安全。」
//     答：共用是對的，安全是錯的。`++count;` 看起來是一行，
//         實際上是「讀取 → 加一 → 寫回」三個步驟。
//         兩個執行緒同時執行時可能都讀到同一個舊值，
//         各自加一後寫回，結果只增加了 1 —— 更新遺失。
//         這在標準中屬於資料競爭（data race），是未定義行為，
//         不是「偶爾算錯」而已。
//     為什麼會錯：把「所有執行緒看到同一個變數」誤讀成
//         「對它的操作是原子的」。共用只保證大家看的是同一份資料，
//         完全不保證存取被正確同步。
//         正解是改用 std::atomic<int>（輕量、無鎖）
//         或以 std::mutex 保護。本課所有計數器範例都是單執行緒，
//         因此未做同步。
// ═══════════════════════════════════════════════════════════════════════════
//
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔不附
//   理由：本課主題是「靜態成員的儲存位置、定義規則與生命週期」，
//   屬於語言的物件模型與連結期規則。LeetCode 評測的是演算法輸入輸出，
//   題目不會因為某個成員是不是 static 而有不同答案
//   （事實上在 LeetCode 上使用 static 成員反而危險 ——
//    評測平台可能在同一個行程內連續跑多個測資，
//    上一筆測資留下的 static 狀態會污染下一筆，這是常見的 WA 來源）。
//   依規格「寧缺勿濫」從缺——本檔第八節的遊戲實體管理系統
//   本身就是靜態計數器最典型的實務用途。
// -----------------------------------------------------------------------------

/*
 * ============================================================================
 *  第 24 課：類別內的靜態成員變數 —— 完整複習總結
 * ============================================================================
 *
 *  本檔案整合了第 24 課所有 8 個範例檔案的核心概念，
 *  涵蓋靜態成員變數的定義、初始化、訪問方式、生命週期、
 *  訪問控制、sizeof 特性、constexpr static，以及綜合實戰範例。
 *  閱讀本檔案即可完整複習本課所有知識點。
 *
 *  目錄：
 *    第一節：靜態成員變數的基本概念與傳統語法（對應檔案 1）
 *    第二節：C++17 inline static 簡化語法（對應檔案 2）
 *    第三節：靜態成員的兩種訪問方式（對應檔案 3）
 *    第四節：靜態成員的生命週期（對應檔案 4）
 *    第五節：靜態成員與訪問控制（private/public）（對應檔案 5）
 *    第六節：sizeof 與靜態成員——不佔對象空間（對應檔案 6）
 *    第七節：constexpr static——編譯期常量（對應檔案 7）
 *    第八節：綜合範例——遊戲實體管理系統（對應檔案 8）
 *    附錄：普通成員 vs 靜態成員 對比總結表
 *
 * ============================================================================
 */

#include <iostream>
#include <string>
using namespace std;

/* ============================================================================
 *  第一節：靜態成員變數的基本概念與傳統語法
 * ============================================================================
 *
 *  核心觀念：
 *    - 普通成員變數：屬於「單一對象」，每個對象各有一份
 *    - 靜態成員變數：屬於「整個類別」，所有對象共享同一份
 *
 *  傳統語法（C++11 之前）：
 *    1. 在類別內「聲明」（declaration）：   static int totalCount_;
 *    2. 在類別外「定義並初始化」（definition）：int Soldier::totalCount_ = 0;
 *
 *  為什麼需要類別外定義？
 *    - 類別內的 static int totalCount_; 只是告訴編譯器「這個變數存在」
 *    - 類別外的 int Soldier::totalCount_ = 0; 才是在記憶體中真正分配空間
 *    - 靜態成員不住在棧上的對象裡面，它住在「靜態/全域存儲區」
 *    - 如果只有聲明沒有定義，會導致連結錯誤（linker error）：
 *      undefined reference to `Soldier::totalCount_'
 *
 *  記憶體佈局示意：
 *    棧（Stack）：存放 s1, s2 等對象（各有自己的 name_, id_）
 *    靜態/全域區：存放 totalCount_, nextId_（不屬於任何對象，所有對象共享）
 *
 *  常見用途：
 *    - 對象計數器：追蹤目前有多少個對象存活
 *    - 唯一 ID 產生器：每個新對象自動獲得遞增 ID
 */

// --- 範例：士兵管理（傳統語法：類別內聲明 + 類別外定義）---
class Soldier {
private:
    string name_;       // 普通成員：每個士兵各有一份
    int id_;            // 普通成員：每個士兵各有一份

    // 靜態成員變數的「聲明」——只告訴編譯器它存在
    static int totalCount_;     // 追蹤所有士兵的總數
    static int nextId_;         // 下一個可用的 ID（自動遞增）

public:
    // 建構函數：每創建一個士兵，totalCount_ 加 1，id_ 自動分配
    Soldier(const string& name)
        : name_(name), id_(nextId_++)   // nextId_++ 先用再加，確保每人 ID 不同
    {
        totalCount_++;                  // 全體共享的計數器加 1
        cout << "  [入伍] " << name_ << " (ID:" << id_
             << " 總人數:" << totalCount_ << ")" << endl;
    }

    // 解構函數：士兵退役時，totalCount_ 減 1
    ~Soldier() {
        totalCount_--;
        cout << "  [退役] " << name_ << " (ID:" << id_
             << " 總人數:" << totalCount_ << ")" << endl;
    }

    void report() const {
        cout << "  士兵 " << name_ << " (ID:" << id_ << ") 報到！" << endl;
    }

    // 靜態成員可以通過普通成員函數訪問
    void showTotal() const {
        cout << "  目前總人數：" << totalCount_ << endl;
    }
};

// 靜態成員變數的「定義與初始化」——在類別外，真正分配記憶體空間
// 注意語法：類型 類別名::變數名 = 初始值;
int Soldier::totalCount_ = 0;      // 初始時沒有士兵
int Soldier::nextId_ = 1001;       // ID 從 1001 開始

// --- 測試函數 ---
void demo_section1() {
    cout << "\n==============================" << endl;
    cout << "  第一節：靜態成員變數基礎" << endl;
    cout << "==============================" << endl;

    cout << "\n--- 創建士兵 ---" << endl;
    Soldier s1("阿強");     // ID:1001, 總人數:1
    Soldier s2("阿明");     // ID:1002, 總人數:2
    Soldier s3("阿華");     // ID:1003, 總人數:3

    cout << "\n--- 報到 ---" << endl;
    s1.report();
    s2.report();
    s3.report();
    s1.showTotal();         // 顯示：目前總人數：3

    cout << "\n--- 作用域結束，逆序解構 ---" << endl;
    // s3、s2、s1 依次解構（LIFO 順序），totalCount_ 逐步遞減
}


/* ============================================================================
 *  第二節：C++17 inline static 簡化語法
 * ============================================================================
 *
 *  C++17 引入了 inline 關鍵字用於靜態成員，可以直接在類別內定義並初始化，
 *  不再需要在類別外額外定義。這是目前推薦的寫法。
 *
 *  三種初始化方式比較：
 *
 *    方式 1（C++11 之前）：
 *      類別內：  static int count_;
 *      類別外：  int MyClass::count_ = 0;
 *
 *    方式 2（C++11）：
 *      類別內：  static const int MAX = 100;    // 只限 const 整數型別
 *      不需要類別外定義
 *
 *    方式 3（C++17，推薦）：
 *      類別內：  inline static int count_ = 0;  // 任何類型都行
 *      不需要類別外定義
 *
 *  const static 整數型別的特例：
 *    - 即使是 C++11，static const int 也可以在類別內初始化
 *    - 但非整數型別（如 string、double）需要 C++17 的 inline static const
 */

// --- 範例：遊戲配置（C++17 inline static）---
class GameConfig {
public:
    // C++17 inline static：直接在類別內定義並初始化，無需類別外定義
    inline static int maxPlayers = 4;           // 可修改的靜態變數
    inline static double gravity = 9.8;         // 可修改的靜態變數
    inline static string version = "1.0.3";     // 可修改的靜態變數

    // const static 整數型別：即使 C++11 也可以在類別內初始化
    static const int MAX_LEVEL = 100;           // 不可修改的整數常量
    static const int MAX_ITEMS = 999;           // 不可修改的整數常量

    // C++17 inline static const：可用於非整數型別（如 string）
    inline static const string GAME_NAME = "冒險世界";  // 不可修改的字串常量

    // 靜態函數可以直接訪問靜態成員（不需要對象）
    static void printConfig() {
        cout << "  遊戲：" << GAME_NAME << " v" << version << endl;
        cout << "  最大玩家數：" << maxPlayers << endl;
        cout << "  重力：" << gravity << endl;
        cout << "  最大等級：" << MAX_LEVEL << endl;
        cout << "  最大物品：" << MAX_ITEMS << endl;
    }
};

// 不需要類別外定義了！（C++17 的好處）

void demo_section2() {
    cout << "\n==============================" << endl;
    cout << "  第二節：C++17 inline static" << endl;
    cout << "==============================" << endl;

    // 直接通過類別名調用靜態函數
    GameConfig::printConfig();

    // 可以修改非 const 的靜態成員
    cout << "\n--- 修改配置 ---" << endl;
    GameConfig::maxPlayers = 8;        // 合法：非 const
    GameConfig::gravity = 15.0;        // 合法：非 const
    GameConfig::version = "2.0.0";     // 合法：非 const
    GameConfig::printConfig();

    // GameConfig::MAX_LEVEL = 200;    // 編譯錯誤！const 不可修改
    // GameConfig::GAME_NAME = "新名";  // 編譯錯誤！const 不可修改
}


/* ============================================================================
 *  第三節：靜態成員的兩種訪問方式
 * ============================================================================
 *
 *  方式 1（推薦）：通過類別名訪問  ->  Enemy::totalKilled
 *    - 清楚表明這是屬於類別的共享數據
 *
 *  方式 2（不推薦）：通過對象訪問   ->  e1.totalKilled
 *    - 語法合法但容易誤導，看起來像是對象自己的數據
 *    - 實際上 e1.totalKilled 和 e2.totalKilled 是完全同一個變數
 *
 *  重要提醒：無論用哪種方式訪問，都是同一個變數，修改一處全部改變。
 */

// --- 範例：敵人擊殺統計（兩種訪問方式對比）---
class Enemy {
public:
    // inline static 公開成員：可從外部直接訪問
    inline static int totalKilled = 0;      // 被擊殺的總數
    inline static int totalSpawned = 0;     // 生成的總數

private:
    string name_;
    bool alive_;

public:
    Enemy(const string& name) : name_(name), alive_(true) {
        totalSpawned++;     // 每生成一個敵人，計數加 1
    }

    void kill() {
        if (alive_) {           // 防止重複擊殺
            alive_ = false;
            totalKilled++;      // 擊殺計數加 1
            cout << "  " << name_ << " 被擊殺！" << endl;
        }
    }

    const string& getName() const { return name_; }
};

void demo_section3() {
    cout << "\n==============================" << endl;
    cout << "  第三節：靜態成員的訪問方式" << endl;
    cout << "==============================" << endl;

    // 重置靜態成員（因為前面的範例可能已經改變了值）
    Enemy::totalKilled = 0;
    Enemy::totalSpawned = 0;

    Enemy e1("哥布林");
    Enemy e2("骷髏兵");
    Enemy e3("狼人");

    e1.kill();
    e3.kill();

    // 方式 1：通過類別名訪問（推薦）——清楚表明是類別共享的數據
    cout << "\n--- 方式 1：通過類別名訪問（推薦）---" << endl;
    cout << "  生成數：" << Enemy::totalSpawned << endl;    // 3
    cout << "  擊殺數：" << Enemy::totalKilled << endl;     // 2

    // 方式 2：通過對象訪問（不推薦）——容易誤導
    cout << "\n--- 方式 2：通過對象訪問（不推薦）---" << endl;
    cout << "  生成數：" << e1.totalSpawned << endl;        // 3（同一個變數）
    cout << "  擊殺數：" << e2.totalKilled << endl;         // 2（同一個變數）

    // 驗證：無論通過哪個對象訪問，都是同一個值
    cout << "\n--- 驗證：同一個變數 ---" << endl;
    cout << "  e1.totalKilled = " << e1.totalKilled << endl;    // 2
    cout << "  e2.totalKilled = " << e2.totalKilled << endl;    // 2
    cout << "  e3.totalKilled = " << e3.totalKilled << endl;    // 2
    cout << "  都是同一個值！" << endl;
}


/* ============================================================================
 *  第四節：靜態成員的生命週期
 * ============================================================================
 *
 *  靜態成員的生命週期與全域變數相同：
 *    - 程式啟動時初始化（在 main() 之前）
 *    - 程式結束時銷毀（在 main() 之後）
 *    - 即使所有對象都被銷毀了，靜態成員仍然存活
 *
 *  時間線：
 *    程式啟動 -> 靜態成員初始化
 *                  |
 *            main() 開始
 *                  |
 *            創建 obj -> 普通成員初始化
 *                  |
 *            obj 銷毀 -> 普通成員解構
 *                  |
 *            main() 結束
 *                  |
 *            靜態成員解構
 *    程式結束
 *
 *  關鍵區別：
 *    - 普通成員：隨對象創建而生，隨對象銷毀而亡
 *    - 靜態成員：隨程式啟動而生，隨程式結束而亡，壽命超過任何單一對象
 */

// --- 輔助類別：追蹤建構/解構時機 ---
class Tracker {
private:
    string label_;

public:
    Tracker(const string& label) : label_(label) {
        cout << "  [建構] " << label_ << endl;
    }
    ~Tracker() {
        cout << "  [解構] " << label_ << endl;
    }

    void ping() const {
        cout << "  " << label_ << " 存活中" << endl;
    }
};

// --- 範例：靜態成員 vs 普通成員的生命週期對比 ---
class LifecycleDemo {
public:
    // 靜態成員：程式開始時初始化，程式結束時才銷毀
    inline static Tracker staticTracker{"靜態成員 Tracker"};

    // 普通成員：隨對象創建/銷毀
    Tracker memberTracker;

    LifecycleDemo(const string& name) : memberTracker("普通成員 " + name) {
        cout << "  [建構] LifecycleDemo " << name << endl;
    }

    ~LifecycleDemo() {
        cout << "  [解構] LifecycleDemo" << endl;
    }
};

void demo_section4() {
    cout << "\n==============================" << endl;
    cout << "  第四節：靜態成員的生命週期" << endl;
    cout << "==============================" << endl;
    cout << "(靜態成員已在 main 之前初始化)" << endl;

    cout << "\n--- 創建對象 ---" << endl;
    {
        LifecycleDemo obj("測試");

        cout << "\n--- 使用中 ---" << endl;
        LifecycleDemo::staticTracker.ping();   // 靜態成員存活
        obj.memberTracker.ping();               // 普通成員存活

        cout << "\n--- 作用域結束 ---" << endl;
    }
    // obj 已銷毀（普通成員也被解構），但靜態成員還活著

    cout << "\n--- obj 已銷毀，靜態成員仍在 ---" << endl;
    LifecycleDemo::staticTracker.ping();       // 靜態成員依然存活！
}


/* ============================================================================
 *  第五節：靜態成員與訪問控制（private / public）
 * ============================================================================
 *
 *  靜態成員同樣受 public / private / protected 訪問控制：
 *    - private 靜態成員：外部不能直接訪問，必須通過公開的靜態函數（getter/setter）
 *    - public 靜態成員：外部可以直接訪問
 *
 *  最佳實踐：
 *    - 將靜態成員設為 private（封裝保護）
 *    - 提供 public 的靜態 getter/setter 函數來控制訪問
 *    - 在 setter 中加入驗證邏輯，防止非法修改
 *
 *  靜態成員函數的特點（詳見第 25 課）：
 *    - 不需要對象就能調用：BankAccount::getAccountCount()
 *    - 沒有 this 指標
 *    - 只能直接訪問靜態成員，不能訪問普通成員
 */

// --- 範例：銀行帳戶（私有靜態成員 + 公開靜態 getter/setter）---
class BankAccount {
private:
    string owner_;       // 普通成員：帳戶持有人
    double balance_;     // 普通成員：帳戶餘額

    // 私有靜態成員：外部不能直接訪問
    inline static double totalDeposits_ = 0.0;  // 銀行總存款
    inline static int accountCount_ = 0;         // 帳戶總數
    inline static double interestRate_ = 0.03;   // 共享利率（3%）

public:
    BankAccount(const string& owner, double initial)
        : owner_(owner), balance_(initial > 0 ? initial : 0)
    {
        accountCount_++;                // 帳戶數加 1
        totalDeposits_ += balance_;     // 總存款增加
        cout << "  [開戶] " << owner_ << " 存入 " << balance_ << endl;
    }

    void deposit(double amount) {
        if (amount <= 0) return;
        balance_ += amount;
        totalDeposits_ += amount;
    }

    double getBalance() const { return balance_; }
    const string& getOwner() const { return owner_; }

    // 計算利息——使用共享的靜態利率
    // 這展示了普通成員函數如何同時訪問普通成員和靜態成員
    double calculateInterest() const {
        return balance_ * interestRate_;    // 個人餘額 * 共享利率
    }

    // 公開靜態 getter：安全地讀取私有靜態數據
    static int getAccountCount() { return accountCount_; }
    static double getTotalDeposits() { return totalDeposits_; }
    static double getInterestRate() { return interestRate_; }

    // 公開靜態 setter（帶驗證）：安全地修改私有靜態數據
    static void setInterestRate(double rate) {
        if (rate < 0 || rate > 0.20) {      // 驗證利率範圍
            cout << "  利率必須在 0%~20% 之間！" << endl;
            return;
        }
        interestRate_ = rate;
        cout << "  利率已調整為 " << (rate * 100) << "%" << endl;
    }

    void printStatement() const {
        cout << "  " << owner_ << "：餘額 " << balance_
             << "，利息 " << calculateInterest() << endl;
    }
};

void demo_section5() {
    cout << "\n==============================" << endl;
    cout << "  第五節：靜態成員與訪問控制" << endl;
    cout << "==============================" << endl;

    cout << "\n--- 開戶 ---" << endl;
    BankAccount a1("陳信安", 10000);
    BankAccount a2("王小明", 5000);
    BankAccount a3("李大華", 20000);

    // 通過公開的靜態函數訪問私有靜態數據
    cout << "\n--- 銀行統計 ---" << endl;
    cout << "  帳戶數：" << BankAccount::getAccountCount() << endl;       // 3
    cout << "  總存款：" << BankAccount::getTotalDeposits() << endl;      // 35000
    cout << "  當前利率：" << (BankAccount::getInterestRate() * 100) << "%" << endl; // 3%

    // 計算各帳戶利息（利率 3%）
    cout << "\n--- 利息計算（利率 3%）---" << endl;
    a1.printStatement();    // 10000 * 0.03 = 300
    a2.printStatement();    // 5000 * 0.03 = 150
    a3.printStatement();    // 20000 * 0.03 = 600

    // 調整利率——影響所有帳戶（因為利率是共享的靜態成員）
    cout << "\n--- 調整利率 ---" << endl;
    BankAccount::setInterestRate(0.05);     // 合法：5%

    cout << "\n--- 利息計算（利率 5%）---" << endl;
    a1.printStatement();    // 10000 * 0.05 = 500
    a2.printStatement();    // 5000 * 0.05 = 250
    a3.printStatement();    // 20000 * 0.05 = 1000

    // 非法利率——被 setter 中的驗證攔截
    BankAccount::setInterestRate(0.50);     // 被攔截！超過 20%

    // BankAccount::totalDeposits_ = 0;     // 編譯錯誤！private 不可直接訪問
}


/* ============================================================================
 *  第六節：sizeof 與靜態成員——不佔對象空間
 * ============================================================================
 *
 *  靜態成員不計入 sizeof(對象) 的大小：
 *    - 靜態成員存放在「靜態/全域存儲區」，不在對象內部
 *    - 無論有多少個靜態成員，對象大小都不會增加
 *    - 這是因為靜態成員屬於類別，不屬於對象
 *
 *  這意味著：
 *    - 使用靜態成員不會增加每個對象的記憶體開銷
 *    - 適合用於需要所有對象共享的數據（如配置、計數器等）
 */

// --- 範例：對比有無靜態成員的類別大小 ---
class WithStatic {
    int a_;                             // 4 bytes
    int b_;                             // 4 bytes
    inline static int shared_ = 0;      // 不計入 sizeof（存在靜態區）
    inline static double config_ = 0;   // 不計入 sizeof（存在靜態區）
};

class WithoutStatic {
    int a_;     // 4 bytes
    int b_;     // 4 bytes
};

void demo_section6() {
    cout << "\n==============================" << endl;
    cout << "  第六節：sizeof 與靜態成員" << endl;
    cout << "==============================" << endl;

    cout << "  WithStatic 大小：" << sizeof(WithStatic) << " bytes" << endl;
    cout << "  WithoutStatic 大小：" << sizeof(WithoutStatic) << " bytes" << endl;
    cout << "  兩者相同！靜態成員不佔對象空間。" << endl;
    // 兩者都是 8 bytes（兩個 int），靜態成員的空間不算在對象裡
}


/* ============================================================================
 *  第七節：constexpr static——編譯期常量
 * ============================================================================
 *
 *  constexpr static 在編譯期就確定了值，是最高效的常量定義方式。
 *
 *  C++17 以後，constexpr static 成員變數隱含 inline，不需要在類別外定義。
 *
 *  優點：
 *    - 編譯期計算，零執行期開銷
 *    - 可以用在需要編譯期常量的地方（如陣列大小、模板參數）
 *    - 其他 constexpr 常量可以基於已有的 constexpr 常量計算
 *
 *  三種靜態常量的比較：
 *
 *    static const int X = 10;
 *      -> 可以類別內初始化（僅限整數型別）
 *      -> 執行期常量
 *
 *    inline static const string S = "hello";
 *      -> C++17，任何型別
 *      -> 執行期常量
 *
 *    static constexpr double PI = 3.14;
 *      -> 編譯期常量（最高效）
 *      -> 可以用在陣列大小、模板參數等需要編譯期值的地方
 */

// --- 範例：數學常數類別（constexpr static）---
class MathConstants {
public:
    // constexpr static：編譯期常量
    static constexpr double PI = 3.14159265358979;
    static constexpr double E  = 2.71828182845905;
    static constexpr int    MAX_DIMENSION = 3;

    // 編譯期計算：基於已有的 constexpr 常量推導新常量
    static constexpr double TWO_PI = PI * 2.0;         // 編譯期計算完成
    static constexpr double PI_SQUARED = PI * PI;       // 編譯期計算完成

    // 靜態函數使用 constexpr 常量
    static double circleArea(double r) {
        return PI * r * r;
    }

    static double sphereVolume(double r) {
        return (4.0 / 3.0) * PI * r * r * r;
    }
};

// constexpr static 不需要類別外定義（C++17）
// 下面這行如果寫了反而會導致 multiple definition 錯誤：
// constexpr double MathConstants::PI;  // 不需要！

void demo_section7() {
    cout << "\n==============================" << endl;
    cout << "  第七節：constexpr static" << endl;
    cout << "==============================" << endl;

    cout << "  PI = " << MathConstants::PI << endl;
    cout << "  E  = " << MathConstants::E << endl;
    cout << "  2*PI = " << MathConstants::TWO_PI << endl;
    cout << "  PI^2 = " << MathConstants::PI_SQUARED << endl;

    cout << "\n  圓面積(r=5)：" << MathConstants::circleArea(5) << endl;
    cout << "  球體積(r=3)：" << MathConstants::sphereVolume(3) << endl;

    // constexpr 可以用在編譯期需要常量的地方（如陣列大小）
    int arr[MathConstants::MAX_DIMENSION] = {1, 2, 3};
    cout << "\n  陣列大小：" << sizeof(arr) / sizeof(arr[0]) << endl;
}


/* ============================================================================
 *  第八節：綜合範例——遊戲實體管理系統
 * ============================================================================
 *
 *  本節整合了前面所有概念，展示一個完整的實體管理系統：
 *    - inline static 成員變數：nextId_, activeCount_, totalCreated_, totalDestroyed_
 *    - constexpr static 常量：MAX_ENTITIES
 *    - 建構/解構函數中更新靜態計數器
 *    - 靜態查詢函數：printStatistics()
 *    - 對象的啟用/停用影響活躍計數
 *    - 棧對象離開作用域自動解構 vs 堆對象手動 delete
 */

// --- 範例：遊戲實體管理系統 ---
class Entity {
private:
    string name_;           // 實體名稱
    string type_;           // 實體類型（玩家、NPC、敵人、陷阱...）
    int id_;                // 唯一 ID（由靜態 nextId_ 自動分配）
    bool active_;           // 是否處於活躍狀態

    // ====== 靜態成員：類別級別的管理數據 ======
    inline static int nextId_ = 1;              // 下一個可用 ID
    inline static int activeCount_ = 0;          // 目前活躍的實體數量
    inline static int totalCreated_ = 0;         // 總共創建過的實體數量
    inline static int totalDestroyed_ = 0;       // 總共銷毀過的實體數量

    // 靜態常量：最大實體數量上限
    static constexpr int MAX_ENTITIES = 100;

public:
    Entity(const string& name, const string& type)
        : name_(name), type_(type), id_(nextId_++), active_(true)
    {
        totalCreated_++;
        activeCount_++;

        // 超過上限時發出警告（利用 constexpr 常量進行比較）
        if (activeCount_ > MAX_ENTITIES) {
            cout << "  警告：實體數量超過上限 " << MAX_ENTITIES << "！" << endl;
        }

        cout << "  [+] " << type_ << " \"" << name_ << "\" (ID:"
             << id_ << ") 已創建 [活躍:" << activeCount_ << "]" << endl;
    }

    ~Entity() {
        if (active_) {          // 只有活躍的實體銷毀時才減少活躍計數
            activeCount_--;
        }
        totalDestroyed_++;
        cout << "  [-] " << type_ << " \"" << name_ << "\" (ID:"
             << id_ << ") 已銷毀 [活躍:" << activeCount_ << "]" << endl;
    }

    // 停用實體（不銷毀對象，但不計入活躍數）
    void deactivate() {
        if (active_) {
            active_ = false;
            activeCount_--;
            cout << "  [x] " << name_ << " 已停用 [活躍:"
                 << activeCount_ << "]" << endl;
        }
    }

    // 重新啟用實體
    void activate() {
        if (!active_) {
            active_ = true;
            activeCount_++;
            cout << "  [o] " << name_ << " 已重新啟用 [活躍:"
                 << activeCount_ << "]" << endl;
        }
    }

    // Getter
    const string& getName() const { return name_; }
    int getId() const { return id_; }
    bool isActive() const { return active_; }

    // ====== 靜態查詢函數 ======
    static int getActiveCount() { return activeCount_; }
    static int getTotalCreated() { return totalCreated_; }
    static int getTotalDestroyed() { return totalDestroyed_; }
    static constexpr int getMaxEntities() { return MAX_ENTITIES; }

    // 打印全局統計信息
    static void printStatistics() {
        cout << "  +--------------------------+" << endl;
        cout << "  | 實體管理統計              |" << endl;
        cout << "  | 已創建：" << totalCreated_ << endl;
        cout << "  | 已銷毀：" << totalDestroyed_ << endl;
        cout << "  | 活躍中：" << activeCount_ << endl;
        cout << "  | 上限：  " << MAX_ENTITIES << endl;
        cout << "  +--------------------------+" << endl;
    }
};

void demo_section8() {
    cout << "\n==============================" << endl;
    cout << "  第八節：綜合範例" << endl;
    cout << "==============================" << endl;

    // 初始統計
    cout << "\n=== 初始狀態 ===" << endl;
    Entity::printStatistics();

    // 創建堆上的實體（需要手動 delete）
    cout << "\n=== 創建實體 ===" << endl;
    Entity* player = new Entity("勇者", "玩家");
    Entity* npc = new Entity("村長", "NPC");

    {
        // 創建棧上的實體（離開作用域時自動銷毀）
        cout << "\n=== 進入戰鬥區域 ===" << endl;
        Entity enemy1("哥布林", "敵人");
        Entity enemy2("骷髏兵", "敵人");
        Entity trap("地刺陷阱", "陷阱");

        Entity::printStatistics();      // 5 個活躍實體

        // 停用陷阱（觸發後不再活躍，但對象仍然存在）
        cout << "\n--- 陷阱觸發 ---" << endl;
        trap.deactivate();              // activeCount_ 減 1
        Entity::printStatistics();      // 4 個活躍實體

        cout << "\n--- 離開戰鬥區域 ---" << endl;
    }
    // enemy1, enemy2, trap 的解構函數被自動調用
    // 注意：trap 已停用，解構時不會再減 activeCount_

    cout << "\n=== 回到安全區 ===" << endl;
    Entity::printStatistics();          // 2 個活躍實體（player, npc）

    // 手動清理堆上的對象
    cout << "\n=== 清理 ===" << endl;
    delete npc;
    delete player;

    cout << "\n=== 最終統計 ===" << endl;
    Entity::printStatistics();          // 0 個活躍實體，5 個已創建，5 個已銷毀
}


/* ============================================================================
 *  附錄：普通成員 vs 靜態成員 對比總結表
 * ============================================================================
 *
 *  +----------------+------------------+------------------+
 *  | 特性           | 普通成員變數      | 靜態成員變數      |
 *  +----------------+------------------+------------------+
 *  | 歸屬           | 屬於對象          | 屬於類別          |
 *  | 份數           | 每個對象一份      | 整個類別一份      |
 *  | 存儲位置       | 對象內部（棧/堆） | 靜態存儲區        |
 *  | 計入 sizeof    | 是               | 否               |
 *  | 生命週期       | 與對象相同        | 與程式相同        |
 *  | 訪問方式       | obj.member       | Class::member    |
 *  | 需要對象才能用 | 是               | 否               |
 *  | this 指標      | 通過 this 訪問   | 沒有 this        |
 *  | 類別外定義     | 不需要           | 需要（或 inline） |
 *  +----------------+------------------+------------------+
 *
 *  靜態成員變數的常見用途：
 *    1. 對象計數器：static int count_;  追蹤目前有多少個對象存活
 *    2. 唯一 ID 產生器：static int nextId_;  每個新對象自動獲得遞增 ID
 *    3. 全域配置：static double gravity_;  所有物理對象共用的重力值
 *    4. 共享常量：static const int MAX_HP = 9999;
 *    5. 快取/共享資源：static map<string, Texture> textureCache_;
 *
 * ============================================================================
 */


// ============================================================================
//  主函數：依序執行所有範例
// ============================================================================
int main() {
    cout << "============================================" << endl;
    cout << "  第 24 課：類別內的靜態成員變數" << endl;
    cout << "  完整複習總結" << endl;
    cout << "============================================" << endl;

    demo_section1();    // 靜態成員變數基礎（傳統語法）
    demo_section2();    // C++17 inline static
    demo_section3();    // 兩種訪問方式
    demo_section4();    // 生命週期
    demo_section5();    // 訪問控制
    demo_section6();    // sizeof 與靜態成員
    demo_section7();    // constexpr static
    demo_section8();    // 綜合範例

    cout << "\n============================================" << endl;
    cout << "  全部範例執行完畢！" << endl;
    cout << "============================================" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary
//   （本檔用到 C++17 的 inline static 與 constexpr static 隱含 inline，
//     必須以 -std=c++17 以上編譯）

// ── 輸出但書 ────────────────────────────────────────────────────────────
// 1. 本檔輸出完全由語言規則與程式邏輯決定，逐位元組可重現
//    （實測連跑 5 次 md5 相同）。
// 2. 第六節的 sizeof 實測值 8 bytes 屬實作定義
//    （本機 GCC 15.2.0 / libstdc++ / x86-64；WithStatic 的非靜態成員為
//     兩個 int，4 + 4 = 8；它的 static int 與 static double 都不計入）。
//    重點不在「8」這個數字，而在「有無 static 成員的兩個類別 sizeof 相同」
//    —— 那才是標準保證的性質：靜態成員不佔物件空間。
// 3. 本課所有計數器範例皆為單執行緒。靜態成員的 ++／-- 不是原子操作，
//    多執行緒下直接這樣用屬於資料競爭（未定義行為），
//    需改用 std::atomic<int> 或以 mutex 保護。
// 4. 本檔沒有跨 translation unit 的靜態物件，因此不涉及
//    static initialization order fiasco；真實專案有多個 .cpp 時，
//    跨 TU 的初始化順序是未指定的，不可互相依賴。
// 5. 本機環境：GCC 15.2.0 (Ubuntu 15.2.0-16ubuntu1) / libstdc++ / x86-64。

// === 預期輸出 ===
//   [建構] 靜態成員 Tracker
// ============================================
//   第 24 課：類別內的靜態成員變數
//   完整複習總結
// ============================================
//
// ==============================
//   第一節：靜態成員變數基礎
// ==============================
//
// --- 創建士兵 ---
//   [入伍] 阿強 (ID:1001 總人數:1)
//   [入伍] 阿明 (ID:1002 總人數:2)
//   [入伍] 阿華 (ID:1003 總人數:3)
//
// --- 報到 ---
//   士兵 阿強 (ID:1001) 報到！
//   士兵 阿明 (ID:1002) 報到！
//   士兵 阿華 (ID:1003) 報到！
//   目前總人數：3
//
// --- 作用域結束，逆序解構 ---
//   [退役] 阿華 (ID:1003 總人數:2)
//   [退役] 阿明 (ID:1002 總人數:1)
//   [退役] 阿強 (ID:1001 總人數:0)
//
// ==============================
//   第二節：C++17 inline static
// ==============================
//   遊戲：冒險世界 v1.0.3
//   最大玩家數：4
//   重力：9.8
//   最大等級：100
//   最大物品：999
//
// --- 修改配置 ---
//   遊戲：冒險世界 v2.0.0
//   最大玩家數：8
//   重力：15
//   最大等級：100
//   最大物品：999
//
// ==============================
//   第三節：靜態成員的訪問方式
// ==============================
//   哥布林 被擊殺！
//   狼人 被擊殺！
//
// --- 方式 1：通過類別名訪問（推薦）---
//   生成數：3
//   擊殺數：2
//
// --- 方式 2：通過對象訪問（不推薦）---
//   生成數：3
//   擊殺數：2
//
// --- 驗證：同一個變數 ---
//   e1.totalKilled = 2
//   e2.totalKilled = 2
//   e3.totalKilled = 2
//   都是同一個值！
//
// ==============================
//   第四節：靜態成員的生命週期
// ==============================
// (靜態成員已在 main 之前初始化)
//
// --- 創建對象 ---
//   [建構] 普通成員 測試
//   [建構] LifecycleDemo 測試
//
// --- 使用中 ---
//   靜態成員 Tracker 存活中
//   普通成員 測試 存活中
//
// --- 作用域結束 ---
//   [解構] LifecycleDemo
//   [解構] 普通成員 測試
//
// --- obj 已銷毀，靜態成員仍在 ---
//   靜態成員 Tracker 存活中
//
// ==============================
//   第五節：靜態成員與訪問控制
// ==============================
//
// --- 開戶 ---
//   [開戶] 陳信安 存入 10000
//   [開戶] 王小明 存入 5000
//   [開戶] 李大華 存入 20000
//
// --- 銀行統計 ---
//   帳戶數：3
//   總存款：35000
//   當前利率：3%
//
// --- 利息計算（利率 3%）---
//   陳信安：餘額 10000，利息 300
//   王小明：餘額 5000，利息 150
//   李大華：餘額 20000，利息 600
//
// --- 調整利率 ---
//   利率已調整為 5%
//
// --- 利息計算（利率 5%）---
//   陳信安：餘額 10000，利息 500
//   王小明：餘額 5000，利息 250
//   李大華：餘額 20000，利息 1000
//   利率必須在 0%~20% 之間！
//
// ==============================
//   第六節：sizeof 與靜態成員
// ==============================
//   WithStatic 大小：8 bytes
//   WithoutStatic 大小：8 bytes
//   兩者相同！靜態成員不佔對象空間。
//
// ==============================
//   第七節：constexpr static
// ==============================
//   PI = 3.14159
//   E  = 2.71828
//   2*PI = 6.28319
//   PI^2 = 9.8696
//
//   圓面積(r=5)：78.5398
//   球體積(r=3)：113.097
//
//   陣列大小：3
//
// ==============================
//   第八節：綜合範例
// ==============================
//
// === 初始狀態 ===
//   +--------------------------+
//   | 實體管理統計              |
//   | 已創建：0
//   | 已銷毀：0
//   | 活躍中：0
//   | 上限：  100
//   +--------------------------+
//
// === 創建實體 ===
//   [+] 玩家 "勇者" (ID:1) 已創建 [活躍:1]
//   [+] NPC "村長" (ID:2) 已創建 [活躍:2]
//
// === 進入戰鬥區域 ===
//   [+] 敵人 "哥布林" (ID:3) 已創建 [活躍:3]
//   [+] 敵人 "骷髏兵" (ID:4) 已創建 [活躍:4]
//   [+] 陷阱 "地刺陷阱" (ID:5) 已創建 [活躍:5]
//   +--------------------------+
//   | 實體管理統計              |
//   | 已創建：5
//   | 已銷毀：0
//   | 活躍中：5
//   | 上限：  100
//   +--------------------------+
//
// --- 陷阱觸發 ---
//   [x] 地刺陷阱 已停用 [活躍:4]
//   +--------------------------+
//   | 實體管理統計              |
//   | 已創建：5
//   | 已銷毀：0
//   | 活躍中：4
//   | 上限：  100
//   +--------------------------+
//
// --- 離開戰鬥區域 ---
//   [-] 陷阱 "地刺陷阱" (ID:5) 已銷毀 [活躍:4]
//   [-] 敵人 "骷髏兵" (ID:4) 已銷毀 [活躍:3]
//   [-] 敵人 "哥布林" (ID:3) 已銷毀 [活躍:2]
//
// === 回到安全區 ===
//   +--------------------------+
//   | 實體管理統計              |
//   | 已創建：5
//   | 已銷毀：3
//   | 活躍中：2
//   | 上限：  100
//   +--------------------------+
//
// === 清理 ===
//   [-] NPC "村長" (ID:2) 已銷毀 [活躍:1]
//   [-] 玩家 "勇者" (ID:1) 已銷毀 [活躍:0]
//
// === 最終統計 ===
//   +--------------------------+
//   | 實體管理統計              |
//   | 已創建：5
//   | 已銷毀：5
//   | 活躍中：0
//   | 上限：  100
//   +--------------------------+
//
// ============================================
//   全部範例執行完畢！
// ============================================
//   [解構] 靜態成員 Tracker
