// =============================================================================
//  第 25 課：類別內的靜態成員函數 3  —  靜態工廠函式（named constructor idiom）
// =============================================================================
//
// 【主題資訊 Information】
//   模式:  private 建構子 + public static 工廠函式
//     class C {
//         C(...);                              // private：外部無法直接建立
//     public:
//         static C createX();                  // 具名的建立方式
//         static std::optional<C> tryParse(s); // 可能失敗的建立方式
//     };
//   名稱: named constructor idiom（具名建構子慣用法）
//   標準版本: C++98 起可用；回傳 optional 需 C++17
//   標頭檔: <string>；本檔實務範例另需 <optional>
//
// 【詳細解釋 Explanation】
//
// 【1. 建構子的兩個先天限制】
//   (a) 建構子不能有名字 —— 它永遠叫類別名。
//       所以「建立小型藥水」和「建立大型藥水」只能靠參數型別區分，
//       一旦參數型別相同就無法多載：
//           Potion(int heal, int price);        // 小型?大型?
//       呼叫端寫 Potion p(30, 50); 完全看不出意圖。
//   (b) 建構子不能「失敗而不產生物件」——
//       一旦進入建構子，物件就一定會生出來，出錯只能丟例外。
//   靜態工廠一次解決兩者：可以取名字（createSmall / createLarge），
//   也可以回傳 optional / nullptr 表示建立失敗。
//
// 【2. 為什麼建構子要設成 private】
//   如果建構子仍是 public，使用者可以繞過工廠直接建立未經驗證的物件，
//   工廠的驗證邏輯（本檔 createCustom 的範圍檢查）就形同虛設。
//   把建構子關起來，等於強制「所有物件都必須經過驗證的入口」——
//   這是類別不變量（class invariant）的守門機制。
//
// 【3. 回傳的物件會被複製嗎】
//   不會。C++17 起，return Potion(...) 這種「回傳純右值」的寫法適用
//   guaranteed copy elision：標準規定直接在呼叫端的儲存空間就地建構，
//   根本不存在需要省略的複製。
//   C++11/14 時代這叫 RVO，是「允許但不強制」的最佳化；
//   C++17 之後它是語言規則，即使拷貝建構子被 delete 也照樣合法。
//
// 【4. 工廠函式為什麼一定要是 static】
//   因為呼叫它的時候還沒有物件 —— 它的任務正是把物件生出來。
//   非靜態成員函式需要 this，等於要求「先有一個物件才能造物件」，
//   邏輯上自相矛盾。
//
// 【概念補充 Concept Deep Dive】
//   * 工廠也可以回傳智慧指標（static std::unique_ptr<C> create()），
//     用於多型場景 —— 回傳基底類別指標、實際建立衍生類別。
//     本檔回傳值（by value）適用於「不需要多型」的簡單值型別。
//   * private 建構子 + 工廠也是實作 Singleton 的基礎：
//     把工廠改成回傳唯一實例的參考即可。
//   * createCustom 目前用「靜默修正」處理非法輸入（heal 超出範圍就改成 50）。
//     這在教學上直觀，但實務上是危險設計：呼叫端不知道自己的值被改掉了。
//     正式做法是回傳 std::optional 表示失敗，或丟例外。
//     下方實務範例示範 optional 的寫法。
//   * 工廠函式無法被繼承後覆寫（靜態函式不能是 virtual）。
//     需要「由子類別決定建立什麼」時，那是 factory method pattern，
//     要用非靜態的 virtual 函式，兩者名字像但機制不同。
//
// 【注意事項 Pay Attention】
//   1. 建構子設 private 之後，該類別就無法被外部直接建立，也不能被繼承後呼叫。
//   2. 靜態工廠不能是 virtual，需要多型的建立流程要用 factory method pattern。
//   3. 靜默修正非法輸入會隱藏錯誤；優先回傳 optional 或丟例外。
//   4. C++17 起 return C(...) 保證不複製（guaranteed copy elision）。
//   5. 若類別同時提供 public 建構子與工廠，驗證就會被繞過，失去意義。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】靜態工廠函式
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 既然有建構子，為什麼還需要靜態工廠函式?
//     答：建構子有兩個做不到的事。
//         第一，它不能有名字，參數型別相同時無法表達不同意圖
//         （createSmall() 與 createLarge() 若寫成建構子就無法多載）。
//         第二，它不能「失敗而不產生物件」，錯誤只能用例外表達；
//         工廠可以回傳 std::optional 或 nullptr。
//         此外工廠還能回傳子類別物件、回傳快取的既有實例。
//     追問：為什麼建構子要設成 private?
//         → 否則使用者可以繞過工廠直接建立未經驗證的物件，
//         工廠裡的驗證就失效了。關掉建構子才能保證所有物件都經過同一個入口。
//
// 🔥 Q2. static Potion createSmall() 回傳物件時會發生幾次拷貝?
//     答：C++17 起是零次。return Potion(...) 回傳的是純右值，
//         標準規定直接在呼叫端的儲存空間就地建構（guaranteed copy elision），
//         這不是最佳化而是語言規則 ——
//         即使該類別的拷貝建構子被 = delete，這段程式碼依然合法。
//     追問：C++11/14 呢?
//         → 那時叫 RVO，是「允許但不強制」的最佳化。
//         語意上仍需要可用的拷貝／移動建構子，即使實際上沒被呼叫。
//
// ⚠️ 陷阱. 「createCustom 已經做了範圍檢查，所以傳什麼進去都安全。」
//     答：安全，但不正確。本檔的作法是「靜默修正」——
//         heal 傳 5000 會被悄悄改成 50，呼叫端完全不知道。
//         程式不會崩潰，卻產生了一個與使用者意圖不符的物件，
//         而且錯誤被推遲到很久以後才以「數值不對」的形式浮現，極難追查。
//     為什麼會錯：把「不會崩潰」等同於「處理好了」。
//         驗證的目的是讓錯誤「盡早且明確地」被發現，
//         靜默修正正好相反 —— 它把錯誤藏起來。
//         正確作法是回傳 std::optional（失敗就是 nullopt）或丟例外，
//         讓呼叫端無法忽略。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 從缺，理由如下
//   工廠函式解決的是「物件怎麼被建立、由誰把關」的設計問題，
//   LeetCode 判題只驗演算法的輸入輸出，不會考物件建立的入口設計。
//   本檔改以「資料庫連線設定」的實務範例呈現同一個模式，
//   並示範用 std::optional 取代靜默修正。
//
// =============================================================================

#include <iostream>
#include <optional>
#include <string>
using namespace std;

class Potion {
private:
    string name_;
    int healAmount_;
    int price_;

    // 私有建構函數：外部不能直接 new
    // 只能透過工廠函數創建
    Potion(const string& name, int heal, int price)
        : name_(name), healAmount_(heal), price_(price)
    {
    }

public:
    // ====== 靜態工廠函數：提供命名的創建方式 ======
    // 每個工廠函數都創建一種特定類型的藥水
    // 這些函數可以有清晰的名稱，讓使用者知道創建的是什麼類型的物件
    static Potion createSmall() {
        return Potion("小型藥水", 30, 50);
    }

    static Potion createMedium() {
        return Potion("中型藥水", 70, 120);
    }

    static Potion createLarge() {
        return Potion("大型藥水", 150, 300);
    }

    static Potion createCustom(const string& name, int heal, int price) {
        // 帶驗證的創建
        int safeHeal = (heal > 0 && heal <= 999) ? heal : 50;
        int safePrice = (price > 0 && price <= 9999) ? price : 100;
        return Potion(name, safeHeal, safePrice);
    }

    void printInfo() const {
        cout << "  " << name_ << " (回復:" << healAmount_
             << " 價格:" << price_ << " 金幣)" << endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】資料庫連線設定的具名工廠
//   情境：同一個 DbConfig 類別需要好幾種建立方式 ——
//         正式環境（讀環境變數）、本機開發（預設值）、單元測試（記憶體資料庫）。
//         這三種的參數型別完全一樣，用建構子多載根本無法區分，
//         但寫成具名工廠就一目了然。
//   改進點：本檔上方的 createCustom 用「靜默修正」處理非法輸入，
//         這裡改用 std::optional —— 連接埠不合法就回傳 nullopt，
//         呼叫端無法忽略錯誤。
// -----------------------------------------------------------------------------
class DbConfig {
private:
    string host_;
    int    port_;
    string dbName_;
    bool   useTls_;

    // private：所有物件都必須經過下面的工廠，杜絕未驗證的設定
    DbConfig(const string& host, int port, const string& db, bool tls)
        : host_(host), port_(port), dbName_(db), useTls_(tls) {}

public:
    // 具名工廠 1：正式環境（強制 TLS）
    static DbConfig production(const string& host, const string& db) {
        return DbConfig(host, 5432, db, true);
    }

    // 具名工廠 2：本機開發（不加密，連 localhost）
    static DbConfig localDevelopment() {
        return DbConfig("127.0.0.1", 5432, "app_dev", false);
    }

    // 具名工廠 3：單元測試（記憶體資料庫，埠號 0 代表不走網路）
    static DbConfig forTesting() {
        return DbConfig(":memory:", 0, "test", false);
    }

    // 可能失敗的工廠：連接埠不合法就回傳 nullopt，而不是偷偷改成預設值
    static optional<DbConfig> withCustomPort(const string& host,
                                             int port,
                                             const string& db) {
        if (port < 1 || port > 65535) return nullopt;   // 明確失敗，不靜默修正
        return DbConfig(host, port, db, port == 443);
    }

    void print(const string& label) const {
        cout << "  [" << label << "] " << host_ << ":" << port_
             << " db=" << dbName_
             << " TLS=" << (useTls_ ? "on" : "off") << endl;
    }
};

int main() {
    cout << "=== 工廠函數 ===" << endl;

    // 不能直接建構：
    // Potion p("藥水", 50, 100);  // ❌ 建構函數是 private

    // 透過靜態工廠函數創建
    cout << "\n--- 預設藥水 ---" << endl;
    Potion small = Potion::createSmall();
    Potion medium = Potion::createMedium();
    Potion large = Potion::createLarge();

    small.printInfo();
    medium.printInfo();
    large.printInfo();

    cout << "\n--- 自定義藥水 ---" << endl;
    Potion special = Potion::createCustom("秘製靈藥", 500, 2000);
    special.printInfo();

    // 靜默修正的示範：傳入超出範圍的數值
    cout << "\n--- 靜默修正的問題（heal 傳 5000，超出上限 999）---" << endl;
    Potion bad = Potion::createCustom("過量藥水", 5000, 100);
    bad.printInfo();
    cout << "  ↑ 回復量被悄悄改成 50，呼叫端完全不知道自己傳錯了。" << endl;
    cout << "    程式沒崩潰，但物件與使用者意圖不符 —— 這才是危險之處。" << endl;

    cout << "\n=== 日常實務：資料庫連線設定的具名工廠 ===" << endl;
    DbConfig::production("db.example.com", "app_prod").print("正式環境");
    DbConfig::localDevelopment().print("本機開發");
    DbConfig::forTesting().print("單元測試");
    cout << "  ↑ 三者參數型別相同，用建構子多載無法區分，具名工廠一看就懂。" << endl;

    cout << "\n--- 可能失敗的工廠：用 optional 取代靜默修正 ---" << endl;
    const int ports[] = {8080, 443, 0, 70000, -1};
    for (int p : ports) {
        auto cfg = DbConfig::withCustomPort("db.internal", p, "app");
        cout << "  連接埠 " << p << " → ";
        if (cfg) {
            cfg->print("建立成功");
        } else {
            cout << "建立失敗（nullopt），呼叫端必須自行處理" << endl;
        }
    }
    cout << "  ↑ 非法輸入不會被偷偷修正成合法值，錯誤在第一時間就浮現。" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 25 課：類別內的靜態成員函數3.cpp -o static_func3

// === 預期輸出 ===
// === 工廠函數 ===
//
// --- 預設藥水 ---
//   小型藥水 (回復:30 價格:50 金幣)
//   中型藥水 (回復:70 價格:120 金幣)
//   大型藥水 (回復:150 價格:300 金幣)
//
// --- 自定義藥水 ---
//   秘製靈藥 (回復:500 價格:2000 金幣)
//
// --- 靜默修正的問題（heal 傳 5000，超出上限 999）---
//   過量藥水 (回復:50 價格:100 金幣)
//   ↑ 回復量被悄悄改成 50，呼叫端完全不知道自己傳錯了。
//     程式沒崩潰，但物件與使用者意圖不符 —— 這才是危險之處。
//
// === 日常實務：資料庫連線設定的具名工廠 ===
//   [正式環境] db.example.com:5432 db=app_prod TLS=on
//   [本機開發] 127.0.0.1:5432 db=app_dev TLS=off
//   [單元測試] :memory::0 db=test TLS=off
//   ↑ 三者參數型別相同，用建構子多載無法區分，具名工廠一看就懂。
//
// --- 可能失敗的工廠：用 optional 取代靜默修正 ---
//   連接埠 8080 →   [建立成功] db.internal:8080 db=app TLS=off
//   連接埠 443 →   [建立成功] db.internal:443 db=app TLS=on
//   連接埠 0 → 建立失敗（nullopt），呼叫端必須自行處理
//   連接埠 70000 → 建立失敗（nullopt），呼叫端必須自行處理
//   連接埠 -1 → 建立失敗（nullopt），呼叫端必須自行處理
//   ↑ 非法輸入不會被偷偷修正成合法值，錯誤在第一時間就浮現。
