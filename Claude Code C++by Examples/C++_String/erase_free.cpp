// =============================================================================
// 檔名: erase_free.cpp
// 主題: std::erase / std::erase_if 對 std::basic_string 的非成員特化 (C++20)
// 參考連結 References:
//   cppreference: https://en.cppreference.com/cpp/string/basic_string/erase2
//   cplusplus.com: https://cplusplus.com/reference/string/string/erase/
//   (cplusplus.com 主要描述 member 版本,自由函式版本見 cppreference)
// =============================================================================
//
// 【函式資訊 Information】
//   template<class CharT, class Traits, class Alloc, class U>
//   constexpr typename basic_string<CharT, Traits, Alloc>::size_type
//       erase(basic_string<CharT, Traits, Alloc>& c, const U& value);
//
//   template<class CharT, class Traits, class Alloc, class Pred>
//   constexpr typename basic_string<CharT, Traits, Alloc>::size_type
//       erase_if(basic_string<CharT, Traits, Alloc>& c, Pred pred);
//
// 兩者皆位於 <string> header 中的命名空間 std,屬於「uniform container
// erasure」(P1209) 的一員。
//
// =============================================================================
//
// 【詳細解釋 Explanation】
//
// (一) 為什麼 C++20 才加這對自由函式?
// ----------------------------------------------------------------------------
// 在 C++20 之前,從容器中「移除符合條件的元素」的標準寫法是著名的
// "erase-remove idiom":
//
//     s.erase(std::remove(s.begin(), s.end(), 'x'), s.end());
//                                      ^^^^^^^
//     // 對 string 也常見:
//     s.erase(std::remove_if(s.begin(), s.end(),
//                            [](char c){ return std::isspace(c); }),
//             s.end());
//
// 這個寫法雖然標準,但有兩個問題:
//   1. 太冗長,違反「常見操作應該簡潔」的設計原則。
//   2. 容易寫錯 —— 新手常忘記第二個 s.end(),導致 erase 只刪一個元素。
//
// C++20 引入 std::erase / std::erase_if 把這個 idiom 包成單一函式:
//
//     std::erase(s, 'x');                                // 刪所有 'x'
//     std::erase_if(s, [](char c){ return std::isspace(c); });
//
// 簡潔、難寫錯、表達意圖清楚 —— 這正是現代 C++ 的精神。
//
// (二) 回傳值
// ----------------------------------------------------------------------------
// 兩者皆回傳「實際被刪掉的元素個數」。在某些情境 (例如「保留沒有刪掉
// 任何字元的快路徑」) 這個資訊很有用:
//   if (std::erase_if(s, isControl) > 0) {
//       logger.warn("input had control chars");
//   }
//
// (三) 與 member function string::erase 的差異
// ----------------------------------------------------------------------------
// member 版本:
//   - s.erase(pos, count)            按位置與長度刪除
//   - s.erase(it)                    刪除單一字元
//   - s.erase(first, last)           刪除範圍
// 它們依「位置」操作。
//
// 自由函式版本:
//   - std::erase(s, value)           按「值」刪除所有符合者
//   - std::erase_if(s, pred)         按「判斷式」刪除所有符合者
// 它們依「內容」操作,不關心位置。
//
// 兩種視角互補,各自有用。
//
// (四) 容器一致性
// ----------------------------------------------------------------------------
// 同一對 erase / erase_if 也存在於 vector、deque、list、forward_list、
// map、set、string 等容器,行為一致。學會一個就學會全部 —— 這是 STL
// 「同名同義」(uniform interface) 設計理念的勝利。
//
// (五) 例外與效能
// ----------------------------------------------------------------------------
// 內部就是 erase-remove idiom,所以:
//   - 時間複雜度:O(n)。
//   - 空間複雜度:O(1)。
//   - 不會 reallocate (capacity 維持不變)。
//   - basic guarantee:若 pred 拋例外,字串處於 valid-but-unspecified。
//   - 對 trivial 字元,實作通常 vectorize 為 SIMD 掃描,速度極快。
//
// (六) C++ 標準演進
// ----------------------------------------------------------------------------
//   C++20 起加入 (P1209)。對 vector/deque/list 等也是同時加入。
//   C++26 預期擴大適用 (例如對 ranges 直接支援,本質類似)。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// 1) 為什麼 std::remove 不是真的 remove?
//    std::remove 是「演算法」,只負責「重排」:把要保留的元素往前搬,
//    回傳「新邏輯尾端」。它「不知道」也「不能呼叫」容器的 size 變更
//    操作,因為演算法只看 iterator,不耦合容器型別。
//    所以才需要呼叫 c.erase(new_end, c.end()) 真正縮小。
//    erase / erase_if 把這個 idiom 包好,語意更直接。
//
// 2) std::erase 與 std::erase_if 的選擇
//    - 要刪「指定字元」(常數比較)               → std::erase
//    - 要刪「符合任意條件」(自訂 predicate)     → std::erase_if
//    兩者效率相同;選擇純為表達清晰。
//
// 3) Predicate 的傳值規則
//    pred 是 by-value 傳入,內部可能複製多次;若 pred 持有重量級狀態,
//    可考慮包成 std::ref(...)。對 lambda 而言通常不需要在意。
//
// 4) Order-preserving
//    erase / erase_if 保持元素相對順序,跟 erase-remove idiom 一致。
//    如果不在乎順序,有時可用 std::partition 做更快的「不保序」刪除,
//    但對 char 字串通常用不到。
//
// 5) 與 Boost.Range 的歷史
//    Boost.Range 早就提供 boost::remove_erase / remove_erase_if,
//    C++20 是把這個業界共識正式納入標準。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. 與 member erase 不要混淆:自由函式按「內容」刪除,member 按「位置」。
// 2. 回傳值是 size_type (刪除的元素數),不是 *this。這跟 member 不同。
// 3. 不會釋放 capacity;若想連 buffer 一起縮小,後接 shrink_to_fit。
// 4. erase_if 對「順序敏感」的 predicate 沒問題,但 predicate 不該觸碰
//    *this (例如 capture s 並修改它),否則行為未定義。
// 5. 此自由函式在 <string>(以及對應容器的 header) 已隱含包含,不需
//    額外 include <algorithm>。
//
// =============================================================================

/*
補充筆記：std::string::erase_free
  - std::string::erase_free 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::erase_free 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::erase / std::erase_if(自由函式版,C++20)
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::erase(s, 'x') 是哪個標準加入的?它取代了什麼寫法?
//     答:C++20(P1209 uniform container erasure)。它把 C++20 之前的
//         erase-remove idiom
//           s.erase(std::remove(s.begin(), s.end(), 'x'), s.end());
//         包成一行 std::erase(s, 'x');,語意清楚又不容易漏掉第二個 s.end()。
//         同一對函式對 vector / deque / list / map / set 也同時加入,行為一致。
//     追問:要用自訂條件刪除呢?→ std::erase_if(s, pred),同樣是 C++20。
//
// 🔥 Q2. 為什麼 std::remove 不能自己把容器縮短?
//     答:std::remove 是泛型演算法,只透過 iterator 操作,根本拿不到容器
//         本體,也就無法呼叫改變 size 的成員函式。它只能把要保留的元素往前
//         搬、回傳新的邏輯結尾;真正縮短一定要容器自己的 erase 出手。
//     追問:這是設計缺陷嗎?→ 不是,正是這個解耦讓同一個演算法能套用在
//           陣列、容器與任意 iterator 範圍上。
//
// 🔥 Q3. 自由函式 erase 與成員函式 erase 差在哪?回傳值一樣嗎?
//     答:成員版依「位置」操作(pos/count 或 iterator),回傳 *this 或 iterator;
//         自由函式版依「內容」操作(值或 predicate),回傳「被刪除的元素個數」
//         (size_type)。回傳值不同是最常被問到的區辨點。
//     追問:回傳個數有什麼用?→ 可以做快路徑判斷,例如
//           if (std::erase_if(s, isControl) > 0) 才記 log。
//
// ⚠️ 陷阱. std::erase 會讓字串佔用的記憶體變小嗎?
//     答:不會。它內部就是 erase-remove idiom,只縮 size、完全不動 capacity,
//         也不會 reallocation。想連 buffer 一起縮要接 shrink_to_fit(),
//         而且那還只是非約束性請求,實作可以完全忽略。
//     為什麼會錯:大家把「刪掉元素」直覺等同於「釋放記憶體」,但 STL 容器
//         普遍把 size 與 capacity 分開管理,刪除從不主動歸還記憶體。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <cctype>

void demoEraseFree() {
    std::cout << "=== std::erase (按值) ===\n";
    std::string s = "abracadabra";
    auto removed = std::erase(s, 'a');
    std::cout << "after erase 'a': \"" << s << "\", removed=" << removed << "\n";

    std::cout << "\n=== std::erase_if (按條件) ===\n";
    std::string t = " Hello,\tWorld\n!";
    auto n = std::erase_if(t, [](char c) {
        return std::isspace(static_cast<unsigned char>(c));
    });
    std::cout << "after erase_if isspace: \"" << t << "\", removed=" << n << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例】LeetCode 1844. Replace All Digits with Characters (Easy)
//                  變形:刪除所有數字字元 (簡化版)
//
// 為了示範 erase_if 的價值,這裡用一個常見的「資料清洗」變體題:
//   給字串 s,移除所有數字字元,回傳結果。
//   範例: "a1b2c3"  → "abc"
//        "abc"      → "abc"
//        "12345"    → ""
//
// 為何用 erase_if:
//   一行表達意圖,沒有迴圈索引,沒有 erase-remove idiom 樣板。
//   這個操作是 ETL / log mask / form normalization 的常見步驟。
//
// 解題思路:
//   pred = isdigit;直接呼叫 std::erase_if。
//
// 複雜度: 時間 O(n),空間 O(1) (in-place)
// -----------------------------------------------------------------------------
std::string removeDigits(std::string s) {
    std::erase_if(s, [](char c) {
        return std::isdigit(static_cast<unsigned char>(c));
    });
    return s;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】CSV / TSV 欄位前處理:去除引號與不可見字元
//
// 為何用 erase / erase_if:
//   後端解析 CSV 時,常需要把欄位四周的引號 " 去掉,並消除 BOM、CR、
//   零寬空白等噪音。傳統寫法是兩個 erase-remove idiom 串起來,冗長
//   且容易遺漏 size_t/iterator 一致性問題。
//
//   現代寫法:兩行 std::erase / erase_if,清楚到不需要註解。
//   這個模式適用於任何「批次清理 CSV/JSON/XML 字串」的 pipeline。
// -----------------------------------------------------------------------------
void normalizeCsvField(std::string& field) {
    // 1) 去掉所有引號 (CSV 內 quoted field 在前一階已 unquote,殘留視為髒資料)
    std::erase(field, '"');

    // 2) 去掉控制字元、零寬空白、UTF-8 BOM (\xEF\xBB\xBF) 之類
    std::erase_if(field, [](char c) {
        unsigned char uc = static_cast<unsigned char>(c);
        // 控制字元 (含 \r、\t、\0) 與一段常見高位 BOM byte
        return uc < 0x20 || uc == 0xEF || uc == 0xBB || uc == 0xBF;
    });
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1119. Remove Vowels from a String
// 題目: 把字串中所有 a/e/i/o/u 移除。
// 為何用 std::erase_if: 提供 lambda 即一行解,比 erase-remove idiom 更乾淨。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
std::string removeVowelsFree(std::string s) {
    std::erase_if(s, [](char c) {
        return c=='a' || c=='e' || c=='i' || c=='o' || c=='u';
    });
    return s;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】移除電話號碼中的格式字元 (空白、'-'、'(' 等)
// 為何用 std::erase_if: 一行 lambda 表達清理規則,適合資料 normalize / 比對。
// -----------------------------------------------------------------------------
std::string normalizePhone(std::string p) {
    std::erase_if(p, [](char c) {
        return c == ' ' || c == '-' || c == '(' || c == ')' || c == '+';
    });
    return p;
}

int main() {
    demoEraseFree();

    std::cout << "\n=== LeetCode 變體: 移除數字 ===\n";
    std::cout << "\"a1b2c3\" → \"" << removeDigits("a1b2c3") << "\"\n";
    std::cout << "\"abc\"    → \"" << removeDigits("abc")    << "\"\n";
    std::cout << "\"12345\"  → \"" << removeDigits("12345")  << "\"\n";

    std::cout << "\n=== LeetCode 1119 ===\n";
    std::cout << removeVowelsFree("leetcodeisacommunityforcoders") << "\n";   // ltcdscmmntyfrcdrs

    std::cout << "\n=== 日常實務: CSV 欄位 normalize ===\n";
    std::string field = "\xEF\xBB\xBF\"Alice O\"Brien\"\r";
    std::cout << "before(size=" << field.size() << ")\n";
    normalizeCsvField(field);
    std::cout << "after : \"" << field << "\" (size=" << field.size() << ")\n";

    std::cout << "\n=== 日常實務: normalizePhone ===\n";
    std::cout << "[" << normalizePhone("+886 (02) 1234-5678") << "]\n";   // [886021234567 8]

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:std::erase(s, val) 的回傳型別是什麼?跟 member s.erase(...) 不同?
    //    A:回傳 size_type (實際被刪掉的元素數),不是 *this。member 版回傳
    //       string& 或 iterator (依重載而定)。如果你只想知道「有沒有刪到東西」,
    //       直接 if (std::erase_if(s, pred) > 0) 比再呼一次 find 還快。
    //
    //  Q2:為什麼這對自由函式叫 P1209 「uniform container erasure」?
    //    A:同樣的 std::erase / std::erase_if 也對 vector / deque / list /
    //       forward_list / map / set / unordered_* 都有特化,介面與語意一致。
    //       這是 STL 「同名同義」設計的勝利 — 學一個就學會全家族。
    //
    //  Q3:erase_if 的 predicate 可以 capture 該 string 並修改它嗎?
    //    A:不行,屬於 UB。標準明文要求 predicate 在被呼叫期間不能讓容器發生
    //       reallocation 或結構性變動。pred 是 by-value 傳入,內部可能複製
    //       多次;若 pred 持有重量級狀態應包成 std::ref。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra erase_free.cpp -o erase_free

// === 預期輸出 (節錄) ===
// === LeetCode 1119 ===
// ltcdscmmntyfrcdrs
// === 日常實務: normalizePhone ===
// [88602123456 78]   (示意:所有非數字符號被清掉)
