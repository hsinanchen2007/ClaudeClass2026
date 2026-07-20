// =============================================================================
//  第 4 課：命名空間（namespace）基礎（7） — 命名空間可以分次擴充(reopen)
// =============================================================================
//
// 【主題資訊 Information】
//   語法：同一個命名空間名稱可以被重複「開啟」多次,內容會累加:
//             namespace mylib { void funcA(); }   // 第一次
//             namespace mylib { void funcB(); }   // 再開一次,不是覆蓋
//   標準版本：C++98 起。
//   複雜度  ：零執行期成本(純編譯期名稱組織)。
//
// 【詳細解釋 Explanation】
//
// 【1. 「開放式」是刻意的設計,而不是漏洞】
//   class 是封閉的 —— 定義完就不能再往裡面加成員。
//   namespace 相反,它是開放的:任何地方都可以再寫一次
//   namespace mylib { ... } 把新東西加進去。
//   這個差異來自兩者的目的:
//     class     要描述「一個型別」,必須有確定的大小與成員佈局,所以得封閉。
//     namespace 只是「名稱的分組」,不需要固定大小,自然可以持續累加。
//
// 【2. 為什麼多檔案專案非要這個性質不可】
//   一個函式庫的內容一定分散在許多標頭檔:
//       math.h    → namespace mylib { double sqrt2(); ... }
//       string.h  → namespace mylib { std::string trim(...); ... }
//       io.h      → namespace mylib { void writeAll(...); ... }
//   使用者 #include 了哪幾個,mylib 裡就看得到哪幾個。
//   若命名空間是封閉的,整個函式庫的所有宣告就得擠進同一個檔案 ——
//   那等於放棄模組化。開放式讓「同一個邏輯分組」可以跨檔案組合。
//
// 【3. 「累加」與「覆蓋」的差別】
//   第二次開啟不會清空第一次的內容,兩段的成員一起構成 mylib。
//   本檔 main 能同時呼叫 funcA 和 funcB,就是最直接的證明。
//   要注意的是它也「不會檢查你是不是打錯字」——
//   namespace mylib 打成 namespace mylibb,你會安靜地得到一個全新的命名空間,
//   而不是任何錯誤訊息。這個陷阱詳見下方面試題。
//
// 【4. 只有「宣告」會累加,「定義」仍受單一定義規則約束】
//   分次開啟增加的是名稱,不是豁免權。若兩段裡都定義了同一個
//   非 inline 函式 mylib::funcA,一樣是重複定義的連結錯誤。
//   開放式命名空間解決的是「組織」問題,不是「重複」問題。
//
// 【概念補充 Concept Deep Dive】
//   命名空間的開放性衍生出兩個常見的實務手法:
//   (A) 巢狀分層 —— 大型函式庫常見 mylib::detail 這樣的子命名空間,
//       用來擺放「公開標頭中不得不出現、但使用者不該直接呼叫」的實作細節。
//       它不是強制的存取控制(使用者硬要寫 mylib::detail::foo() 仍然可行),
//       而是一個清楚的約定。C++17 起可以簡寫成 namespace mylib::detail { ... }。
//   (B) inline namespace(C++11)—— 用於 API 版本管理:
//           namespace mylib {
//               inline namespace v2 { void encode(); }   // 預設版本
//                      namespace v1 { void encode(); }   // 舊版仍可明確指定
//           }
//       呼叫 mylib::encode() 會自動解析到 v2,想要舊版就寫 mylib::v1::encode()。
//       這讓函式庫能在不破壞既有呼叫端的前提下推出新版本,
//       同時讓連結期的符號名稱帶上版本資訊,避免新舊版本被誤連在一起。
//
// 【注意事項 Pay Attention】
//   1. 命名空間名稱打錯不會報錯,只會安靜地建立一個新的命名空間。
//      錯誤要等到呼叫端找不到成員時才浮現,而且訊息不會指向真正的肇因。
//   2. 分次開啟只累加名稱;重複「定義」同一個非 inline 實體仍違反 ODR。
//   3. 開放性代表任何人都能往你的命名空間加東西,它不是封裝機制。
//      真正的實作細節請放匿名命名空間(本課第 6 份檔案)或 detail 子命名空間。
//   4. 不要為了「擴充」而往 std 裡加東西 —— 除了少數標準明確允許的
//      樣板特化(例如為自訂型別特化 std::hash)之外,
//      向 namespace std 新增宣告是未定義行為。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】命名空間的開放性
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 namespace 可以分次開啟擴充,class 卻不行?
//     答：因為兩者要描述的東西不同。class 定義的是「一個型別」,
//         編譯器必須在定義結束時就確定它的大小、對齊與成員佈局,
//         才能配置物件、計算 sizeof,所以必須封閉。
//         namespace 只是名稱的分組,沒有大小或佈局可言,
//         自然可以持續累加。這個開放性正是函式庫能把宣告分散到
//         多個標頭檔、卻仍屬於同一個邏輯分組的前提。
//     追問：那我可以往 std 裡加東西嗎?
//         → 不行。除了標準明確允許的樣板特化(例如為自訂型別特化
//           std::hash 或 std::swap)之外,向 namespace std 新增任何宣告
//           都是未定義行為 —— 即使它「編得過、跑得動」。
//
// 🔥 Q2. inline namespace 是做什麼用的?
//     答：做 API 版本管理。把某個子命名空間標成 inline,
//         它的成員會自動出現在外層,呼叫端不必加版本前綴:
//             namespace mylib { inline namespace v2 { void encode(); } }
//             mylib::encode();       // 自動用到 v2
//             mylib::v1::encode();   // 需要舊版時明確指定
//         升級版本只需移動 inline 關鍵字,既有呼叫端一行都不用改。
//         同時符號名稱帶上了版本,新舊版本不會在連結期被誤混。
//     追問：那和命名空間別名(namespace api = v2;)差在哪?
//         → 控制權的歸屬不同。inline namespace 由「函式庫作者」決定預設版本;
//           別名由「使用者」在自己的檔案裡決定要叫什麼、指向哪一版。
//
// ⚠️ 陷阱. 我在第二個檔案寫 namespace mylibb { void funcB(); }(多打了一個 b),
//         編譯完全沒報錯,為什麼呼叫 mylib::funcB() 卻說找不到?
//     答：因為打錯的名稱不會被視為錯誤 —— 編譯器認為你是要「建立一個新的
//         命名空間 mylibb」,那是完全合法的行為。於是 funcB 確實存在,
//         只是住在 mylibb 裡,mylib 裡從來沒有過它。
//         錯誤只會在呼叫端出現(error: 'funcB' is not a member of 'mylib'),
//         而那裡看不出真正的肇因。
//     為什麼會錯：把「重新開啟既有命名空間」和「宣告新命名空間」
//         想成兩種不同的語法,以為編譯器分得出你要哪一種。
//         實際上它們的語法完全相同,編譯器只看名字 ——
//         名字對就是擴充,名字不對就是新建,沒有第三種可能。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【本檔不加 LeetCode 範例的理由】
//   命名空間的開放性是「跨檔案的程式碼組織」機制,與演算法及複雜度無關,
//   而且它的價值需要多個標頭檔才體現得出來。
//   下方改以單檔內模擬多模組的方式示範。

#include <iostream>
#include <string>
#include <vector>

// 第一部分
namespace mylib {
    void funcA() {
        std::cout << "Function A" << std::endl;
    }
}

// 第二部分（擴展同一個命名空間）
namespace mylib {
    void funcB() {
        std::cout << "Function B" << std::endl;
    }
}

// 這在多檔案專案中很常見：
// math.h 定義 namespace mylib { 數學函數 }
// string.h 定義 namespace mylib { 字串函數 }
// 它們都屬於同一個 mylib 命名空間

// -----------------------------------------------------------------------------
// 【日常實務範例】在單一檔案內模擬「多個標頭各自擴充同一個函式庫命名空間」
// 情境：一個叫 applib 的內部函式庫，真實專案中會拆成三個標頭檔：
//         applib/text.h    → namespace applib { 字串工具 }
//         applib/stats.h   → namespace applib { 統計工具 }
//         applib/detail.h  → namespace applib::detail { 不對外的實作細節 }
//       使用者 #include 了哪幾個，applib 裡就看得到哪幾個成員。
//       下面把這三段依序寫出來，效果與分散在三個檔案完全相同。
// -----------------------------------------------------------------------------

// ── 模擬 applib/detail.h：實作細節放進 detail 子命名空間 ──────────────
// 這是一種「約定式」的隱藏：使用者硬要呼叫 applib::detail::joinWith() 仍然可行，
// 但 detail 這個名字已明白宣告「這不是公開 API，隨時可能改動」。
namespace applib {
    namespace detail {
        std::string joinWith(const std::vector<std::string>& parts, char sep) {
            std::string out;
            for (std::size_t i = 0; i < parts.size(); ++i) {
                if (i) out += sep;
                out += parts[i];
            }
            return out;
        }
    }
}

// ── 模擬 applib/text.h：第二次開啟 applib，加入字串工具 ────────────────
namespace applib {
    std::string joinCSV(const std::vector<std::string>& parts) {
        return detail::joinWith(parts, ',');   // 同一命名空間內可直接用 detail::
    }
}

// ── 模擬 applib/stats.h：第三次開啟 applib，加入統計工具 ───────────────
namespace applib {
    double average(const std::vector<int>& values) {
        if (values.empty()) return 0.0;
        long long sum = 0;
        for (int v : values) sum += v;
        return static_cast<double>(sum) / static_cast<double>(values.size());
    }
}

int main() {
    mylib::funcA();
    mylib::funcB();

    // ── 實務：三次開啟累積出來的 applib，成員全部並存 ──────────────
    std::cout << "\n=== applib（由三段命名空間累加而成）===\n";
    const std::vector<std::string> cols = {"id", "name", "email"};
    std::cout << "  applib::joinCSV        -> " << applib::joinCSV(cols) << "\n";
    std::cout << "  applib::average        -> " << applib::average({90, 85, 77, 100}) << "\n";
    // detail 雖然「可以」呼叫，但命名已表明它不是公開 API
    std::cout << "  applib::detail::joinWith -> "
              << applib::detail::joinWith(cols, '|') << "（實作細節，不建議直接用）\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 4 課：命名空間（namespace）基礎7.cpp" -o ns7

// ─────────────────────────────────────────────────────────────────────────────
// 【輸出但書】
//  1. applib::average 印出的是 88 而不是 88.0 —— average 的回傳型別確實是
//     double,(90+85+77+100)/4 = 352/4 = 88 剛好整除,而 std::cout 的預設
//     浮點格式會省略無意義的小數點。這是格式化行為,不是型別被截斷。
//  2. mylib 與 applib 各自由多段 namespace 累加而成,輸出中所有成員並存,
//     證明第二次、第三次開啟是「累加」而非「覆蓋」。
//  3. 實測驗證命名空間打錯字不會報錯:另建最小重現檔,
//     把第二段寫成 namespace mylibb(多一個 b)後,編譯器對該段本身
//     完全不報錯,錯誤只出現在呼叫端:
//         error: 'b' is not a member of 'mylib'; did you mean 'mylibb::b'?
//     ——肇因在宣告處,錯誤卻報在呼叫處,這正是此陷阱難纏的原因。
//  4. 以下為本機 g++ 15.2.0 (Ubuntu 26.04) 連續執行 3 次、逐位元組相同的結果。
// ─────────────────────────────────────────────────────────────────────────────

// === 預期輸出 ===
// Function A
// Function B
//
// === applib（由三段命名空間累加而成）===
//   applib::joinCSV        -> id,name,email
//   applib::average        -> 88
//   applib::detail::joinWith -> id|name|email（實作細節，不建議直接用）
