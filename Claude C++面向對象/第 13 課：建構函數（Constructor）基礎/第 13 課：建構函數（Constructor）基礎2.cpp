// =============================================================================
//  第 13 課：建構函數（Constructor）基礎 2  —  C 風格的「手動初始化」錯在哪裡
// =============================================================================
//
// 【主題資訊 Information】
//   主題      : C 的 struct + init 函式 vs C++ 的 constructor
//   語法      : typedef struct { ... } T;  void t_init(T*, ...);
//   標準版本  : 本檔僅用 C++98 就有的功能；strcpy 來自 C89
//   標頭檔    : <cstring>（strcpy / snprintf 相關）、<string>
//   關鍵結論  : 問題不在「C 沒有語法糖」，而在「初始化沒有被語言強制」
//
// 【詳細解釋 Explanation】
//
// 【1. C 模式的真正缺陷：初始化是「約定」而不是「保證」】
//   在 C 裡，「建立物件」與「讓物件變成合法狀態」是兩件分離的事：
//       Student s;                       // 第一步：配置記憶體，內容是垃圾
//       student_init(&s, "張三", 20, 3.8);  // 第二步：希望你記得呼叫
//   語言完全不知道第二步存在。你忘了、寫錯順序、在某個 early return 前漏掉，
//   編譯器一句話都不會說。這種「靠紀律維持正確性」的設計，在幾萬行的專案裡
//   必然會被違反——不是因為工程師不小心，而是因為機率。
//
//   C++ 的 constructor 把兩步合成一步，並且**語言層面保證**：
//   物件的生命週期從 constructor 成功結束才開始。你沒有辦法「忘記」它。
//
// 【2. 第二個缺陷：沒有對稱的清理機制】
//   如果 Student 裡改成 char* name（動態配置），C 還需要一個 student_free()。
//   於是每個 return 路徑、每個 goto、每個錯誤分支都要記得呼叫它。
//   C++ 用 destructor 解決：離開 scope 就自動執行，連 exception 拋出時也會執行。
//   這就是 RAII 的起點——constructor 取得資源、destructor 釋放資源。
//
// 【3. 第三個缺陷：無法封裝，不變式（invariant）無從保障】
//   C struct 的欄位全是公開的，任何人都能寫 s.age = -5;。
//   即使 student_init 做了驗證，事後也能被繞過。
//   C++ 可以把成員設為 private，讓「唯一的入口」是 constructor 與成員函式，
//   於是「age 一定介於 0~150」這種不變式才真的守得住。
//
// 【4. 本檔另一個藏起來的地雷：strcpy】
//   strcpy(s->name, name) 不檢查長度。只要傳進來的字串超過 49 個字元，
//   就會寫爆 name[50] 這塊緩衝區，覆蓋掉相鄰的 age、gpa 甚至回傳位址。
//   這是 CWE-120 buffer overflow，數十年來最大宗的遠端執行漏洞來源。
//   本檔用 snprintf 示範安全寫法（保證寫入 NUL 結尾且不越界）。
//   而在 C++ 裡，直接用 std::string 成員就從根本上沒有這個問題。
//
// 【概念補充 Concept Deep Dive】
//   ▍為什麼 C++ 還是保留了「和 C 一樣快」的能力？
//     C++ 的 constructor 不是執行期的額外機制。對這種簡單類別，
//     編譯器產生的機器碼和你手寫的 init 函式幾乎一樣，甚至更好
//     （可以 inline、可以省略中間的暫存物件）。
//     你付出的只有「寫法」的改變，不是效能。
//
//   ▍trivial type 與 memcpy
//     這個 C 風格 struct 是 trivially copyable，可以合法地用 memcpy 整塊複製、
//     可以寫進檔案再讀回來。一旦成員改成 std::string，這些就都不再成立
//     （string 內部有指標，memcpy 之後兩個物件會指向同一塊記憶體）。
//     這是 C 風格 struct 唯一真正的優勢場景：與 C API、檔案格式、網路封包互通。
//
//   ▍char name[50] 的固定成本
//     不論名字多短，每個 Student 都固定佔 50 bytes。std::string 則是
//     動態配置 + SSO（short string optimization，門檻為實作定義，
//     libstdc++ 常見為 15 bytes）。兩者是空間與彈性的取捨，沒有絕對優劣。
//
// 【注意事項 Pay Attention】
//   1. 忘記呼叫 init 之後讀取成員 → 讀取 indeterminate value，屬 UB，
//      不會有固定的錯誤現象，也可能「看起來正常」。
//   2. strcpy 沒有邊界檢查；本檔保留它是為了示範問題，實務請用 snprintf
//      或直接改用 std::string。
//   3. sizeof 的結果是實作定義（含對齊 padding），本檔輸出的數值僅代表
//      本機 x86-64 / GCC 的結果。
//   4. 本檔的「C 風格」在 C++ 中仍然合法，且在與 C API 互通時是正確選擇；
//      要批判的是「把它當成一般業務物件的預設寫法」。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】為什麼需要 constructor
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. constructor 比「struct + init 函式」好在哪？講三點。
//     答：(1) 強制性——語言保證它被呼叫，不可能忘記；
//         (2) 對稱性——有 destructor 自動清理，連 exception 路徑也涵蓋；
//         (3) 封裝性——成員可以是 private，不變式才守得住。
//         效能不是理由，兩者產生的機器碼通常一樣。
//     追問：那 C 風格 struct 什麼時候仍然是對的？→ 與 C API 互通、
//         需要 trivially copyable 以便 memcpy／寫入檔案／送上網路時。
//
// 🔥 Q2. 「物件從什麼時候開始算存在」在 C 和 C++ 有何不同？
//     答：C 只有記憶體概念，配置完就能存取。C++ 有明確的物件生命週期：
//         constructor 成功結束後生命週期才開始，destructor 開始執行後就結束。
//         在這之外存取成員是 UB。這個定義是 RAII、exception safety 的地基。
//     追問：constructor 中途拋出例外會怎樣？→ 物件生命週期從未開始，
//         destructor 不會被呼叫；但已完成建構的成員會依反序被解構。
//
// ⚠️ 陷阱. 「反正我每次都記得呼叫 init()，所以兩種寫法一樣安全」
//     答：不一樣。差別不在你這次記不記得，而在「編譯器能不能檢查」。
//         程式碼會被別人修改、會新增 early return、會被複製貼上到別處。
//         能被違反的約定，長期來看一定會被違反。
//     為什麼會錯：把「正確性」當成紀律問題，而不是設計問題。
//         好的介面應該讓錯誤的用法寫不出來，而不是靠註解提醒。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <cstdio>   // snprintf
#include <cstring>  // strcpy

// -----------------------------------------------------------------------------
// C 語言風格：資料與初始化分離
// -----------------------------------------------------------------------------
typedef struct {
    char  name[50];
    int   age;
    float gpa;
} StudentC;

// 必須手動呼叫！而且有可能忘記呼叫！
void student_init(StudentC* s, const char* name, int age, float gpa) {
    strcpy(s->name, name);   // ⚠️ 無邊界檢查：name 超過 49 字元就會寫爆緩衝區
    s->age = age;
    s->gpa = gpa;
}

// 安全版：用 snprintf 保證不越界且一定有 NUL 結尾
void student_init_safe(StudentC* s, const char* name, int age, float gpa) {
    std::snprintf(s->name, sizeof(s->name), "%s", name);
    s->age = age;
    s->gpa = gpa;
}

// -----------------------------------------------------------------------------
// C++ 風格：建立即合法，語言強制執行
// -----------------------------------------------------------------------------
class StudentCpp {
private:
    std::string name;   // 不需要煩惱長度上限，也沒有緩衝區溢位問題
    int         age;
    float       gpa;

public:
    StudentCpp(const std::string& n, int a, float g)
        : name(n), age(a), gpa(g) {}   // 不可能「忘記呼叫」

    void print() const {
        std::cout << "  姓名: " << name << ", 年齡: " << age
                  << ", GPA: " << gpa << std::endl;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】從缺
//   理由：本檔比較的是「C 的初始化慣例 vs C++ 的語言保證」，屬於工程設計議題，
//         沒有任何一題 LeetCode 在考這件事。硬掛一題設計題（例如 155 Min Stack）
//         也只會變成「用 C++ 寫一題」，跟本檔想講的對比毫無關係。
//         本課真正對應的設計題集中在 summary.cpp。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】與 C API 互通：把固定長度欄位安全地轉成 std::string
//   情境：讀取二進位記錄檔／網路封包時，欄位常是 char[N] 且「不保證有 NUL 結尾」
//         （寫滿 N 個字元時就沒有結尾符）。直接丟給 std::string(const char*)
//         會一路讀到下一個 NUL，把後面的欄位一起吃進來，甚至越界。
//   正解：用 strnlen 限制在 N 之內，明確指定長度建構 std::string。
// -----------------------------------------------------------------------------
struct WireRecord {          // 模擬網路封包／檔案格式中的固定長度記錄
    char userId[8];          // 寫滿 8 個字元時「沒有」NUL 結尾
    char status[4];
};

std::string fixedFieldToString(const char* field, std::size_t capacity) {
    // strnlen 最多只看 capacity 個位元組，找不到 NUL 就回傳 capacity
    std::size_t len = ::strnlen(field, capacity);
    return std::string(field, len);      // 明確給長度，不依賴 NUL 結尾
}

int main() {
    std::cout << "=== C 風格：初始化是「約定」，不是「保證」 ===" << std::endl;
    StudentC s1;
    student_init_safe(&s1, "張三", 20, 3.8f);
    std::cout << "  有呼叫 init : 姓名=" << s1.name
              << ", 年齡=" << s1.age << ", GPA=" << s1.gpa << std::endl;
    std::cout << "  忘記呼叫 init: 成員為不確定值，讀取即 UB —— 故意不印出來，"
              << std::endl;
    std::cout << "                 因為印一個沒有意義的數字並不能證明什麼。"
              << std::endl;

    std::cout << "\n=== C++ 風格：不可能忘記 ===" << std::endl;
    StudentCpp s2("李四", 22, 3.5f);   // 建立的同時就完成初始化
    s2.print();
    std::cout << "  這裡沒有「第二步」可以忘記，語言替你保證了。" << std::endl;

    std::cout << "\n=== strcpy 的邊界問題 ===" << std::endl;
    // 刻意用 ASCII 字串：位元組截斷若切在多位元組字元中間，會產生半個 UTF-8 字元
    // （這本身就是 char[N] 欄位的另一個陷阱，見下方說明）
    const char* longName = "Bartholomew-Fitzgerald-Montgomery-Wellington-III-Esq";
    StudentC s3;
    student_init_safe(&s3, longName, 30, 3.0f);   // 安全版：截斷而非寫爆
    std::cout << "  原字串長度 = " << std::strlen(longName) << " bytes" << std::endl;
    std::cout << "  緩衝區容量 = " << sizeof(s3.name) << " bytes（實作定義）" << std::endl;
    std::cout << "  snprintf 截斷後 = [" << s3.name << "]" << std::endl;
    std::cout << "  若改用 strcpy，超出的部分會覆寫相鄰記憶體（CWE-120），"
              << std::endl;
    std::cout << "  後果不可預測，故本檔不實際示範。" << std::endl;
    std::cout << "  另一個陷阱：snprintf 是以「位元組」截斷的，若字串是 UTF-8 中文，"
              << std::endl;
    std::cout << "  截斷點可能切在一個字元的中間，產生半個字元的亂碼。" << std::endl;

    std::cout << "\n=== 日常實務：固定長度欄位轉 std::string ===" << std::endl;
    WireRecord rec;
    // 刻意「寫滿」userId，模擬沒有 NUL 結尾的真實封包
    std::memcpy(rec.userId, "u1234567", 8);
    std::memcpy(rec.status, "OK\0\0",   4);

    std::string uid    = fixedFieldToString(rec.userId, sizeof(rec.userId));
    std::string status = fixedFieldToString(rec.status, sizeof(rec.status));
    std::cout << "  userId = [" << uid    << "] (長度 " << uid.size()    << ")" << std::endl;
    std::cout << "  status = [" << status << "] (長度 " << status.size() << ")" << std::endl;
    std::cout << "  userId 寫滿 8 bytes 沒有 NUL 結尾，靠 strnlen 才不會讀過頭。"
              << std::endl;

    std::cout << "\n=== 記憶體佈局對照（皆為實作定義）===" << std::endl;
    std::cout << "  sizeof(StudentC)   = " << sizeof(StudentC)
              << " bytes（char[50] + int + float + padding）" << std::endl;
    std::cout << "  sizeof(StudentCpp) = " << sizeof(StudentCpp)
              << " bytes（std::string + int + float + padding）" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 13 課：建構函數（Constructor）基礎2.cpp" -o ctor2
//
// 註：sizeof 為實作定義（本機 x86-64 / GCC 15.2 / libstdc++），
//     換平台、換標準函式庫實作都可能不同。


// === 預期輸出 ===
// === C 風格：初始化是「約定」，不是「保證」 ===
//   有呼叫 init : 姓名=張三, 年齡=20, GPA=3.8
//   忘記呼叫 init: 成員為不確定值，讀取即 UB —— 故意不印出來，
//                  因為印一個沒有意義的數字並不能證明什麼。
//
// === C++ 風格：不可能忘記 ===
//   姓名: 李四, 年齡: 22, GPA: 3.5
//   這裡沒有「第二步」可以忘記，語言替你保證了。
//
// === strcpy 的邊界問題 ===
//   原字串長度 = 52 bytes
//   緩衝區容量 = 50 bytes（實作定義）
//   snprintf 截斷後 = [Bartholomew-Fitzgerald-Montgomery-Wellington-III-]
//   若改用 strcpy，超出的部分會覆寫相鄰記憶體（CWE-120），
//   後果不可預測，故本檔不實際示範。
//   另一個陷阱：snprintf 是以「位元組」截斷的，若字串是 UTF-8 中文，
//   截斷點可能切在一個字元的中間，產生半個字元的亂碼。
//
// === 日常實務：固定長度欄位轉 std::string ===
//   userId = [u1234567] (長度 8)
//   status = [OK] (長度 2)
//   userId 寫滿 8 bytes 沒有 NUL 結尾，靠 strnlen 才不會讀過頭。
//
// === 記憶體佈局對照（皆為實作定義）===
//   sizeof(StudentC)   = 60 bytes（char[50] + int + float + padding）
//   sizeof(StudentCpp) = 40 bytes（std::string + int + float + padding）
