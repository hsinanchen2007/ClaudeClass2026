// =============================================================================
//  summary.cpp（第 4 課 總複習） — 命名空間:C++ 用來切割名稱世界的唯一工具
// =============================================================================
//
// 【主題資訊 Information】
//   定義      ：namespace 名稱 { 宣告... }                       // C++98
//   巢狀(傳統)：namespace a { namespace b { ... } }              // C++98
//   巢狀(簡寫)：namespace a::b { ... }                           // ★ C++17
//   巢狀+inline：namespace a::inline b { ... }                   // ★ C++20
//   限定存取  ：a::b::member
//   using 宣告：using 命名空間::成員;                             // C++98
//   using 指示：using namespace 命名空間;                         // C++98
//   別名      ：namespace 別名 = 原命名空間;                      // C++98
//   匿名      ：namespace { ... }                                // C++98
//   標頭檔    ：無須額外引入,namespace 是語言關鍵字。
//   複雜度    ：全部零執行期成本。命名空間只影響「編譯期的名稱查找」
//               與「連結期的符號修飾」,不產生任何物件或間接層。
//
//   ★ 標準版本已用 -pedantic-errors 實測驗證(見末尾但書),不是憑印象:
//     namespace a::b        在 -std=c++14 下明確報錯,需 C++17。
//     namespace a::inline b 在 -std=c++17 下明確報錯,需 C++20。
//
// 【詳細解釋 Explanation】
//
// 【1. 命名空間要解決的問題:全域只有一個,但函式庫有很多個】
//   C 語言只有一個全域名稱空間。你的 draw() 和圖形函式庫的 draw()
//   在連結期會直接撞成 multiple definition,而且無解 —— 只能改名。
//   業界的 C 慣例是靠前綴硬撐:gl_draw、SDL_draw、cv_draw。
//   C++ 把這個「前綴」提升成語言機制:
//       namespace graphics { void draw(); }
//       namespace logging  { void draw(); }
//   兩者都叫 draw,卻能共存,因為完整名稱不同。而且因為編譯器認得這個結構,
//   你還額外得到:巢狀分層、選擇性引入、取別名、分次擴充 ——
//   這些都是純字串前綴給不了的。
//
// 【2. 三種存取方式,以及該怎麼選】
//   ① 完全限定名稱  math_tools::PI
//      最安全、最明確,讀者一眼看出出處。標頭檔「只能」用這種。
//   ② using 宣告    using string_tools::toUpperCase;
//      白名單,只引入你點名的那一個。是「想少打字」時的正確折衷。
//   ③ using 指示    using namespace math_tools;
//      全開,把整個命名空間倒進目前作用域。最方便也最危險。
//   選擇的判準是「引入的名稱數量」與「作用域大小」的乘積:
//   兩者都要壓到最小。本檔 main 把 ②③ 都關在 { } 區塊內,
//   離開區塊即失效 —— 這正是實務上該有的紀律。
//
// 【3. 巢狀命名空間:用層次反映組織結構】
//   company::graphics::draw() 這種寫法讓命名空間對應到真實的模組邊界。
//   C++17 之前只能一層層寫大括號,巢狀四層就要四對括號與四次縮排;
//   C++17 起可以寫成 namespace company::network { ... },意義完全相同。
//   本檔兩種寫法都示範了:company::graphics/audio 用傳統寫法,
//   company::network 用 C++17 簡寫,而 main 裡的存取方式一模一樣 ——
//   證明兩者產生的結構確實相同,只是語法糖。
//
// 【4. 擴展(reopen):為什麼函式庫能分散在多個標頭檔】
//   同一個命名空間名稱可以被重複開啟,內容累加而非覆蓋。
//   本檔的 mylib 分兩段定義,main 裡 funcA 與 funcB 都能呼叫。
//   這個開放性是模組化的前提:math.h 與 string.h 各自往 mylib 加東西,
//   使用者 include 了哪幾個就看得到哪幾個。
//   ★ 但要注意:累加的是「名稱」,不是 ODR 的豁免權。
//     兩處定義同一個非 inline 函式,仍然是重複定義的連結錯誤。
//
// 【5. 匿名命名空間:檔案私有的正統做法】
//   namespace { ... } 裡的名稱只有本翻譯單元看得到(internal linkage)。
//   它取代了 C 風格的 static,主因是能力更強:static 只能修飾變數與函式,
//   而匿名命名空間還能容納「型別定義」—— class 是不能寫 static 的。
//   本檔的 callCount 與 logCall 即為此例:它們是實作細節,
//   不該出現在全域符號表,也不該讓其他 .cpp 看見。
//
// 【概念補充 Concept Deep Dive】
//
// (A) ADL:即使你不寫任何 using,std 仍可能參與重載解析
//   引數依賴查找(Argument-Dependent Lookup)規定:呼叫非限定函式時,
//   引數型別所屬的命名空間也會被納入查找範圍。
//   這就是為什麼 std::string s; std::cout << s; 能運作 ——
//   operator<< 是在 std 裡找到的,而你從未寫過 using namespace std。
//   推論:「完全不引入 std」並不代表 std 的名稱永遠不會被考慮。
//   ADL 也是「using std::swap; swap(a, b);」這個慣用法的基礎。
//
// (B) 名稱修飾(name mangling):命名空間在連結期的實體
//   命名空間會被編碼進符號名稱。本機以 nm 實測(g++ 15.2.0,Itanium ABI):
//       math_utils::add(int,int)       → T _ZN10math_utils3addEii
//       namespace 中的 const double PI → r _ZN10math_utilsL2PIE
//       匿名命名空間的成員             → t _ZN12_GLOBAL__N_1L16internalFunctionEv
//   大寫代表外部連結、小寫代表內部連結;_GLOBAL__N_1 就是編譯器
//   替匿名命名空間生成的唯一名字。這正是「不會撞名」的實際機制。
//
// (C) inline namespace(C++11):函式庫的版本控管
//       namespace mylib {
//           inline namespace v2 { void encode(); }   // 預設版本
//                  namespace v1 { void encode(); }   // 舊版仍可明確指定
//       }
//   mylib::encode() 自動解析到 v2,mylib::v1::encode() 取得舊版。
//   升級只需移動 inline 關鍵字,呼叫端一行都不用改;
//   同時符號帶上版本資訊,新舊版本不會在連結期被誤混。
//
// (D) 命名空間範圍的 const 具有 internal linkage
//   這是 C++ 與 C 少數不相容之處。C++ 中 const double PI = 3.14;
//   寫在標頭檔不會造成重複定義(每個 .cpp 各一份);C 則會撞。
//   要在標頭中提供「唯一一份」的變數,C++17 起請用 inline 變數。
//
// 【注意事項 Pay Attention】
//   1. ★ 標頭檔的全域範圍,絕對不可出現 using(宣告與指示皆然)。
//      #include 的本質是文字貼上,那一行會出現在每一個引入者的全域範圍,
//      而他們完全不知情。這是實務上最難追查的名稱汙染來源。
//   2. using 指示造成的衝突,錯誤報在「使用處」而非「宣告處」。
//      本機實測:全域 int count 搭配 using namespace std 與 <algorithm>,
//      宣告行完全不報錯,直到使用 count 的那一行才爆
//      error: reference to 'count' is ambiguous。
//   3. 命名空間名稱打錯不會報錯,只會安靜地新建一個命名空間。
//      實測錯誤訊息出現在呼叫端:
//      error: 'b' is not a member of 'mylib'; did you mean 'mylibb::b'?
//   4. 匿名命名空間絕不可放進標頭檔:每個翻譯單元會各得一份「獨立」副本,
//      a.cpp 改了值 b.cpp 讀不到,而且編譯連結全部成功,只是行為不對。
//   5. 不要往 namespace std 新增宣告。除了標準明文允許的樣板特化
//      (例如為自訂型別特化 std::hash)之外,一律是未定義行為。
//   6. 命名空間不是存取控制。它沒有 private,任何人都能再開啟它加東西。
//      要隱藏實作細節,請用匿名命名空間或 detail 子命名空間的約定。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】命名空間
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼不能在標頭檔裡寫 using namespace std;?
//     答：因為 #include 的本質是文字展開。那一行會被貼進每一個引入該標頭的
//         .cpp 的全域範圍,效力涵蓋該檔「引入之後」的所有程式碼。
//         於是別人的全域 count、size、data 突然變成歧義,
//         而編譯錯誤指向他自己的檔案,他幾乎不可能想到肇因是你的標頭。
//         標頭檔只能用完全限定名稱。
//     追問：那寫在 .cpp 的全域範圍可以嗎?
//         → 小型單檔程式可接受,影響僅限該檔、風險自負。但更好的做法是
//           把它關進函式,或改用 using 宣告只引入需要的名稱。
//
// 🔥 Q2. 命名空間有執行期成本嗎?編譯後它變成什麼?
//     答：零執行期成本。它只影響編譯期的名稱查找,以及連結期的符號修飾。
//         本機 nm 實測:math_utils::add(int,int) 的符號是
//         _ZN10math_utils3addEii,其中 10math_utils 是命名空間名稱與長度。
//         沒有任何額外的物件、指標或間接呼叫被產生。
//     追問：那 namespace 和 class 的 static 成員函式該怎麼選?
//         → 只是要把一批自由函式分組就用 namespace(可分次擴充、可取別名);
//           需要存取控制、需要實體化或繼承,才用 class。
//           用 class 純粹當「函式容器」是 Java 帶來的習慣,在 C++ 並不必要。
//
// 🔥 Q3. 匿名命名空間和 static 都能做到檔案私有,差別在哪?
//     答：能力差異。static 只能修飾變數和函式;匿名命名空間還能容納
//         「型別定義」—— class 不能寫 static,所以「讓一個類別成為檔案私有」
//         只有匿名命名空間辦得到。其次是語意單一:static 在 C++ 有三種意思
//         (內部連結、靜態儲存期、類別靜態成員),匿名命名空間只有一種。
//     追問：符號表上看得出差別嗎?
//         → 兩者都是 local symbol(nm 顯示小寫),差在修飾後的名稱:
//           static 是 _ZL9staticVar,匿名命名空間是
//           _ZN12_GLOBAL__N_1L15internalCounterE。
//
// ⚠️ 陷阱 1. 我寫了 using namespace std; 又宣告全域 int count = 0;,
//          編譯器在宣告那行沒報錯 —— 是不是代表安全?
//     答：不是。實測(g++ 15.2.0 + <algorithm>)顯示宣告行確實不報錯,
//         但只要有任何一處使用非限定的 count,那一行就會爆
//         error: reference to 'count' is ambiguous。
//         using 指示不是把名稱複製進來,而是讓查找時「也去看」std,
//         所以衝突要等到查找真正發生時才被偵測到。
//     為什麼會錯：把「目前編得過」當成安全證明。using 指示埋下的是
//         延遲引爆的地雷 —— 可能在你新增一行程式碼、或升級標準時才觸發
//         (C++17 新增了 std::size,C++20 新增了大量 ranges 名稱)。
//
// ⚠️ 陷阱 2. 為了讓每個檔案都有自己的私有計數器,我把匿名命名空間寫進標頭檔。
//          編譯連結都成功,為什麼 a.cpp 加到 10、b.cpp 讀出來還是 0?
//     答：因為「每個翻譯單元各得一份獨立副本」正是匿名命名空間的定義。
//         標頭被 include 到 N 個 .cpp,就產生 N 個彼此無關的變數。
//         沒有符號衝突,所以編譯與連結都不會有任何警告 —— 程式只是行為不對。
//     為什麼會錯：把匿名命名空間想成「加了保護的全域變數」,
//         以為它仍是「一個」變數。它改變的是「有幾份」,不是「誰能碰」。
//         標頭中要提供唯一的共享變數,C++17 起請用 inline 變數。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【本檔不加 LeetCode 範例的理由】
//   命名空間是「程式碼組織與連結可見性」的機制,不影響任何演算法的
//   時間或空間複雜度,與 LeetCode 的評量面向沒有交集。
//   附帶一提:LeetCode 提交範本裡的 using namespace std; 正是這個寫法
//   最合理的使用場景 —— 單檔、生命週期只有幾分鐘、沒有人需要維護。
//   把同樣的習慣帶進正式專案,才是本課要提醒的問題。

#include <iostream>
#include <string>

// ================================================================
// ===== 重點一：為何需要命名空間？=====
// 說明：當多個函式庫都定義了相同名稱的函式或變數時，就會
//       發生「名稱衝突（name collision）」，編譯器無法區分。
//       C 語言的做法是在名稱前加前綴（如 math_add、string_add），
//       C++ 提供了更優雅的解決方案：命名空間。
// 重要性：是大型專案和多函式庫協作的基礎機制。
//
// 示意（若沒有命名空間）：
//   void draw();  // 圖形庫的 draw
//   void draw();  // 日誌庫的 draw  ← 衝突！編譯錯誤
//
// 有了命名空間：
//   namespace graphics { void draw(); }
//   namespace logging  { void draw(); }
//   graphics::draw();  // 明確！
//   logging::draw();   // 明確！
// ================================================================


// ===== 重點二：定義命名空間 =====
// 語法：namespace 名稱 { 變數、函式、類別... }
// 重要性：將相關的識別符號（變數、函式、型別）組織在同一個
//         邏輯命名空間下，避免與外部名稱衝突。

namespace math_tools {
    // 命名空間內可以放常數
    const double PI = 3.14159265358979;
    const double E  = 2.71828182845905;

    // 命名空間內可以放函式
    double circleArea(double radius) {
        return PI * radius * radius;
    }

    double circleCircumference(double radius) {
        return 2.0 * PI * radius;
    }

    // 使用 int 參數的簡單冪次計算（避免引入 cmath 的 pow）
    int power(int base, int exp) {
        int result = 1;
        for (int i = 0; i < exp; i++) {
            result *= base;
        }
        return result;
    }
}

// ===== 重點三：字串工具命名空間（展示不同命名空間隔離） =====
namespace string_tools {
    // 即使函式名稱與其他地方相同也不衝突
    std::string toUpperCase(const std::string& str) {
        std::string result = str;
        for (char& c : result) {
            if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
        }
        return result;
    }

    std::string toLowerCase(const std::string& str) {
        std::string result = str;
        for (char& c : result) {
            if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
        }
        return result;
    }

    std::string repeat(const std::string& str, int times) {
        std::string result;
        for (int i = 0; i < times; i++) result += str;
        return result;
    }
}

// ===== 重點四：巢狀命名空間（Nested Namespace） =====
// 說明：命名空間可以嵌套，模擬公司/專案/模組的層次結構。
// 傳統寫法（C++11/14）：
//   namespace company { namespace graphics { void draw(); } }
// C++17 簡化寫法：
//   namespace company::graphics { void draw(); }
// 重要性：讓大型專案的命名空間層次結構更清晰。

namespace company {
    namespace graphics {
        void draw() {
            std::cout << "  [Graphics] 繪製圖形\n";
        }
        void clear() {
            std::cout << "  [Graphics] 清除畫面\n";
        }
    }
    namespace audio {
        void play() {
            std::cout << "  [Audio] 播放音效\n";
        }
        void stop() {
            std::cout << "  [Audio] 停止播放\n";
        }
    }
}

// C++17 巢狀命名空間簡化寫法（等同於上面的巢狀寫法）
namespace company::network {
    void connect() {
        std::cout << "  [Network] 建立連線\n";
    }
}

// ===== 重點五：擴展命名空間 =====
// 說明：同一個命名空間名稱可以在多個地方出現，編譯器會自動合併。
//       這在多檔案專案中非常常見：
//       - math.h    定義 namespace mylib { 數學函式 }
//       - string.h  定義 namespace mylib { 字串函式 }
//       它們共同組成完整的 mylib 命名空間。
// 重要性：讓大型函式庫可以分散在多個標頭檔中，但共用同一個命名空間。

namespace mylib {
    void funcA() { std::cout << "  mylib::funcA() 已呼叫\n"; }
}

namespace mylib {  // 擴展：繼續在 mylib 中加入新成員
    void funcB() { std::cout << "  mylib::funcB() 已呼叫\n"; }
}

// ===== 重點六：匿名命名空間 =====
// 說明：沒有名稱的命名空間，其成員只在「當前翻譯單元（.cpp 檔）」內可見。
//       等效於 C 語言的 static，但 C++ 更推薦使用匿名命名空間。
// 重要性：隱藏檔案內部實作細節，防止連結時與其他檔案的同名符號衝突。
// 使用時機：輔助函式、檔案內部計數器、只在本檔案用到的工具函式。

namespace {
    int callCount = 0;  // 只在本檔案內可見

    void logCall(const std::string& funcName) {
        callCount++;
        std::cout << "  [Log #" << callCount << "] 呼叫: " << funcName << "\n";
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例】兩個第三方函式庫撞名 —— 命名空間存在的原始理由
// 情境：專案同時整合了兩套 SDK：
//         - 金流廠商的 SDK：有 connect() / send() / Status
//         - 訊息推播的 SDK：也有 connect() / send() / Status
//       在 C 語言裡這是死局，只能請其中一方改名（或自己 fork 改前綴）。
//       C++ 讓兩者各自待在自己的命名空間，完整名稱不同就能共存，
//       而且呼叫端一眼看得出這一行在跟誰講話——這比 pay_connect() 這種
//       字串前綴更好，因為編譯器認得這個結構（可巢狀、可別名、可選擇性引入）。
// -----------------------------------------------------------------------------
namespace payment_sdk {
    struct Status { int code; std::string message; };

    Status connect(const std::string& endpoint) {
        return Status{200, "payment 已連線 " + endpoint};
    }
    Status send(double amount) {
        return Status{201, "已送出付款 " + std::to_string(static_cast<long long>(amount)) + " 元"};
    }
}

namespace push_sdk {
    // 同名的 Status／connect／send —— 與 payment_sdk 完全不衝突
    struct Status { int code; bool delivered; };

    Status connect(const std::string& endpoint) {
        std::cout << "  [push] 連線至 " << endpoint << "\n";
        return Status{0, false};
    }
    Status send(const std::string& title) {
        std::cout << "  [push] 推播標題「" << title << "」\n";
        return Status{0, true};
    }
}

// ================================================================
// main() 展示所有重點
// ================================================================
int main() {
    std::cout << "================================================================\n";
    std::cout << "  第4課：命名空間（namespace）基礎 總複習\n";
    std::cout << "================================================================\n\n";

    // ------------------------------------------------------------------
    // 重點二 示範：使用完全限定名稱（:: 運算子）
    // ------------------------------------------------------------------
    // 說明：「命名空間::成員」的完整形式稱為「完全限定名稱」。
    //       這是最安全、最推薦的使用方式，意圖明確，不會引起歧義。
    // 缺點：程式碼較長，但在大型專案中這是值得的代價。
    std::cout << "===== [存取方式一] 完全限定名稱（Fully Qualified Name） =====\n";
    std::cout << "  math_tools::PI            = " << math_tools::PI << "\n";
    std::cout << "  math_tools::E             = " << math_tools::E << "\n";
    std::cout << "  math_tools::circleArea(5) = " << math_tools::circleArea(5) << "\n";
    std::cout << "  math_tools::power(2, 10)  = " << math_tools::power(2, 10) << "\n";
    std::cout << "\n";

    // ------------------------------------------------------------------
    // 重點三 示範：using 宣告（引入特定成員）
    // ------------------------------------------------------------------
    // 說明：using 命名空間::成員;
    //       只將指定的一個成員引入目前作用域，其他成員仍需完整名稱。
    // 優點：比全域 using namespace 更安全，只引入真正需要的名稱。
    // 建議：在函式內部使用，避免污染全域命名空間。
    std::cout << "===== [存取方式二] using 宣告（引入特定成員） =====\n";
    {
        using string_tools::toUpperCase;
        using string_tools::toLowerCase;

        std::string text = "Hello World";
        std::cout << "  原始:    " << text << "\n";
        std::cout << "  toUpperCase: " << toUpperCase(text) << "\n";
        std::cout << "  toLowerCase: " << toLowerCase(text) << "\n";

        // repeat 沒有用 using，仍需完整名稱
        std::cout << "  repeat(\"Hi\", 3): " << string_tools::repeat("Hi", 3) << "\n";
    }
    // 離開大括號後，toUpperCase / toLowerCase 的 using 宣告失效
    std::cout << "\n";

    // ------------------------------------------------------------------
    // 重點三 示範：using 指令（引入整個命名空間）
    // ------------------------------------------------------------------
    // 說明：using namespace 名稱;
    //       將整個命名空間的所有成員引入目前作用域。
    // 缺點：可能造成名稱衝突，在標頭檔（.h）中絕對禁止使用！
    // 安全用法：限制在函式內部或區塊內部，縮小影響範圍。
    std::cout << "===== [存取方式三] using 指令（僅在區塊內使用） =====\n";
    {
        // 只在這個 {} 區塊內有效，出了區塊就失效
        using namespace math_tools;

        std::cout << "  在區塊內直接使用 PI: " << PI << "\n";
        std::cout << "  在區塊內直接使用 circleArea(3): " << circleArea(3) << "\n";
    }
    // 離開區塊後，又必須使用完整名稱
    std::cout << "  離開區塊後需完整名稱: math_tools::PI = " << math_tools::PI << "\n";
    std::cout << "\n";

    // ------------------------------------------------------------------
    // 重點四 示範：命名空間別名
    // ------------------------------------------------------------------
    // 說明：namespace 別名 = 原命名空間;
    //       為名稱很長的命名空間建立一個簡短別名。
    // 重要性：常用於簡化 std::filesystem、boost::asio 等長命名空間。
    //         原始名稱仍然完全有效。
    std::cout << "===== [重點四] 命名空間別名 =====\n";
    {
        namespace mt = math_tools;       // 為 math_tools 建立別名 mt
        namespace st = string_tools;     // 為 string_tools 建立別名 st
        namespace gfx = company::graphics;  // 為巢狀命名空間建立別名

        std::cout << "  mt::PI = " << mt::PI << "\n";
        std::cout << "  st::toUpperCase(\"alias\") = " << st::toUpperCase("alias") << "\n";
        gfx::draw();

        // 原名稱仍然有效
        std::cout << "  math_tools::E = " << math_tools::E << "（原名稱仍有效）\n";
    }
    std::cout << "\n";

    // ------------------------------------------------------------------
    // 重點四 示範：巢狀命名空間存取
    // ------------------------------------------------------------------
    std::cout << "===== [重點四] 巢狀命名空間 =====\n";
    company::graphics::draw();
    company::graphics::clear();
    company::audio::play();
    company::audio::stop();
    company::network::connect();  // C++17 簡化寫法定義的

    {
        // 使用別名簡化巢狀命名空間的存取
        namespace sfx = company::audio;
        std::cout << "  使用別名 sfx：\n";
        sfx::play();
    }
    std::cout << "\n";

    // ------------------------------------------------------------------
    // 重點五 示範：擴展命名空間
    // ------------------------------------------------------------------
    std::cout << "===== [重點五] 擴展命名空間 =====\n";
    // funcA 和 funcB 都屬於 mylib，即使定義在不同地方
    mylib::funcA();
    mylib::funcB();
    std::cout << "\n";

    // ------------------------------------------------------------------
    // 重點六 示範：匿名命名空間
    // ------------------------------------------------------------------
    std::cout << "===== [重點六] 匿名命名空間 =====\n";
    // callCount 和 logCall 定義在匿名命名空間，直接使用（無需前綴）
    logCall("math_tools::circleArea");
    logCall("string_tools::toUpperCase");
    logCall("mylib::funcA");
    std::cout << "  總呼叫次數: " << callCount << "\n";
    std::cout << "\n";

    // ------------------------------------------------------------------
    // 日常實務：兩套 SDK 撞名，靠命名空間共存
    // ------------------------------------------------------------------
    std::cout << "===== [日常實務] 兩套 SDK 都有 connect/send/Status =====\n";
    {
        // 兩個 Status 是「完全不同的型別」，各自有不同的成員
        payment_sdk::Status ps = payment_sdk::connect("api.pay.example.com");
        std::cout << "  payment: code=" << ps.code << " msg=" << ps.message << "\n";

        payment_sdk::Status pay = payment_sdk::send(1250);
        std::cout << "  payment: code=" << pay.code << " msg=" << pay.message << "\n";

        push_sdk::Status us = push_sdk::connect("push.example.com");
        std::cout << "  push:    code=" << us.code
                  << " delivered=" << (us.delivered ? "true" : "false") << "\n";

        push_sdk::Status pushed = push_sdk::send("訂單已成立");
        std::cout << "  push:    code=" << pushed.code
                  << " delivered=" << (pushed.delivered ? "true" : "false") << "\n";

        // 用別名讓長名稱變短，但仍保留「看得出出處」的前綴
        namespace pay_api = payment_sdk;
        std::cout << "  用別名呼叫: " << pay_api::connect("backup.pay.example.com").message << "\n";
        std::cout << "  → 若沒有命名空間，這兩套 SDK 只能有一套活下來\n";
    }
    std::cout << "\n";

    // ------------------------------------------------------------------
    // 重點七：最佳實踐總結
    // ------------------------------------------------------------------
    std::cout << "===== [重點七] 最佳實踐速查 =====\n";
    std::cout << "  標頭檔（.h）中：絕對不能使用 using namespace！\n";
    std::cout << "  大型專案：    優先使用完全限定名稱（std::cout）\n";
    std::cout << "  局部簡化：    在函式/區塊內用 using 宣告（引入特定成員）\n";
    std::cout << "  長命名空間：  用 namespace 別名 = 原命名空間\n";
    std::cout << "  檔案內部：    使用匿名命名空間取代 static\n";
    std::cout << "  C++17 巢狀：  namespace a::b::c { } 簡化寫法\n";

    std::cout << "\n================================================================\n";
    std::cout << "  總複習完成！\n";
    std::cout << "================================================================\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary
//   ★ 必須是 C++17 或更新:本檔用了 namespace company::network { } 這個
//     C++17 才有的巢狀簡寫。以 -std=c++14 -pedantic-errors 編譯會直接失敗。

// ─────────────────────────────────────────────────────────────────────────────
// 【輸出但書】
//  1. 標準版本的說法皆以 -pedantic-errors 實測驗證(g++ 15.2.0),非憑印象:
//       -std=c++14 編 namespace a::b { }
//         → error: nested namespace definitions only available with '-std=c++17'
//       -std=c++17 編 namespace a::inline b { }
//         → error: nested inline namespace definitions only available with '-std=c++20'
//     ——這也是為什麼驗證標準版本必須用 -pedantic-errors:
//       只用 -fsyntax-only 或不加 -pedantic-errors 時,GCC 會把它當擴充放行,
//       看起來「舊標準也支援」,實際上並不可攜。
//  2. math_tools::PI 印出 3.14159、circleArea(5) 印出 78.5398,
//     是 std::cout 預設 6 位有效數字的結果,不是數值被截斷。
//  3. payment_sdk::Status 與 push_sdk::Status 是兩個「完全不同的型別」,
//     成員也不同(前者有 message,後者有 delivered)。它們同名而不衝突,
//     正是命名空間存在的原始理由。
//  4. 匿名命名空間的 callCount 在本檔內累加到 3。它「每個翻譯單元各一份」的
//     特性需要多個 .cpp 才觀察得到,本檔為單檔範例,故僅以文字說明。
//  5. 以下為本機 g++ 15.2.0 (Ubuntu 26.04) 連續執行 3 次、逐位元組相同的結果。
// ─────────────────────────────────────────────────────────────────────────────

// === 預期輸出 ===
// ================================================================
//   第4課：命名空間（namespace）基礎 總複習
// ================================================================
//
// ===== [存取方式一] 完全限定名稱（Fully Qualified Name） =====
//   math_tools::PI            = 3.14159
//   math_tools::E             = 2.71828
//   math_tools::circleArea(5) = 78.5398
//   math_tools::power(2, 10)  = 1024
//
// ===== [存取方式二] using 宣告（引入特定成員） =====
//   原始:    Hello World
//   toUpperCase: HELLO WORLD
//   toLowerCase: hello world
//   repeat("Hi", 3): HiHiHi
//
// ===== [存取方式三] using 指令（僅在區塊內使用） =====
//   在區塊內直接使用 PI: 3.14159
//   在區塊內直接使用 circleArea(3): 28.2743
//   離開區塊後需完整名稱: math_tools::PI = 3.14159
//
// ===== [重點四] 命名空間別名 =====
//   mt::PI = 3.14159
//   st::toUpperCase("alias") = ALIAS
//   [Graphics] 繪製圖形
//   math_tools::E = 2.71828（原名稱仍有效）
//
// ===== [重點四] 巢狀命名空間 =====
//   [Graphics] 繪製圖形
//   [Graphics] 清除畫面
//   [Audio] 播放音效
//   [Audio] 停止播放
//   [Network] 建立連線
//   使用別名 sfx：
//   [Audio] 播放音效
//
// ===== [重點五] 擴展命名空間 =====
//   mylib::funcA() 已呼叫
//   mylib::funcB() 已呼叫
//
// ===== [重點六] 匿名命名空間 =====
//   [Log #1] 呼叫: math_tools::circleArea
//   [Log #2] 呼叫: string_tools::toUpperCase
//   [Log #3] 呼叫: mylib::funcA
//   總呼叫次數: 3
//
// ===== [日常實務] 兩套 SDK 都有 connect/send/Status =====
//   payment: code=200 msg=payment 已連線 api.pay.example.com
//   payment: code=201 msg=已送出付款 1250 元
//   [push] 連線至 push.example.com
//   push:    code=0 delivered=false
//   [push] 推播標題「訂單已成立」
//   push:    code=0 delivered=true
//   用別名呼叫: payment 已連線 backup.pay.example.com
//   → 若沒有命名空間，這兩套 SDK 只能有一套活下來
//
// ===== [重點七] 最佳實踐速查 =====
//   標頭檔（.h）中：絕對不能使用 using namespace！
//   大型專案：    優先使用完全限定名稱（std::cout）
//   局部簡化：    在函式/區塊內用 using 宣告（引入特定成員）
//   長命名空間：  用 namespace 別名 = 原命名空間
//   檔案內部：    使用匿名命名空間取代 static
//   C++17 巢狀：  namespace a::b::c { } 簡化寫法
//
// ================================================================
//   總複習完成！
// ================================================================
