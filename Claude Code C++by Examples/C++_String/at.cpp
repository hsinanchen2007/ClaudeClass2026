// =============================================================================
// 檔名: at.cpp
// 主題: std::string::at (帶邊界檢查的元素存取)
// 參考連結 References:
//   cppreference: https://en.cppreference.com/cpp/string/basic_string/at
//   cplusplus.com: https://cplusplus.com/reference/string/string/at/
// =============================================================================
//
// 【函式資訊 Information】
//   reference       at(size_type pos);
//   const_reference at(size_type pos) const;
//
// 回傳: 第 pos 個字元的參考 (reference)。
// 例外: 若 pos >= size(),會丟出 std::out_of_range。
//
// =============================================================================
//
// 【詳細解釋 Explanation】
//
// (一) at() 為什麼存在?
// ----------------------------------------------------------------------------
// 若 STL 只提供 operator[],那「來自外部、不確定範圍」的索引就只能由使用者
// 自己手動檢查邊界。這種「忘記檢查 → UB → 隨機崩潰」是 C/C++ 最常見的
// 安全漏洞之一 (buffer overflow)。at() 的存在就是把邊界檢查內建到呼叫端
// 的責任之外,讓越界自動變成 catchable exception。
//
// 對比表:
//   操作                | 邊界檢查 | 越界行為                  | 適用情境
//   --------------------|----------|---------------------------|------------------
//   at(pos)             | 有       | throw std::out_of_range   | 不可信輸入、防禦
//   operator[](pos)     | 無       | Undefined Behavior        | 熱迴圈、可信邊界
//   *(s.data() + pos)   | 無       | UB / SEGV                 | 與 C API 接合
//
// (二) 例外的代價:多大?
// ----------------------------------------------------------------------------
// at() 在「正常路徑 (沒越界)」幾乎沒額外成本 —— 只有一次 size 比較 + 條件
// 分支。CPU 的分支預測可以幾乎免費跳過。
// 但若真的丟出例外,就會涉及 stack unwinding,成本相對昂貴。所以原則:
//   - 預期幾乎不會越界 → at 是「安全又便宜」的好選擇
//   - 預期越界很常發生 → 應該先用 if 檢查,而非依賴 try/catch
//
// (三) at() 的時間複雜度
// ----------------------------------------------------------------------------
// O(1)。雖然多了一個 if 分支,但常數時間沒變,對 Big-O 分析無影響。
//
// (四) at(size()) 的行為
// ----------------------------------------------------------------------------
//   - operator[](s.size()) 在 const string 上合法 (回傳 '\0')。
//   - at(s.size()) 一律 throw std::out_of_range,不論 const 與否。
// 為什麼有這個差異?因為 at 的語意是「存取第 pos 個有效字元」,而 size()
// 是「結尾後的位置」,不算有效字元;標準刻意維持嚴格的語意。
//
// (五) const-correctness
// ----------------------------------------------------------------------------
// at() 有 const 與非 const 兩個版本,以維持 const-correctness:
//   const std::string& cs = ...;
//   cs.at(0) = 'X';   // 編譯失敗 — const_reference 不可寫
//   std::string ms = ...;
//   ms.at(0) = 'X';   // 合法 — reference 可寫
//
// (六) 與 std::vector::at、std::array::at、std::deque::at 的一致性
// ----------------------------------------------------------------------------
// 所有 STL 容器的 at() 行為一致:有檢查、越界丟 out_of_range。
// 這是「容器介面一致性」的設計典範,你學會 string::at 就等於學會所有容器。
//
// (七) C++ 標準演進
// ----------------------------------------------------------------------------
//   C++03 起即存在 (basic_string 的設計從一開始就有 at)。
//   C++11 起加入 noexcept 規範:at 不是 noexcept,因為它可能 throw。
//   C++20 起 at 在 constexpr 上下文可用 (constexpr basic_string)。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// 1) Defensive Programming vs Performance
//    防禦式程式設計 (defensive programming) 主張「假設輸入都不可信」,
//    處處檢查;高效能設計則信任 invariant 不浪費 cycle 在多餘檢查。
//    at vs operator[] 正是這個哲學分歧在 STL 介面上的具體呈現。
//    成熟工程師會「分層設計」:外部介面層用 at,內部熱路徑用 [],
//    讓檢查只發生在「資料進入信任邊界的那一刻」。
//
// 2) 為何 STL 用 exception 而非 return code?
//    在「應該幾乎不會發生」的錯誤路徑上,exception 比 return code 更乾淨:
//      - 不污染回傳值 (at() 必須回傳 char&,沒有「無效值」可回傳)
//      - 自動傳遞到能處理的層級,不會被忘記檢查
//      - C++ 標準函式庫整體採用 exception 處理 logic_error 與 runtime_error
//
// 3) std::out_of_range 的位階
//    它繼承自 std::logic_error,語意上代表「程式邏輯錯誤」(本來就不該發生
//    的事)。對比之下,std::runtime_error 是「外部因素導致的錯誤」(例如
//    IO 失敗、網路斷線)。這個分類有助於 catch 策略 — logic_error 通常
//    應該讓程式 fail-fast,而非試圖回復。
//
// 4) 例外安全 (Exception Safety) 等級
//    at() 提供 strong exception guarantee:若 throw,string 維持不變。
//    這是 STL 大多數操作的最低品質標準。理解 strong/basic/no-throw
//    三等級對寫穩固代碼非常重要。
//
// 5) 為何不直接用 assert?
//    assert 在 NDEBUG (Release build) 下會被消除。對於「正式環境的安全性」
//    需求,assert 不夠 — 必須用 exception 或更可靠的錯誤回報機制。
//    at() 的檢查在 Release 下也會執行,這正是它的價值。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. 即便是 const string 上呼叫 at,也不能呼叫 at(size())(會丟例外),
//    operator[](size()) 在 const 物件上是合法的(回傳 '\0'),但 at 不允許。
// 2. 修改 at() 回傳的字元會直接改到 string;C++11 起無 COW 問題。
// 3. at() 的例外開銷雖小,但在熱路徑大量呼叫仍會比 operator[] 慢。
// 4. 取得字元後,若再呼叫任何「會使迭代器失效」的操作 (insert/erase/append/
//    reserve 觸發 reallocation 等),原本拿到的 reference 也會失效。
// 5. at 不是 noexcept,catch 時要記得用 std::out_of_range 或 std::exception。
//
// =============================================================================

/*
補充筆記：std::string::at
  - std::string::at 會做邊界檢查，越界時丟 std::out_of_range。
  - operator[] 不做檢查，適合已經證明索引合法的熱路徑。
  - 教學時 at 很適合用來區分「語法可用」和「索引真的安全」。
  - std::string::at 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/
#include <iostream>
#include <string>
#include <stdexcept>

void demoAt() {
    std::string s = "ABCDE";

    // 讀取
    std::cout << "s.at(0) = " << s.at(0) << "\n";   // 'A'
    std::cout << "s.at(4) = " << s.at(4) << "\n";   // 'E'

    // 寫入
    s.at(0) = 'Z';
    std::cout << "after write: " << s << "\n";      // "ZBCDE"

    // 越界 → out_of_range
    try {
        char c = s.at(100);
        (void)c;
    } catch (const std::out_of_range& e) {
        std::cout << "out_of_range: " << e.what() << "\n";
    }
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例】LeetCode 125. Valid Palindrome (Easy)
//
// 題目敘述:
//   給定字串 s,只考慮其中的英數字 (alphanumeric) 並忽略大小寫,判斷它
//   是否為迴文 (palindrome)。
//   範例: "A man, a plan, a canal: Panama" → true
//        "race a car" → false
//
// 為何用 at:
//   雙指標 i, j 從兩端往中間移動,每次都要檢查當前字元是否為英數字。
//   雖然迴圈條件已限制在 [0, size()),理論上 [] 也安全,但這題剛好
//   示範 at() 的 defensive 用法 — 即便邏輯正確,加上邊界檢查讓未來
//   修改代碼時更不易出錯 (例如忘記 ++i 導致無窮迴圈也能被即早發現)。
//
// 解題思路:
//   雙指標夾擊;遇非英數字跳過;比對忽略大小寫。
//
// 複雜度: 時間 O(n),空間 O(1)
// -----------------------------------------------------------------------------
#include <cctype>
bool isPalindrome(const std::string& s) {
    size_t i = 0;
    size_t j = s.size();
    if (j == 0) return true;
    --j;

    while (i < j) {
        // 跳過非英數字
        while (i < j && !std::isalnum(static_cast<unsigned char>(s.at(i)))) ++i;
        while (i < j && !std::isalnum(static_cast<unsigned char>(s.at(j)))) --j;

        if (std::tolower(static_cast<unsigned char>(s.at(i))) !=
            std::tolower(static_cast<unsigned char>(s.at(j))))
            return false;
        ++i;
        if (j == 0) break;
        --j;
    }
    return true;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】CSV 欄位的安全索引存取
//
// 為何用 at:
//   解析 CSV 後常用 vector<string> 或單列字串存欄位。若行格式不符
//   (欄位數不足),用 [] 直接存取就是 UB,可能導致 silently 讀到亂值。
//   改用 at() 會直接 throw,呼叫端可以 try/catch 並 log 出錯誤行,
//   完成優雅的 ETL 容錯處理。
//
// 這是後端 / Data Pipeline / log 解析的日常需求。
// -----------------------------------------------------------------------------
std::string safeColumn(const std::string& csvRow, size_t colIndex) {
    // 簡化版:以逗號切欄位
    size_t start = 0, count = 0;
    for (size_t i = 0; i <= csvRow.size(); ++i) {
        if (i == csvRow.size() || csvRow[i] == ',') {
            if (count == colIndex) {
                return csvRow.substr(start, i - start);
            }
            start = i + 1;
            ++count;
        }
    }
    // 找不到該欄位 → 模擬 at() 行為丟例外
    throw std::out_of_range("CSV column " + std::to_string(colIndex) + " not found");
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 709. To Lower Case (Easy)
// 題目: 把字串內每個英文字母改小寫。
// 為何用 at: 教學「用 at 取得可讀位置 + 用回值處理」的搭配。本題其實也可改用
//            operator[];改寫成 at() 是為了展示「外部資料來源仍想保有 throw 保護」。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
std::string toLowerCase(std::string s) {
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s.at(i);
        if (c >= 'A' && c <= 'Z') s.at(i) = c + ('a' - 'A');
    }
    return s;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】固定格式 ISO 日期 "YYYY-MM-DD" 抽欄位
// 為何用 at: 對「來自外部設定檔」的日期字串作驗證式存取,長度或格式錯時直接 throw,
//            比 sscanf 安默不出聲更穩。教學「外部來源 + 想要 catch 例外」場景。
// -----------------------------------------------------------------------------
struct YMD { int y, m, d; };
YMD parseIsoDate(const std::string& s) {
    // 預期格式 YYYY-MM-DD,長度 10;at() 對任何格式不符直接丟 out_of_range
    if (s.size() != 10) throw std::out_of_range("bad iso date length");
    YMD r{0, 0, 0};
    r.y = (s.at(0) - '0') * 1000 + (s.at(1) - '0') * 100 +
          (s.at(2) - '0') * 10   + (s.at(3) - '0');
    r.m = (s.at(5) - '0') * 10 + (s.at(6) - '0');
    r.d = (s.at(8) - '0') * 10 + (s.at(9) - '0');
    return r;
}

int main() {
    demoAt();
    std::cout << "\n=== LeetCode 125 ===\n";
    std::cout << std::boolalpha
              << isPalindrome("A man, a plan, a canal: Panama") << "\n"   // true
              << isPalindrome("race a car") << "\n";                       // false

    std::cout << "\n=== LeetCode 709 ===\n";
    std::cout << toLowerCase("Hello") << "\n";              // hello
    std::cout << toLowerCase("LOVELY") << "\n";             // lovely

    std::cout << "\n=== 日常實務: CSV 安全欄位 ===\n";
    try {
        std::cout << "col 2 = " << safeColumn("Alice,30,Taipei", 2) << "\n";  // Taipei
        std::cout << "col 5 = " << safeColumn("Alice,30,Taipei", 5) << "\n";  // throws
    } catch (const std::out_of_range& e) {
        std::cout << "caught: " << e.what() << "\n";
    }

    std::cout << "\n=== 日常實務: ISO 日期解析 ===\n";
    auto d = parseIsoDate("2024-05-18");
    std::cout << d.y << "/" << d.m << "/" << d.d << "\n";
    try { parseIsoDate("2024/05"); }
    catch (const std::out_of_range& e) { std::cout << "caught: " << e.what() << "\n"; }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：at() 與 operator[] 的效能差距有多大?
    //    A：差距極小。at() 多一次 size 比較與條件分支,在「沒越界」的正常
    //       路徑上現代 CPU 的分支預測幾乎可零成本跳過。Big-O 都是 O(1)。
    //       只有真的 throw 時才會付出 stack unwinding 的昂貴代價。
    //
    //  Q2：為什麼 operator[](size()) 合法但 at(size()) 會 throw?
    //    A：operator[] 對 const string 在 size() 位置回傳結尾 '\0' 的參考
    //       (C++11 起明文保證);at() 的語意是「存取第 pos 個有效字元」,
    //       而 size() 是 past-the-end 位置,不算有效字元,因此一律 throw
    //       std::out_of_range,不論 const 與否。
    //
    //  Q3：什麼時候應該選 at() 而不是 operator[]?
    //    A：當索引「來自外部、不可信」(使用者輸入、JSON、CSV 欄位、網路封包
    //       offset) 就用 at(),把越界自動轉成 catchable exception。當索引
    //       是程式內部已驗證、在熱迴圈中需要榨乾效能,則用 operator[]。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra at.cpp -o at

// === 預期輸出 (節錄) ===
// === LeetCode 125 ===
// true
// false
// === LeetCode 709 ===
// hello
// lovely
// === 日常實務: ISO 日期解析 ===
// 2024/5/18
// caught: bad iso date length
