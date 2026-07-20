// =============================================================================
//  第 21 課：getter 與 setter 設計模式 6  —  鏈式呼叫與 fluent interface
// =============================================================================
//
// 【主題資訊 Information】
//   語法：
//     DialogBox& setTitle(const std::string& t) { title_ = t; return *this; }
//     dlg.setTitle("警告").setMessage("...").setSize(400, 200).show();
//   關鍵：setter 回傳 *this 的「參考」,不是複本。
//   標準版本：C++98 起即有;ref-qualifier(&& 版本)為 C++11。
//   複雜度：每個 setter 皆 O(1),回傳參考不產生任何拷貝。
//   標頭檔：<string>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼回傳型別必須是 DialogBox&(參考)而不是 DialogBox(值)】
//   這是本主題唯一真正的重點,也是最常寫錯的地方。
//       DialogBox& setTitle(...) { ...; return *this; }   // ✅ 回傳自己
//       DialogBox  setTitle(...) { ...; return *this; }   // ❌ 回傳一份複本
//   若回傳值,`return *this` 會呼叫拷貝建構子產生一個「暫存複本」,
//   後續 .setMessage(...) 改的是那個暫存物件,而它在整條敘述結束後就被丟棄。
//   結果是:程式完全合法、不會崩潰、不會有警告,但除了第一個 setter 之外
//   的所有設定全部消失。這是鏈式呼叫最典型、也最難察覺的 bug。
//
// 【2. *this 是什麼】
//   this 是指向目前物件的指標(型別 DialogBox*),*this 就是把它解參考,
//   得到目前物件本身的左值。因此 `return *this;` 搭配回傳型別 `DialogBox&`
//   時,回傳的是「呼叫者自己」,而不是任何新物件。
//   this 指標的完整機制是第 26 課的主題,這裡先用到它最直接的一個應用。
//
// 【3. fluent interface 的適用與不適用】
//   適合:組態型物件(對話框、HTTP request、查詢建構器)——
//         一次要設定很多欄位,且欄位之間沒有順序相依。
//   不適合:有嚴格狀態機的物件。因為鏈式寫法會鼓勵「一口氣全設完」,
//         若某個步驟其實必須在另一步之後,語法上看不出來。
//   也不適合搭配「可能失敗」的操作:鏈式呼叫沒有地方回傳錯誤碼,
//         中途失敗只能丟例外,或像本檔一樣默默套用預設值。
//
// 【4. 本檔的驗證策略:靜默回退預設值】
//   setSize() 對非正值直接回退成 200x100,而不是回報錯誤。
//   這是刻意的取捨——鏈式介面沒有回傳錯誤的位置。實務上要留意:
//   靜默修正輸入會讓呼叫端不知道自己傳錯了。若正確性重要,
//   應改丟例外,或放棄鏈式、改用會回傳 bool 的一般 setter。
//
// 【概念補充 Concept Deep Dive】
//   * 鏈式呼叫的每一環都回傳參考,整條敘述完全沒有任何物件被複製,
//     組譯後基本上就是連續幾次成員寫入,零額外成本。
//   * C++11 起可用 ref-qualifier 區分左值/右值呼叫:
//       DialogBox&  setTitle(...) &;    // 左值物件呼叫
//       DialogBox&& setTitle(...) &&;   // 右值(暫存物件)呼叫,可回傳 &&
//     這讓 builder 能安全地支援 `Builder{}.setA().setB().build()` 的寫法。
//   * 這個模式在標準庫也看得到:std::ostream 的 operator<< 回傳
//     ostream&,正是為了讓 cout << a << b << c 能串起來。
//     本檔的 print() 裡每一行 cout << ... << ... 其實都在做同一件事。
//
// 【注意事項 Pay Attention】
//   1. 回傳型別務必是 T&。寫成 T 會產生暫存複本,設定悄悄遺失,
//      且沒有任何編譯警告——這是本主題最大的地雷。
//   2. 不要在鏈式 setter 上加 const:它們本來就要修改物件。
//   3. 鏈式呼叫的 setter 不要回傳 const T&,否則後續無法再接非 const 的 setter。
//   4. 對「暫存物件」做鏈式呼叫時,不可把中途結果綁成參考長期保存:
//        DialogBox& r = DialogBox{}.setTitle("x");   // 暫存物件已解構 → 懸空
//      之後使用 r 是未定義行為(UB),不保證任何特定結果。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】鏈式呼叫
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 要支援 obj.setA(1).setB(2).setC(3),setter 該怎麼宣告?為什麼?
//     答：回傳型別必須是「自身型別的非 const 左值參考」,並 return *this:
//             DialogBox& setA(int v) { a_ = v; return *this; }
//         回傳參考才能讓後續的 .setB() 作用在「同一個物件」上,
//         而且整條鏈完全沒有拷貝。
//     追問：為什麼不能回傳 const DialogBox&?→ 因為 const 參考上不能再呼叫
//         非 const 的 setter,鏈子第二環就編譯失敗。
//
// 🔥 Q2. this 和 *this 差在哪?為什麼寫 return *this 而不是 return this?
//     答：this 是指向目前物件的「指標」(DialogBox*),*this 是解參考後的
//         「物件本身」(左值)。回傳型別宣告成 DialogBox& 時,要回傳的是物件,
//         所以用 *this;若寫 return this 會是型別不符(指標 vs 參考),編譯失敗。
//     追問：那可以宣告成回傳 DialogBox* 並 return this 嗎?→ 語法上可以,
//         但呼叫端得寫 obj->setA(1)->setB(2),可讀性差,也無法自然支援暫存物件。
//
// ⚠️ 陷阱. 把回傳型別從 DialogBox& 手滑寫成 DialogBox,會發生什麼?
//     答：程式仍然編譯成功、執行不崩潰、零警告,但只有鏈上第一個 setter
//         真正生效。因為 return *this 在回傳值的情況下會拷貝出一個暫存物件,
//         之後的 .setMessage()/.setSize() 全都作用在那個暫存複本上,
//         整條敘述結束後複本即被丟棄。原物件只被改了第一項。
//     為什麼會錯：把 `return *this` 當成「回傳自己」的固定咒語,而忽略了
//         「回傳型別」才是決定回傳的是本體還是複本的關鍵。
//         *this 只是運算式,是否複製完全取決於函式簽章那個 & 有沒有寫。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 從缺,理由如下
//   鏈式呼叫是 API 人體工學(ergonomics)議題,LeetCode 的設計類題目
//   (如 155、1603)規定了固定的方法簽章,不會要求回傳 *this,
//   判題也不看呼叫端寫法。硬掛一題無法示範本檔重點(回傳 T& vs T),
//   故從缺;改以下方「HTTP 請求建構器」實務範例呈現真實用法。
//
// =============================================================================

#include <iostream>
#include <string>
using namespace std;

class DialogBox {
private:
    string title_;
    string message_;
    int width_;
    int height_;
    bool visible_;

public:
    DialogBox()
        : title_("未命名"), message_(""), width_(200), height_(100)
        , visible_(false)
    {
    }

    // setter 返回自身引用，支持鏈式調用
    // 注意：這種設計允許在一行中連續調用多個 setter，提升代碼的流暢性和可讀性
    // 例如：dlg.setTitle("警告").setMessage("確定要刪除嗎？").setSize(400, 200).show();
    // 注意：這些 setter 都返回 DialogBox&，允許鏈式調用
    // 注意：這些 setter 都帶有驗證邏輯，確保對象保持有效狀態
    DialogBox& setTitle(const string& title) {
        title_ = title;
        return *this;       // 返回自身, 支持鏈式調用
    }

    DialogBox& setMessage(const string& msg) {
        message_ = msg;
        return *this;       // 返回自身, 支持鏈式調用
    }

    DialogBox& setSize(int w, int h) {
        width_ = (w > 0) ? w : 200;
        height_ = (h > 0) ? h : 100;
        return *this;       // 返回自身, 支持鏈式調用
    }

    DialogBox& show() {
        visible_ = true;
        return *this;       // 返回自身, 支持鏈式調用
    }

    void print() const {
        cout << "  ┌";
        for (int i = 0; i < 30; i++) cout << "─";
        cout << "┐" << endl;
        cout << "  │ [" << title_ << "]" << endl;
        cout << "  │ " << message_ << endl;
        cout << "  │ Size: " << width_ << "x" << height_ << endl;
        cout << "  │ Visible: " << (visible_ ? "Yes" : "No") << endl;
        cout << "  └";
        for (int i = 0; i < 30; i++) cout << "─";
        cout << "┘" << endl;
    }
};

// -----------------------------------------------------------------------------
// 【對照組】把回傳型別寫成 T（值）而不是 T&（參考）會怎樣
//   這是本主題最重要的一個坑，值得實際跑一次看清楚。
//   注意：以下程式碼完全合法、無 UB、無警告，只是語意不是你要的。
//   每個 setter 回傳一份「複本」，後續設定都作用在那個暫存複本上，
//   整條敘述結束後複本被丟棄，原物件只保留了第一項設定。
// -----------------------------------------------------------------------------
class BrokenBox {
private:
    string title_;
    string message_;

public:
    BrokenBox() : title_("未命名"), message_("(空)") {}

    // ❌ 回傳「值」而非「參考」—— 鏈式呼叫會失效
    BrokenBox setTitle(const string& t)   { title_ = t;   return *this; }
    BrokenBox setMessage(const string& m) { message_ = m; return *this; }

    void print() const {
        cout << "    title=[" << title_ << "]  message=[" << message_ << "]" << endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】HTTP 請求建構器（fluent builder）
//   情境：發一個 API 請求要設 method / url / 逾時 / 重試 / 標頭，
//         參數多且彼此無順序相依 —— 正是 fluent interface 最適合的場景。
//   每個 setter 回傳 HttpRequest&，最後用 describe() 取得結果。
//   這也是 curl、OkHttp、requests 這類函式庫在各語言中的共通寫法。
// -----------------------------------------------------------------------------
class HttpRequest {
private:
    string method_;
    string url_;
    string body_;
    int    timeoutMs_;
    int    maxRetries_;
    bool   followRedirect_;
    string headers_;      // 為了不引入額外標頭，這裡用單一字串累加

public:
    HttpRequest()
        : method_("GET"), url_("/"), body_(""), timeoutMs_(5000)
        , maxRetries_(0), followRedirect_(true), headers_("") {}

    HttpRequest& method(const string& m)  { method_ = m;   return *this; }
    HttpRequest& url(const string& u)     { url_ = u;      return *this; }
    HttpRequest& body(const string& b)    { body_ = b;     return *this; }

    // 帶驗證的 setter：非法值靜默回退成預設（鏈式介面沒有回傳錯誤的位置）
    HttpRequest& timeoutMs(int ms)        { timeoutMs_  = (ms > 0) ? ms : 5000; return *this; }
    HttpRequest& retries(int n)           { maxRetries_ = (n >= 0) ? n : 0;     return *this; }
    HttpRequest& followRedirect(bool on)  { followRedirect_ = on;               return *this; }

    HttpRequest& header(const string& k, const string& v) {
        if (!headers_.empty()) headers_ += ", ";
        headers_ += k + ": " + v;
        return *this;
    }

    void describe() const {
        cout << "    " << method_ << " " << url_ << endl;
        cout << "    timeout=" << timeoutMs_ << "ms  retries=" << maxRetries_
             << "  followRedirect=" << (followRedirect_ ? "true" : "false") << endl;
        cout << "    headers: " << (headers_.empty() ? "(無)" : headers_) << endl;
        cout << "    body: "    << (body_.empty()    ? "(無)" : body_)    << endl;
    }
};

int main() {
    cout << "=== 鏈式調用 ===" << endl;

    // 傳統方式：一行一行設定
    cout << "\n--- 傳統方式 ---" << endl;
    DialogBox dlg1;
    dlg1.setTitle("警告");
    dlg1.setMessage("你確定要刪除嗎？");
    dlg1.setSize(400, 200);
    dlg1.show();
    dlg1.print();

    // 鏈式調用：一氣呵成
    cout << "\n--- 鏈式調用 ---" << endl;
    DialogBox dlg2;
    dlg2.setTitle("歡迎")
        .setMessage("歡迎回到遊戲世界！")
        .setSize(500, 250)
        .show();
    dlg2.print();

    // ─────────────────────────────────────────────────────────
    cout << "\n=== 對照組：回傳「值」而非「參考」的後果 ===" << endl;
    BrokenBox bad;
    bad.setTitle("警告").setMessage("這行設定會不見");
    cout << "  鏈式呼叫後，原物件實際狀態：" << endl;
    bad.print();
    cout << "  ↑ 只有第一個 setTitle 生效；setMessage 作用在暫存複本上，" << endl;
    cout << "    整條敘述結束後複本即被丟棄（合法程式碼，但語意錯誤）" << endl;

    // ─────────────────────────────────────────────────────────
    cout << "\n=== 日常實務：HTTP 請求建構器 ===" << endl;
    HttpRequest req;
    req.method("POST")
       .url("https://api.example.com/v1/orders")
       .header("Content-Type", "application/json")
       .header("Authorization", "Bearer <token>")
       .body(R"({"sku":"A-100","qty":2})")
       .timeoutMs(3000)
       .retries(2)
       .followRedirect(false);
    req.describe();

    cout << "\n  非法參數會靜默回退成預設值：" << endl;
    HttpRequest bad2;
    bad2.url("/health").timeoutMs(-1).retries(-5);
    bad2.describe();

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 21 課：getter 與 setter 設計模式6.cpp" -o l21_6
// 執行: ./l21_6        (rc=0)

// === 預期輸出 ===
// === 鏈式調用 ===
//
// --- 傳統方式 ---
//   ┌──────────────────────────────┐
//   │ [警告]
//   │ 你確定要刪除嗎？
//   │ Size: 400x200
//   │ Visible: Yes
//   └──────────────────────────────┘
//
// --- 鏈式調用 ---
//   ┌──────────────────────────────┐
//   │ [歡迎]
//   │ 歡迎回到遊戲世界！
//   │ Size: 500x250
//   │ Visible: Yes
//   └──────────────────────────────┘
//
// === 對照組：回傳「值」而非「參考」的後果 ===
//   鏈式呼叫後，原物件實際狀態：
//     title=[警告]  message=[(空)]
//   ↑ 只有第一個 setTitle 生效；setMessage 作用在暫存複本上，
//     整條敘述結束後複本即被丟棄（合法程式碼，但語意錯誤）
//
// === 日常實務：HTTP 請求建構器 ===
//     POST https://api.example.com/v1/orders
//     timeout=3000ms  retries=2  followRedirect=false
//     headers: Content-Type: application/json, Authorization: Bearer <token>
//     body: {"sku":"A-100","qty":2}
//
//   非法參數會靜默回退成預設值：
//     GET /health
//     timeout=5000ms  retries=0  followRedirect=true
//     headers: (無)
//     body: (無)
