// =============================================================================
//  第 2.5 章：std::move — 強制轉換為右值 (1)
//  主題: std::move(x) 與 static_cast<T&&>(x) 完全等價 — 證明 move 不移動任何東西
// =============================================================================
//
// 【主題資訊 Information】
//   template<class T>
//   constexpr std::remove_reference_t<T>&& move(T&& t) noexcept;
//
//   標頭檔  : <utility>
//   標準版本: C++11 引入;C++14 起加上 constexpr(N3471 constexpr library additions)
//             C++20 起標準要求 [[nodiscard]](P0600R1)
//   複雜度  : O(1) — 而且是「編譯期 0 條指令」,見下方【概念補充】
//   回傳    : 一個指向同一個物件的 rvalue reference(不複製、不搬移、不配置記憶體)
//
//   本機實測(GCC 15.2 / libstdc++,/usr/include/c++/15/bits/move.h:135-139):
//     template<typename _Tp>
//       [[__nodiscard__,__gnu__::__always_inline__]]
//       constexpr typename std::remove_reference<_Tp>::type&&
//       move(_Tp&& __t) noexcept
//       { return static_cast<typename std::remove_reference<_Tp>::type&&>(__t); }
//   注意它是「無條件」constexpr + nodiscard(不是用版本巨集包起來),所以在
//   -std=c++11 -pedantic-errors 下也看得到這兩個行為(實測通過)。這是 libstdc++
//   的寬鬆實作(用保留拼法 __nodiscard__ 所以不違反 -pedantic-errors),
//   可攜程式碼不可以依賴它 — 版本規則仍以標準為準。
//
// 【詳細解釋 Explanation】
//
// 【1. std::move 是「型別轉換」,不是「動作」】
//   函式名稱是 C++ 標準庫最著名的取名失誤。std::move 的函式體只有一行 static_cast,
//   它做的事情是:把一個左值(lvalue)「重新貼標籤」成右值(rvalue),讓它在
//   overload resolution(多載決議)時能匹配到 T&& 參數。
//
//   真正把資料搬走的,是「被選中的那個 move constructor / move assignment」。
//   如果目標型別根本沒有 move 版本(或因 const 而選不到),那就什麼也沒搬,
//   靜靜地退回 copy。std::move 對此毫不知情,也不會報錯。
//
//   一句話總結:std::move 是「請求」而非「保證」。它只負責換標籤,搬不搬由對方決定。
//
// 【2. 既然等價,為什麼不直接寫 static_cast?】
//   本檔的兩段程式碼行為完全相同:
//       std::string a = std::move(s);
//       std::string b = static_cast<std::string&&>(s);
//   std::move 勝在三點:
//     (a) 意圖明確 — 讀者一眼看出「我要把這個物件的資源交出去」,
//         而 static_cast<std::string&&> 混在一堆 cast 中不容易辨識。
//     (b) 不必手寫型別 — static_cast 版本要把型別完整拼出來;型別一改(例如
//         std::string 換成 std::wstring)就得跟著改,漏改則靜默變成別的行為。
//         std::move 由 template 自動推導,永遠正確。
//     (c) 對 template 程式碼是唯一解 — 在 template<class T> 內你不知道 T 是
//         什麼,也不知道它是不是引用;std::move 內部的 remove_reference_t
//         會替你處理乾淨(見檔案 2 的 my_move 實作)。
//
// 【概念補充 Concept Deep Dive】
//
// (A) std::move 產生「零條指令」— 可以親眼驗證
//   std::move 只改變運算式的「值類別(value category)」與型別,這是純編譯期概念;
//   在機器碼層面 T& 與 T&& 都只是一個位址。因此最佳化後 std::move 完全消失。
//   本機實測(g++ -std=c++17 -O2 -S):
//       std::string&& f1(std::string& s) { return std::move(s); }
//       std::string&& f2(std::string& s) { return static_cast<std::string&&>(s); }
//   產生的組語逐字元相同,兩者都是:
//       endbr64            ← CET landing pad,與 move 無關
//       movq  %rdi, %rax   ← 把傳入的位址原封不動放到回傳暫存器
//       ret
//   也就是說「移動的成本」完全不在 std::move 身上,而在後續被選中的
//   move constructor 上。
//
// (B) 為什麼必須先 remove_reference 再加 &&
//   因為 T 在 forwarding reference(T&&)推導下可能已經是引用型別:
//       傳入左值 std::string  → T 推導成 std::string&
//       傳入右值 std::string  → T 推導成 std::string
//   若直接寫 static_cast<T&&>,對前者會觸發 reference collapsing(引用摺疊):
//       std::string& &&  →  std::string&      ← 又摺回左值引用!move 就失效了
//   所以必須先用 remove_reference_t<T> 把引用剝掉得到純 std::string,再補上 &&,
//   才能保證「不管傳進來什麼,出去的一定是 rvalue reference」。
//   (remove_reference_t 是 C++14 的別名;C++11 要寫 typename remove_reference<T>::type)
//
// (C) 值類別(value category)的角度
//   std::move(s) 的結果是一個 xvalue(eXpiring value,將亡值) — 它是 glvalue
//   (有身分、有位址),同時也是 rvalue(可被搬移)。這正是「同一個物件,但我
//   宣告我不再需要它的值」的精確表達。
//
// 【注意事項 Pay Attention】
//   1. std::move 不會清空來源。來源變成什麼,取決於接收端的 move constructor。
//   2. 被移動後的物件處於「valid but unspecified state(有效但未指定的狀態)」:
//      可以安全解構、可以重新賦值、可以呼叫任何「無前置條件」的成員函式
//      (如 size()、empty()、clear());但「不可以假設它的值是什麼」。
//      本機實測 libstdc++ 對 std::string 一律清空(連 SSO 短字串也清),
//      但這不是標準保證,換一套標準庫或換版本都可能不同。
//   3. 對 const 物件用 std::move 會靜默退化成複製(見檔案 3),編譯器不報錯也不警告。
//   4. std::move 自 C++20 起標準規定為 [[nodiscard]];本機 libstdc++ 在所有
//      -std 模式都已加上,所以 `std::move(s);` 這種丟棄回傳值的寫法會觸發
//      -Wunused-result 警告(實測 c++11/14/17/20/23 全部會警告)。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::move 的本質
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::move 真的有「移動」任何東西嗎?
//     答：沒有。它是一個編譯期的 static_cast<remove_reference_t<T>&&>,
//         函式體只有一行 cast,-O2 下產生 0 條指令(本機實測組語只剩
//         movq %rdi,%rax; ret,與手寫 static_cast 完全相同)。
//         真正搬移資料的是「被 overload resolution 選中的 move constructor
//         或 move assignment operator」。std::move 只負責把左值標記成右值,
//         好讓那個多載有機會被選中。
//     追問：那如果目標型別沒有 move constructor 會怎樣?
//         → 靜默退回 copy constructor,不會編譯錯誤,也不會有任何警告。
//
// 🔥 Q2. 一個物件被 std::move 之後,處於什麼狀態?還能用嗎?
//     答：處於「valid but unspecified state(有效但未指定的狀態)」。
//         「valid」表示它仍是一個合法物件 — 解構安全、可以重新賦值、可以呼叫
//         任何沒有前置條件的成員函式(size()/empty()/clear())。
//         「unspecified」表示它的「值」是什麼由實作決定,標準不做保證。
//     追問：那 s.front() 或 s[0] 可以呼叫嗎?
//         → 不可以。這兩個有「字串非空」的前置條件,而你無法保證它非空 → UB。
//
// ⚠️ 陷阱. `std::string s = "Hi"; std::string t = std::move(s);` 之後
//          s 保證是空字串 ""。這句話對嗎?
//     答：不對。標準只保證 valid but unspecified,並沒有保證清空。
//         本機實測 libstdc++ 確實一律清空(長度 2/13/15/16/40 移動後 size() 都是 0),
//         但那是實作行為,不是標準承諾。
//     為什麼會錯：多數人把「在我電腦上跑出空字串」當成語言規則。
//         正確心態是:移動後的來源只能「重新賦值」或「讓它解構」,
//         絕不能「讀它的值再依賴那個值」。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <utility>
#include <vector>

// -----------------------------------------------------------------------------
// 【日常實務範例】把讀進來的檔案內容搬進 LogRecord,而不是複製一份
//   情境: 從磁碟讀了一大段 log 內容(實務上可能好幾 MB),要包成一筆 record
//         放進容器。若用複製的方式建構,整段內容會被再配置 + memcpy 一次;
//         用 std::move 把 buffer 的所有權交出去,只搬「指標 + 長度 + 容量」,
//         堆積上的位元組完全不動。
//   慣用法: 參數用 by-value,函式內再 std::move 進成員 —
//           呼叫端傳左值就複製一次、傳右值就搬移,兩種需求一份程式碼。
// -----------------------------------------------------------------------------
struct LogRecord {
    std::string source;   // 來源檔名
    std::string content;  // 檔案內容(可能很大)
};

LogRecord makeRecord(std::string source, std::string content) {
    // source / content 是本函式的區域參數,是「具名的左值」,
    // 所以要用 std::move 才能把它們的緩衝區交給成員,避免再複製一次。
    return LogRecord{std::move(source), std::move(content)};
}

int main() {
    std::cout << "=== 1. std::move 與 static_cast 完全等價 ===\n";

    std::string s = "Hello, World!";

    // 方式 1：std::move
    std::string a = std::move(s);
    std::cout << "s after std::move: \"" << s << "\"\n";

    s = "Hello, World!";  // 重新賦值(被移動後「重新賦值」永遠是合法的)

    // 方式 2：static_cast
    std::string b = static_cast<std::string&&>(s);
    std::cout << "s after static_cast: \"" << s << "\"\n";

    // 兩者行為完全相同
    std::cout << "a = \"" << a << "\"\n";
    std::cout << "b = \"" << b << "\"\n";

    std::cout << "\n=== 2. 移動後的來源:valid but unspecified ===\n";
    // 「valid」的意思:下面這些操作全部合法
    std::string src = "some payload";
    std::string dst = std::move(src);
    std::cout << "dst        = \"" << dst << "\"\n";
    std::cout << "src.size() = " << src.size() << "  (呼叫 size() 合法)\n";
    std::cout << "src.empty()= " << std::boolalpha << src.empty() << "  (呼叫 empty() 合法)\n";
    src = "reassigned";  // 重新賦值 → 完全復活
    std::cout << "重新賦值後 src = \"" << src << "\"\n";
    std::cout << "(本機 libstdc++ 清空了來源,但標準只保證 valid but unspecified)\n";

    std::cout << "\n=== 3. 日常實務:把 buffer 搬進 LogRecord ===\n";
    std::vector<LogRecord> records;

    std::string path    = "/var/log/app.log";
    std::string content = "2026-07-20 10:00:00 [ERROR] disk quota exceeded";

    std::cout << "搬移前 content 長度 = " << content.size() << "\n";
    records.push_back(makeRecord(std::move(path), std::move(content)));
    std::cout << "搬移後 content 長度 = " << content.size()
              << "  (所有權已轉移,本機實測歸零)\n";

    std::cout << "records[0].source  = \"" << records[0].source  << "\"\n";
    std::cout << "records[0].content = \"" << records[0].content << "\"\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.5 章：stdmove — 強制轉換為右值1.cpp" -o move1

// === 預期輸出 ===
// (執行結果為固定值,不含位址或計時,每次執行皆相同)
//
// === 1. std::move 與 static_cast 完全等價 ===
// s after std::move: ""
// s after static_cast: ""
// a = "Hello, World!"
// b = "Hello, World!"
//
// === 2. 移動後的來源:valid but unspecified ===
// dst        = "some payload"
// src.size() = 0  (呼叫 size() 合法)
// src.empty()= true  (呼叫 empty() 合法)
// 重新賦值後 src = "reassigned"
// (本機 libstdc++ 清空了來源,但標準只保證 valid but unspecified)
//
// === 3. 日常實務:把 buffer 搬進 LogRecord ===
// 搬移前 content 長度 = 47
// 搬移後 content 長度 = 0  (所有權已轉移,本機實測歸零)
// records[0].source  = "/var/log/app.log"
// records[0].content = "2026-07-20 10:00:00 [ERROR] disk quota exceeded"
