// =============================================================================
//  第 15 課：帶參數的建構函數 9  —  單參數建構函數與隱式轉換
// =============================================================================
//
// 【主題資訊 Information】
//   概念：只有一個參數（或其餘參數都有預設值）且未標 explicit 的建構函數，
//         會成為「轉換建構函數 converting constructor」，
//         讓編譯器可以自動把參數型別轉成類別型別。
//   觸發時機：複製初始化、函數傳參、回傳值、運算子的運算元
//   標準版本：C++98 起即有；C++11 起大括號初始化也適用同一套規則
//   複雜度：轉換本身就是一次建構，O(建構成本)——通常不便宜
//   標頭檔：<string>
//
// 【詳細解釋 Explanation】
//
// 【1. 什麼是轉換建構函數】
//   Distance(double m) 這種「一個參數」的建構函數，除了讓你明確建立物件，
//   還悄悄賦予編譯器一項權力：**在需要 Distance 的地方，自動把 double 變成
//   Distance**。這條轉換路徑叫做使用者定義轉換（user-defined conversion）。
//   所以下面兩行是等價的：
//       Distance d = 50.0;              // 編譯器補成 Distance(50.0)
//       showDistance(200.0);            // 編譯器補成 showDistance(Distance(200.0))
//
// 【2. 為什麼這個功能存在】
//   因為它有時候非常好用，讓 API 讀起來很自然：
//       std::string s = "hello";        // const char* → std::string
//       std::vector<int> v; v.push_back(3);
//   如果沒有隱式轉換，你得到處寫 std::string("hello")，很囉唆。
//   標準函式庫大量依賴這個機制。
//
// 【3. 為什麼它同時是個危險特性】
//   問題在於「悄悄」。編譯器最多只會插入**一次**使用者定義轉換，
//   但這一次就足以讓錯誤的呼叫通過編譯：
//       void setTimeout(Distance d);
//       setTimeout(30);                 // 我以為傳的是「30 秒」，
//                                       // 實際上建了一個「30 公尺」的 Distance
//   型別系統本來應該擋下這種語意錯誤，隱式轉換卻把牆拆了。
//   更麻煩的是它會產生看不見的建構成本——本檔輸出中每次隱式轉換都會印出
//   一行「建構 Distance」，就是要讓你親眼看到那些你沒寫出來的物件。
//
// 【4. 判準：這個轉換「在語意上成立」嗎】
//   ● 合理：型別只是同一個概念的不同表示法。
//     const char* → std::string（都是字串）、int → std::complex（實數是複數的特例）
//   ● 不合理：參數只是「用來建構」的原料，不是同一個東西。
//     int → std::vector（10 不是「一個 vector」，它是「大小」）——
//     所以 std::vector 的 explicit vector(size_type n) 就標了 explicit。
//   本檔的 Distance(double) 屬於灰色地帶：一個裸 double 到底是幾公尺、
//   幾公里、還是幾英尺？語意其實不明確，因此實務上會建議標 explicit
//   （這正是下一個檔案 10.cpp 的主題）。
//
// 【5. 只會轉一次】
//   編譯器最多插入一次使用者定義轉換。所以若有 A→B、B→C 兩個轉換建構函數，
//   傳 A 給需要 C 的函數**不會**自動走兩步，會編譯失敗。
//   （內建型別之間的標準轉換，例如 int→double，不算在這一次額度內，
//     所以 showDistance(200) 傳 int 仍然可以：int→double 是標準轉換，
//     double→Distance 才是那一次使用者定義轉換。）
//
// 【概念補充 Concept Deep Dive】
//
//   ● 「單參數」的精確定義
//     不只是恰好一個參數。只要「可以用一個引數呼叫」就算，例如
//         Rect(int w, int h = 10);        // 可用 Rect r = 5; → 也是轉換建構函數
//     反之，C++11 起若參數包含 initializer_list，規則會更複雜。
//
//   ● 轉換也可以反過來做：轉換運算子
//         operator double() const { return meters; }
//     這會讓 Distance 自動變回 double。同樣要小心，通常建議標 explicit，
//     或改成具名函數 getMeters()（本檔採用具名函數的作法）。
//
//   ● 隱式轉換與重載決議的交互作用
//     當多個重載都需要不同的使用者定義轉換時，容易產生歧義而編譯失敗；
//     錯誤訊息通常很長很難讀。標 explicit 可以大幅減少這類問題。
//
//   ● 臨時物件的生命週期
//     showDistance(200.0) 產生的臨時 Distance，活到該完整運算式結束為止。
//     所以在函數內使用它是安全的；但若函數把它的位址存起來，就會懸空。
//
// 【注意事項 Pay Attention】
//   1. 隱式轉換會產生你沒寫出來的建構呼叫，成本是隱形的。
//   2. 語意不明確的單參數建構函數請標 explicit（見 10.cpp）。
//   3. 編譯器最多只插入一次使用者定義轉換，不會連鎖。
//   4. 標準函式庫的慣例值得參考：std::string(const char*) 不標 explicit
//      （語意相符），std::vector(size_type) 標 explicit（語意不符）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】單參數建構函數與隱式轉換
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是轉換建構函數？Distance d = 50.0; 背後發生了什麼？
//     答：未標 explicit 的單參數建構函數會成為轉換建構函數，允許編譯器把
//         double 隱式轉成 Distance。Distance d = 50.0; 是複製初始化，
//         編譯器補成 Distance(50.0)；C++17 起因保證的複製省略，
//         直接就地建構，沒有多餘的臨時物件與複製。
//     追問：那 showDistance(200.0) 產生的臨時物件活多久？
//         → 活到該完整運算式（整個函數呼叫）結束為止。
//
// 🔥 Q2. 如果有 A→B 和 B→C 兩個轉換建構函數，把 A 傳給吃 C 的函數可以嗎？
//     答：不行。編譯器最多插入**一次**使用者定義轉換，不會自動連鎖兩次。
//         必須自己寫成 f(C(B(a))) 之類的明確轉換。
//     追問：那為什麼 showDistance(200) 傳 int 卻可以？
//         → int→double 是標準轉換，不佔用那一次額度；
//           真正的使用者定義轉換只有 double→Distance 這一次。
//
// ⚠️ 陷阱. 隱式轉換只是「方便」，反正結果一樣，對嗎？
//     答：不對。它會製造出你沒寫出來的物件，帶來看不見的建構成本；
//         更嚴重的是它會讓語意錯誤的呼叫通過編譯——例如把代表「秒數」的 30
//         悄悄變成代表「公尺」的 Distance，型別系統本來該擋下這種錯誤。
//     為什麼會錯：把型別轉換想成「只是換個包裝，值沒變」。但型別的意義
//         不只是儲存格式，它還攜帶「這個數字代表什麼」的語意；
//         隱式轉換等於允許編譯器替你猜語意。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
using namespace std;

class Distance {
private:
    double meters;

public:
    // 單參數且未標 explicit → 這是「轉換建構函數」
    // 代價：編譯器可以在任何需要 Distance 的地方，自動把 double 轉過來
    Distance(double m) : meters(m) {
        cout << "    [建構 Distance: " << meters << " m]" << endl;
    }

    // 用具名函數取出值，而不是提供 operator double()
    // 這樣「轉回 double」也必須明講，避免雙向隱式轉換造成歧義
    double getMeters() const { return meters; }

    void print() const {
        cout << "  距離: " << meters << " 公尺" << endl;
    }
};

void showDistance(Distance d) {
    d.print();
}

// -----------------------------------------------------------------------------
// 【日常實務範例】設定逾時的 API：隱式轉換如何讓語意錯誤溜過型別檢查
//   情境：連線設定同時有「逾時秒數」與「重試次數」兩個概念，兩者都是整數。
//         若各自包成型別卻允許隱式轉換，傳錯順序時編譯器不會吭聲。
//   對照：Seconds 允許隱式轉換（危險）、Retries 標了 explicit（安全）。
//         這是 10.cpp explicit 主題的預告。
// -----------------------------------------------------------------------------
class Seconds {
private:
    int v_;
public:
    Seconds(int v) : v_(v) { }            // 未標 explicit → 可隱式轉換
    int value() const { return v_; }
};

class Retries {
private:
    int v_;
public:
    explicit Retries(int v) : v_(v) { }   // 標了 explicit → 必須明講
    int value() const { return v_; }
};

void connectLoose(Seconds timeout) {
    cout << "  [寬鬆版] 逾時設為 " << timeout.value() << " 秒" << endl;
}

void connectStrict(Retries retries) {
    cout << "  [嚴格版] 重試次數設為 " << retries.value() << " 次" << endl;
}

int main() {
    cout << "=== 正常使用：明確建立物件 ===" << endl;
    Distance d1(100.0);
    d1.print();

    cout << "\n=== 隱式轉換：複製初始化 ===" << endl;
    // 編譯器補成 Distance(50.0)；注意輸出多了一行「建構」
    Distance d2 = 50.0;
    d2.print();

    cout << "\n=== 隱式轉換：函數參數 ===" << endl;
    // 等同於 showDistance(Distance(200.0))，臨時物件活到這行結束
    showDistance(200.0);

    cout << "\n=== 隱式轉換：傳 int 也可以 ===" << endl;
    // int→double 是標準轉換（不佔額度），double→Distance 才是
    // 那一次使用者定義轉換，所以合法
    showDistance(300);

    cout << "\n=== 日常實務：語意錯誤如何溜過型別檢查 ===" << endl;
    // 寬鬆版：直接傳一個裸 int 就通過了，看不出來這個 5 是什麼單位
    connectLoose(5);
    // 嚴格版：explicit 逼你把單位寫出來，語意一目了然
    connectStrict(Retries(5));
    // connectStrict(5);   // 編譯錯誤：explicit 禁止隱式轉換（正是我們要的）
    cout << "  connectStrict(5) 會編譯失敗 —— explicit 擋下了語意不明的呼叫" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 15 課：帶參數的建構函數9.cpp" -o demo9

// === 預期輸出 ===
// === 正常使用：明確建立物件 ===
//     [建構 Distance: 100 m]
//   距離: 100 公尺
//
// === 隱式轉換：複製初始化 ===
//     [建構 Distance: 50 m]
//   距離: 50 公尺
//
// === 隱式轉換：函數參數 ===
//     [建構 Distance: 200 m]
//   距離: 200 公尺
//
// === 隱式轉換：傳 int 也可以 ===
//     [建構 Distance: 300 m]
//   距離: 300 公尺
//
// === 日常實務：語意錯誤如何溜過型別檢查 ===
//   [寬鬆版] 逾時設為 5 秒
//   [嚴格版] 重試次數設為 5 次
//   connectStrict(5) 會編譯失敗 —— explicit 擋下了語意不明的呼叫
