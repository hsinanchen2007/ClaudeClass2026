// =============================================================================
//  第 13 課：vector 元素新增：push_back、emplace_back6.cpp
//    —  決策指南：四種情況下該用 push_back 還是 emplace_back
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<vector>
//   void      push_back(const T&);           // 複製
//   void      push_back(T&&);                // 移動
//   template<class... Args>
//   reference emplace_back(Args&&...);       // 就地建構（C++17 起回傳 reference）
//   複雜度：皆為攤銷 O(1)
//
// 【詳細解釋 Explanation】
//
// 【1. 四種情況的完整決策表】
//   情況 1：已有物件，之後還要用它
//       string s1 = "Hello";
//       v.push_back(s1);          // 複製，s1 保持完好
//       v.emplace_back(s1);       // 一模一樣，也是複製
//       → 兩者**完全等價**。用 push_back，意圖比較清楚。
//
//   情況 2：已有物件，之後不再需要
//       string s2 = "World";
//       v.push_back(move(s2));    // 移動
//       v.emplace_back(move(s2)); // 一模一樣，也是移動
//       → 兩者**完全等價**。用 push_back(move(x))，意圖比較清楚。
//
//   情況 3：從字面量或零件建立新元素
//       v.push_back("Foo");       // 造臨時 string → 移動進去 → 臨時物件解構
//       v.emplace_back("Bar");    // 就地建構，沒有臨時物件
//       → **emplace_back 較優**，省下一次移動與一次解構。
//
//   情況 4：建構子需要多個參數
//       v.push_back(T(a, b, c));  // 一定得先組出一個 T
//       v.emplace_back(a, b, c);  // 零件直接轉發
//       → **emplace_back 較優**，而且只有它寫得出來這種形式。
//
// 【2. 分界線只有一條】
// 把上面四種情況濃縮成一句話：
//     「你手上已經是成品」→ 兩者等價，選 push_back（語意清楚）
//     「你手上只是零件」  → emplace_back 較優（省掉臨時成品）
// push_back 的參數型別是 T，所以它永遠需要一個成品；
// emplace_back 的參數是 Args&&...，所以它可以收零件自己組。
// 這條線畫清楚，就不必再背什麼「emplace 比較快」的口訣。
//
// 【3. 為什麼「等價」時仍推薦 push_back】
//   (a) 意圖明確：push_back(s) 一看就知道「我在把 s 放進去」；
//       emplace_back(s) 讀者得多想一秒「這是放物件還是傳建構子參數？」
//   (b) 型別安全：emplace_back 走 direct-init，會繞過 explicit 保護、
//       也不做 narrowing 檢查，參數打錯比較容易悄悄編過。
//   (c) 沒有任何效能損失：這種情況下兩者生成的動作完全相同。
//   換句話說，選 push_back 是「不花錢買到的可讀性與安全性」。
//
// 【概念補充 Concept Deep Dive】
// 情況 3 的 push_back("Foo") 為什麼會多一次移動？
// push_back 的兩個重載參數型別都是 std::string（const string& / string&&），
// 而 "Foo" 的型別是 const char[4]。編譯器必須先做一次隱式轉換，
// 也就是呼叫 string(const char*) 造出一個臨時 string，
// 這個臨時物件才能繫結到 string&& 那個重載上，然後被移動進 vector。
//
// emplace_back("Bar") 則完全不同：Args 被推導成 const char(&)[4]，
// 直接轉發給 vector 內部，在尾端的原始記憶體上呼叫 string(const char*)。
// 從頭到尾沒有任何 std::string 的臨時物件存在過。
//
// 補充一個實務細節：對短字串而言，libstdc++ 有 SSO（small string
// optimization），"Foo" 這種長度會直接存在 string 物件內部、不配置 heap，
// 所以那次「多出來的移動」其實只是複製十幾個位元組，成本非常低。
// 字串一長（超過 SSO 門檻，libstdc++ 實測為 15 個字元）才會真的配置 heap，
// 這時 emplace_back 省下的就不只是搬位元組，而是一次 malloc + free。
//
// 【注意事項 Pay Attention】
// 1. 情況 1、2 下 emplace_back **沒有**比較快，兩者完全等價。
// 2. 情況 2 用了 move 之後，來源物件處於 valid but unspecified 狀態，
//    不要再讀它的值。
// 3. emplace_back 走 direct-init：可呼叫 explicit 建構子、不做窄化檢查。
// 4. SSO 門檻（libstdc++ 實測 15 字元）是實作定義，不是標準保證。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】push_back 與 emplace_back 的選擇
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 用一句話說明什麼時候該用 emplace_back。
//     答：當你手上只有「建構子參數」而還沒有成品物件時。
//         push_back 的參數型別是 T，你一定得先組出一個 T（臨時物件）；
//         emplace_back 收的是 Args&&...，可以把零件直接轉發給建構子就地建構。
//         反之，手上已經是現成的 T 時，兩者完全等價，用 push_back 更清楚。
//     追問：那 v.push_back("Foo") 算哪一種？
//         → 算「只有零件」：字面量不是 string，編譯器得先造臨時 string，
//           所以這裡 emplace_back("Foo") 較優。
//
// ⚠️ 陷阱 Q2. 有人主張「專案規範一律用 emplace_back，反正不會比較慢」，
//              這個規範有什麼問題？
//     答：效能上確實不會比較慢，但會失去兩層編譯期保護：
//         (a) emplace_back 用 direct-init，可以呼叫 explicit 建構子——
//             型別作者刻意加的 explicit 被繞過了；
//         (b) 不做 narrowing 檢查，v.emplace_back(3.9) 進 vector<int>
//             會靜默截斷成 3（本機實測），push_back({3.9}) 則編譯失敗。
//         參數一多、型別又相近時，這個風險是真實的。
//     為什麼會錯：只用「效能」單一維度評估，忽略了兩者的初始化語意不同
//         （copy-init vs direct-init），而 direct-init 的彈性正是
//         把編譯器的護欄拆掉換來的。
//
// 🔥 Q3. v.push_back(s) 與 v.emplace_back(s)（s 是具名 string）差在哪？
//     答：完全不差。s 是 lvalue，emplace_back 的 std::forward 會把它
//         原封不動轉發成 lvalue，最後呼叫的一樣是 string 的複製建構子。
//         兩者生成的機器碼通常完全相同。
//     追問：那 v.emplace_back(std::move(s)) 呢？
//         → 一樣等價於 v.push_back(std::move(s))，都是移動。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <string>
#include <sstream>

// 會「說話」的字串包裝：用來實際觀察四種情況各自做了什麼
struct Text {
    std::string data;

    Text(const char* s) : data(s) {
        std::cout << "    [從 const char* 就地建構] " << data << std::endl;
    }
    Text(const Text& o) : data(o.data) {
        std::cout << "    [複製] " << data << std::endl;
    }
    Text(Text&& o) noexcept : data(std::move(o.data)) {
        std::cout << "    [移動] " << data << std::endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】批次讀取 log 檔的每一行
//   情境：監控程式把一整段 log 文字切成行，交給後續的分析流程。
//   為什麼用 push_back：getline 已經把內容讀進 line 這個現成的 string 了，
//                       屬於「情況 2」——之後不再需要 line，
//                       所以 push_back(std::move(line)) 最恰當：
//                       語意清楚（我在放一個現成的東西），又避免了深複製。
//                       這裡改用 emplace_back(std::move(line)) 完全等價，
//                       沒有任何效能差別。
// -----------------------------------------------------------------------------
std::vector<std::string> readLines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.empty()) continue;
        // line 讀完之後就要被下一輪覆寫，資源可以直接搬走
        lines.push_back(std::move(line));
        line.clear();   // move 後狀態未指定，明確重設以便安全重複使用
    }
    return lines;
}

int main() {
    std::vector<Text> v;
    v.reserve(10);   // 撐開 capacity，排除擴容干擾

    std::cout << "=== 情況 1：已有物件，之後還要用（兩者等價：都是複製）===" << std::endl;
    Text t1("Hello");
    std::cout << "  push_back(t1):" << std::endl;
    v.push_back(t1);            // 複製，t1 仍然有效
    std::cout << "  emplace_back(t1):" << std::endl;
    v.emplace_back(t1);         // 也是複製！
    std::cout << "  → t1 依然完好: " << t1.data << std::endl;

    std::cout << "\n=== 情況 2：已有物件，之後不再需要（兩者等價：都是移動）===" << std::endl;
    Text t2("World");
    std::cout << "  push_back(std::move(t2)):" << std::endl;
    v.push_back(std::move(t2));     // 移動
    Text t2b("World2");
    std::cout << "  emplace_back(std::move(t2b)):" << std::endl;
    v.emplace_back(std::move(t2b)); // 也是移動

    std::cout << "\n=== 情況 3：從字面量建立（emplace_back 較優）===" << std::endl;
    std::cout << "  push_back(\"Foo\")  → 造臨時物件再移動:" << std::endl;
    v.push_back("Foo");         // const char* → 臨時 Text → 移動
    std::cout << "  emplace_back(\"Bar\") → 就地建構，無臨時物件:" << std::endl;
    v.emplace_back("Bar");      // 直接就地建構

    std::cout << "\n=== vector 最終內容 ===" << std::endl;
    std::cout << "  ";
    for (const Text& t : v) std::cout << t.data << " ";
    std::cout << std::endl;

    std::cout << "\n=== 日常實務：批次讀取 log 行 ===" << std::endl;
    std::string raw =
        "2026-07-19 09:15:02 [INFO]  worker pool started\n"
        "2026-07-19 09:15:44 [WARN]  queue depth 812\n"
        "\n"
        "2026-07-19 09:16:01 [ERROR] upstream timeout after 30s\n";
    std::vector<std::string> lines = readLines(raw);
    std::cout << "  共讀到 " << lines.size() << " 行（空行已略過）" << std::endl;
    for (const std::string& l : lines) {
        std::cout << "    " << l << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 13 課：vector 元素新增：push_back、emplace_back6.cpp" -o choose_which

// === 預期輸出 ===
// === 情況 1：已有物件，之後還要用（兩者等價：都是複製）===
//     [從 const char* 就地建構] Hello
//   push_back(t1):
//     [複製] Hello
//   emplace_back(t1):
//     [複製] Hello
//   → t1 依然完好: Hello
//
// === 情況 2：已有物件，之後不再需要（兩者等價：都是移動）===
//     [從 const char* 就地建構] World
//   push_back(std::move(t2)):
//     [移動] World
//     [從 const char* 就地建構] World2
//   emplace_back(std::move(t2b)):
//     [移動] World2
//
// === 情況 3：從字面量建立（emplace_back 較優）===
//   push_back("Foo")  → 造臨時物件再移動:
//     [從 const char* 就地建構] Foo
//     [移動] Foo
//   emplace_back("Bar") → 就地建構，無臨時物件:
//     [從 const char* 就地建構] Bar
//
// === vector 最終內容 ===
//   Hello Hello World World2 Foo Bar
//
// === 日常實務：批次讀取 log 行 ===
//   共讀到 3 行（空行已略過）
//     2026-07-19 09:15:02 [INFO]  worker pool started
//     2026-07-19 09:15:44 [WARN]  queue depth 812
//     2026-07-19 09:16:01 [ERROR] upstream timeout after 30s
