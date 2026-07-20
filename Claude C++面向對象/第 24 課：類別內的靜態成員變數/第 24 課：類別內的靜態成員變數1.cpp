// =============================================================================
//  第 24 課：類別內的靜態成員變數 1  —  宣告與定義分離、物件計數與 ID 產生器
// =============================================================================
//
// 【主題資訊 Information】
//   宣告(類別內)： static int totalCount_;
//   定義(類別外)： int Soldier::totalCount_ = 0;       // C++98 起的傳統寫法
//   C++17 新寫法 ： inline static int totalCount_ = 0;  // 類別內一次搞定
//   標準版本      ： static 成員 C++98;inline 變數(含 inline static)C++17
//   標頭檔        ： 無(語言特性)
//   儲存期        ： static storage duration —— 程式啟動前配置,程式結束才釋放
//
// 【詳細解釋 Explanation】
//
// 【1. static 成員變數到底屬於誰】
//   一般成員變數(name_、id_)每個物件各有一份,存在物件自己的記憶體裡。
//   static 成員變數則是「整個類別只有一份」,不存在任何物件裡,
//   而是像全域變數一樣放在程式的靜態資料區。
//   所以本檔三個 Soldier 物件共用同一個 totalCount_ 與 nextId_——
//   任何一個建構子把 totalCount_ 加一,其他物件「看到的」也跟著變。
//   換個角度說:static 成員描述的是「這個類別的整體狀態」,
//   而不是「某一個物件的狀態」。
//
// 【2. 為什麼一定要在類別外再寫一次定義(C++17 以前)】
//   類別定義通常放在標頭檔,而標頭檔會被多個 .cpp 引入。
//   如果類別內的那行就直接配置記憶體,每個引入的 .cpp 都會配置一份,
//   連結時就會出現 multiple definition 錯誤。
//   所以 C++ 把它拆成兩件事:
//       類別內  static int totalCount_;        // 宣告:告訴編譯器「有這個東西」
//       類別外  int Soldier::totalCount_ = 0;  // 定義:真正配置記憶體,只能出現一次
//   定義必須放在某一個 .cpp(不能放標頭檔),這正是初學者最常撞牆的
//   「undefined reference to `Soldier::totalCount_`」連結錯誤的來源。
//   C++17 的 inline static 讓連結器自行合併重複定義,
//   從此可以在類別內一次寫完(見本課檔案 2)。
//
// 【3. 這個檔案示範的兩種典型用途】
//   (a) 物件計數 totalCount_:建構子 ++、解構子 --,隨時反映「現在活著幾個」。
//   (b) 唯一 ID 產生器 nextId_:建構子用 nextId_++ 取號,只增不減。
//   注意兩者的差異——totalCount_ 在解構時要減回去,nextId_ 絕對不能減。
//   如果 nextId_ 也跟著減,那麼「建立 A、建立 B、銷毀 B、建立 C」
//   就會讓 C 拿到跟 B 一樣的 ID,唯一性立刻被破壞。
//
// 【4. 初始化順序:成員初始化列表依「宣告順序」執行】
//   建構子寫成 : name_(name), id_(nextId_++)。
//   這裡剛好與宣告順序(name_、id_)一致,所以沒問題。
//   但要記住規則:成員的初始化順序 只 由類別中的宣告順序決定,
//   與初始化列表裡寫的先後 完全無關。
//   若把列表寫成 id_(nextId_++), name_(name),編譯器仍照宣告順序做,
//   而且會發出 -Wreorder 警告。當某個成員的初值依賴另一個成員時,這個規則就會咬人。
//
// 【概念補充 Concept Deep Dive】
//   (A) static 成員不佔物件空間。
//       sizeof(Soldier) 只算 name_ 與 id_,totalCount_ / nextId_ 完全不計入
//       (本課檔案 6 有直接量測)。這也解釋了為什麼 static 成員不能用
//       成員初始化列表初始化——它根本不屬於任何物件的建構過程。
//
//   (B) 靜態成員的初始化時機。
//       本檔兩個都是整數且以常數初始化,屬於「常數初始化」(constant initialization),
//       在程式載入時就已經有值,不需要執行任何程式碼。
//       若靜態成員的初值需要呼叫函式(例如 static Logger g_log{"app.log"};),
//       就變成「動態初始化」,會在 main() 之前執行——
//       而跨編譯單元之間的動態初始化順序是 未指定 的,
//       這就是著名的 static initialization order fiasco(見檔案 4 與 summary)。
//
//   (C) 解構順序為什麼是反的。
//       s1、s2、s3 在同一個區塊內依序建立,離開區塊時以相反順序銷毀
//       (阿華 → 阿明 → 阿強)。這是標準保證的:自動儲存期物件的解構順序
//       與建構順序相反,因為後建立的可能依賴先建立的。
//       輸出中的總人數 2 → 1 → 0 正是這個順序的直接證據。
//
//   (D) 這個計數器不是執行緒安全的。
//       totalCount_++ 是「讀取 → 加一 → 寫回」三個步驟,不是不可分割的原子操作。
//       多執行緒同時建立 Soldier 會遺失更新,而且屬於 data race(UB)。
//       正式做法是改用 static std::atomic<int> totalCount_{0};
//       此時 ++ 才是原子的。ID 產生器更要注意——重號會造成真正的邏輯錯誤。
//
// 【注意事項 Pay Attention】
//   1. C++17 以前,類別外的定義漏寫 = 連結錯誤(undefined reference),
//      而且錯誤出現在連結階段而非編譯階段,對初學者相當難查。
//   2. 類別外定義 不要 再寫 static 關鍵字:應寫 int Soldier::totalCount_ = 0;
//      加上 static 的語意完全不同(那是在宣告一個檔案內部連結的變數)。
//   3. 計數用的 static 成員要記得在解構子減回去;ID 產生器則絕對不能減。
//   4. 多執行緒環境請改用 std::atomic,否則計數會少、ID 會重複。
//   5. 成員初始化順序只看宣告順序,不看初始化列表的書寫順序(-Wreorder 會提醒)。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】靜態成員變數的宣告與定義
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 static 成員變數要在類別外再定義一次?不寫會怎樣?
//     答：類別內那行只是宣告,告訴編譯器有這個名字與型別,不配置記憶體。
//         因為類別定義通常在標頭檔、會被多個 .cpp 引入,若當場配置就會重複定義。
//         真正的定義必須出現在恰好一個 .cpp。漏寫的話編譯會過,
//         但連結階段報 undefined reference to `Soldier::totalCount_`。
//     追問：C++17 之後呢?→ 可以寫 inline static int totalCount_ = 0;
//         inline 允許同一個定義出現在多個編譯單元,由連結器合併成一份,
//         從此不必再寫類別外定義。
//
// 🔥 Q2. 物件計數器要在解構子減一,那 ID 產生器 nextId_ 也要減嗎?
//     答：絕對不行。totalCount_ 表達「現在有幾個」,是可增可減的即時數量;
//         nextId_ 表達「下一個沒用過的號碼」,必須單調遞增。
//         若解構時也減,「建 A、建 B、殺 B、建 C」會讓 C 拿到 B 用過的 ID,
//         唯一性就破了——而且這種 bug 通常要等到資料對不起來才會被發現。
//     追問：那 ID 會不會用完?→ int 會溢位(有號整數溢位本身就是 UB)。
//         長壽服務應改用 unsigned long long,或改發 UUID。
//
// ⚠️ 陷阱. 「static 成員也是成員,所以可以寫進建構子的成員初始化列表。」
//     答：不行,編譯錯誤。成員初始化列表初始化的是「這個物件的成員」,
//         而 static 成員不屬於任何物件——它在第一個物件誕生之前就已經存在了。
//         要給初值只能在類別外定義處,或用 C++17 的 inline static 在類別內給。
//     為什麼會錯：把「宣告在類別裡」等同於「是物件的一部分」。
//         實際上 static 成員只是「名字被關在類別的作用域裡」,
//         它的儲存位置與生命週期跟全域變數一樣,與物件完全脫鉤。
//         sizeof(Soldier) 不含它,正是最直接的證據。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
using namespace std;

class Soldier {
private:
    string name_;
    int id_;

    // ====== 靜態成員變數：聲明 ======
    // 靜態成員變數屬於類別本身，而不是任何特定的對象
    // 用於跟蹤所有士兵的總數和分配唯一 ID
    static int totalCount_;     // 所有士兵的總數
    static int nextId_;         // 下一個可用的 ID

public:
    Soldier(const string& name)
        : name_(name), id_(nextId_++)
    {
        totalCount_++;
        cout << "  [入伍] " << name_ << " (ID:" << id_
             << " 總人數:" << totalCount_ << ")" << endl;
    }

    ~Soldier() {
        totalCount_--;
        cout << "  [退役] " << name_ << " (ID:" << id_
             << " 總人數:" << totalCount_ << ")" << endl;
    }

    void report() const {
        cout << "  士兵 " << name_ << " (ID:" << id_ << ") 報到！" << endl;
    }

    // 靜態成員可以通過普通成員函數訪問
    // 顯示目前總人數
    void showTotal() const {
        cout << "  目前總人數：" << totalCount_ << endl;
    }
};

// ====== 靜態成員變數：定義與初始化（類別外）======
// 靜態成員變數必須在類別外定義和初始化
// 這裡我們將 totalCount_ 初始化為 0，nextId_ 初始化為 1001
// 注意：這裡的初始化是針對整個類別的，而不是任何特定的對象
int Soldier::totalCount_ = 0;
int Soldier::nextId_ = 1001;

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔從缺,理由如下
//   static 成員的核心是「同一個類別的所有物件共享一份資料」。
//   LeetCode 的設計題(146、155、705、707、1603、1656)在判題時只會建立
//   一個物件並對它下指令,所以「共享 vs 各自一份」的差異在那些題目上
//   根本觀察不到——用 static 或用一般成員都能 AC。
//   更關鍵的是:多筆測資之間若用 static 保存狀態,反而會互相污染而 WA,
//   也就是說 LeetCode 環境其實是在「懲罰」static 的使用。
//   本課的 summary.cpp 會用 LeetCode 1603. Design Parking System 專門說明
//   「為什麼那題的容量 不該 是 static」,此處不重複。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】連線工作階段的三種 static 統計:即時數、流水號、尖峰水位
//   情境：一個後端服務要回報三個維運指標——
//         (1) 目前同時在線的 session 數（可增可減）
//         (2) 每個 session 的唯一追蹤編號（只增不減，用於串接 log）
//         (3) 啟動至今的同時在線尖峰值（單調遞增的水位線）
//   為什麼三者都適合 static：它們描述的都是「整個服務」的狀態，
//         不屬於任何一個 session 物件。
//   重點是三者的更新規則完全不同，這是本範例真正要教的：
//         activeCount_ 建構 ++ / 解構 --      → 反映當下
//         nextTraceId_ 只在建構 ++            → 保證唯一，絕不回頭
//         peakActive_  只在「超過」時更新     → 水位線只升不降
//   實務註記：正式環境要換成 std::atomic，否則多執行緒下計數會漏、編號會重複。
// -----------------------------------------------------------------------------
class HttpSession {
private:
    string clientIp_;
    long long traceId_;

    static int activeCount_;        // 當下在線數：可增可減
    static long long nextTraceId_;  // 流水號：只增不減
    static int peakActive_;         // 尖峰水位：只升不降

public:
    explicit HttpSession(const string& ip)
        : clientIp_(ip), traceId_(nextTraceId_++) {
        ++activeCount_;
        if (activeCount_ > peakActive_) peakActive_ = activeCount_;   // 更新水位線
    }

    ~HttpSession() { --activeCount_; }

    long long traceId() const { return traceId_; }
    const string& clientIp() const { return clientIp_; }

    static int activeCount() { return activeCount_; }
    static int peakActive()  { return peakActive_; }
    static long long issuedTotal() { return nextTraceId_ - 9000; }   // 已發出的編號數
};

// 類別外定義（C++17 以前唯一的做法；本檔刻意保留傳統寫法以對照檔案 2）
int       HttpSession::activeCount_  = 0;
long long HttpSession::nextTraceId_  = 9000;   // 追蹤編號從 9000 起跳
int       HttpSession::peakActive_   = 0;

int main() {
    cout << "=== 靜態成員變數基礎 ===" << endl;

    cout << "\n--- 創建士兵 ---" << endl;
    Soldier s1("阿強");
    Soldier s2("阿明");
    Soldier s3("阿華");

    cout << "\n--- 報到 ---" << endl;
    s1.report();
    s2.report();
    s3.report();
    s1.showTotal();

    cout << "\n=== 日常實務: session 的三種 static 統計 ===" << endl;
    {
        HttpSession a("10.0.0.7");
        HttpSession b("10.0.0.8");
        cout << "  a.traceId = " << a.traceId() << " (" << a.clientIp() << ")" << endl;
        cout << "  b.traceId = " << b.traceId() << " (" << b.clientIp() << ")" << endl;
        cout << "  目前在線 = " << HttpSession::activeCount()
             << "，尖峰 = " << HttpSession::peakActive() << endl;

        {
            HttpSession c("10.0.0.9");
            cout << "  c.traceId = " << c.traceId() << endl;
            cout << "  三人同時在線 -> 在線 = " << HttpSession::activeCount()
                 << "，尖峰 = " << HttpSession::peakActive() << endl;
        }   // c 離開，在線數減少，但尖峰不會退回

        cout << "  c 離線後 -> 在線 = " << HttpSession::activeCount()
             << "，尖峰仍為 " << HttpSession::peakActive() << "（水位線只升不降）" << endl;

        HttpSession d("10.0.0.10");
        cout << "  d.traceId = " << d.traceId()
             << "（沒有沿用 c 的 " << 9002 << "，流水號絕不回收）" << endl;
    }
    cout << "  全部離線 -> 在線 = " << HttpSession::activeCount()
         << "，尖峰 = " << HttpSession::peakActive()
         << "，累計發出編號 = " << HttpSession::issuedTotal() << " 個" << endl;

    cout << "\n--- 作用域結束，逆序解構 ---" << endl;
    // s3、s2、s1 依次解構

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第\ 24\ 課：類別內的靜態成員變數1.cpp -o static1

// 【輸出說明】
//   1. 三名士兵的 ID 是 1001/1002/1003 —— nextId_ 是「整個類別共用一份」，
//      所以編號連續遞增，不會因為換了另一個物件就從頭來過。
//   2. 退役段落的總人數 2 → 1 → 0，同時證明兩件事：
//      (a) static 計數器確實被所有物件共享；
//      (b) 區塊內的自動物件以「建構的相反順序」解構（阿華最先退役）。
//   3. 實務段的尖峰值停在 3 不會退回 2，示範「水位線只升不降」；
//      d 拿到 9003 而不是回收 c 的 9002，示範「流水號絕不回頭」。
//   4. 本檔的計數器都不是 atomic，僅適用單執行緒；
//      輸出在單執行緒下完全決定性，連續執行 3 次 md5 相同。

// === 預期輸出 ===
// === 靜態成員變數基礎 ===
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
// === 日常實務: session 的三種 static 統計 ===
//   a.traceId = 9000 (10.0.0.7)
//   b.traceId = 9001 (10.0.0.8)
//   目前在線 = 2，尖峰 = 2
//   c.traceId = 9002
//   三人同時在線 -> 在線 = 3，尖峰 = 3
//   c 離線後 -> 在線 = 2，尖峰仍為 3（水位線只升不降）
//   d.traceId = 9003（沒有沿用 c 的 9002，流水號絕不回收）
//   全部離線 -> 在線 = 0，尖峰 = 3，累計發出編號 = 4 個
//
// --- 作用域結束，逆序解構 ---
//   [退役] 阿華 (ID:1003 總人數:2)
//   [退役] 阿明 (ID:1002 總人數:1)
//   [退役] 阿強 (ID:1001 總人數:0)
//
