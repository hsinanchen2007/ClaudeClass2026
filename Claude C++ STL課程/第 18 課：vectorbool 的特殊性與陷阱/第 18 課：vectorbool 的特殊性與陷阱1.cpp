// =============================================================================
//  第 18 課：vectorbool 的特殊性與陷阱1.cpp  —  vector<bool> 的第一印象：看起來很正常
// =============================================================================
//
// 【主題資訊 Information】
//   template<class Allocator> class vector<bool, Allocator>;   // 標準「強制」的特化
//   標頭檔  : <vector>
//   標準版本: C++98 起即為標準強制的 partial specialization（不是實作自己加的）
//   value_type      : bool
//   reference       : 實作定義的 proxy class（不是 bool&）
//   const_reference : bool（標準明文規定就是純量 bool，不是 proxy）
//   range-for 逐一讀取: O(N)
//
// 【詳細解釋 Explanation】
//
// 【1. 這支程式為什麼「看起來」完全正常】
//   std::vector<bool> vb = {true, false, true, true, false};
//   for (bool b : vb) std::cout << b << " ";
//
//   輸出 1 0 1 1 0，跟 std::vector<int> 的寫法一模一樣。這正是 vector<bool> 最
//   危險的地方：它在「最常見的用法」上偽裝得非常好，你要寫到比較進階的用法
//   （取址、綁引用、傳給模板）才會突然爆炸。所以本課的順序是刻意的——先讓你
//   看到它正常的一面，再逐檔拆解它不正常的地方。
//
// 【2. 為什麼這裡寫 bool b 就沒事】
//   range-for 會展開成大致如下的形式：
//       for (auto __it = vb.begin(); __it != vb.end(); ++__it) {
//           bool b = *__it;            // ← 關鍵在這一行
//           ...
//       }
//   *__it 的型別不是 bool&，而是 proxy 物件。但因為你宣告的是 bool b，proxy 的
//   operator bool() const 會被呼叫，把「那個 bit 目前的值」轉換成一個獨立的
//   bool 副本。副本一旦生成，就跟容器脫鉤了，行為完全正常。
//
//   換句話說：宣告成 bool 這個動作，順手幫你「拆掉」了 proxy。這也是本課最重要
//   的一條實務守則——讀 vector<bool> 的元素時明確寫 bool，不要寫 auto。
//
// 【3. 四種 range-for 寫法的差別（本檔實測）】
//   for (bool b : vb)     → OK。每輪由 proxy 隱式轉成 bool 副本。最推薦。
//   for (auto&& b : vb)   → OK。rvalue reference 可以綁定 proxy 這個 prvalue，
//                           b 是 proxy，可以寫回容器。泛型程式碼常用這招。
//   for (auto& b : vb)    → 編譯錯誤！non-const lvalue reference 綁不住 prvalue。
//                           對 vector<int> 天天在用的寫法，在這裡直接掛掉。
//   for (auto b : vb)     → 能編譯，但 b 是 proxy 副本，仍然指向容器內部的 bit，
//                           不是你以為的獨立 bool（詳見第 4 檔）。
//
// 【概念補充 Concept Deep Dive】
//   vector<bool> 的底層不是 bool 陣列，而是一串「word」（libstdc++ 用
//   unsigned long，本機實測 sizeof = 8 bytes = 64 bits，此為實作定義）。
//   第 i 個 bool 存在第 (i / 64) 個 word 的第 (i % 64) 個 bit。
//   讀取一個元素其實是：
//       (*(word_ptr + i / 64) >> (i % 64)) & 1ul
//   這是幾道 shift 與 and 指令，比讀一個 byte 稍貴，但省了 8 倍記憶體頻寬——
//   在資料量夠大、cache miss 才是瓶頸的場合，整體反而更快。
//
// 【注意事項 Pay Attention】
//   1. vector<bool> 的特化是「標準強制」的，不是某個實作的怪癖。任何符合標準的
//      實作都必須這樣做，換編譯器也躲不掉。
//   2. 但 bit packing 的細節（word 型別、word 大小、bit 在 word 內的排列順序）
//      是【實作定義】，程式碼不可以依賴。
//   3. 讀取元素請寫 bool，不要寫 auto。
//   4. for (auto& b : vb) 在 vector<bool> 上是編譯錯誤，這是泛型程式碼被
//      vector<bool> 打斷的最常見入口。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector<bool> 的基本認識
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::vector<bool> 和 std::vector<int> 是同一個 class template 嗎？
//     答：不是同一份實例化結果。vector<bool> 是標準「強制規定」的 partial
//         specialization，內部改用 bit packing 儲存，介面也跟主模板不一致
//         （例如沒有 data()、operator[] 不回傳 bool&）。
//         這是標準規定，不是某個 STL 實作自己的最佳化。
//     追問：那它還算不算一個 Container？→ 不算完整的。標準的 Container 需求
//         要求 &c[0] 能得到 T*，vector<bool> 做不到，所以它不滿足 Container
//         requirements，泛型程式碼不能假設它跟其他 vector 一樣。
//
// 🔥 Q2. for (bool b : vb) 為什麼可以正常運作？
//     答：range-for 展開後是 bool b = *it;。*it 回傳 proxy，宣告成 bool 會觸發
//         proxy 的 operator bool() const，得到一個與容器脫鉤的獨立副本。
//         正是「明確寫 bool」這個動作救了你。
//     追問：改成 for (auto& b : vb) 會怎樣？→ 編譯錯誤。*it 是 prvalue proxy，
//         non-const lvalue reference 綁不住它。要能寫回去必須用 auto&&。
//
// ⚠️ 陷阱. 「vector<bool> 只是省記憶體的最佳化，用法跟 vector<int> 一樣。」
//     答：錯。省記憶體是動機，但代價是它回傳 proxy 而非真引用，因而破壞了
//         取址、綁引用、data()、以及大量泛型模板的假設。它不是「一樣但更小」，
//         而是「介面就不一樣的另一種容器」。
//     為什麼會錯：多數人腦中把它想成 vector<char> 的壓縮版，以為壓縮只發生在
//         儲存層、介面不受影響。但 C++ 最小可定址單位是 byte，壓到 bit 之後
//         就再也生不出 bool& 了，介面被迫跟著改。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>

int main() {
    std::vector<bool> vb = {true, false, true, true, false};

    std::cout << "=== 原始範例：for (bool b : vb) ===" << std::endl;
    for (bool b : vb) {
        std::cout << b << " ";
    }
    std::cout << std::endl;

    // -------------------------------------------------------------------------
    // 四種 range-for 寫法對照
    // -------------------------------------------------------------------------
    std::cout << "\n=== 寫法 1：bool b（推薦，取得獨立副本）===" << std::endl;
    for (bool b : vb) std::cout << b;
    std::cout << "   <- 每輪由 proxy 隱式轉成 bool" << std::endl;

    std::cout << "\n=== 寫法 2：auto&& b（可寫回容器）===" << std::endl;
    for (auto&& b : vb) {
        b = !b;                 // b 是 proxy，寫入會直接改到容器內的 bit
    }
    std::cout << "全部取反後：";
    for (bool b : vb) std::cout << b;
    std::cout << std::endl;

    for (auto&& b : vb) b = !b;         // 還原

    std::cout << "\n=== 寫法 3：auto& b（編譯錯誤，故註解）===" << std::endl;
    // for (auto& b : vb) { b = true; }
    //   error: cannot bind non-const lvalue reference of type
    //          'std::_Bit_reference&' to an rvalue of type 'std::_Bit_reference'
    // 對 vector<int> 完全正常的寫法，在 vector<bool> 上直接編不過。
    std::cout << "auto& 綁不住 prvalue proxy，這行必須註解掉才能編譯" << std::endl;

    std::cout << "\n=== 寫法 4：auto b（能編譯，但 b 是 proxy 不是 bool）===" << std::endl;
    for (auto b : vb) {
        std::cout << b;                 // 讀取時看起來正常，陷阱在第 4 檔展開
    }
    std::cout << "   <- sizeof(b) = " << sizeof(*vb.begin())
              << " bytes，不是 1（實作定義）" << std::endl;

    // -------------------------------------------------------------------------
    // 對照組：同樣的寫法在 vector<int> 上全部合法
    // -------------------------------------------------------------------------
    std::cout << "\n=== 對照組：vector<int> 三種寫法都合法 ===" << std::endl;
    std::vector<int> vi = {1, 2, 3};
    for (int x : vi)     std::cout << x;          // OK
    std::cout << " / ";
    for (auto&& x : vi)  std::cout << x;          // OK
    std::cout << " / ";
    for (auto& x : vi)   std::cout << x;          // OK <- 只有這個在 bool 版掛掉
    std::cout << std::endl;

    std::cout << "\n結論：vector<bool> 在最單純的讀取上偽裝良好，" << std::endl;
    std::cout << "      一旦需要「真引用」，差異就浮現了。" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 18 課：vectorbool 的特殊性與陷阱1.cpp" -o vb1

// === 預期輸出 ===
// === 原始範例：for (bool b : vb) ===
// 1 0 1 1 0
//
// === 寫法 1：bool b（推薦，取得獨立副本）===
// 10110   <- 每輪由 proxy 隱式轉成 bool
//
// === 寫法 2：auto&& b（可寫回容器）===
// 全部取反後：01001
//
// === 寫法 3：auto& b（編譯錯誤，故註解）===
// auto& 綁不住 prvalue proxy，這行必須註解掉才能編譯
//
// === 寫法 4：auto b（能編譯，但 b 是 proxy 不是 bool）===
// 10110   <- sizeof(b) = 16 bytes，不是 1（實作定義）
//
// === 對照組：vector<int> 三種寫法都合法 ===
// 123 / 123 / 123
//
// 結論：vector<bool> 在最單純的讀取上偽裝良好，
//       一旦需要「真引用」，差異就浮現了。
