// =============================================================================
// 主題: 值類別如何決定「複製」還是「移動」—— 按值傳參的實際成本
// =============================================================================
//
// 【主題資訊 Information】
//   語法    ：void f(T param);        // 按值接收
//             f(x);                   // lvalue → 呼叫複製建構子
//             f(std::move(x));        // xvalue → 呼叫移動建構子
//   標準版本：右值參考 / std::move / 移動建構子  皆為 C++11
//   標頭檔  ：<utility>（std::move）
//   複雜度  ：複製為 O(N)（N = 字串長度）；移動為 O(1)（只搬指標）
//   本檔宣告的標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 按值傳參不等於「一定要複製」】
//   void process(std::string s) 的參數 s 是一個全新的物件，
//   它必須從引數「初始化」而來。用哪個建構子，取決於引數的 value category：
//       process(original);              // original 是 lvalue → 複製建構子
//       process(std::move(original));   // 是 xvalue          → 移動建構子
//   同一個函式簽章，兩種完全不同的成本 —— 決定權在呼叫端。
//
// 【2. 為什麼移動是 O(1)】
//   std::string 內部持有一個指向堆積字元陣列的指標。
//   複製要另外配置一塊同樣大的記憶體並逐字元拷貝（O(N)）；
//   移動只要把指標抄過來、再把來源的指標設成空（O(1)），
//   完全不碰堆積上的字元資料。
//
// 【3. 移動之後 original 變成什麼】
//   標準只保證「有效但未指定（valid but unspecified）」：
//   可以安全銷毀、可以重新賦值、可以呼叫沒有前提條件的操作。
//   但「內容是什麼」不保證 —— 本檔的字串較長（走堆積），
//   本實作觀察到它變成空字串；短字串因 SSO 可能整段複製而內容不變。
//   因此不可寫出依賴「移動後必為空」的邏輯。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 這正是「按值傳參 + std::move」慣例的基礎
//     現代 C++ 常見寫法：
//         Widget(std::string name) : m_name(std::move(name)) {}
//     呼叫端傳 lvalue → 1 次複製 + 1 次移動；
//     呼叫端傳 rvalue → 1 次移動 + 1 次移動（移動極便宜）。
//     這讓一個函式同時服務兩種呼叫端，不必寫 const&/&& 兩份重載。
//
// (B) 什麼時候「不該」對它 std::move
//     只有在「這個物件之後不會再被使用」時才可以移動。
//     若 original 後面還要讀取，移動它就是把自己的資料丟掉。
//     判斷準則永遠是：這是它最後一次被使用嗎？
//
// 【注意事項 Pay Attention】
//   1. std::move 只是型別轉換，真正搬東西的是被選中的移動建構子。
//   2. 被移動後的物件處於「有效但未指定」狀態，不可假設它一定變空。
//   3. 移動之後除了「重新賦值」或「銷毀」外，不要再讀取它的值。
//   4. 對 const 物件 std::move 會安靜退回複製（const 物件無法被掏空）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】按值傳參與移動
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. void process(std::string s) 按值接收，傳入時一定會複製嗎？
//     答：不一定。參數 s 由引數初始化，用哪個建構子取決於引數的 value category：
//         傳 lvalue → 複製建構子（O(N)）；傳右值 → 移動建構子（O(1)）。
//         同一個簽章，成本由呼叫端決定。
//     追問：那按值傳參會不會比 const& 慢？→ 傳 lvalue 時多一次複製；
//         但它換來的是「傳 rvalue 時可以移動」，且不必寫兩份重載。
//         除非在極熱路徑，按值 + std::move 是可讀性與效能的好平衡。
//
// ⚠️ 陷阱. std::move(original) 之後，original 的內容一定是空字串嗎？
//     答：不保證。標準只說「有效但未指定」。
//         本檔的字串較長、走堆積配置，本實作觀察到變成空字串；
//         但短字串因 SSO（小字串最佳化）存在物件內部，
//         實作可以選擇直接複製字元而讓來源原封不動。
//     為什麼會錯：把「移動」想成一個保證會清空來源的動作。
//         實際上標準只規範「來源仍處於可安全銷毀/可重新賦值的狀態」，
//         內容屬於實作細節。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <utility>
#include <vector>

void process(std::string s) {
    std::cout << "  processing: " << s << "\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】把解析結果交給下游 —— 一次都不複製
//   情境：從原始日誌行解析出訊息內容，交給處理管線的下一站。
//         中介層的職責只是「轉手」，沒有理由複製 payload。
//   為什麼用到本主題：
//     parseMessage 產生的是暫時字串（prvalue）→ 直接移動進容器；
//     而 sink 收到後也是最後一站 → 再移動進儲存區。
//     全程只搬指標，字元資料一次都沒有被複製。
// -----------------------------------------------------------------------------
std::string parseMessage(const std::string& logLine) {
    std::size_t p = logLine.find("] ");
    if (p == std::string::npos) return {};
    return logLine.substr(p + 2);     // 回傳 prvalue，呼叫端可直接移動
}

class MessageSink {
    std::vector<std::string> m_stored;
public:
    // 按值接收 + move：呼叫端傳 rvalue 時全程零複製
    void accept(std::string msg) {
        m_stored.push_back(std::move(msg));
    }
    std::size_t count() const { return m_stored.size(); }
    const std::string& at(std::size_t i) const { return m_stored[i]; }
};

int main() {
    std::string original = "This is a long string that lives on the heap";

    std::cout << "=== 傳入左值（觸發複製）===\n";
    process(original);  // original 是左值 → 複製
    std::cout << "original after copy: " << original << "\n\n";

    std::cout << "=== 傳入右值（觸發移動）===\n";
    process(std::move(original));  // std::move(original) 是 xvalue → 移動
    std::cout << "original after move: \"" << original << "\"\n";
    std::cout << "  ⚠️ 標準只保證「有效但未指定」。此處觀察到空字串是本實作\n";
    std::cout << "     對長字串的行為，短字串（SSO）可能內容不變，不可依賴。\n";

    // 移動後唯一安全的操作：重新賦值或銷毀
    original = "重新賦值後即可正常使用";
    std::cout << "  重新賦值後: \"" << original << "\"\n\n";

    // =========================================================================
    // 日常實務：解析 → 轉手 → 儲存，全程零複製
    // =========================================================================
    std::cout << "=== 日常實務：日誌解析管線（全程移動）===\n";

    const std::string rawLines[] = {
        "2026-07-19 10:00:01 [INFO] service started successfully",
        "2026-07-19 10:00:05 [WARN] cache miss rate is above threshold",
        "2026-07-19 10:00:07 [ERROR] database connection refused",
        "這行沒有分隔符號會被略過"
    };

    MessageSink sink;
    for (const std::string& line : rawLines) {
        std::string msg = parseMessage(line);   // 回傳 prvalue
        if (msg.empty()) {
            std::cout << "  略過無法解析的行\n";
            continue;
        }
        sink.accept(std::move(msg));            // msg 有名字是 lvalue → 須明寫 move
    }

    std::cout << "  已收集 " << sink.count() << " 則訊息：\n";
    for (std::size_t i = 0; i < sink.count(); ++i)
        std::cout << "    " << sink.at(i) << "\n";
    std::cout << "  全程只搬指標，字元資料一次都沒有被複製。\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.1 章：左值與右值的定義 — Value Categories 完整解析5.cpp" -o vc_move_demo

// === 預期輸出 ===
// === 傳入左值（觸發複製）===
//   processing: This is a long string that lives on the heap
// original after copy: This is a long string that lives on the heap
//
// === 傳入右值（觸發移動）===
//   processing: This is a long string that lives on the heap
// original after move: ""
//   ⚠️ 標準只保證「有效但未指定」。此處觀察到空字串是本實作
//      對長字串的行為，短字串（SSO）可能內容不變，不可依賴。
//   重新賦值後: "重新賦值後即可正常使用"
//
// === 日常實務：日誌解析管線（全程移動）===
//   略過無法解析的行
//   已收集 3 則訊息：
//     service started successfully
//     cache miss rate is above threshold
//     database connection refused
//   全程只搬指標，字元資料一次都沒有被複製。
