// =============================================================================
//  第 13 課：vector 元素新增：push_back、emplace_back9.cpp
//    —  完美轉發：emplace_back 怎麼「記得」你傳的是 lvalue 還是 rvalue
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<vector>
//   template<class... Args>
//   reference emplace_back(Args&&... args);   // Args&& 是 forwarding reference
//   複雜度：攤銷 O(1)
//
//   內部關鍵：
//       ::new (ptr) T(std::forward<Args>(args)...);
//   std::forward 保留每個參數的**值類別 (value category)**，
//   使 T 的建構子重載決議結果，與「直接呼叫 T(args...)」完全一致。
//
// 【詳細解釋 Explanation】
//
// 【1. Args&& 不是右值參考，而是 forwarding reference】
// 看到 T&& 直覺會想「這是右值參考」，但在**模板參數推導**的情境下不是。
// 當 Args 是待推導的模板參數時，Args&& 稱為 forwarding reference
// （舊稱 universal reference），它會依傳入的東西推導出不同結果：
//
//     傳入          Args 推導成        Args&& 實際型別
//     ----------------------------------------------------
//     lvalue        std::string&      std::string& &&  → std::string&
//     rvalue        std::string       std::string&&    → std::string&&
//
// 最右欄的「摺疊」就是**參考摺疊 (reference collapsing)** 規則：
//     & &   → &        & &&  → &
//     && &  → &        && && → &&
// 白話說就是「只要出現一個 lvalue reference，結果就是 lvalue reference」。
// 這條規則正是完美轉發能運作的語言基礎。
//
// 【2. std::forward 做了什麼】
// std::forward<Args>(args) 本質上是一個有條件的 static_cast：
//     * Args 被推導成 std::string&（來源是 lvalue）→ 轉成 lvalue → 選複製建構子
//     * Args 被推導成 std::string （來源是 rvalue）→ 轉成 rvalue → 選移動建構子
// 對照 std::move：它是**無條件**轉成 rvalue，不管來源是什麼。
//     std::move    → 我確定要放棄這個物件
//     std::forward → 我要原封不動地把「值類別」傳下去
// 在寫模板時，把該用 forward 的地方寫成 move，會把呼叫端的 lvalue
// 也偷偷搬空，是很嚴重的 bug。
//
// 【3. 本檔三次呼叫各自選中哪個建構子】
// Widget 有兩個建構子：Widget(const string&) 與 Widget(string&&)。
//
//   v.emplace_back(s);              // s 是具名變數 → lvalue
//       Args = std::string&，轉發後仍是 lvalue → 選中 Widget(const string&)
//       實測輸出：「從 const string& 建構」
//
//   v.emplace_back(std::move(s));   // move 轉成 rvalue
//       Args = std::string，轉發後是 rvalue → 選中 Widget(string&&)
//       實測輸出：「從 string&& 建構」
//
//   v.emplace_back("World");        // 字面量是 const char[6]
//       Args = const char(&)[6]。Widget 沒有收 const char* 的建構子，
//       所以編譯器先隱式造出臨時 std::string，該臨時物件是 rvalue
//       → 選中 Widget(string&&)
//       實測輸出：「從 string&& 建構」
//
// 【4. 這證明了什麼】
// 第一次呼叫最關鍵：**emplace_back(s) 對 lvalue 選的是複製建構子**。
// 這就是「emplace_back 永遠比較快」為什麼是錯的——
// 轉發保留了值類別，lvalue 進去、lvalue 出來，最後照樣複製。
// emplace_back 省掉的是「臨時 Widget」，不是「複製 string」。
//
// 【概念補充 Concept Deep Dive】
// 為什麼 emplace_back 要用 Args&&... 而不是簡單的 const Args&...？
// 如果簽章寫成 const Args&...，所有參數都會退化成 const lvalue reference，
// 那麼即使呼叫端傳的是 rvalue（例如 std::move(s)），
// 轉發進去也只剩 const string&，就**永遠只能複製、無法移動**。
// 移動語意會在跨越這一層函式呼叫時整個失效。
// 反過來若寫成 Args&&... 但轉發時用 std::move，
// 則呼叫端的 lvalue 會被意外搬空。
// 兩個坑都只有「forwarding reference + std::forward」這個組合能同時避開，
// 這也是為什麼幾乎所有「把參數傳給別人」的泛型介面
// （emplace 系列、make_unique、make_shared、thread 建構子、bind）
// 都是這個寫法。
//
// 補充一個容易混淆的細節：Args 是 parameter pack，
// std::forward<Args>(args)... 的省略號會對每個參數各展開一次，
// 也就是每個參數獨立保留自己的值類別。
// 傳 (lvalue, rvalue) 進去，出來還是 (lvalue, rvalue)，不會互相影響。
//
// 【注意事項 Pay Attention】
// 1. Args&& 在模板推導情境下是 forwarding reference，不是右值參考。
// 2. std::forward 是**有條件**轉型；std::move 是**無條件**轉型。
//    模板中該用 forward 的地方寫成 move，會偷搬呼叫端的 lvalue。
// 3. emplace_back(s) 對 lvalue 選的是複製建構子，不會比 push_back(s) 快。
// 4. 本檔 Widget 的建構子刻意**不是** explicit，
//    所以 push_back("World") 也能編譯；若加上 explicit 就只有 emplace_back
//    能用（見本課第 7 個範例檔的陷阱）。
// 5. Widget 沒有自訂複製／移動建構子，用的是編譯器產生的版本，
//    所以本檔輸出只會看到那兩個「從 ... 建構」訊息。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】完美轉發與 emplace_back
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. emplace_back 的參數為什麼寫成 Args&&... ？這是右值參考嗎？
//     答：不是右值參考，是 **forwarding reference**（舊稱 universal
//         reference）。因為 Args 是待推導的模板參數，傳 lvalue 時
//         Args 推導成 T&，經參考摺疊 T& && → T&，結果是 lvalue reference；
//         傳 rvalue 時 Args 推導成 T，結果是 T&&。
//         這樣同一個簽章就能同時接住 lvalue 與 rvalue 並保留其值類別。
//     追問：參考摺疊的規則是什麼？
//         → & & → &、& && → &、&& & → &、&& && → &&。
//           只要出現一個 lvalue reference，結果就是 lvalue reference。
//
// 🔥 Q2. std::move 與 std::forward 差在哪？
//     答：兩者都是 static_cast，都不搬資料。差別在條件：
//         std::move 是**無條件**轉成 rvalue（我確定要放棄這個物件）；
//         std::forward<Args> 是**有條件**轉型——Args 推導自 lvalue 就轉成
//         lvalue，推導自 rvalue 才轉成 rvalue（原封不動傳遞值類別）。
//     追問：在模板裡把 forward 誤寫成 move 會怎樣？
//         → 呼叫端傳進來的 lvalue 會被當成 rvalue 搬空，
//           呼叫者的物件莫名其妙變成 unspecified 狀態，
//           而且編譯器不會有任何警告。
//
// ⚠️ 陷阱 Q3. v.emplace_back(s)（s 是具名 std::string）會呼叫 Widget 的哪個建構子？
//     答：呼叫 Widget(const std::string&)，也就是**複製**版本。
//         s 是 lvalue，Args 推導成 std::string&，
//         std::forward 轉發出來仍是 lvalue，重載決議因此選中 const& 版本。
//         本檔實測輸出「從 const string& 建構」可證。
//     為什麼會錯：以為「emplace = 就地建構 = 一定用移動」。
//         就地建構指的是「在容器的記憶體上建構」，
//         至於用哪個建構子，完全由你傳進去的參數值類別決定。
//
// 🔥 Q4. 如果 emplace_back 的簽章改成 const Args&... 會有什麼後果？
//     答：所有參數都會退化成 const lvalue reference，
//         即使呼叫端傳 std::move(s)，轉發進去也只剩 const string&，
//         於是**永遠只能複製、無法移動**——移動語意會在這層呼叫中整個失效。
//         這正是必須用 forwarding reference 的原因。
//     追問：還有哪些標準庫介面用同樣的手法？
//         → make_unique、make_shared、std::thread 建構子、std::bind、
//           以及所有容器的 emplace / emplace_hint / try_emplace。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <string>

struct Widget {
    std::string data;

    // 刻意提供兩個重載，好觀察轉發後究竟選中哪一個
    Widget(const std::string& s) : data(s) {
        std::cout << "  從 const string& 建構（複製）" << std::endl;
    }

    Widget(std::string&& s) : data(std::move(s)) {
        std::cout << "  從 string&& 建構（移動）" << std::endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】組裝待送出的通知訊息佇列
//   情境：通知服務要把多則訊息排入佇列。訊息來源有兩種——
//         (a) 樣板字串：之後還要重複使用，必須保留 → 傳 lvalue，複製
//         (b) 現場組出來的字串：送出後就不再需要 → std::move，搬走不複製
//   為什麼這裡看得出完美轉發的價值：同一個 emplace_back 呼叫式，
//   光靠「有沒有加 std::move」就決定了複製或移動，
//   不需要為兩種情況寫兩個不同的函式。
// -----------------------------------------------------------------------------
std::vector<Widget> buildNotificationQueue() {
    std::vector<Widget> queue;
    queue.reserve(3);

    // (a) 樣板之後還要用 → 傳 lvalue，走複製，template 保持完好
    std::string dailyTemplate = "[daily] 系統健康報告已產生";
    queue.emplace_back(dailyTemplate);

    // (b) 現場組出來的一次性訊息 → move 走，避免深複製
    std::string alert = "[alert] 磁碟使用率 " + std::to_string(92) + "%";
    queue.emplace_back(std::move(alert));

    // (c) 直接由字面量建立：先造臨時 string（rvalue）→ 走移動
    queue.emplace_back("[info] 排程任務全部完成");

    std::cout << "  樣板變數仍可重複使用: \"" << dailyTemplate << "\""
              << std::endl;
    return queue;
}

int main() {
    std::vector<Widget> v;
    v.reserve(5);   // 撐開 capacity，排除擴容干擾

    std::string s = "Hello";

    std::cout << "=== emplace_back(s)：s 是 lvalue ===" << std::endl;
    v.emplace_back(s);              // 轉發 lvalue → 選中 const string& 版本
    std::cout << "  → 轉發保留了 lvalue，所以選中複製版本" << std::endl;
    std::cout << "  → s 依然完好: \"" << s << "\"" << std::endl;

    std::cout << "\n=== emplace_back(std::move(s))：轉成 rvalue ===" << std::endl;
    v.emplace_back(std::move(s));   // 轉發 rvalue → 選中 string&& 版本
    std::cout << "  → 轉發保留了 rvalue，所以選中移動版本" << std::endl;
    std::cout << "  → s 已被搬走，處於 valid but unspecified 狀態" << std::endl;

    std::cout << "\n=== emplace_back(\"World\")：字面量 ===" << std::endl;
    v.emplace_back("World");        // 先造臨時 string（rvalue）→ 移動版本
    std::cout << "  → const char* 先隱式造出臨時 string，"
                 "臨時物件是 rvalue，故選中移動版本" << std::endl;

    std::cout << "\n=== vector 內容 ===" << std::endl;
    std::cout << "  ";
    for (const Widget& w : v) std::cout << "\"" << w.data << "\" ";
    std::cout << std::endl;

    std::cout << "\n=== 關鍵結論 ===" << std::endl;
    std::cout << "  emplace_back 對 lvalue 選的是「複製」建構子，" << std::endl;
    std::cout << "  所以它並不會讓 push_back(s) 這種呼叫變快。" << std::endl;
    std::cout << "  它省下的是「臨時 Widget 物件」，不是「複製 string」。"
              << std::endl;

    std::cout << "\n=== 日常實務：通知訊息佇列 ===" << std::endl;
    std::vector<Widget> queue = buildNotificationQueue();
    std::cout << "  佇列共 " << queue.size() << " 則:" << std::endl;
    for (const Widget& w : queue) {
        std::cout << "    " << w.data << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 13 課：vector 元素新增：push_back、emplace_back9.cpp" -o perfect_forwarding

// === 預期輸出 ===
// === emplace_back(s)：s 是 lvalue ===
//   從 const string& 建構（複製）
//   → 轉發保留了 lvalue，所以選中複製版本
//   → s 依然完好: "Hello"
// 
// === emplace_back(std::move(s))：轉成 rvalue ===
//   從 string&& 建構（移動）
//   → 轉發保留了 rvalue，所以選中移動版本
//   → s 已被搬走，處於 valid but unspecified 狀態
// 
// === emplace_back("World")：字面量 ===
//   從 string&& 建構（移動）
//   → const char* 先隱式造出臨時 string，臨時物件是 rvalue，故選中移動版本
// 
// === vector 內容 ===
//   "Hello" "Hello" "World" 
// 
// === 關鍵結論 ===
//   emplace_back 對 lvalue 選的是「複製」建構子，
//   所以它並不會讓 push_back(s) 這種呼叫變快。
//   它省下的是「臨時 Widget 物件」，不是「複製 string」。
// 
// === 日常實務：通知訊息佇列 ===
//   從 const string& 建構（複製）
//   從 string&& 建構（移動）
//   從 string&& 建構（移動）
//   樣板變數仍可重複使用: "[daily] 系統健康報告已產生"
//   佇列共 3 則:
//     [daily] 系統健康報告已產生
//     [alert] 磁碟使用率 92%
//     [info] 排程任務全部完成
