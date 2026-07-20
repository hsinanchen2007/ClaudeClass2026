// ============================================================
// 第 2.1 章 總結：左值與右值的定義 — Value Categories 完整解析
// ============================================================
//
// 【主題資訊 Information】
//   主題    ：expression 的 value category（lvalue / prvalue / xvalue）
//   標準版本：三分法 gl/pr/x 的正式定義      C++11
//             prvalue 語意調整（保證的複製省略）C++17
//             decltype((expr)) 查詢值類別     C++11
//   標頭檔  ：<utility>（std::move）、<type_traits>
//   複雜度  ：純編譯期分類，執行期零成本
//   本檔宣告的標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼要有 value category：型別不足以決定「能不能搬」】
//   同樣是 std::string，有的可以安全掏空、有的不行：
//       std::string s = "hi";
//       f(s);                  // s 之後還要用 → 不能掏空
//       f(std::string("hi"));  // 暫時物件，沒有下一個使用者 → 可以掏空
//   兩者「型別」完全一樣，差別在於「表達式的性質」。
//   value category 就是標準用來描述這個性質的分類系統，
//   而重載決議正是靠它來選擇複製版本或移動版本。
//
// 【2. 兩個判準決定五種類別】
//   標準用兩個獨立的問題來分類：
//       (a) 有沒有「身分（identity）」—— 能不能取位址、能否辨認是同一個物件？
//       (b) 能不能被「移動（movable）」—— 掏空它會不會有人受害？
//   組合出三個基本類別：
//       lvalue ：有身分、不可移動   （具名變數、*ptr、arr[i]、++x）
//       prvalue：無身分、可移動     （42、x+y、func()、x++）
//       xvalue ：有身分、可移動     （std::move(x)、回傳 T&& 的函式呼叫）
//   再由它們組成兩個聯集：
//       glvalue = lvalue ∪ xvalue  （generalized lvalue，「有身分」的統稱）
//       rvalue  = prvalue ∪ xvalue （「可移動」的統稱）
//   xvalue 同時屬於 glvalue 與 rvalue —— 這正是它存在的意義：
//   一個「有位址、但已被宣告可以掏空」的物件。
//
// 【3. 幾個違反直覺、但一定要記住的案例】
//   * 字串字面值 "hello" 是 lvalue（型別為 const char[6]）。
//     它存放在靜態儲存區、有位址、可以取址，所以是 lvalue。
//     其他字面值（42、3.14、true）則是 prvalue。
//   * ++x 是 lvalue（前置遞增回傳 int&），x++ 是 prvalue（回傳舊值的副本）。
//     這也解釋了為什麼前置版本通常較有效率 —— 不必製造副本。
//   * 有名字的右值參考是 lvalue：
//         void f(std::string&& s) { /* s 這個表達式是 lvalue！ */ }
//     型別是 string&&，但表達式是 lvalue —— 型別與 value category 是兩件事。
//
// 【4. 本檔的判定工具怎麼運作】
//   value_category<decltype((expr))> 利用了 decltype 的規則 2：
//       decltype((expr)) 對 lvalue 得 T&、對 xvalue 得 T&&、對 prvalue 得 T
//   再用模板偏特化把這三種型別對應回三個名稱。
//   注意那對「多餘的括號」是必要的 —— 沒有它，具名變數會走 decltype 規則 1
//   （回傳宣告型別 T），就分不出 lvalue 了。
//
// 【概念補充 Concept Deep Dive】
//
// (A) C++17 對 prvalue 的重新定義（很重要的觀念轉變）
//     C++11 的 prvalue 是「一個暫時物件」；
//     C++17 改為「一個尚未具體化（materialize）的初始化式」——
//     它只有在真正需要一個物件時才會被具體化。
//     這個改動讓「保證的複製省略」成為可能：
//         std::string s = std::string("hi");   // C++17 保證零次複製/移動
//     C++11 只是「允許」編譯器省略，C++17 則是「規定」根本不存在中間物件。
//     實務影響：回傳暫時物件的工廠函式，即使型別不可複製也不可移動，仍然合法。
//
// (B) 為什麼 const T&& 幾乎沒有用
//     T&& 存在的意義是「我可以掏空你」，但 const 讓你無法修改來源，
//     也就無法把來源設成空 —— 兩者互相抵消。
//     實務後果：對 const 物件呼叫 std::move 會安靜退回複製。
//         const std::string cs = "hi";
//         std::string s2 = std::move(cs);   // 實際上是複製，不是移動
//     這是「加了 move 卻沒有變快」的常見原因之一。
//
// (C) 取址測試是最實用的判斷法
//     &expr 合法 ⇔ expr 是 lvalue。
//     這是實務上最快的判斷方式：
//         &x     ✅ lvalue           &42           ❌ prvalue
//         &++x   ✅ lvalue           &x++          ❌ prvalue
//         &"hi"  ✅ lvalue（字面值）  &std::move(x) ❌ xvalue（rvalue 不可取址）
//
// 【注意事項 Pay Attention】
//   1. 型別與 value category 是兩件不同的事：string&& 型別的具名變數，
//      其表達式是 lvalue。
//   2. 字串字面值是 lvalue，其他字面值是 prvalue —— 這是唯一的字面值例外。
//   3. ++x 是 lvalue，x++ 是 prvalue。
//   4. std::move 不移動任何東西，它只是把表達式轉成 xvalue（型別轉換而已）。
//   5. 被移動後的物件處於「有效但未指定」狀態；不可假設它一定變空 ——
//      短字串因 SSO 可能根本沒有被掏空。
//   6. 對 const 物件 std::move 會退回複製，因為 const 物件無法被掏空。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Value Categories
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 請說明 lvalue、prvalue、xvalue 的區別。
//     答：用兩個判準劃分 ——「有沒有身分」與「能不能被移動」。
//         lvalue：有身分、不可移動（具名變數、*ptr、arr[i]）。
//         prvalue：無身分、可移動（42、a+b、回傳 T 的函式呼叫）。
//         xvalue：有身分、可移動（std::move(x)、回傳 T&& 的函式呼叫）。
//         glvalue = lvalue ∪ xvalue；rvalue = prvalue ∪ xvalue。
//     追問：為什麼需要 xvalue 這個「中間類別」？→ 因為移動語意需要一個
//         「有位址（所以能修改它、把它設成空）但已被宣告可放棄」的類別。
//
// 🔥 Q2. void f(std::string&& s) 裡面，s 是左值還是右值？
//     答：s 的「型別」是 std::string&&，但 s 這個「表達式」是 lvalue。
//         因為它有名字、可以取址、在函式內可能被使用多次。
//         要把它當右值傳下去，必須再寫一次 std::move(s)。
//     追問：為什麼標準要這樣設計？→ 安全考量。若具名的 && 自動當右值，
//         第一次使用就會被掏空，後續使用會讀到空物件。
//
// 🔥 Q3. "hello" 是左值還是右值？
//     答：左值。字串字面值的型別是 const char[N]，存放在靜態儲存區，
//         有位址、可以取址，所以是 lvalue。
//         其他字面值（42、3.14、true、nullptr）都是 prvalue。
//     追問：那 &"hello" 合法嗎？→ 合法，型別是 const char(*)[6]。
//         這正好印證「可取址 = lvalue」這個判斷法。
//
// ⚠️ 陷阱. std::move(x) 之後，x 一定會變成空的嗎？
//     答：不一定。std::move 本身什麼都沒做，只是型別轉換。
//         真正掏空 x 的是「接手的那個移動建構子/賦值運算子」。
//         若對方沒有移動操作（或 x 是 const），就會退回複製，x 完全不變。
//         即使真的移動了，標準也只保證「有效但未指定」，不保證變空 ——
//         短字串因 SSO 常常是整段複製，內容原封不動。
//     為什麼會錯：把 std::move 當成一個「會清空來源」的動作函式。
//         正確認知：它只是一個轉型，決定權在接手的那一方。
// ═══════════════════════════════════════════════════════════════════════════
//
// ============================================================
// 【C++ 值類別三分法】
//
//        expression
//        ├── glvalue（有身分）
//        │   ├── lvalue
//        │   └── xvalue  ←─┐（xvalue 同時屬於兩邊）
//        └── rvalue（可移動）
//            ├── xvalue  ←─┘
//            └── prvalue
//
// 註：此處改用框線字元繪製。若用 ASCII 的反斜線收尾（例如 "/    \"），
//     行末的 '\' 會被視為續行符號而把下一行併入註解，並觸發 -Wcomment 警告。
//
// 【lvalue 左值】有身份（identity），不可移動
//   - 有名字的變數：int x; → x 是 lvalue
//   - 引用：int& ref = x; → ref 是 lvalue
//   - 陣列元素：arr[0]
//   - 解引用：*ptr
//   - 字串字面值："Hello" → 是 lvalue！（存在靜態儲存區）
//   - 前置遞增：++x（回傳引用 → lvalue）
//   - 回傳引用的函式呼叫：get_ref(x)
//   → 可以取位址（&x 合法）
//
// 【prvalue 純右值】沒有身份，不可移動（將要移動）
//   - 非字串的字面值：42, 3.14, true, nullptr
//   - 算術運算結果：x + y, x > 0
//   - 回傳非引用型別的函式呼叫：func()
//   - 型別轉換：int(42), static_cast<double>(x)
//   - Lambda 表達式：[](int a){ return a; }
//   - 後置遞增：x++（回傳舊值的副本 → prvalue）
//   → 不能取位址
//
// 【xvalue 將亡值】有身份，且可被移動
//   - std::move(x) → 將 lvalue 轉為 xvalue
//   → 告訴編譯器「這個物件可以被移動」
//
// 【判斷規則】
//   有名字 → lvalue    沒名字 → rvalue
//   可取址 → lvalue    不可取址 → rvalue
//   std::move(x) → xvalue（有身份 + 可移動）
// ============================================================

#include <iostream>
#include <string>
#include <utility>
#include <type_traits>
#include <vector>

// ============================================================
// 值類別判定工具
// ============================================================
template<typename T> struct value_category         { static constexpr const char* value = "prvalue"; };
template<typename T> struct value_category<T&>     { static constexpr const char* value = "lvalue"; };
template<typename T> struct value_category<T&&>    { static constexpr const char* value = "xvalue"; };

#define PRINT_CATEGORY(expr) \
    std::cout << "  " << #expr << " → " \
              << value_category<decltype((expr))>::value << "\n"

// 測試用函數
std::string make_string()                  { return "temp"; }
std::string& get_ref(std::string& s)       { return s; }

// ============================================================
// 【LeetCode 實戰範例】—— 本章刻意從缺，理由如下
// ============================================================
// value category 是「表達式的分類規則」，屬於語言規格層面，
// 不是資料結構或演算法。LeetCode 沒有任何一題在考它，
// 解題過程也不會因為知道 xvalue 而改變作法。
// 硬把某題掛上來只會製造「為了湊而湊」的假關聯，故本章從缺。
// 改以一個真實會被 value category 影響結果的工程情境示範。
// ============================================================

// ------------------------------------------------------------
// 【日常實務範例】以重載「看見」value category：日誌系統的字串接收
//   情境：日誌函式要把訊息存進緩衝區。呼叫端可能傳
//         (a) 既有變數（之後還要用）→ 必須複製
//         (b) 當場組出來的暫時字串   → 可以直接接收，不必複製
//   為什麼用到本主題：
//     兩個重載 const string& 與 string&& 的選擇，完全由「引數的
//     value category」決定 —— 這是 value category 在真實程式碼裡
//     唯一會被「看見」的地方，也是移動語意得以運作的基礎。
// ------------------------------------------------------------
class LogBuffer {
    std::vector<std::string> m_lines;
    int m_copies = 0;
    int m_moves  = 0;

public:
    // 左值版本：來源之後還要用 → 只能複製
    void append(const std::string& line) {
        m_lines.push_back(line);              // 複製
        ++m_copies;
    }

    // 右值版本：來源即將消亡 → 直接接手，不複製
    void append(std::string&& line) {
        m_lines.push_back(std::move(line));   // line 有名字是 lvalue，須再 move 一次
        ++m_moves;
    }

    void report() const {
        std::cout << "    緩衝區共 " << m_lines.size() << " 行"
                  << "（複製 " << m_copies << " 次、移動 " << m_moves << " 次）\n";
        for (std::size_t i = 0; i < m_lines.size(); ++i)
            std::cout << "      " << m_lines[i] << "\n";
    }
};

int main() {
    // ============================================================
    // 1. Lvalue 範例
    // ============================================================
    std::cout << "===== 1. Lvalue（有名字、可取址）=====\n";
    int x = 42;
    int& ref = x;
    int arr[3] = {1, 2, 3};

    PRINT_CATEGORY(x);        // lvalue
    PRINT_CATEGORY(ref);      // lvalue
    PRINT_CATEGORY(arr[0]);   // lvalue

    // 驗證：lvalue 可以取位址
    std::cout << "  &x     = " << &x << "\n";
    std::cout << "  &ref   = " << &ref << " （和 &x 相同）\n";
    std::cout << "  &arr[0]= " << &arr[0] << "\n\n";

    // ============================================================
    // 2. Prvalue 範例
    // ============================================================
    std::cout << "===== 2. Prvalue（臨時值、不可取址）=====\n";
    PRINT_CATEGORY(42);               // prvalue — 整數字面值
    PRINT_CATEGORY(x + 1);            // prvalue — 算術運算結果
    PRINT_CATEGORY(x > 0);            // prvalue — 比較結果
    PRINT_CATEGORY(make_string());     // prvalue — 回傳非引用
    PRINT_CATEGORY(static_cast<double>(x)); // prvalue — 型別轉換
    // &42;  // ❌ 編譯錯誤！prvalue 不可取址
    std::cout << "\n";

    // 特別注意：前置 vs 後置遞增
    std::cout << "===== 前置 vs 後置遞增 =====\n";
    int y = 10;
    PRINT_CATEGORY(++y);   // lvalue（回傳引用）
    PRINT_CATEGORY(y++);   // prvalue（回傳舊值副本）
    std::cout << "\n";

    // ============================================================
    // 3. Xvalue 範例
    // ============================================================
    std::cout << "===== 3. Xvalue（std::move 產生）=====\n";
    std::string s = "Hello";
    PRINT_CATEGORY(std::move(x));    // xvalue
    PRINT_CATEGORY(std::move(s));    // xvalue
    std::cout << "\n";

    // ============================================================
    // 4. 函式回傳值的值類別
    // ============================================================
    std::cout << "===== 4. 函式回傳值的值類別 =====\n";
    std::string str = "Hello";
    PRINT_CATEGORY(make_string());    // prvalue（回傳值）
    PRINT_CATEGORY(get_ref(str));     // lvalue（回傳引用）
    std::cout << "\n";

    // ============================================================
    // 5. 值類別對移動的影響
    // ============================================================
    std::cout << "===== 5. 值類別與移動 =====\n";
    {
        std::string original = "This is a long string on the heap";

        // 傳入 lvalue → 觸發複製
        std::cout << "  傳入 lvalue（複製）：\n";
        std::string copy = original;
        std::cout << "    original 仍然有效：\"" << original << "\"\n";

        // 傳入 xvalue（std::move）→ 觸發移動
        std::cout << "  傳入 xvalue（移動）：\n";
        std::string moved = std::move(original);
        std::cout << "    original 移動後的內容：\"" << original << "\"\n";
        std::cout << "    moved = \"" << moved << "\"\n";
        std::cout << "    ⚠️ 標準只保證 original 處於「有效但未指定」狀態。\n";
        std::cout << "       此處觀察到它變成空字串，是本實作的行為，非標準保證，\n";
        std::cout << "       不可寫出依賴「移動後必為空」的程式邏輯。\n";
    }

    // ============================================================
    // 6. 移動後狀態：只保證「有效」，內容不保證
    // ============================================================
    std::cout << "\n===== 6. 移動後狀態不可假設 =====\n";
    {
        std::string shortStr = "abc";                                  // 短，可能走 SSO
        std::string longStr  = "0123456789012345678901234567890123";   // 長，走堆積

        std::string s1 = std::move(shortStr);
        std::string s2 = std::move(longStr);

        std::cout << "  短字串移動後長度: " << shortStr.size() << "（本次觀察值）\n";
        std::cout << "  長字串移動後長度: " << longStr.size()  << "（本次觀察值）\n";
        std::cout << "  本實作兩者都變成空，但這不是標準保證 ——\n";
        std::cout << "  短字串因 SSO 存在物件內部、沒有堆積指標可搬，\n";
        std::cout << "  其他實作完全可以選擇「直接複製字元、來源不變」。\n";
        std::cout << "  唯一能依賴的是：移動後的物件仍可安全銷毀或重新賦值。\n";
        std::cout << "  搬移結果: s1=\"" << s1 << "\", s2 長度=" << s2.size() << "\n";

        // 示範「唯一安全的後續操作」：重新賦值
        shortStr = "重新賦值後又可正常使用";
        std::cout << "  重新賦值後 shortStr = \"" << shortStr << "\"\n";
    }

    // ============================================================
    // 重點整理
    // ============================================================
    std::cout << "\n=== 值類別速查 ===\n";
    std::cout << "  lvalue：有名字、可取址（變數、引用、*ptr、arr[i]）\n";
    std::cout << "  prvalue：臨時值（42、x+y、func()、cast）\n";
    std::cout << "  xvalue：std::move(x)（有身份但可被移動）\n";
    std::cout << "  字串字面值 \"hello\" 是 lvalue（特殊！）\n";
    std::cout << "  ++x 是 lvalue，x++ 是 prvalue\n";

    // ============================================================
    // 日常實務：以重載「看見」value category
    // ============================================================
    std::cout << "\n===== 日常實務：日誌緩衝區（重載選擇由值類別決定）=====\n";
    {
        LogBuffer log;

        std::string prefix = "[INFO] 服務啟動";
        log.append(prefix);                                   // lvalue  → const& → 複製
        std::cout << "  傳入具名變數後，prefix 仍可用: \"" << prefix << "\"\n";

        log.append(std::string("[WARN] 快取未命中率偏高"));    // prvalue → &&    → 移動
        log.append(prefix + "（第二次）");                     // prvalue → &&    → 移動
        log.append(std::move(prefix));                        // xvalue  → &&    → 移動

        log.report();
        std::cout << "  重點：4 次呼叫選到不同重載，完全由引數的 value category 決定。\n";
        std::cout << "        prefix 以左值傳入時被複製（之後仍可用），\n";
        std::cout << "        最後用 std::move 才交出所有權。\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "summary.cpp" -o vc_summary
//  ⚠️ 注意：下列輸出中的記憶體位址「每次執行都不同」（受堆積/堆疊配置與 ASLR 影響），
//     此處僅為某一次實際執行的樣本，不具重現性；其餘文字內容則是穩定可重現的。

// === 預期輸出 ===
// ===== 1. Lvalue（有名字、可取址）=====
//   x → lvalue
//   ref → lvalue
//   arr[0] → lvalue
//   &x     = 0x7fff8cf127e8
//   &ref   = 0x7fff8cf127e8 （和 &x 相同）
//   &arr[0]= 0x7fff8cf12854
//
// ===== 2. Prvalue（臨時值、不可取址）=====
//   42 → prvalue
//   x + 1 → prvalue
//   x > 0 → prvalue
//   make_string() → prvalue
//   static_cast<double>(x) → prvalue
//
// ===== 前置 vs 後置遞增 =====
//   ++y → lvalue
//   y++ → prvalue
//
// ===== 3. Xvalue（std::move 產生）=====
//   std::move(x) → xvalue
//   std::move(s) → xvalue
//
// ===== 4. 函式回傳值的值類別 =====
//   make_string() → prvalue
//   get_ref(str) → lvalue
//
// ===== 5. 值類別與移動 =====
//   傳入 lvalue（複製）：
//     original 仍然有效："This is a long string on the heap"
//   傳入 xvalue（移動）：
//     original 移動後的內容：""
//     moved = "This is a long string on the heap"
//     ⚠️ 標準只保證 original 處於「有效但未指定」狀態。
//        此處觀察到它變成空字串，是本實作的行為，非標準保證，
//        不可寫出依賴「移動後必為空」的程式邏輯。
//
// ===== 6. 移動後狀態不可假設 =====
//   短字串移動後長度: 0（本次觀察值）
//   長字串移動後長度: 0（本次觀察值）
//   本實作兩者都變成空，但這不是標準保證 ——
//   短字串因 SSO 存在物件內部、沒有堆積指標可搬，
//   其他實作完全可以選擇「直接複製字元、來源不變」。
//   唯一能依賴的是：移動後的物件仍可安全銷毀或重新賦值。
//   搬移結果: s1="abc", s2 長度=34
//   重新賦值後 shortStr = "重新賦值後又可正常使用"
//
// === 值類別速查 ===
//   lvalue：有名字、可取址（變數、引用、*ptr、arr[i]）
//   prvalue：臨時值（42、x+y、func()、cast）
//   xvalue：std::move(x)（有身份但可被移動）
//   字串字面值 "hello" 是 lvalue（特殊！）
//   ++x 是 lvalue，x++ 是 prvalue
//
// ===== 日常實務：日誌緩衝區（重載選擇由值類別決定）=====
//   傳入具名變數後，prefix 仍可用: "[INFO] 服務啟動"
//     緩衝區共 4 行（複製 1 次、移動 3 次）
//       [INFO] 服務啟動
//       [WARN] 快取未命中率偏高
//       [INFO] 服務啟動（第二次）
//       [INFO] 服務啟動
//   重點：4 次呼叫選到不同重載，完全由引數的 value category 決定。
//         prefix 以左值傳入時被複製（之後仍可用），
//         最後用 std::move 才交出所有權。
