// =============================================================================
//  第 28 課 -2  —  拷貝省略（Copy Elision）、RVO 與 NRVO：那個「消失的拷貝」
// =============================================================================
//
// 【主題資訊 Information】
//   主題       ： 編譯器在什麼情況下可以（或必須）省略拷貝／移動建構函數
//   相關術語   ： RVO  (Return Value Optimization)        —— 回傳純右值
//                 NRVO (Named Return Value Optimization)  —— 回傳具名區域變數
//                 Copy Elision                            —— 兩者的統稱
//   標準版本   ： C++98 起允許省略（可選）；
//                 C++17 起「純右值初始化」的省略變成強制保證
//   驗證旗標   ： -fno-elide-constructors（GCC/Clang，關閉可選的省略）
//   標頭檔     ： 本檔用 <cstring>（strlen/strcpy）與 <iostream>
//
// 【詳細解釋 Explanation】
//
// 【1. 拷貝省略是「唯一被允許改變可觀察行為」的最佳化】
//   C++ 的最佳化通常受 as-if 規則約束 —— 不能改變程式的可觀察行為。
//   拷貝省略是明文寫進標準的例外：即使拷貝建構函數裡有 std::cout，
//   編譯器省略它之後那行輸出就真的不見了，而這completely合法。
//   本檔就是這個例外的直接證據：同一份原始碼，
//   加不加 -fno-elide-constructors 會印出不同的行數。
//
// 【2. C++17 劃下的那條線：哪些是「保證」，哪些仍是「可選」】
//   這是本課最值得記住、也最常被講錯的一點：
//       * 純右值（prvalue）初始化 → C++17 起「保證」省略，不可關閉。
//         例如 Tracer result = makeTracer();
//         makeTracer() 回傳的是一個 prvalue，C++17 重新定義了 prvalue 的語義：
//         它不再是「一個暫時物件」，而是「一份還沒具體化的初始化配方」，
//         直接就地初始化 result。既然從未有第二個物件存在，
//         自然沒有拷貝可言 —— 這不是「省掉」，是「根本沒發生過」。
//       * NRVO（回傳具名區域變數，本檔的 return t;）→ 永遠是「可選」的。
//         標準允許但不強制，-fno-elide-constructors 就能把它關掉。
//   本機實測驗證了這條線（見檔尾輸出說明）：
//       C++14 + -fno-elide-constructors → 2 次拷貝建構
//       C++17 + -fno-elide-constructors → 1 次拷貝建構
//   少掉的那一次，正是 C++17 保證省略、旗標關不掉的那一次。
//
// 【3. 為什麼「不要 return std::move(local);」】
//   直覺上加 std::move 應該更快，實際上更慢。理由：
//       return t;              → 符合 NRVO 條件，可能「完全不建構第二個物件」
//       return std::move(t);   → 運算式變成 xvalue，不再是「回傳具名物件」，
//                                NRVO 條件失效 → 退化成一次移動建構
//   從「0 次」變成「1 次」。本檔的實務範例用計數器把這個差別量出來：
//   return r; 是 copies=0 moves=0；return std::move(r); 是 moves=1。
//   GCC 甚至內建了 -Wpessimizing-move 警告專門抓這個寫法。
//   （對照第 31 課「std::move 只是 cast」—— 它不會讓事情變快，
//     這裡它還擋住了比 move 更強的最佳化。）
//
// 【4. 省略發生時，解構函數也跟著少一次】
//   省略掉的物件從未存在，所以它的解構函數也不會被呼叫。
//   本檔預設輸出只有 1 次 [建構] 與 1 次 [解構]，
//   關掉省略後才會看到 [拷貝建構] 與多出來的那次 [解構]。
//   這也是為什麼「在拷貝建構函數裡做副作用（計數、log、註冊）」很危險 ——
//   那些副作用會隨著最佳化等級與標準版本而時有時無。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 編譯器實際上怎麼做到的：呼叫端傳入「回傳位址」
//   在常見 ABI（如 System V AMD64）中，回傳一個非平凡類別時，
//   呼叫端會先配置好目的地空間，把它的位址透過隱藏參數傳給被呼叫函式。
//   函式內部就「直接在那塊空間上」建構區域變數 t。
//   於是 return 的時候沒有任何東西需要搬 —— t 從一開始就住在 result 的位置。
//   這就是 NRVO 的真面目：不是「搬得比較快」，而是「一開始就蓋在正確的地方」。
//
// (B) NRVO 為什麼無法被標準強制
//   函式若有多條 return 路徑回傳不同的具名變數，
//   編譯器無法在進入函式時就決定「哪一個變數該蓋在回傳位址上」。
//   有分支、有例外處理、變數是參數（參數的儲存位置由呼叫慣例決定）時，
//   NRVO 都可能失效。標準因此只能「允許」而不能「保證」。
//   實務守則：即使不保證，也照樣寫 return t; —— 給編譯器機會，
//   最差就是一次移動，而 return std::move(t) 是把機會直接放棄。
//
// (C) 這個類別本身其實違反 Rule of Three
//   Tracer 持有裸 char*、寫了拷貝建構與解構，卻沒有寫拷貝賦值運算子。
//   本檔沒有用到賦值所以不會出事，但若寫 a = b 就會是淺拷貝 → double free。
//   這正是第 29 課與第 34 課要處理的問題；本檔專注在省略，
//   刻意保留原樣以免模糊焦點 —— 但讀者不該把它當成類別設計的範本。
//
// 【注意事項 Pay Attention】
//   1. 拷貝省略可以改變可觀察行為（少印幾行），這是標準明文允許的例外。
//      因此不要在拷貝建構函數裡放「一定要發生」的副作用。
//   2. C++17 只保證「純右值初始化」的省略；NRVO 永遠是可選的。
//      不要把「本機看到 0 次拷貝」當成跨編譯器的保證。
//   3. 不要寫 return std::move(local);，那會把 NRVO 從 0 次拷貝
//      降級成 1 次移動。GCC 的 -Wpessimizing-move 會警告。
//   4. -fno-elide-constructors 是教學／驗證用旗標，不要用在正式建置。
//   5. 本檔輸出行數會隨「標準版本」與「是否加 -fno-elide-constructors」
//      而改變，這是預期中的行為，不是 bug。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】拷貝省略 / RVO / NRVO
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++17 對拷貝省略做了什麼改變？是不是所有拷貝都保證被省略了？
//     答：不是。C++17 只把「純右值（prvalue）初始化」的省略變成強制保證，
//         例如 Tracer result = makeTracer();。做法是重新定義 prvalue ——
//         它不再代表一個暫時物件，而是一份「尚未具體化的初始化配方」，
//         直接就地初始化目標，所以根本沒有第二個物件產生。
//         而 NRVO（回傳具名區域變數）在 C++17 之後仍然只是「允許」，不保證。
//     追問：怎麼證明這條線真的存在？
//         → 加 -fno-elide-constructors 編譯：C++14 會出現 2 次拷貝建構，
//           C++17 只剩 1 次（本機實測）。被旗標關不掉的那一次，
//           就是 C++17 保證省略的那一次。
//
// 🔥 Q2. 為什麼 return std::move(local); 是反模式？
//     答：加上 std::move 之後，回傳運算式從「具名物件」變成 xvalue，
//         不再符合 NRVO 的條件。本來可能是「0 次拷貝、0 次移動」
//         （直接建構在呼叫端的空間上），現在保證退化成 1 次移動建構。
//         正確寫法就是 return local;。
//     追問：那什麼時候 return std::move(x) 才是對的？
//         → 當 x 本來就不適用 NRVO 時，例如 x 是函式參數、
//           或是要回傳成員變數 std::move(m_buf)。這些情形 NRVO 本來就不會發生，
//           加 move 才有意義（否則會退成拷貝）。
//
// ⚠️ 陷阱. 「拷貝省略只是最佳化，不會影響程式行為，所以拷貝建構函數裡
//           放計數器／log 也沒關係。」
//     答：拷貝省略是標準明文允許「改變可觀察行為」的例外，
//         它不受 as-if 規則約束。省略發生時，拷貝建構函數裡的
//         std::cout、計數器遞增、資源註冊全都不會執行，
//         連對應的解構函數也不會被呼叫。
//         本檔預設只印 1 次 [建構] + 1 次 [解構]，
//         加上 -fno-elide-constructors 才會多出 [拷貝建構] 與一次 [解構]。
//     為什麼會錯：把拷貝省略當成一般最佳化，
//         以為「最佳化不能改變程式的可觀察行為」這條規則也適用於它。
//         實際上標準特別為它開了後門，正因如此才需要小心副作用。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔從缺，理由如下
//   拷貝省略是編譯器最佳化與物件生命週期的議題，
//   LeetCode 只比對函式的回傳值與執行時間，
//   不會、也無法觀察你的類別被建構了幾次。
//   硬掛一題（例如 146. LRU Cache）並不能示範 RVO/NRVO，
//   反而會讓讀者以為兩者相關。這裡改用「工廠函式回傳報表物件」
//   這個天天會寫到的場景，並用計數器把省略的效果直接量出來。
//
// -----------------------------------------------------------------------------
// lesson28_elision.cpp — 比較有無拷貝省略的效果
// -----------------------------------------------------------------------------

#include <iostream>
#include <cstring>
#include <string>
#include <utility>

class Tracer {
private:
    char* m_data;

public:
    Tracer(const char* str) {
        m_data = new char[std::strlen(str) + 1];
        std::strcpy(m_data, str);
        std::cout << "  [建構] \"" << m_data << "\"\n";
    }

    Tracer(const Tracer& other) {
        m_data = new char[std::strlen(other.m_data) + 1];
        std::strcpy(m_data, other.m_data);
        std::cout << "  [拷貝建構] \"" << m_data << "\"\n";
    }

    ~Tracer() {
        std::cout << "  [解構] \"" << m_data << "\"\n";
        delete[] m_data;
    }

    const char* c_str() const { return m_data; }
};

Tracer makeTracer() {
    Tracer t("from function");
    return t;   // NRVO 可能省略拷貝（可選，非保證）
}

// -----------------------------------------------------------------------------
// 【日常實務範例】工廠函式回傳「每日報表」物件
//   情境：排程工作每天產生一份報表物件（內含組好的內容字串），
//         由 buildDailyReport() 之類的工廠函式回傳給呼叫端。
//         這是最常見的「函式回傳一個持有資源的物件」場景。
//   為什麼用到本主題：這一行 return 決定了要不要多做一次深拷貝。
//         寫 return r; 讓 NRVO 有機會把 r 直接建構在呼叫端的空間上（0 次）；
//         多此一舉寫成 return std::move(r); 反而讓 NRVO 失效，
//         保證退化成 1 次移動建構。
//   量測方式：用計數器而不是計時 —— 計數是決定性的，可以寫進預期輸出；
//         計時每次執行都不同，不能當教材的驗證依據。
// -----------------------------------------------------------------------------
namespace practice {

int g_copies = 0;
int g_moves  = 0;

class Report {
    std::string m_body;
public:
    explicit Report(std::string body) : m_body(std::move(body)) {}
    Report(const Report& o) : m_body(o.m_body)                { ++g_copies; }
    Report(Report&& o) noexcept : m_body(std::move(o.m_body)) { ++g_moves;  }
    const std::string& body() const { return m_body; }
};

// 正確寫法：符合 NRVO 條件
Report buildReportGood() {
    Report r("2026-07-20 daily summary");
    return r;
}

// 反模式：多寫一個 std::move 就把 NRVO 擋掉了
// 註：GCC 的 -Wpessimizing-move 會對下一行發出警告 —— 編譯器自己就知道這是錯的。
//     這裡刻意保留這個寫法當教材，並只在這一小段關掉該警告，
//     好讓整份檔案仍能以 -Wall -Wextra 乾淨編譯。
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpessimizing-move"
Report buildReportBad() {
    Report r("2026-07-20 daily summary");
    return std::move(r);      // ← 反模式：NRVO 失效，退化成一次移動
}
#pragma GCC diagnostic pop

void demo() {
    g_copies = 0;
    g_moves  = 0;
    {
        Report a = buildReportGood();
        (void)a;
    }
    std::cout << "  return r;             拷貝=" << g_copies
              << " 移動=" << g_moves << "   ← NRVO 生效，兩者都是 0\n";

    g_copies = 0;
    g_moves  = 0;
    {
        Report b = buildReportBad();
        (void)b;
    }
    std::cout << "  return std::move(r);  拷貝=" << g_copies
              << " 移動=" << g_moves << "   ← NRVO 被擋掉，多了一次移動\n";
}

}  // namespace practice

int main() {
    std::cout << "=== 1. 測試 RVO / NRVO ===\n";
    Tracer result = makeTracer();
    std::cout << "result = \"" << result.c_str() << "\"\n";
    std::cout << "--- 結束 ---\n";

    std::cout << "\n=== 2. 日常實務：return r; vs return std::move(r); ===\n";
    practice::demo();

    std::cout << "\n=== 3. 結論 ===\n";
    std::cout << "  拷貝省略是標準允許改變可觀察行為的例外，少印的那幾行不是 bug。\n";
    std::cout << "  C++17 只保證「純右值初始化」的省略；NRVO 永遠是可選的。\n";
    std::cout << "  回傳區域變數就寫 return r;，加 std::move 只會讓它變慢。\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -Wreorder "第 28 課：拷貝建構函數（Copy Constructor）2.cpp" -o elision
//
// 對照實驗（教學用，不要用在正式建置）：
//   g++ -std=c++17 -Wall -Wextra -fno-elide-constructors <本檔> -o no_elision
//   g++ -std=c++14 -Wall -Wextra -fno-elide-constructors <本檔> -o no_elision14

// ── 輸出說明（讀之前先看這裡）────────────────────────────────────────────
// * 下面貼的是「預設建置」（允許省略）的輸出，本機實測連跑 5 次逐位元組相同。
//   沒有位址、沒有耗時，完全決定性。
// * 第 1 段只有 1 次 [建構] 與 1 次 [解構]，看不到 [拷貝建構] ——
//   這不是漏印，是 RVO/NRVO 讓那個物件從未存在。
//   拷貝省略是標準明文允許「改變可觀察行為」的例外，不受 as-if 規則約束。
// * 本機對照實驗（同一份原始碼，只換旗標／標準版本，實測數據）：
//       預設（C++17，允許省略）            → [拷貝建構] 出現 0 次
//       C++17 + -fno-elide-constructors    → [拷貝建構] 出現 1 次
//       C++14 + -fno-elide-constructors    → [拷貝建構] 出現 2 次
//   C++14 的兩次分別是「t → 回傳暫時物件」與「暫時物件 → result」。
//   C++17 保證省略後者（純右值初始化），所以旗標只關得掉前者（NRVO）。
//   這組數字就是「C++17 保證的只有 prvalue 那一半」最直接的證據。
// * -O2 的輸出與預設相同（省略在 -O0 就已經發生，它不是靠最佳化等級開啟的）。
// * 第 2 段用計數器而非計時：0 vs 1 是決定性的結果，可重現。
//   不過 NRVO 本身仍是「可選」的最佳化，其他編譯器可能給出不同數字 ——
//   會變的是 return r; 那一行，return std::move(r); 的 1 次移動則是保證的。
// ─────────────────────────────────────────────────────────────────────────

// === 預期輸出 ===
// === 1. 測試 RVO / NRVO ===
//   [建構] "from function"
// result = "from function"
// --- 結束 ---
//
// === 2. 日常實務：return r; vs return std::move(r); ===
//   return r;             拷貝=0 移動=0   ← NRVO 生效，兩者都是 0
//   return std::move(r);  拷貝=0 移動=1   ← NRVO 被擋掉，多了一次移動
//
// === 3. 結論 ===
//   拷貝省略是標準允許改變可觀察行為的例外，少印的那幾行不是 bug。
//   C++17 只保證「純右值初始化」的省略；NRVO 永遠是可選的。
//   回傳區域變數就寫 return r;，加 std::move 只會讓它變慢。
//   [解構] "from function"
