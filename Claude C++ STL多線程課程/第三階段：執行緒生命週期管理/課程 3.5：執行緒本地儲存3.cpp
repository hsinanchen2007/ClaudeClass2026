// =============================================================================
//  課程 3.5：執行緒本地儲存 3  —  thread_local 的三種宣告位置
// =============================================================================
//
// 【主題資訊 Information】
//   thread_local 可以出現在三個位置:
//
//     (1) 命名空間範圍(全域)
//         thread_local int global_tl = 0;
//
//     (2) 函式內的區域變數
//         void f() { thread_local static int local_tl = 0; }
//         // static 是多餘的:thread_local 已隱含靜態儲存期
//
//     (3) 類別的【靜態】資料成員
//         class C { thread_local static int member_tl; };
//         thread_local int C::member_tl = 0;    // 類別外定義(C++11/14/17 必要)
//
//   標準版本：C++11
//   標頭檔  ：不需要(語言關鍵字)
//   不可用於：非靜態資料成員、函式參數
//
// 【詳細解釋 Explanation】
//
// 【1. 三種位置的共通點與差異】
//   共通點:三者都是【每條執行緒一份獨立實體】,生命週期涵蓋整條執行緒。
//   差異只在【可見範圍】與【初始化時機】:
//
//     位置            可見範圍              初始化時機
//     ─────────────  ────────────────────  ──────────────────────────
//     (1) 命名空間    整個翻譯單元/程式      執行緒啟動時或首次使用前
//     (2) 函式內      只有該函式            首次執行到該宣告時(lazy)
//     (3) 類別靜態    依存取權限(public…)  同 (1)
//
//   實務選擇原則:能縮小可見範圍就縮小。函式內的 thread_local 最不容易
//   被誤用,因為外面根本看不到它;全域的則方便跨模組共用(例如 log
//   框架的 trace id),但也最容易變成隱性耦合。
//
// 【2. 為什麼函式內要寫 static?其實不用】
//       thread_local static int local_tl = 0;
//   這裡的 static 是【多餘的】。C++ 標準規定:區域變數若標了 thread_local,
//   就隱含具有靜態儲存期(thread storage duration 本身就不是 automatic)。
//   寫出來的唯一價值是「向讀者強調這不是普通區域變數」。
//   反過來說,寫成 static thread_local 或 thread_local static 都合法且等價。
//
//   ⚠️ 但要注意:函式內的 thread_local 是【lazy 初始化】的 ——
//   每條執行緒第一次執行到那行宣告時才初始化。這代表每次存取
//   理論上都要檢查「初始化了沒」(GCC 會產生一個 TLS wrapper 函式)。
//   對可常數初始化的簡單型別,編譯器通常能最佳化掉這個檢查;
//   對有建構函式的型別則不一定。熱路徑上要留意這件事。
//
// 【3. 類別的 thread_local 靜態成員:為什麼必須在類別外定義】
//       class MyClass { thread_local static int member_tl; };   // 宣告
//       thread_local int MyClass::member_tl = 0;                // 定義
//
//   類別內只是【宣告】(告訴編譯器有這個成員),真正配置儲存空間的
//   【定義】必須在類別外、且只能出現一次(One Definition Rule)。
//   注意類別外定義時【不重複寫 static】,但【要重複寫 thread_local】——
//   這是很多人會寫錯的地方:
//       static int MyClass::member_tl = 0;              // ✗ 編譯錯誤
//       thread_local int MyClass::member_tl = 0;        // ✓ 正確
//
//   C++17 起若把它宣告成 inline,就可以在類別內直接初始化,不必分開寫:
//       class MyClass { inline static thread_local int member_tl = 0; };
//   這是 C++17 的 inline variable 特性,能省掉一個 .cpp 檔的樣板程式碼。
//
// 【4. 不能用在哪裡】
//     * 非靜態資料成員:
//           class C { thread_local int x; };     // ✗ 編譯錯誤
//       因為非靜態成員的生命週期由所屬物件決定,與「執行緒儲存期」
//       在概念上直接衝突 —— 同一個物件被兩條執行緒看到時,
//       它的成員到底該有幾份?這個問題沒有合理答案,所以標準禁止。
//     * 函式參數:
//           void f(thread_local int x);          // ✗ 編譯錯誤
//       參數本質上是 automatic 儲存期。
//
// 【5. 本檔示範的重點:計數彼此完全獨立】
//   兩條執行緒各呼叫 func() 兩次。若 global_tl / local_tl 是普通全域變數,
//   兩條執行緒會互相干擾,最終值取決於交錯順序(而且是 data race)。
//   有了 thread_local,每條執行緒看到的都是「1, 2」——
//   這是【確定的結果】,與排程完全無關。
//   ⚠️ 唯一不確定的是【兩條執行緒輸出的先後順序】,
//   所以本檔把結果收集到各自的槽位、join 之後再由主執行緒統一輸出,
//   讓輸出成為可驗證的確定結果。
//
// 【概念補充 Concept Deep Dive】
//
// (A) thread_local 物件的解構順序
//   在執行緒正常結束時,該執行緒的 thread_local 物件以【建構的相反順序】
//   解構。這與靜態物件的規則一致。但有兩個重要例外:
//     * detached thread 若在 main() 結束後才被作業系統終止,解構不保證發生。
//     * std::exit() / std::abort() 不會執行其他執行緒的 thread_local 解構。
//   因此 thread_local 不適合持有「必須被釋放」的資源(檔案、連線、鎖)。
//
// (B) 為什麼 main 執行緒也有自己的一份?
//   main 執行緒同樣是一條執行緒,它也有自己的 TLS 區塊。
//   所以在 main 裡讀 global_tl 得到的是 main 那一份,永遠不會看到
//   子執行緒改過的值。這常讓初學者困惑:「我明明加到 2 了,
//   為什麼 main 印出來是 0?」—— 因為那根本是兩個不同的物件。
//
// (C) 動態初始化的成本
//   若 thread_local 變數的初始化不是常數初始化(例如型別有建構函式),
//   編譯器必須確保「每條執行緒首次使用前完成初始化」。
//   GCC 的做法是產生一個 TLS init wrapper:每次存取先檢查一個
//   guard 變數。這比直接的 %fs 偏移存取貴,但仍遠低於取鎖的成本。
//   若想避免,就用可常數初始化的型別(int、指標、constexpr 建構的類別)。
//
// (D) TLS 與 fork() 的互動
//   fork() 只複製呼叫它的那一條執行緒。子行程中,原本其他執行緒的
//   thread_local 物件雖然在記憶體裡,卻沒有對應的執行緒 ——
//   若那些物件持有鎖,子行程就會繼承一個永遠不會被釋放的鎖。
//   這是多執行緒程式呼叫 fork() 的經典陷阱,POSIX 因此規定
//   fork() 之後在 exec() 之前只能呼叫 async-signal-safe 函式。
//
// 【注意事項 Pay Attention】
//   1. 類別的 thread_local 靜態成員必須在類別外定義,且定義時
//      【要寫 thread_local、不寫 static】。C++17 起可用 inline 省略。
//   2. thread_local 不能用於非靜態資料成員或函式參數。
//   3. 函式內的 static 在有 thread_local 時是多餘的(僅作強調用)。
//   4. thread_local 物件的解構在 detached thread 與 std::exit() 時
//      【不保證】發生。
//   5. 多條執行緒同時對 std::cout 輸出,行的順序不確定且可能交錯;
//      本檔改為「先收集、後統一輸出」以取得確定的結果。
//   6. 原始碼使用 std::cout 但 include 齊全;本檔另補 <string>、<vector>
//      供實務範例使用。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】thread_local 的宣告位置與限制
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. thread_local 可以用在哪些地方?不能用在哪裡?為什麼?
//     答：可以用在命名空間範圍的變數、函式內的區域變數、以及類別的
//         【靜態】資料成員。不能用在非靜態資料成員與函式參數上。
//         非靜態成員被禁止的理由是概念衝突:成員的生命週期由所屬物件
//         決定,而 thread_local 要求「每條執行緒一份」——
//         同一個物件被兩條執行緒看到時,它的成員該有幾份?無解,故禁止。
//     追問：類別的 thread_local 靜態成員要怎麼定義?
//         → 類別內宣告、類別外定義,定義時【要寫 thread_local、不寫
//           static】:thread_local int C::x = 0;。寫成 static 會編譯錯誤。
//           C++17 起可以寫 inline static thread_local int x = 0; 直接在
//           類別內初始化,省掉類別外那一行。
//
// 🔥 Q2. 在 main() 裡讀一個 thread_local 全域變數,會看到子執行緒改過的值嗎?
//     答：不會。main 執行緒也是一條執行緒,它有自己的 TLS 區塊與自己的
//         那一份實體。子執行緒操作的是【完全不同的物件】,連記憶體位址
//         都不一樣。所以 main 讀到的永遠是它自己那份(通常還是初始值)。
//     追問：那要怎麼把子執行緒的結果帶回 main?
//         → 用一般的共享機制:寫進各自的槽位再由 main 匯總(本檔做法)、
//           用 std::promise/future、或用受保護的共享容器。
//           thread_local 的用途是「避免共享」,不是「傳遞結果」。
//
// ⚠️ 陷阱 1. 「函式內的 thread_local static int x = 0;
//              每次呼叫這個函式,x 都會被重設為 0 吧?」
//     答：不會。thread_local 隱含靜態儲存期,初始化【每條執行緒只做一次】
//         (第一次執行到該宣告時),之後的呼叫沿用上次的值。
//         所以本檔中同一條執行緒呼叫 func() 兩次,會看到 1 然後 2。
//     為什麼會錯：把它看成普通區域變數,以為每次進函式都重新初始化。
//         實際上它的行為是「static 變數,但每條執行緒各有一個」。
//
// ⚠️ 陷阱 2. 「thread_local 的物件一定會在執行緒結束時被解構,
//              所以可以拿它來管理資源(RAII)。」
//     答：不可靠。解構只在執行緒【正常結束】時保證發生。
//         detached thread 若在 main() 結束後才被系統終止,解構不保證執行;
//         呼叫 std::exit() 或 std::abort() 時,其他執行緒的 thread_local
//         解構也完全不會執行。用它管理必須釋放的資源會漏。
//     為什麼會錯：把 thread_local 的解構保證等同於區域物件的 RAII 保證。
//         區域物件的解構由作用域嚴格保證;thread_local 的解構則依賴
//         「執行緒正常結束」這個前提,而那個前提並不總是成立。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

// -----------------------------------------------------------------------------
// 1. 命名空間範圍(全域)的 thread_local
// -----------------------------------------------------------------------------
thread_local int global_tl = 0;

// -----------------------------------------------------------------------------
// 2. 函式內的 thread_local
//    注意:static 是多餘的(thread_local 已隱含靜態儲存期),
//         保留只是為了強調「這不是普通區域變數」。
// -----------------------------------------------------------------------------
std::string func() {
    thread_local static int local_tl = 0;

    ++global_tl;
    ++local_tl;

    return "global_tl=" + std::to_string(global_tl)
         + " local_tl=" + std::to_string(local_tl);
}

// -----------------------------------------------------------------------------
// 3. 類別的 thread_local 靜態成員
//    類別內【宣告】,類別外【定義】——定義時寫 thread_local、不寫 static。
// -----------------------------------------------------------------------------
class MyClass {
public:
    thread_local static int member_tl;

    static std::string bump() {
        ++member_tl;
        return "member_tl=" + std::to_string(member_tl);
    }
};

thread_local int MyClass::member_tl = 0;   // ✓ 正確(寫 static 會編譯錯誤)

// -----------------------------------------------------------------------------
// 示範 1:兩條執行緒各呼叫兩次 —— 結果彼此完全獨立
//
//   為了讓輸出成為【確定的結果】,每條執行緒把自己的字串收進專屬槽位,
//   join 之後再由主執行緒統一輸出。若讓兩條執行緒直接 std::cout,
//   行的順序會隨排程改變。
// -----------------------------------------------------------------------------
void demoThreeForms() {
    std::vector<std::vector<std::string>> out(2);

    std::thread t1([&out]() {
        out[0].push_back("T1 第一次: " + func());
        out[0].push_back("T1 第二次: " + func());
        out[0].push_back("T1 類別成員: " + MyClass::bump());
    });
    std::thread t2([&out]() {
        out[1].push_back("T2 第一次: " + func());
        out[1].push_back("T2 第二次: " + func());
        out[1].push_back("T2 類別成員: " + MyClass::bump());
    });

    t1.join();
    t2.join();

    for (const auto& lines : out) {
        for (const auto& l : lines) std::cout << "  " << l << "\n";
    }

    std::cout << "  兩條執行緒都得到 1 然後 2 —— 互不干擾,與排程無關\n";
    std::cout << "  main 的 global_tl = " << global_tl
              << ",MyClass::member_tl = " << MyClass::member_tl
              << "(main 自己那一份,從未被子執行緒碰過)\n";
}

// -----------------------------------------------------------------------------
// 示範 2:同一條執行緒內,thread_local 不會每次呼叫都重設
// -----------------------------------------------------------------------------
void demoNotResetPerCall() {
    std::vector<std::string> lines;

    std::thread t([&lines]() {
        for (int i = 0; i < 5; ++i) lines.push_back(func());
    });
    t.join();

    std::cout << "  同一條執行緒連續呼叫 func() 五次:\n";
    for (const auto& l : lines) std::cout << "    " << l << "\n";
    std::cout << "  數值持續累加,證明它不是每次呼叫都重新初始化\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔【刻意不提供】
//
//   說明:LeetCode 的多執行緒題(1114 Print in Order、1115 Print FooBar
//   Alternately、1116 Print Zero Even Odd、1117 Building H2O、
//   1195 Fizz Buzz Multithreaded)全部要求執行緒【互相看得到彼此的狀態】
//   才能協調順序,而 thread_local 的語意恰好是「彼此看不到」。
//   把那些題目的共享狀態改成 thread_local,解法會直接失效
//   (每條執行緒只看得到自己的計數器,永遠等不到別人)。
//   兩者方向相反、無交集,故從缺。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】每執行緒獨立的亂數產生器
//
//   情境:負載測試工具要為每個請求產生隨機參數(隨機使用者 id、
//         隨機延遲、隨機商品)。跑在 N 條 worker 上,每秒數十萬次。
//
//   為什麼這是 thread_local 的教科書用例:
//     * std::mt19937 【不是執行緒安全的】—— 它內部有 624 個 word 的狀態,
//       每次 operator() 都會更新。多條執行緒共用一個就是 data race(UB)。
//     * 用 mutex 保護?那所有 worker 會在每次取亂數時互相排隊,
//       而取亂數是最熱的路徑之一 —— 鎖競爭會直接吃掉平行化的好處。
//     * 每次現場建立一個?mt19937 的建構要初始化 2.5KB 的狀態,比取一個
//       亂數貴上千倍,而且會產生大量重複序列。
//     * thread_local:每條執行緒一個產生器,零共享、零鎖、只建構一次。
//
//   ⚠️ 種子必須【每條執行緒不同】,否則所有 worker 會產生一模一樣的序列。
//   這裡用固定的 baseSeed + 執行緒索引,讓結果可重現(便於驗證);
//   真實壓測工具通常用 std::random_device 或時間戳混入。
// -----------------------------------------------------------------------------
thread_local std::mt19937 g_rng{0};
thread_local bool         g_rngSeeded = false;

void seedThreadRng(unsigned seed) {
    g_rng.seed(seed);
    g_rngSeeded = true;
}

int nextRequestSize() {
    // 若忘了設種子,至少不要靜默產生全部相同的序列
    if (!g_rngSeeded) seedThreadRng(12345);
    std::uniform_int_distribution<int> dist(1, 1000);
    return dist(g_rng);
}

int main() {
    std::cout << "=== 示範 1:三種宣告位置,結果彼此獨立 ===\n";
    demoThreeForms();

    std::cout << "\n=== 示範 2:thread_local 不會每次呼叫都重設 ===\n";
    demoNotResetPerCall();

    std::cout << "\n=== 日常實務:每執行緒獨立的亂數產生器 ===\n";
    {
        const int T = 4;
        const int N = 5;
        std::vector<std::vector<int>> samples(T);
        std::vector<std::thread>      workers;
        workers.reserve(T);

        for (int i = 0; i < T; ++i) {
            workers.emplace_back([i, N, &samples]() {
                seedThreadRng(1000u + static_cast<unsigned>(i));  // 每條種子不同
                for (int k = 0; k < N; ++k) samples[i].push_back(nextRequestSize());
            });
        }
        for (auto& w : workers) w.join();

        for (int i = 0; i < T; ++i) {
            std::cout << "  worker " << i << " 產生:";
            for (std::size_t k = 0; k < samples[i].size(); ++k) {
                std::cout << (k ? ", " : " ") << samples[i][k];
            }
            std::cout << "\n";
        }

        bool allDifferent = true;
        for (int i = 1; i < T; ++i) {
            if (samples[i] == samples[0]) allDifferent = false;
        }
        std::cout << "  各 worker 的序列互不相同: " << std::boolalpha
                  << allDifferent << "(種子不同 → 序列不同)\n";
        std::cout << "  全程零鎖、零共享;mt19937 每條執行緒只建構一次\n";
    }

    std::cout << "\n=== 全部示範結束 ===\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.5：執行緒本地儲存3.cpp" -o tls3

// 註:本檔輸出是【確定的】—— 所有子執行緒的結果都先收進各自的槽位,
//     join() 之後才由主執行緒統一輸出,因此不受排程順序影響。
//     亂數序列因為使用固定種子(1000 + 執行緒索引)也可完全重現;
//     真實壓測工具會改用 std::random_device,那才會每次不同。

// === 預期輸出 ===
// === 示範 1:三種宣告位置,結果彼此獨立 ===
//   T1 第一次: global_tl=1 local_tl=1
//   T1 第二次: global_tl=2 local_tl=2
//   T1 類別成員: member_tl=1
//   T2 第一次: global_tl=1 local_tl=1
//   T2 第二次: global_tl=2 local_tl=2
//   T2 類別成員: member_tl=1
//   兩條執行緒都得到 1 然後 2 —— 互不干擾,與排程無關
//   main 的 global_tl = 0,MyClass::member_tl = 0(main 自己那一份,從未被子執行緒碰過)
//
// === 示範 2:thread_local 不會每次呼叫都重設 ===
//   同一條執行緒連續呼叫 func() 五次:
//     global_tl=1 local_tl=1
//     global_tl=2 local_tl=2
//     global_tl=3 local_tl=3
//     global_tl=4 local_tl=4
//     global_tl=5 local_tl=5
//   數值持續累加,證明它不是每次呼叫都重新初始化
//
// === 日常實務:每執行緒獨立的亂數產生器 ===
//   worker 0 產生: 654, 206, 116, 611, 951
//   worker 1 產生: 307, 380, 266, 552, 197
//   worker 2 產生: 129, 43, 462, 108, 417
//   worker 3 產生: 598, 5, 436, 700, 17
//   各 worker 的序列互不相同: true(種子不同 → 序列不同)
//   全程零鎖、零共享;mt19937 每條執行緒只建構一次
//
// === 全部示範結束 ===
