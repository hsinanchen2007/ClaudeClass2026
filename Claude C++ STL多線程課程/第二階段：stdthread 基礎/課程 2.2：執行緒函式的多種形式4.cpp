// =============================================================================
//  課程 2.2：執行緒函式的多種形式4.cpp  —  函式物件(Functor)與最令人困惑的解析
// =============================================================================
//
// 【主題資訊 Information】
//   語法      : std::thread t(functorObject);   / std::thread t{Task()};
//   標準版本  : C++11(std::thread、統一初始化語法 {})
//   標頭檔    : <thread>
//   Functor   : 任何多載了 operator() 的類別,其實例即可被當成函式呼叫
//   關鍵陷阱  : std::thread t(Task()); 會被解析成「函式宣告」而非物件定義
//               —— 本機 GCC 15.2.0 會以 -Wvexing-parse 警告
//
// 【詳細解釋 Explanation】
//
// 【1. Functor 是什麼、為什麼還需要它】
// Functor(函式物件)就是一個多載了 operator() 的類別。它相對一般函式的
// 唯一優勢是:**它可以帶狀態**。一般函式要記住東西只能靠全域變數或 static,
// 那在多執行緒下正是災難的來源;functor 則把狀態放在自己的成員裡,
// 每個實例各自獨立。
//
// 有人會問:C++11 有 lambda 了,還需要 functor 嗎?
//   * 需要具名、可重用、可被多處建立的任務類型時 → functor 較清楚。
//   * 需要繼承、需要多個成員函式、需要單元測試該類型時 → 只能用 functor。
//   * 一次性的簡短邏輯 → lambda 更好。
// 事實上 lambda 本來就是編譯器幫你產生的匿名 functor,兩者是同一件事的
// 兩種寫法。理解 functor,就理解了 lambda 的本體。
//
// 【2. 最令人困惑的解析(Most Vexing Parse)】
// 這是本檔真正的重點。底下這行看起來像是「建立一個執行緒,執行一個臨時的 Task」:
//
//     std::thread t2(Task());        // ✗ 這不是你以為的意思!
//
// C++ 的文法規定:只要一段宣告「可以」被解析成函式宣告,它「就會」被解析成
// 函式宣告。上面那行的意思其實是:
//     宣告一個名為 t2 的函式,它不接受參數(而是接受「一個回傳 Task 的函式指標」),
//     回傳 std::thread。
// 本機實測 GCC 15.2.0 的錯誤訊息直接證實了這一點:
//     warning: parentheses were disambiguated as a function declaration [-Wvexing-parse]
//     error: request for member 'join' in 't', which is of non-class type
//            'std::thread(Task (*)())'
// 也就是說,t2 根本不是一個 thread 物件,你連 t2.join() 都呼叫不了。
//
// 【3. 三種正確寫法】
//     Task task;
//     std::thread t1(task);          // (a) 傳一個具名物件 —— 沒有歧義
//     std::thread t2((Task()));      // (b) 多包一層括號 —— 破壞函式宣告的文法
//     std::thread t3{Task()};        // (c) 大括號初始化 —— 推薦
// (c) 是最好的:大括號在文法上不可能是函式宣告,所以歧義從根本上消失。
// 這也是「能用 {} 就用 {}」這條現代 C++ 準則最有說服力的理由之一。
//
// 【4. 一個容易忽略的語意:functor 會被「複製」進執行緒】
//     Task task;
//     std::thread t1(task);          // task 被複製了一份
// 執行緒操作的是它自己的副本,對副本的任何修改都不會反映到外面的 task。
// 若你真的想讓執行緒操作「同一個」物件,必須明確表達:
//     std::thread t(std::ref(task)); // 傳參考包裝
// 但那樣就回到共享狀態,需要自己處理資料競爭(見課程 2.3)。
// 本檔的 Task::operator() 宣告為 const 且不碰狀態,所以複製與否沒有差別 ——
// 但這是刻意的設計,不是巧合。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 C++ 會有 Most Vexing Parse 這種文法
//   它源自 C 的宣告語法與 C++ 的建構語法在文法上重疊。
//   標準選擇「有歧義時一律當成宣告」,是為了向下相容 C 的既有程式碼。
//   代價就是這個困擾了所有 C++ 程式員的陷阱。
//   C++11 引入統一初始化 {} 的動機之一,正是提供一條沒有歧義的路。
//
// (B) 為什麼 (Task()) 多一層括號就好了
//   函式宣告的參數列表裡,「一個被括號包住的參數名稱」是合法的
//   (例如 int (x) 等同 int x),但「一個被括號包住的臨時物件運算式」
//   無法被解析成參數宣告。多這一層括號讓「函式宣告」這條路走不通,
//   編譯器只好回頭選擇「物件定義 + 複製初始化」。
//
// (C) operator() 該不該宣告成 const
//   本例宣告為 const,代表這個任務不修改自身狀態。這是好習慣:
//   const 明確告訴讀者「這個 functor 是無狀態或唯讀的」,
//   在多執行緒情境下,這正是「這個物件被複製或共享都無所謂」的訊號。
//   若 operator() 需要修改成員,就不能是 const —— 那時你必須認真思考
//   這個物件是否會被多條執行緒共享。
//
// 【注意事項 Pay Attention】
// 1. 絕對不要寫 std::thread t(Task());,它是函式宣告。用 {} 或多一層括號。
// 2. 開啟編譯器警告很重要:GCC 的 -Wvexing-parse(-Wall 已包含)
//    會直接指出這個問題,否則錯誤訊息會非常難懂。
// 3. functor 是被「複製」進執行緒的;要共享同一個物件必須用 std::ref,
//    但那就要自己處理資料競爭。
// 4. 建立執行緒後一定要 join() 或 detach()。
// 5. 三條執行緒印出同樣的字串時,行的先後順序不確定;
//    本檔用 mutex 保護輸出,避免字元互相穿插。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Functor 與 Most Vexing Parse
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::thread t(Task()); 這行做了什麼?
//     答：它不是建立執行緒,而是宣告了一個名為 t 的「函式」——
//         參數是「回傳 Task 的函式指標」,回傳型別是 std::thread。
//         這就是 Most Vexing Parse:只要一段宣告可以被解析成函式宣告,
//         C++ 就一定會那樣解析。本機 GCC 會警告 -Wvexing-parse,
//         而且後續 t.join() 會直接編譯失敗。
//     追問：怎麼修?
//         → 寫成 std::thread t{Task()};(推薦,大括號不可能是函式宣告),
//           或 std::thread t((Task()));,或先建立具名物件再傳進去。
//
// ⚠️ 陷阱. 「我把 functor 傳給 std::thread,然後在 operator() 裡累加成員變數,
//         等 join 完再讀那個成員,就能拿到執行緒算出來的結果。」哪裡錯了?
//     答：讀到的會是原封不動的初始值。std::thread 會把 functor「複製」
//         一份到執行緒自己的儲存空間,執行緒累加的是那個副本,
//         你手上的原始物件從頭到尾沒被碰過,而且那個副本在執行緒結束後就消失了。
//     為什麼會錯：以為傳物件給 thread 像傳給一般函式的參考那樣共享。
//         實際上 std::thread 的建構子會「複製或移動」所有參數 ——
//         這是刻意的設計,為了避免執行緒存取到已銷毀的物件。
//         要取得結果,正確做法是用 std::ref 共享(並自行同步),
//         或改用 std::promise/std::future 把結果傳回來。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <mutex>
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
// 原始示範:最單純的 functor
// -----------------------------------------------------------------------------
class Task {
public:
    void operator()() const {
        say("  Task executed");
    }
};

// -----------------------------------------------------------------------------
// 示範「functor 被複製」這件事
// -----------------------------------------------------------------------------
class CountingTask {
    int  runs_ = 0;
    int  id_;
public:
    explicit CountingTask(int id) : id_(id) {}

    void operator()() {         // 注意:會改成員,所以不能是 const
        ++runs_;
        say("  [CountingTask " + std::to_string(id_) + "] 副本內的 runs = " +
            std::to_string(runs_));
    }

    int runs() const { return runs_; }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】可設定的批次處理任務(functor 帶狀態的真正價值)
//   情境: 資料匯入工具要平行處理多個檔案,每條執行緒負責一個檔案,
//         而且各自有不同的設定(批次大小、是否跳過錯誤行)。
//         用一般函式就得把這些設定塞成一長串參數或全域變數;
//         functor 讓「設定」自然地成為物件的成員,建立時一次配置好。
//   為什麼用本主題: 這正是 functor 相對一般函式的核心優勢 —— 攜帶狀態,
//                   而且每個實例的狀態彼此獨立,天生適合多執行緒。
// -----------------------------------------------------------------------------
class ImportJob {
    std::string file_;
    int         batchSize_;
    bool        skipBadRows_;

public:
    ImportJob(std::string file, int batchSize, bool skipBadRows)
        : file_(std::move(file)), batchSize_(batchSize), skipBadRows_(skipBadRows) {}

    void operator()() const {
        // 模擬:檔名長度當作總行數,其中每 4 行有 1 行是壞的
        int totalRows = static_cast<int>(file_.size()) * 3;
        int badRows   = totalRows / 4;
        int imported  = skipBadRows_ ? (totalRows - badRows) : totalRows;
        int batches   = (imported + batchSize_ - 1) / batchSize_;

        say("    [匯入] " + file_ + " 共 " + std::to_string(totalRows) +
            " 行,匯入 " + std::to_string(imported) + " 行,分 " +
            std::to_string(batches) + " 批(batchSize=" +
            std::to_string(batchSize_) +
            (skipBadRows_ ? ", 略過壞行)" : ", 不略過壞行)"));
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔從缺,理由如下
//   本檔聚焦在「functor 的建立語法與 Most Vexing Parse」,是 C++ 文法主題,
//   LeetCode 不會考文法陷阱。不過本課的總整理檔(第 6 個範例檔)會示範
//   LeetCode 1115. Print FooBar Alternately —— 那題的類別要讓兩條執行緒
//   分別執行它的兩個「成員函式」,正好對應本課「可呼叫物件的多種形式」,
//   是真正用得上的地方。此處硬湊不如從缺。
// -----------------------------------------------------------------------------

int main() {
    std::cout << "=== 原始示範:三種建立 functor 執行緒的正確寫法 ===" << std::endl;
    {
        Task task;

        // 方式一:傳入具名物件(沒有歧義)
        std::thread t1(task);

        // 方式二:傳入臨時物件,多包一層括號破壞函式宣告的文法
        std::thread t2((Task()));

        // 方式三:大括號初始化(推薦,文法上不可能是函式宣告)
        std::thread t3{Task()};

        t1.join();
        t2.join();
        t3.join();
        std::cout << "  ↑ 三種寫法效果相同,都真的建立了執行緒" << std::endl;
    }

    std::cout << "\n=== Most Vexing Parse:錯誤寫法長什麼樣 ===" << std::endl;
    std::cout << "  std::thread t(Task());   // ✗ 這是函式宣告,不是物件!" << std::endl;
    std::cout << "  本機 GCC 15.2.0 實測會回報:" << std::endl;
    std::cout << "    warning: parentheses were disambiguated as a function"
                 " declaration [-Wvexing-parse]" << std::endl;
    std::cout << "    error: request for member 'join' in 't', which is of"
                 " non-class type 'std::thread(Task (*)())'" << std::endl;
    std::cout << "  (該行未寫進本檔,否則整支程式無法編譯)" << std::endl;

    std::cout << "\n=== functor 是被「複製」進執行緒的 ===" << std::endl;
    {
        CountingTask original(1);
        std::thread t(original);      // original 被複製一份
        t.join();
        std::cout << "  執行緒結束後,原始物件的 runs = " << original.runs()
                  << " (仍是 0 —— 被累加的是副本,不是它)" << std::endl;

        // 想共享同一個物件,必須明確用 std::ref
        CountingTask shared(2);
        std::thread t2(std::ref(shared));
        t2.join();
        std::cout << "  改用 std::ref 之後,原始物件的 runs = " << shared.runs()
                  << " (這次真的被改到了)" << std::endl;
    }

    std::cout << "\n=== 實務:平行匯入,每條執行緒帶自己的設定 ===" << std::endl;
    {
        std::vector<std::thread> pool;
        pool.emplace_back(ImportJob("users.csv",  100, true));
        pool.emplace_back(ImportJob("orders.csv",  50, false));
        pool.emplace_back(ImportJob("logs.csv",   200, true));
        for (std::thread& t : pool) t.join();
        std::cout << "  ↑ 三個任務各自帶著不同設定,狀態互不干擾" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.2：執行緒函式的多種形式4.cpp" -o functor_thread

// 注意:以下為某一次實際執行的結果。
//   * 同一組內各執行緒印出的先後順序每次執行都可能不同
//     (三行 "Task executed"、三行「匯入」都是如此)。
//   * 但每一行都是完整的(有 mutex 保護),而且各段的內容固定 ——
//     尤其「原始物件的 runs = 0」與「用 std::ref 後 runs = 1」
//     這兩個對照結果每次都一樣,那是語意保證,不是巧合。

// === 預期輸出 ===
// === 原始示範:三種建立 functor 執行緒的正確寫法 ===
//   Task executed
//   Task executed
//   Task executed
//   ↑ 三種寫法效果相同,都真的建立了執行緒
//
// === Most Vexing Parse:錯誤寫法長什麼樣 ===
//   std::thread t(Task());   // ✗ 這是函式宣告,不是物件!
//   本機 GCC 15.2.0 實測會回報:
//     warning: parentheses were disambiguated as a function declaration [-Wvexing-parse]
//     error: request for member 'join' in 't', which is of non-class type 'std::thread(Task (*)())'
//   (該行未寫進本檔,否則整支程式無法編譯)
//
// === functor 是被「複製」進執行緒的 ===
//   [CountingTask 1] 副本內的 runs = 1
//   執行緒結束後,原始物件的 runs = 0 (仍是 0 —— 被累加的是副本,不是它)
//   [CountingTask 2] 副本內的 runs = 1
//   改用 std::ref 之後,原始物件的 runs = 1 (這次真的被改到了)
//
// === 實務:平行匯入,每條執行緒帶自己的設定 ===
//     [匯入] users.csv 共 27 行,匯入 21 行,分 1 批(batchSize=100, 略過壞行)
//     [匯入] logs.csv 共 24 行,匯入 18 行,分 1 批(batchSize=200, 略過壞行)
//     [匯入] orders.csv 共 30 行,匯入 30 行,分 1 批(batchSize=50, 不略過壞行)
//   ↑ 三個任務各自帶著不同設定,狀態互不干擾
