// =============================================================================
//  第 13 課：建構函數基礎 1  —  沒有建構函式的物件：成員值是「不確定的」
// =============================================================================
//
// 【主題資訊 Information】
//   主題：示範「沒有初始化成員」會發生什麼事，藉此帶出建構函式存在的理由
//   標準版本：C++98 起皆然；本檔用到的 std::is_trivially_default_constructible 需 C++11
//   標頭檔：<string>
//   ⚠️ 本檔刻意示範「讀取未初始化成員」——那是 undefined behavior（UB），
//      是反面教材，不是可以照抄的寫法。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 age 和 gpa 沒有值，name 卻是空字串？】
//   關鍵在於「這個成員是不是 class type」：
//     * std::string name：它自己有預設建構函式，物件建立時一定會被呼叫，
//       所以 name 保證是空字串 —— 這是有保證的行為，不是運氣。
//     * int age / float gpa：這是內建型別（built-in type），沒有建構函式。
//       編譯器生成的預設建構函式對它們「什麼都不做」，於是它們保留了
//       那塊記憶體原本的位元內容 —— 值是「不確定的」（indeterminate）。
//   這個差別讓 bug 特別陰險：同一個類別裡，有些成員安全、有些成員危險。
//
// 【2. 「不確定的值」不等於「一定是垃圾」，也不等於「一定是 0」】
//   標準對未初始化的內建型別成員只說值是 indeterminate，讀取它是 UB。
//   實務上你可能觀察到：
//     * 剛啟動的程式，堆疊記憶體常常剛好是 0（作業系統給行程的新頁面會清零）
//     * 跑久一點、堆疊被反覆使用之後，就會讀到前一次呼叫遺留的內容
//     * 開了最佳化（-O2）之後，編譯器可能整個省略那次讀取，行為又不一樣
//   所以本檔的輸出「這一次」長什麼樣，不代表下一次、換台機器、換個編譯選項
//   也一樣。任何「未初始化的 int 一定是 0」的說法都是錯的。
//
// 【3. 這為什麼是嚴重問題，而不只是「值不好看」？】
//   UB 的可怕之處在於它不保證「錯得很明顯」。真實專案裡常見的情況是：
//     * 開發時堆疊剛好是 0，測試全過；上線後跑久了讀到殘留值，偶發崩潰
//     * 未初始化的指標成員拿去 delete，造成難以追查的記憶體毀損
//     * 未初始化的長度欄位拿去配置記憶體，變成安全漏洞
//   因此正解不是「記得手動賦值」，而是「讓語言保證它不可能沒被初始化」——
//   這正是建構函式（下一檔開始）要解決的問題。
//
// 【4. 三種立即可用的補救方式（依推薦度排序）】
//   (a) 建構函式（第 13 課主角）：把初始化寫進建構函式，物件一誕生就合法。
//   (b) NSDMI（C++11 成員預設值）：`int age = 0;` 直接寫在宣告處，最省事。
//   (c) 值初始化：`Student s{};` 對「沒有使用者提供建構函式」的類別會零初始化。
//       注意這只在呼叫端生效，類別作者無法強制，因此可靠度最低。
//
// 【概念補充 Concept Deep Dive】
//   * 編譯器「自動生成」的預設建構函式（implicitly-defined default constructor）
//     並不是什麼都不做 —— 它會依序呼叫每個 class type 成員的預設建構函式，
//     只是對內建型別不做任何事。理解這一點就能解釋本檔的現象。
//   * 這種類別是 trivially default constructible（本檔用 static_assert 驗證），
//     代表「建構」在機器碼層面可能完全沒有指令 —— 這正是它快、也正是它危險的原因。
//     C++ 的設計哲學是「不為你沒要求的東西付出代價」（zero-overhead principle），
//     自動清零會讓每個物件都付出成本，因此標準選擇不做。
//   * 編譯器通常抓不到這種錯誤。-Wall -Wextra 對「成員」未初始化多半沒有警告
//     （對區域變數才比較會提醒）。要抓它請用 -fsanitize=memory（Clang）
//     或 valgrind --track-origins=yes，本檔輸出後附了實測說明。
//
// 【注意事項 Pay Attention】
//   1. 絕對不要依賴未初始化成員的值 —— 那是 UB，任何觀察到的結果都不可推廣。
//   2. 不要用「印出來看起來是 0」來論證程式正確；換個編譯選項或執行環境就會變。
//   3. std::string / std::vector 這類有預設建構函式的成員是安全的，
//      但同一個類別中的內建型別成員仍然危險，不能因為「有些成員安全」就放心。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】未初始化的成員變數
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 這個類別的物件建立後，name / age / gpa 各是什麼值？
//     答：name 保證是空字串 —— std::string 有自己的預設建構函式，一定會被呼叫。
//         age 與 gpa 是內建型別，編譯器生成的預設建構函式對它們不做任何事，
//         值是不確定的（indeterminate），讀取它們是 undefined behavior。
//     追問：那實務上會印出什麼？→ 不可預測。可能是 0、可能是殘留資料，
//           也可能因最佳化而整個被省略。不能依賴任何觀察到的結果。
//
// 🔥 Q2. 為什麼 C++ 不乾脆自動把所有成員清零？
//     答：zero-overhead principle —— 不為使用者沒要求的功能付出代價。
//         若強制清零，每個物件建立都要多付清零成本，對高效能場景
//         （每秒建立百萬個小物件）是無法接受的負擔。C++ 選擇把控制權交給你。
//     追問：那要怎麼安全又不失效能？→ 用 NSDMI 或建構函式明確初始化；
//           需要時編譯器仍能最佳化掉多餘的寫入，成本可控。
//
// ⚠️ 陷阱. 「我測過了，未初始化的 int 就是 0」這句話錯在哪？
//     答：那只是「這次剛好」。作業系統給新行程的記憶體頁面通常已清零，
//         所以程式剛啟動時觀察到 0 很常見；但只要那塊堆疊被用過一輪，
//         就會讀到前一次函式呼叫遺留的內容。開最佳化後行為又可能再變。
//     為什麼會錯：把「一次執行的觀察結果」當成「語言的保證」。
//         UB 最危險的特性正是它經常「看起來能動」，直到上線才爆炸。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <type_traits>
using namespace std;

// ❌ 反面教材：沒有任何初始化機制
class Student {
public:
    string name;   // 安全：std::string 有自己的預設建構函式 → 保證是空字串
    int age;       // 危險：內建型別，值不確定（indeterminate）
    float gpa;     // 危險：同上

    void print() {
        // ⚠️ 讀取 age / gpa 是 undefined behavior。
        //    這裡刻意示範問題，正式程式碼絕不可如此。
        cout << "姓名: [" << name << "]"
             << ", 年齡: " << age
             << ", GPA: " << gpa << endl;
    }
};

// ✅ 正解之一：NSDMI（C++11）—— 在宣告處直接給預設值，最省事
class StudentFixed {
public:
    string name = "未命名";
    int    age = 0;
    float  gpa = 0.0f;

    void print() const {
        cout << "姓名: [" << name << "]"
             << ", 年齡: " << age
             << ", GPA: " << gpa << endl;
    }
};

// 編譯期驗證「為什麼會這樣」：Student 是 trivially default constructible，
// 代表它的預設建構在機器碼層面可能完全沒有指令 —— 這正是成員沒被清零的原因。
static_assert(std::is_trivially_default_constructible<Student>::value == false,
              "Student 含 std::string，整體不是 trivial（但 int/float 成員仍未被初始化）");
static_assert(std::is_trivially_default_constructible<int>::value,
              "int 的預設初始化不做任何事");

// -----------------------------------------------------------------------------
// 【日常實務範例】未初始化欄位如何變成真實的線上事故
//   情境：解析設定檔時，若某個欄位在設定檔中缺席而程式又忘了給預設值，
//   retryCount 就會帶著不確定的值進入重試迴圈。
//   開發環境測試時它可能剛好是 0（不重試，看起來正常），
//   上線後在長時間運行的行程中卻可能讀到極大值 → 無窮重試 → 打爆下游服務。
//   這正是「UB 看起來能動」最典型的樣貌：問題不在程式碼被讀到的那一刻爆發。
//   正解：所有欄位一律給 NSDMI 預設值，讓「忘記設定」變成安全的預設行為。
// -----------------------------------------------------------------------------
struct RetryPolicyUnsafe {
    int  retryCount;      // ❌ 忘了給預設值 → 值不確定
    int  backoffMs;       // ❌ 同上
    bool enabled;         // ❌ 同上
};

struct RetryPolicySafe {
    int  retryCount = 3;       // ✅ 明確的、有意義的預設值
    int  backoffMs  = 200;
    bool enabled    = true;
};

// 模擬「套用設定」：只覆蓋設定檔中真的有出現的欄位
RetryPolicySafe loadRetryPolicy(bool hasRetryCount, int retryCountFromFile) {
    RetryPolicySafe p;                       // 先取得一組合法的預設值
    if (hasRetryCount) p.retryCount = retryCountFromFile;
    return p;                                // 缺席的欄位自動維持安全預設
}

int main() {
    cout << "=== ❌ 沒有初始化（讀取結果不可預測，屬 UB）===" << endl;
    Student s;
    s.print();
    cout << "  ↑ name 保證為空字串；age / gpa 的值不確定，" << endl;
    cout << "    每次執行、每台機器、每組編譯選項都可能不同。" << endl;

    cout << "\n=== ✅ 用 NSDMI 給預設值 ===" << endl;
    StudentFixed sf;
    sf.print();
    cout << "  ↑ 三個欄位都有明確且可重現的值" << endl;

    cout << "\n=== ✅ 值初始化：呼叫端也能自保 ===" << endl;
    Student s2{};    // 值初始化：對無使用者提供建構函式的類別會零初始化
    s2.print();
    cout << "  ↑ 加上 {} 之後 age / gpa 被零初始化（此為標準保證），" << endl;
    cout << "    但這要靠呼叫端記得寫，類別作者無法強制 → 可靠度最低" << endl;

    cout << "\n=== 實務：設定檔缺欄位時的兩種下場 ===" << endl;
    RetryPolicySafe p1 = loadRetryPolicy(true, 5);
    cout << "  設定檔有指定 retryCount=5  -> retryCount=" << p1.retryCount
         << ", backoffMs=" << p1.backoffMs
         << ", enabled=" << (p1.enabled ? "true" : "false") << endl;

    RetryPolicySafe p2 = loadRetryPolicy(false, 0);
    cout << "  設定檔沒有 retryCount 欄位 -> retryCount=" << p2.retryCount
         << ", backoffMs=" << p2.backoffMs
         << ", enabled=" << (p2.enabled ? "true" : "false")
         << "  ← 自動退回安全預設，不會失控" << endl;

    cout << "\n（若改用 RetryPolicyUnsafe，缺席欄位會帶著不確定的值進入重試迴圈，" << endl;
    cout << "  可能無限重試打爆下游服務 —— 這是真實存在的線上事故樣態）" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 13 課：建構函數（Constructor）基礎1.cpp" -o demo1
//
// 抓這類問題的工具（-Wall -Wextra 對「成員」未初始化通常不會警告）：
//   valgrind --track-origins=yes ./demo1        # 追蹤未初始化值的來源
//   clang++ -fsanitize=memory ...               # MemorySanitizer（需 Clang）

// === 預期輸出 ===
// ⚠️ 注意：下方 age / gpa 的數值來自「讀取未初始化成員」，屬 undefined behavior。
//    這是某一次實際執行的結果，每次執行都可能不同（換機器、換最佳化選項亦然），
//    絕不可視為保證值。name 為空字串、StudentFixed 與值初始化的結果才是有保證的。
// === ❌ 沒有初始化（讀取結果不可預測，屬 UB）===
// 姓名: [], 年齡: 2132286712, GPA: 4.59163e-41
//   ↑ name 保證為空字串；age / gpa 的值不確定，
//     每次執行、每台機器、每組編譯選項都可能不同。
//
// === ✅ 用 NSDMI 給預設值 ===
// 姓名: [未命名], 年齡: 0, GPA: 0
//   ↑ 三個欄位都有明確且可重現的值
//
// === ✅ 值初始化：呼叫端也能自保 ===
// 姓名: [], 年齡: 0, GPA: 0
//   ↑ 加上 {} 之後 age / gpa 被零初始化（此為標準保證），
//     但這要靠呼叫端記得寫，類別作者無法強制 → 可靠度最低
//
// === 實務：設定檔缺欄位時的兩種下場 ===
//   設定檔有指定 retryCount=5  -> retryCount=5, backoffMs=200, enabled=true
//   設定檔沒有 retryCount 欄位 -> retryCount=3, backoffMs=200, enabled=true  ← 自動退回安全預設，不會失控
//
// （若改用 RetryPolicyUnsafe，缺席欄位會帶著不確定的值進入重試迴圈，
//   可能無限重試打爆下游服務 —— 這是真實存在的線上事故樣態）
