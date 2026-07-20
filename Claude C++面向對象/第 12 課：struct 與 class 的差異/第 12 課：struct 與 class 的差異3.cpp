// =============================================================================
//  第 12 課：struct 與 class 的差異 3  —  C 的 struct vs C++ 的 struct
// =============================================================================
//
// 【主題資訊 Information】
//   主題：同一個關鍵字 struct，在 C 與 C++ 兩個語言中能力天差地遠
//   標準版本：C++98 起 struct 即具備完整 class 能力；本檔的 NSDMI（成員預設值）
//             需要 C++11，而「有 NSDMI 仍算 aggregate」則要 C++14（見下方實測）
//   標頭檔：<string>（C++ 版用 std::string）、<cstring>（C 風格字串操作）
//
// 【詳細解釋 Explanation】
//
// 【1. C 的 struct：只是「一坨貼在一起的變數」】
//   在 C 裡，struct 的唯一功能是把幾個變數打包成一個型別，它沒有：
//     * 成員函式（行為要另外寫成 free function，並手動把 struct* 傳進去）
//     * 存取控制（沒有 public/private，所有欄位全開）
//     * 建構／解構函式（初始化要自己記得呼叫，忘了就是垃圾值）
//     * 繼承與多型
//   而且 C 的 struct 名字不會自動進入型別名稱空間，必須寫 `struct C_Style s;`
//   或用 typedef 包一層 —— 這就是為什麼大量 C 標頭檔充滿 `typedef struct {...} X;`。
//   C++ 取消了這個限制：class-name 本身就是型別名，直接寫 `C_Style s;` 即可。
//
// 【2. C++ 的 struct：功能完全等同 class】
//   C++ 的 struct 可以有成員函式、建構函式、繼承、virtual、模板 —— 一切 class
//   能做的它都能做。它與 class 的差別只剩「預設存取權」這一件事（見本課第 2 檔）。
//   本檔的 CPP_Style 就示範了：資料 + 行為（show / isPassing）寫在同一個型別裡，
//   呼叫端不必再記得「要配哪一個 free function」。
//
// 【3. 為什麼「資料與行為放在一起」是進步？】
//   C 版本的 score 是裸露的 float，判斷及格的規則（>= 60）散落在每個呼叫端。
//   哪天規則改成 >= 50，你得全專案 grep。C++ 版把規則收斂進 isPassing()，
//   改一處即可 —— 這就是封裝真正的價值：不是「藏起來」，而是「規則只有一份」。
//
// 【4. char[50] vs std::string —— 兩種字串模型的取捨】
//   C 的 char name[50] 是「固定長度、內嵌在結構體內」：
//     優點：大小固定可預期、可直接 memcpy 寫進檔案或送上網路（POD 特性）
//     缺點：超過 49 個字元就截斷或緩衝區溢位；strcpy 不檢查長度，是 CWE-120 的溫床
//   std::string 是「動態長度、自動管理記憶體」：
//     優點：長度不限、自動釋放、有 size()、支援比較與串接
//     缺點：sizeof 是固定的（實作定義，本機實測見輸出），但實際資料可能在 heap，
//           因此不能直接 memcpy 整個結構體去序列化（指標寫進檔案毫無意義）
//
// 【概念補充 Concept Deep Dive】
//   * C_Style 是 POD（Plain Old Data，C++20 起術語拆成 trivial + standard-layout）：
//     可以安全地 memcpy、可以直接 fwrite 成二進位檔、與 C 完全 ABI 相容。
//     CPP_Style 因為含 std::string，不再是 trivially copyable，上述操作全部失效。
//     本檔用 static_assert 在編譯期把這件事釘死（見程式碼），這比口頭說明可靠。
//   * 「有 NSDMI 還算不算 aggregate」是常考的版本差異，實測結果（GCC 15.2 +
//     -pedantic-errors）：C++11 拒絕、C++14 起接受。所以 CPP_Style{...} 這種
//     aggregate initialization 在 C++11 編不過，C++14 才行。
//   * sizeof(C_Style) 不等於 50+4+4=58：編譯器會為了對齊插入 padding。
//     實際數值是實作定義，本機實測見預期輸出。
//
// 【注意事項 Pay Attention】
//   1. strcpy 不做長度檢查，來源超過目的地容量就是緩衝區溢位（UB）。本檔改用
//      snprintf 並明確傳入容量，這是 C 風格字串該有的寫法。
//   2. 不要對含 std::string / 含虛擬函式的型別做 memcpy 或 fwrite，那是 UB。
//      要序列化請逐欄位處理。
//   3. C 的 struct 未初始化就讀取，成員值是不確定的（indeterminate），
//      讀它是 UB —— 不能假設「一定是 0」也不能假設「一定是垃圾」。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】C struct 與 C++ struct
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C 的 struct 和 C++ 的 struct 差在哪？
//     答：C 的 struct 只能放資料，沒有成員函式、沒有存取控制、沒有建構／解構函式、
//         不能繼承，而且型別名要寫成 `struct X`。C++ 的 struct 具備完整 class 能力，
//         與 class 只差在預設存取權。
//     追問：那 C++ 為什麼還保留 struct？→ 為了與 C 的標頭檔和 ABI 相容，
//           同時讓「純資料聚合」有個表達意圖的關鍵字。
//
// 🔥 Q2. 什麼樣的型別可以安全地 memcpy？
//     答：trivially copyable 的型別。含 std::string、std::vector、虛擬函式、
//         或自訂複製建構函式的型別都不是，對它們 memcpy 是 UB（會複製到指標，
//         造成 double free 或懸空指標）。可用 std::is_trivially_copyable 在編譯期檢查。
//     追問：那要怎麼把 CPP_Style 存檔？→ 逐欄位序列化：先寫字串長度再寫內容，
//           或用 protobuf / JSON 這類既有格式，不要整塊搬。
//
// ⚠️ 陷阱. sizeof(C_Style) 是 50 + 4 + 4 = 58 嗎？
//     答：不是。編譯器會插入 padding 讓 int / float 落在自然對齊邊界上，
//         實際值是實作定義的（本機實測見預期輸出）。
//     為什麼會錯：把 struct 想成「欄位大小的單純加總」，忽略了對齊（alignment）。
//         這在跨平台寫二進位檔或定義網路封包時會直接造成資料錯位，
//         正式做法是明確 #pragma pack 或逐欄位讀寫，而不是賭 sizeof。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <cstdio>       // snprintf
#include <cstring>      // strlen
#include <type_traits>  // is_trivially_copyable
using namespace std;

// ===== C 語言的 struct（在 C++ 中也能編譯）=====
// 只能有資料，不能有函數, 沒有存取控制，沒有繼承
// C 中必須寫 struct C_Style s; （C++ 中可省略 struct 關鍵字）
// 注意：C++ 中的 struct 和 class 的唯一區別是預設的存取控制不同：
//       struct 預設 public，class 預設 private
struct C_Style {
    char name[50];
    int age;
    float score;
};

// C 風格：行為必須寫成外部函式，並手動把 struct* 傳進去。
// 注意這裡用 snprintf 而不是 strcpy —— strcpy 不檢查長度，來源過長即緩衝區溢位。
void cStyleInit(C_Style* s, const char* name, int age, float score) {
    snprintf(s->name, sizeof(s->name), "%s", name);   // 明確傳入容量，保證不溢位
    s->age = age;
    s->score = score;
}
// 及格規則散落在外部，改規則要全專案 grep —— 這正是 C 風格的維護痛點
bool cStyleIsPassing(const C_Style* s) {
    return s->score >= 60.0f;
}

// ===== C++ 的 struct =====
// 可以有函數、建構函數、存取控制、繼承……
// 資料與規則收斂在同一個型別內，改規則只要改這裡一處
struct CPP_Style {
    string name;
    int age = 0;          // NSDMI（成員預設值）：C++11 起支援
    float score = 0.0f;

    void show() const {
        cout << "  " << name << ", " << age << " 歲, " << score << " 分" << endl;
    }

    bool isPassing() const {
        return score >= 60.0f;   // 及格規則只有這一份
    }
};

// 編譯期把「哪個能 memcpy」這件事釘死，比註解可靠
static_assert(std::is_trivially_copyable<C_Style>::value,
              "C_Style 應為 trivially copyable（可安全 memcpy / fwrite）");
static_assert(!std::is_trivially_copyable<CPP_Style>::value,
              "CPP_Style 含 std::string，不可 memcpy");

// -----------------------------------------------------------------------------
// 【日常實務範例】固定長度二進位紀錄 vs 動態字串紀錄
//   情境：把一筆學生成績寫進二進位檔（例如舊系統的 .dat 檔或網路封包）。
//   C_Style 因為是 trivially copyable，可以整塊 memcpy 進 buffer；
//   CPP_Style 則必須逐欄位序列化，否則寫進去的是 std::string 內部的指標值，
//   換一次執行就完全無效。這是實務上處理舊格式檔案時每天都會碰到的取捨。
// -----------------------------------------------------------------------------
size_t serializeCStyle(const C_Style& rec, unsigned char* buf, size_t cap) {
    if (cap < sizeof(C_Style)) return 0;          // 容量不足就拒絕，不要硬寫
    memcpy(buf, &rec, sizeof(C_Style));           // 合法：trivially copyable
    return sizeof(C_Style);
}

// CPP_Style 的正確序列化：長度前綴 + 內容，逐欄位處理
string serializeCppStyle(const CPP_Style& rec) {
    string out;
    out += to_string(rec.name.size());            // 先寫長度，讀回來才知道要吃幾個 byte
    out += '|';
    out += rec.name;
    out += '|';
    out += to_string(rec.age);
    out += '|';
    out += to_string(rec.score);
    return out;
}

int main() {
    cout << "=== C 風格 struct ===" << endl;
    C_Style cs;
    cStyleInit(&cs, "小明", 20, 85.5f);
    cout << "  " << cs.name << ", " << cs.age << " 歲, " << cs.score << " 分" << endl;
    cout << "  及格（規則寫在外部函式）: " << (cStyleIsPassing(&cs) ? "是" : "否") << endl;

    // 示範 snprintf 的截斷保護：故意塞超長名字
    C_Style overflow;
    string longName(80, 'X');                     // 80 個字元，遠超過 char[50]
    cStyleInit(&overflow, longName.c_str(), 1, 0.0f);
    cout << "  超長名字被安全截斷為 " << strlen(overflow.name)
         << " 個字元（緩衝區 " << sizeof(overflow.name) << "）" << endl;

    cout << "\n=== C++ 風格 struct ===" << endl;
    CPP_Style cpps;
    cpps.name = "小明";
    cpps.age = 20;
    cpps.score = 85.5f;
    cpps.show();
    cout << "  及格（規則收斂在型別內）: " << (cpps.isPassing() ? "是" : "否") << endl;

    // aggregate initialization：需要 C++14 以上（C++11 因 NSDMI 而不算 aggregate）
    CPP_Style topStudent{"小美", 22, 95.0f};
    topStudent.show();

    cout << "\n=== 記憶體佈局（實作定義，本機 GCC 15.2 x86-64）===" << endl;
    cout << "  sizeof(C_Style)   = " << sizeof(C_Style)
         << " （欄位加總 50+4+4=58，差額是對齊 padding）" << endl;
    cout << "  alignof(C_Style)  = " << alignof(C_Style) << endl;
    cout << "  sizeof(CPP_Style) = " << sizeof(CPP_Style)
         << " （含 std::string 本體；字串內容可能另在 heap）" << endl;

    cout << "\n=== 實務：序列化 ===" << endl;
    unsigned char buf[128];
    size_t n = serializeCStyle(cs, buf, sizeof(buf));
    cout << "  C_Style 整塊 memcpy 成功，寫入 " << n << " bytes" << endl;
    cout << "  CPP_Style 逐欄位序列化: " << serializeCppStyle(cpps) << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 12 課：struct 與 class 的差異3.cpp" -o demo3
// 註：本檔用到 aggregate initialization + NSDMI，需 C++14 以上；C++11 會編譯失敗。

// === 預期輸出 ===
// === C 風格 struct ===
//   小明, 20 歲, 85.5 分
//   及格（規則寫在外部函式）: 是
//   超長名字被安全截斷為 49 個字元（緩衝區 50）
//
// === C++ 風格 struct ===
//   小明, 20 歲, 85.5 分
//   及格（規則收斂在型別內）: 是
//   小美, 22 歲, 95 分
//
// === 記憶體佈局（實作定義，本機 GCC 15.2 x86-64）===
//   sizeof(C_Style)   = 60 （欄位加總 50+4+4=58，差額是對齊 padding）
//   alignof(C_Style)  = 4
//   sizeof(CPP_Style) = 40 （含 std::string 本體；字串內容可能另在 heap）
//
// === 實務：序列化 ===
//   C_Style 整塊 memcpy 成功，寫入 60 bytes
//   CPP_Style 逐欄位序列化: 6|小明|20|85.500000
