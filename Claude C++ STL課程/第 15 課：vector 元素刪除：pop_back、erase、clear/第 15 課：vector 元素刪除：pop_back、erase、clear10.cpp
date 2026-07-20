// =============================================================================
//  第 15 課：vector 元素刪除 10  —  Erase-Remove 慣用法
// =============================================================================
//
// 【主題資訊 Information】
//   template<class ForwardIt, class UnaryPredicate>
//   ForwardIt remove_if(ForwardIt first, ForwardIt last, UnaryPredicate p);
//   template<class ForwardIt, class T>
//   ForwardIt remove(ForwardIt first, ForwardIt last, const T& value);
//   標頭檔：<algorithm>（remove/remove_if）、<vector>（erase）
//   標準版本：C++98。
//   複雜度：remove_if 是 O(n)（每個元素恰好檢查一次）；
//           後續的 erase 只刪尾段，不必搬移任何元素。整體 O(n)。
//   回傳：remove_if 回傳「新的邏輯結尾」——保留下來的元素之後的第一個位置。
//   ★ 關鍵：remove/remove_if【不會改變容器的 size】。
//     它們是演算法，演算法只能透過迭代器操作元素，碰不到容器本身。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼需要兩個步驟：演算法碰不到容器】
//   這是整個 STL 設計哲學在此處的直接後果。
//   std::remove_if 只拿到一對迭代器 [first, last)，它完全不知道
//   這段區間屬於哪個容器——可能是 vector、可能是 array、
//   也可能是原生指標。既然不知道容器是誰，就無法呼叫容器的 erase，
//   也就無法改變 size。
//   所以 remove_if 能做的只有「重排元素」：把要保留的往前搬，
//   然後告訴你「有效資料到這裡為止」。
//   真正把 size 縮小，只有容器自己的成員函式 erase 做得到。
//   兩者合起來就是 erase-remove 慣用法：
//       v.erase(std::remove_if(v.begin(), v.end(), pred), v.end());
//
// 【2. remove_if 之後，尾巴那些元素是什麼】
//   常見的誤解是「尾巴保留著被刪掉的原值」。標準的說法更弱也更精確：
//   [新結尾, end()) 這段元素處於【有效但未指定】的狀態。
//   實務上 libstdc++ 是用移動賦值把保留的元素往前搬，
//   所以尾巴留下的是「被移走後的殘骸」——
//   對 int 這種型別看起來像舊值，對 std::string 則多半是空字串。
//   結論：那段資料不可讀、不可依賴，唯一該做的就是 erase 掉。
//
// 【3. 為什麼這個組合是 O(n)】
//   remove_if：一次線性掃描，每個元素最多被移動一次 → O(n)
//   erase(新結尾, end())：last 就是 end()，後面沒有元素要搬，
//     只要解構尾段 → O(被刪除的數量)
//   總計 O(n)。
//   對比「迴圈裡逐一 erase」是 O(n²)——每刪一個就搬移一次後半段。
//   本課第 12 檔有實測對照。
//
// 【4. remove 系列是「穩定」的】
//   保留下來的元素會維持原本的相對順序。這一點標準有明確保證，
//   對「篩選但要保序」的需求很重要。
//   若你不需要保序，還有更快的做法：swap-and-pop（見第 13 檔），
//   單一元素的刪除可以做到 O(1)。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼叫 remove 卻不 remove
//     這個命名確實不好，是 STL 最常被抱怨的地方之一。
//     比較準確的名字應該是 "shift_kept_to_front" 或 "partition_out"。
//     但它的行為和 std::partition 家族是一致的——
//     所有「重排型」演算法都只搬移、不改變容器大小。
//     C++20 加入 std::erase / std::erase_if 這兩個【自由函式】，
//     正是為了把這個容易誤用的兩步驟收編成一步（見第 11 檔）。
//
// (B) remove_if 對 std::list 是可以用，但有更好的選擇
//     list 有自己的成員函式 list::remove_if，它是 O(n) 且
//     直接把節點刪掉（不必搬移資料、也不會使其他迭代器失效）。
//     對 list 應該用成員版本。這是「容器有專屬版本時優先用成員函式」
//     的通則：list::sort、list::unique、map::find 都是同樣的道理。
//
// (C) 述詞不該有副作用
//     標準允許實作以未指定的次數呼叫述詞。
//     若你的 lambda 內有計數器或輸出，結果可能與預期不符。
//     需要統計刪除數量時，正確做法是用回傳值計算：
//         auto newEnd = std::remove_if(...);
//         auto count  = std::distance(newEnd, v.end());
//     C++20 的 std::erase_if 則直接回傳刪除的數量。
//
// 【注意事項 Pay Attention】
//   1. remove/remove_if 不改變 size，一定要接 erase。
//      只寫 std::remove_if(...) 而忘了 erase，是最經典的錯誤。
//   2. [新結尾, end()) 的元素是「有效但未指定」，不可讀取。
//   3. erase 的第二個參數一定是 v.end()，不是別的。
//   4. 述詞不該有副作用（呼叫次數未指定）。
//   5. std::list 請用成員函式 list::remove_if，效率更好也不失效。
//   6. C++20 起可用 std::erase_if(v, pred) 一行完成（見第 11 檔）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Erase-Remove 慣用法
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼刪除元素要寫成 `v.erase(std::remove(...), v.end())` 兩步驟？
//        std::remove 為什麼不自己把元素刪掉？
//     答：因為 std::remove 是【演算法】，它只拿到一對迭代器，
//         完全不知道這段區間屬於哪個容器——可能是 vector、
//         可能是原生陣列、也可能是別的東西。
//         既然不知道容器是誰，就無法呼叫它的成員函式，也就無法改變 size。
//         remove 能做的只有重排元素並回傳「新的邏輯結尾」；
//         真正縮小 size 只有容器的成員函式 erase 做得到。
//         這是 STL「容器與演算法正交」的直接代價。
//     追問：那 remove 之後，尾巴那些元素是什麼？
//         → 標準只說它們處於「有效但未指定」的狀態，不可讀取。
//           實作上是被移動走之後的殘骸——對 string 通常是空字串。
//
// 🔥 Q2. erase-remove 的複雜度是多少？和迴圈逐一 erase 差多少？
//     答：erase-remove 是 O(n)：remove_if 一次線性掃描（每個元素
//         最多搬一次），接著的 erase 因為 last 就是 end()、
//         後面沒有元素要搬，只需解構尾段。
//         迴圈逐一 erase 是 O(n²)：每刪一個都要搬移一次後半段。
//         元素多時差距可達數百倍（見本課第 12 檔的實測）。
//     追問：如果不需要保持順序，有更快的嗎？
//         → 有。swap-and-pop：把最後一個元素移到要刪的位置再 pop_back，
//           單一元素刪除是 O(1)。代價是順序被打亂。見第 13 檔。
//
// ⚠️ 陷阱. 有人寫了 `std::remove_if(v.begin(), v.end(), pred);`
//         然後印出 v，發現前面幾個確實是想保留的元素，
//         就以為刪除成功了。這錯在哪？
//     答：size 完全沒有改變。remove_if 只是把要保留的元素往前搬，
//         容器後面仍然掛著同樣數量的「殘骸」元素。
//         印出 v 會看到前段正確、後段是不該存在的垃圾，
//         而 v.size() 仍是原本的數字。
//         少了 v.erase(newEnd, v.end()) 這一步，
//         這些殘骸會一直留著，後續任何遍歷、加總、輸出都是錯的。
//     為什麼會錯：被函式名字誤導。remove 這個名字讓人以為它會移除，
//         但它其實是「把不要的往後推」的重排演算法。
//         更根本的原因是沒有意識到「演算法碰不到容器」這條 STL 鐵律——
//         任何會改變 size 的事，只能由容器的成員函式完成。
//         本檔的 main 會把「只 remove 不 erase」的中間狀態印出來。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <algorithm>
#include <string>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 27. Remove Element
//   題目：給定陣列 nums 和數值 val，就地移除所有等於 val 的元素，
//         回傳新長度 k；nums 的前 k 個元素必須是保留下來的元素（順序不限）。
//   為什麼用到本主題：這題就是 std::remove 的定義本身——
//     題目要的「回傳新長度、前 k 個是保留的元素」，
//     正好對應 remove 回傳的「新邏輯結尾」。
//     而題目【不允許】改變陣列大小（只回傳長度），
//     這恰恰解釋了為什麼 std::remove 只搬移不刪除：
//     它服務的正是這種「無法或不該改變容器大小」的場合。
//   複雜度：O(n) 時間、O(1) 額外空間。
// -----------------------------------------------------------------------------
int removeElement(std::vector<int>& nums, int val) {
    // std::remove 把不等於 val 的元素往前搬，回傳新的邏輯結尾
    auto newEnd = std::remove(nums.begin(), nums.end(), val);
    // LeetCode 只要長度，不要我們真的縮小陣列 —— 所以【不】呼叫 erase
    return static_cast<int>(newEnd - nums.begin());
}

// -----------------------------------------------------------------------------
// 【日常實務範例】清理設定檔：移除註解行與空白行
//   情境：讀進一份設定檔的所有行，把註解（# 開頭）與空白行濾掉，
//         只留下真正的設定項目，而且必須維持原本的順序。
//   為什麼用到本主題：這是 erase-remove 最典型的實務用途——
//     「一次篩掉一批不連續的元素，並保持順序」。
//     用迴圈逐一 erase 是 O(n²)，設定檔幾千行時就有感；
//     erase-remove 是 O(n)，而且只有一行。
//   保序性：std::remove_if 保證保留下來的元素維持原相對順序，
//     這對設定檔（後面的設定覆蓋前面的）是必要的。
// -----------------------------------------------------------------------------
bool isBlankOrComment(const std::string& line) {
    // 找第一個非空白字元
    size_t i = line.find_first_not_of(" \t\r\n");
    if (i == std::string::npos) return true;   // 整行都是空白
    return line[i] == '#';                     // 第一個非空白字元是 #
}

void stripNoise(std::vector<std::string>& lines) {
    lines.erase(std::remove_if(lines.begin(), lines.end(), isBlankOrComment),
                lines.end());
}

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    std::cout << "=== 原始示範：Erase-Remove 慣用法（一行）===\n";
    std::cout << "刪除前: ";
    for (int x : v) std::cout << x << " ";
    std::cout << "  (size=" << v.size() << ")\n";

    // Erase-Remove 慣用法（一行）
    v.erase(std::remove_if(v.begin(), v.end(),
                           [](int x) { return x % 2 == 0; }),
            v.end());

    std::cout << "刪除偶數後: ";
    for (int x : v) std::cout << x << " ";  // 1 3 5 7 9
    std::cout << "  (size=" << v.size() << ")\n";

    std::cout << "\n=== 拆解：只 remove 不 erase 會發生什麼 ===\n";
    {
        std::vector<int> w = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        std::cout << "原始:            ";
        for (int x : w) std::cout << x << " ";
        std::cout << "  size=" << w.size() << "\n";

        auto newEnd = std::remove_if(w.begin(), w.end(),
                                     [](int x) { return x % 2 == 0; });

        std::cout << "remove_if 之後:  ";
        for (int x : w) std::cout << x << " ";
        std::cout << "  size=" << w.size() << "  ← size 完全沒變！\n";
        std::cout << "新邏輯結尾的索引: " << (newEnd - w.begin()) << "\n";
        std::cout << "有效資料只有前 " << (newEnd - w.begin()) << " 個: ";
        for (auto it = w.begin(); it != newEnd; ++it) std::cout << *it << " ";
        std::cout << "\n";
        std::cout << "後面 " << (w.end() - newEnd)
                  << " 個是「有效但未指定」的殘骸，不可讀取。\n";
        std::cout << "（上面那行把它們印出來只是為了展示問題——正式程式碼不該這樣做）\n";

        w.erase(newEnd, w.end());       // 補上關鍵的第二步
        std::cout << "erase 之後:      ";
        for (int x : w) std::cout << x << " ";
        std::cout << "  size=" << w.size() << "\n";
    }

    std::cout << "\n=== 用回傳值算出「刪掉了幾個」===\n";
    {
        std::vector<int> a = {5, 3, 5, 1, 5, 9, 5};
        auto ne = std::remove(a.begin(), a.end(), 5);
        auto removed = std::distance(ne, a.end());
        a.erase(ne, a.end());
        std::cout << "從 {5,3,5,1,5,9,5} 刪除所有 5 -> ";
        for (int x : a) std::cout << x << " ";
        std::cout << "，共刪除 " << removed << " 個\n";
        std::cout << "（不要在述詞裡放計數器——標準未規定述詞被呼叫幾次）\n";
    }

    std::cout << "\n=== remove_if 是穩定的：保留原相對順序 ===\n";
    {
        std::vector<std::string> names = {
            "alice", "BOB", "carol", "DAVE", "eve", "FRANK"
        };
        names.erase(std::remove_if(names.begin(), names.end(),
                                   [](const std::string& s) {
                                       return !s.empty() && std::isupper(
                                           static_cast<unsigned char>(s[0]));
                                   }),
                    names.end());
        std::cout << "刪除大寫開頭的: ";
        for (const auto& s : names) std::cout << s << " ";
        std::cout << "  ← 順序與原本一致（標準保證）\n";
    }

    std::cout << "\n=== LeetCode 27 Remove Element ===\n";
    {
        auto run = [](std::vector<int> nums, int val) {
            std::cout << "  nums=[";
            for (size_t i = 0; i < nums.size(); ++i) std::cout << (i ? "," : "") << nums[i];
            std::cout << "], val=" << val;
            int k = removeElement(nums, val);
            std::cout << "  ->  k=" << k << ", 前 k 個=[";
            for (int i = 0; i < k; ++i) std::cout << (i ? "," : "") << nums[i];
            std::cout << "]\n";
        };
        run({3, 2, 2, 3}, 3);
        run({0, 1, 2, 2, 3, 0, 4, 2}, 2);
        run({1}, 1);
        std::cout << "  注意這題【不】呼叫 erase——題目只要長度，不要縮小陣列。\n";
        std::cout << "  這正好說明 std::remove 為什麼只搬移不刪除：\n";
        std::cout << "  它服務的就是這種「不能改變容器大小」的場合。\n";
    }

    std::cout << "\n=== 日常實務：清理設定檔的註解與空白行 ===\n";
    {
        std::vector<std::string> config = {
            "# 資料庫設定",
            "db.host = 10.0.0.5",
            "",
            "db.port = 5432",
            "   # 這行縮排後才是註解",
            "\t",
            "db.name = production",
            "# db.password = 已移到環境變數",
            "cache.ttl = 300",
        };

        std::cout << "原始 " << config.size() << " 行\n";
        stripNoise(config);
        std::cout << "清理後 " << config.size() << " 行：\n";
        for (const auto& l : config) std::cout << "  " << l << "\n";
        std::cout << "→ 一行 erase-remove，O(n)，而且保證維持原順序\n";
        std::cout << "  （設定檔常有「後面覆蓋前面」的語意，保序是必要的）\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 15 課：vector 元素刪除10.cpp -o demo10

// === 預期輸出 ===
// === 原始示範：Erase-Remove 慣用法（一行）===
// 刪除前: 1 2 3 4 5 6 7 8 9 10   (size=10)
// 刪除偶數後: 1 3 5 7 9   (size=5)
//
// === 拆解：只 remove 不 erase 會發生什麼 ===
// 原始:            1 2 3 4 5 6 7 8 9 10   size=10
// remove_if 之後:  1 3 5 7 9 6 7 8 9 10   size=10  ← size 完全沒變！
// 新邏輯結尾的索引: 5
// 有效資料只有前 5 個: 1 3 5 7 9
// 後面 5 個是「有效但未指定」的殘骸，不可讀取。
// （上面那行把它們印出來只是為了展示問題——正式程式碼不該這樣做）
// erase 之後:      1 3 5 7 9   size=5
//
// === 用回傳值算出「刪掉了幾個」===
// 從 {5,3,5,1,5,9,5} 刪除所有 5 -> 3 1 9 ，共刪除 4 個
// （不要在述詞裡放計數器——標準未規定述詞被呼叫幾次）
//
// === remove_if 是穩定的：保留原相對順序 ===
// 刪除大寫開頭的: alice carol eve   ← 順序與原本一致（標準保證）
//
// === LeetCode 27 Remove Element ===
//   nums=[3,2,2,3], val=3  ->  k=2, 前 k 個=[2,2]
//   nums=[0,1,2,2,3,0,4,2], val=2  ->  k=5, 前 k 個=[0,1,3,0,4]
//   nums=[1], val=1  ->  k=0, 前 k 個=[]
//   注意這題【不】呼叫 erase——題目只要長度，不要縮小陣列。
//   這正好說明 std::remove 為什麼只搬移不刪除：
//   它服務的就是這種「不能改變容器大小」的場合。
//
// === 日常實務：清理設定檔的註解與空白行 ===
// 原始 9 行
// 清理後 4 行：
//   db.host = 10.0.0.5
//   db.port = 5432
//   db.name = production
//   cache.ttl = 300
// → 一行 erase-remove，O(n)，而且保證維持原順序
//   （設定檔常有「後面覆蓋前面」的語意，保序是必要的）
