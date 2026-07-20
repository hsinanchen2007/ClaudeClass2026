// =============================================================================
//  第七課：演算法（Algorithm）與容器的分離設計9.cpp
//    —  replace / remove 與 erase-remove idiom（本課最重要的一課）
// =============================================================================
//
// 【主題資訊 Information】
//   void  replace   (FwdIt f, FwdIt l, const T& old_v, const T& new_v);  // C++98
//   void  replace_if(FwdIt f, FwdIt l, UnaryPred p,    const T& new_v);  // C++98
//   FwdIt remove    (FwdIt f, FwdIt l, const T& value);                  // C++98
//   FwdIt remove_if (FwdIt f, FwdIt l, UnaryPred p);                     // C++98
//
//   C++20 新增（真正會刪除、直接吃容器）：
//   size_type std::erase   (std::vector<T,A>& c, const U& value);        // C++20 ★
//   size_type std::erase_if(std::vector<T,A>& c, Pred pred);             // C++20 ★
//
//   標準版本：replace/remove 系列為 C++98；**std::erase / erase_if 是 C++20**
//   迭代器需求：Forward Iterator
//   複雜度：全部 O(N)，恰好比較 N 次
//   回傳：remove / remove_if 回傳「新的邏輯結尾」；replace 系列回傳 void
//   標頭檔：<algorithm>（std::erase / erase_if 在對應容器的標頭，如 <vector>）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 std::remove 不會真的刪除元素——這是全課的核心】
// 這是整個第 7 課「分離設計」最直接、也最痛的後果：
//   **演算法只拿到 iterator，拿不到容器本體。**
// 刪除元素會改變容器的 size、可能要釋放記憶體、可能要搬移後續元素，
// 這些全都是**容器**的職責，需要呼叫 vector::erase 這類成員函式。
// 而 remove 手上只有 first 和 last 兩個迭代器，它根本**沒有辦法**知道
// 這段範圍屬於哪個容器，更沒有辦法呼叫那個容器的成員函式。
// 所以 remove 能做的只有一件事：**在原地把要保留的元素往前搬**，
// 然後回傳「保留區的結尾」，剩下的清理工作交還給呼叫端。
// 這不是設計缺陷，是分離設計必然的代價——換來的是同一個 remove
// 可以用在 vector、deque、array、原生陣列上。
//
// 【2. remove 到底做了什麼：一次完整的追蹤】
// 以 {1, 2, 3, 2, 4, 2, 5} 移除 2 為例，remove 的行為類似：
//     讀 1 → 保留 → 寫到位置 0
//     讀 2 → 丟棄
//     讀 3 → 保留 → 寫到位置 1
//     讀 2 → 丟棄
//     讀 4 → 保留 → 寫到位置 2
//     讀 2 → 丟棄
//     讀 5 → 保留 → 寫到位置 3
//     回傳指向位置 4 的迭代器（= new_end）
// 結果容器變成 {1, 3, 4, 5, ?, ?, ?}，size **仍然是 7**。
// 前 4 格是有效資料，後 3 格是「valid but unspecified（有效但未指定）」狀態。
//
// 【3. 「valid but unspecified」到底是什麼意思】
// C++11 起 remove 內部使用 **move assignment** 搬移元素，
// 所以尾端那些位置是「被搬走之後的來源物件」。標準對 moved-from 物件的規定是：
//   * 它仍是**合法物件**（可以安全解構、可以被賦新值）
//   * 但它的**值是未指定的**（unspecified）
// 對 int 這類 trivially copyable 型別，實務上舊值會原封不動留著
// （本檔輸出中看到的尾端數字就是這樣來的，屬 **libstdc++ 實測結果，非標準保證**）。
// 但對 std::string 就完全不同了——被 move 走的 string 通常會變成空字串。
// **所以絕對不能依賴尾端的值**，唯一該做的事就是把它們 erase 掉。
//
// 【4. erase-remove idiom：兩個角色各司其職】
//     v.erase( std::remove(v.begin(), v.end(), 2), v.end() );
//     └─容器的活─┘ └────────演算法的活────────┘  └─到真正的尾端─┘
//   * std::remove（演算法）：只會搬，把該留的往前擠，回傳新邏輯結尾
//   * vector::erase（成員函式）：只有它能真正改變 size、釋放元素
// 這行程式碼是 C++ 最經典的慣用法之一，也是面試最愛考的一行。
// 注意 erase 的第二個參數必須是 **v.end()**（真正的實體結尾），不是 new_end。
//
// 【5. C++20 之後：終於可以寫一行】
//     std::erase(v, 2);                                    // C++20
//     std::erase_if(v, [](int n){ return n % 2 == 0; });    // C++20
// 這兩個是**非成員函式，直接吃容器**（所以拿得到容器本體，能真的刪除），
// 而且回傳被刪掉的元素個數。C++20 起應優先使用，它們消除了
// 「忘記包 erase」這個經典 bug。本檔用 C++17 編譯，所以示範傳統寫法；
// 若專案已在 C++20，請直接用 std::erase / std::erase_if。
//
// 【6. replace 為什麼不會有這個問題】
// replace 是**原地替換**，元素個數完全不變，所以它不需要碰容器本體，
// 回傳 void 就夠了。凡是「不改變元素個數」的演算法（replace、transform、
// sort、reverse、fill）都不會有 remove 這種困擾。
// **會改變「有效元素個數」的演算法（remove、unique）才需要搭配 erase。**
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 remove 不是穩定地「交換」而是「覆蓋」
//   remove 用的是 move assignment 覆寫，不是 std::swap。這表示被丟棄的元素
//   **不會**被保存到尾端——尾端只是「搬移後的殘骸」。
//   若你需要同時拿到「保留的」和「丟棄的」兩組資料，
//   應該用 std::partition / std::stable_partition（它們是交換，兩邊都保留），
//   或用 remove_copy_if 把結果複製到別處而不動原容器。
//
// (B) list 有自己的 remove，而且更快
//   std::list 提供成員函式 lst.remove(value)，它**直接解開節點指標並釋放節點**，
//   是真正的刪除，O(N) 一次搞定、不需要搬移元素、也不會使其他元素的
//   iterator 失效。對 list 用 erase-remove idiom 雖然能編譯，
//   但會做無謂的元素搬移，效能較差。**同名時優先用成員函式**
//   （見本課第 16 個檔案）。
//
// (C) erase-remove 對 vector 的 iterator 失效規則
//   vector::erase 會使**被刪除位置以後**的所有 iterator、reference、pointer 失效。
//   所以慣用法寫成一行是安全的（沒有留存舊迭代器），
//   但若你先存了 auto it = v.begin() + 5 再做 erase-remove，那個 it 就可能已失效。
//
// 【注意事項 Pay Attention】
// 1. **std::remove 不會改變容器的 size**，只回傳新的邏輯結尾。
//    忘記包 v.erase(...) 是 C++ 最經典的 bug 之一。
// 2. remove 之後，[new_end, end) 的元素是 **valid but unspecified**。
//    對 int 看起來像舊值（libstdc++ 實測），對 std::string 通常已被清空。
//    **任何情況下都不可依賴這些值。**
// 3. erase 的第二個參數要用 **v.end()**，不是 new_end。
//    寫成 v.erase(new_end, new_end) 什麼也不會刪。
// 4. **對 std::list 請用成員函式 lst.remove(value)**，不要用 erase-remove。
// 5. **對 std::set / std::map 兩者都不能用**：元素是 const 的，無法被搬移。
//    要用它們自己的 s.erase(value)。
// 6. C++20 起優先用 std::erase(v, val) / std::erase_if(v, pred)，一行搞定且不會忘。
// 7. remove 不保證被移除元素的「殘骸」留在尾端；它是覆蓋不是交換。
//    需要保留兩組資料請用 stable_partition 或 remove_copy_if。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】remove 與 erase-remove idiom
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::remove 為什麼不會真的刪除元素？
//     答：因為演算法只拿到 [first, last) 兩個迭代器，**拿不到容器本體**。
//         刪除元素要改變 size、可能要釋放記憶體，那是容器成員函式的職責，
//         remove 沒有能力呼叫 vector::erase，甚至不知道這段範圍屬於哪個容器。
//         所以它只能把要保留的元素往前搬，回傳新的邏輯結尾，
//         真正的刪除交還給呼叫端：v.erase(std::remove(...), v.end())。
//     追問：這是設計缺陷嗎？→ 不是，是分離設計的必然代價。換來的是同一個
//         remove 能用在 vector、deque、array 和原生陣列上。C++20 補了
//         std::erase(v, val) 這個「直接吃容器」的非成員函式來消除這個坑。
//
// 🔥 Q2. 執行 std::remove(v.begin(), v.end(), 2) 之後，v.size() 是多少？
//        尾端那些元素的值是什麼？
//     答：size **完全不變**（remove 動不了 size）。[new_end, v.end()) 這段是
//         「valid but unspecified」——C++11 起 remove 用 move assignment 搬移，
//         尾端是被 move 走之後的殘骸。對 int 這類型別實務上會看到舊值
//         （libstdc++ 實測），但對 std::string 通常已變成空字串。
//         標準不保證任何特定值，**絕不可依賴**。
//     追問：那正確的完整寫法是什麼？→ v.erase(std::remove(v.begin(), v.end(), 2), v.end());
//         注意第二個參數是 v.end() 不是 new_end。
//
// 🔥 Q3. 對 std::list 可以用 erase-remove idiom 嗎？該用嗎？
//     答：可以編譯、結果也正確，但**不該用**。list 提供成員函式 lst.remove(value)，
//         它直接解開節點指標並釋放節點，不需要搬移任何元素；
//         而 std::remove 會逐一 move assignment 搬移元素值，做了大量無謂的工作。
//         原則：**容器有同名成員函式時一律優先用成員函式**。
//     追問：set / map 呢？→ 兩種寫法都不能用。set 的元素透過 iterator 取得時是
//         const 的，無法被 move assignment 覆寫，erase-remove 根本編譯不過。
//         必須用 s.erase(value)。
//
// ⚠️ 陷阱. 這段程式碼錯在哪？
//        std::remove(v.begin(), v.end(), 0);
//        std::cout << "剩下 " << v.size() << " 個元素";
//     答：兩個錯。第一，remove 的回傳值被丟棄了，沒有 erase，元素根本沒被刪除；
//         第二，v.size() 從頭到尾都沒變過，印出來的是原本的大小。
//         程式看起來「跑得很順」，但完全沒做到預期的事。
//     為什麼會錯：名字騙人。remove 這個字在其他語言（Python 的 list.remove、
//         Java 的 Collection.remove）都是真的刪除，只有 STL 的 remove 因為
//         拿不到容器而只能搬移。**這個名字是 STL 公認的設計失誤之一**，
//         C++20 才用 std::erase 補救。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <algorithm>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 27. Remove Element
//   題目：就地移除 nums 中所有等於 val 的元素，回傳剩餘元素個數 k，
//         且 nums 的前 k 個位置必須是那些剩餘元素（順序不限）。
//   為什麼用到本主題：這題的規格**就是 std::remove 的合約**——
//         「把要保留的搬到前面，回傳新的邏輯結尾」，連「不要求處理尾端」
//         這點都一模一樣。LeetCode 只檢查前 k 個，正對應 remove 不清尾端。
//   複雜度：O(N)。
// -----------------------------------------------------------------------------
int removeElement(std::vector<int>& nums, int val) {
    auto new_end = std::remove(nums.begin(), nums.end(), val);
    return static_cast<int>(new_end - nums.begin());   // 前 k 個就是答案
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 283. Move Zeroes
//   題目：就地把所有 0 移到陣列尾端，同時**保持非零元素的相對順序**。
//   為什麼用到本主題：這題不能用 remove（remove 會把尾端變成未指定狀態，
//         而這題要求尾端必須是 0）。正解是 std::stable_partition——
//         它「交換」而非「覆蓋」，兩邊資料都保留，且保證穩定性。
//         這正好對照出 remove 與 partition 的關鍵差異。
//   複雜度：stable_partition 為 O(N)（有額外記憶體時）或 O(N log N)（無額外記憶體時）。
// -----------------------------------------------------------------------------
void moveZeroes(std::vector<int>& nums) {
    // stable_partition：非零的排前面且保持原順序，零的自動被擠到後面
    std::stable_partition(nums.begin(), nums.end(),
                          [](int n) { return n != 0; });
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】設定檔清洗：移除註解行與空白行
//   情境：讀進一份 .conf 設定檔，送進解析器之前要先把註解行（# 開頭）
//         和空白行清掉。這是 erase-remove idiom 最典型的真實用途。
//   為什麼用到本主題：條件式移除用 remove_if，且**必須**包上 erase，
//         否則 vector 的 size 不變，後續 for 迴圈會處理到未指定的殘骸。
//   注意：元素型別是 std::string，被 move 走的 string 通常變成空字串——
//         這正是「不可依賴尾端值」的最好證明。
// -----------------------------------------------------------------------------
std::vector<std::string> cleanConfigLines(std::vector<std::string> lines) {
    lines.erase(
        std::remove_if(lines.begin(), lines.end(),
                       [](const std::string& s) {
                           if (s.empty()) return true;                 // 空行
                           std::size_t p = s.find_first_not_of(" \t");
                           if (p == std::string::npos) return true;    // 全是空白
                           return s[p] == '#';                         // 註解行
                       }),
        lines.end());          // ★ 第二個參數必須是 end()，不是 remove 的回傳值
    return lines;
}

int main() {
    // replace：替換所有等於某值的元素, 不會改變容器大小
    // ★ replace 是原地替換，元素個數不變，所以不需要 erase
    std::cout << "=== replace ===" << std::endl;
    std::vector<int> vec = {1, 2, 3, 2, 4, 2, 5};
    std::replace(vec.begin(), vec.end(), 2, 99);
    std::cout << "把 2 替換為 99: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    // replace_if：條件替換, 不會改變容器大小
    std::cout << "\n=== replace_if ===" << std::endl;
    std::vector<int> vec2 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::replace_if(vec2.begin(), vec2.end(),
        [](int n) { return n % 2 == 0; }, 0);
    std::cout << "偶數替換為 0: ";
    for (int n : vec2) std::cout << n << " ";
    std::cout << std::endl;

    // remove：移除所有等於某值的元素, 但不會改變容器大小
    // 重要：remove 不會真正刪除元素，而是把要保留的移到前面！剩下的元素是未定義的
    std::cout << "\n=== remove（重要！）===" << std::endl;
    std::vector<int> vec3 = {1, 2, 3, 2, 4, 2, 5};
    std::cout << "原始: ";
    for (int n : vec3) std::cout << n << " ";
    std::cout << "(size=" << vec3.size() << ")" << std::endl;

    auto new_end = std::remove(vec3.begin(), vec3.end(), 2);
    std::cout << "remove 後: ";
    for (int n : vec3) std::cout << n << " ";
    std::cout << "(size=" << vec3.size() << ")" << std::endl;
    std::cout << "  ↑ size 完全沒變！尾端 " << (vec3.end() - new_end)
              << " 格是 valid but unspecified" << std::endl;
    std::cout << "  ↑ 這裡看到的尾端數值是 libstdc++ 實測結果，非標準保證，不可依賴"
              << std::endl;

    std::cout << "有效範圍: ";
    for (auto it = vec3.begin(); it != new_end; ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // ★ 對 string 做同樣的事：尾端殘骸長得完全不一樣（被 move 走 → 空字串）
    std::cout << "\n=== 尾端殘骸不可依賴：string 的情況 ===" << std::endl;
    std::vector<std::string> names = {"alice", "bob", "alice", "carol"};
    auto s_end = std::remove(names.begin(), names.end(), std::string("alice"));
    std::cout << "移除 alice 後逐格檢視: ";
    for (std::size_t i = 0; i < names.size(); ++i) {
        std::cout << "[" << names[i] << "]";
    }
    std::cout << std::endl;
    std::cout << "  ↑ 尾端 " << (names.end() - s_end)
              << " 格被 move 走後成了空字串（libstdc++ 實測），"
                 "與 int 的表現完全不同" << std::endl;

    // 真正刪除：erase-remove 慣用法, 先 remove 把要保留的元素移到前面，再用 erase 刪除多餘的元素
    std::cout << "\n=== erase-remove 慣用法 ===" << std::endl;
    std::vector<int> vec4 = {1, 2, 3, 2, 4, 2, 5};
    vec4.erase(std::remove(vec4.begin(), vec4.end(), 2), vec4.end());
    std::cout << "erase-remove 後: ";
    for (int n : vec4) std::cout << n << " ";
    std::cout << "(size=" << vec4.size() << ")" << std::endl;

    // ★ 對照組：只有 remove 沒有 erase（最經典的 bug）
    // ☆ 現代 libstdc++ 把 std::remove 標成 [[nodiscard]]，丟棄回傳值時
    //   g++ -Wall 會發出 -Wunused-result 警告，等於編譯器主動幫你抓這個 bug。
    //   這裡為了示範「忘記 erase 的後果」，用 (void) 明確表示我們是刻意丟棄的。
    //   在真實程式碼中看到這個警告，幾乎可以確定就是漏掉了 erase。
    std::cout << "\n=== 忘記 erase 的後果 ===" << std::endl;
    std::vector<int> buggy = {1, 2, 3, 2, 4, 2, 5};
    (void)std::remove(buggy.begin(), buggy.end(), 2);  // 回傳值被丟棄（刻意示範）
    std::cout << "只呼叫 remove 之後 size = " << buggy.size()
              << "  ← 什麼都沒刪掉" << std::endl;
    std::cout << "  ↑ 少了 erase，這正是編譯器用 [[nodiscard]] 想警告你的事"
              << std::endl;

    // ★ list 有自己的 remove，是真正的刪除
    std::cout << "\n=== list::remove 是真正的刪除（成員函式優先）===" << std::endl;
    std::list<int> lst = {1, 2, 3, 2, 4, 2, 5};
    lst.remove(2);                                     // 直接解開節點指標
    std::cout << "list::remove(2) 後: ";
    for (int n : lst) std::cout << n << " ";
    std::cout << "(size=" << lst.size() << ")  ← size 真的變了" << std::endl;

    std::cout << "\n=== LeetCode 27. Remove Element ===" << std::endl;
    std::vector<int> lc27a = {3, 2, 2, 3};
    int k1 = removeElement(lc27a, 3);
    std::cout << "[3,2,2,3] 移除 3 -> k=" << k1 << ", 前 k 個: ";
    for (int i = 0; i < k1; ++i) std::cout << lc27a[i] << " ";
    std::cout << std::endl;

    std::vector<int> lc27b = {0, 1, 2, 2, 3, 0, 4, 2};
    int k2 = removeElement(lc27b, 2);
    std::cout << "[0,1,2,2,3,0,4,2] 移除 2 -> k=" << k2 << ", 前 k 個: ";
    for (int i = 0; i < k2; ++i) std::cout << lc27b[i] << " ";
    std::cout << std::endl;

    std::cout << "\n=== LeetCode 283. Move Zeroes ===" << std::endl;
    std::vector<int> lc283 = {0, 1, 0, 3, 12};
    moveZeroes(lc283);
    std::cout << "[0,1,0,3,12] -> ";
    for (int n : lc283) std::cout << n << " ";
    std::cout << "  (stable_partition 是交換不是覆蓋，所以 0 真的被保留在尾端)"
              << std::endl;

    std::cout << "\n=== 日常實務：清洗設定檔 ===" << std::endl;
    std::vector<std::string> conf = {
        "# database settings",
        "host = 10.0.0.5",
        "",
        "   ",
        "port = 5432",
        "# timeout in seconds",
        "timeout = 30"
    };
    std::cout << "原始 " << conf.size() << " 行" << std::endl;
    auto cleaned = cleanConfigLines(conf);
    std::cout << "清洗後 " << cleaned.size() << " 行:" << std::endl;
    for (const auto& line : cleaned) std::cout << "  " << line << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第七課：演算法（Algorithm）與容器的分離設計9.cpp -o demo9

//   ↑ 這裡看到的尾端數值是 libstdc++ 實測結果，非標準保證，不可依賴

// === 預期輸出 ===
// === replace ===
// 把 2 替換為 99: 1 99 3 99 4 99 5 
//
// === replace_if ===
// 偶數替換為 0: 1 0 3 0 5 0 7 0 9 0 
//
// === remove（重要！）===
// 原始: 1 2 3 2 4 2 5 (size=7)
// remove 後: 1 3 4 5 4 2 5 (size=7)
//   ↑ size 完全沒變！尾端 3 格是 valid but unspecified
// 有效範圍: 1 3 4 5 
//
// === 尾端殘骸不可依賴：string 的情況 ===
// 移除 alice 後逐格檢視: [bob][carol][alice][]
//   ↑ 尾端 2 格被 move 走後成了空字串（libstdc++ 實測），與 int 的表現完全不同
//
// === erase-remove 慣用法 ===
// erase-remove 後: 1 3 4 5 (size=4)
//
// === 忘記 erase 的後果 ===
// 只呼叫 remove 之後 size = 7  ← 什麼都沒刪掉
//   ↑ 少了 erase，這正是編譯器用 [[nodiscard]] 想警告你的事
//
// === list::remove 是真正的刪除（成員函式優先）===
// list::remove(2) 後: 1 3 4 5 (size=4)  ← size 真的變了
//
// === LeetCode 27. Remove Element ===
// [3,2,2,3] 移除 3 -> k=2, 前 k 個: 2 2 
// [0,1,2,2,3,0,4,2] 移除 2 -> k=5, 前 k 個: 0 1 3 0 4 
//
// === LeetCode 283. Move Zeroes ===
// [0,1,0,3,12] -> 1 3 12 0 0   (stable_partition 是交換不是覆蓋，所以 0 真的被保留在尾端)
//
// === 日常實務：清洗設定檔 ===
// 原始 7 行
// 清洗後 3 行:
//   host = 10.0.0.5
//   port = 5432
//   timeout = 30
