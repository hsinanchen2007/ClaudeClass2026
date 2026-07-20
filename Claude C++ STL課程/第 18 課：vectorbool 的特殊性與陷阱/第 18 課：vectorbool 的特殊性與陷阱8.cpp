// =============================================================================
//  第 18 課-8：陷阱五 —— vector<bool> 如何悄悄弄壞你的泛型模板
// =============================================================================
//
// 【主題資訊 Information】
//   問題樣式：
//     template <typename T>
//     void process(std::vector<T>& v) {
//         T& elem = v[i];          // 對 vector<int> OK，對 vector<bool> 編譯失敗
//     }
//   根因：vector<bool>::operator[] 回傳 vector<bool>::reference（代理物件），
//         而 T&（即 bool&）無法綁定到它
//   通用解法：用 auto&&（C++11）或 decltype(auto)（C++14）取代 T&
//   標準版本：vector<bool> 特化自 C++98；auto&& 需 C++11
//   標頭檔：<vector>、<type_traits>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼這是所有陷阱裡最嚴重的一個】
//   前面幾個陷阱（取不到指標、綁不到引用、沒有 data()）
//   都是你「直接對 vector<bool> 動手」時才會遇到。
//   這一個不同：你寫的模板可能根本沒想過 bool，
//   它在 vector<int>、vector<double>、vector<MyClass> 上都跑得好好的，
//   直到某天有人寫了 process(myBoolVector)，
//   編譯器才吐出一大串指向「模板內部」的錯誤訊息。
//   錯誤點離呼叫點很遠，訊息又長又難讀——這是 C++ 模板錯誤的典型痛點。
//
// 【2. 泛型契約被打破的確切位置】
//   容器的隱含契約是「vector<T>::operator[] 回傳 T&」。
//   對所有 T 這都成立，唯獨 T = bool 時標準明文規定它回傳代理物件。
//   於是模板中任何依賴這個契約的寫法都會失效：
//     T& elem = v[i];                  // 綁定失敗
//     T* p = &v[i];                    // 型別不符
//     std::swap(v[i], v[j]);           // 需要 T&（實際上 vector<bool> 有特化版可用）
//     for (T& x : v) { ... }           // 範圍 for 取引用同樣失敗
//     someFunc(v[i]) where someFunc(T&)// 傳參失敗
//
// 【3. 通用解法：auto&& 取代 T&】
//     for (auto&& elem : v) { ... }        // 對所有 T 都成立，包含 bool
//     auto&& elem = v[i];                  // 同上
//   auto&& 是轉發引用，會依初始化運算式的實際型別推導：
//     對 vector<int>  → 推導成 int&（真正的引用）
//     對 vector<bool> → 推導成 vector<bool>::reference（代理物件本身）
//   兩種情況下賦值都會正確寫回容器，模板因此得以統一。
//   代價是可讀性稍差，而且 elem 的型別對讀者不再一目了然。
//
// 【4. 另一條路：用 if constexpr 明確分支（C++17）】
//     if constexpr (std::is_same_v<T, bool>) { ... } else { T& e = v[i]; ... }
//   當 bool 的處理邏輯本來就該不同時，這比 auto&& 更清楚。
//   注意 if constexpr 是 C++17 才有的（C++11/14 要用 tag dispatch
//   或 SFINAE 才能達到同樣效果），這個版本差異在面試中常被追問。
//
// 【5. 最務實的建議：一開始就別用 vector<bool>】
//   除非你真的在意那 8 倍的記憶體，而且確定不會把它餵給任何模板，
//   否則用 vector<uint8_t> 或 std::vector<char> 省下的麻煩，
//   遠比省下的記憶體值錢。
//   Scott Meyers 在《Effective STL》第 18 條的標題就是
//   「Avoid using vector<bool>」——理由正是本檔這一條。
//
// 【概念補充 Concept Deep Dive】
//   ▸ 為什麼 std::swap(v[i], v[j]) 反而可以用
//     標準庫特地為 vector<bool>::reference 提供了 swap 的重載
//     （[vector.bool] 明文規定 static void swap(reference, reference)），
//     所以這個特定操作被補起來了。
//     但這正說明問題的本質：每個被打破的操作都得個別補救，
//     而使用者自訂的模板不會有人替你補。
//   ▸ 為什麼 for (T& x : v) 也會失敗
//     範圍 for 展開後是 T& x = *__begin;，
//     而 vector<bool> 的迭代器解參考同樣得到代理物件，
//     一樣綁不到 bool&。改成 for (auto&& x : v) 即可。
//   ▸ 偵測「是否為 vector<bool>」的標準做法
//     用 std::is_same_v<std::decay_t<decltype(v[0])>, bool> 判斷
//     operator[] 的回傳型別是不是真的 bool，
//     比直接檢查 T 是否為 bool 更貼近問題本身
//     （因為問題出在回傳型別，不是元素型別）。
//
// 【注意事項 Pay Attention】
//   1. 模板中不要寫 T& elem = v[i];，改用 auto&& elem = v[i];。
//   2. 範圍 for 取引用也要用 auto&&，不要寫 for (T& x : v)。
//   3. 錯誤訊息會出現在模板內部，離呼叫點很遠，除錯時要往上追呼叫端。
//   4. std::swap 對 vector<bool>::reference 有標準提供的特化，可以用。
//   5. if constexpr 是 C++17；C++11/14 要用 tag dispatch 或 SFINAE。
//   6. 最省事的做法是一開始就選 vector<uint8_t>，除非記憶體真的關鍵。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector<bool> 與泛型模板
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 下面這個模板對 vector<int> 正常，對 vector<bool> 卻編不過，為什麼？
//        template <typename T> void f(std::vector<T>& v) { T& e = v[0]; }
//     答：因為 vector<bool> 是標準規定的特化，operator[] 回傳
//         vector<bool>::reference 代理物件而非 bool&。
//         T& 展開成 bool&，而非 const 的左值引用無法綁定到
//         「型別不同、且是右值」的代理物件，所以綁定失敗。
//     追問：怎麼改才能讓模板對所有 T 都成立？
//         → 把 T& e = v[0]; 改成 auto&& e = v[0];。
//           auto&& 是轉發引用，對 vector<int> 推導成 int&、
//           對 vector<bool> 推導成代理物件本身，兩者賦值都會正確寫回容器。
//
// 🔥 Q2. 為什麼說「破壞泛型」是 vector<bool> 最嚴重的問題？
//     答：其他陷阱都要你直接對 vector<bool> 動手才會遇到，
//         這一個卻會炸在「完全沒想過 bool」的既有模板上。
//         而且錯誤訊息出現在模板內部、離呼叫點很遠、又長又難讀。
//         它違反的是 C++ 最基本的期待：同一個模板對所有型別
//         應該有一致的介面語意。
//     追問：標準為什麼不把它拿掉？
//         → ABI 與原始碼相容性。Herb Sutter 曾提議移除，
//           但已有大量既存程式碼依賴它。
//           委員會後來的共識是「它應該叫 bit_vector」，
//           錯在名字讓人以為它滿足 vector 的泛型契約。
//
// ⚠️ 陷阱. 「我的模板只用 for (auto x : v) 唯讀走訪，
//          應該不受 vector<bool> 影響吧？」
//     答：唯讀走訪確實不會編譯失敗，但 x 的型別是
//         vector<bool>::reference 而不是 bool。
//         如果你在迴圈裡把 x 存進另一個容器、或讓它活過這一輪，
//         它仍然連結著原容器的那個 bit——原容器一變它就跟著變。
//         而且 decltype(x) 也不是 bool，任何依賴型別的邏輯都會出錯。
//     為什麼會錯：以為「值語意的 auto」一定會產生獨立副本。
//         auto 推導的是運算式的型別，而 v[i] 的型別本來就是代理物件；
//         auto 忠實地複製了那個代理（連同它指向的 bit），
//         而不是複製那個 bit 的值。要取獨立副本必須明確寫 bool。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

// -----------------------------------------------------------------------------
// 壞掉的版本：用 T& 取元素引用（對 vector<bool> 編譯失敗）
// -----------------------------------------------------------------------------
template <typename T>
void processBroken(std::vector<T>& v) {
    for (std::size_t i = 0; i < v.size(); ++i) {
        T& elem = v[i];   // 對 vector<int> OK；對 vector<bool> 編譯錯誤
        elem = elem;      // （示意：這裡代表任何會修改元素的處理）
    }
}

// -----------------------------------------------------------------------------
// 修好的版本：用 auto&& 接住，對所有 T（含 bool）都成立
// -----------------------------------------------------------------------------
template <typename T>
void doubleOrToggle(std::vector<T>& v) {
    for (auto&& elem : v) {        // 關鍵：auto&& 而非 T&
        if constexpr (std::is_same_v<T, bool>) {
            elem = !elem;          // bool：翻轉
        } else {
            elem = static_cast<T>(elem * 2);   // 其他：乘以 2
        }
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例】通用的欄位正規化器
//   情境：資料管線要對「任意型別的欄位陣列」做正規化——
//         數值欄位做上下界裁切，布林欄位統一成 true/false。
//         這種工具函式一定寫成模板，也一定會被人拿去餵 vector<bool>。
//   為什麼用本主題：這正是「泛型工具被 bool 打爆」的真實形態。
//         範例用 auto&& + if constexpr 同時解決兩個問題：
//         引用綁定（auto&&）與邏輯分支（if constexpr，C++17）。
// -----------------------------------------------------------------------------
template <typename T>
std::size_t normalizeColumn(std::vector<T>& column, T lo, T hi) {
    std::size_t changed = 0;
    for (auto&& cell : column) {              // 對 vector<bool> 也成立
        if constexpr (std::is_same_v<T, bool>) {
            // 布林欄位：這裡的正規化規則是「缺值(false)一律視為 false」，
            // 不需要裁切，僅統計目前為 true 的筆數
            if (cell) ++changed;
        } else {
            T before = cell;
            if (cell < lo) cell = lo;
            if (cell > hi) cell = hi;
            if (!(cell == before)) ++changed;
        }
    }
    return changed;
}

int main() {
    std::cout << std::boolalpha;

    std::cout << "=== 一、T& 版本：對 vector<int> 可用 ===" << std::endl;
    std::vector<int> vi = {1, 2, 3};
    processBroken(vi);   // OK
    std::cout << "processBroken(vector<int>) 編譯並執行成功" << std::endl;

    std::cout << "\n=== 二、T& 版本對 vector<bool> 編譯失敗（故註解）===" << std::endl;
    std::vector<bool> vb = {true, false, true};
    // processBroken(vb);   // 編譯錯誤！T& elem = v[i] 展開成 bool&，綁不到代理物件
    std::cout << "processBroken(vector<bool>) 無法編譯——這正是陷阱五" << std::endl;

    std::cout << "\n=== 三、auto&& 版本：對兩者都成立 ===" << std::endl;
    std::vector<int> a = {1, 2, 3, 4};
    doubleOrToggle(a);
    std::cout << "vector<int>  {1,2,3,4} 乘 2 → ";
    for (int x : a) std::cout << x << " ";
    std::cout << std::endl;

    std::vector<bool> b = {true, false, true};
    doubleOrToggle(b);
    std::cout << "vector<bool> {T,F,T} 翻轉 → ";
    for (std::size_t i = 0; i < b.size(); ++i) std::cout << static_cast<bool>(b[i]) << " ";
    std::cout << std::endl;

    std::cout << "\n=== 四、auto 唯讀走訪也有型別陷阱 ===" << std::endl;
    std::vector<bool> t = {true, false};
    auto  proxy = t[0];   // 型別是 vector<bool>::reference，不是 bool
    bool  copy  = t[0];   // 型別是 bool，獨立副本
    std::cout << "decltype(t[0]) 是 bool 嗎? "
              << std::is_same_v<decltype(t[0]), bool> << std::endl;
    t[0] = false;
    std::cout << "把 t[0] 改成 false 後： auto proxy = " << static_cast<bool>(proxy)
              << "（跟著變）， bool copy = " << copy << "（沒變）" << std::endl;

    std::cout << "\n=== 五、日常實務：通用欄位正規化器 ===" << std::endl;
    std::vector<int> ages = {-5, 25, 130, 40, 200};
    std::size_t c1 = normalizeColumn(ages, 0, 120);
    std::cout << "年齡欄裁切到 [0,120]，修正 " << c1 << " 筆 → ";
    for (int x : ages) std::cout << x << " ";
    std::cout << std::endl;

    std::vector<double> scores = {-0.5, 0.3, 1.8, 0.9};
    std::size_t c2 = normalizeColumn(scores, 0.0, 1.0);
    std::cout << "分數欄裁切到 [0,1]，修正 " << c2 << " 筆 → ";
    for (double x : scores) std::cout << x << " ";
    std::cout << std::endl;

    std::vector<bool> subscribed = {true, false, true, true, false};
    std::size_t c3 = normalizeColumn(subscribed, false, true);
    std::cout << "訂閱欄（vector<bool> 也能通過同一個模板），true 共 "
              << c3 << " 筆" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 18 課：vectorbool 的特殊性與陷阱8.cpp" -o vb_template
//   註：本檔使用 if constexpr 與 std::is_same_v，兩者皆為 C++17 特性，
//       因此必須以 -std=c++17（或更新）編譯。
//
// 【本檔未附 LeetCode 範例的理由】
//   本檔談的是「模板特化如何破壞泛型契約」，屬於 C++ 型別系統
//   與函式庫設計層面的議題。LeetCode 的題目都是具體型別的具體演算法，
//   不存在「同一份模板要對任意 T 成立」的需求，
//   硬套一題無法呈現本主題的價值，因此改以資料管線的通用欄位正規化器
//   這個真正會被 bool 打爆的實務範例呈現。

// === 預期輸出 ===
// === 一、T& 版本：對 vector<int> 可用 ===
// processBroken(vector<int>) 編譯並執行成功
//
// === 二、T& 版本對 vector<bool> 編譯失敗（故註解）===
// processBroken(vector<bool>) 無法編譯——這正是陷阱五
//
// === 三、auto&& 版本：對兩者都成立 ===
// vector<int>  {1,2,3,4} 乘 2 → 2 4 6 8
// vector<bool> {T,F,T} 翻轉 → false true false
//
// === 四、auto 唯讀走訪也有型別陷阱 ===
// decltype(t[0]) 是 bool 嗎? false
// 把 t[0] 改成 false 後： auto proxy = false（跟著變）， bool copy = true（沒變）
//
// === 五、日常實務：通用欄位正規化器 ===
// 年齡欄裁切到 [0,120]，修正 3 筆 → 0 25 120 40 120
// 分數欄裁切到 [0,1]，修正 2 筆 → 0 0.3 1 0.9
// 訂閱欄（vector<bool> 也能通過同一個模板），true 共 3 筆
