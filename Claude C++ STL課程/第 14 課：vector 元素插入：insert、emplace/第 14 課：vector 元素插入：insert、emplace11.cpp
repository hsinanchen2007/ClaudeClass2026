// =============================================================================
//  第 14 課：vector 元素插入：insert、emplace11.cpp
//    —  邊走訪邊插入：在標記位置插入內容並移除標記
// =============================================================================
//
// 【主題資訊 Information】
//   iterator insert(const_iterator pos, const T& value);  // 回傳指向新元素
//   iterator erase(const_iterator pos);                   // 回傳指向被刪元素的「下一個」
//   標頭檔: <vector>
//   標準版本: C++11(兩者的 iterator 回傳值語意在 C++11 定案)
//   複雜度: 每次 insert/erase 都是 O(n);走訪本身 O(n)
//
// 【詳細解釋 Explanation】
//
// 【1. 這個模式的核心難點:兩個函式的回傳值語意「不對稱」】
//     insert 回傳 → 指向**新插入的元素**(位置往前)
//     erase  回傳 → 指向**被刪元素的下一個**(位置往後)
// 走訪迴圈中混用這兩者,最容易出錯的就是「插入/刪除之後 it 到底在哪」。
// 唯一可靠的做法是每一步都問自己「現在 it 指向哪個元素」,
// 而不是憑感覺 ++/--。本檔逐行標註了每一步的狀態。
//
// 【2. 逐步追蹤本檔的流程】
//   起始:  {L1, L2, MARK, L3, L4},it 指向 MARK(索引 2)
//   insert(it, "New Content")
//     → {L1, L2, NEW, MARK, L3, L4},回傳指向 NEW(索引 2),it = NEW
//   ++it
//     → it 指向 MARK(索引 3)
//   erase(it)
//     → {L1, L2, NEW, L3, L4},回傳指向被刪元素的下一個 = L3(索引 3),it = L3
//   結果: {L1, L2, NEW, L3, L4} —— 標記被換成了新內容
//
// 【3. 為什麼「邊走訪邊修改」在 vector 上特別危險】
// 每次 insert/erase 都可能讓迴圈變數 it 失效。本檔因為插入後**立刻**用回傳值
// 更新 it、而且處理完就 break,所以是安全的。但若要處理**多個**標記而繼續迴圈,
// 就必須確保每一輪都拿到有效的 it —— 而且別忘了 `end()` 也會失效,
// `it != lines.end()` 的比較對象每輪都要重新取得(range-based for 迴圈
// 在迴圈開始時就固定了 end,所以**絕對不能**在 range-for 裡插入元素)。
//
// 【4. 更好的替代方案】
// 需要處理多處插入時,逐一 insert 是 O(k×n)。實務上更常用的是
// 「**建一個新 vector**」:走訪原容器,遇到標記就 push_back 新內容、
// 否則 push_back 原內容,最後 swap 回去。這是 O(n) 且完全不必煩惱失效問題,
// 程式碼也更好讀。本檔末段示範了這個做法。
//
// 【概念補充 Concept Deep Dive】
// 「邊走訪邊修改容器」是 STL 最經典的 bug 溫床,各容器規則不同:
//   * vector : insert/erase 後 it 一律要用回傳值更新(可能全失效)
//   * list   : insert 不使任何 iterator 失效;erase 只讓被刪的那個失效
//   * map/set: 同 list —— erase 只讓被刪節點失效,其他不受影響
// C++20 起對常見的「條件刪除」提供了 std::erase / std::erase_if,
// 直接一行解決,不必自己寫 remove-erase idiom:
//     std::erase_if(lines, [](const std::string& s){ return s.empty(); });
// (注意這是 C++20;C++17 以前要寫 lines.erase(std::remove_if(...), lines.end()))
//
// 【注意事項 Pay Attention】
// 1. **絕對不要在 range-based for 迴圈中 insert/erase**。
//    range-for 展開後 end() 在迴圈開始前就取好了,插入後它已失效,
//    迴圈條件比較的是懸空 iterator → UB。
// 2. erase 回傳的是「下一個」,所以刪除後**不要**再 ++it,否則會跳過一個元素。
//    這是 remove-erase 迴圈最常見的 off-by-one。
// 3. 若要處理多個位置,優先考慮「建新 vector」而非原地反覆 insert ——
//    前者 O(n) 且無失效風險,後者 O(k×n) 且容易寫錯。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】走訪中插入與刪除
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. insert 與 erase 的回傳值分別指向哪裡?為什麼設計得不一樣?
//     答：insert 回傳指向「新插入的元素」;erase 回傳指向「被刪元素的下一個」。
//         不一樣是因為兩者要讓走訪能自然接續:插入後你通常想從新元素開始處理,
//         刪除後那個位置的元素已經不存在,只能給你下一個。
//     追問：所以 erase 之後還要不要 ++it?→ 不要。erase 已經幫你前進了,
//         再 ++ 會跳過一個元素,這是最常見的 off-by-one。
//
// ⚠️ 陷阱. 為什麼絕對不能在 range-based for 裡面 insert?
//         for (auto& line : lines) { if (...) lines.insert(...); }
//     答：range-for 展開後大致是
//           auto __end = lines.end();
//           for (auto __it = lines.begin(); __it != __end; ++__it)
//         __end 在迴圈**開始前**就取好了。insert 之後它已失效,
//         之後每一輪的 `__it != __end` 比較都是 UB。
//     為什麼會錯：range-for 把 iterator 藏起來了,看不到就以為不存在。
//         它其實只是語法糖,底下的失效規則一條都沒少。
//
// 🔥 Q2. 要把 vector 中所有符合條件的元素換成別的內容,怎麼做最好?
//     答：建一個新 vector,走訪原容器逐一 push_back(符合條件就放新內容),
//         最後 swap 回去。O(n)、無失效風險、可讀性最好。
//         原地反覆 insert/erase 是 O(k×n),而且每一步都要小心 iterator。
//     追問：只是要「刪除」符合條件的元素呢?→ C++20 用 std::erase_if 一行解決;
//         C++17 以前用 remove-erase idiom:
//         v.erase(std::remove_if(v.begin(), v.end(), pred), v.end())。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <string>

int main() {
    std::vector<std::string> lines = {
        "Line 1",
        "Line 2",
        "// INSERT HERE",
        "Line 3",
        "Line 4"
    };

    // 找到標記並插入新內容，然後移除標記
    // 逐步狀態：
    //   起始   {L1, L2, MARK, L3, L4}      it -> MARK
    //   insert {L1, L2, NEW, MARK, L3, L4} it -> NEW   (insert 回傳新元素)
    //   ++it                               it -> MARK
    //   erase  {L1, L2, NEW, L3, L4}       it -> L3    (erase 回傳下一個)
    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (*it == "// INSERT HERE") {
            it = lines.insert(it, "New Content");
            ++it;                  // 跳過剛插入的元素，現在指向標記
            it = lines.erase(it);  // 刪除標記，it 指向 "Line 3"
            break;                 // 只處理第一個標記
        }
    }

    for (const auto& line : lines) {
        std::cout << line << std::endl;
    }

    // -------------------------------------------------------------------------
    // 【日常實務範例 1】設定檔區段展開：把 @include 指示替換成實際內容
    // 情境：載入設定檔時，遇到 "@include database" 這種指示行，
    //       要展開成對應的多行設定。可能有多個指示、每個展開成多行。
    // 做法：不要原地反覆 insert（O(k*n) 且要小心失效），
    //       改成建新 vector 一次走完 —— O(n)、無失效風險、好讀。
    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務: 設定檔 @include 展開 ===" << std::endl;

    std::vector<std::string> config = {
        "app.name = demo",
        "@include database",
        "app.port = 8080",
        "@include logging"
    };

    std::vector<std::string> expanded;
    expanded.reserve(config.size() * 2);   // 粗估容量，減少 reallocation

    for (const auto& line : config) {
        if (line == "@include database") {
            expanded.push_back("db.host = localhost");
            expanded.push_back("db.port = 5432");
        } else if (line == "@include logging") {
            expanded.push_back("log.level = info");
            expanded.push_back("log.file = /var/log/demo.log");
        } else {
            expanded.push_back(line);
        }
    }
    config.swap(expanded);

    for (const auto& line : config) {
        std::cout << line << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 14 課：vector 元素插入：insert、emplace11.cpp" -o insert11

// === 預期輸出 ===
// Line 1
// Line 2
// New Content
// Line 3
// Line 4
//
// === 日常實務: 設定檔 @include 展開 ===
// app.name = demo
// db.host = localhost
// db.port = 5432
// app.port = 8080
// log.level = info
// log.file = /var/log/demo.log
