// =============================================================================
//  第 10 課：vector 的宣告與初始化方式 3  —  初始化串列 initializer_list
// =============================================================================
//
// 【主題資訊 Information】
//   vector(std::initializer_list<T> init,
//          const Allocator& alloc = Allocator());          // C++11
//
//   標頭檔　：<vector>（std::initializer_list 本身定義在 <initializer_list>，
//             但 <vector> 會間接引入，所以平常不必自己 include）
//   標準版本：C++11
//   複雜度　：O(n)，n 為串列元素個數
//
//   三種等價寫法：
//     std::vector<int> v1 = {1, 2, 3, 4, 5};    // 複製初始化（最常見）
//     std::vector<int> v2  {1, 2, 3, 4, 5};     // 直接初始化，省略等號
//     std::vector<int> v3 ({1, 2, 3, 4, 5});    // 明確把串列當引數傳入（少見）
//
// 【詳細解釋 Explanation】
//
// 【1. initializer_list 解決了什麼問題？】
//   C++98 時代想給容器塞初值，只能這樣：
//       int tmp[] = {1, 2, 3};
//       std::vector<int> v(tmp, tmp + 3);       // 得先造一個陣列當跳板
//   或是 v.push_back(1); v.push_back(2); ... 一行一個，又臭又長。
//   而 C 的陣列卻可以直接 int a[] = {1,2,3};——「內建型別做得到、
//   使用者自訂型別做不到」正是 C++11 想消除的不一致。
//   initializer_list 讓任何 class 都能提供「大括號列舉初值」的語法。
//
// 【2. initializer_list 到底是什麼？】
//   它不是容器，而是一個「輕量的唯讀視窗」，內部只有兩個成員：
//   一根指向 const T 陣列的指標，加上元素個數。
//   編譯器看到 {1, 2, 3} 時：
//     ① 在某處（通常是唯讀資料段或 stack）建立一個 const T[3] 的暫存陣列
//     ② 造出一個指向該陣列的 initializer_list<T>
//     ③ 把它傳給建構子；vector 再把元素逐一「複製」進自己的 heap 空間
//   所以 initializer_list 只有 begin()、end()、size() 三個成員函式，
//   而且不能修改元素——因為型別是 const T。
//
// 【3. 為什麼元素一定被「複製」而不是「移動」？】
//   因為 initializer_list 的 begin() 回傳 const T*。
//   const 物件無法被移動（move 需要非 const 的右值參考），
//   所以 vector 只能退而求其次呼叫 copy constructor。
//   這帶來兩個實際後果：
//     * 效能：vector<std::string> v{s1, s2} 會複製兩個字串，不是搬移。
//     * 相容性：move-only 型別（std::unique_ptr、std::thread、
//       std::fstream）根本不能用大括號串列初始化——本機實測會編譯錯誤
//       「use of deleted function unique_ptr(const unique_ptr&)」。
//       正解是先建空 vector，再用 push_back(std::move(...)) 或 emplace_back。
//
// 【4. 三種寫法的細微差別】
//   v1 = {...} 是複製初始化，v2{...} 是直接初始化。對 vector 而言結果相同，
//   但複製初始化不能呼叫 explicit 建構子——這對其他 class 可能有差。
//   另外 v1 = {...} 這種寫法不允許 narrowing conversion（見注意事項 2），
//   這其實是大括號初始化的通則，不限於 vector。
//
// 【概念補充 Concept Deep Dive】
//   ▸ capacity 剛好等於元素個數：
//     本機 libstdc++ 實測 vector<int> v{1,2,3} 的 capacity() 是 3——
//     和 vector(n) 一樣，實作知道確切大小就配置剛好的量（【實作定義】）。
//
//   ▸ 暫存陣列的生命週期：
//     那個 const T[] 暫存陣列的壽命，和 initializer_list 物件本身相同。
//     在建構子呼叫式中它活到整個完整運算式結束，所以 vector 來得及複製完。
//     但若你把 initializer_list 存成成員變數或從函式回傳，暫存陣列早已消失
//     → dangling，這是 initializer_list 最危險的誤用（見注意事項 3）。
//
//   ▸ 對 vector<bool> 也一樣適用，但 vector<bool> 是 proxy 特化容器，
//     行為和其他 vector 不同，本課不深入（見後續課程）。
//
// 【注意事項 Pay Attention】
//   1. ★ 大括號會「強烈優先」選 initializer_list 建構子。
//      vector<int> v{5, 10} 是兩個元素 5 和 10，不是「5 個 10」。
//      這是本課最重要的陷阱，第 4 個檔案專門處理。
//   2. 大括號初始化禁止 narrowing conversion：
//      vector<int> v{1.5};       // 編譯錯誤（double → int 會失去精度）
//      vector<int> v(1, 1.5);    // 可以（小括號允許隱式轉換，值變成 1）
//      這是刻意的安全設計，屬於大括號初始化的通則。
//   3. 不要保存 initializer_list：把它存成成員或當回傳值，
//      背後的暫存陣列已經銷毀，之後存取是 UB。它只適合當「即用即棄」的參數。
//   4. move-only 型別不能用大括號串列建構（unique_ptr、thread 等），
//      因為串列元素是 const，無法移出。請改用 push_back/emplace_back。
//   5. 空的大括號 vector<int> v{}; 走的是 default constructor，
//      不是「含 0 個元素的 initializer_list」——結果一樣是空 vector，但路徑不同。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】initializer_list 建構
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::vector<std::string> v{s1, s2}; 這行會複製還是移動 s1、s2？
//     答：一定是複製。initializer_list::begin() 回傳 const T*，
//         const 物件無法被移動，所以 vector 只能呼叫 copy constructor。
//         想避免複製必須寫 v.push_back(std::move(s1));。
//     追問：那 vector<unique_ptr<int>> v{make_unique<int>(1)}; 呢？
//         → 編譯錯誤（本機實測：use of deleted function，複製建構子被刪除）。
//         move-only 型別完全不能用大括號串列初始化。
//
// 🔥 Q2. initializer_list 是容器嗎？它的元素存在哪裡？
//     答：不是容器，是「指標 + 長度」的輕量唯讀視窗，只有 begin/end/size。
//         元素放在編譯器產生的 const T[] 暫存陣列裡，
//         壽命和 initializer_list 物件相同（通常只到完整運算式結束）。
//     追問：所以能把 initializer_list 存成 class 成員嗎？
//         → 不行，暫存陣列會先銷毀，之後存取是 UB。它只適合當即用即棄的參數。
//
// ⚠️ 陷阱. std::vector<int> v{1.5, 2.5}; 能編譯嗎？vector<int> v(2, 1.5); 呢？
//     答：前者編譯錯誤，後者可以。大括號初始化禁止 narrowing conversion，
//         double → int 會失去精度所以被擋下；小括號則允許隱式轉換，
//         1.5 被截斷成 1，得到 {1, 1}。
//     為什麼會錯：以為「{} 只是另一種括號」。實際上大括號多了一層
//         型別安全檢查——這是它相對於小括號的優點，不是限制。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <memory>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【日常實務範例 1】HTTP 狀態碼分類：用初始化串列建立常數查表
//   情境：API gateway 需要判斷「這個回應碼要不要重試」。
//   為什麼用 initializer_list：這些碼是寫死的業務規則，數量固定、
//     內容一眼可讀，直接列舉遠比 push_back 五行清楚。
//     搭配 static const 讓它只建構一次，之後每次查詢都不再配置記憶體。
// -----------------------------------------------------------------------------
bool shouldRetry(int statusCode) {
    // 429 Too Many Requests、502/503/504 屬於暫時性錯誤，值得重試
    static const std::vector<int> kRetryable = {408, 429, 500, 502, 503, 504};

    for (int code : kRetryable) {
        if (code == statusCode) return true;
    }
    return false;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】上傳檔案的副檔名白名單
//   情境：使用者上傳頭像，只接受特定影像格式，其餘一律拒絕。
//   為什麼用 initializer_list：白名單是設定性質的固定清單，
//     用大括號直接列出，日後要增減格式時一目了然、不易改錯。
// -----------------------------------------------------------------------------
bool isAllowedImage(const std::string& extension) {
    static const std::vector<std::string> kAllowed = {"jpg", "jpeg", "png", "webp", "gif"};

    for (const std::string& ext : kAllowed) {
        if (ext == extension) return true;
    }
    return false;
}

int main() {
    std::cout << "=== 三種初始化串列寫法完全等價 ===\n";
    std::vector<int> v1 = {1, 2, 3, 4, 5};      // 複製初始化
    std::vector<int> v2  {1, 2, 3, 4, 5};       // 直接初始化
    std::vector<int> v3 ({1, 2, 3, 4, 5});      // 明確傳入串列

    auto dump = [](const char* tag, const std::vector<int>& v) {
        std::cout << tag << " size=" << v.size() << " capacity=" << v.capacity() << " 內容: ";
        for (int x : v) std::cout << x << " ";
        std::cout << "\n";
    };
    dump("v1 = {..}", v1);
    dump("v2   {..}", v2);
    dump("v3  ({..})", v3);
    std::cout << "三者內容相同？ " << std::boolalpha
              << (v1 == v2 && v2 == v3) << "\n";
    std::cout << "（capacity 剛好等於元素個數，是【實作定義】的觀察）\n";

    std::cout << "\n=== 空的大括號走的是 default ctor，不是空串列 ===\n";
    std::vector<int> empty1{};
    std::vector<int> empty2;
    std::cout << "vector<int> v{} size=" << empty1.size()
              << " / vector<int> v; size=" << empty2.size() << "（結果相同）\n";

    std::cout << "\n=== 串列元素是 const → 一定複製，不會移動 ===\n";
    std::string a = "Alpha", b = "Bravo";
    std::vector<std::string> words = {a, b};    // 複製，不是移動
    std::cout << "建構後原字串仍完好: a=[" << a << "] b=[" << b << "]\n";
    std::cout << "vector 內容: ";
    for (const std::string& w : words) std::cout << "[" << w << "] ";
    std::cout << "\n（若是移動，a、b 會變成未指定狀態；串列建構不會這樣做）\n";

    std::cout << "\n=== move-only 型別必須改用 push_back / emplace_back ===\n";
    // 下面這行無法編譯（unique_ptr 的複製建構子被刪除）：
    //   std::vector<std::unique_ptr<int>> bad{ std::make_unique<int>(1) };
    std::vector<std::unique_ptr<int>> ptrs;     // 正解：先建空 vector
    ptrs.push_back(std::make_unique<int>(10));
    ptrs.emplace_back(std::make_unique<int>(20));
    std::cout << "unique_ptr vector size=" << ptrs.size() << " 值: ";
    for (const auto& p : ptrs) std::cout << *p << " ";
    std::cout << "\n";

    std::cout << "\n=== 大括號禁止 narrowing、小括號允許隱式轉換 ===\n";
    // std::vector<int> narrow{1.5, 2.5};       // 編譯錯誤：narrowing conversion
    std::vector<int> paren(2, static_cast<int>(1.5));
    std::cout << "vector<int> v(2, (int)1.5) -> ";
    for (int x : paren) std::cout << x << " ";
    std::cout << "（1.5 被截斷為 1；大括號版本會直接編譯失敗）\n";

    std::cout << "\n=== 日常實務 1：HTTP 狀態碼是否值得重試 ===\n";
    const int codes[] = {200, 404, 429, 500, 503, 418};
    for (int c : codes) {
        std::cout << "  HTTP " << c << " -> " << (shouldRetry(c) ? "重試" : "不重試") << "\n";
    }

    std::cout << "\n=== 日常實務 2：上傳圖片副檔名白名單 ===\n";
    const std::vector<std::string> uploads = {"png", "exe", "webp", "svg", "jpeg"};
    for (const std::string& ext : uploads) {
        std::cout << "  ." << ext << " -> " << (isAllowedImage(ext) ? "接受" : "拒絕") << "\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 10 課：vector 的宣告與初始化方式3.cpp" -o lesson10_3

// === 預期輸出 ===
// === 三種初始化串列寫法完全等價 ===
// v1 = {..} size=5 capacity=5 內容: 1 2 3 4 5
// v2   {..} size=5 capacity=5 內容: 1 2 3 4 5
// v3  ({..}) size=5 capacity=5 內容: 1 2 3 4 5
// 三者內容相同？ true
// （capacity 剛好等於元素個數，是【實作定義】的觀察）
//
// === 空的大括號走的是 default ctor，不是空串列 ===
// vector<int> v{} size=0 / vector<int> v; size=0（結果相同）
//
// === 串列元素是 const → 一定複製，不會移動 ===
// 建構後原字串仍完好: a=[Alpha] b=[Bravo]
// vector 內容: [Alpha] [Bravo]
// （若是移動，a、b 會變成未指定狀態；串列建構不會這樣做）
//
// === move-only 型別必須改用 push_back / emplace_back ===
// unique_ptr vector size=2 值: 10 20
//
// === 大括號禁止 narrowing、小括號允許隱式轉換 ===
// vector<int> v(2, (int)1.5) -> 1 1 （1.5 被截斷為 1；大括號版本會直接編譯失敗）
//
// === 日常實務 1：HTTP 狀態碼是否值得重試 ===
//   HTTP 200 -> 不重試
//   HTTP 404 -> 不重試
//   HTTP 429 -> 重試
//   HTTP 500 -> 重試
//   HTTP 503 -> 重試
//   HTTP 418 -> 不重試
//
// === 日常實務 2：上傳圖片副檔名白名單 ===
//   .png -> 接受
//   .exe -> 拒絕
//   .webp -> 接受
//   .svg -> 拒絕
//   .jpeg -> 接受
