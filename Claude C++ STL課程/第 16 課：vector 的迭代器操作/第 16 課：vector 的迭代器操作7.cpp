// =============================================================================
//  第 16 課：vector 的迭代器操作 7  —  auto 與迭代器：簡潔之外的正確性
// =============================================================================
//
// 【主題資訊 Information】
//   auto it  = v.begin();    // 推導為 iterator（v 非 const 時）
//   auto it  = v.cbegin();   // 推導為 const_iterator
//   auto x   = *it;          // 拷貝一份元素
//   auto& x  = *it;          // 綁定到元素本身（可改）
//   const auto& x = *it;     // 綁定到元素本身（唯讀、不拷貝）
//   標準版本：auto 型別推導是 C++11
//   複雜度：純編譯期行為，執行期零成本
//
// 【詳細解釋 Explanation】
//
// 【1. auto 解決的第一個問題：型別名稱長到沒人想寫】
//   完整寫法是 std::vector<std::pair<std::string, int>>::const_iterator，
//   光是宣告一個迴圈變數就佔滿一行。更糟的是，當容器型別改變時
//   （vector 換成 deque），每一處明寫的迭代器型別都要跟著改。
//   auto 讓型別跟著來源自動走，是真正的解耦。
//
// 【2. auto 解決的第二個問題（更重要）：意外的隱式轉換與拷貝】
//   明寫型別時，如果寫錯但「剛好可以轉換」，編譯器會默默幫你轉，
//   而那個轉換往往是一次完整的拷貝。最有名的例子出現在 map：
//       std::map<std::string, int> m;
//       for (const std::pair<std::string, int>& p : m) { ... }
//   看起來完全正確，但 map 的元素型別其實是 pair<const std::string, int>
//   （key 是 const）。型別不匹配 → 編譯器建立一個暫時物件再綁定 →
//   每一輪迴圈都多複製一個 std::string。寫 const auto& 就不會有這個問題。
//   這就是 Scott Meyers 在《Effective Modern C++》Item 5 說的
//   「auto 讓你避開你根本沒察覺的型別不匹配」。
//
// 【3. auto 的推導規則：它會丟掉引用與 const】
//   auto 沿用 template 參數推導規則，預設會做 decay：
//       auto  x = *it;   // 拷貝，即使 *it 回傳的是引用
//       auto& x = *it;   // 綁定到本體，可修改
//       const auto& x = *it;  // 綁定到本體，唯讀
//   所以「要不要拷貝」這件事仍然要你自己決定，auto 不會替你猜。
//   對 vector<int> 拷貝一個 int 無所謂；對 vector<std::string> 就是每輪
//   一次堆積配置 + 一次字串複製。
//
// 【4. auto 與 cbegin() 的搭配】
//   對非 const 的容器，auto it = v.begin() 推導成可寫的 iterator。
//   若你的意圖是唯讀，要主動寫 v.cbegin()，讓編譯器替你把關。
//   這正是 C++11 加入 cbegin()/cend() 的理由（詳見第 4 個檔案）。
//
// 【概念補充 Concept Deep Dive】
//   auto 的推導在編譯期完成，產生的機器碼與明寫型別完全相同 —— 它不是
//   動態型別，也沒有任何執行期成本。真正該擔心的從來不是 auto 本身，
//   而是「你有沒有想清楚要值還是要引用」。
//   一個實用的判斷順序：
//       只讀且元素較大 → const auto&
//       要修改         → auto&
//       元素是 int/double 等純量、或本來就要一份副本 → auto
//   另外提醒：auto&& 是萬用引用，主要用在泛型程式碼與 proxy 容器
//   （例如 vector<bool>，見第 9 個檔案），一般 vector 用不到。
//
// 【注意事項 Pay Attention】
//   1. auto 會丟掉 const 與引用，要保留必須自己寫 auto& / const auto&。
//   2. auto it = v.begin() 對非 const 容器是可寫迭代器；想唯讀請用 cbegin()。
//   3. auto x = *it 對大型元素是一次完整拷貝，在熱路徑上是實際的效能問題。
//   4. auto 不會讓失效問題消失 —— 型別推導與迭代器有效性是兩件事。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】auto 與迭代器
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. auto it = v.begin(); 推導出的型別是什麼？如果 v 是 const 呢？
//     答：v 非 const 時推導為 std::vector<T>::iterator（可寫）；
//         v 是 const 時 begin() 的 const 多載生效，推導為 const_iterator。
//         若容器非 const 但你只想讀，必須主動用 cbegin()，auto 不會替你決定。
//     追問：那 auto 有沒有可能推導出 reference？
//         → 不會，auto 預設 decay 掉引用與 const；要引用得寫 auto& 或 const auto&。
//
// 🔥 Q2. 為什麼《Effective Modern C++》說用 auto 能避免效能問題？
//     答：明寫型別時若與實際型別不完全一致但可隱式轉換，編譯器會靜默產生
//         暫時物件。經典例子是走訪 map 時寫 const pair<string,int>&，
//         而實際元素是 pair<const string,int>，於是每輪多複製一個 string，
//         而且沒有任何警告。用 const auto& 型別必然吻合，不會有多餘拷貝。
//
// ⚠️ 陷阱. for (auto x : names) 與 for (const auto& x : names) 差在哪？
//         names 是 vector<std::string>。
//     答：前者每一輪都把字串完整拷貝一份（可能觸發堆積配置），迴圈結束就丟掉；
//         後者只綁定引用，零拷貝。對 10 萬筆長字串，兩者的差距是可測量的。
//     為什麼會錯：多數人把 auto 讀成「編譯器會挑最有效率的方式」，
//         但 auto 只做型別推導，值語意或引用語意完全由你寫的 &（或沒寫）決定。
//         養成習慣：唯讀走訪一律寫 const auto&，只有純量型別才省略。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <type_traits>

void demo_auto() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // 不用 auto（冗長，且換容器就得全部改）
    for (std::vector<int>::iterator it = v.begin(); it != v.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // 使用 auto（簡潔）
    for (auto it = v.begin(); it != v.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // 如果只讀取，用 cbegin() / cend()
    for (auto it = v.cbegin(); it != v.cend(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // 驗證推導結果：begin() 與 cbegin() 得到不同型別
    auto it_rw = v.begin();
    auto it_ro = v.cbegin();
    std::cout << std::boolalpha;
    std::cout << "auto+begin()  是 iterator？       "
              << std::is_same<decltype(it_rw), std::vector<int>::iterator>::value << std::endl;
    std::cout << "auto+cbegin() 是 const_iterator？ "
              << std::is_same<decltype(it_ro), std::vector<int>::const_iterator>::value << std::endl;

    // auto 會 decay：拷貝 vs 引用
    auto  copy_val = *v.begin();   // 拷貝
    auto& ref_val  = *v.begin();   // 引用
    copy_val = 999;                // 只改副本
    std::cout << "副本 copy_val = " << copy_val
              << "，但 v[0] = " << v[0] << "（未變）" << std::endl;
    ref_val = 999;                 // 改本體
    std::cout << "改 auto& 引用後 v[0] = " << v[0] << "（已變）" << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】CSV 資料列清洗：去除每個欄位的前後空白
//   情境：從匯出的 CSV 讀進來的欄位常帶有多餘空白（"  Alice ", "台北  "），
//         寫進資料庫前必須先 trim，否則查詢比對永遠對不上。
//   為什麼用到本主題：清洗要「就地修改」欄位，所以迴圈必須用 auto&；
//         若手滑寫成 auto，改的是副本，原始資料一個字都不會變，
//         而且編譯完全通過、沒有任何警告 —— 這是實務上很常見的無聲 bug。
// -----------------------------------------------------------------------------
std::vector<std::string> splitCsvRow(const std::string& line) {
    std::vector<std::string> fields;
    std::istringstream ss(line);
    std::string field;
    while (std::getline(ss, field, ',')) {
        fields.push_back(field);
    }
    return fields;
}

void trimFields(std::vector<std::string>& fields) {
    // 必須是 auto&：要修改容器裡的字串本身
    for (auto& f : fields) {
        const std::string ws = " \t";
        std::size_t b = f.find_first_not_of(ws);
        if (b == std::string::npos) { f.clear(); continue; }
        std::size_t e = f.find_last_not_of(ws);
        f = f.substr(b, e - b + 1);
    }
}

int main() {
    std::cout << "=== auto 與迭代器 ===" << std::endl;
    demo_auto();

    std::cout << "\n=== 日常實務：CSV 欄位 trim ===" << std::endl;
    std::string row = "  Alice ,\t台北  ,  28  ,   ";
    std::vector<std::string> fields = splitCsvRow(row);

    std::cout << "清洗前：";
    for (const auto& f : fields) std::cout << "[" << f << "]";
    std::cout << std::endl;

    trimFields(fields);

    std::cout << "清洗後：";
    for (const auto& f : fields) std::cout << "[" << f << "]";
    std::cout << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 16 課：vector 的迭代器操作7.cpp" -o iter7

// === 預期輸出 ===
// === auto 與迭代器 ===
// 10 20 30 40 50 
// 10 20 30 40 50 
// 10 20 30 40 50 
// auto+begin()  是 iterator？       true
// auto+cbegin() 是 const_iterator？ true
// 副本 copy_val = 999，但 v[0] = 10（未變）
// 改 auto& 引用後 v[0] = 999（已變）
//
// === 日常實務：CSV 欄位 trim ===
// 清洗前：[  Alice ][	台北  ][  28  ][   ]
// 清洗後：[Alice][台北][28][]
