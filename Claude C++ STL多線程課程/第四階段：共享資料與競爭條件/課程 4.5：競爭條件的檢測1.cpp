// =============================================================================
//  課程 4.5：競爭條件的檢測1.cpp  —  怎麼「證明」程式有沒有資料競爭
// =============================================================================
//
// 【本檔性質】原始版本是給 ThreadSanitizer 練習用的最小重現案例：
//   兩條執行緒對同一個 counter 遞增，無同步 → genuine data race → 【UB】。
//   原始版本【完全沒有輸出】，因為它的用途是「拿去餵給工具」，不是給人看。
//   本檔保留該最小案例，並補上「為什麼肉眼不可靠、工具為什麼可靠」的完整說明
//   與可執行的壓力測試示範。
//
// 【主題資訊 Information】
//   主題：    資料競爭的偵測方法：TSan / Helgrind / 壓力測試 / 人工審查
//   編譯旗標：-fsanitize=thread -g   （TSan，g++ 與 clang++ 皆支援）
//   標準版本：std::thread 為 C++11；TSan 是編譯器擴充，不是標準的一部分
//   標頭檔：  <thread>、<atomic>
//   本機環境：Ubuntu 26.04 / g++ 15.2.0 / 16 核心
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「跑很多次都對」不能當作證據】
//   這是整個第四階段最重要、也最反直覺的一句話：
//   【資料競爭是「有沒有」的問題，不是「多常發生」的問題】。
//   data race 一旦成立就是 UB，標準不再對程式的任何行為負責 ——
//   包含「看起來完全正常」。所以：
//     * 跑 100 萬次都對 → 只證明這 100 萬次沒撞上，不證明沒有競爭。
//     * 換一台機器、換一個最佳化等級、換一個負載，隨時會現形。
//   把並行 bug 當成「有機率出現的錯誤答案」並用重複執行去否證，
//   是這個領域最常見、也最昂貴的錯誤心智模型。
//
// 【2. ThreadSanitizer 為什麼可靠】
//   TSan 不靠碰運氣，它做的是【形式化判定】：
//     * 為每個記憶體位置維護 shadow memory（記錄最近幾次存取的
//       執行緒 id、時鐘值、讀/寫類型）
//     * 為每條執行緒維護 vector clock（向量時鐘），
//       透過 mutex lock/unlock、thread create/join、atomic 操作等
//       同步點來推進時鐘
//     * 每次存取時比對：這兩次存取之間存在 happens-before 關係嗎？
//       不存在 → 立刻報告 data race
//   關鍵在於：【只要執行過一次衝突路徑就會報警】，
//   不需要真的觀察到錯誤的數值。這就是它與壓力測試的本質差異。
//
//   代價（本機量級）：記憶體約 5～10 倍、執行速度約 2～20 倍。
//   所以 TSan 適合在 CI 或開發環境跑，不適合上正式環境。
//
// 【3. 四種偵測手段的比較】
//   ┌──────────────┬────────────┬──────────┬────────────────────────────┐
//   │ 手段          │ 需重新編譯 │ 速度      │ 能力與限制                  │
//   ├──────────────┼────────────┼──────────┼────────────────────────────┤
//   │ ThreadSanitizer│ 要         │ 慢 2-20x │ 最準；只看「執行到的路徑」    │
//   │ Helgrind      │ 不用       │ 慢 20-50x│ 不用重編，較慢，誤報略多      │
//   │ 壓力測試       │ 不用       │ 快        │ 只能提高機率，無法證明不存在  │
//   │ 人工審查       │ 不用       │ —        │ 能發現工具跑不到的路徑        │
//   └──────────────┴────────────┴──────────┴────────────────────────────┘
//   → 這四者是【互補】而非取代關係。TSan 最強，但它只能報告
//     「實際被執行到」的程式碼路徑；沒被測試涵蓋到的分支它一樣看不到。
//     所以 TSan 的效果取決於測試涵蓋率，人工審查仍不可少。
//
// 【4. 人工審查要找什麼（五大競爭模式）】
//   ① Check-Then-Act：if (條件) { 依賴該條件的動作 }      → 4.1 / 4.4-3
//   ② Read-Modify-Write：counter++、x = x + 1            → 4.4-2
//   ③ 複合操作：一個不變量橫跨多個變數，卻分開更新          → 4.2-3
//   ④ 迭代器失效：一邊走訪一邊修改容器                     → 4.4-4
//   ⑤ 部分更新：物件的多個欄位分開賦值，讀者看到混搭狀態     → 4.4-6
//   在 code review 時直接拿這張清單去對，命中率很高。
//
// 【5. TSan 報告怎麼讀】
//   典型報告的結構：
//       WARNING: ThreadSanitizer: data race (pid=12345)
//         Write of size 4 at 0x... by thread T2:      ← 誰、做了什麼
//           #0 increment() race_demo.cpp:8
//         Previous read of size 4 at 0x... by thread T1:  ← 與誰衝突
//           #0 increment() race_demo.cpp:8
//         Location is global 'counter' of size 4 at 0x... ← 哪個變數
//   要看三件事：①衝突的兩次存取分別在哪一行 ②是讀還是寫 ③是哪個變數。
//   必須加 -g 才會有行號，否則只有位址，幾乎無法閱讀。
//
// 【概念補充 Concept Deep Dive】
//
// (A) TSan 為什麼會漏報（false negative）
//   TSan 只分析【實際執行到】的路徑。若某段程式碼在測試中從未被執行，
//   或某個分支需要特定輸入才會進入，TSan 就看不到它。
//   此外 shadow memory 每個位置只保留有限筆歷史（預設 4 筆），
//   極端情況下舊的存取記錄會被擠掉。
//   → 結論：TSan 沒報錯 ≠ 程式沒有競爭，只代表「這次跑到的路徑沒有」。
//
// (B) 為什麼不能同時用 TSan 和 ASan
//   兩者都需要接管記憶體配置與 shadow memory 佈局，互相衝突，
//   必須分開編譯、分開執行。
//   實務上的做法是 CI 跑兩個獨立的 job。
//
// (C) 為什麼 -O2 會讓競爭「消失」
//   最佳化可能把整個迴圈摺疊成一道指令（例如
//   `add DWORD PTR counter[rip], 1000`），競爭視窗窄到幾乎撞不到。
//   但那道指令沒有 lock 前綴，仍非原子操作，UB 依然存在。
//   TSan 在 -O1/-O2 下仍然抓得到，因為它是在編譯期插樁（instrumentation），
//   判定依據是 happens-before 而不是實際的數值錯誤。
//   官方建議搭配 -O1 與 -fno-omit-frame-pointer 以取得可讀的堆疊。
//
// 【注意事項 Pay Attention】
// 1. 「跑幾次都對」永遠不能當作沒有 data race 的證據。
// 2. TSan 必須加 -g 才有行號；建議搭配 -O1 -fno-omit-frame-pointer。
// 3. TSan 與 AddressSanitizer 不能同時開啟，要分開編譯執行。
// 4. TSan 沒報錯只代表「這次執行的路徑」乾淨，不代表整個程式乾淨。
// 5. TSan 會讓程式變慢 2～20 倍、記憶體變 5～10 倍，別放上正式環境。
// 6. 本檔第一段是 UB 示範，任何「一定會…」的敘述都是錯的。
//
// 【LeetCode 說明】本檔不附 LeetCode 範例。
//   本檔主題是「偵測工具與方法論」，不是演算法或資料結構；
//   允許使用的設計題（146/155/705/707/1603）與並行題（1114～1117/1195）
//   都無法誠實對應「如何證明程式沒有 data race」這件事。
//   硬湊一題只會稀釋重點，故從缺，改以下方兩個真實情境
//   （CI 的 TSan 關卡、上線前的壓力測試腳本）呈現。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】資料競爭的偵測
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. ThreadSanitizer 是怎麼偵測 data race 的?為什麼比壓力測試可靠?
//     答：它為每個記憶體位置維護 shadow memory，為每條執行緒維護向量時鐘，
//         並在 mutex / thread join / atomic 等同步點推進時鐘。
//         每次存取時直接判定「這兩次存取之間有沒有 happens-before 關係」，
//         沒有就報告。關鍵是【只要執行過一次衝突路徑就會報警】，
//         不需要真的觀察到錯誤數值 —— 壓力測試則必須剛好撞上才看得到。
//     追問：那 TSan 沒報錯就代表程式安全嗎?
//         → 不代表。它只分析實際執行到的路徑，
//           沒被測試涵蓋的分支它看不到，所以效果取決於測試涵蓋率。
//
// 🔥 Q2. 為什麼開了 -O2 之後，原本會出錯的程式好像就正常了?
//     答：最佳化可能把整個迴圈摺疊成單一指令，競爭視窗窄到撞不到。
//         但那道指令沒有 lock 前綴，仍非原子操作，data race 與 UB 都還在。
//         TSan 在 -O2 下仍然抓得到，因為它是編譯期插樁、
//         依據 happens-before 判定，不依賴數值是否出錯。
//     追問：那 TSan 該用哪個最佳化等級?
//         → 官方建議 -O1 搭配 -g -fno-omit-frame-pointer：
//           太低會慢到難以忍受，太高則堆疊資訊會被最佳化掉不好讀。
//
// ⚠️ 陷阱. 「我加了壓力測試，連續跑一百萬次都正確，可以上線了」——錯在哪?
//     答：壓力測試只能提高撞上競爭的機率，永遠無法證明競爭不存在。
//         而且 UB 的表現之一就是「看起來完全正常」。
//         換一台核心數不同的機器、換一個編譯器版本、
//         或線上流量模式改變，隨時會現形。
//     為什麼會錯：把並行正確性當成可以用抽樣統計來驗證的性質。
//         它其實是【程式的結構性質】——要嘛所有衝突存取之間都有
//         happens-before 關係，要嘛沒有。這需要形式化判定（TSan）
//         或人工推理，不是靠跑幾次。
// ═══════════════════════════════════════════════════════════════════════════

// 檔案：race_demo.cpp
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <string>

// -----------------------------------------------------------------------------
// 【最小重現案例】給 TSan 練習用 —— 本課主角
//   刻意保持極簡：越小的重現案例，工具報告越好讀、越容易溝通。
//   回報並行 bug 時，「附上最小重現案例」比長篇描述有用一百倍。
// -----------------------------------------------------------------------------
int counter = 0;

void increment() {
    for (int i = 0; i < 1000; ++i) {
        counter++;
    }
}

// -----------------------------------------------------------------------------
// 【示範 1】壓力測試：能提高機率，但無法證明「不存在」
//   重複執行「兩條執行緒各加 N 次」很多輪，統計有幾輪的結果不等於預期。
//   注意：這裡每一輪都是 data race（UB），統計只是為了說明
//   「肉眼觀察到的失敗率」和「錯誤是否存在」是兩件不同的事。
// -----------------------------------------------------------------------------
int stressTestRaces(int rounds, int perThread) {
    int mismatches = 0;
    for (int r = 0; r < rounds; ++r) {
        int shared = 0;
        std::thread a([&shared, perThread] {
            for (int i = 0; i < perThread; ++i) ++shared;   // 無同步 → UB
        });
        std::thread b([&shared, perThread] {
            for (int i = 0; i < perThread; ++i) ++shared;
        });
        a.join();
        b.join();
        if (shared != perThread * 2) ++mismatches;
    }
    return mismatches;
}

// 對照組：同樣的壓力測試，但用 atomic（沒有 UB，必定全對）
int stressTestAtomic(int rounds, int perThread) {
    int mismatches = 0;
    for (int r = 0; r < rounds; ++r) {
        std::atomic<int> shared{0};
        std::thread a([&shared, perThread] {
            for (int i = 0; i < perThread; ++i)
                shared.fetch_add(1, std::memory_order_relaxed);
        });
        std::thread b([&shared, perThread] {
            for (int i = 0; i < perThread; ++i)
                shared.fetch_add(1, std::memory_order_relaxed);
        });
        a.join();
        b.join();
        if (shared.load() != perThread * 2) ++mismatches;
    }
    return mismatches;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】CI 的並行測試關卡
//   情境：專案裡所有涉及多執行緒的測試，都要在 CI 用 TSan 跑一遍。
//         這是業界標準做法（Chromium、Firefox、TensorFlow 都有 TSan bot）。
//   關鍵設計：
//     ① TSan job 與一般 job 分開（TSan 慢 2-20 倍，不該拖慢每次提交）
//     ② TSAN_OPTIONS=halt_on_error=1 讓第一個競爭就中斷，避免洗版
//     ③ 用 suppressions 檔案暫時忽略第三方函式庫的已知問題，
//        但自己的程式碼一律零容忍
//     ④ TSan 與 ASan 分成兩個 job（兩者不能同時開）
//   下面用一個「檢查清單」函式把這套流程可執行地表達出來。
// -----------------------------------------------------------------------------
struct CiGate {
    std::string name;
    std::string command;
    bool blocking;      // 失敗是否阻擋合併
};

std::vector<CiGate> concurrencyGates() {
    return {
        {"unit-tests",
         "g++ -std=c++17 -O2 -Wall -Wextra -pthread ... && ./tests", true},
        {"tsan",
         "g++ -std=c++17 -O1 -g -fno-omit-frame-pointer "
         "-fsanitize=thread -pthread ... && TSAN_OPTIONS=halt_on_error=1 ./tests", true},
        {"asan",
         "g++ -std=c++17 -O1 -g -fsanitize=address,undefined -pthread ... && ./tests", true},
        {"stress",
         "for i in $(seq 200); do ./tests --concurrency || exit 1; done", false},
        {"helgrind",
         "valgrind --tool=helgrind ./tests   # 不需重編，較慢，作為補充", false},
    };
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】把「共享狀態」盤點成可審查的清單
//   情境：接手一份既有的多執行緒程式碼，第一件事不是跑工具，
//         而是先盤點「哪些狀態是共享的、各自由誰保護」。
//         這份表格在 code review 與交接時的價值極高 ——
//         工具只能檢查跑到的路徑，這張表能讓人一眼看出哪裡沒被保護。
// -----------------------------------------------------------------------------
struct SharedStateEntry {
    std::string name;        // 狀態名稱
    std::string kind;        // 全域 / static / 成員 / 堆積
    std::string guardedBy;   // 由誰保護（鎖名 / atomic / 不需要）
    std::string pattern;     // 若未受保護，屬於哪一種競爭模式
};

std::vector<SharedStateEntry> auditSharedState() {
    return {
        {"counter",        "全域 int",    "❌ 無保護",      "Read-Modify-Write"},
        {"g_configMap",    "全域 map",    "configMutex",   "—"},
        {"g_requestStats", "全域 struct", "statsMutex",    "—"},
        {"s_callCount",    "函式內 static","❌ 無保護",     "Read-Modify-Write"},
        {"g_isReady",      "全域 bool",   "std::atomic",   "—"},
        {"tlsBuffer",      "thread_local","不需要（不共享）","—"},
    };
}

int main() {
    std::cout << "=== 最小重現案例（供 TSan 使用；本身是 data race / UB）===\n";
    {
        std::thread t1(increment);
        std::thread t2(increment);
        t1.join();
        t2.join();
        std::cout << "counter = " << counter << " (預期 2000)\n";
        std::cout << "→ 這個值每次執行都可能不同；等於 2000 也不代表程式正確。\n";
    }

    std::cout << "\n=== 示範 1：壓力測試能提高機率，但無法證明不存在 ===\n";
    {
        const int rounds = 200, perThread = 50000;
        int raced = stressTestRaces(rounds, perThread);
        int atomicBad = stressTestAtomic(rounds, perThread);

        std::cout << "無同步版本：" << rounds << " 輪中有 " << raced
                  << " 輪結果不等於 " << (perThread * 2) << "\n";
        std::cout << "atomic 版本：" << rounds << " 輪中有 " << atomicBad
                  << " 輪結果不等於 " << (perThread * 2) << " (必定為 0)\n";
        std::cout << "→ 無同步版本的失敗輪數每次執行都不同；\n";
        std::cout << "  就算某次剛好 0 輪失敗，data race 依然存在。\n";
        std::cout << "  壓力測試提高的是「撞上的機率」，不是「正確性的證明」。\n";
    }

    std::cout << "\n=== 示範 2：偵測手段比較 ===\n";
    {
        std::cout << "手段              需重編  相對速度   能力\n";
        std::cout << "----------------------------------------------------------\n";
        std::cout << "ThreadSanitizer   要      2-20x 慢   最準，但只看執行到的路徑\n";
        std::cout << "Helgrind          不用    20-50x 慢  免重編，較慢，誤報略多\n";
        std::cout << "壓力測試          不用    1x         只提高機率，無法證明不存在\n";
        std::cout << "人工審查          不用    —          能涵蓋工具跑不到的路徑\n";
    }

    std::cout << "\n=== 日常實務 1：CI 的並行測試關卡 ===\n";
    {
        for (const auto& g : concurrencyGates()) {
            std::cout << (g.blocking ? "[阻擋] " : "[警告] ") << g.name << "\n";
            std::cout << "        " << g.command << "\n";
        }
        std::cout << "註：TSan 與 ASan 不能同時開啟，必須分成兩個獨立的 job。\n";
    }

    std::cout << "\n=== 日常實務 2：共享狀態盤點表（人工審查用）===\n";
    {
        std::cout << "狀態名稱         種類            保護方式           風險模式\n";
        std::cout << "--------------------------------------------------------------------\n";
        for (const auto& e : auditSharedState()) {
            std::cout << e.name;
            for (size_t i = e.name.size(); i < 17; ++i) std::cout << ' ';
            std::cout << e.kind;
            for (size_t i = e.kind.size(); i < 16; ++i) std::cout << ' ';
            std::cout << e.guardedBy;
            for (size_t i = e.guardedBy.size(); i < 19; ++i) std::cout << ' ';
            std::cout << e.pattern << "\n";
        }
        std::cout << "→ 標記 ❌ 的兩筆就是要優先處理的目標。\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 4.5：競爭條件的檢測1.cpp' -o race_demo
//
// ── 本課重點：用工具而不是用眼睛 ──────────────────────────────────────────
// ThreadSanitizer（最推薦，務必實際跑一次看報告長什麼樣子）:
//   g++ -std=c++17 -O1 -g -fno-omit-frame-pointer -fsanitize=thread -pthread '課程 4.5：競爭條件的檢測1.cpp' -o race_tsan
//   TSAN_OPTIONS=halt_on_error=1 ./race_tsan
//
// Helgrind（不需重新編譯，較慢）:
//   valgrind --tool=helgrind ./race_demo
//
// AddressSanitizer（另一個 job；不可與 TSan 同時開啟）:
//   g++ -std=c++17 -O1 -g -fsanitize=address,undefined -pthread ... -o race_asan

// ⚠️ 兩處數字【每次執行都不同】，下面只是本機某一次的真實實測：
//   (1) 第一段的 counter ——該段是 genuine data race → UB。
//       本機連續三次實測為 1012 / 2000 / 2000；
//       其中兩次「剛好正確」，正是本課要破除的錯覺。
//   (2) 壓力測試「無同步版本」的失敗輪數 ——同樣是 UB。
//       本機連續三次實測為 200 / 200 / 199（共 200 輪）。
//       這個數字高只是因為每輪工作量夠大（各 50000 次）撐開了競爭視窗；
//       它不是保證值，也不是「正確性的量測」。
// atomic 對照組必定為 0 輪失敗；其餘各段為靜態表格，內容固定。

// === 預期輸出 ===
// === 最小重現案例（供 TSan 使用；本身是 data race / UB）===
// counter = 2000 (預期 2000)
// → 這個值每次執行都可能不同；等於 2000 也不代表程式正確。
//
// === 示範 1：壓力測試能提高機率，但無法證明不存在 ===
// 無同步版本：200 輪中有 200 輪結果不等於 100000
// atomic 版本：200 輪中有 0 輪結果不等於 100000 (必定為 0)
// → 無同步版本的失敗輪數每次執行都不同；
//   就算某次剛好 0 輪失敗，data race 依然存在。
//   壓力測試提高的是「撞上的機率」，不是「正確性的證明」。
//
// === 示範 2：偵測手段比較 ===
// 手段              需重編  相對速度   能力
// ----------------------------------------------------------
// ThreadSanitizer   要      2-20x 慢   最準，但只看執行到的路徑
// Helgrind          不用    20-50x 慢  免重編，較慢，誤報略多
// 壓力測試          不用    1x         只提高機率，無法證明不存在
// 人工審查          不用    —          能涵蓋工具跑不到的路徑
//
// === 日常實務 1：CI 的並行測試關卡 ===
// [阻擋] unit-tests
//         g++ -std=c++17 -O2 -Wall -Wextra -pthread ... && ./tests
// [阻擋] tsan
//         g++ -std=c++17 -O1 -g -fno-omit-frame-pointer -fsanitize=thread -pthread ... && TSAN_OPTIONS=halt_on_error=1 ./tests
// [阻擋] asan
//         g++ -std=c++17 -O1 -g -fsanitize=address,undefined -pthread ... && ./tests
// [警告] stress
//         for i in $(seq 200); do ./tests --concurrency || exit 1; done
// [警告] helgrind
//         valgrind --tool=helgrind ./tests   # 不需重編，較慢，作為補充
// 註：TSan 與 ASan 不能同時開啟，必須分成兩個獨立的 job。
//
// === 日常實務 2：共享狀態盤點表（人工審查用）===
// 狀態名稱         種類            保護方式           風險模式
// --------------------------------------------------------------------
// counter          全域 int      ❌ 無保護      Read-Modify-Write
// g_configMap      全域 map      configMutex        —
// g_requestStats   全域 struct   statsMutex         —
// s_callCount      函式內 static❌ 無保護      Read-Modify-Write
// g_isReady        全域 bool     std::atomic        —
// tlsBuffer        thread_local    不需要（不共享）—
// → 標記 ❌ 的兩筆就是要優先處理的目標。
