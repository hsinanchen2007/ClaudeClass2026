// =============================================================================
//  第 12 課：vector 元素存取 9  —  用 std::optional 表達「可能不存在」
// =============================================================================
//
// 【主題資訊 Information】
//   template<class T> class std::optional;
//   標頭檔：<optional>
//   標準版本：**C++17**（本機以 -std=c++14 -pedantic-errors 實測會被拒絕）。
//   本檔的核心函式：
//       template<typename T>
//       std::optional<T> safe_get(const std::vector<T>& v, size_t index);
//   常用介面：
//       has_value() / operator bool()   有沒有值
//       operator*  / operator->         取值（無值時是【未定義行為】）
//       value()                         取值（無值時丟 std::bad_optional_access）
//       value_or(fallback)              取值，無值時回傳預設
//       reset()                         清空
//   大小：sizeof(optional<int>) 在本機是 8（4 bytes 的 int + 1 byte 旗標 + 對齊）。
//
// 【詳細解釋 Explanation】
//
// 【1. vector 的三種存取方式，各自把「越界」當成什麼】
//     v[i]      —— 越界是【未定義行為】。不檢查、不報錯，最快。
//                  適合「索引已由邏輯保證合法」的熱路徑。
//     v.at(i)   —— 越界丟 std::out_of_range。把越界視為【例外狀況】。
//                  適合「越界代表程式有 bug 或輸入非法」，該中斷流程。
//     safe_get  —— 越界回傳 std::nullopt。把越界視為【正常的一種結果】。
//                  適合「查不到是預期中的事」，例如查表、解析使用者輸入。
//   三者不是誰取代誰，而是對應三種不同的「越界代表什麼意義」。
//   選錯的代價：熱路徑用 at() 白付檢查成本；用 [] 處理外部輸入則是安全漏洞；
//   用例外處理「每三次就有一次查不到」的情境，效能會很難看。
//
// 【2. 為什麼要用 optional 而不是「回傳 -1 / 空字串 / nullptr」當哨兵】
//   哨兵值有三個根本問題：
//     ① 佔用了值域。若 -1 是合法的資料值（溫度、座標偏移、盈虧），
//        你就無法區分「查不到」與「查到 -1」。
//     ② 沒有型別安全。呼叫端很容易忘記檢查，編譯器不會提醒。
//     ③ 每個 API 的哨兵都不一樣（npos、-1、nullptr、空 string），
//        必須逐一查文件。
//   optional 把「有沒有值」提升成型別的一部分：
//   回傳型別寫著 optional<T>，呼叫端就【看得見】它可能沒有值。
//   而且 optional<T> 不佔用 T 的任何值域——T 的每個值都還是合法的。
//
// 【3. optional 沒有動態配置】
//   optional<T> 內部是「一塊夠大的原始儲存空間 + 一個 bool」，
//   值就地建構在那塊空間裡（placement new），完全不碰堆積。
//   所以它和回傳 T 的成本幾乎一樣，只多一個 bool 與可能的對齊填充。
//   本檔的 main 會印出 sizeof 佐證。
//   這也是它和 std::unique_ptr<T>（會配置堆積）的關鍵差別。
//
// 【4. 本檔的 safe_get 回傳「值的複本」，不是參考】
//   注意簽章是 optional<T> 不是 optional<T&>。
//   C++17 的 std::optional 【不支援參考型別】——optional<T&> 無法具現化。
//   （這是委員會刻意留下的空缺：optional<T&> 的賦值語意有爭議，
//     到 C++26 才被納入。）
//   所以本檔的 safe_get 一定會複製一份元素出來。
//   對 int 無所謂，對大型結構就是實在的成本。
//   若要避免複製，實務上的替代方案是：
//     * 回傳 T*（找不到回 nullptr）—— 老派但有效
//     * 回傳 optional<std::reference_wrapper<T>> —— 可行但很囉唆
//   本檔的 main 兩種都示範。
//
// 【概念補充 Concept Deep Dive】
//
// (A) *opt 與 opt.value() 不是同一件事
//     opt.value()  —— 無值時丟 std::bad_optional_access（有檢查）
//     *opt         —— 無值時是【未定義行為】（無檢查，最快）
//     這組對應關係和 v.at(i) 與 v[i] 完全一樣：
//     一個檢查、一個不檢查。既然你已經用 if (opt) 檢查過了，
//     裡面就該用 *opt（不必再付一次檢查成本）。
//     本檔的 main 用的正是 `if (auto val = safe_get(...))` 搭配 `*val`。
//
// (B) `if (auto val = safe_get(v, 1))` 這行做了兩件事
//     這是 C++17 的「帶初始化的 if」與 optional 的 operator bool 合作：
//     先宣告 val（作用域限於這個 if），再用 operator bool 判斷有沒有值。
//     好處是 val 不會洩漏到外層作用域，也不可能在未檢查的情況下被使用。
//     注意 operator bool 是 explicit 的——所以只能用在條件式，
//     不能寫 bool b = opt;（要寫 bool b = opt.has_value();）。
//
// (C) value_or 的一個容易忽略的成本
//       return opt.value_or(expensiveDefault());
//     value_or 的引數是【一般函式參數】，所以 expensiveDefault()
//     一定會被求值，即使 opt 有值也一樣。
//     真的很貴的話要寫成
//       return opt ? *opt : expensiveDefault();
//     這是 optional 少數會讓人意外的效能陷阱。
//
// 【注意事項 Pay Attention】
//   1. std::optional 是 C++17。用 -std=c++14 編譯會失敗。
//   2. 對無值的 optional 做 *opt 或 opt->x 是未定義行為，
//      不保證崩潰。要有檢查請用 .value()。
//   3. C++17 的 optional 不支援參考型別，optional<T&> 無法具現化。
//      要避免複製請回傳 T* 或 optional<reference_wrapper<T>>。
//   4. value_or 的引數一定會被求值，昂貴的預設值請改用三元運算。
//   5. optional 不做動態配置，成本很低；但它仍會複製一份 T，
//      對大型物件要斟酌。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::optional 與安全存取
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. v[i]、v.at(i)、回傳 optional 的 safe_get，三者該怎麼選？
//     答：看「越界對你的程式代表什麼意義」。
//         v[i]：越界是 UB，最快，用在索引已由邏輯保證合法的熱路徑。
//         v.at(i)：越界丟 out_of_range，用在「越界代表 bug 或非法輸入」、
//                  該中斷流程的場合。
//         optional：越界回 nullopt，用在「查不到是預期結果」的場合
//                  （查表、解析使用者輸入）——因為用例外處理常態情形
//                  既昂貴又語意不對。
//     追問：例外的成本真的很高嗎？
//         → 現代實作是「零成本例外」：不拋的時候幾乎沒有開銷，
//           但真的拋出時很貴（要走 unwind table）。
//           所以例外適合「罕見」的錯誤，不適合每幾次就發生一次的情況。
//
// 🔥 Q2. 為什麼要用 optional，而不是回傳 -1 或 nullptr 當「查不到」？
//     答：哨兵值會佔用型別的值域。若 -1 是合法資料（溫度、盈虧、偏移量），
//         就無法區分「查不到」與「查到 -1」。而且哨兵沒有型別安全——
//         呼叫端忘記檢查時編譯器完全不會提醒。
//         optional<T> 把「可能沒有值」寫進回傳型別，呼叫端看得見，
//         而且完全不佔用 T 的值域。
//     追問：optional 會不會有額外的堆積配置？
//         → 不會。它內部是「原始儲存空間 + bool 旗標」，就地建構，
//           不碰堆積。本機 sizeof(optional<int>) 是 8。
//
// ⚠️ 陷阱. 「optional 比較安全，所以 `*opt` 一定比 `v[i]` 安全」——對嗎？
//     答：不對。對一個沒有值的 optional 做 `*opt`，
//         和對 vector 越界做 `v[i]` 一樣是【未定義行為】，兩者都不檢查。
//         optional 提供的安全，來自於它【強迫你在型別上看見】
//         「這裡可能沒有值」，而不是來自 operator* 本身會檢查。
//         想要有檢查的版本是 opt.value()（無值時丟 bad_optional_access），
//         它對應的是 v.at(i)。
//     為什麼會錯：把「型別層次的安全」誤當成「執行期的檢查」。
//         optional 的價值是讓漏檢查變成「看得見的疏忽」而非「隱形的 bug」，
//         但它不會替你檢查。這和 unique_ptr 一樣——
//         它保證不洩漏，但解參考一個空的 unique_ptr 照樣是 UB。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <optional>
#include <string>
#include <functional>   // std::reference_wrapper

template <typename T>
std::optional<T> safe_get(const std::vector<T>& v, size_t index) {
    if (index < v.size()) {
        return v[index];        // 隱式建構成 optional<T>（會複製一份）
    }
    return std::nullopt;
}

// 不複製的替代方案 1：回傳指標，找不到回 nullptr
template <typename T>
const T* safe_ptr(const std::vector<T>& v, size_t index) {
    return index < v.size() ? &v[index] : nullptr;
}

// 不複製的替代方案 2：optional<reference_wrapper<const T>>
// （C++17 的 optional 不支援 optional<T&>，只能繞這一圈）
template <typename T>
std::optional<std::reference_wrapper<const T>> safe_ref(const std::vector<T>& v, size_t index) {
    if (index < v.size()) return std::cref(v[index]);
    return std::nullopt;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔刻意不放
//   理由：LeetCode 的題目型態是「給定保證合法的輸入，算出答案」，
//   索引越界一律屬於「解法寫錯了」，不是需要被表達的正常結果。
//   所有題目的函式簽章也都由平台固定（回傳 int / vector<int> / bool），
//   根本沒有回傳 optional 的餘地。
//   optional 真正的價值在「跟外部世界打交道」——解析設定、查表、
//   讀使用者輸入，也就是下方的實務範例。硬掛一題只會誤導讀者。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】解析 CSV 一行，安全地取出指定欄位
//   情境：讀進來的 CSV 是外部資料，欄位數可能不足（檔案損毀、
//         版本不同、使用者手動編輯過）。取第 5 欄時，
//         「那一行只有 3 欄」是完全可預期的正常情況，不是程式 bug。
//   為什麼用到本主題：這正是 optional 的典型用途——
//     「查不到」是預期結果之一，不該用例外，也不該用哨兵值
//     （空字串是合法的欄位內容，不能拿來當「不存在」）。
//   注意 field() 用 optional，而 fieldOr() 提供預設值，
//   兩者對應實務上「必要欄位」與「選填欄位」的差別。
// -----------------------------------------------------------------------------
class CsvRow {
public:
    explicit CsvRow(const std::string& line) {
        size_t start = 0;
        while (true) {
            size_t comma = line.find(',', start);
            if (comma == std::string::npos) {
                fields_.push_back(line.substr(start));
                break;
            }
            fields_.push_back(line.substr(start, comma - start));
            start = comma + 1;
        }
    }

    size_t size() const { return fields_.size(); }

    // 取欄位：不存在時回 nullopt（不是空字串——空字串是合法內容）
    std::optional<std::string> field(size_t i) const {
        return safe_get(fields_, i);
    }

    // 選填欄位：不存在時給預設值
    std::string fieldOr(size_t i, const std::string& fallback) const {
        // 注意：value_or 的引數一定會被求值。這裡 fallback 已經算好了，
        // 所以沒有成本；若是昂貴的運算就該寫成三元式。
        return field(i).value_or(fallback);
    }

    // 取數值欄位：不存在或格式不對都回 nullopt
    std::optional<int> intField(size_t i) const {
        auto s = field(i);
        if (!s || s->empty()) return std::nullopt;
        try {
            size_t pos = 0;
            int val = std::stoi(*s, &pos);
            if (pos != s->size()) return std::nullopt;   // 有殘留字元 → 不是純數字
            return val;
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }

private:
    std::vector<std::string> fields_;
};

int main() {
    std::vector<int> v = {10, 20, 30};

    std::cout << "=== 原始示範：safe_get 回傳 optional ===\n";

    // 安全存取，不會拋例外也不會未定義行為
    if (auto val = safe_get(v, 1)) {
        std::cout << "v[1] = " << *val << std::endl;  // 20
    }

    if (auto val = safe_get(v, 10)) {
        std::cout << "v[10] = " << *val << std::endl;
    } else {
        std::cout << "索引 10 無效" << std::endl;
    }

    std::cout << "\n=== 三種存取方式的對照 ===\n";
    {
        std::cout << "v[1]        = " << v[1] << "   （不檢查，越界是 UB）\n";
        std::cout << "v.at(1)     = " << v.at(1) << "   （檢查，越界丟例外）\n";
        std::cout << "safe_get(1) = " << *safe_get(v, 1) << "   （檢查，越界回 nullopt）\n";

        std::cout << "\n越界時各自的反應：\n";
        std::cout << "  v[10]        -> 未定義行為（本檔不示範，不可預測）\n";
        try {
            std::cout << "  v.at(10)     -> ";
            std::cout << v.at(10) << "\n";
        } catch (const std::out_of_range& e) {
            std::cout << "丟出 std::out_of_range: " << e.what() << "\n";
        }
        std::cout << "  safe_get(10) -> "
                  << (safe_get(v, 10).has_value() ? "有值" : "nullopt（沒有值）") << "\n";
    }

    std::cout << "\n=== optional 的取值方式：* vs value() vs value_or() ===\n";
    {
        auto some = safe_get(v, 0);
        auto none = safe_get(v, 99);

        std::cout << "has_value(): some=" << std::boolalpha << some.has_value()
                  << ", none=" << none.has_value() << "\n";
        std::cout << "*some        = " << *some << "（已檢查過才解參考）\n";
        std::cout << "some.value() = " << some.value() << "（有檢查版）\n";
        std::cout << "none.value_or(-1) = " << none.value_or(-1) << "（無值時給預設）\n";

        try {
            std::cout << "none.value() -> ";
            std::cout << none.value() << "\n";
        } catch (const std::bad_optional_access& e) {
            std::cout << "丟出 std::bad_optional_access: " << e.what() << "\n";
        }
        std::cout << "*none        -> 未定義行為（和 v[越界] 一樣不檢查，本檔不示範）\n";
    }

    std::cout << "\n=== optional 不做動態配置 ===\n";
    {
        std::cout << "sizeof(int)                = " << sizeof(int) << "\n";
        std::cout << "sizeof(std::optional<int>) = " << sizeof(std::optional<int>)
                  << "（int + bool 旗標 + 對齊；不碰堆積）\n";
        std::cout << "sizeof(std::string)                = " << sizeof(std::string) << "\n";
        std::cout << "sizeof(std::optional<std::string>) = "
                  << sizeof(std::optional<std::string>) << "\n";
        std::cout << "（以上為本機 g++ 15.2 / libstdc++ 的值，屬實作定義）\n";
    }

    std::cout << "\n=== 避免複製：回傳指標 / reference_wrapper ===\n";
    {
        std::vector<std::string> big = {"alpha", "beta", "gamma"};

        // optional<T> 會複製一份字串
        auto copied = safe_get(big, 1);
        std::cout << "safe_get  取得: " << *copied << "（這是一份複本）\n";

        // 回傳指標：不複製
        if (const std::string* p = safe_ptr(big, 1)) {
            std::cout << "safe_ptr  取得: " << *p
                      << "（指向原元素，位址與 &big[1] 相同: "
                      << (p == &big[1]) << "）\n";
        }

        // optional<reference_wrapper<const T>>：不複製，但寫法囉唆
        if (auto r = safe_ref(big, 1)) {
            const std::string& ref = r->get();
            std::cout << "safe_ref  取得: " << ref
                      << "（同樣指向原元素: " << (&ref == &big[1]) << "）\n";
        }
        std::cout << "→ C++17 的 optional 不支援 optional<T&>，所以要繞這一圈。\n";
    }

    std::cout << "\n=== 日常實務：解析 CSV 並安全取欄位 ===\n";
    {
        std::vector<std::string> lines = {
            "1001,Alice,engineering,120000",
            "1002,Bob,sales",                  // 欄位不足：沒有薪資
            "1003,,marketing,95000",           // 第 2 欄是空的（合法內容，不是「不存在」）
            "1004,Dave,hr,not-a-number",       // 薪資欄格式錯誤
        };

        for (const auto& line : lines) {
            CsvRow row(line);
            std::cout << "原始行: " << line << "\n";
            std::cout << "  欄位數 : " << row.size() << "\n";
            std::cout << "  員工編號: " << row.fieldOr(0, "(缺)") << "\n";

            // 關鍵示範：空字串與「欄位不存在」是兩件不同的事
            auto name = row.field(1);
            if (!name) {
                std::cout << "  姓名   : (欄位不存在)\n";
            } else if (name->empty()) {
                std::cout << "  姓名   : (欄位存在但為空字串)\n";
            } else {
                std::cout << "  姓名   : " << *name << "\n";
            }

            auto salary = row.intField(3);
            std::cout << "  薪資   : "
                      << (salary ? std::to_string(*salary) : std::string("(無法取得)")) << "\n";
            std::cout << "\n";
        }
        std::cout << "→ 若用空字串當「不存在」的哨兵，第 3 行的空姓名\n";
        std::cout << "  就會被誤判成「沒有這個欄位」。optional 讓兩者可以區分。\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 12 課：vector 元素存取9.cpp -o demo9
// ⚠️ 必須用 C++17（或更新）—— std::optional 是 C++17 特性。
//    以 g++ -std=c++14 -pedantic-errors 編譯會被明確拒絕。

// === 預期輸出 ===
// === 原始示範：safe_get 回傳 optional ===
// v[1] = 20
// 索引 10 無效
//
// === 三種存取方式的對照 ===
// v[1]        = 20   （不檢查，越界是 UB）
// v.at(1)     = 20   （檢查，越界丟例外）
// safe_get(1) = 20   （檢查，越界回 nullopt）
//
// 越界時各自的反應：
//   v[10]        -> 未定義行為（本檔不示範，不可預測）
//   v.at(10)     -> 丟出 std::out_of_range: vector::_M_range_check: __n (which is 10) >= this->size() (which is 3)
//   safe_get(10) -> nullopt（沒有值）
//
// === optional 的取值方式：* vs value() vs value_or() ===
// has_value(): some=true, none=false
// *some        = 10（已檢查過才解參考）
// some.value() = 10（有檢查版）
// none.value_or(-1) = -1（無值時給預設）
// none.value() -> 丟出 std::bad_optional_access: bad optional access
// *none        -> 未定義行為（和 v[越界] 一樣不檢查，本檔不示範）
//
// === optional 不做動態配置 ===
// sizeof(int)                = 4
// sizeof(std::optional<int>) = 8（int + bool 旗標 + 對齊；不碰堆積）
// sizeof(std::string)                = 32
// sizeof(std::optional<std::string>) = 40
// （以上為本機 g++ 15.2 / libstdc++ 的值，屬實作定義）
//
// === 避免複製：回傳指標 / reference_wrapper ===
// safe_get  取得: beta（這是一份複本）
// safe_ptr  取得: beta（指向原元素，位址與 &big[1] 相同: true）
// safe_ref  取得: beta（同樣指向原元素: true）
// → C++17 的 optional 不支援 optional<T&>，所以要繞這一圈。
//
// === 日常實務：解析 CSV 並安全取欄位 ===
// 原始行: 1001,Alice,engineering,120000
//   欄位數 : 4
//   員工編號: 1001
//   姓名   : Alice
//   薪資   : 120000
//
// 原始行: 1002,Bob,sales
//   欄位數 : 3
//   員工編號: 1002
//   姓名   : Bob
//   薪資   : (無法取得)
//
// 原始行: 1003,,marketing,95000
//   欄位數 : 4
//   員工編號: 1003
//   姓名   : (欄位存在但為空字串)
//   薪資   : 95000
//
// 原始行: 1004,Dave,hr,not-a-number
//   欄位數 : 4
//   員工編號: 1004
//   姓名   : Dave
//   薪資   : (無法取得)
//
// → 若用空字串當「不存在」的哨兵，第 3 行的空姓名
//   就會被誤判成「沒有這個欄位」。optional 讓兩者可以區分。
