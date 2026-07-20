// =============================================================================
//  第 13 課：vector 元素新增：push_back、emplace_back1.cpp
//    —  push_back 最基本的三種傳值方式：複製、臨時物件、std::move
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<vector>
//   void push_back(const T& value);   // C++03 起：複製建構一份到尾端
//   void push_back(T&& value);        // C++11 起：移動建構到尾端
//   複雜度：攤銷 O(1)（amortized constant）——單次可能因 reallocation 而是 O(n)
//
// 【詳細解釋 Explanation】
//
// 【1. 兩個重載，編譯器替你選】
// push_back 有兩個重載，差別只在參數是 const T& 還是 T&&。你不需要手動選，
// 編譯器依「你傳進去的東西是 lvalue 還是 rvalue」自動決定：
//     v.push_back(hello);        // hello 有名字，是 lvalue → const T&  → 複製
//     v.push_back("World");      // 由字面量造出的臨時 string 是 rvalue → T&& → 移動
//     v.push_back(move(hello));  // move 把 lvalue 轉成 rvalue        → T&& → 移動
// 對 vector<int> 這種內建型別，複製與移動成本相同，差別可以忽略；
// 對 vector<string> 這種持有 heap 資源的型別，移動只是搬指標，差別很大。
//
// 【2. std::move 沒有「移動」任何東西】
// std::move 只是一個 static_cast<T&&>，它不搬資料、不產生任何機器碼，
// 純粹是「把這個 lvalue 標記成可被偷走的 rvalue」，好讓重載決議選中 T&& 版本。
// 真正搬資料的是後續被呼叫的移動建構子。
//
// 【3. 被 move 走之後的物件處於什麼狀態】
// 標準的說法是 "valid but unspecified state"（有效但未指定）：
//     * 有效：你可以安全地解構它、也可以重新賦值給它。
//     * 未指定：**不保證**內容是什麼，讀它的值沒有意義。
// libstdc++ 實測 string 被 move 後是空字串，但這是實作行為，
// 標準沒有這個保證，寫程式時絕不可依賴。
//
// 【概念補充 Concept Deep Dive】
// push_back("World") 其實做了兩件事，而不是一件：
//   ① const char* "World" 不是 std::string，先隱式呼叫 string 的建構子造出臨時物件
//   ② 這個臨時物件是 rvalue，被移動建構到 vector 內部
//   ③ 臨時物件解構
// 也就是說仍有一次「臨時 string 的建構與解構」。
// 這一步正是 emplace_back("World") 能省掉的——它把 "World" 完美轉發進去，
// 直接在 vector 的記憶體上建構 string，全程沒有臨時物件。
// （但注意：只有這種「本來就要造臨時物件」的情況 emplace_back 才贏，
//   傳現成物件時兩者完全一樣，詳見本課第 8 個範例檔。）
//
// 【注意事項 Pay Attention】
// 1. 被 move 走的物件不要再讀它的值——那是 unspecified，不是「保證為空」。
// 2. push_back 回傳 void，不能鏈式呼叫；要鏈式請用 C++17 的 emplace_back。
// 3. push_back 是**攤銷** O(1)，觸發 reallocation 的那一次是 O(n)。
// 4. 任何一次 reallocation 都會讓所有 iterator / pointer / reference 失效。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】push_back 的複製與移動
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. v.push_back(x) 什麼時候是複製、什麼時候是移動？
//     答：看 x 的值類別(value category)。x 是具名變數（lvalue）→ 選中
//         const T& 重載 → 複製；x 是臨時物件或 std::move(x)（rvalue）→ 選中
//         T&& 重載 → 移動。編譯器透過重載決議自動選，不需要手動指定。
//     追問：那 push_back(v[0]) 呢？→ v[0] 回傳 T&，是 lvalue，所以是複製。
//
// 🔥 Q2. std::move 到底做了什麼事？
//     答：什麼都沒做。它只是一個 static_cast<T&&>，把 lvalue 轉型成 rvalue
//         reference，好讓重載決議挑中移動版本。它不搬任何資料、不產生機器碼。
//         實際搬移動作發生在之後被選中的移動建構子裡。
//     追問：那對 const 物件 move 會怎樣？
//         → const T&& 無法繫結到 T&&，會退回 const T&，**默默變成複製**，
//           而且沒有任何警告。這是實務上很常見的效能陷阱。
//
// ⚠️ 陷阱 Q3. 「被 std::move 之後的 string 一定變成空字串」——對嗎？
//     答：錯。標準只保證處於 "valid but unspecified state"：可以安全解構、
//         可以重新賦值，但**內容未指定**。libstdc++ 實測是空字串，
//         這是實作行為，不是標準保證。libc++ 對短字串（SSO 範圍內）
//         甚至可能保留原內容，因為短字串的「移動」其實就是複製。
//     為什麼會錯：把某個編譯器上觀察到的結果當成語言規則。
//         正確心態是「被 move 走的物件，除了重新賦值以外什麼都別做」。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <string>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 118. Pascal's Triangle
//   題目：給定 numRows，回傳巴斯卡三角形的前 numRows 列。
//   為什麼用到本主題：這是「逐步累積結果」最純粹的形式——外層每算完一列就
//                     push_back 進結果，內層每算完一個數就 push_back 進該列。
//                     列數與每列長度都事先已知，正好示範 reserve + push_back。
// -----------------------------------------------------------------------------
std::vector<std::vector<int>> generatePascal(int numRows) {
    std::vector<std::vector<int>> triangle;
    triangle.reserve(static_cast<size_t>(numRows));

    for (int r = 0; r < numRows; ++r) {
        std::vector<int> row;
        row.reserve(static_cast<size_t>(r) + 1);   // 第 r 列有 r+1 個數
        for (int c = 0; c <= r; ++c) {
            if (c == 0 || c == r) {
                row.push_back(1);                  // 兩端固定是 1
            } else {
                // 上一列相鄰兩數之和
                row.push_back(triangle[r - 1][c - 1] + triangle[r - 1][c]);
            }
        }
        // row 之後不再使用 → move 進去，避免整列的深複製
        triangle.push_back(std::move(row));
    }
    return triangle;
}

int main() {
    std::cout << "=== push_back 基本用法（vector<int>）===" << std::endl;
    std::vector<int> v;

    v.push_back(10);
    v.push_back(20);
    v.push_back(30);

    for (int x : v) {
        std::cout << x << " ";  // 10 20 30
    }
    std::cout << std::endl;

    std::cout << "\n=== 三種傳值方式（vector<string>）===" << std::endl;
    std::vector<std::string> words;

    std::string hello = "Hello";
    words.push_back(hello);             // lvalue → const T& 重載 → 複製
    words.push_back("World");           // 由字面量造臨時 string → T&& 重載 → 移動
    words.push_back(std::move(hello));  // move 轉成 rvalue → T&& 重載 → 移動

    // 被 move 走的物件是 "valid but unspecified"：
    // 只檢查它是否為空，不把「一定是空字串」當成語言保證。
    std::cout << "被 move 後 hello.empty() = " << std::boolalpha << hello.empty()
              << std::endl;
    std::cout << "（libstdc++ 實測值；標準只保證 valid but unspecified，"
                 "不保證內容）" << std::endl;

    std::cout << "vector 內容: ";
    for (const auto& w : words) {
        std::cout << w << " ";  // Hello World Hello
    }
    std::cout << std::endl;

    std::cout << "\n=== LeetCode 118. Pascal's Triangle ===" << std::endl;
    for (const auto& row : generatePascal(5)) {
        std::cout << "  ";
        for (int x : row) std::cout << x << " ";
        std::cout << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 13 課：vector 元素新增：push_back、emplace_back1.cpp" -o push_back_basic

// （libstdc++ 實測值；標準只保證 valid but unspecified，不保證內容）

// === 預期輸出 ===
// === push_back 基本用法（vector<int>）===
// 10 20 30 
// 
// === 三種傳值方式（vector<string>）===
// 被 move 後 hello.empty() = true
// vector 內容: Hello World Hello 
// 
// === LeetCode 118. Pascal's Triangle ===
//   1 
//   1 1 
//   1 2 1 
//   1 3 3 1 
//   1 4 6 4 1 
