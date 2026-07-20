// =============================================================================
//  第 16 課：vector 的迭代器操作 8  —  範圍 for 迴圈：它其實就是迭代器
// =============================================================================
//
// 【主題資訊 Information】
//   for (declaration : range-expression) statement;
//   標準版本：C++11 引入；C++17（P0184R0）放寬 begin/end 可為不同型別
//   複雜度：與手寫迭代器迴圈完全相同，無額外成本
//   前置條件：range-expression 必須能取得 begin()/end()
//             （成員函式、或 ADL 找得到的自由函式 begin()/end()、或原生陣列）
//
// 【詳細解釋 Explanation】
//
// 【1. 編譯器實際展開成什麼】
//   C++17 起的展開大致是（省略若干細節）：
//       {
//           auto&& __range = range-expression;
//           auto   __begin = begin_expr;      // 通常是 __range.begin()
//           auto   __end   = end_expr;        // 通常是 __range.end()
//           for (; __begin != __end; ++__begin) {
//               declaration = *__begin;
//               statement;
//           }
//       }
//   三個重點藏在這段展開裡：
//     (a) __end 只求值「一次」，在迴圈開始前。所以迴圈中改變容器大小，
//         __end 不會跟著更新 —— 這是範圍 for 不能邊走邊增刪的根本原因。
//     (b) 終止條件是 !=，不是 <。這讓範圍 for 對所有迭代器類別都適用。
//     (c) declaration = *__begin，所以你寫 auto 就是拷貝、寫 auto& 就是引用
//         （第 9 個檔案會完整比較三種寫法）。
//
// 【2. C++11 與 C++17 的差別：sentinel 支援】
//   C++11/14 要求 __begin 與 __end 必須是同型別。C++17 的 P0184R0 放寬了
//   這個限制，允許 end 回傳一個「不同型別的哨兵（sentinel）」，只要
//   __begin != __end 有定義即可。
//   本機實測：一個 begin() 回傳 Iter、end() 回傳 Sentinel 的自訂型別，
//   以 -std=c++14 -pedantic-errors 編譯會報
//   "inconsistent begin/end types in range-based 'for' statement"，
//   改用 -std=c++17 則通過。這個放寬正是後來 C++20 Ranges（如
//   views::take_while、無限序列）能運作的語言基礎。
//
// 【3. 它找 begin/end 的順序】
//   (a) range-expression 是原生陣列 → 直接用陣列邊界，不呼叫任何函式。
//   (b) 型別有成員 begin() 且有成員 end() → 用成員版。
//   (c) 否則以 ADL 尋找自由函式 begin(__range) / end(__range)
//       （會把 std 命名空間納入考慮）。
//   這個規則讓「你無法修改原始碼的第三方型別」也能支援範圍 for ——
//   只要在它的命名空間裡提供自由函式 begin/end 即可。
//
// 【概念補充 Concept Deep Dive】
//   注意 auto&& __range 這個綁定。它是萬用引用，因此左值容器與右值暫時物件
//   都能綁；綁到暫時物件時，該暫時物件的生存期會被延長到迴圈結束，
//   所以 for (int x : makeVector()) 是安全的。
//   但生存期延長「只涵蓋直接綁定到 __range 的那個物件」。若 range-expression
//   是一個更複雜的運算式，中間產生的其他暫時物件在 C++20 以前不會被延長，
//   例如 for (char c : getConfig().name())，getConfig() 的回傳值在迴圈開始前
//   就已被銷毀，之後走訪的是懸空資料（UB）。C++23 的 P2718R0 擴大了
//   範圍 for 中暫時物件的生存期延長，改善了這一類寫法；
//   在你能確定編譯器與標準版本之前，最安全的做法仍是先把結果存進具名變數：
//       auto cfg = getConfig();
//       for (char c : cfg.name()) { ... }
//
// 【注意事項 Pay Attention】
//   1. 迴圈中不可 push_back / erase / resize 改變容器結構 —— __end 已固定，
//      且重新配置會讓 __begin 與 __end 全部失效，後續行為是 UB。
//   2. 範圍 for 拿不到「目前是第幾個元素」。需要索引時請寫傳統迴圈，
//      或自行維護計數器。
//   3. 需要修改元素必須寫 auto&；只讀且元素較大寫 const auto&。
//   4. 想反向走訪不能直接用範圍 for，要搭配 reverse adaptor 或手寫
//      rbegin()/rend() 迴圈。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】範圍 for 迴圈
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 範圍 for 迴圈會被編譯器展開成什麼？請寫出等價的傳統迴圈。
//     答：auto&& __range = 容器；auto __begin = __range.begin();
//         auto __end = __range.end(); for (; __begin != __end; ++__begin)
//         { declaration = *__begin; ... }
//         關鍵是 __end 只在迴圈前求值一次，且條件用 != 而非 <。
//     追問：那它怎麼支援原生陣列？
//         → 原生陣列是特例，編譯器直接用陣列的頭尾位址，不呼叫 begin()/end()。
//
// 🔥 Q2. 為什麼不能在範圍 for 裡對容器 push_back？
//     答：兩個原因疊加。__end 在迴圈開始前就取好且不再更新，
//         所以新增的元素不會被走訪；更嚴重的是 push_back 可能觸發重新配置，
//         使 __begin 與 __end 同時失效，之後的 ++、比較、解參考都是 UB。
//         正確做法是把要新增的內容先收集到另一個 vector，迴圈結束後再合併。
//
// ⚠️ 陷阱. 範圍 for 走訪暫時物件安全嗎？
//     答：直接綁定的暫時物件是安全的 —— for (int x : makeVector()) 中，
//         makeVector() 的結果綁到 auto&& __range，生存期延長到迴圈結束。
//         但若 range-expression 是更複雜的運算式（例如 f().member()），
//         中間的暫時物件在 C++20 以前不會被延長，走訪的可能是已銷毀的資料，
//         這是 UB —— 可能讀到看似正常的舊值，也可能讀到亂碼，不保證會崩潰。
//         C++23 的 P2718R0 擴大了這類生存期延長。
//     為什麼會錯：一般人記住「範圍 for 會延長暫時物件生存期」就以為所有寫法
//         都安全，卻沒注意到延長的只有「直接綁到 __range 的那一個」。
//         保險做法：先存成具名變數再走訪。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>

void demo_range_for() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // 範圍 for 迴圈
    for (int x : v) {
        std::cout << x << " ";
    }
    std::cout << std::endl;

    // 上面等同於（這就是編譯器實際展開的樣子）
    // 註：標準用的名字是 __range/__begin/__end，但「雙底線開頭」是保留給實作的
    //     識別字，使用者程式碼不可自行宣告，所以這裡改用一般名稱示範。
    {
        auto&& range_ref = v;
        auto   it_begin  = range_ref.begin();
        auto   it_end    = range_ref.end();   // 只求值一次！
        for (; it_begin != it_end; ++it_begin) {
            int x = *it_begin;
            std::cout << x << " ";
        }
    }
    std::cout << std::endl;

    // 原生陣列也適用（編譯器直接用陣列邊界，不呼叫 begin()/end()）
    int arr[4] = {1, 2, 3, 4};
    std::cout << "原生陣列：";
    for (int x : arr) std::cout << x << " ";
    std::cout << std::endl;

    // 綁定到暫時物件是安全的：生存期延長到迴圈結束
    std::cout << "走訪暫時物件：";
    for (int x : std::vector<int>{7, 8, 9}) std::cout << x << " ";
    std::cout << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】訂單批次結算：加總金額並統計超額筆數
//   情境：電商後台每天收盤時要把當日訂單跑一次，算出總營業額，
//         同時挑出金額超過風控門檻、需要人工複核的訂單編號。
//   為什麼用到本主題：這是範圍 for 最典型的用途 —— 唯讀走訪整批資料。
//         元素是含字串的結構，所以用 const auto& 避免每筆都複製一次；
//         而「需要複核的編號」另外收集到新 vector，
//         絕不在走訪途中對原容器做任何增刪（見【注意事項】1）。
// -----------------------------------------------------------------------------
struct Order {
    std::string id;
    std::string customer;
    double amount;
};

struct Settlement {
    double total = 0.0;
    std::vector<std::string> need_review;
};

Settlement settle(const std::vector<Order>& orders, double review_threshold) {
    Settlement s;
    for (const auto& o : orders) {        // const auto&：唯讀且零拷貝
        s.total += o.amount;
        if (o.amount > review_threshold) {
            s.need_review.push_back(o.id);   // 新增到「另一個」容器，安全
        }
    }
    return s;
}

int main() {
    std::cout << "=== 範圍 for 與它的展開形式 ===" << std::endl;
    demo_range_for();

    std::cout << "\n=== 日常實務：訂單批次結算 ===" << std::endl;
    std::vector<Order> orders = {
        {"A-1001", "Alice",   1280.0},
        {"A-1002", "Bob",    52000.0},
        {"A-1003", "Charlie",  349.5},
        {"A-1004", "Dora",   18800.0},
        {"A-1005", "Evan",     990.0}
    };
    Settlement s = settle(orders, 10000.0);
    std::cout << "訂單筆數 = " << orders.size() << std::endl;
    std::cout << "總營業額 = " << s.total << std::endl;
    std::cout << "需人工複核（>10000）：";
    for (const auto& id : s.need_review) std::cout << id << " ";
    std::cout << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 16 課：vector 的迭代器操作8.cpp" -o iter8

// === 預期輸出 ===
// === 範圍 for 與它的展開形式 ===
// 10 20 30 40 50 
// 10 20 30 40 50 
// 原生陣列：1 2 3 4 
// 走訪暫時物件：7 8 9 
//
// === 日常實務：訂單批次結算 ===
// 訂單筆數 = 5
// 總營業額 = 73419.5
// 需人工複核（>10000）：A-1002 A-1004 
