// =============================================================================
//  第 2 課：C 與 C++ 的關鍵差異 8  —  C++ 的 bool：型別系統裡的一等公民
// =============================================================================
//
// 【主題資訊 Information】
//   關鍵字   ：bool / true / false（C++98 起就是語言關鍵字，不需要標頭檔）
//   輸出格式 ：預設印 1/0；std::boolalpha 改印 true/false（<ios>，隨 <iostream> 引入）
//   標準版本 ：bool 本身 C++98；explicit operator bool 是 C++11
//              （在此之前要用繁瑣的 safe bool idiom）
//   sizeof   ：本機 g++ 15.2 / x86-64 上 sizeof(bool) == 1（實作定義）
//   注意     ：std::vector<bool> 是特化版，operator[] 不回傳 bool&
//
// 【詳細解釋 Explanation】
//   （本檔聚焦「bool 作為型別在 C++ 型別系統裡引發的效應」；
//     C 的 _Bool / stdbool.h 歷史請見同一課第 7 檔，兩檔刻意不重複。）
//
// 【1. bool 是關鍵字，代價是「相容性斷點」】
//   C++ 在 1993 年左右把 bool 加進語言，當時就知道會撞到既有程式碼裡的
//   typedef int bool;。C++ 選擇了與 C 相反的路線：直接把 bool 變成關鍵字，
//   讓那些程式碼編譯失敗、強迫修改。
//   C 選擇不破壞既有程式碼（用 _Bool + 標頭檔巨集），C++ 選擇語言的整潔。
//   這個取捨差異，某種程度上概括了兩個語言的性格。
//
// 【2. 有了真正的 bool，多載解析才有意義】
//   這是 bool 成為「型別」而非「巨集」後最大的收穫：
//       void log(bool ok);
//       void log(int  code);
//       void log(const std::string& msg);
//   編譯器能依實際型別挑對函式。在 C 裡這三者全是 int，根本無法區分。
//   但它也帶來一個 C 不會有的新陷阱——見下方【4】。
//
// 【3. 為什麼 std::cout << flag 預設印 1 而不是 true】
//   因為 operator<<(ostream&, bool) 預設會先把 bool 提升成 int 再輸出，
//   這是為了與 C 的 printf("%d") 慣例一致。
//   要印文字必須明確要求：
//       std::cout << std::boolalpha << flag;     // true / false
//       std::cout << std::noboolalpha << flag;   // 切回 1 / 0
//   關鍵是 boolalpha 是「黏著的（sticky）」——它修改的是 stream 的格式狀態，
//   一旦設定，之後所有對該 stream 的 bool 輸出都會受影響，直到被改回來。
//   這跟 std::setw 只影響下一次輸出完全不同，是很常見的誤解。
//   本檔輸出會實際驗證這個黏著性。
//
// 【4. 隱式轉換：bool 是「最貪心」的目標型別】
//   幾乎所有東西都能隱式轉成 bool：整數、浮點數、指標、有 operator bool 的類別。
//   在多載解析裡，「指標 → bool」屬於布林轉換，而「const char* → std::string」
//   需要使用者自訂轉換（user-defined conversion），前者的優先序比後者高。
//   結果就是這個著名的陷阱：
//       void f(bool);
//       void f(const std::string&);
//       f("hello");        // 呼叫的是 f(bool)，不是 f(const std::string&)！
//   而且 "hello" 轉成 bool 是 true（非空指標），所以完全不會有任何警告。
//   本機 g++ 15.2 實測確認走 f(bool)（見本檔輸出）。
//   解法：把 bool 多載標成 explicit 無效（explicit 不能用在函式參數），
//   實務做法是改用 enum class 讓意圖明確：
//       enum class Verbose { No, Yes };
//       void f(Verbose v);              // 呼叫端必須寫 f(Verbose::Yes)
//
// 【5. C++11 的 explicit operator bool】
//   要讓自訂類別能寫 if (obj)，需要提供轉換到 bool 的能力。但若寫成
//   非 explicit 的 operator bool()，就會意外允許一堆荒謬的用法：
//       if (file1 == file2)      // 兩個 file 各自轉成 bool 再比較，語意全錯
//       int n = file1 + 1;       // 竟然能編譯
//   C++11 的 explicit operator bool 精準解決了這件事：
//   它「只」在需要布林的語境（if、while、!、&&、||、三元條件）自動生效，
//   其他情況一律要求顯式轉換。這被稱為 contextual conversion。
//   std::unique_ptr、std::optional、iostream 全部採用這個做法。
//
// 【概念補充 Concept Deep Dive】
//   * std::vector<bool> 是標準明訂的「特化」，內部把每個元素壓成一個 bit
//     以節省空間。代價是它不滿足一般容器的要求：
//       - operator[] 回傳的是一個 proxy 物件，不是 bool&
//         （本機實測：decltype(vb[0]) 不是 bool&，vi[0] 才是 int&）；
//       - 因此 auto x = vb[0]; 拿到的是 proxy 而不是 bool，
//         若原 vector 被銷毀或重新配置，這個 proxy 就懸空了；
//       - 也不能取得 &vb[0] 這種真正的 bool 指標。
//     需要真正的布林容器時，實務上用 std::vector<char> 或 std::deque<bool>。
//     本機實測 sizeof(std::vector<bool>) == 40，sizeof(std::vector<int>) == 24
//     （皆為 libstdc++ 的實作定義值，前者多存了 bit 位移資訊）。
//   * bool 的有效位元樣式只有 0 和 1。透過 reinterpret_cast 或 memcpy 塞進
//     其他值再讀取是未定義行為——編譯器會假設它非 0 即 1 並據此最佳化。
//   * 算術運算中 bool 會被整數提升成 int：true + true == 2（本機實測）。
//     這可以拿來做「計數有幾個條件成立」的技巧，但可讀性差，
//     現代寫法建議用 std::count_if 或明確的三元運算子。
//
// 【注意事項 Pay Attention】
//   1. std::boolalpha 是黏著的格式狀態，會持續影響該 stream 後續所有 bool 輸出。
//   2. 小心 f(bool) 與 f(const std::string&) 並存時，字串字面值會選到 bool 多載。
//   3. std::vector<bool> 不是一般容器：operator[] 回傳 proxy，
//      不要對它用 auto，也不能取真正的 bool*。
//   4. 自訂類別的 operator bool 一律加 explicit（C++11 起）。
//   5. 不要對 bool 做位元運算當旗標集合用，那應該是 enum 或 bitset 的工作。
//   6. bool 不要用在跨平台的二進位序列化，改用固定寬度整數。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】C++ 的 bool 與型別系統
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 std::cout << true 印出 1 而不是 true？怎麼改？
//     答：operator<< 對 bool 的預設格式是把它當整數輸出（沿襲 C 的慣例）。
//         要印文字必須加 std::boolalpha。
//         重點是 boolalpha 修改的是「stream 的格式狀態」而且是黏著的——
//         設定之後該 stream 後續所有 bool 輸出都會是 true/false，
//         要還原得用 std::noboolalpha。這跟只影響下一次輸出的 std::setw 不同。
//     追問：那它會不會影響 std::cerr？→ 不會。格式狀態是每個 stream 各自獨立的，
//         對 cout 設定不會影響 cerr。
//
// 🔥 Q2. std::vector<bool> 有什麼特別的？為什麼大家說它「不是容器」？
//     答：它是標準明訂的特化，把每個元素壓成 1 bit 來省空間。
//         代價是 operator[] 無法回傳 bool&（無法取得單一 bit 的參考），
//         只能回傳一個 proxy 物件。於是它不滿足標準容器的要求：
//         不能取 &v[0]、對它用 auto 會拿到 proxy 而不是 bool、
//         proxy 在容器重新配置後會懸空。
//     追問：那要用什麼替代？→ 需要真正 bool 元素時用 std::vector<char> 或
//         std::deque<bool>；若真的要省空間且大小固定，用 std::bitset。
//
// ⚠️ 陷阱. 這段程式碼會呼叫哪一個多載？
//         void f(bool);
//         void f(const std::string&);
//         f("hello");
//     答：會呼叫 f(bool)，而且 "hello" 轉成 bool 是 true。
//         原因是多載解析的優先序：const char* → bool 屬於「標準轉換」
//         （布林轉換），而 const char* → std::string 需要「使用者自訂轉換」，
//         標準轉換的排序優先於使用者自訂轉換，所以 bool 版本勝出。
//         本機 g++ 15.2 實測確認，而且完全沒有警告。
//     為什麼會錯：直覺上會覺得「字串當然配字串版本」，
//         因為語意上明顯是那個意思。但多載解析看的是轉換的「代價階級」，
//         不是語意合理性——它根本不知道你想表達什麼。
//         這也是為什麼現代 C++ 建議用 enum class 取代布林參數：
//         讓呼叫端寫出 f(Verbose::Yes)，意圖與型別都不再有歧義。
//
// 註：bool 的型別系統行為（多載解析、boolalpha、vector<bool> 特化）屬於語言
//     機制，LeetCode 不會考這些，硬套題號會誤導，故本檔僅附實務範例。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

// -----------------------------------------------------------------------------
// 【示範用】多載解析：字串字面值會選到 bool 版本
// -----------------------------------------------------------------------------
namespace overload_demo {
void which(bool)                     { std::cout << "選到 f(bool)\n"; }
void which(const std::string&)       { std::cout << "選到 f(const std::string&)\n"; }
}  // namespace overload_demo

// -----------------------------------------------------------------------------
// 【日常實務範例 1】用 explicit operator bool 做「資源是否有效」的判斷
//
// 情境：包裝一個設定檔載入結果。呼叫端希望能直接寫 if (cfg) 判斷是否成功，
//   但絕不希望 cfg 能被拿去做算術或互相比較。
// 為何用到本主題：這正是 C++11 explicit operator bool 的標準用途，
//   也是 std::optional / unique_ptr / iostream 採用的同一個模式。
// -----------------------------------------------------------------------------
class ConfigResult {
public:
    ConfigResult() = default;
    ConfigResult(std::string key, std::string value)
        : key_(std::move(key)), value_(std::move(value)), valid_(true) {}

    // explicit：只在需要布林的語境自動生效（if / while / ! / && / || / 三元）
    explicit operator bool() const noexcept { return valid_; }

    const std::string& key()   const { return key_; }
    const std::string& value() const { return value_; }

private:
    std::string key_;
    std::string value_;
    bool        valid_ = false;
};

ConfigResult lookupConfig(const std::string& key) {
    if (key == "host")    return ConfigResult("host", "192.168.1.10");
    if (key == "port")    return ConfigResult("port", "8080");
    return ConfigResult();                       // 找不到 → 無效結果
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】用 enum class 取代布林參數，消除呼叫端的歧義
//
// 情境：一個送出 HTTP 請求的函式，需要「是否重試」與「是否記錄」兩個開關。
// 為何用到本主題：布林參數在呼叫端會退化成 send(url, true, false) 這種
//   完全看不懂的程式碼，而且參數順序寫反編譯器不會攔（兩個都是 bool）。
//   enum class 讓型別系統重新獲得辨別能力——這是「有了 bool 之後，
//   反而要學會少用 bool」的實務結論。
// -----------------------------------------------------------------------------
enum class Retry   { No, Yes };
enum class LogCall { No, Yes };

std::string sendRequest(const std::string& url, Retry retry, LogCall log) {
    std::string desc = "送出 " + url;
    desc += (retry == Retry::Yes)   ? "，失敗會重試" : "，失敗不重試";
    desc += (log   == LogCall::Yes) ? "，寫入 log"   : "，不寫 log";
    return desc;
}

int main() {
    std::cout << "=== 原始示範 ===\n";
    bool flag = true;  // bool 是內建類型，不需要標頭檔

    if (flag) {
        std::cout << "flag is true" << std::endl;
    }

    // 可以直接輸出布林值
    std::cout << "flag = " << flag << std::endl;              // 輸出 1
    std::cout << "flag = " << std::boolalpha << flag << std::endl;  // 輸出 true

    std::cout << "\n=== boolalpha 是「黏著」的格式狀態 ===\n";
    // 上面已經設過 boolalpha 了，所以這裡不必再設，仍然是 true/false
    std::cout << "承接上面的設定，直接印 false = " << false << "\n";
    std::cout << "切回 noboolalpha 之後   = " << std::noboolalpha << false << "\n";
    std::cout << "再設回 boolalpha        = " << std::boolalpha << false << "\n";
    std::cout << "→ 它改的是 stream 狀態，不是只影響下一次輸出\n";
    std::cout << std::noboolalpha;   // 還原，避免影響後面的段落

    std::cout << "\n=== 多載解析：字串字面值會選到 bool ===\n";
    std::cout << "  which(\"hello\")            → ";
    overload_demo::which("hello");
    std::cout << "  which(std::string(\"hello\")) → ";
    overload_demo::which(std::string("hello"));
    std::cout << "  → 前者完全沒有警告，是最容易被忽略的多載陷阱\n";

    std::cout << "\n=== std::vector<bool> 是特化，不是一般容器 ===\n";
    {
        std::vector<bool> vb(4, true);
        std::vector<int>  vi(4, 1);
        std::cout << "  decltype(vi[0]) 是 int&  ? " << std::boolalpha
                  << std::is_same_v<decltype(vi[0]), int&> << "\n";
        std::cout << "  decltype(vb[0]) 是 bool& ? "
                  << std::is_same_v<decltype(vb[0]), bool&> << "\n";
        std::cout << "  → vector<bool> 回傳的是 proxy 物件，不是真正的參考\n";
        std::cout << "  sizeof(std::vector<int>)  = " << sizeof(vi)
                  << "（本機 libstdc++ 實作定義值）\n";
        std::cout << "  sizeof(std::vector<bool>) = " << sizeof(vb)
                  << "（多存了 bit 位移資訊）\n";
        std::cout << std::noboolalpha;
    }

    std::cout << "\n=== bool 在算術中會提升成 int ===\n";
    {
        const bool a = true, b = true;
        std::cout << "  true + true = " << (a + b) << "（提升成 int 後相加）\n";
        const int score = (5 > 3) + (2 > 1) + (1 > 9);
        std::cout << "  用來數有幾個條件成立 = " << score
                  << "（可行但可讀性差，不建議）\n";
    }

    std::cout << "\n=== 日常實務 1：explicit operator bool ===\n";
    {
        const char* keys[] = {"host", "port", "timeout"};
        for (const char* k : keys) {
            const ConfigResult r = lookupConfig(k);
            if (r) {                       // explicit operator bool 在此自動生效
                std::cout << "  找到 " << r.key() << " = " << r.value() << "\n";
            } else {
                std::cout << "  找不到設定：" << k << "\n";
            }
        }
        std::cout << "  → 因為是 explicit，寫 int n = cfg; 或 cfg1 == cfg2\n";
        std::cout << "    都會編譯失敗，正是我們要的保護\n";
    }

    std::cout << "\n=== 日常實務 2：enum class 取代布林參數 ===\n";
    {
        // 對照組（不良寫法）：sendRequest("/api", true, false)
        //   呼叫端完全看不出這兩個 true/false 各是什麼意思，寫反也不會被攔
        std::cout << "  " << sendRequest("/api/users", Retry::Yes, LogCall::No) << "\n";
        std::cout << "  " << sendRequest("/api/login", Retry::No, LogCall::Yes) << "\n";
        std::cout << "  → 呼叫端自己就是文件，而且參數寫反會編譯失敗\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2 課：C 與 C++ 的關鍵差異8.cpp" -o demo8
//
// 說明：
//   1. sizeof(std::vector<bool>) == 40 與 sizeof(std::vector<int>) == 24
//      都是本機 g++ 15.2 / libstdc++ 的「實作定義」值，換標準函式庫會不同。
//      標準只規定行為，不規定這些大小。
//   2. sizeof(bool) == 1 同樣是實作定義。
//   3. 「which("hello") 選到 f(bool)」是標準規定的多載解析結果，
//      不是實作差異——任何符合標準的編譯器都會這樣選。

// === 預期輸出 ===
// === 原始示範 ===
// flag is true
// flag = 1
// flag = true
//
// === boolalpha 是「黏著」的格式狀態 ===
// 承接上面的設定，直接印 false = false
// 切回 noboolalpha 之後   = 0
// 再設回 boolalpha        = false
// → 它改的是 stream 狀態，不是只影響下一次輸出
//
// === 多載解析：字串字面值會選到 bool ===
//   which("hello")            → 選到 f(bool)
//   which(std::string("hello")) → 選到 f(const std::string&)
//   → 前者完全沒有警告，是最容易被忽略的多載陷阱
//
// === std::vector<bool> 是特化，不是一般容器 ===
//   decltype(vi[0]) 是 int&  ? true
//   decltype(vb[0]) 是 bool& ? false
//   → vector<bool> 回傳的是 proxy 物件，不是真正的參考
//   sizeof(std::vector<int>)  = 24（本機 libstdc++ 實作定義值）
//   sizeof(std::vector<bool>) = 40（多存了 bit 位移資訊）
//
// === bool 在算術中會提升成 int ===
//   true + true = 2（提升成 int 後相加）
//   用來數有幾個條件成立 = 2（可行但可讀性差，不建議）
//
// === 日常實務 1：explicit operator bool ===
//   找到 host = 192.168.1.10
//   找到 port = 8080
//   找不到設定：timeout
//   → 因為是 explicit，寫 int n = cfg; 或 cfg1 == cfg2
//     都會編譯失敗，正是我們要的保護
//
// === 日常實務 2：enum class 取代布林參數 ===
//   送出 /api/users，失敗會重試，不寫 log
//   送出 /api/login，失敗不重試，寫入 log
//   → 呼叫端自己就是文件，而且參數寫反會編譯失敗
