// =============================================================================
//  第 2 課：C 與 C++ 的關鍵差異 10  —  函數重載（Function Overloading）
// =============================================================================
//
// 【主題資訊 Information】
//   語法：同一個作用域內，允許多個同名函式，只要「參數列」不同即可。
//         int    add(int, int);
//         double add(double, double);
//         int    add(int, int, int);
//   標準版本：C++98 起即有；C 語言（含 C23）完全不支援。
//   標頭檔：本檔用 <iostream>。
//   複雜度：重載解析（overload resolution）完全發生在「編譯期」，
//           執行期成本為零——最終呼叫的就是一個普通的直接呼叫。
//
// 【詳細解釋 Explanation】
//
// 【1. C 為什麼做不到？關鍵在符號名稱（symbol name）】
//   C 的連結模型極為簡單：函式 add 在目標檔（.o）裡的符號就叫 `add`。
//   一個符號名只能對應一個定義，所以兩個 add 必然衝突（duplicate symbol）。
//   C 程式設計師只好手動替名字加後綴：addi / addd / add3，
//   或者用巨集耍花招。標準函式庫就是這樣做的——
//       fabs / fabsf / fabsl        （double / float / long double）
//       abs / labs / llabs          （int / long / long long）
//   這串命名混亂正是「沒有重載」留下的疤痕。
//
// 【2. C++ 的解法：Name Mangling（名稱修飾）】
//   C++ 編譯器會把「參數型別」編碼進符號名稱。以 Itanium C++ ABI
//   （Linux/gcc/clang 採用）為例，本檔三個函式的實際符號是：
//       int    add(int, int)        →  _Z3addii
//       double add(double, double)  →  _Z3adddd
//       int    add(int, int, int)   →  _Z3addiii
//   規則拆解：_Z 是 C++ 符號前綴，3 是名字長度，add 是名字，
//             之後每個字母代表一個參數型別（i = int，d = double）。
//   你可以用 `nm -C` 或 `c++filt` 把它還原回可讀形式。
//   ★ 重要推論：因為「回傳型別沒有被編碼進去」，所以
//     「只有回傳型別不同」的兩個函式無法重載——編譯器會直接報錯，
//     因為它們會產生一模一樣的符號。
//
// 【3. 重載解析的三個階段】
//   編譯器看到 add(3, 5) 時，做的是：
//     (a) 名稱查找（name lookup）：找出所有叫 add 的候選函式。
//     (b) 篩選可行候選（viable functions）：參數個數對得上、
//         每個引數都存在可行的隱式轉換。
//     (c) 挑最佳者（best viable function）：逐一比較每個引數的
//         「轉換等級」，必須在所有引數上都不比別人差、且至少一個更好。
//   轉換等級由好到壞大致是：
//         完全符合  >  型別提升(promotion)  >  標準轉換  >  使用者定義轉換  >  ...
//   若找不到唯一最佳者，就是 ambiguous（歧義），編譯失敗——
//   注意這是「編譯錯誤」，不是執行期挑錯人。
//
// 【4. 為什麼 add(3.14, 2.86) 會選 double 版本】
//   字面值 3.14 的型別本來就是 double，因此 double 版是「完全符合」，
//   而 int 版需要 double→int 的窄化轉換（等級差很多），勝負分明。
//   反過來 add(3, 5)：int 版完全符合，double 版需要 int→double 提升，
//   所以選 int 版。
//
// 【概念補充 Concept Deep Dive】
//   ● extern "C" 會關閉 name mangling。
//     當你要把 C++ 函式提供給 C 呼叫時，得寫 extern "C"，
//     此時符號名回到樸素的 `add`——代價是：該函式不能被重載。
//     這正好反證了「重載能力來自 mangling」。
//   ● 重載 ≠ 覆寫（override）。
//     重載是同一作用域、編譯期、看參數；
//     覆寫是基底/衍生類別之間、執行期、看虛擬表。兩者常被混為一談。
//   ● 名稱遮蔽（name hiding）：衍生類別只要宣告了同名函式，
//     基底類別「所有」同名重載都會被藏起來，即使參數完全不同。
//     解法是在衍生類別寫 `using Base::add;` 把它們拉回候選集合。
//   ● 重載對執行期效能沒有任何負擔，也不會產生虛擬表或間接跳躍。
//
// 【注意事項 Pay Attention】
//   1. 「只有回傳型別不同」無法重載（符號會撞名），這是最常考的規則。
//   2. 頂層 const 不構成重載：f(int) 與 f(const int) 是同一個函式；
//      但 f(int&) 與 f(const int&)、f(int*) 與 f(const int*) 是不同的重載。
//   3. 重載搭配預設參數容易產生歧義，例如同時有
//         void g(int);
//         void g(int, int = 0);
//      呼叫 g(1) 就是歧義，編譯錯誤。
//   4. 混用 int / unsigned / char 的重載常出現意外選擇，
//      因為整數提升的規則相當細膩；有疑慮就顯式轉型再呼叫。
//   5. 本檔輸出中 add(3.14, 2.86) 印成 6，是因為 iostream 預設精度是
//      6 位有效數字；不代表兩個十進位小數在二進位浮點下加起來剛好是 6。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】函數重載
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++ 如何實作函數重載？C 為什麼辦不到？
//     答：靠 name mangling——編譯器把參數型別編碼進符號名稱，
//         int add(int,int) 變成 _Z3addii、double 版變成 _Z3adddd，
//         兩者在連結器眼中是完全不同的符號，因此可以並存。
//         C 不做修飾，符號就叫 add，兩個定義必然重複而衝突。
//     追問：那 extern "C" 的函式可以重載嗎？
//         → 不行。extern "C" 正是關閉 mangling、退回單一樸素符號名，
//           所以同名只能有一個。
//
// 🔥 Q2. 為什麼「只有回傳型別不同」不能構成重載？
//     答：因為 mangling 只編碼參數型別，不編碼回傳型別，兩者會產生
//         相同的符號名。更根本的原因是：呼叫端寫 `add(1,2);` 而忽略
//         回傳值時，編譯器沒有任何資訊可以決定該選哪一個。
//     追問：那為什麼「參數的 const 修飾」有時算、有時不算？
//         → 頂層 const（如 int vs const int）在函式型別上會被忽略，
//           不算不同；但 const int& / const int* 這種「被指向物的 const」
//           是型別的一部分，算不同的重載。
//
// ⚠️ 陷阱. 重載解析是在執行期依實際型別挑選的嗎？
//     答：不是。重載解析 100% 發生在編譯期，依據的是「引數的靜態型別」。
//         程式跑起來時，呼叫哪一個早就寫死在機器碼裡了。
//     為什麼會錯：很多人把重載和虛擬函式（多型）的心智模型混在一起。
//         虛擬函式才是執行期依「物件實際型別」分派；重載是編譯期依
//         「運算式的靜態型別」分派。若把基底指標指向衍生物件再呼叫重載函式，
//         選到的一定是基底版本——這正是兩者差異最容易踩雷的地方。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <cstdio>    // snprintf：不要依賴其他標頭「碰巧」把它帶進來

// C++ 允許同名函數，只要參數不同
int add(int a, int b) {
    return a + b;
}

double add(double a, double b) {
    return a + b;
}

int add(int a, int b, int c) {
    return a + b + c;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】用重載提供一組「型別安全」的欄位輸出函式
//   情境：寫一個小型 log/報表模組，欄位可能是字串、整數、浮點數或布林。
//   在 C 裡你得記住 %s / %d / %.2f / 還要自己把 bool 轉成文字，
//   一旦格式字串和引數對不上就是未定義行為（而且編譯器不一定抓得到）。
//   在 C++ 只要重載同一個名字，呼叫端永遠寫 field("key", value) 就好，
//   由編譯器在編譯期挑對版本——這就是「型別安全」的實際價值。
// -----------------------------------------------------------------------------
std::string field(const std::string& key, const std::string& value) {
    return key + "=\"" + value + "\"";
}

std::string field(const std::string& key, int value) {
    return key + "=" + std::to_string(value);
}

std::string field(const std::string& key, double value) {
    // 固定兩位小數，避免浮點數輸出長短不一
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.2f", value);
    return key + "=" + buf;
}

std::string field(const std::string& key, bool value) {
    return key + "=" + (value ? "true" : "false");
}

// 註：這裡刻意不加 LeetCode 範例。
//     函數重載是「語言機制」而非「演算法題型」，LeetCode 的題目不會因為
//     用了重載而變好解；硬套一題只會模糊焦點。真正需要重載的場景是
//     API 設計（如上面的 field()），所以本檔只放實務範例。

int main() {
    std::cout << add(3, 5) << std::endl;        // 調用 int 版本
    std::cout << add(3.14, 2.86) << std::endl;  // 調用 double 版本
    std::cout << add(1, 2, 3) << std::endl;     // 調用三參數版本

    std::cout << "\n=== 重載解析：誰被選中 ===" << std::endl;
    std::cout << "  add(3, 5)        -> int 版本    = " << add(3, 5) << std::endl;
    std::cout << "  add(3.14, 2.86)  -> double 版本 = " << add(3.14, 2.86) << std::endl;
    std::cout << "  add(1, 2, 3)     -> 三參數版本  = " << add(1, 2, 3) << std::endl;

    std::cout << "\n=== 日常實務：型別安全的欄位輸出 ===" << std::endl;
    std::cout << "  " << field("service", std::string("auth-api")) << std::endl;
    std::cout << "  " << field("retries", 3) << std::endl;
    std::cout << "  " << field("latency_ms", 12.5) << std::endl;
    std::cout << "  " << field("healthy", true) << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2 課：C 與 C++ 的關鍵差異10.cpp" -o diff10

// === 預期輸出 ===
// 8
// 6
// 6
//
// === 重載解析：誰被選中 ===
//   add(3, 5)        -> int 版本    = 8
//   add(3.14, 2.86)  -> double 版本 = 6
//   add(1, 2, 3)     -> 三參數版本  = 6
//
// === 日常實務：型別安全的欄位輸出 ===
//   service="auth-api"
//   retries=3
//   latency_ms=12.50
//   healthy=true
