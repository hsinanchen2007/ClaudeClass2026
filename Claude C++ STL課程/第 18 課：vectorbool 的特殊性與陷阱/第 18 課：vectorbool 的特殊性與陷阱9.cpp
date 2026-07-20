// =============================================================================
//  第 18 課-9：flip() —— vector<bool> 獨有、而且比你想的更快的操作
// =============================================================================
//
// 【主題資訊 Information】
//   void vector<bool>::flip() noexcept;              // 翻轉容器中所有位元
//   void vector<bool>::reference::flip() noexcept;   // 翻轉單一位元（代理物件的成員）
//   對照：一般的 vector<T> 完全沒有 flip()——它是 vector<bool> 特化才有的
//   標準版本：C++98 起（[vector.bool]）
//   複雜度：單一元素 O(1)；整個容器 O(bit 數)，但常數極小（見下）
//   標頭檔：<vector>
//
// 【詳細解釋 Explanation】
//
// 【1. flip() 為什麼只有 vector<bool> 有】
//   「翻轉」這個概念只對布林值有意義，對 int 或 string 沒有對應語意，
//   所以它不可能出現在通用的 vector<T> 介面裡。
//   這也是 vector<bool> 少數「特化帶來好處」的地方——
//   前面幾課講的都是它的代價，這一課講它換到了什麼。
//
// 【2. 整體 flip() 的效能優勢：一次翻 64 個 bit】
//   如果元素是一個一個存的（例如 vector<uint8_t>），
//   翻轉 N 個旗標就要跑 N 次迴圈。
//   但 vector<bool> 底層是一串字組（libstdc++ 在 x86-64 上用
//   64-bit 的 unsigned long），整體 flip() 可以直接對每個字組做
//   按位取反 ~word，一次翻掉 64 個 bit。
//   所以 N 個旗標只要約 N/64 次運算——這是它真正的效能賣點。
//   同理，如果你要對兩組旗標做 AND/OR，vector<bool> 反而做不到
//   （它沒有提供位元運算子），那正是 std::bitset 的守備範圍。
//
// 【3. 單一元素的 flip()：代理物件的成員函式】
//   vb[0].flip() 呼叫的不是容器的成員，而是「operator[] 回傳的
//   那個代理物件」的成員。它內部大致是 *word_ ^= mask_，
//   也就是對那個字組做 XOR，只翻掉目標那一個 bit。
//   這也再次說明代理物件不只是「假裝成 bool」——
//   它其實提供了 bool& 做不到的操作（bool& 沒有 flip 成員）。
//
// 【4. flip() vs vb[i] = !vb[i]】
//   兩者結果相同。差別在於：
//     vb[i] = !vb[i];     讀出 bit → 轉成 bool → 取反 → 寫回（多了轉換來回）
//     vb[i].flip();       直接對字組做一次 XOR
//   單次呼叫的差距微不足道，但意圖表達上 flip() 更清楚。
//   真正有意義的差距在「整體翻轉」：
//     for (auto&& b : vb) b = !b;    逐 bit 處理，O(N)
//     vb.flip();                     逐字組處理，約 O(N/64)
//
// 【5. 注意 flip() 沒有回傳值】
//   兩個版本都回傳 void，所以不能寫成 auto x = vb.flip();。
//   要取得翻轉後的值必須分兩步：vb[0].flip(); bool v = vb[0];
//
// 【概念補充 Concept Deep Dive】
//   ▸ 為什麼 std::bitset 有 operator~ 而 vector<bool> 沒有
//     bitset 的大小是編譯期常數，兩個 bitset<N> 必定等長，
//     所以 &、|、^、~ 都有明確定義。
//     vector<bool> 的長度是執行期決定的，兩個 vector<bool>
//     可能不等長，位元運算的語意會變得含糊（要截斷？補零？丟例外？），
//     標準因此只提供了單邊的 flip() 而沒有雙目運算子。
//   ▸ noexcept 的意義
//     兩個 flip() 都標了 noexcept——它們只做位元運算，
//     不配置記憶體、不呼叫使用者程式碼，不可能丟例外。
//     這讓它們可以安全地用在解構子與 noexcept 的函式裡。
//   ▸ 為什麼 vb.flip() 不會使迭代器失效
//     它不改變 size、不改變 capacity、不重新配置，
//     只是就地修改既有位元的內容。所有迭代器仍然有效。
//
// 【注意事項 Pay Attention】
//   1. flip() 是 vector<bool> 特化獨有的；一般 vector<T> 沒有這個成員。
//   2. 兩個版本都回傳 void，不能拿來初始化變數。
//   3. vb[i].flip() 呼叫的是代理物件的成員，不是容器的成員。
//   4. 整體 flip() 是逐字組處理，比手寫逐元素迴圈快得多。
//   5. vector<bool> 沒有 &、|、^、~ 等位元運算子；需要它們請用 std::bitset。
//   6. flip() 不改變 size/capacity，也不會使任何迭代器失效。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector<bool>::flip()
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. vb.flip() 和 for (auto&& b : vb) b = !b; 有什麼差別？
//     答：結果相同，效能不同。手寫迴圈是逐個 bit 處理，O(N)；
//         vb.flip() 直接對底層的每個字組做按位取反 ~word，
//         libstdc++ 在 x86-64 上用 64-bit 字組，
//         等於一次翻掉 64 個 bit，約 O(N/64)。
//         這是 vector<bool> 位元壓縮佈局換來的真正好處。
//     追問：那為什麼 vector<bool> 沒有 operator~ 或 operator&？
//         → 因為它的長度是執行期決定的，兩個 vector<bool> 可能不等長，
//           雙目位元運算的語意會含糊（截斷？補零？丟例外？）。
//           std::bitset<N> 的長度是編譯期常數、必定等長，
//           所以它才提供完整的位元運算子。
//
// 🔥 Q2. vb[0].flip() 呼叫的是誰的成員函式？
//     答：是 operator[] 回傳的那個代理物件
//         vector<bool>::reference 的成員函式，不是容器的成員。
//         它內部對目標字組做 XOR（*word_ ^= mask_），只翻那一個 bit。
//         這也說明代理物件不只是「模仿 bool&」——
//         它還提供了 bool& 根本沒有的 flip() 操作。
//     追問：flip() 會讓迭代器失效嗎？
//         → 不會。它不改變 size、不改變 capacity、不重新配置，
//           只是就地修改既有位元的內容，所有迭代器仍然有效。
//
// ⚠️ 陷阱. bool b = vb[0].flip(); 為什麼編譯不過？
//     答：因為 flip() 的回傳型別是 void——它是個「動作」而非「取值」。
//         要拿到翻轉後的結果必須分兩步：
//           vb[0].flip();  bool b = vb[0];
//     為什麼會錯：直覺上把 flip 當成類似 !x 的運算式（回傳新值），
//         但它其實是就地修改的命令式操作，和 std::advance 回傳 void
//         是同一種設計風格——動作與取值分離。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【日常實務範例】權限遮罩的批次反轉：黑名單 ↔ 白名單切換
//   情境：一個大型系統對數十萬個資源維護「可存取」旗標。
//         管理介面提供一個「反選」按鈕（把目前選取的整批反轉），
//         以及在黑名單模式與白名單模式之間切換的功能——
//         後者在語意上就是把整份遮罩翻轉。
//   為什麼用本主題：這正是 flip() 的主場——
//         整批反轉不需要逐一處理，一次呼叫就是逐字組運算。
//         範例同時對照手寫迴圈，讓「為什麼要用 flip()」變得具體。
// -----------------------------------------------------------------------------
class AccessMask {
    std::vector<bool> allowed_;
    bool              whitelistMode_ = true;
public:
    explicit AccessMask(std::size_t n) : allowed_(n, false) {}

    void allow(std::size_t id)  { allowed_[id] = true; }
    void deny(std::size_t id)   { allowed_[id] = false; }
    bool canAccess(std::size_t id) const { return allowed_[id]; }

    // 反選：把整份遮罩翻轉。一次呼叫，逐字組運算。
    void invertSelection() { allowed_.flip(); }

    // 黑白名單模式切換：語意上就是整份遮罩反轉
    void toggleMode() {
        whitelistMode_ = !whitelistMode_;
        allowed_.flip();
    }
    bool isWhitelistMode() const { return whitelistMode_; }

    std::size_t countAllowed() const {
        std::size_t c = 0;
        for (std::size_t i = 0; i < allowed_.size(); ++i) if (allowed_[i]) ++c;
        return c;
    }
    std::size_t size() const { return allowed_.size(); }
};

int main() {
    std::cout << std::boolalpha;

    std::cout << "=== 一、單一元素的 flip（代理物件的成員函式）===" << std::endl;
    std::vector<bool> vb = {true, false, true, true, false};

    std::cout << "翻轉前：";
    for (std::size_t i = 0; i < vb.size(); ++i) std::cout << (vb[i] ? 1 : 0);
    std::cout << std::endl;

    vb[0].flip();
    std::cout << "vb[0].flip()：";
    for (std::size_t i = 0; i < vb.size(); ++i) std::cout << (vb[i] ? 1 : 0);
    std::cout << std::endl;

    std::cout << "\n=== 二、整個容器的 flip（容器的成員函式）===" << std::endl;
    vb.flip();
    std::cout << "vb.flip()：";
    for (std::size_t i = 0; i < vb.size(); ++i) std::cout << (vb[i] ? 1 : 0);
    std::cout << std::endl;

    std::cout << "\n=== 三、flip() 回傳 void（以下為編譯錯誤，故註解）===" << std::endl;
    // bool b = vb[0].flip();   // 編譯錯誤！flip() 回傳 void
    vb[0].flip();
    bool b = vb[0];             // 正確：分兩步，先動作再取值
    std::cout << "先 flip 再取值：vb[0] = " << b << std::endl;

    std::cout << "\n=== 四、flip() 不會使迭代器失效 ===" << std::endl;
    std::vector<bool> w = {true, true, false};
    std::size_t capBefore = w.capacity();
    std::size_t sizeBefore = w.size();
    w.flip();
    std::cout << "flip 前後 size 相同: " << (w.size() == sizeBefore)
              << "，capacity 相同: " << (w.capacity() == capBefore)
              << "（不重新配置，故迭代器仍有效）" << std::endl;

    std::cout << "\n=== 五、flip() vs 手寫迴圈：結果相同、機制不同 ===" << std::endl;
    const std::size_t N = 1000;
    std::vector<bool> byFlip(N, false);
    std::vector<bool> byLoop(N, false);
    for (std::size_t i = 0; i < N; i += 3) { byFlip[i] = true; byLoop[i] = true; }

    byFlip.flip();                              // 逐字組：約 N/64 次運算
    for (auto&& bit : byLoop) bit = !bit;       // 逐 bit：N 次運算

    bool same = true;
    for (std::size_t i = 0; i < N && same; ++i) {
        if (static_cast<bool>(byFlip[i]) != static_cast<bool>(byLoop[i])) same = false;
    }
    std::cout << "兩種做法結果一致: " << same << std::endl;
    std::cout << "但 flip() 是對底層 64-bit 字組做 ~word（本機 libstdc++），"
              << "約 " << ((N + 63) / 64) << " 次運算即可完成 " << N << " 個 bit"
              << std::endl;

    std::cout << "\n=== 六、日常實務：權限遮罩的黑白名單切換 ===" << std::endl;
    AccessMask mask(1000);
    for (std::size_t i = 0; i < 1000; i += 4) mask.allow(i);   // 每 4 個放行一個
    std::cout << "白名單模式（isWhitelist=" << mask.isWhitelistMode()
              << "）：允許 " << mask.countAllowed() << " / " << mask.size() << std::endl;

    mask.toggleMode();
    std::cout << "切換為黑名單（isWhitelist=" << mask.isWhitelistMode()
              << "）：允許 " << mask.countAllowed() << " / " << mask.size() << std::endl;

    mask.invertSelection();
    std::cout << "再按一次「反選」：允許 " << mask.countAllowed()
              << " / " << mask.size() << "（回到原本的選取）" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 18 課：vectorbool 的特殊性與陷阱9.cpp" -o vb_flip
//
// 【關於下方預期輸出的但書】
//   「一次翻掉 64 個 bit」中的 64，來自 libstdc++ 在 x86-64 上
//   以 unsigned long（64 bits）作為底層字組單位，屬實作定義；
//   32-bit 平台或其他標準庫的字組大小可能不同。
//   標準只保證 flip() 的語意（翻轉所有位元），未規定其實作方式。
//
// 【本檔未附 LeetCode 範例的理由】
//   flip() 是 vector<bool> 特化獨有的容器 API，
//   LeetCode 的位元運算題（例如 191 Number of 1 Bits、190 Reverse Bits）
//   考的是整數的位元操作，用的是 &、|、^、>> 等運算子，
//   與容器的 flip() 沒有實質關聯——那些題目更適合放在
//   同課的 std::bitset 範例（13.cpp）中。
//   這裡改以權限遮罩的黑白名單切換呈現 flip() 真正的使用情境。

// === 預期輸出 ===
// === 一、單一元素的 flip（代理物件的成員函式）===
// 翻轉前：10110
// vb[0].flip()：00110
//
// === 二、整個容器的 flip（容器的成員函式）===
// vb.flip()：11001
//
// === 三、flip() 回傳 void（以下為編譯錯誤，故註解）===
// 先 flip 再取值：vb[0] = false
//
// === 四、flip() 不會使迭代器失效 ===
// flip 前後 size 相同: true，capacity 相同: true（不重新配置，故迭代器仍有效）
//
// === 五、flip() vs 手寫迴圈：結果相同、機制不同 ===
// 兩種做法結果一致: true
// 但 flip() 是對底層 64-bit 字組做 ~word（本機 libstdc++），約 16 次運算即可完成 1000 個 bit
//
// === 六、日常實務：權限遮罩的黑白名單切換 ===
// 白名單模式（isWhitelist=true）：允許 250 / 1000
// 切換為黑名單（isWhitelist=false）：允許 750 / 1000
// 再按一次「反選」：允許 250 / 1000（回到原本的選取）
