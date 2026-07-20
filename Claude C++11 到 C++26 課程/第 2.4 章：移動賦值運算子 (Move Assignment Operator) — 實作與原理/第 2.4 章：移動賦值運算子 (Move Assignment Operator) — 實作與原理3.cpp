// =============================================================================
// 主題: 移動賦值的五種觸發時機
// =============================================================================
//
// 【主題資訊 Information】
//   五種時機：① 賦值左值（走複製）② std::move ③ 臨時物件
//             ④ 函式回傳值      ⑤ std::swap（內部使用移動賦值）
//   標準版本：移動賦值 / 右值參考 / noexcept   C++11
//   標頭檔  ：<utility>（std::move、std::swap）、<vector>、<algorithm>
//   複雜度  ：移動賦值 O(1)；複製賦值 O(N)
//   本檔宣告的標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 判準只有一個：賦值運算子右邊是不是右值】
//   五種時機看起來各異，本質完全相同 ——
//   重載決議會依「= 右邊那個表達式的 value category」選擇：
//       左值 → operator=(const T&)   （複製賦值）
//       右值 → operator=(T&&)        （移動賦值）
//   ① 是左值所以走複製；②③④⑤ 都產生右值，所以走移動。
//
// 【2. 時機 ⑤ 最容易被忽略：std::swap 內部就是移動】
//   std::swap 的預設實作是：
//       T tmp = std::move(a);   // 一次移動建構
//       a     = std::move(b);   // 一次移動賦值
//       b     = std::move(tmp); // 一次移動賦值
//   所以一個型別的 swap 效能，完全取決於它的移動操作寫得好不好。
//   若移動賦值沒寫（或沒標 noexcept），swap 就會退化成三次深拷貝。
//   這也是為什麼排序等演算法對「有沒有正確的移動操作」非常敏感。
//
// 【3. 為什麼要先 reserve】
//   本檔在 push_back 前先 reserve(10)。
//   若不 reserve，擴容時 vector 會把既有元素搬到新緩衝區，
//   產生額外的移動輸出，讓「時機」的對照變得混亂。
//   reserve 讓示範聚焦在我們想觀察的那幾次賦值上。
//
// 【4. 本檔的標籤加工只為觀察用】
//   Tracker 的賦值運算子刻意在標籤後面加上 "(copy=)" 或 "(move=)"，
//   並把被移動的來源設成 "(empty)"，目的是讓輸出能一眼看出走了哪條路。
//   真實程式碼的賦值運算子不應該做這種額外加工。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼時機 ③④ 一定走移動，而不會被複製省略吃掉
//     複製省略（RVO/保證省略）只發生在「初始化」的情境：
//         Tracker c = make();     // 初始化 → 可能完全省略
//     但賦值不同：
//         a = make();             // a 已經存在 → 必須真的執行一次賦值
//     所以這裡一定看得到 [移動賦值] 的輸出，不會被省略掉。
//     這是「初始化」與「賦值」的重要差別。
//
// (B) 被移動後的來源狀態由你決定
//     本檔的移動賦值把來源設成 "(empty)"。
//     對「你自己寫的類別」，移動後的狀態完全由你的實作決定 ——
//     你的責任是讓它處於可安全解構、可重新賦值的狀態。
//     （對標準庫型別則是「有效但未指定」，不可假設具體內容。）
//
// (C) 連鎖賦值為何需要回傳 *this
//     a = b = c; 會先算 b = c（回傳 b 的參考），再把結果賦給 a。
//     若 operator= 回傳 void 就無法連鎖；若回傳值則會多一次複製。
//     回傳 ClassName& 是與內建型別一致的正確做法。
//
// 【注意事項 Pay Attention】
//   1. 賦值走複製還是移動，只取決於 = 右邊表達式的 value category。
//   2. std::swap 內部使用移動建構與移動賦值 —— 移動寫不好，swap 就慢。
//   3. 賦值不像初始化，不會被複製省略吃掉；③④ 一定會真的執行一次移動賦值。
//   4. push_back 前先 reserve，避免擴容產生額外移動干擾觀察。
//   5. 本檔在賦值運算子中加工標籤只為教學觀察，真實程式碼不應如此。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】移動賦值的觸發時機
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::swap(a, b) 內部發生了什麼？
//     答：預設實作是一次移動建構加兩次移動賦值：
//         T tmp = std::move(a); a = std::move(b); b = std::move(tmp);
//         所以 swap 的效能完全取決於該型別的移動操作。
//     追問：若型別沒有移動操作會怎樣？→ 三個 std::move 都會退回複製，
//         swap 變成三次深拷貝 —— 這會讓 sort 等演算法明顯變慢。
//
// 🔥 Q2. a = make(); 會不會像 T a = make(); 一樣被複製省略掉？
//     答：不會。複製省略只適用於「初始化」的情境。
//         a = make(); 中 a 早已存在，必須真的執行一次賦值運算子；
//         回傳的臨時物件是右值，所以走移動賦值。
//     追問：那要怎麼避免這次賦值？→ 若語意允許，
//         改寫成初始化 T a = make(); 就能享受複製省略（成本為零）。
//
// ⚠️ 陷阱. a = b; 其中 b 之後就不再使用了，編譯器會自動改用移動嗎？
//     答：不會。b 是具名變數，作為表達式是左值，重載決議一律選複製賦值。
//         編譯器不會替你分析「b 之後還會不會被用到」。
//         要移動必須自己寫 a = std::move(b);。
//     為什麼會錯：以為編譯器會做「最後一次使用」的自動分析。
//         C++ 的設計是把這個決定權留給程式設計者 ——
//         因為只有你知道這個物件之後還需不需要。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>

class Tracker {
    std::string label_;
public:
    Tracker(const char* l) : label_(l) {}
    Tracker(const Tracker& o) : label_(o.label_) {}
    Tracker(Tracker&& o) noexcept : label_(std::move(o.label_)) { o.label_ = "(empty)"; }

    Tracker& operator=(const Tracker& o) {
        label_ = o.label_; label_ += "(copy=)";
        std::cout << "  [複製賦值] " << label_ << "\n";
        return *this;
    }
    Tracker& operator=(Tracker&& o) noexcept {
        label_ = std::move(o.label_); label_ += "(move=)";
        o.label_ = "(empty)";
        std::cout << "  [移動賦值] " << label_ << "\n";
        return *this;
    }

    const std::string& label() const { return label_; }
};

Tracker make() { return Tracker("factory"); }

int main() {
    Tracker a("A"), b("B"), c("C");

    std::cout << "--- 時機 1：直接賦值左值 ---\n";
    a = b;                    // 複製賦值

    std::cout << "\n--- 時機 2：std::move ---\n";
    a = std::move(c);         // 移動賦值

    std::cout << "\n--- 時機 3：賦值臨時物件 ---\n";
    a = Tracker("temp");      // 移動賦值（臨時物件是右值）

    std::cout << "\n--- 時機 4：賦值函式回傳值 ---\n";
    a = make();               // 移動賦值（回傳值是右值）

    std::cout << "\n--- 時機 5：容器中的搬移 ---\n";
    std::vector<Tracker> vec;
    vec.reserve(10);
    vec.push_back(Tracker("V1"));
    vec.push_back(Tracker("V2"));

    // swap 內部使用移動賦值
    std::cout << "swap:\n";
    std::swap(vec[0], vec[1]);

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.4 章：移動賦值運算子 (Move Assignment Operator) — 實作與原理3.cpp" -o ma_demo3

// === 預期輸出 ===
// --- 時機 1：直接賦值左值 ---
//   [複製賦值] B(copy=)
//
// --- 時機 2：std::move ---
//   [移動賦值] C(move=)
//
// --- 時機 3：賦值臨時物件 ---
//   [移動賦值] temp(move=)
//
// --- 時機 4：賦值函式回傳值 ---
//   [移動賦值] factory(move=)
//
// --- 時機 5：容器中的搬移 ---
// swap:
//   [移動賦值] V2(move=)
//   [移動賦值] V1(move=)
