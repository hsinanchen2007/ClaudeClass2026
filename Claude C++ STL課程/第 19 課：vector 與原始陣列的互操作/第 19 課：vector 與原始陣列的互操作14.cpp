// =============================================================================
//  第 19 課：vector 與原始陣列的互操作 14  —  data() 指標的生命週期：懸空指標
// =============================================================================
//
// 【主題資訊 Information】
//   T* vector<T>::data() noexcept;               // C++11 起
//
//   標頭檔：<vector>
//   複雜度：O(1)
//   本檔標準：C++17
//   本檔性質：**危險示範**。真正會觸發 UB 的那一行「刻意保持註解狀態」，
//             檔案本身可以正常編譯與執行，不會崩潰。
//
// 【詳細解釋 Explanation】
//
// 【1. data() 借出去的是「使用權」，不是「所有權」】
//   v.data() 回傳一個裸指標，指向 vector 內部那塊 heap 記憶體。
//   關鍵在於：這塊記憶體的**所有權自始至終屬於 vector**。
//   vector 一旦解構，它的 allocator 就會歸還那塊記憶體，
//   而你手上那個裸指標完全不知情——它仍然保有同一個數值，
//   只是那個位址已經不再屬於你。這就是懸空指標（dangling pointer）。
//
//   借書比喻：data() 像是把書借給你看，vector 是書的擁有者。
//   vector 解構等於「擁有者把書還給圖書館」，你手上的借書單號碼還在，
//   但那本書已經不在原來的架位上了。
//
// 【2. 為什麼這個 bug 特別難抓】
//   釋放後的記憶體通常不會立刻被作業系統收回——glibc 的 malloc 會把它
//   放進 free list 留待重用。所以「讀懸空指標」往往：
//     * 短時間內讀到的還是舊值 → 測試通過，你以為沒事
//     * 等到程式規模變大、有其他配置介入，那塊記憶體被別人拿去用
//       → 讀到別人的資料，或寫壞別人的資料
//     * 於是崩潰點離真正的 bug 十萬八千里遠
//   這正是為什麼「它看起來能跑」是 UB 最危險的性質，而不是最安全的性質。
//
// 【3. 三種正確做法】
//   (a) 縮短指標的生命週期，讓它不要活得比 vector 久（最好的做法）。
//   (b) 需要資料活得比 vector 久 → 把「值」複製出來，而不是留「指標」。
//   (c) 需要共享所有權 → 用 std::shared_ptr<std::vector<int>>，
//       讓 vector 本身的壽命由參考計數決定。
//
// 【4. 哪些操作會讓 data() 指標失效】
//   * vector 解構（本檔主題）
//   * 重新配置：push_back / emplace_back / insert / resize 變大 / reserve 變大
//   * assign、operator=、clear 之後再填入
//   * shrink_to_fit（可能重新配置）
//   * swap（兩個 vector 的緩衝區直接對調，指標會跟到「另一個」vector 身上）
//   反過來說，**不會**失效的有：純讀取、operator[] 寫入既有元素、
//   pop_back（size 變小但 buffer 不動）。
//
// 【概念補充 Concept Deep Dive】
//   ● 為什麼編譯器不警告
//     一般情況下，指標的來源與作用域跨越多層函式呼叫，編譯器沒有足夠資訊
//     做這種跨程序分析。GCC 的 -Wdangling-pointer（GCC 12 起）能抓到部分
//     簡單情形，但絕不能依賴它——真實專案的 bug 幾乎都藏在跨函式的呼叫鏈裡。
//
//   ● 用工具抓
//     編譯時加 -fsanitize=address 是最有效的手段：AddressSanitizer 會把
//     已釋放的記憶體標記成 poisoned，一旦讀寫就立刻報 heap-use-after-free，
//     並印出「配置在哪、釋放在哪、現在在哪讀」三段 stack trace。
//     Valgrind 也能抓，且不需重新編譯，但速度慢很多。
//     ⚠️ 注意：工具沒報錯不等於程式正確——它只能偵測到「實際執行到」的路徑。
//
//   ● 與 iterator 失效的關係
//     data() 指標失效和 iterator 失效是同一件事的兩種說法。
//     vector 的 iterator 在多數實作上就是包裝過的指標，
//     所以「哪些操作使 iterator 失效」的規則可以原封不動套用到 data()。
//
// 【注意事項 Pay Attention】
//   1. 讀寫懸空指標是 **未定義行為**。它不保證崩潰，也不保證印出任何特定值，
//      更不保證每次執行結果一樣。**任何「它會印出 X」的說法都是錯的。**
//   2. 「測試時沒出事」完全不能當作正確性的證據。
//   3. 指標的數值（位址）在 vector 解構後仍然存在，比較 ptr != nullptr
//      **無法**偵測懸空——這是最常見的誤解。
//   4. 本檔第 2 節示範的正確做法是「複製值出來」，不是「複製指標出來」。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】data() 指標的生命週期
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 下面這段程式有什麼問題？
//         int* p;
//         { std::vector<int> v = {1,2,3}; p = v.data(); }
//         std::cout << p[0];
//     答：v 在區塊結束時解構，歸還了底層記憶體，p 成為懸空指標。
//         最後一行讀取已釋放的記憶體，屬於未定義行為。
//     追問：那它實際上會印出什麼？→ **不能回答任何特定值**。
//         正確答案是「未定義，不保證任何結果，也不保證每次一樣」。
//         面試官問這句就是在測你會不會把 UB 講成固定行為。
//
// 🔥 Q2. 除了 vector 解構，還有哪些情況會讓 data() 指標失效？
//     答：任何造成重新配置的操作——push_back / emplace_back / insert /
//         reserve 變大 / resize 變大 / assign / shrink_to_fit / swap。
//         規則和 iterator 失效完全相同。
//     追問：pop_back 會讓它失效嗎？→ 不會。pop_back 只讓 size 變小、
//         銷毀最後一個元素，緩衝區本身不動，前面元素的指標仍然有效。
//
// ⚠️ 陷阱. 「我先檢查 if (p != nullptr) 再用，這樣就安全了吧？」
//     答：完全無效。vector 的解構子只歸還自己的記憶體，它不會（也無從）
//         把你手上那個變數改成 nullptr，所以這個檢查擋不掉任何東西。
//         更嚴格地說：依 [basic.stc]，對一個已失效的指標值做任何使用
//         （包含只是拿來和 nullptr 比較）都是 implementation-defined，
//         連「檢查」這個動作本身都已經不在標準保證的範圍內了。
//     為什麼會錯：把「指標為空」和「指標無效」當成同一件事。
//         實際上這是兩個獨立的概念：nullptr 是「明確表示不指向任何東西」，
//         懸空是「指向一個曾經有效、現在不再有效的位址」。
//         C++ 沒有任何內建機制能在執行期分辨後者——這正是要靠
//         RAII、智慧指標、以及「不要讓裸指標活得比擁有者久」的紀律來避免的原因。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <memory>
#include <string>

// -----------------------------------------------------------------------------
// 【日常實務範例】設定快取：回傳「值」而不是「內部指標」
//   情境：一個設定管理類別，內部把每個設定區段存在 vector<std::string> 裡。
//         新手常寫成回傳 const char*（指向內部資料）以「避免複製」，
//         結果只要內部容器一被重整（reload、push_back），外面拿到的指標就懸空。
//   下面示範兩個版本：危險介面與安全介面。
// -----------------------------------------------------------------------------
class ConfigCache {
    std::vector<std::string> sections_;
public:
    void add(const std::string& s) { sections_.push_back(s); }

    // ❌ 危險介面：回傳指向內部緩衝區的裸指標。
    //    呼叫端一旦在取得指標後再 add()，觸發重新配置，指標立刻懸空。
    //    保留此函式僅為對照說明，本檔不會用它做危險操作。
    const char* unsafe_get(std::size_t i) const {
        return sections_[i].c_str();
    }

    // ✅ 安全介面：回傳值的複本，呼叫端拿到的是自己的資料，
    //    之後容器怎麼變都與它無關。
    std::string safe_get(std::size_t i) const {
        return sections_[i];
    }

    std::size_t size() const { return sections_.size(); }
};

// 註：本檔不附 LeetCode 範例。懸空指標屬於「記憶體所有權與生命週期」議題，
//     LeetCode 的判題環境不會考這個（它給你的容器在整個函式期間都活著）。
//     硬掛一題與生命週期無關的陣列題只會稀釋本檔重點。

int main() {
    std::cout << "=== 1. 懸空指標是怎麼產生的 ===\n";
    int* dangerous_ptr = nullptr;

    {
        std::vector<int> v = {10, 20, 30};
        dangerous_ptr = v.data();
        std::cout << "在作用域內：" << dangerous_ptr[0] << std::endl;  // 10，OK
        std::cout << "此時指標有效，因為 v 還活著\n";
    }
    // v 已經被銷毀，底層記憶體已釋放

    // dangerous_ptr 現在是懸空指標（dangling pointer）
    // ⚠️ 下面這行是「未定義行為」，故意保持註解狀態。
    //    取消註解後：可能印出舊值、可能印出垃圾、可能崩潰，
    //    也可能在你的機器上「看起來正常」——這正是它最危險的地方。
    //    絕對不可以說「它一定會印出 X」。
    // std::cout << dangerous_ptr[0] << std::endl;  // 未定義行為！

    std::cout << "離開作用域後：dangerous_ptr 已成為無效指標值\n";
    std::cout << "  關鍵觀念：vector 的解構子只歸還它自己的記憶體，\n";
    std::cout << "  它不會、也沒有辦法把你手上這個變數改成 nullptr——\n";
    std::cout << "  所以 if (p != nullptr) 這種檢查對懸空指標完全無效。\n";
    std::cout << "  （本檔連「讀取 dangerous_ptr 的值來比較」都不做：依 [basic.stc]，\n";
    std::cout << "   對已失效的指標值做任何使用都是 implementation-defined，\n";
    std::cout << "   即使只是拿來和 nullptr 比較也一樣，不值得冒險示範。）\n";

    // 讓編譯器知道我們刻意不再使用它，同時避免任何對無效指標值的存取
    (void)dangerous_ptr;

    std::cout << "\n=== 2. 正確做法 A：把「值」複製出來 ===\n";
    std::vector<int> snapshot;                 // 活得比內層區塊久
    {
        std::vector<int> v = {10, 20, 30};
        snapshot = v;                          // 複製值，不是留指標
    }
    std::cout << "區塊結束後仍可安全使用：";
    for (int x : snapshot) std::cout << x << " ";
    std::cout << "（snapshot 擁有自己的記憶體）\n";

    std::cout << "\n=== 3. 正確做法 B：讓 vector 本身活得夠久（shared_ptr）===\n";
    std::shared_ptr<std::vector<int>> shared;
    {
        auto inner = std::make_shared<std::vector<int>>(
            std::initializer_list<int>{100, 200, 300});
        shared = inner;                        // 參考計數 +1
    }   // inner 離開作用域，但 vector 還活著，因為 shared 仍持有它
    std::cout << "use_count = " << shared.use_count() << "\n";
    std::cout << "資料仍然有效：";
    for (int x : *shared) std::cout << x << " ";
    std::cout << "\n此時取 shared->data() 是安全的，因為所有權被延長了\n";

    std::cout << "\n=== 4. 哪些操作「不會」讓指標失效 ===\n";
    std::vector<int> stable = {1, 2, 3, 4, 5};
    stable.reserve(100);                       // 先預留，之後不再重新配置
    int* p = stable.data();
    std::cout << "reserve(100) 後取得指標，p[0] = " << p[0] << "\n";
    stable[0] = 42;                            // 改既有元素 → 不失效
    std::cout << "改 stable[0] = 42 後，p[0] = " << p[0] << "（仍有效）\n";
    stable.pop_back();                         // size 變小 → 不失效
    std::cout << "pop_back() 後，p[0] = " << p[0] << "（仍有效）\n";
    stable.push_back(99);                      // 容量夠，不重新配置 → 不失效
    std::cout << "push_back(99)（容量足夠）後，p[0] = " << p[0] << "（仍有效）\n";
    std::cout << "  ↑ 但這只在 capacity 足夠時成立，不可依賴；\n";
    std::cout << "    安全的寫法永遠是「用之前重新呼叫 data()」\n";

    std::cout << "\n=== 日常實務：設定快取的安全 vs 危險介面 ===\n";
    ConfigCache cfg;
    cfg.add("[database]");
    cfg.add("[logging]");

    // ✅ 安全：拿到的是值的複本
    std::string kept = cfg.safe_get(0);
    cfg.add("[network]");                      // 內部 vector 可能重新配置
    cfg.add("[cache]");
    std::cout << "安全介面取得的內容：" << kept << "（新增區段後仍正確）\n";
    std::cout << "目前區段數：" << cfg.size() << "\n";
    std::cout << "若改用 unsafe_get() 取回 const char* 再繼續 add()，\n";
    std::cout << "那個指標會在容器重新配置後懸空——編譯器不會有任何警告。\n";

    std::cout << "\n提示：用 g++ -fsanitize=address 重新編譯並取消第 1 節那行註解，\n";
    std::cout << "      AddressSanitizer 會報 heap-use-after-free 並指出配置/釋放位置。\n";
    std::cout << "      但工具沒報錯不等於程式正確——它只檢查實際執行到的路徑。\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 19 課：vector 與原始陣列的互操作14.cpp" -o demo14
// 抓 UB: g++ -std=c++17 -Wall -Wextra -fsanitize=address -g "第 19 課：vector 與原始陣列的互操作14.cpp" -o demo14_asan

// === 預期輸出 ===
// === 1. 懸空指標是怎麼產生的 ===
// 在作用域內：10
// 此時指標有效，因為 v 還活著
// 離開作用域後：dangerous_ptr 已成為無效指標值
//   關鍵觀念：vector 的解構子只歸還它自己的記憶體，
//   它不會、也沒有辦法把你手上這個變數改成 nullptr——
//   所以 if (p != nullptr) 這種檢查對懸空指標完全無效。
//   （本檔連「讀取 dangerous_ptr 的值來比較」都不做：依 [basic.stc]，
//    對已失效的指標值做任何使用都是 implementation-defined，
//    即使只是拿來和 nullptr 比較也一樣，不值得冒險示範。）
//
// === 2. 正確做法 A：把「值」複製出來 ===
// 區塊結束後仍可安全使用：10 20 30 （snapshot 擁有自己的記憶體）
//
// === 3. 正確做法 B：讓 vector 本身活得夠久（shared_ptr）===
// use_count = 1
// 資料仍然有效：100 200 300
// 此時取 shared->data() 是安全的，因為所有權被延長了
//
// === 4. 哪些操作「不會」讓指標失效 ===
// reserve(100) 後取得指標，p[0] = 1
// 改 stable[0] = 42 後，p[0] = 42（仍有效）
// pop_back() 後，p[0] = 42（仍有效）
// push_back(99)（容量足夠）後，p[0] = 42（仍有效）
//   ↑ 但這只在 capacity 足夠時成立，不可依賴；
//     安全的寫法永遠是「用之前重新呼叫 data()」
//
// === 日常實務：設定快取的安全 vs 危險介面 ===
// 安全介面取得的內容：[database]（新增區段後仍正確）
// 目前區段數：4
// 若改用 unsafe_get() 取回 const char* 再繼續 add()，
// 那個指標會在容器重新配置後懸空——編譯器不會有任何警告。
//
// 提示：用 g++ -fsanitize=address 重新編譯並取消第 1 節那行註解，
//       AddressSanitizer 會報 heap-use-after-free 並指出配置/釋放位置。
//       但工具沒報錯不等於程式正確——它只檢查實際執行到的路徑。
