// =============================================================================
//  第 11 課：vector 的容量管理：size、capacity、reserve7.cpp
//  —  max_size()：理論上界，以及 reserve 超過它時丟的 length_error
// =============================================================================
//
// 【主題資訊 Information】
//   size_type max_size() const noexcept;
//
//   標頭檔：<vector>
//   標準版本：C++98 起；C++11 加 noexcept；C++20 起為 constexpr。
//   複雜度：O(1)（純算術，不查詢系統記憶體）。
//   回傳：這個 vector **理論上**可能容納的最大元素數
//         （標準用字：the maximum size of the largest possible container）。
//
//   libstdc++ 的實際公式（本機 GCC 15.2 實測）：
//       max_size() == PTRDIFF_MAX / sizeof(T)
//   所以它**隨元素型別而變**，不是一個固定的常數：
//       vector<char>   → 9223372036854775807  (PTRDIFF_MAX / 1)
//       vector<int>    → 2305843009213693951  (PTRDIFF_MAX / 4)
//       vector<double> → 1152921504606846975  (PTRDIFF_MAX / 8)
//
// 【詳細解釋 Explanation】
//
// 【1. max_size 不是「你能配置多少」】
// 這是最重要的一點：max_size() **不會**去問作業系統還有多少記憶體。它只是
// 一個由型別大小推出來的算術上界。本機實測 vector<int>::max_size() 約
// 2.3 × 10¹⁸，換算成記憶體是約 9.2 EB（exabyte）—— 遠超過任何真實機器。
// 換句話說，你在實務上幾乎永遠會先撞到 std::bad_alloc（真的沒記憶體了），
// 而不是撞到 max_size。
//
// 所以 max_size 的用途不是「檢查我配置得下嗎」，而是「這個數字在型別上
// 根本不合法嗎」—— 它區分的是**邏輯錯誤**（例如算出負數再轉成無號的巨大值）
// 與**資源不足**。
//
// 【2. 為什麼上界是 PTRDIFF_MAX / sizeof(T) 而不是 SIZE_MAX / sizeof(T)】
// 因為兩個 iterator 相減的結果型別是 difference_type（有號的 ptrdiff_t）。
// 若容器大小超過 PTRDIFF_MAX，`v.end() - v.begin()` 就會溢位 —— 有號整數
// 溢位是 UB。為了讓「任兩個 iterator 相減」永遠合法，實作把上界壓在
// PTRDIFF_MAX。這是型別系統推導出來的硬限制，不是保守估計。
//
// 【3. 超過 max_size 時會發生什麼：length_error】
// reserve(n) / resize(n) 在 n > max_size() 時，標準規定丟 **std::length_error**
// （定義在 <stdexcept>）。注意它和 bad_alloc 的分工：
//     std::length_error → 「你要的數量在型別上就不合法」（邏輯錯誤）
//     std::bad_alloc    → 「數量合法，但系統記憶體不夠」（資源錯誤）
// 本檔實測 reserve(max_size() + 1) 丟出 length_error，what() 是 "vector::reserve"。
//
// 【4. 這個機制真正救到人的場合】
// 最典型的是**無號數下溢**。例如：
//     std::vector<int> v;                  // 空的
//     v.reserve(v.size() - 1);             // 0 - 1 → SIZE_MAX！
// size() 是無號型別，0 - 1 會 wrap around 成巨大的數，遠超過 max_size
// → 丟 length_error。若沒有這道檢查，程式會拿著一個荒謬的數字去要記憶體。
// 這正是為什麼「vector 的大小相關運算要小心無號減法」（見 1.cpp 陷阱 2）。
//
// 【概念補充 Concept Deep Dive】
// (A) max_size 與 allocator 的關係
//   標準規定 max_size() 應反映 allocator_traits<Allocator>::max_size()。
//   預設的 std::allocator<T> 回傳 PTRDIFF_MAX / sizeof(T)，所以我們看到
//   上面那組數字。若你換成自訂 allocator（例如固定大小的記憶體池），
//   max_size 可以小很多 —— 此時它就真的變成有意義的實務上界了。
//
// (B) 為什麼 max_size 是 O(1) 且 noexcept
//   因為它只是編譯期常數除法，不做任何系統呼叫、不查詢可用記憶體。
//   這也解釋了它為何「不切實際」：真正的可用記憶體是動態的、會變的，
//   不可能用一個 noexcept 的 O(1) 函式回答。
//
// (C) vector<bool> 的 max_size 是特例
//   std::vector<bool> 是位元壓縮的特化版本，一個 byte 存 8 個 bool，
//   其 max_size 的計算方式與一般 vector<T> 不同（受位元定址限制）。
//   這是 vector<bool> 眾多「它其實不是容器」的怪異之處之一。
//
// 【注意事項 Pay Attention】
// 1. **max_size() 不代表你配置得到那麼多。** 它只是型別上界；
//    實務上會先撞 std::bad_alloc。拿它來做容量規劃是沒有意義的。
// 2. 它的值**隨 sizeof(T) 而變**。常見的錯誤說法是「64 位元上就是
//    4611686018427387903」—— 那個數字對應的是 2 bytes 的元素。
//    本機 vector<int> 實測是 2305843009213693951。
// 3. 具體數值是**實作定義**：libstdc++ 用 PTRDIFF_MAX / sizeof(T)，
//    MSVC 與 libc++ 的公式不同，數字會不一樣。
// 4. reserve/resize 超過 max_size 丟 **std::length_error**（不是 bad_alloc、
//    也不是靜默失敗）。要 #include <stdexcept> 才能接。
// 5. 小心無號下溢：`v.reserve(v.size() - 1)` 在空 vector 上是 SIZE_MAX，
//    會直接丟 length_error。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】max_size
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. max_size() 回傳的是「我最多能配置多少元素」嗎？
//     答：不是。它只是由型別推出的**理論上界**（libstdc++ 是
//         PTRDIFF_MAX / sizeof(T)），完全不查詢系統可用記憶體。
//         本機 vector<int> 約 2.3×10¹⁸ 個元素、約 9.2 EB，
//         實務上一定先撞 std::bad_alloc。
//     追問：那它有什麼用？
//         → 區分「邏輯錯誤」與「資源不足」：超過 max_size 丟 length_error
//           （數量本身不合法），記憶體不夠才丟 bad_alloc。無號下溢算出
//           SIZE_MAX 這種 bug 就是靠它擋下來的。
//
// 🔥 Q2. 為什麼上界是 PTRDIFF_MAX / sizeof(T)，而不是 SIZE_MAX / sizeof(T)？
//     答：因為兩個 iterator 相減的結果型別是有號的 difference_type
//         (ptrdiff_t)。若元素數超過 PTRDIFF_MAX，v.end() - v.begin()
//         就會有號溢位（UB）。把上界壓在 PTRDIFF_MAX 才能保證任兩個
//         iterator 相減永遠合法。
//     追問：所以 max_size 會隨元素型別改變？
//         → 會。本機實測 vector<char> 是 9223372036854775807、
//           vector<int> 是 2305843009213693951、
//           vector<double> 是 1152921504606846975。
//
// ⚠️ 陷阱 1. `v.reserve(v.size() - 1)` 在空 vector 上會發生什麼事？
//     答：size() 是無號型別，0 - 1 會 wrap around 成 SIZE_MAX，
//         遠超過 max_size() → 丟 **std::length_error**。
//         不是「reserve 0 個」，也不是靜默忽略。
//     為什麼會錯：把 size() 當成有號 int，以為 0-1 是 -1、
//         再以為負數會被當成 0 處理。無號運算沒有負數這回事。
//
// ⚠️ 陷阱 2. 超過容量上限時丟的是 bad_alloc 還是 length_error？
//     答：**length_error**。兩者分工明確：length_error 表示「你要的數量
//         在型別上就不合法」（超過 max_size）；bad_alloc 表示「數量合法，
//         但系統給不出這麼多記憶體」。接錯例外型別會漏掉真正的錯誤。
//     為什麼會錯：直覺認為「跟記憶體有關的失敗都是 bad_alloc」，
//         忽略了標準刻意用兩種例外區分邏輯錯誤與資源錯誤。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <stdexcept>
#include <string>
#include <cstdint>   // PTRDIFF_MAX

int main() {
    std::vector<int> v;

    std::cout << "=== max_size 隨元素型別而變 ===" << std::endl;
    std::cout << "vector<char>   max_size: "
              << std::vector<char>().max_size() << std::endl;
    std::cout << "vector<int>    max_size: "
              << v.max_size() << std::endl;
    std::cout << "vector<double> max_size: "
              << std::vector<double>().max_size() << std::endl;
    std::cout << "vector<string> max_size: "
              << std::vector<std::string>().max_size() << std::endl;

    std::cout << "\n=== 驗證公式 PTRDIFF_MAX / sizeof(T) ===" << std::endl;
    // libstdc++ 實作：max_size() == PTRDIFF_MAX / sizeof(T)
    const std::size_t ptrdiffMax = static_cast<std::size_t>(PTRDIFF_MAX);
    std::cout << "PTRDIFF_MAX          : " << ptrdiffMax << std::endl;
    std::cout << "PTRDIFF_MAX/sizeof(int): "
              << ptrdiffMax / sizeof(int) << std::endl;
    std::cout << "與 max_size() 相符    : "
              << (ptrdiffMax / sizeof(int) == v.max_size() ? "是" : "否")
              << std::endl;

    std::cout << "\n=== 這個上界有多不切實際 ===" << std::endl;
    // 換算成 EB（exabyte）：1 EB = 10^18 bytes
    const double bytesNeeded =
        static_cast<double>(v.max_size()) * sizeof(int);
    std::cout << "裝滿 vector<int> 需要約 "
              << static_cast<long long>(bytesNeeded / 1e18)
              << " EB 記憶體 → 實務上必定先撞 std::bad_alloc" << std::endl;

    std::cout << "\n=== 超過 max_size 丟 std::length_error ===" << std::endl;
    try {
        v.reserve(v.max_size() + 1);
        std::cout << "reserve 成功（不應該發生）" << std::endl;
    } catch (const std::length_error& e) {
        std::cout << "捕捉到 std::length_error, what() = " << e.what()
                  << std::endl;
    }

    std::cout << "\n=== 經典陷阱：無號下溢 ===" << std::endl;
    std::vector<int> empty;
    std::cout << "empty.size() = " << empty.size() << std::endl;
    // 0 - 1 在無號型別上 wrap around 成 SIZE_MAX，而非 -1
    std::cout << "empty.size() - 1 = " << (empty.size() - 1)
              << "  ← 不是 -1！" << std::endl;
    try {
        empty.reserve(empty.size() - 1);
        std::cout << "reserve 成功（不應該發生）" << std::endl;
    } catch (const std::length_error&) {
        std::cout << "reserve(size()-1) 丟出 std::length_error → 被擋下來了"
                  << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 11 課：vector 的容量管理：size、capacity、reserve7.cpp" -o demo7

// 註：以下 max_size 數值為 libstdc++ / GCC 15.2（64-bit）實測，**非標準保證**。
//     libstdc++ 的公式是 PTRDIFF_MAX / sizeof(T)，故數值隨元素型別而變；
//     MSVC 與 libc++ 的計算方式不同，會得到不同數字。
//     （常見的錯誤說法「64 位元上就是 4611686018427387903」對應的是
//       2 bytes 的元素，並非 vector<int>。）

// === 預期輸出 ===
// === max_size 隨元素型別而變 ===
// vector<char>   max_size: 9223372036854775807
// vector<int>    max_size: 2305843009213693951
// vector<double> max_size: 1152921504606846975
// vector<string> max_size: 288230376151711743
//
// === 驗證公式 PTRDIFF_MAX / sizeof(T) ===
// PTRDIFF_MAX          : 9223372036854775807
// PTRDIFF_MAX/sizeof(int): 2305843009213693951
// 與 max_size() 相符    : 是
//
// === 這個上界有多不切實際 ===
// 裝滿 vector<int> 需要約 9 EB 記憶體 → 實務上必定先撞 std::bad_alloc
//
// === 超過 max_size 丟 std::length_error ===
// 捕捉到 std::length_error, what() = vector::reserve
//
// === 經典陷阱：無號下溢 ===
// empty.size() = 0
// empty.size() - 1 = 18446744073709551615  ← 不是 -1！
// reserve(size()-1) 丟出 std::length_error → 被擋下來了
