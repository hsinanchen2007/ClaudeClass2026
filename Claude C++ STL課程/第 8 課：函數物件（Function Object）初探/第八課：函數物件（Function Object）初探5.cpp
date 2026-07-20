// =============================================================================
//  第八課：函數物件 5  —  述詞是「以值傳遞」的：for_each 的回傳值為何存在
// =============================================================================
//
// 【主題資訊 Information】
//   template<class InputIt, class UnaryFunction>
//   UnaryFunction for_each(InputIt first, InputIt last, UnaryFunction f);
//                                                       ^^^^^^^^^^^^^ 值傳遞
//                 ^^^^^^^^^^^^^ 回傳「被演算法用過的那一份」
//   標頭檔：<algorithm>
//   標準版本：C++98；C++11 起 f 是以 std::move 回傳；
//             C++17 加了 for_each_n 與平行版（平行版**不回傳** f）。
//   複雜度：對每個元素恰好呼叫一次 f，O(n)。
//   ★ for_each 是少數**保證呼叫順序**（由前往後）的演算法，
//     這讓它成為唯一適合放「有副作用的可呼叫物件」的地方。
//
// 【詳細解釋 Explanation】
//
// 【1. 核心事實：你傳進去的述詞，演算法拿到的是一份「副本」】
//   看清楚 for_each 的簽章——UnaryFunction f 是**值參數**。
//   所以：
//       Counter counter(2);
//       std::for_each(vec.begin(), vec.end(), counter);   // 傳進去的是複本
//       counter.get_count();       // 永遠是 0！原件從頭到尾沒被碰過
//   演算法對副本累加，函式結束時副本就消失了。
//   這是初學者最容易踩的坑之一，而且它**不會報錯**，
//   只是安靜地給你一個 0。
//
// 【2. 所以 for_each 才需要回傳值】
//   標準委員會當然知道這個問題，解法是：
//   **把演算法用過的那份副本回傳出來**。
//       Counter result = std::for_each(vec.begin(), vec.end(), counter);
//       result.get_count();        // 4，這才是累積後的結果
//   for_each 的回傳型別是 UnaryFunction 而不是 void，唯一的理由就是這個。
//   看到這個回傳值，就該想到「述詞是值傳遞的」這件事。
//   C++11 起標準改成以 std::move 回傳，所以帶大狀態的 functor 也不會多一次複製。
//
// 【3. 為什麼標準要用值傳遞，而不是傳參考】
//   幾個理由疊在一起：
//     * **泛型一致性**：述詞可能是臨時物件（GreaterThan(5)、lambda 字面值），
//       值傳遞才能統一接受左值與右值。
//     * **可自由複製**：多數演算法內部會複製述詞（例如遞迴時各分支一份），
//       這對無狀態述詞完全沒有成本。
//     * **鼓勵無副作用**：值傳遞讓「述詞不該有副作用」這個約定變得自然——
//       你想有副作用也傳不回來。
//   代價就是本檔的陷阱。標準用「回傳 f」補上這個缺口，
//   但**只有 for_each 這麼做**（因為只有它保證呼叫順序）。
//
// 【4. 現代寫法：三種取代方案】
//   (a) 用 for_each 的回傳值 —— 本檔原始寫法，可行但少見。
//   (b) **lambda 以參考捕獲外部變數** —— 最常見的現代寫法：
//           int count = 0;
//           std::for_each(v.begin(), v.end(), [&count](int x){ if (x==2) ++count; });
//       狀態在 lambda 外面，複製 closure 不影響它。
//   (c) **直接用對的演算法** —— 這個例子其實根本不該用 for_each：
//           auto count = std::count(v.begin(), v.end(), 2);
//       需求是「數幾個」就用 count，不要自己拿 for_each 造輪子。
//   實務優先序：(c) > (b) > (a)。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 其他演算法沒有「回傳述詞」這個補救
//     std::count_if、std::find_if、std::remove_if、std::sort…
//     全都不回傳述詞。所以在那些演算法上放有狀態的述詞，
//     狀態**完全拿不回來**，而且更糟——
//     它們也**不保證呼叫次數與順序**，
//     所以累積出來的數字即使拿得到也沒有意義。
//     結論：除了 for_each，述詞一律該是純函數。
//
// (B) C++17 的平行版 for_each 連回傳值都拿掉了
//     std::for_each(std::execution::par, first, last, f);   // 回傳 void
//     因為平行執行時 f 會被複製到多個執行緒，
//     「哪一份才是最終結果」根本沒有定義。
//     這反過來印證了：靠述詞累積狀態是個脆弱的設計。
//     平行版要累積結果請用 std::reduce / std::transform_reduce。
//
// (C) 為什麼 operator() 這裡沒有標 const
//     Counter::operator() 要修改 count_，所以不能標 const。
//     這是「述詞有副作用」的訊號——看到沒有 const 的 operator()，
//     就該警覺它可能踩到本檔的陷阱。
//     反過來說，第 4 檔的 GreaterThan 標了 const，
//     那種述詞複製幾份都無所謂。
//
// (D) 累積狀態的正統做法是 std::accumulate
//     for_each + 副作用 是「命令式」寫法；
//     accumulate（C++17 起還有 reduce / transform_reduce）是「函數式」寫法，
//     把狀態明確地當成參數傳進傳出，沒有隱藏的副作用，
//     也因此天然適合平行化。
//
// 【注意事項 Pay Attention】
//   1. 述詞是**值傳遞**：原件不會被修改，別去讀原件的狀態。
//   2. 只有 for_each 回傳述詞；其他演算法的述詞狀態拿不回來。
//   3. 除 for_each 外，標準**不保證**述詞的呼叫次數與順序。
//   4. C++17 平行版 for_each 不回傳述詞，也不保證順序。
//   5. 需求是「統計」時優先找現成演算法（count / count_if / accumulate），
//      不要用 for_each + 副作用自己造。
//   6. 用 lambda 參考捕獲雖然可行，但要確保被捕獲的變數活得夠久。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】述詞的值傳遞語意
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 std::for_each 要回傳它的第三個參數？其他演算法都不這麼做。
//     答：因為述詞是**以值傳遞**的，演算法操作的是副本，
//         呼叫端手上的原件從頭到尾沒被碰過。
//         如果述詞有累積狀態（例如計數器），結果會留在副本裡然後消失。
//         for_each 把「用過的那份副本」回傳出來，就是唯一的補救管道。
//         而且 for_each 是少數**保證呼叫順序**的演算法，
//         所以在它身上放有副作用的述詞才有意義。
//     追問：那 C++17 的平行版 for_each 呢？
//         → 回傳 void。因為平行執行時述詞會被複製到多個執行緒，
//           「哪一份是最終結果」沒有定義。這反過來說明
//           靠述詞累積狀態本來就是脆弱的設計。
//
// 🔥 Q2. 這段程式印出什麼？為什麼？
//        Counter c(2);
//        std::for_each(v.begin(), v.end(), c);
//        std::cout << c.get_count();
//     答：印出 0。for_each 收到的是 c 的副本，累加發生在副本上，
//         函式返回時副本就銷毀了，c 本身從未被修改。
//         這個 bug 不會有任何警告，只是安靜地給你 0。
//         要拿到結果必須接住回傳值：
//             Counter r = std::for_each(v.begin(), v.end(), c);
//     追問：現代 C++ 會怎麼寫這段？
//         → 這個需求根本不該用 for_each，直接
//           auto n = std::count(v.begin(), v.end(), 2); 就好。
//           若真的需要自訂累積邏輯，用 lambda 參考捕獲外部變數
//           （狀態在 closure 外面，複製 closure 不影響它）。
//
// ⚠️ 陷阱. 「那我把 Counter 傳給 std::count_if，
//         一樣接住回傳值不就好了？」
//     答：接不到——count_if 回傳的是**符合條件的個數**，不是述詞。
//         除了 for_each 之外，沒有任何演算法會把述詞還給你。
//         更根本的問題是：count_if **不保證**述詞被呼叫幾次、
//         以什麼順序呼叫，也可以任意複製它。
//         就算你用參考捕獲硬是把數字取出來，那個數字也依賴實作細節，
//         換一版標準函式庫就可能不同。
//         對 std::sort 這類演算法更嚴重：有副作用的比較器可能讓
//         「同一對元素比較兩次得到不同結果」，直接違反嚴格弱序合約，
//         那是未定義行為。
//     為什麼會錯：把「for_each 回傳述詞」理解成一個通用的取回機制，
//         而不是一個**針對 for_each 特例**的補救。
//         正確的心智模型是：述詞應該是純函數；
//         for_each 的回傳值是標準留給遺留程式碼的一扇小門，不是主要用法。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <numeric>

class Counter {
private:
    int count_;   // 狀態：計數器
    int target_;  // 狀態：目標值

public:
    // 建構子：初始化狀態
    Counter(int target) : count_(0), target_(target) {}

    // operator()：讓物件可以像函數一樣被呼叫
    // 注意這裡**沒有** const —— 因為要修改 count_。
    // 沒有 const 的 operator() 就是「這個述詞有副作用」的訊號。
    bool operator()(int x) {
        if (x == target_) {
            ++count_;
            return true;
        }
        return false;
    }

    // 取得累積的狀態
    int get_count() const { return count_; }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔刻意不放
//   理由：本檔講的是「述詞以值傳遞」這個**標準函式庫的呼叫慣例**，
//   屬於 API 語意層面的知識，不是演算法技巧。
//   LeetCode 的評分只看回傳值對不對，不會因為你用 for_each 還是
//   count 而有差別；硬掛一題只會變成「用一個有陷阱的寫法解一題簡單的題」，
//   反而在示範壞習慣。
//   本檔真正該傳達的是「什麼時候不要用 for_each」，
//   這個判斷力用下面的實務範例（log 統計）呈現更貼切。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】統計一批 log 各等級的筆數 —— 三種寫法的優劣
//   情境：每天要對服務的 log 出一份摘要：INFO / WARN / ERROR 各幾筆、
//         錯誤率多少。這是最典型的「掃一遍並累積統計」需求。
//   為什麼用到本主題：這正是最容易誤用 for_each + 有狀態 functor 的場景。
//     下面把三種寫法並列，可以直接看到
//     「錯的寫法安靜地給你 0」以及「對的寫法長什麼樣」。
// -----------------------------------------------------------------------------
struct LogStats {
    int info = 0, warn = 0, error = 0;

    // 沒有 const：這是有副作用的述詞
    void operator()(const std::string& line) {
        if (line.find("[ERROR]") != std::string::npos)      ++error;
        else if (line.find("[WARN]") != std::string::npos)  ++warn;
        else if (line.find("[INFO]") != std::string::npos)  ++info;
    }

    int total() const { return info + warn + error; }
    double errorRate() const {
        return total() == 0 ? 0.0 : static_cast<double>(error) / total();
    }
};

int main() {
    std::vector<int> vec = {1, 2, 3, 2, 4, 2, 5, 2, 6};

    // 建立函數物件
    Counter counter(2);

    // 傳給演算法（注意：for_each 會回傳函數物件的副本）
    Counter result = std::for_each(vec.begin(), vec.end(), counter);

    std::cout << "找到 2 的次數: " << result.get_count() << std::endl;

    // -------------------------------------------------------------------------
    std::cout << "\n=== 關鍵對照：原件 vs 回傳的副本 ===" << std::endl;
    {
        std::cout << "counter.get_count()（原件）= " << counter.get_count()
                  << "  ← 永遠是 0，原件從頭到尾沒被碰過" << std::endl;
        std::cout << "result.get_count() （副本）= " << result.get_count()
                  << "  ← 這才是累積後的結果" << std::endl;
        std::cout << "→ for_each 的第三個參數是**值參數**，" << std::endl;
        std::cout << "  演算法操作的是副本；回傳值是唯一的取回管道。" << std::endl;
        std::cout << "  這個 bug 不會有任何警告，只會安靜地給你 0。" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 三種正確寫法（實務優先序由高到低）===" << std::endl;
    {
        // (c) 最好：直接用對的演算法
        auto byCount = std::count(vec.begin(), vec.end(), 2);
        std::cout << "(c) std::count            = " << byCount
                  << "  ← 需求是「數幾個」就用 count，最清楚" << std::endl;

        // (b) 次好：lambda 參考捕獲，狀態放在 closure 外面
        int n = 0;
        std::for_each(vec.begin(), vec.end(), [&n](int x) { if (x == 2) ++n; });
        std::cout << "(b) lambda 參考捕獲        = " << n
                  << "  ← 狀態在外面，複製 closure 不影響它" << std::endl;

        // (a) 可行但少見：接住 for_each 的回傳值
        Counter r = std::for_each(vec.begin(), vec.end(), Counter(2));
        std::cout << "(a) 接住 for_each 回傳值   = " << r.get_count()
                  << "  ← 可行，但現代程式碼很少這樣寫" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 其他演算法連這個補救都沒有 ===" << std::endl;
    {
        Counter c(2);
        auto k = std::count_if(vec.begin(), vec.end(), c);
        std::cout << "count_if 回傳的是「個數」= " << k
                  << "，不是述詞" << std::endl;
        std::cout << "c.get_count()（原件）    = " << c.get_count()
                  << "  ← 一樣是 0，而且沒有任何管道取回副本" << std::endl;
        std::cout << "→ 除 for_each 外，標準也**不保證**述詞的呼叫次數與順序，"
                  << std::endl;
        std::cout << "  所以就算硬取到數字也沒有意義。" << std::endl;
        std::cout << "  結論：除了 for_each，述詞一律該是純函數。" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務：統計 log 各等級筆數 ===" << std::endl;
    {
        const std::vector<std::string> logs = {
            "10:00:01 [INFO]  service started",
            "10:00:07 [ERROR] db connect failed",
            "10:00:09 [WARN]  retrying",
            "10:00:15 [INFO]  connected",
            "10:00:31 [ERROR] timeout on /api/user",
            "10:00:44 [INFO]  request ok",
            "10:01:02 [WARN]  slow query 1.4s",
            "10:01:19 [INFO]  request ok",
        };

        // ✗ 錯誤寫法：讀原件
        LogStats wrong;
        std::for_each(logs.begin(), logs.end(), wrong);
        std::cout << "✗ 讀原件            : INFO=" << wrong.info
                  << " WARN=" << wrong.warn
                  << " ERROR=" << wrong.error
                  << "  ← 全是 0，安靜地錯" << std::endl;

        // ✓ 寫法 1：接住 for_each 的回傳值
        LogStats got = std::for_each(logs.begin(), logs.end(), LogStats{});
        std::cout << "✓ 接住回傳值        : INFO=" << got.info
                  << " WARN=" << got.warn
                  << " ERROR=" << got.error << std::endl;

        // ✓ 寫法 2：lambda 參考捕獲（最常見的現代寫法）
        std::map<std::string, int> byLevel;
        std::for_each(logs.begin(), logs.end(), [&byLevel](const std::string& line) {
            for (const char* lv : {"[ERROR]", "[WARN]", "[INFO]"}) {
                if (line.find(lv) != std::string::npos) { ++byLevel[lv]; break; }
            }
        });
        std::cout << "✓ lambda 參考捕獲   : ";
        for (const auto& kv : byLevel) std::cout << kv.first << "=" << kv.second << " ";
        std::cout << std::endl;

        // ✓ 寫法 3：用現成演算法，完全沒有副作用
        auto errors = std::count_if(logs.begin(), logs.end(),
            [](const std::string& l) { return l.find("[ERROR]") != std::string::npos; });
        std::cout << "✓ count_if（純函數）: ERROR=" << errors << std::endl;

        std::cout << "錯誤率: " << got.errorRate() * 100 << "%（" << got.error
                  << "/" << got.total() << "）" << std::endl;
        std::cout << "→ 統計需求優先找現成演算法；真要自訂就用參考捕獲，" << std::endl;
        std::cout << "  別依賴「述詞內部累積的狀態」這件事。" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第八課：函數物件（Function Object）初探5.cpp -o demo5

// === 預期輸出 ===
// 找到 2 的次數: 4
//
// === 關鍵對照：原件 vs 回傳的副本 ===
// counter.get_count()（原件）= 0  ← 永遠是 0，原件從頭到尾沒被碰過
// result.get_count() （副本）= 4  ← 這才是累積後的結果
// → for_each 的第三個參數是**值參數**，
//   演算法操作的是副本；回傳值是唯一的取回管道。
//   這個 bug 不會有任何警告，只會安靜地給你 0。
//
// === 三種正確寫法（實務優先序由高到低）===
// (c) std::count            = 4  ← 需求是「數幾個」就用 count，最清楚
// (b) lambda 參考捕獲        = 4  ← 狀態在外面，複製 closure 不影響它
// (a) 接住 for_each 回傳值   = 4  ← 可行，但現代程式碼很少這樣寫
//
// === 其他演算法連這個補救都沒有 ===
// count_if 回傳的是「個數」= 4，不是述詞
// c.get_count()（原件）    = 0  ← 一樣是 0，而且沒有任何管道取回副本
// → 除 for_each 外，標準也**不保證**述詞的呼叫次數與順序，
//   所以就算硬取到數字也沒有意義。
//   結論：除了 for_each，述詞一律該是純函數。
//
// === 日常實務：統計 log 各等級筆數 ===
// ✗ 讀原件            : INFO=0 WARN=0 ERROR=0  ← 全是 0，安靜地錯
// ✓ 接住回傳值        : INFO=4 WARN=2 ERROR=2
// ✓ lambda 參考捕獲   : [ERROR]=2 [INFO]=4 [WARN]=2
// ✓ count_if（純函數）: ERROR=2
// 錯誤率: 25%（2/8）
// → 統計需求優先找現成演算法；真要自訂就用參考捕獲，
//   別依賴「述詞內部累積的狀態」這件事。
