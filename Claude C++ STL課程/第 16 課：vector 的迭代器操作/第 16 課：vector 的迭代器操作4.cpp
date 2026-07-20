// =============================================================================
//  第 16 課：vector 的迭代器操作 4  —  cbegin() / cend() 與 const_iterator
// =============================================================================
//
// 【主題資訊 Information】
//   const_iterator cbegin() const noexcept;   // 唯讀，指向第一個元素
//   const_iterator cend()   const noexcept;   // 唯讀哨兵
//   標頭檔：<vector>
//   標準版本：cbegin() / cend() 是 C++11（本機以 -std=c++03 -pedantic-errors
//             實測會報 "has no member named 'cbegin'"，確認確為 C++11 新增）
//   複雜度：O(1)
//   關鍵型別關係：iterator → const_iterator 可隱式轉換；反向不行。
//
// 【詳細解釋 Explanation】
//
// 【1. const_iterator 到底把什麼變成 const】
//   這是最容易誤解的一點。const_iterator 限制的是「所指向的元素」不可被修改，
//   迭代器「自己」仍然可以移動。兩者是完全不同的東西：
//       std::vector<int>::const_iterator it;   // 元素唯讀，it 可以 ++
//       const std::vector<int>::iterator it;   // it 不可 ++，但 *it = 5 合法！
//   對照指標更清楚：
//       const int* p;     ↔  const_iterator          （指向的東西唯讀）
//       int* const p;     ↔  const iterator          （指標本身唯讀）
//   實務上你要的幾乎永遠是前者。
//
// 【2. 既然 const vector 的 begin() 已經回傳 const_iterator，為何還要 cbegin()】
//   因為 auto。begin() 有兩個多載，回傳型別取決於「容器是不是 const」：
//       std::vector<int> v;              // 非 const
//       auto it1 = v.begin();            // 推導成 iterator       ← 可寫入！
//       auto it2 = v.cbegin();           // 推導成 const_iterator ← 明確唯讀
//   在 C++11 引入 auto 之前，你可以靠明寫型別 const_iterator 來表達意圖；
//   有了 auto 之後，型別被藏起來了，於是需要一個「在非 const 容器上也一定
//   回傳唯讀迭代器」的函式 —— 這就是 cbegin() 存在的唯一理由。
//   它讓「我只打算讀」這件事寫在程式碼裡，而不是寫在註解裡。
//
// 【3. 轉換規則：單向道】
//   iterator → const_iterator：可以隱式轉換（加上 const 是安全的收緊）。
//       std::vector<int>::const_iterator ci = v.begin();   // OK
//   const_iterator → iterator：不行，這等於偷偷拿掉 const。
//       std::vector<int>::iterator it = v.cbegin();        // 編譯錯誤（本機實測）
//   真的需要換回來時，標準做法是用「距離」重算，而不是 const_cast：
//       auto it = v.begin() + (ci - v.cbegin());
//   這個寫法只對隨機存取迭代器成立；對 list 要用 std::next(v.begin(), n)。
//
// 【4. C++11 的一個相關修正：erase/insert 改吃 const_iterator】
//   C++98 的 erase(iterator) 只接受可寫迭代器，於是「我只是要指出位置，
//   卻被迫拿一個可寫的迭代器」很不合理。C++11 起簽名改為
//   erase(const_iterator)、insert(const_iterator, ...)，
//   所以現在可以直接把 cbegin() 得到的位置交給 erase（本機實測可編譯）。
//   注意：容器本身仍必須是非 const，const_iterator 只是「位置」的型別。
//
// 【概念補充 Concept Deep Dive】
//   libstdc++ 中兩者其實是同一個 template 的不同特化：
//       iterator       = __normal_iterator<int*,       vector<int>>
//       const_iterator = __normal_iterator<const int*, vector<int>>
//   隱式轉換是靠 __normal_iterator 提供的一個 converting constructor 達成的，
//   而它只在「從 iterator 轉到 const_iterator」的方向上啟用（用 SFINAE 限制）。
//   因為兩者底層都只是一個指標，這層 const 完全是編譯期的事，
//   執行期沒有任何額外成本 —— const 正確性在 C++ 裡是免費的。
//
// 【注意事項 Pay Attention】
//   1. const_iterator ≠ const iterator，兩者意思相反，別寫錯。
//   2. 不要用 const_cast 把 const_iterator 硬轉成 iterator；
//      若原物件真的是 const，透過它修改是 UB。用距離重算才安全。
//   3. cbegin() 不能用來讓一個 const vector 變得可寫，它只是表達唯讀意圖。
//   4. const_iterator 一樣會因為容器重新配置而失效 —— const 保護的是元素值，
//      不是迭代器的有效性。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】const_iterator
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. const vector 的 begin() 已經回傳 const_iterator 了，為什麼還需要 cbegin()？
//     答：為了搭配 auto。對非 const 的 vector，auto it = v.begin() 會推導成
//         可寫的 iterator；只有 v.cbegin() 才保證推導成 const_iterator。
//         cbegin() 讓「唯讀」這個意圖出現在程式碼裡，而不是只存在於註解。
//     追問：那 C++11 之前的人怎麼辦？
//         → 明寫型別 std::vector<int>::const_iterator it = v.begin();
//           靠 begin() 的隱式轉換得到唯讀迭代器。
//
// 🔥 Q2. const_iterator 和 const iterator 差在哪？
//     答：const_iterator 是「元素唯讀、迭代器可移動」（類比 const int*）；
//         const iterator 是「迭代器不可移動、元素卻可以改」（類比 int* const）。
//         後者幾乎從來不是你想要的，而且 ++it 會直接編譯失敗。
//
// ⚠️ 陷阱. 可以把 const_iterator 轉回 iterator 嗎？
//     答：不能隱式轉換（本機實測會編譯錯誤：conversion from
//         __normal_iterator<const int*, ...> to non-scalar type ... requested）。
//         正確做法是用距離重算：auto it = v.begin() + (ci - v.cbegin());
//         絕對不要用 const_cast 硬轉 —— 若底層物件本身是 const，
//         透過轉出來的指標寫入是 UB。
//     為什麼會錯：多數人把 const 想成「一個可以隨手脫掉的標籤」，
//         但它是型別系統的承諾；能不能安全脫掉取決於「原始物件是不是 const」，
//         而那件事編譯器在轉型當下無從得知，所以標準乾脆不提供這條路。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>

void demo_const_iterator() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // cbegin() / cend() 回傳 const_iterator，只能讀取，不能修改元素
    for (std::vector<int>::const_iterator it = v.cbegin(); it != v.cend(); ++it) {
        std::cout << *it << " ";
        // *it = 100;  // 編譯錯誤！不可透過 const_iterator 修改元素
    }
    std::cout << std::endl;

    // auto 的差別：begin() 推導成 iterator，cbegin() 推導成 const_iterator
    auto writable = v.begin();
    *writable = 11;                    // 合法
    auto readonly = v.cbegin();
    // *readonly = 11;                 // 編譯錯誤
    std::cout << "auto + begin() 可寫入，v[0] 現在 = " << *readonly << std::endl;

    // iterator → const_iterator 隱式轉換（單向道）
    std::vector<int>::const_iterator ci = v.begin();
    std::cout << "iterator 隱式轉成 const_iterator，*ci = " << *ci << std::endl;

    // const_iterator → iterator 要用距離重算，不可 const_cast
    auto ci3 = v.cbegin() + 3;
    auto back_to_it = v.begin() + (ci3 - v.cbegin());
    *back_to_it = 44;
    std::cout << "用距離換回可寫迭代器後 v[3] = " << v[3] << std::endl;

    // C++11 起 erase 接受 const_iterator（位置唯讀，但容器本身仍需非 const）
    v.erase(v.cbegin());
    std::cout << "erase(cbegin()) 之後：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】感測器樣本的唯讀統計（函式承諾絕不修改輸入）
//   情境：監控模組要對「已收集完成」的一批感測器樣本算出最大值、最小值與平均，
//         這個函式屬於分析端，絕對不該動到原始資料。
//   為什麼用到本主題：參數用 const std::vector<double>&，函式內只能拿到
//         const_iterator；即使有人日後在函式裡不小心寫了 *it = 0，
//         也會在編譯期就被擋下來，而不是在正式環境才發現資料被汙染。
// -----------------------------------------------------------------------------
struct SampleStats {
    double min_v;
    double max_v;
    double avg;
    std::size_t count;
};

SampleStats analyze(const std::vector<double>& samples) {
    SampleStats st{0.0, 0.0, 0.0, 0};
    if (samples.empty()) return st;

    // 對 const 容器呼叫 begin()，型別就是 const_iterator（等同 cbegin()）
    auto it = samples.cbegin();
    st.min_v = st.max_v = *it;
    double sum = 0.0;

    for (; it != samples.cend(); ++it) {
        // *it = 0.0;   // ← 若手滑寫了這行，編譯期就會失敗，資料不可能被汙染
        if (*it < st.min_v) st.min_v = *it;
        if (*it > st.max_v) st.max_v = *it;
        sum += *it;
    }
    st.count = samples.size();
    st.avg = sum / static_cast<double>(st.count);
    return st;
}

int main() {
    std::cout << "=== const_iterator 基本行為 ===" << std::endl;
    demo_const_iterator();

    std::cout << "\n=== 日常實務：感測器樣本唯讀統計 ===" << std::endl;
    std::vector<double> samples = {21.5, 22.0, 21.8, 23.1, 22.4, 20.9};
    SampleStats st = analyze(samples);
    std::cout << "count = " << st.count
              << "  min = " << st.min_v
              << "  max = " << st.max_v
              << "  avg = " << st.avg << std::endl;
    std::cout << "原始資料未被修改，第一筆仍為 " << samples.front() << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 16 課：vector 的迭代器操作4.cpp" -o iter4

// === 預期輸出 ===
// === const_iterator 基本行為 ===
// 10 20 30 40 50 
// auto + begin() 可寫入，v[0] 現在 = 11
// iterator 隱式轉成 const_iterator，*ci = 11
// 用距離換回可寫迭代器後 v[3] = 44
// erase(cbegin()) 之後：20 30 44 50 
//
// === 日常實務：感測器樣本唯讀統計 ===
// count = 6  min = 20.9  max = 23.1  avg = 21.95
// 原始資料未被修改，第一筆仍為 21.5
