// =============================================================================
//  第四課：迭代器（Iterator）的核心概念 4  —  auto 與迭代器型別名稱
// =============================================================================
//
// 【主題資訊 Information】
//   語法：
//     std::vector<int>::iterator it = vec.begin();          // C++98 完整寫法
//     auto it = vec.begin();                                 // C++11 型別推導
//     std::map<std::string,int>::const_iterator cit = m.cbegin();
//   標準版本：auto 作為型別推導是 C++11（C++98 的 auto 是「自動儲存期」
//             這個幾乎沒人寫的儲存類別指定子，C++11 把它重新賦予意義）。
//             泛型 lambda 參數用 auto 是 C++14；函式參數用 auto（abbreviated
//             function template）是 C++20。
//   複雜度：auto 純粹是編譯期推導，執行期零成本。
//   標頭檔：無額外需求。
//
// 【詳細解釋 Explanation】
//
// 【1. auto 解決的不只是「打字太長」】
//   把 `std::map<std::string, std::vector<int>>::const_iterator` 換成 `auto`
//   當然省字，但真正的價值在三個地方：
//
//   (a) 避免寫錯型別而觸發隱式轉換。經典例子：
//         std::map<std::string, int> m;
//         for (const std::pair<std::string, int>& p : m) { ... }
//       看起來對，其實 map 的 value_type 是 `std::pair<const std::string, int>`
//       （key 是 const！）。型別不匹配 → 每次迴圈都**複製一份 pair**，
//       而且那個 const& 綁的是暫時物件。改成 `const auto& p` 就完全沒事。
//
//   (b) 型別會跟著重構自動改變。把容器從 vector 換成 deque，
//       所有寫死型別的地方都要改；寫 auto 的地方一行都不用動。
//
//   (c) 有些型別根本寫不出來。lambda 的型別是編譯器產生的匿名型別，
//       只能用 auto 承接。
//
// 【2. auto 推導規則：它會丟掉 const 與參考】
//   `auto` 的推導規則與 template 參數推導相同，會**剝除頂層 const 與參考**：
//       const std::vector<int> cv = {1,2,3};
//       auto        it1 = cv.begin();   // const_iterator（來自 cv 是 const，
//                                       //   begin() 的 const 重載回傳 const_iterator）
//       auto        x   = cv[0];        // int（複製！const 被剝掉）
//       const auto& y   = cv[0];        // const int&（不複製）
//   常見錯誤是 `auto e = vec[i];` 對大型物件造成靜默複製。
//   實務建議：讀取用 `const auto&`，要改用 `auto&`，
//   只有真的需要副本時才用 `auto`。
//
// 【3. 迭代器型別到底叫什麼名字】
//   每個容器都有四個公開的迭代器 typedef：
//       Container::iterator                可讀可寫、正向
//       Container::const_iterator          唯讀、正向
//       Container::reverse_iterator        可讀可寫、反向
//       Container::const_reverse_iterator  唯讀、反向
//   這些名字是**標準保證存在的介面**，但它們**背後別名到什麼型別是實作定義**。
//   本機 libstdc++（g++ 15.2）的 vector<int>::iterator 是
//   `__gnu_cxx::__normal_iterator<int*, std::vector<int>>`；
//   而 map 的是紅黑樹節點迭代器 `std::_Rb_tree_iterator<...>`。
//   所以寫 `int* p = vec.begin();` 不可移植（而且在 libstdc++ 直接編譯失敗）。
//
// 【4. 什麼時候「不該」用 auto】
//   auto 會隱藏型別，有幾個場合反而該寫清楚：
//     * 需要明確的數值型別時：`auto x = 0;` 是 int，
//       但你可能要 std::size_t 或 double。
//     * 代理物件（proxy）陷阱：`std::vector<bool>` 的 operator[] 回傳的是
//       `std::vector<bool>::reference` 代理物件，不是 bool&。
//       `auto b = vb[0];` 得到的是代理物件，若原 vector 被銷毀就變成懸空。
//       這種場合要寫 `bool b = vb[0];` 強制轉換。
//     * 介面邊界（公開 API 的回傳型別）寫清楚有助讀者理解。
//
// 【概念補充 Concept Deep Dive】
//   auto 是純粹的編譯期機制，不是動態型別。編譯器在語意分析階段就把
//   `auto it = vec.begin();` 的 it 定型為具體型別，產生的機器碼與手寫
//   完整型別**完全相同**（可用 `g++ -S` 對照組譯碼驗證）。
//   它與 Python 的動態型別、Java 的 var（同樣是編譯期推導）性質不同於前者、
//   相同於後者。
//   想在編譯期「看到」auto 推導出什麼，有一個常用技巧：宣告一個
//   未定義的 template `template<class T> struct TD;` 然後寫 `TD<decltype(it)> td;`
//   ——編譯錯誤訊息會把完整型別印出來。本檔改用 typeid + c++filt 風格的
//   執行期示範（注意 typeid().name() 的字串內容是**實作定義**）。
//
// 【注意事項 Pay Attention】
//   1. `auto` 不會推導出參考。要參考必須明寫 `auto&` / `const auto&` / `auto&&`。
//   2. `auto` 會剝除頂層 const，對大型物件容易造成非預期的複製。
//   3. `typeid(x).name()` 回傳的字串格式是**實作定義**：GCC/Clang 回傳
//      mangled name（如 `i` 代表 int），MSVC 回傳可讀字串。
//      要看可讀版本需要 abi::__cxa_demangle 或外部 c++filt。
//   4. 從 const 容器呼叫 begin() 得到的是 const_iterator——這是
//      begin() 有 const 與非 const 兩個重載造成的，不是 auto 的功勞。
//   5. 原教材此處宣告了 map_it1 卻沒用到，會觸發
//      -Wunused-but-set-variable 警告；本檔補上實際使用以維持零警告。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】auto 與迭代器型別
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. `for (const std::pair<std::string, int>& p : myMap)` 有什麼效能問題？
//     答：std::map<K,V> 的 value_type 是 `std::pair<const K, V>`，key 帶 const。
//         寫成 `std::pair<std::string,int>` 型別不匹配，於是每次迭代都會
//         **建構一個暫時的 pair 副本**（複製了整個 std::string），
//         const& 綁到那個暫時物件上。迴圈跑 N 次就複製 N 次。
//         正解是 `for (const auto& p : myMap)`。
//     追問：那 C++17 有更好的寫法嗎？
//         → 有，structured bindings：`for (const auto& [name, score] : myMap)`，
//           可讀性更好且同樣零複製。
//
// 🔥 Q2. `std::vector<int>::iterator` 就是 `int*` 嗎？
//     答：不保證，這是**實作定義**。本機 libstdc++ 是
//         `__gnu_cxx::__normal_iterator<int*, std::vector<int>>`——一個把
//         int* 包起來的 class template，目的是型別安全（避免 vector 的
//         迭代器與裸指標互相隱式轉換）與支援 _GLIBCXX_DEBUG 檢查模式。
//         標準只保證它滿足 Random Access Iterator 的需求。
//     追問：那要拿到底層指標怎麼辦？
//         → 用 `vec.data()`（C++11）；或對非空容器用 `&*vec.begin()`。
//           C++20 起 vector 的迭代器保證是 contiguous_iterator，
//           可用 `std::to_address(it)` 通用取得指標。
//
// ⚠️ 陷阱. `std::vector<bool> vb{true}; auto b = vb[0]; vb.clear(); if (b) ...`
//         為什麼可能出事？
//     答：std::vector<bool> 是位元壓縮的特化版，operator[] 回傳的不是
//         `bool&` 而是 `std::vector<bool>::reference` **代理物件**，
//         內部持有指向位元組的指標與位元遮罩。`auto b = vb[0];` 推導出
//         的是那個代理物件而非 bool；clear() 之後代理物件指向的儲存
//         已經失效，再讀取它是 UB。
//     為什麼會錯：直覺認為 `auto` 會給我「值」。對一般容器確實如此，
//         但代理物件的存在打破了這個直覺。凡是可能回傳 proxy 的地方
//         （vector<bool>、部分矩陣函式庫、std::bitset::operator[]），
//         都要明寫目標型別：`bool b = vb[0];` 才會觸發轉換取得真值。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <typeinfo>

int main() {
    // 完整型別宣告（冗長）
    std::vector<int> vec = {1, 2, 3};
    std::vector<int>::iterator it1 = vec.begin();

    // 使用 auto（簡潔）
    auto it2 = vec.begin();

    std::cout << "=== auto 與完整型別完全等價 ===" << std::endl;
    std::cout << "兩者等價：*it1 = " << *it1 << ", *it2 = " << *it2 << std::endl;
    std::cout << "型別相同嗎? "
              << (typeid(it1) == typeid(it2) ? "是（編譯期就是同一型別）" : "否")
              << std::endl;

    // 對於複雜型別，auto 更加重要
    std::map<std::string, int> scores;
    scores["Alice"] = 95;
    scores["Bob"] = 87;

    // 不用 auto，型別非常長
    std::map<std::string, int>::iterator map_it1 = scores.begin();

    // 用 auto，清爽多了
    auto map_it2 = scores.begin();

    std::cout << "\n=== map 迭代器 ===" << std::endl;
    std::cout << "完整型別寫法：" << map_it1->first << " = " << map_it1->second << std::endl;
    std::cout << "auto 寫法    ：" << map_it2->first << " = " << map_it2->second << std::endl;

    // 迭代器型別名稱（mangled name，格式是實作定義）
    std::cout << "\n=== 迭代器實際型別（typeid 字串格式為實作定義）===" << std::endl;
    std::cout << "vector<int>::iterator -> " << typeid(it2).name() << std::endl;
    std::cout << "map 的 iterator       -> " << typeid(map_it2).name() << std::endl;
    std::cout << "（GCC 回傳 mangled name，可用 c++filt -t 還原成可讀型別）" << std::endl;

    // map 的 value_type 帶 const key —— 這是 auto 最重要的實務理由
    std::cout << "\n=== map 的 value_type 是 pair<const K, V> ===" << std::endl;
    std::cout << "value_type 與 pair<string,int> 同型別嗎? "
              << (typeid(std::map<std::string, int>::value_type)
                      == typeid(std::pair<std::string, int>)
                          ? "是"
                          : "否 —— key 帶 const，寫錯會每圈複製一次")
              << std::endl;

    std::cout << "\n寫 const auto& 就不會有複製問題：" << std::endl;
    for (const auto& p : scores) {
        std::cout << "  " << p.first << " => " << p.second << std::endl;
    }

    // C++17 structured bindings：更好讀，一樣零複製
    std::cout << "\n=== C++17 structured bindings ===" << std::endl;
    for (const auto& [name, score] : scores) {
        std::cout << "  " << name << " 得分 " << score << std::endl;
    }

    // auto 會剝除頂層 const 與參考
    std::cout << "\n=== auto 會剝除 const 與參考 ===" << std::endl;
    const std::vector<int> cv = {100, 200, 300};
    auto x = cv[0];          // int（複製，const 被剝除）
    const auto& y = cv[0];   // const int&（不複製）
    std::cout << "auto x        = " << x << "（值複製）" << std::endl;
    std::cout << "const auto& y = " << y << "（參考，無複製）" << std::endl;
    auto cit = cv.begin();   // const 容器 → begin() 的 const 重載 → const_iterator
    std::cout << "從 const 容器取得的迭代器是 const_iterator 嗎? "
              << (typeid(cit) == typeid(std::vector<int>::const_iterator) ? "是" : "否")
              << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第四課：迭代器（Iterator）的核心概念4.cpp" -o iter4

// === 預期輸出 ===
// === auto 與完整型別完全等價 ===
// 兩者等價：*it1 = 1, *it2 = 1
// 型別相同嗎? 是（編譯期就是同一型別）
//
// === map 迭代器 ===
// 完整型別寫法：Alice = 95
// auto 寫法    ：Alice = 95
//
// === 迭代器實際型別（typeid 字串格式為實作定義）===
// vector<int>::iterator -> N9__gnu_cxx17__normal_iteratorIPiSt6vectorIiSaIiEEEE
// map 的 iterator       -> St17_Rb_tree_iteratorISt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEiEE
// （GCC 回傳 mangled name，可用 c++filt -t 還原成可讀型別）
//
// === map 的 value_type 是 pair<const K, V> ===
// value_type 與 pair<string,int> 同型別嗎? 否 —— key 帶 const，寫錯會每圈複製一次
//
// 寫 const auto& 就不會有複製問題：
//   Alice => 95
//   Bob => 87
//
// === C++17 structured bindings ===
//   Alice 得分 95
//   Bob 得分 87
//
// === auto 會剝除 const 與參考 ===
// auto x        = 100（值複製）
// const auto& y = 100（參考，無複製）
// 從 const 容器取得的迭代器是 const_iterator 嗎? 是
