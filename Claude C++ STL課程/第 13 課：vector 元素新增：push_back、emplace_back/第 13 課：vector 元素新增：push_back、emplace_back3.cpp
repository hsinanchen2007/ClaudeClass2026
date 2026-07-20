// =============================================================================
//  第 13 課：vector 元素新增：push_back、emplace_back3.cpp
//    —  emplace_back 就地建構 vs push_back 臨時物件：省下的到底是什麼
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<vector>
//   template<class... Args>
//   reference emplace_back(Args&&... args);   // C++17 起回傳 reference
//   template<class... Args>
//   void      emplace_back(Args&&... args);   // C++11 / C++14 回傳 void
//   複雜度：攤銷 O(1)
//
//   emplace_back 是 variadic template：接受任意數量、任意型別的參數，
//   用 std::forward 完美轉發給 T 的建構子，在容器內部的記憶體上直接建構。
//
// 【詳細解釋 Explanation】
//
// 【1. 兩者的動作序列並排比較】
//   v.emplace_back(1);
//       ① 在 vector 尾端那塊未初始化記憶體上直接 placement new 建構 Tracker(1)
//       完畢。輸出只有一行「建構」。
//
//   v.push_back(Tracker(2));
//       ① 在呼叫端的堆疊上建構臨時 Tracker(2)
//       ② 臨時物件是 rvalue → 移動建構到 vector 內部
//       ③ 完整運算式結束，臨時物件解構
//       輸出三行：建構、移動建構、銷毀。
//
//   差距是「一次移動建構 + 一次解構」。對本例只有一個 int 成員的 Tracker
//   來說微不足道；但若 T 持有 heap 資源、或移動建構子未標 noexcept
//   而退化成深複製，這個差距就會被放大。
//
// 【2. 省的是「臨時物件」，不是「複製」】
// 這是最關鍵、也最常被誤解的一點。emplace_back 之所以快，是因為
// **完全不需要那個中介的臨時物件**，而不是因為它有什麼魔法能避免複製。
// 如果你傳給 emplace_back 的本來就是一個現成的物件：
//     Tracker existing(9);
//     v.emplace_back(existing);   // existing 是 lvalue
// std::forward 會把 lvalue 原封不動轉發出去，最後呼叫的是**複製建構子**，
// 跟 push_back(existing) 產生的機器碼完全一樣。
// 所以「emplace_back 永遠比較快」是錯的，它只在「原本要造臨時物件」時才贏。
//
// 【3. 為什麼參數型別不一樣，用法就不一樣】
//   push_back 的參數是 T           → 你必須交出一個「成品」
//   emplace_back 的參數是 Args&&... → 你交出的是「零件」，由容器組裝
// 因此：
//     v.push_back(Tracker(2));   // 交成品：得先自己造一個
//     v.emplace_back(2);         // 交零件：把 2 交出去，容器自己造
// 這也解釋了為何 emplace_back 能接受多個參數（見本課第 4 個範例檔的 Person）：
// 有幾個建構子參數就傳幾個，反正都會被轉發過去。
//
// 【概念補充 Concept Deep Dive】
// emplace_back 內部大致等價於：
//     if (size_ == cap_) reallocate(grow());
//     ::new (data_ + size_) T(std::forward<Args>(args)...);   // placement new
//     ++size_;
//     return data_[size_ - 1];      // C++17 起才有回傳值
//
// 這裡的 placement new 是關鍵：vector 的 capacity 是「已配置但尚未建構物件」
// 的原始記憶體。reserve(3) 配置了容納 3 個 Tracker 的位元組，但那片區域上
// **一個 Tracker 都還不存在**（size() 仍是 0）。
// emplace_back 就是在那片原始記憶體的正確位置上，呼叫建構子把物件「長出來」。
//
// 這也解釋了 size() 與 capacity() 的本質差異：
//     size()     = 已經建構好、真實存在的物件個數
//     capacity() = 配置好的記憶體能容納幾個物件（大部分還是空的）
// 兩者不是「用了多少 / 總共多少」這麼表面的關係，而是「建構了幾個 /
// 有空間建構幾個」。
//
// 【注意事項 Pay Attention】
// 1. emplace_back 只在「省掉臨時物件」時才比 push_back 快；
//    傳現成物件時兩者完全等價。
// 2. emplace_back 用 direct-init，可呼叫 explicit 建構子、
//    **不做 narrowing 檢查**，參數打錯可能悄悄編過。
// 3. emplace_back 的回傳值 C++17 起才是 reference，C++11/14 是 void。
// 4. 本檔輸出中「銷毀 Tracker(2)」出現在 push_back 之後，
//    是臨時物件的解構，不是 vector 元素被銷毀。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】emplace_back 就地建構
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. emplace_back 相較 push_back 具體省下了什麼？
//     答：省下「臨時物件的建構 + 移動（或複製）進容器 + 臨時物件解構」。
//         emplace_back 用 placement new 直接在容器尾端的未初始化記憶體上
//         呼叫 T 的建構子，全程沒有中介物件。
//     追問：如果 T 是 int 之類的內建型別，還有差嗎？
//         → 幾乎沒有。移動一個 int 就是複製一個 int，編譯器最佳化後
//           兩者生成的機器碼通常完全相同。
//
// 🔥 Q2. emplace_back 為什麼能接受任意數量的參數？
//     答：它是 variadic template（template<class... Args>），
//         用 std::forward<Args>(args)... 把整包參數完美轉發給 T 的建構子。
//         T 的建構子需要幾個參數就傳幾個，型別也由轉發自動保持。
//     追問：什麼是「完美轉發」的「完美」？
//         → 指的是連「值類別」都原封不動地保留：傳進來是 lvalue 就轉發成
//           lvalue，是 rvalue 就轉發成 rvalue，不會被中途退化。
//
// ⚠️ 陷阱 Q3. 既然 emplace_back 比較快，是不是應該把所有 push_back 都換掉？
//     答：不應該。當傳入的已經是現成的同型別物件時，兩者產生的動作完全一樣
//         （lvalue 都是複製，std::move 過的都是移動），沒有任何效能差異。
//         而且 emplace_back 用 direct-init，會繞過 explicit 的保護、
//         也不做窄化檢查，型別打錯反而更容易悄悄編過。
//         「已有物件」用 push_back 意圖更清楚，也更安全。
//     為什麼會錯：把 emplace_back 理解成「push_back 的最佳化版」。
//         實際上它們的參數語意根本不同：一個收「成品」，一個收「零件」。
//         用途不同，不是誰取代誰。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>

class Tracker {
public:
    int id;

    Tracker(int i) : id(i) {
        std::cout << "建構 Tracker(" << id << ")" << std::endl;
    }

    Tracker(const Tracker& other) : id(other.id) {
        std::cout << "複製建構 Tracker(" << id << ")" << std::endl;
    }

    Tracker(Tracker&& other) noexcept : id(other.id) {
        std::cout << "移動建構 Tracker(" << id << ")" << std::endl;
    }

    ~Tracker() {
        std::cout << "銷毀 Tracker(" << id << ")" << std::endl;
    }
};

int main() {
    std::vector<Tracker> v;
    v.reserve(4);  // 撐開 capacity，排除擴容搬移的干擾

    std::cout << "=== emplace_back（就地建構）===" << std::endl;
    v.emplace_back(1);  // 只有一次建構：placement new 直接造在 vector 裡

    std::cout << "\n=== 對比 push_back（經過臨時物件）===" << std::endl;
    v.push_back(Tracker(2));  // 建構臨時 + 移動進容器 + 臨時物件解構

    // 關鍵對照：傳「現成物件」時，emplace_back 並沒有比較快
    std::cout << "\n=== 傳現成物件：兩者完全等價 ===" << std::endl;
    Tracker existing(9);
    std::cout << "push_back(existing):" << std::endl;
    v.push_back(existing);      // lvalue → 複製建構
    std::cout << "emplace_back(existing):" << std::endl;
    v.emplace_back(existing);   // lvalue 被完美轉發 → 一樣是複製建構！

    std::cout << "\n=== 程式結束（以下是解構）===" << std::endl;
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 13 課：vector 元素新增：push_back、emplace_back3.cpp" -o emplace_vs_push

// === 預期輸出 ===
// === emplace_back（就地建構）===
// 建構 Tracker(1)
// 
// === 對比 push_back（經過臨時物件）===
// 建構 Tracker(2)
// 移動建構 Tracker(2)
// 銷毀 Tracker(2)
// 
// === 傳現成物件：兩者完全等價 ===
// 建構 Tracker(9)
// push_back(existing):
// 複製建構 Tracker(9)
// emplace_back(existing):
// 複製建構 Tracker(9)
// 
// === 程式結束（以下是解構）===
// 銷毀 Tracker(9)
// 銷毀 Tracker(1)
// 銷毀 Tracker(2)
// 銷毀 Tracker(9)
// 銷毀 Tracker(9)
