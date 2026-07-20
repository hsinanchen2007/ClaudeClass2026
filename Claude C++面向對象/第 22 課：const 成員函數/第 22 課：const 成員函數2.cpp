// =============================================================================
//  第 22 課：const 成員函數 2  —  const 物件與 const 參考參數
// =============================================================================
//
// 【主題資訊 Information】
//   三種 const 情境:
//     const Shield s(...);        // const 物件:一生只能呼叫 const 成員函式
//     const Shield& r = s;        // const 參考:透過它只看得到 const 介面
//     void f(const Shield& s);    // const 參數:最常見的傳參方式
//   標準版本:C++98 起即有。
//   複雜度:const 為編譯期概念,零執行期成本;傳 const& 也避免了拷貝。
//   標頭檔:<string>
//
// 【詳細解釋 Explanation】
//
// 【1. const 物件不是「唯讀資料」,而是「只露出唯讀介面」】
//   const Shield legendaryShield(...) 之後,這個物件的可用介面立刻縮減成
//   「所有標了 const 的成員函式」。takeDamage()/repair() 不是「呼叫會失敗」,
//   而是「根本無法通過編譯」。這個檢查發生在編譯期,不是執行期判斷。
//
// 【2. 為什麼 const T& 是 C++ 最常用的傳參方式】
//   void inspectShield(const Shield& s) 同時達成三件事:
//     (a) 不拷貝 —— 傳的是參考,對大型物件省下整份深拷貝成本。
//     (b) 不修改 —— const 保證函式不會動到呼叫端的物件。
//     (c) 可接受各種來源 —— 非 const 物件、const 物件、暫存物件都能綁定。
//   注意 (c):非 const 物件可以綁到 const&(加上 const 是允許的隱式轉換),
//   反過來則不行 —— const 物件不能綁到非 const 參考。這個單向性
//   正是 const 正確性能成立的基礎。
//
// 【3. 「加 const 容易,拿掉 const 很難」】
//   本檔 inspectShield(shield) 傳的是非 const 的 shield,完全合法。
//   但若有個函式參數是 Shield&(非 const),就無法傳入 const 物件。
//   所以介面設計上,「參數盡量宣告成 const&」會讓函式的可用範圍最大;
//   反之,少寫一個 const 就會把 const 物件全部擋在門外。
//
// 【4. const 物件的成員也全部變成 const】
//   對 const Shield 而言,它的 name_/defense_/durability_ 在該物件的
//   整個生命期內都是 const。這也解釋了為何 const 物件必須在建構時
//   就完成初始化 —— 之後沒有任何合法途徑可以再賦值。
//
// 【概念補充 Concept Deep Dive】
//   * 綁定規則的方向性:Shield → const Shield& 是合法的隱式轉換(qualification
//     conversion);const Shield → Shield& 則不合法。這是單向的。
//   * const 參考可以延長暫存物件的生命期:
//         const Shield& r = Shield("臨時", 1, 1);
//     暫存物件的生命期會延長到 r 的作用域結束。但這個規則只適用於
//     「直接綁定到暫存物件」,不適用於「綁定到暫存物件的成員的回傳值」。
//   * 對 const 物件呼叫 const 成員函式,編譯後的呼叫指令與非 const 物件
//     完全相同 —— const 不影響程式碼產生,只影響編譯期的可呼叫性判定。
//
// 【注意事項 Pay Attention】
//   1. const 物件必須在宣告時初始化,之後無法賦值。
//   2. 若某個 getter 忘了加 const,const 物件就再也讀不到那個值;
//      這時正確的修法是去補那個 const,不是用 const_cast 硬解。
//   3. const_cast 拿掉 const 之後,若真的去修改「原本就宣告為 const 的物件」,
//      那是未定義行為(UB),不保證任何特定結果 —— 包含「看起來正常運作」。
//   4. 本檔 inspectShield() 內被註解掉的兩行不是「執行期會失敗」,
//      而是「取消註解就編譯不過」,兩者性質完全不同。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】const 物件與 const 參考
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼函式參數推薦寫成 const T&?
//     答：一次達成三件事:不拷貝(對大型物件省下深拷貝)、
//         不修改(型別系統保證函式不會動到呼叫端物件)、
//         接受面最廣(非 const 物件、const 物件、暫存物件都能綁)。
//         少寫那個 const,函式就再也不能接受 const 物件了。
//     追問：那什麼時候不該用 const T&?→ 型別很小且可平凡複製時
//         (int、double、指標),直接傳值反而更快,因為參考在 ABI 上
//         是指標,讀值要多一次記憶體間接。
//
// 🔥 Q2. 非 const 物件可以傳給 const T& 參數嗎?反過來呢?
//     答：前者可以 —— 這是合法的隱式轉換(加上 const 永遠安全)。
//         後者不行 —— const 物件不能綁到非 const 參考,否則就能透過
//         那個參考修改 const 物件,const 保證會瞬間崩潰。
//         這個單向性是整個 const 正確性得以成立的基礎。
//     追問：那 const_cast 為什麼存在?→ 主要用於呼叫「介面忘了加 const
//         的舊有 C API」。若拿它去修改本來就宣告為 const 的物件,是 UB。
//
// ⚠️ 陷阱. 「const 物件呼叫非 const 成員函式,執行時會出錯嗎?」
//     答：不會執行到那一步 —— 它根本編譯不過。這是編譯期的可呼叫性判定,
//         不是執行期檢查。理解這點很重要:const 的所有保護都在編譯期完成,
//         程式一旦成功編譯,執行期不會為 const 付出任何代價,
//         也不會有任何 const 相關的執行期檢查。
//     為什麼會錯：把 const 想像成執行期的唯讀旗標(像檔案權限那樣),
//         以為違規時會拋例外或回傳錯誤。實際上它純粹是型別系統的一部分,
//         違規的程式碼根本無法產生出可執行檔。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 從缺,理由如下
//   本檔談的是傳參方式與 const 綁定規則,屬 C++ 型別系統議題;
//   LeetCode 判題不檢查參數是否為 const&。本課 summary.cpp 會以
//   705. Design HashSet 說明查詢/修改介面切分與 const 的對應關係。
//
// =============================================================================

#include <iostream>
#include <string>
using namespace std;

class Shield {
private:
    string name_;
    int defense_;
    int durability_;

public:
    Shield(const string& name, int def, int dur)
        : name_(name), defense_(def), durability_(dur)
    {
    }

    // const 成員函數
    // const 成員函數承諾不修改對象的狀態（成員變量）
    // 這允許在 const 對象上調用這些函數，並且編譯器會強制執行這一承諾
    const string& getName() const { return name_; }
    int getDefense() const { return defense_; }
    int getDurability() const { return durability_; }

    void printInfo() const {
        cout << "  " << name_ << " [防禦:" << defense_
             << " 耐久:" << durability_ << "]" << endl;
    }

    // 非 const 成員函數
    // 非 const 成員函數可以修改對象的狀態，這些函數不能在 const 對象上調用
    // 這裡的 takeDamage() 和 repair() 函數會修改 durability_，所以不能是 const
    void takeDamage(int dmg) {
        durability_ -= dmg;
        if (durability_ < 0) durability_ = 0;
        cout << "  " << name_ << " 耐久 -" << dmg
             << " (剩餘:" << durability_ << ")" << endl;
    }

    void repair() {
        durability_ = 100;
        cout << "  " << name_ << " 修復完成" << endl;
    }
};

// 接收 const 引用的函數——模擬「只看不碰」
// 這個函數接受一個 const Shield&，表示它只能「看」這個盾牌，但不能修改它
void inspectShield(const Shield& s) {
    cout << "\n--- 檢查盾牌（const 引用）---" << endl;

    // ✅ 可以調用 const 成員函數
    s.printInfo();
    cout << "  防禦力：" << s.getDefense() << endl;
    cout << "  耐久度：" << s.getDurability() << endl;

    // ❌ 不能調用非 const 成員函數
    // s.takeDamage(10);   // 編譯錯誤！
    // s.repair();          // 編譯錯誤！
}

// -----------------------------------------------------------------------------
// 【日常實務範例】設定物件:啟動後凍結成 const，杜絕執行期被偷改
//   情境：伺服器啟動時載入一次設定，之後在整個生命期內不得再變動。
//         常見做法是「載入階段用非 const 物件填好，之後只以 const&
//         發送給各模組」—— 型別系統直接保證沒有人能改。
//   重點：validate() / describe() 這類函式一律接 const&，
//         因此它們連「不小心寫錯成賦值」的機會都沒有。
// -----------------------------------------------------------------------------
class ServerConfig {
private:
    string bindAddr_;
    int    port_;
    int    workerThreads_;
    bool   tlsEnabled_;

public:
    ServerConfig(const string& addr, int port, int workers, bool tls)
        : bindAddr_(addr), port_(port), workerThreads_(workers), tlsEnabled_(tls) {}

    const string& bindAddr() const { return bindAddr_; }
    int  port()          const { return port_; }
    int  workerThreads() const { return workerThreads_; }
    bool tlsEnabled()    const { return tlsEnabled_; }

    // 衍生查詢：同樣 const
    bool isPrivilegedPort() const { return port_ < 1024; }
};

// 啟動前檢查：只讀不改 → const&
static bool validateConfig(const ServerConfig& c) {
    bool ok = true;
    if (c.port() <= 0 || c.port() > 65535) {
        cout << "    [錯誤] port 超出範圍：" << c.port() << endl;
        ok = false;
    }
    if (c.workerThreads() <= 0) {
        cout << "    [錯誤] workerThreads 必須為正數：" << c.workerThreads() << endl;
        ok = false;
    }
    if (c.isPrivilegedPort()) {
        cout << "    [警告] port " << c.port() << " 小於 1024，需要 root 權限" << endl;
    }
    if (!c.tlsEnabled()) {
        cout << "    [警告] 未啟用 TLS，正式環境不建議" << endl;
    }
    // c.port_ = 8080;   // ❌ 編譯錯誤：不只因為 private，const& 也擋住了寫入
    return ok;
}

static void describeConfig(const ServerConfig& c) {
    cout << "    監聽 " << c.bindAddr() << ":" << c.port()
         << "  workers=" << c.workerThreads()
         << "  TLS=" << (c.tlsEnabled() ? "on" : "off") << endl;
}

int main() {
    cout << "=== const 對象的限制 ===" << endl;

    // 非 const 對象：所有函數都能調用
    // ✅ 非 const 對象可以調用 const 和非 const 成員函數
    cout << "\n--- 非 const 對象 ---" << endl;
    Shield shield("鐵盾", 40, 100);
    shield.printInfo();        // ✅ const 函數
    shield.takeDamage(20);     // ✅ 非 const 函數
    shield.repair();           // ✅ 非 const 函數

    // const 對象：只能調用 const 函數
    // ✅ const 對象只能調用 const 成員函數，不能調用非 const 成員函數
    cout << "\n--- const 對象 ---" << endl;
    const Shield legendaryShield("傳說之盾", 100, 999);
    legendaryShield.printInfo();       // ✅ const 函數
    legendaryShield.getDefense();      // ✅ const 函數
    // legendaryShield.takeDamage(10); // ❌ 編譯錯誤！
    // legendaryShield.repair();       // ❌ 編譯錯誤！

    // const 引用參數
    // ✅ const 引用允許我們在函數內部「只看不碰」對象，這是非常常見的用法  
    // 這裡我們傳入 legendaryShield 的 const 引用，函數內部只能調用 const 成員函數
    // 這裡的 inspectShield() 函數接受一個 const Shield&，表示它只能「看」這個盾牌，但不能修改它
    inspectShield(shield);

    // ─────────────────────────────────────────────────────────
    cout << "\n=== 日常實務：伺服器設定啟動後凍結 ===" << endl;

    // 載入階段：正式環境設定（宣告為 const，之後誰都改不了）
    const ServerConfig prod("0.0.0.0", 8443, 16, true);
    cout << "\n--- 正式環境設定 ---" << endl;
    describeConfig(prod);
    cout << "    設定有效？" << (validateConfig(prod) ? "是" : "否") << endl;

    // 一份有問題的設定：檢查函式只讀，卻能完整報出所有問題
    const ServerConfig bad("127.0.0.1", 80, 0, false);
    cout << "\n--- 有問題的設定 ---" << endl;
    describeConfig(bad);
    cout << "    設定有效？" << (validateConfig(bad) ? "是" : "否") << endl;

    // 非 const 物件同樣能傳給 const& 參數（加上 const 永遠安全）
    ServerConfig staging("10.0.0.5", 3000, 4, false);
    cout << "\n--- 非 const 物件也能傳給 const& 參數 ---" << endl;
    describeConfig(staging);

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 22 課：const 成員函數2.cpp" -o l22_2
// 執行: ./l22_2        (rc=0)

// === 預期輸出 ===
// === const 對象的限制 ===
//
// --- 非 const 對象 ---
//   鐵盾 [防禦:40 耐久:100]
//   鐵盾 耐久 -20 (剩餘:80)
//   鐵盾 修復完成
//
// --- const 對象 ---
//   傳說之盾 [防禦:100 耐久:999]
//
// --- 檢查盾牌（const 引用）---
//   鐵盾 [防禦:40 耐久:100]
//   防禦力：40
//   耐久度：100
//
// === 日常實務：伺服器設定啟動後凍結 ===
//
// --- 正式環境設定 ---
//     監聽 0.0.0.0:8443  workers=16  TLS=on
//     設定有效？是
//
// --- 有問題的設定 ---
//     監聽 127.0.0.1:80  workers=0  TLS=off
//     設定有效？    [錯誤] workerThreads 必須為正數：0
//     [警告] port 80 小於 1024，需要 root 權限
//     [警告] 未啟用 TLS，正式環境不建議
// 否
//
// --- 非 const 物件也能傳給 const& 參數 ---
//     監聽 10.0.0.5:3000  workers=4  TLS=off
