// =============================================================================
//  第 35 課：std::move 的使用（1） — 用計數器把「拷貝 vs 移動」量出來
// =============================================================================
//
// 【主題資訊 Information】
//   template <class T>
//   constexpr std::remove_reference_t<T>&& move(T&& t) noexcept;   // <utility>
//   標準版本：C++11(constexpr 自 C++14,[[nodiscard]] 自 C++20)
//   複雜度  ：std::move 本身 O(1) 且零成本;真正的成本在接收端的 move ctor。
//
//   本檔的教學手法：TrackedString 在每個拷貝/移動路徑上印一行並累加靜態計數器。
//   於是「哪一種寫法會多一次拷貝」不再是口頭宣稱,而是可以數出來的數字。
//   這也是為什麼本檔用「計數」而不是「計時」——計數可重現,計時每次都不同。
//
// 【詳細解釋 Explanation】
//
// 【1. 五個場景各自要證明什麼】
//   場景 1 放棄所有權   ：同樣是初始化,b = a 走拷貝、c = std::move(a) 走移動。
//   場景 2 建構子傳值   ：同一個 Hero 建構子,傳左值 = 2 拷貝 + 2 移動,
//                        傳右值 = 0 拷貝 + 4 移動。一份程式碼吃兩種情況。
//   場景 3 容器插入     ：push_back(拷貝) / push_back(move)(移動) /
//                        emplace_back(原地建構,兩者都是 0)三條路徑的對照。
//   場景 4 從容器取出   ：vec.back() 是左值 → 拷貝;std::move(vec.back()) → 移動。
//   場景 5 swap         ：std::swap 內部是 1 次移動建構 + 2 次移動賦值 = 3 次移動。
//
// 【2. 為什麼場景 3 要先 reserve(3)】
//   vector 容量不足時會配置新記憶體並把既有元素整批搬過去,那些搬移也會被
//   計數器算進去,實驗就被雜訊污染了。reserve 把擴容排除掉,讓輸出只反映
//   「這一次 push_back 本身」的成本。這是設計實驗時的控制變因,不是效能技巧。
//   (擴容倍率是實作定義的:libstdc++ 為 2 倍。)
//
// 【3. 場景 4 的順序不能顛倒】
//   TrackedString b = std::move(vec.back());   // ① 先把資源搬出來
//   vec.pop_back();                            // ② 再讓那個空殼被銷毀
//   反過來寫 —— 先 pop_back() 再存取 back() —— 就是在存取已被銷毀的元素,
//   那是未定義行為。move 只是「授權搬走」,不會延長物件壽命。
//
// 【概念補充 Concept Deep Dive】
//   std::swap 的通用版本長這樣:
//       template <class T> void swap(T& a, T& b) {
//           T tmp = std::move(a);   // ① 移動建構
//           a     = std::move(b);   // ② 移動賦值
//           b     = std::move(tmp); // ③ 移動賦值
//       }
//   所以場景 5 的「3 次移動」不是巧合,而是這個演算法的固定成本。
//   若型別沒有移動能力,同一份程式碼會退化成 3 次拷貝 —— swap 突然變得很貴,
//   這正是「為型別提供 move ctor」最直接的回報。
//
// 【注意事項 Pay Attention】
//   1. 場景 2 中兩個參數誰先建構是 未指定行為(unspecified)。C++17 只保證
//      各引數求值不交錯,不保證順序。本機 g++ 15.2.0 實測為由右至左
//      (先 King 後 Arthur),換編譯器行序會變,但「次數」不變。
//   2. 被移動後的物件處於 valid but unspecified state。本檔各場景刻意都不去
//      印出被移動來源的內容,只印計數 —— 計數是標準保證的,內容不是。
//   3. 移動建構子/移動賦值請一律加 noexcept,否則 vector 擴容時
//      std::move_if_noexcept 會改走拷貝,你的最佳化等於不存在。
//   4. std::move 對 const 物件無效(退化為拷貝)、對沒有 move ctor 的型別
//      也無效,而且兩種情況編譯器都不會警告。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】用計數驗證移動語意
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 場景 2 傳左值時是 2 拷貝 + 2 移動,傳右值時是 0 拷貝 + 4 移動,
//        為什麼右值那條路「移動次數變多了」卻反而比較好?
//     答：因為兩次移動加起來還是比一次拷貝便宜。傳左值時,參數 name 必須從
//         呼叫端的具名變數「拷貝」一份(真的複製字元),再 move 進成員;
//         傳右值時兩段都只是搬指標。比較的標準是「有沒有真的複製資料」,
//         不是「呼叫了幾次特殊成員函式」。
//     追問：那 pass-by-value + move 什麼時候不划算?
//         → 當該型別的 move 和 copy 一樣貴時(例如 std::array<int,1000>,
//           成員是沒有間接層的內嵌陣列),多出來的那次移動就是純虧損,
//           此時該回頭寫 const T& / T&& 兩個重載。
//
// 🔥 Q2. 場景 3 的 emplace_back("Delta") 為什麼拷貝和移動都是 0?
//     答：push_back 需要先有一個物件,再把它拷貝或移動進容器;
//         emplace_back 則是把參數完美轉發到容器記憶體上,直接呼叫建構子
//         就地生出物件,中間根本不存在「另一個物件」,自然沒有東西要搬。
//     追問：那是不是一律用 emplace_back 就好?
//         → 不是。emplace_back 走的是 explicit 建構子也能通過的直接初始化,
//           會讓 v.emplace_back(10) 這種本意是「放一個值」的呼叫,
//           意外呼叫到單參數建構子;已有現成物件時 push_back 語意更清楚。
//
// ⚠️ 陷阱. 場景 4 若寫成 vec.pop_back(); TrackedString b = std::move(vec.back());
//     答：這是未定義行為。pop_back() 已經銷毀了最後一個元素,
//         之後 back() 取到的是容器範圍外的位置。
//     為什麼會錯：直覺把 std::move 當成「先把東西拿在手上」的動作,
//         以為拿了就與容器無關;但 std::move 只是轉型,它不複製、不搬移、
//         也不延長壽命 —— 真正的搬移發生在 b 的建構子裡,那已經太晚了。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【本檔不加 LeetCode 範例的理由】
//   本檔是純粹的「儀器」——用計數器量測特殊成員函式被呼叫的次數,
//   沒有演算法內容。LeetCode 題目衡量的是時間/空間複雜度,
//   硬掛一題上來只會模糊本檔的教學目的。std::move 的 LeetCode 應用
//   (1656. Design an Ordered Stream)放在本課的 summary.cpp。

#include <iostream>
#include <string>
#include <vector>
#include <utility>

// 追蹤用的字串類別
class TrackedString {
private:
    std::string m_data;
    static int s_copyCount;
    static int s_moveCount;

public:
    TrackedString(const char* s = "") : m_data(s) {
        std::cout << "    [建構] \"" << m_data << "\"\n";
    }

    TrackedString(const TrackedString& other) : m_data(other.m_data) {
        ++s_copyCount;
        std::cout << "    [拷貝] \"" << m_data << "\" (累計 "
                  << s_copyCount << " 次拷貝)\n";
    }

    TrackedString(TrackedString&& other) noexcept : m_data(std::move(other.m_data)) {
        ++s_moveCount;
        std::cout << "    [移動] \"" << m_data << "\" (累計 "
                  << s_moveCount << " 次移動)\n";
    }

    TrackedString& operator=(const TrackedString& other) {
        m_data = other.m_data;
        ++s_copyCount;
        std::cout << "    [拷貝賦值] \"" << m_data << "\"\n";
        return *this;
    }

    TrackedString& operator=(TrackedString&& other) noexcept {
        m_data = std::move(other.m_data);
        ++s_moveCount;
        std::cout << "    [移動賦值] \"" << m_data << "\"\n";
        return *this;
    }

    const std::string& str() const { return m_data; }

    static void resetCounters() { s_copyCount = 0; s_moveCount = 0; }
    static void printCounters() {
        std::cout << "    >>> 總計：拷貝 " << s_copyCount
                  << " 次，移動 " << s_moveCount << " 次\n";
    }
};

int TrackedString::s_copyCount = 0;
int TrackedString::s_moveCount = 0;

// 接收按值參數，move 進成員
class Hero {
    TrackedString m_name;
    TrackedString m_title;
public:
    Hero(TrackedString name, TrackedString title)
        : m_name(std::move(name))
        , m_title(std::move(title))
    {}
};

// -----------------------------------------------------------------------------
// 【日常實務範例】待辦工作佇列:取出任務時「搬」而不是「複製」
// 情境：背景工作者從佇列取出任務來執行。任務物件帶著 payload(可能是幾 MB 的
//       請求內容),取出後佇列裡那份就不再需要了 —— 這正是場景 4 的真實版本。
//       pop() 的正確順序:先 std::move(back()) 把資源搬出來,再 pop_back()。
//       這裡用 TrackedString 當 payload,所以取出成本可以直接被計數器量到。
// -----------------------------------------------------------------------------
class TaskQueue {
    std::vector<TrackedString> m_tasks;
public:
    void reserveHint() { m_tasks.reserve(4); }  // 預留空間,排除擴容造成的額外移動
    void submit(TrackedString payload) {        // 傳值 + move:左右值都吃
        m_tasks.push_back(std::move(payload));
    }
    // 取出最後一筆(LIFO);呼叫前必須確認 !empty()
    TrackedString takeOne() {
        TrackedString task = std::move(m_tasks.back());  // ① 先搬走資源
        m_tasks.pop_back();                              // ② 再銷毀空殼
        return task;                                     // 不寫 std::move,留給 NRVO
    }
    bool        empty() const { return m_tasks.empty(); }
    std::size_t size()  const { return m_tasks.size(); }
};

int main() {
    // ===== 場景 1：放棄所有權 =====
    std::cout << "===== 場景 1：放棄所有權 =====\n";
    TrackedString::resetCounters();
    {
        TrackedString a("Excalibur");
        std::cout << "  不用 move：\n";
        TrackedString b = a;                // 拷貝
        std::cout << "  用 move：\n";
        TrackedString c = std::move(a);     // 移動
    }
    TrackedString::printCounters();
    std::cout << "\n";

    // ===== 場景 2：建構函數中 move 進成員 =====
    std::cout << "===== 場景 2：建構函數中 move 進成員 =====\n";

    std::cout << "  --- 傳入左值 ---\n";
    TrackedString::resetCounters();
    {
        TrackedString name("Arthur");
        TrackedString title("King");
        Hero h1(name, title);   // 按值傳入：拷貝建構 name, title → move 進成員
    }
    TrackedString::printCounters();

    std::cout << "\n  --- 傳入右值 ---\n";
    TrackedString::resetCounters();
    {
        TrackedString name("Arthur");
        TrackedString title("King");
        Hero h2(std::move(name), std::move(title));  // 移動建構 → move 進成員
    }
    TrackedString::printCounters();
    std::cout << "\n";

    // ===== 場景 3：push_back 到 vector =====
    std::cout << "===== 場景 3：push_back 到 vector =====\n";
    TrackedString::resetCounters();
    {
        std::vector<TrackedString> vec;
        vec.reserve(3);   // 預留空間，避免擴容干擾

        TrackedString s1("Alpha");
        TrackedString s2("Beta");
        TrackedString s3("Gamma");

        std::cout << "  push_back 拷貝：\n";
        vec.push_back(s1);                // 拷貝

        std::cout << "  push_back 移動：\n";
        vec.push_back(std::move(s2));     // 移動

        std::cout << "  emplace_back：\n";
        vec.emplace_back("Delta");        // 直接建構（零拷貝零移動）
    }
    TrackedString::printCounters();
    std::cout << "\n";

    // ===== 場景 4：從容器移出元素 =====
    std::cout << "===== 場景 4：從容器移出元素 =====\n";
    TrackedString::resetCounters();
    {
        std::vector<TrackedString> vec;
        vec.reserve(3);
        vec.emplace_back("First");
        vec.emplace_back("Second");
        vec.emplace_back("Third");

        std::cout << "  拷貝取出最後一個：\n";
        TrackedString a = vec.back();               // 拷貝

        std::cout << "  移動取出最後一個：\n";
        TrackedString b = std::move(vec.back());    // 移動
        vec.pop_back();
    }
    TrackedString::printCounters();
    std::cout << "\n";

    // ===== 場景 5：swap =====
    std::cout << "===== 場景 5：swap（3 次移動，0 次拷貝）=====\n";
    TrackedString::resetCounters();
    {
        TrackedString x("Left");
        TrackedString y("Right");
        std::cout << "  swap 前：x=\"" << x.str() << "\"  y=\"" << y.str() << "\"\n";
        std::swap(x, y);
        std::cout << "  swap 後：x=\"" << x.str() << "\"  y=\"" << y.str() << "\"\n";
    }
    TrackedString::printCounters();
    std::cout << "\n";

    // ===== 日常實務：工作佇列 =====
    std::cout << "===== 日常實務：TaskQueue（取出時搬而不複製）=====\n";
    TrackedString::resetCounters();
    {
        TaskQueue q;
        q.reserveHint();                       // 排除擴容雜訊，理由同場景 3
        std::cout << "  submit 三筆任務：\n";
        q.submit(TrackedString("job-A"));
        q.submit(TrackedString("job-B"));
        q.submit(TrackedString("job-C"));
        std::cout << "  佇列長度：" << q.size() << "\n";

        std::cout << "  取出一筆執行：\n";
        TrackedString job = q.takeOne();
        std::cout << "  取出的任務：\"" << job.str() << "\"，剩餘 " << q.size() << " 筆\n";
    }
    TrackedString::printCounters();

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -g "第 35 課：stdmove 的使用1.cpp" -o lesson35

// ─────────────────────────────────────────────────────────────────────────────
// 【輸出但書】
//  1. 場景 2「傳入左值」中,Arthur 與 King 誰先被拷貝是 未指定行為(unspecified):
//     C++17 只保證各引數求值不交錯,不保證順序。本機 g++ 15.2.0 實測為由右至左
//     (King 先),MSVC 常見為由左至右。拷貝/移動的「次數」則不受影響。
//  2. 實務範例中 takeOne() 寫的是 return task;(不是 return std::move(task);),
//     因此輸出只看到 1 次移動 —— NRVO 把回傳那一次搬移完全省掉了。
//     若改寫成 return std::move(task); 這裡會多出一次移動,正是本課的反面教材。
//  3. 各場景只印計數,不印被移動來源的內容:moved-from 是
//     valid but unspecified state,其值不是可以寫進教材的固定結果。
//  4. 以下為本機 g++ 15.2.0 (Ubuntu 26.04) 連續執行 5 次、逐位元組相同的結果。
// ─────────────────────────────────────────────────────────────────────────────

// === 預期輸出 ===
// ===== 場景 1：放棄所有權 =====
//     [建構] "Excalibur"
//   不用 move：
//     [拷貝] "Excalibur" (累計 1 次拷貝)
//   用 move：
//     [移動] "Excalibur" (累計 1 次移動)
//     >>> 總計：拷貝 1 次，移動 1 次
//
// ===== 場景 2：建構函數中 move 進成員 =====
//   --- 傳入左值 ---
//     [建構] "Arthur"
//     [建構] "King"
//     [拷貝] "King" (累計 1 次拷貝)
//     [拷貝] "Arthur" (累計 2 次拷貝)
//     [移動] "Arthur" (累計 1 次移動)
//     [移動] "King" (累計 2 次移動)
//     >>> 總計：拷貝 2 次，移動 2 次
//
//   --- 傳入右值 ---
//     [建構] "Arthur"
//     [建構] "King"
//     [移動] "King" (累計 1 次移動)
//     [移動] "Arthur" (累計 2 次移動)
//     [移動] "Arthur" (累計 3 次移動)
//     [移動] "King" (累計 4 次移動)
//     >>> 總計：拷貝 0 次，移動 4 次
//
// ===== 場景 3：push_back 到 vector =====
//     [建構] "Alpha"
//     [建構] "Beta"
//     [建構] "Gamma"
//   push_back 拷貝：
//     [拷貝] "Alpha" (累計 1 次拷貝)
//   push_back 移動：
//     [移動] "Beta" (累計 1 次移動)
//   emplace_back：
//     [建構] "Delta"
//     >>> 總計：拷貝 1 次，移動 1 次
//
// ===== 場景 4：從容器移出元素 =====
//     [建構] "First"
//     [建構] "Second"
//     [建構] "Third"
//   拷貝取出最後一個：
//     [拷貝] "Third" (累計 1 次拷貝)
//   移動取出最後一個：
//     [移動] "Third" (累計 1 次移動)
//     >>> 總計：拷貝 1 次，移動 1 次
//
// ===== 場景 5：swap（3 次移動，0 次拷貝）=====
//     [建構] "Left"
//     [建構] "Right"
//   swap 前：x="Left"  y="Right"
//     [移動] "Left" (累計 1 次移動)
//     [移動賦值] "Right"
//     [移動賦值] "Left"
//   swap 後：x="Right"  y="Left"
//     >>> 總計：拷貝 0 次，移動 3 次
//
// ===== 日常實務：TaskQueue（取出時搬而不複製）=====
//   submit 三筆任務：
//     [建構] "job-A"
//     [移動] "job-A" (累計 1 次移動)
//     [建構] "job-B"
//     [移動] "job-B" (累計 2 次移動)
//     [建構] "job-C"
//     [移動] "job-C" (累計 3 次移動)
//   佇列長度：3
//   取出一筆執行：
//     [移動] "job-C" (累計 4 次移動)
//   取出的任務："job-C"，剩餘 2 筆
//     >>> 總計：拷貝 0 次，移動 4 次
