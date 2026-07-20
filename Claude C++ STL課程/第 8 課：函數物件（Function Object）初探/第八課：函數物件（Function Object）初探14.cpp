// =============================================================================
//  第八課 14  —  泛型 Lambda（Generic Lambda）：參數用 auto
// =============================================================================
//
// 【主題資訊 Information】
//   語法    : [capture](auto x, const auto& y) { ... }
//   標準版本: C++14（本機以 g++ -std=c++11 -pedantic-errors 實測會直接報錯：
//             "use of 'auto' in lambda parameter declaration only available
//              with '-std=c++14'"）
//   標頭檔  : 無（語言核心特性）
//   延伸    : C++20 起可寫明確模板參數 []<typename T>(T x){ ... }
//
// 【詳細解釋 Explanation】
//
// 【1. 泛型 lambda 的本質：operator() 是模板】
//   一般 lambda 生成的 class 長這樣：
//       class __lambda { public: void operator()(int x) const; };
//   泛型 lambda 只差在 operator() 變成成員模板：
//       class __lambda {
//       public:
//           template<typename T>
//           void operator()(const T& x) const { std::cout << x << " "; }
//       };
//   所以「一個 lambda 能吃 int、double、string」不是什麼執行期多型，
//   而是編譯器對每個用到的型別各實例化一份——與函式模板完全相同的機制，
//   零執行期成本。
//
// 【2. 為什麼這個特性在 C++14 才出現】
//   C++11 定案時 lambda 已經夠複雜，泛型版本被延到下一版。
//   在 C++11 只能這樣繞：
//       struct Print { template<class T> void operator()(const T& x) const { ... } };
//   也就是「自己手寫 functor」。C++14 的泛型 lambda 等於把這件事變成一行。
//
// 【3. auto 參數的推導規則 = 函式模板的推導規則】
//   (auto x)        → template<class T> ... (T x)        值：會複製
//   (const auto& x) → template<class T> ... (const T& x) 常參考：不複製，唯讀
//   (auto&& x)      → template<class T> ... (T&& x)      轉發參考：可完美轉發
//   本檔用 (const auto& x) 是對的：對 std::string 這種要複製成本的型別
//   不會多一次拷貝，對 int 也只是多一層間接（編譯器會最佳化掉）。
//
// 【4. 泛型 lambda 的限制】
//   * 錯誤訊息會變差：型別錯誤要到「實例化」才會被發現，訊息又長又深。
//   * 過度泛型會接受不該接受的型別；C++20 可用 concept 約束：
//         [](std::integral auto x) { ... }
//   * 無法對 auto 參數做偏特化（那是模板的規則，lambda 也繼承了）。
//
// 【概念補充 Concept Deep Dive】
//   本檔 compare(3, 5) 印出的是 1 而不是 true——因為 operator< 回傳 bool，
//   而 std::cout 預設把 bool 以整數輸出（1／0）。要印 true／false 得先送
//   std::boolalpha。這與泛型 lambda 無關，是 iostream 的預設格式旗標，
//   但初學者常在這裡誤以為「lambda 回傳了整數」。
//   另外注意 compare("apple", "banana")：若直接傳 const char*，比較的會是
//   「指標大小」而不是字串內容（那是實作定義、無意義的比較）。本檔特意寫成
//   compare(std::string("apple"), std::string("banana")) 就是為了避開這個坑。
//
// 【注意事項 Pay Attention】
// 1. 泛型 lambda 是 C++14。用 -std=c++11 編譯本檔會失敗（已實測）。
//    驗證標準版本一定要用 -pedantic-errors，只用 -fsyntax-only 有時會被
//    GCC 當成擴充放行。
// 2. (const auto&) 對唯讀參數是最佳預設；要修改或轉發才用 (auto) 或 (auto&&)。
// 3. 泛型 lambda 對 const char* 做 < 比較，比的是指標而非內容。
//    要比字串內容請確保至少一邊是 std::string 或 std::string_view。
// 4. 每多一種實際使用的型別就多實例化一份程式碼，過度使用會讓 binary 變大
//    （template code bloat），這是編譯期多型的固有代價。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】泛型 Lambda
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 泛型 lambda 是哪個標準加入的？編譯器把 auto 參數變成了什麼？
//     答：C++14。編譯器把 closure 的 operator() 生成為「成員模板」，
//         每個實際用到的型別各實例化一份。所以它是編譯期多型（模板），
//         不是執行期多型（虛擬函式），沒有任何執行期開銷。
//     追問：那 C++11 要怎麼達到同樣效果？→ 手寫一個 struct，
//         把 operator() 寫成 template<class T> 的成員模板。
//
// 🔥 Q2. (auto x)、(const auto& x)、(auto&& x) 該怎麼選？
//     答：規則與函式模板一模一樣。唯讀且可能是大物件 → (const auto&)；
//         需要修改自己那份副本 → (auto x)；要完美轉發給別人 →
//         (auto&& x) 搭配 std::forward。預設選 (const auto&) 最安全。
//     追問：(auto&&) 為什麼叫轉發參考而不是右值參考？→ 因為型別是被推導的，
//         推導時會套用參考摺疊，左值進來會推成 T&，所以它左右值都能接。
//
// ⚠️ 陷阱. auto compare = [](const auto& a, const auto& b){ return a < b; };
//          呼叫 compare("apple", "banana") 為什麼結果不可靠？
//     答：字串字面值的型別是 const char[6] / const char[7]，
//         推導後退化成 const char*。比較兩個指標比的是「位址大小」，
//         結果由編譯器如何配置字面值決定，與字串內容毫無關係。
//     為什麼會錯：在原始碼裡 "apple" < "banana" 讀起來就像在比字串，
//         而且測試時常常「剛好」得到符合直覺的答案（字面值常按出現順序
//         連續配置），於是這個 bug 可以潛伏很久。正解是至少讓一邊是
//         std::string 或 std::string_view，本檔即採此寫法。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】從缺。
//   理由：泛型 lambda 解決的是「同一段邏輯要套用到多種型別」的泛型程式設計
//   問題。LeetCode 題目的輸入型別都是固定的（vector<int>、string），
//   泛型完全派不上用場。強行加一題只會讓「auto 參數」看起來像可有可無的
//   裝飾，故從缺，改以實務範例呈現它真正的價值。

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <map>

// -----------------------------------------------------------------------------
// 【日常實務範例】設定檔傾印：同一段格式化邏輯套用到多種欄位型別
//   場景：服務啟動時要把讀進來的設定（字串、整數、布林、浮點）
//         以統一格式印到啟動 log，方便事後排查「當時到底跑在什麼設定下」。
//   為什麼用泛型 lambda：欄位型別各不相同，但格式化規則完全一樣。
//     沒有泛型 lambda 就得為每種型別各寫一個 overload，或寫一個
//     成員模板 functor。泛型 lambda 讓這件事變成一行，而且因為是模板，
//     每種型別都是直接呼叫、沒有型別抹除的開銷。
// -----------------------------------------------------------------------------
void dumpConfig() {
    // 對齊欄名並印出值——型別由編譯器各自實例化
    auto row = [](const std::string& key, const auto& value) {
        std::cout << "  " << key;
        for (size_t i = key.size(); i < 16; ++i) std::cout << '.';
        std::cout << " " << value << "\n";
    };

    row("service.name", std::string("order-api"));
    row("service.port", 8080);
    row("db.timeout_s", 3.5);
    row("cache.enabled", true);      // 注意：bool 預設印成 1
}

int main() {
    // 泛型 Lambda：參數用 auto
    auto print = [](const auto& x) {
        std::cout << x << " ";
    };
    
    std::cout << "=== 泛型 Lambda ===" << std::endl;
    
    // 用在 int
    std::vector<int> ints = {1, 2, 3, 4, 5};
    std::cout << "ints: ";
    std::for_each(ints.begin(), ints.end(), print);
    std::cout << std::endl;
    
    // 用在 double
    std::vector<double> doubles = {1.1, 2.2, 3.3};
    std::cout << "doubles: ";
    std::for_each(doubles.begin(), doubles.end(), print);
    std::cout << std::endl;
    
    // 用在 string
    std::vector<std::string> strings = {"Hello", "World"};
    std::cout << "strings: ";
    std::for_each(strings.begin(), strings.end(), print);
    std::cout << std::endl;
    
    // 泛型比較
    auto compare = [](const auto& a, const auto& b) {
        return a < b;
    };
    
    std::cout << "\n=== 泛型比較 ===" << std::endl;
    std::cout << "compare(3, 5): " << compare(3, 5) << std::endl;
    std::cout << "compare(3.14, 2.72): " << compare(3.14, 2.72) << std::endl;
    std::cout << "compare(\"apple\", \"banana\"): " 
              << compare(std::string("apple"), std::string("banana")) << std::endl;

    // 上面三行印出的是 1 / 0，因為 std::cout 預設把 bool 當整數輸出。
    // 加上 std::boolalpha 才會印 true / false：
    std::cout << "\n=== 同樣的比較，加上 std::boolalpha ===" << std::endl;
    std::cout << std::boolalpha;
    std::cout << "compare(3, 5): " << compare(3, 5) << std::endl;
    std::cout << "compare(3.14, 2.72): " << compare(3.14, 2.72) << std::endl;
    std::cout << std::noboolalpha;   // 還原，以免影響後續輸出

    std::cout << "\n=== 日常實務: 啟動設定傾印 ===" << std::endl;
    dumpConfig();

    return 0;
}

// 編譯: g++ -std=c++14 -Wall -Wextra 第八課：函數物件（Function Object）初探14.cpp -o generic_lambda   （泛型 lambda 需要 C++14 以上；用 -std=c++11 會編譯失敗）

// === 預期輸出 ===
// === 泛型 Lambda ===
// ints: 1 2 3 4 5 
// doubles: 1.1 2.2 3.3 
// strings: Hello World 
//
// === 泛型比較 ===
// compare(3, 5): 1
// compare(3.14, 2.72): 0
// compare("apple", "banana"): 1
//
// === 同樣的比較，加上 std::boolalpha ===
// compare(3, 5): true
// compare(3.14, 2.72): false
//
// === 日常實務: 啟動設定傾印 ===
//   service.name.... order-api
//   service.port.... 8080
//   db.timeout_s.... 3.5
//   cache.enabled... 1
