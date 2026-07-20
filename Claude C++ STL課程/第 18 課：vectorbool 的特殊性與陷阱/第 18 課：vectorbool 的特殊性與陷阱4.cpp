// =============================================================================
//  第 18 課：vectorbool 的特殊性與陷阱4.cpp  —  陷阱一：auto 接到的是 proxy 不是 bool
// =============================================================================
//
// 【主題資訊 Information】
//   auto val  = vb[0];   // val 的型別是 vector<bool>::reference（proxy），不是 bool
//   bool val  = vb[0];   // 觸發 operator bool()，取得與容器脫鉤的獨立副本
//   auto val  = static_cast<bool>(vb[0]);   // 也可以，但不如直接寫 bool 清楚
//   標頭檔  : <vector>
//   標準版本: 問題自 C++98 特化就存在；auto 型別推導是 C++11 引入，兩者一相遇就出事
//   複雜度  : O(1)
//
// 【詳細解釋 Explanation】
//
// 【1. auto 到底做了什麼】
//   auto 的推導規則跟函式模板參數推導一樣：以初始化式的型別為準，去掉 reference
//   與頂層 const。這裡的初始化式 vb[0] 是一個 prvalue proxy 物件，所以：
//       auto val = vb[0];    // val 的型別 = vector<bool>::reference
//   注意 auto 完全沒有做錯——它忠實地推導出了運算式的型別。錯的是我們的預期：
//   我們以為 vb[0] 的型別是 bool，但它不是。
//
//   對比 vector<int>：
//       auto x = vi[0];      // vi[0] 是 int&，auto 去掉 reference → x 是 int（副本）
//   在 vector<int> 上，auto 幫我們「複製了值」；在 vector<bool> 上，auto 幫我們
//   「複製了座標」。這一字之差就是整個陷阱。
//
// 【2. 為什麼複製 proxy 不會讓它脫鉤】
//   proxy 內部是 { word 指標, bit 遮罩 }。複製 proxy 是複製這兩個欄位，複製出來的
//   新 proxy 指向的仍然是「同一個 word 的同一個 bit」。
//
//       vb 的儲存:   word[0] = ...0000_0101   （bit0=1, bit1=0, bit2=1）
//                                      ↑
//       auto val ──────────────────────┘   （val 存的是「指向 bit0」這個座標）
//
//   之後 vb[0] = false 把 word 的 bit0 改成 0，val 再讀出來自然就是 0。
//   val 從頭到尾都不是「當時那個值的快照」，而是「一個永遠讀取最新值的視窗」。
//
//   換成 bool val2 = vb[1]; 則完全不同：operator bool() 立刻把那個 bit 讀成一個
//   獨立的 bool 存進 val2，之後 word 怎麼變都與 val2 無關。
//
// 【3. 這個 bug 為什麼特別難抓】
//   (a) 它不會有任何編譯警告——auto val = vb[0]; 是完全合法的程式碼。
//   (b) 印出來的型別看起來也對：std::cout << val 會走 operator bool()，
//       顯示 0/1，跟 bool 一模一樣。
//   (c) 只有在「先讀值、後改容器、再用先前讀到的值」這個時序下才會出錯。
//       單元測試若沒有這個時序，可以一路綠燈。
//   (d) 對 vector<int> 寫一樣的程式碼是對的，所以 code review 也很容易放過。
//
// 【4. 更危險的變形：dangling proxy】
//   proxy 存的是指向容器內部 word 的指標，所以它跟 iterator 一樣會失效：
//       std::vector<bool> v(10, false);
//       auto p = v[0];       // p 指向目前這塊 buffer
//       v.resize(100000);    // 重新配置！舊 buffer 被釋放
//       bool b = p;          // 讀取已釋放的記憶體 → undefined behavior
//   同理，容器一旦離開作用域被解構，先前存下的 proxy 也就懸空了。
//   這是 undefined behavior：可能讀到舊值、可能讀到垃圾、可能觸發記憶體錯誤，
//   也可能表面上「看起來正常」——正因為它常常看起來正常，才更難察覺。
//   本檔刻意不執行這種寫法，只在註解中說明。
//
// 【5. 正確寫法】
//       bool val = vb[0];                       // 最推薦，意圖最清楚
//       auto val = static_cast<bool>(vb[0]);    // 等價，但囉嗦
//       auto val = bool(vb[0]);                 // 同上
//   若你在泛型模板裡不能寫死 bool，可以用：
//       auto val = static_cast<std::vector<bool>::value_type>(vb[0]);   // value_type 是 bool
//   或 C++20 起用 std::ranges::range_value_t 取得 value_type。
//
// 【概念補充 Concept Deep Dive】
//   這其實是「proxy object 破壞 auto」的通例，不只 vector<bool>：
//     * Eigen / Armadillo 等線性代數函式庫的 expression template
//       （auto x = A + B; 存下的是「尚未求值的運算式」，A、B 改了 x 就變，
//        甚至 A、B 死了 x 就懸空）
//     * std::bitset::operator[] 也回傳 proxy（std::bitset::reference）
//     * 各種資料庫 / JSON 函式庫的 row["col"] 代理
//   通用守則：只要一個型別的 operator[] 或運算子回傳的不是你要的值本身，
//   就不要用 auto 存它；要嘛立刻明確轉型，要嘛立刻用完丟掉。
//
//   Scott Meyers 在《Effective Modern C++》Item 6 講的就是這件事：
//   「對 invisible proxy types 使用 explicitly typed initializer idiom」，
//   也就是寫 auto val = static_cast<bool>(vb[0]); 或乾脆寫 bool val = vb[0];。
//
// 【注意事項 Pay Attention】
//   1. auto val = vb[0]; 不會有編譯警告，也不會立刻出錯，非常容易漏掉。
//   2. proxy 會隨容器重新配置（resize/push_back/reserve）或解構而失效，
//      使用失效的 proxy 是 undefined behavior，不保證任何特定結果。
//   3. 對 const 容器 auto 是安全的（const_reference 就是 bool），但不要依賴這點
//      來決定何時可以用 auto——加不加 const 會讓型別整個換掉，太脆弱。
//   4. 同樣的陷阱存在於 std::bitset 與各種 expression template 函式庫。
//   5. 唯一好記且不會錯的守則：讀 vector<bool> 的元素一律明確寫 bool。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】auto 與 proxy reference
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. auto val = vb[0]; 之後修改 vb[0]，val 會不會跟著變？為什麼？
//     答：會變。auto 推導出的是 proxy（vector<bool>::reference），內部存的是
//         「word 指標 + bit 遮罩」這組座標，不是值。複製 proxy 只複製座標，
//         新舊 proxy 仍指向同一個 bit，所以讀 val 永遠讀到那個 bit 的最新值。
//         寫 bool val = vb[0]; 才會觸發 operator bool() 取得獨立副本。
//     追問：那 auto val = vi[0];（vector<int>）為什麼就沒事？→ vi[0] 是 int&，
//         auto 推導會去掉 reference，得到 int 的副本。真正的差別在於
//         vector<bool> 的 operator[] 回傳的根本不是 T&。
//
// 🔥 Q2. 為什麼這個 bug 在實務上特別難抓？
//     答：四個原因疊加——沒有編譯警告；印出來跟 bool 完全一樣（會走
//         operator bool()）；只在「讀值 → 改容器 → 用舊值」的特定時序下才出錯；
//         而且同樣寫法在 vector<int> 上是對的，code review 容易放過。
//     追問：怎麼系統性避免？→ 團隊規約：vector<bool> 的元素一律明確寫 bool；
//         或 Meyers 的 explicitly typed initializer idiom
//         （auto val = static_cast<bool>(vb[0]);）。
//
// ⚠️ 陷阱. 「那我 auto val = vb[0]; 之後馬上用，不改容器就沒事吧？」
//     答：讀值馬上用確實通常正確，但 proxy 還有生命期問題：它指向容器目前的
//         buffer，一旦容器 resize/push_back 觸發重新配置，或容器被解構，
//         這個 proxy 就懸空了。之後讀它是 undefined behavior，不保證任何結果，
//         包括「看起來正常」在內。
//     為什麼會錯：把 proxy 想成「值 + 一點小怪癖」，其實它更像 iterator——
//         有 aliasing 也有失效問題，該用對待 iterator 的謹慎去對待它。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>   // std::min

// -----------------------------------------------------------------------------
// 【日常實務範例】feature flag 熱重載造成的「幽靈開關」
//   情境：服務用一個 vector<bool> 存幾千個 feature flag（用 flag id 當索引）。
//         背景執行緒每分鐘從設定中心拉一次新設定並覆寫這些 flag。
//         請求處理流程會在進入時「快照」目前的旗標，之後整個請求都依這份快照走，
//         以確保同一個請求前後行為一致（不會處理到一半開關被翻掉）。
//   Bug：快照如果寫成 auto，存下的是 proxy 而非值，設定一重載，
//        「快照」就跟著變了——同一個請求前半段走 A 分支、後半段走 B 分支，
//        產生極難重現的間歇性錯誤。
//   修法：快照一律明確寫 bool。
// -----------------------------------------------------------------------------
class FeatureFlags {
public:
    explicit FeatureFlags(std::size_t n) : flags_(n, false) {}

    void set(std::size_t id, bool on) { flags_[id] = on; }
    bool get(std::size_t id) const { return flags_[id]; }   // 回傳值，安全

    // 模擬設定中心推來一份新設定（熱重載）
    // 刻意用「逐元素就地覆寫」而不是 flags_ = incoming：
    // operator= 有可能重新配置 buffer，那會讓先前取得的 proxy 直接懸空（UB），
    // 本檔要示範的是 aliasing 而不是 UB，所以這裡保證不重新配置。
    void reload(const std::vector<bool>& incoming) {
        const std::size_t n = std::min(flags_.size(), incoming.size());
        for (std::size_t i = 0; i < n; ++i) {
            flags_[i] = incoming[i];        // 就地寫 bit，buffer 位址不變
        }
    }

    // 錯誤示範：把「快照」宣告成 auto，實際存到的是 proxy
    // 回傳型別刻意寫成 proxy 以重現 bug（真實程式碼請勿模仿）
    std::vector<bool>::reference snapshotWrong(std::size_t id) { return flags_[id]; }

    // 正確示範：明確寫 bool，取得與容器脫鉤的值
    bool snapshotRight(std::size_t id) const { return flags_[id]; }

private:
    std::vector<bool> flags_;
};

int main() {
    std::cout << "=== 原始範例：auto vs bool ===" << std::endl;

    std::vector<bool> vb = {true, false, true};

    // 陷阱：auto 推導為 proxy，不是 bool
    auto val = vb[0];   // 型別是 vector<bool>::reference

    vb[0] = false;      // 修改原容器

    // val 是代理物件，它「連結」到 vb 的內部
    // 所以 val 也跟著變了！
    std::cout << "val = " << val << std::endl;  // 0（false），不是預期的 1（true）

    // 正確做法：明確宣告為 bool
    bool val2 = vb[1];  // 強制轉型為 bool，取得獨立的副本
    vb[1] = true;
    std::cout << "val2 = " << val2 << std::endl;    // 0（false），不受影響
    std::cout << "vb[1] = " << vb[1] << std::endl;  // 1（true），已修改

    // -------------------------------------------------------------------------
    // 三種寫法並排對照
    // -------------------------------------------------------------------------
    std::cout << "\n=== 三種寫法並排對照 ===" << std::endl;
    std::vector<bool> v = {true, true, true};

    auto  a = v[0];                          // proxy（危險）
    bool  b = v[1];                          // 值（安全）
    auto  c = static_cast<bool>(v[2]);       // 值（安全，Meyers 的 idiom）

    std::cout << "改動容器前： a=" << a << " b=" << b << " c=" << c << std::endl;

    v[0] = false;
    v[1] = false;
    v[2] = false;

    std::cout << "改動容器後： a=" << a << " b=" << b << " c=" << c << std::endl;
    std::cout << "             ↑ 只有 auto 那個跟著變了" << std::endl;

    // -------------------------------------------------------------------------
    // 對照組：vector<int> 上同樣的寫法完全正常
    // -------------------------------------------------------------------------
    std::cout << "\n=== 對照組：vector<int> 沒有這個問題 ===" << std::endl;
    std::vector<int> vi = {10, 20};
    auto ai = vi[0];                         // ai 是 int 副本（auto 去掉了 reference）
    vi[0] = 99;
    std::cout << "vi[0] 改成 99 後，ai = " << ai
              << "（不受影響，因為 auto 從 int& 推出 int）" << std::endl;

    // -------------------------------------------------------------------------
    // proxy 也可以拿來「寫」——這是它存在的理由，不全然是壞事
    // -------------------------------------------------------------------------
    std::cout << "\n=== proxy 的正面用途：當成可寫的把手 ===" << std::endl;
    std::vector<bool> w = {false, false, false};
    auto handle = w[1];                      // 刻意保留 proxy 當作「把手」
    handle = true;                           // 透過把手寫回容器
    std::cout << "透過 proxy 寫入後 w = ";
    for (bool x : w) std::cout << x;
    std::cout << "   <- w[1] 被改成 1 了" << std::endl;
    std::cout << "（proxy 的設計目的正是讓 vb[i] = true 這種語法能運作）" << std::endl;

    // -------------------------------------------------------------------------
    // dangling proxy：只說明不執行
    // -------------------------------------------------------------------------
    std::cout << "\n=== dangling proxy（只說明，本檔不執行）===" << std::endl;
    std::cout << "  std::vector<bool> t(10, false);" << std::endl;
    std::cout << "  auto p = t[0];      // p 指向目前的 buffer" << std::endl;
    std::cout << "  t.resize(100000);   // 重新配置，舊 buffer 被釋放" << std::endl;
    std::cout << "  bool x = p;         // 讀取已釋放記憶體 -> undefined behavior" << std::endl;
    std::cout << "UB 不保證任何特定結果，包括「看起來正常」在內，所以不可依賴。"
              << std::endl;

    // -------------------------------------------------------------------------
    // 日常實務：feature flag 熱重載
    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務：feature flag 熱重載的幽靈開關 ===" << std::endl;
    const std::size_t kNewCheckout = 7;

    FeatureFlags ff(64);
    ff.set(kNewCheckout, true);              // 目前新版結帳流程是開的

    // 請求進來，各自快照一份
    auto snap_wrong = ff.snapshotWrong(kNewCheckout);   // 存到 proxy（錯）
    bool snap_right = ff.snapshotRight(kNewCheckout);    // 存到 bool（對）

    std::cout << "請求開始： 錯誤快照=" << snap_wrong
              << " 正確快照=" << snap_right << std::endl;

    // 請求處理到一半，設定中心把這個 flag 關掉
    std::vector<bool> incoming(64, false);
    ff.reload(incoming);

    std::cout << "熱重載後： 錯誤快照=" << snap_wrong
              << " 正確快照=" << snap_right << std::endl;
    std::cout << "錯誤快照在請求進行中被翻掉了 -> 同一個請求前後走不同分支" << std::endl;
    std::cout << "正確快照維持請求開始時的值 -> 行為一致" << std::endl;
    std::cout << "註：本例的 reload 刻意逐元素就地覆寫，buffer 不變，所以只是 aliasing。"
              << std::endl;
    std::cout << "    若 reload 寫成 flags_ = incoming 而觸發重新配置，"
              << "那個 proxy 會直接懸空（UB）。" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 18 課：vectorbool 的特殊性與陷阱4.cpp" -o vb4

// === 預期輸出 ===
// === 原始範例：auto vs bool ===
// val = 0
// val2 = 0
// vb[1] = 1
//
// === 三種寫法並排對照 ===
// 改動容器前： a=1 b=1 c=1
// 改動容器後： a=0 b=1 c=1
//              ↑ 只有 auto 那個跟著變了
//
// === 對照組：vector<int> 沒有這個問題 ===
// vi[0] 改成 99 後，ai = 10（不受影響，因為 auto 從 int& 推出 int）
//
// === proxy 的正面用途：當成可寫的把手 ===
// 透過 proxy 寫入後 w = 010   <- w[1] 被改成 1 了
// （proxy 的設計目的正是讓 vb[i] = true 這種語法能運作）
//
// === dangling proxy（只說明，本檔不執行）===
//   std::vector<bool> t(10, false);
//   auto p = t[0];      // p 指向目前的 buffer
//   t.resize(100000);   // 重新配置，舊 buffer 被釋放
//   bool x = p;         // 讀取已釋放記憶體 -> undefined behavior
// UB 不保證任何特定結果，包括「看起來正常」在內，所以不可依賴。
//
// === 日常實務：feature flag 熱重載的幽靈開關 ===
// 請求開始： 錯誤快照=1 正確快照=1
// 熱重載後： 錯誤快照=0 正確快照=1
// 錯誤快照在請求進行中被翻掉了 -> 同一個請求前後走不同分支
// 正確快照維持請求開始時的值 -> 行為一致
// 註：本例的 reload 刻意逐元素就地覆寫，buffer 不變，所以只是 aliasing。
//     若 reload 寫成 flags_ = incoming 而觸發重新配置，那個 proxy 會直接懸空（UB）。
