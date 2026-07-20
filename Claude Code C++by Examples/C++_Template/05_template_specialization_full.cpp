// ============================================================================
//  05_template_specialization_full.cpp  ──  完全特化 (Full Specialization)
// ============================================================================
//
//  【本篇目標】
//    解釋什麼是「特化 (specialization)」、為什麼需要、跟 overloading 的差
//    別、以及怎麼寫一個 full specialization。
//
// ----------------------------------------------------------------------------
//  【為什麼需要特化】
//    Template 像個「通用模板」，但有時對「某些特定型別」我們想換一套作
//    法，原因可能是：
//      - 該型別的通用版會編譯失敗 (例如 char* 比較 vs string 比較)
//      - 該型別有更高效的演算法 (例如 bool 的 vector<bool> 用 bit-packing)
//      - 該型別需要呼叫不同的 API (例如序列化 std::string vs int)
//
//  【術語】
//    Primary template     ：原始的、通用版本。
//    Full specialization  ：把所有 template 參數都「填死」的特化版本。
//    Partial specialization：只填一部分；只能對 class template，不能對
//                           function template (file 06 詳述)。
//
// ----------------------------------------------------------------------------
//  【語法】
//
//    ① Function template full specialization：
//
//          template<typename T>            // primary
//          T to_value(const std::string& s);
//
//          template<>                      // full specialization 標記：空角括號
//          int to_value<int>(const std::string& s) { return std::stoi(s); }
//
//          template<>
//          double to_value<double>(const std::string& s) { return std::stod(s); }
//
//      注意：function template 通常「建議用 overloading 取代特化」，因為特
//      化和 overload resolution 互動有時很反直覺 (Herb Sutter 在 GotW #49
//      就是在說這個)。本篇示範特化以理解語法，實務若可改 overload 就用
//      overload。
//
//    ② Class template full specialization：
//
//          template<typename T>
//          struct TypeName { static const char* get() { return "unknown"; } };
//
//          template<>                      // full specialization
//          struct TypeName<int> { static const char* get() { return "int"; } };
//
//      Class template 沒有 overloading，所以特化是唯一選擇 (除了 if constexpr)。
//
// ----------------------------------------------------------------------------
//  【特化 vs Overloading】
//
//      template<typename T> void f(T);            // primary
//      template<>           void f<int>(int);     // specialization
//      void f(int);                               // overload
//
//    呼叫 f(42)：
//      → 編譯器先做 overload resolution：non-template overload 比 template
//         primary 優先 → 選 void f(int) (普通函式)。
//      → specialization 不參與 overload resolution，它只在 primary 被選中
//         之後決定要不要走特化版本。
//
//    結論：「想加一個型別專屬版本」首選 overload。
//    參考：https://en.cppreference.com/cpp/language/template_specialization
// ============================================================================

/*
補充筆記：template_specialization_full
  - template_specialization_full 涉及模板實例化；請先判斷哪些型別由呼叫端推導，哪些型別由程式指定。
  - 泛型程式要把型別需求寫清楚，例如可比較、可移動、可呼叫或符合某個 concept。
  - 模板技巧的價值在減少重複且保留型別安全，不是讓錯誤訊息變得更長。
  - template_specialization_full 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】完全特化（Full Specialization）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 全特化與偏特化差在哪？各自哪些東西能用？
//     答：全特化把所有模板參數填死（template<> struct Foo<int>）；偏特化只填一部分
//         或限定參數的「形狀」（Foo<T*>）。類別模板兩者都可以，函式模板「只能全
//         特化、不能偏特化」。匹配優先序：全特化 > 最特化的偏特化 > 主模板。
//     追問：函式模板想要偏特化的效果怎麼辦？（改用重載，或包一層 class 再偏特化）
//
// 🔥 Q2. 想替某個型別客製行為，該用特化還是重載？
//     答：函式一律優先用重載。重載決議時「非模板函式」贏過模板：同時存在
//         void f(int) 與 template<typename T> void f(T)，f(42) 會選非模板版本，
//         連特化都輪不到。類別模板沒有重載機制，特化才是唯一手段。
//
// ⚠️ 陷阱. 函式模板的全特化，到底特化到「哪一個」主模板？
//     答：特化必須先掛在某一個主模板底下，而重載決議是「先在各主模板之間選出勝者，
//         選完才看該主模板有沒有對應的特化」。所以：
//             template<class T> void f(T);      // #1
//             template<class T> void f(T*);     // #2
//             template<> void f<int*>(int*);    // <int*> 使它掛在 #1（T=int*）
//         呼叫 f(p)（p 為 int*）時重載決議選中 #2，那個特化永遠不會被呼叫，而且
//         沒有任何警告。要特化 #2 必須寫成 template<> void f<int>(int*)。
//     為什麼會錯：多數人以為特化跟重載一起參與決議、「長得最像的那個會贏」。
//         實際上特化完全不參與重載決議，它只是掛在某個主模板底下的替換實作。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

// ─── 1. Class template full specialization：TypeName<T> ──────────────────
//   debug 工具：給定 T，回傳一個 human-readable 字串。
//   primary template 預設只回 "unknown"，再針對常見型別特化。
template <typename T>
struct TypeName {
    static const char* get() { return "unknown"; }
};

template<> struct TypeName<int>          { static const char* get() { return "int"; } };
template<> struct TypeName<double>       { static const char* get() { return "double"; } };
template<> struct TypeName<std::string>  { static const char* get() { return "std::string"; } };
template<> struct TypeName<bool>         { static const char* get() { return "bool"; } };

// ─── 2. 工作實用：Serializer<T> 對 bool / string 特化 ──────────────────────
//   情境：把資料寫成 JSON-like 字串。bool 要輸出 true/false，string 要加
//   雙引號，數值直接 to_string。
template <typename T>
struct Serializer {
    static std::string to_str(const T& v) {
        return std::to_string(v);          // primary：交給 std::to_string
    }
};

template<>
struct Serializer<bool> {
    static std::string to_str(const bool& v) {
        return v ? "true" : "false";
    }
};

template<>
struct Serializer<std::string> {
    static std::string to_str(const std::string& v) {
        return "\"" + v + "\"";
    }
};

// ─── 3. Leetcode 387 ── First Unique Character in a String (用 specialization 概念) ──
//   原題：給字串 s，找第一個不重複字元的 index；沒有則回 -1。
//   範例：s = "leetcode" → 0   ;   s = "loveleetcode" → 2
//
//   為什麼這題進來？
//     原題寫死 char。但「找第一個不重複元素」這個邏輯通用：可對 vector<int>
//     做、對 string 做、甚至對自定型別做。我們做一個 primary template，再
//     針對 std::string「特化」一個更高效的版本 (因為 string 只有 26 個英文
//     字母，可以用固定陣列代替 hash map)。
//
//   時間複雜度：
//     - primary 版 (vector<T>)：兩次掃描，O(n)；hash 查表 O(1) avg
//     - string 特化版          ：兩次掃描，O(n)，但用 size 26 的陣列，常數更小
//   空間複雜度：
//     - primary：O(n)（hash map）
//     - 特化  ：O(1)（固定陣列 26 個 int）
//   邊界：
//     - 空輸入 → 回 -1
template <typename T>
int first_unique_index(const std::vector<T>& v) {
    std::unordered_map<T, int> count;
    for (const T& x : v) ++count[x];
    for (int i = 0; i < (int)v.size(); ++i)
        if (count[v[i]] == 1) return i;
    return -1;
}

// 為了示範「特化」的概念，這裡為 std::string 寫一個獨立的 overload (對應的
// 是 char 容器；因為 std::string 不是 std::vector<T>，所以實際上這是一個
// 「另開一個函式名稱重載」的解法，但精神同特化)。
int first_unique_index(const std::string& s) {
    int count[26] = {0};
    for (char c : s) ++count[c - 'a'];
    for (int i = 0; i < (int)s.size(); ++i)
        if (count[s[i] - 'a'] == 1) return i;
    return -1;
}

// ─── 4. Leetcode 136 ── Single Number (整數 vs 字串：不同特化) ───────────
//   難度: easy
//   題目：給陣列 nums，每個元素出現兩次，只有一個只出現一次。找出那個元素。
//   範例：[2,2,1] → 1；[4,1,2,1,2] → 4
//
//   經典 XOR 解法：a ^ a = 0，a ^ 0 = a，最後剩下的就是答案。O(n) 時間、O(1) 空間。
//
//   為什麼放在這裡？
//     XOR trick 只對 integral 有意義。我們把 primary template 寫成「不支援」
//     版本，再對 int / long long 做完全特化，套用 XOR 演算法。
//
//   時間：O(n)；空間：O(1)。
template <typename T>
struct SingleNumberOp {
    static T find(const std::vector<T>&) {
        static_assert(sizeof(T) == 0, "SingleNumberOp: 此型別未提供特化版本");
        return T{};
    }
};

template <>
struct SingleNumberOp<int> {
    static int find(const std::vector<int>& v) {
        int r = 0;
        for (int x : v) r ^= x;
        return r;
    }
};

template <>
struct SingleNumberOp<long long> {
    static long long find(const std::vector<long long>& v) {
        long long r = 0;
        for (long long x : v) r ^= x;
        return r;
    }
};

// ─── 5. 工作實用：通用 Default<T>，依型別給不同預設值 ────────────────────
//   常見場景：表單欄位、設定檔。對 int 預設 0，string 預設 "(unset)"，
//   bool 預設 false。primary 給「value-initialize」，特定型別走特化。
template <typename T>
struct Default {
    static T value() { return T{}; }
};

template <>
struct Default<std::string> {
    static std::string value() { return "(unset)"; }
};

template <>
struct Default<bool> {
    static bool value() { return false; }
};

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    // (1) TypeName
    std::cout << "TypeName<int>          = " << TypeName<int>::get()         << "\n";
    std::cout << "TypeName<double>       = " << TypeName<double>::get()      << "\n";
    std::cout << "TypeName<std::string>  = " << TypeName<std::string>::get() << "\n";
    std::cout << "TypeName<float>        = " << TypeName<float>::get()       << "\n"; // unknown

    // (2) Serializer
    std::cout << "Serializer<int>:  "    << Serializer<int>::to_str(42)             << "\n";
    std::cout << "Serializer<bool>: "    << Serializer<bool>::to_str(true)          << "\n";
    std::cout << "Serializer<string>: "  << Serializer<std::string>::to_str("hi")   << "\n";
    std::cout << "Serializer<double>: "  << Serializer<double>::to_str(3.14)        << "\n";

    // (3) Leetcode 387
    std::cout << "first_unique(\"leetcode\")     = "
              << first_unique_index(std::string("leetcode")) << "\n";
    std::cout << "first_unique(\"loveleetcode\") = "
              << first_unique_index(std::string("loveleetcode")) << "\n";

    std::vector<int> arr{1, 2, 3, 2, 1, 4};
    std::cout << "first_unique({1,2,3,2,1,4}) = " << first_unique_index(arr) << "\n";

    // (4) Leetcode 136 Single Number
    std::cout << "single_number({4,1,2,1,2})   = "
              << SingleNumberOp<int>::find({4, 1, 2, 1, 2}) << "\n";

    // (5) Default<T>
    std::cout << "Default<int>    = " << Default<int>::value() << "\n";
    std::cout << "Default<string> = " << Default<std::string>::value() << "\n";
    std::cout << "Default<bool>   = " << std::boolalpha << Default<bool>::value() << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：full specialization 與 function overload 為什麼結果常常不一樣？
    //    A：overload resolution 是「先在 primary template 與普通函式之
    //       間挑」，挑到 primary template 後才看有沒有對應 specialization。
    //       所以 `void f(int)` 普通函式永遠贏 `template<> void f<int>(int)`
    //       特化。Herb Sutter 的 GotW #49 就是因為這點建議 function 用
    //       overload 而非 specialize；class template 沒這問題，特化是唯一手段。
    //
    //  Q2：可以對 std::hash 做特化嗎？需要注意什麼？
    //    A：可以也是常見手法，目的是讓自定型別能放進 std::unordered_map。
    //       規則是必須在 namespace std 裡寫特化、且只能特化「使用者定義
    //       型別」(不能特化標準型別)、必須提供 `size_t operator()(const T&)
    //       const noexcept` 介面。標準明文允許這個例外。
    //
    //  Q3：完全特化跟「把 template 函式重新定義一次」的非 template 版本，
    //      ABI / 編譯期表現有沒有差？
    //    A：full specialization 的特化函式仍然算 template 實例 (mangled
    //       name 帶 template signature)，可放 header 但要小心 ODR；非
    //       template overload 是普通函式，放 header 必須加 inline 否則
    //       多 .cpp include 會 link error。一般場景非 template overload
    //       行為更直覺、與 overload resolution 互動更好預測。
    //
    return 0;
}

// ============================================================================
//  【容易踩的坑】
//    1. function template 加上特化往往會與 overload resolution 互動出乎意
//       料。實務首選 overload，特化保留給 class template 用。
//    2. 特化必須在 primary template 可見之後才能寫；通常 primary 放 header
//       前段，特化放後段。
//    3. 不能對「成員函式 template」單獨特化但類別本身不特化 (語法不允許)。
//    4. 特化在 namespace 中要寫在原始 namespace 下；對標準庫如 std::hash
//       要 open std namespace 是合法的「特例」(只能特化 user type 給的)。
//
//  【下一篇】
//    06_template_specialization_partial.cpp ── 偏特化。
// ============================================================================

// 編譯: g++ -std=c++20 -Wall -Wextra 05_template_specialization_full.cpp -o 05_template_specialization_full

// === 預期輸出 ===
// TypeName<int>          = int
// TypeName<double>       = double
// TypeName<std::string>  = std::string
// TypeName<float>        = unknown
// Serializer<int>:  42
// Serializer<bool>: true
// Serializer<string>: "hi"
// Serializer<double>: 3.140000
// first_unique("leetcode")     = 0
// first_unique("loveleetcode") = 2
// first_unique({1,2,3,2,1,4}) = 2
// single_number({4,1,2,1,2})   = 4
// Default<int>    = 0
// Default<string> = (unset)
// Default<bool>   = false
