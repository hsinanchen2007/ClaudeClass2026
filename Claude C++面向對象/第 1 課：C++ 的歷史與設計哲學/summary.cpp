/*
 * ============================================================
 * 【第 1 課：C++ 的歷史與設計哲學】總複習 summary.cpp
 * ============================================================
 *
 * 本課程重點：
 * 1. C++ 的誕生背景與歷史時間線
 * 2. 創造者 Bjarne Stroustrup 的設計動機
 * 3. 名稱「C++」的由來
 * 4. C++ 的四大設計哲學原則
 * 5. C++ 的多範式程式設計
 * 6. 各種程式設計範式的實際範例
 *
 * ============================================================
 * 學習目的：
 * 理解 C++ 的歷史背景和設計哲學，有助於理解「為什麼 C++
 * 要這樣設計」，而不只是「C++ 能做什麼」。當你了解設計者
 * 的思維，很多看似複雜的語法規則都會變得合理。
 * ============================================================
 */

// =============================================================================
// 【主題資訊 Information】
// -----------------------------------------------------------------------------
//   主題：    C++ 的歷史與設計哲學 —— 「為什麼 C++ 長這樣」
//   關鍵人物：Bjarne Stroustrup（1979 年起於貝爾實驗室開發 "C with Classes"）
//   關鍵年份：1972 C 誕生 / 1979 C with Classes / 1983 更名 C++ /
//             1998 首個 ISO 標準 C++98 / 2011 C++11（現代 C++ 的分水嶺）/
//             之後每三年一版：C++14、17、20、23
//   標頭檔：  <iostream>、<vector>、<algorithm>、<string>
//   本檔標準：使用 auto、lambda、range-based for → 需 C++11 起；示範以 C++17 編譯。
//
// =============================================================================
// 【詳細解釋 Explanation】
// -----------------------------------------------------------------------------
//
// 【1. C++ 要解決的原始問題】
//   Stroustrup 的博士研究用 Simula 做模擬 —— Simula 的類別與繼承讓他
//   能把複雜系統表達得很清楚，但**執行速度太慢**，跑不動真實規模的模擬。
//   他後來改用 BCPL（接近機器的語言）重寫，速度夠了，但**抽象能力全失**。
//   C++ 就是在回答這個矛盾：
//       「能不能同時擁有 Simula 的表達力，與 C 的執行效率？」
//   理解這一點，就理解了 C++ 幾乎所有設計取捨的來由 ——
//   它從來不是在追求「最優雅」或「最安全」，而是在追求
//   **「不犧牲效率的前提下盡可能提高抽象層次」**。
//
// 【2. 四大設計哲學（本檔重點三）】
//   (a) **零開銷抽象**（zero-overhead abstraction）
//       Stroustrup 的原話是：「你不使用的東西，不需要付出代價；
//       你使用的東西，不可能手寫得更好。」
//       所以 C++ 沒有強制的 GC、沒有強制的執行期型別檢查、
//       沒有強制的邊界檢查 —— 這些都是「你沒用就不該付費」的東西。
//   (b) **信任程式設計師**
//       C++ 允許你做危險的事（指標算術、reinterpret_cast、手動記憶體管理），
//       因為系統程式設計有時真的需要。代價是：語言不會攔住你的錯誤。
//   (c) **直接映射硬體**
//       int 對應暫存器、指標對應位址、陣列對應連續記憶體、
//       位元運算對應 CPU 指令。你寫的東西與機器實際做的事距離很近。
//   (d) **多範式**
//       不強迫你用某一種風格。同一個問題可以用程序式、物件導向、
//       泛型、函數式來寫，由你依情境選擇。
//
// 【3. 「零開銷」的真正含義（最常被誤解的一點）】
//   零開銷**不是**「所有 C++ 特性都免費」，而是兩個更精確的承諾：
//     (i)  沒用到的特性不收費 —— 沒有 virtual 函式的類別不會有 vptr；
//          沒寫 try/catch 的程式在正常路徑上不為例外付出成本。
//     (ii) 用到的特性，其成本不會高於你手寫同樣功能的 C 程式碼 ——
//          virtual 呼叫的成本，就等於你自己用函式指標表手刻的成本。
//   **有成本的特性依然有成本**：virtual 會多一次間接跳轉、
//   例外處理會讓二進位檔變大、RTTI 需要型別資訊表。
//   零開銷保證的是「不浪費」，不是「不花錢」。
//
// 【4. 為什麼叫 C++ 而不是 D 或 C＋1】
//   ++ 是 C 的遞增運算子，取「在 C 的基礎上前進一步」之意，
//   由 Rick Mascitti 於 1983 年提議。
//   ★ 有個很好的雙關梗：C++ 這個名字用的是**後置**遞增 ——
//     而後置遞增的語意是「先回傳舊值，再遞增」。
//     所以嚴格照語意讀，C++ 這個運算式的值其實還是 C。
//     真要表達「先進化再使用」，應該寫成 ++C。
//     這個玩笑常出現在面試的暖場題。
//
// 【5. 多範式在本檔的四個示範】
//     程序式    proceduralAdd()      —— 函式 + 資料分離，最接近 C
//     物件導向  Calculator 類別      —— 資料與行為封裝在一起
//     泛型      getMax<T>()          —— 一份程式碼適用多種型別，編譯期產生
//     函數式    sort + lambda        —— 把行為當成值傳遞，強調不可變與組合
//   四者可以在同一個檔案、甚至同一個函式裡混用 ——
//   這正是 C++ 與「純 OOP 語言」最大的文化差異。
//
// =============================================================================
// 【概念補充 Concept Deep Dive】
// -----------------------------------------------------------------------------
// (A) 泛型的零開銷是怎麼做到的
//   getMax<int> 與 getMax<double> 在編譯期會被**實體化成兩份獨立的機器碼**，
//   各自針對該型別最佳化，完全沒有執行期的型別判斷或裝箱（boxing）。
//   這與 Java 的泛型（type erasure，執行期其實都是 Object）是根本不同的路線：
//   C++ 用**編譯期展開**換取零執行期成本，代價是二進位檔可能變大
//   （code bloat）與編譯時間變長。
//
// (B) 「直接映射硬體」在本檔的體現
//   demonstrateHardwareMapping() 裡的指標與位元運算，
//   幾乎是一對一對應到 CPU 指令的：
//     *ptr = 100    → 一次記憶體寫入
//     flags << 2    → 一道 shift 指令
//   這種「所寫即所得」的可預測性，正是作業系統、嵌入式、
//   遊戲引擎、高頻交易仍選擇 C++ 的核心理由。
//
// (C) 「信任程式設計師」的代價，與現代 C++ 的修正方向
//   這個哲學讓 C++ 極其強大，但也讓它成為記憶體安全問題的重災區。
//   現代 C++（C++11 起）的演進方向，其實是在**不移除**這些能力的前提下，
//   提供更安全的預設選項：
//     裸指標 + new/delete  →  unique_ptr / shared_ptr（RAII）
//     C 陣列               →  std::array / std::vector
//     手寫迴圈             →  range-based for / 演算法
//   語言沒有禁止舊寫法（相容性是 C++ 的鐵律），
//   而是讓正確的寫法變得更方便 —— 這也是一種設計哲學。
//
// (D) 相容性作為約束
//   C++ 幾乎不移除任何東西。這讓三十年前的程式碼今天多半還能編譯，
//   代價是語言累積了大量歷史包袱（如 C 風格轉型、隱式轉換規則、
//   most vexing parse）。理解「相容性優先」這條約束，
//   就能理解為什麼很多明顯的缺陷至今仍在。
//
// =============================================================================
// 【注意事項 Pay Attention】
// -----------------------------------------------------------------------------
//  1. 「零開銷」是指「不用不收費、用了不比手寫差」，
//     **不是**「所有特性都免費」。virtual、例外、RTTI 都有真實成本。
//  2. C++ **不是** C 的超集。有效的 C 程式未必是有效的 C++
//     （例如 C 允許 void* 隱式轉成 T*，C++ 不允許）。
//  3. 多範式不代表「隨便混」—— 同一個模組內風格應保持一致，
//     否則維護者要同時理解多套心智模型。
//  4. 本檔輸出含記憶體位址（0x...），**每次執行都不同**（stack 位置 + ASLR），
//     不可寫入測試斷言。
//  5. template 的零開銷代價是編譯時間與二進位大小，大型專案需留意。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】C++ 的設計哲學
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是「零開銷抽象」？請舉例說明。
//     答：Stroustrup 的定義是「你不使用的東西不付出代價；
//         你使用的東西，不可能手寫得更好」。
//         例：沒有 virtual 函式的類別不會有 vptr，
//         所以 struct Point { int x, y; } 的 sizeof 就是 8，
//         與 C 的 struct 完全相同、零額外負擔。
//         而用了 virtual 時，其成本等同你自己手刻函式指標表。
//     追問：那 virtual 是零開銷嗎？
//         → 不是「免費」，是「不浪費」。它有一次間接跳轉的成本
//           與每物件一個 vptr 的空間成本，
//           但你若要在 C 裡實作同樣的動態分派，成本一模一樣。
//
// 🔥 Q2. C++ 是 C 的超集嗎？
//     答：**不是**，這是很常見的錯誤說法。雖然相容性極高，
//         但有效的 C 程式未必是有效的 C++。
//         最典型的例子：C 允許 void* 隱式轉成 T*
//         （所以 int* p = malloc(n); 在 C 合法、在 C++ 編譯錯誤）；
//         C 允許同一個 struct 名稱與變數名共存；
//         C99 的 VLA（變長陣列）不在 C++ 標準內。
//     追問：那為什麼大家常說是超集？
//         → 因為 Stroustrup 的目標是「盡可能相容」，
//           實務上絕大多數 C 程式碼確實能直接編譯，
//           所以口語上被簡化成「超集」。
//
// 🔥 Q3. C++ 的泛型與 Java 的泛型有什麼本質差別？
//     答：C++ 的 template 是**編譯期實體化** ——
//         getMax<int> 與 getMax<double> 會產生兩份獨立且各自最佳化的機器碼，
//         執行期零成本。Java 用 type erasure，
//         泛型資訊在編譯後被抹除，執行期其實都是 Object，
//         需要裝箱與型別轉換，有執行期成本。
//     追問：C++ 這樣做的代價是什麼？
//         → code bloat（每個型別一份程式碼，二進位變大）
//           與編譯時間變長；而且錯誤訊息出現在實體化時，
//           往往很難讀（C++20 的 concepts 就是為了改善這點）。
//
// ⚠️ 陷阱 1. 「C++ 這個名字」照運算子語意讀，值是多少？
//     答：還是 **C**。因為 ++ 在這裡是**後置**遞增，
//         語意是「先回傳舊值，再遞增」。
//         真要表達「先進化再使用」，該寫 ++C。
//     為什麼會錯：大家把 C++ 直接讀成「C 加強版」，
//         沒有把它當成一個真的運算式來解析。
//         這題考的是對前置／後置遞增語意的掌握，不是歷史。
//
// ⚠️ 陷阱 2. 既然 C++ 支援四種範式，那把它們混在同一個函式裡是好事嗎？
//     答：**不是**。多範式的價值在於「依問題性質選擇最合適的工具」，
//         而非「同時使用全部」。同一個模組內風格不一致，
//         會讓維護者必須同時載入多套心智模型，
//         反而比只用一種範式更難懂。
//     為什麼會錯：把「語言支援」誤解成「應該使用」。
//         能力是選項，不是義務 —— 這在 C++ 尤其重要，
//         因為它給的選項比任何主流語言都多。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <numeric>
#include <iterator>

// ===== 重點一：C++ 的誕生背景 =====
//
// 歷史時間線：
//   1972年：Dennis Ritchie 在貝爾實驗室創造了 C 語言
//   1979年：Bjarne Stroustrup 開始開發「C with Classes」
//   1983年：正式更名為 C++（Rick Mascitti 建議）
//   1985年：第一版《The C++ Programming Language》出版
//   1998年：C++98 成為第一個 ISO 標準
//   2011年：C++11 發布（現代 C++ 的起點）
//   2014年：C++14 發布
//   2017年：C++17 發布
//   2020年：C++20 發布
//   2023年：C++23 發布
//
// 創造動機：
//   Bjarne Stroustrup（丹麥電腦科學家）在博士期間使用 Simula 語言，
//   Simula 是世界上第一個支援「類別」和「物件」的語言。
//   - Simula 的優點：程式結構清晰，易於組織大型程式
//   - Simula 的缺點：執行效率太慢
//   - C 語言：執行效率極高，但缺乏組織大型程式的機制
//
//   Stroustrup 的核心想法：
//   「能不能把 Simula 的組織能力加到 C 語言上，同時保持 C 的效率？」
//   這就是 C++ 誕生的核心動機。


// ===== 重點二：名稱「C++」的由來 =====
//
// 原名「C with Classes」（帶有類別的 C）
// 1983年更名為「C++」
//
// 名稱來自 C 語言的遞增運算子 ++，意思是「C 的下一個版本」：
//   int c = 1;
//   c++;  // c 變成 2，代表「進化後的 C」
//
// 有趣的是：按照 C 語言語義，c++ 是「先使用 c 的值，再遞增」
// 所以有人開玩笑說應該叫「++C」才對！
// 意思是：「C++ 是先以 C 為基礎，然後再進化」


// ===== 重點三：C++ 的四大設計哲學原則 =====

// 原則一：零開銷原則（Zero-overhead Principle）
// 「不為你不用的東西付出代價」
// 如果你不使用某個功能，它不應該對程式產生任何額外開銷
// （無論是執行時間還是記憶體）

// 示範：你可以只用 C 風格，不會有 OOP 的開銷
int simpleAdd(int a, int b) {
    return a + b;  // 純 C 風格函數，零 OOP 開銷
}

// 只有當你選擇使用類別時，才會有類別的機制
class SimpleCalculator {
private:
    int lastResult;  // 只有使用時才有此開銷
public:
    SimpleCalculator() : lastResult(0) {}

    int multiply(int a, int b) {
        lastResult = a * b;
        return lastResult;
    }

    int getLastResult() const {
        return lastResult;
    }
};

// 原則二：直接映射硬體（Direct Mapping to Hardware）
// C++ 的抽象機制不應該阻止程式設計師直接操作硬體
// 保留了 C 的底層能力：指標、直接存取記憶體、位元操作
void demonstrateHardwareMapping() {
    int value = 42;
    int* ptr = &value;       // 指標操作：直接取得記憶體位址
    *ptr = 100;              // 直接修改記憶體中的值

    unsigned char flags = 0b00001111;  // 二進位初始化
    flags = flags << 2;                // 位元左移操作
    // 這些底層操作在 C++ 中完全保留，不受任何限制
}

// 原則三：C 的相容性（C Compatibility）
// C++ 被設計為 C 的超集，大部分合法的 C 程式碼也是合法的 C++ 程式碼
// 這讓已有的大量 C 程式碼可以逐步遷移到 C++，而不是完全重寫

// 原則四：提供高階抽象，但不強制使用
// 類別、繼承、多型、模板等高階功能都是可選的
// 你可以根據需要選擇使用，不需要全部用上


// ===== 重點四：C++ 的多範式程式設計 =====
//
// C++ 支援多種程式設計範式（programming paradigms）：
//
// 1. 程序式（Procedural）：使用函數組織程式碼，類似 C
//    → 函數、流程控制、迴圈
//
// 2. 物件導向（Object-Oriented）：使用類別和物件組織程式碼
//    → class、繼承（inheritance）、多型（polymorphism）
//
// 3. 泛型（Generic）：使用模板寫出適用於多種類型的程式碼
//    → template<typename T>
//
// 4. 函數式（Functional）：使用函數作為一等公民
//    → lambda 表達式、std::function
//
// 這意味著你可以根據問題的性質，選擇最適合的方式來解決問題！


// 範式一：程序式風格（Procedural Style）
// 這就是純 C 風格的函數，沒有任何物件導向的元素
// C++ 完全支援這種寫法
int proceduralAdd(int a, int b) {
    return a + b;
}

// 範式二：物件導向風格（Object-Oriented Style）
// 展示：class 關鍵字、private/public 存取控制、建構函數、成員函數
class Calculator {
private:
    int lastResult;  // 私有成員變數，外界無法直接存取

public:
    // 建構函數：使用初始化列表初始化成員
    // 初始化列表語法「: lastResult(0)」比在函數體內賦值更高效
    Calculator() : lastResult(0) {}

    int multiply(int a, int b) {
        lastResult = a * b;
        return lastResult;
    }

    // const 成員函數：保證不會修改物件的狀態
    int getLastResult() const {
        return lastResult;
    }
};

// 範式三：泛型風格（Generic Style）
// template<typename T> 讓函數可以接受任何類型
// 只要該類型支援 > 運算子即可
// 這樣就不需要為 int、double、long 等各自寫一個版本
template<typename T>
T getMax(T a, T b) {
    return (a > b) ? a : b;
}

// 範式四：函數式風格（Functional Style）
// Lambda 表達式是一個「匿名函數」，可以直接傳遞給其他函數使用
// 語法：[捕獲列表](參數列表) { 函數體 }
// 範例：[](int a, int b) { return a < b; }
// 用於 std::sort 的比較函數

// ===== 主程式：展示所有範式 =====

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 136. Single Number
//   題目：一個整數陣列中，除了一個元素只出現一次外，其餘每個元素都出現兩次。
//         找出那個只出現一次的元素（要求線性時間、常數額外空間）。
//   為什麼用到本主題：這題是展示**同一個問題、四種範式**的絕佳素材 ——
//         解法核心都是「把所有元素 XOR 起來」（a^a=0、a^0=a），
//         但用四種風格寫出來，讀起來完全不同。
//         這正是本課「C++ 是多範式語言」最具體的證據：
//         語言不強迫你選邊站，四種寫法都是道地的 C++。
//   複雜度：四種寫法皆為時間 O(n)、額外空間 O(1)。
// -----------------------------------------------------------------------------

// 【範式一：程序式】最接近 C —— 明確的索引迴圈，資料與邏輯分離
int singleNumber_procedural(const int* nums, int n) {
    int result = 0;
    for (int i = 0; i < n; ++i) {
        result ^= nums[i];
    }
    return result;
}

// 【範式二：物件導向】把狀態與行為封裝起來，可分次餵資料
class XorAccumulator {
    int m_acc = 0;
public:
    XorAccumulator& feed(int v) { m_acc ^= v; return *this; }
    int result() const { return m_acc; }
    void reset() { m_acc = 0; }
};

int singleNumber_oop(const std::vector<int>& nums) {
    XorAccumulator acc;
    for (int v : nums) acc.feed(v);
    return acc.result();
}

// 【範式三：泛型】一份程式碼適用所有支援 ^ 的整數型別，編譯期各自實體化
template <typename T>
T singleNumber_generic(const std::vector<T>& nums) {
    T result = T{};
    for (const T& v : nums) result ^= v;
    return result;
}

// 【範式四：函數式】把「怎麼合併」當成值傳進演算法，沒有可見的迴圈與可變狀態
int singleNumber_functional(const std::vector<int>& nums) {
    return std::accumulate(nums.begin(), nums.end(), 0,
                           [](int acc, int v) { return acc ^ v; });
}

// -----------------------------------------------------------------------------
// 【日常實務範例】伺服器日誌分析：依情境選擇範式，而不是全部混用
//   情境：從一批 access log 中找出「回應時間超過門檻的請求」，
//         並統計平均回應時間。這是每個後端工程師都寫過的東西。
//   為什麼用到本主題：這裡刻意示範「**選擇**範式」而非「炫技混用」——
//     - LogEntry 用 struct（純資料聚合，沒有不變量）；
//     - 篩選與統計用**函數式**風格（copy_if / accumulate + lambda），
//       因為這類「轉換一份資料成另一份」的工作，函數式最簡潔且不易出錯；
//     - 而「維護跨呼叫的統計狀態」用**物件導向**（LatencyStats），
//       因為那需要一個有生命週期、有不變量的物件。
//   結論：範式的選擇取決於「這段邏輯有沒有需要維護的狀態」。
//         無狀態的資料轉換 → 函數式；有狀態的累積 → 物件導向。
// -----------------------------------------------------------------------------
struct LogEntry {              // 純資料聚合 → struct
    std::string path;
    int         status = 0;
    double      latencyMs = 0.0;
};

// 函數式風格：輸入一份資料，輸出一份新資料，不修改任何外部狀態
std::vector<LogEntry> findSlowRequests(const std::vector<LogEntry>& logs, double thresholdMs) {
    std::vector<LogEntry> slow;
    std::copy_if(logs.begin(), logs.end(), std::back_inserter(slow),
                 [thresholdMs](const LogEntry& e) { return e.latencyMs > thresholdMs; });
    return slow;
}

double averageLatency(const std::vector<LogEntry>& logs) {
    if (logs.empty()) return 0.0;
    double total = std::accumulate(logs.begin(), logs.end(), 0.0,
                                   [](double acc, const LogEntry& e) { return acc + e.latencyMs; });
    return total / static_cast<double>(logs.size());
}

// 物件導向風格：需要跨多次呼叫維護狀態，且有不變量（count 與 total 必須同步）
class LatencyStats {
    double m_total = 0.0;
    double m_max   = 0.0;
    int    m_count = 0;
public:
    void add(double ms) {
        m_total += ms;
        if (ms > m_max) m_max = ms;
        ++m_count;
    }
    double average() const { return m_count == 0 ? 0.0 : m_total / m_count; }
    double peak()    const { return m_max; }
    int    count()   const { return m_count; }
};

int main() {
    std::cout << "=== C++ 多範式程式設計展示 ===" << std::endl;
    std::cout << std::endl;

    // --------------------------------------------------
    // 1. 程序式風格
    // --------------------------------------------------
    std::cout << "[程序式風格]" << std::endl;
    // 直接呼叫函數，沒有任何物件的概念
    int sum = proceduralAdd(3, 5);
    std::cout << "proceduralAdd(3, 5) = " << sum << std::endl;
    std::cout << std::endl;

    // --------------------------------------------------
    // 2. 物件導向風格
    // --------------------------------------------------
    std::cout << "[物件導向風格]" << std::endl;
    // 建立物件（Calculator 的實例）
    Calculator calc;
    // 透過物件呼叫成員函數
    int product = calc.multiply(4, 6);
    std::cout << "calc.multiply(4, 6) = " << product << std::endl;
    std::cout << "calc.getLastResult() = " << calc.getLastResult() << std::endl;
    std::cout << std::endl;

    // --------------------------------------------------
    // 3. 泛型風格
    // --------------------------------------------------
    std::cout << "[泛型風格]" << std::endl;
    // 同一個函數模板，自動推導類型
    // 當傳入 int 時，T 被推導為 int
    std::cout << "getMax(10, 20) = " << getMax(10, 20) << std::endl;
    // 當傳入 double 時，T 被推導為 double
    std::cout << "getMax(3.14, 2.71) = " << getMax(3.14, 2.71) << std::endl;
    // 甚至可以用於字元比較
    std::cout << "getMax('a', 'z') = " << getMax('a', 'z') << std::endl;
    std::cout << std::endl;

    // --------------------------------------------------
    // 4. 函數式風格
    // --------------------------------------------------
    std::cout << "[函數式風格]" << std::endl;
    std::vector<int> numbers = {5, 2, 8, 1, 9, 3};

    // Lambda 表達式：[](int a, int b) { return a < b; }
    // 這是一個「匿名函數」，作為排序的比較準則
    // 意義：當 a < b 時返回 true，表示 a 排在 b 前面（升序）
    std::sort(numbers.begin(), numbers.end(), [](int a, int b) {
        return a < b;  // 升序排列
    });

    std::cout << "排序後的數字: ";
    // 範圍 for 迴圈（C++11）：遍歷容器的每個元素
    for (int n : numbers) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
    std::cout << std::endl;

    // --------------------------------------------------
    // 5. 零開銷原則示範
    // --------------------------------------------------
    std::cout << "[零開銷原則示範]" << std::endl;
    // 只用程序式，沒有 OOP 開銷
    std::cout << "simpleAdd(7, 8) = " << simpleAdd(7, 8) << std::endl;

    // 使用 OOP，但只在需要時才付出代價
    SimpleCalculator sc;
    std::cout << "sc.multiply(3, 7) = " << sc.multiply(3, 7) << std::endl;
    std::cout << std::endl;

    // --------------------------------------------------
    // 6. 硬體映射示範
    // --------------------------------------------------
    std::cout << "[硬體映射示範]" << std::endl;
    int value = 42;
    int* ptr = &value;
    std::cout << "value = " << value << std::endl;
    std::cout << "ptr 指向位址: " << ptr << std::endl;
    *ptr = 100;  // 透過指標直接修改記憶體
    std::cout << "透過指標修改後 value = " << value << std::endl;

    unsigned char flags = 0b00001111;  // 二進位：00001111 = 15
    std::cout << "flags 初始值: " << (int)flags << std::endl;
    flags = flags << 2;  // 左移兩位：00111100 = 60
    std::cout << "左移兩位後 flags: " << (int)flags << std::endl;


    std::cout << "\n[LeetCode 136. Single Number —— 同一題的四種範式]" << std::endl;
    std::vector<int> nums{4, 1, 2, 1, 2};
    std::cout << "  輸入: {4, 1, 2, 1, 2}" << std::endl;
    std::cout << "  程序式  singleNumber_procedural = "
              << singleNumber_procedural(nums.data(), static_cast<int>(nums.size())) << std::endl;
    std::cout << "  物件導向 singleNumber_oop        = " << singleNumber_oop(nums) << std::endl;
    std::cout << "  泛型     singleNumber_generic<int>= " << singleNumber_generic(nums) << std::endl;
    std::cout << "  函數式   singleNumber_functional = " << singleNumber_functional(nums) << std::endl;
    std::cout << "  -> 四種寫法結果相同，都是道地的 C++" << std::endl;

    std::vector<long long> big{7LL, 3LL, 7LL};
    std::cout << "  泛型同一份程式碼也吃 long long: " << singleNumber_generic(big) << std::endl;

    std::cout << "\n[日常實務：日誌分析 —— 依情境選擇範式]" << std::endl;
    std::vector<LogEntry> logs{
        {"/api/users",   200,  42.5},
        {"/api/report",  200, 1250.0},
        {"/api/health",  200,   8.0},
        {"/api/search",  500,  980.3},
        {"/api/login",   200,  120.7},
    };

    std::cout << "  全部 " << logs.size() << " 筆，平均延遲 "
              << averageLatency(logs) << "ms" << std::endl;

    std::cout << "  超過 500ms 的慢請求（函數式篩選）:" << std::endl;
    for (const LogEntry& e : findSlowRequests(logs, 500.0)) {
        std::cout << "    " << e.path << "  status=" << e.status
                  << "  " << e.latencyMs << "ms" << std::endl;
    }

    LatencyStats stats;                       // 物件導向：跨呼叫維護狀態
    for (const LogEntry& e : logs) stats.add(e.latencyMs);
    std::cout << "  累積統計（物件導向）: 筆數=" << stats.count()
              << " 平均=" << stats.average()
              << "ms 尖峰=" << stats.peak() << "ms" << std::endl;

    return 0;
}

/*
 * ============================================================
 * 本課重點回顧表
 * ============================================================
 *
 * 概念              說明
 * ────────────────  ─────────────────────────────────────────
 * C++ 創造者        Bjarne Stroustrup（1979年開始開發）
 * 設計動機          結合 C 的效率和 Simula 的組織能力
 * 名稱由來          ++ 是 C 的遞增運算子，表示「C 的進化」
 * 零開銷原則        不使用的功能不產生開銷
 * 硬體映射          保留指標、位元操作等底層能力
 * C 相容性          大部分 C 程式碼也是合法的 C++ 程式碼
 * 多範式            支援程序式、物件導向、泛型、函數式
 *
 * ============================================================
 * 思考問題
 * ============================================================
 * 1. 為什麼 Stroustrup 選擇在 C 的基礎上擴展，而不是創造全新語言？
 *    → 保持向後相容性，讓已有的大量 C 程式碼可以繼續使用
 *    → 繼承 C 的高效能和底層控制能力
 *
 * 2. 「零開銷原則」對程式設計師有什麼實際意義？
 *    → 可以按需要選擇使用功能，不用為未使用的功能付代價
 *    → 適合嵌入式系統等資源受限的環境
 *
 * 3. 多範式語言有什麼優點和缺點？
 *    → 優點：靈活，可以選擇最適合的方式解決問題
 *    → 缺點：學習曲線較陡，可能導致風格不一致
 * ============================================================
 */

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// ★ 注意：「硬體映射示範」中的 0x... 位址每次執行都不同
//    （stack 位置 + ASLR），下面是本機某一次的實測值。

// === 預期輸出 ===
//
// === C++ 多範式程式設計展示 ===
// 
// [程序式風格]
// proceduralAdd(3, 5) = 8
// 
// [物件導向風格]
// calc.multiply(4, 6) = 24
// calc.getLastResult() = 24
// 
// [泛型風格]
// getMax(10, 20) = 20
// getMax(3.14, 2.71) = 3.14
// getMax('a', 'z') = z
// 
// [函數式風格]
// 排序後的數字: 1 2 3 5 8 9
// 
// [零開銷原則示範]
// simpleAdd(7, 8) = 15
// sc.multiply(3, 7) = 21
// 
// [硬體映射示範]
// value = 42
// ptr 指向位址: 0x7ffd4bb99958
// 透過指標修改後 value = 100
// flags 初始值: 15
// 左移兩位後 flags: 60
// 
// [LeetCode 136. Single Number —— 同一題的四種範式]
//   輸入: {4, 1, 2, 1, 2}
//   程序式  singleNumber_procedural = 4
//   物件導向 singleNumber_oop        = 4
//   泛型     singleNumber_generic<int>= 4
//   函數式   singleNumber_functional = 4
//   -> 四種寫法結果相同，都是道地的 C++
//   泛型同一份程式碼也吃 long long: 3
// 
// [日常實務：日誌分析 —— 依情境選擇範式]
//   全部 5 筆，平均延遲 480.3ms
//   超過 500ms 的慢請求（函數式篩選）:
//     /api/report  status=200  1250ms
//     /api/search  status=500  980.3ms
//   累積統計（物件導向）: 筆數=5 平均=480.3ms 尖峰=1250ms
