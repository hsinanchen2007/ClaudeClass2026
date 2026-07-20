// =============================================================================
//  第 16 課：vector 的迭代器操作 5  —  crbegin() / crend()：唯讀 + 反向
// =============================================================================
//
// 【主題資訊 Information】
//   const_reverse_iterator crbegin() const noexcept;  // 唯讀，指向最後一個元素
//   const_reverse_iterator crend()   const noexcept;  // 唯讀，指向第一個元素之前
//   標頭檔：<vector>
//   標準版本：C++11（與 cbegin()/cend() 同批加入）
//   複雜度：O(1)
//   型別展開：const_reverse_iterator = std::reverse_iterator<const_iterator>
//
// 【詳細解釋 Explanation】
//
// 【1. 四組迭代器其實是兩個獨立維度的組合】
//   vector 的八個迭代器函式，看起來很多，其實只是兩個是非題的乘積：
//                        │  可寫            │  唯讀（c 前綴）
//       ─────────────────┼─────────────────┼──────────────────────
//       正向             │  begin / end     │  cbegin / cend
//       反向（r 前綴）   │  rbegin / rend   │  crbegin / crend
//   命名規則就是把修飾詞疊上去：c 表 const、r 表 reverse。
//   理解了 const_iterator（第 4 個檔案）與 reverse_iterator（第 3 個檔案），
//   crbegin() 不需要任何新知識 —— 它就是
//       std::reverse_iterator<std::vector<T>::const_iterator>
//   兩個 adaptor 疊在一起而已。
//
// 【2. base() 在 const 版一樣差一格】
//   crbegin().base() == cend()、crend().base() == cbegin()，
//   而且 base() 回傳的是 const_iterator（唯讀性質會被保留，不會被偷偷拿掉）。
//   所以第 3 個檔案講的「差一格」規則在這裡完全適用。
//
// 【3. 什麼時候真的會用到 crbegin()】
//   最典型的是「對一個 const 容器做反向走訪」。此時 rbegin() 其實也能用
//   （const 容器的 rbegin() 多載本來就回傳 const_reverse_iterator），
//   但如果容器是非 const、你又只想讀，就只有 crbegin() 能保證唯讀 ——
//   理由與 cbegin() 完全相同：auto 會把型別藏起來，你需要一個
//   「無論容器 const 與否都回傳唯讀迭代器」的入口。
//   常見場景：顯示最近 N 筆記錄、從尾端找最新的一筆、由新到舊列出歷史。
//
// 【概念補充 Concept Deep Dive】
//   const_reverse_iterator 是兩層 adaptor 的疊加，展開後大致是：
//       reverse_iterator< __normal_iterator<const T*, vector<T>> >
//   每一層都只包一個成員、函式全部 inline，所以疊兩層在 -O2 之後仍然沒有
//   額外執行期成本。這也是 STL 大量使用 adaptor 的底氣：
//   抽象層是編譯期的概念，不是執行期的物件。
//   另外注意型別轉換方向：reverse_iterator → const_reverse_iterator 可以
//   隱式轉換（因為內層 iterator → const_iterator 可以），反向依然不行。
//
// 【注意事項 Pay Attention】
//   1. *v.crend() 是 UB；對空 vector，crbegin() == crend()，也不可解參考。
//   2. crbegin() 得到的是唯讀迭代器，*it = x 會在編譯期失敗（這正是重點）。
//   3. 唯讀不等於不會失效：容器重新配置後 const_reverse_iterator 一樣失效。
//   4. 若只是要「反向印出一個 const 容器」，直接用 rbegin() 也對；
//      crbegin() 的價值在非 const 容器上明確表達唯讀意圖。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】crbegin() / crend()
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. const_reverse_iterator 是什麼型別的組合？crbegin().base() 指向哪裡？
//     答：它是 std::reverse_iterator<const_iterator>，兩層 adaptor 疊加。
//         base() 規則不變，一樣差一格：crbegin().base() == cend()，
//         crend().base() == cbegin()，且回傳型別是 const_iterator。
//     追問：疊兩層 adaptor 會不會比較慢？
//         → 不會。每層只含一個成員、函式全部 inline，-O2 後與直接操作指標等價。
//
// ⚠️ 陷阱. 容器是非 const 時，rbegin() 和 crbegin() 有什麼差別？寫錯會怎樣？
//     答：對非 const 容器，rbegin() 回傳可寫的 reverse_iterator，
//         crbegin() 回傳唯讀的 const_reverse_iterator。
//         寫成 for (auto it = v.rbegin(); ...) 時，迴圈裡的 *it = x 會編譯成功，
//         於是「本來只想讀」的迴圈變成可以誤改資料，而且不會有任何警告。
//     為什麼會錯：一般人以為 r 系列函式「反正是反向走訪，都差不多」，
//         忽略了 const 與 reverse 是兩個獨立維度；auto 又把型別藏起來，
//         使誤寫完全看不出來。只想讀就用 c 開頭的版本，讓編譯器幫你把關。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <iomanip>

void demo_const_reverse() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // 反向 + 唯讀
    std::cout << "常數反向遍歷：";
    for (auto rit = v.crbegin(); rit != v.crend(); ++rit) {
        std::cout << *rit << " ";
        // *rit = 100;  // 編譯錯誤！const_reverse_iterator 不可修改元素
    }
    std::cout << std::endl;

    // base() 在 const 版一樣差一格
    std::cout << std::boolalpha;
    std::cout << "crbegin().base() == cend()   : " << (v.crbegin().base() == v.cend())   << std::endl;
    std::cout << "crend().base()   == cbegin() : " << (v.crend().base()   == v.cbegin()) << std::endl;
    std::cout << "*crbegin() = " << *v.crbegin()
              << "，*(crbegin().base()-1) = " << *(v.crbegin().base() - 1)
              << "（同一個元素）" << std::endl;

    // 空容器：crbegin() == crend()，迴圈安全不執行
    std::vector<int> empty_v;
    std::cout << "空 vector 的 crbegin() == crend()：" << (empty_v.crbegin() == empty_v.crend())
              << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】交易明細：由新到舊列出最近 N 筆（唯讀）
//   情境：帳務頁面預設顯示「最近 5 筆交易」，資料是依時間遞增 append 的，
//         所以最新的在尾端。這個函式只負責呈現，絕不該改動交易紀錄。
//   為什麼用到本主題：crbegin() 一次滿足兩個需求 ——「從最新開始」（reverse）
//         與「保證不修改帳務資料」（const）。若寫成 rbegin()，
//         迴圈裡誤寫 it->amount = 0 會編譯成功，帳目就被改掉了。
// -----------------------------------------------------------------------------
struct Transaction {
    std::string date;
    std::string item;
    double amount;
};

void printRecentTransactions(const std::vector<Transaction>& txs, std::size_t n) {
    std::size_t shown = 0;
    for (auto rit = txs.crbegin(); rit != txs.crend() && shown < n; ++rit, ++shown) {
        // rit->amount = 0;   // ← 編譯錯誤：唯讀迭代器擋下誤改帳務
        std::cout << "  " << rit->date
                  << "  " << std::left << std::setw(12) << rit->item
                  << std::right << std::fixed << std::setprecision(2)
                  << std::setw(10) << rit->amount << std::endl;
    }
    if (shown == 0) std::cout << "  (無交易紀錄)" << std::endl;
}

int main() {
    std::cout << "=== crbegin() / crend() 基本行為 ===" << std::endl;
    demo_const_reverse();

    std::cout << "\n=== 日常實務：最近 3 筆交易（由新到舊）===" << std::endl;
    std::vector<Transaction> txs = {
        {"2026-07-14", "電費",     1280.00},
        {"2026-07-15", "雲端訂閱",  600.00},
        {"2026-07-16", "書籍",      949.50},
        {"2026-07-18", "餐費",      320.00},
        {"2026-07-19", "交通",       55.00}
    };
    printRecentTransactions(txs, 3);

    std::cout << "\n=== 空紀錄的情況 ===" << std::endl;
    std::vector<Transaction> none;
    printRecentTransactions(none, 3);

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 16 課：vector 的迭代器操作5.cpp" -o iter5

// === 預期輸出 ===
// === crbegin() / crend() 基本行為 ===
// 常數反向遍歷：50 40 30 20 10 
// crbegin().base() == cend()   : true
// crend().base()   == cbegin() : true
// *crbegin() = 50，*(crbegin().base()-1) = 50（同一個元素）
// 空 vector 的 crbegin() == crend()：true
//
// === 日常實務：最近 3 筆交易（由新到舊）===
//   2026-07-19  交通           55.00
//   2026-07-18  餐費          320.00
//   2026-07-16  書籍          949.50
//
// === 空紀錄的情況 ===
//   (無交易紀錄)
