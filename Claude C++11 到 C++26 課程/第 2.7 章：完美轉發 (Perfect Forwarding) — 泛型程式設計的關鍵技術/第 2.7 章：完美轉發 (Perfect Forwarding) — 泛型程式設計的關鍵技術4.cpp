// =============================================================================
//  第 2.7 章 範例 4  —  enable_if：阻止轉發建構子搶走複製建構子
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<type_traits>（enable_if / is_same / decay）、<utility>（forward）
//   本檔使用 C++11 的完整寫法：
//       typename std::enable_if<!std::is_same<typename std::decay<T>::type,
//                                             Widget>::value>::type
//   C++14 起可簡寫成 std::enable_if_t<...>、std::decay_t<T>；
//   C++17 起再加上 std::is_same_v<...>。三種寫法語意完全相同。
//   C++20 起建議改用 requires 子句或 concepts，錯誤訊息可讀性高得多。
//
// 【詳細解釋 Explanation】
//
// 【1. 沒有 enable_if 時到底發生什麼（本機實測過的錯誤訊息）】
//   若把 enable_if 拿掉，寫 Widget w2(w1);（w1 是非 const 的 Widget），
//   編譯器會報：
//       error: no matching function for call to
//              'std::__cxx11::basic_string<char>::basic_string(Widget&)'
//   訊息裡完全沒有「複製建構子」四個字，而是在抱怨 std::string 不能用 Widget
//   初始化。原因是多載決議根本沒選複製建構子，而是選了模板建構子，
//   把 Widget 拿去初始化 name_ 這個 std::string 成員。
//   錯誤訊息指向的是「症狀發生的地方」，不是「決策錯誤的地方」——
//   這是本陷阱最難除錯的部分。
//
// 【2. 為什麼模板會贏過複製建構子】
//   對 Widget w2(w1)，w1 的型別是 Widget（非 const 左值）：
//     * 複製建構子 Widget(const Widget&)：需要加上 const，屬於
//       qualification conversion，不是完全精確匹配。
//     * 模板建構子 Widget(T&&)：T 可推導成 Widget&，參數型別變成 Widget&，
//       與引數完全精確匹配，一次轉換都不用。
//   多載決議的規則是「較佳的轉換序列獲勝」，精確匹配勝過需要加 const，
//   所以模板贏。這不是編譯器的 bug，是規則忠實執行的結果。
//
// 【3. 為什麼 const Widget w4; Widget w5(w4); 反而正常】
//   此時引數是 const Widget 左值：
//     * 複製建構子的 const Widget& 是完全精確匹配
//     * 模板推導 T = const Widget&，也是精確匹配
//   兩者打平時，標準規定「非模板函式優先於模板函式」，所以複製建構子獲勝。
//   本機已實測：只留 const 版本可以編譯成功，加入非 const 版本才失敗。
//   「加 const 就好了、不加就壞」是這個 bug 的典型指紋。
//
// 【4. enable_if 為什麼要配 std::decay】
//   T 可能被推導成 Widget、Widget&、const Widget&。
//   直接寫 is_same<T, Widget> 只擋得住第一種。
//   decay 會一次去掉參考與 cv 限定（也會把陣列與函式退化成指標），
//   讓所有「本質上就是 Widget 自己」的情況都被排除在模板之外。
//
// 【概念補充 Concept Deep Dive】
//   (A) SFINAE：enable_if 的運作原理是「替換失敗不算錯誤」。
//       當條件為 false 時，std::enable_if<false>::type 不存在，
//       這個模板特化在推導階段就被安靜地從候選集合中移除，
//       不會產生編譯錯誤，於是複製建構子順利勝出。
//   (B) 為什麼本檔把 enable_if 寫在「預設模板參數」而不是回傳型別？
//       因為建構子沒有回傳型別可以寫。這個位置是建構子唯一可行的選擇。
//   (C) 繼承會讓問題更嚴重：若 Widget 是基底類別，衍生類別物件傳進來時，
//       decay_t<T> 會是 Derived 而非 Widget，仍然通不過排除條件，
//       於是又被模板攔截。實務上常改用 is_base_of 做更寬的排除。
//
// 【注意事項 Pay Attention】
//   1. 只寫 is_same<T, Widget> 不夠，必須先 decay。
//   2. 有繼承關係時，考慮用 std::is_base_of 而非 is_same。
//   3. 「用兩個 typename= 預設參數做兩個不同的 enable_if」會造成
//      重複定義（預設參數不參與簽名區分），必須寫在同一個條件裡。
//   4. C++20 的 concepts 是這個問題的正解，enable_if 屬於過渡期技法。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】enable_if 與轉發建構子
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼需要用 enable_if 排除自身型別？不加會怎樣？
//     答：不加的話，Widget w2(w1)（w1 非 const）會選中模板建構子而非複製建構子，
//         因為模板能推導出 Widget&，是完全精確匹配，勝過需要加 const 的複製建構子。
//         結果是編譯器試圖用 Widget 初始化 std::string 成員而報錯，
//         錯誤訊息還指向 std::string，完全不提複製建構子。
//     追問：為什麼 const 物件反而沒問題？
//         → 此時兩者都是精確匹配，而標準規定非模板優先於模板，複製建構子勝出。
//
// 🔥 Q2. enable_if 為什麼要搭配 std::decay？
//     答：T 可能被推導成 Widget、Widget&、const Widget& 等多種形式。
//         decay 一次去掉參考與 cv 限定，讓所有「本質上是自身型別」的情況
//         都被排除，只寫 is_same<T, Widget> 只擋得住最單純的那一種。
//
// ⚠️ 陷阱. enable_if 用「替換失敗」擋掉模板，為什麼這不算編譯錯誤？
//     答：這是 SFINAE（Substitution Failure Is Not An Error）。
//         當條件為 false，std::enable_if<false>::type 這個型別不存在，
//         但標準規定：模板參數替換階段的失敗只會把該候選從多載集合移除，
//         不會直接報錯。於是複製建構子安靜地勝出。
//     為什麼會錯：直覺上「型別不存在」聽起來就該是編譯錯誤，
//         但 SFINAE 刻意把「推導階段的失敗」與「函式本體的錯誤」分開處理——
//         只有前者才享有這個豁免。若錯誤發生在函式本體內（不在簽名上），
//         就會變成真正的 hard error，SFINAE 救不了。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <utility>
#include <type_traits>

class Widget {
    std::string name_;

public:
    // 用 enable_if 排除 Widget 型別本身
    template<typename T,
             typename = typename std::enable_if<
                 !std::is_same<typename std::decay<T>::type, Widget>::value
             >::type>
    Widget(T&& name) : name_(std::forward<T>(name)) {
        std::cout << "  模板建構子\n";
    }

    Widget(const Widget& other) : name_(other.name_) {
        std::cout << "  複製建構子\n";
    }

    Widget(Widget&& other) noexcept : name_(std::move(other.name_)) {
        std::cout << "  移動建構子\n";
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】設定值包裝型別 ConfigValue
//   情境：設定系統裡常見一個「能從字串、數字、字面值建構」的萬用值型別。
//     它天生就需要一個轉發建構子（不然要為每種來源型別各寫一個多載），
//     因此也天生會踩到「模板搶走複製建構子」這個陷阱。
//     而設定值一定會被複製（傳進函式、存進 map、回傳給呼叫端），
//     所以複製建構子必須正常運作——這是 enable_if 在實務上最常出現的位置。
// -----------------------------------------------------------------------------
class ConfigValue {
    std::string raw_;
public:
    // 只接受「不是 ConfigValue 自己」的型別，把複製/移動留給下面兩個
    template<typename T,
             typename = typename std::enable_if<
                 !std::is_same<typename std::decay<T>::type, ConfigValue>::value
             >::type>
    ConfigValue(T&& v) : raw_(std::forward<T>(v)) {
        std::cout << "    ConfigValue 由來源值建構: " << raw_ << "\n";
    }

    ConfigValue(const ConfigValue& o) : raw_(o.raw_) {
        std::cout << "    ConfigValue 複製: " << raw_ << "\n";
    }
    ConfigValue(ConfigValue&& o) noexcept : raw_(std::move(o.raw_)) {
        std::cout << "    ConfigValue 移動: " << raw_ << "\n";
    }

    const std::string& str() const { return raw_; }
};

// 模擬「把設定值存進設定表」——這裡一定會用到複製建構子
void registerSetting(const std::string& key, ConfigValue v) {
    std::cout << "    註冊 " << key << " = " << v.str() << "\n";
}

int main() {
    Widget w1("Hello");         // 模板建構子 ✅
    Widget w2(w1);              // 複製建構子 ✅（不再被模板搶走）
    Widget w3(std::move(w1));   // 移動建構子 ✅

    const Widget w4("World");
    Widget w5(w4);              // 複製建構子 ✅

    // 避免 -Wunused-variable：這些物件的存在本身就是示範重點
    (void)w2; (void)w3; (void)w5;

    std::cout << "\n=== 日常實務：ConfigValue 設定值包裝型別 ===\n";
    std::cout << "  由字面值建構（走模板建構子）:\n";
    ConfigValue timeout("3000");

    std::cout << "  複製一份存進設定表（走複製建構子，未被模板搶走）:\n";
    registerSetting("timeout_ms", timeout);

    std::cout << "  移動一份（走移動建構子）:\n";
    ConfigValue moved(std::move(timeout));
    std::cout << "  moved = " << moved.str() << "\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.7 章：完美轉發 (Perfect Forwarding) — 泛型程式設計的關鍵技術4.cpp" -o enable_if_demo

// 註：本檔未附 LeetCode 範例。enable_if / SFINAE 屬於函式庫與 API 設計層次的技術，
//     LeetCode 題目給定的類別介面不會出現這種泛型建構子；硬套會失真。

// === 預期輸出 ===
//   模板建構子
//   複製建構子
//   移動建構子
//   模板建構子
//   複製建構子
//
// === 日常實務：ConfigValue 設定值包裝型別 ===
//   由字面值建構（走模板建構子）:
//     ConfigValue 由來源值建構: 3000
//   複製一份存進設定表（走複製建構子，未被模板搶走）:
//     ConfigValue 複製: 3000
//     註冊 timeout_ms = 3000
//   移動一份（走移動建構子）:
//     ConfigValue 移動: 3000
//   moved = 3000
