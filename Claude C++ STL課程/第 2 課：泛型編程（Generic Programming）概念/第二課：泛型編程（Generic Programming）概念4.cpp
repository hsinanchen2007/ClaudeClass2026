// =============================================================================
//  第二課：泛型編程（Generic Programming）概念4.cpp
//   —  模板引數推導的邊界：為什麼 find_max(10, 3.14) 編譯不過
// =============================================================================
//
// 【主題資訊 Information】
//   語法：
//     find_max(a, b)          // 讓編譯器推導 T          （deduced）
//     find_max<double>(a, b)  // 明確指定 T，略過推導     （explicit）
//
//   標準版本：C++98 起（本檔用 -std=c++17 編譯，未依賴新語法）
//   標頭檔  ：<iostream>
//   關鍵詞  ：template argument deduction、deduction conflict、
//             implicit conversion、overload resolution
//
//   相關但不同的機制（別混淆）：
//     * CTAD（class template argument deduction，C++17）—— 那是「類別」模板
//       的推導，例如 std::vector v{1,2,3}；本檔談的是**函式**模板推導。
//       本機以 -pedantic-errors 實測：CTAD 在 -std=c++14 失敗、-std=c++17 通過。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 find_max(10, 3.14) 會失敗】
// 簽名是 `template <typename T> T find_max(T a, T b)` —— **兩個參數共用同一個 T**。
// 編譯器對每個參數各推導一次：
//
//     從第一個實參 10    推導出  T = int
//     從第二個實參 3.14  推導出  T = double
//
// 兩次推導對同一個 T 得到不同答案，這叫 **deduction conflict**（推導衝突）。
// 編譯器不會替你挑一個，而是判定「找不到可用的候選函式」，錯誤訊息通常是：
//     error: no matching function for call to 'find_max(int, double)'
//     note: template argument deduction/substitution failed:
//     note: deduced conflicting types for parameter 'T' ('int' and 'double')
//
// 【2. 最反直覺的一點：推導階段不做隱式轉換】
// 一般函式呼叫時，`void f(double);` 傳 int 進去會自動轉型，這是理所當然的。
// 但模板推導**不是**這樣運作的：
//
//     推導 T 的階段，編譯器在做的是「型別比對」（pattern matching），
//     不是「找一個大家都能轉過去的共同型別」。
//
// 編譯器沒有義務去尋找 int 和 double 的共同型別。它只是把實參型別
// 往參數的樣式上套，套出兩個不同答案就宣告失敗。
// 記住這句話：**先推導，後轉換**。T 一旦定案，該做的隱式轉換才會發生 ——
// 這就是為什麼下面解法一能成立。
//
// 【3. 三種解法，語意各不相同】
//
//   解法一：明確指定型別 find_max<double>(10, 3.14)
//     直接告訴編譯器 T = double，**完全略過推導**。既然 T 已定案，
//     參數型別就是 (double, double)，此時 10 這個 int 實參
//     照一般函式呼叫規則做隱式轉換 int → double。
//     這是「先推導後轉換」規則的直接印證：跳過推導，轉換就恢復正常了。
//
//   解法二：讓兩個實參型別一致 find_max(10.0, 3.14)
//     在呼叫端就把 10 寫成 10.0，兩邊都推導出 double，衝突消失。
//     最單純，但需要呼叫者自己注意。
//
//   解法三（本檔另外示範）：改用兩個型別參數
//         template <typename T, typename U>
//         auto find_max2(T a, U b) -> decltype(a > b ? a : b);
//     讓兩個參數各自獨立推導，回傳型別交給 decltype 從三元運算子推出來
//     （這裡會套用 usual arithmetic conversions，int 與 double 的共同型別是
//      double）。這才是「真正支援混合型別」的泛型寫法。
//     註：C++14 起可直接寫 `auto find_max2(T a, U b)` 讓回傳型別自動推導，
//         本機以 -pedantic-errors 實測：-std=c++11 失敗、-std=c++14 通過。
//         本檔使用 C++11 起可用的尾置回傳型別（trailing return type）寫法，
//         以清楚呈現回傳型別是怎麼決定的。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 明確指定與推導可以混用
//   模板引數可以只指定前面幾個，其餘讓編譯器推導：
//       template <typename R, typename T> R convert(T v);
//       auto x = convert<double>(42);      // R = double 指定，T = int 推導
//   這也是為什麼「需要明確指定的模板參數要放在參數列表前面」——
//   後面的可以推導出來，前面的沒法跳過。std::static_pointer_cast、
//   std::get<N> 都是這個設計的實例。
//
// (B) 推導與 overload resolution 的先後順序
//   完整流程是：
//     1) 找出所有同名的候選者（一般函式 + 函式模板）
//     2) 對每個函式模板嘗試推導；推導失敗的**安靜地移出候選集**
//        （這就是 SFINAE：Substitution Failure Is Not An Error）
//     3) 對剩下的候選者做 overload resolution，比較轉換代價
//     4) 若最佳者不唯一 → ambiguous；若候選集為空 → no matching function
//   關鍵規則：當一個非模板函式與一個模板實例化結果**同樣匹配**時，
//   **非模板函式優先**。這使得「為特定型別手寫最佳化版本」成為可能：
//       template <typename T> void process(T v);   // 泛型版
//       void process(int v);                        // int 專用，會被優先選中
//   本檔用實際輸出印證這條規則。
//
// (C) 為什麼 find_max(10, 3.14) 的錯誤訊息裡沒有「型別不符」字樣
//   因為候選函式根本沒被成功生成出來 —— 失敗發生在「推導/替換」階段，
//   而不是「參數型別檢查」階段。所以你看到的是
//   「no matching function」而非「cannot convert int to double」。
//   看到 template 相關的 no matching function，第一個要懷疑的就是推導衝突。
//
// 【注意事項 Pay Attention】
// 1. 明確指定型別是雙面刃：它讓轉換恢復，但也意味著你放棄了型別檢查的保護。
//    find_max<int>(3.9, 2.1) 會把 double 截斷成 int，安靜地失去精度
//    （本檔實際示範這個後果）。
// 2. 混合型別的泛型函式，回傳型別怎麼定是個真問題。用 decltype 或 auto 讓
//    編譯器推導，比硬選 T 或 U 其中之一安全。C++11 起還有
//    std::common_type<T, U>::type 可用。
// 3. 推導**不會**跨越使用者定義的轉換。即使某型別有 operator double()，
//    也不會讓 T 被推導成 double —— 推導只做型別比對。
// 4. 陣列與函式在按值傳參時會 decay（const char[6] → const char*），
//    但綁到 T& 時**不會** decay（T 會被推導成 const char(&)[6]）。
//    這個差異是許多推導謎題的根源。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】模板引數推導與多載決議
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. template<typename T> T find_max(T,T) 呼叫 find_max(10, 3.14)
//        為什麼編譯失敗？int 不是可以自動轉 double 嗎？
//     答：兩個參數共用同一個 T，第一個推導出 int、第二個推導出 double，
//         同一個 T 得到兩個答案 → deduction conflict。關鍵在於**推導階段
//         不做隱式轉換**：編譯器是在做型別比對，沒有義務去找共同型別。
//         規則是「先推導，後轉換」—— T 定案之後轉換才會發生。
//     追問：那 find_max<double>(10, 3.14) 為什麼就可以？
//         → 明確指定 T=double 等於**略過推導**，參數型別直接是 (double,double)，
//           此時 int 實參照一般規則隱式轉成 double，完全合法。
//
// 🔥 Q2. 一個非模板函式和一個函式模板同名且都能匹配，編譯器選哪個？
//     答：兩者匹配程度相同（轉換代價一樣）時，**非模板函式優先**。
//         但如果模板能提供更好的匹配（例如非模板需要型別轉換、模板剛好精確
//         匹配），則模板勝出 —— 先比轉換代價，代價相同才用「非模板優先」
//         這條規則決勝。
//     追問：這條規則實務上有什麼用？
//         → 用來為熱點型別手寫最佳化版本：留一份泛型實作服務所有型別，
//           再為 int 之類的常見型別提供專屬重載，呼叫端完全不必改。
//
// ⚠️ 陷阱. 既然明確指定型別能解決推導衝突，那 find_max<int>(3.9, 2.1)
//          應該也很安全吧？
//     答：不安全。指定 T=int 之後，兩個 double 實參會被隱式轉換（截斷）成
//         int：3.9 → 3、2.1 → 2，回傳 3。程式照樣編譯、照樣執行，
//         但小數部分安靜地消失了。
//     為什麼會錯：把「明確指定型別」當成純粹的語法補救，忘了它同時
//         **關掉了推導原本提供的型別一致性保護**。推導衝突其實是編譯器在
//         告訴你「這兩個引數型別不一致，你確定嗎」；用顯式引數壓掉它，
//         等於回答「我確定」，後果自負。加 -Wconversion 可以讓這類截斷現形。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>

template <typename T>
T find_max(T a, T b) {
    return (a > b) ? a : b;
}

// 解法三：兩個獨立的型別參數，回傳型別由三元運算子的共同型別決定。
// 尾置回傳型別（trailing return type）是 C++11 起的語法。
template <typename T, typename U>
auto find_max2(T a, U b) -> decltype(a > b ? a : b) {
    return (a > b) ? a : b;
}

// 示範「非模板優先」：泛型版 + int 專用版
template <typename T>
const char* which_overload(T) { return "泛型模板版 which_overload<T>"; }

const char* which_overload(int) { return "非模板版 which_overload(int)"; }

int main() {
    std::cout << "=== 推導衝突 ===" << std::endl;
    // 問題：10 是 int，3.14 是 double，同一個 T 推導出兩種型別
    // std::cout << find_max(10, 3.14);  // 編譯錯誤！
    //   error: no matching function for call to 'find_max(int, double)'
    //   note:  deduced conflicting types for parameter 'T' ('int' and 'double')
    std::cout << "find_max(10, 3.14) 無法編譯：T 同時被推導為 int 與 double"
              << std::endl;

    std::cout << "\n=== 解法一：明確指定型別（略過推導，轉換恢復）===" << std::endl;
    std::cout << "find_max<double>(10, 3.14) = "
              << find_max<double>(10, 3.14) << std::endl;

    std::cout << "\n=== 解法二：讓兩個實參型別一致 ===" << std::endl;
    std::cout << "find_max(10.0, 3.14) = " << find_max(10.0, 3.14) << std::endl;

    std::cout << "\n=== 解法三：兩個獨立型別參數（真正支援混合型別）===" << std::endl;
    std::cout << "find_max2(10, 3.14) = " << find_max2(10, 3.14) << std::endl;
    std::cout << "find_max2(3.14, 10) = " << find_max2(3.14, 10) << std::endl;

    std::cout << "\n=== 陷阱：明確指定型別會關掉保護 ===" << std::endl;
    std::cout << "find_max<int>(3.9, 2.1) = " << find_max<int>(3.9, 2.1)
              << "   <- 3.9 與 2.1 被截斷成 3 與 2，小數安靜消失" << std::endl;

    std::cout << "\n=== 多載決議：非模板函式優先 ===" << std::endl;
    std::cout << "which_overload(42)   -> " << which_overload(42) << std::endl;
    std::cout << "which_overload(3.14) -> " << which_overload(3.14) << std::endl;
    std::cout << "which_overload<int>(42) -> " << which_overload<int>(42)
              << "   <- 加上 <int> 就強制走模板版" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第二課：泛型編程（Generic Programming）概念4.cpp -o concept4

// === 預期輸出 ===
// === 推導衝突 ===
// find_max(10, 3.14) 無法編譯：T 同時被推導為 int 與 double
//
// === 解法一：明確指定型別（略過推導，轉換恢復）===
// find_max<double>(10, 3.14) = 10
//
// === 解法二：讓兩個實參型別一致 ===
// find_max(10.0, 3.14) = 10
//
// === 解法三：兩個獨立型別參數（真正支援混合型別）===
// find_max2(10, 3.14) = 10
// find_max2(3.14, 10) = 10
//
// === 陷阱：明確指定型別會關掉保護 ===
// find_max<int>(3.9, 2.1) = 3   <- 3.9 與 2.1 被截斷成 3 與 2，小數安靜消失
//
// === 多載決議：非模板函式優先 ===
// which_overload(42)   -> 非模板版 which_overload(int)
// which_overload(3.14) -> 泛型模板版 which_overload<T>
// which_overload<int>(42) -> 泛型模板版 which_overload<T>   <- 加上 <int> 就強制走模板版
