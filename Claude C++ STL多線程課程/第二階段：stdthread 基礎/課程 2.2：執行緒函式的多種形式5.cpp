// =============================================================================
//  課程 2.2：執行緒函式的多種形式5.cpp  —  帶狀態的 Functor:用建構子配置任務
// =============================================================================
//
// 【主題資訊 Information】
//   語法      : std::thread t{Counter(5)};      // 大括號初始化,避免 vexing parse
//   標準版本  : C++11
//   標頭檔    : <thread>
//   核心概念  : functor 的「建構子參數」= 任務的組態;
//               operator() 的參數 = 任務執行時才知道的資料
//   複製語意  : std::thread 會把 functor 複製(或移動)一份到執行緒自己的空間
//
// 【詳細解釋 Explanation】
//
// 【1. 兩種傳參數的管道,不要搞混】
// 用 functor 當執行緒進入點時,有兩個地方可以傳資料進去,語意完全不同:
//
//   (a) 建構子參數 —— Counter(5)
//       這是「任務的組態」,在建立任務物件時就決定,之後不再改變。
//       例如:要數到幾、批次大小、輸出格式、重試次數。
//
//   (b) operator() 的參數 —— std::thread t(functor, arg1, arg2)
//       這是「執行時才提供的資料」,由 std::thread 轉發給 operator()。
//
// 本檔示範的是 (a)。它的好處是:任務物件一旦建好就是自足的,
// 可以放進容器、排入佇列、延後執行,不必再攜帶一堆散落的參數。
// 這正是執行緒池與任務佇列的基礎設計。
//
// 【2. 為什麼用 {} 而不是 ()】
//     std::thread t{Counter(5)};    // ✅
//     std::thread t(Counter(5));    // ✅ 這行其實也可以
//     std::thread t(Counter());     // ✗ 這行會被當成函式宣告!
// 差別在於:當臨時物件「有參數」時(如 Counter(5)),它不可能被解析成
// 參數宣告,所以沒有歧義;但「無參數」的 Counter() 就會觸發
// Most Vexing Parse(見本課第 4 個範例檔)。
// 既然 {} 在兩種情況下都正確,養成一律用 {} 的習慣就不必每次判斷。
//
// 【3. const operator() 的意義】
// 本例的 operator() 宣告為 const,代表「執行這個任務不會改變任務本身」。
// 這在多執行緒下是很有價值的宣告:它告訴讀者這個物件被複製幾份、
// 被幾條執行緒同時持有都無所謂,因為沒有人會寫入它。
// 反之,若 operator() 需要修改成員,你就必須回答一個問題:
// 「這個物件會被多條執行緒共享嗎?」——若會,就需要同步。
//
// 【4. functor 與 lambda 的分工】
// 這個 Counter 完全可以寫成 lambda:
//     int n = 5;
//     std::thread t([n]{ for (int i = 0; i < n; ++i) std::cout << i << " "; });
// 那什麼時候該用 functor?判準是「這個任務需不需要一個名字」:
//   * 只用一次、邏輯三五行 → lambda。
//   * 會在多處建立、需要被單元測試、需要繼承或多個成員函式 → functor。
// 換句話說,functor 是「任務」升格成「型別」的時候。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 建構子參數存在哪裡
//   Counter(5) 建立的臨時物件,其成員 count 存著 5。
//   std::thread 建構時會把整個物件複製(此處是移動)到執行緒的內部儲存,
//   所以臨時物件在敘述結束時被銷毀完全沒有影響 —— 執行緒手上有自己的一份。
//   這和 lambda 的值捕獲在機制上是同一件事。
//
// (B) 為什麼複製而不是參考
//   std::thread 的設計原則是「不要讓執行緒指向可能消失的東西」。
//   臨時物件在建立執行緒的那一行結束後就沒了,若 thread 只存參考,
//   執行緒一開始跑就是懸空。統一複製是最安全的預設。
//   真的要共享時必須明確寫 std::ref —— 讓危險的選擇需要多打幾個字,
//   是很好的 API 設計。
//
// (C) 成員初始化順序的小提醒
//   若 functor 有多個成員,它們的初始化順序由「宣告順序」決定,
//   而不是初始化列表裡寫的順序。順序寫反時 GCC 會以 -Wreorder 警告
//   (-Wall 已包含)。這在成員之間有依賴時會出真正的 bug。
//
// 【注意事項 Pay Attention】
// 1. 無參數的臨時 functor 一定要用 {} 或多一層 (),否則是函式宣告。
// 2. functor 被複製進執行緒;想共享同一物件要用 std::ref,並自行同步。
// 3. operator() 不改狀態就宣告成 const,這是有意義的文件。
// 4. 建立執行緒後一定要 join() 或 detach()。
// 5. 多個成員的初始化順序看宣告順序,不是初始化列表的書寫順序。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】帶狀態的 Functor
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 用 functor 當執行緒進入點時,建構子參數和 operator() 的參數
//        分別該放什麼?
//     答：建構子參數放「任務的組態」—— 建立時就決定、之後不變的東西,
//         例如批次大小、重試次數、輸出格式。operator() 的參數放
//         「執行時才提供的資料」,由 std::thread 轉發過去。
//         前者讓任務物件變得自足,可以放進佇列延後執行,
//         這正是執行緒池的基礎。
//     追問：為什麼 std::thread t{Counter(5)}; 要用大括號?
//         → 有參數時其實 () 也可以;但無參數的 Counter() 會觸發
//           Most Vexing Parse 被當成函式宣告。一律用 {} 就不必每次判斷。
//
// ⚠️ 陷阱. 「Counter(5) 是個臨時物件,建立執行緒那行結束後它就被銷毀了,
//         那執行緒不就在用一個已經死掉的物件嗎?」
//     答：不會。std::thread 的建構子會把整個 functor「複製或移動」
//         到執行緒自己的內部儲存空間,執行緒用的是那一份,
//         和原本的臨時物件無關。臨時物件被銷毀完全不影響它。
//     為什麼會錯：把 std::thread 想成「持有一個指向 functor 的參考」。
//         實際上它是「擁有」一份自己的副本 —— 這正是標準刻意的設計,
//         為的就是避免執行緒指向已消失的物件。也因為如此,
//         想共享同一個物件反而必須額外寫 std::ref 才行。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <mutex>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

// 保護 std::cout,避免多執行緒輸出互相切開
std::mutex g_ioMutex;

void say(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_ioMutex);
    std::cout << msg << std::endl;
}

// -----------------------------------------------------------------------------
// 原始示範:建構子帶入「要數到幾」
// -----------------------------------------------------------------------------
class Counter {
    int count;
public:
    Counter(int c) : count(c) {}

    void operator()() const {          // const:執行任務不改變任務本身
        // 用「分隔符放在前面」的寫法,避免行尾多出一個空白 ——
        // 行尾空白在比對輸出時是常見的干擾來源。
        std::string line;
        for (int i = 0; i < count; ++i) {
            if (i) line += ' ';
            line += std::to_string(i);
        }
        say("  " + line);
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】把陣列切段平行加總,每條執行緒帶著自己的區間
//   情境: 這是平行運算最基本的骨架 —— 把一個大工作切成互不重疊的區間,
//         每條執行緒處理一段,最後合併。每段的「起點、終點、要寫到哪裡」
//         正是典型的「任務組態」,由建構子帶入最自然。
//   為什麼用本主題: 完美對應【詳細解釋 1】(a):建構子參數 = 任務組態。
//         同時示範一個關鍵的執行緒安全設計 ——
//         每條執行緒只寫 results[myIndex],沒有兩條寫同一格,
//         因此不需要任何 mutex 就是安全的(切割寫入,而非共享寫入)。
// -----------------------------------------------------------------------------
class RangeSum {
    const std::vector<long>* data_;   // 唯讀共享:多條執行緒同時「讀」是安全的
    std::size_t begin_;
    std::size_t end_;
    long*       out_;                 // 每條執行緒各自的輸出格,互不重疊

public:
    RangeSum(const std::vector<long>& data, std::size_t b, std::size_t e, long* out)
        : data_(&data), begin_(b), end_(e), out_(out) {}

    void operator()() const {
        long sum = 0;
        for (std::size_t i = begin_; i < end_; ++i) sum += (*data_)[i];
        *out_ = sum;
        say("    [區間 " + std::to_string(begin_) + "~" + std::to_string(end_) +
            ") 小計 = " + std::to_string(sum));
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔從缺,理由如下
//   本檔的主題是「functor 如何用建構子攜帶組態」,屬於 C++ 的物件設計問題。
//   LeetCode 的並行題(1114 Print in Order、1115 Print FooBar Alternately、
//   1116 Print Zero Even Odd、1117 Building H2O、1195 Fizz Buzz Multithreaded)
//   的類別骨架是題目給定的,不能自己決定建構子要帶什麼組態,
//   考點也全在同步原語上。本課第 6 個範例檔會示範真正對應得上的
//   1115 Print FooBar Alternately(兩條執行緒各執行一個成員函式),
//   此處從缺以免失焦。
// -----------------------------------------------------------------------------

int main() {
    std::cout << "=== 原始示範:Counter(5) ===" << std::endl;
    {
        std::thread t{Counter(5)};
        t.join();
    }

    std::cout << "\n=== 同一個 functor 型別,不同組態的三個實例 ===" << std::endl;
    {
        std::vector<std::thread> pool;
        pool.emplace_back(Counter(3));
        pool.emplace_back(Counter(6));
        pool.emplace_back(Counter(9));
        for (std::thread& t : pool) t.join();
        std::cout << "  ↑ 三條執行緒各自帶著自己的 count,狀態完全獨立"
                  << std::endl;
    }

    std::cout << "\n=== 實務:切段平行加總 ===" << std::endl;
    {
        // 準備資料 1..1000
        std::vector<long> data(1000);
        std::iota(data.begin(), data.end(), 1L);

        const std::size_t threadCount = 4;
        const std::size_t chunk = data.size() / threadCount;
        std::vector<long> partial(threadCount, 0);
        std::vector<std::thread> pool;

        for (std::size_t i = 0; i < threadCount; ++i) {
            std::size_t b = i * chunk;
            std::size_t e = (i + 1 == threadCount) ? data.size() : b + chunk;
            pool.emplace_back(RangeSum(data, b, e, &partial[i]));
        }
        for (std::thread& t : pool) t.join();

        long total = 0;
        for (long p : partial) total += p;
        std::cout << "  合計 = " << total << "  (1+2+...+1000 = 500500,"
                  << (total == 500500 ? " 正確" : " 錯誤") << ")" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.2：執行緒函式的多種形式5.cpp" -o counter_functor

// 注意:以下為某一次實際執行的結果。
//   * 第二、三段各執行緒印出的先後順序每次執行都可能不同
//     (例如「區間 500~750」可能排在「區間 0~250」之前)。
//   * 但每一行都完整(有 mutex 保護),而且最後的合計永遠是 500500 ——
//     因為那是在全部 join 之後才計算的,而且各執行緒寫的是互不重疊的格子。

// === 預期輸出 ===
// === 原始示範:Counter(5) ===
//   0 1 2 3 4
//
// === 同一個 functor 型別,不同組態的三個實例 ===
//   0 1 2
//   0 1 2 3 4 5
//   0 1 2 3 4 5 6 7 8
//   ↑ 三條執行緒各自帶著自己的 count,狀態完全獨立
//
// === 實務:切段平行加總 ===
//     [區間 0~250) 小計 = 31375
//     [區間 500~750) 小計 = 156375
//     [區間 750~1000) 小計 = 218875
//     [區間 250~500) 小計 = 93875
//   合計 = 500500  (1+2+...+1000 = 500500, 正確)
