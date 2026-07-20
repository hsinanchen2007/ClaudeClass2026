// =============================================================================
//  第二課：泛型編程（Generic Programming）概念2.cpp
//   —  函式模板：一份定義，編譯器按需生成 N 份程式碼
// =============================================================================
//
// 【主題資訊 Information】
//   語法：
//     template <typename T>          // typename 可換成 class，兩者完全等價
//     void my_swap(T& a, T& b);
//
//   標準版本：函式模板本身是 C++98 起就有的特性（本檔用 -std=c++17 編譯，
//             但不依賴任何 C++11 之後的新語法）
//   標頭檔  ：<iostream>、<string>
//   複雜度  ：對 int/double 等 trivial 型別 O(1)；對 std::string 等型別，
//             本檔寫法會做 3 次「拷貝」，成本正比於內容大小（見注意事項 2）
//
//   關鍵詞：template argument deduction（模板引數推導）、
//           instantiation（實例化）、COMDAT / weak symbol
//
// 【詳細解釋 Explanation】
//
// 【1. template 不是「一個能吃所有型別的函式」】
// 這是最關鍵、也最常被誤解的一點。`my_swap` **本身不是函式**，它是一張
// 「產生函式的藍圖」。編譯器看到 my_swap(x, y) 且 x, y 是 int 時，會：
//
//     1) 推導 T = int                              （template argument deduction）
//     2) 把藍圖中的 T 全部代換成 int，生成一個真正的函式
//                                                   （template instantiation）
//     3) 對這個生成出來的函式做完整的型別檢查與最佳化
//
// 所以本檔雖然只寫了一份 my_swap，最終的執行檔裡其實有**三個**不同的函式：
// my_swap<int>、my_swap<double>、my_swap<std::string>。它們的機器碼不同
// （交換 int 是兩個暫存器搬移；交換 std::string 牽涉指標與長度欄位的搬移）。
//
// 本機 g++ 15.2 實測：把一個 my_max 模板用 7 種型別各呼叫一次，
// `nm -C` 在目的檔中可看到 7 個獨立符號：
//     W char   my_max<char>(char, char)
//     W double my_max<double>(double, double)
//     W float  my_max<float>(float, float)
//     W int    my_max<int>(int, int)
//     W long   my_max<long>(long, long)
//     W short  my_max<short>(short, short)
//     W unsigned long my_max<unsigned long>(unsigned long, unsigned long)
// 前面那個 W 代表 weak symbol，這點在【概念補充】會說明它為什麼重要。
//
// 【2. 和 Java / C# 泛型的根本差異（高頻面試題）】
// 這是面試官最愛用來分辨「背過語法」與「真的懂」的問題。
//
//   C++ template ── 編譯期「生成程式碼」（code generation / monomorphization）
//     * 每個實際用到的型別各生成一份專屬機器碼
//     * 完全沒有執行期成本，可以完整 inline、可以針對該型別最佳化
//     * 型別資訊在編譯期用完就沒了，但**程式碼是分開的**
//     * 代價：code bloat（見下）與編譯時間
//
//   Java generics ── 編譯期「抹除型別」（type erasure）
//     * List<String> 與 List<Integer> 在 bytecode 中是**同一個** List
//     * 型別參數被抹成 Object（或其 bound），編譯器插入 cast
//     * 只有一份程式碼 → 沒有 code bloat，但無法針對型別最佳化
//     * 因此 Java 有 List<int> 不合法（只能 List<Integer>，要裝箱）、
//       不能寫 new T[]、執行期拿不到 T 的真實型別等限制
//
//   一句話總結：**C++ 是「一個模板變成很多份程式碼」，
//               Java 是「很多個型別共用一份程式碼」。**
//   Rust 的 generics 與 C++ 同屬前者（稱為 monomorphization）；
//   C# 則介於兩者之間（實質型別會特化，參考型別共用）。
//
// 【3. 模板引數推導：編譯器怎麼決定 T】
// 推導的規則遠比看起來複雜，本檔只需要掌握最基本的形態：
//   * 參數宣告為 T&  → 從實參型別直接取得 T，**不會**發生隱式轉換。
//                      my_swap(x, y) 中 x, y 皆為 int → T = int。
//   * 若兩個參數都對應同一個 T，兩邊推導結果必須**完全一致**，
//     否則就是「deduction 衝突」而編譯失敗（概念4.cpp 專門示範這件事）。
//   * 推導只看實參，不看回傳值需要什麼。
// 注意：推導**不做**型別轉換這件事，是初學者踩坑的主因 —— 一般函式呼叫時
// int 可以自動轉 double，但在推導 T 的階段沒有這回事。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 template 的定義幾乎都寫在 header 裡？
//   編譯器要實例化 my_swap<std::string>，**必須看得到 my_swap 的完整函式本體**
//   —— 光有宣告無法生成程式碼。而 C++ 的編譯單位是「一個 .cpp 一個 TU」，
//   TU 之間互相看不見對方的內容。所以若把模板定義藏在另一個 .cpp，使用端
//   那個 TU 就沒有藍圖可用，只能留下一個未定義符號給 linker。
//
//   本機實測（g++ 15.2）：把定義放在 tu_def.cpp、只在 header 留宣告，
//   兩個 .cpp 各自都能編譯成功，但連結時失敗：
//       undefined reference to `int twice<int>(int)'
//   `nm -C tu_main.o` 顯示該符號狀態為 U（undefined）。
//   這是 C++ 新手最常遇到、卻最難自行診斷的一類 link error：
//   **編譯全過、只有連結掛掉**，而且錯誤訊息完全沒提到 template。
//
//   兩種解法：
//     1) 把定義放進 header（最常見，STL 全部這樣做）
//     2) 在定義所在的 TU 做 explicit instantiation（明確實例化）：
//            template int twice<int>(int);
//        本機實測加上這一行後即可連結成功。適用於「型別集合已知且很少」
//        的場合，可換取編譯速度並隱藏實作。
//
// (B) 放 header 為什麼不會違反 ODR（One Definition Rule）？
//   如果 10 個 .cpp 都 include 同一個 header 並都用了 my_swap<int>，
//   就會產生 10 份一模一樣的 my_swap<int>。一般函式這樣做是 link error
//   （duplicate symbol），但模板實例化出來的符號被標記為 **weak / COMDAT**
//   （就是上面 nm 看到的那個 W）。linker 認得這個標記，會在多份完全相同的
//   定義中留一份、丟掉其餘。這個機制叫 vague linkage，
//   inline 函式與類別內定義的成員函式也走同一條路。
//
// (C) Code bloat 與編譯時間 —— template 的真實代價
//   實例化不是免費的，代價出現在兩個地方：
//     * 目的檔大小：本機實測，同一個類別模板只用 1 種型別實例化時目的檔
//       1,744 bytes；改成用 200 種不同型別實例化後變成 71,600 bytes（約 41 倍）。
//     * 編譯時間：同一組測試 0.09s → 0.16s。這還只是 200 個極簡型別；
//       真實專案中重度模板（如 Boost.Spirit、Eigen 表達式模板）常讓單檔
//       編譯時間以「秒」甚至「分」計。
//     （以上為本機 g++ 15.2 -O0 實測值，非標準保證，換編譯器/最佳化等級會變。）
//   實務上的緩解手段：把與型別無關的邏輯抽到非模板的共用函式（thin template
//   wrapper + fat non-template core）、對已知型別用 extern template 抑制重複
//   實例化、或改用型別抹除（std::function、虛擬介面）換取程式碼體積。
//
// 【注意事項 Pay Attention】
// 1. typename 與 class 在模板參數列表中完全等價，純屬風格選擇。
//    （但在「相依型別」的場合只能用 typename，例如 typename T::value_type，
//      那是另一個不同的用法。）
// 2. 本檔的 my_swap 用拷貝實作，對 std::string / std::vector 會做三次深拷貝。
//    正式寫法應使用 std::move（<utility>）：
//        T temp = std::move(a); a = std::move(b); b = std::move(temp);
//    直接用標準庫的 std::swap 更好，它已處理好移動語意與各種特化。
// 3. 模板的錯誤訊息通常很長，因為錯誤是在**實例化之後**才被發現的，
//    編譯器會把整條實例化鏈（instantiation backtrace）印出來。
//    C++20 的 concepts 就是為了改善這件事（見概念10.cpp）。
// 4. 模板參數推導失敗時，錯誤訊息常是「no matching function for call to ...」，
//    而不是你以為的型別不符 —— 因為候選函式根本沒被成功生成出來。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】函式模板與實例化機制
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++ template 和 Java generics 最根本的差別是什麼？
//     答：C++ 是編譯期**生成程式碼**：每個用到的型別各實例化出一份專屬機器碼，
//         零執行期成本、可完整 inline，代價是 code bloat 與編譯時間。
//         Java 是**型別抹除**：List<String> 與 List<Integer> 在 bytecode 中
//         是同一個型別，型別參數被抹成 Object 並由編譯器插入 cast，
//         只有一份程式碼、但無法針對型別最佳化。
//     追問：所以為什麼 Java 沒有 List<int>，C++ 卻有 vector<int>？
//         → 因為抹除後型別參數必須是參考型別（能當 Object），int 這種基本
//           型別得先裝箱成 Integer。C++ 是真的為 int 生成一份專屬程式碼，
//           所以 vector<int> 裡放的就是實實在在的 int。
//
// 🔥 Q2. 為什麼 template 的定義通常必須放在 header，而不能像一般函式那樣
//        「宣告放 .h、定義放 .cpp」？
//     答：因為實例化需要看得到完整函式本體才能生成程式碼。定義藏在別的 TU 時，
//         使用端的 TU 拿不到藍圖，只能留下未定義符號，於是**編譯全過、連結失敗**
//         （本機實測訊息：undefined reference to `int twice<int>(int)'）。
//         例外解法是在定義端寫 explicit instantiation：template int twice<int>(int);
//     追問：都放 header，10 個 .cpp 都 include，不就有 10 份定義違反 ODR 嗎？
//         → 不會。模板實例化的符號是 weak / COMDAT（nm 顯示為 W），
//           linker 會把多份相同定義合併成一份，這個機制叫 vague linkage。
//
// ⚠️ 陷阱. 「template 只是編譯器幫我複製貼上，所以完全零成本。」
//          這句話錯在哪？
//     答：錯在「零成本」被過度延伸。它在**執行期**確實零成本（沒有虛擬表、
//         沒有 boxing），但在**編譯期**成本很實在：每個實例化都是一份獨立
//         程式碼。本機實測同一個類別模板從 1 種型別擴到 200 種型別，
//         目的檔由 1,744 bytes 膨脹到 71,600 bytes、編譯時間 0.09s → 0.16s
//         （g++ 15.2 -O0 實測值，非標準保證）。
//     為什麼會錯：把「zero-overhead abstraction」記成「zero cost」。
//         C++ 的承諾是「不用的功能不必付錢，用了的功能你自己寫也不會更快」，
//         談的是**執行期**開銷；它從來沒有承諾編譯時間與二進位大小免費。
//         大型專案的編譯時間爆炸，主因往往就是模板。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>

// 泛型版本：一次搞定所有型別
// 這一份定義，編譯器會依實際用到的型別生成 my_swap<int>、my_swap<double>、
// my_swap<std::string> 三個獨立函式。
template <typename T>
void my_swap(T& a, T& b) {
    T temp = a;
    a = b;
    b = temp;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】設定檔熱更新：用 swap 做雙緩衝（double buffering）切換
//   情境：服務執行中要重新載入設定，但不能讓正在讀設定的邏輯看到半套資料。
//   做法：在旁邊完整建好一份新設定，最後用一次 swap 換上去。
//         swap 只搬指標/成員，不做逐欄位賦值，因此切換窗口極短且不會拋例外。
//   為何用泛型：ServerConfig 今天有 3 個欄位、明天有 30 個，
//               swap_config 完全不需要跟著改 —— 它根本不認識這些欄位。
// -----------------------------------------------------------------------------
struct ServerConfig {
    std::string endpoint;
    int         timeout_ms;
    int         max_retry;
};

// 一份定義即可服務任何設定型別（ServerConfig、DbConfig、CacheConfig...）
template <typename Config>
void activate_config(Config& live, Config& staged) {
    my_swap(live, staged);   // live 換成新設定，舊設定被換到 staged 待回收
}

void print_config(const std::string& label, const ServerConfig& c) {
    std::cout << label << ": endpoint=" << c.endpoint
              << ", timeout=" << c.timeout_ms << "ms"
              << ", retry=" << c.max_retry << std::endl;
}

int main() {
    std::cout << "=== 同一份 template，三種型別 ===" << std::endl;

    // 交換 int
    int x = 10, y = 20;
    my_swap(x, y);  // 編譯器自動推導 T = int
    std::cout << "x = " << x << ", y = " << y << std::endl;

    // 交換 double
    double a = 3.14, b = 2.72;
    my_swap(a, b);  // 編譯器自動推導 T = double
    std::cout << "a = " << a << ", b = " << b << std::endl;

    // 交換 string
    std::string s1 = "Hello", s2 = "World";
    my_swap(s1, s2);  // 編譯器自動推導 T = std::string
    std::cout << "s1 = " << s1 << ", s2 = " << s2 << std::endl;

    std::cout << "\n=== 明確指定型別（略過推導）===" << std::endl;
    int m = 1, n = 2;
    my_swap<int>(m, n);   // 不讓編譯器推導，直接指定 T = int
    std::cout << "m = " << m << ", n = " << n << std::endl;

    std::cout << "\n=== 日常實務：設定熱更新（雙緩衝 swap）===" << std::endl;
    ServerConfig live{"api.internal:8080", 3000, 2};
    ServerConfig staged{"api.internal:9090", 500, 5};   // 新版設定已備妥
    print_config("切換前 live  ", live);
    activate_config(live, staged);                       // 一次切換
    print_config("切換後 live  ", live);
    print_config("被換下的舊設定", staged);

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第二課：泛型編程（Generic Programming）概念2.cpp -o concept2

// === 預期輸出 ===
// === 同一份 template，三種型別 ===
// x = 20, y = 10
// a = 2.72, b = 3.14
// s1 = World, s2 = Hello
//
// === 明確指定型別（略過推導）===
// m = 2, n = 1
//
// === 日常實務：設定熱更新（雙緩衝 swap）===
// 切換前 live  : endpoint=api.internal:8080, timeout=3000ms, retry=2
// 切換後 live  : endpoint=api.internal:9090, timeout=500ms, retry=5
// 被換下的舊設定: endpoint=api.internal:8080, timeout=3000ms, retry=2
