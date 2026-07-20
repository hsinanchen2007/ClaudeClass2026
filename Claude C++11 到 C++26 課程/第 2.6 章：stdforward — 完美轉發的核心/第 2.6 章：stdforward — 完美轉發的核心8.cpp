// =============================================================================
//  第 2.6 章 範例 8  —  auto&&：變數宣告處的轉發參考
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<type_traits>（本檔用來「印出」推導結果）、<utility>
//   語法：auto&& name = expr;
//   auto&& 的推導規則與 template<class T> f(T&&) 完全相同（都是轉發參考），
//   因此同一套引用折疊規則適用。C++11 起可用。
//
// 【詳細解釋 Explanation】
//
// 【1. auto&& 為什麼是轉發參考而不是右值參考】
//   auto 的推導規則被標準定義成「與模板參數推導相同」。所以
//       auto&& r = expr;
//   等價於把 expr 傳給 template<class T> void f(T&& r)。於是：
//       expr 是左值 → auto 推導為 X&  → X& && 折疊成 X&   → r 綁到原物件
//       expr 是右值 → auto 推導為 X   → X&&              → r 延長臨時物件壽命
//   一個 auto&& 就同時吃得下左值與右值，這是 const X& 之外唯一做得到的寫法，
//   而且它比 const X& 多了「可以修改」與「保留右值身分」兩項能力。
//
// 【2. 最實用的場景：range-based for 的元素宣告】
//   for (auto&& elem : range)
//   這是唯一「對所有 range 都正確」的寫法：
//     * range 的元素是左值（vector<string>）→ elem 是 string&，不複製
//     * range 產生的是臨時值（例如 views 或回傳 proxy 的容器）→ elem 是右值參考
//     * vector<bool> 的 operator[] 回傳 proxy 物件（不是真的 bool&），
//       只有 auto&& 接得住而且語意正確；寫 auto& 會直接編譯失敗。
//   相對地：
//     for (auto elem : v)        每個元素複製一份
//     for (const auto& elem : v) 不複製但唯讀，且無法轉發
//     for (auto& elem : v)       可改，但接不住臨時值與 proxy
//
// 【3. auto&& 綁到臨時物件時會延長壽命】
//   auto&& r = std::string("tmp");
//   這個臨時字串的生命期被延長到 r 的作用域結束為止，不是語句結束。
//   規則與 const X& 的壽命延長相同，但 auto&& 綁出來的是「可修改」的參考。
//   注意：壽命延長只對「直接綁定的臨時物件」有效；若右邊是函式回傳的參考，
//   就沒有任何延長效果（回傳的是別人的物件）。
//
// 【4. 要轉發 auto&& 變數時，forward 的引數怎麼寫】
//   auto&& 沒有一個叫 T 的模板參數可用，正確寫法是用 decltype：
//       std::forward<decltype(r)>(r)
//   這也是 C++20 縮寫函式模板 void f(auto&& x) 內部該用的寫法。
//
// 【概念補充 Concept Deep Dive】
//   (A) 怎麼「看見」推導結果？型別本身不能直接印，本檔改用 <type_traits> 的
//       std::is_lvalue_reference_v / is_rvalue_reference_v 對 decltype(ref) 提問，
//       把答案印成布林值。這是編譯期資訊，執行期輸出必然完全穩定。
//   (B) decltype(變數名) 與 decltype((變數名)) 不同：前者給宣告型別，
//       後者多一層括號會給「運算式的型別」，對具名變數會得到左值參考。
//       本檔用的是前者。
//   (C) auto&& 不會做 decay：陣列不會退化成指標、函式不會退化成函式指標，
//       這點與 auto（值）不同。
//
// 【注意事項 Pay Attention】
//   1. auto&& 綁到函式回傳的參考時「不會」延長壽命——回傳懸空參考照樣懸空。
//   2. 具名的 auto&& 變數本身是左值（同 T&& 參數），要轉發必須寫
//      std::forward<decltype(r)>(r)。
//   3. 對 vector<bool> 用 for (auto& b : v) 會編譯失敗，因為 operator[]
//      回傳的是 proxy 臨時物件；要用 auto&&。
//   4. 別只因為「看起來比較泛用」就到處寫 auto&&；純唯讀走訪時
//      const auto& 意圖更清楚。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】auto&& 與轉發參考
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. auto&& 是右值參考嗎？
//     答：不是。auto 的推導規則與模板參數相同，所以 auto&& 是「轉發參考」。
//         綁左值時折疊成左值參考，綁右值時才是右值參考。
//         真正的右值參考要寫具體型別，例如 std::string&& r = ...。
//     追問：怎麼在程式裡證明？
//         → 對 decltype(r) 問 std::is_lvalue_reference_v，本檔即如此驗證。
//
// 🔥 Q2. range-based for 為什麼推薦寫 for (auto&& elem : range)？
//     答：它是唯一對所有 range 都成立的寫法。一般容器時 elem 是左值參考、
//         不複製；range 產生臨時值或 proxy（如 vector<bool>）時也接得住。
//         寫 auto 會逐一複製，寫 auto& 則接不住臨時值與 proxy。
//     追問：那什麼時候該寫 const auto&？
//         → 確定只讀不改、也不需要轉發時，const auto& 意圖更明確。
//
// ⚠️ 陷阱. std::vector<bool> v{true,false}; for (auto& b : v) 為什麼編譯失敗？
//     答：vector<bool> 是位元壓縮的特化，operator[] 回傳的是 proxy 物件
//         （std::_Bit_reference）的「臨時值」，不是 bool&。
//         非 const 的左值參考不能綁定臨時物件，因此編譯錯誤。
//         改成 auto&& 即可，它會折疊成對該 proxy 的右值參考。
//     為什麼會錯：大家預期「vector<T> 的元素就是 T」，
//         但 vector<bool> 是標準裡著名的例外——它不是容器語意的模範生，
//         元素根本沒有獨立位址可取。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <type_traits>

// 把「推導結果是左值參考還是右值參考」印出來（純編譯期資訊，輸出完全穩定）
template<typename T>
const char* refKind() {
    return std::is_lvalue_reference<T>::value ? "左值參考 (X&)"
         : std::is_rvalue_reference<T>::value ? "右值參考 (X&&)"
         : "非參考 (X)";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】走訪設定表並就地正規化（去除前後空白）
//   情境：讀進來的設定值常帶有多餘空白。要「就地修改」容器元素，
//         就不能用 const auto&（唯讀），也不該用 auto（改到的是副本，白改）。
//         auto&& 在這裡同時滿足「可修改」與「不複製」。
// -----------------------------------------------------------------------------
void trimInPlace(std::string& s) {
    const char* ws = " \t";
    auto b = s.find_first_not_of(ws);
    if (b == std::string::npos) { s.clear(); return; }
    auto e = s.find_last_not_of(ws);
    s = s.substr(b, e - b + 1);
}

int main() {
    std::string s = "Hello";

    std::cout << "=== 1. auto&& 綁左值 vs 綁右值 ===\n";
    auto&& ref1 = s;                    // s 是左值 → auto = string& → ref1 是 string&
    auto&& ref2 = std::string("tmp");   // 右值 → auto = string → ref2 是 string&&

    std::cout << "  auto&& ref1 = s;                  → " << refKind<decltype(ref1)>() << "\n";
    std::cout << "  auto&& ref2 = std::string(\"tmp\"); → " << refKind<decltype(ref2)>() << "\n";

    // ref1 綁到 s 本身：改 ref1 就是改 s（證明不是副本）
    ref1 += ", World";
    std::cout << "  改了 ref1 之後 s = \"" << s << "\"（證明 ref1 綁的是本體）\n";

    // ref2 綁到臨時物件：壽命被延長到 ref2 的作用域結束，而且可修改
    ref2 += "!";
    std::cout << "  ref2 = \"" << ref2 << "\"（臨時物件壽命已被延長，且可修改）\n";

    std::cout << "\n=== 2. range-based for 中的 auto&& ===\n";
    std::vector<std::string> vec = {"a", "b", "c"};
    for (auto&& elem : vec) {
        // vec 的元素是左值 → elem 推導為 string&，不複製
        std::cout << "  elem = \"" << elem << "\"，型別為 "
                  << refKind<decltype(elem)>() << "\n";
    }

    std::cout << "\n=== 3. auto&& 接得住 vector<bool> 的 proxy ===\n";
    {
        std::vector<bool> flags = {true, false, true};
        // 注意：這裡寫 for (auto& b : flags) 會編譯失敗，因為 operator[]
        //       回傳的是臨時的 proxy 物件，不是 bool&。
        int trueCount = 0;
        for (auto&& b : flags) {
            if (b) ++trueCount;
        }
        std::cout << "  vector<bool> 中為 true 的個數 = " << trueCount << "\n";
        std::cout << "  （auto& 在此會編譯失敗，auto&& 才接得住 proxy）\n";
    }

    std::cout << "\n=== 日常實務：就地正規化設定值 ===\n";
    {
        std::vector<std::string> values = {"  8080 ", "\tlocalhost", "debug  "};
        std::cout << "  正規化前:\n";
        for (const auto& v : values) std::cout << "    [" << v << "]\n";

        for (auto&& v : values) {   // 需要就地修改 → 不能用 const auto&
            trimInPlace(v);
        }

        std::cout << "  正規化後:\n";
        for (const auto& v : values) std::cout << "    [" << v << "]\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.6 章：stdforward — 完美轉發的核心8.cpp" -o auto_rref

// 註：本檔未附 LeetCode 範例。auto&& 是語言層的宣告技巧，
//     在題目解法中通常只是 for 迴圈的一個寫法選擇，構不成一題的核心，
//     硬掛一題反而模糊焦點。

// === 預期輸出 ===
// === 1. auto&& 綁左值 vs 綁右值 ===
//   auto&& ref1 = s;                  → 左值參考 (X&)
//   auto&& ref2 = std::string("tmp"); → 右值參考 (X&&)
//   改了 ref1 之後 s = "Hello, World"（證明 ref1 綁的是本體）
//   ref2 = "tmp!"（臨時物件壽命已被延長，且可修改）
//
// === 2. range-based for 中的 auto&& ===
//   elem = "a"，型別為 左值參考 (X&)
//   elem = "b"，型別為 左值參考 (X&)
//   elem = "c"，型別為 左值參考 (X&)
//
// === 3. auto&& 接得住 vector<bool> 的 proxy ===
//   vector<bool> 中為 true 的個數 = 2
//   （auto& 在此會編譯失敗，auto&& 才接得住 proxy）
//
// === 日常實務：就地正規化設定值 ===
//   正規化前:
//     [  8080 ]
//     [	localhost]
//     [debug  ]
//   正規化後:
//     [8080]
//     [localhost]
//     [debug]
