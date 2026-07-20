// =============================================================================
//  第三課：STL 的六大組件概覽 6  —  函數物件（Functor）：函式指標 vs 仿函式 vs 帶狀態
// =============================================================================
//
// 【主題資訊 Information】
//   概念：「函數物件（function object / functor）」= 任何多載了 operator() 的類別物件。
//         凡是能寫成 f(args...) 的東西都叫 Callable：
//           函式指標、functor、lambda（其實就是編譯器產生的 functor）、
//           std::function、成員函式指標（需 std::invoke / std::mem_fn 包裝）。
//   簽名：
//     class IsEven { public: bool operator()(int n) const { return n % 2 == 0; } };
//     ptrdiff_t count_if(InputIt first, InputIt last, UnaryPredicate p);   // <algorithm>
//   標準版本：functor 自 C++98；lambda 是 C++11；泛型 lambda（auto 參數）是 C++14。
//             std::unary_function / binary_function 於 C++11 起 deprecated，
//             C++17 正式移除 —— 舊教材要繼承它們的寫法已經不能編了。
//   複雜度：count_if 為 O(N)，呼叫 N 次述詞（predicate）。
//   標頭檔：<algorithm>（count_if）、<functional>（std::function、內建 functor）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼要有 functor —— 函式指標不夠用在哪】
//   函式指標有兩個致命缺點：
//     (a) 不能帶狀態。
//         想要「判斷是不是 3 的倍數」與「判斷是不是 5 的倍數」，
//         用函式指標就得寫兩個函式，或者用全域變數（多執行緒下立刻爆炸）。
//         functor 把 divisor 存成成員變數，同一個類別就能產生無限多種述詞。
//     (b) 難以 inline。
//         傳函式指標時，演算法內部拿到的是一個「執行期才知道指向哪」的位址，
//         編譯器通常無法把呼叫展開；每個元素都要付一次真正的 call 指令。
//         functor 傳的是「型別」—— 每個 functor 型別會讓 count_if 實例化出
//         一份專屬版本，operator() 的本體直接被 inline 進迴圈。
//
// 【2. 型別即資訊：為什麼 functor 能被 inline】
//   關鍵在於 count_if 的樣板參數：
//       template <class It, class Pred>
//       ptrdiff_t count_if(It first, It last, Pred p);
//   傳 IsEven() 時，Pred = IsEven，編譯器在實例化時就完全知道 p(x) 要執行哪段程式碼，
//   於是能整個展開成 (x % 2 == 0)，連函式呼叫都不存在。
//   傳 bool(*)(int) 時，Pred = bool(*)(int) —— 型別只說明「這是個函式指標」，
//   到底指向哪個函式要看執行期的值，因此通常只能乖乖做間接呼叫。
//   （現代編譯器在能看到完整定義且成功做常數傳播時，仍可能把函式指標去虛擬化，
//     但那是「可能」，functor 則是「保證有機會」。）
//
// 【3. 空 functor 不佔空間：EBO】
//   IsEven 沒有任何成員變數，sizeof(IsEven) 仍然是 1（C++ 規定完整物件至少 1 byte，
//   以保證不同物件位址相異）。但當它被當成基底類別時，
//   空基底最佳化（Empty Base Optimization, EBO）可以讓它完全不佔空間 ——
//   這正是 std::vector 能在同時持有 allocator 的情況下，
//   sizeof(vector<int>) 仍然只有 24 bytes（三個指標）的原因。
//
// 【4. lambda 其實就是 functor 的語法糖】
//   編譯器看到 [divisor](int n) { return n % divisor == 0; } 時，
//   會產生一個大致如下的匿名類別（closure type）：
//       class __lambda_1 {
//           int divisor;                          // 捕獲的變數變成成員
//       public:
//           explicit __lambda_1(int d) : divisor(d) {}
//           bool operator()(int n) const { return n % divisor == 0; }
//       };
//   所以本檔的 IsDivisibleBy 與對應 lambda 在編譯後幾乎是同一份東西。
//   差別只在：lambda 寫起來短、且每個 lambda 運算式都有**獨一無二的型別**
//   （即使兩個 lambda 長得一模一樣，型別也不同）。
//
// 【概念補充 Concept Deep Dive】
//   「述詞（predicate）必須是純函式」是 STL 的隱含要求。
//   標準規定傳給演算法的述詞不得修改被走訪的元素，也不應依賴自身被呼叫的次數
//   —— 因為標準並未保證述詞被呼叫的次數與順序（實作可能為了最佳化改變它）。
//   典型的踩雷：
//       int count = 0;
//       std::count_if(v.begin(), v.end(), [&count](int){ ++count; return true; });
//   這種「靠副作用計數」的寫法在不同實作、不同最佳化層級下可能得到不同結果。
//   若真的要邊走邊累積狀態，正確工具是 std::for_each（標準保證恰好呼叫 N 次、
//   且依序），或直接寫迴圈。
//
// 【注意事項 Pay Attention】
//   1. functor 的 operator() 請加 const。演算法會複製述詞，
//      若 operator() 非 const 而你又依賴它累積的狀態，複製會讓狀態遺失。
//   2. std::unary_function / std::binary_function 在 C++17 已移除，別再繼承。
//   3. 述詞不應有副作用；呼叫次數與順序未被標準保證（for_each 例外）。
//   4. 傳 functor 是傳「值」（複製），大型 functor 要留意複製成本；
//      需要共享狀態時請捕獲參考或用 std::ref，並自行確保生命週期。
//   5. std::function 雖然通用，但會做型別抹除（type erasure），
//      通常伴隨一次間接呼叫、可能的堆積配置，且阻擋 inline ——
//      當樣板參數就夠用時不要用 std::function。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】函數物件（Functor）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 STL 演算法偏好 functor 而不是函式指標？
//     答：兩個理由。(1) functor 可以攜帶狀態（成員變數），
//         同一個類別能產生無限多種述詞，函式指標只能靠全域變數（不安全）。
//         (2) functor 是以「型別」傳遞，樣板實例化時編譯器完全知道要執行哪段碼，
//         operator() 能被 inline；函式指標的目標要執行期才知道，通常無法展開。
//     追問：那 std::function 呢？它能不能取代 functor？
//           → 不建議當預設。std::function 做型別抹除以支援「任意 Callable」，
//             代價是一次間接呼叫、可能的堆積配置、且阻擋 inline。
//             只有在真的需要「把不同型別的 Callable 存進同一個容器 / 成員變數」
//             時才用它。
//
// 🔥 Q2. lambda 和 functor 有什麼關係？
//     答：lambda 就是編譯器自動產生的 functor。捕獲的變數變成 closure 類別的
//         成員變數，lambda 本體變成 operator() const。所以兩者效能特性相同，
//         lambda 只是寫法更短。
//     追問：兩個內容完全相同的 lambda，型別是不是一樣？
//           → 不一樣。每個 lambda **運算式**都會產生獨一無二的 closure type，
//             即使原始碼一字不差。這也是為什麼不能寫
//             `decltype(lambda_a) b = lambda_b;`，以及為什麼需要 std::function
//             才能把不同 lambda 存進同一個容器。
//
// ⚠️ 陷阱. 帶狀態的 functor 想在 for_each 中累加計數，為什麼結果常常是 0？
//     答：因為演算法是**按值**接收述詞的 —— 它操作的是你那個物件的副本，
//         累加的狀態留在副本裡，函式返回後就消失了。
//         正解一：用 std::for_each 的**回傳值**（它會回傳那份被操作過的 functor）。
//         正解二：捕獲參考（[&sum]）或改用 std::ref 包裝。
//     為什麼會錯：把 functor 想成「傳進去的是我這個物件本人」，
//         但簽名 `count_if(It, It, Pred p)` 的 p 是按值參數。
//         這也是為什麼標準特別規定 for_each 會把 functor 回傳給你。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

// 方法一：普通函數
bool is_even_func(int n) {
    return n % 2 == 0;
}

// 方法二：函數物件
class IsEven {
public:
    bool operator()(int n) const {
        return n % 2 == 0;
    }
};

// 方法三：帶狀態的函數物件
class IsDivisibleBy {
    int divisor;
public:
    IsDivisibleBy(int d) : divisor(d) {}
    bool operator()(int n) const {
        return n % divisor == 0;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 179. Largest Number
//   題目：給一組非負整數，重新排列後串接，組出「數值最大」的字串。
//         例如 [3,30,34,5,9] → "9534330"。
//   為什麼用到本主題：這題的核心就是**自訂比較器**——不能比大小，
//         要比 (a+b) 與 (b+a) 兩種串接結果誰大。這正是 functor 存在的理由：
//         把「怎麼算誰該排前面」這個策略包成物件交給 std::sort。
//         這裡刻意用 struct functor 而非 lambda，讓比較器的本質看得更清楚。
//   複雜度：時間 O(N log N × L)（L 為數字字串長度）、空間 O(N)。
//   注意：比較器必須是嚴格弱序 —— a+b > b+a 用的是 >（不含等於），正確。
// -----------------------------------------------------------------------------
struct ConcatGreater {
    bool operator()(const std::string& a, const std::string& b) const {
        return a + b > b + a;   // a 排在 b 前面，若 ab 串起來比 ba 大
    }
};

std::string largestNumber(const std::vector<int>& nums) {
    std::vector<std::string> parts;
    parts.reserve(nums.size());
    for (int n : nums) parts.push_back(std::to_string(n));

    std::sort(parts.begin(), parts.end(), ConcatGreater{});

    // 特例：全是 0 時會串出 "000...", 要收斂成 "0"
    if (!parts.empty() && parts[0] == "0") return "0";

    std::string result;
    for (const std::string& p : parts) result += p;
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】可設定門檻的日誌過濾器（log level filter）
//   情境：監控系統要能在執行期依設定檔決定「只保留 WARN 以上的日誌」，
//         而門檻是使用者可調的。這是帶狀態 functor 的教科書用途：
//         門檻存在物件裡，同一個類別產生任意多個不同門檻的過濾器。
//   為什麼用到本主題：門檻不是編譯期常數，用函式指標做不到；
//                     而 functor 可以在建構時吃進門檻並被 count_if / copy_if 使用。
// -----------------------------------------------------------------------------
enum class Level { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3 };

struct LogEntry {
    Level       level;
    std::string message;
};

class MinLevelFilter {
    Level threshold;
public:
    explicit MinLevelFilter(Level t) : threshold(t) {}
    bool operator()(const LogEntry& e) const {
        return static_cast<int>(e.level) >= static_cast<int>(threshold);
    }
};

const char* levelName(Level l) {
    switch (l) {
        case Level::DEBUG: return "DEBUG";
        case Level::INFO:  return "INFO";
        case Level::WARN:  return "WARN";
        case Level::ERROR: return "ERROR";
    }
    return "?";
}

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    std::cout << "=== 三種寫法產生同樣的述詞 ===" << std::endl;

    // 使用普通函數, 直接將函數指針傳遞給count_if
    int count1 = static_cast<int>(std::count_if(vec.begin(), vec.end(), is_even_func));
    std::cout << "偶數個數（函數）: " << count1 << std::endl;

    // 使用函數物件, 這裡直接創建一個IsEven物件並傳遞給count_if
    int count2 = static_cast<int>(std::count_if(vec.begin(), vec.end(), IsEven()));
    std::cout << "偶數個數（函數物件）: " << count2 << std::endl;

    // 使用帶狀態的函數物件, 這裡創建一個IsDivisibleBy物件，並傳遞給count_if來計算3的倍數和5的倍數
    int count3 = static_cast<int>(std::count_if(vec.begin(), vec.end(), IsDivisibleBy(3)));
    std::cout << "3的倍數個數: " << count3 << std::endl;

    int count4 = static_cast<int>(std::count_if(vec.begin(), vec.end(), IsDivisibleBy(5)));
    std::cout << "5的倍數個數: " << count4 << std::endl;

    // lambda：其實就是編譯器產生的 functor
    int count5 = static_cast<int>(
        std::count_if(vec.begin(), vec.end(), [](int n) { return n % 2 == 0; }));
    std::cout << "偶數個數（lambda）: " << count5 << std::endl;

    // 空 functor 的大小（實作定義，見檔尾說明）
    std::cout << "\n=== functor 的大小 ===" << std::endl;
    std::cout << "sizeof(IsEven)（無成員）        = " << sizeof(IsEven) << " byte" << std::endl;
    std::cout << "sizeof(IsDivisibleBy)（含 int） = " << sizeof(IsDivisibleBy) << " bytes" << std::endl;
    auto lam = [](int n) { return n % 2 == 0; };
    std::cout << "sizeof(無捕獲 lambda)           = " << sizeof(lam) << " byte" << std::endl;
    int d = 3;
    auto lam2 = [d](int n) { return n % d == 0; };
    std::cout << "sizeof(捕獲一個 int 的 lambda)  = " << sizeof(lam2) << " bytes" << std::endl;

    // 「述詞是按值傳遞」的實際演示
    std::cout << "\n=== 述詞按值傳遞：for_each 要接回傳值 ===" << std::endl;
    struct Counter {
        int seen = 0;
        void operator()(int) { ++seen; }
    };
    Counter c;
    std::for_each(vec.begin(), vec.end(), c);       // 操作的是 c 的副本
    std::cout << "直接看原物件 c.seen  = " << c.seen << "   ← 副本被改，原物件沒動" << std::endl;
    Counter back = std::for_each(vec.begin(), vec.end(), Counter{});
    std::cout << "接 for_each 的回傳值 = " << back.seen << "  ← 這才是正解" << std::endl;

    std::cout << "\n=== LeetCode 179. Largest Number ===" << std::endl;
    std::cout << "[10,2]        → " << largestNumber({10, 2}) << std::endl;
    std::cout << "[3,30,34,5,9] → " << largestNumber({3, 30, 34, 5, 9}) << std::endl;
    std::cout << "[0,0]         → " << largestNumber({0, 0}) << std::endl;

    std::cout << "\n=== 日常實務：可設定門檻的日誌過濾器 ===" << std::endl;
    std::vector<LogEntry> logs = {
        {Level::DEBUG, "cache warm-up finished"},
        {Level::INFO,  "user 1024 logged in"},
        {Level::WARN,  "disk usage 85%"},
        {Level::ERROR, "payment gateway timeout"},
        {Level::INFO,  "order 5566 created"},
        {Level::ERROR, "db connection lost"},
    };

    for (Level th : {Level::DEBUG, Level::WARN, Level::ERROR}) {
        MinLevelFilter filter(th);
        int n = static_cast<int>(std::count_if(logs.begin(), logs.end(), filter));
        std::cout << "門檻 " << levelName(th) << " 以上：" << n << " 筆" << std::endl;
    }

    std::cout << "門檻 WARN 以上的內容：" << std::endl;
    std::vector<LogEntry> alerts;
    std::copy_if(logs.begin(), logs.end(), std::back_inserter(alerts),
                 MinLevelFilter(Level::WARN));
    for (const LogEntry& e : alerts) {
        std::cout << "  [" << levelName(e.level) << "] " << e.message << std::endl;
    }

    return 0;
}

// 注意：sizeof 的結果為「實作定義」。以下數值是本機 g++ 15.2 / libstdc++ / x86-64
//       的實測值：空類別為 1 byte（標準要求完整物件至少 1 byte），
//       含一個 int 的 functor 與捕獲一個 int 的 lambda 皆為 4 bytes。
//       其他平台的對齊規則可能給出不同結果。

// 編譯: g++ -std=c++17 -Wall -Wextra 第三課：STL 的六大組件概覽6.cpp -o demo6

// === 預期輸出 ===
// === 三種寫法產生同樣的述詞 ===
// 偶數個數（函數）: 5
// 偶數個數（函數物件）: 5
// 3的倍數個數: 3
// 5的倍數個數: 2
// 偶數個數（lambda）: 5
//
// === functor 的大小 ===
// sizeof(IsEven)（無成員）        = 1 byte
// sizeof(IsDivisibleBy)（含 int） = 4 bytes
// sizeof(無捕獲 lambda)           = 1 byte
// sizeof(捕獲一個 int 的 lambda)  = 4 bytes
//
// === 述詞按值傳遞：for_each 要接回傳值 ===
// 直接看原物件 c.seen  = 0   ← 副本被改，原物件沒動
// 接 for_each 的回傳值 = 10  ← 這才是正解
//
// === LeetCode 179. Largest Number ===
// [10,2]        → 210
// [3,30,34,5,9] → 9534330
// [0,0]         → 0
//
// === 日常實務：可設定門檻的日誌過濾器 ===
// 門檻 DEBUG 以上：6 筆
// 門檻 WARN 以上：3 筆
// 門檻 ERROR 以上：2 筆
// 門檻 WARN 以上的內容：
//   [WARN] disk usage 85%
//   [ERROR] payment gateway timeout
//   [ERROR] db connection lost
