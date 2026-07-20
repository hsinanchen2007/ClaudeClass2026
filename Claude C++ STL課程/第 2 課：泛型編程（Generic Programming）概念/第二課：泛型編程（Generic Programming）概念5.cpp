// =============================================================================
//  第二課：泛型編程（Generic Programming）概念5.cpp
//   —  多個型別參數：每個參數獨立推導，以及它的自然延伸 variadic template
// =============================================================================
//
// 【主題資訊 Information】
//   語法：
//     template <typename T1, typename T2>
//     void print_pair(T1 first, T2 second);   // 兩個獨立的型別參數
//
//   標準版本：多型別參數本身 C++98 起即有
//             （本檔另示範 C++17 的 fold expression，故以 -std=c++17 編譯）
//   標頭檔  ：<iostream>、<string>
//
//   本機以 -pedantic-errors 實測的版本界線：
//     * fold expression `(args + ...)`  → -std=c++14 失敗、-std=c++17 通過
//     * generic lambda  `[](auto x){}`  → -std=c++11 失敗、-std=c++14 通過
//
// 【詳細解釋 Explanation】
//
// 【1. 兩個型別參數 = 兩次獨立推導】
// 概念4.cpp 的 find_max(T, T) 因為兩個參數共用一個 T 而產生推導衝突。
// 本檔的 print_pair(T1, T2) 沒有這個問題 —— T1 由第一個實參決定、
// T2 由第二個實參決定，兩者互不干涉：
//
//     print_pair(1, 3.14)                    → T1 = int,         T2 = double
//     print_pair("Name", 42)                 → T1 = const char*, T2 = int
//     print_pair(std::string("Hello"), 'W')  → T1 = std::string, T2 = char
//
// 這三次呼叫會實例化出**三個不同的函式**。設計泛型介面時的判斷準則很簡單：
//   * 兩個參數在語意上必須是同一型別（例如要互相比較、要交換）→ 共用一個 T
//   * 兩個參數本來就可以不同（例如一個 key、一個 value）→ 用兩個參數
// 用錯會產生兩種相反的災難：該共用卻分開 → 型別不一致的 bug 溜過去；
// 該分開卻共用 → 明明合理的呼叫卻編譯不過。
//
// 【2. 對 T1 與 T2 的隱含要求】
// 本檔的函式本體是 `std::cout << first << second`，所以隱含要求是
// 「T1 與 T2 都必須有可用的 operator<<」。這解釋了為什麼傳自訂的 struct
// 進來會編譯失敗 —— 除非你為它定義 operator<<（概念9.cpp 就做了這件事）。
//
// 注意 `const char*` 能運作，是因為 <ostream> 為 const char* 提供了專門的
// operator<< 重載（印出字串內容，而非指標位址）。這是特別為 C 字串設計的；
// 換成其他指標型別（例如 int*）就會印出位址。
//
// 【3. 從兩個參數到任意多個：variadic template】
// print_pair 只能處理剛好兩個值。真實的日誌/格式化工具需要「任意多個」，
// 這就是 C++11 引入 variadic template（可變參數模板）的原因：
//
//     template <typename... Args>          // Args 是一個「型別包」(parameter pack)
//     void print_all(const Args&... args); // args 是對應的「值包」
//
// 展開值包的方式有兩種：
//   * C++11/14：遞迴 —— 每次處理第一個、剩下的丟給自己，另寫一個終止版本。
//   * C++17：fold expression —— 一行搞定：
//         (std::cout << ... << args);
//     本機以 -pedantic-errors 實測：這個語法在 -std=c++14 失敗、-std=c++17 通過。
//
// fold expression 是本課少數「新標準真的讓程式碼變短一半」的例子：
// 遞迴版需要兩個函式（遞迴 + 終止），fold 版只要一個且沒有遞迴深度問題。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 每多一種型別組合，就多一份程式碼
//   print_pair 有兩個型別參數，實例化的份數是「實際用到的組合數」。
//   本檔用了 3 種組合 → 3 份程式碼。若一個模板有 3 個型別參數、每個常用
//   5 種型別，最壞情況是 5×5×5 = 125 份實例化。這就是 code bloat 在多參數
//   模板上放大的方式（概念2.cpp 有本機實測數據：200 種型別讓目的檔從
//   1,744 bytes 變成 71,600 bytes，g++ 15.2 -O0 實測）。
//   緩解方式是把與型別無關的邏輯抽到非模板函式裡，讓模板只當薄薄一層轉接。
//
// (B) 為什麼 print_pair 按值傳參是個小缺點
//   `void print_pair(T1 first, T2 second)` 會複製兩個實參。傳 std::string
//   時就是一次完整的字串複製，而函式其實只是要把它印出來 —— 純讀取卻付了
//   複製的錢。泛型程式碼的慣例寫法是 `const T1&`，本檔的實務範例即採用此形式。
//   （若函式需要「取得所有權」，則應改用「按值 + std::move」或完美轉發，
//     那屬於後續課程的主題。）
//
// (C) parameter pack 的長度是編譯期常數
//   sizeof...(Args) 可以取得包裡有幾個型別，且它是編譯期常數，
//   可以直接用在 static_assert 或 if constexpr（C++17）裡。
//   本機以 -pedantic-errors 實測：if constexpr 在 -std=c++14 失敗、
//   -std=c++17 通過。這使得「參數個數不同就走不同邏輯」可以完全在編譯期決定，
//   不產生任何執行期分支。
//
// 【注意事項 Pay Attention】
// 1. 多個型別參數不代表可以隨便混用。若函式本體對兩者做了運算
//    （例如 first + second），那就額外隱含了「T1 與 T2 之間要有合法運算」
//    這條要求，而它同樣不會寫在簽名裡。
// 2. 傳字串字面量時 T1 會被推導成 const char*，不是 std::string。
//    若你在函式內對它呼叫 .size()，會編譯失敗。需要 std::string 語意時，
//    請在呼叫端明確建構，或把參數型別直接寫成 std::string_view（C++17）。
// 3. fold expression 對空的 parameter pack 有特殊規則：
//    只有 &&、|| 與逗號運算子有預設值（true / false / void()），
//    其他運算子（如 +）用在空包上會編譯失敗，需要寫成有初值的形式
//    `(init + ... + args)`。本檔的實務範例使用逗號展開，天然支援零參數。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】多型別參數與可變參數模板
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. template<typename T> f(T,T) 與 template<typename T1,typename T2> f(T1,T2)
//        該怎麼選？
//     答：看語意上兩個參數是否「必須同型別」。要互相比較、交換、放進同一容器
//         → 共用一個 T，讓編譯器幫你擋住型別不一致。本來就可能不同（key/value、
//         標籤/數值）→ 用兩個獨立參數，否則合理的呼叫會被誤擋。
//     追問：如果想「允許不同型別但要能運算」呢？
//         → 用兩個參數，回傳型別交給 decltype 或 std::common_type<T1,T2>::type
//           決定，C++20 起還可以用 concepts 明確表達這個約束。
//
// 🔥 Q2. 要寫一個能接受任意個數、任意型別參數的 log 函式，該用什麼？
//        C++11 和 C++17 的寫法差在哪？
//     答：用 variadic template（template<typename... Args>）。
//         C++11/14 只能遞迴展開：處理第一個參數，剩下的遞迴呼叫自己，
//         再寫一個終止用的重載。C++17 起可用 fold expression 一行展開
//         `(std::cout << ... << args);`，不需遞迴也不需終止函式。
//     追問：sizeof...(Args) 是什麼時候求值的？
//         → 編譯期。它是常數運算式，可直接用在 static_assert 或 if constexpr。
//
// ⚠️ 陷阱. print_pair("Name", 42) 中 T1 被推導成什麼？std::string 嗎？
//     答：不是。是 const char*。字串字面量的型別是 const char[5]，
//         按值傳參時 decay 成 const char*。它之所以能正確印出文字，
//         是因為 <ostream> 特別為 const char* 提供了印出內容的 operator<<
//         重載 —— 這是為 C 字串量身訂做的例外，不是指標的通則。
//     為什麼會錯：看到輸出是文字就以為型別是字串。實際上若你在函式裡對
//         first 呼叫 .size() 或 .substr()，會直接編譯失敗；
//         而換成 int* 之類的其他指標，印出來的就是位址而非內容了。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>

template <typename T1, typename T2>
void print_pair(T1 first, T2 second) {
    std::cout << "(" << first << ", " << second << ")" << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】結構化日誌：任意欄位的 key=value 格式化
//   情境：服務端日誌要輸出成 key=value 形式方便被 Loki / ELK 之類的系統解析，
//         例如：level=INFO event=user_login uid=10247 latency_ms=38 ok=1
//   痛點：欄位的個數與型別因呼叫點而異 —— 有的是字串、有的是整數、有的是布林。
//   為何用泛型：一個 log_kv 服務所有欄位組合，呼叫端不必先把值轉成字串，
//               也就避免了大量臨時 std::string 的建構。
//   這裡用兩層：log_field 處理「一個欄位」，log_line 用 C++17 fold expression
//   把任意多個欄位展開。
// -----------------------------------------------------------------------------
template <typename V>
void log_field(const char* key, const V& value) {
    std::cout << ' ' << key << '=' << value;
}

// 逗號 fold（C++17）：對空的參數包也安全
template <typename... Fields>
void log_line(const char* level, const Fields&... fields) {
    std::cout << "level=" << level;
    // 每個欄位依序展開；逗號運算子的 fold 對零個參數也合法
    (log_field(fields.first, fields.second), ...);
    std::cout << std::endl;
}

// 一個輕量的欄位包裝（避免依賴 std::pair 的細節，聚焦在泛型本身）
template <typename V>
struct Field {
    const char* first;
    V           second;
};

// 輔助函式：讓呼叫端不必寫出型別（C++17 起也可直接用 CTAD 寫 Field{"k", v}）
//
// 注意這裡參數刻意寫成「按值」的 V value，而不是 const V& —— 這正是
// 【注意事項 2】提到的 decay 差異，作者實作本檔時真的踩到了：
//   * 寫成 const V&：字串字面量不會 decay，V 被推導成 char[12]（陣列型別），
//     於是 Field<char[12]> 裡的成員 `V second;` 是個陣列，
//     無法用 `Field<V>{key, value}` 初始化 →
//     error: array must be initialized with a brace-enclosed initializer
//   * 寫成 V value  ：按值傳參會觸發 decay，V 被推導成 const char*，一切正常。
// 這是「綁到參考不 decay、按值傳參才 decay」最典型的實際後果。
template <typename V>
Field<V> f(const char* key, V value) {
    return Field<V>{key, value};
}

int main() {
    std::cout << "=== 兩個獨立型別參數 ===" << std::endl;
    print_pair(1, 3.14);                    // T1 = int,         T2 = double
    print_pair("Name", 42);                 // T1 = const char*, T2 = int
    print_pair(std::string("Hello"), 'W');  // T1 = std::string, T2 = char

    std::cout << "\n=== 日常實務：結構化日誌（任意欄位）===" << std::endl;
    log_line("INFO", f("event", "user_login"), f("uid", 10247),
             f("latency_ms", 38), f("ok", true));
    log_line("WARN", f("event", "cache_miss"), f("key", "session:8842"),
             f("ratio", 0.87));
    log_line("ERROR", f("event", "db_timeout"), f("retry", 3));
    log_line("DEBUG");   // 零個欄位也合法 —— 逗號 fold 對空包安全

    std::cout << "\n=== 每種型別組合各生成一份程式碼 ===" << std::endl;
    std::cout << "上面 print_pair 用了 3 種組合，就實例化出 3 個不同的函式。"
              << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第二課：泛型編程（Generic Programming）概念5.cpp -o concept5

// === 預期輸出 ===
// === 兩個獨立型別參數 ===
// (1, 3.14)
// (Name, 42)
// (Hello, W)
//
// === 日常實務：結構化日誌（任意欄位）===
// level=INFO event=user_login uid=10247 latency_ms=38 ok=1
// level=WARN event=cache_miss key=session:8842 ratio=0.87
// level=ERROR event=db_timeout retry=3
// level=DEBUG
//
// === 每種型別組合各生成一份程式碼 ===
// 上面 print_pair 用了 3 種組合，就實例化出 3 個不同的函式。
