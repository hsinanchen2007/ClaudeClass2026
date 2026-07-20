// =============================================================================
//  第 18 課：對象的生命週期 3  —  控制結構中的作用域（if / for 迴圈）
// =============================================================================
//
// 【主題資訊 Information】
//   主題：if、for、while 等控制結構所隱含的作用域，以及物件在其中的
//         建構/解構時機。
//   關鍵規則：迴圈本體是一個作用域，每一次迭代都會「重新進入」它，
//             因此本體內宣告的物件是「每輪建構一次、每輪解構一次」。
//   標準版本：if/for 的初始化區宣告變數是 C++98 起就有；
//             C++17 另外新增了 if/switch 的「初始化敘述」語法
//             （if (auto x = f(); x > 0) ...）。
//   標頭檔：<iostream>、<string>
//   複雜度：n 輪迴圈 → n 次建構 + n 次解構。若物件昂貴，這就是效能熱點。
//
// 【詳細解釋 Explanation】
//
// 【1. 迴圈本體是作用域，不是「一段程式碼」】
//   最容易誤解的一點：
//       for (int i = 0; i < 3; i++) {
//           Tracker t("...");        // 每輪都是「全新的 t」
//       }
//   很多人以為 t 只建構一次、迴圈中被重複使用。實際上大括號劃出的是
//   一個作用域，每次迭代進入時建構、離開時（含 continue、break）解構。
//   本檔輸出會清楚看到 [+] [-] [+] [-] [+] [-] 交錯出現，
//   而不是三個 [+] 之後才三個 [-]。
//
// 【2. 這件事的效能意涵（很實際）】
//   若迴圈本體宣告的是昂貴物件（例如 std::string、std::vector、
//   資料庫連線），那就是每輪都要配置與釋放一次記憶體：
//       for (...) { std::string buf; buf = makeLine(i); use(buf); }   // n 次配置
//   把宣告提到迴圈外，就能重複利用已配置的容量：
//       std::string buf;
//       for (...) { buf.clear(); buf += makeLine(i); use(buf); }      // 多半只配置一次
//   ★ 但這是有代價的取捨：把宣告提到外面會擴大作用域、增加「殘留舊值」
//     的風險。判準是——只有在量測證明它是熱點時才這樣做，
//     否則優先維持最小作用域。本檔實務範例會實際數出配置次數。
//
// 【3. for 的三個部分各屬於哪個作用域？】
//       for (init; cond; inc) body
//   init 宣告的變數，其作用域涵蓋 cond、inc 與 body，
//   但迴圈結束後就消失（這是 C++ 與 C89 最明顯的差異之一）。
//   而 body 內宣告的變數，作用域只有 body 本身，每輪重來。
//   所以下面兩者的生命週期完全不同：
//       for (Tracker t("A"); ...; ...)  { }   // t 只建構一次，整個迴圈共用
//       for (...) { Tracker t("B"); }         // t 每輪各建構一次
//
// 【4. if 條件式中的宣告】
//       if (bool condition = check()) { ... }
//   condition 的作用域涵蓋 if 本體與對應的 else 區塊，if 結束後消失。
//   這個語法的價值在於：把「取得值」與「判斷值」綁在一起，
//   避免變數在不需要它的地方繼續存在。
//   C++17 更進一步允許初始化敘述與條件分離：
//       if (auto it = m.find(key); it != m.end()) { use(it->second); }
//   讓 it 的作用域嚴格限制在 if/else 內——這是現代 C++ 很常見的寫法。
//
// 【概念補充 Concept Deep Dive】
//   ● 編譯器不會「每輪重新配置堆疊空間」。
//     迴圈本體的區域物件通常在函式開場時就已在堆疊上保留好位置，
//     每輪只是在同一塊記憶體上重新執行建構/解構。
//     所以「每輪重建」的成本來自建構/解構函式本身（例如 heap 配置），
//     不是來自堆疊指標的移動。
//   ● continue 也會解構：它跳到迭代結尾，等同離開本體作用域。
//     break 亦然。這保證了無論用哪種方式跳出，資源都不會洩漏。
//   ● range-based for（C++11）背後展開成一般的 for 迴圈，
//     其中 `for (auto x : v)` 的 x 也是每輪建構/解構一次——
//     這正是為什麼建議寫 `for (const auto& x : v)` 以避免每輪複製。
//
// 【注意事項 Pay Attention】
//   1. 迴圈本體內的物件是「每輪重建」，不是重複使用；昂貴物件要注意成本。
//   2. 但不要為了效能就無條件把宣告提到迴圈外——那會擴大作用域、
//      引入殘留狀態的風險。先量測，確認是熱點再改。
//   3. for 的 init 區宣告的變數，迴圈結束後就不存在（C++ 與 C89 的差異）。
//   4. continue / break 一樣會觸發本體內物件的解構，不會洩漏。
//   5. range-based for 用 `auto x` 會每輪複製一次；唯讀請用 `const auto&`。
//   6. C++17 的 if 初始化敘述 `if (init; cond)` 是限縮作用域的利器，
//      但需要 -std=c++17（本檔以 C++17 編譯）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】控制結構中的作用域與生命週期
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 迴圈本體內宣告的物件，是每輪重建還是只建一次？
//     答：每輪重建。迴圈本體的大括號劃出一個作用域，每次迭代進入時建構、
//         離開時解構，所以輸出是 [+][-][+][-][+][-] 交錯，
//         不是三個 [+] 之後才三個 [-]。
//     追問：那這樣會不會每輪都動到堆疊指標、很慢？
//         → 不會。堆疊空間通常在函式開場就保留好了，每輪只是在同一塊
//           記憶體上重跑建構/解構。真正的成本來自建構函式本身
//           （例如 std::string 的 heap 配置），不是堆疊操作。
//
// 🔥 Q2. `for (int i = 0; ...)` 的 i，迴圈結束後還能用嗎？
//     答：不能，C++ 中 i 的作用域僅限 for 敘述本身。
//         若需要在迴圈後知道停在哪裡，必須把宣告移到 for 之外。
//         這與 C89 相反——C89 必須在區塊開頭宣告，i 會活到區塊結束。
//     追問：C++17 為 if/switch 新增了什麼相關語法？
//         → 初始化敘述：if (auto it = m.find(k); it != m.end()) { ... }
//           讓 it 的作用域嚴格限制在 if/else 內，離開就消失。
//
// ⚠️ 陷阱. 把物件宣告移到迴圈外，一定比較快嗎？
//     答：不一定，而且這是常被過度套用的「優化」。它只在
//         「建構/解構本身昂貴」時才有意義（例如每輪重新配置堆積記憶體）。
//         對 int 這種平凡型別，移出去不會變快，反而擴大了作用域。
//     為什麼會錯：把「每輪都建構」直接等同於「每輪都很貴」。
//         實際上對 trivially constructible 的型別，建構是零指令；
//         編譯器也可能把整個物件最佳化進暫存器。
//         沒有量測就搬動宣告，通常只是用可讀性換一個不存在的收益——
//         本檔的實務範例會用實際的配置次數，把「有沒有變快」變成數字。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Tracker {
private:
    string name;
public:
    Tracker(const string& n) : name(n) {
        cout << "  [+] " << name << endl;
    }
    ~Tracker() {
        cout << "  [-] " << name << endl;
    }
    void work() const {
        cout << "  [=] " << name << " 工作中" << endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】迴圈內 vs 迴圈外宣告緩衝區：實際數出配置次數
//   情境：把一批感測器讀數格式化成文字行，寫進報表。
//   這是「每輪重建 vs 重複利用」最典型的場景。
//   下面用一個會自報配置次數的緩衝區類別，把差異變成可驗證的數字，
//   而不是憑感覺說「這樣比較快」。
//   注意：這裡計數的是「配置動作的次數」，不是耗時——
//         耗時會因機器與負載而異，次數則是可重現的。
// -----------------------------------------------------------------------------
struct CountingBuffer {
    static int allocCount;      // 全域統計：總共配置了幾次
    string storage;

    CountingBuffer() { ++allocCount; }        // 視為一次配置
    void clear() { storage.clear(); }         // 清內容但保留已配置的容量
    void append(const string& s) { storage += s; }
};
int CountingBuffer::allocCount = 0;

// 寫法 A：緩衝區宣告在迴圈「內」——每輪重建
int formatInsideLoop(const vector<int>& values) {
    CountingBuffer::allocCount = 0;
    for (size_t i = 0; i < values.size(); ++i) {
        CountingBuffer buf;                   // 每輪建構一次
        buf.append("value=");
        buf.append(to_string(values[i]));
    }
    return CountingBuffer::allocCount;
}

// 寫法 B：緩衝區宣告在迴圈「外」——重複利用
int formatOutsideLoop(const vector<int>& values) {
    CountingBuffer::allocCount = 0;
    CountingBuffer buf;                       // 只建構一次
    for (size_t i = 0; i < values.size(); ++i) {
        buf.clear();                          // 重用，但必須自己記得清乾淨
        buf.append("value=");
        buf.append(to_string(values[i]));
    }
    return CountingBuffer::allocCount;
}

// 註：本檔不加 LeetCode 範例。
//     「控制結構的作用域規則」是語言機制，任何 LeetCode 題目都不會因為
//     理解它而改變解法或複雜度；硬掛一題只會失焦，故從缺。
//     本課真正適合對照 LeetCode 的是 2.cpp（巢狀作用域 ↔ 括號配對）。

int main() {
    // ====== if 語句中的對象 ======
    cout << "=== if 語句 ===" << endl;
    if (bool condition = true) {
        Tracker t("if 區塊物件");
        t.work();
        // condition 的作用域涵蓋整個 if 本體，這裡仍看得到它
        cout << "  condition 在 if 本體內仍可見, 值 = "
             << (condition ? "true" : "false") << endl;
    }   // t 在這裡死亡；condition 也在這裡離開作用域
    cout << "  if 區塊已結束\n" << endl;
    
    // ====== for 迴圈中的對象 ======
    cout << "=== for 迴圈 ===" << endl;
    for (int i = 0; i < 3; i++) {
        Tracker t("迴圈物件 #" + to_string(i));
        t.work();
        // t 在每次迭代結束時死亡，下次迭代重新建構
    }
    cout << "  for 迴圈已結束\n" << endl;
    
    // ====== for 初始化部分的對象 ======
    cout << "=== for 初始化區 ===" << endl;
    // 注意：C++ 沒有直接在 for 初始化區放自定義對象的語法
    // 但可以這樣理解等效行為
    {
        Tracker outer("迴圈外圍物件");
        for (int i = 0; i < 2; i++) {
            Tracker inner("迴圈內部 #" + to_string(i));
            inner.work();
        }
    }
    cout << "  完成\n" << endl;

    // ====== continue / break 也會解構 ======
    cout << "=== continue 與 break 一樣會解構 ===" << endl;
    for (int i = 0; i < 4; i++) {
        Tracker t("第 " + to_string(i) + " 輪");
        if (i == 1) {
            cout << "  (continue)" << endl;
            continue;             // 仍會解構 t
        }
        if (i == 2) {
            cout << "  (break)" << endl;
            break;                // 仍會解構 t
        }
    }
    cout << "  迴圈已離開\n" << endl;

    // ====== C++17 的 if 初始化敘述 ======
    cout << "=== C++17：if 初始化敘述（限縮作用域）===" << endl;
    {
        vector<int> data = {3, 7, 11};
        // pos 的作用域嚴格限制在 if/else 內，離開就消失
        if (size_t pos = data.size(); pos > 2) {
            cout << "  data 有 " << pos << " 個元素（pos 只在此可見）" << endl;
        } else {
            cout << "  data 只有 " << pos << " 個元素" << endl;
        }
    }

    cout << "\n=== 日常實務：迴圈內 vs 迴圈外宣告（實測配置次數）===" << endl;
    {
        vector<int> values = {10, 20, 30, 40, 50};
        int inside  = formatInsideLoop(values);
        int outside = formatOutsideLoop(values);
        cout << "  處理 " << values.size() << " 筆資料" << endl;
        cout << "  緩衝區宣告在迴圈內：配置 " << inside  << " 次" << endl;
        cout << "  緩衝區宣告在迴圈外：配置 " << outside << " 次" << endl;
        cout << "  → 差異在於「建構次數」，只有當建構本身昂貴時才值得搬動宣告" << endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 18 課：對象的生命週期（Object Lifetime）3.cpp" -o life3

// === 預期輸出 ===
// === if 語句 ===
//   [+] if 區塊物件
//   [=] if 區塊物件 工作中
//   condition 在 if 本體內仍可見, 值 = true
//   [-] if 區塊物件
//   if 區塊已結束
//
// === for 迴圈 ===
//   [+] 迴圈物件 #0
//   [=] 迴圈物件 #0 工作中
//   [-] 迴圈物件 #0
//   [+] 迴圈物件 #1
//   [=] 迴圈物件 #1 工作中
//   [-] 迴圈物件 #1
//   [+] 迴圈物件 #2
//   [=] 迴圈物件 #2 工作中
//   [-] 迴圈物件 #2
//   for 迴圈已結束
//
// === for 初始化區 ===
//   [+] 迴圈外圍物件
//   [+] 迴圈內部 #0
//   [=] 迴圈內部 #0 工作中
//   [-] 迴圈內部 #0
//   [+] 迴圈內部 #1
//   [=] 迴圈內部 #1 工作中
//   [-] 迴圈內部 #1
//   [-] 迴圈外圍物件
//   完成
//
// === continue 與 break 一樣會解構 ===
//   [+] 第 0 輪
//   [-] 第 0 輪
//   [+] 第 1 輪
//   (continue)
//   [-] 第 1 輪
//   [+] 第 2 輪
//   (break)
//   [-] 第 2 輪
//   迴圈已離開
//
// === C++17：if 初始化敘述（限縮作用域）===
//   data 有 3 個元素（pos 只在此可見）
//
// === 日常實務：迴圈內 vs 迴圈外宣告（實測配置次數）===
//   處理 5 筆資料
//   緩衝區宣告在迴圈內：配置 5 次
//   緩衝區宣告在迴圈外：配置 1 次
//   → 差異在於「建構次數」，只有當建構本身昂貴時才值得搬動宣告
