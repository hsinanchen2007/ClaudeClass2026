// =============================================================================
//  第 20 課：vector 效能分析與最佳實踐 5  —  emplace_back 的隱式轉換陷阱
// =============================================================================
//
// 【主題資訊 Information】
//   template <class... Args>
//   reference emplace_back(Args&&... args);        // C++11（C++17 起回傳 reference）
//   void push_back(const T& value);
//   void push_back(T&& value);                     // C++11
//
//   標頭檔：<vector>
//   複雜度：兩者皆為攤銷 O(1)
//   本檔標準：C++17（emplace_back 回傳 reference 是 C++17 才加的；
//                    C++11/14 時它回傳 void）
//
// 【詳細解釋 Explanation】
//
// 【1. emplace_back 到底做了什麼】
//   emplace_back 把你給的所有參數**原封不動地轉發**（perfect forwarding）
//   給元素型別的建構子，在容器的記憶體上直接建構物件：
//       vv.emplace_back(5, 0);   →   new (位址) std::vector<int>(5, 0);
//   關鍵在最後那一步：它呼叫的是 **T 的建構子**，而不是「把參數當成元素」。
//   當 T 本身是 std::vector<int> 時，vector<int>(5, 0) 的意思是
//   「5 個 0」，於是你得到一個 size()==5 的內層 vector，而不是 {5, 0}。
//
// 【2. push_back 為什麼沒有這個問題】
//   push_back 的參數型別就是 T（或 T&&），它只接受「一個已經成形的 T」。
//   所以你必須自己寫出 {5, 0}，意圖是明確的：
//       vv.push_back({5, 0});   →   先用 initializer_list 建出 vector<int>{5,0}，再移動進去
//   換句話說：push_back 逼你把型別轉換寫在呼叫端（看得見），
//   emplace_back 則把型別轉換藏進建構子（看不見）。
//
// 【3. 這不是 emplace_back 的 bug，是它的設計代價】
//   emplace_back 的價值在於「就地建構、省掉一次移動」，這個能力必然要求
//   它接受任意參數並轉發給建構子。一旦如此，它就無可避免地會啟用
//   **所有** 建構子，包括 explicit 的、包括你沒想到的那個重載。
//   這也是為什麼 emplace_back 能繞過 explicit：
//       std::vector<std::unique_ptr<int>> v;
//       v.push_back(new int(5));      // 編譯錯誤（unique_ptr 建構子是 explicit）
//       v.emplace_back(new int(5));   // 編譯通過！但若之後擴容失敗會洩漏
//   「編譯通過」在這裡不是好消息——explicit 本來就是設計來擋住這種寫法的。
//
// 【4. 什麼時候用哪個】
//   * 元素是「用多個參數建構」的物件（Point(x, y)、pair、自訂類別）→ emplace_back 好用。
//   * 元素本身是容器、或建構子有 (count, value) 這類數量語意的重載 → 用 push_back 明示。
//   * 已經有現成物件要放進去 → push_back(std::move(obj))，語意最清楚。
//   * 一般準則：**當參數個數與意圖不是一眼看得懂時，選 push_back。**
//
// 【概念補充 Concept Deep Dive】
//   ● 為什麼 vector<int>(5, 0) 是「5 個 0」
//     這與 vector 的 (size_type count, const T& value) 建構子有關。
//     它和「區間建構子 (InputIt first, InputIt last)」形成歧義，
//     標準明文規定：當兩個參數都是整數型別時，必須選 count/value 版本。
//     所以 (5, 0) 是「5 個 0」，而不是「從位址 5 到位址 0 的區間」。
//
//   ● 大括號 {} 為什麼會改變意思
//     只要型別有 initializer_list 建構子，{} 就會**優先**選它——
//     這個優先權高到會蓋過其他所有重載。所以：
//         std::vector<int> a(5, 0);   // 5 個 0        → size 5
//         std::vector<int> b{5, 0};   // 元素 5 和 0   → size 2
//     這是 C++11 引入統一初始化語法後最常被踩的坑之一。
//
//   ● emplace_back 回傳值的版本差異
//     C++11/14：回傳 void。
//     C++17 起：回傳 reference（指向剛插入的元素），可以寫成
//         auto& row = vv.emplace_back();   // 建一個空的內層 vector 並直接拿到它
//     本檔第 4 節會用到這個 C++17 特性，所以編譯標準必須是 C++17 以上。
//
// 【注意事項 Pay Attention】
//   1. emplace_back 會啟用 explicit 建構子，push_back 不會。
//      這代表 emplace_back 可能讓「本來應該編譯失敗」的程式通過編譯。
//   2. emplace_back 對「已經是 T 的物件」沒有任何效能優勢——
//      它一樣要複製或移動一次，此時 push_back 可讀性更好。
//   3. emplace_back(裸 new 指標) 到智慧指標容器有例外安全風險：
//      若容器擴容時丟出例外，那個裸指標還沒被接管，就洩漏了。
//      正解永遠是 emplace_back(std::make_unique<int>(5))。
//   4. vv.emplace_back(5, 0) 與 vv.push_back({5, 0}) 的結果**完全不同**，
//      而編譯器不會有任何警告——這是本檔最重要的一點。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】emplace_back vs push_back
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::vector<std::vector<int>> vv; vv.emplace_back(5, 0);
//        vv[0] 裡有什麼？
//     答：5 個 0（size()==5）。emplace_back 把 (5, 0) 轉發給元素型別的建構子，
//         也就是呼叫 std::vector<int>(5, 0) ——「5 個 0」的 count/value 建構子。
//         它不是把 5 和 0 當成兩個元素。
//     追問：那要怎麼寫才會得到 {5, 0} 兩個元素？
//         → vv.push_back({5, 0})，或 vv.emplace_back(std::initializer_list<int>{5, 0})。
//
// 🔥 Q2. emplace_back 一定比 push_back 快嗎？
//     答：不一定。只有當參數是「建構元素所需的原料」時，emplace_back 才省下
//         一次臨時物件的建構與移動。若你手上已經有一個 T 物件，
//         兩者都要複製／移動一次，效能相同，此時 push_back 更好讀。
//     追問：那為什麼還是常聽到「一律用 emplace_back」？→ 那是過度簡化的建議。
//         它忽略了 emplace_back 會啟用 explicit 建構子、會產生本題這種
//         意料外的重載選擇，可讀性與安全性都可能變差。
//
// ⚠️ 陷阱. 「emplace_back 更泛型、更現代，所以永遠優先用它」——錯在哪？
//     答：錯在把「效能」當成唯一標準，忽略了 emplace_back 會**繞過 explicit**。
//         例如 std::vector<std::unique_ptr<int>> v;
//              v.push_back(new int(5));      // 編譯失敗 ← 這是好事，擋住了危險寫法
//              v.emplace_back(new int(5));   // 編譯成功 ← 但擴容丟例外時會洩漏
//     為什麼會錯：腦中的模型是「emplace 就是 push 的高效版本」，
//         但兩者的**參數語意完全不同**：push_back 收「一個 T」，
//         emplace_back 收「建構 T 的原料」。前者的型別檢查發生在呼叫端（嚴格），
//         後者發生在建構子內部（寬鬆）。這個差異才是選擇的真正依據，效能只是次要。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <memory>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 118. Pascal's Triangle
//   題目：給定 numRows，回傳巴斯卡三角形的前 numRows 列。
//   為什麼用到本主題：回傳型別正是 vector<vector<int>>，
//     而且每一列都要「先建出指定長度、預設值為 1 的內層 vector」——
//     這裡 emplace_back(i + 1, 1) 的 (count, value) 語意剛好就是我們要的，
//     是少數 emplace_back 的「隱式轉換」正好符合意圖的場合。
//     本檔同時示範 C++17 的 emplace_back 回傳 reference 寫法。
// -----------------------------------------------------------------------------
std::vector<std::vector<int>> generate(int numRows) {
    std::vector<std::vector<int>> triangle;
    triangle.reserve(static_cast<std::size_t>(numRows));

    for (int i = 0; i < numRows; ++i) {
        // emplace_back(i + 1, 1) → vector<int>(i+1, 1)：i+1 個 1
        // 這正是巴斯卡三角形每列的初始狀態（頭尾都是 1）
        // C++17：emplace_back 回傳剛插入元素的 reference，可直接拿來改
        std::vector<int>& row = triangle.emplace_back(
            static_cast<std::size_t>(i + 1), 1);

        // 中間的值 = 上一列相鄰兩數之和
        for (int j = 1; j < i; ++j) {
            row[static_cast<std::size_t>(j)] =
                triangle[static_cast<std::size_t>(i - 1)][static_cast<std::size_t>(j - 1)] +
                triangle[static_cast<std::size_t>(i - 1)][static_cast<std::size_t>(j)];
        }
    }
    return triangle;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】建立固定欄數的 CSV 資料表
//   情境：讀 CSV 前已知欄數，要先配置好每一列。
//         這裡刻意展示兩種寫法的差異——用錯就會得到「一列 N 個空字串」
//         vs「一列 2 個指定字串」，而且編譯器完全不會提醒你。
// -----------------------------------------------------------------------------
struct CsvTable {
    std::vector<std::vector<std::string>> rows;

    // ✅ 意圖：新增一列，共 columnCount 個空欄位（等待填值）
    //    這裡的 (count, value) 語意是刻意使用的，所以寫註解說明
    std::vector<std::string>& addBlankRow(std::size_t columnCount) {
        return rows.emplace_back(columnCount, std::string{});
    }

    // ✅ 意圖：新增一列，內容就是給定的這些欄位
    //    用 push_back 明示「我給的是一個完整的列」
    void addRow(std::vector<std::string> cells) {
        rows.push_back(std::move(cells));
    }
};

int main() {
    std::cout << "=== 1. emplace_back 的隱式轉換陷阱 ===\n";
    std::vector<std::vector<int>> vv;
    vv.reserve(4);

    // emplace_back 可能導致意外的隱式轉換
    vv.emplace_back(5, 0);
    // 這呼叫的是 vector<int>(5, 0)，建立一個含 5 個 0 的 vector
    // 你可能以為是把 5 和 0 當成兩個元素？不是！

    std::cout << "vv.emplace_back(5, 0)\n";
    std::cout << "vv[0].size() = " << vv[0].size() << std::endl;
    std::cout << "vv[0] 內容：";
    for (int x : vv[0]) std::cout << x << " ";
    std::cout << std::endl;

    // push_back 在這種情況下更安全、更明確
    vv.push_back({5, 0});  // 明確：用初始化串列建立含 5 和 0 兩個元素的 vector
    std::cout << "\nvv.push_back({5, 0})\n";
    std::cout << "vv[1].size() = " << vv[1].size() << std::endl;
    std::cout << "vv[1] 內容：";
    for (int x : vv[1]) std::cout << x << " ";
    std::cout << std::endl;
    std::cout << "→ 同樣寫 5 和 0，結果一個 5 個元素、一個 2 個元素，"
                 "編譯器不會有任何警告\n";

    std::cout << "\n=== 2. 想用 emplace_back 又要 {5, 0} 該怎麼寫 ===\n";
    vv.emplace_back(std::initializer_list<int>{5, 0});   // 明確指定型別
    std::cout << "vv.emplace_back(std::initializer_list<int>{5, 0})\n";
    std::cout << "vv[2].size() = " << vv[2].size() << " 內容：";
    for (int x : vv[2]) std::cout << x << " ";
    std::cout << "（與 push_back({5,0}) 等價，但囉唆得多）\n";

    std::cout << "\n=== 3. 同樣的坑也存在於單層 vector ===\n";
    std::vector<int> a(5, 0);      // 圓括號：count/value → 5 個 0
    std::vector<int> b{5, 0};      // 大括號：initializer_list → 元素 5 和 0
    std::cout << "vector<int> a(5, 0) → size=" << a.size() << "\n";
    std::cout << "vector<int> b{5, 0} → size=" << b.size() << "\n";
    std::cout << "→ 只差一對括號，語意天差地遠\n";

    std::cout << "\n=== 4. C++17：emplace_back 回傳 reference ===\n";
    std::vector<std::vector<int>> grid;
    // C++11/14 時 emplace_back 回傳 void，必須寫 grid.back()；
    // C++17 起可直接接住剛插入的元素
    std::vector<int>& newRow = grid.emplace_back();   // 建一個空的內層 vector
    newRow.push_back(42);
    newRow.push_back(43);
    std::cout << "grid[0] 內容：";
    for (int x : grid[0]) std::cout << x << " ";
    std::cout << "(size=" << grid[0].size() << ")\n";

    std::cout << "\n=== 5. emplace_back 會繞過 explicit（觀念示範）===\n";
    std::vector<std::unique_ptr<int>> ptrs;
    // ptrs.push_back(new int(5));      // ← 編譯失敗：unique_ptr 建構子是 explicit（這是好事）
    // ptrs.emplace_back(new int(5));   // ← 可編譯，但擴容丟例外時裸指標會洩漏
    ptrs.emplace_back(std::make_unique<int>(5));   // ✅ 正解：先讓智慧指標接管
    std::cout << "用 make_unique 安全放入，*ptrs[0] = " << *ptrs[0] << "\n";
    std::cout << "（push_back(new int(5)) 編譯失敗是 explicit 在保護你；\n";
    std::cout << "  emplace_back 會讓它通過編譯，反而失去這層保護）\n";

    std::cout << "\n=== LeetCode 118. Pascal's Triangle ===\n";
    auto triangle = generate(5);
    for (const auto& row : triangle) {
        std::cout << "  ";
        for (int x : row) std::cout << x << " ";
        std::cout << "\n";
    }
    std::cout << "共 " << triangle.size() << " 列"
              << "（此處 emplace_back(i+1, 1) 的 count/value 語意正好符合需求）\n";

    std::cout << "\n=== 日常實務：固定欄數的 CSV 資料表 ===\n";
    CsvTable table;

    // 用完整的一列
    table.addRow({"姓名", "部門", "到職日"});

    // 先開一列空白（3 欄），稍後再填
    std::vector<std::string>& blank = table.addBlankRow(3);
    blank[0] = "王小明";
    blank[1] = "研發部";
    blank[2] = "2026-03-01";

    table.addRow({"李小華", "業務部", "2025-11-15"});

    for (const auto& row : table.rows) {
        std::cout << "  ";
        for (const auto& cell : row) std::cout << "[" << cell << "]";
        std::cout << "\n";
    }
    std::cout << "共 " << table.rows.size() << " 列，每列 "
              << table.rows[0].size() << " 欄\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 20 課：vector 效能分析與最佳實踐5.cpp" -o demo5

// === 預期輸出 ===
// === 1. emplace_back 的隱式轉換陷阱 ===
// vv.emplace_back(5, 0)
// vv[0].size() = 5
// vv[0] 內容：0 0 0 0 0
//
// vv.push_back({5, 0})
// vv[1].size() = 2
// vv[1] 內容：5 0
// → 同樣寫 5 和 0，結果一個 5 個元素、一個 2 個元素，編譯器不會有任何警告
//
// === 2. 想用 emplace_back 又要 {5, 0} 該怎麼寫 ===
// vv.emplace_back(std::initializer_list<int>{5, 0})
// vv[2].size() = 2 內容：5 0 （與 push_back({5,0}) 等價，但囉唆得多）
//
// === 3. 同樣的坑也存在於單層 vector ===
// vector<int> a(5, 0) → size=5
// vector<int> b{5, 0} → size=2
// → 只差一對括號，語意天差地遠
//
// === 4. C++17：emplace_back 回傳 reference ===
// grid[0] 內容：42 43 (size=2)
//
// === 5. emplace_back 會繞過 explicit（觀念示範）===
// 用 make_unique 安全放入，*ptrs[0] = 5
// （push_back(new int(5)) 編譯失敗是 explicit 在保護你；
//   emplace_back 會讓它通過編譯，反而失去這層保護）
//
// === LeetCode 118. Pascal's Triangle ===
//   1
//   1 1
//   1 2 1
//   1 3 3 1
//   1 4 6 4 1
// 共 5 列（此處 emplace_back(i+1, 1) 的 count/value 語意正好符合需求）
//
// === 日常實務：固定欄數的 CSV 資料表 ===
//   [姓名][部門][到職日]
//   [王小明][研發部][2026-03-01]
//   [李小華][業務部][2025-11-15]
// 共 3 列，每列 3 欄
