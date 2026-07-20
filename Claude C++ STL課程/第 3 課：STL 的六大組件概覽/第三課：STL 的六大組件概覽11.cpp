// =============================================================================
//  第三課：STL 的六大組件概覽 11  —  std::bind、佔位符，以及為何 lambda 通常更好
// =============================================================================
//
// 【主題資訊 Information】
//   簽名：template <class F, class... Args>
//         /* unspecified */ bind(F&& f, Args&&... args);
//   標頭檔：<functional>（同時提供 std::placeholders::_1.._N）
//   標準版本：
//     std::bind          C++11 引入（取代 C++98 的 bind1st / bind2nd）
//     std::bind1st/2nd   C++11 起 deprecated，**C++17 正式移除**
//     std::not1/not2     C++17 deprecated，C++20 移除；改用 std::not_fn（C++17）
//     lambda             C++11；泛型 lambda（auto 參數）C++14
//   複雜度：bind 產生的可呼叫物件本身是 O(1)，但通常比 lambda 難以 inline。
//
// 【詳細解釋 Explanation】
//
// 【1. bind 在解決什麼問題：參數的「部分套用」】
//   std::greater<int> 需要兩個參數，但 count_if 只會餵一個。
//   bind 的工作是把「兩參數函式」轉成「單參數函式」，並固定其中一個：
//       auto gt5 = std::bind(std::greater<int>(), std::placeholders::_1, 5);
//       gt5(7);   // 等價於 std::greater<int>()(7, 5) → true
//   這在函數式程式設計中叫 partial application（部分套用）。
//   在 C++98 沒有 lambda 的年代，這是把外部值「綁進」述詞的唯一標準做法。
//
// 【2. 佔位符 _1 _2 _3 的真正意義：位置重排】
//   佔位符不只是「這裡留給呼叫者填」，它還指定「填第幾個實參」。
//   這讓 bind 能做到 lambda 之外的事 —— 交換參數順序：
//       auto f = std::bind(std::minus<int>(), _2, _1);
//       f(3, 10);   // 實際計算 10 - 3 = 7，參數被對調了
//   注意佔位符位於 std::placeholders 命名空間，用之前通常會寫
//       using namespace std::placeholders;
//   否則每次都得打 std::placeholders::_1。
//
// 【3. 為什麼現代 C++ 建議優先用 lambda】
//   (a) 可讀性：[](int n){ return n > 5; } 一眼看懂；
//       bind(greater<int>(), _1, 5) 得先在腦中還原成 greater(n, 5) 才知道是 n > 5。
//       參數順序尤其容易搞反 —— 這是 bind 最常見的 bug 來源。
//   (b) 效能：bind 回傳的是實作定義的包裝型別，內部用 tuple 存綁定的參數並在
//       呼叫時展開；雖然現代編譯器多半能最佳化掉，但 lambda 是編譯器直接生成的
//       closure，inline 更確定。
//   (c) 除錯：bind 的錯誤訊息以難讀著稱（層層樣板展開）。
//   (d) 相容性：bind 對多載函式、成員函式、可變參數的處理都有邊角案例；
//       lambda 沒有這些問題。
//   Scott Meyers 在《Effective Modern C++》Item 34 的結論就是：
//   **C++14 起 lambda 全面優於 bind；C++11 只在極少數情境（如移動捕獲）才需要 bind。**
//
// 【4. bind 至今仍偶有價值的地方】
//   - 綁定成員函式：std::bind(&Class::method, obj_ptr, _1)
//     （不過 C++11 起也可寫 [obj_ptr](auto x){ return obj_ptr->method(x); }）
//   - 需要重排參數順序時，bind 的寫法比 lambda 略短
//   - 維護大量 C++98/03 時期的既有程式碼時必須讀得懂它
//   實務建議：**新程式碼一律寫 lambda，讀舊程式碼時要認得 bind。**
//
// 【概念補充 Concept Deep Dive】
//   bind 的參數是**按值複製並存進內部 tuple** 的（除非用 std::ref/std::cref 包裝）。
//   這帶來兩個常被忽略的後果：
//     (1) 綁定大型物件會產生複製成本，且複製發生在 bind 呼叫的當下。
//     (2) 綁定的是「當下的值」，之後改變原變數不會影響已綁定的物件 ——
//         這與 lambda 的按值捕獲一致，但與按參考捕獲 [&] 不同。
//   要綁定參考必須明確寫出來：
//       int counter = 0;
//       auto f = std::bind(some_func, std::ref(counter));   // 綁參考
//   這正是 std::ref / std::cref 這兩個看起來很奇怪的函式存在的主要理由：
//   在「預設按值」的樣板轉發語境中，明確表達「我要參考語意」。
//   同樣的道理也適用於 std::thread 的建構子與 std::make_pair。
//
// 【注意事項 Pay Attention】
//   1. 佔位符在 std::placeholders 命名空間；忘了 using 會編譯錯誤。
//   2. bind 的參數預設**按值複製**；要參考語意必須用 std::ref / std::cref，
//      並自行確保被參考物件的生命週期。
//   3. std::bind1st / bind2nd 已於 C++17 移除；std::not1 / not2 於 C++20 移除。
//      舊教材上的寫法直接編不過，改用 lambda 或 std::not_fn。
//   4. bind 綁定多載函式時會失敗（無法推導要哪個多載），需要 static_cast
//      明確指定函式指標型別 —— 這是 bind 最惱人的邊角。
//   5. 巢狀 bind（把一個 bind 結果當另一個 bind 的參數）會被特別處理
//      並在呼叫時展開，語意複雜且極難閱讀，請避免。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::bind 與 lambda
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::bind 和 lambda 該怎麼選？
//     答：新程式碼一律優先 lambda。理由是可讀性（bind 需在腦中還原參數順序）、
//         inline 確定性（lambda 是編譯器直接生成的 closure）、
//         以及錯誤訊息可讀性。bind 只在維護舊程式碼、或需要重排參數順序時才值得。
//         Effective Modern C++ Item 34 的結論即為此。
//     追問：C++11 有哪個情境是 lambda 做不到而必須用 bind 的？
//           → 移動捕獲。C++11 的 lambda 沒有初始化捕獲，
//             無法把 unique_ptr 搬進 closure；用 bind 可以。
//             但 C++14 加入 [p = std::move(ptr)] 之後這個理由就消失了。
//
// 🔥 Q2. std::bind 綁定的參數是按值還是按參考？怎麼改變它？
//     答：預設**按值複製**，複製發生在呼叫 bind 的當下，之後改變原變數不影響它。
//         要參考語意必須用 std::ref(x) 或 std::cref(x) 明確包裝，
//         並自行確保被參考物件活得夠久。
//     追問：std::ref 為什麼存在？直接傳 & 不行嗎？
//           → 在樣板轉發語境中，參數型別是被推導的，
//             推導結果會把參考「衰減」成值。std::ref 回傳一個
//             std::reference_wrapper<T>，這是一個可複製的物件，
//             但能隱式轉回 T&，因此能安全地穿過按值傳遞的樣板層。
//             std::thread 的建構子也是同一個原因需要 std::ref。
//
// ⚠️ 陷阱. std::bind(std::minus<int>(), _1, 10) 和 std::bind(std::minus<int>(), 10, _1)
//          有什麼差別？很多人會答錯。
//     答：前者是「參數 - 10」，後者是「10 - 參數」，順序完全相反。
//         f1(3) = 3 - 10 = -7；f2(3) = 10 - 3 = 7。
//         佔位符 _1 所在的**位置**就是實參被填入的位置，不是「第一個參數」的意思。
//     為什麼會錯：把 _1 讀成「呼叫時傳的東西」而忽略它的位置語意，
//         於是兩種寫法看起來都像「拿參數跟 10 做減法」。
//         這正是 bind 最容易寫出安靜錯誤的地方 ——
//         而 lambda [](int n){ return n - 10; } 完全沒有這個歧義。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】不強加。
//   理由：std::bind 是「怎麼把一個可呼叫物件包成另一個形狀」的語法工具，
//         不對應任何演算法或資料結構，因此沒有哪一題 LeetCode 是「用 bind 解的」。
//         在競賽/面試碼中它更是被 lambda 完全取代（可讀性差、寫錯風險高）。
//         硬掛一題只會誤導讀者以為 bind 是解題技巧。
//         真正需要 bind 語意（把不同簽名的處理器統一成同一種形狀）的是
//         下面的「事件處理器註冊」實務範例。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】事件處理器註冊表（bind 的正當用途：統一簽名）
//   情境：一個小型事件系統，各模組要註冊自己的處理器。
//         但各模組的處理函式簽名不一致：
//           - 有的是自由函式 void(const string&)
//           - 有的是成員函式 void(Class::*)(const string&)
//           - 有的需要額外的固定參數（例如所屬子系統名稱）
//         事件匯流排只能存一種型別，因此都要包成 std::function<void(const string&)>。
//   為什麼用到本主題：bind 能把「成員函式 + 物件指標」與「多參數函式 + 固定值」
//         這兩種不同形狀，統一成同一個單參數簽名 —— 這是它最站得住腳的用途。
//         同時下面也給出等價的 lambda 寫法，讓讀者自己比較可讀性。
// -----------------------------------------------------------------------------
using Handler = std::function<void(const std::string&)>;

// (a) 自由函式，但多了一個「子系統名稱」參數
void logEvent(const std::string& subsystem, const std::string& event) {
    std::cout << "  [" << subsystem << "] " << event << std::endl;
}

// (b) 成員函式
class MetricsCollector {
    int count_ = 0;
public:
    void onEvent(const std::string& event) {
        ++count_;
        std::cout << "  [metrics] 第 " << count_ << " 筆事件：" << event << std::endl;
    }
    int count() const { return count_; }
};

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // std::bind：把二元的 greater 固定一邊，變成一元述詞
    // 等價於 [](int n) { return n > 5; }
    using namespace std::placeholders;
    auto is_greater_than_5 = std::bind(std::greater<int>(), _1, 5);

    int count = static_cast<int>(std::count_if(vec.begin(), vec.end(), is_greater_than_5));
    std::cout << "大於5的個數: " << count << std::endl;

    // 等價的 Lambda 寫法（更推薦）
    int count2 = static_cast<int>(std::count_if(vec.begin(), vec.end(),
        [](int n) { return n > 5; }
    ));
    std::cout << "大於5的個數（Lambda）: " << count2 << std::endl;

    // 佔位符的「位置」語意 —— bind 最容易寫錯的地方
    std::cout << "\n=== 佔位符的位置決定一切 ===" << std::endl;
    auto minus_10   = std::bind(std::minus<int>(), _1, 10);   // n - 10
    auto ten_minus  = std::bind(std::minus<int>(), 10, _1);   // 10 - n
    std::cout << "  bind(minus, _1, 10)(3) = " << minus_10(3)  << "   ← 3 - 10" << std::endl;
    std::cout << "  bind(minus, 10, _1)(3) = " << ten_minus(3) << "    ← 10 - 3" << std::endl;
    std::cout << "  對照 lambda：[](int n){return n-10;}(3) = "
              << [](int n) { return n - 10; }(3) << "  ← 沒有任何歧義" << std::endl;

    // 參數重排：bind 少數贏過 lambda 的地方
    std::cout << "\n=== 參數重排 ===" << std::endl;
    auto swapped = std::bind(std::minus<int>(), _2, _1);
    std::cout << "  bind(minus, _2, _1)(3, 10) = " << swapped(3, 10)
              << "  ← 實際算 10 - 3" << std::endl;

    // 綁定是「按值複製」，除非用 std::ref
    std::cout << "\n=== bind 預設按值複製 ===" << std::endl;
    int threshold = 5;
    auto by_value = std::bind(std::greater<int>(), _1, threshold);
    auto by_ref   = std::bind(std::greater<int>(), _1, std::cref(threshold));
    threshold = 100;
    std::cout << "  改 threshold 為 100 之後，判斷 50：" << std::endl;
    std::cout << "    按值綁定 : " << (by_value(50) ? "true" : "false")
              << "   ← 仍用舊值 5" << std::endl;
    std::cout << "    std::cref: " << (by_ref(50) ? "true" : "false")
              << "  ← 用新值 100" << std::endl;

    std::cout << "\n=== 日常實務：事件處理器註冊表 ===" << std::endl;
    MetricsCollector metrics;
    std::vector<std::pair<std::string, Handler>> registry;

    // (a) bind 固定第一個參數，把二參數自由函式變成單參數處理器
    registry.emplace_back("auth 記錄器",
                          std::bind(logEvent, "auth", _1));
    registry.emplace_back("payment 記錄器",
                          std::bind(logEvent, "payment", _1));

    // (b) bind 成員函式 + 物件指標
    registry.emplace_back("指標統計器",
                          std::bind(&MetricsCollector::onEvent, &metrics, _1));

    // (c) 等價的 lambda 寫法（現代 C++ 建議）
    registry.emplace_back("lambda 版記錄器",
                          [](const std::string& e) { logEvent("lambda", e); });

    std::cout << "已註冊 " << registry.size() << " 個處理器，派送事件："
              << std::endl;
    for (const auto& entry : registry) {
        entry.second("user.login");
    }
    std::cout << "MetricsCollector 累計收到 " << metrics.count() << " 筆" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第三課：STL 的六大組件概覽11.cpp -o demo11

// === 預期輸出 ===
// 大於5的個數: 5
// 大於5的個數（Lambda）: 5
//
// === 佔位符的位置決定一切 ===
//   bind(minus, _1, 10)(3) = -7   ← 3 - 10
//   bind(minus, 10, _1)(3) = 7    ← 10 - 3
//   對照 lambda：[](int n){return n-10;}(3) = -7  ← 沒有任何歧義
//
// === 參數重排 ===
//   bind(minus, _2, _1)(3, 10) = 7  ← 實際算 10 - 3
//
// === bind 預設按值複製 ===
//   改 threshold 為 100 之後，判斷 50：
//     按值綁定 : true   ← 仍用舊值 5
//     std::cref: false  ← 用新值 100
//
// === 日常實務：事件處理器註冊表 ===
// 已註冊 4 個處理器，派送事件：
//   [auth] user.login
//   [payment] user.login
//   [metrics] 第 1 筆事件：user.login
//   [lambda] user.login
// MetricsCollector 累計收到 1 筆
