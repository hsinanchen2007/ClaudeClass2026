// =============================================================================
//  第 18 課：對象的生命週期 8  —  懸空引用（Dangling Reference）
// =============================================================================
//
// 【主題資訊 Information】
//   主題：回傳「區域物件的參考」造成的懸空參考，以及安全的替代做法。
//   狀態：本檔可以編譯、可以執行。
//   ★ 但編譯時 gcc/clang 會發出警告：
//        warning: reference to local variable 'local' returned [-Wreturn-local-addr]
//     這個警告是刻意保留的——它正是本課要教的東西：
//     編譯器有能力在編譯期就抓到這個錯誤，請務必把警告當成錯誤看待。
//     本檔 main() 中「使用」該懸空參考的程式碼已被註解掉，
//     因為那會是未定義行為，其結果無法預測、也不該被寫成預期輸出。
//   標頭檔：<iostream>、<string>、<vector>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼會懸空？】
//       Data& dangerous() {
//           Data local(42);      // local 在堆疊上
//           return local;        // 回傳它的參考
//       }                        // ← local 在這裡解構，記憶體歸還
//   函式返回時，區域物件的生命週期結束、解構函式已執行，
//   它所在的那塊堆疊空間會被後續的函式呼叫覆寫。
//   回傳的參考指向的就是這塊「已死」的記憶體。
//
// 【2. 為什麼它常常「看起來能跑」？—— 最危險的地方】
//   堆疊記憶體不會被立刻清空或標記為不可讀，它只是「可以被重用」。
//   若你在函式返回後立刻讀取，那塊記憶體可能還沒被覆寫，
//   於是你讀到了「看起來正確」的值。
//   ★ 但這仍然是未定義行為，而且它的不穩定性極高：
//     加一行 log、換一個最佳化等級、換一個編譯器版本，
//     甚至只是在中間插入另一個函式呼叫，值就變了。
//   這種「測試通過但隨時會爆」的 bug，比直接崩潰危險得多。
//   ★ 正因如此，本檔絕不把「使用懸空參考的輸出」寫成預期輸出——
//     未定義行為沒有「預期輸出」可言。
//
// 【3. 安全的替代方案】
//   (a) 回傳值（本檔的 safe()）
//       Data safe() { Data local(42); return local; }
//       C++17 起，回傳 prvalue 保證不產生額外複製；
//       回傳具名區域變數則適用 NRVO，多數情況也會省略複製，
//       最差情況也只是搬移（move），成本很低。
//       ★ 「回傳值一定比回傳參考慢」是過時的觀念。
//   (b) 回傳「呼叫者擁有」的東西的參考
//       例如回傳成員變數的參考（物件活著就安全）、
//       或回傳傳入參數的參考。
//   (c) 回傳智慧指標（std::unique_ptr）——當物件必須在堆積上時。
//   (d) 由呼叫端提供緩衝區，函式只負責填寫。
//
// 【4. 編譯器與工具能幫你多少】
//   ● gcc/clang 的 -Wreturn-local-addr（含在 -Wall 中）能抓到
//     「直接回傳區域變數」這種明顯的情況。
//   ● 但它抓不到繞了一圈的版本，例如把位址存進成員再回傳、
//     或透過另一個函式間接回傳。
//   ● AddressSanitizer（-fsanitize=address）能在「執行期」抓到
//     stack-use-after-return（需另外開啟 ASAN_OPTIONS=detect_stack_use_after_return=1）。
//   ● 因此守則是：把 -Wall -Wextra 當成必要條件而非充分條件，
//     並在測試環境跑 sanitizer。
//
// 【概念補充 Concept Deep Dive】
//   ● 參考在底層通常以指標實作，所以懸空參考與野指標的本質完全相同，
//     差別只在懸空參考「看起來像個正常變數」，更容易騙過讀者。
//   ● 生命週期結束的時點是「解構函式開始執行」，不是「記憶體被覆寫」。
//     所以即使記憶體內容還在，物件也已經不存在了——
//     讀取它就是未定義行為，不需要等到值真的被改掉。
//   ● 同類陷阱還有：回傳區域 std::string 的 .c_str()、
//     回傳指向區域陣列的指標、lambda 以參考捕捉區域變數後被存起來稍後呼叫。
//     最後一種在非同步/回呼程式碼中特別常見且難查。
//
// 【注意事項 Pay Attention】
//   1. 本檔會產生 -Wreturn-local-addr 警告，這是刻意保留的教學重點。
//   2. 使用懸空參考是未定義行為——不可以說它「一定崩潰」或「一定印出垃圾」，
//      它可能安靜地給出看似正確的結果，那才是最危險的情況。
//   3. 回傳值不慢：C++17 的保證複製省略與 NRVO 讓它幾乎沒有額外成本。
//   4. 編譯器警告只涵蓋最直接的形式，繞一圈就抓不到；請搭配 sanitizer。
//   5. static 區域物件回傳參考是安全的（生命週期是整個程式），
//      但會帶來全域狀態與執行緒安全的問題，見 4.cpp 與 6.cpp。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】懸空引用
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 回傳區域變數的參考會發生什麼事？為什麼有時「好像能跑」？
//     答：區域物件在函式返回時就已解構，回傳的參考指向已結束生命週期的
//         堆疊記憶體，使用它是未定義行為。之所以常常看起來正常，
//         是因為那塊堆疊還沒被後續呼叫覆寫，於是讀到殘留的舊值。
//         但只要加一行 log、換最佳化等級或編譯器，結果就可能改變。
//     追問：那回傳值不是要多複製一次嗎？
//         → 不會。C++17 對 prvalue 保證複製省略；回傳具名區域變數
//           也適用 NRVO，最差也只是 move。「回傳值比較慢」是過時觀念。
//
// 🔥 Q2. 編譯器能抓到所有懸空參考嗎？
//     答：不能。-Wreturn-local-addr（含在 -Wall）只能抓到
//         「直接 return 區域變數」這種明顯形式。若把位址存進成員、
//         透過另一個函式間接回傳，或用 lambda 以參考捕捉後延後執行，
//         編譯器就無能為力。這類問題要靠 AddressSanitizer 在執行期偵測。
//     追問：回傳 static 區域變數的參考安全嗎？
//         → 安全，因為它的生命週期是整個程式。但代價是引入全域狀態，
//           且該物件在多執行緒下的存取需要自行同步。
//
// ⚠️ 陷阱. 這段程式碼跑出了正確答案，是不是代表它沒問題？
//          Data& r = dangerous();
//          std::cout << r.value;      // 印出 42
//     答：不代表。這是未定義行為，「印出 42」只是這一次的偶然結果。
//         未定義行為的定義是「標準對此不作任何要求」——
//         包含「碰巧給出你要的答案」也完全合法。
//     為什麼會錯：把「測試通過」當成「程式正確」。對未定義行為而言，
//         測試通過完全不構成任何保證：編譯器在最佳化時會假設 UB 不存在，
//         因此可能做出讓程式在別處以離奇方式失敗的變換。
//         真正的判準是「有沒有違反語言規則」，而不是「這次跑出來對不對」。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Data {
public:
    int value;
    Data(int v) : value(v) {
        cout << "  [+] Data(" << value << ")" << endl;
    }
    ~Data() {
        cout << "  [-] Data(" << value << ")" << endl;
    }
};

// 危險！返回局部對象的引用
// ★ 編譯時會出現：warning: reference to local variable 'local' returned
//   這個警告是本課的重點，刻意保留，請不要忽略它。
Data& dangerous() {
    Data local(42);       // local 是局部對象
    return local;         // 返回 local 的引用
}   // local 在這裡死亡！返回的引用指向已死的對象！
// 這段程式碼展示了 C++ 中對象生命週期的陷阱：
// - dangerous() 函數返回了一個局部對象 local 的引用，但 local 在函數結束時就被解構了，因此返回的引用指向一個已經死亡的對象，這會導致未定義行為。

// 安全：返回值（複製）
Data safe() {
    Data local(42);
    return local;         // 返回副本（編譯器可能優化掉複製）
                          // local 在這裡死亡，但返回的是副本，所以不會有問題
}

// -----------------------------------------------------------------------------
// 【安全做法 1】回傳「成員」的參考 —— 只要物件活著就安全
//   這是回傳參考最正當的用法：參考指向的東西，其生命週期由呼叫端持有的
//   物件決定，函式返回並不會讓它消失。
// -----------------------------------------------------------------------------
class Buffer {
    vector<int> data;
public:
    Buffer() : data{10, 20, 30} {}
    // 安全：data 是成員，只要 Buffer 物件還活著，這個參考就有效
    const vector<int>& view() const { return data; }
    // 也可以回傳可修改的參考
    vector<int>& mutableView() { return data; }
};

// -----------------------------------------------------------------------------
// 【安全做法 2】回傳「傳入參數」的參考
//   物件的擁有權在呼叫端，函式只是挑一個回傳，不涉及生命週期轉移。
// -----------------------------------------------------------------------------
const string& pickLonger(const string& a, const string& b) {
    return a.size() >= b.size() ? a : b;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】從快取取值：正確 vs 危險的兩種寫法
//   情境：一個小型的設定快取，外界要讀取某個 key 的值。
//   ● 危險寫法（已註解）：在函式內組出一個區域 string 再回傳它的參考。
//   ● 正確寫法：回傳「儲存在 map 裡」的那份資料的參考，
//     其生命週期由 cache 物件持有，函式返回後依然有效。
//   這個對比就是實務上最常見的懸空參考來源。
// -----------------------------------------------------------------------------
class ConfigCache {
    vector<pair<string, string>> entries;
    string notFound;                    // 成員，用來當「查無此鍵」的安全回傳目標
public:
    ConfigCache() : notFound("") {
        entries.emplace_back("log.level", "INFO");
        entries.emplace_back("db.host",   "127.0.0.1");
        entries.emplace_back("db.port",   "5432");
    }

    // 正確：回傳的參考指向 entries 內的資料（或成員 notFound），
    //       兩者的生命週期都由 ConfigCache 物件持有
    const string& get(const string& key) const {
        for (const auto& kv : entries) {
            if (kv.first == key) return kv.second;
        }
        return notFound;
    }

    // 危險寫法示意（刻意不實作，寫出來就會懸空）：
    //   const string& getWithPrefix(const string& key) const {
    //       string tmp = "cfg." + get(key);   // 區域變數
    //       return tmp;                        // ← 懸空！tmp 立刻解構
    //   }
    // 正確做法是回傳「值」：
    string getWithPrefix(const string& key) const {
        return "cfg." + get(key);              // 回傳值，安全且幾乎無額外成本
    }
};

// 註：本檔不加 LeetCode 範例。
//     懸空參考是記憶體生命週期的錯誤模式，LeetCode 的題目不會考它，
//     且其行為屬未定義、無法寫出可驗證的預期輸出；硬掛一題會失焦，故從缺。

int main() {
    cout << "=== 陷阱：懸空引用 ===" << endl;
    
    // Data& ref = dangerous();   // 未定義行為！
    // cout << ref.value << endl;  // 可能印出垃圾值或崩潰
    // 編譯器通常會警告：returning reference to local variable
    cout << "  dangerous() 的呼叫已刻意註解掉" << endl;
    cout << "  原因：使用懸空引用是未定義行為，沒有「預期輸出」可言" << endl;
    cout << "  請注意編譯時的 -Wreturn-local-addr 警告，那就是本課重點" << endl;
    
    cout << "\n=== 安全：返回值 ===" << endl;
    Data d = safe();
    cout << "  d.value = " << d.value << endl;  // OK

    cout << "\n=== 安全做法 1：回傳成員的參考 ===" << endl;
    {
        Buffer buf;
        const vector<int>& v = buf.view();      // buf 還活著，v 有效
        cout << "  view() = [";
        for (size_t i = 0; i < v.size(); ++i)
            cout << v[i] << (i + 1 < v.size() ? "," : "");
        cout << "]" << endl;

        buf.mutableView().push_back(40);        // 透過參考修改
        cout << "  push_back(40) 之後 size = " << buf.view().size() << endl;
    }   // buf 在這裡解構，此後任何殘留的參考都會懸空

    cout << "\n=== 安全做法 2：回傳傳入參數的參考 ===" << endl;
    {
        string a = "short";
        string b = "much longer string";
        const string& longer = pickLonger(a, b);   // a、b 都還活著
        cout << "  較長的是: " << longer << endl;
    }

    cout << "\n=== 日常實務：設定快取的安全讀取 ===" << endl;
    {
        ConfigCache cache;
        cout << "  log.level = " << cache.get("log.level") << endl;
        cout << "  db.host   = " << cache.get("db.host")   << endl;
        cout << "  不存在的鍵 = [" << cache.get("no.such.key") << "]" << endl;
        cout << "  加前綴（回傳值，安全）= " << cache.getWithPrefix("db.port") << endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 18 課：對象的生命週期（Object Lifetime）8.cpp" -o life8
//   ★ 編譯時會出現一個刻意保留的警告（這正是本課重點）：
//       warning: reference to local variable 'local' returned [-Wreturn-local-addr]

// 【輸出說明】dangerous() 的呼叫已被註解，故執行期不會觸發未定義行為，
//   下列輸出是完全確定且可重現的。

// === 預期輸出 ===
// === 陷阱：懸空引用 ===
//   dangerous() 的呼叫已刻意註解掉
//   原因：使用懸空引用是未定義行為，沒有「預期輸出」可言
//   請注意編譯時的 -Wreturn-local-addr 警告，那就是本課重點
//
// === 安全：返回值 ===
//   [+] Data(42)
//   d.value = 42
//
// === 安全做法 1：回傳成員的參考 ===
//   view() = [10,20,30]
//   push_back(40) 之後 size = 4
//
// === 安全做法 2：回傳傳入參數的參考 ===
//   較長的是: much longer string
//
// === 日常實務：設定快取的安全讀取 ===
//   log.level = INFO
//   db.host   = 127.0.0.1
//   不存在的鍵 = []
//   加前綴（回傳值，安全）= cfg.5432
//   [-] Data(42)
