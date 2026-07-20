// =============================================================================
//  第 4 課：命名空間（namespace）基礎（6） — 匿名命名空間:檔案私有的正統做法
// =============================================================================
//
// 【主題資訊 Information】
//   語法：namespace { 宣告... }        // unnamed / anonymous namespace
//   標準版本：C++98 起。C++11 之後,它是「檔案內部連結」的建議做法,
//             取代 C 風格的 static。
//   複雜度  ：零執行期成本。編譯器只是給它一個你打不出來的內部名字。
//
// 【詳細解釋 Explanation】
//
// 【1. 它做了什麼:給成員 internal linkage】
//   放在匿名命名空間裡的名稱,只有「目前這個翻譯單元(.cpp)」看得到。
//   別的 .cpp 即使寫了同名的東西也不會衝突,連結器根本看不到它們。
//   這正是實作細節該有的狀態:對外不可見、不佔用全域符號。
//   本機實測(g++ 15.2.0,nm 觀察目的檔的符號表)清楚顯示了差異:
//       int  globalVar;              → B globalVar                      (大寫=外部連結)
//       void globalFunc();           → T _Z10globalFuncv                (大寫=外部連結)
//       static int staticVar;        → b _ZL9staticVar                  (小寫=內部連結)
//       namespace { int internalCounter; }
//                                    → b _ZN12_GLOBAL__N_1L15internalCounterE
//       namespace { void internalFunction(); }
//                                    → t _ZN12_GLOBAL__N_1L16internalFunctionEv
//   注意最後兩個名字裡的 _GLOBAL__N_1 —— 那就是編譯器替匿名命名空間
//   生成的、每個翻譯單元各不相同的唯一名字,而 nm 的小寫 b/t 代表 local symbol。
//
// 【2. 為什麼可以不加前綴直接使用】
//   匿名命名空間的定義,語意上等同於:
//       namespace 某個唯一名字 { ... }
//       using namespace 某個唯一名字;      // ← 編譯器自動補上這一行
//   因為有這個隱含的 using 指示,你才能像本檔 main 那樣直接寫
//   internalCounter 而不必加前綴。而由於那個唯一名字外部拼不出來,
//   別的翻譯單元也就無從存取。
//
// 【3. 為什麼它取代了 static】
//   C 用 static 表示「檔案私有」。C++ 保留了這個用法,但匿名命名空間更好:
//     (a) static 只能用在變數和函式;匿名命名空間還能放「型別」——
//         而類別定義是絕對不能寫 static 的。
//         這是最實際的差別:只有匿名命名空間能讓一個 class 成為檔案私有。
//     (b) 語意單一。static 在 C++ 裡有三種意思(內部連結、靜態儲存期、
//         類別的靜態成員),閱讀時得看上下文;匿名命名空間只有一個意思。
//     (c) 匿名命名空間裡的名稱仍具有「名稱」,可以被樣板當作型別引數;
//         C++11 之前的 static 名稱在這方面有限制。
//   結論:新程式碼寫檔案私有,一律用匿名命名空間。
//
// 【4. 它與 private 的差別】
//   兩者都是「隱藏」,但維度完全不同:
//     private            → 對「類別外的程式碼」隱藏,但同一個類別可以跨檔案使用。
//     匿名命名空間        → 對「其他翻譯單元」隱藏,和類別無關。
//   典型用法是兩者搭配:公開的類別放在標頭檔,只有這個 .cpp 用得到的
//   輔助函式與常數則放進匿名命名空間。
//
// 【概念補充 Concept Deep Dive】
//   匿名命名空間有一個非常重要的使用禁忌:★ 絕對不要放在標頭檔裡 ★
//   原因來自它的定義 —— 每個「翻譯單元」都會得到一份自己的副本。
//   若把它寫進標頭檔,那麼:
//     - 每個 #include 該標頭的 .cpp 都會擁有一份「各自獨立」的
//       internalCounter,彼此不是同一個變數。
//     - a.cpp 把它改成 10,b.cpp 讀到的還是 0 —— 而且完全不會有任何
//       編譯或連結錯誤,程式就只是「行為不對」。
//   這種 bug 極難追查,因為所有工具都認為程式碼是正確的。
//   同理,若標頭中的 inline 函式使用了匿名命名空間裡的東西,
//   會違反單一定義規則(ODR),屬於未定義行為。
//
// 【注意事項 Pay Attention】
//   1. 匿名命名空間只能出現在 .cpp。標頭檔請改用 inline 變數(C++17)
//      或 constexpr 常數。
//   2. 別忘了裡面的名稱仍在「命名空間範圍」,具有靜態儲存期,
//      程式啟動時初始化、結束時銷毀,和一般全域變數一樣。
//   3. 它不提供任何執行緒安全保證。檔案私有 ≠ 沒有競態 ——
//      本檔的 internalCounter 若被多執行緒寫入,一樣需要同步機制。
//   4. 匿名命名空間可以巢狀在具名命名空間裡:
//      namespace mylib { namespace { void helper(); } }
//      這時 helper 是「mylib 內、且僅此檔案可見」。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】匿名命名空間
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 匿名命名空間和 static 都能讓名稱只在本檔可見,為什麼 C++ 建議用前者?
//     答：主要是能力差異。static 只能修飾變數和函式,
//         而匿名命名空間還能容納「型別定義」—— class 是不能寫 static 的,
//         所以「讓一個類別成為檔案私有」只有匿名命名空間辦得到。
//         其次是語意清晰:static 在 C++ 有三種不同意思(內部連結、
//         靜態儲存期、類別靜態成員),匿名命名空間只有一種。
//     追問：兩者在符號表上有差別嗎?
//         → 本機 nm 實測:兩者都是小寫的 local symbol(b/t),
//           差別在修飾後的名稱 —— static 是 _ZL9staticVar,
//           匿名命名空間是 _ZN12_GLOBAL__N_1L15internalCounterE,
//           其中 _GLOBAL__N_1 就是編譯器給該翻譯單元生成的唯一名字。
//
// 🔥 Q2. 為什麼匿名命名空間裡的東西不用寫前綴就能直接使用?
//     答：因為它的語意等於「宣告一個名字唯一的命名空間,
//         再自動補一行 using namespace 那個唯一名字;」。
//         那行隱含的 using 指示讓成員在本檔可以非限定存取,
//         而外部因為拼不出那個唯一名字,永遠無法存取。
//     追問：那它會不會像 using namespace std 一樣造成名稱汙染?
//         → 範圍完全不同。它只引入「你自己剛寫的那幾個名稱」,
//           數量與內容都在你掌控中;using namespace std 引入的是
//           數千個你沒寫過、也不知道有哪些的名稱。
//
// ⚠️ 陷阱. 我把匿名命名空間寫進標頭檔,想讓每個引入者都有個私有計數器。
//         編譯連結都成功,為什麼 a.cpp 加到 10,b.cpp 讀出來還是 0?
//     答：因為「每個翻譯單元各得一份獨立副本」正是匿名命名空間的定義。
//         標頭被 include 到 N 個 .cpp,就產生 N 個彼此無關的變數,
//         它們同名但不是同一個物件。因為沒有任何符號衝突,
//         編譯與連結都不會報錯 —— 程式只是行為不對。
//     為什麼會錯：把匿名命名空間想成「加了存取控制的全域變數」,
//         以為它仍然是「一個」變數,只是被保護起來。
//         實際上它改變的是「有幾份」,不是「誰能碰」。
//         要在標頭中提供唯一的共享變數,C++17 起請用 inline 變數。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【本檔不加 LeetCode 範例的理由】
//   匿名命名空間是連結期的可見性機制,與演算法及複雜度無關;
//   而且它的效果需要「多個 .cpp」才看得出來,單檔的 LeetCode 提交
//   根本無從體現。下方改以單檔內可觀察的實務結構來示範。

#include <iostream>
#include <string>

// 匿名命名空間
namespace {
    int internalCounter = 0;  // 只在這個檔案內可見

    void internalFunction() {
        std::cout << "這是內部函數" << std::endl;
    }

    // -------------------------------------------------------------------------
    // 【日常實務範例】把「只有這個 .cpp 用得到的實作細節」全部關進來
    // 情境：一個設定檔解析模組。對外只公開 parseConfigLine()（宣告在標頭檔），
    //       但實作過程需要一個修剪空白的輔助函式、一個統計用的計數器，
    //       以及一個小型的結果結構。這三者都不該出現在全域符號表：
    //         - trim()      名字太通用，很容易和別的模組撞名
    //         - s_parsedCount  是本模組的內部統計
    //         - ParsedEntry 是實作細節，對外只需要知道 key/value
    //       ★ 注意 ParsedEntry 是「型別」——這正是 static 辦不到、
    //         只有匿名命名空間能做到的事。
    // -------------------------------------------------------------------------
    struct ParsedEntry {          // 型別也可以檔案私有（static 無法做到這件事）
        std::string key;
        std::string value;
        bool ok = false;
    };

    int s_parsedCount = 0;        // 模組內部統計，不對外公開

    std::string trim(const std::string& s) {   // 通用名字，關在檔案內才安全
        const std::string spaces = " \t\r\n";
        const auto b = s.find_first_not_of(spaces);
        if (b == std::string::npos) return "";
        const auto e = s.find_last_not_of(spaces);
        return s.substr(b, e - b + 1);
    }

    ParsedEntry parseLine(const std::string& line) {
        ParsedEntry entry;
        const std::string t = trim(line);
        if (t.empty() || t[0] == '#') return entry;   // 空行與註解直接略過
        const auto eq = t.find('=');
        if (eq == std::string::npos) return entry;    // 沒有等號 → 格式錯誤
        entry.key   = trim(t.substr(0, eq));
        entry.value = trim(t.substr(eq + 1));
        entry.ok    = !entry.key.empty();
        if (entry.ok) ++s_parsedCount;
        return entry;
    }
}  // namespace（匿名）

int main() {
    internalCounter = 10;  // 可以直接使用，不需前綴
    internalFunction();
    std::cout << "internalCounter = " << internalCounter << "\n";

    // ── 實務：解析設定檔 ────────────────────────────────────────────
    std::cout << "\n=== 設定檔解析（實作細節全在匿名命名空間內）===\n";
    const std::string lines[] = {
        "  host = 192.168.1.10  ",
        "# 這一行是註解",
        "port=8080",
        "",
        "這行沒有等號",
        "  timeout_ms =  3000"
    };

    for (const auto& line : lines) {
        const ParsedEntry e = parseLine(line);
        if (e.ok) std::cout << "  OK   key=[" << e.key << "] value=[" << e.value << "]\n";
        else      std::cout << "  略過 原始行=[" << line << "]\n";
    }
    std::cout << "  成功解析筆數 = " << s_parsedCount << "\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 4 課：命名空間（namespace）基礎6.cpp" -o ns6

// ─────────────────────────────────────────────────────────────────────────────
// 【輸出但書】
//  1. 「略過 原始行=[]」那一行對應輸入陣列裡的空字串,不是輸出缺漏。
//  2. 「略過 原始行=[# 這一行是註解]」印的是 trim 之前的原始行,
//     所以看得到原始的前後空白狀態。
//  3. 本檔關於連結屬性的說法皆為本機實測(g++ 15.2.0,以 nm 觀察目的檔):
//         int globalVar;             → B globalVar                 (大寫 = 外部連結)
//         void globalFunc();         → T _Z10globalFuncv           (大寫 = 外部連結)
//         static int staticVar;      → b _ZL9staticVar             (小寫 = 內部連結)
//         namespace{ int  internalCounter;  } → b _ZN12_GLOBAL__N_1L15internalCounterE
//         namespace{ void internalFunction();} → t _ZN12_GLOBAL__N_1L16internalFunctionEv
//     _GLOBAL__N_1 即編譯器為匿名命名空間生成的唯一名字。
//     名稱修飾屬 Itanium ABI(g++/clang),MSVC 的編碼完全不同。
//  4. 匿名命名空間「每個翻譯單元各一份」的效果需要多個 .cpp 才觀察得到,
//     本檔為單檔範例,故該性質只在上方以文字說明,未在輸出中呈現。
//  5. 以下為本機 g++ 15.2.0 (Ubuntu 26.04) 連續執行 3 次、逐位元組相同的結果。
// ─────────────────────────────────────────────────────────────────────────────

// === 預期輸出 ===
// 這是內部函數
// internalCounter = 10
//
// === 設定檔解析（實作細節全在匿名命名空間內）===
//   OK   key=[host] value=[192.168.1.10]
//   略過 原始行=[# 這一行是註解]
//   OK   key=[port] value=[8080]
//   略過 原始行=[]
//   略過 原始行=[這行沒有等號]
//   OK   key=[timeout_ms] value=[3000]
//   成功解析筆數 = 3
