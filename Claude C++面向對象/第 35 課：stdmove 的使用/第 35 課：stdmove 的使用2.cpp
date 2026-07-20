// =============================================================================
//  第 35 課：std::move 的使用（2） — 綜合演練:兩個重載 vs 一條移動路徑
// =============================================================================
//
// 【主題資訊 Information】
//   本檔示範 std::move 在一個「小型物品系統」中的完整生命週期:
//     GameItem  — 帶 std::string 成員的資源型別,五個特殊成員函式都手動實作以便觀察
//     Inventory — 用 const T& / T&& 兩個重載,把拷貝與移動路徑分開
//   標準版本：C++11 起(移動語意);本檔以 C++17 編譯,因此涉及
//             prvalue 的複製省略是「標準強制」而非最佳化(見注意事項 3)。
//   標頭檔  ：<utility>(std::move)、<vector>、<string>
//
// 【詳細解釋 Explanation】
//
// 【1. addItem 的兩個重載:分流的關鍵在 value category】
//     void addItem(const GameItem& item);   // 左值走這裡 → 拷貝
//     void addItem(GameItem&& item);        // 右值走這裡 → 移動
//   呼叫端寫什麼決定走哪一條:
//     bag.addItem(sword);                   // sword 有名字,是左值 → 拷貝路徑
//     bag.addItem(std::move(shield));       // 轉型成 xvalue → 移動路徑
//     bag.addItem(GameItem("Thunder", 70)); // 臨時物件是 prvalue → 移動路徑
//   注意第三種寫法「不需要」std::move —— 臨時物件本來就沒有名字,
//   編譯器不必問就能偷。對它再寫一次 std::move 是多餘的。
//
// 【2. T&& 參數本身是左值 — 移動路徑裡為什麼還要再寫一次 std::move】
//   void addItem(GameItem&& item) {
//       m_items.push_back(std::move(item));   // ← 這個 std::move 不能省
//   }
//   item 的「型別」是右值引用,但 item 這個「運算式」是具名的,因此是左值。
//   如果不寫 std::move,push_back 會選到 const T& 重載 → 又變成拷貝。
//   規則記法:有名字的東西一律是左值,型別上有幾個 && 都一樣。
//
// 【3. takeLastItem:從容器搬走元素的標準寫法】
//       GameItem item = std::move(m_items.back());  // ① 先把資源搬出來
//       m_items.pop_back();                         // ② 再讓空殼被銷毀
//       return item;                                // ③ 交給 NRVO,不要寫 move
//   三個步驟的順序都不能動:
//     ①② 顛倒 → 存取已銷毀的元素,未定義行為。
//     ③ 若寫成 return std::move(item); → NRVO 被破壞,反而多一次移動建構。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 這個類別違反了 Rule of Zero,是刻意的
//   GameItem 的成員只有 std::string 和 int,兩者都能正確地自我複製與搬移,
//   因此「什麼特殊成員函式都不寫」(Rule of Zero)才是正式產品該有的寫法 ——
//   編譯器生成的版本又快又正確。本檔全部手寫出來,唯一的理由是要在每條路徑上
//   印一行字,讓學習者看見哪一條被走到。真實專案請不要照抄這個結構。
//
// (B) 為什麼 move ctor 裡要寫 other.m_power = 0
//   m_name 是 std::string,被 std::move 之後由 string 自己負責把來源掏空。
//   但 m_power 是 int —— 內建型別的「移動」就是複製,來源不會自動歸零。
//   若你希望被移動後的物件有明確狀態,就得自己動手。這也帶出一個重點:
//   moved-from 物件的狀態,能保證多少,取決於「誰寫的移動邏輯」。
//   本檔 main 裡因此只印 power(我們自己歸零、有保證),不印 name(標準不保證)。
//
// (C) 為什麼 addItem 兩個重載會在參數變多時失控
//   兩個參數就要 4 個重載,三個參數要 8 個。這就是上一份檔案示範的
//   pass-by-value + move 想解決的問題:用「多一次移動」換掉指數級的重載爆炸。
//   兩種寫法都對,取捨點在於該型別的移動成本。
//
// 【注意事項 Pay Attention】
//   1. 本檔沒有 reserve(),vector 擴容時會把既有元素整批移動,
//      因此輸出中會出現「額外」的 [移動] 行。擴容倍率是實作定義的:
//      libstdc++ 為 2 倍(1→2→4)。這不是 bug,是真實世界的成本。
//   2. 被移動後的物件不可再讀其值(可解構、可重新賦值)。本檔輸出刻意
//      不印 moved-from 的字串內容,理由見 main 內註解。
//   3. bag.addItem(GameItem("Thunder Staff", 70)) 中,那個臨時物件在 C++17
//      是「保證省略複製」的:標準規定 prvalue 直接在參數位置初始化,
//      不存在「先建一個再搬進去」。所以你只會看到一次 [建構],不會看到多餘的搬移。
//   4. GameItem 的 move ctor/assignment 都標了 noexcept —— 少了它,
//      vector 擴容會改走拷貝路徑,本檔的輸出會完全不同。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】重載分流與移動路徑
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. void addItem(GameItem&& item) 裡面,為什麼還要寫 push_back(std::move(item))?
//        item 的型別不是已經是右值引用了嗎?
//     答：因為「型別」和「value category」是兩件事。item 是具名參數,
//         凡是有名字的運算式都是左值,即使它的型別是 GameItem&&。
//         不加 std::move 的話 push_back 會選到 const T& 重載,靜靜地拷貝。
//     追問：這個設計是刻意的嗎?
//         → 是。如果 T&& 參數自動保持右值性,函式內第一次使用它就會把它掏空,
//           第二次使用就拿到空殼了。強制你明確寫出 std::move,
//           等於強制你標示「這是最後一次使用」。
//
// 🔥 Q2. bag.addItem(GameItem("Thunder Staff", 70)) 會產生幾次建構、幾次移動?
//     答：臨時物件走 T&& 重載。C++17 保證複製省略,那個 prvalue 直接在
//         參數的位置上建構,所以只有 1 次 [建構];接著函式內的
//         push_back(std::move(item)) 產生 1 次 [移動]進 vector。
//         若此時觸發擴容,還會多出「把既有元素搬到新記憶體」的移動。
//     追問：這裡需要寫 std::move(GameItem(...)) 嗎?
//         → 不需要,而且有害。臨時物件已經是 prvalue,多包一層 std::move
//           會把 prvalue 變成 xvalue,反而讓 C++17 的保證複製省略失效。
//
// ⚠️ 陷阱. takeLastItem() 最後寫 return std::move(item); 是不是更有效率?
//     答：不是,反而更慢。return item; 可以觸發 NRVO —— 編譯器直接把 item
//         建構在呼叫端的回傳位置上,搬移次數是 0。加了 std::move 之後,
//         回傳運算式變成右值引用,NRVO 的條件被破壞,強制多做一次移動建構。
//     為什麼會錯：腦中的比較對象錯了。以為在比「移動 vs 拷貝」,
//         實際上在比「移動 vs 完全不動」。C++11 起,回傳局部變數時
//         編譯器本來就會先當右值處理,std::move 完全是多餘的。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【本檔不加 LeetCode 範例的理由】
//   本檔是物件生命週期的觀察實驗(哪條重載被選中、搬了幾次),
//   不含任何演算法。LeetCode 衡量的是複雜度,硬掛一題會模糊焦點。
//   本課的 LeetCode 應用(1656. Design an Ordered Stream)放在 summary.cpp。
//
// 驗證：valgrind --leak-check=full ./lesson35b

#include <iostream>
#include <string>
#include <vector>
#include <utility>

class GameItem {
private:
    std::string m_name;
    int m_power;

public:
    GameItem(std::string name, int power)
        : m_name(std::move(name))   // move 進成員
        , m_power(power)
    {
        std::cout << "  [建構] " << m_name << " (power=" << m_power << ")\n";
    }

    // Rule of Zero：成員都是 string 和 int，不需要自定義任何特殊函數
    // 但為了觀察，加上追蹤訊息：

    GameItem(const GameItem& other)
        : m_name(other.m_name), m_power(other.m_power) {
        std::cout << "  [拷貝] " << m_name << "\n";
    }

    GameItem(GameItem&& other) noexcept
        : m_name(std::move(other.m_name)), m_power(other.m_power) {
        other.m_power = 0;
        std::cout << "  [移動] " << m_name << "\n";
    }

    GameItem& operator=(const GameItem& other) {
        m_name = other.m_name;
        m_power = other.m_power;
        std::cout << "  [拷貝賦值] " << m_name << "\n";
        return *this;
    }

    GameItem& operator=(GameItem&& other) noexcept {
        m_name = std::move(other.m_name);
        m_power = other.m_power;
        other.m_power = 0;
        std::cout << "  [移動賦值] " << m_name << "\n";
        return *this;
    }

    ~GameItem() = default;

    const std::string& name() const { return m_name; }
    int power() const { return m_power; }
};

class Inventory {
    std::vector<GameItem> m_items;

public:
    // 用 const T& 和 T&& 重載，分別處理拷貝和移動
    void addItem(const GameItem& item) {
        std::cout << "  addItem (拷貝路徑):\n";
        m_items.push_back(item);
    }

    void addItem(GameItem&& item) {
        std::cout << "  addItem (移動路徑):\n";
        m_items.push_back(std::move(item));
    }

    // 移出最後一個物品
    GameItem takeLastItem() {
        GameItem item = std::move(m_items.back());  // 從 vector 移出
        m_items.pop_back();
        return item;  // NRVO 或隱式移動
    }

    void print() const {
        std::cout << "  背包 (" << m_items.size() << " 個物品): ";
        for (const auto& item : m_items) {
            std::cout << "[" << item.name() << " +" << item.power() << "] ";
        }
        std::cout << "\n";
    }
};

int main() {
    std::cout << "===== 建立物品 =====\n";
    GameItem sword("Fire Sword", 50);
    GameItem shield("Ice Shield", 30);

    Inventory bag;

    std::cout << "\n===== 拷貝加入背包 =====\n";
    bag.addItem(sword);       // sword 是左值 → 拷貝
    bag.print();
    std::cout << "  sword 仍然存在：" << sword.name() << "\n";

    std::cout << "\n===== 移動加入背包 =====\n";
    bag.addItem(std::move(shield));  // 移動
    bag.print();
    // ★ 刻意不印出 shield.name() 的內容:被移動後的物件處於
    //   valid but unspecified state,標準不保證它是空字串。
    //   但 m_power 是我們自己在 move ctor 裡明確歸零的,那才是有保證的值。
    std::cout << "  shield 已被移動：name 為 valid but unspecified（不印）"
              << "，power 已由我們的 move ctor 明確歸零 = " << shield.power() << "\n";

    std::cout << "\n===== 直接建構加入 =====\n";
    bag.addItem(GameItem("Thunder Staff", 70));  // 暫時物件 → 移動
    bag.print();

    std::cout << "\n===== 取出最後一個物品 =====\n";
    GameItem taken = bag.takeLastItem();
    std::cout << "  取出：" << taken.name() << " +" << taken.power() << "\n";
    bag.print();

    std::cout << "\n===== 結束 =====\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -g "第 35 課：stdmove 的使用2.cpp" -o lesson35b

// ─────────────────────────────────────────────────────────────────────────────
// 【輸出但書】
//  1. 「[移動] Fire Sword」「[移動] Ice Shield」這些看似多餘的行,來自
//     vector 擴容 —— 本檔刻意不呼叫 reserve(),讓真實成本現形。
//     擴容倍率是實作定義的:libstdc++ 為 2 倍(容量 1→2→4),
//     所以第 2 次加入搬 1 個舊元素、第 3 次搬 2 個。換 MSVC(1.5 倍)行數會不同。
//  2. 「直接建構加入」只有 1 次 [建構]:C++17 保證 prvalue 複製省略,
//     臨時物件直接在參數位置上建構,不存在額外的搬移。
//  3. 取出物品時只有 1 次 [移動]:return item; 觸發 NRVO,回傳那一次被省略。
//  4. shield 被移動後只印 power(我們在 move ctor 裡明確歸零、有保證),
//     不印 name(std::string 的 moved-from 是 valid but unspecified)。
//  5. 以下為本機 g++ 15.2.0 (Ubuntu 26.04) 連續執行 5 次、內容完全相同的結果。
//     唯一的差異:程式實際輸出中「背包 (N 個物品): [...] 」各行尾端帶有空白字元
//     (print() 每印一個物品就補一個空白),下方為避免行尾空白已將其去除。
// ─────────────────────────────────────────────────────────────────────────────

// === 預期輸出 ===
// ===== 建立物品 =====
//   [建構] Fire Sword (power=50)
//   [建構] Ice Shield (power=30)
//
// ===== 拷貝加入背包 =====
//   addItem (拷貝路徑):
//   [拷貝] Fire Sword
//   背包 (1 個物品): [Fire Sword +50]
//   sword 仍然存在：Fire Sword
//
// ===== 移動加入背包 =====
//   addItem (移動路徑):
//   [移動] Ice Shield
//   [移動] Fire Sword
//   背包 (2 個物品): [Fire Sword +50] [Ice Shield +30]
//   shield 已被移動：name 為 valid but unspecified（不印），power 已由我們的 move ctor 明確歸零 = 0
//
// ===== 直接建構加入 =====
//   [建構] Thunder Staff (power=70)
//   addItem (移動路徑):
//   [移動] Thunder Staff
//   [移動] Fire Sword
//   [移動] Ice Shield
//   背包 (3 個物品): [Fire Sword +50] [Ice Shield +30] [Thunder Staff +70]
//
// ===== 取出最後一個物品 =====
//   [移動] Thunder Staff
//   取出：Thunder Staff +70
//   背包 (2 個物品): [Fire Sword +50] [Ice Shield +30]
//
// ===== 結束 =====
