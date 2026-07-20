// =============================================================================
//  第 5 課：輸入輸出流（iostream）入門9.cpp  —  std::getline 讀取「一整行」
// =============================================================================
//
// 【主題資訊 Information】
//   template<class CharT, class Traits, class Alloc>
//   std::basic_istream<CharT,Traits>&
//   getline(std::basic_istream<CharT,Traits>& input,
//           std::basic_string<CharT,Traits,Alloc>& str,
//           CharT delim = '\n');                       // <string>
//
//   語意：從 input 讀字元附加進 str（先清空 str），
//         直到讀到 delim、到達檔尾、或 str 達到 max_size()。
//         delim【會被讀走並丟棄】，不會出現在 str 裡。
//   標準版本：C++98 起（第三參數版本亦然）
//   複雜度  ：O(該行字元數)
//   標頭檔  ：<string>（注意：是 <string> 而不是 <iostream>）
//
// 【詳細解釋 Explanation】
//
// 【1. getline 是「非格式化輸入」】
//   這是理解它與 >> 一切差異的鑰匙。
//   格式化輸入（>>）會做型別解析、會跳過前導空白；
//   非格式化輸入（getline、get、read、ignore）則是【逐字元搬運】，
//   不解析、不跳空白、不管內容長什麼樣。
//   所以 getline 讀到的字串裡，空格、Tab 全都原封不動保留下來。
//
// 【2. 分隔符被「吃掉但不放進去」】
//   這個設計很細緻但很重要：
//       輸入 "Hello World\n"
//         → str 得到 "Hello World"（沒有換行符）
//         → 緩衝區裡的 '\n' 已經被消耗掉了
//   若 delim 被留在緩衝區，下一次 getline 就會立刻讀到空行；
//   若 delim 被放進字串，每一行結尾都要自己再 trim 一次。
//   標準選的是「取走但不收錄」，讓連續呼叫 getline 讀多行變得自然。
//
// 【3. 第三個參數可以換成任何分隔符 —— 這才是 getline 的完整威力】
//   getline(is, s, ',') 就變成了一個 CSV 欄位讀取器；
//   getline(is, s, '\0') 可以讀 NUL 分隔的資料（xargs -0 的格式）。
//   配合 std::istringstream，這是解析結構化文字最省事的組合：
//       std::istringstream iss(line);
//       while (std::getline(iss, field, ',')) { ... }
//
// 【4. 空行與檔尾的區別】
//   兩者都會讓 str 變成空字串，但串流狀態不同：
//     • 讀到空行     → str 為空、good() 仍為真（成功讀了一個 0 長度的行）
//     • 一開始就 EOF → str 為空、failbit 與 eofbit 都立起來
//   所以判斷「還有沒有下一行」要用回傳值當條件，
//   不能用「字串是不是空的」：
//       while (std::getline(std::cin, line)) { ... }   // 正確
//       while (!line.empty()) { ... }                  // 錯：空行會提前結束
//
// 【概念補充 Concept Deep Dive】
//
// (A) getline 定義在 <string>，不是 <iostream>
//   std::getline 是自由函式，宣告在 <string> 裡（因為它要操作 std::string）。
//   本檔剛好兩個標頭都有包含所以看不出問題，
//   但只 include <iostream> 就使用 std::getline 是【不保證可攜】的 ——
//   在某些實作上會因為 <iostream> 間接包含 <string> 而僥倖編過。
//   另有一個 istream::getline 成員函式，那是給 char 陣列用的、
//   語意不同（會寫入 '\0'、超長時設 failbit），別搞混。
//
// (B) 為什麼 getline 通常比迴圈呼叫 get() 快得多
//   getline 內部使用 sentry 物件一次鎖定串流，
//   並可直接在 streambuf 的內部緩衝區上做 memchr 找分隔符、
//   再整段複製。逐字元呼叫 get() 則每次都要走一遍 sentry 檢查
//   與虛擬函式呼叫。讀大檔時差距可達數倍。
//
// (C) 行尾的 '\r'（Windows 換行）不會被處理掉
//   以文字模式讀 Windows 產生的 CRLF 檔案時，
//   getline 以 '\n' 為分隔，'\r' 會【留在字串結尾】。
//   這是跨平台處理文字檔最常見的隱形 bug：
//   字串看起來一樣，比較卻永遠不相等。
//   實務上讀完要自己檢查並移除結尾的 '\r'。
//
// 【注意事項 Pay Attention】
//   1. std::getline 宣告在 <string>，使用時請明確包含它。
//   2. 分隔符會被消耗但不會放進字串裡。
//   3. 判斷是否還有資料要用 while (std::getline(...))，
//      不能用「字串是否為空」—— 空行是合法的一行。
//   4. 前面若有用過 >>，要先 ignore 掉殘留的換行符，
//      否則這裡的 getline 會立刻讀到空字串（同課 11 號檔）。
//   5. 讀 CRLF 檔案時字串結尾會多一個 '\r'，需要自行處理。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::getline 與 operator>> 的分工
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::getline 和 std::cin >> 讀字串，差別在哪？
//     答：>> 是格式化輸入，會跳過前導空白、遇到空白就停，一次只拿一個詞；
//         getline 是非格式化輸入，不跳空白、讀到換行才停，
//         能完整保留行內的空格。另外 getline 會把換行符消耗掉但不放進字串，
//         >> 則把停下來的那個空白留在緩衝區。
//     追問：那 getline 讀到的字串裡有換行符嗎？
//           → 沒有。分隔符被讀走後直接丟棄，這樣連續呼叫才不會每次讀到空行。
//
// 🔥 Q2. while (std::getline(std::cin, line)) 為什麼要把回傳值當條件？
//     答：getline 回傳 istream&，而 istream 有 explicit operator bool()
//         （C++11 起；C++98 是 operator void*），會回報串流是否仍可用。
//         用它當條件才能正確區分「讀到一個空行」與「沒有資料了」——
//         兩者都會讓字串是空的，但只有後者會讓串流變成 false。
//     追問：為什麼那個 operator bool 要宣告成 explicit？
//           → 避免 if (std::cin) 以外的荒謬用法通過編譯，
//             例如 int x = std::cin; 或 std::cin + 1。
//             explicit 讓它只在【需要 bool 的語境】（if/while/!）中自動轉換。
//
// ⚠️ 陷阱. 用 while (std::getline(f, line)) 讀檔，發現最後一行「不見了」——
//          是 getline 漏讀嗎？
//     答：不是。幾乎可以確定是那個檔案的最後一行【沒有結尾換行符】，
//         而你誤以為漏讀。實際上 getline 仍會把那些字元讀進 str
//         並設定 eofbit；但因為同時沒有讀到 delim，
//         回傳的串流在【下一次】迴圈判斷時才變成 false ——
//         也就是說最後一行其實有被處理到。
//         真正會漏掉的情況是自己寫成 while (!f.eof()) { getline(...); ... }，
//         那才會多跑一輪或少跑一輪。
//     為什麼會錯：把 eof() 當成「還有沒有資料」的預先檢查。
//         eofbit 是【已經讀到底之後】才被設起來的事後狀態，
//         不是預測。所以任何 while (!stream.eof()) 的迴圈都是有問題的寫法，
//         正確做法永遠是「把讀取動作本身當條件」。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>

int main() {
    std::string fullLine;
    
    std::cout << "請輸入一段文字: ";
    std::getline(std::cin, fullLine);
    
    std::cout << "你輸入的是: [" << fullLine << "]" << std::endl;
    
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 5 課：輸入輸出流（iostream）入門9.cpp" -o io9
// 執行: printf 'Hello World Everyone\n' | ./io9

// 註 1:本檔與同課 8 號檔【只差一行】——8 號用 std::cin >> word，
//      本檔用 std::getline(std::cin, fullLine)。餵完全相同的輸入：
//        8 號檔 → [Hello]
//        本  檔 → [Hello World Everyone]
//      這組對照就是 >>（以詞為單位）與 getline（以行為單位）
//      最乾淨的差異示範。
//
// 註 2:輸出的方括號裡【沒有】換行符 —— getline 會把分隔符
//      讀走並丟棄，不放進字串裡。這也是為什麼 [ ] 能緊貼著內容。
//
// 註 3:若餵入含前導空白的輸入（printf '   Hello\n'），
//      getline 會【完整保留】那三個空格，印出 [   Hello]。
//      >> 則會把它們跳過。已實測確認。

// === 預期輸出 ===
// 請輸入一段文字: 你輸入的是: [Hello World Everyone]
