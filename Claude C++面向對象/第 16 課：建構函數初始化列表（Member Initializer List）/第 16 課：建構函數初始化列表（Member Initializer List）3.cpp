// =============================================================================
//  第 16 課：初始化列表 3  —  必用場景之二：參考成員（T&）
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  class X { std::ostream& out; public: X(std::ostream& o) : out(o) {} };
//   標準版本：C++98 起即有
//   複雜度：O(1)；參考成員在物件內通常佔一個指標大小（**實作定義**，
//           標準並未規定參考是否佔空間，但實務上都以指標實作）
//   標頭檔：<iostream>、<string>
//
// 【詳細解釋 Explanation】
//
// 【1. 參考的本質：它是「別名」，不是「可以改指向的變數」】
//   int& r = x;  代表「從現在起 r 就是 x 的另一個名字」。
//   之後寫 r = 5; 不是「讓 r 改指向 5」，而是「把 5 寫進 x」。
//   換句話說，參考一旦綁定就**永遠**綁在同一個對象上，無法重新綁定。
//   這個性質直接推導出兩件事：
//     ● 參考必須在誕生的那一刻就綁定 → 不能有「尚未綁定」的參考
//     ● 參考成員只能在初始化列表初始化 → 函數體內已經太遲了
//
// 【2. 為什麼函數體內的 output = os; 不是綁定】
//   如果你寫成：
//       Logger(ostream& os) { output = os; }
//   編譯器有兩種反應，都不是你要的：
//     (a) 若參考成員完全沒在初始化列表出現 → 直接報錯
//         error: uninitialized reference member 'Logger::output'
//     (b) 就算它能編（例如型別可指派），output = os; 的語意也是
//         「把 os 的內容**賦值給 output 所綁定的那個物件**」，
//         而不是「讓 output 改綁到 os」——因為參考不能重新綁定。
//   所以參考成員與 const 成員一樣，唯一的初始化管道就是初始化列表。
//
// 【3. 參考成員的最大風險：生命週期】
//   參考成員不擁有它指涉的物件，只是借用。所以你必須確保
//   **被指涉的物件活得比這個類別的物件久**，否則就是懸空參考（dangling）。
//   本檔的 Logger 綁定的是 std::cout，那是全域資料流，
//   生命週期涵蓋整個程式，所以絕對安全——這也是這個範例挑 ostream& 的原因。
//   反例（危險）：
//       Logger makeLogger() {
//           std::ostringstream local;
//           return Logger(local, "X");   // local 離開函數就沒了 → 懸空
//       }
//
// 【4. 參考成員的連帶影響：複製指派被刪除】
//   跟 const 成員一樣，因為參考不能重新綁定，編譯器無法生成有意義的
//   operator=，所以**隱式的複製指派運算子被定義為 deleted**。
//   複製建構仍然可以（新物件的參考綁到同一個對象）。
//   因此帶參考成員的類別同樣不能放進需要賦值的容器操作。
//
// 【5. 什麼時候該用參考成員，什麼時候該用指標】
//   ● 參考成員：依賴關係是**必需且固定**的（一定有、且永不更換）。
//     好處是語法乾淨、不必檢查空值。
//   ● 指標成員：依賴關係是**可選或可更換**的（可能沒有、或中途要換）。
//     代價是要處理空指標。
//   本檔的 Logger 一定要有輸出目標，而且不會中途換，所以參考成員很合適。
//
// 【概念補充 Concept Deep Dive】
//
//   ● 參考在記憶體中佔不佔空間
//     標準沒有規定。實務上，作為類別成員的參考幾乎都以指標實作，
//     所以 sizeof 會反映出一個指標的大小——但這是**實作定義**，不可依賴。
//     獨立的區域參考則常被編譯器完全最佳化掉。
//
//   ● 為什麼不能有「參考的陣列」或「參考的參考」
//     因為參考不是物件，只是別名。沒有物件就沒有位址，
//     也就無法構成陣列元素。需要「一群可更換的引用」時請用指標或
//     std::reference_wrapper（後者可指派，正是為了填補這個空缺）。
//
//   ● std::reference_wrapper：可指派的參考
//     若你需要「像參考但可以重新綁定」，標準提供 std::reference_wrapper<T>
//     （<functional>）。它內部存指標，因此可以複製指派，
//     也因此可以放進 std::vector。
//
//   ● 依賴注入（dependency injection）
//     本檔的 Logger 把「往哪裡輸出」變成建構參數，而不是寫死 std::cout。
//     這讓測試時可以傳入 std::ostringstream 攔截輸出來做驗證——
//     這正是依賴注入最實際的好處，本檔的實務範例會示範。
//
// 【注意事項 Pay Attention】
//   1. 參考成員不擁有對象；務必確保被指涉者活得比自己久。
//   2. 參考成員會讓隱式的複製指派運算子被刪除（與 const 成員相同）。
//   3. 需要「可更換的引用」請用指標或 std::reference_wrapper。
//   4. 參考成員的宣告順序同樣影響初始化順序，不要在初始化列表中
//      用一個尚未初始化的成員去綁定另一個成員。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】參考成員
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼參考成員一定要用初始化列表？
//     答：參考是別名，必須在誕生的那一刻綁定對象，而且之後無法重新綁定。
//         函數體內的 = 對參考而言是「寫入它所綁定的物件」，不是重新綁定；
//         而在進入函數體之前，參考若沒被初始化就已經是不合法的狀態。
//         所以唯一的綁定時機就是初始化列表。
//     追問：那參考成員可以不初始化嗎？
//         → 不行，編譯器會報 uninitialized reference member。
//
// 🔥 Q2. 帶參考成員的類別，可以複製嗎？可以指派嗎？
//     答：可以複製建構（新物件的參考綁到同一個對象），
//         但**不能複製指派**——隱式的 operator= 會被定義為 deleted，
//         因為參考無法重新綁定。這點與 const 成員一樣。
//     追問：那要怎麼讓它可指派？
//         → 改用指標成員，或用 std::reference_wrapper<T>（內部存指標，可指派）。
//
// ⚠️ 陷阱. 參考成員比指標安全，因為參考「不會是空的」，所以不用擔心懸空，對嗎？
//     答：不對。參考確實不會是「空」的，但完全可能懸空——只要被指涉的物件
//         比類別物件先死亡，之後任何使用都是未定義行為，而且沒有 nullptr
//         可以檢查，反而更難察覺。
//     為什麼會錯：把「不會是 nullptr」誤解成「一定有效」。參考只保證
//         綁定時是合法對象，不保證那個對象一直活著；
//         生命週期管理仍然是你的責任。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <sstream>
#include <string>
using namespace std;

class Logger {
private:
    ostream& output;   // 參考成員：必須在初始化時綁定，且之後不可更換
    string prefix;

public:
    // 錯誤寫法（函數體內賦值）：
    //   Logger(ostream& os, const string& p) {
    //       output = os;   // 這不是綁定，而是寫入 output 所指涉的物件；
    //                      // 而且 output 根本還沒被初始化 → 編譯錯誤
    //       prefix = p;
    //   }

    // 正確寫法：參考成員只能在初始化列表綁定
    Logger(ostream& os, const string& p)
        : output(os), prefix(p)
    { }

    void log(const string& message) const {
        output << "[" << prefix << "] " << message << endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】依賴注入 + 單元測試：把輸出目標抽換成字串流來驗證
//   情境：一個「訂單處理器」在處理過程中會寫日誌。正式執行時寫到 std::cout
//         （或檔案），但單元測試時我們需要**攔截**這些輸出來斷言內容正確。
//   重點：因為輸出目標是建構時注入的參考，測試可以傳 std::ostringstream，
//         完全不必改動被測程式碼。這是參考成員最有價值的實務用途。
// -----------------------------------------------------------------------------
class OrderProcessor {
private:
    Logger& logger_;    // 注入的相依物件；一定要有、且不會更換 → 用參考
    int processed_;

public:
    explicit OrderProcessor(Logger& logger)
        : logger_(logger), processed_(0)
    { }

    void process(const string& orderId, double amount) {
        if (amount <= 0.0) {
            logger_.log("訂單 " + orderId + " 金額不合法，已拒絕");
            return;
        }
        ++processed_;
        logger_.log("訂單 " + orderId + " 處理完成");
    }

    int processedCount() const { return processed_; }
};

int main() {
    cout << "=== 參考成員：綁定到 std::cout ===" << endl;
    Logger consoleLogger(cout, "INFO");
    consoleLogger.log("程式啟動");
    consoleLogger.log("初始化完成");

    cout << "\n=== 同一個類別，綁定到不同的輸出目標 ===" << endl;
    Logger errLogger(cerr, "ERROR");
    cout << "  （下一行會寫到 stderr，在終端上看起來與 stdout 混在一起）" << endl;
    errLogger.log("這行走的是 cerr");

    cout << "\n=== 日常實務：依賴注入讓輸出可被攔截測試 ===" << endl;
    // 正式路徑：寫到 cout
    Logger prodLogger(cout, "PROD");
    OrderProcessor prod(prodLogger);
    prod.process("A-1001", 1250.0);
    prod.process("A-1002", -5.0);
    cout << "  正式處理成功筆數: " << prod.processedCount() << endl;

    // 測試路徑：把輸出接到字串流，事後檢查內容
    ostringstream captured;
    Logger testLogger(captured, "TEST");
    OrderProcessor underTest(testLogger);
    underTest.process("B-2001", 99.0);
    underTest.process("B-2002", 0.0);

    cout << "\n  --- 被攔截下來的日誌內容 ---" << endl;
    cout << captured.str();
    cout << "  測試處理成功筆數: " << underTest.processedCount() << endl;

    // 簡易斷言：確認拒絕訊息真的有被寫出來
    bool rejected = captured.str().find("金額不合法") != string::npos;
    cout << "  斷言「有記錄到金額不合法」: " << (rejected ? "通過" : "失敗") << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 16 課：建構函數初始化列表（Member Initializer List）3.cpp" -o demo3
//
// ※ 註：上面「這行走的是 cerr」實際輸出到 stderr。下方預期輸出是以
//   2>&1 合併擷取的結果；若只看 stdout 就不會有那一行。
//   cerr 預設不緩衝，與 cout 的相對順序在合併時可能因平台而異。

// === 預期輸出 ===
// === 參考成員：綁定到 std::cout ===
// [INFO] 程式啟動
// [INFO] 初始化完成
//
// === 同一個類別，綁定到不同的輸出目標 ===
//   （下一行會寫到 stderr，在終端上看起來與 stdout 混在一起）
// [ERROR] 這行走的是 cerr
//
// === 日常實務：依賴注入讓輸出可被攔截測試 ===
// [PROD] 訂單 A-1001 處理完成
// [PROD] 訂單 A-1002 金額不合法，已拒絕
//   正式處理成功筆數: 1
//
//   --- 被攔截下來的日誌內容 ---
// [TEST] 訂單 B-2001 處理完成
// [TEST] 訂單 B-2002 金額不合法，已拒絕
//   測試處理成功筆數: 1
//   斷言「有記錄到金額不合法」: 通過
