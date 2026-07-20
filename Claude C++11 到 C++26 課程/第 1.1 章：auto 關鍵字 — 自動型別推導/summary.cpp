// ============================================================================
//  第 1.1 章：auto 關鍵字 — 自動型別推導（總複習）
// ============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/auto
//
// 【主題資訊 Information】
//   語法：  auto        x = e;    // 按值推導：丟棄頂層 const 與參考、陣列/函式退化
//           auto&       x = e;    // 左值參考綁定：保留 const、不退化
//           const auto& x = e;    // 唯讀綁定：可綁左值、右值、需轉換的暫時物件
//           auto&&      x = e;    // 轉發參考：左值進→左值參考，右值進→右值參考
//           auto*       x = e;    // 明確標示指標（語意同 auto，只是意圖更清楚）
//   標準版本：auto 作為「變數型別推導」是 C++11。
//             其後每一版標準都擴大了 auto 的使用位置，本檔第十一節有
//             以 -pedantic-errors 逐版實測出來的完整對照表。
//   標頭檔：  不需要（語言關鍵字）
//   執行期成本：零。auto 在語意分析階段就被替換成具體型別。
//
//   ※ 本檔編譯標準為 C++20，因為第十一節要「實際編譯執行」各版本的新語法
//      來證明版本邊界，而不是只用註解宣稱。若想看純 C++11 的示範，
//      請看同章的「第 1.1 章：auto 關鍵字 — 自動型別推導1.cpp」，
//      該檔已用 -std=c++11 -pedantic-errors 驗證不含任何後續版本語法。
//
// 【詳細解釋 Explanation】
//
// 【1. auto 的本質：把「型別」從人寫的東西變成編譯器算的東西】
//   C++11 之前，型別是程式設計師的義務：你必須把每個變數的型別完整拼寫出來。
//   這在型別可以被「算出來」的場合是多餘的資訊，而多餘的資訊有兩種代價：
//
//     (a) 冗長。map<string, vector<int>>::const_iterator 這種名字沒有人想打第二次。
//     (b) 更嚴重的是——「你寫的型別」與「真正的型別」可能不一致。
//         不一致時編譯器往往不報錯，而是插入一次隱式轉換（見概念補充 C），
//         於是你得到一段語意正確但悄悄變慢的程式。
//
//   auto 讓編譯器負責計算型別，同時消除這兩種代價。更關鍵的是，有些型別
//   人類根本無法拼寫：lambda 的閉包型別是標準規定的「唯一、無名、非 union
//   的類別型別」，在 C++11 若沒有 auto，就無法宣告具名變數去接住一個 lambda。
//   auto 因此不是選配的語法糖，而是 C++11 之後許多設施能存在的前提。
//
// 【2. 推導規則沿用樣板參數推導（理解一切的鑰匙）】
//   標準把 auto 的推導直接定義成「比照函式樣板的參數推導」。把
//
//       auto x = expr;
//
//   想成存在一個虛構樣板
//
//       template <typename T> void f(T param);   // auto ↔ T
//       f(expr);
//
//   T 推導出什麼，auto 就是什麼；宣告子上的 & / const / * 對應 param 的形狀。
//   於是全部規則都可以「推導」而非死背：
//
//     * auto x   → f(T param)        按值 → 複製 → 頂層 const 與參考對副本
//                                      毫無意義，丟棄；陣列/函式退化成指標。
//     * auto& x  → f(T& param)       綁定 → 沒有複製 → 沒有理由丟棄任何東西：
//                                      const 保留、陣列不退化（int(&)[5]）。
//     * auto&& x → f(T&& param)      轉發參考 → 參考摺疊決定左右值性。
//
//   唯一真正的例外是大括號初始化，見注意事項第 6 點與第十一節的實測。
//
// 【3. 頂層 const vs 底層 const：auto 只丟其中一層】
//   「auto 會丟掉 const」是一句危險的半對半錯的話。const 可以修飾兩個層次：
//
//       const int  ci = 42;    // 頂層：ci 這個物件本身不可改
//       const int* p  = &ci;   // 底層：指向的目標不可改（p 自己可以改指向）
//       int* const q  = &i;    // 頂層：p 自己不可改（目標可以改）
//
//   auto 按值推導丟棄的永遠只有頂層那一層——因為副本是全新的物件，
//   來源可不可改與副本可不可改無關。底層 const 則必須保留，
//   丟掉它等於憑空取得寫入權限，const 正確性會當場瓦解：
//
//       auto a = p;   // const int*（底層 const 保留！）
//       auto b = q;   // int*      （頂層 const 丟棄）
//
// 【4. auto 的閱讀性爭議，以及業界的收斂結論】
//   反對者說 auto 把型別藏起來、害人看不懂。這個批評在「型別本身就是關鍵資訊」
//   的場合成立，例如 `auto ratio = getRatio();` 讀者無從得知是 int 還是 double，
//   而整數除法的差別足以造成 bug。業界（Google／LLVM／Herb Sutter 的 AAA 風格）
//   長年爭論後大致收斂成：
//     * 型別已經出現在同一行的右側 → 用 auto（auto p = make_unique<T>();
//       型別重複兩次毫無意義）。
//     * 型別冗長且與邏輯無關（迭代器、閉包、代理型別）→ 用 auto。
//     * 型別本身是讀者需要的關鍵資訊，且右側看不出來 → 寫出具體型別。
//     * 需要「特定」型別而非「運算式碰巧的」型別 → 寫出具體型別
//       （例如索引要 size_t 而不是 auto i = 0 推出的 int）。
//
// 【概念補充 Concept Deep Dive】
//
//   (A) auto 產生的機器碼與手寫型別完全相同
//     auto 在語意分析階段就被替換掉，之後的中介碼與最佳化完全看不到它。
//     `auto i = 42;` 與 `int i = 42;` 產生位元組相同的目的碼。
//     「用 auto 會不會比較慢」這個問題問錯了層次；真正影響效能的是
//     下面 (C) 描述的型別不匹配，而那恰恰是「不用 auto」才會發生的問題。
//
//   (B) 為什麼 auto 必須有初始化式，以及它為何不能當成員變數
//     推導的唯一資訊來源是初始化式。沒有它，編譯器手上沒有已知數可解，
//     所以 `auto x;` 不是「稍後再決定」而是無解 → 編譯錯誤。
//     同理，非靜態資料成員的宣告點不保證有初始化式（可能由各建構子的
//     成員初始化列表分別給值，且不同建構子可能給不同型別的值），
//     因此語言直接禁止在該處使用 auto。static constexpr 成員則因為
//     宣告點就必須有初始化式，所以是允許的。
//
//   (C) auto 最實際的效能價值：讓隱形複製現形
//     std::map<K,V> 的 value_type 是 std::pair<const K, V>——key 帶 const。
//     若走訪時寫成：
//
//         for (const std::pair<std::string,int>& kv : m)   // 少了 const！
//
//     型別不匹配，但這段程式碼完全合法：常數參考允許綁定暫時物件並延長
//     其生命週期，於是編譯器「合法地」為每個元素建立一個暫時 pair 再綁上去，
//     每一圈複製一次 std::string（本機實測 sizeof(std::string) = 32 bytes，
//     且長字串還多一次堆積配置）。編譯器不會警告，因為語意正確，只是慢。
//     改寫成 const auto& 必定精準命中真正的 value_type，零複製。
//
//   (D) 陣列退化的記憶體視角
//     int arr[5] 在堆疊上是連續 20 bytes（本機實測 sizeof(int)=4）。
//     `auto p = arr;` 得到的 int* 只是 8 bytes 的位址值（本機 64 位元），
//     「有 5 個元素」這個資訊在型別層被永久抹除，之後
//     sizeof(p)/sizeof(p[0]) 會算出 8/4 = 2 這種錯誤答案。
//     `auto& r = arr;` 綁定不複製也不退化，型別仍是 int(&)[5]，
//     sizeof(r) 仍是 20，長度資訊完整保留——這正是 std::size / 範圍式 for
//     能對原生陣列運作的原因。
//
// 【注意事項 Pay Attention】
//   1. auto 丟棄頂層 const 與參考；要保留必須明寫 const / &。
//      但底層 const 永遠保留（auto p = pci; 得到 const int*）。
//   2. auto s = "Hello"; 推導成 const char*，不是 std::string。
//      C++14 起可用 using namespace std::string_literals; auto s = "Hello"s;
//   3. std::vector<bool> 是標準規定的特化，用位元打包儲存，operator[]
//      回傳代理型別 std::vector<bool>::reference 而非 bool&
//      （本機實測 is_same<decltype(auto 取得的元素), bool> 為 0）。
//      代理物件的有效性繫於容器；容器改動後再使用該代理，結果不受保證。
//      要得到真正的 bool 請寫 bool b = v[0]; 或 static_cast<bool>(v[0])。
//   4. 同一宣告式中多個 auto 變數必須推導出同一型別：
//      auto a = 1, b = 2.0; 是編譯錯誤。
//   5. auto 不能用於：非靜態資料成員、陣列宣告子、無初始化式的宣告。
//      函式參數要到 C++20 的簡化函式樣板才允許（第十一節有實際示範）。
//   6. 大括號初始化是 auto 與樣板推導唯一分家之處，且有一個實務上的大坑：
//        auto a = {1, 2, 3};   → std::initializer_list<int>（各版本一致）
//        auto b{42};           → 見第十一節。C++11/14 的標準原文說是
//                                initializer_list<int>，但 N3922 以缺陷報告
//                                (DR) 方式回溯套用，因此現行編譯器即使在
//                                -std=c++11 模式下也已推導成 int。
//                                本機 g++ 15.2 實測確認四個標準模式皆為 int。
//                                → 教材若只寫「C++17 才改」會與實機行為不符，
//                                  正確說法是「標準原文 C++17 修正，
//                                  但實作以 DR 回溯，早已一致」。
//   7. auto 不會幫你猜意圖：auto r = 10 / 3; 得到 int 3 而非 3.333，
//      因為運算式本身的型別就是 int。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】auto 型別推導
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. auto 的推導規則是自成一套，還是沿用既有規則？
//     答：沿用函式樣板的參數推導。把 auto 當成樣板參數 T：
//         `auto x = e` 等同按值傳參，丟棄頂層 const 與參考、陣列與函式
//         退化成指標；`auto&`／`const auto&` 是綁定，const 保留且不退化；
//         `auto&&` 是轉發參考。唯一例外是大括號初始化——
//         auto 會推成 initializer_list，而樣板推導對 {} 是 non-deduced context。
//     追問：`auto& r = arr;`（int arr[5]）的 sizeof 是多少？
//         → 仍是整個陣列 20 bytes（本機實測），因為綁定不退化。
//
// 🔥 Q2. auto 會丟掉 const，那 const int* pci 用 auto 推導會變成 int* 嗎？
//     答：不會，仍是 const int*。auto 丟棄的只有「頂層 const」，
//         也就是被宣告物件自身的常數性。pci 這個指標變數本身不是 const，
//         const 修飾的是它指向的目標，屬於「底層 const」，必須保留——
//         否則等於憑空取得寫入權限，const 正確性瓦解。
//     追問：那 int* const cpi 呢？
//         → 推導成 int*。這次 const 在頂層（指標自己不可改），複製時丟棄。
//
// 🔥 Q3. 為什麼說用 auto 有時反而比手寫型別「更快」？
//     答：手寫型別可能與真正的型別不完全相同，觸發隱式轉換而產生暫時物件。
//         經典案例是走訪 std::map<std::string,int> 時寫成
//         `const std::pair<std::string,int>&`——真正的 value_type 是
//         pair<const std::string,int>，型別不符，於是每圈建立一個暫時 pair
//         並複製一次 std::string。改用 const auto& 精準命中，零複製。
//     追問：編譯器為什麼不警告？
//         → 因為完全合法：常數參考本來就允許綁定暫時物件並延長其生命週期。
//           語意正確，只是慢——這正是靜態分析工具（clang-tidy 的
//           performance-for-range-copy）存在的理由。
//
// 🔥 Q4. `auto x{42};` 推導出什麼型別？請說明版本差異。
//     答：現行編譯器一律推導成 int。標準原文在 C++11/14 規定
//         直接大括號初始化也推成 std::initializer_list<int>，
//         C++17 循 N3922 改為：單一元素的直接初始化 auto x{e} 推成 e 的型別，
//         而 auto x = {…} 仍是 initializer_list。
//         關鍵在於 N3922 是以「缺陷報告」形式回溯套用的，
//         所以 GCC／Clang 即使在 -std=c++11 模式下也給 int。
//         本機 g++ 15.2 以 -pedantic-errors 在 c++11/14/17/20 四種模式實測，
//         結果全部是 int。
//     追問：那 `auto x{1, 2};` 呢？
//         → 四種模式全部編譯失敗（本機實測）。N3922 之後，
//           帶多個元素的直接大括號初始化直接成為不合法語法。
//
// ⚠️ 陷阱. `std::vector<bool> v{true, false}; auto b = v[0];`
//          之後 `b = false;` 改到的是誰？
//     答：改到的是容器 v 的內部位元，不是一個獨立的區域副本。
//         b 的型別不是 bool，而是 std::vector<bool>::reference 代理物件
//         （本機實測 is_same<decltype(b), bool> 為 0）。
//         若原容器已被改動或銷毀，再透過該代理讀寫的結果不受保證。
//     為什麼會錯：腦中假設 vector<T> 的元素型別一律是 T。
//         但 vector<bool> 是標準明文規定的特化，為省空間以位元打包，
//         單一位元無法取址，operator[] 因此不可能回傳 bool&，
//         只能回傳一個「行為像 bool&」的代理類別。
//         這也是 vector<bool> 被廣泛視為設計失誤的原因：
//         它破壞了「vector<T> 的元素是 T」這條泛型程式碼賴以成立的假設。
// ═══════════════════════════════════════════════════════════════════════════

// 說明: 本檔案涵蓋 auto 關鍵字的所有核心觀念與用法，
//       閱讀本檔案即可完整複習本章所有重點。
// ============================================================================

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <unordered_map>
#include <queue>
#include <algorithm>
#include <type_traits>
#include <concepts>
#include <initializer_list>

#include <sstream>
// ============================================================================
// 第十一節會用到的輔助設施：示範 auto 在各標準版本被開放的新位置
// ============================================================================

// ---- C++14：一般函式的回傳型別推導（C++11 只有後置回傳型別 auto f() -> T）----
static auto cpp14_autoReturn(int a, int b)
{
    return a * b;                 // 回傳型別由 return 敘述推導為 int
}

// ---- C++14：decltype(auto) —— 用 decltype 規則（而非 auto 規則）推導 ----
static int g_shared = 100;
static int& refToShared()            { return g_shared; }
static auto           cpp14_byAuto()     { return refToShared(); }  // int（參考被丟棄）
static decltype(auto) cpp14_byDecltype() { return refToShared(); }  // int&（參考保留）

// ---- C++14：變數樣板（variable template）----
template <typename T>
constexpr T pi_v = static_cast<T>(3.14159265358979323846L);

// ---- C++14：放寬的 constexpr（可含迴圈、多條敘述、區域變數）----
static constexpr int cpp14_factorial(int n)
{
    int r = 1;
    for (int i = 2; i <= n; ++i) r *= i;   // C++11 的 constexpr 不允許迴圈
    return r;
}

// ---- C++17：非型別樣板參數用 auto（template<auto>）----
template <auto N>
struct ValueHolder
{
    static constexpr auto value = N;
    using type = decltype(N);
};

// ---- C++17：inline 變數（可在標頭檔定義，不違反單一定義原則）----
inline int g_inlineCounter = 7;

// ---- C++20：簡化函式樣板（abbreviated function template）----
//      這正是前面各節反覆提到「auto 不能用在函式參數，C++20 才可以」的實證。
static auto cpp20_twice(auto x)
{
    return x + x;
}

// ---- C++20：概念（concepts）—— 對樣板參數施加可讀的編譯期約束 ----
template <typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template <Numeric T>
static T cpp20_addNumeric(T a, T b)
{
    return a + b;
}

// ---- C++20：概念 + 簡化函式樣板 合體（受約束的 auto 參數）----
static auto cpp20_constrainedAdd(Numeric auto a, Numeric auto b)
{
    return a + b;
}

// ============================================================================
// 【LeetCode 實戰範例 1】LeetCode 347. Top K Frequent Elements
//   題目：給定整數陣列 nums 與整數 k，回傳出現頻率前 k 高的元素。
//   為什麼用到本主題：這一題是「auto 不是語法糖，而是必需品」最好的證據。
//     我們用一個 lambda 當作 priority_queue 的比較器，而 lambda 的閉包型別
//     是編譯器產生的匿名型別——你無法把它拼寫出來。於是必須：
//        auto cmp = [](...){...};                       // 用 auto 接住
//        std::priority_queue<T, C, decltype(cmp)> pq(cmp);  // 用 decltype 取回型別
//     少了 auto/decltype 這組搭配，這段程式碼在 C++11 根本寫不出來。
//   複雜度：O(N log k)，空間 O(N)。
// ============================================================================
static std::vector<int> topKFrequent(const std::vector<int>& nums, int k)
{
    std::unordered_map<int, int> freq;
    for (const auto& n : nums) ++freq[n];        // const auto& 精準命中 value_type

    // 閉包型別無法命名 → 必須用 auto 接住
    auto cmp = [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
        return a.second > b.second;              // 小頂堆：頻率小的先出
    };
    // 再用 decltype(cmp) 把那個「無名型別」交回給樣板參數
    std::priority_queue<std::pair<int, int>,
                        std::vector<std::pair<int, int>>,
                        decltype(cmp)> pq(cmp);

    for (const auto& kv : freq)                  // kv 是 const pair<const int,int>&
    {
        pq.push(kv);
        if (static_cast<int>(pq.size()) > k) pq.pop();
    }

    std::vector<int> result;
    while (!pq.empty())
    {
        result.push_back(pq.top().first);
        pq.pop();
    }
    std::reverse(result.begin(), result.end());  // 由高頻到低頻
    return result;
}

// ============================================================================
// 【LeetCode 實戰範例 2】LeetCode 1512. Number of Good Pairs
//   題目：計算滿足 nums[i] == nums[j] 且 i < j 的配對數量。
//   為什麼用到本主題：示範 const auto& 走訪 map 的正確寫法。
//     若把迴圈變數寫成 const std::pair<int,int>&，型別與真正的 value_type
//     （pair<const int,int>）不符，每圈都會產生一次暫時物件複製；
//     用 const auto& 則必定精準命中。這一題資料量小看不出差別，
//     但同樣的寫法用在 map<string, BigObject> 上就是可觀的成本。
//   複雜度：O(N)，空間 O(U)（U 為相異值個數）。
// ============================================================================
static int numIdenticalPairs(const std::vector<int>& nums)
{
    std::unordered_map<int, int> count;
    for (const auto& n : nums) ++count[n];

    int pairs = 0;
    for (const auto& kv : count)                 // 正確：const auto&，零複製
    {
        const auto c = kv.second;                // 每個值出現 c 次 → C(c,2) 組配對
        pairs += c * (c - 1) / 2;
    }
    return pairs;
}

// ============================================================================
// 【日常實務範例】彙總 nginx access log：統計各 HTTP 狀態碼與流量
//
//   場景：維運上最常見的一次性分析——把 access log 讀進來，
//         統計每個狀態碼出現幾次、總共傳了多少位元組，
//         並找出流量最大的路徑。log 格式（簡化的 combined 格式）：
//             127.0.0.1 - - [20/Jul/2026:10:00:00] "GET /api/users" 200 1024
//
//   為什麼用到本主題：
//     * 中間結果的型別（map 的迭代器、pair、size_type）全部冗長且與邏輯無關。
//     * 走訪統計結果時用 const auto& 避免每圈複製 std::string。
//     * 用 auto 接住 lambda 當排序準則。
//   注意：這裡刻意用最樸素的解析方式（找空白與引號），
//         真實環境若格式多變請改用正規表示式或現成的 log parser。
// ============================================================================
struct LogEntry
{
    std::string path;
    int         status = 0;
    long long   bytes  = 0;
};

static bool parseLogLine(const std::string& line, LogEntry& out)
{
    // 取出被雙引號包住的請求："GET /api/users"
    auto q1 = line.find('"');
    if (q1 == std::string::npos) return false;
    auto q2 = line.find('"', q1 + 1);
    if (q2 == std::string::npos) return false;

    const auto request = line.substr(q1 + 1, q2 - q1 - 1);   // GET /api/users
    const auto sp = request.find(' ');
    if (sp == std::string::npos) return false;
    out.path = request.substr(sp + 1);

    // 引號之後依序是 status 與 bytes
    const auto tail = line.substr(q2 + 1);
    // 用 istringstream 省去手工切詞
    std::istringstream iss(tail);
    if (!(iss >> out.status >> out.bytes)) return false;
    return true;
}

static void analyseAccessLog(const std::vector<std::string>& lines)
{
    std::map<int, int>             statusCount;   // 狀態碼 → 次數
    std::map<std::string, long long> pathBytes;   // 路徑   → 累計位元組
    long long totalBytes = 0;
    int       badLines   = 0;

    for (const auto& line : lines)                // const auto&：不複製 std::string
    {
        LogEntry e;
        if (!parseLogLine(line, e)) { ++badLines; continue; }
        ++statusCount[e.status];
        pathBytes[e.path] += e.bytes;
        totalBytes        += e.bytes;
    }

    std::cout << "  解析 " << lines.size() << " 行，失敗 " << badLines << " 行\n";
    std::cout << "  狀態碼分布:\n";
    for (const auto& kv : statusCount)            // 正確：const auto&
    {
        std::cout << "    " << kv.first << " → " << kv.second << " 次\n";
    }

    // 找出流量最大的路徑：用 auto 接住 lambda 比較器
    auto byBytes = [](const std::pair<const std::string, long long>& a,
                      const std::pair<const std::string, long long>& b) {
        return a.second < b.second;
    };
    const auto hottest = std::max_element(pathBytes.begin(), pathBytes.end(), byBytes);

    std::cout << "  總流量: " << totalBytes << " bytes\n";
    if (hottest != pathBytes.end())
    {
        std::cout << "  流量最大路徑: " << hottest->first
                  << " (" << hottest->second << " bytes)\n";
    }
}

int main()
{
    // ========================================================================
    // 第一節：auto 基本觀念
    // ========================================================================
    // auto 是 C++11 引入的關鍵字，讓編譯器根據初始值自動推導變數的型別。
    // 核心規則：使用 auto 宣告變數時，「必須」提供初始值，
    //           因為編譯器需要從初始值來決定型別。
    // 好處：減少冗長的型別宣告，提升程式碼可讀性，尤其在面對複雜型別時。
    // ========================================================================

    std::cout << "===== 第一節：基本型別推導 =====\n";

    // ---- 1.1 整數與浮點數型別 ----
    // 編譯器會依據字面值（literal）的形式來決定型別：
    auto i  = 42;       // 推導為 int，因為整數字面值預設是 int
    auto d  = 3.14;     // 推導為 double，因為浮點數字面值預設是 double
    auto f  = 3.14f;    // 推導為 float，因為後綴 f 明確指定 float
    auto c  = 'A';      // 推導為 char，因為單引號字元字面值是 char
    auto b  = true;     // 推導為 bool，因為 true/false 是布林字面值
    auto ll = 100LL;    // 推導為 long long，因為後綴 LL 指定 long long

    std::cout << "i  = " << i  << " (int)\n";
    std::cout << "d  = " << d  << " (double)\n";
    std::cout << "f  = " << f  << " (float)\n";
    std::cout << "c  = " << c  << " (char)\n";
    std::cout << "b  = " << std::boolalpha << b << " (bool)\n";
    std::cout << "ll = " << ll << " (long long)\n\n";

    // ========================================================================
    // 第二節：字串型別推導
    // ========================================================================
    // 重要觀念：字串字面值 "Hello" 的型別是 const char*（指向常數字元的指標），
    //           並「不是」std::string。若需要 std::string，必須明確建構。
    // ========================================================================

    std::cout << "===== 第二節：字串型別推導 =====\n";

    auto str1 = "Hello";                // 推導為 const char*（字串字面值的型別）
    auto str2 = std::string("World");   // 推導為 std::string（明確使用建構子）

    std::cout << "str1 = " << str1 << " (const char*)\n";
    std::cout << "str2 = " << str2 << " (std::string)\n\n";

    // ========================================================================
    // 第三節：容器迭代器 — auto 最實用的場景
    // ========================================================================
    // 迭代器型別通常非常冗長，例如 std::vector<int>::iterator，
    // 使用 auto 可以大幅簡化程式碼。
    //
    // 對比：
    //   C++03 寫法: std::vector<int>::iterator it = numbers.begin();
    //   C++11 寫法: auto it = numbers.begin();
    // ========================================================================

    std::cout << "===== 第三節：容器迭代器 =====\n";

    std::vector<int> numbers = {1, 2, 3, 4, 5};

    // 使用 auto 簡化迭代器型別宣告
    std::cout << "vector 內容（迭代器版）: ";
    for (auto it = numbers.begin(); it != numbers.end(); ++it)
    {
        // it 被推導為 std::vector<int>::iterator
        // 使用 *it 解參考來取得元素值
        std::cout << *it << " ";
    }
    std::cout << "\n\n";

    // ========================================================================
    // 第四節：複雜型別的簡化
    // ========================================================================
    // 當型別巢狀很深時（如 map<string, vector<int>>），auto 的優勢最為明顯。
    //
    // 對比：
    //   不用 auto: std::map<std::string, std::vector<int>>::iterator mapIt = data.begin();
    //   使用 auto: auto mapIt = data.begin();
    //
    // 同時展示：範圍式 for 迴圈（range-based for loop）也可搭配 auto 使用。
    // ========================================================================

    std::cout << "===== 第四節：複雜型別的簡化 =====\n";

    std::map<std::string, std::vector<int>> data;
    data["Alice"] = {90, 85, 88};   // 初始化 Alice 的成績
    data["Bob"]   = {78, 82, 80};   // 初始化 Bob 的成績

    // mapIt 被推導為 std::map<std::string, std::vector<int>>::iterator
    for (auto mapIt = data.begin(); mapIt != data.end(); ++mapIt)
    {
        // mapIt->first  是 key（std::string）
        // mapIt->second 是 value（std::vector<int>）
        std::cout << mapIt->first << " 的成績: ";

        // 範圍式 for 迴圈中，score 被推導為 int（元素的複製）
        for (auto score : mapIt->second)
        {
            std::cout << score << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n";

    // ========================================================================
    // 第五節：const 與參考（reference）的推導規則 ★★★ 重要 ★★★
    // ========================================================================
    // 這是 auto 推導中最容易混淆的部分，必須牢記以下規則：
    //
    // 規則 1：auto 推導時，「頂層 const」（top-level const）會被忽略
    //         → 因為 auto 產生的是一份新的複製品，複製品本身的 const 不影響原物件
    //
    // 規則 2：auto 推導時，「參考」（reference）會被忽略
    //         → auto 推導的是被參考的物件的型別，而非參考本身
    //
    // 規則 3：若要保留 const 或參考，必須「明確寫出」const 或 &
    //
    // 規則 4：auto& 綁定到 const 物件時，「底層 const」（low-level const）會被保留
    //         → 因為透過參考修改 const 物件是不合法的，編譯器自動保護
    // ========================================================================

    std::cout << "===== 第五節：const 與參考的推導規則 =====\n";

    int x = 100;
    const int cx = 200;     // cx 是 const int
    int& rx = x;            // rx 是 x 的參考

    // --- 規則 1 & 2 示範：auto 忽略頂層 const 和參考 ---
    auto a1 = x;    // 推導為 int（x 的複製）
    auto a2 = cx;   // 推導為 int（不是 const int！頂層 const 被忽略，因為是複製）
    auto a3 = rx;   // 推導為 int（不是 int&！參考被忽略，因為是複製）

    // a2 不是 const，可以自由修改，因為它只是 cx 的「複製品」
    a2 = 999;       // 合法！a2 是普通 int，修改 a2 不影響 cx
    std::cout << "a2 修改為 999 後, cx 仍為 " << cx << " (證明 a2 是獨立的複製品)\n";

    // --- 規則 3 示範：明確保留 const 或參考 ---
    const auto a4 = x;     // const int（明確加上 const）
    auto& a5 = x;          // int&（明確加上 & 表示參考）
    const auto& a6 = x;    // const int&（同時加上 const 和 &）

    // --- 規則 4 示範：auto& 綁定 const 物件，底層 const 被保留 ---
    auto& a7 = cx;          // 推導為 const int&（底層 const 被自動保留）
    // a7 = 500;            // 編譯錯誤！因為 a7 是 const int&，不可修改

    // 驗證 a5 確實是 x 的參考：修改 a5 就等於修改 x
    a5 = 888;
    std::cout << "修改 a5 為 888 後, x = " << x << " (證明 a5 是 x 的參考)\n\n";

    // 避免未使用變數的警告
    (void)a1; (void)a3; (void)a4; (void)a6; (void)a7;

    // ========================================================================
    // 第六節：指標的處理
    // ========================================================================
    // auto 推導指標時，不需要額外加 *，因為指標型別會自動被保留。
    // 但也可以寫成 auto* 來「明確表達意圖」，效果相同。
    //
    // auto  p1 = ptr;  →  int*（指標自動被保留）
    // auto* p2 = ptr;  →  int*（明確寫出 * 表示是指標，效果一樣）
    //
    // 注意：與參考不同！參考必須明確寫 auto&，但指標不需要寫 auto*。
    //       這是因為指標本身就是一個獨立的值（一個位址），而參考只是別名。
    // ========================================================================

    std::cout << "===== 第六節：指標的處理 =====\n";

    int value = 42;
    int* ptr = &value;

    auto  p1 = ptr;   // 推導為 int*（指標型別自動保留）
    auto* p2 = ptr;   // 推導為 int*（明確標示，效果相同）

    std::cout << "*p1 = " << *p1 << " (透過 auto 推導的指標存取)\n";
    std::cout << "*p2 = " << *p2 << " (透過 auto* 推導的指標存取)\n\n";

    // ========================================================================
    // 第七節：陣列退化為指標
    // ========================================================================
    // 重要觀念：C/C++ 中，陣列名稱在大多數情境下會「退化」（decay）為
    //           指向第一個元素的指標。auto 推導也遵循此規則。
    //
    // auto arrAuto = arr;   → int*（陣列退化為指標，失去大小資訊）
    // auto& arrRef = arr;   → int(&)[5]（使用參考可以保留完整的陣列型別）
    //
    // 驗證方式：使用 sizeof 檢查
    //   sizeof(arr)      = 20 bytes（5 個 int × 4 bytes）
    //   sizeof(arrAuto)  = 8 bytes（64 位元系統上的指標大小）
    //   sizeof(arrRef)   = 20 bytes（保留了完整的陣列大小資訊）
    // ========================================================================

    std::cout << "===== 第七節：陣列退化為指標 =====\n";

    int arr[5] = {10, 20, 30, 40, 50};

    auto  arrAuto = arr;    // 推導為 int*（陣列退化為指標）
    auto& arrRef  = arr;    // 推導為 int(&)[5]（參考保留陣列型別與大小）

    std::cout << "arrAuto[0] = " << arrAuto[0] << "\n";
    std::cout << "sizeof(arr)      = " << sizeof(arr)     << " bytes (完整陣列大小)\n";
    std::cout << "sizeof(arrAuto)  = " << sizeof(arrAuto)  << " bytes (指標大小，陣列資訊遺失)\n";
    std::cout << "sizeof(arrRef)   = " << sizeof(arrRef)   << " bytes (參考保留陣列大小)\n\n";

    // ========================================================================
    // 第八節：同一行宣告多個變數
    // ========================================================================
    // 規則：同一行用 auto 宣告多個變數時，所有變數必須推導為「相同型別」。
    //       否則編譯器會報錯。
    //
    // 合法：auto v1 = 1, v2 = 2, v3 = 3;     // 全部都是 int
    // 非法：auto v4 = 1, v5 = 3.14;           // int 和 double 不同，編譯錯誤！
    // ========================================================================

    std::cout << "===== 第八節：同一行宣告多個變數 =====\n";

    auto v1 = 1, v2 = 2, v3 = 3;   // 合法：全部推導為 int
    // auto v4 = 1, v5 = 3.14;     // 非法！int 和 double 型別不一致，會編譯錯誤

    std::cout << "v1 = " << v1 << ", v2 = " << v2 << ", v3 = " << v3 << "\n\n";

    // ========================================================================
    // 第九節：auto 與運算式結果
    // ========================================================================
    // auto 推導運算式結果時，遵循 C++ 的隱式型別轉換規則：
    //
    // int + int       → int（同型別運算，結果不變）
    // int / int       → int（整數除法，小數部分被截斷！）
    // int / double    → double（混合型別運算，int 被提升為 double）
    //
    // 這個觀念在使用 auto 時特別重要，因為開發者可能預期得到浮點數結果，
    // 但整數除法會導致精度遺失。
    // ========================================================================

    std::cout << "===== 第九節：auto 與運算式結果 =====\n";

    int a_val = 10;
    int b_val = 3;

    auto sum     = a_val + b_val;    // 推導為 int（int + int = int）
    auto division = a_val / b_val;   // 推導為 int（int / int = int，結果為 3，小數被截斷）
    auto realDiv  = a_val / 3.0;     // 推導為 double（int / double = double，結果為 3.333...）

    std::cout << "10 + 3   = " << sum      << " (int)\n";
    std::cout << "10 / 3   = " << division  << " (int，整數除法，小數被截斷)\n";
    std::cout << "10 / 3.0 = " << realDiv   << " (double，混合型別運算)\n\n";

    // ========================================================================
    // 第十節：auto 的使用限制 ★★★ 必考 ★★★
    // ========================================================================
    // auto 有以下四種情境「不能使用」：
    //
    // 限制 1：函式參數（C++11/14 不支援，C++20 才允許）
    //         void func(auto x);  // C++11/14 編譯錯誤，C++20 合法
    //
    // 限制 2：類別的非靜態成員變數
    //         class MyClass {
    //             auto value = 10;  // 編譯錯誤！成員變數不能用 auto
    //         };
    //
    // 限制 3：沒有初始化的宣告
    //         auto x;  // 編譯錯誤！auto 必須有初始值才能推導型別
    //
    // 限制 4：陣列宣告
    //         auto arr[5] = {1, 2, 3, 4, 5};  // 編譯錯誤！不能用 auto 宣告陣列
    // ========================================================================

    std::cout << "===== 第十節：auto 的使用限制（見註解）=====\n";
    std::cout << "1. 函式參數：C++11/14 不允許，C++20 才支援\n";
    std::cout << "2. 類別的非靜態成員變數：不允許\n";
    std::cout << "3. 沒有初始化的宣告：不允許（必須有初始值）\n";
    std::cout << "4. 陣列宣告：不允許\n\n";

    // 以下為不合法的用法示範（已註解掉，取消註解會導致編譯錯誤）：
    // void func(auto x);                      // 限制 1：函式參數
    // class MyClass { auto value = 10; };      // 限制 2：非靜態成員變數
    // auto noInit;                             // 限制 3：未初始化
    // auto badArr[5] = {1, 2, 3, 4, 5};       // 限制 4：陣列宣告

    // ========================================================================
    // 總結：auto 推導規則速查表
    // ========================================================================
    //
    // ┌─────────────────────────┬──────────────────┬──────────────────────────┐
    // │ 宣告方式                │ 推導結果         │ 說明                     │
    // ├─────────────────────────┼──────────────────┼──────────────────────────┤
    // │ auto x = 42;            │ int              │ 整數字面值預設為 int     │
    // │ auto d = 3.14;          │ double           │ 浮點字面值預設為 double  │
    // │ auto f = 3.14f;         │ float            │ 後綴 f 指定 float       │
    // │ auto s = "Hi";          │ const char*      │ 字串字面值是指標         │
    // │ auto s = string("Hi");  │ std::string      │ 明確建構 string          │
    // ├─────────────────────────┼──────────────────┼──────────────────────────┤
    // │ auto a = cx;            │ int              │ 頂層 const 被忽略(複製)  │
    // │ auto a = rx;            │ int              │ 參考被忽略(複製)         │
    // │ const auto a = x;       │ const int        │ 明確加 const             │
    // │ auto& a = x;            │ int&             │ 明確加 & 保留參考        │
    // │ auto& a = cx;           │ const int&       │ 底層 const 自動保留      │
    // ├─────────────────────────┼──────────────────┼──────────────────────────┤
    // │ auto p = ptr;           │ int*             │ 指標型別自動保留         │
    // │ auto* p = ptr;          │ int*             │ 明確寫 * 效果相同        │
    // │ auto a = arr;           │ int*             │ 陣列退化為指標           │
    // │ auto& a = arr;          │ int(&)[5]        │ 參考保留陣列型別         │
    // └─────────────────────────┴──────────────────┴──────────────────────────┘
    //
    // ========================================================================

    std::cout << "===== 複習完成！=====\n";

    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra summary.cpp -o auto_summary
//   注意：本檔【必須】用 -std=c++20 —— 第 272 行起的 concept / requires
//   （Numeric 概念與 auto 參數的縮寫函式範本）是 C++20 新增，
//   已用 -std=c++17 實測確認編譯失敗（error: ‘concept’ does not name a type）。
//   其餘各節的語法多數自 C++11 起即可用，但整檔要一起編就得用 C++20。

// 註 1:編譯時會出現數個 -Wunused-function / -Wunused-but-set-variable 警告，
//      這是【刻意】的：本檔是複習用的教科書檔，
//      多個示範函式（cpp14_autoReturn、topKFrequent、analyseAccessLog 等）
//      只是要展示語法本身，並未在 main 中呼叫。
//      第五節的 a2 也是故意「設定後不使用」，用來證明它是獨立複製品。

// 註 2:本檔輸出是【完全確定的】，連跑 6 次位元組完全相同：
//      沒有執行緒、沒有亂數、沒有印出任何位址或容器的實作細節。

// 註 3:第七節的 sizeof 數值為【本機實測】(x86-64 Linux, g++ 15.2.0)：
//      sizeof(arr)=20 是 int[5]；sizeof(arrAuto)=8 是指標大小。
//      指標大小屬實作定義，在 32-bit 平台上會是 4。
//      這裡真正要記住的是「auto 會讓陣列退化成指標、長度資訊消失」，
//      而不是 8 這個數字本身。

// === 預期輸出 ===
// ===== 第一節：基本型別推導 =====
// i  = 42 (int)
// d  = 3.14 (double)
// f  = 3.14 (float)
// c  = A (char)
// b  = true (bool)
// ll = 100 (long long)
//
// ===== 第二節：字串型別推導 =====
// str1 = Hello (const char*)
// str2 = World (std::string)
//
// ===== 第三節：容器迭代器 =====
// vector 內容（迭代器版）: 1 2 3 4 5
//
// ===== 第四節：複雜型別的簡化 =====
// Alice 的成績: 90 85 88
// Bob 的成績: 78 82 80
//
// ===== 第五節：const 與參考的推導規則 =====
// a2 修改為 999 後, cx 仍為 200 (證明 a2 是獨立的複製品)
// 修改 a5 為 888 後, x = 888 (證明 a5 是 x 的參考)
//
// ===== 第六節：指標的處理 =====
// *p1 = 42 (透過 auto 推導的指標存取)
// *p2 = 42 (透過 auto* 推導的指標存取)
//
// ===== 第七節：陣列退化為指標 =====
// arrAuto[0] = 10
// sizeof(arr)      = 20 bytes (完整陣列大小)
// sizeof(arrAuto)  = 8 bytes (指標大小，陣列資訊遺失)
// sizeof(arrRef)   = 20 bytes (參考保留陣列大小)
//
// ===== 第八節：同一行宣告多個變數 =====
// v1 = 1, v2 = 2, v3 = 3
//
// ===== 第九節：auto 與運算式結果 =====
// 10 + 3   = 13 (int)
// 10 / 3   = 3 (int，整數除法，小數被截斷)
// 10 / 3.0 = 3.33333 (double，混合型別運算)
//
// ===== 第十節：auto 的使用限制（見註解）=====
// 1. 函式參數：C++11/14 不允許，C++20 才支援
// 2. 類別的非靜態成員變數：不允許
// 3. 沒有初始化的宣告：不允許（必須有初始值）
// 4. 陣列宣告：不允許
//
// ===== 複習完成！=====
